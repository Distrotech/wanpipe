/*****************************************************************************
* zapmon.c	AFT Debugger/Monitor
*
* Author:       David Rokhvarg <davidr@sangoma.com>	
*		Alex Feldman <alex@sangoma.com>
*
* Copyright:	(c) 1984-2005 Sangoma Technologies Inc.
*
*		This program is free software; you can redistribute it and/or
*		modify it under the terms of the GNU General Public License
*		as published by the Free Software Foundation; either version
*		2 of the License, or (at your option) any later version.
* ----------------------------------------------------------------------------
* Nov 08, 2004	David Rokhvarg	Initial version
* Jan 27, 2009 Alex Feldman	Update spike test for Dahdi
*	
*****************************************************************************/

/******************************************************************************
 * 			INCLUDE FILES					      *
 *****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>	/* offsetof(), etc. */
#include <ctype.h>
#include <sys/time.h>
#include <time.h>	/* ctime() */
#include <sys/types.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <fcntl.h>	/* for O_RDWR, O_NONBLOCK */
#include <errno.h>
#include <math.h>	/* for log10() */
#if defined(__LINUX__)
# include <linux/version.h>
# include "wanpipe_api.h"
# include "zapcompat.h"
#else
#endif
#include "fe_lib.h"
#include "wanpipemon.h"

/******************************************************************************
 * 			DEFINES/MACROS					      *
 *****************************************************************************/
#if LINUX_VERSION_CODE >= 0x020100
#define LINUX_2_1
#endif

#define BANNER(str)  		banner(str,0)

#define ZAPMON_SPIKE_FNAME_DEF	"wan_spike"

#define SPIKE_FNAME_LEN		100
/******************************************************************************
 * 			TYPEDEF/STRUCTURE				      *
 *****************************************************************************/
/* Structures for data casting */

/******************************************************************************
 * 			GLOBAL VARIABLES				      *
 *****************************************************************************/
extern int 	zap_monitor, dahdi_monitor;
extern int 	tdmv_chan;

static int 	zfd=0;
static char 	*zap_fn = "/dev/zap/channel";
static char 	*dahdi_fn = "/dev/dahdi/channel";
static char	spike_fout[SPIKE_FNAME_LEN];

static unsigned char	spike_info[sizeof(wan_espike_hdr_t) + WAN_SPIKE_MAX_SAMPLE_LEN];

int echo_detection_state;
int is_call_active;
int ed_enabled;
unsigned int num_of_echo_absent_calls;
unsigned int num_of_echo_present_calls;

#ifdef TDMV_SAMPLE_STATE_DECODE
#undef TDMV_SAMPLE_STATE_DECODE
#endif
#define TDMV_SAMPLE_STATE_DECODE(state)			\
	((state == ECHO_PRESENT) ? "On" :		\
	 (state == ECHO_ABSENT) ?  "Off" : 		\
         (state == INDETERMINATE) ? "?" : "Invalid")

#define TMP_BUFF_LEN	512

