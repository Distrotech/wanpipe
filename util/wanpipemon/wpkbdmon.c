/*****************************************************************************
* wpkbdmon.c    Keyboard Led Debugger/Monitor
*
* Author:       Nenad Corbic <ncorbic@sangoma.com>	
*
* Copyright:	(c) 2000-2001 Sangoma Technologies Inc.
*
*		This program is free software; you can redistribute it and/or
*		modify it under the terms of the GNU General Public License
*		as published by the Free Software Foundation; either version
*		2 of the License, or (at your option) any later version.
* ----------------------------------------------------------------------------
* Nov 13, 2000	Nenad Corbic	Initial version based on kblights() program
* 				written by Joseph Nicholas.
*****************************************************************************/

#include <stdio.h>                        //printf()
#include <stdlib.h>
#include <sys/types.h>                    //open()
#include <sys/stat.h>                     //open()
#include <fcntl.h>                        //open()
#include <sys/ioctl.h>                    //ioctl()
#include <linux/kd.h>                     //KDSETLED,LED_SCR,LED_CAP,LED_NUM
#include <errno.h>                        //errno()
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>

#define MAX_TOKENS 32
#define KBD_NUM 'n'
#define KBD_CAP 'c'
#define KBD_SCR 's'

char CONSOLE[] = "/dev/console";
static unsigned int tx_orig=0, rx_orig=0;
static void cleanup(void);

int kbd_lights(unsigned char led)
{
	int ttyfd;                        //fd for console device
	int result;                       //results of ioctl()


	//This is the console device we will open

	//open console device for later ioctl() call

	ttyfd = open(CONSOLE, O_RDWR);
	if (ttyfd < 0)
	{
		printf("cannot open %s\n", CONSOLE);
		return 1;
	} // end if()

	result = ioctl(ttyfd,KDSETLED,&led);
	if (result < 0)
	{
		printf("ioctl returned error: %d\n", errno);
		close(ttyfd);
		return 1;
	}

	close(ttyfd);
	return 0;
} // end kbd_lights()


void sig_handler(int signum)
{
	kbd_lights(0);
	cleanup();
	exit(0);
}


/*============================================================================
 * Strip leading and trailing spaces off the string str.
 */
char* str_strip (char* str, char* s)
{
	char* eos = str + strlen(str);		/* -> end of string */

	while (*str && strchr(s, *str))
		++str;				/* strip leading spaces */
	while ((eos > str) && strchr(s, *(eos - 1)))
		--eos;				/* strip trailing spaces */
	*eos = '\0';
	return str;
}


/*============================================================================
 * Tokenize string.
 *	Parse a string of the following syntax:
 *		<tag>=<arg1>,<arg2>,...
 *	and fill array of tokens with pointers to string elements.
 *
 *	Return number of tokens.
 */
int tokenize (char* str, char **tokens, char *arg1, char *arg2, char *arg3)
{
	int cnt = 0;

	tokens[0] = strtok(str, arg1);
	while (tokens[cnt] && (cnt < MAX_TOKENS - 1)) {
		tokens[cnt] = str_strip(tokens[cnt], arg2);
		tokens[++cnt] = strtok(NULL, arg3);
	}
	return cnt;
}

void get_tx_rx_data (char *if_name, unsigned int *tx_data, unsigned int *rx_data)
{
	char net_dev[] = "/proc/net/dev";
	struct stat file_stat;
	int toknum;
	char* token[MAX_TOKENS];
	FILE *pipe_fd;
	char line[500];
	int i,len,found=0;
	char *ptr;
	
	*tx_data = *rx_data = 0;

	if (stat(net_dev, &file_stat) < 0)
		return;

	pipe_fd = popen("cat /proc/net/dev", "r");
	if (pipe_fd == NULL){
		printf("Failed to open /proc/net/dev\n");
		return;
	}
	
	i=0;	
	while (fgets(line,sizeof(line)-1,pipe_fd)){
		if (++i < 2)
			continue;
		if (strstr(line,if_name) != NULL){
			found = 1;
			break;
		}
	}

	pclose(pipe_fd);

	if (!found){
		return;
	}

	/* Bug Fix: Dec 7 2000
	 * When the packet count gets too high, 
	 * the space between the interface name and the
	 * count is used yp. This breaks our parsing procedure,
	 * Therefore, replace the first colon by SPACE, this
	 * way there will always be space between the interface
	 * name and the byte count */
	
	if ((ptr=strchr(line,':')) != NULL){
		*ptr=' ';
	}
		
	len = strlen(line) + 1;
	toknum = tokenize(line, token, " ", " \t\n", " ");

	if (toknum < 13)
		return;
	
	*rx_data = atoi(token[2]);
	*tx_data = atoi(token[10]);

	return;
}


