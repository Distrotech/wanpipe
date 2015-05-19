/*****************************************************************************
* aftpipemon.c	AFT Debugger/Monitor
*
* Author:       Nenad Corbic <ncorbic@sangoma.com>	
*
* Copyright:	(c) 2004 Sangoma Technologies Inc.
*
*		This program is free software; you can redistribute it and/or
*		modify it under the terms of the GNU General Public License
*		as published by the Free Software Foundation; either version
*		2 of the License, or (at your option) any later version.
* ----------------------------------------------------------------------------
* Jan 06, 2004	Nenad Corbic	Initial version based on aftpipemon
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
# include <linux/wanpipe.h>
# include <linux/sdla_xilinx.h>
#else
# include <wanpipe_defines.h>
# include <wanpipe_cfg.h>
# include <wanpipe.h>
# include <sdla_xilinx.h>
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
/* Command routines */
static void modem( void );
#if 0
static void global_stats( void );
#endif
static void comm_err_stats( void );
static void read_code_version( void );
static void link_status( void );
static void operational_stats( void );
static void line_trace( int );
static void aft_router_up_time( void );
static void flush_operational_stats( void );
static void flush_comm_err_stats( void );
static void read_ft1_te1_56k_config( void );


static char *gui_main_menu[]={
"aft_card_stats_menu","Card Status",
"aft_stats_menu", "Card Statistics",
"aft_trace_menu", "Trace Data",
"csudsu_menu", "CSU DSU Config/Stats",
"aft_flush_menu","Flush Statistics",
"."
};


static char *aft_card_stats_menu[]={
"xm","Modem Status",
"xl","Link Status",
"xcv","Read Code Version",
"xru","Display Router UP time",
"."
};
	
static char *aft_stats_menu[]={
"sc","Communication Error Statistics",
"so","Operational Statistics",
"."
};

static char *aft_trace_menu[]={
"tr","Raw Hex trace",
"ti","Interpreted trace",
"."
};

static char *aft_flush_menu[]={
"fc","Flush Communication Error Statistics",
"fo","Flush Operational Statistics",
"."
};


static struct cmd_menu_lookup_t gui_cmd_menu_lookup[]={
	{"aft_card_stats_menu",aft_card_stats_menu},
	{"aft_stats_menu",aft_stats_menu},
	{"aft_trace_menu",aft_trace_menu},
	{"csudsu_menu",csudsu_menu},
	{"aft_flush_menu",aft_flush_menu},
	{".",NULL}
};


char ** AFTget_main_menu(int *len)
{
	int i=0;
	while(strcmp(gui_main_menu[i],".") != 0){
		i++;
	}
	*len=i/2;
	return gui_main_menu;
}

char ** AFTget_cmd_menu(char *cmd_name,int *len)
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
int AFTConfig(void)
{
	char codeversion[10];
	unsigned char x=0;
   
	protocol_cb_size = sizeof(wan_mgmt_t) + 
			   sizeof(wan_cmd_t) + 
			   sizeof(wan_trace_info_t) + 1;
	wan_udp.wan_udphdr_command= READ_CONFIGURATION;
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

	is_508 = WAN_FALSE;
   
	strlcpy(codeversion, "?.??", 10);
   
	wan_udp.wan_udphdr_command = READ_CODE_VERSION;
	wan_udp.wan_udphdr_data_len = 0;
	wan_udp.wan_udphdr_return_code = 0xaa;
	DO_COMMAND(wan_udp);
	if (wan_udp.wan_udphdr_return_code == 0) {
		wan_udp.wan_udphdr_data[wan_udp.wan_udphdr_data_len] = 0;
		strlcpy(codeversion, (char*)wan_udp.wan_udphdr_data, 10);
	}
	
	return(WAN_TRUE);
}; 


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
#endif

static void modem( void )
{
#if 1
	unsigned char cts_dcd;
	wan_udp.wan_udphdr_command= AFT_MODEM_STATUS; 
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
	}
#endif
}; /* modem */


