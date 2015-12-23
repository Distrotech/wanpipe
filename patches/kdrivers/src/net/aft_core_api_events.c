
/* aft_core_api_events.c */

#include "wanpipe_includes.h"
#include "wanpipe_defines.h"
#include "wanpipe.h"
#include "wanpipe_abstr.h"
#include "wanpipe_api_deprecated.h"
#include "if_wanpipe_common.h"    /* Socket Driver common area */
#include "sdlapci.h"
#include "aft_core.h"
#include "wanpipe_iface.h"
#include "wanpipe_tdm_api.h"
#include "sdla_tdmv_dummy.h"



#ifdef __WINDOWS__
#define wptdm_os_lock_irq(card,flags)   wan_spin_lock_irq(card,flags)
#define wptdm_os_unlock_irq(card,flags) wan_spin_unlock_irq(card,flags)
#else
#define wptdm_os_lock_irq(card,flags)
#define wptdm_os_unlock_irq(card,flags)
#endif

#if defined(AFT_API_SUPPORT) ||  defined(AFT_TDM_API_SUPPORT)

/* API VoIP event */


/*--------------------------------------------------------
 * PRIVATE EVENT FUNCTIONS Declarations
 *-------------------------------------------------------*/


static void wan_aft_api_tone (void* card_id, wan_event_t *event);
static void wan_aft_api_hook (void* card_id, wan_event_t *event);
static void wan_aft_api_ringtrip (void* card_id, wan_event_t *event);
static void wan_aft_api_ringdetect (void* card_id, wan_event_t *event);


static int aft_read_rbs_bits(void *chan_ptr, u32 ch, u8 *rbs_bits);
static int aft_write_rbs_bits(void *chan_ptr, u32 ch, u8 rbs_bits);
static int aft_write_hdlc_frame(void *chan_ptr, netskb_t *skb,  wp_api_hdr_t *hdr);
static int aft_write_hdlc_check(void *chan_ptr, int lock, int buffers);
static int aft_write_hdlc_timeout(void *chan_ptr, int lock);
static int aft_fake_dchan_transmit(sdla_t *card, void *chan_ptr, void *src_data_buffer, unsigned int buffer_len);

/*--------------------------------------------------------
 * PRIVATE EVENT FUNCTIONS
 *-------------------------------------------------------*/

static int aft_read_rbs_bits(void *chan_ptr, u32 fe_chan, u8 *rbs_bits)
{
	private_area_t *chan = (private_area_t *)chan_ptr;
	wan_smp_flag_t flags;
	int err=-EINVAL;

	if (!chan_ptr){
		return err;
	}

	chan->card->hw_iface.hw_lock(chan->card->hw,&flags);
	if (chan->card->wandev.fe_iface.read_rbsbits) {
   	 	*rbs_bits = chan->card->wandev.fe_iface.read_rbsbits(
							&chan->card->fe,
					   	 	fe_chan,
					   	 	WAN_TE_RBS_UPDATE);
		err=0;
	}  
	chan->card->hw_iface.hw_unlock(chan->card->hw,&flags);

	return err;

}

static int aft_write_rbs_bits(void *chan_ptr, u32 fe_chan, u8 rbs_bits)
{
	private_area_t *chan = (private_area_t *)chan_ptr;
	wan_smp_flag_t flags;
	int err=-EINVAL;

	if (!chan_ptr){
		return err;
	}

	chan->card->hw_iface.hw_lock(chan->card->hw,&flags);
	if (chan->card->wandev.fe_iface.set_rbsbits) {
		err = chan->card->wandev.fe_iface.set_rbsbits(&chan->card->fe,
							fe_chan,
							rbs_bits);
	}
	chan->card->hw_iface.hw_unlock(chan->card->hw,&flags);

	return err;
}



static void wan_aft_api_tone (void* card_id, wan_event_t *event)
{
	sdla_t		*card = (sdla_t*)card_id;
	private_area_t	*chan = NULL;
	netskb_t	*new_skb = NULL;
	api_rx_hdr_t 	*rx_hdr;
	wp_api_event_t *api_el;
	int		i;

	if (event->type == WAN_EVENT_EC_DTMF){
		DEBUG_TEST("%s: Received Tone Event at AFT API (%d:%c:%s:%s)!\n",
			card->devname,
			event->channel,
			event->digit,
			(event->tone_port == WAN_EC_CHANNEL_PORT_ROUT)?"ROUT":"SOUT",
			(event->tone_type == WAN_EC_TONE_PRESENT)?"PRESENT":"STOP");
	}else if (event->type == WAN_EVENT_RM_DTMF){
		DEBUG_TEST("%s: Received DTMF Event at AFT API (%d:%c)!\n",
			card->devname,
			event->channel,
			event->digit);
	}

	for (i=0;i<card->u.aft.num_of_time_slots;i++){
		if (wan_test_bit(i,&card->u.aft.logic_ch_map)){
			unsigned long	ts_map;
			chan=(private_area_t*)card->u.aft.dev_to_ch_map[i];
			ts_map = chan->time_slot_map;
			if (IS_T1_CARD(card) || IS_FXOFXS_CARD(card)){
				/* Convert active_ch bit map to user */
				ts_map	= ts_map << 1;
			}
			if (wan_test_bit(event->channel,&ts_map)){
				break;
			}
			chan = NULL;
		}
	}
	if (chan == NULL){
	 	DEBUG_EVENT("%s: Failed to find channel device (channel=%d)!\n",
					card->devname, event->channel);
		return;
	}
#if defined(__LINUX__)
	new_skb=wan_skb_alloc(sizeof(api_rx_hdr_t)+sizeof(wp_api_event_t));
	if (new_skb == NULL)
			return;

	rx_hdr=(api_rx_hdr_t*)wan_skb_put(new_skb,sizeof(api_rx_hdr_t));
	memset(rx_hdr,0,sizeof(api_rx_hdr_t));

	api_el=(wp_api_event_t *)wan_skb_put(new_skb,sizeof(wp_api_event_t));
	memset(api_el,0,sizeof(wp_api_event_t));

	rx_hdr->wp_api_hdr_operation_status = 0;
	api_el->wp_tdm_api_event_type		= WP_API_EVENT_DTMF;
	api_el->wp_tdm_api_event_channel	= event->channel;
	api_el->wp_tdm_api_event_dtmf_digit	= event->digit;
	api_el->wp_tdm_api_event_dtmf_type	= event->tone_type;
	api_el->wp_tdm_api_event_dtmf_port	= event->tone_port;

	new_skb->protocol = htons(PVC_PROT);
	wan_skb_reset_mac_header(new_skb);
	new_skb->dev      = chan->common.dev;
	new_skb->pkt_type = WAN_PACKET_DATA;

	//if (wan_api_rx_dtmf(chan,new_skb) != 0){
	if (wan_api_rx(chan,new_skb) != 0){
		DEBUG_EVENT("%s: Failed to send up DTMF event!\n",
						card->devname);
		wan_skb_free(new_skb);
	}
#else
	DEBUG_EVENT("%s:%s: DTMF Event report is not supported!\n",
				card->devname, chan->if_name);
	new_skb = NULL;
	rx_hdr = NULL;
#endif
	return;
}


