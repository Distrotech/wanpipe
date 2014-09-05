
/* IMPLEMENT PRIVATE FUNCTIONS USED BY THE PROTOCOL */
#include "wanpipe_katm.h"

#if 0
#define WP_ATM_DEBUG 1
#warning "ATM DEBUG ENABLED"
#else
#undef WP_ATM_DEBUG 
#endif

int
wan_lip_katm_setsockopt(struct atm_vcc *vcc, int level, int optname,
		      void *optval, int optlen)
{
    wpabs_debug_event("%s:%d\n",__FUNCTION__,__LINE__);
    return -EINVAL;
}

int
wan_lip_katm_getsockopt(struct atm_vcc *vcc, int level, int optname,
		      void *optval, int optlen)
{
    wpabs_debug_event("%s:%d\n",__FUNCTION__,__LINE__);
    return -EINVAL;
}

/* This is not implemented yet, so I have it remarked out */
int
wan_lip_katm_sg_send(struct atm_vcc *vcc, unsigned long start,
		   unsigned long size)
{
	/* Scatter/Gather sending */
    wpabs_debug_event("%s:%d Size %lu %lu\n",__FUNCTION__,__LINE__,start,size);
    return vcc->qos.aal == ATM_AAL5 && !((start | size) & 3);
    /* don't tolerate misalignment */
}
/*=============================================================
  wan_lip_katm_close	

*/

static void wan_lip_katm_close_wait(wp_katm_channel_t *chan) 
{
	DECLARE_WAITQUEUE(wait,current);

	add_wait_queue(&chan->tx_wait,&wait);
	set_current_state(TASK_INTERRUPTIBLE);
	for (;;) {
	
		if (wan_skb_queue_len(&chan->tx_queue) == 0) {
			break;
		}
		
		DEBUG_EVENT("%d:%d: TX left = %i\n",
			chan->vpi,chan->vci,wan_skb_queue_len(&chan->tx_queue));
		
		if (signal_pending(current)) {
			DEBUG_EVENT("%d:%d: TX left = %i: Signal wakupe!\n",
				chan->vpi,chan->vci,wan_skb_queue_len(&chan->tx_queue));
                        break;
                }

		schedule();
		set_current_state(TASK_INTERRUPTIBLE);
	}
	set_current_state(TASK_RUNNING);
	remove_wait_queue(&chan->tx_wait,&wait);
}

void wan_lip_katm_close(struct atm_vcc *vcc)
{
	unsigned char tx_tclass = vcc->qos.txtp.traffic_class;
	unsigned char rx_tclass = vcc->qos.rxtp.traffic_class;
	wp_katm_t *atm_link = vcc->dev->dev_data; 
	wp_katm_channel_t *chan = vcc->dev_data;
	if (!chan) {
		wpabs_debug_event("%s:%d Error no device\n",__FUNCTION__,__LINE__);
		return;
	}
	
	
	
	wpabs_clear_bit(ATM_VF_READY,&vcc->flags);
	
	// Is there a receive interface open ?
	if (rx_tclass != ATM_NONE) {
		/* TODO: do any HW cleanup for turning off reception on this channel */	
		/* Also clean up any buffers that might bave been alloced for RX*/	
	}
	// Is there a transmit interface open ?
	if (tx_tclass != ATM_NONE) {
		/* TODO: do any HW cleanup for turning off transmission on this channel */
		/* Also clean up any buffers that might bave been alloced for TX*/
    	}
	
	wan_lip_katm_close_wait(chan);
	
	wpabs_debug_event("%d:%d:  %s %d Txq=%i\n", 
			chan->vpi,chan->vci,__FUNCTION__,__LINE__,
			wan_skb_queue_len(&chan->tx_queue));
	
	chan->atm_link=NULL;
	
	wan_set_bit(0,&chan->critical);
	
	wp_atm_close_chan(chan->sar_vcc);
	
	wp_unregister_atm_chan(chan->sar_vcc);
	chan->sar_vcc=NULL;
	
	wpkatm_remove_vccdev(atm_link,chan);
	
	wan_skb_queue_purge(&chan->tx_queue);
	
	vcc->dev_data = NULL;
	kfree(chan);

	wpabs_clear_bit(ATM_VF_ADDR,&vcc->flags);
}

