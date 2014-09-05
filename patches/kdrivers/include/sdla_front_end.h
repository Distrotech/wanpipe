/*
 *************************************************************************************
 *                                                                                   *
 * sdla_front_end.h - the 'C' header file for the Sangoma S508/S514 adapter front-end API. *
 *                                                                                   *
 *************************************************************************************
*/
#ifndef _SDLA_FRONT_END_H_
#define _SDLA_FRONT_END_H_


# include "sdla_56k.h"
# include "sdla_te1.h"
# include "sdla_te3.h"
# include "sdla_remora.h"
# include "sdla_bri.h"
# include "sdla_serial.h"
# include "sdla_gsm.h"

/*
*************************************************************************
*			  DEFINES AND MACROS				*	
*************************************************************************
*/
/* Front-End media type */
#define WAN_MEDIA_NONE		0x00
#define WAN_MEDIA_T1		0x01
#define WAN_MEDIA_E1		0x02
#define WAN_MEDIA_56K		0x03
#define WAN_MEDIA_DS3		0x04
#define WAN_MEDIA_E3		0x05
#define WAN_MEDIA_STS1		0x06
#define WAN_MEDIA_J1		0x07
#define WAN_MEDIA_FXOFXS	0x08
#define WAN_MEDIA_BRI		0x09
#define WAN_MEDIA_SERIAL	0x0A
#define WAN_MEDIA_GSM           0x0B

/*The line code */
#define WAN_LCODE_NONE          0x00
#define WAN_LCODE_AMI           0x01	/* T1/E1/DS3/E3 */
#define WAN_LCODE_B8ZS          0x02	/* T1 */
#define WAN_LCODE_HDB3          0x03	/* E1/E3 */
#define WAN_LCODE_B3ZS          0x04	/* DS3 */

/* Framing modes */
#define WAN_FR_NONE		0x00
#define WAN_FR_ESF		0x01
#define WAN_FR_D4		0x02
#define WAN_FR_ESF_JAPAN	0x03
#define WAN_FR_CRC4		0x04
#define WAN_FR_NCRC4		0x05
#define WAN_FR_UNFRAMED		0x06
#define WAN_FR_E3_G751		0x07
#define WAN_FR_E3_G832		0x08
#define WAN_FR_DS3_Cbit		0x09
#define WAN_FR_DS3_M13		0x0A
#define WAN_FR_SLC96		0x0B

/* Clocking Master/Normal */
#define WAN_NORMAL_CLK		0x01
#define WAN_MASTER_CLK		0x02

/* Serial Card FE options */
#define WAN_FE_V35		0x01
#define WAN_FE_RS232		0x02

/* Front-End DEBUG flags */
#define WAN_FE_DEBUG_NONE		0x00
#define WAN_FE_DEBUG_RBS		0x01
#define WAN_FE_DEBUG_ALARM		0x02
#define WAN_FE_DEBUG_VOLTAGE		0x03
#define WAN_FE_DEBUG_RECONFIG		0x04
#define WAN_FE_DEBUG_REG		0x05
#define WAN_FE_DEBUG_CONFIG_VERIFY	0x06
#define WAN_FE_DEBUG_HOOK		0x07

/* Front-End DEBUG sub-flags */
/* Sub-flags for RBS debugging */
#define WAN_FE_DEBUG_RBS_RX_ENABLE	0x01
#define WAN_FE_DEBUG_RBS_TX_ENABLE	0x02
#define WAN_FE_DEBUG_RBS_RX_DISABLE	0x03
#define WAN_FE_DEBUG_RBS_TX_DISABLE	0x04
#define WAN_FE_DEBUG_RBS_READ		0x05
#define WAN_FE_DEBUG_RBS_SET		0x06
#define WAN_FE_DEBUG_RBS_PRINT		0x07
#define WAN_FE_DEBUG_RBS_DECODE(mode)					\
	((mode) == WAN_FE_DEBUG_RBS_RX_ENABLE) ? "Enable RBS RX" :	\
	((mode) == WAN_FE_DEBUG_RBS_TX_ENABLE) ? "Enable RBS TX" :	\
	((mode) == WAN_FE_DEBUG_RBS_RX_DISABLE) ? "Disable RBS RX" :	\
	((mode) == WAN_FE_DEBUG_RBS_TX_DISABLE) ? "Disable RBS TX" :	\
	((mode) == WAN_FE_DEBUG_RBS_READ) ? "Read RBS" :		\
	((mode) == WAN_FE_DEBUG_RBS_SET) ? "Set RBS" :			\
	((mode) == WAN_FE_DEBUG_RBS_PRINT) ? "Print RBS" : "Unknown"

