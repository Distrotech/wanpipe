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
# include <linux/wanpipe_defines.h>
# include <linux/wanpipe_cfg.h>
# include <linux/wanpipe_abstr.h>
# include <linux/wanpipe.h>
# include <linux/sdla_ss7.h>
# include <linux/ss7_linux.h>
#else
# error "Non Linux OS Not supproted!"
#endif
#include "fe_lib.h"
#include "wanpipemon.h"


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
/* Structures for data casting */

/******************************************************************************
 * 			GLOBAL VARIABLES				      *
 *****************************************************************************/
/* The ft1_lib needs these global variables */

/******************************************************************************
 * 			FUNCTION PROTOTYPES				      *
 *****************************************************************************/
static void error( void );

/* Command routines */
static void modem( void );
static void global_stats( void );
static void comm_stats( void );
static void read_code_version( void );
static void ss7_configuration( void );
static void link_status( void );
static void operational_stats( void );
static void line_trace( int );
static void ss7_router_up_time( void );
static void flush_operational_stats( void );
static void flush_global_stats( void );
static void flush_comm_stats( void );
//static void set_FT1_monitor_status( unsigned char);
//static void read_ft1_op_stats( void );
//static void read_ft1_te1_56k_config( void );
//static void ft1_usage (void);


static char *gui_main_menu[]={
"ss7_card_stats_menu","Card Status",
"ss7_card_config_menu","Card Configuration",
"ss7_stats_menu", "Card Statistics",
"ss7_trace_menu", "Trace Data",
"csudsu_menu", "CSU DSU Config/Stats",
"ss7_flush_menu","Flush Statistics",
"."
};


static char *ss7_card_stats_menu[]={
"xm","Modem Status",
"xl","Link Status",
"xcv","Read Code Version",
"xru","Display Router UP time",
"."
};
	
static char *ss7_card_config_menu[]={
"crc","Read Configuration\n",
"creset","Reset Adapter\n",
"."
};

static char *ss7_stats_menu[]={
"sg","Global Statistics",
"sc","Communication Error Statistics",
"so","Operational Statistics",
"ss","SLARP and CDP Statistics",
"."
};

static char *ss7_trace_menu[]={
"tr" ,"Trace ALL  Diff SS7 frames, in Hex",
"trf","Trace Diff FISU frames, in Hex",
"trl","Trace Diff LSSU frames, in Hex",
"trm","Trace Diff MSU frames,  in Hex",

"."
};

