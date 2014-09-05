/************************************************************************
* wanpipe_defines.h														*
*		WANPIPE(tm) 	Global definition for Sangoma 					*
*				Mailbox/API/UDP	structures.								*
*																		*
* Author:	Alex Feldman <al.feldman@sangoma.com>						*
*=======================================================================*
* May 10, 2002	Alex Feldman	Initial version							*
*																		* 
* Nov 27,  2007 David Rokhvarg	Implemented functions/definitions for   *
*                              Sangoma MS Windows Driver and API.       *
*																		*
*																		*
*************************************************************************/

#ifndef __WANPIPE_DEFINES_H
# define __WANPIPE_DEFINES_H

/************************************************
 *	SET COMMON KERNEL DEFINE		*
 ************************************************/
#if defined (__KERNEL__) || defined (KERNEL) || defined (_KERNEL)
# ifndef WAN_KERNEL
#  define WAN_KERNEL
# endif
#endif

#include "wanpipe_version.h"
#if defined(WAN_KERNEL)
 #include "wanpipe_kernel.h"
#endif
#include "wanpipe_abstr_types.h" /* Basic data types */


#if defined(__WINDOWS__)
# if defined(WAN_KERNEL) 
#  include "wanpipe_skb.h"
#  define inline __inline
#  if defined(NTSTRSAFE_USE_SECURE_CRT)
#   define wp_snwprintf	RtlStringCbPrintfW
#   define wp_strcpy	RtlStringCchCopy
#  else
#   define wp_snwprintf	_snwprintf
#  endif/* NTSTRSAFE_USE_SECURE_CRT */
# endif/* WAN_KERNEL */

# define wp_strlcpy		strncpy
# define wp_strncasecmp	_strnicmp
# define wp_strcasecmp	_stricmp
# define wp_snprintf	_snprintf
# define wp_vsnprintf	_vsnprintf
# define wp_unlink		_unlink
#else/* ! __WINDOWS__ */
# define wp_strlcpy		strlcpy
# define wp_strncasecmp	wp_linux_strncasecmp
# define wp_strcasecmp	strcasecmp
# define wp_snprintf	snprintf
# define wp_vsnprintf	vsnprintf
# define wp_unlink		unlink
# define wp_sleep		sleep
# define wp_gettimeofday gettimeofday
# define wp_localtime_r	localtime_r
# define wp_usleep		usleep
#endif


/************************************************
 *	GLOBAL SANGOMA PLATFORM DEFINITIONS	*
 ************************************************/
#define WAN_LINUX_PLATFORM	0x01
#define WAN_WIN98_PLATFORM	0x02
#define WAN_WINNT_PLATFORM	0x03
#define WAN_WIN2K_PLATFORM	0x04
#define WAN_FREEBSD_PLATFORM	0x05
#define WAN_OPENBSD_PLATFORM	0x06
#define WAN_SOLARIS_PLATFORM	0x07
#define WAN_SCO_PLATFORM	0x08
#define WAN_NETBSD_PLATFORM	0x09

#if defined(__FreeBSD__)
# define WAN_PLATFORM_ID	WAN_FREEBSD_PLATFORM
#elif defined(__OpenBSD__)
# define WAN_PLATFORM_ID	WAN_OPENBSD_PLATFORM
#elif defined(__NetBSD__)
# define WAN_PLATFORM_ID	WAN_NETBSD_PLATFORM
#elif defined(__LINUX__)
# define WAN_PLATFORM_ID	WAN_LINUX_PLATFORM
#elif defined(__WINDOWS__)
# define WAN_PLATFORM_ID	WAN_WIN2K_PLATFORM
#endif


/*
************************************************
**	GLOBAL SANGOMA DEFINITIONS			
************************************************
*/
#define WAN_FALSE	0
#define WAN_TRUE	1

#if defined(__FreeBSD__)
# undef WANPIPE_VERSION
# undef WANPIPE_VERSION_BETA
# undef WANPIPE_SUB_VERSION
# undef WANPIPE_LITE_VERSION
# define WANPIPE_VERSION	WANPIPE_VERSION_FreeBSD
# define WANPIPE_VERSION_BETA	WANPIPE_VERSION_BETA_FreeBSD
# define WANPIPE_SUB_VERSION	WANPIPE_SUB_VERSION_FreeBSD
# define WANPIPE_LITE_VERSION	WANPIPE_LITE_VERSION_FreeBSD
#elif defined(__OpenBSD__)
# undef WANPIPE_VERSION
# undef WANPIPE_VERSION_BETA
# undef WANPIPE_SUB_VERSION
# undef WANPIPE_LITE_VERSION
# define WANPIPE_VERSION	WANPIPE_VERSION_OpenBSD
# define WANPIPE_VERSION_BETA	WANPIPE_VERSION_BETA_OpenBSD
# define WANPIPE_SUB_VERSION	WANPIPE_SUB_VERSION_OpenBSD
# define WANPIPE_LITE_VERSION	WANPIPE_LITE_VERSION_OpenBSD
#elif defined(__NetBSD__)
# undef WANPIPE_VERSION
# undef WANPIPE_VERSION_BETA
# undef WANPIPE_SUB_VERSION
# undef WANPIPE_LITE_VERSION
# define WANPIPE_VERSION	WANPIPE_VERSION_NetBSD
# define WANPIPE_VERSION_BETA	WANPIPE_VERSION_BETA_NetBSD
# define WANPIPE_SUB_VERSION	WANPIPE_SUB_VERSION_NetBSD
# define WANPIPE_LITE_VERSION	WANPIPE_LITE_VERSION_NetBSD
#elif defined(__WINDOWS__)
#endif

