/*****************************************************************************
* sdla_gsm_tdmv.c 
* 		
* 		WANPIPE(tm) AFT W400 Hardware Support for DAHDI (AKA Zaptel)
*
* Authors: 	Moises Silva <moy@sangoma.com>
*
* Copyright:	(c) 2011 Sangoma Technologies Inc.
*
*		This program is free software; you can redistribute it and/or
*		modify it under the terms of the GNU General Public License
*		as published by the Free Software Foundation; either version
*		2 of the License, or (at your option) any later version.
* ============================================================================
* Oct 11, 2011  Moises Silva Initial Version
*****************************************************************************/

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
# include "if_wanpipe_common.h"
# include "sdla_gsm.h"
# include "sdla_gsm_tdmv.h"
# include "sdla_gsm_demo.h"
# include "zapcompat.h"
# include "wanpipe_dahdi_abstr.h"

#define WP_TDMV_REGISTER	1	/*0x01*/

/* our GSM hardware gives us linear audio */
#define GSM_CHUNKSIZE ZT_CHUNKSIZE * 2

#define gsm_swap16(sample) (((sample >> 8) & 0x00FF) | ((sample << 8) & 0xFF00))

extern WAN_LIST_HEAD(, wan_tdmv_) wan_tdmv_head;
static int wp_gsm_no = 0;

/*******************************************************************************
*			  FUNCTION PROTOTYPES
*******************************************************************************/
static int wp_tdmv_gsm_check_mtu(void* pcard, unsigned long timeslot_map, int *mtu);
static int wp_tdmv_gsm_create(void* pcard, wan_tdmv_conf_t*);
static void wp_tdmv_gsm_report_alarms(void* pcard, uint32_t te_alarm);
static int wp_tdmv_gsm_remove(void* pcard);
static int wp_tdmv_gsm_reg(void* pcard, wan_tdmv_if_conf_t*, unsigned int, unsigned char,netdevice_t*);
static int wp_tdmv_gsm_unreg(void* pcard, unsigned long ts_map);
static int wp_tdmv_gsm_software_init(wan_tdmv_t *wan_tdmv);
static int wp_tdmv_gsm_state(void* pcard, int state);
static int wp_tdmv_gsm_running(void* pcard);
static int wp_tdmv_gsm_is_rbsbits(wan_tdmv_t *wan_tdmv);
static int wp_tdmv_gsm_rx_tx_span(void *pcard);
static int wp_tdmv_gsm_rx_chan(wan_tdmv_t*, int,unsigned char*,unsigned char*); 
static int wp_tdmv_gsm_rx_dchan(wan_tdmv_t*, int,unsigned char*,unsigned int); 
static int wp_tdmv_gsm_ec_span(void *pcard);
#ifdef DAHDI_25
static int wp_gsm_chanconfig(struct file *file, struct zt_chan *chan, int sigtype);
#else
static int wp_gsm_chanconfig(struct zt_chan *chan, int sigtype);
#endif
static int wp_gsm_zap_open(struct zt_chan *chan);
static int wp_gsm_zap_close(struct zt_chan *chan);
static int wp_gsm_zap_hooksig(struct zt_chan *chan, zt_txsig_t txsig);
static int wp_gsm_zap_watchdog(struct zt_span *span, int event);
		
static int wp_gsm_zap_ioctl(struct zt_chan *chan, unsigned int cmd, unsigned long data);

static void wp_tdmv_gsm_service_uart(wp_tdmv_gsm_t *gsm_span, int flush);

#if defined(DAHDI_24) || defined(DAHDI_25)

static const struct dahdi_span_ops wp_tdm_span_ops = {
	.owner = THIS_MODULE,
	.chanconfig = wp_gsm_chanconfig,
	.open = wp_gsm_zap_open,
	.close  = wp_gsm_zap_close,
	.ioctl = wp_gsm_zap_ioctl,
	.hooksig = wp_gsm_zap_hooksig,
	.watchdog = wp_gsm_zap_watchdog,
	.echocan_create = NULL,
};
 
#endif


