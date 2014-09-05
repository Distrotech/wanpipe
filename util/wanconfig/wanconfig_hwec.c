/*
 *	wanconfig_hwec.c - functions for configuring Hardware Echo Canceller
 *		on Sangoma AFT card. Moved here from wanconfig.c.
 *
 */

#if defined(__WINDOWS__)
# include <windows.h>
# include <stdio.h>
# include <stdlib.h>

# define WEXITSTATUS(status)		status
# define get_active_channels_str	wancfglib_active_channels_bitmap_to_str
# define parse_active_channel		wancfglib_parse_active_channel

int wancfglib_active_channels_bitmap_to_str(unsigned int chan_map, int start_channel, int stop_channel, char* chans_str);
unsigned int wancfglib_parse_active_channel(char *val);

/* executable file under Windows (different from Linux!): */
# define WANEC_CLIENT_NAME	"wanec_client"
/* import functions from waneclib.dll */
# pragma comment( lib, "waneclib" )

#else
# include <stddef.h>
# include <stdlib.h>
# include <stdio.h>
# include <errno.h>
# include <fcntl.h>
# include <string.h>
# include <ctype.h>
# include <sys/stat.h>
# include <sys/ioctl.h>
# include <sys/types.h>
# include <dirent.h>
# include <unistd.h>
# include <sys/socket.h>
# include <netdb.h>
# include <sys/un.h>
# include <sys/wait.h>
# include <unistd.h>
# include <signal.h>
# include <time.h>
# include "lib/safe-read.h"
# include "wanpipe_version.h"
# include "wanpipe_defines.h"
# include "wanpipe_cfg.h"
# include "wanproc.h"
# include "wanpipe.h"

/* executable file under Linux */
# define WANEC_CLIENT_NAME	"wan_ec_client"

#endif

#include "wanconfig.h"
#include "wanpipe_events.h"
#include "wanec_api.h"

#define DBG_WANCONFIG_HWEC	printf("%s():", __FUNCTION__);printf

#if defined(WAN_HWEC)
static int wanconfig_hwec_config(char *devname);
static int wanconfig_hwec_release(char *devname);
static int wanconfig_hwec_enable(char *devname, char *, uint8_t);
static int wanconfig_hwec_modify(char *devname, chan_def_t *def);
static int wanconfig_hwec_bypass(char *devname, chan_def_t *def, int enable);
static int wanconfig_hwec_tone(char *devname, int, int, char *);


#define WAN_EC_PID	"/etc/wanpipe/wan_ec/wan_ec_pid"
#define WAN_EC_DIR	"/etc/wanpipe/wan_ec"

int wanconfig_hwec(chan_def_t *def)
{
	struct link_def	*linkdef = def->link;
	int		err;
	char	devname[100];
	
	/* We never want to enable dsp for non voice apps */
	if (strcmp(def->usedby, "XMTP2_API") == 0 ||
		strcmp(def->usedby, "STACK") == 0 ||
		strcmp(def->usedby, "DATA_API") == 0) {
		def->chanconf->hwec.enable = WANOPT_NO;
		return 0;
	}

	if (linkdef->config_id != WANCONFIG_AFT_TE1 &&
	     linkdef->config_id != WANCONFIG_AFT_ANALOG &&
	     linkdef->config_id != WANCONFIG_AFT_ISDN_BRI) {
		
		/* Do not enalbe hwec for non TE1/ANALOG/BRI cards */
		def->chanconf->hwec.enable = WANOPT_NO;
		linkdef->linkconf->tdmv_conf.hw_dtmf = WANOPT_NO;
		linkdef->linkconf->tdmv_conf.hw_fax_detect = WANOPT_NO;
		return 0;    
	}

	/* If both hwec and global dtmf are set to no then
	   nothing for us to do here */
	if (def->chanconf->hwec.enable != WANOPT_YES &&
	    linkdef->linkconf->tdmv_conf.hw_dtmf != WANOPT_YES) {
		return 0;    
	}

#if defined(__WINDOWS__)
	/* must add 'if1' after 'wanpipe?' */
	wp_snprintf(devname, sizeof(devname), "%s_if1", linkdef->name);
#else
	wp_snprintf(devname, sizeof(devname), "%s", linkdef->name);
#endif

	if ((err = wanconfig_hwec_config(devname))){
		return err;
	}
	

	if ((err = wanconfig_hwec_enable(devname, def->active_ch, linkdef->linkconf->hwec_conf.operation_mode))){
		wanconfig_hwec_release(devname);
		return err;
	}


	if (linkdef->config_id == WANCONFIG_AFT_ISDN_BRI ||
		linkdef->linkconf->hwec_conf.persist_disable) {
		
		if ((err = wanconfig_hwec_bypass(devname, def, 0))){
			wanconfig_hwec_release(devname);
			return err;
		}
	} else {
		
		if ((err = wanconfig_hwec_bypass(devname, def, 1))){
			wanconfig_hwec_release(devname);
			return err;
		}
	}
	
	if ((err = wanconfig_hwec_modify(devname, def))){
		wanconfig_hwec_release(devname);
		return err;
	}
		
	if (linkdef->linkconf->tdmv_conf.hw_dtmf == WANOPT_YES){
		
		err = wanconfig_hwec_tone(devname, 1, WP_API_EVENT_TONE_DTMF, def->active_ch);
		if (err){
			wanconfig_hwec_release(devname);
			return err;		
		}

		if (linkdef->linkconf->tdmv_conf.hw_fax_detect == WANOPT_YES){
				
			err = wanconfig_hwec_tone(devname, 1, WP_API_EVENT_TONE_FAXCALLING, def->active_ch);
			if (err){
				wanconfig_hwec_release(devname);
				return err;		
			}
		}
	} 
	
	/* In this case the hwec was not selected yes in interface
	   section, but dtmf or fax detection was still requested globaly. 
	   Therefore, had to proceed to configure the hwec */
	if (def->chanconf->hwec.enable != WANOPT_YES) {

		if (linkdef->linkconf->tdmv_conf.hw_dtmf == WANOPT_YES && 
			linkdef->linkconf->hwec_conf.dtmf_removal == WANOPT_YES) {
			/* If dtmf_removal has been set then customer wants us to
			   remove dtmf from the media but does not want us to peform
			   echo cancelation. In this case keep the bypass enabled, but
			   set the operation mode to NO_ECHO.  This way media will still
			   flow through the chip but no echo cancelation will occour */
        	err=wanconfig_hwec_enable(devname, def->active_ch, WANOPT_OCT_CHAN_OPERMODE_NO_ECHO);	
			if (err) {
            	wanconfig_hwec_release(devname); 
				return err;
			}
		} else {
            /* In this case no media processing is needed, dtmf can
			   still be obtained via event.  Thus we can proceed to do
			   a full bypass fo the echo canceler */
			wanconfig_hwec_bypass(devname, def, 0);
		}
	}
	
	return 0;
}

