/*****************************************************************************
* wanpipemon.c	X25 Debugg Monitor.
*
* Authors:	Nenad Corbic <ncorbic@sangoma.com>	
*
* Copyright:	(c) 1999 Sangoma Technologies Inc.
*
*		This program is free software; you can redistribute it and/or
*		modify it under the terms of the GNU General Public License
*		as published by the Free Software Foundation; either version
*		2 of the License, or (at your option) any later version.
*
* NOTE:  This program is still under construction.  Only critical features
*        are enabled.
* ----------------------------------------------------------------------------
* Jun 28, 1999	Nenad Corbic 	Initial version based on wanpipemon for linux.
*****************************************************************************/

#if !defined(__LINUX__)
# error "X25 Monitor not supported on FreeBSD/OpenBSD OS!"
#endif

#include <linux/version.h>
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

#if defined (__LINUX__)
#include <linux/if_ether.h>
#include <linux/if_packet.h>
#else
#error "Not supported on NON Linux Platforms"
#endif

#include "wanpipe_api.h"
#include "sdla_x25.h"
#include "fe_lib.h"
#include "wanpipemon.h"


#define TIMEOUT 1
#define MDATALEN 2024

#define BANNER(str)  banner(str,0)

/******************************************************************************
 * Structure for tracing
 *
 */

/* Prototypes */
void error( char return_code);
void read_hdlc_stat(void);
void flush_hdlc_stat(void);

/* global for now */
static unsigned char      station_config;
int                off_counter, green_counter, red_counter;
int                loop_counter, fail;

/* defines for now */
extern int lcn_number;


int X25Config (void) 
{
   	unsigned char x;
  	char codeversion[10]; 
   	x = 0;
   	wan_udp.wan_udphdr_command = X25_READ_CONFIGURATION;
   	wan_udp.wan_udphdr_data_len = 0;
   	wan_udp.wan_udphdr_x25_lcn = 0;
   	while (++x < 4){
   		DO_COMMAND(wan_udp); 
      		if (wan_udp.wan_udphdr_return_code == 0x00){
			break;
		}
      		if (wan_udp.wan_udphdr_return_code == 0xaa){
	 		printf("Error: Command timeout occurred\n"); 
	 		return(WAN_FALSE);
      		}
      		if (wan_udp.wan_udphdr_return_code == 0xCC ) return(WAN_FALSE);
   	}
   
	if (x >= 4) return(WAN_FALSE);
   	station_config = wan_udp.wan_udphdr_data[0];
   
   	strcpy(codeversion, "?.??");
   
   	wan_udp.wan_udphdr_command = X25_READ_CODE_VERSION;
  	wan_udp.wan_udphdr_data_len = 0;
   	DO_COMMAND(wan_udp);

   	if (wan_udp.wan_udphdr_return_code == 0) {
      		wan_udp.wan_udphdr_data[wan_udp.wan_udphdr_data_len] = 0;
      		strcpy(codeversion, (char*)wan_udp.wan_udphdr_data);
   	}

	protocol_cb_size=sizeof(wan_mgmt_t) + sizeof(wan_cmd_t) + 1;
   	return(WAN_TRUE);
}; /* ObtainConfiguration */

void error( char return_code ) 
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

/*===============================================
 * Link Status
 *
 *==============================================*/

void link_status( void ) 
{
	wan_udp.wan_udphdr_command = X25_HDLC_LINK_STATUS;
	wan_udp.wan_udphdr_data_len = 0;
	wan_udp.wan_udphdr_x25_lcn = 0;
	DO_COMMAND(wan_udp);

        if(wan_udp.wan_udphdr_return_code == 0 || wan_udp.wan_udphdr_return_code == 1){

                BANNER("LINK STATUS");

                switch (wan_udp.wan_udphdr_return_code){
		case 0:
              		printf("\t    Link is in a disconnected mode : Disconnected\n");
			break;
		case 1:
			printf("\t Asynchronously Balanced Mode (ABM): Connected\n");
			break;
		default:
			printf("\t Error: Command Failed, Please try again\n");
			return;
		}

        	printf("\tNumber of Inf. Frames queued for tx: %i\n", 
                     			wan_udp.wan_udphdr_data[0]);
                printf("\tNumber of Inf. Frames queued for rx: %i\n",
                    			wan_udp.wan_udphdr_data[1]);
		switch (wan_udp.wan_udphdr_data[2]){
		case 1: 
			printf("\t                 Link Configured as: DTE\n");
			break;
		case 2:
			printf("\t                 Link Confiugred as: DCE\n");
			break;
		default:
  			printf("\t    Unknown Link Configuration: DTE or DCE ?\n");
		}
                
          	printf("\t            Supervisory frame count: %i\n",wan_udp.wan_udphdr_data[4]); 
                 
	}else
             	error(wan_udp.wan_udphdr_return_code);
      		
} /* link_status */


/*===============================================
 * Modem Status 
 *
 *==============================================*/

