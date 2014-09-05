/*
 * Copyright (c) 2001
 *	Alex Feldman <al.feldman@sangoma.com>.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by Alex Feldman.
 * 4. Neither the name of the author nor the names of any co-contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY Alex Feldman AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL Alex Feldman OR THE VOICES IN HIS HEAD
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 *
 *	$Id: sdla_te1.h,v 1.72 2008-04-02 20:52:09 sangoma Exp $
 */

/*****************************************************************************
 * sdla_te1.h	Sangoma TE1 configuration definitions.
 *
 * Author:      Alex Feldman
 *
 * ============================================================================
 * Aprl 30, 2001	Alex Feldman	Initial version.
 ****************************************************************************
*/
#ifndef	_SDLA_TE1_H
#    define	_SDLA_TE1_H

#ifdef SDLA_TE1
# define WP_EXTERN
#else
# define WP_EXTERN extern
#endif

/************************************************************************
 *			  DEFINES AND MACROS				*
 ***********************************************************************/

/*
*************************************************************************
*			  DEFINES AND MACROS				*
*************************************************************************
*/

#define WAN_TE_CHIP_PMC		0x00
#define WAN_TE_CHIP_DM		0x01

#define NUM_OF_T1_CHANNELS	24
#define NUM_OF_E1_TIMESLOTS	31
#define NUM_OF_E1_CHANNELS	32
#define ENABLE_ALL_CHANNELS	0xFFFFFFFF

#define E1_FRAMING_TIMESLOT	0
#define E1_SIGNALING_TIMESLOT	16

#define WAN_TE_SIG_POLL		0x01
#define WAN_TE_SIG_INTR		0x02

/* Framer Alarm bit mask */
#define WAN_TE_ALARM_LIU		0x80000000

#define WAN_TE_ALARM_FRAMER_MASK    	0x0000FFFF
#define WAN_TE_ALARM_LIU_MASK	    	0x00F00000
#define WAN_TE_BIT_ALARM_ALOS		 0x00000001
#define WAN_TE_BIT_ALARM_LOS		 0x00000002
#define WAN_TE_BIT_ALARM_ALTLOS		 0x00000004
#define WAN_TE_BIT_ALARM_OOF		 0x00000008
#define WAN_TE_BIT_ALARM_RED		 0x00000010
#define WAN_TE_BIT_ALARM_AIS		 0x00000020
#define WAN_TE_BIT_ALARM_OOSMF		 0x00000040
#define WAN_TE_BIT_ALARM_OOCMF		 0x00000080
#define WAN_TE_BIT_ALARM_OOOF		 0x00000100
#define WAN_TE_BIT_ALARM_RAI		 0x00000200
#define WAN_TE_BIT_ALARM_YEL		 0x00000400
#define WAN_TE_BIT_ALARM_LOF		 0x00000800
#define WAN_TE_BIT_LOOPUP_CODE		 0x00002000	
#define WAN_TE_BIT_LOOPDOWN_CODE	 0x00004000	
#define WAN_TE_BIT_ALARM_LIU		 0x00100000
#define WAN_TE_BIT_ALARM_LIU_SC		 0x00100000
#define WAN_TE_BIT_ALARM_LIU_OC		 0x00200000
#define WAN_TE_BIT_ALARM_LIU_LOS	 0x00400000

#define IS_TE_ALARM(alarm, mask)	(alarm & mask)
#define IS_TE_ALARM_ALOS(alarm)		IS_TE_ALARM(alarm, WAN_TE_BIT_ALARM_ALOS)
#define IS_TE_ALARM_LOS(alarm)		IS_TE_ALARM(alarm, WAN_TE_BIT_ALARM_LOS)
#define IS_TE_ALARM_OOF(alarm)		IS_TE_ALARM(alarm, WAN_TE_BIT_ALARM_OOF)
#define IS_TE_ALARM_LOF(alarm)		IS_TE_ALARM(alarm, WAN_TE_BIT_ALARM_LOF)
#define IS_TE_ALARM_RED(alarm)		IS_TE_ALARM(alarm, WAN_TE_BIT_ALARM_RED)
#define IS_TE_ALARM_AIS(alarm)		IS_TE_ALARM(alarm, WAN_TE_BIT_ALARM_AIS)
#define IS_TE_ALARM_OOSMF(alarm)	IS_TE_ALARM(alarm, WAN_TE_BIT_ALARM_OOSMF)
#define IS_TE_ALARM_OOCMF(alarm)	IS_TE_ALARM(alarm, WAN_TE_BIT_ALARM_OOCMF)
#define IS_TE_ALARM_OOOF(alarm)		IS_TE_ALARM(alarm, WAN_TE_BIT_ALARM_OOOF)
#define IS_TE_ALARM_RAI(alarm)		IS_TE_ALARM(alarm, WAN_TE_BIT_ALARM_RAI)
#define IS_TE_ALARM_YEL(alarm)		IS_TE_ALARM(alarm, WAN_TE_BIT_ALARM_YEL)

/* Needed for backward compatibility */
#ifndef IS_TE_ALOS_ALARM
#define IS_TE_ALOS_ALARM IS_TE_ALARM_ALOS
#endif

#ifndef IS_TE_LOS_ALARM
#define IS_TE_LOS_ALARM IS_TE_ALARM_LOS
#endif

#ifndef IS_TE_RED_ALARM
#define IS_TE_RED_ALARM IS_TE_ALARM_RED
#endif

#ifndef IS_TE_AIS_ALARM
#define IS_TE_AIS_ALARM IS_TE_ALARM_AIS
#endif


#ifndef IS_TE_RAI_ALARM
#define IS_TE_RAI_ALARM IS_TE_ALARM_RAI
#endif


#ifndef IS_TE_YEL_ALARM
#define IS_TE_YEL_ALARM IS_TE_ALARM_YEL
#endif

#ifndef IS_TE_OOF_ALARM
#define IS_TE_OOF_ALARM IS_TE_ALARM_OOF
#endif

