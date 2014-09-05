/*****************************************************************************
* fe_lib.c	Front End Library 
*
* Authors:	Nenad Corbic <ncorbic@sangoma.com>	
*			Alex Feldman <al.feldman@sangoma.com>
*			David Rokhvarg <davidr@sangoma.com>
*
* Copyright:	(c) 1995-2000 Sangoma Technologies Inc.
*
*		This program is free software; you can redistribute it and/or
*		modify it under the terms of the GNU General Public License
*		as published by the Free Software Foundation; either version
*		2 of the License, or (at your option) any later version.
* ----------------------------------------------------------------------------
* Feb 08, 2010  David Rokhvarg  Added decoding of "LIU Dual Loopback"
* 								Fixed parsing of '--chan' parameter for BERT
*								test and also it is shifter left by one to be
*								in synch with the kernel-mode code which
*								usese 'bert->un.cfg.chan_map'.
*								Stopping a BERT test will NOT automatically
*								disable Loopback modes.
*
* Feb 06, 2009  David Rokhvarg  Ported to MS Windows and added Makefile.Windows
* 								to compile into SangomaFeLib.lib
*
* Jan 11, 2005  David Rokhvarg  Added code to run above AFT card with protocol
* 								in the LIP layer. Fixed many not working options.
*
* May 24, 2000  Gideon Hack     Modifications for FT1 adapters
*
* Sep 21, 1999  Nenad Corbic    Changed the input parameters, hearders
*                               data types. More user friendly.
*****************************************************************************/

/******************************************************************************
 * 			INCLUDE FILES					      *
 *****************************************************************************/

#if defined(__WINDOWS__)
#include <conio.h>				/* for _kbhit */
#include "wanpipe_includes.h"
#include "wanpipe_defines.h"	/* for 'wan_udp_hdr_t' */
#include "wanpipe_time.h"		/* for 'struct wan_timeval' */
#include "wanpipe_common.h"		/* for 'wp_strcasecmp' */

static int kbdhit(int *key)
{
	_kbhit();
	*key = _getch();
	return *key;
}

#else
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <sys/time.h>
#include <sys/types.h>

#include "unixio.h"
#endif

#include "wanpipe_api.h"
#include "fe_lib.h"
#include "wanpipemon.h"

/******************************************************************************
 * 			DEFINES/MACROS					      *
 *****************************************************************************/
#define TIMEOUT 1
#define NUM_OF_DS3_CHANNELS		672
#define MDATALEN 2024

#define CB_SIZE 	sizeof(wan_mgmt_t) + sizeof(wan_cmd_t) + 1

#if 0
#define CALL_CSU_DSU_FUNC(name)	if (wan_protocol == WANCONFIG_CHDLC){	\
				CHDLC_##name();			\
			}else if (wan_protocol==WANCONFIG_FR || wan_protocol==WANCONFIG_MFR){	\
				FR_##name();			\
			}else if (wan_protocol==WANCONFIG_PPP){	\
				PPP_##name();			\
			}	
#endif	

static int set_lb_modes_status(u_int8_t, u_int8_t, u_int32_t, int);
static int loopback_command(u_int8_t type, u_int8_t mode, u_int32_t);
static int hw_get_femedia_type(wan_femedia_t*);
static int hw_get_fe_type(unsigned char* adapter_type);

unsigned int felib_parse_active_channel_str(char* val);
static unsigned int get_active_channels(int channel_flag, int start_channel, int stop_channel);

/******************************************************************************
 * 			TYPEDEF/STRUCTURE				      *
 *****************************************************************************/


/******************************************************************************
 * 			GLOBAL VARIABLES				      *
 *****************************************************************************/
extern unsigned char 	par_port_A_byte;
extern unsigned char 	par_port_B_byte;
extern int 		gfail;
extern 			FT1_LED_STATUS FT1_LED;
extern int 		wan_protocol;
extern wan_udp_hdr_t	wan_udp;
extern wan_femedia_t	femedia;

/******************************************************************************
 * 			GUI MENU DEFINITION				      *
 *****************************************************************************/

char *csudsu_menu[]={
"","-- TE3/TE1/56K (S514-4-5-7-8/AFT) Stats --",
""," ",
"Ta","Read TE3/TE1/56K alarms",
"Tallb","Activate Line/Remote Loopback T1/E1/DS3/E3",
"Tdllb","Deactivate Line/Remote Loopback T1/E1/DS3/E3",
"Taplb","Activate Payload Loopback T1/E1/DS3/E3",
"Tdplb","Deactivate Payload Loopback T1/E1/DS3/E3",
"Tadlb","Activate Diag Digital Loopback T1/E1/DS3/E3",
"Tddlb","Deactivate Diag Digital Loopback T1/E1/DS3/E3",
"Talalb","Activate LIU Analog Loopback T1/E1-DM",
"Tdlalb","Deactivate LIU Analog Loopback T1/E1-DM",
"Talllb","Activate LIU Local Loopback T1/E1-DM",
"Tdlllb","Deactivate LIU Local Loopback T1/E1-DM",
"Talrlb","Activate LIU Remote Loopback T1/E1-DM",
"Tdlrlb","Deactivate LIU Remote Loopback T1/E1-DM",
"Taldlb","Activate LIU Dual Loopback T1/E1-DM",
"Tdldlb","Deactivate LIU Dual Loopback T1/E1-DM",
"Tsalb","Send Loopback Activate Code",  
"Tsdlb","Send Loopback Deactive Code",  
"Tsaplb","Send Payload Loopback Activate Code",  
"Tsdplb","Send Payload Loopback Deactive Code",  
"Tread","Read CSU/DSU cfg",
""," ",
"","--- FT1 (S508/S5143) Stats  ----",
""," ",
"Tv","View Status",
"Ts","Self Test",
"Tl","Line Loop Test",
"Td","Digital Loop Test",
"Tr","Remote Test",
"To","Operational Mode",
"Tread","Read CSU/DSU cfg",
"."
};

char *csudsu_menu_te3[]={
"","-- DS3/E3 (AFT) Stats --",
""," ",
"Ta","Read DS3/E3 alarms",
"Tallb3","Activate Analog Local Loopback DS3/E3",
"Tdllb3","Deactivate Analog Local Loopback DS3/E3",
"Tarlb3","Activate Remote Loopback DS3/E3",
"Tdrlb3","Deactivate Remote Loopback DS3/E3",
"Tadlb3","Activate Digital Loopback DS3/E3",
"Tddlb3","Deactivate Digital Loopback DS3/E3",
"."
};

char *csudsu_menu_te1_pmc[]={
"","-- T1/E1 (S514-4-5-7-8/AFT) Stats --",
""," ",
"Ta","Read T1/E1 alarms",
"Tallb","Activate Line Loopback T1/E1",
"Tdllb","Deactivate Line Loopback T1/E1",
"Taplb","Activate Payload Loopback T1/E1",
"Tdplb","Deactivate Payload Loopback T1/E1",
"Tadlb","Activate Diag Digital Loopback T1/E1",
"Tddlb","Deactivate Diag Digital Loopback T1/E1",
"Tsalb","Send Loopback Activate Code",  
"Tsdlb","Send Loopback Deactivate Code",  
"Tsaplb","Send Payload Loopback Activate Code",  
"Tsdplb","Send Payload Loopback Deactive Code",  
"Tread","Read CSU/DSU cfg",
""," ",
"","--- FT1 (S508/S5143) Stats  ----",
""," ",
"Tv","View Status",
"Ts","Self Test",
"Tl","Line Loop Test",
"Td","Digital Loop Test",
"Tr","Remote Test",
"To","Operational Mode",
"Tread","Read CSU/DSU cfg",
"."
};

char *csudsu_menu_te1_dm[]={
"","-- T1/E1 (AFT T1/E1-DM) Stats --",
""," ",
"Ta","Read T1/E1 alarms",
"Tallb","Activate Line/Remote Loopback T1/E1",
"Tdllb","Deactivate Line/Remote Loopback T1/E1",
"Taplb","Activate Payload Loopback T1/E1",
"Tdplb","Deactivate Payload Loopback T1/E1",
"Tadlb","Activate Diag Digital Loopback T1/E1",
"Tddlb","Deactivate Diag Digital Loopback T1/E1",
"Talalb","Activate LIU Analog Loopback T1/E1",
"Tdlalb","Deactivate LIU Analog Loopback T1/E1",
"Talllb","Activate LIU Local Loopback T1/E1",
"Tdlllb","Deactivate LIU Local Loopback T1/E1",
"Talrlb","Activate LIU Remote Loopback T1/E1",
"Tdlrlb","Deactivate LIU Remote Loopback T1/E1",
"Taldlb","Activate LIU Dual Loopback T1/E1",
"Tdldlb","Deactivate LIU Dual Loopback T1/E1",
"Tsalb","Send Line Loopback Activate Code",  
"Tsdlb","Send Line Loopback Deactivate Code",  
"Tsaplb","Send Payload Loopback Activate Code",  
"Tsdplb","Send Payload Loopback Deactive Code",  
"Tapclb","Activate Per-channel Loopback T1/E1",  
"Tdpclb","Deactivate Per-channel Loopback T1/E1",  
"."
};

/******************************************************************************
 * 			FUNCTION DEFINITION				      *
 *****************************************************************************/