static void wan_aft_api_hook (void* card_id, wan_event_t *event)
{
	sdla_t		*card = (sdla_t*)card_id;
	private_area_t	*chan = NULL;
	netskb_t	*new_skb = NULL;
	api_rx_hdr_t 	*rx_hdr;
	wp_api_event_t *api_el;
	int		i;

	if (event->type != WAN_EVENT_RM_LC){
		DEBUG_ERROR("ERROR: %s: Invalid Event type (%04X)!\n",
				card->devname, event->type);
		return;
	}
	DEBUG_EVENT("%s: Received %s (%d) Event at AFT API (%d)!\n",
			card->devname,
			WAN_EVENT_RXHOOK_DECODE(event->rxhook), event->rxhook,
			event->channel);

	for (i=0;i<card->u.aft.num_of_time_slots;i++){
		if (wan_test_bit(i,&card->u.aft.logic_ch_map)){
			unsigned long	ts_map;
			chan=(private_area_t*)card->u.aft.dev_to_ch_map[i];
			ts_map = chan->time_slot_map;
			if (IS_T1_CARD(card) || IS_FXOFXS_CARD(card)){
				/* Convert active_ch bit map to user */
				ts_map	= ts_map << 1;
			}
			if (wan_test_bit(event->channel,&ts_map)){
				break;
			}
			chan = NULL;
		}
	}
	if (chan == NULL){
	 	DEBUG_EVENT("%s: Failed to find channel device (channel=%d)!\n",
					card->devname, event->channel);
		return;
	}
#if defined(__LINUX__)
	new_skb=wan_skb_alloc(sizeof(api_rx_hdr_t)+sizeof(wp_api_event_t));
	if (new_skb == NULL) return;

	rx_hdr=(api_rx_hdr_t*)wan_skb_put(new_skb,sizeof(api_rx_hdr_t));
	memset(rx_hdr,0,sizeof(api_rx_hdr_t));

	api_el=(wp_api_event_t *)wan_skb_put(new_skb,sizeof(wp_api_event_t));
	memset(api_el,0,sizeof(wp_api_event_t));

	rx_hdr->wp_api_hdr_operation_status = 0;
	api_el->wp_tdm_api_event_type	= WP_API_EVENT_RXHOOK;
	api_el->wp_tdm_api_event_channel	= event->channel;

	if (event->rxhook == WAN_EVENT_RXHOOK_OFF){
		api_el->wp_tdm_api_event_hook_state = WP_API_EVENT_RXHOOK_OFF;
	}else if (event->rxhook == WAN_EVENT_RXHOOK_ON){
		api_el->wp_tdm_api_event_hook_state = WP_API_EVENT_RXHOOK_ON;
	}

	new_skb->protocol = htons(PVC_PROT);
	wan_skb_reset_mac_header(new_skb);
	new_skb->dev      = chan->common.dev;
	new_skb->pkt_type = WAN_PACKET_DATA;

	if (wan_api_rx(chan,new_skb) != 0){
		DEBUG_EVENT("%s: Failed to send up HOOK event!\n",
						card->devname);
		wan_skb_free(new_skb);
	}
#else
	DEBUG_EVENT("%s:%s: RXHOOK Event report is not supported!\n",
				card->devname, chan->if_name);
	new_skb = NULL;
	rx_hdr = NULL;
#endif
	return;
}



static void wan_aft_api_ringtrip (void* card_id, wan_event_t *event)
{
	sdla_t		*card = (sdla_t*)card_id;
	private_area_t	*chan = NULL;
	netskb_t	*new_skb = NULL;
	api_rx_hdr_t 	*rx_hdr;
	wp_api_event_t *api_el;
	int		i;

	if (event->type != WP_API_EVENT_RING_TRIP_DETECT){
		DEBUG_ERROR("ERROR: %s: Invalid Event type (%04X)!\n",
				card->devname, event->type);
		return;
	}
	DEBUG_EVENT("%s: Received Ring Trip Detect %s (%d) Event at AFT API (%d)!\n",
			card->devname,
			WAN_EVENT_RING_DECODE(event->ring_mode), event->ring_mode,
			event->channel);

	for (i=0;i<card->u.aft.num_of_time_slots;i++){
		if (wan_test_bit(i,&card->u.aft.logic_ch_map)){
			unsigned int ts_map;
			chan=(private_area_t*)card->u.aft.dev_to_ch_map[i];
			ts_map = chan->time_slot_map;
			if (IS_T1_CARD(card) || IS_FXOFXS_CARD(card)){
				/* Convert active_ch bit map to user */
				ts_map	= ts_map << 1;
			}
			if (wan_test_bit(event->channel,&ts_map)){
				break;
			}
			chan = NULL;
		}
	}
	if (chan == NULL){
	 	DEBUG_EVENT("%s: Failed to find channel device (channel=%d)!\n",
					card->devname, event->channel);
		return;
	}
#if defined(__LINUX__)
	new_skb=wan_skb_alloc(sizeof(api_rx_hdr_t)+sizeof(wp_api_event_t));
	if (new_skb == NULL) return;

	rx_hdr=(api_rx_hdr_t*)wan_skb_put(new_skb,sizeof(api_rx_hdr_t));
	memset(rx_hdr,0,sizeof(api_rx_hdr_t));

	api_el=(wp_api_event_t *)wan_skb_put(new_skb,sizeof(wp_api_event_t));
	memset(api_el,0,sizeof(wp_api_event_t));

	rx_hdr->wp_api_hdr_operation_status 			= 0;
	api_el->wp_tdm_api_event_type 			= WP_API_EVENT_RING_TRIP_DETECT;
	api_el->wp_tdm_api_event_channel	= event->channel;
	if (event->ring_mode == WAN_EVENT_RING_PRESENT){
		api_el->wp_tdm_api_event_ring_state = WAN_EVENT_RING_TRIP_PRESENT;
	}else if (event->ring_mode == WAN_EVENT_RING_STOP){
		api_el->wp_tdm_api_event_ring_state = WAN_EVENT_RING_TRIP_STOP;
	}else{
	 	DEBUG_EVENT("%s: Invalid Rind Detect mode (%d)!\n",
					card->devname, event->ring_mode);
		wan_skb_free(new_skb);
		return ;
	}

	new_skb->protocol = htons(PVC_PROT);
	wan_skb_reset_mac_header(new_skb);
	new_skb->dev      = chan->common.dev;
	new_skb->pkt_type = WAN_PACKET_DATA;

	if (wan_api_rx(chan,new_skb) != 0){
		DEBUG_EVENT("%s: Failed to send up HOOK event!\n",
						card->devname);
		wan_skb_free(new_skb);
	}
#else
	DEBUG_EVENT("%s:%s: RXHOOK Event report is not supported!\n",
				card->devname, chan->if_name);
	new_skb = NULL;
	rx_hdr = NULL;
#endif
	return;
}


