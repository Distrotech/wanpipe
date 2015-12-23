/*
 *	NET3:	A (fairly minimal) implementation of synchronous PPP for Linux
 *		as well as a CISCO HDLC implementation. See the copyright 
 *		message below for the original source.
 *
 *	This program is free software; you can redistribute it and/or
 *	modify it under the terms of the GNU General Public License
 *	as published by the Free Software Foundation; either version
 *	2 of the license, or (at your option) any later version.
 *
 *	Note however. This code is also used in a different form by FreeBSD.
 *	Therefore when making any non OS specific change please consider
 *	contributing it back to the original author under the terms
 *	below in addition.
 *		-- Alan
 *
 *	Port for Linux-2.1 by Jan "Yenya" Kasprzak <kas@fi.muni.cz>
 */

/*
 * Synchronous PPP/Cisco link level subroutines.
 * Keepalive protocol implemented in both Cisco and PPP modes.
 *
 * Copyright (C) 1994 Cronyx Ltd.
 * Author: Serge Vakulenko, <vak@zebub.msk.su>
 *
 * This software is distributed with NO WARRANTIES, not even the implied
 * warranties for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * Authors grant any other persons or organisations permission to use
 * or modify this software as long as this message is kept with the software,
 * all derivative works or modified versions.
 *
 * Version 1.9, Wed Oct  4 18:58:15 MSK 1995
 * Version 2.0, Fri Aug 30 09:59:07 EDT 2002
 * Version 2.1, Wed Mar 26 10:03:00 EDT 2003
 * 
 * $Id: wanpipe_syncppp.c,v 1.33 2008-05-05 17:13:26 sangoma Exp $
 * $Id: wanpipe_syncppp.c,v 1.33 2008-05-05 17:13:26 sangoma Exp $
 */

/*
 *
 * Changes: v2.1 by David Rokhvarg <davidr@sangoma.com>
 *  
 * Added PAP and CHAP support.
 * 	o Only as a peer, not an authenticator. 
 *
 * Changes: v2.0 by Nenad Corbic <ncorbic@sangoma.com>
 *
 * Added /proc/net/wanrouter/syncppp support.
 * 	o It displays all PPP protocol statistics
 * 	o Allows the user to enable debugging
 * 	  dynamically via /proc file system by 
 * 	  writting commands to the /proc/net/syncppp file.
 *	o Replaced all cli() instances with spin_lock_irqsave().
 *	o The state of the ppp link can be determined via
 *	  /proc file system.
 *
 * FIXME: A next step would be to create /proc/net/syncppp
 *        directory and have the mib,config,status files
 *        instead of one big file.
 */

#undef DEBUG
#define _K22X_MODULE_FIX_
#include <linux/wanpipe_includes.h>
#include <linux/wanpipe_defines.h>
#include <linux/wanpipe_debug.h>
#include <linux/wanpipe_common.h>
#include <linux/wanpipe_kernel.h>
#include <linux/wanpipe_wanrouter.h>
#include <linux/wanpipe_syncppp.h>
#include <linux/wanpipe_cfg.h>
#include <linux/wanproc.h>

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,4,10) 
#define	snprintf(a,b,c,d...) sprintf(a,c,##d)
#endif

#define MAXALIVECNT     6               /* max. alive packets */

#define LCP_CONF_REQ    1               /* PPP LCP configure request */
#define LCP_CONF_ACK    2               /* PPP LCP configure acknowledge */
#define LCP_CONF_NAK    3               /* PPP LCP configure negative ack */
#define LCP_CONF_REJ    4               /* PPP LCP configure reject */
#define LCP_TERM_REQ    5               /* PPP LCP terminate request */
#define LCP_TERM_ACK    6               /* PPP LCP terminate acknowledge */
#define LCP_CODE_REJ    7               /* PPP LCP code reject */
#define LCP_PROTO_REJ   8               /* PPP LCP protocol reject */
#define LCP_ECHO_REQ    9               /* PPP LCP echo request */
#define LCP_ECHO_REPLY  10              /* PPP LCP echo reply */
#define LCP_DISC_REQ    11              /* PPP LCP discard request */

#define LCP_OPT_MRU             1       /* maximum receive unit */
#define LCP_OPT_ASYNC_MAP       2       /* async control character map */
#define LCP_OPT_AUTH_PROTO      3       /* authentication protocol */
#define LCP_OPT_QUAL_PROTO      4       /* quality protocol */
#define LCP_OPT_MAGIC           5       /* magic number */
#define LCP_OPT_RESERVED        6       /* reserved */
#define LCP_OPT_PROTO_COMP      7       /* protocol field compression */
#define LCP_OPT_ADDR_COMP       8       /* address/control field compression */

#define IPCP_CONF_REQ   LCP_CONF_REQ    /* PPP IPCP configure request */
#define IPCP_CONF_ACK   LCP_CONF_ACK    /* PPP IPCP configure acknowledge */
#define IPCP_CONF_NAK   LCP_CONF_NAK    /* PPP IPCP configure negative ack */
#define IPCP_CONF_REJ   LCP_CONF_REJ    /* PPP IPCP configure reject */
#define IPCP_TERM_REQ   LCP_TERM_REQ    /* PPP IPCP terminate request */
#define IPCP_TERM_ACK   LCP_TERM_ACK    /* PPP IPCP terminate acknowledge */
#define IPCP_CODE_REJ   LCP_CODE_REJ    /* PPP IPCP code reject */

#define CISCO_MULTICAST         0x8f    /* Cisco multicast address */
#define CISCO_UNICAST           0x0f    /* Cisco unicast address */
#define CISCO_KEEPALIVE         0x8035  /* Cisco keepalive protocol */
#define CISCO_ADDR_REQ          0       /* Cisco address request */
#define CISCO_ADDR_REPLY        1       /* Cisco address reply */
#define CISCO_KEEPALIVE_REQ     2       /* Cisco keepalive request */

/* states are named and numbered according to RFC 1661 */
#define STATE_INITIAL	0
#define STATE_STARTING	1
#define STATE_CLOSED	2
#define STATE_STOPPED	3
#define STATE_CLOSING	4
#define STATE_STOPPING	5
#define STATE_REQ_SENT	6
#define STATE_ACK_RCVD	7
#define STATE_ACK_SENT	8
#define STATE_OPENED	9

#define PAP_REQ			1	/* PAP name/password request */
#define PAP_ACK			2	/* PAP acknowledge */
#define PAP_NAK			3	/* PAP fail */

#define CHAP_CHALLENGE		1	/* CHAP challenge request */
#define CHAP_RESPONSE		2	/* CHAP challenge response */
#define CHAP_SUCCESS		3	/* CHAP response ok */
#define CHAP_FAILURE		4	/* CHAP response failed */

#define CHAP_MD5		5	/* hash algorithm - MD5 */

WAN_DECLARE_NETDEV_OPS(wan_netdev_ops)

struct ppp_header {
	u8 address;
	u8 control;
	u16 protocol;
};
#define PPP_HEADER_LEN          sizeof (struct ppp_header)

struct lcp_header {
	u8 type;
	u8 ident;
	u16 len;
};

struct ipcp_header {
	u8 type;
	u8 len;
	unsigned char data[1];
};


#define LCP_HEADER_LEN          sizeof (struct lcp_header)

struct cisco_packet {
	u32 type;
	u32 par1;
	u32 par2;
	u16 rel;
	u16 time0;
	u16 time1;
};
#define CISCO_PACKET_LEN 18
#define CISCO_BIG_PACKET_LEN 20

#define MAX_AUTHENTICATE_PACKET_LEN	256

/////////////////////////////

/*
 * We follow the spelling and capitalization of RFC 1661 here, to make
 * it easier comparing with the standard.  Please refer to this RFC in
 * case you can't make sense out of these abbreviation; it will also
 * explain the semantics related to the various events and actions.
 */
struct cp {
	u_short	proto;		/* PPP control protocol number */
	u_char protoidx;	/* index into state table in struct sppp */
	u_char flags;
#define CP_LCP		0x01	/* this is the LCP */
#define CP_AUTH		0x02	/* this is an authentication protocol */
#define CP_NCP		0x04	/* this is a NCP */
#define CP_QUAL		0x08	/* this is a quality reporting protocol */
	const char *name;	/* name of this control protocol */
	/* event handlers */
	void	(*Up)(struct sppp *sp);
	void	(*Down)(struct sppp *sp);
	void	(*Open)(struct sppp *sp);
	void	(*Close)(struct sppp *sp);
	void	(*TO)(void *sp);
	int	(*RCR)(struct sppp *sp, struct lcp_header *h, int len);
	void	(*RCN_rej)(struct sppp *sp, struct lcp_header *h, int len);
	void	(*RCN_nak)(struct sppp *sp, struct lcp_header *h, int len);
	/* actions */
	void	(*tlu)(struct sppp *sp);
	void	(*tld)(struct sppp *sp);
	void	(*tls)(struct sppp *sp);
	void	(*tlf)(struct sppp *sp);
	void	(*scr)(struct sppp *sp);
};

//static struct sppp *spppq;
#if defined(__FreeBSD__) && __FreeBSD__ >= 3
static struct callout_handle keepalive_ch;
#endif

#if defined(__FreeBSD__) && __FreeBSD__ >= 3
#define	SPP_FMT		"%s%d: "
#define	SPP_ARGS(ifp)	(ifp)->if_name, (ifp)->if_unit
#else
#define	SPP_FMT		"%s: "
#define	SPP_ARGS(ifp)	(ifp)->name
#endif

/* almost every function needs these */
#define STDDCL					\
	struct net_device *dev = sp->pp_if;	\
	int debug = sp->pp_flags & PP_DEBUG

# define UNTIMEOUT( arg ) 	auth_clear_timeout(arg)
# define TIMEOUT(fun, arg, time_in_seconds) auth_timeout(fun, arg, time_in_seconds)
# define IOCTL_CMD_T	int

////////////////////////////

static struct sppp *spppq;
static struct timer_list sppp_keepalive_timer;

wan_spinlock_t  spppq_lock;
wan_spinlock_t	authenticate_lock;

static void sppp_keepalive (unsigned long dummy);
static void sppp_cp_send (struct sppp *sp, u16 proto, u8 type,
	u8 ident, u16 len, void *data);
static void sppp_cisco_send (struct sppp *sp, int type, long par1, long par2);
static void sppp_lcp_input (struct sppp *sp, struct sk_buff *m);
static void sppp_cisco_input (struct sppp *sp, struct sk_buff *m);
static void sppp_ipcp_input (struct sppp *sp, struct sk_buff *m);
static void sppp_lcp_open (struct sppp *sp);
static void sppp_ipcp_open (struct sppp *sp);
static int sppp_lcp_conf_parse_options (struct sppp *sp, struct lcp_header *h,
	int len, u32 *magic);
static void sppp_cp_timeout (unsigned long arg);
static char *sppp_lcp_type_name (u8 type);
static char *sppp_ipcp_type_name (u8 type);
static void sppp_print_bytes (u8 *p, u16 len);

static int sppp_strnlen(u_char *p, int max);
static const char * sppp_auth_type_name(u_short proto, u_char type);
static void sppp_print_string(const char *p, u_short len);
static void sppp_cp_change_state(const struct cp *cp, struct sppp *sp,
				 int newstate);
static void
sppp_auth_send(struct sppp *sp,
               unsigned int protocol, unsigned int type, unsigned int id,
	       ...);

static void auth_timeout(void * function, struct sppp *p, unsigned int seconds);
static void auth_clear_timeout(struct sppp *p);
static void sppp_phase_network(struct sppp *sp);
static const char * sppp_phase_name(enum ppp_phase phase);

static const char * sppp_proto_name(u_short proto);
static const char * sppp_state_name(int state);
static void sppp_null(struct sppp *unused);

static void sppp_pap_scr(struct sppp *sp);

static void sppp_chap_input(struct sppp *sp, struct sk_buff *skb);
static void sppp_chap_init(struct sppp *sp);
static void sppp_chap_open(struct sppp *sp);
static void sppp_chap_close(struct sppp *sp);
static void sppp_chap_TO(void *sp);
static void sppp_chap_tlu(struct sppp *sp);
static void sppp_chap_tld(struct sppp *sp);
static void sppp_chap_scr(struct sppp *sp);

#ifdef CONFIG_PROC_FS
#include <linux/proc_fs.h>
#include <asm/uaccess.h>       /* copy_to_user */
#include <linux/ctype.h>

/* Proc filesystem interface */

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,5,0)
static int router_proc_perms(struct inode *, int);
static ssize_t router_proc_read(struct file *file, char *buf, size_t count, 					loff_t *ppos);
static ssize_t router_proc_write(struct file *file, const char *buf, size_t count, 
   loff_t *ppos);

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,3,0)
static int status_get_info(char* buf, char** start, off_t offs, int len);
#else
static int status_get_info(char* buf, char** start, off_t offs, int len, int dummy);
#endif

static char *decode_lcp_state(int state);
static char *decode_ipcp_state(int state);

int sppp_proc_init (void);
void sppp_proc_cleanup (void);
#endif

#endif

/*Added by Nenad Corbic 
 *Dynamically configurable PPP options*/
static unsigned int sppp_keepalive_interval;
static unsigned int sppp_max_keepalive_count;

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,20))  
static void sppp_bh (void *sppp);
#else
static void sppp_bh (struct work_struct *work);
#endif

extern unsigned long wan_get_ip_address (netdevice_t *dev, int option);
extern unsigned long wan_set_ip_address (netdevice_t *dev, int option, unsigned long ip);
extern void wan_add_gateway(netdevice_t *dev);

static int debug;

/*
 * 
 */
static const struct cp chap = {
	PPP_CHAP, IDX_CHAP, CP_AUTH, "chap",
	sppp_null, sppp_null, sppp_chap_open, sppp_chap_close,
	sppp_chap_TO, 0, 0, 0,
	sppp_chap_tlu, sppp_chap_tld, sppp_null, sppp_null,
	sppp_chap_scr
};


/*
 *	Interface down stub
 */	

static void if_down(struct net_device *dev)
{
	struct sppp *sp = (struct sppp *)sppp_of(dev);
	WAN_ASSERT1((!sp));
	
	sp->pp_link_state=SPPP_LINK_DOWN;
}

/*
 * Timeout routine activations.
 */

static void sppp_set_timeout(struct sppp *p,int s) 
{
	if (! (p->pp_flags & PP_TIMO)) 
	{
		init_timer(&p->pp_timer);
		p->pp_timer.function=sppp_cp_timeout;
		p->pp_timer.expires=jiffies+s*HZ;
		p->pp_timer.data=(unsigned long)p;
		p->pp_flags |= PP_TIMO;
		add_timer(&p->pp_timer);
	}
}

static void sppp_clear_timeout(struct sppp *p)
{
	if (p->pp_flags & PP_TIMO) 
	{
		del_timer(&p->pp_timer);
		p->pp_flags &= ~PP_TIMO; 
	}
}

/*
 * Timeout routines for authentication protocols
 */

static void auth_timeout(void * function, struct sppp *p, unsigned int seconds) 
{
	//do nothing for now!!
	return;
	
	//printk(KERN_INFO "auth_timeout() : seconds : %u,  p->pp_auth_flags : 0x%X\n",
	//	seconds, p->pp_auth_flags);

	//only one of them can be waiting to timeout at a time
	if (! (p->pp_auth_flags & PP_TIMO1)) 
	{
		printk(KERN_INFO "starting pp_auth_timer...\n");
		init_timer(&p->pp_auth_timer);
		p->pp_auth_timer.function = function;
		p->pp_auth_timer.expires  = jiffies + seconds * HZ;
		p->pp_auth_timer.data = (unsigned long)p;
		p->pp_auth_flags |= PP_TIMO1;
		add_timer(&p->pp_auth_timer);
	}
}

