/*
 * Copyright (c) 2002
 *	Alex Feldman <al.feldman@sangoma.com>.  All rights reserved.
 *
 *	$Id: wanpipe_common.h,v 1.215 2008-04-16 17:23:17 sangoma Exp $
 */

/****************************************************************************
 * wanpipe_common.h	WANPIPE(tm) Multiprotocol WAN Link Driver.
 *
 * Authors: 	Alex Feldman <al.feldman@sangoma.com>
 *				David Rokhvarg <davidr@sangoma.com>
 *
 * ==========================================================================
 *
 * Apr 26,  2010 David Rokhvarg	Added "Global Irq lock" and "Mutex"
 *								functions.
 *
 * Nov 09,  2009 David Rokhvarg	Added wan_get_timestamp()
 *
 * Nov 27,  2007 David Rokhvarg	Implemented functions/definitions for
 *                              Sangoma MS Windows Driver and API.
 *
 * July 17, 2002 Alex Feldman 	Initial Version
 ****************************************************************************
 */

#ifndef	__WANPIPE_COMMON_H
# define __WANPIPE_COMMON_H

#if defined(WAN_KERNEL)
# include "wanpipe_kernel.h"
#endif
# include "wanpipe_cfg_def.h"

# include "wanpipe_logger.h"

#if defined(__WINDOWS__)
# if defined(WAN_KERNEL)
#  include "wanpipe_skb.h"
VOID
free_dma_buffer(
	IN	PDMA_ADAPTER	DmaAdapterObject,
	IN	u32				DmaBufferLength,
	IN	PVOID			virtual_addr,
	IN	u32				physical_addr
	);
int
allocate_dma_buffer(
	IN	PDMA_ADAPTER	DmaAdapterObject,
	IN	u32				DmaBufferLength,
	OUT PVOID			*virtual_addr,
	OUT u32				*physical_addr
	);
# else
#  include "wanpipe_api.h"
# endif/* !WAN_KERNEL */
#endif/* __WINDOWS__ */


/****************************************************************************
**			D E F I N E S				
****************************************************************************/
#ifndef NIPQUAD
# define NIPQUAD(addr) \
	((unsigned char *)&addr)[0], \
	((unsigned char *)&addr)[1], \
	((unsigned char *)&addr)[2], \
	((unsigned char *)&addr)[3]
#endif

#define NAME_PLACEHOLDER

#if defined(__NetBSD__) || defined(__FreeBSD__) || defined(__OpenBSD__)

# define WAN_LIST_HEAD(name, type)			LIST_HEAD(name, type)
# define WAN_LIST_HEAD_INITIALIZER(head)		LIST_HEAD_INITIALIZER(head)
# define WAN_LIST_ENTRY(type) 				LIST_ENTRY(type)
# define WAN_LIST_EMPTY(head)				LIST_EMPTY(head)
# define WAN_LIST_FIRST(head)				LIST_FIRST(head)		
# define WAN_LIST_FOREACH(var, head, field)		LIST_FOREACH(var, head, field)
# define WAN_LIST_INIT(head)				LIST_INIT(head)
# define WAN_LIST_INSERT_AFTER(listelm, elm, field)	LIST_INSERT_AFTER(listelm, elm, field)
/*# define WAN_LIST_INSERT_BEFORE(listelm, elm, field)	LIST_INSERT_BEFORE(listelm, elm, field)*/
# define WAN_LIST_INSERT_HEAD(head, elm, field) 	LIST_INSERT_HEAD(head, elm, field)
# define WAN_LIST_NEXT(elm, field)			LIST_NEXT(elm, field)
# define WAN_LIST_REMOVE(elm, field)			LIST_REMOVE(elm, field)

#elif defined(__SOLARIS__)

/*                ********* S O L A R I S *****************/

# define WAN_LIST_HEAD(name, type)		struct name { struct type * lh_first; }
# define WAN_LIST_HEAD_INITIALIZER(head)	{ NULL }
# define WAN_LIST_ENTRY(type) 			struct { struct type *le_next; struct type **le_prev; }
# define WAN_LIST_FIRST(head)			((head)->lh_first)
# define WAN_LIST_END(head)			NULL
# define WAN_LIST_EMPTY(head)			(WAN_LIST_FIRST(head) == WAN_LIST_END(head))
# define WAN_LIST_NEXT(elm, field)		((elm)->field.le_next)
# define WAN_LIST_FOREACH(var, head, field)	for((var) = WAN_LIST_FIRST(head);	\
							(var);				\
							(var) = WAN_LIST_NEXT(var, field))
# define WAN_LIST_INIT(head)		do { WAN_LIST_FIRST(head) = NULL;}\
		while(0)

#define	WAN_LIST_INSERT_HEAD(head, elm, field) do {				\
	if ((WAN_LIST_NEXT((elm), field) = WAN_LIST_FIRST((head))) != NULL)	\
		WAN_LIST_FIRST((head))->field.le_prev = &WAN_LIST_NEXT((elm), field);\
	WAN_LIST_FIRST((head)) = (elm);					\
	(elm)->field.le_prev = &WAN_LIST_FIRST((head));			\
} while (0)
#define	WAN_LIST_INSERT_AFTER(listelm, elm, field) do {			\
	if ((WAN_LIST_NEXT((elm), field) = WAN_LIST_NEXT((listelm), field)) != NULL)\
		WAN_LIST_NEXT((listelm), field)->field.le_prev =		\
		    &WAN_LIST_NEXT((elm), field);				\
	WAN_LIST_NEXT((listelm), field) = (elm);				\
	(elm)->field.le_prev = &WAN_LIST_NEXT((listelm), field);		\
} while (0)
#define	WAN_LIST_REMOVE(elm, field) do {					\
	if (WAN_LIST_NEXT((elm), field) != NULL)				\
		WAN_LIST_NEXT((elm), field)->field.le_prev = 		\
		    (elm)->field.le_prev;				\
	*(elm)->field.le_prev = WAN_LIST_NEXT((elm), field);		\
} while (0)


#elif defined(__LINUX__)
/* ********* L I N U X *****************/

# define WAN_LIST_HEAD(name, type)		struct name { struct type * lh_first; }
# define WAN_LIST_HEAD_INITIALIZER(head)	{ NULL }
# define WAN_LIST_ENTRY(type) 			struct { struct type *le_next; struct type **le_prev; }
# define WAN_LIST_FIRST(head)			((head)->lh_first)
# define WAN_LIST_END(head)			NULL
# define WAN_LIST_EMPTY(head)			(WAN_LIST_FIRST(head) == WAN_LIST_END(head))
# define WAN_LIST_NEXT(elm, field)		((elm)->field.le_next)
# define WAN_LIST_FOREACH(var, head, field)	for((var) = WAN_LIST_FIRST(head);	\
							(var);				\
							(var) = WAN_LIST_NEXT(var, field))
# define WAN_LIST_INIT(head)		do { WAN_LIST_FIRST(head) = NULL;}\
		while(0)

#define	WAN_LIST_INSERT_HEAD(head, elm, field) do {				\
	if ((WAN_LIST_NEXT((elm), field) = WAN_LIST_FIRST((head))) != NULL)	\
		WAN_LIST_FIRST((head))->field.le_prev = &WAN_LIST_NEXT((elm), field);\
	WAN_LIST_FIRST((head)) = (elm);					\
	(elm)->field.le_prev = &WAN_LIST_FIRST((head));			\
} while (0)
#define	WAN_LIST_INSERT_AFTER(listelm, elm, field) do {			\
	if ((WAN_LIST_NEXT((elm), field) = WAN_LIST_NEXT((listelm), field)) != NULL)\
		WAN_LIST_NEXT((listelm), field)->field.le_prev =		\
		    &WAN_LIST_NEXT((elm), field);				\
	WAN_LIST_NEXT((listelm), field) = (elm);				\
	(elm)->field.le_prev = &WAN_LIST_NEXT((listelm), field);		\
} while (0)
#define	WAN_LIST_REMOVE(elm, field) do {					\
	if (WAN_LIST_NEXT((elm), field) != NULL)				\
		WAN_LIST_NEXT((elm), field)->field.le_prev = 		\
		    (elm)->field.le_prev;				\
	*(elm)->field.le_prev = WAN_LIST_NEXT((elm), field);		\
} while (0)

#elif defined(__WINDOWS__)


# define WAN_LIST_HEAD(name, type)		struct name { struct type * lh_first; }
# define WAN_LIST_HEAD_INITIALIZER(head)	{ NULL }
# define WAN_LIST_ENTRY(type) 			struct { struct type *le_next; struct type **le_prev; }
# define WAN_LIST_FIRST(head)			((head)->lh_first)
# define WAN_LIST_END(head)			NULL
# define WAN_LIST_EMPTY(head)			(WAN_LIST_FIRST(head) == WAN_LIST_END(head))
# define WAN_LIST_NEXT(elm, field)		((elm)->field.le_next)
# define WAN_LIST_FOREACH(var, head, field)	for((var) = WAN_LIST_FIRST(head);	\
							(var);				\
							(var) = WAN_LIST_NEXT(var, field))
# define WAN_LIST_INIT(head)		do { WAN_LIST_FIRST(head) = NULL;}\
		while(0)

#define	WAN_LIST_INSERT_HEAD(head, elm, field) do {				\
	if ((WAN_LIST_NEXT((elm), field) = WAN_LIST_FIRST((head))) != NULL)	\
		WAN_LIST_FIRST((head))->field.le_prev = &WAN_LIST_NEXT((elm), field);\
	WAN_LIST_FIRST((head)) = (elm);					\
	(elm)->field.le_prev = &WAN_LIST_FIRST((head));			\
} while (0)
#define	WAN_LIST_INSERT_AFTER(listelm, elm, field) do {			\
	if ((WAN_LIST_NEXT((elm), field) = WAN_LIST_NEXT((listelm), field)) != NULL)\
		WAN_LIST_NEXT((listelm), field)->field.le_prev =		\
		    &WAN_LIST_NEXT((elm), field);				\
	WAN_LIST_NEXT((listelm), field) = (elm);				\
	(elm)->field.le_prev = &WAN_LIST_NEXT((listelm), field);		\
} while (0)
#define	WAN_LIST_REMOVE(elm, field) do {					\
	if (WAN_LIST_NEXT((elm), field) != NULL)				\
		WAN_LIST_NEXT((elm), field)->field.le_prev = 		\
		    (elm)->field.le_prev;				\
	*(elm)->field.le_prev = WAN_LIST_NEXT((elm), field);		\
} while (0)

#define GFP_KERNEL 0

#else
# error "WAN_LISTx macros not supported yet!"
#endif

#if defined(WAN_KERNEL)

static __inline void WP_MDELAY (u32 ms) {
	/* wait for "milliseconds * 1/1000" of sec */	
	for (; ms > 0; --ms){	
		WP_DELAY(1000);				
	}
}

#define WAN_STIMEOUT(start, timeout)					\
		((SYSTEM_TICKS - (start)) > ((timeout) * HZ))



#if defined(__LINUX__)
# define WAN_COPY_FROM_USER(k,u,l)	copy_from_user(k,u,l)
# define WAN_COPY_TO_USER(u,k,l)	copy_to_user(u,k,l)
#elif defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__)
# define WAN_COPY_FROM_USER(k,u,l)	copyin(u,k,l)
# define WAN_COPY_TO_USER(u,k,l)	copyout(k,u,l)
#elif defined(__WINDOWS__)

#else
# error "Undefined WAN_COPY_FROM_USER/WAN_COPY_TO_USER macros!"
#endif

#if defined(__LINUX__)
# if (LINUX_VERSION_CODE < KERNEL_VERSION(2,3,43))
#  define WAN_NETIF_WAKE_QUEUE(dev)	do {				\
					clear_bit(0, &dev->tbusy);	\
					mark_bh(NET_BH);		\
					} while(0)
#  define WAN_NETIF_START_QUEUE(dev)	do {				\
					dev->tbusy = 0;			\
					dev->interrupt = 0;		\
					dev->start = 1;			\
					} while(0);
#  define WAN_NETIF_STOP_QUEUE(dev)	set_bit(0, &dev->tbusy)
#  define WAN_NETIF_RUNNING(dev)	dev->start
#  define WAN_NETDEVICE_START(dev)	dev->start = 1
#  define WAN_NETDEVICE_STOP(dev)	dev->start = 0
#  define WAN_NETIF_QUEUE_STOPPED(dev)	test_bit(0,&dev->tbusy)
#  define WAN_NETIF_CARRIER_OFF(dev)	
#  define WAN_NETIF_CARRIER_ON(dev)	
#  define WAN_NETIF_CARRIER_OK(dev)	1	
# else
#if 0
#  define WAN_NETIF_WAKE_QUEUE(dev)	do{ 				\
					if (((wanpipe_common_t *)dev->priv)->usedby == TDM_VOICE){ \
						DEBUG_EVENT("%s: TDM VOICE not waking but starting!!!!\n",dev->name); \
						netif_start_queue(dev);			\
					}else{						\
						netif_wake_queue(dev);			\
					}						\
					}while(0)
#endif
#  define WAN_NETIF_WAKE_QUEUE(dev)	netif_wake_queue(dev);
#  define WAN_NETIF_START_QUEUE(dev)	netif_start_queue(dev)
#  define WAN_NETIF_STOP_QUEUE(dev)	netif_stop_queue(dev)
#  define WAN_NETIF_RUNNING(dev)	netif_running(dev)
#  define WAN_NETDEVICE_START(dev)
#  define WAN_NETDEVICE_STOP(dev)
#  define WAN_NETIF_QUEUE_STOPPED(dev)	netif_queue_stopped(dev)
#  define WAN_NETIF_CARRIER_OFF(dev)	netif_carrier_off(dev)
#  define WAN_NETIF_CARRIER_ON(dev)	netif_carrier_on(dev)
#  define WAN_NETIF_CARRIER_OK(dev)	netif_carrier_ok(dev)
# endif
# define WAN_NETIF_UP(dev)		((dev)->flags&IFF_UP)
# define WAN_NET_RATELIMIT		net_ratelimit

