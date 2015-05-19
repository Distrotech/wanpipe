/*****************************************************************************
* dsapipemon.c	Wanpipe DSL Monitor
*
* Author:       Nenad Corbic <ncorbic@sangoma.com>	
*
* Copyright:	(c) 2002 Sangoma Technologies Inc.
*
*		This program is free software; you can redistribute it and/or
*		modify it under the terms of the GNU General Public License
*		as published by the Free Software Foundation; either version
*		2 of the License, or (at your option) any later version.
* ----------------------------------------------------------------------------
* May 15, 2002  Nenad Corbic 	Initial version based on cpipemon	
*****************************************************************************/

/******************************************************************************
 * 			INCLUDE FILES					      *
 *****************************************************************************/

#include <stdio.h>
#include <ctype.h>
#include <stddef.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <arpa/inet.h>
#if defined(__LINUX__)
# include <linux/version.h>
# include <linux/types.h>
# include <linux/if_packet.h>
# include <linux/if_wanpipe.h>
# include <linux/if_ether.h>
# include <linux/wanpipe_defines.h>
# include <linux/wanpipe_cfg.h>
# include <linux/wanpipe.h>
# include <linux/sdla_adsl.h>
#else
# include <netinet/in_systm.h>
# include <netinet/in.h>
# include <netinet/ip.h>
# include <netinet/udp.h>
# include <wanpipe_defines.h>
# include <wanpipe_cfg.h>
# include <wanpipe.h>
# include <sdla_adsl.h>
#endif
#include "fe_lib.h"
#include "wanpipemon.h"


#define GTI_WORD        unsigned short

/******************************************************************************
 * 			DEFINES/MACROS					      *
 *****************************************************************************/

#define TIMEOUT 	1
#define MDATALEN 	MAX_LGTH_UDP_MGNT_PKT
#define MAX_TRACE_BUF 	((MDATALEN+200)*2)

#define BANNER(str)  banner(str,0)

/******************************************************************************
 * 			TYPEDEF/STRUCTURE				      *
 *****************************************************************************/


/******************************************************************************
 * 			GLOBAL VARIABLES				      *
 *****************************************************************************/

/* Global variables */
extern int		sock;
extern wan_udp_hdr_t 	wan_udp;
extern int		protocol_cb_size;
extern int 		trace_all_data;
unsigned int 		loop_counter, frame_count;

/******************************************************************************
 * 			FUNCTION PROTOTYPES				      *
 *****************************************************************************/
extern int  	DoCommand(wan_udp_hdr_t*);
int	    	ADSLMain(char*,int,char**);
int        	ADSLUsage(void);
int         	ADSLConfig(void);
int 		ADSLDisableTrace(void);

/* Command routines */
static void error(void);
static void link_status(void);
static void line_trace(unsigned char trace_opt); 
static void failure_counters(void);
static void last_failed_status(void);
static void error_counters(void);
static void router_up_time(void);
static void adsl_baud_rate(void);
static void adsl_atm_config(void);

/******************************************************************************
 * 			FUNCTION DEFINITION				      *
 *****************************************************************************/
int ADSLConfig( void )
{
	char codeversion[10];
	unsigned char x;
   
	x = 0;
	wan_udp.wan_udphdr_command	= ADSL_TEST_DRIVER_RESPONSE;
	wan_udp.wan_udphdr_data_len 	= 0;
	wan_udp.wan_udphdr_return_code 	= 0xAA;
	while (++x < 4){
		DO_COMMAND(wan_udp); 
		if (wan_udp.wan_udphdr_return_code == 0x00){
			break;
		}
		if (wan_udp.wan_udphdr_return_code == 0xaa){
			printf("Error: Command timeout occurred\n"); 
			return(FALSE);
		}

		if (wan_udp.wan_udphdr_return_code == 0xCC ) return(FALSE);
		wan_udp.wan_udphdr_return_code 	= 0xAA;
	}

	if (x >= 4) return(FALSE);

	strlcpy(codeversion, "?.??", 10);
  
	wan_udp.wan_udphdr_command	= ADSL_READ_DRIVER_VERSION;
	wan_udp.wan_udphdr_data_len 	= 0;
	wan_udp.wan_udphdr_return_code 	= 0xaa;
	DO_COMMAND(wan_udp);
	if (wan_udp.wan_udphdr_return_code == 0){
		wan_udp.wan_udphdr_data[wan_udp.wan_udphdr_data_len] = 0;
		strlcpy(codeversion, (char*)wan_udp.wan_udphdr_data, 10);
	}
	protocol_cb_size=sizeof(wan_mgmt_t) + sizeof(wan_cmd_t) + sizeof(wan_trace_info_t) + 1;
	return TRUE;
} /* ObtainConfiguration */


