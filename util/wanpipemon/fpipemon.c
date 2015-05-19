/*****************************************************************************
* wanpipemon.c	Frame Relay Monitor.
*
* Authors:	Nenad Corbic
* 		Jaspreet Singh	
*
* Copyright:	(c) 1995-2000 Sangoma Technologies Inc.
*
*		This program is free software; you can redistribute it and/or
*		modify it under the terms of the GNU General Public License
*		as published by the Free Software Foundation; either version
*		2 of the License, or (at your option) any later version.
* ----------------------------------------------------------------------------
* Feb  2, 2006  David Rokhvarg  Made sure to call FRConfig() each time before
*				using 'station_config', otherwise left uninitialized.
* Jan 12, 2005  David Rokhvarg  Added code to run above AFT card with protocol
* 				in the LIP layer.
* Jul 05, 2004	David Rokhvarg	Added i4 option to decode IPV4 level data
* Oct 24, 2001  Nenad Corbic	Added the -full option to display all trace
*                               packets. i.e. disable the default 25 byte 
*                               cutoff.
* Oct 15, 2001  Nenad Corbic    Added the -x option: to format all output in
*                               XML format.
* Mar 14, 2000	Nenad Corbic	Added Raw Socket API support. No IP addresses.
* Mar 22, 1997	Jaspreet Singh	Improved Error handling
* Nov 24, 1997	Jaspreet Singh	Added new stats for driver statistics
* Nov 13, 1997	Jaspreet Singh	Fixed descriptions of Global Error Statistics
* Oct 20, 1997 	Jaspreet Singh	Added new commands for driver specific stats
*				and router up time.
* Jul 28, 1997	Jaspreet Singh	Added a new command for running line trace 
*				displaying RAW data.
* Jul 25, 1997	Jaspreet Singh	Added commands for viewing specific DLCI data 
*				including FECN and BECN. 
* Jun 24, 1997	Jaspreet Singh	S508/FT1 test commands		
* Apr 25, 1997	Farhan Thawar	Initial version based on wanpipemon for WinNT.
*****************************************************************************/

/******************************************************************************
 * 			INCLUDE FILES					      *
 *****************************************************************************/
#include <stdio.h>
#include <ctype.h>
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

#include "wanpipe_api.h"
#include "sdla_fr.h"     
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
#define MAX_CMD_ARG 10

#define HDR_SIZE sizeof(fr_encap_hdr_t)+sizeof(ip_pkt_t)+sizeof(udp_pkt_t) 
#define CB_SIZE sizeof(wp_mgmt_t)+sizeof(cblock_t)+1

#define FR_DBG if(1) printf

#define PVC_STATE_NEW       0x01
#define PVC_STATE_ACTIVE    0x02

/******************************************************************************
 * 			GLOBAL VARIABLES				      *
 *****************************************************************************/

unsigned char station_config;

char *fr_card_stats_menu[]={
"xm","Modem Status",
"xl","Link Status",
"xru","Display Router UP time",
"."
};

char *fr_card_config_menu[]={
"cl","List Active DLCIs",
"clr","List All Reported DLCIs",
"."
};

char *fr_stats_menu[]={
"sg","Global Statistics",
"sc","Communication Error Statistics",
"se","Error Statistics",
//"sd","Read Statistics for a specific DLCI",
"."
};

char *fr_trace_menu[]={
"ti","Trace and Interpret ALL frames",
//"ti4","Trace and Interpret IP V4 DATA frames",
"tr","Trace ALL frames in HEX format",
//"tip","Trace and Interpret PROTOCOL frames",
//"tid","Trace and Interpret DATA frames",
//"trp","Trace RAW PROTOCOL frames",
//"trd","Trace RAW DATA frames",
"."
};

#if 0
char *fr_csudsu_menu[]={
"","--- T1/E1 (S514-4/7/8) Stats ---",
""," ",
"Ta","Read T1/E1/56K alrams", 
"Tallb","E Line Loopback",  
"Tdllb","D Line Loopback",  
"Taplb","E Payload Loopback",  
"Tdplb","D Payload Loopback",  
"Tadlb","E Diag Digital Loopback",  
"Tddlb","D Diag Digital Loopback",  
"Tsalb","Send Loopback Activate Code",  
"Tsdlb","Send Loopback Deactive Code",  
"Tread","Read CSU/DSU cfg",
""," ",
"","--- FT1 (S508/S5143) Stats  ----",
""," ",
"Tv","View Status",
"Ts","Self Test",
"Tl","Line Loop Test",
"Td","Digital Loop Test",
"Tr","Remote Test",
"To","Operational Mode)",
"Tread","Read CSU/DSU cfg",
"."
};
#endif

char *fr_driver_menu[]={
"dd","Display Driver Statistics",
//"ds","Display Send Driver Statistics\n",
"."
};

char *fr_flush_menu[]={
"fg","Flush Global Statistics",
"fc","Flush Communication Error Statistics",
"fe","Flush Error Statistics",
//"fi","Flush DLCI Statistics",
"fd","Flush Driver Statistics",
"fpm","Flush T1/E1 performance monitoring cnters",
"."
}; 


char *fr_main_menu[]={
"fr_card_stats_menu","Card Status",
//"fr_card_config_menu","Card Configuration",
"fr_stats_menu","Card Statistics",
"fr_trace_menu","Trace Data",
"csudsu_menu","CSU DSU Config/Stats",
"fr_driver_menu","Driver Statistics",
"fr_flush_menu","Flush Statistics",
"."
};

static struct cmd_menu_lookup_t fr_cmd_menu_lookup[]={
	{"fr_card_stats_menu",fr_card_stats_menu},
	{"fr_card_config_menu",fr_card_config_menu},
	{"fr_stats_menu",fr_stats_menu},
	{"fr_trace_menu",fr_trace_menu},
	{"csudsu_menu",csudsu_menu},
	{"fr_driver_menu",fr_driver_menu},
	{"fr_flush_menu",fr_flush_menu},
	{".",NULL},
};