#elif defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__)
# define WAN_NETIF_QUEUE_STOPPED(dev)	((dev)->if_drv_flags & IFF_DRV_OACTIVE)
# define WAN_NETIF_WAKE_QUEUE(dev)	((dev)->if_drv_flags &= ~IFF_DRV_OACTIVE)
#if 0
# define WAN_NETIF_STOP_QUEUE(dev)
# define WAN_NETIF_START_QUEUE(dev)
#endif
# define WAN_NETIF_STOP_QUEUE(dev)	(dev)->if_drv_flags |= IFF_DRV_OACTIVE
# define WAN_NETIF_START_QUEUE(dev)	(dev)->if_drv_flags &= ~IFF_DRV_OACTIVE
# define WAN_NETIF_RUNNING(dev)		((dev)->if_drv_flags&IFF_DRV_RUNNING)
# define WAN_NETIF_UP(dev)		((dev)->if_flags&IFF_UP)
# define WAN_NETDEVICE_STOP(dev)
# define WAN_NETDEVICE_START(dev)
# define NET_ADMIN_CHECK()
# define WAN_NET_RATELIMIT()		1
# define MOD_INC_USE_COUNT
# define MOD_DEC_USE_COUNT

# define WAN_NETIF_CARRIER_OFF(dev)	
# define WAN_NETIF_CARRIER_ON(dev)	
# define WAN_NETIF_CARRIER_OK(dev)	1

#elif defined(__WINDOWS__)
# define WAN_NET_RATELIMIT()		1

# if defined(SPROTOCOL)
#  define WAN_NETIF_QUEUE_STOPPED(dev)	0 /* for LIP layer to work, it must be zero */
#  define WAN_NETIF_CARRIER_OFF(dev)	netif_carrier_off(dev)
#  define WAN_NETIF_CARRIER_ON(dev)		netif_carrier_on(dev)
# else
#  define WAN_NETIF_QUEUE_STOPPED(dev)	1 /*test_bit(0,&dev->tbusy)*/ /* must be always stopped so aft_core.c would call wanpipe_wake_stack() */
#  define WAN_NETIF_CARRIER_OFF(dev)	((dev)->current_line_state = SANG_STATUS_LINE_DISCONNECTED)
#  define WAN_NETIF_CARRIER_ON(dev)		((dev)->current_line_state = SANG_STATUS_LINE_CONNECTED)
# endif

# define WAN_NETIF_STOP_QUEUE(dev)	
# define WAN_NETIF_START_QUEUE(dev)	

//# define WAN_NETIF_QUEUE_STOPPED(dev)	test_bit(0,&dev->tbusy)
//# define WAN_NETIF_STOP_QUEUE(dev)	set_bit(0, &dev->tbusy)
//# define WAN_NETIF_START_QUEUE(dev)	dev->tbusy = 0;

# define WAN_NETIF_WAKE_QUEUE(dev)	netif_wake_queue(dev)

/*# define WAN_NETIF_UP(dev)			((dev)->current_line_state == SANG_STATUS_LINE_CONNECTED)*/
# define WAN_NETIF_UP(dev)			(1)

# define WAN_IFQ_LEN(ifqueue)		skb_queue_len(ifqueue)
# define NET_ADMIN_CHECK()
# define WARN_ON(a)
# define MOD_INC_USE_COUNT
# define MOD_DEC_USE_COUNT

#else
# error "Undefined WAN_NETIF_x macros!"
#endif

/* Update Network Interface statistics */
#if defined(__LINUX__) || defined(__WINDOWS__)

# define WAN_NETIF_STATS_INC_RX_PACKETS(common)		(common)->if_stats.rx_packets++
# define WAN_NETIF_STATS_INC_TX_PACKETS(common)		(common)->if_stats.tx_packets++
# define WAN_NETIF_STATS_RX_PACKETS(common)		((common)->if_stats.rx_packets)
# define WAN_NETIF_STATS_TX_PACKETS(common)		((common)->if_stats.tx_packets)
# define WAN_NETIF_STATS_INC_RX_BYTES(common,len)	(common)->if_stats.rx_bytes+=(len)
# define WAN_NETIF_STATS_INC_TX_BYTES(common,len)	(common)->if_stats.tx_bytes+=(len)
# define WAN_NETIF_STATS_INC_RX_ERRORS(common)		(common)->if_stats.rx_errors++
# define WAN_NETIF_STATS_INC_TX_ERRORS(common)		(common)->if_stats.tx_errors++
# define WAN_NETIF_STATS_INC_RX_DROPPED(common)		(common)->if_stats.rx_dropped++
# define WAN_NETIF_STATS_RX_DROPPED(common)		(common)->if_stats.rx_dropped
# define WAN_NETIF_STATS_INC_TX_DROPPED(common)		(common)->if_stats.tx_dropped++
# define WAN_NETIF_STATS_INC_MULTICAST(common)		(common)->if_stats.multicast++
# define WAN_NETIF_STATS_INC_COLLISIONS(common)		(common)->if_stats.collisions++
# define WAN_NETIF_STATS_INC_RX_LENGTH_ERRORS(common)	(common)->if_stats.rx_length_errors++
# define WAN_NETIF_STATS_INC_RX_OVER_ERRORS(common)	(common)->if_stats.rx_over_errors++
# define WAN_NETIF_STATS_INC_RX_CRC_ERRORS(common)	(common)->if_stats.rx_crc_errors++
# define WAN_NETIF_STATS_INC_RX_FRAME_ERRORS(common)	(common)->if_stats.rx_frame_errors++
# define WAN_NETIF_STATS_INC_RX_FIFO_ERRORS(common)	(common)->if_stats.rx_fifo_errors++
# define WAN_NETIF_STATS_INC_RX_MISSED_ERRORS(common)	(common)->if_stats.rx_missed_errors++
# define WAN_NETIF_STATS_INC_TX_ABORTED_ERRORS(common)	(common)->if_stats.tx_aborted_errors++
# define WAN_NETIF_STATS_INC_TX_CARRIER_ERRORS(common)	(common)->if_stats.tx_carrier_errors++
# define WAN_NETIF_STATS_TX_CARRIER_ERRORS(common)	(common)->if_stats.tx_carrier_errors
# define WAN_NETIF_STATS_INC_TX_FIFO_ERRORS(common)	(common)->if_stats.tx_fifo_errors++
# define WAN_NETIF_STATS_INC_TX_HEARTBEAT_ERRORS(common) (common)->if_stats.tx_heartbeat_errors++
# define WAN_NETIF_STATS_INC_TX_WINDOW_ERRORS(common)	(common)->if_stats.tx_window_errors++
# define WAN_NETIF_STATS_INC_RX_COMPRESSED(common)	(common)->if_stats.rx_compressed++
# define WAN_NETIF_STATS_INC_TX_COMPRESSED(common)	(common)->if_stats.tx_compressed++

#elif defined(__FreeBSD__) || defined(__OpenBSD__)

# define WAN_NETIF_STATS_INC_RX_PACKETS(common)		if ((common)->dev) (common)->dev->if_ipackets++
# define WAN_NETIF_STATS_INC_TX_PACKETS(common)		if ((common)->dev) (common)->dev->if_opackets++
# define WAN_NETIF_STATS_RX_PACKETS(common)		((common)->dev) ? (common)->dev->if_ipackets : 0
# define WAN_NETIF_STATS_TX_PACKETS(common)		((common)->dev) ? (common)->dev->if_opackets : 0
# define WAN_NETIF_STATS_INC_RX_BYTES(common,len)	if ((common)->dev) (common)->dev->if_ibytes+=(len)
# define WAN_NETIF_STATS_INC_TX_BYTES(common,len)	if ((common)->dev) (common)->dev->if_obytes+=(len)
# define WAN_NETIF_STATS_INC_RX_ERRORS(common)		if ((common)->dev) (common)->dev->if_ierrors++
# define WAN_NETIF_STATS_INC_TX_ERRORS(common)		if ((common)->dev) (common)->dev->if_oerrors++
# define WAN_NETIF_STATS_INC_RX_DROPPED(common)		if ((common)->dev) (common)->dev->if_iqdrops++
# define WAN_NETIF_STATS_RX_DROPPED(common)		if ((common)->dev) (common)->dev->if_iqdrops
# define WAN_NETIF_STATS_INC_TX_DROPPED(common)
# define WAN_NETIF_STATS_INC_MULTICAST(common)		if ((common)->dev) (common)->dev->if_omcasts++
# define WAN_NETIF_STATS_INC_COLLISIONS(common)		if ((common)->dev) (common)->dev->if_collisions++
# define WAN_NETIF_STATS_INC_RX_LENGTH_ERRORS(common)	if ((common)->dev) (common)->dev->if_ierrors++
# define WAN_NETIF_STATS_INC_RX_OVER_ERRORS(common)	if ((common)->dev) (common)->dev->if_ierrors++
# define WAN_NETIF_STATS_INC_RX_CRC_ERRORS(common)	if ((common)->dev) (common)->dev->if_ierrors++
# define WAN_NETIF_STATS_INC_RX_FRAME_ERRORS(common)	if ((common)->dev) (common)->dev->if_ierrors++
# define WAN_NETIF_STATS_INC_RX_FIFO_ERRORS(common)	if ((common)->dev) (common)->dev->if_ierrors++
# define WAN_NETIF_STATS_INC_RX_MISSED_ERRORS(common)	if ((common)->dev) (common)->dev->if_ierrors++
# define WAN_NETIF_STATS_INC_TX_ABORTED_ERRORS(common)	if ((common)->dev) (common)->dev->if_oerrors++
# define WAN_NETIF_STATS_INC_TX_CARRIER_ERRORS(common)	if ((common)->dev) (common)->dev->if_oerrors++
# define WAN_NETIF_STATS_TX_CARRIER_ERRORS(common)	if ((common)->dev) (common)->dev->if_oerrors
# define WAN_NETIF_STATS_INC_TX_FIFO_ERRORS(common)	if ((common)->dev) (common)->dev->if_oerrors++
# define WAN_NETIF_STATS_INC_TX_HEARTBEAT_ERRORS(common)	if ((common)->dev) (common)->dev->if_oerrors++
# define WAN_NETIF_STATS_INC_TX_WINDOW_ERRORS(common)	if ((common)->dev) (common)->dev->if_oerrors++
# define WAN_NETIF_STATS_INC_RX_COMPRESSED(common)	
# define WAN_NETIF_STATS_INC_TX_COMPRESSED(common)

#else

# error "Undefined WAN_NETIF_STATSx macro!"
# define WAN_NETIF_STATS_INC_RXPACKETS(common)
# define WAN_NETIF_STATS_INC_TXPACKETS(common)
# define WAN_NETIF_STATS_INC_RXBYTES(common)
# define WAN_NETIF_STATS_INC_TXBYTES(common)
# define WAN_NETIF_STATS_INC_RXERRORS(common)
# define WAN_NETIF_STATS_INC_TXERRORS(common)
# define WAN_NETIF_STATS_INC_RXDROPPED(common)
# define WAN_NETIF_STATS_INC_TXDROPPED(common)
# define WAN_NETIF_STATS_INC_MULTICAST(common)
# define WAN_NETIF_STATS_INC_COLLISIONS(common)
# define WAN_NETIF_STATS_INC_RXLENGTHERRORS(common)
# define WAN_NETIF_STATS_INC_RXOVERERRORS(common)
# define WAN_NETIF_STATS_INC_RXCRCERRORS(common)
# define WAN_NETIF_STATS_INC_RXFRAMESERRORS(common)
# define WAN_NETIF_STATS_INC_RXFIFOERRORS(common)
# define WAN_NETIF_STATS_INC_RXMISSEDERRORS(common)

# define WAN_NETIF_STATS_INC_TXABORTEDERRORS(common)
# define WAN_NETIF_STATS_INC_TXCARRIERERRORS(common)
# define WAN_NETIF_STATS_INC_TXFIFOERRORS(common)
# define WAN_NETIF_STATS_INC_TXHEARTBEATERRORS(common)
# define WAN_NETIF_STATS_INC_TXWINDOWERRORS(common)

# define WAN_NETIF_STATS_INC_RXCOMPRESSED(common)
# define WAN_NETIF_STATS_INC_TXCOMPRESSED(common)
#endif

#if defined(__LINUX__)
# define WAN_BPF_DIR_IN		(1<<0)
# define WAN_BPF_DIR_OUT		(1<<1)
# define WAN_BPF_REPORT(dev,m)
#elif defined(__FreeBSD__)
#elif defined(__OpenBSD__)
#elif defined(__NetBSD__)
#elif defined(__WINDOWS__)
#else
# error "Undefined WAN_BPF_REPORT macro!"
#endif


#if defined (__LINUX__)
# define WAN_DEV_PUT(dev)	dev_put(dev)
# define WAN_DEV_HOLD(dev)	dev_hold(dev)
# define __WAN_PUT(str)		wan_atomic_dec(&(str)->refcnt)
# define WAN_PUT(str)		if (atomic_dec_and_test(&(str)->refcnt)){ \
		       			wan_kfree(str); 		\
                       		} 
# define WAN_HOLD(str)		wan_atomic_inc(&(str)->refcnt)
#elif defined(__FreeBSD__)
# define WAN_DEV_PUT(dev)
# define WAN_DEV_HOLD(dev)
# define __WAN_PUT(str)		wan_atomic_dec(&(str)->refcnt)
# define WAN_PUT(str)		wan_atomic_dec(&str->refcnt);	\
				if (str->refcnt){		\
		       			WAN_FREE(str);		\
                       		} 
# define WAN_HOLD(str)		wan_atomic_inc(&(str)->refcnt)
#elif defined(__NetBSD__) || defined(__OpenBSD__)
# define WAN_DEV_PUT(dev)
# define WAN_DEV_HOLD(dev)
# define __WAN_PUT(str)	str->refcnt--
# define WAN_PUT(str)	str->refcnt--;	\
			if (str->refcnt){		\
		       		WAN_FREE(str);		\
                       	} 
