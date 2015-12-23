/*****************************************************************************
* wanpipemon.c	PPP Monitor.
*
* Author:	Nenad Corbic <ncorbic@sangoma.com>		
*
* Copyright:	(c) 1995-1999 Sangoma Technologies Inc.
*
*		This program is free software; you can redistribute it and/or
*		modify it under the terms of the GNU General Public License
*		as published by the Free Software Foundation; either version
*		2 of the License, or (at your option) any later version.
* ----------------------------------------------------------------------------
* Oct 18, 1999  Nenad Corbic    Added new features for 2.1.0 release version
* Aug 13, 1998	Jaspreet Singh	Improved line tracing code
* Dec 19, 1997	Jaspreet Singh	Added 'irq timeouts' stat in 
*				'READ_COMMS_ERR_STATISTICS'.
* Nov 24, 1997  Jaspreet Singh  Added new stats for driver statistics
* Oct 20, 1997	Jaspreet Singh	Added new wan_udphdr_commands for Driver statistics and
*				Router Up time.
* Jul 28, 1997	Jaspreet Singh	Added a new wan_udphdr_command for running line trace 
*				displaying RAW data. 
* Jun 24, 1997  Jaspreet Singh	S508/FT1 test wan_udphdr_commands
* Apr 25, 1997	Farhan Thawar	Initial version based on wanpipemon for WinNT.
*****************************************************************************/

/******************************************************************************
 * 			INCLUDE FILES					      *
 *****************************************************************************/
#include <stdio.h>
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
#if defined(__LINUX__)
# include <linux/wanpipe_defines.h>
# include <linux/wanpipe_cfg.h>
# include <linux/wanpipe.h>
# include <linux/sdla_ppp.h>
#else
# include <netinet/in_systm.h>
# include <netinet/in.h>
# include <netinet/ip.h>
# include <netinet/udp.h>      
# include <wanpipe_defines.h>
# include <wanpipe_cfg.h>
# include <wanpipe.h>
# include <sdla_ppp.h>
#endif
#include "fe_lib.h" 
#include "wanpipemon.h"

/******************************************************************************
 * 			DEFINES/MACROS					      *
 *****************************************************************************/
#define TIMEOUT 1
#define MDATALEN MAX_LGTH_UDP_MGNT_PKT
#define MAX_TRACE_BUF ((MDATALEN+200)*2)

#define BANNER(str) banner(str,0)


/******************************************************************************
 * 			GLOBAL VARIABLES				      *
 *****************************************************************************/
/* storage for FT1 LEDs */
FT1_LED_STATUS FT1_LED;

/* for S508/FT1 test wan_udphdr_commands */
int fail;

/* defines for now */
extern int sock;
extern int raw_data;
extern int udp_port;
extern int trace_all_data;
extern char i_name[];
extern int is_508;
/******************************************************************************
 * 			FUNCTION PROTOTYPES				      *
 *****************************************************************************/
static unsigned long decode_bps( unsigned char bps );
static void set_FT1_monitor_status( unsigned char);

extern void read_te1_56k_stat(void);
extern void flush_te1_pmon(void);

/******************************************************************************
 * 			FUNCTION DEFINITION				      *
 *****************************************************************************/
void PPP_set_FT1_mode( void )
{
	for(;;){ 
		wan_udp.wan_udphdr_command = SET_FT1_MODE;
		wan_udp.wan_udphdr_return_code = 0xaa;
		wan_udp.wan_udphdr_data_len = 0;
		DO_COMMAND(wan_udp);
		if(wan_udp.wan_udphdr_return_code == 0){
			break;
		}
	}
} /* set_FT1_mode */


int PPPConfig( void ) 
{
	unsigned char x=0;
	char codeversion[20];
   
	protocol_cb_size=sizeof(wan_mgmt_t) + sizeof(wan_cmd_t) + 1;
	wan_udp.wan_udphdr_command = PPP_READ_CONFIG;
	wan_udp.wan_udphdr_return_code = 0xaa;
	wan_udp.wan_udphdr_data_len = 0;

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

	if( wan_udp.wan_udphdr_data_len == 0x73 ) {
		is_508 = WAN_TRUE;
	} else {
		is_508 = WAN_FALSE;
	} 
   
	strlcpy(codeversion, "?.??", 10);
   
	wan_udp.wan_udphdr_command = PPP_READ_CODE_VERSION;
	wan_udp.wan_udphdr_return_code = 0xaa;
	wan_udp.wan_udphdr_data_len = 0;
	DO_COMMAND(wan_udp);

	if (wan_udp.wan_udphdr_return_code == 0) {
		wan_udp.wan_udphdr_data[wan_udp.wan_udphdr_data_len] = 0;
		strlcpy(codeversion, (char*)wan_udp.wan_udphdr_data, 10);
	}

	return(WAN_TRUE);
}; 

void PPP_read_FT1_status( void )
{
	wan_udp.wan_udphdr_command = PPIPE_FT1_READ_STATUS;
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
	}
} /* read_FT1_status */



static unsigned long decode_bps( unsigned char bps ) 
{
	unsigned long number;
 
	switch( bps ) {
		case 0x00:
			number = 0;
			break;
		case 0x01:
			number = 1200;
			break;
		case 0x02:
			number = 2400;
			break;
		case 0x03:
			number = 4800;
			break;
		case 0x04:
			number = 9600;
			break;
		case 0x05:
			number = 19200;
			break;
		case 0x06:
			number = 38400;
			break;
		case 0x07:
			number = 45000;
			break;
		case 0x08:
			number = 56000;
			break;
		case 0x09:
			number = 64000;
			break;
		case 0x0A:
			number = 74000;
			break;
		case 0x0B:
			number = 112000;
			break;
		case 0x0C:
			number = 128000;
			break;
		case 0x0D:
			number = 156000;
			break;
		default:
			number = 0;
			break;
	} /* switch */

	return number;
}; /* decode_bps */

static void error( void ) 
{

	printf("Error: Command failed!\n");

}; /* error */

static void general_conf( void ) 
{
	unsigned long baud;
	unsigned short s_number, mtu, mru;
	unsigned char misc;
 
	wan_udp.wan_udphdr_command = PPP_READ_CONFIG;
	wan_udp.wan_udphdr_return_code = 0xaa;
	wan_udp.wan_udphdr_data_len = 0;
	DO_COMMAND(wan_udp);

	if( wan_udp.wan_udphdr_return_code == 0 ) {

		if( is_508 == WAN_TRUE ) {
			BANNER("GENERAL CONFIGURATION 508 Board");
			memcpy(&baud,&wan_udp.wan_udphdr_data[0],4);
			memcpy(&s_number,&wan_udp.wan_udphdr_data[4],2);
			printf("Transmit buffer allocation in percent: %u\n",
				s_number);
			misc = wan_udp.wan_udphdr_data[6];
			printf("Use alternate adapter frequency: ");
			( misc & 0x10 ) ? printf("Yes\n") : printf("No\n");
			printf("Set interface type to RS-232: ");
			( misc & 0x20 ) ? printf("Yes\n") : printf("No\n");
			memcpy(&mtu, &wan_udp.wan_udphdr_data[8],2);
			memcpy(&mru, &wan_udp.wan_udphdr_data[10],2);
		} else {
			BANNER("GENERAL CONFIGURATION 502 Board");
			baud = decode_bps(wan_udp.wan_udphdr_data[0]);
			misc = wan_udp.wan_udphdr_data[3];
			printf("Discard transmit-aborted frames: ");
			( misc & 0x01 ) ? printf("Yes\n") : printf("No\n");
			memcpy(&mtu, &wan_udp.wan_udphdr_data[5],2);
			memcpy(&mru, &wan_udp.wan_udphdr_data[7],2);
		} 

		printf("Baud rate in bps: %lu\n",baud);
		printf("Update transmit statistics( user data ): ");
		( misc & 0x02 ) ? printf("Yes\n") : printf("No\n");
		printf("Update receive statistics( user data ): ");
		( misc & 0x04 ) ? printf("Yes\n") : printf("No\n");
		printf("Timestamp received packets: ");
		( misc & 0x08 ) ? printf("Yes\n") : printf("No\n");
		printf("Maximum Receive/Transmit Unit(MRU/MTU): %u\n",mtu);
		printf("Minimum remote MRU required on connection: %u\n",mtu);
	} else {
		error();
	} 
}; /* general_conf */

static void timers( void ) 
{
	int i;
	unsigned short tmp;
   
	wan_udp.wan_udphdr_command = PPP_READ_CONFIG;
	wan_udp.wan_udphdr_return_code = 0xaa;
	wan_udp.wan_udphdr_data_len = 0;
	DO_COMMAND(wan_udp);

	if( wan_udp.wan_udphdr_return_code == 0 ) {

		BANNER("PPP TIMERS");

		if( is_508 == WAN_TRUE ) {
			i = 12;
		} else {
			i = 9;
		} 

		memcpy(&tmp,&wan_udp.wan_udphdr_data[i],2);
		printf("Restart timer: %u\n",tmp);
		memcpy(&tmp,&wan_udp.wan_udphdr_data[i+2],2);
		printf("Authentication restart timer: %u\n",tmp);
		memcpy(&tmp,&wan_udp.wan_udphdr_data[i+4],2);
		printf("Authentication wait timer: %u\n",tmp);
		memcpy(&tmp,&wan_udp.wan_udphdr_data[i+6],2);
		printf("DCD/CTS failure detection timer: %u\n",tmp);
		memcpy(&tmp,&wan_udp.wan_udphdr_data[i+8],2);
		printf("Drop DTR duration timer: %u\n",tmp);
		memcpy(&tmp,&wan_udp.wan_udphdr_data[i+10],2);
		printf("Connection attempt timeout: %u\n",tmp);
		memcpy(&tmp,&wan_udp.wan_udphdr_data[i+12],2);
		printf("Max-Configure counter: %u\n",tmp);
		memcpy(&tmp,&wan_udp.wan_udphdr_data[i+14],2);
		printf("Max-Terminate counter: %u\n",tmp);
		memcpy(&tmp,&wan_udp.wan_udphdr_data[i+16],2);
		printf("Max-Failure counter: %u\n",tmp);
		memcpy(&tmp,&wan_udp.wan_udphdr_data[i+18],2);
		printf("Max-Authenticate counter: %u\n",tmp);

	} else {
		error();
	} 
}; /* timers */