static unsigned short u2s[] = {
 /* negative */
0x8284, 0x8684, 0x8A84, 0x8E84, 0x9284, 0x9684, 0x9A84, 0x9E84,
0xA284, 0xA684, 0xAA84, 0xAE84, 0xB284, 0xB684, 0xBA84, 0xBE84,
0xC184, 0xC384, 0xC584, 0xC784, 0xC984, 0xCB84, 0xCD84, 0xCF84,
0xD184, 0xD384, 0xD584, 0xD784, 0xD984, 0xDB84, 0xDD84, 0xDF84,
0xE104, 0xE204, 0xE304, 0xE404, 0xE504, 0xE604, 0xE704, 0xE804,
0xE904, 0xEA04, 0xEB04, 0xEC04, 0xED04, 0xEE04, 0xEF04, 0xF004,
0xF0C4, 0xF144, 0xF1C4, 0xF244, 0xF2C4, 0xF344, 0xF3C4, 0xF444,
0xF4C4, 0xF544, 0xF5C4, 0xF644, 0xF6C4, 0xF744, 0xF7C4, 0xF844,
0xF8A4, 0xF8E4, 0xF924, 0xF964, 0xF9A4, 0xF9E4, 0xFA24, 0xFA64,
0xFAA4, 0xFAE4, 0xFB24, 0xFB64, 0xFBA4, 0xFBE4, 0xFC24, 0xFC64,
0xFC94, 0xFCB4, 0xFCD4, 0xFCF4, 0xFD14, 0xFD34, 0xFD54, 0xFD74,
0xFD94, 0xFDB4, 0xFDD4, 0xFDF4, 0xFE14, 0xFE34, 0xFE54, 0xFE74,
0xFE8C, 0xFE9C, 0xFEAC, 0xFEBC, 0xFECC, 0xFEDC, 0xFEEC, 0xFEFC,
0xFF0C, 0xFF1C, 0xFF2C, 0xFF3C, 0xFF4C, 0xFF5C, 0xFF6C, 0xFF7C,
0xFF88, 0xFF90, 0xFF98, 0xFFA0, 0xFFA8, 0xFFB0, 0xFFB8, 0xFFC0,
0xFFC8, 0xFFD0, 0xFFD8, 0xFFE0, 0xFFE8, 0xFFF0, 0xFFF8, 0x0000,
/* positive */
0x7D7C, 0x797C, 0x757C, 0x717C, 0x6D7C, 0x697C, 0x657C, 0x617C,
0x5D7C, 0x597C, 0x557C, 0x517C, 0x4D7C, 0x497C, 0x457C, 0x417C,
0x3E7C, 0x3C7C, 0x3A7C, 0x387C, 0x367C, 0x347C, 0x327C, 0x307C,
0x2E7C, 0x2C7C, 0x2A7C, 0x287C, 0x267C, 0x247C, 0x227C, 0x207C,
0x1EFC, 0x1DFC, 0x1CFC, 0x1BFC, 0x1AFC, 0x19FC, 0x18FC, 0x17FC,
0x16FC, 0x15FC, 0x14FC, 0x13FC, 0x12FC, 0x11FC, 0x10FC, 0x0FFC,
0x0F3C, 0x0EBC, 0x0E3C, 0x0DBC, 0x0D3C, 0x0CBC, 0x0C3C, 0x0BBC,
0x0B3C, 0x0ABC, 0x0A3C, 0x09BC, 0x093C, 0x08BC, 0x083C, 0x07BC,
0x075C, 0x071C, 0x06DC, 0x069C, 0x065C, 0x061C, 0x05DC, 0x059C,
0x055C, 0x051C, 0x04DC, 0x049C, 0x045C, 0x041C, 0x03DC, 0x039C,
0x036C, 0x034C, 0x032C, 0x030C, 0x02EC, 0x02CC, 0x02AC, 0x028C,
0x026C, 0x024C, 0x022C, 0x020C, 0x01EC, 0x01CC, 0x01AC, 0x018C,
0x0174, 0x0164, 0x0154, 0x0144, 0x0134, 0x0124, 0x0114, 0x0104,
0x00F4, 0x00E4, 0x00D4, 0x00C4, 0x00B4, 0x00A4, 0x0094, 0x0084,
0x0078, 0x0070, 0x0068, 0x0060, 0x0058, 0x0050, 0x0048, 0x0040,
0x0038, 0x0030, 0x0028, 0x0020, 0x0018, 0x0010, 0x0008, 0x0000
};