/* Sub-flags for ALARM debugging */
#define WAN_FE_DEBUG_ALARM_AIS_ENABLE	0x01
#define WAN_FE_DEBUG_ALARM_AIS_DISABLE	0x02
#define WAN_FE_DEBUG_ALARM_DECODE(mode)					\
	((mode) == WAN_FE_DEBUG_ALARM_AIS_ENABLE) ? "Enable TX AIS" :	\
	((mode) == WAN_FE_DEBUG_ALARM_AIS_DISABLE) ? "Disable TX AIS" :	\
							"Unknown"

/* Front-End TX tri-state mode flags */
#define WAN_FE_TXMODE_ENABLE	0x01
#define WAN_FE_TXMODE_DISABLE	0x02

/* Read alarm flag */
#define WAN_FE_ALARM_NONE	0x00
#define WAN_FE_ALARM_READ	0x01
#define WAN_FE_ALARM_PRINT	0x02
#define WAN_FE_ALARM_UPDATE	0x04
#define IS_FE_ALARM_READ(action)	((action) & WAN_FE_ALARM_READ)
#define IS_FE_ALARM_PRINT(action)	((action) & WAN_FE_ALARM_PRINT)
#define IS_FE_ALARM_UPDATE(action)	((action) & WAN_FE_ALARM_UPDATE)

/* Read pmon flag */
#define WAN_FE_PMON_UPDATE	0x01
#define WAN_FE_PMON_PRINT	0x02
#define WAN_FE_PMON_READ	0x04
#define IS_FE_PMON_UPDATE(action)	((action) & WAN_FE_PMON_UPDATE)
#define IS_FE_PMON_PRINT(action)	((action) & WAN_FE_PMON_PRINT)
#define IS_FE_PMON_READ(action)		((action) & WAN_FE_PMON_READ)

/* FE interrupt action command */
#define WAN_FE_INTR_ENABLE	0x01
#define WAN_FE_INTR_MASK	0x02

/* FE event action command */
#define WAN_FE_EVENT_ENABLE	0x01
#define WAN_FE_EVENT_MASK	0x02

/* Alaw/Mulaw */
#define WAN_TDMV_ALAW		0x01
#define WAN_TDMV_MULAW		0x02
#define WAN_TDMV_HDLC		0x03

#define FE_MEDIA(fe_cfg)	((fe_cfg)->media)
#define FE_SUBMEDIA(fe_cfg)	((fe_cfg)->sub_media)
#define FE_LCODE(fe_cfg)	((fe_cfg)->lcode)
#define FE_FRAME(fe_cfg)	((fe_cfg)->frame)
#define FE_LINENO(fe_cfg)	((fe_cfg)->line_no)
#define FE_TXTRISTATE(fe_cfg)	((fe_cfg)->tx_tristate_mode)
#define FE_TDMV_LAW(fe_cfg)	((fe_cfg)->tdmv_law)
#define FE_NETWORK_SYNC(fe_cfg)	((fe_cfg)->network_sync)

#define IS_T1_MEDIA(fe_cfg)	(FE_MEDIA(fe_cfg) == WAN_MEDIA_T1)
#define IS_E1_MEDIA(fe_cfg)	(FE_MEDIA(fe_cfg) == WAN_MEDIA_E1)
#define IS_J1_MEDIA(fe_cfg)	(FE_MEDIA(fe_cfg) == WAN_MEDIA_T1 &&	\
				FE_SUBMEDIA(fe_cfg) == WAN_MEDIA_J1)
#define IS_TE1_MEDIA(fe_cfg)	(IS_T1_MEDIA(fe_cfg) ||	\
	       			 IS_E1_MEDIA(fe_cfg) || 	\
				 IS_J1_MEDIA(fe_cfg))
#define IS_56K_MEDIA(fe_cfg)	(FE_MEDIA(fe_cfg) == WAN_MEDIA_56K)
#define IS_TE1_56K_MEDIA(fe_cfg)(IS_TE1_MEDIA(fe_cfg) || 	\
				 IS_56K_MEDIA(fe_cfg)
