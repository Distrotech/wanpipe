/*****************************************************************************
* wanpipemon.c	Cisco HDLC Monitor
*
* Author:       Nenad Corbic <ncorbic@sangoma.com>	
*
* Copyright:	(c) 1995-1999 Sangoma Technologies Inc.
*
*		This program is free software; you can redistribute it and/or
*		modify it under the terms of the GNU General Public License
*		as published by the Free Software Foundation; either version
*		2 of the License, or (at your option) any later version.
* ----------------------------------------------------------------------------
*
* Jan 11, 2005  David Rokhvarg  Added code to run above AFT card with protocol
* 				in the LIP layer.
* Dec 05, 2001  Alex Feldman    Added T1/E1 Statistics.
* Nov 07, 2000  Nenad Corbic    Added Keyboard Led Debugging Commands.
* Mar 14, 2000  Nenad Corbic	Added API support. No IP addresses needed.
* Sep 21, 1999  Nenad Corbic    Changed the input parameters, hearders
*                               data types. More user friendly.
* Aug 06, 1998	David Fong	Initial version based on ppipemon
*****************************************************************************/

/******************************************************************************
 * 			INCLUDE FILES					      *
 *****************************************************************************/
#include <stdio.h>
#include <stddef.h>	/* offsetof(), etc. */
#include <ctype.h>
#include <time.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <netinet/in_systm.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/udp.h>

#if defined(__LINUX__)
# include <linux/version.h>
# include <linux/types.h>
# include <linux/if_packet.h>
# include <linux/if_wanpipe.h>
# include <linux/if_ether.h>
#endif

#include "wanpipe_api.h"
#include "sdla_chdlc.h"

#include "fe_lib.h"
#include "wanpipemon.h"
#include "wanpipe_sppp_iface.h"


/******************************************************************************
 * 			DEFINES/MACROS					      *
 *****************************************************************************/
#if LINUX_VERSION_CODE >= 0x020100
#define LINUX_2_1
#endif

#define TIMEOUT 1
#define MDATALEN MAX_LGTH_UDP_MGNT_PKT
#define MAX_TRACE_BUF ((MDATALEN+200)*2)


#define BANNER(str)  banner(str,0)
/******************************************************************************
 * 			TYPEDEF/STRUCTURE				      *
 *****************************************************************************/

/******************************************************************************
 * 			GLOBAL VARIABLES				      *
 *****************************************************************************/
/* The ft1_lib needs these global variables */
static int	gfail;
extern char*	progname;

/******************************************************************************
 * 			FUNCTION PROTOTYPES				      *
 *****************************************************************************/
static void error( void );

/* Command routines */
static void line_trace_S514( int trace_mode, int trace_sub_type);
static void modem_S514( void );
static void comm_err_S514(void);
static void flush_comm_err_S514( void );
static void operational_stats_S514( void );
static void flush_operational_stats_S514( void );
static void read_code_version_S514(void);
static void link_status_S514(void);

static void flush_slarp_cdp_stats(void);

static void comm_err( void );
static void flush_comm_err( void );

static void operational_stats( void );
static void flush_operational_stats( void );

static void chdlc_router_up_time( void );

/*
static void global_stats (void);
static void flush_global_stats( void );
*/

static void read_code_version( void );
static void chdlc_configuration( void );

static void link_status( void );
static void slarp_stats( void );

static void set_FT1_monitor_status( unsigned char);
static void read_ft1_op_stats( void );
static void read_ft1_te1_56k_config( void );
static void ft1_usage (void);

static wp_trace_output_iface_t trace_iface;

static char *gui_main_menu[]={
"chdlc_card_stats_menu","Card Status",
//"chdlc_card_config_menu","Card Configuration",
"chdlc_stats_menu", "Card Statistics",
"chdlc_trace_menu", "Trace Data",
"csudsu_menu", "CSU DSU Config/Stats",
"chdlc_flush_menu","Flush Statistics",
"."
};


static char *chdlc_card_stats_menu[]={
"xm","Modem Status",
"xl","Link Status",
"xcv","Read Code Version",
"xru","Display Router UP time",
"."
};
	
static char *chdlc_card_config_menu[]={
"crc","Read Configuration\n",
"."
};

static char *chdlc_stats_menu[]={
//"sg","Global Statistics",
"sc","Communication Error Statistics",
"so","Operational Statistics",
"ss","SLARP and CDP Statistics",
"."
};

static char *chdlc_trace_menu[]={
"ti","Trace and Interpret ALL frames",
//"tip","Trace and Interpret PROTOCOL frames only",
//"tid","Trace and Interpret DATA frames only",
"tr","Trace ALL frames, in RAW format",
//"trp","Trace PROTOCOL frames only, in RAW format",
//"trd","Trace DATA frames only, in RAW format",
"."
};

#if 0
static char *chdlc_csudsu_menu[]={
"Tv","View Status",
"Ts","Self Test",
"Tl","Line Loop Test",
"Td","Digital Loop Test",
"Tr","Remote Test",
"To","Operational Mode",
"Tp","Operational stats (FT1)",
"Tf", "Flush operational stats (FT1)",
"Tset", "Set FT1 configuration",
"Tread","Read CSU/DSU config (FT1/T1/E1)",  
"Tallb","E Line Loopback (T1/E1)",  
"Tdllb","D Line Loopback (T1/E1)",  
"Taplb","E Payload Loopback (T1/E1)",  
"Tdplb","D Payload Loopback (T1/E1)",  
"Tadlb","E Diag Digital Loopback (T1/E1)",  
"Tddlb","D Diag Digital Loopback (T1/E1)",  
"Tsalb","Send Loopback Activate Code (T1/E1)",  
"Tsdlb","Send Loopback Deactive Code (T1/E1)",  
"Ta","Read T1/E1/56K alrams",
"."
};
#endif

static char *chdlc_flush_menu[]={
//"fg","Flush Global Statistics",
"fc","Flush Communication Error Statistics",
"fo","Flush Operational Statistics",
"fs","Flush SLARP and CDP Statistics",
"fpm","Flush T1/E1 performance monitoring counters",
"."
};


static struct cmd_menu_lookup_t gui_cmd_menu_lookup[]={
	{"chdlc_card_stats_menu",chdlc_card_stats_menu},
	{"chdlc_card_config_menu",chdlc_card_config_menu},
	{"chdlc_stats_menu",chdlc_stats_menu},
	{"chdlc_trace_menu",chdlc_trace_menu},
	{"csudsu_menu",csudsu_menu},
	{"chdlc_flush_menu",chdlc_flush_menu},
	{".",NULL}
};


char ** CHDLCget_main_menu(int *len)
{
	int i=0;
	while(strcmp(gui_main_menu[i],".") != 0){
		i++;
	}
	*len=i/2;
	return gui_main_menu;
}

char ** CHDLCget_cmd_menu(char *cmd_name,int *len)
{
	int i=0,j=0;
	char **cmd_menu=NULL;
	
	while(gui_cmd_menu_lookup[i].cmd_menu_ptr != NULL){
		if (strcmp(cmd_name,gui_cmd_menu_lookup[i].cmd_menu_name) == 0){
			cmd_menu=gui_cmd_menu_lookup[i].cmd_menu_ptr;
			while (strcmp(cmd_menu[j],".") != 0){
				j++;
			}
			break;
		}
		i++;
	}
	*len=j/2;
	return cmd_menu;
}



/******************************************************************************
 * 			FUNCTION DEFINITION				      *
 *****************************************************************************/
int CHDLCConfig(void)
{
	char codeversion[10];
	unsigned char x=0;
   
	protocol_cb_size = sizeof(wan_mgmt_t) + 
			   sizeof(wan_cmd_t) + 
			   sizeof(wan_trace_info_t) + 1;
	wan_udp.wan_udphdr_command= READ_CHDLC_CONFIGURATION;
	wan_udp.wan_udphdr_return_code = 0xaa;
	wan_udp.wan_udphdr_data_len = 0;
	while (++x < 4) {
		DO_COMMAND(wan_udp); 
		if (wan_udp.wan_udphdr_return_code == 0x00){
			break;
		}
		if (wan_udp.wan_udphdr_return_code == 0xaa){
			printf("Error: Command timeout occurred\n"); 
			return(WAN_FALSE);
		}

		if (wan_udp.wan_udphdr_return_code == 0xCC){ 
			return(WAN_FALSE);
		}
		wan_udp.wan_udphdr_return_code = 0xaa;
	}

	if (x >= 4) return(WAN_FALSE);

	if (wan_udp.wan_udphdr_data_len == sizeof(CHDLC_CONFIGURATION_STRUCT)) {
		is_508 = WAN_TRUE;
	} else {
		is_508 = WAN_FALSE;
	} 
   
	strlcpy(codeversion, "?.??", 10);
   
	wan_udp.wan_udphdr_command = READ_CHDLC_CODE_VERSION;
	wan_udp.wan_udphdr_data_len = 0;
	wan_udp.wan_udphdr_return_code = 0xaa;
	DO_COMMAND(wan_udp);
	if (wan_udp.wan_udphdr_return_code == 0) {
		wan_udp.wan_udphdr_data[wan_udp.wan_udphdr_data_len] = 0;
		strlcpy(codeversion, (char*)wan_udp.wan_udphdr_data,10);
	}
	
	return(WAN_TRUE);
}; 