#if 1
//ZAPTEL table
static unsigned short a2s[] = {

0xEB00, 0xEC00, 0xE900, 0xEA00, 0xEF00, 0xF000, 0xED00, 0xEE00,
0xE300, 0xE400, 0xE100, 0xE200, 0xE700, 0xE800, 0xE500, 0xE600, 
0xF580, 0xF600, 0xF480, 0xF500, 0xF780, 0xF800, 0xF680, 0xF700, 
0xF180, 0xF200, 0xF080, 0xF100, 0xF380, 0xF400, 0xF280, 0xF300, 
0xAC00, 0xB000, 0xA400, 0xA800, 0xBC00, 0xC000, 0xB400, 0xB800, 
0x8C00, 0x9000, 0x8400, 0x8800, 0x9C00, 0xA000, 0x9400, 0x9800, 
0xD600, 0xD800, 0xD200, 0xD400, 0xDE00, 0xE000, 0xDA00, 0xDC00, 
0xC600, 0xC800, 0xC200, 0xC400, 0xCE00, 0xD000, 0xCA00, 0xCC00, 
0xFEB0, 0xFEC0, 0xFE90, 0xFEA0, 0xFEF0, 0xFF00, 0xFED0, 0xFEE0, 
0xFE30, 0xFE40, 0xFE10, 0xFE20, 0xFE70, 0xFE80, 0xFE50, 0xFE60, 
0xFFB0, 0xFFC0, 0xFF90, 0xFFA0, 0xFFF0, 0x0000, 0xFFD0, 0xFFE0, 
0xFF30, 0xFF40, 0xFF10, 0xFF20, 0xFF70, 0xFF80, 0xFF50, 0xFF60, 
0xFAC0, 0xFB00, 0xFA40, 0xFA80, 0xFBC0, 0xFC00, 0xFB40, 0xFB80, 
0xF8C0, 0xF900, 0xF840, 0xF880, 0xF9C0, 0xFA00, 0xF940, 0xF980, 
0xFD60, 0xFD80, 0xFD20, 0xFD40, 0xFDE0, 0xFE00, 0xFDA0, 0xFDC0, 
0xFC60, 0xFC80, 0xFC20, 0xFC40, 0xFCE0, 0xFD00, 0xFCA0, 0xFCC0, 

0x1500, 0x1400, 0x1700, 0x1600, 0x1100, 0x1000, 0x1300, 0x1200, 
0x1D00, 0x1C00, 0x1F00, 0x1E00, 0x1900, 0x1800, 0x1B00, 0x1A00, 
0x0A80, 0x0A00, 0x0B80, 0x0B00, 0x0880, 0x0800, 0x0980, 0x0900, 
0x0E80, 0x0E00, 0x0F80, 0x0F00, 0x0C80, 0x0C00, 0x0D80, 0x0D00, 
0x5400, 0x5000, 0x5C00, 0x5800, 0x4400, 0x4000, 0x4C00, 0x4800, 
0x7400, 0x7000, 0x7C00, 0x7800, 0x6400, 0x6000, 0x6C00, 0x6800, 
0x2A00, 0x2800, 0x2E00, 0x2C00, 0x2200, 0x2000, 0x2600, 0x2400, 
0x3A00, 0x3800, 0x3E00, 0x3C00, 0x3200, 0x3000, 0x3600, 0x3400, 
0x0150, 0x0140, 0x0170, 0x0160, 0x0110, 0x0100, 0x0130, 0x0120, 
0x01D0, 0x01C0, 0x01F0, 0x01E0, 0x0190, 0x0180, 0x01B0, 0x01A0, 
0x0050, 0x0040, 0x0070, 0x0060, 0x0010, 0x0000, 0x0030, 0x0020, 
0x00D0, 0x00C0, 0x00F0, 0x00E0, 0x0090, 0x0080, 0x00B0, 0x00A0, 
0x0540, 0x0500, 0x05C0, 0x0580, 0x0440, 0x0400, 0x04C0, 0x0480, 
0x0740, 0x0700, 0x07C0, 0x0780, 0x0640, 0x0600, 0x06C0, 0x0680, 
0x02A0, 0x0280, 0x02E0, 0x02C0, 0x0220, 0x0200, 0x0260, 0x0240,
0x03A0, 0x0380, 0x03E0, 0x03C0, 0x0320, 0x0300, 0x0360, 0x0340
};
#endif

