/*****************************************************************************
* pos_api.c	POS API
*
* Author(s):	Nenad Corbic <ncorbic@sangoma.com>
*
* Copyright:	(c) 2003 Sangoma Technologies Inc.
*
*		This program is free software; you can redistribute it and/or
*		modify it under the terms of the GNU General Public License
*		as published by the Free Software Foundation; either version
*		2 of the License, or (at your option) any later version.
* ============================================================================
* Description:
*
* 	The pos_api.c utility interfaces to the POS kernel driver
* 	via socket ioctl() calls.
*
* 	Please refer to the POS API document that conatins 
* 	detailed information on all POS commands.
*
* 	Please use the program as a building block for your POS
* 	application.
*
*	This example has been written for a single interface in mind, 
*	where the same process handles tx and rx data.
*
* 
*/

/* General Linux header files */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <linux/if.h>

/* Wanpipe header files */
#include <linux/wanrouter.h>
#include <linux/wanpipe.h>
#include <linux/sdla_pos.h>
#include <linux/if_wanpipe.h>


/*===================================================
 * Defines
 *==================================================*/

#define FALSE	0
#define TRUE	1

#define LGTH_CRC_BYTES	2
#define MAX_TX_DATA     50	/* Size of tx data */  
#define MAX_FRAMES 	5000     /* Number of frames to transmit */  

#define POS_UDELAY	100000 /* 100ms */

#define POS_PORT_ENABLE   0
#define POS_PORT_DISABLE -1

#define is_port_ready(x)  ((x>0)?1:0) 

#define POS_ASYNC_PORT1	1
#define POS_ASYNC_PORT2 2

#define POS_PORT1	1
#define POS_PORT2	2


/*===================================================
 * Golobal data
 *==================================================*/

unsigned short Rx_lgth;
unsigned char Rx_data[4000];
unsigned char Tx_data[MAX_TX_DATA + sizeof(wan_api_hdr_t)];
int 	sock;

/* Structure used to execute POS commands
 * 
 * struct {
 * 	unsigned char wan_pos_opp_flag
 * 	unsigned char wan_pos_pos_state
 *      unsigned char wan_pos_async_state
 *      unsigned char wan_pos_command		
 *      unsigned short wan_pos_data_len
 *      unsigned char wan_pos_return_code		
 *      unsigned char wan_pos_port_num	
 *      unsigned char wan_pos_attr		
 *      unsigned char wan_pos_data[1030]	
 * }wan_mbox_t
 *
 * NOTE: This is the representation of the wan_mbox_t
 *       structure. Note how its actually defined.
 *       It should be used for information purposes 
 *       only.  i.e. DO NOT TRY TO USE THIS STRUCTURE 
 *                   DEFINITION IN THE ACTUAL CODE :)
 */
static wan_mbox_t mbox_g;


/*=================================================== 
 * Function Prototypes 
 *==================================================*/
int MakeConnection(wan_mbox_t *, char *, char *);
void process_con(wan_mbox_t *);
int pos_data_ready(int sock, wan_mbox_t *mbox, 
		   short *rx_port1, short *rx_port2, 
		   short *rx_async_port1, short *tx_async_port1,
		   short *rx_async_port2, short *tx_async_port2,
		   unsigned int udelay);


/*===================================================
 * DoCommand
 *
 * 	Execute a POS command directly on the
 * 	sangoma adapter. 
 * 
 */

int DoCommand (wan_mbox_t *mbox)
{
	return ioctl(sock, SIOC_WANPIPE_EXEC_CMD, mbox);
}

/*===================================================
 * configure_pos
 *
 * 	Setup a POS configuration structure and
 * 	execute a POS CONFIGURATION command.
 *
 * IMPORTANT:
 * 	The current configuration is only a 
 * 	SAMPLE.  It should be changed to sute
 * 	your network.
 * 
 */


