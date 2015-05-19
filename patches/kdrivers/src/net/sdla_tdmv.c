/***************************************************************************
 * sdla_tdmv.c	WANPIPE(tm) Multiprotocol WAN Link Driver. 
 *				TDM voice board configuration.
 *
 * Author: 	Alex Feldman   <al.feldman@sangoma.com>
 *              Nenad Corbic   <ncorbic@sangoma.com>
 *              David Rokvargh <davidr@sangoma.com>
 *
 * Copyright:	(c) 1995-2009 Sangoma Technologies Inc.
 *
 *		This program is free software; you can redistribute it and/or
 *		modify it under the terms of the GNU General Public License
 *		as published by the Free Software Foundation; either version
 *		2 of the License, or (at your option) any later version.
 * ============================================================================
 * Jul 22, 2001	Nenad Corbic	Initial version.
 * Oct 01, 2001 Gideon Hack	Modifications for interrupt usage.
 * Aug  9, 2005	David Rokhvarg	Added Echo Detection and Control (EDAC).
 * Sep 06, 2008	Moises Silva    DAHDI support.
 ******************************************************************************
 */
/*
 **********CONFIGURING********************************************************************
			   INCLUDE FILES
 ******************************************************************************
*/

# include "wanpipe_includes.h"
# include "wanpipe_defines.h"
# include "wanpipe_debug.h"
# include "wanpipe_abstr.h"
# include "wanpipe_common.h"
# include "wanpipe_events.h"
# include "wanpipe.h"
# include "wanpipe_events.h"
# include "if_wanpipe_common.h"	/* for 'wanpipe_common_t' used in 'aft_core.h'*/
# include "sdla_remora.h"
# include "wanpipe_cdev_iface.h"

#if defined (__WINDOWS__)
# include <wanpipe\csu_dsu.h>
#else
# include "zapcompat.h" /* Map of Zaptel -> DAHDI definitions */
# include "wanpipe_dahdi_abstr.h"
#endif
/*
 ******************************************************************************
			  DEFINES AND MACROS
 ******************************************************************************
*/
#define DEBUG_ECHO	if(0) DEBUG_EVENT

#define WP_MAX_CARDS		32

#define FIXME_MSG(func)		DEBUG_EVENT("(%s): FIXME: line %d\n", func, __LINE__)
#define DBG_FUNC_START(func)	DEBUG_EVENT("(DEBUG)(TDM Voice): %s - Start\n", func)
#define DBG_FUNC_END(func)	DEBUG_EVENT("(DEBUG)(TDM Voice): %s - End\n", func)

/* flags bits */
#define WP_TDMV_REGISTER	1	/*0x01*/
#define WP_TDMV_RUNNING		2	/*0x02*/
#define WP_TDMV_UP		3	/*0x04*/
#define WP_TDMV_SIG_ENABLE	4	/*0x08*/
#define WP_TDMV_SIG_POLL	5	/*0x10*/
#define WP_TDMV_RBS_READY	6	/*0x20*/
#define WP_TDMV_RBS_BUSY	7	/*0x40*/
#define WP_TDMV_RBS_UPDATE	8	/*0x80*/

#define IS_TDMV_RUNNING(wp)	wan_test_bit(WP_TDMV_RUNNING, &(wp)->flags)
#define IS_TDMV_UP(wp)		wan_test_bit(WP_TDMV_UP, &(wp)->flags)
#define IS_TDMV_SIG_POLL(wp)	wan_test_bit(WP_TDMV_SIG_POLL, &(wp)->flags)
#define IS_TDMV_SIG_ENABLE(wp)	wan_test_bit(WP_TDMV_SIG_ENABLE, &(wp)->flags)
#define IS_TDMV_UP_RUNNING(wp)	(IS_TDMV_UP(wp) && IS_TDMV_RUNNING(wp))
#define IS_TDMV_SIG(wp)		(IS_TDMV_SIG_POLL(wp) && IS_TDMV_SIG_ENABLE(wp))
#define IS_TDMV_RBS_READY(wp)	wan_test_bit(WP_TDMV_RBS_READY, &(wp)->flags)

#define WP_TDMV_ENABLE		0x01
#define WP_TDMV_DISABLE		0x02

#if defined(CONFIG_PRODUCT_WANPIPE_TDM_VOICE_DCHAN_ZAPTEL)
# define IS_CHAN_HARDHDLC(chan)	(((chan)->flags & ZT_FLAG_NOSTDTXRX) || ((chan)->flags & ZT_FLAG_HDLC))
#else
# define IS_CHAN_HARDHDLC(chan)	((chan)->flags & ZT_FLAG_HDLC)
#endif	

#if defined(CONFIG_PRODUCT_WANPIPE_TDM_VOICE_DCHAN)
# if defined(CONFIG_PRODUCT_WANPIPE_TDM_VOICE_DCHAN_ZAPTEL)
#  undef ZT_DCHAN_TX
# endif
#endif

#define wp_fax_tone_timeout_set(wr,chan) do { DEBUG_TEST("%s:%d: s%dc%d fax timeout set\n", \
											__FUNCTION__,__LINE__, \
											wp->spanno+1,chan+1); \
											wp->ec_fax_detect_timeout[chan]=SYSTEM_TICKS; } while(0);


#if defined(__FreeBSD__)
extern short *__zt_mulaw;
#endif

#if 0
static unsigned char wp_tdmv_ulaw[] = {
     0,    0,    0,    0,    0,    1,    1,    1,
     1,    2,    2,    2,    2,    3,    3,    3,
     3,    4,    4,    4,    4,    5,    5,    5,
     5,    6,    6,    6,    6,    7,    7,    7,
     7,    8,    8,    8,    8,    9,    9,    9,
     9,   10,   10,   10,   10,   11,   11,   11,
    11,   12,   12,   12,   12,   13,   13,   13,
    13,   14,   14,   14,   14,   15,   15,   15,
    15,   16,   16,   17,   17,   18,   18,   19,
    19,   20,   20,   21,   21,   22,   22,   23,
    23,   24,   24,   25,   25,   26,   26,   27,
    27,   28,   28,   29,   29,   30,   30,   31,
    31,   32,   33,   34,   35,   36,   37,   38,
    39,   40,   41,   42,   43,   44,   45,   46,
    47,   49,   51,   53,   55,   57,   59,   61,
    63,   66,   70,   74,   78,   84,   92,  104,
   254,  231,  219,  211,  205,  201,  197,  193,
   190,  188,  186,  184,  182,  180,  178,  176,
   175,  174,  173,  172,  171,  170,  169,  168,
   167,  166,  165,  164,  163,  162,  161,  160,
   159,  159,  158,  158,  157,  157,  156,  156,
   155,  155,  154,  154,  153,  153,  152,  152,
   151,  151,  150,  150,  149,  149,  148,  148,
   147,  147,  146,  146,  145,  145,  144,  144,
   143,  143,  143,  143,  142,  142,  142,  142,
   141,  141,  141,  141,  140,  140,  140,  140,
   139,  139,  139,  139,  138,  138,  138,  138,
   137,  137,  137,  137,  136,  136,  136,  136,
   135,  135,  135,  135,  134,  134,  134,  134,
   133,  133,  133,  133,  132,  132,  132,  132,
   131,  131,  131,  131,  130,  130,  130,  130,
   129,  129,  129,  129,  128,  128,  128,  128,
};
#endif

/*
 ******************************************************************************
			STRUCTURES AND TYPEDEFS
 ******************************************************************************
*/

typedef struct wp_tdmv_rbs
{
	int	chan;
	unsigned char data;
}wp_tdmv_rbs_t;

typedef struct wp_tdmv_pvt_area
{
	sdla_t*		card;
	char*		devname;
	wan_spinlock_t	lockirq;
	wan_spinlock_t	tx_rx_lockirq;
	int		ise1;
	int		num;
	int		spanno;
	int		flags;
	int		lbo;
	int 		lcode;
	int 		frame;
	int		usecount;
	int		sync;
	int		blinktimer;
	int		alarmtimer;
	int		loopupcnt;
	int		loopdowncnt;
#ifdef FANCY_ALARM
	int		alarmpos;
#endif
	/* T1 signalling */
	struct zt_span	span;					/* Span */
#ifdef DAHDI_ISSUES
#ifdef DAHDI_22
	struct dahdi_echocan_state ec[31];		/* echocan state for each channel */
#endif
	struct zt_chan	*chans_ptrs[31];			/* Channel ptrs */

#ifdef DAHDI_26
	struct dahdi_device *ddev;
	struct device dev;
#endif
	
#endif
	struct zt_chan	chans[31];				/* Channels */
	unsigned char	ec_chunk1[31][ZT_CHUNKSIZE];
	unsigned char	ec_chunk2[31][ZT_CHUNKSIZE];
	unsigned long	config_tsmap;
	unsigned long	timeslot_map;
	int		max_timeslots;		/* T1: 24, E1: 31 */
	int		timeslots;		/* configured timeslots */
	unsigned long	sig_timeslot_map;
	int		sig_timeslots;
	wan_skb_queue_t rbs_tx_q;
#if 0
	unsigned long	rbs_tx_status;
	unsigned long	rbs_tx1_status;
	unsigned char	rbs_tx[31];
	unsigned char	rbs_tx1[31];
#endif
	unsigned char	rbs_rx[31];
	unsigned long	rbs_rx_pending;
	u32		intcount;
	unsigned long   rbs_sanity_timeout;
	unsigned long	rbs_timeout;
	unsigned int	brt_ctrl;
	unsigned char	hwec;
	unsigned char	echo_off;
	unsigned long	echo_off_map;
	unsigned int	max_rxtx_len;

	unsigned int	channelized;	/* WAN_TRUE or WAN_FALSE */
	unsigned char	*tx_unchan;	/* tx data pointer for unchannelized mode */

	/* AHDLC: Hardware HDLC arguments */
	unsigned int	dchan_map;
	netdevice_t	*dchan_dev;

	u_int8_t	tonesupport;
	unsigned int	toneactive;
	unsigned int	tonemask;
	unsigned int	tonemutemask;
	
	unsigned long	ec_fax_detect_timeout[31+1];
	unsigned int	ec_off_on_fax;

} wp_tdmv_softc_t;



/*
*******************************************************************************
**			   GLOBAL VARIABLES
*******************************************************************************
*/
static int		wp_card_no = 0;
WAN_LIST_HEAD(, wan_tdmv_) wan_tdmv_head = 
			WAN_LIST_HEAD_INITIALIZER(&wan_tdmv_head);
/*
*******************************************************************************
**			  FUNCTION PROTOTYPES
*******************************************************************************
*/

static int wp_tdmv_check_mtu(void* pcard, unsigned long timeslot_map, int *mtu);
static int wp_tdmv_create(void* pcard, wan_tdmv_conf_t*);
static int wp_tdmv_remove(void* pcard);
static int wp_tdmv_reg(void* pcard, wan_tdmv_if_conf_t*, unsigned int, unsigned char,netdevice_t*);
static int wp_tdmv_unreg(void* pcard, unsigned long ts_map);
static int wp_tdmv_software_init(wan_tdmv_t *wan_tdmv);
static int wp_tdmv_shutdown(struct zt_span *span);
static int wp_tdmv_maint(struct zt_span *span, int cmd);

#ifdef DAHDI_25
static int wp_tdmv_startup(struct file *file, struct zt_span *span);
static int wp_tdmv_chanconfig(struct file *file, struct zt_chan *chan, int sigtype);
static int wp_tdmv_spanconfig(struct file *file, struct zt_span *span, struct zt_lineconfig *lc);
#else
static int wp_tdmv_startup(struct zt_span *span);
static int wp_tdmv_chanconfig(struct zt_chan *chan, int sigtype);
static int wp_tdmv_spanconfig(struct zt_span *span, struct zt_lineconfig *lc);
#endif
static int wp_tdmv_open(struct zt_chan *chan);
static int wp_tdmv_close(struct zt_chan *chan);
static void wp_tdmv_release(wp_tdmv_softc_t *wp);

#if defined(__FreeBSD__) || defined(__OpenBSD__)
static int wp_tdmv_ioctl(struct zt_chan*, unsigned int, caddr_t);
#else
static int wp_tdmv_ioctl(struct zt_chan*, unsigned int, unsigned long);
#endif

#ifdef DAHDI_22
static int wp_tdmv_hwec_create(struct dahdi_chan *chan, 
							   struct dahdi_echocanparams *ecp,
							   struct dahdi_echocanparam *p, 
							   struct dahdi_echocan_state **ec);
static void wp_tdmv_hwec_free(struct dahdi_chan *chan, 
							   struct dahdi_echocan_state *ec);
#else
static int wp_tdmv_hwec(struct zt_chan *chan, int enable);
#endif

#ifdef DAHDI_25
static const char *wp_tdmv_hwec_name(const struct zt_chan *chan);
#endif


#if defined(CONFIG_PRODUCT_WANPIPE_TDM_VOICE_DCHAN) && defined(CONFIG_PRODUCT_WANPIPE_TDM_VOICE_DCHAN_ZAPTEL)
static void wp_tdmv_tx_hdlc_hard(struct zt_chan *chan);
#endif
static int wp_tdmv_state(void* pcard, int state);
static int wp_tdmv_running(void* pcard);

/* RBS functions */
static int wp_tdmv_sigctrl(sdla_t* card, wp_tdmv_softc_t *wp, int channel, int status);
static int wp_tdmv_rbsbits(struct zt_chan *chan, int bits);
static int wp_tdmv_tx_rbsbits(wp_tdmv_softc_t *wp);
static int wp_tdmv_is_rbsbits(wan_tdmv_t *wan_tdmv);
static int wp_tdmv_rbsbits_poll(wan_tdmv_t *wan_tdmv, void *card1);
static void wp_tdmv_report_rbsbits(void* pcard, int channel, unsigned char status);

static void wp_tdmv_report_alarms(void* pcard, uint32_t te_alarm);

/* Rx/Tx functions */
static int wp_tdmv_rx_tx(void* pcard, netskb_t* skb);
static int wp_tdmv_rx_tx_span(void *pcard);
static int wp_tdmv_span_buf_rotate(void *pcard, u32, unsigned long, int);
static int wp_tdmv_ec_span(void *pcard);
static int wp_tdmv_rx_chan(wan_tdmv_t*, int, unsigned char*, unsigned char*); 
#if defined(CONFIG_PRODUCT_WANPIPE_TDM_VOICE_DCHAN) && defined(ZT_DCHAN_TX)
static int wp_tdmv_tx_dchan(struct zt_chan *chan, int len);
#endif
#if defined(CONFIG_PRODUCT_WANPIPE_TDM_VOICE_DCHAN)
static int wp_tdmv_rx_dchan(wan_tdmv_t*, int, unsigned char*, unsigned int); 
#endif
static int wp_tdmv_rxcopy(wan_tdmv_t *wan_tdmv, unsigned char* rxbuf, int max_len);
static int wp_tdmv_rxprep(wan_tdmv_t* wan_tdmv, unsigned char*, int);
static int wp_tdmv_txcopy(wan_tdmv_t* wan_tdmv, unsigned char*, int);
static int wp_tdmv_txprep(wan_tdmv_t* wan_tdmv, unsigned char*, int);

static inline void start_alarm(wp_tdmv_softc_t* wp);
static inline void stop_alarm(wp_tdmv_softc_t* wp);

static int wp_tdmv_init(void* pcard, wanif_conf_t *conf);

static void wp_tdmv_callback_tone (void*, wan_event_t*);


#if defined(DAHDI_24) || defined(DAHDI_25)

