/*****************************************************************************
* wanpipemon.c	Sangoma WANPIPE Monitor
*
* Author:       Alex Feldman <al.feldman@sangoma.com>	
* 		Nenad Corbic <ncorbic@sangoma.com>
*
* Copyright:	(c) 1995-2002 Sangoma Technologies Inc.
*
* ----------------------------------------------------------------------------
* Jan 11, 2005  David Rokhvarg  Added code to run above AFT card with protocol
* 				in the LIP layer. Fixed many not working options.
* Aug 02, 2002  Nenad Corbic	Updated all linux utilites and added 
*                               GUI features.
* May 30, 2002	Alex Feldman	Initial version based on ppipemon
*****************************************************************************/

/******************************************************************************
 * 			INCLUDE FILES					      *
 *****************************************************************************/
#if defined(__WINDOWS__)
# include <conio.h>				/* for _kbhit */
# include <stdio.h>				
# include "wanpipe_includes.h"
# include "wanpipe_defines.h"	/* for 'wan_udp_hdr_t' */
# include "wanpipe_time.h"		/* for 'struct timeval' */
# include "wanpipe_common.h"	/* for 'wp_strcasecmp(), wp_unlink()' */
#else
#include <stdio.h>
#include <ctype.h>
#include <errno.h>
#include <time.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#if defined(__LINUX__)
# include <linux/version.h>
# include <linux/types.h>
# include <linux/if_packet.h>
# include <linux/if_wanpipe.h>
# include <linux/if_ether.h>
#else
# include <netinet/in_systm.h>
# include <netinet/in.h>
# include <netinet/ip.h>
# include <netinet/udp.h>  
#endif
#endif

#include "wanpipe_api.h"
#include "fe_lib.h"
#include "wanpipemon.h"

/******************************************************************************
 * 			DEFINES/MACROS					      *
 *****************************************************************************/
#define MDATALEN 2024

#ifndef SIOC_WANPIPE_MONITOR 
#define SIOC_WANPIPE_MONITOR SIOC_WANPIPE_PIPEMON
#endif


/******************************************************************************
 * 			GLOBAL VARIABLES				      *
 *****************************************************************************/
#if defined(CONFIG_PRODUCT_WANPIPE_GENERIC)
char*	progname = "wanutil";
#else
char*	progname = "wanpipemon";
#endif


wan_femedia_t	femedia;
wan_udp_hdr_t	wan_udp;

int 	wan_protocol = 0;
int		get_config_flag = 0;
int		protocol_cb_size = sizeof(wan_mgmt_t)+sizeof(wan_cmd_t)+1;

int 		is_508;
int		gui_interface=0;


extern int trace_hdlc_data(wanpipe_hdlc_engine_t *hdlc_eng, void *data, int len);
extern int get_femedia_type(wan_femedia_t *fe_media);

sng_fd_t sangoma_fd=INVALID_HANDLE_VALUE;
char ipaddress[WAN_IFNAME_SZ+1];

#ifdef __LINUX__
int		sock = 0;
static sa_family_t		af = AF_INET;
static struct sockaddr_in	soin;
#endif

int is_logger_dev = 0;

int  udp_port = 9000;

short dlci_number=16;
char if_name[WAN_IFNAME_SZ+1];
int ip_addr = -1;
int start_xml_router=0;
int stop_xml_router=1;
int raw_data=0;
int trace_all_data=0;
int zap_monitor=0;
int dahdi_monitor=0;
int tdmv_chan = 0;
int lcn_number=0;
unsigned char par_port_A_byte, par_port_B_byte;

char *global_command;
int global_argc;
char** global_argv;
char original_global_if_name[WAN_IFNAME_SZ+1];
int original_global_wan_protocol;
int global_card_type = WANOPT_AFT;
int connected_to_hw_level = 0;

int sys_timestamp=0;
int no_exit=0;
int annexg_trace =0;

int pcap_output=0;
int pcap_prot=0;
int pcap_isdn_network=0;
FILE *pcap_output_file;
char pcap_output_file_name[1024];

FILE *trace_bin_out;
FILE *trace_bin_in;
int  trace_binary=0;

int mtp2_msu_only=0;
int trace_only_diff=0;
int trace_rx_only=0;
int trace_tx_only=0; 

int cmd_timeout=-1;
int led_blink=0;

wanpipe_hdlc_engine_t *rx_hdlc_eng; 

trace_prot_t trace_prot_opt[]={ 
	{"FR", FRAME,107},
	{"LAPB", LAPB, 0},
	{"X25", X25, 200},
	{"PPP", PPP,9},
	{"CHDLC", CHDLC,104},
	{"ETH", ETH, 1},
	{"IP", IP,12},
	{"LAPD", LAPD, 177},
	{"ISDN", LAPD, 177},
	{"MTP2", MTP2, 139},
	{"",-1, 0},
};

trace_prot_t trace_x25_prot_opt[]={
	{"DATA",DATA},
	{"PROT",PROT},
	{"",-1},
};

/* tracing global variables */
unsigned int TRACE_DLCI=0;
unsigned int TRACE_PROTOCOL=ALL_PROT;
unsigned int TRACE_X25_LCN=0;
unsigned char TRACE_X25_OPT=ALL_X25;
char TRACE_EBCDIC=0;
char TRACE_ASCII=0;
char TRACE_HEX=0;

char *cmd[MAX_CMD_ARG];
FT1_LED_STATUS FT1_LED;

/******************************************************************************
 * 			FUNCTION PROTOTYPES				      *
 *****************************************************************************/
#ifdef __WINDOWS__
BOOL sig_end(DWORD dwCtrlType);
#define close(x) CloseConnection(&x)
#else
void sig_end(int signal);
#endif

int		main(int, char**);
static int 	init(int, char**,char*);
static void 	usage(void);
#if !defined(CONFIG_PRODUCT_WANPIPE_GENERIC)
static void 	usage_trace_info(void);
static void 	usage_long(void);
#endif
static int 	GetWANConfig(void);
static char*	GetMasterDevName( void );
#if 0
static void 	sig_handler(int sigint);
#endif
int      	fail=0;
int 		xml_output=0;
//static sa_family_t	get_if_family(char*);

#if defined(CONFIG_PRODUCT_WANPIPE_GENERIC)
struct fun_protocol function_lookup[] = {
	{ WANCONFIG_CHDLC,	"hdlc", CHDLCConfig, CHDLCUsage, CHDLCMain, CHDLCDisableTrace,
		                         CHDLCget_main_menu, CHDLCget_cmd_menu, 
					 CHDLC_read_FT1_status, CHDLC_set_FT1_mode, 2}, 
					 
	{ WANCONFIG_AFT,	"aft",   AFTConfig, AFTUsage, AFTMain, AFTDisableTrace, 
		                         AFTget_main_menu, AFTget_cmd_menu, 
					 NULL,NULL, 2 },
	
	{ WANCONFIG_AFT_TE1,	"aft",   AFTConfig, AFTUsage, AFTMain, AFTDisableTrace, 
		                         AFTget_main_menu, AFTget_cmd_menu, 
					 NULL,NULL, 2 },
	{ WANCONFIG_AFT_T116,	"aft",   AFTConfig, AFTUsage, AFTMain, AFTDisableTrace, 
		                         AFTget_main_menu, AFTget_cmd_menu, 
					 NULL,NULL, 2 },

