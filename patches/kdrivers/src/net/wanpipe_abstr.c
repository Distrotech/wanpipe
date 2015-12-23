/*****************************************************************************
* wanpipe_abstr.c WANPIPE(tm) Kernel Abstraction Layer.
*
* Authors: 	Nenad Corbic <ncorbic@sangoma.com>
*		Alex Feldman <al.feldman@sangoma.com
*
* Copyright:	(c) 2003 Sangoma Technologies Inc.
*
* ============================================================================
* Jan 20, 2003  Nenad Corbic	Initial version
* ============================================================================
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2, or (at your option)
* any later version.
*
*/


/*
**************************************************************************
**			I N C L U D E S  F I L E S			**
**************************************************************************
*/

# include "wanpipe_includes.h"
# include "wanpipe_defines.h"
# include "wanpipe_debug.h"
# include "wanpipe_common.h"
# include "wanpipe_events.h"
# include "wanpipe.h"
# include "wanpipe_abstr.h"
# include "wanproc.h"


/*
******************************************************************************
**		Call Back function for kernel memory buffers
******************************************************************************
*/
/*
** wpabs_skb_data() - 
*/
unsigned char* wpabs_skb_data(void* skb)
{
	return wan_skb_data(skb);
}

/*
** wpabs_skb_tail() - 
*/
unsigned char* wpabs_skb_tail(void* skb)
{
	return wan_skb_tail(skb);
}


/*
** wpabs_skb_len() - 
*/
int wpabs_skb_len(void* skb)
{
	return wan_skb_len(skb);
}

/*
** wpabs_skb_len() - 
*/
int wpabs_skb_headroom(void* skb)
{
	return wan_skb_headroom(skb);
}


/*
**
*/
void* wpabs_skb_alloc_queue(void)
{
	return wan_malloc(sizeof(wan_skb_queue_t));
}

/*
**
*/
void wpabs_skb_free_queue(void* queue_ptr)
{

	if (!queue_ptr){
		DEBUG_EVENT("%s: Wanpipe Assertion: queue_ptr==NULL!\n",
				__FUNCTION__);
		return;
	}

	WAN_IFQ_PURGE((wan_skb_queue_t*)queue_ptr);
	wan_free((wan_skb_queue_t*)queue_ptr);
}

/*
**
*/
void wpabs_skb_queue_init(void *queue_ptr)
{
	wan_skb_queue_init(queue_ptr);
}

/*
** wpabs_skb_alloc() - 
*/
void* wpabs_skb_alloc(unsigned int len)
{
	void	*skb;
	skb = wan_skb_alloc(len);
	return skb;
}

/*
** wpabs_skb_free() - 
*/
void wpabs_skb_free(void* skb)
{
	wan_skb_free(skb);
}

/*
** wpabs_skb_copyback() - 
*/
#if defined(__WINDOWS__) || defined(__FreeBSD__)
void wpabs_skb_copyback(void* skb, int off, int len, caddr_t cp)
#else
void wpabs_skb_copyback(void* skb, int off, int len, unsigned long cp)
#endif
{
	wan_skb_copyback(skb, off, len, (caddr_t)cp);	
}

void wpabs_skb_copyback_user(void* skb, int off, int len, ulong_ptr_t cp)
{
	wan_skb_copyback_user(skb, off, len, (caddr_t)cp);	
}


/*
** wpabs_skb_pull() - 
*/
unsigned char* wpabs_skb_pull(void* skb, int len)
{
	return wan_skb_pull(skb, len); 
}

/*
** wpabs_skb_push() - 
*/
unsigned char* wpabs_skb_push(void* skb, int len)
{
	return wan_skb_push(skb, len); 
}

/*
** wpabs_skb_reserve() - 
*/
void wpabs_skb_reserve(void* skb, int len)
{
	wan_skb_reserve(skb, len); 
}


/*
** wpabs_skb_put() - 
*/
unsigned char* wpabs_skb_put(void* skb, int len)
{
	return wan_skb_put(skb, len); 
}


/*
** wpabs_skb_trim() - Trim from tail
*/
void wpabs_skb_trim(void* skb, unsigned int len)
{
	wan_skb_trim(skb, len);
}

