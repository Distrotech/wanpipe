/*****************************************************************************
* server.c	X25 API: SVC Server Application
*
* Author(s):	Nenad Corbic <ncorbic@sangoma.com>
*
* Copyright:	(c) 1995-2000 Sangoma Technologies Inc.
*
*		This program is free software; you can redistribute it and/or
*		modify it under the terms of the GNU General Public License
*		as published by the Free Software Foundation; either version
*		2 of the License, or (at your option) any later version.
* ============================================================================
* Due Credit :)
* 	This sample utility was based on real-world
* 	application written by: 
* 		Mauricio Rodriguez <Mauricio_Rodriguez@qs1.com>
* 
* Description:
*
* 	The server.c utility will accept, user defined number, of 
* 	outgoing calls and tx/rx, user defined number, of packets on each
* 	active svc.  
*
* 	This utility should be used as an architectual model.  It is up to
* 	the user to handle all conditions of x25.  Please refer to the
* 	X25API programming manual for futher details.
*
*/


#include "server.h"


/*=============================================================
 * Main:
 *
 *    o Make a listen socket connection to the driver.
 *    o Wait for the connection to come in.
 *    o Create a new socket for the incoming connection
 *      using the accept() system call.
 *    o Call the p-thread to handle the new connection
 *
 *============================================================*/   