#define IS_DS3_MEDIA(fe_cfg)	(FE_MEDIA(fe_cfg) == WAN_MEDIA_DS3)
#define IS_E3_MEDIA(fe_cfg)		(FE_MEDIA(fe_cfg) == WAN_MEDIA_E3)
#define IS_FXOFXS_MEDIA(fe_cfg)	(FE_MEDIA(fe_cfg) == WAN_MEDIA_FXOFXS)
#define IS_BRI_MEDIA(fe_cfg)	(FE_MEDIA(fe_cfg) == WAN_MEDIA_BRI)
#define IS_GSM_MEDIA(fe_cfg)	(FE_MEDIA(fe_cfg) == WAN_MEDIA_GSM)
#define IS_SERIAL_MEDIA(fe_cfg)	(FE_MEDIA(fe_cfg) == WAN_MEDIA_SERIAL)

#define IS_TXTRISTATE(fe_cfg)	(FE_TXTRISTATE(fe_cfg) == WANOPT_YES)

#define IS_FR_UNFRAMED(fe_cfg)	(FE_FRAME(fe_cfg) == WAN_FR_UNFRAMED)

#define MEDIA_DECODE(fe_cfg)					\
		(FE_MEDIA(fe_cfg) == WAN_MEDIA_T1 &&		\
	 	 FE_SUBMEDIA(fe_cfg)==WAN_MEDIA_J1)?"J1" :	\
		(FE_MEDIA(fe_cfg) == WAN_MEDIA_T1) ? "T1" :	\
		(FE_MEDIA(fe_cfg) == WAN_MEDIA_E1) ? "E1" : 	\
		(FE_MEDIA(fe_cfg) == WAN_MEDIA_J1) ? "J1" : 	\
		(FE_MEDIA(fe_cfg) == WAN_MEDIA_56K) ? "56K" : 	\
		(FE_MEDIA(fe_cfg) == WAN_MEDIA_DS3) ? "DS3" : 	\
		(FE_MEDIA(fe_cfg) == WAN_MEDIA_E3) ? "E3" :	\
		(FE_MEDIA(fe_cfg) == WAN_MEDIA_FXOFXS) ? "FXO/FXS" :	\
		(FE_MEDIA(fe_cfg) == WAN_MEDIA_BRI) ? "BRI" :	\
		(FE_MEDIA(fe_cfg) == WAN_MEDIA_SERIAL) ? "V35/RS232" :	\
						"Unknown"

#define LCODE_DECODE(fe_cfg)					\
		(FE_LCODE(fe_cfg) == WAN_LCODE_AMI)  ? "AMI" :	\
		(FE_LCODE(fe_cfg) == WAN_LCODE_B8ZS) ? "B8ZS" :	\
		(FE_LCODE(fe_cfg) == WAN_LCODE_HDB3) ? "HDB3" :	\
		(FE_LCODE(fe_cfg) == WAN_LCODE_B3ZS) ? "B3ZS" : "Unknown"

#define FRAME_DECODE(fe_cfg)					\
		(FE_FRAME(fe_cfg) == WAN_FR_ESF)	? "ESF"  : 	\
		(FE_FRAME(fe_cfg) == WAN_FR_D4) 	? "D4"   : 	\
		(FE_FRAME(fe_cfg) == WAN_FR_CRC4)	? "CRC4" : 	\
		(FE_FRAME(fe_cfg) == WAN_FR_NCRC4)	? "non-CRC4" :	\
		(FE_FRAME(fe_cfg) == WAN_FR_UNFRAMED)	? "Unframed" : 	\
		(FE_FRAME(fe_cfg) == WAN_FR_E3_G751)	? "G.751" : 	\
		(FE_FRAME(fe_cfg) == WAN_FR_E3_G832)	? "G.832" : 	\
		(FE_FRAME(fe_cfg) == WAN_FR_DS3_Cbit)	? "C-bit" : 	\
		(FE_FRAME(fe_cfg) == WAN_FR_DS3_M13)	? "M13" : 	\
		(FE_FRAME(fe_cfg) == WAN_FR_SLC96)	? "SLC-96" : "Unknown"



/* front-end configuration and access interface commands */
#define READ_FRONT_END_REGISTER		(WAN_FE_CMD_START+0)	/* 0x90 read from front-end register */
#define WRITE_FRONT_END_REGISTER	(WAN_FE_CMD_START+1)	/* 0x91 write to front-end register */
#define READ_FRONT_END_STATISTICS	(WAN_FE_CMD_START+2)	/* 0x92 read the front-end statistics */
#define FLUSH_FRONT_END_STATISTICS	(WAN_FE_CMD_START+3)	/* 0x93 flush the front-end statistics */

