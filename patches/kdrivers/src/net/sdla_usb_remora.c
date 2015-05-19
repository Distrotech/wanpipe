/***************************************************************************
 * sdla_usb_remora.c	WANPIPE(tm) Multiprotocol WAN Link Driver. 
 *				AFT REMORA and FXO/FXS support module.
 *
 * Author: 	Alex Feldman   <al.feldman@sangoma.com>
 *
 * Copyright:	(c) 2005 Sangoma Technologies Inc.
 *
 *		This program is free software; you can redistribute it and/or
 *		modify it under the terms of the GNU General Public License
 *		as published by the Free Software Foundation; either version
 *		2 of the License, or (at your option) any later version.
 * ============================================================================
 * Oct  6, 2005	Alex Feldman	Initial version.
 * Jul 27, 2006	David Rokhvarg	<davidr@sangoma.com>	Ported to Windows.
 ******************************************************************************
 */

/******************************************************************************
**			   INCLUDE FILES
*******************************************************************************/
#if defined(__FreeBSD__) || defined(__OpenBSD__)
# include <wanpipe_includes.h>
# include <wanpipe_defines.h>
# include <wanpipe_debug.h>
# include <wanpipe_abstr.h>
# include <wanpipe_common.h>
# include <wanpipe_events.h>
# include <wanpipe.h>
# include <sdla_usb_remora.h>
# include <wanpipe_events.h>
#elif (defined __WINDOWS__)
# include <wanpipe_includes.h>
# include <wanpipe.h>
# include <sdla_usb_remora.h>
# include <wanpipe_events.h>
#else
# include <linux/wanpipe_includes.h>
# include <linux/wanpipe_defines.h>
# include <linux/wanpipe_debug.h>
# include <linux/wanpipe_common.h>
# include <linux/wanpipe_events.h>
# include <linux/wanpipe.h>
# include <linux/sdla_usb_remora.h>
# include <linux/wanpipe_events.h>
#endif

#if !defined(__WINDOWS__)
#if 1
#define AFT_FUNC_DEBUG()
#else
#define AFT_FUNC_DEBUG()  DEBUG_EVENT("%s:%d\n",__FUNCTION__,__LINE__)
#endif
#endif

/*******************************************************************************
**			  DEFINES AND MACROS
*******************************************************************************/

#undef WAN_REMORA_AUTO_CALIBRATE


#if 0
# define AFT_RM_INTR_SUPPORT
#else
# undef	AFT_RM_INTR_SUPPORT
#endif

#undef AUDIO_RINGCHECK

#if 1
/* Current A200 designs (without interrupts) */
# define AFT_RM_VIRTUAL_INTR_SUPPORT
#else
# undef	AFT_RM_VIRTUAL_INTR_SUPPORT
#endif

/* TE1 critical flag */
#define WP_RM_TIMER_RUNNING		0x01
#define WP_RM_TIMER_KILL 		0x02
#define WP_RM_CONFIGURED 		0x03
#define WP_RM_TIMER_EVENT_PENDING	0x04

#define WP_RM_POLL_TIMER	100
#define WP_RM_POLL_EVENT_TIMER	10
#define WP_RM_POLL_TONE_TIMER	5000
#define WP_RM_POLL_RING_TIMER	10000
enum {
	WP_RM_POLL_TONE_DIAL	= 1,
	WP_RM_POLL_TONE_BUSY,
	WP_RM_POLL_TONE_RING,
	WP_RM_POLL_TONE_CONGESTION,
	WP_RM_POLL_TONE_DONE,
	WP_RM_POLL_TDMV,
	WP_RM_POLL_EVENT,
	WP_RM_POLL_INIT,
	WP_RM_POLL_POWER,
	WP_RM_POLL_LC,
	WP_RM_POLL_RING_TRIP,
	WP_RM_POLL_DTMF,
	WP_RM_POLL_RING,
	WP_RM_POLL_TONE,
	WP_RM_POLL_TXSIG_KEWL,
	WP_RM_POLL_TXSIG_START,
	WP_RM_POLL_TXSIG_OFFHOOK,
	WP_RM_POLL_TXSIG_ONHOOK,
	WP_RM_POLL_ONHOOKTRANSFER,
	WP_RM_POLL_SETPOLARITY,
	WP_RM_POLL_RING_DETECT,
	WP_RM_POLL_READ,
	WP_RM_POLL_WRITE,
	WP_RM_POLL_RXSIG_OFFHOOK,
	WP_RM_POLL_RXSIG_ONHOOK
};

#define WP_RM_POLL_DECODE(type)							\
	((type) == WP_RM_POLL_TONE_DIAL) ? "Tone (dial)":			\
	((type) == WP_RM_POLL_TONE_BUSY) ? "Tone (busy)":			\
	((type) == WP_RM_POLL_TONE_RING) ? "Tone (ring)":			\
	((type) == WP_RM_POLL_TONE_CONGESTION) ? "Tone (congestion)":		\
	((type) == WP_RM_POLL_TONE_DONE) ? "Tone (done)":			\
	((type) == WP_RM_POLL_TDMV) ? "TDMV":					\
	((type) == WP_RM_POLL_EVENT) ? "RM-Event":				\
	((type) == WP_RM_POLL_INIT) ? "Init":					\
	((type) == WP_RM_POLL_POWER) ? "Power":					\
	((type) == WP_RM_POLL_LC) ? "Loop closure":				\
	((type) == WP_RM_POLL_RING_TRIP) ? "Ring Trip":				\
	((type) == WP_RM_POLL_DTMF) ? "DTMF":					\
	((type) == WP_RM_POLL_RING) ? "Ring":					\
	((type) == WP_RM_POLL_TONE) ? "Tone":					\
	((type) == WP_RM_POLL_TXSIG_KEWL) ? "TX Sig KEWL":			\
	((type) == WP_RM_POLL_TXSIG_START) ? "TX Sig Start":			\
	((type) == WP_RM_POLL_TXSIG_OFFHOOK) ? "TX Sig Off-hook":		\
	((type) == WP_RM_POLL_TXSIG_ONHOOK) ? "TX Sig On-hook":			\
	((type) == WP_RM_POLL_ONHOOKTRANSFER) ? "On-hook transfer":		\
	((type) == WP_RM_POLL_SETPOLARITY) ? "Set polarity":			\
	((type) == WP_RM_POLL_RING_DETECT) ? "Ring Detect":			\
	((type) == WP_RM_POLL_READ) ? "FE Read":				\
	((type) == WP_RM_POLL_WRITE) ? "FE Write":				\
	((type) == WP_RM_POLL_RXSIG_OFFHOOK) ? "RX Sig Off-hook":		\
	((type) == WP_RM_POLL_RXSIG_ONHOOK) ? "RX Sig On-hook":			\
						"Unknown RM poll event"


/* tone_struct DialTone
** OSC1= 350 Hz OSC2= 440 Hz .0975 Volts -18 dBm */
#define	DIALTONE_IR13	0x7b30
#define	DIALTONE_IR14	0x0063	
#define	DIALTONE_IR16	0x7870	
#define	DIALTONE_IR17	0x007d	
#define	DIALTONE_DR32	6	
#define	DIALTONE_DR33	6	
#define	DIALTONE_DR36	0	
#define	DIALTONE_DR37	0	
#define	DIALTONE_DR38	0	
#define	DIALTONE_DR39	0	
#define	DIALTONE_DR40	0	
#define	DIALTONE_DR41	0	
#define	DIALTONE_DR42	0	
#define	DIALTONE_DR43	0	

/* tone_struct BusySignal
** OSC1= 480  OSC2 = 620 .0975 Voltz -18 dBm 8 */
#define	BUSYTONE_IR13	0x7700
#define	BUSYTONE_IR14	0x0089	
#define	BUSYTONE_IR16	0x7120	
#define	BUSYTONE_IR17	0x00B2	
#define	BUSYTONE_DR32	0x1E	
#define	BUSYTONE_DR33	0x1E	
#define	BUSYTONE_DR36	0xa0	
#define	BUSYTONE_DR37	0x0f	
#define	BUSYTONE_DR38	0xa0	
#define	BUSYTONE_DR39	0x0f	
#define	BUSYTONE_DR40	0xa0	
#define	BUSYTONE_DR41	0x0f	
#define	BUSYTONE_DR42	0xa0	
#define	BUSYTONE_DR43	0x0f	

/* tone_struct RingbackNormal
** OSC1 = 440 Hz OSC2 = 480 .0975 Volts -18 dBm */
#define	RINGBACKTONE_IR13	0x7870	
#define	RINGBACKTONE_IR14	0x007D	
#define	RINGBACKTONE_IR16	0x7700	
#define	RINGBACKTONE_IR17	0x0089	
#define	RINGBACKTONE_DR32	0x1E	
#define	RINGBACKTONE_DR33	0x1E	
#define	RINGBACKTONE_DR36	0x80	
#define	RINGBACKTONE_DR37	0x3E	
#define	RINGBACKTONE_DR38	0x0	
#define	RINGBACKTONE_DR39	0x7d	
#define	RINGBACKTONE_DR40	0x80	
#define	RINGBACKTONE_DR41	0x3E	
#define	RINGBACKTONE_DR42	0x0	
#define	RINGBACKTONE_DR43	0x7d

/* tone_struct CongestionTone
** OSC1= 480 Hz OSC2 = 620 .0975 Volts -18 dBM */
#define	CONGESTIONTONE_IR13	0x7700
#define	CONGESTIONTONE_IR14	0x0089	
#define	CONGESTIONTONE_IR16	0x7120	
#define	CONGESTIONTONE_IR17	0x00B2	
#define	CONGESTIONTONE_DR32	0x1E	
#define	CONGESTIONTONE_DR33	0x1E	
#define	CONGESTIONTONE_DR36	0x40	
#define	CONGESTIONTONE_DR37	0x06	
#define	CONGESTIONTONE_DR38	0x60	
#define	CONGESTIONTONE_DR39	0x09	
#define	CONGESTIONTONE_DR40	0x40	
#define	CONGESTIONTONE_DR41	0x06	
#define	CONGESTIONTONE_DR42	0x60	
#define	CONGESTIONTONE_DR43	0x09	

#define WP_RM_CHUNKSIZE		8

#define RING_DEBOUNCE           16     /* Ringer Debounce (64 ms) */
#define DEFAULT_BATT_DEBOUNCE   16     /* Battery debounce (64 ms) */
#define POLARITY_DEBOUNCE       16     /* Polarity debounce (64 ms) */
#define DEFAULT_BATT_THRESH     3      /* Anything under this is "no battery" */

#define OHT_TIMER		6000	/* How long after RING to retain OHT */

#define MAX_ALARMS		10

#define WP_RM_WATCHDOG_TIMEOUT	100

static alpha  indirect_regs[] =
{
{0,255,"DTMF_ROW_0_PEAK",0x55C2},
{1,255,"DTMF_ROW_1_PEAK",0x51E6},
{2,255,"DTMF_ROW2_PEAK",0x4B85},
{3,255,"DTMF_ROW3_PEAK",0x4937},
{4,255,"DTMF_COL1_PEAK",0x3333},
{5,255,"DTMF_FWD_TWIST",0x0202},
{6,255,"DTMF_RVS_TWIST",0x0202},
{7,255,"DTMF_ROW_RATIO_TRES",0x0198},
{8,255,"DTMF_COL_RATIO_TRES",0x0198},
{9,255,"DTMF_ROW_2ND_ARM",0x0611},
{10,255,"DTMF_COL_2ND_ARM",0x0202},
{11,255,"DTMF_PWR_MIN_TRES",0x00E5},
{12,255,"DTMF_OT_LIM_TRES",0x0A1C},
{13,0,"OSC1_COEF",0x7B30},
{14,1,"OSC1X",0x0063},
{15,2,"OSC1Y",0x0000},
{16,3,"OSC2_COEF",0x7870},
{17,4,"OSC2X",0x007D},
{18,5,"OSC2Y",0x0000},
{19,6,"RING_V_OFF",0x0000},
{20,7,"RING_OSC",0x7EF0},
{21,8,"RING_X",0x0160},
{22,9,"RING_Y",0x0000},
{23,255,"PULSE_ENVEL",0x2000},
{24,255,"PULSE_X",0x2000},
{25,255,"PULSE_Y",0x0000},
/*{26,13,"RECV_DIGITAL_GAIN",0x4000},*/	/* playback volume set lower */
{26,13,"RECV_DIGITAL_GAIN",0x2000},	/* playback volume set lower */
{27,14,"XMIT_DIGITAL_GAIN",0x4000},
/*{27,14,"XMIT_DIGITAL_GAIN",0x2000}, */
{28,15,"LOOP_CLOSE_TRES",0x1000},
{29,16,"RING_TRIP_TRES",0x3600},
{30,17,"COMMON_MIN_TRES",0x1000},
{31,18,"COMMON_MAX_TRES",0x0200},
{32,19,"PWR_ALARM_Q1Q2",0x07C0},
{33,20,"PWR_ALARM_Q3Q4",0x2600},
{34,21,"PWR_ALARM_Q5Q6",0x1B80},
{35,22,"LOOP_CLOSURE_FILTER",0x8000},
{36,23,"RING_TRIP_FILTER",0x0320},
{37,24,"TERM_LP_POLE_Q1Q2",0x008C},
{38,25,"TERM_LP_POLE_Q3Q4",0x0100},
{39,26,"TERM_LP_POLE_Q5Q6",0x0010},
{40,27,"CM_BIAS_RINGING",0x0C00},
{41,64,"DCDC_MIN_V",0x0C00},
{42,255,"DCDC_XTRA",0x1000},
{43,66,"LOOP_CLOSE_TRES_LOW",0x1000},
};

static struct fxo_mode {
	char *name;
	/* FXO */
	int ohs;
	int ohs2;
	int rz;
	int rt;
	int ilim;
	int dcv;
	int mini;
	int acim;
	int ring_osc;
	int ring_x;
} fxo_modes[] =
{
	{ "FCC", 0, 0, 0, 1, 0, 0x3, 0, 0, }, 	/* US, Canada */
	{ "TBR21", 0, 0, 0, 0, 1, 0x3, 0, 0x2, 0x7e6c, 0x023a, },
		/* Austria, Belgium, Denmark, Finland, France, Germany, 
		   Greece, Iceland, Ireland, Italy, Luxembourg, Netherlands,
		   Norway, Portugal, Spain, Sweden, Switzerland, and UK */
	{ "ARGENTINA", 0, 0, 0, 0, 0, 0x3, 0, 0, },
	{ "AUSTRALIA", 1, 0, 0, 0, 0, 0, 0x3, 0x3, },
	{ "AUSTRIA", 0, 1, 0, 0, 1, 0x3, 0, 0x3, },
	{ "BAHRAIN", 0, 0, 0, 0, 1, 0x3, 0, 0x2, },
	{ "BELGIUM", 0, 1, 0, 0, 1, 0x3, 0, 0x2, },
	{ "BRAZIL", 0, 0, 0, 0, 0, 0, 0x3, 0, },
	{ "BULGARIA", 0, 0, 0, 0, 1, 0x3, 0x0, 0x3, },
	{ "CANADA", 0, 0, 0, 0, 0, 0x3, 0, 0, },
	{ "CHILE", 0, 0, 0, 0, 0, 0x3, 0, 0, },
	{ "CHINA", 0, 0, 0, 0, 0, 0, 0x3, 0xf, },
	{ "COLUMBIA", 0, 0, 0, 0, 0, 0x3, 0, 0, },
	{ "CROATIA", 0, 0, 0, 0, 1, 0x3, 0, 0x2, },
	{ "CYPRUS", 0, 0, 0, 0, 1, 0x3, 0, 0x2, },
	{ "CZECH", 0, 0, 0, 0, 1, 0x3, 0, 0x2, },
	{ "DENMARK", 0, 1, 0, 0, 1, 0x3, 0, 0x2, },
	{ "ECUADOR", 0, 0, 0, 0, 0, 0x3, 0, 0, },
	{ "EGYPT", 0, 0, 0, 0, 0, 0, 0x3, 0, },
	{ "ELSALVADOR", 0, 0, 0, 0, 0, 0x3, 0, 0, },
	{ "FINLAND", 0, 1, 0, 0, 1, 0x3, 0, 0x2, },
	{ "FRANCE", 0, 1, 0, 0, 1, 0x3, 0, 0x2, },
	{ "GERMANY", 0, 1, 0, 0, 1, 0x3, 0, 0x3, },
	{ "GREECE", 0, 1, 0, 0, 1, 0x3, 0, 0x2, },
	{ "GUAM", 0, 0, 0, 0, 0, 0x3, 0, 0, },
	{ "HONGKONG", 0, 0, 0, 0, 0, 0x3, 0, 0, },
	{ "HUNGARY", 0, 0, 0, 0, 0, 0x3, 0, 0, },
	{ "ICELAND", 0, 1, 0, 0, 1, 0x3, 0, 0x2, },
	{ "INDIA", 0, 0, 0, 0, 0, 0x3, 0, 0x4, },
	{ "INDONESIA", 0, 0, 0, 0, 0, 0x3, 0, 0, },
	{ "IRELAND", 0, 1, 0, 0, 1, 0x3, 0, 0x2, },
	{ "ISRAEL", 0, 0, 0, 0, 1, 0x3, 0, 0x2, },
	{ "ITALY", 0, 1, 0, 0, 1, 0x3, 0, 0x2, },
	{ "JAPAN", 0, 0, 0, 0, 0, 0, 0x3, 0, },
	{ "JORDAN", 0, 0, 0, 0, 0, 0, 0x3, 0, },
	{ "KAZAKHSTAN", 0, 0, 0, 0, 0, 0x3, 0, },
	{ "KUWAIT", 0, 0, 0, 0, 0, 0x3, 0, 0, },
	{ "LATVIA", 0, 0, 0, 0, 1, 0x3, 0, 0x2, },
	{ "LEBANON", 0, 0, 0, 0, 1, 0x3, 0, 0x2, },
	{ "LUXEMBOURG", 0, 1, 0, 0, 1, 0x3, 0, 0x2, },
	{ "MACAO", 0, 0, 0, 0, 0, 0x3, 0, 0, },
	{ "MALAYSIA", 0, 0, 0, 0, 0, 0, 0x3, 0, },	/* Current loop >= 20ma */
	{ "MALTA", 0, 0, 0, 0, 1, 0x3, 0, 0x2, },
	{ "MEXICO", 0, 0, 0, 0, 0, 0x3, 0, 0, },
	{ "MOROCCO", 0, 0, 0, 0, 1, 0x3, 0, 0x2, },
	{ "NETHERLANDS", 0, 1, 0, 0, 1, 0x3, 0, 0x2, },
	{ "NEWZEALAND", 0, 0, 0, 0, 0, 0x3, 0, 0x4, },
	{ "NIGERIA", 0, 0, 0, 0, 0x1, 0x3, 0, 0x2, },
	{ "NORWAY", 0, 1, 0, 0, 1, 0x3, 0, 0x2, },
	{ "OMAN", 0, 0, 0, 0, 0, 0, 0x3, 0, },
	{ "PAKISTAN", 0, 0, 0, 0, 0, 0, 0x3, 0, },
	{ "PERU", 0, 0, 0, 0, 0, 0x3, 0, 0, },
	{ "PHILIPPINES", 0, 0, 0, 0, 0, 0, 0x3, 0, },
	{ "POLAND", 0, 0, 1, 1, 0, 0x3, 0, 0, },
	{ "PORTUGAL", 0, 1, 0, 0, 1, 0x3, 0, 0x2, },
	{ "ROMANIA", 0, 0, 0, 0, 0, 3, 0, 0, },
	{ "RUSSIA", 0, 0, 0, 0, 0, 0, 0x3, 0, },
	{ "SAUDIARABIA", 0, 0, 0, 0, 0, 0x3, 0, 0, },
	{ "SINGAPORE", 0, 0, 0, 0, 0, 0x3, 0, 0, },
	{ "SLOVAKIA", 0, 0, 0, 0, 0, 0x3, 0, 0x3, },
	{ "SLOVENIA", 0, 0, 0, 0, 0, 0x3, 0, 0x2, },
	{ "SOUTHAFRICA", 1, 0, 1, 0, 0, 0x3, 0, 0x3, },
	{ "SOUTHKOREA", 0, 0, 0, 0, 0, 0x3, 0, 0, },
	{ "SPAIN", 0, 1, 0, 0, 1, 0x3, 0, 0x2, },
	{ "SWEDEN", 0, 1, 0, 0, 1, 0x3, 0, 0x2, },
	{ "SWITZERLAND", 0, 1, 0, 0, 1, 0x3, 0, 0x2, },
	{ "SYRIA", 0, 0, 0, 0, 0, 0, 0x3, 0, },
	{ "TAIWAN", 0, 0, 0, 0, 0, 0, 0x3, 0, },
	{ "THAILAND", 0, 0, 0, 0, 0, 0, 0x3, 0, },
	{ "UAE", 0, 0, 0, 0, 0, 0x3, 0, 0, },
	{ "UK", 0, 1, 0, 0, 1, 0x3, 0, 0x5, },
	{ "USA", 0, 0, 0, 0, 0, 0x3, 0, 0, },
	{ "YEMEN", 0, 0, 0, 0, 0, 0x3, 0, 0, },
};
static int acim2tiss[16] = { 0x0, 0x1, 0x4, 0x5, 0x7, 0x0, 0x0, 0x6, 0x0, 0x0, 0x0, 0x2, 0x0, 0x3 };

