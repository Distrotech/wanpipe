/*****************************************************************************
* fe_lib.c	Front End Library 
*
* Author:       Nenad Corbic <ncorbic@sangoma.com>	
*		Alex Feldman <al.feldman@sangoma.com>
*
* Copyright:	(c) 1995-2000 Sangoma Technologies Inc.
*
*		This program is free software; you can redistribute it and/or
*		modify it under the terms of the GNU General Public License
*		as published by the Free Software Foundation; either version
*		2 of the License, or (at your option) any later version.
* ----------------------------------------------------------------------------
* May 24, 2000  Gideon Hack     Modifications for FT1 adapters
* Sep 21, 1999  Nenad Corbic    Changed the input parameters, hearders
*                               data types. More user friendly.
*****************************************************************************/

/******************************************************************************
 * 			INCLUDE FILES					      *
 *****************************************************************************/
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>

#include "unixio.h"
#if defined(__LINUX__)
# include <linux/wanpipe_defines.h>
# include <linux/wanpipe_cfg.h>
# include <linux/wanpipe.h>
#else
# include <wanpipe_defines.h>
# include <wanpipe_cfg.h>
# include <wanpipe.h>
#endif
#include "fe_lib.h"
#include "wanpipemon.h"

/******************************************************************************
 * 			DEFINES/MACROS					      *
 *****************************************************************************/
#define TIMEOUT 1
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

/******************************************************************************
 * 			GUI MENU DEFINITION				      *
 *****************************************************************************/