#if 0
//YATE table
static unsigned short a2s[] = {
/* negative */
0xEA80, 0xEB80, 0xE880, 0xE980, 0xEE80, 0xEF80, 0xEC80, 0xED80,
0xE280, 0xE380, 0xE080, 0xE180, 0xE680, 0xE780, 0xE480, 0xE580,
0xF540, 0xF5C0, 0xF440, 0xF4C0, 0xF740, 0xF7C0, 0xF640, 0xF6C0,
0xF140, 0xF1C0, 0xF040, 0xF0C0, 0xF340, 0xF3C0, 0xF240, 0xF2C0,
0xAA00, 0xAE00, 0xA200, 0xA600, 0xBA00, 0xBE00, 0xB200, 0xB600,
0x8A00, 0x8E00, 0x8200, 0x8600, 0x9A00, 0x9E00, 0x9200, 0x9600,
0xD500, 0xD700, 0xD100, 0xD300, 0xDD00, 0xDF00, 0xD900, 0xDB00,
0xC500, 0xC700, 0xC100, 0xC300, 0xCD00, 0xCF00, 0xC900, 0xCB00,
0xFEA8, 0xFEB8, 0xFE88, 0xFE98, 0xFEE8, 0xFEF8, 0xFEC8, 0xFED8,
0xFE28, 0xFE38, 0xFE08, 0xFE18, 0xFE68, 0xFE78, 0xFE48, 0xFE58,
0xFFA8, 0xFFB8, 0xFF88, 0xFF98, 0xFFE8, 0xFFF8, 0xFFC8, 0xFFD8,
0xFF28, 0xFF38, 0xFF08, 0xFF18, 0xFF68, 0xFF78, 0xFF48, 0xFF58,
0xFAA0, 0xFAE0, 0xFA20, 0xFA60, 0xFBA0, 0xFBE0, 0xFB20, 0xFB60,
0xF8A0, 0xF8E0, 0xF820, 0xF860, 0xF9A0, 0xF9E0, 0xF920, 0xF960,
0xFD50, 0xFD70, 0xFD10, 0xFD30, 0xFDD0, 0xFDF0, 0xFD90, 0xFDB0,
0xFC50, 0xFC70, 0xFC10, 0xFC30, 0xFCD0, 0xFCF0, 0xFC90, 0xFCB0,
/* positive */
0x1580, 0x1480, 0x1780, 0x1680, 0x1180, 0x1080, 0x1380, 0x1280,
0x1D80, 0x1C80, 0x1F80, 0x1E80, 0x1980, 0x1880, 0x1B80, 0x1A80,
0x0AC0, 0x0A40, 0x0BC0, 0x0B40, 0x08C0, 0x0840, 0x09C0, 0x0940,
0x0EC0, 0x0E40, 0x0FC0, 0x0F40, 0x0CC0, 0x0C40, 0x0DC0, 0x0D40,
0x5600, 0x5200, 0x5E00, 0x5A00, 0x4600, 0x4200, 0x4E00, 0x4A00,
0x7600, 0x7200, 0x7E00, 0x7A00, 0x6600, 0x6200, 0x6E00, 0x6A00,
0x2B00, 0x2900, 0x2F00, 0x2D00, 0x2300, 0x2100, 0x2700, 0x2500,
0x3B00, 0x3900, 0x3F00, 0x3D00, 0x3300, 0x3100, 0x3700, 0x3500,
0x0158, 0x0148, 0x0178, 0x0168, 0x0118, 0x0108, 0x0138, 0x0128,
0x01D8, 0x01C8, 0x01F8, 0x01E8, 0x0198, 0x0188, 0x01B8, 0x01A8,
0x0058, 0x0048, 0x0078, 0x0068, 0x0018, 0x0008, 0x0038, 0x0028,
0x00D8, 0x00C8, 0x00F8, 0x00E8, 0x0098, 0x0088, 0x00B8, 0x00A8,
0x0560, 0x0520, 0x05E0, 0x05A0, 0x0460, 0x0420, 0x04E0, 0x04A0,
0x0760, 0x0720, 0x07E0, 0x07A0, 0x0660, 0x0620, 0x06E0, 0x06A0,
0x02B0, 0x0290, 0x02F0, 0x02D0, 0x0230, 0x0210, 0x0270, 0x0250,
0x03B0, 0x0390, 0x03F0, 0x03D0, 0x0330, 0x0310, 0x0370, 0x0350
};
#endif