/*=============================================================
  wp_katm_activate_channel	

*/

static int wp_katm_activate_channel(struct atm_vcc *vcc, wp_katm_channel_t * chan)
{
	unsigned char rx_traffic_class = vcc->qos.rxtp.traffic_class;
	unsigned char tx_traffic_class = vcc->qos.txtp.traffic_class;
	wan_atm_conf_if_t atm_cfg;
	int err;
	
	wp_katm_t *atm_link=vcc->dev->dev_data; //Get Hardware device pointer
	if (!atm_link) {
		wpabs_debug_event("%s:%d Error no device\n",__FUNCTION__,__LINE__);
		return -ENODEV;
	}

	wpabs_debug_event("%s:%d: %s\n",__FUNCTION__,__LINE__,atm_link->name);

	
	wpabs_memcpy(&atm_cfg,&atm_link->cfg,sizeof(wan_atm_conf_if_t));
	
	// Is a receive interface required ?
	if (rx_traffic_class != ATM_NONE) {
		//enable receive only interface
		//return on error otherwise
	}
	if (tx_traffic_class != ATM_NONE) {
		// Only currently support UBR and CBR tx classes
		if (!((tx_traffic_class == ATM_UBR) || (tx_traffic_class == ATM_CBR))) {
			printk(KERN_WARNING "tx traffic class not supported\n");
			return -EINVAL;
		}
		//enable tx channel
	}

	atm_cfg.vpi=chan->vpi;
	atm_cfg.vci=chan->vci;
	
	
	chan->sar_vcc = wp_register_atm_chan(chan, 
					     atm_link->sar_dev, 
					     atm_link->name,
					     &atm_cfg,
					     WPLIP_RAW);     
	if (!chan->sar_vcc) {
		wpabs_debug_event("%s:%d Error failed to register atm vcc\n",__FUNCTION__,__LINE__);
		return -EINVAL;
	}
	
	err=wp_atm_open_chan(chan->sar_vcc);
	if (err) {
		wpabs_debug_event("%s:%d Error failed to open atm vcc\n",__FUNCTION__,__LINE__);
		wp_unregister_atm_chan(chan->sar_vcc);
		return -EINVAL;
	}	
		     
	return 0;
}