static const struct dahdi_span_ops wp_tdm_span_ops = {
	.owner = THIS_MODULE,
	.spanconfig = wp_tdmv_spanconfig,
	.chanconfig = wp_tdmv_chanconfig,
	.startup = wp_tdmv_startup,
	.shutdown = wp_tdmv_shutdown,
	.rbsbits = wp_tdmv_rbsbits,
	.maint = wp_tdmv_maint,
	.open = wp_tdmv_open,
	.close  = wp_tdmv_close,
	.ioctl = wp_tdmv_ioctl,
#if defined(CONFIG_PRODUCT_WANPIPE_TDM_VOICE_DCHAN) && defined(CONFIG_PRODUCT_WANPIPE_TDM_VOICE_DCHAN_ZAPTEL)   
	.hdlc_hard_xmit = wp_tdmv_tx_hdlc_hard,
#endif
#if 0
	.dacs = ,
#endif
	.echocan_create = wp_tdmv_hwec_create,
#ifdef DAHDI_25
	.echocan_name = wp_tdmv_hwec_name,
#endif

};
       
#endif   

#ifdef DAHDI_22
/*
*******************************************************************************
**			   DAHDI HWEC STRUCTURES
*******************************************************************************
*/
static const struct dahdi_echocan_features wp_tdmv_ec_features = {
	.NLP_automatic = 1,
	.CED_tx_detect = 1,
	.CED_rx_detect = 1,
};

static const struct dahdi_echocan_ops wp_tdmv_ec_ops = {
#ifndef DAHDI_25
	.name = "WANPIPE_HWEC",
#endif
	.echocan_free = wp_tdmv_hwec_free,
};
#endif

/******************************************************************************
** wp_tdmv_te1_init() - 
**
**	OK
*/
int wp_tdmv_te1_init(wan_tdmv_iface_t *iface)
{
	WAN_ASSERT(iface == NULL);

	iface->check_mtu	= wp_tdmv_check_mtu;
	iface->create		= wp_tdmv_create;
	iface->remove		= wp_tdmv_remove;
	iface->reg		= wp_tdmv_reg;
	iface->unreg		= wp_tdmv_unreg;
	iface->software_init	= wp_tdmv_software_init;
	iface->state		= wp_tdmv_state;
	iface->running		= wp_tdmv_running;
	iface->rx_tx		= wp_tdmv_rx_tx;
	iface->rx_chan		= wp_tdmv_rx_chan;
#if defined(CONFIG_PRODUCT_WANPIPE_TDM_VOICE_DCHAN)
	iface->rx_dchan		= wp_tdmv_rx_dchan;
#endif
	iface->rx_tx_span	= wp_tdmv_rx_tx_span;
	iface->is_rbsbits	= wp_tdmv_is_rbsbits;	//????
	iface->rbsbits_poll	= wp_tdmv_rbsbits_poll;	//?????
	iface->init		= wp_tdmv_init;
	iface->buf_rotate	= wp_tdmv_span_buf_rotate;
	iface->ec_span		= wp_tdmv_ec_span;

	return 0;
}

static int wp_tdmv_check_mtu(void* pcard, unsigned long timeslot_map, int *mtu)
{
	sdla_t	*card = (sdla_t*)pcard;
	int	x, num_of_channels = 0, max_channels;

	max_channels = GET_TE_CHANNEL_RANGE(&card->fe);
	for (x = 0; x < max_channels; x++) {
		if (wan_test_bit(x,&timeslot_map)){
			num_of_channels++;
		}
	}
	*mtu = ZT_CHUNKSIZE * num_of_channels;
	return 0;
}

/******************************************************************************
** wp_tdmv_create() - 
**
**	OK
*/
static int wp_tdmv_create(void* pcard, wan_tdmv_conf_t *tdmv_conf)
{
	sdla_t		*card = (sdla_t*)pcard;
	wp_tdmv_softc_t	*wp = NULL;
	wan_tdmv_t	*tmp = NULL;

#ifdef DAHDI_ISSUES
	int err=0;
	int i;
#endif

	WAN_ASSERT(card == NULL);
	WAN_ASSERT(tdmv_conf->span_no == 0);
	memset(&card->wan_tdmv, 0x0, sizeof(wan_tdmv_t));
	
	/* We are forcing to register wanpipe devices at the same sequence
	 * that it defines in /etc/zaptel.conf */
   	WAN_LIST_FOREACH(tmp, &wan_tdmv_head, next){
		if (tmp->spanno == tdmv_conf->span_no){
			DEBUG_EVENT("%s: Registering device with an incorrect span number!\n",
					card->devname);
			DEBUG_EVENT("%s: Another wanpipe device already configured to span #%d!\n",
					card->devname, tdmv_conf->span_no);
			return -EINVAL;
		}
		if (!WAN_LIST_NEXT(tmp, next)){
			break;
		}
	}

	card->wan_tdmv.max_timeslots	= GET_TE_CHANNEL_RANGE(&card->fe);
	card->wan_tdmv.spanno		= tdmv_conf->span_no;

	wp = wan_kmalloc(sizeof(wp_tdmv_softc_t));
	if (wp == NULL){
		return -ENOMEM;
	}
	memset(wp, 0x0, sizeof(wp_tdmv_softc_t));
	card->wan_tdmv.sc	= wp;
	wp->spanno		= tdmv_conf->span_no-1;


#ifdef DAHDI_ISSUES
	err=wp_dahdi_create_device(card,wp);
	if (err) {
		wan_free(wp);
		return -ENOMEM;
	}

	WP_DAHDI_SET_STR_INFO(wp,manufacturer,"Sangoma Technologies");
	
	switch(card->adptr_type){
	case A101_ADPTR_1TE1:
		WP_DAHDI_SET_STR_INFO(wp,devicetype, "A101");
		break;
	case A101_ADPTR_2TE1:
		WP_DAHDI_SET_STR_INFO(wp,devicetype, "A102");
		break;
	case A200_ADPTR_ANALOG:
		WP_DAHDI_SET_STR_INFO(wp,devicetype, "A200");
		break;
	case A400_ADPTR_ANALOG:
		WP_DAHDI_SET_STR_INFO(wp,devicetype, "A400");
		break;
	case A104_ADPTR_4TE1:
		WP_DAHDI_SET_STR_INFO(wp,devicetype, "A104");
		break;
	case A108_ADPTR_8TE1:
		WP_DAHDI_SET_STR_INFO(wp,devicetype, "A108");
		break;	
	case A116_ADPTR_16TE1:
		WP_DAHDI_SET_STR_INFO(wp,devicetype, "A116");
		break;	
	case AFT_ADPTR_B601:
		WP_DAHDI_SET_STR_INFO(wp,devicetype, "B601");
		break;
	}

	WP_DAHDI_SET_STR_INFO(wp,location,"SLOT=%d, BUS=%d", card->wandev.S514_slot_no, card->wandev.S514_bus_no);
#endif

#ifndef DAHDI_26
	wp->span.irq 	= card->wandev.irq;
#endif
	wp->num			= wp_card_no++;
	wp->card		= card;
	wp->devname		= card->devname;
	wp->lcode		= WAN_FE_LCODE(&card->fe);
	wp->frame		= WAN_FE_FRAME(&card->fe);
	wp->lbo			= WAN_TE1_LBO(&card->fe);
	wp->ise1		= IS_E1_FEMEDIA(&card->fe) ? 1 : 0;
	wp->max_timeslots	= IS_E1_FEMEDIA(&card->fe) ? 31: 24;
	wp->max_rxtx_len		= 0;
	wan_spin_lock_irq_init(&wp->lockirq, "wan_te1tdmv_lock");
	wan_spin_lock_irq_init(&wp->tx_rx_lockirq, "wan_te1tdmv_txrx_lock");
	wan_skb_queue_init(&wp->rbs_tx_q);
#ifdef DAHDI_ISSUES
	for (i = 0; i < sizeof(wp->chans)/sizeof(wp->chans[0]); i++) {
		wp->chans_ptrs[i] = &wp->chans[i];
	}
#endif
	/* AHDLC */
	if (tdmv_conf->dchan){
		/* PRI signalling is selected with hw HDLC (dchan is not 0) */
		wp->dchan_map = tdmv_conf->dchan;
		if (wp->ise1) {
			wan_clear_bit(0,&wp->dchan_map);
		}
	}

	if (tmp){
		WAN_LIST_INSERT_AFTER(tmp, &card->wan_tdmv, next);
	}else{
		WAN_LIST_INSERT_HEAD(&wan_tdmv_head, &card->wan_tdmv, next);
	}
	
	/* Bind alarms only when wp has been initialized */
	card->wandev.te_report_rbsbits  = wp_tdmv_report_rbsbits;
	card->wandev.te_report_alarms	= wp_tdmv_report_alarms;

	return 0;
}

/******************************************************************************
** wp_tdmv_reg() - 
**
** Returns: 	0-31	- Return TDM Voice channel number.
**		-EINVAL - otherwise
**	OK
*/
static int wp_tdmv_reg(	void			*pcard, 
			wan_tdmv_if_conf_t	*tdmv_conf,
			unsigned int		active_ch,
			u8			ec_enable,
			netdevice_t 		*dev)
{
	sdla_t		*card = (sdla_t*)pcard;
	wan_tdmv_t	*wan_tdmv = &card->wan_tdmv;
	wp_tdmv_softc_t	*wp = NULL;
	int		i, channo = 0, cnt = 0;
	
	WAN_ASSERT(wan_tdmv->sc == NULL);
	wp = wan_tdmv->sc;
		
	if (wan_test_bit(WP_TDMV_REGISTER, &wp->flags)){
		DEBUG_ERROR("%s: Error: Master device has already been configured!\n",
				card->devname);
		return -EINVAL;
	}
		
	/*
	 * T1: 1-24
	 * E1: 0-31 */
	for(i = 0; i<wp->max_timeslots; i++){
		if (wan_test_bit(i, &active_ch)){
			wan_set_bit(i, &wp->timeslot_map);
			if (tdmv_conf->tdmv_echo_off){
				wan_set_bit(i, &wp->echo_off_map);
			}
			channo = i;
			cnt++;
		}
	}	

	if (!cnt){
		DEBUG_ERROR("%s: Error: TDMV iface %s configured with 0 timeslots!\n",
				card->devname, wan_netif_name(dev));
		return -EINVAL;
	}

	if (cnt > 1){
		/* Unchannelized implementation */
		DEBUG_EVENT("%s:    TDMV Mode     :Unchannelized!\n",
				wp->devname);
		channo = 0;
#if 0
		if (is_last != WAN_TRUE){
			DEBUG_ERROR("%s: Error: Unchannelized interface must be the Master If (slots=%i)!\n",
				wp->devname,cnt);
			return -EINVAL;	
		}
#endif
		if (tdmv_conf->tdmv_echo_off){
			DEBUG_EVENT("%s:    TDMV Echo Ctrl:Off\n",
					wp->devname);
		}
		wp->channelized = WAN_FALSE;

		if (wp->dchan_map){
			if (dev == NULL){
				DEBUG_ERROR("%s: ERROR: Device pointer is NULL for D-chan!\n",
							wp->devname);
				return -EINVAL;
			}
			wp->dchan_dev = dev;
		}
	}else{
			
		/* Channelized implementation */
		if (wan_test_bit(channo, &wp->dchan_map)){
			if (dev == NULL){
				DEBUG_ERROR("%s: ERROR: Device pointer is NULL for D-chan!\n",
							wp->devname);
				return -EINVAL;
			}
			wp->dchan_dev = dev;
		}
		
		if (tdmv_conf->tdmv_echo_off){
			DEBUG_EVENT("%s:    TDMV Echo Ctrl:Off\n",
					wp->devname);
		}
		memset(wp->chans[channo].sreadchunk, WAN_TDMV_IDLE_FLAG, ZT_CHUNKSIZE);
		memset(wp->chans[channo].swritechunk, WAN_TDMV_IDLE_FLAG, ZT_CHUNKSIZE);
		wp->chans[channo].readchunk = wp->chans[channo].sreadchunk;
		wp->chans[channo].writechunk = wp->chans[channo].swritechunk;
		wp->channelized = WAN_TRUE;
	}
	wp->hwec = ec_enable;
	wp_tdmv_check_mtu(card, active_ch, &wp->max_rxtx_len);
	return channo;
}


/******************************************************************************
** wp_tdmv_del() - 
**
**	OK
*/
static int wp_tdmv_unreg(void* pcard, unsigned long ts_map)
{
	sdla_t		*card = (sdla_t*)pcard;
	wan_tdmv_t	*wan_tdmv = &card->wan_tdmv;
	wp_tdmv_softc_t	*wp = NULL;
	int		channo = 0;

	WAN_ASSERT(wan_tdmv->sc == NULL);
	wp = wan_tdmv->sc;


	for(channo = 0; channo < wp->max_timeslots; channo++){
		if (wan_test_bit(channo, &wp->timeslot_map)){
			wan_clear_bit(channo, &wp->timeslot_map);
			wan_clear_bit(channo, &wp->echo_off_map);
			memset(wp->chans[channo].sreadchunk, 
					WAN_TDMV_IDLE_FLAG, 
					ZT_CHUNKSIZE);
			memset(wp->chans[channo].swritechunk, 
					WAN_TDMV_IDLE_FLAG, 
					ZT_CHUNKSIZE);
			wp->chans[channo].readchunk = 
					wp->chans[channo].sreadchunk;
			wp->chans[channo].writechunk = 
					wp->chans[channo].swritechunk;
		}
	}
	return 0;
}


/******************************************************************************
** wp_tdmv_running() - 
**
**	OK
*/
static int wp_tdmv_running(void* pcard)
{
	sdla_t		*card = (sdla_t*)pcard;
	wan_tdmv_t	*wan_tdmv = &card->wan_tdmv;
	wp_tdmv_softc_t	*wp = NULL;

	wp = wan_tdmv->sc;
	if (wp && wp->usecount){
		DEBUG_EVENT("%s: WARNING: Wanpipe is still used by Asterisk (cnt=%i)!\n",
				card->devname,wp->usecount);
		return -EINVAL;
	}
	return 0;
}

/******************************************************************************
** wp_tdmv_remove() - 
**
**	OK
*/
static int wp_tdmv_remove(void* pcard)
{
	sdla_t		*card = (sdla_t*)pcard;
	wan_tdmv_t	*wan_tdmv = &card->wan_tdmv;
	wp_tdmv_softc_t	*wp = NULL;
	wan_smp_flag_t  flags;

	if (!card->wan_tdmv.sc){
		return 0;
	}
	
	wp = wan_tdmv->sc;
	DEBUG_TDMV("Removing TDM VOICE interfaces\n");
	/* Release span, possibly delayed */
	if (wp && wp->timeslot_map){
		DEBUG_EVENT("%s: Some interfaces are not unregistered (%08lX)!\n",
				card->devname, wp->timeslot_map);
		return -EINVAL;
	}
	if (wp && wp->usecount){
		DEBUG_ERROR("%s: ERROR: Wanpipe is still used by Asterisk (cnt=%i)!\n",
				card->devname,wp->usecount);
		return -EINVAL;
	}

	wan_spin_lock_irq(&wp->lockirq, &flags);
	card->wandev.te_report_rbsbits = NULL;
	card->wandev.te_report_alarms = NULL;	
	card->wandev.event_callback.tone = NULL;
	wan_spin_unlock_irq(&wp->lockirq, &flags);
	
	if (wp){
		wan_clear_bit(WP_TDMV_RUNNING, &wp->flags);
		wan_clear_bit(WP_TDMV_UP, &wp->flags);
		wan_tdmv->sc = NULL;
		WAN_LIST_REMOVE(wan_tdmv, next);
		wp_tdmv_sigctrl(card, wp, 0, WP_TDMV_DISABLE);
		wp_tdmv_release(wp);
	}else{
		wan_tdmv->sc = NULL;
	}
	return 0;
}

