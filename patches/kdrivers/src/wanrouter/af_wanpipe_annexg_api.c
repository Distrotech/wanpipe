/*****************************************************************************
* af_wanpipe_annexg_api.c	
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
#include <linux/wanpipe.h>
#include <linux/wanrouter.h>
#include <linux/if_wanpipe.h>
#include <linux/if_wanpipe_common.h>

#ifdef CONFIG_PRODUCT_WANPIPE_ANNEXG
#include "wanpipe_annexg_api.h"
#include "wanpipe_x25_kernel.h"
#include "wanpipe_dsp_kernel.h"
#endif


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
static struct sock * wanpipe_sklist = NULL;

static rwlock_t wanpipe_sklist_lock = RW_LOCK_UNLOCKED;

static atomic_t af_mem_alloc;
static atomic_t af_skb_alloc;

static atomic_t wanpipe_socks_nr;
extern struct proto_ops wanpipe_ops;

static struct sock *wanpipe_annexg_alloc_socket(void);
static void check_write_queue(struct sock *);


void wanpipe_annexg_kill_sock(struct sock *sock);

#if 0
void af_wan_sock_rfree(struct sk_buff *skb)
{
	struct sock *sk = skb->sk;
	atomic_sub(skb->truesize, &sk->rmem_alloc);
	wan_skb_destructor(skb);
}

void wan_skb_set_owner_r(struct sk_buff *skb, struct sock *sk)
{
	skb->sk = sk;
	skb->destructor = af_wan_sock_rfree;
	atomic_add(skb->truesize, &sk->rmem_alloc);
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
 *  wanpipe_annexg_ioctl
 *	
 * 	Execute a user commands, and set socket options.
 *
 * FIXME: More thought should go into this function.
 *
 *===========================================================*/

int wanpipe_annexg_ioctl(struct socket *sock, unsigned int cmd, unsigned long arg)
{
	struct sock *sk = sock->sk;
	int err;
	int pid;

	if (!sk->bound_dev_if)
		return -EINVAL;
	
	switch(cmd) 
	{
		case FIOSETOWN:
		case SIOCSPGRP:
			err = get_user(pid, (int *) arg);
			if (err)
				return err; 
			if (current->pid != pid && current->pgrp != -pid && 
			    !capable(CAP_NET_ADMIN))
				return -EPERM;
			sk->proc = pid;
			return(0);
			
		case FIOGETOWN:
		case SIOCGPGRP:
			return put_user(sk->proc, (int *)arg);
			
		case SIOCGSTAMP:
			if(sk->stamp.tv_sec==0)
				return -ENOENT;
			err = -EFAULT;
			if (!copy_to_user((void *)arg, &sk->stamp, sizeof(struct timeval)))
				err = 0;
			return err;

		case SIOC_WANPIPE_CHECK_TX:
			if (atomic_read(&sk->wmem_alloc))
				return 1;

			goto dev_private_ioctl;

		case SIOC_WANPIPE_SOCK_STATE:

			if (sk->state == WANSOCK_CONNECTED)
				return 0;
			
			return 1;

		case SIOC_WANPIPE_SET_NONBLOCK:

			if (sk->state != WANSOCK_DISCONNECTED)
				return -EINVAL;

			sock->file->f_flags |= O_NONBLOCK;
			return 0;
	
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
			    (cmd <= (SIOCDEVPRIVATE + 35))){
				netdevice_t *dev;
				struct ifreq ifr;
dev_private_ioctl:
				dev = (struct net_device *)sk->user_data;
				if (!dev)
					return -ENODEV;

				if (!dev->do_ioctl)
					return -ENODEV;
				
				ifr.ifr_data = (void*)arg;
			
				return dev->do_ioctl(dev,&ifr,cmd);
			}
			return -EOPNOTSUPP;
			
	}
	/*NOTREACHED*/
}



