/*****************************************************************************
* aft_core_prot.c
*
* 		WANPIPE(tm) AFT CORE Hardware Support - Protocol/API
*
* Authors: 	Nenad Corbic <ncorbic@sangoma.com>
*
* Copyright:	(c) 2003-2008 Sangoma Technologies Inc.
*
*		This program is free software; you can redistribute it and/or
*		modify it under the terms of the GNU General Public License
*		as published by the Free Software Foundation; either version
*		2 of the License, or (at your option) any later version.
* ============================================================================*/

#include "wanpipe_includes.h"
#include "wanpipe_defines.h"
#include "wanpipe.h"

#if defined (__LINUX__)
# include "wanproc.h"
# include "if_wanpipe.h"
#endif

#include "wanpipe_abstr.h"
#include "if_wanpipe_common.h"    /* Socket Driver common area */
#include "sdlapci.h"
#include "aft_core.h"
#include "wanpipe_iface.h"
#include "wanpipe_tdm_api.h"
#include "sdla_tdmv_dummy.h"



#if defined(AFT_XMTP2_API_SUPPORT)
/* This call back is not used by xmtp2. It is here for complete sake
 * It might be used in the future */
int wp_xmtp2_callback (void *prot_ptr, unsigned char *data, int len)
{
	private_area_t *chan = (private_area_t*)prot_ptr;
#if 0
	void * tx_skb;
	unsigned char *buff;
	int err;
#endif

	if (!chan || !data || len <= 0){
		if (WAN_NET_RATELIMIT()) {
		DEBUG_EVENT("%s:%d: Assert prot=%p  data=%p len=%d\n",
					__FUNCTION__,__LINE__,chan,data,len);
		}
		return -1;
	}

	DEBUG_EVENT("%s:%d: TX CALL BACK CALLED prot=%p  data=%p len=%d\n",
					__FUNCTION__,__LINE__,chan,data,len);

	return -1;
}
#endif



#if defined(AFT_RTP_SUPPORT)
enum {
	WAN_TDM_RTP_NO_CHANGE,
	WAN_TDM_RTP_CALL_START,
	WAN_TDM_RTP_CALL_STOP
};

void aft_rtp_unconfig(sdla_t *card)
{
	wan_rtp_chan_t *rtp_chan;
	int i;
	netskb_t *skb;

	card->wandev.rtp_len=0;

	card->rtp_conf.rtp_ip=0;
	card->rtp_conf.rtp_sample=0;

	if (card->wandev.rtp_dev) {
              	dev_put(card->wandev.rtp_dev);
		card->wandev.rtp_dev=NULL;
	}

	for (i=0;i<32;i++) {
		rtp_chan=&card->wandev.rtp_chan[i];
		if (rtp_chan->rx_skb) {
			wan_skb_free(rtp_chan->rx_skb);
			rtp_chan->rx_skb=NULL;
		}
		if (rtp_chan->tx_skb) {
			wan_skb_free(rtp_chan->tx_skb);
			rtp_chan->rx_skb=NULL;
		}
	}

	while ((skb=wan_skb_dequeue(&card->u.aft.rtp_tap_list))) {
		wan_skb_free(skb);
	}

}