static int wp_gsm_zap_ioctl(struct zt_chan *chan, unsigned int cmd, unsigned long data)
{
	wp_tdmv_gsm_t *gsm_span = NULL;
	sdla_t *card = NULL;

	gsm_span = WP_PRIV_FROM_CHAN(chan, wp_tdmv_gsm_t);
	WAN_ASSERT2(gsm_span == NULL, -ENODEV);
	
	WAN_ASSERT(gsm_span->card == NULL);
	card = gsm_span->card;

	switch (cmd) {
	case ZT_TONEDETECT:
		return -ENOTTY;
		break;
	default:
		DEBUG_ERROR("%s: GSM channel %d can't perform operation %d\n", gsm_span->devname, chan->chanpos, cmd);
		DEBUG_ERROR("%s: chan=%d, cmd=0x%x\n", gsm_span->devname, chan->chanpos, cmd);
		DEBUG_ERROR("%s: IOC_TYPE=0x%02X\n", gsm_span->devname, _IOC_TYPE(cmd));
		DEBUG_ERROR("%s: IOC_DIR=0x%02X\n", gsm_span->devname, _IOC_DIR(cmd));
		DEBUG_ERROR("%s: IOC_NR=0x%02X\n", gsm_span->devname, _IOC_NR(cmd));
		DEBUG_ERROR("%s: IOC_SIZE=0x%02X\n", gsm_span->devname, _IOC_SIZE(cmd));
		dump_stack();
		return -ENOTTY;
		break;
	}
	return 0;
}

static int wp_gsm_zap_hooksig(struct zt_chan *chan, zt_txsig_t txsig)
{
	wp_tdmv_gsm_t *gsm_span = NULL;
	sdla_t *card = NULL;

	gsm_span = WP_PRIV_FROM_CHAN(chan, wp_tdmv_gsm_t);
	WAN_ASSERT2(gsm_span == NULL, -ENODEV);
	WAN_ASSERT(gsm_span->card == NULL);
	card = gsm_span->card;

	DEBUG_ERROR("%s: GSM Module %d can't perform hook operations\n", gsm_span->devname, chan->chanpos);

	return -EINVAL;
}

static int wp_gsm_zap_open(struct zt_chan *chan)
{
	wp_tdmv_gsm_t *gsm_span = NULL;
	sdla_t *card = NULL; 

	WAN_ASSERT2(chan == NULL, -ENODEV);
	gsm_span = WP_PRIV_FROM_CHAN(chan, wp_tdmv_gsm_t);
	WAN_ASSERT2(gsm_span == NULL, -ENODEV);
	WAN_ASSERT2(gsm_span->card == NULL, -ENODEV);
	card = gsm_span->card; 
	gsm_span->usecount++;
	wanpipe_open(card);
	DEBUG_EVENT("%s: GSM Open (usecount=%d, channo=%d, chanpos=%d)...\n", 
				gsm_span->devname,
				gsm_span->usecount,
				chan->channo,
				chan->chanpos);

	/* Make sure there is nothing in the FIFOs */
	wp_tdmv_gsm_service_uart(gsm_span, 1);
	return 0;
}

static int wp_gsm_zap_close(struct zt_chan *chan)
{
	sdla_t *card = NULL;	
	wp_tdmv_gsm_t *gsm_span = NULL;
	sdla_fe_t *fe = NULL;

	WAN_ASSERT2(chan == NULL, -ENODEV);
	gsm_span = WP_PRIV_FROM_CHAN(chan, wp_tdmv_gsm_t);
	WAN_ASSERT2(gsm_span == NULL, -ENODEV);
	card = gsm_span->card;
	fe = &card->fe;
	gsm_span->usecount--;
	wanpipe_close(card);   
	return 0;
}

static int wp_gsm_zap_watchdog(struct zt_span *span, int event)
{
	DEBUG_ERROR("GSM Watchdog?!!\n");
	return -1;
}

static int wp_tdmv_gsm_hook(sdla_fe_t *fe, int mod_no, int off_hook)
{
	DEBUG_ERROR("GSM hook?!!\n");
	return -EINVAL;
}

static int wp_tdmv_gsm_check_hook(sdla_fe_t *fe, int mod_no)
{
	DEBUG_ERROR("GSM check hook?!!\n");
	return -EINVAL;
}