	{ 0, "N/A", 0, 0, 0, 0,0 }, 
};
#else
struct fun_protocol function_lookup[] = {
#ifndef __WINDOWS__
	{ WANCONFIG_FR,		"fr",    FRConfig, FRUsage, FRMain, FRDisableTrace, 
		                         FRget_main_menu, FRget_cmd_menu, 
					 FR_read_FT1_status, FR_set_FT1_mode, 0 }, 
#if defined(__LINUX__)
	{ WANCONFIG_MFR,	"fr",    FRConfig, FRUsage, FRMain, FRDisableTrace, 
		                         FRget_main_menu, FRget_cmd_menu,
					 FR_read_FT1_status, FR_set_FT1_mode,0 },
#else
	{ WANCONFIG_MFR, 	"fr",  NULL, NULL, NULL, NULL,NULL,NULL,0 }, 
#endif
	{ WANCONFIG_PPP,	"ppp",   PPPConfig, PPPUsage, PPPMain, PPPDisableTrace, 
		                         PPPget_main_menu, PPPget_cmd_menu, 
					 PPP_read_FT1_status, PPP_set_FT1_mode,0 }, 
					 
	{ WANCONFIG_CHDLC,	"chdlc", CHDLCConfig, CHDLCUsage, CHDLCMain, CHDLCDisableTrace,
		                         CHDLCget_main_menu, CHDLCget_cmd_menu, 
					 CHDLC_read_FT1_status, CHDLC_set_FT1_mode, 2}, 
					 
	{ WANCONFIG_ATM,	"atm",   ATMConfig, ATMUsage, ATMMain, ATMDisableTrace, 
		                         ATMget_main_menu, ATMget_cmd_menu, 
					 ATM_read_FT1_status, ATM_set_FT1_mode, 2 },
#endif /* !__WINDOWS__*/
					 
	{ WANCONFIG_AFT,	"aft",   AFTConfig, AFTUsage, AFTMain, AFTDisableTrace, 
		                         AFTget_main_menu, AFTget_cmd_menu, 
					 NULL,NULL, 2 },

	{ WANCONFIG_AFT_TE1,	"aft",   AFTConfig, AFTUsage, AFTMain, AFTDisableTrace, 
		                         AFTget_main_menu, AFTget_cmd_menu, 
					 NULL,NULL, 2 },

	{ WANCONFIG_AFT_56K,	"aft",   AFTConfig, AFTUsage, AFTMain, AFTDisableTrace, 
		                         AFTget_main_menu, AFTget_cmd_menu, 
					 NULL,NULL, 2 },

	{ WANCONFIG_AFT_ISDN_BRI,	"aft",   AFTConfig, AFTUsage, AFTMain, AFTDisableTrace, 
		                         AFTget_main_menu, AFTget_cmd_menu, 
					 NULL,NULL, 2 },

	{ WANCONFIG_AFT_SERIAL,	"aft",   AFTConfig, AFTUsage, AFTMain, AFTDisableTrace, 
		                         AFTget_main_menu, AFTget_cmd_menu, 
					 NULL,NULL, 2 },

	{ WANCONFIG_AFT_ANALOG,	"aft",   AFTConfig, AFTUsage, AFTMain, AFTDisableTrace, 
		                         AFTget_main_menu, AFTget_cmd_menu, 
					 NULL,NULL, 2 },
	{ WANCONFIG_AFT_GSM,	"aft",   AFTConfig, AFTUsage, AFTMain, AFTDisableTrace, 
		                         AFTget_main_menu, AFTget_cmd_menu, 
					 NULL,NULL, 2 },
	{ WANCONFIG_AFT_T116,	"aft",   AFTConfig, AFTUsage, AFTMain, AFTDisableTrace, 
		                         AFTget_main_menu, AFTget_cmd_menu, 
					 NULL,NULL, 2 },
#if defined(CONFIG_PRODUCT_WANPIPE_USB)
         { WANCONFIG_USB_ANALOG, "usb",  USBConfig, USBUsage, USBMain, NULL,
                                         USBget_main_menu, USBget_cmd_menu,
                                         NULL,NULL, 2 },
#endif

#ifdef WANPIPEMON_ZAP
	{ WANCONFIG_ZAP,	"zap",   NULL, ZAPUsage, ZAPMain, NULL, 
		                         ZAPget_main_menu, ZAPget_cmd_menu, 
					 NULL,NULL, 0 },
#endif

	{ WANCONFIG_AFT_TE3,	"aft",   AFTConfig, AFTUsage, AFTMain, AFTDisableTrace, 
		                         AFTget_main_menu, AFTget_cmd_menu, 
					 NULL,NULL, 2 },

#ifndef __WINDOWS__

	{ WANCONFIG_ADSL,	"adsl",  ADSLConfig, ADSLUsage, ADSLMain, ADSLDisableTrace, 
		                         ADSLget_main_menu, ADSLget_cmd_menu, 
					 NULL, NULL, 2 }, 

#if defined(__LINUX__)
	{ WANCONFIG_SS7,	"ss7",   SS7Config, SS7Usage, SS7Main, SS7DisableTrace, 
		                         SS7get_main_menu, SS7get_cmd_menu, 
					 NULL, NULL, 2 },
	{ WANCONFIG_X25, 	"x25",   X25Config, X25Usage, X25Main, X25DisableTrace, 
					 X25get_main_menu, X25get_cmd_menu , 
					 NULL, NULL, 0}, 
	{ WANCONFIG_BITSTRM, "bitstrm",  BITSTRMConfig, BITSTRMUsage, BITSTRMMain, NULL, 
					 BITSTRMget_main_menu, BITSTRMget_cmd_menu , 
					 NULL, NULL, 0},
#else
	{ WANCONFIG_SS7, 	"ss7",  NULL, NULL, NULL, NULL,NULL,NULL,0 }, 
	{ WANCONFIG_X25, 	"x25",  NULL, NULL, NULL, NULL,NULL,NULL,0 }, 
#endif
	{ WANCONFIG_BSC, 	"bsc",  NULL, NULL, NULL, NULL,NULL,NULL,0 }, 
	{ WANCONFIG_HDLC, 	"hdlc", NULL, NULL, NULL, NULL,NULL,NULL,0 }, 

#endif /* !__WINDOWS__*/
	
	{ 0, "N/A", 0, 0, 0, 0,0 }, 
};
#endif

/******************************************************************************
 * 			FUNCTION DEFINITION				      *
 *****************************************************************************/
#ifdef __WINDOWS__
int MakeConnection( void )
{
	WIN_DBG("%s()\n", __FUNCTION__);
	WIN_DBG("if_name: %s\n", if_name);
	
	sangoma_fd = sangoma_open_dev_by_name(if_name);

    if (sangoma_fd == INVALID_HANDLE_VALUE){
		printf("Error: sangoma_open_dev_by_name() failed for %s!!\n", if_name);
		sangoma_fd = 0;
		return WAN_FALSE;
	}

	if (!wp_strcasecmp(if_name, WP_LOGGER_DEV_NAME)){
		is_logger_dev = 1;
		return WAN_TRUE;
	}

	if (!GetWANConfig()){
		printf("Error: Unable to obtain network interface information.\n");
		printf("Make sure the IP and UDP port are correct.\n");
		CloseConnection(&sangoma_fd);
		return WAN_FALSE;
	}   

	return WAN_TRUE;
}

void CloseConnection(sng_fd_t *fd)
{
	sangoma_close(fd);
}

#else