void CHDLC_set_FT1_mode( void )
{
	for(;;){ 
		wan_udp.wan_udphdr_command= SET_FT1_MODE;
		wan_udp.wan_udphdr_data_len = 0;
		wan_udp.wan_udphdr_return_code = 0xaa;
		DO_COMMAND(wan_udp);
		if (wan_udp.wan_udphdr_return_code == 0){
			break;
		}
	}
} /* set_FT1_mode */

void CHDLC_read_FT1_status( void )
{

	wan_udp.wan_udphdr_command= CPIPE_FT1_READ_STATUS;
	wan_udp.wan_udphdr_data_len = 0;
	wan_udp.wan_udphdr_return_code = 0xaa;
	DO_COMMAND(wan_udp); 
	if(wan_udp.wan_udphdr_return_code == 0) {
		par_port_A_byte = wan_udp.wan_udphdr_data[0];
		par_port_B_byte = wan_udp.wan_udphdr_data[1];

		if(!(par_port_A_byte & PP_A_RT_NOT_RED)) {
			FT1_LED.RT_red ++;
		}
		if(!(par_port_A_byte & PP_A_RT_NOT_GREEN)) {
			FT1_LED.RT_green ++;
 		}
		if((par_port_A_byte & (PP_A_RT_NOT_GREEN | PP_A_RT_NOT_RED))
			== (PP_A_RT_NOT_GREEN | PP_A_RT_NOT_RED)) {
			FT1_LED.RT_off ++;
		}
 		if(!(par_port_A_byte & PP_A_LL_NOT_RED)) {
			FT1_LED.LL_red ++;
		}
		else {
			FT1_LED.LL_off ++;
		}
		if(!(par_port_A_byte & PP_A_DL_NOT_RED)) {
			FT1_LED.DL_red ++;
		}
		else {
			FT1_LED.DL_off ++;
		}
		if(!(par_port_B_byte & PP_B_RxD_NOT_GREEN)) {
			FT1_LED.RxD_green ++;
		}
		if(!(par_port_B_byte & PP_B_TxD_NOT_GREEN)) {
			FT1_LED.TxD_green ++;
		}
		if(!(par_port_B_byte & PP_B_ERR_NOT_GREEN)) {
			FT1_LED.ERR_green ++;
		}
		if(!(par_port_B_byte & PP_B_ERR_NOT_RED)) {
			FT1_LED.ERR_red ++;
		}
		if((par_port_B_byte & (PP_B_ERR_NOT_GREEN | PP_B_ERR_NOT_RED))
			== (PP_B_ERR_NOT_GREEN | PP_B_ERR_NOT_RED)) {
			FT1_LED.ERR_off ++;
		}
		if(!(par_port_B_byte & PP_B_INS_NOT_RED)) {
			FT1_LED.INS_red ++;
		}
		if(!(par_port_B_byte & PP_B_INS_NOT_GREEN)) {
			FT1_LED.INS_green ++;
		}
		if((par_port_B_byte & (PP_B_INS_NOT_GREEN | PP_B_INS_NOT_RED))
			== (PP_B_INS_NOT_GREEN | PP_B_INS_NOT_RED)) {
			FT1_LED.INS_off ++;
		}
		if(!(par_port_B_byte & PP_B_ST_NOT_GREEN)) {
			FT1_LED.ST_green ++;
		}
		if(!(par_port_B_byte & PP_B_ST_NOT_RED)) {
			FT1_LED.ST_red ++;
		}
		if((par_port_B_byte & (PP_B_ST_NOT_GREEN | PP_B_ST_NOT_RED))
			== (PP_B_ST_NOT_GREEN | PP_B_ST_NOT_RED)) {
			FT1_LED.ST_off ++;
		}
	}
} /* read_FT1_status */

static void error( void ) 
{
	printf("Error: Command failed!\n");
}

#if 0
static void global_stats (void)
{
	GLOBAL_STATS_STRUCT* global_stats;
	wan_udp.wan_udphdr_command= READ_GLOBAL_STATISTICS; 
	wan_udp.wan_udphdr_return_code = 0xaa;
	wan_udp.wan_udphdr_data_len = 0;
	DO_COMMAND(wan_udp);

	if (wan_udp.wan_udphdr_return_code == 0 ) {
		BANNER("GLOBAL STATISTICS");
		global_stats = (GLOBAL_STATS_STRUCT *)&wan_udp.wan_udphdr_data[0];
		printf("Times application did not respond to IRQ: %u",
			global_stats->app_IRQ_timeout_count);
	} else {
		error();
	}
};  /* global stats */

static void flush_global_stats (void)
{
	wan_udp.wan_udphdr_command= FLUSH_GLOBAL_STATISTICS; 
	wan_udp.wan_udphdr_return_code = 0xaa;
	wan_udp.wan_udphdr_data_len = 0;
	DO_COMMAND(wan_udp);

	if (wan_udp.wan_udphdr_return_code != 0 ) error();
};  /* flush global stats */
#endif

static void modem( void )
{
  if(!connected_to_hw_level){
    if(make_hardware_level_connection()){
      return;
    }
  }
  
  if(global_card_type == WANOPT_S51X){
    modem_S514();
  }else{
    hw_modem();
  }
  
  if(connected_to_hw_level){
    cleanup_hardware_level_connection();
  }
}

static void modem_S514( void )
{
	unsigned char cts_dcd;
	wan_udp.wan_udphdr_command= READ_MODEM_STATUS; 
	wan_udp.wan_udphdr_return_code = 0xaa;
	wan_udp.wan_udphdr_data_len = 0;
	DO_COMMAND(wan_udp);

	if (wan_udp.wan_udphdr_return_code == 0 ) {
		BANNER("MODEM STATUS");
		memcpy(&cts_dcd, &wan_udp.wan_udphdr_data[0],1);
		printf("DCD: ");
		(cts_dcd & 0x08) ? printf("High\n") : printf("Low\n");
		printf("CTS: ");
		(cts_dcd & 0x20) ? printf("High\n") : printf("Low\n");
	} else {
		error();
	}
}; /* modem */

static void comm_err(void)
{
  if(!connected_to_hw_level){
    if(make_hardware_level_connection()){
      return;
    }
  }
  
  if(global_card_type == WANOPT_S51X){
    comm_err_S514();
  }else{
    hw_comm_err();
  }
  
  if(connected_to_hw_level){
    cleanup_hardware_level_connection();
  }
}

static void comm_err_S514(void)
{
	COMMS_ERROR_STATS_STRUCT* comm_err_stats;
	wan_udp.wan_udphdr_command= READ_COMMS_ERROR_STATS; 
	wan_udp.wan_udphdr_return_code = 0xaa;
	wan_udp.wan_udphdr_data_len = 0;
	DO_COMMAND(wan_udp);

	if (wan_udp.wan_udphdr_return_code == 0 ) {
	 	BANNER("COMMUNICATION ERROR STATISTICS");

		comm_err_stats = (COMMS_ERROR_STATS_STRUCT *)&wan_udp.wan_udphdr_data[0];

		printf("                        Number of receiver overrun errors:  %u\n",
			       	comm_err_stats->Rx_overrun_err_count);
		printf("                            Number of receiver CRC errors:  %u\n",
			       	comm_err_stats->CRC_err_count);
		printf("                          Number of abort frames received:  %u\n",
			       	comm_err_stats->Rx_abort_count);
		printf("                        Number of times receiver disabled:  %u\n",
			       	comm_err_stats->Rx_dis_pri_bfrs_full_count);
		printf(" Number of transmitted abort frames (missed Tx interrupt):  %u\n",
			       	comm_err_stats->sec_Tx_abort_msd_Tx_int_count);
		printf("                             Number of transmit underruns:  %u\n",
			       	comm_err_stats->missed_Tx_und_int_count);
		printf("                       Number of abort frames transmitted:  %u\n",
			       	comm_err_stats->sec_Tx_abort_count);
		printf("                        Number of times DCD changed state:  %u\n",
			       	comm_err_stats->DCD_state_change_count);
		printf("                        Number of times CTS changed state:  %u\n",
			       	comm_err_stats->CTS_state_change_count);
	} else {
		error();
	}
}; /* comm_stats */