int aft_rtp_config(sdla_t *card)
{
	netdevice_t *dev;
	card->wandev.rtp_tap=NULL;

	if (!card->rtp_conf.rtp_ip || !card->rtp_conf.rtp_sample) {
		return 1;
	}

	if (card->rtp_conf.rtp_sample < 10 || card->rtp_conf.rtp_sample > 150) {
		DEBUG_ERROR("%s: Error: Invalid RTP Sample %d [Min=10 Max=150ms]\n",
				card->devname,card->rtp_conf.rtp_sample);
		goto aft_rtp_init_exit;
	}

	DEBUG_EVENT("%s:    RTP TAP [ %d.%d.%d.%d:%d %dms %s %02X:%02X:%02X:%02X:%02X:%02X ]\n",
			card->devname,
			NIPQUAD(card->rtp_conf.rtp_ip),
			card->rtp_conf.rtp_port,
			card->rtp_conf.rtp_sample,
			card->rtp_conf.rtp_devname,
			card->rtp_conf.rtp_mac[0],
			card->rtp_conf.rtp_mac[1],
			card->rtp_conf.rtp_mac[2],
			card->rtp_conf.rtp_mac[3],
			card->rtp_conf.rtp_mac[4],
			card->rtp_conf.rtp_mac[5]);

	card->wandev.rtp_len = (card->rtp_conf.rtp_sample * 8) + sizeof(wan_rtp_pkt_t);

	if ((card->wandev.rtp_dev=wan_dev_get_by_name(card->rtp_conf.rtp_devname)) == NULL){
		DEBUG_EVENT("%s: Failed to open rtp tx device %s\n",
				card->devname,
				card->rtp_conf.rtp_devname);
        	goto aft_rtp_init_exit;
	}

	dev=(netdevice_t*)card->wandev.rtp_dev;

	memcpy(card->rtp_conf.rtp_local_mac,dev->dev_addr,dev->addr_len);
	card->rtp_conf.rtp_local_ip=wan_get_ip_address(card->wandev.rtp_dev,WAN_LOCAL_IP);
	if (card->rtp_conf.rtp_local_ip == 0) {
         	goto aft_rtp_init_exit;
	}

	card->wandev.rtp_tap=&aft_rtp_tap;

	memset(card->wandev.rtp_chan,0,sizeof(card->wandev.rtp_chan));

	return 0;

aft_rtp_init_exit:


	aft_rtp_unconfig(card);

	DEBUG_EVENT("%s: Failed to configure rtp tap!\n",card->devname);

	return -1;

}


__inline void aft_rtp_tap_chan(sdla_t *card, u8 *data, u32 len,
                                    netskb_t **skb_q,
				    u32 *timestamp,
				    u8 call_status,
				    u32 chan)
{
	wan_rtp_pkt_t *pkt;
	u8 *buf;
	netskb_t *skb;
	u32 ts;

	if ((skb=*skb_q) == NULL) {
		*skb_q=wan_skb_alloc(card->wandev.rtp_len+128);
		if (!*skb_q) {
                 	return;
		}
		skb=*skb_q;
		pkt = (wan_rtp_pkt_t*)wan_skb_put(skb,sizeof(wan_rtp_pkt_t));
		memset(pkt,0,sizeof(wan_rtp_pkt_t));
		pkt->rtp_hdr.version=2;
		if (IS_T1_CARD(card)) {
                	pkt->rtp_hdr.pt=0;
		} else {
                        pkt->rtp_hdr.pt=1;
		}
		DEBUG_TEST("%s: RTP(%d) SKB Allocated Len=%d \n",
				card->devname,chan,card->wandev.rtp_len);
	}

	pkt = (wan_rtp_pkt_t*)wan_skb_data(skb);

	if (call_status == WAN_TDM_RTP_CALL_START) {
        	pkt->rtp_hdr.seq=0;
		pkt->rtp_hdr.ts=0;
		*timestamp=0;
	}

	buf=wan_skb_put(skb,len);
	memcpy(buf,data,len);

	ts=htonl(pkt->rtp_hdr.ts);
	ts+=len;
	pkt->rtp_hdr.ts = htonl(ts);

        if (wan_skb_len(skb) >= card->wandev.rtp_len ||
	    call_status==WAN_TDM_RTP_CALL_STOP) {
	        netskb_t *nskb;
		u16 seq;

		pkt->rtp_hdr.ts = *timestamp;

		wan_ip_udp_setup(card,
				 &card->rtp_conf,
				 chan,
				 wan_skb_data(skb),
				 wan_skb_len(skb)-sizeof(wan_rtp_pkt_t));


		nskb=wan_skb_clone(skb);
		if (nskb) {
                	nskb->next = nskb->prev = NULL;
			nskb->dev = card->wandev.rtp_dev;
			nskb->protocol = htons(ETH_P_802_2);
			wan_skb_reset_mac_header(nskb);
			wan_skb_reset_network_header(nskb);

			wan_skb_queue_tail(&card->u.aft.rtp_tap_list,nskb);
			aft_core_taskq_trigger(card,AFT_RTP_TAP_Q);

			DEBUG_TEST("%s: RTP(%d) SKB Tx on dev %s \n",
				card->devname,chan,nskb->dev->name);
		}

		wan_skb_trim(skb,sizeof(wan_rtp_pkt_t));

		pkt = (wan_rtp_pkt_t*)wan_skb_data(skb);

		seq=htons(pkt->rtp_hdr.seq);
		seq++;
		pkt->rtp_hdr.seq=htons(seq);

		*timestamp = htonl(ts);
		DEBUG_TEST("Chan=%d Seq=%d TS=%d\n",chan,seq,ts);
		pkt->rtp_hdr.ts = htonl(ts);
	}
}

