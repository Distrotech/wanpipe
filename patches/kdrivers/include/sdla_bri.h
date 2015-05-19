/***************************************************************************
 * sdla_bri.h	Sangoma ISDN-BRI definitions (for Cologne XHFC-2SU chip).
 *
 * Author(s): 	David Rokhvarg   <davidr@sangoma.com>
 *
 * Copyright:	(c) 1984 - 2007 Sangoma Technologies Inc.
 *
 *		This program is free software; you can redistribute it and/or
 *		modify it under the terms of the GNU General Public License
 *		as published by the Free Software Foundation; either version
 *		2 of the License, or (at your option) any later version.
 *
 * ============================================================================
 * February	14, 2007	David Rokhvarg
 *				v1.0 Initial version.
 *
 * February	26, 2008	David Rokhvarg	
 *				v1.1 Imrovements in SU State transition code.
 *				Implemented T3 and T4 timers.
 *
 * July		21  2009	David Rokhvarg	
 *				v1.4 Implemented T1 timer for NT.
 *
 ******************************************************************************
 */

#ifndef	__SDLA_BRI_H
# define __SDLA_BRI_H


#include "xhfc24succ.h"

/*******************************************************************************
**			  DEFINES and MACROS
*******************************************************************************/

#define IS_BRI_CARD(card)		IS_BRI_FEMEDIA(&(card)->fe)

#define BRI_FE_CFG(fe_cfg)		(fe_cfg.cfg.bri)

#define BRI_FE_CLK(fe_cfg)		BRI_FE_CFG(fe_cfg).clock_mode

#define BRI_CARD_CLK(card)		BRI_FE_CLK(card->fe.fe_cfg)

#define WAN_BRI_START_CHANNEL		1

#define MAX_BRI_REMORAS			4
#define MAX_BRI_MODULES_PER_REMORA	3

/*
	Maximum of 4 remoras, 3 modules on each - maximum 12 modules.
	On each module there is 1 dual-port ISDN BRI chip. 
	On each line there are 2 B-channels and 1 D-channel.
*/
#define MAX_BRI_MODULES			12
#define MAX_BRI_CHANNELS		2	/* Number of channels per device */
#define MAX_BRI_LINES			(MAX_BRI_MODULES*MAX_BRI_CHANNELS)
#define A700_MAX_BRI_LINES		4

/*	
	The maximum number of BRI timeslots is 32.
	This is needed for the d-chan, which is always at the otherwise unused timeslot 31.
	But AFT hardware is actually using only 24 timeslots - only for the b-chans.
*/
#define MAX_TIMESLOTS			32
#define BRI_DCHAN_LOGIC_CHAN		31

#define BRI_DCHAN_ACTIVE_CFG_CHAN	2

#define MAX_BRI_TIMESLOTS		3 /* this is the number of b-chans (2) and the d-chan on one BRI line */

#define MAX_TENT_CHANNELS		MAX_BRI_MODULES

#define MOD_TYPE_NONE			0	/* corresponds to module which is not installed */
#define MOD_TYPE_NT			1
#define MOD_TYPE_TE			2
#define MOD_TYPE_TEST			3

#define WP_BRI_DECODE_MOD_TYPE(type)		\
		(type == MOD_TYPE_NT) ? "NT" :	\
		(type == MOD_TYPE_TE) ? "TE" :	"Unknown"

/* SPI interface */
#define SPI_INTERFACE_REG	0x54
#define SPI_MAX_RETRY_COUNT	10

#define MOD_SPI_RESET		0x40000000
#define MOD_SPI_BUSY		0x80000000
#define MOD_SPI_V3_STAT		0x08000000	/* bit 27 used as START in version 1-2-3 */
#define MOD_SPI_START		0x08000000

#define BRI_FE_MAGIC		0x150D

#define WAN_BRI_OPERMODE_LEN	20

#if !defined(__WINDOWS__)
/* Front-End UDP command */
#define WAN_FE_TONES		(WAN_FE_UDP_CMD_START + 0)
#define WAN_FE_RING		(WAN_FE_UDP_CMD_START + 1)
#define WAN_FE_REGDUMP		(WAN_FE_UDP_CMD_START + 2)
#define WAN_FE_STATS		(WAN_FE_UDP_CMD_START + 3)

#define WAN_BRI_SET_ECHOTUNE	_IOW (ZT_CODE, 63, struct wan_bri_echo_coefs)
#endif

/******************************************************************************
** XHFC definitions
*******************************************************************************/

/* max. number of S/U ports per XHFC-2SU controller */
#define BRI_MAX_PORTS_PER_CHIP	2
#define PORT_0	0
#define PORT_1	1

/* flags in _u16  port mode */
#define PORT_UNUSED		0x0000
#define PORT_MODE_NT		0x0001
#define PORT_MODE_TE		0x0002
#define PORT_MODE_S0		0x0004
#define PORT_MODE_UP		0x0008
#define PORT_MODE_EXCH_POL	0x0010
#define PORT_MODE_LOOP		0x0020
#define NT_TIMER		0x8000

