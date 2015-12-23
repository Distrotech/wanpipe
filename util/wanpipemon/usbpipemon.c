/*****************************************************************************
* usbpipemon.c	USB-FXO Debugger/Monitor
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
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#if defined(__FreeBSD__)
# include <limits.h>
#endif
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <net/if.h>
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
# include <linux/wanpipe_usb.h>
#else
# include <wanpipe_defines.h>
# include <wanpipe_cfg.h>
# include <wanpipe.h>
# include <wanpipe_usb.h>
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
#if 0
static void global_stats( void );
#endif
static void comm_err_stats( void );
static void read_code_version( void );
static void operational_stats( void );
static void flush_operational_stats( void );
static void flush_comm_err_stats( void );

static int usb_remora_tones(int);
static int usb_remora_ring(int);
static int usb_remora_regdump(int);

static char *gui_main_menu[]={
"usb_card_stats_menu","Card Status",
"usb_stats_menu", "Card Statistics",
"usb_flush_menu","Flush Statistics",
"."
};

static char *usb_card_stats_menu[]={
"xru","Display Router UP time",
"."
};
	
static char *usb_stats_menu[]={
"sc","Communication Error Statistics",
"so","Operational Statistics",
"."
};

static char *usb_flush_menu[]={
"fc","Flush Communication Error Statistics",
"fo","Flush Operational Statistics",
"."
};

static struct cmd_menu_lookup_t gui_cmd_menu_lookup[]={
	{"usb_card_stats_menu", usb_card_stats_menu},
	{"usb_stats_menu",	usb_stats_menu},
	{"usb_flush_menu",	usb_flush_menu},
	{".",NULL}
};


char ** USBget_main_menu(int *len)
{
	int i=0;
	while(strcmp(gui_main_menu[i],".") != 0){
		i++;
	}
	*len=i/2;
	return gui_main_menu;
}

char ** USBget_cmd_menu(char *cmd_name,int *len)
{
	int i=0,j=0;
	char **cmd_menu=NULL;
	
	while(gui_cmd_menu_lookup[i].cmd_menu_ptr != NULL){
		if (strcmp(cmd_name,gui_cmd_menu_lookup[i].cmd_menu_name) == 0){
			cmd_menu=(char**)gui_cmd_menu_lookup[i].cmd_menu_ptr;
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
int USBConfig(void)
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
   
	strlcpy(codeversion, "?.??",10);
   
	wan_udp.wan_udphdr_command = READ_CODE_VERSION;
	wan_udp.wan_udphdr_data_len = 0;
	wan_udp.wan_udphdr_return_code = 0xaa;
	DO_COMMAND(wan_udp);
	if (wan_udp.wan_udphdr_return_code == 0) {
		wan_udp.wan_udphdr_data[wan_udp.wan_udphdr_data_len] = 0;
		strlcpy(codeversion, (char*)wan_udp.wan_udphdr_data,10);
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


static void comm_err_stats (void)
{
	wan_udp.wan_udphdr_command= READ_COMMS_ERROR_STATS;
	wan_udp.wan_udphdr_return_code = 0xaa;
	wan_udp.wan_udphdr_data_len = 0;
	printf("Please wait...\n");
	DO_COMMAND(wan_udp);
	
	if (wan_udp.wan_udphdr_return_code == 0 && wan_udp.wan_udphdr_data_len == sizeof(wp_usb_comm_err_stats_t)){
		wp_usb_comm_err_stats_t	*stats = (wp_usb_comm_err_stats_t*)&wan_udp.wan_udphdr_data[0];
		
		BANNER("USB-FXO COMMUNICATION ERROR STATISTICS");
		printf("Common Stats:\n");
		printf("              Number of command overrun errors: %d\n", stats->cmd_overrun);
		printf("              Number of command timeout errors: %d\n", stats->cmd_timeout);
		printf("              Number of invalid command errors: %d\n", stats->cmd_invalid);
		printf("               Number of core not ready errors: %d\n", stats->core_notready_cnt);
		printf("                               USB FIFO Status: %02X\n", stats->fifo_status);
		printf("                               USB UART Status: %02X\n", stats->uart_status);
		printf("                             USB HOSTIF Status: %02X\n", stats->hostif_status);
		
		printf("RX Stats:\n");
		printf("         Number of receiver Out-of-Sync errors: %d\n", stats->rx_sync_err_cnt);
		printf("   Number of receiver Start Framing Bit errors: %d\n", stats->rx_start_fr_err_cnt);
		printf("           Number of receiver Start Bit errors: %d\n", stats->rx_start_err_cnt);
		printf("       Number of receiver Reset Command errors: %d\n", stats->rx_cmd_reset_cnt);
		printf("        Number of receiver Command Drop errors: %d\n", stats->rx_cmd_drop_cnt);
		printf("     Number of receiver Unknown command errors: %d\n", stats->rx_cmd_unknown);
		printf("             Number of receiver overrun errors: %d\n", stats->rx_overrun_cnt);
		printf("             Number of receiver underun errors: %d\n", stats->rx_underrun_cnt);
		
		printf("TX Stats:\n");
		printf("          Number of transmitter overrun errors: %d\n", stats->tx_overrun_cnt);
		printf("        Number of transmitter not ready errors: %d\n", stats->tx_notready_cnt);
		
	}
	return;
}; /* comm_err_stats */