/* Display the status of all the lights */
void view_FT1_status( void )
{
	int FT1_LED_read_count = 0;
	int key;
	struct wan_timeval to;
	long curr_sec;

	printf("The FT1 status is depicted by the eight LEDs shown below. ");
        printf("LED colours are: \n");
        printf("   R=Red   G=Green   O=Orange   Blank=Off\n");

	printf("INS (In-service)   :solid green=unit is in service, ");
	printf("flashing red=red alarm,\n");
	printf("                    flashing orange=yellow alarm, ");
	printf("flashing green=blue alarm\n");

	printf("ERR (Error)        :off=no line errors, ");
	printf("red=Severely Errored Seconds,\n");
	printf("                    orange=Bursty Errored Seconds, ");
	printf("green=Errored Seconds\n");
     
	printf("TxD (Transmit Data):flashing green=transmit data is present\n");

	printf("RxD (Receive Data) :flashing green=receive data is present\n");

	printf("ST (Self Test)     Note: line must be disconnected for this test.\n"); 
	printf("                    off=Self Test not selected, ");
        printf("flashing red=awaiting result of\n");
        printf("                    Self Test, flashing green=test passed\n");

	printf("DL (Digital Loop)  :off=Digital Loop not selected, ");
	printf("flashing red=Digital Loop\n");
	printf("                    selected\n");

	printf("LL (Local Loop)    :off=Local Loop not selected, ");
	printf("flashing red=Local Loop\n");
	printf("                    selected\n");

	printf("RT (Remote Test)   Note: will only work if remote card is a Sangoma FT1 card.\n"); 
	printf("                    off=Remote Test not selected, ");
	printf("flashing red=awaiting response\n");
	printf("                    from remote station, ");
	printf("solid green=remote station running\n");
	printf("                    test, flashing green=valid response received\n\n");
  
	printf("Press <ESC> to exit   Press <M> to change FT1 mode\n");   
        printf("\n  INS   ERR   TxD   RxD    ST    DL    LL    RT\n");
 
	memset(&FT1_LED, 0, sizeof(FT1_LED_STATUS));
	wp_gettimeofday(&to, NULL);
	curr_sec = to.tv_sec;
	
	/* loop and display the FT1 LEDs until the <ESC> or <M> key is hit */
	for(;;) {
               if(kbdhit(&key)) {
			/* <M> hit, so change the FT1 mode */
			if(toupper((char)key) == 'M') {
				/*set_FT1_mode(WAN_FALSE);*/
				EXEC_PROT_VOID_FUNC(set_FT1_mode,wan_protocol,());
                                printf("       Current mode:             ");
				/* delay 1/10th sec to let FT1 settle down */
 				wp_usleep(100000);
				wp_gettimeofday(&to, NULL);
				curr_sec = to.tv_sec;
				memset(&FT1_LED, 0, sizeof(FT1_LED_STATUS));
			}
			/* <ESC> hit, so exit */
                        if((char)key == 0x1b) {
				printf("\n\nChecking current FT1 status...\n");
				memset(&FT1_LED, 0, sizeof(FT1_LED_STATUS));
				FT1_LED_read_count = 0;
				/* read the FT1 status for 1.5 seconds to */
				/* find out if we are in-service */
        			for(;;) {
			                EXEC_PROT_VOID_FUNC(read_FT1_status,wan_protocol,());
			                if((++ FT1_LED_read_count) == 30) {
						break;
					}
					wp_usleep(50000);
				}
				if(FT1_LED.ST_red || FT1_LED.ST_green ||
					FT1_LED.DL_red || FT1_LED.LL_red ||
					FT1_LED.RT_red || FT1_LED.RT_green) {
					printf("The FT1 is not currently in the operational mode and the unit is not in-service.\n");
					printf("Do you wish to return to the operational mode? (Y/N)\n");
					fflush(stdout);
					for(;;) {
						kbdhit(&key);
						if(toupper((char)key) == 'Y') {
							FT1_operational_mode();
							break;
						}
						if(toupper((char)key) == 'N') {
							break;
						}
					}
				}
				break;
                        }
		}
   
		EXEC_PROT_VOID_FUNC(read_FT1_status,wan_protocol,());
		display_FT1_LEDs();
		/* update the current FT1 status at 1 second intervals */
		wp_gettimeofday(&to, NULL);
		if(curr_sec != to.tv_sec) {
		        curr_sec = to.tv_sec;
			printf("   |  Current mode: | ");
			if(FT1_LED.ST_red || FT1_LED.ST_green) {
				printf("Self Test   ");
			} else if(FT1_LED.DL_red) {
				printf("Digital Loop");
			} else if(FT1_LED.LL_red) {
                                printf("Local Loop  ");
                        } else if(FT1_LED.RT_red || FT1_LED.RT_green) {
                                printf("Remote Test ");
                        } else {
				printf("In-service  ");
			}
			//printf(" | ");
        		fflush(stdout);
			memset(&FT1_LED, 0, sizeof(FT1_LED_STATUS));
		}
	}

	printf("Preparing FT1 LED summary...\n");
	memset(&FT1_LED, 0, sizeof(FT1_LED_STATUS));
	FT1_LED_read_count = 0;

	/* read the FT1 status for 2.0 seconds find the current LED settings */
	for(;;) {
                EXEC_PROT_VOID_FUNC(read_FT1_status,wan_protocol,());
		if((++ FT1_LED_read_count) == 40) {
                        break;
                }
		wp_usleep(50000);
        }

	printf("\nINS (In-service)   : ");
	if(FT1_LED.INS_green && !FT1_LED.INS_red && !FT1_LED.INS_off) {
		printf("Solid Green - Unit is in service");
	} else if (FT1_LED.INS_red && !FT1_LED.INS_green && FT1_LED.INS_off) {
		printf("Flashing Red - Red alarm");
	} else if (FT1_LED.INS_red && FT1_LED.INS_green && FT1_LED.INS_off) {
		printf("Flashing Orange - Yellow alarm");
	} else if (FT1_LED.INS_green && !FT1_LED.INS_red && FT1_LED.INS_off) {
		printf("Flashing Green - Blue alarm");
        } else {
		printf("Unknown state - Red %u, Green %u, Off %u",
			FT1_LED.INS_red, FT1_LED.INS_green, FT1_LED.INS_off);
	}

	printf("\nERR (Error)        : ");
	if(!FT1_LED.ERR_green && !FT1_LED.ERR_red && FT1_LED.ERR_off) {
                printf("Off - No line errors received");
	} else if(FT1_LED.ERR_red && !FT1_LED.ERR_green && !FT1_LED.ERR_off) {
		printf("Solid Red - Severely Errored Seconds");
	} else if(FT1_LED.ERR_red && FT1_LED.ERR_green) {
                printf("Orange - Bursty Errored Seconds");
	} else if(!FT1_LED.ERR_red && FT1_LED.ERR_green) {
		printf("Green - Errored Seconds");
	} else {
		printf("Unknown state - Red %u, Green %u, Off %u",
			FT1_LED.ERR_red, FT1_LED.ERR_green, FT1_LED.ERR_off);
	}
 
	printf("\nTxD (Transmit Data): ");
	if(FT1_LED.TxD_green) {
		printf("Flashing Green - Transmit data is present");
	} else {
                printf("Off - Transmit data is not present");
	}

	printf("\nRxD (Receive Data) : ");
	if(FT1_LED.RxD_green) {
		printf("Flashing Green - Receive data is present");
	} else {
		printf("Off - Receive data is not present");
	}

	printf("\nST (Self Test)     : ");
	if(!FT1_LED.ST_green && !FT1_LED.ST_red && FT1_LED.ST_off) {
                printf("Off");
 	} else if(FT1_LED.ST_red && !FT1_LED.ST_green && FT1_LED.ST_off) {
		printf("Flashing Red - Awaiting confirmation of self test");
	} else if(FT1_LED.ST_green && !FT1_LED.ST_red && FT1_LED.ST_off) {
                printf("Flashing Green - Self test has passed");
	} else {
		printf("Unknown state - Red %u, Green %u, Off %u",
			FT1_LED.ST_red, FT1_LED.ST_green, FT1_LED.ST_off);
	}

	printf("\nDL (Digital Loop)  : ");
	if(!FT1_LED.DL_red && FT1_LED.DL_off) {
                printf("Off");
  	} else if (FT1_LED.DL_red && FT1_LED.DL_off) {
		printf("Flashing Red - Bi-directional digital loop selected");
	}
	else {
		printf("Unknown state - Red %u, Off %u",
                        FT1_LED.DL_red, FT1_LED.DL_off);
  	}

	printf("\nLL (Local Loop)    : ");
	if(!FT1_LED.LL_red && FT1_LED.LL_off) {
		printf("Off");
	} else if (FT1_LED.LL_red && FT1_LED.LL_off) {
		printf("Flashing Red - Line loop selected");
	}
	else {
		printf("Unknown state - Red %u, Off %u",
			FT1_LED.LL_red, FT1_LED.LL_off);
	}

        printf("\nRT (Remote Test)   : ");
        if(!FT1_LED.RT_green && !FT1_LED.RT_red && FT1_LED.RT_off) {
                printf("Off");
        } else if(FT1_LED.RT_red && !FT1_LED.RT_green && FT1_LED.RT_off) {
                printf("Flashing Red - Awaiting response from remote station");
        } else if(FT1_LED.RT_green && !FT1_LED.RT_red && !FT1_LED.RT_off) {
                printf("Solid Green - Remote station running test");
	} else if(FT1_LED.RT_green && !FT1_LED.RT_red && FT1_LED.RT_off) {
                printf("Flashing Green - Valid response received");
        } else {
                printf("Unknown state - Red %u, Green %u, Off %u",
                        FT1_LED.RT_red, FT1_LED.RT_green, FT1_LED.RT_off);
        }
 
}/* view_FT1_status */

void display_FT1_LEDs(void)
{

	printf("\r| ");

	/* display INS LED */
 	switch (par_port_B_byte & (PP_B_INS_NOT_GREEN | PP_B_INS_NOT_RED)) {
		case (PP_B_INS_NOT_GREEN | PP_B_INS_NOT_RED):
			putchar(' ');
			break;
		case (!(PP_B_INS_NOT_GREEN | PP_B_INS_NOT_RED)):
			putchar('O');
			break;
		case (PP_B_INS_NOT_GREEN):
			putchar('R');
			break;
                case (PP_B_INS_NOT_RED):
                        putchar('G');
                        break;
		default:
			break;
	}

	/* display ERR LED */
	printf("  |  ");
	switch (par_port_B_byte & (PP_B_ERR_NOT_GREEN | PP_B_ERR_NOT_RED)) {
                case (PP_B_ERR_NOT_GREEN | PP_B_ERR_NOT_RED):
                        putchar(' ');
                        break;
                case (!(PP_B_ERR_NOT_GREEN | PP_B_ERR_NOT_RED)):
                        putchar('O');
                        break;
                case (PP_B_ERR_NOT_GREEN):
                        putchar('R');
                        break;
                case (PP_B_ERR_NOT_RED):
                        putchar('G');
                        break;
                default:
                        break;
        }

	/* display TxD LED */
	printf("  |  ");
 	switch (par_port_B_byte & PP_B_TxD_NOT_GREEN) {
                case (PP_B_TxD_NOT_GREEN):
                        putchar(' ');
                        break;
                case (!PP_B_TxD_NOT_GREEN):
                        putchar('G');
                        break;
                default:
                        break;
        }

	/* display RxD LED */
        printf("  |  ");
        switch (par_port_B_byte & PP_B_RxD_NOT_GREEN) {
                case (PP_B_RxD_NOT_GREEN):
                        putchar(' ');
                        break;
                case (!PP_B_RxD_NOT_GREEN):
                        putchar('G');
                        break;
                default:
                        break;
        }

	/* display ST LED */
	printf("  |  ");
        switch (par_port_B_byte & (PP_B_ST_NOT_GREEN | PP_B_ST_NOT_RED)) {
                case (PP_B_ST_NOT_GREEN | PP_B_ST_NOT_RED):
                        putchar(' ');
                        break;
                case (!(PP_B_ST_NOT_GREEN | PP_B_ST_NOT_RED)):
                        putchar('O');
                        break;
                case (PP_B_ST_NOT_GREEN):
                        putchar('R');
                        break;
                case (PP_B_ST_NOT_RED):
                        putchar('G');
                        break;
                default:
                        break;
        }

	/* display DL LED */
        printf("  |  ");
        switch (par_port_A_byte & PP_A_DL_NOT_RED) {
                case (PP_A_DL_NOT_RED):
                        putchar(' ');
                        break;
                case (!PP_A_DL_NOT_RED):
                        putchar('R');
                        break;
                default:
                        break;
        }

	/* display LL LED */
        printf("  |  ");
        switch (par_port_A_byte & PP_A_LL_NOT_RED) {
                case (PP_A_LL_NOT_RED):
                        putchar(' ');
                        break;
                case (!PP_A_LL_NOT_RED):
                        putchar('R');
                        break;
                default:
                        break;
        }

        /* display RT LED */
        printf("  |  ");
        switch (par_port_A_byte & (PP_A_RT_NOT_GREEN | PP_A_RT_NOT_RED)) {
                case (PP_A_RT_NOT_GREEN | PP_A_RT_NOT_RED):
                        putchar(' ');
                        break;
                case (!(PP_A_RT_NOT_GREEN | PP_A_RT_NOT_RED)):
                        putchar('O');
                        break;
                case (PP_A_RT_NOT_GREEN):
                        putchar('R');
                        break;
                case (PP_A_RT_NOT_RED):
                        putchar('G');
                        break;
                default:
                        break;
        }
	
	fflush(stdout);
}



