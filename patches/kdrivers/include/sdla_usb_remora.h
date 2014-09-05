/*******************************************************************************
** sdla_remora.h	
**
** Author: 	Alex Feldman  <al.feldman@sangoma.com>
**
** Copyright:	(c) 2005 Sangoma Technologies Inc.
**
**		This program is free software; you can redistribute it and/or
**		modify it under the terms of the GNU General Public License
**		as published by the Free Software Foundation; either version
**		2 of the License, or (at your option) any later version.
** ============================================================================
** Oct 6, 2005	Alex Feldman	Initial version.
*******************************************************************************/

#ifndef __SDLA_USB_REMORA_H
# define __SDLA_USB_REMORA_H

#ifdef __SDLA_REMORA_SRC
# define WP_EXTERN
#else
# define WP_EXTERN extern
#endif

/*******************************************************************************
**			  DEFINES and MACROS
*******************************************************************************/

#define IS_FXOFXS_CARD(card)		IS_FXOFXS_FEMEDIA(&(card)->fe)

#define MAX_USB_REMORA_MODULES		2
#define MAX_USB_FXOFXS_CHANNELS		MAX_USB_REMORA_MODULES

/* Front-End UDP command */
#if defined(__WINDOWS__)
#define WAN_FE_TONES		13
#define WAN_FE_RING		(WAN_FE_TONES + 1)
#define WAN_FE_REGDUMP		(WAN_FE_TONES + 2)
#define WAN_FE_STATS		(WAN_FE_TONES + 3)
#else
#define WAN_FE_TONES		(WAN_FE_UDP_CMD_START + 0)
#define WAN_FE_RING		(WAN_FE_UDP_CMD_START + 1)
#define WAN_FE_REGDUMP		(WAN_FE_UDP_CMD_START + 2)
#define WAN_FE_STATS		(WAN_FE_UDP_CMD_START + 3)
#endif

#define WAN_RM_SET_ECHOTUNE	_IOW (ZT_CODE, 63, struct wan_rm_echo_coefs)

/* RM interrupt types */
#define WAN_RM_INTR_NONE	0x00
#define WAN_RM_INTR_GLOBAL	0x01

/* Signalling types */
#define __WAN_RM_SIG_FXO	 (1 << 12)			/* Never use directly */
#define __WAN_RM_SIG_FXS	 (1 << 13)			/* Never use directly */

#define WAN_RM_SIG_NONE		(0)				/* Channel not configured */
#define WAN_RM_SIG_FXSLS	((1 << 0) | __WAN_RM_SIG_FXS)	/* FXS, Loopstart */
#define WAN_RM_SIG_FXSGS	((1 << 1) | __WAN_RM_SIG_FXS)	/* FXS, Groundstart */
#define WAN_RM_SIG_FXSKS	((1 << 2) | __WAN_RM_SIG_FXS)	/* FXS, Kewlstart */

#define WAN_RM_SIG_FXOLS	((1 << 3) | __WAN_RM_SIG_FXO)	/* FXO, Loopstart */
#define WAN_RM_SIG_FXOGS	((1 << 4) | __WAN_RM_SIG_FXO)	/* FXO, Groupstart */
#define WAN_RM_SIG_FXOKS	((1 << 5) | __WAN_RM_SIG_FXO)	/* FXO, Kewlstart */

#define WAN_RM_SIG_EM		(1 << 6)			/* Ear & Mouth (E&M) */


/*******************************************************************************
**			  TYPEDEF STRUCTURE
*******************************************************************************/

#if defined(WAN_KERNEL)

#define NUM_CAL_REGS		12

#if !defined(WAN_DEBUG_FE)
# define WRITE_USB_RM_REG(mod_no,reg,val)					\
	fe->write_fe_reg(	((sdla_t*)fe->card)->hw,		\
				(int)mod_no,			\
				(int)reg, (int)val)
# define READ_USB_RM_REG(mod_no,reg)					\
	fe->read_fe_reg(	((sdla_t*)fe->card)->hw,			\
				(int)mod_no,		\
				(int)reg)
#else
# define WRITE_USB_RM_REG(mod_no,reg,val)					\
	fe->write_fe_reg(	((sdla_t*)fe->card)->hw,				\
				(int)mod_no,				\
				(int)reg, (int)val,__FILE__,(int)__LINE__)
# define READ_USB_RM_REG(mod_no,reg)					\
	fe->read_fe_reg(	((sdla_t*)fe->card)->hw,				\
				(int)mod_no,				\
				(int)reg,__FILE__,(int)__LINE__)
#endif

/* Sangoma A200 event bit map */
#define WAN_RM_EVENT_DTMF		1	/* DTMF event */
#define WAN_RM_EVENT_LC			2	/* Loop closure event */
#define WAN_RM_EVENT_RING_TRIP		3	/* Ring trip event */
#define WAN_RM_EVENT_POWER		4	/* Power event */
#define WAN_RM_EVENT_RING		5	/* Ring event */
#define WAN_RM_EVENT_TONE		6	/* Play tone */
#define WAN_RM_EVENT_RING_DETECT	7	/* Ring detect event */

#endif /* WAN_KERNEL */

/*******************************************************************************
**			  FUNCTION PROTOTYPES
*******************************************************************************/
extern int	wp_usb_remora_iface_init(void*, void*);

#undef WP_EXTERN
#endif	/* __SDLA_USB_REMORA_H */