/* Performance monitor counters bit mask */
#define WAN_TE_BIT_PMON_LCV		0x01	/* line code violation counter */
#define WAN_TE_BIT_PMON_BEE		0x02	/* bit errror event (T1) */
#define WAN_TE_BIT_PMON_OOF		0x04	/* frame out of sync counter */
#define WAN_TE_BIT_PMON_FEB		0x08	/* far end block counter */
#define WAN_TE_BIT_PMON_CRC4		0x10	/* crc4 error counter (E1) */
#define WAN_TE_BIT_PMON_FER		0x20	/* framing bit error (T1-pmc) */
#define WAN_TE_BIT_PMON_FAS		0x40	/* Frame Alginment signal (E1) */

/* T1/E1 statistics bit mask */
#define WAN_TE_STATS_BIT_ALARM		0x0001
#define WAN_TE_STATS_BIT_RXLEVEL	0x0002

/* For T1 only */
#define WAN_T1_LBO_NONE		0x00
#define WAN_T1_LBO_0_DB		0x01
#define WAN_T1_LBO_75_DB	0x02
#define WAN_T1_LBO_15_DB	0x03
#define WAN_T1_LBO_225_DB	0x04
#define WAN_T1_0_110		0x05
#define WAN_T1_110_220		0x06
#define WAN_T1_220_330		0x07
#define WAN_T1_330_440		0x08
#define WAN_T1_440_550		0x09
#define WAN_T1_550_660		0x0A
/* For E1 only */
#define WAN_E1_120		0x0B
#define WAN_E1_75		0x0C
/* T1/E1 8 ports */
#define WAN_T1_0_133		0x0D
#define WAN_T1_133_266		0x0E
#define WAN_T1_266_399		0x0F
#define WAN_T1_399_533		0x10
#define WAN_T1_533_655		0x11

/* T1/E1: Recever */
#define WAN_TE1_RX_SLEVEL_NONE		0
#define WAN_TE1_RX_SLEVEL_43_DB		430	/* 43 dB   E1, RMONEN=0 */
#define WAN_TE1_RX_SLEVEL_36_DB		360	/* 36 dB   T1, RMONEN=0 */
#define WAN_TE1_RX_SLEVEL_30_DB		300 /* 30 dB   RMONEN=0 | 1 */
#define WAN_TE1_RX_SLEVEL_225_DB	225	/* 22.5 dB RMONEN=1 */
#define WAN_TE1_RX_SLEVEL_18_DB		180	/* 18 dB   RMONEN=0 */
#define WAN_TE1_RX_SLEVEL_175_DB	175	/* 17.5 dB RMONEN=1 */
#define WAN_TE1_RX_SLEVEL_12_DB		120	/* 12 dB   RMONEN=0 | 1 */
#define WAN_TE1_RX_SLEVEL_DECODE(slevel)						\
		((slevel) == WAN_TE1_RX_SLEVEL_43_DB) ? "43dB" :		\
		((slevel) == WAN_TE1_RX_SLEVEL_36_DB) ? "36dB" :		\
		((slevel) == WAN_TE1_RX_SLEVEL_30_DB) ? "30dB" :		\
		((slevel) == WAN_TE1_RX_SLEVEL_225_DB) ? "22.5dB" :		\
		((slevel) == WAN_TE1_RX_SLEVEL_18_DB) ? "18dB" :		\
		((slevel) == WAN_TE1_RX_SLEVEL_175_DB) ? "17.5dB" :		\
		((slevel) == WAN_TE1_RX_SLEVEL_12_DB) ? "12dB" : "0dB"

/* For T1 only (long or short haul) */
#define WAN_T1_LONG_HAUL	0x01
#define WAN_T1_SHORT_HAUL	0x02

/* Line loopback modes */
#define WAN_TE1_LB_NONE			0x00
#define WAN_TE1_LINELB_MODE		0x01
#define WAN_TE1_PAYLB_MODE		0x02
#define WAN_TE1_DDLB_MODE		0x03
#define WAN_TE1_TX_LINELB_MODE	0x04
#define WAN_TE1_LIU_ALB_MODE	0x05
#define WAN_TE1_LIU_LLB_MODE	0x06
#define WAN_TE1_LIU_RLB_MODE	0x07
#define WAN_TE1_LIU_DLB_MODE	0x08
#define WAN_TE1_TX_PAYLB_MODE	0x09
#define WAN_TE1_PCLB_MODE		0x0A

#define WAN_TE1_LB_MODE_DECODE(mode)						\
		((mode) == WAN_TE1_LINELB_MODE) ? "Line/Remote Loopback" :	\
		((mode) == WAN_TE1_PAYLB_MODE) ? "Payload Loopback" :		\
		((mode) == WAN_TE1_DDLB_MODE) ? "Diagnostic Digital Loopback" :	\
		((mode) == WAN_TE1_TX_LINELB_MODE) ? "TX Line Loopback" :	\
		((mode) == WAN_TE1_LIU_ALB_MODE) ? "Analog LIU Loopback" :	\
		((mode) == WAN_TE1_LIU_LLB_MODE) ? "Local LIU Loopback" :	\
		((mode) == WAN_TE1_LIU_RLB_MODE) ? "Remote LIU Loopback" :	\
		((mode) == WAN_TE1_LIU_DLB_MODE) ? "Dual LIU Loopback" :	\
		((mode) == WAN_TE1_TX_PAYLB_MODE) ? "TX Payload Loopback" :	\
		((mode) == WAN_TE1_PCLB_MODE) ? "Per-channel Loopback" :	\
						"Unknown Loopback"

/* Line loopback activate/deactive modes */
#define WAN_TE1_LB_ENABLE	0x01
#define WAN_TE1_LB_DISABLE	0x02
#define WAN_TE1_LB_ACTION_DECODE(action)			\
		((action) == WAN_TE1_LB_ENABLE) ? "Enable" :	\
		((action) == WAN_TE1_LB_DISABLE) ? "Disable" :	\
						"Unknown"