static void error( void ) 
{
	printf("Error: Command failed!\n");
	printf("This command cannot execute during re-training/connecting!\n");
}; /* error */


static void remove_vendor_id (void)
{
	memset(&wan_udp.wan_udphdr_data[0],0,10);
	wan_udp.wan_udphdr_command= ADSL_RMT_VENDOR_ID_STATUS; 
	wan_udp.wan_udphdr_return_code = 0xaa;
	wan_udp.wan_udphdr_data_len = 0;
	DO_COMMAND(wan_udp);

	if (wan_udp.wan_udphdr_return_code == 0) {
		int i;
		
		BANNER("REMOTE VENDOR");
	
		printf("	Remote Vendor (hex): ");
		for (i=0;i<wan_udp.wan_udphdr_data_len;i++){
			printf ("%02x",wan_udp.wan_udphdr_data[i]);
		}

		printf("\n\n");
	
	}else{
		error();
	}
	
}

static char *decode_modulation[]={
"T1_413",
"G_LITE",
"G_DMT",
"ALCTL14",
"MULTIMODE",
"ADI",
"ALCTL",
"UAWG",
"T1_413_AUTO"
};
#define MAX_DECODE_NUM 8

static void link_status (void)
{
	unsigned short *data_ptr;
	wan_udp.wan_udphdr_command= ADSL_OP_STATE; 
	wan_udp.wan_udphdr_return_code = 0xaa;
	wan_udp.wan_udphdr_data_len = 0;
	DO_COMMAND(wan_udp);

	data_ptr = (unsigned short*)&wan_udp.wan_udphdr_data[0];

	if (wan_udp.wan_udphdr_return_code == 0) {
		
		BANNER("LINK STATUS");	
	
		printf("	Link status:\t%s , 0x%x\n\n",
				((data_ptr[0] == 0x23) ? "Connected" :
				 (data_ptr[0] == 0x18) ? "Connecting" :
				 "Disconnected"),data_ptr[0]);
		
	} else {
		error();
	}

	wan_udp.wan_udphdr_command= ADSL_START_PROGRESS; 
	wan_udp.wan_udphdr_return_code = 0xaa;
	wan_udp.wan_udphdr_data_len = 0;
	DO_COMMAND(wan_udp);

	data_ptr = (unsigned short*)&wan_udp.wan_udphdr_data[0];
	
	if (wan_udp.wan_udphdr_return_code == 0) {

		printf("	Startup Status:\t0x%x\n",
				data_ptr[0]);
		
	} else {
		error();
	}

	wan_udp.wan_udphdr_command= ADSL_GET_MODULATION; 
	wan_udp.wan_udphdr_return_code = 0xaa;
	wan_udp.wan_udphdr_data_len = 0;
	DO_COMMAND(wan_udp);

	if (wan_udp.wan_udphdr_return_code == 0) {
		if (wan_udp.wan_udphdr_data[0] > MAX_DECODE_NUM){
			printf("	Modulation:\t0x%x\n",
					wan_udp.wan_udphdr_data[0]);
		}else{
			printf("	Modulation:\t%s\n",
					decode_modulation[wan_udp.wan_udphdr_data[0]]);
		}
	}else {
		error();
	}

	
}; /* Link Status */		

static void attenuation (void)
{
	wan_udp.wan_udphdr_command= ADSL_ATTENUATION_STATUS; 
	wan_udp.wan_udphdr_return_code = 0xaa;
	wan_udp.wan_udphdr_data_len = 0;
	DO_COMMAND(wan_udp);

	if (wan_udp.wan_udphdr_return_code == 0) {
		
		BANNER("ATTENUATION");	
	
		printf("	Attenuation:  %i dB\n",
				wan_udp.wan_udphdr_data[0]);
	} else {
		error();
	}
}; /* Link Status */		



static void signal_to_noise (void)
{
	wan_udp.wan_udphdr_command= ADSL_LCL_SNR_MARGIN; 
	wan_udp.wan_udphdr_return_code = 0xaa;
	wan_udp.wan_udphdr_data_len = 0;
	DO_COMMAND(wan_udp);

	if (wan_udp.wan_udphdr_return_code == 0) {
		
		BANNER("SIGNAL TO NOISE RATIO");	
	
		printf("	Local SNR Ratio :  %i dB\n",
				wan_udp.wan_udphdr_data[0]);
	} else {
		error();
	}

	wan_udp.wan_udphdr_command= ADSL_UPSTREAM_MARGIN_STATUS; 
	wan_udp.wan_udphdr_return_code = 0xaa;
	wan_udp.wan_udphdr_data_len = 0;
	DO_COMMAND(wan_udp);

	if (wan_udp.wan_udphdr_return_code == 0) {
		
		printf("	Remote SNR Ratio:  %i dB\n",
				wan_udp.wan_udphdr_data[0]);
	} else {
		error();
	}

	
}; /* Link Status */		