typedef struct {
	unsigned char	media;		/* must be first !!! */
	unsigned char	sub_media;
	unsigned char	chip_id;
	unsigned char	max_ports;

} wan_femedia_t;

typedef struct {
	unsigned char	media;
	unsigned char	sub_media;
	unsigned char	lcode;
	unsigned char	frame;
	unsigned int	line_no;
	unsigned char	tx_tristate_mode;
	unsigned int	tdmv_law;
	unsigned char	poll_mode;
	int				network_sync;

	union {
		sdla_te_cfg_t		te_cfg;
		sdla_te3_cfg_t		te3_cfg;
		sdla_remora_cfg_t	remora;
		sdla_bri_cfg_t		bri;
	} cfg;
} sdla_fe_cfg_t;

typedef struct {
	u_int32_t 	alarms;
	union {
		sdla_te_stats_t	te1_stats;
		/*sdla_te_pmon_t	te_pmon;*/
		sdla_te3_pmon_t	te3_pmon;
	} u;
	u_int32_t	tx_alarms;
	u_int32_t	tx_maint_alarms;
#define te_pmon	u.te1_stats.pmon	
} sdla_fe_stats_t;

typedef struct {
	unsigned char	type;
	unsigned char	mode;
	int		mod_no;	/* A200/A400 */
	union{
		struct {
			int		channel;
			unsigned char	abcd;
		} rbs;
		struct {
			int		read;
			int		reg;
			unsigned char	value;
		} reg;
		struct {
			int		offhook;
		}hook;
	} fe_debug_un;
#define fe_debug_rbs	fe_debug_un.rbs	
#define fe_debug_reg	fe_debug_un.reg	
#define fe_debug_hook	fe_debug_un.hook	
} sdla_fe_debug_t;

#define WAN_FE_LBMODE_CMD_POLL		0x00
#define WAN_FE_LBMODE_CMD_SET		0x01
#define WAN_FE_LBMODE_CMD_GET		0x02

#define WAN_FE_LBMODE_RC_SUCCESS        0x00
#define WAN_FE_LBMODE_RC_PENDING        0x01
#define WAN_FE_LBMODE_RC_FAILED         0x02
#define WAN_FE_LBMODE_RC_LINKDOWN       0x03
#define WAN_FE_LBMODE_RC_TXBUSY         0x04
#define WAN_FE_LBMODE_RC_DECODE(rc)                                     \
		((rc) == WAN_FE_LBMODE_RC_SUCCESS)  ? "Done" :          \
		((rc) == WAN_FE_LBMODE_RC_PENDING)  ? "In progress" :   \
		((rc) == WAN_FE_LBMODE_RC_FAILED)   ? "Failed" :        \
		((rc) == WAN_FE_LBMODE_RC_LINKDOWN) ? "Failed (Link Down)" :    \
		((rc) == WAN_FE_LBMODE_RC_TXBUSY)   ? "Failed (Busy)" : "Internal Error"

typedef struct 
{
	u_int8_t 	cmd;
	u_int8_t 	type;
	u_int8_t 	mode;
	u_int32_t	chan_map;
	u_int32_t	type_map;	/* out: bit-map of loopbacks */
	u_int8_t	rc;		/* out: */

} sdla_fe_lbmode_t;

/* Front-End status */
#define FE_STATUS_DECODE(fe_status)							\
		(fe_status == FE_UNITIALIZED)	? "uninitialized"	:	\
		(fe_status == FE_DISCONNECTED)  ? "disconnected":	\
		(fe_status == FE_CONNECTED)     ? "connected"	:	\
		(fe_status == FE_CONNECTING)    ? "connecting"	:	\
		(fe_status == FE_DISCONNECTING) ? "disconnecting" :	\
						      "unknown"

#define WAN_FE_STATUS_DECODE(fe)	FE_STATUS_DECODE((fe)->fe_status)
#define LINK_STATUS_DECODE(fe_status) 	FE_STATUS_DECODE(fe_status)

#ifdef WAN_KERNEL

#define FE_ASSERT(val)	if (val) return;
#define FE_ASSERT1(val)	if (val) return 1;

#define WAN_FECALL(dev, func, x)					\
	((dev)->fe_iface.func) ? (dev)->fe_iface.func x : -EINVAL