void aft_rtp_tap(void *card_ptr, u8 chan, u8* rx, u8* tx, u32 len)
{
	sdla_t *card = (sdla_t *)card_ptr;
	u8 call_status=WAN_TDM_RTP_NO_CHANGE;
	wan_rtp_chan_t *rtp_chan;
	u32 span;

	if (!card || !rx || !tx ) {
		if (WAN_NET_RATELIMIT()) {
			DEBUG_ERROR("%s: Internal Error: rtp tap invalid pointers chan %d\n",
					__FUNCTION__,chan);
		}
		return;
	}

	if (!card->rtp_conf.rtp_ip ||
	    !card->rtp_conf.rtp_sample ||
	    !card->wandev.rtp_len ||
	    !card->wandev.rtp_dev) {
		if (WAN_NET_RATELIMIT()) {
			DEBUG_EVENT("%s: RTP Tap Not configured %d\n",
					card->devname,chan);
		}
         	return;
	}

	if (chan >= 32) {
		if (WAN_NET_RATELIMIT()) {
			DEBUG_ERROR("%s: Internal Error: rtp tap chan out of range %d\n",
					card->devname,chan);
		}
		return;
	}

	span = card->tdmv_conf.span_no-1;
	rtp_chan = &card->wandev.rtp_chan[chan];

	if (wan_test_bit(chan,&card->wandev.rtp_tap_call_map)) {
		if (!wan_test_and_set_bit(chan,&card->wandev.rtp_tap_call_status)) {
        		/* Start of the call */
			call_status=WAN_TDM_RTP_CALL_START;
			DEBUG_TEST("%s: CALL Start on ch %d\n",
					card->devname,chan);
		}
	} else {
		if (!wan_test_bit(chan,&card->wandev.rtp_tap_call_status)) {
			/* Call not up */
			return;
		}
		wan_clear_bit(chan,&card->wandev.rtp_tap_call_status);
		call_status=WAN_TDM_RTP_CALL_STOP;
		DEBUG_TEST("%s: CALL Stop on ch %d\n",card->devname,chan);
	}


	aft_rtp_tap_chan(card, rx, len, &rtp_chan->rx_skb, &rtp_chan->rx_ts,
			 call_status, (span<<4|(chan+1)));
	aft_rtp_tap_chan(card, tx, len, &rtp_chan->tx_skb, &rtp_chan->tx_ts,
			 call_status, (span<<4|(chan+1))+2000);

}
#endif



/**SECTION*************************************************************
 *
 * 	Protocol API Support Functions
 *
 **********************************************************************/


int protocol_init (sdla_t *card, netdevice_t *dev,
		          private_area_t *chan,
			  wanif_conf_t* conf)
{

	chan->common.protocol = conf->protocol;

	DEBUG_TEST("%s: Protocol init 0x%X PPP=0x0%x FR=0x0%X\n",
			wan_netif_name(dev), chan->common.protocol,
			WANCONFIG_PPP,
			WANCONFIG_FR);

#ifndef CONFIG_PRODUCT_WANPIPE_GENERIC
	DEBUG_EVENT("%s: AFT Driver doesn't directly support any protocols!\n",
			chan->if_name);
	return -1;

#else
	if (chan->common.protocol == WANCONFIG_PPP ||
	    chan->common.protocol == WANCONFIG_CHDLC){

		struct ifreq		ifr;
		struct if_settings	ifsettings;

		wanpipe_generic_register(card, dev, wan_netif_name(dev));
		chan->common.prot_ptr = dev;

		if (chan->common.protocol == WANCONFIG_CHDLC){
			DEBUG_EVENT("%s: Starting Kernel CISCO HDLC protocol\n",
					chan->if_name);
			ifsettings.type = IF_PROTO_CISCO;
		}else{
			DEBUG_EVENT("%s: Starting Kernel Sync PPP protocol\n",
					chan->if_name);
			ifsettings.type = IF_PROTO_PPP;

		}
		ifr.ifr_data = (caddr_t)&ifsettings;
		if (wp_lite_set_proto(dev, &ifr)){
			wanpipe_generic_unregister(dev);
			return -EINVAL;
		}

	}else if (chan->common.protocol == WANCONFIG_GENERIC){
		chan->common.prot_ptr = dev;

	}else{
		DEBUG_EVENT("%s:%s: Unsupported protocol %d\n",
				card->devname,chan->if_name,chan->common.protocol);
		return -EPROTONOSUPPORT;
	}
#endif

	return 0;
}