static void wan_aft_api_ringdetect (void* card_id, wan_event_t *event)
{
	sdla_t		*card = (sdla_t*)card_id;
	private_area_t	*chan = NULL;
	netskb_t	*new_skb = NULL;
	api_rx_hdr_t 	*rx_hdr;
	wp_api_event_t *api_el;
	int		i;

	if (event->type != WAN_EVENT_RM_RING_DETECT){
		DEBUG_ERROR("ERROR: %s: Invalid Event type (%04X)!\n",
				card->devname, event->type);
		return;
	}
	DEBUG_EVENT("%s: Received Ring Detect %s (%d) Event at AFT API (%d)!\n",
			card->devname,
			WAN_EVENT_RING_DECODE(event->ring_mode), event->ring_mode,
			event->channel);

	for (i=0;i<card->u.aft.num_of_time_slots;i++){
		if (wan_test_bit(i,&card->u.aft.logic_ch_map)){
			unsigned long	ts_map;
			chan=(private_area_t*)card->u.aft.dev_to_ch_map[i];
			ts_map = chan->time_slot_map;
			if (IS_T1_CARD(card) || IS_FXOFXS_CARD(card)){
				/* Convert active_ch bit map to user */
				ts_map	= ts_map << 1;
			}
			if (wan_test_bit(event->channel,&ts_map)){
				break;
			}
			chan = NULL;
		}
	}
	if (chan == NULL){
	 	DEBUG_EVENT("%s: Failed to find channel device (channel=%d)!\n",
					card->devname, event->channel);
		return;
	}
#if defined(__LINUX__)
	new_skb=wan_skb_alloc(sizeof(api_rx_hdr_t)+sizeof(wp_api_event_t));
	if (new_skb == NULL) return;

	rx_hdr=(api_rx_hdr_t*)wan_skb_put(new_skb,sizeof(api_rx_hdr_t));
	memset(rx_hdr,0,sizeof(api_rx_hdr_t));

	api_el=(wp_api_event_t *)wan_skb_put(new_skb,sizeof(wp_api_event_t));
	memset(api_el,0,sizeof(wp_api_event_t));

	rx_hdr->wp_api_hdr_operation_status 	= 0;
	api_el->wp_tdm_api_event_type 			= WP_API_EVENT_RING_DETECT;
	api_el->wp_tdm_api_event_channel		= event->channel;
	if (event->ring_mode == WAN_EVENT_RING_PRESENT){
		api_el->wp_tdm_api_event_ring_state = WAN_EVENT_RING_PRESENT;
	}else if (event->ring_mode == WAN_EVENT_RING_STOP){
		api_el->wp_tdm_api_event_ring_state = WAN_EVENT_RING_STOP;
	}else{
	 	DEBUG_EVENT("%s: Invalid Rind Detect mode (%d)!\n",
					card->devname, event->ring_mode);
		wan_skb_free(new_skb);
		return ;
	}

	new_skb->protocol = htons(PVC_PROT);
	wan_skb_reset_mac_header(new_skb);
	new_skb->dev      = chan->common.dev;
	new_skb->pkt_type = WAN_PACKET_DATA;

	if (wan_api_rx(chan,new_skb) != 0){
		DEBUG_EVENT("%s: Failed to send up HOOK event!\n",
						card->devname);
		wan_skb_free(new_skb);
	}
#else
	DEBUG_EVENT("%s:%s: RXHOOK Event report is not supported!\n",
				card->devname, chan->if_name);
	new_skb = NULL;
	rx_hdr = NULL;
#endif
	return;
}
 
static int aft_fake_dchan_transmit(sdla_t *card, void *chan_ptr, void *src_data_buffer, unsigned int buffer_len)
{
	int		err = 0;
	/* XXX isdn_bri_dchan_tx should be renamed to dchan_tx as it works for other protocols as well (ie GSM) XXX*/
	if (card->wandev.fe_iface.isdn_bri_dchan_tx){
		err = card->wandev.fe_iface.isdn_bri_dchan_tx(
					&card->fe,
					src_data_buffer, 
					buffer_len);
	}else{
		DEBUG_WARNING("%s():%s: Warning: uninitialized isdn_bri_dchan_tx() pointer.\n",
			__FUNCTION__, card->devname);
	}

	return err;
}

static int aft_write_hdlc_check(void *chan_ptr, int lock, int buffers)
{
	private_area_t *chan = (private_area_t *)chan_ptr;
	sdla_t *card=chan->card;
	wan_smp_flag_t smp_flags=0;
	int rc=0;


	/* If we are disconnected do now allow tx */
	if (chan->common.state != WAN_CONNECTED) {
		return 1;
	}

	if (AFT_HAS_FAKE_DCHAN(card) && (chan->dchan_time_slot >= 0)){
		
		rc=aft_fake_dchan_transmit(card, chan,
						NULL,
						0);

		if (rc) {
			/* Tx busy */
			return 1;
		}

		return 0;
	}


	if (lock) {
		wan_spin_lock_irq(&card->wandev.lock, &smp_flags);
	}

	if (chan->sw_hdlc_mode) {
		if (wp_mtp1_tx_check(chan->sw_hdlc_dev)) {
			rc=1; 
		}
	} else {
		rc = wan_chan_dev_stopped(chan);
		if (!rc && buffers > 1) {
			if (chan->max_tx_bufs - wan_skb_queue_len(&chan->wp_tx_pending_list) <= buffers) {
				rc=1;
			}
		}
	}

	if (lock) {
		wan_spin_unlock_irq(&card->wandev.lock, &smp_flags);
	}

	return rc;
}