static int wp_tdmv_state(void* pcard, int state)
{
	sdla_t*	card = (sdla_t*)pcard;
	wan_tdmv_t*	wan_tdmv = &card->wan_tdmv;
	wp_tdmv_softc_t*	wp = NULL;

	WAN_ASSERT(wan_tdmv->sc == NULL);
	wp = (wp_tdmv_softc_t*)wan_tdmv->sc;
	
	switch(state){
	case WAN_CONNECTED:
		wp->rbs_rx_pending = wp->sig_timeslot_map;
		wan_set_bit(WP_TDMV_UP, &wp->flags);
		memset(wp->rbs_rx,0,sizeof(wp->rbs_rx));
		wan_clear_bit(WP_TDMV_RBS_BUSY, &wp->flags);
		wan_skb_queue_purge(&wp->rbs_tx_q);
		break;

	case WAN_DISCONNECTED:
		wan_clear_bit(WP_TDMV_UP, &wp->flags);
		wp->rbs_rx_pending = wp->sig_timeslot_map;
		memset(wp->rbs_rx,0,sizeof(wp->rbs_rx));
		wan_clear_bit(WP_TDMV_RBS_BUSY, &wp->flags);
		wan_skb_queue_purge(&wp->rbs_tx_q);
		break;
	}
	return 0;
}

static int wp_tdmv_sigctrl(sdla_t* card, wp_tdmv_softc_t *wp, int channel, int status)
{

	WAN_ASSERT(wp == NULL);
	WAN_ASSERT(card == NULL);

	if (status == WP_TDMV_ENABLE && IS_TDMV_RBS_READY(wp)){
		return 0;
	}else if (status == WP_TDMV_DISABLE && !IS_TDMV_RBS_READY(wp)){
		return 0;
	}
	
	DEBUG_EVENT("%s: %s signalling mode for all channels!\n",
			wp->devname,
		       	(status==WP_TDMV_ENABLE)?"Enable":"Disable");
  		
	if (card->wandev.fe_iface.set_fe_sigctrl){
		if (wan_test_bit(WP_TDMV_SIG_POLL, &wp->flags)){
			card->wandev.fe_iface.set_fe_sigctrl(
					&card->fe, 
					WAN_TE_SIG_POLL,
					ENABLE_ALL_CHANNELS,
					(status == WP_TDMV_ENABLE)?WAN_ENABLE:WAN_DISABLE);
		}else{
			card->wandev.fe_iface.set_fe_sigctrl(
					&card->fe,
					WAN_TE_SIG_INTR,
					ENABLE_ALL_CHANNELS,
					(status == WP_TDMV_ENABLE)?WAN_ENABLE:WAN_DISABLE);
		}
	}
	switch(status){
	case WP_TDMV_ENABLE:
   		wan_set_bit(WP_TDMV_SIG_ENABLE, &wp->flags);
   		wan_set_bit(WP_TDMV_RBS_READY, &wp->flags);
		break;
	case WP_TDMV_DISABLE:
   		wan_clear_bit(WP_TDMV_RBS_READY, &wp->flags);
   		wan_clear_bit(WP_TDMV_SIG_ENABLE, &wp->flags);
		break;
	default:
		DEBUG_EVENT("%s: Unknown signalling mode (%d)\n", 
				wp->devname, status);
		return -EINVAL;
	}
	return 0;
}

/******************************************************************************
** wp_tdmv_report_rbsbits() - 	Report A,B bit status changes to TDM Voice
**				requests. 
**
**	DONE
*/
static void wp_tdmv_report_rbsbits(void* pcard, int channel, unsigned char status)
{
	sdla_t*			card = (sdla_t*)pcard;
	wan_tdmv_t*		wan_tdmv = &card->wan_tdmv;
	wp_tdmv_softc_t*	wp = NULL;
	int 			rxs = 0, i = 0;

	WAN_ASSERT1(wan_tdmv->sc == NULL);
	wp = (wp_tdmv_softc_t*)wan_tdmv->sc;

	if (!wan_test_bit(channel-1, &wp->timeslot_map)){
		return;
	}

	if (status & WAN_RBS_SIG_A) rxs |= ZT_ABIT;
	if (status & WAN_RBS_SIG_B) rxs |= ZT_BBIT;
	if (status & WAN_RBS_SIG_C) rxs |= ZT_CBIT;
	if (status & WAN_RBS_SIG_D) rxs |= ZT_DBIT;
#if 0
	if (wp->ise1){
		FIXME_MSG("wp_tdmv_report_rbsbits");
		/* Read 5 registers at a time, loading 10 channels at a time */
		for (i = (x *5); i < (x * 5) + 5; i++) {
			/* FIXME a = __t1_get_reg(wp, 0x31 + i); */
			/* Get high channel in low bits */
			rxs = (a & 0xf);
			if (!(wp->chans[i+15].sig & ZT_SIG_CLEAR)) {
				if (wp->chans[i+15].rxsig != rxs)
					zt_rbsbits(&wp->chans[i+15], rxs);
			}
			rxs = (a >> 4) & 0xf;
			if (!(wp->chans[i].sig & ZT_SIG_CLEAR)) {
				if (wp->chans[i].rxsig != rxs)
					zt_rbsbits(&wp->chans[i], rxs);
			}
		}
	}
#endif
	for(i=0; i < wp->span.channels;i++){
		if (wp->chans[i].chanpos == channel)
			break;
	}
	if (i == wp->span.channels){
		return;
	}
	if (!(wp->chans[i].sig & ZT_SIG_CLEAR) &&
	    (wp->chans[i].rxsig != rxs)){
		zt_rbsbits(&wp->chans[i], rxs);

		DEBUG_TDMV(
		"[TDMV] %s: %s:%02d(%d) RX RBS: A:%1d B:%1d C:%1d D:%1d\n",
				wp->devname, 
				(wp->ise1) ? "E1" : "T1",
				channel, wp->chans[i].channo,
				(rxs & ZT_ABIT) ? 1 : 0,
				(rxs & ZT_BBIT) ? 1 : 0,
				(rxs & ZT_CBIT) ? 1 : 0,
				(rxs & ZT_DBIT) ? 1 : 0);
	}
}

/******************************************************************************
** wp_tdmv_report_alarms() - 
**
**	DONE
*/
static void wp_tdmv_report_alarms(void* pcard, uint32_t te_alarm)
{
	sdla_t		*card = (sdla_t*)pcard;
	wan_tdmv_t	*wan_tdmv = &card->wan_tdmv;
	wp_tdmv_softc_t	*wp = NULL;
	unsigned int alarms = 0, prev_alarms;
	int		x,j;



	/* The sc pointer can be NULL, on shutdown. In this
	 * case don't generate error, just get out */
	wp = wan_tdmv->sc;
	if (!wp){
		return;
	}

	/* And consider only carrier alarms */
	wp->span.alarms &= (ZT_ALARM_RED | ZT_ALARM_BLUE | ZT_ALARM_NOTOPEN);
	prev_alarms = wp->span.alarms;

	if (wp->ise1){
		/* XXX Implement me XXX */
	}else{
		/* Detect loopup code if we're not sending one */
		if (!wp->span.mainttimer && (te_alarm & WAN_TE_BIT_LOOPUP_CODE)){
			/* Loop-up code detected */
			if ((wp->loopupcnt++ > 80)  && (wp->span.maintstat != ZT_MAINT_REMOTELOOP)){
				card->wandev.fe_iface.set_fe_lbmode(
						&wp->card->fe,
					       	WAN_TE1_DDLB_MODE,
					       	WAN_TE1_LB_DISABLE,
						ENABLE_ALL_CHANNELS);
				card->wandev.fe_iface.set_fe_lbmode(
						&wp->card->fe,
						WAN_TE1_LINELB_MODE,
					       	WAN_TE1_LB_ENABLE,
						ENABLE_ALL_CHANNELS);
				wp->span.maintstat = ZT_MAINT_REMOTELOOP;
			}
		}else{
			wp->loopupcnt = 0;
		}
		/* Same for loopdown code */
		if (!wp->span.mainttimer && (te_alarm & WAN_TE_BIT_LOOPDOWN_CODE)){
			/* Loop-down code detected */
			if ((wp->loopdowncnt++ > 80)  && (wp->span.maintstat == ZT_MAINT_REMOTELOOP)){
				card->wandev.fe_iface.set_fe_lbmode(
						&wp->card->fe,
						WAN_TE1_DDLB_MODE,
					       	WAN_TE1_LB_DISABLE,
						ENABLE_ALL_CHANNELS);
				card->wandev.fe_iface.set_fe_lbmode(
						&wp->card->fe,
						WAN_TE1_LINELB_MODE,
					       	WAN_TE1_LB_DISABLE,
						ENABLE_ALL_CHANNELS);
				wp->span.maintstat = ZT_MAINT_NONE;
			}
		}else{
			wp->loopdowncnt = 0;
		}
	}

	if (wp->span.lineconfig & ZT_CONFIG_NOTOPEN) {
		for (x=0,j=0;x < wp->span.channels;x++){
			if ((wp->chans[x].flags & ZT_FLAG_OPEN) ||
#if defined(DAHDI_ISSUES) && !defined(DAHDI_FLAG_NETDEV)
				(wp->chans[x].flags & dahdi_have_netdev(&wp->chans[x]))){
#else
			    (wp->chans[x].flags & ZT_FLAG_NETDEV)){
#endif
				j++;
			}
		}
		if (!j){
			alarms |= ZT_ALARM_NOTOPEN;
		}
	}

	/* Note: The alarm checking should match
	         Alarm checking in sdla_te1.c and
		 sdla_8te1.c 
	 */

	if (wp->ise1) {
		if (te_alarm & WAN_TE_BIT_ALARM_RED) 
			alarms |= ZT_ALARM_RED;
		if (te_alarm & WAN_TE_BIT_ALARM_LOF) 
			alarms |= ZT_ALARM_RED;
	} else {
		/* Check actual alarm status */
		if (te_alarm & WAN_TE_BIT_ALARM_RED) 
			alarms |= ZT_ALARM_RED;
		if (te_alarm & WAN_TE_BIT_ALARM_LOF) 
			alarms |= ZT_ALARM_RED;
	}
	/* Keep track of recovering */
	if ((!alarms) && wp->span.alarms)
		wp->alarmtimer = ZT_ALARMSETTLE_TIME;

#if 0
	/* If receiving alarms, go into Yellow alarm state */
	if (alarms && (!wp->span.alarms)) {
		DEBUG_TDMV("%s: Going into yellow alarm\n",
				wp->devname);
		if (card->wandev.fe_iface.set_fe_alarm){
			card->wandev.fe_iface.set_fe_alarm(&card->fe, WAN_TE_BIT_ALARM_YEL);
		}
	}
#endif

	if (wp->span.alarms != alarms) {
		/* FIXME: Alarm status changed!!! */
	 	DEBUG_TDMV("%s: Alarm status changed %X!\n",
			       	wp->devname,alarms);
	}
	/*
	** if (wp->alarmtimer)
	** alarms |= ZT_ALARM_RECOVER; */
	if (te_alarm & WAN_TE_BIT_ALARM_YEL || te_alarm & WAN_TE_BIT_ALARM_RAI)
		alarms |= ZT_ALARM_YELLOW;

	wp->span.alarms = alarms;
	zt_alarm_notify(&wp->span);
	return;
}

/******************************************************************************
** wp_tdmv_txcopy() - 
**
**	OK
*/
static int 
wp_tdmv_txcopy(wan_tdmv_t *wan_tdmv, unsigned char* txbuf, int max_len)
{
	wp_tdmv_softc_t	*wp = wan_tdmv->sc;
	int		x = 0, y = 0, offset = 0;

	WAN_ASSERT(wp == NULL);
	for (y=0;y<ZT_CHUNKSIZE;y++) {
		for (x=0;x<wp->span.channels;x++){
			if (!wan_test_bit(x,&wp->timeslot_map)){
				continue;
			}
			txbuf[offset++] =  wp->chans[x].writechunk[y];
		}
	}
	return (offset+1);
}

/******************************************************************************
** wp_tdmv_txprep() - 
**
**	OK
*/
static int 
wp_tdmv_txprep(wan_tdmv_t *wan_tdmv, unsigned char* txbuf, int max_len)
{
	wp_tdmv_softc_t	*wp = wan_tdmv->sc;

	WAN_ASSERT2(wp == NULL, 0);
	zt_transmit(&wp->span);
	return wp_tdmv_txcopy(wan_tdmv, txbuf, max_len);
}

/******************************************************************************
** wp_tdmv_rxcopy() - 
**
**	OK
*/
static int 
wp_tdmv_rxcopy(wan_tdmv_t *wan_tdmv, unsigned char* rxbuf, int max_len)
{
	wp_tdmv_softc_t	*wp = wan_tdmv->sc;
	int		x, y, offset = 0;
	int		channels = wp->span.channels;
	unsigned char	value;

	WAN_ASSERT(wp == NULL);
	for (y=0;y<ZT_CHUNKSIZE;y++){
		for (x=0;x<channels;x++){
			if (wan_test_bit(x,&wp->timeslot_map)){
				value = rxbuf[offset++];
			}else{
				value = WAN_TDMV_IDLE_FLAG;	/* 0xFE; */
			}
			wp->chans[x].readchunk[y] = value;
		}
	}
	return offset;
}

/******************************************************************************
** wp_tdmv_rxprep() - 
**
**	OK
*/
static int 
wp_tdmv_rxprep(wan_tdmv_t *wan_tdmv, unsigned char* rxbuf, int max_len)
{
	wp_tdmv_softc_t	*wp = wan_tdmv->sc;
	int		x, offset = 0, channels = wp->span.channels;
	
	WAN_ASSERT2(wp == NULL, 0);

	offset = wp_tdmv_rxcopy(wan_tdmv, rxbuf, max_len);

	if (!wp->echo_off_map){
		for (x = 0; x < channels; x++){
#ifdef CONFIG_PRODUCT_WANPIPE_TDM_VOICE_ECHOMASTER
			wan_tdmv_rxtx_pwr_t *pwr_rxtx = &wan_tdmv->chan_pwr[x];
			wp_tdmv_echo_check(wan_tdmv, &wp->chans[0], x);
			if(pwr_rxtx->current_state != ECHO_ABSENT){
#endif
				zt_ec_chunk(
					&wp->chans[x], 
					wp->chans[x].readchunk, 
					wp->chans[x].writechunk);
#if 0
				zt_ec_chunk(
					&wp->chans[x], 
					wp->chans[x].readchunk, 
					wp->ec_chunk1[x]);
				memcpy(
					wp->ec_chunk1[x],
					wp->chans[x].writechunk,
					ZT_CHUNKSIZE);
#endif

#ifdef CONFIG_PRODUCT_WANPIPE_TDM_VOICE_ECHOMASTER
			}/* if() */
#endif
		}/* for() */
	}/* if() */

	zt_receive(&wp->span);
	return (offset);
}


