/*
 * Defines for synchronous PPP/Cisco link level subroutines.
 *
 * Copyright (C) 1994 Cronyx Ltd.
 * Author: Serge Vakulenko, <vak@zebub.msk.su>
 *
 * This software is distributed with NO WARRANTIES, not even the implied
 * warranties for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * Authors grant any other persons or organizations permission to use
 * or modify this software as long as this message is kept with the software,
 * all derivative works or modified versions.
 *
 * Version 1.7, Wed Jun  7 22:12:02 MSD 1995
 * 
 * Version 2.1, Wed Mar 26 10:03:00 EDT 2003
 *
 *
 */

/*
 *
 * Changes: v2.1 by David Rokhvarg <davidr@sangoma.com>
 *  
 * Changed for use by Sangoma WANPIPE driver. Added MD5 algorithm
 * declarations.
 * 	
 */

#ifndef _SYNCPPP_H_
#define _SYNCPPP_H_ 1

#ifdef __KERNEL__

#define PPP_ALLSTATIONS 0xff            /* All-Stations broadcast address */
#define PPP_UI          0x03            /* Unnumbered Information */
#define PPP_IP          0x0021          /* Internet Protocol */
#define PPP_ISO         0x0023          /* ISO OSI Protocol */
#define PPP_XNS         0x0025          /* Xerox NS Protocol */
#define PPP_IPX         0x002b          /* Novell IPX Protocol */
#define PPP_LCP         0xc021          /* Link Control Protocol */
#define PPP_IPCP        0x8021          /* Internet Protocol Control Protocol */
#define PPP_PAP		0xc023		/* Password Authentication Protocol */
#define PPP_CHAP	0xc223		/* Challenge-Handshake Auth Protocol */

struct slcp {
	u16	state;          /* state machine */
	u32	magic;          /* local magic number */
	u_char	echoid;         /* id of last keepalive echo request */
	u_char	confid;         /* id of last configuration request */

	u32	opts;		/* LCP options to send (bitfield) */
	u32	mru;		/* our max receive unit */
	u32	their_mru;	/* their max receive unit */
	u32	protos;		/* bitmask of protos that are started */
	/* restart max values, see RFC 1661 */
	int	timeout;
	int	max_terminate;
	int	max_configure;
	int	max_failure;
};

struct sipcp {
	u16	state;          /* state machine */
	u_char  confid;         /* id of last configuration request */
};

#define IDX_LCP 0		/* idx into state table */
#define IDX_IPCP 1		/* idx into state table */
#define IDX_IPV6CP 2		/* idx into state table */

#define AUTHNAMELEN	64
#define AUTHKEYLEN	16

struct sauth {
	u_short	proto;			/* authentication protocol to use */
	u_short	flags;
#define AUTHFLAG_NOCALLOUT	1	/* do not require authentication on */
					/* callouts */
#define AUTHFLAG_NORECHALLENGE	2	/* do not re-challenge CHAP */
	u_char	name[AUTHNAMELEN];	/* system identification name */
	u_char	secret[AUTHKEYLEN];	/* secret password */
	u_char	challenge[AUTHKEYLEN];	/* random challenge */
};

#define IDX_PAP		3
#define IDX_CHAP	4

#define IDX_COUNT (IDX_CHAP + 1) /* bump this when adding cp's! */

/*
 * Don't change the order of this.  Ordering the phases this way allows
 * for a comparision of ``pp_phase >= PHASE_AUTHENTICATE'' in order to
 * know whether LCP is up.
 */
enum ppp_phase {
	PHASE_DEAD, PHASE_ESTABLISH, PHASE_TERMINATE,
	PHASE_AUTHENTICATE, PHASE_NETWORK
};

#define PP_MTU          1500    /* default/minimal MRU */
#define PP_MAX_MRU	2048	/* maximal MRU we want to negotiate */

/*
 * This is a cut down struct sppp (see below) that can easily be
 * exported to/ imported from userland without the need to include
 * dozens of kernel-internal header files.  It is used by the
 * SPPPIO[GS]DEFS ioctl commands below.
 */