int main(int argc, char **argv) 
{
	void* pchnl_num; 
	pthread_t threadID;                                                                                   
	int chnl,value,holdon = 0;
 	pthread_attr_t attr;
        size_t stacksize = STACKSIZE;
	sock_dyn_status= new BitMap(MAX_CHNL+1);
	status = new BitMap(MAX_CHNL+1);
	X25_HDR = sizeof(x25api_hdr_t);
	struct sigaction sa_old;
	struct sigaction sa_new;
	char ptr4[20];
	int err = 0,max_err = 0;
	int next_chnl = 1,counter;
	struct wan_sockaddr_ll sa;
	x25api_t api_cmd;
	void* ptr;
	char i_name[25];
	bool found = FALSE;
	fd_set  wait_for_call;

	errno = 0;

	if (argc != 2){
		printf("Usage: ./server wanpipe#  (#=1,2...)\n");
		exit(1);
	}

	/* User defined wanpipe device */
	strncpy(i_name,argv[argc-1],(sizeof(i_name)-1));

	
	/* Initialize the signal handler
	 * SIGINT = Ctrl C
	 * If the user terminates the program, the 
	 * signal handler will  cleanly close all sockets 
	 * before terminating.
	 */
	sa_new.sa_handler = cleanup;
	sa_new.sa_flags = 0;
	sigemptyset(&sa_new.sa_mask);
	sigaddset(&sa_new.sa_mask,SIGINT);
	sigaddset(&sa_new.sa_mask,SIGPIPE);
	sigaddset(&sa_new.sa_mask,SIGSEGV);
	if (sigaction(SIGSEGV,&sa_new,&sa_old) == ERROR)
		perror("main-sigaction");
	else
		puts("Success setting the SIGSEGV signal");
	if (sigaction(SIGINT,&sa_new,&sa_old) == ERROR)
		perror("main-sigaction");
	else
		puts("Success setting the signal SIGINT");

	
	/* Open a log file */
	if ((fd = fopen("server.log", "wr")) == NULL) {
        	fprintf(stderr, "Unable to open log\n");
        	exit(1);
  	}
	sprintf(ptr4,"Main PID = %d\n",getpid());
	fwrite(ptr4,1,20,fd);


  	printf("\n**************************************************\n");
        printf("*****  STARTING WANPIPE1 PORT1  LIVE SWITCH    ************\n");
        printf("**************************************************\n\n");

	/* Initialize P-Threads */
        if (pthread_attr_init(&attr))
                perror("main-pthread_attr_init");

	/* Initialize the thread stack size to 2MB
	 * The kernel defaults the stack size to 8MB, this
	 * however causes a problem when running 250+ threads.
	 * By setting the stack size lower, we can have more
	 * threads.  
	 *
	 * WARNING: You must ensure that all you local
	 * variables inside the thread do not exceed 2MB
	 * In this application 512 KB should be sufficient.
	 *
	 * By default this option is NOT enabled */
#ifdef STACK_SIZE_2MB
        if(pthread_attr_setstacksize(&attr,stacksize))
              perror("main-pthread_attr_setstacksize");
#endif

	/* For each channel setup semaphores that will 
	 * control each p-thread, and initialize the channel
	 * private data */
	for (chnl = 1; chnl <= MAX_CHNL; chnl++)
	{
		sem_init(&semaphore[chnl],0,0);
		sem_init(&switch_up[chnl],0,0);
		chnl_c[chnl] = chnl;
		chnl_flag[chnl] = R_CALL;
	}

	chnl_flag[0] = BUSY;


	/* For all possible channels (MAX_CHNL), create a p-thread.
	 * Each p-thread will execute the process_con()
	 * function. 
	 */
     	for (chnl = 1; chnl <= MAX_CHNL; chnl++)
        {
                pchnl_num = &chnl_c[chnl];
 
                value = pthread_create(
			&threadID, 		// threadID if success 
                        &attr,      		// thread attributes 
                        process_con,		// method to be run by this thread
                        pchnl_num);         	// argument for new thread

		if (value)
			perror("main-pthread_create,process_con");

        } // end of for loop

	
	if (sigaction(SIGPIPE,&sa_new,&sa_old) == ERROR)
		perror("main-sigaction");
	else
		puts("Success setting the SIGPIPE signal");

	memset(&sa,0,sizeof(struct wan_sockaddr_ll));

	/* Create a new X25API socket */

   	soc = socket(AF_WANPIPE, SOCK_RAW, 0);
   	if( soc == SOCKET_ERROR ) {
      		perror("server-Socket");
		exit(-1);
   	} 

	/* Fill in binding information. 
	 * Before we use listen() system call
         * a socket must be binded into a virtual
         * network interface svc_listen.
         * 
         * Card name must be supplied as well. In 
         * this case the user supplies the name
         * eg: wanpipe1.
         */ 

	sa.sll_family = AF_WANPIPE;
	sa.sll_protocol = htons(X25_PROT);
	strcpy((char*)sa.sll_device,"svc_listen");
	strcpy((char*)sa.sll_card,i_name);
		
	/* Bind a sock using the above address structure */

        if(bind(soc, (struct sockaddr *)&sa, sizeof(struct wan_sockaddr_ll)) == -1){
                perror("bind");
		printf("Failed to bind socket to %s interface\n",i_name);
                exit(0);
        }
	else
		printf("SUCCESSFULL BINDING \n");

	/* Put the sock into listening mode. Number 10 does not 
         * mean anything, since number of connections is depended
	 * on number of LCNs configured */

	listen(soc,SOMAXCONN);

	/* Start an infinite loop, and accept any incoming
	 * calls.  Once the call is received via accept() system
	 * call, signal the appropriate p-thread to handle 
	 * the call acceptance as well as tx and rx data.
	 */
	
	for (;;){

		found = FALSE;	
		holdon = 0;

		/* Make sure there are free channels available
		 * If there are not channels available, wait 300ms
		 * and retry again.  Once a channel is free, proceed
		 * to wait for an incoming connection */
 		while(!found) {
                        if (chnl_flag[(next_chnl%MAX_CHNL)] == R_CALL) {
                                chnl = next_chnl%MAX_CHNL;
                                found = TRUE;
				next_chnl++;
				continue;
                        }
			next_chnl++;
			if ((++holdon) > WAIT_FOR_A_WHILE){
				printf("Waiting until somebody frees a channel, should not be long...\n");
				holdon = 0;
				usleep(300);
			}
                }

		/* Use select() to wait for an incoming call */
		FD_ZERO(&wait_for_call);
		FD_SET(soc,&wait_for_call);
		if (select(soc + 1, &wait_for_call, NULL, NULL,NULL)){
			
			if (FD_ISSET(soc,&wait_for_call)){
				
				/* accept() will create a new socket for an
				 * incoming call.  We must parse the
				 * called and calling data and determine if we
				 * want to accept the call.  This will be done
				 * in a P-thread */
				sock[chnl] = accept(soc,NULL,NULL);
			}else{
				sock[chnl] == INVALID_SOCKET;
			}
		}else{
			/* Error in select() */
			sock[chnl] == INVALID_SOCKET;
		}

		if (sock[chnl] == INVALID_SOCKET) {
			printf("\nSERVER failing calls... exiting \n");
			exit(-1);
		}	


		/* The accept() call has created the new socket
		 * for an incoming call.  Use sem_pose() to
		 * wake up an appropriate thread, so it can
		 * handle the new connection.  The process_con()
		 * function is used to handle new connections.
		 */
		chnl_flag[chnl] = BUSY;

		if(sem_post(&semaphore[chnl]) == ERROR)
			perror("server:sem_post");

	}

	printf("*** SERVER thread has EXITED ... Terminating application .... \n");	

	return 0;
} // end of the main function





