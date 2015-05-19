/*****************************************************************************
* bstrm_hdlc_test_multi.c: Multiple Bstrm Test Receive Module
*
* Author(s):    Nenad Corbic <ncorbic@sangoma.com>
*
* Copyright:	(c) 1995-2006 Sangoma Technologies Inc.
*
*		This program is free software; you can redistribute it and/or
*		modify it under the terms of the GNU General Public License
*		as published by the Free Software Foundation; either version
*		2 of the License, or (at your option) any later version.
* ============================================================================
* Description:
*
* 	The chdlc_api.c utility will bind to a socket to a chdlc network
* 	interface, and continously tx and rx packets to an from the sockets.
*
*	This example has been written for a single interface in mind, 
*	where the same process handles tx and rx data.
*
*	A real world example, should use different processes to handle
*	tx and rx spearately.  
*/



#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <linux/if_wanpipe.h>
#include <string.h>
#include <signal.h>
#include <wait.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <pthread.h>
#include <linux/wanpipe.h>
#include <linux/wanpipe_tdm_api.h>
#include <libsangoma.h>
#include <wanpipe_hdlc.h>

#define FALSE	0
#define TRUE	1

/* Enable/Disable tx of random frames */
#define RAND_FRAME 0

#define MAX_NUM_OF_TIMESLOTS  32*32

#define LGTH_CRC_BYTES	2
#define MAX_TX_DATA     15000 //MAX_NUM_OF_TIMESLOTS*10	/* Size of tx data */
#define MAX_TX_FRAMES 	1000000     /* Number of frames to transmit */  

#define WRITE 1
#define MAX_IF_NAME 20
typedef struct {

	int sock;
	int rx_cnt;
	int tx_cnt;
	int data;
	int last_error;
	int frames;
	char if_name[MAX_IF_NAME+1];
	wanpipe_hdlc_engine_t *hdlc_eng;
	
} timeslot_t;


timeslot_t tslot_array[MAX_NUM_OF_TIMESLOTS];

int tx_change_data=0, tx_change_data_cnt=0;
int end=0;


void print_packet(unsigned char *buf, int len)
{
	int x;
	printf("{  | ");
	for (x=0;x<len;x++){
		if (x && x%24 == 0){
			printf("\n  ");
		}
		if (x && x%8 == 0)
			printf(" | ");
		printf("%02x ",buf[x]);
	}
	printf("}\n");
}

void sig_end(int sigid)
{
	//printf("%d: Got Signal %i\n",getpid(),sigid);
	end=1;
}        


static unsigned long next = 1;

/* RAND_MAX assumed to be 32767 */
int myrand(int max_rand) {
next = next * 1103515245 + 12345;
return((unsigned)(next/65536) % max_rand);
}

void mysrand(unsigned seed) {
next = seed;
}


/*===================================================
 * MakeConnection
 *
 *   o Create a Socket
 *   o Bind a socket to a wanpipe network interface
 *       (Interface name is supplied by the user)
 *==================================================*/         


/***************************************************************
 * Main:
 *
 *    o Make a socket connection to the driver.
 *    o Call process_con() to read/write the socket 
 *
 **************************************************************/  

typedef unsigned int  guint32;
typedef int gint32;
typedef unsigned short guint16;

static int trace_aft_hdlc_data(wanpipe_hdlc_engine_t *hdlc_eng, void *data, int len)
{
	printf("GOT HDLC DATA Len=%i\n",len);
	print_packet(data,len);
}

#pragma pack(1)
typedef struct pcap_hdr_s {
	guint32 magic_number;   /* magic number */
	guint16 version_major;  /* major version number */
	guint16 version_minor;  /* minor version number */
	gint32  thiszone;       /* GMT to local correction */
	guint32 sigfigs;        /* accuracy of timestamps */
	guint32 snaplen;        /* max length of captured packets, in octets */
	guint32 network;        /* data link type */
} pcap_hdr_t;