void FT1_operational_mode(void)
{

	int FT1_LED_read_count = 0;
        int op_mode_search_count = 0;
 
	printf("\nSetting FT1 into operational mode...");
	fflush(stdout);

        memset(&FT1_LED, 0, sizeof(FT1_LED_STATUS));

 	/* change the FT1 mode at 1 second intervals until we enter the */
	/* operational (in-service) mode */
        for(;;) {
                EXEC_PROT_VOID_FUNC(read_FT1_status,wan_protocol,());
                wp_usleep(50000);
                if((++ FT1_LED_read_count) == 20) {
                        if(FT1_LED.ST_red || FT1_LED.ST_green || FT1_LED.DL_red
				|| FT1_LED.LL_red || FT1_LED.RT_red
				|| FT1_LED.RT_green) {
                                if((++ op_mode_search_count) == 10) {
					printf("Failed");
					printf("\nCould not enter FT1 operational mode\n");
					if(remote_running_RT_test()) {
						return;
					}
					printf("Please contact your Sangoma representative\n");
					return;
                                }
                                EXEC_PROT_VOID_FUNC(set_FT1_mode,wan_protocol,());
                                /* delay 1/10th sec to let FT1 settle down */
                                wp_usleep(100000);
                                memset(&FT1_LED, 0, sizeof(FT1_LED_STATUS));
                                FT1_LED_read_count = 0;
                        } else {
                                printf("Done\n");
                                break;
                        }
                }
        }

} /* FT1_operational_mode */


void FT1_self_test(void)
{

        int FT1_LED_read_count = 0;
	int ST_mode_search_count = 0;
 
	printf("\nSetting FT1 for Self Test mode...");
	fflush(stdout);
 
        memset(&FT1_LED, 0, sizeof(FT1_LED_STATUS));

	/* change the FT1 mode at 1 second intervals until we enter ST mode */
        for(;;) {
                EXEC_PROT_VOID_FUNC(read_FT1_status,wan_protocol,());
                wp_usleep(50000);
                if((++ FT1_LED_read_count) == 20) {
                        if(!FT1_LED.ST_red) {
				if((++ ST_mode_search_count) == 10) {
					printf("Failed");
					printf("\nCould not enter Self Test mode\n");
                                        if(remote_running_RT_test()) {
                                                return;
					}
 					printf("Remove T1 connection from adapter and repeat the Self Test\n");
					FT1_operational_mode();
					return;
				}
				EXEC_PROT_VOID_FUNC(set_FT1_mode,wan_protocol,());
                                /* delay 1/10th sec to let FT1 settle down */
                                wp_usleep(100000);
				memset(&FT1_LED, 0, sizeof(FT1_LED_STATUS));
				FT1_LED_read_count = 0;
			} else {
				printf("Done \nSelf Test started...\n");
				break;
			}
                }
        }
 
        memset(&FT1_LED, 0, sizeof(FT1_LED_STATUS));
	FT1_LED_read_count = 0;

	/* if the ST LED is not green within 6 seconds then we fail this test */
        for(;;) {
                EXEC_PROT_VOID_FUNC(read_FT1_status,wan_protocol,());
		if(FT1_LED.ST_green) {
			printf("Self Test passed\n");
			break;
		}
                wp_usleep(50000);
                if((++ FT1_LED_read_count) == 120) {
			printf("Self Test failed\n");
			printf("Please contact your Sangoma representative\n");
                        break;
                }
        }

	FT1_operational_mode();

} /* FT1_self_test */


void FT1_digital_loop_mode( void )
{

        int FT1_LED_read_count = 0;
        int DL_mode_search_count = 0;

        printf("\nSetting FT1 for Bi-Directional Digital Loop mode...");
	fflush(stdout);

        memset(&FT1_LED, 0, sizeof(FT1_LED_STATUS));

        /* change the FT1 mode at 1 second intervals until we enter DL mode */
        for(;;) {
                EXEC_PROT_VOID_FUNC(read_FT1_status,wan_protocol,());
                wp_usleep(50000);
                if((++ FT1_LED_read_count) == 20) {
                        if(!FT1_LED.DL_red) {
                                if((++ DL_mode_search_count) == 10) {
                                        printf("Failed");
					printf("\nCould not enter Digital Loop mode\n");
                                        if(remote_running_RT_test()) {
                                                return;
					}
                                        FT1_operational_mode();
                                        return;
                                }
                                EXEC_PROT_VOID_FUNC(set_FT1_mode,wan_protocol,());
                                /* delay 1/10th sec to let FT1 settle down */
                                wp_usleep(100000);
                                memset(&FT1_LED, 0, sizeof(FT1_LED_STATUS));
                                FT1_LED_read_count = 0;
                        } else {
				printf("Done");
                                break;
                        }
                }
        }

	printf("\n\nIMPORTANT: This FT1 device is in the Digital Loop mode and is not in service.\n\n");

} /* FT1_digital_loop_mode */


void FT1_local_loop_mode( void )
{

        int FT1_LED_read_count = 0;
        int LL_mode_search_count = 0;

        printf("\nSetting FT1 for Local Loop mode...");
	fflush(stdout);

        memset(&FT1_LED, 0, sizeof(FT1_LED_STATUS));

	/* change the FT1 mode at 1 second intervals until we enter LL mode */
        for(;;) {
                EXEC_PROT_VOID_FUNC(read_FT1_status,wan_protocol,());
                wp_usleep(50000);
                if((++ FT1_LED_read_count) == 20) {
                        if(!FT1_LED.LL_red) {
                                if((++ LL_mode_search_count) == 10) {
					printf("Failed");
                                        printf("\nCould not enter Local Loop mode\n");
                                        if(remote_running_RT_test()) {
                                                return;
                                        }
                                        printf("Remove T1 connection from adapter before entering the Local Loop mode\n");
                                        FT1_operational_mode();
                                        return;
                                }
                                EXEC_PROT_VOID_FUNC(set_FT1_mode,wan_protocol,());
                                /* delay 1/10th sec to let FT1 settle down */
                                wp_usleep(100000);
                                memset(&FT1_LED, 0, sizeof(FT1_LED_STATUS));
                                FT1_LED_read_count = 0;
                        } else {
				printf("Done");
                                break;
                        }
                }
	}

	printf("\n\nIMPORTANT: This FT1 device is in the Local Loop mode and is not in service.\n\n");
 
} /* FT1_local_loop_mode */

void FT1_remote_test( void )
{

        int FT1_LED_read_count = 0;
        int RT_mode_search_count = 0;

        printf("\nSetting FT1 for Remote Test mode...");
	fflush(stdout);

        memset(&FT1_LED, 0, sizeof(FT1_LED_STATUS));

	/* change the FT1 mode at 1 second intervals until we enter RT mode */
        for(;;) {
                EXEC_PROT_VOID_FUNC(read_FT1_status,wan_protocol,());
                wp_usleep(50000);
                if((++ FT1_LED_read_count) == 20) {
                        if(!FT1_LED.RT_red) {
                                if((++ RT_mode_search_count) == 10) {
					printf("Failed");
                                        printf("\nCould not enter Remote Test mode\n");
                                        if(remote_running_RT_test()) {
                                                return;
 					}
 					printf("Please contact your Sangoma representative\n");
                                        FT1_operational_mode();
                                        return;
                                }
                                EXEC_PROT_VOID_FUNC(set_FT1_mode,wan_protocol,());
                                /* delay 1/10th sec to let FT1 settle down */
                                wp_usleep(100000);
                                memset(&FT1_LED, 0, sizeof(FT1_LED_STATUS));
                                FT1_LED_read_count = 0;
                        } else {
                                printf("Done \nRemote Test started...");
				fflush(stdout);
                                break;
                        }
                }
        }

	/* delay 10 seconds after starting the RT before checking the result */
	for(FT1_LED_read_count = 0; FT1_LED_read_count < 10;
		FT1_LED_read_count ++) {
		wp_usleep(1000000);
    		printf(".");
		fflush(stdout);
       	}

        memset(&FT1_LED, 0, sizeof(FT1_LED_STATUS));
        FT1_LED_read_count = 0;

	/* the RT LED must be flashing green to pass this test, so read the */
	/* RT LEDs for the next 2 seconds to get the test result */
        for(;;) {
                EXEC_PROT_VOID_FUNC(read_FT1_status,wan_protocol,());
                if(FT1_LED.RT_green && FT1_LED.RT_off) {
                        printf("\nRemote Test passed\n");
                        break;
                }
                wp_usleep(50000);
                if((++ FT1_LED_read_count) == 40) {
                        printf("\nRemote Test failed\n");
			printf("Please ensure that the remote CSU/DSU is a Sangoma FT1 card.\n");
                        break;
                }
        }

        FT1_operational_mode();
  
} /* FT1_remote_test */



int remote_running_RT_test(void)
{
	/* if the RT LED is on solid green, then the remote end is running */
	/* the RT test */
	if(FT1_LED.RT_green && !FT1_LED.RT_off) {
		printf("\nThe remote Sangoma card is running the Remote Test\n");
		printf("Exit the Remote Test at the remote end\n\n");
		return(1);
	}
	return(0);
}

static int hw_get_femedia_type(wan_femedia_t *fe_media)
{
	/* Read Adapter Type */
	wan_udp.wan_udphdr_command = WAN_GET_MEDIA_TYPE;
	wan_udp.wan_udphdr_data[0] = WAN_MEDIA_NONE;
	wan_udp.wan_udphdr_data_len = 0;
    	wan_udp.wan_udphdr_return_code = 0xaa;
	DO_COMMAND(wan_udp);
	if (wan_udp.wan_udphdr_return_code != 0){
		printf("Failed to read Adapter Type.\n");
		return 1;
	}

	memcpy((void*)fe_media, get_wan_udphdr_data_ptr(0), sizeof(wan_femedia_t));
	return 0;
}

int get_femedia_type(wan_femedia_t *fe_media)
{
	int rc;
  
	if(make_hardware_level_connection()){
		return 0;
	}
	rc = hw_get_femedia_type(fe_media);

	cleanup_hardware_level_connection();

	return rc;
}


#if 1
static int hw_get_fe_type(unsigned char* adapter_type)
{
	/* Read Adapter Type */
	wan_udp.wan_udphdr_command = WAN_GET_MEDIA_TYPE;
	wan_udp.wan_udphdr_data[0] = WAN_MEDIA_NONE;
	wan_udp.wan_udphdr_data_len = 0;
    	wan_udp.wan_udphdr_return_code = 0xaa;
	DO_COMMAND(wan_udp);
	if (wan_udp.wan_udphdr_return_code != 0){
		printf("Failed to read Adapter Type.\n");
		return 1;
	}
#if 0
	if (wan_protocol == WANCONFIG_CHDLC){ 
		*adapter_type = wan_udp.wan_udphdr_chdlc_data[0];
	}else{
		*adapter_type = wan_udp.wan_udphdr_data[0];
	}
#endif

	*adapter_type =	get_wan_udphdr_data_byte(0);
	return 0;
}