int protocol_stop (sdla_t *card, netdevice_t *dev)
{
	private_area_t *chan=wan_netif_priv(dev);
	int err = 0;

	if (!chan)
		return 0;

	return err;
}

int protocol_shutdown (sdla_t *card, netdevice_t *dev)
{
	private_area_t *chan=wan_netif_priv(dev);

	if (!chan)
		return 0;

#ifndef CONFIG_PRODUCT_WANPIPE_GENERIC

	return 0;
#else

	if (chan->common.protocol == WANCONFIG_PPP ||
	    chan->common.protocol == WANCONFIG_CHDLC){

		chan->common.prot_ptr = NULL;
		wanpipe_generic_unregister(dev);

	}else if (chan->common.protocol == WANCONFIG_GENERIC){
		DEBUG_EVENT("%s:%s Protocol shutdown... \n",
				card->devname, chan->if_name);
	}
#endif
	return 0;
}

void protocol_recv(sdla_t *card, private_area_t *chan, netskb_t *skb)
{

#ifdef CONFIG_PRODUCT_WANPIPE_GENERIC
	if (chan->common.protocol == WANCONFIG_PPP ||
	    chan->common.protocol == WANCONFIG_CHDLC){
		wanpipe_generic_input(chan->common.dev, skb);
		return 0;
	}

#if defined(__LINUX__)
	if (chan->common.protocol == WANCONFIG_GENERIC){
		skb->protocol = htons(ETH_P_HDLC);
		skb->dev = chan->common.dev;
		wan_skb_reset_mac_header(skb);
		netif_rx(skb);
		return 0;
	}
#endif

#endif

#if defined(__LINUX__)

	skb->protocol = htons(ETH_P_IP);
	skb->dev = chan->common.dev;
	wan_skb_reset_mac_header(skb);
	netif_rx(skb);

#elif defined(__FreeBSD__)

	wan_skb_set_csum(skb,0);

	if (wan_iface.input && wan_iface.input(chan->common.dev, skb) != 0){
		WAN_NETIF_STATS_INC_RX_DROPPED(&chan->common);	//++chan->if_stats.rx_dropped;
		wan_skb_free(skb);
		return;
	}

#else
	DEBUG_EVENT("%s: Action not supported (IP)!\n",
				card->devname);
	wan_skb_free(skb);
#endif

	return;
}



#ifdef CONFIG_PRODUCT_WANPIPE_ANNEXG
int bind_annexg(netdevice_t *dev, netdevice_t *annexg_dev)
{
	wan_smp_flag_t smp_flags=0;
	private_area_t* chan = wan_netif_priv(dev);
	sdla_t *card = chan->card;
	if (!chan)
		return -EINVAL;

	if (chan->common.usedby != ANNEXG)
		return -EPROTONOSUPPORT;

	if (chan->annexg_dev)
		return -EBUSY;

	wan_spin_lock_irq(&card->wandev.lock,&smp_flags);
	chan->annexg_dev = annexg_dev;
	wan_spin_unlock_irq(&card->wandev.lock,&smp_flags);
	return 0;
}


netdevice_t * un_bind_annexg(wan_device_t *wandev, netdevice_t *annexg_dev)
{
	struct wan_dev_le	*devle;
	netdevice_t *dev;
	wan_smp_flag_t smp_flags=0;
	sdla_t *card = wandev->priv;

	WAN_LIST_FOREACH(devle, &card->wandev.dev_head, dev_link){
		private_area_t* chan;

		dev = WAN_DEVLE2DEV(devle);
		if (dev == NULL || (chan = wan_netif_priv(dev)) == NULL)
			continue;

		if (!chan->annexg_dev || chan->common.usedby != ANNEXG)
			continue;

		if (chan->annexg_dev == annexg_dev){
			wan_spin_lock_irq(&card->wandev.lock,&smp_flags);
			chan->annexg_dev = NULL;
			wan_spin_unlock_irq(&card->wandev.lock,&smp_flags);
			return dev;
		}
	}
	return NULL;
}