/*============================================================
 * wanpipe_annexg_make_new
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

static struct sock *wanpipe_annexg_make_new(struct sock *osk)
{
	struct sock *sk;

	if (osk->type != SOCK_RAW)
		return NULL;

	if ((sk = wanpipe_annexg_alloc_socket()) == NULL)
		return NULL;

	sk->family      = osk->family;
	sk->type        = osk->type;
	sk->socket      = osk->socket;
	sk->priority    = osk->priority;
	sk->protocol    = osk->protocol;
	sk->num		= osk->num;
	sk->rcvbuf      = osk->rcvbuf;
	sk->sndbuf      = osk->sndbuf;
	sk->debug       = osk->debug;
	sk->state       = WANSOCK_CONNECTING;
	sk->sleep       = osk->sleep;

	return sk;
}



/*============================================================
 * wanpipe_annexg_listen_rcv
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

static int wanpipe_annexg_listen_rcv (struct sk_buff *skb,  struct sock *sk)
{

	struct wan_sockaddr_ll *sll;
	struct sock *newsk;
	netdevice_t *dev; 
	x25api_hdr_t *x25_api_hdr;
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
		sk->state = WANSOCK_BIND_LISTEN;
		sk->data_ready(sk,0);
		return 0;
	}

	if (sk->state != WANSOCK_LISTEN){
		printk(KERN_INFO "wansock_annexg: Listen rcv in disconnected state!\n");
		sk->data_ready(sk,0);
		return -EINVAL;
	}

	/* Check if the receive buffer will hold the incoming
	 * call request */
	if (atomic_read(&sk->rmem_alloc) + skb->truesize >= 
			(unsigned)sk->rcvbuf){
		
		return -ENOMEM;
	}	

	
	sll = (struct wan_sockaddr_ll*)skb->cb;
	
	dev = skb->dev; 
	if (!dev){
		printk(KERN_INFO "wansock_annexg: LISTEN ERROR, No Free Device\n");
		return -ENODEV;
	}

	/* Allocate a new sock, which accept will bind
         * and pass up to the user 
	 */
	if ((newsk = wanpipe_annexg_make_new(sk)) == NULL){
		return -ENOMEM;
	}

	/* Bind the new socket into the lower layer. The lower
	 * layer will increment the sock reference count. */
	ifr.ifr_data = (void*)newsk;
	if (!dev->do_ioctl || dev->do_ioctl(dev,&ifr,SIOC_ANNEXG_BIND_SK) != 0){
		wanpipe_annexg_kill_sock(newsk);
		return -ENODEV;
	}

	newsk->zapped=1;
	newsk->bound_dev_if = dev->ifindex;
	dev_hold(dev);
	newsk->user_data = dev;

	write_lock_irqsave(&wanpipe_sklist_lock,flags);
	newsk->next = wanpipe_sklist;
	wanpipe_sklist = newsk;
	sock_hold(newsk);
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
	x25_api_hdr = (x25api_hdr_t*)skb->data;
	
	newsk->num = skb->protocol;
	newsk->state = WANSOCK_CONNECTING;

	/* Fill in the standard sock address info */

	sll->sll_family = AF_ANNEXG_WANPIPE;
	sll->sll_hatype = dev->type;
	sll->sll_protocol = newsk->num;
	sll->sll_pkttype = skb->pkt_type;
	sll->sll_ifindex = dev->ifindex;
	sll->sll_halen = 0;

	sk->ack_backlog++;

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
	sk->data_ready(sk,skb->len);
	skb_queue_tail(&sk->receive_queue, skb);
	
	return 0;
}


/*======================================================================
 * wanpipe_annexg_listen
 *
 *	X25API Specific function. Set a socket into LISTENING  MODE.
 *=====================================================================*/


int wanpipe_annexg_listen(struct socket *sock, int backlog)
{
	struct sock *sk = sock->sk;

 	/* This is x25 specific area if protocol doesn't
         * match, return error */
	if (sk->num != htons(ETH_P_X25) && sk->num != htons(DSP_PROT))
		return -EINVAL;

	if (sk->state == WANSOCK_BIND_LISTEN) {

		sk->max_ack_backlog = backlog;
		sk->state           = WANSOCK_LISTEN;
		return 0;
	}else{
		printk(KERN_INFO "wansock_annexg: Listening sock was not binded\n");
	}

	return -EINVAL;
}




/*======================================================================
 * wanpipe_annexg_accept
 *
 *	ACCEPT() System call.	X25API Specific function. 
 *	For each incoming call, create a new socket and 
 *      return it to the user.	
 *=====================================================================*/

