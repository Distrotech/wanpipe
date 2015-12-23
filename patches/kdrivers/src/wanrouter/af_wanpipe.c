/*****************************************************************************
* af_wanpipe.c	
*
* 		WANPIPE(tm) Annexg Secure Socket Layer.
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
* Due Credit:
*               Wanpipe socket layer is based on Packet and 
*               the X25 socket layers. The above sockets were 
*               used for the specific use of Sangoma Technoloiges 
*               API programs. 
*               Packet socket Authors: Ross Biro, Fred N. van Kempen and 
*                                      Alan Cox.
*               X25 socket Author: Jonathan Naylor.
* ============================================================================
* Apr 25, 2000  Nenad Corbic     o Added the ability to send zero length packets.
* Mar 13, 2000  Nenad Corbic	 o Added a tx buffer check via ioctl call.
* Mar 06, 2000  Nenad Corbic     o Fixed the corrupt sock lcn problem.
*                                  Server and client applicaton can run
*                                  simultaneously without conflicts.
* Feb 29, 2000  Nenad Corbic     o Added support for PVC protocols, such as
*                                  CHDLC, Frame Relay and HDLC API.
* Jan 17, 2000 	Nenad Corbic	 o Initial version, based on AF_PACKET socket.
*			           X25API support only. 
*
******************************************************************************/

#include <linux/wanpipe_includes.h>
#include <linux/wanpipe_defines.h>
#include <linux/wanpipe.h>
#include <linux/if_wanpipe_kernel.h>

#include <linux/if_wanpipe.h>
#include <linux/if_wanpipe_common.h>
#include <linux/wanpipe_x25_kernel.h>
#include <linux/wanpipe_dsp_kernel.h>
#include <linux/if_wanpipe_kernel.h>

#if defined(LINUX_2_1)
 #define dev_put(a)
 #define __sock_put(a)
 #define sock_hold(a)
 #define DECLARE_WAITQUEUE(a,b) \
		struct wait_queue a = { b, NULL }
#endif


#define AF_DEBUG_MEM
#ifdef AF_DEBUG_MEM

 #define AF_SKB_DEC(x)  atomic_sub(x,&af_skb_alloc)
 #define AF_SKB_INC(x)	atomic_add(x,&af_skb_alloc)

 #define ALLOC_SKB(skb,len) { skb = dev_alloc_skb(len);			\
			      if (skb != NULL) AF_SKB_INC(skb->truesize);	}
 #define KFREE_SKB(skb)     { AF_SKB_DEC(skb->truesize); dev_kfree_skb_any(skb); }


 #define AF_MEM_DEC(x)  atomic_sub(x,&af_mem_alloc)
 #define AF_MEM_INC(x)	 atomic_add(x,&af_mem_alloc) 			      
			      
 #define KMALLOC(ptr,len,flag)	{ ptr=kmalloc(len, flag); \
	  			  if (ptr != NULL) AF_MEM_INC(len); }
 #define KFREE(ptr)		{AF_MEM_DEC(sizeof(*ptr)); kfree(ptr);}
			      
#else
 #define KMALLOC(ptr,len,flag)	ptr=kmalloc(len, flag)
 #define KFREE(ptr)		kfree(ptr)			
			      
 #define ALLOC_SKB(new_skb,len, dsp) new_skb = dev_alloc_skb(len)
 #define KFREE_SKB(skb)	             dev_kfree_skb_any(skb)

 #define AF_SKB_DEC(x) 
 #define AF_SKB_INC(x)	 
 #define AF_MEM_DEC(x) 
 #define AF_MEM_INC(x)	
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(3,19,0)
static inline int memcpy_from_msg(void *data, struct msghdr *msg, int len)
{
	/* XXX: stripping const */
	return memcpy_fromiovec(data, (struct iovec *)msg->msg_iov, len);
}

static inline int memcpy_to_msg(struct msghdr *msg, void *data, int len)
{
	return memcpy_toiovec((struct iovec *)msg->msg_iov, data, len);

}
#endif

#define WP_API_HDR_SZ 16

#ifdef CONFIG_PRODUCT_WANPIPE_SOCK_DATASCOPE
extern void wanpipe_unbind_sk_from_parent(struct sock *sk);
extern int wanpipe_bind_sk_to_parent(struct sock *sk, netdevice_t *dev, struct wan_sockaddr_ll *sll);
extern int wanpipe_sk_parent_rx(struct sock *parent_sk, struct sk_buff *skb);
#endif
			  

		
/* The code below is used to test memory leaks. It prints out
 * a message every time kmalloc and kfree system calls get executed.
 * If the calls match there is no leak :)
 */

/***********FOR DEBUGGING PURPOSES*********************************************
#define KMEM_SAFETYZONE 8

static void * dbg_kmalloc(unsigned int size, int prio, int line) {
	void * v = kmalloc(size,prio);
	printk(KERN_INFO "line %d  kmalloc(%d,%d) = %p\n",line,size,prio,v);
	return v;
}
static void dbg_kfree(void * v, int line) {
	printk(KERN_INFO "line %d  kfree(%p)\n",line,v);
	kfree(v);
}

#define kmalloc(x,y) dbg_kmalloc(x,y,__LINE__)
#define kfree(x) dbg_kfree(x,__LINE__)
******************************************************************************/

/* List of all wanpipe sockets. */

#ifdef LINUX_2_6
HLIST_HEAD(wanpipe_sklist);

#ifdef AF_WANPIPE_2612_UPDATE
static struct proto packet_proto = {
	.name	  = "AF_WANPIPE",
	.owner	  = THIS_MODULE,
	.obj_size = sizeof(struct sock),
};
#endif

#else
static struct sock * wanpipe_sklist = NULL;
#endif

static rwlock_t wanpipe_sklist_lock;
rwlock_t wanpipe_parent_sklist_lock;

static atomic_t af_mem_alloc;
static atomic_t af_skb_alloc;

atomic_t wanpipe_socks_nr;
extern struct proto_ops wanpipe_ops;

#ifdef sk_net_refcnt
static struct sock *wanpipe_alloc_socket(struct sock *, void *net, int kern);
#else
static struct sock *wanpipe_alloc_socket(struct sock *, void *net);
#endif
static void check_write_queue(struct sock *);


void wanpipe_kill_sock(struct sock *sock);

#if 0
void af_wan_sock_rfree(struct sk_buff *skb)
{
	struct sock *sk = skb->sk;
	atomic_sub(skb->truesize, &sk->sk_rmem_alloc);
	wan_skb_destructor(skb);
}

void wan_skb_set_owner_r(struct sk_buff *skb, struct sock *sk)
{
	skb->sk = sk;
	skb->destructor = af_wan_sock_rfree;
	atomic_add(skb->truesize, &sk->sk_rmem_alloc);
}
#endif

static inline void af_skb_queue_purge(struct sk_buff_head *list)
{
	struct sk_buff *skb;
	while ((skb=skb_dequeue(list))!=NULL){
		KFREE_SKB(skb);
	}
}


/*============================================================
 *  wanpipe_ioctl
 *	
 * 	Execute a user commands, and set socket options.
 *
 * FIXME: More thought should go into this function.
 *
 *===========================================================*/

