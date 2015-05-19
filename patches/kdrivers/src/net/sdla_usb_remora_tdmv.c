/***************************************************************************
 * sdla_remora_tdmv.c	WANPIPE(tm) Multiprotocol WAN Link Driver. 
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
 * Oct 6, 2005	Alex Feldman	Initial version.
 ******************************************************************************
 */
/*******************************************************************************
**			   INCLUDE FILES
*******************************************************************************/

# include "wanpipe_includes.h"
# include "wanpipe_debug.h"
# include "wanpipe_defines.h"

# include "wanpipe_abstr.h"
# include "wanpipe_common.h"
# include "wanpipe.h"
# include "wanpipe_events.h"
# include "sdla_usb_remora.h"

#if defined (__WINDOWS__)
# include <wanpipe\csu_dsu.h>
#else
# include "zapcompat.h" /* Map of Zaptel -> DAHDI definitions */
# include "wanpipe_dahdi_abstr.h"
#endif


#if defined(CONFIG_PRODUCT_WANPIPE_USB)        

/*******************************************************************************
**			  DEFINES AND MACROS
*******************************************************************************/

#if 0
# define WP_USB_TDMV_FUNC_DEBUG
#else
# define WP_USB_TDMV_FUNC_DEBUG  DEBUG_EVENT("[USB-TDMV] %s:%d\n",__FUNCTION__,__LINE__)
#endif

#define REG_SHADOW
#define REG_WRITE_SHADOW

#if 0
# define	SPI2STEP
#endif
/* The constants below control the 'debounce' periods enforced by the
** check_hook routines; these routines are called once every 4 interrupts
** (the interrupt cycles around the four modules), so the periods are
** specified in _4 millisecond_ increments
*/
#define RING_DEBOUNCE           16     /* Ringer Debounce (64 ms) */
#define DEFAULT_BATT_DEBOUNCE   16     /* Battery debounce (64 ms) */
#define POLARITY_DEBOUNCE       16     /* Polarity debounce (64 ms) */
#define DEFAULT_BATT_THRESH     3      /* Anything under this is "no battery" */

#define OHT_TIMER		6000	/* How long after RING to retain OHT */

#define FXO_LINK_DEBOUNCE	400 /* 200 */

#define MAX_ALARMS		10

/* Interrupt flag enable */
#if 0
# define WAN_REMORA_FXS_LCIP
#endif
#if 0
# define WAN_REMORA_FXS_DTMF
#endif


/* flags bits */
#define WP_TDMV_REGISTER	1	/*0x01*/
#define WP_TDMV_RUNNING		2	/*0x02*/
#define WP_TDMV_UP		3	/*0x04*/

#define IS_TDMV_RUNNING(wr)	wan_test_bit(WP_TDMV_RUNNING, &(wr)->flags)
#define IS_TDMV_UP(wr)		wan_test_bit(WP_TDMV_UP, &(wr)->flags)
#define IS_TDMV_UP_RUNNING(wr)	(IS_TDMV_UP(wr) && IS_TDMV_RUNNING(wr))

/*******************************************************************************
**			STRUCTURES AND TYPEDEFS
*******************************************************************************/
typedef struct {
	int	ready;
	int	offhook;
	int	lastpol;
	int	polarity;
	int	polaritydebounce;
	int	battery;
	int	battdebounce;
	int	ringdebounce;
	int	nobatttimer;
	int	wasringing;
	
	int				echotune;	/* echo tune */
	struct wan_rm_echo_coefs	echoregs;	/* echo tune */
} tdmv_fxo_t;

typedef struct {
	int	ready;
	int	lasttxhook;
	int	lasttxhook_update;
	int	lastrxhook;
	int	oldrxhook;
	int	debouncehook;
	int	debounce;
	int	palarms;
	int	ohttimer;
} tdmv_fxs_t;

typedef struct wp_usb_tdmv_remora_ {
	void		*card;
	char		*devname;
	int		num;
	int		flags;
	wan_spinlock_t	lockirq;
	wan_spinlock_t	tx_rx_lockirq;
	union {
		tdmv_fxo_t	fxo;
		tdmv_fxs_t	fxs;
	} mod[MAX_USB_REMORA_MODULES];

	int		spanno;
	struct zt_span	span;
#ifdef DAHDI_ISSUES
#ifdef DAHDI_22
	struct dahdi_echocan_state ec[MAX_REMORA_MODULES]; /* echocan state for each channel */
#endif
	struct zt_chan	*chans_ptrs[MAX_REMORA_MODULES];
#ifdef DAHDI_26
	struct dahdi_device *ddev;
	struct device dev;
#endif

#endif
	struct zt_chan	chans[MAX_USB_REMORA_MODULES];
	unsigned long	reg_module_map;	/* Registered modules */

	unsigned char	reg0shadow[MAX_USB_REMORA_MODULES];	/* read> fxs: 68 fxo: 5 */
	unsigned char	reg1shadow[MAX_USB_REMORA_MODULES];	/* read> fxs: 64 fxo: 29 */
	unsigned char	reg2shadow[MAX_USB_REMORA_MODULES];	/* read> fxs: 64 fxo: 29 */

	unsigned char	reg0shadow_write[MAX_USB_REMORA_MODULES];	/* write> fxs: 68 fxo: 5 */
	int		reg0shadow_update[MAX_USB_REMORA_MODULES];

	/* Global configuration */

	u32		intcount;
	int		pollcount;
	unsigned char	ec_chunk1[31][ZT_CHUNKSIZE];
	unsigned char	ec_chunk2[31][ZT_CHUNKSIZE];
	int		usecount;
	u16		max_timeslots;		/* up to MAX_USB_REMORA_MODULES */
	int		max_rxtx_len;
	int		channelized;
	unsigned char	hwec;
	unsigned long	echo_off_map;
	
	int		battdebounce;		/* global for FXO */
	int		battthresh;		/* global for FXO */

	u_int8_t	tonesupport;
	unsigned int	toneactive;
	unsigned int	tonemask;
	unsigned int	tonemutemask;

} wp_usb_tdmv_remora_t;

/*******************************************************************************
**			   GLOBAL VARIABLES
*******************************************************************************/
static int	wp_remora_no = 0;
extern WAN_LIST_HEAD(, wan_tdmv_) wan_tdmv_head;
//static int battdebounce = DEFAULT_BATT_DEBOUNCE;
//static int battthresh = DEFAULT_BATT_THRESH;

/*******************************************************************************
**			  FUNCTION PROTOTYPES
*******************************************************************************/
static int wp_usb_tdmv_remora_check_mtu(void* pcard, unsigned long timeslot_map, int *mtu);
static int wp_usb_tdmv_remora_create(void* pcard, wan_tdmv_conf_t*);
static int wp_usb_tdmv_remora_remove(void* pcard);
static int wp_usb_tdmv_remora_reg(void* pcard, wan_tdmv_if_conf_t*, unsigned int, unsigned char,netdevice_t*);
static int wp_usb_tdmv_remora_unreg(void* pcard, unsigned long ts_map);
static int wp_usb_tdmv_remora_software_init(wan_tdmv_t *wan_tdmv);
static int wp_usb_tdmv_remora_state(void* pcard, int state);
static int wp_usb_tdmv_remora_running(void* pcard);
static int wp_usb_tdmv_remora_is_rbsbits(wan_tdmv_t *wan_tdmv);
static int wp_usb_tdmv_remora_rx_tx_span(void *pcard);
static int wp_usb_tdmv_remora_rx_chan(wan_tdmv_t*, int,unsigned char*,unsigned char*); 
static int wp_usb_tdmv_remora_ec_span(void *pcard);
static int wp_usb_tdmv_remora_update_regs(void *pcard, int mod_no, u8 *regs);

static void wp_usb_tdmv_remora_tone (void* card_id, wan_event_t *event);

extern int wp_init_proslic(sdla_fe_t *fe, int mod_no, int fast, int sane);
extern int wp_init_voicedaa(sdla_fe_t *fe, int mod_no, int fast, int sane);

static int wp_remora_zap_open(struct zt_chan *chan);
static int wp_remora_zap_close(struct zt_chan *chan);
static int wp_remora_zap_hooksig(struct zt_chan *chan, zt_txsig_t txsig);
static int wp_remora_zap_watchdog(struct zt_span *span, int event);
#if defined(__FreeBSD__) || defined(__OpenBSD__)
static int wp_remora_zap_ioctl(struct zt_chan *chan, unsigned int cmd, caddr_t data);
#else
static int wp_remora_zap_ioctl(struct zt_chan *chan, unsigned int cmd, unsigned long data);
#endif

#ifdef DAHDI_22
static int wp_usb_tdmv_remora_hwec_create(struct dahdi_chan *chan, 
									  struct dahdi_echocanparams *ecp,
									  struct dahdi_echocanparam *p, 
									  struct dahdi_echocan_state **ec);
static void wp_usb_tdmv_remora_hwec_free(struct dahdi_chan *chan, 
									 struct dahdi_echocan_state *ec);
#endif


#if defined(DAHDI_24) || defined(DAHDI_25)

static const struct dahdi_span_ops wp_tdm_span_ops = {
	.owner = THIS_MODULE,
	.open = wp_remora_zap_open,
	.close  = wp_remora_zap_close,
	.ioctl = wp_remora_zap_ioctl,
	.hooksig	= wp_remora_zap_hooksig,
	.watchdog	= wp_remora_zap_watchdog,
#if 0
	/* FIXME: Add native bridging */
	.dacs = ,
#endif
	.echocan_create = wp_usb_tdmv_remora_hwec_create,
};
 
#endif

									 
#ifdef DAHDI_22
/*
*******************************************************************************
**			   DAHDI HWEC STRUCTURES
*******************************************************************************
*/
static const struct dahdi_echocan_features wp_tdmv_remora_ec_features = {
	.NLP_automatic = 1,
	.CED_tx_detect = 1,
	.CED_rx_detect = 1,
};