static int wp_tdmv_gsm_buf_rotate(void *pcard, u32 buf_sz, unsigned long mask, int circ_buf_len)
{
	return 0;
}

int wp_tdmv_gsm_init(wan_tdmv_iface_t *iface)
{
	WAN_ASSERT(iface == NULL);

	memset(iface, 0, sizeof(wan_tdmv_iface_t));
	iface->check_mtu	= wp_tdmv_gsm_check_mtu;
	iface->create		= wp_tdmv_gsm_create;
	iface->remove		= wp_tdmv_gsm_remove;
	iface->reg		= wp_tdmv_gsm_reg;
	iface->unreg		= wp_tdmv_gsm_unreg;
	iface->software_init	= wp_tdmv_gsm_software_init;
	iface->state		= wp_tdmv_gsm_state;
	iface->running		= wp_tdmv_gsm_running;
	iface->is_rbsbits	= wp_tdmv_gsm_is_rbsbits;
	iface->rx_tx_span	= wp_tdmv_gsm_rx_tx_span;
	iface->rx_chan		= wp_tdmv_gsm_rx_chan;
	iface->rx_dchan         = wp_tdmv_gsm_rx_dchan;
	iface->ec_span		= wp_tdmv_gsm_ec_span;  
	iface->buf_rotate       = wp_tdmv_gsm_buf_rotate;

	return 0;
}

#ifdef DAHDI_25
static int wp_gsm_chanconfig(struct file *file, struct zt_chan *chan, int sigtype)
#else
static int wp_gsm_chanconfig(struct zt_chan *chan, int sigtype)
#endif
{
	wp_tdmv_gsm_t *gsm_span = NULL;
	sdla_t *card = NULL;

	WAN_ASSERT2(chan == NULL, -ENODEV);
	gsm_span = WP_PRIV_FROM_CHAN(chan, wp_tdmv_gsm_t);
	WAN_ASSERT2(gsm_span == NULL, -ENODEV);
	card = gsm_span->card;

	DEBUG_EVENT("%s: Configuring GSM chan %d with sigtype %d ...\n", gsm_span->devname, chan->chanpos, sigtype);

	return 0;		
}

