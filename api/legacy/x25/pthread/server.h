#ifndef _SERVER_H
#define _SERVER_H

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <time.h>
#include <semaphore.h>
#include <pthread.h>
#include <netinet/in.h>
#include <linux/if_wanpipe.h>
#include <string.h>
#include <errno.h>
#include <arpa/inet.h>
#include <sys/poll.h>
#include <linux/wanpipe.h>
#include <linux/sdla_x25.h>
#include <linux/if_ether.h>

#include "bitmap.h"

#define INVALID_SOCKET  -1
#define ERROR 		-1
#define SOCKET_ERROR    -1
#define TRUE 		1
#define FALSE		0	
#define MAX_CHNL 	128	
#define STACKSIZE 	0x200000		// 2 Mb  STACK SIZE
#define BUFFER_SIZE 	1024	
#define MAX_FRM_LGTH   	1008 
#define WAIT_FOR_A_WHILE 2
				// thread declarations 

void* process_con(void* arg);
void* switch_recv(void* threadID);

				// function declarations

int connect_switch(int switch_num,int which);
static void cleanup(int signo);
void close_connection(int,fd_set);
void printing(unsigned char*,int);
int find_separator(char*ptr);
int accept_call (int sock, int chnl, x25api_t *api_cmd);
void handle_oob_event(x25api_t* api_hdr, int sk_index);



				// variable declarations

int sock[MAX_CHNL+1];
enum { BUSY = 0 , R_CALL  = 1};
int chnl_flag[MAX_CHNL+1];
int switch_socket[MAX_CHNL+1];
sem_t semaphore[MAX_CHNL+1],switch_up[MAX_CHNL+1];
char called_address[MAX_CHNL+1][15];
pthread_t threadID[MAX_CHNL+1];
pthread_t threadID1[MAX_CHNL+1];
int chnl_c[MAX_CHNL+1],soc;              
struct sockaddr_in ClientAddr,ClientAddr2; 
BitMap *status,*sock_dyn_status;
int X25_HDR,who = 0;
char SWITCH06[] = "10.1.3.29";	 //switch06-nt.qs1.com
char SWITCH10[] = "10.1.3.33";	//switch10-nt.qs1.com
FILE *fd;

#endif // _SERVER_H

