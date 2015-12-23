/*****************************************************************************
* wanpipe_lip_hdlc_iface.c	
*
* 		WANPIPE Protocol Template Module
*
* Author:	Nenad Corbic
*
* Copyright:	(c)2005 Sangoma Technologies Inc.
*
*		This program is free software; you can redistribute it and/or
*		modify it under the terms of the GNU General Public License
*		as published by the Free Software Foundation; either version
*		2 of the License, or (at your option) any later version.
* ============================================================================
*	This module:
*		This module is free software; you can redistribute it and/or
*		modify it under the terms of the GNU General Public License
*		as published by the Free Software Foundation; either version
*		2 of the License, or (at your option) any later version.
*/

#include "wanpipe_lip_hdlc.h"

extern void lip_hdlc_bs_handler (int fi, int len, unsigned char * p_rxbs, unsigned char * p_txbs);
extern int lip_hdlc_register    (void *, char *, int (*callback)(void*, unsigned char*, int));
extern int lip_hdlc_unregister  (int);
extern int lip_hdlc_facility_state_change (int fi, int state);

static int hdlc_data_available (struct wanpipe_hdlc_engine *hdlc_eng, void *data, int len);
static void wp_lip_hdlc_connect_indication(wp_lip_hdlc_t *prot, int reason);
static void wp_lip_hdlc_disconnect_indication(wp_lip_hdlc_t *prot, int reason);


int wp_lip_hdlc_tx(void *prot_ptr, void *skb, int type)
/********************************************************************************************/
{
	wp_lip_hdlc_t *prot = (wp_lip_hdlc_t *)prot_ptr;

	if (wpabs_skb_queue_len(prot->tx_q) > 32) {
		return 1;
	}

	wpabs_skb_queue_tail(prot->tx_q, skb);

	return 0;
}

/*======================================================
 * wp_lip_hdlc_data_transmit
 *
 * Transmitt the ppp packet to the lower layer,
 * vi its master (tx_dev) device.
 * 
 */
int wp_lip_hdlc_data_transmit(wp_lip_hdlc_t *prot, void *skb)
/********************************************************************************************/
{
	wpabs_debug_test("%s: %s() Len=%i\n",
			prot->name,__FUNCTION__,wpabs_skb_len(skb));

	if (prot->link_dev && prot->reg.tx_link_down){
		return prot->reg.tx_link_down(prot->link_dev,skb);
	}else{
		wpabs_debug_event("%s: Critical error link dev=%p tx_link_down=%p\n",
				__FUNCTION__,prot->link_dev,prot->reg.tx_link_down);
				
	}
	
	return 1;
}

void *wp_register_lip_hdlc_prot(void *link_ptr, 
		 	        char *devname, 
		                void *cfg_ptr, 
			        wplip_prot_reg_t *prot_reg)
/********************************************************************************************/
{
	wp_lip_hdlc_t *prot;

	wpabs_debug_test("%s: %s %d\n",
			devname,__FUNCTION__,__LINE__);

	if (!devname || !cfg_ptr || !prot_reg){
		wpabs_debug_event("%s:%d: Assert Error invalid args\n",
				__FUNCTION__,__LINE__);
		return NULL;
	}

	prot=wpabs_malloc(sizeof(wp_lip_hdlc_t));
	if (!prot){
		wpabs_debug_event( "lip_hdlc: Failed to create protocol %s\n",
				devname);
		return NULL;
	}


	/* Copy the LIP call back functions used to call
	 * the LIP layer */
	wpabs_memcpy(&prot->reg,prot_reg,sizeof(wplip_prot_reg_t));

	
	/* Initialize the LINK pointer used to call
	 * the LIP layer */
	prot->link_dev = link_ptr;

	/* INITIALIZE THE PROTOCOL HERE */

	FUNC_END();
		
	/* Pass the prot pointer to the LIP Layer, so
	 * that LIP can call us with it */
	
	return prot;
}