static int wanpipe_ioctl(struct socket *sock, unsigned int cmd, unsigned long arg)
{
	struct sock *sk = sock->sk;

	WAN_ASSERT_EINVAL(!SK_PRIV(sk));

	switch(cmd) 
	{
		case SIOC_WANPIPE_CHECK_TX:
			if (atomic_read(&sk->sk_wmem_alloc))
				return 1;

			goto dev_private_ioctl;

		case SIOC_WANPIPE_SOCK_STATE:

			if (sk->sk_state == WANSOCK_CONNECTED){
				return 0;
			}else if (sk->sk_state == WANSOCK_DISCONNECTED){	
				return 1;
			}else{
				return 2;
			}

		case SIOC_WANPIPE_SOCK_FLUSH_BUFS:
			{
				netskb_t *skb;
				int err;
				while ((skb=skb_dequeue(&sk->sk_error_queue)) != NULL){
					AF_SKB_DEC(skb->truesize);
					skb_free_datagram(sk, skb);
				}
				while ((skb=skb_recv_datagram(sk,0,1,&err)) != NULL){
					AF_SKB_DEC(skb->truesize);
					skb_free_datagram(sk, skb);
				}
			}
			return 0;

		case SIOC_WANPIPE_SET_NONBLOCK:

			if (sk->sk_state != WANSOCK_DISCONNECTED)
				return -EINVAL;

			sock->file->f_flags |= O_NONBLOCK;
			return 0;

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,15)	
		case SIOCGIFFLAGS:
#ifndef CONFIG_INET
		case SIOCSIFFLAGS:
#endif
		case SIOCGIFCONF:
		case SIOCGIFMETRIC:
		case SIOCSIFMETRIC:
		case SIOCGIFMEM:
		case SIOCSIFMEM:
		case SIOCGIFMTU:
		case SIOCSIFMTU:
		case SIOCSIFLINK:
		case SIOCGIFHWADDR:
		case SIOCSIFHWADDR:
		case SIOCSIFMAP:
		case SIOCGIFMAP:
		case SIOCSIFSLAVE:
		case SIOCGIFSLAVE:
		case SIOCGIFINDEX:
		case SIOCGIFNAME:
		case SIOCGIFCOUNT:
		case SIOCSIFHWBROADCAST:
			return(dev_ioctl(cmd,(void *) arg));
#endif
			
#ifdef CONFIG_INET
		case SIOCADDRT:
		case SIOCDELRT:
		case SIOCDARP:
		case SIOCGARP:
		case SIOCSARP:
		case SIOCDRARP:
		case SIOCGRARP:
		case SIOCSRARP:
		case SIOCGIFADDR:
		case SIOCSIFADDR:
		case SIOCGIFBRDADDR:
		case SIOCSIFBRDADDR:
		case SIOCGIFNETMASK:
		case SIOCSIFNETMASK:
		case SIOCGIFDSTADDR:
		case SIOCSIFDSTADDR:
		case SIOCSIFFLAGS:
		case SIOCADDDLCI:
		case SIOCDELDLCI:
			return inet_dgram_ops.ioctl(sock, cmd, arg);
#endif

		default:
			if ((cmd >= SIOCDEVPRIVATE) &&
			    (cmd <= (SIOCDEVPRIVATE + 50))){
				netdevice_t *dev;
				struct ifreq ifr;
dev_private_ioctl:

				dev = (struct net_device *)SK_PRIV(sk)->dev;
				if (!dev)
					return -ENODEV;

				if (!WAN_NETDEV_TEST_IOCTL(dev)) 
					return -ENODEV;
				
				ifr.ifr_data = (void*)arg;
			
				return WAN_NETDEV_IOCTL(dev,&ifr,cmd);
			}

			DEBUG_EVENT("%s: Ioctl call not supported DevPriv %i Cmd %i \n",
					__FUNCTION__,SIOCDEVPRIVATE,cmd);
			return -EOPNOTSUPP;
			
	}
	/*NOTREACHED*/
}

/*============================================================
 * wanpipe_make_new
 *
 *	Create a new sock, and allocate a wanpipe private
 *      structure to it. Also, copy the important data
 *      from the original sock to the new sock.
 *
 *      This function is used by wanpipe_listen_rcv() listen
 *      bottom half handler.  A copy of the listening sock
 *      is created using this function.
 *
 *===========================================================*/

struct sock *wanpipe_make_new(struct sock *osk)
{
	struct sock *sk;
	wait_queue_head_t *tmp1;
	wait_queue_head_t *tmp2;

	if (osk->sk_type != SOCK_RAW)
		return NULL;

#ifdef sk_net_refcnt
	if ((sk = wanpipe_alloc_socket(osk,NULL,1)) == NULL)
		return NULL;
#else
	if ((sk = wanpipe_alloc_socket(osk,NULL)) == NULL)
		return NULL;
#endif

	sk->sk_family      = osk->sk_family;
	sk->sk_type        = osk->sk_type;
	sk->sk_socket      = osk->sk_socket;
	sk->sk_priority    = osk->sk_priority;
	SK_PRIV(sk)->num   = SK_PRIV(osk)->num;
	sk->sk_rcvbuf      = osk->sk_rcvbuf;
	sk->sk_sndbuf      = osk->sk_sndbuf;
	sk->sk_debug       = osk->sk_debug;
	sk->sk_state       = WANSOCK_CONNECTING;
	tmp1 = WAN_SK_SLEEP(sk);
	tmp2 = WAN_SK_SLEEP(osk);
	tmp1 = tmp2;

	return sk;
}



/*============================================================
 * wanpipe_listen_rcv
 *
 *	Wanpipe LISTEN socket bottom half handler.  This function
 *      is called by the WANPIPE device drivers to queue an
 *      incomming call into the socket listening queue. 
 *      Once the packet is queued, the waiting accept() process 
 *      is woken up.
 *
 *      During socket bind, this function is bounded into
 *      WANPIPE driver private. 
 * 
 *      IMPORTANT NOTE:
 *          The accept call() is waiting for an skb packet
 *          which contains a pointer to a device structure.
 *
 *          When we do a bind to a device structre, we 
 *          bind a newly created socket into "chan->sk".  Thus, 
 *          when accept receives the skb packet, it will know 
 *          from which dev it came form, and in turn it will know
 *          the address of the new sock.
 *
 *  	NOTE: This function gets called from driver ISR.
 *===========================================================*/

static int wanpipe_listen_rcv (struct sk_buff *skb,  struct sock *sk)
{

	struct wan_sockaddr_ll *sll;
	struct sock *newsk;
	netdevice_t *dev; 
	//x25api_hdr_t *x25_api_hdr;
	struct ifreq ifr;
	unsigned long flags;

	if (!sk){
		printk(KERN_INFO "Wanpipe listen recv error! No skid!\n");
		return 0;
	}
	
	/* If we receive a NULL skb pointer, it means that the
	 * lower layer is dead. Change state and wakeup any waiting
	 * processes. */
	if (!skb){
		sk->sk_state = WANSOCK_BIND_LISTEN;
		SK_DATA_READY(sk, 0);
		return 0;
	}

	if (sk->sk_state != WANSOCK_LISTEN){
		printk(KERN_INFO "af_wanpipe: Listen rcv in disconnected state!\n");
		SK_DATA_READY(sk, 0);
		return -EINVAL;
	}

	/* Check if the receive buffer will hold the incoming
	 * call request */
	if (atomic_read(&sk->sk_rmem_alloc) + skb->truesize >= 
			(unsigned)sk->sk_rcvbuf){
		
		return -ENOMEM;
	}	

	
	sll = (struct wan_sockaddr_ll*)skb->cb;
	
	dev = skb->dev; 
	if (!dev){
		printk(KERN_INFO "af_wanpipe: LISTEN ERROR, No Free Device\n");
		return -ENODEV;
	}

	/* Allocate a new sock, which accept will bind
         * and pass up to the user 
	 */
	if ((newsk = wanpipe_make_new(sk)) == NULL){
		return -ENOMEM;
	}

	/* Bind the new socket into the lower layer. The lower
	 * layer will increment the sock reference count. */
	ifr.ifr_data = (void*)newsk;
	if (!WAN_NETDEV_TEST_IOCTL(dev) || WAN_NETDEV_IOCTL(dev,&ifr,SIOC_ANNEXG_BIND_SK) != 0){
		wanpipe_kill_sock(newsk);
		return -ENODEV;
	}

	wansk_set_zapped(newsk);
	newsk->sk_bound_dev_if = dev->ifindex;
	dev_hold(dev);
	SK_PRIV(newsk)->dev = dev;

	write_lock_irqsave(&wanpipe_sklist_lock,flags);
#ifdef LINUX_2_6
	sk_add_node(newsk, &wanpipe_sklist);
#else
	newsk->next = wanpipe_sklist;
	wanpipe_sklist = newsk;
	sock_hold(newsk);
#endif
	write_unlock_irqrestore(&wanpipe_sklist_lock,flags);

	/* Insert the sock into the main wanpipe
         * sock list.
         */
	atomic_inc(&wanpipe_socks_nr);

	/* Register the lcn on which incoming call came
         * from. Thus, if we have to clear it, we know
         * whic lcn to clear 
	 */ 

	//FIXME Obtain the LCN from the SKB buffer
	//x25_api_hdr = (x25api_hdr_t*)skb->data;
	
	SK_PRIV(newsk)->num = skb->protocol;
	newsk->sk_state = WANSOCK_CONNECTING;

	/* Fill in the standard sock address info */

	sll->sll_family = sk->sk_family;
	sll->sll_hatype = dev->type;
	sll->sll_protocol = SK_PRIV(newsk)->num;
	sll->sll_pkttype = skb->pkt_type;
	sll->sll_ifindex = dev->ifindex;
	sll->sll_halen = 0;

	sk->sk_ack_backlog++;

	/* We also store the new sock pointer in side the
	 * skb buffer, just incase the lower layer disapears,
	 * or gets unconfigured */
	sock_hold(newsk);
	*(unsigned long *)&skb->data[0]=(unsigned long)newsk;

	dev_put(skb->dev);
	skb->dev=NULL;

	AF_SKB_INC(skb->truesize);
	
	/* We must do this manually, since the sock_queue_rcv_skb()
	 * function sets the skb->dev to NULL.  However, we use
	 * the dev field in the accept function.*/ 

	skb_set_owner_r(skb, sk);
	SK_DATA_READY(sk, skb->len);
	skb_queue_tail(&sk->sk_receive_queue, skb);
	
	return 0;
}


