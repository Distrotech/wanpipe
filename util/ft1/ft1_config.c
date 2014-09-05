/*****************************************************************************
* ft1_config.c    X25 Test Menu Driven Application based on X25 API
*
* Author:       Nenad Corbic <ncorbic@sangoma.com>
*
* Copyright:    (c) 1999 Sangoma Technologies Inc.
*
*               This program is free software; you can redistribute it and/or
*               modify it under the terms of the GNU General Public License
*               as published by the Free Software Foundation; either version
*               2 of the License, or (at your option) any later version.
* ----------------------------------------------------------------------------
*
* Nov 25, 1999	Nenad Corbic	Inital Verstion
*				Supports Standard, Advanced and Auto config
*      				as well as reading CSU/DSU config.
*
*
*****************************************************************************/

/*****************************************************************************
*
* !!! WARNING !!!
*
* This program is used by 'ft1conf' script. It is not supposed to be 
* used by itself.  Please run the ft1conf script to use functions
* of this program.
*
* Usage:
*   wanpipe_ft1exec [{switches}] {device} ]
*
*   ex: ./wanpipe_ft1exec wanpipe1 <command> <options>
*
* Where:
*   {device}    WANPIPE device name in /proc/net/router directory
*
*****************************************************************************/

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/mman.h>
#if defined(__LINUX__)
# include <linux/wanpipe_defines.h>
# include <linux/wanpipe_cfg.h>
# include <linux/wanpipe.h>
# include <linux/sdla_chdlc.h>
#else
# include <wanpipe_defines.h>
# include <wanpipe_cfg.h>
# include <wanpipe.h>
# include <sdla_chdlc.h>
#endif
/****** Defines *************************************************************/

#ifndef min
#define min(a,b)        (((a)<(b))?(a):(b))
#endif

/* Error exit codes */
enum ErrCodes
{
	ERR_SYSTEM = 1,         /* System error */
	ERR_SYNTAX,             /* Command line syntax error */
	ERR_LIMIT
};


/* Defaults */
#define TRY_AGAIN 	0x07

/* Device directory and WANPIPE device name */
#define WANDEV_NAME "/dev/wanrouter"

/****** Data Types **********************************************************/

/****** Function Prototypes *************************************************/

void show_error (int err);
extern  int close (int);
int advanced_config (int);
int auto_config (int);
int standard_config (int , char **);
int read_configuration (int);
int advanced_config (int);
int auto_config (int dev);
int kbdhit(int *);

/****** Global Data *********************************************************/

int dev; 

enum {REG_CONF, AUTO_CONF, ADV_CONF, READ_CONF, EXIT}; 


sdla_exec_t *exec;
#if defined(__NetBSD__) || defined(__FreeBSD__) || defined(__OpenBSD__)
wan_conf_t 	config;
#endif
wan_cmd_t*	ctrl = NULL;
void *data;

/*
 * Strings & message tables.
 */
char progname[] = "wanpipe_ft1exec";
char wandev_dir[] = "/proc/net/wanrouter";         /* location of WAN devices */

char banner[] =
	"WANPIPE FT1 Configuration Utility"
	"(c) 1999 Sangoma Technologies Inc."
;
char helptext[] =
	"Usage:\twanpipe_ft1exec [{switches}] {device} \n"
	"\nWhere:"
	"\t{device}\tWANPIPE device name from /proc/net/router directory\n"
	"\t{switches}\tone of the following:\n"
	"\t\t\t-v\tverbose mode\n"
	"\t\t\t-h|?\tshow this help\n"
;

char* err_messages[] =          /* Error messages */
{
	"Invalid command line syntax",  /* ERR_SYNTAX */
	"Unknown error code",           /* ERR_LIMIT */
};


/*------------------------- MAIN PROGRAM --------------------------*/


/******************************************************************
* Main 
*
*
*
******************************************************************/