static int wanconfig_hwec_config(char *devname)
{
	return wanec_api_config(devname, 0, NULL);
}

static int wanconfig_hwec_release(char *devname)
{
	wanec_api_release_t	release;

	memset(&release, 0, sizeof(wanec_api_release_t));
	return wanec_api_release(devname, 0, &release);
}


static int wanconfig_hwec_modify(char *devname, chan_def_t *def)
{
	int	status, len, i;
	char	cmd[500], *conf_string;
	
#if defined(__LINUX__)
	DIR 	*dir;

	dir = opendir(WAN_EC_DIR);

        if(dir == NULL) {
        	return 0;
        }

	closedir(dir);
#endif

	len = 0;
	for(i = 0; i < def->chanconf->ec_conf.param_no; i++){
		len += (strlen(def->chanconf->ec_conf.params[i].name) +
			strlen(def->chanconf->ec_conf.params[i].sValue) + 5);
	}

	/* Nenad: if no parameters to modify do not run modify */
	if (len == 0) {
		return 0;
	}

	conf_string = malloc(len+1);
	if (conf_string == NULL){
		fprintf(stderr,
		"wanconfig: %s: Failed to allocate memory for EC custom config (len=%d)!\n",
				devname, len+5);
		return -EINVAL;
	}
	memset(conf_string, 0, len+1);
	for(i = 0; i < def->chanconf->ec_conf.param_no; i++){
		sprintf(&conf_string[strlen(conf_string)], "--%s=%s",
				def->chanconf->ec_conf.params[i].name,
				def->chanconf->ec_conf.params[i].sValue);
	}

	if (wp_strcasecmp(def->active_ch, "all") == 0){
		wp_snprintf(cmd, sizeof(cmd), "%s %s modify all %s",
			WANEC_CLIENT_NAME, devname, conf_string);
	}else{
		wp_snprintf(cmd, sizeof(cmd), "%s %s modify %s %s",
			WANEC_CLIENT_NAME, devname, def->active_ch, conf_string);
	}
	
	status = system(cmd);
	if (WEXITSTATUS(status) != 0){
		fprintf(stderr,"--> Error: system command: %s\n",cmd);
		fprintf(stderr,
		"wanconfig: Failed to modify EC device %s (err=%d)!\n",
				devname,WEXITSTATUS(status));
		if (conf_string) free(conf_string);
		return -EINVAL;
	}
	if (conf_string) free(conf_string);
	return 0;
}