struct sppp_parms {
	enum ppp_phase pp_phase;	/* phase we're currently in */
	int	enable_vj;		/* VJ header compression enabled */
	int	enable_ipv6;		/*
					 * Enable IPv6 negotiations -- only
					 * needed since each IPv4 i/f auto-
					 * matically gets an IPv6 address
					 * assigned, so we can't use this as
					 * a decision.
					 */
	struct slcp lcp;		/* LCP params */
	struct sipcp ipcp;		/* IPCP params */
	struct sipcp ipv6cp;		/* IPv6CP params */
	struct sauth myauth;		/* auth params, i'm peer */
	struct sauth hisauth;		/* auth params, i'm authenticator */
};

/*
 * Definitions to pass struct sppp_parms data down into the kernel
 * using the SIOC[SG]IFGENERIC ioctl interface.
 *
 * In order to use this, create a struct spppreq, fill in the cmd
 * field with SPPPIOGDEFS, and put the address of this structure into
 * the ifr_data portion of a struct ifreq.  Pass this struct to a
 * SIOCGIFGENERIC ioctl.  Then replace the cmd field by SPPPIOSDEFS,
 * modify the defs field as desired, and pass the struct ifreq now
 * to a SIOCSIFGENERIC ioctl.
 */

#define SPPPIOGDEFS  ((caddr_t)(('S' << 24) + (1 << 16) +\
	sizeof(struct sppp_parms)))
#define SPPPIOSDEFS  ((caddr_t)(('S' << 24) + (2 << 16) +\
	sizeof(struct sppp_parms)))

struct spppreq {
	int	cmd;
	struct sppp_parms defs;
};

/* bits for pp_flags */
#define PP_KEEPALIVE    0x01    /* use keepalive protocol */
				/* 0x04 was PP_TIMO */
#define PP_CALLIN	0x08	/* we are being called */
#define PP_NEEDAUTH	0x10	/* remote requested authentication */

struct sppp 
{
	struct sppp *	pp_next;	/* next interface in keepalive list */
	u32		pp_flags;	/* use Cisco protocol instead of PPP */
	u16		pp_alivecnt;	/* keepalive packets counter */
	u16		pp_loopcnt;	/* loopback detection counter */
	//u32		pp_seq;		/* local sequence number */
	//u32		pp_rseq;	/* remote sequence number */
	struct slcp	lcp;		/* LCP params */
	struct sipcp	ipcp;		/* IPCP params */
	u32		ibytes,obytes;	/* Bytes in/out */
	u32		ipkts,opkts;	/* Packets in/out */
	struct timer_list	pp_timer;
	struct net_device	*pp_if;
	char		pp_link_state;	/* Link status */
	
	u32  pp_seq[IDX_COUNT];      /* local sequence number */
        u32  pp_rseq[IDX_COUNT];     /* remote sequence number */
        enum ppp_phase pp_phase;        /* phase we're currently in */
        int     state[IDX_COUNT];       /* state machine */
        u_char  confid[IDX_COUNT];      /* id of last configuration request */
        int     rst_counter[IDX_COUNT]; /* restart counter */
        int     fail_counter[IDX_COUNT]; /* negotiation failure counter */
        int     confflags;      /* administrative configuration flags */
#define CONF_ENABLE_VJ    0x01  /* VJ header compression enabled */
#define CONF_ENABLE_IPV6  0x02  /* IPv6 administratively enabled */
        time_t  pp_last_recv;   /* time last packet has been received */
        time_t  pp_last_sent;   /* time last packet has been sent */

	struct sauth myauth;		/* auth params, i'm peer */
	struct sauth hisauth;		/* auth params, i'm authenticator */
	struct timer_list	pp_auth_timer;
	u32	pp_auth_flags;

	unsigned char dynamic_ip;
	unsigned long local_ip;
	unsigned long remote_ip;
	unsigned char gateway;

	struct tq_struct sppp_task;
	unsigned long task_working;
	int	dynamic_failures;

	unsigned char hwdevname[WAN_IFNAME_SZ+1];
};

struct ppp_device
{	
	struct net_device *dev;	/* Network device pointer */
	struct sppp sppp;	/* Synchronous PPP */
};

