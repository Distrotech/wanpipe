/*****************************************************************************
* bpipemon.c	Bitstream  Monitor
* Author:       Nenad Corbic <ncorbic@sangoma.com>	
*
* Copyright:	(c) 1995-2004 Sangoma Technologies Inc.
*
*		This program is free software; you can redistribute it and/or
*		modify it under the terms of the GNU General Public License
*		as published by the Free Software Foundation; either version
*		2 of the License, or (at your option) any later version.
* ----------------------------------------------------------------------------
* Dec 18, 2003	Nenad Corbic	Initial version based on ppipemon
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
#include "wanpipe_api.h"
#include "sdla_bitstrm.h"
#else
# error "Bitstrm is not supported on Non-Linux platforms"
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
 * 			FUNCTION PROTOTYPES				      *
 *****************************************************************************/
static void error( void );

/* Command routines */
static void modem( void );
static void global_stats( void );
static void comm_stats( void );
static void read_code_version( void );
static void operational_stats( void );
static void bitstrm_router_up_time( void );
static void flush_operational_stats( void );
static void flush_global_stats( void );
static void flush_comm_stats( void );
static void read_ft1_te1_56k_config( void );


static char *gui_main_menu[]={
"bitstrm_card_stats_menu","Card Status",
"bitstrm_stats_menu", "Card Statistics",
"csudsu_menu", "CSU DSU Config/Stats",
"bitstrm_flush_menu","Flush Statistics",
"."
};


static char *bitstrm_card_stats_menu[]={
"xm","Modem Status",
"xl","Link Status",
"xcv","Read Code Version",
"xru","Display Router UP time",
"."
};
	
static char *bitstrm_stats_menu[]={
"sg","Global Statistics",
"sc","Communication Error Statistics",
"so","Operational Statistics",
"."
};

static char *bitstrm_flush_menu[]={
"fg","Flush Global Statistics",
"fc","Flush Communication Error Statistics",
"fo","Flush Operational Statistics",
"fp","Flush T1/E1 performance monitoring counters",
"."
};


static struct cmd_menu_lookup_t gui_cmd_menu_lookup[]={
	{"bitstrm_card_stats_menu",bitstrm_card_stats_menu},
	{"bitstrm_stats_menu",bitstrm_stats_menu},
	{"csudsu_menu",csudsu_menu},
	{"bitstrm_flush_menu",bitstrm_flush_menu},
	{".",NULL}
};


char ** BITSTRMget_main_menu(int *len)
{
	int i=0;
	while(strcmp(gui_main_menu[i],".") != 0){
		i++;
	}
	*len=i/2;
	return gui_main_menu;
}

char ** BITSTRMget_cmd_menu(char *cmd_name,int *len)
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
int BITSTRMConfig(void)
{
	char codeversion[10];
	unsigned char x=0;
   
	protocol_cb_size = sizeof(wan_mgmt_t) + 
			   sizeof(wan_cmd_t) + 
			   sizeof(wan_trace_info_t) + 1;
	wan_udp.wan_udphdr_command= READ_BSTRM_CONFIGURATION;
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

	if (wan_udp.wan_udphdr_data_len == sizeof(READ_BSTRM_CONFIGURATION)) {
		is_508 = WAN_TRUE;
	} else {
		is_508 = WAN_FALSE;
	} 
   
	strcpy(codeversion, "?.??");
   
	wan_udp.wan_udphdr_command = READ_BSTRM_CODE_VERSION;
	wan_udp.wan_udphdr_data_len = 0;
	wan_udp.wan_udphdr_return_code = 0xaa;
	DO_COMMAND(wan_udp);
	if (wan_udp.wan_udphdr_return_code == 0) {
		wan_udp.wan_udphdr_data[wan_udp.wan_udphdr_data_len] = 0;
		strcpy(codeversion, (char*)wan_udp.wan_udphdr_data);
	}
	
	return(WAN_TRUE);
}; 

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

		printf("                        Number of receiver overrun errors:  %d\n", comm_stats->Rx_overrun_err_count);
		printf("             Primary port - missed Tx DMA interrupt count:  %d\n", comm_stats->pri_missed_Tx_DMA_int_count);
		printf("                               Synchronization lost count:  %d\n", comm_stats->sync_lost_count);
		printf("                           Synchronization achieved count:  %d\n", comm_stats->sync_achieved_count);
		printf("                        Number of times receiver disabled:  %d\n", comm_stats->Rx_dis_pri_bfrs_full_count);
		printf("                        Number of times DCD changed state:  %d\n", comm_stats->DCD_state_change_count);
		printf("                        Number of times CTS changed state:  %d\n", comm_stats->CTS_state_change_count);

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
	wan_udp.wan_udphdr_command= READ_BSTRM_CODE_VERSION; 
	wan_udp.wan_udphdr_return_code = 0xaa;
	wan_udp.wan_udphdr_data_len = 0;
	DO_COMMAND(wan_udp);

	if (wan_udp.wan_udphdr_return_code == 0) {
		unsigned char version[10];
		BANNER("CODE VERSION");
		memcpy(version,&wan_udp.wan_udphdr_data,wan_udp.wan_udphdr_data_len);
		version[wan_udp.wan_udphdr_data_len] = '\0';
		printf("Bit Streaming Code version: %s\n", version);
	}
}; /* read code version */