char ** FRget_main_menu(int *len)
{
	int i=0;
	while(strcmp(fr_main_menu[i],".") != 0){
		i++;
	}
	*len=i/2;
	return fr_main_menu;
}

char ** FRget_cmd_menu(char *cmd_name,int *len)
{
	int i=0,j=0;
	char **cmd_menu=NULL;
	
	while(fr_cmd_menu_lookup[i].cmd_menu_ptr != NULL){
		if (strcmp(cmd_name,fr_cmd_menu_lookup[i].cmd_menu_name) == 0){
			cmd_menu=fr_cmd_menu_lookup[i].cmd_menu_ptr;
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

void FR_set_FT1_mode( void ){
 
	for(;;){
		wan_udp.wan_udphdr_command = FR_SET_FT1_MODE;
		wan_udp.wan_udphdr_data_len = 0;
		wan_udp.wan_udphdr_return_code = 0xaa;
		wan_udp.wan_udphdr_fr_dlci = 0;
		DO_COMMAND(wan_udp);
		if (wan_udp.wan_udphdr_return_code == 0){
			break;
		}else if (wan_udp.wan_udphdr_return_code == WAN_UDP_INVALID_NET_CMD){
                	printf("Error: Cannot run this command from Underneath.\n");
			exit(1);
		}

	}
} /* set_FT1_mode */

int FRConfig( void ) 
{
   	unsigned char x=0;
	char codeversion[10];
   
	protocol_cb_size=sizeof(wan_mgmt_t) + sizeof(wan_cmd_t) + 1;
   	wan_udp.wan_udphdr_command = FR_READ_CONFIG;
   	wan_udp.wan_udphdr_data_len = 0;
      	wan_udp.wan_udphdr_return_code = 0xaa;
   	wan_udp.wan_udphdr_fr_dlci = 0;
   	while (++x < 4){
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
   	station_config = wan_udp.wan_udphdr_data[0];
	
   	strlcpy(codeversion, "?.??",10);
   
   	wan_udp.wan_udphdr_command = FR_READ_CODE_VERSION;
      	wan_udp.wan_udphdr_return_code = 0xaa;
  	wan_udp.wan_udphdr_data_len = 0;
   	DO_COMMAND(wan_udp);

   	if (wan_udp.wan_udphdr_return_code == 0) {
      		wan_udp.wan_udphdr_data[wan_udp.wan_udphdr_data_len] = 0;
      		strlcpy(codeversion, (char*)wan_udp.wan_udphdr_data,10);
   	}

   	return(WAN_TRUE);
}; 

void FR_read_FT1_status( void ){
     wan_udp.wan_udphdr_command = FPIPE_FT1_READ_STATUS;
     wan_udp.wan_udphdr_return_code = 0xaa;
     wan_udp.wan_udphdr_data_len = 0;

     DO_COMMAND(wan_udp); 

     if( wan_udp.wan_udphdr_return_code == 0 ){
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
     }else{
	     printf("FR Error failed to recieve!\n");
     }
} /* read_FT1_status */

static void error( char return_code ) 
{
	switch( return_code ){
		case 0x04:
			printf("Error: An invalid DLCI was selected\n");
			break;
		case 0x10:
			printf("Error: A modem failure occurred - DCD and/or CTS were found to be unexpectedly low\n");
			break;
		case 0x11:
			printf("Error: The Channel moved from Operative to being Inoperative\n");
			break;
		case 0x12:
			printf("Error: The Channel moved from Inoperative to being Operative\n");
			break;
		case 0x13:
			printf("Error: The Access Node has reported a change in the status of a DLCI or a number of DLCIs\n");
			break;
		case 0x14:
			printf("Error: A Full Status Report included a DLCI or a number of DLCIis which were not included before\n"); 
			break;
		case 0x1F:
			printf("Error: The frame relay command is invalid\n");
			break;
		default:
			break;
   	}		
}; /* error */
 
static void global_stats( void ) 
{
	fr_link_stat_t *link_stats;

	FRConfig();

   	ResetWanUdp(&wan_udp);
   	wan_udp.wan_udphdr_command = FR_READ_STATISTICS;
      	wan_udp.wan_udphdr_return_code = 0xaa;
   	wan_udp.wan_udphdr_data_len = 0;
   	wan_udp.wan_udphdr_fr_dlci = 0;
   	DO_COMMAND(wan_udp);
   
	if (wan_udp.wan_udphdr_return_code != 0){
		error(wan_udp.wan_udphdr_return_code);	
		return;
	}
	
	link_stats=(fr_link_stat_t *)&wan_udp.wan_udphdr_data[0];
	
	if (xml_output){
		output_start_xml_router();
      		if( station_config == 0 ) {
			output_start_xml_header("Global Statistics: CPE");
	 		output_xml_val_data("Full Status Enquiry messages sent",
					link_stats->cpe_tx_FSE);
	 		output_xml_val_data("Link Integrity Verification Status Enquiry messages sent", 
					link_stats->cpe_tx_LIV);
	 		output_xml_val_data("Full Status messages received", 
					link_stats->cpe_rx_FSR);
	 		output_xml_val_data("Link Integrity Verification Status messages received", 
					link_stats->cpe_rx_LIV);
      		} else {
			output_start_xml_header("Global Statistics: Node");
	 		output_xml_val_data("Full Status Enquiry messages received", 
					link_stats->node_rx_FSE);
	 		output_xml_val_data("Link Integrity Verification Status Enquiry messages received", 
					link_stats->node_rx_LIV);
	 		output_xml_val_data("Full Status Reply messages sent", 
					link_stats->node_tx_FSR);
	 		output_xml_val_data("Link Integrity Verification Status messages set", 
					link_stats->node_tx_LIV);
      		} //if
      		output_xml_val_data("CPE initializations", 
					link_stats->cpe_SSN_RSN);
      		output_xml_val_data("Current Send Sequence Number", 
					link_stats->current_SSN);
      		output_xml_val_data("Current Receive Sequence Number", 
					link_stats->current_RSN);
      		output_xml_val_data("Current N392 count", 
					link_stats->current_N392);
      		output_xml_val_data("Current N393 count", 
					link_stats->current_N393);
		output_stop_xml_header();
		output_stop_xml_router();
	}else{
	

      		if( station_config == 0 ) {
			banner("GLOBAL STATISTICS: CPE",0);

	 		printf("                       Full Status Enquiry messages sent: %u\n",
						link_stats->cpe_tx_FSE);
	 		printf("Link Integrity Verification Status Enquiry messages sent: %u\n", 
						link_stats->cpe_tx_LIV);
	 		printf("                           Full Status messages received: %u\n", 
						link_stats->cpe_rx_FSR);
	 		printf("    Link Integrity Verification Status messages received: %u\n", 
						link_stats->cpe_rx_LIV);
      		} else {
			banner("GLOBAL STATISTICS: NODE",0);
	 		printf("                   Full Status Enquiry messages received: %u\n", 
						link_stats->node_rx_FSE);
	 		printf("Link Integrity Verification Status Enquiry mesg received: %u\n", 
						link_stats->node_rx_LIV);
	 		printf("                         Full Status Reply messages sent: %u\n", 
						link_stats->node_tx_FSR);
	 		printf("        Link Integrity Verification Status messages sent: %u\n", 
						link_stats->node_tx_LIV);
      		} //if
      		printf("                                     CPE initializations: %u\n", 
					link_stats->cpe_SSN_RSN);
      		printf("                            Current Send Sequence Number: %u\n", 
					link_stats->current_SSN);
      		printf("                         Current Receive Sequence Number: %u\n", 
					link_stats->current_RSN);
      		printf("                                      Current N392 count: %u\n", 
					link_stats->current_N392);
      		printf("                                      Current N393 count: %u\n", 
					link_stats->current_N393);
   	}
}; /* global_stats */

static void flush_global_stats( void ) 
{
   	wan_udp.wan_udphdr_command = FR_FLUSH_STATISTICS;
      	wan_udp.wan_udphdr_return_code = 0xaa;
   	wan_udp.wan_udphdr_data_len = 1;
   	wan_udp.wan_udphdr_fr_dlci = 0;
	wan_udp.wan_udphdr_data[0] = 0x01;
   	DO_COMMAND(wan_udp);
   
	if (wan_udp.wan_udphdr_return_code != 0) { 
		switch(wan_udp.wan_udphdr_return_code){
			case 0x06:
				printf("Error: Global Statistics not flushed\n");
				break;
			default:
				error(wan_udp.wan_udphdr_return_code);
				break;	  
		}
	}	
}; /* flush_global_stats */

static void error_stats( void ) 
{
	FRConfig();
	ResetWanUdp(&wan_udp);

	wan_udp.wan_udphdr_command = FR_READ_STATISTICS;
      	wan_udp.wan_udphdr_return_code = 0xaa;
   	wan_udp.wan_udphdr_data_len = 0;
   	wan_udp.wan_udphdr_fr_dlci = 0;
   	DO_COMMAND(wan_udp);

	if (wan_udp.wan_udphdr_return_code != 0)
		return;
	

	if (xml_output){
		output_start_xml_router();
		output_start_xml_header("Error Statistics");
		output_xml_val_data("I-frames not transmitted after a tx. int. due to exessive frame length",*(unsigned short*)&wan_udp.wan_udphdr_data[0]);
      		output_xml_val_data("I-frames not transmitted after a tx. int. due to excessive throughput",*(unsigned short*)&wan_udp.wan_udphdr_data[2]);
      		output_xml_val_data("Received frames discarded as they were either too short or too long",*(unsigned short*)&wan_udp.wan_udphdr_data[4]);
      		output_xml_val_data("discarded I-frames with unconfigured DLCI",*(unsigned short*)&wan_udp.wan_udphdr_data[6]);
      		output_xml_val_data("discarded I-frames due to a format error",*(unsigned short*)&wan_udp.wan_udphdr_data[8]);
      		output_xml_val_data("App. didn't respond to the triggered IRQ within the given timeout period",*(unsigned short*)&wan_udp.wan_udphdr_data[10]);
      		output_xml_val_data("discarded In-channel Signalling frames due to a format error",*(unsigned short*)&wan_udp.wan_udphdr_data[28]);
      		output_xml_val_data("In-channel frames received with an invalid Send Seq. Numbers received",*(unsigned short*)&wan_udp.wan_udphdr_data[32]);
      		output_xml_val_data("In-channel frames received with an invalid Receive Seq. Numbers received",*(unsigned short*)&wan_udp.wan_udphdr_data[34]);
      		if( station_config == 0 ) {
	 		output_xml_val_data("Number of unsolicited responses from the Access Node",*(unsigned short*)&wan_udp.wan_udphdr_data[30]);
	 		output_xml_val_data("timeouts on the T391 timer",*(unsigned short*)&wan_udp.wan_udphdr_data[36]);
	 		output_xml_val_data("consecutive timeouts on the T391 timer",*(unsigned short*)&wan_udp.wan_udphdr_data[48]);
      		} else {
	 		output_xml_val_data("timeouts on the T392 timer",*(unsigned short*)&wan_udp.wan_udphdr_data[38]);
	 		output_xml_val_data("consecutive timeouts on the T392 timer",*(unsigned short*)&wan_udp.wan_udphdr_data[50]);
      		} 
      		output_xml_val_data("times that N392 error threshold was reached during N393 monitored events",*(unsigned short*)&wan_udp.wan_udphdr_data[40]);
		output_stop_xml_header();
		output_stop_xml_router();
	}else{
	
		banner("ERROR STATISTICS",0);

      		printf("  I-frames not transmitted after a tx. int. due to exessive frame length: %u\n",*(unsigned short*)&wan_udp.wan_udphdr_data[0]);
      		printf("   I-frames not transmitted after a tx. int. due to excessive throughput: %u\n",*(unsigned short*)&wan_udp.wan_udphdr_data[2]);
      		printf("     Received frames discarded as they were either too short or too long: %u\n",*(unsigned short*)&wan_udp.wan_udphdr_data[4]);
      		printf("                               discarded I-frames with unconfigured DLCI: %u\n",*(unsigned short*)&wan_udp.wan_udphdr_data[6]);
      		printf("                                discarded I-frames due to a format error: %u\n",*(unsigned short*)&wan_udp.wan_udphdr_data[8]);
      		printf("App. didn't respond to the triggered IRQ within the given timeout period: %u\n",*(unsigned short*)&wan_udp.wan_udphdr_data[10]);
      		printf("            discarded In-channel Signalling frames due to a format error: %u\n",*(unsigned short*)&wan_udp.wan_udphdr_data[28]);
      		printf("   In-channel frames received with an invalid Send Seq. Numbers received: %u\n",*(unsigned short*)&wan_udp.wan_udphdr_data[32]);
      		printf("In-channel frames received with an invalid Receive Seq. Numbers received: %u\n",*(unsigned short*)&wan_udp.wan_udphdr_data[34]);
      		if( station_config == 0 ) {
	 		printf("                    Number of unsolicited responses from the Access Node: %u\n",*(unsigned short*)&wan_udp.wan_udphdr_data[30]);
	 		printf("                                              timeouts on the T391 timer: %u\n",*(unsigned short*)&wan_udp.wan_udphdr_data[36]);
	 		printf("                                  consecutive timeouts on the T391 timer: %u\n",*(unsigned short*)&wan_udp.wan_udphdr_data[48]);
      		} else {
	 		printf("                                              timeouts on the T392 timer: %u\n",*(unsigned short*)&wan_udp.wan_udphdr_data[38]);
	 		printf("                                  consecutive timeouts on the T392 timer: %u\n",
					*(unsigned short*)&wan_udp.wan_udphdr_data[50]);
      		} 
      		printf("times that N392 error threshold was reached during N393 monitored events: %u\n",
				*(unsigned short*)&wan_udp.wan_udphdr_data[40]);
	}

}; /* error_stats */

int FRDisableTrace(void)
{
	wan_udp.wan_udphdr_command = FPIPE_DISABLE_TRACING;
      	wan_udp.wan_udphdr_return_code = 0xaa;
	wan_udp.wan_udphdr_data_len = 0;
	wan_udp.wan_udphdr_data[0] = 0;
	DO_COMMAND(wan_udp);
	return 0;
}

#if 0
static void list_all_dlcis(void)
{
	int i;
   	wan_udp.wan_udphdr_command = FR_READ_STATUS;
      	wan_udp.wan_udphdr_return_code = 0xaa;
   	wan_udp.wan_udphdr_data_len = 0;
	wan_udp.wan_udphdr_fr_dlci = 0;
   	DO_COMMAND(wan_udp);

	if (wan_udp.wan_udphdr_return_code != 0) {
		error(wan_udp.wan_udphdr_return_code);
		return;
	}

	if (xml_output){
		output_start_xml_router();
		output_start_xml_header("Global DLCI Status");
		output_xml_val_asc("Channel Status",wan_udp.wan_udphdr_data[0] ? "OPERATIVE" : "INOPERATIVE");	
		if (wan_udp.wan_udphdr_data_len > 2){
			unsigned char str_val[50];
			int cnt_val=0;
			for (i=1;i<wan_udp.wan_udphdr_data_len;){
				output_xml_val_data("DLCI",*((unsigned short*)&wan_udp.wan_udphdr_data[i]));
				if (wan_udp.wan_udphdr_data[i+2] & 0x40){
					cnt_val=snprintf(str_val,50,"Included, ");
				}else{
					cnt_val=snprintf(str_val,50,"Excluded, ");
				}		
			
				cnt_val= snprintf((str_val+cnt_val),50-cnt_val,"%s",
					(wan_udp.wan_udphdr_data[i+2] & 0x01) ? "Deleted" :
					(wan_udp.wan_udphdr_data[i+2] & 0x02) ? "Active" :
					(wan_udp.wan_udphdr_data[i+2] & 0x04) ? "Waiting" :
					(wan_udp.wan_udphdr_data[i+2] & 0x08) ? "New" : "Inactive");
				
				*(str_val+cnt_val)='\0';
				
				output_xml_val_asc("Status",str_val);
			}
		}
		output_stop_xml_header();
		output_stop_xml_router();
	}else{
		banner("GLOBAL DLCI STATUS",0);
		
		printf("  Channel Status: %s\n",wan_udp.wan_udphdr_data[0] ? "OPERATIVE" : "INOPERATIVE");

		if (wan_udp.wan_udphdr_data_len < 2){
			printf("\n  No DLCIs reported by switch!\n");
			return;
		}
		

		printf("\n  DLCI\t\tSTATUS\n");
		for (i=1;i<wan_udp.wan_udphdr_data_len;){

			printf("   %u",
				*((unsigned short*)&wan_udp.wan_udphdr_data[i]));

			if (wan_udp.wan_udphdr_data[i+2] & 0x40){
				printf("\t\tIncluded, ");
			}else{
				printf("\t\tExcluded, ");
			}

			printf("%s\n",
				(wan_udp.wan_udphdr_data[i+2] & 0x01) ? "Deleted" :
				(wan_udp.wan_udphdr_data[i+2] & 0x02) ? "Active" :
				(wan_udp.wan_udphdr_data[i+2] & 0x04) ? "Waiting" :
				(wan_udp.wan_udphdr_data[i+2] & 0x08) ? "New" : "Inactive");
			i+=3;
		}
	}
}
#endif

static void list_configured_dlcis(void)
{
	int i, num_of_dlcis_in_the_list;
	wan_fr_dlci_t	*wan_fr_dlci_ptr;

   	wan_udp.wan_udphdr_command = FR_LIST_CONFIGURED_DLCIS;
      	wan_udp.wan_udphdr_return_code = 0xaa;
   	wan_udp.wan_udphdr_data_len = 0;
	wan_udp.wan_udphdr_fr_dlci = 0;
   	DO_COMMAND(wan_udp);

	if (wan_udp.wan_udphdr_return_code != 0) {
		error(wan_udp.wan_udphdr_return_code);
		return;
	}

	banner("DLCI STATUS LIST",0);
	
	num_of_dlcis_in_the_list = *((int*)wan_udp.wan_udphdr_data);
	wan_fr_dlci_ptr = (wan_fr_dlci_t*)&wan_udp.wan_udphdr_data[sizeof(int)];

	printf("Number of DLCIs in the list: %d\n", num_of_dlcis_in_the_list);

	for (i=0; i < num_of_dlcis_in_the_list; i++){
		printf("%s:\tDLCI %d:\t%sactive\t%s\n", 
	       		wan_fr_dlci_ptr->name,
		        wan_fr_dlci_ptr->dlci,
	       		wan_fr_dlci_ptr->dlci_state & PVC_STATE_ACTIVE ? " " : "in",
	       		wan_fr_dlci_ptr->dlci_state & PVC_STATE_NEW ? " new" : " ");

		wan_fr_dlci_ptr++;
	}
}

static void list_active_dlcis( void )
{
	int i, num_of_dlcis_in_the_list, number_of_active_dlcis;
	wan_fr_dlci_t	*wan_fr_dlci_ptr;

   	wan_udp.wan_udphdr_command = FR_LIST_ACTIVE_DLCI;
      	wan_udp.wan_udphdr_return_code = 0xaa;
   	wan_udp.wan_udphdr_data_len = 0;
	wan_udp.wan_udphdr_fr_dlci = 0;
   	DO_COMMAND(wan_udp);

	if (wan_udp.wan_udphdr_return_code != 0) {
		error(wan_udp.wan_udphdr_return_code);
		return;
	}

	banner("LIST OF ACTIVE DLCIs",0);
	
	num_of_dlcis_in_the_list = *((int*)wan_udp.wan_udphdr_data);
	wan_fr_dlci_ptr = (wan_fr_dlci_t*)&wan_udp.wan_udphdr_data[sizeof(int)];

	number_of_active_dlcis = 0;
	for (i=0; i < num_of_dlcis_in_the_list && wan_fr_dlci_ptr->dlci_state & PVC_STATE_ACTIVE; i++){
		printf("%s:\tDLCI %d:\tactive\n", 
	       		wan_fr_dlci_ptr->name,
		        wan_fr_dlci_ptr->dlci);

		number_of_active_dlcis++;
		wan_fr_dlci_ptr++;
	}

	if(number_of_active_dlcis == 0){
		printf("All DLCIs are Inactive.\n");
	}
 
} /* list_active_dlcis */

static void read_dlci_stat( void )
{
     	wan_udp.wan_udphdr_command = FR_READ_STATISTICS;
      	wan_udp.wan_udphdr_return_code = 0xaa;
     	wan_udp.wan_udphdr_fr_dlci = dlci_number;
     	wan_udp.wan_udphdr_data_len = 1;
     	wan_udp.wan_udphdr_data[0] = 0;
    
	if (!dlci_number){
		printf("Error: Please enter a non-zero DLCI\n");
		return;
	}
	
	DO_COMMAND(wan_udp);
     	if( wan_udp.wan_udphdr_return_code != 0 ){
		error(wan_udp.wan_udphdr_return_code);
		return;
     	}


	if (xml_output){
		char tmp[100];
		snprintf(tmp,100,"Statistics for dlci %i",dlci_number);
		output_start_xml_router();
		output_start_xml_header(tmp);
	
		if( (wan_udp.wan_udphdr_return_code == 0) && (wan_udp.wan_udphdr_data_len == 0x20)){
			output_xml_val_data("Information frames transmitted", 
				*(u_int32_t*)&wan_udp.wan_udphdr_data[0]);
			output_xml_val_data("Information bytes transmitted",
				*(u_int32_t*)&wan_udp.wan_udphdr_data[4]);
			output_xml_val_data("Information frames received", 
				*(u_int32_t*)&wan_udp.wan_udphdr_data[8]);
			output_xml_val_data("Information bytes received", 
				*(u_int32_t*)&wan_udp.wan_udphdr_data[12]);
			output_xml_val_data("Received I-frames discarded due to inactive DLCI", 
				*(u_int32_t*)&wan_udp.wan_udphdr_data[20]);
			output_xml_val_data("I-frames received with Discard Eligibility (DE) indicator set", 
				*(u_int32_t*)&wan_udp.wan_udphdr_data[28]); 
		}
	
		wan_udp.wan_udphdr_command = FR_READ_ADD_DLC_STATS;
      		wan_udp.wan_udphdr_return_code = 0xaa;
		wan_udp.wan_udphdr_fr_dlci = dlci_number;
		wan_udp.wan_udphdr_data_len = 0;
		wan_udp.wan_udphdr_data[0] = 0;
		DO_COMMAND(wan_udp);
	     
		if( wan_udp.wan_udphdr_return_code == 0 ){
			output_xml_val_data("I-frames received with the FECN bit set", 
				*(unsigned short*)&wan_udp.wan_udphdr_data[0]);
			output_xml_val_data("I-frames received with the BECN bit set", 
				*(unsigned short*)&wan_udp.wan_udphdr_data[2]);
		} 
		output_stop_xml_header();
		output_stop_xml_router();
	}else{
	
		banner("STATISTICS FOR DLCI",dlci_number);

		if(wan_udp.wan_udphdr_return_code == 0){
			printf("                                Information frames transmitted: %u\n",
				*(u_int32_t*)&wan_udp.wan_udphdr_data[0]);
			printf("                                 Information bytes transmitted: %u\n", 
				*(u_int32_t*)&wan_udp.wan_udphdr_data[4]);
			printf("                                   Information frames received: %u\n", 
				*(u_int32_t*)&wan_udp.wan_udphdr_data[8]);
			printf("                                    Information bytes received: %u\n", 
				*(u_int32_t*)&wan_udp.wan_udphdr_data[12]);
			printf("              Received I-frames discarded due to inactive DLCI: %u\n", 
				*(u_int32_t*)&wan_udp.wan_udphdr_data[20]);
			printf(" I-frames received with Discard Eligibility (DE) indicator set: %u\n", 
				*(u_int32_t*)&wan_udp.wan_udphdr_data[28]); 
		}
	     
		wan_udp.wan_udphdr_command = FR_READ_ADD_DLC_STATS;
      		wan_udp.wan_udphdr_return_code = 0xaa;
		wan_udp.wan_udphdr_fr_dlci = dlci_number;
		wan_udp.wan_udphdr_data_len = 0;
		wan_udp.wan_udphdr_data[0] = 0;
		DO_COMMAND(wan_udp);
	     
		if( wan_udp.wan_udphdr_return_code == 0 ){
			printf("                       I-frames received with the FECN bit set: %u\n", 
				*(unsigned short*)&wan_udp.wan_udphdr_data[0]);
			printf("                       I-frames received with the BECN bit set: %u\n", 
				*(unsigned short*)&wan_udp.wan_udphdr_data[2]);
		
		} else { 
			printf("Error: Please enter a non-zero DLCI\n");
		}
	}
	
} /* read_dlci_stat */

static void flush_dlci_stats( void )
{
     	wan_udp.wan_udphdr_command = FR_FLUSH_STATISTICS;
      	wan_udp.wan_udphdr_return_code = 0xaa;
     	wan_udp.wan_udphdr_fr_dlci = dlci_number;
     	wan_udp.wan_udphdr_data_len = 0;
     	DO_COMMAND(wan_udp);
     	
	if( wan_udp.wan_udphdr_return_code != 0 ) {
		switch( wan_udp.wan_udphdr_return_code ){
			case 0x06:
				printf("DLCI Statistics are not flushed\n");
				break;
			default:
				error(wan_udp.wan_udphdr_return_code);
		}
	}
} /* flush_dlci_stats */

static void set_FT1_monitor_status( unsigned char status) 
{
	fail = 0;
     	wan_udp.wan_udphdr_command = FR_FT1_STATUS_CTRL;
      	wan_udp.wan_udphdr_return_code = 0xaa;
      	wan_udp.wan_udphdr_data_len = 1;
      	wan_udp.wan_udphdr_data[0] = status; 	
      	DO_COMMAND(wan_udp);
      
	if( wan_udp.wan_udphdr_return_code != 0 && status){
		fail = 1;
		if( wan_udp.wan_udphdr_return_code == 0xCD )
                	printf("Error:  Cannot run this command from Underneath.\n");
		else 
			printf("Error:  This command is only possible with S508/FT1 board!");
      	}

} /* set_FT1_monitor_status */

#if 0
static void fr_driver_stat_ifsend( void )
{
	if_send_stat_t *stats;
      	wan_udp.wan_udphdr_command = FPIPE_DRIVER_STAT_IFSEND;
      	wan_udp.wan_udphdr_return_code = 0xaa;
      	wan_udp.wan_udphdr_data_len = 0;
      	wan_udp.wan_udphdr_data[0] = 0;
      	DO_COMMAND(wan_udp);
    

	if (wan_udp.wan_udphdr_return_code != 0){
		return;
	}	
	
	stats = (if_send_stat_t *)&wan_udp.wan_udphdr_data[0];	


	if (xml_output){
		output_start_xml_router();
		output_start_xml_header("Driver if_send statistics");
		output_xml_val_data("Total Number of Send entries",
				stats->if_send_entry);
#if defined(__LINUX__)
		output_xml_val_data("Number of Send entries with SKB = NULL",
				stats->if_send_skb_null);
#else
		output_xml_val_data("Number of Send entries with mbuf = NULL",
				stats->if_send_skb_null);
#endif
		output_xml_val_data("Number of Send entries with broadcast addressed packet discarded",
				stats->if_send_broadcast);
		output_xml_val_data("Number of Send entries with multicast addressed packet discarded",
			 stats->if_send_multicast);
		output_xml_val_data("Number of Send entries with CRITICAL_RX_INTR set", 
			stats->if_send_critical_ISR);
		output_xml_val_data("Number of Send entries with Critical set and packet discarded", 
			stats->if_send_critical_non_ISR);
		output_xml_val_data("Number of Send entries with Device Busy set", 
			stats->if_send_tbusy);
		output_xml_val_data("Number of Send entries with Device Busy Timeout", 
			stats->if_send_tbusy_timeout);
		output_xml_val_data("Number of Send entries with FPIPE MONITOR Request", 
			stats->if_send_PIPE_request);
		output_xml_val_data("Number of Send entries with WAN Disconnected", 
			stats->if_send_wan_disconnected);
		output_xml_val_data("Number of Send entries with DLCI Disconnected", 
			stats->if_send_dlci_disconnected);
		output_xml_val_data("Number of Send entries with check for Buffers failed", 
			stats->if_send_no_bfrs);
		output_xml_val_data("Number of Send entries with Send failed", 
			stats->if_send_adptr_bfrs_full);
		output_xml_val_data("Number of Send entries with Send passed", 
			stats->if_send_bfr_passed_to_adptr);
		output_xml_val_data("Number of Consecutive send failures for a packet", 
			stats->if_send_consec_send_fail);

		output_stop_xml_header();
		output_stop_xml_router();
	}else{
	
		banner("DRIVER IF_SEND STATISTICS",0);

		printf("                                    Total Number of Send entries:  %u\n",
			stats->if_send_entry);
#if defined(__LINUX__)
		printf("                          Number of Send entries with SKB = NULL:  %u\n", 
			stats->if_send_skb_null);
#else
		printf("                            Number of Send entries with mbuf = NULL:  %u\n", 
			stats->if_send_skb_null);
#endif
		printf("Number of Send entries with broadcast addressed packet discarded:  %u\n",
			 stats->if_send_broadcast);
		printf("Number of Send entries with multicast addressed packet discarded:  %u\n",
			 stats->if_send_multicast);
		printf("                Number of Send entries with CRITICAL_RX_INTR set:  %u\n", 
			stats->if_send_critical_ISR);
		printf("   Number of Send entries with Critical set and packet discarded:  %u\n", 
			stats->if_send_critical_non_ISR);
		printf("                     Number of Send entries with Device Busy set:  %u\n", 
			stats->if_send_tbusy);
		printf("                 Number of Send entries with Device Busy Timeout:  %u\n", 
			stats->if_send_tbusy_timeout);
		printf("               Number of Send entries with FPIPE MONITOR Request:  %u\n", 
			stats->if_send_PIPE_request);
		printf("                    Number of Send entries with WAN Disconnected:  %u\n", 
			stats->if_send_wan_disconnected);
		printf("                   Number of Send entries with DLCI Disconnected:  %u\n", 
			stats->if_send_dlci_disconnected);
		printf("            Number of Send entries with check for Buffers failed:  %u\n", 
			stats->if_send_no_bfrs);
		printf("                         Number of Send entries with Send failed:  %u\n", 
			stats->if_send_adptr_bfrs_full);
		printf("                         Number of Send entries with Send passed:  %u\n", 
			stats->if_send_bfr_passed_to_adptr);
		printf("                   Number of Consecutive send failures for a packet:  %u\n", 
			stats->if_send_consec_send_fail);
	}
} /* fr_driver_stat_ifsend */
#endif

static void link_status( void ) 
{

	hw_link_status();
#if 0
	wan_udp.wan_udphdr_command = FR_READ_STATUS;
	wan_udp.wan_udphdr_data_len = 0;
      	wan_udp.wan_udphdr_return_code = 0xaa;
	wan_udp.wan_udphdr_fr_dlci = 0;
	DO_COMMAND(wan_udp);
	if (wan_udp.wan_udphdr_return_code != 0) {
		error(wan_udp.wan_udphdr_return_code);
		return;
	}

	banner("LINK STATUS", 0);

	if (wan_udp.wan_udphdr_data[0] == WAN_CONNECTED){
		printf("Channel status: OPERATIVE\n");
	}else{
		printf("Channel status: INOPERATIVE\n");
	}	
#endif
}

static void modem_status( void ) 
{
  hw_modem();
}

static void comm_err( void) 
{
  hw_comm_err();
}

static void flush_comm_err( void ) 
{
  hw_flush_comm_err();
}

static void line_trace( int trace_mode, int trace_sub_type)
{
  hw_line_trace(trace_mode);
}

static void fr_driver_stat_gen( void )
{
  hw_general_stats();
}

static void flush_driver_stats( void )
{
  hw_flush_general_stats();
}

static void fr_router_up_time( void )
{
  hw_router_up_time();
}

int FRUsage( void ) 
{
	printf("wanpipemon: Wanpipe Frame Relay Debugging Utility\n\n");
	printf("Usage:\n");
	printf("-----\n\n");
	printf("wanpipemon -i <ip-addr or interface> -u <port> -c <command> -d <dlci num>\n\n");
	printf("\tOption -i: \n");
	printf("\t\tWanpipe remote IP address or\n");
	printf("\t\tWanpipe network interface name (ex: w1g1f16)\n");   
	printf("\tOption -u: (optional, defaults to 9000)\n");
	printf("\t\tWanpipe UDPPORT specified in /etc/wanpipe#.conf\n");
	printf("\tOption -d: \n");
	printf("\t\tDLCI Number: (optional, defaults to 0) \n");
	//printf("\tOption -x: (optional)\n");
	//printf("\t\tDisplay all output/results in XML format\n");
	printf("\tOption -full: (optional trace command)\n");
	printf("\t\tDisplay raw packets in full: default trace pkt len=25bytes\n");
	printf("\tOption -c: \n");
	printf("\t\twanpipemon Command\n"); 
	printf("\t\t\tFirst letter represents a command group:\n"); 
	printf("\t\t\tex: xm = View Modem Status\n");
	printf("\tSupported Commands: x=status : s=statistics : t=trace \n");
	printf("\t                    c=config : T=FT1 stats  : f=flush\n\n");
	printf("\tCommand:  Options:   Description \n");	
	printf("\t-------------------------------- \n\n");    
	printf("\tCard Status\n");
	printf("\t   x         m       Modem Status\n");
	printf("\t             l       Link Status\n");
	printf("\t             ru      Display Router UP time\n");
	printf("\tCard Configuration\n");
	printf("\t   c         l       List Active DLCIs\n");
	printf("\t             lr      List All Reported DLCIs\n");
	printf("\tCard Statistics\n");
	printf("\t   s         g       Global Statistics\n");
	printf("\t             c       Communication Error Statistics\n");
	printf("\t             e       Error Statistics\n");
	printf("\t             d       Read Statistics for a specific DLCI\n");
	printf("\tTrace Data \n");
	printf("\t   t         i       Trace and Interpret frames\n");
	printf("\t             i4      Trace and Interpret ALL frames. Try to decode data as IP V4 frames.\n");
	printf("\t             r       Trace ALL frames, in RAW format\n");
	//printf("\t             ip      Trace and Interpret PROTOCOL frames only\n");
	//printf("\t             id      Trace and Interpret DATA frames only\n");
	//printf("\t             rp      Trace PROTOCOL frames only, in RAW format\n");
	//printf("\t             rd      Trace DATA frames only, in RAW format\n");
	printf("\tFT1/T1/E1/56K Configuration/Statistics\n");
	printf("\t   T         v       View Status \n");
	printf("\t             s       Self Test\n");
	printf("\t             l       Line Loop Test\n");
	printf("\t             d       Digital Loop Test\n");
	printf("\t             r       Remote Test\n");
	printf("\t             o       Operational Mode\n");
	printf("\t             read    Read CSU/DSU configuration (FT1/T1/E1 card)\n");  
	printf("\t             allb    Active Line Loopback mode (T1/E1 card only)\n");  
	printf("\t             dllb    Deactive Line Loopback mode (T1/E1 card only)\n");  
	printf("\t             aplb    Active Payload Loopback mode (T1/E1 card only)\n");  
	printf("\t             dplb    Deactive Payload Loopback mode (T1/E1 card only)\n");  
	printf("\t             adlb    Active Diagnostic Digital Loopback mode (T1/E1 card only)\n");  
	printf("\t             ddlb    Deactive Diagnostic Digital Loopback mode (T1/E1 card only)\n");  
	printf("\t             salb    Send Loopback Activate Code (T1/E1 card only)\n");  
	printf("\t             sdlb    Send Loopback Deactive Code (T1/E1 card only)\n");  
	printf("\t             a       Read T1/E1/56K alrams\n");  
	printf("\tDriver Statistics\n");
	//printf("\t   d         s       Display Send Driver Statistics\n");
	printf("\t   d         d       Display Driver Statistics\n");	
	printf("\tFlush Statistics\n");
	printf("\t   f         g       Flush Global Statistics\n");
	printf("\t             c       Flush Communication Error Statistics\n");
	printf("\t             e       Flush Error Statistics\n");
	printf("\t             i       Flush DLCI Statistics\n");
	printf("\t             d       Flush Driver Statistics\n");
	printf("\t             pm      Flush T1/E1 performance monitoring counters\n");
	printf("\tExamples:\n");
	printf("\t--------\n\n");
	printf("\tex: wanpipemon -i w1g1f16 -u 9000 -c xm   :View Modem Status \n");
	printf("\tex: wanpipemon -i 201.1.1.2 -u 9000 -c ti  :Trace and Interpret frames\n");
	printf("\tex: wanpipemon -i fr17 -u 9000 -c sd -d 16 :Statistics for DLCI 16 \n\n");
	return 0;

}; //usage

int FRMain(char* command, int argc, char* argv[])
{
	char *opt=&command[1];
	int err=0;
   	
	global_command = command;
	global_argc = argc;
	global_argv = argv;

	switch(command[0]){
		case 'x':
			if (!strcmp(opt,"m")){
				modem_status();
			}else if (!strcmp(opt, "l")){
				link_status();
			}else if (!strcmp(opt, "ru")){
				fr_router_up_time();
			}else{
				output_error("Invalid Status Command 'x', Type wanpipemon <cr> for help");
				err=-1;
			}
			break;
		case 's':
			if (!strcmp(opt,"c")){
				comm_err();
			}else if (!strcmp(opt,"g")){
				global_stats();
			}else if (!strcmp(opt,"e")){
				error_stats();
			}else if (!strcmp(opt,"d")){
				read_dlci_stat();
			}else {
				output_error("Invalid Status Command 's', Type wanpipemon <cr> for help");
				err=-1;
			}	
			break;
		case 't':
			if(!strcmp(opt,"i" )){
				raw_data = WAN_FALSE;
				line_trace(TRACE_ALL, WP_OUT_TRACE_RAW);
			}else if (!strcmp(opt, "r")){
				raw_data = WAN_TRUE;
				line_trace(TRACE_ALL, WP_OUT_TRACE_RAW);
			}else if (!strcmp(opt, "i4")){
				raw_data = WAN_FALSE;
				line_trace(TRACE_ALL, WP_OUT_TRACE_INTERP_IPV4);
			/*
			}
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
			*/
			}else{
				output_error("Invalid Status Command 't', Type wanpipemon <cr> for help");
				err=-1;
			}
			break;

		case 'c':
			if (!strcmp(opt, "l")){
				list_active_dlcis();
				
			}else if (!strcmp(opt, "lr")){
				/* list_all_dlcis(); */
				list_configured_dlcis();

			}else if (!strcmp(opt, "lc")){
				list_configured_dlcis();

			}else{
				output_error("Invalid Status Command 'c', Type wanpipemon <cr> for help");
				err=-1;
			}
			break;
			
		case 'd':
			/* Different signature for Driver Statistics */
			//if(!strcmp(opt, "s")){
			//	fr_driver_stat_ifsend();
			//}else
			if (!strcmp(opt, "d")){
				fr_driver_stat_gen();
			}else{
				output_error("Invalid Status Command 'd', Type wanpipemon <cr> for help");
				err=-1;
			}
			break;

		case 'f':
			if (!strcmp(opt, "c")){
				flush_comm_err();
				comm_err();
			}else if (!strcmp(opt, "g")){
				flush_global_stats();
				global_stats();
			}else if (!strcmp(opt, "e")){
				flush_global_stats();
				error_stats();
			}else if (!strcmp(opt, "i")){
				flush_dlci_stats();
				read_dlci_stat();
			}else if (!strcmp(opt, "d")){
				flush_driver_stats();
			}else if (!strcmp(opt,"pm" )){
				flush_te1_pmon();
			}else{
				output_error("Invalid Status Command 'f', Type wanpipemon <cr> for help");
				err=-1;
			}
			break;
		case 'T':
			if (!strcmp(opt, "v")){
				set_FT1_monitor_status(0x01);
				if(!fail){
					view_FT1_status();
				}
				set_FT1_monitor_status(0x00);
			}else if (!strcmp(opt, "s")){
				set_FT1_monitor_status(0x01);
				if(!fail){
					FT1_self_test();
				}
				set_FT1_monitor_status(0x00);
			}else if (!strcmp(opt, "l")){
				set_FT1_monitor_status(0x01);
				if(!fail){
					FT1_local_loop_mode();
				}
				set_FT1_monitor_status(0x00);
			}else if (!strcmp(opt, "d")){
				set_FT1_monitor_status(0x01);
				if(!fail){
					FT1_digital_loop_mode();
				}
				set_FT1_monitor_status(0x00);
			}else if (!strcmp(opt, "r")){
				set_FT1_monitor_status(0x01);
				if(!fail){
					FT1_remote_test();		
				}
				set_FT1_monitor_status(0x00);	
			}else if (!strcmp(opt, "o")){
				set_FT1_monitor_status(0x01);
				if(!fail){
					FT1_operational_mode();
				}
				set_FT1_monitor_status(0x00);
			
			}else if (!strcmp(opt,"read")){
				read_te1_56k_config();
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
			}else{
				output_error("Invalid Status Command 'T', Type wanpipemon <cr> for help");
				err=-1;
			}
			break;	
		default:
			output_error("ERROR: Invalid Command, Type wanpipemon <cr> for help");
			err=-1;
			break;

	} //switch

	fflush(stdout);
	return err;
}; //main

/*
 * EOF wanpipemon.c
 */