static void authent( void ) 
{
	unsigned char tmp;
   
	wan_udp.wan_udphdr_command = PPP_READ_CONFIG;
	wan_udp.wan_udphdr_return_code = 0xaa;
	wan_udp.wan_udphdr_data_len = 0;
	DO_COMMAND(wan_udp);

	if( wan_udp.wan_udphdr_return_code == 0 ) {
	
		BANNER("PPP AUTHENTICATION");

		if( is_508 == WAN_TRUE ) {
			tmp = wan_udp.wan_udphdr_data[32];
		} else {
			tmp = wan_udp.wan_udphdr_data[29];
		} 
	
		if( tmp & 0x01 ) {
		     printf("Allow the use of PAP for inbound/outbound: Yes\n");
			if( tmp & 0x80 ) {
				printf("Using inbound authentication.\n");
			} else {
				printf("Using outbound authentication.\n");
			} //if
		} else {
			printf("Allow the use of PAP for inbound/outbound: No\n"				);
		} //if

		if( tmp & 0x02 ) {
	 	    printf("Allow the use of CHAP for inbound/outbound: Yes\n");
			if( tmp & 0x80 ) {
				printf("Using inbound authentication.\n");
			} else {
				printf("Using outbound authentication.\n");
			} //if
		} else {
		     printf("Allow the use of CHAP for inbound/outbound: No\n");
		} //if

	} else {
		error();
	} //if
}; /* authent */

static void ip_config( void ) 
{
	int i;
   
	wan_udp.wan_udphdr_command = PPP_READ_CONFIG;
	wan_udp.wan_udphdr_return_code = 0xaa;
	wan_udp.wan_udphdr_data_len = 0;

	DO_COMMAND(wan_udp);

	if( wan_udp.wan_udphdr_return_code == 0 ) {

		BANNER("PPP IP CONFIGURATION");

		if( is_508 == WAN_TRUE ) {
			i = 33;
		} else {
			i = 30;
		} //if
		printf("Enable the use of IP: ");
		( wan_udp.wan_udphdr_data[i] & 0x80 ) ? printf("Yes\n") : printf("No\n");
		printf("Notify remote of locally-configure address: ");
		( wan_udp.wan_udphdr_data[i] & 0x01 ) ? printf("Yes\n") : printf("No\n");
		printf("Local IP address( 0.0.0.0 = request ): %u.%u.%u.%u\n",
			wan_udp.wan_udphdr_data[i+1],wan_udp.wan_udphdr_data[i+2],wan_udp.wan_udphdr_data[i+3],wan_udp.wan_udphdr_data[i+4]);
		printf("Request remote to provide local address: ");
		( wan_udp.wan_udphdr_data[i] & 0x02 ) ? printf("Yes\n") : printf("No\n");
		printf("Provide remote with pre-configured address: ");
		( wan_udp.wan_udphdr_data[i] & 0x04 ) ? printf("Yes\n") : printf("No\n");
		printf("Remote IP address: %u.%u.%u.%u\n",wan_udp.wan_udphdr_data[i+5],
			wan_udp.wan_udphdr_data[i+6],wan_udp.wan_udphdr_data[i+7],wan_udp.wan_udphdr_data[i+8]);
		printf("Require that remote provide an address: ");
		( wan_udp.wan_udphdr_data[i] & 0x08 ) ? printf("Yes\n") : printf("No\n");
	} else {
		error();
	} 
}; /* ip_config */

static void ipx_config( void )
{
	int i;
   
	wan_udp.wan_udphdr_command = PPP_READ_CONFIG;
	wan_udp.wan_udphdr_return_code = 0xaa;
	wan_udp.wan_udphdr_data_len = 0;

	DO_COMMAND(wan_udp);

	if( wan_udp.wan_udphdr_return_code == 0 ) {
	
		BANNER("PPP IPX CONFIGURATION");

		if( is_508 == WAN_TRUE ) {
			i = 42;
		} else {
			i = 39;
		} //if

		printf("Enable the use of IPX: ");
		( wan_udp.wan_udphdr_data[i] & 0x80 ) ? printf("Yes\n") : printf("No\n");
		printf("Include network number in Config-Request: ");
		( wan_udp.wan_udphdr_data[i] & 0x01 ) ? printf("Yes\n") : printf("No\n");
		printf("Network number( 00000000 = request ): %02X %02X %02X %02X\n",wan_udp.wan_udphdr_data[i+1],wan_udp.wan_udphdr_data[i+2],wan_udp.wan_udphdr_data[i+3],wan_udp.wan_udphdr_data[i+4]);
		printf("Include local node # in Config-Request: ");
		( wan_udp.wan_udphdr_data[i] & 0x02 ) ? printf("Yes\n") : printf("No\n");
		printf("Local node number( 000000000000 = request ): %02X %02X %02X %02X %02X %02X\n",wan_udp.wan_udphdr_data[i+5],wan_udp.wan_udphdr_data[i+6],wan_udp.wan_udphdr_data[i+7],wan_udp.wan_udphdr_data[i+8],wan_udp.wan_udphdr_data[i+9],wan_udp.wan_udphdr_data[i+10]);
		printf("Force remote to accept remote node number: ");
		( wan_udp.wan_udphdr_data[i] & 0x04 ) ? printf("Yes\n") : printf("No\n");
		printf("Remote node number: %02X %02X %02X %02X %02X %02X\n",
			wan_udp.wan_udphdr_data[i+11],wan_udp.wan_udphdr_data[i+12],wan_udp.wan_udphdr_data[i+13],wan_udp.wan_udphdr_data[i+14],
			wan_udp.wan_udphdr_data[i+15],wan_udp.wan_udphdr_data[i+16]);
		printf("Include config-complete in Config-Request: ");
		( wan_udp.wan_udphdr_data[i] & 0x40 ) ? printf("Yes\n") : printf("No\n");

		if( wan_udp.wan_udphdr_data[i] & 0x20 ) {
			printf("Routing protocol: Request default( normally RIP/SAP )\n");
		} else if( wan_udp.wan_udphdr_data[i] & 0x18 ){
			printf("Routing protocol: Use either RIP/SAP or NLSP\n");
		} else if( wan_udp.wan_udphdr_data[i] & 0x08 ){
			printf("Routing protocol: Use RIP/SAP only\n");
		} else if( wan_udp.wan_udphdr_data[i] & 0x10 ){
			printf("Routing protocol: Use NLSP only\n");
		} else {
			printf("Routing protocol: No routing protocol\n");
		} //if
		printf("Local router name( max. 47 characters ): %s\n",
			&wan_udp.wan_udphdr_data[i+17]);
	} else {
		error();
	} //if
}; /* ipx_config */

static void set_state( unsigned char tmp ) 
{
	switch( tmp ) {
		case 0:
			printf("Initial\n");
			break;
		case 1:
			printf("Starting\n");
			break;
		case 2:
			printf("Closed\n");
			break;
		case 3:
			printf("Stopped\n");
			break;
		case 4:
			printf("Closing\n");
			break;
		case 5:
			printf("Stopping\n");
			break;
		case 6:
			printf("Request Sent\n");
			break;
		case 7:
			printf("Ack Received\n");
			break;
		case 8:
			printf("Ack Sent\n");
			break;
		case 9:
			printf("Opened\n");
			break;
		default:
			printf("Unknown\n");
			break;
	} //switch

}; /* set_state */

static void state( void ) 
{
	wan_udp.wan_udphdr_command = PPIPE_GET_IBA_DATA;
	wan_udp.wan_udphdr_return_code = 0xaa;
	wan_udp.wan_udphdr_data_len = 0;
	DO_COMMAND(wan_udp);

	if( wan_udp.wan_udphdr_return_code == 0 ) {

		BANNER("PPP LINK STATE");

		switch( wan_udp.wan_udphdr_data[2] ) {
			case 0:
				printf("PPP Phase: Link Dead\n");
				break;
			case 1:
				printf("PPP Phase: Establishment( LCP )\n");
				break;
			case 2:
				printf("PPP Phase: Authentication\n");
				break;
			case 3:
				printf("PPP Phase: Network Layer\n");
				break;
			case 4:
				printf("PPP Phase: Link Termination\n");
				break;
			default:
				printf("PPP Phase: Unknown\n");
				break;
		} 

		printf("LCP State: ");
		set_state( wan_udp.wan_udphdr_data[1] );
		printf("IPCP State: ");
		set_state( wan_udp.wan_udphdr_data[3] );
		printf("IPXCP State: ");
		set_state( wan_udp.wan_udphdr_data[4] );
		switch( wan_udp.wan_udphdr_data[5] ) {
			case 0:
				printf("PAP State: Initial( Inactive )\n");
				break;
			case 1:
				printf("PAP State: Failed\n");
				break;
			case 2:
				printf("PAP State: Request Sent\n");
				break;
			case 3:
				printf("PAP State: Waiting\n");
				break;
			case 4:
				printf("PAP State: Opened( Success )\n");
				break;
			default:
				printf("PAP State: Unknown\n");
				break;
		} 

		switch( wan_udp.wan_udphdr_data[5] ) {
			case 0:
				printf("CHAP State: Initial( Inactive )\n");
				break;
			case 1:
				printf("CHAP State: Failed\n");
				break;
			case 2:
				printf("CHAP State: Challenge Sent\n");
				break;
			case 3:
				printf("CHAP State: Waiting for Challenge\n");
				break;
			case 4:
				printf("CHAP State: Response Sent\n");
				break;
			case 5:
				printf("CHAP State: Opened( Success )\n");
				break;
			default:
				printf("CHAP State: Unknown\n");
				break;
		} 
	} else {
		error();
	} 
}; /* state */