/* RWM - After talking with the ATM stack maintainer, I have learned that
the 2nd and 3rd parameters are deprecated. We do not need to use
them here. All VCC's are kept in a kernel maintained hash list, and will be
passed in with the atm_vcc struct */
int wan_lip_katm_open(struct atm_vcc *vcc) //, short vpi, int vci)
{
	wp_katm_t *atm_link = vcc->dev->dev_data; //Device data is passed in
	wp_katm_channel_t *chan = NULL; //each new vcc chan data unallocated
	unsigned char rx_traffic_class = vcc->qos.rxtp.traffic_class;
	unsigned char tx_traffic_class = vcc->qos.txtp.traffic_class;
	int result;

	wpabs_debug_event("%s: %s %d AAL=%s\n", atm_link->name,__FUNCTION__,__LINE__,
			vcc->qos.aal == ATM_AAL0 ? "AAL0" : 
			(vcc->qos.aal == ATM_AAL5 ? "AAL5" : "OTHER") );
	
     /* Make sure we are opening an AAL0 or AAL5 connection */
	/* Though this level doesn't parse the AAL5 protocol */
     /* It is responsible for allowing that connection type to be established */
	if ((vcc->qos.aal != ATM_AAL0) && (vcc->qos.aal != ATM_AAL5))  {
		wpabs_debug_event("ATM_AALx was invalid for this driver\n");
		return -EINVAL;
	}

	/* Make sure traffic class is valid */
	if ((rx_traffic_class == ATM_NONE) && (tx_traffic_class == ATM_NONE)) {
        	wpabs_debug_event("rx and tx traffic class not specified\n");
        	return -EINVAL;
	}

	/* Set open bit in progress, this lets the stack know that the requested address
	is now in use. */
	wpabs_set_bit(ATM_VF_ADDR,&vcc->flags);
	
	/* TODO: Ok here we need to actually open the channel, or at least 
	manipulate hardware 'open'  and 'activate' a channel. This should 
	be comprised of allocated rx/tx buffers as needed, 
	based on the traffic classes, buffer descriptors */
	
	chan = kmalloc(sizeof(wp_katm_channel_t), GFP_KERNEL);
	if (!chan) {
		wpabs_clear_bit(ATM_VF_ADDR,&vcc->flags);
		wpabs_debug_event("Unable to allocate chan during open\n");
		return -ENOMEM;
	}
	wpabs_memset(chan,0,sizeof(wp_katm_channel_t));
	wpabs_memcpy(chan->name,atm_link->name,sizeof(chan->name));

	chan->aal = vcc->qos.aal;
	chan->vcc = vcc;
	
	chan->atm_link = atm_link;
	wpabs_skb_queue_init(&chan->tx_queue);
	init_waitqueue_head(&chan->tx_wait);
	
	/* ToDo: Need to assign all the values for chan here */

	vcc->dev_data = chan; /* save channel structure */

	/* Assign VPI and VCI in both atm_vcc and cpm_channel_t */
	chan->vpi = vcc->vpi; 
	chan->vci = vcc->vci; 
	wpabs_debug_event("RWM: Open vcc->vpi=%d : vcc->vci=%d\n", vcc->vpi, vcc->vci);

	if ((result = wp_katm_activate_channel(vcc, chan))) {
		//wp_katm_undo_activate_channel(vcc, chan);
		//could we just call close here instead?
		wan_lip_katm_close(vcc);
		return result;
	}

	wpkatm_insert_vccdev(atm_link,chan);

	/* Tells the stack that this VC is ready for Tx/Rx */
	wpabs_set_bit(ATM_VF_READY,&vcc->flags);

	return 0;		
}




int gcnt=0;

