/*****************************************************************************
* af_wanpipe_datascope.c
* 		
* 		WANPIPE(tm) Datascope Socket Layer.
*
* Author:	Nenad Corbic	<ncorbic@sangoma.com>
*
* Copyright:	(c) 2003 Sangoma Technologies Inc.
*
*		This program is free software; you can redistribute it and/or
*		modify it under the terms of the GNU General Public License
*		as published by the Free Software Foundation; either version
*		2 of the License, or (at your option) any later version.
* ============================================================================
*/

#include <linux/wanpipe_includes.h>
#include <linux/wanpipe_defines.h>
#include <linux/wanpipe.h>
#include <linux/if_wanpipe.h>
#include <linux/if_wanpipe_common.h>
#include <linux/if_wanpipe_kernel.h>

/*===================================================================
 * 
 * DEFINES
 * 
 *==================================================================*/

#define INC_CRC_CNT(a)   if (++a >= MAX_SOCK_CRC_QUEUE) a=0;
#define GET_FIN_CRC_CNT(a)  { if (--a < 0) a=MAX_SOCK_CRC_QUEUE-1; \
		              if (--a < 0) a=MAX_SOCK_CRC_QUEUE-1; }

#define FLIP_CRC(a,b)  { b=0; \
			 b |= (a&0x000F)<<12 ; \
			 b |= (a&0x00F0) << 4; \
			 b |= (a&0x0F00) >> 4; \
			 b |= (a&0xF000) >> 12; }

#define DECODE_CRC(a) { a=( (((~a)&0x000F)<<4) | \
		            (((~a)&0x00F0)>>4) | \
			    (((~a)&0x0F00)<<4) | \
			    (((~a)&0xF000)>>4) ); }
#define BITSINBYTE 8

#define NO_FLAG 	0
#define OPEN_FLAG 	1
#define CLOSING_FLAG 	2


/*===================================================================
 * 
 * GLOBAL VARIABLES
 * 
 *==================================================================*/

static const int MagicNums[8] = { 0x1189, 0x2312, 0x4624, 0x8C48, 0x1081, 0x2102, 0x4204, 0x8408 };
static unsigned short CRC_TABLE[256];
static unsigned long init_crc_g=0;
static const char FLAG[]={ 0x7E, 0xFC, 0xF9, 0xF3, 0xE7, 0xCF, 0x9F, 0x3F };


/*===================================================================
 * 
 * FUNCTION PROTOTYPES
 * 
 *==================================================================*/

#ifdef LINUX_2_6
HLIST_HEAD(wanpipe_parent_sklist);
#else
struct sock * wanpipe_parent_sklist = NULL;
#endif
extern rwlock_t wanpipe_parent_sklist_lock;

static int wanpipe_check_prot_options(struct sock *sk);
void wanpipe_unbind_sk_from_parent(struct sock *sk);
static int wanpipe_attach_sk_to_parent (struct sock *parent_sk, struct sock *sk);
static int wanpipe_unattach_sk_from_parent (struct sock *parent_sk, struct sock *sk);
static wanpipe_hdlc_engine_t *
	wanpipe_get_hdlc_engine (struct sock *parent_sk,unsigned long active_ch);
static void free_hdlc_engine(wanpipe_hdlc_engine_t *hdlc_eng);
	
static int bind_sk_to_parent_prot(void **,unsigned char*,struct sock *sk);
static int unbind_sk_from_parent_prot(void **, unsigned char *,struct sock *sk);
static int bind_hdlc_into_timeslots(struct sock *parent_sk,
		                     wanpipe_hdlc_engine_t *hdlc_eng);
static void unbind_hdlc_from_timeslots(struct sock *parent_sk, wanpipe_hdlc_engine_t *hdlc_eng);

static int decode_byte (wanpipe_hdlc_engine_t *hdlc_eng, 
		        wanpipe_hdlc_decoder_t *chan,
			unsigned char *byte,
			unsigned char txdir);
static void init_hdlc_decoder(wanpipe_hdlc_decoder_t *chan);

static void init_crc(void);
static void calc_rx_crc(wanpipe_hdlc_decoder_t *chan);
static void tx_up_decode_pkt(wanpipe_hdlc_engine_t *,wanpipe_hdlc_decoder_t *,unsigned char);
static int wanpipe_hdlc_decode (wanpipe_hdlc_engine_t *hdlc_eng, unsigned char, struct sk_buff *skb);

static int wanpipe_sk_ss7_monitor_rx(void**map, struct sk_buff **skb_ptr);
static int wanpipe_skb_timeslot_parse(struct sock *parent_sk, struct sk_buff *skb, unsigned char dir);
static int wanpipe_handle_hdlc_data(struct sock *parent_sk);
static int wanpipe_rx_prot_data(void **map, unsigned short protocol, struct sk_buff *skb);
//static void print_hdlc_list(wanpipe_hdlc_engine_t **list);
static void wanpipe_free_parent_sock(struct sock *sk);

static void wanpipe_sk_parent_rx_bh (unsigned long data);

extern struct sock *wanpipe_make_new(struct sock *);
extern atomic_t wanpipe_socks_nr;




/*============================================================
 *  wanpipe_bind_sk_to_parent
 *
 *===========================================================*/