static void comm_err_stats (void)
{
#if 1
	aft_comm_err_stats_t* comm_err_stats;
	wan_udp.wan_udphdr_command= READ_COMMS_ERROR_STATS; 
	wan_udp.wan_udphdr_return_code = 0xaa;
	wan_udp.wan_udphdr_data_len = 0;
	DO_COMMAND(wan_udp);

	if (wan_udp.wan_udphdr_return_code == 0 ) {

		BANNER("AFT COMMUNICATION ERROR STATISTICS");

		comm_err_stats = (aft_comm_err_stats_t *)&wan_udp.wan_udphdr_data[0];

		printf("RX Stats:\n");
		printf("                        Number of receiver overrun errors:  %u\n", 
				comm_err_stats->Rx_overrun_err_count);
		printf("                            Number of receiver CRC errors:  %u\n", 
				comm_err_stats->Rx_crc_err_count);
		printf("                          Number of receiver Abort errors:  %u\n", 
				comm_err_stats->Rx_abort_count);
		printf("                     Number of receiver corruption errors:  %u\n", 
				comm_err_stats->Rx_hdlc_corrupiton);
		printf("                            Number of receiver PCI errors:  %u\n", 
				comm_err_stats->Rx_pci_errors);
		printf("                 Number of receiver DMA descriptor errors:  %u\n", 
				comm_err_stats->Rx_dma_descr_err);
		printf("TX Stats:\n");
		printf("                         Number of transmitter PCI errors:  %u\n", 
				comm_err_stats->Tx_pci_errors);
		printf("               Number of transmitter PCI latency warnings:  %u\n", 
				comm_err_stats->Tx_pci_latency);
		printf("              Number of transmitter DMA descriptor errors:  %u\n", 
				comm_err_stats->Tx_dma_errors);
		printf("       Number of transmitter DMA descriptor length errors:  %u\n", 
				comm_err_stats->Tx_dma_len_nonzero);

#if 0
		printf("                        Number of times DCD changed state:  %u\n", 
				comm_err_stats->DCD_state_change_count);
		printf("                        Number of times CTS changed state:  %u\n", 
				comm_err_stats->CTS_state_change_count);
#endif
	}
#endif
}; /* comm_err_stats */


static void flush_comm_err_stats( void ) 
{
#if 1
	wan_udp.wan_udphdr_command= FLUSH_COMMS_ERROR_STATS;
	wan_udp.wan_udphdr_return_code = 0xaa;
	wan_udp.wan_udphdr_data_len = 0;
	DO_COMMAND(wan_udp);

#endif
}; /* flush_comm_err_stats */


static void read_code_version (void)
{
	wan_udp.wan_udphdr_command= READ_CODE_VERSION; 
	wan_udp.wan_udphdr_return_code = 0xaa;
	wan_udp.wan_udphdr_data_len = 0;
	DO_COMMAND(wan_udp);

	if (wan_udp.wan_udphdr_return_code == 0) {
		BANNER("AFT CODE VERSION");
		printf("Code version: HDLC rev.%X\n",
				wan_udp.wan_udphdr_data[0]);
	}else{
		printf("Error: Rc=%x\n",wan_udp.wan_udphdr_return_code);
	}
}; /* read code version */


static void link_status (void)
{
	wan_udp.wan_udphdr_command= AFT_LINK_STATUS; 
	wan_udp.wan_udphdr_return_code = 0xaa;
	wan_udp.wan_udphdr_data_len = 0;
	DO_COMMAND(wan_udp);
	
	BANNER("AFT LINK STATUS");
	
	if (wan_udp.wan_udphdr_return_code == 0){
		printf("Device Link Status:	%s\n", 
		wan_udp.wan_udphdr_data[0] ? "Connected":"Disconnected");
	}
	
	return;
}; /* Link Status */		