static void auth_clear_timeout(struct sppp *p)
{
	//do nothing for now!!
	return;

	//printk(KERN_INFO "auth_clear_timeout()\n");

	if (p->pp_auth_flags & PP_TIMO1) 
	{
		printk("deleting pp_auth_timer timer...\n");
		del_timer(&p->pp_auth_timer);
		p->pp_auth_flags &= ~PP_TIMO1; 
	}
}

  
/**
 *	wp_sppp_input -	receive and process a WAN PPP frame
 *	@skb:	The buffer to process
 *	@dev:	The device it arrived on
 *
 *	This can be called directly by cards that do not have
 *	timing constraints but is normally called from the network layer
 *	after interrupt servicing to process frames queued via netif_rx().
 *
 *	We process the options in the card. If the frame is destined for
 *	the protocol stacks then it requeues the frame for the upper level
 *	protocol. If it is a control from it is processed and discarded
 *	here.
 */
 
void wp_sppp_input (struct net_device *dev, struct sk_buff *skb)
{
	struct ppp_header *h;
	struct sppp *sp = (struct sppp *)sppp_of(dev);
	
	skb->dev=dev;
	wan_skb_reset_mac_header(skb);
	wan_skb_reset_network_header(skb);


	if (dev->flags & IFF_RUNNING)
	{
		/* Count received bytes, add FCS and one flag */
		sp->ibytes+= skb->len + 3;
		sp->ipkts++;
	}

	if (skb->len <= PPP_HEADER_LEN) {
		/* Too small packet, drop it. */
		if (sp->pp_flags & PP_DEBUG)
			printk (KERN_DEBUG "%s: input packet is too small, %d bytes\n",
				dev->name, skb->len);
drop:           kfree_skb(skb);
		return;
	}

	/* Get PPP header. */
	h = (struct ppp_header *)skb->data;
	skb_pull(skb,sizeof(struct ppp_header));

	switch (h->address) {
	default:        /* Invalid PPP packet. */
invalid:        if (sp->pp_flags & PP_DEBUG)
			printk (KERN_WARNING "%s: invalid input packet <0x%x 0x%x 0x%x>\n",
				dev->name,
				h->address, h->control, ntohs (h->protocol));
		goto drop;
	case PPP_ALLSTATIONS:
		if (h->control != PPP_UI)
			goto invalid;
		if (sp->pp_flags & PP_CISCO) {
			if (sp->pp_flags & PP_DEBUG)
				printk (KERN_WARNING "%s: PPP packet in Cisco mode <0x%x 0x%x 0x%x>\n",
					dev->name,
					h->address, h->control, ntohs (h->protocol));
			goto drop;
		}

		switch (ntohs (h->protocol)) {
		default:
			if (sp->lcp.state == LCP_STATE_OPENED)
				sppp_cp_send (sp, PPP_LCP, LCP_PROTO_REJ,
					++sp->pp_seq[IDX_LCP], skb->len + 2,
					&h->protocol);
			if (sp->pp_flags & PP_DEBUG)
				printk (KERN_WARNING "%s: invalid input protocol <0x%x 0x%x 0x%x>\n",
					dev->name,
					h->address, h->control, ntohs (h->protocol));
			goto drop;
		case PPP_LCP:
			sppp_lcp_input (sp, skb);
			kfree_skb(skb);
			return;
		case PPP_IPCP:
			if (sp->lcp.state == LCP_STATE_OPENED)
				sppp_ipcp_input (sp, skb);
			else
				printk(KERN_DEBUG "IPCP when still waiting LCP finish.\n");
			kfree_skb(skb);
			return;
		case PPP_IP:
			if (sp->ipcp.state == IPCP_STATE_OPENED) {
				if(sp->pp_flags&PP_DEBUG)
					printk(KERN_DEBUG "Yow an IP frame.\n");
				skb->protocol=htons(ETH_P_IP);
				wan_skb_reset_mac_header(skb);
				wan_skb_reset_network_header(skb);
				netif_rx(skb);
				return;
			}
			break;
#ifdef IPX
		case PPP_IPX:
			/* IPX IPXCP not implemented yet */
			if (sp->lcp.state == LCP_STATE_OPENED) {
				skb->protocol=htons(ETH_P_IPX);
				wan_skb_reset_mac_header(skb);
				wan_skb_reset_network_header(skb);
				netif_rx(skb);
				return;
			}
			break;
#endif
		case PPP_PAP:
			{
				struct lcp_header * lcph = (struct lcp_header *)skb->data;
				int len = skb->len;
			
				switch (lcph->type) {
				
				case PAP_REQ:
					//Should not get here if peer. It is valid only for authenticator.
					printk(KERN_INFO "%s: Error : Got PAP_REQ !!\n", dev->name);
					break;
					
				case  PAP_ACK:
					skb->data[len] = '\0';
					//print the message in ack packet
					printk(KERN_INFO "%s: Pap Ack: Remote Message : %s\n", dev->name,
						&skb->data[sizeof(struct lcp_header) + sizeof(u8)]);
					
					//set LCP state to opened
					sp->lcp.state = LCP_STATE_OPENED;
					sppp_ipcp_open (sp);
					break;
					
				case  PAP_NAK:
					skb->data[len] = '\0';
					//print the message in nak packet
					printk(KERN_INFO "%s: Pap Nack: Remote Message : %s\n", dev->name,
						&skb->data[sizeof(struct lcp_header) + sizeof(u8)]);

					//await LCP shutdown by authenticator
					break;

				default:
					printk(KERN_INFO "%s : Unknown PAP packet !!!\n", dev->name);
					break;				
				}
			}

			break;

		case PPP_CHAP:
			sppp_chap_input(sp, skb);
			break;		
		}
		break;
	case CISCO_MULTICAST:
	case CISCO_UNICAST:
		/* Don't check the control field here (RFC 1547). */
		if (! (sp->pp_flags & PP_CISCO)) {
			if (sp->pp_flags & PP_DEBUG)
				printk (KERN_WARNING "%s: Cisco packet in PPP mode <0x%x 0x%x 0x%x>\n",
					dev->name,
					h->address, h->control, ntohs (h->protocol));
			goto drop;
		}
		switch (ntohs (h->protocol)) {
		default:
			goto invalid;
		case CISCO_KEEPALIVE:
			sppp_cisco_input (sp, skb);
			kfree_skb(skb);
			return;
#ifdef CONFIG_INET
		case ETH_P_IP:
			skb->protocol=htons(ETH_P_IP);
			wan_skb_reset_mac_header(skb);
			wan_skb_reset_network_header(skb);
			netif_rx(skb);
			return;
#endif
#ifdef CONFIG_IPX
		case ETH_P_IPX:
			skb->protocol=htons(ETH_P_IPX);
			wan_skb_reset_mac_header(skb);
			wan_skb_reset_network_header(skb);
			netif_rx(skb);
			return;
#endif
		}
		break;
	}
	kfree_skb(skb);
}

EXPORT_SYMBOL(wp_sppp_input);


/*
 *--------------------------------------------------------------------------*
 *                                                                          *
 *                        The CHAP implementation.                          *
 *                                                                          *
 *--------------------------------------------------------------------------*
 */

/*
 * The authentication protocols don't employ a full-fledged state machine as
 * the control protocols do, since they do have Open and Close events, but
 * not Up and Down, nor are they explicitly terminated.  Also, use of the
 * authentication protocols may be different in both directions (this makes
 * sense, think of a machine that never accepts incoming calls but only
 * calls out, it doesn't require the called party to authenticate itself).
 *
 * Our state machine for the local authentication protocol (we are requesting
 * the peer to authenticate) looks like:
 *
 *						    RCA-
 *	      +--------------------------------------------+
 *	      V					    scn,tld|
 *	  +--------+			       Close   +---------+ RCA+
 *	  |	   |<----------------------------------|	 |------+
 *   +--->| Closed |				TO*    | Opened	 | sca	|
 *   |	  |	   |-----+		       +-------|	 |<-----+
 *   |	  +--------+ irc |		       |       +---------+
 *   |	    ^		 |		       |	   ^
 *   |	    |		 |		       |	   |
 *   |	    |		 |		       |	   |
 *   |	 TO-|		 |		       |	   |
 *   |	    |tld  TO+	 V		       |	   |
 *   |	    |	+------->+		       |	   |
 *   |	    |	|	 |		       |	   |
 *   |	  +--------+	 V		       |	   |
 *   |	  |	   |<----+<--------------------+	   |
 *   |	  | Req-   | scr				   |
 *   |	  | Sent   |					   |
 *   |	  |	   |					   |
 *   |	  +--------+					   |
 *   | RCA- |	| RCA+					   |
 *   +------+	+------------------------------------------+
 *   scn,tld	  sca,irc,ict,tlu
 *
 *
 *   with:
 *
 *	Open:	LCP reached authentication phase
 *	Close:	LCP reached terminate phase
 *
 *	RCA+:	received reply (pap-req, chap-response), acceptable
 *	RCN:	received reply (pap-req, chap-response), not acceptable
 *	TO+:	timeout with restart counter >= 0
 *	TO-:	timeout with restart counter < 0
 *	TO*:	reschedule timeout for CHAP
 *
 *	scr:	send request packet (none for PAP, chap-challenge)
 *	sca:	send ack packet (pap-ack, chap-success)
 *	scn:	send nak packet (pap-nak, chap-failure)
 *	ict:	initialize re-challenge timer (CHAP only)
 *
 *	tlu:	this-layer-up, LCP reaches network phase
 *	tld:	this-layer-down, LCP enters terminate phase
 *
 * Note that in CHAP mode, after sending a new challenge, while the state
 * automaton falls back into Req-Sent state, it doesn't signal a tld
 * event to LCP, so LCP remains in network phase.  Only after not getting
 * any response (or after getting an unacceptable response), CHAP closes,
 * causing LCP to enter terminate phase.
 *
 * With PAP, there is no initial request that can be sent.  The peer is
 * expected to send one based on the successful negotiation of PAP as
 * the authentication protocol during the LCP option negotiation.
 *
 * Incoming authentication protocol requests (remote requests
 * authentication, we are peer) don't employ a state machine at all,
 * they are simply answered.  Some peers [Ascend P50 firmware rev
 * 4.50] react allergically when sending IPCP requests while they are
 * still in authentication phase (thereby violating the standard that
 * demands that these NCP packets are to be discarded), so we keep
 * track of the peer demanding us to authenticate, and only proceed to
 * phase network once we've seen a positive acknowledge for the
 * authentication.
 */

/*
 * Handle incoming CHAP packets.
 */
static void
sppp_chap_input(struct sppp *sp, struct sk_buff *skb)
{
	STDDCL;
	struct lcp_header *h;
	int len;//, x;
	u_char *value, *name, digest[AUTHKEYLEN], dsize;
	int value_len, name_len;
	struct wp_MD5Context ctx;
	
	//strcpy(sp->hisauth.name, "RYELLE_LINX");
	//strcpy(sp->hisauth.secret, "ryelle");

	len = skb->len;
	
	if (len < 4) {
		if (debug){
			printk(KERN_INFO "%s: chap invalid packet length: %d bytes\n",
				dev->name, len);
		}
		return;
	}

	h = (struct lcp_header *)skb->data;
		
	if (len > ntohs (h->len))
		len = ntohs (h->len);

	switch (h->type) {
	/* challenge, failure and success are his authproto */
	case CHAP_CHALLENGE:
		
		value = 1 + (u_char*)(h+1);
		value_len = value[-1];
		name = value + value_len;
		name_len = len - value_len - 5;
		
		if (name_len < 0) {
			if (debug){
				printk(	KERN_INFO "%s: chap corrupted challenge <%s id=0x%x len=%d",
					dev->name,
					sppp_auth_type_name(PPP_CHAP, h->type),
					h->ident, ntohs(h->len));
				
				sppp_print_bytes((u_char*) (h+1), len-4);
				printk(">\n");
			}
			break;
		}

		if (debug){
			printk(KERN_INFO
			    	"%s: chap input <%s id=0x%x len=%d name=",
				dev->name,
			    	sppp_auth_type_name(PPP_CHAP, h->type),
			    	h->ident,
			    	ntohs(h->len));
			
			sppp_print_string((char*) name, name_len);
			
			printk(" value-size=%d value=", value_len);
			sppp_print_bytes(value, value_len);
			printk(">\n");
		}
		
		/* Compute reply value. */
		wp_MD5Init(&ctx);
		wp_MD5Update(&ctx, &h->ident, 1);
		wp_MD5Update(&ctx, sp->myauth.secret, sppp_strnlen(sp->myauth.secret, AUTHKEYLEN));
		wp_MD5Update(&ctx, value, value_len);
		wp_MD5Final(digest, &ctx);
		dsize = sizeof digest;

		sppp_auth_send(sp, PPP_CHAP, CHAP_RESPONSE, h->ident,
			       sizeof dsize, (const char *)&dsize,
			       sizeof digest, digest,
			       (size_t)sppp_strnlen(sp->myauth.name, AUTHNAMELEN),
			       sp->myauth.name,
			       0);

		break;

	case CHAP_SUCCESS:

		if (debug){
			printk(KERN_INFO "%s: chap success ", dev->name);
			if (len > 4) {
				printk(": ");
				sppp_print_string((char*)(h + 1), len - 4);
			}
			printk("\n");
		}
		
		sp->pp_flags &= ~PP_NEEDAUTH;

		//set LCP state to opened
		sp->lcp.state = LCP_STATE_OPENED;
		sppp_ipcp_open (sp);

		break;

	case CHAP_FAILURE:

		if (debug){
			printk(KERN_INFO "%s: chap failure", dev->name);
			if (len > 4) {
				printk(": ");
				sppp_print_string((char*)(h + 1), len - 4);
			}
			printk("\n");
		}else
			printk(KERN_INFO "%s: chap failure\n", dev->name);
		
		/* await LCP shutdown by authenticator */
		break;

	/* response is my authproto */
	case CHAP_RESPONSE:

		printk(KERN_INFO "%s : CHAP_RESPONSE - not implemented yet.\n",
			dev->name);
		break;
		
		value = 1 + (u_char*)(h+1);
		value_len = value[-1];
		name = value + value_len;
		name_len = len - value_len - 5;
		if (name_len < 0) {
			if (debug) {
				printk(KERN_INFO
				    SPP_FMT "chap corrupted response "
				    "<%s id=0x%x len=%d",
				    SPP_ARGS(dev),
				    sppp_auth_type_name(PPP_CHAP, h->type),
				    h->ident, ntohs(h->len));
				sppp_print_bytes((u_char*)(h+1), len-4);
				printk(">\n");
			}
			break;
		}
		if (h->ident != sp->confid[IDX_CHAP]) {
			if (debug)
				printk(KERN_INFO SPP_FMT "chap dropping response for old ID "
				    "(got %d, expected %d)\n",
				    SPP_ARGS(dev),
				    h->ident, sp->confid[IDX_CHAP]);
			break;
		}
		if (name_len != sppp_strnlen(sp->hisauth.name, AUTHNAMELEN)
		    || memcmp(name, sp->hisauth.name, name_len) != 0) {
			printk(	KERN_INFO SPP_FMT "chap response, his name ",
			    	SPP_ARGS(dev));
			sppp_print_string(name, name_len);
			printk(" != expected ");
			sppp_print_string(sp->hisauth.name,
					  sppp_strnlen(sp->hisauth.name, AUTHNAMELEN));
			printk("\n");
		}
		if (debug) {
			printk(	KERN_INFO SPP_FMT "chap input(%s) "
				"<%s id=0x%x len=%d name=",
			    	SPP_ARGS(dev),
			    	sppp_state_name(sp->state[IDX_CHAP]),
			    	sppp_auth_type_name(PPP_CHAP, h->type),
			    	h->ident, ntohs (h->len));
			sppp_print_string((char*)name, name_len);
			printk(" value-size=%d value=", value_len);
			sppp_print_bytes(value, value_len);
			printk(">\n");
		}
		if (value_len != AUTHKEYLEN) {
			if (debug)
				printk(	KERN_INFO SPP_FMT "chap bad hash value length: "
					"%d bytes, should be %d\n",
				    	SPP_ARGS(dev), value_len,
				    	AUTHKEYLEN);
			break;
		}

		wp_MD5Init(&ctx);
		wp_MD5Update(&ctx, &h->ident, 1);
		wp_MD5Update(&ctx, sp->hisauth.secret,
			  sppp_strnlen(sp->hisauth.secret, AUTHKEYLEN));
		wp_MD5Update(&ctx, sp->myauth.challenge, AUTHKEYLEN);
		wp_MD5Final(digest, &ctx);

#define FAILMSG "Failed..."
#define SUCCMSG "Welcome!"

		if (value_len != sizeof digest ||
		    memcmp(digest, value, value_len) != 0) {
			// action scn, tld
			sppp_auth_send(sp, PPP_CHAP, CHAP_FAILURE, h->ident,
				       sizeof(FAILMSG) - 1, (u_char *)FAILMSG,
				       0);
			chap.tld(sp);
			break;
		}
		// action sca, perhaps tlu
		if (sp->state[IDX_CHAP] == STATE_REQ_SENT ||
		    sp->state[IDX_CHAP] == STATE_OPENED)
			sppp_auth_send(sp, PPP_CHAP, CHAP_SUCCESS, h->ident,
				       sizeof(SUCCMSG) - 1, (u_char *)SUCCMSG,
				       0);
		if (sp->state[IDX_CHAP] == STATE_REQ_SENT) {
			sppp_cp_change_state(&chap, sp, STATE_OPENED);
			chap.tlu(sp);
		}
		break;

	default:
		printk(KERN_INFO "Unknown CHAP packet type !!\n");
		
		/* Unknown CHAP packet type -- ignore. */
		
		if (sp->pp_flags & PP_DEBUG){
			printk(KERN_INFO "%s: chap unknown input <type=0x%x id=0x%xh len=%d",
				dev->name,
			    	h->type, h->ident, ntohs(h->len));
			
			sppp_print_bytes((u_char*)(h+1), len-4);

			printk(">\n");
		}
		
		break;

	}
}