#if 0
typedef struct pcaprec_hdr_s {
	unsigned int ts_sec;         /* timestamp seconds */
	unsigned int ts_usec;        /* timestamp microseconds */
	unsigned short incl_len;
} pcaprec_hdr_t;
#else
typedef struct pcaprec_hdr_s {
	guint32 ts_sec;         /* timestamp seconds */
	guint32 ts_usec;        /* timestamp microseconds */
	guint32 incl_len;       /* number of octets of packet saved in file */
	guint32 orig_len;       /* actual length of packet */
} pcaprec_hdr_t;
#endif
#pragma pack()

int main(int argc, char* argv[])
{
	int proceed;
	char router_name[20];	
	char file_name[100];
	unsigned char line[80];
	FILE *file;
	int x=0,i=0;
	int err;
	int packets=0,error_bit=0,error_crc=0,error_abort=0,error_frm=0;
	pcap_hdr_t *global_hdr;
	pcaprec_hdr_t *packet_hdr;  
	int plen=sizeof(*packet_hdr);
	wanpipe_hdlc_engine_t *hdlc_eng;
	int offset=14+20+8;
	int len;
	
	
	
	nice(-11);
	
	signal(SIGINT,&sig_end);
	signal(SIGTERM,&sig_end);
    
	sprintf(file_name, argv[1]);
    file=fopen(file_name, "rb");
	if (!file) {
		printf("Failed to open file %s ",file_name);
     	return -1;
	}
	
	hdlc_eng = wanpipe_reg_hdlc_engine();
	hdlc_eng->hdlc_data = trace_aft_hdlc_data;
		
	err=fread(line, 1, sizeof(*global_hdr),  file);
	global_hdr=(pcap_hdr_t*)&line[0];
	
	if (global_hdr->magic_number == 0xa1b2c3d4) {
     	printf("Got Indetical\n");
	} else if (global_hdr->magic_number == 0xd4c3b2a1) {
     	printf("Got Swaped\n");
	} else {
     	printf("Not a pcap file\n");
		exit (1);
	}
	
	printf("Snap len = %i global=%i packet=%i err=%i vmj=%i vmin=%i\n",
		   global_hdr->snaplen,sizeof(*global_hdr),sizeof(*packet_hdr),err,
		   global_hdr->version_major,global_hdr->version_minor);
	print_packet(line,err);
		
	printf("Size of Packet =%i\n",sizeof(pcaprec_hdr_t));
	
	
	while (i++ < 100) {
	
		err=fread(line, 1, plen,  file);	
		if (err != plen) {
			printf("Error: failed to read expecting=%i read=%i\n",
				   plen,err);
			exit(1);
		}
		
		
		packet_hdr=(pcaprec_hdr_t*)&line[0];
		print_packet(line,err);
		
		len=packet_hdr->incl_len-offset;
		
		printf("Packet(%i) Len=%i Olen=%i data=%i \n",
			   plen,packet_hdr->incl_len,packet_hdr->orig_len, len);
		
		/* Ethernet + TCP + UDP */
		err=fread(line, 1, offset,  file);
		if (err != offset) {
			printf("Error: failed to read expecting=%i read=%i\n",
				   packet_hdr->incl_len,err);
         	break;
		}
		
		err=fread(line, 1, len,  file);
		
#if 0
		if (x==0 && *(unsigned int*)&line[0] != 0x7e7e7e7e) {
         	continue;
		}
#endif
		print_packet(line,err);

		x=1;
		wanpipe_hdlc_decode(hdlc_eng,line,err);

		packets+=wanpipe_get_rx_hdlc_packets(hdlc_eng);
	 	error_bit+=wanpipe_get_rx_hdlc_errors(hdlc_eng);
	    error_crc+=hdlc_eng->decoder.stats.crc;
		error_abort+=hdlc_eng->decoder.stats.abort;
	    error_frm+=hdlc_eng->decoder.stats.frame_overflow;
		printf("Status:  Read=%i packet=%i crc=%i abort=%i frm=%i error=%i\n",
			err,packets,error_crc,error_abort,error_frm,error_bit);
	
	}


	wanpipe_unreg_hdlc_engine(hdlc_eng);
	
	return 0;
};