int MakeConnection( void )
{
#ifdef WANPIPEMON_ZAP
	if(zap_monitor == 1 || dahdi_monitor == 1){
		return MakeZapConnection();
	}
#endif
	if (ip_addr == WAN_TRUE){

		sock = socket((af == AF_INET) ? PF_INET : PF_INET6, SOCK_DGRAM, 0);
		if (sock < 0){
			perror("socket");
			return WAN_FALSE;
		} /* if */

		if (af == AF_INET){
			soin.sin_addr.s_addr = inet_addr(ipaddress);
			soin.sin_family = AF_INET;
			soin.sin_port = htons((u_short)udp_port);
			if (soin.sin_addr.s_addr < 0){
	                	printf("Error: Invalid IP address!\n");
        	        	return WAN_FALSE;
        		}
			if (connect( sock, (struct sockaddr *)&soin, sizeof(soin)) != 0){
				perror("connect");	
				printf("Make sure the IP address is correct\n");
				return( WAN_FALSE );
			} /* if */
     		}else{
#ifdef ENABLE_IPV6
			inet_pton(AF_INET6, ipaddress, &soin6.sin6_addr);
			//soin6.sin6_len = sizeof(struct sockaddr_in6);
			soin6.sin6_family = AF_INET6;
			soin6.sin6_scope_id = 0xc;
			soin6.sin6_port = htons((u_short)udp_port);
			if (connect( sock, (struct sockaddr *)&soin6, sizeof(soin6)) != 0){
				perror("connect");
				printf("Make sure the IP address is correct\n");
				return WAN_FALSE;
			} /* if */
#else
			printf("Ipv6 protocol not supported!\n");
			return WAN_FALSE;
#endif

		}
	}else{
			if (sangoma_fd == INVALID_HANDLE_VALUE) {
				int span,chan;
				sangoma_interface_toi(if_name,&span,&chan);
				sangoma_fd = __sangoma_open_api_span_chan(span,chan);
			}

	    	sock = socket((af == AF_INET) ? PF_INET : PF_INET6, SOCK_STREAM, IPPROTO_IP);
	    	if (sock < 0){
				perror("socket");
				return WAN_FALSE;
    		}

	}

	if (!GetWANConfig()){
		printf("Error: Unable to obtain network interface information.\n");
		printf("Make sure the IP and UDP port are correct.\n");
		close(sock);
		return WAN_FALSE;
	}   

	return WAN_TRUE;
} /* MakeConnection */
#endif

static int GetWANConfig( void )
{
	int cnt = 0, err = 0;
	
	if (wan_protocol != 0){
		
	/* If the user specified protocol, do not try to
	* autodetect it.  Old drivers do not have protocol
		* detect option. */
		goto skip_protocol_detect;
	}
	
	/* Get Protocol type */ 
	wan_udp.wan_udphdr_command	= WAN_GET_PROTOCOL;
	wan_udp.wan_udphdr_data_len 	= 0;
	wan_udp.wan_udphdr_return_code 	= WAN_UDP_TIMEOUT_CMD;
	while (DO_COMMAND(wan_udp) < 0 && ++cnt < 4) {
		if (wan_udp.wan_udphdr_return_code == WAN_UDP_TIMEOUT_CMD) {
			printf("Error: Command timeout occurred\n"); 
			return WAN_FALSE;
		}
		if (wan_udp.wan_udphdr_return_code == 0xCC ) 
			return WAN_FALSE;
	}
	
	if (cnt >= 4){
		return WAN_FALSE;
	}
	
	wan_protocol = wan_udp.wan_udphdr_data[0];

#ifdef __WINDOWS__
	printf("wan_protocol: %s (%d)\n", SDLA_DECODE_PROTOCOL(wan_protocol), wan_protocol);
#endif

skip_protocol_detect:
	if(get_config_flag == 1){
#if !defined(CONFIG_PRODUCT_WANPIPE_GENERIC)
		printf("Network interface running %s protocol...\n",
			DECODE_PROT(wan_protocol));
#endif
		
		EXEC_PROT_FUNC(config,wan_protocol,err,());
	}else{
		err = WAN_TRUE;
	}
	return err;
} 

extern char* GetMasterDevName( void )
{
	int cnt = 0;

	wan_udp.wan_udphdr_command	= WAN_GET_MASTER_DEV_NAME;
	wan_udp.wan_udphdr_data_len 	= 0;
	wan_udp.wan_udphdr_return_code 	= WAN_UDP_TIMEOUT_CMD;

	//important! clean the data area before calling the driver
	memset(wan_udp.wan_udphdr_data, 0x00, WAN_IFNAME_SZ+1);
		
	while (DO_COMMAND(wan_udp) < 0 && ++cnt < 4) {
		if (wan_udp.wan_udphdr_return_code == WAN_UDP_TIMEOUT_CMD) {
			printf("Error: Command timeout occurred\n"); 
			return NULL;
		}
		if (wan_udp.wan_udphdr_return_code) {
			//command failed
			return NULL;
		}
	}

	if (cnt >= 4){
		//command failed
		return NULL;
	}
	
	return (char*)wan_udp.wan_udphdr_data;
}

#ifdef __WINDOWS__
wanpipe_hdlc_engine_t *wanpipe_reg_hdlc_engine (void)
{
	WIN_DBG_FUNC_NOT_IMPLEMENTED();
	return NULL;
}

void wanpipe_unreg_hdlc_engine(wanpipe_hdlc_engine_t *hdlc_eng)
{
	WIN_DBG_FUNC_NOT_IMPLEMENTED();
}

char* get_hardware_level_interface_name(char* interface_name)
{
	return interface_name;
}
#else
char* get_hardware_level_interface_name(char* interface_name)
{
	char* tmp_master_dev_name = NULL;
	static char master_dev_name[WAN_IFNAME_SZ+1];
	char found_hardware_level = 0;
	
	//initialize 'master name' with current name, so in case
	//of failure to get the 'maser name' current name will be used.
	strncpy(master_dev_name, interface_name, WAN_IFNAME_SZ);
	master_dev_name[WAN_IFNAME_SZ]='\0'; 	
	
	//save the original values.
	strncpy(original_global_if_name, if_name, WAN_IFNAME_SZ);
	original_global_if_name[WAN_IFNAME_SZ]='\0'; 	
	original_global_wan_protocol = wan_protocol;
	
	while(found_hardware_level == 0){
		
		tmp_master_dev_name = NULL;
		wan_protocol = 0;
		//get_config_flag = 0;
		
		if(interface_name != if_name){
			//on the first iteration 'interface_name' is the same pointer as the global 'if_name'.
			//after the first iteration, use 'tmp_master_dev_name'.
			strncpy(if_name, interface_name, WAN_IFNAME_SZ);
			if_name[WAN_IFNAME_SZ]='\0'; 	
		}
		
		if(MakeConnection() == WAN_FALSE){ 
			close(sock);
			return NULL;
		}
		
		//get_config_flag = 1;
		
		tmp_master_dev_name = GetMasterDevName();
		//if 'tmp_master_dev_name' was NOT changed, we reached
		//the the hardware level.
		if(strlen(tmp_master_dev_name) == 0){
			//printf("Reached Hardware Level!\n");
			found_hardware_level = 1;
			if(wan_protocol == WANCONFIG_CHDLC){
				//on S514 LIP layer runs above CHDLC firmware.
				global_card_type = WANOPT_S51X;
			}else{
				global_card_type = WANOPT_AFT;
			}
		}else{
			strncpy(if_name, tmp_master_dev_name, WAN_IFNAME_SZ+1);
			strncpy(master_dev_name, tmp_master_dev_name, WAN_IFNAME_SZ+1);
		}
		
		close(sock);
		
	}//while()
	
	//restore the original values.
	strncpy(if_name, original_global_if_name, WAN_IFNAME_SZ+1);
	wan_protocol = original_global_wan_protocol;
	
	//and restore the original connection to the driver
	if(MakeConnection() == WAN_FALSE){ 
		close(sock);
		return NULL;
	}
	
	return master_dev_name;
}
#endif