/*******************************************************************************
**			STRUCTURES AND TYPEDEFS
*******************************************************************************/

/*******************************************************************************
**			   GLOBAL VARIABLES
*******************************************************************************/
extern int gl_usb_rw_fast;
#if !defined(__WINDOWS__)
extern WAN_LIST_HEAD(, wan_tdmv_) wan_tdmv_head;
#endif
static int battdebounce = 64; //DEFAULT_BATT_DEBOUNCE;
static int battthresh = DEFAULT_BATT_THRESH;

/*******************************************************************************
**			  FUNCTION PROTOTYPES
*******************************************************************************/
int wp_usb_init_proslic(sdla_fe_t *fe, int mod_no, int fast, int sane);
int wp_usb_init_voicedaa(sdla_fe_t *fe, int mod_no, int fast, int sane);

static int wp_usb_remora_config(void *pfe);
static int wp_usb_remora_unconfig(void *pfe);
static int wp_usb_remora_post_init(void *pfe);
static int wp_usb_remora_post_unconfig(void* pfe);
static int wp_usb_remora_if_config(void *pfe, u32 mod_map, u8);
static int wp_usb_remora_if_unconfig(void *pfe, u32 mod_map, u8);
static int wp_usb_remora_disable_irq(void *pfe); 
static int wp_usb_remora_intr(sdla_fe_t *); 
static int wp_usb_remora_check_intr(sdla_fe_t *); 
static int wp_usb_remora_polling(sdla_fe_t*);
static int wp_usb_remora_udp(sdla_fe_t*, void*, unsigned char*);
static unsigned int wp_usb_remora_active_map(sdla_fe_t* fe, unsigned char);
static unsigned char wp_usb_remora_fe_media(sdla_fe_t *fe);
static int wp_usb_remora_set_dtmf(sdla_fe_t*, int, unsigned char);
static int wp_usb_remora_intr_ctrl(sdla_fe_t*, int, u_int8_t, u_int8_t, unsigned int);
static int wp_usb_remora_event_ctrl(sdla_fe_t*, wan_event_ctrl_t*);

static int wp_usb_remora_add_timer(sdla_fe_t*, unsigned long);
static int wp_usb_remora_add_event(sdla_fe_t*, sdla_fe_timer_event_t*);

#if defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__)
static void wp_usb_remora_timer(void*);
#elif defined(__WINDOWS__)
static void wp_usb_remora_timer(IN PKDPC,void*,void*,void*);
#else
static void wp_usb_remora_timer(unsigned long);
#endif

static int wp_usb_remora_dialtone(sdla_fe_t*, int);
static int wp_usb_remora_busytone(sdla_fe_t*, int);
static int wp_usb_remora_ringtone(sdla_fe_t*, int);
static int wp_usb_remora_congestiontone(sdla_fe_t*, int);
static int wp_usb_remora_disabletone(sdla_fe_t*, int);
static int wp_usb_remora_get_link_status(sdla_fe_t *fe, unsigned char *status,int mod_no);

#if defined(AFT_TDM_API_SUPPORT)
static int wp_usb_remora_watchdog(void *card);
static void wp_usb_remora_voicedaa_check_hook(sdla_fe_t *fe, int mod_no);
#endif

extern int sdla_usb_read_poll(void *phw, unsigned char off, unsigned char *data);
extern int sdla_usb_write_poll(void *phw, unsigned char off, unsigned char data);

/*******************************************************************************
**			  FUNCTION DEFINITIONS
*******************************************************************************/

static void wait_just_a_bit(int foo, int fast)
{

#if defined(__FreeBSD__) || defined(__OpenBSD__)
	WP_SCHEDULE(foo, "A-USB");
#else
	wan_ticks_t	start_ticks;
	start_ticks = SYSTEM_TICKS + foo;
	while(SYSTEM_TICKS < start_ticks){
		WP_DELAY(100);
# if defined(__LINUX__)
		if (!fast) WP_SCHEDULE(foo, "A-USB");
# endif
	}
#endif
}

static int
wp_proslic_setreg_indirect(sdla_fe_t *fe, int mod_no, unsigned char address, unsigned short data)
{
	DEBUG_REG("%s: Indirect Register %d = %X\n",
				fe->name, address, data);
	WRITE_USB_RM_REG(mod_no, IDA_LO,(unsigned char)(data & 0xFF));
	WRITE_USB_RM_REG(mod_no, IDA_HI,(unsigned char)((data & 0xFF00)>>8));
	WRITE_USB_RM_REG(mod_no, IAA,address);
	return 0;
}

static int
wp_proslic_getreg_indirect(sdla_fe_t *fe, int mod_no, unsigned char address)
{ 
	int res = -1;
	unsigned char data1, data2;

	WRITE_USB_RM_REG(mod_no, IAA, address);
	data1 = READ_USB_RM_REG(mod_no, IDA_LO);
	data2 = READ_USB_RM_REG(mod_no, IDA_HI);
	res = data1 | (data2 << 8);
	return res;
}

static int wp_proslic_init_indirect_regs(sdla_fe_t *fe, int mod_no)
{
	unsigned char i;

	DEBUG_CFG("%s: Initializing Indirect Registers...\n", fe->name);
	for (i=0; i<sizeof(indirect_regs) / sizeof(indirect_regs[0]); i++){
		if(wp_proslic_setreg_indirect(fe, mod_no, indirect_regs[i].address,indirect_regs[i].initial))
			return -1;
	}
	DEBUG_CFG("%s: Initializing Indirect Registers...Done!\n", fe->name);

	return 0;
}

static int wp_proslic_verify_indirect_regs(sdla_fe_t *fe, int mod_no)
{ 
	int passed = 1;
	unsigned short i, initial;
	int j;

	DEBUG_CFG("%s: Verifing Indirect Registers...\n", fe->name);
	for (i=0; i<sizeof(indirect_regs) / sizeof(indirect_regs[0]); i++){ 
		
		j = wp_proslic_getreg_indirect(
				fe,
				mod_no,
				(unsigned char) indirect_regs[i].address);
		if (j < 0){
			DEBUG_EVENT("%s: Module %d: Failed to read indirect register %d\n",
						fe->name, mod_no+1,
						i);
			return -1;
		}
		initial= indirect_regs[i].initial;

		if ( j != initial && indirect_regs[i].altaddr != 255){
			DEBUG_EVENT(
			"%s: Module %d: Internal Error: iReg=%s (%X) Value=%X (%X)\n",
						fe->name, mod_no+1,
						indirect_regs[i].name,
						indirect_regs[i].address,
						j, initial);
			 passed = 0;
		}	
	}

	if (!passed) {
		DEBUG_EVENT(
		"%s: Module %d: Init Indirect Registers UNSUCCESSFULLY.\n",
						fe->name, mod_no+1);
		return -1;
	}
	DEBUG_CFG("%s: Verifing Indirect Registers...Done!\n", fe->name);
	return 0;
}

/******************************************************************************
** wp_usb_remora_module_detect() - 
**
**	OK
*/
static int wp_usb_remora_module_detect(sdla_fe_t *fe)
{
	int		mod_no;
	unsigned char	byte;

	for(mod_no = 0;mod_no < MAX_USB_REMORA_MODULES; mod_no ++){
		WRITE_USB_RM_REG(mod_no,1,0x80);
		byte = READ_USB_RM_REG(mod_no,2);
		if (byte == 0x03){
			DEBUG_RM("%s: Module %d FXO\n",
					fe->name, mod_no+1);
			fe->rm_param.mod[mod_no].type	=
						MOD_TYPE_FXO;
		}else{
			DEBUG_RM("%s: Module %d - Not detected!\n",
					fe->name, mod_no+1);
			fe->rm_param.mod[mod_no].type	=
						MOD_TYPE_NONE;
		}
	}
	return 0;
}

static int wp_usb_init_proslic_insane(sdla_fe_t *fe, int mod_no)
{
	unsigned char	value;

	value = READ_USB_RM_REG(mod_no, 0);
	if ((value & 0x30) >> 4){
		DEBUG_RM("%s: Proslic on module %d is not a Si3210 (%02X)!\n",
					fe->name,
					mod_no+1,
					value);
		return -1;
	}
	if (((value & 0x0F) == 0) || ((value & 0x0F) == 0x0F)){
		DEBUG_RM("%s: Proslic is not loaded!\n",
					fe->name);
		return -1;
	}
	if ((value & 0x0F) < 2){
		DEBUG_EVENT("%s: Proslic 3210 version %d is too old!\n",
					fe->name,
					value & 0x0F);
		return -1;
	}

	value = READ_USB_RM_REG(mod_no, 8);
	if (value != 0x2) {
		DEBUG_EVENT(
		"%s: Proslic on module %d insane (1) %d should be 2!\n",
					fe->name, mod_no+1, value);
		return -1;
	}

	value = READ_USB_RM_REG(mod_no, 64);
	if (value != 0x0) {
		DEBUG_EVENT(
		"%s: Proslic on modyle %d insane (2) %d should be 0!\n",
					fe->name, mod_no+1, value);
		return -1;
	} 

	value = READ_USB_RM_REG(mod_no, 11);
	if (value != 0x33) {
		DEBUG_EVENT(
		"%s: Proslic on module %d insane (3) %02X should be 0x33\n",
					fe->name, mod_no+1, value);
		return -1;
	}
	WRITE_USB_RM_REG(mod_no, 30, 0);
	return 0;
}

static int wp_powerup_proslic(sdla_fe_t *fe, int mod_no, int fast)
{
	wp_remora_fxs_t	*fxs;
	wan_ticks_t	start_ticks;
	int		loopcurrent = 20, lim;
	unsigned char	vbat;
	
	DEBUG_CFG("%s: PowerUp SLIC initialization...\n", fe->name);
	fxs = &fe->rm_param.mod[mod_no].u.fxs;
	/* set the period of the DC-DC converter to 1/64 kHz  START OUT SLOW*/
	WRITE_USB_RM_REG(mod_no, 92, 0xf5);

	if (fast) return 0;

	/* powerup */ 
	//WRITE_USB_RM_REG(mod_no, 93, 0x1F);
	WRITE_USB_RM_REG(mod_no, 14, 0x0);	/* DIFF DEMO 0x10 */

	start_ticks = SYSTEM_TICKS;
	while((vbat = READ_USB_RM_REG(mod_no, 82)) < 0xC0){
		/* Wait no more than 500ms */
		if ((SYSTEM_TICKS - start_ticks) > HZ/2){
			break;
		}
		wait_just_a_bit(HZ/10, fast);
	}

	if (vbat < 0xc0){
		if (fxs->proslic_power == PROSLIC_POWER_UNKNOWN){
			DEBUG_EVENT(
			"%s: Module %d: Failed to powerup within %d ms (%dV : %dV)!\n",
					fe->name,
					mod_no+1,
					(u_int32_t)(((SYSTEM_TICKS - start_ticks) * 1000 / HZ)),
					(vbat * 375)/1000, (0xc0 * 375)/1000);
			DEBUG_EVENT(
			"%s: Module %d: Did you remember to plug in the power cable?\n",
					fe->name,
					mod_no+1);

		}
		fxs->proslic_power = PROSLIC_POWER_WARNED;
		return -1;
	}
	fxs->proslic_power = PROSLIC_POWER_ON;
	DEBUG_RM("%s: Module %d: Current Battery1 %dV, Battery2 %dV\n",
					fe->name, mod_no+1,
					READ_USB_RM_REG(mod_no, 82)*375/1000,
					READ_USB_RM_REG(mod_no, 83)*375/1000);

        /* Proslic max allowed loop current, reg 71 LOOP_I_LIMIT */
        /* If out of range, just set it to the default value     */
        lim = (loopcurrent - 20) / 3;
        if ( loopcurrent > 41 ) {
                lim = 0;
		DEBUG_EVENT(
		"%s: Module %d: Loop current out of range (default 20mA)!\n",
					fe->name, mod_no+1);
        }else{
		DEBUG_RM("%s: Loop current set to %dmA!\n",
					fe->name,
					(lim*3)+20);
	}
        WRITE_USB_RM_REG(mod_no, 71,lim);

	WRITE_USB_RM_REG(mod_no, 93, 0x99);  /* DC-DC Calibration  */
	/* Wait for DC-DC Calibration to complete */
	start_ticks = SYSTEM_TICKS;
	while(0x80 & READ_USB_RM_REG(mod_no, 93)){
		if ((SYSTEM_TICKS - start_ticks) > 2*HZ){
			DEBUG_EVENT(
			"%s: Module %d: Timeout waiting for DC-DC calibration (%02X)\n",
						fe->name, mod_no+1,
						READ_USB_RM_REG(mod_no, 93));
			return -EINVAL;
		}
		wait_just_a_bit(HZ/10, fast);
	}
	DEBUG_CFG("%s: PowerUp SLIC initialization...Done!\n", fe->name);
	return 0;
}

static int wp_proslic_powerleak_test(sdla_fe_t *fe, int mod_no, int fast)
{
	wan_ticks_t	start_ticks;
	unsigned char	vbat;

	DEBUG_CFG("%s: PowerLeak ProSLIC testing...\n", fe->name);
	/* powerleak */ 
	WRITE_USB_RM_REG(mod_no, 64, 0);
	WRITE_USB_RM_REG(mod_no, 14, 0x10);
	/* wait for 1 s */
	start_ticks = SYSTEM_TICKS;
	wait_just_a_bit(HZ, fast);
	vbat = READ_USB_RM_REG(mod_no, 82);
	if (vbat < 0x6){
		DEBUG_EVENT(
		"%s: Module %d: Excessive leakage detected: %d volts (%02x) after %d ms\n",
					fe->name, mod_no+1,
					376 * vbat / 1000,
					vbat,
					(u_int32_t)((SYSTEM_TICKS - start_ticks) * 1000 / HZ));
		return -1;
	}
	DEBUG_RM("%s: Module %d: Post-leakage voltage: %d volts\n",
					fe->name,
					mod_no+1,
					376 * vbat / 1000);

	DEBUG_CFG("%s: PowerLeak ProSLIC testing...Done!\n", fe->name);
	return 0;
}

#if defined(WAN_REMORA_AUTO_CALIBRATE)
static int wp_proslic_calibrate(sdla_fe_t *fe, int mod_no, int fast)
{
	volatile wan_ticks_t	start_ticks;

	DEBUG_CFG("%s: ProSLIC calibration...\n", fe->name);
	/* perform all calibration */
	WRITE_USB_RM_REG(mod_no, 97, 0x1f);
	/* start */
	WRITE_USB_RM_REG(mod_no, 96, 0x5f);

	start_ticks = SYSTEM_TICKS;
	while(READ_USB_RM_REG(mod_no, 96)){
		if ((SYSTEM_TICKS - start_ticks) > 2*HZ){
			DEBUG_EVENT(
			"%s: Module %d: Timeout on module calibration!\n",
					fe->name, mod_no+1);
			return -1;
		}
		wait_just_a_bit(HZ/10, fast);
	}
	DEBUG_CFG("%s: ProSLIC calibration...Done!\n", fe->name);
	return 0;
}
#else
static int wp_proslic_manual_calibrate(sdla_fe_t *fe, int mod_no, int fast)
{
	volatile wan_ticks_t	start_ticks;
	int			i=0;

	DEBUG_CFG("%s: ProSLIC manual calibration...\n", fe->name);
	WRITE_USB_RM_REG(mod_no, 21, 0x00);
	WRITE_USB_RM_REG(mod_no, 22, 0x00);
	WRITE_USB_RM_REG(mod_no, 23, 0x00);
	WRITE_USB_RM_REG(mod_no, 64, 0x00);

	/* Step 14 */
	WRITE_USB_RM_REG(mod_no, 97, 0x18);
	WRITE_USB_RM_REG(mod_no, 96, 0x47);

	/* Step 15 */
	start_ticks = SYSTEM_TICKS;
	while(READ_USB_RM_REG(mod_no, 96) != 0){
		if ((SYSTEM_TICKS - start_ticks) > 800){
			DEBUG_EVENT(
			"%s: Module %d: Timeout on SLIC calibration (15)!\n",
					fe->name, mod_no+1);
			return -1;
		}
		wait_just_a_bit(HZ/10, fast);
	}

	wait_just_a_bit(HZ/10, fast);
	wp_proslic_setreg_indirect(fe, mod_no, 88, 0x00);
	wp_proslic_setreg_indirect(fe, mod_no, 89, 0x00);
	wp_proslic_setreg_indirect(fe, mod_no, 90, 0x00);
	wp_proslic_setreg_indirect(fe, mod_no, 91, 0x00);
	wp_proslic_setreg_indirect(fe, mod_no, 92, 0x00);
	wp_proslic_setreg_indirect(fe, mod_no, 93, 0x00);

	/* Step 16 */
	/* Insert manual calibration for sangoma Si3210 */
	WRITE_USB_RM_REG(mod_no, 98, 0x10);
	WRITE_USB_RM_REG(mod_no, 99, 0x10);

	for (i = 0x1f; i > 0; i--){
		WRITE_USB_RM_REG(mod_no, 98, i);
		wait_just_a_bit(4, fast);
		if ((READ_USB_RM_REG(mod_no, 88)) == 0){
			break;
		}
	}
	for (i = 0x1f; i > 0; i--){
		WRITE_USB_RM_REG(mod_no, 99, i);
		wait_just_a_bit(4, fast);
		if ((READ_USB_RM_REG(mod_no, 89)) == 0){
			break;
		}
	}
	WRITE_USB_RM_REG(mod_no, 64, 0x01);
	wait_just_a_bit(HZ, fast);
	WRITE_USB_RM_REG(mod_no, 64, 0x00);
	/* Step 17 */
	WRITE_USB_RM_REG(mod_no, 23, 0x04);

	/* Step 18 */
	/* DAC offset and without common mode calibration. */
	WRITE_USB_RM_REG(mod_no, 97, 0x01);	/* Manual after */
	/* Calibrate common mode and differential DAC mode DAC + ILIM */
	WRITE_USB_RM_REG(mod_no, 96, 0x40);

	/* Step 19 */
	wait_just_a_bit(HZ*2, fast);
	start_ticks = SYSTEM_TICKS;
	while(READ_USB_RM_REG(mod_no, 96) != 0){
		if ((SYSTEM_TICKS - start_ticks) > 400){
			DEBUG_EVENT(
			"%s: Module %d: Timeout on SLIC calibration (%ld:%ld)!\n",
				fe->name, mod_no+1,
				(unsigned long)start_ticks,
				(unsigned long)SYSTEM_TICKS);
			return -1;
		}
		wait_just_a_bit(HZ/10, fast);
	}
	DEBUG_RM("%s: Module %d: Calibration is done\n",
				fe->name, mod_no+1);
	/*READ_USB_RM_REG(mod_no, 96);*/
	DEBUG_CFG("%s: ProSLIC manual calibration...Done!\n", fe->name);
	return 0;
}
#endif