void get_active_inactive(wan_device_t *wandev, netdevice_t *dev,
			       void *wp_stats_ptr)
{
	private_area_t* 	chan = wan_netif_priv(dev);
	wp_stack_stats_t *wp_stats = (wp_stack_stats_t *)wp_stats_ptr;

	if (chan->common.usedby == ANNEXG && chan->annexg_dev){
		if (IS_FUNC_CALL(lapb_protocol,lapb_get_active_inactive)){
			lapb_protocol.lapb_get_active_inactive(chan->annexg_dev,wp_stats);
		}
	}

	if (chan->common.state == WAN_CONNECTED){
		wp_stats->fr_active++;
	}else{
		wp_stats->fr_inactive++;
	}
}

int get_map(wan_device_t *wandev, netdevice_t *dev, struct seq_file* m, int* stop_cnt)
{
	private_area_t*	chan = wan_netif_priv(dev);

	if (!(dev->flags&IFF_UP)){
		return m->count;
	}

	if (chan->common.usedby == ANNEXG && chan->annexg_dev){
		if (IS_FUNC_CALL(lapb_protocol,lapb_get_map)){
			return lapb_protocol.lapb_get_map(chan->annexg_dev,
							 m);
		}
	}

	PROC_ADD_LINE(m,
		"%15s:%s:%c:%s:%c\n",
		chan->label,
		wandev->name,(wandev->state == WAN_CONNECTED) ? '*' : ' ',
		dev->name,(chan->common.state == WAN_CONNECTED) ? '*' : ' ');

	return m->count;
}

#endif


#ifdef AFT_TDM_API_SUPPORT