/******************************************************************************
** wp_tdmv_rx_tx() - 
**
**	OK
*/
static int wp_tdmv_rx_tx(void* pcard, netskb_t* skb)
{
	sdla_t		*card = (sdla_t*)pcard;
	wan_tdmv_t	*wan_tdmv = &card->wan_tdmv;
	wp_tdmv_softc_t	*wp = NULL;
	int 		len;

	WAN_ASSERT(wan_tdmv->sc == NULL);
	WAN_ASSERT(skb == NULL);
	wp = wan_tdmv->sc;

	if (wan_skb_len(skb) != wp->max_rxtx_len){
		if (WAN_NET_RATELIMIT()) {
		DEBUG_ERROR("%s: Internal Error[%s:%d]: Wrong buffer lenght %d (%d)!\n", 
					wp->devname, 
					__FUNCTION__,__LINE__,
					wan_skb_len(skb),
					wp->max_rxtx_len); 
		}
                return 0;
        }

	wp->intcount ++;

	len = wp_tdmv_rxprep(
			wan_tdmv,
			wan_skb_data(skb),
			wan_skb_len(skb)); 
	len = wp_tdmv_txprep(
			wan_tdmv,
			wan_skb_data(skb),
			wan_skb_len(skb)); 


	return wp->max_rxtx_len;
}


/*
**		STATIC FUNCTION DEFINITIONS
**
*/

/******************************************************************************
** wp_tdmv_software_init() - 
**
** OK
*/
static int wp_tdmv_software_init(wan_tdmv_t *wan_tdmv)
{
	sdla_t		*card = NULL;
	wp_tdmv_softc_t	*wp = wan_tdmv->sc;
	int		x = 0;

	WAN_ASSERT(wp == NULL);
	WAN_ASSERT(wp->card == NULL);
	card = wp->card;

	if (wan_test_bit(WP_TDMV_REGISTER, &wp->flags)){
		DEBUG_EVENT("%s: Wanpipe device is already registered to Zaptel span # %d!\n", 
					wp->devname, wp->span.spanno);
		return 0;
	}
#if 0
	if (wp->ise1 && wp->channelized == WAN_FALSE){
		/* Timeslot map for E1 includes ts0. TDM voice does never
		** use ts0, so i will shift map 1 bit right to get 
		** everything alignment as T1 */
		wp->timeslot_map = wp->timeslot_map >> 1;
		wp->echo_off_map = wp->echo_off_map >> 1;
	}
#endif
	if (wp->ise1){
		sprintf(wp->span.name, "WPE1/%d", wp->num);
	}else{
		sprintf(wp->span.name, "WPT1/%d", wp->num);
	}
	sprintf(wp->span.desc, "%s card %d", wp->devname, wp->num);

#if defined(DAHDI_24) || defined(DAHDI_25) 
    wp->span.ops = &wp_tdm_span_ops;
#else

	wp->span.pvt = wp;

	wp->span.spanconfig = wp_tdmv_spanconfig;
	wp->span.chanconfig = wp_tdmv_chanconfig;
	wp->span.startup = wp_tdmv_startup;
	wp->span.shutdown = wp_tdmv_shutdown;
	wp->span.rbsbits = wp_tdmv_rbsbits;
	wp->span.maint = wp_tdmv_maint;
	wp->span.open = wp_tdmv_open;
	wp->span.close = wp_tdmv_close;
	wp->span.ioctl = wp_tdmv_ioctl;
	
	/* Set this pointer only if card has hw echo canceller module */
	if (wp->hwec == WANOPT_YES && card->wandev.ec_dev){
		/* Initialize it only if HWEC option is enabled */
#ifdef DAHDI_22
		wp->span.echocan_create = wp_tdmv_hwec_create;
#else
		wp->span.echocan = wp_tdmv_hwec;
#endif
	}

#if defined(DAHDI_23)
    wp->span.owner = THIS_MODULE;
#endif

#if defined(CONFIG_PRODUCT_WANPIPE_TDM_VOICE_DCHAN) && defined(CONFIG_PRODUCT_WANPIPE_TDM_VOICE_DCHAN_ZAPTEL)
	if (wp->dchan_map){
		DEBUG_EVENT("%s: Enable Dahdi/Zaptel HW DCHAN interface\n",
				wp->devname);
		wp->span.hdlc_hard_xmit = wp_tdmv_tx_hdlc_hard;
	}
#endif

#endif /* NON DAHDI_24 */

#ifdef DAHDI_ISSUES
	wp->span.chans = wp->chans_ptrs;
#else
	wp->span.chans = wp->chans;
#endif

	wp->span.flags = ZT_FLAG_RBS;
	wp->span.channels = wp->max_timeslots;


	if (wp->ise1){
		wp->span.deflaw = ZT_LAW_ALAW;
		card->fe.fe_cfg.tdmv_law = WAN_TDMV_ALAW;
		wp->span.linecompat = ZT_CONFIG_HDB3 | ZT_CONFIG_CCS | ZT_CONFIG_CRC4;
#ifdef DAHDI_ISSUES
		wp->span.spantype=SPANTYPE_DIGITAL_E1;
#endif
	}else{
		wp->span.deflaw = ZT_LAW_MULAW;
		card->fe.fe_cfg.tdmv_law = WAN_TDMV_MULAW;
		wp->span.linecompat = ZT_CONFIG_AMI | ZT_CONFIG_B8ZS | ZT_CONFIG_D4 | ZT_CONFIG_ESF;
#ifdef DAHDI_ISSUES
		wp->span.spantype=SPANTYPE_DIGITAL_T1;
#endif
	}
	
#if !defined(__FreeBSD__) && !defined(__OpenBSD__)
#ifndef DAHDI_25
	init_waitqueue_head(&wp->span.maintq);
#endif
#endif

	for (x=0;x<wp->span.channels;x++) {
		sprintf(wp->chans[x].name, "%s/%d", wp->span.name, x+1);
		if (wan_test_bit(x,&wp->timeslot_map)){
			DEBUG_TEST("%s: Configure channel %d for voice (%s)!\n", 
					wp->devname, 
					x + 1,
					wp->chans[x].name);
			wp->chans[x].sigcap = 
				ZT_SIG_EM | ZT_SIG_CLEAR | ZT_SIG_EM_E1 | 
				ZT_SIG_FXSLS | ZT_SIG_FXSGS |  
				ZT_SIG_FXSKS | ZT_SIG_FXOLS | ZT_SIG_DACS_RBS |
				ZT_SIG_FXOGS | ZT_SIG_FXOKS | ZT_SIG_CAS 
#if defined(CONFIG_PRODUCT_WANPIPE_TDM_VOICE_DCHAN) && defined(CONFIG_PRODUCT_WANPIPE_TDM_VOICE_DCHAN_ZAPTEL)
 				| ZT_SIG_HARDHDLC 
#endif
#if defined(ZT_SIG_MTP2)
				| ZT_SIG_MTP2
#endif
#if defined(ZT_SIG_SF)
				| ZT_SIG_SF
#endif
				;
		}else{
			wp->chans[x].sigcap = ZT_SIG_NONE;
		}
		wp->chans[x].pvt = wp;
		wp->chans[x].chanpos = x + 1;
		wp->chans[x].rxsig = 0x00;
	}

	if (wp_dahdi_register_device(wp)) {
		DEBUG_EVENT("%s: Unable to register span with DAHDI\n",
					wp->devname);
		return -EINVAL;
	}
	
	if (wp->span.spanno != wp->spanno +1){
		DEBUG_EVENT("\n");
		DEBUG_EVENT("WARNING: Span number %d is already used by another device!\n",
						wp->spanno + 1);
		DEBUG_EVENT("         Possible cause: Another TDM driver already loaded!\n");
		DEBUG_EVENT("         Solution:       Unload wanpipe and check currently\n");
#if defined(__LINUX__)
		DEBUG_EVENT("                         used spans in /proc/zaptel directory.\n");
#else
		DEBUG_EVENT("                         used spans.\n");
#endif
		DEBUG_EVENT("         Reconfiguring device %s to new span number # %d\n",
						wp->devname,wp->span.spanno);
		DEBUG_EVENT("\n");
		wp->spanno = wp->span.spanno-1;
	}else{
		DEBUG_EVENT("%s: Wanpipe device is registered to Zaptel span # %d!\n", 
					wp->devname, wp->span.spanno);
	}
	if (wp->channelized == WAN_FALSE && wp->dchan_map){
		/* Apr 15, 2005
		 * For unchannelized mode, if HW HDLC protocol is selected
		 * by using dchan configuration option, remove dchan timeslot
		 * from timeslot and echo map. */
		if (wp->ise1) {
               		wp->dchan_map=wp->dchan_map>>1;
			wan_clear_bit(0,&wp->dchan_map);
		}

		wp->timeslot_map &= ~wp->dchan_map;
		wp->echo_off_map &= ~wp->dchan_map;		
	}

	wp_tdmv_check_mtu(card, wp->timeslot_map, &wp->max_rxtx_len);
	wan_set_bit(WP_TDMV_REGISTER, &wp->flags);
	if (!wan_tdmv->sig_intr_enable){
		wan_set_bit(WP_TDMV_SIG_POLL, &wp->flags);
		/* These function is available if poll used for signalling detection */
		card->tdmv_iface.is_rbsbits	= wp_tdmv_is_rbsbits;
		card->tdmv_iface.rbsbits_poll	= wp_tdmv_rbsbits_poll;
	}
		
	wp->tonesupport = card->u.aft.tdmv_hw_tone;
	/* Initialize Callback event function pointers */	
	if (wp->tonesupport == WANOPT_YES){
		DEBUG_EVENT("%s: Enable HW Tone detection!\n", wp->devname);
		card->wandev.event_callback.tone = wp_tdmv_callback_tone;
	}
	
	return 0;
}


/******************************************************************************
** wp_tdmv_startup() - 
**
**	OK
*/

#ifdef DAHDI_25
static int wp_tdmv_startup(struct file *file, struct zt_span *span)
#else
static int wp_tdmv_startup(struct zt_span *span)
#endif
{
	wp_tdmv_softc_t*	wp = NULL;
	int			i;
	sdla_t		*card = NULL;

	WAN_ASSERT2(span == NULL, -ENODEV);
	wp		= WP_PRIV_FROM_SPAN(span, wp_tdmv_softc_t);
	WAN_ASSERT2(wp == NULL, -ENODEV);
	WAN_ASSERT2(wp->card == NULL, -ENODEV);
	card	= wp->card;
	
	/* initialize the start value for the entire chunk of last ec buffer */
	for(i = 0; i < span->channels; i++){
#ifdef DAHDI_ISSUES
		memset(wp->ec_chunk1[i],
			ZT_LIN2X(0,span->chans[i]),ZT_CHUNKSIZE);
		memset(wp->ec_chunk2[i],
			ZT_LIN2X(0,span->chans[i]),ZT_CHUNKSIZE);
#else
		memset(wp->ec_chunk1[i],
			ZT_LIN2X(0,&span->chans[i]),ZT_CHUNKSIZE);
		memset(wp->ec_chunk2[i],
			ZT_LIN2X(0,&span->chans[i]),ZT_CHUNKSIZE);
#endif

		/* Disable hwec on signalling channel */
		if (wp->hwec == WANOPT_YES && card->wandev.ec_dev && card->wandev.ec_enable) {
#ifdef DAHDI_ISSUES
			struct zt_chan *chan = span->chans[i];
#else
			struct zt_chan *chan = &span->chans[i];
#endif
			unsigned int hdlcsig=chan->sig&~(ZT_SIG_CLEAR); 
			if (IS_CHAN_HARDHDLC(chan) || hdlcsig & ZT_SIG_HDLCRAW || hdlcsig & ZT_SIG_HDLCFCS) {
				DEBUG_TEST("%s: Disabling EC Chan=%i, sigtype 0x%03X, HDLCRAW=0x%X  FCS=0x%X\n",
							card->devname, chan->chanpos, hdlcsig, ZT_SIG_HDLCRAW, ZT_SIG_HDLCFCS);
					card->wandev.ec_enable(card, 0, chan->chanpos);
			}
		}	
	}


	if (!(span->flags & ZT_FLAG_RUNNING)) {
		/* Only if we're not already going */
		span->flags |= ZT_FLAG_RUNNING;
	}
	wan_set_bit(WP_TDMV_RUNNING, &wp->flags);

	return 0;
}

/******************************************************************************
** wp_tdmv_shutdown() - 
**
**	OK
*/
static int wp_tdmv_shutdown(struct zt_span *span)
{
	wp_tdmv_softc_t*	wp = NULL;
	wan_smp_flag_t		flags;

	WAN_ASSERT2(span == NULL, -ENODEV);
	wp		= WP_PRIV_FROM_SPAN(span, wp_tdmv_softc_t);
	WAN_ASSERT2(wp == NULL, -ENODEV);

	wan_clear_bit(WP_TDMV_RUNNING, &wp->flags);
	wan_spin_lock_irq(&wp->lockirq, &flags);
	span->flags &= ~ZT_FLAG_RUNNING;
	wan_spin_unlock_irq(&wp->lockirq, &flags);
	return 0;
}

/******************************************************************************
** wp_tdmv_maint() - 
**
**	OK
*/
static int wp_tdmv_maint(struct zt_span *span, int cmd)
{
	wp_tdmv_softc_t	*wp;
	sdla_t		*card;
	int		res = 0;
	wan_smp_flag_t	flags,smp_flags;
	
	WAN_ASSERT2(span == NULL, -ENODEV);
	wp		= WP_PRIV_FROM_SPAN(span, wp_tdmv_softc_t);
	WAN_ASSERT2(wp == NULL, -ENODEV);
   	card = wp->card;   

	card->hw_iface.hw_lock(card->hw,&smp_flags);
	wan_spin_lock_irq(&wp->lockirq, &flags);
	switch(cmd) {
	case ZT_MAINT_NONE:
		DEBUG_EVENT("%s: Set to normal mode (no local/remote loop)\n",
				   wp->card->devname);	
		card->wandev.fe_iface.set_fe_lbmode(
				&wp->card->fe,
				WAN_TE1_DDLB_MODE,
				WAN_TE1_LB_DISABLE,
				ENABLE_ALL_CHANNELS);
		card->wandev.fe_iface.set_fe_lbmode(
				&wp->card->fe,
				WAN_TE1_LINELB_MODE,
				WAN_TE1_LB_DISABLE,
				ENABLE_ALL_CHANNELS);
		card->wandev.fe_iface.set_fe_lbmode(
				&wp->card->fe,
				WAN_TE1_PAYLB_MODE,
				WAN_TE1_LB_DISABLE,
				ENABLE_ALL_CHANNELS);
		break;
	case ZT_MAINT_LOCALLOOP:
		DEBUG_EVENT("%s: Set to local loopback mode (local/no remote loop)\n",
				   wp->card->devname);	
		card->wandev.fe_iface.set_fe_lbmode(
				&wp->card->fe,
				WAN_TE1_DDLB_MODE,
				WAN_TE1_LB_ENABLE,
				ENABLE_ALL_CHANNELS);
		card->wandev.fe_iface.set_fe_lbmode(
				&wp->card->fe,
				WAN_TE1_LINELB_MODE,
				WAN_TE1_LB_DISABLE,
				ENABLE_ALL_CHANNELS);
		break;
	case ZT_MAINT_REMOTELOOP:
		DEBUG_EVENT("%s: Set to remote loopback mode (no local/remote loop)\n",
				   wp->card->devname);	
		card->wandev.fe_iface.set_fe_lbmode(
				&wp->card->fe,
				WAN_TE1_LINELB_MODE,
				WAN_TE1_LB_ENABLE,
				ENABLE_ALL_CHANNELS);
		card->wandev.fe_iface.set_fe_lbmode(
				&wp->card->fe,
				WAN_TE1_DDLB_MODE,
				WAN_TE1_LB_DISABLE,
				ENABLE_ALL_CHANNELS);
		break;
#if DAHDI_ISSUES
	case DAHDI_RESET_COUNTERS:
		DEBUG_EVENT("%s: Reset Front End Counters\n",
				   wp->card->devname);	
		card->wandev.fe_iface.flush_pmon(&wp->card->fe);
		break;
#endif
#if 0
	case ZT_MAINT_LOOPUP:
		DEBUG_EVENT("%s: Send loopup code\n",
				   wp->card->devname);	
		card->wandev.fe_iface.set_fe_lbmode(
				&wp->card->fe,
				WAN_TE1_TX_LINELB_MODE,
				WAN_TE1_LB_ENABLE,
				ENABLE_ALL_CHANNELS);
		break;
	case ZT_MAINT_LOOPDOWN:
		DEBUG_EVENT("%s: Send loopdown code\n",
				   wp->card->devname);	
		card->wandev.fe_iface.set_fe_lbmode(
				&wp->card->fe,
				WAN_TE1_TX_LINELB_MODE,
				WAN_TE1_LB_DISABLE,
				ENABLE_ALL_CHANNELS);
		break;
#ifndef DAHDI_25
	case ZT_MAINT_LOOPSTOP:
		DEBUG_EVENT("%s: Stop sending loop code\n",
				   wp->card->devname);	
		/* FIXME __t1_set_reg(wp,0x30,0); */	/* stop sending loopup code */
		break;
#endif
#endif
	default:
		DEBUG_EVENT("%s: T1: Unknown maintenance mode (%d)\n", 
				wp->card->devname, cmd);
		res = -EINVAL;
	}
	wan_spin_unlock_irq(&wp->lockirq, &flags);
	card->hw_iface.hw_unlock(card->hw,&smp_flags);
	return res;
}