static void flush_comm_err( void )
{
  if(!connected_to_hw_level){
    if(make_hardware_level_connection()){
      return;
    }
  }
  
  if(global_card_type == WANOPT_S51X){
    flush_comm_err_S514();
  }else{
    hw_flush_comm_err();
  }

  if(connected_to_hw_level){
    cleanup_hardware_level_connection();
  }
}

static void flush_comm_err_S514( void ) 
{
	wan_udp.wan_udphdr_command= FLUSH_COMMS_ERROR_STATS;
	wan_udp.wan_udphdr_return_code = 0xaa;
	wan_udp.wan_udphdr_data_len = 0;
	DO_COMMAND(wan_udp);

	if (wan_udp.wan_udphdr_return_code != 0) error();
}; /* flush_comm_stats */

static void read_code_version (void)
{
  if(!connected_to_hw_level){
    if(make_hardware_level_connection()){
      return;
    }
  }
  
  if(global_card_type == WANOPT_S51X){
    read_code_version_S514();
  }else{
    hw_read_code_version();
  }

  if(connected_to_hw_level){
    cleanup_hardware_level_connection();
  }
}

static void read_code_version_S514(void)
{
	wan_udp.wan_udphdr_command= READ_CHDLC_CODE_VERSION; 
	wan_udp.wan_udphdr_return_code = 0xaa;
	wan_udp.wan_udphdr_data_len = 0;
	DO_COMMAND(wan_udp);

	if (wan_udp.wan_udphdr_return_code == 0) {
		unsigned char version[10];
		BANNER("CODE VERSION");
		memcpy(version,&wan_udp.wan_udphdr_data,wan_udp.wan_udphdr_data_len);
		version[wan_udp.wan_udphdr_data_len] = '\0';
		printf("Cisco HDLC Code version: %s\n", version);
	}
}; /* read code version */

static void chdlc_configuration (void)
{
	wan_sppp_if_conf_t *chdlc_cfg;
	wan_udp.wan_udphdr_command = READ_CHDLC_CONFIGURATION; 
	wan_udp.wan_udphdr_return_code = 0xaa;
	wan_udp.wan_udphdr_data_len = 0;
	DO_COMMAND(wan_udp);

	if (wan_udp.wan_udphdr_return_code == 0) {
		chdlc_cfg = (wan_sppp_if_conf_t*)&wan_udp.wan_udphdr_data[0];
		BANNER("CHDLC CONFIGURATION");

		printf("Ignore Keepalive\t: %s\n", 
			(chdlc_cfg->sppp_keepalive_timer == 0 ? "Yes" : "No"));

		if(chdlc_cfg->sppp_keepalive_timer){
			printf("Keepalive interval (s)\t: %u\n", 
				chdlc_cfg->sppp_keepalive_timer);
			printf("Keepalive reception error tolerance: %u",
				chdlc_cfg->keepalive_err_margin);
		}
		
	} else {
		error();
	}
}; /* chdlc_configuration */


static void link_status (void)
{
  if(!connected_to_hw_level){
    if(make_hardware_level_connection()){
      return;
    }
  }
  
  if(global_card_type == WANOPT_S51X){
    link_status_S514();
  }else{
    hw_link_status();
  }

  if(connected_to_hw_level){
    cleanup_hardware_level_connection();
  }
}

static void link_status_S514(void)
{
	CHDLC_LINK_STATUS_STRUCT *status;
	wan_udp.wan_udphdr_command= READ_CHDLC_LINK_STATUS; 
	wan_udp.wan_udphdr_return_code = 0xaa;
	wan_udp.wan_udphdr_data_len = 0;
	DO_COMMAND(wan_udp);

	if (wan_udp.wan_udphdr_return_code == 0) {
		
		BANNER("LINK STATUS");	
	
		status = (CHDLC_LINK_STATUS_STRUCT *)&wan_udp.wan_udphdr_data[0];

		printf("                          Link status:  ");
		     switch (status->CHDLC_link_status) {
			case CHDLC_LINK_INACTIVE:
				printf("Inactive\n");
				break;
			case CHDLC_LINK_ACTIVE:
				printf("Active\n");
				break;
		     }

		printf("Available data frames for application:  %u\n",
			status->no_Data_frms_for_app);

		printf("                      Receiver status:  ");
		     if (status->receiver_status & 0x01)
			printf("Disabled due to flow control restrictions\n");
		     else printf("Enabled\n");
	} else {
		error();
	}
}; /* Link Status */		

static void operational_stats (void)
{
  if(!connected_to_hw_level){
    if(make_hardware_level_connection()){
      return;
    }
  }

  if(global_card_type == WANOPT_S51X){
    operational_stats_S514();
  }else{
    hw_general_stats();
  }
  
  if(connected_to_hw_level){
    cleanup_hardware_level_connection();
  }
}

static void operational_stats_S514(void)
{
	CHDLC_OPERATIONAL_STATS_STRUCT *stats;
	wan_udp.wan_udphdr_command= READ_CHDLC_OPERATIONAL_STATS; 
	wan_udp.wan_udphdr_return_code = 0xaa;
	wan_udp.wan_udphdr_data_len = 0;
	DO_COMMAND(wan_udp);

	if (wan_udp.wan_udphdr_return_code == 0) {
		BANNER("OPERATIONAL STATISTICS");
		stats = (CHDLC_OPERATIONAL_STATS_STRUCT *)&wan_udp.wan_udphdr_data[0];
 
		printf(    "             Number of frames transmitted:   %u",stats->Data_frames_Tx_count);
		printf(  "\n              Number of bytes transmitted:   %u",stats->Data_bytes_Tx_count);
		printf(  "\n                      Transmit Throughput:   %u",stats->Data_Tx_throughput);
		printf(  "\n Transmit frames discarded (length error):   %u",stats->Tx_Data_discard_lgth_err_count);

		printf("\n\n                Number of frames received:   %u",stats->Data_frames_Rx_count);
		printf(  "\n                 Number of bytes received:   %u",stats->Data_bytes_Rx_count);
		printf(  "\n                       Receive Throughput:   %u",stats->Data_Rx_throughput);
		printf(  "\n    Received frames discarded (too short):   %u",stats->Rx_Data_discard_short_count);
		printf(  "\n     Received frames discarded (too long):   %u",stats->Rx_Data_discard_long_count);
		printf(  "\nReceived frames discarded (link inactive):   %u",stats->Rx_Data_discard_inactive_count);


		printf("\n\nIncoming frames with format errors");
		printf(  "\n               Frames of excessive length:   %u", stats->Rx_frms_too_long_count);	
		printf(  "\n                  Incomplete Cisco Header:   %u", stats->Rx_frm_incomp_CHDLC_hdr_count);
		printf(  "\n          Invalid Cisco HDLC link address:   %u", stats->Rx_invalid_CHDLC_addr_count);
		printf(  "\n               Invalid Cisco HDLC control:   %u", stats->Rx_invalid_CHDLC_ctrl_count);
		printf(  "\n            Invalid Cisco HDLC frame type:   %u", stats->Rx_invalid_CHDLC_type_count);

		printf("\n\nCisco HDLC link active/inactive and loopback statistics");
		printf(  "\n                           Times that the link went active:   %u", stats->link_active_count);	
		printf(  "\n         Times that the link went inactive (modem failure):   %u", stats->link_inactive_modem_count);
		printf(  "\n     Times that the link went inactive (keepalive failure):   %u", stats->link_inactive_keepalive_count);
		printf(  "\n                                         link looped count:   %u", stats->link_looped_count);
	} else {
		error();
	}
} /* Operational_stats */

static void flush_operational_stats( void )
{
  if(!connected_to_hw_level){
    if(make_hardware_level_connection()){
      return;
    }
  }
  
  if(global_card_type == WANOPT_S51X){
    flush_operational_stats_S514();
  }else{
    hw_flush_general_stats();
  }

  if(connected_to_hw_level){
    cleanup_hardware_level_connection();
  }
}
	
static void flush_operational_stats_S514( void )
{
	wan_udp.wan_udphdr_command= FLUSH_CHDLC_OPERATIONAL_STATS;
	wan_udp.wan_udphdr_return_code = 0xaa;
	wan_udp.wan_udphdr_data_len = 0;
	DO_COMMAND(wan_udp);

	if (wan_udp.wan_udphdr_return_code != 0 ) {
		error();
	}
}