#if 0
static char *ss7_csudsu_menu[]={
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

static char *ss7_flush_menu[]={
"fg","Flush Global Statistics",
"fc","Flush Communication Error Statistics",
"fo","Flush Operational Statistics",
"fs","Flush SLARP and CDP Statistics",
"fp","Flush T1/E1 performance monitoring counters",
"."
};


static struct cmd_menu_lookup_t gui_cmd_menu_lookup[]={
	{"ss7_card_stats_menu",ss7_card_stats_menu},
	{"ss7_card_config_menu",ss7_card_config_menu},
	{"ss7_stats_menu",ss7_stats_menu},
	{"ss7_trace_menu",ss7_trace_menu},
	{"csudsu_menu",csudsu_menu},
	{"ss7_flush_menu",ss7_flush_menu},
	{".",NULL}
};


char ** SS7get_main_menu(int *len)
{
	int i=0;
	while(strcmp(gui_main_menu[i],".") != 0){
		i++;
	}
	*len=i/2;
	return gui_main_menu;
}

char ** SS7get_cmd_menu(char *cmd_name,int *len)
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
int SS7Config(void)
{
	char codeversion[10];
	unsigned char x=0;
   
	protocol_cb_size = sizeof(wan_mgmt_t) + 
			   sizeof(wan_cmd_t) + 
			   sizeof(wan_trace_info_t) + 1;
	wan_udp.wan_udphdr_command= L2_READ_CONFIGURATION;
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

	if (wan_udp.wan_udphdr_data_len == sizeof(L2_CONFIGURATION_STRUCT)) {
		is_508 = WAN_TRUE;
	} else {
		is_508 = WAN_FALSE;
	} 
   
	strcpy(codeversion, "?.??");
   
	wan_udp.wan_udphdr_command = READ_SS7_CODE_VERSION;
	wan_udp.wan_udphdr_data_len = 0;
	wan_udp.wan_udphdr_return_code = 0xaa;
	DO_COMMAND(wan_udp);
	if (wan_udp.wan_udphdr_return_code == 0) {
		wan_udp.wan_udphdr_data[wan_udp.wan_udphdr_data_len] = 0;
		strcpy(codeversion, (char*)wan_udp.wan_udphdr_data);
	}
	
	return(WAN_TRUE);
}; 

#if 0
void ss7_set_FT1_mode( void )
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

void ss7_read_FT1_status( void )
{

	wan_udp.wan_udphdr_command= SPIPE_FT1_READ_STATUS;
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
 
#endif

static void error( void ) 
{
	printf("Error: Command failed!\n");

}; /* error */


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

static void modem( void )
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


static void comm_stats (void)
{
	COMMS_ERROR_STATS_STRUCT* comm_stats;
	wan_udp.wan_udphdr_command= READ_COMMS_ERROR_STATS; 
	wan_udp.wan_udphdr_return_code = 0xaa;
	wan_udp.wan_udphdr_data_len = 0;
	DO_COMMAND(wan_udp);

	if (wan_udp.wan_udphdr_return_code == 0 ) {
	 	BANNER("COMMUNICATION ERROR STATISTICS");

		comm_stats = (COMMS_ERROR_STATS_STRUCT *)&wan_udp.wan_udphdr_data[0];

		printf("                        Number of receiver overrun errors:  %u\n", comm_stats->Rx_overrun_err_count);
		printf("                            Number of receiver CRC errors:  %u\n", comm_stats->CRC_err_count);
		printf("                          Number of abort frames received:  %u\n", comm_stats->Rx_abort_count);
		printf("                        Number of times receiver disabled:  %u\n", comm_stats->Rx_dis_pri_bfrs_full_count);
		printf("                        Number of times DCD changed state:  %u\n", comm_stats->DCD_state_change_count);
		printf("                        Number of times CTS changed state:  %u\n", comm_stats->CTS_state_change_count);
	} else {
		error();
	}
}; /* comm_stats */


static void flush_comm_stats( void ) 
{
	wan_udp.wan_udphdr_command= FLUSH_COMMS_ERROR_STATS;
	wan_udp.wan_udphdr_return_code = 0xaa;
	wan_udp.wan_udphdr_data_len = 0;
	DO_COMMAND(wan_udp);

	if (wan_udp.wan_udphdr_return_code != 0) error();
}; /* flush_comm_stats */


static void read_code_version (void)
{
	wan_udp.wan_udphdr_command= READ_SS7_CODE_VERSION; 
	wan_udp.wan_udphdr_return_code = 0xaa;
	wan_udp.wan_udphdr_data_len = 0;
	DO_COMMAND(wan_udp);

	if (wan_udp.wan_udphdr_return_code == 0) {
		unsigned char version[10];
		BANNER("CODE VERSION");
		memcpy(version,&wan_udp.wan_udphdr_data,wan_udp.wan_udphdr_data_len);
		version[wan_udp.wan_udphdr_data_len] = '\0';
		printf("SS7 Code version: %s\n", version);
	}
}; /* read code version */

static void ss7_configuration (void)
{
	L2_CONFIGURATION_STRUCT *ss7_cfg;
	wan_udp.wan_udphdr_command = L2_READ_CONFIGURATION; 
	wan_udp.wan_udphdr_return_code = 0xaa;
	wan_udp.wan_udphdr_data_len = 0;
	DO_COMMAND(wan_udp);

	if (wan_udp.wan_udphdr_return_code == 0) {
		ss7_cfg = (L2_CONFIGURATION_STRUCT *)&wan_udp.wan_udphdr_data[0];
		BANNER("SS7 CONFIGURATION");

		printf("Baud rate: %lu", ss7_cfg->baud_rate);

		printf("\nLine configuration: ");
		     switch (ss7_cfg->line_config_options) {
			case INTERFACE_LEVEL_V35:
				printf("V.35");
				break;
			case INTERFACE_LEVEL_RS232:
				printf("RS-232");
				break;
		     }

		printf("\n                     Modem Config Options on: 0x%04X",
				ss7_cfg->modem_config_options);

		printf("\n                          Modem Status Timer: %u",
				ss7_cfg->modem_status_timer);

		printf("\n                              L2 API Options:  0x%04X",
			ss7_cfg->L2_API_options);

		printf("\n                         L2 Protocol Options: 0x%04X",
			ss7_cfg->L2_protocol_options);

	} else {
		error();
	}
}; /* ss7_configuration */


static void link_status (void)
{
	L2_LINK_STATUS_STRUCT *status;
	wan_udp.wan_udphdr_command= L2_READ_LINK_STATUS; 
	wan_udp.wan_udphdr_return_code = 0xaa;
	wan_udp.wan_udphdr_data_len = 0;
	DO_COMMAND(wan_udp);

	if (wan_udp.wan_udphdr_return_code == 0) {
		
		BANNER("LINK STATUS");	
	
		status = (L2_LINK_STATUS_STRUCT *)&wan_udp.wan_udphdr_data[0];

		printf("                          Link status:  ");
		     switch (status->L2_link_status) {
			case L2_LINK_STAT_IN_SERVICE:
				printf("In Service\n");
				break;
			default:
				printf("Inactive 0x%X\n",
						wan_udp.wan_udphdr_return_code);
				break;
		     }

		printf("Available data frames for application:  %u\n",
			status->no_Rx_MSUs_avail_for_L3);

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
	L2_OPERATIONAL_STATS_STRUCT *stats;
	wan_udp.wan_udphdr_command= L2_READ_OPERATIONAL_STATS; 
	wan_udp.wan_udphdr_return_code = 0xaa;
	wan_udp.wan_udphdr_data_len = 0;
	DO_COMMAND(wan_udp);

	if (wan_udp.wan_udphdr_return_code == 0) {
		BANNER("OPERATIONAL STATISTICS");
		stats = (L2_OPERATIONAL_STATS_STRUCT *)&wan_udp.wan_udphdr_data[0];
 
	} else {
		error();
	}
} /* Operational_stats */


static void flush_operational_stats( void ) 
{
	wan_udp.wan_udphdr_command= L2_FLUSH_OPERATIONAL_STATS;
	wan_udp.wan_udphdr_return_code = 0xaa;
	wan_udp.wan_udphdr_data_len = 0;
	DO_COMMAND(wan_udp);

	if (wan_udp.wan_udphdr_return_code != 0 ) {
		error();
	}

}; /* flush_operational_stats */

int SS7DisableTrace(void)
{
	wan_udp.wan_udphdr_command= SPIPE_DISABLE_TRACING;
	wan_udp.wan_udphdr_return_code = 0xaa;
	wan_udp.wan_udphdr_data_len = 0;
	DO_COMMAND(wan_udp);
	return 0;
}

static void line_trace(int trace_mode) 
{
	unsigned int num_frames, num_chars;
	unsigned short curr_pos = 0;
	wan_trace_pkt_t *trace_pkt;
	unsigned int i, j;
	int recv_buff = sizeof(wan_udp_hdr_t)+ 100;
	fd_set ready;
	struct timeval to;
	
	setsockopt( sock, SOL_SOCKET, SO_RCVBUF, &recv_buff, sizeof(int) );

	/* Disable trace to ensure that the buffers are flushed */
	wan_udp.wan_udphdr_command= SPIPE_DISABLE_TRACING;
	wan_udp.wan_udphdr_return_code = 0xaa;
	wan_udp.wan_udphdr_data_len = 0;
	DO_COMMAND(wan_udp);

	wan_udp.wan_udphdr_command= SPIPE_ENABLE_TRACING;
	wan_udp.wan_udphdr_return_code = 0xaa;
	wan_udp.wan_udphdr_data_len = 1;
	wan_udp.wan_udphdr_data[0] = trace_mode;
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

		wan_udp.wan_udphdr_command = SPIPE_GET_TRACE_INFO;
		wan_udp.wan_udphdr_return_code = 0xaa;
		wan_udp.wan_udphdr_data_len = 0;
		DO_COMMAND(wan_udp);

		if (wan_udp.wan_udphdr_return_code == 0 && wan_udp.wan_udphdr_data_len) { 
		     num_frames = wan_udp.wan_udphdr_ss7_num_frames;

	 	  for ( i = 0; i < num_frames; i++) {
			trace_pkt= (wan_trace_pkt_t *)(&wan_udp.wan_udphdr_data[0] + curr_pos);

			/*  frame type */
			if (trace_pkt->status & 0x01) {
				printf("\nOUTGOING\t");
			} else {
				if (trace_pkt->status & 0x10) { 
					printf("\nINCOMING - Aborted\t");
				} else if (trace_pkt->status & 0x20) {
					printf("\nINCOMING - CRC Error\t");
				} else if (trace_pkt->status & 0x40) {
					printf("\nINCOMING - Overrun Error\t");
				} else {
					printf("\nINCOMING\t");
				}
			}

			/* real length and time stamp */
			printf("Len=%-5u\tTimestamp=%-5u\t", 
			     trace_pkt->real_length, trace_pkt->time_stamp);

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
			curr_pos += trace_pkt->real_length - 1;

			if (!trace_all_data){
				num_chars = (unsigned char)
					((trace_pkt->real_length <= DEFAULT_TRACE_LEN)
					? trace_pkt->real_length : DEFAULT_TRACE_LEN);
			}else{
				num_chars = trace_pkt->real_length;
			}

			printf("\n\t\t");
			for( j=0; j<num_chars; j++ ) {
				if (j!=0 && !(j%20)){
					printf("\n\t\t");
				}
					printf("%02X ", (unsigned char)trace_pkt->data[j]);
			 }

			if (num_chars < trace_pkt->real_length){
				printf("..");
			}
			printf("\n");
trace_skip:
		     	fflush(stdout);
		
		  } //for
		
		}else{ //if
			//printf("No Frames Rc=%i Data len %i!\n",
			//		wan_udp.wan_udphdr_return_code,
			//		wan_udp.wan_udphdr_data_len);
		}
			
		curr_pos = 0;

		if (!wan_udp.wan_udphdr_ss7_ismoredata){
			to.tv_sec = 0;
			to.tv_usec = WAN_TRACE_DELAY; 
		}else{
			to.tv_sec = 0;
			to.tv_usec = 0;
		}
	}
	
	
	wan_udp.wan_udphdr_command= SPIPE_DISABLE_TRACING;
	wan_udp.wan_udphdr_return_code = 0xaa;
	wan_udp.wan_udphdr_data_len = 0;
	DO_COMMAND(wan_udp);
}; /* line_trace */

//CORBA
int SS7Usage(void)
{
	printf("wanpipemon: Wanpipe SS7 Debugging Utility\n\n");
	printf("Usage:\n");
	printf("-----\n\n");
	printf("wanpipemon -i <interface name> -c <command>\n\n");
	printf("\tOption -i: \n");
	printf("\t\tWanpipe network interface name (ex: wp1ss7)\n");   
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
	printf("\tCard Configuration\n");
	printf("\t   c         rc      Read SS7 Configuration\n");
	printf("\tCard Statistics\n");
	printf("\t   s         g       Global Statistics\n");
	printf("\t             c       Communication Error Statistics\n");
	printf("\t             o       Operational Statistics\n");
	printf("\tTrace Data \n");
	printf("\t   t         i       Trace and Interpret ALL frames\n");
	printf("\t             ip      Trace and Interpret PROTOCOL frames only\n");
	printf("\t             id      Trace and Interpret DATA frames only\n");
	printf("\t             r       Trace ALL frames, in RAW format\n");
	printf("\t             rp      Trace PROTOCOL frames only, in RAW format\n");
	printf("\t             rd      Trace DATA frames only, in RAW format\n");
	printf("\tFT1/T1/E1/56K Configuration/Statistics\n");
	printf("\t   T         v       View Status \n");
	printf("\t             s       Self Test\n");
	printf("\t             l       Line Loop Test\n");
	printf("\t             d       Digital Loop Test\n");
	printf("\t             r       Remote Test\n");
	printf("\t             o       Operational Mode\n");
	printf("\t             p       FT1 CSU/DSU operational statistics\n");
	printf("\t             f       Flush FT1 CSU/DSU operational statistcs\n");
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
	printf("\tFlush Statistics\n");
	printf("\t   f         g       Flush Global Statistics\n");
	printf("\t             c       Flush Communication Error Statistics\n");
	printf("\t             o       Flush Operational Statistics\n");
	printf("\t             p       Flush T1/E1 performance monitoring counters\n");
	printf("\tExamples:\n");
	printf("\t--------\n\n");
	printf("\tex: wanpipemon -i wp1_chdlc0 -u 9000 -c xm :View Modem Status \n");
	printf("\tex: wanpipemon -i wp1_chdlc0 -c Tset       :View Help for CSU/DSU Setup\n");
	printf("\tex: wanpipemon -i 201.1.1.2 -u 9000 -c ti  :Trace and Interpret ALL frames\n\n");
	return 0;
}




static void ss7_router_up_time( void )
{
     	unsigned long time;
     
     	wan_udp.wan_udphdr_command= SPIPE_ROUTER_UP_TIME;
	wan_udp.wan_udphdr_return_code = 0xaa;
     	wan_udp.wan_udphdr_data_len = 0;
     	wan_udp.wan_udphdr_data[0] = 0;
     	DO_COMMAND(wan_udp);
    
     	time = *(unsigned long*)&wan_udp.wan_udphdr_data[0];
	
	BANNER("ROUTER UP TIME");

	
     	if (time < 3600) {
		if (time<60) 
     			printf("    Router UP Time:  %lu seconds\n", time);
		else
     			printf("    Router UP Time:  %lu minute(s)\n", (time/60));
     	}else
     		printf("    Router UP Time:  %lu hour(s)\n", (time/3600));
			
      
}

#if 0
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

#endif

#if 0
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
			printf("CSU/DSU %s Conriguration:\n", if_name);
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
#endif

int SS7Main(char *command,int argc, char* argv[])
{
	char *opt=&command[1];

	switch(command[0]){

		case 'x':
			if (!strcmp(opt,"m")){	
				modem();
			}else if (!strcmp(opt, "cv")){
				read_code_version();
			}else if (!strcmp(opt,"l")){
				link_status();
			}else if (!strcmp(opt,"ru")){
				ss7_router_up_time();
			}else{
				printf("ERROR: Invalid Status Command 'x', Type wanpipemon <cr> for help\n\n");
			}
			break;	
		case 's':
			if (!strcmp(opt,"g")){
				global_stats();
			}else if (!strcmp(opt,"c")){
				comm_stats();
			}else if (!strcmp(opt,"o")){
				operational_stats();
			}else{
				printf("ERROR: Invalid Statistics Command 's', Type wanpipemon <cr> for help\n\n");

			}
			break;
		case 'c':
			if (!strcmp(opt,"rc")){
				ss7_configuration();

			}else{
				printf("ERROR: Invalid Configuration Command 'c', Type wanpipemon <cr> for help\n\n");
			}
			break;
		case 't':
			if (!strcmp(opt, "r")){
				raw_data = WAN_TRUE;
				line_trace(TRACE_FISU|TRACE_LSSU|TRACE_MSU);
			}else if (!strcmp(opt, "rf")){
				raw_data = WAN_TRUE;
				line_trace(TRACE_FISU);
			}else if (!strcmp(opt, "rl")){
				raw_data = WAN_TRUE;
				line_trace(TRACE_LSSU);
			}else if (!strcmp(opt, "rm")){
				raw_data = WAN_TRUE;
				annexg_trace = WAN_TRUE;
				line_trace(TRACE_MSU);
			}else{
				printf("ERROR: Invalid Trace Command 't', Type wanpipemon <cr> for help\n\n");
			}
			break;
		case 'f':
			if (!strcmp(opt, "o")){
				flush_operational_stats();
				operational_stats();
			}else if (!strcmp(opt, "g")){
				flush_global_stats();
				global_stats();
			}else if (!strcmp(opt, "c")){
				flush_comm_stats();
				comm_stats();
			}else if (!strcmp(opt, "p")){
				flush_te1_pmon();
			} else{
				printf("ERROR: Invalid Flush Command 'f', Type wanpipemon <cr> for help\n\n");
			}
			break;

		case 'T':
#if 0
			if (!strcmp(opt, "v")){
				set_FT1_monitor_status(0x01);
				if(!gfail){
					view_FT1_status();
				 }
				set_FT1_monitor_status(0x00);
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
			if (!strcmp(opt,"read")){
				read_ft1_te1_56k_config();
#endif
			if (!strcmp(opt,"allb")){
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
				read_te1_56k_stat();
			} else{
				printf("ERROR: Invalid FT1 Command 'T', Type wanpipemon <cr> for help\n\n");
			} 
			break;

		default:
			printf("ERROR: Invalid Command, Type wanpipemon <cr> for help\n\n");
			break;
	}//switch 
   	printf("\n");
   	fflush(stdout);
   	return 0;
}; //main
