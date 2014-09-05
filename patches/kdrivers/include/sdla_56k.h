/*****************************************************************************
 * sdla_56k.h	Sangoma 56K configuration definitions.
 *
 * Author:      Nenad Corbic
 *
 * Copyright:	(c) 1995-2001 Sangoma Technologies Inc.
 *
 *		This program is free software; you can redistribute it and/or
 *		modify it under the terms of the GNU General Public License
 *		as published by the Free Software Foundation; either version
 *		2 of the License, or (at your option) any later version.

 * ============================================================================
 * July 20, 2001	Nenad Corbic	Initial version.
 ****************************************************************************
*/

#ifndef _SDLA_56K_H_
#define _SDLA_56K_H_

/*******************************************************************************
			  DEFINES AND MACROS
 ******************************************************************************/

#define IS_56K_CARD(card)	IS_56K_FEMEDIA(&(card)->fe)

#define RRA(alarm) ((alarm>>8)&0xFF)
#define RRC(alarm) ((alarm)&0xFF)
			
#define INS_ALARM_56K(alarm) ((RRA(alarm)&0x0F) > 8) ? "GREEN" : "RED"
#define DMI_ALARM_56K(alarm) (RRC(alarm)&0x80) ? "RED" : "OFF"
#define ZCS_ALARM_56K(alarm) (RRC(alarm)&0x40) ? "RED" : "OFF"
#define CMI_ALARM_56K(alarm) (RRC(alarm)&0x20) ? "RED" : "OFF"
#define OOS_ALARM_56K(alarm) (RRC(alarm)&0x10) ? "RED" : "OFF"
#define OOF_ALARM_56K(alarm) (RRC(alarm)&0x08) ? "RED" : "OFF"
#define DLP_ALARM_56K(alarm) (RRC(alarm)&0x04) ? "RED" : "OFF"
#define UMC_ALARM_56K(alarm) (RRC(alarm)&0x02) ? "RED" : "OFF"
#define RLOS_ALARM_56K(alarm) (RRA(alarm)&0x80) ? "RED" : "OFF"


/* registers on 56K card */
#define REG_DEV_CTRL		0x00
#define REG_TX_CTRL		0x01
#define REG_RX_CTRL		0x02
#define REG_EIA_SEL		0x05
#define REG_EIA_TX_DATA		0x07
#define REG_INT_EN_STAT		0x08
#define REG_EIA_CTRL		0x09
#define REG_DEV_STAT		0x0A
#define REG_RX_SLICER_LVL	0x0B
#define REG_RX_CODES		0x0C
#define REG_RX_INVALID_BPV	0x0D

/* setting for each register */ 
#define BIT_DEV_CTRL_DDS_PRI	0x00
#define BIT_DEV_CTRL_SCT_E_OUT	0x10
#define BIT_DEV_CTRL_XTALI_INT	0x40

#define BIT_INT_EN_STAT_ACTIVE	0x01
#define BIT_INT_EN_STAT_RX_CODE	0x20
#define BIT_INT_EN_STAT_IDEL	0x40

#define BIT_EIA_CTRL_RTS_ACTIVE	0x01
#define BIT_EIA_CTRL_DTR_ACTIVE 0x02
#define	BIT_EIA_CTRL_DTE_ENABLE	0x04
#define BIT_DEV_STAT_IL_44_dB	0x08
#define BIT_DEV_STAT_RLOS	0x80

#define BIT_RX_CTRL_DSU_LOOP	0x80
#define BIT_RX_CTRL_CSU_LOOP	0x20

#define BIT_RX_CODES_UNMTCH	0x01
#define BIT_RX_CODES_UMC	0x02
#define BIT_RX_CODES_DLP	0x04
#define BIT_RX_CODES_OOF	0x08
#define BIT_RX_CODES_OOS	0x10
#define BIT_RX_CODES_CMI	0x20
#define BIT_RX_CODES_ZSC	0x40
#define BIT_RX_CODES_DMI	0x80


/*******************************************************************************
			  FUNCTION PROTOTYPES
 ******************************************************************************/

#ifdef WAN_KERNEL

typedef struct {
	unsigned char	RLOS_56k;
	unsigned char	RR8_reg_56k;
	unsigned char	RRA_reg_56k;
	unsigned char	RRC_reg_56k;
	unsigned char	prev_RRC_reg_56k;
	unsigned char	delta_RRC_reg_56k;
} sdla_56k_param_t;

extern int sdla_56k_default_cfg(void* arg1, void* p56k_cfg);
extern int sdla_56k_iface_init(void* pfe, void *p_fe_iface);

#endif /* WAN_KERNEL */

#endif