#define IS_T1_FEMEDIA(fe)	IS_T1_MEDIA(&((fe)->fe_cfg))
#define IS_E1_FEMEDIA(fe)	IS_E1_MEDIA(&((fe)->fe_cfg))
#define IS_TE1_FEMEDIA(fe)	IS_TE1_MEDIA(&((fe)->fe_cfg))
#define IS_56K_FEMEDIA(fe)	IS_56K_MEDIA(&((fe)->fe_cfg))
#define IS_J1_FEMEDIA(fe)	IS_J1_MEDIA(&((fe)->fe_cfg))
#define IS_FXOFXS_FEMEDIA(fe)	IS_FXOFXS_MEDIA(&((fe)->fe_cfg))
#define IS_GSM_FEMEDIA(fe)	IS_GSM_MEDIA(&((fe)->fe_cfg))
#define IS_FE_TXTRISTATE(fe)	IS_TXTRISTATE(&((fe)->fe_cfg))
#define IS_FR_FEUNFRAMED(fe)	IS_FR_UNFRAMED(&((fe)->fe_cfg))
#define IS_BRI_FEMEDIA(fe)	IS_BRI_MEDIA(&((fe)->fe_cfg))
#define IS_SERIAL_FEMEDIA(fe)	IS_SERIAL_MEDIA(&((fe)->fe_cfg))

#define WAN_FE_MEDIA(fe)	FE_MEDIA(&((fe)->fe_cfg))
#define WAN_FE_LCODE(fe)	FE_LCODE(&((fe)->fe_cfg))
#define WAN_FE_FRAME(fe)	FE_FRAME(&((fe)->fe_cfg))
#define WAN_FE_LINENO(fe)	FE_LINENO(&((fe)->fe_cfg))
#define WAN_FE_TXTRISTATE(fe)	FE_TXTRISTATE(&((fe)->fe_cfg))
#define WAN_FE_TDMV_LAW(fe)	FE_TDMV_LAW(&((fe)->fe_cfg))

#define WAN_FE_NETWORK_SYNC(fe)		FE_NETWORK_SYNC(&((fe)->fe_cfg))

#define FE_MEDIA_DECODE(fe)	MEDIA_DECODE(&((fe)->fe_cfg))
#define FE_LCODE_DECODE(fe)	LCODE_DECODE(&((fe)->fe_cfg))
#define FE_FRAME_DECODE(fe)	FRAME_DECODE(&((fe)->fe_cfg))

#define WAN_FE_START_CHANNEL(fe)				\
	(IS_TE1_FEMEDIA(fe))	? GET_TE_START_CHANNEL(fe) :	\
	(IS_FXOFXS_FEMEDIA(fe))	? WAN_RM_START_CHANNEL:		\
	(IS_BRI_FEMEDIA(fe))	? WAN_BRI_START_CHANNEL:	0

#define WAN_FE_MAX_CHANNELS(fe)					\
	(IS_TE1_FEMEDIA(fe))	? GET_TE_CHANNEL_RANGE(fe) :	\
	(IS_FXOFXS_FEMEDIA(fe))	? MAX_FXOFXS_CHANNELS :		\
	(IS_GSM_FEMEDIA(fe))	? MAX_GSM_CHANNELS :		\
	(IS_BRI_FEMEDIA(fe))	? MAX_BRI_CHANNELS : 0

#if 0
enum fe_status {
	FE_UNITIALIZED = 0x00,
	FE_DISCONNECTED,
	FE_CONNECTED
};
#endif

/* adapter configuration interface commands */
#define SET_ADAPTER_CONFIGURATION	(WAN_INTERFACE_CMD_START+0)	/* 0xA0 set adapter configuration */
#define READ_ADAPTER_CONFIGURATION	(WAN_INTERFACE_CMD_START+1)	/* 0xA1 read adapter configuration */

/* return codes from interface commands */
#define LGTH_FE_CFG_DATA_INVALID       0x91 /* the length of the FE_RX_DISC_TX_IDLE_STRUCT is invalid */
#define LGTH_ADAPTER_CFG_DATA_INVALID  0x91 /* the length of the passed configuration data is invalid */
#define INVALID_FE_CFG_DATA            0x92 /* the passed SET_FE_RX_DISC_TX_IDLE_CFG data is invalid */
#define ADPTR_OPERATING_FREQ_INVALID   0x92 /* an invalid adapter operating frequency was selected */
#define PROT_CFG_BEFORE_FE_CFG         0x93 /* set the protocol-level configuration before setting the FE configuration */

