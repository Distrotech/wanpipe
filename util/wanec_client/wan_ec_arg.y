
%{

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#if defined(__WINDOWS__)
#include "wanpipe_includes.h"
#else
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/queue.h>
#include <sys/ioctl.h>
#endif

#include "wan_ecmain.h"
#include "wanpipe_events.h"
#include "wanpipe_api_iface.h"
#include "wanec_api.h"

extern wanec_client_t		ec_client;
extern wan_custom_conf_t	*custom_conf;
extern int			action;
extern char			yytext[];
extern char			**targv;
extern unsigned 		offset;
static int			start_channel = 0, range = 0;
extern int			gl_err;

extern int yylex(void);
extern int help(int);

long convert_str(char* str, int type);
static int is_channel(unsigned long);
void yyerror(char* msg);

static int wanec_client_param_name(char *key);
static int wanec_client_param_sValue(char*);
static int wanec_client_param_dValue(unsigned int);

%}

%union {
#define YYSTYPE YYSTYPE
	char*	str;
	long	val;
}


%token LOAD_TOKEN
%token UNLOAD_TOKEN
%token CONFIG_TOKEN
%token RELEASE_TOKEN
%token KILL_TOKEN
%token ENABLE_TOKEN
%token DISABLE_TOKEN
%token BYPASS_ENABLE_TOKEN
%token BYPASS_DISABLE_TOKEN
%token MODE_NORMAL_TOKEN
%token MODE_HT_FREEZE_TOKEN
%token MODE_HT_RESET_TOKEN
%token MODE_POWERDOWN_TOKEN
%token MODE_NO_ECHO_TOKEN
%token MODE_SPEECH_RECOGNITION_TOKEN
%token DTMF_ENABLE_TOKEN
%token DTMF_DISABLE_TOKEN
%token FAX_ENABLE_TOKEN
%token FAX_DISABLE_TOKEN
%token STATS_TOKEN
%token STATS_FULL_TOKEN
%token ALL_TOKEN
%token HELP_TOKEN
%token HELP1_TOKEN
%token HWIMAGE_TOKEN
%token MONITOR_TOKEN
%token MONITOR120_TOKEN
%token MODIFY_TOKEN
%token BUFFER_LOAD_TOKEN
%token BUFFER_UNLOAD_TOKEN
%token PLAYOUT_START_TOKEN
%token PLAYOUT_STOP_TOKEN
%token PCM_ULAW_TOKEN
%token PCM_ALAW_TOKEN
%token DURATION_TOKEN
%token REPEAT_TOKEN
%token TEST_TOKEN
%token CUSTOM_PARAM_TOKEN
%token MUTE_TOKEN
%token UNMUTE_TOKEN
%token RIN_PORT_TOKEN
%token ROUT_PORT_TOKEN
%token SIN_PORT_TOKEN
%token SOUT_PORT_TOKEN
%token ALL_PORT_TOKEN

%token CHAR_STRING
%token DEC_CONSTANT
%token HEX_CONSTANT
%token DIAL_STRING

%%

start_args	: TEST_TOKEN
		  { action = WAN_EC_ACT_TEST; }
		| CHAR_STRING
		  { memcpy(ec_client.devname, $<str>1, strlen($<str>1));
		    wanec_api_init(); }
				command
		;