static void wp_tdmv_set_clear(wp_tdmv_softc_t* wp)
{
	WAN_ASSERT1(wp == NULL);

	/* FIXME: Add action for this function  */
}

/******************************************************************************
** sigstr() - 
**
**	OK	
*/
#if 0
static char *wp_tdmv_sigstr(int sig)
{
	switch (sig) {
		case ZT_SIG_FXSLS:
			return "FXSLS";
		case ZT_SIG_FXSKS:
			return "FXSKS";
		case ZT_SIG_FXSGS:
			return "FXSGS";
		case ZT_SIG_FXOLS:
			return "FXOLS";
		case ZT_SIG_FXOKS:
			return "FXOKS";
		case ZT_SIG_FXOGS:
			return "FXOGS";
		case ZT_SIG_EM:
			return "E&M";
		case ZT_SIG_CLEAR:
			return "Clear";
		case ZT_SIG_HDLCRAW:
			return "HDLCRAW";
		case ZT_SIG_HDLCFCS:
			return "HDLCFCS";
		case ZT_SIG_HDLCNET:
			return "HDLCNET";
		case ZT_SIG_SLAVE:
			return "Slave";
		case ZT_SIG_CAS:
			return "CAS";
		case ZT_SIG_DACS:
			return "DACS";
		case ZT_SIG_SF:
			return "SF (ToneOnly)";
		case ZT_SIG_NONE:
		default:
			return "Unconfigured";
	}

}
#endif

#ifdef DAHDI_25
static int wp_tdmv_chanconfig(struct file *file, struct zt_chan *chan, int sigtype)
#else
static int wp_tdmv_chanconfig(struct zt_chan *chan, int sigtype)
#endif
{
	sdla_t		*card;
	wp_tdmv_softc_t	*wp = NULL;
	
	WAN_ASSERT2(chan == NULL, -ENODEV);
	wp		= WP_PRIV_FROM_CHAN(chan, wp_tdmv_softc_t);
	
	WAN_ASSERT2(wp == NULL, -ENODEV);
	card	= (sdla_t*)wp->card;

	if (chan->span->flags & ZT_FLAG_RUNNING){
		wp_tdmv_set_clear(wp);
	}

	if (!wan_test_and_set_bit(chan->chanpos, &wp->config_tsmap)){
		wp->timeslots++;
	}
	if (!(sigtype & ZT_SIG_CLEAR)){
		/* Set Signalling channel map */
		if (!wan_test_and_set_bit(chan->chanpos-1, &wp->sig_timeslot_map)){
			wp->sig_timeslots ++;
			wan_set_bit(chan->chanpos-1, &wp->rbs_rx_pending);
		}
		wp_tdmv_sigctrl(card, wp, chan->chanpos, WP_TDMV_ENABLE);
	}

	return 0;
}

/******************************************************************************
** wp_tdmv_spanconfig() - 
**
**	OK
*/
#ifdef DAHDI_25
static int wp_tdmv_spanconfig(struct file *file, struct zt_span *span, struct zt_lineconfig *lc)
#else
static int wp_tdmv_spanconfig(struct zt_span *span, struct zt_lineconfig *lc)
#endif
{
	wp_tdmv_softc_t	*wp = NULL;
	sdla_t		*card = NULL;
	int		err = 0;
	wan_smp_flag_t smp_flags;
	int 	need_reconfig=0;

	WAN_ASSERT2(span == NULL, -ENODEV);

	wp		= WP_PRIV_FROM_SPAN(span, wp_tdmv_softc_t);
	WAN_ASSERT2(wp == NULL, -ENODEV);

	card = (sdla_t*)wp->card;
	DEBUG_EVENT("%s: DAHDI Configuring device [name=%s, span=%i, sync=%i, lcfg=0x%X,]\n", 
			wp->devname,lc->name, lc->span,lc->sync,lc->lineconfig);

#if 0
	/* This is not supported yet */
	if (lc->sync == 0) {
		if (WAN_TE1_CLK(&card->fe) != WAN_MASTER_CLK) {
			WAN_TE1_CLK(&card->fe) = WAN_MASTER_CLK;
			need_reconfig=1;	
		}
	} else {
		if (WAN_TE1_CLK(&card->fe) != WAN_NORMAL_CLK) {
			WAN_TE1_CLK(&card->fe) = WAN_NORMAL_CLK;
			need_reconfig=1;		
		}
	}
#endif

	if (!wp->ise1) {
		if (lc->lineconfig & ZT_CONFIG_B8ZS) {
			span->lineconfig |= ZT_CONFIG_B8ZS;
	    	wp->lcode = WAN_LCODE_B8ZS; 
		} else if (lc->lineconfig & ZT_CONFIG_AMI) { 
			span->lineconfig |= ZT_CONFIG_AMI;
			wp->lcode = WAN_LCODE_AMI;
		} else {
			DEBUG_ERROR("%s: Error: Invalid DAHDI T1 Coding: Lineconfig (0x%X)\n",
					card->devname, lc->lineconfig);
			return -EINVAL;
		}

		if (lc->lineconfig & ZT_CONFIG_ESF) {
			span->lineconfig |= ZT_CONFIG_ESF;
			wp->frame = WAN_FR_ESF; 
		} else if (lc->lineconfig & ZT_CONFIG_D4) {
			span->lineconfig |= ZT_CONFIG_D4;
			wp->frame = WAN_FR_D4;
		} else {
			DEBUG_ERROR("%s: Error: Invalid DAHDI T1 Framing: Lineconfig (0x%X)\n",
                    card->devname, lc->lineconfig);
            return -EINVAL;
		}

		if (WAN_TE1_LBO(&card->fe) != (lc->lbo+WAN_T1_0_133)) {
			WAN_TE1_LBO(&card->fe) =  lc->lbo+WAN_T1_0_133;
			need_reconfig=1;
		}

	} else {
		if (lc->lineconfig & ZT_CONFIG_HDB3){
			span->lineconfig |= ZT_CONFIG_HDB3;
			wp->lcode = WAN_LCODE_HDB3;
		} else {
			DEBUG_ERROR("%s: Error: Invalid DAHDI E1 Coding: Lineconfig (0x%X)\n",
					card->devname, lc->lineconfig);
			return -EINVAL;
		}

        wp->frame = WAN_FR_NCRC4;
		if (lc->lineconfig & ZT_CONFIG_CRC4) {
			span->lineconfig |= ZT_CONFIG_CRC4;
            wp->frame = WAN_FR_CRC4;
        } 

		if (lc->lineconfig & ZT_CONFIG_CCS) {
			span->lineconfig |= ZT_CONFIG_CCS;

			if (card->fe.fe_cfg.cfg.te_cfg.sig_mode != WAN_TE1_SIG_CCS) {
				card->fe.fe_cfg.cfg.te_cfg.sig_mode = WAN_TE1_SIG_CCS;
				need_reconfig=1;
			}
		} else {
			if (card->fe.fe_cfg.cfg.te_cfg.sig_mode != WAN_TE1_SIG_CAS) {
				need_reconfig=1;
				card->fe.fe_cfg.cfg.te_cfg.sig_mode = WAN_TE1_SIG_CAS;
			}
        } 

		WAN_TE1_LBO(&card->fe) = WAN_E1_120; 
	}

	if (WAN_FE_LCODE(&card->fe) != wp->lcode ||
        WAN_FE_FRAME(&card->fe) != wp->frame) {

		WAN_FE_LCODE(&card->fe) = wp->lcode;
		WAN_FE_FRAME(&card->fe) =  wp->frame;
		need_reconfig=1;
	}


	if (need_reconfig) { 
		if (card->wandev.fe_iface.config && 
			card->wandev.fe_iface.unconfig &&
			card->wandev.fe_iface.reconfig) {

			card->hw_iface.hw_lock(card->hw,&smp_flags);
			card->wandev.fe_iface.unconfig(&card->fe);
			card->hw_iface.hw_unlock(card->hw,&smp_flags);

			card->wandev.fe_iface.post_unconfig(&card->fe);

			card->hw_iface.hw_lock(card->hw,&smp_flags);
			card->wandev.fe_iface.config(&card->fe);
			card->wandev.fe_iface.reconfig(&card->fe);
			card->hw_iface.hw_unlock(card->hw,&smp_flags);

			card->wandev.fe_iface.post_init(&card->fe);
		}
	} else {
		DEBUG_EVENT("%s: Wanpipe front end skipped, no changes in config\n", wp->devname);
	}
		
	span->txlevel = 0;
	switch(wp->lbo){
	case WAN_T1_LBO_0_DB:  
       		DEBUG_TE1("%s: LBO 0 dB\n", card->devname);
		span->txlevel = 0;
		break;
	case WAN_T1_LBO_75_DB:
       		DEBUG_TE1("%s: LBO 7.5 dB\n", card->devname);
		span->txlevel = 5;
		break;
	case WAN_T1_LBO_15_DB: 
       		DEBUG_TE1("%s: LBO 15 dB\n", card->devname);
		span->txlevel = 6;
		break;
 	case WAN_T1_LBO_225_DB:
       		DEBUG_TE1("%s: LBO 22.5 dB\n", card->devname);
		span->txlevel = 7;
		break;
	case WAN_T1_0_110:    
        	DEBUG_TE1("%s: LBO 0-110 ft.\n", card->devname);
		span->txlevel = 0;
		break;
 	case WAN_T1_110_220:   
       		DEBUG_TE1("%s: LBO 110-220 ft.\n", card->devname);
		span->txlevel = 1;
		break;
 	case WAN_T1_220_330:   
       		DEBUG_TE1("%s: LBO 220-330 ft.\n", card->devname);
		span->txlevel = 2;
		break;
 	case WAN_T1_330_440:   
       		DEBUG_TE1("%s: LBO 330-440 ft.\n", card->devname);
		span->txlevel = 3;
		break;
 	case WAN_T1_440_550:   
       		DEBUG_TE1("%s: LBO 440-550 ft.\n", card->devname);
		span->txlevel = 3;
		break;
 	case WAN_T1_550_660:  
       		DEBUG_TE1("%s: LBO 550-660 ft.\n", card->devname);
		span->txlevel = 4;
		break;
	}

	span->rxlevel = 0;
	/* Do we want to SYNC on receive or not */
	wp->sync = lc->sync;

	/* */
	/* If already running, apply changes immediately */
	if (span->flags & ZT_FLAG_RUNNING){
#ifdef DAHDI_25
		err = wp_tdmv_startup(file,span);
#else
		err = wp_tdmv_startup(span);
#endif
	}

	if (card->wandev.fe_iface.read_alarm) {
		u32 alarm;
		card->hw_iface.hw_lock(card->hw,&smp_flags);
		alarm=card->wandev.fe_iface.read_alarm(&card->fe,WAN_FE_ALARM_READ|WAN_FE_ALARM_UPDATE);
		if (card->wandev.te_report_alarms) {
			card->wandev.te_report_alarms(card,alarm);
		}
		card->hw_iface.hw_unlock(card->hw,&smp_flags);
		
	}

	return err;
}


/******************************************************************************
** wp_tdmv_rbsbits() - Set A,B bits according TDM Voice requests. 
**
**	DONE
*/
static int wp_tdmv_rbsbits(struct zt_chan *chan, int bits)
{
	wp_tdmv_softc_t	*wp = NULL;
	sdla_t		*card = NULL;
	unsigned char	ABCD_bits = 0x00;
	wp_tdmv_rbs_t *rbs_pkt;
	netskb_t *rbs_skb;
			
	/* Byte offset */
	WAN_ASSERT2(chan == NULL, 0);
	wp	= WP_PRIV_FROM_CHAN(chan, wp_tdmv_softc_t);
	WAN_ASSERT2(wp == NULL, -ENODEV); 

	WAN_ASSERT2(wp->card == NULL, 0);
	card = (sdla_t*)wp->card;
	if (!wan_test_bit(chan->chanpos-1, &wp->timeslot_map)){
		return 0;	
	}
   	if (!wan_test_bit(WP_TDMV_SIG_ENABLE, &wp->flags)){
                return 0;
        }
	if (bits & ZT_ABIT) ABCD_bits |= WAN_RBS_SIG_A;
	if (bits & ZT_BBIT) ABCD_bits |= WAN_RBS_SIG_B;
	if (bits & ZT_CBIT) ABCD_bits |= WAN_RBS_SIG_C;
	if (bits & ZT_DBIT) ABCD_bits |= WAN_RBS_SIG_D;

	if (IS_CHAN_HARDHDLC(chan)){
		return 0;
	}
	DEBUG_TDMV(
	"[TDMV] %s: %s:%02d(%d) TX RBS: A:%1d B:%1d C:%1d D:%1d\n", 
			wp->devname, 
			(wp->ise1) ? "E1" : "T1",
			chan->chanpos, chan->channo,
			(ABCD_bits & WAN_RBS_SIG_A) ? 1 : 0,
			(ABCD_bits & WAN_RBS_SIG_B) ? 1 : 0,
			(ABCD_bits & WAN_RBS_SIG_C) ? 1 : 0,
			(ABCD_bits & WAN_RBS_SIG_D) ? 1 : 0);

	rbs_skb=wan_skb_alloc(sizeof(wp_tdmv_rbs_t));
	if (!rbs_skb) {
		DEBUG_EVENT("%s: Critical Error: TX RBS overrun for channel %d\n",
					wp->devname,
					chan->chanpos);
		return 0;	
	}
	rbs_pkt=(wp_tdmv_rbs_t*)wan_skb_put(rbs_skb,sizeof(wp_tdmv_rbs_t));

	DEBUG_TEST("%s: TX RBS Chan=%02i Data=0x%02X\n",
			wp->devname, chan->chanpos, ABCD_bits);
	rbs_pkt->chan=chan->chanpos;
	rbs_pkt->data=ABCD_bits;
	wan_skb_queue_tail(&wp->rbs_tx_q, rbs_skb);
	
#if 0
	if (wan_test_and_set_bit(chan->chanpos-1, &wp->rbs_tx_status)){
		if (ABCD_bits == wp->rbs_tx[chan->chanpos-1]){
			return 0;
		}
		if (wan_test_and_set_bit(chan->chanpos-1, &wp->rbs_tx1_status)){
			if (ABCD_bits == wp->rbs_tx1[chan->chanpos-1]){
				return 0;
			}
			DEBUG_EVENT("%s: Critical Error: TX RBS overrun for channel %d\n",
						wp->devname,
						chan->chanpos);
		}
		wp->rbs_tx1[chan->chanpos-1] = ABCD_bits;
	}else{
		wp->rbs_tx[chan->chanpos-1] = ABCD_bits;
	}
#endif
#if 0
	wan_set_bit(7, &ABCD_bits);
	if (wan_test_and_set_bit(7, &wp->rbs_tx[chan->chanpos-1])){
		if (ABCD_bits == wp->rbs_tx[chan->chanpos-1]){
			return 0;
		}
		if (wan_test_and_set_bit(7, &wp->rbs_tx1[chan->chanpos-1])){
			if (ABCD_bits == wp->rbs_tx1[chan->chanpos-1]){
				return 0;
			}
			DEBUG_EVENT("%s: Critical Error: TX RBS for channel %d\n",
						wp->devname,
						chan->chanpos);
		}
		wp->rbs_tx1[chan->chanpos-1] = ABCD_bits;
	}else{
		wp->rbs_tx[chan->chanpos-1] = ABCD_bits;
	}
#endif
	return 0;
}