static int aft_write_hdlc_timeout(void *chan_ptr, int lock)
{
	private_area_t *chan = (private_area_t *)chan_ptr;
	sdla_t *card=chan->card;
	wan_smp_flag_t smp_flags=0;
	
	if (AFT_HAS_FAKE_DCHAN(card)) {
		return 0;
	}

	if (lock) {
		wan_spin_lock_irq(&card->wandev.lock, &smp_flags);
	}

	aft_tx_fifo_under_recover(card,chan);

	if (lock) {
		wan_spin_unlock_irq(&card->wandev.lock, &smp_flags);
	}

	return 0;
}

static int aft_write_hdlc_frame(void *chan_ptr, netskb_t *skb,  wp_api_hdr_t *hdr)
{
	private_area_t *chan = (private_area_t *)chan_ptr;
	sdla_t *card=chan->card;
	wan_smp_flag_t smp_flags;
	int err=-EINVAL;
	private_area_t	*top_chan;
	netskb_t *repeat_skb=NULL;

	if (!chan_ptr || !chan->common.dev || !card){
		WAN_ASSERT(1);
		return -EINVAL;
	}

	/* FIXME: For sw hdlc check len based on sw_hdlc mtu */
	if (!chan->sw_hdlc_mode && wan_skb_len(skb) > chan->mtu) {
		if (WAN_NET_RATELIMIT()) {
			DEBUG_ERROR("%s: %s Error: skb len %i > mtu = %i  \n",
				chan->if_name,__FUNCTION__,wan_skb_len(skb),chan->mtu);
		}
		return -EINVAL;
	}
#if defined(__WINDOWS__)
	top_chan=chan;
#else
	if (card->u.aft.tdmv_dchan){
		top_chan=wan_netif_priv(chan->common.dev);
	} else {
		top_chan=chan;
	}
#endif

	if (chan->common.state != WAN_CONNECTED){
		DEBUG_TEST("%s: %s Error Device disconnected!\n",
				chan->if_name,__FUNCTION__);
		return -EBUSY;
	}

#if defined(CONFIG_PRODUCT_WANPIPE_AFT_BRI)
	if(AFT_HAS_FAKE_DCHAN(card) && (chan->dchan_time_slot >= 0)){
		/* D-chan data NOT transmitted using DMA. */
		/* NOTE: BRI dchan tx has to be done inside IRQ lock.
			It allows to synchronize access to SPI on the card. */
		card->hw_iface.hw_lock(card->hw,&smp_flags);

		err=aft_fake_dchan_transmit(card, chan, wan_skb_data(skb), wan_skb_len(skb));

		card->hw_iface.hw_unlock(card->hw,&smp_flags);
		
		hdr->tx_h.max_tx_queue_length = 1;
		hdr->tx_h.current_number_of_frames_in_tx_queue = 1;

		/* D-channel will return 0 after accepting a frame for transmission or -EBUSY. 
		 * That means 0 is a success return code - successful tx but now queue is full. */
		if (err == 0) {

			chan->opstats.Data_frames_Tx_count++;
			chan->opstats.Data_bytes_Tx_count+=wan_skb_len(skb);
			chan->chan_stats.tx_packets++;
			chan->chan_stats.tx_bytes+=wan_skb_len(skb);
			WAN_NETIF_STATS_INC_TX_PACKETS(&chan->common);	//chan->if_stats.tx_packets++;
			WAN_NETIF_STATS_INC_TX_BYTES(&chan->common,wan_skb_len(skb));	//chan->if_stats.tx_bytes+=wan_skb_len(dma_chain->skb);

			wan_capture_trace_packet(chan->card, &top_chan->trace_info,
				     skb,TRC_OUTGOING_FRM);

			wan_skb_free(skb);
			err = 1;/* Successful tx but now queue is full - as expected by wanpipe_tdm_api.c  */		
		}


		return err;
	}
#endif

	if (chan->cfg.ss7_enable) {
        	err=aft_ss7_tx_mangle(card,chan,skb,hdr);
                if (err){
			return err;
		}
	} else if ((chan->hdlc_eng || chan->sw_hdlc_mode) && chan->cfg.hdlc_repeat) {
		err=aft_hdlc_repeat_mangle(card,chan,skb,hdr,&repeat_skb);
		if (err) {
			return err;
		}
		if (chan->tx_hdlc_rpt_on_close_skb == NULL) {
			chan->tx_hdlc_rpt_on_close_skb = wan_skb_copy(repeat_skb);
		}
	}
	
	wan_spin_lock_irq(&card->wandev.lock, &smp_flags);

	if (chan->sw_hdlc_mode) {

		if (wp_mtp1_poll_check(chan->sw_hdlc_dev)) {
			WAN_TASKLET_SCHEDULE((&chan->common.bh_task));
		}
		
		err=wp_mtp1_tx_data(chan->sw_hdlc_dev, skb, hdr, repeat_skb);
		if (err) {
			WAN_NETIF_STOP_QUEUE(chan->common.dev);

			/* FIXME: Update the header queue len based on sw_hdlc queues */
#if 0
			hdr->tx_h.max_tx_queue_length = (u8)chan->max_tx_bufs;
			hdr->tx_h.current_number_of_frames_in_tx_queue = (u8)wan_skb_queue_len(&chan->wp_tx_pending_list);

			if (chan->dma_chain_opmode != WAN_AFT_DMA_CHAIN_SINGLE) {
				hdr->tx_h.current_number_of_frames_in_tx_queue += chan->tx_chain_data_sz;
				hdr->tx_h.max_tx_queue_length += MAX_AFT_DMA_CHAINS;
			}

			hdr->tx_h.tx_idle_packets = chan->chan_stats.tx_idle_packets;
#endif
			wan_chan_dev_stop(chan);
			aft_dma_tx(card,chan);

			wan_spin_unlock_irq(&card->wandev.lock, &smp_flags);

			if (repeat_skb) {
				if (chan->tx_hdlc_rpt_on_close_skb == repeat_skb) {
					chan->tx_hdlc_rpt_on_close_skb = NULL;
				}

				wan_skb_free(repeat_skb);
			}

			return -EBUSY;
		}
		
	} else {

		
		if (wan_skb_queue_len(&chan->wp_tx_pending_list) >= chan->max_tx_bufs){
			WAN_NETIF_STOP_QUEUE(chan->common.dev);
			hdr->tx_h.max_tx_queue_length = (u8)chan->max_tx_bufs;
			hdr->tx_h.current_number_of_frames_in_tx_queue = (u8)wan_skb_queue_len(&chan->wp_tx_pending_list);
	
			if (chan->dma_chain_opmode != WAN_AFT_DMA_CHAIN_SINGLE) {
				hdr->tx_h.current_number_of_frames_in_tx_queue += chan->tx_chain_data_sz;
				hdr->tx_h.max_tx_queue_length += MAX_AFT_DMA_CHAINS;
			}
	
			hdr->tx_h.tx_idle_packets = chan->chan_stats.tx_idle_packets;
			wan_chan_dev_stop(chan);
			aft_dma_tx(card,chan);
			wan_spin_unlock_irq(&card->wandev.lock, &smp_flags);
			
			if (repeat_skb) {
				if (chan->tx_hdlc_rpt_on_close_skb == repeat_skb) {
					chan->tx_hdlc_rpt_on_close_skb = NULL;
				}

				wan_skb_free(repeat_skb);
			}
			return -EBUSY;
		}
	
		wan_skb_unlink(skb);
		wan_skb_queue_tail(&chan->wp_tx_pending_list,skb);
	
		if (repeat_skb) {
			wan_skb_queue_tail(&chan->wp_tx_hdlc_rpt_list,repeat_skb);
		}
	
		aft_dma_tx(card,chan);

	}

	hdr->tx_h.max_tx_queue_length = (u8)chan->max_tx_bufs;
	hdr->tx_h.current_number_of_frames_in_tx_queue = (u8)wan_skb_queue_len(&chan->wp_tx_pending_list);
	hdr->tx_h.tx_idle_packets = chan->chan_stats.tx_idle_packets;
	hdr->tx_h.errors = WP_AFT_TX_ERROR_SUM(chan->chan_stats);
	
	if (chan->dma_chain_opmode != WAN_AFT_DMA_CHAIN_SINGLE) {
		hdr->tx_h.max_tx_queue_length += MAX_AFT_DMA_CHAINS;
		hdr->tx_h.current_number_of_frames_in_tx_queue += chan->tx_chain_data_sz;
	}  

	if (wan_skb_queue_len(&chan->wp_tx_pending_list) >= chan->max_tx_bufs){
		WAN_NETIF_STOP_QUEUE(chan->common.dev);
		wan_chan_dev_stop(chan);
		err=1;
	} else {
		wan_netif_set_ticks(chan->common.dev, SYSTEM_TICKS);
		WAN_NETIF_START_QUEUE(chan->common.dev);
		wan_chan_dev_start(chan);
		err=0;
	}

	wan_spin_unlock_irq(&card->wandev.lock, &smp_flags);

	return err;
}