static void link_status (void)
{
	READ_BSTRM_STATUS_STRUCT *status;
	wan_udp.wan_udphdr_command= READ_BSTRM_STATUS; 
	wan_udp.wan_udphdr_return_code = 0xaa;
	wan_udp.wan_udphdr_data_len = 0;
	DO_COMMAND(wan_udp);

	if (wan_udp.wan_udphdr_return_code == 0) {
		
		BANNER("LINK STATUS");	
	
		status = (READ_BSTRM_STATUS_STRUCT *)&wan_udp.wan_udphdr_data[0];

		printf("                          Link status:  ");
		     switch (status->sync_status) {
			case SYNC_ACHIEVED:
				printf("Active\n");
				break;
			default:
				printf("Inactive (0x%X)\n",status->sync_status);
		     }

		printf("Available data frames for application:  %u\n",
			status->no_Rx_data_blocks_avail);

		printf("                      Receiver status:  ");
		     if (status->receiver_status & 0x01)
			printf("Disabled due to flow control restrictions\n");
		     else printf("Enabled\n");

		printf("                 Exception Conditions:  0x%X",
				status->BSTRM_excep_conditions);
		    
	} else {
		error();
	}
}; /* Link Status */		


static void operational_stats (void)
{
	BSTRM_OPERATIONAL_STATS_STRUCT *stats;
	wan_udp.wan_udphdr_command= READ_BSTRM_OPERATIONAL_STATS; 
	wan_udp.wan_udphdr_return_code = 0xaa;
	wan_udp.wan_udphdr_data_len = 0;
	DO_COMMAND(wan_udp);

	if (wan_udp.wan_udphdr_return_code == 0) {
		BANNER("OPERATIONAL STATISTICS");
		stats = (BSTRM_OPERATIONAL_STATS_STRUCT *)&wan_udp.wan_udphdr_data[0];

		
		printf(    "             Number of blocks transmitted:   %ld",stats->blocks_Tx_count);
		printf(  "\n              Number of bytes transmitted:   %ld",stats->bytes_Tx_count);
		printf(  "\n                      Transmit Throughput:   %ld",stats->Tx_throughput);
		printf(  "\n                            Tx idle count:   %ld",stats->Tx_idle_count);
		printf(  "\n Tx blocks discarded (length error) count:   %ld",stats->Tx_discard_lgth_err_count);
		printf("\n\n                Number of blocks received:   %ld",stats->blocks_Rx_count);
		printf(  "\n                 Number of bytes received:   %ld",stats->bytes_Rx_count);
		printf(  "\n                       Receive Throughput:   %ld",stats->Rx_throughput);
		printf(  "\n                    Receive discard count:   %ld",stats->Rx_discard_count);

	} else {
		error();
	}
} /* Operational_stats */


static void flush_operational_stats( void ) 
{
	wan_udp.wan_udphdr_command= FLUSH_BSTRM_OPERATIONAL_STATS;
	wan_udp.wan_udphdr_return_code = 0xaa;
	wan_udp.wan_udphdr_data_len = 0;
	DO_COMMAND(wan_udp);

	if (wan_udp.wan_udphdr_return_code != 0 ) {
		error();
	}

}; /* flush_operational_stats */


int BITSTMRDisableTrace(void)
{
	wan_udp.wan_udphdr_command= BPIPE_DISABLE_TRACING;
	wan_udp.wan_udphdr_return_code = 0xaa;
	wan_udp.wan_udphdr_data_len = 0;
	DO_COMMAND(wan_udp);
	return 0;
}

int BITSTRMUsage(void)
{

	printf("bpipemon: Wanpipe Bisync Debugging Utility\n\n");
	printf("Usage:\n");
	printf("-----\n\n");
	printf("bpipemon -i <ip-address or interface name> -u <port> -c <command>\n\n");
	printf("\tOption -i: \n");
	printf("\t\tWanpipe remote IP address must be supplied\n");
	printf("\t\t<or> Wanpipe network interface name (ex: wp1_bstrm)\n");   
	printf("\tOption -u: (Optional, default: 9000)\n");
	printf("\t\tWanpipe UDPPORT specified in /etc/wanpipe#.conf\n");
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
	printf("\t             cv      Read Code Version\n");
	printf("\tCard Statistics\n");
	printf("\t   s         g       Global Statistics\n");
	printf("\t             c       Communication Error Statistics\n");
	printf("\t             o       Operational Statistics\n");
	printf("\tExamples:\n");
	printf("\t--------\n\n");
	printf("\tex: bpipemon -i wp1_bstrm0 -u 9000 -c xm :View Modem Status \n");

	return 0;
}




static void bitstrm_router_up_time( void )
{
     	u_int32_t time;
     
     	wan_udp.wan_udphdr_command= BPIPE_ROUTER_UP_TIME;
	wan_udp.wan_udphdr_return_code = 0xaa;
     	wan_udp.wan_udphdr_data_len = 0;
     	wan_udp.wan_udphdr_data[0] = 0;
     	DO_COMMAND(wan_udp);
    
     	time = *(u_int32_t*)&wan_udp.wan_udphdr_data[0];
	
	BANNER("ROUTER UP TIME");
        print_router_up_time(time);
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
		printf("Unsuported Media : adapter_type=0x%X\n",adapter_type);
		break;

	case WAN_MEDIA_T1:
	case WAN_MEDIA_E1:
	case WAN_MEDIA_56K:
		read_te1_56k_config();
		break;
	}
	return;
}


int BITSTRMMain(char *command,int argc, char* argv[])
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
				bitstrm_router_up_time();
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
				read_te1_56k_stat(0);
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