#define INITIAL_LINEAR_TX_SPIKE 16384
#define NO			0
#define YES 			1
#define DB_LOSS_THRESHOLD	-45.00//-50.00//-60.00
#define DB_SILENCE		80.00

typedef struct{
	float db_loss;
	short linear_power_value;
}db_sample_t;

db_sample_t db_loss[WAN_SPIKE_MAX_SAMPLE_LEN];

/******************************************************************************
 * 			FUNCTION PROTOTYPES				      *
 *****************************************************************************/
/* Command routines */
static int zap_do_command(int echo_detect_ioctl, int channo, void *ioctl_result);

/* Echo Detect routines */
static int 	get_echo_spike(int channo, unsigned char sample_mode);
static int 	send_echo_spike(int channo, unsigned char sample_mode);
static void	linearize_spike_buffer(wan_espike_hdr_t *, unsigned char*, int*);
static int 	write_spike_buffer_to_file(wan_espike_hdr_t*, int*);
static void	summary_of_linear_spike_buffer(wan_espike_hdr_t*, int*);
static void 	proc_sample(unsigned short, int);
static void 	analyze_db_loss(wan_espike_hdr_t *spike_hdr);
static int 	usecsleep(int sec, int usecs);

/*****************************************************************************/
static char *gui_main_menu[]={
"zap_status_menu","Zap Status Menu",
"."
};

static char *zap_status_menu[]={
"xz","Zap Status Command",
"."
};
	
static struct cmd_menu_lookup_t gui_cmd_menu_lookup[]={
	{"zap_status_menu",zap_status_menu},
	{".",NULL}
};


char ** ZAPget_main_menu(int *len)
{
	int i=0;
	while(strcmp(gui_main_menu[i],".") != 0){
		i++;
	}
	*len=i/2;
	return gui_main_menu;
}

char ** ZAPget_cmd_menu(char *cmd_name,int *len)
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

/******************************************************************************
 * 			FUNCTION DEFINITION				      *
 *****************************************************************************/

int ZAPUsage(void)
{
	printf("wanpipemon: Wanpipe/Zaptel Debugging Utility\n\n");
	printf("Usage:\n");
	printf("------\n\n");
	printf("wanpipemon -zap -c <command>\n\n");
	printf("wanpipemon -dahdi -f <filename> -c <command>\n\n");
	printf("\tOption -c: \n");
	printf("\t\tCommand is split into two parts:\n"); 
	printf("\t\t\tFirst letter is a command and the rest are options:\n"); 
	printf("\t\t\tex: ses = Send Echo Spike and collect sample.\n\n");
	printf("\tSupported Commands: s=send\n\n");
	printf("\tOption -tdmvchan: zaptel/dahdi channel number.\n\n");
	printf("\tCommand:  Options:	Description \n");	
	printf("\t-------------------------------- \n\n");    
	printf("\tSend Echo Spike\n");
	printf("\t   s         es	Send Echo Spike\n");
	printf("\t             esa	Send Echo Spike and collect sample After Echo Canceller\n");
	printf("\n");
	printf("\tExample 1: wanpipemon -zap -c ses -tdmvchan 1 : Send Echo Spike on zap channel 1\n");
	printf("\tExample 1: wanpipemon -dahdi -c ses -tdmvchan 1 : Send Echo Spike on dahdi channel 1\n");
#if 0
	printf("\t\t\tand collect the Echo Spike sample BEFORE echo canceller.\n");
	printf("\tExample 2: wanpipemon -zap -c sesa -zapchan 1 : Send Echo Spike on zap channel 1\n");
	printf("\t\t\tand collect the Echo Spike sample AFTER echo canceller.\n");
#endif
	printf("\n");
	return 0;
}

