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

#ifndef __SDLA_REMORA_H
# define __SDLA_REMORA_H

#ifdef __SDLA_REMORA_SRC
# define WP_EXTERN
#else
# define WP_EXTERN extern
#endif

# include "aft_core_options.h"
# include "sdla_a600_remora.h"
# include "sdla_b800_remora.h"
# include "sdla_remora_proslic.h"
# include "wanpipe_events.h"


#define DBG_FALSE_RING1 0
#define DBG_FALSE_RING2 0

#if defined(__WINDOWS__)
# define DEBUG_FALSE_RING	if(0)DbgPrint
#else
# define DEBUG_FALSE_RING	if(0)DEBUG_EVENT
#endif

/*******************************************************************************
**			  DEFINES and MACROS
*******************************************************************************/

#define IS_FXOFXS_CARD(card)		IS_FXOFXS_FEMEDIA(&(card)->fe)

#define IS_A200(fe) (((sdla_t*)(fe->card))->adptr_type == A200_ADPTR_ANALOG ||\
		     ((sdla_t*)(fe->card))->adptr_type == A400_ADPTR_ANALOG)

#define IS_A200_CARD(card) (card->adptr_type == A200_ADPTR_ANALOG || \
				card->adptr_type == A400_ADPTR_ANALOG)

#define IS_A700(fe) (((sdla_t*)(fe->card))->adptr_type == AFT_ADPTR_FLEXBRI)
#define IS_A700_CARD(card) (card->adptr_type == AFT_ADPTR_FLEXBRI)

#define WAN_RM_START_CHANNEL		1

#define MAX_REMORA_MODULES		24
#define MAX_FXOFXS_CHANNELS		MAX_REMORA_MODULES
#define NUM_A700_ANALOG_PORTS	2

#define MOD_TYPE_NONE			0
#define MOD_TYPE_FXS			1
#define MOD_TYPE_FXO			2
#define MOD_TYPE_TEST			3
#define MOD_TYPE_EMPTY			4
#define WP_REMORA_DECODE_TYPE(type)			\
		(type == MOD_TYPE_FXS) ? "FXS" :	\
		(type == MOD_TYPE_FXO) ? "FXO" :	\
		(type == MOD_TYPE_TEST) ? "TST" :	\
		(type == MOD_TYPE_TEST) ? "EMPTY" :	\
					"Unknown"

/* SPI interface */
#define MOD_CHAIN_DISABLED	0
#define MOD_CHAIN_ENABLED	1

#define A700_ANALOG_SPI_INTERFACE_REG 0x5C
#define A700_ANALOG_SPI_CS_FXS_CHIP_0 0XFD
#define A700_ANALOG_SPI_CS_FXS_CHIP_1 0XFE
#define A700_ANALOG_SPI_MOD_START_BIT (1<<31)

#define SPI_INTERFACE_REG	0x54

#define MOD_SPI_ADDR_FXS_READ	0x80
#define MOD_SPI_ADDR_FXS_WRITE	0x00

#define MOD_SPI_CS_FXS_CHIP_0	0x01
#define MOD_SPI_CS_FXS_CHIP_1	0x02

#define MOD_SPI_CS_FXO_CHIP_1	0x08
#define MOD_SPI_CS_FXO_WRITE	0x00
#define MOD_SPI_CS_FXO_READ	0x40

#define MOD_SPI_CTRL_V3_START	0x08	/* bit 27 used as START in version 1-2-3 */
#define MOD_SPI_CTRL_START	0x80
#define MOD_SPI_CTRL_FXS	0x10
#define MOD_SPI_CTRL_CHAIN	0x20

#define MOD_SPI_RESET		0x40000000
#define MOD_SPI_BUSY		0x80000000
#define MOD_SPI_V3_STAT		0x08000000	/* bit 27 used as START in version 1-2-3 */
#define MOD_SPI_START		0x08000000

#define WAN_FXS_NUM_REGS		109
#define WAN_FXS_NUM_INDIRECT_REGS	105

#define WAN_FXO_NUM_REGS		59

#define WAN_REMORA_READREG		0x01;

#define RM_FE_MAGIC		0x150D

#define WAN_RM_OPERMODE_LEN	20

/* Front-End UDP command */
#define WAN_FE_TONES		(WAN_FE_UDP_CMD_START + 0)
#define WAN_FE_RING			(WAN_FE_UDP_CMD_START + 1)
#define WAN_FE_REGDUMP		(WAN_FE_UDP_CMD_START + 2)
#define WAN_FE_STATS		(WAN_FE_UDP_CMD_START + 3)

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