static void negot( void ) {
	unsigned short mru;
   
	wan_udp.wan_udphdr_command = PPP_GET_CONNECTION_INFO;
	wan_udp.wan_udphdr_return_code = 0xaa;
	wan_udp.wan_udphdr_data_len = 0;
	DO_COMMAND(wan_udp);

	if( wan_udp.wan_udphdr_return_code == 0 ) {

		BANNER("PPP NEGOTIATIONS");

		memcpy(&mru,&wan_udp.wan_udphdr_data[0],2);
		printf("Remote Maximum Receive Unit: %u\n",mru);
		printf("Negotiated IP options: %02X",wan_udp.wan_udphdr_data[2]);
		( wan_udp.wan_udphdr_data[2] & 0x80 ) ? printf("( IP Enabled )\n") : 
					printf("( IP Disabled )\n");
		printf("Local IP address: %u.%u.%u.%u\n",wan_udp.wan_udphdr_data[3],wan_udp.wan_udphdr_data[4],
			wan_udp.wan_udphdr_data[5],wan_udp.wan_udphdr_data[6]);
		printf("Remote IP address: %u.%u.%u.%u\n",wan_udp.wan_udphdr_data[7],wan_udp.wan_udphdr_data[8],
			wan_udp.wan_udphdr_data[9],wan_udp.wan_udphdr_data[10]);
		printf("Negotiated IPX options: %02X",wan_udp.wan_udphdr_data[11]);
		( wan_udp.wan_udphdr_data[11] & 0x80 ) ? printf("( IPX Enabled )\n") : 
					printf("( IPX Disabled )\n");
		printf("IPX network number: %02X %02X %02X %02X\n",wan_udp.wan_udphdr_data[12],
			wan_udp.wan_udphdr_data[13],wan_udp.wan_udphdr_data[14],wan_udp.wan_udphdr_data[15]);
		printf("Local IPX node number: %02X %02X %02X %02X %02X %02X\n",
			wan_udp.wan_udphdr_data[16],wan_udp.wan_udphdr_data[17],wan_udp.wan_udphdr_data[18],wan_udp.wan_udphdr_data[19],
			wan_udp.wan_udphdr_data[20],wan_udp.wan_udphdr_data[21]);
		printf("Remote IPX node number: %02X %02X %02X %02X %02X %02X\n"
			,wan_udp.wan_udphdr_data[22],wan_udp.wan_udphdr_data[23],wan_udp.wan_udphdr_data[24],wan_udp.wan_udphdr_data[25],
			wan_udp.wan_udphdr_data[26],wan_udp.wan_udphdr_data[27]);
		printf("Remote IPX router name: %s\n",&wan_udp.wan_udphdr_data[28]);
		
		switch( wan_udp.wan_udphdr_data[76] ) {
			case 0:
				printf("Authentication status: No inbound authentication negotiated\n");
				break;
			case 1:
				printf("Authentication status: PeerID valid, Password Incorrect\n");
				break;
			case 2:
				printf("Authentication status: PeerID was invalid\n");
				break;
			case 3:
				printf("Authentication status: PeerID and Password were correct\n");
				break;
			default:
				printf("Authentication status: Unknown\n");
				break;
		} 

		printf("Inbound PeerID: %s\n",&wan_udp.wan_udphdr_data[77]);
		
	} else {
		error();
	} //if
}; /* negot */

static void cause( void ) 
{
	unsigned short disc;
  
	wan_udp.wan_udphdr_command = PPIPE_GET_IBA_DATA;
	wan_udp.wan_udphdr_return_code = 0xaa;
	wan_udp.wan_udphdr_data_len = 0;
	DO_COMMAND(wan_udp);

	if( wan_udp.wan_udphdr_return_code == 0 ) {

		BANNER("LAST CAUSE OF LINK FAILURE");

		memcpy(&disc,&wan_udp.wan_udphdr_data[7],2);
		printf("Local request by termination phase: ");
		(disc & 0x0100) ? printf("Yes\n") : printf("No\n");
		printf("DCD and/or CTS dropped: ");
		(disc & 0x0200) ? printf("Yes\n") : printf("No\n");
		printf("Disabled communications locally: ");
		(disc & 0x0400) ? printf("Yes\n") : printf("No\n");
		printf("Inbound/Outbound authentication failed: ");
		(disc & 0x0800) ? printf("Yes\n") : printf("No\n");
		printf("Failed to negotiate inbound auth. protocol with peer:");
		(disc & 0x1000) ? printf(" Yes\n") : printf(" No\n");
		printf("Rejected peers request for authentication: ");
		(disc & 0x2000) ? printf("Yes\n") : printf("No\n");
		printf("Peer rejected MRU option of config-request: ");
		(disc & 0x4000) ? printf("Yes\n") : printf("No\n");
		printf("MRU of peer was below required minumum: ");
		(disc & 0x8000) ? printf("Yes\n") : printf("No\n");
		printf("Rejected peers LCP option(s) too many times: ");
		(disc & 0x0001) ? printf("Yes\n") : printf("No\n");
		printf("Rejected peers IPCP option(s) too many times: ");
		(disc & 0x0002) ? printf("Yes\n") : printf("No\n");
		printf("Rejected peers IPXCP option(s) too many times: ");
		(disc & 0x0004) ? printf("Yes\n") : printf("No\n");
	} else {
		error();
	} //if
}; /* cause */

static void modem( void )
{
	unsigned char cts_dcd;
   
	wan_udp.wan_udphdr_command = PPIPE_GET_IBA_DATA;
	wan_udp.wan_udphdr_return_code = 0xaa;
	wan_udp.wan_udphdr_data_len = 0;
	DO_COMMAND(wan_udp);

	if( wan_udp.wan_udphdr_return_code == 0 ) {

		BANNER("MODEM STATUS");

		memcpy(&cts_dcd,&wan_udp.wan_udphdr_data[0],1);
		printf("DCD: ");
		(cts_dcd & 0x08) ? printf("High\n") : printf("Low\n");
		printf("CTS: ");
		(cts_dcd & 0x20) ? printf("High\n") : printf("Low\n");
	} else {
		error();
	} //if
}; /* modem */

static void general_stats( void ) 
{
	unsigned short tmp;
	unsigned long l_tmp;
   
	wan_udp.wan_udphdr_command = PPP_READ_STATISTICS;
	wan_udp.wan_udphdr_return_code = 0xaa;
	wan_udp.wan_udphdr_data_len = 0;
	DO_COMMAND(wan_udp);

	if( wan_udp.wan_udphdr_return_code == 0 ) {

		BANNER ("GENERAL STATISTICS");

		if( is_508 == WAN_TRUE ) {
			memcpy(&tmp,&wan_udp.wan_udphdr_data[2],2);
			printf("Number of received frames discarded due to bad length: %u\n",tmp);
		} else {
			memcpy(&tmp,&wan_udp.wan_udphdr_data[0],2);
			printf("Number of received frames discarded at the interrupt level: %u\n",tmp);
			memcpy(&tmp,&wan_udp.wan_udphdr_data[2],2);
			printf("Number of received frames discarded at the application level: %u\n",tmp);
			memcpy(&tmp,&wan_udp.wan_udphdr_data[4],2);
			printf("Number of received retransmitted due to aborts: %u\n",tmp);
		} 

		memcpy(&l_tmp,&wan_udp.wan_udphdr_data[6],4);
		printf("Number of user frames transmitted: %lu\n",l_tmp);
		memcpy(&l_tmp,&wan_udp.wan_udphdr_data[10],4);
		printf("Number of user bytes transmitted: %lu\n",l_tmp);
		memcpy(&l_tmp,&wan_udp.wan_udphdr_data[14],4);
		printf("Number of user frames received: %lu\n",l_tmp);
		memcpy(&l_tmp,&wan_udp.wan_udphdr_data[18],4);
		printf("Number of user bytes received: %lu\n",l_tmp);
	} else {
		error();
	} 
}; /* general_stats */

static void flush_general_stats( void ) 
{
	wan_udp.wan_udphdr_command = PPP_FLUSH_STATISTICS;
	wan_udp.wan_udphdr_return_code = 0xaa;
	wan_udp.wan_udphdr_data_len = 0;
	DO_COMMAND(wan_udp);

	if( wan_udp.wan_udphdr_return_code != 0 ) {
		error();
	} //if

}; /* flush_general_stats */

static void comm_err( void ) 
{
	wan_udp.wan_udphdr_command = PPP_READ_ERROR_STATS;
	wan_udp.wan_udphdr_return_code = 0xaa;
	wan_udp.wan_udphdr_data_len = 0;
	DO_COMMAND(wan_udp);

	if( wan_udp.wan_udphdr_return_code == 0 ) {

		BANNER("COMMUNICATION ERROR STATISTICS");

		if( is_508 == WAN_TRUE ) {
			printf("Number of times receiver was halted due to full buffers: %u\n",wan_udp.wan_udphdr_data[3]);
		} else {
			printf("Number of frames discarded at the interrupt level due to frame being too long: %u\n",wan_udp.wan_udphdr_data[3]);
			printf("Number of transmit underruns: %u\n",wan_udp.wan_udphdr_data[5]);
		} //if
		printf("Number of receiver overrun errors: %u\n",wan_udp.wan_udphdr_data[0]);
		printf("Number of receiver CRC errors: %u\n",wan_udp.wan_udphdr_data[1]);
		printf("Number of abort frames received: %u\n",wan_udp.wan_udphdr_data[2]);
		printf("Number of abort frames transmitted: %u\n",wan_udp.wan_udphdr_data[4]);
		printf("Number of missed transmit underrun interrupts: %u\n",wan_udp.wan_udphdr_data[6]);
		printf("Number of IRQ timeouts: %u\n",wan_udp.wan_udphdr_data[7]);
		printf("Number of times DCD changed state: %u\n",wan_udp.wan_udphdr_data[8]);
		printf("Number of times CTS changed state: %u\n",wan_udp.wan_udphdr_data[9]);
	} else {
		error();
	} //if
}; /* comm_err */