/***************************************************
*  process_con
*
*   o Send and Receive data via socket 
*   o Create a tx packet using x25api_t data type. 
*   o Both the tx and rx packets contains 16 bytes headers
*     in front of the real data.  It is the responsibility
*     of the applicatino to insert this 16 bytes on tx, and
*     remove the 16 bytes on rx.
*
*	------------------------------------------
*      |  16 bytes      |        X bytes        ...
*	------------------------------------------
* 	   Header              Data
*
*   o 16 byte Header: data structure:
*
*     	typedef struct {
*		unsigned char  qdm	PACKED;	Q/D/M bits 
*		unsigned char  cause	PACKED;	cause field 
*		unsigned char  diagn	PACKED; diagnostics 
*		unsigned char  pktType  PACKED; 
*		unsigned short length   PACKED;
*		unsigned char  result	PACKED;
*		unsigned short lcn	PACKED;
*		char reserved[7]	PACKED;
*	}x25api_hdr_t;
*
*	typedef struct {
*		x25api_hdr_t hdr	PACKED;
*		char data[X25_MAX_DATA]	PACKED;
*	}x25api_t;
*
* TX DATA:
* --------
* 	Each tx data packet must contain the above 16 byte header!
* 	
* 	Only relevant byte in the 16 byte tx header, is the
* 	QDM byte.  The driver will look at this byte to determine
* 	if the Mbit (more data bit) should be set.
* 		
*       QDM: byte is a bit map of three bits:
*
* 	Q bit: Bit 2 : Qualifier bit, special kind of packet.
*                      Can be used as control packet.  
*       D bit: Bit 1 : Data acknolwedge bit. The remote will
*                      acknowledge every packet sent.
*       M bit: Bit 0 : If your packet is greater than x25 MTU,
*                      i.e 1024 bytes, than cut the packet to MTU size
*                          and set the M bit to indicate more data.                
*
* RX DATA:
* --------
* 	Each rx data will contain the above 16 byte header!
*
* 	Relevant bytes in the 16 byte rx header, are the
* 	LCN and QDM bytes.
*
*	QDM: byte is a bit map of three bits:
*
* 	Q bit: Bit 2 : Qualifier bit, special kind of packet.
*                      Can be used as control packet.  
*       D bit: Bit 1 : Data acknolwedge bit. The remote will
*                      acknowledge every packet sent.
*       M bit: Bit 0 : If your packet is greater than x25 MTU,
*                      i.e 1024 bytes, than cut the packet to MTU size
*                          and set the M bit to indicate more data.  
*                          
* 	LCN = contains the lcn number for the rx frame.
* 	
* OOB DATA:
* ---------
* 	The OOB (out of band) data is used by the driver to 
* 	indicate x25 events for the active channel: 
* 	clear call, or restarts.
*
* 	Each OOB packet contains the above 16 byte header!
*
*	Relevant bytes in the 16 byte oob header, are the
*	pktType, cause and diagn bytes.
*
*	pktType = event type (ex Clear Call, Restart ...)
*	cause   = x25 cause of the event
*	diagn   = x25 diagnostic information used to determine
*	          the cause of the event.
*
*	Upon receiving an event, the sock should be considered 
*	DEAD!!!  Meaning it must be closed using the close()
*	function.
*/