/*
 * Send a PAP or CHAP proto packet.
 *
 * Varadic function, each of the elements for the ellipsis is of type
 * ``size_t mlen, const u_char *msg''.  Processing will stop iff
 * mlen == 0.
 * NOTE: never declare variadic functions with types subject to type
 * promotion (i.e. u_char). This is asking for big trouble depending
 * on the architecture you are on...
 */

static void
sppp_auth_send(struct sppp *sp, unsigned int protocol, unsigned int type,
	       unsigned int id, ...)
{
	struct net_device *dev = sp->pp_if;

	u8 * data_buf;
	unsigned int len, mlen;
	const char *msg;
	va_list ap;
	u_char *p;
	
	p = data_buf = kmalloc (MAX_AUTHENTICATE_PACKET_LEN, GFP_ATOMIC);
	
	if (data_buf == NULL){
		printk(KERN_INFO "%s: sppp_auth_send() : Failed to allocate memory !!\n",
			dev->name);
		return;
	}

	va_start(ap, id);
	len = 0;

	while ((mlen = (unsigned int)va_arg(ap, size_t)) != 0) {
		msg = va_arg(ap, const char *);
		len += mlen;
		
		//????
		if (len > MAX_AUTHENTICATE_PACKET_LEN) {
			printk(KERN_INFO "%s: sppp_auth_send() : Parameter array is too long !!\n",
				dev->name);
			va_end(ap);
			kfree(data_buf);
			return;
		}
		
		memmove(p, msg, mlen);
		p += mlen;
	}
	va_end(ap);

#if 0
	if (sp->pp_flags & PP_DEBUG){
		printk(KERN_INFO "%s: %s output <%s id=0x%x len=%d",
			dev->name,
		    	(protocol == PPP_PAP ? "pap" : "chap"),
		    	sppp_auth_type_name(protocol, type),
		    	id, len);
		
		sppp_print_bytes((u_char*) (data_buf), len);
		printk(">\n");
	}
#endif
	sppp_cp_send (sp, protocol, type, id, len, data_buf);
	kfree(data_buf);	
}

static const char *
sppp_auth_type_name(u_short proto, u_char type)
{
	static char buf[12];
	switch (proto) {
	case PPP_CHAP:
		switch (type) {
		case CHAP_CHALLENGE:	return "challenge";
		case CHAP_RESPONSE:	return "response";
		case CHAP_SUCCESS:	return "success";
		case CHAP_FAILURE:	return "failure";
		}
	case PPP_PAP:
		switch (type) {
		case PAP_REQ:		return "req";
		case PAP_ACK:		return "ack";
		case PAP_NAK:		return "nak";
		}
	}

	snprintf (buf, sizeof(buf), "auth/0x%x", type);
	return buf;
}

static void
sppp_print_string(const char *p, u_short len)
{
	u_char c;

	while (len-- > 0) {
		c = *p++;
		/*
		 * Print only ASCII chars directly.  RFC 1994 recommends
		 * using only them, but we don't rely on it.  */
		if (c < ' ' || c > '~')
			printk( "\\x%x", c);
		else
			printk( "%c", c);
	}
}

static void
sppp_chap_init(struct sppp *sp)
{
	/* Chap doesn't have STATE_INITIAL at all. */
	sp->state[IDX_CHAP] = STATE_CLOSED;
	sp->fail_counter[IDX_CHAP] = 0;
	sp->pp_seq[IDX_CHAP] = 0;
	sp->pp_rseq[IDX_CHAP] = 0;
#if defined(__FreeBSD__) && __FreeBSD__ >= 3
	callout_handle_init(&sp->ch[IDX_CHAP]);
#endif
}

static void
sppp_chap_open(struct sppp *sp)
{
	printk(KERN_INFO "sppp_chap_open()\n");
	
	if (sp->myauth.proto == PPP_CHAP &&
	    (sp->lcp.opts & (1 << LCP_OPT_AUTH_PROTO)) != 0) {
		/* we are authenticator for CHAP, start it */
		chap.scr(sp);
		sp->rst_counter[IDX_CHAP] = sp->lcp.max_configure;
		sppp_cp_change_state(&chap, sp, STATE_REQ_SENT);
	}
	/* nothing to be done if we are peer, await a challenge */
}

static void
sppp_chap_close(struct sppp *sp)
{
	printk(KERN_INFO "sppp_chap_close()\n");

	if (sp->state[IDX_CHAP] != STATE_CLOSED)
		sppp_cp_change_state(&chap, sp, STATE_CLOSED);
}

static void
sppp_chap_TO(void *cookie)
{
	struct sppp *sp = (struct sppp *)cookie;
	STDDCL;
	unsigned long flags;

	printk(KERN_INFO "sppp_chap_TO()\n");
	
	wan_spin_lock_irq(&authenticate_lock,&flags);

	if (debug)
		printk(KERN_INFO SPP_FMT "chap TO(%s) rst_counter = %d\n",
		    SPP_ARGS(dev),
		    sppp_state_name(sp->state[IDX_CHAP]),
		    sp->rst_counter[IDX_CHAP]);

	if (--sp->rst_counter[IDX_CHAP] < 0)
		/* TO- event */
		switch (sp->state[IDX_CHAP]) {
		case STATE_REQ_SENT:
			chap.tld(sp);
			sppp_cp_change_state(&chap, sp, STATE_CLOSED);
			break;
		}
	else
		/* TO+ (or TO*) event */
		switch (sp->state[IDX_CHAP]) {
		case STATE_OPENED:
			/* TO* event */
			sp->rst_counter[IDX_CHAP] = sp->lcp.max_configure;
			/* FALLTHROUGH */
		case STATE_REQ_SENT:
			chap.scr(sp);
			/* sppp_cp_change_state() will restart the timer */
			sppp_cp_change_state(&chap, sp, STATE_REQ_SENT);
			break;
		}

	wan_spin_unlock_irq(&authenticate_lock,&flags);
}

static void
sppp_chap_tlu(struct sppp *sp)
{
	STDDCL;
	int i;
	unsigned long flags;

	printk(KERN_INFO "sppp_chap_tlu()\n");
	
	i = 0;
	sp->rst_counter[IDX_CHAP] = sp->lcp.max_configure;

	/*
	 * Some broken CHAP implementations (Conware CoNet, firmware
	 * 4.0.?) don't want to re-authenticate their CHAP once the
	 * initial challenge-response exchange has taken place.
	 * Provide for an option to avoid rechallenges.
	 */
	if ((sp->hisauth.flags & AUTHFLAG_NORECHALLENGE) == 0) {
		/*
		 * Compute the re-challenge timeout.  This will yield
		 * a number between 300 and 810 seconds.
		 */
		//i = 300 + ((unsigned)(random() & 0xff00) >> 7);
		i = 300;// + ((jiffies & 0x0000ff00) >> 7);
		TIMEOUT(chap.TO, (void *)sp, i);

	}

	if (debug) {
		printk(	KERN_INFO SPP_FMT "chap %s, ",
			SPP_ARGS(dev),
		    	sp->pp_phase == PHASE_NETWORK? "reconfirmed": "tlu");

		if ((sp->hisauth.flags & AUTHFLAG_NORECHALLENGE) == 0)
			printk(KERN_INFO "next re-challenge in %d seconds\n", i);
		else
			printk(KERN_INFO "re-challenging supressed\n");
	}

	wan_spin_lock_irq(&authenticate_lock,&flags);

	/* indicate to LCP that we need to be closed down */
	sp->lcp.protos |= (1 << IDX_CHAP);

	if (sp->pp_flags & PP_NEEDAUTH) {
		/*
		 * Remote is authenticator, but his auth proto didn't
		 * complete yet.  Defer the transition to network
		 * phase.
		 */
		
		wan_spin_unlock_irq(&authenticate_lock,&flags);
		return;
	}
	
	wan_spin_unlock_irq(&authenticate_lock,&flags);

	/*
	 * If we are already in phase network, we are done here.  This
	 * is the case if this is a dummy tlu event after a re-challenge.
	 */
	if (sp->pp_phase != PHASE_NETWORK)
		sppp_phase_network(sp);
}

static void
sppp_chap_tld(struct sppp *sp)
{
	STDDCL;

	if (debug)
		printk(KERN_INFO SPP_FMT "chap tld\n", SPP_ARGS(dev));
	UNTIMEOUT( (void *)sp );
	sp->lcp.protos &= ~(1 << IDX_CHAP);

	//lcp.Close(sp);
	
	//????
	/* Shut down the PPP link. */
	sp->lcp.magic = jiffies;
	sp->lcp.state = LCP_STATE_CLOSED;
	sp->ipcp.state = IPCP_STATE_CLOSED;
	//sppp_clear_timeout (sp);
	/* Initiate negotiation. */
	sppp_lcp_open (sp);
}

static void
sppp_chap_scr(struct sppp *sp)
{
	u_long *ch, seed;
	u_char clen;

	printk(KERN_INFO "sppp_chap_scr()\n");
	
	/* Compute random challenge. */
	ch = (u_long *)sp->myauth.challenge;
	
#if defined(__FreeBSD__) && __FreeBSD__ >= 3
	read_random(&seed, sizeof seed);
#else
	{
		//struct timeval tv;
		//microtime(&tv);
		//seed = tv.tv_sec ^ tv.tv_usec;
		seed = jiffies;
	}
#endif
	ch[0] = jiffies & 0x000000ff;//seed ^ random();
	ch[1] = jiffies & 0x0000ff00;//seed ^ random();
	ch[2] = jiffies & 0x00ff0000;//seed ^ random();
	ch[3] = jiffies & 0xff000000;//seed ^ random();
	clen = AUTHKEYLEN;

	sp->confid[IDX_CHAP] = ++sp->pp_seq[IDX_CHAP];

	sppp_auth_send(sp, PPP_CHAP, CHAP_CHALLENGE, sp->confid[IDX_CHAP],
		       sizeof clen, (const char *)&clen,
		       (size_t)AUTHKEYLEN, sp->myauth.challenge,
		       (size_t)sppp_strnlen(sp->myauth.name, AUTHNAMELEN),
		       sp->myauth.name,
		       0);
}

/*
 * Change the state of a control protocol in the state automaton.
 * Takes care of starting/stopping the restart timer.
 */
static void
sppp_cp_change_state(const struct cp *cp, struct sppp *sp, int newstate)
{
	sp->state[cp->protoidx] = newstate;

	UNTIMEOUT((void *)sp);
	
	switch (newstate) {
	case STATE_INITIAL:
	case STATE_STARTING:
	case STATE_CLOSED:
	case STATE_STOPPED:
	case STATE_OPENED:
		break;
	case STATE_CLOSING:
	case STATE_STOPPING:
	case STATE_REQ_SENT:
	case STATE_ACK_RCVD:
	case STATE_ACK_SENT:
		
		TIMEOUT(cp->TO, (void *)sp, sp->lcp.timeout);
		break;
	}
}


/* a dummy, used to drop uninteresting events */
static void
sppp_null(struct sppp *unused)
{
	printk(KERN_INFO "sppp_null()\n");
	//UNTIMEOUT( unused );
	/* do just nothing */
}

static void
sppp_phase_network(struct sppp *sp)
{
	STDDCL;
	//int i;
	//u32 mask;

	sp->pp_phase = PHASE_NETWORK;

	if (debug)
		printk(KERN_INFO SPP_FMT "phase %s\n", SPP_ARGS(dev),
			sppp_phase_name(sp->pp_phase));

	/* Notify NCPs now. */
	//????????
	/*
	for (i = 0; i < IDX_COUNT; i++)
		if ((cps[i])->flags & CP_NCP)
			(cps[i])->Open(sp);
	*/
	/* Send Up events to all NCPs. */
	//????????
	/*
	for (i = 0, mask = 1; i < IDX_COUNT; i++, mask <<= 1)
		if ((sp->lcp.protos & mask) && ((cps[i])->flags & CP_NCP))
			(cps[i])->Up(sp);
	*/
	/* if no NCP is starting, all this was in vain, close down */
	//sppp_lcp_check_and_close(sp);
	
	sp->ipcp.state = IPCP_STATE_CLOSED;
	/* Initiate renegotiation. */
	sppp_ipcp_open (sp);
}

/*
 * Check the open NCPs, return true if at least one NCP is open.
 */
/*
static int
sppp_ncp_check(struct sppp *sp)
{
	int i, mask;

	//?????
	for (i = 0, mask = 1; i < IDX_COUNT; i++, mask <<= 1)
		if ((sp->lcp.protos & mask) && (cps[i])->flags & CP_NCP)
			return 1;
	return 0;
	
	
}
*/

/*
 * Re-check the open NCPs and see if we should terminate the link.
 * Called by the NCPs during their tlf action handling.
 */
/*
static void
sppp_lcp_check_and_close(struct sppp *sp)
{

	if (sp->pp_phase < PHASE_NETWORK)
		// don't bother, we are already going down
		return;

	if (sppp_ncp_check(sp))
		return;

	lcp.Close(sp);
}
*/