static int wp_tdmv_gsm_software_init(wan_tdmv_t *wan_tdmv)
{
	sdla_t *card = NULL;
	sdla_fe_t *fe = NULL;
	wp_tdmv_gsm_t *gsm_span = wan_tdmv->sc;

	WAN_ASSERT(gsm_span == NULL);
	WAN_ASSERT(gsm_span->card == NULL);
	card = gsm_span->card;
	fe = &card->fe;

	if (wan_test_bit(WP_TDMV_REGISTER, &gsm_span->flags)){
		DEBUG_EVENT("%s: Wanpipe device is already registered to DAHDI span # %d!\n", gsm_span->devname, gsm_span->span.spanno);
		return 0;
	}
	/* Fill in DAHDI fields */
	sprintf(gsm_span->span.name, "WGSM/%d", gsm_span->num);
	sprintf(gsm_span->span.desc, "Wanpipe GSM Board %d", gsm_span->num + 1);
	switch(fe->fe_cfg.tdmv_law){
	case WAN_TDMV_ALAW:
		gsm_span->span.deflaw = ZT_LAW_ALAW;
		break;
	case WAN_TDMV_MULAW:
		gsm_span->span.deflaw = ZT_LAW_MULAW;
		break;
	}
	
	/* Setup the voice channel */
	sprintf(gsm_span->chans[GSM_VOICE_CHANNEL].name, "WGSM/%d/1", gsm_span->num);
	gsm_span->chans[GSM_VOICE_CHANNEL].sigcap = ZT_SIG_CLEAR;
	gsm_span->chans[GSM_VOICE_CHANNEL].chanpos = 1;
	gsm_span->chans[GSM_VOICE_CHANNEL].pvt = gsm_span;

	/* Setup the UART (d-chan) channel */
	sprintf(gsm_span->chans[GSM_UART_CHANNEL].name, "WGSM/%d/2", gsm_span->num);
	gsm_span->chans[GSM_UART_CHANNEL].sigcap = ZT_SIG_HARDHDLC;
	gsm_span->chans[GSM_UART_CHANNEL].chanpos = 2;
	gsm_span->chans[GSM_UART_CHANNEL].pvt = gsm_span;

#if defined(DAHDI_24) || defined(DAHDI_25)
	gsm_span->span.ops = &wp_tdm_span_ops;
#else

	gsm_span->span.pvt = gsm_span;
#if defined(DAHDI_23)
	gsm_span->span.owner = THIS_MODULE;
#endif
	gsm_span->span.hooksig	= wp_gsm_zap_hooksig;
	gsm_span->span.open = wp_gsm_zap_open;
	gsm_span->span.close = wp_gsm_zap_close;
	gsm_span->span.ioctl = wp_gsm_zap_ioctl;
	gsm_span->span.watchdog	= wp_gsm_zap_watchdog;
	gsm_span->span.chanconfig = wp_gsm_chanconfig;
#endif

#ifdef DAHDI_ISSUES
	gsm_span->span.chans = gsm_span->chans_ptrs;
#else
	gsm_span->span.chans = gsm_span->chans;
#endif
	gsm_span->span.channels	= MAX_GSM_CHANNELS;

#if defined(__LINUX__)
#ifndef DAHDI_25
	init_waitqueue_head(&gsm_span->span.maintq);
#endif
#endif
	if (wp_dahdi_register_device(gsm_span)) {
		DEBUG_EVENT("%s: Unable to register zaptel span\n", gsm_span->devname);
		return -EINVAL;
	}

	if (gsm_span->span.spanno != gsm_span->spanno +1){
		DEBUG_EVENT("\n");
		DEBUG_EVENT("WARNING: Span number %d is already used by another device!\n", gsm_span->spanno + 1);
		DEBUG_EVENT("         Possible cause: Another TDM driver already loaded!\n");
		DEBUG_EVENT("         Solution:       Unload wanpipe and check currently\n");
		DEBUG_EVENT("                         used spans in /proc/zaptel directory.\n");
		DEBUG_EVENT("         Reconfiguring device %s to new span number # %d\n",
						gsm_span->devname, gsm_span->span.spanno);
		DEBUG_EVENT("\n");
		gsm_span->spanno = gsm_span->span.spanno - 1;
	} else {
		DEBUG_EVENT("%s: Wanpipe GSM device is registered to Zaptel span # %d!\n", gsm_span->devname, gsm_span->span.spanno);
	}
	wan_set_bit(WP_TDMV_REGISTER, &gsm_span->flags);

	return 0;
}

static void wp_tdmv_gsm_release(wp_tdmv_gsm_t *gsm_span)
{
	WAN_ASSERT1(gsm_span == NULL);
	if (wan_test_bit(WP_TDMV_REGISTER, &gsm_span->flags)){
		DEBUG_EVENT("%s: Unregistering Wanpipe GSM from DAHDI!\n", gsm_span->devname);
		wp_dahdi_unregister_device(gsm_span);
		wan_clear_bit(WP_TDMV_REGISTER, &gsm_span->flags);
	}
	wp_dahdi_free_device(gsm_span);
	wan_free(gsm_span);
	return;
}

static int wp_tdmv_gsm_check_mtu(void *pcard, unsigned long timeslot_map, int *mtu)
{
	*mtu = GSM_CHUNKSIZE;
	return 0;
}

static void wp_tdmv_gsm_report_alarms(void *pcard, uint32_t te_alarm)
{
	sdla_t		*card = (sdla_t*)pcard;
	wan_tdmv_t	*wan_tdmv = &card->wan_tdmv;
	wp_tdmv_gsm_t	*gsm_span = wan_tdmv->sc;

	if (te_alarm == 0){
		gsm_span->span.alarms = ZT_ALARM_NONE;
		zt_alarm_notify(&gsm_span->span);
	} else {
		gsm_span->span.alarms = ZT_ALARM_RED;
		zt_alarm_notify(&gsm_span->span);
	}
}