/* T1/E1 front end Master clock source */
#define WAN_TE1_REFCLK_OSC	0x00
#define WAN_TE1_REFCLK_LINE1	0x01
#define WAN_TE1_REFCLK_LINE2	0x02
#define WAN_TE1_REFCLK_LINE3	0x03
#define WAN_TE1_REFCLK_LINE4	0x04

/* Loopback commands (T1.107-1995 p.44) */
#define WAN_T1_FDL_MSG_TIME		250	/* 250 us */
#define WAN_T1_ESF_LINELB_TX_CNT	10	
#define WAN_T1_D4_LINELB_TX_CNT		5	

#define RBOC_CODE_YEL		0x00
#define LINELB_ACTIVATE_CODE	0x07
#define LINELB_DEACTIVATE_CODE	0x1C
#define PAYLB_ACTIVATE_CODE	0x0A
#define PAYLB_DEACTIVATE_CODE	0x19
#define UNIVLB_DEACTIVATE_CODE	0x12

#define WAN_TE1_BOC_LB_CODE_DECODE(boc)						\
		((boc) == LINELB_ACTIVATE_CODE) ? "LINELB Activate"	:	\
		((boc) == LINELB_DEACTIVATE_CODE) ? "LINELB Deactivate"	:	\
		((boc) == PAYLB_ACTIVATE_CODE) ? "PAYLB Activate"	:	\
		((boc) == PAYLB_DEACTIVATE_CODE) ? "PAYLB Deactivate"	:	\
		((boc) == UNIVLB_DEACTIVATE_CODE) ? "Universal LB Deactivate"	:	\
							"Unsupported BOC"
 
/* Interrupt polling delay */
#define POLLING_TE1_TIMER		1	/* 1 sec */
#define WAN_T1_ALARM_THRESHOLD_LOF_ON	(3)	/* 2-3 sec */
#define WAN_T1_ALARM_THRESHOLD_LOF_OFF	(10)	/* 10 sec */
#define WAN_T1_ALARM_THRESHOLD_AIS_ON	(3)   // must be 2.5s
#define WAN_T1_ALARM_THRESHOLD_AIS_OFF	(10)	// must be 10s
#define WAN_T1_ALARM_THRESHOLD_LOS_ON	(3)   // must be 2.5s
#define WAN_T1_ALARM_THRESHOLD_LOS_OFF	(10)    // must be 10s

/* TE1 critical flag */
#define TE_TIMER_RUNNING 		0x01
#define TE_TIMER_KILL 			0x02
#define LINELB_WAITING			0x03
#define LINELB_CODE_BIT			0x04
#define LINELB_CHANNEL_BIT		0x05
#define TE_CONFIGURED			0x06
#define TE_TIMER_EVENT_PENDING 		0x07
#define TE_TIMER_EVENT_INPROGRESS	0x08
#define TE_AIS_TX_STARTUP		0x09				

/* TE1 sw irq types */
enum {
	WAN_TE1_SWIRQ_TYPE_NONE 	= 0,

	WAN_TE1_SWIRQ_TYPE_LINK,
	WAN_TE1_SWIRQ_TYPE_ALARM_AIS,
	WAN_TE1_SWIRQ_TYPE_ALARM_LOS,
	WAN_TE1_SWIRQ_TYPE_ALARM_LOF,

	WAN_TE1_SWIRQ_MAX
};
#define WAN_TE1_SWIRQ_TYPE_DECODE(type)						\
		((type) == WAN_TE1_SWIRQ_TYPE_LINK) 	 ? "T1/E1 Link" :	\
		((type) == WAN_TE1_SWIRQ_TYPE_ALARM_AIS) ? "T1/E1 Alarm (AIS)" :\
		((type) == WAN_TE1_SWIRQ_TYPE_ALARM_LOS) ? "T1/E1 Alarm (LOS)" :\
		((type) == WAN_TE1_SWIRQ_TYPE_ALARM_LOF) ? "T1/E1 Alarm (LOF)" :\
								"Unknown"

enum {
	WAN_TE1_SWIRQ_SUBTYPE_NONE	= 0,

	WAN_TE1_SWIRQ_SUBTYPE_LINKDOWN,
	WAN_TE1_SWIRQ_SUBTYPE_LINKREADY,
	WAN_TE1_SWIRQ_SUBTYPE_LINKCRIT,
	WAN_TE1_SWIRQ_SUBTYPE_LINKUP,

	WAN_TE1_SWIRQ_SUBTYPE_ALARM_ON,
	WAN_TE1_SWIRQ_SUBTYPE_ALARM_OFF
};
#define WAN_TE1_SWIRQ_SUBTYPE_DECODE(subtype)						\
		((subtype) == WAN_TE1_SWIRQ_SUBTYPE_LINKDOWN) 	 ? "Down" :		\
		((subtype) == WAN_TE1_SWIRQ_SUBTYPE_LINKREADY) 	 ? "Ready" :		\
		((subtype) == WAN_TE1_SWIRQ_SUBTYPE_LINKCRIT) 	 ? "Crit" :		\
		((subtype) == WAN_TE1_SWIRQ_SUBTYPE_LINKUP) 	 ? "UP" :		\
		((subtype) == WAN_TE1_SWIRQ_SUBTYPE_ALARM_ON) 	 ? "activating" :	\
		((subtype) == WAN_TE1_SWIRQ_SUBTYPE_ALARM_OFF) 	 ? "deactivating" :	\
								"Unknown"


/* TE1 timer flags (polling) */
#define TE_LINELB_TIMER		0x01
#define TE_LINKDOWN_TIMER	0x02
#define TE_SET_INTR		0x03
#define TE_RBS_READ		0x04
#define TE_LINKUP_TIMER		0x05
#define TE_SET_RBS		0x06
#define TE_UPDATE_PMON		0x07
#define TE_SET_LB_MODE		0x08
#define TE_RBS_ENABLE		0x09
#define TE_RBS_DISABLE		0x0A
#define TE_POLL_CONFIG		0x0B
#define TE_POLL_READ		0x0C
#define TE_POLL_WRITE		0x0D
#define TE_POLL_CONFIG_VERIFY	0x0E
#define TE_LINKCRIT_TIMER	0x0F
#define WAN_TE_POLL_LINKREADY	0x10
#define WAN_TE_POLL_BERT	0x11
#define WAN_TE_POLL_ALARM_PENDING	0x12