/* static */
int wp_usb_init_proslic(sdla_fe_t *fe, int mod_no, int fast, int sane)
{
	unsigned short		tmp[5];
	unsigned char		value;
	volatile int		x;

	/* By default, don't send on hook */
	if (fe->fe_cfg.cfg.remora.reversepolarity){
		fe->rm_param.mod[mod_no].u.fxs.idletxhookstate = 5;
	}else{
		fe->rm_param.mod[mod_no].u.fxs.idletxhookstate = 1;
	}

	DEBUG_CFG("%s: Start ProSLIC configuration...\n", fe->name);
	/* Step 8 */
	if (!sane && wp_usb_init_proslic_insane(fe, mod_no)){
		return -2;
	}

	if (sane){
		WRITE_USB_RM_REG(mod_no, 14, 0x10);
	}

	if (!fast){
		fe->rm_param.mod[mod_no].u.fxs.proslic_power = PROSLIC_POWER_UNKNOWN;
	}

	/* Step 9 */
	if (wp_proslic_init_indirect_regs(fe, mod_no)) {
		DEBUG_EVENT(
		"%s: Module %d: Indirect Registers failed to initialize!\n",
							fe->name,
							mod_no+1);
		return -1;
	}
	wp_proslic_setreg_indirect(fe, mod_no, 97,0);

	/* Steo 10 */
	WRITE_USB_RM_REG(mod_no, 8, 0);		/*DIGIUM*/
	WRITE_USB_RM_REG(mod_no, 108, 0xeb);	/*DIGIUM*/
	WRITE_USB_RM_REG(mod_no, 67, 0x17);
	WRITE_USB_RM_REG(mod_no, 66, 1);

	/* Flush ProSLIC digital filters by setting to clear, while
	** saving old values */
	for (x=0;x<5;x++) {
		tmp[x] = (unsigned short)wp_proslic_getreg_indirect(fe, mod_no, x + 35);
		wp_proslic_setreg_indirect(fe, mod_no, x + 35, 0x8000);
	}

	/* Power up the DC-DC converter */
	if (wp_powerup_proslic(fe, mod_no, fast)) {
		DEBUG_EVENT(
		"%s: Module %d: Unable to do INITIAL ProSLIC powerup!\n",
					fe->name,
					mod_no+1);
		return -1;
	}

	if (!fast){

		if (wp_proslic_powerleak_test(fe, mod_no, fast)){
			DEBUG_EVENT(
			"%s: Module %d: Proslic failed leakge the short circuit\n",
						fe->name,
						mod_no+1);
		}

		/* Step 12 */
		if (wp_powerup_proslic(fe, mod_no, fast)) {
			DEBUG_EVENT(
			"%s: Module %d: Unable to do FINAL ProSLIC powerup!\n",
						fe->name,
						mod_no+1);
			return -1;
		}

		/* Step 13 */
		WRITE_USB_RM_REG(mod_no, 64, 0);

#if defined(WAN_REMORA_AUTO_CALIBRATE)
		if (wp_proslic_calibrate(fe, mod_no, fast)){
			return -1;
		}
#else
		if (wp_proslic_manual_calibrate(fe, mod_no, fast)){
			return -1;
		}
#endif
		/* Perform DC-DC calibration */
		WRITE_USB_RM_REG(mod_no,  93, 0x99);
		wait_just_a_bit(10, fast);
		value = READ_USB_RM_REG(mod_no, 107);
		if ((value < 0x2) || (value > 0xd)) {
			DEBUG_EVENT(
			"%s: Module %d: DC-DC calibration has a surprising direct 107 of 0x%02x!\n",
						fe->name,
						mod_no+1,
						value);
			WRITE_USB_RM_REG(mod_no,  107, 0x8);
		}

		/* Save calibration vectors */
		for (x=0;x<NUM_CAL_REGS;x++){
			fe->rm_param.mod[mod_no].u.fxs.callregs.vals[x] =
					READ_USB_RM_REG(mod_no, 96 + x);
		}

	}else{
		/* Restore calibration vectors */
		for (x=0;x<NUM_CAL_REGS;x++){
			WRITE_USB_RM_REG(mod_no, 96 + x, 
				fe->rm_param.mod[mod_no].u.fxs.callregs.vals[x]);
		}
	}    

	for (x=0;x<5;x++) {
		wp_proslic_setreg_indirect(fe, mod_no, x + 35, tmp[x]);
	}

	if (wp_proslic_verify_indirect_regs(fe, mod_no)) {
		DEBUG_EVENT(
		"%s: Module %d: Indirect Registers failed verification.\n",
					fe->name,
					mod_no+1);
		return -1;
	}

	if (fe->fe_cfg.tdmv_law == WAN_TDMV_ALAW){
		WRITE_USB_RM_REG(mod_no, 1, 0x20);
	}else if (fe->fe_cfg.tdmv_law == WAN_TDMV_MULAW){
		WRITE_USB_RM_REG(mod_no, 1, 0x28);
	}
	/* U-Law 8-bit interface */
	/* Tx Start count low byte  0 */
	WRITE_USB_RM_REG(mod_no, 2, mod_no * 8 + 1);
	/* Tx Start count high byte 0 */
	WRITE_USB_RM_REG(mod_no, 3, 0);
	/* Rx Start count low byte  0 */
	WRITE_USB_RM_REG(mod_no, 4, mod_no * 8 + 1);
	/* Rx Start count high byte 0 */
	WRITE_USB_RM_REG(mod_no, 5, 0);
	/* Clear all interrupt */
	WRITE_USB_RM_REG(mod_no, 18, 0xff);
	WRITE_USB_RM_REG(mod_no, 19, 0xff);
	WRITE_USB_RM_REG(mod_no, 20, 0xff);
	WRITE_USB_RM_REG(mod_no, 73, 0x04);

	if (!strcmp(fxo_modes[fe->fe_cfg.cfg.remora.opermode].name, "AUSTRALIA")) {
		value = (unsigned char)acim2tiss[fxo_modes[fe->fe_cfg.cfg.remora.opermode].acim];
		WRITE_USB_RM_REG(mod_no, 10, 0x8 | value);
		if (fxo_modes[fe->fe_cfg.cfg.remora.opermode].ring_osc){
			wp_proslic_setreg_indirect(
				fe, mod_no, 20,
				(unsigned char)fxo_modes[fe->fe_cfg.cfg.remora.opermode].ring_osc);
		}
		if (fxo_modes[fe->fe_cfg.cfg.remora.opermode].ring_x){
			wp_proslic_setreg_indirect(
				fe, mod_no, 21,
				(unsigned char)fxo_modes[fe->fe_cfg.cfg.remora.opermode].ring_x);
		}
	}

	/* lowpower */
	if (fe->fe_cfg.cfg.remora.fxs_lowpower == WANOPT_YES){
		WRITE_USB_RM_REG(mod_no, 72, 0x10);
	}

	if (fe->fe_cfg.cfg.remora.fxs_fastringer == WANOPT_YES){
		/* Speed up Ringer */
		wp_proslic_setreg_indirect(fe, mod_no, 20, 0x7e6d);
		wp_proslic_setreg_indirect(fe, mod_no, 21, 0x01b9);
		/* Beef up Ringing voltage to 89V */
		if (!strcmp(fxo_modes[fe->fe_cfg.cfg.remora.opermode].name, "AUSTRALIA")) {
			WRITE_USB_RM_REG(mod_no, 74, 0x3f);
			if (wp_proslic_setreg_indirect(fe, mod_no, 21, 0x247)){ 
				return -1;
			}
			DEBUG_EVENT("%s: Module %d: Boosting fast ringer (89V peak)\n",
					fe->name, mod_no + 1);
		} else if (fe->fe_cfg.cfg.remora.fxs_lowpower == WANOPT_YES){
			if (wp_proslic_setreg_indirect(fe, mod_no, 21, 0x14b)){ 
				return -1;
			}
			DEBUG_EVENT("%s: Module %d: Reducing fast ring power (50V peak)\n",
					fe->name, mod_no + 1);
		} else {
			DEBUG_EVENT("%s: Module %d: Speeding up ringer (25Hz)\n",
					fe->name, mod_no + 1);
		}
	}else{
		if (!strcmp(fxo_modes[fe->fe_cfg.cfg.remora.opermode].name, "AUSTRALIA")) {
			if (fe->fe_cfg.cfg.remora.fxs_ringampl){
				u16	ringx=0x00;
				u8	vbath = 0x00;
				switch(fe->fe_cfg.cfg.remora.fxs_ringampl){
				case 47: ringx = 0x163; vbath = 0x31; break;
				case 45: ringx = 0x154; vbath = 0x2f; break;
				case 40: ringx = 0x12e; vbath = 0x2b; break;
				case 35: ringx = 0x108; vbath = 0x26; break;
				case 30: ringx = 0xe2; vbath = 0x21; break;
				case 25: ringx = 0xbc; vbath = 0x1d; break;
				case 20: ringx = 0x97; vbath = 0x1b; break;
				case 15: ringx = 0x71; vbath = 0x13; break;
				case 10: ringx = 0x4b; vbath = 0x0e; break;
				}
				if (ringx && vbath){
					DEBUG_EVENT(
					"%s: Module %d: Ringing Amplitude %d (RNGX:%04X VBATH:%02X)\n",
							fe->name,mod_no+1, fe->fe_cfg.cfg.remora.fxs_ringampl, ringx, vbath);
					WRITE_USB_RM_REG(mod_no, 74, vbath);
					if (wp_proslic_setreg_indirect(fe, mod_no, 21, ringx)){
						DEBUG_EVENT(
						"%s: Module %d: Failed to set RingX value!\n", 
							fe->name, mod_no+1); 
					}
					 
				}else{
					DEBUG_EVENT(
					"%s: Module %d: Invalid Ringing Amplitude value %d\n",
							fe->name, mod_no+1, fe->fe_cfg.cfg.remora.fxs_ringampl); 
				}
			}else{
				WRITE_USB_RM_REG(mod_no, 74, 0x3f);
				if (wp_proslic_setreg_indirect(fe, mod_no, 21, 0x1d1)){
					return -1;
				} 
				DEBUG_EVENT("%s: Module %d: Boosting ringer (89V peak)\n",
						fe->name, mod_no+1);
			}
		} else if (fe->fe_cfg.cfg.remora.fxs_lowpower == WANOPT_YES){
			if (wp_proslic_setreg_indirect(fe, mod_no, 21, 0x108)){
				return -1;
			} 
			DEBUG_EVENT("%s: Module %d: Reducing ring power (50V peak)\n",
						fe->name, mod_no+1);
		}
	}

	/* Adjust RX/TX gains */
	if (fe->fe_cfg.cfg.remora.fxs_txgain || fe->fe_cfg.cfg.remora.fxs_rxgain) {
		DEBUG_EVENT("%s: Module %d: Adjust TX Gain to %s\n", 
					fe->name, mod_no+1,
					(fe->fe_cfg.cfg.remora.fxs_txgain == 35) ? "3.5dB":
					(fe->fe_cfg.cfg.remora.fxs_txgain == -35) ? "-3.5dB":"0dB");
		value = READ_USB_RM_REG(mod_no, 9);
		switch (fe->fe_cfg.cfg.remora.fxs_txgain) {
		case 35:
			value |= 0x8;
			break;
		case -35:
			value |= 0x4;
			break;
		case 0: 
			break;
		}
	
		DEBUG_EVENT("%s: Module %d: Adjust RX Gain to %s\n", 
					fe->name, mod_no+1,
					(fe->fe_cfg.cfg.remora.fxs_rxgain == 35) ? "3.5dB":
					(fe->fe_cfg.cfg.remora.fxs_rxgain == -35) ? "-3.5dB":"0dB");
		switch (fe->fe_cfg.cfg.remora.fxs_rxgain) {
		case 35:
			value |= 0x2;
			break;
		case -35:
			value |= 0x01;
			break;
		case 0:
			break;
		}
		WRITE_USB_RM_REG(mod_no, 9, value);
	}

	if (!fast){
		/* Disable interrupt while full initialization */
		WRITE_USB_RM_REG(mod_no, 21, 0);
		WRITE_USB_RM_REG(mod_no, 22, 0xFC);
		WRITE_USB_RM_REG(mod_no, 23, 0);
		
#if defined(AFT_RM_INTR_SUPPORT)
		fe->rm_param.mod[mod_no].u.fxs.imask1 = 0x00;
		fe->rm_param.mod[mod_no].u.fxs.imask2 = 0x03;
		fe->rm_param.mod[mod_no].u.fxs.imask3 = 0x01;
#else		
		fe->rm_param.mod[mod_no].u.fxs.imask1 = 0x00;
		fe->rm_param.mod[mod_no].u.fxs.imask2 = 0xFC;
		fe->rm_param.mod[mod_no].u.fxs.imask3 = 0x00;
#endif		
	}

#if 0					
	DEBUG_RM("%s: Module %d: Current Battery1 %dV, Battery2 %dV (%d)\n",
					fe->name, mod_no+1,
					READ_USB_RM_REG(mod_no, 82)*375/1000,
					READ_USB_RM_REG(mod_no, 83)*375/1000,
					__LINE__);

	/* verify TIP/RING voltage */
	if (!fast){
		WRITE_USB_RM_REG(mod_no, 8, 0x2);
		wait_just_a_bit(HZ, fast);
		start_ticks = SYSTEM_TICKS;
		while(READ_USB_RM_REG(mod_no, 81) < 0x75){
			if ((SYSTEM_TICKS - start_ticks) > HZ*10){
				break;	
			}
			wait_just_a_bit(HZ, fast);
		}
		wait_just_a_bit(HZ, fast);
		if (READ_USB_RM_REG(mod_no, 81) < 0x75){
			if (sane){
				DEBUG_EVENT(
				"%s: Module %d: TIP/RING is too low on FXS %d!\n",
						fe->name,
						mod_no,
						READ_USB_RM_REG(mod_no, 81) * 375 / 1000);
			}
			WRITE_USB_RM_REG(mod_no, 8, 0x0);
			return -1;
		}
		WRITE_USB_RM_REG(mod_no, 8, 0x0);
	}
#endif
	WRITE_USB_RM_REG(mod_no, 64, 0x1);

	wait_just_a_bit(HZ, fast);
	if (READ_USB_RM_REG(mod_no, 81) < 0x0A){
		DEBUG_EVENT(
		"%s: Module %d: TIP/RING is too low on FXS %d!\n",
				fe->name,
				mod_no,
				READ_USB_RM_REG(mod_no, 81) * 375 / 1000);
		return -1;
	}

	DEBUG_RM("%s: Module %d: Current Battery1 %dV, Battery2 %dV\n",
					fe->name, mod_no+1,
					READ_USB_RM_REG(mod_no, 82)*375/1000,
					READ_USB_RM_REG(mod_no, 83)*375/1000);
	DEBUG_CFG("%s: Start ProSLIC configuration...Done!\n", fe->name);
	return 0;
}

static int wp_usb_voicedaa_insane(sdla_fe_t *fe, int mod_no)
{
	unsigned char	byte;

	byte = READ_USB_RM_REG(mod_no, 2);
	if (byte != 0x3)
		return -2;
	byte = READ_USB_RM_REG(mod_no, 11);
	DEBUG_TEST("%s: Module %d: VoiceDAA System: %02x\n",
				fe->name,
				mod_no+1,
				byte & 0xf);
	return 0;
}