static int wp_tdmv_gsm_create(void *pcard, wan_tdmv_conf_t *tdmv_conf)
{
	sdla_t *card = (sdla_t*)pcard;
	wp_tdmv_gsm_t *gsm_span = NULL;
	wan_tdmv_t *tmp = NULL;
	sdla_fe_t *fe = NULL;
#ifdef DAHDI_ISSUES
	int i;
#endif

	WAN_ASSERT(card == NULL);
	WAN_ASSERT(tdmv_conf->span_no == 0);

	fe = &card->fe;

   	WAN_LIST_FOREACH(tmp, &wan_tdmv_head, next){
		if (tmp->spanno == tdmv_conf->span_no){
			DEBUG_EVENT("%s: Registering GSM device with an incorrect span number!\n",
					card->devname);
			DEBUG_EVENT("%s: Another Wanpipe device already configured to span #%d!\n",
					card->devname, tdmv_conf->span_no);
			return -EINVAL;
		}
		if (!WAN_LIST_NEXT(tmp, next)){
			break;
		}
	}

	memset(&card->wan_tdmv, 0x0, sizeof(wan_tdmv_t));
	card->wan_tdmv.max_timeslots			= MAX_GSM_CHANNELS;
	card->wan_tdmv.spanno				= tdmv_conf->span_no;
	card->wandev.fe_notify_iface.hook_state		= wp_tdmv_gsm_hook;
	card->wandev.fe_notify_iface.check_hook_state	= wp_tdmv_gsm_check_hook;

	gsm_span = wan_malloc(sizeof(*gsm_span));
	if (gsm_span == NULL){
		return -ENOMEM;
	}
	memset(gsm_span, 0, sizeof(*gsm_span));
	card->wan_tdmv.sc = gsm_span;
	gsm_span->spanno = tdmv_conf->span_no - 1;
#ifdef DAHDI_ISSUES

	if (wp_dahdi_create_device(card,gsm_span)) {
    	wan_free(gsm_span);
    	return -ENOMEM;
	}

	WP_DAHDI_SET_STR_INFO(gsm_span,manufacturer,"Sangoma Technologies");

	switch(card->adptr_type){
	case AFT_ADPTR_W400:
		WP_DAHDI_SET_STR_INFO(gsm_span,devicetype,"W400");
		break;
	default:
		WP_DAHDI_SET_STR_INFO(gsm_span,devicetype,"nsupported GSM Adapter");
		break;
	}
	
	WP_DAHDI_SET_STR_INFO(gsm_span,location,"SLOT=%d, BUS=%d", card->wandev.S514_slot_no, card->wandev.S514_bus_no);

#endif


#ifndef DAHDI_26
	gsm_span->span.irq = card->wandev.irq;
#endif
	gsm_span->num = wp_gsm_no++;
	gsm_span->card = card;
	gsm_span->devname = card->devname;
#ifdef DAHDI_ISSUES
	for (i = 0; i < sizeof(gsm_span->chans)/sizeof(gsm_span->chans[0]); i++) {
		gsm_span->chans_ptrs[i] = &gsm_span->chans[i];
	}
#endif

	if (tmp) {
		WAN_LIST_INSERT_AFTER(tmp, &card->wan_tdmv, next);
	} else {
		WAN_LIST_INSERT_HEAD(&wan_tdmv_head, &card->wan_tdmv, next);
	}

	card->wandev.te_report_alarms = wp_tdmv_gsm_report_alarms;
	if (fe->fe_status != FE_CONNECTED) {
		gsm_span->span.alarms = ZT_ALARM_RED;
	}

	return 0;
}