int wanpipe_annexg_accept(struct socket *sock, struct socket *newsock, int flags)
{
	struct sock *sk;
	struct sock *newsk=NULL, *newsk_frmskb=NULL;
	struct sk_buff *skb;
	DECLARE_WAITQUEUE(wait, current);
	int err=-EINVAL;
	struct ifreq ifr;

	if (newsock->sk != NULL){
		printk(KERN_INFO "wansock_annexg: Accept is Releaseing an unused sock !\n");
		wanpipe_annexg_kill_sock(newsock->sk);
		newsock->sk=NULL;
	}
	
	if ((sk = sock->sk) == NULL)
		return -ENOTSOCK;

	if (sk->type != SOCK_RAW)
		return -EOPNOTSUPP;

	if (sk->state != WANSOCK_LISTEN)
		return -ENETDOWN;

	if (sk->num != htons(ETH_P_X25) &&sk->num != htons(DSP_PROT))
		return -EPROTOTYPE;

	add_wait_queue(sk->sleep,&wait);
	current->state = TASK_INTERRUPTIBLE;
	for (;;){
		skb = skb_dequeue(&sk->receive_queue);
		if (skb){
			err=0;
			break;
		}
		
		if (sk->state != WANSOCK_LISTEN){
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
	remove_wait_queue(sk->sleep,&wait);

	if (err != 0)
		return err;

	ifr.ifr_data=NULL;

	newsk=(struct sock*)*(unsigned long *)&skb->data[0];
	if (newsk){
		
		struct net_device *dev;
			
		/* Decrement the usage count of newsk, since
		 * we go the pointer for skb */
		__sock_put(newsk);
				
		if (newsk->zapped && (dev = (struct net_device *)newsk->user_data)){
			if (dev && dev->do_ioctl){
				struct sock* dev_sk;
				dev->do_ioctl(dev,&ifr,SIOC_ANNEXG_GET_SK);
				
				if ((dev_sk=(struct sock*)ifr.ifr_data)!=NULL){
					__sock_put(dev_sk);

					if (dev_sk == newsk){
						/* Got the right device with correct
						 * sk_id */
						goto accept_newsk_ok;
					}else{
						/* This should never happen */
						printk(KERN_INFO "wansock_annexg: Accept killing newsk, layer ptr mismatch!\n");
					}
				}else{
					printk(KERN_INFO "wansock_annexg: Accept killing newsk, get sk failed!\n");
				}

				ifr.ifr_data=(void*)newsk;
				if (dev->do_ioctl(dev,&ifr,SIOC_ANNEXG_UNBIND_SK)==0){
					dev->do_ioctl(dev,NULL,SIOC_ANNEXG_CLEAR_CALL);	
				}
			}else{
				printk(KERN_INFO "wansock_annexg: Accept killing newsk, lower layer down!\n");
			}
		}else{
			/* This can happen if the device goes down, before we get
			 * the chance to accept the call */
			printk(KERN_INFO "wansock_annexg: Accept killing newsk, newsk not zapped!\n");
		}

		newsk->dead=1;
		wanpipe_annexg_kill_sock(newsk);

		KFREE_SKB(skb);
		return -EINVAL;
	}else{
		KFREE_SKB(skb);
		return -ENODEV;
	}

accept_newsk_ok:
	newsk->pair = NULL;
	newsk->socket = newsock;
	newsk->sleep = &newsock->wait;

	/* Now attach up the new socket */
	sk->ack_backlog--;
	newsock->sk = newsk;

	if (newsk_frmskb){
		__sock_put(newsk_frmskb);
		newsk_frmskb=NULL;
	}
	KFREE_SKB(skb);
	return 0;
}




/*============================================================
 * wanpipe_annexg_api_sock_rcv
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

static int wanpipe_annexg_api_sock_rcv(struct sk_buff *skb, netdevice_t *dev,  struct sock *sk)
{
	struct wan_sockaddr_ll *sll = (struct wan_sockaddr_ll*)skb->cb;
	/*
	 *	When we registered the protocol we saved the socket in the data
	 *	field for just this event.
	 */

	if (!sk->zapped){
		return -EINVAL;
	}
	
	sll->sll_family = AF_ANNEXG_WANPIPE;
	sll->sll_hatype = dev->type;
	sll->sll_protocol = skb->protocol;
	sll->sll_pkttype = skb->pkt_type;
	sll->sll_ifindex = dev->ifindex;
	sll->sll_halen = 0;

	if (dev->hard_header_parse)
		sll->sll_halen = dev->hard_header_parse(skb, sll->sll_addr);

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
			if (sock_queue_rcv_skb(sk,skb)<0){
				AF_SKB_DEC(skb->truesize);
				return -ENOMEM;
			}
			break;
		case WAN_PACKET_CMD:
			/* Bug fix: update Mar6. 
                         * Do not set the sock lcn number here, since
         		 * cmd is not guaranteed to be executed on the
                         * board, thus Lcn could be wrong */
			sk->data_ready(sk,skb->len);
			KFREE_SKB(skb);
			break;
		case WAN_PACKET_ERR:
			if (sock_queue_err_skb(sk,skb)<0){
				AF_SKB_DEC(skb->truesize);
				return -ENOMEM;
			}
			break;
		default:
			printk(KERN_INFO "wansock_annexg: BH Illegal Packet Type Dropping\n");
			KFREE_SKB(skb); 
			break;
	}

	return 0;
}

/*============================================================
 * wanpipe_annexg_alloc_socket
 *
 *	Allocate memory for the a new sock, and sock
 *      private data.  
 *	
 *	Increment the module use count.
 *       	
 *===========================================================*/

static struct sock *wanpipe_annexg_alloc_socket(void)
{
	struct sock *sk;

	if ((sk = sk_alloc(PF_WANPIPE, GFP_ATOMIC, 1)) == NULL)
		return NULL;

	AF_MEM_INC(sizeof(struct sock));
	
	MOD_INC_USE_COUNT;
	sock_init_data(NULL, sk);
	return sk;
}