int wan_lip_katm_send(struct atm_vcc *vcc,struct sk_buff *skb)
{
	wp_katm_t *atm_link=vcc->dev->dev_data; //Get Hardware device pointer
	wp_katm_channel_t *chan= (wp_katm_channel_t*)vcc->dev_data; //channel pointer, not sure if we need this here.
	int err;
	struct sk_buff *nskb;

//	wpabs_debug_event("%s:%d Size %i %s\n",
//			__FUNCTION__,__LINE__,wpabs_skb_len(skb),
//			vcc->qos.aal == ATM_AAL0 ? "AAL0" : 
//			(vcc->qos.aal == ATM_AAL5 ? "AAL5" : "OTHER") );

//This for documentation only
//#define ATM_SKB(skb) (((struct atm_skb_data *) (skb)->cb))

#if 0 
	{
	unsigned char *data=wpabs_skb_data(skb);
	int i;
		for (i=0;i<wpabs_skb_len(skb);i++) {
			wpabs_debug_event("%d\n",data[i]);
		}
	}
#endif
	
	if (!atm_link) {
		wpabs_debug_event("%s:%d (%d:%d) No atm link\n",
				__FUNCTION__,__LINE__,chan->vpi,chan->vci);
		if (vcc->pop) vcc->pop(vcc,skb);
		else dev_kfree_skb(skb);
		return -1;
	}
	
	if (!skb) {
		wpabs_debug_event("%s:%d (%d:%d) !skb in eni_send ?\n",
				__FUNCTION__,__LINE__,chan->vpi,chan->vci);
		if (vcc->pop) vcc->pop(vcc,skb);
		else dev_kfree_skb(skb);
		return -EINVAL;
	}
	
	if (vcc->qos.aal == ATM_AAL0) {
		wpabs_debug_event("RWM: AAL0 send in progress?\n");
		if (skb->len != ATM_CELL_SIZE-1) {
			if (vcc->pop) vcc->pop(vcc,skb);
			else dev_kfree_skb(skb);
			return -EINVAL;
		} else {
			*(u32 *) skb->data = htonl(*(u32 *) skb->data);
		}
	}


	nskb=skb_clone(skb,GFP_KERNEL);
	if (!nskb) {
		 wpabs_debug_event("%s:%d (%d:%d) Error no memory !\n",
                                __FUNCTION__,__LINE__,chan->vpi,chan->vci);

		if (vcc->pop) vcc->pop(vcc,skb);
                else dev_kfree_skb(skb);
                return -ENOMEM;
	}

#ifdef WP_ATM_DEBUG	
#if 0
	if (chan->vpi == 0 && chan->vci==5) {
	++gcnt;
	if (gcnt > 5) {
		 wpabs_debug_event("%s:%d (%d:%d) Blocking !\n",
                                __FUNCTION__,__LINE__,chan->vpi,chan->vci);

		if (gcnt > 100) {
			gcnt=0;
		}
	
		if (vcc->pop) vcc->pop(vcc,skb);
                else dev_kfree_skb(skb);
                return -1;
	}
	}
#endif
#endif

	err = wp_atm_tx(chan->sar_vcc,nskb,0);

	if (err) {
		if (chan->vpi == 0 && chan->vci==5) {
		wpabs_debug_test("%s:%d (%d:%d) ATM Tx Failed qlen=%i\n",
				__FUNCTION__,__LINE__,chan->vpi,chan->vci,
				wan_skb_queue_len(&chan->tx_queue));
		}
		wpabs_skb_free(nskb);
		if (vcc->pop) vcc->pop(vcc,skb);
		else dev_kfree_skb(skb);
		
		wpkatm_priv_bh(atm_link);
		
		return -1;
	}
	
#ifdef WP_ATM_DEBUG	
	if (chan->vpi == 0 && chan->vci==5) {
	wpabs_debug_event("%s:%d (%d:%d) ATM Tx \n",
				__FUNCTION__,__LINE__,chan->vpi,chan->vci);
	}
#endif
		
	if (vcc->pop) vcc->pop(vcc,skb);
	else dev_kfree_skb(skb);
	
	wpkatm_priv_bh(atm_link);
	
//	wpabs_debug_event("%s:%d Send OK\n",
//			__FUNCTION__,__LINE__);
	
	return 0;
}

int wan_lip_katm_rx(wp_katm_channel_t *chan, struct sk_buff *skb)
{
	struct atm_vcc *vcc = chan->vcc;
	if (!vcc) {
		wpabs_debug_event("%s:%d %s, VPI:VCI=%d:%d\n",
                        __FUNCTION__,__LINE__, chan->name, chan->vpi, chan->vci);
		wpabs_skb_free(skb);
		return 0;
	}
	
#ifdef WP_ATM_DEBUG	
	if (vcc->vpi == 0 && vcc->vci==5) {
	wpabs_debug_event("%s:%d (%d:%d) ATM Rx \n",
				__FUNCTION__,__LINE__,chan->vpi,chan->vci);
	}
#endif

	
	if (vcc->qos.aal == ATM_AAL0) {
		wpabs_debug_event("%s: Received AAL0 packet!\n", __FUNCTION__);
		*(unsigned long *) skb->data =
				    ntohl(*(unsigned long *) skb->data);
	} 

	vcc->push(vcc,skb);	
	
	return 0;
}

//-------------------------------------------------------
// Private Callback Funcitons

int wplip_katm_link_prot_change_state (void *wplip_id,int state, unsigned char *data, int len)
{
	wp_katm_t *atm_link = (wp_katm_t *)wplip_id;
	if (!atm_link) {
		return 0;
	}

	if (atm_link->link_dev) {	
		atm_link->reg.prot_set_state(atm_link->link_dev,
				     state,
				     data,
				     len);

	} else {
		wpabs_debug_event("%s: %s %d: Error no link!\n",
                        atm_link->name,__FUNCTION__,__LINE__);
	}


	if (atm_link->dev) {
		atm_link->reg.chan_set_state(atm_link->dev,
                                     state,
                                     data,
                                     len);
	} else {
		wpabs_debug_event("%s: %s %d: Error no link dev!\n",
			atm_link->name,__FUNCTION__,__LINE__);

	}
	
	wpabs_debug_event("%s: %s %d\n",
			atm_link->name,__FUNCTION__,__LINE__);
	return 0;
}