# define WAN_HOLD(str)	str->refcnt++
#elif defined(__WINDOWS__)

# define WAN_DEV_PUT(dev)	wan_atomic_dec(&dev->refcnt)
# define WAN_DEV_HOLD(dev)	wan_atomic_inc(&(dev)->refcnt)
# define __WAN_PUT(str)		wan_atomic_dec(&(str)->refcnt)

# define WAN_HOLD(str)		wan_atomic_inc(&(str)->refcnt)
#else
# warning "Undefined WAN_HOLD/WAN_PUT macro!"
#endif

#if defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__)
# ifdef ENABLE_SPPP
#  define WAN_SPPP_ENABLED		1
#  define WAN_SPPP_ATTACH(ifp)		sppp_attach(ifp)
#  define WAN_SPPP_DETACH(ifp)		sppp_detach(ifp)
#  define WAN_SPPP_FLUSH(ifp)		sppp_flush(ifp)
#  define WAN_SPPP_PICK(ifp)		sppp_pick(ifp)
#  define WAN_SPPP_DEQUEUE(ifp)		sppp_dequeue(ifp)
#  define WAN_SPPP_ISEMPTY(ifp)		sppp_isempty(ifp)
#  define WAN_SPPP_INPUT(ifp,skb)	sppp_input(ifp,skb)
#  define WAN_SPPP_IOCTL(ifp,cmd,data)	sppp_ioctl(ifp,cmd,data);
# else
#  define WAN_SPPP_ENABLED		0
#  define WAN_SPPP_ATTACH(ifp)		
#  define WAN_SPPP_DETACH(ifp)		
#  define WAN_SPPP_FLUSH(ifp)		
#  define WAN_SPPP_PICK(ifp)		NULL
#  define WAN_SPPP_DEQUEUE(ifp)		NULL
#  define WAN_SPPP_ISEMPTY(ifp)		0
#  define WAN_SPPP_INPUT(ifp,skb)	
#  define WAN_SPPP_IOCTL(ifp,cmd,data)	-EOPNOTSUPP
# endif
#elif defined(__LINUX__)
# define WAN_SPPP_ENABLED		1
# define WAN_SPPP_IOCTL(ifp,cmd,data)	-EOPNOTSUPP
#elif defined(__WINDOWS__)
#else
# error "Undefined WAN_SPPP_x macros!"
#endif

#define WAN_MAX_TRACE_TIMEOUT	(5*HZ)

#if 0
/*
 * Variable argument list macro definitions
 */
#ifndef _VALIST
#define _VALIST
typedef char *va_list;
#endif /* _VALIST */
#define _WAN_VA_SIZE(type) (((sizeof(type) + sizeof(long) - 1) / sizeof(long)) * sizeof(long))

#define WAN_VA_START(ap, A)	((ap) = (va_list) &(A) + _WAN_VA_SIZE(A))
#define WAN_VA_ARG(ap, T)	(*(T *)((ap) += _WAN_VA_SIZE(T),(ap) - _WAN_VA_SIZE (T)))
#define WAN_VA_END(ap)		(void) 0
#endif


/****************************************************************************
**			T Y P E D E F S				
****************************************************************************/
# if defined(__NetBSD__) || defined(__FreeBSD__) || defined(__OpenBSD__)
/*
 * Ethernet statistics collection data
 */ 
struct net_device_stats 
{
	unsigned long rx_packets;	/* total packets received    */
	unsigned long tx_packets;	/* total packets transmited  */
	unsigned long rx_bytes;		/* total bytes received    */
	unsigned long tx_bytes;		/* total bytes transmited  */
	unsigned long rx_errors;	/* bad packet received       */      
	unsigned long tx_errors;	/* packet transmit problems  */
	unsigned long rx_dropped;	/* no space in buffers       */
	unsigned long tx_dropped;	/* no space available        */
	unsigned long multicast;	/* multicast packet received */
	unsigned long collisions;   

	/* detailed rx_errors */
	unsigned long rx_length_errors;
	unsigned long rx_over_errors;	/* receiver ring off buff overflow */
	unsigned long rx_crc_errors;	/* recv'd pkt with crc error       */
	unsigned long rx_frame_errors;	/* recv'd frame alignment error    */
	unsigned long rx_fifo_errors;	/* recv'r fifo overrun             */
	unsigned long rx_missed_errors;	/* receiver missed packet          */

	/* detailed tx_errors */
	unsigned long tx_aborted_errors;
	unsigned long tx_carrier_errors;
	unsigned long tx_fifo_errors;
	unsigned long tx_heartbeat_errors;
	unsigned long tx_window_errors;

	/* for cslip etc */
	unsigned long	rx_compressed;
	unsigned long	tx_compressed;
};
#endif

/****************************************************************************
**		F U N C T I O N   P R O T O T Y P E S				
****************************************************************************/
unsigned int 	wan_dec2uint (unsigned char* str, int len);
char*		wanpipe_get_state_string (void*);
void 		wanpipe_set_state (void*, int);
char 		wanpipe_get_state (void*);
void		wanpipe_card_lock_irq (void *,unsigned long *);
void		wanpipe_card_unlock_irq (void *,unsigned long *);
void		wanpipe_set_baud(void*card,unsigned int baud);
unsigned long 	wan_get_ip_addr   (void*, int);
int		wan_udp_pkt_type  (void*, caddr_t);
int		wan_reply_udp	  (void*, unsigned char*, unsigned int);
unsigned short	wan_calc_checksum(char *data, int len);
void 		wanpipe_debug_timer_init(void*);
void 		wan_trace_info_init(wan_trace_t *trace, int max_trace_queue);
int 		wan_trace_purge (wan_trace_t *trace);
int 		wan_trace_enqueue(wan_trace_t *trace, void *skb_ptr);
int		wan_tracing_enabled(wan_trace_t *trace_info);
#if defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__)
void		wanpipe_debugging (void* data, int pending);
#else
void		wanpipe_debugging (ulong_ptr_t data);
#endif










/****************************************************************************
**		I N L I N E   F U N C T I O N S				
****************************************************************************/
/******************* WANPIPE MALLOC/FREE FUNCTION ******************/
/*
** wan_malloc - 
*/

#ifdef WAN_DEBUG_MEM

#include "wan_mem_debug.h"

#define wan_malloc(size) __wan_malloc(size,(const char*)__FUNCTION__,(const int)__LINE__)
static __inline void* __wan_malloc(int size, const char *func_name, const int line)
#else
static __inline void* wan_malloc(int size)
#endif
{
	void*	ptr = NULL;
#if defined(__LINUX__)
	ptr = kmalloc(size, GFP_ATOMIC);
#elif defined(__SOLARIS__)
	ptr=kmem_alloc(size,KM_NOSLEEP);
#elif defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__)
	ptr = malloc(size, M_DEVBUF, M_NOWAIT); 
#elif defined(__WINDOWS__)
	ptr = kmalloc(size, '1lmk'/* memory pool tag */);
#else
# error "wan_malloc() function is not supported yet!"
#endif
	if (ptr){
		memset(ptr, 0, size);
#ifdef WAN_DEBUG_MEM
		sdla_memdbg_push(ptr, func_name, line, size);
#endif
	}
	return ptr;
}

#ifdef WAN_DEBUG_MEM
#define wan_kmalloc(size) __wan_kmalloc(size,(const char*)__FUNCTION__,(const int)__LINE__)
static __inline void* __wan_kmalloc(int size, const char *func_name, const int line)
#else
static __inline void* wan_kmalloc(int size)
#endif
{
	void*	ptr = NULL;
#if defined(__LINUX__)
	ptr = kmalloc(size, GFP_KERNEL);
#elif defined(__SOLARIS__)
	ptr=kmem_alloc(size,KM_NOSLEEP);
#elif defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__)
	ptr = malloc(size, M_DEVBUF, M_NOWAIT); 
#elif defined(__WINDOWS__)
	ptr = kmalloc(size, '2lmk'/* memory pool tag */);
#else
# error "wan_malloc() function is not supported yet!"
#endif
	if (ptr){
		memset(ptr, 0, size);
#ifdef WAN_DEBUG_MEM
		sdla_memdbg_push(ptr, func_name, line, size);
#endif
	}
	return ptr;
}     

/*
** wan_free - 
*/
#ifdef WAN_DEBUG_MEM
#define wan_free(ptr) __wan_free(ptr,(const char*)__FUNCTION__,(const int)__LINE__)
static __inline void __wan_free(void* ptr, const char *func_name, const int line)
#else
static __inline void wan_free(void* ptr)
#endif
{
	if (!ptr){
		DEBUG_EVENT("wan_free: NULL PTR !!!!!\n");
		return;
	}
	
#ifdef WAN_DEBUG_MEM
	if (sdla_memdbg_pull(ptr, func_name, line) != 0) {
		DEBUG_EVENT("%s: %d: %s Invalid Free\n",func_name,line,__FUNCTION__);
	}
#endif

#if defined(__LINUX__)
	kfree(ptr);
#elif defined(__SOLARIS__)
	kmem_free(ptr,sizeof(*ptr));
	DEBUG_EVENT("%s: Feeing Size %i\n",__FUNCTION__,sizeof(*ptr));
#elif defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__)
	free(ptr, M_DEVBUF); 
#elif defined(__WINDOWS__)
	kfree(ptr);
#else
# error "wan_free() function is not supported yet!"
#endif
}


#ifdef WAN_DEBUG_MEM
#define wan_vmalloc(size) __wan_vmalloc(size,(const char*)__FUNCTION__,(const int)__LINE__)
static __inline void* __wan_vmalloc(int size, const char *func_name, const int line)
#else
static __inline void* wan_vmalloc(int size)
#endif
{
	void*	ptr = NULL;
#if defined(__LINUX__)
	ptr = vmalloc(size);
#elif defined(__FreeBSD__)
	ptr = (caddr_t)kmem_alloc(kernel_map, size + sizeof(vm_size_t));
	if (ptr){
		vm_size_t	*ptr1 = (vm_size_t*)ptr;
		bzero(ptr, size + sizeof(vm_size_t));
		*ptr1 = size + sizeof(vm_size_t);
		ptr = ptr1++;
	}
#elif defined(__OpenBSD__) || defined(__NetBSD__)
	ptr = (caddr_t)uvm_km_alloc(kernel_map, size + sizeof(vsize_t));
	if (ptr){
		vsize_t	*ptr1 = (vsize_t*)ptr;
		bzero(ptr, size);
		*ptr1 = size + sizeof(vsize_t);
		ptr = ptr1++;
	}
#elif defined(__SOLARIS__)
	ptr = kmalloc(size);
#elif defined(__WINDOWS__)
	ptr = kmalloc(size, '3lmk'/* memory pool tag */);
#else
# error "wan_vmalloc() function is not supported yet!"
#endif
	if (ptr){
		memset(ptr, 0, size);
#ifdef WAN_DEBUG_MEM
		sdla_memdbg_push(ptr, func_name, line, size);
#endif
	}
	return ptr;
}

/*
** wan_vfree - 
*/


#ifdef WAN_DEBUG_MEM
#define wan_vfree(ptr) __wan_vfree(ptr,(const char*)__FUNCTION__,(const int)__LINE__)
static __inline void __wan_vfree(void* ptr, const char *func_name, const int line)
#else
static __inline void wan_vfree(void* ptr)
#endif

{
	if (!ptr){
		DEBUG_EVENT("wan_vfree: NULL PTR !!!!!\n");
		return;
	}

#ifdef WAN_DEBUG_MEM
	if (sdla_memdbg_pull(ptr, func_name, line) != 0) {
		DEBUG_EVENT("%s: %d: %s Invalid Free\n",func_name,line,__FUNCTION__);
	}
#endif

#if defined(__LINUX__)
	vfree(ptr);
#elif defined(__FreeBSD__)
	{
		vm_size_t	*ptr1 = (vm_size_t*)ptr;
		ptr1 --;
		kmem_free(kernel_map, (vm_offset_t)ptr1, (vm_size_t)*ptr1);
	}
#elif defined(__OpenBSD__) || defined(__NetBSD__)
	{
		vsize_t	*ptr1 = (vsize_t*)ptr;
		ptr1 --;
		uvm_km_free(kernel_map, (vaddr_t)ptr1, (vsize_t)*ptr1);
	}
#elif defined(__SOLARIS__)
#elif defined(__WINDOWS__)
	kfree(ptr);
#else
# error "wan_free() function is not supported yet!"
#endif
	return;
}


/******************* WANPIPE VIRT<->BUS SPACE FUNCTION ******************/
/*
** wan_virt2bus
*/
static __inline unsigned long wan_virt2bus(unsigned long* ptr)
{
#if defined(__LINUX__)
	return virt_to_bus(ptr);
#elif defined(__FreeBSD__)
	return vtophys((vm_offset_t)ptr);
#elif defined(__OpenBSD__) || defined(__NetBSD__)
	return vtophys((vaddr_t)ptr);
#elif defined(__WINDOWS__)
	FUNC_NOT_IMPL();
	return 0;
#else
# error "wan_virt2bus() function is not supported yet!"
#endif
}

/*
** wan_bus2virt
*/
static __inline unsigned long* wan_bus2virt(unsigned long virt_addr)
{
#if defined(__LINUX__)
	return bus_to_virt(virt_addr);
#elif defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__)
	return (unsigned long*)virt_addr;
#elif defined(__WINDOWS__)
	FUNC_NOT_IMPL();
	return 0;
#else
# error "wan_bus2virt() function is not supported yet!"
#endif
}


/******************* WANPIPE DMA FUNCTION ******************/