#define MIN_OFFHOOK_THRESHOLD 5
#define MAX_OFFHOOK_THRESHOLD 20

/*Analog FXO Operation mode */

#define	WAN_RM_DEFAULT		0x00
#define WAN_RM_TAPPING		0x01

/*******************************************************************************
**			  TYPEDEF STRUCTURE
*******************************************************************************/
typedef struct sdla_remora_cfg_ {

	int		opermode;
	char		opermode_name[WAN_RM_OPERMODE_LEN];
/*	int	tdmv_law;*/	/* WAN_TDMV_ALAW or WAN_TDMV_MULAW */
	int		reversepolarity;
	int		battthresh;
	int 		ohthresh;
	int		battdebounce;	

	u_int8_t	relaxcfg;	/* do not failed during config if one of 
					** the modules failed to configure */

	int		fxs_rxgain;
	int		fxs_txgain;
	u_int8_t	fxs_fastringer;
	u_int8_t	fxs_lowpower;
	u_int8_t	fxs_pulsedialing;

	int		fxo_rxgain;
	int		fxo_txgain;

	int		fxs_ringampl;
	u_int8_t	rm_mode; 	/*Analog Operation mode: default or tapping */
        u_int8_t        fake_polarity;  /*FAKE Polarity event generation : YES or NO*/
        int             fake_polarity_thres; /*Threshold value(only for Analoge cards,units defined in L16 sample value)
					       to decide generation of FAKE Polarity event*/
	u_int8_t	rm_lcm; /*Analog Loop Current Measure (LCM) : Yes Or NO */
} sdla_remora_cfg_t;

typedef struct {
	unsigned char direct[WAN_FXS_NUM_REGS];
	unsigned short indirect[WAN_FXS_NUM_INDIRECT_REGS];
} wan_remora_fxs_regs_t;

typedef struct {
	unsigned char direct[WAN_FXO_NUM_REGS];
} wan_remora_fxo_regs_t;

typedef struct {
	int		tip_volt;	/* TIP voltage (mV) (FXS) */
	int		ring_volt;	/* RING voltage (mV) (FXS) */
	int		bat_volt;	/* VBAT voltage (mV) (FXS) */
	int		volt;		/* Line voltage status (FXO) */
	/*u_int8_t	hook;	*/	/* On/Off hook state */
	unsigned char	status;		/* Line connected/disconnecd */	
} wan_remora_stats_t;

typedef struct {
	int	mod_no;
	int	type;
	
	union {
		wan_remora_fxs_regs_t	regs_fxs;
		wan_remora_fxo_regs_t	regs_fxo;
		wan_remora_stats_t	stats;
	} u;
} wan_remora_udp_t;

struct wan_rm_echo_coefs {
	unsigned char acim;
	unsigned char coef1;
	unsigned char coef2;
	unsigned char coef3;
	unsigned char coef4;
	unsigned char coef5;
	unsigned char coef6;
	unsigned char coef7;
	unsigned char coef8;
};



#if defined(WAN_KERNEL)

#define NUM_CAL_REGS		12

#if !defined(WAN_DEBUG_FE)
# define WRITE_RM_FXS_REG(mod_no,chain,reg,val)			\
	fe->write_fe_reg(	((sdla_t*)fe->card)->hw,	\
				(int)mod_no,		\
				(int)MOD_TYPE_FXS,	\
				(int)chain,		\
				(int)reg,		\
				(int)val)
# define READ_RM_FXS_REG(mod_no,chain,reg)			\
	fe->read_fe_reg(	((sdla_t*)fe->card)->hw,	\
				(int)mod_no,		\
				(int)MOD_TYPE_FXS,	\
				(int)chain,		\
				(int)reg)
# define WRITE_RM_FXO_REG(mod_no,chain,reg,val)			\
	fe->write_fe_reg(	((sdla_t*)fe->card)->hw,		\
				(int)mod_no,		\
				(int)MOD_TYPE_FXO,	\
				(int)chain,		\
				(int)reg,		\
				(int)val)
# define READ_RM_FXO_REG(mod_no,chain,reg)			\
	fe->read_fe_reg(	((sdla_t*)fe->card)->hw,		\
				(int)mod_no,		\
				(int)MOD_TYPE_FXO,	\
				(int)chain,		\
				(int)reg)