static void flush_comm_err( void ) 
{
	wan_udp.wan_udphdr_command = PPP_FLUSH_ERROR_STATS;
	wan_udp.wan_udphdr_return_code = 0xaa;
	wan_udp.wan_udphdr_data_len = 0;
	DO_COMMAND(wan_udp);

	if( wan_udp.wan_udphdr_return_code != 0 ) {
		error();
	} 
}; /* flush_general_stats */

static void packet( void ) 
{
   	ppp_pkt_stats_t *packet_stats;
	wan_udp.wan_udphdr_command = PPP_READ_PACKET_STATS;
	wan_udp.wan_udphdr_return_code = 0xaa;
	wan_udp.wan_udphdr_data_len = 0;
	DO_COMMAND(wan_udp);

	packet_stats = (ppp_pkt_stats_t *)&wan_udp.wan_udphdr_data[0];

	if( wan_udp.wan_udphdr_return_code == 0 ) {

		BANNER("PACKET STATISTICS");

		printf("Number discards( bad header ): %u\n",
				packet_stats->rx_bad_header);
		printf("Number discards( unknown/unsupported protocol ): %u\n",
				packet_stats->rx_prot_unknwn);
		printf("Number discards(unknown/unsupported protocol+too large for Protocol-Reject): %u\n",
				packet_stats->rx_too_large);
		printf("\n\t\t\tReceived\tTransmitted\n");
		printf("Number of LCP packets: %u\t\t%u\n",
				packet_stats->rx_lcp,packet_stats->tx_lcp);
		printf("Number of IPCP packets: %u\t\t%u\n",
				packet_stats->rx_ipcp,  packet_stats->tx_ipcp);
		printf("Number of IPXCP packets: %u\t\t%u\n",
				packet_stats->rx_ipxcp,packet_stats->tx_ipxcp);
		printf("Number of PAP packets: %u\t\t%u\n",
				packet_stats->rx_pap,packet_stats->tx_pap);
		printf("Number of CHAP packets: %u\t\t%u\n",
				packet_stats->rx_chap,packet_stats->tx_chap);
		printf("Number of LQR packets: %u\t\t%u\n",
				packet_stats->rx_lqr,packet_stats->tx_lqr);
		printf("Number of IP packets:  %u\t\t%u\n",
				packet_stats->rx_ip,packet_stats->tx_ip);
		printf("Number of IPX packets: %u\t\t%u\n",
				packet_stats->rx_ipx,packet_stats->tx_ipx);
	} else {
		error();
	} 
}; /* packet */

static void flush_packet( void ) 
{
	wan_udp.wan_udphdr_command = PPP_FLUSH_PACKET_STATS;
	wan_udp.wan_udphdr_return_code = 0xaa;
	wan_udp.wan_udphdr_data_len = 0;
	DO_COMMAND(wan_udp);

	if( wan_udp.wan_udphdr_return_code != 0 ) {
		error();
	} 
}; /* flush_packet */

static void lcp( void ) 
{
	unsigned short tmp, tmp2;
 
	wan_udp.wan_udphdr_command = PPP_READ_LCP_STATS;
	wan_udp.wan_udphdr_return_code = 0xaa;
	wan_udp.wan_udphdr_data_len = 0;
	DO_COMMAND(wan_udp);

	if( wan_udp.wan_udphdr_return_code == 0 ) {

		BANNER("LCP STATISTICS");

		memcpy(&tmp,&wan_udp.wan_udphdr_data[0],2);
		printf("Packets discarded (unknown LCP code): %u\n",tmp);
		memcpy(&tmp,&wan_udp.wan_udphdr_data[48],2);
		printf("Received LCP packets too large: %u\n",tmp);
		memcpy(&tmp,&wan_udp.wan_udphdr_data[50],2);
		printf("Received packets invalid or out-of-sequence Configure-Acks: %u\n",tmp);
		memcpy(&tmp,&wan_udp.wan_udphdr_data[52],2);
		printf("Received packets invalid Configure-Naks or Configure-Rejects: %u\n",tmp);
		memcpy(&tmp,&wan_udp.wan_udphdr_data[54],2);
		printf("Configure-Naks or Configure-Rejects with bad Identifier: %u\n",tmp);
		printf("\n\t\t\tReceived\tTransmitted\n");
		memcpy(&tmp,&wan_udp.wan_udphdr_data[2],2);
		memcpy(&tmp2,&wan_udp.wan_udphdr_data[26],2);
		printf("Number of Config-Request pkts: %u\t%u\n",tmp,tmp2);
		memcpy(&tmp,&wan_udp.wan_udphdr_data[4],2);
		memcpy(&tmp2,&wan_udp.wan_udphdr_data[28],2);
		printf("Number of Config-Ack packets : %u\t%u\n",tmp,tmp2);
		memcpy(&tmp,&wan_udp.wan_udphdr_data[6],2);
		memcpy(&tmp2,&wan_udp.wan_udphdr_data[30],2);
		printf("Number of Config-Nack packets: %u\t%u\n",tmp,tmp2);
		memcpy(&tmp,&wan_udp.wan_udphdr_data[8],2);
		memcpy(&tmp2,&wan_udp.wan_udphdr_data[32],2);
		printf("Number of Config-Reject packets: %u\t%u\n",tmp,tmp2);
		memcpy(&tmp,&wan_udp.wan_udphdr_data[10],2);
		memcpy(&tmp2,&wan_udp.wan_udphdr_data[34],2);
		printf("Number of Term-Reqst packets : %u\t%u\n",tmp,tmp2);
		memcpy(&tmp,&wan_udp.wan_udphdr_data[12],2);
		memcpy(&tmp2,&wan_udp.wan_udphdr_data[36],2);
		printf("Number of Terminate-Ack packets: %u\t%u\n",tmp,tmp2);
		memcpy(&tmp,&wan_udp.wan_udphdr_data[14],2);
		memcpy(&tmp2,&wan_udp.wan_udphdr_data[38],2);
		printf("Number of Code-Reject packets: %u\t%u\n",tmp,tmp2);
		memcpy(&tmp,&wan_udp.wan_udphdr_data[16],2);
		memcpy(&tmp2,&wan_udp.wan_udphdr_data[40],2);
		printf("Number of Protocol-Rej packets: %u\t%u\n",tmp,tmp2);
		memcpy(&tmp,&wan_udp.wan_udphdr_data[18],2);
		memcpy(&tmp2,&wan_udp.wan_udphdr_data[42],2);
		printf("Number of Echo-Request packets: %u\t%u\n",tmp,tmp2);
		memcpy(&tmp,&wan_udp.wan_udphdr_data[20],2);
		memcpy(&tmp2,&wan_udp.wan_udphdr_data[44],2);
		printf("Number of Echo-Reply packets : %u\t%u\n",tmp,tmp2);
		memcpy(&tmp,&wan_udp.wan_udphdr_data[22],2);
		memcpy(&tmp2,&wan_udp.wan_udphdr_data[46],2);
		printf("Number of Discard-Request packets: %u\t%u\n",tmp,tmp2);
	} else {
		error();
	} 
}; /* lcp */

static void flush_lcp( void ) 
{
	wan_udp.wan_udphdr_command = PPP_FLUSH_LCP_STATS;
	wan_udp.wan_udphdr_return_code = 0xaa;
	wan_udp.wan_udphdr_data_len = 0;

	DO_COMMAND(wan_udp);
	if( wan_udp.wan_udphdr_return_code != 0 ) {
		error();
	} 
}; /* flush_packet */

static void loopback( void ) 
{
	unsigned short tmp;
   
	wan_udp.wan_udphdr_command = PPP_READ_LCP_STATS;
	wan_udp.wan_udphdr_return_code = 0xaa;
	wan_udp.wan_udphdr_data_len = 0;
	DO_COMMAND(wan_udp);
	
	if( wan_udp.wan_udphdr_return_code == 0 ) {

		BANNER("LOOPBACK STATISTICS");

		memcpy(&tmp,&wan_udp.wan_udphdr_data[0],2);
		printf("Looped-back link possible given Config-Req/Nak/Rej: %u\n",tmp);
		memcpy(&tmp,&wan_udp.wan_udphdr_data[2],2);
		printf("Looped-back link detected with Echo-Request: %u\n",tmp);
		memcpy(&tmp,&wan_udp.wan_udphdr_data[4],2);
		printf("Echo-Request received from bad source: %u\n",tmp);
		memcpy(&tmp,&wan_udp.wan_udphdr_data[6],2);
		printf("Looped-back link detected with Echo-Reply: %u\n",tmp);
		memcpy(&tmp,&wan_udp.wan_udphdr_data[8],2);
		printf("Echo-Reply received from bad source: %u\n",tmp);
		memcpy(&tmp,&wan_udp.wan_udphdr_data[10],2);
		printf("Looped-back link detected with Discard-Request: %u\n",
			tmp);
		memcpy(&tmp,&wan_udp.wan_udphdr_data[12],2);
		printf("Discard-Request received from bad source: %u\n",tmp);
		memcpy(&tmp,&wan_udp.wan_udphdr_data[14],2);
		printf("Echo-Reply discarded( transmit collision ): %u\n",tmp);
		memcpy(&tmp,&wan_udp.wan_udphdr_data[16],2);
		printf("Echo-Reply discarded( receive collision ): %u\n",tmp);
	
	} else {
		error();
	} 
}; /* loopback */

static void flush_loopback( void ) 
{
	wan_udp.wan_udphdr_command = PPP_FLUSH_LPBK_STATS;
	wan_udp.wan_udphdr_return_code = 0xaa;
	wan_udp.wan_udphdr_data_len = 0;
	DO_COMMAND(wan_udp);

	if( wan_udp.wan_udphdr_return_code != 0 ) {
		error();
	}
 
}; /* flush_loopback */