int wp_usb_init_voicedaa(sdla_fe_t *fe, int mod_no, int fast, int sane)
{
	unsigned char	reg16=0, reg26=0, reg30=0, reg31=0;
	wan_ticks_t	start_ticks;

	if (!sane && wp_usb_voicedaa_insane(fe, mod_no)){
		return -2;
	}

	/* Software reset */
	WRITE_USB_RM_REG(mod_no, 1, 0x80);

	/* Wait just a bit */
	if (fast) {
		WP_DELAY(10000);
	} else {
		wait_just_a_bit(HZ/10, fast);
	}

	/* Enable PCM, ulaw */
	if (fe->fe_cfg.tdmv_law == WAN_TDMV_ALAW){
		WRITE_USB_RM_REG(mod_no, 33, 0x20);
	}else if (fe->fe_cfg.tdmv_law == WAN_TDMV_MULAW){
		WRITE_USB_RM_REG(mod_no, 33, 0x28);
	}

	/* Set On-hook speed, Ringer impedence, and ringer threshold */
	reg16 |= (fxo_modes[fe->fe_cfg.cfg.remora.opermode].ohs << 6);
	reg16 |= (fxo_modes[fe->fe_cfg.cfg.remora.opermode].rz << 1);
	reg16 |= (fxo_modes[fe->fe_cfg.cfg.remora.opermode].rt);
	WRITE_USB_RM_REG(mod_no, 16, reg16);
	
	/* Set DC Termination:
	**	Tip/Ring voltage adjust,
	**	minimum operational current,
	**	current limitation */
	reg26 |= (fxo_modes[fe->fe_cfg.cfg.remora.opermode].dcv << 6);
	reg26 |= (fxo_modes[fe->fe_cfg.cfg.remora.opermode].mini << 4);
	reg26 |= (fxo_modes[fe->fe_cfg.cfg.remora.opermode].ilim << 1);
	WRITE_USB_RM_REG(mod_no, 26, reg26);

	/* Set AC Impedence */
	reg30 = (unsigned char)(fxo_modes[fe->fe_cfg.cfg.remora.opermode].acim);
	WRITE_USB_RM_REG(mod_no, 30, reg30);

	/* Misc. DAA parameters */
	reg31 = 0xa3;
	reg31 |= (fxo_modes[fe->fe_cfg.cfg.remora.opermode].ohs2 << 3);
	WRITE_USB_RM_REG(mod_no, 31, reg31);

	/* Set Transmit/Receive timeslot */
	WRITE_USB_RM_REG(mod_no, 34, mod_no * 8 + 1);
	WRITE_USB_RM_REG(mod_no, 35, 0x00);
	WRITE_USB_RM_REG(mod_no, 36, mod_no * 8 + 1);
	WRITE_USB_RM_REG(mod_no, 37, 0x00);

	/* Enable ISO-Cap */
	WRITE_USB_RM_REG(mod_no, 6, 0x00);

	if (!fast) {
		/* Wait 1000ms for ISO-cap to come up */
		start_ticks = SYSTEM_TICKS + 2*HZ;
		while(!(READ_USB_RM_REG(mod_no, 11) & 0xf0)){
			if (SYSTEM_TICKS > start_ticks){
				break;
			}
			wait_just_a_bit(HZ/10, fast);
		}
	
		if (!(READ_USB_RM_REG(mod_no, 11) & 0xf0)) {
			DEBUG_EVENT(
			"%s: Module %d: VoiceDAA did not bring up ISO link properly!\n",
						fe->name,
						mod_no+1);
			return -1;
		}
		DEBUG_TEST("%s: Module %d: ISO-Cap is now up, line side: %02x rev %02x\n", 
				fe->name,
				mod_no+1,
				READ_USB_RM_REG(mod_no, 11) >> 4,
				(READ_USB_RM_REG(mod_no, 13) >> 2) & 0xf);
	} else {
		WP_DELAY(10000);
	}

	/* Enable on-hook line monitor */
	WRITE_USB_RM_REG(mod_no, 5, 0x08);
	WRITE_USB_RM_REG(mod_no, 3, 0x00);
#if defined(AFT_RM_INTR_SUPPORT)
	WRITE_USB_RM_REG(mod_no, 2, 0x87/*0x84*/);
	fe->rm_param.mod[mod_no].u.fxo.imask = 0xFF;
#else
	WRITE_USB_RM_REG(mod_no, 2, 0x04 | 0x03);	/* Ring detect mode (begin/end) */
	fe->rm_param.mod[mod_no].u.fxo.imask = 0x00;
#endif		

	/* Take values for fxotxgain and fxorxgain and apply them to module */
	if (fe->fe_cfg.cfg.remora.fxo_txgain) {
		if (fe->fe_cfg.cfg.remora.fxo_txgain >= -150 && fe->fe_cfg.cfg.remora.fxo_txgain < 0) {
			DEBUG_EVENT("%s: Module %d: Adjust TX Gain to %2d.%d dB\n", 
					fe->name, mod_no+1,
					fe->fe_cfg.cfg.remora.fxo_txgain / 10,
					fe->fe_cfg.cfg.remora.fxo_txgain % -10);
			WRITE_USB_RM_REG(mod_no, 38, 16 + (fe->fe_cfg.cfg.remora.fxo_txgain/-10));
			if(fe->fe_cfg.cfg.remora.fxo_txgain % 10) {
				WRITE_USB_RM_REG(mod_no, 40, 16 + (-fe->fe_cfg.cfg.remora.fxo_txgain%10));
			}
		}
		else if (fe->fe_cfg.cfg.remora.fxo_txgain <= 120 && fe->fe_cfg.cfg.remora.fxo_txgain > 0) {
			DEBUG_EVENT("%s: Module %d: Adjust TX Gain to %2d.%d dB\n", 
					fe->name, mod_no+1,
					fe->fe_cfg.cfg.remora.fxo_txgain / 10,
					fe->fe_cfg.cfg.remora.fxo_txgain % 10);
			WRITE_USB_RM_REG(mod_no, 38, fe->fe_cfg.cfg.remora.fxo_txgain/10);
			if (fe->fe_cfg.cfg.remora.fxo_txgain % 10){
				WRITE_USB_RM_REG(mod_no, 40, (fe->fe_cfg.cfg.remora.fxo_txgain % 10));
			}
		}
	}
	if (fe->fe_cfg.cfg.remora.fxo_rxgain) {
		if (fe->fe_cfg.cfg.remora.fxo_rxgain >= -150 && fe->fe_cfg.cfg.remora.fxo_rxgain < 0) {
			DEBUG_EVENT("%s: Module %d: Adjust RX Gain to %2d.%d dB\n",
					fe->name, mod_no+1,
					fe->fe_cfg.cfg.remora.fxo_rxgain / 10,
					(-1) * (fe->fe_cfg.cfg.remora.fxo_rxgain % 10));
			WRITE_USB_RM_REG(mod_no, 39, 16 + (fe->fe_cfg.cfg.remora.fxo_rxgain/-10));
			if(fe->fe_cfg.cfg.remora.fxo_rxgain%10) {
				WRITE_USB_RM_REG(mod_no, 41, 16 + (-fe->fe_cfg.cfg.remora.fxo_rxgain%10));
			}
		}else if (fe->fe_cfg.cfg.remora.fxo_rxgain <= 120 && fe->fe_cfg.cfg.remora.fxo_rxgain > 0) {
			DEBUG_EVENT("%s: Module %d: Adjust RX Gain to %2d.%d dB\n",
					fe->name, mod_no+1,
					fe->fe_cfg.cfg.remora.fxo_rxgain / 10,
					fe->fe_cfg.cfg.remora.fxo_rxgain % 10);
			WRITE_USB_RM_REG(mod_no, 39, fe->fe_cfg.cfg.remora.fxo_rxgain/10);
			if(fe->fe_cfg.cfg.remora.fxo_rxgain % 10) {
				WRITE_USB_RM_REG(mod_no, 41, (fe->fe_cfg.cfg.remora.fxo_rxgain%10));
			}
		}
	}

	/* NZ -- crank the tx gain up by 7 dB */
	if (!strcmp(fxo_modes[fe->fe_cfg.cfg.remora.opermode].name, "NEWZEALAND")) {
		DEBUG_EVENT("%s: Module %d: Adjusting gain\n",
					fe->name,
					mod_no+1);
		WRITE_USB_RM_REG(mod_no, 38, 0x7);
	}

	return 0;
}


/******************************************************************************
** wp_usb_remora_iface_init) - 
**
**	OK
*/
int wp_usb_remora_iface_init(void *p_fe, void *pfe_iface)
{
	sdla_fe_t	*fe = (sdla_fe_t*)p_fe;
	sdla_fe_iface_t	*fe_iface = (sdla_fe_iface_t*)pfe_iface;

	fe_iface->config	= &wp_usb_remora_config;
	fe_iface->unconfig	= &wp_usb_remora_unconfig;
	
	fe_iface->post_init	= &wp_usb_remora_post_init;
	
	fe_iface->if_config	= &wp_usb_remora_if_config;
	fe_iface->if_unconfig	= &wp_usb_remora_if_unconfig;
	fe_iface->post_unconfig	= &wp_usb_remora_post_unconfig;
	fe_iface->active_map	= &wp_usb_remora_active_map;

	fe_iface->isr		= &wp_usb_remora_intr;
	fe_iface->disable_irq	= &wp_usb_remora_disable_irq;
	fe_iface->check_isr	= &wp_usb_remora_check_intr;

	fe_iface->polling	= &wp_usb_remora_polling;
	fe_iface->process_udp	= &wp_usb_remora_udp;
	fe_iface->get_fe_media	= &wp_usb_remora_fe_media;

	fe_iface->set_dtmf	= &wp_usb_remora_set_dtmf;
	fe_iface->intr_ctrl	= &wp_usb_remora_intr_ctrl;

	fe_iface->event_ctrl	= &wp_usb_remora_event_ctrl;
#if defined(AFT_TDM_API_SUPPORT) || defined(AFT_API_SUPPORT)
	fe_iface->watchdog	= &wp_usb_remora_watchdog;
#endif

	fe_iface->get_fe_status = &wp_usb_remora_get_link_status;     

	WAN_LIST_INIT(&fe->event);
	wan_spin_lock_irq_init(&fe->lockirq, "wan_rm_lock");
	return 0;
}

/******************************************************************************
** wp_usb_remora_opermode() - 
**
**	OK
*/
static int wp_usb_remora_opermode(sdla_fe_t *fe)
{
	sdla_fe_cfg_t	*fe_cfg = &fe->fe_cfg;

	if (!strlen(fe_cfg->cfg.remora.opermode_name)){
		memcpy(fe_cfg->cfg.remora.opermode_name, "FCC", 3);
		fe_cfg->cfg.remora.opermode = 0;
	}else{
		int x;
		for (x=0;x<(sizeof(fxo_modes) / sizeof(fxo_modes[0])); x++) {
			if (!strcmp(fxo_modes[x].name, fe_cfg->cfg.remora.opermode_name))
				break;
		}
		if (x < sizeof(fxo_modes) / sizeof(fxo_modes[0])) {
			fe_cfg->cfg.remora.opermode = x;
		} else {
			DEBUG_EVENT(
			"%s: Invalid/unknown operating mode '%s' specified\n",
						fe->name,
						fe_cfg->cfg.remora.opermode_name);
			DEBUG_EVENT(
			"%s: Please choose one of:\n",
						fe->name);
			for (x=0;x<sizeof(fxo_modes) / sizeof(fxo_modes[0]); x++)
				DEBUG_EVENT("%s:    %s\n",
					fe->name, fxo_modes[x].name);
			return -ENODEV;
		}
	}
	
	return 0;
}

/******************************************************************************
** wp_usb_remora_config() - 
**
**	OK
*/
static int wp_usb_remora_config(void *pfe)
{
	sdla_fe_t	*fe = (sdla_fe_t*)pfe;
	int		err=0, mod_no, mod_cnt = 0, err_cnt = 0, retry;
	int		sane=0;

	DEBUG_EVENT("%s: Configuring FXS/FXO Front End ...\n",
        		     	fe->name);
	fe->rm_param.max_fe_channels 	= MAX_USB_REMORA_MODULES;
	fe->rm_param.module_map 	= 0;
	fe->rm_param.intcount		= 0;
	fe->rm_param.last_watchdog 	= SYSTEM_TICKS;
	if (wp_usb_remora_opermode(fe)){
		return -EINVAL;
	}

	wait_just_a_bit(HZ, 0);

	/* Search for installed modules and enable chain for all modules */
	if (wp_usb_remora_module_detect(fe)){
		DEBUG_EVENT("%s: Failed enable chain mode for all modules!\n",
				fe->name);
		return -EINVAL;
	}

	/* Auto detect FXS and FXO modules */
	for(mod_no = 0; mod_no < MAX_USB_REMORA_MODULES; mod_no++){
		sane = 0; err = 0; retry = 0;
retry_cfg:
		switch(fe->rm_param.mod[mod_no].type){
		case MOD_TYPE_FXS:
			if (!(err = wp_usb_init_proslic(fe, mod_no, 0, sane))){
				DEBUG_EVENT(
				"%s: Module %d: Installed -- Auto FXS!\n",
					fe->name, mod_no+1);	
				wan_set_bit(mod_no, &fe->rm_param.module_map);
				fe->rm_param.mod[mod_no].u.fxs.oldrxhook = 1;	/* default (off-hook) */
				mod_cnt++;
			}
			break;

		case MOD_TYPE_FXO:
			err = wp_usb_init_voicedaa(fe, mod_no, 0, sane);
			if (!err){
				DEBUG_EVENT(
				"%s: Module %d: Installed -- Auto FXO (%s mode)!\n",
					fe->name, mod_no+1,
					fxo_modes[fe->fe_cfg.cfg.remora.opermode].name);
				wan_set_bit(mod_no, &fe->rm_param.module_map);
				mod_cnt++;
			}
			break;

		case MOD_TYPE_TEST:
			DEBUG_EVENT(
			"%s: Module %d: Installed -- FXS/FXO tester!\n",
					fe->name, mod_no+1);
			wan_set_bit(mod_no, &fe->rm_param.module_map);
			((sdla_t*)fe->card)->fe_no_intr = 0x1;
			mod_cnt++;
			break;
		default:
			DEBUG_TDMV("%s: Module %d: Not Installed!\n",
						fe->name, mod_no+1);	
			break;
		}
		if (err/* && !sane*/){
			if (!sane/*retry++ < 10*/){
				sane = 1;
				DEBUG_EVENT("%s: Module %d: Retry configuration...\n",
					fe->name, mod_no+1);
				goto retry_cfg;
			}
			DEBUG_EVENT("%s: Module %d: %s failed!\n",
				fe->name, mod_no+1,
				WP_REMORA_DECODE_TYPE(fe->rm_param.mod[mod_no].type));
			err_cnt++;
		}
	}

	/* NC REMOVED IT TEMPORARILY */	
	if (err_cnt && fe->fe_cfg.cfg.remora.relaxcfg != WANOPT_YES){
		DEBUG_EVENT(
		"%s: %d FXO/FXS module(s) are failed to initialize!\n",
					fe->name, err_cnt);
		return -EINVAL;
	}
	
#if defined(__WINDOWS__)
	fe->remora_modules_counter = mod_cnt;
#endif

	if (mod_cnt == 0){
		if (err_cnt){
			DEBUG_EVENT(
			"ERROR: %s: Configuration is failed for all FXO/FXS modules!\n",
					fe->name);
		}else{
			DEBUG_ERROR("ERROR: %s: No FXO/FXS modules are found!\n",
					fe->name);
		}
		return -EINVAL;
	}
	/* Initialize and start T1/E1 timer */
	wan_set_bit(WP_RM_TIMER_KILL,(void*)&fe->rm_param.critical);

	wan_init_timer(
		&fe->timer, 
		wp_usb_remora_timer,
	       	(wan_timer_arg_t)fe);
	
	wan_clear_bit(WP_RM_TIMER_KILL,(void*)&fe->rm_param.critical);
	wan_clear_bit(WP_RM_TIMER_RUNNING,(void*)&fe->rm_param.critical);
	wan_set_bit(WP_RM_CONFIGURED,(void*)&fe->rm_param.critical);
	
#if 0
	{
		sdla_fe_timer_event_t	event;
		event.type		= WAN_EVENT_RM_DTMF;
		event.mode		= WAN_RM_EVENT_ENABLE;
		event.rm_event.mod_no	= 0;
		wp_usb_remora_add_event(fe, &event);
	
		event.type		= WAN_EVENT_RM_LC;
		event.mode		= WAN_RM_EVENT_ENABLE;
		event.rm_event.mod_no	= 0;
		wp_usb_remora_add_event(fe, &event);
	
		event.type		= WAN_EVENT_RM_DTMF;
		event.mode		= WAN_RM_EVENT_ENABLE;
		event.rm_event.mod_no	= 0;
		wp_usb_remora_add_event(fe, &event);
	}
#endif

	return 0;
}

/******************************************************************************
** wp_usb_remora_unconfig() - 
**
**	OK
*/
static int wp_usb_remora_unconfig(void *pfe)
{
	sdla_fe_t		*fe = (sdla_fe_t*)pfe;
	int			mod_no;

	DEBUG_EVENT("%s: Unconfiguring FXS/FXO Front End...\n",
        		     	fe->name);

	/* Disable interrupt (should be done before ) */				
	wp_usb_remora_disable_irq(fe);
					
	/* Clear and Kill TE timer poll command */
	wan_clear_bit(WP_RM_CONFIGURED,(void*)&fe->rm_param.critical);
	
	wan_set_bit(WP_RM_TIMER_KILL,(void*)&fe->rm_param.critical);
	
	for(mod_no = 0; mod_no < MAX_USB_REMORA_MODULES; mod_no++){
		if (!wan_test_bit(mod_no, &fe->rm_param.module_map)) continue;
		if (fe->rm_param.mod[mod_no].type == MOD_TYPE_FXO){
			WRITE_USB_RM_REG(mod_no, 1, 0x80);
		}
		wan_clear_bit(mod_no, &fe->rm_param.module_map);
	}
	return 0;
}

/*
 ******************************************************************************
 *			sdla_te_post_unconfig()	
 *
 * Description: T1/E1 pre release routines (not locked).
 * Arguments:
 * Returns:
 ******************************************************************************
 */
static int wp_usb_remora_post_unconfig(void* pfe)
{
	sdla_fe_t		*fe = (sdla_fe_t*)pfe;
	sdla_fe_timer_event_t	*fe_event = NULL;
	wan_smp_flag_t		smp_flags;
	int			empty = 0;
	
	/* Kill TE timer poll command */

	if (wan_test_bit(WP_RM_TIMER_RUNNING,(void*)&fe->rm_param.critical)){
		wan_del_timer(&fe->timer);
	}

	wan_clear_bit(WP_RM_TIMER_RUNNING,(void*)&fe->rm_param.critical);
	do{
		wan_spin_lock_irq(&fe->lockirq,&smp_flags);
		if (!WAN_LIST_EMPTY(&fe->event)){
			fe_event = WAN_LIST_FIRST(&fe->event);
			WAN_LIST_REMOVE(fe_event, next);
		}else{
			empty = 1;
		}
		wan_spin_unlock_irq(&fe->lockirq,&smp_flags);
		/* Free should be called not from spin_lock_irq (windows) !!!! */
		if (fe_event) wan_free(fe_event);
		fe_event = NULL;
	}while(!empty);
	
	return 0;
}

/******************************************************************************
** wp_usb_remora_post_init() - 
**
**	OK
*/
static int wp_usb_remora_post_init(void *pfe)
{
	sdla_fe_t	*fe = (sdla_fe_t*)pfe;

	DEBUG_EVENT("%s: Running post initialization...\n", fe->name);
	return 0;	//wp_usb_remora_add_timer(fe, HZ);
}

/******************************************************************************
** wp_usb_remora_if_config() - 
**
**	OK
*/
static int wp_usb_remora_if_config(void *pfe, u32 mod_map, u8 usedby)
{
	sdla_fe_t	*fe = (sdla_fe_t*)pfe;

#if defined(AFT_RM_INTR_SUPPORT)
	int		mod_no;
#endif
	
	if (!wan_test_bit(WP_RM_CONFIGURED,(void*)&fe->rm_param.critical)){
		return -EINVAL;
	}

	if (usedby == TDM_VOICE_API){
#if defined(AFT_RM_INTR_SUPPORT)
		/* Enable remora interrupts (only for TDM_API) */
		for(mod_no = 0; mod_no < MAX_USB_REMORA_MODULES; mod_no++){
			if (!wan_test_bit(mod_no, &fe->rm_param.module_map) ||
			    !wan_test_bit(mod_no, &mod_map)){
				continue;
			}
			wp_usb_remora_intr_ctrl(	fe, 
						mod_no, 
						WAN_RM_INTR_GLOBAL, 
						WAN_FE_INTR_ENABLE, 
						0x00);
		}
#endif		
	}
	return 0;
}

/******************************************************************************
** wp_usb_remora_if_unconfig() - 
**
**	OK
*/
static int wp_usb_remora_if_unconfig(void *pfe, u32 mod_map, u8 usedby)
{
	sdla_fe_t	*fe = (sdla_fe_t*)pfe;
#if defined(AFT_RM_INTR_SUPPORT)
	int		mod_no;
#endif
	
	if (!wan_test_bit(WP_RM_CONFIGURED,(void*)&fe->rm_param.critical)){
		return -EINVAL;
	}

	if (usedby == TDM_VOICE_API){
#if defined(AFT_RM_INTR_SUPPORT)
		for(mod_no = 0; mod_no < MAX_USB_REMORA_MODULES; mod_no++){
			if (!wan_test_bit(mod_no, &fe->rm_param.module_map) ||
			!wan_test_bit(mod_no, &mod_map)){
			continue;
			}
			wp_usb_remora_intr_ctrl(	fe, 
						mod_no, 
						WAN_RM_INTR_GLOBAL, 
						WAN_FE_INTR_MASK, 
						0x00);
		}
#endif
	}
	return 0;	
}

