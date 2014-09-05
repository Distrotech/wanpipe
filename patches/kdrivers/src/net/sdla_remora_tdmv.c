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
 * Sep 06, 2008	Moises Silva    DAHDI support.
 ******************************************************************************
 */

/*******************************************************************************
**			   INCLUDE FILES
*******************************************************************************/
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
# include "sdla_remora_analog.h"
# include "sdla_remora_tdmv.h"

#if !defined (__WINDOWS__)
# include "zapcompat.h" /* Map of Zaptel -> DAHDI definitions */
# include "wanpipe_dahdi_abstr.h" /* Map of Zaptel -> DAHDI definitions */
#endif

#if 0
#ifdef DEBUG_TEST
#undef DEBUG_TEST
#define DEBUG_TEST DEBUG_EVENT
#endif
#endif

/*******************************************************************************
**			  DEFINES AND MACROS
*******************************************************************************/
#if 0
# define	SPI2STEP
#endif
/* The constants below control the 'debounce' periods enforced by the
** check_hook routines; these routines are called once every 4 interrupts
** (the interrupt cycles around the four modules), so the periods are
** specified in _4 millisecond_ increments
*/
#define RING_DEBOUNCE           16     /* Ringer Debounce (64 ms) */

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

#define wp_fax_tone_timeout_set(wr,chan) do { DEBUG_TEST("%s:%d: s%dc%d fax timeout set\n", \
											__FUNCTION__,__LINE__, \
											wr->spanno+1,chan+1); \
											wr->ec_fax_detect_timeout[chan]=SYSTEM_TICKS; } while(0);
											
/*******************************************************************************
**			   GLOBAL VARIABLES
*******************************************************************************/
static int	wp_remora_no = 0;
extern WAN_LIST_HEAD(, wan_tdmv_) wan_tdmv_head;


/*******************************************************************************
**			  FUNCTION PROTOTYPES
*******************************************************************************/
static int wp_tdmv_remora_check_mtu(void* pcard, unsigned long timeslot_map, int *mtu);
static int wp_tdmv_remora_create(void* pcard, wan_tdmv_conf_t*);
static int wp_tdmv_remora_remove(void* pcard);
static int wp_tdmv_remora_reg(void* pcard, wan_tdmv_if_conf_t*, unsigned int, unsigned char,netdevice_t*);
static int wp_tdmv_remora_unreg(void* pcard, unsigned long ts_map);
static int wp_tdmv_remora_software_init(wan_tdmv_t *wan_tdmv);
static int wp_tdmv_remora_state(void* pcard, int state);
static int wp_tdmv_remora_running(void* pcard);
static int wp_tdmv_remora_is_rbsbits(wan_tdmv_t *wan_tdmv);
static int wp_tdmv_remora_rx_tx_span(void *pcard);
static int wp_tdmv_remora_rx_chan(wan_tdmv_t*, int,unsigned char*,unsigned char*); 
static int wp_tdmv_remora_ec_span(void *pcard);
#ifdef DAHDI_25
static int wp_remora_chanconfig(struct file *file, struct zt_chan *chan, int sigtype);
#else
static int wp_remora_chanconfig(struct zt_chan *chan, int sigtype);
#endif
static int wp_remora_zap_open(struct zt_chan *chan);
static int wp_remora_zap_close(struct zt_chan *chan);
static int wp_remora_zap_hooksig(struct zt_chan *chan, zt_txsig_t txsig);
static int wp_remora_zap_watchdog(struct zt_span *span, int event);
		
#if defined(__FreeBSD__) || defined(__OpenBSD__)
static int wp_remora_zap_ioctl(struct zt_chan *chan, unsigned int cmd, caddr_t data);
#else
static int wp_remora_zap_ioctl(struct zt_chan *chan, unsigned int cmd, unsigned long data);
#endif

static void wp_tdmv_remora_tone (void* card_id, wan_event_t *event);
#ifdef DAHDI_22
static int wp_tdmv_remora_hwec_create(struct dahdi_chan *chan, 
									  struct dahdi_echocanparams *ecp,
									  struct dahdi_echocanparam *p, 
									  struct dahdi_echocan_state **ec);
static void wp_tdmv_remora_hwec_free(struct dahdi_chan *chan, 
									 struct dahdi_echocan_state *ec);
#endif

#ifdef DAHDI_25
static const char *wp_tdmv_remora_hwec_name(const struct zt_chan *chan);
#endif

#if 0
#define WAN_SYNC_RX_TX_TEST 1
#warning "WAN_SYNC_RX_TX_TEST: Test option Enabled" 
static int wp_tdmv_remora_rx_chan_sync_test(sdla_t *card, wp_tdmv_remora_t *wr, int channo, 
					    unsigned char *rxbuf,
					    unsigned char *txbuf);
#else
#undef WAN_SYNC_RX_TX_TEST
#endif

	
#if defined(DAHDI_24) || defined(DAHDI_25)