int wp_unregister_lip_hdlc_prot(void *prot_ptr)
/********************************************************************************************/
{	
	wp_lip_hdlc_t *prot =(wp_lip_hdlc_t*)prot_ptr;

	wpabs_debug_test("%s: %s %d\n",
			prot->name,__FUNCTION__,__LINE__);
	
	FUNC_BEGIN();
	prot->link_dev=NULL;
	
	/* UNREGISTER LINK HERE */
	
	
	/* Free the allocated Protocol structure */
	wp_dev_put(prot);

	FUNC_END();
	return 0;
}


void *wp_register_lip_hdlc_chan(void *if_ptr, 
			        void *prot_ptr, 
		                char *devname, 
			        void *cfg_ptr,
			        unsigned char type)
/********************************************************************************************/
{
	wp_lip_hdlc_t *prot =(wp_lip_hdlc_t*)prot_ptr;
	wan_lip_hdlc_if_conf_t *cfg = (wan_lip_hdlc_if_conf_t *)cfg_ptr;
// 	unsigned char data[100];
// 	int data_len=sizeof(data);
// 	unsigned char hdata[200];
// 	int hdata_len;
	
	wpabs_strncpy(prot->name,devname,sizeof(prot->name)-1);
	wpabs_memcpy(&prot->cfg,cfg,sizeof(wan_lip_hdlc_if_conf_t));

	wpabs_debug_test("%s: %s %d\n",
			prot->name,__FUNCTION__,__LINE__);

	wpabs_debug_event("%s: Registering HDLC Protocol (%i bit | %i crc)\n",
		devname,cfg->seven_bit_hdlc?7:8,cfg->rx_crc_bytes);

	prot->hdlc_eng = wanpipe_reg_hdlc_engine(cfg);
	if (!prot->hdlc_eng) {
		wpabs_debug_event("%s: Error: Failed to register HDLC Channel\n",
			prot->name);
		return NULL;
	}

	prot->tx_q = wpabs_skb_alloc_queue();
	if (!prot->tx_q) {
		wanpipe_unreg_hdlc_engine(prot->hdlc_eng);
		prot->hdlc_eng=NULL;
		return NULL;
	}

	wpabs_skb_queue_init(prot->tx_q);

	prot->hdlc_eng->prot_ptr = prot;
	prot->hdlc_eng->hdlc_data = &hdlc_data_available;
	prot->next_idle=0x7E;

	/* Note we are not allocating a channel device
	 * because this example is for non multiplexed 
	 * protocols. Thus keep using the prot device
	 * that is already registered */
	prot->dev = if_ptr;
	prot->type= type;

// 	wpabs_memset(data,0x55,sizeof(data));

// 	wanpipe_hdlc_encode(prot->hdlc_eng,
// 	       	data,data_len,
// 			hdata,&hdata_len, 
// 			&prot->next_idle);

	/* CONFIGURE/INITIALIZE THE PROTOCOL STRUCTURE */
	
	return prot;
}


int wp_unregister_lip_hdlc_chan(void *chan_ptr)
/********************************************************************************************/
{
	wp_lip_hdlc_t *prot =(wp_lip_hdlc_t*)chan_ptr;

	wpabs_debug_test("%s: %s %d\n",
			prot->name,__FUNCTION__,__LINE__);

	/* UNREGISTER CHAN FROM PROTOCOL */
	wpabs_skb_queue_purge(prot->tx_q);
	wanpipe_unreg_hdlc_engine(prot->hdlc_eng);

	prot->dev=NULL;

	return 0;
}


int wp_lip_hdlc_open(void *prot_ptr)
/********************************************************************************************/
{
	wp_lip_hdlc_t *prot =(wp_lip_hdlc_t*)prot_ptr;

	wpabs_debug_test("%s: %s %d\n",
			prot->name,__FUNCTION__,__LINE__);

	wp_lip_hdlc_connect_indication(prot,0);
	/* OPEN CONFIGURATION 
	 * (ifconfig up) 
	 * 
	 */
	
	return 0;
}