/* TE1 T1/E1 interrupt setting delay */
#define INTR_TE1_TIMER		150	/* 50 ms */

/* T1/E1 RBS flags (bit-map) */
#define WAN_TE_RBS_NONE		0x00
#define WAN_TE_RBS_UPDATE	0x01
#define WAN_TE_RBS_REPORT	0x02


#define IS_T1_CARD(card)	IS_T1_FEMEDIA(&(card)->fe)
#define IS_E1_CARD(card)	IS_E1_FEMEDIA(&(card)->fe)
#define IS_TE1_CARD(card)	IS_TE1_FEMEDIA(&(card)->fe)

#define FE_LBO(fe_cfg)			(fe_cfg)->cfg.te_cfg.lbo	
#define FE_CLK(fe_cfg)			(fe_cfg)->cfg.te_cfg.te_clock	
#define FE_REFCLK(fe_cfg)		(fe_cfg)->cfg.te_cfg.te_ref_clock	
#define FE_HIMPEDANCE_MODE(fe_cfg)	(fe_cfg)->cfg.te_cfg.high_impedance_mode
#define FE_ACTIVE_CH(fe_cfg)		(fe_cfg)->cfg.te_cfg.active_ch
#define FE_SIG_MODE(fe_cfg)		(fe_cfg)->cfg.te_cfg.sig_mode
#define FE_RX_SLEVEL(fe_cfg)		(fe_cfg)->cfg.te_cfg.rx_slevel

#define GET_TE_START_CHANNEL(fe)			\
	(IS_T1_FEMEDIA(fe) ? 1 :			\
	 IS_E1_FEMEDIA(fe) ? 0 :0)

#define GET_TE_CHANNEL_RANGE(fe)				\
	(IS_T1_FEMEDIA(fe) ? NUM_OF_T1_CHANNELS :\
	 IS_E1_FEMEDIA(fe) ? NUM_OF_E1_CHANNELS :0)
			

#define WAN_TE_PRN_ALARM(alarm, bit)	((alarm) & (bit)) ? "ON" : "OFF"

#define WAN_TE_PRN_ALARM_ALOS(alarm)	WAN_TE_PRN_ALARM(alarm, WAN_TE_BIT_ALARM_ALOS)
#define WAN_TE_PRN_ALARM_LOS(alarm)	WAN_TE_PRN_ALARM(alarm, WAN_TE_BIT_ALARM_LOS)
#define WAN_TE_PRN_ALARM_OOF(alarm)	WAN_TE_PRN_ALARM(alarm, WAN_TE_BIT_ALARM_OOF)
#define WAN_TE_PRN_ALARM_LOF(alarm)	WAN_TE_PRN_ALARM(alarm, WAN_TE_BIT_ALARM_LOF)
#define WAN_TE_PRN_ALARM_RED(alarm)	WAN_TE_PRN_ALARM(alarm, WAN_TE_BIT_ALARM_RED)
#define WAN_TE_PRN_ALARM_AIS(alarm)	WAN_TE_PRN_ALARM(alarm, WAN_TE_BIT_ALARM_AIS)
#define WAN_TE_PRN_ALARM_OOSMF(alarm)	WAN_TE_PRN_ALARM(alarm, WAN_TE_BIT_ALARM_OOSMF)
#define WAN_TE_PRN_ALARM_OOCMF(alarm)	WAN_TE_PRN_ALARM(alarm, WAN_TE_BIT_ALARM_OOCMF)
#define WAN_TE_PRN_ALARM_OOOF(alarm)	WAN_TE_PRN_ALARM(alarm, WAN_TE_BIT_ALARM_OOOF)
#define WAN_TE_PRN_ALARM_RAI(alarm)	WAN_TE_PRN_ALARM(alarm, WAN_TE_BIT_ALARM_RAI)
#define WAN_TE_PRN_ALARM_YEL(alarm)	WAN_TE_PRN_ALARM(alarm, WAN_TE_BIT_ALARM_YEL)

#define WAN_TE_PRN_ALARM_LIU_SC(alarm)	WAN_TE_PRN_ALARM(alarm, WAN_TE_BIT_ALARM_LIU_SC)
#define WAN_TE_PRN_ALARM_LIU_OC(alarm)	WAN_TE_PRN_ALARM(alarm, WAN_TE_BIT_ALARM_LIU_OC)
#define WAN_TE_PRN_ALARM_LIU_LOS(alarm)	WAN_TE_PRN_ALARM(alarm, WAN_TE_BIT_ALARM_LIU_LOS)

#define TECLK_DECODE(fe_cfg)			\
	(FE_CLK(fe_cfg) == WAN_NORMAL_CLK) ? "Normal" :	\
	(FE_CLK(fe_cfg) == WAN_MASTER_CLK) ? "Master" : "Unknown"