/* NT / TE defines */
#define CLK_DLY_TE	0x0e	/* CLKDEL in TE mode */
#define CLK_DLY_NT	0x6c	/* CLKDEL in NT mode */
#define STA_ACTIVATE	0x60	/* start activation   in A_SU_WR_STA */
#define STA_DEACTIVATE	0x40	/* start deactivation in A_SU_WR_STA */
#define LIF_MODE_NT	0x04	/* Line Interface NT mode */

#define XHFC_TIMER_T3   8000    /* 8s activation timer T3 */
#define XHFC_TIMER_T4   500     /* 500ms deactivation timer T4 */

#define XHFC_TIMER_T1   10000	/* 10s de-activation timer T1 */

/* xhfc Layer1 physical commands */
#define HFC_L1_ACTIVATE_TE			0x01
#define HFC_L1_FORCE_DEACTIVATE_TE	0x02
#define HFC_L1_ACTIVATE_NT			0x03
#define HFC_L1_DEACTIVATE_NT		0x04
#define HFC_L1_ENABLE_LOOP_B1		0x05
#define HFC_L1_ENABLE_LOOP_B2		0x06
#define HFC_L1_DISABLE_LOOP_B1		0x07
#define HFC_L1_DISABLE_LOOP_B2		0x08

/* xhfc Layer1 Flags (stored in bri_xhfc_port_t->l1_flags) */
#define HFC_L1_ACTIVATING	1
#define HFC_L1_ACTIVATED	2
#define HFC_L1_DEACTTIMER	4
#define HFC_L1_ACTTIMER		8

/* Layer1 timer Flags (stored in bri_xhfc_port_t->timer_flags) */
/* TE timer flags. bits 1-8	*/
#define T3_TIMER_ACTIVE		1
#define T4_TIMER_ACTIVE     2
#define T3_TIMER_EXPIRED	3
/* NT timer flags. bits 9-16 */
#define T1_TIMER_ACTIVE		9
#define T1_TIMER_EXPIRED	10

#define FIFO_MASK_TX	0x55555555
#define FIFO_MASK_RX	0xAAAAAAAA


#ifndef MAX_DFRAME_LEN_L1
#define MAX_DFRAME_LEN_L1 300
#endif

#define USE_F0_COUNTER		1	/* akkumulate F0 counter diff every irq */
#define TRANSP_PACKET_SIZE	0	/* minium tranparent packet size for transmittion to upper layer */

typedef struct sdla_bri_cfg_ {
	int	not_used;
	int	opermode;
	char	opermode_name[WAN_BRI_OPERMODE_LEN];
/*	int	tdmv_law;*/	/* WAN_TDMV_ALAW or WAN_TDMV_MULAW */
	char	clock_mode;
} sdla_bri_cfg_t;

#if defined(WAN_KERNEL)

#if !defined(WAN_DEBUG_FE)

/* no debugging info compiled */
/* NT/TE */
# define WRITE_BRI_REG(mod_no,reg,val)					\
	fe->write_fe_reg(	fe->card,				\
				(int)mod_no,				\
				(int)fe->bri_param.mod[mod_no].type,	\
				(int)reg,				\
				(int)val)

# define READ_BRI_REG(mod_no,reg)					\
	fe->read_fe_reg(	fe->card,				\
				(int)mod_no,				\
				(int)fe->bri_param.mod[mod_no].type,	\
				(int)0,					\
				(int)reg)

#else

#if defined(__WINDOWS__)
# pragma message("WAN_DEBUG_FE - Debugging Defined")
#else
# warning "WAN_DEBUG_FE - Debugging Defined"
#endif

/* NT/TE */
# define WRITE_BRI_REG(mod_no,reg,val)					\
	fe->write_fe_reg(	fe->card,				\
				(int)mod_no,				\
				(int)fe->bri_param.mod[mod_no].type,	\
				(int)reg, (int)val,__FILE__,(int)__LINE__)

# define READ_BRI_REG(mod_no,reg)					\
	fe->read_fe_reg(	fe->card,				\
				(int)mod_no,				\
				(int)fe->bri_param.mod[mod_no].type,	\
				(int)0,					\
				(int)reg,__FILE__,(int)__LINE__)

#endif/* #if !defined(WAN_DEBUG_FE) */

#define __READ_BRI_REG(mod_no,reg)					\
	fe->__read_fe_reg(	fe->card,				\
				(int)mod_no,				\
				(int)fe->bri_param.mod[mod_no].type,	\
				(int)0,					\
				(int)reg)