#endif /* AFT_API_SUPPORT */

/*--------------------------------------------------------
 * PUBLIC FUNCTIONS
 *-------------------------------------------------------*/

int aft_event_ctrl(void *chan_ptr, wan_event_ctrl_t *p_event)
{
	sdla_t		*card = NULL;
	private_area_t 	*chan = (private_area_t *)chan_ptr;
	wan_event_ctrl_t *event_ctrl;
	int		err = -EINVAL;

	card = chan->card;
	event_ctrl = wan_malloc(sizeof(wan_event_ctrl_t));
	if (event_ctrl == NULL){
		DEBUG_EVENT("%s: Failed to allocate event control!\n",
					card->devname);
		return -EFAULT;
	}
	memset(event_ctrl, 0, sizeof(wan_event_ctrl_t));
	memcpy(event_ctrl, p_event, sizeof(wan_event_ctrl_t));

	switch (event_ctrl->type) {

	case WAN_EVENT_EC_DTMF:
	case WAN_EVENT_EC_FAX_DETECT:

		if (card->wandev.ec_dev){
#if defined(CONFIG_WANPIPE_HWEC)
			DEBUG_TEST("%s: Event control request EC_DTMF...\n",
						chan->if_name);
			err = wanpipe_ec_event_ctrl(card->wandev.ec_dev, card, event_ctrl);
#else
			DEBUG_ERROR("%s: Error: Hardware EC not compiled! Failed to enable DTMF\n",
						chan->if_name);
			err=-EINVAL;
#endif
		} else {
			err=-EINVAL;
		}
		break;

	default:

		if (chan->card->wandev.fe_iface.event_ctrl){

			wan_smp_flag_t irq_flags,flags;
			DEBUG_TDMAPI("%s: FE Event control request...\n", chan->if_name);
	
			chan->card->hw_iface.hw_lock(chan->card->hw,&flags);
			wan_spin_lock_irq(&card->wandev.lock,&irq_flags);
	
			err = chan->card->wandev.fe_iface.event_ctrl(&chan->card->fe, event_ctrl);
	
			wan_spin_unlock_irq(&card->wandev.lock,&irq_flags);
			chan->card->hw_iface.hw_unlock(chan->card->hw,&flags);
	
			/* Front end interface does not use queue */
			wan_free(event_ctrl);
			event_ctrl=NULL;

		}else{
			DEBUG_EVENT("%s: Unsupported event control request (%X)\n",
						chan->if_name, event_ctrl->type);
		}
		break;
	}

	if (err){
		if (event_ctrl) wan_free(event_ctrl);
	}
	return err;
}