static void ipcp( void ) 
{
	unsigned short tmp, tmp2;
   
	wan_udp.wan_udphdr_command = PPP_READ_IPCP_STATS;
	wan_udp.wan_udphdr_return_code = 0xaa;
	wan_udp.wan_udphdr_data_len = 0;
	DO_COMMAND(wan_udp);

	if( wan_udp.wan_udphdr_return_code == 0 ) {

		BANNER("IPCP STATISTICS");

		memcpy(&tmp,&wan_udp.wan_udphdr_data[0],2);
		printf("Packets discarded (unknown IPCP code): %u\n",tmp);
		memcpy(&tmp,&wan_udp.wan_udphdr_data[32],2);
		printf("Received IPCP packets too large: %u\n",tmp);
		memcpy(&tmp,&wan_udp.wan_udphdr_data[34],2);
		printf("Received packets invalid or out-of-sequence Configure-Acks: %u\n",tmp);
		memcpy(&tmp,&wan_udp.wan_udphdr_data[36],2);
		printf("Received packets invalid Configure-Naks or Configure-Rejects: %u\n",tmp);
		memcpy(&tmp,&wan_udp.wan_udphdr_data[38],2);
		printf("Configure-Naks or Configure-Rejects with bad Identifier: %u\n",tmp);
		printf("\n\t\t\tReceived\tTransmitted\n");
		memcpy(&tmp,&wan_udp.wan_udphdr_data[2],2);
		memcpy(&tmp2,&wan_udp.wan_udphdr_data[18],2);
		printf("Number of Config-Request pkts: %u\t%u\n",tmp,tmp2);
		memcpy(&tmp,&wan_udp.wan_udphdr_data[4],2);
		memcpy(&tmp2,&wan_udp.wan_udphdr_data[20],2);
		printf("Number of Config-Ack packets : %u\t%u\n",tmp,tmp2);
		memcpy(&tmp,&wan_udp.wan_udphdr_data[6],2);
		memcpy(&tmp2,&wan_udp.wan_udphdr_data[22],2);
		printf("Number of Config-Nack packets: %u\t%u\n",tmp,tmp2);
		memcpy(&tmp,&wan_udp.wan_udphdr_data[8],2);
		memcpy(&tmp2,&wan_udp.wan_udphdr_data[24],2);
		printf("Number of Config-Reject packets: %u\t%u\n",tmp,tmp2);
		memcpy(&tmp,&wan_udp.wan_udphdr_data[10],2);
		memcpy(&tmp2,&wan_udp.wan_udphdr_data[26],2);
		printf("Number of Term-Reqst packets : %u\t%u\n",tmp,tmp2);
		memcpy(&tmp,&wan_udp.wan_udphdr_data[12],2);
		memcpy(&tmp2,&wan_udp.wan_udphdr_data[28],2);
		printf("Number of Terminate-Ack packets: %u\t%u\n",tmp,tmp2);
		memcpy(&tmp,&wan_udp.wan_udphdr_data[14],2);
		memcpy(&tmp2,&wan_udp.wan_udphdr_data[30],2);
		printf("Number of Code-Reject packets: %u\t%u\n",tmp,tmp2);
	} else {
		error();
	} 
}; /* ipcp */

static void flush_ipcp( void ) 
{
	wan_udp.wan_udphdr_command = PPP_FLUSH_IPCP_STATS;
	wan_udp.wan_udphdr_return_code = 0xaa;
	wan_udp.wan_udphdr_data_len = 0;
	DO_COMMAND(wan_udp);

	if( wan_udp.wan_udphdr_return_code != 0 ) {
		error();
	} 
}; /* flush_ipcp */

static void ipxcp( void ) 
{
	unsigned short tmp, tmp2;
  
	wan_udp.wan_udphdr_command = PPP_READ_IPXCP_STATS;
	wan_udp.wan_udphdr_return_code = 0xaa;
	wan_udp.wan_udphdr_data_len = 0;
	DO_COMMAND(wan_udp);
	if( wan_udp.wan_udphdr_return_code == 0 ) {

		BANNER("IPXCP STATISTICS");

		memcpy(&tmp,&wan_udp.wan_udphdr_data[0],2);
		printf("Packets discarded (unknown IPXCP code): %u\n",tmp);
		memcpy(&tmp,&wan_udp.wan_udphdr_data[32],2);
		printf("Received IPXCP packets too large: %u\n",tmp);
		memcpy(&tmp,&wan_udp.wan_udphdr_data[34],2);
		printf("Received packets invalid or out-of-sequence Configure-Acks: %u\n",tmp);
		memcpy(&tmp,&wan_udp.wan_udphdr_data[36],2);
		printf("Received packets invalid Configure-Naks or Configure-Rejects: %u\n",tmp);
		memcpy(&tmp,&wan_udp.wan_udphdr_data[38],2);
		printf("Configure-Naks or Configure-Rejects with bad Identifier: %u\n",tmp);
		printf("\n\t\t\tReceived\tTransmitted\n");
		memcpy(&tmp,&wan_udp.wan_udphdr_data[2],2);
		memcpy(&tmp2,&wan_udp.wan_udphdr_data[18],2);
		printf("Number of Config-Request pkts: %u\t%u\n",tmp,tmp2);
		memcpy(&tmp,&wan_udp.wan_udphdr_data[4],2);
		memcpy(&tmp2,&wan_udp.wan_udphdr_data[20],2);
		printf("Number of Config-Ack packets : %u\t%u\n",tmp,tmp2);
		memcpy(&tmp,&wan_udp.wan_udphdr_data[6],2);
		memcpy(&tmp2,&wan_udp.wan_udphdr_data[22],2);
		printf("Number of Config-Nack packets: %u\t%u\n",tmp,tmp2);
		memcpy(&tmp,&wan_udp.wan_udphdr_data[8],2);
		memcpy(&tmp2,&wan_udp.wan_udphdr_data[24],2);
		printf("Number of Config-Reject packets: %u\t%u\n",tmp,tmp2);
		memcpy(&tmp,&wan_udp.wan_udphdr_data[10],2);
		memcpy(&tmp2,&wan_udp.wan_udphdr_data[26],2);
		printf("Number of Term-Reqst packets : %u\t%u\n",tmp,tmp2);
		memcpy(&tmp,&wan_udp.wan_udphdr_data[12],2);
		memcpy(&tmp2,&wan_udp.wan_udphdr_data[28],2);
		printf("Number of Terminate-Ack packets: %u\t%u\n",tmp,tmp2);
		memcpy(&tmp,&wan_udp.wan_udphdr_data[14],2);
		memcpy(&tmp2,&wan_udp.wan_udphdr_data[30],2);
		printf("Number of Code-Reject packets: %u\t%u\n",tmp,tmp2);
	} else {
		error();
	} //if
}; /* ipxcp */

static void flush_ipxcp( void ) 
{
	wan_udp.wan_udphdr_command = PPP_FLUSH_IPXCP_STATS;
	wan_udp.wan_udphdr_return_code = 0xaa;
	wan_udp.wan_udphdr_data_len = 0;
	DO_COMMAND(wan_udp);
	if( wan_udp.wan_udphdr_return_code != 0 ) {
		error();
	}
}; /* flush_ipxcp */

static void pap( void ) 
{
	unsigned short tmp, tmp2;
   
	wan_udp.wan_udphdr_command = PPP_READ_PAP_STATS;
	wan_udp.wan_udphdr_return_code = 0xaa;
	wan_udp.wan_udphdr_data_len = 0;
	DO_COMMAND(wan_udp);

	if( wan_udp.wan_udphdr_return_code == 0 ) {


		BANNER("PAP STATISTICS");

		memcpy(&tmp,&wan_udp.wan_udphdr_data[0],2);
		printf("Packets discarded (unknown PAP code): %u\n",tmp);
		memcpy(&tmp,&wan_udp.wan_udphdr_data[16],2);
		printf("Received PAP packets too large: %u\n",tmp);
		memcpy(&tmp,&wan_udp.wan_udphdr_data[18],2);
		printf("Received packets invalid inbound PeerID: %u\n",tmp);
		memcpy(&tmp,&wan_udp.wan_udphdr_data[20],2);
		printf("Received packets invalid inbound Password: %u\n",tmp);
		printf("\n\t\t\tReceived\tTransmitted\n");
		memcpy(&tmp,&wan_udp.wan_udphdr_data[2],2);
		memcpy(&tmp2,&wan_udp.wan_udphdr_data[10],2);
		printf("Number of Authent-Request pkts: %u\t%u\n",tmp,tmp2);
		memcpy(&tmp,&wan_udp.wan_udphdr_data[4],2);
		memcpy(&tmp2,&wan_udp.wan_udphdr_data[12],2);
		printf("Number of Authent-Ack packets : %u\t%u\n",tmp,tmp2);
		memcpy(&tmp,&wan_udp.wan_udphdr_data[6],2);
		memcpy(&tmp2,&wan_udp.wan_udphdr_data[14],2);
		printf("Number of Authent-Nack packets: %u\t%u\n",tmp,tmp2);
	} else {
		error();
	} 
}; /* pap */

static void flush_pap( void ) 
{
	wan_udp.wan_udphdr_command = PPP_FLUSH_PAP_STATS;
	wan_udp.wan_udphdr_return_code = 0xaa;
	wan_udp.wan_udphdr_data_len = 0;
	DO_COMMAND(wan_udp);
	if( wan_udp.wan_udphdr_return_code != 0 ) {
		error();
	} 
}; /* flush_pap */