command		: CONFIG_TOKEN custom_param_list
		  { gl_err = wanec_client_config(); }
		| RELEASE_TOKEN
		  { gl_err = wanec_client_release(); }
		| KILL_TOKEN
		  { gl_err = wanec_client_release(); }
		| ENABLE_TOKEN		channel_map
		  { gl_err = wanec_client_mode(1); }
		| DISABLE_TOKEN		channel_map
		  { gl_err = wanec_client_mode(0); }
		| BYPASS_ENABLE_TOKEN	channel_map
		  { gl_err = wanec_client_bypass(1); }
		| BYPASS_DISABLE_TOKEN	channel_map
		  { gl_err = wanec_client_bypass(0); }
		| MODE_NORMAL_TOKEN	channel_map
		  { gl_err = wanec_client_opmode(WANEC_API_OPMODE_NORMAL); }
		| MODE_HT_FREEZE_TOKEN	channel_map
		  { gl_err = wanec_client_opmode(WANEC_API_OPMODE_HT_FREEZE); }
		| MODE_HT_RESET_TOKEN	channel_map
		  { gl_err = wanec_client_opmode(WANEC_API_OPMODE_HT_RESET); }
		| MODE_POWERDOWN_TOKEN	channel_map
		  { gl_err = wanec_client_opmode(WANEC_API_OPMODE_POWER_DOWN); }
		| MODE_NO_ECHO_TOKEN	channel_map
		  { gl_err = wanec_client_opmode(WANEC_API_OPMODE_NO_ECHO); }
		| MODE_SPEECH_RECOGNITION_TOKEN	channel_map
		  { gl_err = wanec_client_opmode(WANEC_API_OPMODE_SPEECH_RECOGNITION); }
		| MODIFY_TOKEN		channel_map custom_param_list
		  { gl_err = wanec_client_modify(); }
		| MUTE_TOKEN		channel_map port_list
		  { gl_err = wanec_client_mute(1); }
		| UNMUTE_TOKEN		channel_map port_list
		  { gl_err = wanec_client_mute(2); }
		| DTMF_ENABLE_TOKEN	channel_map port_list
		  { gl_err = wanec_client_tone(WP_API_EVENT_TONE_DTMF, 1); }
		| DTMF_DISABLE_TOKEN	channel_map port_list
		  { gl_err = wanec_client_tone(WP_API_EVENT_TONE_DTMF, 0); }
		| FAX_ENABLE_TOKEN	channel_map port_list
		  { gl_err = wanec_client_tone(WP_API_EVENT_TONE_FAXCALLING, 1); }
		| FAX_DISABLE_TOKEN	channel_map port_list
		  { gl_err = wanec_client_tone(WP_API_EVENT_TONE_FAXCALLING, 0); }
		| STATS_TOKEN		stats_debug_args
		  { gl_err = wanec_client_stats(0); }
		| STATS_FULL_TOKEN	stats_debug_args
		  { gl_err = wanec_client_stats(1); }
		| HWIMAGE_TOKEN
		  { gl_err = wanec_client_hwimage(); }
		| BUFFER_LOAD_TOKEN	CHAR_STRING 
		  { strcpy(ec_client.filename,$<str>2); }
			buffer_load_args
		  { gl_err = wanec_client_bufferload(); }
		| BUFFER_UNLOAD_TOKEN	DEC_CONSTANT
		  { gl_err = wanec_client_bufferunload($<val>2); }
		| PLAYOUT_START_TOKEN	DEC_CONSTANT
		  { ec_client.fe_chan = $<val>2; }
					DEC_CONSTANT
		  { ec_client.buffer_id = $<val>4; 
		    ec_client.repeat_cnt = 1; }
						playout_start_args
		  { gl_err = wanec_client_playout(1); }
		| PLAYOUT_STOP_TOKEN	DEC_CONSTANT
		  { ec_client.fe_chan = $<val>2; }
					DEC_CONSTANT
		  { ec_client.buffer_id = $<val>4; }
					port
		  { ec_client.port = $<val>6; gl_err = wanec_client_playout(0); }
		| MONITOR_TOKEN		stats_debug_args
		  { gl_err = wanec_client_monitor(16); }
		| MONITOR120_TOKEN	stats_debug_args
		  { gl_err = wanec_client_monitor(120); }
		;

buffer_load_args	: 
			| PCM_ULAW_TOKEN
			  { ec_client.pcmlaw = WAN_EC_PCM_U_LAW; }
			| PCM_ALAW_TOKEN
			  { ec_client.pcmlaw = WAN_EC_PCM_A_LAW; }
			;

playout_start_args	: /* empty */
			| playout_start_args playout_start_arg
			;
		
playout_start_arg	: DURATION_TOKEN DEC_CONSTANT
			  { ec_client.duration = $<val>2; }
			| REPEAT_TOKEN DEC_CONSTANT
			  { ec_client.repeat_cnt = $<val>2; }
			| port
			  { ec_client.port = $<val>1; }
			;
		
channel_map	: ALL_TOKEN
		  { ec_client.fe_chan_map = 0xFFFFFFFF; }
		| channel_list
		;

channel_list	: /* empty */
		  { ec_client.fe_chan_map = 0xFFFFFFFF; } 
		| DEC_CONSTANT
		  { is_channel($<val>1);
		    if (range){
			int	i=0;
			for(i=start_channel;i<=$<val>1;i++){
				ec_client.fe_chan_map |= (1<<i);
			}
			start_channel=0;
		        range = 0;
		    }else{
		  	ec_client.fe_chan_map |= (1 << $<val>1);
		    }
		  }
		| DEC_CONSTANT '-'
		  { is_channel($<val>1);
		    range = 1; start_channel = $<val>1; }
					channel_list
		| DEC_CONSTANT '.'
		  { is_channel($<val>1);
		    if (range){
			int	i=0;
			for(i=start_channel;i<=$<val>1;i++){
				ec_client.fe_chan_map |= (1<<i);
			}
			start_channel=0;
		        range = 0;
		    }else{
		  	ec_client.fe_chan_map |= (1 << $<val>1);
		    }
		  }
					channel_list
		;
		