int wan_aft_api_ioctl(sdla_t *card, private_area_t *chan, char *user_data)
{
	api_tx_hdr_t		tx_hdr;
	wan_event_ctrl_t	event_ctrl;
	int			err = -EINVAL;

	if (WAN_COPY_FROM_USER(&tx_hdr, user_data, sizeof(api_tx_hdr_t))){
		DEBUG_EVENT("%s: Failed to copy data from user space!\n",
					card->devname);
		return -EFAULT;
	}
	memset(&event_ctrl, 0, sizeof(wan_event_ctrl_t));

	switch(tx_hdr.wp_api_tx_hdr_event_type){

	case WP_API_EVENT_DTMF:
		DEBUG_TEST("%s: %s HW DTMF events!\n",
			card->devname,
			(tx_hdr.wp_api_tx_hdr_event_mode==WP_API_EVENT_ENABLE)?
						"Enable": "Disable");
		event_ctrl.type   = WAN_EVENT_EC_DTMF;
		event_ctrl.ts_map = tx_hdr.wp_api_tx_hdr_event_channel;
		if (tx_hdr.wp_api_tx_hdr_event_mode == WP_API_EVENT_ENABLE){
			event_ctrl.mode = WAN_EVENT_ENABLE;
		}else{
			event_ctrl.mode = WAN_EVENT_DISABLE;
		}
		err = aft_event_ctrl(chan, &event_ctrl);
		break;

	case WP_API_EVENT_RM_DTMF:
		DEBUG_TEST("%s: %s RM DTMF events!\n",
			card->devname,
			(tx_hdr.wp_api_tx_hdr_event_mode==WP_API_EVENT_ENABLE)?
						"Enable": "Disable");
		event_ctrl.type   = WAN_EVENT_RM_DTMF;
		event_ctrl.mod_no = tx_hdr.wp_api_tx_hdr_event_channel;
		if (tx_hdr.wp_api_tx_hdr_event_mode == WP_API_EVENT_ENABLE){
			event_ctrl.mode = WAN_EVENT_ENABLE;
		}else{
			event_ctrl.mode = WAN_EVENT_DISABLE;
		}
		err = aft_event_ctrl(chan, &event_ctrl);
		break;

	case WP_API_EVENT_RXHOOK:
		DEBUG_TEST("%s: %s OFFHOOK/ONHOOK events!\n",
			card->devname,
			(tx_hdr.wp_api_tx_hdr_event_mode==WP_API_EVENT_ENABLE)?
						"Enable": "Disable");
		event_ctrl.type   = WAN_EVENT_RM_LC;
		event_ctrl.mod_no = tx_hdr.wp_api_tx_hdr_event_channel;
		if (tx_hdr.wp_api_tx_hdr_event_mode == WP_API_EVENT_ENABLE){
			event_ctrl.mode = WAN_EVENT_ENABLE;
		}else{
			event_ctrl.mode = WAN_EVENT_DISABLE;
		}
		err = aft_event_ctrl(chan, &event_ctrl);
		break;

	case WP_API_EVENT_RING:
		DEBUG_TEST("%s: %s RING events!\n",
			card->devname,
			(tx_hdr.wp_api_tx_hdr_event_mode==WP_API_EVENT_ENABLE)?
						"Enable": "Disable");
		event_ctrl.type   = WAN_EVENT_RM_RING;
		event_ctrl.mod_no = tx_hdr.wp_api_tx_hdr_event_channel;
		if (tx_hdr.wp_api_tx_hdr_event_mode == WP_API_EVENT_ENABLE){
			event_ctrl.mode = WAN_EVENT_ENABLE;
		}else{
			event_ctrl.mode = WAN_EVENT_DISABLE;
		}
		err = aft_event_ctrl(chan, &event_ctrl);
		break;

	case WP_API_EVENT_TONE:
		DEBUG_TEST("%s: %s TONE events!\n",
			card->devname,
			(tx_hdr.wp_api_tx_hdr_event_mode==WP_API_EVENT_ENABLE)?
						"Enable": "Disable");
		event_ctrl.type   = WAN_EVENT_RM_TONE;
		event_ctrl.mod_no = tx_hdr.wp_api_tx_hdr_event_channel;
		switch(tx_hdr.wp_api_tx_hdr_event_tone){
		case WP_API_EVENT_TONE_DIAL:
			event_ctrl.tone	= WAN_EVENT_RM_TONE_TYPE_DIAL;
			break;
		case WP_API_EVENT_TONE_BUSY:
			event_ctrl.tone	= WAN_EVENT_RM_TONE_TYPE_BUSY;
			break;
		case WP_API_EVENT_TONE_RING:
			event_ctrl.tone	= WAN_EVENT_RM_TONE_TYPE_RING;
			break;
		case WP_API_EVENT_TONE_CONGESTION:
			event_ctrl.tone	= WAN_EVENT_RM_TONE_TYPE_CONGESTION;
			break;
		default:
			if (tx_hdr.wp_api_tx_hdr_event_mode == WP_API_EVENT_ENABLE){
				DEBUG_EVENT("%s: Unsupported tone type %d!\n",
						card->devname,
						tx_hdr.wp_api_tx_hdr_event_tone);
				return -EINVAL;
			}
			break;
		}
		if (tx_hdr.wp_api_tx_hdr_event_mode == WP_API_EVENT_ENABLE){
			event_ctrl.mode	= WAN_EVENT_ENABLE;
		}else{
			event_ctrl.mode	= WAN_EVENT_DISABLE;
		}
		err = aft_event_ctrl(chan, &event_ctrl);
		break;

	case WP_API_EVENT_TXSIG_KEWL:
		DEBUG_TEST("%s: TXSIG KEWL for module %d!\n",
				card->devname,
				tx_hdr.wp_api_tx_hdr_event_channel);
		event_ctrl.type   = WAN_EVENT_RM_TXSIG_KEWL;
		event_ctrl.mod_no = tx_hdr.wp_api_tx_hdr_event_channel;
		err = aft_event_ctrl(chan, &event_ctrl);
		break;

	case WP_API_EVENT_TXSIG_START:
		DEBUG_TEST("%s: TXSIG START for module %d!\n",
				card->devname,
				tx_hdr.wp_api_tx_hdr_event_channel);
		event_ctrl.type   = WAN_EVENT_RM_TXSIG_START;
		event_ctrl.mod_no = tx_hdr.wp_api_tx_hdr_event_channel;
		err = aft_event_ctrl(chan, &event_ctrl);
		break;

	case WP_API_EVENT_TXSIG_OFFHOOK:
		DEBUG_TEST("%s: RM TXSIG OFFHOOK for module %d!\n",
				card->devname,
				tx_hdr.wp_api_tx_hdr_event_channel);
		event_ctrl.type   = WAN_EVENT_RM_TXSIG_OFFHOOK;
		event_ctrl.mod_no = tx_hdr.wp_api_tx_hdr_event_channel;
		err = aft_event_ctrl(chan, &event_ctrl);
		break;

	case WP_API_EVENT_TXSIG_ONHOOK:
		DEBUG_TEST("%s: RM TXSIG ONHOOK for module %d!\n",
				card->devname,
				tx_hdr.wp_api_tx_hdr_event_channel);
		event_ctrl.type   = WAN_EVENT_RM_TXSIG_ONHOOK;
		event_ctrl.mod_no = tx_hdr.wp_api_tx_hdr_event_channel;
		err = aft_event_ctrl(chan, &event_ctrl);
		break;

	case WP_API_EVENT_ONHOOKTRANSFER:
		DEBUG_TEST("%s: RM ONHOOKTRANSFER for module %d!\n",
				card->devname,
				tx_hdr.wp_api_tx_hdr_event_channel);
		event_ctrl.type   = WAN_EVENT_RM_ONHOOKTRANSFER;
		event_ctrl.mod_no = tx_hdr.wp_api_tx_hdr_event_channel;
		event_ctrl.ohttimer = tx_hdr.wp_api_tx_hdr_event_ohttimer;
		err = aft_event_ctrl(chan, &event_ctrl);
		break;

	case WP_API_EVENT_SETPOLARITY:
		DEBUG_TEST("%s: RM SETPOLARITY for module %d!\n",
				card->devname,
				tx_hdr.wp_api_tx_hdr_event_channel);
		event_ctrl.type   = WAN_EVENT_RM_SETPOLARITY;
		event_ctrl.mod_no = tx_hdr.wp_api_tx_hdr_event_channel;
		event_ctrl.polarity = tx_hdr.wp_api_tx_hdr_event_polarity;
		err = aft_event_ctrl(chan, &event_ctrl);
		break;

	case WP_API_EVENT_RING_DETECT:
		DEBUG_TEST("%s: %s: RM RING DETECT events for module %d!\n",
			card->devname,
			WP_API_EVENT_MODE_DECODE(tx_hdr.wp_api_tx_hdr_event_mode),
			tx_hdr.wp_api_tx_hdr_event_channel);
		event_ctrl.type   = WAN_EVENT_RM_RING_DETECT;
		event_ctrl.mod_no = tx_hdr.wp_api_tx_hdr_event_channel;
		if (tx_hdr.wp_api_tx_hdr_event_mode == WP_API_EVENT_ENABLE){
			event_ctrl.mode = WAN_EVENT_ENABLE;
		}else{
			event_ctrl.mode = WAN_EVENT_DISABLE;
		}
		err = aft_event_ctrl(chan, &event_ctrl);
		break;

	case WP_API_EVENT_RING_TRIP_DETECT:
		DEBUG_TEST("%s: %s: Ring Trip Detection Event on module %d!\n",
			card->devname,
			WP_API_EVENT_MODE_DECODE(tx_hdr.wp_api_tx_hdr_event_mode),
			tx_hdr.wp_api_tx_hdr_event_channel);
		event_ctrl.type   = WAN_EVENT_RM_RING_TRIP;
		event_ctrl.mod_no = tx_hdr.wp_api_tx_hdr_event_channel;
		if (tx_hdr.wp_api_tx_hdr_event_mode == WP_API_EVENT_ENABLE){
			event_ctrl.mode = WAN_EVENT_ENABLE;
		}else{
			event_ctrl.mode = WAN_EVENT_DISABLE;
		}
		err = aft_event_ctrl(chan, &event_ctrl);
		break;

	case WP_API_EVENT_RBS:
		DEBUG_TEST("%s: %s RBS events!\n",
			card->devname,
			(tx_hdr.wp_api_tx_hdr_event_mode==WP_API_EVENT_ENABLE)?
						"Enable": "Disable");
		event_ctrl.type   = WAN_EVENT_TE_RBS;
		event_ctrl.mod_no = tx_hdr.wp_api_tx_hdr_event_channel;
		if (tx_hdr.wp_api_tx_hdr_event_mode == WP_API_EVENT_ENABLE){
			event_ctrl.mode = WAN_EVENT_ENABLE;
		}else{
			event_ctrl.mode = WAN_EVENT_DISABLE;
		}
		err = aft_event_ctrl(chan, &event_ctrl);
		break;

	case WP_API_EVENT_MODEM_STATUS:

		if (!IS_SERIAL_CARD(card)) {
			err=-EINVAL;
			break;
		}

		if (tx_hdr.wp_api_tx_hdr_event_mode == WP_API_EVENT_SET){
			u32 reg;
			card->hw_iface.bus_read_4(card->hw,AFT_PORT_REG(card,AFT_SERIAL_LINE_CFG_REG),&reg);

			DEBUG_TEST("%s: Set Serial Status 0x%X\n",
				card->devname,tx_hdr.wp_api_tx_hdr_event_serial_status);

			reg&=~(0x03);
			reg|=tx_hdr.wp_api_tx_hdr_event_serial_status&0x03;
			
			card->hw_iface.bus_write_4(card->hw,AFT_PORT_REG(card,AFT_SERIAL_LINE_CFG_REG),reg);

		} else {
			u32 reg;
			card->hw_iface.bus_read_4(card->hw,AFT_PORT_REG(card,AFT_SERIAL_LINE_CFG_REG),&reg);
			tx_hdr.wp_api_tx_hdr_event_serial_status =0;
			tx_hdr.wp_api_tx_hdr_event_serial_status = (u8)reg&AFT_SERIAL_LCFG_CTRL_BIT_MASK;

			DEBUG_TEST("%s: Get Serial Status Status=0x%X\n",
						card->devname,tx_hdr.wp_api_tx_hdr_event_serial_status);
		}
		err=0;
		break;

	default:
		DEBUG_EVENT("%s: Unknown event type %02X!\n",
				card->devname,
				tx_hdr.wp_api_tx_hdr_event_type);
		err = -EINVAL;
		break;
	}
	return err;
}