static void flush_slarp_cdp_stats(void)
{
	wan_udp.wan_udphdr_command= FLUSH_CHDLC_OPERATIONAL_STATS;
	wan_udp.wan_udphdr_return_code = 0xaa;
	wan_udp.wan_udphdr_data_len = 0;
	DO_COMMAND(wan_udp);

	if (wan_udp.wan_udphdr_return_code != 0 ) {
		error();
	}
}

static void slarp_stats (void)
{
	CHDLC_OPERATIONAL_STATS_STRUCT *stats;
	wan_udp.wan_udphdr_command= READ_CHDLC_OPERATIONAL_STATS; 
	wan_udp.wan_udphdr_return_code = 0xaa;
	wan_udp.wan_udphdr_data_len = 0;
	DO_COMMAND(wan_udp);

	if (wan_udp.wan_udphdr_return_code == 0) {
		BANNER("SLARP STATISTICS");
		/*stats = (CHDLC_OPERATIONAL_STATS_STRUCT *)&wan_udp.wan_udphdr_data[0];*/ /* WRONG OFFSET!!! */
		stats = (CHDLC_OPERATIONAL_STATS_STRUCT *)&wan_udp.wan_udphdr_data[0];

		printf("\n SLARP frame transmission/reception statistics");
		printf(  "\n       SLARP request packets transmitted:   %u",
 stats->CHDLC_SLARP_REQ_Tx_count);
		printf(  "\n          SLARP request packets received:   %u",
 stats->CHDLC_SLARP_REQ_Rx_count);
		printf("\n\n         SLARP Reply packets transmitted:   %u",
 stats->CHDLC_SLARP_REPLY_Tx_count);
		printf(  "\n            SLARP Reply packets received:   %u",
 stats->CHDLC_SLARP_REPLY_Rx_count);
		printf("\n\n     SLARP keepalive packets transmitted:   %u",
 stats->CHDLC_SLARP_KPALV_Tx_count);
		printf(  "\n        SLARP keepalive packets received:   %u",
 stats->CHDLC_SLARP_KPALV_Rx_count);

		printf(  "\n\nIncoming SLARP Packets with format errors");
		printf(  "\n                      Invalid SLARP Code:   %u", stats->Rx_SLARP_invalid_code_count);
		printf(  "\n                Replies with bad IP addr:   %u", stats->Rx_SLARP_Reply_bad_IP_addr);
		printf(  "\n                Replies with bad netmask:   %u",stats->Rx_SLARP_Reply_bad_netmask);
		printf(  "\n\nSLARP timeout/retry statistics");
		printf(  "\n                  SLARP Request timeouts:   %u",stats->SLARP_Request_TO_count);
		printf(  "\n            keepalive reception timeouts:   %u",stats->SLARP_Rx_keepalive_TO_count);

		printf("\n\nCisco Discovery Protocol frames");
		printf(  "\n                             Transmitted:   %u", stats->CHDLC_CDP_Tx_count);
		printf(  "\n                                Received:   %u", stats->CHDLC_CDP_Rx_count);

	} else {
		error();
	}

} /* slarp_stats */


int CHDLCDisableTrace(void)
{
	wan_udp.wan_udphdr_command= CPIPE_DISABLE_TRACING;
	wan_udp.wan_udphdr_return_code = 0xaa;
	wan_udp.wan_udphdr_data_len = 0;
	DO_COMMAND(wan_udp);
	return 0;
}

static int print_local_time (char *date_string, int max_len)
{
	
  	char tmp_time[50];
	time_t time_val;
	struct tm *time_tm;
	
	date_string[0]='\0';

	time_val=time(NULL);
		
	/* Parse time and date */
	time_tm=localtime(&time_val);

	strftime(tmp_time,sizeof(tmp_time),"%b",time_tm);
	snprintf(date_string, max_len - strlen(date_string)," %s ",tmp_time);

	strftime(tmp_time,sizeof(tmp_time),"%d",time_tm);
	snprintf(date_string+strlen(date_string), max_len - strlen(date_string),"%s ",tmp_time);

	strftime(tmp_time,sizeof(tmp_time),"%H",time_tm);
	snprintf(date_string+strlen(date_string), max_len - strlen(date_string),"%s:",tmp_time);

	strftime(tmp_time,sizeof(tmp_time),"%M",time_tm);
	snprintf(date_string+strlen(date_string), max_len - strlen(date_string),"%s:",tmp_time);

	strftime(tmp_time,sizeof(tmp_time),"%S",time_tm);
	snprintf(date_string+strlen(date_string), max_len - strlen(date_string),"%s",tmp_time);

	return 0;
}

static int loop_rx_data(int passnum)
{
	unsigned int num_frames;
	unsigned short curr_pos = 0;
	wan_trace_pkt_t *trace_pkt;
	unsigned int i;
	struct timeval to;
	int timeout=0;
	char date_string[100];
	
	gettimeofday(&to, NULL);
	to.tv_sec = 0;
	to.tv_usec = 0;
	
	print_local_time(date_string, 100);
		
	printf("%s | Test %04i | ",
		date_string, passnum);
	
        for(;;) {
	
		select(1,NULL, NULL, NULL, &to);
		
		wan_udp.wan_udphdr_command = CPIPE_GET_TRACE_INFO;
		wan_udp.wan_udphdr_return_code = 0xaa;
		wan_udp.wan_udphdr_data_len = 0;
		DO_COMMAND(wan_udp);
		
		if (wan_udp.wan_udphdr_return_code == 0 && wan_udp.wan_udphdr_data_len) { 
		     
			num_frames = wan_udp.wan_udphdr_aft_num_frames;

		     	for ( i = 0; i < num_frames; i++) {
				trace_pkt= (wan_trace_pkt_t *)&wan_udp.wan_udphdr_data[curr_pos];

				/*  frame type */
				if (trace_pkt->status & 0x01) {
					trace_iface.status |= WP_TRACE_OUTGOING;
				}else{
					if (trace_pkt->status & 0x10) { 
						trace_iface.status |= WP_TRACE_ABORT;
					} else if (trace_pkt->status & 0x20) {
						trace_iface.status |= WP_TRACE_CRC;
					} else if (trace_pkt->status & 0x40) {
						trace_iface.status |= WP_TRACE_OVERRUN;
					}
				}

				trace_iface.len = trace_pkt->real_length;
				trace_iface.timestamp=trace_pkt->time_stamp;
				trace_iface.sec = trace_pkt->sec;
				trace_iface.usec = trace_pkt->usec;
				
				curr_pos += sizeof(wan_trace_pkt_t);
		
				if (trace_pkt->real_length >= WAN_MAX_DATA_SIZE){
					printf("\t:the frame data is to big (%u)!",
									trace_pkt->real_length);
					fflush(stdout);
					continue;

				}else if (trace_pkt->data_avail == 0) {

					printf("\t: the frame data is not available" );
					fflush(stdout);
					continue;
				} 

				
				/* update curr_pos again */
				curr_pos += trace_pkt->real_length;
			
				trace_iface.trace_all_data=trace_all_data;
				trace_iface.data=(unsigned char*)&trace_pkt->data[0];

				
				if (trace_pkt->status & 0x01){ 
					continue;
				}
				
				if (trace_iface.data[0] == (0x0F) &&
				    trace_iface.data[1] == (0x0F) &&
				    trace_iface.data[2] == (0x0F) &&
				    trace_iface.data[3] == (0x0F)) {
				 	printf("Successful (%s)!\n",
						trace_iface.status & WP_TRACE_ABORT ? "Abort" :
						trace_iface.status & WP_TRACE_CRC ? "Crc" :
						trace_iface.status & WP_TRACE_OVERRUN ? "Overrun" : "Ok"
						);   
					return 0;
				} 
				
				

		   	} //for
		} //if
		curr_pos = 0;

		if (!wan_udp.wan_udphdr_chdlc_ismoredata){
			to.tv_sec = 0;
			to.tv_usec = WAN_TRACE_DELAY;
			timeout++;
			if (timeout > 100) {
				printf("Timeout!\n");
				break;
			}   
		}else{
			to.tv_sec = 0;
			to.tv_usec = 0;
		}
	}
	return 0;
}