/*
** wpabs_skb_clone() - Clone SKB Buffer
*/
void *wpabs_skb_clone(void* skb)
{
	return wan_skb_clone(skb);
}
/*
** wpabs_skb_copy() - Copy Skb buffer
*/
void *wpabs_skb_copy(void* skb)
{
	return wan_skb_copy(skb);
}


/*
** wpabs_skb_tailroom() - Skb tail room
*/
int wpabs_skb_tailroom(void* skb)
{
	return wan_skb_tailroom(skb);
}

/*
** wpabs_skb_queue_len() - Length of skb queue
*/

int wpabs_skb_queue_len(void *queue)
{
	return wan_skb_queue_len(queue);
}

void wpabs_skb_queue_purge(void *queue)
{
	WAN_IFQ_PURGE((wan_skb_queue_t*)queue);
}

void *wpabs_skb_dequeue(void *queue)
{
	return wan_skb_dequeue((wan_skb_queue_t*)queue);
}

/*
** wpabs_skb_queue_tail() - Length of skb queue
*/

void wpabs_skb_queue_tail(void *queue,void *skb)
{
	wan_skb_queue_tail(queue,skb);
}

void wpabs_skb_queue_head(void *queue,void *skb)
{
	wan_skb_queue_head(queue,skb);
}

void wpabs_skb_append(void *skb_prev, void *skb_cur, void *list)
{
	wan_skb_append(skb_prev,skb_cur,list);
}

void wpabs_skb_unlink(void *skb)
{
	wan_skb_unlink(skb);
}



/*
** wpabs_skb_init() - Init data pointer
*/
void wpabs_skb_init(void* skb, unsigned int len)
{
	wan_skb_init(skb, len);
}

void wpabs_skb_set_dev(void *skb_new_ptr, void *dev)
{
	wan_skb_set_dev(skb_new_ptr,dev);
}

void wpabs_skb_set_raw(void *skb_new_ptr)
{
	wan_skb_set_raw(skb_new_ptr);
}

void wpabs_skb_set_protocol(void *skb_new_ptr, unsigned int prot)
{
	wan_skb_set_protocol(skb_new_ptr,prot);
}

void wpabs_skb_set_csum(void *skb_new_ptr, unsigned int csum)
{
	wan_skb_set_csum(skb_new_ptr,csum);
}

unsigned int wpabs_skb_csum(void *skb_new_ptr)
{
	return wan_skb_csum(skb_new_ptr);
}



void *wpabs_netif_alloc(unsigned char *dev_name,int ifType, int *err)
{
	return wan_netif_alloc(dev_name,ifType,err);
}

void wpabs_netif_free(void *dev)
{
	wan_netif_free(dev);
}


unsigned char* wpabs_netif_name(void *dev)
{
	return wan_netif_name(dev);
}

/*
** wpabs_netif_queue_stopped
*/
int wpabs_netif_queue_stopped(void* dev)
{
	return WAN_NETIF_QUEUE_STOPPED(((netdevice_t*)dev));
}

/*
** wpabs_netif_queue_stopped
*/
int wpabs_netif_dev_up(void* dev)
{
	return WAN_NETIF_UP((netdevice_t*)dev);
}



/*
** wpabs_netif_wake_queue
*/
void wpabs_netif_wake_queue(void* dev)
{
	WAN_NETIF_WAKE_QUEUE((netdevice_t*)dev);
}

void* wpabs_timer_alloc(void)
{
	return wan_malloc(sizeof(wan_timer_t));	
}

/*
** wpabs_add_timer() - Set timer
*/
void wpabs_add_timer(void* timer_info, unsigned long delay)
{
	wan_timer_t*	timer = (wan_timer_t*)timer_info;

	WAN_ASSERT1(timer == NULL);
	wan_add_timer(timer, delay);
}

/*
** wpabs_init_timer
*/
void wpabs_init_timer(void* timer_info, void* timer_func,
#if defined(__WINDOWS__)
					 wan_timer_arg_t data)
#else
					 unsigned long data)
#endif
{
	wan_timer_t*	timer = (wan_timer_t*)timer_info;
	
	WAN_ASSERT1(timer == NULL);
	timer->MiniportTimerFunction = (wan_timer_func_t)timer_func;
	timer->MiniportAdapterHandle = (void*)data;
	timer->MiniportTimerContext  = (void*)data;
	
	wan_init_timer(timer, timer_func, (wan_timer_arg_t)data);
}