static void operational_stats (void)
{
	aft_op_stats_t *stats;
	wan_udp.wan_udphdr_command= READ_OPERATIONAL_STATS; 
	wan_udp.wan_udphdr_return_code = 0xaa;
	wan_udp.wan_udphdr_data_len = 0;
	DO_COMMAND(wan_udp);

	if (wan_udp.wan_udphdr_return_code == 0) {
		BANNER("AFT OPERATIONAL STATISTICS");
		stats = (aft_op_stats_t *)&wan_udp.wan_udphdr_data[0];
 
		printf(    "             Number of frames transmitted:   %u",
				stats->Data_frames_Tx_count);
		printf(  "\n              Number of bytes transmitted:   %u",
				stats->Data_bytes_Tx_count);
		printf(  "\n                      Transmit Throughput:   %u",
				stats->Data_Tx_throughput);
		printf(  "\n Transmit frames discarded (length error):   %u",
				stats->Tx_Data_discard_lgth_err_count);
		printf(  "\n                Transmit frames realigned:   %u",
				stats->Data_frames_Tx_realign_count);


		printf("\n\n                Number of frames received:   %u",
				stats->Data_frames_Rx_count);
		printf(  "\n                 Number of bytes received:   %u",
				stats->Data_bytes_Rx_count);
		printf(  "\n                       Receive Throughput:   %u",
				stats->Data_Rx_throughput);
		printf(  "\n    Received frames discarded (too short):   %u",
				stats->Rx_Data_discard_short_count);
		printf(  "\n     Received frames discarded (too long):   %u",
				stats->Rx_Data_discard_long_count);
		printf(  "\nReceived frames discarded (link inactive):   %u",
				stats->Rx_Data_discard_inactive_count);


		printf("\n\nHDLC link active/inactive and loopback statistics");
		printf(  "\n                           Times that the link went active:   %u", stats->link_active_count);	
		printf(  "\n         Times that the link went inactive (modem failure):   %u", stats->link_inactive_modem_count);
		printf(  "\n     Times that the link went inactive (keepalive failure):   %u", stats->link_inactive_keepalive_count);
		printf(  "\n                                         link looped count:   %u", stats->link_looped_count);


	} 
} /* Operational_stats */


static void flush_operational_stats( void ) 
{
#if 1
	wan_udp.wan_udphdr_command= FLUSH_OPERATIONAL_STATS;
	wan_udp.wan_udphdr_return_code = 0xaa;
	wan_udp.wan_udphdr_data_len = 0;
	DO_COMMAND(wan_udp);
#endif
}; /* flush_operational_stats */


int AFTDisableTrace(void)
{
	wan_udp.wan_udphdr_command= DISABLE_TRACING;
	wan_udp.wan_udphdr_return_code = 0xaa;
	wan_udp.wan_udphdr_data_len = 0;
	DO_COMMAND(wan_udp);
	return 0;
}

static void line_trace(int trace_mode) 
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
	wan_udp.wan_udphdr_command= DISABLE_TRACING;
	wan_udp.wan_udphdr_return_code = 0xaa;
	wan_udp.wan_udphdr_data_len = 0;
	DO_COMMAND(wan_udp);

	wan_udp.wan_udphdr_command= ENABLE_TRACING;
	wan_udp.wan_udphdr_return_code = 0xaa;
	wan_udp.wan_udphdr_data_len = 1;
	wan_udp.wan_udphdr_data[0]=trace_mode;

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
	
		if(select(1,&ready, NULL, NULL, &to)) {
			break;
		} /* if */

		wan_udp.wan_udphdr_command = GET_TRACE_INFO;
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

				if (raw_data) {
					trace_iface.type=WP_OUT_TRACE_RAW;
				}else if (pcap_output){
					trace_iface.type=WP_OUT_TRACE_PCAP;
				}else{
					trace_iface.type=WP_OUT_TRACE_INTERP;
				}

				trace_iface.link_type=wan_protocol;

				wp_trace_output(&trace_iface);

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
	
	
	wan_udp.wan_udphdr_command= DISABLE_TRACING;
	wan_udp.wan_udphdr_return_code = 0xaa;
	wan_udp.wan_udphdr_data_len = 0;
	DO_COMMAND(wan_udp);
}; /* line_trace */