int main (int argc, char *argv[])
{
	int err=0, max_flen = sizeof(wandev_dir) + WAN_DRVNAME_SZ;
	char filename[max_flen + 2];

	if (argc < 3){
		printf("Prog name is %s %i\n", argv[0],WAN_DRVNAME_SZ );
		show_error(ERR_SYNTAX);
		return ERR_SYNTAX;
	}

#if defined(__LINUX__)
	snprintf(filename, max_flen, "%s/%s", wandev_dir, argv[1]);
#else
	snprintf(filename, max_flen, "%s", WANDEV_NAME);
#endif
	dev = open(filename, O_RDONLY);
	if (dev < 0){
		return -EINVAL;
	}
	
	exec = malloc(sizeof(sdla_exec_t));
	ctrl = malloc(sizeof(wan_cmd_t));
	data = malloc(SIZEOF_MB_DATA_BFR);
	exec->magic = WANPIPE_MAGIC;
	exec->cmd = ctrl;
	exec->data = data; 
	memset(ctrl, 0, sizeof(wan_cmd_t));
	memset(data, 0, SIZEOF_MB_DATA_BFR);

	/* Special for FreeBSD */
#if defined(__NetBSD__) || defined(__FreeBSD__) || defined(__OpenBSD__)
	strlcpy(config.devname, argv[1], WAN_DRVNAME_SZ);
	config.arg = exec;
#endif

	switch (atoi(argv[2])){
	
       		case REG_CONF: 
			if (argc != 9){
				printf("Illegal number of arguments\n");
				break;
			}
			if ((err=standard_config(dev,&argv[3])) == 0){
				sleep(1);
				err=read_configuration(dev);
			}
                    	break;
	      	case AUTO_CONF: 
			err=auto_config(dev);
		      	break;
	      	case ADV_CONF: 
			printf("Advanced Configurator Activated, Press ESC to Exit!\n\n");
			err=advanced_config(dev);
			break;
		case READ_CONF:
			err=read_configuration(dev);
			break;
		case EXIT:
			break;
              	default:
                     	printf("%s: Error Illegal Option\n",progname); 
                      	break;
       	}

	free(exec);             
	free(ctrl);             
	free(data);
	
	if (dev >= 0) close(dev);

	return err;
}


/******************************************************************
* Standard Configuration
*
*
*
******************************************************************/