#ifdef __WINDOWS__
int make_hardware_level_connection()
{
	if (MakeConnection() == WAN_FALSE){ 
		if(sangoma_fd != INVALID_HANDLE_VALUE){
			CloseConnection(&sangoma_fd);
		}
		return 1;	
	}
	return 0;
}
#else
int make_hardware_level_connection()
{
	char* tmp = NULL;
	
	if(connected_to_hw_level){
		return 0;	  
	}
	
	get_config_flag = 0;
	
	//save the original values.
	strncpy(original_global_if_name, if_name, WAN_IFNAME_SZ+1);
	original_global_wan_protocol = wan_protocol;
	
	tmp = get_hardware_level_interface_name(if_name);
	if(tmp == NULL){
		printf("Failed to get Hardware Level Interface name!!\n");
		return 1;
	}
	
	strncpy(if_name, tmp, WAN_IFNAME_SZ+1);
	wan_protocol = 0;
	
	//connect to the hardware level
	if (MakeConnection() == WAN_FALSE){ 
		close(sock);
		return 2;	
	}
	
	connected_to_hw_level = 1;
	
	return 0;
}
#endif

#ifdef __WINDOWS__
void cleanup_hardware_level_connection()
{
	WIN_DBG("%s()\n", __FUNCTION__);
	WIN_DBG("if_name: %s: doing nothing!\n", if_name);
	//do nothing
}
#else
void cleanup_hardware_level_connection()
{
	if(!connected_to_hw_level){
		return;
	}
	
	//close what was opened by make_hardware_level_connection()
	close(sock);
	
	//restore the original values.
	strncpy(if_name, original_global_if_name, WAN_IFNAME_SZ+1);
	wan_protocol = original_global_wan_protocol;
	
	connected_to_hw_level = 0;
	
	//and restore the original connection to the driver
	if(MakeConnection() == WAN_FALSE){ 
		close(sock);
		return;
	}
	get_config_flag = 1;
}
#endif

void hw_line_trace(int trace_mode)
{
	int err;

	if(make_hardware_level_connection()){
    		return;
  	}
	//depending on 'wan_protocol' cpipemon or aftpipemon will be called
	EXEC_PROT_FUNC(main, wan_protocol, err,(global_command, global_argc, global_argv));
	EXEC_PROT_VOID_FUNC(disable_trace,wan_protocol,());
	cleanup_hardware_level_connection();
}

void hw_modem( void )
{
	int err;

	if(make_hardware_level_connection()){
    		return;
  	}
	//depending on 'wan_protocol' cpipemon or aftpipemon will be called
	EXEC_PROT_FUNC(main, wan_protocol, err,(global_command, global_argc, global_argv));
	cleanup_hardware_level_connection();
}

void hw_general_stats( void ) 
{
	int err;
	
	if(make_hardware_level_connection()){
    		return;
  	}
	global_command = "so"; //run operational_stats() - at the hardware layer
	//depending on 'wan_protocol' cpipemon or aftpipemon will be called
	EXEC_PROT_FUNC(main, wan_protocol, err,(global_command, global_argc, global_argv));
	cleanup_hardware_level_connection();
}

void hw_flush_general_stats( void ) 
{
	int err;
	if(make_hardware_level_connection()){
    		return;
  	}
	global_command = "fo"; //run flush_operational_stats() - at the hardware layer
	//depending on 'wan_protocol' cpipemon or aftpipemon will be called
	EXEC_PROT_FUNC(main, wan_protocol, err,(global_command, global_argc, global_argv));
	cleanup_hardware_level_connection();
}

void hw_comm_err( void ) 
{
	int err;
	if(make_hardware_level_connection()){
    		return;
  	}
	global_command = "sc"; //run comm_err_stats() - at the hardware layer
	//depending on 'wan_protocol' cpipemon or aftpipemon will be called
	EXEC_PROT_FUNC(main, wan_protocol, err,(global_command, global_argc, global_argv));
	cleanup_hardware_level_connection();
}

void hw_flush_comm_err( void ) 
{
	int err;

	if(make_hardware_level_connection()){
    		return;
  	}
	global_command = "fc"; //run flush_comm_err_stats() - at the hardware layer
	//depending on 'wan_protocol' cpipemon or aftpipemon will be called
	EXEC_PROT_FUNC(main, wan_protocol, err,(global_command, global_argc, global_argv));
	cleanup_hardware_level_connection();
}

void hw_router_up_time()
{
	int err;
	
	if(make_hardware_level_connection()){
    		return;
  	}
		
	global_command = "xru"; //run aft_router_up_time() - at the hardware layer
	//depending on 'wan_protocol' cpipemon or aftpipemon will be called
	EXEC_PROT_FUNC(main, wan_protocol, err,(global_command, global_argc, global_argv));

	cleanup_hardware_level_connection();
}

void print_router_up_time(u_int32_t time)
{
	u_int32_t minutes;
	u_int32_t seconds;
	
     	if (time < 3600) {
		if (time<60){
     			printf("    Router UP Time:  %u seconds\n", time);
		}else{
     			printf("    Router UP Time:  %u minute(s),  %u seconds\n",
			       	time/60, time%60);
		}
     	}else{
		minutes = (time%3600) / 60;
		seconds = (time%3600) % 60;
			
     		printf("    Router UP Time:  %u hour(s), %u minute(s),  %u seconds.\n",
			time/3600, minutes, seconds);
	}
}

void hw_read_code_version()
{
	int err;
	
	if(make_hardware_level_connection()){
    		return;
  	}
		
	global_command = "xcv"; //run aft_router_up_time() - at the hardware layer
	//depending on 'wan_protocol' cpipemon or aftpipemon will be called
	EXEC_PROT_FUNC(main, wan_protocol, err,(global_command, global_argc, global_argv));

	cleanup_hardware_level_connection();
}

void hw_link_status()
{
	int err;
	
	if(make_hardware_level_connection()){
    		return;
  	}
		
	global_command = "xl"; //run aft_router_up_time() - at the hardware layer
	//depending on 'wan_protocol' cpipemon or aftpipemon will be called
	EXEC_PROT_FUNC(main, wan_protocol, err,(global_command, global_argc, global_argv));

	cleanup_hardware_level_connection();
}

#ifdef __WINDOWS__

int DoCommand(wan_udp_hdr_t* wan_udp)
{
	/* call libsangoma */
	return sangoma_mgmt_cmd(sangoma_fd, wan_udp);
}

#define inet_aton(a,b)	0