static const struct dahdi_span_ops wp_tdm_span_ops = {
	.owner = THIS_MODULE,
	.chanconfig = wp_remora_chanconfig,
	.open = wp_remora_zap_open,
	.close  = wp_remora_zap_close,
	.ioctl = wp_remora_zap_ioctl,
	.hooksig	= wp_remora_zap_hooksig,
	.watchdog	= wp_remora_zap_watchdog,
#if 0
	/* FIXME: Add native bridging */
	.dacs = ,
#endif
	.echocan_create = wp_tdmv_remora_hwec_create,
#ifdef DAHDI_25
	.echocan_name = wp_tdmv_remora_hwec_name,
#endif
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
	/* This option is used on dahdi 2.4 or lower */
	.name = "WANPIPE_HWEC",
#endif
	.echocan_free = wp_tdmv_remora_hwec_free,
};

#endif

/*******************************************************************************
**			  FUNCTION DEFINITIONS
*******************************************************************************/


#if defined(__FreeBSD__) || defined(__OpenBSD__)
static int wp_remora_zap_ioctl(struct zt_chan *chan, unsigned int cmd, caddr_t data)
#else
static int wp_remora_zap_ioctl(struct zt_chan *chan, unsigned int cmd, unsigned long data)
#endif
{
	wp_tdmv_remora_t	*wr;
	sdla_t			*card = NULL;
	sdla_fe_t		*fe = NULL;
	wan_event_ctrl_t	*event_ctrl = NULL;
	int			x, err;

	wr = WP_PRIV_FROM_CHAN(chan,wp_tdmv_remora_t);
	WAN_ASSERT2(wr == NULL, -ENODEV);
	
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
			WRITE_RM_REG(chan->chanpos - 1, 64, wr->mod[chan->chanpos - 1].fxs.lasttxhook);
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
		WRITE_RM_REG(chan->chanpos - 1, 64, wr->mod[chan->chanpos - 1].fxs.lasttxhook);
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
			WRITE_RM_REG(chan->chanpos - 1, 30, echoregs.acim);

			/* Set the digital echo canceller registers */
			WRITE_RM_REG(chan->chanpos - 1, 45, echoregs.coef1);
			WRITE_RM_REG(chan->chanpos - 1, 46, echoregs.coef2);
			WRITE_RM_REG(chan->chanpos - 1, 47, echoregs.coef3);
			WRITE_RM_REG(chan->chanpos - 1, 48, echoregs.coef4);
			WRITE_RM_REG(chan->chanpos - 1, 49, echoregs.coef5);
			WRITE_RM_REG(chan->chanpos - 1, 50, echoregs.coef6);
			WRITE_RM_REG(chan->chanpos - 1, 51, echoregs.coef7);
			WRITE_RM_REG(chan->chanpos - 1, 52, echoregs.coef8);

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
	wp_tdmv_remora_t	*wr;
	sdla_t			*card = NULL;
	sdla_fe_t		*fe = NULL;

	wr = WP_PRIV_FROM_CHAN(chan,wp_tdmv_remora_t);
	WAN_ASSERT2(wr == NULL, -ENODEV);
	WAN_ASSERT(wr->card == NULL);
	card	= wr->card;
	fe	= &card->fe;

	if (fe->rm_param.mod[chan->chanpos - 1].type == MOD_TYPE_FXO) {
		/* XXX Enable hooksig for FXO XXX */
		switch(txsig) {
		case ZT_TXSIG_OFFHOOK:
				wp_fax_tone_timeout_set(wr,(chan->chanpos-1));
				/* Drop down */
				
		case ZT_TXSIG_START:
			DEBUG_TDMV("%s: Module %d: goes off-hook (txsig %d)\n", 
					wr->devname, chan->chanpos, txsig);
			wr->mod[chan->chanpos - 1].fxo.offhook = 1;
#if defined(REG_WRITE_SHADOW)
			 fe->rm_param.reg0shadow[chan->chanpos-1] = 0x09;
			 fe->rm_param.reg0shadow_update[chan->chanpos-1] = 1;
#else
			WRITE_RM_REG(chan->chanpos - 1, 5, 0x9);
#endif
			break;
		case ZT_TXSIG_ONHOOK:
			DEBUG_TDMV("%s: Module %d: goes on-hook (txsig %d)\n", 
					wr->devname, chan->chanpos, txsig);
			wr->mod[chan->chanpos - 1].fxo.offhook = 0;
#if defined(REG_WRITE_SHADOW)
			 fe->rm_param.reg0shadow[chan->chanpos-1] = 0x08;
			 fe->rm_param.reg0shadow_update[chan->chanpos-1] = 1;
#else
			WRITE_RM_REG(chan->chanpos - 1, 5, 0x8);
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
			wp_fax_tone_timeout_set(wr,(chan->chanpos-1));
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
		WRITE_RM_REG(chan->chanpos - 1, 64, wr->mod[chan->chanpos-1].fxs.lasttxhook);
#endif
	}

	return 0;
}

static int wp_remora_zap_open(struct zt_chan *chan)
{
	wp_tdmv_remora_t	*wr = NULL;
	sdla_t		*card = NULL; 

	WAN_ASSERT2(chan == NULL, -ENODEV);
	wr = WP_PRIV_FROM_CHAN(chan,wp_tdmv_remora_t);
	WAN_ASSERT2(wr == NULL, -ENODEV);
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
	wp_tdmv_remora_t*	wr = NULL;
	sdla_fe_t	*fe = NULL;

	WAN_ASSERT2(chan == NULL, -ENODEV);
	wr = WP_PRIV_FROM_CHAN(chan,wp_tdmv_remora_t);
	WAN_ASSERT2(wr == NULL, -ENODEV);
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

#ifdef DAHDI_25
/* This funciton is used for dahdi 2.5 or higher */
static const char *wp_tdmv_remora_hwec_name(const struct zt_chan *chan)
{
	wp_tdmv_remora_t *wr = NULL;
	sdla_t *card = NULL;
	
	WAN_ASSERT2(chan == NULL, NULL);
	wr = WP_PRIV_FROM_CHAN(chan,wp_tdmv_remora_t);
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
** wp_remora_zap_hwec() - 
**
**	OK
*/
static int wp_tdmv_remora_hwec_create(struct dahdi_chan *chan, 
									  struct dahdi_echocanparams *ecp,
									  struct dahdi_echocanparam *p, 
									  struct dahdi_echocan_state **ec)
{
	wp_tdmv_remora_t *wr = NULL;
	sdla_t *card = NULL;
	int err = -ENODEV;
	
	WAN_ASSERT2(chan == NULL, -ENODEV);
	wr = WP_PRIV_FROM_CHAN(chan,wp_tdmv_remora_t);
	WAN_ASSERT2(wr == NULL, -ENODEV);
	WAN_ASSERT2(wr->card == NULL, -ENODEV);
	card = wr->card;

	
	if (ecp->param_count > 0) {
		DEBUG_TDMV("[TDMV] Wanpipe echo canceller does not support parameters; failing request\n");
		return -EINVAL;
	}

	*ec = &wr->ec[chan->chanpos - 1];
	(*ec)->ops = &wp_tdmv_remora_ec_ops;
	(*ec)->features = wp_tdmv_remora_ec_features;
	
	wan_set_bit(chan->chanpos-1,&card->wandev.rtp_tap_call_map);
	wp_fax_tone_timeout_set(wr,(chan->chanpos-1));
	
	if (card->wandev.ec_enable && wr->hwec == WANOPT_YES){
		/* The ec persist flag enables and disables
		* persistent echo control.  In persist mode
		* echo cancellation is enabled regardless of
		* asterisk.  In persist mode off asterisk 
		* controls hardware echo cancellation */		 
		if (card->hwec_conf.persist_disable) {
			/* ec enable expects values starting from 1, zero is not
               allowed, therefore we must use chan->chanpos because it
 			   starts from 1 */
			err = card->wandev.ec_enable(card, 1, chan->chanpos);
		} else {
			err = 0;			
		}           
		DEBUG_TDMV("[TDMV_RM]: %s: %s HW echo canceller on channel %d\n",        
				   wr->devname,
				   /*(enable) ? "Enable" : "Disable",*/
				   (err==0) ? "Enable" : "Disable",
				   chan->chanpos);
	}

	return err;
}


/******************************************************************************
** wp_tdmv_remora_hwec_free() - 
**
**	OK
*/
static void wp_tdmv_remora_hwec_free(struct dahdi_chan *chan, struct dahdi_echocan_state *ec)
{
	wp_tdmv_remora_t *wr = NULL;
	sdla_t *card = NULL;

	memset(ec, 0, sizeof(*ec));

	if(chan == NULL) return;
	
	wr = WP_PRIV_FROM_CHAN(chan,wp_tdmv_remora_t);
	WAN_ASSERT_VOID(wr == NULL);
	
	if(wr->card == NULL) return;
	card = wr->card;

	wan_clear_bit(chan->chanpos-1, &card->wandev.rtp_tap_call_map);

	if (card->wandev.ec_enable && wr->hwec == WANOPT_YES) {
		/* The ec persist flag enables and disables
	         * persistent echo control.  In persist mode
                 * echo cancellation is enabled regardless of
                 * asterisk.  In persist mode off asterisk 
                 * controls hardware echo cancellation */

        if (wr->ec_off_on_fax) {
            DEBUG_EVENT("%s: Re-enabling hwec after fax chan=%i \n",card->devname,chan->chanpos);
            card->wandev.ec_enable(card, 1, chan->chanpos);
            wr->ec_off_on_fax=0;   
		} else {

			if (card->hwec_conf.persist_disable) {
				/* ec enable expects values starting from 1, zero is not
				   allowed, therefore we must use chan->chanpos because it
				   starts from 1 */
				card->wandev.ec_enable(card, 0, chan->chanpos);
			}
			DEBUG_TDMV("[TDMV] %s: Disable HW echo canceller on channel %d\n",
					wr->devname, chan->chanpos);
		}
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
	wp_tdmv_remora_t	*wr = NULL;
	sdla_t			*card = NULL;
	int			err = -ENODEV;
	
	WAN_ASSERT2(chan == NULL, -ENODEV);
	wr = WP_PRIV_FROM_CHAN(chan,wp_tdmv_remora_t);
	WAN_ASSERT2(wr == NULL, -ENODEV);
	WAN_ASSERT2(wr->card == NULL, -ENODEV);
	card = wr->card;


	if (enable) {
		wan_set_bit(chan->chanpos-1,&card->wandev.rtp_tap_call_map);
		wp_fax_tone_timeout_set(wr,(chan->chanpos-1));
	} else {
		wan_clear_bit(chan->chanpos-1,&card->wandev.rtp_tap_call_map);
	}

	if (card->wandev.ec_enable && wr->hwec == WANOPT_YES){
		/* The ec persist flag enables and disables
	         * persistent echo control.  In persist mode
                 * echo cancellation is enabled regardless of
                 * asterisk.  In persist mode off asterisk 
                 * controls hardware echo cancellation */		 


		if (!enable &&  wr->ec_off_on_fax) {
            DEBUG_EVENT("%s: Re-enabling hwec after fax chan=%i \n",card->devname,chan->chanpos);
            card->wandev.ec_enable(card, 1, chan->chanpos);
            wr->ec_off_on_fax=0;   
			err=0;        
		} else {
			if (card->hwec_conf.persist_disable) {
				/* ec enable expects values starting from 1, zero is not
				   allowed, therefore we must use chan->chanpos because it
				   starts from 1 */
				err = card->wandev.ec_enable(card, enable, chan->chanpos);
			} else {
				err = 0;			
			}           
			DEBUG_TDMV("[TDMV_RM]: %s: %s HW echo canceller on channel %d\n",
					wr->devname,
					(enable) ? "Enable" : "Disable",
					chan->chanpos);
		}
	}
	return err;
}
#endif



static int wp_tdmv_remora_hook(sdla_fe_t *fe, int mod_no, int off_hook)
{
	sdla_t			*card = fe->card;
	wan_tdmv_t		*wan_tdmv = NULL;
	wp_tdmv_remora_t	*wr = NULL;

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
** wp_tdmv_remora_init() - 
**
**	OK
*/
int wp_tdmv_remora_init(wan_tdmv_iface_t *iface)
{
	WAN_ASSERT(iface == NULL);

	memset(iface, 0, sizeof(wan_tdmv_iface_t));
	iface->check_mtu	= wp_tdmv_remora_check_mtu;
	iface->create		= wp_tdmv_remora_create;
	iface->remove		= wp_tdmv_remora_remove;
	iface->reg		= wp_tdmv_remora_reg;
	iface->unreg		= wp_tdmv_remora_unreg;
	iface->software_init	= wp_tdmv_remora_software_init;
	iface->state		= wp_tdmv_remora_state;
	iface->running		= wp_tdmv_remora_running;
	iface->is_rbsbits	= wp_tdmv_remora_is_rbsbits;
	iface->rx_tx_span	= wp_tdmv_remora_rx_tx_span;
	iface->rx_chan		= wp_tdmv_remora_rx_chan;
	iface->ec_span		= wp_tdmv_remora_ec_span;  

	return 0;
}

#ifdef DAHDI_25
static int wp_remora_chanconfig(struct file *file, struct zt_chan *chan, int sigtype)
#else
static int wp_remora_chanconfig(struct zt_chan *chan, int sigtype)
#endif
{
	wp_tdmv_remora_t	*wr = NULL;
	sdla_t			*card = NULL;

	WAN_ASSERT2(chan == NULL, -ENODEV);
	wr = WP_PRIV_FROM_CHAN(chan,wp_tdmv_remora_t);
	WAN_ASSERT2(wr == NULL, -ENODEV);
	card = wr->card;

	DEBUG_TDMV("%s: Configuring chan %d SigType %i..\n", wr->devname, chan->chanpos, sigtype);

#ifdef ZT_POLICY_WHEN_FULL
	if (WAN_FE_NETWORK_SYNC(&card->fe)) {
		chan->txbufpolicy = ZT_POLICY_WHEN_FULL;
		chan->txdisable = 1;
	}
#endif

	return 0;		
}

static int wp_tdmv_remora_software_init(wan_tdmv_t *wan_tdmv)
{
	sdla_t			*card = NULL;
	sdla_fe_t		*fe = NULL;
	wp_tdmv_remora_t	*wr = wan_tdmv->sc;
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
	for (x = 0; x < MAX_REMORA_MODULES; x++) {
		if (wan_test_bit(x, &fe->rm_param.module_map)){

			sprintf(wr->chans[x].name, "WRTDM/%d/%d", wr->num, x);
			DEBUG_TDMV("%s: Configure Module %d for voice (%s, type %s)!\n", 
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
	wr->span.chanconfig 	= wp_remora_chanconfig;

	/* Set this pointer only if card has hw echo canceller module */
	if (wr->hwec == WANOPT_YES && card->wandev.ec_dev){
#ifdef DAHDI_22
		wr->span.echocan_create = wp_tdmv_remora_hwec_create;
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
	wp_tdmv_remora_check_mtu(card, wr->reg_module_map, &wr->max_rxtx_len);
	wan_set_bit(WP_TDMV_REGISTER, &wr->flags);

	/* Initialize Callback event function pointers */	
	if (wr->tonesupport == WANOPT_YES){
		DEBUG_EVENT("%s: Enable HW Tone detection!\n", wr->devname);
		card->wandev.event_callback.tone = wp_tdmv_remora_tone;
	}
	if (fe->fe_cfg.cfg.remora.fxs_pulsedialing == WANOPT_YES){
		DEBUG_EVENT("%s: Enable Pulse Dialing mode\n", 
					wr->devname);
	}
	fe->rm_param.wp_rm_chunk_size = ZT_CHUNKSIZE;

	return 0;
}

/******************************************************************************
** wp_tdmv_release() - 
**
**	OK
*/
static void wp_tdmv_release(wp_tdmv_remora_t *wr)
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


static wp_tdmv_remora_t *wan_remora_search(sdla_t * card)
{
	return NULL;
}

/******************************************************************************
** wp_tdmv_remora_check_mtu() - 
**
**	OK
*/
static int wp_tdmv_remora_check_mtu(void* pcard, unsigned long timeslot_map, int *mtu)
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
** wp_tdmv_remora_create() - 
*tdmv_*
**	OK
*/
static int wp_tdmv_remora_create(void* pcard, wan_tdmv_conf_t *tdmv_conf)
{
	sdla_t			*card = (sdla_t*)pcard;
	wp_tdmv_remora_t	*wr = NULL;
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
	card->wandev.fe_notify_iface.hook_state		= wp_tdmv_remora_hook;
	card->wandev.fe_notify_iface.check_hook_state	= wp_tdmv_remora_check_hook;

	wr = wan_malloc(sizeof(wp_tdmv_remora_t));
	if (wr == NULL){
		return -ENOMEM;
	}
	memset(wr, 0x0, sizeof(wp_tdmv_remora_t));
	card->wan_tdmv.sc	= wr;
	wr->spanno		= tdmv_conf->span_no-1;
#ifdef DAHDI_ISSUES
	if (wp_dahdi_create_device(card,wr)) {
       wan_free(wr);
       return -ENOMEM;
	}

	WP_DAHDI_SET_STR_INFO(wr,manufacturer,"Sangoma Technologies");

	switch(card->adptr_type){
	case A200_ADPTR_ANALOG:
		WP_DAHDI_SET_STR_INFO(wr,devicetype, "A200");
		break;
	case A400_ADPTR_ANALOG:
		WP_DAHDI_SET_STR_INFO(wr,devicetype, "A400");
		break;
	case AFT_ADPTR_B800:
		WP_DAHDI_SET_STR_INFO(wr,devicetype, "B800");
		break;
	case AFT_ADPTR_A600:
		WP_DAHDI_SET_STR_INFO(wr,devicetype, "B600");
		break;
	case AFT_ADPTR_B601:
		WP_DAHDI_SET_STR_INFO(wr,devicetype, "B601");
		break;
	case AFT_ADPTR_FLEXBRI:
		WP_DAHDI_SET_STR_INFO(wr,devicetype, "B700");
		break;
	case U100_ADPTR:
		WP_DAHDI_SET_STR_INFO(wr,devicetype, "U100");
		break;
	}
	
	WP_DAHDI_SET_STR_INFO(wr,location,"SLOT=%d, BUS=%d", card->wandev.S514_slot_no, card->wandev.S514_bus_no);

#ifndef DAHDI_26
	wr->span.irq            = card->wandev.irq;
#endif
#endif

	wr->num			= wp_remora_no++;
	wr->card		= card;
	wr->devname		= card->devname;
	wr->max_timeslots	= card->fe.rm_param.max_fe_channels;
	wr->max_rxtx_len	= 0;
	wan_spin_lock_irq_init(&wr->lockirq, "wan_rmtdmv_lock");
	wan_spin_lock_irq_init(&wr->tx_rx_lockirq, "wan_rmtdmv_txrx_lock");
#ifdef DAHDI_ISSUES
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
** wp_tdmv_reg() - 
**
** Returns: 	0-31	- Return TDM Voice channel number.
**		-EINVAL - otherwise
**	OK
*/
static int wp_tdmv_remora_reg(	void			*pcard,
				wan_tdmv_if_conf_t	*tdmv_conf, 
				unsigned int 		active_ch, 
				unsigned char		ec_enable,
				netdevice_t		*dev)
{
	sdla_t			*card = (sdla_t*)pcard;
	sdla_fe_t		*fe = &card->fe;
	wan_tdmv_t		*wan_tdmv = &card->wan_tdmv;
	wp_tdmv_remora_t	*wr = NULL;
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
	wp_tdmv_remora_check_mtu(card, active_ch, &wr->max_rxtx_len);
	return channo;
}


/******************************************************************************
** wp_tdmv_unreg() - 
**
**	OK
*/
static int wp_tdmv_remora_unreg(void* pcard, unsigned long ts_map)
{
	sdla_t			*card = (sdla_t*)pcard;
	sdla_fe_t		*fe = &card->fe;
	wan_tdmv_t		*wan_tdmv = &card->wan_tdmv;
	wp_tdmv_remora_t	*wr = NULL;
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
** wp_tdmv_remove() - 
**
**	OK
*/
static int wp_tdmv_remora_remove(void* pcard)
{
	sdla_t			*card = (sdla_t*)pcard;
	wan_tdmv_t		*wan_tdmv = &card->wan_tdmv;
	wp_tdmv_remora_t	*wr = NULL;

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
		wp_tdmv_release(wr);
	}else{
		wan_tdmv->sc = NULL;
	}
	return 0;
}

static int wp_tdmv_remora_state(void* pcard, int state)
{
	sdla_t			*card = (sdla_t*)pcard;
	wan_tdmv_t		*wan_tdmv = &card->wan_tdmv;
	wp_tdmv_remora_t	*wr = NULL;

	WAN_ASSERT(wan_tdmv->sc == NULL);
	wr = (wp_tdmv_remora_t*)wan_tdmv->sc;
	
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
** wp_tdmv_running() - 
**
**	OK
*/
static int wp_tdmv_remora_running(void* pcard)
{
	sdla_t			*card = (sdla_t*)pcard;
	wan_tdmv_t		*wan_tdmv = &card->wan_tdmv;
	wp_tdmv_remora_t	*wr = NULL;

	wr = wan_tdmv->sc;
	if (wr && wr->usecount){
		DEBUG_EVENT("%s: WARNING: Wanpipe is still used by Asterisk!\n",
				card->devname);
		return -EINVAL;
	}
	return 0;
}

/******************************************************************************
** wp_tdmv_remora_is_rbsbits() - 
**
**	OK
*/
static int wp_tdmv_remora_is_rbsbits(wan_tdmv_t *wan_tdmv)
{
	return 0;
}


static inline void wp_tdmv_dtmfcheck_fakepolarity(wp_tdmv_remora_t *wr, int channo, unsigned char *rxbuf)
{
	sdla_t		*card = wr->card;
	sdla_fe_t	*fe = &card->fe;
	int sample;
	int dtmf=1;

    	/* only look for sound on the line if dtmf flag is on, it is an fxo card and line is onhook */ 
	if (!dtmf || !(fe->rm_param.mod[channo].type == MOD_TYPE_FXO) || wr->mod[channo].fxo.offhook) {
        	return;
	}

   	/* don't look for noise if we're already processing it, or there is a ringing tone */
	if(!wr->mod[channo].fxo.readcid && !wr->mod[channo].fxo.wasringing  &&
		fe->rm_param.intcount > wr->mod[channo].fxo.cidtimer + 400 ) {
		sample = ZT_XLAW((*rxbuf), (&(wr->chans[channo])));
		if (sample > 16000 || sample < -16000) {
			wr->mod[channo].fxo.readcid = 1;
			wr->mod[channo].fxo.cidtimer = fe->rm_param.intcount;
			DEBUG_TEST("DTMF CLIP on %i\n",channo+1);
			zt_qevent_lock(&wr->chans[channo], ZT_EVENT_POLARITY);
		}
	} else if(wr->mod[channo].fxo.readcid && fe->rm_param.intcount > wr->mod[channo].fxo.cidtimer + 2000) {
        /* reset flags if it's been a while */
		wr->mod[channo].fxo.cidtimer = fe->rm_param.intcount;
		wr->mod[channo].fxo.readcid = 0;
	}
}

/******************************************************************************
** wp_tdmv_rx_chan() - 
**
**	OK
*/


static int wp_tdmv_remora_rx_chan(wan_tdmv_t *wan_tdmv, int channo, 
			unsigned char *rxbuf,
			unsigned char *txbuf)
{
	wp_tdmv_remora_t	*wr = wan_tdmv->sc;
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

#if 0
if (channo == 1){ 
DEBUG_EVENT("Module %d: RX: %02X %02X %02X %02X %02X %02X %02X %02X\n",
		channo,
					rxbuf[0],
					rxbuf[1],
					rxbuf[2],
					rxbuf[3],
					rxbuf[4],
					rxbuf[5],
					rxbuf[6],
					rxbuf[7]
					);
}
#endif

#ifdef ZT_POLICY_WHEN_FULL
	/* This feature is used to change zaptel buffering that improves
	   faxing between analog & SMG. Enable this feature ONLY when 
	   network sync is ON */
	if (WAN_FE_NETWORK_SYNC(&card->fe) == WANOPT_YES && 
	    wr->chans[channo].txbufpolicy != ZT_POLICY_WHEN_FULL) {
		DEBUG_EVENT("%s: RX CHAN %i Setting FULL POLICY\n", 
			card->devname,channo);
		wr->chans[channo].txbufpolicy = ZT_POLICY_WHEN_FULL;
	}
#endif

#ifdef WAN_SYNC_RX_TX_TEST
	/* This feature should be used with HWDTMF enabled, otherwise
  	 * analog will not be able to dial. This is a debugging feature
	 * should NEVER be used in production only for testing */
	wp_tdmv_remora_rx_chan_sync_test(card,wr,channo,rxbuf,txbuf);
#endif
 
	wr->chans[channo].readchunk = rxbuf;	
	wr->chans[channo].writechunk = txbuf;	

	if (card->fe.fe_cfg.cfg.remora.fake_polarity == WANOPT_YES){
		wp_tdmv_dtmfcheck_fakepolarity(wr,channo,rxbuf);
	}

#ifdef CONFIG_PRODUCT_WANPIPE_TDM_VOICE_ECHOMASTER
	wp_tdmv_echo_check(wan_tdmv, &wr->chans[channo], channo);
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

static int wp_tdmv_remora_rx_tx_span(void *pcard)
{
	
	sdla_t			*card = (sdla_t*)pcard;
	sdla_fe_t		*fe = &card->fe;
	wan_tdmv_t		*wan_tdmv = &card->wan_tdmv;
        wp_tdmv_remora_t	*wr = NULL;
		
	WAN_ASSERT(wan_tdmv->sc == NULL);
	wr = wan_tdmv->sc;
	fe->rm_param.intcount++;
	wp_tdmv_remora_rx_tx_span_common(pcard);
	zt_receive(&wr->span);
	zt_transmit(&wr->span);

	return 0;
}

static int wp_tdmv_remora_ec_span(void *pcard)
{
	sdla_t          *card = (sdla_t*)pcard;
	wan_tdmv_t      *wan_tdmv = &card->wan_tdmv;
	wp_tdmv_remora_t	*wr = NULL;

	WAN_ASSERT(wan_tdmv->sc == NULL);
	wr = wan_tdmv->sc;


	zt_ec_span(&wr->span);

	return 0;

}

static void wp_tdmv_remora_tone (void* card_id, wan_event_t *event)
{
	sdla_t	*card = (sdla_t*)card_id;
	wan_tdmv_t      *wan_tdmv = &card->wan_tdmv;
	wp_tdmv_remora_t	*wr = NULL;
	int fechan;

	WAN_ASSERT1(wan_tdmv->sc == NULL);
	wr = wan_tdmv->sc;

	if (event->channel <= 0) {
     	DEBUG_ERROR("%s: Error: wp_tdmv_remora_tone() Invalid Event Channel = %i\n",
				card->devname, event->channel);
		return;
	}

	fechan = event->channel-1;

	switch(event->type) {
     	case WAN_EVENT_EC_DTMF:
		case WAN_EVENT_EC_FAX_1100:
			 DEBUG_TDMV(
				"[TDMV] %s: Received EC Tone (%s) Event at TDM (chan=%d digit=%c port=%s type=%s)!\n",
						card->devname,
						WAN_EVENT_TYPE_DECODE(event->type),
						event->channel,
						event->digit,
						(event->tone_port == WAN_EC_CHANNEL_PORT_ROUT)?"ROUT":"SOUT",
						(event->tone_type == WAN_EC_TONE_PRESENT)?"PRESENT":"STOP");   
			break;
		case  WAN_EVENT_RM_DTMF:
            	DEBUG_TDMV(
					"[TDMV_RM]: %s: Received RM DTMF Event at TDM (%d:%c)!\n",
						card->devname,
						event->channel,
						event->digit);	  
			break;
		default:
			DEBUG_ERROR("%s: %s() Error Invalid event type %X (%s)!\n",
				card->devname, __FUNCTION__, event->type, WAN_EVENT_TYPE_DECODE(event->type));
			return;
	}
	
	if (!(wr->tonemask & (1 << (event->channel-1)))){
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

		if (wr->ec_fax_detect_timeout[fechan] == 0) {
			DEBUG_TDMV("%s: FAX DETECT TIMEOUT --- Not initialized!\n",card->devname);
			return;

		} else 	if (card->tdmv_conf.hw_fax_detect &&
	    		   (SYSTEM_TICKS - wr->ec_fax_detect_timeout[fechan]) >= card->tdmv_conf.hw_fax_detect*HZ) {
#ifdef WAN_DEBUG_TDMAPI 
			if (WAN_NET_RATELIMIT()) {
				DEBUG_WARNING("%s: Warning: Ignoring Fax detect during call (s%dc%d) - Call Time: %ld  Max: %d!\n",
					card->devname,
					wr->spanno+1,
					event->channel,
					(SYSTEM_TICKS - wr->ec_fax_detect_timeout[fechan])/HZ,
					card->tdmv_conf.hw_fax_detect);
			}
#endif
			return;
		} else {
			DEBUG_TDMV("%s: FAX DETECT OK --- Ticks=%lu Timeout=%lu Diff=%lu! s%dc%d\n",
				card->devname,SYSTEM_TICKS,wr->ec_fax_detect_timeout[fechan],
				(SYSTEM_TICKS - wr->ec_fax_detect_timeout[fechan])/HZ,
				card->wan_tdmv.spanno,fechan);
		}
	}

	if (event->tone_type == WAN_EC_TONE_PRESENT){
		wr->toneactive |= (1 << (event->channel-1));
#ifdef DAHDI_ISSUES
		zt_qevent_lock(
				wr->span.chans[event->channel-1],
				(ZT_EVENT_DTMFDOWN | event->digit));
#else
		zt_qevent_lock(
				&wr->span.chans[event->channel-1],
				(ZT_EVENT_DTMFDOWN | event->digit));
#endif
        if (wr->hwec == WANOPT_YES && card->wandev.ec_dev && card->wandev.ec_enable && card->tdmv_conf.ec_off_on_fax) {
			/* Disable hwec on fax event if configuration option is enabled 
			   Disable hwec only if persist disalbe is not enabled, since in that mode asterisk controls hwec.
			   Disable hwec only once even though there might be many fax events */
           	if (!card->hwec_conf.persist_disable && !wr->ec_off_on_fax) {
				 DEBUG_EVENT("%s: Disabling hwec on fax event chan=%i\n",card->devname,event->channel);
				 card->wandev.ec_enable(card, 0, event->channel);  
                 wr->ec_off_on_fax=1;
			}
		}      

	}else{
		wr->toneactive &= ~(1 << (event->channel-1));
#ifdef DAHDI_ISSUES
		zt_qevent_lock(
				wr->span.chans[event->channel-1],
				(ZT_EVENT_DTMFUP | event->digit));
#else
		zt_qevent_lock(
				&wr->span.chans[event->channel-1],
				(ZT_EVENT_DTMFUP | event->digit));
#endif

	}
	return;
}


#ifdef WAN_SYNC_RX_TX_TEST

#warning "WAN_SYNC_RX_TX_TEST: Test option Enabled" 

static unsigned char gstat_rx_chan[1024];
static unsigned char gstat_tx_chan[1024];
static unsigned char gstat_sync[1024];
static unsigned char gstat_sync_stat[1024];

static int wp_tdmv_remora_rx_chan_sync_test(sdla_t *card, wp_tdmv_remora_t *wr, int channo, 
					    unsigned char *rxbuf,
					    unsigned char *txbuf)

{

	/* This feature should be used with HWDTMF enabled, otherwise
  	 * analog will not be able to dial. This is a debugging feature
	 * should NEVER be used in production only for testing */
	if (1 || wan_test_bit(channo,&card->wandev.rtp_tap_call_map)) {

#if 0
		if (*(unsigned int*)&wr->chans[channo].writechunk[0] == 0xD5D5D5D5 &&
		    *(unsigned int*)&wr->chans[channo].writechunk[4] == 0xD5D5D5D5) {
			DEBUG_EVENT("%s: Chan %i Rx Frame Slip!\n",		
					card->devname,channo);
		}
#endif

		int i;

		/* Pass up a sequence */
		for (i=0;i<8;i++) {
			rxbuf[i]=++gstat_rx_chan[channo];
		}


		/* Check for incoming sequence */
		if (gstat_sync_stat[channo] == 0) {
			DEBUG_EVENT("%s: Starting to hunt for sync on %i  map=0x%lX\n",		
					card->devname,channo, card->wandev.rtp_tap_call_map);
			gstat_sync_stat[channo]++;
		}
		
		for (i=0;i<8;i++) {

			if (gstat_sync[channo] == 0) {

				if (wr->chans[channo].writechunk[i] == 0x01) {
					gstat_sync[channo] = 1;
					gstat_tx_chan[channo]=wr->chans[channo].writechunk[i];
					DEBUG_EVENT("%s: Chan=%i Sync got=%i offset=%i\n",
						card->devname,channo,wr->chans[channo].writechunk[i],i);
				}

			} else {  

				gstat_tx_chan[channo]++;
				if (gstat_tx_chan[channo] !=  wr->chans[channo].writechunk[i]) {
					int x;
					DEBUG_EVENT("%s: Chan=%i Out of Sync expecting=%i got=%i offset=%i\n",
							card->devname,channo,
							gstat_tx_chan[channo],wr->chans[channo].writechunk[i],i);

					gstat_tx_chan[channo] = wr->chans[channo].writechunk[i];
					gstat_sync[channo] = 0;

					for (x=0;x<8;x++){ 
						DEBUG_EVENT("chan=%i off=%i data=%i\n",channo,x,wr->chans[channo].writechunk[x]);
					}
				}
			}
		}
	}

	return 0;
}
#endif