#define WANROUTER_MAJOR_VER	2
#define WANROUTER_MINOR_VER	1

#define WANPIPE_MAJOR_VER	1
#define WANPIPE_MINOR_VER	1
/*
*************************************************
**	GLOBAL SANGOMA TYPEDEF
*************************************************
*/
#if defined(__LINUX__)
typedef struct ethhdr		ethhdr_t;
typedef	struct iphdr		iphdr_t;
typedef	struct udphdr		udphdr_t;
typedef	struct tcphdr		tcphdr_t;
# define w_eth_dest	h_dest
# define w_eth_src	h_source
# define w_eth_proto	h_proto
# define w_ip_v		version
# define w_ip_hl	ihl
# define w_ip_tos	tos
# define w_ip_len	tot_len
# define w_ip_id	id
# define w_ip_off	frag_off
# define w_ip_ttl	ttl
# define w_ip_p		protocol
# define w_ip_sum	check
# define w_ip_src	saddr
# define w_ip_dst	daddr
# define w_udp_sport	source
# define w_udp_dport	dest
# define w_udp_len	len
# define w_udp_sum	check
# define w_tcp_sport	source
# define w_tcp_dport	dest
# define w_tcp_seq	seq
# define w_tcp_ack_seq	ack_seq

#elif defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__)
typedef	struct ip		iphdr_t;
typedef	struct udphdr		udphdr_t;
typedef	struct tcphdr		tcphdr_t;
# define w_ip_v		ip_v
# define w_ip_hl	ip_hl
# define w_ip_tos	ip_tos
# define w_ip_len	ip_len
# define w_ip_id	ip_id
# define w_ip_off	ip_off
# define w_ip_ttl	ip_ttl
# define w_ip_p		ip_p
# define w_ip_sum	ip_sum
# define w_ip_src	ip_src.s_addr
# define w_ip_dst	ip_dst.s_addr
# define w_udp_sport	uh_sport
# define w_udp_dport	uh_dport
# define w_udp_len	uh_ulen
# define w_udp_sum	uh_sum
# define w_tcp_sport	th_sport
# define w_tcp_dport	th_dport
# define w_tcp_seq	th_seq
# define w_tcp_ack_seq	th_ack

# if (__FreeBSD_version > 700000)
# define wan_time_t time_t
# else /* includes FreeBSD-5/6/OpenBSD/NetBSD */
# define wan_time_t long
# endif
#define wan_suseconds_t suseconds_t



#elif defined(__WINDOWS__)
/* Intel X86 */
#define __LITTLE_ENDIAN_BITFIELD

struct iphdr {
#if defined(__LITTLE_ENDIAN_BITFIELD)
	__u8    ihl:4,
		version:4;
#elif defined (__BIG_ENDIAN_BITFIELD)
	__u8    version:4,
		ihl:4;
#else
# error  "unknown byteorder!"
#endif
	__u8    tos;
	__u16   tot_len;
	__u16   id;
	__u16   frag_off;
	__u8    ttl;
	__u8    protocol;
	__u16   check;
	__u32   saddr;
	__u32   daddr;
	/*The options start here. */
};

struct udphdr {
	__u16   source;
	__u16   dest;
	__u16   len;
	__u16   check;
};

typedef	struct	iphdr	iphdr_t;
typedef	struct	udphdr	udphdr_t;


# define w_eth_dest	h_dest
# define w_eth_src	h_source
# define w_eth_proto	h_proto
# define w_ip_v		version
# define w_ip_hl	ihl
# define w_ip_tos	tos
# define w_ip_len	tot_len
# define w_ip_id	id
# define w_ip_off	frag_off
# define w_ip_ttl	ttl
# define w_ip_p		protocol
# define w_ip_sum	check
# define w_ip_src	saddr
# define w_ip_dst	daddr
# define w_udp_sport	source
# define w_udp_dport	dest
# define w_udp_len	len
# define w_udp_sum	check
# define w_tcp_sport	source
# define w_tcp_dport	dest
# define w_tcp_seq	seq
# define w_tcp_ack_seq	ack_seq

#if !defined snprintf
# define snprintf	_snprintf
#endif

#else
# error "Unknown OS system!"
#endif

#if defined(__FreeBSD__)
typedef u_int8_t		u8;
typedef u_int16_t		u16;
typedef u_int32_t		u32;
#elif defined(__OpenBSD__)
typedef u_int8_t		u8;
typedef u_int16_t		u16;
typedef u_int32_t		u32;
typedef u_int64_t		u64;
#elif defined(__NetBSD__)
typedef u_int8_t		u8;
typedef u_int16_t		u16;
typedef u_int32_t		u32;
typedef u_int64_t		u64;
#endif