void get_wanpipe_state (char *device_name, char *state)
{
	char router_dir[] = "/proc/net/wanrouter";	
	FILE *pipe_fd;
	char line[100];
	int i,len,found=0;
	int toknum;
	char* token[MAX_TOKENS];
	struct stat file_stat;
	
	strcpy(state,"unconfigured");
	
	if (stat(router_dir, &file_stat) < 0)
		return;
	
	pipe_fd = popen("cat /proc/net/wanrouter/status", "r");
	if (pipe_fd == NULL)
		return;

	i=0;
	while (fgets(line,sizeof(line)-1,pipe_fd)){
		if (strstr(line,device_name) != NULL){
			found = 1;
			break;
		}
	}

	pclose(pipe_fd);

	if (!found){
		return;
	}

	len = strlen(line) + 1;
	toknum = tokenize(line, token, "|", " \t\n", "|");

	if (toknum <= 3)
		return;

	if (strcmp(token[0],device_name))
		return;

	strcpy(state,token[3]);
	
	return;
}

int check_and_set_lock_file(void)
{
	char wanpipe_dir[] = "/usr/local/wanrouter";
	char lock_file[] = "/usr/local/wanrouter/wpkbd_lock";
	struct stat file_stat;
	
	if (stat(wanpipe_dir, &file_stat) < 0){
		printf("wpkbdmon: Error, WANPIPE utilites not installed\n");
		printf("wpkbdmon: Directory not found: %s\n",wanpipe_dir);
		return 1;
	}
	
	if (stat(lock_file, &file_stat) < 0){
		system ("touch /usr/local/wanrouter/wpkbd_lock");
	}else{
		printf("\nwpkbdmon: Error, WANPIPE Keyboard Led Monitor already running!\n");
		  printf("                 The /usr/local/wanrouter/wpkbd_lock exits.\n\n");
		return 1;
	}

	return 0;
}


void cleanup (void)
{
	system("rm -f /usr/local/wanrouter/wpkbd_lock");
}


int main (int argc, char **argv)
{
	int update_interval=5;
	char device[15];
	char interface[15];
	char state[50];
	int errno;
	unsigned int tx_data, rx_data;
	unsigned char led_state=0;
	struct sigaction sa;
	
	//General Error Message
	const char *help =
	      "\n  wpkbdmon v1.0 - (c)2000 Nenad Corbic <ncorbic@sangoma.com>\n\n"
                "  This program is distributed under the terms of GPL.\n\n"
		"  This program uses the keyboard leds to convey \n"
		"  operational statistics of the Sangoma WANPIPE adapter.\n"
		"  	NUM_LOCK    = Line State  (On=connected,    Off=disconnected)\n"
		"  	CAPS_LOCK   = Tx data	  (On=transmitting, Off=no tx data)\n" 	
		"	SCROLL_LOCK = Rx data     (On=receiving,    Off=no rx data)\n\n"
                "  Usage:\n"
                "  wpkbdmon <wanpipe device> <interface name> [update_interval]\n\n"
                "           <wanpipe device>  = wanpipe# (#=1,2,3 ...)\n"
		"           <interface name>  = wanpipe network interface ex: wp1_fr16\n"
		"           [update interval] = 1-60 seconds , optional (default 5 sec)\n\n";

	if (argc < 3){
		printf("\nError in usage: Arguments missing\n");
		printf("%s",help);
		exit(1);
	}

	if (argc == 4){
		update_interval = atoi(argv[3]);
		if (update_interval < 1 || update_interval > 60){
			printf("Error: Update interval out of range!\n");	
			printf("%s",help);
			exit(1);
		}
		printf("\nUsing Time Interval: %i sec\n",update_interval);
	}else{
		printf("\nUsing Default Time Interval: %i sec\n",update_interval);
	}

	if (check_and_set_lock_file())
		exit(0);
	
	memset(&sa,0,sizeof(sa));
	sa.sa_handler=sig_handler;
	
	if (sigaction(SIGINT,&sa,NULL)){
		perror("sigaction");
	}
	
	strcpy(device,argv[1]);
	strcpy(interface,argv[2]);

	printf ("Starting Wanpipe Keyboard Debugger, Device: %s Interface: %s\n",argv[1],argv[2]);
	for (;;){
	
		get_wanpipe_state(device, state);

		if (!strcmp(state,"unconfigured")){
			kbd_lights(0);
			printf("Warning: Device %s is down, exiting debugger\n",device);
			goto wpkbd_exit;
			
		}else if (!strcmp(state, "connected")){
			led_state |= LED_NUM;
		}else{
			if (kbd_lights(0)) 
				goto wpkbd_exit;
			goto skip_tx_rx;
		}
	

		get_tx_rx_data(interface,&tx_data,&rx_data);

		/* If number of tx packets has not changed, or 
		 * number of tx packets is 0 while original value is
		 * not, the packet transmission has stopped.  
		 * Thus, turn off the CAP led. */

		if (tx_data == tx_orig || (tx_orig && !tx_data)){
			led_state &= ~LED_CAP;
		}else{
			led_state |= LED_CAP;
		}
		tx_orig=tx_data;
		
		if (rx_data == rx_orig || (rx_orig && !rx_data)){
			led_state &= ~LED_SCR;
		}else{
			led_state |= LED_SCR;
		}
		rx_orig=rx_data;

		if (kbd_lights(led_state)) 
			goto wpkbd_exit;

skip_tx_rx:		
		sleep(update_interval);
	}

wpkbd_exit:
	
	cleanup();
	return 0;
}