/*
** wan_dma_alloc
*/
static __inline int
wan_dma_alloc_org(void* hw, wan_dma_descr_org_t* dma_descr)
{
	int	err = 0;
#if defined(__FreeBSD__)
	err = bus_dma_tag_create(/*parent*/NULL, 
				/*alignemnt*/1, 
				/*boundary*/0,
			        /*lowaddr*/BUS_SPACE_MAXADDR_32BIT,
			        /*highaddr*/BUS_SPACE_MAXADDR,
			        /*filter*/NULL, /*filterarg*/NULL,
			        /*maxsize*/dma_descr->max_length,
# if (__FreeBSD_version >= 502000)
			        /*nsegments*/1,
# else
			        /*nsegments*/BUS_SPACE_UNRESTRICTED,
# endif
			        /*maxsegsz*/BUS_SPACE_MAXSIZE_32BIT,
			        /*flags*/0, 
# if (__FreeBSD_version >= 502000)
				/*lockfunc*/NULL, /*lockfuncarg*/NULL,
# endif
				&dma_descr->dmat);
	if (err){
		DEBUG_EVENT("Failed create DMA tag (size=%ld)!\n",
					dma_descr->max_length);
		dma_descr->max_length = 0;
		return err;
	}
	err = bus_dmamem_alloc(dma_descr->dmat, 
			       (void**)&dma_descr->vAddr, 
			       BUS_DMA_NOWAIT, 
			       &dma_descr->dmamap);
	if (err){
		DEBUG_EVENT("Failed allocate DMA (size=%ld)!\n",
					dma_descr->max_length);
		bus_dma_tag_destroy(dma_descr->dmat);
		dma_descr->max_length = 0;
		return err;
	}
#elif defined(__OpenBSD__) || defined(__NetBSD__)
	err = bus_dmamem_alloc(dma_descr->dmat, 	/* dma tag */
				dma_descr->max_length, 	/* size */
				PAGE_SIZE, 		/* alignment */
				0,			/* boundary */
				&dma_descr->dmaseg,	/* serments */
				1,			/* num of segments */
				&dma_descr->rsegs,	/* R num of segments */
				BUS_DMA_NOWAIT);	
	if (err){
		DEBUG_EVENT("Failed allocate DMA segment (size=%ld)!\n",
					dma_descr->max_length);
		dma_descr->max_length = 0;
		return err;
	}
	err = bus_dmamem_map(dma_descr->dmat,		/* dma tag */
				&dma_descr->dmaseg,	/* segments */
				dma_descr->rsegs,	/* return num of segments */
				dma_descr->max_length, 	/* size */
				(caddr_t*)&dma_descr->vAddr,	/* kernel virtual address */
				BUS_DMA_NOWAIT);
	if (err){
		DEBUG_EVENT("Failed map DMA segment (size=%ld)!\n",
					dma_descr->max_length);
		dma_descr->max_length = 0;
		bus_dmamem_free(dma_descr->dmat, &dma_descr->dmaseg, dma_descr->rsegs);
		return err;
	}		
#elif defined(__LINUX__)
	dma_descr->vAddr = pci_alloc_consistent(NULL,
					dma_descr->max_length,
					(dma_addr_t *)&dma_descr->pAddr); 
	if (dma_descr->vAddr == NULL){
		err = -ENOMEM;
	}
#elif defined(__WINDOWS__)
	{
		PVOID	virtual_addr;
		u32		physical_addr;

		if(allocate_dma_buffer(	dma_descr->DmaAdapterObject, 
								dma_descr->max_length,
								&virtual_addr, 
								&physical_addr)){
			DEBUG_EVENT("%s(): Error: failed to allocate DMA memory!\n", __FUNCTION__);
			return -ENOMEM;
		}
		dma_descr->vAddr = virtual_addr;
		dma_descr->pAddr = physical_addr;
	}
#else
# error "wan_dma_alloc() function is not supported yet!"
#endif
	return err;
}

/*
** wan_dma_free
*/
static __inline int
wan_dma_free_org(void* hw, wan_dma_descr_org_t* dma_descr)
{
#if defined(__FreeBSD__)
	bus_dmamem_free(dma_descr->dmat, dma_descr->vAddr, dma_descr->dmamap);
	return bus_dma_tag_destroy(dma_descr->dmat);
#elif defined(__OpenBSD__) || defined(__NetBSD__)
	bus_dmamem_unmap(dma_descr->dmat, (caddr_t)dma_descr->vAddr, dma_descr->max_length);
	bus_dmamem_free(dma_descr->dmat, &dma_descr->dmaseg, dma_descr->rsegs);
#elif defined(__LINUX__)

	DEBUG_TEST("Freeing Pages 0x%p len=%li  order=%i\n",
		dma_descr->vAddr,
		dma_descr->max_length,
		get_order(dma_descr->max_length));

	pci_free_consistent(NULL, dma_descr->max_length,dma_descr->vAddr,dma_descr->pAddr);
	dma_descr->vAddr = NULL;
	dma_descr->pAddr = 0;
#elif defined(__WINDOWS__)
	free_dma_buffer(dma_descr->DmaAdapterObject, dma_descr->max_length,
					dma_descr->vAddr, dma_descr->pAddr);
	dma_descr->vAddr = NULL;
	dma_descr->pAddr = 0;
#else
# error "wan_dma_free() function is not supported yet!"
#endif
	return 0;
}

static __inline unsigned long* wan_dma_get_vaddr(void* card, wan_dma_descr_org_t* dma)
{
	return dma->vAddr;
}

static __inline unsigned long wan_dma_get_paddr(void* card, wan_dma_descr_org_t* dma)
{
#if defined(__WINDOWS__)
	return dma->pAddr;
#else
	return wan_virt2bus(dma->vAddr);
#endif
}





/********************** WANPIPE TIMER FUNCTION **************************/



static __inline int wan_getcurrenttime(wan_time_t *sec, wan_suseconds_t *usec)
{
#if defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__)
	struct timeval 	tv;
	microtime(&tv);
	if (sec) *sec = tv.tv_sec;
	if (usec) *usec = tv.tv_usec;
	return 0;
#elif defined(__LINUX__)
	struct timeval 	tv;
	do_gettimeofday(&tv);
	if (sec) *sec = tv.tv_sec;
	if (usec) *usec = tv.tv_usec;
	return 0;
#elif defined(__WINDOWS__)
	struct timeval 	tv;
	do_gettimeofday(&tv);
	if (sec) *sec = tv.tv_sec;
	if (usec) *usec = tv.tv_usec;
	return 0;
#else
# error "wan_getcurrenttime() function is not supported yet!"
#endif
}


/*
 * Obtain the number of seconds elapsed since midnight (00:00:00), 
 * January 1, 1970.
 * Equivalent to "time()" and "_time64()" in user-space.
 * User-mode applications can call "ctime()" and "_ctime64()" or
 * sangoma_ctime() for interpretation of this timestamp.
 * 
 */

static __inline int wan_get_timestamp(wan_time_t *sec, wan_suseconds_t *usec)
{
#if defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__)
	struct timeval 	tv;
	microtime(&tv);
	if (sec) *sec = tv.tv_sec;
	if (usec) *usec = tv.tv_usec;
	return 0;
#elif defined(__LINUX__)
	struct timeval 	tv;
	jiffies_to_timeval(jiffies,&tv);
	if (sec) *sec = tv.tv_sec;
	if (usec) *usec = tv.tv_usec;
	return 0;
#elif defined(__WINDOWS__)
	struct timeval 	tv;
	wp_time(&tv.tv_sec);	/* number of seconds elapsed since 
							 * midnight (00:00:00), January 1, 1970 */
	tv.tv_usec = get_milliseconds_in_current_second() * 1000;
	if (sec) *sec = tv.tv_sec;
	if (usec) *usec = tv.tv_usec;
	return 0;
#else
# error "wan_get_timestamp() function is not supported yet!"
#endif
}


/*
** wan_init_timer
*/
static __inline void
wan_init_timer(wan_timer_t* wan_timer, wan_timer_func_t timer_func, wan_timer_arg_t arg)
{
#if defined(__LINUX__)
	init_timer(&wan_timer->timer_info);
	wan_timer->timer_info.function = timer_func;
	wan_timer->timer_info.data = arg;
#elif defined(__FreeBSD__)
	/* FIXME_ADSL_TIMER */
	callout_handle_init(&wan_timer->timer_info);
	wan_timer->timer_func = timer_func;
	wan_timer->timer_arg  = arg;
#elif defined(__OpenBSD__)
	timeout_set(&wan_timer->timer_info, timer_func, (void*)arg);
	wan_timer->timer_func = timer_func;
	wan_timer->timer_arg  = arg;
#elif defined(__NetBSD__)
	callout_init(&wan_timer->timer_info);
	wan_timer->timer_func = timer_func;
	wan_timer->timer_arg  = arg;
#elif defined(__WINDOWS__)
	wan_timer->timer_func = timer_func;
	wan_timer->timer_arg  = arg;
	KeInitializeDpc(&wan_timer->timer_info.TimerDpcObject,
			wan_timer->timer_func,
			wan_timer->timer_arg);
	KeInitializeTimer(&wan_timer->timer_info.Timer);
#else
# error "wan_init_timer() function is not supported yet!"
#endif /* linux */
}

/*
** wan_del_timer
*/
static __inline void
wan_del_timer(wan_timer_t* wan_timer)
{
#if defined(__LINUX__)
	if (!wan_timer->timer_info.function){
		if (WAN_NET_RATELIMIT()){
		DEBUG_EVENT("%s:%d Warning: WAN Timer del error: func=%p\n",
				__FUNCTION__,__LINE__,
				wan_timer->timer_info.function);
		}
		return;
	}
	del_timer(&wan_timer->timer_info);
#elif defined(__FreeBSD__)
	untimeout(wan_timer->timer_func,
		  (void*)wan_timer->timer_arg, 
		  wan_timer->timer_info);
	callout_handle_init(&wan_timer->timer_info);
#elif defined(__OpenBSD__)
	timeout_del(&wan_timer->timer_info);
#elif defined(__NetBSD__)
	callout_stop(&wan_timer->timer_info);
#elif defined(__WINDOWS__)
	KeCancelTimer(&wan_timer->timer_info.Timer);
#else
# error "wan_del_timer() function is not supported yet!"
#endif /* linux */
}

/*
** wan_add_timer
*/
static __inline int 
wan_add_timer(wan_timer_t* wan_timer, unsigned long delay)
{
#if defined(__LINUX__)
	if (timer_pending(&wan_timer->timer_info) ||
	    !wan_timer->timer_info.function){
		if (WAN_NET_RATELIMIT()){
		DEBUG_EVENT("%s:%d Warning: WAN Timer add error: pending or func=%p\n",
				__FUNCTION__,__LINE__,
				wan_timer->timer_info.function);
		}
		return -EINVAL;
	}
    	wan_timer->timer_info.expires = SYSTEM_TICKS + delay;
    	add_timer(&wan_timer->timer_info);
#elif defined(__FreeBSD__)
	wan_timer->timer_info = 
			timeout(wan_timer->timer_func,
				(void*)wan_timer->timer_arg, 
				delay);
	WAN_ASSERT(wan_timer->timer_info.callout == NULL);
#elif defined(__OpenBSD__)
	timeout_add(&wan_timer->timer_info, delay);
#elif defined(__NetBSD__)
	wan_timer->timer_info.c_time = delay;
	callout_reset(&wan_timer->timer_info,
			delay,
			wan_timer->timer_func,
			wan_timer->timer_arg); 
#elif defined(__WINDOWS__)
	LARGE_INTEGER	DueTime;

	/**********************************
	  The 'delay' is in SYSTEM TICKS!
	***********************************/

	/* KeSetTimer(): the expiration time is expressed in system
	 * time units (100-nanosecond intervals).
	 *
	 * KeQueryTimeIncrement(): returns the number of 
	 * 100-nanosecond units that are added to the system time 
	 * each time the interval clock interrupts.
	 *
	 * If the value of the DueTime parameter is negative,
	 * the expiration time is relative to the current system time.
	 *
	 * Relative expiration times are NOT affected by system time changes. 
	 */
	DueTime.QuadPart = -1 * ((LONGLONG) (delay * KeQueryTimeIncrement()));

	KeSetTimer(&wan_timer->timer_info.Timer, DueTime,
		&wan_timer->timer_info.TimerDpcObject);
#else
# error "wan_add_timer() function is not supported yet!"
#endif /* linux */
	return 0;
}

/********************** WANPIPE KERNEL BUFFER **************************/
/*
** wan_skb_data() - 
**		Returns pointer to data.
*/
static __inline unsigned char* wan_skb_data(void* skb)
{
#if defined(__LINUX__)
	return ((struct sk_buff*)skb)->data;
#elif defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__)
	return mtod((struct mbuf*)skb, caddr_t);
#elif defined(__SOLARIS__)
	return ((netskb_t*)mp)->b_rptr;	
#elif defined(__WINDOWS__)
	return dev_skb_data(skb);
#else
# error "wan_skb_data() function is not supported yet!"
#endif
}

/*
** wan_skb_tail() - 
**		Returns pointer to data.
*/
static __inline unsigned char* wan_skb_tail(void* skb)
{
#if defined(__LINUX__)
	return __wan_skb_tail_pointer(skb);
#elif defined(__NetBSD__) || defined(__FreeBSD__) || defined(__OpenBSD__)
	return mtod((struct mbuf*)skb, caddr_t) + ((struct mbuf*)skb)->m_len;
#elif defined(__SOLARIS__)
	return ((netskb_t*)mp)->b_wptr;
#elif defined(__WINDOWS__)
	return dev_skb_tail(skb);
#else
# error "wan_skb_tail() function is not supported yet!"
#endif
}

/*
** wan_skb_append() - 
**		Returns pointer to data.
*/
static __inline void wan_skb_append(void* skbprev, void *skb, void *list)
{
#if defined(__LINUX__)
# if  (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,14))
	skb_append(skbprev,skb);
# else
	skb_append(skbprev,skb,list);
# endif
#elif defined(__NetBSD__) || defined(__FreeBSD__) || defined(__OpenBSD__)
	m_cat (skbprev, skb);
#elif defined(__WINDOWS__)
# if WP_USE_INTERLOCKED_LIST_FUNCTIONS
	/* Interlocked function do NOT allow to insert in the middle
	 * of list. */
	skb_queue_tail(skb, list);
# else
	skb_append(skbprev,skb);
# endif
#else
# error "wan_skb_append() function is not supported yet!"
#endif
}