/*============================================================
 * wanpipe_annexg_sendmsg
 *
 *	This function implements a sendto() system call,
 *      for AF_WANPIPE socket family. 
 *      During socket bind() sk->bound_dev_if is initialized
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

int wanpipe_annexg_sendmsg(struct socket *sock, struct msghdr *msg, int len,
			  struct scm_cookie *scm)
{
	struct sock *sk = sock->sk;
	struct wan_sockaddr_ll *saddr=(struct wan_sockaddr_ll *)msg->msg_name;
	struct sk_buff *skb;
	netdevice_t *dev;
	unsigned short proto;
	unsigned char *addr;
	int ifindex, err=-EINVAL, reserve = 0;

	
	if (!sk->zapped)
		return -ENETDOWN;

	if (sk->state != WANSOCK_CONNECTED)
		return -ENOTCONN;	

	if (msg->msg_flags&~MSG_DONTWAIT) 
		return(-EINVAL);

	/* it was <=, now one can send
         * zero length packets */
	if (sk->protocol == htons(ETH_P_X25) && (len < sizeof(x25api_hdr_t)))
		return -EINVAL;
	if (sk->protocol == htons(DSP_PROT) && (len < sizeof(dspapi_hdr_t)))
		return -EINVAL;

	if (saddr == NULL) {
		ifindex	= sk->bound_dev_if;
		proto	= sk->num;
		addr	= NULL;

	}else{
		if (msg->msg_namelen < sizeof(struct wan_sockaddr_ll)){ 
			return -EINVAL;
		}

		ifindex = sk->bound_dev_if;
		proto	= saddr->sll_protocol;
		addr	= saddr->sll_addr;
	}

	dev = (struct net_device *)sk->user_data;
	if (dev == NULL){
		printk(KERN_INFO "wansock_annexg: Send failed, dev index: %i\n",ifindex);
		return -ENXIO;
	}

	if (netif_queue_stopped(dev))
		return -EBUSY;
	
	if (sock->type == SOCK_RAW)
		reserve = dev->hard_header_len;

	if (len > dev->mtu+reserve){
  		return -EMSGSIZE;
	}

      #ifndef LINUX_2_4
	dev_lock_list();
      #endif
      
	ALLOC_SKB(skb,len+dev->hard_header_len+15);
	if (skb==NULL){
		goto out_unlock;
	}
		
	skb_reserve(skb, (dev->hard_header_len+15)&~15);
	skb->nh.raw = skb->data;

	/* Returns -EFAULT on error */
	err = memcpy_fromiovec(skb_put(skb,len), msg->msg_iov, len);
	if (err){
		goto out_free;
	}

	if (dev->hard_header) {
		int res;
		err = -EINVAL;
		res = dev->hard_header(skb, dev, ntohs(proto), addr, NULL, len);
		if (res<0){
			goto out_free;
		}
	}

	skb->protocol = proto;
	skb->priority = sk->priority;
	skb->pkt_type = WAN_PACKET_DATA;

	err = -ENETDOWN;
	if (!(dev->flags & IFF_UP))
		goto out_free;

      #ifndef LINUX_2_4
	dev_unlock_list();
      #endif

	AF_SKB_DEC(skb->truesize);
	if (!dev->hard_start_xmit(skb,dev)){
		return(len);
	}else{
		err = -EBUSY;
	}
	AF_SKB_INC(skb->truesize);
	
out_free:
	KFREE_SKB(skb);
out_unlock:
#ifndef LINUX_2_4
	dev_unlock_list();
#endif
	return err;
}

/*============================================================
 * wanpipe_annexg_kill_sock 
 *
 *	Used by wanpipe_release, to delay release of
 *      the socket.
 *===========================================================*/
void wanpipe_annexg_kill_sock (struct sock *sk)
{

	struct sock **skp;
	unsigned long flags;
	
	if (!sk)
		return;

	sk->state = WANSOCK_DISCONNECTED;
	sk->zapped=0;
	sk->bound_dev_if=0;
	if (sk->user_data){
		dev_put((struct net_device*)sk->user_data);
	}
	sk->user_data=NULL;
	
	/* This functin can be called from interrupt. We must use
	 * appropriate locks */
	
	write_lock_irqsave(&wanpipe_sklist_lock,flags);
	for (skp = &wanpipe_sklist; *skp; skp = &(*skp)->next) {
		if (*skp == sk) {
			*skp = sk->next;
			__sock_put(sk);
			break;
		}
	}
	write_unlock_irqrestore(&wanpipe_sklist_lock,flags);

	sk->socket = NULL;

	/* Purge queues */
	af_skb_queue_purge(&sk->receive_queue);
	af_skb_queue_purge(&sk->error_queue);
	
      #ifdef LINUX_2_4
	if (atomic_read(&sk->refcnt) != 1){
		DEBUG_TEST("wansock_annexg: Error, wrong reference count: %i ! :Kill.\n",
					atomic_read(&sk->refcnt));
		atomic_set(&sk->refcnt,1);
	}
	sock_put(sk);
      #else
	sk_free(sk);
      #endif
	AF_MEM_DEC(sizeof(struct sock));
	atomic_dec(&wanpipe_socks_nr);
	MOD_DEC_USE_COUNT;
	return;
}