/******************************************************************************
** wp_usb_remora_disable_irq() - 
**
**	OK
*/
static int wp_usb_remora_disable_irq(void *pfe)
{
	sdla_fe_t	*fe = (sdla_fe_t*)pfe;
	int		mod_no;

	if (!wan_test_bit(WP_RM_CONFIGURED,(void*)&fe->rm_param.critical)){
		return -EINVAL;
	}

	for(mod_no = 0; mod_no < MAX_USB_REMORA_MODULES; mod_no++){
		if (wan_test_bit(mod_no, &fe->rm_param.module_map)) {
			wp_usb_remora_intr_ctrl(	fe, 
						mod_no, 
						WAN_RM_INTR_GLOBAL, 
						WAN_FE_INTR_MASK, 
						0x00);
		}
	}
	return 0;
}

static unsigned int wp_usb_remora_active_map(sdla_fe_t* fe, unsigned char not_used)
{
	return fe->rm_param.module_map;
}

/******************************************************************************
 *				wp_usb_remora_fe_status()	
 *
 * Description:
 * Arguments:	
 * Returns:
 ******************************************************************************
 */
static unsigned char wp_usb_remora_fe_media(sdla_fe_t *fe)
{
	return fe->fe_cfg.media;
}

/******************************************************************************
 *				wp_usb_remora_set_dtmf()	
 *
 * Description:
 * Arguments:	
 * Returns:
 ******************************************************************************
 */
static int wp_usb_remora_set_dtmf(sdla_fe_t *fe, int mod_no, unsigned char val)
{

	if (mod_no > MAX_USB_REMORA_MODULES){
		DEBUG_EVENT("%s: Module %d: Module number out of range!\n",
					fe->name, mod_no+1);
		return -EINVAL;
	}	
	if (!wan_test_bit(mod_no-1, &fe->rm_param.module_map)){
		DEBUG_EVENT("%s: Module %d: Not configures yet!\n",
					fe->name, mod_no+1);
		return -EINVAL;
	}
	
	return -EINVAL;
}

/*
 ******************************************************************************
 *				sdla_remora_timer()	
 *
 * Description:
 * Arguments:
 * Returns:
 ******************************************************************************
 */
#if defined(__NetBSD__) || defined(__FreeBSD__) || defined(__OpenBSD__)
static void wp_usb_remora_timer(void* pfe)
#elif defined(__WINDOWS__)
static void wp_usb_remora_timer(IN PKDPC Dpc, void* pfe, void* arg2, void* arg3)
#else
static void wp_usb_remora_timer(unsigned long pfe)
#endif
{
	sdla_fe_t	*fe = (sdla_fe_t*)pfe;
	sdla_t 		*card = (sdla_t*)fe->card;
	wan_device_t	*wandev = &card->wandev;
	wan_smp_flag_t	smp_flags;
	int		empty = 1;

	DEBUG_TEST("%s: RM timer!\n", fe->name);
	
	if (wan_test_bit(WP_RM_TIMER_KILL,(void*)&fe->rm_param.critical)){
		wan_clear_bit(WP_RM_TIMER_RUNNING,(void*)&fe->rm_param.critical);
		return;
	}
	if (!wan_test_bit(WP_RM_TIMER_RUNNING,(void*)&fe->rm_param.critical)){
		/* Somebody clear this bit */
		DEBUG_EVENT("WARNING: %s: Timer bit is cleared (should never happened)!\n", 
					fe->name);
		return;
	}
	wan_clear_bit(WP_RM_TIMER_RUNNING,(void*)&fe->rm_param.critical);

	/* Enable hardware interrupt for TE1 */
	wan_spin_lock_irq(&fe->lockirq,&smp_flags);	
	empty = WAN_LIST_EMPTY(&fe->event);
	wan_spin_unlock_irq(&fe->lockirq,&smp_flags);	

	if (!empty){
		if (wan_test_and_set_bit(WP_RM_TIMER_EVENT_PENDING,(void*)&fe->rm_param.critical)){
			DEBUG_EVENT("%s: USB-FXO timer event is pending!\n", fe->name);
			return;
		}
		if (wandev->fe_enable_timer){
			wandev->fe_enable_timer(fe->card);
		}else{
DEBUG_EVENT("%s: USB-FXO event timer event is NULL!\n", fe->name);
			//gl_usb_rw_fast=1;			
//			wp_usb_remora_polling(fe);
			//gl_usb_rw_fast=0;			
		}
	}else{
//		wp_usb_remora_add_timer(fe, WP_RM_POLL_TIMER);
	}
	return;
}

/*
 ******************************************************************************
 *				wp_usb_remora_add_timer()	
 *
 * Description: Enable software timer interrupt in delay ms.
 * Arguments:
 * Returns:
 ******************************************************************************
 */
static int wp_usb_remora_add_timer(sdla_fe_t* fe, unsigned long delay)
{
	int	err;
	
	DEBUG_TEST("%s: Add new RM timer!\n", fe->name);
	err = wan_add_timer(&fe->timer, delay * HZ / 1000);
	if (err){
		/* Failed to add timer */
		return -EINVAL;
	}
	wan_set_bit(WP_RM_TIMER_RUNNING,(void*)&fe->rm_param.critical);
	return 0;
}


/******************************************************************************
**				wp_usb_remora_polling()	
**
** Description:
** Arguments:
** Returns:
******************************************************************************/
static int wp_usb_remora_polling(sdla_fe_t* fe)
{
	sdla_t			*card = (sdla_t*)fe->card;
	sdla_fe_timer_event_t	*fe_event;
	wan_event_t		event;
	wan_smp_flag_t		smp_flags;
	int			pending = 0, mod_no = 0;
	u8			imask;
#if defined(CONFIG_PRODUCT_WANPIPE_TDM_VOICE)
	int			err = 0;
#endif

	WAN_ASSERT_RC(fe->write_fe_reg == NULL,0);
	WAN_ASSERT_RC(fe->read_fe_reg == NULL, 0);

#if 0	
	DEBUG_EVENT("%s: %s:%d: ---------------START ----------------------\n",
				fe->name, __FUNCTION__,__LINE__);
	WARN_ON(1);
	DEBUG_EVENT("%s: %s:%d: ---------------STOP ----------------------\n",
				fe->name, __FUNCTION__,__LINE__);
#endif
	wan_spin_lock_irq(&fe->lockirq,&smp_flags);			
	if (WAN_LIST_EMPTY(&fe->event)){
		wan_clear_bit(WP_RM_TIMER_EVENT_PENDING,(void*)&fe->rm_param.critical);
		wan_spin_unlock_irq(&fe->lockirq,&smp_flags);	
		DEBUG_EVENT("%s: WARNING: No FE events in a queue!\n",
					fe->name);
//		wp_usb_remora_add_timer(fe, HZ);
		return 0;
	}
	fe_event = WAN_LIST_FIRST(&fe->event);
	WAN_LIST_REMOVE(fe_event, next);
	wan_spin_unlock_irq(&fe->lockirq,&smp_flags);

#if defined(__WINDOWS__)
	/* FIXME: Try to make common code for all OS */
	/* poll is NOT locked outside! */
	wan_spin_lock_irq(&card->wandev.lock,&smp_flags);
#endif
	
	mod_no = fe_event->rm_event.mod_no;
	DEBUG_RM("[RM] %s: Module %d: RM Polling State=%s Cmd=%s(%X) Mode=%s!\n", 
			fe->name, mod_no+1,
			fe->fe_status==FE_CONNECTED?"Con":"Disconn",
			WP_RM_POLL_DECODE(fe_event->type), fe_event->type,
			WAN_EVENT_MODE_DECODE(fe_event->mode));

	switch(fe_event->type){
	case WP_RM_POLL_POWER:
		imask = READ_USB_RM_REG(mod_no, 22);
		if (fe_event->mode == WAN_EVENT_ENABLE){
			imask |= 0xFC;
			wan_set_bit(WAN_RM_EVENT_POWER,
				&fe->rm_param.mod[mod_no].events);
		}else{
			imask &= ~0xFC;
			wan_clear_bit(WAN_RM_EVENT_POWER,
				&fe->rm_param.mod[mod_no].events);
		}
		WRITE_USB_RM_REG(mod_no, 22, imask);
		break;
	case WP_RM_POLL_LC:
		imask = READ_USB_RM_REG(mod_no, 22);
		if (fe_event->mode == WAN_EVENT_ENABLE){
			imask |= 0x02;
			wan_set_bit(WAN_RM_EVENT_LC,
				&fe->rm_param.mod[mod_no].events);
		}else{
			imask &= ~0x02;
			wan_clear_bit(WAN_RM_EVENT_LC,
				&fe->rm_param.mod[mod_no].events);
		}
		WRITE_USB_RM_REG(mod_no, 22, imask);
		break;
	case WP_RM_POLL_RING_TRIP:
		imask = READ_USB_RM_REG(mod_no, 22);
		if (fe_event->mode == WAN_EVENT_ENABLE){
			imask |= 0x01;
			wan_set_bit(WAN_RM_EVENT_RING_TRIP,
				&fe->rm_param.mod[mod_no].events);
		}else{
			imask &= ~0x01;
			wan_clear_bit(WAN_RM_EVENT_RING_TRIP,
				&fe->rm_param.mod[mod_no].events);
		}			
		WRITE_USB_RM_REG(mod_no, 22, imask);
		break;
	case WP_RM_POLL_DTMF:
		imask = READ_USB_RM_REG(mod_no, 23);
		if (fe_event->mode == WAN_EVENT_ENABLE){
			imask |= 0x01;
			wan_set_bit(WAN_RM_EVENT_DTMF,
				&fe->rm_param.mod[mod_no].events);
		}else{
			imask &= ~0x01;
			wan_clear_bit(WAN_RM_EVENT_DTMF,
				&fe->rm_param.mod[mod_no].events);
		}
		WRITE_USB_RM_REG(mod_no, 23, imask);
		break;	
	case WP_RM_POLL_RING:
		if (fe_event->mode == WAN_EVENT_ENABLE){
			WRITE_USB_RM_REG(mod_no, 64, 0x04);
			wan_set_bit(WAN_RM_EVENT_RING,
				&fe->rm_param.mod[mod_no].events);
		}else{
			WRITE_USB_RM_REG(mod_no, 64, 0x01);
			wan_clear_bit(WAN_RM_EVENT_RING,
				&fe->rm_param.mod[mod_no].events);
		}
		break;
	case WP_RM_POLL_TONE:
		if (fe_event->mode == WAN_EVENT_ENABLE){
			DEBUG_RM("%s: Module %d: %s %s events (%d)!\n",
					fe->name, mod_no+1,
					WAN_EVENT_MODE_DECODE(fe_event->mode),
					WP_RM_POLL_DECODE(fe_event->type),
					fe_event->rm_event.tone);
			switch(fe_event->rm_event.tone){
			case WAN_EVENT_RM_TONE_TYPE_DIAL:
				wp_usb_remora_dialtone(fe, mod_no);
				break;
			case WAN_EVENT_RM_TONE_TYPE_BUSY:
				wp_usb_remora_busytone(fe, mod_no);	
				break;
			case WAN_EVENT_RM_TONE_TYPE_RING:
				wp_usb_remora_ringtone(fe, mod_no);	
				break;
			case WAN_EVENT_RM_TONE_TYPE_CONGESTION:
				wp_usb_remora_congestiontone(fe, mod_no);	
				break;	
			}
			wan_set_bit(WAN_RM_EVENT_TONE,
				&fe->rm_param.mod[mod_no].events);
		}else{
			wp_usb_remora_disabletone(fe, mod_no);
			wan_clear_bit(WAN_RM_EVENT_TONE,
				&fe->rm_param.mod[mod_no].events);
		}
		break;
	case WP_RM_POLL_TXSIG_KEWL:
		fe->rm_param.mod[mod_no].u.fxs.lasttxhook = 0;
		WRITE_USB_RM_REG(mod_no, 64, fe->rm_param.mod[mod_no].u.fxs.lasttxhook);
		break;
	case WP_RM_POLL_TXSIG_START:
		if (fe->rm_param.mod[mod_no].type == MOD_TYPE_FXS){
			DEBUG_RM("%s: Module %d: txsig START.\n",
					fe->name, mod_no+1);
			fe->rm_param.mod[mod_no].u.fxs.lasttxhook = 4;
			WRITE_USB_RM_REG(mod_no, 64,
				fe->rm_param.mod[mod_no].u.fxs.lasttxhook);
		}else if (fe->rm_param.mod[mod_no].type == MOD_TYPE_FXO){
			DEBUG_RM("%s: Module %d: goes off-hook.\n", 
					fe->name, mod_no+1);
			fe->rm_param.mod[mod_no].u.fxo.offhook = 1;
			WRITE_USB_RM_REG(mod_no, 5, 0x9);
		}
		break;
	case WP_RM_POLL_TXSIG_OFFHOOK:
		if (fe->rm_param.mod[mod_no].type == MOD_TYPE_FXS){
			DEBUG_RM("%s: Module %d: goes off-hook.\n",
						fe->name, mod_no+1);
			switch(fe->rm_param.mod[mod_no].sig) {
			case WAN_RM_SIG_EM:
				fe->rm_param.mod[mod_no].u.fxs.lasttxhook = 5;
				break;
			default:
				fe->rm_param.mod[mod_no].u.fxs.lasttxhook =
					fe->rm_param.mod[mod_no].u.fxs.idletxhookstate;
				break;
			}
			WRITE_USB_RM_REG(mod_no, 64,
				fe->rm_param.mod[mod_no].u.fxs.lasttxhook);
		}else if (fe->rm_param.mod[mod_no].type == MOD_TYPE_FXO){
			DEBUG_TDMV("%s: Module %d: goes off-hook.\n", 
					fe->name, mod_no+1);
			fe->rm_param.mod[mod_no].u.fxo.offhook = 1;
			WRITE_USB_RM_REG(mod_no, 5, 0x9);
		}		
		break;
	case WP_RM_POLL_TXSIG_ONHOOK:
		if (fe->rm_param.mod[mod_no].type == MOD_TYPE_FXS){
			DEBUG_RM("%s: Module %d: goes on-hook.\n",
						fe->name, mod_no+1);
			switch(fe->rm_param.mod[mod_no].sig) {
			case WAN_RM_SIG_EM:
			case WAN_RM_SIG_FXOKS:
			case WAN_RM_SIG_FXOLS:
				fe->rm_param.mod[mod_no].u.fxs.lasttxhook =
					fe->rm_param.mod[mod_no].u.fxs.idletxhookstate;
				break;
			case WAN_RM_SIG_FXOGS:
				fe->rm_param.mod[mod_no].u.fxs.lasttxhook = 3;
				break;
			}
			WRITE_USB_RM_REG(mod_no, 64,
				fe->rm_param.mod[mod_no].u.fxs.lasttxhook);
		}else if (fe->rm_param.mod[mod_no].type == MOD_TYPE_FXO){
			DEBUG_RM("%s: Module %d: goes on-hook.\n", 
					fe->name, mod_no+1);
			fe->rm_param.mod[mod_no].u.fxo.offhook = 0;
			WRITE_USB_RM_REG(mod_no, 5, 0x8);
		}
		break;
	case WP_RM_POLL_ONHOOKTRANSFER:
		fe->rm_param.mod[mod_no].u.fxs.ohttimer = 
				fe_event->rm_event.ohttimer << 3;
		if (fe->fe_cfg.cfg.remora.reversepolarity){
			/* OHT mode when idle */
			fe->rm_param.mod[mod_no].u.fxs.idletxhookstate  = 0x6;
		}else{
			fe->rm_param.mod[mod_no].u.fxs.idletxhookstate  = 0x2;
		}
		if (fe->rm_param.mod[mod_no].u.fxs.lasttxhook == 0x1) {
			/* Apply the change if appropriate */
			if (fe->fe_cfg.cfg.remora.reversepolarity){
				fe->rm_param.mod[mod_no].u.fxs.lasttxhook = 0x6;
			}else{
				fe->rm_param.mod[mod_no].u.fxs.lasttxhook = 0x2;
			}
			WRITE_USB_RM_REG(mod_no, 64,
				fe->rm_param.mod[mod_no].u.fxs.lasttxhook);
		}
		break;
	case WP_RM_POLL_SETPOLARITY:
		/* Can't change polarity while ringing or when open */
		if ((fe->rm_param.mod[mod_no].u.fxs.lasttxhook == 0x04) ||
		    (fe->rm_param.mod[mod_no].u.fxs.lasttxhook == 0x00)){
#if 0 /*defined(__WINDOWS__)*/
			//done with the event, reset the pointer.
			fe->rm_param.mod[mod_no].current_control_event_ptr = NULL;

			wan_spin_lock_irq(&card->wandev.lock,&smp_flags);
			card->event_control_running = 0;
			wan_spin_unlock_irq(&card->wandev.lock,&smp_flags);
#endif
			break;
		}
	
		if ((fe_event->mode && !fe->fe_cfg.cfg.remora.reversepolarity) ||
		    (!fe_event->mode && fe->fe_cfg.cfg.remora.reversepolarity)){
			fe->rm_param.mod[mod_no].u.fxs.lasttxhook |= 0x04;
		}else{
			fe->rm_param.mod[mod_no].u.fxs.lasttxhook &= ~0x04;
		}
		WRITE_USB_RM_REG(mod_no, 64, fe->rm_param.mod[mod_no].u.fxs.lasttxhook);
		break;

	case WP_RM_POLL_RING_DETECT:
		if (fe->rm_param.mod[mod_no].type == MOD_TYPE_FXO){
			imask = READ_USB_RM_REG(mod_no, 3);
			if (fe_event->mode == WAN_EVENT_ENABLE){
				imask |= 0x80;
				wan_set_bit(WAN_RM_EVENT_RING_DETECT,
					&fe->rm_param.mod[mod_no].events);
			}else{
				imask &= ~0x80;
				wan_clear_bit(WAN_RM_EVENT_RING_DETECT,
					&fe->rm_param.mod[mod_no].events);
			}
			WRITE_USB_RM_REG(mod_no, 3, imask);
		}
		break;
			
#if defined(CONFIG_PRODUCT_WANPIPE_TDM_VOICE)
	case WP_RM_POLL_TDMV:
#if defined(__WINDOWS__)
		//FIXME: implement
		TDM_FUNC_DBG();
#else
		WAN_TDMV_CALL(polling, (card), err);
#endif
		break;
#endif
	case WP_RM_POLL_INIT:
		wp_usb_init_proslic(fe, fe_event->rm_event.mod_no, 1, 1);
		break;
	case WP_RM_POLL_READ:
		fe->rm_param.reg_dbg_value = READ_USB_RM_REG(mod_no,fe_event->rm_event.reg);
		fe->rm_param.reg_dbg_ready = 1;		
		DEBUG_RM("%s: Module %d: Reg %d = %02X\n",
					fe->name, mod_no+1,
					fe_event->rm_event.reg,
					fe->rm_param.reg_dbg_value); 
		break;
	case WP_RM_POLL_WRITE:
		WRITE_USB_RM_REG(mod_no, fe_event->rm_event.reg, fe_event->rm_event.value);
		break;
	case WP_RM_POLL_RXSIG_ONHOOK:
		event.type	= WAN_EVENT_RM_LC;
		event.channel	= mod_no+1;
		event.rxhook	= WAN_EVENT_RXHOOK_ON;
		if (card->wandev.event_callback.hook){
			card->wandev.event_callback.hook(card, &event);
		}
		break;
	case WP_RM_POLL_RXSIG_OFFHOOK:
		event.type	= WAN_EVENT_RM_LC;
		event.channel	= mod_no+1;
		event.rxhook	= WAN_EVENT_RXHOOK_OFF;
		if (card->wandev.event_callback.hook){
			card->wandev.event_callback.hook(card, &event);
		}
		break;
	default:
		DEBUG_ERROR("ERROR: %s: Invalid poll event type %X!\n", 
				fe->name, fe_event->type);
		break;
	}

#if defined(__WINDOWS__)
	/* poll is NOT locked outside! */
	wan_spin_unlock_irq(&card->wandev.lock,&smp_flags);
#endif

#if 0/*defined(__WINDOWS__)*/
	//done with the event, reset the pointer.
	fe->rm_param.mod[mod_no].current_control_event_ptr = NULL;

	wan_spin_lock_irq(&card->wandev.lock,&smp_flags);
	card->event_control_running = 0;
	wan_spin_unlock_irq(&card->wandev.lock,&smp_flags);
#endif   
	
	/* Add new event */
	if (pending){
		wp_usb_remora_add_event(fe, fe_event);
	}
	wan_clear_bit(WP_RM_TIMER_EVENT_PENDING,(void*)&fe->rm_param.critical);
	if (fe_event) wan_free(fe_event);

#if 0
	/* Add fe timer */
	fe_event = WAN_LIST_FIRST(&fe->event);
	if (fe_event){
		wp_usb_remora_add_timer(fe, fe_event->delay);	
	}else{
		wp_usb_remora_add_timer(fe, WP_RM_POLL_TIMER);	
	}
#endif
	return 0;
}