/******************************************************************************
** wp_tdmv_is_rbsbits() -
**
**	Returns: 1 - start RBS poll routine, 0 - otherwise
*/
static int wp_tdmv_is_rbsbits(wan_tdmv_t *wan_tdmv)
{
	int ret=0;
	wp_tdmv_softc_t	*wp = NULL;
	
	WAN_ASSERT(wan_tdmv->sc == NULL);
	wp = wan_tdmv->sc;

	/* Do not read signalling bits if Asterisk not configured to */
   	if (!IS_TDMV_SIG(wp)){
		return 0;
	}

	if (!IS_TDMV_UP(wp)){
		wan_clear_bit(WP_TDMV_RBS_BUSY, &wp->flags);
		return 0;
	}

	if (wan_skb_queue_len(&wp->rbs_tx_q)) {
		ret=1;
	}
	
	if (wan_test_and_set_bit(WP_TDMV_RBS_BUSY, &wp->flags)){
		/* RBS read still in progress or not ready*/
		return ret;
	}

	/* Check rbs every 20ms */
	if ((SYSTEM_TICKS - wp->rbs_timeout) > HZ/50) {
		wp->rbs_timeout = SYSTEM_TICKS;
		ret=1;
		return ret;
	}

	/* Wait for the next time */
	wan_clear_bit(WP_TDMV_RBS_BUSY, &wp->flags);
	return ret;
}

/******************************************************************************
** wp_tdmv_rbsbits_poll() -
**
** Note: this code MUST run locked with hw_lock.
**       The calling code must take the hw_lock because this function
**       accesses the front end.
**
** DONE
*/

static int wp_tdmv_rbsbits_poll(wan_tdmv_t *wan_tdmv, void *card1)
{
	sdla_t		*card = (sdla_t*)card1;
	wp_tdmv_softc_t	*wp = NULL;
	int 		i, x;
	unsigned char status=0;
	
	WAN_ASSERT(wan_tdmv->sc == NULL);
	wp = wan_tdmv->sc;

	/* TX rbsbits */
	if (wan_skb_queue_len(&wp->rbs_tx_q)) {
		wp_tdmv_tx_rbsbits(wp);
	}
	
   	if (!IS_TDMV_UP(wp)){
		wan_clear_bit(WP_TDMV_RBS_BUSY, &wp->flags);
		return 0;
	}
	
	if (wp->rbs_rx_pending){

		DEBUG_TEST("%s: %s:%d: Reading RBS pending=0x%08X max_time_slot=%i\n",
				wp->devname,__FUNCTION__,__LINE__, wp->rbs_rx_pending,wp->max_timeslots);
				
		for(i=0; i < wp->max_timeslots;i++){
			if (wan_test_bit(i, &wp->rbs_rx_pending)){
				wan_clear_bit(i, &wp->rbs_rx_pending);

				DEBUG_TEST("%s: %s:%d: Reading RBS (pending=0x%08X) maxts=%i chan=%i\n",
				wp->devname,
				__FUNCTION__,__LINE__,
				wp->rbs_rx_pending,wp->max_timeslots,i+1);

				card->wandev.fe_iface.read_rbsbits(
					&card->fe, 
					i+1,
					WAN_TE_RBS_UPDATE|WAN_TE_RBS_REPORT);
			}
		}

		wan_clear_bit(WP_TDMV_RBS_BUSY, &wp->flags);
		return 0;
	}

	/* RX rbsbits */

	DEBUG_TEST("%s: %s:%d: Reading RBS \n",
				wp->devname,
				__FUNCTION__,__LINE__);

#if 1

	/* NENAD: The check_rbs seems to be broken for
	          E1 channel 31.  Default to reading rbs
	 		  each time intead of using check_rbs */
	for (x=0; x < wp->max_timeslots; x++) {
		if (wan_test_bit(x, &wp->sig_timeslot_map)) {
			status = card->wandev.fe_iface.read_rbsbits(
					&card->fe,
					x+1,
					WAN_TE_RBS_UPDATE|WAN_TE_RBS_REPORT);
		}
	}

	wan_clear_bit(WP_TDMV_RBS_BUSY, &wp->flags);


#else

	if (SYSTEM_TICKS - wp->rbs_sanity_timeout > HZ) {
		wp->rbs_sanity_timeout=SYSTEM_TICKS;
		for (x = 0; x < wp->max_timeslots; x++){
			if (wan_test_bit(x, &wp->sig_timeslot_map)){
				card->wandev.fe_iface.read_rbsbits(
					&card->fe,
					x+1,
					WAN_TE_RBS_UPDATE);
			}
		}
	} else {
		if (card->wandev.fe_iface.check_rbsbits == NULL){
                	DEBUG_ERROR("%s: Internal Error [%s:%d]!\n",
					wp->devname,
					__FUNCTION__,__LINE__); 
			return -EINVAL;
		}

		card->wandev.fe_iface.check_rbsbits(
				&card->fe, 
				1, wp->sig_timeslot_map, 0);
		card->wandev.fe_iface.check_rbsbits(
				&card->fe, 
				9, wp->sig_timeslot_map, 0);
		card->wandev.fe_iface.check_rbsbits(
				&card->fe, 
				17, wp->sig_timeslot_map, 0);
		if (wp->ise1){
			card->wandev.fe_iface.check_rbsbits(
					&card->fe, 
					25, wp->sig_timeslot_map, 0);
		}
	}

	wan_set_bit(WP_TDMV_RBS_UPDATE, &wp->flags);

#endif

	
	return 0;
}

/******************************************************************************
** wp_tdmv_tx_rbsbits() -
**
**	DONE
*/
static int wp_tdmv_tx_rbsbits(wp_tdmv_softc_t *wp)
{
	sdla_t	*card;
	//int	x;
	wp_tdmv_rbs_t *rbs_pkt;
	netskb_t *rbs_skb;

	WAN_ASSERT2(wp->card == NULL, 0);
	card = (sdla_t*)wp->card;
	
#if 0
	for(x=0;x<wp->max_timeslots;x++){
		if (wan_test_bit(x, &wp->rbs_tx_status)){
			card->wandev.fe_iface.set_rbsbits(
					&wp->card->fe, 
					x+1, 
					wp->rbs_tx[x]);
			wan_clear_bit(x, &wp->rbs_tx_status);
			if (wan_test_bit(x, &wp->rbs_tx1_status)){
				card->wandev.fe_iface.set_rbsbits(
						&wp->card->fe, 
						x+1, 
						wp->rbs_tx1[x]);
				wan_clear_bit(x, &wp->rbs_tx1_status);
			}
		}
	}
#else

	while ((rbs_skb=wan_skb_dequeue(&wp->rbs_tx_q))) {
		rbs_pkt=(wp_tdmv_rbs_t*)wan_skb_data(rbs_skb);

		DEBUG_TDMV("%s: TX RBS HW Chan=%02i Data=0x%02X\n",
			wp->devname, rbs_pkt->chan, rbs_pkt->data);

		card->wandev.fe_iface.set_rbsbits(
			&card->fe,
			rbs_pkt->chan, 
			rbs_pkt->data);

		wan_skb_free(rbs_skb);
	}
	
#endif

	return 0;
}

/******************************************************************************
** wp_tdmv_ioctl() - 
**
**	OK
*/
static int 
#if defined(__FreeBSD__) || defined(__OpenBSD__)
wp_tdmv_ioctl(struct zt_chan *chan, unsigned int cmd, caddr_t data)
#else
wp_tdmv_ioctl(struct zt_chan *chan, unsigned int cmd, unsigned long data)
#endif
{
	sdla_t			*card = NULL;
	wp_tdmv_softc_t		*wp = NULL;
	wan_event_ctrl_t	*event_ctrl = NULL;
	int			err = -ENOTTY, x;

#ifdef CONFIG_PRODUCT_WANPIPE_TDM_VOICE_ECHOMASTER
	int echo_detect_chan = 0;
#endif

	WAN_ASSERT(chan == NULL || chan->pvt == NULL);
	wp = chan->pvt;
	WAN_ASSERT(wp->card == NULL);
	card	= wp->card;
	
	switch(cmd) 
	{
	case ZT_TONEDETECT:
		err = WAN_COPY_FROM_USER(&x, (int*)data, sizeof(int));
		if (err) return -EFAULT;

		if (wp->tonesupport != WANOPT_YES || card->wandev.ec_dev == NULL){
			return -ENOSYS;
		}
		DEBUG_TDMV("[TDMV] %s: Hardware Tone Event detection (%s:%s)!\n",
			wp->devname,
			(x & ZT_TONEDETECT_ON) ? "ON" : "OFF",
			(x & ZT_TONEDETECT_MUTE) ? "Mute ON" : "Mute OFF");
					
		if (x & ZT_TONEDETECT_ON){
			wp->tonemask |= (1 << (chan->chanpos - 1));
		}else{
			wp->tonemask &= ~(1 << (chan->chanpos - 1));
		}
		if (x & ZT_TONEDETECT_MUTE){
			wp->tonemutemask |= (1 << (chan->chanpos - 1));
		}else{
			wp->tonemutemask &= ~(1 << (chan->chanpos - 1));
		}

#if defined(CONFIG_WANPIPE_HWEC)
		event_ctrl = wan_malloc(sizeof(wan_event_ctrl_t));
		if (event_ctrl==NULL){
			DEBUG_EVENT(
			"%s: Failed to allocate memory for event ctrl!\n",
						wp->devname);
			return -EFAULT;
		}
		event_ctrl->type = WAN_EVENT_EC_CHAN_MODIFY;
		event_ctrl->channel = chan->chanpos-1;
		event_ctrl->mode = (x & ZT_TONEDETECT_MUTE) ? WAN_EVENT_ENABLE : WAN_EVENT_DISABLE;
		if (wanpipe_ec_event_ctrl(card->wandev.ec_dev, card, event_ctrl)){
			wan_free(event_ctrl);
		}
		err = 0;
#else
		err = -EINVAL;
#endif
		break;
	
#if defined(CONFIG_PRODUCT_WANPIPE_TDM_VOICE_DCHAN) && defined(ZT_DCHAN_TX)
	case ZT_DCHAN_TX:

		if (wp->dchan_dev && wp->dchan_dev->hard_start_xmit){
			wp_tdmv_tx_dchan(chan, (int)data);
			err=0;
		}else{
			err=-EOPNOTSUPP;
		}
		break;
#endif
		
#ifdef CONFIG_PRODUCT_WANPIPE_TDM_VOICE_ECHOMASTER
	case SANGOMA_GET_ED_STATE:
		DEBUG_ECHO("%s():SANGOMA_GET_ED_STATE\n", __FUNCTION__);

		WAN_ASSERT(chan == NULL || chan->pvt == NULL);

		echo_detect_chan = chan->chanpos - 1;
		
		DEBUG_ECHO("on span: %d, chanpos: %d\n", 
					wp->spanno, 
					echo_detect_chan);
			
		if(echo_detect_chan > 30 || echo_detect_chan < 0){
			err=-EOPNOTSUPP;
		}else{
			wan_tdmv_t *wan_tdmv = &wp->card->wan_tdmv;
			wan_tdmv_rxtx_pwr_t *pwr_rxtx = &wan_tdmv->chan_pwr[echo_detect_chan];
	
			DEBUG_ECHO("%s():using %s table.\n", __FUNCTION__,
				(chan->xlaw == __zt_mulaw ? "MULAW" : "ALAW"));
						
			/* This will be used when reporting Echo Cancellation state 
			   from Asterisk CLI.
			*/
			chan->echo_detect_struct.echo_state = pwr_rxtx->current_state;
			
			DEBUG_ECHO("echo_state:%s\n",
				TDMV_SAMPLE_STATE_DECODE(chan->echo_detect_struct.echo_state));

			chan->echo_detect_struct.echo_present_samples_number =
				pwr_rxtx->echo_present_samples_number_history;
			chan->echo_detect_struct.echo_absent_samples_number = 
				pwr_rxtx->echo_absent_samples_number_history;
		}

		err = 0;
		break;
#endif /* CONFIG_PRODUCT_WANPIPE_TDM_VOICE_ECHOMASTER */
	
	default:
		/*DEBUG_EVENT("%s(): uknown cmd!\n", __FUNCTION__);*/
		err = -ENOTTY;
		break;
	}
	return err;
}

#ifdef DAHDI_25
/* This funciton is used for dahdi 2.5 or higher */
static const char *wp_tdmv_hwec_name(const struct zt_chan *chan)
{
	wp_tdmv_softc_t *wr = NULL;
	sdla_t *card = NULL;
	
	WAN_ASSERT2(chan == NULL, NULL);
	wr = WP_PRIV_FROM_CHAN(chan,wp_tdmv_softc_t);
	WAN_ASSERT2(wr == NULL, NULL);
	WAN_ASSERT2(wr->card == NULL, NULL);
	card = wr->card;
 
	if (card->wandev.ec_enable && wr->hwec == WANOPT_YES){
		return "WANPIPE_HWEC";
	}
    return NULL;
}
#endif


#ifdef DAHDI_22
/******************************************************************************
** wp_tdmv_hwec_create() - 
**
**	OK
*/
static int wp_tdmv_hwec_create(struct dahdi_chan *chan, 
							   struct dahdi_echocanparams *ecp,
							   struct dahdi_echocanparam *p, 
							   struct dahdi_echocan_state **ec)
{
	wp_tdmv_softc_t	*wp = NULL;
	sdla_t *card = NULL;
	int	err = -ENODEV;
	
	if (ecp->param_count > 0) {
		DEBUG_TDMV("[TDMV] Wanpipe echo canceller does not support parameters; failing request\n");
		return -EINVAL;
	}
	
	WAN_ASSERT2(chan == NULL, -ENODEV);
	WAN_ASSERT2(chan->pvt == NULL, -ENODEV);
	wp = chan->pvt;
	WAN_ASSERT2(wp->card == NULL, -ENODEV);
	card = wp->card;
	
	if (wp->hwec != WANOPT_YES || !card->wandev.ec_dev){
    	return err;
	}

	*ec = &wp->ec[chan->chanpos - 1];
	(*ec)->ops = &wp_tdmv_ec_ops;
	(*ec)->features = wp_tdmv_ec_features;
	