/*
** wpabs_del_timer
*/
void wpabs_del_timer(void* timer_info)
{
	wan_timer_t*	timer = (wan_timer_t*)timer_info;

	WAN_ASSERT1(timer == NULL);
	wan_del_timer(timer);
}



unsigned long* wpabs_dma_get_vaddr(void* pcard, void* dma_descr)
{
	return wan_dma_get_vaddr(pcard,dma_descr);
}

unsigned long wpabs_dma_get_paddr(void* pcard, void* dma_descr)
{
	return wan_dma_get_paddr(pcard,dma_descr);
}

void* wpabs_malloc(int size)
{
	return wan_malloc(size);
}

void* wpabs_kmalloc(int size)
{
	return wan_kmalloc(size);
}

void wpabs_free(void* ptr)
{
	wan_free(ptr);
}

/*
** wpabs_virt2bus
*/
unsigned long wpabs_virt2bus(unsigned long* ptr)
{
	return wan_virt2bus(ptr);
}

/*
** wpabs_bus2virt
*/
unsigned long* wpabs_bus2virt(unsigned long ptr)
{
	return wan_bus2virt(ptr);
}

/*
**
*/
unsigned char wpabs_bus_read_1(void* cardp, int offset)
{
	sdla_t*		card = (sdla_t*)cardp;
	unsigned char	data = 0x00;

	WAN_ASSERT(card == NULL);
	card->hw_iface.bus_read_1(card->hw, offset, &data);
	return data;
}

/*
**
*/
unsigned long wpabs_bus_read_4(void* cardp, int offset)
{
	sdla_t*		card = (sdla_t*)cardp;
	u32 data = 0x00;

	WAN_ASSERT(card == NULL);
	card->hw_iface.bus_read_4(card->hw, offset, &data);
	return data;
}

/*
**
*/
void wpabs_bus_write_1(void* cardp, int offset, unsigned char data)
{
	sdla_t*	card = (sdla_t*)cardp;

	WAN_ASSERT1(card == NULL);
	card->hw_iface.bus_write_1(card->hw, offset, data);
}

/*
**
*/
void wpabs_bus_write_4(void* cardp, int offset, unsigned long data)
{
	sdla_t*	card = (sdla_t*)cardp;

	WAN_ASSERT1(card == NULL);
	card->hw_iface.bus_write_4(card->hw, offset, data);
}

/*
**
*/
void wpabs_udelay(unsigned long microsecs)
{
	WP_DELAY(microsecs);
}

void* wpabs_spinlock_alloc(void)
{
	return wan_malloc(sizeof(wan_spinlock_t));
}
void wpabs_spinlock_free(void* lock)
{
	wan_free(lock);
}

/*
**
*/
void wpabs_spin_lock_irqsave(void* lock,unsigned long *flags)
{
	wan_spinlock_t*	SpinLock = (wan_spinlock_t*)lock;

	WAN_ASSERT1(SpinLock == NULL);
	wan_spin_lock_irq(SpinLock,(wan_smp_flag_t*)flags);
}
/*
**
*/
void wpabs_spin_unlock_irqrestore(void* lock,unsigned long *flags)
{
	wan_spinlock_t*	SpinLock = (wan_spinlock_t*)lock;

	WAN_ASSERT1(SpinLock == NULL);
	wan_spin_unlock_irq(SpinLock,(wan_smp_flag_t*)flags);
}

/*
**
*/
void wpabs_spin_lock_init(void* lock, char *name)
{
	wan_spinlock_t*	SpinLock = (wan_spinlock_t*)lock;

	WAN_ASSERT1(SpinLock == NULL);
	wan_spin_lock_irq_init(SpinLock, name);

}

#if 0
/*
**
*/
void* wpabs_rwlock_alloc(void)
{
	return wan_malloc(sizeof(wan_rwlock_t));
}
void wpabs_rwlock_free(void* lock)
{
	return wan_free(lock);
}

void wpabs_rwlock_init(void* lock)
{
	wan_rwlock_t*	rwlock = (wan_rwlock_t*)lock;
	WAN_ASSERT1(lock == NULL);
	WAN_RWLOCK_INIT(rwlock);
}