static int aft_driver_ctrl(void *chan_ptr, int cmd, wanpipe_api_cmd_t *api_cmd)
{
	sdla_t		*card = NULL;
	private_area_t 	*chan = (private_area_t *)chan_ptr;
	wan_smp_flag_t smp_flags;
	int		err = 0;

	smp_flags=0;
	

	if (!chan) {
		DEBUG_ERROR("%s: ERROR: chan=NULL\n",__FUNCTION__);
		return -ENODEV;
	}

	card = chan->card;
	if (!card) {
		DEBUG_ERROR("%s: ERROR: card=NULL\n",__FUNCTION__);
		return -ENODEV;
	}

	api_cmd->result=0;

	switch (cmd)
	{

	case WP_API_CMD_SET_TX_Q_SIZE:
		if (api_cmd->tx_queue_sz) { 
			chan->max_tx_bufs = (u16)api_cmd->tx_queue_sz;
		} else {
			err=-EINVAL;
		}
		break;
	case WP_API_CMD_GET_TX_Q_SIZE:
		 api_cmd->tx_queue_sz = chan->max_tx_bufs;
		break;

	case WP_API_CMD_GET_STATS:
		chan->chan_stats.max_tx_queue_length = (u8)chan->max_tx_bufs;
		chan->chan_stats.max_rx_queue_length = (u8)chan->dma_per_ch;

		wptdm_os_lock_irq(&card->wandev.lock, &smp_flags);
		chan->chan_stats.current_number_of_frames_in_tx_queue = (u8)wan_skb_queue_len(&chan->wp_tx_pending_list);

		if (chan->dma_chain_opmode != WAN_AFT_DMA_CHAIN_SINGLE){
			chan->chan_stats.current_number_of_frames_in_tx_queue += chan->tx_chain_data_sz;
			chan->chan_stats.max_tx_queue_length += MAX_AFT_DMA_CHAINS;
		}

		chan->chan_stats.current_number_of_frames_in_rx_queue = (u8)wan_skb_queue_len(&chan->wp_rx_complete_list);
		wptdm_os_unlock_irq(&card->wandev.lock, &smp_flags);
	
		if (AFT_HAS_FAKE_DCHAN(card) && (chan->dchan_time_slot >= 0)) {

			if (aft_bri_dchan_transmit(card, chan, NULL, 0)) {
				/* Tx busy. It means there is a single frame in tx queue. */
				chan->chan_stats.current_number_of_frames_in_tx_queue = 1;
			} else {
				chan->chan_stats.current_number_of_frames_in_tx_queue = 0;
			}

			chan->chan_stats.max_tx_queue_length = 1;
		}
		
		memcpy(&api_cmd->stats,&chan->chan_stats,sizeof(wanpipe_chan_stats_t));
		break;

	case WP_API_CMD_SS7_GET_CFG_STATUS:
        api_cmd->ss7_cfg_status.ss7_hw_enable = chan->cfg.ss7_enable;
        api_cmd->ss7_cfg_status.ss7_hw_mode = chan->cfg.ss7_mode;
        api_cmd->ss7_cfg_status.ss7_hw_lssu_size = chan->cfg.ss7_lssu_size;
        api_cmd->ss7_cfg_status.ss7_driver_repeat=chan->cfg.hdlc_repeat;
		break;

	case WP_API_CMD_RESET_STATS:
		memset(&chan->chan_stats,0,sizeof(wanpipe_chan_stats_t));
		memset(&chan->common.if_stats,0,sizeof(struct net_device_stats));
		break;

	case WP_API_CMD_SS7_FORCE_RX:
		if (chan->cfg.ss7_enable) {
			wan_spin_lock_irq(&card->wandev.lock, &smp_flags);
			aft_set_ss7_force_rx(card,chan);
			wan_spin_unlock_irq(&card->wandev.lock, &smp_flags);
		} else {
			err=-EINVAL;
		}
		break;

	case WP_API_CMD_DRIVER_VERSION:
		{
		wan_driver_version_t *drv_ver=(wan_driver_version_t*)&api_cmd->data[0];
		drv_ver->major=WANPIPE_VERSION_MAJOR;
		drv_ver->minor=WANPIPE_VERSION_MINOR;
		drv_ver->minor1=WANPIPE_VERSION_MINOR1;
		drv_ver->minor2=WANPIPE_VERSION_MINOR2;
		api_cmd->data_len=sizeof(wan_driver_version_t);
		}
		break;

	case WP_API_CMD_FIRMWARE_VERSION:
		api_cmd->data[0] = card->u.aft.firm_ver;
		api_cmd->data_len=1;
		break;

	case WP_API_CMD_CPLD_VERSION:
		api_cmd->data[0] = 0;
		api_cmd->data_len=1;
		break;
	
	case WP_API_CMD_GEN_FIFO_ERR_TX:
		DEBUG_EVENT("%s: Span %i enable TX fifo error !\n",
				card->devname, card->tdmv_conf.span_no);
		card->wp_debug_gen_fifo_err_tx=1;
        break;

	case WP_API_CMD_GEN_FIFO_ERR_RX:
		DEBUG_EVENT("%s: Span %i enable RX fifo error !\n",
				card->devname, card->tdmv_conf.span_no);
		card->wp_debug_gen_fifo_err_rx=1;
        break;

	case WP_API_CMD_START_CHAN_SEQ_DEBUG:
		DEBUG_EVENT("%s: Span %i channel sequence deugging enabled !\n",
				card->devname, card->tdmv_conf.span_no);
		card->wp_debug_chan_seq=1;
        break; 

	case WP_API_CMD_STOP_CHAN_SEQ_DEBUG:
		DEBUG_EVENT("%s: Span %i channel sequence deugging disabled !\n",
				card->devname, card->tdmv_conf.span_no);
		card->wp_debug_chan_seq=0;
        break;

	case WP_API_CMD_SET_IDLE_FLAG:
		if(chan->tx_idle_skb){
					
			chan->idle_flag = (unsigned char)api_cmd->idle_flag;

			memset(wan_skb_data(chan->tx_idle_skb), chan->idle_flag, wan_skb_len(chan->tx_idle_skb));

		}else{
			DEBUG_ERROR("%s: Error: WP_API_CMD_SET_IDLE_FLAG: tx_idle_skb is NULL!\n",
				chan->if_name);
		}
		break;

	case WP_API_CMD_FLUSH_BUFFERS:
		{
		netskb_t *skb;
		if (chan->tx_hdlc_rpt_on_close_skb) {
			skb=chan->tx_hdlc_rpt_on_close_skb;
			chan->tx_hdlc_rpt_on_close_skb=NULL;
			wan_spin_lock_irq(&card->wandev.lock, &smp_flags);
			wan_skb_queue_tail(&chan->wp_tx_hdlc_rpt_list,skb);
			wan_spin_unlock_irq(&card->wandev.lock, &smp_flags);
		}
		}
		break;             

	default:
		DEBUG_ERROR("%s: ERROR: driver_ctrl ioctl %i not supported\n",card->devname,cmd);
		api_cmd->result=SANG_STATUS_OPTION_NOT_SUPPORTED;
		err=-EOPNOTSUPP;
		break;
	}

	return err;
}