# define WRITE_RM_REG(mod_no,reg,val)					\
	fe->write_fe_reg(	((sdla_t*)fe->card)->hw,		\
				(int)mod_no,			\
				(int)fe->rm_param.mod[mod_no].type,	\
				(int)fe->rm_param.mod[mod_no].chain,	\
				(int)reg, (int)val)
# define READ_RM_REG(mod_no,reg)					\
	fe->read_fe_reg(	((sdla_t*)fe->card)->hw,				\
				(int)mod_no,				\
				(int)fe->rm_param.mod[mod_no].type,		\
				(int)fe->rm_param.mod[mod_no].chain,		\
				(int)reg)
#else
# define WRITE_RM_FXS_REG(mod_no,chain,reg,val)		\
	fe->write_fe_reg(	((sdla_t*)fe->card)->hw,		\
				(int)mod_no,		\
				(int)MOD_TYPE_FXS,	\
				(int)chain,		\
				(int)reg, (int)val,	\
				__FILE__,(int)__LINE__)
# define READ_RM_FXS_REG(mod_no,chain,reg)				\
	fe->read_fe_reg(	((sdla_t*)fe->card)->hw, 		\
				(int)mod_no,		\
				(int)MOD_TYPE_FXS,	\
				(int)chain,		\
				(int)reg,		\
				__FILE__,__LINE__)
# define WRITE_RM_FXO_REG(mod_no,chain,reg,val)				\
	fe->write_fe_reg(	((sdla_t*)fe->card)->hw,		\
				(int)mod_no,		\
				(int)MOD_TYPE_FXO,	\
				(int)chain,		\
				(int)reg,(int)val,	\
				__FILE__,(int)__LINE__)
# define READ_RM_FXO_REG(mod_no,chain,reg)				\
	fe->read_fe_reg(	((sdla_t*)fe->card)->hw, 		\
				(int)mod_no,		\
				(int)MOD_TYPE_FXO,	\
				(int)chain,		\
				(int)reg,		\
				__FILE__,(int)__LINE__)
# define WRITE_RM_REG(mod_no,reg,val)					\
	fe->write_fe_reg(	((sdla_t*)fe->card)->hw,				\
				(int)mod_no,				\
				(int)fe->rm_param.mod[mod_no].type,		\
				(int)fe->rm_param.mod[mod_no].chain,		\
				(int)reg, (int)val,__FILE__,(int)__LINE__)
# define READ_RM_REG(mod_no,reg)					\
	fe->read_fe_reg(	((sdla_t*)fe->card)->hw,				\
				(int)mod_no,				\
				(int)fe->rm_param.mod[mod_no].type,		\
				(int)fe->rm_param.mod[mod_no].chain,		\
				(int)reg,__FILE__,(int)__LINE__)
#endif
#define __READ_RM_REG(mod_no,reg)					\
	fe->__read_fe_reg(	((sdla_t*)fe->card)->hw,				\
				(int)mod_no,				\
				(int)fe->rm_param.mod[mod_no].type,		\
				(int)fe->rm_param.mod[mod_no].chain,		\
				(int)reg)


enum proslic_power_warn {
	PROSLIC_POWER_UNKNOWN = 0,
	PROSLIC_POWER_ON,
	PROSLIC_POWER_WARNED,
};

/* Sangoma A200 event bit map */
#define WAN_RM_EVENT_DTMF		1	/* DTMF event */
#define WAN_RM_EVENT_LC			2	/* Loop closure event */
#define WAN_RM_EVENT_RING_TRIP		3	/* Ring trip event */
#define WAN_RM_EVENT_POWER		4	/* Power event */
#define WAN_RM_EVENT_RING		5	/* Ring event */
#define WAN_RM_EVENT_TONE		6	/* Play tone */
#define WAN_RM_EVENT_RING_DETECT	7	/* Ring detect event */

typedef struct {
	unsigned char vals[NUM_CAL_REGS];
} callregs_t;

typedef struct {
	int	ready;

	u8	status;			/* line status (connected/disconnected) */
	int	statusdebounce;		/* line status debounce */

	int	ring_detect;	
	
	int	offhook;		/* Xswitch */
	int	battery;		/* Xswitch */
	int	battdebounce;		/* Xswitch */
	int i_debounce;
	int ringdebounce;
	int	wasringing;
	int	nobatttimer;
	int	lastpol;
	int	polarity;
	int	polaritydebounce;
	
	/* Variables required for tapper mode */
	int	ohdebounce;
	int	going_offhook;	/* current ohdebounce is for off-hk or on-hk */
	
	unsigned char	imask;		/* interrupt mask */

	int   				readcid;
        unsigned int 			cidtimer;

	/*Additional for Zaptel mode*/
#if defined(CONFIG_PRODUCT_WANPIPE_TDM_VOICE)
	int				echotune;	/* echo tune */
	struct wan_rm_echo_coefs	echoregs;	/* echo tune */
#endif
} wp_remora_fxo_t;