static void power_status(void)
{
	unsigned short *data_ptr;
	wan_udp.wan_udphdr_command= ADSL_LCL_XMIT_POWER_STATUS; 
	wan_udp.wan_udphdr_return_code = 0xaa;
	wan_udp.wan_udphdr_data_len = 0;
	DO_COMMAND(wan_udp);

	data_ptr = (unsigned short*)&wan_udp.wan_udphdr_data[0];

	if (wan_udp.wan_udphdr_return_code == 0) {
		
		BANNER("XMIT POWER STATUS");	
	
		printf("	Local Attenuation :  %i dB\n",
				data_ptr[0]);

		printf("	Local Num of Bins :  %i\n",
				data_ptr[1]);
	
	} else {
		error();
	}

	wan_udp.wan_udphdr_command= ADSL_RMT_TX_POWER_STATUS; 
	wan_udp.wan_udphdr_return_code = 0xaa;
	wan_udp.wan_udphdr_data_len = 0;
	DO_COMMAND(wan_udp);

	data_ptr = (unsigned short*)&wan_udp.wan_udphdr_data[0];
	
	if (wan_udp.wan_udphdr_return_code == 0) {
		
		printf("	Remote Num of bins:  %i\n",
				data_ptr[0]);
	} else {
		error();
	}
}

static void last_failed_status (void)
{
	wan_udp.wan_udphdr_command= ADSL_LAST_FAILED_STATUS; 
	wan_udp.wan_udphdr_return_code = 0xaa;
	wan_udp.wan_udphdr_data_len = 0;
	DO_COMMAND(wan_udp);

	if (wan_udp.wan_udphdr_return_code == 0) {
		
		BANNER("LAST FAILED STATUS");	
	
		printf("	Last failed status: 0x%x \n",
				*((unsigned short*)&wan_udp.wan_udphdr_data[0]));
	} else {
		error();
	}
}; /* Link Status */	


static void failure_counters (void)
{
	int j;
	wan_udp.wan_udphdr_command= ADSL_FAILURES; 
	wan_udp.wan_udphdr_return_code = 0xaa;
	wan_udp.wan_udphdr_data_len = 0;
	
	DO_COMMAND(wan_udp);
	
	if (wan_udp.wan_udphdr_return_code == 0) {
		for(j=0;j<wan_udp.wan_udphdr_data_len;j++){
			printf("Cnt %i = %i\n",
				j,wan_udp.wan_udphdr_data[j]);
		}

	} else {
		error();
	}
}; /* Link Status */		

int ADSLDisableTrace()
{
	wan_udp.wan_udphdr_command	= ADSL_DISABLE_TRACING;
	wan_udp.wan_udphdr_data_len 	= 0;
	wan_udp.wan_udphdr_return_code	= 0xaa;
	DO_COMMAND(wan_udp);
	return 0;
}