void* process_con(void* threadID)
{
        int chnl =*(int *)threadID;
	int err,ready=0,i = 0;
	fd_set  readset,oobset,writeset;
	
	int bytes_read,bytes_sent;	
	unsigned char buffer[BUFFER_SIZE];
	unsigned char tx_buffer[BUFFER_SIZE];
	char ptr[20];
	struct timeval timeout;
        unsigned short Tx_lgth;
	
        x25api_t* api_rx_el;
	x25api_t* api_tx_el;
        x25api_hdr_t *api_data;
	
	void* ptr3;
	enum more {ON = 1,OFF = 0};
	x25api_t api_cmd;
	
	pthread_detach(pthread_self());		// free system resources as soon as it exits
	
	timeout.tv_sec = 1;
	timeout.tv_usec = 0;

	sprintf(ptr,"pro_con PID = %d\n",getpid());
	fwrite(ptr,1,20,fd);

	/* Initialize and create a tx packet */
	memset(tx_buffer,0,sizeof(tx_buffer));

	api_tx_el=(x25api_t*)tx_buffer;

	/* This is how to set more bit
	 * for the current packet */
#ifdef TX_MORE_BIT_SET
	api_tx_el->hdr.qdm = 0x01
#endif
		
	for (i=0; i<Tx_lgth ; i++){
               	api_tx_el->data[i] = (unsigned char) i;
	}

	
	/* Each P-thread will call this function */

	for(;;)	{

waiting:
		/* Wait for a call to be detected by the main process.
		 * Once a new socket is created by the accept() system call.
		 * the main process will wake us up.
		 */
		sem_wait(&semaphore[chnl]);			// is connection UP ?

		/* The call is currently pending. Thus, check the 
		 * x25 address information and accept the call */
		if (accept_call (sock[chnl], chnl, &api_cmd) != 0)
			goto waiting;
		
		sock_dyn_status->Mark(chnl);	
		status->Mark(chnl);	

                FD_ZERO(&readset);
                FD_ZERO(&oobset);
		FD_ZERO(&writeset);

		for(;;) {

                	FD_SET(sock[chnl],&readset);
                	FD_SET(sock[chnl],&oobset);
#ifdef ENABLE_WRITE
			FD_SET(sock[chnl],&writeset);
#endif			


			/* The select function must be used to implement flow control.
			 * WANPIPE socket will block the user if the socket cannot send
			 * or there is nothing to receive.
			 *
			 * By using the last socket file descriptor +1 select will wait
			 * for all active x25 sockets.
			 *
			 * If the NONBLOCKING option has been used during connect()
			 * the select will wait untill the channel is connected 
			 * (i.e. accept has been received).  Once the channel is connected
			 * the tx and rx will start.  However, if the call is cleared for
			 * any reason, the OOB message will indicate that event.
			 */ 

			
			ready = select(sock[chnl]+1,&readset,&writeset,&oobset,&timeout);

			if(ready) { 	

				/* First check for OOB messages */

				if (FD_ISSET(sock[chnl], &oobset)){
				
					/* The OOB Message will indicate that an 
					 * asynchronous event occured.  The applicaton 
					 * must stop everything and check the state of 
					 * the link.  Since link might  
					 * have gone down */

					/* In this example I just exit. A real 
					 * application would check the state of the 
					 * sock first by reading the header information. 
					 */

					/* IMPORTANT:
					 * If we fail to read the OOB message, we can 
					 * assume that the link is down.  Thus, close 
					 * the socket and continue !!! */
					
					bytes_read = recv(sock[chnl],(char*)&buffer,BUFFER_SIZE,MSG_OOB);
                                	
					if(bytes_read < 0 ) {
						/* The state of the socket is disconnected.
						 * We must close the socket and continue with
						 * operation */
						printf("# %d has been CLEARED OOB FAILURE\n",chnl);
						close_connection(chnl,readset);
						goto waiting;
					}

                                        api_rx_el = (x25api_t *)buffer;
					

					handle_oob_event(api_rx_el, chnl);
					
					memset(&api_cmd, 0, sizeof(x25api_t));
					if(ioctl(sock[chnl],SIOC_WANPIPE_SOCK_STATE,&api_cmd) == 1){ // Disconnected !
						close_connection(chnl,readset);
						goto waiting;
					}

					/* Do what ever you have to do to handle
					 * this condiditon */


			    	} // end of oobset read


				
				/* Check for RX data  */

				if (FD_ISSET(sock[chnl], &readset)) {   
	
					bytes_read = recv(sock[chnl],(char*)&buffer,BUFFER_SIZE,MSG_NOSIGNAL);
					switch(bytes_read)
					{
						case SOCKET_ERROR: case 0: 
							printf("# %d SOCKET_ERROR ....\n",sock[chnl]);
							close_connection(chnl,readset);
							goto waiting;

						default:

							/* Packet received OK !
							 *
							 * Every received packet comes with the 16
							 * bytes of header which driver adds on. 
							 * Header is structured as x25api_hdr_t. 
							 * (same as above)
							 */
	
           						api_data = (x25api_hdr_t *)buffer;
						
							if (api_data->qdm & 0x01){
								/* More bit is set, thus
								 * handle it accordingly */
							}
						
							printf("Rx: Size=%i, Mbit=%i, Lcn=%i\n",
								bytes_read,
								(api_data->qdm & 0x01),
								api_data->lcn);
					} // end of switch

				}
			
				/* Check if the socket is ready for writing */
				if (FD_ISSET(sock[chnl], &writeset)) { 
					 
					/* Socket is ready to tx data */

					/* The tx packet length contains the 16 byte
					 * header. The tx packet was created above. */
					
					bytes_sent = send(sock[chnl], tx_buffer, 
						         Tx_lgth + sizeof(x25api_hdr_t), 0);

					/* err contains number of bytes transmited */		
					if (err>0){
						printf("SockId=%i : TX\n",
								sock[chnl]);
					}				
					
					/* If err<=0 it means that the send failed and that
					 * driver is busy.  Thus, the packet should be
					 * requeued for re-transmission on the next
					 * try !!!! 
					 */
				}

			}	// end of ready 
			else {

				/* select() timeout condition */
				timeout.tv_sec = 1 ; 
				if(!sock_dyn_status->Test(chnl)) {
					memset(&api_cmd, 0, sizeof(x25api_t));
					api_cmd.hdr.qdm = 0x00;
					ioctl(sock[chnl],SIOC_WANPIPE_CLEAR_CALL,&api_cmd);
					close_connection(chnl,readset);
					goto waiting;
				}
			}

		}// end of the inner for statement

	}// end of the outter for statement

exiting:

	printf("%d is exiting ...\n",getpid());
	return ptr3;

} // end of the process_con 