/*
 ******************************************************************************
 *				wp_usb_remora_add_event()	
 *
 * Description: Enable software timer interrupt in delay ms.
 * Arguments:
 * Returns:
 ******************************************************************************
 */
static int
wp_usb_remora_add_event(sdla_fe_t *fe, sdla_fe_timer_event_t *fe_event)
{
	sdla_t			*card = (sdla_t*)fe->card;
	sdla_fe_timer_event_t	*event = NULL;
	wan_smp_flag_t		smp_flags;

	WAN_ASSERT_RC(card == NULL, -EINVAL);
	
	DEBUG_RM("%s: Remora Event=0x%X\n",
			fe->name,fe_event->type);

	/* Creating event timer */	
	event = wan_malloc(sizeof(sdla_fe_timer_event_t));
	if (event == NULL){
		DEBUG_EVENT(
       		"%s: Failed to allocate memory for timer event!\n",
					fe->name);
		return -EINVAL;
	}
	
	memcpy(event, fe_event, sizeof(sdla_fe_timer_event_t));
	
#if 0	
	DEBUG_EVENT("%s: %s:%d: ---------------START ----------------------\n",
				fe->name, __FUNCTION__,__LINE__);
	WARN_ON(1);
	DEBUG_EVENT("%s: %s:%d: ---------------STOP ----------------------\n",
				fe->name, __FUNCTION__,__LINE__);
#endif	
	wan_spin_lock_irq(&fe->lockirq,&smp_flags);	
	if (WAN_LIST_EMPTY(&fe->event)){
		WAN_LIST_INSERT_HEAD(&fe->event, event, next);
	}else{
#if defined(__WINDOWS__)
		/* only one event allowed at a time */
		DEBUG_RM("%s: returning EBUSY\n",
			fe->name);
		wan_spin_unlock_irq(&fe->lockirq, &smp_flags);	
		return EBUSY;
#else
		DEBUG_RM("%s: returning EBUSY\n",
			fe->name);
		wan_spin_unlock_irq(&fe->lockirq, &smp_flags);	
		return EBUSY;

//		sdla_fe_timer_event_t	*tmp;
//		WAN_LIST_FOREACH(tmp, &fe->event, next){
//			if (!WAN_LIST_NEXT(tmp, next)) break;
//		}
//		if (tmp == NULL){
//			DEBUG_ERROR("%s: Internal Error!!!\n", fe->name);
//			wan_spin_unlock_irq(&fe->lockirq, &smp_flags);	
//			return -EINVAL;
//		}
//		WAN_LIST_INSERT_AFTER(tmp, event, next);
#endif
	}
	wan_spin_unlock_irq(&fe->lockirq, &smp_flags);	

	wp_usb_remora_add_timer(fe, fe_event->delay);	
	return 0;
}

/******************************************************************************
*				wp_usb_remora_event_verification()	
*
* Description: 
* Arguments: mod_no -  Module number (1,2,3,... MAX_USB_REMORA_MODULES)
* Returns:
******************************************************************************/
static int
wp_usb_remora_event_verification(sdla_fe_t *fe, wan_event_ctrl_t *ectrl)
{
	int	mod_no = ectrl->mod_no-1;

	/* Event verification */
	if (fe->rm_param.mod[mod_no].type == MOD_TYPE_FXS){
		switch(ectrl->type){
		case WAN_EVENT_RM_POWER:
		case WAN_EVENT_RM_LC:
		case WAN_EVENT_RM_DTMF:
		case WAN_EVENT_RM_RING_TRIP:
		case WAN_EVENT_RM_RING:
		case WAN_EVENT_RM_TONE:
		case WAN_EVENT_RM_TXSIG_KEWL:
		case WAN_EVENT_RM_TXSIG_START:
		case WAN_EVENT_RM_TXSIG_OFFHOOK:
		case WAN_EVENT_RM_TXSIG_ONHOOK:
		case WAN_EVENT_RM_ONHOOKTRANSFER:
		case WAN_EVENT_RM_SETPOLARITY:
			break;
		default:
			DEBUG_EVENT(
			"%s: Module %d: Remora RING Event is only valid for FXS module (%X)\n",
					fe->name,mod_no+1,
					ectrl->type);
			return -EINVAL;
		}
	}else if (fe->rm_param.mod[mod_no].type == MOD_TYPE_FXO){
		switch(ectrl->type){
		case WAN_EVENT_RM_RING_DETECT:
		case WAN_EVENT_RM_TXSIG_START:
		case WAN_EVENT_RM_TXSIG_OFFHOOK:
		case WAN_EVENT_RM_TXSIG_ONHOOK:
			break;
		default:
			DEBUG_EVENT(
			"%s: Module %d: Remora RING Event is only valid for FXO module(%X)\n",
					fe->name,mod_no+1,
					ectrl->type);
			return -EINVAL;
		}
	}else{
		DEBUG_EVENT(
		"%s: Module %d: Unknown Module type %X\n",
				fe->name,mod_no+1,
				fe->rm_param.mod[mod_no].type);
		return -EINVAL;	
	}

	return 0;
}
/******************************************************************************
*				wp_usb_remora_event_ctrl()	
*
* Description: Enable/Disable event types
* Arguments: mod_no -  Module number (1,2,3,... MAX_USB_REMORA_MODULES)
* Returns:
******************************************************************************/
static int
wp_usb_remora_event_ctrl(sdla_fe_t *fe, wan_event_ctrl_t *ectrl)
{
	sdla_fe_timer_event_t	fe_event;
	int			mod_no = ectrl->mod_no-1, err = 0;

	WAN_ASSERT(ectrl == NULL);
	
	if (mod_no+1 > MAX_USB_REMORA_MODULES){
		DEBUG_EVENT("%s: Module %d: Module number is out of range!\n",
					fe->name, mod_no+1);
		return -EINVAL;
	}
	if (!wan_test_bit(mod_no, &fe->rm_param.module_map)) {
		DEBUG_EVENT("%s: Module %d: Unconfigured module!\n",
					fe->name, mod_no);
		return -EINVAL;
	}

	if (wp_usb_remora_event_verification(fe, ectrl)){
		return -EINVAL;
	}
		
	DEBUG_RM("%s: Module %d: Scheduling %s event (%s:%X)!\n",
				fe->name, mod_no+1,
				WAN_EVENT_TYPE_DECODE(ectrl->type),
				WAN_EVENT_MODE_DECODE(ectrl->mode), ectrl->mode);

	fe_event.mode		= ectrl->mode;
	fe_event.rm_event.mod_no= mod_no;
	fe_event.delay		= WP_RM_POLL_TIMER;
	switch(ectrl->type){
	case WAN_EVENT_RM_POWER:
		fe_event.type		= WP_RM_POLL_POWER;
		break;
	case WAN_EVENT_RM_LC:
		fe_event.type		= WP_RM_POLL_LC;
		break;				
	case WAN_EVENT_RM_DTMF:
		fe_event.type		= WP_RM_POLL_DTMF;
		break;	
	case WAN_EVENT_RM_RING_TRIP:
		fe_event.type		= WP_RM_POLL_RING_TRIP;
		break;
	case WAN_EVENT_RM_RING:
		fe_event.type		= WP_RM_POLL_RING;
		break;
	case WAN_EVENT_RM_RING_DETECT:
		fe_event.type		= WP_RM_POLL_RING_DETECT;
		break;
	case WAN_EVENT_RM_TONE:
		fe_event.type		= WP_RM_POLL_TONE;
		fe_event.rm_event.tone	= ectrl->tone;
		break;
	case WAN_EVENT_RM_TXSIG_KEWL:
		fe_event.type		= WP_RM_POLL_TXSIG_KEWL;
		break;
	case WAN_EVENT_RM_TXSIG_START:		/* FXS/FXO */
		fe_event.type		= WP_RM_POLL_TXSIG_START;
		break;
	case WAN_EVENT_RM_TXSIG_OFFHOOK:	/* FXS/FXO */
		fe_event.type		= WP_RM_POLL_TXSIG_OFFHOOK;
		break;
	case WAN_EVENT_RM_TXSIG_ONHOOK:		/* FXS/FXO */
		fe_event.type		= WP_RM_POLL_TXSIG_ONHOOK;
		break;	
	case WAN_EVENT_RM_ONHOOKTRANSFER:
		fe_event.type		= WP_RM_POLL_ONHOOKTRANSFER;
		fe_event.rm_event.ohttimer= ectrl->ohttimer;
		break;			
	case WAN_EVENT_RM_SETPOLARITY:
		fe_event.type		= WP_RM_POLL_SETPOLARITY;
		break;		
	}
	err = wp_usb_remora_add_event(fe, &fe_event);	
	return err;	
}

static int wp_usb_remora_dialtone(sdla_fe_t *fe, int mod_no)
{

	DEBUG_RM("%s: Module %d: Enable Dial tone\n",
				fe->name, mod_no+1);
	wp_proslic_setreg_indirect(fe, mod_no, 13,DIALTONE_IR13);
	wp_proslic_setreg_indirect(fe, mod_no, 14,DIALTONE_IR14);
	wp_proslic_setreg_indirect(fe, mod_no, 16,DIALTONE_IR16);
	wp_proslic_setreg_indirect(fe, mod_no, 17,DIALTONE_IR17);

	WRITE_USB_RM_REG(mod_no, 36,  DIALTONE_DR36);
	WRITE_USB_RM_REG(mod_no, 37,  DIALTONE_DR37);
	WRITE_USB_RM_REG(mod_no, 38,  DIALTONE_DR38);
	WRITE_USB_RM_REG(mod_no, 39,  DIALTONE_DR39);
	WRITE_USB_RM_REG(mod_no, 40,  DIALTONE_DR40);
	WRITE_USB_RM_REG(mod_no, 41,  DIALTONE_DR41);
	WRITE_USB_RM_REG(mod_no, 42,  DIALTONE_DR42);
	WRITE_USB_RM_REG(mod_no, 43,  DIALTONE_DR43);

	WRITE_USB_RM_REG(mod_no, 32,  DIALTONE_DR32);
	WRITE_USB_RM_REG(mod_no, 33,  DIALTONE_DR33);
	return 0;
}

static int wp_usb_remora_busytone(sdla_fe_t *fe, int mod_no)
{

	DEBUG_RM("%s: Module %d: Enable Busy tone\n",
				fe->name, mod_no+1);
	wp_proslic_setreg_indirect(fe, mod_no, 13,BUSYTONE_IR13);
	wp_proslic_setreg_indirect(fe, mod_no, 14,BUSYTONE_IR14);
	wp_proslic_setreg_indirect(fe, mod_no, 16,BUSYTONE_IR16);
	wp_proslic_setreg_indirect(fe, mod_no, 17,BUSYTONE_IR17);

	WRITE_USB_RM_REG(mod_no, 6,  BUSYTONE_DR36);
	WRITE_USB_RM_REG(mod_no, 37,  BUSYTONE_DR37);
	WRITE_USB_RM_REG(mod_no, 38,  BUSYTONE_DR38);
	WRITE_USB_RM_REG(mod_no, 39,  BUSYTONE_DR39);
	WRITE_USB_RM_REG(mod_no, 40,  BUSYTONE_DR40);
	WRITE_USB_RM_REG(mod_no, 41,  BUSYTONE_DR41);
	WRITE_USB_RM_REG(mod_no, 42,  BUSYTONE_DR42);
	WRITE_USB_RM_REG(mod_no, 43,  BUSYTONE_DR43);
  
	WRITE_USB_RM_REG(mod_no, 32,  BUSYTONE_DR32);
	WRITE_USB_RM_REG(mod_no, 33,  BUSYTONE_DR33);
	return 0;
}

static int wp_usb_remora_ringtone(sdla_fe_t *fe, int mod_no)
{

	DEBUG_RM("%s: Module %d: Enable Ring tone\n",
				fe->name, mod_no+1);
	wp_proslic_setreg_indirect(fe, mod_no, 13,RINGBACKTONE_IR13);
	wp_proslic_setreg_indirect(fe, mod_no, 14,RINGBACKTONE_IR14);
	wp_proslic_setreg_indirect(fe, mod_no, 16,RINGBACKTONE_IR16);
	wp_proslic_setreg_indirect(fe, mod_no, 17,RINGBACKTONE_IR17);
	
	WRITE_USB_RM_REG(mod_no, 36,  RINGBACKTONE_DR36);
	WRITE_USB_RM_REG(mod_no, 37,  RINGBACKTONE_DR37);
	WRITE_USB_RM_REG(mod_no, 38,  RINGBACKTONE_DR38);
	WRITE_USB_RM_REG(mod_no, 39,  RINGBACKTONE_DR39);
	WRITE_USB_RM_REG(mod_no, 40,  RINGBACKTONE_DR40);
	WRITE_USB_RM_REG(mod_no, 41,  RINGBACKTONE_DR41);
	WRITE_USB_RM_REG(mod_no, 42,  RINGBACKTONE_DR42);
	WRITE_USB_RM_REG(mod_no, 43,  RINGBACKTONE_DR43);
  
	WRITE_USB_RM_REG(mod_no, 32,  RINGBACKTONE_DR32);
	WRITE_USB_RM_REG(mod_no, 33,  RINGBACKTONE_DR33);
	return 0;
}

static int wp_usb_remora_congestiontone(sdla_fe_t *fe, int mod_no)
{

	DEBUG_RM("%s: Module %d: Enable Congestion tone\n",
				fe->name, mod_no+1);
	wp_proslic_setreg_indirect(fe, mod_no, 13,CONGESTIONTONE_IR13);
	wp_proslic_setreg_indirect(fe, mod_no, 14,CONGESTIONTONE_IR14);
	wp_proslic_setreg_indirect(fe, mod_no, 16,CONGESTIONTONE_IR16);
	wp_proslic_setreg_indirect(fe, mod_no, 17,CONGESTIONTONE_IR17);
	
	WRITE_USB_RM_REG(mod_no, 36,  CONGESTIONTONE_DR36);
	WRITE_USB_RM_REG(mod_no, 37,  CONGESTIONTONE_DR37);
	WRITE_USB_RM_REG(mod_no, 38,  CONGESTIONTONE_DR38);
	WRITE_USB_RM_REG(mod_no, 39,  CONGESTIONTONE_DR39);
	WRITE_USB_RM_REG(mod_no, 40,  CONGESTIONTONE_DR40);
	WRITE_USB_RM_REG(mod_no, 41,  CONGESTIONTONE_DR41);
	WRITE_USB_RM_REG(mod_no, 42,  CONGESTIONTONE_DR42);
	WRITE_USB_RM_REG(mod_no, 43,  CONGESTIONTONE_DR43);

	WRITE_USB_RM_REG(mod_no, 32,  CONGESTIONTONE_DR32);
	WRITE_USB_RM_REG(mod_no, 33,  CONGESTIONTONE_DR33);
	return 0;
}

static int wp_usb_remora_disabletone(sdla_fe_t* fe, int mod_no)
{
	WRITE_USB_RM_REG(mod_no, 32, 0);
	WRITE_USB_RM_REG(mod_no, 33, 0);
	WRITE_USB_RM_REG(mod_no, 36, 0);
	WRITE_USB_RM_REG(mod_no, 37, 0);
	WRITE_USB_RM_REG(mod_no, 38, 0);
	WRITE_USB_RM_REG(mod_no, 39, 0);
	WRITE_USB_RM_REG(mod_no, 40, 0);
	WRITE_USB_RM_REG(mod_no, 41, 0);
	WRITE_USB_RM_REG(mod_no, 42, 0);
	WRITE_USB_RM_REG(mod_no, 43, 0);

	return 0;
}


static int wp_usb_remora_regdump(sdla_fe_t* fe, unsigned char *data)
{
	wan_remora_udp_t	*rm_udp = (wan_remora_udp_t*)data;
	wan_remora_fxs_regs_t	*regs_fxs = NULL;
	wan_remora_fxo_regs_t	*regs_fxo = NULL;
	int			mod_no = 0, reg = 0;

	mod_no = rm_udp->mod_no;
	if (fe->rm_param.mod[mod_no].type == MOD_TYPE_FXS){

		rm_udp->type = MOD_TYPE_FXS;
		regs_fxs = &rm_udp->u.regs_fxs;
		for(reg = 0; reg < WAN_FXS_NUM_REGS; reg++){
			regs_fxs->direct[reg] = READ_USB_RM_REG(mod_no, reg);
		}

		for (reg=0; reg < WAN_FXS_NUM_INDIRECT_REGS; reg++){
			regs_fxs->indirect[reg] =
				(unsigned short)wp_proslic_getreg_indirect(fe, mod_no, (unsigned char)reg);
		}
	}else if (fe->rm_param.mod[mod_no].type == MOD_TYPE_FXO){

		rm_udp->type = MOD_TYPE_FXO;
		regs_fxo = &rm_udp->u.regs_fxo;
		for(reg = 0; reg < WAN_FXO_NUM_REGS; reg++){
			regs_fxo->direct[reg] = READ_USB_RM_REG(mod_no, reg);
		}
	}else{
		return 0;
	}
	return sizeof(wan_remora_udp_t);
}

static int wp_usb_remora_stats(sdla_fe_t* fe, unsigned char *data)
{
	wan_remora_udp_t	*rm_udp = (wan_remora_udp_t*)data;
	int			mod_no = 0;

	mod_no = rm_udp->mod_no;
	if (fe->rm_param.mod[mod_no].type == MOD_TYPE_FXS){
		rm_udp->type = MOD_TYPE_FXS;
		rm_udp->u.stats.tip_volt = READ_USB_RM_REG(mod_no, 80);
		rm_udp->u.stats.ring_volt = READ_USB_RM_REG(mod_no, 81);
		rm_udp->u.stats.bat_volt = READ_USB_RM_REG(mod_no, 82);

		rm_udp->u.stats.status = FE_CONNECTED;

	} else if (fe->rm_param.mod[mod_no].type == MOD_TYPE_FXO){
		rm_udp->type = MOD_TYPE_FXO;
		rm_udp->u.stats.volt = READ_USB_RM_REG(mod_no, 29);

		rm_udp->u.stats.status = fe->rm_param.mod[mod_no].u.fxo.status; 
	} else{
		return 0;
	}

	return sizeof(wan_remora_udp_t);
}

