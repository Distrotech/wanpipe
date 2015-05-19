/*****************************************************************************
* mppipemon.c	Multi-Port PPP Monitor
*
* Author:       Nenad Corbic <ncorbic@sangoma.com>	
*
* Copyright:	(c) 1995-2000 Sangoma Technologies Inc.
*
*		This program is free software; you can redistribute it and/or
*		modify it under the terms of the GNU General Public License
*		as published by the Free Software Foundation; either version
*		2 of the License, or (at your option) any later version.
* ----------------------------------------------------------------------------
* Jul 17, 2000  Nenad Corbic	Exactly the same as RAW cpipemon
*****************************************************************************/

#if !defined(__LINUX__)
# error "Multi-Port PPP Monitor not supported on FreeBSD/OpenBSD OS!"
#endif

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
#include "ft1_lib.h"

#if LINUX_VERSION_CODE >= 0x020100
#define LINUX_2_1
#endif

#define TIMEOUT 1
#define MDATALEN 2024

/* Structures for data casting */
#pragma pack(1)
typedef struct {
        unsigned char link_address;     /* Broadcast or Unicast */
        unsigned char control;
        unsigned short packet_type;     /* IP/SLARP/CDP */
} cisco_header_t;
#pragma pack()

#pragma pack(1)
typedef struct {
	wp_mgmt_t		wp_mgmt		PACKED;
	cblock_t                cblock          PACKED;
	trace_info_t       	trace_info      PACKED;
	unsigned char           data[SIZEOF_MB_DATA_BFR]      PACKED;
} udp_mgmt_pkt_t;
#pragma pack()

#pragma pack(1)
typedef struct {
	ip_pkt_t 	ip_pkt;
	udp_pkt_t	udp_pkt;
	udp_mgmt_pkt_t	cb;
}udp_packet;
#pragma pack()

//#define CB_SIZE sizeof(wp_mgmt_t)+sizeof(cblock_t)+sizeof(trace_info_t)
#define CB_SIZE sizeof(wp_mgmt_t)+sizeof(cblock_t)+sizeof(trace_info_t)+1

/* Link addresses */
#define CISCO_ADDR_UNICAST      0x0F
#define CISCO_ADDR_BCAST        0x8F

/* Packet Types */
#define CISCO_PACKET_IP         0x0800
#define CISCO_PACKET_SLARP      0x8035
#define CISCO_PACKET_CDP        0x2000

#pragma pack(1)
typedef struct {
        unsigned long code;     /* Slarp Control Packet Code */
        union {
                struct {
                        unsigned long address;  /* IP address */
                        unsigned long mask;     /* IP mask    */
                        unsigned short reserved[3];
                } address;
                struct {
                        unsigned long my_sequence;
                        unsigned long your_sequence;
                        unsigned short reliability;
                        unsigned short t1;      /* time alive (upper) */
                        unsigned short t2;      /* time alive (lower) */
                } keepalive;
        } un;
} cisco_slarp_t;
#pragma pack()
/* Packet Codes */

#define SLARP_REQUEST   0
#define SLARP_REPLY     1
#define SLARP_KEEPALIVE 2

/* storage for FT1 LEDs */
FT1_LED_STATUS FT1_LED;

/* Prototypes */

int    main(int, char** );
void    init(int, char** );
void    usage( void );
#ifdef LINUX_2_1
int 	MakeRawConnection( void );
#endif
int     ObtainConfiguration( void );
unsigned char DoCommand( void );
void    error( void );

/* Command routines */
void    modem( void );
void    global_stats( void );
void    comm_stats( void );
void    read_code_version( void );
void    chdlc_configuration( void );
void    link_status( void );
void    operational_stats( void );
void    slarp_stats( void );
void    line_trace( int );
void    chdlc_router_up_time( void );
void    flush_operational_stats( void );
void    flush_global_stats( void );
void    flush_comm_stats( void );
void    get_ip_addr(char *if_name);
void	banner(char *);
void 	set_FT1_monitor_status( unsigned char);
void 	set_FT1_mode(int verbose);
void 	read_FT1_status( void );
void 	read_ft1_op_stats( void );


/* Global variables */
int                     sock = 0;
struct sockaddr_in      soin;
unsigned int loop_counter, frame_count;
int is_508, raw_data=WAN_FALSE, udp_port = 9000;
char ipaddress[16];
char src_ip[16];
char i_name[16];
char cmd[5];
char raw_sock=0;

/* The ft1_lib needs these global variables */
unsigned char par_port_A_byte;
unsigned char par_port_B_byte;
int gfail;
udp_mgmt_pkt_t cb;


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
	strcpy (ifr.ifr_name,i_name);
   	ioctl(sock,SIOCGIFINDEX,&ifr);
	soin.sll_ifindex = ifr.ifr_ifindex;


	if (bind(sock, (struct sockaddr *)&soin, sizeof(soin)) < 0) {
      		printf("Error: Cannot make connection!\nMake sure the IP address is correct\n");
      		return( WAN_FALSE );
   	} /* if */

   	if (!ObtainConfiguration()) {
      		printf("Error: Unable to obtain CHDLC information.\n");
                printf("Make sure the IP and UDP port are correct.\n");
      		close( sock );
      		return( WAN_FALSE );
   	} /* if */   

   	return( WAN_TRUE );

}; /* MakeRawConnection */
#endif