/*
** wan_skb_len() - 
**		Returns current kernel buffer length.
*/
static __inline int wan_skb_len(void* skb)
{
#if defined(__LINUX__)
	return ((struct sk_buff*)skb)->len;
#elif defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__)
	return ((struct mbuf*)skb)->m_len;
#elif defined(__SOLARIS__)
	mblk_t*  tmp = skb;
    	int      len = 0;
    	while(tmp) {
        	len += (tmp->b_wptr - tmp->b_rptr);
		tmp = tmp->b_cont;
    	}
    	return len;
#elif defined(__WINDOWS__)
	return dev_skb_len(skb);
#else
# error "wan_skb_len() function is not supported yet!"
#endif
}

/*
** wan_skb_free() - 
** Free kernel memory buffer.
*/

#ifdef WAN_DEBUG_MEM
#define wan_skb_free(ptr) __wan_skb_free(ptr,(const char*)__FUNCTION__,(const int)__LINE__)
static __inline void __wan_skb_free(void* skb, const char *func_name, const int line)
#else
static __inline void wan_skb_free(void* skb)
#endif
{

#if defined(WAN_DEBUG_MEM)
	if (sdla_memdbg_pull(skb, func_name, line) != 0) {
		DEBUG_EVENT("%s: %d: %s Invalid Free\n",func_name,line,__FUNCTION__);
	}
#endif

#if defined(__LINUX__)
	dev_kfree_skb_any(skb);
#elif defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__)
	m_freem(skb);
#elif defined(__SOLARIS__)
	freemsg(skb);
#elif defined(__WINDOWS__)
	dev_kfree_skb_any(skb);
#else
# error "wan_skb_free() function is not supported yet!"
#endif
}

/*
** wan_skb_set_mark() - 
**		Set mark for skb.
*/
static __inline void wan_skb_set_mark(void* pskb)
{
#if defined(__LINUX__)
	return;
#elif defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__)
	((netskb_t*)pskb)->m_flags |= WAN_MFLAG_PRV;
#endif
	return;
}

/*
** wan_skb_clear_mark() - 
**		Clear mark from skb.
*/
static __inline void wan_skb_clear_mark(void* pskb)
{
#if defined(__LINUX__)
	return;
#elif defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__)
	((netskb_t*)pskb)->m_flags &= ~WAN_MFLAG_PRV;
#endif
	return;
}

/*
** wan_skb_mark() - 
**		Return 1 if mark is set, otherwise 0.
*/
static __inline int wan_skb_mark(void* pskb)
{
#if defined(__LINUX__)
	return 0;
#elif defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__)
	return (((netskb_t*)pskb)->m_flags & WAN_MFLAG_PRV);
#endif
	return 0;
}

/*
** wan_skb_alloc() - 
**		Allocate kernel buffer with len.
*/
#if defined(WAN_DEBUG_MEM)
#define wan_skb_alloc(size) __wan_skb_alloc(size,(const char*)__FUNCTION__,(const int)__LINE__)
static __inline void* __wan_skb_alloc(unsigned int len, const char *func_name, const int line)
#else
static __inline void* wan_skb_alloc(unsigned int len)
#endif
{
#if defined(__LINUX__) || defined(__WINDOWS__)

	struct sk_buff *skb=dev_alloc_skb(len+64);
	if (skb){
		skb_reserve(skb,64);
#if defined(WAN_DEBUG_MEM)
		sdla_memdbg_push(skb, func_name,line,skb->truesize);
#endif
	}
	return (void*)skb;

#elif defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__)
	struct mbuf	*nm = NULL;

	nm = m_getcl(M_DONTWAIT,MT_DATA,M_PKTHDR);
	if (nm != NULL){
		nm->m_data += 64;
		wan_skb_set_mark(nm);
# if defined(WAN_DEBUG_MEM)
		sdla_memdbg_push(nm, func_name,line,sizeof(struct mbuf));
# endif
	}
	return nm;
#elif defined (__SOLARIS__)
	mblk_t *mp=allocb(ROUNDUP(len+64, IOC_LINESIZE), BPRI_MED);
	if (mp){
		caddr_t ptr= (caddr_t) ROUNDUP((long)mp->b_rptr, 1);
		mp->b_rptr=(uchar_t *)ptr+64;
	}
	return mp;
#else
# error "wan_skb_alloc() function is not supported yet!"
#endif
}


#if defined(WAN_DEBUG_MEM)
#define wan_skb_kalloc(size) __wan_skb_kalloc(size,(const char*)__FUNCTION__,(const int)__LINE__)
static __inline void* __wan_skb_kalloc(unsigned int len, const char *func_name, const int line)
#else
static __inline void* wan_skb_kalloc(unsigned int len)
#endif
{
#if defined(__LINUX__) || defined(__WINDOWS__)
	struct sk_buff *skb=__dev_alloc_skb(len+64,GFP_KERNEL);
	if (skb){
		skb_reserve(skb,64);
#if defined(WAN_DEBUG_MEM)
		sdla_memdbg_push(skb, func_name,line,skb->truesize);
#endif
	}
	return (void*)skb;

#elif defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__)
	struct mbuf	*nm = NULL;

	nm = m_getcl(M_DONTWAIT,MT_DATA,M_PKTHDR);
	if (nm != NULL){
		nm->m_data += 64;
		wan_skb_set_mark(nm);
# if defined(WAN_DEBUG_MEM)
		sdla_memdbg_push(nm, func_name,line,sizeof(struct mbuf));
# endif
	}
	return nm;
#elif defined (__SOLARIS__)
	mblk_t *mp=allocb(ROUNDUP(len+64, IOC_LINESIZE), BPRI_MED);
	if (mp){
		caddr_t ptr= (caddr_t) ROUNDUP((long)mp->b_rptr, 1);
		mp->b_rptr=(uchar_t *)ptr+64;
	}
	return mp;
#else
# error "wan_skb_kalloc() function is not supported yet!"
#endif
}


/*
** wan_skb_set_dev() - 
**		Set device point.
*/
static __inline void wan_skb_set_dev(void* pskb, void* dev)
{
#if defined(__LINUX__)
	struct sk_buff *skb = (struct sk_buff*)pskb;
	if (skb){
		skb->dev = dev;
	}
#elif defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__)
	netskb_t*	m = (netskb_t*)pskb;
	WAN_SKBCRITASSERT(m);
	if (m){
		m->m_pkthdr.rcvif = dev;
	}
#elif defined(__WINDOWS__)
	struct sk_buff *skb = (struct sk_buff*)pskb;
	skb->dev = dev;
#else
# error "wan_skb_set_dev() function is not supported yet!"
#endif
}

static __inline void wan_skb_set_protocol(void* pskb, unsigned int protocol)
{
#if defined(__LINUX__)
	struct sk_buff *skb = (struct sk_buff*)pskb;
	if (skb){
		skb->protocol = htons((u16)protocol);
	}
#elif defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__)
	struct mbuf	*mbuf = (struct mbuf*)pskb;
	if (protocol == ETH_P_IPX){
		mbuf->m_flags |= WAN_MFLAG_IPX;
	}
#elif defined(__WINDOWS__)
	struct sk_buff *skb = (struct sk_buff*)pskb;
	/* RtlUshortByteSwap converts a USHORT from 
	little-endian to big-endian, and vice versa. */
	skb->protocol = RtlUshortByteSwap(protocol);
#else
# warning "wan_skb_set_protocol() function is not supported yet!"
#endif
}

static __inline void wan_skb_set_raw(void* pskb)
{
#if defined(__LINUX__)
	struct sk_buff *skb = (struct sk_buff*)pskb;
	if (skb){
		__wan_skb_reset_mac_header(skb);
		__wan_skb_reset_network_header(skb);
	}
#elif defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__)
#elif defined(__WINDOWS__)
	struct sk_buff *skb = (struct sk_buff*)pskb;
	/* skb->mac.raw = skb->data; */
	/*FUNC_NOT_IMPL();*/
#else
# warning "wan_skb_set_raw() function is not supported yet!"
#endif
}

static __inline void wan_skb_reset_mac_header(void* pskb)
{
#if defined(__LINUX__)
	struct sk_buff *skb = (struct sk_buff*)pskb;
	if (skb){
		__wan_skb_reset_mac_header(skb);
	}
#elif defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__)
#elif defined(__WINDOWS__)
	FUNC_NOT_IMPL();
#else
# warning "wan_skb_reset_mac_hdr() function is not supported yet!"
#endif
}

static __inline void wan_skb_reset_network_header(void* pskb)
{
#if defined(__LINUX__)
	struct sk_buff *skb = (struct sk_buff*)pskb;
	if (skb){
		__wan_skb_reset_network_header(skb);
	}
#elif defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__)
#elif defined(__WINDOWS__)
	FUNC_NOT_IMPL();
#else
# warning "wan_skb_reset_network_hdr() function is not supported yet!"
#endif
}

/*
** wan_skb_set_csum() - 
**		Set checksum.
*/
static __inline void wan_skb_set_csum(void* skb, unsigned int csum)
{
#if defined(__LINUX__)
	struct sk_buff *sk = (struct sk_buff*)skb;
	if (sk){
		sk->csum = csum;
	}
#elif defined(__OpenBSD__)
	netskb_t*	m = (netskb_t*)skb;
	if (m){
# if (OpenBSD >= 200511)
		m->m_pkthdr.csum_flags = csum;
# else
		m->m_pkthdr.csum = csum;
# endif
	}
#elif defined(__NetBSD__) || defined(__FreeBSD__)
	netskb_t*	m = (netskb_t*)skb;
	WAN_SKBCRITASSERT(m);
	if (m){
		m->m_pkthdr.csum_data = csum;
	}
#elif defined(__WINDOWS__)
	struct sk_buff *sk = (struct sk_buff*)skb;
	if (sk){
		sk->csum = csum;
	}
#else
# error "wan_skb_set_csum() function is not supported yet!"
#endif
}

/*
** wan_skb_csum() - 
**		Return checksum value.
*/
static __inline unsigned int wan_skb_csum(void* skb)
{
#if defined(__LINUX__)
	struct sk_buff *sk = (struct sk_buff*)skb;
	return (sk) ? sk->csum : 0;
#elif defined(__NetBSD__) || defined(__FreeBSD__)
	netskb_t*	m = (netskb_t*)skb;
	WAN_SKBCRITASSERT(m);
	return (m) ? m->m_pkthdr.csum_data : 0;
#elif defined(__OpenBSD__)
	netskb_t*	m = (netskb_t*)skb;
# if (OpenBSD >= 200511)
	return (m) ? m->m_pkthdr.csum_flags : 0;
# else
	return (m) ? m->m_pkthdr.csum : 0;
# endif
#elif defined(__WINDOWS__)
	struct sk_buff *sk = (struct sk_buff*)skb;
	return (sk) ? sk->csum : 0;
#else
# error "wan_skb_csum() function is not supported yet!"
#endif
}

/*
** wan_skb_check() - 
**		Check if packet consists from one skb block.
*/
static __inline int wan_skb_check(void* skb)
{
#if defined(__LINUX__)
	return 0;
#elif defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__)
	netskb_t*	m = (netskb_t*)skb;
	WAN_SKBCRITASSERT(m);
	if (m->m_pkthdr.len != m->m_len){
		return 1;
	}
	return 0;
#elif defined(__WINDOWS__)
	return 0;
#else
# error "wan_skb_check() function is not supported yet!"
#endif
}

/*
** wan_skb_reserve() - 
**		Reserve extra bytes before data
*/
static __inline void wan_skb_reserve(void* skb, unsigned int len)
{
#if defined(__LINUX__)
	skb_reserve(skb, len);
#elif defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__)
	struct mbuf	*m = (struct mbuf*)skb;

	m->m_data += len;
#elif defined(__WINDOWS__)
	skb_reserve(skb, len);
#else
# error "wan_skb_reserve() function is not supported yet!"
#endif
}

/*
** wan_skb_copyback() - 
** 	Copy data from a buffer back into the indicated mbuf chain,
** 	starting "off" bytes from the beginning, extending the mbuf
** 	chain if necessary.
*/
static __inline void wan_skb_copyback(void* skb, int off, int len, caddr_t cp)
{
#if defined(__LINUX__)
	struct sk_buff* sk = (struct sk_buff*)skb;
	unsigned char* data = NULL;
	if (off == wan_skb_len(skb)){
		if (sk->tail + len > sk->end){	
			DEBUG_EVENT(
			"%s:%d: Internal Error (off=%d,len=%d,skb_len=%d)!\n",
				__FUNCTION__,__LINE__,
				off, len, wan_skb_len(skb));
			return;
		}else{
			data = skb_put(skb, len);
			memcpy(data, cp, len);
		}
	}else{
		if (off + len > wan_skb_len(skb)){
			data = skb_put(skb, len);
			memcpy(data + off, cp, len);
			skb_trim(skb, off + len);
		}
	}
#elif defined(__FreeBSD__)
	struct mbuf*	m = (struct mbuf*)skb;
	caddr_t		data = mtod(m, caddr_t);

	WAN_SKBCRITASSERT(m);
	if (off + len > m->m_ext.ext_size){
		panic("%s:%d: Buffer is too small (%d:%d;%d)!",
				__FUNCTION__,__LINE__,
				off,len,m->m_ext.ext_size);
		return;
	}
	bcopy(cp, &data[off], len); 
	m->m_len 	= off + len;
	m->m_pkthdr.len = off + len;
#elif defined(__OpenBSD__) || defined(__NetBSD__)
	struct mbuf*	m = (struct mbuf*)skb;
	caddr_t		data = mtod(m, caddr_t);

	bcopy(cp, &data[off], len); 
	m->m_len 	= off + len;
	m->m_pkthdr.len = off + len;
#elif defined(__WINDOWS__)
	struct sk_buff* sk = (struct sk_buff*)skb;
	unsigned char* data = NULL;
	if (off == wan_skb_len(skb)){
		if (sk->tail + len > sk->end){	
			DEBUG_EVENT(
			"%s:%d: Internal Error (off=%d,len=%d,skb_len=%d)!\n",
				__FUNCTION__,__LINE__,
				off, len, wan_skb_len(skb));
			return;
		}else{
			data = skb_put(skb, len);
			memcpy(data, cp, len);
		}
	}else{
		if (off + len > wan_skb_len(skb)){
			data = skb_put(skb, len);
			memcpy(data + off, cp, len);
			skb_trim(skb, off + len);
		}
	}
#else
# error "wan_skb_copyback() function is not supported yet!"
#endif
}