#include <linux/if_wanpipe_common.h>

static inline struct sppp *sppp_of(netdevice_t *dev)
{
	wanpipe_common_t *chan=wan_netif_priv(dev);
	struct ppp_device *pppdev=(struct ppp_device *)chan->prot_ptr;
	return &pppdev->sppp;
}

#if 0
#define sppp_of(dev)	\
	    (& \
	      ((struct ppp_device *) \
	         (\
		  ((wanpipe_common_t *)\
			   ((dev)->priv)\
	           )->prot_ptr\
	       )->sppp\
              )
#endif

#define PP_KEEPALIVE    0x01    /* use keepalive protocol */
#define PP_CISCO        0x02    /* use Cisco protocol instead of PPP */
#define PP_TIMO         0x04    /* cp_timeout routine active */
#define PP_DEBUG	0x08
#define PP_TIMO1        0x20    /* one of the authentication timeout routines is active */
	      
#define PPP_MTU          1500    /* max. transmit unit */

#define LCP_STATE_CLOSED        0       /* LCP state: closed (conf-req sent) */
#define LCP_STATE_ACK_RCVD      1       /* LCP state: conf-ack received */
#define LCP_STATE_ACK_SENT      2       /* LCP state: conf-ack sent */
#define LCP_STATE_OPENED        3       /* LCP state: opened */

#define IPCP_STATE_CLOSED       0       /* IPCP state: closed (conf-req sent) */
#define IPCP_STATE_ACK_RCVD     1       /* IPCP state: conf-ack received */
#define IPCP_STATE_ACK_SENT     2       /* IPCP state: conf-ack sent */
#define IPCP_STATE_OPENED       3       /* IPCP state: opened */

#define SPPP_LINK_DOWN		0	/* link down - no keepalive */
#define SPPP_LINK_UP		1	/* link is up - keepalive ok */

void wp_sppp_attach (struct ppp_device *pd);
void wp_sppp_detach (struct net_device *dev);
void wp_sppp_input (struct net_device *dev, struct sk_buff *m);
int wp_sppp_do_ioctl (struct net_device *dev, struct ifreq *ifr, int cmd);
struct sk_buff *sppp_dequeue (struct net_device *dev);
int sppp_isempty (struct net_device *dev);
void sppp_flush (struct net_device *dev);
int wp_sppp_open (struct net_device *dev);
int wp_sppp_reopen (struct net_device *dev);
int wp_sppp_close (struct net_device *dev);

#ifndef wp_min
#define wp_min(x,y) \
	({ unsigned int __x = (x); unsigned int __y = (y); __x < __y ? __x: __y; })
#endif

#ifndef wp_max
#define wp_max(x,y) \
	({ unsigned int __x = (x); unsigned int __y = (y); __x > __y ? __x: __y; })
#endif
	
#endif

#define SPPPIOCCISCO	(SIOCDEVPRIVATE)
#define SPPPIOCPPP	(SIOCDEVPRIVATE+1)
#define SPPPIOCDEBUG	(SIOCDEVPRIVATE+2)
#define SPPPIOCSFLAGS	(SIOCDEVPRIVATE+3)
#define SPPPIOCGFLAGS	(SIOCDEVPRIVATE+4)


#define AUTHNAMELEN	64
#define AUTHKEYLEN	16

#include <asm/byteorder.h>
#ifdef __BIG_ENDIAN
#define WORDS_BIGENDIAN
#endif

typedef u32 UWORD32;
typedef u32 UINT32;
typedef u8 UINT8;
typedef u16 UINT16;

#define md5byte unsigned char

struct wp_MD5Context {
	UWORD32 buf[4];
	UWORD32 bytes[2];
	UWORD32 in[16];
};

void wp_MD5Init(struct wp_MD5Context *context);
void wp_MD5Update(struct wp_MD5Context *context, md5byte const *buf, unsigned len);
void wp_MD5Final(unsigned char digest[16], struct wp_MD5Context *context);
void wp_MD5Transform(UWORD32 buf[4], UWORD32 const in[16]);


#endif /* _SYNCPPP_H_ */