int standard_config (int dev, char *argv[])
{
	
	ft1_config_t *ft1_config;
	int len;
	
	ft1_config = (ft1_config_t *)data;

	ft1_config->framing_mode   = atoi(argv[0]);
	ft1_config->encoding_mode  = atoi(argv[1]);
	ft1_config->line_build_out = atoi(argv[2]);  
	ft1_config->channel_base   = atoi(argv[3]);
	ft1_config->baud_rate_kbps = atoi(argv[4]);
	ft1_config->clock_mode     = atoi(argv[5]);

	ctrl->wan_cmd_command = SET_FT1_CONFIGURATION;
	ctrl->wan_cmd_data_len = sizeof(ft1_config_t);

	len = ctrl->wan_cmd_data_len/2;

#if defined(__LINUX__)
	if ((dev < 0) || (ioctl(dev, WANPIPE_EXEC, exec ) < 0)){
#else
	if ((dev < 0) || (ioctl(dev, WANPIPE_EXEC, &config ) < 0)){
#endif
	       	return -EIO;
        }                               	

	return ctrl->wan_cmd_return_code;
}

/******************************************************************
* Read Configuration 
*
*
*
******************************************************************/

int read_configuration (int dev)
{
	int i;
	
	ctrl->wan_cmd_command = READ_FT1_CONFIGURATION;
	ctrl->wan_cmd_data_len = 0;

	memset(data,0,SIZEOF_MB_DATA_BFR);

#if defined(__LINUX__)
	if ((dev < 0) || (ioctl(dev, WANPIPE_EXEC, exec ) < 0)){
#else
	if ((dev < 0) || (ioctl(dev, WANPIPE_EXEC, &config ) < 0)){
#endif
	       	return -EIO;
        }                               	

	/* Check for errors */
	if (ctrl->wan_cmd_return_code){
		printf("Read Rc is %d\n",ctrl->wan_cmd_return_code);
		return ctrl->wan_cmd_return_code;
	}
	for (i=0;i<(ctrl->wan_cmd_data_len/2);i++){
		printf("%i\n",*((unsigned short*)data + i));
	}
	return WAN_CMD_OK;
}

/******************************************************************
* Advanced Configuration
*
*
*
******************************************************************/

int advanced_config (int dev)
{

	int key_int, i;
	char key_ch;

	memset(data,0,SIZEOF_MB_DATA_BFR);

	for (;;){
		if(kbdhit(&key_int)){
			key_ch = key_int;

			if (key_ch == 0x1b){
				break;
			}

			ctrl->wan_cmd_command = TRANSMIT_ASYNC_DATA_TO_FT1;
			ctrl->wan_cmd_data_len = sizeof(key_ch);
			*((unsigned char *)data) = key_ch;

#if defined(__LINUX__)
			if ((dev < 0) || (ioctl(dev, WANPIPE_EXEC, exec) < 0)){
#else
			if ((dev < 0) || (ioctl(dev, WANPIPE_EXEC, &config) < 0)){
#endif
				printf("Write ioctl error\n");
	       			return -EIO;
        		}
     
			if (ctrl->wan_cmd_return_code != 0){
			        printf("Write return code %i\n",
						ctrl->wan_cmd_return_code);
				return ctrl->wan_cmd_return_code;
			}
        	}else{       
			ctrl->wan_cmd_command = RECEIVE_ASYNC_DATA_FROM_FT1;
			ctrl->wan_cmd_data_len = 0;

#if defined(__LINUX__)
			if ((dev < 0) || (ioctl(dev, WANPIPE_EXEC, exec) < 0)){
#else
			if ((dev < 0) || (ioctl(dev, WANPIPE_EXEC, &config) < 0)){
#endif
				printf("Read ioctl error\n");
       				return -EIO;
       			}
		
			if (ctrl->wan_cmd_return_code != WAN_CMD_OK && 
			    ctrl->wan_cmd_return_code != TRY_AGAIN){
				printf("Read return code %i\n",
						ctrl->wan_cmd_return_code);
				return ctrl->wan_cmd_return_code;
			}
			
			if (ctrl->wan_cmd_data_len){
				for (i=0;i<ctrl->wan_cmd_data_len;i++){
					putchar(*((unsigned char *)data + i));
					fflush(stdout);
				}
			}
		}
	
	}
	return 0;
}


/******************************************************************
* Automatic Configuration
*
*
*
******************************************************************/

int auto_config (int dev)
{

	volatile ft1_config_t *ft1_config;
	int i,key;
	unsigned short current_FT1_baud_rate=0;

	ft1_config = (ft1_config_t *)data;

	ft1_config->framing_mode   = ESF_FRAMING;
	ft1_config->encoding_mode  = B8ZS_ENCODING;		
	ft1_config->line_build_out = LN_BLD_CSU_0dB_DSX1_0_to_133;  
	ft1_config->channel_base   = 0x01;	
	ft1_config->baud_rate_kbps = BAUD_RATE_FT1_AUTO_CONFIG;
	ft1_config->clock_mode     = CLOCK_MODE_NORMAL;	

	ctrl->wan_cmd_command = SET_FT1_CONFIGURATION;
	ctrl->wan_cmd_data_len = sizeof(ft1_config_t);

#if defined(__LINUX__)
	if ((dev < 0) || (ioctl(dev, WANPIPE_EXEC, exec) < 0)){
#else
	if ((dev < 0) || (ioctl(dev, WANPIPE_EXEC, &config) < 0)){
#endif
	       	return -EIO;
        }                               	

	if (ctrl->wan_cmd_return_code){
		printf("Automatic, Write Command Failed\n");
		fflush(stdout);
		return ctrl->wan_cmd_return_code;
	}

	if (ft1_config->baud_rate_kbps == 0xFFFF){
		printf("\nPerforming FT1 auto configuration ... Press any key to abort\n");
		fflush(stdout);	

		do{
			ctrl->wan_cmd_command = READ_FT1_CONFIGURATION;
			ctrl->wan_cmd_data_len = 0;

#if defined(__LINUX__)
			if ((dev < 0) || (ioctl(dev, WANPIPE_EXEC, exec) < 0)){
#else
			if ((dev < 0) || (ioctl(dev, WANPIPE_EXEC, &config) < 0)){
#endif
	       			return -EIO;
        		} 			

			switch(ctrl->wan_cmd_return_code){

			case WAN_CMD_OK:
				printf("The FT1 configuration has been successfully completed. Baud rate is %u Kbps\n", ft1_config->baud_rate_kbps);			
				for (i=0;i<(ctrl->wan_cmd_data_len/2);i++){
					printf("%i\n",*((unsigned short*)data + i));
				}
				return WAN_CMD_OK;
			
			case AUTO_FT1_CONFIG_NOT_COMPLETE:
				if (ft1_config->baud_rate_kbps != current_FT1_baud_rate){
					current_FT1_baud_rate = ft1_config->baud_rate_kbps;
					printf("Testing line for baud rate %i Kbps\n",
						current_FT1_baud_rate);
					fflush(stdout);
				}
				break;

			case AUTO_FT1_CFG_FAIL_OP_MODE:
				return ctrl->wan_cmd_return_code;
	
			case AUTO_FT1_CFG_FAIL_INVALID_LINE:
				return ctrl->wan_cmd_return_code;

			default:
				printf("Unknown return code 0x%02X\n", 
						ctrl->wan_cmd_return_code);
				fflush(stdout);
				return ctrl->wan_cmd_return_code;
			}
 
		}while (!kbdhit(&key));

	}
	return ctrl->wan_cmd_return_code;

}

/*******************************************************************
* Show error message.
*
*
*
*******************************************************************/
void show_error (int err)
{
	if (err == ERR_SYSTEM) fprintf(stderr, "%s: SYSTEM ERROR %d: %s!\n",
		progname, errno, strerror(errno))
	;
	else fprintf(stderr, "%s: %s!\n", progname,
		err_messages[min(err, ERR_LIMIT) - 2])
	;
}

/****** End *****************************************************************/