/*
** wan_skb_copyback_user() - 
** 	Copy data from a buffer back into the indicated mbuf chain,
** 	starting "off" bytes from the beginning, extending the mbuf
** 	chain if necessary.
**      Data being copied is coming from user space, thus we must
**      use a special function to copy it into kernel space.
*/
static __inline int wan_skb_copyback_user(void* skb, int off, int len, caddr_t cp)
{
#if defined(__LINUX__)
	struct sk_buff* sk = (struct sk_buff*)skb;
	unsigned char* data = NULL;
	if (off == wan_skb_len(skb)){
		if (__wan_skb_tail_pointer(sk) + len > __wan_skb_end_pointer(sk)){	
			DEBUG_EVENT(
			"%s:%d: Internal Error (off=%d,len=%d,skb_len=%d)!\n",
					__FUNCTION__,__LINE__,
					off, len, wan_skb_len(skb));
			return -EINVAL;
		}else{
			data = skb_put(skb, len);
			if (WAN_COPY_FROM_USER(data, cp, len)){
				DEBUG_EVENT(
				"%s:%d: Internal Error (off=%d,len=%d,skb_len=%d)!\n",
					__FUNCTION__,__LINE__,
					off, len, wan_skb_len(skb));
				return -EFAULT;
			}
		}
	}else{
		if (off + len > wan_skb_len(skb)){
			data = skb_put(skb, len);
			if (WAN_COPY_FROM_USER(data+off, cp, len)){
				DEBUG_EVENT(
				"%s:%d: Internal Error (off=%d,len=%d,skb_len=%d)!\n",
					__FUNCTION__,__LINE__,
					off, len, wan_skb_len(skb));
				return -EFAULT;
			}
			skb_trim(skb, off + len);
		}
	}
	return 0;
#elif defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__)
	struct mbuf*	m = (struct mbuf*)skb;
	caddr_t		data = mtod(m, caddr_t);

	WAN_SKBCRITASSERT(m);
	if (off + len > m->m_ext.ext_size){
		panic("%s:%d: Buffer is too small (%d:%d:%d)!\n",
				__FUNCTION__,__LINE__, off,len,m->m_ext.ext_size);
		return 0;
	}
	WAN_COPY_FROM_USER(cp, &data[off], len);
	m->m_len 	= off + len;
	m->m_pkthdr.len = off + len;
#elif defined(__WINDOWS__)
	FUNC_NOT_IMPL();
#else
# error "wan_skb_copyback_user() function is not supported yet!"
#endif
	return 0;
}


/*
** wan_skb_copyback() - 
**	Copy data from an mbuf chain starting "off" bytes from the beginning,
**	continuing for "len" bytes, into the indicated buffer.
*/
static __inline void wan_skb_copydata(void* skb, int off, int len, caddr_t cp)
{
#if defined(__LINUX__)
	if (off + len > wan_skb_len(skb)){
		DEBUG_EVENT(
		"%s:%d: Internal error (off=%d, len=%d, skb_len=%d)!\n",
					__FUNCTION__,__LINE__,
					off, len, wan_skb_len(skb));
		return;
	}
	memcpy(cp, wan_skb_data(skb), len);
#elif defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__)
# if (__FreeBSD_version >= 502000)
	m_copydata(skb, off, len, cp);
# else
	caddr_t		data = mtod((struct mbuf*)skb, caddr_t);

	bcopy(cp, &data[off], len); 
# endif
#elif defined(__SOLARIS__)
	mblk_t* tmp = (mblk_t*)skb;
	unsigned char* ptr = NULL;
    	unsigned i = 0, num = 0;
	while(tmp != NULL) {
		ptr = tmp->b_rptr;
		num = tmp->b_wptr - tmp->b_rptr; 
		bcopy(ptr, &cp[i], num);
		i += num;
		tmp = tmp->b_cont;
    	}
#elif defined(__WINDOWS__)
	if (off + len > wan_skb_len(skb)){
		DEBUG_EVENT(
		"%s:%d: Internal error (off=%d, len=%d, skb_len=%d)!\n",
					__FUNCTION__,__LINE__,
					off, len, wan_skb_len(skb));
		return;
	}
	memcpy(cp, wan_skb_data(skb), len);
#else
# error "wan_skb_copydata() function is not supported yet!"
#endif
}

/*
** wan_skb_copy()  
*/
static __inline void * wan_skb_copy(void *skb)
{
#if defined(__LINUX__)
	return skb_copy(skb,GFP_ATOMIC);
#elif defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__)
	return m_copym(skb, 0, wan_skb_len(skb), M_DONTWAIT);
#elif defined(__WINDOWS__)
	return skb_copy(skb,GFP_ATOMIC);
#else
# error "wan_skb_copy() function is not supported yet"
#endif
	
}

/*
** wan_skb_clone()  
*/
static __inline void * wan_skb_clone(void *skb)
{
#if defined(__LINUX__)
	return skb_clone(skb,GFP_ATOMIC);
#elif defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__)
	return m_copym(skb, 0, wan_skb_len(skb), M_DONTWAIT);
#elif defined(__WINDOWS__)
	return skb_clone(skb,GFP_ATOMIC);
#else
# error "wan_skb_clone() function is not supported yet"
#endif
	
}


/*
** wan_skb2buffer() - 
**		Correct skb block.
*/
static __inline int wan_skb2buffer(void** skb)
{
#if defined(__LINUX__)
	return 0;
#elif defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__)
	netskb_t	*m = (netskb_t*)(*skb);
	netskb_t	*nm = NULL;
	int		s;

	s = splnet();
	nm = wan_skb_alloc(1);
	if (nm){
		struct mbuf	*tmp = m;
		char	*buffer = nm->m_data;

		for( ; tmp; tmp = tmp->m_next) {
			bcopy(mtod(tmp, caddr_t), buffer, tmp->m_len);
			buffer += tmp->m_len;
			nm->m_len += tmp->m_len;
		}
		wan_skb_free(m);
		*skb = nm;
		splx(s);
		return 0;
	}
	splx(s);
	return -EINVAL;
#elif defined(__WINDOWS__)
	return 0;
#else
# error "wan_skb_correct() function is not supported yet!"
#endif
}

/*
** wan_skb_pull() - 
**
*/
static __inline unsigned char* wan_skb_pull(void* skb, int len)
{
#if defined(__LINUX__)
	return skb_pull((struct sk_buff*)skb, len);
#elif defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__)
	struct mbuf	*m = (struct mbuf*)skb;

	WAN_SKBCRITASSERT(m);
	if (len > m->m_ext.ext_size){
		panic("%s:%d: Buffer is too small (%d:%d)!\n",
				__FUNCTION__,__LINE__,
				len,m->m_ext.ext_size);
		return NULL;
	}
	m_adj(skb, len);
#if 0
	struct mbuf*	m = (struct mbuf*)skb;
	m->m_data += len;
	m->m_pkthdr.len -= len;
	m->m_len 	= m->m_pkthdr.len;
#endif
	return wan_skb_data(skb);
#elif defined(__WINDOWS__)
	return skb_pull((struct sk_buff*)skb, len);
#else
# error "wan_skb_pull() function is not supported yet!"
#endif
}

/*
** wan_skb_put() - 
**
*/
static __inline unsigned char* wan_skb_put(void* skb, int len)
{
#if defined(__LINUX__)
	return skb_put((struct sk_buff*)skb, len);
#elif defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__)
	struct mbuf*	m = (struct mbuf*)skb;
	int		org_len = wan_skb_len(skb);
	unsigned char*	data = wan_skb_data(skb);

	WAN_SKBCRITASSERT(m);
	if (org_len + len > m->m_ext.ext_size){
		panic("%s:%d: Buffer is too small (%d:%d:%d)!\n",
				__FUNCTION__,__LINE__,
				org_len,len,m->m_ext.ext_size);
		return NULL;
	}

	m->m_len 	= org_len + len;
	m->m_pkthdr.len = org_len + len;
/*Alex Sep27,2004 last tail but not data pointer	return wan_skb_data(skb);*/
	return data + org_len;
#elif defined(__SOLARIS__)
	mblk_t mp=(mblk_t*)skb;	
	unsigned char *wptr=mp->b_wptr;
	mp->b_wptr += len;
	return mp->b_wptr;
#elif defined(__WINDOWS__)
	return skb_put((struct sk_buff*)skb, len);
#else
# error "wan_skb_put() function is not supported yet!"
#endif
}

/*
** wan_skb_push() - 
**
*/
static __inline unsigned char* wan_skb_push(void* skb, int len)
{
#if defined(__LINUX__)
	return skb_push((struct sk_buff*)skb, len);
#elif defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__)
	struct mbuf	*m = (struct mbuf*)skb;
	int		org_len = wan_skb_len(skb);

	WAN_SKBCRITASSERT(m);
	if (m->m_flags & M_EXT){
		if ((m->m_data - len)  < m->m_ext.ext_buf){
			panic("%s:%d: Can't push %d bytes (%p:%p)!\n", 
					__FUNCTION__,__LINE__,
					len,m->m_data,m->m_ext.ext_buf);
			return NULL;
		}
	}else{
		if ((m->m_data - len) < m->m_pktdat){
			panic("%s:%d: Can't push %d bytes (%p:%p)!\n", 
					__FUNCTION__,__LINE__,
					len,m->m_data,m->m_pktdat);
			return NULL;
		}
	}
	m->m_data	-= len;
	m->m_len 	= org_len + len;
	m->m_pkthdr.len = org_len + len;
	return wan_skb_data(skb);
#elif defined(__WINDOWS__)
	return skb_push((struct sk_buff*)skb, len);
#else
# error "wan_skb_push() function is not supported yet!"
#endif
}


/*
** wan_skb_tailroom() - Tail room
**			
** 
*/
static __inline int wan_skb_tailroom(void* skb)
{
#if defined(__LINUX__)
	return skb_tailroom(skb);
#elif defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__)
	struct mbuf*	m = (struct mbuf*)skb;

	WAN_SKBCRITASSERT(m);
	if (m->m_flags & M_EXT){
		return (m->m_ext.ext_size/*MCLBYTES*/ - m->m_len);
	}
	return (MHLEN - m->m_len);
#elif defined(__WINDOWS__)
	return skb_tailroom(skb);
#else
# error "wan_skb_tailroom() function is not supported yet!"
#endif
}

/*
** wan_skb_tailroom() - Head room
**			
** 
*/
static __inline int wan_skb_headroom(void* skb)
{
#if defined(__LINUX__)
	return skb_headroom(skb);
#elif defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__)
	struct mbuf*	m = (struct mbuf*)skb;

	WAN_SKBCRITASSERT(m);
	if (m->m_flags & M_EXT){
		return (m->m_data - m->m_ext.ext_buf);
	}
	return (m->m_data - m->m_pktdat);
#elif defined(__WINDOWS__)
	return skb_headroom(skb);
#else
# error "wan_skb_headroom() function is not supported yet!"
#endif
}



/*
** wan_skb_trim() - Trim from tail
**			
** 
*/
static __inline void wan_skb_trim(void* skb, unsigned int len)
{
#if defined(__LINUX__)
	skb_trim(skb, len);
#elif defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__)
	struct mbuf*	m = (struct mbuf*)skb;

	WAN_SKBCRITASSERT(m);
	m->m_pkthdr.len	= len;
	m->m_len	= m->m_pkthdr.len;
#elif defined(__WINDOWS__)
	skb_trim(skb, len);
#else
# error "wan_skb_trim() function is not supported yet!"
#endif
}

/*
** wan_skb_init() - Setup skb data ptr
**			
** 
*/
static __inline void wan_skb_init(void* pskb, unsigned int len)
{
#if defined(__LINUX__)
	struct sk_buff* skb = (struct sk_buff*)pskb;
	skb->data = skb->head + len;
	__wan_skb_reset_tail_pointer(skb);
	skb->len  = 0;
	skb->data_len = 0;   

#elif defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__)
	struct mbuf*	m = (struct mbuf*)pskb;

	WAN_SKBCRITASSERT(m);
	if (m->m_ext.ext_buf == NULL){
		panic("wan_skb_init: Invalid mbuf ext_buf pointer!\n");
		return;
	}
	m->m_data	= m->m_ext.ext_buf + len;
	m->m_pkthdr.len	= 0;		//was len
	m->m_len		= 0;	//was len
#elif defined(__WINDOWS__)
	struct sk_buff* skb = (struct sk_buff*)pskb;
	skb->data = skb->head + len;
	skb->tail = skb->data;
	skb->data_len = 0;   
#else
# error "wan_skb_init() function is not supported yet!"
#endif
}

static __inline int wan_skb_print(void* skb)
{
#if defined(__LINUX__)
	int len	      	    = wan_skb_len(skb);
	unsigned char *data = wan_skb_data(skb);
	int i;

	DEBUG_EVENT("DBG Packet %d bytes: ",len);
	for(i=0;i<len;i++){
		DEBUG_EVENT("%02X ", data[i]);
	} 
	DEBUG_EVENT("\n");

#elif defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__)
	netskb_t	*m = (netskb_t*)skb;
	int 		len = m->m_pkthdr.len, i;
	unsigned char	*data = wan_skb_data(skb);

	DEBUG_EVENT("Packet %d bytes: ", len);
	while(m){
		if (m->m_type & M_PKTHDR)
			DEBUG_EVENT("M_PKTHDR flag set (%d)!\n",
				m->m_pkthdr.len);
		if (m->m_type & M_EXT)
			DEBUG_EVENT("M_EXT flag set (%d)!\n",
				m->m_pkthdr.len);
		data = mtod(m, unsigned char*);	
		len = wan_skb_len(m);
		DEBUG_EVENT("mbuf chain %d bytes: ", len);
		for(i=0;i<len;i++){
			DEBUG_EVENT("%02X ", data[i]);
		} 
		DEBUG_EVENT("\n");
		m = m->m_next;
	}
#elif defined(__WINDOWS__)
	dev_print_skb(skb);
#endif
	return 0;
}