/************************************************
**	GLOBAL SANGOMA MACROS
************************************************/
#if defined(__LINUX__)
# if !defined(strlcpy)
#  define strlcpy(d,s,l)	strcpy((d),(s))
# endif
#elif defined(__FreeBSD__)
# if !defined(strlcpy)
#  define strlcpy(d,s,l)	strcpy((d),(s))
# endif
#endif

#ifdef WAN_KERNEL


/*
******************************************************************
**	D E F I N E S
******************************************************************
*/
#define MAX_PACKET_SIZE		5000
#if defined(__FreeBSD__)
/******************* F R E E B S D ******************************/
# define WAN_MOD_LOAD		MOD_LOAD
# define WAN_MOD_UNLOAD		MOD_UNLOAD
# if (__FreeBSD_version > 503000)
#  define WAN_MOD_SHUTDOWN	MOD_SHUTDOWN
#  define WAN_MOD_QUIESCE	MOD_QUIESCE
# else 
#  define WAN_MOD_SHUTDOWN	WAN_MOD_UNLOAD+1
#  define WAN_MOD_QUIESCE	WAN_MOD_UNLOAD+2
# endif
# define WP_DELAY		DELAY
# define WP_SCHEDULE(arg,name)	{void*ptr=(name);tsleep(ptr,PPAUSE,(name),(arg)); }
/*# define WP_SCHEDULE(arg,name)	tsleep(&(arg),PPAUSE,(name),(arg))*/
# define SYSTEM_TICKS		ticks
# define HZ			hz
# define RW_LOCK_UNLOCKED	0
# define ETH_P_IP		AF_INET
# define ETH_P_IPV6		AF_INET6
# define ETH_P_IPX		AF_IPX
# define WAN_IFT_OTHER		IFT_OTHER
# define WAN_IFT_ETHER		IFT_ETHER
# define WAN_IFT_PPP		IFT_PPP
# define WAN_MFLAG_PRV		M_PROTO1
# define WAN_MFLAG_IPX		M_PROTO2
typedef u_long			wan_ioctl_cmd_t;
#elif defined(__OpenBSD__)
/******************* O P E N B S D ******************************/
# define WAN_MOD_LOAD		LKM_E_LOAD
# define WAN_MOD_UNLOAD		LKM_E_UNLOAD
# define WP_DELAY		DELAY
# define WP_SCHEDULE(arg,name)	tsleep(&(arg),PPAUSE,(name),(arg))
# define SYSTEM_TICKS		ticks
# define HZ			hz
# define RW_LOCK_UNLOCKED	0
# define ETH_P_IP		AF_INET
# define ETH_P_IPV6		AF_INET6
# define ETH_P_IPX		AF_IPX
# define WAN_IFT_OTHER		IFT_OTHER
# define WAN_IFT_ETHER		IFT_ETHER
# define WAN_IFT_PPP		IFT_PPP
# define WAN_MFLAG_PRV		M_PROTO1
# define WAN_MFLAG_IPX		M_PROTO2
typedef u_long			wan_ioctl_cmd_t;
#elif defined(__NetBSD__)
/******************* N E T B S D ******************************/
# define WAN_MOD_LOAD		LKM_E_LOAD
# define WAN_MOD_UNLOAD		LKM_E_UNLOAD
# define WP_DELAY		DELAY
# define SYSTEM_TICKS		tick
# define HZ			hz
# define RW_LOCK_UNLOCKED	0
# define WAN_IFT_OTHER		IFT_OTHER
# define WAN_IFT_ETHER		IFT_ETHER
# define WAN_IFT_PPP		IFT_PPP
typedef u_long			wan_ioctl_cmd_t;
#elif defined(__LINUX__)
/*********************** L I N U X ******************************/
# define ETHER_ADDR_LEN		ETH_ALEN

static __inline void WP_DELAY(int usecs)
{
   if ((usecs) <= 1000) {
   		udelay(usecs) ; 
   } else { 
       int delay=usecs/1000; 
   	   int i;              
	   if (delay < 1) {
	   		delay=1;   
	   }
	   for (i=0;i<delay;i++) { 
           	udelay(1000);      
	   }                       
   }

   return;
}    

# define atomic_set_int(name, val)	atomic_set(name, val)
# define SYSTEM_TICKS		jiffies
# define WP_SCHEDULE(arg,name)	schedule()
# define wan_atomic_read	atomic_read
# define wan_atomic_set		atomic_set
# define wan_atomic_inc		atomic_inc
# define wan_atomic_dec		atomic_dec
# define WAN_IFT_OTHER		0x00
# define WAN_IFT_ETHER		0x00
# define WAN_IFT_PPP		0x00
typedef int			wan_ioctl_cmd_t;
#elif defined(__WINDOWS__)
/******************* W I N D O W S ******************************/
# define SYSTEM_TICKS	get_systemticks()
# define jiffies		SYSTEM_TICKS