static int wanconfig_hwec_bypass(char *devname, chan_def_t *def, int enable)
{
	int	status;
	char cmd[500];
	

	if (wp_strcasecmp(def->active_ch, "all") == 0) {
		char chan_str [20];
		unsigned int tdmv_dchan_map = def->link->linkconf->tdmv_conf.dchan;
		memset(chan_str, 0, sizeof(chan_str));

        /* Disalbe EC on CAS channel */
        if (def->link->linkconf->fe_cfg.media == WAN_MEDIA_E1 &&
            def->link->linkconf->fe_cfg.cfg.te_cfg.sig_mode == WAN_TE1_SIG_CAS) {
            tdmv_dchan_map|=1<<15;
        }

		if (tdmv_dchan_map) {
			if (def->link->linkconf->fe_cfg.media == WAN_MEDIA_T1) {
				get_active_channels_str(~tdmv_dchan_map, 1, 24, chan_str);
			} else {
				get_active_channels_str(~tdmv_dchan_map, 1, 31, chan_str);
			}
			wp_snprintf(cmd, sizeof(cmd), "%s %s %s %s",
				WANEC_CLIENT_NAME, devname, (enable)?"be":"bd", chan_str);
		} else {
			wp_snprintf(cmd, sizeof(cmd), "%s %s %s all",
				WANEC_CLIENT_NAME, devname, (enable)?"be":"bd");
		}
	}else{
		wp_snprintf(cmd, sizeof(cmd), "%s %s %s %s",
					WANEC_CLIENT_NAME,
					devname, 
					(enable)?"be":"bd",
					def->active_ch);
	}

	
	status = system(cmd);
	if (WEXITSTATUS(status) != 0){
		fprintf(stderr,"--> Error: system command: %s\n",cmd);
		fprintf(stderr,
		"wanconfig: Failed to Bypass enable EC device %s (err=%d)!\n",
				devname,WEXITSTATUS(status));
		return -EINVAL;
	}
	return 0;
}

static int
wanconfig_hwec_enable(char *devname, char *channel_list, uint8_t op_mode)
{
# if 1
	wanec_api_opmode_t	opmode;
	unsigned int		fe_chan_map;

	
	fe_chan_map = parse_active_channel(channel_list);
	memset(&opmode, 0, sizeof(wanec_api_opmode_t));

	switch (op_mode) {
	
	case WANOPT_OCT_CHAN_OPERMODE_NORMAL:
    	opmode.mode=WANEC_API_OPMODE_NORMAL;   	 
		break;
	case WANOPT_OCT_CHAN_OPERMODE_SPEECH:
    	opmode.mode=WANEC_API_OPMODE_SPEECH_RECOGNITION;   	 
		break;
	case WANOPT_OCT_CHAN_OPERMODE_NO_ECHO:
    	opmode.mode=WANEC_API_OPMODE_NO_ECHO;   	 
		break;
	default:
    	opmode.mode=WANEC_API_OPMODE_NORMAL;   	 
		break;
	}
		
	opmode.fe_chan_map	= fe_chan_map;
	return wanec_api_opmode(devname, 0, &opmode);
# else
	wanec_api_mode_t	mode;
	unsigned int		fe_chan_map;

	fe_chan_map = parse_active_channel(channel_list);
	memset(&mode, 0, sizeof(wanec_api_mode_t));
	mode.enable	= 1;
	mode.fe_chan_map= fe_chan_map;
	return wanec_api_mode(devname, 0, &mode);
# endif
}


static int
wanconfig_hwec_tone(char *devname, int enable, int id, char *channel_list)
{
	wanec_api_tone_t	tone;
	unsigned int		fe_chan_map;
	int			err;
	
	fe_chan_map = parse_active_channel(channel_list);
	memset(&tone, 0, sizeof(wanec_api_tone_t));
	tone.id			= id;
	tone.enable		= enable;
	tone.fe_chan_map	= fe_chan_map;
	tone.port_map		= WAN_EC_CHANNEL_PORT_SOUT;
	tone.type_map		= WAN_EC_TONE_PRESENT;
	err = wanec_api_tone(devname, 0, &tone);
	return err;
}

#endif /* WAN_HWEC */

int get_active_channels_str(unsigned int chan_map, int start_channel, int stop_channel, char* chans_str)
{
	int i;
	unsigned char enabled = 0;
	unsigned char  first_chan = 0;
	unsigned char last_chan = 0;
	int str_len = 0;
	
	for(i = start_channel; i <= stop_channel; i++) {
		if (chan_map & (1 <<( i-1))) {
			if (!enabled) {
				enabled = 1;
				if (str_len > 0) {
					str_len += sprintf(&chans_str[str_len], ".");
				}
				first_chan = i;
			}
			if (last_chan < i) {
				last_chan = i;
			}
		} else {
			if (enabled) {
				enabled = 0;
				if (last_chan > first_chan) {
					str_len += sprintf(&chans_str[str_len], "%d-%d", first_chan, last_chan);
				} else {
					str_len += sprintf(&chans_str[str_len], "%d", first_chan);
				}
			}
		}
	}
	if (enabled) {
		if (last_chan > first_chan) {
			str_len += sprintf(&chans_str[str_len], "%d-%d", first_chan, last_chan);
		} else {
			str_len += sprintf(&chans_str[str_len], "%d", first_chan);
		}
	}
	return 0;
}