int wp_lip_hdlc_close(void *prot_ptr)
/********************************************************************************************/
{
	wp_lip_hdlc_t *prot =(wp_lip_hdlc_t*)prot_ptr;

	wpabs_debug_test("%s: %s %d\n",
			prot->name,__FUNCTION__,__LINE__);

	/* CLOSE CONFIGURATION 
	 * (ifconfig down) 
	 * 
	 */

	return 0;
}

static int wp_lip_tx_idle(wp_lip_hdlc_t *prot, void * p_rx_skb, int tx_out)
{
	int err;
	unsigned char *data = wpabs_skb_data(p_rx_skb);
	int len = wpabs_skb_len(p_rx_skb);

	if (tx_out > len) {
		wpabs_skb_free(p_rx_skb);
		return 0;
	}

	if (prot->cfg.lineidle == WANOPT_IDLE_MARK) {
		wpabs_memset(data, 0xFF, len);
	} else {
		wpabs_memset(data, prot->next_idle, len);
	}

	err=prot->reg.tx_chan_down(prot->dev,p_rx_skb);
	if (err) {
		wpabs_skb_free(p_rx_skb);
	}
	return 0;
}

/********************************************************************************************/
static int orig_len=0;
int wp_lip_hdlc_rx (void *prot_ptr, void * p_rx_skb)
{
	wp_lip_hdlc_t    *prot =(wp_lip_hdlc_t*)prot_ptr;
	int err;
	void * tx_skb;
	void * hdlc_skb;

	unsigned char *tx_data;
	unsigned char *tx_hdata;
	int tx_data_len;
	int tx_hdlc_len;

	int tx_out=0;

	unsigned char *p_rx_data;	
	int rx_skb_len = wpabs_skb_len (p_rx_skb);
 
	/* We received a packet; at this point we own this packet, 
         * we must either use 
	 * it or dealocate it. */
	p_rx_data = wpabs_skb_data (p_rx_skb);

	wanpipe_hdlc_decode(prot->hdlc_eng,p_rx_data,rx_skb_len);

	/*========================================================*/

	for (;;) {

		tx_skb=wpabs_skb_dequeue(prot->tx_q);
	
		if (!tx_skb) {
	
			wp_lip_tx_idle(prot,p_rx_skb, tx_out);
			break;
	
		} else {
	
			tx_data_len = wpabs_skb_len(tx_skb);
	
			hdlc_skb=wpabs_skb_alloc(tx_data_len*2);
	
			if (!hdlc_skb) {

				wpabs_skb_free(tx_skb);
				wp_lip_tx_idle(prot,p_rx_skb, tx_out);
				break;
				
			} else {

				tx_hdlc_len = (tx_data_len*2);
		
				tx_data=wpabs_skb_data(tx_skb);
				tx_hdata=wpabs_skb_data(hdlc_skb);
	
				wanpipe_hdlc_bkp(prot->hdlc_eng);
	
				err = wanpipe_hdlc_encode(prot->hdlc_eng,
							tx_data,tx_data_len,
							tx_hdata,&tx_hdlc_len,
							&prot->next_idle);
	
				if (err) {
					wanpipe_hdlc_restore(prot->hdlc_eng);
					prot->next_idle=prot->hdlc_eng->encoder.tx_flag_idle;
					wpabs_skb_queue_head(prot->tx_q,tx_skb);
	
					wpabs_skb_free(hdlc_skb);
					wp_lip_tx_idle(prot,p_rx_skb,tx_out);
					break;
				}
	#if 0
	if (orig_len == 0) {
		orig_len = tx_hdlc_len;
		wpabs_debug_event("(Txdata=%i) ORIG LEN = %i\n",tx_data_len,tx_hdlc_len);
	} else {
		if (orig_len != tx_hdlc_len) {
			wpabs_debug_event("ORIG LEN %i != %i (txdata=%i)\n",orig_len,tx_hdlc_len,tx_data_len);
		}
	}
	#endif
	
				wpabs_skb_put(hdlc_skb,tx_hdlc_len);
	
				err=prot->reg.tx_chan_down(prot->dev,hdlc_skb);
				if (err != 0) {
					wanpipe_hdlc_restore(prot->hdlc_eng);
					prot->next_idle=prot->hdlc_eng->encoder.tx_flag_idle;
	
					wpabs_skb_queue_head(prot->tx_q,tx_skb);
					wpabs_skb_free(hdlc_skb);
					wpabs_skb_free(p_rx_skb);
					break;
				} 
				
				/* Tx Frame was transmitted try to
				 * transmit another one */
				wpabs_skb_free(tx_skb);
				tx_out += tx_hdlc_len;
				
			}
	
		}	

	}

	FUNC_END();
	return 0;
}


