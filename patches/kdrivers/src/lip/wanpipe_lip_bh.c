#if defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__)
# include <wanpipe_lip.h>
#elif defined(__WINDOWS__)
# include "wanpipe_lip.h"
#else
# include <linux/wanpipe_lip.h>
#endif

#if defined(__LINUX__)
void wplip_link_bh(unsigned long data);
#elif defined(__WINDOWS__)
void wplip_link_bh(IN PKDPC Dpc, IN PVOID data, IN PVOID SystemArgument1, IN PVOID SystemArgument2);
#else
void wplip_link_bh(void* data, int pending);
#endif

static int wplip_bh_receive(wplip_link_t *lip_link)
{
	netskb_t *skb;
	int err;

	while((skb=wan_skb_dequeue(&lip_link->rx_queue)) != NULL){

		err=wplip_prot_rx(lip_link,skb);			
		if (err){
			wan_skb_free(skb);
		}
		
	}

	return 0;
}

static int wplip_bh_transmit(wplip_link_t *lip_link)
{
	netskb_t *skb;
	wplip_dev_t *lip_dev=NULL;
	int err=0;
	unsigned long timeout_cnt=SYSTEM_TICKS;
#if 0
	int tx_pkt_cnt=0;
#endif

	if (wan_test_bit(WPLIP_BH_AWAITING_KICK,&lip_link->tq_working)){
		if (wan_test_bit(WPLIP_KICK,&lip_link->tq_working)){
			DEBUG_TEST("%s: KICK but await still set!\n",
					lip_link->name);
		}else{
			return 0;
		}
	}

	wan_clear_bit(WPLIP_KICK,&lip_link->tq_working);
	wan_clear_bit(WPLIP_BH_AWAITING_KICK,&lip_link->tq_working);
	
	wan_clear_bit(WPLIP_MORE_LINK_TX,&lip_link->tq_working);
	
	while((skb=wan_skb_dequeue(&lip_link->tx_queue))){
		err=wplip_data_tx_down(lip_link,skb);
		if (err != 0){
			wan_skb_queue_head(&lip_link->tx_queue,skb);
			wan_set_bit(WPLIP_BH_AWAITING_KICK,&lip_link->tq_working);
			goto wplip_bh_link_transmit_exit;
		}

		if (SYSTEM_TICKS-timeout_cnt > 10){
			DEBUG_EVENT("%s: Link TxBH Time squeeze - link\n",lip_link->name);
			goto wplip_bh_link_transmit_exit;
		}
	}

	if ((lip_dev=lip_link->cur_tx) != NULL){

		if (lip_dev->magic != WPLIP_MAGIC_DEV){
			DEBUG_EVENT("%s: Error1:  Invalid Dev Magic dev=%p Magic=0x%lX\n",
					lip_link->name,
					lip_dev,
					lip_dev->magic);
			goto wplip_bh_transmit_exit;
		}

	}else{
		lip_dev=WAN_LIST_FIRST(&lip_link->list_head_ifdev);
		if (!lip_dev){
			goto wplip_bh_transmit_exit;
		}

		if (lip_dev->magic != WPLIP_MAGIC_DEV){
			DEBUG_EVENT("%s: Error2:  Invalid Dev Magic dev=%p Magic=0x%lX\n",
					lip_link->name,
					lip_dev,
					lip_dev->magic);
			goto wplip_bh_transmit_exit;
		}

		lip_link->cur_tx=lip_dev;
	}
	
	for (;;){

		if (SYSTEM_TICKS - timeout_cnt > 10){
			DEBUG_EVENT("%s: LipDev TxBH Time squeeze dev\n",lip_link->name);
			goto wplip_bh_transmit_exit;
		}

		if (wan_test_bit(WPLIP_DEV_UNREGISTER,&lip_dev->critical)){
			goto wplip_bh_transmit_skip;
		}
	
		skb=wan_skb_dequeue(&lip_dev->tx_queue);
		if (skb){
			err=wplip_data_tx_down(lip_link,skb);
			if (err != 0){
				wan_skb_queue_head(&lip_dev->tx_queue,skb);
				wan_set_bit(WPLIP_BH_AWAITING_KICK,&lip_link->tq_working);
				goto wplip_bh_transmit_exit;
			}

			/* Removed Tx statistics from here so that idle frames do not get counted */

		}

		if (!wan_test_bit(0,&lip_dev->if_down) && 
		    WAN_NETIF_QUEUE_STOPPED(lip_dev->common.dev)){
			if (lip_dev->common.usedby == API){
				DEBUG_TEST("%s: Api waking stack!\n",lip_dev->name);
				WAN_NETIF_START_QUEUE(lip_dev->common.dev);
#if defined(__LINUX__)
				wan_wakeup_api(lip_dev);
#elif defined(__WINDOWS__)
				WAN_NETIF_WAKE_QUEUE (lip_dev->common.dev);
#endif
			}else if (lip_dev->common.lip){ /*STACK*/
				WAN_NETIF_START_QUEUE(lip_dev->common.dev);
				wplip_kick(lip_dev->common.lip,0);
				
			}else{
				WAN_NETIF_WAKE_QUEUE (lip_dev->common.dev);
			}
		}
		
		wplip_prot_kick(lip_link,lip_dev);

		if (wan_skb_queue_len(&lip_dev->tx_queue)){
			wan_set_bit(WPLIP_MORE_LINK_TX,&lip_link->tq_working);
		}

wplip_bh_transmit_skip:

		lip_dev=WAN_LIST_NEXT(lip_dev,list_entry);
		if (lip_dev == NULL){
			lip_dev=WAN_LIST_FIRST(&lip_link->list_head_ifdev);
		}

		if (lip_dev == lip_link->cur_tx){
#if 0
			if (lip_dev->protocol == WANCONFIG_LIP_HDLC){
				if (!wan_test_bit(WPLIP_MORE_LINK_TX,&lip_link->tq_working)) {
					break;
				}
			} else {
				/* We went through the whole list */
				break;
			}

			if (SYSTEM_TICKS-timeout_cnt > 2){
				break;
			}
#else
			break;
#endif	
		}

	}

wplip_bh_transmit_exit:

	lip_link->cur_tx=lip_dev;

wplip_bh_link_transmit_exit:

	if (wan_skb_queue_len(&lip_link->tx_queue)){
		wan_set_bit(WPLIP_MORE_LINK_TX,&lip_link->tq_working);
	}

	if (wan_test_bit(WPLIP_MORE_LINK_TX,&lip_link->tq_working)){
		return 1;
	}
	
	return 0;

}