static void flush_comm_err_stats( void ) 
{
	wan_udp.wan_udphdr_command= FLUSH_COMMS_ERROR_STATS;
	wan_udp.wan_udphdr_return_code = 0xaa;
	wan_udp.wan_udphdr_data_len = 0;
	DO_COMMAND(wan_udp);
}; /* flush_comm_err_stats */


static void read_code_version (void)
{
	wan_udp.wan_udphdr_command= READ_CODE_VERSION; 
	wan_udp.wan_udphdr_return_code = 0xaa;
	wan_udp.wan_udphdr_data_len = 0;
	DO_COMMAND(wan_udp);

	if (wan_udp.wan_udphdr_return_code == 0) {
		BANNER("USB CODE VERSION");
		printf("Code version: HDLC rev.%X\n",
				wan_udp.wan_udphdr_data[0]);
	}else{
		printf("Error: Rc=%x\n",wan_udp.wan_udphdr_return_code);
	}
}; /* read code version */


static void operational_stats (void)
{
	wp_usb_op_stats_t *stats;
	wan_udp.wan_udphdr_command= READ_OPERATIONAL_STATS; 
	wan_udp.wan_udphdr_return_code = 0xaa;
	wan_udp.wan_udphdr_data_len = 0;
	DO_COMMAND(wan_udp);

	if (wan_udp.wan_udphdr_return_code == 0) {
		BANNER("USB OPERATIONAL STATISTICS");
		stats = (wp_usb_op_stats_t *)&wan_udp.wan_udphdr_data[0];
		printf("              Number of Rx/Tx interrupts: %d\n", stats->isr_no);

 
	} 
}; /* Operational_stats */


static void flush_operational_stats( void ) 
{
#if 1
	wan_udp.wan_udphdr_command= FLUSH_OPERATIONAL_STATS;
	wan_udp.wan_udphdr_return_code = 0xaa;
	wan_udp.wan_udphdr_data_len = 0;
	DO_COMMAND(wan_udp);
#endif
}; /* flush_operational_stats */


static int usb_remora_tones(int mod_no)
{
	int	cnt = 0;
	char	ch;
	/* Disable trace to ensure that the buffers are flushed */
	wan_udp.wan_udphdr_command	= WAN_FE_TONES;
	wan_udp.wan_udphdr_return_code	= 0xaa;
	wan_udp.wan_udphdr_data_len	= 2;
	wan_udp.wan_udphdr_data[0]	= mod_no;
	wan_udp.wan_udphdr_data[1]	= 1;
	DO_COMMAND(wan_udp);

	if (wan_udp.wan_udphdr_return_code) { 
		printf("Failed to start tone on Module %d!\n", mod_no); 
		fflush(stdout);
		return 0;
	}

	printf("Press enter to stop the tone ...");fflush(stdout);ch=getchar();
tone_stop_again:
	/* Disable A200/A400 Ring event */
	wan_udp.wan_udphdr_command	= WAN_FE_TONES;
	wan_udp.wan_udphdr_return_code	= 0xaa;
	wan_udp.wan_udphdr_data_len	= 2;
	wan_udp.wan_udphdr_data[0]	= mod_no;
	wan_udp.wan_udphdr_data[1]	= 0;
	DO_COMMAND(wan_udp);

	if (wan_udp.wan_udphdr_return_code) {
		if (cnt++ > 10){
			sleep(1);
			goto tone_stop_again;
		} 
		printf("Failed to stop tone on Module %d (timeout)!\n",
					mod_no); 
		fflush(stdout);
		return 0;
	}
	return 0;
}