extern unsigned char wp_brt[256]; 
static __inline void wan_skb_reverse(void *skb)
{
	unsigned char *data = wan_skb_data(skb);
	int len = wan_skb_len(skb);
	int i;
	for (i=0; i < len; i++){
		data[i]=wp_brt[data[i]];
	}
}

/*
** wan_skb_unlink() - Unlink skb from the list
**			
** 
*/
static __inline void wan_skb_unlink(void* pskb)
{
#if defined(__LINUX__)
# if  (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,13))
	skb_unlink((struct sk_buff*)pskb);
# endif
#elif defined(__NetBSD__) || defined(__FreeBSD__) || defined(__OpenBSD__)
#elif defined(__WINDOWS__)
#else
# error "wan_skb_unlink() function is not supported yet!"
#endif
}


/*
** wan_skb_queue_init() - 
**
*/
static __inline void wan_skb_queue_init(void *list)
{
#if defined(__LINUX__)
	skb_queue_head_init(list);
#elif defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__)
	WAN_IFQ_INIT((wan_skb_queue_t*)list, IFQ_MAXLEN);
#elif defined(__WINDOWS__)
	skb_queue_head_init(list);
#else
# error "wan_skb_queue_init() function is not supported yet!"
#endif
}

/*
** wan_skb_queue_purge() - 
**
*/

static __inline void wan_skb_queue_purge(void *list)
{
#if defined(__LINUX__) || defined(__WINDOWS__)
	struct sk_buff *skb;
	while ((skb=skb_dequeue(list))!=NULL){
		wan_skb_free(skb);
	}
#elif defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__)
	WAN_IFQ_PURGE(((wan_skb_queue_t*)list));
#else
# error "wan_skb_queue_purge() function is not supported yet!"
#endif
}

/*
** wan_skb_queue_tail() - 
**
*/
static __inline void wan_skb_queue_tail(void* list, void* newsk)
{
#if defined(__LINUX__)
	skb_queue_tail(list, newsk);
#elif defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__)
	IF_ENQUEUE((wan_skb_queue_t*)list, (struct mbuf*)newsk);
#elif defined(__WINDOWS__)
	skb_queue_tail(list, newsk);
#else
# error "wan_skb_queue_tail() function is not supported yet!"
#endif
}

/*
** wan_skb_queue_head() - 
**
*/
static __inline void wan_skb_queue_head(void* list, void* newsk)
{
#if defined(__LINUX__)
	skb_queue_head(list, newsk);
#elif defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__)
	IF_PREPEND((wan_skb_queue_t*)list, (struct mbuf*)newsk);
#elif defined(__WINDOWS__)
	skb_queue_head(list, newsk);
#endif
}


/*
** wan_skb_queue_len() - 
**
*/
static __inline int wan_skb_queue_len(void* list)
{
#if defined(__LINUX__)
	return skb_queue_len(list);
#elif defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__)
	wan_skb_queue_t* ifqueue = (wan_skb_queue_t*)list;
	return WAN_IFQ_LEN(ifqueue);
#elif defined(__WINDOWS__)
	return skb_queue_len(list);
#else
# error "wan_skb_queue_len() function is not supported yet!"
#endif
}


/*
** wan_skb_dequeue() - 
**
*/
static __inline void *wan_skb_dequeue(void* list)
{
#if defined(__LINUX__)
	return skb_dequeue(list);
#elif defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__)
	netskb_t*	newsk = NULL;
	
	IFQ_DEQUEUE((wan_skb_queue_t*)list, newsk);
	return (void*)newsk;
#elif defined(__WINDOWS__)
	return skb_dequeue(list);
#else
# error "wan_skb_dequeue() function is not supported yet!"
#endif

}


/*
**************************************************************************
**			 WANPIPE NETWORK INTERFACE 			**
**************************************************************************
*/



static __inline int wan_netif_init(netdevice_t* dev, char* ifname)
{
#if defined(__FreeBSD__)
	int	ifunit = 0; 
	int	prefix_len = 0, len = 0;
	int	base = 0;
        while((ifname[len] != '\0') && (len <= strlen(ifname))){
		if ((ifname[len] >= '0') && (ifname[len] <= '9')){
			if (!base){
				int i=0;
				base = 1;
				for(i=0;i<strlen(ifname)-len-1;i++) 
					base = base * 10;
			}
			ifunit += ((ifname[len] - '0') * base);
			base = base / 10;
		}else{
			prefix_len++;
		}
		len++;
	}
# if (__FreeBSD_version >= 502000)
	if_initname(dev, ifname, IF_DUNIT_NONE);
# else
	dev->if_unit	= ifunit;
	if (dev->if_name == NULL){
		dev->if_name = wan_malloc(prefix_len+1);
		if (dev->if_name == NULL){
			return -ENOMEM;
		}
	}
	memcpy(dev->if_name, ifname, prefix_len);
	dev->if_name[prefix_len] = '\0';
# endif
	WAN_IFQ_SET_MAXLEN(&dev->if_snd, ifqmaxlen);
#elif defined(__OpenBSD__) || defined(__NetBSD__)
	if (strlen(ifname) >= WAN_IFNAME_SZ){
		return -ENOMEM;
	}
    	bcopy(ifname, dev->if_xname, strlen(ifname));
	WAN_IFQ_SET_MAXLEN(&dev->if_snd, IFQ_MAXLEN);
#elif defined(__LINUX__)
# if  (LINUX_VERSION_CODE < KERNEL_VERSION(2,3,43))
	dev->name = ifname;
# else	
	strcpy(dev->name, ifname);
# endif
#elif defined(__WINDOWS__)
	if(ifname){
		strncpy(dev->name, ifname, WAN_IFNAME_SZ);
	}
#else
# error "wan_netif_init() function is not supported yet!"
#endif
	return 0;
}

static __inline int wan_netif_del(netdevice_t* dev)
{
	WAN_ASSERT(dev == NULL);
#if defined(__FreeBSD__)
	dev->if_init = NULL;
# if (__FreeBSD_version >= 502000)
	dev->if_dname = NULL;
# elif (__FreeBSD_version > 400000)
	/* Free interface name (only for FreeBSD-4.0) */
	free(dev->if_name, M_DEVBUF);
# endif
#elif defined(__OpenBSD__) || defined(__NetBSD__)
	/* Do nothing */
#elif defined(__LINUX__)
	/* Do nothing */
#elif defined(__WINDOWS__)
	/* Do nothing */
#else
# error "wan_netif_del() function is not supported yet!"
#endif
	return 0;
}


#if defined(__LINUX__)
static __inline void wan_netif_fake_init(netdevice_t *d)
{
#ifdef LINUX_FEAT_2624
	d->header_ops = NULL;
#else
	d->hard_header	= NULL; 
	d->rebuild_header = NULL;
	d->header_cache_update = NULL;
	d->hard_header_cache = NULL;
#endif

	return;
}
#endif
	
static __inline void*
wan_netif_alloc(unsigned char *devname, int ifType, int *err)
{
#if defined(__LINUX__)
# if defined(LINUX_2_6)
#  if  (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,4))
	return __dev_alloc(devname, err);
#  else
	return alloc_netdev(0,devname,wan_netif_fake_init);	
# endif
# elif defined(LINUX_2_4)
	netdevice_t *dev=wan_malloc(sizeof(netdevice_t));
	*err=0;

	if (!dev){
		*err=-ENOMEM;
	}
	memset(dev, 0, sizeof(netdevice_t));
	
	strncpy(dev->name,devname,30);
	return dev;	
# else	
	netdevice_t *dev=wan_malloc(sizeof(netdevice_t));
	*err=0;

	if (!dev){
		*err=-ENOMEM;
	}
	memset(dev, 0, sizeof(netdevice_t));

	dev->name=wan_malloc(35);
	if (!dev->name){
		wan_free(dev);
		dev=NULL;
		*err=-ENOMEM;
		return NULL;
	}
	strncpy(dev->name,devname,30);
	return dev;
# endif
#elif defined(__FreeBSD__) && (__FreeBSD_version > 600000)
	struct ifnet* ifp;
	ifp = IFALLOC(ifType);
	/*ifp = wan_malloc(sizeof(struct ifnet));*/
	if (ifp == NULL){
		*err = -ENOMEM;
		return NULL;
	}
	if (devname){
		*err = wan_netif_init(ifp, devname);
		if (*err){
			wan_netif_del(ifp);
			return NULL;
		}
	}
	return ifp;
#elif defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__)
	struct ifnet* ifp;
	switch(ifType){
	case WAN_IFT_PPP:
		ifp = (struct ifnet*)wan_malloc(sizeof(struct sppp));
		if (ifp) bzero((struct sppp*)ifp, sizeof(struct sppp));
		break;
	case WAN_IFT_ETHER:
		ifp = (struct ifnet*)wan_malloc(sizeof(wan_ethercom_t));
		if (ifp) bzero(WAN_IFP2AC(ifp), sizeof(wan_ethercom_t));
		break;
	case WAN_IFT_OTHER:
	default:
		ifp = wan_malloc(sizeof(struct ifnet));
		if (ifp) bzero(ifp, sizeof(struct ifnet));
		break;
	}
	/*ifp = IFALLOC(ifType);*/
	/*ifp = wan_malloc(sizeof(struct ifnet));*/
	if (ifp == NULL){
		*err = -ENOMEM;
		return NULL;
	}
	if (devname){
		*err = wan_netif_init(ifp, devname);
		if (*err){
			wan_netif_del(ifp);
			return NULL;
		}
	}
	return ifp;
#elif defined(__WINDOWS__)
	netdevice_t *dev=wan_malloc(sizeof(netdevice_t));
	*err=0;

	if (!dev){
		*err=-ENOMEM;
		return NULL;
	}
	memset(dev, 0, sizeof(netdevice_t));
	
	strncpy(dev->name,devname,WAN_IFNAME_SZ);
	return dev;	
#else
# error "wan_netif_alloc() unsupported"
#endif

}

static __inline void wan_netif_free(netdevice_t *dev)
{

#if defined(__LINUX__)	
# if defined(LINUX_2_6)
	free_netdev(dev);
# elif defined(LINUX_2_4)
	wan_free(dev);
# else
	if (dev->name){
		wan_free(dev->name);
		dev->name=NULL;
	}
	wan_free(dev);
# endif
#elif defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__)
	wan_netif_del(dev);
	IFFREE(dev);	/*wan_free(dev);*/
#elif defined(__WINDOWS__)
	wan_free(dev);
#else
#error "wan_netif_free() not supported!"
#endif

}

	
static __inline char* wan_netif_name(netdevice_t* dev)
{
	static char ifname[WAN_IFNAME_SZ+1];
	WAN_ASSERT2(dev == NULL, NULL);
#if defined(__LINUX__)
	strcpy(ifname, dev->name);
#elif defined(__FreeBSD__)
# if (__FreeBSD_version >= 502000)
	strcpy(ifname, dev->if_xname);
# else
	sprintf(ifname, "%s%d", dev->if_name, dev->if_unit);
# endif
#elif defined(__OpenBSD__) || defined(__NetBSD__)
	sprintf(ifname, "%s", dev->if_xname);
#elif defined(__WINDOWS__)
	strcpy(ifname, dev->name);
#else
# error "wan_get_ifname() function is not supported yet!"
#endif
	return ifname;
}

static __inline void* wan_netif_priv(netdevice_t* dev)
{
	WAN_ASSERT2(dev == NULL, NULL);
#if defined(__LINUX__)
#  if  (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,28))
	return dev->priv;
#  else
	return dev->ml_priv;
#  endif
#elif defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__)
	return dev->if_softc;
#elif defined(__WINDOWS__)
	return dev->priv;
#else
# error "wan_netif_priv() function is not supported yet!"
#endif
}

static __inline void wan_netif_set_priv(netdevice_t* dev, void* priv)
{
	WAN_ASSERT1(dev == NULL);
#if defined(__LINUX__)
#  if  (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,28))
	dev->priv = priv;
#  else
	dev->ml_priv = priv;
#  endif
#elif defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__)
	dev->if_softc = priv;
#elif defined(__WINDOWS__)
	dev->priv = priv;
#else
# error "wan_netif_priv() function is not supported yet!"
#endif
	return;
}

static __inline int wan_netif_up(netdevice_t* dev)
{
	WAN_ASSERT2(dev == NULL, -EINVAL);
#if defined(__LINUX__)
	return WAN_NETIF_UP(dev);
#elif defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__)
	return WAN_NETIF_UP(dev);
#elif defined(__WINDOWS__)
	return WAN_NETIF_UP(dev);
#else
# error "wan_netif_up() function is not supported yet!"
#endif
}


static __inline unsigned int wan_netif_flags(netdevice_t* dev)
{
	WAN_ASSERT(dev == NULL);
#if defined(__LINUX__)
	return dev->flags;
#elif defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__)
	return dev->if_flags;
#elif defined(__WINDOWS__)
	FUNC_NOT_IMPL();
#else
# error "wan_netif_flags() function is not supported yet!"
#endif
}

static __inline int wan_netif_mcount(netdevice_t* dev)
{
#if defined(__LINUX__)
	return netdev_mc_count(dev);
#elif defined(__FreeBSD__)
	return dev->if_amcount;
#elif defined(__OpenBSD__) || defined(__NetBSD__)
	return 0;
#elif defined(__WINDOWS__)
	FUNC_NOT_IMPL();
#else
# error "wan_netif_mcount() function is not supported yet!"
#endif
}

