#ifndef __WAN_ECMAIN_H__
# define __WAN_ECMAIN_H__


#if defined(__LINUX__)
# include <linux/wanpipe_defines.h>
# include <linux/wanpipe_common.h>
# include <linux/if_wanpipe.h>
# include <linux/wanpipe_cfg.h>
#elif defined(__WINDOWS__)
# include <windows.h>
# include <stdio.h>	//printf()
# include <stdlib.h>	//free()
# include <wanpipe_defines.h>
# include <sang_api.h>
# include <wanpipe_cfg.h>
# include <sang_status_defines.h>
# define strlcpy	strncpy
#else
# include <wanpipe_defines.h>
# include <wanpipe_cfg.h>
# include <wanpipe_common.h>
#endif

#define MAX_FILENAME_LEN		100

#define MAX_EC_CLIENT_CHANNELS_LEN	50
#define MAX_EC_CLIENT_PARAM_LEN		50
#define MAX_EC_CLIENT_VALUE_LEN		50

enum {
	WAN_EC_ACT_CMD = 0,
	WAN_EC_ACT_HELP,
	WAN_EC_ACT_HELP1,
	WAN_EC_ACT_TEST
};

typedef struct {
	char		devname[WAN_DRVNAME_SZ+1];
//	unsigned char	if_name[WAN_IFNAME_SZ+1];
	int		verbose;
	
	int		fe_chan;
	unsigned long	fe_chan_map;
//	char		channels[MAX_EC_CLIENT_CHANNELS_LEN];

	char		filename[MAX_FILENAME_LEN];
	unsigned char	pcmlaw;

	unsigned int	buffer_id;
	unsigned int	duration;
	unsigned int	repeat_cnt;

	unsigned char	port;
	unsigned char	port_map;

	wan_custom_conf_t	conf;
} wanec_client_t;


int wan_ec_args_parse_and_run(int argc, char* argv[]);
int wanec_client_config(void);
int wanec_client_release(void);
int wanec_client_mode(int enable);
int wanec_client_bypass(int enable);
int wanec_client_opmode(int mode);
int wanec_client_modify(void);
int wanec_client_mute(int mode);
int wanec_client_tone(int id, int enable);
int wanec_client_stats(int full);
int wanec_client_bufferload(void);
int wanec_client_bufferunload(unsigned long buffer_id);
int wanec_client_playout(int start);
int wanec_client_monitor(int data_mode);
int wanec_client_hwimage(void);

#endif /* __WAN_ECMAIN_H__ */