/*======================================================================
 * wanpipe_listen
 *
 *	X25API Specific function. Set a socket into LISTENING  MODE.
 *=====================================================================*/


static int wanpipe_listen(struct socket *sock, int backlog)
{
	struct sock *sk = sock->sk;

 	/* This is x25 specific area if protocol doesn't
         * match, return error */
	if (SK_PRIV(sk)->num != htons(ETH_P_X25) && 
	    SK_PRIV(sk)->num != htons(WP_X25_PROT) && 
	    SK_PRIV(sk)->num != htons(DSP_PROT))
		return -EINVAL;

	if (sk->sk_state == WANSOCK_BIND_LISTEN) {
		sk->sk_max_ack_backlog = backlog;
		sk->sk_state           = WANSOCK_LISTEN;
		return 0;
	}else{
		printk(KERN_INFO "af_wanpipe: Listening sock was not binded\n");
	}

	return -EINVAL;
}




/*======================================================================
 * wanpipe_accept
 *
 *	ACCEPT() System call.	X25API Specific function. 
 *	For each incoming call, create a new socket and 
 *      return it to the user.	
 *=====================================================================*/

static int wanpipe_accept(struct socket *sock, struct socket *newsock, int flags)
{
	struct sock *sk;
	struct sock *newsk=NULL, *newsk_frmskb=NULL;
	struct sk_buff *skb;
	DECLARE_WAITQUEUE(wait, current);
	int err=-EINVAL;
	struct ifreq ifr;

	if (newsock->sk != NULL){
		printk(KERN_INFO "af_wanpipe: Accept is Releaseing an unused sock !\n");
		wanpipe_kill_sock(newsock->sk);
		newsock->sk=NULL;
	}
	
	if ((sk = sock->sk) == NULL)
		return -ENOTSOCK;

	if (sk->sk_type != SOCK_RAW)
		return -EOPNOTSUPP;

	if (sk->sk_state != WANSOCK_LISTEN)
		return -ENETDOWN;

	if (SK_PRIV(sk)->num != htons(ETH_P_X25) && SK_PRIV(sk)->num != htons(WP_X25_PROT) && SK_PRIV(sk)->num != htons(DSP_PROT))
		return -EPROTOTYPE;

	add_wait_queue(WAN_SK_SLEEP(sk),&wait);
	current->state = TASK_INTERRUPTIBLE;
	for (;;){
		skb = skb_dequeue(&sk->sk_receive_queue);
		if (skb){
			err=0;
			break;
		}
		
		if (sk->sk_state != WANSOCK_LISTEN){
			err = -ENETDOWN;
			break;
		}
		
		if (signal_pending(current)) {
			err = -ERESTARTSYS;
			break;
		}
		schedule();
	}
	current->state = TASK_RUNNING;
	remove_wait_queue(WAN_SK_SLEEP(sk),&wait);

	if (err != 0)
		return err;

	ifr.ifr_data=NULL;

	newsk=(struct sock*)*(unsigned long *)&skb->data[0];
	if (newsk){
		
		struct net_device *dev;
			
		/* Decrement the usage count of newsk, since
		 * we go the pointer for skb */
		__sock_put(newsk);
				
		if (wansk_is_zapped(newsk) && SK_PRIV(newsk) && 
		   (dev = (struct net_device *)SK_PRIV(newsk)->dev)){

			if (dev && WAN_NETDEV_TEST_IOCTL(dev)){
				struct sock* dev_sk;
				WAN_NETDEV_IOCTL(dev,&ifr,SIOC_ANNEXG_GET_SK);
				
				if ((dev_sk=(struct sock*)ifr.ifr_data)!=NULL){
					__sock_put(dev_sk);

					if (dev_sk == newsk){
						/* Got the right device with correct
						 * sk_id */
						goto accept_newsk_ok;
					}else{
						/* This should never happen */
						printk(KERN_INFO "af_wanpipe: Accept killing newsk, layer ptr mismatch!\n");
					}
				}else{
					printk(KERN_INFO "af_wanpipe: Accept killing newsk, get sk failed!\n");
				}

				ifr.ifr_data=(void*)newsk;
				if (WAN_NETDEV_IOCTL(dev,&ifr,SIOC_ANNEXG_UNBIND_SK)==0){
					WAN_NETDEV_IOCTL(dev,NULL,SIOC_ANNEXG_CLEAR_CALL);	
				}
			}else{
				printk(KERN_INFO "af_wanpipe: Accept killing newsk, lower layer down!\n");
			}
		}else{
			/* This can happen if the device goes down, before we get
			 * the chance to accept the call */
			printk(KERN_INFO "af_wanpipe: Accept killing newsk, newsk not zapped!\n");
		}

#ifdef LINUX_2_6
		sock_set_flag(newsk, SOCK_DEAD);
#else
		newsk->dead=1;
#endif

		wanpipe_kill_sock(newsk);

		KFREE_SKB(skb);
		return -EINVAL;
	}else{
		KFREE_SKB(skb);
		return -ENODEV;
	}

accept_newsk_ok:

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,9)
	newsk->sk_pair = NULL;
#endif
	newsk->sk_socket = newsock;
#if defined(CONFIG_RPS) && LINUX_VERSION_CODE > KERNEL_VERSION(2,6,32)
	WAN_SK_SLEEP(newsk);
#else
	newsk->sk_sleep = &newsock->wait;
#endif
	/* Now attach up the new socket */
	sk->sk_ack_backlog--;
	newsock->sk = newsk;

	if (newsk_frmskb){
		__sock_put(newsk_frmskb);
		newsk_frmskb=NULL;
	}
	KFREE_SKB(skb);
	return 0;
}




/*============================================================
 * wanpipe_api_sock_rcv
 *
 *	Wanpipe socket bottom half handler.  This function
 *      is called by the WANPIPE device drivers to queue a
 *      incomming packet into the socket receive queue. 
 *      Once the packet is queued, all processes waiting to 
 *      read are woken up.
 *
 *      During socket bind, this function is bounded into
 *      WANPIPE driver private.
 *===========================================================*/

static int wanpipe_api_sock_rcv(struct sk_buff *skb, netdevice_t *dev,  struct sock *sk)
{
	struct wan_sockaddr_ll *sll;
	/*
	 *	When we registered the protocol we saved the socket in the data
	 *	field for just this event.
	 */

	if (!skb){
		return -ENODEV;
	}

	if (!wansk_is_zapped(sk)){
		return -EINVAL;
	}

	sll = (struct wan_sockaddr_ll*)skb->cb;

	sll->sll_family = sk->sk_family;
	sll->sll_hatype = dev->type;
	sll->sll_protocol = skb->protocol;
	sll->sll_pkttype = skb->pkt_type;
	sll->sll_ifindex = dev->ifindex;
	sll->sll_halen = 0;

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,4,23) 
	if (dev->hard_header_parse)
		sll->sll_halen = dev->hard_header_parse(skb, sll->sll_addr);
#endif

	/* 
	 * WAN_PACKET_DATA : Data which should be passed up the receive queue.
         * WAN_PACKET_ASYC : Asynchronous data like place call, which should
         *                   be passed up the listening sock.
         * WAN_PACKET_ERR  : Asynchronous data like clear call or restart 
         *                   which should go into an error queue.
         */

	AF_SKB_INC(skb->truesize);
	switch (skb->pkt_type){

		case WAN_PACKET_DATA:

#ifdef CONFIG_PRODUCT_WANPIPE_SOCK_DATASCOPE
			if (SK_PRIV(sk) && DATA_SC(sk) && DATA_SC(sk)->parent){
#ifdef LINUX_2_6
				if (sock_flag(sk, SOCK_DEAD)){
#else				
				if (test_bit(0,&sk->dead)){
#endif
					return -ENODEV;
				}
				
				return wanpipe_sk_parent_rx(sk,skb);	
			}else{
				if (sock_queue_rcv_skb(sk,skb)<0){
					AF_SKB_DEC(skb->truesize);
					SK_DATA_READY(sk, 0);
					return -ENOMEM;
				}
			}
#else
			if (sock_queue_rcv_skb(sk,skb)<0){
				AF_SKB_DEC(skb->truesize);
				SK_DATA_READY(sk, 0);
				return -ENOMEM;
			}
#endif
			break;
		case WAN_PACKET_CMD:
			/* Bug fix: update Mar6. 
                         * Do not set the sock lcn number here, since
         		 * cmd is not guaranteed to be executed on the
                         * board, thus Lcn could be wrong */
			SK_DATA_READY(sk, skb->len);
			KFREE_SKB(skb);
			break;
		case WAN_PACKET_ERR:
			if (sock_queue_err_skb(sk,skb)<0){
				AF_SKB_DEC(skb->truesize);
				return -ENOMEM;
			}
			break;
		default:
			printk(KERN_INFO "af_wanpipe: BH Illegal Packet Type Dropping\n");
			KFREE_SKB(skb); 
			break;
	}

	return 0;
}