#else
int DoCommand(wan_udp_hdr_t* wan_udp)
{
        static unsigned char id = 0;
        fd_set ready;
        struct timeval to;
	int data_len = protocol_cb_size+wan_udp->wan_udphdr_data_len;
        int x, err=0;

	if (sangoma_fd != INVALID_HANDLE_VALUE) {
     	return sangoma_mgmt_cmd(sangoma_fd, wan_udp); 	
	}

	if (ip_addr == WAN_TRUE){
		for( x = 0; x < 4; x += 1 ) {
			wan_udp->wan_udphdr_request_reply = 0x01;
			wan_udp->wan_udphdr_id = id;
			/* 0xAA is our special return code indicating packet timeout */
			//wan_udp.wan_udphdr_return_code = WAN_UDP_TIMEOUT_CMD;
			//err = send(sock, &wan_udp. 35 + cb.cblock.buffer_length, 0);
			err = send(sock, (caddr_t)wan_udp, data_len, 0); 
			if (err < 0){
				perror("Send: ");	
				continue;
			}
			
			FD_ZERO(&ready);
			FD_SET(sock,&ready);
			to.tv_sec = 5;
			to.tv_usec = 0;
	
			if((err = select(sock + 1,&ready, NULL, NULL, &to))){
				err = recv(sock, (caddr_t)wan_udp, sizeof(wan_udp_hdr_t), 0);
				if (err < 0){
					perror("Recv: ");
				}
        	               	break;
	               	}else{
				printf("Error: No Data received from the connected socket.\n");
               		}
		}
	}else{
		struct ifreq ifr;
		wan_udp->wan_udphdr_request_reply = 0x01;
		wan_udp->wan_udphdr_id = id;
		strncpy(ifr.ifr_name, if_name, sizeof(ifr.ifr_name));
		ifr.ifr_data = (caddr_t)wan_udp;

        	if ((err=ioctl(sock, SIOC_WANPIPE_MONITOR , &ifr)) < 0){
			if (errno != EBUSY){
				perror("Ioctl: ");
				exit(1);
			}
			return err;
    		}
	}

#if 0
	{
		int i;
		printf("Rx Packet :\n");
		for (i=0;i<sizeof(wan_udp);i++){
			printf("%x ",*((unsigned char *)wan_udp+i));
		}
		printf("\n");
	}
#endif
        return err;

} /* DoCommand */
#endif

