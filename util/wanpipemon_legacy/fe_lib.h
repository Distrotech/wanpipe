/*****************************************************************************
* ft1_lib.h	WANPIPE(tm) Multiprotocol WAN Link Driver.
*		Definitions for the Sangoma FT1 adapters.
*
* Author:	Gideon Hack	<ghack@sangoma.com>
*
* Copyright:	(c) 2000 Sangoma Technologies Inc.
*
*		This program is free software; you can redistribute it and/or
*		modify it under the terms of the GNU General Public License
*		as published by the Free Software Foundation; either version
*		2 of the License, or (at your option) any later version.
* ============================================================================
* May 19, 2000	Gideon Hack	Initial version.
*****************************************************************************/
#ifndef	_SDLA_FT1_H
#define	_SDLA_FT1_H

/****** Defines *************************************************************/

/* FT1 LEDs read from parallel port A */
#define PP_A_RT_NOT_RED		0x01		/* the RT LED is not red */
#define PP_A_RT_NOT_GREEN	0x02		/* the RT LED is not green */
#define PP_A_LL_NOT_RED		0x04		/* the LL LED is not red */
#define PP_A_DL_NOT_RED		0x08		/* the DL LED is not red */
 
/* FT1 LEDs read from parallel port B */
#define PP_B_RxD_NOT_GREEN	0x01		/* the RxD LED is not green */
#define PP_B_TxD_NOT_GREEN	0x02		/* the TxD LED is not green */
#define PP_B_ERR_NOT_GREEN	0x04		/* the ERR LED is not green */
#define PP_B_ERR_NOT_RED	0x08		/* the ERR LED is not red */
#define PP_B_INS_NOT_RED	0x10		/* the INS LED is not red */
#define PP_B_INS_NOT_GREEN	0x20		/* the INS LED is not green */
#define PP_B_ST_NOT_GREEN	0x40		/* the ST LED is not green */
#define PP_B_ST_NOT_RED		0x80		/* the ST LED is not red */


typedef struct {
	short RT_red;
	short RT_green;
	short RT_off;
	short LL_red;
	short LL_off;
	short DL_red;
	short DL_off;
	short RxD_green;
	short TxD_green;
	short ERR_green;
	short ERR_red;
	short ERR_off;
	short INS_red;
	short INS_green;
	short INS_off;
	short ST_green;
	short ST_red;
	short ST_off;
} FT1_LED_STATUS;

extern void view_FT1_status( void );
extern void FT1_self_test( void );
extern void FT1_local_loop_mode( void );
extern void FT1_digital_loop_mode( void );
extern void FT1_remote_test( void );
extern void FT1_operational_mode( void );
extern int  remote_running_RT_test( void );
extern void display_FT1_LEDs( void );

extern int get_fe_type(unsigned char*);
extern void set_lb_modes(unsigned char type, unsigned char mode);
extern void read_te1_56k_stat(void);
extern void read_te1_56k_config (void);


#define WAN_TRUE  1 
#define WAN_FALSE 0
#endif	/* _SDLA_FT1_H */