static int wp_tdmv_gsm_reg(void	*pcard, wan_tdmv_if_conf_t *tdmv_conf, 
			unsigned int active_ch, 
			unsigned char ec_enable,
			netdevice_t *dev)
{
	sdla_t *card = (sdla_t*)pcard;
	wan_tdmv_t *wan_tdmv = &card->wan_tdmv;
	wp_tdmv_gsm_t *gsm_span = NULL;
	
	WAN_ASSERT(wan_tdmv->sc == NULL);

	gsm_span = wan_tdmv->sc;
		
	if (wan_test_bit(WP_TDMV_REGISTER, &gsm_span->flags)){
		DEBUG_ERROR("%s: Error: Master device has already been configured!\n", card->devname);
		return -EINVAL;
	}

	/* 1 is voice channel ... anything else must be the D-channel */
	if (active_ch != 1) {
		return GSM_UART_CHANNEL;
	}

	memset(gsm_span->chans[GSM_VOICE_CHANNEL].sreadchunk, 0, ZT_CHUNKSIZE);
	memset(gsm_span->chans[GSM_VOICE_CHANNEL].swritechunk, 0, ZT_CHUNKSIZE);
	gsm_span->chans[GSM_VOICE_CHANNEL].readchunk = gsm_span->chans[GSM_VOICE_CHANNEL].sreadchunk;
	gsm_span->chans[GSM_VOICE_CHANNEL].writechunk = gsm_span->chans[GSM_VOICE_CHANNEL].swritechunk;

	return GSM_VOICE_CHANNEL;
}


static int wp_tdmv_gsm_unreg(void* pcard, unsigned long ts_map)
{
	sdla_t *card = (sdla_t*)pcard;
	wan_tdmv_t *wan_tdmv = &card->wan_tdmv;
	wp_tdmv_gsm_t *gsm_span = NULL;

	WAN_ASSERT(wan_tdmv->sc == NULL);

	gsm_span = wan_tdmv->sc;

	memset(gsm_span->chans[GSM_VOICE_CHANNEL].sreadchunk, 0, ZT_CHUNKSIZE);
	memset(gsm_span->chans[GSM_VOICE_CHANNEL].swritechunk, 0, ZT_CHUNKSIZE);
	gsm_span->chans[GSM_VOICE_CHANNEL].readchunk = gsm_span->chans[GSM_VOICE_CHANNEL].sreadchunk;
	gsm_span->chans[GSM_VOICE_CHANNEL].writechunk = gsm_span->chans[GSM_VOICE_CHANNEL].swritechunk;

	return 0;
}

static int wp_tdmv_gsm_remove(void *pcard)
{
	wp_tdmv_gsm_t *gsm_span = NULL;
	sdla_t *card = (sdla_t*)pcard;
	wan_tdmv_t *wan_tdmv = &card->wan_tdmv;

	gsm_span = wan_tdmv->sc;
	if (!gsm_span){
		return -EINVAL;
	}
	
	if (gsm_span->usecount) {
		DEBUG_ERROR("%s: ERROR: Wanpipe is still in use!\n", card->devname);
		return -EINVAL;
	}

	wan_tdmv->sc = NULL;
	WAN_LIST_REMOVE(wan_tdmv, next);
	wp_tdmv_gsm_release(gsm_span);
	return 0;
}

static int wp_tdmv_gsm_state(void *pcard, int state)
{
	sdla_t *card = (sdla_t*)pcard;
	wan_tdmv_t *wan_tdmv = &card->wan_tdmv;
	wp_tdmv_gsm_t *gsm_span = NULL;

	WAN_ASSERT(wan_tdmv->sc == NULL);
	gsm_span = (wp_tdmv_gsm_t *)wan_tdmv->sc;
	
	switch(state){
	case WAN_CONNECTED:
		DEBUG_EVENT("%s: TDMV GSM state is CONNECTED!\n", gsm_span->devname);
		break;

	case WAN_DISCONNECTED:
		DEBUG_EVENT("%s: TDMV GSM state is DISCONNECTED!\n", gsm_span->devname);
		break;
	}

	return 0;
}

static int wp_tdmv_gsm_running(void *pcard)
{
	sdla_t *card = (sdla_t*)pcard;
	wan_tdmv_t *wan_tdmv = &card->wan_tdmv;
	wp_tdmv_gsm_t *gsm_span = NULL;

	gsm_span = wan_tdmv->sc;
	if (gsm_span && gsm_span->usecount){
		DEBUG_EVENT("%s: WARNING: Span is still in use!\n", card->devname);
		return -EINVAL;
	}
	return 0;
}

static int wp_tdmv_gsm_is_rbsbits(wan_tdmv_t *wan_tdmv)
{
	return -EINVAL;
}