int aft_core_tdmapi_event_init(private_area_t *chan)
{

#if defined(AFT_TDM_API_SUPPORT)
	chan->wp_tdm_api_dev->event_ctrl		= aft_event_ctrl;
	chan->wp_tdm_api_dev->read_rbs_bits	= aft_read_rbs_bits;
	chan->wp_tdm_api_dev->write_rbs_bits	= aft_write_rbs_bits;
	chan->wp_tdm_api_dev->write_hdlc_frame	= aft_write_hdlc_frame;
	chan->wp_tdm_api_dev->write_hdlc_check  = aft_write_hdlc_check;
	chan->wp_tdm_api_dev->write_hdlc_timeout  = aft_write_hdlc_timeout;
	chan->wp_tdm_api_dev->pipemon		= wan_user_process_udp_mgmt_pkt;
	chan->wp_tdm_api_dev->driver_ctrl 	= aft_driver_ctrl;
#endif

	return 0;
}


int aft_core_api_event_init(sdla_t *card)
{

#if defined(AFT_API_SUPPORT)
	card->wandev.event_callback.tone	= wan_aft_api_tone;
	card->wandev.event_callback.hook	= wan_aft_api_hook;
	card->wandev.event_callback.ringtrip	= wan_aft_api_ringtrip;
	card->wandev.event_callback.ringdetect	= wan_aft_api_ringdetect;
#endif

	return 0;
}