/*============================================================
 * wanpipe_alloc_socket
 *
 *	Allocate memory for the a new sock, and sock
 *      private data.  
 *	
 *	Increment the module use count.
 *       	
 *===========================================================*/

#ifdef sk_net_refcnt
static struct sock *wanpipe_alloc_socket(struct sock *osk, void *net, int kern)
#else
static struct sock *wanpipe_alloc_socket(struct sock *osk, void *net)
#endif
{
	struct sock *sk;
	struct wanpipe_opt *wan_opt;

#ifdef LINUX_2_6

# if defined(LINUX_FEAT_2624)	
	if (!osk && !net) {
		DEBUG_EVENT("%s:%d ASSERT osk net pointer = NULL! \n",
			__FUNCTION__,__LINE__);
		return NULL;
	}

	if (osk) {
		net=wan_sock_net(osk);
	}

#ifdef sk_net_refcnt
	sk = sk_alloc((struct net*)net, PF_WANPIPE, GFP_ATOMIC, &packet_proto, kern);
#else
	sk = sk_alloc((struct net*)net, PF_WANPIPE, GFP_ATOMIC, &packet_proto);
#endif

# elif defined(AF_WANPIPE_2612_UPDATE)
	sk = sk_alloc(PF_WANPIPE, GFP_ATOMIC, &packet_proto,1);
# else
	sk = sk_alloc(PF_WANPIPE, GFP_ATOMIC, 1,NULL);
# endif

#else
	sk = sk_alloc(PF_WANPIPE, GFP_ATOMIC, 1);
#endif

	if (sk == NULL) {
		return NULL;
	}

	if ((wan_opt = kmalloc(sizeof(struct wanpipe_opt), GFP_ATOMIC)) == NULL) {
		sk_free(sk);
		return NULL;
	}

	memset(wan_opt, 0x00, sizeof(struct wanpipe_opt));

	SK_PRIV_INIT(sk,wan_opt);
	
	AF_MEM_INC(sizeof(struct sock));
	AF_MEM_INC(sizeof(struct wanpipe_opt));

#ifndef LINUX_2_6
	MOD_INC_USE_COUNT;
#endif

	sock_init_data(NULL, sk);
	return sk;
}


/*============================================================
 * wanpipe_sendmsg
 *
 *	This function implements a sendto() system call,
 *      for AF_WANPIPE socket family. 
 *      During socket bind() sk->sk_bound_dev_if is initialized
 *      to a correct network device. This number is used
 *      to find a network device to which the packet should
 *      be passed to.
 *
 *      Each packet is queued into sk->write_queue and 
 *      delayed transmit bottom half handler is marked for 
 *      execution.
 *
 *      A socket must be in WANSOCK_CONNECTED state before
 *      a packet is queued into sk->write_queue.
 *===========================================================*/

#ifdef sk_cookie
static int wanpipe_sendmsg(struct socket *sock,
			       struct msghdr *msg, size_t len)

#elif defined(LINUX_2_6)
static int wanpipe_sendmsg(struct kiocb *iocb, struct socket *sock,
			       struct msghdr *msg, size_t len)

#else
static int wanpipe_sendmsg(struct socket *sock, struct msghdr *msg, int len,
			  struct scm_cookie *scm)
#endif
{
	struct sock *sk = sock->sk;
	struct wan_sockaddr_ll *saddr=(struct wan_sockaddr_ll *)msg->msg_name;
	struct sk_buff *skb;
	netdevice_t *dev;
	unsigned short proto;
	unsigned char *addr;
	int ifindex, err=-EINVAL, reserve = 0;

	WAN_ASSERT_EINVAL(!SK_PRIV(sk));
	
	if (!wansk_is_zapped(sk))
		return -ENETDOWN;

	if (sk->sk_state != WANSOCK_CONNECTED)
		return -ENOTCONN;	

	if (msg->msg_flags&~MSG_DONTWAIT) 
		return(-EINVAL);

	if ((SK_PRIV(sk)->num == htons(ETH_P_X25) ||
	     SK_PRIV(sk)->num == htons(WP_X25_PROT)) && (len < sizeof(x25api_hdr_t)))
		return -EINVAL;

	if (SK_PRIV(sk)->num == htons(DSP_PROT) && (len < sizeof(dspapi_hdr_t)))
		return -EINVAL;

	if (len < WP_API_HDR_SZ){
		return -EINVAL;
	}

	if (saddr == NULL) {
		ifindex	= sk->sk_bound_dev_if;
		proto	= SK_PRIV(sk)->num;
		addr	= NULL;

	}else{
		if (msg->msg_namelen < sizeof(struct wan_sockaddr_ll)){ 
			return -EINVAL;
		}

		ifindex = sk->sk_bound_dev_if;
		proto	= saddr->sll_protocol;
		addr	= saddr->sll_addr;
	}

	dev = (struct net_device *)SK_PRIV(sk)->dev;
	if (dev == NULL){
		printk(KERN_INFO "af_wanpipe: Send failed, dev index: %i\n",ifindex);
		return -ENXIO;
	}

	if (netif_queue_stopped(dev))
		return -EBUSY;
	
	if (sock->type == SOCK_RAW)
		reserve = dev->hard_header_len;

	if (len > dev->mtu+reserve){
  		return -EMSGSIZE;
	}

#if defined(LINUX_2_1)
	dev_lock_list();
#endif
      
	ALLOC_SKB(skb,len+dev->hard_header_len+15);
	if (skb==NULL){
		goto out_unlock;
	}
		
	skb_reserve(skb, (dev->hard_header_len+15)&~15);
	wan_skb_reset_network_header(skb);

	/* Returns -EFAULT on error */
	err = memcpy_from_msg(skb_put(skb,len), msg, len);
	if (err){
		goto out_free;
	}

#ifndef LINUX_FEAT_2624
	if (dev->hard_header) {
		int res;
		err = -EINVAL;
		res = dev->hard_header(skb, dev, ntohs(proto), addr, NULL, len);
		if (res<0){
			goto out_free;
		}
	}
#endif

	skb->protocol = proto;
	skb->priority = sk->sk_priority;
	skb->pkt_type = WAN_PACKET_DATA;

	err = -ENETDOWN;
	if (!(dev->flags & IFF_UP))
		goto out_free;

#if defined(LINUX_2_1)
	dev_unlock_list();
#endif

	AF_SKB_DEC(skb->truesize);
	if (!WAN_NETDEV_XMIT(skb,dev)){
		return(len);
	}else{
		err = -EBUSY;
	}
	AF_SKB_INC(skb->truesize);
	
out_free:
	KFREE_SKB(skb);
out_unlock:
#if defined(LINUX_2_1)
	dev_unlock_list();
#endif
	return err;
}

/*============================================================
 * wanpipe_kill_sock 
 *
 *	Used by wanpipe_release, to delay release of
 *      the socket.
 *===========================================================*/
void wanpipe_kill_sock (struct sock *sk)
{
#ifndef LINUX_2_6
	struct sock **skp;
#endif
	unsigned long flags;
	
	if (!sk)
		return;

	WAN_ASSERT_VOID(!SK_PRIV(sk));
	
	sk->sk_state = WANSOCK_DISCONNECTED;
	wansk_reset_zapped(sk);
	sk->sk_bound_dev_if=0;
	if (SK_PRIV(sk)->dev){
		dev_put(SK_PRIV(sk)->dev);
	}
	SK_PRIV(sk)->dev=NULL;
	
	/* This functin can be called from interrupt. We must use
	 * appropriate locks */
	
	write_lock_irqsave(&wanpipe_sklist_lock,flags);
#ifdef LINUX_2_6
	sk_del_node_init(sk);	
#else
	for (skp = &wanpipe_sklist; *skp; skp = &(*skp)->next) {
		if (*skp == sk) {
			*skp = sk->next;
			__sock_put(sk);
			break;
		}
	}
#endif
	write_unlock_irqrestore(&wanpipe_sklist_lock,flags);

	sk->sk_socket = NULL;

	/* Purge queues */
	af_skb_queue_purge(&sk->sk_receive_queue);
	af_skb_queue_purge(&sk->sk_error_queue);

	KFREE(SK_PRIV(sk));
	SK_PRIV_INIT(sk,NULL);
	
#if defined(LINUX_2_4)||defined(LINUX_2_6)
	if (atomic_read(&sk->sk_refcnt) != 1){
		DEBUG_TEST("af_wanpipe: Error, wrong reference count: %i ! :Kill.\n",
					atomic_read(&sk->sk_refcnt));
		atomic_set(&sk->sk_refcnt,1);
	}
	sock_put(sk);
#else
	sk_free(sk);
#endif
	AF_MEM_DEC(sizeof(struct sock));
	atomic_dec(&wanpipe_socks_nr);

#ifndef LINUX_2_6
	MOD_DEC_USE_COUNT;
#endif
	return;
}