void modem_status( void ) 
{
   	wan_udp.wan_udphdr_command = X25_READ_MODEM_STATUS;
   	wan_udp.wan_udphdr_data_len = 0;
   	DO_COMMAND(wan_udp);
   
	if (wan_udp.wan_udphdr_return_code == 0) {

		BANNER("MODEM STATUS");

                /* If bit 3 is set DCD is hight, otherwise low */
      		if (wan_udp.wan_udphdr_data[0] & 0x08) 
	 		printf("DCD: HIGH\n");
      		else
	 		printf("DCD: LOW\n");
      	
                /* If bit 5 is set CTS is hight, otherwise low */
      		if( wan_udp.wan_udphdr_data[0] & 0x20) 
	 		printf("CTS: HIGH\n");
      		else 
	 		printf("CTS: LOW\n");
      	} else {
      		error(wan_udp.wan_udphdr_return_code);
   	} 

} 
 
/*===============================================
 * HDLC Statistics
 *
 *==============================================*/

void read_hdlc_stat( void)
{
	read_hdlc_stat_t *stat;
	ResetWanUdp(&wan_udp);        
  	wan_udp.wan_udphdr_command = X25_HDLC_READ_STATS;
   	wan_udp.wan_udphdr_data_len = 0;
   	DO_COMMAND(wan_udp);

   	stat = (read_hdlc_stat_t *)&wan_udp.wan_udphdr_data[0];

        if (!wan_udp.wan_udphdr_return_code)
	{
		BANNER("X25:  HDLC STATISTICS");

		printf("       Inf. Frames received successfuly : %i\n",
					stat->inf_frames_rx_ok);
		printf("   Inf. Frames received out of sequence : %i\n",
					stat->inf_frames_rx_out_of_seq);
		printf("Inf. Frames received with no data filed : %i\n",
					stat->inf_frames_rx_no_data);
		printf("        Incomming Inf. Frames discarded : %i\n",
					stat->inf_frames_rx_dropped);
		printf("Incomming Inf. Frames with data to long : %i\n",
					stat->inf_frames_rx_data_too_long);
		printf(" Frames received with invalid HDLC addr : %i\n",
					stat->inf_frames_rx_invalid_addr);
		printf("   Inf. Frames tarnsmitted successfully : %i\n",
					stat->inf_frames_tx_ok);
		printf("              Inf. Frames retransmitted : %i\n",
					stat->inf_frames_tx_retransmit);
		printf("                  Number of T1 Timeouts : %i\n",
					stat->T1_timeouts);
		printf("         Number of SABM frames received : %i\n",
					stat->SABM_frames_rx);
		printf("         Number of DISC frames received : %i\n",
					stat->DISC_frames_rx);
		printf("           Number of DM frames received : %i\n",
					stat->DM_frames_rx);
		printf("         Number of FRMR frames received : %i\n",
					stat->FRMR_frames_rx);
		printf("      Number of SABM frames transmitted : %i\n",
					stat->SABM_frames_tx);
		printf("      Number of DISC frames transmitted : %i\n",
					stat->DISC_frames_tx);
		printf("        Number of DM frames transmitted : %i\n",
					stat->DM_frames_tx);
		printf("      Number of FRMR frames transmitted : %i\n",
					stat->FRMR_frames_tx);
	}else
		error(wan_udp.wan_udphdr_return_code);

} /* read_hdlc_stat */

/*===============================================
 * Flush HDLC Statistics
 *
 *==============================================*/

void flush_hdlc_stat( void)
{
  	wan_udp.wan_udphdr_command = X25_HDLC_FLUSH_STATS;
   	wan_udp.wan_udphdr_data_len = 0;
   	DO_COMMAND(wan_udp);

	if (!wan_udp.wan_udphdr_return_code)
		printf("\n\tHDLC Statistics have been cleared \n");
	else
		error(wan_udp.wan_udphdr_return_code);
	
} /* flush_hdlc_stat */


/*===============================================
 * Communication Error Statistics 
 *
 *==============================================*/

void comm_err(void) 
{
	read_comms_err_stats_t *stat; 
	ResetWanUdp(&wan_udp);
   	wan_udp.wan_udphdr_command = X25_HDLC_READ_COMM_ERR;
   	wan_udp.wan_udphdr_data_len = 0;
   	wan_udp.wan_udphdr_x25_lcn = 0;	// for supervisor display
   
	DO_COMMAND(wan_udp);

	stat = (read_comms_err_stats_t *)&wan_udp.wan_udphdr_data[0];
   
	if (wan_udp.wan_udphdr_return_code == 0 && wan_udp.wan_udphdr_data_len == 0x0A) {

		BANNER("X25: COMMUNICATION ERROR STATISTICS");

      		printf("               Number of receiver overrun errors: %d\n",
						stat->overrun_err_rx);
      		printf("                   Number of receiver CRC errors: %d\n",
						stat->CRC_err);
      		printf("                 Number of abort frames received: %d\n",
						stat->abort_frames_rx);
      		printf("Number of times receiver disabled (buffers full): %d\n",
						stat->frames_dropped_buf_full);
      		printf("              Number of abort frames transmitted: %d\n",
						stat->abort_frames_tx);
                printf("                    Number fo transmit underruns: %d\n",
						stat->transmit_underruns);
      		printf("   Number of transmit underrun interrupts missed: %d\n",
						stat->missed_tx_underruns_intr);
      		printf("        Number of times DCD dropped unexpectedly: %d\n",
						stat->DCD_drop);
      		printf("        Number of times CTS dropped unexpectedly: %d\n",
						stat->CTS_drop);
   	} else {
      		error(wan_udp.wan_udphdr_return_code);
   	} 
}; /* comm_err(); */