static void line_trace(unsigned char trace_opt) 
{
	unsigned char num_frames, num_chars;
	unsigned short curr_pos = 0;
	wan_trace_pkt_t *trace_pkt;
	unsigned int i, j;
	char outstr[MAX_TRACE_BUF];
	int recv_buff = MDATALEN + 100;
	fd_set ready;
	struct timeval to;
	int count=0;
 
	setsockopt( sock, SOL_SOCKET, SO_RCVBUF, &recv_buff, sizeof(int) );

	/* Disable trace to ensure that the buffers are flushed */
	wan_udp.wan_udphdr_command= ADSL_DISABLE_TRACING;
	wan_udp.wan_udphdr_return_code = 0xaa;
	wan_udp.wan_udphdr_data_len = 0;
	DO_COMMAND(wan_udp);

	wan_udp.wan_udphdr_command= ADSL_ENABLE_TRACING;
	wan_udp.wan_udphdr_return_code = 0xaa;
	wan_udp.wan_udphdr_data_len = 1;
	wan_udp.wan_udphdr_data[0]=trace_opt;
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

		wan_udp.wan_udphdr_command = ADSL_GET_TRACE_INFO;
		wan_udp.wan_udphdr_return_code = 0xaa;
		wan_udp.wan_udphdr_data_len = 0;
		DO_COMMAND(wan_udp);
	
		if (wan_udp.wan_udphdr_return_code == 0 && wan_udp.wan_udphdr_data_len) { 
		     	
			num_frames = wan_udp.wan_udphdr_adsl_num_frames;
			for(i = 0; i < num_frames; i++) {
				
				trace_pkt= (wan_trace_pkt_t *)(&wan_udp.wan_udphdr_data[curr_pos]);
				
				/*  frame type */
				if (trace_pkt->status & 0x01) {
					snprintf(outstr,MAX_TRACE_BUF,"\nOUTGOING\t");
				} else {
					if (trace_pkt->status & 0x10) { 
						snprintf(outstr,MAX_TRACE_BUF,"\nINCOMING - Aborted\t");
					} else if (trace_pkt->status & 0x20) {
						snprintf(outstr,MAX_TRACE_BUF,"\nINCOMING - CRC Error\t");
					} else if (trace_pkt->status & 0x40) {
						snprintf(outstr,MAX_TRACE_BUF,"\nINCOMING - Overrun Error\t");
					} else {
						snprintf(outstr,MAX_TRACE_BUF,"\nINCOMING\t");
					}
				}

				/* real length and time stamp */
				snprintf(outstr+strlen(outstr), MAX_TRACE_BUF-strlen(outstr),
					"Len=%-5u\tTimestamp=%-5u\t", 
				     trace_pkt->real_length, trace_pkt->time_stamp);

				/* first update curr_pos */
				curr_pos += sizeof(wan_trace_pkt_t);
				if (trace_pkt->real_length >= WAN_MAX_DATA_SIZE){
					snprintf( outstr+strlen(outstr), MAX_TRACE_BUF-strlen(outstr),
						"\t:the frame data is to big (%u)!",
										trace_pkt->real_length);
				}else if (trace_pkt->data_avail == 0) {
					snprintf( outstr+strlen(outstr), MAX_TRACE_BUF-strlen(outstr),
						"\t:the frame data is not available" );
				}else{
					/* update curr_pos again */
					curr_pos += trace_pkt->real_length;

					if (!trace_all_data){
						if (trace_opt != 2){
							num_chars = (unsigned char)
								((trace_pkt->real_length <= 53)
								? trace_pkt->real_length : 53);
						}else{
							num_chars = (unsigned char)
								((trace_pkt->real_length <= 52)
								? trace_pkt->real_length : 52);
						}
					}else{
						num_chars = trace_pkt->real_length;
					}

					count = 0;
					snprintf(outstr+strlen(outstr),MAX_TRACE_BUF-strlen(outstr),"\n\t\t");
				        for( j=0; j<num_chars; j++ ) {
						count ++;

						if (trace_opt != 2){
							if (count == 15){
								snprintf(outstr+strlen(outstr),MAX_TRACE_BUF-strlen(outstr),"\n\t\t");
							}else if (count == 35){
								snprintf(outstr+strlen(outstr),MAX_TRACE_BUF-strlen(outstr),"\n\t\t");
								count=40;
							}else if (count>40 && !(count%20)){
								snprintf(outstr+strlen(outstr),MAX_TRACE_BUF-strlen(outstr),"\n\t\t");
							}
						}else{
							if (count == 5){

								/*
								sprintf(outstr+strlen(outstr)," %x %x %x %x",
								trace_pkt->data[j-3],
								trace_pkt->data[j-2],
								trace_pkt->data[j-1],
								trace_pkt->data[j-0]);
								*/
							
								snprintf(outstr+strlen(outstr),MAX_TRACE_BUF-strlen(outstr),
								"\tVpi=%d",
								trace_pkt->data[j-4]<<4 |
								(trace_pkt->data[j-3]&0xF0)>>4);

								snprintf(outstr+strlen(outstr),MAX_TRACE_BUF-strlen(outstr),"\tVci=%d",
								(trace_pkt->data[j-3]&0x0F)<<12 |
								(trace_pkt->data[j-2]<<4)|
								(trace_pkt->data[j-1]&0xF0)>>4);
						
								
								snprintf(outstr+strlen(outstr),MAX_TRACE_BUF-strlen(outstr),"\n\t\t");
								count=20;
								
							}else if (count>20 && !(count%20)){
								snprintf(outstr+strlen(outstr),MAX_TRACE_BUF-strlen(outstr),"\n\t\t");
							}
						}
						
						snprintf(outstr+strlen(outstr),MAX_TRACE_BUF-strlen(outstr),
							"%02X ", 
							(unsigned char)trace_pkt->data[j]);
				        }

					if (num_chars < trace_pkt->real_length){
						snprintf(outstr+strlen(outstr),MAX_TRACE_BUF-strlen(outstr),
							".. ");
		          		}

					outstr[strlen(outstr)-1] = '\0';
				}
	
			     printf("%s\n",outstr);
			     fflush(stdout);
			} //for
		}else{
			if (wan_udp.wan_udphdr_return_code != 0){
				printf("\n\nFailed to get trace data, exiting trace rc=0x%x!\n",
						wan_udp.wan_udphdr_return_code);
				break;	
			}
		}//if
		curr_pos = 0;

		if (wan_udp.wan_udphdr_adsl_ismoredata){
			to.tv_sec = 0;
			to.tv_usec = 0;
		}else{
			to.tv_sec = 0;
			to.tv_usec = WAN_TRACE_DELAY;
		}
	}//for

	wan_udp.wan_udphdr_command= ADSL_DISABLE_TRACING;
	wan_udp.wan_udphdr_return_code = 0xaa;
	wan_udp.wan_udphdr_data_len = 0;
	DO_COMMAND(wan_udp);
}; /* line_trace */