# define wan_atomic_read	atomic_read
# define wan_atomic_set		atomic_set
# define wan_atomic_inc		atomic_inc
# define wan_atomic_dec		atomic_dec
# define RW_LOCK_UNLOCKED	0
typedef int			wan_ioctl_cmd_t;

# define WP_SCHEDULE(arg,name)	WP_MILLISECONDS_DELAY(arg)
# define WAN_IFT_OTHER		0x00

/* this pseudo lock is used only for debugging of LIP layer */
#define wan_rwlock_init(plock)  *(plock)=RW_LOCK_UNLOCKED;

#endif /* __WINDOWS__ */

#if defined(__FreeBSD__)
# define WAN_MODULE_VERSION(module, version)			\
	MODULE_VERSION(module, version)
# define WAN_MODULE_DEPEND(module, mdepend, vmin, vpref, vmax)	\
	MODULE_DEPEND(module, mdepend, vmin, vpref, vmax)		 
# define WAN_MODULE_DEFINE(name,name_str,author,descr,lic,mod_init,mod_exit,devsw)\
	int load_##name (module_t mod, int cmd, void *arg);	\
	int load_##name (module_t mod, int cmd, void *arg){	\
		switch(cmd){					\
		case WAN_MOD_LOAD: return mod_init((devsw));	\
		case WAN_MOD_UNLOAD: return mod_exit((devsw));	\
		case WAN_MOD_SHUTDOWN: return 0;\
		case WAN_MOD_QUIESCE: return 0;	\
		}						\
		return -EINVAL;					\
	}							\
	DEV_MODULE(name, load_##name, NULL);
#elif defined(__OpenBSD__)
# define WAN_MODULE_VERSION(module, version)
# define WAN_MODULE_DEPEND(module, mdepend, vmin, vpref, vmax)
# define WAN_MODULE_DEFINE(name,name_str,author,descr,lic,mod_init,mod_exit,devsw)\
	int (name)(struct lkm_table* lkmtp, int cmd, int ver);\
	MOD_DEV(name_str, LM_DT_CHAR, -1, (devsw));	\
	int load_##name(struct lkm_table* lkm_tp, int cmd){	\
		switch(cmd){					\
		case WAN_MOD_LOAD: return mod_init(NULL);	\
		case WAN_MOD_UNLOAD: return mod_exit(NULL);	\
		}						\
		return -EINVAL;					\
	}							\
	int (name)(struct lkm_table* lkmtp, int cmd, int ver){\
		DISPATCH(lkmtp,cmd,ver,load_##name,load_##name,lkm_nofunc);\
	}
#elif defined(__NetBSD__)
# define WAN_MODULE_VERSION(module, version)
# define WAN_MODULE_DEPEND(module, mdepend, vmin, vpref, vmax)
# if (__NetBSD_Version__ < 200000000)
#  define WAN_MOD_DEV(name,devsw) MOD_DEV(name,LM_DT_CHAR,-1,(devsw));
# else
#  define WAN_MOD_DEV(name,devsw) MOD_DEV(name,name,NULL,-1,(devsw),-1);
# endif
# define WAN_MODULE_DEFINE(name,name_str,author,descr,lic,mod_init,mod_exit,devsw)\
	int (##name_lkmentry)(struct lkm_table* lkmtp, int cmd, int ver);\
	WAN_MOD_DEV(name_str, (devsw));				\
	int load_##name(struct lkm_table* lkm_tp, int cmd){	\
		switch(cmd){					\
		case WAN_MOD_LOAD: return mod_init(NULL);	\
		case WAN_MOD_UNLOAD: return mod_exit(NULL);	\
		}						\
		return -EINVAL;					\
	}							\
	int (##name_lkmentry)(struct lkm_table* lkmtp, int cmd, int ver){\
		DISPATCH(lkmtp,cmd,ver,load_##name,load_##name,lkm_nofunc);\
	}
#elif defined(__LINUX__)
# define WAN_MODULE_VERSION(module, version)
# define WAN_MODULE_DEPEND(module, mdepend, vmin, vpref, vmax)
# define WAN_MODULE_DEFINE(name,name_str,author,descr,lic,mod_init,mod_exit,devsw)\
	MODULE_AUTHOR (author);					\
	MODULE_DESCRIPTION (descr);				\
	MODULE_LICENSE(lic);					\
	int __init load_##name(void){return mod_init(NULL);}	\
	void __exit unload_##name(void){mod_exit(NULL);}	\
	module_init(load_##name);				\
	module_exit(unload_##name);				

#elif defined(__SOLARIS__)
# define WAN_MODULE_VERSION(module, version)
# define WAN_MODULE_DEPEND(module, mdepend, vmin, vpref, vmax)
# define WAN_MODULE_DEFINE(name,name_str,author,descr,lic,mod_init,mod_exit,devsw)\
	int _init(void){\
		int err=mod_init(NULL);				\
		if (err) return err; 				\
		err=mod_install(&modlinkage)			\
		if (err) cmn_err(CE_CONT, "mod_install: failed\n"); \
		return err;					\
	}\
	void _fini(void){					\
		int status					\
		mod_exit(NULL); 				\
		if ((status = mod_remove(&modlinkage)) != 0)	\
        		cmn_err(CE_CONT, "mod_remove: failed\n"); \
		return status;					\
	}\
	int _info(struct modinfo* modinfop)			\
	{							\
    		dcmn_err((CE_CONT, "Get module info!\n"));	\
    		return (mod_info(&modlinkage, modinfop));	\
	}
#elif defined(__WINDOWS__)
# define WAN_MODULE_VERSION(module, version)
# define WAN_MODULE_DEPEND(module, mdepend, vmin, vpref, vmax)
# define WAN_MODULE_DEFINE(name,name_str,author,descr,lic,mod_init,mod_exit,devsw)
#endif

/*
******************************************************************
**	T Y P E D E F
******************************************************************
*/

#if !defined(__WINDOWS__)
#if !defined(offsetof)
# define offsetof(type, member)	((size_t)(&((type*)0)->member))
#endif
#endif

#if defined(__LINUX__)
/**************************** L I N U X **************************************/
typedef struct sk_buff		netskb_t;
typedef struct sk_buff_head	wan_skb_queue_t;
typedef struct timer_list	wan_timer_info_t;
typedef void 			(*wan_timer_func_t)(unsigned long);
typedef unsigned long		wan_timer_arg_t;
typedef void 			wan_tasklet_func_t(unsigned long);
# if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,20))
typedef void 			(*wan_taskq_func_t)(void *);
# else
typedef void 			(*wan_taskq_func_t)(struct work_struct *);
#endif

/* Due to 2.6.20 kernel, the wan_taskq_t must be declared
 * here as a workqueue structre.  The tq_struct is declared
 * as work queue in wanpipe_kernel.h */
typedef struct tq_struct 	wan_taskq_t;

typedef void*				virt_addr_t;
typedef unsigned long		wp_phys_addr_t;
typedef spinlock_t			wan_spinlock_t;
#ifdef DEFINE_MUTEX
typedef struct mutex		wan_mutexlock_t;
#else
typedef spinlock_t			wan_mutexlock_t;
#endif
typedef rwlock_t			wan_rwlock_t;
typedef unsigned long		wan_smp_flag_t;
typedef unsigned long 		wan_rwlock_flag_t;

typedef void			(*TASKQ_FUNC)(void *);
typedef struct tty_driver	ttydriver_t;
typedef struct tty_struct	ttystruct_t;
typedef struct termios		termios_t;
# if LINUX_VERSION_CODE < KERNEL_VERSION(2,4,10)
#  define vsnprintf(a,b,c,d)	vsprintf(a,c,d)
# endif
typedef void*			wan_dma_tag_t;
typedef wait_queue_head_t	wan_waitq_head_t;
typedef void			(wan_pci_ifunc_t)(void*);
#elif defined(__FreeBSD__)
/**************************** F R E E B S D **********************************/
typedef struct ifnet		netdevice_t;
typedef struct mbuf		netskb_t;
# ifdef ALTQ
typedef struct ifaltq		wan_skb_queue_t;
# else
typedef	struct ifqueue		wan_skb_queue_t;
# endif
typedef struct ether_header	ethhdr_t;
typedef struct callout_handle	wan_timer_info_t;
typedef timeout_t*		wan_timer_func_t;
typedef void*			wan_timer_arg_t;
typedef task_fn_t		wan_tasklet_func_t;
typedef task_fn_t*		wan_taskq_func_t;
typedef caddr_t			virt_addr_t;
typedef u_int32_t		wp_phys_addr_t;
typedef int			atomic_t;
typedef dev_t			ttydriver_t;
typedef struct tty		ttystruct_t;
typedef struct termios		termios_t;
typedef int 			(get_info_t)(char *, char **, off_t, int, int);
#if defined(SPINLOCK_OLD)
typedef int			wan_spinlock_t;
#else
typedef struct mtx		wan_spinlock_t;
#endif
typedef int 			wan_rwlock_t;
typedef int			wan_smp_flag_t;
typedef int			wan_rwlock_flag_t;
# if (__FreeBSD_version < 450000)
typedef struct proc		wan_dev_thread_t;
# else
typedef d_thread_t		wan_dev_thread_t;
# endif
typedef bus_dma_tag_t		wan_dma_tag_t;
typedef int			wan_waitq_head_t;
typedef void			(wan_pci_ifunc_t)(void*);
#elif defined(__OpenBSD__)
/**************************** O P E N B S D **********************************/
typedef struct ifnet		netdevice_t;
typedef struct mbuf		netskb_t;
# ifdef ALTQ
typedef struct ifaltq		wan_skb_queue_t;
# else
typedef	struct ifqueue		wan_skb_queue_t;
# endif
typedef struct ether_header	ethhdr_t;
typedef struct timeout		wan_timer_info_t;
typedef void 			(*wan_timer_func_t)(void*);
typedef void*			wan_timer_arg_t;
typedef void 			wan_tasklet_func_t(void*, int);
typedef void 			(*wan_taskq_func_t)(void*, int);
typedef caddr_t			virt_addr_t;
typedef u_int32_t		wp_phys_addr_t;
typedef int			atomic_t;
typedef dev_t			ttydriver_t;
typedef struct tty		ttystruct_t;
typedef struct termios		termios_t;
typedef int 			(get_info_t)(char *, char **, off_t, int, int);
typedef bus_dma_tag_t		wan_dma_tag_t;
typedef int			wan_spinlock_t;
typedef int			wan_smp_flag_t;
typedef int 			wan_rwlock_t;
typedef int			wan_rwlock_flag_t;
typedef int			(wan_pci_ifunc_t)(void*);
#elif defined(__NetBSD__)
/**************************** N E T B S D **********************************/
typedef struct ifnet		netdevice_t;
typedef struct mbuf		netskb_t;
# ifdef ALTQ
typedef struct ifaltq		wan_skb_queue_t;
# else
typedef	struct ifqueue		wan_skb_queue_t;
# endif
typedef struct ether_header	ethhdr_t;
typedef struct callout		wan_timer_info_t;
typedef void 			(*wan_timer_func_t)(void*);
typedef void*			wan_timer_arg_t;
typedef void 			wan_tasklet_func_t(void*, int);
typedef void 			(*wan_taskq_func_t)(void*, int);
typedef caddr_t			virt_addr_t;
typedef u_int32_t		wp_phys_addr_t;
typedef int			atomic_t;
typedef dev_t			ttydriver_t;
typedef struct tty		ttystruct_t;
typedef struct termios		termios_t;
typedef int 			(get_info_t)(char *, char **, off_t, int, int);
typedef bus_dma_tag_t		wan_dma_tag_t;
typedef int			wan_spinlock_t;
typedef int			wan_smp_flag_t;
typedef int 			wan_rwlock_t;
typedef int			wan_rwlock_flag_t;
typedef void			(wan_pci_ifunc_t)(void*);
#elif defined(__SOLARIS__)
typedef mblk_t			netskb_t;

#elif defined(__WINDOWS__)
/**************************** W I N D O W S **********************************/

typedef struct sk_buff		netskb_t;
typedef struct sk_buff_head	wan_skb_queue_t;

typedef struct 
{
    u8 DestAddr[ETHER_ADDR_LEN];
    u8 SrcAddr[ETHER_ADDR_LEN];
    u16 EtherType;
} ethhdr_t;


typedef int		wan_rwlock_t;
typedef int		wan_rwlock_flag_t;
typedef int		pid_t;
typedef void	(wan_pci_ifunc_t)(void*);

typedef wan_spinlock_t wan_mutexlock_t;

#define WAN_IOCTL_RET_TYPE	int

#endif


/*
 * Spin Locks
 */
#if 0
typedef struct _wan_spinlock
{
#if defined(__LINUX__)
   	spinlock_t      	slock;
    	unsigned long   	flags;
#elif defined(MAC_OS)
    	ULONG           	slock;
#elif defined(__NetBSD__) || defined(__FreeBSD__) || defined(__OpenBSD__)
    	int			slock;
#endif /* OS */
} wan_spinlock_t;
#endif

/*
 * Read Write Locks
 */
#if 0
typedef struct _wan_rwlock
{
#if defined(__LINUX__)
	rwlock_t      	rwlock;
#elif defined(MAC_OS)
 	#error "wan_rwlock_t not defined"
#elif defined(__NetBSD__) || defined(__FreeBSD__) || defined(__OpenBSD__)
	volatile unsigned int lock;
#else
	#error "wan_rwlock_t not defined"    
#endif /* OS */
} wan_rwlock_t;
#endif

/*
** FIXME: Redefined from sdla_adsl.c
** DMA structure for ADSL ONLY!!!!!!! 
*/
typedef struct _wan_dma_descr_org
{
	unsigned long*		vAddr;
    unsigned long		pAddr;
    unsigned long		length;
    unsigned long		max_length;
#if defined(__FreeBSD__)
	bus_dma_tag_t		dmat;
	bus_dmamap_t		dmamap;
#elif defined(__OpenBSD__)
	bus_dma_tag_t		dmat;
	bus_dma_segment_t	dmaseg;
	int			rsegs;
#elif defined(__NetBSD__)
	bus_dma_tag_t		dmat;
	bus_dma_segment_t	dmaseg;
	int			rsegs;
#elif defined(__WINDOWS__)
	PDMA_ADAPTER	DmaAdapterObject;
#else	/* other OS */
#endif
} wan_dma_descr_org_t;/*, *PDMA_DESCRIPTION;*/

/*
** TASK structure
*/
typedef struct _wan_tasklet
{
	unsigned long		running;
#if defined(__FreeBSD__) && (__FreeBSD_version >= 410000)
	struct task		task_id;
#elif defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__)
	wan_tasklet_func_t*	task_func;
	void*			data;	
#elif defined(__LINUX__)
	struct tasklet_struct 	task_id;
#elif  defined(__WINDOWS__)
	KDPC 	tqueue;
#elif  defined(__SOLARIS__)
#error "wan_tasklet: not defined in solaris"
#endif
} wan_tasklet_t;

#if !defined(__LINUX__) && !defined(__WINDOWS__)
typedef struct _wan_taskq
{
	unsigned char		running;
#if defined(__FreeBSD__) && (__FreeBSD_version >= 410000)
	struct task		tqueue;
#elif defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__)
	wan_taskq_func_t	tfunc;
	void*			data;	
#elif defined(__LINUX__)
/* Due to 2.6.20 kernel, we cannot abstract the
 * wan_taskq_t here, we must declare it as work queue */
# error "Linux doesnt support wan_taskq_t here!"
# if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,20)) 
    	struct tq_struct 	tqueue;
# else
        struct work_struct 	tqueue;
# endif
#elif  defined(__SOLARIS__)
#error "_wan_taskq: not defined in solaris"
#endif
} wan_taskq_t;
#endif


typedef struct wan_trace
{
	u_int32_t  		tracing_enabled;
	wan_skb_queue_t		trace_queue;
	wan_ticks_t		trace_timeout;/* WARNING: has to be 'unsigned long' !!!*/
	unsigned int   		max_trace_queue;
	unsigned char		last_trace_direction;
	u_int32_t		missed_idle_rx_counter;
	u_int32_t		time_stamp;
}wan_trace_t;



/*
** TIMER structure
*/
#if !defined(__WINDOWS__)
typedef struct _wan_timer
{
#define NDIS_TIMER_TAG 0xBEEF0005
	unsigned long           Tag;
	wan_timer_func_t        MiniportTimerFunction;
	void *                  MiniportTimerContext;
	void *                  MiniportAdapterHandle;
	wan_timer_info_t	timer_info;
#if defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__)
	wan_timer_func_t	timer_func;
#endif
	void*			timer_arg;
} wan_timer_t;
#endif


#if !defined(LINUX_2_6) && !defined(__WINDOWS__)
/* Define this structure for BSDs and not Linux-2.6 */
struct seq_file {
	char*		buf;	/* pointer to buffer	(buf)*/
	size_t		size;	/* total buffer len	(len)*/
	size_t		from;	/* total buffer len	(offs)*/
	size_t		count;	/* offset into buffer	(cnt)*/
# if defined(__NetBSD__) || defined(__FreeBSD__) || defined(__OpenBSD__)
	unsigned long	index;	/* iteration index*/
# else
	loff_t		index;	/* iteration index*/
# endif
	int		stop_cnt;/* last stop offset*/
};
#endif


#pragma pack(1)
#if defined(WAN_BIG_ENDIAN) || (1)

/* We use BIG ENDIAN because
   RTP TAP needs to be transmitted 
   BIG ENDIAN */

typedef struct {
  uint8_t cc:4;	/* CSRC count             */
  uint8_t x:1;		/* header extension flag  */
  uint8_t p:1;		/* padding flag           */
  uint8_t version:2;	/* protocol version       */
  uint8_t pt:7;	/* payload type           */
  uint8_t m:1;		/* marker bit             */
  uint16_t seq;		/* sequence number        */
  uint32_t ts;		/* timestamp              */
  uint32_t ssrc;	/* synchronization source */
} wan_rtp_hdr_t;

#else /*  BIG_ENDIAN */

/* not used */

typedef struct {
  uint8_t version:2;	/* protocol version       */
  uint8_t p:1;		/* padding flag           */
  uint8_t x:1;		/* header extension flag  */
  uint8_t cc:4;	/* CSRC count             */
  uint8_t m:1;		/* marker bit             */
  uint8_t pt:7;	/* payload type           */
  uint16_t seq;		/* sequence number        */
  uint32_t ts;		/* timestamp              */
  uint32_t ssrc;	/* synchronization source */
} wan_rtp_hdr_t;

#endif

typedef struct wan_rtp_pkt {
	ethhdr_t	eth_hdr;
	iphdr_t		ip_hdr;
	udphdr_t	udp_hdr;       
	wan_rtp_hdr_t	rtp_hdr;
#define wan_eth_dest			eth_hdr.w_eth_dest	
#define wan_eth_src 			eth_hdr.w_eth_src
#define wan_eth_proto  			eth_hdr.w_eth_proto         
} wan_rtp_pkt_t;

#pragma pack()

#if (!defined __WINDOWS__)
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,0,0)
#define LINUX_HAS_NET_DEVICE_OPS
#endif
#endif

//#if defined(HAVE_NET_DEVICE_OPS) || LINUX_VERSION_CODE >= KERNEL_VERSION(3,0,0)
#if defined(HAVE_NET_DEVICE_OPS) || defined(LINUX_HAS_NET_DEVICE_OPS)
#define WAN_DECLARE_NETDEV_OPS(_ops_name) static struct net_device_ops _ops_name = {0};

#define WAN_NETDEV_OPS_BIND(dev,_ops_name)  dev->netdev_ops = &_ops_name

#define WAN_NETDEV_OPS_INIT(dev,ops,wan_init)				ops.ndo_init = wan_init
#define WAN_NETDEV_OPS_OPEN(dev,ops,wan_open)				ops.ndo_open = wan_open
#define WAN_NETDEV_OPS_STOP(dev,ops,wan_stop)				ops.ndo_stop = wan_stop
#define WAN_NETDEV_OPS_XMIT(dev,ops,wan_send)				ops.ndo_start_xmit = wan_send
#define WAN_NETDEV_OPS_STATS(dev,ops,wan_stats)				ops.ndo_get_stats = wan_stats
#define WAN_NETDEV_OPS_TIMEOUT(dev,ops,wan_timeout)			ops.ndo_tx_timeout = wan_timeout
#define WAN_NETDEV_OPS_IOCTL(dev,ops,wan_ioctl)				ops.ndo_do_ioctl = wan_ioctl
#define WAN_NETDEV_OPS_MTU(dev,ops,wan_mtu)				ops.ndo_change_mtu = wan_mtu
#define WAN_NETDEV_OPS_CONFIG(dev,ops,wan_set_config)			ops.ndo_set_config = wan_set_config
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,1,0)
#define WAN_NETDEV_OPS_SET_MULTICAST_LIST(dev,ops,wan_multicast_list)	
#else
#define WAN_NETDEV_OPS_SET_MULTICAST_LIST(dev,ops,wan_multicast_list)	ops.ndo_set_multicast_list = wan_multicast_list
#endif
//#define WAN_CHANGE_MTU(dev)						dev->netdev_ops->ndo_change_mtu
//#define WAN_XMIT(dev)                                               	dev->netdev_ops->ndo_start_xmit
//#define WAN_IOCTL(dev)						dev->netdev_ops->ndo_do_ioctl
#define WAN_NETDEV_TEST_XMIT(dev)					dev->netdev_ops->ndo_start_xmit
#define WAN_NETDEV_XMIT(skb,dev)					dev->netdev_ops->ndo_start_xmit(skb,dev)
#define WAN_NETDEV_TEST_IOCTL(dev)					dev->netdev_ops->ndo_do_ioctl
#define WAN_NETDEV_IOCTL(dev,ifr,cmd)					dev->netdev_ops->ndo_do_ioctl(dev,ifr,cmd)
#define WAN_NETDEV_TEST_MTU(dev)					dev->netdev_ops->ndo_change_mtu
#define WAN_NETDEV_CHANGE_MTU(dev,skb)					dev->netdev_ops->ndo_change_mtu(dev,skb)

#else

#define WAN_DECLARE_NETDEV_OPS(_ops_name) 
#define WAN_NETDEV_OPS_BIND(dev,_ops_name)
#define WAN_NETDEV_OPS_INIT(dev,ops,wan_init)				dev->init = wan_init
#define WAN_NETDEV_OPS_OPEN(dev,ops,wan_open)				dev->open = wan_open	
#define WAN_NETDEV_OPS_STOP(dev,ops,wan_stop)				dev->stop = wan_stop
#define WAN_NETDEV_OPS_XMIT(dev,ops,wan_send)				dev->hard_start_xmit = wan_send
#define WAN_NETDEV_OPS_STATS(dev,ops,wan_stats)				dev->get_stats = wan_stats
#define WAN_NETDEV_OPS_TIMEOUT(dev,ops,wan_timeout)			dev->tx_timeout = wan_timeout
#define WAN_NETDEV_OPS_IOCTL(dev,ops,wan_ioctl)				dev->do_ioctl = wan_ioctl
#define WAN_NETDEV_OPS_MTU(dev,ops,wan_mtu)				dev->change_mtu = wan_mtu
#define WAN_NETDEV_OPS_CONFIG(dev,ops,wan_set_config)			dev->set_config = wan_set_config
#define WAN_NETDEV_OPS_SET_MULTICAST_LIST(dev,ops,wan_multicast_list)	dev->set_multicast_list = wan_multicast_list
//#define WAN_CHANGE_MTU(dev)						dev->change_mtu
//#define WAN_XMIT(dev)                                                 dev->hard_start_xmit
//#define WAN_IOCTL(dev)                                                dev->do_ioctl
#define WAN_NETDEV_TEST_XMIT(dev)					dev->hard_start_xmit
#define WAN_NETDEV_XMIT(skb,dev)					dev->hard_start_xmit(skb,dev)
#define WAN_NETDEV_TEST_IOCTL(dev)					dev->do_ioctl
#define WAN_NETDEV_IOCTL(dev,ifr,cmd)					dev->do_ioctl(dev,ifr,cmd)
#define WAN_NETDEV_TEST_MTU(dev)					dev->change_mtu
#define WAN_NETDEV_CHANGE_MTU(dev,skb)					dev->change_mtu(dev,skb)

#endif /* HAVE_NET_DEVICE_OPS */

#endif /* KERNEL */ 

#endif /* __WANPIPE_DEFINES_H */