int configure_pos (wan_mbox_t *mbox)
{
	CONFIGURATION_STRUCT cfg;

	mbox->wan_pos_command=CONFIGURE;
	mbox->wan_pos_port_num=0;

	memset(&cfg, 0, sizeof(CONFIGURATION_STRUCT));

	/* number of active SDLC lines    */
	cfg.sdlc_lines=0x02;
	

	/* maximum tx data packet */
	cfg.sdlc_maxdata = DFT_POSDATA;
	
	/* number of active ASYNC lines   */
	cfg.async_lines=0x02;
	
	/* asynchronous line(s) speed     */
 	cfg.async_speed=0x03; /* 19200 bps*/
	
	/* half/full-duplex configuration 
	 * 1=Full Duplex
	 * 0=Half Duplex */
	cfg.half_duplex = 1;

	memcpy(mbox->wan_pos_data,&cfg,sizeof(CONFIGURATION_STRUCT));
	mbox->wan_pos_data_len=sizeof(CONFIGURATION_STRUCT);

	if (DoCommand(mbox) != 0){
		printf("Failed to configure pos!\n");
		return FALSE;
	}

	printf("Pos Configured\n");


	/* Enable POS Port 1 */
	memset(mbox,0, 0x16);
	
	mbox->wan_pos_command=ENABLE_POS;
	mbox->wan_pos_data_len=0;
	mbox->wan_pos_port_num=1;

	if (DoCommand(mbox) != 0){
		printf("Failed to enable port 1!\n");
		return FALSE;
	}

	
	memset(mbox,0, 0x16);

	/* Enable POS Port 2 */
	mbox->wan_pos_command=ENABLE_POS;
	mbox->wan_pos_data_len=0;
	mbox->wan_pos_port_num=2;

	if (DoCommand(mbox) != 0){
		printf("Failed to enable port 2!\n");
		return FALSE;
	}

	printf("Enabled Port 1 and 2\n");

	return TRUE;
}


/*===================================================
 * MakeConnection
 *
 *   o Create a Socket
 *   o Bind a socket to a wanpipe network interface
 *     (Interface name is supplied by the user)
 *   
 *   The "sock" socket descriptor will be use by 
 *   the rest of the program to execute ioctl()
 *   commands down to the kernel driver.
 *==================================================*/         

int MakeConnection(wan_mbox_t *mbox, char *r_name, char *i_name ) 
{
	struct wan_sockaddr_ll 	sa;

	memset(&sa,0,sizeof(struct wan_sockaddr_ll));
	errno = 0;
   	sock = socket(AF_WANPIPE, SOCK_RAW, 0);
   	if( sock < 0 ) {
      		perror("Socket");
      		return( FALSE );
   	} /* if */
  
	printf("\nConnecting to router %s, interface %s\n", r_name, i_name);
	
	strcpy( sa.sll_device, i_name);
	strcpy( sa.sll_card, r_name);
	sa.sll_protocol = htons(PVC_PROT);
	sa.sll_family=AF_WANPIPE;

        if(bind(sock, (struct sockaddr *)&sa, sizeof(struct wan_sockaddr_ll)) < 0){
                perror("bind");
		printf("Failed to bind a socket to %s interface\n",i_name);
                exit(0);
        }
	printf("Socket bound to %s\n\n",i_name);

	memset(mbox,0, 0x16);

	return configure_pos(mbox);
}

/*===================================================
 * pos_receive_data
 *
 *	Execute POS receive command for a 
 *	specific PORT.
 *
 *	Copy the rx packet into the user supplied
 *	Rx_data buffer if the rx command is
 *	successful.
 *==================================================*/         

int pos_receive_data(wan_mbox_t *mbox, char cmd, char port, unsigned char *Rx_data)
{
	memset(mbox,0, 0x16);
	mbox->wan_pos_command = cmd;
	mbox->wan_pos_data_len = 0;
	mbox->wan_pos_port_num = port;

	if (DoCommand(mbox) != 0){
		printf("Failed to receive on %s port %i: %s!\n", 
				cmd==RECEIVE_POS?"POS":"Async",
				port,
				strerror(errno));
		return -EINVAL;
	}

	memcpy(Rx_data, mbox->wan_pos_data, mbox->wan_pos_data_len);

	/* Debugging:
	 * Prints out the received packet in HEX or ASCII */
	{
		int i;
		printf("DATA: ");
		for (i=0;i<mbox->wan_pos_data_len;i++){

			//Print in HEX
			printf("0x%02X ",Rx_data[i]);

			//Print in ASCII
			//printf("%c ",Rx_data[i]);
		}
		printf("\n\n");
	}
	
	return mbox->wan_pos_data_len;
}

/*===================================================
 * pos_send_data
 *
 *	Execute POS send command for a 
 *	specific ASYNC TX PORT.
 *
 *==================================================*/
int pos_send_data(wan_mbox_t *mbox, char port, unsigned char *Tx_data, int Tx_len)
{
	memset(mbox,0, 0x16);

	mbox->wan_pos_command = SEND_ASYNC;
	mbox->wan_pos_data_len = Tx_len;
	mbox->wan_pos_port_num = port;

	memcpy(mbox->wan_pos_data,Tx_data,Tx_len);
	
	if (DoCommand(mbox) != 0){
		printf("Failed to tx on Async port %i: %s!\n", 
				port,
				strerror(errno));
		return -EINVAL;
	}

	return Tx_len;
}