static char *couter_names[] = {
	"fec-I",
	"fec-F",
	"crc-I",
	"crc-F",
	"sef",
	"l-los",
	"ffec-I",
	"ffec-F",
	"febe-I",
	"febe-F",
	"rdi",
	"f-los"
};



static void error_counters (void)
{
	int j;
	wan_udp.wan_udphdr_command= ADSL_COUNTERS; 
	wan_udp.wan_udphdr_return_code = 0xaa;
	wan_udp.wan_udphdr_data_len = 0;
	
	DO_COMMAND(wan_udp);
	

	if (wan_udp.wan_udphdr_return_code == 0) {
		
		adsl_failures_t *adsl_failures=
				(adsl_failures_t*)&wan_udp.wan_udphdr_data[0];

		BANNER("ERROR COUNTERS");	

		printf("Errors CRC/Min:      %04ld\tOverall Failures:  %04ld\n",
			adsl_failures->CrcErrorsPerMin,
			adsl_failures->ExcessiveConsecutiveOverallFailureCount);

		printf("Execessive CRC/Tick: %04ld\tExcessive CRC/Min: %04ld\n\n",
			adsl_failures->ExcessiveConsecutiveCrcErrorsPerTickCount,
			adsl_failures->ExcessiveConsecutiveCrcErrorsPerMinCount);

		for(j=sizeof(adsl_failures_t);j<wan_udp.wan_udphdr_data_len;j+=2){
			printf("Cnt %02i\t:\t%s\t%03i",
				(int32_t)(j-sizeof(adsl_failures_t)),
				(j-sizeof(adsl_failures_t))>11?"N/A":
					couter_names[j-sizeof(adsl_failures_t)],
				wan_udp.wan_udphdr_data[j]);

			printf("\t");
			
			printf("Cnt %02i\t:\t%s\t%03i\n",
				(int32_t)(j+1-sizeof(adsl_failures_t)),
				(j+1-sizeof(adsl_failures_t))>11?"N/A":
				couter_names[j+1-sizeof(adsl_failures_t)],
				wan_udp.wan_udphdr_data[j+1]);
		}

	} else {
		error();
	}
}; /* Link Status */		


static void router_up_time( void )
{
     	u_int32_t time;
     	wan_udp.wan_udphdr_command= ADSL_ROUTER_UP_TIME;
	wan_udp.wan_udphdr_return_code = 0xaa;
     	wan_udp.wan_udphdr_data_len = 0;
     	wan_udp.wan_udphdr_data[0] = 0;
     	DO_COMMAND(wan_udp);
     
     	time = *(u_int32_t*)&wan_udp.wan_udphdr_data[0];

     	if (time < 3600) {
		if (time<60) 
     			printf("    Router UP Time:  %u seconds\n", time);
		else
     			printf("    Router UP Time:  %u minute(s)\n", (time/60));
     	}else
     		printf("    Router UP Time:  %u hour(s)\n", (time/3600));
      
}