static __inline int wan_netif_set_ticks(netdevice_t* dev, unsigned long ticks)
{
#if defined(__LINUX__) || defined(__WINDOWS__)
	dev->trans_start = ticks;
#elif defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__)
#else
# error "wan_netif_set_ticks() function is not supported yet!"
#endif
	return 0;
}

static __inline int wan_netif_set_mtu(netdevice_t* dev, unsigned long mtu)
{
#if defined(__LINUX__) || defined(__WINDOWS__)
#elif defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__)
	dev->if_mtu = mtu;
#else
# error "wan_netif_set_mtu() function is not supported yet!"
#endif
	return 0;
}

static __inline void
wan_bpf_report(netdevice_t* dev, void* pkt, int flag, int dir)
{

#if defined(__LINUX__)
	/* Do nothing */
#elif defined(__FreeBSD__)
	if (dev->if_bpf != NULL){ /* BF-0002 */ 
		WAN_BPF_REPORT(dev, pkt, dir);
	}
#elif defined(__OpenBSD__) || defined(__NetBSD__)
	if (dev->if_bpf != NULL){ /* BF-0002 */ 
		if (flag){
			struct mbuf m0;
			u_int32_t af = AF_INET;
			m0.m_next = pkt;
			m0.m_len = 4;
			m0.m_data = (char*)&af;
			WAN_BPF_REPORT(dev, &m0, dir);
		}else{
			WAN_BPF_REPORT(dev, pkt, dir);
		}
	}
#elif defined(__WINDOWS__)
	/* Do nothing */
	FUNC_NOT_IMPL();
#else
# error "wan_bpf_report() function is not supported yet!"
#endif
}





static __inline void wan_spin_lock_init(void *lock, char *name)
{
#if defined(__LINUX__)
	spin_lock_init(((spinlock_t*)lock));	
#elif defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__)
	/*(*(wan_smp_flag_t*)flag) = 0;*/
#if defined(SPINLOCK_OLD)
#else
	mtx_init((struct mtx*)lock, name, MTX_NETWORK_LOCK, MTX_DEF);
#endif
#elif defined(__WINDOWS__)
	spin_lock_init((wan_spinlock_t*)lock, name);	
#else
# warning "wan_spin_lock_init() function is not supported yet!"
#endif	
}

static __inline void wan_spin_lock_irq_init(void *lock, char *name)
{
#if defined(__LINUX__)
	spin_lock_init(((spinlock_t*)lock));	
#elif defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__)
	/*(*(wan_smp_flag_t*)flag) = 0;*/
#if defined(SPINLOCK_OLD)
#else
	mtx_init((struct mtx*)lock, name, MTX_NETWORK_LOCK, MTX_SPIN);
#endif
#elif defined(__WINDOWS__)
	spin_lock_init((wan_spinlock_t*)lock, name);	
#else
# warning "wan_spin_lock_init() function is not supported yet!"
#endif	
}

static __inline int wan_spin_is_locked(void *lock)
{
#if defined(__LINUX__)
	return spin_is_locked(((spinlock_t*)lock));	
#elif defined(__NetBSD__) || defined(__OpenBSD__)
	return 0;/*((*(wan_smp_flag_t*)flag) & imask[IPL_NET]);*/
#elif defined(__FreeBSD__)
# if (__FreeBSD_version > 500000)
#  if defined(SPINLOCK_OLD)
	return 0;
#  else
	return mtx_owned((struct mtx*)lock);
#  endif
# else
	return 0;/*((*(wan_smp_flag_t*)flag) & net_imask);*/
# endif
#elif defined(__WINDOWS__)
	return 0;/* always returns 'unlocked' */
#else
# warning "wan_spin_is_lock() function is not supported yet!"
#endif	
}

static __inline void wan_spin_lock_irq(void *lock, wan_smp_flag_t *flag)
{
#if defined(__LINUX__)
	spin_lock_irqsave(((spinlock_t*)lock),*flag);	
#elif defined(__FreeBSD__)
	/* Feb 10, 2005 Change splnet to splimp
	** (i think it was cause to system crash) */
#if defined(SPINLOCK_OLD)
	*flag = splhigh();	/*splimp();*/
#else
	if (!mtx_initialized((struct mtx*)lock)){
		DEBUG_EVENT(
		"INTERNAL ERROR: Spin Lock IRQ is no initialized!\n");
		return;
	}
	mtx_lock_spin((struct mtx*)lock);
#endif
#elif defined(__OpenBSD__)
	*flag = splnet();
#elif defined(__NetBSD__)
# if (__NetBSD_Version__ >= 106000200)
	*flag = splvm();
# else
	*flag = splimp();
# endif
#elif defined(__WINDOWS__)
	spin_lock_irqsave((wan_spinlock_t*)lock, flag/* irql before lock */);
#else
# warning "wan_spin_lock_irq() function is not supported yet!"
#endif	
}

static __inline void wan_spin_unlock_irq(void *lock, wan_smp_flag_t *flag)
{
#if defined(__LINUX__)
	spin_unlock_irqrestore((spinlock_t*)lock,*flag);	
#elif defined(__FreeBSD__)
#if defined(SPINLOCK_OLD)
	splx(*flag);
#else
	if (!mtx_initialized((struct mtx*)lock)){
		DEBUG_EVENT(
		"INTERNAL ERROR: Spin Lock IRQ is no initialized!\n");
		return;
	}
	mtx_unlock_spin((struct mtx*)lock);
#endif
#elif defined(__OpenBSD__) || defined(__NetBSD__)
	splx(*flag);
#elif defined(__WINDOWS__)
	spin_unlock_irqrestore((wan_spinlock_t*)lock, flag/* irql to be restored */);
#else
# warning "wan_spin_unlock_irq() function is not supported yet!"
#endif	
}

/* special locking functions for the Wanpipe Protocol Layer (LIP) */
#if defined(__WINDOWS__)
# define wplip_spin_lock_irq_init	spin_lock_init
# define wplip_spin_lock_irq		wp_spin_lock
# define wplip_spin_unlock_irq		wp_spin_unlock
#else
# define wplip_spin_lock_irq_init	wan_spin_lock_irq_init
# define wplip_spin_lock_irq		wan_spin_lock_irq
# define wplip_spin_unlock_irq		wan_spin_unlock_irq
#endif	

/* SPINLOCK functions */
static __inline int wan_spin_trylock(void *lock, wan_smp_flag_t *flag)
{
#if defined(__LINUX__)
	return spin_trylock(((spinlock_t*)lock));	
#elif defined(__FreeBSD__)
	return mtx_trylock((struct mtx*)lock);
#elif defined(__OpenBSD__)
	#warning "FIXME: Complete this code.!!!!!!!!!!!"
#elif defined(__NetBSD__)
	#warning "FIXME: Complete this code.!!!!!!!!!!!"
#elif defined(__WINDOWS__)
	return wp_spin_trylock((wan_spinlock_t*)lock, flag);
#else
# warning "wan_spin_trylock() function is not supported yet!"
#endif	
	return 0;
}

static __inline void wan_spin_lock(void *lock, wan_smp_flag_t *flag)
{
#if defined(__LINUX__)
	spin_lock(((spinlock_t*)lock));	
#elif defined(__FreeBSD__)
	/* Feb 10, 2005 Change splnet to splimp
	** (i think it was cause to system crash) */
#if defined(SPINLOCK_OLD)
	*((wan_spinlock_t*)lock) = splimp();
#else
	if (!mtx_initialized((struct mtx*)lock)){
		DEBUG_EVENT(
		"INTERNAL ERROR: Spin Lock IRQ is no initialized!\n");
		return;
	}
	mtx_lock((struct mtx*)lock);
#endif
#elif defined(__OpenBSD__)
	*((wan_spinlock_t*)lock) = splnet();
#elif defined(__NetBSD__)
# if (__NetBSD_Version__ >= 106000200)
	*((wan_spinlock_t*)lock) = splvm();
# else
	*((wan_spinlock_t*)lock) = splimp();
# endif
#elif defined(__WINDOWS__)
	wp_spin_lock((wan_spinlock_t*)lock, flag);	
#else
# warning "wan_spin_lock() function is not supported yet!"
#endif	
}

static __inline void wan_spin_unlock(void *lock,  wan_smp_flag_t *flag)
{
#if defined(__LINUX__)
	spin_unlock(((spinlock_t*)lock));	
#elif defined(__FreeBSD__)
#if defined(SPINLOCK_OLD)
	splx(*(wan_spinlock_t*)lock);
#else
	if (!mtx_initialized((struct mtx*)lock)){
		DEBUG_EVENT(
		"INTERNAL ERROR: Spin Lock IRQ is no initialized!\n");
		return;
	}
	mtx_unlock((struct mtx*)lock);
#endif
#elif defined(__OpenBSD__) || defined(__NetBSD__)
	splx(*(wan_spinlock_t*)lock);
#elif defined(__WINDOWS__)
	wp_spin_unlock((wan_spinlock_t*)lock, flag);	
#else
# warning "wan_spin_unlock() function is not supported yet!"
#endif	
}

/*==============================================================
 * MUTEX functions.
 */


static __inline void wan_mutex_lock_init(void *lock, char *name)
{
#if defined(__LINUX__)
#ifdef DEFINE_MUTEX
	mutex_init(((wan_mutexlock_t*)lock));
#else
  	spin_lock_init(((spinlock_t*)lock));
#endif
#elif defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__)
	/*(*(wan_smp_flag_t*)flag) = 0;*/
#if defined(SPINLOCK_OLD)
#else
	mtx_init((struct mtx*)lock, name, MTX_NETWORK_LOCK, MTX_DEF);
#endif
#elif defined(__WINDOWS__)
	spin_lock_init((wan_spinlock_t*)lock, name);	
#else
# warning "wan_spin_lock_init() function is not supported yet!"
#endif
}

static __inline void wan_mutex_lock(void *mutex, wan_smp_flag_t *flag /* FIXME: 'flag' should be removed */)
{
#if defined(__WINDOWS__)
	wp_mutex_lock(mutex);	
#else
#ifdef DEFINE_MUTEX
	mutex_lock(mutex);
#else
    spin_lock(((spinlock_t*)mutex));	                 
#endif
#endif	
}

static __inline void wan_mutex_unlock(void *mutex, wan_smp_flag_t *flag /* FIXME: 'flag' should be removed */)
{
#if defined(__WINDOWS__)
	wp_mutex_unlock(mutex);	
#else
#ifdef DEFINE_MUTEX
	mutex_unlock(mutex);
#else
    spin_unlock(((spinlock_t*)mutex));	                 
#endif
#endif	
}

static __inline int wan_mutex_trylock(void *mutex, wan_smp_flag_t *flag /* FIXME: 'flag' should be removed */)
{
#if defined(__WINDOWS__)
	return wp_mutex_trylock(mutex);	
#else
#ifdef DEFINE_MUTEX
	return mutex_trylock(mutex);
#else
	return spin_trylock(((spinlock_t*)mutex));	
#endif
#endif	
}


static __inline int wan_mutex_is_locked(void *lock)
{
#if defined(__LINUX__)
#ifdef DEFINE_MUTEX
	return mutex_is_locked(((wan_mutexlock_t*)lock));	
#else
	return spin_is_locked(((spinlock_t*)lock));	
#endif
#elif defined(__WINDOWS__)
	return wp_mutex_is_locked(((wan_mutexlock_t*)lock));		
#else
# warning "wan_mutex_is_lock() function is not supported yet!"
#endif	
}


#if 0
static __inline void wan_read_rw_lock(void *lock)
{
#if defined(__LINUX__)
	read_lock(((rwlock_t*)lock));
#else
# warning "wan_read_rw_lock() function is not supported yet!"
#endif
}

static __inline void wan_read_rw_unlock(void *lock)
{
#if defined(__LINUX__)
	read_unlock(((rwlock_t*)lock));
#else
# warning "wan_read_rw_unlock() function is not supported yet!"
#endif
}

static __inline void wan_write_rw_lock_irq(void *lock, unsigned long *flag)
{
#if defined(__LINUX__)
	write_lock_irqsave(((rwlock_t*)lock),flag);
#else
# warning "wan_read_rw_lock() function is not supported yet!"
#endif
}

static __inline void wan_write_rw_unlock_irq(void *lock, unsigned long *flag)
{
#if defined(__LINUX__)
	write_unlock_irqrestore(((rwlock_t*)lock),flag);
#else
# warning "wan_read_rw_unlock() function is not supported yet!"
#endif
}
#endif

#if 0
static __inline void wan_read_bus_4(void *phw, void *virt, int offset, unsigned int *value)
{
#if defined(__LINUX__)
	*value = wp_readl((unsigned char*)virt + offset); 
#else
        sdla_bus_read_4(phw,offset,value);
#endif	
}

static __inline void wan_write_bus_4(void *phw, void *virt, int offset, unsigned int value)
{
#if defined(__LINUX__)
	wp_writel(value,(u8*)virt + offset);
#else
        sdla_bus_write_4(phw,offset,value);
#endif	
}   
#endif

#if defined(__WINDOWS__)

#define WAN_IFQ_INIT(ifq, max_pkt)			wan_skb_queue_init((ifq))
#define WAN_IFQ_DESTROY(ifq)
#define WAN_IFQ_PURGE(ifq)					wan_skb_queue_purge((ifq))
#define WAN_IFQ_DMA_PURGE(ifq)				wan_skb_queue_purge(ifq)
#define WAN_IFQ_ENQUEUE(ifq, skb, arg, err)	wan_skb_queue_tail((ifq), (skb))

#if WP_USE_INTERLOCKED_LIST_FUNCTIONS
# pragma deprecated(wan_skb_append)
# pragma deprecated(wan_skb_unlink)
#endif

#endif	/* __WINDOWS__ */

#endif  /* WAN_KERNEL */
#endif	/* __WANPIPE_COMMON_H */