static int usb_remora_ring(int mod_no)
{
	int	cnt=0;
	char	ch;

	/* Enable A200/A400 Ring event */
	wan_udp.wan_udphdr_command	= WAN_FE_RING;
	wan_udp.wan_udphdr_return_code	= 0xaa;
	wan_udp.wan_udphdr_data_len	= 2;
	wan_udp.wan_udphdr_data[0]	= mod_no;
	wan_udp.wan_udphdr_data[1]	= 1;
	DO_COMMAND(wan_udp);

	if (wan_udp.wan_udphdr_return_code) { 
		printf("Failed to start ring for Module %d!\n", mod_no); 
		fflush(stdout);
		return 0;
	}
	fflush(stdout);

	printf("Press enter to stop the ring ...");fflush(stdout);ch=getchar();
ring_stop_again:
	/* Disable A200/A400 Ring event */
	wan_udp.wan_udphdr_command	= WAN_FE_RING;
	wan_udp.wan_udphdr_return_code	= 0xaa;
	wan_udp.wan_udphdr_data_len	= 2;
	wan_udp.wan_udphdr_data[0]	= mod_no;
	wan_udp.wan_udphdr_data[1]	= 0;
	DO_COMMAND(wan_udp);

	if (wan_udp.wan_udphdr_return_code) { 
		if (cnt++ > 10){
			sleep(1);
			goto ring_stop_again;
		} 
		printf("Failed to stop ring for Module %d (timeout)!\n", mod_no); 
		fflush(stdout);
		return 0;
	}
	fflush(stdout);

	return 0;
}

static int usb_remora_regdump(int mod_no)
{
	wan_remora_udp_t	*rm_udp;
	int			reg;

	rm_udp = (wan_remora_udp_t *)&wan_udp.wan_udphdr_data[0];
	rm_udp->mod_no = mod_no;
	wan_udp.wan_udphdr_command	= WAN_FE_REGDUMP;
	wan_udp.wan_udphdr_return_code	= 0xaa;
	wan_udp.wan_udphdr_data_len	= sizeof(wan_remora_udp_t);
	DO_COMMAND(wan_udp);

	if (wan_udp.wan_udphdr_return_code || !wan_udp.wan_udphdr_data_len) { 
		printf("Failed to get register dump!\n"); 
		fflush(stdout);
		return 0;
	}
	rm_udp = (wan_remora_udp_t *)&wan_udp.wan_udphdr_data[0];
	printf("\t------- Direct registers (%s,port %d) -------\n",
					WP_REMORA_DECODE_TYPE(rm_udp->type),
					rm_udp->mod_no);
	if (rm_udp->type == MOD_TYPE_FXS){

		for(reg = 0; reg < WAN_FXS_NUM_REGS; reg++){
			if (reg % 8 == 0) printf("\n\t");
			printf("%3d. %02X ", reg, rm_udp->u.regs_fxs.direct[reg]);
		}
		printf("\n\t-----------------------------\n");
		printf("\n");
		printf("\t------- Indirect registers (port %d) -------\n",
							rm_udp->mod_no);
		for (reg=0; reg < WAN_FXS_NUM_INDIRECT_REGS; reg++){
			if (reg % 6 == 0) printf("\n\t");
			printf("%3d. %04X ", reg, rm_udp->u.regs_fxs.indirect[reg]);
		}
		printf("\n\t-----------------------------\n");
		printf("\n");
		printf("TIP\t: -%d Volts\n", (rm_udp->u.regs_fxs.direct[80]*376)/1000);
		printf("RING\t: -%d Volts\n", (rm_udp->u.regs_fxs.direct[81]*376)/1000);
		printf("VBAT\t: -%d Volts\n", (rm_udp->u.regs_fxs.direct[82]*376)/1000);
	} else if (rm_udp->type == MOD_TYPE_FXO){

		for(reg = 0; reg < WAN_FXO_NUM_REGS; reg++){
			if (reg % 8 == 0) printf("\n\t");
			printf("%3d. %02X ", reg, rm_udp->u.regs_fxo.direct[reg]);
		}
		printf("\n\t-----------------------------\n");
		printf("\n");
	}

	fflush(stdout);
	return 0;
}