static void actual_config (void)
{
	unsigned short *data_ptr;
	adsl_cfg_t*	adsl_cfg = NULL;
	wan_udp.wan_udphdr_command= ADSL_ACTUAL_CONFIGURATION;
	wan_udp.wan_udphdr_return_code = 0xaa;
     	wan_udp.wan_udphdr_data_len = 0;
     	wan_udp.wan_udphdr_data[0] = 0;
     	DO_COMMAND(wan_udp);
	
	data_ptr = (unsigned short*)&wan_udp.wan_udphdr_data[0];
	adsl_cfg = (adsl_cfg_t*)&wan_udp.wan_udphdr_data[0];
	
	BANNER("ACTUAL ADSL CONFIGURATION");

	printf("Framing overhead          : %i\n", adsl_cfg->adsl_framing);
	printf("Trellis                   : %s\n",
				(adsl_cfg->adsl_trellis) ? "Enabled":"Disabled");
	printf("\n");
	printf("AS0 bytes	          : %i\n", adsl_cfg->adsl_as0);
	printf("AS0 latency buf           : %s\n",
				(adsl_cfg->adsl_as0_latency)?"Fast":"Interleave");
	printf("AS1 bytes	          : %i\n", adsl_cfg->adsl_as1);
	printf("AS1 latency buf	          : %s\n",
				(adsl_cfg->adsl_as1)?"Fast":"Interleave");
	printf("\n");
	printf("LS0 bytes	          : %i\n", adsl_cfg->adsl_ls0);
	printf("LS0 latency buf           : %s\n",
				(adsl_cfg->adsl_ls0)?"Fast":"Interleave");  
	printf("LS1 bytes	          : %i\n", adsl_cfg->adsl_ls1);
	printf("LS1 latency buf	          : %s\n",
				(adsl_cfg->adsl_ls1)?"Fast":"Interleave");
	printf("\n");

	printf("Fast UP redundand bytes   : %i\n", adsl_cfg->adsl_redundant_bytes);
	printf("Intr UP S value bufs      : %i\n", adsl_cfg->adsl_interleave_s_up);
	printf("Interleave UP depth       : %i\n", adsl_cfg->adsl_interleave_d_up);
	printf("Intr UP redundant bufs    : %i\n", adsl_cfg->adsl_interleave_r_up); 
	printf("\n");

	printf("Fast DOWN redundand bytes : %i\n", adsl_cfg->adsl_fast_r_up);
	printf("Intr DOWN S value bufs    : %i\n", adsl_cfg->adsl_interleave_s_down);
	printf("Interleave DOWN depth     : %i\n", adsl_cfg->adsl_interleave_d_down);
	printf("Intr DOWN redundant bufs  : %i\n", adsl_cfg->adsl_interleave_r_down);
	printf("\n");
	printf("Selected standard buf	  : %i\n", adsl_cfg->adsl_selected_standard);
	
#if 0
	printf("Framing overhead          : %i\n", data_ptr[GTI_FRAMING_OVERHEAD_ACTUAL]);
	printf("Trellis                   : %s\n",
		data_ptr[GTI_TRELLIS_ACTUAL] ? "Enabled":"Disabled");
	printf("\n");
	printf("AS0 bytes	          : %i\n",data_ptr[GTI_AS0_BYTES_ACTUAL]);
	printf("AS0 latency buf           : %s\n",
		data_ptr[GTI_AS0_LATENCY_ACTUAL]?"Fast":"Interleave");
	printf("AS1 bytes	          : %i\n",data_ptr[GTI_AS1_BYTES_ACTUAL]);
	printf("AS1 latency buf	          : %s\n",
		data_ptr[GTI_AS0_LATENCY_ACTUAL]?"Fast":"Interleave");
	printf("\n");
	printf("LS0 bytes	          : %i\n",data_ptr[GTI_LS0_BYTES_ACTUAL]);
	printf("LS0 latency buf           : %s\n",
		data_ptr[GTI_LS0_LATENCY_ACTUAL]?"Fast":"Interleave");  
	printf("LS1 bytes	          : %i\n",data_ptr[GTI_LS1_BYTES_ACTUAL]);
	printf("LS1 latency buf	          : %s\n",
		data_ptr[GTI_LS0_LATENCY_ACTUAL]?"Fast":"Interleave");
	printf("\n");

	printf("Fast UP redundand bytes   : %i\n",data_ptr[GTI_FAST_R_UP_ACTUAL]);
	printf("Intr UP S value bufs      : %i\n",data_ptr[GTI_INTERLEAVE_S_UP_ACTUAL]);
	printf("Interleave UP depth       : %i\n",data_ptr[GTI_INTERLEAVE_D_UP_ACTUAL]);
	printf("Intr UP redundant bufs    : %i\n",data_ptr[GTI_INTERLEAVE_R_UP_ACTUAL]); 
	printf("\n");

	printf("Fast DOWN redundand bytes : %i\n",data_ptr[GTI_FAST_R_DOWN_ACTUAL]);
	printf("Intr DOWN S value bufs    : %i\n",data_ptr[GTI_INTERLEAVE_S_DOWN_ACTUAL]);
	printf("Interleave DOWN depth     : %i\n",data_ptr[GTI_INTERLEAVE_D_DOWN_ACTUAL]);
	printf("Intr DOWN redundant bufs  : %i\n",data_ptr[GTI_INTERLEAVE_R_DOWN_ACTUAL]);
	printf("\n");
	printf("Selected standard buf	  : %i\n",data_ptr[GTI_SELECTED_STANDARD_ACTUAL]);
#endif	
	return;
}

static void interleave_status (void)
{
	wan_udp.wan_udphdr_command= ADSL_ACTUAL_INTERLEAVE_STATUS;
	wan_udp.wan_udphdr_return_code = 0xaa;
     	wan_udp.wan_udphdr_data_len = 0;
     	wan_udp.wan_udphdr_data[0] = 0;
     	DO_COMMAND(wan_udp);

	BANNER("INTERLEAVE STATUS");

	printf("Status %i\n",*((unsigned short*)&wan_udp.wan_udphdr_data[0]));
}