static const char *
sppp_state_name(int state)
{
	switch (state) {
	case STATE_INITIAL:	return "initial";
	case STATE_STARTING:	return "starting";
	case STATE_CLOSED:	return "closed";
	case STATE_STOPPED:	return "stopped";
	case STATE_CLOSING:	return "closing";
	case STATE_STOPPING:	return "stopping";
	case STATE_REQ_SENT:	return "req-sent";
	case STATE_ACK_RCVD:	return "ack-rcvd";
	case STATE_ACK_SENT:	return "ack-sent";
	case STATE_OPENED:	return "opened";
	}
	return "illegal";
}

static const char *
sppp_phase_name(enum ppp_phase phase)
{
	switch (phase) {
	case PHASE_DEAD:	return "dead";
	case PHASE_ESTABLISH:	return "establish";
	case PHASE_TERMINATE:	return "terminate";
	case PHASE_AUTHENTICATE: return "authenticate";
	case PHASE_NETWORK:	return "network";
	}
	return "illegal";
}

////////////////////////////

/*
 *	Handle transmit packets.
 */
 




#ifdef LINUX_FEAT_2624
static int sppp_hard_header(struct sk_buff *skb,
			    struct net_device *dev, __u16 type,
			    const void *daddr, const void *saddr,
			    unsigned int len)
#else
static int sppp_hard_header(struct sk_buff *skb, struct net_device *dev, __u16 type,
		void *daddr, void *saddr, unsigned int len)
#endif
{
	struct sppp *sp = (struct sppp *)sppp_of(dev);
	struct ppp_header *h;

	WAN_ASSERT2((!sp),0);

	if (skb_headroom(skb) < sizeof(struct ppp_header)){
		DEBUG_EVENT("%s: Error in hard_header() headroom (%d) < ppp hdr (%d)\n",
				dev->name,
				skb_headroom(skb),
				sizeof(struct ppp_header));
		return 0;
	}
	
	skb_push(skb,sizeof(struct ppp_header));
	h=(struct ppp_header *)skb->data;
	if(sp->pp_flags&PP_CISCO)
	{
		h->address = CISCO_UNICAST;
		h->control = 0;
	}
	else
	{
		h->address = PPP_ALLSTATIONS;
		h->control = PPP_UI;
	}
	if(sp->pp_flags & PP_CISCO)
	{
		h->protocol = htons(type);
	}
	else switch(type)
	{
		case ETH_P_IP:
			h->protocol = htons(PPP_IP);
			break;
		case ETH_P_IPX:
			h->protocol = htons(PPP_IPX);
			break;
	}
	return sizeof(struct ppp_header);
}

#ifdef LINUX_FEAT_2624
static const struct header_ops sppp_header_ops = {
	.create = sppp_hard_header,
};
#endif



/*
 * Send keepalive packets, every 10 seconds.
 */

static void sppp_keepalive (unsigned long dummy)
{
	struct sppp *sp;
	//unsigned long flags;

	//wan_spin_lock_irq(&spppq_lock, flags);

	for (sp=spppq; sp; sp=sp->pp_next) 
	{
		struct net_device *dev = sp->pp_if;

		/* Keepalive mode disabled or channel down? */
		if (! (sp->pp_flags & PP_KEEPALIVE) ||
		    ! (dev->flags & IFF_UP))
			continue;

		/* No keepalive in PPP mode if LCP not opened yet. */
		if (! (sp->pp_flags & PP_CISCO) &&
		    sp->lcp.state != LCP_STATE_OPENED)
			continue;

		if (sp->pp_alivecnt == sppp_max_keepalive_count) {
			/* No keepalive packets got.  Stop the interface. */
			printk (KERN_WARNING "%s: protocol down\n", dev->name);
			if_down (dev);
			if (! (sp->pp_flags & PP_CISCO)) {
				/* Shut down the PPP link. */
				sp->lcp.magic = jiffies;
				sp->lcp.state = LCP_STATE_CLOSED;
				sp->ipcp.state = IPCP_STATE_CLOSED;
				sppp_clear_timeout (sp);
				/* Initiate negotiation. */
				sppp_lcp_open (sp);
			}
		}
		if (sp->pp_alivecnt <= sppp_max_keepalive_count)
			++sp->pp_alivecnt;
		if (sp->pp_flags & PP_CISCO)
			sppp_cisco_send (sp, CISCO_KEEPALIVE_REQ, ++sp->pp_seq[IDX_LCP],
				sp->pp_rseq[IDX_LCP]);
		else if (sp->lcp.state == LCP_STATE_OPENED) {
			long nmagic = htonl (sp->lcp.magic);
			sp->lcp.echoid = ++sp->pp_seq[IDX_LCP];
			sppp_cp_send (sp, PPP_LCP, LCP_ECHO_REQ,
				sp->lcp.echoid, 4, &nmagic);
		}
	}
	//wan_spin_unlock_irq(&spppq_lock, &flags);
	sppp_keepalive_timer.expires=jiffies+sppp_keepalive_interval*HZ;
	add_timer(&sppp_keepalive_timer);
}

/*
 * Handle incoming PPP Link Control Protocol packets.
 */
 
static void sppp_lcp_input (struct sppp *sp, struct sk_buff *skb)
{
	struct lcp_header *h;
	struct net_device *dev = sp->pp_if;
	int len = skb->len;
	u8 *p, opt[6];
	u32 rmagic=0;
	int rc_from_lcp_options = 0;

	if (len < 4) {
		if (sp->pp_flags & PP_DEBUG)
			printk (KERN_WARNING "%s: invalid lcp packet length: %d bytes\n",
				dev->name, len);
		return;
	}
	h = (struct lcp_header *)skb->data;
	skb_pull(skb,sizeof(struct lcp_header *));
	
	if (sp->pp_flags & PP_DEBUG) 
	{
		char state = '?';
		switch (sp->lcp.state) {
		case LCP_STATE_CLOSED:   state = 'C'; break;
		case LCP_STATE_ACK_RCVD: state = 'R'; break;
		case LCP_STATE_ACK_SENT: state = 'S'; break;
		case LCP_STATE_OPENED:   state = 'O'; break;
		}
		printk (KERN_WARNING "%s: lcp input(%c): %d bytes <%s id=%xh len=%xh",
			dev->name, state, len,
			sppp_lcp_type_name (h->type), h->ident, ntohs (h->len));
		if (len > 4)
			sppp_print_bytes ((u8*) (h+1), len-4);
		printk (">\n");
	}

	if (len > ntohs (h->len))
		len = ntohs (h->len);
	switch (h->type) {
	default:
		/* Unknown packet type -- send Code-Reject packet. */
		sppp_cp_send (sp, PPP_LCP, LCP_CODE_REJ, ++sp->pp_seq[IDX_LCP],
			skb->len, h);
		break;
	case LCP_CONF_REQ:

		if (len < 4) {
			if (sp->pp_flags & PP_DEBUG)
				printk (KERN_DEBUG"%s: invalid lcp configure request packet length: %d bytes\n",
					dev->name, len);
			break;
		}

		rc_from_lcp_options = sppp_lcp_conf_parse_options (sp, h, len, &rmagic);
			
		if (len>4 && !rc_from_lcp_options)
			goto badreq;

		if (rmagic == sp->lcp.magic) {
			/* Local and remote magics equal -- loopback? */
			if (sp->pp_loopcnt >= sppp_max_keepalive_count*5) {
				printk (KERN_WARNING "%s: loopback\n",
					dev->name);
				sp->pp_loopcnt = 0;
				if (dev->flags & IFF_UP) {
					if_down (dev);
				}
			} else if (sp->pp_flags & PP_DEBUG)
				printk (KERN_DEBUG "%s: conf req: magic glitch\n",
					dev->name);
			++sp->pp_loopcnt;

			/* MUST send Conf-Nack packet. */
			rmagic = ~sp->lcp.magic;
			opt[0] = LCP_OPT_MAGIC;
			opt[1] = sizeof (opt);
			opt[2] = rmagic >> 24;
			opt[3] = rmagic >> 16;
			opt[4] = rmagic >> 8;
			opt[5] = rmagic;
			sppp_cp_send (sp, PPP_LCP, LCP_CONF_NAK,
				h->ident, sizeof (opt), &opt);
badreq:
			switch (sp->lcp.state) {
			case LCP_STATE_OPENED:
				/* Initiate renegotiation. */
				sppp_lcp_open (sp);
				/* fall through... */
			case LCP_STATE_ACK_SENT:
				/* Go to closed state. */
				sp->lcp.state = LCP_STATE_CLOSED;
				sp->ipcp.state = IPCP_STATE_CLOSED;
			}
			break;
		}
		/* Send Configure-Ack packet. */
		sp->pp_loopcnt = 0;

		if (sp->lcp.state != LCP_STATE_OPENED) {
			sppp_cp_send (sp, PPP_LCP, LCP_CONF_ACK, h->ident, len-4, h+1);
		}

		//AFTER the ack send PAP request
		//Wait until LCP is finished
#if 0
		if(rc_from_lcp_options == PPP_PAP){
			
			//authenticator wants PAP. initiate PAP request.
			sp->confid[IDX_PAP] = h->ident;
			sppp_pap_scr(sp);
			break;
		}
#endif
		//NC. Kernel change 
		//sppp_cp_send (sp, PPP_LCP, LCP_CONF_ACK,
		//		h->ident, len-4, h+1);
				
		/* Change the state. */
		switch (sp->lcp.state) {
		case LCP_STATE_CLOSED:
			sp->lcp.state = LCP_STATE_ACK_SENT;
			break;
		case LCP_STATE_ACK_RCVD:
			sp->lcp.state = LCP_STATE_OPENED;

			/* 3/20/2006 CXH begin */
			if(sp->pp_flags & PP_NEEDAUTH){
				//authenticator wants PAP. initiate PAP request.
				sp->confid[IDX_PAP] = h->ident;
				sppp_pap_scr(sp);

				/* we don't want to continue to wpsppp_ipcp_open() 
				 * yet, PAP_ACK will do it for us */
				break;  
			}
			/* CXH end */
			
			sppp_ipcp_open (sp);
			break;
		case LCP_STATE_OPENED:
			/* Remote magic changed -- close session. */
			sp->lcp.state = LCP_STATE_CLOSED;
			sp->ipcp.state = IPCP_STATE_CLOSED;
			/* Initiate renegotiation. */
			sppp_lcp_open (sp);

			/* Send ACK after our REQ in attempt to break loop */
			sppp_cp_send (sp, PPP_LCP, LCP_CONF_ACK,
				h->ident, len-4, h+1);
			sp->lcp.state = LCP_STATE_ACK_SENT;
			break;
		}
		break;
	case LCP_CONF_ACK:
		
		if (h->ident != sp->lcp.confid)
			break;
		sppp_clear_timeout (sp);
		
		if ((sp->pp_link_state != SPPP_LINK_UP) &&
		    (dev->flags & IFF_UP)) {
			/* Coming out of loopback mode. */
			sp->pp_link_state=SPPP_LINK_UP;
			printk (KERN_INFO "%s: protocol up\n", dev->name);
		}
		
		switch (sp->lcp.state) {
		case LCP_STATE_CLOSED:
			sp->lcp.state = LCP_STATE_ACK_RCVD;
			sppp_set_timeout (sp, 5);
			break;
		case LCP_STATE_ACK_SENT:
			sp->lcp.state = LCP_STATE_OPENED;
			
			/* 3/20/2006 CXH begin */
			if(sp->pp_flags & PP_NEEDAUTH){
				//authenticator wants PAP. initiate PAP request.
				sp->confid[IDX_PAP] = h->ident;
				sppp_pap_scr(sp);

				/* we don't want to continue to wpsppp_ipcp_open() 
				 * yet, PAP_ACK will do it for us */
				break;  
			}
			/* CXH end */

			sppp_ipcp_open (sp);
			break;
		}
		break;
	case LCP_CONF_NAK:
		if (h->ident != sp->lcp.confid)
			break;
		p = (u8*) (h+1);
		if (len>=10 && p[0] == LCP_OPT_MAGIC && p[1] >= 4) {
			rmagic = (u32)p[2] << 24 |
				(u32)p[3] << 16 | p[4] << 8 | p[5];
			if (rmagic == ~sp->lcp.magic) {
				int newmagic;
				if (sp->pp_flags & PP_DEBUG)
					printk (KERN_DEBUG "%s: conf nak: magic glitch\n",
						dev->name);
				get_random_bytes(&newmagic, sizeof(newmagic));
				sp->lcp.magic += newmagic;
			} else
				sp->lcp.magic = rmagic;
			}
		if (sp->lcp.state != LCP_STATE_ACK_SENT) {
			/* Go to closed state. */
			sp->lcp.state = LCP_STATE_CLOSED;
			sp->ipcp.state = IPCP_STATE_CLOSED;
		}
		/* The link will be renegotiated after timeout,
		 * to avoid endless req-nack loop. */
		sppp_clear_timeout (sp);
		sppp_set_timeout (sp, 2);
		break;
	case LCP_CONF_REJ:
		if (h->ident != sp->lcp.confid)
			break;
		sppp_clear_timeout (sp);
		/* Initiate renegotiation. */
		sppp_lcp_open (sp);
		if (sp->lcp.state != LCP_STATE_ACK_SENT) {
			/* Go to closed state. */
			sp->lcp.state = LCP_STATE_CLOSED;
			sp->ipcp.state = IPCP_STATE_CLOSED;
		}
		break;
	case LCP_TERM_REQ:
		sppp_clear_timeout (sp);
		/* Send Terminate-Ack packet. */
		sppp_cp_send (sp, PPP_LCP, LCP_TERM_ACK, h->ident, 0, 0);
		/* Go to closed state. */
		sp->lcp.state = LCP_STATE_CLOSED;
		sp->ipcp.state = IPCP_STATE_CLOSED;

		/* Added by Nenad Corbic
		 * The Link should go down if LCP_TERM_REQ is
		 * received. */
		if (sp->pp_link_state){ 	
			printk (KERN_INFO "%s: protocol down\n", dev->name);
			if_down(dev);
		}
		
		/* Initiate renegotiation. */
		sppp_lcp_open (sp);
		break;
	case LCP_TERM_ACK:
	case LCP_CODE_REJ:
	case LCP_PROTO_REJ:
		/* Ignore for now. */
		break;
	case LCP_DISC_REQ:
		/* Discard the packet. */
		break;
	case LCP_ECHO_REQ:
		if (sp->lcp.state != LCP_STATE_OPENED)
			break;
		if (len < 8) {
			if (sp->pp_flags & PP_DEBUG)
				printk (KERN_WARNING "%s: invalid lcp echo request packet length: %d bytes\n",
					dev->name, len);
			break;
		}
		if (ntohl (*(long*)(h+1)) == sp->lcp.magic) {
			/* Line loopback mode detected. */
			printk (KERN_WARNING "%s: loopback\n", dev->name);
			if_down (dev);

			/* Shut down the PPP link. */
			sp->lcp.state = LCP_STATE_CLOSED;
			sp->ipcp.state = IPCP_STATE_CLOSED;
			sppp_clear_timeout (sp);
			/* Initiate negotiation. */
			sppp_lcp_open (sp);
			break;
		}
		*(long*)(h+1) = htonl (sp->lcp.magic);
		sppp_cp_send (sp, PPP_LCP, LCP_ECHO_REPLY, h->ident, len-4, h+1);
		break;
	case LCP_ECHO_REPLY:
		if (h->ident != sp->lcp.echoid)
			break;
		if (len < 8) {
			if (sp->pp_flags & PP_DEBUG)
				printk (KERN_WARNING "%s: invalid lcp echo reply packet length: %d bytes\n",
					dev->name, len);
			break;
		}
		if (ntohl (*(long*)(h+1)) != sp->lcp.magic)
		sp->pp_alivecnt = 0;
		break;
	}
}