/*===========================================================
 * process_con 
 *
 *   o Setup the tx and rx buffers
 *   
 *   o Poll the POS ports 1 and 2 and POS ASYNC ports 1 and 2
 *     Note, that one must select which port the polling
 *     routine should poll on.
 *     
 *   o Receive and Tx
 *
 * 
 *   IMPORTANT:
 *     On TX if the send command fails, one must 
 *     retransmit the same buffer again.
 */
void process_con(wan_mbox_t *mbox) 
{
	unsigned int Rx_count,Tx_count,Tx_length;
	short rx_port1,rx_port2,
	      rx_async_port1,tx_async_port1,
	      rx_async_port2,tx_async_port2;
	int err,i;

        Rx_count = 0;
	Tx_count = 0;
	Tx_length = MAX_TX_DATA;

	memset(&Tx_data[0],0,MAX_TX_DATA);


	/* Create a data packet */
	for (i=0;i<Tx_length;i++){
		Tx_data[i] = (unsigned char)1;
	}

	printf("Starting Pos Tx/Rx, POS_UDELAY=%i \n",POS_UDELAY);
	
	for(;;) {	

		/* Indicate which on which ports we'd like to tx or
		 * receive. Note port1 and port2 are receive only.
		 *
		 * The pos_data_ready() function will use these values
		 * to determin what data the user is waiting for.  
		 *
		 * POS_PORT_ENABLE: Indicates that we'd like to be
		 *                  woken up when data is available.
		 *
		 * POS_PORT_DISABLE:Ignore data on this port
		 *
		 * In this case we would like to rx/tx data on
		 * all available ports
		 */
		rx_port1=POS_PORT_ENABLE;
		rx_port2=POS_PORT_ENABLE;
	        rx_async_port1=POS_PORT_ENABLE;
		tx_async_port1=POS_PORT_DISABLE;
		rx_async_port2=POS_PORT_ENABLE;
		tx_async_port2=POS_PORT_DISABLE;

		
		/* The pos_data_ready() function must be used to implement flow control.
	         * It will block using the POS_UDELAY if no data is available, before
		 * POS buffers are re-checked.
		 */ 

		if(pos_data_ready(sock,mbox,
				  &rx_port1,&rx_port2,
				  &rx_async_port1,&tx_async_port1,
				  &rx_async_port2,&tx_async_port2,
				  POS_UDELAY) == 0){

			/* Rx packets on PORT 1*/
                   	if (is_port_ready(rx_port1)){
				err = pos_receive_data(mbox,RECEIVE_POS, POS_PORT1,Rx_data);
				/* err indicates bytes received */
				if(err > 0) {
					printf("Received packet %i, Port %i Length = %i\n", 
							++Rx_count,
							POS_PORT1,
							err);
				} 
			 } 
		  
			/* Rx packets on PORT 2 */
		   	if (is_port_ready(rx_port2)){
				err = pos_receive_data(mbox,RECEIVE_POS, POS_PORT2,Rx_data);
				if (err > 0){

					printf("Received packet %i, Port %i Length = %i\n", 
							++Rx_count,
							POS_PORT2,
							err);
				}
		   	}

			/* Rx packets on Async PORT 1 */
                   	if (is_port_ready(rx_async_port1)){
				err = pos_receive_data(mbox,RECEIVE_ASYNC, POS_ASYNC_PORT1,Rx_data);
				if(err > 0) {
					printf("Received packet %i, Async Port %i Length = %i\n", 
							++Rx_count,
							POS_ASYNC_PORT1,
							err);
				} 
			 } 

			/* Tx packets on Async PORT 1 */
                   	if (is_port_ready(tx_async_port1)){

				if (tx_async_port2 < Tx_length){
					printf("Tx failed on Async port not enough room %i<%i\n",
							tx_async_port2,Tx_length);
				}else{
					err = pos_send_data(mbox,POS_ASYNC_PORT1,Tx_data,Tx_length);
					if(err > 0) {
						printf("Tx packet %i, Async Port %i Length = %i\n", 
								++Tx_count,
								Tx_length,
								POS_ASYNC_PORT1);
					}else{
						//printf("Tx failed on Async port %i\n",
						//		POS_ASYNC_PORT1);
					}
				}
			 } 

			/* Rx packets on Async PORT 2 */
                   	if (is_port_ready(rx_async_port2)){
				err = pos_receive_data(mbox,RECEIVE_ASYNC, POS_ASYNC_PORT2,Rx_data);
				if(err > 0) {
					printf("Received packet %i, Async Port %i Length = %i\n", 
							++Rx_count,
							POS_ASYNC_PORT2,
							err);
				} 
			 } 

			/* Tx packets on Async PORT 2 */
                   	if (is_port_ready(tx_async_port2)){

				if (tx_async_port2 < Tx_length){
					printf("Tx failed on Async port 2 not enough room %i<%i\n",
							tx_async_port2,Tx_length);
				}else{
				
					err = pos_send_data(mbox,POS_ASYNC_PORT2,Tx_data,Tx_length);
					if(err > 0) {
						printf("Tx packet %i, Async Port %i Length = %i\n", 
								++Tx_count,
								Tx_length,
								POS_ASYNC_PORT2);
					}else{
						//printf("Tx failed on Async port %i\n",
						//		POS_ASYNC_PORT2);
					}
				}
			} 

		   	/* Stop the program afer transmitting MAX_FRAMES */
			if (Tx_count == MAX_FRAMES)
				break;

		} else {
                	printf("\nError selecting socket\n");
			break;
		}
	}
	close (sock);
} 