/*
 ******************************************************************************
 *				wp_usb_remora_udp()	
 *
 * Description:
 * Arguments:
 * Returns:
 ******************************************************************************
 */
static int wp_usb_remora_udp(sdla_fe_t *fe, void* p_udp_cmd, unsigned char* data)
{
	wan_cmd_t		*udp_cmd = (wan_cmd_t*)p_udp_cmd;
	wan_femedia_t		*fe_media = NULL;
	sdla_fe_debug_t		*fe_debug;	
	sdla_fe_timer_event_t	event;
	int			err = -EINVAL;

	memset(&event, 0, sizeof(sdla_fe_timer_event_t));
	switch(udp_cmd->wan_cmd_command){
	case WAN_GET_MEDIA_TYPE:
	
                fe_media = (wan_femedia_t*)data;
                memset(fe_media, 0, sizeof(wan_femedia_t));
                fe_media->media         = fe->fe_cfg.media;
                fe_media->sub_media     = fe->fe_cfg.sub_media;
                fe_media->chip_id       = 0x00;
                fe_media->max_ports     = 1;
                udp_cmd->wan_cmd_return_code = WAN_CMD_OK;
                udp_cmd->wan_cmd_data_len = sizeof(wan_femedia_t);
                break;

	case WAN_FE_TONES:
		event.type	= WP_RM_POLL_TONE;
		event.rm_event.mod_no	= data[0];
		if (data[1]){
			event.mode	= WAN_EVENT_ENABLE;
		}else{
			event.mode	= WAN_EVENT_DISABLE;
		}
		event.rm_event.tone	= WAN_EVENT_RM_TONE_TYPE_DIAL;
		event.delay		= WP_RM_POLL_TIMER;
		wp_usb_remora_add_event(fe, &event);
		udp_cmd->wan_cmd_return_code = WAN_CMD_OK;
		break;

	case WAN_FE_RING:
		event.type	= WP_RM_POLL_RING;
		event.mode	= WAN_EVENT_ENABLE;

		event.rm_event.mod_no	= data[0];
		if (data[1]){
			event.mode	= WAN_EVENT_ENABLE;
		}else{
			event.mode	= WAN_EVENT_DISABLE;
		}
		event.delay		= WP_RM_POLL_TIMER;
		wp_usb_remora_add_event(fe, &event);
		udp_cmd->wan_cmd_return_code = WAN_CMD_OK;
		break;

	case WAN_FE_REGDUMP:
		err = wp_usb_remora_regdump(fe, data);
		if (err){
			udp_cmd->wan_cmd_return_code = WAN_CMD_OK;
			udp_cmd->wan_cmd_data_len = (unsigned short)err; 
		}
		break;

	case WAN_FE_STATS:
		err = wp_usb_remora_stats(fe, data);
		if (err){
			udp_cmd->wan_cmd_return_code = WAN_CMD_OK;
			udp_cmd->wan_cmd_data_len = (unsigned short)err; 
		}
		break;
		
	case WAN_FE_SET_DEBUG_MODE:
		fe_debug = (sdla_fe_debug_t*)data;
		switch(fe_debug->type){
		case WAN_FE_DEBUG_REG:
			if (fe->rm_param.reg_dbg_busy){
				if (fe_debug->fe_debug_reg.read == 2 && fe->rm_param.reg_dbg_ready){
					/* Poll the register value */
					fe_debug->fe_debug_reg.value = fe->rm_param.reg_dbg_value;
					udp_cmd->wan_cmd_return_code = WAN_CMD_OK;
					fe->rm_param.reg_dbg_busy = 0;
				}
				break;
			}
			event.type		= (fe_debug->fe_debug_reg.read) ? 
							WP_RM_POLL_READ : WP_RM_POLL_WRITE;
			event.rm_event.mod_no	= fe_debug->mod_no;
			event.rm_event.reg	= (u_int16_t)fe_debug->fe_debug_reg.reg;
			event.rm_event.value	= fe_debug->fe_debug_reg.value;
			event.delay		= WP_RM_POLL_TIMER;
			if (fe_debug->fe_debug_reg.read){
				fe->rm_param.reg_dbg_busy = 1;
				fe->rm_param.reg_dbg_ready = 0;
			}
			wp_usb_remora_add_event(fe, &event);
			udp_cmd->wan_cmd_return_code = WAN_CMD_OK;
			break;

		case WAN_FE_DEBUG_HOOK:
			event.type		= (fe_debug->fe_debug_hook.offhook) ? 
							WP_RM_POLL_TXSIG_OFFHOOK:
							WP_RM_POLL_TXSIG_ONHOOK;
			event.rm_event.mod_no	= fe_debug->mod_no;
			wp_usb_remora_add_event(fe, &event);
			udp_cmd->wan_cmd_return_code = WAN_CMD_OK;
			break;
			
		default:	
			udp_cmd->wan_cmd_return_code = WAN_UDP_INVALID_CMD;
		    	udp_cmd->wan_cmd_data_len = 0;
			break;
		}
		break;
		
	default:
		udp_cmd->wan_cmd_return_code = WAN_UDP_INVALID_CMD;
	    	udp_cmd->wan_cmd_data_len = 0;
		break;
	}
	return 0;
}


static int wp_usb_init_proslic_recheck_sanity(sdla_fe_t *fe, int mod_no)
{
	int	res;
		
	res = READ_USB_RM_REG(mod_no, 64);
	if (!res && (res != fe->rm_param.mod[mod_no].u.fxs.lasttxhook)) {
		res = READ_USB_RM_REG(mod_no, 8);
		if (res) {
			sdla_fe_timer_event_t	event;
			DEBUG_EVENT(
			"%s: Module %d: Ouch, part reset, quickly restoring reality\n",
					fe->name, mod_no+1);
			event.type		= WP_RM_POLL_INIT;
			event.delay		= WP_RM_POLL_TIMER;
			event.rm_event.mod_no	= mod_no;
			wp_usb_remora_add_event(fe, &event);
			return 1;
			//wp_usb_init_proslic(fe, mod_no, 1, 1);
		} else {
			if (fe->rm_param.mod[mod_no].u.fxs.palarms++ < MAX_ALARMS) {
				DEBUG_EVENT(
				"%s: Module %d: Power alarm, resetting!\n",
					fe->name, mod_no + 1);
				if (fe->rm_param.mod[mod_no].u.fxs.lasttxhook == 4){
					fe->rm_param.mod[mod_no].u.fxs.lasttxhook = 1;
				}
				WRITE_USB_RM_REG(mod_no, 64, fe->rm_param.mod[mod_no].u.fxs.lasttxhook);
			} else {
				if (fe->rm_param.mod[mod_no].u.fxs.palarms == MAX_ALARMS){
					DEBUG_EVENT(
					"%s: Module %d: Too many power alarms, NOT resetting!\n",
						fe->name, mod_no + 1);
				}
			}
		}
	}
	return 0;
}

#if defined(AFT_TDM_API_SUPPORT) || defined(AFT_API_SUPPORT)
static void wp_usb_remora_voicedaa_check_hook(sdla_fe_t *fe, int mod_no)
{
//#undef DEBUG_RM
//#define DEBUG_RM DEBUG_EVENT

	sdla_t		*card = NULL;
	wan_event_t	event;
#ifndef AUDIO_RINGCHECK
	unsigned char res;
#endif	
	signed char b;
	int poopy = 0;

	WAN_ASSERT1(fe->card == NULL);
	card	= (sdla_t*)fe->card;

	/* Try to track issues that plague slot one FXO's */
	b = READ_USB_RM_REG(mod_no, 5);
	if ((b & 0x2) || !(b & 0x8)) {
		/* Not good -- don't look at anything else */
		DEBUG_RM("%s: Module %d: Poopy (%02x)!\n",
				fe->name, mod_no + 1, b); 
		poopy++;
	}
	b &= 0x9b;
	if (fe->rm_param.mod[mod_no].u.fxo.offhook) {
		if (b != 0x9){
			WRITE_USB_RM_REG(mod_no, 5, 0x9);
		}
	} else {
		if (b != 0x8){
			WRITE_USB_RM_REG(mod_no, 5, 0x8);
		}
	}
	if (poopy)
		return;

#ifndef AUDIO_RINGCHECK
	if (!fe->rm_param.mod[mod_no].u.fxo.offhook) {
		res = READ_USB_RM_REG(mod_no, 5);
		if ((res & 0x60) && fe->rm_param.mod[mod_no].u.fxo.battery) {
			fe->rm_param.mod[mod_no].u.fxo.ringdebounce += (WP_RM_CHUNKSIZE * 4);
			if (fe->rm_param.mod[mod_no].u.fxo.ringdebounce >= WP_RM_CHUNKSIZE * 64) {
				if (!fe->rm_param.mod[mod_no].u.fxo.wasringing) {
					fe->rm_param.mod[mod_no].u.fxo.wasringing = 1;
					DEBUG_RM("%s: Module %d: RING!\n",
							fe->name,
							mod_no + 1);
					event.type	= WAN_EVENT_RM_RING_DETECT;
					event.channel	= mod_no+1;
					event.ring_mode	= WAN_EVENT_RING_PRESENT;
					if (card->wandev.event_callback.ringdetect){
						card->wandev.event_callback.ringdetect(card, &event);
					}	
								
				}
				fe->rm_param.mod[mod_no].u.fxo.ringdebounce = WP_RM_CHUNKSIZE * 64;
			}
		} else {
			fe->rm_param.mod[mod_no].u.fxo.ringdebounce -= WP_RM_CHUNKSIZE * 2;
			if (fe->rm_param.mod[mod_no].u.fxo.ringdebounce <= 0) {
				if (fe->rm_param.mod[mod_no].u.fxo.wasringing) {
					fe->rm_param.mod[mod_no].u.fxo.wasringing = 0;
					DEBUG_RM("%s: Module %d: NO RING!\n",
							fe->name,
							mod_no + 1);
					event.type	= WAN_EVENT_RM_RING_DETECT;
					event.channel	= mod_no+1;
					event.ring_mode	= WAN_EVENT_RING_STOP;
					if (card->wandev.event_callback.ringdetect){
						card->wandev.event_callback.ringdetect(card, &event);
					}	
				}
				fe->rm_param.mod[mod_no].u.fxo.ringdebounce = 0;
			}
		}
	}
#endif

	b = READ_USB_RM_REG(mod_no, 29);
	if (abs(b) < battthresh) {
		fe->rm_param.mod[mod_no].u.fxo.nobatttimer++;
#if 0
		if (wr->mod[mod_no].fxo.battery)
			printk("Battery loss: %d (%d debounce)\n", b, wr->mod[mod_no].fxo.battdebounce);
#endif
		if (fe->rm_param.mod[mod_no].u.fxo.battery &&
		    !fe->rm_param.mod[mod_no].u.fxo.battdebounce) {
			DEBUG_RM("%s Module %d: NO BATTERY!\n",
						fe->name,
						mod_no + 1);
			fe->rm_param.mod[mod_no].u.fxo.battery =  0;
#ifdef	JAPAN
			if ((!fe->rm_param.mod[mod_no].u.fxo.ohdebounce) &&
			     fe->rm_param.mod[mod_no].u.fxo.offhook) {
				DEBUG_RM("%s: Module %d: On-Hook status!\n",
							fe->name,
							mod_no + 1);
				event.type	= WAN_EVENT_RM_LC;
				event.channel	= mod_no+1;
				event.rxhook	= WAN_EVENT_RXHOOK_ON;
				if (card->wandev.event_callback.hook){
					card->wandev.event_callback.hook(
								card, &event);
				}
							
#ifdef	ZERO_BATT_RING
				fe->rm_param.mod[mod_no].u.fxo.onhook++;
#endif
			}
#else
			DEBUG_RM("%s: Module %d: On-Hook status!\n",
							fe->name,
							mod_no + 1);
			event.type	= WAN_EVENT_RM_LC;
			event.channel	= mod_no+1;
			event.rxhook	= WAN_EVENT_RXHOOK_ON;
			if (card->wandev.event_callback.hook){
				card->wandev.event_callback.hook(card, &event);
			}
#endif
			fe->rm_param.mod[mod_no].u.fxo.battdebounce = battdebounce;
		} else if (!fe->rm_param.mod[mod_no].u.fxo.battery)
			fe->rm_param.mod[mod_no].u.fxo.battdebounce = battdebounce;

	} else if (abs(b) > battthresh) {

		if (!fe->rm_param.mod[mod_no].u.fxo.battery &&
		    !fe->rm_param.mod[mod_no].u.fxo.battdebounce) {
			DEBUG_RM("%s: Module %d: BATTERY (%s)!\n",
						fe->name,
						mod_no + 1,
						(b < 0) ? "-" : "+");			    
#ifdef	ZERO_BATT_RING
			if (fe->rm_param.mod[mod_no].u.fxo.onhook) {
				fe->rm_param.mod[mod_no].u.fxo.onhook = 0;
				DEBUG_RM("%s: Module %d: Off-Hook status!\n",
							fe->name,
							mod_no + 1);
				event.type	= WAN_EVENT_RM_LC;
				event.channel	= mod_no+1;
				event.rxhook	= WAN_EVENT_RXHOOK_OFF;
				if (card->wandev.event_callback.hook){
					card->wandev.event_callback.hook(card, &event);
				}		
			}
#else
			DEBUG_RM("%s: Module %d: Off-Hook status!\n",
						fe->name,
						mod_no + 1);
			event.type	= WAN_EVENT_RM_LC;
			event.channel	= mod_no+1;
			event.rxhook	= WAN_EVENT_RXHOOK_OFF;
			if (card->wandev.event_callback.hook){
				card->wandev.event_callback.hook(card, &event);
			}
#endif
			fe->rm_param.mod[mod_no].u.fxo.battery = 1;
			fe->rm_param.mod[mod_no].u.fxo.nobatttimer = 0;
			fe->rm_param.mod[mod_no].u.fxo.battdebounce = battdebounce;
		} else if (fe->rm_param.mod[mod_no].u.fxo.battery)
			fe->rm_param.mod[mod_no].u.fxo.battdebounce = battdebounce;

		if (fe->rm_param.mod[mod_no].u.fxo.lastpol >= 0) {
		    if (b < 0) {
			fe->rm_param.mod[mod_no].u.fxo.lastpol = -1;
			fe->rm_param.mod[mod_no].u.fxo.polaritydebounce = POLARITY_DEBOUNCE;
		    }
		} 
		if (fe->rm_param.mod[mod_no].u.fxo.lastpol <= 0) {
		    if (b > 0) {
			fe->rm_param.mod[mod_no].u.fxo.lastpol = 1;
			fe->rm_param.mod[mod_no].u.fxo.polaritydebounce = POLARITY_DEBOUNCE;
		    }
		}
	} else {
		/* It's something else... */
		fe->rm_param.mod[mod_no].u.fxo.battdebounce = battdebounce;
	}
	if (fe->rm_param.mod[mod_no].u.fxo.battdebounce)
		fe->rm_param.mod[mod_no].u.fxo.battdebounce--;
	if (fe->rm_param.mod[mod_no].u.fxo.polaritydebounce) {
	        fe->rm_param.mod[mod_no].u.fxo.polaritydebounce--;
		if (fe->rm_param.mod[mod_no].u.fxo.polaritydebounce < 1) {
			if (fe->rm_param.mod[mod_no].u.fxo.lastpol !=
					fe->rm_param.mod[mod_no].u.fxo.polarity) {
				DEBUG_RM(
				"%s: Module %d: Polarity reversed (%d -> %d) (%ul)\n",
						fe->name, mod_no + 1,
						fe->rm_param.mod[mod_no].u.fxo.polarity, 
						fe->rm_param.mod[mod_no].u.fxo.lastpol,
						(unsigned int)SYSTEM_TICKS);
				if (fe->rm_param.mod[mod_no].u.fxo.polarity){
					/* FIXME: Add revers polarity event */
				}
				fe->rm_param.mod[mod_no].u.fxo.polarity =
						fe->rm_param.mod[mod_no].u.fxo.lastpol;
			}
		}
	}
	return;
//#undef DEBUG_RM
//#define DEBUG_RM DEBUG_TEST
}
#endif

#if 0
#if !defined(AFT_RM_INTR_SUPPORT)
static void wp_usb_remora_proslic_check_hook(sdla_fe_t *fe, int mod_no)
{
	sdla_t		*card = NULL;	
	wan_event_t	event;
	int		hook;
	char		res;

	WAN_ASSERT1(fe->card == NULL);
	card	= fe->card;
	/* For some reason we have to debounce the
	   hook detector.  */

	res = READ_USB_RM_REG(mod_no, 68);
	hook = (res & 1);
	if (hook != fe->rm_param.mod[mod_no].u.fxs.lastrxhook) {
		/* Reset the debounce (must be multiple of 4ms) */
		fe->rm_param.mod[mod_no].u.fxs.debounce = 4 * (4 * 8);
		DEBUG_EVENT(
		"%s: Module %d: Resetting debounce hook %d, %d\n",
				fe->name, mod_no + 1, hook,
				fe->rm_param.mod[mod_no].u.fxs.debounce);
	} else {
		if (fe->rm_param.mod[mod_no].u.fxs.debounce > 0) {
			fe->rm_param.mod[mod_no].u.fxs.debounce-= 16 * WP_RM_CHUNKSIZE;
			DEBUG_EVENT(
			"%s: Module %d: Sustaining hook %d, %d\n",
					fe->name, mod_no + 1,
					hook, fe->rm_param.mod[mod_no].u.fxs.debounce);
			if (!fe->rm_param.mod[mod_no].u.fxs.debounce) {
				DEBUG_EVENT(
				"%s: Module %d: Counted down debounce, newhook: %d\n",
							fe->name,
							mod_no + 1,
							hook);
				fe->rm_param.mod[mod_no].u.fxs.debouncehook = hook;
			}
			if (!fe->rm_param.mod[mod_no].u.fxs.oldrxhook &&
			    fe->rm_param.mod[mod_no].u.fxs.debouncehook) {
				/* Off hook */
				DEBUG_RM("%s: Module %d: Going off hook\n",
							fe->name, mod_no + 1);
				event.type	= WAN_EVENT_RM_LC;
				event.channel	= mod_no+1;
				event.rxhook	= WAN_EVENT_RXHOOK_OFF;
				if (card->wandev.event_callback.hook){
					card->wandev.event_callback.hook(card, &event);
				}		
				//zt_hooksig(&wr->chans[mod_no], ZT_RXSIG_OFFHOOK);
#if 0
				if (robust)
					wp_usb_init_proslic(wc, card, 1, 0, 1);
#endif
				fe->rm_param.mod[mod_no].u.fxs.oldrxhook = 1;
			
			} else if (fe->rm_param.mod[mod_no].u.fxs.oldrxhook &&
				   !fe->rm_param.mod[mod_no].u.fxs.debouncehook) {
				/* On hook */
				DEBUG_RM("%s: Module %d: Going on hook\n",
							fe->name, mod_no + 1);
				event.type	= WAN_EVENT_RM_LC;
				event.channel	= mod_no+1;
				event.rxhook	= WAN_EVENT_RXHOOK_OFF;
				if (card->wandev.event_callback.hook){
					card->wandev.event_callback.hook(card, &event);
				}		
				//zt_hooksig(&wr->chans[mod_no], ZT_RXSIG_ONHOOK);
				fe->rm_param.mod[mod_no].u.fxs.oldrxhook = 0;
			}
		}
	}
	fe->rm_param.mod[mod_no].u.fxs.lastrxhook = hook;
}
#endif
#endif

