/*****************************************************************************
* atmpipemon.c	ATM Debugger/Monitor
*
* Author:       Nenad Corbic <ncorbic@sangoma.com>	
*
* Copyright:	(c) 2003 Sangoma Technologies Inc.
*
*		This program is free software; you can redistribute it and/or
*		modify it under the terms of the GNU General Public License
*		as published by the Free Software Foundation; either version
*		2 of the License, or (at your option) any later version.
* ----------------------------------------------------------------------------
* Jul 05, 2004	David Rokhvarg	Added i4, iphy, rphy options.
* Jan 06, 2003	Nenad Corbic	Initial version based on cpipemon
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
# include <linux/sdla_atm.h>
#else
# include <wanpipe_defines.h>
# include <wanpipe_cfg.h>
# include <wanpipe.h>
# include <sdla_atm.h>
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

/* Link addresses */
#define CISCO_ADDR_UNICAST      0x0F
#define CISCO_ADDR_BCAST        0x8F

/* Packet Types */
#define CISCO_PACKET_IP         0x0800
#define CISCO_PACKET_SLARP      0x8035
#define CISCO_PACKET_CDP        0x2000

/* Packet Codes */
#define SLARP_REQUEST   0
#define SLARP_REPLY     1
#define SLARP_KEEPALIVE 2

#define BANNER(str)  banner(str,0)
/******************************************************************************
 * 			TYPEDEF/STRUCTURE				      *
 *****************************************************************************/
/* Structures for data casting */

/* types of ATM cells */
enum ATM_CELL_TYPE{
	ATM_UNKNOWN_CELL,
	ATM_IDLE_CELL,
	ATM_PHYSICAL_LAYER_OAM_CELL,
	ATM_RESERVED_FOR_PHYSICAL_LAYER_CELL,
	ATM_UNASSIGNED_CELL,
	ATM_USER_DATA_CELL,
	ATM_OAM_F5_CELL
};

#pragma pack(1)

#define LENGTH_ATM_CELL			53
#define LENGTH_PAYLOAD_ATM_CELL		48

typedef struct {
	unsigned char GFC_VPI;
	unsigned char VPI_VCI;
	unsigned char VCI;
	unsigned char VCI_PT_CLP;
	unsigned char HEC;
} ATM_HEADER_STRUCT;

#define byte0 GFC_VPI
#define byte1 VPI_VCI
#define byte2 VCI
#define byte3 VCI_PT_CLP
#define byte4 HEC

typedef struct {
	ATM_HEADER_STRUCT ATM_header;
	unsigned char ATM_payload[LENGTH_PAYLOAD_ATM_CELL];
} ATM_CELL_STRUCT;
#pragma pack()

typedef struct{

	unsigned char UU;
	unsigned char CPI;
	unsigned short length;
	unsigned long CRC;

}AAL5_CPCS_PDU_TRAILER;

/******************************************************************************
 * 			GLOBAL VARIABLES				      *
 *****************************************************************************/
/* The ft1_lib needs these global variables */
static int gfail;

/******************************************************************************
 * 			FUNCTION PROTOTYPES				      *
 *****************************************************************************/
static void error( void );

/* Command routines */
static void modem( void );
static void global_stats( void );
static void comm_stats( void );
static void read_code_version( void );
static void atm_configuration( void );
static void link_status( void );
static void operational_stats( void );
static void line_trace( int trace_mode, int trace_sub_type);
static void atm_router_up_time( void );
static void flush_operational_stats( void );
static void flush_global_stats( void );
static void flush_comm_stats( void );
static void set_FT1_monitor_status( unsigned char);
static void read_ft1_te1_56k_config( void );

/* Other routines */
static int get_cell_type(char* data, int len);
void decode_user_data_cell(unsigned char* data, int len);

static char *gui_main_menu[]={
"atm_card_stats_menu","Card Status",
"atm_card_config_menu","Card Configuration",
"atm_stats_menu", "Card Statistics",
"atm_trace_menu", "Trace Data",
"csudsu_menu", "CSU DSU Config/Stats",
"atm_flush_menu","Flush Statistics",
"."
};


static char *atm_card_stats_menu[]={
"xm","Modem Status",
"xl","Link Status",
"xcv","Read Code Version",
"xru","Display Router UP time",
"."
};
	
static char *atm_card_config_menu[]={
"crc","Read Configuration\n",
"."
};

static char *atm_stats_menu[]={
"sg","Global Statistics",
"sc","Communication Error Statistics",
"so","Operational Statistics",
"."
};