static void adsl_atm_counters( void )
{
	atm_stats_t *atm_stats;
     	wan_udp.wan_udphdr_command= ADSL_ATM_CELL_COUNTER;
	wan_udp.wan_udphdr_return_code = 0xaa;
     	wan_udp.wan_udphdr_data_len = 0;
     	wan_udp.wan_udphdr_data[0] = 0;
	DO_COMMAND(wan_udp);

	atm_stats=(atm_stats_t*)&wan_udp.wan_udphdr_data[0];
	
	BANNER("CHIP ATM CELL COUNTERS");

	printf("Rx Chip          : %i\n",atm_stats->rx_chip);
	printf("Tx Chip          : %i\n",atm_stats->tx_chip);

	BANNER("DRIVER RX ATM CELL COUNTERS");
	
	printf("Valid Cells      : %i\n",atm_stats->rx_valid);
	printf("Idll  Cells      : %i\n",atm_stats->rx_empty);
	printf("Invalid Atm Hdr  : %i\n",atm_stats->rx_invalid_atm_hdr);
	printf("Invalid Prot Hdr : %i\n",atm_stats->rx_invalid_prot_hdr);
	printf("Invalid PDU Size : %i\n",atm_stats->rx_atm_pdu_size);
	printf("Rx Congestion    : %i\n",atm_stats->rx_congestion);
	printf("Rx CLP Set       : %i\n",atm_stats->rx_clp);

	BANNER("DRIVER TX ATM CELL COUNTERS");

	printf("Valid Cells      : %i\n",atm_stats->tx_valid);
}


static void adsl_baud_rate( void )
{
     	u_int32_t down, up,rxbuf,txbuf;
     	wan_udp.wan_udphdr_command= ADSL_BAUD_RATE;
	wan_udp.wan_udphdr_return_code = 0xaa;
     	wan_udp.wan_udphdr_data_len = 0;
     	wan_udp.wan_udphdr_data[0] = 0;
     	DO_COMMAND(wan_udp);
     
     	down = *(u_int32_t*)&wan_udp.wan_udphdr_data[0];
	up = *(u_int32_t*)&wan_udp.wan_udphdr_data[4];
	rxbuf=*(u_int32_t*)&wan_udp.wan_udphdr_data[8];
	txbuf=*(u_int32_t*)&wan_udp.wan_udphdr_data[12];
	
	BANNER("ADSL BAUD RATE");

	printf("	Down Rate: %i kbps : %s\n",down,rxbuf?"Fast Buf":"Interleave Buf");
	printf("	Up   Rate: %i kbps : %s\n",up,txbuf?"Fast Buf":"Interleave Buf");
}

static void adsl_atm_config( void )
{
     	u_int32_t vpi,vci,mode;
     	wan_udp.wan_udphdr_command= ADSL_ATM_CONF;
	wan_udp.wan_udphdr_return_code = 0xaa;
     	wan_udp.wan_udphdr_data_len = 0;
     	wan_udp.wan_udphdr_data[0] = 0;
     	DO_COMMAND(wan_udp);
     
     	vpi = *(u_int32_t*)&wan_udp.wan_udphdr_data[0];
	vci = *(u_int32_t*)&wan_udp.wan_udphdr_data[4];
	mode =  *(u_int32_t*)&wan_udp.wan_udphdr_data[8];
	
	BANNER("ADSL ATM CONFIGURATION");

	printf("	VPI: %i\n",vpi);
	printf("	VCI: %i\n",vci);
}

int ADSLUsage(void)
{
	printf("dslpipemon: Wanpipe ADSL Debugging Utility\n\n");
	printf("Usage:\n");
	printf("-----\n\n");
	printf("dslpipemon -i <interface name> -c <command>\n\n");
	printf("\tOption -i: \n");
	printf("\t\tWanpipe network interface name (ex: wp1_adsl)\n");   
	printf("\tOption -c: \n");
	printf("\t\tCommand is split into two parts:\n"); 
	printf("\t\t\tFirst letter is a command and the rest are options:\n"); 
	printf("\t\t\tex: ls = Link Status\n\n");
	printf("\tSupported Commands: x=status : c=config : s=statistics \n\n");
	printf("\tCommand:  Options:   Description \n");	
	printf("\t-------------------------------- \n\n");    
	printf("\tCard Status\n");
	printf("\t   x         l       Link Status\n");
	printf("\t             fs      Last Failed Status\n");
	printf("\t             ru      Display Router UP time\n");
	printf("\tCard Configuration\n");
	printf("\t   c         atm     ATM Configuration\n");
	printf("\t             b       Baud Rate\n");
	printf("\t             ac      Actual Config\n");
	printf("\t             is      Interleave Status\n");
	printf("\t             rvi     Remote vendor id\n");
	printf("\tCard Statistics\n");
	printf("\t   s         snr     Signal to noise ratio\n");
	printf("\t             a       Attenuation\n");
	printf("\t             ec      Error Counters\n");
	printf("\t             acc     Atm Cell Counters\n");
	printf("\t             ps      Xmit Power Status\n");
	printf("\tLine Trace\n");
	printf("\t   t         r       Raw Hex trace, Layer 3 network packets\n");
	printf("\t             rl2     Raw Hex trace, Layer 2 ATM PDU packets\n");
	printf("\t             rl1     Raw Hex trace, Layer 1 ATM Cells\n");
	printf("\tExamples:\n");
	printf("\t--------\n\n");
	printf("\tex: dslpipemon -i wp1_adsl -c xl 	:View Link Status \n");
	printf("\tex: dslpipemon -i wp1_adsl -c cb 	:View Baud Rate\n");
	printf("\tex: dslpipemon -i wp1_adsl -c sec	:View Error Counters\n\n");
	return 0;
}