static int usb_remora_stats(int mod_no)
{
	wan_remora_udp_t	*rm_udp;

	rm_udp = (wan_remora_udp_t *)&wan_udp.wan_udphdr_data[0];
	rm_udp->mod_no = mod_no;
	wan_udp.wan_udphdr_command	= WAN_FE_STATS;
	wan_udp.wan_udphdr_return_code	= 0xaa;
	wan_udp.wan_udphdr_data_len	= sizeof(wan_remora_udp_t);
	DO_COMMAND(wan_udp);

	if (wan_udp.wan_udphdr_return_code || !wan_udp.wan_udphdr_data_len) { 
		printf("Failed to get voltage stats!\n"); 
		fflush(stdout);
		return 0;
	}
	rm_udp = (wan_remora_udp_t *)&wan_udp.wan_udphdr_data[0];
	if (rm_udp->type == MOD_TYPE_FXS){
		printf("\t------- Voltage Status  (%s,port %d) -------\n\n",
						WP_REMORA_DECODE_TYPE(rm_udp->type),
						rm_udp->mod_no);
		printf("TIP\t: -%7.4f Volts\n", (float)(rm_udp->u.stats.tip_volt*376)/1000);
		printf("RING\t: -%7.4f Volts\n", (float)(rm_udp->u.stats.ring_volt*376)/1000);
		printf("VBAT\t: -%7.4f Volts\n", (float)(rm_udp->u.stats.bat_volt*376)/1000);

	}else if (rm_udp->type == MOD_TYPE_FXO){
		unsigned char	volt = rm_udp->u.stats.volt;
		if (volt & 0x80){
			volt = ~volt + 1;
		}
		printf("\t------- Voltage Status  (%s,port %d) -------\n\n",
						WP_REMORA_DECODE_TYPE(rm_udp->type),
						rm_udp->mod_no);
		printf("VOLTAGE\t: %d Volts\n", volt);
		printf("\n");
		printf("\t------- Line Status  (%s,port %d) -------\n\n",
					WP_REMORA_DECODE_TYPE(rm_udp->type),
					rm_udp->mod_no);
		printf("Line\t: %s\n", FE_STATUS_DECODE(rm_udp->u.stats.status));
		printf("\n");
	}
	fflush(stdout);
	return 0;
}


static int usb_remora_hook(int mod_no, int offhook)
{
	sdla_fe_debug_t	fe_debug;

	fe_debug.type			= WAN_FE_DEBUG_HOOK;
	fe_debug.mod_no			= mod_no;
	fe_debug.fe_debug_hook.offhook	= offhook;
	aft_remora_debug_mode(&fe_debug);
	return 0;
}

//CORBA
int USBUsage(void)
{
	printf("wanpipemon: Wanpipe USB Hardware Level Debugging Utility\n\n");
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
	printf("\tCard Statistics\n");
	printf("\t   s         c       Communication Error Statistics\n");
	printf("\t             o       Operational Statistics\n");
	printf("\tFlush Statistics\n");
	printf("\t   f         c       Flush Communication Error Statistics\n");
	printf("\t             o       Flush Operational Statistics\n");
	printf("\t             pm      Flush T1/E1 performance monitoring counters\n");
	printf("\tAFT Remora Statistics\n");
	printf("\t   a         tone    Play a tones ( -m <mod_no> - Module number)\n");
	printf("\t   a         ring    Rings phone ( -m <mod_no> - Module number)\n");
	printf("\t   a         regdump Dumps FXS/FXO registers ( -m <mod_no> - Module number)\n");
	printf("\t   a         stats   Voltage status ( -m <mod_no> - Module number)\n");
	printf("\tExamples:\n");
	printf("\t--------\n\n");
	printf("\tex: wanpipemon -i w1g1 -c sc :Communication Error Statistics \n");
	printf("\n");
	return 0;

}