static char *atm_trace_menu[]={
"ti4", "Interpreted IP Data frames",
"tr",  "Raw Hex, Data packets",
"trl2","Raw Hex, ATM-PDU packets",
"trl1","Raw Hex, ATM Cells",
"."
};

static char *atm_flush_menu[]={
"fg","Flush Global Statistics",
"fc","Flush Communication Error Statistics",
"fo","Flush Operational Statistics",
"."
};


static struct cmd_menu_lookup_t gui_cmd_menu_lookup[]={
	{"atm_card_stats_menu",atm_card_stats_menu},
	{"atm_card_config_menu",atm_card_config_menu},
	{"atm_stats_menu",atm_stats_menu},
	{"atm_trace_menu",atm_trace_menu},
	{"csudsu_menu",csudsu_menu},
	{"atm_flush_menu",atm_flush_menu},
	{".",NULL}
};


char ** ATMget_main_menu(int *len)
{
	int i=0;
	while(strcmp(gui_main_menu[i],".") != 0){
		i++;
	}
	*len=i/2;
	return gui_main_menu;
}

char ** ATMget_cmd_menu(char *cmd_name,int *len)
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
int ATMConfig(void)
{
	char codeversion[10];
	unsigned char x=0;
   
	protocol_cb_size = sizeof(wan_mgmt_t) + 
			   sizeof(wan_cmd_t) + 
			   sizeof(wan_trace_info_t) + 1;
	wan_udp.wan_udphdr_command= PHY_READ_CONFIGURATION;
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

	if (wan_udp.wan_udphdr_data_len == sizeof(PHY_CONFIGURATION_STRUCT)) {
		is_508 = WAN_TRUE;
	} else {
		is_508 = WAN_FALSE;
	} 
   
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

void ATM_set_FT1_mode( void )
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

void ATM_read_FT1_status( void )
{

	wan_udp.wan_udphdr_command= FT1_READ_STATUS;
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

		printf("Number of receiver overrun errors:  %u\n", comm_stats->Rx_overrun_err_count);
		printf("Number of times DCD changed state:  %u\n", comm_stats->DCD_state_change_count);
		printf("Number of times CTS changed state:  %u\n", comm_stats->CTS_state_change_count);
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
	wan_udp.wan_udphdr_command= READ_CODE_VERSION; 
	wan_udp.wan_udphdr_return_code = 0xaa;
	wan_udp.wan_udphdr_data_len = 0;
	DO_COMMAND(wan_udp);

	if (wan_udp.wan_udphdr_return_code == 0) {
		BANNER("CODE VERSION");
		printf("Code version: %s\n",
				wan_udp.wan_udphdr_data);
	}else{
		printf("Error: Rc=%x\n",wan_udp.wan_udphdr_return_code);
	}
}; /* read code version */

static void atm_configuration (void)
{
	PHY_CONFIGURATION_STRUCT *cfg;
	wan_udp.wan_udphdr_command = PHY_READ_CONFIGURATION; 
	wan_udp.wan_udphdr_return_code = 0xaa;
	wan_udp.wan_udphdr_data_len = 0;
	DO_COMMAND(wan_udp);

	cfg = (PHY_CONFIGURATION_STRUCT *)&wan_udp.wan_udphdr_data[0];

	BANNER("ATM CONFIGURATION");
	
	if (wan_udp.wan_udphdr_return_code == 0) {

		printf("%-25s: %-8lu  \t%-25s: %s\n",
				"Baud rate",cfg->baud_rate,
				"Line configuration",
				 (cfg->line_config_options==INTERFACE_LEVEL_V35)?
					 "V.35":"RS-232");
	

		printf("%-25s: 0x%-8X\t%-25s: %u\n",
				"Modem config options",cfg->modem_config_options,
				"Modem Status Timer",cfg->modem_status_timer);
		
		printf("%-25s: %-8u  \t%-25s: 0x%-8X\n",
				"Modem Status Timer",cfg->modem_status_timer,
			        "API Options",cfg->API_options);
		

		printf("%-25s: 0x%-8X\t%-25s: 0x%-8X\n",
				"Protocol Options",cfg->protocol_options,
				"HEC Options",cfg->HEC_options);		
			
		printf("%-25s: 0x%-8X\t%-25s: 0x%-8X\n",
				"Custom RX COSET",cfg->custom_Rx_COSET,
				"Custom TX COSET",cfg->custom_Tx_COSET);
		
		printf("%-25s: 0x%-8X\t%-25s: %u\n",
				"Buffer Options",cfg->buffer_options,
				"Max Cells in Tx Block", cfg->max_cells_in_Tx_block);
		
		printf("%-25s: %-8u  \t%-25s: %-8u\n",
				"Tx Underrun Cell GFC",cfg->Tx_underrun_cell_GFC,
				"Tx Underrun Cell PT",cfg->Tx_underrun_cell_PT);
			
		
		printf("%-25s: %-8u  \t%-25s: %-8u\n",
				"Tx Underrun Cell CLP",cfg->Tx_underrun_cell_CLP,
				"Tx Underrun Cell Payload",cfg->Tx_underrun_cell_payload);	
			
		printf("%-25s: %-8u  \t%-25s: %-8u\n",
				"Mac Cells in Rx Block",cfg->max_cells_in_Rx_block,
				"Rx Hunt Timer",cfg->Rx_hunt_timer);
		
		printf("%-25s: 0x%-8X\t%-25s: %u\n",
				"Rx Sync Bytes",cfg->Rx_sync_bytes,
				"Rx Sync Offset",cfg->Rx_sync_offset);	
			
		printf("%-25s: %-8u  \t%-25s: %-8u\n",
				"Cell Rx sync loss timer",cfg->cell_Rx_sync_loss_timer,
				"Rx HEC Check Timer",cfg->Rx_HEC_check_timer);
			
		printf("%-25s: %-8u  \t%-25s: %-8u\n",
				"Rx Bad HEC Timer",cfg->Rx_bad_HEC_timer,
				"Rx Max Bad HEC Count",cfg->Rx_max_bad_HEC_count);	
		
		printf("%-25s: 0x%-8X",
				"Statistics Options",cfg->statistics_options);	
		
	} else {
		error();
	}
}; /* atm_configuration */


static void link_status (void)
{
	wan_udp.wan_udphdr_command= ATM_LINK_STATUS; 
	wan_udp.wan_udphdr_return_code = 0xaa;
	wan_udp.wan_udphdr_data_len = 0;
	DO_COMMAND(wan_udp);
	
	BANNER("LINK STATUS");
	
	if (wan_udp.wan_udphdr_return_code == 0){
		printf("ATM PHY Status:	%s\n", 
		wan_udp.wan_udphdr_data[0] ? "Connected":"Disconnected");
		printf("ATM SAR Status:	%s\n",
		 (wan_udp.wan_udphdr_data[1]==ATM_CONNECTED)?"Connected":
		 (wan_udp.wan_udphdr_data[1]==ATM_DISCONNECTED)?"Disconnected":
		 (wan_udp.wan_udphdr_data[1]==ATM_AIS)?"Alarm AIS":
		 "Unknown");
	}
	
	return;
}; /* Link Status */		


static void operational_stats (void)
{
	PHY_OPERATIONAL_STATS_STRUCT *stats;
	wan_udp.wan_udphdr_command= PHY_READ_OPERATIONAL_STATS; 
	wan_udp.wan_udphdr_return_code = 0xaa;
	wan_udp.wan_udphdr_data_len = 0;
	DO_COMMAND(wan_udp);

	if (wan_udp.wan_udphdr_return_code == 0) {
		BANNER("OPERATIONAL STATISTICS");
		stats = (PHY_OPERATIONAL_STATS_STRUCT *)&wan_udp.wan_udphdr_data[0];
 
		printf(    "    Number of ATM Cell blocks transmitted:   %lu",stats->blocks_Tx_count);
		printf(  "\n     Number of ATM Cell bytes transmitted:   %lu",stats->bytes_Tx_count);
		printf(  "\n     Transmit (idle+data) Throughput(bps):   %lu",stats->Tx_throughput);
		printf(  "\n                   Tx Underrun cell count:   %lu",stats->Tx_underrun_cell_count);

		printf("\n\n       Number of ATM Cell blocks received:   %lu",stats->blocks_Rx_count);
		printf(  "\n        Number of ATM Cell bytes received:   %lu",stats->bytes_Rx_count);
		printf(  "\n      Receive (idle+data) Throughput(bps):   %lu",stats->Rx_throughput);

		printf("\n\n");
		printf("%-25s: %-8lu  \t%-25s: %-8lu\n",
				"TX cell frames discarded",stats->Tx_length_error_count,
				"RX ATM Cell frames disc",stats->Rx_blocks_discard_count);

		printf("%-25s: %-8lu  \t%-25s: %-8lu\n",
				"Rx Idle Cell discarded",stats->Rx_Idle_Cell_discard_count,
				"Rx Unassigned Cell disc",stats->Rx_Unas_Cell_discard_count);
		
		printf("%-25s: %-8lu  \t%-25s: %-8lu\n",
				"Rx Phys Lyr Cell disc",stats->Rx_Phys_Lyr_Cell_discard_count,
				"Rx_bad_HEC_count",stats->Rx_bad_HEC_count);
		

		printf("%-25s: %-8lu  \t%-25s: %-8lu\n",
				"Rx_sync_attempt_count",stats->Rx_sync_attempt_count,
				"Rx_sync_achieved_count",stats->Rx_sync_achieved_count);
		
		printf("%-25s: %-8lu  \t%-25s: %-8lu\n",
				"Rx_sync_failure_count",stats->Rx_sync_failure_count,
				"Rx_hunt_attempt_count",stats->Rx_hunt_attempt_count);
		
		printf("%-25s: %-8lu  \t%-25s: %-8lu\n",
				"Rx_hunt_char_sync_count",stats->Rx_hunt_char_sync_count,
				"Rx_hunt_timeout_count",stats->Rx_hunt_timeout_count);

		printf("%-25s: %-8lu  \t%-25s: %-8lu\n",
				"Rx_hunt_achieved_count",stats->Rx_hunt_achieved_count,
				"Rx_hunt_failure_count",stats->Rx_hunt_failure_count);

		printf("%-25s: %-8lu  \t%-25s: %-8lu\n",
				"Rx_presync_attempt_count",stats->Rx_presync_attempt_count,
				"Rx_presync_achieved_count",stats->Rx_presync_achieved_count);
		
		printf("%-25s: %-8lu  \t%-25s: %-8lu\n",
				"Rx_presync_failure_count",stats->Rx_presync_failure_count,
				"Rx_resync_bad_HEC_count",stats->Rx_resync_bad_HEC_count);
		
		printf("%-25s: %-8lu  \t%-25s: %-8lu\n",
				"Rx_resync_rx_loss_count",stats->Rx_resync_reception_loss_count,
				"Rx_resync_overrun_count",stats->Rx_resync_overrun_count);	

	} else {
		error();
	}
} /* Operational_stats */


static void flush_operational_stats( void ) 
{
	wan_udp.wan_udphdr_command= PHY_FLUSH_OPERATIONAL_STATS;
	wan_udp.wan_udphdr_return_code = 0xaa;
	wan_udp.wan_udphdr_data_len = 0;
	DO_COMMAND(wan_udp);

	if (wan_udp.wan_udphdr_return_code != 0 ) {
		error();
	}

}; /* flush_operational_stats */


int ATMDisableTrace(void)
{
	wan_udp.wan_udphdr_command= DISABLE_TRACING;
	wan_udp.wan_udphdr_return_code = 0xaa;
	wan_udp.wan_udphdr_data_len = 0;
	DO_COMMAND(wan_udp);
	return 0;
}


static void line_trace( int trace_mode, int trace_sub_type)
{
	unsigned int num_frames, num_chars;
	unsigned short curr_pos = 0;
	wan_trace_pkt_t *trace_pkt;
	unsigned int i, j;
	int recv_buff = sizeof(wan_udp_hdr_t)+ 100;
	fd_set ready;
	struct timeval to;
	int count;
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
		     
			num_frames = wan_udp.wan_udphdr_atm_num_frames;

		     	for ( i = 0; i < num_frames; i++) {
				trace_pkt= (wan_trace_pkt_t *)&wan_udp.wan_udphdr_data[curr_pos];

				/*  frame type */
				if (trace_pkt->status & 0x01) {
					printf("\nOUTGOING\t");
				}else{
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
			
				if (!trace_all_data){
					if (trace_mode != 2){
						num_chars = (unsigned char)
							((trace_pkt->real_length <= 53)
							? trace_pkt->real_length : 53);
					}else{
						num_chars = trace_pkt->real_length;
					}
				}else{
					num_chars = trace_pkt->real_length;
				}

	  			trace_iface.link_type = WANCONFIG_ATM;
				trace_iface.type = WP_OUT_TRACE_INTERP;
	        		trace_iface.sub_type = trace_sub_type;

			    	trace_iface.trace_all_data = trace_all_data;
			    	trace_iface.data=trace_pkt->data;
					
          			trace_iface.len = trace_pkt->real_length;
          			//trace_iface.len = num_chars;//trace_pkt->real_length;
        			
	  		  	wp_trace_output(&trace_iface);

  				count = 0;
				//printf("\t");

				//WP_OUT_TRACE_ATM_INTERPRETED_PHY
				if(trace_mode == 2 || trace_mode == 3){
					
					if(trace_sub_type == WP_OUT_TRACE_ATM_INTERPRETED_PHY){
						//printf("\n\t");
						//printf("\t");
						printf("\t\tHeader\t   : ");

				  		j = 3;		
				  		printf("GFC=%d ",
					  		(trace_pkt->data[j-3] & 0xF0) >> 4 );
						
				  		printf("VPI=%d ",
					  		trace_pkt->data[j-3]<<4 |
						  	(trace_pkt->data[j-2]&0xF0)>>4);

						printf("VCI=%d ",
							(trace_pkt->data[j-2]&0x0F)<<12 |
							(trace_pkt->data[j-1]<<4)|
	  						(trace_pkt->data[j-0]&0xF0)>>4);
							
		  				printf("PT=0x%X ",
			  				(trace_pkt->data[j]&0x0F)>>1);
							
				  		printf("CLP=0x%X ",
						  		trace_pkt->data[j] & 0x01);

					  	printf("HEC=0x%02X ",
						  	trace_pkt->data[j+1]);
						

						printf("Cell Type: ");
						switch(get_cell_type((char*)trace_pkt->data, trace_pkt->real_length))
						{
						case ATM_USER_DATA_CELL:
							decode_user_data_cell(trace_pkt->data,
										trace_pkt->real_length);
							break;
						default:
							;//do nothting
						}
					}
					//printf("\n\t\t");
				}//if()
	

			  	printf("\n");
			  	fflush(stdout);

			}//for ( i = 0; i < num_frames; i++)
    		}
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


static int get_cell_type(char* data, int len)
{
	ATM_HEADER_STRUCT * atm_hdr_ptr = (ATM_HEADER_STRUCT * )data;
	ATM_CELL_STRUCT * ATM_cell_ptr = (ATM_CELL_STRUCT * )data;

	unsigned short gfc_vpi = ((ATM_cell_ptr->ATM_header.GFC_VPI << 4) & 0x0FF0) |
				((ATM_cell_ptr->ATM_header.VPI_VCI >> 4) & 0x000F);

	unsigned short	vci = ((ATM_cell_ptr->ATM_header.VPI_VCI << 12) & 0xF000) |
				((ATM_cell_ptr->ATM_header.VCI << 4) & 0x0FF0) |
					((ATM_cell_ptr->ATM_header.VCI_PT_CLP >> 4) & 0x000F);

	unsigned char pti = ((ATM_cell_ptr->ATM_header.VCI_PT_CLP >> 1) & 0x07);

	unsigned char clp = ATM_cell_ptr->ATM_header.VCI_PT_CLP & 0x01;

	//unsigned char hec = ATM_cell_ptr->ATM_header.HEC;

	if(!(gfc_vpi & 0x00FF) && !vci){
		
		if(	!atm_hdr_ptr->byte0 &&
			!atm_hdr_ptr->byte1 && 
			!atm_hdr_ptr->byte2){
			
			if(!(atm_hdr_ptr->byte3 & 0xFE) && (clp & 0x01) ){
			
				printf("Idle");
				return ATM_IDLE_CELL;
			}

			if(!(atm_hdr_ptr->byte3 & 0xFC) && (atm_hdr_ptr->byte3 & 0x03) ){
			
				printf("PHY Layer");
				return ATM_PHYSICAL_LAYER_OAM_CELL;
			}
		}
		
		if(	!(atm_hdr_ptr->byte0 & 0x0F)	&&
			!atm_hdr_ptr->byte1		&& 
			!atm_hdr_ptr->byte2		&&
			!(atm_hdr_ptr->byte3 & 0xF0)){
			
			if(atm_hdr_ptr->byte3 & 0x01  ){
	
				printf("PHY Layer");
				return ATM_RESERVED_FOR_PHYSICAL_LAYER_CELL;
			}else{
				
				printf("Unassigned");
				return ATM_UNASSIGNED_CELL;
			}
		}
	}else{
		if(!(pti & 0x04)){
			
			printf("User Data");
			return ATM_USER_DATA_CELL;
		}else if(pti & 0x04){

			printf("OAM");
			return ATM_OAM_F5_CELL;
		}
	}

	printf("Unknown");
	return ATM_UNKNOWN_CELL;
}

void decode_user_data_cell(unsigned char* data, int len)
{
	int i;
	unsigned char* puchar_tmp;
	ATM_CELL_STRUCT * ATM_cell_ptr = (ATM_CELL_STRUCT * )data;
	AAL5_CPCS_PDU_TRAILER * trailer_ptr = (AAL5_CPCS_PDU_TRAILER * )&data[45];
	unsigned char pti = ((ATM_cell_ptr->ATM_header.VCI_PT_CLP >> 1) & 0x07);

	if(pti & 0x01){
		
		printf("\n\t\tPDU trailer: ");
		printf("UU=0x%02X ", trailer_ptr->UU);
		printf("CPI=0x%02X ", trailer_ptr->CPI);
		printf("length=%u ", ntohs(trailer_ptr->length));
		
		printf("CRC=0x");
		puchar_tmp = (unsigned char*)&trailer_ptr->CRC;

		for(i = 0; i < 4; i++){
			printf("%02X", *puchar_tmp);
			puchar_tmp++;
		}
	}
}
/*
if(!strcmp(opt,"il3" )){
	line_trace(0, WP_OUT_TRACE_RAW);
}else if (!strcmp(opt, "rl2")){
	line_trace(1, WP_OUT_TRACE_RAW);
*/

//CORBA
int ATMUsage(void)
{
	printf("wanpipemon: Wanpipe ATM Debugging Utility\n\n");
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
	printf("\tCard Configuration\n");
	printf("\t   c         rc      Read CHDLC Configuration\n");
	printf("\tCard Statistics\n");
	printf("\t   s         g       Global Statistics\n");
	printf("\t             c       Communication Error Statistics\n");
	printf("\t             o       Operational Statistics\n");
	printf("\t             s       SLARP and CDP Statistics\n");
	printf("\tTrace Data \n");
	printf("\t   t         rl3     Trace DATA frames only (no header), in RAW format\n");
	printf("\t             rl2     Trace DATA frames only (display header), in RAW format\n");
	printf("\t             rl1     Trace PHY level cells, in RAW format (surpress idle)\n");
	printf("\t             il1     Trace PHY level cells, in Interpreted format (surpress idle)\n");
	printf("\t             rphy    Trace PHY level cells, in RAW format\n");
	printf("\t             iphy    Trace PHY level cells, in Interpreted format.\n");
	printf("\t             i4      Trace DATA frames only. Interpret data as IP V4 frames.\n");
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
	printf("\t             s       Flush SLARP and CDP Statistics\n");
	printf("\t             p       Flush T1/E1 performance monitoring counters\n");
	printf("\tExamples:\n");
	printf("\t--------\n\n");
	printf("\tex: wanpipemon -i wp1_atm -u 9000 -c xm :View Modem Status \n");
	printf("\tex: wanpipemon -i 201.1.1.2 -u 9000 -c ti  :Trace and Interpret ALL frames\n\n");
	return 0;
}

static void atm_router_up_time( void )
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
	}
	return;
}