static const struct dahdi_echocan_ops wp_tdmv_remora_ec_ops = {
#ifndef DAHDI_25
	.name = "WANPIPE_HWEC",
#endif
	.echocan_free = wp_usb_tdmv_remora_hwec_free,
};
#endif

/*******************************************************************************
**			  FUNCTION DEFINITIONS
*******************************************************************************/

static int
#if defined(__FreeBSD__) || defined(__OpenBSD__)
wp_remora_zap_ioctl(struct zt_chan *chan, unsigned int cmd, caddr_t data)
#else
wp_remora_zap_ioctl(struct zt_chan *chan, unsigned int cmd, unsigned long data)
#endif
{
	wp_usb_tdmv_remora_t	*wr = chan->pvt;
	sdla_t			*card = NULL;
	sdla_fe_t		*fe = NULL;
	wan_event_ctrl_t	*event_ctrl = NULL;
	int			x, err;

	WAN_ASSERT(wr->card == NULL);
	card	= wr->card;
	fe	= &card->fe;

	switch (cmd) {
	case ZT_ONHOOKTRANSFER:
		if (fe->rm_param.mod[chan->chanpos - 1].type != MOD_TYPE_FXS) {
			return -EINVAL;
		}
		err = WAN_COPY_FROM_USER(&x, (int*)data, sizeof(int));
		/*err = get_user(x, (int *)data);*/
		if (err) return -EFAULT;
		wr->mod[chan->chanpos - 1].fxs.ohttimer = x << 3;
		if (fe->fe_cfg.cfg.remora.reversepolarity){
			/* OHT mode when idle */
			fe->rm_param.mod[chan->chanpos - 1].u.fxs.idletxhookstate  = 0x6;
		}else{
			fe->rm_param.mod[chan->chanpos - 1].u.fxs.idletxhookstate  = 0x2;
		}
		if (wr->mod[chan->chanpos - 1].fxs.lasttxhook == 0x1) {
			/* Apply the change if appropriate */
			if (fe->fe_cfg.cfg.remora.reversepolarity){
				wr->mod[chan->chanpos - 1].fxs.lasttxhook = 0x6;
			}else{
				wr->mod[chan->chanpos - 1].fxs.lasttxhook = 0x2;
			}
#if defined(REG_WRITE_SHADOW)
			wr->mod[chan->chanpos-1].fxs.lasttxhook_update = 1;
#else
			WRITE_USB_RM_REG(chan->chanpos - 1, 64, wr->mod[chan->chanpos - 1].fxs.lasttxhook);
#endif
		}
		break;

	case ZT_SETPOLARITY:
		err = WAN_COPY_FROM_USER(&x, (int*)data, sizeof(int));
		/*err = get_user(x, (int *)data);*/
		if (err) return -EFAULT;
		if (fe->rm_param.mod[chan->chanpos - 1].type != MOD_TYPE_FXS) {
			return -EINVAL;
		}
		/* Can't change polarity while ringing or when open */
		if ((wr->mod[chan->chanpos - 1].fxs.lasttxhook == 0x04) ||
		    (wr->mod[chan->chanpos - 1 ].fxs.lasttxhook == 0x00)){
			return -EINVAL;
		}

		if ((x && !fe->fe_cfg.cfg.remora.reversepolarity) || (!x && fe->fe_cfg.cfg.remora.reversepolarity)){
			wr->mod[chan->chanpos - 1].fxs.lasttxhook |= 0x04;
		}else{
			wr->mod[chan->chanpos - 1].fxs.lasttxhook &= ~0x04;
		}
#if defined(REG_WRITE_SHADOW)
		wr->mod[chan->chanpos-1].fxs.lasttxhook_update = 1;
#else
		WRITE_USB_RM_REG(chan->chanpos - 1, 64, wr->mod[chan->chanpos - 1].fxs.lasttxhook);
#endif
		break;

	case WAN_RM_SET_ECHOTUNE:
		if (fe->rm_param.mod[chan->chanpos - 1].type == MOD_TYPE_FXO) {
		
			err = WAN_COPY_FROM_USER(
					&wr->mod[chan->chanpos-1].fxo.echoregs,
					(struct wan_rm_echo_coefs*)data,
					sizeof(struct wan_rm_echo_coefs));
			if (err) return -EFAULT;

#if 1
			wr->mod[chan->chanpos-1].fxo.echotune = 1;
#else
			DEBUG_EVENT("%s: Module %d: Setting echo registers: \n",
					wr->devname, chan->chanpos-1);
			/* Set the ACIM register */
			WRITE_USB_RM_REG(chan->chanpos - 1, 30, echoregs.acim);

			/* Set the digital echo canceller registers */
			WRITE_USB_RM_REG(chan->chanpos - 1, 45, echoregs.coef1);
			WRITE_USB_RM_REG(chan->chanpos - 1, 46, echoregs.coef2);
			WRITE_USB_RM_REG(chan->chanpos - 1, 47, echoregs.coef3);
			WRITE_USB_RM_REG(chan->chanpos - 1, 48, echoregs.coef4);
			WRITE_USB_RM_REG(chan->chanpos - 1, 49, echoregs.coef5);
			WRITE_USB_RM_REG(chan->chanpos - 1, 50, echoregs.coef6);
			WRITE_USB_RM_REG(chan->chanpos - 1, 51, echoregs.coef7);
			WRITE_USB_RM_REG(chan->chanpos - 1, 52, echoregs.coef8);

			DEBUG_EVENT("%s: Module %d: Set echo registers successfully\n",
					wr->devname, chan->chanpos-1);
#endif
			break;
		} else {
			return -EINVAL;

		}
		break;

	case ZT_TONEDETECT:

		if (WAN_COPY_FROM_USER(&x, (int*)data, sizeof(int))){
			return -EFAULT;
		}

		if (wr->tonesupport != WANOPT_YES || card->wandev.ec_dev == NULL){
			return -ENOSYS;
		}
		DEBUG_TDMV("[TDMV_RM]: %s: HW Tone Detection %s on channel %d (%s)!\n",
			wr->devname,
			(x & ZT_TONEDETECT_ON) ? "ON" : "OFF", chan->chanpos - 1,
			(x & ZT_TONEDETECT_MUTE) ? "Mute ON" : "Mute OFF");
					
		if (x & ZT_TONEDETECT_ON){
			wr->tonemask |= (1 << (chan->chanpos - 1));
		}else{
			wr->tonemask &= ~(1 << (chan->chanpos - 1));
		}
		if (x & ZT_TONEDETECT_MUTE){
			wr->tonemutemask |= (1 << (chan->chanpos - 1));
		}else{
			wr->tonemutemask &= ~(1 << (chan->chanpos - 1));
		}
		
#if defined(CONFIG_WANPIPE_HWEC)
		event_ctrl = wan_malloc(sizeof(wan_event_ctrl_t));
		if (event_ctrl==NULL){
			DEBUG_EVENT(
			"%s: Failed to allocate memory for event ctrl!\n",
						wr->devname);
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

	default:
		return -ENOTTY;
		break;
	}
	return 0;
}

static int wp_remora_zap_hooksig(struct zt_chan *chan, zt_txsig_t txsig)
{
	wp_usb_tdmv_remora_t	*wr = chan->pvt;
	sdla_t			*card = NULL;
	sdla_fe_t		*fe = NULL;

	WAN_ASSERT(wr->card == NULL);
	card	= wr->card;
	fe	= &card->fe;

	if (fe->rm_param.mod[chan->chanpos - 1].type == MOD_TYPE_FXO) {
		/* XXX Enable hooksig for FXO XXX */
		switch(txsig) {
		case ZT_TXSIG_START:
		case ZT_TXSIG_OFFHOOK:
			DEBUG_TDMV("%s: Module %d: goes off-hook (txsig %d)\n", 
					wr->devname, chan->chanpos, txsig);
			wr->mod[chan->chanpos - 1].fxo.offhook = 1;
#if defined(REG_WRITE_SHADOW)
			wr->reg0shadow[chan->chanpos-1] = 0x09;
			wr->reg0shadow_update[chan->chanpos-1] = 1;
#else
			WRITE_USB_RM_REG(chan->chanpos - 1, 5, 0x9);
#endif
			break;
		case ZT_TXSIG_ONHOOK:
			DEBUG_TDMV("%s: Module %d: goes on-hook (txsig %d)\n", 
					wr->devname, chan->chanpos, txsig);
			wr->mod[chan->chanpos - 1].fxo.offhook = 0;
#if defined(REG_WRITE_SHADOW)
			wr->reg0shadow[chan->chanpos-1] = 0x08;
			wr->reg0shadow_update[chan->chanpos-1] = 1;
#else
			WRITE_USB_RM_REG(chan->chanpos - 1, 5, 0x8);
#endif
			break;
		default:
			DEBUG_TDMV("%s: Can't set tx state to %d (chan %d)\n",
					wr->devname, txsig, chan->chanpos);
		}
	}else if (fe->rm_param.mod[chan->chanpos - 1].type == MOD_TYPE_FXS) {
		switch(txsig) {
		case ZT_TXSIG_ONHOOK:
			DEBUG_TDMV("%s: Module %d: goes on-hook (txsig %d).\n",
					wr->devname, chan->chanpos, txsig);
			switch(chan->sig) {
			case ZT_SIG_EM:
			case ZT_SIG_FXOKS:
			case ZT_SIG_FXOLS:
				wr->mod[chan->chanpos-1].fxs.lasttxhook =
					fe->rm_param.mod[chan->chanpos-1].u.fxs.idletxhookstate;
				break;
			case ZT_SIG_FXOGS:
				wr->mod[chan->chanpos-1].fxs.lasttxhook = 3;
				break;
			}
			break;
		case ZT_TXSIG_OFFHOOK:
			DEBUG_TDMV("%s: Module %d: goes off-hook (txsig %d).\n",
					wr->devname, chan->chanpos, txsig);
			switch(chan->sig) {
			case ZT_SIG_EM:
				wr->mod[chan->chanpos-1].fxs.lasttxhook = 5;
				break;
			default:
				wr->mod[chan->chanpos-1].fxs.lasttxhook =
					fe->rm_param.mod[chan->chanpos-1].u.fxs.idletxhookstate;
				break;
			}
			break;
		case ZT_TXSIG_START:
			DEBUG_TDMV("%s: Module %d: txsig START (txsig %d).\n",
					wr->devname, chan->chanpos, txsig);
			wr->mod[chan->chanpos-1].fxs.lasttxhook = 4;
			break;
		case ZT_TXSIG_KEWL:
			wr->mod[chan->chanpos-1].fxs.lasttxhook = 0;
			break;
		default:
			DEBUG_EVENT("%s: Can't set tx state to %d\n",
					wr->devname, txsig);
			return 0;
			break;
		}
#if defined(REG_WRITE_SHADOW)
		wr->mod[chan->chanpos-1].fxs.lasttxhook_update = 1;
#else
		WRITE_USB_RM_REG(chan->chanpos - 1, 64, wr->mod[chan->chanpos-1].fxs.lasttxhook);
#endif
	}

	return 0;
}


static int wp_remora_zap_open(struct zt_chan *chan)
{
	wp_usb_tdmv_remora_t	*wr = NULL;
	sdla_t		*card = NULL; 

	WAN_ASSERT2(chan == NULL, -ENODEV);
	WAN_ASSERT2(chan->pvt == NULL, -ENODEV);
	wr = chan->pvt;
	WAN_ASSERT2(wr->card == NULL, -ENODEV);
    card = wr->card; 
	wr->usecount++;
	wan_set_bit(WP_TDMV_RUNNING, &wr->flags);
	wanpipe_open(card);
	DEBUG_EVENT("%s: Open (usecount=%d, channo=%d, chanpos=%d)...\n", 
				wr->devname,
				wr->usecount,
				chan->channo,
				chan->chanpos);
	return 0;
}

static int wp_remora_zap_close(struct zt_chan *chan)
{
	sdla_t		*card = NULL;	
	wp_usb_tdmv_remora_t*	wr = NULL;
	sdla_fe_t	*fe = NULL;

	WAN_ASSERT2(chan == NULL, -ENODEV);
	WAN_ASSERT2(chan->pvt == NULL, -ENODEV);
	wr	= chan->pvt;
	card	= wr->card;
	fe	= &card->fe;
	wr->usecount--;
	wanpipe_close(card);   
	wan_clear_bit(WP_TDMV_RUNNING, &wr->flags);

#if 1
	if (fe->rm_param.mod[chan->chanpos - 1].type == MOD_TYPE_FXS) {
		if (fe->fe_cfg.cfg.remora.reversepolarity)
			fe->rm_param.mod[chan->chanpos - 1].u.fxs.idletxhookstate = 5;
		else
			fe->rm_param.mod[chan->chanpos - 1].u.fxs.idletxhookstate = 1;
	}
#endif
	return 0;
}

static int wp_remora_zap_watchdog(struct zt_span *span, int event)
{
#if 0
	printk("TDM: Restarting DMA\n");
	wctdm_restart_dma(span->pvt);
#endif
	return 0;
}





#ifdef DAHDI_22

/******************************************************************************
** wp_remora_zap_hwec() - 
**
**	OK
*/
static int wp_usb_tdmv_remora_hwec_create(struct dahdi_chan *chan, 
									  struct dahdi_echocanparams *ecp,
									  struct dahdi_echocanparam *p, 
									  struct dahdi_echocan_state **ec)
{
	wp_usb_tdmv_remora_t *wr = NULL;
	sdla_t *card = NULL;
	int err = -ENODEV;
	
	WAN_ASSERT2(chan == NULL, -ENODEV);
	WAN_ASSERT2(chan->pvt == NULL, -ENODEV);
	wr = chan->pvt;
	WAN_ASSERT2(wr->card == NULL, -ENODEV);
	card = wr->card;

	
	if (ecp->param_count > 0) {
		DEBUG_TDMV("[TDMV] Wanpipe echo canceller does not support parameters; failing request\n");
		return -EINVAL;
	}

	*ec = &wr->ec[chan->chanpos-1];
	(*ec)->ops = &wp_tdmv_remora_ec_ops;
	(*ec)->features = wp_tdmv_remora_ec_features;

	wan_set_bit(chan->chanpos-1, &card->wandev.rtp_tap_call_map);
	
	
	if (card->wandev.ec_enable){
		/* The ec persist flag enables and disables
		* persistent echo control.  In persist mode
		* echo cancellation is enabled regardless of
		* asterisk.  In persist mode off asterisk 
		* controls hardware echo cancellation */		 
		if (card->hwec_conf.persist_disable) {
			err = card->wandev.ec_enable(card, 1, chan->chanpos);
		} else {
			err = 0;			
		}           
		DEBUG_TDMV("[TDMV_RM]: %s: %s HW echo canceller on channel %d\n",        
				   wr->devname,
				   (enable) ? "Enable" : "Disable",
				   chan->chanpos);
	}
	return err;
}


/******************************************************************************
** wp_tdmv_remora_hwec_free() - 
**
**	OK
*/
static void wp_usb_tdmv_remora_hwec_free(struct dahdi_chan *chan, struct dahdi_echocan_state *ec)
{
	wp_usb_tdmv_remora_t *wr = NULL;
	sdla_t *card = NULL;

	memset(ec, 0, sizeof(*ec));

	if(chan == NULL) return;
	if(chan->pvt == NULL) return;
	wr = chan->pvt;
	if(wr->card == NULL) return;
	card = wr->card;

	wan_clear_bit(chan->chanpos-1, &card->wandev.rtp_tap_call_map);

	if (card->wandev.ec_enable) {
		/* The ec persist flag enables and disables
		* persistent echo control.  In persist mode
		* echo cancellation is enabled regardless of
		* asterisk.  In persist mode off asterisk 
		* controls hardware echo cancellation */
		if (card->hwec_conf.persist_disable) {
			card->wandev.ec_enable(card, 0, chan->chanpos);
		}
		DEBUG_TDMV("[TDMV] %s: Disable HW echo canceller on channel %d\n",
				   wr->devname, chan->chanpos);
	}
}

#else
/******************************************************************************
** wp_remora_zap_hwec() - 
**
**	OK
*/
static int wp_remora_zap_hwec(struct zt_chan *chan, int enable)
{
	wp_usb_tdmv_remora_t	*wr = NULL;
	sdla_t			*card = NULL;
	int			err = -ENODEV;
	
	WAN_ASSERT2(chan == NULL, -ENODEV);
	WAN_ASSERT2(chan->pvt == NULL, -ENODEV);
	wr = chan->pvt;
	WAN_ASSERT2(wr->card == NULL, -ENODEV);
	card = wr->card;

	if (enable) {
		wan_set_bit(chan->chanpos-1, &card->wandev.rtp_tap_call_map);
	} else {
		wan_clear_bit(chan->chanpos-1, &card->wandev.rtp_tap_call_map);
	}

	if (card->wandev.ec_enable){
		/* The ec persist flag enables and disables
	         * persistent echo control.  In persist mode
                 * echo cancellation is enabled regardless of
                 * asterisk.  In persist mode off asterisk 
                 * controls hardware echo cancellation */		 
		if (card->hwec_conf.persist_disable) {
			err = card->wandev.ec_enable(card, enable, chan->chanpos);
		} else {
			err = 0;			
		}           
		DEBUG_TDMV("[TDMV_RM]: %s: %s HW echo canceller on channel %d\n",        
				wr->devname,
				(enable) ? "Enable" : "Disable",
				chan->chanpos);
	}
	return err;
}
#endif

static void wp_usb_tdmv_remora_proslic_recheck_sanity(wp_usb_tdmv_remora_t *wr, int mod_no)
{
	sdla_t		*card = NULL;	
	sdla_fe_t	*fe = NULL;
	int res;

	WAN_ASSERT1(wr->card == NULL);
	card	= wr->card;
	fe	= &card->fe;

	/* Check loopback */
#if 0
#if defined(REG_SHADOW)
	res = wr->reg2shadow[mod_no];
#else
        res = READ_USB_RM_REG(mod_no, 8);
#endif
        if (res) {
                DEBUG_EVENT(
		"%s: Module %d: Ouch, part reset, quickly restoring reality (%02X) -- Comment out\n",
				wr->devname, mod_no, res);
                wp_init_proslic(fe, mod_no, 1, 1);
		return;
        }
#endif

#if defined(REG_SHADOW)
	res = wr->reg1shadow[mod_no];
#else
	res = READ_USB_RM_REG(mod_no, 64);
#endif
	if (!res && (res != wr->mod[mod_no].fxs.lasttxhook)) {
#if defined(REG_SHADOW)
		res = wr->reg2shadow[mod_no];
#else
		res = READ_USB_RM_REG(mod_no, 8);
#endif
		if (res) {
			DEBUG_EVENT(
			"%s: Module %d: Ouch, part reset, quickly restoring reality\n",
					wr->devname, mod_no+1);
			wp_init_proslic(fe, mod_no, 1, 1);
		} else {
			if (wr->mod[mod_no].fxs.palarms++ < MAX_ALARMS) {
				DEBUG_EVENT(
				"%s: Module %d: Power alarm, resetting!\n",
					wr->devname, mod_no + 1);
				if (wr->mod[mod_no].fxs.lasttxhook == 4)
					wr->mod[mod_no].fxs.lasttxhook = 1;
				WRITE_USB_RM_REG(mod_no, 64, wr->mod[mod_no].fxs.lasttxhook);
			} else {
				if (wr->mod[mod_no].fxs.palarms == MAX_ALARMS)
					DEBUG_EVENT(
					"%s: Module %d: Too many power alarms, NOT resetting!\n",
						wr->devname, mod_no + 1);
			}
		}
	}
	return;
}

static void
wp_usb_tdmv_remora_voicedaa_recheck_sanity(wp_usb_tdmv_remora_t *wr, int mod_no)
{
	sdla_t		*card = NULL;	
	sdla_fe_t	*fe = NULL;
	int res;

	WAN_ASSERT1(wr->card == NULL);
	card	= wr->card;
	fe	= &card->fe;

	/* Check loopback */
#if defined(REG_SHADOW)
	res = wr->reg2shadow[mod_no];
#else
        res = READ_USB_RM_REG(mod_no, 34);
#endif
        if (!res) {
		DEBUG_EVENT(
		"%s: Module %d: Ouch, part reset, quickly restoring reality\n",
					wr->devname, mod_no+1);
		wp_init_voicedaa(fe, mod_no, 1, 1);
	}
	return;	
}

static void wp_usb_tdmv_remora_voicedaa_check_hook(wp_usb_tdmv_remora_t *wr, int mod_no)
{
	sdla_t		*card = NULL;	
	sdla_fe_t	*fe = NULL;
#ifndef AUDIO_RINGCHECK
	unsigned char res;
#endif	
	signed char b;
	int poopy = 0;

	WAN_ASSERT1(wr->card == NULL);
	card	= wr->card;
	fe	= &card->fe;

	/* Try to track issues that plague slot one FXO's */
#if defined(REG_SHADOW)
	b = wr->reg0shadow[mod_no];
#else
	b = READ_USB_RM_REG(mod_no, 5);
#endif
	if ((b & 0x2) || !(b & 0x8)) {
		/* Not good -- don't look at anything else */
		DEBUG_TDMV("%s: Module %d: Poopy (Reg:5 = %02X)!\n",
				wr->devname, mod_no + 1, (unsigned char)b); 
		poopy++;
	}
	b &= 0x9b;
	if (wr->mod[mod_no].fxo.offhook) {
		if (b != 0x9){
			DEBUG_TDMV("%s: Module %d: Correct Reg:5 <- 0x09 (%02X:offhook=%d:%d)!\n",
					wr->devname, mod_no + 1,
					(unsigned char)b, wr->mod[mod_no].fxo.offhook,
					(u32)SYSTEM_TICKS); 
			if (!wr->reg0shadow_update[mod_no]){
				wr->reg0shadow[mod_no] = 0x09;
				wr->reg0shadow_update[mod_no] = 1;
			}
//			WRITE_USB_RM_REG(mod_no, 5, 0x9);
		}
	} else {
		if (b != 0x8){
			DEBUG_TDMV("%s: Module %d: Correct Reg:5 <- 0x08 (%02X:offhook=%d:%d)!\n",
					wr->devname, mod_no + 1,
					(unsigned char)b, wr->mod[mod_no].fxo.offhook,
					(u32)SYSTEM_TICKS); 
			if (!wr->reg0shadow_update[mod_no]){
				wr->reg0shadow[mod_no] = 0x08;
				wr->reg0shadow_update[mod_no] = 1;
			}
//			WRITE_USB_RM_REG(mod_no, 5, 0x8);
		}
	}
	if (poopy)
		return;
#ifndef AUDIO_RINGCHECK
	if (!wr->mod[mod_no].fxo.offhook) {
#if defined(REG_SHADOW)
		res = wr->reg0shadow[mod_no];
#else
		res = READ_USB_RM_REG(mod_no, 5);
#endif
		if ((res & 0x60) && wr->mod[mod_no].fxo.battery) {
			wr->mod[mod_no].fxo.ringdebounce += (ZT_CHUNKSIZE * 16);
			if (wr->mod[mod_no].fxo.ringdebounce >= ZT_CHUNKSIZE * 64) {
				if (!wr->mod[mod_no].fxo.wasringing) {
					wr->mod[mod_no].fxo.wasringing = 1;
					zt_hooksig(&wr->chans[mod_no], ZT_RXSIG_RING);
					DEBUG_TDMV("%s: Module %d: RING on span %d (%X)!\n",
							wr->devname,
							mod_no + 1,
							wr->span.spanno, res);
				}
				wr->mod[mod_no].fxo.ringdebounce = ZT_CHUNKSIZE * 64;
			}
		} else {
			wr->mod[mod_no].fxo.ringdebounce -= ZT_CHUNKSIZE * 4;
			if (wr->mod[mod_no].fxo.ringdebounce <= 0) {
				if (wr->mod[mod_no].fxo.wasringing) {
					wr->mod[mod_no].fxo.wasringing = 0;
					zt_hooksig(&wr->chans[mod_no], ZT_RXSIG_OFFHOOK);
					DEBUG_TDMV("%s: Module %d: NO RING on span %d (off-hook) (%X)!\n",
							wr->devname,
							mod_no + 1,
							wr->span.spanno, res);
				}
				wr->mod[mod_no].fxo.ringdebounce = 0;
			}
		}
	}
#endif

#if defined(REG_SHADOW)
	b = wr->reg1shadow[mod_no];
#else
	b = READ_USB_RM_REG(mod_no, 29);
#endif
#if 0
	{
		static int count = 1;
		if (mod_no == 0) count++;
		if ((count % 100) == 0) {
			DEBUG_EVENT("ADBG> %s: Module %d: Voltage: %d  Debounce %d\n", 
					wr->devname, mod_no + 1, 
			       		b, wr->mod[mod_no].fxo.battdebounce);
		}
	}
#endif	

	if (abs(b) <= 1){
		fe->rm_param.mod[mod_no].u.fxo.statusdebounce ++;
		if (fe->rm_param.mod[mod_no].u.fxo.statusdebounce >= FXO_LINK_DEBOUNCE){
			if (fe->rm_param.mod[mod_no].u.fxo.status != FE_DISCONNECTED){
				DEBUG_EVENT(
				"%s: Module %d: FXO Line is disconnnected!\n",
								wr->devname,
								mod_no + 1);
				fe->rm_param.mod[mod_no].u.fxo.status = FE_DISCONNECTED;
			}
			fe->rm_param.mod[mod_no].u.fxo.statusdebounce = FXO_LINK_DEBOUNCE;
		}
	}else{
		fe->rm_param.mod[mod_no].u.fxo.statusdebounce--;
		if (fe->rm_param.mod[mod_no].u.fxo.statusdebounce <= 0) {
			if (fe->rm_param.mod[mod_no].u.fxo.status != FE_CONNECTED){
				DEBUG_EVENT(
				"%s: Module %d: FXO Line is connected!\n",
								wr->devname,
								mod_no + 1);
				fe->rm_param.mod[mod_no].u.fxo.status = FE_CONNECTED;
			}
			fe->rm_param.mod[mod_no].u.fxo.statusdebounce = 0;
		}
	}

	if (abs(b) < wr->battthresh) {
		wr->mod[mod_no].fxo.nobatttimer++;
#if 0
		if (wr->mod[mod_no].fxo.battery)
			DEBUG_EVENT("%s: Module %d: Battery loss: %d (%d debounce)\n", 
				wr->devname, mod_no+1,
				b, wr->mod[mod_no].fxo.battdebounce);
#endif
		if (wr->mod[mod_no].fxo.battery && !wr->mod[mod_no].fxo.battdebounce) {
			DEBUG_TDMV(
			"%s: Module %d: NO BATTERY on span %d (%02X)!\n",
						wr->devname,
						mod_no + 1,
						wr->span.spanno, (unsigned char)b);
			wr->mod[mod_no].fxo.battery =  0;
#ifdef	JAPAN
			if ((!wr->mod[mod_no].fxo.ohdebounce) &&
		            wr->mod[mod_no].fxo.offhook) {
				zt_hooksig(&wr->chans[mod_no], ZT_RXSIG_ONHOOK);
				DEBUG_TDMV(
				"%s: Module %d: Signalled On Hook span %d\n",
							wr->devname,
							mod_no + 1,
							wr->span.spanno);
#ifdef	ZERO_BATT_RING
				wr->mod[mod_no].fxo.onhook++;
#endif
			}
#else
			DEBUG_TDMV(
			"%s: Module %d: Signalled On Hook span %d (%02X)\n",
							wr->devname,
							mod_no + 1,
							wr->span.spanno, (unsigned char)b);
			zt_hooksig(&wr->chans[mod_no], ZT_RXSIG_ONHOOK);
#endif
			wr->mod[mod_no].fxo.battdebounce = wr->battdebounce;
		} else if (!wr->mod[mod_no].fxo.battery)
			wr->mod[mod_no].fxo.battdebounce = wr->battdebounce;
	} else if (abs(b) > wr->battthresh) {
		if (!wr->mod[mod_no].fxo.battery && !wr->mod[mod_no].fxo.battdebounce) {
			DEBUG_TDMV(
			"%s: Module %d: BATTERY on span %d (%s) (%02X)!\n",
						wr->devname,
						mod_no + 1,
						wr->span.spanno,
						(b < 0) ? "-" : "+", (unsigned char)b);
#ifdef	ZERO_BATT_RING
			if (wr->mod[mod_no].fxo.onhook) {
				wr->mod[mod_no].fxo.onhook = 0;
				zt_hooksig(&wr->chans[mod_no], ZT_RXSIG_OFFHOOK);
				DEBUG_TDMV(
				"%s: Module %d: Signalled Off Hook span %d\n",
							wr->devname,
							mod_no + 1,
							wr->span.spanno);
			}
#else
			zt_hooksig(&wr->chans[mod_no], ZT_RXSIG_OFFHOOK);
			DEBUG_TDMV(
			"%s: Module %d: Signalled Off Hook span %d (%02X)\n",
							wr->devname,
							mod_no + 1,
							wr->span.spanno, (unsigned char)b);
#endif
			wr->mod[mod_no].fxo.battery = 1;
			wr->mod[mod_no].fxo.nobatttimer = 0;
			wr->mod[mod_no].fxo.battdebounce = wr->battdebounce;
		} else if (wr->mod[mod_no].fxo.battery)
			wr->mod[mod_no].fxo.battdebounce = wr->battdebounce;

		if (wr->mod[mod_no].fxo.lastpol >= 0) {
			if (b < 0) {
				wr->mod[mod_no].fxo.lastpol = -1;
				wr->mod[mod_no].fxo.polaritydebounce = POLARITY_DEBOUNCE;
			}
		} 
		if (wr->mod[mod_no].fxo.lastpol <= 0) {
			if (b > 0) {
				wr->mod[mod_no].fxo.lastpol = 1;
				wr->mod[mod_no].fxo.polaritydebounce = POLARITY_DEBOUNCE;
			}
		}
	} else {
		/* It's something else... */
		wr->mod[mod_no].fxo.battdebounce = wr->battdebounce;
	}

	if (wr->mod[mod_no].fxo.battdebounce)
		wr->mod[mod_no].fxo.battdebounce--;
	if (wr->mod[mod_no].fxo.polaritydebounce) {
	        wr->mod[mod_no].fxo.polaritydebounce--;
		if (wr->mod[mod_no].fxo.polaritydebounce < 1) {
			if (wr->mod[mod_no].fxo.lastpol != wr->mod[mod_no].fxo.polarity) {
				DEBUG_TDMV(
				"%s: Module %d: Polarity reversed %d -> %d (%u)\n",
						wr->devname,
						mod_no + 1,
						wr->mod[mod_no].fxo.polarity, 
						wr->mod[mod_no].fxo.lastpol,
						(u32)SYSTEM_TICKS);
				if (wr->mod[mod_no].fxo.polarity){
					zt_qevent_lock(&wr->chans[mod_no],
							ZT_EVENT_POLARITY);
				}
				wr->mod[mod_no].fxo.polarity =
						wr->mod[mod_no].fxo.lastpol;
			}
		}
	}
	return;
}

static void wp_usb_tdmv_remora_proslic_check_hook(wp_usb_tdmv_remora_t *wr, int mod_no)
{
	sdla_t		*card = NULL;	
	sdla_fe_t	*fe = NULL;
	int		hook;
	char		res;

	WAN_ASSERT1(wr->card == NULL);
	card	= wr->card;
	fe	= &card->fe;
	/* For some reason we have to debounce the
	   hook detector.  */

#if defined(REG_SHADOW)
	res = wr->reg0shadow[mod_no];
#else
	res = READ_USB_RM_REG(mod_no, 68);
#endif
	hook = (res & 1);
	if (hook != wr->mod[mod_no].fxs.lastrxhook) {
		/* Reset the debounce (must be multiple of 4ms) */
		wr->mod[mod_no].fxs.debounce = 4 * (4 * 8);
		DEBUG_TDMV(
		"%s: Module %d: Resetting debounce hook %d, %d\n",
				wr->devname, mod_no + 1, hook,
				wr->mod[mod_no].fxs.debounce);
	} else {
		if (wr->mod[mod_no].fxs.debounce > 0) {
			wr->mod[mod_no].fxs.debounce-= 16 * ZT_CHUNKSIZE;
			DEBUG_TDMV(
			"%s: Module %d: Sustaining hook %d, %d\n",
					wr->devname, mod_no + 1,
					hook, wr->mod[mod_no].fxs.debounce);
			if (!wr->mod[mod_no].fxs.debounce) {
				DEBUG_TDMV(
				"%s: Module %d: Counted down debounce, newhook: %d\n",
							wr->devname,
							mod_no + 1,
							hook);
				wr->mod[mod_no].fxs.debouncehook = hook;
			}
			if (!wr->mod[mod_no].fxs.oldrxhook && wr->mod[mod_no].fxs.debouncehook) {
				/* Off hook */
				DEBUG_TDMV(
				"%s: Module %d: Going off hook\n",
							wr->devname, mod_no + 1);
				zt_hooksig(&wr->chans[mod_no], ZT_RXSIG_OFFHOOK);
#if 0
				if (robust)
					wp_init_proslic(wc, card, 1, 0, 1);
#endif
				wr->mod[mod_no].fxs.oldrxhook = 1;
			
			} else if (wr->mod[mod_no].fxs.oldrxhook && !wr->mod[mod_no].fxs.debouncehook) {
				/* On hook */
				DEBUG_TDMV(
				"%s: Module %d: Going on hook\n",
							wr->devname, mod_no + 1);
				zt_hooksig(&wr->chans[mod_no], ZT_RXSIG_ONHOOK);
				wr->mod[mod_no].fxs.oldrxhook = 0;
			}
		}
	}
	wr->mod[mod_no].fxs.lastrxhook = hook;
}

static int wp_usb_tdmv_remora_check_hook(sdla_fe_t *fe, int mod_no)
{
	sdla_t			*card = fe->card;
	wan_tdmv_t		*wan_tdmv = NULL;
	wp_usb_tdmv_remora_t	*wr = NULL;

	wan_tdmv = &card->wan_tdmv;
	WAN_ASSERT(wan_tdmv->sc == NULL);
	wr	= wan_tdmv->sc;

	if (fe->rm_param.mod[mod_no].type == MOD_TYPE_FXS) {
		wp_usb_tdmv_remora_proslic_check_hook(wr, mod_no);
	} else if (fe->rm_param.mod[mod_no].type == MOD_TYPE_FXO) {
		wp_usb_tdmv_remora_voicedaa_check_hook(wr, mod_no);
	}
	
	return 0;
}

static int wp_usb_tdmv_remora_hook(sdla_fe_t *fe, int mod_no, int off_hook)
{
	sdla_t			*card = fe->card;
	wan_tdmv_t		*wan_tdmv = NULL;
	wp_usb_tdmv_remora_t	*wr = NULL;

	wan_tdmv = &card->wan_tdmv;
	WAN_ASSERT(wan_tdmv->sc == NULL);
	wr	= wan_tdmv->sc;

	if (off_hook){
		zt_hooksig(&wr->chans[mod_no], ZT_RXSIG_OFFHOOK);
	}else{
		zt_hooksig(&wr->chans[mod_no], ZT_RXSIG_ONHOOK);
	}
	wr->mod[mod_no].fxs.lastrxhook = off_hook;
	return 0;
}


/******************************************************************************
** wp_usb_rm_tdmv_init() - 
**
**	OK
*/
int wp_usb_rm_tdmv_init(wan_tdmv_iface_t *iface)
{
	WAN_ASSERT(iface == NULL);

	memset(iface, 0, sizeof(wan_tdmv_iface_t));
	iface->check_mtu	= wp_usb_tdmv_remora_check_mtu;
	iface->create		= wp_usb_tdmv_remora_create;
	iface->remove		= wp_usb_tdmv_remora_remove;
	iface->reg		= wp_usb_tdmv_remora_reg;
	iface->unreg		= wp_usb_tdmv_remora_unreg;
	iface->software_init	= wp_usb_tdmv_remora_software_init;
	iface->state		= wp_usb_tdmv_remora_state;
	iface->running		= wp_usb_tdmv_remora_running;
	iface->is_rbsbits	= wp_usb_tdmv_remora_is_rbsbits;
	iface->rx_tx_span	= wp_usb_tdmv_remora_rx_tx_span;
	iface->rx_chan		= wp_usb_tdmv_remora_rx_chan;
	iface->ec_span		= wp_usb_tdmv_remora_ec_span;  
	iface->update_regs	= wp_usb_tdmv_remora_update_regs;
	return 0;
}

static int wp_usb_tdmv_remora_software_init(wan_tdmv_t *wan_tdmv)
{
	sdla_t			*card = NULL;
	sdla_fe_t		*fe = NULL;
	wp_usb_tdmv_remora_t	*wr = wan_tdmv->sc;
	int			x = 0, num = 0;

	WAN_ASSERT(wr == NULL);
	WAN_ASSERT(wr->card == NULL);
	card = wr->card;
	fe = &card->fe;

	if (wan_test_bit(WP_TDMV_REGISTER, &wr->flags)){
		DEBUG_EVENT(
		"%s: Wanpipe device is already registered to Zaptel span # %d!\n",
					wr->devname, wr->span.spanno);
		return 0;
	}
	/* Zapata stuff */
	sprintf(wr->span.name, "WRTDM/%d", wr->num);
	sprintf(wr->span.desc, "wrtdm Board %d", wr->num + 1);
	switch(fe->fe_cfg.tdmv_law){
	case WAN_TDMV_ALAW:
		DEBUG_EVENT(
		"%s: ALAW override parameter detected. Device will be operating in ALAW\n",
					wr->devname);
		wr->span.deflaw = ZT_LAW_ALAW;
		break;
	case WAN_TDMV_MULAW:
		wr->span.deflaw = ZT_LAW_MULAW;
		break;
	}
	
	wr->tonesupport = card->u.aft.tdmv_hw_tone;
	wr->battthresh	= DEFAULT_BATT_THRESH;
	wr->battdebounce= DEFAULT_BATT_DEBOUNCE;
	if (fe->fe_cfg.cfg.remora.battthresh && 
	    fe->fe_cfg.cfg.remora.battthresh != DEFAULT_BATT_THRESH){
		wr->battthresh = fe->fe_cfg.cfg.remora.battthresh;
		DEBUG_EVENT("%s: A200/A400 Remora Battery Threshhold changed %d -> %d\n", 
					wr->devname, DEFAULT_BATT_THRESH, wr->battthresh);
	}
	if (fe->fe_cfg.cfg.remora.battdebounce && 
            fe->fe_cfg.cfg.remora.battdebounce != DEFAULT_BATT_DEBOUNCE){
		wr->battdebounce = fe->fe_cfg.cfg.remora.battdebounce;
		DEBUG_EVENT("%s: A200/A400 Remora Battery Debounce changed %d -> %d\n", 
					wr->devname, DEFAULT_BATT_DEBOUNCE, wr->battdebounce);
	}
	
	for (x = 0; x < MAX_USB_REMORA_MODULES; x++) {
		if (wan_test_bit(x, &fe->rm_param.module_map)){

			sprintf(wr->chans[x].name, "WRTDM/%d/%d", wr->num, x);
			DEBUG_EVENT("%s: Configure Module %d for voice (%s, type %s)!\n", 
					wr->devname, 
					x + 1,
					wr->chans[x].name,
					WP_REMORA_DECODE_TYPE(fe->rm_param.mod[x].type));
			if (fe->rm_param.mod[x].type == MOD_TYPE_FXO){
				wr->chans[x].sigcap =	ZT_SIG_FXSKS |
							ZT_SIG_FXSLS |
							ZT_SIG_SF |
							ZT_SIG_CLEAR;
			}else if (fe->rm_param.mod[x].type == MOD_TYPE_FXS){
				wr->chans[x].sigcap =	ZT_SIG_FXOKS |
							ZT_SIG_FXOLS |
							ZT_SIG_FXOGS |
							ZT_SIG_SF |
							ZT_SIG_EM |
							ZT_SIG_CLEAR;
			}
			wr->chans[x].chanpos = x+1;
			wr->chans[x].pvt = wr;
			num++;
		}else{

			sprintf(wr->chans[x].name, "WRTDM/%d/%d", wr->num, x);
			DEBUG_TEST("%s: Not used module %d!\n", 
					wr->devname, 
					x + 1);
			wr->chans[x].sigcap = ZT_SIG_CLEAR;
			wr->chans[x].chanpos = x+1;
			wr->chans[x].pvt = wr;
			num++;
		}
	}

#if defined(DAHDI_24) || defined(DAHDI_25)
	wr->span.ops = &wp_tdm_span_ops;
#else

	wr->span.pvt = wr;

#if defined(DAHDI_23)
	wr->span.owner          = THIS_MODULE;
#endif

	wr->span.hooksig	= wp_remora_zap_hooksig;
	wr->span.open		= wp_remora_zap_open;
	wr->span.close		= wp_remora_zap_close;
	wr->span.ioctl		= wp_remora_zap_ioctl;
	wr->span.watchdog	= wp_remora_zap_watchdog;
	
	/* Set this pointer only if card has hw echo canceller module */
	if (wr->hwec == WANOPT_YES && card->wandev.ec_dev){
#ifdef DAHDI_22
		wr->span.echocan_create = wp_usb_tdmv_remora_hwec_create;
#else
		wr->span.echocan = wp_remora_zap_hwec;
#endif
	}

#endif


#ifdef DAHDI_ISSUES
	wr->span.chans		= wr->chans_ptrs;
#else
	wr->span.chans		= wr->chans;
#endif

	wr->span.flags		= ZT_FLAG_RBS;
	wr->span.channels	= num/*wr->max_timeslots*/;


#if defined(__LINUX__)
#ifndef DAHDI_25
	init_waitqueue_head(&wr->span.maintq);
#endif
#endif


	if (wp_dahdi_register_device(wr)) {
		DEBUG_EVENT("%s: Unable to register span with zaptel\n",
				wr->devname);
		return -EINVAL;
	}

	if (wr->span.spanno != wr->spanno +1){
		DEBUG_EVENT("\n");
		DEBUG_EVENT("WARNING: Span number %d is already used by another device!\n",
						wr->spanno + 1);
		DEBUG_EVENT("         Possible cause: Another TDM driver already loaded!\n");
		DEBUG_EVENT("         Solution:       Unload wanpipe and check currently\n");
		DEBUG_EVENT("                         used spans in /proc/zaptel directory.\n");
		DEBUG_EVENT("         Reconfiguring device %s to new span number # %d\n",
						wr->devname,wr->span.spanno);
		DEBUG_EVENT("\n");
		wr->spanno = wr->span.spanno-1;
	}else{
		DEBUG_EVENT("%s: Wanpipe device is registered to Zaptel span # %d!\n", 
					wr->devname, wr->span.spanno);
	}
	wp_usb_tdmv_remora_check_mtu(card, wr->reg_module_map, &wr->max_rxtx_len);
	wan_set_bit(WP_TDMV_REGISTER, &wr->flags);

	/* Initialize Callback event function pointers */	
	if (wr->tonesupport == WANOPT_YES){
		DEBUG_EVENT("%s: Enable HW Tone detection!\n", wr->devname);
		card->wandev.event_callback.tone = wp_usb_tdmv_remora_tone;
	}
	if (fe->fe_cfg.cfg.remora.fxs_pulsedialing == WANOPT_YES){
		DEBUG_EVENT("%s: Enable Pulse Dialing mode\n", 
					wr->devname);
	}
	return 0;
}

/******************************************************************************
** wp_usb_tdmv_release() - 
**
**	OK
*/
static void wp_usb_tdmv_release(wp_usb_tdmv_remora_t *wr)
{
	WAN_ASSERT1(wr == NULL);
	if (wan_test_bit(WP_TDMV_REGISTER, &wr->flags)){
		DEBUG_EVENT("%s: Unregister WAN FXS/FXO device from Zaptel!\n",
				wr->devname);
		wan_clear_bit(WP_TDMV_REGISTER, &wr->flags);
		wp_dahdi_unregister_device(wr);
		wan_clear_bit(WP_TDMV_REGISTER, &wr->flags);
	}
	wp_dahdi_free_device(wr);
	wan_free(wr);
	return;
}


static wp_usb_tdmv_remora_t *wan_remora_search(sdla_t * card)
{
	return NULL;
}

/******************************************************************************
** wp_usb_tdmv_remora_check_mtu() - 
**
**	OK
*/
static int wp_usb_tdmv_remora_check_mtu(void* pcard, unsigned long timeslot_map, int *mtu)
{
	sdla_t	*card = (sdla_t*)pcard;
	int	x, num_of_channels = 0, max_channels;

	max_channels = WAN_FE_MAX_CHANNELS(&card->fe);
	for (x = 0; x < max_channels; x++) {
		if (wan_test_bit(x,&timeslot_map)){
			num_of_channels++;
		}
	}
	*mtu = ZT_CHUNKSIZE * num_of_channels;
	return 0;
}

/******************************************************************************
** wp_usb_tdmv_remora_create() - 
*tdmv_*
**	OK
*/
static int wp_usb_tdmv_remora_create(void* pcard, wan_tdmv_conf_t *tdmv_conf)
{
	sdla_t			*card = (sdla_t*)pcard;
	wp_usb_tdmv_remora_t	*wr = NULL;
	wan_tdmv_t		*tmp = NULL;
#ifdef DAHDI_ISSUES
	int i;
#endif	

	WAN_ASSERT(card == NULL);
	WAN_ASSERT(tdmv_conf->span_no == 0);
	wr = wan_remora_search(card);
	if (wr){
		DEBUG_EVENT("%s: AFT remora FXO/FXS card already configured!\n",
					card->devname);
		return -EINVAL;
	}
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

	memset(&card->wan_tdmv, 0x0, sizeof(wan_tdmv_t));
	card->wan_tdmv.max_timeslots			= card->fe.rm_param.max_fe_channels;
	card->wan_tdmv.spanno				= tdmv_conf->span_no;
	card->wandev.fe_notify_iface.hook_state		= wp_usb_tdmv_remora_hook;
	card->wandev.fe_notify_iface.check_hook_state	= wp_usb_tdmv_remora_check_hook;

	wr = wan_malloc(sizeof(wp_usb_tdmv_remora_t));
	if (wr == NULL){
		return -ENOMEM;
	}
	memset(wr, 0x0, sizeof(wp_usb_tdmv_remora_t));
	card->wan_tdmv.sc	= wr;
	wr->spanno		= tdmv_conf->span_no-1;
	wr->num			= wp_remora_no++;
	wr->card		= card;
	wr->devname		= card->devname;
	wr->max_timeslots	= card->fe.rm_param.max_fe_channels;
	wr->max_rxtx_len	= 0;
	wan_spin_lock_irq_init(&wr->lockirq, "wan_rmtdmv_lock");
	wan_spin_lock_irq_init(&wr->tx_rx_lockirq, "wan_rmtdmv_txrx_lock");

    if (wp_dahdi_create_device(card,wr)) {
       wan_free(wr);
       return -ENOMEM;
    }


#ifdef DAHDI_ISSUES

    WP_DAHDI_SET_STR_INFO(wr,manufacturer,"Sangoma Technologies");
    WP_DAHDI_SET_STR_INFO(wr,devicetype, "U100");
    WP_DAHDI_SET_STR_INFO(wr,location,"SLOT=%d, BUS=%d", card->wandev.S514_slot_no, card->wandev.S514_bus_no);

	for (i = 0; i < sizeof(wr->chans)/sizeof(wr->chans[0]); i++) {
		wr->chans_ptrs[i] = &wr->chans[i];
	}
#endif

	if (tmp){
		WAN_LIST_INSERT_AFTER(tmp, &card->wan_tdmv, next);
	}else{
		WAN_LIST_INSERT_HEAD(&wan_tdmv_head, &card->wan_tdmv, next);
	}
	return 0;
}


/******************************************************************************
** wp_usb_tdmv_reg() - 
**
** Returns: 	0-31	- Return TDM Voice channel number.
**		-EINVAL - otherwise
**	OK
*/
static int wp_usb_tdmv_remora_reg(	void			*pcard,
				wan_tdmv_if_conf_t	*tdmv_conf, 
				unsigned int 		active_ch, 
				unsigned char		ec_enable,
				netdevice_t		*dev)
{
	sdla_t			*card = (sdla_t*)pcard;
	sdla_fe_t		*fe = &card->fe;
	wan_tdmv_t		*wan_tdmv = &card->wan_tdmv;
	wp_usb_tdmv_remora_t	*wr = NULL;
	int			i, channo = 0;

	WAN_ASSERT(wan_tdmv->sc == NULL);
	wr = wan_tdmv->sc;
		
	if (wan_test_bit(WP_TDMV_REGISTER, &wr->flags)){
		DEBUG_EVENT(
		"%s: Error: Master device has already been configured!\n",
				card->devname);
		return -EINVAL;
	}
		
	for(i = 0; i < wr->max_timeslots; i++){
		if (wan_test_bit(i, &active_ch) &&
		    wan_test_bit(i, &fe->rm_param.module_map)){
			if (tdmv_conf->tdmv_echo_off){
				wan_set_bit(i, &wr->echo_off_map);
			}
			channo = i;
			break;
		}
	}	

	if (i == wr->max_timeslots){
		DEBUG_EVENT(
		"%s: Error: TDMV iface %s failed to configure for %08X timeslots!\n",
					card->devname,
					wan_netif_name(dev),
					active_ch);
		return -EINVAL;
	}

   	DEBUG_EVENT(
	"%s: Registering TDMV %s iface to module %d!\n",
			card->devname,
			WP_REMORA_DECODE_TYPE(fe->rm_param.mod[channo].type),
			channo+1);
	wan_set_bit(channo, &wr->reg_module_map);

	if (tdmv_conf->tdmv_echo_off){
		DEBUG_EVENT("%s:    TDMV Echo Ctrl:Off\n",
				wr->devname);
	}
	memset(wr->chans[channo].sreadchunk, WAN_TDMV_IDLE_FLAG, ZT_CHUNKSIZE);
	memset(wr->chans[channo].swritechunk, WAN_TDMV_IDLE_FLAG, ZT_CHUNKSIZE);
	wr->chans[channo].readchunk = wr->chans[channo].sreadchunk;
	wr->chans[channo].writechunk = wr->chans[channo].swritechunk;
	wr->channelized = WAN_TRUE;
	wr->hwec = ec_enable;
	wp_usb_tdmv_remora_check_mtu(card, active_ch, &wr->max_rxtx_len);
	return channo;
}


/******************************************************************************
** wp_usb_tdmv_unreg() - 
**
**	OK
*/
static int wp_usb_tdmv_remora_unreg(void* pcard, unsigned long ts_map)
{
	sdla_t			*card = (sdla_t*)pcard;
	sdla_fe_t		*fe = &card->fe;
	wan_tdmv_t		*wan_tdmv = &card->wan_tdmv;
	wp_usb_tdmv_remora_t	*wr = NULL;
	int			channo = 0;

	WAN_ASSERT(wan_tdmv->sc == NULL);
	wr = wan_tdmv->sc;

	for(channo = 0; channo < wr->max_timeslots; channo++){
		if (wan_test_bit(channo, &wr->reg_module_map)){
			DEBUG_EVENT(
			"%s: Unregistering TDMV %s iface from module %d!\n",
				card->devname,
				WP_REMORA_DECODE_TYPE(fe->rm_param.mod[channo].type),
				channo+1);
			wan_clear_bit(channo, &wr->reg_module_map);
			wan_clear_bit(channo, &wr->echo_off_map);
			memset(wr->chans[channo].sreadchunk, 
					WAN_TDMV_IDLE_FLAG, 
					ZT_CHUNKSIZE);
			memset(wr->chans[channo].swritechunk, 
					WAN_TDMV_IDLE_FLAG, 
					ZT_CHUNKSIZE);
			wr->chans[channo].readchunk = 
					wr->chans[channo].sreadchunk;
			wr->chans[channo].writechunk = 
					wr->chans[channo].swritechunk;
		}
	}
	return 0;
}


/******************************************************************************
** wp_usb_tdmv_remove() - 
**
**	OK
*/
static int wp_usb_tdmv_remora_remove(void* pcard)
{
	sdla_t			*card = (sdla_t*)pcard;
	wan_tdmv_t		*wan_tdmv = &card->wan_tdmv;
	wp_usb_tdmv_remora_t	*wr = NULL;

	if (!card->wan_tdmv.sc){
		return 0;
	}
	
	wr = wan_tdmv->sc;
	/* Release span, possibly delayed */
	if (wr && wr->reg_module_map){
		DEBUG_EVENT(
		"%s: Some interfaces are not unregistered (%08lX)!\n",
				card->devname,
				wr->reg_module_map);
		return -EINVAL;
	}
	if (wr && wr->usecount){
		DEBUG_ERROR("%s: ERROR: Wanpipe is still used by Asterisk!\n",
				card->devname);
		return -EINVAL;
	}

	if (wr){
		wan_clear_bit(WP_TDMV_RUNNING, &wr->flags);
		wan_clear_bit(WP_TDMV_UP, &wr->flags);
		wan_tdmv->sc = NULL;
		WAN_LIST_REMOVE(wan_tdmv, next);
		wp_usb_tdmv_release(wr);
	}else{
		wan_tdmv->sc = NULL;
	}
	return 0;
}

static int wp_usb_tdmv_remora_state(void* pcard, int state)
{
	sdla_t			*card = (sdla_t*)pcard;
	wan_tdmv_t		*wan_tdmv = &card->wan_tdmv;
	wp_usb_tdmv_remora_t	*wr = NULL;

	WAN_ASSERT(wan_tdmv->sc == NULL);
	wr = (wp_usb_tdmv_remora_t*)wan_tdmv->sc;
	
	switch(state){
	case WAN_CONNECTED:
		DEBUG_TDMV("%s: TDMV Remora state is CONNECTED!\n",
					wr->devname);
		wan_set_bit(WP_TDMV_UP, &wr->flags);
		break;

	case WAN_DISCONNECTED:
		DEBUG_TDMV("%s: TDMV Remora state is DISCONNECTED!\n",
					wr->devname);
		wan_clear_bit(WP_TDMV_UP, &wr->flags);
		break;
	}
	return 0;
}

/******************************************************************************
** wp_usb_tdmv_running() - 
**
**	OK
*/
static int wp_usb_tdmv_remora_running(void* pcard)
{
	sdla_t			*card = (sdla_t*)pcard;
	wan_tdmv_t		*wan_tdmv = &card->wan_tdmv;
	wp_usb_tdmv_remora_t	*wr = NULL;

	wr = wan_tdmv->sc;
	if (wr && wr->usecount){
		DEBUG_EVENT("%s: WARNING: Wanpipe is still used by Asterisk!\n",
				card->devname);
		return -EINVAL;
	}
	return 0;
}

/******************************************************************************
** wp_usb_tdmv_remora_is_rbsbits() - 
**
**	OK
*/
static int wp_usb_tdmv_remora_is_rbsbits(wan_tdmv_t *wan_tdmv)
{
	return 0;
}

/******************************************************************************
** wp_usb_tdmv_rx_chan() - 
**
**	OK
*/
static int wp_usb_tdmv_remora_rx_chan(wan_tdmv_t *wan_tdmv, int channo, 
			unsigned char *rxbuf,
			unsigned char *txbuf)
{
	wp_usb_tdmv_remora_t	*wr = wan_tdmv->sc;
#ifdef CONFIG_PRODUCT_WANPIPE_TDM_VOICE_ECHOMASTER
	wan_tdmv_rxtx_pwr_t	*pwr_rxtx = NULL;
#endif
	sdla_t *card;

	WAN_ASSERT2(wr == NULL, -EINVAL);
	WAN_ASSERT2(channo < 0, -EINVAL);
	WAN_ASSERT2(channo > 31, -EINVAL);

	if (!IS_TDMV_UP(wr)){
		return -EINVAL;
	}
	card = wr->card;

#ifdef CONFIG_PRODUCT_WANPIPE_TDM_VOICE_ECHOMASTER
	pwr_rxtx = &wan_tdmv->chan_pwr[channo];
#endif

	wr->chans[channo].readchunk = rxbuf;	
	wr->chans[channo].writechunk = txbuf;	

#ifdef CONFIG_PRODUCT_WANPIPE_TDM_VOICE_ECHOMASTER
	wp_usb_tdmv_echo_check(wan_tdmv, &wr->chans[channo], channo);
#endif		

	if ((!card->wandev.ec_enable || card->wandev.ec_enable_map == 0) && 
	     !wan_test_bit(channo, &wr->echo_off_map)) {

/*Echo spike starts at 25bytes*/
#ifdef CONFIG_PRODUCT_WANPIPE_TDM_VOICE_ECHOMASTER
		if(pwr_rxtx->current_state != ECHO_ABSENT){
#endif

		if (wan_test_bit(AFT_TDM_SW_RING_BUF,&card->u.aft.chip_cfg_status)) {
			/* Updated for SWRING buffer 
 		 	 * Sets up the spike at 3 bytes */
			zt_ec_chunk(
				&wr->chans[channo], 
				wr->chans[channo].readchunk, 
				wr->chans[channo].writechunk);
		} else {
			/* This should be used without SWRING Echo spike starts at 9 bytes*/
			zt_ec_chunk(
				&wr->chans[channo], 
				wr->chans[channo].readchunk, 
				wr->ec_chunk1[channo]);
			memcpy(
				wr->ec_chunk1[channo],
				wr->chans[channo].writechunk,
				ZT_CHUNKSIZE);
		}

#if 0			
/*Echo spike starts at bytes*/
			zt_ec_chunk(
				&wr->chans[channo], 
				wr->chans[channo].readchunk, 
				wr->ec_chunk1[channo]);
			memcpy(
				wr->ec_chunk1[channo],
				wr->ec_chunk2[channo],
				ZT_CHUNKSIZE);

			memcpy(
				wr->ec_chunk2[channo],
				wr->chans[channo].writechunk,
				ZT_CHUNKSIZE);
#endif

#ifdef CONFIG_PRODUCT_WANPIPE_TDM_VOICE_ECHOMASTER
		} /*if(pwr_rxtx->current_state != ECHO_ABSENT) */
#endif
	} /* if (!wan_test_bit(channo, &wr->echo_off_map)) */

	return 0;
}

static int wp_usb_tdmv_remora_update_regs(void *pcard, int mod_no, u8 *regs)
{
	sdla_t			*card = (sdla_t*)pcard;
	sdla_fe_t		*fe = &card->fe;
	wan_tdmv_t		*wan_tdmv = &card->wan_tdmv;
        wp_usb_tdmv_remora_t	*wr = NULL;
	
	WAN_ASSERT(wan_tdmv->sc == NULL);
	wr = wan_tdmv->sc;

	if (fe->rm_param.mod[mod_no].type == MOD_TYPE_FXS) {
		/* Read first shadow reg */
		wr->reg0shadow[mod_no] = regs[0];	// 68
		/* Read second shadow reg */	
		wr->reg1shadow[mod_no] = regs[1];	// 64
		/* Read third shadow reg */
		wr->reg2shadow[mod_no] = regs[2];	// 8
	}else if (fe->rm_param.mod[mod_no].type == MOD_TYPE_FXO) {
		/* Read first shadow reg */
		if (!wr->reg0shadow_update[mod_no]) wr->reg0shadow[mod_no] = regs[0];	//5
		/* Read second shadow reg */
		wr->reg1shadow[mod_no] = regs[1];	//29
		/* Read third shadow reg */
		wr->reg2shadow[mod_no] = regs[2];	//34
	}
	return 0;
}

static int wp_usb_tdmv_remora_rx_tx_span(void *pcard)
{
	sdla_t			*card = (sdla_t*)pcard;
	sdla_fe_t		*fe = &card->fe;
	wan_tdmv_t		*wan_tdmv = &card->wan_tdmv;
        wp_usb_tdmv_remora_t	*wr = NULL;
	u_int16_t		x;
	
	WAN_ASSERT(wan_tdmv->sc == NULL);
	wr = wan_tdmv->sc;

	wr->intcount++;
	for (x = 0; x < wr->max_timeslots; x++) {
		if (!wan_test_bit(x, &wr->reg_module_map)){
			continue;
		}
		if (fe->rm_param.mod[x].type == MOD_TYPE_FXS){
#if defined(REG_WRITE_SHADOW)
			if (wr->mod[x].fxs.lasttxhook_update){
				WRITE_USB_RM_REG(x, 64, wr->mod[x].fxs.lasttxhook);
				wr->mod[x].fxs.lasttxhook_update = 0;
				continue;
			}
#endif

			if (wr->mod[x].fxs.lasttxhook == 0x4) {
				/* RINGing, prepare for OHT */
				wr->mod[x].fxs.ohttimer = OHT_TIMER << 3;
				if (fe->fe_cfg.cfg.remora.reversepolarity){
					/* OHT mode when idle */
					fe->rm_param.mod[x].u.fxs.idletxhookstate = 0x6;
				}else{
					fe->rm_param.mod[x].u.fxs.idletxhookstate = 0x2; 
				}
			} else {
				if (wr->mod[x].fxs.ohttimer) {
					wr->mod[x].fxs.ohttimer-= ZT_CHUNKSIZE;
					if (!wr->mod[x].fxs.ohttimer) {
						if (fe->fe_cfg.cfg.remora.reversepolarity){
							/* Switch to active */
							fe->rm_param.mod[x].u.fxs.idletxhookstate = 0x5;
						}else{
							fe->rm_param.mod[x].u.fxs.idletxhookstate = 0x1;
						}
						if ((wr->mod[x].fxs.lasttxhook == 0x2) || (wr->mod[x].fxs.lasttxhook == 0x6)) {
							/* Apply the change if appropriate */
							if (fe->fe_cfg.cfg.remora.reversepolarity){ 
								wr->mod[x].fxs.lasttxhook = 0x5;
							}else{
								wr->mod[x].fxs.lasttxhook = 0x1;
							}
							WRITE_USB_RM_REG(x, 64, wr->mod[x].fxs.lasttxhook);
						}
					}
				}
			}

		} else if (fe->rm_param.mod[x].type == MOD_TYPE_FXO) {

			if (wr->mod[x].fxo.echotune){
				DEBUG_RM("%s: Module %d: Setting echo registers: \n",
							fe->name, x);

				/* Set the ACIM register */
				WRITE_USB_RM_REG(x, 30, wr->mod[x].fxo.echoregs.acim);

				/* Set the digital echo canceller registers */
				WRITE_USB_RM_REG(x, 45, wr->mod[x].fxo.echoregs.coef1);
				WRITE_USB_RM_REG(x, 46, wr->mod[x].fxo.echoregs.coef2);
				WRITE_USB_RM_REG(x, 47, wr->mod[x].fxo.echoregs.coef3);
				WRITE_USB_RM_REG(x, 48, wr->mod[x].fxo.echoregs.coef4);
				WRITE_USB_RM_REG(x, 49, wr->mod[x].fxo.echoregs.coef5);
				WRITE_USB_RM_REG(x, 50, wr->mod[x].fxo.echoregs.coef6);
				WRITE_USB_RM_REG(x, 51, wr->mod[x].fxo.echoregs.coef7);
				WRITE_USB_RM_REG(x, 52, wr->mod[x].fxo.echoregs.coef8);

				DEBUG_RM("%s: Module %d: Set echo registers successfully\n",
						fe->name, x+1);
				wr->mod[x].fxo.echotune = 0;
			}
#if defined(REG_WRITE_SHADOW)
			if (wr->reg0shadow_update[x]){
				/* Read first shadow reg */
				DEBUG_TDMV("%s: Module %d: Update Reg:5 <= %02X (%d)!\n",
						wr->devname, x+1,
						(unsigned char)wr->reg0shadow[x], 
						(u32)SYSTEM_TICKS);
				WRITE_USB_RM_REG(x, 5, wr->reg0shadow[x]);
				wr->reg0shadow_update[x] = 0;
			}
#endif
		}

		if (fe->fe_cfg.cfg.remora.fxs_pulsedialing == WANOPT_YES){
			/*
			** Alex 31 Mar, 2006
			** Check for HOOK status every interrupt
			** (in pulse mode is very critical) */
			wp_usb_tdmv_remora_check_hook(fe, x);
		}
	}

	x = wr->intcount % MAX_USB_REMORA_MODULES;
	if (wan_test_bit(x, &wr->reg_module_map)) {

		if (fe->fe_cfg.cfg.remora.fxs_pulsedialing != WANOPT_YES){
			wp_usb_tdmv_remora_check_hook(fe, x);
		}
		if (!(wr->intcount & 0xf0)){
			if (fe->rm_param.mod[x].type == MOD_TYPE_FXS) {
				wp_usb_tdmv_remora_proslic_recheck_sanity(wr, x);
			}else if (fe->rm_param.mod[x].type == MOD_TYPE_FXO) {
				wp_usb_tdmv_remora_voicedaa_recheck_sanity(wr, x);
			}
		}
	}

	if (!(wr->intcount % 10000)) {
		/* Accept an alarm once per 10 seconds */
		for (x = 0; x < wr->max_timeslots; x++) 
			if (wan_test_bit(x, &wr->reg_module_map) &&
			    (fe->rm_param.mod[x].type == MOD_TYPE_FXS)) {
				if (wr->mod[x].fxs.palarms){
					wr->mod[x].fxs.palarms--;
				}
			}
	}

	zt_receive(&wr->span);
	zt_transmit(&wr->span);
	return 0;
}

static int wp_usb_tdmv_remora_ec_span(void *pcard)
{
	sdla_t          *card = (sdla_t*)pcard;
        wan_tdmv_t      *wan_tdmv = &card->wan_tdmv;
        wp_usb_tdmv_remora_t	*wr = NULL;

        WAN_ASSERT(wan_tdmv->sc == NULL);
        wr = wan_tdmv->sc;

	zt_ec_span(&wr->span);

	return 0;

}

static void wp_usb_tdmv_remora_tone (void* card_id, wan_event_t *event)
{
	sdla_t	*card = (sdla_t*)card_id;
	wan_tdmv_t      *wan_tdmv = &card->wan_tdmv;
	wp_usb_tdmv_remora_t	*wr = NULL;

	WAN_ASSERT1(wan_tdmv->sc == NULL);
	wr = wan_tdmv->sc;
	
	if (event->type == WAN_EVENT_EC_DTMF){
		DEBUG_TDMV(
		"[TDMV_RM]: %s: Received EC DTMF Event at TDM (%d:%c:%s:%s)!\n",
			card->devname,
			event->channel,
			event->digit,
			(event->tone_port == WAN_EC_CHANNEL_PORT_ROUT)?"ROUT":"SOUT",
			(event->tone_type == WAN_EC_TONE_PRESENT)?"PRESENT":"STOP");
	}else if (event->type == WAN_EVENT_RM_DTMF){
		DEBUG_TDMV(
		"[TDMV_RM]: %s: Received RM DTMF Event at TDM (%d:%c)!\n",
			card->devname,
			event->channel,
			event->digit);	
	}
					
	if (!(wr->tonemask & (1 << (event->channel-1)))){
		DEBUG_TDMV(
		"[TDMV] %s: Tone detection is not enabled for the channel %d\n",
					card->devname,
					event->channel);
		return;
	}

	if (event->tone_type == WAN_EC_TONE_PRESENT){
		wr->toneactive |= (1 << (event->channel-1));
		zt_qevent_lock(
				&wr->chans[event->channel-1],
				(ZT_EVENT_DTMFDOWN | event->digit));
	}else{
		wr->toneactive &= ~(1 << (event->channel-1));
		zt_qevent_lock(
				&wr->chans[event->channel-1],
				(ZT_EVENT_DTMFUP | event->digit));
	}
	return;
}

#endif   /* #if defined(CONFIG_PRODUCT_WANPIPE_USB)        */