static void chap( void ) 
{
	unsigned short tmp, tmp2;
   
	wan_udp.wan_udphdr_command = PPP_READ_CHAP_STATS;
	wan_udp.wan_udphdr_return_code = 0xaa;
	wan_udp.wan_udphdr_data_len = 0;
	DO_COMMAND(wan_udp);

	if( wan_udp.wan_udphdr_return_code == 0 ) {

		BANNER("CHAP STATISTICS");

		memcpy(&tmp,&wan_udp.wan_udphdr_data[0],2);
		printf("Packets discarded (unknown CHAP code): %u\n",tmp);
		memcpy(&tmp,&wan_udp.wan_udphdr_data[20],2);
		printf("Received CHAP packets too large: %u\n",tmp);
		memcpy(&tmp,&wan_udp.wan_udphdr_data[22],2);
		printf("Received packets invalid inbound PeerID: %u\n",tmp);
		memcpy(&tmp,&wan_udp.wan_udphdr_data[24],2);
		printf("Received packets invalid inbound Password/Secret: %u\n",
			tmp);
		memcpy(&tmp,&wan_udp.wan_udphdr_data[26],2);
		printf("Received packets invalid inbound MD5 message digest format: %u\n",tmp);
		memcpy(&tmp,&wan_udp.wan_udphdr_data[28],2);
		printf("Invalid inbound ID or out-of-order or unelicited responses: %u\n",tmp);
		printf("\n\t\t\tReceived\tTransmitted\n");
		memcpy(&tmp,&wan_udp.wan_udphdr_data[2],2);
		memcpy(&tmp2,&wan_udp.wan_udphdr_data[12],2);
		printf("Number of Challenge packets  : %u\t%u\n",tmp,tmp2);
		memcpy(&tmp,&wan_udp.wan_udphdr_data[4],2);
		memcpy(&tmp2,&wan_udp.wan_udphdr_data[14],2);
		printf("Number of Response packets   : %u\t%u\n",tmp,tmp2);
		memcpy(&tmp,&wan_udp.wan_udphdr_data[6],2);
		memcpy(&tmp2,&wan_udp.wan_udphdr_data[16],2);
		printf("Number of Success packets    : %u\t%u\n",tmp,tmp2);
		memcpy(&tmp,&wan_udp.wan_udphdr_data[8],2);
		memcpy(&tmp2,&wan_udp.wan_udphdr_data[18],2);
		printf("Number of Failure packets    : %u\t%u\n",tmp,tmp2);
	} else {
		error();
	} 
}; /* chap */

static void flush_chap( void ) 
{
	wan_udp.wan_udphdr_command = PPP_FLUSH_CHAP_STATS;
	wan_udp.wan_udphdr_return_code = 0xaa;
	wan_udp.wan_udphdr_data_len = 0;
	DO_COMMAND(wan_udp);

	if( wan_udp.wan_udphdr_return_code != 0 ) {
		error();
	} 
}; /* flush_chap */

int PPPUsage( void ) 
{
	printf("wanpipemon: Wanpipe PPP Debugging Utility\n\n");
	printf("Usage:\n");
	printf("-----\n\n");
	printf("wanpipemon -i <ip-address or interface name> -u <port> -c <command>\n\n");
	printf("\tOption -i: \n");
	printf("\t\tWanpipe remote IP address must be supplied\n");
	printf("\t\t<or> Wanpipe network interface name (ex: wp1_ppp)\n");   
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
	printf("\t             n       Parameters Negotiated on Last Connection/Attempt\n");
	printf("\t             ru      Display Router UP time\n");
	printf("\t             u       PPP Timers and Counters\n");
	printf("\t             s       PPP FSM Current State\n");
	printf("\t             c       Cause for Last Disconnection\n");
	printf("\tCard Configuration\n");
	printf("\t   c         g       PPP General Configuration\n");
	printf("\t             a       Authentication Configuration\n");
	printf("\t             i       IP Configuration\n");
	printf("\t             x       IPX Configuration\n");
	printf("\tCard Statistics\n");
	printf("\t   s         g       Global Statistics\n");
	printf("\t             c       Communication Error Statistics\n");
	printf("\t             p       Packet Statistics\n");
	printf("\t             lpc     LCP Statistics\n");
	printf("\t             lo      Loopback Detection / LCP Error Statistics\n");
	printf("\t             ipc     IP Control Protocol( IPCP )Statistics\n");
	printf("\t             xpc     IPX Control Protocol( IPXCP )Statistics\n");
	printf("\t             pap     Password Authentication (PAP) Statistics\n");
	printf("\t             chp     Challenge-Handshake Auth.(CHAP) Statistics\n");
	printf("\tTrace Data\n");
	printf("\t   t         i       Trace and Interpret ALL frames\n");
	printf("\t             ip      Trace and Interpret PROTOCOL frames only\n");
	printf("\t             id      Trace and Interpret DATA frames only\n");
	printf("\t             r       Trace ALL frames, in RAW format\n");
	printf("\t             rp      Trace PROTOCOL frames only, in RAW format\n");
	printf("\t             rd      Trace DATA frames only, in RAW format\n");
	printf("\tFT1/T1/E1/56K Configuration/Statistics\n");
	printf("\t   T         v       View Status\n");
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
	printf("\t             a       Read T1/E1/56K alarms\n");  
	printf("\tFlush Statistics\n");
	printf("\t   f         g       Global Statistics\n");
	printf("\t             c       Communication Error Statistics\n");
	printf("\t             p       Packet Statistics\n");
	printf("\t             lpc     LCP Statistics\n");
	printf("\t             lo      Loopback Detection / LCP Error Statistics\n");
	printf("\t             ipc     IP Control Protocol( IPCP )Statistics\n");
	printf("\t             ipxc    IPX Control Protocol( IPXCP )Statistics\n");
	printf("\t             pap     Password Authentication (PAP) Statistics\n");
	printf("\t             chp     Challenge-Handshake Auth.(CHAP) Statistics\n");
	printf("\t             d       Flush Driver Statistics\n");
	printf("\t             pm      Flush T1/E1 performance monitoring counters\n");
	printf("\tDriver Statistics\n");
	printf("\t   d         ds      Display Send Driver Statistics\n");
	printf("\t             di      Display Interrupt Driver Statistics\n");
	printf("\t             dg      Display General Driver Statistics\n"); 
	printf("\n\tExamples:\n");
	printf("\t--------\n\n");
	printf("\tex: wanpipemon -i wp1_ppp -u 9000 -c xm    :View Modem Status \n");
	printf("\tex: wanpipemon -i 201.1.1.2 -u 9000 -c ti  :Trace and Interpret ALL frames\n\n");
	return 0;
}

static char *gui_main_menu[]={
"ppp_card_stats_menu","Card Status",
"ppp_card_conf_menu","Card Configuration",
"ppp_stats_menu","Card Statistics",
"ppp_trace_menu","Trace Data",
"csudsu_menu","FT1/T1/E1/56K Configuration/Statistics",
"ppp_flush_menu","Flush Statistics",
"ppp_driver_menu","Driver Statistics",
"."
};

static char *ppp_card_stats_menu[]={
"xm","Modem Status",
"xn","Negotiated Parameters",
"xru","Display Router UP time",
"xu","PPP Timers and Counters",
"xs","PPP FSM Current State",
"xc","Cause for Last Disconnection",
"."	
};

static char *ppp_card_conf_menu[]={
"cg","PPP General Configuration",
"ca","Authentication Configuration",
"ci","IP Configuration",
"cx","IPX Configuration",
"."
};
	
static char *ppp_stats_menu[]={
"sg","Global Statistics",
"sc","Communication Error Statistics",
"sp","Packet Statistics",
"slpc","LCP Statistics",
"slo","Loopback Detection / LCP Error Statistics",
"sipc","IP Control Protocol( IPCP )Statistics",
"sxpc","IPX Control Protocol( IPXCP )Statistics",
"spap","Password Authentication (PAP) Statistics",
"schp","Challenge-Handshake Auth.(CHAP) Statistics",
"."
};

static char *ppp_trace_menu[]={
"ti","Trace and Interpret ALL frames",
"tip","Trace and Interpret PROTOCOL frames only",
"tid","Trace and Interpret DATA frames only",
"tr","Trace ALL frames, in RAW format",
"trp","Trace PROTOCOL frames only, in RAW format",
"trd","Trace DATA frames only, in RAW format",
"."
};

