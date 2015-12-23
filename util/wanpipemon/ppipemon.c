/*****************************************************************************
* ppipemon.c	PPP Monitor.
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
* June 2, 2006  David Rokhvarg  Fixed Statistics counters for PPP.
* Jan 11, 2005  David Rokhvarg  Added code to run above AFT card with protocol
* 				in the LIP layer.
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
#if !defined (__LINUX__)
# include <netinet/in_systm.h>
# include <netinet/in.h>
# include <netinet/ip.h>
# include <netinet/udp.h>      
#endif

#include "wanpipe_api.h"
#include "sdla_ppp.h"
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
static u_int32_t decode_bps( unsigned char bps );
static void set_FT1_monitor_status( unsigned char);


static void	ppp_driver_stat_gen( void );

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
}

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
   
	strlcpy(codeversion, "?.??",10);
   
	wan_udp.wan_udphdr_command = PPP_READ_CODE_VERSION;
	wan_udp.wan_udphdr_return_code = 0xaa;
	wan_udp.wan_udphdr_data_len = 0;
	DO_COMMAND(wan_udp);

	if (wan_udp.wan_udphdr_return_code == 0) {
		wan_udp.wan_udphdr_data[wan_udp.wan_udphdr_data_len] = 0;
		strlcpy(codeversion, (char*)wan_udp.wan_udphdr_data,10);
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



static u_int32_t decode_bps( unsigned char bps ) 
{
	u_int32_t number;
 
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
}

static void general_conf( void ) 
{
	u_int32_t baud;
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

		printf("Baud rate in bps: %u\n",baud);
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

static void authent( void ) 
{
        s_auth_t *auth = (s_auth_t *)&wan_udp.wan_udphdr_data[0];

        wan_udp.wan_udphdr_command = PPP_READ_AUTH;
        wan_udp.wan_udphdr_return_code = 0xaa;
        wan_udp.wan_udphdr_data_len = 0;
        DO_COMMAND(wan_udp);

	if( wan_udp.wan_udphdr_return_code == 0 ) {
			
		BANNER("PPP AUTHENTICATION");
		printf("Allow the use of PAP for inbound/outbound:  ");
                if (auth->proto == PPP_PAP) {
                        printf("Yes\n");
                }
                else {
                        printf("No\n");
                }
                printf("Allow the use of CHAP for inbound/outbound: ");
                if (auth->proto == PPP_CHAP) {
                        printf("Yes\n");
                }
                else {
                        printf("No\n");
                }

                printf("Current Authentication Status:");
                if (auth->authenticated == 1) {
                        printf(" Authenticated\n");
                }
                else {
                        printf(" Not Authenticated\n");
                }

#if 0
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
#endif
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

/*
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

}
*/

static void state( void ) 
{
	wan_udp.wan_udphdr_command = PPP_GET_LINK_STATUS;
	wan_udp.wan_udphdr_return_code = 0xaa;
	wan_udp.wan_udphdr_data_len = 0;
	DO_COMMAND(wan_udp);

	if( wan_udp.wan_udphdr_return_code == 0 ) {
		BANNER("PPP LINK STATUS");
		printf("Link Status: %s\n", (wan_udp.wan_udphdr_data[0] == 1 ? "Connected" : "Disconnected"));
	} else {
		error();
	}
}; /* state */