int get_fe_type(unsigned char* adapter_type)
{
	int rc;
  
	if(make_hardware_level_connection()){
		return 0;
	}
	rc = hw_get_fe_type(adapter_type);

	cleanup_hardware_level_connection();
	return rc;
}

#endif

int set_lb_modes(unsigned char type, unsigned char mode)
{
	int 	err = 0;

	err = loopback_command(type, mode, ENABLE_ALL_CHANNELS);

	set_lb_modes_status(type, mode, ENABLE_ALL_CHANNELS, err);

	return err;
}

int set_lb_modes_per_chan(unsigned char type, unsigned char mode, u_int32_t chan_map)
{
	int 	err = 0;

	err = loopback_command(type, mode, chan_map);
	set_lb_modes_status(type, mode, chan_map, err);
	return err;
}

static int set_lb_modes_status(unsigned char type, unsigned char mode, u_int32_t chan_map, int err)
{

	if (femedia.media == WAN_MEDIA_T1 || femedia.media == WAN_MEDIA_E1){
		if (chan_map != ENABLE_ALL_CHANNELS){
			printf("%s %s mode for channels %08X ... %s!\n",
				WAN_TE1_LB_ACTION_DECODE(mode),
				WAN_TE1_LB_MODE_DECODE(type),
				chan_map,
				(!err)?"Done":"Failed");
		}else{
			printf("%s %s mode ... %s!\n",
				WAN_TE1_LB_ACTION_DECODE(mode),
				WAN_TE1_LB_MODE_DECODE(type),
				(!err)?"Done":"Failed");
		}
	}else if (femedia.media == WAN_MEDIA_DS3 || femedia.media == WAN_MEDIA_E3){
		printf("%s %s mode ... %s!\n",
				WAN_TE3_LB_ACTION_DECODE(mode),
				WAN_TE3_LB_TYPE_DECODE(type),
				(!err)?"Done":"Failed");
	}else{
		printf("%s %s mode ... %s (default)!\n",
				WAN_TE1_LB_ACTION_DECODE(mode),
				WAN_TE1_LB_MODE_DECODE(type),
				(!err)?"Done":"Failed");
	}

	printf("LB CMD Return Code: %s (%d)\n", SDLA_DECODE_SANG_STATUS(err), err);

	return err;
}


static int loopback_command(u_int8_t type, u_int8_t mode, u_int32_t chan_map)
{
	sdla_fe_lbmode_t	*lb;
	int			err = 0, cnt = 0;

	if (make_hardware_level_connection()){
		return -EINVAL;
	}

	lb = (sdla_fe_lbmode_t*)get_wan_udphdr_data_ptr(0);
	memset(lb, 0, sizeof(sdla_fe_lbmode_t));
	lb->cmd		= WAN_FE_LBMODE_CMD_SET;
	lb->type	= type;
	lb->mode	= mode;
	lb->chan_map	= chan_map;

lb_poll_again:
	wan_udp.wan_udphdr_command	= WAN_FE_LB_MODE;
	wan_udp.wan_udphdr_data_len	= sizeof(sdla_fe_lbmode_t);
	wan_udp.wan_udphdr_return_code	= 0xaa;
	
	DO_COMMAND(wan_udp);
	
	if (wan_udp.wan_udphdr_return_code){
		err = wan_udp.wan_udphdr_return_code;
		goto loopback_command_exit;
	}

	if (lb->rc == WAN_FE_LBMODE_RC_PENDING){

		if (!cnt) printf("Please wait ..");fflush(stdout);
		if (cnt++ < 10){
			printf(".");fflush(stdout);
			wp_sleep(1);
			lb->cmd	= WAN_FE_LBMODE_CMD_POLL;
			lb->rc	= 0x00;
			goto lb_poll_again;
		}
		err = -EINVAL;
		goto loopback_command_exit;
	}else if (lb->rc != WAN_FE_LBMODE_RC_SUCCESS){
		err = -EINVAL;
	}
	if (cnt) printf("\n");

loopback_command_exit:
	cleanup_hardware_level_connection();
	return err;
}

u_int32_t get_lb_modes(int silent)
{
	sdla_fe_lbmode_t	*lb;

	if ((femedia.media != WAN_MEDIA_T1) && (femedia.media != WAN_MEDIA_E1) &&
	    (femedia.media != WAN_MEDIA_DS3) && (femedia.media != WAN_MEDIA_E3)){
		printf("Error: Unsupported feature for current media type %02X!\n",
						femedia.media);
		return 0;
	}
	if(make_hardware_level_connection()){
		return 0;
	}

	lb = (sdla_fe_lbmode_t*)get_wan_udphdr_data_ptr(0);
	memset(lb, 0, sizeof(sdla_fe_lbmode_t));
	lb->cmd	= WAN_FE_LBMODE_CMD_GET;
	lb->rc	= WAN_FE_LBMODE_RC_FAILED;

	wan_udp.wan_udphdr_command	= WAN_FE_LB_MODE;
	wan_udp.wan_udphdr_data_len	= sizeof(sdla_fe_lbmode_t);
	wan_udp.wan_udphdr_return_code	= 0xaa;

	DO_COMMAND(wan_udp);
	if (wan_udp.wan_udphdr_return_code != 0){
		printf("Failed to get loopback mode!\n");
	}else{

		if (lb->rc != WAN_FE_LBMODE_RC_SUCCESS){
			if (!silent){
				printf("ERROR: Failed to get Loopback mode!\n");
			}
			return 0;
		}

		if (silent) return lb->type_map;
		if (!lb->type_map){
			printf("All loopback modes are disabled!\n");
		}else{
			printf("***** %s: %s Loopback status (0x%08X) *****\n\n",
					if_name,
					(femedia.media == WAN_MEDIA_T1)  ? "T1" :
					(femedia.media == WAN_MEDIA_E1)  ? "E1" :
					(femedia.media == WAN_MEDIA_DS3) ? "DS3" :
					(femedia.media == WAN_MEDIA_E3)  ? "E3" : "Unknown",
					lb->type_map);
			if ((femedia.media == WAN_MEDIA_T1) || (femedia.media == WAN_MEDIA_E1)){
			
				/* LIU LBs */
				if (lb->type_map & (1<<WAN_TE1_LIU_ALB_MODE)){
					/*printf("\tLIU Analog Loopback:\tON\n"); March 31, 2010: report as Diagnostic (Local) LB*/
					printf("\tDiagnostic (Local) Loopback:\tON\n");
				}
				if (lb->type_map & (1<<WAN_TE1_LIU_LLB_MODE)){
					printf("\tLIU Local Loopback:\tON\n");
				}
				if (lb->type_map & (1<<WAN_TE1_LIU_RLB_MODE)){
					/*printf("\tLIU Remote Loopback:\tON\n"); March 31, 2010: report as Line (Remote) LB*/
					printf("\tLine (Remote) Loopback:\tON\n");
				}
				if (lb->type_map & (1<<WAN_TE1_LIU_DLB_MODE)){
					printf("\tLIU Dual Loopback:\tON\n");
				}

				if (lb->type_map & (1<<WAN_TE1_LINELB_MODE)){
					printf("\tLine/Remote Loopback:\t\tON\n");
				}
				if (lb->type_map & (1<<WAN_TE1_PAYLB_MODE)){
					printf("\tPayload Loopback:\tON\n");
				}
				if (lb->type_map & (1<<WAN_TE1_DDLB_MODE)){
					printf("\tDiagnostic Digital Loopback:\tON\n");
				}
			}else{

				if (lb->type_map & (1<<WAN_TE3_LIU_LB_DIGITAL)){
					printf("\tDigital Loopback:\tON\n");
				} 
				if (lb->type_map & (1<<WAN_TE3_LIU_LB_REMOTE)){
					printf("\tRemote Loopback:\tON\n");
				}
				if (lb->type_map & (1<<WAN_TE3_LIU_LB_ANALOG)){
					printf("\tAnalog Loopback:\tON\n");
				}
			}
		}	
	}
	cleanup_hardware_level_connection();

	return 0;
}