int wplip_katm_lipdev_prot_change_state(void *wplip_id,int state, 
		                          unsigned char *data, int len)
{
	wp_katm_channel_t *chan = (wp_katm_channel_t *)wplip_id;
	wp_katm_t *atm_link = chan->atm_link;
	if (!atm_link) {
		return 0;
	}
	
	atm_link->reg.chan_set_state(atm_link->dev,
				     state,
				     data,
				     len);
	
	wpabs_debug_event("%s: %s %d\n",
			atm_link->name,__FUNCTION__,__LINE__);
	return 0;
}

int wplip_katm_link_callback_tx_down (void *wplink_id, void *skb)
{
	
	return 0;
}

#define KATM_MAX_Q 200
int wplip_katm_callback_tx_down (void *lip_dev_ptr, void *skb)
{
	wp_katm_channel_t *chan = (wp_katm_channel_t *)lip_dev_ptr;

	if (!chan->atm_link) {
		wpabs_debug_event("%s: %s %d ERROR no atm_link\n",
			chan->name,__FUNCTION__,__LINE__);
		if (skb) {
			wpabs_skb_free(skb);
		}
		return 0;
	}
	
	if (!skb){
		int free_space=KATM_MAX_Q - wan_skb_queue_len(&chan->tx_queue);	
		if (free_space < 0) {
			return 0;
		} else {
			return free_space;
		}    
	}

	if (wan_skb_queue_len(&chan->tx_queue) > KATM_MAX_Q){
		DEBUG_EVENT("%s: %s() Error  Tx queue full\n",
				chan->name,__FUNCTION__);
		return 1;
	}
	
	
	wan_skb_queue_tail(&chan->tx_queue,skb);
		
	return 0;
}

int wplip_katm_prot_rx_up (void *lip_dev_ptr, void *skb, int type)
{
	wp_katm_channel_t *chan = (wp_katm_channel_t *)lip_dev_ptr;
	wp_katm_t *atm_link = chan->atm_link;

	if (!atm_link || !type) { //Kill all non zero types for now, including OAM
		wpabs_debug_event("%s %d Killing skb type=%d len=%i\n",
			__FUNCTION__,__LINE__,type, wpabs_skb_len(skb));
		wpabs_skb_free(skb);
		return 0;
	}

//	wpabs_debug_event("%s %d len=%i\n",
//			__FUNCTION__,__LINE__,wpabs_skb_len(skb));

	wan_lip_katm_rx(chan,skb);
	return 0;
}



void wpkatm_insert_vccdev(wp_katm_t *atm_link, wp_katm_channel_t *atm_chan)
{
	unsigned long flags;
	wp_katm_channel_t *tmp;

	WP_WRITE_LOCK(&atm_link->dev_list_lock,flags);

        WAN_LIST_FOREACH(tmp, &atm_link->list_head_ifdev, list_entry){
                if (!WAN_LIST_NEXT(tmp, list_entry)){
                        break;
                }
        }
        if (tmp){
                WAN_LIST_INSERT_AFTER(tmp, atm_chan, list_entry);
        }else{
                WAN_LIST_INSERT_HEAD(&atm_link->list_head_ifdev, atm_chan, list_entry);

        }

	WAN_HOLD(atm_chan);
	atm_link->dev_cnt++;
	WP_WRITE_UNLOCK(&atm_link->dev_list_lock,flags);
}


void wpkatm_remove_vccdev(wp_katm_t *atm_link, wp_katm_channel_t *atm_chan)
{
	unsigned long flags;
	
	WP_WRITE_LOCK(&atm_link->dev_list_lock,flags);
	__WAN_PUT(atm_chan);
	WAN_LIST_REMOVE(atm_chan,list_entry);
	atm_link->cur_tx=NULL;
	atm_link->dev_cnt--;
	WP_WRITE_UNLOCK(&atm_link->dev_list_lock,flags);

	return;
}