static void wp_tdmv_gsm_service_uart(wp_tdmv_gsm_t *gsm_span, int flush)
{
	struct zt_chan *uart_chan = NULL;
	int num_bytes = 0;
	int hdlc_bytes = 0;
	int res = 0;
	int mod_no = 0;
	char uart_rx_buffer[AFT_GSM_UART_RX_FIFO_SIZE];
	char uart_tx_buffer[AFT_GSM_UART_TX_FIFO_SIZE];
	sdla_t *card = gsm_span->card;

	card = gsm_span->card;
	mod_no = card->wandev.comm_port + 1;
#ifdef DAHDI_ISSUES
	uart_chan = gsm_span->span.chans[GSM_UART_CHANNEL];
#else
	uart_chan = &gsm_span->span.chans[GSM_UART_CHANNEL];
#endif

	num_bytes = wp_gsm_uart_rx_fifo(card, uart_rx_buffer, sizeof(uart_rx_buffer));
	if (num_bytes) {
		if (flush) {
			DEBUG_WARNING("%s: Dropping %d bytes from UART of GSM module %d\n", gsm_span->devname, num_bytes, mod_no);
		} else {
			zt_hdlc_putbuf(uart_chan, uart_rx_buffer, num_bytes);
			zt_hdlc_finish(uart_chan);
		}
	}

	if (flush) {
		/* We can't flush the write fifos so we're done here */
		return;
	}

	num_bytes = wp_gsm_uart_check_tx_fifo(card);
	/* Try to write to the UART if there is something to write and if possible (UART ready) */
	if (num_bytes > 0) {
		/* There is space available to transmit something, check if there is anything to transmit in the HDLC buffers */
		hdlc_bytes = num_bytes;
		res = zt_hdlc_getbuf(uart_chan, uart_tx_buffer, &hdlc_bytes);
		if (hdlc_bytes) {
			wp_gsm_uart_tx_fifo(card, uart_tx_buffer, hdlc_bytes);
		}
	}

}

static int wp_tdmv_gsm_rx_dchan(wan_tdmv_t *wan_tdmv, int channo, unsigned char *buf, unsigned int len)
{
	sdla_t *card = NULL;
	wp_tdmv_gsm_t *gsm_span = wan_tdmv->sc;
	WAN_ASSERT2(gsm_span == NULL, -EINVAL);
	card = gsm_span->card;
	DEBUG_ERROR("%s: Error: I should never be called??! (chan = %d)\n", card->devname, len);
	return 0;
}

static int wp_tdmv_gsm_rx_chan(wan_tdmv_t *wan_tdmv, int channo, unsigned char *rxbuf, unsigned char *txbuf)
{
	sdla_t *card = NULL;
	wp_tdmv_gsm_t *gsm_span = wan_tdmv->sc;
	struct zt_chan *voice_chan = NULL;

	WAN_ASSERT2(gsm_span == NULL, -EINVAL);

	card = gsm_span->card;
#ifdef DAHDI_ISSUES
	voice_chan = gsm_span->span.chans[GSM_VOICE_CHANNEL];
#else
	voice_chan = &gsm_span->span.chans[GSM_VOICE_CHANNEL];
#endif


	/* Save the DMA tx/rx buffers provided by wanpipe, we'll write/read to/from them in wp_tdmv_gsm_rx_tx_span */
	voice_chan->readchunk = rxbuf;
	voice_chan->writechunk = txbuf;

	/* This should be used without SWRING */
	zt_ec_chunk(voice_chan, voice_chan->readchunk, gsm_span->ec_chunk);
	memcpy(gsm_span->ec_chunk, voice_chan->writechunk, ZT_CHUNKSIZE);

	wp_tdmv_gsm_service_uart(gsm_span, 0);

	wp_gsm_update_sim_status(card);

	return 0;
}