int wp_lip_hdlc_timer(void *prot_ptr, unsigned int *timeout, unsigned int carrier_reliable)
/********************************************************************************************/
{
	return 0;
}


int wp_lip_hdlc_pipemon(void *prot_ptr, int cmd, int addr, 
		    unsigned char* data, unsigned int *len)
/********************************************************************************************/
{	
	wp_lip_hdlc_t *prot = (wp_lip_hdlc_t *)prot_ptr;

	prot = prot;

	*len=1;

	/* IMPLEMENT PIPEMON COMMANDS */

	return -1;
}

/*======================================================
 * wp_lip_hdlc_data_indication
 *
 * Transmitt the lapb packet to the lower layer,
 * vi its master (tx_dev) device.
 * 
 */
int wp_lip_hdlc_data_indication(wp_lip_hdlc_t *prot, void *skb)
/********************************************************************************************/
{
	wpabs_debug_test("%s: %s() Len=%i\n",
			prot->name,__FUNCTION__,wpabs_skb_len(skb));

	if (prot->dev){
		return prot->reg.rx_up(prot->dev,skb,prot->type);
	}

	return 1;
}



/**********************************************************
 * Private Functions
 **********************************************************/


/*======================================================
 * prot_connect_indication
 *
 * Indicate to the upper layer (x25) that the
 * lapb is layer is connected and that it should
 * start its connection process.
 */
static void wp_lip_hdlc_connect_indication(wp_lip_hdlc_t *prot, int reason)
/********************************************************************************************/
{
	wpabs_debug_test("%s: %s() Reason=%i\n",
			prot->name,__FUNCTION__,reason);

	if (prot->dev && prot->reg.chan_set_state){
		prot->reg.chan_set_state(prot->dev,WAN_CONNECTED,NULL, 0);
	}
}


/*======================================================
 * prot_disconnect_indication
 *
 * Indicate to the upper layer (x25) that the
 * lapb is layer is disconnected and that it should
 * drop the connection.
 */
static void wp_lip_hdlc_disconnect_indication(wp_lip_hdlc_t *prot, int reason)
/********************************************************************************************/
{
	wpabs_debug_test("%s: %s() Reason=%i\n",
			prot->name,__FUNCTION__,reason);

	if (prot->dev && prot->reg.chan_set_state){
		prot->reg.chan_set_state(prot->dev,WAN_DISCONNECTED,NULL,0);
	}
}


static int hdlc_data_available (struct wanpipe_hdlc_engine *hdlc_eng, void *data, int len)
{
	void 	      *p_decoded_skb;
	unsigned char *p_decoded_data; 
	wp_lip_hdlc_t *prot; 

	prot = (wp_lip_hdlc_t*) hdlc_eng->prot_ptr;

	if (!prot) {
		return -1;
	}

	if (len <= 2) {
		return -1;
	}

	if (prot->cfg.rx_crc_bytes == 0) {
		/* Remove CRC */
		len = len-2;
	}

	p_decoded_skb = wpabs_skb_alloc(len+128);
	if (!p_decoded_skb) {
		return -1;
	}	
	
	p_decoded_data =  wpabs_skb_put(p_decoded_skb,len);
	
	wpabs_memcpy(p_decoded_data,data,len);
	
	wp_lip_hdlc_data_indication(prot,p_decoded_skb);

	return 0;

}