/*===============================================
 * Flush Communication Error Statistics 
 *
 *==============================================*/


void flush_comm_err( void ) 
{
	wan_udp.wan_udphdr_command = X25_HDLC_FLUSH_COMM_ERR;
	wan_udp.wan_udphdr_data_len = 0;
	wan_udp.wan_udphdr_x25_lcn = 0;
	DO_COMMAND(wan_udp);
	if (!wan_udp.wan_udphdr_return_code) 
		printf("Comm Error Statistics have been Cleared\n");
	else
		error(wan_udp.wan_udphdr_return_code);
}; /* flush_comm_err */


#ifdef NEX_XPIPE 

/* FIXME:  The following code is not used.  The wanpipemon is half
           finished, and only supports critical commands */

void global_stats( void ) 
{
   	ResetWanUdp(&wan_udp);
   	wan_udp.wan_udphdr_command = READ_DLC_STATISTICS;
   	wan_udp.wan_udphdr_data_len = 0;
   	cb.dlci = 0;
   	DO_COMMAND(wan_udp);
   
	if (wan_udp.wan_udphdr_return_code == 0) {
      		if( station_config == 0 ) {
	 		printf("                       Full Status Enquiry messages sent: %d\n", *(unsigned short*)&wan_udp.wan_udphdr_data[12]);
	 		printf("Link Integrity Verification Status Enquiry messages sent: %d\n", *(unsigned short*)&wan_udp.wan_udphdr_data[14]);
	 		printf("                           Full Status messages received: %d\n", *(unsigned short*)&wan_udp.wan_udphdr_data[16]);
	 		printf("    Link Integrity Verification Status messages received: %d\n", *(unsigned short*)&wan_udp.wan_udphdr_data[18]);
      		}
   	} else {
		error(wan_udp.wan_udphdr_return_code);	  	
  	 } 
}; /* global_stats */