	wan_set_bit((chan->chanpos - 1), &card->wandev.rtp_tap_call_map);
	wp_fax_tone_timeout_set(wp, chan->chanpos-1);

	
	if (card->wandev.ec_enable && wp->hwec == WANOPT_YES){
		DEBUG_TDMV("[TDMV] %s: %s HW echo canceller on channel %d\n",
				   wp->devname,
				   "Enable",
				   chan->chanpos);
		

		/* The ec persist flag enables and disables
		* persistent echo control.  In persist mode
		* echo cancellation is enabled regardless of
		* asterisk.  In persist mode off asterisk 
		* controls hardware echo cancellation */
		if (card->hwec_conf.persist_disable || IS_CHAN_HARDHDLC(chan)) {
			err = card->wandev.ec_enable(card, 1, chan->chanpos);
		} else {
			err = 0;			
		}
	}

	return err;
}

/*  wp_tdmv_hwec_free() - 
**
**	OK
*/
static void wp_tdmv_hwec_free(struct dahdi_chan *chan, struct dahdi_echocan_state *ec)
{
	wp_tdmv_softc_t	*wp = NULL;
	sdla_t *card = NULL;

	memset(ec, 0, sizeof(*ec));

	if(chan == NULL) return;
	if(chan->pvt == NULL) return;
	wp = chan->pvt;
	if(wp->card == NULL) return;
	card = wp->card;

	if (wp->hwec != WANOPT_YES || !card->wandev.ec_dev){
    	return;
	}

	wan_clear_bit(chan->chanpos-1, &card->wandev.rtp_tap_call_map);

	if (card->wandev.ec_enable && wp->hwec == WANOPT_YES) {
		DEBUG_TDMV("[TDMV] %s: Disable HW echo canceller on channel %d\n",
				wp->devname, chan->chanpos);

		/* The ec persist flag enables and disables
	     * persistent echo control.  In persist mode
              * echo cancellation is enabled regardless of
                 * asterisk.  In persist mode off asterisk 
                 * controls hardware echo cancellation */

        if (wp->ec_off_on_fax) {
			DEBUG_EVENT("%s: Re-enabling hwec after fax chan=%i \n",card->devname,chan->chanpos);
            card->wandev.ec_enable(card, 1, chan->chanpos);
            wp->ec_off_on_fax=0;
		} else {
	   		if (card->hwec_conf.persist_disable || IS_CHAN_HARDHDLC(chan) || wp->ec_off_on_fax) {
				card->wandev.ec_enable(card, 0, chan->chanpos);
			}
		}
	}
	
	return;
}


#else
/******************************************************************************
** wp_tdmv_hwec() - 
**
**	OK
*/
static int wp_tdmv_hwec(struct zt_chan *chan, int enable)
{
	wp_tdmv_softc_t	*wp = NULL;
	sdla_t		*card = NULL;
	int		err = -EINVAL;
	
	WAN_ASSERT2(chan == NULL, -ENODEV);
	WAN_ASSERT2(chan->pvt == NULL, -ENODEV);
	wp = chan->pvt;
	WAN_ASSERT2(wp->card == NULL, -ENODEV);
	card = wp->card;

	if (enable) {
		wan_set_bit(chan->chanpos-1,&card->wandev.rtp_tap_call_map);
		wp_fax_tone_timeout_set(wp, chan->chanpos-1);
	} else {
		wan_clear_bit(chan->chanpos-1,&card->wandev.rtp_tap_call_map);
	}

	if (card->wandev.ec_enable && wp->hwec == WANOPT_YES){
		DEBUG_TDMV("[TDMV] %s: %s HW echo canceller on channel %d\n",
				wp->devname,
				(enable) ? "Enable" : "Disable",
				chan->chanpos);
		

		/* The ec persist flag enables and disables
	         * persistent echo control.  In persist mode
                 * echo cancellation is enabled regardless of
                 * asterisk.  In persist mode off asterisk 
                 * controls hardware echo cancellation */

        if (!enable &&  wp->ec_off_on_fax) {
            DEBUG_EVENT("%s: Re-enabling hwec after fax chan=%i \n",card->devname,chan->chanpos);
            card->wandev.ec_enable(card, 1, chan->chanpos);
            wp->ec_off_on_fax=0;   
			err=0;
		} else {
			if (card->hwec_conf.persist_disable || IS_CHAN_HARDHDLC(chan)) {
				err = card->wandev.ec_enable(card, enable, chan->chanpos);
			} else {
				err = 0;			
			}
		}

	}

	return err;
}
#endif


/******************************************************************************
** wp_tdmv_open() - 
**
**	OK
*/
static int wp_tdmv_open(struct zt_chan *chan)
{
	wp_tdmv_softc_t	*wp = NULL;
	sdla_t		*card = NULL;
	wan_smp_flag_t flags;

	WAN_ASSERT2(chan == NULL, -ENODEV);
	WAN_ASSERT2(chan->pvt == NULL, -ENODEV);
	wp = chan->pvt;
	WAN_ASSERT2(wp->card == NULL, -ENODEV);
	card = wp->card;
	wan_spin_lock_irq(&wp->lockirq, &flags);
	wp->usecount++;
	wan_spin_unlock_irq(&wp->lockirq, &flags);
	wanpipe_open(card);
	DEBUG_TDMV("%s: Open (usecount=%d, channo=%d, chanpos=%d)...\n", 
				wp->devname,
				wp->usecount,
				chan->channo,
				chan->chanpos);
	return 0;
}


/******************************************************************************
** wp_tdmv_close() - 
**
**	OK
*/
static int wp_tdmv_close(struct zt_chan *chan)
{
	wp_tdmv_softc_t*	wp = NULL;
	sdla_t		*card = NULL;
	wan_smp_flag_t flags;

	WAN_ASSERT2(chan == NULL, -ENODEV);
	WAN_ASSERT2(chan->pvt == NULL, -ENODEV);
	wp = chan->pvt;
	WAN_ASSERT2(wp->card == NULL, -ENODEV);
	card = wp->card;
	wan_spin_lock_irq(&wp->lockirq, &flags);
	wp->usecount--;
	wan_spin_unlock_irq(&wp->lockirq, &flags);
	wanpipe_close(card);
	DEBUG_TDMV("%s: Close (usecount=%d, channo=%d, chanpos=%d)...\n", 
				wp->devname,
				wp->usecount,
				chan->channo,
				chan->chanpos);
	return 0;
}

/******************************************************************************
** wp_tdmv_release() - 
**
**	OK
*/
static void wp_tdmv_release(wp_tdmv_softc_t *wp)
{
	sdla_t *card;

	WAN_ASSERT1(wp == NULL);
	WAN_ASSERT1(wp->card == NULL);
	
	card=wp->card;

	if (wan_test_bit(WP_TDMV_REGISTER, &wp->flags)){
		DEBUG_EVENT("%s: Unregister Wanpipe device from Zaptel!\n",
				wp->devname);
		wan_clear_bit(WP_TDMV_SIG_POLL, &wp->flags);
		wan_clear_bit(WP_TDMV_REGISTER, &wp->flags);

		wp_dahdi_unregister_device(wp);

		wan_clear_bit(WP_TDMV_REGISTER, &wp->flags);
		wan_skb_queue_purge(&wp->rbs_tx_q);
	}

	wp_dahdi_free_device(wp);

	wan_free(wp);
}

static inline void start_alarm(wp_tdmv_softc_t* wp)
{
	WAN_ASSERT1(wp == NULL);
#ifdef FANCY_ALARM
	wp->alarmpos = 0;
#endif
	wp->blinktimer = 0;
}

static inline void stop_alarm(wp_tdmv_softc_t* wp)
{
	WAN_ASSERT1(wp == NULL);
#ifdef FANCY_ALARM
	wp->alarmpos = 0;
#endif
	wp->blinktimer = 0;
}

/************************************************************************************
 * Channelized code for rx/tx
 * *********************************************************************************/

#if defined(CONFIG_PRODUCT_WANPIPE_TDM_VOICE_DCHAN)
/******************************************************************************
** wp_tdmv_rx_dchan() - 
**
**	OK
*/
static int wp_tdmv_rx_dchan(wan_tdmv_t *wan_tdmv, int channo, 
			unsigned char *rxbuf, unsigned int len)
{
	wp_tdmv_softc_t	*wp = wan_tdmv->sc;
	struct zt_chan	*chan = NULL, *ms = NULL;
	wan_smp_flag_t	smp_flags;
	unsigned char	*buf = NULL;
	int		oldbuf;
	int 		i, left;

	WAN_ASSERT(wp == NULL);

        if (!wan_test_bit(channo, &wp->dchan_map)) {
		DEBUG_ERROR("%s: Internal Error: DCHAN Mismatch channo=%i 0x%08X\n",
				   wp->devname,channo,wp->dchan_map);
        	return -EINVAL;
	}

	chan	= &wp->chans[channo];
	WAN_ASSERT(chan == NULL || chan->master == NULL);
	ms = chan->master;
	
	if (!IS_TDMV_UP(wp)){
		DEBUG_TDMV("%s: Asterisk is not running!\n",
				wp->devname); 
		return -EINVAL;
	}
	if (!IS_CHAN_HARDHDLC(ms)){
		DEBUG_TDMV("%s: ERROR: %s not defined as D-CHAN!\n",
				wp->devname, ms->name); 
		return -EINVAL;
	}
	
	if (ms->inreadbuf < 0){
		return -EINVAL;
	}

	if (ms->inreadbuf >= ZT_MAX_NUM_BUFS){
		DEBUG_EVENT("%s: RX buffer (%s) is out of range (%d-%d)!\n",
				wp->devname, ms->name, ms->inreadbuf,ZT_MAX_NUM_BUFS); 
		return -EINVAL;
	}

	/* FIXME wan_spin_lock_irqsave(&wp->tx_rx_lock, smp_flags); */
	wan_spin_lock_irq(&chan->lock, &smp_flags);
	buf = ms->readbuf[ms->inreadbuf];
	left = ms->blocksize - ms->readidx[ms->inreadbuf];
	if (len + 2 > left) {
		DEBUG_ERROR("%s: ERROR: Not ehough space for RX HDLC packet (%d:%d)!\n",
				wp->devname, len+2, left); 
		wan_spin_unlock_irq(&chan->lock, &smp_flags);
		return -EINVAL;
	}
	for(i = 0; i < len; i++){
		buf[ms->readidx[ms->inreadbuf]++] = rxbuf[i];
	}
	/* Add extra 2 bytes for checksum */
	buf[ms->readidx[ms->inreadbuf]++] = 0x00;
	buf[ms->readidx[ms->inreadbuf]++] = 0x00;

	oldbuf = ms->inreadbuf;
	ms->readn[ms->inreadbuf] = ms->readidx[ms->inreadbuf];
	ms->inreadbuf = (ms->inreadbuf + 1) % ms->numbufs;
	if (ms->inreadbuf == ms->outreadbuf) {
		/* Whoops, we're full, and have no where else
		to store into at the moment.  We'll drop it
		until there's a buffer available */
		ms->inreadbuf = -1;
		/* Enable the receiver in case they've got POLICY_WHEN_FULL */
		wp_dahdi_chan_set_rxdisable(ms,0);
	}
	if (ms->outreadbuf < 0) { /* start out buffer if not already */
		ms->outreadbuf = oldbuf;
	}
	/* FIXME wan_spin_unlock_irq(&wp->tx_rx_lock, &smp_flags); */
	wan_spin_unlock_irq(&chan->lock, &smp_flags);
	if (!wp_dahdi_chan_rxdisable(ms)) { /* if receiver enabled */
		DEBUG_TDMV("%s: HDLC block is ready!\n",
					wp->devname);
		/* Notify a blocked reader that there is data available
		to be read, unless we're waiting for it to be full */
#if defined(__LINUX__)
#ifdef DAHDI_25
		wake_up_interruptible(&ms->waitq);
		if (ms->iomask & ZT_IOMUX_READ)
			wake_up_interruptible(&ms->waitq);
#else
		wake_up_interruptible(&ms->readbufq);
		wake_up_interruptible(&ms->sel);
		if (ms->iomask & ZT_IOMUX_READ)
			wake_up_interruptible(&ms->eventbufq);
#endif

#elif defined(__FreeBSD__) || defined(__OpenBSD__)
#ifdef DAHDI_25
		wakeup(&ms->waitq);
		if (ms->iomask & ZT_IOMUX_READ)
			wakeup(&ms->waitq);
#else
		wakeup(&ms->readbufq);
		_selwakeup(&ms->sel);
		if (ms->iomask & ZT_IOMUX_READ)
			wakeup(&ms->eventbufq);
#endif
#endif
	}
	return 0;
}
#endif

#if defined(CONFIG_PRODUCT_WANPIPE_TDM_VOICE_DCHAN) && defined(CONFIG_PRODUCT_WANPIPE_TDM_VOICE_DCHAN_ZAPTEL)
static void wp_tdmv_tx_hdlc_hard(struct zt_chan *chan)
{
	wp_tdmv_softc_t	*wp = NULL;
	netskb_t	*skb = NULL;
	unsigned char	*data = NULL;
	int		size = 0, err = 0, res = 0;

	WAN_ASSERT_VOID(chan == NULL);
	WAN_ASSERT_VOID(chan->pvt == NULL);
	wp	= chan->pvt;
	WAN_ASSERT_VOID(wp->dchan_dev == NULL);

	size = chan->writen[chan->outwritebuf] - chan->writeidx[chan->outwritebuf]-2;
	if (!size) {
     /* Do not transmit zero length frame */
     zt_hdlc_getbuf(chan, data, &size);     
		return;
	}

	skb = wan_skb_alloc(size+1);
	if (skb == NULL){
        	return;
	}
	data = wan_skb_put(skb, size);
	res = zt_hdlc_getbuf(chan, data, &size);
	if (res == 0){
		DEBUG_ERROR("%s: ERROR: TX HW DCHAN %d bytes (res %d)\n",
					wp->devname, size, res);
	}
        err = WAN_NETDEV_XMIT(skb,wp->dchan_dev);
	if (err){
		wan_skb_free(skb);
	}
	return;
}
#endif

#if defined(CONFIG_PRODUCT_WANPIPE_TDM_VOICE_DCHAN) && defined(ZT_DCHAN_TX)
static int wp_tdmv_tx_dchan(struct zt_chan *chan, int len)
{
	wp_tdmv_softc_t	*wp = NULL;
	netskb_t	*skb = NULL;
	wan_smp_flag_t	smp_flags;
	unsigned char	*data = NULL;
	int		err = 0;

	WAN_ASSERT2(chan == NULL, -ENODEV);
	WAN_ASSERT2(chan->pvt == NULL, -ENODEV);
	wp	= chan->pvt;
	WAN_ASSERT(wp->dchan_dev == NULL);

	if (len <= 2){
		return -EINVAL;
	}
	len -= 2; /* Remove checksum */

	skb = wan_skb_alloc(len+1);
	if (skb == NULL){
        	return -ENOMEM;
	}
	data = wan_skb_put(skb, len);
	wan_spin_lock_irq(&chan->lock, &smp_flags);
	memcpy(data, chan->writebuf[chan->inwritebuf], len);
	wan_spin_unlock_irq(&chan->lock, &smp_flags);
#if 0
	{
	int i;
	DEBUG_EVENT("TX DCHAN: ");
	for(i = 0; i < len; i++){
		_DEBUG_EVENT("%02X ", data[i]);
	}
	_DEBUG_EVENT("\n");
	}
#endif
	if (skb){
		err = wp->dchan_dev->hard_start_xmit(skb, wp->dchan_dev);
		if (err){
			wan_skb_free(skb);
		}
	}

	return err;
}
#endif