int wanpipe_bind_sk_to_parent(struct sock *sk, netdevice_t *dev, struct wan_sockaddr_ll *sll)
{
	int err=0;
	struct sock *parent_sk;
	unsigned long flags;
	struct ifreq ifr;
	void *buf;

	memset(&ifr,0,sizeof(struct ifreq));

	if (sll->sll_protocol){
		SK_PRIV(sk)->num = sll->sll_protocol;
	}

	buf = wan_malloc(sizeof(wanpipe_datascope_t));
	if (!buf){
		DEBUG_EVENT("%s: Wanpipe Socket failed to allocate datascope struct!\n",
				dev->name);
		return -ENOMEM;
	}

	DATA_SC_INIT(sk,buf);

	memset(DATA_SC(sk),0,sizeof(wanpipe_datascope_t));

	DATA_SC(sk)->prot_type = sll->sll_prot;
	DATA_SC(sk)->active_ch = sll->sll_active_ch;

	/* Find the parent socket for this interface */
	parent_sk=NULL;
	write_lock_irqsave(&wanpipe_parent_sklist_lock,flags);
#ifdef LINUX_2_6
	{
#if KERN_SK_FOR_NODE_FEATURE == 0
	sk_for_each(parent_sk, &wanpipe_parent_sklist) {
#else
	struct hlist_node *node;
	sk_for_each(parent_sk, node, &wanpipe_parent_sklist) {
#endif
		if (SK_PRIV((parent_sk))->dev == dev) {
			break;
		}
	}
	}
#else
	{
	struct sock **skp;	
	for (skp = &wanpipe_parent_sklist; *skp; skp = &(*skp)->next) {
		if (SK_PRIV((*skp))->dev == dev) {
			parent_sk=*skp;
			break;
		}
	}
	}
#endif
	write_unlock_irqrestore(&wanpipe_parent_sklist_lock,flags);

	DEBUG_EVENT("%s: Dev name %s parensk=%p\n",__FUNCTION__,dev->name, parent_sk);
	
	if (parent_sk == NULL){
	
		init_crc();

		/* Device is not being used
		 *
		 * Create a parent socket that will be used
		 * to interface to network device */
		
		parent_sk=wanpipe_make_new(sk);	
		if (!parent_sk){
			DEBUG_EVENT("wansock: Failed to allocate new ss7 monitor sock!\n");
			return -ENOMEM;
		}	
		atomic_inc(&wanpipe_socks_nr);

		buf = wan_malloc(sizeof(wanpipe_datascope_t));
		if (!buf){
			DEBUG_EVENT("%s: Wanpipe Socket failed to allocate datascope struct!\n",
					dev->name);
			wanpipe_free_parent_sock(parent_sk);
			return -ENOMEM;
		}

		DATA_SC_INIT(parent_sk,buf);
		memset(DATA_SC(parent_sk),0,sizeof(wanpipe_datascope_t));

		buf=wan_malloc(sizeof(wanpipe_parent_t));
		if (!buf){
			DEBUG_EVENT("%s: Wanpipe Socket failed to allocate parent struct!\n",
					dev->name);
			wan_free(DATA_SC(parent_sk));
			wanpipe_free_parent_sock(parent_sk);
			return -ENOMEM;
		}

		PPRIV_INIT(parent_sk,buf);
		memset(PPRIV(parent_sk),0,sizeof(wanpipe_parent_t));

		wan_rwlock_init(&PPRIV(parent_sk)->lock);

		SK_PRIV(parent_sk)->dev=dev;

		tasklet_init((&PPRIV(parent_sk)->rx_task), 
			      wanpipe_sk_parent_rx_bh, (unsigned long)parent_sk);
		skb_queue_head_init(&PPRIV(parent_sk)->rx_queue);
		
		wansk_reset_zapped(parent_sk);
		DATA_SC(parent_sk)->parent=1;

		if (DATA_SC(sk)->active_ch){
			PPRIV(parent_sk)->hdlc_enabled=1;
		}
		
		/* Bind the parent socket to network device */
		
		ifr.ifr_data = (void*)parent_sk;
		err=-EINVAL;
		if (WAN_NETDEV_TEST_IOCTL(dev))
			err=WAN_NETDEV_IOCTL(dev,&ifr,SIOC_WANPIPE_BIND_SK);

		if (err != 0){
			DEBUG_EVENT("%s: Error: Dev busy with another protocol!\n",
					dev->name);
			wanpipe_free_parent_sock(parent_sk);
			return err;
		}	

		
		DEBUG_EVENT("%s: Parent sock registered to dev %s\n",
					dev->name,dev->name);

		PPRIV(parent_sk)->time_slots = 24;
		PPRIV(parent_sk)->media = WAN_MEDIA_T1;
		PPRIV(parent_sk)->seven_bit_hdlc = sll->sll_seven_bit_hdlc;

		DEBUG_EVENT("%s: Parent hdlc engine %s  Bits %d\n",
					dev->name,
					PPRIV(parent_sk)->hdlc_enabled?
						"Enabled":"Disabled",
					PPRIV(parent_sk)->seven_bit_hdlc?7:8);

		PPRIV(parent_sk)->time_slots = 
		                      WAN_NETDEV_IOCTL(dev,NULL,SIOC_WANPIPE_GET_TIME_SLOTS);
		if (PPRIV(parent_sk)->time_slots < 0){
			DEBUG_EVENT("%s: Error, failed to obtain time slots from driver!\n",
					dev->name);
			wanpipe_free_parent_sock(parent_sk);
			return -EINVAL;
		}
		
		PPRIV(parent_sk)->media      =
		                      WAN_NETDEV_IOCTL(dev,NULL,SIOC_WANPIPE_GET_MEDIA_TYPE);
		if (PPRIV(parent_sk)->media < 0){
			DEBUG_EVENT("%s: Error, failed to obtain media type from driver!\n",
					dev->name);
			wanpipe_free_parent_sock(parent_sk);
			return -EINVAL;
		}

		if (PPRIV(parent_sk)->media==WAN_MEDIA_NONE){
			PPRIV(parent_sk)->time_slots=0;
			PPRIV(parent_sk)->hdlc_enabled=1;
			DEBUG_EVENT("%s: Parent connected to Serial Device!\n",
					dev->name);
		}else{
			DEBUG_EVENT("%s: Parent connected to %s Timeslots=%i!\n",
					dev->name,
					PPRIV(parent_sk)->media==WAN_MEDIA_T1?"T1":"E1",
					PPRIV(parent_sk)->time_slots);
		}

		DEBUG_EVENT("%s: Parent hdlc engine %s  Bits %d\n",
					dev->name,
					PPRIV(parent_sk)->hdlc_enabled?
						"Enabled":"Disabled",
					PPRIV(parent_sk)->seven_bit_hdlc?7:8);
		
		/* Insert parent socket into global parent socket list */
		write_lock_irqsave(&wanpipe_parent_sklist_lock,flags);
#ifdef LINUX_2_6
		sk_add_node(parent_sk,&wanpipe_parent_sklist);
#else
		parent_sk->next = wanpipe_parent_sklist;
		wanpipe_parent_sklist = parent_sk;
		wp_sock_hold(parent_sk);
#endif
		write_unlock_irqrestore(&wanpipe_parent_sklist_lock,flags);

		err=WAN_NETDEV_IOCTL(dev,&ifr,SIOC_WANPIPE_DEV_STATE);
		if (err == WANSOCK_CONNECTED){
			parent_sk->sk_state = WANSOCK_CONNECTED;
		}else{
			parent_sk->sk_state = WANSOCK_DISCONNECTED;
		}
		
		parent_sk->sk_bound_dev_if = dev->ifindex; 
		SK_PRIV(parent_sk)->dev = dev;
		wansk_set_zapped(parent_sk);

	}else{
		if (SK_PRIV(parent_sk)->num != SK_PRIV(sk)->num){
			DEBUG_EVENT("%s: Error parent prot 0x%X doesn't match sk prot 0x%X!\n",
					dev->name,SK_PRIV(parent_sk)->num,SK_PRIV(sk)->num);
			return -EFAULT;
		}
	}

	/* Initialize the sk parameters */
	DATA_SC(sk)->parent_sk=parent_sk;
	wp_sock_hold(parent_sk);

	if (PPRIV(parent_sk)->hdlc_enabled){
		
		if (PPRIV(parent_sk)->media != WAN_MEDIA_NONE && !DATA_SC(sk)->active_ch){
			DEBUG_EVENT("%s: Error invalid sk active ch selection: 0x%X!\n",
						dev->name,DATA_SC(sk)->active_ch);
			DEBUG_EVENT("%s: Parent hdlc engine enabled!\n",
						dev->name);
			wanpipe_unbind_sk_from_parent(sk);
			return -EINVAL;
		}
		if (PPRIV(parent_sk)->media == WAN_MEDIA_NONE){
			DATA_SC(sk)->active_ch=0;
		}
	}

	if (!PPRIV(parent_sk)->hdlc_enabled && DATA_SC(sk)->active_ch){
		DEBUG_EVENT("%s: Error invalid sk active ch selection: 0x%X!\n",
					dev->name,DATA_SC(sk)->active_ch);
		DEBUG_EVENT("%s: Parent hdlc engine disabled!\n",
					dev->name);
		wanpipe_unbind_sk_from_parent(sk);
		return -EINVAL;
	}


	DATA_SC(sk)->delta= sll->sll_prot_opt & DELTA_PROT_OPT;
	if (DATA_SC(sk)->delta){
		DEBUG_EVENT("%s: Datascope enabling DELTA parsing\n",dev->name);
	}
	
	if (sll->sll_prot_opt & MULTI_PROT_OPT){
		DATA_SC(sk)->max_mult_cnt=sll->sll_mult_cnt;
		DEBUG_EVENT("%s: Datascope enabling MULTI parsing: Max mult cnt = %i\n",
				dev->name,DATA_SC(sk)->max_mult_cnt);
	}

	if (wanpipe_check_prot_options(sk) != 0){
		DEBUG_EVENT("%s: Error invalid sk protocol specified: 0x%lX!\n",
					dev->name,DATA_SC(sk)->prot_type);
		wanpipe_unbind_sk_from_parent(sk);
		return -EINVAL;
	}
	

	err=wanpipe_attach_sk_to_parent(parent_sk,sk);
	if (err!=0){
		wanpipe_unbind_sk_from_parent(sk);
		return err;
	}

	SK_PRIV(sk)->dev=dev;
	sk->sk_bound_dev_if = dev->ifindex;
	sk->sk_state = parent_sk->sk_state;
	wansk_set_zapped(sk);

	return 0;
}

/*============================================================
 *  wanpipe_unbind_sk_from_parent
 *
 *===========================================================*/

void wanpipe_unbind_sk_from_parent(struct sock *sk)
{
	
	struct sock *parent_sk;
	unsigned long flags;
	int sk_child_exists=0;

	if (!DATA_SC(sk)){
		return;
	}
	
	parent_sk=DATA_SC(sk)->parent_sk;
	if (!parent_sk){
		wan_free(DATA_SC(sk));
		DATA_SC_INIT(sk,NULL);
		return;
	}	

	sk_child_exists = wanpipe_unattach_sk_from_parent(parent_sk,sk);

	/* If this is the last socket in the parents list
	 * remove the parent as well */
	if (!sk_child_exists){

#ifdef LINUX_2_6
		sock_set_flag(parent_sk, SOCK_DEAD);
#else
		set_bit(0,&parent_sk->dead);
#endif
		tasklet_kill(&PPRIV(parent_sk)->rx_task);
		skb_queue_purge(&PPRIV(parent_sk)->rx_queue);
		
		DEBUG_EVENT("%s: Parent sock unregistered from dev %s\n",
				SK_PRIV(parent_sk)->dev->name,
				SK_PRIV(parent_sk)->dev->name);

		/* Remove parent from the global parent list */
		write_lock_irqsave(&wanpipe_parent_sklist_lock,flags);
#ifdef LINUX_2_6
		sk_del_node_init(parent_sk);
#else
		{
		struct sock **skp;
		for (skp = &wanpipe_parent_sklist; *skp; skp = &(*skp)->next) {
			if (*skp == parent_sk) {
				*skp = parent_sk->next;
				wp_sock_put(parent_sk);
				break;
			}
		}
		}
#endif
		wanpipe_free_parent_sock(parent_sk);	
		parent_sk=NULL;

		write_unlock_irqrestore(&wanpipe_parent_sklist_lock,flags);

	}

	wansk_reset_zapped(sk);
	wan_free(DATA_SC(sk));
	DATA_SC_INIT(sk,NULL);
	
	return;
}


void wanpipe_wakup_sk_attached_to_parent(struct sock *parent_sk)
{
	struct sock *sk;
	int i;

	if (!DATA_SC(parent_sk) || !DATA_SC(parent_sk)->parent){
		return;
	}

	for (i=0;i<MAX_PARENT_PROT_NUM;i++){
		if ((sk=PPRIV(parent_sk)->sk_to_prot_map[i]) != NULL){
			sk->sk_state = parent_sk->sk_state;
			SK_DATA_READY(sk, 0);
		}
	}
}

static void wanpipe_sk_parent_rx_bh (unsigned long data)
{
	struct sock *parent_sk = (struct sock *)data;
	struct sk_buff *skb;
	unsigned char txdir=0;
	int err;

#ifdef LINUX_2_6
	if (sock_flag(parent_sk, SOCK_DEAD)){
#else
	if (test_bit(0,&parent_sk->dead)){
#endif
		return;
	}
	
	read_lock(&PPRIV(parent_sk)->lock);

	while ((skb=skb_dequeue(&PPRIV(parent_sk)->rx_queue))){
#ifdef LINUX_2_6
		if (sock_flag(parent_sk, SOCK_DEAD)){
#else		
		if (test_bit(0,&parent_sk->dead)){
#endif
			read_unlock(&PPRIV(parent_sk)->lock);
			return;
		}

		/* Protocol specific here */
		if (PPRIV(parent_sk)->hdlc_enabled){
			
			wp_sock_rx_hdr_t *hdr=(wp_sock_rx_hdr_t *)skb->data;
			txdir=hdr->direction;

			skb_pull(skb,sizeof(wp_sock_rx_hdr_t));
			
			if (wanpipe_skb_timeslot_parse(parent_sk,skb,txdir)){
				/* There is data to be passed up
				 * check all hdlc engines */
				wanpipe_handle_hdlc_data(parent_sk);
			}
			wan_skb_free(skb);
			err = 0;
		}else{
			err = wanpipe_rx_prot_data(PPRIV(parent_sk)->sk_to_prot_map,
						    SK_PRIV(parent_sk)->num,
						    skb);
		}
	}
	
	read_unlock(&PPRIV(parent_sk)->lock);

	return;
}


int wanpipe_sk_parent_rx(struct sock *parent_sk, struct sk_buff *skb)
{
	unsigned char txdir=0;
	int err;
	
	if (skb->len <= WANPIPE_HEADER_SZ){
		wan_skb_free(skb);
		return 0;
	}

	if (!PPRIV(parent_sk)){
		wan_skb_free(skb);
		return 0;
	}

#ifdef LINUX_2_6
	if (sock_flag(parent_sk, SOCK_DEAD)){
#else
	if (test_bit(0,&parent_sk->dead)){
#endif
		wan_skb_free(skb);
		return 0;
	}

	skb_queue_tail(&PPRIV(parent_sk)->rx_queue,skb);
	tasklet_hi_schedule(&PPRIV(parent_sk)->rx_task);

	return 0;
	
	read_lock(&PPRIV(parent_sk)->lock);
	
	/* Protocol specific here */
	if (PPRIV(parent_sk)->hdlc_enabled){
		
		wp_sock_rx_hdr_t *hdr=(wp_sock_rx_hdr_t *)skb->data;
		txdir=hdr->direction;

		skb_pull(skb,sizeof(wp_sock_rx_hdr_t));
		
		if (wanpipe_skb_timeslot_parse(parent_sk,skb,txdir)){
			/* There is data to be passed up
			 * check all hdlc engines */
			wanpipe_handle_hdlc_data(parent_sk);
		}
		wan_skb_free(skb);
		err = 0;
	}else{
		err = wanpipe_rx_prot_data(PPRIV(parent_sk)->sk_to_prot_map,
				            SK_PRIV(parent_sk)->num,
					    skb);
	}
	
	read_unlock(&PPRIV(parent_sk)->lock);
	
	return err;
}


static int wanpipe_rx_prot_data(void **map, unsigned short protocol, struct sk_buff *skb)
{
	int sk_id=MAX_PARENT_PROT_NUM;
	struct sock *sk;
	int err;
	
	/* Protocol specific here */

	if (protocol == htons(SS7_MONITOR_PROT)){
		sk_id=wanpipe_sk_ss7_monitor_rx(map,&skb);
	}else{
		if (net_ratelimit()){
			DEBUG_EVENT("%s: Invalid sk prot configuration 0x%X\n",__FUNCTION__,
				protocol);
		}
		wan_skb_free(skb);
		return 0;
	}

	if (sk_id < 0){
		/* The packet was intentionaly dropped by
		 * the protocol layer.  The packet was
		 * handled by the protocol layer thus,
		 * no deallocation is needed */
		return 0;
	}
	
	if (sk_id >= MAX_PARENT_PROT_NUM){
		if (net_ratelimit()){
			DEBUG_EVENT("%s: Invalid skb pkt len %i id=%i\n",
					__FUNCTION__,
					skb->len,sk_id);
		}
		wan_skb_free(skb);
		return 0;
	}
	
	if ((sk=map[sk_id]) == NULL){
		if (net_ratelimit()){
			DEBUG_EVENT("%s: No sk for prot %s\n",
					__FUNCTION__,
					DECODE_SS7_PROT(sk_id+1));
		}
		wan_skb_free(skb);
		return 0;
	}

	if ((err=sock_queue_rcv_skb(sk,skb))<0){
		SK_DATA_READY(sk, 0);
		if (net_ratelimit()){
			DEBUG_EVENT("%s: Sock failed to rx on prot %s: sock full: err %i truesize %i sktotal %i skblen %i!\n",
					__FUNCTION__,
					DECODE_SS7_PROT(DATA_SC(sk)->prot_type),
					err,skb->truesize,
					sk->sk_rcvbuf,
					skb->len);
		}

		//FIXME: skb ptr can be mangled at this poing
		//       we should never get here
		kfree(skb);
		return 0;
	}

	return 0;
}

static int wanpipe_handle_hdlc_data(struct sock *parent_sk)
{
	wanpipe_hdlc_engine_t *hdlc_eng;	
	struct sk_buff *skb;
	
	for (hdlc_eng = PPRIV(parent_sk)->hdlc_eng_list;
	     hdlc_eng;
	     hdlc_eng=hdlc_eng->next){
		while ((skb=wan_skb_dequeue(&hdlc_eng->data_q)) != NULL){
			wanpipe_rx_prot_data(hdlc_eng->sk_to_prot_map,
				            SK_PRIV(parent_sk)->num,
					    skb);
		}
	}

	return 0;
}

/*******************************************************************
 *
 * PRIVATE FUNCTIONS
 *
 *******************************************************************/


static inline unsigned short wanpipe_skb_get_checksum(struct sk_buff *skb)
{
	return *((unsigned short*)&skb->data[skb->len-2]);
}

static int wanpipe_skb_mult_check(struct sk_buff **orig_skb, 
	                          unsigned short *orig_csum, 
	 	                  struct sk_buff **up_skb, 
				  unsigned short max_mult_cnt)
{
	unsigned short csum;
	struct sk_buff *cur_skb=*orig_skb;
	struct sk_buff *skb=*up_skb;
	unsigned short *cur_skb_cnt,*skb_cnt;

	csum=wanpipe_skb_get_checksum(skb);
	skb_cnt=(unsigned short*)&skb->data[0];

	if (cur_skb == NULL){
		/* This is the first incoming packet for this
		 * protocol */
		*orig_skb = skb;
		*orig_csum = csum;
		*skb_cnt=1;

		return -1;
		
	}

	cur_skb_cnt=(unsigned short*)&cur_skb->data[0];

	if (csum == *orig_csum){			
		/* Same as already stored packet,
		 * increment the packet count */
			
		if (++(*cur_skb_cnt) >= max_mult_cnt){
			/* We have reached the maximum number of
			 * multiple packets, send the packet up
			 * the stack */
	
			DEBUG_EVENT("REACHED MAX MULT COUNT %i\n",
					max_mult_cnt);
			
			*skb_cnt = *cur_skb_cnt;
					
			kfree_skb(cur_skb);
			*orig_skb=NULL;
			*orig_csum=0;
						
		}else{
			/* We incremented the count thus drop
			 * the packet */
			return 1;
		}
	}else{
		/* Different packet recevied, we must pass previous
		 * packet up the stack and store current one */
		*up_skb = cur_skb;

		DEBUG_EVENT("GOT DIFF PKT sending with cnt %i\n",
				*cur_skb_cnt);

		/* Store the incoming packet and incremnet its
		 * rx count */
		*orig_skb=skb;
		*orig_csum=csum;
		*skb_cnt=1;
	}

	return 0;
}



static int wanpipe_sk_ss7_monitor_rx(void **map, struct sk_buff **skb_ptr)
{
	struct sk_buff *skb=*skb_ptr;
	int len=skb->len-WANPIPE_HEADER_SZ;
	
	wp_sock_rx_hdr_t *hdr=(wp_sock_rx_hdr_t *)skb->data;
	int txdir=hdr->direction;
	
	unsigned short csum;
	struct sock *sk;
	int err;

	if ((sk=map[HDLC_BIT_MAP]) != NULL){
		return HDLC_BIT_MAP;
	}
	
	if (len == SS7_FISU_LEN){

		if ((sk=map[SS7_FISU_BIT_MAP]) == NULL){
			goto ss7_drop_pkt;
		}
		
		if (DATA_SC(sk)->delta){

			csum=wanpipe_skb_get_checksum(skb);
			if (txdir){
				if (csum == DATA_SC(sk)->u.ss7.tx_fisu_skb_csum){
					goto ss7_drop_pkt;
				}
				DATA_SC(sk)->u.ss7.tx_fisu_skb_csum=csum;
			}else{
				if (csum == DATA_SC(sk)->u.ss7.rx_fisu_skb_csum){
					goto ss7_drop_pkt;
				}
				DATA_SC(sk)->u.ss7.rx_fisu_skb_csum=csum;
			}

		}else if (DATA_SC(sk)->max_mult_cnt){

			if (txdir){
				err=wanpipe_skb_mult_check((struct sk_buff **)&DATA_SC(sk)->u.ss7.tx_fisu_skb,
							   &DATA_SC(sk)->u.ss7.tx_fisu_skb_csum,
							   skb_ptr,
							   DATA_SC(sk)->max_mult_cnt);
				
			}else{
				err=wanpipe_skb_mult_check((struct sk_buff **)&DATA_SC(sk)->u.ss7.rx_fisu_skb,
							   &DATA_SC(sk)->u.ss7.rx_fisu_skb_csum,
							   skb_ptr,
							   DATA_SC(sk)->max_mult_cnt);
			}
			
			if (err < 0){
				goto ss7_skip_pkt;
			}else if ( err > 0){
				goto ss7_drop_pkt;
			}
		}

		return (SS7_FISU_BIT_MAP);

		
	}else if (len > SS7_FISU_LEN && len < SS7_MSU_LEN){
		
		if ((sk=map[SS7_LSSU_BIT_MAP]) == NULL){
			goto ss7_drop_pkt;
		}

		if (DATA_SC(sk)->delta){

			csum=wanpipe_skb_get_checksum(skb);
			if (txdir){
				if (csum == DATA_SC(sk)->u.ss7.tx_lssu_skb_csum){
					goto ss7_drop_pkt;
				}
				
				DATA_SC(sk)->u.ss7.tx_lssu_skb_csum=csum;
				
			}else{
				if (csum == DATA_SC(sk)->u.ss7.rx_lssu_skb_csum){
					goto ss7_drop_pkt;
				}
				
				DATA_SC(sk)->u.ss7.rx_lssu_skb_csum=csum;
			}
			
		}else if (DATA_SC(sk)->max_mult_cnt){

			if (txdir){
				err=wanpipe_skb_mult_check((struct sk_buff **)&DATA_SC(sk)->u.ss7.tx_lssu_skb,
						   &DATA_SC(sk)->u.ss7.tx_lssu_skb_csum,
						   skb_ptr,
						   DATA_SC(sk)->max_mult_cnt);
			}else{
				err=wanpipe_skb_mult_check((struct sk_buff **)&DATA_SC(sk)->u.ss7.rx_lssu_skb,
						   &DATA_SC(sk)->u.ss7.rx_lssu_skb_csum,
						   skb_ptr,
						   DATA_SC(sk)->max_mult_cnt);
			}
				
			if (err < 0){
				goto ss7_skip_pkt;
			}else if ( err > 0){
				goto ss7_drop_pkt;
			}
		}
		
		return (SS7_LSSU_BIT_MAP);

	}else if (len >= SS7_MSU_LEN && len <= SS7_MSU_END_LEN){
	
		if ((sk=map[SS7_MSU_BIT_MAP]) == NULL){
			goto ss7_drop_pkt;
		}
		
		return (SS7_MSU_BIT_MAP);
	}

#if 1
	if (net_ratelimit()){
		DEBUG_EVENT("%s: Error: Illegal ss7 frame len %i\n",
				__FUNCTION__,len);
	}
#endif

	/* Invalid protocol or ss7 packet */
	return MAX_PARENT_PROT_NUM; 

ss7_drop_pkt:	
	kfree_skb(skb);
	*skb_ptr=NULL;

ss7_skip_pkt:
	return -1;
}




static int wanpipe_check_prot_options(struct sock *sk)
{
	if (SK_PRIV(sk)->num == htons(SS7_MONITOR_PROT)){

		if ((DATA_SC(sk)->prot_type & (1<<HDLC_BIT_MAP))){
			return 0;
		}
		
		if ((DATA_SC(sk)->prot_type & SS7_ALL_PROT) == 0){
			return -EINVAL;
		}
		return 0;
	}

	return -EINVAL;
}

static int wanpipe_attach_sk_to_parent (struct sock *parent_sk, struct sock *sk)
{
	unsigned long flags;

	if (PPRIV(parent_sk)->hdlc_enabled){

		wanpipe_hdlc_engine_t *hdlc_eng;

		hdlc_eng = wanpipe_get_hdlc_engine(parent_sk,DATA_SC(sk)->active_ch);
		if (!hdlc_eng){
			return -ENOMEM;
		}

		DEBUG_INIT("%s: (DEBUG) GOT HDLC ENGINE %p Bound = %d\n",
			SK_PRIV(parent_sk)->dev->name,hdlc_eng,hdlc_eng->bound);

		write_lock_irqsave(&PPRIV(parent_sk)->lock,flags);

		if (bind_sk_to_parent_prot(hdlc_eng->sk_to_prot_map,
					   SK_PRIV(parent_sk)->dev->name,sk) != 0){
			write_unlock_irqrestore(&PPRIV(parent_sk)->lock,flags);
			if (!hdlc_eng->bound){
				free_hdlc_engine(hdlc_eng);
			}
			return -EEXIST;
		}

		/* Insert the HDLC engine in parent hdlc
		 * engine list */
		if (!hdlc_eng->bound){
			int err;
			
			DEBUG_INIT("%s: (DEBUG) BOUND HDLC ENGINE %p to Parent\n",
				SK_PRIV(parent_sk)->dev->name,hdlc_eng);
		
			hdlc_eng->next = PPRIV(parent_sk)->hdlc_eng_list;
			PPRIV(parent_sk)->hdlc_eng_list = hdlc_eng;
			wp_dev_hold(hdlc_eng);

			err=bind_hdlc_into_timeslots(parent_sk,hdlc_eng);
			if (err!=0){
				unbind_hdlc_from_timeslots(parent_sk,hdlc_eng);
				unbind_sk_from_parent_prot(hdlc_eng->sk_to_prot_map,
							   SK_PRIV(parent_sk)->dev->name,
							   sk);
				write_unlock_irqrestore(&PPRIV(parent_sk)->lock,flags);
				free_hdlc_engine(hdlc_eng);
				return -ENOMEM;
			}
		
			hdlc_eng->bound=1;
		}

		DATA_SC(sk)->hdlc_eng = hdlc_eng;
		wp_dev_hold(hdlc_eng);

		write_unlock_irqrestore(&PPRIV(parent_sk)->lock,flags);

	}else{

		write_lock_irqsave(&PPRIV(parent_sk)->lock,flags);

		if (bind_sk_to_parent_prot(PPRIV(parent_sk)->sk_to_prot_map,
					   SK_PRIV(parent_sk)->dev->name,sk) != 0){
			write_unlock_irqrestore(&PPRIV(parent_sk)->lock,flags);
			return -EEXIST;
		}
	
		write_unlock_irqrestore(&PPRIV(parent_sk)->lock,flags);	
	}

	return 0;
}



int wanpipe_unattach_sk_from_parent (struct sock *parent_sk, struct sock *sk)
{
	unsigned long flags;
	int sk_child_exists=0;

	/* Unbind sk from parents list */
	write_lock_irqsave(&PPRIV(parent_sk)->lock,flags);

	if (PPRIV(parent_sk)->hdlc_enabled){

		wanpipe_hdlc_engine_t *hdlc_eng = DATA_SC(sk)->hdlc_eng;
		if (hdlc_eng){
			sk_child_exists = 
				unbind_sk_from_parent_prot(hdlc_eng->sk_to_prot_map,
							   SK_PRIV(parent_sk)->dev->name,
							   sk);
			if (!sk_child_exists){
				unbind_hdlc_from_timeslots(parent_sk,DATA_SC(sk)->hdlc_eng);	
				
				hdlc_eng->bound=0;
				wp_dev_put(hdlc_eng);
				DATA_SC(sk)->hdlc_eng=NULL;

				free_hdlc_engine(hdlc_eng);

			}else{
				wp_dev_put(DATA_SC(sk)->hdlc_eng);
				DATA_SC(sk)->hdlc_eng=NULL;	
			}
		}

		sk_child_exists = (PPRIV(parent_sk)->hdlc_eng_list == NULL) ? 0:1;
	
	}else{
		sk_child_exists = 
			unbind_sk_from_parent_prot(PPRIV(parent_sk)->sk_to_prot_map,
			 	   		   SK_PRIV(parent_sk)->dev->name,
					           sk);
	}
	
	wp_sock_put(DATA_SC(sk)->parent_sk);
	DATA_SC(sk)->parent_sk=NULL;

	write_unlock_irqrestore(&PPRIV(parent_sk)->lock,flags);

	return sk_child_exists;
}


/**************************************************************
 *
 * HDLC ENGINE
 * 
 **************************************************************/

static int wanpipe_skb_timeslot_parse(struct sock *parent_sk, 
		                      struct sk_buff *skb, 
				      unsigned char txdir)
{
	wanpipe_hdlc_engine_t *chan;
	wanpipe_hdlc_list_t *list_el;
	unsigned int ch=0,stream_full=0;
	unsigned char *buf;
	int len=skb->len;

//	read_lock(&PPRIV(parent_sk)->lock);

	if (PPRIV(parent_sk)->media == WAN_MEDIA_NONE){
		list_el=PPRIV(parent_sk)->time_slot_hdlc_map[0];
		if (list_el){
			chan=list_el->hdlc;	

			if (!chan->skb_decode_size){

				chan->skb_decode_size=len;

				if (chan->skb_decode_size > MAX_SOCK_HDLC_LIMIT){
					chan->skb_decode_size=MAX_SOCK_HDLC_LIMIT;
				}
				
				DEBUG_EVENT("%s: Autocfg serial hdlc tx up size to %d\n",
						SK_PRIV(parent_sk)->dev->name,
						chan->skb_decode_size);
			}

			if (wanpipe_hdlc_decode(chan,txdir,skb)){
				stream_full=1;
			}
			return stream_full;
		}
	}
	
	for (;;){

		for (list_el=PPRIV(parent_sk)->time_slot_hdlc_map[ch];
		     list_el;
		     list_el = list_el->next){

			chan=list_el->hdlc;

			if (!chan->skb_decode_size){

				chan->skb_decode_size=len;

				if (chan->skb_decode_size > MAX_SOCK_HDLC_LIMIT){
					chan->skb_decode_size=MAX_SOCK_HDLC_LIMIT;
				}
				
				if (chan->skb_decode_size % chan->timeslots){
					chan->skb_decode_size-=
							chan->skb_decode_size % 
							chan->timeslots;
					chan->skb_decode_size+=chan->timeslots;
				}

				DEBUG_EVENT("%s: Autocfg hdlc tx up size to %d\n",
						SK_PRIV(parent_sk)->dev->name,
						chan->skb_decode_size);
			}


			if (txdir){

				if (chan->raw_tx_skb){
					buf=skb_put(chan->raw_tx_skb,1);
					buf[0]=skb->data[0];

					if (chan->raw_tx_skb->len >= chan->skb_decode_size){
						if (wanpipe_hdlc_decode(chan,txdir,chan->raw_tx_skb)){
							stream_full=1;
						}
						wan_skb_init(chan->raw_tx_skb,16);
						wan_skb_trim(chan->raw_tx_skb,0);
					}
					
				}else{
					DEBUG_EVENT("%s:%s: Critical Error no hdlc raw_tx_skb ptr!\n",
							SK_PRIV(parent_sk)->dev->name,
							__FUNCTION__);
				}
			}else{
				if (chan->raw_rx_skb){
					buf=skb_put(chan->raw_rx_skb,1);
					buf[0]=skb->data[0];

					if (chan->raw_rx_skb->len >= chan->skb_decode_size){
						if (wanpipe_hdlc_decode(chan,txdir,chan->raw_rx_skb)){
							stream_full=1;
						}
						wan_skb_init(chan->raw_rx_skb,16);
						wan_skb_trim(chan->raw_rx_skb,0);
					}
					
				}else{
					DEBUG_EVENT("%s:%s: Critical Error no hdlc raw_rx_skb ptr!\n",
							SK_PRIV(parent_sk)->dev->name,
							__FUNCTION__);
				}
			}
		}
		skb_pull(skb,1);

		if (skb->len < 1){
			break;
		}
	
		if (++ch >= PPRIV(parent_sk)->time_slots){
			ch=0;
		}
	
	}//for(;;)

//	read_unlock(&PPRIV(parent_sk)->lock);
	
	return stream_full;
}

/* HDLC Bitstream Decode Functions */
static int wanpipe_hdlc_decode (wanpipe_hdlc_engine_t *hdlc_eng, unsigned char txdir, struct sk_buff *skb)
{
	int i;
	int word_len;
	int found=0;
	unsigned char *buf;
	int len;
	int gotdata=0;

	buf = skb->data;
	len = skb->len;
	
	word_len=len-(len%4);
	/* Before decoding the packet, make sure that the current
	 * bit stream contains data. Decoding is very expensive,
	 * thus perform word (32bit) comparision test */
	
	for (i=0;i<word_len;i+=4){
		if ((*(unsigned int*)&buf[i]) != *(unsigned int*)buf){
			found=1;
			break;
		}
	}

	if ((len%4) && !found){
		for (i=word_len;i<len;i++){
			if (buf[i]!=buf[0]){
				found=1;
				break;
			}
		}
	}
	
	/* Data found proceed to decode
	 * the bitstream and pull out data packets */
	if (found){
		wanpipe_hdlc_decoder_t *hdlc_decoder;
	
		if (txdir){
			hdlc_decoder = 	&hdlc_eng->tx_decoder;
		}else{
			hdlc_decoder = 	&hdlc_eng->rx_decoder;
		}

		for (i=0; i<len; i++){
			if (decode_byte(hdlc_eng,hdlc_decoder,&buf[i],txdir)){
				gotdata=1;
			}
		}

		if (hdlc_decoder->rx_decode_len >= MAX_SOCK_HDLC_LIMIT){
			DEBUG_EVENT ("ERROR Rx decode len > max\n");		
			init_hdlc_decoder(hdlc_decoder);
		}
	}
	
	return gotdata;
}

static int decode_byte (wanpipe_hdlc_engine_t *hdlc_eng, 
		        wanpipe_hdlc_decoder_t *chan,
			unsigned char *byte_ptr,
			unsigned char txdir)
{
	int i;
	int gotdata=0;
	unsigned long byte=*byte_ptr;

	/* Test each bit in an incoming bitstream byte.  Search
	 * for an hdlc flag 0x7E, six 1s in a row.  Once the
	 * flag is obtained, construct the data packets. 
	 * The complete data packets are sent up the API stack */
	
	for (i=0; i<BITSINBYTE; i++){

		if (hdlc_eng->seven_bit_hdlc && i == 7){
			continue;
		}
		
		if (test_bit(i,&byte)){
			/* Got a 1 */
			
			++chan->rx_decode_onecnt;
			
			/* Make sure that we received a valid flag,
			 * before we start decoding incoming data */
			if (!test_bit(NO_FLAG,&chan->hdlc_flag)){ 
				chan->rx_decode_buf[chan->rx_decode_len] |= (1 << chan->rx_decode_bit_cnt);
				
				if (++chan->rx_decode_bit_cnt >= BITSINBYTE){

					/* Completed a byte of data, update the
					 * crc count, and start on the next 
					 * byte.  */
					calc_rx_crc(chan);
#ifdef PRINT_PKT
					printk(" %02X", data);
#endif
					++chan->rx_decode_len;
					if (chan->rx_decode_len > MAX_SOCK_HDLC_BUF){
						init_hdlc_decoder(&hdlc_eng->tx_decoder);	
					}else{
						chan->rx_decode_buf[chan->rx_decode_len]=0;
						chan->rx_decode_bit_cnt=0;
						chan->hdlc_flag=0;
						set_bit(CLOSING_FLAG,&chan->hdlc_flag);
					}
				}
			}
		}else{
			/* Got a zero */
			if (chan->rx_decode_onecnt == 5){
				
				/* bit stuffed zero detected,
				 * do not increment our decode_bit_count.
				 * thus, ignore this bit*/
				
			
			}else if (chan->rx_decode_onecnt == 6){
				
				/* Got a Flag */
				if (test_bit(CLOSING_FLAG,&chan->hdlc_flag)){
				
					/* Got a closing flag, thus asemble
					 * the packet and send it up the 
					 * stack */
					chan->hdlc_flag=0;
					set_bit(OPEN_FLAG,&chan->hdlc_flag);
				
					if (chan->rx_decode_len >= 3){
						
						GET_FIN_CRC_CNT(chan->crc_cur);
						FLIP_CRC(chan->rx_crc[chan->crc_cur],chan->crc_fin);
						DECODE_CRC(chan->crc_fin);
				
						/* Check CRC error before passing data up
						 * the API socket */
						if (chan->crc_fin==chan->rx_orig_crc){
							tx_up_decode_pkt(hdlc_eng,chan,txdir);
							gotdata=1;
						}else{
							//++chan->ifstats.rx_crc_errors;
							//CRC Error; initialize hdlc eng
							init_hdlc_decoder(&hdlc_eng->tx_decoder);
						}
					}else{
						//Abort
						//++chan->ifstats.rx_frame_errors;
					}

				}else if (test_bit(NO_FLAG,&chan->hdlc_flag)){
					/* Got a very first flag */
					chan->hdlc_flag=0;	
					set_bit(OPEN_FLAG,&chan->hdlc_flag);
				}

				/* After a flag, initialize the decode and
				 * crc buffers and get ready for the next 
				 * data packet */
				chan->rx_decode_len=0;
				chan->rx_decode_buf[chan->rx_decode_len]=0;
				chan->rx_decode_bit_cnt=0;
				chan->rx_crc[0]=-1;
				chan->rx_crc[1]=-1;
				chan->rx_crc[2]=-1;
				chan->crc_cur=0; 
				chan->crc_prv=0;
			}else{
				/* Got a valid zero, thus increment the
				 * rx_decode_bit_cnt, as a result of which
				 * a zero is left in the consturcted
				 * byte.  NOTE: we must have a valid flag */
				
				if (!test_bit(NO_FLAG,&chan->hdlc_flag)){ 	
					if (++chan->rx_decode_bit_cnt >= BITSINBYTE){
						calc_rx_crc(chan);
#ifdef PRINT_PKT
						printk(" %02X", data);
#endif
						++chan->rx_decode_len;
						if (chan->rx_decode_len > MAX_SOCK_HDLC_BUF){
							init_hdlc_decoder(&hdlc_eng->tx_decoder);
						}else{
							chan->rx_decode_buf[chan->rx_decode_len]=0;
							chan->rx_decode_bit_cnt=0;
							chan->hdlc_flag=0;
							set_bit(CLOSING_FLAG,&chan->hdlc_flag);
						}
					}
				}
			}
			chan->rx_decode_onecnt=0;
		}
	}
	
	return gotdata;
}


static void init_crc(void)
{
	int i,j;

	if (test_and_set_bit(0,&init_crc_g)){
		return;
	}
	
	for(i=0;i<256;i++){
		CRC_TABLE[i]=0;
		for (j=0;j<BITSINBYTE;j++){
			if (i & (1<<j)){
				CRC_TABLE[i] ^= MagicNums[j];
			}
		}
	}
}

static void calc_rx_crc(wanpipe_hdlc_decoder_t *chan)
{
	INC_CRC_CNT(chan->crc_cur);

	/* Save the incoming CRC value, so it can be checked
	 * against the calculated one */
	chan->rx_orig_crc = (((chan->rx_orig_crc<<8)&0xFF00) | chan->rx_decode_buf[chan->rx_decode_len]);
	
	chan->rx_crc_tmp = (chan->rx_decode_buf[chan->rx_decode_len] ^ chan->rx_crc[chan->crc_prv]) & 0xFF;
	chan->rx_crc[chan->crc_cur] =  chan->rx_crc[chan->crc_prv] >> 8;
	chan->rx_crc[chan->crc_cur] &= 0x00FF;
	chan->rx_crc[chan->crc_cur] ^= CRC_TABLE[chan->rx_crc_tmp];
	chan->rx_crc[chan->crc_cur] &= 0xFFFF;
	INC_CRC_CNT(chan->crc_prv);
}

static void tx_up_decode_pkt(wanpipe_hdlc_engine_t *hdlc_eng, 
		             wanpipe_hdlc_decoder_t *chan, 
			     unsigned char txdir)
{
	wp_sock_rx_hdr_t *hdr;
	unsigned char *buf;

	struct sk_buff *skb = wan_skb_alloc(chan->rx_decode_len+sizeof(wp_sock_rx_hdr_t));
	if (!skb){
		printk(KERN_INFO "wansock: %s: HDLC Tx up: failed to allocate memory!\n",
				__FUNCTION__);
		return;
	}

	hdr=(wp_sock_rx_hdr_t *)skb_put(skb,sizeof(wp_sock_rx_hdr_t));
	memset(hdr, 0, sizeof(wp_sock_rx_hdr_t));
	hdr->direction=txdir;

	buf = skb_put(skb,chan->rx_decode_len);
	memcpy(buf, 
	       chan->rx_decode_buf, 
	       chan->rx_decode_len);

	DEBUG_TX("Tx UP Dir=%d, decod size %i hdr %i skblen=%i\n",
			txdir,chan->rx_decode_len,
			sizeof(wp_sock_rx_hdr_t),
			skb->len);

	wan_skb_queue_tail(&hdlc_eng->data_q,skb);
	return;

}

/**************************************************************
 *
 * UTILITY FUNCTIONS
 * 
 **************************************************************/



static int bind_hdlc_into_timeslots(struct sock *parent_sk,
		                     wanpipe_hdlc_engine_t *hdlc_eng)
{
	int i=0;
	wanpipe_hdlc_list_t *hdlc_list;

	if (PPRIV(parent_sk)->media == WAN_MEDIA_NONE){
		hdlc_list=wan_malloc(sizeof(wanpipe_hdlc_list_t));
		if (!hdlc_list){
			DEBUG_EVENT("%s: Error failed to allocate list memory!\n",
					SK_PRIV(parent_sk)->dev->name);
			return -ENOMEM;
		}
		memset(hdlc_list,0,sizeof(sizeof(wanpipe_hdlc_list_t)));

		++hdlc_eng->timeslots;
		hdlc_list->hdlc=hdlc_eng;
		wp_dev_hold(hdlc_eng);
			
		hdlc_list->next = PPRIV(parent_sk)->time_slot_hdlc_map[i];
		PPRIV(parent_sk)->time_slot_hdlc_map[i] = hdlc_list;

		DEBUG_EVENT("%s: Binding HDLC engine to Serial\n",
				SK_PRIV(parent_sk)->dev->name);
		return 0;
		
	}

	
	for (i=0;i<PPRIV(parent_sk)->time_slots;i++){
		if (test_bit(i,&hdlc_eng->active_ch)){

			hdlc_list=wan_malloc(sizeof(wanpipe_hdlc_list_t));
			if (!hdlc_list){
				DEBUG_EVENT("%s: Error failed to allocate list memory!\n",
						SK_PRIV(parent_sk)->dev->name);
				return -ENOMEM;
			}
			memset(hdlc_list,0,sizeof(sizeof(wanpipe_hdlc_list_t)));

			++hdlc_eng->timeslots;
			hdlc_list->hdlc=hdlc_eng;
			wp_dev_hold(hdlc_eng);
			
			if (PPRIV(parent_sk)->media == WAN_MEDIA_E1){
				hdlc_list->next = PPRIV(parent_sk)->time_slot_hdlc_map[i+1];
				PPRIV(parent_sk)->time_slot_hdlc_map[i+1] = hdlc_list;
			}else{
				hdlc_list->next = PPRIV(parent_sk)->time_slot_hdlc_map[i];
				PPRIV(parent_sk)->time_slot_hdlc_map[i] = hdlc_list;
			}

			DEBUG_EVENT("%s: Binding HDLC engine to timeslot %i\n",
					SK_PRIV(parent_sk)->dev->name,i+1);
		}
	}
	
	return 0;
}

static int remove_hdlc_from_list(wanpipe_hdlc_engine_t **list, wanpipe_hdlc_engine_t *hdlc_eng)
{
	wanpipe_hdlc_engine_t *tmp,*prev;

	if ((tmp=*list) == hdlc_eng){
		*list = hdlc_eng->next;
		DEBUG_TEST("DEBUG Removing first hdlc_eng %p from hdlc list: list=%p\n",
				hdlc_eng,*list);
		wp_dev_put(hdlc_eng);
		return 0;
	}
	
	while (tmp != NULL && tmp->next != NULL){
		
		prev=tmp;
		tmp=tmp->next;

		if (tmp == hdlc_eng){
			prev->next=tmp->next;	
			DEBUG_TEST("DEBUG Removing hdlc_eng %p from hdlc list: list=%p\n",
				hdlc_eng,*list);
			wp_dev_put(hdlc_eng);
			return 0;
		}
	}

	return -ENODEV;
}

static int remove_hdlc_from_ts_list(wanpipe_hdlc_list_t **list, wanpipe_hdlc_engine_t *hdlc_eng)
{
	wanpipe_hdlc_list_t *tmp,*prev;

	if ((tmp=*list)){

		if (tmp->hdlc == hdlc_eng){
		
			*list = tmp->next;
		
			DEBUG_TEST("DEBUG Removing first hdlc_eng %p from ts list: list=%p\n",
				hdlc_eng,*list);
			wp_dev_put(hdlc_eng);

			wan_free(tmp);
			return 0;
		}
	}
	
	while (tmp != NULL && tmp->next != NULL){
		
		prev=tmp;
		tmp=tmp->next;

		if (tmp->hdlc == hdlc_eng){
			prev->next=tmp->next;
			DEBUG_TEST("DEBUG Removing hdlc_eng %p from list: list=%p\n",
				hdlc_eng,*list);
			wp_dev_put(hdlc_eng);

			wan_free(tmp);
			return 0;
		}
	}

	return -ENODEV;
}



static void unbind_hdlc_from_timeslots(struct sock *parent_sk, wanpipe_hdlc_engine_t *hdlc_eng)
{
	int i;
	remove_hdlc_from_list(&PPRIV(parent_sk)->hdlc_eng_list,hdlc_eng);

	for (i=0;i<MAX_SOCK_CHANNELS;i++){
		if (PPRIV(parent_sk)->time_slot_hdlc_map[i]){
			remove_hdlc_from_ts_list(&PPRIV(parent_sk)->time_slot_hdlc_map[i],
					         hdlc_eng);
		}
	}

	return;
}


static wanpipe_hdlc_engine_t *wanpipe_get_hdlc_engine (struct sock *parent_sk,
		 				       unsigned long active_ch)
{
	wanpipe_hdlc_engine_t *hdlc_eng;
	
	for (hdlc_eng = PPRIV(parent_sk)->hdlc_eng_list; 
	     hdlc_eng;
	     hdlc_eng = hdlc_eng->next){
		if (hdlc_eng->active_ch == active_ch){
			return hdlc_eng;
		}	
	}

	hdlc_eng = wan_malloc(sizeof(wanpipe_hdlc_engine_t));
	if (!hdlc_eng){
		return NULL;
	}	

	memset(hdlc_eng,0,sizeof(wanpipe_hdlc_engine_t));

	hdlc_eng->raw_rx_skb = wan_skb_alloc(MAX_SOCK_HDLC_BUF);
	if (!hdlc_eng->raw_rx_skb){
		wan_free(hdlc_eng);
		return NULL;
	}

	hdlc_eng->raw_tx_skb = wan_skb_alloc(MAX_SOCK_HDLC_BUF);
	if (!hdlc_eng->raw_tx_skb){
		wan_skb_free(hdlc_eng->raw_rx_skb);
		wan_free(hdlc_eng);
		return NULL;
	}

	hdlc_eng->active_ch=active_ch;
	hdlc_eng->seven_bit_hdlc=PPRIV(parent_sk)->seven_bit_hdlc;

	init_hdlc_decoder(&hdlc_eng->rx_decoder);
	init_hdlc_decoder(&hdlc_eng->tx_decoder);
	
	skb_queue_head_init(&hdlc_eng->data_q);

	wp_dev_hold(hdlc_eng);

	return hdlc_eng;	
}

static void init_hdlc_decoder(wanpipe_hdlc_decoder_t *hdlc_decoder)
{
	hdlc_decoder->hdlc_flag=0;
	set_bit(NO_FLAG,&hdlc_decoder->hdlc_flag);
	
	hdlc_decoder->rx_decode_len=0;
	hdlc_decoder->rx_decode_buf[hdlc_decoder->rx_decode_len]=0;
	hdlc_decoder->rx_decode_bit_cnt=0;
	hdlc_decoder->rx_decode_onecnt=0;
	hdlc_decoder->rx_crc[0]=-1;
	hdlc_decoder->rx_crc[1]=-1;
	hdlc_decoder->rx_crc[2]=-1;
	hdlc_decoder->crc_cur=0; 
	hdlc_decoder->crc_prv=0;
}


static void free_hdlc_engine(wanpipe_hdlc_engine_t *hdlc_eng)
{
	skb_queue_purge(&hdlc_eng->data_q);
	if (hdlc_eng->raw_tx_skb){
		wan_skb_free(hdlc_eng->raw_tx_skb);
		hdlc_eng->raw_tx_skb=NULL;
	}
	if (hdlc_eng->raw_rx_skb){
		wan_skb_free(hdlc_eng->raw_rx_skb);
		hdlc_eng->raw_rx_skb=NULL;
	}
	wp_dev_put(hdlc_eng);
}

static int bind_sk_to_parent_prot(void **map, unsigned char *devname,struct sock *sk)
{
	int i;

	for (i=0;i<MAX_PARENT_PROT_NUM;i++){
		if (test_bit(i,&DATA_SC(sk)->prot_type)){
			if (map[i]){
				DEBUG_EVENT("%s: Error SS7 prot %s exists!\n",
					devname,DECODE_SS7_PROT((1<<i)));

				return -EEXIST;
			}else{

				DEBUG_EVENT("%s: Sock prot %s registered\n",
					devname,
					DECODE_SS7_PROT((1<<i)));

				map[i]=sk;	
				wp_sock_hold(sk);
			}
		}
	}
	return 0;
}

static int unbind_sk_from_parent_prot(void**map,unsigned char *devname,struct sock *sk)
{
	int i;
	int sk_child_exists=0;

	for (i=0;i<MAX_PARENT_PROT_NUM;i++){
		if (map[i] == sk){
			DEBUG_EVENT("%s: Sock prot %s unregistered\n",
					devname,
					DECODE_SS7_PROT((1<<i)));
			map[i]=NULL;	
			wp_sock_put(sk);
		}

		if (map[i] != NULL){
			sk_child_exists=1;
		}
	}

	return sk_child_exists;
}

/*============================================================
 *  wanpipe_free_parent_sock
 *
 *	This is a function which performs actual killing
 *      of the sock.  It releases socket resources,
 *      and unlinks the sock from the driver. 
 *===========================================================*/
static void wanpipe_free_parent_sock(struct sock *sk)
{
	unsigned long flags;
	netdevice_t *dev;
	
#ifndef LINUX_2_4
	struct sk_buff *skb;
#endif

	if (!sk)
		return;

	write_lock_irqsave(&PPRIV(sk)->lock,flags);

	dev=SK_PRIV(sk)->dev;
	if (dev && WAN_NETDEV_TEST_IOCTL(dev)){
		struct ifreq ifr;
		memset(&ifr,0,sizeof(struct ifreq));
		ifr.ifr_data = (void*)sk;
		DEBUG_TEST("%s: UNBINDING SK dev=%s\n",__FUNCTION__,dev->name);
		WAN_NETDEV_IOCTL(dev,&ifr,SIOC_WANPIPE_UNBIND_SK);
	}

	sk->sk_socket = NULL;

	/* Purge queues */
#ifdef LINUX_2_4
	skb_queue_purge(&sk->sk_receive_queue);
	skb_queue_purge(&sk->sk_write_queue);
	skb_queue_purge(&sk->sk_error_queue);
#else	
	while ((skb=skb_dequeue(&sk->sk_receive_queue)) != NULL){
		kfree_skb(skb);
	}
	while ((skb=skb_dequeue(&sk->sk_write_queue)) != NULL) {
		kfree_skb(skb);
	}
	while ((skb=skb_dequeue(&sk->sk_error_queue)) != NULL){
		kfree_skb(skb);
	}
#endif

	write_unlock_irqrestore(&PPRIV(sk)->lock,flags);

	
	if (PPRIV(sk)){
		wan_free(PPRIV(sk));
		PPRIV_INIT(sk,NULL);
	}
	
	if (DATA_SC(sk)){
		wan_free(DATA_SC(sk));
		DATA_SC_INIT(sk,NULL);
	}
	
	if (SK_PRIV(sk)){
		kfree(sk->sk_user_data);
		sk->sk_user_data=NULL;
	}

      #ifdef LINUX_2_4
	if (atomic_read(&sk->refcnt) != 1){
		atomic_set(&sk->refcnt,1);
		DEBUG_EVENT("%s:%d: Error, wrong reference count: %i ! :timer.\n",
					__FUNCTION__,__LINE__,atomic_read(&sk->refcnt));
	}
	wp_sock_put(sk);
      #else
	sk_free(sk);
      #endif
	atomic_dec(&wanpipe_socks_nr);
#ifndef LINUX_2_6
	MOD_DEC_USE_COUNT;
#endif
	return;
}


#if 0
static void print_hdlc_list(wanpipe_hdlc_engine_t **list, int ts_list)
{
	wanpipe_hdlc_engine_t *tmp;

	DEBUG_EVENT("\n");
	if (ts_list){
		DEBUG_EVENT("TS LIST: ");
		for (tmp=*list;tmp;tmp=tmp->ts_next){
		__DEBUG_EVENT("%p ->"); 
	
	}else{
		DEBUG_EVENT("LIST: ");
		for (tmp=*list;tmp;tmp=tmp->next){
			__DEBUG_EVENT("%p ->"); 
		}
	}
	__DEBUG_EVENT("NULL \n");
	DEBUG_EVENT("\n");
	
}
#endif