int Zap_parse_args(int argc, char* argv[])
{
	int argi = 0;

	/* default name */
		
	memset(&spike_fout[0],0,SPIKE_FNAME_LEN);
	for(argi = 1; argi < argc; argi++){

		char *parg = argv[argi];
		if (strcmp(parg, "--file") == 0){

			if (argi + 1 >= argc ){
				printf("ERROR: Filename is missing!\n");
				return -EINVAL;				
			}
			strcpy(&spike_fout[0], argv[argi+1]);
		}
	}
	return 0;
}

int ZAPMain(char *command,int argc, char* argv[])
{
	char 		*opt=&command[1];
	unsigned char	sample_mode = WAN_SPIKE_SAMPLE_NONE;
	int		err=0;

	err = Zap_parse_args(argc, argv);
	if (err){
		ZAPUsage();
		goto zapmain_done;
	}

	switch(command[0]){
	case 's':
		if (!strcmp(opt,"es")){	
			/* send Echo Spike. Collect sample BEFORE Software Echo Canceller. */
			sample_mode = WAN_SPIKE_SAMPLE_BEFORE_EC;
		}else if(!strcmp(opt,"esa")){
			/* send Echo Spike. Collect sample AFTER Software Echo Canceller. */
			sample_mode = WAN_SPIKE_SAMPLE_AFTER_EC;
		}else{
			printf("ERROR: Invalid 'spike' command.\n\n");
			ZAPUsage();
			goto zapmain_done;
		}
		break;
	default:
		printf("ERROR: Invalid Command.\n\n");
		ZAPUsage();
		goto zapmain_done;
		break;
	}/*switch*/

	err = send_echo_spike(tdmv_chan, sample_mode);
	if (err){
		goto zapmain_done;	
	}
	/* wait for 1 second for echo to return and get the collected sample */
	usecsleep(1, 0);
	get_echo_spike(tdmv_chan, sample_mode);
	
zapmain_done:
	if(zfd != 0){
		close(zfd);
	}

   	printf("\n");
   	fflush(stdout);
   	return 0;
}/*ZAPMain()*/

int MakeZapConnection()
{
	char *fn = NULL;

	fn = (zap_monitor) ? zap_fn : dahdi_fn;
	zfd = open(fn, O_RDWR | O_NONBLOCK);
	if (zfd < 0) {
		printf("Unable to open '%s': %s. Check Zaptel is loaded.\n",
			fn, strerror(errno));
		return WAN_FALSE;
	}
	return WAN_TRUE;
}

static int zap_do_command(int echo_detect_ioctl, int channo, void *ioctl_result)
{
	/* set the 'channo' in the user buffer */
	*(int*)ioctl_result = channo;

	if (ioctl(zfd, echo_detect_ioctl, ioctl_result)){
		perror("do_echo_detect_ioctl():\n");
		return 1;
	}
	return 0;
}

static int usecsleep(int sec, int usecs)
{
	struct timeval tv;
	tv.tv_sec = sec;
	tv.tv_usec = usecs;
	return select(0, NULL, NULL, NULL, &tv);
}

static int 
write_spike_buffer_to_file(wan_espike_hdr_t *spike_hdr, int *linearized_buff)
{
	FILE 			*file;
	int 			ind;
	char 			strbuff[50];
		
	/* 'fir' - finite impulse response */
	if (!strlen(spike_fout)){
		snprintf(spike_fout, SPIKE_FNAME_LEN, 
			"%s.%s.span%d.chan%d.txt", 
			ZAPMON_SPIKE_FNAME_DEF, 
			(spike_hdr->sample_mode == WAN_SPIKE_SAMPLE_AFTER_EC) ? "after_ec":"before_ec",
			spike_hdr->spanno, spike_hdr->channo);
	}

	/* trancate and open the file for writing */
	file = fopen(spike_fout, "w");
	if(file == NULL){
    		printf("ERROR: Failed to open %s file for writing!\n", spike_fout);
		return 1;
  	}

	for(ind = 0; ind < spike_hdr->sample_len; ind++){
		snprintf(strbuff, 50, "%d\n", linearized_buff[ind]);
  		fputs(strbuff, file);
	}/* for() */
	fclose(file);

	printf("Echo spike was written to '%s' file.\n", spike_fout);

	return 0;
}