#define LBO_DECODE(fe_cfg)				\
	(FE_LBO(fe_cfg) == WAN_T1_LBO_0_DB)	? "0db" :	\
	(FE_LBO(fe_cfg) == WAN_T1_LBO_75_DB)	? "7.5db" :	\
	(FE_LBO(fe_cfg) == WAN_T1_LBO_15_DB)	? "15dB" :	\
	(FE_LBO(fe_cfg) == WAN_T1_LBO_225_DB)	? "22.5dB" :	\
	(FE_LBO(fe_cfg) == WAN_T1_0_110)	? "0-110ft" :	\
	(FE_LBO(fe_cfg) == WAN_T1_110_220)	? "110-220ft" :	\
	(FE_LBO(fe_cfg) == WAN_T1_220_330)	? "220-330ft" :	\
	(FE_LBO(fe_cfg) == WAN_T1_330_440)	? "330-440ft" :	\
	(FE_LBO(fe_cfg) == WAN_T1_440_550)	? "440-550ft" :	\
	(FE_LBO(fe_cfg) == WAN_T1_550_660)	? "550-660ft" : \
	(FE_LBO(fe_cfg) == WAN_T1_0_133)	? "0-133ft"   :	\
	(FE_LBO(fe_cfg) == WAN_T1_133_266)	? "133-266ft" :	\
	(FE_LBO(fe_cfg) == WAN_T1_266_399)	? "266-399ft" :	\
	(FE_LBO(fe_cfg) == WAN_T1_399_533)	? "399-533ft" :	\
	(FE_LBO(fe_cfg) == WAN_T1_533_655)	? "5330-599ft":	\
	(FE_LBO(fe_cfg) == WAN_E1_120)		? "120OH"     :	\
	(FE_LBO(fe_cfg) == WAN_E1_75)		? "75OH"      :	\
							"Unknown"

/* E1 signalling insertion mode */
#define WAN_TE1_SIG_NONE	0x00	/* default */
#define WAN_TE1_SIG_CCS		0x01	/* E1 CCS - default */
#define WAN_TE1_SIG_CAS		0x02	/* E1 CAS */
#define TE1SIG_DECODE(fe_cfg)					\
		((FE_SIG_MODE(fe_cfg)) == WAN_TE1_SIG_CCS) ? "CCS" :	\
		((FE_SIG_MODE(fe_cfg)) == WAN_TE1_SIG_CAS) ? "CAS" :	\
						"Unknown"

/* Front-End UDP command */

#define WAN_FE_SET_LB_MODE	WAN_FE_LB_MODE

#define WAN_FE_GET_STAT		(WAN_FE_UDP_CMD_START + 0)
#define WAN_FE_LB_MODE		(WAN_FE_UDP_CMD_START + 1)
#define WAN_FE_FLUSH_PMON	(WAN_FE_UDP_CMD_START + 2)
#define WAN_FE_GET_CFG		(WAN_FE_UDP_CMD_START + 3)
#define WAN_FE_SET_DEBUG_MODE	(WAN_FE_UDP_CMD_START + 4)
#define WAN_FE_TX_MODE		(WAN_FE_UDP_CMD_START + 5)
#define WAN_FE_BERT_MODE	(WAN_FE_UDP_CMD_START + 6)

/* FE interrupt types bit-map */
#define WAN_TE_INTR_NONE	0x00
#define WAN_TE_INTR_GLOBAL	0x01
#define WAN_TE_INTR_BASIC	0x02
#define WAN_TE_INTR_SIGNALLING	0x04
#define WAN_TE_INTR_FXS_DTMF	0x08
#define WAN_TE_INTR_PMON	0x10


/*----------------------------------------------------------------------------
 * T1/E1 configuration structures.
 */
typedef struct sdla_te_cfg {
	u_int8_t	lbo;
	u_int8_t	te_clock;
	u_int32_t	active_ch;
	u_int32_t	te_rbs_ch;
	u_int8_t	high_impedance_mode;
	int 		rx_slevel;	/* only for high_impedance_mode=YES */
	u_int8_t	te_ref_clock;
	u_int8_t	sig_mode;
	u_int8_t	ignore_yel_alarm;
	u_int8_t	ais_maintenance;
	u_int8_t	ais_auto_on_los;
} sdla_te_cfg_t;

/* Performamce monitor counters */
typedef struct {
	u_int16_t	mask;
	u_int32_t	lcv_errors;	/* Line code violation (T1/E1) */
	u_int16_t	lcv_diff;
	u_int32_t	bee_errors;	/* Bit errors (T1) */
	u_int16_t	bee_diff;
	u_int32_t	oof_errors;	/* Frame out of sync (T1) */
	u_int16_t	oof_diff;
	u_int32_t	crc4_errors;	/* CRC4 errors (E1) */
	u_int16_t	crc4_diff;
	u_int32_t	fas_errors;	/* Frame Aligment Signal (E1)*/
	u_int16_t	fas_diff;
	u_int32_t	feb_errors;	/* Far End Block errors (E1) */
	u_int16_t	feb_diff;
	u_int32_t	fer_errors;	/* Framing bit errors (T1) */
	u_int16_t	fer_diff;
	u_int32_t	sync_errors;
} sdla_te_pmon_t;

#define WAN_PMON_SYNC_ERROR(card) do { \
									sdla_te_pmon_t	*pmon; \
									pmon = &card->fe.fe_stats.te_pmon; \
									pmon->sync_errors++; \
								  } while (0);

#define WAN_TE_RXLEVEL_LEN	20
typedef struct {
	u_int16_t	mask;
	sdla_te_pmon_t	pmon;
	char		rxlevel[WAN_TE_RXLEVEL_LEN];
} sdla_te_stats_t;

#define WAN_TE_BERT_CMD_NONE	0x00
#define WAN_TE_BERT_CMD_STOP	0x01
#define WAN_TE_BERT_CMD_START	0x02
#define WAN_TE_BERT_CMD_STATUS	0x03
#define WAN_TE_BERT_CMD_EIB	0x04
#define WAN_TE_BERT_CMD_RESET	0x05
#define WAN_TE_BERT_CMD_RUNNING	0x06
#define WAN_TE_BERT_CMD_DECODE(cmd)				\
	((cmd) == WAN_TE_BERT_CMD_STOP) ? "Stop" :	\
	((cmd) == WAN_TE_BERT_CMD_START) ? "Start" :		\
	((cmd) == WAN_TE_BERT_CMD_STATUS) ? "Status" :		\
	((cmd) == WAN_TE_BERT_CMD_EIB) ? "Error Insert Bit" : 	\
	((cmd) == WAN_TE_BERT_CMD_RESET) ? "Reset Status" : 	\
	((cmd) == WAN_TE_BERT_CMD_RUNNING) ? "Running" : 	\
						"Unknown"