static void sppp_pap_scr(struct sppp *sp)
{
	u_char idlen, pwdlen;
	
	//sp->confid[IDX_PAP] = ++sp->pp_seq[IDX_PAP];
	pwdlen = sppp_strnlen(sp->myauth.secret, AUTHKEYLEN);
	idlen = sppp_strnlen(sp->myauth.name, AUTHNAMELEN);

	sppp_auth_send(sp, PPP_PAP, PAP_REQ, sp->confid[IDX_PAP],
		       sizeof idlen, (const char *)&idlen,
		       (size_t)idlen, sp->myauth.name,
		       sizeof pwdlen, (const char *)&pwdlen,
		       (size_t)pwdlen, sp->myauth.secret,
		       0);
}

static int
sppp_strnlen(u_char *p, int max)
{
	int len;

	for (len = 0; len < max && *p; ++p)
		++len;
	return len;
}


/*
 * Handle incoming Cisco keepalive protocol packets.
 */

static void sppp_cisco_input (struct sppp *sp, struct sk_buff *skb)
{
	struct cisco_packet *h;
	struct net_device *dev = sp->pp_if;

	if (skb->len != CISCO_PACKET_LEN && skb->len != CISCO_BIG_PACKET_LEN) {
		if (sp->pp_flags & PP_DEBUG)
			printk (KERN_WARNING "%s: invalid cisco packet length: %d bytes\n",
				dev->name,  skb->len);
		return;
	}
	h = (struct cisco_packet *)skb->data;
	skb_pull(skb, sizeof(struct cisco_packet*));
	if (sp->pp_flags & PP_DEBUG)
		printk (KERN_WARNING "%s: cisco input: %d bytes <%xh %xh %xh %xh %xh-%xh>\n",
			dev->name,  skb->len,
			ntohl (h->type), h->par1, h->par2, h->rel,
			h->time0, h->time1);
	switch (ntohl (h->type)) {
	default:
		if (sp->pp_flags & PP_DEBUG)
			printk (KERN_WARNING "%s: unknown cisco packet type: 0x%x\n",
				dev->name,  ntohl (h->type));
		break;
	case CISCO_ADDR_REPLY:
		/* Reply on address request, ignore */
		break;
	case CISCO_KEEPALIVE_REQ:
		sp->pp_alivecnt = 0;
		sp->pp_rseq[IDX_LCP] = ntohl (h->par1);
		if (sp->pp_seq[IDX_LCP] == sp->pp_rseq[IDX_LCP]) {
			/* Local and remote sequence numbers are equal.
			 * Probably, the line is in loopback mode. */
			int newseq;
			if (sp->pp_loopcnt >= sppp_max_keepalive_count) {
				printk (KERN_WARNING "%s: loopback\n",
					dev->name);
				sp->pp_loopcnt = 0;
				if (dev->flags & IFF_UP) {
					if_down (dev);
				}
			}
			++sp->pp_loopcnt;

			/* Generate new local sequence number */
			get_random_bytes(&newseq, sizeof(newseq));
			sp->pp_seq[IDX_LCP] ^= newseq;
			break;
		}
		sp->pp_loopcnt = 0;
		if (sp->pp_link_state==SPPP_LINK_DOWN &&
		    (dev->flags & IFF_UP)) {
			sp->pp_link_state=SPPP_LINK_UP;
			printk (KERN_INFO "%s: protocol up\n", dev->name);
		}
		break;
	case CISCO_ADDR_REQ:
		/* Stolen from net/ipv4/devinet.c -- SIOCGIFADDR ioctl */
		{
		struct in_device *in_dev;
		struct in_ifaddr *ifa;
		u32 addr = 0, mask = ~0; /* FIXME: is the mask correct? */
#ifdef CONFIG_INET

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,3,0)
		if ((in_dev=in_dev_get(dev)) != NULL)
		{
			wp_rcu_read_lock(in_dev);
			for (ifa=in_dev->ifa_list; ifa != NULL;
				ifa=ifa->ifa_next) {
				if (strcmp(dev->name, ifa->ifa_label) == 0) 
				{
					addr = ifa->ifa_local;
					mask = ifa->ifa_mask;
					break;
				}
			}
			wp_rcu_read_unlock(in_dev);
			in_dev_put(in_dev);
		}
#else
		if ((in_dev=dev->ip_ptr) != NULL)
		{
			for (ifa=in_dev->ifa_list; ifa != NULL;
				ifa=ifa->ifa_next)
				if (strcmp(dev->name, ifa->ifa_label) == 0) 
				{
					addr = ifa->ifa_local;
					mask = ifa->ifa_mask;
					break;
				}
		}
#endif
	
#endif		
		/* I hope both addr and mask are in the net order */
		sppp_cisco_send (sp, CISCO_ADDR_REPLY, addr, mask);
		break;
		}
	}
}

/*
 * Send PPP LCP packet.
 */
static void sppp_cp_send (struct sppp *sp, u16 proto, u8 type,
	u8 ident, u16 len, void *data)
{
	struct ppp_header *h;
	struct lcp_header *lh;
	struct sk_buff *skb;
	struct net_device *dev = sp->pp_if;

	skb=alloc_skb(dev->hard_header_len+PPP_HEADER_LEN+LCP_HEADER_LEN+len,
		GFP_ATOMIC);
	if (skb==NULL)
		return;

	skb_reserve(skb,dev->hard_header_len);
	
	h = (struct ppp_header *)skb_put(skb, sizeof(struct ppp_header));
	h->address = PPP_ALLSTATIONS;        /* broadcast address */
	h->control = PPP_UI;                 /* Unnumbered Info */
	h->protocol = htons (proto);         /* Link Control Protocol */

	lh = (struct lcp_header *)skb_put(skb, sizeof(struct lcp_header));
	lh->type = type;
	lh->ident = ident;
	lh->len = htons (LCP_HEADER_LEN + len);

	if (len)
		memcpy(skb_put(skb,len),data, len);

	if (sp->pp_flags & PP_DEBUG) {

		switch (proto){

		case PPP_PAP:
		case PPP_CHAP:
			printk(KERN_INFO "%s: %s output <%s id=0x%x len=%d",
			dev->name,
		    	(proto == PPP_PAP ? "pap" : "chap"),
		    	sppp_auth_type_name(proto, type),
		    	lh->ident, len);
			break;
			
		default:
			printk (KERN_INFO "%s: %s output <%s id=%xh len=%xh",
				dev->name, 
				proto==PPP_LCP ? "lcp" : "ipcp",
				proto==PPP_LCP ? sppp_lcp_type_name (lh->type) :
				sppp_ipcp_type_name (lh->type), lh->ident,
				ntohs (lh->len));
			
			break;
		}
		
		if (len)
			sppp_print_bytes ((u8*) (lh+1), len);
		printk (">\n");
	}
	sp->obytes += skb->len;

	/* Control is high priority so it doesnt get queued behind data */
	skb->priority=TC_PRIO_CONTROL;
	skb->dev = dev;
	skb->protocol = htons(PPP_IP);
	wan_skb_reset_mac_header(skb);
	wan_skb_reset_network_header(skb);
	dev_queue_xmit(skb);
}

/*
 * Send Cisco keepalive packet.
 */

static void sppp_cisco_send (struct sppp *sp, int type, long par1, long par2)
{
	struct ppp_header *h;
	struct cisco_packet *ch;
	struct sk_buff *skb;
	struct net_device *dev = sp->pp_if;
	u32 t = jiffies * 1000/HZ;

	skb=alloc_skb(dev->hard_header_len+PPP_HEADER_LEN+CISCO_PACKET_LEN,
		GFP_ATOMIC);

	if(skb==NULL)
		return;
		
	skb_reserve(skb, dev->hard_header_len);
	h = (struct ppp_header *)skb_put (skb, sizeof(struct ppp_header));
	h->address = CISCO_MULTICAST;
	h->control = 0;
	h->protocol = htons (CISCO_KEEPALIVE);

	ch = (struct cisco_packet*)skb_put(skb, CISCO_PACKET_LEN);
	ch->type = htonl (type);
	ch->par1 = htonl (par1);
	ch->par2 = htonl (par2);
	ch->rel = -1;
	ch->time0 = htons ((u16) (t >> 16));
	ch->time1 = htons ((u16) t);

	if (sp->pp_flags & PP_DEBUG)
		printk (KERN_WARNING "%s: cisco output: <%xh %xh %xh %xh %xh-%xh>\n",
			dev->name,  ntohl (ch->type), ch->par1,
			ch->par2, ch->rel, ch->time0, ch->time1);
	sp->obytes += skb->len;
	skb->priority=TC_PRIO_CONTROL;
	skb->dev = dev;
	wan_skb_reset_mac_header(skb);
	wan_skb_reset_network_header(skb);
	dev_queue_xmit(skb);
}

/**
 *	wp_sppp_close - close down a synchronous PPP or Cisco HDLC link
 *	@dev: The network device to drop the link of
 *
 *	This drops the logical interface to the channel. It is not
 *	done politely as we assume we will also be dropping DTR. Any
 *	timeouts are killed.
 */

int wp_sppp_close (struct net_device *dev)
{
	struct sppp *sp = (struct sppp *)sppp_of(dev);

	WAN_ASSERT2((!sp),-ENODEV);

	sp->pp_link_state = SPPP_LINK_DOWN;
	sp->lcp.state = LCP_STATE_CLOSED;
	sp->ipcp.state = IPCP_STATE_CLOSED;
	sppp_clear_timeout (sp);
	auth_clear_timeout(sp);

	return 0;
}

EXPORT_SYMBOL(wp_sppp_close);

/**
 *	wp_sppp_open - open a synchronous PPP or Cisco HDLC link
 *	@dev:	Network device to activate
 *	
 *	Close down any existing synchronous session and commence
 *	from scratch. In the PPP case this means negotiating LCP/IPCP
 *	and friends, while for Cisco HDLC we simply need to start sending
 *	keepalives
 */

int wp_sppp_open (struct net_device *dev)
{
	struct sppp *sp = (struct sppp *)sppp_of(dev);
	
	WAN_ASSERT2((!sp),-ENODEV);
	
	wp_sppp_close(dev);
	if (!(sp->pp_flags & PP_CISCO)) {
		sppp_lcp_open (sp);
	}
	sp->pp_link_state = SPPP_LINK_DOWN;
	return 0;
}

EXPORT_SYMBOL(wp_sppp_open);

/**
 *	wp_sppp_reopen - notify of physical link loss
 *	@dev: Device that lost the link
 *
 *	This function informs the synchronous protocol code that
 *	the underlying link died (for example a carrier drop on X.21)
 *
 *	We increment the magic numbers to ensure that if the other end
 *	failed to notice we will correctly start a new session. It happens
 *	do to the nature of telco circuits is that you can lose carrier on
 *	one endonly.
 *
 *	Having done this we go back to negotiating. This function may
 *	be called from an interrupt context.
 */
 
int wp_sppp_reopen (struct net_device *dev)
{
	struct sppp *sp = (struct sppp *)sppp_of(dev);

	WAN_ASSERT2((!sp),-ENODEV);
	
	wp_sppp_close(dev);
	if (!(sp->pp_flags & PP_CISCO))
	{
		sp->lcp.magic = jiffies;
		++sp->pp_seq[IDX_LCP];
		sp->lcp.state = LCP_STATE_CLOSED;
		sp->ipcp.state = IPCP_STATE_CLOSED;
		/* Give it a moment for the line to settle then go */
		sppp_set_timeout (sp, 1);
	} 
	sp->pp_link_state=SPPP_LINK_DOWN;
	return 0;
}

EXPORT_SYMBOL(wp_sppp_reopen);

/**
 *	wp_sppp_change_mtu - Change the link MTU
 *	@dev:	Device to change MTU on
 *	@new_mtu: New MTU
 *
 *	Change the MTU on the link. This can only be called with
 *	the link down. It returns an error if the link is up or
 *	the mtu is out of range.
 */
 
int wp_sppp_change_mtu(struct net_device *dev, int new_mtu)
{
	if(new_mtu<128||new_mtu>PPP_MTU||(dev->flags&IFF_UP))
		return -EINVAL;
	dev->mtu=new_mtu;
	return 0;
}

EXPORT_SYMBOL(wp_sppp_change_mtu);

/**
 *	wp_sppp_do_ioctl - Ioctl handler for ppp/hdlc
 *	@dev: Device subject to ioctl
 *	@ifr: Interface request block from the user
 *	@cmd: Command that is being issued
 *	
 *	This function handles the ioctls that may be issued by the user
 *	to control the settings of a PPP/HDLC link. It does both busy
 *	and security checks. This function is intended to be wrapped by
 *	callers who wish to add additional ioctl calls of their own.
 */
 
int wp_sppp_do_ioctl(struct net_device *dev, struct ifreq *ifr, int cmd)
{
	struct sppp *sp = (struct sppp *)sppp_of(dev);

	WAN_ASSERT2((!sp),-ENODEV);

	if(dev->flags&IFF_UP)
		return -EBUSY;
		
	if(!capable(CAP_NET_ADMIN))
		return -EPERM;
	
	switch(cmd)
	{
		case SPPPIOCCISCO:
			sp->pp_flags|=PP_CISCO;
			dev->type = ARPHRD_HDLC;
			break;
		case SPPPIOCPPP:
			sp->pp_flags&=~PP_CISCO;
			dev->type = ARPHRD_PPP;
			break;
		case SPPPIOCDEBUG:
			sp->pp_flags&=~PP_DEBUG;
			if(ifr->ifr_flags)
				sp->pp_flags|=PP_DEBUG;
			break;
		case SPPPIOCGFLAGS:
			if(copy_to_user(ifr->ifr_data, &sp->pp_flags, sizeof(sp->pp_flags)))
				return -EFAULT;
			break;
		case SPPPIOCSFLAGS:
			if(copy_from_user(&sp->pp_flags, ifr->ifr_data, sizeof(sp->pp_flags)))
				return -EFAULT;
			break;

		default:
			return -EINVAL;
	}
	return 0;
}

EXPORT_SYMBOL(wp_sppp_do_ioctl);

/**
 *	wp_sppp_attach - attach synchronous PPP/HDLC to a device
 *	@pd:	PPP device to initialise
 *
 *	This initialises the PPP/HDLC support on an interface. At the
 *	time of calling the dev element must point to the network device
 *	that this interface is attached to. The interface should not yet
 *	be registered. 
 */
 