void read_te1_56k_stat(unsigned char force)
{
	sdla_fe_stats_t	*fe_stats;
	//unsigned char* data = NULL;
	
 	if(make_hardware_level_connection()){
    		return;
  	}

	/* Read T1/E1/56K alarms and T1/E1 performance monitoring counters */
	wan_udp.wan_udphdr_command = WAN_FE_GET_STAT;
	wan_udp.wan_udphdr_data_len = 0;
    	wan_udp.wan_udphdr_return_code = 0xaa;
    	wan_udp.wan_udphdr_fe_force = force;
	DO_COMMAND(wan_udp);
	if (wan_udp.wan_udphdr_return_code != 0){
		printf("Failed to read T1/E1/56K statistics.\n");
  		cleanup_hardware_level_connection();
		return;
	}

#if 0
	if (wan_protocol == WANCONFIG_CHDLC){
		data = &wan_udp.wan_udphdr_chdlc_data[0];
	}else{
		data = &wan_udp.wan_udphdr_data[0];
	}
#endif
	fe_stats = (sdla_fe_stats_t*)get_wan_udphdr_data_ptr(0);
	if (femedia.media == WAN_MEDIA_T1 || femedia.media == WAN_MEDIA_E1){
		printf("***** %s: %s Rx Alarms (Framer) *****\n\n",
			if_name, (femedia.media == WAN_MEDIA_T1) ? "T1" : "E1");
		printf("ALOS:\t%s\t| LOS:\t%s\n", 
				WAN_TE_PRN_ALARM_ALOS(fe_stats->alarms), 
				WAN_TE_PRN_ALARM_LOS(fe_stats->alarms));
		printf("RED:\t%s\t| AIS:\t%s\n", 
				WAN_TE_PRN_ALARM_RED(fe_stats->alarms), 
				WAN_TE_PRN_ALARM_AIS(fe_stats->alarms));
		printf("LOF:\t%s\t| RAI:\t%s\n", 
				WAN_TE_PRN_ALARM_LOF(fe_stats->alarms),
				WAN_TE_PRN_ALARM_RAI(fe_stats->alarms));

		if (fe_stats->alarms & WAN_TE_ALARM_LIU){
			printf("\n***** %s: %s Rx Alarms (LIU) *****\n\n",
				if_name, (femedia.media == WAN_MEDIA_T1) ? "T1" : "E1");
			printf("Short Circuit:\t%s\n", 
					WAN_TE_PRN_ALARM_LIU_SC(fe_stats->alarms));
			printf("Open Circuit:\t%s\n", 
					WAN_TE_PRN_ALARM_LIU_OC(fe_stats->alarms));
			printf("Loss of Signal:\t%s\n", 
					WAN_TE_PRN_ALARM_LIU_LOS(fe_stats->alarms));
		}

		printf("\n***** %s: %s Tx Alarms *****\n\n",
			if_name, (femedia.media == WAN_MEDIA_T1) ? "T1" : "E1");
		printf("AIS:\t%s\t| YEL:\t%s\n", 
				(fe_stats->tx_maint_alarms & WAN_TE_BIT_ALARM_AIS) ? "MAINT" :
				(fe_stats->tx_alarms & WAN_TE_BIT_ALARM_AIS) ? "ON":"OFF",
				WAN_TE_PRN_ALARM_YEL(fe_stats->tx_alarms));

	}else if  (femedia.media == WAN_MEDIA_DS3 || femedia.media == WAN_MEDIA_E3){
		printf("***** %s: %s Alarms *****\n\n",
			if_name, (femedia.media == WAN_MEDIA_DS3) ? "DS3" : "E3");

		if (femedia.media == WAN_MEDIA_DS3){
			printf("AIS:\t%s\t| LOS:\t%s\n",
					WAN_TE3_AIS_ALARM(fe_stats->alarms),
					WAN_TE3_LOS_ALARM(fe_stats->alarms));

			printf("OOF:\t%s\t| YEL:\t%s\n",
					WAN_TE3_OOF_ALARM(fe_stats->alarms),
					WAN_TE3_YEL_ALARM(fe_stats->alarms));
		}else{
			printf("AIS:\t%s\t| LOS:\t%s\n",
					WAN_TE3_AIS_ALARM(fe_stats->alarms),
					WAN_TE3_LOS_ALARM(fe_stats->alarms));

			printf("OOF:\t%s\t| YEL:\t%s\n",
					WAN_TE3_OOF_ALARM(fe_stats->alarms),
					WAN_TE3_YEL_ALARM(fe_stats->alarms));
			
			printf("LOF:\t%s\t\n",
					WAN_TE3_LOF_ALARM(fe_stats->alarms));
		}
		
	}else if (femedia.media == WAN_MEDIA_56K){
		printf("***** %s: 56K CSU/DSU Alarms *****\n\n\n", if_name);
	 	printf("In Service:\t\t%s\tData mode idle:\t\t%s\n",
			 	INS_ALARM_56K(fe_stats->alarms), 
			 	DMI_ALARM_56K(fe_stats->alarms));
	
	 	printf("Zero supp. code:\t%s\tCtrl mode idle:\t\t%s\n",
			 	ZCS_ALARM_56K(fe_stats->alarms), 
			 	CMI_ALARM_56K(fe_stats->alarms));

	 	printf("Out of service code:\t%s\tOut of frame code:\t%s\n",
			 	OOS_ALARM_56K(fe_stats->alarms), 
			 	OOF_ALARM_56K(fe_stats->alarms));
		
	 	printf("Valid DSU NL loopback:\t%s\tUnsigned mux code:\t%s\n",
			 	DLP_ALARM_56K(fe_stats->alarms), 
			 	UMC_ALARM_56K(fe_stats->alarms));

	 	printf("Rx loss of signal:\t%s\t\n",
			 	RLOS_ALARM_56K(fe_stats->alarms)); 
		
	}else{
		printf("***** %s: Unknown Front End 0x%X *****\n\n",
			if_name, femedia.media);
	}

	if (femedia.media == WAN_MEDIA_T1 || femedia.media == WAN_MEDIA_E1){
		sdla_te_pmon_t*	pmon = &fe_stats->te_pmon;

		printf("\n\n***** %s: %s Performance Monitoring Counters *****\n\n",
				if_name, (femedia.media == WAN_MEDIA_T1) ? "T1" : "E1");
		if (pmon->mask & WAN_TE_BIT_PMON_LCV){
			printf("Line Code Violation\t: %d\n",
						pmon->lcv_errors);
		}
		if (pmon->mask & WAN_TE_BIT_PMON_BEE){
			printf("Bit Errors (CRC6/Ft/Fs)\t: %d\n",
						pmon->bee_errors);
		}
		if (pmon->mask & WAN_TE_BIT_PMON_OOF){
			printf("Out of Frame Errors\t: %d\n",
						pmon->oof_errors);
		}
		if (pmon->mask & WAN_TE_BIT_PMON_FEB){
			printf("Far End Block Errors\t: %d\n",
						pmon->feb_errors);
		}
		if (pmon->mask & WAN_TE_BIT_PMON_CRC4){
			printf("CRC4 Errors\t\t: %d\n",
						pmon->crc4_errors);
		}
		if (pmon->mask & WAN_TE_BIT_PMON_FER){
			printf("Framing Bit Errors\t: %d\n",
						pmon->fer_errors);
		}
		if (pmon->mask & WAN_TE_BIT_PMON_FAS){
			printf("FAS Errors\t\t: %d\n",
						pmon->fas_errors);
		}
		printf("Sync Errors\t\t: %d\n",
						pmon->sync_errors);
	}

	if (femedia.media == WAN_MEDIA_DS3 || femedia.media == WAN_MEDIA_E3){
		sdla_te3_pmon_t*	pmon = &fe_stats->u.te3_pmon;

		printf("\n\n***** %s: %s Performance Monitoring Counters *****\n\n",
				if_name, (femedia.media == WAN_MEDIA_DS3) ? "DS3" : "E3");

		printf("Framing Bit Error:\t%d\tLine Code Violation:\t%d\n", 
				pmon->pmon_framing,
				pmon->pmon_lcv);

		if (femedia.media == WAN_MEDIA_DS3){
			printf("Parity Error:\t\t%d\n",
					pmon->pmon_parity);
			printf("CP-Bit Error Event:\t%d\tFEBE Event:\t\t%d\n", 
					pmon->pmon_cpbit,
					pmon->pmon_febe);
		}else{
			printf("Parity Error:\t%d\tFEBE Event:\t\t%d\n",
					pmon->pmon_parity,
					pmon->pmon_febe);
		}
	}
	
	if (femedia.media == WAN_MEDIA_T1 || femedia.media == WAN_MEDIA_E1){
		if (strlen(fe_stats->u.te1_stats.rxlevel)){
			printf("\n\nRx Level\t: %s\n",
					fe_stats->u.te1_stats.rxlevel);		
		}
	}
	
	cleanup_hardware_level_connection();
	return;
}

void flush_te1_pmon(void)
{

	if(make_hardware_level_connection()){
    		return;
  	}

	switch(femedia.media){
	case WAN_MEDIA_T1:
	case WAN_MEDIA_E1:
		/* Flush perfomance mononitoring counters */
		wan_udp.wan_udphdr_command = WAN_FE_FLUSH_PMON;
		wan_udp.wan_udphdr_data_len = 0;
	    	wan_udp.wan_udphdr_return_code = 0xaa;
		DO_COMMAND(wan_udp);
		if (wan_udp.wan_udphdr_return_code != 0){
			printf("Failed to flush Perfomance Monitoring counters.\n");
  			cleanup_hardware_level_connection();
			return;
		}
		break;
	}

	printf("\nDSU/CSU Perfomance Monitoring counters were flushed.\n");
    	cleanup_hardware_level_connection();
	return;
}


void read_te1_56k_config (void)
{

	if (femedia.media == WAN_MEDIA_T1 || femedia.media == WAN_MEDIA_E1){
		int num_of_chan = (femedia.media == WAN_MEDIA_T1) ? 
						NUM_OF_T1_CHANNELS : 
						NUM_OF_E1_TIMESLOTS;
		int i = 0, start_chan = 0;

		if(make_hardware_level_connection()){
    			return;
  		}

		/* T1/E1 card */
		wan_udp.wan_udphdr_command = WAN_FE_GET_CFG;
		wan_udp.wan_udphdr_data_len = 0;
		wan_udp.wan_udphdr_return_code = 0xaa;

		DO_COMMAND(wan_udp);
		if (wan_udp.wan_udphdr_return_code != 0){
			printf("CSU/DSU Read Configuration Failed");
		}else{
			sdla_fe_cfg_t *fe_cfg = NULL;

			fe_cfg = (sdla_fe_cfg_t *)get_wan_udphdr_data_ptr(0);
#if 0
			if (wan_protocol == WANCONFIG_CHDLC){
				te_cfg = (sdla_te_cfg_t *)&wan_udp.wan_udphdr_chdlc_data[0];
			}else{
				te_cfg = (sdla_te_cfg_t *)&wan_udp.wan_udphdr_data[0];
			}
#endif
			printf("CSU/DSU %s Configuration:\n", if_name);
			printf("\tMedia type\t%s\n",
				MEDIA_DECODE(fe_cfg));
			printf("\tFraming\t\t%s\n",
				FRAME_DECODE(fe_cfg));
			printf("\tEncoding\t%s\n",
				LCODE_DECODE(fe_cfg));
			if (femedia.media == WAN_MEDIA_T1){
				printf("\tLine Build\t%s\n",
						LBO_DECODE(fe_cfg));
			}
			printf("\tChannel Base\t");
			for (i = 0, start_chan = 0; i < num_of_chan; i++){
				if (fe_cfg->cfg.te_cfg.active_ch & (1 << i)){
					if (!start_chan){
						start_chan = i+1;
					}
					//printf("%d ", i+1);
				}else{
					if (start_chan){
						if (start_chan != i + 1){
							printf("%d-%d ", start_chan, i);
						}else{
							printf("%d ", start_chan);
						}
						start_chan = 0;
					}
				}
			} 
			if (start_chan){
				if (start_chan != num_of_chan){
					printf("%d-%d ", start_chan, num_of_chan);
				}else{
					printf("%d ", start_chan);
				}
			}
			printf("\n");
			printf("\tClock Mode\t%s\n",
				TECLK_DECODE(fe_cfg));
		}
	}
	else if (femedia.media == WAN_MEDIA_DS3){

		if(make_hardware_level_connection()){
    			return;
  		}

		/* DS3 card */
		wan_udp.wan_udphdr_command = WAN_FE_GET_CFG;
		wan_udp.wan_udphdr_data_len = 0;
		wan_udp.wan_udphdr_return_code = 0xaa;

		DO_COMMAND(wan_udp);
		if (wan_udp.wan_udphdr_return_code != 0){
			printf("CSU/DSU Read Configuration Failed");
		}else{
			sdla_fe_cfg_t *fe_cfg = NULL;

			fe_cfg = (sdla_fe_cfg_t *) get_wan_udphdr_data_ptr(0);
			
			printf("CSU/DSU %s Configuration:\n", if_name);
			printf("\tMedia type\t%s\n",
				MEDIA_DECODE(fe_cfg));
			printf("\tFraming\t\t%s\n",
				FRAME_DECODE(fe_cfg));
			printf("\tEncoding\t%s\n",
				LCODE_DECODE(fe_cfg));
			printf("\tClock Mode\t%s\n", (fe_cfg->cfg.te3_cfg.clock == WAN_NORMAL_CLK) ? "Normal": (fe_cfg->cfg.te3_cfg.clock == WAN_MASTER_CLK) ? "Master" : "Unknown");
		}		
	}
	cleanup_hardware_level_connection();
	return;
}

void set_debug_mode(unsigned char type, unsigned char mode)
{
	sdla_fe_debug_t	fe_debug;

	fe_debug.type	= type;
	fe_debug.mode	= mode;
	set_fe_debug_mode(&fe_debug);
}

