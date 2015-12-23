/*****************************************************************************
* wanpipe_abstr.h WANPIPE(tm) Kernel Abstraction Layer.
*
* Authors: 	Nenad Corbic <ncorbic@sangoma.com>
*		Alex Feldman <al.feldman@sangoma.com>
*		David Rokhvarg <davidr@sangoma.com>
*
* Copyright:	(c) 2003 Sangoma Technologies Inc.
*
* ============================================================================
* Jan 20, 2003  Nenad Corbic	Initial version
*
* Nov 27,  2007 David Rokhvarg	Implemented functions/definitions for
*                               Sangoma MS Windows Driver and API.
*
* ============================================================================
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2, or (at your option)
* any later version.
*
*/

#ifndef _WANPIPE_ABSTR_H
#define _WANPIPE_ABSTR_H


#include "wanpipe_abstr_types.h"


extern unsigned char* 	wpabs_skb_data(void* skb);
extern unsigned char* 	wpabs_skb_tail(void* skb);
extern int 		wpabs_skb_len(void* skb);
extern void*		wpabs_skb_alloc(unsigned int len);
extern void		wpabs_skb_free(void* skb);
#if defined(__WINDOWS__) || defined(__FreeBSD__)
extern void 		wpabs_skb_copyback(void*, int, int, caddr_t); 
#else
extern void 		wpabs_skb_copyback(void*, int, int, unsigned long); 
#endif

extern void 		wpabs_skb_copyback_user(void* skb, int off, int len, ulong_ptr_t cp);
extern unsigned char* 	wpabs_skb_pull(void* skb, int len);
extern unsigned char* 	wpabs_skb_put(void* skb, int len);
extern unsigned char* 	wpabs_skb_push(void* skb, int len);
extern void  		wpabs_skb_reserve(void* skb, int len);
extern void		wpabs_skb_trim(void* skb, unsigned int len);
extern int		wpabs_skb_tailroom(void *skb);
extern int		wpabs_skb_headroom(void *skb);
extern void		wpabs_skb_init(void* skb, unsigned int len);
extern void*		wpabs_skb_clone(void* skb);
extern void*		wpabs_skb_copy(void* skb);

extern void*		wpabs_skb_alloc_queue(void);
extern void		wpabs_skb_free_queue(void*);
extern void		wpabs_skb_queue_init(void *);
extern void 		wpabs_skb_queue_purge(void *);
extern int		wpabs_skb_queue_len(void *);
extern void		wpabs_skb_queue_tail(void *, void *);
extern void 		wpabs_skb_queue_head(void *,void *);
extern void 		wpabs_skb_append(void *,void *, void*);
extern void 		wpabs_skb_unlink(void *);

extern void*		wpabs_skb_dequeue(void *queue);

extern void		wpabs_skb_set_dev(void *skb, void *dev);
extern void		wpabs_skb_set_raw(void *skb);
extern void		wpabs_skb_set_protocol(void *skb, unsigned int prot);
extern void 		wpabs_skb_set_csum(void *skb_new_ptr, unsigned int csum);
extern unsigned int	wpabs_skb_csum(void *skb_new_ptr);


extern void*		wpabs_netif_alloc(unsigned char *,int,int*);
extern void		wpabs_netif_free(void *);
extern unsigned char*	wpabs_netif_name(void *);
extern int 		wpabs_netif_queue_stopped(void*);
extern int		wpabs_netif_dev_up(void*);
extern void 		wpabs_netif_wake_queue(void* dev);

extern void*		wpabs_timer_alloc(void);
extern void 		wpabs_init_timer(void*, void*, 
#if defined(__WINDOWS__)
					 wan_timer_arg_t);
#else
					 unsigned long);
#endif

extern void 		wpabs_del_timer(void*);
extern void 		wpabs_add_timer(void*,unsigned long);

extern void*		wpabs_malloc(int);
extern void*		wpabs_kmalloc(int);
extern void		wpabs_free(void*);


extern unsigned long*	wpabs_bus2virt(unsigned long phys_addr);
extern unsigned long	wpabs_virt2bus(unsigned long* virt_addr);

extern unsigned char	wpabs_bus_read_1(void*, int);      
extern unsigned long	wpabs_bus_read_4(void*, int);      
extern void		wpabs_bus_write_1(void*, int, unsigned char);      
extern void		wpabs_bus_write_4(void*, int, unsigned long);      

extern void 		wpabs_udelay(unsigned long microsecs);

extern void* 		wpabs_spinlock_alloc(void);
extern void 		wpabs_spinlock_free(void*);
extern void 		wpabs_spin_lock_irqsave(void*,unsigned long*);
extern void 		wpabs_spin_unlock_irqrestore(void*,unsigned long*);
extern void 		wpabs_spin_lock_init(void*, char*);