static void lcp( void ) 
{
	ppp_lcp_stats_t *lcp_stats = (ppp_lcp_stats_t *)&wan_udp.wan_udphdr_data[0];

	wan_udp.wan_udphdr_command = PPP_READ_LCP_STATS;
	wan_udp.wan_udphdr_return_code = 0xaa;
	wan_udp.wan_udphdr_data_len = 0;
	DO_COMMAND(wan_udp);

	if( wan_udp.wan_udphdr_return_code == 0 ) {
		BANNER("LCP STATISTICS");

		printf("Packets discarded (unknown LCP code): %u\n",
			lcp_stats->rx_unknown);
		printf("Received LCP packets too large: %u\n",
			lcp_stats->rx_too_large);
		printf("Received packets invalid or out-of-sequence Configure-Acks: %u\n",
			lcp_stats->rx_ack_inval);
		printf("Received packets invalid Configure-Naks or Configure-Rejects: %u\n",
			lcp_stats->rx_rej_inval);
		printf("Configure-Naks or Configure-Rejects with bad Identifier: %u\n",
			lcp_stats->rx_rej_badid);

		printf("\n\t\t\tReceived\tTransmitted\n");
		printf("Number of Config-Request pkts: %u\t%u\n",
			lcp_stats->rx_conf_rqst, lcp_stats->tx_conf_rqst);

		printf("Number of Config-Ack packets : %u\t%u\n",
			lcp_stats->rx_conf_ack ,lcp_stats->tx_conf_ack );
		printf("Number of Config-Nack packets: %u\t%u\n",
			lcp_stats->rx_conf_nak, lcp_stats->tx_conf_nak);
		printf("Number of Config-Reject packets: %u\t%u\n",
			lcp_stats->rx_conf_rej  , lcp_stats->tx_conf_rej);
		printf("Number of Term-Reqst packets : %u\t%u\n",
			lcp_stats->rx_term_rqst, lcp_stats->tx_term_rqst);
		printf("Number of Terminate-Ack packets: %u\t%u\n",
			lcp_stats->rx_term_ack, lcp_stats->tx_term_ack);
		printf("Number of Code-Reject packets: %u\t%u\n",
			lcp_stats->rx_code_rej, lcp_stats->tx_code_rej);
		printf("Number of Protocol-Rej packets: %u\t%u\n",
			lcp_stats->rx_proto_rej, lcp_stats->tx_proto_rej);
		printf("Number of Echo-Request packets: %u\t%u\n",
			lcp_stats->rx_echo_rqst, lcp_stats->tx_echo_rqst);
		printf("Number of Echo-Reply packets : %u\t%u\n",
			lcp_stats->rx_echo_reply, lcp_stats->tx_echo_reply);
		printf("Number of Discard-Request packets: %u\t%u\n",
			lcp_stats->rx_disc_rqst, lcp_stats->tx_disc_rqst);
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

static void ipcp( void ) 
{
	ppp_prot_stats_t *ipcp_stats = (ppp_prot_stats_t*)&wan_udp.wan_udphdr_data[0];
 
	wan_udp.wan_udphdr_command = PPP_READ_IPCP_STATS;
	wan_udp.wan_udphdr_return_code = 0xaa;
	wan_udp.wan_udphdr_data_len = 0;
	DO_COMMAND(wan_udp);

	if( wan_udp.wan_udphdr_return_code == 0 ) {

		BANNER("IPCP STATISTICS");

		printf("Packets discarded (unknown IPCP code): %u\n", ipcp_stats->rx_unknown);
		printf("Received IPCP packets too large: %u\n", ipcp_stats->rx_too_large);
		printf("Received packets invalid or out-of-sequence Configure-Acks: %u\n", ipcp_stats->rx_ack_inval);
		printf("Received packets invalid Configure-Naks or Configure-Rejects: %u\n", ipcp_stats->rx_ack_inval);
		printf("Configure-Naks or Configure-Rejects with bad Identifier: %u\n", ipcp_stats->rx_rej_badid);

		printf("\n\t\t\tReceived\tTransmitted\n");
		printf("Number of Config-Request pkts: %u\t%u\n",
			ipcp_stats->rx_conf_rqst, ipcp_stats->tx_conf_rqst);
		printf("Number of Config-Ack packets : %u\t%u\n",
			ipcp_stats->rx_conf_ack, ipcp_stats->tx_conf_ack);
		printf("Number of Config-Nack packets: %u\t%u\n",
			ipcp_stats->rx_conf_nak, ipcp_stats->tx_conf_nak);
		printf("Number of Config-Reject packets: %u\t%u\n",
			ipcp_stats->rx_conf_rej, ipcp_stats->tx_conf_rej);
		printf("Number of Term-Reqst packets : %u\t%u\n",
			ipcp_stats->rx_term_rqst, ipcp_stats->tx_term_rqst);
		printf("Number of Terminate-Ack packets: %u\t%u\n",
			ipcp_stats->rx_term_ack, ipcp_stats->tx_term_ack);
		printf("Number of Code-Reject packets: %u\t%u\n",
			ipcp_stats->rx_code_rej, ipcp_stats->tx_code_rej);
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

#if 0
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
#endif

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
   	ppp_chap_stats_t *chap_stats;
   
	wan_udp.wan_udphdr_command = PPP_READ_CHAP_STATS;
	wan_udp.wan_udphdr_return_code = 0xaa;
	wan_udp.wan_udphdr_data_len = 0;
	DO_COMMAND(wan_udp);

	if( wan_udp.wan_udphdr_return_code == 0 ) {

		BANNER("CHAP STATISTICS");
   		chap_stats = (ppp_chap_stats_t*)&wan_udp.wan_udphdr_data[0];
		/*chap_stats = (ppp_chap_stats_t*)&wan_udp.wan_udphdr_u.data[0];*/

		printf("Packets discarded (unknown CHAP code): %u\n", chap_stats->rx_unknown);
		printf("Received CHAP packets too large: %u\n", chap_stats->rx_too_large);
		printf("Received packets invalid inbound PeerID: %u\n", chap_stats->rx_bad_peerid);
		printf("Received packets invalid inbound Password/Secret: %u\n", chap_stats->rx_bad_passwd);
		printf("Received packets invalid inbound MD5 message digest format: %u\n", chap_stats->rx_bad_md5);
		/*printf("Invalid inbound ID or out-of-order or unelicited responses: %u\n", chap_stats->);*/
		printf("\n\t\t\tReceived\tTransmitted\n");
		printf("Number of Challenge packets  : %u\t%u\n", chap_stats->rx_challenge, chap_stats->tx_challenge);
		printf("Number of Response packets   : %u\t%u\n", chap_stats->rx_response, chap_stats->tx_response);
		printf("Number of Success packets    : %u\t%u\n", chap_stats->rx_success, chap_stats->tx_success);
		printf("Number of Failure packets    : %u\t%u\n", chap_stats->rx_failure, chap_stats->tx_failure);
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

static void data_packet_stats( void ) 
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

static void flush_data_packet_stats(void)
{
	flush_lcp();
	flush_ipcp();
	flush_pap();
	flush_chap();

	wan_udp.wan_udphdr_command = PPP_FLUSH_PACKET_STATS;
	wan_udp.wan_udphdr_return_code = 0xaa;
	wan_udp.wan_udphdr_data_len = 0;
	DO_COMMAND(wan_udp);

	if( wan_udp.wan_udphdr_return_code != 0 ) {
		error();
	} 
};

int PPPUsage( void ) 
{
	printf("wanpipemon: Wanpipe PPP Debugging Utility\n\n");
	printf("Usage:\n");
	printf("-----\n\n");
	printf("wanpipemon -i <ip-address or interface name> -u <port> -c <command>\n\n");
	printf("\tOption -i: \n");
	printf("\t\tWanpipe remote IP address must be supplied\n");
	printf("\t\t<or> Wanpipe network interface name (ex: w1g1ppp)\n");   
	printf("\tOption -u: (Optional, default: 9000)\n");
	printf("\t\tWanpipe UDPPORT specified in /etc/wanpipe#.conf\n");
	printf("\tOption -full: (Optional, trace option)\n");
	printf("\t\tDisplay raw packets in full: default trace pkt len=25bytes\n");
	printf("\tOption -c: \n");
	printf("\t\tCommand is split into two parts:\n"); 
	printf("\t\t\tFirst letter is a command and the rest are options:\n"); 
	printf("\t\t\tex: xm = View Modem Status\n\n");
	printf("\tSupported Commands: x=status     : s=statistics : t=trace \n");
	printf("\t                    T=FT1 stats  : f=flush\n\n");
	printf("\tCommand:  Options:   Description \n");	
	printf("\t-------------------------------- \n\n");    
	printf("\tCard Status\n");
	printf("\t   x         m       Modem Status\n");
	printf("\t             n       PPP Link Status\n");
	printf("\t             ru      Display Router UP time\n");
	printf("\tCard Statistics\n");
	printf("\t   s         g       Global Statistics\n");
	printf("\t             c       Communication Error Statistics\n");
	printf("\t             p       General Packet Statistics\n");
	printf("\t             lcp     LCP Statistics\n");
	printf("\t             ipc     IP Control Protocol( IPCP )Statistics\n");
	//printf("\t             xcp     IPX Control Protocol( IPXCP )Statistics\n");
	printf("\t             pap     Password Authentication (PAP) Statistics\n");
	printf("\t             chp     Challenge-Handshake Auth.(CHAP) Statistics\n");
	printf("\tTrace Data\n");
	printf("\t   t         i       Trace and Interpret ALL frames\n");
	//printf("\t             ip      Trace and Interpret PROTOCOL frames only\n");
	//printf("\t             id      Trace and Interpret DATA frames only\n");
	printf("\t             r       Trace ALL frames, in RAW format\n");
	//printf("\t             rp      Trace PROTOCOL frames only, in RAW format\n");
	//printf("\t             rd      Trace DATA frames only, in RAW format\n");
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
	printf("\t             p       General Packet Statistics\n");
	printf("\t             lcp     LCP Statistics\n");
	printf("\t             ipc     IP Control Protocol( IPCP )Statistics\n");
	printf("\t             ipxc    IPX Control Protocol( IPXCP )Statistics\n");
	printf("\t             pap     Password Authentication (PAP) Statistics\n");
	printf("\t             chp     Challenge-Handshake Auth.(CHAP) Statistics\n");
	printf("\t             d       Flush Driver Statistics\n");
	printf("\t             pm      Flush T1/E1 performance monitoring counters\n");
	printf("\tDriver Statistics\n");
	printf("\t   d         s      Display Send Driver Statistics\n");
	printf("\t             i      Display Interrupt Driver Statistics\n");
	printf("\t             g      Display General Driver Statistics\n"); 
	printf("\n\tExamples:\n");
	printf("\t--------\n\n");
	printf("\tex: wanpipemon -i w1g1ppp -u 9000 -c xm    :View Modem Status \n");
	printf("\tex: wanpipemon -i 201.1.1.2 -u 9000 -c ti  :Trace and Interpret ALL frames\n\n");
	return 0;
}

static char *gui_main_menu[]={
"ppp_card_stats_menu","Card Status",
"ppp_stats_menu","Card Statistics",
"ppp_trace_menu","Trace Data",
"csudsu_menu","CSU DSU Config/Stats",
"ppp_flush_menu","Flush Statistics",
"ppp_driver_menu","Driver Statistics",
"."
};

static char *ppp_card_stats_menu[]={
"xm","Modem Status",
"xs","PPP Link Status",
"xru","Display Router UP time",
"."	
};

static char *ppp_stats_menu[]={
"sg","Global Statistics",
"sc","Communication Error Statistics",
"sp","Data Packet Statistics",
"slcp","LCP Statistics",
"sipc","IP Control Protocol( IPCP )Statistics",
//"sxcp","IPX Control Protocol( IPXCP )Statistics",
"spap","Password Authentication (PAP) Statistics",
"schp","Challenge-Handshake Auth.(CHAP) Statistics",
"."
};

static char *ppp_trace_menu[]={
"ti","Trace and Interpret ALL frames",
//"tip","Trace and Interpret PROTOCOL frames only",
//"tid","Trace and Interpret DATA frames only",
"tr","Trace ALL frames, in RAW format",
//"trp","Trace PROTOCOL frames only, in RAW format",
//"trd","Trace DATA frames only, in RAW format",
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
"fp","Flush General Packet Statistics",
"flcp","Flush LCP Statistics",
"fipc","Flush IP Control Protocol( IPCP )Statistics",
//"fxcp","Flush IPX Control Protocol( IPXCP )Statistics",
"fpap","Flush Password Authentication (PAP) Statistics",
"fchp","Flush Challenge-Handshake Auth.(CHAP) Statistics",
"fd","Flush Driver Statistics",
"fpm","Flush T1/E1 performance monitoring counters",
"."
};

static char *ppp_driver_menu[]={
"ds","Display Send Driver Statistics",
//"di","Display Interrupt Driver Statistics",
"dg","Display General Driver Statistics", 
"."
};

static struct cmd_menu_lookup_t gui_cmd_menu_lookup[]={
	{"ppp_card_stats_menu",ppp_card_stats_menu},
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
  hw_line_trace(trace_mode);
}

static void modem( void )
{
  hw_modem();
}

static void general_stats( void )
{
  hw_general_stats();
}

static void flush_general_stats( void )
{
  hw_flush_general_stats();
}

static void comm_err( void ) 
{
  hw_comm_err();
}

static void flush_comm_err( void ) 
{
  hw_flush_comm_err();
}

static void ppp_driver_stat_ifsend( void )
{
  ppp_driver_stat_gen();
}

static void ppp_driver_stat_gen( void )
{
  hw_general_stats();
}

static void flush_driver_stats( void )
{
  hw_flush_general_stats();
}

static void ppp_router_up_time( void )
{
  hw_router_up_time();
}

int PPPMain(char *command,int argc, char* argv[])
{
	char *opt=&command[1];
   	
	global_command = command;
	global_argc = argc;
	global_argv = argv;
	
	printf("\n");
	switch(command[0]) {
		
		case 'x':
			if (!strcmp(opt,"m")){
				modem();
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
			}else if (!strcmp(opt,"pap")){
				pap();
			}else if (!strcmp(opt,"chp")){
				chap();			
			}else if (!strcmp(opt,"ipc")){
				ipcp();
			/*
			}else if (!strcmp(opt,"xcp")){
				ipxcp();			
			*/
			}else if (!strcmp(opt,"lcp")){
				lcp();
			}else if (!strcmp(opt,"p")){
				data_packet_stats();
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
				flush_data_packet_stats();
				data_packet_stats();
			}else if (!strcmp(opt,"lcp" )){
				flush_lcp();
				lcp();
			}else if (!strcmp(opt,"ipc" )){
				flush_ipcp();
				ipcp();
			/*
			}else if (!strcmp(opt,"xcp" )){
				flush_ipxcp();
				ipxcp();
			*/
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
				printf("ERROR: Invalid Flush Command: '%s', Type wanpipemon <cr> for help\n\n", opt);
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
				read_te1_56k_stat(0);
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