static int wplip_retrigger_bh(wplip_link_t *lip_link)
{
	if (wan_test_bit(WPLIP_MORE_LINK_TX, &lip_link->tq_working) ||
	    wan_skb_queue_len(&lip_link->rx_queue)){
		return wplip_trigger_bh(lip_link);
	}
#if 0
	if (gdbg_flag){
		DEBUG_EVENT("%s: Not triggered\n", __FUNCTION__);
	}
#endif
	return -EINVAL;	
}


#if defined(__LINUX__)
void wplip_link_bh(unsigned long data)
#elif defined(__WINDOWS__)
void wplip_link_bh(IN PKDPC Dpc, IN PVOID data, IN PVOID SystemArgument1, IN PVOID SystemArgument2)
#else
void wplip_link_bh(void* data, int pending)
#endif
{
	wplip_link_t *lip_link = (wplip_link_t *)data;
	wan_smp_flag_t	s;
	
	DEBUG_TEST("%s: Link BH\n",__FUNCTION__);

	if (wan_test_bit(WPLIP_LINK_DOWN,&lip_link->tq_working)){
		DEBUG_EVENT("%s: Link down in BH\n",__FUNCTION__);
		return;
	}

	wplip_spin_lock_irq(&lip_link->bh_lock, &s);

	wan_set_bit(WPLIP_BH_RUNNING,&lip_link->tq_working);
	
	wplip_bh_receive(lip_link);

	wplip_bh_transmit(lip_link);

	wan_clear_bit(WPLIP_BH_RUNNING,&lip_link->tq_working);

	WAN_TASKLET_END((&lip_link->task));

	wplip_spin_unlock_irq(&lip_link->bh_lock, &s);

	wplip_retrigger_bh(lip_link);

}