static int init(int argc, char *argv[], char* command)
{
	int i = 0, i_cnt = 0, u_cnt = 0, c_cnt = 0, d_cnt = 0;
	struct in_addr *ip_str = NULL;
#ifdef ENABLE_IPV6
	struct in6_addr ip6_str;
#endif


	for (i = 1; i < argc; i++){
		if (!strcmp(argv[i],"-h")){
#ifdef __WINDOWS__
			EXEC_NAME_FUNC(usage,"aft",());
#else
#if defined(CONFIG_PRODUCT_WANPIPE_GENERIC)
			EXEC_NAME_FUNC(usage,"hdlc",());
#else
			usage_long();
#endif
#endif
			return WAN_FALSE;

#if !defined(CONFIG_PRODUCT_WANPIPE_GENERIC)
		}else if (!strcmp(argv[i],"-traceinfo")){
			usage_trace_info();
			return WAN_FALSE;
#endif			
		}else if (!strcmp(argv[i],"-i")){
			if (i+1 > argc-1){
				printf("ERROR: Invalid IP or Interface Name!\n");
				printf("Type wanpipemon <cr> for help\n");
				return WAN_FALSE;
			}

			strncpy(ipaddress,argv[i+1], WAN_IFNAME_SZ+1);
			if (inet_aton(ipaddress,ip_str) != 0 ){
				ip_addr = WAN_TRUE;
			}else{
#ifdef ENABLE_IPV6
				if (inet_pton(AF_INET6,ipaddress,(void*)&ip6_str) == 1){
					af = AF_INET6;
					ip_addr = WAN_TRUE;
				}else{		
					ip_addr = WAN_FALSE;
					strncpy(if_name, ipaddress, WAN_IFNAME_SZ);
				}
#else
				ip_addr = WAN_FALSE;
				strncpy(if_name, ipaddress, WAN_IFNAME_SZ);
#endif
			}
			i_cnt=1;

		}else if (!strcmp(argv[i],"-g")){
#ifdef WANPIPEMON_GUI			
			gui_interface=1;
			trace_all_data=1;
			return WAN_TRUE;
#else
			printf("wanpipemon: Warning GUI interface not compiled in!\n\n");
#endif	

		}else if (!strcmp(argv[i],"-zap")){

			zap_monitor=1;
			wan_protocol = WANCONFIG_ZAP;

		}else if (!strcmp(argv[i],"-dahdi")){

			dahdi_monitor=1;
			wan_protocol = WANCONFIG_ZAP;

		}else if (!strcmp(argv[i],"-tdmvchan") || !strcmp(argv[i],"-chan")){
			if (i+1 > argc-1){
				printf("ERROR: No Zap channel specified! i.e. '-zapchan 1'\n");
				return WAN_FALSE;
			}
			if(isdigit(argv[i+1][0]) == 0 || (tdmv_chan = atoi(argv[i+1])) == 0){
				printf("ERROR: Invalid -tdmvchan input: %s! Has to be a number greater than zero.\n",
					argv[i+1]);
				return WAN_FALSE;
			}
		}else if (!strcmp(argv[i],"-u")){

			if (i+1 > argc-1){
				printf("ERROR: Invalid UDP PORT!\n");
				printf("Type wanpipemon <cr> for help\n");
				return WAN_FALSE;
			}

		 	if(isdigit(argv[i+1][0])){
				udp_port = atoi(argv[i+1]);
			}else{
				printf("ERROR: Invalid UDP Port!\n");
				printf("Type wanpipemon <cr> for help\n");
				return WAN_FALSE;
			}
			u_cnt=1;
		}else if (!strcmp(argv[i],"-c")){

			if (i+1 > argc-1){
				printf("ERROR: Invalid Command!\n");
				printf("Type wanpipemon <cr> for help\n");
				return WAN_FALSE;
			}

			memset(command, 0, MAX_CMD_LENGTH+1);
			strncpy(command,argv[i+1], strlen(argv[i+1]));
			//strncpy(command,argv[i+1], MAX_CMD_LENGTH);
			//command[MAX_CMD_LENGTH]='\0';	
			c_cnt=1;
		}else if (!strcmp(argv[i], "-d")){
			TRACE_DLCI=dlci_number=16;
			d_cnt=1;
		}else if (!strcmp(argv[i], "-timeout")){
			if (i+1 > argc-1){
				printf("ERROR: Invalid Command!\n");
				printf("Type wanpipemon <cr> for help\n");
				return WAN_FALSE;
			}
			cmd_timeout=atoi(argv[i+1]);
			printf("cmd timeout=%d\n",cmd_timeout);
			i++;
#ifndef __WINDOWS__
		}else if (!strcmp(argv[i],"-pv6")){
			af = AF_INET6;	// IPV6
#endif
		}else if (!strcmp(argv[i],"-p")){

			if (i+1 > argc-1){
				printf("ERROR: Invalid Command!\n");
				printf("Type wanpipemon <cr> for help\n");
				return WAN_FALSE;
			}

			if (argc == 3){
				char *name=argv[i+1];
				EXEC_NAME_FUNC(usage,name,());
				return WAN_FALSE;
			}

			if (strcmp(argv[i+1], "chdlc") == 0){
				wp_strlcpy((char*)wan_udp.wan_udphdr_signature,
					UDP_CHDLC_SIGNATURE,
					strlen(wan_udp.wan_udphdr_signature));
				wan_protocol=WANCONFIG_CHDLC;
			}else if (strcmp(argv[i+1], "fr") == 0){
				wp_strlcpy((char*)wan_udp.wan_udphdr_signature, 
					UDP_FR_SIGNATURE,
					strlen(wan_udp.wan_udphdr_signature));
				wan_protocol=WANCONFIG_FR;
			}else if (strcmp(argv[i+1], "ppp") == 0){
				wp_strlcpy((char*)wan_udp.wan_udphdr_signature, 
					UDP_PPP_SIGNATURE,
					strlen(wan_udp.wan_udphdr_signature));
				wan_protocol=WANCONFIG_PPP;
			}else if (strcmp(argv[i+1], "x25") == 0){
				wp_strlcpy((char*)wan_udp.wan_udphdr_signature, 
					UDP_X25_SIGNATURE,
					strlen(wan_udp.wan_udphdr_signature));
				wan_protocol=WANCONFIG_X25;
			}else if (strcmp(argv[i+1], "adsl") == 0){
				wp_strlcpy((char*)wan_udp.wan_udphdr_signature, 
					GLOBAL_UDP_SIGNATURE,
					strlen(wan_udp.wan_udphdr_signature));
				wan_protocol=WANCONFIG_ADSL;
			}else{
				usage();
				return WAN_FALSE;
			}		
		
		}else if (!strcmp(argv[i], "-x")){
			xml_output=1;

		}else if (!strcmp(argv[i], "-full")){
			trace_all_data=1;
		
		}else if (!strcmp(argv[i], "-systime")){
			sys_timestamp=1;
		}else if (!strcmp(argv[i], "-noexit")){
			no_exit=1;
		}else if (!strcmp(argv[i], "-mtp2-msu")){
			mtp2_msu_only=1;
			printf("MTP2 Trace MSU Only\n"); 
   		}else if (!strcmp(argv[i], "-diff")){
			trace_only_diff=1;
			printf("Trace Diff Only\n"); 
		}else if (!strcmp(argv[i], "-rx")){
			trace_rx_only=1;
			printf("Trace Rx Only\n"); 
		}else if (!strcmp(argv[i], "-tx")){
			trace_tx_only=1;
			printf("Trace Tx Only\n"); 

		}else if (!strcmp(argv[i], "-7bit-hdlc")){
			if (rx_hdlc_eng) {
				rx_hdlc_eng->seven_bit_hdlc = 1;	
				printf("Enabling 7bit HDLC\n");
			}

		}else if (!strcmp(argv[i], "-prot")){
			int x;
			if (i+1 > argc-1){
				printf("ERROR: Invalid Command -prot\n");
				printf("ERROR: Type %s <cr> for help\n\n",
						progname);
				exit(1);
			}
			TRACE_PROTOCOL=0;
			for (x=0;;x++){
				if (trace_prot_opt[x].prot_index == -1)
					break;
			
				if (strstr(argv[i+1],(char*)trace_prot_opt[x].prot_name) != NULL){
					TRACE_PROTOCOL|=trace_prot_opt[x].prot_index;
					pcap_prot=trace_prot_opt[x].pcap_prot;
				}
			}

			if (!TRACE_PROTOCOL){
				printf("ERROR: Invalid Protocol in Command -prot!\n");
				printf("ERROR: Type %s <cr> for help\n\n",
						progname);
				exit(1);
			}
		}else if (!strcmp(argv[i], "-lcn")){
			if (i+1 > argc-1){
				printf("ERROR: Invalid Command -lcn, Type %s <cr> for help\n\n", progname);
				exit(1);
			}

			if (isdigit(argv[i+1][0])){
				TRACE_X25_LCN=atoi(argv[i+1]);
			}else{
				printf("ERROR: LCN must be an integer,Type %s <cr> for help\n\n", progname);
				exit(0);
			} 

		}else if (!strcmp(argv[i], "-ascii")){

			TRACE_ASCII=1;
			
		}else if (!strcmp(argv[i], "-ebcdic")){

			TRACE_EBCDIC=1;

		}else if (!strcmp(argv[i], "-hex")){

			TRACE_HEX=1;

		}else if (!strcmp(argv[i], "-pcap")){

			pcap_output=1;

		}else if (!strcmp(argv[i], "-trace_bin")){

			trace_binary=1;
		
		}else if (!strcmp(argv[i], "-pcap_isdn_network")) {

			pcap_isdn_network=1;
		
		}else if (!strcmp(argv[i], "-pcap_file")){	
			
			if (i+1 > argc-1){
				printf("ERROR: Invalid Command -pcap_file, Type %s <cr> for help\n\n", progname);
				exit(1);
			}

			snprintf(pcap_output_file_name, sizeof(pcap_output_file_name), argv[i+1]);
		}else if (!strcmp(argv[i], "-x25opt")){
			int x;
			if (i+1 > argc-1){
				printf("ERROR: Invalid Command -x25prot, Type %s <cr> for help\n\n", progname);
				exit(1);
			}
		
			TRACE_X25_OPT=0;
			for (x=0;;x++){
				if (trace_x25_prot_opt[x].prot_index == -1)
					break;
				
				if (strstr(argv[i+1],(char*)trace_x25_prot_opt[x].prot_name) != NULL){
					TRACE_X25_OPT|=trace_x25_prot_opt[x].prot_index;
				}
			}

			if (!TRACE_X25_OPT){
				printf("ERROR: Invalid X25 Option in Command -x25opt, Type %s <cr> for help\n\n",
					       progname);
				exit(1);
			}
		} 
	}

	if (zap_monitor == 1 || dahdi_monitor == 1){
#ifdef WANPIPEMON_ZAP
		return WAN_TRUE;
#else
		printf("ERROR: Wanpipe Spike Test is not enabled!\n");
		printf("\n"); 
		return WAN_FALSE;
#endif
	}
	
	if (!i_cnt){
		printf("ERROR: No IP address or Interface Name!\n");
		printf("Type %s <cr> for help\n", progname);
		return WAN_FALSE;
	}
	if (!u_cnt && ip_addr == WAN_TRUE){
		printf("ERROR: No UDP Port!\n");
		printf("Type %s <cr> for help\n", progname);
		return WAN_FALSE;
	}
	if (!c_cnt){
		printf("ERROR: No Command!\n");
		printf("Type %s <cr> for help\n", progname);
		return WAN_FALSE;
	}
	if (!d_cnt){
		/* Default DLCI Number to 16 */
		dlci_number = 16;
	}

	return WAN_TRUE;
}

void ResetWanUdp(wan_udp_hdr_t *wan)
{
	memset((void *)&wan->wan_cmd, 0, sizeof(wan_cmd_t));
} 

void banner (char *title,int dlci_number){
	
	int len,i;
	
	len = strlen(title) + strlen(if_name);
	printf("\n\t");
	for (i=0;i<(len+16);i++)
		printf("-");
	printf("\n\t\t%s: %s", if_name, title);
	printf("\n\t");
	for (i=0;i<(len+16);i++)
		printf("-");
	printf("\n\n");

}