static int aft_digital_loop_test( void )
{
	int passnum=0;
	int i;
	
        /* Disable trace to ensure that the buffers are flushed */
        wan_udp.wan_udphdr_command= CPIPE_DISABLE_TRACING;
        wan_udp.wan_udphdr_return_code = 0xaa;
        wan_udp.wan_udphdr_data_len = 0;
        DO_COMMAND(wan_udp);

        wan_udp.wan_udphdr_command= CPIPE_ENABLE_TRACING;
        wan_udp.wan_udphdr_return_code = 0xaa;
        wan_udp.wan_udphdr_data_len = 1;
        wan_udp.wan_udphdr_data[0]=0;

        DO_COMMAND(wan_udp);
	if (wan_udp.wan_udphdr_return_code != 0 && 
            wan_udp.wan_udphdr_return_code != 1) {
		printf("Error: Failed to start loop test: failed to start tracing!\n");
		return -1;
	}
	
	printf("Starting Loop Test (press ctrl-c to exit)!\n\n");

	
	
	while (1) {

		wan_udp.wan_udphdr_command= DIGITAL_LOOPTEST;
		wan_udp.wan_udphdr_return_code = 0xaa;
		
		for (i=0;i<100;i++){ 
			wan_udp.wan_udphdr_data[i] = 0x0F;
		}
		wan_udp.wan_udphdr_data_len = 100;
		DO_COMMAND(wan_udp);	

		switch (wan_udp.wan_udphdr_return_code) {
		
		case 0:
			break;
		case 1:
	                printf("Error: Failed to start loop test: dev not found\n");
			goto loop_rx_exit;
		case 2:
	                printf("Error: Failed to start loop test: dev state not connected\n");
			goto loop_rx_exit;
		case 3:
	                printf("Error: Failed to start loop test: memory error\n");
			goto loop_rx_exit;
		case 4:
	                printf("Error: Failed to start loop test: invalid operation mode\n");
			goto loop_rx_exit;

		default:
			printf("Error: Failed to start loop test: unknown error (%i)\n",
				wan_udp.wan_udphdr_return_code);
			goto loop_rx_exit;
		}


		loop_rx_data(++passnum);
		usleep(500000);
		fflush(stdout);
		
		if (passnum >= 10) {
			break;
		}		
	}

loop_rx_exit:
	
	wan_udp.wan_udphdr_command= CPIPE_DISABLE_TRACING;
        wan_udp.wan_udphdr_return_code = 0xaa;
        wan_udp.wan_udphdr_data_len = 0;
        DO_COMMAND(wan_udp);

	return 0;
}




static void line_trace(int trace_mode, int trace_subtype)
{
  if(!connected_to_hw_level){
    if(make_hardware_level_connection()){
      return;
    }
  }
  
  if(global_card_type == WANOPT_S51X){
    line_trace_S514(trace_mode, trace_subtype);
  }else{
    hw_line_trace(trace_mode);
  }

  if(connected_to_hw_level){
    cleanup_hardware_level_connection();
  }
}

static void line_trace_S514(int trace_mode, int trace_subtype)
{
	unsigned int num_frames;
	unsigned short curr_pos = 0;
	wan_trace_pkt_t *trace_pkt;
	unsigned int i;
	int recv_buff = sizeof(wan_udp_hdr_t)+ 100;
	fd_set ready;
	struct timeval to;
 	wp_trace_output_iface_t trace_iface;
   
	memset(&trace_iface,0,sizeof(wp_trace_output_iface_t));

	setsockopt( sock, SOL_SOCKET, SO_RCVBUF, &recv_buff, sizeof(int) );

	/* Disable trace to ensure that the buffers are flushed */
	wan_udp.wan_udphdr_command= CPIPE_DISABLE_TRACING;
	wan_udp.wan_udphdr_return_code = 0xaa;
	wan_udp.wan_udphdr_data_len = 0;
	DO_COMMAND(wan_udp);

	wan_udp.wan_udphdr_command= CPIPE_ENABLE_TRACING;
	wan_udp.wan_udphdr_return_code = 0xaa;
	wan_udp.wan_udphdr_data_len = 0;
	wan_udp.wan_udphdr_data[0]=0;
	if(trace_mode == TRACE_PROT){
		wan_udp.wan_udphdr_data_len = 1;
		wan_udp.wan_udphdr_data[0] |= TRACE_SLARP_FRAMES | TRACE_CDP_FRAMES; 
		printf("Tracing Protocol only %X\n",wan_udp.wan_udphdr_data[0]);
	}else if(trace_mode == TRACE_DATA){
		wan_udp.wan_udphdr_data_len = 1;
		wan_udp.wan_udphdr_data[0] = TRACE_DATA_FRAMES; 
	}else{
		wan_udp.wan_udphdr_data_len = 1;
		wan_udp.wan_udphdr_data[0] |= TRACE_SLARP_FRAMES | 
			      TRACE_DATA_FRAMES | 
			       TRACE_CDP_FRAMES;
	}

	DO_COMMAND(wan_udp);

	if (wan_udp.wan_udphdr_return_code == 0) { 
		printf("Starting trace...(Press ENTER to exit)\n");
		fflush(stdout);
	} else if(wan_udp.wan_udphdr_return_code == 0xCD ) {
		printf("Cannot Enable Line Tracing from Underneath.\n");
		fflush(stdout);
		return;
	}else if (wan_udp.wan_udphdr_return_code == 0x01 ) {
		printf("Starting trace...(although it's already enabled!)\n");
		printf("Press ENTER to exit.\n");
		fflush(stdout);
	}else{
		printf("Failed to Enable Line Tracing. Return code: 0x%02X\n", 
			wan_udp.wan_udphdr_return_code );
		fflush(stdout);
		return;
	}

	to.tv_sec = 0;
	to.tv_usec = 0;
	for(;;) {
		FD_ZERO(&ready);
		FD_SET(0,&ready);
		fflush(stdout);
	
		if(select(1,&ready, NULL, NULL, &to)) {
			break;
		} /* if */

		wan_udp.wan_udphdr_command = CPIPE_GET_TRACE_INFO;
		wan_udp.wan_udphdr_return_code = 0xaa;
		wan_udp.wan_udphdr_data_len = 0;
		DO_COMMAND(wan_udp);

		if (wan_udp.wan_udphdr_return_code == 0 && wan_udp.wan_udphdr_data_len) { 
		     num_frames = wan_udp.wan_udphdr_chdlc_num_frames;

		 	for ( i = 0; i < num_frames; i++) {
				trace_pkt= (wan_trace_pkt_t *)(&wan_udp.wan_udphdr_data[0] + curr_pos);

#if 0
				if (annexg_trace){
		
					if (trace_pkt->real_length > wan_udp.wan_udphdr_data_len){
						break;		
					}
					curr_pos += sizeof(wan_trace_pkt_t);
				
					if (trace_pkt->data_avail) {
						curr_pos += trace_pkt->real_length -1;
				
						decode_pkt(trace_pkt->data,
						  	   trace_pkt->real_length,
					 		   trace_pkt->status,
							   trace_pkt->time_stamp,
						   	0);
					}
					goto trace_skip;
				}
#endif

				/*  frame type */
				trace_iface.status=0;
				if (trace_pkt->status & 0x01) {
					trace_iface.status|=WP_TRACE_OUTGOING;
				} else {
					if (trace_pkt->status & 0x10) { 
						trace_iface.status|=WP_TRACE_ABORT;
					} else if (trace_pkt->status & 0x20) {
						trace_iface.status|=WP_TRACE_CRC;
					} else if (trace_pkt->status & 0x40) {
						trace_iface.status|=WP_TRACE_OVERRUN;
					}
				}
			
				trace_iface.len = trace_pkt->real_length;
				trace_iface.timestamp = trace_pkt->time_stamp;
				trace_iface.sec = trace_pkt->sec;
				trace_iface.usec = trace_pkt->usec;

				if (trace_pkt->real_length > wan_udp.wan_udphdr_data_len){
					break;		
				}
			
				/* first update curr_pos */
				curr_pos += sizeof(wan_trace_pkt_t);
			
				if (trace_pkt->data_avail == 0) {
					printf("the frame data is not available" );
					printf("\n");
					goto trace_skip;
				}

				/* update curr_pos again */
				curr_pos += trace_pkt->real_length;

				trace_iface.trace_all_data=trace_all_data;
				trace_iface.data=(unsigned char*)&trace_pkt->data[0];
				trace_iface.link_type = 104;
			
				if (pcap_output){
					trace_iface.type=WP_OUT_TRACE_PCAP;
				}else if (raw_data) {
					trace_iface.type=WP_OUT_TRACE_RAW;
				}else{
					trace_iface.type=WP_OUT_TRACE_INTERP;
				}

				trace_iface.sub_type = trace_subtype;

				wp_trace_output(&trace_iface);
trace_skip:
			     	fflush(stdout);
		
			} //for
		} //if

		curr_pos = 0;

		if (!wan_udp.wan_udphdr_chdlc_ismoredata){
			to.tv_sec = 0;
			to.tv_usec = WAN_TRACE_DELAY;
		}else{
			to.tv_sec = 0;
			to.tv_usec = 0;
		}
	}
	
	wan_udp.wan_udphdr_command= CPIPE_DISABLE_TRACING;
	wan_udp.wan_udphdr_return_code = 0xaa;
	wan_udp.wan_udphdr_data_len = 0;
	DO_COMMAND(wan_udp);
}; /* line_trace */