int aft_tdm_api_init(sdla_t *card, private_area_t *chan, wanif_conf_t *conf)
{
	int err=0;
	int chan_no=0;
	wanpipe_tdm_api_span_t *span;

	if (chan->common.usedby != TDM_VOICE_API	&&
	    chan->common.usedby != TDM_VOICE_DCHAN	&&
		!chan->wp_api_op_mode) {
	    	return 0;
	}

	if (chan->tdmv_zaptel_cfg) {
		return 0;
	}

	if (!card->tdm_api_span) {
		card->tdm_api_span=wan_malloc(sizeof(wanpipe_tdm_api_span_t));
		if (!card->tdm_api_span) {
			return -ENOMEM;
		}
		memset(card->tdm_api_span,0,sizeof(wanpipe_tdm_api_span_t));
	}

	span = (wanpipe_tdm_api_span_t*)card->tdm_api_span;
	
	if (IS_TE1_CARD(card)) {
		if (IS_T1_CARD(card)){
			chan_no=(u8)chan->first_time_slot+1;
		} else {
			chan_no=(u8)chan->first_time_slot;
		}
	} else if (IS_BRI_CARD(card)) {

		if (chan->dchan_time_slot >= 0) {
			chan_no = 3;
		} else {
			chan_no = (u8)(chan->first_time_slot % 2)+1;
		}
	} else if (IS_GSM_CARD(card)) {
		if (chan->dchan_time_slot >= 0) {
			chan_no = MAX_GSM_CHANNELS; /* d-chan is the last one */
		} else {
			/* funky math to calculate the chan number based on port number, -1 to remove the dchan from the tdm slot count */
			chan_no=(u8) (chan->first_time_slot + 1) - ((MAX_GSM_CHANNELS-1) * card->wandev.comm_port);
		}
	} else {
		chan_no=(u8)chan->first_time_slot+1;
	}


	/* hdlc_streaming indicates hw hdlc
	   sw_hdlc_mode indicates sw hdlc */
	if (conf->hdlc_streaming || chan->sw_hdlc_mode) {
		wan_clear_bit(chan_no, &span->timeslot_map);
	} else {
		wan_set_bit(chan_no, &span->timeslot_map);
	}

	DEBUG_TEST("%s: Setting Timeslot Map chan =%i  map=0x%X\n",chan->if_name,chan_no,span->timeslot_map);


	/* chan wp_tdm_api_dev is being initalized here: 
	   DO NOT USE wp_tdm_api_dev ABOVE THIS LINE */
	chan->wp_tdm_api_dev = &span->chans[chan_no];

	/* Initialize the channel for first time use */
	memset(chan->wp_tdm_api_dev,0,sizeof(span->chans[chan_no]));

	/* Initilaize TDM API Parameters */
	chan->wp_tdm_api_dev->chan = chan;
	chan->wp_tdm_api_dev->card = card;
	
	strncpy(chan->wp_tdm_api_dev->name,chan->if_name,WAN_IFNAME_SZ);

	if (conf->hdlc_streaming || chan->sw_hdlc_mode) {
		chan->wp_tdm_api_dev->hdlc_framing=1;
	} else {
		chan->wp_tdm_api_dev->hdlc_framing=0;
	}

	aft_core_tdmapi_event_init(chan);

	chan->wp_tdm_api_dev->cfg.rx_disable = 0;
	chan->wp_tdm_api_dev->cfg.tx_disable = 0;
	chan->wp_tdm_api_dev->cfg.tx_queue_sz = MAX_AFT_DMA_CHAINS;


	chan->wp_tdm_api_dev->tdm_chan = chan_no;

	if (IS_TE1_CARD(card)) {
		if (IS_T1_CARD(card)){
			chan->wp_tdm_api_dev->cfg.hw_tdm_coding=WP_MULAW;
		}else{
			chan->wp_tdm_api_dev->cfg.hw_tdm_coding=WP_ALAW;
		}

		if (IS_E1_CARD(card)){
			chan->wp_tdm_api_dev->active_ch = conf->active_ch;
		}else{
			chan->wp_tdm_api_dev->active_ch = conf->active_ch << 1;
		}

	} else if (IS_BRI_CARD(card)) {

		if (chan->dchan_time_slot >= 0) {
			wan_set_bit(chan->wp_tdm_api_dev->tdm_chan,&chan->wp_tdm_api_dev->active_ch);
		} else {
			wan_set_bit(chan->wp_tdm_api_dev->tdm_chan,&chan->wp_tdm_api_dev->active_ch);
			if (chan->num_of_time_slots == 2) {
				wan_set_bit(chan->wp_tdm_api_dev->tdm_chan+1,&chan->wp_tdm_api_dev->active_ch);
			}
		}

		if (card->fe.fe_cfg.tdmv_law == WAN_TDMV_MULAW){
		       	chan->wp_tdm_api_dev->cfg.hw_tdm_coding=WP_MULAW;
		} else {
                chan->wp_tdm_api_dev->cfg.hw_tdm_coding=WP_ALAW;
		}
	} else if (IS_GSM_CARD(card)) {
		DEBUG_EVENT("WANPIPE: IS_GSM_CARD\n");
		/* multiple ports share the same active_ch map, use the port number to decide the proper tdm slot */
		wan_set_bit((chan->wp_tdm_api_dev->tdm_chan + (card->wandev.comm_port * MAX_GSM_CHANNELS)),&chan->wp_tdm_api_dev->active_ch);
				chan->wp_tdm_api_dev->cfg.hw_tdm_coding=WP_LIN16;
	} else {
		if (card->fe.fe_cfg.tdmv_law == WAN_TDMV_MULAW){
		    	chan->wp_tdm_api_dev->cfg.hw_tdm_coding=WP_MULAW;
		} else {
            		chan->wp_tdm_api_dev->cfg.hw_tdm_coding=WP_ALAW;
		}
		chan->wp_tdm_api_dev->active_ch = conf->active_ch << 1;
	}

	DEBUG_TDMAPI("%s: TDM API ACTIVE CH 0x%08X  CHAN=%d\n",
		chan->if_name, chan->wp_tdm_api_dev->active_ch,chan->wp_tdm_api_dev->tdm_chan);


	chan->wp_tdm_api_dev->cfg.idle_flag = conf->u.aft.idle_flag;
	chan->wp_tdm_api_dev->cfg.rbs_tx_bits = conf->u.aft.rbs_cas_idle;

	chan->wp_tdm_api_dev->tdm_span = card->tdmv_conf.span_no;

	DEBUG_TDMAPI("%s: TDM API ACTIVE CH 0x%08X SPAN=%d CHAN=%d\n",
		chan->if_name, chan->wp_tdm_api_dev->active_ch,
		chan->wp_tdm_api_dev->tdm_span,chan->wp_tdm_api_dev->tdm_chan);

	chan->wp_tdm_api_dev->dtmfsupport = card->u.aft.tdmv_hw_tone;
	
	DEBUG_TDMAPI("%s: SPAN=%d, CHAN=%d Chunk=%d Period=%d Mtu=%d\n", chan->if_name,
		chan->wp_tdm_api_dev->tdm_span, chan->wp_tdm_api_dev->tdm_chan,  chan->tdm_api_chunk, chan->tdm_api_period, chan->mtu);

	DEBUG_TDMAPI("%s: conf->mtu=%d\n", chan->if_name, conf->mtu);

	DEBUG_TDMAPI("%s: tdm_api_chunk=%d, tdm_api_period=%d\n", chan->if_name,
		chan->tdm_api_chunk, chan->tdm_api_period);

	chan->wp_tdm_api_dev->operation_mode=chan->wp_api_op_mode;

	if (chan->wp_api_op_mode == WP_TDM_OPMODE_CHAN) {
		chan->wp_tdm_api_dev->operation_mode	= WP_TDM_OPMODE_CHAN;
		chan->wp_tdm_api_dev->cfg.hw_mtu_mru = 8;
		chan->wp_tdm_api_dev->cfg.usr_period = chan->tdm_api_period;
		chan->wp_tdm_api_dev->cfg.usr_mtu_mru = chan->tdm_api_chunk;
		if (chan->wp_tdm_api_dev->cfg.usr_mtu_mru < 160) {
			chan->tdm_api_period=20;
			chan->wp_tdm_api_dev->cfg.usr_period=20;
			chan->tdm_api_chunk=160;
         	chan->wp_tdm_api_dev->cfg.usr_mtu_mru=160;
		}
	} else {
		chan->wp_tdm_api_dev->operation_mode	= WP_TDM_OPMODE_SPAN;
		chan->wp_tdm_api_dev->cfg.hw_mtu_mru = 8;
		chan->wp_tdm_api_dev->cfg.usr_period = chan->tdm_api_period;
		chan->wp_tdm_api_dev->cfg.usr_mtu_mru = chan->mtu;

		/* Overwite the chan number based on group number */	
		chan->wp_tdm_api_dev->tdm_chan = (u8)chan->if_cnt;
	}

	switch(chan->wp_tdm_api_dev->cfg.hw_tdm_coding)	{
		case WP_LIN16:
			chan->wp_tdm_api_dev->cfg.hw_mtu_mru = 16;
			break;
		default:
				chan->wp_tdm_api_dev->cfg.hw_mtu_mru = 8;
			break;
	};

	if (chan->usedby_cfg == MTP1_API) {
		chan->wp_tdm_api_dev->operation_mode = 	WP_TDM_OPMODE_MTP1;
	}
	
	/* Set the API interface mode.  Used on windows to
	 * indicate legacy API interface */
	chan->wp_tdm_api_dev->api_mode = chan->wp_api_iface_mode;

	if (chan->if_cnt == 1) {
		DEBUG_EVENT("%s:    Memory: TDM API %d \n",
				card->devname, sizeof(wanpipe_tdm_api_dev_t));
	}

	DEBUG_TDMAPI("%s: Chunk=%i, Period=%i, MTU=%i\n",
				 card->devname,chan->tdm_api_chunk,
				 chan->tdm_api_period,chan->wp_tdm_api_dev->cfg.usr_mtu_mru);

	err=wanpipe_tdm_api_reg(chan->wp_tdm_api_dev);
	if (err){
		if (span->usage == 0) {
			wan_free(span);
			card->tdm_api_span=NULL;
		}

		return err;
	}

	span->usage++;
	wan_set_bit(0,&chan->wp_tdm_api_dev->init);
	return err;
}