#if !defined(CONFIG_PRODUCT_WANPIPE_GENERIC)
static unsigned char usage_info[]="\n"
"Wanpipemon Verison 1.0\n"
"Copyright (C) 2002 - Sangoma Technologies\n"
"www.sangoma.com\n"
"This may be freely redistributed under the terms of the GNU GPL\n"
"	\n"
"Usage: 	(Options in {} brackets are optional)\n"
"\n"
"	#Command prompt Local debugging\n"
"	wanpipemon -i <interface> -c <command> { -p <protocol> }\n"
"	\n"
"	#Command prompt Remote debugging\n"
"	wanpipemon -i <remote IP> -u <port> -c <command> { -p <protocol> }\n"
"\n"
"	#GUI interface\n"
"	wanpipemon -g { -i <interface|IP> -u <port> -p <protocol> }  \n"
"	\n"
"	#Display usage for each protocol\n"
"	wanpipemon -p [fr|ppp|chdlc|x25|adsl|aft]			\n"
"\n"
"	#Detailed usage for each option\n"
"	wanpipemon -h\n"
"\n"
"	#Detailed usage for advanced tracing\n"
"	wanpipemon -traceinfo\n";
#else
static unsigned char usage_info[]="\n"
"Wanutil Verison 1.0\n"
"Copyright (C) 2004 - Sangoma Technologies\n"
"www.sangoma.com\n"
"This may be freely redistributed under the terms of the GNU GPL\n"
"	\n"
"Usage: 	(Options in {} brackets are optional)\n"
"\n"
"	#Command prompt Local debugging\n"
"	wanutil -i <interface> -c <command>\n"
"	\n"
"	#Command prompt Remote debugging\n"
"	wanutil -i <remote IP> -u <port> -c <command>\n"
"\n"
"	#Detailed usage for each option\n"
"	wanutil -h\n"
"\n"
"	#Detailed usage for advanced tracing\n"
"	wanutil -traceinfo\n";
#endif

static void usage(void)
{
#ifdef __WINDOWS__
	EXEC_NAME_FUNC(usage,"aft",());
#else
	printf("%s\n", usage_info);
#endif
}


#if !defined(CONFIG_PRODUCT_WANPIPE_GENERIC)
static char usage_long_buf[]= "\n"
"\n"
"Wanpipemon Verison 1.0\n"
"Copyright (C) 2002 - Sangoma Technologies\n"
"www.sangoma.com\n"
"This may be freely redistributed under the terms of the GNU GPL\n"
"\n"
"Usage: 	(Options in {} brackets are optional)\n"
"\n"
"	#Command prompt Local debugging\n"
"	wanpipemon -i <interface> -c <command> { -p <protocol> }\n"
"	\n"
"	#Command prompt Remote debugging\n"
"	wanpipemon -i <remote IP> -u <port> -c <command> { -p <protocol> }\n"
"\n"
"	#GUI interface\n"
"	wanpipemon -g { -i <interface|IP> -u <port> -p <protocol> }  \n"
"	\n"
"	#Display usage for each protocol\n"
"	wanpipemon -p [fr|ppp|chdlc|x25|adsl|aft]			\n"
"\n"
"	#Detailed usage for each option\n"
"	wanpipemon -h\n"
"\n"
"-----------------------------------------------------------\n"
"Option -p: \n"
"\n"
"    wanpipemon -p [fr|ppp|chdlc|x25|adsl|aft]\n"
"\n"
"	When used by itself:\n"
"		Displays the command list for each protocol\n"
"\n"
"    wanpipemon -p fr -i wp1_fr16 -c xm	\n"
"	\n"
"	When used with other commands:\n"
"		Specifies the protocol and skips the protocol\n"
"		autodetect feature.  \n"
"		\n"
"		Used for backward compatibility, since\n"
"		old drivers do not support protocol detect option.\n"
"\n"
"-----------------------------------------------------------		\n"
"Option 	-g:\n"
"\n"
"   Start wanpipemon in GUI mode\n"
"\n"
"   wanpipemon -g\n"
"   \n"
"	For local debugging: \n"
"		GUI will display list of active interfaces.\n"
"			\n"
"	For remote debugging:\n"
"		GUI will ask for an IP address and UDP port.\n"
"	\n"
"   wanpipemon -i wp1_fr16 -g\n"
"   \n"
"   	Specifies the interface, do not show the list of interfaces\n"
"\n"
"   wanpipemon -p fr -i wp1_fr16 -g\n"
"        \n"
"   	Specify the interface and protocol. \n"
"	\n"
"-----------------------------------------------------------\n"
"Option -i: \n"
"\n"
"   wanpipemon -i wp1_fr16\n"
"   wanpipemon -i 192.168.1.1\n"
"\n"
"	Specify the Local interface name or Remote IP\n"
"	address which will be used to connect to the driver.\n"
"   \n"
"	\n"
"-----------------------------------------------------------\n"
"Option -u: \n"
"\n"
"   wanpiemon -i 192.168.1.1 -u 9000 -c xm\n"
"\n"
"   	Wanpipe UDPPORT specified in /etc/wanpipe/wanpipe?.conf\n"
"	Note: This option is only valid when using Remote IP address.\n"
"\n"
"\n"
"-----------------------------------------------------------\n"
"Option -c: \n"
"\n"
"	Wanpipe monitor command (for list of command per protocol)\n"
"	For list of commands run: \n"
"		wanpipemon -p [fr|ppp|chdlc|x25|adsl|aft]\n"
"\n"
"\n";

static void usage_long(void)
{
	printf("%s\n",usage_long_buf);
}
#endif


#if !defined(CONFIG_PRODUCT_WANPIPE_GENERIC)
static unsigned char trace_info[]="\n"
"\n"
"Wanpipemon Verison 1.1\n"
"Copyright (C) 2004 - Sangoma Technologies\n"
"www.sangoma.com\n"
"This may be freely redistributed under the terms of the GNU GPL\n"
"	\n"
"Advanced Line Trace Options\n"
"===========================\n"
"\n"
"Advanced trace options include protocol decoding\n"
"and parsing, system timestamping as well as\n"
"packet filtering.\n"
"\n"
"The Advanced trace command is: 'tri'\n"
"	\n"
"	wanpipemon -i <ifname> -c tri { trace options }\n"
"\n"
"\n"
"trace options:\n"
"-------------	\n"
"\n"
"	-prot 	[FR|LAPB|X25]	#Filter packets based on\n"
"				#protocol. Multiple protocols can\n"
"				#be selected:  \n"
"				# <prot>-<prot>...\n"
"				# eg: -prot LAPB-X25\n"
"				#Default: All frames\n"
"\n"
"		[ISDN|MTP2|FR|PPP|CHDLC|IP|ETH|LAPB|X25]\n"
"				#Also used by -pcap option to\n"
"				#specify what protocol we are\n"
"				#capturing. By default protocol is\n"
"				#autodetected, but in datascoping\n"
"				#this option is a must.\n"
"\n"
"	-pcap\n"
"				#Trace to a pcap type file\n"
"				#that can be read by Ethereal\n"
"				#By default file name is wp_trace_pcap.bin\n"
"				#writen in current/local directory\n"
"\n"
"	-pcap_file <filename>\n"
"\n"
"				#Specify your own pcap file name\n"
"\n"
"	-x25opt [DATA|PROT]	#Filter x25 packets based on \n"
"				#protcol or information frames\n"
"				#Default: All frames\n"
"\n"
"	-lcn	<number>	#Filter x25 packets based on\n"
"	                        #specific lcn number\n"
"	                        #Default: All lcns\n"
"\n"
"	-hex			#Display packet info in HEX \n"
"                                #Default: Hex\n"
"\n"
"	-ascii			#Display packet info in ASCII\n"
"\n"
"	-ebcdic			#Display packet info in EBCDIC	\n"
"\n"
"	-systime		#Display timestamp as system time\n"
"	                        #instead of absolute number\n"
"\n"
"	-mtp2-msu		#Trace MTP2 MSU Only layer\n"
"\n"
"	-7bit-hdlc		#Decode hdlc stream as 7bit instead of 8bit\n"
"\n"
"	-full			#Display packet data in full.\n"
"\n"
"Examples:\n"
"---------\n"
"\n"
"#Trace and decode all frames, and display packets\n"
"#in full with timestampe decoded into system time.\n"
"\n"
"  wanpipemon -i wp1mp -c tri -full -systime\n"
"\n"
"#Trace LAPB and X25 protocol frames. Futhtermore, \n"
"#only decode x25 frames with LCN=1\n"
"\n"
"  wanpipemon -i wp1mp -c tri -prot LAPB-X25 -lcn 1\n"
"\n"
"#Trace X25 protocol frames and display x25 data\n"
"#in ASCII.\n"
"\n"
"  wanpipemon -i wp1mp -c tri -prot X25 -ascii -full -systime \n"
"\n"
"#Trace data to a pcap type file\n"
"\n"
"  wanpipemon -i wan0 -pcap -c tr\n"
"  wanpipemon -i wan0 -pcap -pcap_file myfile.bin -c tr\n"
"  wanpipemon -i wan0 -pcap -prot FRAME -c tr\n"
"\n"
"#Trace ISDN/MTP2 data to a pcap type file\n"
"  wanpipemon -i w1g1 -pcap -pcap_file isdn.pcap -prot ISDN -full -systime -c trd\n"
"  wanpipemon -i w1g1 -pcap -pcap_file isdn.pcap -prot ISDN -pcap_isdn_network  -full -systime -c trd"
"\n"
"  wanpipemon -i w1g1 -pcap -pcap_file mtp2.pcap -prot MTP2 -full -systime -c trh"
"\n"
"\n";