static void release_queued_pending_sockets(struct sock *sk)
{
	netdevice_t *dev;
	struct sk_buff *skb;
	struct sock *deadsk;
	
	while ((skb=skb_dequeue(&sk->sk_receive_queue))!=NULL){
		deadsk=(struct sock *)*(unsigned long*)&skb->data[0];
		if (!deadsk){
			KFREE_SKB(skb);
			continue;
		}
#ifdef LINUX_2_6
		sock_set_flag(deadsk, SOCK_DEAD);
#else		
		deadsk->dead=1;
#endif
		printk (KERN_INFO "af_wanpipe: Release: found dead sock!\n");

		if (SK_PRIV(deadsk)){
			dev = (struct net_device *)SK_PRIV(deadsk)->dev;
			if (dev && WAN_NETDEV_TEST_IOCTL(dev)){
				struct ifreq ifr;
				ifr.ifr_data=(void*)sk;
				if (WAN_NETDEV_IOCTL(dev,&ifr,SIOC_ANNEXG_UNBIND_SK)==0){
					WAN_NETDEV_IOCTL(dev,NULL,SIOC_ANNEXG_CLEAR_CALL);
				}
			}	
		}

		/* Decrement the sock refcnt, since we took
		 * the pointer out of the skb buffer */
		__sock_put(deadsk);
		wanpipe_kill_sock(deadsk);
		KFREE_SKB(skb);
	}
}


/*============================================================
 * wanpipe_release
 *
 *	Close a PACKET socket. This is fairly simple. We 
 *      immediately go to 'closed' state and remove our 
 *      protocol entry in the device list.
 *===========================================================*/

#if defined(LINUX_2_4)||defined(LINUX_2_6)
static int wanpipe_release(struct socket *sock)
#else
static int wanpipe_release(struct socket *sock, struct socket *peersock)
#endif
{
	
#if defined(LINUX_2_1)
	struct sk_buff	*skb;
#endif
	struct sock *sk = sock->sk;
#ifndef LINUX_2_6
	struct sock **skp;
#endif
	unsigned long flags;
	
	if (!sk)
		return 0;

	WAN_ASSERT_EINVAL(!SK_PRIV(sk));
	
	check_write_queue(sk);

#ifdef CONFIG_PRODUCT_WANPIPE_SOCK_DATASCOPE
	if (SK_PRIV(sk) && DATA_SC(sk) && DATA_SC(sk)->parent_sk != NULL){
		wansk_reset_zapped(sk);
		wanpipe_unbind_sk_from_parent(sk);
	}
#endif


	/* Kill the tx timer, if we don't kill it now, the timer
         * will run after we kill the sock.  Timer code will 
         * try to access the sock which has been killed and cause
         * kernel panic */

	/*
	 *	Unhook packet receive handler.
	 */

	if (sk->sk_state == WANSOCK_LISTEN || sk->sk_state == WANSOCK_BIND_LISTEN){
		unbind_api_listen_from_protocol	(SK_PRIV(sk)->num,sk);
		if (SK_PRIV(sk)->dev){
			dev_put(SK_PRIV(sk)->dev);
			SK_PRIV(sk)->dev=NULL;
		}
		release_queued_pending_sockets(sk);
		
	}else if ((SK_PRIV(sk)->num == htons(ETH_P_X25) ||
		   SK_PRIV(sk)->num == htons(WP_X25_PROT) || 
		   SK_PRIV(sk)->num == htons(DSP_PROT)) && wansk_is_zapped(sk)){

		netdevice_t *dev = (struct net_device *)SK_PRIV(sk)->dev;
		if (dev){
			if(WAN_NETDEV_TEST_IOCTL(dev)){
				struct ifreq ifr;
				ifr.ifr_data=(void*)sk;
				if (WAN_NETDEV_IOCTL(dev,&ifr,SIOC_ANNEXG_UNBIND_SK)==0){
					WAN_NETDEV_IOCTL(dev,NULL,SIOC_ANNEXG_CLEAR_CALL);
				}
			}
		}else{
			DEBUG_EVENT("%s: No dev on svc release !\n",__FUNCTION__);
		}
	}else if (wansk_is_zapped(sk)){
		netdevice_t *dev = (struct net_device *)SK_PRIV(sk)->dev;
		if (dev){
			if(WAN_NETDEV_TEST_IOCTL(dev)){
				struct ifreq ifr;
				ifr.ifr_data=(void*)sk;
				WAN_NETDEV_IOCTL(dev,&ifr,SIOC_ANNEXG_UNBIND_SK);
			}
		}else{
			DEBUG_EVENT("%s: No dev on pvc release !\n",__FUNCTION__);
		}
	}

	sk->sk_state = WANSOCK_DISCONNECTED;
	wansk_reset_zapped(sk);
	sk->sk_bound_dev_if=0;
	if (SK_PRIV(sk)->dev){
		dev_put((struct net_device*)SK_PRIV(sk)->dev);
	}

	SK_PRIV(sk)->dev=NULL;
	
	//FIXME with spinlock_irqsave
	write_lock_irqsave(&wanpipe_sklist_lock,flags);
#ifdef LINUX_2_6
	sk_del_node_init(sk);	
#else
	for (skp = &wanpipe_sklist; *skp; skp = &(*skp)->next) {
		if (*skp == sk) {
			*skp = sk->next;
			__sock_put(sk);
			break;
		}
	}
#endif
	write_unlock_irqrestore(&wanpipe_sklist_lock,flags);

	/*
	 *	Now the socket is dead. No more input will appear.
	 */

	sk->sk_state_change(sk);	/* It is useless. Just for sanity. */
	
	sock->sk = NULL;
	sk->sk_socket = NULL;

#ifdef LINUX_2_6
	sock_set_flag(sk, SOCK_DEAD);
#else
	sk->dead=1;
#endif

	/* Purge queues */
	af_skb_queue_purge(&sk->sk_receive_queue);
	af_skb_queue_purge(&sk->sk_error_queue);


	KFREE(SK_PRIV(sk));
	SK_PRIV_INIT(sk,NULL);
	
#if defined(LINUX_2_4)||defined(LINUX_2_6)
	if (atomic_read(&sk->sk_refcnt) != 1){
		DEBUG_EVENT("af_wanpipe: Error, wrong reference count: %i !:release.\n",
					atomic_read(&sk->sk_refcnt));
		atomic_set(&sk->sk_refcnt,1);
	}
	sock_put(sk);
#else	
	sk_free(sk);
#endif
	AF_MEM_DEC(sizeof(struct sock));
	atomic_dec(&wanpipe_socks_nr);

#ifndef LINUX_2_6
	MOD_DEC_USE_COUNT;
#endif
	
	return 0;
}

/*============================================================
 * check_write_queue
 *
 *  	During sock shutdown, if the sock state is 
 *      WANSOCK_CONNECTED and there is transmit data 
 *      pending. Wait until data is released 
 *      before proceeding.
 *===========================================================*/

static void check_write_queue(struct sock *sk)
{

	if (sk->sk_state != WANSOCK_CONNECTED)
		return;

	if (!atomic_read(&sk->sk_wmem_alloc))
		return;

	printk(KERN_INFO "af_wanpipe: MAJOR ERROR, Data lost on sock release !!!\n");

}


/*============================================================
 *  wanpipe_bind
 *
 *      BIND() System call, which is bound to the AF_WANPIPE
 *      operations structure.  It checks for correct wanpipe
 *      card name, and cross references interface names with
 *      the card names.  Thus, interface name must belong to
 *      the actual card.
 *===========================================================*/