static int wp_tdmv_gsm_rx_tx_span(void *pcard)
{
	sdla_t *card = (sdla_t*)pcard;
	wan_tdmv_t *wan_tdmv = &card->wan_tdmv;
        wp_tdmv_gsm_t *gsm_span = NULL;
	unsigned short *gsm_rx_samples = NULL;
	unsigned char *gsm_tx_samples = NULL;
	unsigned char rx_samples[ZT_CHUNKSIZE];
	unsigned short tx_samples[ZT_CHUNKSIZE];
	int mod_no = 0;
	int i = 0;
	int debug_i = 0;
	struct zt_chan *voice_chan = NULL;

	WAN_ASSERT(wan_tdmv->sc == NULL);

	gsm_span = wan_tdmv->sc;
	mod_no = card->wandev.comm_port + 1;
#ifdef DAHDI_ISSUES
	voice_chan = gsm_span->span.chans[GSM_VOICE_CHANNEL];
#else
	voice_chan = &gsm_span->span.chans[GSM_VOICE_CHANNEL];
#endif

	if (wan_test_bit(AFT_GSM_AUDIO_DEBUG_TOGGLE_BIT, &card->TracingEnabled)) {
		if (!gsm_span->audio_debug_i) {
			DEBUG_EVENT("%s: Starting playback of debug audio\n", card->devname);
			wan_clear_bit(AFT_GSM_AUDIO_DEBUG_TOGGLE_BIT, &card->TracingEnabled);
			gsm_span->audio_debug_i = 1; /* always skip sample zero */
		} else {
			DEBUG_EVENT("%s: Stopping playback of debug audio\n", card->devname);
			wan_clear_bit(AFT_GSM_AUDIO_DEBUG_TOGGLE_BIT, &card->TracingEnabled);
			gsm_span->audio_debug_i = 0;
		}
	}

	gsm_rx_samples = (short *)voice_chan->readchunk;
	if (gsm_span->audio_debug_i) {
		debug_i = gsm_span->audio_debug_i;
		for (i = 0; i < ZT_CHUNKSIZE; i++) {
			short demo_sample = ZT_MULAW(gsm_demo_congrats[debug_i]);
			rx_samples[i] = ZT_LIN2X(demo_sample, voice_chan);
			debug_i++;
		}
	} else {
		for (i = 0; i < ZT_CHUNKSIZE; i++) {
			rx_samples[i] = ZT_LIN2X(gsm_swap16(gsm_rx_samples[i]), voice_chan);
		}
	}
	/* XXX FIXME: optimize this and transcode without helper buffer rx_samples to avoid this memcpy XXX */
	memcpy(voice_chan->readchunk, rx_samples, ZT_CHUNKSIZE);

	zt_receive(&gsm_span->span);
	zt_transmit(&gsm_span->span);

	gsm_tx_samples = voice_chan->writechunk;
	if (gsm_span->audio_debug_i) {
		debug_i = gsm_span->audio_debug_i;
		for (i = 0; i < ZT_CHUNKSIZE; i++) {
			tx_samples[i] = gsm_swap16(ZT_MULAW(gsm_demo_congrats[debug_i]));
			debug_i++;
		}
	} else {
		for (i = 0; i < ZT_CHUNKSIZE; i++) {
			tx_samples[i] = gsm_swap16(ZT_XLAW(gsm_tx_samples[i], voice_chan));
		}
	}
	memcpy(voice_chan->writechunk, tx_samples, GSM_CHUNKSIZE);

	if (gsm_span->audio_debug_i) {
		gsm_span->audio_debug_i += ZT_CHUNKSIZE;
		if (gsm_span->audio_debug_i >= sizeof(gsm_demo_congrats)) {
			DEBUG_EVENT("%s: Wrapping playback of debug audio\n", card->devname);
			gsm_span->audio_debug_i = 1; /* start over skipping sample 0 which is used as a flag */
		}
	}

	return 0;
}

static int wp_tdmv_gsm_ec_span(void *pcard)
{
	sdla_t *card = (sdla_t*)pcard;
	wan_tdmv_t *wan_tdmv = &card->wan_tdmv;
        wp_tdmv_gsm_t *gsm_span = NULL;

        WAN_ASSERT(wan_tdmv->sc == NULL);
        gsm_span = wan_tdmv->sc;

	zt_ec_span(&gsm_span->span);

	return 0;
}