static void usage_trace_info(void)
{
	printf("%s\n",trace_info);
}
#endif

#if 0
static void sig_handler(int sigint)
{
	int err;
	EXEC_PROT_FUNC(disable_trace,wan_protocol,err,());
}
#endif

#ifdef __WINDOWS__
BOOL sig_end(DWORD dwCtrlType)
{
	if (pcap_output_file){
		fclose(pcap_output_file);
		pcap_output_file=NULL;
	}
    if (trace_bin_out) {
		fclose(trace_bin_out);
		trace_bin_out=NULL;
    }
    if (trace_bin_in) {
		fclose(trace_bin_in);
		trace_bin_in=NULL;
    }
	
	if (sangoma_fd != INVALID_HANDLE_VALUE) {
		CloseConnection(&sangoma_fd);
	}
	
	if (rx_hdlc_eng) {
        wanpipe_unreg_hdlc_engine(rx_hdlc_eng);  
		rx_hdlc_eng=NULL;
	}
	
	//printf("\n\nSignal: Terminating wanpipemon\n");
	
	exit(0);
}
#else
void sig_end(int signal)
{
	if (pcap_output_file){
		fclose(pcap_output_file);
		pcap_output_file=NULL;
	}
    if (trace_bin_out) {
      fclose(trace_bin_out);
      trace_bin_out=NULL;
    }
    if (trace_bin_in) {
      fclose(trace_bin_in);
      trace_bin_in=NULL;
    }

	if (sock) {
		close(sock);
		sock=-1;
	}

	if (rx_hdlc_eng) {
		wanpipe_unreg_hdlc_engine(rx_hdlc_eng);  
		rx_hdlc_eng=NULL;
	}

	if (led_blink) {
		wp_port_led_blink_off();
	}

	//printf("\n\nSignal: Terminating wanpipemon\n");

	exit(0);
}
#endif

int main(int argc, char* argv[])
{
	char command[MAX_CMD_LENGTH+1];
	int err = 0;

	wp_strlcpy((char*)wan_udp.wan_udphdr_signature, 
		GLOBAL_UDP_SIGNATURE,
		strlen(wan_udp.wan_udphdr_signature)); 
	snprintf(pcap_output_file_name,
		strlen(pcap_output_file_name),
		"wp_trace_pcap.bin");
	
#ifdef __WINDOWS__
	if (!SetConsoleCtrlHandler((PHANDLER_ROUTINE)sig_end, TRUE)) {
		printf("ERROR : Unable to register terminate handler ( %d ).\nProcess terminated.\n", 
			GetLastError());
		err=-EINVAL;
		goto main_exit;
	}
#else
	signal(SIGHUP,sig_end);
	signal(SIGINT,sig_end);
	signal(SIGTERM,sig_end);
#endif
	
	rx_hdlc_eng = wanpipe_reg_hdlc_engine();
	if (rx_hdlc_eng) {
		rx_hdlc_eng->hdlc_data = trace_hdlc_data;
	}
	
	printf("\n");
   	if (argc >= 2){
		
		if (init(argc, argv, command) == WAN_FALSE){
			err=-EINVAL;
			goto main_exit;
		}
		
		if (pcap_output){
			
			if (pcap_prot){
				printf("Using custom pcap_prot=%d\n",pcap_prot);
			}
			
			wp_unlink(pcap_output_file_name);/* delete the file */
			pcap_output_file=fopen(pcap_output_file_name,"wb");
			if (!pcap_output_file){
				printf("wanpipemon: Failed to open %s binary file!\n",
					pcap_output_file_name);
				err=-EINVAL;
				goto main_exit;
			}
		}
		if (trace_binary){
			
			printf("Using binary trace\n");
			
			wp_unlink("trace_bin.out");
			wp_unlink("trace_bin.in");
			trace_bin_in=fopen("trace_bin.in","wb");
			if (!trace_bin_in){
				printf("wanpipemon: Failed to open %s binary file!\n",
					"trace_bin.in");
				err=-EINVAL;
				goto main_exit;
			}
			trace_bin_out=fopen("trace_bin.out","wb");
			if (!trace_bin_out){
				printf("wanpipemon: Failed to open %s binary file!\n",
					"trace_bin.out");
				err=-EINVAL;
				goto main_exit;
			}
		}
		
#ifdef WANPIPEMON_GUI
		if (gui_interface && ip_addr==-1){
			if (wan_main_gui() == WAN_FALSE){
				err=-EINVAL;
				goto main_exit;
			}
		}
gui_loop:		
#endif
		
		if (MakeConnection() == WAN_FALSE){ 
			close(sock);
			err=-ENODEV;
			goto main_exit;
		}


		/* Read fe media info for current interface */		
		if (zap_monitor == 0 && dahdi_monitor == 0 && is_logger_dev == 0){
			get_femedia_type(&femedia);
		}
		
		if (is_logger_dev) {
			wan_protocol = WANCONFIG_AFT_TE1;
		}

		//get_hardware_level_interface_name(if_name);
		
#ifdef WANPIPEMON_GUI
		if (gui_interface && wan_protocol != 0){
			if (wan_main_gui() == WAN_FALSE){
				return 0;
			}
			wan_protocol=0;
			goto gui_loop;		
		}
#endif
		/* call per-wanprotocol monitor code: */
		EXEC_PROT_FUNC(main,wan_protocol,err,(command,argc,argv));

		close(sock);
		
		if (pcap_output_file){
			fclose(pcap_output_file);
			pcap_output_file=NULL;
		}
		if (trace_bin_out) {
			fclose(trace_bin_out);
			trace_bin_out=NULL;
		}
		if (trace_bin_in) {
			fclose(trace_bin_in);
			trace_bin_in=NULL;
		}
   	}else{
		usage();
   	}
	
	printf("\n");
	
main_exit:
	
	if (rx_hdlc_eng) {
		wanpipe_unreg_hdlc_engine(rx_hdlc_eng);  
		rx_hdlc_eng=NULL;
	}
	
   	return err;
}


