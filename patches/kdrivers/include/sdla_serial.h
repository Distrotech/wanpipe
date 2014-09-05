/*****************************************************************************
 * sdla_serial.h	Sangoma AFT Base Serial configuration definitions.
 *
 * Author:      Alex Feldman
 *
 * Copyright:	(c) 1995-2001 Sangoma Technologies Inc.
 *
 *		This program is free software; you can redistribute it and/or
 *		modify it under the terms of the GNU General Public License
 *		as published by the Free Software Foundation; either version
 *		2 of the License, or (at your option) any later version.

 * ============================================================================
 * Oct 10, 2007		Alex Feldman Initial version.
 ****************************************************************************
*/

#ifndef _SDLA_SERIAL_H_
#define _SDLA_SERIAL_H_

/*******************************************************************************
			  DEFINES AND MACROS
 ******************************************************************************/

#define AFT_SERIAL_OSC			14745600

#define REG_FLOWCNTRL			0x00

#define BIT_FLOWCNTRL_RTS		0x00000001
#define BIT_FLOWCNTRL_DTR		0x00000002
#define BIT_FLOWCNTRL_CTS		0x00000004
#define BIT_FLOWCNTRL_DCD		0x00000008

#define BITS_BAUDRATE(baud,asyn_if)	((( AFT_SERIAL_OSC / (2 * ((async_if) ? 16 :1) * (baud))) - 1 ) << 8) 

#define BIT_INTCTRL_CLK_MASTER		0x01000000	/* Internal clock (from oscillator) */
#define BIT_INTCTRL_NRZI		0x10000000	/* NRZI		*/
#define BIT_INTCTRL_FM1			0x20000000	/* FM1		*/
#define BIT_INTCTRL_FM0			0x30000000	/* FM0		*/
#define BIT_INTCTRL_MANCHESTER		0x40000000	/* MANCHESTER	*/
#define BIT_INTCTRL_ASYNCH		0x80000000	/* Asynchronous */

#define MAX_SERIAL_LINES		4
/*******************************************************************************
			  FUNCTION PROTOTYPES
 ******************************************************************************/


#ifdef WAN_KERNEL

#define IS_SERIAL_CARD(card)		IS_SERIAL_FEMEDIA(&(card)->fe)

int aft_serial_write_fe(void* phw, ...);
#define __aft_serial_write_fe aft_serial_write_fe
u32 aft_serial_read_fe(void* phw, ...);
#define __aft_serial_read_fe aft_serial_read_fe

int32_t wp_serial_iface_init(void *pfe_iface);

#endif /* WAN_KERNEL */

#endif /* _SDLA_SERIAL_H_ */