/* BERT: Pattern type */
#define WAN_TE_BERT_PATTERN_PSEUDORANDOM_2E7	0x01
#define WAN_TE_BERT_PATTERN_PSEUDORANDOM_2E11	0x02
#define WAN_TE_BERT_PATTERN_PSEUDORANDOM_2E15	0x03
#define WAN_TE_BERT_PATTERN_PSEUDORANDOM_QRSS	0x04
#define WAN_TE_BERT_PATTERN_REPETITIVE		0x05
#define WAN_TE_BERT_PATTERN_WORD		0x06
#define WAN_TE_BERT_PATTERN_DALY		0x07
#define WAN_TE_BERT_PATTERN_PSEUDORANDOM_2E9	0x08
#define WAN_TE_BERT_PATTERN_DECODE(type)						\
	((type) == WAN_TE_BERT_PATTERN_PSEUDORANDOM_2E7) ? "PseudoRandom 2E7-1" :	\
	((type) == WAN_TE_BERT_PATTERN_PSEUDORANDOM_2E11) ? "PseudoRandom 2E11-1" :	\
	((type) == WAN_TE_BERT_PATTERN_PSEUDORANDOM_2E15) ? "PseudoRandom 2E15-1" :	\
	((type) == WAN_TE_BERT_PATTERN_PSEUDORANDOM_QRSS) ? "PseudoRandom QRSS" :	\
	((type) == WAN_TE_BERT_PATTERN_REPETITIVE) ? "Repetitive Pattern" :		\
	((type) == WAN_TE_BERT_PATTERN_WORD) ? "Alternating Word Pattern" :		\
	((type) == WAN_TE_BERT_PATTERN_DALY) ? "Daly Pattern" :			\
	((type) == WAN_TE_BERT_PATTERN_PSEUDORANDOM_2E9) ? "PseudoRandom 2E-9-1" : "Unknown"

#define WAN_TE_BERT_LOOPBACK_NONE		0x00
#define WAN_TE_BERT_LOOPBACK_PAYLOAD		0x01
#define WAN_TE_BERT_LOOPBACK_LINE		0x02
#define WAN_TE_BERT_LOOPBACK_DECODE(type)				\
	((type) == WAN_TE_BERT_LOOPBACK_NONE) ? "Loopback (None)" :	\
	((type) == WAN_TE_BERT_LOOPBACK_PAYLOAD) ? "Payload Loopback" :	\
	((type) == WAN_TE_BERT_LOOPBACK_LINE) ? "Line Loopback" :	\
						"Unknown"

/* BERT: Errot Insert Bits */
#define WAN_TE_BERT_EIB_NONE	0x00
#define WAN_TE_BERT_EIB_SINGLE	0x01
#define WAN_TE_BERT_EIB1	0x02
#define WAN_TE_BERT_EIB2	0x03
#define WAN_TE_BERT_EIB3	0x04
#define WAN_TE_BERT_EIB4	0x05
#define WAN_TE_BERT_EIB5	0x06
#define WAN_TE_BERT_EIB6	0x07
#define WAN_TE_BERT_EIB7	0x08
#define WAN_TE_BERT_EIB_DECODE(eib)					\
	((eib) == WAN_TE_BERT_EIB_NONE) ? "No error insertion" :	\
	((eib) == WAN_TE_BERT_EIB_SINGLE) ? "Single Bit Error Insert" :	\
	((eib) == WAN_TE_BERT_EIB1) ? "EIB 10E-1" :			\
	((eib) == WAN_TE_BERT_EIB2) ? "EIB 10E-2" :			\
	((eib) == WAN_TE_BERT_EIB3) ? "EIB 10E-3" :			\
	((eib) == WAN_TE_BERT_EIB4) ? "EIB 10E-4" :			\
	((eib) == WAN_TE_BERT_EIB5) ? "EIB 10E-5" :			\
	((eib) == WAN_TE_BERT_EIB6) ? "EIB 10E-6" :			\
	((eib) == WAN_TE_BERT_EIB7) ? "EIB 10E-7" : 			\
					"Unknown"

#define WAN_TE_BERT_STATUS_AUTO		0x00
#define WAN_TE_BERT_STATUS_MANUAL	0x01
#define WAN_TE_BERT_STATUS_RUNNING	0x02
#define WAN_TE_BERT_STATUS_STOPPED	0x03

#define WAN_TE_BERT_FLAG_VERBOSE	1
#define WAN_TE_BERT_FLAG_READY		2
#define WAN_TE_BERT_FLAG_INLOCK		3

#define WAN_TE_BERT_RC_SUCCESS		0
#define WAN_TE_BERT_RC_EINVAL		1
#define WAN_TE_BERT_RC_FAILED		2
#define WAN_TE_BERT_RC_RUNNING		3
#define WAN_TE_BERT_RC_STOPPED		4
#define WAN_TE_BERT_RC_DECODE(rc)					\
	((rc) == WAN_TE_BERT_RC_SUCCESS) ? "Success" :			\
	((rc) == WAN_TE_BERT_RC_EINVAL) ? "Invalid parameter" :		\
	((rc) == WAN_TE_BERT_RC_FAILED) ? "Failed" :			\
	((rc) == WAN_TE_BERT_RC_RUNNING) ? "BERT is still running" :	\
	((rc) == WAN_TE_BERT_RC_STOPPED) ? "BERT is not running" :	\
						"Unknown"

typedef struct 
{
	u_int32_t	chan_map;	/* */
	u_int16_t	pattern_type;	/* pattern type */
	u_int16_t	pattern_len;	/* pattern length */
	u_int32_t	pattern;	/* pattern */
	int		count;
	u_int8_t	lb_type;
	u_int16_t	eib;

} sdla_te_bert_cfg_t;

typedef struct 
{
	int		avail_sec;	/* out: */
	int		err_sec;	/* out: */
	int		err_free_sec;	/* out: */
	unsigned long	bit_cnt;	/* out: */
	unsigned long	err_cnt;	/* out: */	
	int		inlock;		/* out: */

} sdla_te_bert_stats_t;

