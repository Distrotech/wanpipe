/*****************************************************************************
* docommand.c	
*
* Author:       Alex Feldman <al.feldman@sangoma.com>	
*
* Copyright:	(c) 1995-1999 Sangoma Technologies Inc.
*
*		This program is free software; you can redistribute it and/or
*		modify it under the terms of the GNU General Public License
*		as published by the Free Software Foundation; either version
*		2 of the License, or (at your option) any later version.
* ----------------------------------------------------------------------------
* Dec 04, 2001	Alex Feldman	Initial version 
*****************************************************************************/

/******************************************************************************
 * 			INCLUDE FILES					      *
 *****************************************************************************/
#include <linux/version.h>
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
#include <linux/wanpipe.h>
#include <linux/sdla_chdlc.h>
#include <linux/if_packet.h>
#include <linux/if_wanpipe.h>
#include <linux/if_ether.h>
#include <errno.h>
#include "ft1_lib.h"

#if LINUX_VERSION_CODE >= 0x020100
#define LINUX_2_1
#endif

/******************************************************************************
 * 			DEFINES/MACROS					      *
 *****************************************************************************/
#define TIMEOUT 1
#define MDATALEN 2024

/******************************************************************************
 * 			TYPEDEF/STRUCTURE				      *
 *****************************************************************************/
#pragma pack(1)
typedef struct {
	ip_pkt_t 	ip_pkt		PACKED;
	udp_pkt_t	udp_pkt		PACKED;
	wp_mgmt_t	wp_mgmt		PACKED;
	char		data[MDATALEN]	PACKED;
} udp_packet_t;
#pragma pack()

/******************************************************************************
 * 			GLOBAL VARIABLES				      *
 *****************************************************************************/
struct sockaddr_in      soin;
char raw_sock=0;
char i_name[16];
extern char ipaddress[];
extern int  sock;
extern int udp_port;

udp_packet_t	udp_pkt;

/******************************************************************************
 * 			FUNCTION PROTOTYPES				      *
 *****************************************************************************/
unsigned char DoCommand(void*, int);



/******************************************************************************
 * 			FUNCTION DEFINITION				      *
 *****************************************************************************/
int MakeUdpConnection( void )
{
	sock = socket( PF_INET, SOCK_DGRAM, 0 );

	if( sock < 0 ) {
		printf("Error: Unable to open socket!\n");
		return( WAN_FALSE );
	} /* if */

	soin.sin_family = AF_INET;
	soin.sin_addr.s_addr = inet_addr(ipaddress);
     	
	if(soin.sin_addr.s_addr < 0){
                printf("Error: Invalid IP address!\n");
                return( WAN_FALSE );
        }
	
	soin.sin_port = htons((u_short)udp_port);

	if( !connect( sock, (struct sockaddr *)&soin, sizeof(soin)) == 0 ) {
		printf("Error: Cannot make connection!\n");
		printf("Make sure the IP address is correct\n");
		return( WAN_FALSE );
	} 

	return( WAN_TRUE );
}; /* MakeConnection */


#ifdef LINUX_2_1
int MakeRawConnection( void ) 
{
	struct ifreq ifr;
	struct sockaddr_ll soin;
	raw_sock=1;

   	sock = socket(PF_PACKET, SOCK_RAW, 0);
   	if (sock < 0){
      		printf("Error: Unable to open socket!\n");
      		return( WAN_FALSE );
   	} /* if */
   	
	soin.sll_family = AF_PACKET;
	soin.sll_protocol = htons(ETH_P_IP);
	strlcpy (ifr.ifr_name,i_name,WAN_IFNAME_SZ);
   	if (ioctl(sock,SIOCGIFINDEX,&ifr) <0){
		perror("Ioctl: ");
		return (WAN_FALSE);
	}
	soin.sll_ifindex = ifr.ifr_ifindex;

	if (bind(sock, (struct sockaddr *)&soin, sizeof(soin)) < 0) {
      		printf("Error: Cannot make connection!\n");
		printf("Make sure the IP address is correct\n");
      		return( WAN_FALSE );
   	} /* if */

   	return( WAN_TRUE );

}; /* MakeRawConnection */
#endif