static int send_echo_spike(int channo, unsigned char sample_mode)
{
	wan_espike_hdr_t 	spike_hdr;

	printf("Sending Echo Spike on channel %d.\n", channo);

	spike_hdr.sample_mode = sample_mode;

	if(zap_do_command(SEND_ECHO_SPIKE, channo, (void*)&spike_hdr) != 0){
		printf("ERROR: Command failed on channel %d!\n", channo);
		printf("Please check kernel messages log.\n"); 
		return 1;
	}
	if (spike_hdr.return_code == WAN_SPIKE_RC_NO_ACTIVE_CALL){
		printf("ERROR: Command failed on channel %d!\n", channo);
		printf("Possible reasons:\n");
		printf("\t1. The 'echocancel' is set to 'no'\n");
		printf("\t2. Call is not up.\n");
		return 2;
	}
	/* no error. do nothing. may print something but don't have to. */
	printf("Sent Echo Spike. Collecting sample %s SW Echo Canceller...\n",
				WAN_SPIKE_SAMPLE_DECODE(spike_hdr.sample_mode));
	return 0;
}

static int get_echo_spike(int channo, unsigned char sample_mode)
{
	wan_espike_hdr_t 	*spike_hdr = (wan_espike_hdr_t*)&spike_info[0];
	unsigned char 		*spike_data = &spike_info[sizeof(wan_espike_hdr_t)];
	int			linearized_buff[WAN_SPIKE_MAX_SAMPLE_LEN];
	
	if (zap_do_command(GET_ECHO_SPIKE_SAMPLE, channo, spike_info)){
		return 1;
	}

	if (spike_hdr->return_code != WAN_SPIKE_RC_OK){
		printf("ERROR: Command failed on channel %d!\n", channo);
		printf("Possible reasons:\n");
		printf("\t1. The 'echocancel' is set to 'no'\n");
		printf("\t2. Call is not up.\n");
		return 1;
	}

	if (spike_hdr->sample_len < WAN_SPIKE_MAX_SAMPLE_LEN){
                printf("\n");
                printf("WARNING: Number of recorded samples are too small (%d samples)!\n",
                                spike_hdr->sample_len);
                printf("Possible reasons:\n");
                printf("\t1. The 'echocancel' is set to 'no'\n");
                printf("\t2. Hardware echo canceller is enabled.\n");
                return 1;
        }

	linearize_spike_buffer(	spike_hdr, spike_data, linearized_buff);

	write_spike_buffer_to_file( spike_hdr, linearized_buff);
		
	summary_of_linear_spike_buffer(	spike_hdr, linearized_buff );
	return 0;
}

static void linearize_spike_buffer(wan_espike_hdr_t *spike_hdr, unsigned char *spike_data, int *linearized_buff)
{
	int		ind;
	unsigned short 	*linear_table;

	if (spike_hdr->law == ZT_LAW_MULAW){
		printf("Using MULAW table...\n");
		linear_table = u2s;
	}else{
		printf("Using ALAW table...\n");
		linear_table = a2s;
	}
	
	memset(linearized_buff, 0x00, WAN_SPIKE_MAX_SAMPLE_LEN);

	for(ind = 0; ind < spike_hdr->sample_len; ind++){
		if(linear_table[spike_data[ind]] == 0){
			linearized_buff[ind] = 0;
		}else if(spike_data[ind] < 128){
			/* make it negative */
			linearized_buff[ind] = -65535 + linear_table[spike_data[ind]];
		}else{
			linearized_buff[ind] = linear_table[spike_data[ind]];
		}
	}/* for() */
}

static void 
summary_of_linear_spike_buffer(wan_espike_hdr_t *spike_hdr, int *linearized_buff)
{
	int i;

	for(i = 0; i < spike_hdr->sample_len; i ++) {
		proc_sample(linearized_buff[i], i);
	}
	analyze_db_loss(spike_hdr);
}