void set_fe_debug_mode(sdla_fe_debug_t *fe_debug)
{
	int	err = 0;
	unsigned char	*data = NULL;

	if(make_hardware_level_connection()){
		return;
	}
	wan_udp.wan_udphdr_command	= WAN_FE_SET_DEBUG_MODE;
	wan_udp.wan_udphdr_data_len	= sizeof(sdla_fe_debug_t);
	wan_udp.wan_udphdr_return_code	= 0xaa;

	data = get_wan_udphdr_data_ptr(0);
	memcpy(data, (unsigned char*)fe_debug, sizeof(sdla_fe_debug_t));

#if 0
	if (wan_protocol == WANCONFIG_CHDLC){
		wan_udp.wan_udphdr_chdlc_data[0] = type;
		wan_udp.wan_udphdr_chdlc_data[1] = mode;
	}else{
		wan_udp.wan_udphdr_data[0] = type;
		wan_udp.wan_udphdr_data[1] = mode;
	}
#endif
	err = DO_COMMAND(wan_udp);
	if (fe_debug->type == WAN_FE_DEBUG_REG && fe_debug->fe_debug_reg.read == 1){
		if (err == 0 && wan_udp.wan_udphdr_return_code == 0){
			int	cnt = 0;
			printf("Please wait.");fflush(stdout);
repeat_read_reg:
			wan_udp.wan_udphdr_return_code	= 0xaa;
			fe_debug->fe_debug_reg.read = 2;
			memcpy(data, (unsigned char*)fe_debug, sizeof(sdla_fe_debug_t));
			wp_usleep(100000);
			err = DO_COMMAND(wan_udp);
			if (err || wan_udp.wan_udphdr_return_code != 0){
				if (cnt < 5){
					printf(".");fflush(stdout);
					goto repeat_read_reg;
				}
			}
			printf("\n\n");
		}
	}

	if (err || wan_udp.wan_udphdr_return_code != 0){
		if (fe_debug->type == WAN_FE_DEBUG_RBS){
			printf("Failed to %s mode.\n",
				WAN_FE_DEBUG_RBS_DECODE(fe_debug->mode));
		}else{
			printf("Failed to execute debug mode (%02X).\n",
						fe_debug->type);		
		}
	}else{
		fe_debug = (sdla_fe_debug_t*)get_wan_udphdr_data_ptr(0);
		switch(fe_debug->type){
		case WAN_FE_DEBUG_RBS:
			if (fe_debug->mode == WAN_FE_DEBUG_RBS_READ){
				printf("Read RBS status is suceeded!\n");
			}else if (fe_debug->mode == WAN_FE_DEBUG_RBS_SET){
				printf("Setting ABCD bits (%X) for channel %d is suceeded!\n",
						fe_debug->fe_debug_rbs.abcd,
						fe_debug->fe_debug_rbs.channel);
			}else{
				printf("%s debug mode!\n",
					WAN_FE_DEBUG_RBS_DECODE(fe_debug->mode));
			}
			break;
		case WAN_FE_DEBUG_ALARM:
			printf("%s AIS alarm!\n",
					WAN_FE_DEBUG_ALARM_DECODE(fe_debug->mode));
			break;
		case WAN_FE_DEBUG_REG:
			if (fe_debug->fe_debug_reg.read == 2){
				printf("Read Front-End Reg:%04X=%02X\n",
						fe_debug->fe_debug_reg.reg,
						fe_debug->fe_debug_reg.value);
			}
			break;
		}
	}
	cleanup_hardware_level_connection();
	return;
}

void set_fe_tx_alarm(int mode)
{
	sdla_fe_debug_t	fe_debug;
	unsigned char	*data = NULL;

	if(make_hardware_level_connection()){
		return;
	}

	wan_udp.wan_udphdr_command	= WAN_FE_SET_DEBUG_MODE;
	wan_udp.wan_udphdr_data_len	= sizeof(sdla_fe_debug_t);
	wan_udp.wan_udphdr_return_code	= 0xaa;
	
	fe_debug.type = WAN_FE_DEBUG_ALARM;
	fe_debug.mode = mode;
	data = get_wan_udphdr_data_ptr(0);
	memcpy(data, (unsigned char*)&fe_debug, sizeof(sdla_fe_debug_t));

	DO_COMMAND(wan_udp);
	if (wan_udp.wan_udphdr_return_code != 0){
		printf("Failed to tx alarm\n");
	}else{
		printf("Alarm transmited ok!\n");
	}
  	
	cleanup_hardware_level_connection();
	return;         
}

void set_fe_tx_mode(unsigned char mode)
{
	sdla_fe_debug_t	fe_debug;
	unsigned char	*data = NULL;

	if(make_hardware_level_connection()){
		return;
	}

	wan_udp.wan_udphdr_command	= WAN_FE_TX_MODE;
	wan_udp.wan_udphdr_data_len	= sizeof(sdla_fe_debug_t);
	wan_udp.wan_udphdr_return_code	= 0xaa;
	
	fe_debug.type = WAN_FE_TX_MODE;
	fe_debug.mode = mode;
	data = get_wan_udphdr_data_ptr(0);
	memcpy(data, (unsigned char*)&fe_debug, sizeof(sdla_fe_debug_t));

	DO_COMMAND(wan_udp);
	if (wan_udp.wan_udphdr_return_code != 0){
		printf("Failed to %s transmitter.\n",
			(mode == WAN_FE_TXMODE_ENABLE) ? 
				"enable" : "disable");
	}else{
		printf("%s transmitter!\n",
			(mode == WAN_FE_TXMODE_ENABLE) ? 
					"Enable" : "Disable");
	}
  	
	cleanup_hardware_level_connection();
	return;
}

void aft_remora_debug_mode(sdla_fe_debug_t *fe_debug)
{
	int	err = 0;
	unsigned char	*data = NULL;

	if(make_hardware_level_connection()){
		return;
	}
	wan_udp.wan_udphdr_command	= WAN_FE_SET_DEBUG_MODE;
	wan_udp.wan_udphdr_data_len	= sizeof(sdla_fe_debug_t);
	wan_udp.wan_udphdr_return_code	= 0xaa;

	data = get_wan_udphdr_data_ptr(0);
	memcpy(data, (unsigned char*)fe_debug, sizeof(sdla_fe_debug_t));
	err = DO_COMMAND(wan_udp);
	if (fe_debug->type == WAN_FE_DEBUG_REG && fe_debug->fe_debug_reg.read == 1){
		if (err == 0 && wan_udp.wan_udphdr_return_code == 0){
			int	cnt = 0;
			printf("Please wait.");fflush(stdout);
repeat_read_reg:
			wan_udp.wan_udphdr_return_code	= 0xaa;
			fe_debug->fe_debug_reg.read = 2;
			memcpy(data, (unsigned char*)fe_debug, sizeof(sdla_fe_debug_t));
			wp_usleep(100000);
			err = DO_COMMAND(wan_udp);
			if (err || wan_udp.wan_udphdr_return_code != 0){
				if (cnt < 5){
					printf(".");fflush(stdout);
					goto repeat_read_reg;
				}
			}
			printf("\n\n");
		}
	}

	if (err || wan_udp.wan_udphdr_return_code != 0){
		printf("Failed to execute RM debug mode (%02X).\n",
						fe_debug->type);		
	}else{
		fe_debug = (sdla_fe_debug_t*)get_wan_udphdr_data_ptr(0);
		switch(fe_debug->type){
		case WAN_FE_DEBUG_REG:
			if (fe_debug->fe_debug_reg.read == 2){
				printf("Read Front-End Reg:%04X=%02X\n",
						fe_debug->fe_debug_reg.reg,
						fe_debug->fe_debug_reg.value);
			}
			break;
		}
	}
	cleanup_hardware_level_connection();
	return;
}

/******************************************************************************
*
* 
* pseudorandom:    wanpipemon -i <ifname> -c Tbert <pattern_type> <eib> <chan_map> 
* repetitive:      wanpipemon -i <ifname> -c Tbert <pattern_type> <pattern> <pattern_len> <eib> <chan_map>
* repetitive word: wanpipemon -i <ifname> -c Tbert <pattern_type> <pattern> <count> <eib> <chan_map>

******************************************************************************/
static int set_fe_bert_help(void)
{
	printf("\n");
	printf("\tSangoma T1 Bit-Error-Test\n\n");
	printf("Usage:\n");
	printf("\n");
	printf(" wanpipemon -i <ifname> -c Tbert <command> <pattern type> [pattern] [pattern len] [wcount] [eib type] [loopback mode] [channel list]\n"); 
	printf(" wanpipemon -i <ifname> -c Tbert help                  : print help message\n"); 
	printf("\n");
	printf("* command (--cmd <command>)\t: BERT command\n");
	printf("    start:\tStart BERT\n");
	printf("    stop:\tStop BERT\n");
	printf("    status:\tPrint BERT statistics\n");
	printf("    eib:\tInsert Error bit\n");
	printf("    reset:\tReset BERT counters\n");
	printf("    running:\tVerify if BERT is running\n");
	printf("    help:\tPrint help message\n");
	printf("* pattern type (--ptype <type>)\t: BERT pattern type\n");
	printf("    pseudor1:\tPseudorandom 2E7-1 (default)\n");
	printf("    pseudor2:\tPseudorandom 2E11-1\n");
	printf("    pseudor3:\tPseudorandom 2E15-1\n");
	printf("    pseudor4:\tPseudorandom Pattern QRSS\n");
	printf("    pseudor5:\tPseudorandom 2E9-1\n");
	printf("    repet:\tRepetitive Pattern (pattern 32 bits)\n");
	printf("    alterw:\tAlternating Word Pattern (pattern 16/32 bits)\n");
	printf("    daly:\tModified 55 Octet (Daly) Pattern\n");
	printf("\n");
	printf("* pattern (--pattern <pattern>)\t: BERT Repetitive Pattern (32 bits)\n");
	printf("* pattern len (--plen <number)\t: BERT Repetitive Pattern Length\n");
	printf("* wcount (--wcount <number)\t: BERT Alternating Word Count (1-256)\n");
	printf("* eib type (--eib <type>)\t: Error Insert Bit type:\n");
	printf("    none:\tNo errors automatically inserted (default)\n");
	printf("    single:\tInsert Single Bit Error\n");
	printf("    eib1:\t10E-1 Bit Error Insert\n");
	printf("    eib2:\t10E-2 Bit Error Insert\n");
	printf("    eib3:\t10E-3 Bit Error Insert\n");
	printf("    eib4:\t10E-4 Bit Error Insert\n");
	printf("    eib5:\t10E-5 Bit Error Insert\n");
	printf("    eib6:\t10E-6 Bit Error Insert\n");
	printf("    eib7:\t10E-7 Bit Error Insert\n");
	printf("\n");
	printf("* loopback mode (--loop <mode>)\t: BERT Loopback mode (T1 only)\n");
	printf("    none:\tNo required to send loopback mode code (default)\n");
	printf("    payload:\tSend payload loopback code to far end before and after BERT\n");
	printf("    line:\tSend line loopback code to far end before and after BERT\n");
	printf("\n");
	printf("* channle list (--chan <list>)\t: Channel list for BERT\n");
	printf("    all:\tUse all active channels (default)\n");
	printf("    A:\tSingle channel\n");
	printf("    A-B:\tChannel map defined as a range (A..B)\n");
	printf("\n");
	printf("Examples:\n");
	printf("  wanpipemon -i w1g1 -c Tbert --cmd start --ptype pseudor1 --eib none --loop none --chan all\n"); 
	printf("  wanpipemon -i w1g1 -c Tbert --cmd start --ptype repetp --pattern 12345678 --plen 32 --eib none --loop none --chan all\n"); 
	printf("  wanpipemon -i w1g1 -c Tbert --cmd start --ptype daly --eib none --loop none --chan all\n"); 
	printf("  wanpipemon -i w1g1 -c Tbert --cmd start --ptype alterw --pattern 1234abcd --wcount 101 --eib none --loop none --chan all\n"); 
	printf("  wanpipemon -i w1g1 -c Tbert --cmd status\n"); 
	printf("\n");
	return 0;
}