void flush_global_stats( void ) 
{
   	wan_udp.wan_udphdr_command = FLUSH_DLC_STATISTICS;
   	wan_udp.wan_udphdr_data_len = 1;
   	cb.dlci = 0;
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

#endif

int X25DisableTrace(void)
{
	wan_udp.wan_udphdr_command = XPIPE_DISABLE_TRACING;
   	wan_udp.wan_udphdr_data_len = 2;
        wan_udp.wan_udphdr_data[0] = 0x00;
        wan_udp.wan_udphdr_data[1] = 0x00;
   	DO_COMMAND(wan_udp);
	return 0;
}


void decode_timestamp (trace_data_t *trace_info)
{
	time_t time_val=trace_info->sec;
	struct tm *time_tm = localtime(&time_val);
	char tmp_time[50];

	/* week day */
	//strftime(tmp_time,sizeof(tmp_time),"%a",time_tm);
	//printf("%s ",tmp_time);
	
	/* month */
	strftime(tmp_time,sizeof(tmp_time),"%b",time_tm);
	printf("%s ",tmp_time);
	
	/* numeric day of the month */
	strftime(tmp_time,sizeof(tmp_time),"%d",time_tm);
	printf("%s ",tmp_time);
	
	/* Hour of day */
	strftime(tmp_time,sizeof(tmp_time),"%H",time_tm);
	printf("%s:",tmp_time);
	
	/* Minute of hour */
	strftime(tmp_time,sizeof(tmp_time),"%M",time_tm);
	printf("%s:",tmp_time);
	
	/* Second of minute */
	strftime(tmp_time,sizeof(tmp_time),"%S",time_tm);
	printf("%s ",tmp_time);

	printf("%u [1/100s]",trace_info->usec );
}

void line_trace(int trace_mode) 
{
  	fd_set ready;
   	struct timeval to;
        char *trace_data;
        trace_data_t *trace_info;
	wp_trace_output_iface_t trace_iface;
   
	memset(&trace_iface,0,sizeof(wp_trace_output_iface_t));

    	ResetWanUdp(&wan_udp);
   	wan_udp.wan_udphdr_command = XPIPE_DISABLE_TRACING;
   	wan_udp.wan_udphdr_data_len = 2;
        wan_udp.wan_udphdr_data[0] = 0x00;
        wan_udp.wan_udphdr_data[1] = 0x00;
   	DO_COMMAND(wan_udp);
 
        ResetWanUdp(&wan_udp);
   	wan_udp.wan_udphdr_command = XPIPE_ENABLE_TRACING;
   	wan_udp.wan_udphdr_data_len = 0x02;
	wan_udp.wan_udphdr_data[0] |= TRACE_DEFAULT;

	if (trace_mode == TRACE_PROT){
		wan_udp.wan_udphdr_data[0] |= TRACE_SUPERVISOR_FRMS | 
			      TRACE_ASYNC_FRMS;

	}else if (trace_mode == TRACE_DATA){
		wan_udp.wan_udphdr_data[0] |= TRACE_DATA_FRMS;

	}else{
		wan_udp.wan_udphdr_data[0] |= TRACE_ALL_HDLC_FRMS;	  
	}
        wan_udp.wan_udphdr_data[1] = 0x00;
   	DO_COMMAND(wan_udp);

   	if( wan_udp.wan_udphdr_return_code == 0 ) { 
      		printf("Starting trace...(Press ENTER to exit)\n");
   	} else if (wan_udp.wan_udphdr_return_code == 0x1F) {
      		printf("Line Tracing is possible only with S508 board.\n");
      		return;
   	} else if( wan_udp.wan_udphdr_return_code == 0xCD ) {
      		printf("Cannot Enable Line Tracing from Underneath.\n");
      		return;
   	} else if( wan_udp.wan_udphdr_return_code == 0x01 ) {
      		printf("Starting trace...(although it's already enabled!)\n");
      		printf("Press ENTER to exit.\n");
   	} else {
      		printf("Failed to Enable Line Tracing. Return code: 0x%02X\n", wan_udp.wan_udphdr_return_code );
      		return;
   	}
  	fflush(stdout);
	to.tv_sec = 0;
        to.tv_usec = 0;
 
	for(;;){
		
		FD_ZERO(&ready);
		FD_SET(0,&ready);

		if(select(1,&ready, NULL, NULL, &to)) {
			break;
		} /* if */

		to.tv_sec = 0;
		to.tv_usec = 0;

		ResetWanUdp(&wan_udp); 
		wan_udp.wan_udphdr_command = XPIPE_GET_TRACE_INFO;
		wan_udp.wan_udphdr_data_len = 0;
		DO_COMMAND(wan_udp);

		trace_info = (trace_data_t *)&wan_udp.wan_udphdr_data[0];
		trace_data = (char *)&trace_info->data;

	    	if (wan_udp.wan_udphdr_return_code == 0 && 
	            wan_udp.wan_udphdr_data_len) { 

			/* frame type */
//                   	printf("\n\nLength     = %i",trace_info->length);  
//                   	printf("\nTime Stamp = 0x%04X",trace_info->timestamp);

			trace_iface.status=0;
			if (trace_info->type & 0x01) {
				trace_iface.status|=WP_TRACE_OUTGOING;
			}

			trace_iface.len = trace_info->length;
			trace_iface.timestamp=trace_info->timestamp;
			trace_iface.sec=trace_info->sec;
			trace_iface.usec=trace_info->usec;

			if (trace_info->length > wan_udp.wan_udphdr_data_len){
				continue;		
			}

			if (trace_info->length == 0) {
				printf("the frame data is not available" );
				printf("\n");
				fflush(stdout);
				continue;
			}

			
		   	trace_iface.trace_all_data=trace_all_data;
			trace_iface.data=(unsigned char*)&trace_data[0];
			trace_iface.link_type = 3;

			if (pcap_output){
				trace_iface.type=WP_OUT_TRACE_PCAP;
			}else if (raw_data) {
				trace_iface.type=WP_OUT_TRACE_RAW;
			}else{
				trace_iface.type=WP_OUT_TRACE_INTERP;
			}

			wp_trace_output(&trace_iface);
		     	fflush(stdout);

			to.tv_sec = 0;
		        to.tv_usec = 0;
      	   	}else{
			to.tv_sec = 0;
	        	to.tv_usec = WAN_TRACE_DELAY;
           	}
  	}

	fflush(stdout);
        ResetWanUdp(&wan_udp);
   	wan_udp.wan_udphdr_command = XPIPE_DISABLE_TRACING;
   	wan_udp.wan_udphdr_data_len = 2;
        wan_udp.wan_udphdr_data[0] = 0x00;
        wan_udp.wan_udphdr_data[1] = 0x00;
   	DO_COMMAND(wan_udp);
}; //line_trace


void x25_driver_stat_ifsend( void )
{
      if_send_stat_t *if_snd_stat;

      wan_udp.wan_udphdr_command = XPIPE_DRIVER_STAT_IFSEND;
      wan_udp.wan_udphdr_data_len = 0;
      wan_udp.wan_udphdr_data[0] = 0;
      DO_COMMAND(wan_udp);
      if_snd_stat = (if_send_stat_t *) &wan_udp.wan_udphdr_data[0];

      BANNER("X25:  DRIVER IF_SEND STATISTICS");       

      printf("                                    Total Number of Send entries:  %lu\n", 
                                     	if_snd_stat->if_send_entry);
      printf("                          Number of Send entries with SKB = NULL:  %lu\n", 
                                      	if_snd_stat->if_send_skb_null);
      printf("Number of Send entries with broadcast addressed packet discarded:  %lu\n", 
                    			if_snd_stat->if_send_broadcast);
      printf("Number of Send entries with multicast addressed packet discarded:  %lu\n", 
					if_snd_stat->if_send_multicast);
      printf("                Number of Send entries with CRITICAL_RX_INTR set:  %lu\n", 
					if_snd_stat->if_send_critical_ISR);
      printf("   Number of Send entries with Critical set and packet discarded:  %lu\n", 
					if_snd_stat->if_send_critical_non_ISR);
      printf("                     Number of Send entries with Device Busy set:  %lu\n", 
					if_snd_stat->if_send_tbusy);
      printf("                 Number of Send entries with Device Busy Timeout:  %lu\n", 
					if_snd_stat->if_send_tbusy_timeout);
      printf("               Number of Send entries with XPIPE MONITOR Request:  %lu\n", 
					if_snd_stat->if_send_PIPE_request);
      printf("                    Number of Send entries with WAN Disconnected:  %lu\n", 
					if_snd_stat->if_send_wan_disconnected);
      printf("                         Number of Send entries with Send failed:  %lu\n", 
					if_snd_stat->if_send_bfr_not_passed_to_adptr);
      printf("        If_send, number of packets passed to the board successfully:  %lu\n", 
					if_snd_stat->if_send_bfr_passed_to_adptr);
      
 
} /* x25_driver_stat_ifsend */

void x25_driver_stat_intr( void )
{
      global_stats_t *global_stat;
      rx_intr_stat_t *rx_int_stat;
      wan_udp.wan_udphdr_command = XPIPE_DRIVER_STAT_INTR;
      wan_udp.wan_udphdr_data_len = 0;
      wan_udp.wan_udphdr_data[0] = 0;
      DO_COMMAND(wan_udp);
      global_stat = (global_stats_t *)&wan_udp.wan_udphdr_data[0];
      rx_int_stat = (rx_intr_stat_t *)&wan_udp.wan_udphdr_data[sizeof(global_stats_t)];
       
      BANNER("X25: DRIVER INTERRUPT STATISTICS");       

      printf("                                   Number of ISR entries:    %lu\n" , 
                              		global_stat->isr_entry);
      printf("                 Number of ISR entries with Critical Set:    %lu\n" , 
					global_stat->isr_already_critical);
      printf("                             Number of Receive Interrupt:    %lu\n" , 
					global_stat->isr_rx);
      printf("                            Number of Transmit Interrupt:    %lu\n" , 
					global_stat->isr_tx);
      printf("             Number of ISR entries for Interrupt Testing:    %lu\n" , 
					global_stat->isr_intr_test);
      printf("                            Number of Spurious Interrupt:    %lu\n" , 
					global_stat->isr_spurious);
      printf("      Number of Times Transmit Interrupts Enabled in ISR:    %lu\n" , 
					global_stat->isr_enable_tx_int);
      printf("        Number of Receive Interrupts with Corrupt Buffer:    %lu\n\n" , 
					global_stat->rx_intr_corrupt_rx_bfr);

      printf("                                  Number of Poll Entries:    %lu\n",
                 			global_stat->poll_entry);
      printf("                Number of Poll Entries with Critical set:    %lu\n", 
					global_stat->poll_already_critical);
      printf("                        Number of Poll Entries Processed:    %lu\n\n", 
					global_stat->poll_processed);


      printf("             Number of Receive Interrupts with No socket:    %lu\n" , 
					rx_int_stat->rx_intr_no_socket);
      printf("  Number of Receive Interrupts for XPIPE MONITOR Request:    %lu\n" , 
					rx_int_stat->rx_intr_PIPE_request);
      printf("Number of Receive Interrupts with Buffer Passed to Stack:    %lu\n" , 
					rx_int_stat->rx_intr_bfr_passed_to_stack);
     

} /* x25_driver_stat_intr */


void x25_driver_stat_gen( void )
{
     
      pipe_mgmt_stat_t *pipe_mgmt_stat;

      wan_udp.wan_udphdr_command = XPIPE_DRIVER_STAT_GEN;
      wan_udp.wan_udphdr_data_len = 0;
      wan_udp.wan_udphdr_data[0] = 0;
      DO_COMMAND(wan_udp);
      pipe_mgmt_stat = (pipe_mgmt_stat_t *)&wan_udp.wan_udphdr_data[0]; 

      BANNER("X25:  DRIVER GENERAL STATISTICS");       

      printf("            Number of XPIPE Monitor call with kmalloc error:  %lu\n", 
					pipe_mgmt_stat->UDP_PIPE_mgmt_kmalloc_err);
      printf("       Number of XPIPE Monitor call with Adapter Type error:  %lu\n", 
					pipe_mgmt_stat->UDP_PIPE_mgmt_adptr_type_err);
      printf("          Number of XPIPE Monitor call with Direction Error:  %lu\n", 
					pipe_mgmt_stat->UDP_PIPE_mgmt_direction_err);
      printf("  Number of XPIPE Monitor call with Adapter Command Timeout:  %lu\n", 
					pipe_mgmt_stat->UDP_PIPE_mgmt_adptr_cmnd_timeout);
      printf("       Number of XPIPE Monitor call with Adapter Command OK:  %lu\n", 
					pipe_mgmt_stat->UDP_PIPE_mgmt_adptr_cmnd_OK);
      printf("      Number of XPIPE Monitor call with Adapter Send passed:  %lu\n", 
					pipe_mgmt_stat->UDP_PIPE_mgmt_adptr_send_passed);
      printf("      Number of XPIPE Monitor call with Adapter Send failed:  %lu\n", 
					pipe_mgmt_stat->UDP_PIPE_mgmt_adptr_send_failed);
      printf("   Number of XPIPE Monitor call with packet passed to stack:  %lu\n", 
					pipe_mgmt_stat->UDP_PIPE_mgmt_passed_to_stack);
      printf("  Number of XPIPE Monitor call with pkt NOT passed to stack:  %lu\n",
 					pipe_mgmt_stat->UDP_PIPE_mgmt_not_passed_to_stack);
      printf("                Number of XPIPE Monitor call with no socket:  %lu\n\n", 
					pipe_mgmt_stat->UDP_PIPE_mgmt_no_socket);

} /* x25_driver_stat_gen */


void flush_driver_stats( void )
{
      wan_udp.wan_udphdr_command = XPIPE_FLUSH_DRIVER_STATS;
      wan_udp.wan_udphdr_data_len = 0;
      wan_udp.wan_udphdr_data[0] = 0;
      DO_COMMAND(wan_udp);

      printf("All Driver Statistics are Flushed.\n");

} /* flush_driver_stats */

void x25_router_up_time( void )
{
     unsigned long time;
     wan_udp.wan_udphdr_command = XPIPE_ROUTER_UP_TIME;
     wan_udp.wan_udphdr_data_len = 0;
     wan_udp.wan_udphdr_data[0] = 0;
     DO_COMMAND(wan_udp);
    
     BANNER("X25: ROUTER/API UP TIME");
 
     time = wan_udp.wan_udphdr_data[0] + (wan_udp.wan_udphdr_data[1]*256) + (wan_udp.wan_udphdr_data[2]*65536) + 
					(wan_udp.wan_udphdr_data[3]*16777216);
     if (time < 3600) {
	if (time<60) 
     		printf("    Router UP Time:  %lu seconds\n", time);
	else
     		printf("    Router UP Time:  %lu minute(s)\n", (time/60));
     }else
     		printf("    Router UP Time:  %lu hour(s)\n", (time/3600));
			
      
} /* x25_router_up_time */

void read_x25_statistics (void)
{
	TX25Stats *stats;

	ResetWanUdp(&wan_udp);
   	wan_udp.wan_udphdr_command = X25_READ_STATISTICS;
   	wan_udp.wan_udphdr_data_len = 0;
	DO_COMMAND(wan_udp);	
	
	BANNER("X25: X25 STATISTICS");

	printf("Command %x\n",wan_udp.wan_udphdr_command);

	if (wan_udp.wan_udphdr_return_code != 0){
		printf("\t Error X25 Statistics Command Failed. Please Try Again\n");
		return;
	}

	stats = (TX25Stats *)&wan_udp.wan_udphdr_data[0];
	
	printf ("             Restart Request Tx : %i\n", stats->txRestartRqst);
	printf ("             Restart Request Rx : %i\n", stats->rxRestartRqst);
	printf ("       Restart Confirmations Tx : %i\n", stats->txRestartConf);
	printf ("       Restart Confirmations Rx : %i\n", stats->rxRestartConf);
	printf ("               Reset Request Tx : %i\n", stats->txResetRqst);
	printf ("               Reset Request Rx : %i\n", stats->rxResetRqst);
	printf ("         Reset Confirmations Tx : %i\n", stats->txResetConf);
	printf ("         Reset Confirmations Rx : %i\n", stats->rxResetConf);
	printf ("                Call Request Tx : %i\n",stats->txCallRequest);	
	printf ("                Call Request Rx : %i\n",stats->rxCallRequest);
	printf ("               Call Accepted Tx : %i\n",stats->txCallAccept);
	printf ("               Call Accepted Rx : %i\n",stats->rxCallAccept); 
	printf ("               Clear Request Tx : %i\n",stats->txClearRqst);
	printf ("               Clear Request Rx : %i\n",stats->rxClearRqst);
	printf ("          Clear Confirmation Tx : %i\n",stats->txClearConf);
	printf ("          Clear Confirmation Rx : %i\n",stats->rxClearConf);
	printf ("         Diagnostics Packets Tx : %i\n",stats->txDiagnostic);
	printf ("         Diagnostics Packets Rx : %i\n",stats->rxDiagnostic);
	printf ("        Registration Request Tx : %i\n",stats->txRegRqst);
	printf ("        Registration Request Rx : %i\n",stats->rxRegRqst);
	printf ("   Registration Confirmation Tx : %i\n",stats->txRegConf);
	printf ("   Registration Confirmation Rx : %i\n",stats->rxRegConf);
	printf ("           Interrupt Packets Tx : %i\n",stats->txInterrupt);
	printf ("           Interrupt Packets Rx : %i\n",stats->rxInterrupt);
	printf ("     Interrupt Confirmations Tx : %i\n",stats->txIntrConf);
	printf ("     Interrupt Confirmations Rx : %i\n",stats->rxIntrConf);
	printf ("                Data Packets Tx : %i\n",stats->txData);
	printf ("                Data Packets Rx : %i\n",stats->rxData);
	printf ("                  RR Packets Tx : %i\n",stats->txRR);
	printf ("                  RR Packets Rx : %i\n",stats->rxRR);
	printf ("                 RNR Packets Tx : %i\n",stats->txRNR);
	printf ("                 RNR Packets Rx : %i\n",stats->rxRNR);
}


//------------------------ END OF FT1 STATISTICS ------------------------



int X25Usage( void ) {

	printf("wanpipemon: Wanpipe X25 Debugging Utility\n\n");
	printf("Usage:\n");
	printf("-----\n\n");
	printf("wanpipemon -i <ip-addr or interface> -u <port> -c <command> -d <dlci num>\n\n");
	printf("\tOption -i: \n");
	printf("\t\tWanpipe remote IP address or\n");
	printf("\t\tWanpipe network interface name (ex: wp1_fr16)\n");   
	printf("\tOption -u: (Optional, default 9000) \n");
	printf("\t\tWanpipe UDPPORT specified in /etc/wanpipe#.conf\n");
	printf("\tOption -c: \n");
	printf("\t\tFpipemon Command\n"); 
	printf("\t\t\tFirst letter is a command and the rest are options:\n"); 
	printf("\t\t\tex: xm = View Modem Status\n\n");
	printf("\tSupported Commands: x=status : s=statistics : t=trace \n");
	printf("\t                    c=config : T=FT1 stats  : f=flush\n\n");
	printf("\tCommand:  Options:   Description \n");	
	printf("\t-------------------------------- \n\n");    
	printf("\tCard Status\n");
	printf("\t   x         m       Modem Status\n");
	printf("\t             l       Link Status\n");
	printf("\t             ru      Display Router UP time\n");
	//printf("\tCard Configuration\n");
	//printf("\t   c         l       List Active DLCIs\n");
	printf("\tCard Statistics\n");
	//printf("\t   s         g       Global Statistics\n");
	printf("\t   s         c       Communication Error Statistics\n");
        printf("\t             x       X25 Statistics\n");
	printf("\t             h       HDLC Statistics\n");
	printf("\tTrace Data \n");
	//printf("\t   t         i       Trace and Interpret ALL frames\n");
	//printf("\t             ip      Trace and Interpret PROTOCOL frames only\n");
	//printf("\t             id      Trace and Interpret DATA frames only\n");
	printf("\t   t         r       Trace ALL frames, in RAW format\n");
	printf("\t             rp      Trace PROTOCOL frames only, in RAW format\n");
	printf("\t             rd      Trace DATA frames only, in RAW format\n");
	//printf("\tFT1 Configuration\n");
	//printf("\t   T         v       View Status \n");
	//printf("\t             s       Self Test\n");
	//printf("\t             l       Line Loop Test\n");
	//printf("\t             d       Digital Loop Test\n");
	//printf("\t             r       Remote Test\n");
	//printf("\t             o       Operational Mode\n");
	printf("\tDriver Statistics\n");
	printf("\t   d         s       Display Send Driver Statistics\n");
	printf("\t             i       Display Interrupt Driver Statistics\n");
	printf("\t             g       Display General Driver Statistics\n");	
	printf("\tFlush Statistics\n");
	//printf("\t   f         g       Flush Global Statistics\n");
	printf("\t   f         c       Flush Communication Error Statistics\n");
        printf("\t             h       Flush HDLC Statistics\n");
	//printf("\t             i       Flush DLCI Statistics\n");
	printf("\t             d       Flush Driver Statistics\n");
	printf("\tExamples:\n");
	printf("\t--------\n\n");
	printf("\tex: wanpipemon -i wp1_svc1 -u 9000 -c xm   :View Modem Status \n");
	printf("\tex: wanpipemon -i 201.1.1.2 -u 9000 -c ti  :Trace and Interpret ALL frames\n");
	//printf("\tex: wanpipemon -i wp1_svc1 -u 9000 -c sd -l 1 :Statistics for LCN 1 \n\n");
	return 0;

}; //usage

static char *gui_main_menu[]={
"card_stats_menu","Card Status",
"stats_menu", "Card Statistics",
"trace_menu", "Trace Data",
"driver_menu","Driver Statistics",
"flush_menu","Flush Statistics",
"."
};

static char *card_stats_menu[]={
"xm","Modem Status",
"xl","Link Status",
"xru","Display Router UP time",
"."
};


static char *stats_menu[]={
"sc","Communication Error Statistics",
"sx","X25 Statistics",
"sh","HDLC Statistics",
"."
};

static char *trace_menu[]={
"ti","Trace and Interpret ALL frames",
"tr","Trace ALL frames, in RAW format",
"."
};

static char *driver_menu[]={
"ds","Display Send Driver Statistics",
"di","Display Interrupt Driver Statistics",
"dg","Display General Driver Statistics",	
"."
};

static char *flush_menu[]={
"fc","Flush Communication Error Statistics",
"fh","Flush HDLC Statistics",
"fd","Flush Driver Statistics",
"."
};

static struct cmd_menu_lookup_t gui_cmd_menu_lookup[]={
	{"card_stats_menu",card_stats_menu},
	{"stats_menu",stats_menu},
	{"trace_menu",trace_menu},
	{"driver_menu",driver_menu},
	{"flush_menu",flush_menu},
	{".",NULL}
};


char ** X25get_main_menu(int *len)
{
	int i=0;
	while(strcmp(gui_main_menu[i],".") != 0){
		i++;
	}
	*len=i/2;
	return gui_main_menu;
}

char ** X25get_cmd_menu(char *cmd_name,int *len)
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



int X25Main(char *command,int argc, char* argv[])
{
	char *opt=&command[1];

	ResetWanUdp(&wan_udp);  
	
	switch(command[0]){

		case 'x':
			if (!strcmp(opt,"m")){
				modem_status();
			}else if (!strcmp(opt, "l")){
				link_status();
			}else if (!strcmp(opt, "ru")){
				x25_router_up_time();
			}else{
				printf("ERROR: Invalid Status Command 'x', Type wanpipemon <cr> for help\n\n");
			}
			break;
		case 's':
			if (!strcmp(opt,"c")){
				comm_err();
			}else if (!strcmp(opt,"x")){
				read_x25_statistics();
		//	}else if (!strcmp(opt,"e")){
		//		error_stats();
			}else if (!strcmp(opt,"h")){
				read_hdlc_stat();
			}else {
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

		case 'c':
			//if (!strcmp(opt, "l")){
			//	list_dlcis();
			//}else{
				printf("ERROR: Invalid Configuration Command 'c', Type wanpipemon <cr> for help\n\n");

			//}
			break;
		case 'd':
				/* Different signature for Driver Statistics */
			if(!strcmp(opt, "s")){
				x25_driver_stat_ifsend();
			}else if (!strcmp(opt, "i")){
				x25_driver_stat_intr();
			}else if (!strcmp(opt, "g")){
				x25_driver_stat_gen();
			}else{
				printf("ERROR: Invalid Driver Statistic 'd', Type wanpipemon <cr> for help\n\n");
			}
			break;
		case 'f':
			if (!strcmp(opt, "c")){
				flush_comm_err();
				comm_err();
			}else if (!strcmp(opt, "h")){
				flush_hdlc_stat();
				read_hdlc_stat();
		//	}else if (!strcmp(opt, "e")){
		//		flush_global_stats();
		//		error_stats();
		//	}else if (!strcmp(opt, "i")){
		//		flush_dlci_stats();
		//		read_dlci_stat();
			}else if (!strcmp(opt, "d")){
				flush_driver_stats();
			}else{
				printf("ERROR: Invalid Flush Command 'f', Type wanpipemon <cr> for help\n\n");
			}
			break;
	/*     			case 'T':

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
					FT1_ll_test();
				}
				set_FT1_monitor_status(0x00);
			}else if (!strcmp(opt, "d")){
				set_FT1_monitor_status(0x01);
				if(!fail){
					FT1_dl_test();
				}
				set_FT1_monitor_status(0x00);
			}else if (!strcmp(opt, "r")){
				set_FT1_monitor_status(0x01);
				if(!fail){
					FT1_rt_test();
				}
				set_FT1_monitor_status(0x00);	
			}else if (!strcmp(opt, "o")){
				set_FT1_monitor_status(0x01);
				if(!fail){
					FT1_operational_mode();
				}
				set_FT1_monitor_status(0x00);
			}else{

				printf("ERROR: Invalid FT1 Command 'T', Type wanpipemon <cr> for help\n\n");

			}
			break;	
	*/
		default:
			printf("ERROR: Invalid Command, Type wanpipemon <cr> for help\n\n");

	} //switch

   	printf("\n");
	return 0;
}; //main

/*
 * EOF wanpipemon.c
 */