int ATMMain(char *command,int argc, char* argv[])
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
				atm_router_up_time();
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
				atm_configuration();
			}else{
				printf("ERROR: Invalid Configuration Command 'c', Type wanpipemon <cr> for help\n\n");
			}
			break;
		case 't':
			if(!strcmp(opt,  "rl3" )){
				line_trace(0, WP_OUT_TRACE_RAW);
			}else if (!strcmp(opt, "rl2")){
				line_trace(1, WP_OUT_TRACE_RAW);
			}else if (!strcmp(opt, "rl1")){
				line_trace(2, WP_OUT_TRACE_RAW);
			}else if (!strcmp(opt, "il1")){
				line_trace(2, WP_OUT_TRACE_ATM_INTERPRETED_PHY);
			}else if (!strcmp(opt, "rphy")){
				line_trace(3, WP_OUT_TRACE_ATM_RAW_PHY);
			}else if (!strcmp(opt, "iphy")){
				line_trace(3, WP_OUT_TRACE_ATM_INTERPRETED_PHY);
			}else if (!strcmp(opt, "i4")){
				line_trace(0, WP_OUT_TRACE_INTERP_IPV4);
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
			}else if (!strcmp(opt, "f")){
				set_FT1_monitor_status(0x04);
				if(!gfail){
					printf("Flushed Operational Statistics\n");
				}else{
					printf("FT1 Flush Failed %i\n",gfail);
				}

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