//CORBA
int CHDLCUsage(void)
{
#if !defined(CONFIG_PRODUCT_WANPIPE_GENERIC)
	printf("%s: Wanpipe Cisco-HDLC Debugging Utility\n\n", progname);
#else
	printf("%s: Wanpipe HDLC Debugging Utility\n\n", progname);
#endif
	printf("Usage:\n");
	printf("-----\n\n");
	printf("wanpipemon -i <ip-address or interface name> -u <port> -c <command>\n\n");
	printf("\tOption -i: \n");
	printf("\t\tWanpipe remote IP address must be supplied\n");
#if !defined(CONFIG_PRODUCT_WANPIPE_GENERIC)
	printf("\t\t<or> Wanpipe network interface name (ex: wp1_chdlc)\n");   
#else
	printf("\t\t<or> Wanpipe network interface name (ex: hdlc0)\n");   
#endif
	printf("\tOption -u: (Optional, default: 9000)\n");
#if !defined(CONFIG_PRODUCT_WANPIPE_GENERIC)
	printf("\t\tWanpipe UDPPORT specified in /etc/wanpipe/wanpipe#.conf\n");
#else
	printf("\t\tWanpipe UDPPORT specified in /etc/wanpipe/ifcfg-hdlcN\n");
#endif
	printf("\tOption -full: (Optional, trace option)\n");
	printf("\t\tDisplay raw packets in full: default trace pkt len=25bytes\n");
	printf("\tOption -c: \n");
	printf("\t\tCommand is split into two parts:\n"); 
	printf("\t\t\tFirst letter is a command and the rest are options:\n"); 
	printf("\t\t\tex: xm = View Modem Status\n\n");
	printf("\tSupported Commands: x=status : s=statistics : t=trace \n");
	printf("\t                    c=config : T=FT1 stats  : f=flush\n\n");
	printf("\tCommand:  Options:   Description \n");	
	printf("\t-------------------------------- \n\n");    
	printf("\tCard Status\n");
	printf("\t   x         m       Modem Status\n");
	printf("\t             l       Link Status\n");
	printf("\t             cv      Read Code Version\n");
	printf("\t             ru      Display Router UP time\n");
	printf("\tProtocol Configuration\n");
	printf("\t   c         rc      Read CHDLC Configuration\n");
	printf("\tCard Statistics\n");
//	printf("\t   s         g       Global Statistics\n");
	printf("\t   s         c       Communication Error Statistics\n");
	printf("\t             o       Operational Statistics\n");
	printf("\t             s       SLARP and CDP Statistics\n");
	printf("\tTrace Data \n");
#if !defined(CONFIG_PRODUCT_WANPIPE_GENERIC)
	printf("\t   t         i       Trace and Interpret ALL frames\n");
	//printf("\t             ip      Trace and Interpret PROTOCOL frames only\n");
	//printf("\t             id      Trace and Interpret DATA frames only\n");
	printf("\t             r       Trace ALL frames, in RAW format\n");
	//printf("\t             rp      Trace PROTOCOL frames only, in RAW format\n");
	//printf("\t             rd      Trace DATA frames only, in RAW format\n");
#else
	printf("\t   t         r       Trace ALL frames, in RAW format\n");
#endif
	printf("\tFT1/T1/E1/56K Configuration/Statistics\n");
	printf("\t   T         v       View Status (S508/FT1 or S5143/FT1)\n");
	printf("\t             s       Self Test (S508/FT1 or S5143/FT1)\n");
	printf("\t             l       Line Loop Test (S508/FT1 or S5143/FT1)\n");
	printf("\t             d       Digital Loop Test (S508/FT1 or S5143/FT1)\n");
	printf("\t             r       Remote Test (S508/FT1 or S5143/FT1)\n");
	printf("\t             o       Operational Mode (S508/FT1 or S5143/FT1)\n");
	printf("\t             p       FT1 CSU/DSU operational statistics (S508/FT1 or S5143/FT1)\n");
	printf("\t             f       Flush FT1 CSU/DSU operational statistcs (S508/FT1 or S5143/FT1)\n");
	printf("\t             set <Opt> Set CSU/DSU configuration\n");
	printf("\t             read      Read CSU/DSU configuration (FT1/T1/E1 card)\n");  
	printf("\t             allb    Active Line Loopback mode (T1/E1 card only)\n");  
	printf("\t             dllb    Deactive Line Loopback mode (T1/E1 card only)\n");  
	printf("\t             aplb    Active Payload Loopback mode (T1/E1 card only)\n");  
	printf("\t             dplb    Deactive Payload Loopback mode (T1/E1 card only)\n");  
	printf("\t             adlb    Active Diagnostic Digital Loopback mode (T1/E1 card only)\n");  
	printf("\t             ddlb    Deactive Diagnostic Digital Loopback mode (T1/E1 card only)\n");  
	printf("\t             salb    Send Loopback Activate Code (T1/E1 card only)\n");  
	printf("\t             sdlb    Send Loopback Deactive Code (T1/E1 card only)\n");  
	printf("\t             a       Read T1/E1/56K alarms.\n");  
	printf("\t             lt      Diagnostic Digital Loopback testing (T1/E1 card only)\n"); 
	printf("\tFlush Statistics\n");
	//printf("\t   f         g       Flush Global Statistics\n");
	printf("\t    f        c       Flush Communication Error Statistics\n");
	printf("\t             o       Flush Operational Statistics\n");
	printf("\t             s       Flush SLARP and CDP Statistics\n");
	printf("\t             pm      Flush T1/E1 performance monitoring counters\n");
	printf("\tExamples:\n");
	printf("\t--------\n\n");
#if !defined(CONFIG_PRODUCT_WANPIPE_GENERIC)
	printf("\tex: wanpipemon -i wp1_chdlc0 -u 9000 -c xm :View Modem Status \n");
	printf("\tex: wanpipemon -i wp1_chdlc0 -c Tset       :View Help for CSU/DSU Setup\n");
	printf("\tex: wanpipemon -i 201.1.1.2 -u 9000 -c ti  :Trace and Interpret ALL frames\n\n");
#else
	printf("\tex: %s -i hdlc0 -u 9000 -c xm :View Modem Status \n", progname);
	printf("\tex: %s -i hdlc0 -c Tset       :View Help for CSU/DSU Setup\n", progname);
	printf("\tex: %s -i 201.1.1.2 -u 9000 -c ti  :Trace and Interpret ALL frames\n\n", progname);
#endif
	return 0;
}

static void chdlc_router_up_time( void )
{
  u_int32_t time;

  if(!connected_to_hw_level){
    if(make_hardware_level_connection()){
      return;
    }
  }
  
  if(global_card_type == WANOPT_S51X){
    wan_udp.wan_udphdr_command= CPIPE_ROUTER_UP_TIME;
    wan_udp.wan_udphdr_return_code = 0xaa;
    wan_udp.wan_udphdr_data_len = 0;
    wan_udp.wan_udphdr_data[0] = 0;
    DO_COMMAND(wan_udp);
    
    time = *(u_int32_t*)&wan_udp.wan_udphdr_data[0];

    BANNER("ROUTER UP TIME (CHDLC)");

    print_router_up_time(time);
  }else{
    hw_router_up_time();
  }
	
  if(connected_to_hw_level){
    cleanup_hardware_level_connection();
  }
}

static void set_FT1_monitor_status( unsigned char status)
{
	gfail = 0;
	
	//cb.data[0] = status; 	
	wan_udp.wan_udphdr_command= FT1_MONITOR_STATUS_CTRL;
	wan_udp.wan_udphdr_return_code = 0xaa;
	wan_udp.wan_udphdr_data_len = 1;
	wan_udp.wan_udphdr_data[0] = status; 	
	DO_COMMAND(wan_udp);
	//if( (cb.wan_udphdr_return_code != 0) && status){
	if ((wan_udp.wan_udphdr_return_code != 0) && status){
		gfail = 1;
		printf("This command is only possible with S508/FT1 board!");
	}
} /* set_FT1_monitor_status */