void wpabs_read_rw_lock(void* lock)
{
	wan_rwlock_t*	rwlock = (wan_rwlock_t*)lock;
	WAN_ASSERT1(lock == NULL);
	wan_read_rw_lock(&rwlock->rwlock);
}

void wpabs_read_rw_unlock(void* lock)
{
	wan_rwlock_t*	rwlock = (wan_rwlock_t*)lock;
	WAN_ASSERT1(lock == NULL);
	wan_read_rw_unlock(&rwlock->rwlock);
}

void wpabs_write_rw_lock_irq(void* lock,unsigned long *flags)
{
	wan_rwlock_t*	rwlock = (wan_rwlock_t*)lock;
	WAN_ASSERT1(lock == NULL);
	wan_write_rw_lock_irq(&rwlock->rwlock,flags);
}

void wpabs_write_rw_unlock_irq(void* lock,unsigned long *flags)
{
	wan_rwlock_t*	rwlock = (wan_rwlock_t*)lock;
	WAN_ASSERT1(lock == NULL);
	wan_write_rw_unlock_irq(&rwlock->rwlock,flags);
}

#endif

/*
**
*/
void wpabs_debug_event(const char * fmt, ...)
{
#ifdef WAN_DEBUG_EVENT
	va_list args;
	char buf[1020];
	va_start(args, fmt);
	wp_vsnprintf(buf, sizeof(buf), fmt, args);
	DEBUG_EVENT("%s", buf);
	va_end(args);
#endif
}


void __wpabs_debug_event(const char * fmt, ...)
{
#ifdef WAN_DEBUG_EVENT
	va_list args;
	char buf[1020];
	va_start(args, fmt);
	wp_vsnprintf(buf, sizeof(buf), fmt, args);
	DEBUG_EVENT("%s", buf);
	va_end(args);
#endif
}

/*
**
*/

void wpabs_debug_test(const char * fmt, ...)
{
#ifdef WAN_DEBUG_TEST
	va_list args;
	char buf[1020];
	va_start(args, fmt);
	wp_vsnprintf(buf, sizeof(buf), fmt, args);
	DEBUG_TEST("%s", buf);
	va_end(args);
#endif
}


/*
**
*/
void wpabs_debug_cfg(const char * fmt, ...)
{
#ifdef WAN_DEBUG_CFG
	va_list args;
	char buf[1020];
	va_start(args, fmt);
	wp_vsnprintf(buf, sizeof(buf), fmt, args);
	DEBUG_CFG("%s",buf);
	va_end(args);
#endif
}

/*
**
*/
void wpabs_debug_init(const char * fmt, ...)
{
#ifdef WAN_DEBUG_INIT_VAR
	va_list args;
	char buf[1020];
	va_start(args, fmt);
	wp_vsnprintf(buf, sizeof(buf), fmt, args);
	DEBUG_INIT("%s",buf);
	va_end(args);
#endif
}

/*
**
*/
void wpabs_debug_tx(const char * fmt, ...)
{
#ifdef WAN_DEBUG_TX
	va_list args;
	char buf[1020];
	va_start(args, fmt);
	wp_vsnprintf(buf, sizeof(buf), fmt, args);
	DEBUG_TX("%s",buf);
	va_end(args);
#endif
}

/*
**
*/
void wpabs_debug_rx(const char * fmt, ...)
{
#ifdef WAN_DEBUG_RX
	va_list args;
	char buf[1020];
	va_start(args, fmt);
	wp_vsnprintf(buf, sizeof(buf), fmt, args);
	DEBUG_RX("%s",buf);
	va_end(args);
#endif
}

/*
**
*/
void wpabs_debug_isr(const char * fmt, ...)
{
#ifdef WAN_DEBUG_ISR
	va_list args;
	char buf[1020];
	va_start(args, fmt);
	wp_vsnprintf(buf, sizeof(buf), fmt, args);
	DEBUG_ISR("%s",buf);
	va_end(args);
#endif
}
/*
**
*/
void wpabs_debug_timer(const char * fmt, ...)
{
#ifdef WAN_DEBUG_TIMER
	va_list args;
	char buf[1020];
	va_start(args, fmt);
	wp_vsnprintf(buf, sizeof(buf), fmt, args);
	DEBUG_TIMER("%s",buf);
	va_end(args);
#endif
}
/*
**
*/
int wpabs_set_bit(int bit, void *ptr)
{
	wan_set_bit(bit,ptr);
	return 0;
}