#endif

int aft_tdm_api_free(sdla_t *card, private_area_t *chan)
{
#ifdef AFT_TDM_API_SUPPORT
	int err=0;
	wanpipe_tdm_api_span_t *span;

	span = (wanpipe_tdm_api_span_t*)card->tdm_api_span;


	if (!span) {
		return 0;
	}

	if (chan->wp_tdm_api_dev && wan_test_bit(0,&chan->wp_tdm_api_dev->init)){
		wan_clear_bit(0,&chan->wp_tdm_api_dev->init);
		wan_clear_bit(chan->wp_tdm_api_dev->tdm_chan, &span->timeslot_map);

		err=wanpipe_tdm_api_unreg(chan->wp_tdm_api_dev);
		if (err){
			wan_set_bit(0,&chan->wp_tdm_api_dev->init);
			return err;
		}
		chan->wp_tdm_api_dev=NULL;

		span->usage--;
		if (span->usage < 0) {
			span->usage = 0;
		}

		if (span->usage == 0) {
			wan_free(span);
			card->tdm_api_span=NULL;
		}

	}
#endif
	return 0;
}




int aft_sw_hdlc_rx_data (void *priv_ptr, u8 *rx_data, int rx_len, uint32_t err_code)
{
	private_area_t *chan = (private_area_t*)priv_ptr;
	sdla_t *card=chan->card;
	netskb_t *skb;
	u8 *data;
	int err=0;

	//DEBUG_EVENT("%s: RX DATA Len=%i\n",chan->if_name,rx_len);
	if (card->u.aft.cfg.rx_crc_bytes == 0) {
		rx_len-=2;
	}
	

	/* FIXME: This will not work on WINDOWS */
	skb=wan_skb_alloc(rx_len+sizeof(wp_api_hdr_t));
	if (!skb) {
		WAN_NETIF_STATS_INC_RX_DROPPED(&chan->common); /* ++chan->if_stats.rx_dropped; */
		WP_AFT_CHAN_ERROR_STATS(chan->chan_stats,rx_dropped);
		return -1;
	}

	data=wan_skb_put(skb,rx_len);
	memcpy(data,rx_data,rx_len);
	
	err=aft_bh_rx(chan, skb, err_code, rx_len);
	if (err) {
		/* FIXME: This will not work on WINDOWS */
		wan_skb_free(skb);
	}

	if (wp_mtp1_poll_check(chan->sw_hdlc_dev)) {
		WAN_TASKLET_SCHEDULE((&chan->common.bh_task));
	}
	
	return err;
}