static void release_queued_pending_sockets(struct sock *sk)
{
	netdevice_t *dev;
	struct sk_buff *skb;
	struct sock *deadsk;
	
	while ((skb=skb_dequeue(&sk->receive_queue))!=NULL){
		deadsk=(struct sock *)*(unsigned long*)&skb->data[0];
		if (!deadsk){
			KFREE_SKB(skb);
			continue;
		}

		deadsk->dead=1;
		printk (KERN_INFO "wansock_annexg: Release: found dead sock!\n");
		dev = (struct net_device *)deadsk->user_data;
		if (dev && dev->do_ioctl){
			struct ifreq ifr;
			ifr.ifr_data=(void*)sk;
			if (dev->do_ioctl(dev,&ifr,SIOC_ANNEXG_UNBIND_SK)==0){
				dev->do_ioctl(dev,NULL,SIOC_ANNEXG_CLEAR_CALL);
			}
		}	

		/* Decrement the sock refcnt, since we took
		 * the pointer out of the skb buffer */
		__sock_put(deadsk);
		wanpipe_annexg_kill_sock(deadsk);
		KFREE_SKB(skb);
	}
}


/*============================================================
 * wanpipe_annexg_release
 *
 *	Close a PACKET socket. This is fairly simple. We 
 *      immediately go to 'closed' state and remove our 
 *      protocol entry in the device list.
 *===========================================================*/

#ifdef LINUX_2_4
int wanpipe_annexg_release(struct socket *sock)
#else
int wanpipe_annexg_release(struct socket *sock, struct socket *peersock)
#endif
{
	
#ifndef LINUX_2_4
	struct sk_buff	*skb;
#endif
	struct sock *sk = sock->sk;
	struct sock **skp;
	unsigned long flags;
	
	if (!sk)
		return 0;

	check_write_queue(sk);

	/* Kill the tx timer, if we don't kill it now, the timer
         * will run after we kill the sock.  Timer code will 
         * try to access the sock which has been killed and cause
         * kernel panic */

	/*
	 *	Unhook packet receive handler.
	 */

	if (sk->state == WANSOCK_LISTEN || sk->state == WANSOCK_BIND_LISTEN){
		unbind_api_listen_from_protocol	(sk->num,sk);
		release_queued_pending_sockets(sk);
		
	}else if ((sk->num == htons(ETH_P_X25) || sk->num == htons(DSP_PROT))&& sk->zapped){
		netdevice_t *dev = (struct net_device *)sk->user_data;
		if (dev){
			if(dev->do_ioctl){
				struct ifreq ifr;
				ifr.ifr_data=(void*)sk;
				if (dev->do_ioctl(dev,&ifr,SIOC_ANNEXG_UNBIND_SK)==0){
					dev->do_ioctl(dev,NULL,SIOC_ANNEXG_CLEAR_CALL);
				}
			}
		}
	}

	sk->state = WANSOCK_DISCONNECTED;
	sk->zapped=0;
	sk->bound_dev_if=0;
	if (sk->user_data){
		dev_put((struct net_device*)sk->user_data);
	}
	sk->user_data=NULL;
	
	//FIXME with spinlock_irqsave
	write_lock_irqsave(&wanpipe_sklist_lock,flags);
	for (skp = &wanpipe_sklist; *skp; skp = &(*skp)->next) {
		if (*skp == sk) {
			*skp = sk->next;
			__sock_put(sk);
			break;
		}
	}
	write_unlock_irqrestore(&wanpipe_sklist_lock,flags);



	/*
	 *	Now the socket is dead. No more input will appear.
	 */

	sk->state_change(sk);	/* It is useless. Just for sanity. */
	
	sock->sk = NULL;
	sk->socket = NULL;
	sk->dead = 1;

	/* Purge queues */
	af_skb_queue_purge(&sk->receive_queue);
	af_skb_queue_purge(&sk->error_queue);
	
      #ifdef LINUX_2_4
	if (atomic_read(&sk->refcnt) != 1){
		DEBUG_TEST("wansock_annexg: Error, wrong reference count: %i !:release.\n",
					atomic_read(&sk->refcnt));
		atomic_set(&sk->refcnt,1);
	}
	sock_put(sk);
      #else	
	sk_free(sk);
      #endif
	AF_MEM_DEC(sizeof(struct sock));
	atomic_dec(&wanpipe_socks_nr);
	
	MOD_DEC_USE_COUNT;
	
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

	if (sk->state != WANSOCK_CONNECTED)
		return;

	if (!atomic_read(&sk->wmem_alloc))
		return;

	printk(KERN_INFO "wansock_annexg: MAJOR ERROR, Data lost on sock release !!!\n");

}


/*============================================================
 *  wanpipe_annexg_bind
 *
 *      BIND() System call, which is bound to the AF_WANPIPE
 *      operations structure.  It checks for correct wanpipe
 *      card name, and cross references interface names with
 *      the card names.  Thus, interface name must belong to
 *      the actual card.
 *===========================================================*/