void wp_sppp_attach(struct ppp_device *pd)
{
	struct net_device *dev = pd->dev;
	struct sppp *sp = &pd->sppp;
	unsigned long flags;

	wan_spin_lock_irq(&spppq_lock, &flags);
	/* Initialize keepalive handler. */
	if (! spppq)
	{
		init_timer(&sppp_keepalive_timer);
		sppp_keepalive_timer.expires=jiffies+10*HZ;
		sppp_keepalive_timer.function=sppp_keepalive;
		add_timer(&sppp_keepalive_timer);
	}
	/* Insert new entry into the keepalive list. */
	sp->pp_next = spppq;
	spppq = sp;
	wan_spin_unlock_irq(&spppq_lock, &flags);

	sp->pp_loopcnt = 0;
	sp->pp_alivecnt = 0;

	sp->pp_seq[IDX_LCP] = 0;
	sp->pp_rseq[IDX_LCP] = 0;
	sppp_chap_init(sp);
	sp->pp_auth_flags = 0;
	
	sp->pp_flags = PP_KEEPALIVE|PP_CISCO|debug;/*PP_DEBUG;*/
	sp->lcp.magic = 0;
	sp->lcp.state = LCP_STATE_CLOSED;
	sp->ipcp.state = IPCP_STATE_CLOSED;
	sp->pp_if = dev;

	if (sp->dynamic_ip){
		DEBUG_EVENT("%s: Dynamic IP Addressing Enabled!\n",dev->name);
	}else{
		DEBUG_EVENT("%s: Dynamic IP Addressing Disabled!\n",dev->name);
	}

	if (strlen(sp->hwdevname) > WAN_IFNAME_SZ || 
	    strlen(sp->hwdevname) == 0){
		DEBUG_EVENT("%s: Warning: hwdevname not initialized!\n",
				dev->name);
	}

	sp->task_working=0;
	sp->local_ip=0;
	sp->remote_ip=0;


#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,20))  
	INIT_WORK((&sp->sppp_task),sppp_bh,sp);
#else
	INIT_WORK((&sp->sppp_task),sppp_bh);
#endif
	sp->task_working=0;

	/* 
	 *	Device specific setup. All but interrupt handler and
	 *	hard_start_xmit.
	 */
#ifdef LINUX_FEAT_2624
	dev->header_ops = &sppp_header_ops;
#else
	dev->hard_header = sppp_hard_header;
#endif

	dev->tx_queue_len = 100;
	dev->type = ARPHRD_HDLC;
	dev->addr_len = 0;
	dev->hard_header_len = sizeof(struct ppp_header);
	dev->mtu = PPP_MTU;
	/*
	 *	These 4 are callers but MUST also call sppp_ functions
	 */
	WAN_NETDEV_OPS_IOCTL(dev,wan_netdev_ops,wp_sppp_do_ioctl);

#if 0
	dev->get_stats = NULL;		/* Let the driver override these */
	dev->open = wp_sppp_open;
	dev->stop = wp_sppp_close;
#endif	
	WAN_NETDEV_OPS_MTU(dev,wan_netdev_ops,wp_sppp_change_mtu);	
	dev->flags = IFF_MULTICAST|IFF_POINTOPOINT|IFF_NOARP;
	
#if 0				
	dev_init_buffers(dev);	/* Let the driver do this */
#endif
}

EXPORT_SYMBOL(wp_sppp_attach);

/**
 *	wp_sppp_detach - release PPP resources from a device
 *	@dev:	Network device to release
 *
 *	Stop and free up any PPP/HDLC resources used by this
 *	interface. This must be called before the device is
 *	freed.
 */
 
void wp_sppp_detach (struct net_device *dev)
{
	struct sppp **q, *p, *sp = (struct sppp *)sppp_of(dev);
	unsigned long flags;
	WAN_ASSERT1((!sp));
	
	wan_spin_lock_irq(&spppq_lock, &flags);
	/* Remove the entry from the keepalive list. */
	for (q = &spppq; (p = *q); q = &p->pp_next)
		if (p == sp) {
			*q = p->pp_next;
			break;
		}

	/* Stop keepalive handler. */
	if (! spppq)
		del_timer(&sppp_keepalive_timer);
	sppp_clear_timeout (sp);
	auth_clear_timeout(sp);
	wan_spin_unlock_irq(&spppq_lock, &flags);
}

EXPORT_SYMBOL(wp_sppp_detach);


/*
 * Analyze the LCP Configure-Request options list
 * for the presence of unknown options.
 * If the request contains unknown options, build and
 * send Configure-reject packet, containing only unknown options.
 */
static int
sppp_lcp_conf_parse_options (struct sppp *sp, struct lcp_header *h,
	int len, u32 *magic)
{
	u8 *buf, *r, *p;
	int rlen = 0;

	u8 * packet_length, * chap_algorithm;
	u16 authproto;
	struct net_device *dev = sp->pp_if;

	len -= 4; //sizeof(struct lcp_header);
	buf = r = kmalloc (len, GFP_ATOMIC);
	if (! buf)
		return (0);

	p = (void*) (h+1);
	
	//p[0] is type
	//p[1] is length
	
	for (rlen=0; len>1 && p[1]; len-=p[1], p+=p[1]) {
		switch (*p) {
		case LCP_OPT_MAGIC:
			/* Magic number -- extract. */
			if (len >= 6 && p[1] == 6) {
				*magic = (u32)p[2] << 24 |
					(u32)p[3] << 16 | p[4] << 8 | p[5];
				continue;
			}
			break;
		case LCP_OPT_ASYNC_MAP:
			/* Async control character map -- check to be zero. */
			if (len >= 6 && p[1] == 6 && ! p[2] && ! p[3] &&
			    ! p[4] && ! p[5])
				continue;
			break;
		case LCP_OPT_MRU:
			/* Maximum receive unit -- always OK. */
			continue;

		case LCP_OPT_AUTH_PROTO:
			//request to authenticate
			packet_length = p + 1;
			
			if (*packet_length < 4) {
				printk(KERN_INFO "%s: LCP_OPT_AUTH_PROTO packet is too short (%d)!\n",
					dev->name, *packet_length);
				break;
			}
			
			//got request to authenticate
			if (sp->myauth.proto == 0) {
				// we are not configured to do auth
				printk(KERN_INFO
			       		"%s: Got authentication request, but not configured to authenticate.\n", 
					dev->name);
				break;
			}
		
			authproto  = ntohs(*(unsigned short *)(p + 2));
			//printk(KERN_INFO "authentication protocol : 0x%X\n", authproto);

			if (sp->myauth.proto != authproto) {
				/* not agreed, nak */
				printk(KERN_INFO "%s: Authentication protocols mismatch! (mine : %s , his : %s )\n",
						dev->name,
					       	sppp_proto_name(sp->myauth.proto),
					       	sppp_proto_name(authproto));
				break;
			}

			/*
			 * Remote want us to authenticate, remember this,
			 * so we stay in PHASE_AUTHENTICATE after LCP got
			 * up.
			 */
			sp->pp_flags |= PP_NEEDAUTH;

			//if we configured for PAP -- ack it
			if(authproto == PPP_PAP){

				kfree(buf);
				return PPP_PAP;
			}else if(authproto == PPP_CHAP){

				chap_algorithm = p + 4;

				if(*chap_algorithm == CHAP_MD5){

					kfree(buf);
					return 1;//return ok
				}else{
					printk(KERN_INFO "Got request for unsupported CHAP algorithm : 0x%X.\
							Only MD5 is supported.\n", *chap_algorithm);
					break;
				}
			}else{
				printk(KERN_INFO "Got request for unsupported authentication protocol : 0x%X\n", 
					authproto);
				break;
			}
				
		default:
			/* Others not supported. */
			printk(KERN_INFO "unknown lcp option : 0x%X\n", *p);
			break;
		}
		/* Add the option to rejected list. */
		memcpy(r, p, p[1]);
		r += p[1];
		rlen += p[1];
	}
	if (rlen)
		sppp_cp_send (sp, PPP_LCP, LCP_CONF_REJ, h->ident, rlen, buf);

	kfree(buf);
	return (rlen == 0);
}

static const char *
sppp_proto_name(u_short proto)
{
	static char buf[12];
	switch (proto) {
	case PPP_LCP:	return "lcp";
	case PPP_IPCP:	return "ipcp";
	case PPP_PAP:	return "pap";
	case PPP_CHAP:	return "chap";
	//case PPP_IPV6CP: return "ipv6cp";
	}
	snprintf(buf, sizeof(buf), "proto/0x%x", (unsigned)proto);
	return buf;
}


#ifdef WORDS_BIGENDIAN
void
byteSwap(UWORD32 *buf, unsigned words)
{
	md5byte *p = (md5byte *)buf;

	do {
		*buf++ = (UWORD32)((unsigned)p[3] << 8 | p[2]) << 16 |
			((unsigned)p[1] << 8 | p[0]);
		p += 4;
	} while (--words);
}
#else
#define byteSwap(buf,words)
#endif

/*
 * Start MD5 accumulation.  Set bit count to 0 and buffer to mysterious
 * initialization constants.
 */
void
wp_MD5Init(struct wp_MD5Context *ctx)
{
	ctx->buf[0] = 0x67452301;
	ctx->buf[1] = 0xefcdab89;
	ctx->buf[2] = 0x98badcfe;
	ctx->buf[3] = 0x10325476;

	ctx->bytes[0] = 0;
	ctx->bytes[1] = 0;
}

/*
 * Update context to reflect the concatenation of another buffer full
 * of bytes.
 */
void
wp_MD5Update(struct wp_MD5Context *ctx, md5byte const *buf, unsigned len)
{
	UWORD32 t;

	/* Update byte count */

	t = ctx->bytes[0];
	if ((ctx->bytes[0] = t + len) < t)
		ctx->bytes[1]++;	/* Carry from low to high */

	t = 64 - (t & 0x3f);	/* Space available in ctx->in (at least 1) */
	if (t > len) {
		memcpy((md5byte *)ctx->in + 64 - t, buf, len);
		return;
	}
	/* First chunk is an odd size */
	memcpy((md5byte *)ctx->in + 64 - t, buf, t);
	byteSwap(ctx->in, 16);
	wp_MD5Transform(ctx->buf, ctx->in);
	buf += t;
	len -= t;

	/* Process data in 64-byte chunks */
	while (len >= 64) {
		memcpy(ctx->in, buf, 64);
		byteSwap(ctx->in, 16);
		wp_MD5Transform(ctx->buf, ctx->in);
		buf += 64;
		len -= 64;
	}

	/* Handle any remaining bytes of data. */
	memcpy(ctx->in, buf, len);
}

/*
 * Final wrapup - pad to 64-byte boundary with the bit pattern 
 * 1 0* (64-bit count of bits processed, MSB-first)
 */
void
wp_MD5Final(md5byte digest[16], struct wp_MD5Context *ctx)
{
	int count = ctx->bytes[0] & 0x3f;	/* Number of bytes in ctx->in */
	md5byte *p = (md5byte *)ctx->in + count;

	/* Set the first char of padding to 0x80.  There is always room. */
	*p++ = 0x80;

	/* Bytes of padding needed to make 56 bytes (-8..55) */
	count = 56 - 1 - count;

	if (count < 0) {	/* Padding forces an extra block */
		memset(p, 0, count + 8);
		byteSwap(ctx->in, 16);
		wp_MD5Transform(ctx->buf, ctx->in);
		p = (md5byte *)ctx->in;
		count = 56;
	}
	memset(p, 0, count);
	byteSwap(ctx->in, 14);

	/* Append length in bits and transform */
	ctx->in[14] = ctx->bytes[0] << 3;
	ctx->in[15] = ctx->bytes[1] << 3 | ctx->bytes[0] >> 29;
	wp_MD5Transform(ctx->buf, ctx->in);

	byteSwap(ctx->buf, 4);
	memcpy(digest, ctx->buf, 16);
	memset(ctx, 0, sizeof(ctx));	/* In case it's sensitive */
}

#ifndef ASM_MD5

/* The four core functions - F1 is optimized somewhat */

/* #define F1(x, y, z) (x & y | ~x & z) */
#define F1(x, y, z) (z ^ (x & (y ^ z)))
#define F2(x, y, z) F1(z, x, y)
#define F3(x, y, z) (x ^ y ^ z)
#define F4(x, y, z) (y ^ (x | ~z))

/* This is the central step in the MD5 algorithm. */
#define wp_MD5STEP(f,w,x,y,z,in,s) \
	 (w += f(x,y,z) + in, w = (w<<s | w>>(32-s)) + x)

/*
 * The core of the MD5 algorithm, this alters an existing MD5 hash to
 * reflect the addition of 16 longwords of new data.  MD5Update blocks
 * the data and converts bytes into longwords for this routine.
 */
void
wp_MD5Transform(UWORD32 buf[4], UWORD32 const in[16])
{
	register UWORD32 a, b, c, d;

	a = buf[0];
	b = buf[1];
	c = buf[2];
	d = buf[3];

	wp_MD5STEP(F1, a, b, c, d, in[0] + 0xd76aa478, 7);
	wp_MD5STEP(F1, d, a, b, c, in[1] + 0xe8c7b756, 12);
	wp_MD5STEP(F1, c, d, a, b, in[2] + 0x242070db, 17);
	wp_MD5STEP(F1, b, c, d, a, in[3] + 0xc1bdceee, 22);
	wp_MD5STEP(F1, a, b, c, d, in[4] + 0xf57c0faf, 7);
	wp_MD5STEP(F1, d, a, b, c, in[5] + 0x4787c62a, 12);
	wp_MD5STEP(F1, c, d, a, b, in[6] + 0xa8304613, 17);
	wp_MD5STEP(F1, b, c, d, a, in[7] + 0xfd469501, 22);
	wp_MD5STEP(F1, a, b, c, d, in[8] + 0x698098d8, 7);
	wp_MD5STEP(F1, d, a, b, c, in[9] + 0x8b44f7af, 12);
	wp_MD5STEP(F1, c, d, a, b, in[10] + 0xffff5bb1, 17);
	wp_MD5STEP(F1, b, c, d, a, in[11] + 0x895cd7be, 22);
	wp_MD5STEP(F1, a, b, c, d, in[12] + 0x6b901122, 7);
	wp_MD5STEP(F1, d, a, b, c, in[13] + 0xfd987193, 12);
	wp_MD5STEP(F1, c, d, a, b, in[14] + 0xa679438e, 17);
	wp_MD5STEP(F1, b, c, d, a, in[15] + 0x49b40821, 22);

	wp_MD5STEP(F2, a, b, c, d, in[1] + 0xf61e2562, 5);
	wp_MD5STEP(F2, d, a, b, c, in[6] + 0xc040b340, 9);
	wp_MD5STEP(F2, c, d, a, b, in[11] + 0x265e5a51, 14);
	wp_MD5STEP(F2, b, c, d, a, in[0] + 0xe9b6c7aa, 20);
	wp_MD5STEP(F2, a, b, c, d, in[5] + 0xd62f105d, 5);
	wp_MD5STEP(F2, d, a, b, c, in[10] + 0x02441453, 9);
	wp_MD5STEP(F2, c, d, a, b, in[15] + 0xd8a1e681, 14);
	wp_MD5STEP(F2, b, c, d, a, in[4] + 0xe7d3fbc8, 20);
	wp_MD5STEP(F2, a, b, c, d, in[9] + 0x21e1cde6, 5);
	wp_MD5STEP(F2, d, a, b, c, in[14] + 0xc33707d6, 9);
	wp_MD5STEP(F2, c, d, a, b, in[3] + 0xf4d50d87, 14);
	wp_MD5STEP(F2, b, c, d, a, in[8] + 0x455a14ed, 20);
	wp_MD5STEP(F2, a, b, c, d, in[13] + 0xa9e3e905, 5);
	wp_MD5STEP(F2, d, a, b, c, in[2] + 0xfcefa3f8, 9);
	wp_MD5STEP(F2, c, d, a, b, in[7] + 0x676f02d9, 14);
	wp_MD5STEP(F2, b, c, d, a, in[12] + 0x8d2a4c8a, 20);

	wp_MD5STEP(F3, a, b, c, d, in[5] + 0xfffa3942, 4);
	wp_MD5STEP(F3, d, a, b, c, in[8] + 0x8771f681, 11);
	wp_MD5STEP(F3, c, d, a, b, in[11] + 0x6d9d6122, 16);
	wp_MD5STEP(F3, b, c, d, a, in[14] + 0xfde5380c, 23);
	wp_MD5STEP(F3, a, b, c, d, in[1] + 0xa4beea44, 4);
	wp_MD5STEP(F3, d, a, b, c, in[4] + 0x4bdecfa9, 11);
	wp_MD5STEP(F3, c, d, a, b, in[7] + 0xf6bb4b60, 16);
	wp_MD5STEP(F3, b, c, d, a, in[10] + 0xbebfbc70, 23);
	wp_MD5STEP(F3, a, b, c, d, in[13] + 0x289b7ec6, 4);
	wp_MD5STEP(F3, d, a, b, c, in[0] + 0xeaa127fa, 11);
	wp_MD5STEP(F3, c, d, a, b, in[3] + 0xd4ef3085, 16);
	wp_MD5STEP(F3, b, c, d, a, in[6] + 0x04881d05, 23);
	wp_MD5STEP(F3, a, b, c, d, in[9] + 0xd9d4d039, 4);
	wp_MD5STEP(F3, d, a, b, c, in[12] + 0xe6db99e5, 11);
	wp_MD5STEP(F3, c, d, a, b, in[15] + 0x1fa27cf8, 16);
	wp_MD5STEP(F3, b, c, d, a, in[2] + 0xc4ac5665, 23);

	wp_MD5STEP(F4, a, b, c, d, in[0] + 0xf4292244, 6);
	wp_MD5STEP(F4, d, a, b, c, in[7] + 0x432aff97, 10);
	wp_MD5STEP(F4, c, d, a, b, in[14] + 0xab9423a7, 15);
	wp_MD5STEP(F4, b, c, d, a, in[5] + 0xfc93a039, 21);
	wp_MD5STEP(F4, a, b, c, d, in[12] + 0x655b59c3, 6);
	wp_MD5STEP(F4, d, a, b, c, in[3] + 0x8f0ccc92, 10);
	wp_MD5STEP(F4, c, d, a, b, in[10] + 0xffeff47d, 15);
	wp_MD5STEP(F4, b, c, d, a, in[1] + 0x85845dd1, 21);
	wp_MD5STEP(F4, a, b, c, d, in[8] + 0x6fa87e4f, 6);
	wp_MD5STEP(F4, d, a, b, c, in[15] + 0xfe2ce6e0, 10);
	wp_MD5STEP(F4, c, d, a, b, in[6] + 0xa3014314, 15);
	wp_MD5STEP(F4, b, c, d, a, in[13] + 0x4e0811a1, 21);
	wp_MD5STEP(F4, a, b, c, d, in[4] + 0xf7537e82, 6);
	wp_MD5STEP(F4, d, a, b, c, in[11] + 0xbd3af235, 10);
	wp_MD5STEP(F4, c, d, a, b, in[2] + 0x2ad7d2bb, 15);
	wp_MD5STEP(F4, b, c, d, a, in[9] + 0xeb86d391, 21);

	buf[0] += a;
	buf[1] += b;
	buf[2] += c;
	buf[3] += d;
}