int parse_bert_args(int argc, char *argv[], sdla_te_bert_t *bert, int *silent)
{
	int	argi = 0;

	*silent = 0;
	for(argi = 1; argi < argc; argi++){

		char *parg = argv[argi], *param;

		if (strcmp(parg, "--cmd") == 0){

			if (argi + 1 >= argc ){
				printf("ERROR: BERT command is missing!\n");
				return -EINVAL;
			}
			param = argv[argi+1]; 
			if (strcmp(param,"start") == 0){
				bert->cmd = WAN_TE_BERT_CMD_START;
			}else if (strcmp(param,"stop") == 0){
				bert->cmd = WAN_TE_BERT_CMD_STOP;
			}else if (strcmp(param,"status") == 0){
				bert->cmd = WAN_TE_BERT_CMD_STATUS;
			}else if (strcmp(param,"running") == 0){
				bert->cmd = WAN_TE_BERT_CMD_RUNNING;
			}else if (strcmp(param,"reset") == 0){
				bert->cmd = WAN_TE_BERT_CMD_RESET;
			}else if (strcmp(param,"eib") == 0){
				bert->cmd = WAN_TE_BERT_CMD_EIB;
			}else if (strcmp(param,"help") == 0){
				bert->cmd = 0x00;
				return 0;
			}else{
				printf("ERROR: Invalid BERT command (%s)!\n", param);
				return -EINVAL;
			}
			
		} else if (strcmp(parg, "--ptype") == 0){
	
			if (argi + 1 >= argc ){
				printf("ERROR: BERT pattern type is missing!\n");
				return -EINVAL;
			}
			param = argv[argi+1]; 
			if (strncmp(param,"pseudor1",8) == 0){
				bert->un.cfg.pattern_type = WAN_TE_BERT_PATTERN_PSEUDORANDOM_2E7;
			}else if (strncmp(param,"pseudor2",8) == 0){
				bert->un.cfg.pattern_type = WAN_TE_BERT_PATTERN_PSEUDORANDOM_2E11;
			}else if (strncmp(param,"pseudor3",8) == 0){
				bert->un.cfg.pattern_type = WAN_TE_BERT_PATTERN_PSEUDORANDOM_2E15;
			}else if (strncmp(param,"pseudor4",8) == 0){
				bert->un.cfg.pattern_type = WAN_TE_BERT_PATTERN_PSEUDORANDOM_QRSS;
			}else if (strncmp(param,"pseudor5",8) == 0){
				bert->un.cfg.pattern_type = WAN_TE_BERT_PATTERN_PSEUDORANDOM_2E9;
			}else if (strncmp(param,"repet",5) == 0){
				bert->un.cfg.pattern_type = WAN_TE_BERT_PATTERN_REPETITIVE;
			}else if (strncmp(param,"alterw",6) == 0){
				bert->un.cfg.pattern_type = WAN_TE_BERT_PATTERN_WORD;
			}else if (strncmp(param,"daly",5) == 0){
				bert->un.cfg.pattern_type = WAN_TE_BERT_PATTERN_DALY;
			}else{
				printf("ERROR: Invalid BERT pattern type (%s)!\n", param);
				return -EINVAL;
			}

		}else if (strcmp(parg, "--pattern") == 0){

			int 	num = 0, i = 0, len;
			char	ch;
	
			if (argi + 1 >= argc ){
				printf("ERROR: BERT pattern is missing!\n");
				return -EINVAL;
			}
			len = strlen(argv[argi+1]);
			bert->un.cfg.pattern = 0x00;
			for(i=0; i < len; i++){
				bert->un.cfg.pattern = bert->un.cfg.pattern << 4;
				ch = argv[argi+1][i];
				if (isdigit(ch)){
					num = ch - '0'; 
				}else if (ch >= 'A' && ch <= 'F'){
					num = 10 + (ch-'A'); 
				}else if (ch >= 'a' && ch <= 'f'){
					num = 10 + (ch-'a'); 
				}else{
					printf("ERROR: Invalid BERT pattern (%s)!\n", argv[argi+1]);
					return -EINVAL;
				}
				bert->un.cfg.pattern |= num;
			}

		}else if (strcmp(parg, "--plen") == 0){

			if (argi + 1 >= argc ){
				printf("ERROR: BERT pattern len is missing!\n");
				return -EINVAL;
			}
			bert->un.cfg.pattern_len = (unsigned short)atoi(argv[argi+1]);
	
		}else if (strcmp(parg, "--wcount") == 0){

			if (argi + 1 >= argc ){
				printf("ERROR: BERT Alternating Word Count is missing!\n");
				return -EINVAL;
			}
			bert->un.cfg.count = atoi(argv[argi+1]);

		}else if (strcmp(parg, "--eib") == 0){

			if (argi + 1 >= argc ){
				printf("ERROR: BERT EIB parameter is missing!\n");
				return -EINVAL;
			}
			param = argv[argi+1]; 
			if (strncmp(param, "none", 4) == 0){
				bert->un.cfg.eib = WAN_TE_BERT_EIB_NONE;
			} else if (strncmp(param, "single", 6) == 0){
				bert->un.cfg.eib = WAN_TE_BERT_EIB_SINGLE;
			} else if (strncmp(param, "eib1", 4) == 0){
				bert->un.cfg.eib = WAN_TE_BERT_EIB1;
			} else if (strncmp(param, "eib2", 4) == 0){
				bert->un.cfg.eib = WAN_TE_BERT_EIB2;
			} else if (strncmp(param, "eib3", 4) == 0){
				bert->un.cfg.eib = WAN_TE_BERT_EIB3;
			} else if (strncmp(param, "eib4", 4) == 0){
				bert->un.cfg.eib = WAN_TE_BERT_EIB4;
			} else if (strncmp(param, "eib5", 4) == 0){
				bert->un.cfg.eib = WAN_TE_BERT_EIB5;
			} else if (strncmp(param, "eib6", 4) == 0){
				bert->un.cfg.eib = WAN_TE_BERT_EIB6;
			} else if (strncmp(param, "eib7", 4) == 0){
				bert->un.cfg.eib = WAN_TE_BERT_EIB7;
			} else {
				printf("ERROR: Invalid BERT EIB type (%s)!\n", param);
				return -EINVAL;
			}

		}else if (strcmp(parg, "--loop") == 0){

			if (argi + 1 >= argc ){
				printf("ERROR: BERT loop mode is missing!\n");
				return -EINVAL;
			}

			if (femedia.media == WAN_MEDIA_E1){
				
				bert->un.cfg.lb_type = WAN_TE_BERT_LOOPBACK_NONE;
			} else {

				if (strncmp(argv[argi+1],"none",4) == 0){
					bert->un.cfg.lb_type = WAN_TE_BERT_LOOPBACK_NONE;
				}else if (strncmp(argv[argi+1],"payload",7) == 0){
					bert->un.cfg.lb_type = WAN_TE_BERT_LOOPBACK_PAYLOAD;
				}else if (strncmp(argv[argi+1],"line",4) == 0){
					bert->un.cfg.lb_type = WAN_TE_BERT_LOOPBACK_LINE;
				} else {
					printf("ERROR: Invalid BERT Loopback mode (%s)!\n", argv[argi+1]);
					return -EINVAL;
				}
			}

		}else if (strcmp(parg, "--chan") == 0){

			if (argi + 1 >= argc ){
				printf("ERROR: BERT channel list is missing!\n");
				return -EINVAL;
			}
			param = argv[argi+1]; 
			if (wp_strcasecmp(param,"all") == 0){
				bert->un.cfg.chan_map = ENABLE_ALL_CHANNELS;
			}else{
				bert->un.cfg.chan_map = felib_parse_active_channel_str(param);
				/* In the kernel code the bits must start from bit 1,
				 * not zero, shift left by one. */
				bert->un.cfg.chan_map = bert->un.cfg.chan_map << 1;
				printf("Channels str: %s, Channels bitmap: 0x%08X\n", param, bert->un.cfg.chan_map);
			}
		}else if (strcmp(parg, "--verbose") == 0){

			bert->verbose = 1;

		}else if (strcmp(parg, "--silent") == 0){
			*silent = 1;
		}
	}
	return 0;
}