int wanpipe_annexg_bind(struct socket *sock, struct sockaddr *uaddr, int addr_len)
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
		printk(KERN_INFO "wansock_annexg: Address length error\n");
		return -EINVAL;
	}
	if (sll->sll_family != AF_ANNEXG_WANPIPE){
		printk(KERN_INFO "wansock_annexg: Illegal family name specified.\n");
		return -EINVAL;
	}

	sk->num = sll->sll_protocol ? sll->sll_protocol : sk->num;
	
	if (!strcmp(sll->sll_device,"svc_connect") ||
	    !strcmp(sll->sll_device,"dsp_connect")){

		strncpy(name,sll->sll_card,14);
		name[14]=0;

		/* Obtain the master device, in case of Annexg this
		 * device would be a lapb network interface, note 
		 * the usage count on dev will be incremented, once
		 * we are finshed with the device we should run
		 * dev_put() to release it */
#ifdef LINUX_2_4
		dev = dev_get_by_name(name);
#else
		dev = dev_get(name);
#endif
		if (dev == NULL){
			printk(KERN_INFO "wansock_annexg: Failed to get Dev from name: %s,\n",
					name);
			return -ENODEV;
		}

		if (!(dev->flags&IFF_UP)){
			printk(KERN_INFO "wansock_annexg: Bind device %s is down!\n",
					name);
			dev_put(dev);
			return -ENODEV;
		}
		
		sk->bound_dev_if = bind_api_to_protocol(dev,sk->num,(void*)sk);	
		
		/* We are finished with the lapb master device, we
		 * need it to find a free svc or dsp device but now
		 * we can release it */
		dev_put(dev);
		
		if (sk->bound_dev_if < 0){
			sk->bound_dev_if=0;
			err=-EINVAL;
		}else{
			sk->state = WANSOCK_DISCONNECTED;
			sk->user_data=dev_get_by_index(sk->bound_dev_if);
			if (sk->user_data){
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

		/* Obtain the master device, in case of Annexg this
		 * device would be a lapb network interface, note 
		 * the usage count on dev will be incremented, once
		 * we are finshed with the device we should run
		 * dev_put() to release it */

#ifdef LINUX_2_4
		dev = dev_get_by_name(name);
#else
		dev = dev_get(name);
#endif
		if (dev == NULL){
			printk(KERN_INFO "wansock_annexg: Failed to get Dev from name: %s,\n",
					name);
			return -ENODEV;
		}

		if (!(dev->flags&IFF_UP)){
			printk(KERN_INFO "wansock_annexg: Bind device %s is down!\n",
					name);
			dev_put(dev);
			return -ENODEV;
		}

		err = bind_api_listen_to_protocol(dev,sk->num,sk);	
		if (!err){
			sk->state = WANSOCK_BIND_LISTEN;
		}


//		printk(KERN_INFO "wansock_annexg: Bind device %s !\n",
//					name);
		
		dev_put(dev);
	}else{
		struct ifreq ifr;
		strncpy(name,sll->sll_device,14);
		name[14]=0;
	
#ifdef LINUX_2_4
		dev = dev_get_by_name(name);
#else
		dev = dev_get(name);
#endif
		if (dev == NULL){
			printk(KERN_INFO "wansock_annexg: Failed to get Dev from name: %s,\n",
					name);
			return -ENODEV;
		}

		if (!(dev->flags&IFF_UP)){
			printk(KERN_INFO "wansock_annexg: Bind device %s is down!\n",
					name);
			dev_put(dev);
			return -ENODEV;
		}

		ifr.ifr_data = (void*)sk;
		err=-ENODEV;

		if (dev->do_ioctl)
			err=dev->do_ioctl(dev,&ifr,SIOC_ANNEXG_BIND_SK);
		
		if (!err){
			sk->bound_dev_if = dev->ifindex; 
			sk->user_data = dev;
			sk->state = WANSOCK_DISCONNECTED;
		}
	}

	if (!err){
		sk->zapped=1;
	}
	
//	printk(KERN_INFO "11-11 Bind Socket Prot %i, X25=%i Zapped %i, Bind Dev %i Sock %u!\n",
//			sk->num,htons(ETH_P_X25),sk->zapped,sk->bound_dev_if,(u32)sk);
	
	return err;
}

/*============================================================
 *  wanpipe_annexg_create
 *	
 * 	SOCKET() System call.  It allocates a sock structure
 *      and adds the socket to the wanpipe_sk_list. 
 *      Crates AF_WANPIPE socket.
 *===========================================================*/

int wanpipe_annexg_create(struct socket *sock, int protocol)
{
	struct sock *sk;
	unsigned long flags;
	
	if (sock->type != SOCK_RAW)
		return -ESOCKTNOSUPPORT;

	sock->state = SS_UNCONNECTED;
	
	if ((sk = wanpipe_annexg_alloc_socket()) == NULL){
		return -ENOMEM;
	}

	sk->reuse = 1;
	sock->ops = &wanpipe_ops;
	sock_init_data(sock,sk);

	sk->zapped=0;
	sk->family = AF_ANNEXG_WANPIPE;
	sk->num = protocol;
	sk->state = WANSOCK_UNCONFIGURED;
	sk->ack_backlog = 0;
	sk->bound_dev_if=0;
	sk->user_data=NULL;

	atomic_inc(&wanpipe_socks_nr);
	
	/* We must disable interrupts because the ISR
	 * can also change the list */
	
	write_lock_irqsave(&wanpipe_sklist_lock,flags);
	sk->next = wanpipe_sklist;
	wanpipe_sklist = sk;
	sock_hold(sk);
	write_unlock_irqrestore(&wanpipe_sklist_lock,flags);
	
	return(0);
}


/*============================================================
 *  wanpipe_annexg_recvmsg
 *	
 *	Pull a packet from our receive queue and hand it 
 *      to the user. If necessary we block.
 *===========================================================*/

int wanpipe_annexg_recvmsg(struct socket *sock, struct msghdr *msg, int len,
			  int flags, struct scm_cookie *scm)
{
	struct sock *sk = sock->sk;
	struct sk_buff *skb;
	int copied, err=-ENOBUFS;
	struct net_device *dev=NULL;

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
		skb=skb_dequeue(&sk->error_queue);
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


	dev = (struct net_device *)sk->user_data;
	if (dev){
		if (dev->do_ioctl){
			dev->do_ioctl(dev,NULL,SIOC_ANNEXG_KICK);
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
	err = memcpy_toiovec(msg->msg_iov, skb->data, copied);
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
 *  wanpipe_annexg_getname
 *	
 * 	I don't know what to do with this yet. 
 *      User can use this function to get sock address
 *      information. Not very useful for Sangoma's purposes.
 *===========================================================*/


int wanpipe_annexg_getname(struct socket *sock, struct sockaddr *uaddr,
			  int *uaddr_len, int peer)
{
	netdevice_t *dev;
	struct sock *sk = sock->sk;
	struct wan_sockaddr_ll *sll = (struct wan_sockaddr_ll*)uaddr;

	sll->sll_family = AF_ANNEXG_WANPIPE;
	sll->sll_ifindex = sk->bound_dev_if;
	sll->sll_protocol = sk->num;
	dev = (struct net_device *)sk->user_data;
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

int wanpipe_annexg_notifier(struct notifier_block *this, unsigned long msg, void *data)
{
	struct sock *sk;
	netdevice_t *dev = (netdevice_t*)data;

	if (dev==NULL)
		return NOTIFY_DONE;
			
	for (sk = wanpipe_sklist; sk; sk = sk->next) {

		switch (msg) {
		case NETDEV_DOWN:
		case NETDEV_UNREGISTER:
			if (dev->ifindex == sk->bound_dev_if) {
				netdevice_t *dev = (struct net_device *)sk->user_data;
				printk(KERN_INFO "wansock_annexg: Device down %s\n",dev->name);

				if (sk->state != WANSOCK_LISTEN && 
				    sk->state != WANSOCK_BIND_LISTEN){
					sk->state = WANSOCK_DISCONNECTED;
				}
				sk->bound_dev_if = 0;
				sk->zapped=0;
				if (sk->user_data){
					dev_put((struct net_device *)sk->user_data);
				}
				sk->user_data=NULL;
			}
			break;
		}
	}
	
	return NOTIFY_DONE;
}

/*======================================================================
 * wanpipe_annexg_poll
 *
 *	Datagram poll: Again totally generic. This also handles
 *	sequenced packet sockets providing the socket receive queue
 *	is only ever holding data ready to receive.
 *
 *	Note: when you _don't_ use this routine for this protocol,
 *	and you use a different write policy from sock_writeable()
 *	then please supply your own write_space callback.
 *=====================================================================*/

int wanpipe_annexg_poll(struct file * file, struct socket *sock, poll_table *wait)
{
	struct sock *sk = sock->sk;
	unsigned int mask;
	struct net_device *dev=NULL;

	if (!sk)
		return 0;

	poll_wait(file, sk->sleep, wait);
	mask = 0;

	/* exceptional events? */
	if (sk->err || !skb_queue_empty(&sk->error_queue)){
		mask |= POLLPRI;
		return mask;
	}
	if (sk->shutdown & RCV_SHUTDOWN)
		mask |= POLLHUP;

	/* readable? */
	if (!skb_queue_empty(&sk->receive_queue)){
		mask |= POLLIN | POLLRDNORM;
	}

	/* connection hasn't started yet */
	if (sk->state == WANSOCK_CONNECTING || sk->state == WANSOCK_LISTEN){
		return mask;
	}

	if (sk->state != WANSOCK_CONNECTED){
		mask = POLLPRI;
		return mask;
	}
	
	if ((dev = (struct net_device *)sk->user_data) == NULL){
		printk(KERN_INFO "wansock_annexg: No Device found in POLL!\n");
		return mask;
	}

	if (!(dev->flags & IFF_UP))
		return mask;
	
	if (!netif_queue_stopped(dev)){
		mask |= POLLOUT | POLLWRNORM | POLLWRBAND;
	}else{
	      #ifdef LINUX_2_4
		set_bit(SOCK_ASYNC_NOSPACE, &sk->socket->flags);
	      #else
 		sk->socket->flags |= SO_NOSPACE;
	      #endif
	}
	
	return mask;
}

static int wanpipe_annexg_api_connected(struct net_device *dev, struct sock *sk)
{
	if (sk == NULL || dev == NULL)
		return -EINVAL;

	if (!sk->zapped)
		return -EINVAL;

	sk->state = WANSOCK_CONNECTED;
	sk->data_ready(sk,0);
	return 0;
}

static int wanpipe_annexg_api_disconnected(struct net_device *dev, struct sock *sk)
{
	if (sk == NULL || dev == NULL)
		return -EINVAL;
	
	if (!sk->zapped)
		return -EINVAL;
		
	sk->state = WANSOCK_DISCONNECTED;

	if (sk->user_data){
		dev_put((struct net_device *)sk->user_data);	
	}
	sk->bound_dev_if=0;
	sk->user_data=NULL;
	sk->zapped=0;
	sk->data_ready(sk,0);
	return 0;
}


/*======================================================================
 *  wanpipe_annexg_connect
 *
 *  	CONNECT() System Call. X25API specific function
 * 	Check the state of the sock, and execute PLACE_CALL command.
 *      Connect can ether block or return without waiting for connection, 
 *      if specified by user.
 *=====================================================================*/

int wanpipe_annexg_connect(struct socket *sock, struct sockaddr *uaddr, int addr_len, int flags)
{
	struct sock *sk = sock->sk;
	struct wan_sockaddr_ll *addr = (struct wan_sockaddr_ll*)uaddr;
	netdevice_t *dev;

	if (!sk->zapped){		/* Must bind first - autobinding does not work */
		return -EINVAL;
	}
	
	if (sk->num != htons(ETH_P_X25) && sk->num != htons(DSP_PROT)){
		return -EINVAL;
	}

	if (sk->state == WANSOCK_CONNECTED){
		return EISCONN;	/* No reconnect on a seqpacket socket */
	}
		
	if (sk->state != WANSOCK_DISCONNECTED){
		printk(KERN_INFO "wansock_annexg: Trying to connect on channel NON DISCONNECT\n");
		return -ECONNREFUSED;
	}

	sk->state   = WANSOCK_DISCONNECTED;	
	sock->state = SS_UNCONNECTED;

	if (addr_len != sizeof(struct wan_sockaddr_ll)){
		return -EINVAL;
	}

	if (addr->sll_family != AF_ANNEXG_WANPIPE){
		return -EINVAL;
	}

	if ((dev = (struct net_device *)sk->user_data) == NULL){
		printk(KERN_INFO "Sock user data is null\n");
		return -ENETUNREACH;
	}

	if (!dev->do_ioctl)
		return -ENETUNREACH;

	sock->state   = SS_CONNECTING;
	sk->state     = WANSOCK_CONNECTING;

	if (dev->do_ioctl(dev,NULL,SIOC_ANNEXG_PLACE_CALL)){
		sk->state   = WANSOCK_DISCONNECTED;	
		sock->state = SS_UNCONNECTED;
		return -ECONNREFUSED; 
	}

	return 0;
}

static int sk_buf_check (struct sock *sk, int len)
{
	if (!sk->zapped)
		return -EINVAL;
		
	if (atomic_read(&sk->rmem_alloc) + len >= (unsigned)sk->rcvbuf)
		return -ENOMEM;

	return 0;
}

static int sk_poll_wake (struct sock *sk)
{
	if (!sk->zapped)
		return -EINVAL;

	sk->data_ready(sk,0);
	return 0;
}


void unregister_annexg_api(void)
{
	printk(KERN_INFO "wansock_annexg: Unregistering Wanpipe Annexg API Socket \n");
	unregister_wanpipe_api_socket();
	return;
}


int register_annexg_api(void)
{
	struct wanpipe_api_register_struct wan_api_reg; 
		
	printk(KERN_INFO "wansock_annexg: Registering Wanpipe Annexg API Socket \n");

	wan_api_reg.wanpipe_api_sock_rcv = wanpipe_annexg_api_sock_rcv; 
	wan_api_reg.wanpipe_api_connected = wanpipe_annexg_api_connected;
	wan_api_reg.wanpipe_api_disconnected = wanpipe_annexg_api_disconnected;
	wan_api_reg.wanpipe_listen_rcv = wanpipe_annexg_listen_rcv;
	wan_api_reg.sk_buf_check = sk_buf_check;
	wan_api_reg.sk_poll_wake = sk_poll_wake;
	
	return register_wanpipe_api_socket(&wan_api_reg);
}