/*
**
*/
int wpabs_test_and_set_bit(int bit, void *ptr)
{
#if defined(__LINUX__)
	return test_and_set_bit(bit,ptr);
#elif defined(__NetBSD__) || defined(__FreeBSD__) || defined(__OpenBSD__)
	if (wan_test_bit(bit,ptr)){
		return 1;
	}
	wan_set_bit(bit,ptr);
	return 0;
#elif defined(__WINDOWS__)
	return test_and_set_bit(bit,ptr);
#else
# error "Error: wpabs_test_and_set_bit() not implemented!"
#endif

}


/*
**
*/
int wpabs_test_bit(int bit, void *ptr)
{
	return wan_test_bit(bit,ptr);
}
/*
**
*/
int wpabs_clear_bit(int bit, void *ptr)
{
	wan_clear_bit(bit,ptr);
	return 0;
}

wan_ticks_t wpabs_get_systemticks(void)
{
	return SYSTEM_TICKS;
}
	
unsigned long wpabs_get_hz(void)
{
	return HZ;
}

unsigned short wpabs_htons(unsigned short data)
{
	return htons(data);
}

unsigned short wpabs_ntohs(unsigned short data)
{
	return ntohs(data);
}

unsigned long wpabs_htonl(unsigned long data)
{
	return htonl(data);
}

unsigned long wpabs_ntohl(unsigned long data)
{
	return ntohl(data);
}



void * wpabs_tasklet_alloc(void)
{
	wan_tasklet_t*	task = NULL;
	task = (wan_tasklet_t*)wan_malloc(sizeof(wan_tasklet_t));
	task->running = 0;
	return (void*)task;
}

void wpabs_tasklet_init(void *task_ptr, int priority, void* func, void* arg)
{
	wan_tasklet_t*		task = (wan_tasklet_t *)task_ptr;
	wan_tasklet_func_t*	task_func = (wan_tasklet_func_t*)func;

	WAN_ASSERT1(task == NULL);
	WAN_TASKLET_INIT(task, priority, task_func, arg);
}


void wpabs_tasklet_schedule(void *task_ptr)
{
	wan_tasklet_t *task = (wan_tasklet_t*)task_ptr;
	
	WAN_ASSERT1(task == NULL);
	WAN_TASKLET_SCHEDULE(task);
}

void wpabs_tasklet_end(void *task_ptr)
{
	wan_tasklet_t *task = (wan_tasklet_t*)task_ptr;			

	WAN_ASSERT1(task == NULL);
	WAN_TASKLET_END(task);
}

void wpabs_tasklet_kill(void *task_ptr)
{
	wan_tasklet_t *task = (wan_tasklet_t*)task_ptr;			

	WAN_ASSERT1(task == NULL);
	WAN_TASKLET_KILL(task);
}

void *wpabs_taskq_alloc(void)
{
	return wan_malloc(sizeof(wan_taskq_t));
}

void wpabs_taskq_init(void *tq_ptr, void *func, void *data)
{
	wan_taskq_t*		tq = (wan_taskq_t *)tq_ptr;
	wan_taskq_func_t	tq_func = (wan_taskq_func_t)func;

	WAN_ASSERT1(tq==NULL);
	
	WAN_TASKQ_INIT(tq, 0, tq_func, data);
}

/*+F*************************************************************************
 * Function:
 *   wpabs_taskq_schedule_event
 *
 * Description:
 *   Schedule an event to be processed by the bottom half handler.
 *-F*************************************************************************/

#ifdef AFT_TASKQ_DEBUG
# define wpabs_taskq_schedule_event(bit, event, tq_ptr) wpabs_taskq_schedule_event(bit, event, tq_ptr, __FUNCTION__, __LINE__) 
void __wpabs_taskq_schedule_event(unsigned int bit, unsigned long *event, void *tq_ptr, const char *func, int line) 
#else
void wpabs_taskq_schedule_event(unsigned int bit, unsigned long *event, void *tq_ptr)
#endif
{
	wan_taskq_t *tq = (wan_taskq_t *)tq_ptr;

	WAN_ASSERT1(tq==NULL)
	DEBUG_TX ("wpabs_wan_schedule_event: scheduling task\n");
    	wan_set_bit(bit, event);

#ifdef AFT_TASKQ_DEBUG
	DEBUG_TASKQ("wpabs_taskq_schedule_event: caller: %s():%d\n", func, line);
#endif

	WAN_TASKQ_SCHEDULE(tq);
}