#if 0
static char *ppp_csudsu_menu[]={
"Tv","View Status",
"Ts","Self Test",
"Tl","Line Loop Test",
"Td","Digital Loop Test",
"Tr","Remote Test",
"To","Operational Mode",
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

static char *ppp_flush_menu[]={
"fg","Flush Global Statistics",
"fc","Flush Communication Error Statistics",
"fp","Flush Packet Statistics",
"flpc","Flush LCP Statistics",
"flo","Flush Loopback Detection / LCP Error Statistics",
"fipc","Flush IP Control Protocol( IPCP )Statistics",
"fxpc","Flush IPX Control Protocol( IPXCP )Statistics",
"fpap","Flush Password Authentication (PAP) Statistics",
"fchp","Flush Challenge-Handshake Auth.(CHAP) Statistics",
"fpm","Flush Driver Statistics",
"fpm","Flush T1/E1 performance monitoring counters",
"."
};

static char *ppp_driver_menu[]={
"ds","Display Send Driver Statistics",
"di","Display Interrupt Driver Statistics",
"dg","Display General Driver Statistics", 
"."
};

static struct cmd_menu_lookup_t gui_cmd_menu_lookup[]={
	{"ppp_card_stats_menu",ppp_card_stats_menu},
	{"ppp_card_conf_menu",ppp_card_conf_menu},
	{"ppp_stats_menu",ppp_stats_menu},
	{"ppp_trace_menu",ppp_trace_menu},
	{"csudsu_menu",csudsu_menu},
	{"ppp_flush_menu",ppp_flush_menu},
	{"ppp_driver_menu",ppp_driver_menu},
	{".",NULL}
};


char ** PPPget_main_menu(int *len)
{
	int i=0;
	while(strcmp(gui_main_menu[i],".") != 0){
		i++;
	}
	*len=i/2;
	return gui_main_menu;
}

char ** PPPget_cmd_menu(char *cmd_name,int *len)
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



int  PPPDisableTrace(void)
{
	wan_udp.wan_udphdr_command = PPIPE_DISABLE_TRACING;
	wan_udp.wan_udphdr_return_code = 0xaa;
        wan_udp.wan_udphdr_data_len = 1;
        wan_udp.wan_udphdr_data[0] = DISABLE_TRACING;
        DO_COMMAND(wan_udp);
	return 0;
}

static void line_trace(int trace_mode) 
{
	unsigned int num_frames;
	unsigned short curr_pos = 0;
	wan_trace_pkt_t* frame_info;
	unsigned int i;
	int recv_buff = sizeof(wan_udp_hdr_t);
	fd_set ready;
	struct timeval to;
	wp_trace_output_iface_t trace_iface;
   
	memset(&trace_iface,0,sizeof(wp_trace_output_iface_t));

	setsockopt( sock, SOL_SOCKET, SO_RCVBUF, &recv_buff, sizeof(int) );

        wan_udp.wan_udphdr_command = PPIPE_DISABLE_TRACING;
	wan_udp.wan_udphdr_return_code = 0xaa;
        wan_udp.wan_udphdr_data_len = 1;
        wan_udp.wan_udphdr_data[0] = DISABLE_TRACING;
        DO_COMMAND(wan_udp);   	

	wan_udp.wan_udphdr_command = PPIPE_ENABLE_TRACING;
	wan_udp.wan_udphdr_return_code = 0xaa;
	wan_udp.wan_udphdr_data_len = 1;
	wan_udp.wan_udphdr_data[0]=0;
	if(trace_mode == TRACE_PROT){
                wan_udp.wan_udphdr_data[0] = TRACE_SIGNALLING_FRAMES;
        }else if(trace_mode == TRACE_DATA){
                wan_udp.wan_udphdr_data[0] = TRACE_DATA_FRAMES;
        }else{
                wan_udp.wan_udphdr_data[0] = TRACE_SIGNALLING_FRAMES | 
			           TRACE_DATA_FRAMES; 
	}
	DO_COMMAND(wan_udp);

	if( wan_udp.wan_udphdr_return_code == 0 ) { 
		printf("Starting trace...(Press ENTER to exit)\n");
		fflush(stdout);
	} else if( wan_udp.wan_udphdr_return_code == 0xCD ) {
		printf("Cannot Enable Line Tracing from Underneath.\n");
		fflush(stdout);
		return;
	} else if( wan_udp.wan_udphdr_return_code == 0x01 ) {
		printf("Starting trace...(although it's already enabled!)\n");
		printf("Press ENTER to exit.\n");
		fflush(stdout);
	} else {
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

		wan_udp.wan_udphdr_command = PPIPE_GET_TRACE_INFO;
		wan_udp.wan_udphdr_return_code = 0xaa;
		wan_udp.wan_udphdr_data_len = 0;
		DO_COMMAND(wan_udp);

		if (wan_udp.wan_udphdr_return_code == 0 && wan_udp.wan_udphdr_data_len) { 
			/* if we got something back then get number of frames */
			num_frames = wan_udp.wan_udphdr_data[0] >> 2;
			for ( i=0; i<num_frames; i++) {
				
				frame_info = (wan_trace_pkt_t*)&wan_udp.wan_udphdr_data[curr_pos];

				/*  frame type */
				if (frame_info->status & 0x01) {
					trace_iface.status|=WP_TRACE_OUTGOING;
				}

				trace_iface.len = frame_info->real_length;
				trace_iface.timestamp = frame_info->time_stamp; 

				/* first update curr_pos */
				curr_pos += sizeof(wan_trace_pkt_t);
				
				if (frame_info->data_avail != 0) {
			 	   	/* update curr_pos again */
				   	curr_pos += (frame_info->real_length-1);

				   	trace_iface.trace_all_data=trace_all_data;
				   	trace_iface.data = (unsigned char*)&frame_info->data[0];
					trace_iface.link_type=9;

					if (pcap_output){
						trace_iface.type=WP_OUT_TRACE_PCAP;
					}else if (raw_data) {
						trace_iface.type=WP_OUT_TRACE_RAW;
					}else{
						trace_iface.type=WP_OUT_TRACE_INTERP;
					}

					wp_trace_output(&trace_iface);
		  		   	fflush(stdout);

				} //if

				/*  we have to stay on even boundary here, so 
				    update the curr_pos if needed. It's done 
				    exactly the same way in driver.
 			 	 */
 
				if (curr_pos & 0x0001) curr_pos++;
			} //for
		} //if
		curr_pos = 0;

		if (!(wan_udp.wan_udphdr_data[0] &= 0x02)) { 
			to.tv_sec = 0;
			to.tv_usec = WAN_TRACE_DELAY;
		}else{
			to.tv_sec = 0;
			to.tv_usec = 0;
		}	
	}

	wan_udp.wan_udphdr_command = PPIPE_DISABLE_TRACING;
	wan_udp.wan_udphdr_return_code = 0xaa;
	wan_udp.wan_udphdr_data_len = 0;
	DO_COMMAND(wan_udp);
}; /* line_trace */


/* This subroutine enables the FT1 monitor on the board */

static void set_FT1_monitor_status( unsigned char status) 
{
	fail = 0;
	wan_udp.wan_udphdr_command = FT1_MONITOR_STATUS_CTRL;
	wan_udp.wan_udphdr_return_code = 0xaa;
	wan_udp.wan_udphdr_data_len = 1;
	wan_udp.wan_udphdr_data[0] = status; 	
	DO_COMMAND(wan_udp);
	
	if( wan_udp.wan_udphdr_return_code != 0 && status){
		fail = 1;
		printf("This wan_udphdr_command is only possible with S508/FT1 board!");
	}
} /* set_FT1_monitor_status */



static void ppp_driver_stat_ifsend( void )
{
	if_send_stat_t *if_stats;
	wan_udp.wan_udphdr_command = PPIPE_DRIVER_STAT_IFSEND;
	wan_udp.wan_udphdr_return_code = 0xaa;
	wan_udp.wan_udphdr_data_len = 0;
	wan_udp.wan_udphdr_data[0] = 0;
	DO_COMMAND(wan_udp);
      	if_stats = (if_send_stat_t *)&wan_udp.wan_udphdr_data[0];


      	printf("                                 Total number of Send entries:  %lu\n", 
  		if_stats->if_send_entry);
#if defined(__LINUX__)
      	printf("                       Number of Send entries with SKB = NULL:  %lu\n",
 		if_stats->if_send_skb_null);
#else
      	printf("                      Number of Send entries with mbuf = NULL:  %lu\n",
 		if_stats->if_send_skb_null);
#endif
      	printf("Number of Send entries with broadcast addressed pkt discarded:  %lu\n",
		if_stats->if_send_broadcast);
      	printf("Number of Send entries with multicast addressed pkt discarded:  %lu\n",
		if_stats->if_send_multicast);
      	printf("             Number of Send entries with CRITICAL_RX_INTR set:  %lu\n",
		if_stats->if_send_critical_ISR);
      	printf("   Number of Send entries with Critical set and pkt discarded:  %lu\n",
		if_stats->if_send_critical_non_ISR);
      	printf("                  Number of Send entries with Device Busy set:  %lu\n",
		if_stats->if_send_tbusy);
      	printf("              Number of Send entries with Device Busy Timeout:  %lu\n",
		if_stats->if_send_tbusy_timeout);
      	printf("           Number of Send entries with PTPIPE MONITOR Request:  %lu\n",
		if_stats->if_send_PIPE_request);
      	printf("                 Number of Send entries with WAN Disconnected:  %lu\n",
		if_stats->if_send_wan_disconnected);
      	printf("                   Number of Send entries with Protocol Error:  %lu\n",
		if_stats->if_send_protocol_error);
      	printf("       Number of Send entries with Transmit Interrupt Enabled:  %lu\n",
		if_stats->if_send_tx_int_enabled);
	printf("              Number of Send entries with Adapter Send Failed:  %lu\n",
		if_stats->if_send_bfr_not_passed_to_adptr);
      	printf("              Number of Send entries with Adapter Send passed:  %lu\n",
		if_stats->if_send_bfr_passed_to_adptr);
// 	printf("               Number of times host irq left disabled in Send:  %u\n", 
//		if_stats. ); 

} /* ppp_driver_stat_ifsend */

static void ppp_driver_stat_intr( void )
{
	rx_intr_stat_t *rx_stat;
	global_stats_t *g_stat;
      	wan_udp.wan_udphdr_command = PPIPE_DRIVER_STAT_INTR;
	wan_udp.wan_udphdr_return_code = 0xaa;
     	wan_udp.wan_udphdr_data_len = 0;
      	wan_udp.wan_udphdr_data[0] = 0;
      	DO_COMMAND(wan_udp);
	g_stat = (global_stats_t *)&wan_udp.wan_udphdr_data[0];
	rx_stat = (rx_intr_stat_t *)&wan_udp.wan_udphdr_data[sizeof(global_stats_t)];
	
      
      	printf("                                    Number of ISR entries:   %lu\n", 
		g_stat->isr_entry );
      	printf("                  Number of ISR entries with Critical Set:   %lu\n", 
		g_stat->isr_already_critical );
      	printf("                              Number of Receive Interrupt:   %lu\n", 
		g_stat->isr_rx );
      	printf("                             Number of Transmit Interrupt:   %lu\n", 
		g_stat->isr_tx );
      	printf("              Number of ISR entries for Interrupt Testing:   %lu\n", 
		g_stat->isr_intr_test );
      	printf("                             Number of Spurious Interrupt:   %lu\n", 
		g_stat->isr_spurious );
      	printf("       Number of times Transmit Interrupts Enabled in ISR:   %lu\n", 
		g_stat->isr_enable_tx_int );
      	printf("         Number of Receive Interrupts with Corrupt Buffer:   %lu\n", 
		g_stat->rx_intr_corrupt_rx_bfr );
      	printf("              Number of Receive Interrupts with No socket:   %lu\n", 
		rx_stat->rx_intr_no_socket );
      	printf("  Number of Receive Interrupts for PTPIPE MONITOR Request:   %lu\n",
		rx_stat->rx_intr_PIPE_request);
      	printf(" Number of Receive Interrupts with Buffer Passed to Stack:   %lu\n",
		rx_stat->rx_intr_bfr_passed_to_stack);
        printf(" Number of Receive Inter. with Buffer NOT Passed to Stack:   %lu\n",
		rx_stat->rx_intr_bfr_not_passed_to_stack);
      	printf("     Number of Receive Interrupts with Device not started:   %lu\n", 
		g_stat->rx_intr_dev_not_started );
      	printf("    Number of Transmit Interrupts with Device not started:   %lu\n", 
		g_stat->tx_intr_dev_not_started );
     
}


static void ppp_driver_stat_gen( void )
{
	pipe_mgmt_stat_t *p_stat;
	global_stats_t *g_stat;
      	wan_udp.wan_udphdr_command = PPIPE_DRIVER_STAT_GEN;
	wan_udp.wan_udphdr_return_code = 0xaa;
      	wan_udp.wan_udphdr_data_len = 0;
      	wan_udp.wan_udphdr_data[0] = 0;
      	DO_COMMAND(wan_udp);
	p_stat = (pipe_mgmt_stat_t *)&wan_udp.wan_udphdr_data[0];
	g_stat = (global_stats_t *)&wan_udp.wan_udphdr_data[sizeof(pipe_mgmt_stat_t)];
     
#if defined(__LINUX__)
      	printf("           Number of PTPIPE Monitor call with Kmalloc error:  %lu\n",
		p_stat->UDP_PIPE_mgmt_kmalloc_err );
#else
      	printf("           Number of PTPIPE Monitor call with Kmalloc error:  %lu\n",
		p_stat->UDP_PIPE_mgmt_kmalloc_err );
#endif
      	printf("      Number of PTPIPE Monitor call with Adapter Type error:  %lu\n",
		p_stat->UDP_PIPE_mgmt_adptr_type_err );
      	printf("         Number of PTPIPE Monitor call with Direction Error:  %lu\n",
		p_stat->UDP_PIPE_mgmt_direction_err );
      	printf(" Number of PTPIPE Monitor call with Adapter Command Timeout:  %lu\n",
		p_stat->UDP_PIPE_mgmt_adptr_cmnd_timeout );
      	printf("      Number of PTPIPE Monitor call with Adapter Command OK:  %lu\n",
		p_stat->UDP_PIPE_mgmt_adptr_cmnd_OK );
      	printf("  Number of PTPIPE Monitor call with pkt passed to Adapter:   %lu\n",
		p_stat->UDP_PIPE_mgmt_passed_to_adptr );
      	printf("    Number of PTPIPE Monitor call with pkt passed to Stack:   %lu\n",
		p_stat->UDP_PIPE_mgmt_passed_to_stack);
      	printf("              Number of PTPIPE Monitor call with no socket:   %lu\n",
		p_stat->UDP_PIPE_mgmt_no_socket);
      	printf("                                    Number of Poll Entries:   %lu\n",
		g_stat->poll_entry);
      	printf("                  Number of Poll Entries with Critical set:   %lu\n",
		g_stat->poll_already_critical);
      	printf("                          Number of Poll Entries Processed:   %lu\n",
		g_stat->poll_processed);
      	printf("            Number of times host irq left disabled in Poll:   %lu\n", 
		g_stat->poll_host_disable_irq);

} /* ppp_driver_stat_ifsend */

static void flush_driver_stats( void )
{
      	wan_udp.wan_udphdr_command = PPIPE_FLUSH_DRIVER_STATS;
	wan_udp.wan_udphdr_return_code = 0xaa;
      	wan_udp.wan_udphdr_data_len = 0;
      	wan_udp.wan_udphdr_data[0] = 0;
      	DO_COMMAND(wan_udp);

      	printf("All Driver Statistics are Flushed.\n");

}

static void ppp_router_up_time( void )
{
     	unsigned long time;
     	wan_udp.wan_udphdr_command = PPIPE_ROUTER_UP_TIME;
	wan_udp.wan_udphdr_return_code = 0xaa;
     	wan_udp.wan_udphdr_data_len = 0;
     	wan_udp.wan_udphdr_data[0] = 0;
     	DO_COMMAND(wan_udp);
     
     	time = *(unsigned long*)&wan_udp.wan_udphdr_data[0];

     	if (time < 3600) {
		if (time<60) 
     			printf("    Router UP Time:  %lu seconds\n", time);
		else
     			printf("    Router UP Time:  %lu minute(s)\n", (time/60));
     	}else
     		printf("    Router UP Time:  %lu hour(s)\n", (time/3600));
			
      
}

int PPPMain(char *command,int argc, char* argv[])
{
	char *opt=&command[1];

   	printf("\n");
	switch(command[0]) {
		
		case 'x':
			if (!strcmp(opt,"m")){
				modem();
			}else if (!strcmp(opt,"u")){
				timers();	
			}else if (!strcmp(opt,"n")){
				negot();
			}else if (!strcmp(opt,"c")){
				cause();
			}else if (!strcmp(opt,"s")){
				state();
			}else if (!strcmp(opt,"ru")){
				ppp_router_up_time();
			}else{
				printf("ERROR: Invalid Status Command 'x', Type wanpipemon <cr> for help\n\n");
			}
			break;


		case 'c':
			if (!strcmp(opt,"i")){
				ip_config();	
			}else if (!strcmp(opt,"x")){
				ipx_config();
			}else if (!strcmp(opt,"a")){
				authent();
			}else if (!strcmp(opt,"g")){
				general_conf();	
			}else{
				printf("ERROR: Invalid Configuration Command 'c', Type wanpipemon <cr> for help\n\n");		
			}
			break;

		case 's':
			if (!strcmp(opt,"g")){
				general_stats();
			}else if (!strcmp(opt,"c")){
				comm_err();
			}else if (!strcmp(opt,"p")){
				packet();
			}else if (!strcmp(opt,"lo")){
				loopback();
			}else if (!strcmp(opt,"pap")){
				pap();
			}else if (!strcmp(opt,"chp")){
				chap();			
			}else if (!strcmp(opt,"ipc")){
				ipcp();
			}else if (!strcmp(opt,"xpc")){
				ipxcp();			
			}else if (!strcmp(opt,"lpc")){
				lcp();
			}else{
				printf("ERROR: Invalid Statistics Command 's', Type wanpipemon <cr> for help\n\n");
			}
			break;

		case 't':
			if(!strcmp(opt,"i" )){
				raw_data = WAN_FALSE;
				line_trace(TRACE_ALL);
			}else if (!strcmp(opt, "ip")){
				raw_data = WAN_FALSE;
				line_trace(TRACE_PROT);
			}else if (!strcmp(opt, "id")){
				raw_data = WAN_FALSE;
				line_trace(TRACE_DATA);
			}else if (!strcmp(opt, "r")){
				raw_data = WAN_TRUE;
				line_trace(TRACE_ALL);
			}else if (!strcmp(opt, "rp")){
				raw_data = WAN_TRUE;
				line_trace(TRACE_PROT);
			}else if (!strcmp(opt, "rd")){
				raw_data = WAN_TRUE;
				line_trace(TRACE_DATA);
			}else{
				printf("ERROR: Invalid Trace Command 't', Type wanpipemon <cr> for help\n\n");
			}
			break;
		case 'd':
			if(!strcmp(opt,"s" )){
				ppp_driver_stat_ifsend();
			}else if (!strcmp(opt,"i" )){ 
				ppp_driver_stat_intr();	 
			}else if (!strcmp(opt,"g" )){ 
				ppp_driver_stat_gen();	 
			}else{
				printf("ERROR: Invalid Driver Stat Command 't', Type wanpipemon <cr> for help\n\n");
			}
			break;
		case 'f':
			if(!strcmp(opt,"g" )){	
				flush_general_stats();
				general_stats();
			}else if (!strcmp(opt,"c" )){
				flush_comm_err();
				comm_err();
			}else if (!strcmp(opt,"p" )){
				flush_packet();
				packet();
			}else if (!strcmp(opt,"lcp" )){
				flush_lcp();
				lcp();
			}else if (!strcmp(opt,"lo" )){
				flush_loopback();
				loopback();
			}else if (!strcmp(opt,"ipc" )){
				flush_ipcp();
				ipcp();
			}else if (!strcmp(opt,"ipxc" )){
				flush_ipxcp();
				ipxcp();
			}else if (!strcmp(opt,"pap" )){
				flush_pap();
				pap();
			}else if (!strcmp(opt,"chp" )){
				flush_chap();
				chap();
			}else if (!strcmp(opt,"d" )){
				flush_driver_stats();
			}else if (!strcmp(opt,"pm" )){
				flush_te1_pmon();
			} else {
				printf("ERROR: Invalid Flush Command 'f', Type wanpipemon <cr> for help\n\n");
			}
			break;
		case 'T':
			if (!strcmp(opt,"v" )){
				set_FT1_monitor_status(0x01);
				if(!fail){
					view_FT1_status();
				 }
				set_FT1_monitor_status(0x00);
			}else if (!strcmp(opt,"s" )){
				set_FT1_monitor_status(0x01);
				if(!fail){	 	
					FT1_self_test();
				}
				set_FT1_monitor_status(0x00);
			}else if (!strcmp(opt,"l" )){
				set_FT1_monitor_status(0x01);
				if(!fail){
					FT1_local_loop_mode();
				}
				set_FT1_monitor_status(0x00);
			}else if (!strcmp(opt,"d" )){
				set_FT1_monitor_status(0x01);
				if(!fail){
					FT1_digital_loop_mode();
				}
				set_FT1_monitor_status(0x00);
			}else if (!strcmp(opt,"r" )){
				set_FT1_monitor_status(0x01);
				if(!fail){
					FT1_remote_test();
				}
				set_FT1_monitor_status(0x00);
			}else if (!strcmp(opt,"o" )){
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
				read_te1_56k_stat();
			} else{
				printf("ERROR: Invalid FT1 Command 'T', Type wanpipemon <cr> for help\n\n");
			} 
			break; 
		default:
			printf("ERROR: Invalid Command, Type wanpipemon <cr> for help\n\n");

	} //switch
   printf("\n");
   fflush(stdout);
   return 0;
}; //main

/*
 * EOF wanpipemon.c
 */