#define SET_FE_RX_DISC_TX_IDLE_CFG      0x98 /* set the front-end Rx discard/Tx idle configuration */
#define READ_FE_RX_DISC_TX_IDLE_CFG     0x99 /* read the front-end Rx discard/Tx idle configuration */
#define SET_TE1_SIGNALING_CFG		0x9A /* set the T1/E1 signaling configuration */
#define READ_TE1_SIGNALING_CFG	0x9B /* read the T1/E1 signaling configuration */


#define COMMAND_INVALID_FOR_ADAPTER    0x9F /* the command is invalid for the adapter type */

 
/* ---------------------------------------------------------------------------------
 * Constants for the SET_FE_RX_DISC_TX_IDLE_CFG/READ_FE_RX_DISC_TX_IDLE_CFG commands
 * --------------------------------------------------------------------------------*/

#define NO_ACTIVE_RX_TIME_SLOTS_T1   24 /* T1 - no active time slots used for reception */
#define NO_ACTIVE_TX_TIME_SLOTS_T1   24 /* T1 - no active time slots used for transmission */
#define NO_ACTIVE_RX_TIME_SLOTS_E1   32 /* E1 - no active time slots used for reception */
#define NO_ACTIVE_TX_TIME_SLOTS_E1   31 /* E1 - no active time slots used for transmission (channel 0 reserved for framing) */

/* the structure used for the SET_FE_RX_DISC_TX_IDLE_CFG/READ_FE_RX_DISC_TX_IDLE_CFG command */
#pragma pack(1)
typedef struct {
        unsigned short lgth_Rx_disc_bfr; /* the length of the Rx discard buffer */
        unsigned short lgth_Tx_idle_bfr; /* the length of the Tx idle buffer */
                                                /* the transmit idle data buffer */
        unsigned char Tx_idle_data_bfr[NO_ACTIVE_TX_TIME_SLOTS_E1];
} FE_RX_DISC_TX_IDLE_STRUCT;
#pragma pack()
                                         

/* ----------------------------------------------------------------------------
 *                       Constants for front-end access
 * --------------------------------------------------------------------------*/

/* the structure used for the READ_FRONT_END_REGISTER/WRITE_FRONT_END_REGISTER command */
#pragma pack(1)
typedef struct {
	unsigned short register_number; /* the register number to be read from or written to */
	unsigned char register_value;	/* the register value read/written */
} FRONT_END_REG_STRUCT;
#pragma pack()

#pragma pack(1)
typedef struct {
	unsigned char opp_flag;	/* opp flag */
	
	union {
		struct {
			unsigned char RR8_56k;	/* register #8 value - 56K CSU/DSU */
			unsigned char RR9_56k;	/* register #9 value - 56K CSU/DSU */
			unsigned char RRA_56k;	/* register #A value - 56K CSU/DSU */	
			unsigned char RRB_56k;	/* register #B value - 56K CSU/DSU */
			unsigned char RRC_56k;	/* register #C value - 56K CSU/DSU */
		} stat_56k;
	} FE_U;

} FRONT_END_STATUS_STRUCT;
#pragma pack()


/* -----------------------------------------------------------------------------
 *            Constants for the READ_FRONT_END_STATISTICS command
 * ---------------------------------------------------------------------------*/

/* the front-end statistics structure */
#pragma pack(1)
typedef struct {
	unsigned int FE_interrupt_count;   /* the number of front-end interrupts generated */
	unsigned int FE_app_timeout_count; /* the number of front-end interrupt application timeouts */
} FE_STATISTICS_STRUCT;
#pragma pack()



/* --------------------------------------------------------------------------------
 * Constants for the SET_ADAPTER_CONFIGURATION/READ_ADAPTER_CONFIGURATION commands
 * -------------------------------------------------------------------------------*/

/* the adapter configuration structure */
#pragma pack(1)
typedef struct {
	unsigned short adapter_type;	/* type of adapter */
	unsigned short adapter_config;	/* miscellaneous adapter configuration options */
	unsigned int operating_frequency;	/* adapter operating frequency */
} ADAPTER_CONFIGURATION_STRUCT;
#pragma pack()

typedef int		(WRITE_FRONT_END_REG_T)(void*, ...);
typedef unsigned char	(READ_FRONT_END_REG_T)(void*, ...);

enum {
   AFT_LED_ON,
   AFT_LED_OFF,
   AFT_LED_TOGGLE
};