void* wpabs_memset(void *b, int c, int len)
{
	return memset(b,c,len);
}

void* wpabs_strncpy(void *b, void * c, int len)
{
	return strncpy(b,c,len);
}

void* wpabs_memcpy(void *b, void *c, int len)
{
	return memcpy(b,c,len);
}

int wpabs_memcmp(void *b, void *c, int len)
{
	return memcmp(b,c,len);
}

int wpabs_strlen(unsigned char *str)
{
	return strlen(str);
}

void wpabs_debug_print_skb(void *skb_ptr, char dir)
{
#if 1
/*#ifdef WAN_DEBUG_TX*/
	netskb_t*	skb = (netskb_t*)skb_ptr;
	unsigned char*	data = NULL;
	int i;

	DEBUG_TX("%s SKB: Len=%i : ",dir?"TX":"RX",wan_skb_len(skb));
	data = wan_skb_data(skb);
	for (i=0; i < wan_skb_len(skb); i++){
		_DEBUG_TX("%02X ", data[i]);
	}
	
	_DEBUG_TX("\n");
#endif
}



/* 
 * ============================================================================
 * Set WAN device state.
 */

void wpabs_decode_ipaddr(unsigned long ipaddr, unsigned char *str, int len)
{
	snprintf(str,len,"%u.%u.%u.%u",NIPQUAD(ipaddr));
}



unsigned long wan_get_ip_addr(void* dev, int option)
{
	netdevice_t*		ifp = (netdevice_t*)dev;

#if defined(__NetBSD__) || defined(__FreeBSD__) || defined(__OpenBSD__)
	struct ifaddr*		ifa = NULL;		
	struct sockaddr_in*	addr = NULL;

	if (ifp == NULL){
		return 0;
	}
	ifa = WAN_TAILQ_FIRST(ifp);
	if (ifa == NULL || ifa->ifa_addr == NULL){
		return 0;
	}
#elif defined(__LINUX__)
	struct in_ifaddr *ifaddr;
	struct in_device *in_dev;

	if ((in_dev = in_dev_get(ifp)) == NULL){
		return 0;
	}
	if ((ifaddr = in_dev->ifa_list)== NULL ){
		in_dev_put(in_dev);
		return 0;
	}
	in_dev_put(in_dev);
#endif	

	switch (option){
	case WAN_LOCAL_IP:
#if defined(__FreeBSD__)
		ifa = WAN_TAILQ_NEXT(ifa);
		if (ifa == NULL) return 0;
		addr = (struct sockaddr_in *)ifa->ifa_addr;
		return addr->sin_addr.s_addr;
#elif defined(__NetBSD__) || defined(__OpenBSD__)
		addr = (struct sockaddr_in *)ifa->ifa_addr;
		return htonl(addr->sin_addr.s_addr);
#elif defined(__WINDOWS__)
		FUNC_NOT_IMPL();
		return 0;
#else
		return ifaddr->ifa_local;
#endif
		break;
	
	case WAN_POINTOPOINT_IP:
#if defined(__FreeBSD__)
		ifa = WAN_TAILQ_NEXT(ifa);
		if (ifa == NULL) return 0;
		addr = (struct sockaddr_in *)ifa->ifa_dstaddr;
		return addr->sin_addr.s_addr;
#elif defined(__NetBSD__) || defined(__OpenBSD__)
		return 0;
#elif defined(__WINDOWS__)
		FUNC_NOT_IMPL();
		return 0;
#else
		return ifaddr->ifa_address;
#endif
		break;	

	case WAN_NETMASK_IP:
#if defined(__NetBSD__) || defined(__FreeBSD__) || defined(__OpenBSD__)
		return 0;
#elif defined(__WINDOWS__)
		FUNC_NOT_IMPL();
		return 0;
#else
		return ifaddr->ifa_mask;
#endif
		break;

	case WAN_BROADCAST_IP:
#if defined(__NetBSD__) || defined(__FreeBSD__) || defined(__OpenBSD__)
		return 0;
#elif defined(__WINDOWS__)
		FUNC_NOT_IMPL();
		return 0;
#else
		return ifaddr->ifa_broadcast;
#endif
		break;
	default:
		break;
	}

	return 0;
}