static int wanpipe_bind(struct socket *sock, struct sockaddr *uaddr, int addr_len)
{
	struct wan_sockaddr_ll *sll = (struct wan_sockaddr_ll*)uaddr;
	struct sock *sk=sock->sk;
	netdevice_t *dev = NULL;
	char name[15];
	int err=-EINVAL;

	/*
	 *	Check legality
	 */
	
	if (addr_len < sizeof(struct wan_sockaddr_ll)){
		printk(KERN_INFO "af_wanpipe: Address length error\n");
		return -EINVAL;
	}
	if (sll->sll_family != AF_WANPIPE){
		printk(KERN_INFO "af_wanpipe: Illegal family name specified.\n");
		return -EINVAL;
	}

	WAN_ASSERT_EINVAL(!SK_PRIV(sk));

	sk->sk_family = sll->sll_family;
	SK_PRIV(sk)->num = sll->sll_protocol ? sll->sll_protocol : SK_PRIV(sk)->num;

	if (!strcmp(sll->sll_device,"svc_connect") ||
	    !strcmp(sll->sll_device,"dsp_connect")){

		strncpy(name,sll->sll_card,14);
		name[14]=0;

		if (SK_PRIV(sk)->num == htons(WP_X25_PROT)){
			SK_PRIV(sk)->num = htons(ETH_P_X25);
		}
		
		/* Obtain the master device, in case of Annexg this
		 * device would be a lapb network interface, note 
		 * the usage count on dev will be incremented, once
		 * we are finshed with the device we should run
		 * dev_put() to release it */
#if defined(LINUX_2_4)||defined(LINUX_2_6)
		dev = wan_dev_get_by_name(name);
#else
		dev = dev_get(name);
#endif
		if (dev == NULL){

			sk->sk_bound_dev_if = bind_api_to_protocol(NULL,
					                        sll->sll_card,
								htons(WP_X25_PROT),
								(void*)sk);
			if (sk->sk_bound_dev_if >= 0){
				SK_PRIV(sk)->num = htons(WP_X25_PROT);
				goto wanpipe_svc_connect_skip;
			}
				
			printk(KERN_INFO "af_wanpipe: Failed to get Dev from name: %s,\n",
					name);
			return -ENODEV;
		}

		if (!(dev->flags&IFF_UP)){
			printk(KERN_INFO "af_wanpipe: Bind device %s is down!\n",
					name);
			dev_put(dev);
			return -ENODEV;
		}
		
		sk->sk_bound_dev_if = bind_api_to_protocol(dev,sll->sll_card,SK_PRIV(sk)->num,(void*)sk);	
		
		/* We are finished with the lapb master device, we
		 * need it to find a free svc or dsp device but now
		 * we can release it */
		dev_put(dev);
	
wanpipe_svc_connect_skip:
		
		if (sk->sk_bound_dev_if < 0){
			sk->sk_bound_dev_if=0;
			err=-EINVAL;
		}else{
			sk->sk_state = WANSOCK_DISCONNECTED;
			SK_PRIV(sk)->dev=wan_dev_get_by_index(sk->sk_bound_dev_if);
			if (SK_PRIV(sk)->dev){
				err=0;
			}else{
				/* This should never ever happen !*/
				err=-EINVAL;
			}
		}
		
	}else if (!strcmp(sll->sll_device,"svc_listen") ||
		  !strcmp(sll->sll_device,"dsp_listen")){

		strncpy(name,sll->sll_card,14);
		name[14]=0;

		if (SK_PRIV(sk)->num == htons(WP_X25_PROT)){
			SK_PRIV(sk)->num = htons(ETH_P_X25);
		}

		/* Obtain the master device, in case of Annexg this
		 * device would be a lapb network interface, note 
		 * the usage count on dev will be incremented, once
		 * we are finshed with the device we should run
		 * dev_put() to release it */

#if defined(LINUX_2_4)||defined(LINUX_2_6)
		dev = wan_dev_get_by_name(name);
#else
		dev = dev_get(name);
#endif
		if (dev == NULL){

			err = bind_api_listen_to_protocol(NULL,sll->sll_card,
				                          htons(WP_X25_PROT),
							  (void*)sk);
			if (err==0){
				SK_PRIV(sk)->num = htons(WP_X25_PROT);
				goto wanpipe_svc_listen_skip;
			}
			
			printk(KERN_INFO "af_wanpipe: Failed to get Dev from name: %s,\n",
					name);
			return -ENODEV;
		}

		if (!(dev->flags&IFF_UP)){
			printk(KERN_INFO "af_wanpipe: Bind device %s is down!\n",
					name);
			dev_put(dev);
			return -ENODEV;
		}

		err = bind_api_listen_to_protocol(dev,sll->sll_card,SK_PRIV(sk)->num,sk);	

		SK_PRIV(sk)->dev=dev;
		sk->sk_bound_dev_if=dev->ifindex;

wanpipe_svc_listen_skip:

		if (!err){
			sk->sk_state = WANSOCK_BIND_LISTEN;
		}

	
	}else{
		struct ifreq ifr;
		strncpy(name,sll->sll_device,14);
		name[14]=0;
	
#if defined(LINUX_2_4)||defined(LINUX_2_6)
		dev = wan_dev_get_by_name(name);
#else
		dev = dev_get(name);
#endif
		if (dev == NULL){
			printk(KERN_INFO "af_wanpipe: Failed to get Dev from name: %s,\n",
					name);
			return -ENODEV;
		}

		if (!(dev->flags&IFF_UP)){
			printk(KERN_INFO "af_wanpipe: Bind device %s is down!\n",
					name);
			dev_put(dev);
			return -ENODEV;
		}

		ifr.ifr_data = (void*)sk;
		err=-ENODEV;

#ifdef CONFIG_PRODUCT_WANPIPE_SOCK_DATASCOPE
		if (SK_PRIV(sk)->num == htons(SS7_MONITOR_PROT)){
			return wanpipe_bind_sk_to_parent(sk,dev,sll);
		}
#endif
		
		if (WAN_NETDEV_TEST_IOCTL(dev))
			err=WAN_NETDEV_IOCTL(dev,&ifr,SIOC_ANNEXG_BIND_SK);
		
		if (err == 0){
			sk->sk_bound_dev_if = dev->ifindex; 
			SK_PRIV(sk)->dev = dev;


			if (SK_PRIV(sk)->num == htons(ETH_P_X25) || 
			    SK_PRIV(sk)->num == htons(WP_X25_PROT) ||
			    SK_PRIV(sk)->num == htons(DSP_PROT)){
				sk->sk_state = WANSOCK_DISCONNECTED;
			}else{
				err=WAN_NETDEV_IOCTL(dev,&ifr,SIOC_WANPIPE_DEV_STATE);
				if (err == WANSOCK_CONNECTED){
					sk->sk_state = WANSOCK_CONNECTED;
				}else{
					sk->sk_state = WANSOCK_CONNECTING;
				}
				err=0;
			}
		}else{
			dev_put(dev);
		}
	}

	if (!err){
		wansk_set_zapped(sk);
	}
	
//	printk(KERN_INFO "11-11 Bind Socket Prot %i, X25=%i Zapped %i, Bind Dev %i Sock %u!\n",
//			SK_PRIV(sk)->num,htons(ETH_P_X25),sk->sk_zapped,sk->sk_bound_dev_if,(u32)sk);
	
	return err;
}

/*============================================================
 *  wanpipe_create
 *	
 * 	SOCKET() System call.  It allocates a sock structure
 *      and adds the socket to the wanpipe_sk_list. 
 *      Crates AF_WANPIPE socket.
 *===========================================================*/