/******************************************************************************
** wp_tdmv_rx_chan() - 
**
**	OK
*/
static int wp_tdmv_rx_chan(wan_tdmv_t *wan_tdmv, int channo, 
			unsigned char *rxbuf,
			unsigned char *txbuf)
{
	wp_tdmv_softc_t	*wp = wan_tdmv->sc;
	sdla_t *card;
	
	WAN_ASSERT2(wp == NULL, -EINVAL);
	WAN_ASSERT2(channo < 0, -EINVAL);
	WAN_ASSERT2(channo > 31, -EINVAL);

	if (!IS_TDMV_UP(wp)){
		return -EINVAL;
	}
	card = wp->card;

	if (wp->channelized == WAN_TRUE){
#ifdef CONFIG_PRODUCT_WANPIPE_TDM_VOICE_ECHOMASTER
		wan_tdmv_rxtx_pwr_t *pwr_rxtx = &wan_tdmv->chan_pwr[channo];
#endif

		wp->chans[channo].readchunk = rxbuf;	
		wp->chans[channo].writechunk = txbuf;	

#ifdef CONFIG_PRODUCT_WANPIPE_TDM_VOICE_ECHOMASTER
		wp_tdmv_echo_check(wan_tdmv, &wp->chans[channo], channo);
#endif		

		/* If using hwec do not even call ec chunk */
		if ((!card->wandev.ec_enable || card->wandev.ec_enable_map == 0) && 
		     !wan_test_bit(channo, &wp->echo_off_map)) {
/*Echo spike starts at 25bytes*/
#ifdef CONFIG_PRODUCT_WANPIPE_TDM_VOICE_ECHOMASTER
			if(pwr_rxtx->current_state != ECHO_ABSENT){
#endif
#if 0
/* Echo spike starts at 16 bytes */	

				zt_ec_chunk(
					&wp->chans[channo], 
					wp->chans[channo].readchunk, 
					wp->chans[channo].writechunk);
#endif

#if 1			
/*Echo spike starts at 9 bytes*/
				zt_ec_chunk(
					&wp->chans[channo], 
					wp->chans[channo].readchunk, 
					wp->ec_chunk1[channo]);
				memcpy(
					wp->ec_chunk1[channo],
					wp->chans[channo].writechunk,
					ZT_CHUNKSIZE);
#endif

#if 0			
/*Echo spike starts at bytes*/
				zt_ec_chunk(
					&wp->chans[channo], 
					wp->chans[channo].readchunk, 
					wp->ec_chunk1[channo]);
				memcpy(
					wp->ec_chunk1[channo],
					wp->ec_chunk2[channo],
					ZT_CHUNKSIZE);

				memcpy(
					wp->ec_chunk2[channo],
					wp->chans[channo].writechunk,
					ZT_CHUNKSIZE);
#endif

#ifdef CONFIG_PRODUCT_WANPIPE_TDM_VOICE_ECHOMASTER
			} /* if(pwr_rxtx->current_state != ECHO_ABSENT) */
#endif
		} /* if (!wan_test_bit(channo, &wp->echo_off_map)) */
	}else{
		int	x, channels = wp->span.channels;

		wp->tx_unchan = txbuf;
		wp_tdmv_rxcopy(wan_tdmv, rxbuf, wp->max_rxtx_len);

		if (!wp->echo_off_map){
			for (x = 0; x < channels; x++){
#if 0
				/* This code never runs. Instead wp_tdmv_rx_chan()
				** is called for A101/A102. All Echo Detection is
				** done there for A101/A102. */
				wp_tdmv_echo_check(wan_tdmv, &wp->chans[0], x);
#endif

				zt_ec_chunk(
					&wp->chans[x], 
					wp->chans[x].readchunk, 
					wp->chans[x].writechunk);

#if 0
				zt_ec_chunk(
					&wp->chans[x], 
					wp->chans[x].readchunk, 
					wp->ec_chunk1[x]);
				memcpy(
					wp->ec_chunk1[x],
					wp->chans[x].writechunk,
					ZT_CHUNKSIZE);
#endif
			}
		}/* if() */
	}/* if() */

	return 0;
}

#undef AFT_TDM_ROTATE_DEBUG
#ifdef AFT_TDM_ROTATE_DEBUG
static int rotate_debug[32];
#warning "AFT_TDM_ROTATE_DEBUG Enabled"
#include "aft_core.h"
#endif
static int wp_tdmv_span_buf_rotate(void *pcard, u32 buf_sz, unsigned long mask, int circ_buf_len)
{
	sdla_t		*card = (sdla_t*)pcard;
	wan_tdmv_t	*wan_tdmv = &card->wan_tdmv;
	wp_tdmv_softc_t	*wp = NULL;
	int x;
	int tidx;
	unsigned int rx_offset, tx_offset;
	void *ptr;
	wan_smp_flag_t flag;

	WAN_ASSERT(wan_tdmv->sc == NULL);
	wp = wan_tdmv->sc; 

#ifdef AFT_TDM_ROTATE_DEBUG
	rotate_debug[card->tdmv_conf.span_no]++;
#endif

	for (x = 0; x < 32; x ++) {

		if (wan_test_bit(x,&wp->timeslot_map)) {

			if (card->u.aft.tdmv_dchan &&
			   (card->u.aft.tdmv_dchan-1) == x) {
				continue;
			}

			if (wp->ise1) {
				tidx=x+1;
			} else {
				tidx=x;
			}

#ifdef AFT_TDM_ROTATE_DEBUG
			if (1) {
				u32 dma_descr, lo_reg, lo_reg_tx, lch;
				lch=x;

				dma_descr=(lch<<4) +
							AFT_PORT_REG(card,AFT_RX_DMA_LO_DESCR_BASE_REG);
				card->hw_iface.bus_read_4(card->hw,dma_descr,&lo_reg);
			
				dma_descr=(lch<<4) +
							AFT_PORT_REG(card,AFT_TX_DMA_LO_DESCR_BASE_REG);
				card->hw_iface.bus_read_4(card->hw,dma_descr,&lo_reg_tx);
			
				lo_reg=(lo_reg&AFT_TDMV_BUF_MASK)/AFT_TDMV_CIRC_BUF;
				lo_reg_tx=(lo_reg_tx&AFT_TDMV_BUF_MASK)/AFT_TDMV_CIRC_BUF;

#if 0
				if (rotate_debug[card->tdmv_conf.span_no] < 5) {
#else
				if (rotate_debug[card->tdmv_conf.span_no] < 5 ||
					abs(card->u.aft.tdm_rx_dma_toggle[tidx] - lo_reg) != 2 ||
					abs(card->u.aft.tdm_tx_dma_toggle[tidx] - lo_reg_tx) != 2) {
#endif
					DEBUG_EVENT("%s: Rotating buffer lch=%i tidx=%i  rxdma=%i (hw=%i), txdma=%i (hw=%i)\n",
						card->devname, lch, tidx, card->u.aft.tdm_rx_dma_toggle[tidx], lo_reg,card->u.aft.tdm_tx_dma_toggle[tidx],lo_reg_tx);
				}
			}
#endif

			rx_offset = buf_sz * card->u.aft.tdm_rx_dma_toggle[tidx];
			tx_offset = buf_sz * card->u.aft.tdm_tx_dma_toggle[tidx];	

			card->u.aft.tdm_rx_dma_toggle[tidx]++;
			if (card->u.aft.tdm_rx_dma_toggle[tidx] >= circ_buf_len) {
				card->u.aft.tdm_rx_dma_toggle[tidx]=0;
			}
			card->u.aft.tdm_tx_dma_toggle[tidx]++;
			if (card->u.aft.tdm_tx_dma_toggle[tidx] >= circ_buf_len) {
				card->u.aft.tdm_tx_dma_toggle[tidx]=0;
			}

			wan_spin_lock(&wp->chans[x].lock,&flag);

			ptr=(void*)((((unsigned long)wp->chans[x].readchunk) & ~(mask)) + rx_offset);
			wp->chans[x].readchunk = ptr;

			ptr=(void*)((((unsigned long)wp->chans[x].writechunk) & ~(mask)) + tx_offset);
			wp->chans[x].writechunk = ptr;
			
			wan_spin_unlock(&wp->chans[x].lock,&flag);


#if defined(__LINUX__)
			if (card->u.aft.tdm_rx_dma[x]) {
				wan_dma_descr_t *dma_descr = card->u.aft.tdm_rx_dma[x]; 
				card->hw_iface.busdma_sync(card->hw, dma_descr, 1, 1, PCI_DMA_FROMDEVICE);
			}

			if (card->u.aft.tdm_tx_dma[x]) {
				wan_dma_descr_t *dma_descr = card->u.aft.tdm_tx_dma[x]; 
				card->hw_iface.busdma_sync(card->hw, dma_descr, 1, 1, PCI_DMA_TODEVICE);
			}

			prefetch(wp->chans[x].readchunk);
			prefetch(wp->chans[x].writechunk);
#endif

			if (card->wandev.rtp_len && card->wandev.rtp_tap) {
				card->wandev.rtp_tap(card,x,
						     wp->chans[x].readchunk,
						     wp->chans[x].writechunk,
						     ZT_CHUNKSIZE);
			}
		}
	}          

	return 0;
}

static int wp_tdmv_ec_span(void *pcard)
{
	sdla_t          *card = (sdla_t*)pcard;
        wan_tdmv_t      *wan_tdmv = &card->wan_tdmv;
        wp_tdmv_softc_t *wp = NULL;

        WAN_ASSERT(wan_tdmv->sc == NULL);
        wp = wan_tdmv->sc;

	zt_ec_span(&wp->span);

	return 0;

}

/******************************************************************************
** wp_tdmv_rx_tx_span() - 
**
**	OK
*/
static int wp_tdmv_rx_tx_span(void *pcard)
{
	sdla_t		*card = (sdla_t*)pcard;
	wan_tdmv_t	*wan_tdmv = &card->wan_tdmv;
	wp_tdmv_softc_t	*wp = NULL;
	
	WAN_ASSERT(wan_tdmv->sc == NULL);
	wp = wan_tdmv->sc;

	wp->intcount++;
	zt_receive(&wp->span);
	zt_transmit(&wp->span);
	
	if (wp->channelized == WAN_FALSE){
		wp_tdmv_txcopy(wan_tdmv,
				wp->tx_unchan,
				wp->max_rxtx_len);
	}

	return 0;
}


/******************************************************************************
** wp_tdmv_init() - 
*
**	OK
*/
static int wp_tdmv_init(void* pcard, wanif_conf_t *conf)
{
	return -EINVAL;
}

/******************************************************************************
** wp_tdmv_callback_tone() - 
*
**	OK
*/
static void wp_tdmv_callback_tone (void* card_id, wan_event_t *event)
{
	sdla_t		*card = (sdla_t*)card_id;
	wan_tdmv_t      *wan_tdmv = &card->wan_tdmv;
	wp_tdmv_softc_t	*wp = NULL;
	int fechan = 	event->channel;
	
	WAN_ASSERT1(wan_tdmv->sc == NULL);
	wp = wan_tdmv->sc;

	if (event->channel <= 0) {
     	DEBUG_ERROR("%s: Error: wp_tdmv_callback_tone() Invalid Event Channel = %i\n",
				card->devname, event->channel);
		return;
	}

	/* fechan check is a sanity check */
    fechan=event->channel-1;
	
	switch(event->type) {
     	case WAN_EVENT_EC_DTMF:
		case WAN_EVENT_EC_FAX_1100:
			break;
		default:
			  DEBUG_ERROR("%s: %s() Error Invalid event type %X (%s)!\n",
				card->devname, __FUNCTION__, event->type, WAN_EVENT_TYPE_DECODE(event->type));      
			return;
	}

	DEBUG_TDMV(
	"[TDMV] %s: Received EC Tone (%s) Event at TDM (chan=%d digit=%c port=%s type=%s)!\n",
			card->devname,
			WAN_EVENT_TYPE_DECODE(event->type),
			event->channel,
			event->digit,
			(event->tone_port == WAN_EC_CHANNEL_PORT_ROUT)?"ROUT":"SOUT",
			(event->tone_type == WAN_EC_TONE_PRESENT)?"PRESENT":"STOP");
					
	if (!(wp->tonemask & (1 << (event->channel-1)))){
		DEBUG_TDMV(
		"[TDMV] %s: Tone detection is not enabled for the channel %d\n",
					card->devname,
					event->channel);
		return;
	}

	if (event->digit == 'f' && fechan >= 0) {

		if (!card->tdmv_conf.hw_fax_detect) {
			DEBUG_TDMV("%s: Received Fax Detect event while hw fax disabled !\n",card->devname);
			return;
		}

		if (card->tdmv_conf.hw_fax_detect == WANOPT_YES) {
         	card->tdmv_conf.hw_fax_detect=8;
		}

		if (wp->ec_fax_detect_timeout[fechan] == 0) {
			DEBUG_TDMV("%s: FAX DETECT TIMEOUT --- Not initialized!\n",card->devname);
			return;

		} else 	if (card->tdmv_conf.hw_fax_detect &&
	    		   (SYSTEM_TICKS - wp->ec_fax_detect_timeout[fechan]) >= card->tdmv_conf.hw_fax_detect*HZ) {
#ifdef WAN_DEBUG_TDMAPI
			if (WAN_NET_RATELIMIT()) {
				DEBUG_WARNING("%s: Warning: Ignoring Fax detect during call (s%dc%d) - Call Time: %ld  Max: %d!\n",
					card->devname,
					wp->spanno+1,
					event->channel,
					(SYSTEM_TICKS - wp->ec_fax_detect_timeout[fechan])/HZ,
					card->tdmv_conf.hw_fax_detect);
			}
#endif
			return;
		} else {

			DEBUG_TDMV("%s: FAX DETECT OK --- Ticks=%lu Timeout=%lu Diff=%lu! s%dc%d\n",
				card->devname,SYSTEM_TICKS,wp->ec_fax_detect_timeout[fechan],
				(SYSTEM_TICKS - wp->ec_fax_detect_timeout[fechan])/HZ,
				card->wan_tdmv.spanno,fechan);
		}
	}

	if (event->tone_type == WAN_EC_TONE_PRESENT){
		wp->toneactive |= (1 << (event->channel-1));
#ifdef DAHDI_ISSUES
		zt_qevent_lock(
				wp->span.chans[event->channel-1],
				(ZT_EVENT_DTMFDOWN | event->digit));
#else
		zt_qevent_lock(
				&wp->span.chans[event->channel-1],
				(ZT_EVENT_DTMFDOWN | event->digit));
#endif

		if (wp->hwec == WANOPT_YES && card->wandev.ec_dev && card->wandev.ec_enable && card->tdmv_conf.ec_off_on_fax) {
			/* Disable hwec on fax event if configuration option is enabled 
			   Disable hwec only if persist disalbe is not enabled, since in that mode asterisk controls hwec.
			   Disable hwec only once even though there might be many fax events */
           	if (!card->hwec_conf.persist_disable && !wp->ec_off_on_fax) {
				 DEBUG_EVENT("%s: Disabling hwec on fax event chan=%i\n",card->devname,event->channel);
				 card->wandev.__ec_enable(card, 0, event->channel);  
                 wp->ec_off_on_fax=1;
			}
		}

	}else{
		wp->toneactive &= ~(1 << (event->channel-1));
#ifdef DAHDI_ISSUES
		zt_qevent_lock(
				wp->span.chans[event->channel-1],
				(ZT_EVENT_DTMFUP | event->digit));
#else
		zt_qevent_lock(
				&wp->span.chans[event->channel-1],
				(ZT_EVENT_DTMFUP | event->digit));
#endif


	}
	return;
}