int ObtainConfiguration( void )
{
	unsigned char codeversion[10];
	unsigned char x;
   
	x = 0;
	cb.cblock.command= READ_CHDLC_CONFIGURATION;
	cb.cblock.buffer_length = 0;

	while (DoCommand() != 0 && ++x < 4) {
		if (cb.cblock.return_code == 0xaa) {
			printf("Error: Command timeout occurred"); 
			return(WAN_FALSE);
		}

		if (cb.cblock.return_code == 0xCC ) return(WAN_FALSE);
	}

	if (x >= 4) return(WAN_FALSE);

	if( cb.cblock.buffer_length == sizeof(CHDLC_CONFIGURATION_STRUCT)) {
		is_508 = WAN_TRUE;
	} else {
		is_508 = WAN_FALSE;
	} 
   
	strcpy(codeversion, "?.??");
   
	cb.cblock.command= READ_CHDLC_CODE_VERSION;
	cb.cblock.buffer_length = 0;
	DoCommand();
	if (cb.cblock.return_code == 0) {
		cb.data[cb.cblock.buffer_length] = 0;
		strcpy(codeversion, cb.data);
	}

	return(WAN_TRUE);
}; /* ObtainConfiguration */


unsigned char DoCommand( void )
{
        static unsigned char id = 0;
        fd_set ready;
        struct timeval to;
        int x, err=0;

	if (raw_sock){
		udp_packet udp_pkt;
		memset(&udp_pkt,0,sizeof(udp_pkt));

		udp_pkt.ip_pkt.ver_inet_hdr_length = 0x45;
		udp_pkt.ip_pkt.service_type = 0;
		udp_pkt.ip_pkt.total_length = sizeof(ip_pkt_t)+
					      sizeof(udp_pkt_t)+CB_SIZE+cb.cblock.buffer_length;
		udp_pkt.ip_pkt.identifier = 0;
      		udp_pkt.ip_pkt.flags_frag_offset = 0;
		udp_pkt.ip_pkt.ttl = 0x7F;
 		udp_pkt.ip_pkt.protocol = 0x11;
		udp_pkt.ip_pkt.hdr_checksum = 0x1232;
		udp_pkt.ip_pkt.ip_src_address = inet_addr("1.1.1.1");
		udp_pkt.ip_pkt.ip_dst_address = inet_addr("1.1.1.2"); 

		udp_pkt.udp_pkt.udp_src_port = htons((u_short)udp_port);
		udp_pkt.udp_pkt.udp_dst_port = htons((u_short)udp_port); 
		udp_pkt.udp_pkt.udp_length   = sizeof(udp_pkt_t)+CB_SIZE+cb.cblock.buffer_length;
		udp_pkt.udp_pkt.udp_checksum = 0x1234;
	
		memcpy(&udp_pkt.cb, &cb, CB_SIZE); 
		if (cb.cblock.buffer_length){
			memcpy(&udp_pkt.cb.data, &cb.data, cb.cblock.buffer_length); 
		}
		
		for( x = 0; x < 4; x += 1 ) {
			udp_pkt.cb.cblock.opp_flag = 0;
			udp_pkt.cb.wp_mgmt.request_reply = 0x01;
			udp_pkt.cb.wp_mgmt.id=id;
			udp_pkt.cb.cblock.return_code = 0xaa;
			send(sock, &udp_pkt, udp_pkt.ip_pkt.total_length,0);
		
			if (err <0){
				perror("Send");	
				continue;
			}
				
			FD_ZERO(&ready);
			FD_SET(sock,&ready);
			to.tv_sec = 5;
			to.tv_usec = 0;

			if((err = select(sock + 1,&ready, NULL, NULL, &to))) {
				err = recv(sock, &udp_pkt, 
				      (sizeof(ip_pkt_t)+sizeof(udp_pkt_t)+CB_SIZE+MDATALEN),0);
				
				if (err >= (CB_SIZE+udp_pkt.cb.cblock.buffer_length)){
				
					memcpy(&cb,&udp_pkt.cb.wp_mgmt.signature,
				       		(CB_SIZE+udp_pkt.cb.cblock.buffer_length));
				}
                        	break;
                	}else{
				printf("Error: No Data received from the connected socket.\n");
                	}
		}

	}else{	

		for( x = 0; x < 4; x += 1 ) {
			cb.cblock.opp_flag = 0;
			cb.wp_mgmt.request_reply = 0x01;
			cb.wp_mgmt.id = id;
			/* 0xAA is our special return code indicating packet timeout */
			cb.cblock.return_code = 0xaa;
			err = send(sock, &cb, 35 + cb.cblock.buffer_length, 0);
			if (err <0){
				perror("Send");	
				continue;
			}
				
			FD_ZERO(&ready);
			FD_SET(sock,&ready);
			to.tv_sec = 5;
			to.tv_usec = 0;

			if((err = select(sock + 1,&ready, NULL, NULL, &to))) {
				
				err = recv(sock, &cb, 35+MDATALEN,0);
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

        if (cb.wp_mgmt.request_reply != 0x02){
	       cb.cblock.return_code = 0xbb;
	}
        id++;
        if (cb.cblock.return_code == 0xCC) {
                printf("Error: Code is not running on the board!");
                exit(1);
        }

        return(cb.cblock.return_code);

}; /* DoCommand */


void init( int argc, char *argv[])
{
	int i, i_cnt=0, u_cnt=0, c_cnt=0, if_found=0;
	struct in_addr *ip_str=NULL;
	cb.wp_mgmt.id = 0;

	for (i=0;i<argc;i++){
		if (!strcmp(argv[i],"-i")){

			if (i+1 > argc-1){
				printf("ERROR: Invalid IP or Interface Name, Type mppipemon <cr> for help\n\n");
				exit(0);
			}

			strcpy(ipaddress,argv[i+1]);
			if (inet_aton(ipaddress,ip_str) != 0 ){
			}else{
				strcpy(i_name,ipaddress);
				if_found=1;
			}
			i_cnt=1;
		}else if (!strcmp(argv[i],"-u")){

			if (i+1 > argc-1){
				printf("ERROR: Invalid UDP PORT, Type mppipemon <cr> for help\n\n");
				exit(0);
			}

		 	if(isdigit(argv[i+1][0])){
				udp_port = atoi(argv[i+1]);
			}else{
				printf("ERROR: Invalid UDP Port, Type mppipemon <cr> for help\n\n");
				exit(0);
			}
			u_cnt=1;
		}else if (!strcmp(argv[i],"-c")){

			if (i+1 > argc-1){
				printf("ERROR: Invalid Command, Type mppipemon <cr> for help\n\n");
				exit(0);
			}

			strcpy(cmd,argv[i+1]);
			c_cnt=1;
		}
	}

	if (!i_cnt){
		printf("ERROR: No IP address or Interface Name, Type mppipemon <cr> for help\n\n");
		exit(0);
	}
	if (!c_cnt){
		printf("ERROR: No Command, Type mppipemon <cr> for help\n\n");
		exit(0);
	}

	if (if_found){
		get_ip_addr(ipaddress);
	}

	/* signature for UDP Management */
	strcpy(cb.wp_mgmt.signature, "CTPIPEAB");
};

void get_ip_addr(char *if_name){

	int skfd;
	struct sockaddr_in *sin;
	struct ifreq ifr;

    	if ((skfd = socket(AF_INET, SOCK_STREAM, IPPROTO_IP)) < 0) {
		printf("SOCKET FAILED\n");
		perror("socket");
		exit(1);
    	}

	strncpy(ifr.ifr_ifrn.ifrn_name, if_name, sizeof(ifr.ifr_ifrn.ifrn_name));

        if (ioctl(skfd, SIOCGIFFLAGS , &ifr) < 0) {
		fprintf(stderr, "Error: Unknown interface: %s\n",if_name);
		exit(0);
    	}

        if (ioctl(skfd, SIOCGIFDSTADDR , &ifr) < 0) {

		strcpy(ipaddress,"0.0.0.0");
	}else{
		sin = (struct sockaddr_in *)&ifr.ifr_ifru.ifru_dstaddr;
		strcpy(ipaddress,inet_ntoa(sin->sin_addr));
	}

	if (ioctl(skfd, SIOCGIFADDR, &ifr) < 0) {
		
		strcpy(src_ip,"0.0.0.0");
	}else{
		sin = (struct sockaddr_in *)&ifr.ifr_ifru.ifru_addr;
		strcpy(src_ip,inet_ntoa(sin->sin_addr));
	}	
 

	close (skfd);

	return;

} 


void error( void ) 
{

	printf("Error: Command failed!\n");

}; /* error */


void global_stats (void)
{
	GLOBAL_STATS_STRUCT* global_stats;
	cb.cblock.command= READ_GLOBAL_STATISTICS; 
	cb.cblock.buffer_length = 0;
	DoCommand();

	if( cb.cblock.return_code == 0 ) {
		banner("GLOBAL STATISTICS");
		global_stats = (GLOBAL_STATS_STRUCT *)&cb.data[0];
		printf("Times application did not respond to IRQ: %d",
			global_stats->app_IRQ_timeout_count);
	} else {
		error();
	}
};  /* global stats */

void flush_global_stats (void)
{
	cb.cblock.command= FLUSH_GLOBAL_STATISTICS; 
	cb.cblock.buffer_length = 0;
	DoCommand();

	if( cb.cblock.return_code != 0 ) error();
};  /* flush global stats */

void modem( void )
{
	unsigned char cts_dcd;
   
	cb.cblock.command= READ_MODEM_STATUS; 
	cb.cblock.buffer_length = 0;
	DoCommand();

	if( cb.cblock.return_code == 0 ) {
		banner("MODEM STATUS");
		memcpy(&cts_dcd,&cb.data[0],1);
		printf("DCD: ");
		(cts_dcd & 0x08) ? printf("High\n") : printf("Low\n");
		printf("CTS: ");
		(cts_dcd & 0x20) ? printf("High\n") : printf("Low\n");
	} else {
		error();
	}
}; /* modem */


void comm_stats (void)
{
	COMMS_ERROR_STATS_STRUCT* comm_stats;
	cb.cblock.command= READ_COMMS_ERROR_STATS; 
	cb.cblock.buffer_length = 0;
	DoCommand();

	if( cb.cblock.return_code == 0 ) {
	 	banner("COMMUNICATION ERROR STATISTICS");

		comm_stats = (COMMS_ERROR_STATS_STRUCT *)&cb.data[0];

		printf("                        Number of receiver overrun errors:  %d\n", comm_stats->Rx_overrun_err_count);
		printf("                            Number of receiver CRC errors:  %d\n", comm_stats->CRC_err_count);
		printf("                          Number of abort frames received:  %d\n", comm_stats->Rx_abort_count);
		printf("                        Number of times receiver disabled:  %d\n", comm_stats->Rx_dis_pri_bfrs_full_count);
		printf(" Number of transmitted abort frames (missed Tx interrupt):  %d\n", comm_stats->sec_Tx_abort_msd_Tx_int_count);
		printf("                             Number of transmit underruns:  %d\n", comm_stats->missed_Tx_und_int_count);
		printf("                       Number of abort frames transmitted:  %d\n", comm_stats->sec_Tx_abort_count);
		printf("                        Number of times DCD changed state:  %d\n", comm_stats->DCD_state_change_count);
		printf("                        Number of times CTS changed state:  %d\n", comm_stats->CTS_state_change_count);
	} else {
		error();
	}
}; /* comm_stats */


void flush_comm_stats( void ) 
{
	cb.cblock.command= FLUSH_COMMS_ERROR_STATS;
	cb.cblock.buffer_length = 0;
	DoCommand();

	if( cb.cblock.return_code != 0 ) error();
}; /* flush_comm_stats */


void read_code_version (void)
{
	cb.cblock.command= READ_CHDLC_CODE_VERSION; 
	cb.cblock.buffer_length = 0;
	DoCommand();

	if (cb.cblock.return_code == 0) {
		unsigned char version[10];
		banner("CODE VERSION");
		memcpy(version,&cb.data,cb.cblock.buffer_length);
		version[cb.cblock.buffer_length] = '\0';
		printf("Cisco HDLC Code version: %s\n", version);
	}
}; /* read code version */

void chdlc_configuration (void)
{
	CHDLC_CONFIGURATION_STRUCT *chdlc_cfg;
	cb.cblock.command = READ_CHDLC_CONFIGURATION; 
	cb.cblock.buffer_length = 0;
	DoCommand();

	if (cb.cblock.return_code == 0) {
		chdlc_cfg = (CHDLC_CONFIGURATION_STRUCT *)&cb.data[0];
		banner("CHDLC CONFIGURATION");

		printf("Baud rate: %ld", chdlc_cfg->baud_rate);

		printf("\nLine configuration: ");
		     switch (chdlc_cfg->line_config_options) {
			case INTERFACE_LEVEL_V35:
				printf("V.35");
				break;
			case INTERFACE_LEVEL_RS232:
				printf("RS-232");
				break;
		     }

		printf("\n\nLink State depends on:");
		     if (chdlc_cfg->CHDLC_protocol_options & IGNORE_DCD_FOR_LINK_STAT)
			     printf ("\n\t\t\tDCD:  NO");
		     else
			     printf ("\n\t\t\tDCD:  YES");
		     
		     if (chdlc_cfg->CHDLC_protocol_options & IGNORE_CTS_FOR_LINK_STAT)
			     printf ("\n\t\t\tCTS:  NO\n");
		     else	
			     printf ("\n\t\t\tCTS:  YES\n");
		     
		     if (chdlc_cfg->CHDLC_protocol_options & IGNORE_KPALV_FOR_LINK_STAT)
			     printf ("\tKeepalive Reception:  NO\n");
		     else
			     printf ("\tKeepalive Reception:  YES\n");

		printf("\n\n                 Maximum data field length:  %d",
			chdlc_cfg->max_CHDLC_data_field_length);

		printf(  "\n          Keepalive transmit interval (ms):  %d",
			chdlc_cfg->transmit_keepalive_timer);

		printf(  "\nExpected keepalive reception interval (ms):  %d",
			chdlc_cfg->receive_keepalive_timer);
		
		printf(  "\n       Keepalive reception error tolerance:  %d",
			chdlc_cfg->keepalive_error_tolerance);

		printf(  "\n      SLARP request transmit interval (ms):  %d",
			chdlc_cfg->SLARP_request_timer);
		
	} else {
		error();
	}
}; /* chdlc_configuration */


void link_status (void)
{
	CHDLC_LINK_STATUS_STRUCT *status;
	cb.cblock.command= READ_CHDLC_LINK_STATUS; 
	cb.cblock.buffer_length = 0;
	DoCommand();

	if (cb.cblock.return_code == 0) {
		
		banner("LINK STATUS");	
	
		status = (CHDLC_LINK_STATUS_STRUCT *)&cb.data[0];

		printf("                          Link status:  ");
		     switch (status->CHDLC_link_status) {
			case CHDLC_LINK_INACTIVE:
				printf("Inactive\n");
				break;
			case CHDLC_LINK_ACTIVE:
				printf("Active\n");
				break;
		     }

		printf("Available data frames for application:  %d\n",
			status->no_Data_frms_for_app);

		printf("                      Receiver status:  ");
		     if (status->receiver_status & 0x01)
			printf("Disabled due to flow control restrictions\n");
		     else printf("Enabled\n");
	} else {
		error();
	}
}; /* Link Status */		


void operational_stats (void)
{
	CHDLC_OPERATIONAL_STATS_STRUCT *stats;
	cb.cblock.command= READ_CHDLC_OPERATIONAL_STATS; 
	cb.cblock.buffer_length = 0;
	DoCommand();

	if (cb.cblock.return_code == 0) {
		banner("OPERATIONAL STATISTICS");
		stats = (CHDLC_OPERATIONAL_STATS_STRUCT *)&cb.data[0];
 
		printf(    "             Number of frames transmitted:   %ld",stats->Data_frames_Tx_count);
		printf(  "\n              Number of bytes transmitted:   %ld",stats->Data_bytes_Tx_count);
		printf(  "\n                      Transmit Throughput:   %ld",stats->Data_Tx_throughput);
		printf(  "\n Transmit frames discarded (length error):   %ld",stats->Tx_Data_discard_lgth_err_count);

		printf("\n\n                Number of frames received:   %ld",stats->Data_frames_Rx_count);
		printf(  "\n                 Number of bytes received:   %ld",stats->Data_bytes_Rx_count);
		printf(  "\n                       Receive Throughput:   %ld",stats->Data_Rx_throughput);
		printf(  "\n    Received frames discarded (too short):   %ld",stats->Rx_Data_discard_short_count);
		printf(  "\n     Received frames discarded (too long):   %ld",stats->Rx_Data_discard_long_count);
		printf(  "\nReceived frames discarded (link inactive):   %ld",stats->Rx_Data_discard_inactive_count);


		printf("\n\nIncoming frames with format errors");
		printf(  "\n               Frames of excessive length:   %d", stats->Rx_frms_too_long_count);	
		printf(  "\n                  Incomplete Cisco Header:   %d", stats->Rx_frm_incomp_CHDLC_hdr_count);
		printf(  "\n          Invalid Cisco HDLC link address:   %d", stats->Rx_invalid_CHDLC_addr_count);
		printf(  "\n               Invalid Cisco HDLC control:   %d", stats->Rx_invalid_CHDLC_ctrl_count);
		printf(  "\n            Invalid Cisco HDLC frame type:   %d", stats->Rx_invalid_CHDLC_type_count);

		printf("\n\nCisco HDLC link active/inactive and loopback statistics");
		printf(  "\n                           Times that the link went active:   %d", stats->link_active_count);	
		printf(  "\n         Times that the link went inactive (modem failure):   %d", stats->link_inactive_modem_count);
		printf(  "\n     Times that the link went inactive (keepalive failure):   %d", stats->link_inactive_keepalive_count);
		printf(  "\n                                         link looped count:   %d", stats->link_looped_count);
	} else {
		error();
	}
} /* Operational_stats */


void flush_operational_stats( void ) 
{
	cb.cblock.command= FLUSH_CHDLC_OPERATIONAL_STATS;
	cb.cblock.buffer_length = 0;
	DoCommand();

	if( cb.cblock.return_code != 0 ) {
		error();
	}

}; /* flush_operational_stats */

void slarp_stats (void)
{
	CHDLC_OPERATIONAL_STATS_STRUCT *stats;
	cb.cblock.command= READ_CHDLC_OPERATIONAL_STATS; 
	cb.cblock.buffer_length = 0;
	DoCommand();

	if (cb.cblock.return_code == 0) {
		banner("SLARP STATISTICS");
		stats = (CHDLC_OPERATIONAL_STATS_STRUCT *)&cb.data[0];

		printf("\n SLARP frame transmission/reception statistics");
		printf(  "\n       SLARP request packets transmitted:   %ld",
 stats->CHDLC_SLARP_REQ_Tx_count);
		printf(  "\n          SLARP request packets received:   %ld",
 stats->CHDLC_SLARP_REQ_Rx_count);
		printf("\n\n         SLARP Reply packets transmitted:   %ld",
 stats->CHDLC_SLARP_REPLY_Tx_count);
		printf(  "\n            SLARP Reply packets received:   %ld",
 stats->CHDLC_SLARP_REPLY_Rx_count);
		printf("\n\n     SLARP keepalive packets transmitted:   %ld",
 stats->CHDLC_SLARP_KPALV_Tx_count);
		printf(  "\n        SLARP keepalive packets received:   %ld",
 stats->CHDLC_SLARP_KPALV_Rx_count);

		printf(  "\n\nIncoming SLARP Packets with format errors");
		printf(  "\n                      Invalid SLARP Code:   %d", stats->Rx_SLARP_invalid_code_count);
		printf(  "\n                Replies with bad IP addr:   %d", stats->Rx_SLARP_Reply_bad_IP_addr);
		printf(  "\n                Replies with bad netmask:   %d",stats->Rx_SLARP_Reply_bad_netmask);
		printf(  "\n\nSLARP timeout/retry statistics");
		printf(  "\n                  SLARP Request timeouts:   %d",stats->SLARP_Request_TO_count);
		printf(  "\n            keepalive reception timeouts:   %d",stats->SLARP_Rx_keepalive_TO_count);

		printf("\n\nCisco Discovery Protocol frames");
		printf(  "\n                             Transmitted:   %ld", stats->CHDLC_CDP_Tx_count);
		printf(  "\n                                Received:   %ld", stats->CHDLC_CDP_Rx_count);

	} else {
		error();
	}

} /* slarp_stats */


#define MAX_OUTSTR_LEN	2000
void line_trace(int trace_mode) 
{
	unsigned char num_frames, num_chars;
	unsigned short curr_pos = 0;
	trace_pkt_t *trace_pkt;
	unsigned int i, j;
	unsigned char outstr[MAX_OUTSTR_LEN];
	int recv_buff = sizeof(udp_mgmt_pkt_t ) + MDATALEN + 100;
	fd_set ready;
	struct timeval to;
	cisco_header_t *cisco_header;
 
	setsockopt( sock, SOL_SOCKET, SO_RCVBUF, &recv_buff, sizeof(int) );

	/* Disable trace to ensure that the buffers are flushed */
	cb.cblock.command= CPIPE_DISABLE_TRACING;
	cb.cblock.buffer_length = 0;
	DoCommand();

	cb.cblock.command= CPIPE_ENABLE_TRACING;
	cb.cblock.buffer_length = 0;
	cb.data[0]=0;
	if(trace_mode == TRACE_PROT){
		cb.cblock.buffer_length = 1;
		cb.data[0] |= TRACE_SLARP_FRAMES | TRACE_CDP_FRAMES; 
		printf("Tracing Protocol only %X\n",cb.data[0]);
	}else if(trace_mode == TRACE_DATA){
		cb.cblock.buffer_length = 1;
		cb.data[0] = TRACE_DATA_FRAMES; 
	}else{
		cb.cblock.buffer_length = 1;
		cb.data[0] |= TRACE_SLARP_FRAMES | 
			      TRACE_DATA_FRAMES | 
			       TRACE_CDP_FRAMES;
	}
	DoCommand();

	if( cb.cblock.return_code == 0 ) { 
		printf("Starting trace...(Press ENTER to exit)\n");
		fflush(stdout);
	} else if( cb.cblock.return_code == 0xCD ) {
		printf("Cannot Enable Line Tracing from Underneath.\n");
		fflush(stdout);
		return;
	} else if( cb.cblock.return_code == 0x01 ) {
		printf("Starting trace...(although it's already enabled!)\n");
		printf("Press ENTER to exit.\n");
		fflush(stdout);
	} else {
		printf("Failed to Enable Line Tracing. Return code: 0x%02X\n", 
			cb.cblock.return_code );
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

		cb.cblock.command = CPIPE_GET_TRACE_INFO;
		cb.cblock.buffer_length = 0;
		DoCommand();
		if (cb.cblock.return_code == 0 && cb.cblock.buffer_length) { 
		     num_frames = cb.trace_info.num_frames;
		     for ( i = 0; i < num_frames; i++) {
			trace_pkt= (trace_pkt_t *)(&cb.data[0] + curr_pos);

			/*  frame type */
			if (trace_pkt->status & 0x01) {
				sprintf(outstr,"OUTGOING\t");
			} else {
				if (trace_pkt->status & 0x10) { 
					sprintf(outstr,"INCOMING - Aborted\t");
				} else if (trace_pkt->status & 0x20) {
					sprintf(outstr,"INCOMING - CRC Error\t");
				} else if (trace_pkt->status & 0x40) {
					sprintf(outstr,"INCOMING - Overrun Error\t");
				} else {
					sprintf(outstr,"INCOMING\t");
				}
			}

			/* real length and time stamp */
			sprintf(outstr+strlen(outstr), "%d\t%d\t", 
			     trace_pkt->real_length, trace_pkt->time_stamp);

			/* first update curr_pos */
			curr_pos += sizeof(trace_pkt_t);
			if (trace_pkt->data_avail == 0) {
				sprintf( outstr+strlen(outstr), "the frame data is not available" );
			} else {
				/* update curr_pos again */
				curr_pos += trace_pkt->real_length - 1;
				num_chars = (unsigned char)
					((trace_pkt->real_length <= 25)
					? trace_pkt->real_length : 25);

				if (raw_data) { /*  show raw data */
				     for( j=0; j<num_chars; j++ ) {
					sprintf(outstr+strlen(outstr), "%02X ", (unsigned char)trace_pkt->data[j]);
				     }
				     outstr[strlen(outstr)-1] = '\0';
				} else { /* show int data */
				     cisco_header = (cisco_header_t *)
							&trace_pkt->data[0];
				     switch(ntohs(cisco_header->packet_type)) {

					case CISCO_PACKET_IP:
					     sprintf(outstr+strlen(outstr),
						"IP packet from %ld.%ld.%ld.%ld to %ld.%ld.%ld.%ld",
	((ip_pkt_t *)(cisco_header + 1))->ip_src_address & 0x000000FF,
	(((ip_pkt_t *)(cisco_header + 1))->ip_src_address & 0x0000FF00) >> 8,
	(((ip_pkt_t *)(cisco_header + 1))->ip_src_address & 0x00FF0000) >>16,
	(((ip_pkt_t *)(cisco_header + 1))->ip_src_address & 0xFF000000) >>24,

	((ip_pkt_t *)(cisco_header + 1))->ip_dst_address & 0x000000FF,
	(((ip_pkt_t *)(cisco_header + 1))->ip_dst_address & 0x0000FF00) >> 8,
	(((ip_pkt_t *)(cisco_header + 1))->ip_dst_address & 0x00FF0000) >>16,
	(((ip_pkt_t *)(cisco_header + 1))->ip_dst_address & 0xFF000000) >>24
);
					     break; 

					case CISCO_PACKET_SLARP: { 
					     cisco_slarp_t *cisco_slarp =
					       (cisco_slarp_t*)(cisco_header+1);

					     switch(ntohl(cisco_slarp->code)) {
						case SLARP_REQUEST:
					     	  sprintf(outstr+strlen(outstr),
						     "SLARP Request packet");
						  break;

						case SLARP_REPLY:
					     	  sprintf(outstr+strlen(outstr),
						     "SLARP Reply packet");
						  break;

						case SLARP_KEEPALIVE:
					     	  sprintf(outstr+strlen(outstr),
						     "SLARP Keepalive packet");
						  break;

						default:
					     	  sprintf(outstr+strlen(outstr),
						   "Unrecognized SLARP packet");
						  break;
					     } /* Slarp code switch */
					     break;
					} /* slarp case */

					case CISCO_PACKET_CDP: 
					     sprintf(outstr+strlen(outstr),
					     "Cisco Discover Protocol packet");
					     break;

					default:
						sprintf(outstr+strlen(outstr),"Unknown type");
				     } /* Switch packet type */
				} //if
			     } //if
 
			     printf("%s\n",outstr);
			     fflush(stdout);
			} //for
		} //if
		curr_pos = 0;

		if (!cb.trace_info.ismoredata){
			to.tv_sec = 0;
			to.tv_usec = WAN_TRACE_DELAY;
		}else{
			to.tv_sec = 0;
			to.tv_usec = 0;
		}
	}

	cb.cblock.command= CPIPE_DISABLE_TRACING;
	cb.cblock.buffer_length = 0;
	DoCommand();
}; /* line_trace */

//CORBA
void usage(void)
{
	printf("mppipemon: Wanpipe Multi-Port PPP Debugging Utility\n\n");
	printf("Usage:\n");
	printf("-----\n\n");
	printf("Multi-Port PPP Debugger\n\n");
	printf("mppipemon -i <ip-address or interface name> -u <port> -c <command>\n\n");
	printf("\tOption -i: \n");
	printf("\t\tWanpipe remote IP address must be supplied\n");
	printf("\t\t<or> Wanpipe network interface name (ex: wp1_chdlc)\n");   
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
	printf("\t   t         i       Trace and Interpret ALL frames\n");
	printf("\t             ip      Trace and Interpret PROTOCOL frames only\n");
	printf("\t             id      Trace and Interpret DATA frames only\n");
	printf("\t             r       Trace ALL frames, in RAW format\n");
	printf("\t             rp      Trace PROTOCOL frames only, in RAW format\n");
	printf("\t             rd      Trace DATA frames only, in RAW format\n");
	printf("\tFT1 Configuration\n");
	printf("\t   T         v       View Status \n");
	printf("\t             s       Self Test\n");
	printf("\t             l       Line Loop Test\n");
	printf("\t             d       Digital Loop Test\n");
	printf("\t             r       Remote Test\n");
	printf("\t             o       Operational Mode\n");
	printf("\t             p       FT1 CSU/DSU operational statistics\n");
	printf("\t             f       Flush FT1 CSU/DSU operational statistcs\n");
	printf("\tFlush Statistics\n");
	printf("\t   f         g       Flush Global Statistics\n");
	printf("\t             c       Flush Communication Error Statistics\n");
	printf("\t             o       Flush Operational Statistics\n");
	printf("\t             s       Flush SLARP and CDP Statistics\n");
	printf("\tExamples:\n");
	printf("\t--------\n\n");
	printf("\tex: mppipemon -i wp1_chdlc0 -u 9000 -c xm :View Modem Status \n");
	printf("\tex: mppipemon -i 201.1.1.2 -u 9000 -c ti  :Trace and Interpret ALL frames\n\n");
}

void chdlc_router_up_time( void )
{
     	unsigned long time;
     	cb.cblock.command= CPIPE_ROUTER_UP_TIME;
     	cb.cblock.buffer_length = 0;
     	cb.data[0] = 0;
     	DoCommand();
     
     	time = *(unsigned long*)&cb.data[0];

     	if (time < 3600) {
		if (time<60) 
     			printf("    Router UP Time:  %ld seconds\n", time);
		else
     			printf("    Router UP Time:  %ld minute(s)\n", (time/60));
     	}else
     		printf("    Router UP Time:  %ld hour(s)\n", (time/3600));
			
      
}

void banner (char *title){
	
	int len,i;
	
	len = strlen(title);
	printf("\n\t");
	for (i=0;i<(len+16);i++)
		printf("-");
	printf("\n\t\t%s",title);
	printf("\n\t");
	for (i=0;i<(len+16);i++)
		printf("-");
	printf("\n\n");

}

void set_FT1_monitor_status( unsigned char status)
{
	gfail = 0;
	cb.cblock.command= FT1_MONITOR_STATUS_CTRL;
	cb.cblock.buffer_length = 1;
	cb.data[0] = status; 	
	DoCommand();
	if( (cb.cblock.return_code != 0) && status){
		gfail = 1;
		printf("This command is only possible with S508/FT1 board!");
	}
} /* set_FT1_monitor_status */


/* Subroutine for changing modes on a FT1 boards */

void set_FT1_mode(int verbose)
{
	int cnt=0;

	for(;;) { 
		cb.cblock.command= SET_FT1_MODE;
		cb.cblock.buffer_length = 0;
		DoCommand();
		if(cb.cblock.return_code == 0) {
			if(verbose) {
		                printf(".");
        		        fflush(stdout);
			}
			break;
		}
		if (++cnt == MAX_FT1_RETRY){
			break;
		}
	}
} /* set_FT1_mode */

/* Read the data for status of all the lights */

void read_FT1_status( void )
{

	cb.cblock.command= CPIPE_FT1_READ_STATUS;
	cb.cblock.buffer_length = 0;
	DoCommand(); 
	if(cb.cblock.return_code == 0) {
		par_port_A_byte = cb.data[0];
		par_port_B_byte = cb.data[1];

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
 
void read_ft1_op_stats( void )
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
		
		cb.cblock.command= READ_FT1_OPERATIONAL_STATS;
 		cb.cblock.buffer_length = 0;
		DoCommand();

		FD_ZERO(&ready);
		FD_SET(0,&ready);
		to.tv_sec = 0;
		to.tv_usec = 50000;
	
		if(select(1,&ready, NULL, NULL, &to)) {
			break;
		} /* if */
		if( cb.cblock.buffer_length ){
			int i=0;
			printf("   TIME    ES   BES   SES   UAS  LOFC    ESF\n");
			length = cb.cblock.buffer_length;
			while(length){
			
				printf("%c", cb.data[i]);
				length--;
				i++;
			}
			printf("\n");
		}
	}
}

int main(int argc, char* argv[])
{
	int proceed;
	char command;
	char *opt;	

	memset(&cb,0,sizeof(udp_mgmt_pkt_t));

  	printf("\n");
   	if( argc > 2 ) {
     
		init( argc, argv);
		command = cmd[0];
		opt   = (char *) &cmd[1];
     
		#ifdef LINUX_2_1
		proceed = MakeRawConnection();
		#else
		printf("ERROR: Multi-Port PPP is not supported for kernels lower than 2.2.X\n");
		printf("       Upgrate to 2.2.X kernel for Multi-Port PPP support\n");
		exit(1);
		#endif
	
		if(proceed == WAN_TRUE){
      			switch(command){

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
						printf("ERROR: Invalid Status Command 'x', Type mppipemon <cr> for help\n\n");
					}
					break;	
      				case 's':
					if (!strcmp(opt,"g")){
	 					global_stats();
					}else if (!strcmp(opt,"c")){
	 					comm_stats();
					}else if (!strcmp(opt,"o")){
						operational_stats();
					}else if (!strcmp(opt,"s")){
						slarp_stats();
					}else{
						printf("ERROR: Invalid Statistics Command 's', Type mppipemon <cr> for help\n\n");
	
					}
					break;
      				case 'c':
					if (!strcmp(opt,"rc")){
	 					chdlc_configuration();
					}else{
						printf("ERROR: Invalid Configuration Command 'c', Type mppipemon <cr> for help\n\n");
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
						printf("ERROR: Invalid Trace Command 't', Type mppipemon <cr> for help\n\n");
					}
	 				break;
      				case 'f':
	    				if (!strcmp(opt, "o")){
	       					flush_operational_stats();
	      		 			operational_stats();
					}else if (!strcmp(opt, "s")){
	       					flush_operational_stats();
	      		 			slarp_stats();
					}else if (!strcmp(opt, "g")){
	       					flush_global_stats();
	       					global_stats();
					}else if (!strcmp(opt, "c")){
	       					flush_comm_stats();
	       					comm_stats();
	 				} else{
	    					printf("ERROR: Invalid Flush Command 'f', Type mppipemon <cr> for help\n\n");
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
					} else{
  	   					printf("ERROR: Invalid FT1 Command 'T', Type mppipemon <cr> for help\n\n");
					} 
					break;

				default:
					printf("ERROR: Invalid Command, Type mppipemon <cr> for help\n\n");
					break;
      			}//switch 
     		} 
     		close( sock );
   	} else {
      		usage();
   	} //if
   printf("\n");

   return 0;
}; //main