int set_fe_bert(int argc, char *argv[])
{
	sdla_te_bert_t	bert;
	int		silent = 0;
	u_int8_t	lb_type = 0; 
	char		*data = NULL;

	if (femedia.media != WAN_MEDIA_T1 && femedia.media != WAN_MEDIA_E1){
		printf("ERROR: Sangoma Bit-Error-Test supports only for T1/E1 (DM) cards!\n");
		return -EINVAL;
	}

	memset(&bert, 0, sizeof(sdla_te_bert_t));

	/* default value */
	bert.cmd = WAN_TE_BERT_CMD_NONE;
	if (bert.cmd == WAN_TE_BERT_CMD_START){
		bert.un.cfg.pattern_type	= WAN_TE_BERT_PATTERN_PSEUDORANDOM_2E7;
		bert.un.cfg.eib				= WAN_TE_BERT_EIB_NONE;
		bert.un.cfg.chan_map		= ENABLE_ALL_CHANNELS;
	}

	if (parse_bert_args(argc, argv, &bert, &silent)){
		return -EINVAL;
	}

	if (!bert.cmd){
		set_fe_bert_help();
		return 0;
	}

#if 0
	if (bert->verbose){
		printf("BERT command        : %s\n", WAN_TE_BERT_CMD_DECODE(bert.cmd));
		printf("BERT channel list   : %08X\n", bert.chan_map); 
		printf("BERT pattern type   : %s\n", WAN_TE_BERT_PATTERN_DECODE(bert.pattern_type)); 
		printf("BERT pattern length : %d\n", bert.pattern_len); 
		printf("BERT pattern        : %08X\n",bert.pattern);
		printf("BERT word count     : %d\n", bert.count); 
		printf("BERT loopback mode  : %s\n", WAN_TE_BERT_LOOPBACK_DECODE(bert.lb_mode)); 
		printf("BERT EIB            : %s\n", WAN_TE_BERT_EIB_DECODE(bert.eib)); 
	}
#endif
	
	if (bert.cmd == WAN_TE_BERT_CMD_START && bert.un.cfg.lb_type != WAN_TE_BERT_LOOPBACK_NONE){

		if (bert.un.cfg.lb_type == WAN_TE_BERT_LOOPBACK_LINE){
			lb_type = WAN_TE1_TX_LINELB_MODE;
		}else if (bert.un.cfg.lb_type == WAN_TE_BERT_LOOPBACK_PAYLOAD){
			lb_type = WAN_TE1_TX_PAYLB_MODE;
		}
		if (set_lb_modes(lb_type, WAN_TE1_LB_ENABLE)){
			printf("ERROR: Failed to execute BERT %s command (%s)!\n",
					WAN_TE_BERT_CMD_DECODE(bert.cmd),
					WAN_TE_BERT_LOOPBACK_DECODE(bert.un.cfg.lb_type));
			return -EINVAL;
		}
	}

	if(make_hardware_level_connection()){
		if (lb_type) set_lb_modes(lb_type, WAN_TE1_LB_DISABLE);
		return -EINVAL;
	}
	data = (char*)get_wan_udphdr_data_ptr(0);
	memcpy(data, &bert, sizeof(sdla_te_bert_t));
	wan_udp.wan_udphdr_command	= WAN_FE_BERT_MODE;
	wan_udp.wan_udphdr_data_len	= sizeof(sdla_te_bert_t);
	wan_udp.wan_udphdr_return_code	= 0xaa;

	DO_COMMAND(wan_udp);
	if (wan_udp.wan_udphdr_return_code != 0){
		
		printf("Failed to execute BERT %s command.\n",
				WAN_TE_BERT_CMD_DECODE(bert.cmd));
		printf("BERT CMD Return Code: %s (%d)\n", 
				SDLA_DECODE_SANG_STATUS(wan_udp.wan_udphdr_return_code), 
				wan_udp.wan_udphdr_return_code);

		cleanup_hardware_level_connection();
		if (lb_type) set_lb_modes(lb_type, WAN_TE1_LB_DISABLE);
		return -EINVAL;
	}

	memcpy(&bert, data, sizeof(sdla_te_bert_t));
	if (bert.rc != WAN_TE_BERT_RC_SUCCESS){
		printf("ERROR: Failed to execute BERT %s command (%s)!\n",
					WAN_TE_BERT_CMD_DECODE(bert.cmd),
					WAN_TE_BERT_RC_DECODE(bert.rc));
		cleanup_hardware_level_connection();
		if (lb_type) set_lb_modes(lb_type, WAN_TE1_LB_DISABLE);
		return -EINVAL;
	}

	if (bert.cmd == WAN_TE_BERT_CMD_RUNNING){
		if (silent == 0){
			printf("BERT test is %s\n",
				(bert.status == WAN_TE_BERT_STATUS_RUNNING)?
					"running":"not running");
		}
		return (bert.status == WAN_TE_BERT_STATUS_RUNNING) ? 0 : 1;
	}
	if (bert.cmd == WAN_TE_BERT_CMD_STATUS){

		sdla_te_bert_stats_t	*stats = &bert.un.stats;
		double			value;

		printf("****************************************\n");
		printf("***           BERT Results           ***\n");
		printf("****************************************\n");
		printf("--- BERT test is %s ---\n",
				(bert.status == WAN_TE_BERT_STATUS_RUNNING)?
					"running":"not running");
		printf("--- Tx/Rx:\n");
		printf("\tBit Counts\t\t: %ld\n", 
					stats->bit_cnt);
		printf("--- Errors:\n");
		printf("\tBit Errors\t\t: %ld\n", 
					stats->err_cnt);
		printf("\tErrored Seconds\t\t: %d\n", 
					stats->err_sec);
		printf("\tError Free Seconds\t: %d\n", 
					stats->err_free_sec);
		printf("--- Statistics:\n");
		printf("\tLock status\t\t: %s\n",
					(stats->inlock)?
						"** IN-LOCK **":
						"** OUT-OF-LOCK **");
		printf("\tAvailable Seconds\t: %d\n", 
					stats->avail_sec);
		value = 0;
		if (stats->bit_cnt == 0){
			printf("\tBit Error Rate\t\t: %e\n", value); 
		}else if (stats->err_cnt == 0){
			printf("\tBit Error Rate\t\t: %e\n", value); 
		}else{
			value = (double)stats->err_cnt / (double)stats->bit_cnt;
			printf("\tBit Error Rate\t\t: %e\n", value); 
		}
		if (stats->avail_sec == 0){
			printf("\t%% Error Free Seconds\t: 100.00%%\n"); 
		}else{
			printf("\t%% Error Free Seconds\t: %d.%d%%\n", 
				(stats->err_free_sec * 100) / stats->avail_sec,
				(stats->err_free_sec * 100) % stats->avail_sec);
		}
	}else{
		printf("Execute BERT %s command... Done!\n",
				WAN_TE_BERT_CMD_DECODE(bert.cmd));
	}

	cleanup_hardware_level_connection();

	if (bert.cmd == WAN_TE_BERT_CMD_STOP){
		printf("\n");
		printf("Stopping Bit-Error-Test... Done!\n");

#if 0
		/* DavidR:	this code produces an error for MOST bert tests! 
		 *			The errors are meaningless because code disables loopbacks which
		 *			were NOT enabled. */
		if (bert.un.stop.lb_type != WAN_TE_BERT_LOOPBACK_NONE){
			if (bert.un.stop.lb_type == WAN_TE_BERT_LOOPBACK_LINE){
				lb_type = WAN_TE1_TX_LINELB_MODE;
			}else if (bert.un.stop.lb_type == WAN_TE_BERT_LOOPBACK_PAYLOAD){
				lb_type = WAN_TE1_TX_PAYLB_MODE;
			}
			if (set_lb_modes(lb_type, WAN_TE1_LB_DISABLE)){
				printf("WARNING: Failed to deactivate Remote %s mode!\n",
						WAN_TE_BERT_LOOPBACK_DECODE(bert.un.stop.lb_type));
			}
		}
#endif

	}

	return 0;
}

int	sw_bert_start(unsigned char bert_sequence_type)
{
	memset(&wan_udp, 0x00, sizeof(wan_udp));

	if (make_hardware_level_connection()) {
		return 1;
	}

	wan_udp.wan_udphdr_command = WANPIPEMON_ENABLE_BERT;
	wan_udp.wan_udphdr_data_len = sizeof(bert_sequence_type);
	*get_wan_udphdr_data_ptr(0) = bert_sequence_type;
	wan_udp.wan_udphdr_return_code = 0xaa;

	if (DO_COMMAND(wan_udp)) {
		printf("Error: ioctl failed!\n");
		return 1;
	}

	if (wan_udp.wan_udphdr_return_code) {
		printf("Failed to start SW BERT! err: %s (%d)\n", 
			SDLA_DECODE_SANG_STATUS(wan_udp.wan_udphdr_return_code),
			wan_udp.wan_udphdr_return_code);
		cleanup_hardware_level_connection();
		return 1;
	}

	printf("SW BERT started\n");	
	cleanup_hardware_level_connection();
	return 0;
}

int	sw_bert_stop(void)
{
	memset(&wan_udp, 0x00, sizeof(wan_udp));

	if (make_hardware_level_connection()) {
		return 1;
	}

	wan_udp.wan_udphdr_command = WANPIPEMON_DISABLE_BERT;
	wan_udp.wan_udphdr_return_code = 0xaa;

	if (DO_COMMAND(wan_udp)) {
		printf("Error: ioctl failed!\n");
		return 1;
	}

	if (wan_udp.wan_udphdr_return_code) {
		printf("Failed to stop SW BERT! err: %s (%d)\n", 
			SDLA_DECODE_SANG_STATUS(wan_udp.wan_udphdr_return_code),
			wan_udp.wan_udphdr_return_code);
		cleanup_hardware_level_connection();
		return 1;
	}
	
	printf("SW BERT stopped\n");
	cleanup_hardware_level_connection();
	return 0;
}

int	sw_bert_status(void)
{
	wp_bert_status_t *wp_bert_status;

	memset(&wan_udp, 0x00, sizeof(wan_udp));

	if (make_hardware_level_connection()) {
		return 1;
	}

	wan_udp.wan_udphdr_command = WANPIPEMON_GET_BERT_STATUS;
	wan_udp.wan_udphdr_return_code = 0xaa;

	if (DO_COMMAND(wan_udp)) {
		printf("Error: ioctl failed!\n");
		return 1;
	}

	if (wan_udp.wan_udphdr_return_code) {
		printf("Failed to get SW BERT status! err: %s (%d)\n", 
			SDLA_DECODE_SANG_STATUS(wan_udp.wan_udphdr_return_code),
			wan_udp.wan_udphdr_return_code);
		cleanup_hardware_level_connection();
		return 1;
	}

	wp_bert_status = (wp_bert_status_t*) get_wan_udphdr_data_ptr(0);

	printf("SW BERT Status/Statistics:\n");
	if (wp_bert_status->state == WP_BERT_STATUS_IN_SYNCH) {
		printf("Status            : IN SYNCH\n");
	} else {
		printf("Status            : OUT OF SYNCH\n");
	}
	
	printf("Error Count       : %d\n", wp_bert_status->errors);
	printf("Synchonized Count : %d\n", wp_bert_status->synchonized_count);

	cleanup_hardware_level_connection();
	return 0;
}

int set_sw_bert(int argc, char *argv[])
{
	int	argi = 0, err = 1;
	char *param;

	for(argi = 1; argi < argc; argi++){
		
		param = argv[argi]; 
		
		if (strcmp(param,"--random") == 0){
			
			err = sw_bert_start(WP_BERT_RANDOM_SEQUENCE);

		}else if (strcmp(param,"--ascendant") == 0){
			
			err = sw_bert_start(WP_BERT_ASCENDANT_SEQUENCE);

		}else if (strcmp(param,"--descendant") == 0){
			
			err = sw_bert_start(WP_BERT_DESCENDANT_SEQUENCE);

		}else if (strcmp(param,"--stop") == 0){

			err = sw_bert_stop();

		}else if (strcmp(param,"--status") == 0){

			err = sw_bert_status();

		}
	}

	return err;
}


/*============================================================================
 * TE1
 * Parse active channel string.
 *
 * Return ULONG value, that include 1 in position `i` if channels i is active.
 */

unsigned int felib_parse_active_channel_str(char* val)
{
#define SINGLE_CHANNEL	0x2
#define RANGE_CHANNEL	0x1
	int channel_flag = 0;
	char* ptr = val;
	int channel = 0, start_channel = 0;
	unsigned int tmp = 0;

	if (strcmp(val,"ALL") == 0)
		return ENABLE_ALL_CHANNELS;
	
	if (strcmp(val,"0") == 0)
		return 0;

	while(*ptr != '\0') {
		//printf("\nMAP DIGIT %c\n", *ptr);
		if (isdigit(*ptr)) {
			channel = strtoul(ptr, &ptr, 10);
			channel_flag |= SINGLE_CHANNEL;
		} else {
			if (*ptr == '-') {
				channel_flag |= RANGE_CHANNEL;
				start_channel = channel;
			} else {
				tmp |= get_active_channels(channel_flag, start_channel, channel);
				channel_flag = 0;
			}
			ptr++;
		}
	}
	if (channel_flag){
		tmp |= get_active_channels(channel_flag, start_channel, channel);
	}

	return tmp;
}

/*============================================================================
 * TE1
 */
static unsigned int get_active_channels(int channel_flag, int start_channel, int stop_channel)
{
	int i = 0;
	unsigned int tmp = 0, mask = 0;

	/* If the channel map is set to 0 then
	 * stop_channel will be zero. In this case just return
	 * 0 */
	if (stop_channel < 1) {
        	return 0;
	}

	if ((channel_flag & (SINGLE_CHANNEL | RANGE_CHANNEL)) == 0)
		return tmp;
	if (channel_flag & RANGE_CHANNEL) { /* Range of channels */
		for(i = start_channel; i <= stop_channel; i++) {
			mask = 1 << (i - 1);
			tmp |=mask;
		}
	} else { /* Single channel */ 
		mask = 1 << (stop_channel - 1);
		tmp |= mask; 
	}
	return tmp;
}


//****** End *****************************************************************/