void close_connection(int chnl,fd_set readset)
{
	if(close(sock[chnl]) == ERROR)
		perror("process_con-closeDynastarsocket");
	FD_CLR(sock[chnl],&readset);
	sock_dyn_status->Clear(chnl);
	status->Clear(chnl);
	chnl_flag[chnl] = R_CALL;
}


static void cleanup(int signo)
{
	int static count = 0,count1 = 0;

	switch(signo){

	case SIGPIPE:

		printf("SIGPIPE signal raised by process PID = %d \n",getpid());
		exit(-1);
		break;
			
	case SIGSEGV:

		printf("SIGSEGV signal raised by process PID = %d \n",getpid());
		fclose(fd);
		exit(-1);
		break;

	case SIGINT:

		if (++count == 1) {
			//printf("%d CLEANING upon Ctrl-C ...\n",getpid());
			for(int chnl = 1 ; chnl < MAX_CHNL; chnl++)
			if (status->Test(chnl))
			{
				printf("%d closing open sockets \n");
				close(sock[chnl]);
			}
			close(soc);
			fclose(fd);
			printf("EXITING ...\n");
        		exit(-1);
		}

	} // end of the switch

} // end of the cleanup method



int accept_call (int sock, int chnl, x25api_t *api_cmd)
{
	int err;
	int counter;

	/* Reset the api command structure */
	memset(api_cmd, 0, sizeof(x25api_t));

	/* The GET CALL DATA ioctl call will write in the 
	 * incoming call data into api_cmd structure. Once
	 * we have call data information, it is up to the 
	 * user to accept or clear call. */
	
	if ((err=ioctl(sock,SIOC_WANPIPE_GET_CALL_DATA,api_cmd)) < 0){
		printf ("Get Call Data Failed %i\n",err);
		close(sock);
		return -EINVAL;
	}else{
			printf("SockId=%d : Incoming Call on Lcn %i\n",chnl,api_cmd->hdr.lcn);
	}


	/* Copy the incoming called address 
	 * The called address format is: 
	 * -d<called address> -s<calling address> -u<user data>
	 * In this case we are just checking the called address */ 
	memset(&called_address[chnl][0],0x0,15);

	counter = -1;
	while(++counter < min(api_cmd->hdr.length,15)){	
		if (api_cmd->data[2+counter] == ' ')
			break;
		
		called_address[chnl][counter] = api_cmd->data[2+counter];
	}

	//printf ("Incoming Called Address : %s\n",counter,called_address[chnl]);
	
	/* In this case I don't care about the called address,
	 * I will accept any call, thus proceed to accept the call */
	if ((err=ioctl(sock,SIOC_WANPIPE_ACCEPT_CALL,0)) != 0){
		printf("Accept failed err=%i lcn=%i\n",err,api_cmd->hdr.lcn);
		close(sock);
		return -EINVAL;
	}

	return 0;
}