stats_debug_args:
		  { ec_client.fe_chan = 0; ec_client.fe_chan_map = 0x00000000; }
		| DEC_CONSTANT
		  { is_channel($<val>1);
		    ec_client.fe_chan = $<val>1; ec_client.fe_chan_map = (1<<$<val>1); }
		;

port_list	: ALL_PORT_TOKEN
		  { ec_client.port_map = 
			WAN_EC_CHANNEL_PORT_SIN|WAN_EC_CHANNEL_PORT_SOUT|WAN_EC_CHANNEL_PORT_RIN|WAN_EC_CHANNEL_PORT_ROUT; }
		| ports
		;

ports		: port
		  { ec_client.port_map |= $<val>1; }
		| port
		  { ec_client.port_map |= $<val>1; }
			ports
		;
		
port		: SIN_PORT_TOKEN
		  { $<val>$ = WAN_EC_CHANNEL_PORT_SIN; }
		| SOUT_PORT_TOKEN
		  { $<val>$ = WAN_EC_CHANNEL_PORT_SOUT; }
		| RIN_PORT_TOKEN
		  { $<val>$ = WAN_EC_CHANNEL_PORT_RIN; }
		| ROUT_PORT_TOKEN
		  { $<val>$ = WAN_EC_CHANNEL_PORT_ROUT; }
		;
			
custom_param_list : 
		| CUSTOM_PARAM_TOKEN CHAR_STRING '=' 
		  { wanec_client_param_name($<str>2); }
			custom_param_value custom_param_list
		  ;

custom_param_value : CHAR_STRING 
		     { wanec_client_param_sValue($<str>1); }
                   | DEC_CONSTANT
		     { wanec_client_param_dValue($<val>1); }
                   | '-' DEC_CONSTANT
		     { wanec_client_param_dValue((-1) * $<val>2); }
		   ;
%%

#if 0
custom_param_list:
		| custom_param custom_param_list
		;

custom_param	: CUSTOM_PARAM_TOKEN CHAR_STRING '='
		  { wanec_client_param_name($<str>2); }
			CHAR_STRING
		  { wanec_client_param_value($<str>5); }
		;
#endif

long convert_str(char* str, int type)
{
	long value = 0;
	switch(type){
	case DEC_CONSTANT:
		sscanf(str, "%lu", &value);
		break;
	case HEX_CONSTANT:
		sscanf(str, "%lx", &value);
		break;
	}
	return value;
}

static int is_channel(unsigned long channel)
{
	if (channel > 31){
		printf("ERROR: Channel number %ld is out of range !\n\n",
					channel);
		exit(1);
	}
	return 0;
}

void yyerror(char* msg)
{
	if (!ec_client.verbose){
		printf("> %s\n", msg);
		help (1);
	}else{ 
		printf("> %s (argv=%s,offset=%d)\n", msg, *targv, offset);
	}
	exit(1);
}


static int wanec_client_param_name(char *key)
{
	if (ec_client.conf.param_no == 0){
		ec_client.conf.params = (wan_custom_param_t *)malloc(sizeof(wan_custom_param_t));
		if (ec_client.conf.params == NULL){
			printf("ERROR: Failed to allocate structure for custom configuration!\n");
			return -EINVAL;
		}
		memset(ec_client.conf.params, 0, sizeof(wan_custom_param_t));
	}
	strlcpy(ec_client.conf.params[ec_client.conf.param_no].name, 
					key, MAX_PARAM_LEN); 
	return 0;
}

static int wanec_client_param_sValue(char *sValue)
{
	if (ec_client.conf.params == NULL){
		printf("ERROR: Custom configuration structure is not allocated!\n");
		return -EINVAL;
	} 
	strlcpy(ec_client.conf.params[ec_client.conf.param_no].sValue,
					sValue, MAX_VALUE_LEN); 
	ec_client.conf.param_no++;
	return 0;
}

static int wanec_client_param_dValue(unsigned int dValue)
{
	if (ec_client.conf.params == NULL){
		printf("ERROR: Custom configuration structure is not allocated!\n");
		return -EINVAL;
	} 
	ec_client.conf.params[ec_client.conf.param_no].dValue = dValue;
	ec_client.conf.param_no++;
	return 0;
}