#endif

static void sppp_ipcp_input (struct sppp *sp, struct sk_buff *skb)
{
	struct lcp_header *h;
	struct ipcp_header *ipcp_h;
	struct net_device *dev = sp->pp_if;
	int len = skb->len;
	unsigned int remote_ip, local_ip;

	if (len < 4) 
	{
		if (sp->pp_flags & PP_DEBUG)
			printk (KERN_WARNING "%s: invalid ipcp packet length: %d bytes\n",
				dev->name,  len);
		return;
	}
	h = (struct lcp_header *)skb->data;
	skb_pull(skb,sizeof(struct lcp_header));
	if (sp->pp_flags & PP_DEBUG) {
		char state = '?';
		switch (sp->ipcp.state) {
		case IPCP_STATE_CLOSED:   state = 'C'; break;
		case IPCP_STATE_ACK_RCVD: state = 'R'; break;
		case IPCP_STATE_ACK_SENT: state = 'S'; break;
		case IPCP_STATE_OPENED:   state = 'O'; break;
		}
		
		printk (KERN_WARNING "%s: ipcp input(%c): %d bytes <%s id=%xh len=%xh",
			dev->name,  
			state,
			len,
			sppp_ipcp_type_name (h->type), h->ident, ntohs (h->len));

		if (len > 4)
			sppp_print_bytes ((u8*) (h+1), len-4);

		printk (">\n");
	}

	if (len > ntohs (h->len))
		len = ntohs (h->len);

	ipcp_h = (struct ipcp_header*)(h+1);

	switch (h->type) {
	default:
		/* Unknown packet type -- send Code-Reject packet. */
		sppp_cp_send (sp, PPP_IPCP, IPCP_CODE_REJ, ++sp->pp_seq[IDX_LCP], len, h);
		break;

	case IPCP_CONF_REQ:

		if (len < 4) {
			if (sp->pp_flags & PP_DEBUG)
				printk (KERN_WARNING "%s: invalid ipcp configure request packet length: %d bytes\n",
					dev->name, len);
			return;
		}
		if (len > 4) {

			if (!sp->dynamic_ip){

				remote_ip=wan_get_ip_address(sp->pp_if,WAN_POINTOPOINT_IP);	

				if (remote_ip == *(unsigned long*)&ipcp_h->data[0]){
					sppp_cp_send (sp, PPP_IPCP, LCP_CONF_ACK, h->ident,
					len-4, h+1);

					sppp_clear_timeout (sp);
					
					remote_ip=*(unsigned long*)&ipcp_h->data[0];
					DEBUG_EVENT("%s: IPCP Static: P-to-P verified: %u.%u.%u.%u\n",
							sp->pp_if->name,
							NIPQUAD(remote_ip));

				} else {

					remote_ip = *(unsigned long*)&ipcp_h->data[0];

					sp->local_ip=wan_get_ip_address(sp->pp_if,WAN_LOCAL_IP);
					sp->remote_ip=wan_get_ip_address(sp->pp_if,WAN_POINTOPOINT_IP);	
					DEBUG_EVENT("%s: IPCP Static Refusing P-to-P %u.%u.%u.%u: Dynamic ip disabled!\n",
							sp->pp_if->name, NIPQUAD(remote_ip));

					DEBUG_EVENT("%s: IPCP Current Cfg: Local %u.%u.%u.%u P-to-P %u.%u.%u.%u\n",
							sp->pp_if->name,
							NIPQUAD(sp->local_ip),
							NIPQUAD(sp->remote_ip));

					if (++sp->dynamic_failures > 10){
				
						sppp_cp_send (sp, PPP_IPCP, IPCP_TERM_REQ, h->ident, 0, 0);
						sp->ipcp.state = IPCP_STATE_CLOSED;
						sppp_set_timeout (sp, 5);
						sp->dynamic_failures=0;
						
					}else{
					
						sppp_cp_send (sp, PPP_IPCP, LCP_CONF_REJ, h->ident,
							len-4, h+1);

						switch (sp->ipcp.state) {
						case IPCP_STATE_OPENED:
							/* Initiate renegotiation. */
							sppp_ipcp_open (sp);
							/* fall through... */
						case IPCP_STATE_ACK_SENT:
							/* Go to closed state. */
							sp->ipcp.state = IPCP_STATE_CLOSED;
						}
					}
				}

			} else {
				sp->remote_ip = *(unsigned long*)&ipcp_h->data[0];
				sppp_cp_send (sp, PPP_IPCP, LCP_CONF_ACK, h->ident,
				len-4, h+1);

				sppp_clear_timeout (sp);
			}
				
		} else {
			/* Send Configure-Ack packet. */
			sppp_cp_send (sp, PPP_IPCP, IPCP_CONF_ACK, h->ident,
				0, 0);

			/* Change the state. */
			if (sp->ipcp.state == IPCP_STATE_ACK_RCVD)
				sp->ipcp.state = IPCP_STATE_OPENED;
			else
				sp->ipcp.state = IPCP_STATE_ACK_SENT;
		}
		break;
		
	case IPCP_CONF_ACK:
		if (h->ident != sp->ipcp.confid)
			break;
		sppp_clear_timeout (sp);
		switch (sp->ipcp.state) {
		case IPCP_STATE_CLOSED:
			sp->ipcp.state = IPCP_STATE_ACK_RCVD;
			sppp_set_timeout (sp, 5);
			break;
		case IPCP_STATE_ACK_SENT:
			sp->ipcp.state = IPCP_STATE_OPENED;

			if (sp->dynamic_ip){
				wan_schedule_task(&sp->sppp_task);
			}
			break;
		}
		break;

	case IPCP_CONF_NAK:

		if (h->ident != sp->ipcp.confid)
			break;

		if (ipcp_h->len >= 6) {

			if (!sp->dynamic_ip) {

				local_ip=wan_get_ip_address(sp->pp_if,WAN_LOCAL_IP);
	
				if (local_ip == *(unsigned long*)&ipcp_h->data[0]) {
					sppp_cp_send (sp, PPP_IPCP, LCP_CONF_REQ, h->ident,
						len-4, h+1);
					
					sp->ipcp.state = IPCP_STATE_ACK_SENT;
		
					local_ip=*(unsigned long*)&ipcp_h->data[0];	
					DEBUG_EVENT("%s: IPCP Static: Local IP verified: %u.%u.%u.%u\n",
							sp->pp_if->name,
							NIPQUAD(local_ip));

					break;
				} else {
					local_ip=*(unsigned long*)&ipcp_h->data[0]; 
					DEBUG_EVENT("%s: IPCP Static: Refusing Local %u.%u.%u.%u: Dynamic ip disabled!\n",
							sp->pp_if->name, NIPQUAD(local_ip));

					DEBUG_EVENT("%s: IPCP Current Cfg: Local %u.%u.%u.%u P-to-P %u.%u.%u.%u\n",
							sp->pp_if->name,
							NIPQUAD(sp->local_ip),
							NIPQUAD(sp->remote_ip));
				
					sppp_cp_send (sp, PPP_IPCP, LCP_CONF_REJ, h->ident,
						len-4, h+1);

					/* Drop down to reject */

				}

			} else {
				sp->local_ip = *(unsigned long*)&ipcp_h->data[0];
				sppp_cp_send (sp, PPP_IPCP, LCP_CONF_REQ, h->ident,
					len-4, h+1);
					
				sp->ipcp.state = IPCP_STATE_ACK_SENT;
				break;
			}
		}
		/* Drop down to reject */
		
	case IPCP_CONF_REJ:
		if (h->ident != sp->ipcp.confid)
			break;
		sppp_clear_timeout (sp);
		/* Initiate renegotiation. */
		sppp_ipcp_open (sp);
		if (sp->ipcp.state != IPCP_STATE_ACK_SENT)
			/* Go to closed state. */
			sp->ipcp.state = IPCP_STATE_CLOSED;
		break;

	case IPCP_TERM_REQ:
		/* Send Terminate-Ack packet. */
		sppp_cp_send (sp, PPP_IPCP, IPCP_TERM_ACK, h->ident, 0, 0);
		/* Go to closed state. */
		sp->ipcp.state = IPCP_STATE_CLOSED;
		/* Initiate renegotiation. */
		sppp_ipcp_open (sp);
		break;
	case IPCP_TERM_ACK:
		/* Ignore for now. */
	case IPCP_CODE_REJ:
		/* Ignore for now. */
		break;
	}
}

static void sppp_lcp_open (struct sppp *sp)
{
	char opt[6];

	if (sp->dynamic_ip){
		sp->local_ip=0;
		sp->remote_ip=0;
		if (wan_get_ip_address(sp->pp_if,WAN_LOCAL_IP)){
			wan_schedule_task(&sp->sppp_task);
		}
	}
	
	if (! sp->lcp.magic)
		sp->lcp.magic = jiffies;
	opt[0] = LCP_OPT_MAGIC;
	opt[1] = sizeof (opt);
	opt[2] = sp->lcp.magic >> 24;
	opt[3] = sp->lcp.magic >> 16;
	opt[4] = sp->lcp.magic >> 8;
	opt[5] = sp->lcp.magic;
	sp->lcp.confid = ++sp->pp_seq[IDX_LCP];
	sppp_cp_send (sp, PPP_LCP, LCP_CONF_REQ, sp->lcp.confid,
		sizeof (opt), &opt);
	sppp_set_timeout (sp, 2);
}

static void sppp_ipcp_open (struct sppp *sp)
{
	if (sp->dynamic_ip){
		unsigned char data[]={3,6,0,0,0,0};
		sp->ipcp.confid = ++sp->pp_seq[IDX_LCP];
		sppp_cp_send (sp, PPP_IPCP, IPCP_CONF_REQ, sp->ipcp.confid, 6, data);
		sppp_set_timeout (sp, 2);
	}else{
		sp->ipcp.confid = ++sp->pp_seq[IDX_LCP];
		sppp_cp_send (sp, PPP_IPCP, IPCP_CONF_REQ, sp->ipcp.confid, 0,0);
		sppp_set_timeout (sp, 2);
	}
}

/*
 * Process PPP control protocol timeouts.
 */
 
static void sppp_cp_timeout (unsigned long arg)
{
	struct sppp *sp = (struct sppp*) arg;
	//unsigned long flags;

	//wan_spin_lock_irq(&spppq_lock, &flags);

	sp->pp_flags &= ~PP_TIMO;
	if (! (sp->pp_if->flags & IFF_UP) || (sp->pp_flags & PP_CISCO)) {
		//wan_spin_unlock_irq(&spppq_lock, &flags);
		return;
	}
	switch (sp->lcp.state) {
	case LCP_STATE_CLOSED:
		/* No ACK for Configure-Request, retry. */
		sppp_lcp_open (sp);
		break;
	case LCP_STATE_ACK_RCVD:
		/* ACK got, but no Configure-Request for peer, retry. */
		sppp_lcp_open (sp);
		sp->lcp.state = LCP_STATE_CLOSED;
		break;
	case LCP_STATE_ACK_SENT:
		/* ACK sent but no ACK for Configure-Request, retry. */
		sppp_lcp_open (sp);
		break;
	case LCP_STATE_OPENED:
		/* LCP is already OK, try IPCP. */
		switch (sp->ipcp.state) {
		case IPCP_STATE_CLOSED:
			/* No ACK for Configure-Request, retry. */
			sppp_ipcp_open (sp);
			break;
		case IPCP_STATE_ACK_RCVD:
			/* ACK got, but no Configure-Request for peer, retry. */
			sppp_ipcp_open (sp);
			sp->ipcp.state = IPCP_STATE_CLOSED;
			break;
		case IPCP_STATE_ACK_SENT:
			/* ACK sent but no ACK for Configure-Request, retry. */
			sppp_ipcp_open (sp);
			break;
		case IPCP_STATE_OPENED:
			/* IPCP is OK. */
			break;
		}
		break;
	}
	//wan_spin_unlock_irq(&spppq_lock, &flags);
}

static char *sppp_lcp_type_name (u8 type)
{
	static char buf [8];
	switch (type) {
	case LCP_CONF_REQ:   return ("conf-req");
	case LCP_CONF_ACK:   return ("conf-ack");
	case LCP_CONF_NAK:   return ("conf-nack");
	case LCP_CONF_REJ:   return ("conf-rej");
	case LCP_TERM_REQ:   return ("term-req");
	case LCP_TERM_ACK:   return ("term-ack");
	case LCP_CODE_REJ:   return ("code-rej");
	case LCP_PROTO_REJ:  return ("proto-rej");
	case LCP_ECHO_REQ:   return ("echo-req");
	case LCP_ECHO_REPLY: return ("echo-reply");
	case LCP_DISC_REQ:   return ("discard-req");
	}
	sprintf (buf, "%xh", type);
	return (buf);
}

static char *sppp_ipcp_type_name (u8 type)
{
	static char buf [8];
	switch (type) {
	case IPCP_CONF_REQ:   return ("conf-req");
	case IPCP_CONF_ACK:   return ("conf-ack");
	case IPCP_CONF_NAK:   return ("conf-nack");
	case IPCP_CONF_REJ:   return ("conf-rej");
	case IPCP_TERM_REQ:   return ("term-req");
	case IPCP_TERM_ACK:   return ("term-ack");
	case IPCP_CODE_REJ:   return ("code-rej");
	}
	sprintf (buf, "%xh", type);
	return (buf);
}