typedef struct sdla_fe_swirq_ {
	unsigned int	pending;
	unsigned char	subtype;
	int		delay;
	wan_ticks_t	start;
} sdla_fe_swirq_t;

typedef struct sdla_fe_timer_event_ {
	unsigned char	type;
	u_int8_t	mode;
	int		delay;
	wan_ticks_t	start;
	union{
#define te_event	u_fe.te
#define rm_event	u_fe.rm
		sdla_te_event_t	te;
		sdla_rm_event_t	rm;
	} u_fe;
	WAN_LIST_ENTRY(sdla_fe_timer_event_)	next;
} sdla_fe_timer_event_t;

#define WAN_FE_MAX_QEVENT_LEN	20
typedef struct {
	char		*name;
	void		*card;
	sdla_fe_cfg_t	fe_cfg;
	/* FIXME: Remove the following parameters from wandev_t */
	unsigned char	fe_status;
	unsigned int	fe_prev_alarm;
	unsigned char	fe_chip_id;
	unsigned char	fe_max_ports;
	unsigned char	fe_debug;
	/* ^^^ */
	union {
#define te_param	fe_param.te
#define te3_param	fe_param.te3
#define rm_param	fe_param.remora
#define bri_param	fe_param.bri
		sdla_te_param_t		te;
		sdla_56k_param_t	k56_param;
		sdla_te3_param_t	te3;
		sdla_remora_param_t	remora;
		sdla_bri_param_t	bri;
		sdla_gsm_param_t	gsm;
	} fe_param;
#define fe_alarm		fe_stats.alarms
	sdla_fe_stats_t		fe_stats;
	
	wan_spinlock_t		lockirq;
	wan_timer_t		timer;
#if defined(__WINDOWS__)
	WAN_LIST_HEAD(fe_timer_event_list_head, sdla_fe_timer_event_)	event;
#else
	WAN_LIST_HEAD(, sdla_fe_timer_event_)	event;
#endif
	unsigned int	event_map;
	
	sdla_fe_swirq_t	*swirq;
	unsigned int	swirq_map;

	int		(*write_cpld)(void*, unsigned short, unsigned char);
	int		(*read_cpld)(void*, unsigned short, unsigned char);
	int		(*write_fe_cpld)(void*, unsigned short, unsigned char);
	int		(*read_fe_cpld)(void*, unsigned short, unsigned char);
	int 		(*write_framer)(void*,unsigned short,unsigned short);
	unsigned int 	(*read_framer)(void*,unsigned short);
	void		(*reset_fe)(void*);
	WRITE_FRONT_END_REG_T	*write_fe_reg;
	READ_FRONT_END_REG_T	*read_fe_reg;
	READ_FRONT_END_REG_T	*__read_fe_reg;
} sdla_fe_t;

/*
** Sangoma Front-End interface structure 
*/
#if 0
typedef struct {
	/* In-Service or Not (T1/E1/56K) */
	unsigned int	(*get_fe_service_status)(void*);
	/* Print Front-End alarm (T1/E1/56K) */
	void		(*print_fe_alarm)(void*,unsigned int);
	/* Print Front-End alarm (T1/E1/56K) */
	char*		(*print_fe_act_channels)(void*);
	/* Set Front-End alarm (T1/E1) */
	void		(*set_fe_alarm)(void*,unsigned int);
	/* Set extra interrupt type (after link get connected)) */
	int		(*set_fe_sigcfg)(void*, unsigned int);
} sdla_fe_iface_t;
#endif

#include "wanpipe_events.h"