unsigned char DoCommand(void* pkt, int data_len)
{
	wp_mgmt_t* packet = (wp_mgmt_t*)pkt;
        static unsigned char id = 0;
        fd_set ready;
        struct timeval to;
        int x, err=0;

	if (raw_sock){
		struct ifreq ifr;

		udp_pkt.ip_pkt.ver_inet_hdr_length = 0x45;
		udp_pkt.ip_pkt.service_type = 0;
		udp_pkt.ip_pkt.total_length = sizeof(ip_pkt_t)+
					      sizeof(udp_pkt_t) + data_len;
		udp_pkt.ip_pkt.identifier = 0;
      		udp_pkt.ip_pkt.flags_frag_offset = 0;
		udp_pkt.ip_pkt.ttl = 0x7F;
 		udp_pkt.ip_pkt.protocol = 0x11;
		udp_pkt.ip_pkt.hdr_checksum = 0x1232;
		udp_pkt.ip_pkt.ip_src_address = inet_addr("1.1.1.1");
		udp_pkt.ip_pkt.ip_dst_address = inet_addr("1.1.1.2"); 

		udp_pkt.udp_pkt.udp_src_port = htons((u_short)udp_port);
		udp_pkt.udp_pkt.udp_dst_port = htons((u_short)udp_port); 
		udp_pkt.udp_pkt.udp_length   = sizeof(udp_pkt_t) + data_len;
		udp_pkt.udp_pkt.udp_checksum = 0x1234;

		memcpy(&udp_pkt.wp_mgmt.signature[0], packet, data_len); 

		for( x = 0; x < 4; x += 1 ) {
			udp_pkt.wp_mgmt.request_reply = 0x01;
			udp_pkt.wp_mgmt.id=id;

			ifr.ifr_data = (void*)&udp_pkt;
			strlcpy (ifr.ifr_name,i_name,WAN_IFNAME_SZ);
			ioctl(sock,SIOC_WANPIPE_PIPEMON,&ifr);
			
			if (err < 0){
				perror("Ioctl:");	
				if (errno == -EBUSY){
					continue;
				}else{
					break;
				}	
			}
				
			memcpy(packet,
			       &udp_pkt.wp_mgmt.signature, 
			       MDATALEN-sizeof(ip_pkt_t)-sizeof(udp_pkt_t)); 
                        break;
		}
	}else{	
		for( x = 0; x < 4; x += 1 ) {
			packet->request_reply = 0x01;
			packet->id = id;
			/* 0xAA is our special return code indicating packet timeout */
			//cb.cblock.return_code = 0xaa;
			//err = send(sock, &cb, 35 + cb.cblock.buffer_length, 0);
			err = send(sock, packet, data_len, 0); 
			if (err <0){
				perror("Send");	
				continue;
			}
				
			FD_ZERO(&ready);
			FD_SET(sock,&ready);
			to.tv_sec = 5;
			to.tv_usec = 0;

			if((err = select(sock + 1,&ready, NULL, NULL, &to))) {
				
				err = recv(sock, packet, MDATALEN, 0);
				if( err < 0 ){
					perror("Recv");
				}
                        	break;
                	}else{
				printf("Error: No Data received from the connected socket.\n");
                	}
		}
        }

        /* make sure the id is correct (returning results from a previous
           call may be disastrous if not recognized)
           also make sure it is a reply
        */

        //if (cb.wp_mgmt.id != id || cb.wp_mgmt.request_reply != 0x02){
        if (packet->id != id || packet->request_reply != 0x02){
	       //cb.cblock.return_code = 0xbb;
	       // FIXME: packet->cblock.return_code = 0xbb;
	}
        id++;
        //if (cb.cblock.return_code == 0xCC) {
        //        printf("Error: Code is not running on the board!");
        //        exit(1);
        //}

        //return(cb.cblock.return_code);
        return 0;

}; /* DoCommand */