void handle_oob_event(x25api_t* api_hdr, int sk_index)
{

	switch (api_hdr->hdr.pktType){

		case ASE_RESET_RQST:
			printf("SockId=%i : OOB : Rx Reset Call : Lcn=%i : diag=0x%02X : cause=0x%02X\n",
					sock[sk_index],
					api_hdr->hdr.lcn,
					api_hdr->hdr.diagn,
					api_hdr->hdr.cause);

			/* NOTE: we don't have to close the socket,
			 * since the reset doesn't clear the call
			 * however, it means that there is something really
			 * wrong and that data has been lost */
			return;
		
		
		case ASE_CLEAR_RQST:
			printf("SockId=%i : OOB : Rx Clear Call : Lcn=%i : diag=0x%02X : cause=0x%02X\n",
					sock[sk_index],
					api_hdr->hdr.lcn,
					api_hdr->hdr.diagn,
					api_hdr->hdr.cause);
			break;		
			
		case ASE_RESTART_RQST:
			printf("SockId=%i : OOB : Rx Restart Req : Lcn=%i : diag=0x%02X : cause=0x%02X\n",
					sock[sk_index],
					api_hdr->hdr.lcn,
					api_hdr->hdr.diagn,
					api_hdr->hdr.cause);
			
			break;
		case ASE_INTERRUPT:
			printf("SockId=%i : OOB : Rx Interrupt Req : Lcn=%i : diag=0x%02X : cause=0x%02X\n",
					sock[sk_index],
					api_hdr->hdr.lcn,
					api_hdr->hdr.diagn,
					api_hdr->hdr.cause);
			
			break;


		default:
			printf("SockId=%i : OOB : Rx Type=0x%02X : Lcn=%i : diag=0x%02X : cause=0x%02X\n",
					sock[sk_index],
					api_hdr->hdr.pktType,
					api_hdr->hdr.lcn,
					api_hdr->hdr.diagn,
					api_hdr->hdr.cause);
			break;
	}
}