static void read_ft1_op_stats( void )
{
        fd_set ready;
	struct timeval to;
	unsigned short length;
 
   
	printf("TIME: Time in seconds since the FT1 op stats were last reset\n");
	printf("  ES: Errored Second\n");
	printf(" BSE: Bursty Errored Second\n");
	printf(" SES: Severly Errored Second\n");
	printf(" UAS: Unavailable Second\n");
	printf("LOFC: Loss of Frame Count\n");
	printf(" ESF: ESF Error Count\n");
   
	printf("\nFT1 CSU/DSU Operational Stats...(Press ENTER to exit)\n");
	for(;;){	
		
		
 		
		wan_udp.wan_udphdr_command= READ_FT1_OPERATIONAL_STATS;
		wan_udp.wan_udphdr_return_code = 0xaa;
 		wan_udp.wan_udphdr_data_len = 0;
		DO_COMMAND(wan_udp);

		FD_ZERO(&ready);
		FD_SET(0,&ready);
		to.tv_sec = 0;
		to.tv_usec = 50000;
	
		if(select(1,&ready, NULL, NULL, &to)) {
			break;
		} /* if */
		//if( cb.wan_udphdr_data_len ){
		if (wan_udp.wan_udphdr_data_len ){
			int i=0;
			printf("   TIME    ES   BES   SES   UAS  LOFC    ESF\n");
			//length = cb.wan_udphdr_data_len;
			length = wan_udp.wan_udphdr_data_len;
			while(length){
			
				//printf("%c", cb.data[i]);
				printf("%c", wan_udp.wan_udphdr_data[i]);
				length--;
				i++;
			}
			printf("\n");
		}
	}
}

static unsigned char ft1_usage_buf[]="\n"
"\n"
"---------------------------------------------------------------------------------------\n"
"		\n"
"Usage: wanpipemon -i <int name> -u <udp port> -c Tset <Framing> <Encoding> <LineBuild> \\n"
"                                                    <Channel> <Baudrate> <Clock>\n"
"\n"
"Example: Setup CSU/DSU for T1, ESF, B8ZS:\n"
"	    wanpipemon -i chdlc0 -u 9000 -c Tset 0 0 0 1 1536 0\n"
"---------------------------------------------------------------------------------------\n"
"						    \n"
"FRAMING:        0 = ESF ,       1 = D4\n\\n"
"		Please select one of the above options as specified by\n\\n"
"                your FT1 provider.\n\n"
"\n"
"ENCODING:       0 = B8ZS ,      1 = AMI\n\\n"
"		Please select one of the above options as specified by\n\\n"
"		your FT1 provider.\n\n"
"		\n"
"LINE_BUILD:     0 = CSU: 0db, DSX: 0-133ft\n\\n"
"		1 = DSX: 266-399ft\n\\n"
"		2 = DSX: 399-533ft\n\\n"
"		3 = DSX: 399-533ft\n\\n"
"		4 = DSX: 533-655ft\n\\n"
"		5 = CSU -7dB\n\\n"
"		6 = CSU -15dB\n\\n"
"		7 = CSU -22.5dB\n\\n"
"\n"
"		For WANPIPE FT1 Card setup, select the first option:\n"
"		* CSU 0dB DSX1 0-133ft (option 0)\n"
"\n"
"		For any other setup, select an option as specified by\n"
"		your FT1 provider.\n\n"
"		\n"
"CHANNEL:        Number between 1 to 24.\n"
"		Represents the first active channel.\n"
"		(Default = 1)\n"
"\n"
"		The T1 line consists of 24 channels, each 64 or 56\n"
"		Kbps. If this is a full T1 line, then the first\n"
"		active channel is 1.  Otherwise, set the first\n"
"		active channel as specified by your FT1 provider.\n\n"
"		\n"
"BAUD:           Line baud rate (speed).\n"
"\n"
"		For B8ZS Encoding:\n"
"			Full T1 baud rate is: 1536 Kbps.\n"
"			Each T1 channel runs at: 64 Kbps.\n"
"		For AMI Encoding:\n"
"			Full T1 baud rate is: 1344 Kbps.\n"
"			Each T1 channel runs at: 56 Kbps.\n"
"\n"
"		Enter the speed of your connection in Kbps, as\n"
"		specified by your FT1 provider.\n"
"\n"
"		To calculate your baud rate, find out how many\n"
"		active T1 channels you have.  Then multiply that\n"
"		number to ether 64 (for B8ZS) or 56 (for AMI).\n"
"		Note, that first active channel should be set to\n"
"		1 in most cases.  (Note: CHANNEL option)\n"
"		\n"
"CLOCK:          0 = NORMAL,     1 = MASTER\n"
"\n"
"		Default is always Normal.\n"
"\n"
"		Select clocking options as specified by your FT1\n"
"		provider.\n"
"\n";

static void ft1_usage (void)
{
	printf("%s\n",ft1_usage_buf);
}



static void set_ft1_config(int argc, char ** argv)
{
	ft1_config_t *ft1_config = (ft1_config_t *)&wan_udp.wan_udphdr_data[0];
	int i=0,found=0;
	
	wan_udp.wan_udphdr_command = SET_FT1_CONFIGURATION;
	wan_udp.wan_udphdr_return_code = 0xaa;
	wan_udp.wan_udphdr_data_len = sizeof(ft1_config_t);

	for (i=0;i<argc;i++){
		if (!strcmp(argv[i],"Tset")){
			i++;
			found=1;
			break;
		}
	}
	
	if (!found){
		printf("Error, failed to find 'Tset' command\n");
		ft1_usage();
		return;
	}
	
	if (argc <= i+5){
		printf("Error, arguments missing: argc %i\n",argc);
		ft1_usage();
		return;
	}
	
	ft1_config->framing_mode   = atoi(argv[i++]);
	ft1_config->encoding_mode  = atoi(argv[i++]);
	ft1_config->line_build_out = atoi(argv[i++]);  
	ft1_config->channel_base   = atoi(argv[i++]);
	ft1_config->baud_rate_kbps = atoi(argv[i++]);
	ft1_config->clock_mode     = atoi(argv[i]);


	if (ft1_config->framing_mode > 1){
		printf("Error: Invalid Framing Mode (0=ESF 1=D4)\n");
		return;
	}
	if (ft1_config->encoding_mode > 1){
		printf("Error: Invalid Encoding Mode (0=B8ZS 1=AMI)\n");
		return;
	}
	if (ft1_config->line_build_out > 7){
		printf("Error: Invalid Line Build Mode (0-7) Default:0\n");
		return;
	}
	if (ft1_config->channel_base > 24){
		printf("Error: Invalid Channel Base (1-24) Default:1 \n");
		return;
	}	
	if ((ft1_config->encoding_mode ==0 && ft1_config->baud_rate_kbps > 1536)||
	    (ft1_config->encoding_mode ==1 && ft1_config->baud_rate_kbps > 1344)){
		printf("Error: Invalid Baud Rate: B8ZS Max=1536KBps; AMI Max=1344KBps\n");
		return;
	}

	if ((ft1_config->encoding_mode ==0 && ft1_config->baud_rate_kbps < 64)||
	    (ft1_config->encoding_mode ==1 && ft1_config->baud_rate_kbps < 56)){
		printf("Error: Invalid Baud Rate: B8ZS Min=64KBps; AMI Max=56KBps\n");
		return;
	}

	if ((ft1_config->encoding_mode ==0 && ft1_config->baud_rate_kbps%64 != 0)||
	    (ft1_config->encoding_mode ==1 && ft1_config->baud_rate_kbps%56 != 0)){
		printf("Error: Invalid Baud Rate: \n");
		printf("       For B8ZS: Baud rate must be a multiple of 64KBps.\n"); 
		printf("       For AMI:  Baud rate must be a multiple of 56KBps.\n");
		return;
	}
	
	if (ft1_config->clock_mode  > 1){
		printf("Error: Invalid Clock Mode: (0=Normal, 1=Master) Default:0\n");
		return;
	}
	
	DO_COMMAND(wan_udp);
	//if (cb.wan_udphdr_return_code == 0){
	if (wan_udp.wan_udphdr_return_code == 0){
		printf("CSU/DSU Configuration Successfull!\n");
		
	//}else if (cb.wan_udphdr_return_code == 0xaa){
	}else if (wan_udp.wan_udphdr_return_code == 0xaa){
		printf("Sangoma Adaptor is not equiped with an onboard CSU/DSU!\n");
		printf("\tCSU/DSU Configuration Failed!\n");
	
	}else{
		//printf("CSU/DSU Configuration Failed: rc %x",cb.wan_udphdr_return_code);
		printf("CSU/DSU Configuration Failed: rc %x",wan_udp.wan_udphdr_return_code);
	}
}