#if defined(AFT_TDM_API_SUPPORT) || defined(AFT_API_SUPPORT)
/******************************************************************************
*			wp_usb_remora_watchdog()	
*
* Description:
* Arguments: mod_no -  Module number (1,2,3,... MAX_USB_REMORA_MODULES)
* Returns:
******************************************************************************/
static int wp_usb_remora_watchdog(void *card_ptr)
{
	int	mod_no;
	sdla_t			*card = (sdla_t*)card_ptr;
	sdla_fe_t		*fe  = &card->fe;

	for (mod_no = 0; mod_no < fe->rm_param.max_fe_channels; mod_no++) {
		if (!wan_test_bit(mod_no, &fe->rm_param.module_map)) {
			continue;
		}
		if (fe->rm_param.mod[mod_no].type == MOD_TYPE_FXS){

			if (fe->rm_param.mod[mod_no].u.fxs.lasttxhook == 0x4) {
				/* RINGing, prepare for OHT */
				fe->rm_param.mod[mod_no].u.fxs.ohttimer = OHT_TIMER << 3;
				if (fe->fe_cfg.cfg.remora.reversepolarity){
					/* OHT mode when idle */
					fe->rm_param.mod[mod_no].u.fxs.idletxhookstate = 0x6;
				}else{
					fe->rm_param.mod[mod_no].u.fxs.idletxhookstate = 0x2; 
				}
			} else {
				if (fe->rm_param.mod[mod_no].u.fxs.ohttimer) {
					fe->rm_param.mod[mod_no].u.fxs.ohttimer-= WP_RM_CHUNKSIZE;
					if (!fe->rm_param.mod[mod_no].u.fxs.ohttimer) {
						if (fe->fe_cfg.cfg.remora.reversepolarity){
							/* Switch to active */
							fe->rm_param.mod[mod_no].u.fxs.idletxhookstate = 0x5;
						}else{
							fe->rm_param.mod[mod_no].u.fxs.idletxhookstate = 0x1;
						}
						if ((fe->rm_param.mod[mod_no].u.fxs.lasttxhook == 0x2) ||
						    (fe->rm_param.mod[mod_no].u.fxs.lasttxhook = 0x6)) {
							/* Apply the change if appropriate */
							if (fe->fe_cfg.cfg.remora.reversepolarity){ 
								fe->rm_param.mod[mod_no].u.fxs.lasttxhook = 0x5;
							}else{
								fe->rm_param.mod[mod_no].u.fxs.lasttxhook = 0x1;
							}
							WRITE_USB_RM_REG(mod_no, 64, fe->rm_param.mod[mod_no].u.fxs.lasttxhook);
						}
					}
				}
			}

		} else if (fe->rm_param.mod[mod_no].type == MOD_TYPE_FXO) {

#if 0
			if (wr->mod[x].fxo.echotune){
				DEBUG_EVENT("%s: Module %d: Setting echo registers: \n",
							fe->name, x);

				/* Set the ACIM register */
				B_RM_REG(x, 30, wr->mod[x].fxo.echoregs.acim);

				/* Set the digital echo canceller registers */
				B_RM_REG(x, 45, wr->mod[x].fxo.echoregs.coef1);
				B_RM_REG(x, 46, wr->mod[x].fxo.echoregs.coef2);
				B_RM_REG(x, 47, wr->mod[x].fxo.echoregs.coef3);
				B_RM_REG(x, 48, wr->mod[x].fxo.echoregs.coef4);
				B_RM_REG(x, 49, wr->mod[x].fxo.echoregs.coef5);
				B_RM_REG(x, 50, wr->mod[x].fxo.echoregs.coef6);
				B_RM_REG(x, 51, wr->mod[x].fxo.echoregs.coef7);
				B_RM_REG(x, 52, wr->mod[x].fxo.echoregs.coef8);

				DEBUG_EVENT("%s: Module %d: Set echo registers successfully\n",
						fe->name, x);
				wr->mod[x].fxo.echotune = 0;
			}
#endif			
			wp_usb_remora_voicedaa_check_hook(fe, mod_no);
		}
	}
	
#if defined(AFT_RM_VIRTUAL_INTR_SUPPORT)
	if (SYSTEM_TICKS - fe->rm_param.last_watchdog  > WP_RM_WATCHDOG_TIMEOUT) {
		fe->rm_param.last_watchdog = SYSTEM_TICKS;

		if (wp_usb_remora_check_intr(fe)){
			wp_usb_remora_intr(fe);
		}
	}
#endif

	return 0;
}
#endif


/******************************************************************************
 *				wp_remora_get_link_status()	
 *
 * Description:
 * Arguments:	
 * Returns:
 ******************************************************************************
 */
static int wp_usb_remora_get_link_status(sdla_fe_t *fe, unsigned char *status,int mod_no)
{
	/*mod_no = chan -1 */
	if (fe->rm_param.mod[mod_no-1].type == MOD_TYPE_FXO) {
		*status = fe->rm_param.mod[mod_no -1].u.fxo.status;
		
	} else {
		/* FXS module does not have a state. Thus its always connected */
		*status=FE_CONNECTED;
	}
	return 0;
}


/******************************************************************************
*				wp_usb_remora_intr_ctrl()	
*
* Description: Enable/Disable extra interrupt types
* Arguments: mod_no -  Module number (1,2,3,... MAX_USB_REMORA_MODULES)
* Returns:
******************************************************************************/
static int
wp_usb_remora_intr_ctrl(sdla_fe_t *fe, int mod_no, u_int8_t type, u_int8_t mode, unsigned int ts_map)
{
	int		err = 0;

	if (mod_no >= MAX_USB_REMORA_MODULES){
		DEBUG_EVENT(
		"%s: Module %d: Module number is out of range!\n",
					fe->name, mod_no+1);
		return -EINVAL;
	}
	if (!wan_test_bit(mod_no, &fe->rm_param.module_map)) {
		DEBUG_EVENT("%s: Module %d: Unconfigured module!\n",
					fe->name, mod_no+1);
		return -EINVAL;
	}
	if (fe->rm_param.mod[mod_no].type == MOD_TYPE_FXS){

		switch(type){
		case WAN_RM_INTR_GLOBAL:
			if (mode == WAN_FE_INTR_ENABLE){
				WRITE_USB_RM_REG(mod_no, 21, fe->rm_param.mod[mod_no].u.fxs.imask1);
				WRITE_USB_RM_REG(mod_no, 22, fe->rm_param.mod[mod_no].u.fxs.imask2);
				WRITE_USB_RM_REG(mod_no, 23, fe->rm_param.mod[mod_no].u.fxs.imask3);
			}else{
				WRITE_USB_RM_REG(mod_no, 21, 0x00);
				WRITE_USB_RM_REG(mod_no, 22, 0x00);
				WRITE_USB_RM_REG(mod_no, 23, 0x00);
			}
			break;
		default:
			DEBUG_EVENT(
			"%s: Module %d: Unsupported FXS interrupt type (%X)!\n",
					fe->name, mod_no+1, type);
			err = -EINVAL;
			break;
		}

	}else if (fe->rm_param.mod[mod_no].type == MOD_TYPE_FXO){
		switch(type){
		case WAN_RM_INTR_GLOBAL:
			if (mode == WAN_FE_INTR_ENABLE){
				WRITE_USB_RM_REG(mod_no, 3, fe->rm_param.mod[mod_no].u.fxo.imask);
			}else{
				WRITE_USB_RM_REG(mod_no, 3, 0x00);
			}
			break;
		default:
			DEBUG_EVENT(
			"%s: Module %d: Unsupported FXO interrupt type (%X)!\n",
					fe->name, mod_no+1, type);
			err = -EINVAL;
			break;
		}
	}
	return err;

}

static int wp_usb_remora_check_intr_fxs(sdla_fe_t *fe, int mod_no)
{
	unsigned char	stat1 = 0x0, stat2 = 0x00, stat3 = 0x00;
	
#if 1	
	if (wp_usb_init_proslic_recheck_sanity(fe, mod_no)){
		return 0;
	}
#else
	if (__READ_USB_RM_REG(mod_no, 8)){
		sdla_fe_timer_event_t	event;
		int	err = 0;
		DEBUG_EVENT(
		"%s: Module %d: Oops, part reset, quickly restoring reality\n",
				fe->name, mod_no+1);
#if 0
		wp_usb_init_proslic(fe, mod_no, 1, 1);
#else
		event.type		= WP_RM_POLL_INIT;
		event.delay		= WP_RM_POLL_TIMER;
		event.rm_event.mod_no	= mod_no;
		wp_usb_remora_add_event(fe, &event);
#endif
		return 0;
	}
#endif
	stat1 = READ_USB_RM_REG(mod_no, 18);
	stat2 = READ_USB_RM_REG(mod_no, 19);
	stat3 = READ_USB_RM_REG(mod_no, 20);

	if (stat1 || stat2 || stat3) return 1;
	return 0;	
}

static int wp_usb_remora_check_intr_fxo(sdla_fe_t *fe, int mod_no)
{
	u_int8_t	status;
	
	status = READ_USB_RM_REG(mod_no, 4);
	return (status) ? 1 : 0;
}

static int wp_usb_remora_check_intr(sdla_fe_t *fe)
{
	int	mod_no = 0, pending = 0;

	fe->rm_param.intcount++;
	for(mod_no = 0; mod_no < MAX_USB_REMORA_MODULES; mod_no++){
		if (wan_test_bit(mod_no, &fe->rm_param.module_map)) {
			if (fe->rm_param.mod[mod_no].type == MOD_TYPE_FXS){
				pending = wp_usb_remora_check_intr_fxs(fe, mod_no);
			}else if (fe->rm_param.mod[mod_no].type == MOD_TYPE_FXO){
				pending = wp_usb_remora_check_intr_fxo(fe, mod_no);
			}
			if (pending){
				return pending;
			}
		}
	}
#if 0
	if (card->wan_tdmv.sc && fe->rm_param.intcount % 100 == 0){
		wp_usb_remora_enable_timer(fe, mod_no, WP_RM_POLL_TDMV, WP_RM_POLL_TIMER);
	}
#endif
	return 0;
} 

static int wp_usb_remora_read_dtmf(sdla_fe_t *fe, int mod_no)
{
	sdla_t		*card;
	unsigned char	status;

	WAN_ASSERT(fe->card == NULL);
	card		= fe->card;
	status = READ_USB_RM_REG(mod_no, 24);
	if (status & 0x10){
		unsigned char	digit = 0xFF;
		status &= 0xF;
		switch(status){
		case 0x01: digit = '1'; break;
		case 0x02: digit = '2'; break;
		case 0x03: digit = '3'; break;
		case 0x04: digit = '4'; break;
		case 0x05: digit = '5'; break;
		case 0x06: digit = '6'; break;
		case 0x07: digit = '7'; break;
		case 0x08: digit = '8'; break;
		case 0x09: digit = '9'; break;
		case 0x0A: digit = '0'; break;
		case 0x0B: digit = '*'; break;
		case 0x0C: digit = '#'; break;
		case 0x0D: digit = 'A'; break;
		case 0x0E: digit = 'B'; break;
		case 0x0F: digit = 'C'; break;
		case 0x00: digit = 'D'; break;
		}
		DEBUG_RM("%s: Module %d: TDMV digit %c!\n",
				fe->name,
				mod_no + 1,
				digit);
		WRITE_USB_RM_REG(mod_no, 20, 0x1);
		if (card->wandev.event_callback.tone){
			wan_event_t	event;
			event.type	= WAN_EVENT_RM_DTMF;
			event.digit	= digit;
			event.channel	= mod_no+1;
			event.tone_port = WAN_EC_CHANNEL_PORT_SOUT;
			event.tone_type = WAN_EC_TONE_STOP;
			card->wandev.event_callback.tone(card, &event);
		}
	}
	return 0;
}

static int wp_usb_remora_intr_fxs(sdla_fe_t *fe, int mod_no)
{
	sdla_t		*card = NULL;
	wan_event_t	event;
	unsigned char	stat1 = 0x0, stat2 = 0x00, stat3 = 0x00;

	WAN_ASSERT(fe->card == NULL);
	card		= fe->card;

	stat1 = READ_USB_RM_REG(mod_no, 18);
	if (stat1){
		/* Ack interrupts for now */
		WRITE_USB_RM_REG(mod_no, 18, stat1);
	}

	stat2 = READ_USB_RM_REG(mod_no, 19);
	if (stat2){
		unsigned char	status = READ_USB_RM_REG(mod_no, 68);

		if (stat2 & 0x02){
			DEBUG_RM(
			"%s: Module %d: LCIP interrupt pending!\n",
				fe->name, mod_no+1);
#if 0
			if (card->wandev.fe_notify_iface.hook_state){
				card->wandev.fe_notify_iface.hook_state(
						fe, mod_no, status & 0x01);
			}
#endif
			event.type	= WAN_EVENT_RM_LC;
			event.channel	= mod_no+1;
			if (status & 0x01){
				DEBUG_RM(
				"RM: %s: Module %d: Off-hook status!\n",
						fe->name, mod_no+1);
				fe->rm_param.mod[mod_no].u.fxs.oldrxhook = 1;
				event.rxhook	= WAN_EVENT_RXHOOK_OFF;
			}else/* if (fe->rm_param.mod[mod_no].u.fxs.oldrxhook)*/{
				DEBUG_RM(
				"RM: %s: Module %d: On-hook status!\n",
						fe->name, mod_no+1);
				fe->rm_param.mod[mod_no].u.fxs.oldrxhook = 0;
				event.rxhook	= WAN_EVENT_RXHOOK_ON;
			}
			if (card->wandev.event_callback.hook){
				card->wandev.event_callback.hook(
							card,
						       	&event);
			}
		}
		if (stat2 & 0x01){
			DEBUG_RM(
			"%s: Module %d: Ring TRIP interrupt pending!\n",
				fe->name, mod_no+1);
#if 0
			if (card->wandev.fe_notify_iface.hook_state){
				card->wandev.fe_notify_iface.hook_state(
						fe, mod_no, status & 0x01);
			}
#endif
			/*
			A ring trip event signals that the terminal equipment has gone
			off-hook during the ringing state.
			At this point the application should stop the ring because the
			call was answered.
			*/
			event.type	= WAN_EVENT_RM_RING_TRIP;
			event.channel	= mod_no+1;
			if (status & 0x02){
				DEBUG_RM(
				"%s: Module %d: Ring Trip detect occured!\n",
						fe->name, mod_no+1);
				event.ring_mode	= WAN_EVENT_RING_TRIP_PRESENT;
			}else{
				DEBUG_RM(
				"%s: Module %d: Ring Trip detect not occured!\n",
						fe->name, mod_no+1);
				event.ring_mode	= WAN_EVENT_RING_TRIP_STOP;
			}
			if (card->wandev.event_callback.ringtrip){
				card->wandev.event_callback.ringtrip(card, &event);
			}
		}
		if (stat2 & 0x80) {
			DEBUG_EVENT("%s: Module %d: Power Alarm Q6!\n",
					fe->name, mod_no+1);
		}
		if (stat2 & 0x40) {
			DEBUG_EVENT("%s: Module %d: Power Alarm Q5!\n",
					fe->name, mod_no+1);
		}
		if (stat2 & 0x20) {
			DEBUG_EVENT("%s: Module %d: Power Alarm Q4!\n",
					fe->name, mod_no+1);
		}
		if (stat2 & 0x10) {
			DEBUG_EVENT("%s: Module %d: Power Alarm Q3!\n",
					fe->name, mod_no+1);
		}
		if (stat2 & 0x08) {
			DEBUG_EVENT("%s: Module %d: Power Alarm Q2!\n",
					fe->name, mod_no+1);
		}
		if (stat2 & 0x04) {
			DEBUG_EVENT("%s: Module %d: Power Alarm Q1!\n",
					fe->name, mod_no+1);
		}
		DEBUG_RM(
		"%s: Module %d: Reg[64]=%02X Reg[68]=%02X\n",
			fe->name, mod_no+1,
			READ_USB_RM_REG(mod_no,64),
			status);
		WRITE_USB_RM_REG(mod_no, 19, stat2);
	}else{
#if defined(__WINDOWS__)
		TDM_FUNC_DBG();
#endif
	}

	stat3 = READ_USB_RM_REG(mod_no, 20);
	if (stat3){
		if (stat3 & 0x1){
			wp_usb_remora_read_dtmf(fe, mod_no);
		}
		WRITE_USB_RM_REG(mod_no, 20, stat3);
	}
	return 0;
}

static int wp_usb_remora_intr_fxo(sdla_fe_t *fe, int mod_no)
{
	sdla_t		*card = NULL;
	wan_event_t	event;
	u_int8_t	status;

	WAN_ASSERT(fe->card == NULL);
	card		= fe->card;
	
	status = READ_USB_RM_REG(mod_no, 4);
	if (status & 0x80){
		u_int8_t	mode;
		mode = READ_USB_RM_REG(mod_no, 2);
		event.type	= WAN_EVENT_RM_RING_DETECT;
		event.channel	= mod_no+1;
		if (mode & 0x04){
			if (fe->rm_param.mod[mod_no].u.fxo.ring_detect){
				DEBUG_RM(
				"%s: Module %d: End of ring burst\n",
					fe->name, mod_no+1);
				fe->rm_param.mod[mod_no].u.fxo.ring_detect = 0;			
				event.ring_mode	= WAN_EVENT_RING_STOP;
			} else {
				DEBUG_RM(
				"%s: Module %d: Beginning of ring burst\n",
					fe->name, mod_no+1);
				fe->rm_param.mod[mod_no].u.fxo.ring_detect = 1;
				event.ring_mode	= WAN_EVENT_RING_PRESENT;
			}
		} else {
			DEBUG_RM(
			"%s: Module %d: Beginning of ring burst\n",
				fe->name, mod_no+1);
			fe->rm_param.mod[mod_no].u.fxo.ring_detect = 1;
			event.ring_mode	= WAN_EVENT_RING_PRESENT;
		}
#ifdef AUDIO_RINGCHECK
/* If audio ringcheck is enabled, we use this code
 * if disabled we use the check_hook code. 
 * Right now we use the check_hook code because
 * this code does not have debounce */
		if (card->wandev.event_callback.ringdetect){
			card->wandev.event_callback.ringdetect(card, &event);
		}
#endif	
	}
	if (status & 0x01){
		DEBUG_RM("%s: Module %d: Polarity reversed!\n", 
					fe->name, mod_no+1);
	}
	WRITE_USB_RM_REG(mod_no, 4, 0x00);

	return 0;
}

static int wp_usb_remora_intr(sdla_fe_t *fe)
{
	int	mod_no = 0;

	for(mod_no = 0; mod_no < MAX_USB_REMORA_MODULES; mod_no++){
		if (!wan_test_bit(mod_no, &fe->rm_param.module_map)) {
			continue;
		}
		if (fe->rm_param.mod[mod_no].type == MOD_TYPE_FXS){
			wp_usb_remora_intr_fxs(fe, mod_no);
		}else if (fe->rm_param.mod[mod_no].type == MOD_TYPE_FXO){
			wp_usb_remora_intr_fxo(fe, mod_no);
		}
	}

	return 0;
}