extern void 		wpabs_rwlock_init (void*);
extern void 		wpabs_read_rw_lock(void*);
extern void 		wpabs_read_rw_unlock(void*);
extern void 		wpabs_write_rw_lock_irq(void*,unsigned long*);
extern void		wpabs_write_rw_unlock_irq(void*,unsigned long*);

extern void 		wpabs_debug_event(const char * fmt, ...);
extern void 		__wpabs_debug_event(const char * fmt, ...);
extern void 		wpabs_debug_init(const char * fmt, ...);
extern void 		wpabs_debug_cfg(const char * fmt, ...);
extern void 		wpabs_debug_tx(const char * fmt, ...);
extern void 		wpabs_debug_rx(const char * fmt, ...);
extern void 		wpabs_debug_isr(const char * fmt, ...);
extern void 		wpabs_debug_timer(const char * fmt, ...);
extern void 		wpabs_debug_test(const char * fmt, ...);

extern int 		wpabs_set_bit(int bit, void *ptr);
extern int 		wpabs_test_bit(int bit, void *ptr);
extern int		wpabs_test_and_set_bit(int bit, void *ptr);
extern int 		wpabs_clear_bit(int bit, void *ptr);

extern wan_ticks_t 	wpabs_get_systemticks(void);
extern unsigned long 	wpabs_get_hz(void);
extern unsigned short 	wpabs_htons(unsigned short data);
extern unsigned short	wpabs_ntohs(unsigned short);
extern unsigned long	wpabs_htonl(unsigned long data);
extern unsigned long	wpabs_ntohl(unsigned long);

#if 0
extern void		wpabs_lan_rx(void*,void*,unsigned long,unsigned char*, int);
extern void		wpabs_tx_complete(void*, int, int);

extern void* 		wpabs_ttydriver_alloc(void);
extern void 		wpabs_ttydriver_free(void*);
extern void* 		wpabs_termios_alloc(void);
extern void		wpabs_tty_hangup(void*);

extern int 		wpabs_wan_register(void *,
		             	  void *, 
				  char *,
		             	  unsigned char);

extern void 		wpabs_wan_unregister(void *, unsigned char);
#endif

extern void* 		wpabs_taskq_alloc(void);
extern void 		wpabs_taskq_init(void *, void *, void *);
extern void 		wpabs_taskq_schedule_event(unsigned int, unsigned long *, void *);

extern void 		wpabs_tasklet_kill(void *);
extern void 		wpabs_tasklet_end(void *);
extern void 		wpabs_tasklet_schedule(void *);
extern void 		wpabs_tasklet_init(void *, int , void *, void* );
extern void*		wpabs_tasklet_alloc(void);

extern void* 		wpabs_memset(void *, int , int);
extern void* 		wpabs_strncpy(void *, void* , int);
extern void* 		wpabs_memcpy(void *, void* , int);
extern int 		wpabs_memcmp(void *, void* , int);

extern int		wpabs_strlen(unsigned char *);

extern void		wpabs_debug_print_skb(void*,char);
 
extern void 		wpabs_decode_ipaddr(unsigned long, unsigned char *,int);

extern int 		wpabs_detect_prot_header(unsigned char *,int, char*,int);
extern int		wpabs_net_ratelimit(void);
extern void 		wpabs_get_random_bytes(void *ptr, int len);

/* ===========================================
 * FUNCTION PRIVATE TO WANPIPE MODULES 
 * ==========================================*/

extern void		wpabs_set_baud(void*, unsigned int);
extern void		wpabs_set_state(void*, int);
extern char		wpabs_get_state(void*);
extern unsigned long	wpabs_get_ip_addr(void*, int);


extern void		wpabs_card_lock_irq(void *,unsigned long *);
extern void		wpabs_card_unlock_irq(void *,unsigned long *);

extern void* 		wpabs_dma_alloc(void*, unsigned long);
extern int 		wpabs_dma_free(void*, void*);
extern unsigned long*	wpabs_dma_get_vaddr(void*, void*);
extern unsigned long	wpabs_dma_get_paddr(void*, void*);

extern int		wpabs_trace_queue_len(void *trace_ptr);
extern int		wpabs_tracing_enabled(void*);
extern int		wpabs_trace_enqueue(void*, void*);
extern int		wpabs_trace_purge(void*);
extern void* 		wpabs_trace_info_alloc(void);
extern void 		wpabs_trace_info_init(void *trace_ptr, int max_queue);
extern void 		wpabs_set_last_trace_direction(void *trace_ptr, unsigned char direction);
extern unsigned char 	wpabs_get_last_trace_direction(void *trace_ptr);

extern int		wpabs_bpf_report(void *dev, void *skb, int,int);

#if defined(__WINDOWS__)
# if WP_USE_INTERLOCKED_LIST_FUNCTIONS
#  pragma deprecated(wpabs_skb_append)
#  pragma deprecated(wpabs_skb_unlink)
# endif
#endif

#endif