static void sppp_print_bytes (u_char *p, u16 len)
{
	printk (" %x", *p++);
	while (--len > 0)
		printk ("-%x", *p++);
}


#if 0

/**
 *	sppp_rcv -	receive and process a WAN PPP frame
 *	@skb:	The buffer to process
 *	@dev:	The device it arrived on
 *	@p: Unused
 *
 *	Protocol glue. This drives the deferred processing mode the poorer
 *	cards use. This can be called directly by cards that do not have
 *	timing constraints but is normally called from the network layer
 *	after interrupt servicing to process frames queued via netif_rx.
 */

static int sppp_rcv(struct sk_buff *skb, struct net_device *dev, struct packet_type *p)
{
	wp_sppp_input(dev,skb);
	return 0;
}


#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,3,0)
struct packet_type sppp_packet_type = {
	type:	__constant_htons(ETH_P_WAN_PPP),
	func:	sppp_rcv,
};
#else
struct packet_type sppp_packet_type=
{
	0,
	NULL,
	sppp_rcv,
	NULL,
	NULL
};
#endif

#endif

static char fullname[] = "WANPIPE(tm) PPP/Cisco HDLC Protocol";

static int __init sync_ppp_init(void)
{
	if(debug)
		debug=PP_DEBUG;

		DEBUG_EVENT("%s %s.%s %s %s\n",
			fullname, WANPIPE_VERSION, WANPIPE_SUB_VERSION,
			WANPIPE_COPYRIGHT_DATES,WANPIPE_COMPANY);

#if 0
	dev_add_pack(&sppp_packet_type);
#endif
	wan_spin_lock_init(&spppq_lock,"spppq_lock");
	wan_spin_lock_init(&authenticate_lock,"authenticate_lock");
	
	sppp_keepalive_interval=10;
	sppp_max_keepalive_count=MAXALIVECNT;

#if defined(CONFIG_PROC_FS) && LINUX_VERSION_CODE < KERNEL_VERSION(2,5,0)
	sppp_proc_init();
#endif
	return 0;
}

static void __exit sync_ppp_cleanup(void)
{
#if defined(CONFIG_PROC_FS) &&  LINUX_VERSION_CODE < KERNEL_VERSION(2,5,0)
	sppp_proc_cleanup();
#endif
#if 0
	dev_remove_pack(&sppp_packet_type);
#endif
}

module_init(sync_ppp_init);
module_exit(sync_ppp_cleanup);

MODULE_AUTHOR ("Nenad Corbic <ncorbic@sangoma.com>");
MODULE_DESCRIPTION ("Sangoma WANPIPE: WAN PPP Driver");
MODULE_LICENSE("GPL");

/* PROC FILE SYSTEM ADDITION BY NENAD CORBIC */

#if defined(CONFIG_PROC_FS) &&  LINUX_VERSION_CODE < KERNEL_VERSION(2,5,0)
 
#ifndef	min
#define min(a,b) (((a)<(b))?(a):(b))
#endif
#ifndef	max
#define max(a,b) (((a)>(b))?(a):(b))
#endif
#define PROC_BUFSZ 4000
#define MAX_TOKENS 32

static ssize_t router_proc_read(struct file* file, char* buf, size_t count,
				loff_t *ppos)
{
	struct inode *inode = file->f_dentry->d_inode;
	struct proc_dir_entry* dent;
	char* page;
	int pos, offs, len;

	if (count <= 0)
		return 0;
		
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,18)
        dent = inode->i_private;
#else
        dent = inode->u.generic_ip;
#endif

	if ((dent == NULL) || (dent->get_info == NULL)){
		printk(KERN_INFO "NO DENT\n");
		return 0;
	}
		
	page = kmalloc(PROC_BUFSZ, GFP_KERNEL);
	if (page == NULL)
		return -ENOBUFS;
	
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,3,0)
	pos = dent->get_info(page, dent->data, 0, 0);
#else
	pos = dent->get_info(page, dent->data, 0, 0, 0);
#endif
	offs = file->f_pos;
	if (offs < pos) {
		len = wp_min(pos - offs, count);
		if(copy_to_user(buf, (page + offs), len))
			return -EFAULT;
		file->f_pos += len;
	}else{
		len = 0;
	}
	kfree(page);
	return len;
}

/*============================================================================
 * Strip leading and trailing spaces off the string str.
 */
char* str_strip (char* str, char* s)
{
	char* eos = str + strlen(str);		/* -> end of string */

	while (*str && strchr(s, *str))
		++str;				/* strip leading spaces */
	while ((eos > str) && strchr(s, *(eos - 1)))
		--eos;				/* strip trailing spaces */
	*eos = '\0';
	return str;
}

/*============================================================================
 * Tokenize string.
 *	Parse a string of the following syntax:
 *		<tag>=<arg1>,<arg2>,...
 *	and fill array of tokens with pointers to string elements.
 *
 *	Return number of tokens.
 */
int tokenize (char* str, char **tokens, char *arg1, char *arg2, char *arg3)
{
	int cnt = 0;

	tokens[0] = strsep(&str, arg1);
	while (tokens[cnt] && (cnt < MAX_TOKENS - 1)) {
		tokens[cnt] = str_strip(tokens[cnt], arg2);
		tokens[++cnt] = strsep(&str, arg3);
	}
	return cnt;
}

static ssize_t router_proc_write(struct file *file, const char *buf, size_t count, loff_t *ppos)
{
	struct inode *inode = file->f_dentry->d_inode;
	struct proc_dir_entry* dent;
	char* page;
	int toknum;
	char* token[MAX_TOKENS];
	struct sppp *sp;	
	struct net_device *dev;
	unsigned long flags;
	char msg[100];
	char print_msg=0;
	
	if (count <= 0 || count > PROC_BUFSZ)
		return -EIO;
		
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,18)
	        dent = inode->i_private;
#else
	        dent = inode->u.generic_ip;
#endif

	if ((dent == NULL) || (dent->get_info == NULL))
		return -EIO;

	page = kmalloc(count, GFP_KERNEL);
	if (page == NULL)
		return -ENOBUFS;

	if (copy_from_user(page,buf,count)){
		kfree(page);
		return -EFAULT;	
	}

	toknum = tokenize(page, token, " ", " \t\n", " ");
	if (toknum < 2){
		kfree(page);
		return -EINVAL;
	}

	wan_spin_lock_irq(&spppq_lock, &flags);
	for (sp=spppq; sp; sp=sp->pp_next){
		dev = sp->pp_if;
		if (!strcmp(dev->name,token[0]) && toknum == 3){
			if (!strcmp(token[1], "debug")){

				if (!strcmp(token[2],"on")){
					sprintf(msg,"%s: SyncPPP debug: on\n",dev->name);
					print_msg=1;
					sp->pp_flags |= PP_DEBUG;
				}else{
					sprintf(msg,"%s: SyncPPP debug: off\n",dev->name);
					print_msg=1;
					sp->pp_flags &= ~PP_DEBUG;
				}
			}
		}
		if (!strcmp(token[0], "keepalive_interval") && toknum == 2){
			unsigned int interval=sppp_keepalive_interval;
			if ((interval=(unsigned)simple_strtoul(token[1],NULL,10)) && 
			     interval > 0 && interval < 101){
				if (interval != sppp_keepalive_interval){
					sprintf(msg,"SyncPPP keepalive interval: %i\n",interval);
					print_msg=1;
					sppp_keepalive_interval=interval;
				}
			}
		}
		if (!strcmp(token[0], "max_keepalive_count") && toknum == 2){
			unsigned int kcount=sppp_max_keepalive_count;
			if ((kcount=(unsigned)simple_strtoul(token[1],NULL,10)) && 
			     kcount > 0 && kcount < 101){
				if (kcount != sppp_max_keepalive_count){
					print_msg=1;
					sprintf(msg,"SyncPPP max keepalive count: %i\n",kcount);
					sppp_max_keepalive_count=kcount;
				}
			}
		}
	}
	wan_spin_unlock_irq(&spppq_lock, &flags);
	
	if (print_msg){
		printk(KERN_INFO "%s",msg);
	}
	
	kfree(page);
	return count;
}



static int router_proc_perms (struct inode* inode, int op)
{
	int mode = inode->i_mode;
	if (!current->euid){
		mode >>= 6;
	}else if (in_egroup_p(0)){
		mode >>= 3;
	}
	
	if ((mode & op & 0007) == op)
		return 0;

	return -EACCES;
}

static char proc_name[]	= "syncppp";

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,3,0)
static struct file_operations router_fops =
{
	read: router_proc_read,	
	write: router_proc_write,
};

static struct inode_operations router_inode =
{
	permission: router_proc_perms
};
#else
static struct file_operations router_fops =
{
	NULL,			/* lseek   */
	router_proc_read,	/* read	   */
	router_proc_write,	/* write   */
	NULL,			/* readdir */
	NULL,			/* select  */
	NULL,			/* ioctl   */
	NULL,			/* mmap	   */
	NULL,			/* no special open code	   */
	NULL,			/* flush */
	NULL,			/* no special release code */
	NULL			/* can't fsync */
};

static struct inode_operations router_inode =
{
	&router_fops,
	NULL,			/* create */
	NULL,			/* lookup */
	NULL,			/* link */
	NULL,			/* unlink */
	NULL,			/* symlink */
	NULL,			/* mkdir */
	NULL,			/* rmdir */
	NULL,			/* mknod */
	NULL,			/* rename */
	NULL,			/* follow link */
	NULL,			/* readlink */
	NULL,			/* readpage */
	NULL,			/* writepage */
	NULL,			/* bmap */
	NULL,			/* truncate */
	router_proc_perms
};
#endif

extern struct proc_dir_entry *proc_router;

int sppp_proc_init (void)
{
	struct proc_dir_entry *p;

	if (!proc_router){
		return -ENODEV;
	}
	
	p = create_proc_entry(proc_name,0644,proc_router);
	if (!p)
		return -ENOMEM;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,3,0)
	p->proc_fops = &router_fops;
	p->proc_iops = &router_inode;
#else
	p->ops = &router_inode;
	p->nlink = 1;
#endif
	p->get_info = status_get_info;
	
	return 0;
}
void sppp_proc_cleanup (void)
{
	remove_proc_entry(proc_name, proc_router);
}


static char stat_hdr[] = 
        "----------------------------------------------------------------------------\n"
	"|Interface Name| Protocol | Debug | LCP State   | ICMP State  | Link State |\n"
	"----------------------------------------------------------------------------\n";

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,3,0)
static int status_get_info(char* buf, char** start, off_t offs, int len)
#else
static int status_get_info(char* buf, char** start, off_t offs, int len,int dummy)
#endif
{
	int cnt = 0;
	struct sppp *sp;	
	struct net_device *dev;
	
	strcpy(&buf[cnt], stat_hdr);
	cnt += sizeof(stat_hdr) - 1;

	for (sp=spppq; sp; sp=sp->pp_next){
		dev = sp->pp_if;
		cnt += sprintf(&buf[cnt],
			"| %-13s| %-9s| %-6s| %-12s| %-12s| %-10s |\n",
			dev->name, 
			(sp->pp_flags & PP_CISCO) ? "CHDLC" : "PPP",
			(sp->pp_flags & PP_DEBUG) ? "on" : "off",
			decode_lcp_state(sp->lcp.state),
			decode_ipcp_state(sp->ipcp.state),
			sp->pp_link_state ? "up" : "down");
	}

	cnt += sprintf(&buf[cnt], "----------------------------------------------------------------------------\n");
	cnt += sprintf(&buf[cnt], "\nPPP Config: keepalive_interval: %u | max_keepalive_count: %u |\n\n",
			sppp_keepalive_interval,sppp_max_keepalive_count);
	return cnt;
}


static char *decode_lcp_state(int state)
{
	switch (state){

		case LCP_STATE_CLOSED:        	/* LCP state: closed (conf-req sent) */
			return "closed";
		case LCP_STATE_ACK_RCVD:	/* LCP state: conf-ack received */
			return "conf-ack rx";
		case LCP_STATE_ACK_SENT:      	/* LCP state: conf-ack sent */
			return "conf-ack tx";
		case LCP_STATE_OPENED:		/* LCP state: opened */
			return "opened";
	}
	return "invalid";
}

static char *decode_ipcp_state(int state)
{
	switch (state){

		case IPCP_STATE_CLOSED:      	/* IPCP state: closed (conf-req sent) */
			return "closed";
		case IPCP_STATE_ACK_RCVD:	/* IPCP state: conf-ack received */
			return "conf-ack rx";
		case IPCP_STATE_ACK_SENT:	/* IPCP state: conf-ack sent */
			return "conf-ack tx";
		case IPCP_STATE_OPENED:	 	/* IPCP state: opened */
			return "opened";
	}
	return "invalid";
}

#endif


#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,20))  
static void sppp_bh (void *sp_ptr)
#else
static void sppp_bh (struct work_struct *work)
#endif 
{
	struct net_device *dev;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,20))   
        struct sppp *sp = container_of(work, struct sppp, sppp_task);
	dev=sp->pp_if;
	if (!dev || !sp->dynamic_ip) {
		return;
	}	
#else     
	struct sppp *sp=sp_ptr;
	dev=sp->pp_if;

	if (!sp->dynamic_ip || !sp->pp_if)
		return;
#endif

	if (test_and_set_bit(0,&sp->task_working)){
		DEBUG_EVENT("%s: Critical in sppp bh!\n",dev->name);
		return;
	}

	if (sp->ipcp.state == IPCP_STATE_OPENED){

		int err=wan_set_ip_address(sp->pp_if,WAN_LOCAL_IP,sp->local_ip);
		if (!err){
			DEBUG_EVENT("%s: Local IP Address %u.%u.%u.%u\n",
					dev->name,NIPQUAD(sp->local_ip));
		}else{
			DEBUG_EVENT("%s: Failed to set Local IP Address %u.%u.%u.%u: Rc=%i\n",
					dev->name,NIPQUAD(sp->local_ip),err);
		}
		err=wan_set_ip_address(sp->pp_if,WAN_POINTOPOINT_IP,sp->remote_ip);
		if (!err){
			DEBUG_EVENT("%s: P-to-P IP Address %u.%u.%u.%u\n",
					dev->name,NIPQUAD(sp->remote_ip));
		}else{
			DEBUG_EVENT("%s: Failed to set P-to-P IP Address %u.%u.%u.%u: Rc=%i\n",
					dev->name,NIPQUAD(sp->remote_ip),err);
		}

		if (sp->gateway){
			if (dev->flags & IFF_UP){
				wan_add_gateway(dev);
			}else{
				DEBUG_EVENT("%s: Failed to add gateway dev not up!\n",
						dev->name);
			}
		}

		wan_run_wanrouter(sp->hwdevname,dev->name,"script");

	}else{
		sp->local_ip=0;
		sp->remote_ip=0;
		DEBUG_EVENT("%s: Local IP Address %u.%u.%u.%u\n",
					dev->name,NIPQUAD(sp->local_ip));
		if (wan_get_ip_address(sp->pp_if,WAN_LOCAL_IP)){
			DEBUG_TX("%s: Local IP Address Exists Removing\n",
					dev->name);
			wan_set_ip_address(sp->pp_if,WAN_LOCAL_IP,sp->local_ip);
		}	
		DEBUG_TX("%s: END Local IP Address %u.%u.%u.%u\n",
					dev->name,NIPQUAD(sp->local_ip));
	}

	clear_bit(0,&sp->task_working);
}