int aft_sw_hdlc_rx_reject (void *priv_ptr, char *reason)
{
	/* Do nothing */
#if 0
	private_area_t *chan = (private_area_t*)priv_ptr;
	WP_AFT_CHAN_ERROR_STATS(chan->chan_stats,rx_crc_errors);
#endif
	return 0;
	
}

int aft_sw_hdlc_rx_suerm (void *priv_ptr)
{
	/* Pass up suerm event */
#if 0
	private_area_t *chan = (private_area_t*)priv_ptr;
	WP_AFT_CHAN_ERROR_STATS(chan->chan_stats,rx_crc_errors);
#endif
	return 0;
}

int aft_sw_hdlc_wakup(void *priv_ptr)
{
	private_area_t *chan = (private_area_t*)priv_ptr;
	wanpipe_wake_stack(chan);
	return 0;
}

int aft_sw_hdlc_trace(void *priv_ptr, u8 *data, int len, int dir)
{
	private_area_t *chan = (private_area_t*)priv_ptr;
	private_area_t *top_chan;
	int err;
	int channel=IS_E1_CARD(chan->card) ? chan->first_time_slot : chan->first_time_slot+1;
	
	top_chan=chan;
	if (chan->channelized_cfg) {
		top_chan=wan_netif_priv(chan->common.dev);
	} 
	
	if (dir) {
		err=wan_capture_trace_packet_buffer(chan->card, &top_chan->trace_info, data, len ,TRC_INCOMING_FRM, channel);
	} else {
		err=wan_capture_trace_packet_buffer(chan->card, &top_chan->trace_info, data, len ,TRC_OUTGOING_FRM, channel);
	}
	if (err) {
		WAN_NETIF_STATS_INC_RX_DROPPED(&chan->common);	
	}

	return err;
}