static void usb_router_up_time( void )
{
     	u_int32_t time;
     
     	wan_udp.wan_udphdr_command= ROUTER_UP_TIME;
	wan_udp.wan_udphdr_return_code = 0xaa;
     	wan_udp.wan_udphdr_data_len = 0;
     	wan_udp.wan_udphdr_data[0] = 0;
     	DO_COMMAND(wan_udp);
    
     	time = *(u_int32_t*)&wan_udp.wan_udphdr_data[0];
	
	BANNER("ROUTER UP TIME");

        print_router_up_time(time);
}


int USBMain(char *command,int argc, char* argv[])
{
	char		*opt=&command[1];
	int		mod_no = 0, i, err;
	sdla_fe_debug_t	fe_debug;
			
	switch(command[0]){

		case 'x':
			if (!strcmp(opt,"ru")){
				usb_router_up_time();
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
				printf("command: %s\n", command);
			}
			break;
			
		case 'f':
			if (!strcmp(opt, "o")){
				flush_operational_stats();
				printf("Operational statistics flushed\n");
				/*operational_stats();*/
			}else if (!strcmp(opt, "c")){
				flush_comm_err_stats();
				printf("Communication statistics flushed\n");
			/*	comm_err_stats();*/
			} else{
				printf("ERROR: Invalid Flush Command 'f', Type wanpipemon <cr> for help\n\n");
			}
			break;

		case 'a':
			err = 0;
			for(i=0;i<argc;i++){
				if (!strcasecmp(argv[i], "-m") && argv[i+1]){
					err = sscanf(argv[i+1], "%d", &mod_no);
					if (err){
						if (mod_no < 1 || mod_no > 16){
							printf("ERROR: Invalid Module number!\n\n");
						   	fflush(stdout);
					   		return 0;
						}
						mod_no--;
					}else{
						printf("ERROR: Invalid Module number!\n\n");
					   	fflush(stdout);
				   		return 0;
					}
					break;
				}
			}

			if (strcmp(opt,"tone") == 0){	
				usb_remora_tones(mod_no);
			}else if (strcmp(opt,"ring") == 0){	
				usb_remora_ring(mod_no);
			}else if (strcmp(opt,"regdump") == 0){	
				usb_remora_regdump(mod_no);
			}else if (!strcmp(opt,"reg")){
				long	value;
				fe_debug.type = WAN_FE_DEBUG_REG;
				if (argc < 6){
					printf("ERROR: Invalid command argument!\n");
					break;				
				}
				value = strtol(argv[5],(char**)NULL, 10);
				if (value == LONG_MIN || value == LONG_MAX){
					printf("ERROR: Invalid argument 5: %s!\n",
								argv[5]);
					break;				
				}
				fe_debug.fe_debug_reg.reg  = value;
				fe_debug.fe_debug_reg.read = 1;
				if (strcmp(argv[6], "-m") && argc > 6){
					value = strtol(argv[6],(char**)NULL, 16);
					if (value == LONG_MIN || value == LONG_MAX){
						printf("ERROR: Invalid argument 6: %s!\n",
									argv[6]);
						break;
					}
					fe_debug.fe_debug_reg.read = 0;
					fe_debug.fe_debug_reg.value = value;
				}
				fe_debug.mod_no = mod_no;
				aft_remora_debug_mode(&fe_debug);
			}else if (strcmp(opt,"stats") == 0){	
				usb_remora_stats(mod_no);
			}else if (strcmp(opt,"offhook") == 0){	
				usb_remora_hook(mod_no, 1);
			}else if (strcmp(opt,"onhook") == 0){	
				usb_remora_hook(mod_no, 0);
			}else{
				printf("ERROR: Invalid Status Command 'a', Type wanpipemon <cr> for help\n\n");
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