/* 
** Sangoma Front-End interface structure (new version)
** FIXME: replace sdla_fe_iface_t with the new version! */
typedef struct {
	int		(*reset)(void *fe, int, int);
	int		(*global_config)(void *fe);
	int		(*global_unconfig)(void *fe);
	int		(*chip_config)(void *fe);
	int		(*config)(void *fe);
	int		(*post_init)(void*);
	/* Set extra T1/E1 configuration */
	int		(*reconfig)(sdla_fe_t*);
	int		(*unconfig)(void *fe);
	int		(*post_unconfig)(void*);
	int		(*if_config)(void *fe, u32, u8);
	int		(*if_unconfig)(void *fe, u32, u8);
	int		(*disable_irq)(void *fe);
	int		(*polling)(sdla_fe_t *fe);
	int		(*add_timer)(sdla_fe_t *fe, unsigned long);
	int		(*isr)(sdla_fe_t *fe);
	int		(*process_udp)(sdla_fe_t *fe, void*, unsigned char*);
 	u_int32_t 	(*read_alarm)(sdla_fe_t *fe, int);
	
	int		(*read_pmon)(sdla_fe_t *fe, int);
	int		(*flush_pmon)(sdla_fe_t *fe);
	/* Set Front-End alarm (T1/E1) */
	int		(*set_fe_alarm)(sdla_fe_t *fe, unsigned int);
	/* Set extra interrupt type (after link get connected)) */
	int		(*set_fe_sigctrl)(sdla_fe_t*, int, unsigned long, int);
	/* Print Front-End alarm (T1/E1/56K) */
	char*		(*print_fe_act_channels)(sdla_fe_t*);
	/* Print Front-End alarm (T1/E1/56K) */
	int		(*print_fe_alarm)(sdla_fe_t*,unsigned int);
	/* Get front end status: Connected/Disconnected  */
	/* added mod_no to get status of individual fxo (mod_no only used for Analog/FXO cards) */
	int		(*get_fe_status)(sdla_fe_t *fe, unsigned char*, int mod_no);
	/* Set front end status: Connected/Disconnected only ISDN BRI */
	int		(*set_fe_status)(sdla_fe_t *fe, unsigned char);
	/* Get front end media type */
	unsigned char	(*get_fe_media)(sdla_fe_t *fe);
	/* Get front end media type string */
	char*		(*get_fe_media_string)(void);
	/* Set Line-loopback modes */
	int		(*set_fe_lbmode)(sdla_fe_t*, u_int8_t, u_int8_t, u_int32_t);
	/* Update Alarm Status for proc file system */
	int		(*update_alarm_info)(sdla_fe_t*, struct seq_file*, int*);
	/* Update PMON Status for proc file system */
	int		(*update_pmon_info)(sdla_fe_t*, struct seq_file*, int*);
	/* AFT T1/E1 cards only */
	int		(*led_ctrl)(sdla_fe_t*, int mode);
	/* Check RBS bits (T1/E1 cards) */
	int		(*check_rbsbits)(sdla_fe_t*, int, unsigned int, int);
	/* Read RBS bits (T1/E1 cards) */
	unsigned char	(*read_rbsbits)(sdla_fe_t*, int, int);
	/* Set RBS bits (T1/E1 cards, voice) */
	int		(*set_rbsbits)(sdla_fe_t*, int, unsigned char);
	/* Report RBS bits (T1/E1 cards) */
	int		(*report_rbsbits)(sdla_fe_t*);
	/* Get Front-End SNMP data */
	int		(*get_snmp_data)(sdla_fe_t*, void*, void*);
	/* Check if the interrupt is for this port */
	int		(*check_isr)(sdla_fe_t *fe);
	/* Available front-end map */
	/* BRI addition - one Wanpipe may have two lines, provide the 'line_no' when
	   getting the active channels map.
	*/
	unsigned int	(*active_map)(sdla_fe_t *fe, unsigned char line_no);
	/* Transmit DTMF number */
	int		(*set_dtmf)(sdla_fe_t*, int, unsigned char);
	/* Enable/Disable FE interrupt */
	int		(*intr_ctrl)(sdla_fe_t*, int, u_int8_t, u_int8_t, unsigned int);
	/* Event Control */
	int		(*event_ctrl)(sdla_fe_t*, wan_event_ctrl_t*);
	/* Front-End watchdog */
	int		(*watchdog)(void * card);
	/* Transmit ISDN BRI D-chan data */
	int		(*isdn_bri_dchan_tx)(sdla_fe_t*, void*, unsigned int);
	/* Receive ISDN BRI D-chan data */
	int		(*isdn_bri_dchan_rx)(sdla_fe_t*, void*, unsigned int);
	/* Enable/Disable Clock recovery from the line */
	int		(*clock_ctrl)(sdla_fe_t*, u_int8_t /* WANOPT_NO/WANOPT_YES */);
	u_int32_t 	(*read_tx_alarm)(sdla_fe_t *fe, int);
} sdla_fe_iface_t;

/* 
** Sangoma Front-End interface structure (new version)
** FIXME: replace sdla_fe_iface_t with the new version! */
typedef struct {

	int		(*check_hook_state)(sdla_fe_t *, int);
	int		(*hook_state)(sdla_fe_t *, int, int);

} sdla_fe_notify_iface_t;

#endif	/* WAN_KERNEL */

#endif