static void read_ft1_te1_56k_config (void)
{
	unsigned char	adapter_type = 0x00;

	/* Read Adapter Type */
	if (get_fe_type(&adapter_type)){
		return;
	}

	switch(adapter_type){
	case WAN_MEDIA_NONE:
		
		wan_udp.wan_udphdr_command = READ_FT1_CONFIGURATION;
		wan_udp.wan_udphdr_return_code = 0xaa;
		wan_udp.wan_udphdr_data_len = 0;

		DO_COMMAND(wan_udp);
		if (wan_udp.wan_udphdr_return_code != 0){
			printf("CSU/DSU Read Configuration Failed");
		}else{
			//ft1_config_t *ft1_config = (ft1_config_t *)&cb.data[0];
			ft1_config_t *ft1_config = (ft1_config_t *)&wan_udp.wan_udphdr_data[0];
			printf("CSU/DSU %s Configuration:\n", if_name);
			printf("\tFraming\t\t%s\n",ft1_config->framing_mode ? "D4" : "ESF");
			printf("\tEncoding\t%s\n",ft1_config->encoding_mode ? "AMI" : "B8ZS");
			printf("\tLine Build\t%i\n",ft1_config->line_build_out);
			printf("\tChannel Base\t%i\n",ft1_config->channel_base);
			printf("\tBaud Rate\t%i\n",ft1_config->baud_rate_kbps);
			printf("\tClock Mode\t%s\n",ft1_config->clock_mode ? "MASTER":"NORMAL");
		}
		break;

	case WAN_MEDIA_T1:
	case WAN_MEDIA_E1:
	case WAN_MEDIA_56K:
		read_te1_56k_config();
		break;
	}
	return;
}


int CHDLCMain(char *command,int argc, char* argv[])
{
	char *opt=&command[1];
	
	global_command = command;
	global_argc = argc;
	global_argv = argv;

	switch(command[0]){

		case 'x':
			if (!strcmp(opt,"m")){	
				modem();
			}else if (!strcmp(opt, "cv")){
				read_code_version();
			}else if (!strcmp(opt,"l")){
				link_status();
			}else if (!strcmp(opt,"ru")){
				chdlc_router_up_time();
			}else{
				printf("ERROR: Invalid Status Command 'x'\n");
				printf("ERROR: Type %s <cr> for help\n\n", progname);
			}
			break;	
		case 's':
			//if (!strcmp(opt,"g")){
			//	global_stats();
			//}else
			if (!strcmp(opt,"c")){
				comm_err();
			}else if (!strcmp(opt,"o")){
				operational_stats();
			}else if (!strcmp(opt,"s")){
				slarp_stats();
			}else{
				printf("ERROR: Invalid Statistics Command 's'\n");
				printf("ERROR: Type %s <cr> for help\n\n", progname);
			}
			break;

		case 'c':
			if (!strcmp(opt,"rc")){
				chdlc_configuration();
/*
			}else if (!strcmp(opt,"reset")){
				chdlc_reset_adapter();
*/
			}else{
				printf("ERROR: Invalid Configuration Command 'c'\n");
				printf("ERROR: Type %s <cr> for help\n\n", progname);
			}
			break;

		case 't':
			if(!strcmp(opt,"i" )){
				raw_data = WAN_FALSE;
				line_trace(TRACE_ALL, WP_OUT_TRACE_RAW);
			}else if (!strcmp(opt, "r")){
				raw_data = WAN_TRUE;
				line_trace(TRACE_ALL, WP_OUT_TRACE_RAW);
			}
			/*
			else if (!strcmp(opt, "ip")){
				raw_data = WAN_FALSE;
				line_trace(TRACE_PROT, WP_OUT_TRACE_RAW);
			}else if (!strcmp(opt, "id")){
				raw_data = WAN_FALSE;
				line_trace(TRACE_DATA, WP_OUT_TRACE_RAW);
			}else if (!strcmp(opt, "rp")){
				raw_data = WAN_TRUE;
				line_trace(TRACE_PROT, WP_OUT_TRACE_RAW);
			}else if (!strcmp(opt, "rd")){
				raw_data = WAN_TRUE;
				line_trace(TRACE_DATA, WP_OUT_TRACE_RAW);
			}else if (!strcmp(opt, "ra")){
				raw_data = WAN_TRUE;
				annexg_trace = WAN_TRUE;
				line_trace(TRACE_ALL, WP_OUT_TRACE_RAW);
			}
			*/
			else{
				printf("ERROR: Invalid Trace Command 't'\n");
				printf("ERROR: Type %s <cr> for help\n\n", progname);
			}
			break;

		case 'f':
			if (!strcmp(opt, "o")){
				flush_operational_stats();
				operational_stats();
			}else if (!strcmp(opt, "s")){
				flush_slarp_cdp_stats();
				slarp_stats();
			//}else if (!strcmp(opt, "g")){
			//	flush_global_stats();
			//	global_stats();
			}else if (!strcmp(opt, "c")){
				flush_comm_err();
				comm_err();
			}else if (!strcmp(opt, "pm")){
				flush_te1_pmon();
			} else{
				printf("ERROR: Invalid Flush Command 'f'\n");
				printf("ERROR: Type %s <cr> for help\n\n", progname);
			}
			break;

		case 'T':
			if (!strcmp(opt, "v")){
				set_FT1_monitor_status(0x01);
				if(!gfail){
					view_FT1_status();
				 }
				set_FT1_monitor_status(0x00);
			
			}else if (!strcmp(opt,"lt")){
 				aft_digital_loop_test();
				
			}else if (!strcmp(opt, "s")){
				set_FT1_monitor_status(0x01);
				if(!gfail){	 	
					FT1_self_test();
				}
				set_FT1_monitor_status(0x00);
			}else if (!strcmp(opt, "l")){
				set_FT1_monitor_status(0x01);
				if(!gfail){
					FT1_local_loop_mode();
				}
				set_FT1_monitor_status(0x00);
			}else if (!strcmp(opt, "d")){
				set_FT1_monitor_status(0x01);
				if(!gfail){
					FT1_digital_loop_mode();
				}
				set_FT1_monitor_status(0x00);
			}else if (!strcmp(opt, "r")){
				set_FT1_monitor_status(0x01);
				if(!gfail){
					FT1_remote_test();
				}
				set_FT1_monitor_status(0x00);
			}else if (!strcmp(opt, "o")){
				set_FT1_monitor_status(0x01);
				if(!gfail){
					FT1_operational_mode();
				}
					}else if (!strcmp(opt,"txe")){
				set_fe_tx_mode(WAN_FE_TXMODE_ENABLE);
			}else if (!strcmp(opt,"txd")){
				set_fe_tx_mode(WAN_FE_TXMODE_DISABLE);
set_FT1_monitor_status(0x00);
			}else if (!strcmp(opt, "p")){
				set_FT1_monitor_status(0x02);
				if(!gfail)
					read_ft1_op_stats();
				set_FT1_monitor_status(0x00);
			}else if (!strcmp(opt, "f")){
				set_FT1_monitor_status(0x04);
				if(!gfail){
					printf("Flushed Operational Statistics\n");
				}else{
					printf("FT1 Flush Failed %i\n",gfail);
				}

			}else if (!strcmp(opt, "set")){
				set_ft1_config(argc, argv);
			}else if (!strcmp(opt,"read")){
				read_ft1_te1_56k_config();
			}else if (!strcmp(opt,"allb")){
				set_lb_modes(WAN_TE1_LINELB_MODE, WAN_TE1_LB_ENABLE);
			}else if (!strcmp(opt,"dllb")){
				set_lb_modes(WAN_TE1_LINELB_MODE, WAN_TE1_LB_DISABLE);
			}else if (!strcmp(opt,"aplb")){
				set_lb_modes(WAN_TE1_PAYLB_MODE, WAN_TE1_LB_ENABLE);
			}else if (!strcmp(opt,"dplb")){
				set_lb_modes(WAN_TE1_PAYLB_MODE, WAN_TE1_LB_DISABLE);
			}else if (!strcmp(opt,"adlb")){
				set_lb_modes(WAN_TE1_DDLB_MODE, WAN_TE1_LB_ENABLE);
			}else if (!strcmp(opt,"ddlb")){
				set_lb_modes(WAN_TE1_DDLB_MODE, WAN_TE1_LB_DISABLE);
			}else if (!strcmp(opt,"salb")){
				set_lb_modes(WAN_TE1_TX_LINELB_MODE, WAN_TE1_LB_ENABLE);
			}else if (!strcmp(opt,"sdlb")){
				set_lb_modes(WAN_TE1_TX_LINELB_MODE, WAN_TE1_LB_DISABLE);
			}else if (!strcmp(opt,"a")){
				read_te1_56k_stat(0);
			} else{
				printf("ERROR: Invalid FT1/T1/E1 Command 'T'\n");
				printf("ERROR: Type %s <cr> for help\n\n", progname);
			} 
			break;

		default:
			printf("ERROR: Invalid Command!\n");
			printf("ERROR: Type %s <cr> for help\n\n", progname);
			break;
	}//switch 
   	printf("\n");
   	fflush(stdout);
   	return 0;
}; //main