#if defined(DECLARE_SOCKADDR) || LINUX_VERSION_CODE > KERNEL_VERSION(2,6,31)	
int wanpipe_create(struct net *net, struct socket *sock, int protocol, int kern)
#elif defined(LINUX_FEAT_2624) 
int wanpipe_create(struct net *net, struct socket *sock, int protocol)
#else
int wanpipe_create(struct socket *sock, int protocol)
#endif
{
	struct sock *sk;
	unsigned long flags;

#ifndef LINUX_FEAT_2624 
	/* Used to fake the net structure for lower kernels */
	void *net = NULL;
#endif

	if (sock->type != SOCK_RAW)
		return -ESOCKTNOSUPPORT;

	sock->state = SS_UNCONNECTED;
	
#ifdef sk_net_refcnt
	if ((sk = wanpipe_alloc_socket(NULL, net, kern)) == NULL){
#else
	if ((sk = wanpipe_alloc_socket(NULL, net)) == NULL){
#endif
		return -ENOMEM;
	}

	sk->sk_reuse = 1;
	sock->ops = &wanpipe_ops;
	sock_init_data(sock,sk);

	wansk_reset_zapped(sk);
	sk->sk_family = AF_WANPIPE;
	SK_PRIV(sk)->num = protocol;
	sk->sk_state = WANSOCK_UNCONFIGURED;
	sk->sk_ack_backlog = 0;
	sk->sk_bound_dev_if=0;
	SK_PRIV(sk)->dev=NULL;

	atomic_inc(&wanpipe_socks_nr);
	
	/* We must disable interrupts because the ISR
	 * can also change the list */
	
	write_lock_irqsave(&wanpipe_sklist_lock,flags);
#ifdef LINUX_2_6
	sk_add_node(sk, &wanpipe_sklist);
#else
	sk->next = wanpipe_sklist;
	wanpipe_sklist = sk;
	sock_hold(sk);
#endif
	write_unlock_irqrestore(&wanpipe_sklist_lock,flags);
	
	return(0);
}


/*============================================================
 *  wanpipe_recvmsg
 *	
 *	Pull a packet from our receive queue and hand it 
 *      to the user. If necessary we block.
 *===========================================================*/

#ifdef sk_cookie
static int wanpipe_recvmsg(struct socket *sock,
			  struct msghdr *msg, size_t len, int flags)
#elif defined(LINUX_2_6)

static int wanpipe_recvmsg(struct kiocb *iocb, struct socket *sock,
			  struct msghdr *msg, size_t len, int flags)
#else
static int wanpipe_recvmsg(struct socket *sock, struct msghdr *msg, int len,
			  int flags, struct scm_cookie *scm)
#endif
{
	struct sock *sk = sock->sk;
	struct sk_buff *skb;
	int copied, err=-ENOBUFS;
	struct net_device *dev=NULL;

	WAN_ASSERT_EINVAL(!SK_PRIV(sk));

	if (!wansk_is_zapped(sk))
		return -ENETDOWN;

	/*
	 *	If the address length field is there to be filled in, we fill
	 *	it in now.
	 */

	msg->msg_namelen = sizeof(struct wan_sockaddr_ll);

	/*
	 *	Call the generic datagram receiver. This handles all sorts
	 *	of horrible races and re-entrancy so we can forget about it
	 *	in the protocol layers.
	 *
	 *	Now it will return ENETDOWN, if device have just gone down,
	 *	but then it will block.
	 */

	if (flags & MSG_OOB){	
		skb=skb_dequeue(&sk->sk_error_queue);
	}else{
		skb=skb_recv_datagram(sk,flags,1,&err);
	}
	/*
	 *	An error occurred so return it. Because skb_recv_datagram() 
	 *	handles the blocking we don't see and worry about blocking
	 *	retries.
	 */

	if(skb==NULL)
		goto out;


	dev = (struct net_device *)SK_PRIV(sk)->dev;
	if (dev){
		if (WAN_NETDEV_TEST_IOCTL(dev)){
			WAN_NETDEV_IOCTL(dev,NULL,SIOC_ANNEXG_KICK);
		}
	}
	
	/*
	 *	You lose any data beyond the buffer you gave. If it worries a
	 *	user program they can ask the device for its MTU anyway.
	 */

	copied = skb->len;
	if (copied > len)
	{
		copied=len;
		msg->msg_flags|=MSG_TRUNC;
	}

	/* We can't use skb_copy_datagram here */
	err = memcpy_to_msg(msg, skb->data, copied);
	if (err)
		goto out_free;
	
#ifdef LINUX_2_1
	sk->stamp=skb->stamp;
#else
	sock_recv_timestamp(msg, sk, skb);
#endif
	
	if (msg->msg_name)
		memcpy(msg->msg_name, skb->cb, msg->msg_namelen);

	/*
	 *	Free or return the buffer as appropriate. Again this
	 *	hides all the races and re-entrancy issues from us.
	 */
	err = (flags&MSG_TRUNC) ? skb->len : copied;

out_free:
	AF_SKB_DEC(skb->truesize);
	skb_free_datagram(sk, skb);
out:
	return err;
}

/*============================================================
 *  wanpipe_getname
 *	
 * 	I don't know what to do with this yet. 
 *      User can use this function to get sock address
 *      information. Not very useful for Sangoma's purposes.
 *===========================================================*/


static int wanpipe_getname(struct socket *sock, struct sockaddr *uaddr,
			  int *uaddr_len, int peer)
{
	netdevice_t *dev;
	struct sock *sk = sock->sk;
	struct wan_sockaddr_ll *sll = (struct wan_sockaddr_ll*)uaddr;

	WAN_ASSERT_EINVAL(!SK_PRIV(sk));
	
	sll->sll_family = sk->sk_family;
	sll->sll_ifindex = sk->sk_bound_dev_if;
	sll->sll_protocol = SK_PRIV(sk)->num;
	dev = (struct net_device *)SK_PRIV(sk)->dev;
	if (dev) {
		sll->sll_hatype = dev->type;
		sll->sll_halen = dev->addr_len;
		memcpy(sll->sll_addr, dev->dev_addr, dev->addr_len);
	} else {
		sll->sll_hatype = 0;	/* Bad: we have no ARPHRD_UNSPEC */
		sll->sll_halen = 0;
	}
	*uaddr_len = sizeof(*sll);
	
	return 0;
}

/*============================================================
 *  wanpipe_notifier
 *	
 *	If driver turns off network interface, this function
 *      will be envoked. Currently I treate it as a 
 *      call disconnect. More thought should go into this
 *      function.
 *
 * FIXME: More thought should go into this function.
 *
 *===========================================================*/

int wanpipe_notifier(struct notifier_block *this, unsigned long msg, void *data)
{
	struct sock *sk;
#if KERN_SK_FOR_NODE_FEATURE > 0
	struct hlist_node *node;
#endif
	netdevice_t *dev = (netdevice_t*)data;

	if (dev==NULL){
		DEBUG_EVENT("af_wanpiep:%s: Dev==NULL!\n",__FUNCTION__);
		return NOTIFY_DONE;
	}	

	read_lock(&wanpipe_sklist_lock);
#if KERN_SK_FOR_NODE_FEATURE == 0
	sk_for_each(sk, &wanpipe_sklist) {
#elif defined(LINUX_2_6)
	sk_for_each(sk, node, &wanpipe_sklist) {
#else
	for (sk = wanpipe_sklist; sk; sk = sk->next) {
#endif
		switch (msg) {
		case NETDEV_DOWN:
		case NETDEV_UNREGISTER:
			if (dev->ifindex == sk->sk_bound_dev_if) {
				
				printk(KERN_INFO "af_wanpipe: Device down %s\n",dev->name);

				if (sk->sk_state != WANSOCK_LISTEN && 
				    sk->sk_state != WANSOCK_BIND_LISTEN){
					sk->sk_state = WANSOCK_DISCONNECTED;
				}
				sk->sk_bound_dev_if = 0;
				wansk_reset_zapped(sk);
				
				if (SK_PRIV(sk) && SK_PRIV(sk)->dev){
					dev_put((struct net_device *)SK_PRIV(sk)->dev); 
					SK_PRIV(sk)->dev=NULL;
				}
				SK_DATA_READY(sk, 0);
			}
			break;
		}
	}
	read_unlock(&wanpipe_sklist_lock);

	return NOTIFY_DONE;
}

/*======================================================================
 * wanpipe_poll
 *
 *	Datagram poll: Again totally generic. This also handles
 *	sequenced packet sockets providing the socket receive queue
 *	is only ever holding data ready to receive.
 *
 *	Note: when you _don't_ use this routine for this protocol,
 *	and you use a different write policy from sock_writeable()
 *	then please supply your own write_space callback.
 *=====================================================================*/

static unsigned int wanpipe_poll(struct file * file, struct socket *sock, poll_table *wait)
{
	struct sock *sk = sock->sk;
	unsigned int mask=0;
	struct net_device *dev=NULL;

	if (!sk){
		return 0;
	}

	if (!SK_PRIV(sk) || !wansk_is_zapped(sk)){
		mask |= POLLPRI;
		return mask;
	}

	DEBUG_TX("%s: Sock State %p = %d\n",
			__FUNCTION__,sk,sk->sk_state);

	poll_wait(file, WAN_SK_SLEEP(sk), wait);

	/* exceptional events? */
	if (!SK_PRIV(sk) || 
	    !wansk_is_zapped(sk)  || 
	    sk->sk_err      || 
	    !skb_queue_empty(&sk->sk_error_queue)){
		mask |= POLLPRI;
		return mask;
	}
	if (sk->sk_shutdown & RCV_SHUTDOWN){
		mask |= POLLHUP;
	}

	/* readable? */
	if (!skb_queue_empty(&sk->sk_receive_queue)){
		mask |= POLLIN | POLLRDNORM;
	}
	
	/* connection hasn't started yet */
	if (sk->sk_state == WANSOCK_CONNECTING || sk->sk_state == WANSOCK_LISTEN){
		DEBUG_TEST("%s: Sk state connecting\n",__FUNCTION__);
		return mask;
	}

	if (sk->sk_state != WANSOCK_CONNECTED){
		mask = POLLPRI;
		DEBUG_TEST("%s: Sock not connected event on sock %p State=%i\n",
				__FUNCTION__,sk,sk->sk_state); 
		return mask;
	}
	
	if ((dev = (struct net_device *)SK_PRIV(sk)->dev) == NULL){
		printk(KERN_INFO "af_wanpipe: No Device found in POLL!\n");
		return mask;
	}

	if (!(dev->flags & IFF_UP))
		return mask;
	
	if (!netif_queue_stopped(dev)){
		DEBUG_TEST("%s: Dev stopped\n",__FUNCTION__);
		mask |= POLLOUT | POLLWRNORM | POLLWRBAND;
	}else{
#if defined(LINUX_2_4)||defined(LINUX_2_6)
		set_bit(SOCK_ASYNC_NOSPACE, &sk->sk_socket->flags);
#else
 		sk->sk_socket->flags |= SO_NOSPACE;
#endif
	}
	
	return mask;
}

static int wanpipe_api_connected(struct net_device *dev, struct sock *sk)
{
	if (sk == NULL || dev == NULL)
		return -EINVAL;

	if (!wansk_is_zapped(sk))
		return -EINVAL;

	DEBUG_TEST("%s: API Connected!\n",__FUNCTION__);
	
	sk->sk_state = WANSOCK_CONNECTED;
	SK_DATA_READY(sk, 0);
	return 0;
}

static int wanpipe_api_connecting(struct net_device *dev, struct sock *sk)
{
	if (sk == NULL || dev == NULL)
		return -EINVAL;

	if (!wansk_is_zapped(sk))
		return -EINVAL;

	DEBUG_TEST("%s: API Connecting!\n",__FUNCTION__);
	
	sk->sk_state = WANSOCK_CONNECTING;
	SK_DATA_READY(sk, 0);
	return 0;
}


static int wanpipe_api_disconnected(struct sock *sk)
{
	if (sk == NULL)
		return -EINVAL;


	if (sk->sk_state == WANSOCK_BIND_LISTEN ||
	    sk->sk_state == WANSOCK_LISTEN) {
		
		sk->sk_state = WANSOCK_DISCONNECTED;
		SK_DATA_READY(sk, 0);
		return 0;
	}
	
	
	if (!wansk_is_zapped(sk)){
		return -EINVAL;
	}
	
	DEBUG_TEST("%s: API Disconnected!\n",__FUNCTION__);
	
	sk->sk_state = WANSOCK_DISCONNECTED;

	SK_DATA_READY(sk, 0);
	return 0;
}


/*======================================================================
 *  wanpipe_connect
 *
 *  	CONNECT() System Call. X25API specific function
 * 	Check the state of the sock, and execute PLACE_CALL command.
 *      Connect can ether block or return without waiting for connection, 
 *      if specified by user.
 *=====================================================================*/

static int wanpipe_connect(struct socket *sock, struct sockaddr *uaddr, int addr_len, int flags)
{
	struct sock *sk = sock->sk;
	struct wan_sockaddr_ll *addr = (struct wan_sockaddr_ll*)uaddr;
	netdevice_t *dev;
	int err;

	if (!wansk_is_zapped(sk)){		/* Must bind first - autobinding does not work */
		return -EINVAL;
	}
	
	WAN_ASSERT_EINVAL(!SK_PRIV(sk));
	
	if (SK_PRIV(sk)->num != htons(ETH_P_X25) && 
	    SK_PRIV(sk)->num != htons(WP_X25_PROT) && 
	    SK_PRIV(sk)->num != htons(DSP_PROT) &&
	    SK_PRIV(sk)->num != htons(LAPB_PROT) &&
	    SK_PRIV(sk)->num != htons(SS7_PROT)){
		DEBUG_EVENT("%s: Illegal protocol in connect() sys call!\n",
				__FUNCTION__);
		return -EINVAL;
	}

	if (sk->sk_state == WANSOCK_CONNECTED){
		return EISCONN;	/* No reconnect on a seqpacket socket */
	}

	if (SK_PRIV(sk)->num == htons(SS7_PROT)){
		sock->state   = SS_CONNECTING;
		sk->sk_state     = WANSOCK_CONNECTING;
		return 0;
	}
		
	if (sk->sk_state != WANSOCK_DISCONNECTED){
		printk(KERN_INFO "af_wanpipe: Trying to connect on channel NON DISCONNECT\n");
		return -ECONNREFUSED;
	}

	sk->sk_state   = WANSOCK_DISCONNECTED;	
	sock->state = SS_UNCONNECTED;

	if (addr_len != sizeof(struct wan_sockaddr_ll)){
		return -EINVAL;
	}

	if (addr->sll_family != AF_WANPIPE){
		return -EINVAL;
	}

	if ((dev = (struct net_device *)SK_PRIV(sk)->dev) == NULL){
		printk(KERN_INFO "Sock user data is null\n");
		return -ENETUNREACH;
	}

	if (!WAN_NETDEV_TEST_IOCTL(dev))
		return -ENETUNREACH;

	sock->state   = SS_CONNECTING;
	sk->sk_state     = WANSOCK_CONNECTING;

	err=WAN_NETDEV_IOCTL(dev,NULL,SIOC_ANNEXG_PLACE_CALL);
	if (err){
		sk->sk_state   = WANSOCK_DISCONNECTED;	
		sock->state = SS_UNCONNECTED;
		return err; 
	}

	return 0;
}

static int sk_buf_check (struct sock *sk, int len)
{
	if (!wansk_is_zapped(sk))
		return -EINVAL;
		
	if (atomic_read(&sk->sk_rmem_alloc) + len >= (unsigned)sk->sk_rcvbuf)
		return -ENOMEM;

	return 0;
}

static int sk_poll_wake (struct sock *sk)
{
	if (!wansk_is_zapped(sk))
		return -EINVAL;

	SK_DATA_READY(sk, 0);
	return 0;
}


#if defined(LINUX_2_6)
struct proto_ops wanpipe_ops = {
	.family 	=PF_WANPIPE,
	.owner		=THIS_MODULE,
	.release 	=wanpipe_release,
	.bind		=wanpipe_bind,
	.connect 	=wanpipe_connect,
	.socketpair 	=sock_no_socketpair,
	.accept 	=wanpipe_accept,
	.getname 	=wanpipe_getname, 
	.poll 		=wanpipe_poll,
	.ioctl 		=wanpipe_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl 	=wanpipe_ioctl,
#endif
	.listen 	=wanpipe_listen, 
	.shutdown 	=sock_no_shutdown,
	.setsockopt 	=sock_no_setsockopt,
	.getsockopt	=sock_no_getsockopt,
	.sendmsg 	=wanpipe_sendmsg,
	.recvmsg 	=wanpipe_recvmsg
};

static struct net_proto_family wanpipe_family_ops = {
	.family =	PF_WANPIPE,
	.create =	wanpipe_create,
	.owner	=	THIS_MODULE,
};

static struct notifier_block wanpipe_netdev_notifier = {
	.notifier_call =wanpipe_notifier,
};

#else


struct proto_ops wanpipe_ops = {
	family: 	PF_WANPIPE,
	release: 	wanpipe_release,
	bind:		wanpipe_bind,
	connect :	wanpipe_connect,
	socketpair :	sock_no_socketpair,
	accept :	wanpipe_accept,
	getname: 	wanpipe_getname, 
	poll :		wanpipe_poll,
	ioctl :		wanpipe_ioctl,
	listen :	wanpipe_listen, 
	shutdown: 	sock_no_shutdown,
	setsockopt: 	sock_no_setsockopt,
	getsockopt:	sock_no_getsockopt,
	sendmsg:	wanpipe_sendmsg,
	recvmsg:	wanpipe_recvmsg
};

static struct net_proto_family wanpipe_family_ops = {
	PF_WANPIPE,
	wanpipe_create
};

struct notifier_block wanpipe_netdev_notifier={
	wanpipe_notifier,
	NULL,
	0
};

#endif



MODULE_AUTHOR ("Nenad Corbic <ncorbic@sangoma.com>");
MODULE_DESCRIPTION ("Sangoma WANPIPE: API Socket Support");
MODULE_LICENSE("GPL");

void __exit af_wanpipe_exit(void)
{
	DEBUG_EVENT("af_wanpipe: Unregistering Wanpipe API Socket Module\n");

	unregister_wanpipe_api_socket();

	unregister_netdevice_notifier(&wanpipe_netdev_notifier);
	sock_unregister(PF_WANPIPE);

	return;
}


static char fullname[]	= "WANPIPE(tm) Socket API Module";

int __init af_wanpipe_init(void)
{
	struct wanpipe_api_register_struct wan_api_reg; 

		DEBUG_EVENT("%s %s.%s %s %s\n",
			fullname, WANPIPE_VERSION, WANPIPE_SUB_VERSION,
			WANPIPE_COPYRIGHT_DATES, WANPIPE_COMPANY);
 

	wan_rwlock_init(&wanpipe_sklist_lock);
	wan_rwlock_init(&wanpipe_parent_sklist_lock);

	sock_register(&wanpipe_family_ops);
	register_netdevice_notifier(&wanpipe_netdev_notifier);
	
	wan_api_reg.wanpipe_api_sock_rcv = wanpipe_api_sock_rcv; 
	wan_api_reg.wanpipe_api_connected = wanpipe_api_connected;
	wan_api_reg.wanpipe_api_disconnected = wanpipe_api_disconnected;
	wan_api_reg.wanpipe_api_connecting = wanpipe_api_connecting;
	wan_api_reg.wanpipe_listen_rcv = wanpipe_listen_rcv;
	wan_api_reg.sk_buf_check = sk_buf_check;
	wan_api_reg.sk_poll_wake = sk_poll_wake;
	
	return register_wanpipe_api_socket(&wan_api_reg);
}

module_init(af_wanpipe_init);
module_exit(af_wanpipe_exit);