static char *gui_main_menu[]={
"card_stats_menu","Card Status",
"card_config_menu","Card Configuration",
"stats_menu", "Card Statistics",
"trace_menu", "Trace Data",
"."
};

static char *card_stats_menu[]={
"xl","Link Status",
"xfs","Last Failed Status",
"xru","Display Router UP time",
"."
};

static char *card_config_menu[]={
"catm","ATM Configuration",
"cb","Baud Rate",
"cac","Actual Config",
"cis","Interleave Status",
"crvi", "Remote Vendor Id",
"."
};

static char *stats_menu[]={
"ssnr","Signal to noise ratio",
"sa","Attenuation",
"sec","Error Counters",
"sacc","Atm Cell Counters",
"sps","Xmit Power Status",
"."
};

static char *trace_menu[]={
"tr","Raw Hex trace, Layer 3 packets",
"trl2","Raw Hex trace, Layer 2 ATM-PDU packets",
"trl1","Raw Hex trace, Layer 1 ATM Cell packets",
"."
};

static struct cmd_menu_lookup_t gui_cmd_menu_lookup[]={
	{"card_stats_menu",card_stats_menu},
	{"card_config_menu",card_config_menu},
	{"stats_menu",stats_menu},
	{"trace_menu",trace_menu},
	{".",NULL}
};


char ** ADSLget_main_menu(int *len)
{
	int i=0;
	while(strcmp(gui_main_menu[i],".") != 0){
		i++;
	}
	*len=i/2;
	return gui_main_menu;
}

char ** ADSLget_cmd_menu(char *cmd_name,int *len)
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




int ADSLMain(char* command, int argc, char* argv[])
{
	char* opt = &command[1];
	
	switch(command[0]){

		case 'x':
			if (!strcmp(opt,"l")){
				link_status();
			}else if (!strcmp(opt,"ru")){
				router_up_time();
			}else if (!strcmp(opt,"fs")){
				last_failed_status();
			}else{
				printf("ERROR: Invalid Status Command 'x', Type wanpipemon <cr> for help\n\n");
			}
			break;	
		case 'c':
			if (!strcmp(opt,"atm")){
				adsl_atm_config();
			}else if (!strcmp(opt,"b")){
				adsl_baud_rate();
			}else if (!strcmp(opt,"ac")){
				actual_config();
			}else if (!strcmp(opt,"is")){	
				interleave_status();
			}else if (!strcmp(opt,"rvi")){
				remove_vendor_id();	
			}else{
				printf("ERROR: Invalid Status Command 'c', Type wanpipemon <cr> for help\n\n");
			}
			break;
			
		case 's':
			if (!strcmp(opt,"ec")){
				error_counters();
			}else if (!strcmp(opt,"fc")){
				failure_counters();
			}else if (!strcmp(opt,"snr")){
				signal_to_noise();
			}else if (!strcmp(opt,"ps")){
				power_status();
			}else if (!strcmp(opt,"a")){
				attenuation();
			}else if (!strcmp(opt,"acc")){	
				adsl_atm_counters();
			}else{
				printf("ERROR: Invalid Statistics Command 's', Type wanpipemon <cr> for help\n\n");
			}
			break;

		case 't':
			if (!strcmp(opt, "r")){
				line_trace(0);

			}else if (!strcmp(opt, "rl2")){
				line_trace(1);
		
			}else if (!strcmp(opt, "rl1")){
				line_trace(2);
				
			}else{
				printf("ERROR: Invalid Statistics Command 't', Type wanpipemon <cr> for help\n\n");

			}
			break;

		default:
			printf("ERROR: Invalid Command, Type wanpipemon <cr> for help\n\n");
			break;
	}//switch 
   
	return 0;
}; //main