typedef struct 
{
	u_int32_t		chan_map;
	u_int8_t		lb_type;

} sdla_te_bert_stop_t;

typedef struct 
{
	u_int8_t	cmd;
	int		verbose;
	int		rc;
	u_int8_t	status;

	union {
		sdla_te_bert_cfg_t	cfg;
		sdla_te_bert_stats_t	stats;
		sdla_te_bert_stop_t	stop;
	} un;

} sdla_te_bert_t;

/*
 ******************************************************************************
			STRUCTURES AND TYPEDEFS
 ******************************************************************************
*/

#ifdef WAN_KERNEL

/* Connection status threshold */
/* Original 5 
** Note (May 7 2009: We are waiting at least 10 sec anyway for other alarms */
#define WAN_TE1_STATUS_THRESHOLD	3	/* Feb 18, 2010.
										 * DavidR: changed to 3 from 1 to
										 *	avoid line bouncing during port
										 *	startup. */

#define WAN_TE1_LBO(fe)		FE_LBO(&((fe)->fe_cfg))
#define WAN_TE1_CLK(fe)		FE_CLK(&((fe)->fe_cfg))
#define WAN_TE1_REFCLK(fe)	FE_REFCLK(&((fe)->fe_cfg))
#define WAN_TE1_HI_MODE(fe)	FE_HIMPEDANCE_MODE(&((fe)->fe_cfg))
#define WAN_TE1_ACTIVE_CH(fe)	FE_ACTIVE_CH(&((fe)->fe_cfg))
#define WAN_TE1_SIG_MODE(fe)	FE_SIG_MODE(&((fe)->fe_cfg))

#define TE_LBO_DECODE(fe)	LBO_DECODE(&((fe)->fe_cfg))
#define TE_CLK_DECODE(fe)	TECLK_DECODE(&((fe)->fe_cfg))

#define WAN_TE1_SIG_DECODE(fe)	TE1SIG_DECODE(&((fe)->fe_cfg))

/* Read/Write to front-end register */
#if 0
#if 0
#define READ_REG(reg)		card->wandev.read_front_end_reg(card, reg)
#define WRITE_REG(reg, value)	card->wandev.write_front_end_reg(card, reg, (unsigned char)(value))
#endif
#define WRITE_REG(reg,val)						\
	fe->write_fe_reg(						\
		fe->card,						\
		(int)((reg) + (fe->fe_cfg.line_no*PMC4_LINE_DELTA)),	\
		(int)(val))

#define WRITE_REG_LINE(fe_line_no, reg,val)				\
	fe->write_fe_reg(						\
		fe->card,						\
		(int)((reg) + (fe_line_no)*PMC4_LINE_DELTA),	\
		(int)(val))
	
#define READ_REG(reg)							\
	fe->read_fe_reg(						\
		fe->card,						\
		(int)((reg) + (fe->fe_cfg.line_no*PMC4_LINE_DELTA)))
	
#define READ_REG_LINE(fe_line_no, reg)					\
	fe->read_fe_reg(						\
		fe->card,						\
		(int)((reg) + (fe_line_no)*PMC4_LINE_DELTA))
#endif

/* -----------------------------------------------------------------------------
 * Constants for the SET_T1_E1_SIGNALING_CFG/READ_T1_E1_SIGNALING_CFG commands
 * ---------------------------------------------------------------------------*/

/* the structure for setting the signaling permission */
#pragma pack(1)
typedef struct {
	unsigned char time_slot[32];
} te_signaling_perm_t;
#pragma pack()

/* settings for the signaling permission structure */
#define TE_SIG_DISABLED		0x00 /* signaling is disabled */
#define TE_RX_SIG_ENABLED	0x01 /* receive signaling is enabled */
#define TE_TX_SIG_ENABLED	0x02 /* transmit signaling is enabled */
#define TE_SET_TX_SIG_BITS	0x80 /* a flag indicating that outgoing 
					signaling bits should be set */

/* the structure used for the 
 * SET_T1_E1_SIGNALING_CFG/READ_T1_E1_SIGNALING_CFG command 
 */
#pragma pack(1)
typedef struct {
	/* signaling permission structure */
	te_signaling_perm_t sig_perm;
	/* loop signaling processing counter */
	unsigned char sig_processing_counter;
	/* pointer to the signaling permission structure */
	unsigned long ptr_te_sig_perm_struct;
	/* pointer to the receive signaling structure */
	unsigned long ptr_te_Rx_sig_struct;
	/* pointer to the transmit signaling structure */
	unsigned long ptr_te_Tx_sig_struct;
} te_signaling_cfg_t;
#pragma pack()

/* the structure used for reading and setting the signaling bits */
#pragma pack(1)
typedef struct {
	unsigned char time_slot[32];
} te_signaling_status_t;
#pragma pack()

typedef struct {
	unsigned long	ch_map;
	int 		rbs_channel;	/* tx rbs per channel */
	unsigned char	rbs_abcd;	/* tx rbs pec channel */
	unsigned char	lb_type;	/* loopback type */	
	unsigned short	reg;		/* fe register */	
	unsigned char	value;		/* fe register value */	
} sdla_te_event_t;