unsigned long wpabs_get_ip_addr(void* dev, int option)
{
	return wan_get_ip_addr(dev, option);
}

#define UNKNOWN_PROT	"Unknown Prot"
#define IP_LLC_ATM	"Classic IP (LLC) over ATM"
#define IP_VC_ATM	"IP (VC) over ATM"
#define B_LLC_ETH	"Bridged LLC Ethernet over ATM"
#define B_VC_ETH	"Bridged VC Ethernet over ATM"
#define PPP_LLC_ATM	"PPP (LLC) over ATM"
#define PPP_VC_ATM	"PPP (VC) over ATM"
int wpabs_detect_prot_header(unsigned char *data,int dlen, char* temp, int tlen)
{
	int i,cnt=0;

	if (temp){
		memset(temp, 0, tlen);
	}
	
	if (data[0] == 0xAA && data[1] == 0xAA){
		if (data[6] == 0x08){
			if (data[7] == 0x00 || data[7] == 0x06){
				if (temp){
					memcpy(temp, IP_LLC_ATM, strlen(IP_LLC_ATM)); 
				}
				return 0;
			}
		}else if (data[6] == 0x00 && data[7] == 0x07){
			if (temp){
				memcpy(temp, B_LLC_ETH, strlen(B_LLC_ETH)); 
			}
			return 0;
		}
		goto detect_unknown;
	}

	if ((data[0] == 0xFE && data[1] == 0xFE) ||
	    (data[0] == 0xFE && data[1] == 0x03)){
		if (temp){
			memcpy(temp, PPP_LLC_ATM, strlen(PPP_LLC_ATM)); 
		}
		return 0;
	}
		
	if (data[0] == 0xC0 && data[1] == 0x21){
		if (temp){
			memcpy(temp, PPP_VC_ATM, strlen(PPP_VC_ATM)); 
		}
		return 0;
	}

	if (data[0] == 0x45){
		if (temp){
			memcpy(temp, IP_VC_ATM, strlen(IP_VC_ATM)); 
		}
		return 0;		
	}
	
detect_unknown:
	if (temp){
		for (i=0;i<dlen && i<10;i++){
			cnt+=sprintf(temp+cnt,"%02X ",data[i]);
		}
		temp[cnt] = '\0';
	}
	return 1;
}

/*+F*************************************************************************
 * Function:
 *   
 *
 * Description:
 *   Limits kernel driver output messages.
 *
 *-F*************************************************************************/
int wpabs_net_ratelimit(void)
{
#ifdef __LINUX__
	return net_ratelimit();
#elif defined(__NetBSD__) || defined(__FreeBSD__) || defined(__OpenBSD__)
	/* Do nothing for FreeBSD/OpenBSD! 
	** For these OS always print all messages.
	*/
	return 1;
#elif defined(__WINDOWS__)
	return 1;
#else
# error "wpabs_netrate_limit not supported"
#endif
}

void wpabs_get_random_bytes(void *ptr, int len)
{
#if defined(__LINUX__)
	get_random_bytes(ptr,len);
#elif defined(__FreeBSD__) || defined(__NetBSD__)
	read_random(ptr,len);
#elif defined(__OpenBSD__)
	get_random_bytes(ptr,len);
#elif defined(__WINDOWS__)
	get_random_bytes(ptr, len);
#else
# error "wpabs_get_random_bytes not supported"
#endif

}

/*+F*************************************************************************
 * Function:
 *   Tracing support functions 
 *
 * Description:
 *
 *
 *-F*************************************************************************/

void wpabs_set_last_trace_direction(void *trace_ptr, unsigned char direction)
{
	((wan_trace_t*)trace_ptr)->last_trace_direction = direction;
}

unsigned char wpabs_get_last_trace_direction(void *trace_ptr)
{
	return ((wan_trace_t*)trace_ptr)->last_trace_direction;
}

/*
** wpabs_bpf_report
*/
int wpabs_bpf_report(void* dev, void* skb, int flag, int dir)
{
	wan_bpf_report((netdevice_t*)dev, (netskb_t*)skb, flag, dir);
	return 0;
}