typedef struct {
	int		ready;
	int		idletxhookstate;
	enum proslic_power_warn proslic_power;
	callregs_t	callregs;
	unsigned char	imask1;		/* interrupt mask 1 */
	unsigned char	imask2;
	unsigned char	imask3;

	int		oldrxhook;		/* Xswitch */
	int		lasttxhook;		/* Xswitch */
	int		lastrxhook;
	int		ohttimer;		/* Xswitch */
	int		palarms;		/* Xswitch */
	int		debounce;
	int		debouncehook;

	/*Additional for Zaptel mode */
	int	lasttxhook_update;
	/*Flash/hook timer */

	int		itimer;
	int		rxflashtime;

} wp_remora_fxs_t;

typedef struct {
	int		type;
	int		chain;
	unsigned long	events;
	unsigned long 	lastcmd;

#if 0
#if defined(__WINDOWS__)
	wan_event_ctrl_t	 current_control_event;
	wan_event_ctrl_t	*current_control_event_ptr;
#else
	WAN_LIST_HEAD(, wan_event_ctrl_)	event_head;
#endif
#endif	
	/* TDM Voice applications */
	int		sig;
	/* Special fxs/fxo settings */
	union {
		wp_remora_fxo_t	fxo;
		wp_remora_fxs_t	fxs;
	} u;

} wp_remora_module_t;

typedef struct {
//	u_int16_t	type;
	unsigned int	mod_no;		/* A200-Remora */	
	unsigned char	ec_dtmf_port;	/* EC DTMF: SOUT or ROUT */
	unsigned long	ts_map;
	u_int8_t	tone;
	int		ohttimer;	/* On-hook transfer */
	int		polarity;	/* SETPOLARITY */		
	unsigned short	reg;		/* fe register */	
	unsigned char	value;		/* fe register value */	
	int	 	rm_gain;	/* Tx/Rx Gain */
} sdla_rm_event_t;

typedef struct sdla_remora_param {
	int			not_used;

	wp_remora_module_t	mod[MAX_REMORA_MODULES];

	u32		module_map;	/* Map of available module */
	u16		max_fe_channels;/* Number of available modules */
	
	u8		critical;
//	wan_timer_t	timer;
//	u8		timer_cmd;
//	u16		timer_mod_no;
//	u16		timer_delay;

	u32		intcount;
	wan_ticks_t	last_watchdog;	

	int		reg_dbg_busy;	
	int		reg_dbg_ready;	
	unsigned char	reg_dbg_value;
	/* use REG_SHADOW in all modes */
	unsigned char	reg0shadow[MAX_REMORA_MODULES];	/* read> fxs: 68 fxo: 5 */
	unsigned char	reg1shadow[MAX_REMORA_MODULES];	/* read> fxs: 64 fxo: 29 */
	unsigned char	reg2shadow[MAX_REMORA_MODULES];	/* read> fxs: 64 fxo: 29 */
	unsigned char	reg3shadow[MAX_REMORA_MODULES]; /* read > fxs : 19 for Ring/Trip Evnet , FXO -used LCS2 (read 28) */
	unsigned char	reg4shadow[MAX_REMORA_MODULES]; /* read > fxs : 20 for DTMF Evnet , FXO no use yet */

	unsigned char	reg0shadow_write[MAX_REMORA_MODULES];	/* write> fxs: 68 fxo: 5 */
	int		reg0shadow_update[MAX_REMORA_MODULES];
	int		battdebounce;		/* global for FXO */
	int		battthresh;		/* global for FXO */
	int		wp_rm_chunk_size;	/* TDM API set as MTU for Zaptel set as ZT_CHUNK_SIZE */

#if DBG_FALSE_RING1
	u32 last_system_ticks;
	u32 last_intcount;
	u32	ticks_diff;
	u32 int_diff;
#endif

} sdla_remora_param_t;


#endif /* WAN_KERNEL */

/*******************************************************************************
**			  FUNCTION PROTOTYPES
*******************************************************************************/
extern int	wp_remora_iface_init(void*, void*);
extern int	wp_a700_remora_iface_init(void*, void*);

#undef WP_EXTERN
#endif	/* __SDLA_REMORA_H */