typedef struct {

	unsigned char	lb_tx_cmd;
	unsigned char	lb_tx_mode;
	unsigned char	lb_tx_code;
	unsigned long	lb_tx_cnt;

	unsigned long	critical;
	
	//unsigned char	timer_cmd;
	//unsigned long	timer_ch_map;
	//int 		timer_channel;	/* tx rbs per channel */
	//int		timer_delay;
	//unsigned char	timer_abcd;	/* tx rbs pec channel */
		
	unsigned char	SIGX_chg_30_25;
	unsigned char	SIGX_chg_24_17;
	unsigned char	SIGX_chg_16_9;
	unsigned char	SIGX_chg_8_1;

	unsigned long	ptr_te_sig_perm_off;
	unsigned long	ptr_te_Rx_sig_off;
	unsigned long	ptr_te_Tx_sig_off;

	unsigned char	intr_src1;
	unsigned char	intr_src2;
	unsigned char	intr_src3;

	unsigned int	max_channels;

	unsigned long	rx_rbs_status;
	unsigned char	rx_rbs[32];
	unsigned char	tx_rbs[32];

	unsigned long	tx_rbs_A;
	unsigned long	tx_rbs_B;
	unsigned long	tx_rbs_C;
	unsigned long	tx_rbs_D;

	unsigned long	rx_rbs_A;
	unsigned long	rx_rbs_B;
	unsigned long	rx_rbs_C;
	unsigned long	rx_rbs_D;

	unsigned char	xlpg_scale;
	
	u_int16_t	status_cnt;

	int		reg_dbg_busy;	
	int		reg_dbg_ready;	
	unsigned char	reg_dbg_value;

	wan_ticks_t	crit_alarm_start;
	unsigned int	lb_mode_map;
	
	int		tx_yel_alarm;

	unsigned int		bert_flag;
	wan_ticks_t		bert_start;
	sdla_te_bert_cfg_t	bert_cfg;
	sdla_te_bert_stats_t	bert_stats;
	
	uint8_t			tx_ais_alarm;
	wan_ticks_t		tx_ais_startup_timeout;

} sdla_te_param_t;


/*
 ******************************************************************************
			  FUNCTION PROTOTYPES
 ******************************************************************************
*/

WP_EXTERN int sdla_te_default_cfg(void* pfe, void* fe_cfg, int media);
WP_EXTERN int sdla_te_copycfg(void* pfe, void* fe_cfg);

WP_EXTERN int sdla_te_iface_init(void *p_fe, void *p_fe_iface);
WP_EXTERN int sdla_ds_te1_iface_init(void *p_fe, void *p_fe_iface);

#endif /* WAN_KERNEL */

#undef WP_EXTERN


/* Deprecated defines */
#define WAN_TE_ALARM_MASK_FRAMER    	WAN_TE_ALARM_FRAMER_MASK 
#define WAN_TE_ALARM_MASK_LIU       	WAN_TE_ALARM_LIU_MASK 
#define WAN_TE_BIT_ALOS_ALARM         	WAN_TE_BIT_ALARM_ALOS 
#define WAN_TE_BIT_LOS_ALARM         	WAN_TE_BIT_ALARM_LOS 
#define WAN_TE_BIT_ALTLOS_ALARM         WAN_TE_BIT_ALARM_ALTLOS 
#define WAN_TE_BIT_OOF_ALARM         	WAN_TE_BIT_ALARM_OOF 
#define WAN_TE_BIT_RED_ALARM         	WAN_TE_BIT_ALARM_RED 
#define WAN_TE_BIT_AIS_ALARM         	WAN_TE_BIT_ALARM_AIS 
#define WAN_TE_BIT_OOSMF_ALARM         	WAN_TE_BIT_ALARM_OOSMF 
#define WAN_TE_BIT_OOCMF_ALARM         	WAN_TE_BIT_ALARM_OOCMF 
#define WAN_TE_BIT_OOOF_ALARM         	WAN_TE_BIT_ALARM_OOOF 
#define WAN_TE_BIT_RAI_ALARM         	WAN_TE_BIT_ALARM_RAI 
#define WAN_TE_BIT_YEL_ALARM         	WAN_TE_BIT_ALARM_YEL  
#define WAN_TE_BIT_LIU_ALARM         	WAN_TE_BIT_ALARM_LIU 
#define WAN_TE_BIT_LIU_ALARM_SC         WAN_TE_BIT_ALARM_LIU_SC 
#define WAN_TE_BIT_LIU_ALARM_OC         WAN_TE_BIT_ALARM_LIU_OC 
#define WAN_TE_BIT_LIU_ALARM_LOS     	WAN_TE_BIT_ALARM_LIU_LOS 

#define WAN_TE_ALARM(alarm, bit)    ((alarm) & (bit)) ? "ON" : "OFF"

#define WAN_TE_ALOS_ALARM(alarm)    	WAN_TE_ALARM(alarm, WAN_TE_BIT_ALOS_ALARM)
#define WAN_TE_LOS_ALARM(alarm)        	WAN_TE_ALARM(alarm, WAN_TE_BIT_LOS_ALARM)
#define WAN_TE_OOF_ALARM(alarm)        	WAN_TE_ALARM(alarm, WAN_TE_BIT_OOF_ALARM)
#define WAN_TE_RED_ALARM(alarm)        	WAN_TE_ALARM(alarm, WAN_TE_BIT_RED_ALARM)
#define WAN_TE_AIS_ALARM(alarm)        	WAN_TE_ALARM(alarm, WAN_TE_BIT_AIS_ALARM)
#define WAN_TE_OOSMF_ALARM(alarm)    	WAN_TE_ALARM(alarm, WAN_TE_BIT_OOSMF_ALARM)
#define WAN_TE_OOCMF_ALARM(alarm)    	WAN_TE_ALARM(alarm, WAN_TE_BIT_OOCMF_ALARM)
#define WAN_TE_OOOF_ALARM(alarm)    	WAN_TE_ALARM(alarm, WAN_TE_BIT_OOOF_ALARM)
#define WAN_TE_RAI_ALARM(alarm)        	WAN_TE_ALARM(alarm, WAN_TE_BIT_RAI_ALARM)
#define WAN_TE_YEL_ALARM(alarm)        	WAN_TE_ALARM(alarm, WAN_TE_BIT_YEL_ALARM)    
#define WAN_TE_LIU_ALARM_SC(alarm)    	WAN_TE_ALARM(alarm, WAN_TE_BIT_LIU_ALARM_SC)
#define WAN_TE_LIU_ALARM_OC(alarm)    	WAN_TE_ALARM(alarm, WAN_TE_BIT_LIU_ALARM_OC)
#define WAN_TE_LIU_ALARM_LOS(alarm)    	WAN_TE_ALARM(alarm, WAN_TE_BIT_LIU_ALARM_LOS)


#endif /* _SDLA_TE1_H */