char *csudsu_menu[]={
"","--- TE3/TE1 (S514-X/A10X) Stats ---",
""," ",
"Ta","Read TE3/TE1/56K alarms", 
"Tallb","E Line Loopback",  
"Tdllb","D Line Loopback",  
"Taplb","E Payload Loopback",  
"Tdplb","D Payload Loopback",  
"Tadlb","E Diag Digital Loopback",  
"Tddlb","D Diag Digital Loopback",  
"Tsalb","Send Loopback Activate Code",  
"Tsdlb","Send Loopback Deactive Code",  
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


/******************************************************************************
 * 			FUNCTION DEFINITION				      *
 *****************************************************************************/
/* Display the status of all the lights */
void view_FT1_status( void )
{
	int FT1_LED_read_count = 0;
	int key;
	struct timeval to;
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
	gettimeofday(&to, NULL);
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
 				usleep(100000);
				gettimeofday(&to, NULL);
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
					usleep(50000);
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
		gettimeofday(&to, NULL);
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
		usleep(50000);
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
                usleep(50000);
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
                                usleep(100000);
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
                usleep(50000);
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
                                usleep(100000);
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
                usleep(50000);
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
                usleep(50000);
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
                                usleep(100000);
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
                usleep(50000);
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
                                usleep(100000);
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
                usleep(50000);
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
                                usleep(100000);
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
		usleep(1000000);
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
                usleep(50000);
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



int get_fe_type(unsigned char* adapter_type)
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

void set_lb_modes(unsigned char type, unsigned char mode)
{
	
	wan_udp.wan_udphdr_command	= WAN_FE_LB_MODE;
	wan_udp.wan_udphdr_data_len	= 2;
	wan_udp.wan_udphdr_return_code	= 0xaa;

	set_wan_udphdr_data_byte(0,type);
	set_wan_udphdr_data_byte(1,mode);

#if 0
	if (wan_protocol == WANCONFIG_CHDLC){
		wan_udp.wan_udphdr_chdlc_data[0] = type;
		wan_udp.wan_udphdr_chdlc_data[1] = mode;
	}else{
		wan_udp.wan_udphdr_data[0] = type;
		wan_udp.wan_udphdr_data[1] = mode;
	}
#endif
	DO_COMMAND(wan_udp);
	if (wan_udp.wan_udphdr_return_code != 0){
		printf("Failed to %s line loopback mode.\n",
			(mode == WAN_TE1_LB_ENABLE) ? "activate" : "deactivate");
	}else{
		printf("Line loopback mode is %s!\n",
			(mode == WAN_TE1_LB_ENABLE) ? "activated" : "deactivated");
	}
	return;
}



void read_te1_56k_stat(void)
{
	unsigned char* data = NULL;
	unsigned long 	alarms = 0x00;
	unsigned char	adapter_type = 0x00;

	/* Read Adapter Type */
	if (get_fe_type(&adapter_type)){
		return;
	}

	/* Read T1/E1/56K alarms and T1/E1 performance monitoring counters */
	wan_udp.wan_udphdr_command = WAN_FE_GET_STAT;
	wan_udp.wan_udphdr_data_len = 0;
    	wan_udp.wan_udphdr_return_code = 0xaa;
	DO_COMMAND(wan_udp);
	if (wan_udp.wan_udphdr_return_code != 0){
		printf("Failed to read T1/E1/56K statistics.\n");
		return;
	}

	data = get_wan_udphdr_data_ptr(0);

#if 0
	if (wan_protocol == WANCONFIG_CHDLC){
		data = &wan_udp.wan_udphdr_chdlc_data[0];
	}else{
		data = &wan_udp.wan_udphdr_data[0];
	}
#endif

	alarms = *(unsigned long*)data;
	if (adapter_type == WAN_MEDIA_T1 || adapter_type == WAN_MEDIA_E1){
		printf("***** %s: %s Alarms *****\n\n",
			if_name, (adapter_type == WAN_MEDIA_T1) ? "T1" : "E1");
		printf("ALOS:\t%s\t| LOS:\t%s\n", 
				WAN_TE_PRN_ALARM_ALOS(alarms), 
				WAN_TE_PRN_ALARM_LOS(alarms));
		printf("RED:\t%s\t| AIS:\t%s\n", 
				WAN_TE_PRN_ALARM_RED(alarms), 
				WAN_TE_PRN_ALARM_AIS(alarms));
		if (adapter_type == WAN_MEDIA_T1){ 
			printf("YEL:\t%s\t| LOF:\t%s\n", 
					WAN_TE_PRN_ALARM_YEL(alarms), 
					WAN_TE_PRN_ALARM_LOF(alarms));
		}else{
			printf("LOF:\t%s\n", 
					WAN_TE_PRN_ALARM_LOF(alarms));
		}

	}else if  (adapter_type == WAN_MEDIA_DS3 || adapter_type == WAN_MEDIA_E3){
		printf("***** %s: %s Alarms *****\n\n",
			if_name, (adapter_type == WAN_MEDIA_DS3) ? "DS3" : "E3");

		if (adapter_type == WAN_MEDIA_DS3){
			printf("AIS:\t%s\t| LOS:\t%s\n",
					WAN_TE3_AIS_ALARM(alarms),
					WAN_TE3_LOS_ALARM(alarms));

			printf("OOF:\t%s\t| YEL:\t%s\n",
					WAN_TE3_OOF_ALARM(alarms),
					WAN_TE3_YEL_ALARM(alarms));
		}else{
			printf("AIS:\t%s\t| LOS:\t%s\n",
					WAN_TE3_AIS_ALARM(alarms),
					WAN_TE3_LOS_ALARM(alarms));

			printf("OOF:\t%s\t| YEL:\t%s\n",
					WAN_TE3_OOF_ALARM(alarms),
					WAN_TE3_YEL_ALARM(alarms));
			
			printf("LOF:\t%s\t\n",
					WAN_TE3_LOF_ALARM(alarms));
		}
		
	}else if (adapter_type == WAN_MEDIA_56K){
		printf("***** %s: 56K CSU/DSU Alarms *****\n\n\n", if_name);
	 	printf("In Service:\t\t%s\tData mode idle:\t\t%s\n",
			 	INS_ALARM_56K(alarms), 
			 	DMI_ALARM_56K(alarms));
	
	 	printf("Zero supp. code:\t%s\tCtrl mode idle:\t\t%s\n",
			 	ZCS_ALARM_56K(alarms), 
			 	CMI_ALARM_56K(alarms));

	 	printf("Out of service code:\t%s\tOut of frame code:\t%s\n",
			 	OOS_ALARM_56K(alarms), 
			 	OOF_ALARM_56K(alarms));
		
	 	printf("Valid DSU NL loopback:\t%s\tUnsigned mux code:\t%s\n",
			 	DLP_ALARM_56K(alarms), 
			 	UMC_ALARM_56K(alarms));

	 	printf("Rx loss of signal:\t%s\t\n",
			 	RLOS_ALARM_56K(alarms)); 
		
	}else{
		printf("***** %s: Unknown Front End 0x%X *****\n\n",
			if_name, adapter_type);
	}

	if (adapter_type == WAN_MEDIA_T1 || adapter_type == WAN_MEDIA_E1){
		sdla_te_pmon_t*	pmon = NULL;
		pmon = (sdla_te_pmon_t*)&data[sizeof(unsigned long)];
		printf("\n\n***** %s: %s Performance Monitoring Counters *****\n\n",
				if_name, (adapter_type == WAN_MEDIA_T1) ? "T1" : "E1");
		if (pmon->mask & WAN_TE_BIT_PMON_LCV){
			printf("Line Code Violation\t: %d\n",
						pmon->lcv_errors);
		}
		if (pmon->mask & WAN_TE_BIT_PMON_BEE){
			printf("Bit Errors\t: %d\n",
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
			printf("CRC4 Errors\t: %d\n",
						pmon->crc4_errors);
		}
		if (pmon->mask & WAN_TE_BIT_PMON_FER){
			printf("Framing Bit Errors\t: %d\n",
						pmon->fer_errors);
		}
		if (pmon->mask & WAN_TE_BIT_PMON_FAS){
			printf("FAS Errors\t: %d\n",
						pmon->fas_errors);
		}
	}

	if (adapter_type == WAN_MEDIA_DS3 || adapter_type == WAN_MEDIA_E3){
		sdla_te3_pmon_t*	pmon = NULL;
		pmon = (sdla_te3_pmon_t*)&data[sizeof(unsigned long)];
		printf("\n\n***** %s: %s Performance Monitoring Counters *****\n\n",
				if_name, (adapter_type == WAN_MEDIA_DS3) ? "DS3" : "E3");

		printf("Framing Bit Error:\t%d\tLine Code Violation:\t%d\n", 
				pmon->pmon_framing,
				pmon->pmon_lcv);

		if (adapter_type == WAN_MEDIA_DS3){
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
	
	return;
}

void flush_te1_pmon(void)
{
	unsigned char	adapter_type = 0x00;

	/* Read Adapter Type */
	if (get_fe_type(&adapter_type)){
		return;
	}

	switch(adapter_type){
	case WAN_MEDIA_T1:
	case WAN_MEDIA_E1:
		/* Flush perfomance mono\itoring counters */
		wan_udp.wan_udphdr_command = WAN_FE_FLUSH_PMON;
		wan_udp.wan_udphdr_data_len = 0;
	    	wan_udp.wan_udphdr_return_code = 0xaa;
		DO_COMMAND(wan_udp);
		if (wan_udp.wan_udphdr_return_code != 0){
			printf("Failed to flush Perfomance Monitoring counters.\n");
			return;
		}
		break;
	}
	return;
}


void read_te1_56k_config (void)
{
	unsigned char	adapter_type = 0x00;

	/* Read Adapter Type */
	if (get_fe_type(&adapter_type)){
		return;
	}

	if (adapter_type == WAN_MEDIA_T1 || adapter_type == WAN_MEDIA_E1){
		int num_of_chan = (adapter_type == WAN_MEDIA_T1) ? 
						NUM_OF_T1_CHANNELS : 
						NUM_OF_E1_TIMESLOTS;
		int i = 0, start_chan = 0;

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
			printf("CSU/DSU %s Conriguration:\n", if_name);
			printf("\tMedia type\t%s\n",
				MEDIA_DECODE(fe_cfg));
			printf("\tFraming\t\t%s\n",
				FRAME_DECODE(fe_cfg));
			printf("\tEncoding\t%s\n",
				LCODE_DECODE(fe_cfg));
			if (adapter_type == WAN_MEDIA_T1){
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
	return;
}