static void proc_sample(unsigned short linear, int sample_no)
{
	unsigned short val;
	float d;
	float db;
	int neg_voltage = NO;

	db_loss[sample_no].linear_power_value = linear;

	if(linear & 0xF000) {
		neg_voltage = YES;
		val = linear ^ 0xFFFF;
	} else {
		val = linear;
	}
		
	val = val >> 3;
	val &= 0x0FFF;
	d = (float)val / (float)(INITIAL_LINEAR_TX_SPIKE >> 3);

	if(d != 0.00) {
		db = 20 * log10(d);
	} else {
		db = -DB_SILENCE;
	}

	db_loss[sample_no].db_loss = db;

/*	printf("sample[%04d]%2.5f\n", sample_no, db); */
	return;
}

static void analyze_db_loss(wan_espike_hdr_t *spike_hdr)
{
	float min_db_loss = -DB_SILENCE;
	int min_db_loss_sample_no=0;
	float first_db_loss_th;
	int sample_no_first_db_loss_th = 0xFFFF;
	float last_db_loss_th;
	int sample_no_last_db_loss_th = 0xFFFF;
	int i;

	for(i = 0; i < spike_hdr->sample_len; i ++) {
		if(db_loss[i].db_loss > min_db_loss) {
			min_db_loss = db_loss[i].db_loss;
			min_db_loss_sample_no = i;
		}
		if(sample_no_first_db_loss_th == 0xFFFF) {
			if(db_loss[i].db_loss > DB_LOSS_THRESHOLD) {
				sample_no_first_db_loss_th = i;
				first_db_loss_th = db_loss[i].db_loss;
			}
		}
		if(db_loss[i].db_loss > DB_LOSS_THRESHOLD) {
			last_db_loss_th = db_loss[i].db_loss;
			sample_no_last_db_loss_th = i;
		}
	}

	if(min_db_loss == -DB_SILENCE) {
		printf("The line is silent. There is NO Echo on this call.\n");
	}else if(sample_no_first_db_loss_th == 0xFFFF || sample_no_last_db_loss_th == 0xFFFF){

		if (spike_hdr->sample_mode == WAN_SPIKE_SAMPLE_AFTER_EC){
			printf("Sample Analysis conclusion: There is NO Echo on this call. OR\n");
			printf("\tIt was removed by the Echo Canceller.\n");
		}else{
			printf("Sample Analysis conclusion: There is NO Echo on this call.\n");
		}
 	}else {
		printf("\nEcho Spike reached it's maximum at sample %d, linear power is %d\n", 
			min_db_loss_sample_no + 1,
			db_loss[min_db_loss_sample_no].linear_power_value);

		printf("First sample containing Echo Spike is %d, linear power is %d\n", 
			sample_no_first_db_loss_th + 1,
			db_loss[sample_no_first_db_loss_th].linear_power_value);

		printf("Last sample containing Echo Spike is %d, linear power is %d\n", 
			sample_no_last_db_loss_th + 1,
			db_loss[sample_no_last_db_loss_th].linear_power_value);

		/* suggest something depending on number of Echo Cancellor taps */
		printf("\nYour Echo Cancellor set to use %d taps.\n", spike_hdr->echocancel);
		if(spike_hdr->echocancel < sample_no_first_db_loss_th + 1){
			printf("It can not cancel the echo because delay is longer than %d taps.\n",
				spike_hdr->echocancel);	
	
		}else if(spike_hdr->echocancel > sample_no_first_db_loss_th + 1 &&
		    spike_hdr->echocancel < sample_no_last_db_loss_th + 1){
			printf("Echo starts WITHIN %d taps, but ends AFTER %d taps, it means\n\
echo can not be cancelled completely.\n",
				spike_hdr->echocancel, 
				spike_hdr->echocancel);	
		}else{
			printf("Echo starts and ends WITHIN %d taps, it should be cancelled properly.\n",
				spike_hdr->echocancel);	
		}
	}	
}