//CORBA
int AFTUsage(void)
{
	printf("wanpipemon: Wanpipe AFT Hardware Level Debugging Utility\n\n");
	printf("Usage:\n");
	printf("-----\n\n");
	printf("wanpipemon -i <ip-address or interface name> -u <port> -c <command>\n\n");
	printf("\tOption -i: \n");
	printf("\t\tWanpipe remote IP address must be supplied\n");
	printf("\t\t<or> Wanpipe network interface name (ex: wp1_chdlc)\n");   
	printf("\tOption -u: (Optional, default: 9000)\n");
	printf("\t\tWanpipe UDPPORT specified in /etc/wanpipe#.conf\n");
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
	printf("\tCard Statistics\n");
	printf("\t   s         c       Communication Error Statistics\n");
	printf("\t             o       Operational Statistics\n");
	printf("\tTrace Data \n");
	printf("\t   t         i       Trace and Interpret ALL frames\n");
	printf("\t             r       Trace ALL frames, in RAW format\n");
	printf("\tT1/E1 Configuration/Statistics\n");
	printf("\t   T         a       Read T1/E1/56K alarms.\n");  
	printf("\t             allb    Active Line Loopback mode (T1/E1 card only)\n");  
	printf("\t             dllb    Deactive Line Loopback mode (T1/E1 card only)\n");  
	printf("\t             aplb    Active Payload Loopback mode (T1/E1 card only)\n");  
	printf("\t             dplb    Deactive Payload Loopback mode (T1/E1 card only)\n");  
	printf("\t             adlb    Active Diagnostic Digital Loopback mode (T1/E1 card only)\n");  
	printf("\t             ddlb    Deactive Diagnostic Digital Loopback mode (T1/E1 card only)\n");  
	printf("\t             salb    Send Loopback Activate Code (T1/E1 card only)\n");  
	printf("\t             sdlb    Send Loopback Deactive Code (T1/E1 card only)\n");  
	printf("\tFlush Statistics\n");
	printf("\t             c       Flush Communication Error Statistics\n");
	printf("\t             o       Flush Operational Statistics\n");
	printf("\t             p       Flush T1/E1 performance monitoring counters\n");
	printf("\tExamples:\n");
	printf("\t--------\n\n");
	printf("\tex: wanpipemon -i w1g1 -u 9000 -c xm :View Modem Status \n");
	printf("\tex: wanpipemon -i 201.1.1.2 -u 9000 -c ti  :Trace and Interpret ALL frames\n\n");
	return 0;
}




static void aft_router_up_time( void )
{
     	unsigned long time;
     
     	wan_udp.wan_udphdr_command= ROUTER_UP_TIME;
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


static void read_ft1_te1_56k_config (void)
{
	unsigned char	adapter_type = 0x00;

	/* Read Adapter Type */
	if (get_fe_type(&adapter_type)){
		return;
	}

	printf("Adapter type %i\n",adapter_type);
	
	switch(adapter_type){
	case WAN_MEDIA_NONE:
		printf("CSU/DSU Read Configuration Failed");
		break;

	case WAN_MEDIA_T1:
	case WAN_MEDIA_E1:
	case WAN_MEDIA_56K:
		read_te1_56k_config();
		break;

	default:
		printf("Unknown Front End Adapter Type: 0x%X",adapter_type);
		break;
		
	}
	return;
}


int AFTMain(char *command,int argc, char* argv[])
{
	char *opt=&command[1];

	switch(command[0]){

		case 'x':
			if (!strcmp(opt,"m")){	
				modem();
			}else if (!strcmp(opt,"l")){
				link_status();
			}else if (!strcmp(opt, "cv")){
				read_code_version();
			}else if (!strcmp(opt,"ru")){
				aft_router_up_time();
			}else{
				printf("ERROR: Invalid Status Command 'x', Type wanpipemon <cr> for help\n\n");
			}
			break;	
		case 's':
			if (!strcmp(opt,"c")){
				comm_err_stats();
			}else if (!strcmp(opt,"o")){
				operational_stats();
			}else{
				printf("ERROR: Invalid Statistics Command 's', Type wanpipemon <cr> for help\n\n");
			}
			break;
			
		case 't':
			if (!strcmp(opt, "r")){
				raw_data = WAN_TRUE;
				line_trace(0);
			}else if (!strcmp(opt, "i")){
				raw_data = WAN_FALSE;
				line_trace(0);
			}else{
				printf("ERROR: Invalid Trace Command 't', Type wanpipemon <cr> for help\n\n");
			}
			break;

		case 'f':
			if (!strcmp(opt, "o")){
				flush_operational_stats();
				operational_stats();
			}else if (!strcmp(opt, "c")){
				flush_comm_err_stats();
				comm_err_stats();
			}else if (!strcmp(opt, "p")){
				flush_te1_pmon();
			} else{
				printf("ERROR: Invalid Flush Command 'f', Type wanpipemon <cr> for help\n\n");
			}
			break;

		case 'T':
			if (!strcmp(opt,"read")){
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