/*===========================================================
 * pos_data_ready 
 *
 *  	Execute the POS STATUS command and check if
 *  	there is any data on each of the POS ports.
 *
 *  	We also check the user defined ports we
 *  	should be waiting on.  The port must
 *  	be enabled and data must be found on that
 *  	port before we exit and indicate event.
 */


int pos_data_ready(int sock, wan_mbox_t *mbox, 
		   short *rx_port1, short *rx_port2, 
		   short *rx_async_port1, short *tx_async_port1,
		   short *rx_async_port2, short *tx_async_port2,
		   unsigned int udelay)
{
	int err;
	struct timeval tv;
	POSSTATESTRUC *tmp_pos_state;
	
	for (;;){
		memset(mbox,0, 0x16);

		mbox->wan_pos_command = SEND_RECV_STATE;
		mbox->wan_pos_data_len = 0;

		if ((err=DoCommand(mbox)) != 0){
			printf("Failed to get pos status info %x!\n",err);
			return -EINVAL;
		}

		tmp_pos_state = (POSSTATESTRUC*)mbox->wan_pos_data;	

		if (*rx_port1==POS_PORT_ENABLE && tmp_pos_state->POS1_received){
			*rx_port1=tmp_pos_state->POS1_received;
			udelay=0;
		}
		
		if (*rx_port2==POS_PORT_ENABLE && tmp_pos_state->POS2_received){
			*rx_port2=tmp_pos_state->POS2_received;
			udelay=0;
		}
		
		if (*rx_async_port1==POS_PORT_ENABLE && tmp_pos_state->async1_recvd){
			*rx_async_port1=tmp_pos_state->async1_recvd;
			udelay=0;
		}

		if (*tx_async_port1==POS_PORT_ENABLE && tmp_pos_state->async1_transm){
			*tx_async_port1=tmp_pos_state->async1_transm;
			udelay=0;
		}
		
		if (*rx_async_port2==POS_PORT_ENABLE && tmp_pos_state->async2_recvd){
			*rx_async_port2=tmp_pos_state->async2_recvd;
			udelay=0;
		}

		if (*tx_async_port2==POS_PORT_ENABLE && tmp_pos_state->async2_transm){
			*tx_async_port2=tmp_pos_state->async2_transm;
			udelay=0;
		}

		/* Something has been found, or the user has put
		 * in a ZERO delay which means that we shouldn't
		 * block */
		if (!udelay){
#if 0
			printf("POS State: PP1=%d, PP2=%d, APr1=%d, APt1=%d, APr2=%d, APt2=%d\n",
					tmp_pos_state->POS1_received,
					tmp_pos_state->POS2_received,
					tmp_pos_state->async1_recvd,
					tmp_pos_state->async1_transm,
					tmp_pos_state->async2_recvd,
					tmp_pos_state->async2_transm);
#endif					
			
			break;
		}
		
		/* Nothing found on any of the ports,
		 * sleep for user defined time and
		 * proceed with polling */
		tv.tv_usec = udelay; 
		tv.tv_sec = 0;

		select(sock+1, NULL,NULL,NULL,&tv);
	}

	return 0;
}


/***************************************************************
 * Main:
 *
 *    o Make a socket connection to the driver.
 *    o Call process_con() to read/write the POS API 
 *
 **************************************************************/


int main(int argc, char* argv[])
{
	int proceed;

	if (argc != 3){
		printf("Usage: rec_wan_sock <router name> <interface name>\n");
		exit(0);
	}	
	
	memset(&mbox_g,0,sizeof(wan_mbox_t));
	
	proceed = MakeConnection(&mbox_g,argv[argc - 2], argv[argc - 1]);
	if( proceed == TRUE ){
		process_con(&mbox_g);
		return 0;
	}

	return 1;
};