/***************************************************************/
/* port struct for each S/U port */
typedef struct {
	u_int8_t	idx;		/* port idx in hw->port[idx] */
	void		*hw;		/* back pointer to 'wp_bri_module_t' */
	u_int16_t	mode;		/* NT/TE / ST/U */
	u_int32_t	l1_flags;
	u_int32_t	timer_flags;
	
	int drx_indx;
	int dtx_indx, bytes2transmit;
	u_int8_t drxbuf[MAX_DFRAME_LEN_L1];
	u_int8_t dtxbuf[MAX_DFRAME_LEN_L1];	

	/* layer1 ISDN timers */
	/* TE timers */
	wan_timer_t	t3_timer;
	wan_timer_t	t4_timer;
	/* NT timers */
	wan_timer_t	t1_timer;

	/* chip registers */
	reg_a_su_rd_sta	su_state;
	reg_a_su_ctrl0	su_ctrl0;
	reg_a_su_ctrl1	su_ctrl1;
	reg_a_su_ctrl2	su_ctrl2;
	reg_a_st_ctrl3	st_ctrl3;
	
	/* DavidR */
	reg_a_sl_cfg	sl_cfg;/* for data direction programming on pins STIO1 and STIO2 */
	
	u_int32_t	clock_routing_state, clock_routing_counter;

	u_int32_t	l1_state;/* BRI L1 current state of the line */
 	int		nt_timer;
} bri_xhfc_port_t;


/*******************************************************************************
**			  TYPEDEF STRUCTURE
*******************************************************************************/
typedef struct {
	unsigned int	mod_no;
	/* Both ports of a module will always be in the
	   SAME mode - TE or NT. */
	int		type;
	unsigned long	events;

	/* TDM Voice applications */
	int		sig;

	/**********************************/
	int		num_ports;			/* number of S and U interfaces */
	int		max_fifo;			/* always 4 fifos per port */
	u_int8_t	max_z;				/* fifo depth -1 */
        
	u32		fifo_irqmsk;

	bri_xhfc_port_t port[BRI_MAX_PORTS_PER_CHIP]; /* 2 ports - one for each Line intercace */

	u_int32_t	irq_cnt;			/* count irqs */
	u_int32_t	f0_cnt;				/* last F0 counter value */
	u_int32_t	f0_akku;			/* akkumulated f0 counter deltas */

	/* chip registers */
	reg_r_pcm_md0	pcm_md0;
	reg_r_pcm_md1	pcm_md1;
	/**********************************/

	void	*fe;	/* pointer back to 'sdla_fe_t' */

	u32	initialization_complete;

} wp_bri_module_t;

typedef struct {
/*	u_int16_t	type; */
	unsigned int	mod_no;		/* A500 - ISDN BRI Remora */	
	unsigned char	ec_dtmf_port;	/* EC DTMF: SOUT or ROUT */
	unsigned long	ts_map;
	u_int8_t	tone;
} sdla_bri_event_t;

typedef struct sdla_bri_param {
	int		not_used;

	/* each module has 2 lines, */
	wp_bri_module_t	mod[MAX_BRI_LINES];
	/*
	Maximum of 4 remoras, 3 modules on each - maximum 12 modules.
	On each module there is 1 dual-port ISDN BRI chip. 
	On each line there are 2 B-channels and 1 D-channel.

	It means we have 48 timeslots (B-channels only!), and 32 bits are not enough.
	First 0-23 timeslots will be mapped on 'module_map[0]' and 24-47 will be mapped
	on 'module_map[1]'.
	*/
	u_int32_t		module_map[2];		/* Map of available module */
	u_int16_t		max_fe_channels;	/* Number of available modules */
	
	wan_bitmap_t	critical;

	/*
	Flag indicating if this logical 'card' (sdla_t) which represents a BRI line,
	is the 'clock reference port' for the physical A500 card.
	Only ONE 'card' may have this flag set to WANOPT_YES, all others must be
	WANOPT_NO.
	TE mode: if 'is_clock_master' is set to WANOPT_YES, clock recovered from
		    the line will be routed to ALL other BRI modules on the card.
	*/
	u_int8_t	is_clock_master;

	u_int8_t	use_512khz_recovery_clock;

} sdla_bri_param_t;

/* macros to find out type of module */
#define IS_BRI_TE_MOD(bri_param_ptr, line_no) ((bri_param_ptr)->mod[line_no].type == MOD_TYPE_TE)
#define IS_BRI_NT_MOD(bri_param_ptr, line_no) ((bri_param_ptr)->mod[line_no].type == MOD_TYPE_NT)

/* Translate FE_LINENO to physical module number divisible by BRI_MAX_PORTS_PER_CHIP. */

/*******************************************************************************
				  DEFINES AND MACROS
 *******************************************************************************/

/* Definition of AFT 32bit register at offset 0x54. */
typedef struct bri_reg{
	u8	data;		/* lsb */
	u8	contrl;

	u8	reserv1;
	u8	mod_addr:2;	/* 00, 01, 10 - adresses modules,
				   code 11 - address of status register per-remora. */

	u8	remora_addr:4;	/* Select Remora. Maximum is 4 (from 0 to 3) for BRI */
	u8	reset:1;
	u8	start:1;	/* msb */
}bri_reg_t;

#define RM_BRI_STATUS_READ	0x3

#define CPLD_USE_512KHZ_RECOVERY_CLOCK_BIT (1)
#define READ_BIT	(1 << 7)
#define ADDR_BIT	(1 << 6)


/*******************************************************************************
**			  FUNCTION PROTOTYPES
*******************************************************************************/
extern int	wp_bri_iface_init(void*);

#endif/* WAN_KERNEL */

/***************************************************************/

#endif /* __SDLA_BRI_H */
