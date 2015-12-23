/*
 ************************************************************************
 * wanpipe_debug.h	WANPIPE(tm) 	Global definition for Sangoma 		*
 *					Debugging messages									*
 *																		*
 * Authors:		Alex Feldman <al.feldman@sangoma.com>					*
 *				David Rokhvarg <davidr@sangoma.com>						*
 *======================================================================*
 *																		*
 *	September	21 2009		David Rokhvarg								*
 *		Added Wanpipe Logger definitions.								*
 *		Improved cross-platform macro definitions for driver debugging.	*
 *																		*
 *	May			10 2002		Alex Feldman								*
 *		Initial version													*
 *																		*
 ************************************************************************
 */

#ifndef __WANPIPE_DEBUG_H
# define __WANPIPE_DEBUG_H


#if defined(WAN_KERNEL)

#include "wanpipe_logger.h"


/* NC: If the undefs are used here, one cannot
   use the Makefile to enable them.  Code is left
   here as overview of all available options. Please
   use the Makefile to enable them on compile time. */

#if 0
#undef WAN_DEBUG_TE1
#undef WAN_DEBUG_HWEC
#undef WAN_DEBUG_TDMAPI
#undef WAN_DEBUG_BRI

#undef WAN_DEBUG_KERNEL
#undef WAN_DEBUG_MOD
#undef WAN_DEBUG_CFG
#undef WAN_DEBUG_REG
#undef WAN_DEBUG_INIT_VAR
#undef WAN_DEBUG_IOCTL
#undef WAN_DEBUG_CMD
#undef WAN_DEBUG_ISR
#undef WAN_DEBUG_RX
#undef WAN_DEBUG_RX_ERROR
#undef WAN_DEBUG_TX
#undef WAN_DEBUG_TX_ERROR
#undef WAN_DEBUG_TIMER
#undef WAN_DEBUG_UDP
#undef WAN_DEBUG_56K
#undef WAN_DEBUG_A600
#undef WAN_DEBUG_PROCFS
#undef WAN_DEBUG_TDM_VOICE
#undef WAN_DEBUG_TEST
#undef WAN_DEBUG_DBG
#undef WAN_DEBUG_DMA
#undef WAN_DEBUG_SNMP
#undef WAN_DEBUG_TE3
#undef WAN_DEBUG_RM
#undef WAN_DEBUG_FE
#undef WAN_DEBUG_NG
#undef WAN_DEBUG_MEM
#undef WAN_DEBUG_BRI_INIT
#undef WAN_DEBUG_USB
#undef WAN_DEBUG_FUNC
#endif


#define WAN_DEBUG_EVENT	/* must be defined for wpabs_debug_event() */

#define AFT_FUNC_DEBUG() 
#define	WAN_KRN_BREAK_POINT()

#if defined (__WINDOWS__)
extern void OutputLogString(const char *fmt, ...); /* Print to wanpipelog.txt (NOT to Debugger). */
# define DEBUG_PRINT(...)	OutputLogString(## __VA_ARGS__)
# define _DEBUG_PRINT(...)	OutputLogString(## __VA_ARGS__)

# undef		WAN_KRN_BREAK_POINT
# define	WAN_KRN_BREAK_POINT()	if(0)DbgBreakPoint()
#endif

#if (defined __FreeBSD__) || (defined __OpenBSD__) || defined(__NetBSD__)
# define DEBUG_PRINT(format,msg...)		log(LOG_INFO, format, ##msg)
# define _DEBUG_PRINT(format,msg...)	log(LOG_INFO, format, ##msg)
#endif

#if defined (__LINUX__)
# define DEBUG_PRINT(...)	printk(KERN_INFO ## __VA_ARGS__)
# define _DEBUG_PRINT(...)	printk(## __VA_ARGS__)
#endif

#if 1
# define DBG_BATTERY_REMOVAL 	DEBUG_TEST
#else
# if defined (__WINDOWS__)
#  define DBG_BATTERY_REMOVAL	if(1)DbgPrint
# else
#  define DBG_BATTERY_REMOVAL	if(1)DEBUG_EVENT
# endif
#endif

#if defined (__WINDOWS__)
# define DEBUG_TASKQ	if(0)DbgPrint
#else
# define DEBUG_TASKQ	if(0)DEBUG_EVENT
#endif

/*========================================================
  COMMON CODE
 *========================================================*/

#define DEBUG_KERNEL(...)	
#define DEBUG_MOD(...)		 
#define DEBUG_CFG(...)		
#define DEBUG_REG(...)		
#define DEBUG_INIT(...)		
#define DEBUG_IOCTL(...)	
#define DEBUG_CMD(...)		
#define DEBUG_ISR(...)		
#define DEBUG_RX(...)		
#define DEBUG_RX_ERR(...)	
#define DEBUG_TX(...)		
#define _DEBUG_TX(...)		
#define DEBUG_TX_ERR(...)   
#define DEBUG_TIMER(...)    
#define DEBUG_UDP(...)		
#define DEBUG_TE3(...)		
#define DEBUG_56K(...)		
#define DEBUG_A600(...)		
#define DEBUG_PROCFS(...)	
#define DEBUG_TDMV(...)		
#define DEBUG_TEST(...)		
#define DEBUG_DBG(...)		
#define DEBUG_DMA(...)		
#define DEBUG_SNMP(...)		
#define DEBUG_RM(...)		
#define DEBUG_NG(...)		
#define DEBUG_BRI_INIT(...)	
#define DEBUG_USB(...)		
#define _DEBUG_EVENT(...)	

#define DEBUG_ADD_MEM(a)
#define DEBUG_SUB_MEM(a)
#define WAN_DEBUG_FUNC_START
#define WAN_DEBUG_FUNC_END
#define WAN_DEBUG_FUNC_LINE


#if 0
# undef _DEBUG_EVENT
# if 1
#  define _DEBUG_EVENT(...)				_DEBUG_PRINT(...)
# else
#  define _DEBUG_EVENT(format,msg...)	_DEBUG_PRINT(format,##msg)
# endif
#endif

#ifdef WAN_DEBUG_KERNEL
# undef  DEBUG_KERNEL
# define DEBUG_KERNEL(...)	DEBUG_PRINT(## __VA_ARGS__)
#endif 
#ifdef WAN_DEBUG_MOD
# undef  DEBUG_MOD
# define DEBUG_MOD(...)		DEBUG_PRINT(## __VA_ARGS__)
#endif 
#ifdef WAN_DEBUG_CFG
# undef  DEBUG_CFG
# define DEBUG_CFG(...)		DEBUG_PRINT(## __VA_ARGS__)
#endif 
#ifdef WAN_DEBUG_REG
# undef  DEBUG_REG
# define DEBUG_REG(...)		DEBUG_PRINT(## __VA_ARGS__)
#endif 
#ifdef WAN_DEBUG_INIT_VAR
# undef  DEBUG_INIT
# define DEBUG_INIT(...)	DEBUG_PRINT(## __VA_ARGS__)
#endif
#ifdef WAN_DEBUG_IOCTL
# undef  DEBUG_IOCTL
# define DEBUG_IOCTL(...)	DEBUG_PRINT(## __VA_ARGS__)
#endif
#ifdef WAN_DEBUG_CMD
# undef  DEBUG_CMD
# define DEBUG_CMD(...)		DEBUG_PRINT(## __VA_ARGS__)
#endif
#ifdef WAN_DEBUG_ISR
# undef  DEBUG_ISR
# define DEBUG_ISR(...)		DEBUG_PRINT(## __VA_ARGS__)
#endif 
#ifdef WAN_DEBUG_RX
# undef  DEBUG_RX
# define DEBUG_RX(...)		DEBUG_PRINT(## __VA_ARGS__)
#endif 
#ifdef WAN_DEBUG_RX_ERROR
# undef  DEBUG_RX_ERR
# define DEBUG_RX_ERR		DEBUG_PRINT(## __VA_ARGS__)
#endif 
#ifdef WAN_DEBUG_TX
# undef  DEBUG_TX
# define DEBUG_TX(...)		DEBUG_PRINT(## __VA_ARGS__)
# undef  _DEBUG_TX
# define _DEBUG_TX(...)		_DEBUG_PRINT(## __VA_ARGS__)
#endif 
#ifdef WAN_DEBUG_TX_ERROR
# undef  DEBUG_TX_ERR
# define DEBUG_TX_ERR(...)	DEBUG_PRINT(## __VA_ARGS__)
#endif 
#ifdef WAN_DEBUG_TIMER
# undef  DEBUG_TIMER
# define DEBUG_TIMER(...)	DEBUG_PRINT(## __VA_ARGS__)
#endif 
#ifdef WAN_DEBUG_UDP
# undef  DEBUG_UDP
# define DEBUG_UDP(...)		DEBUG_PRINT(## __VA_ARGS__)
#endif 
#ifdef WAN_DEBUG_TE3
# undef  DEBUG_TE3
# define DEBUG_TE3(...)		DEBUG_PRINT(## __VA_ARGS__)
#endif 
#ifdef WAN_DEBUG_56K
# undef  DEBUG_56K
# define DEBUG_56K(...)		DEBUG_PRINT(## __VA_ARGS__)
#endif 
#ifdef WAN_DEBUG_A600
# undef  DEBUG_A600
# define DEBUG_A600(...)	DEBUG_PRINT(## __VA_ARGS__)
#endif 
#ifdef WAN_DEBUG_PROCFS
# undef  DEBUG_PROCFS
# define DEBUG_PROCFS(...)	DEBUG_PRINT(## __VA_ARGS__)
#endif 
#ifdef WAN_DEBUG_TDM_VOICE
# undef  DEBUG_TDMV
# define DEBUG_TDMV(...)	DEBUG_PRINT(## __VA_ARGS__)
#endif 
#ifdef WAN_DEBUG_TEST
# undef  DEBUG_TEST
# define DEBUG_TEST(...)	DEBUG_PRINT(## __VA_ARGS__)
#endif
#ifdef WAN_DEBUG_DBG
# undef  DEBUG_DBG
# define DEBUG_DBG(...)		DEBUG_PRINT(## __VA_ARGS__)
#endif
#ifdef WAN_DEBUG_DMA
# undef  DEBUG_DMA
# define DEBUG_DMA(...)		DEBUG_PRINT(## __VA_ARGS__)
#endif
#ifdef WAN_DEBUG_SNMP
# undef  DEBUG_SNMP
# define DEBUG_SNMP(...)	DEBUG_PRINT(## __VA_ARGS__)
#endif
#ifdef WAN_DEBUG_RM
# undef  DEBUG_RM
# define DEBUG_RM(...)		DEBUG_PRINT(## __VA_ARGS__)
#endif 
#ifdef WAN_DEBUG_NG
# undef  DEBUG_NG
# define DEBUG_NG(...)		DEBUG_PRINT(## __VA_ARGS__)
#endif 
#ifdef WAN_DEBUG_BRI_INIT
# undef  DEBUG_BRI_INIT
# define DEBUG_BRI_INIT(...) DEBUG_PRINT(## __VA_ARGS__)
#endif 
#ifdef WAN_DEBUG_USB
# undef  DEBUG_USB
# define DEBUG_USB(...)		DEBUG_PRINT(## __VA_ARGS__)
#endif                                   


/*=================================================*/
/* general Wanpipe Logger macros */
#ifdef WP_LOGGER_DISABLE

/* Debug case in order to check print argument mismatches */
#ifdef __LINUX__
#warning "WP_LOGGER_DISABLE Enabled"
#endif

#ifdef WAN_DEBUG_EVENT_AS_KERN_DEBUG
#define DEBUG_EVENT(format,msg...)		printk(KERN_DEBUG format, ##msg)      
#else
#define DEBUG_EVENT(format,msg...)		printk(KERN_INFO format, ##msg)      
#endif
#define DEBUG_WARNING(format,msg...)	printk(KERN_WARNING format, ##msg)      
#define DEBUG_ERROR(format,msg...)		printk(KERN_ERR format, ##msg)      

#else

/* Default case */

#define DEBUG_EVENT(...)	\
	WP_DEBUG(WAN_LOGGER_DEFAULT, SANG_LOGGER_INFORMATION, ## __VA_ARGS__)
#define DEBUG_WARNING(...)	\
	WP_DEBUG(WAN_LOGGER_DEFAULT, SANG_LOGGER_WARNING, ## __VA_ARGS__)
#define DEBUG_ERROR(...)	\
	WP_DEBUG(WAN_LOGGER_DEFAULT, SANG_LOGGER_ERROR, ## __VA_ARGS__)
#endif


/***************************************/
/* task-specific Wanpipe Logger macros */

/* T1/E1 */
#define DEBUG_TE1(...)	\
	WP_DEBUG(WAN_LOGGER_TE1, SANG_LOGGER_TE1_DEFAULT, ## __VA_ARGS__)

/* Hardware Echo Canceller */
#define DEBUG_HWEC(...)	\
	WP_DEBUG(WAN_LOGGER_HWEC, SANG_LOGGER_HWEC_DEFAULT, ## __VA_ARGS__)

/* TDM API */
#define DEBUG_TDMAPI(...)	\
	WP_DEBUG(WAN_LOGGER_TDMAPI, SANG_LOGGER_TDMAPI_DEFAULT, ## __VA_ARGS__)

/* General Front End code */
#define DEBUG_FE(...)	\
	WP_DEBUG(WAN_LOGGER_FE, SANG_LOGGER_FE_DEFAULT, ## __VA_ARGS__)

/* BRI */
#define DEBUG_BRI(...)	\
	WP_DEBUG(WAN_LOGGER_BRI, SANG_LOGGER_BRI_DEFAULT, ## __VA_ARGS__)
#define DEBUG_HFC_S0_STATES(...)	\
	WP_DEBUG(WAN_LOGGER_BRI, SANG_LOGGER_BRI_HFC_S0_STATES, ## __VA_ARGS__)
#define DEBUG_L2_TO_L1_ACTIVATION(...)	\
	WP_DEBUG(WAN_LOGGER_BRI, SANG_LOGGER_BRI_L2_TO_L1_ACTIVATION, ## __VA_ARGS__)

/*==== End of Wanpipe Logger macro definitions ====*/
/*=================================================*/


#define WAN_DEBUG_FLINE	DEBUG_EVENT("[%s]: %s:%d\n",			\
				__FILE__,__FUNCTION__,__LINE__);

#if defined(WAN_DEBUG_FUNC)
# undef WAN_DEBUG_FUNC_START
# define WAN_DEBUG_FUNC_START	DEBUG_EVENT("[%s]: %s:%d: Start (%d)\n",\
	       		__FILE__,__FUNCTION__,__LINE__, (unsigned int)SYSTEM_TICKS);
# undef WAN_DEBUG_FUNC_END
# define WAN_DEBUG_FUNC_END	DEBUG_EVENT("[%s]: %s:%d: End (%d)\n",	\
	       		__FILE__,__FUNCTION__,__LINE__,(unsigned int)SYSTEM_TICKS);
# undef WAN_DEBUG_FUNC_LINE
# define WAN_DEBUG_FUNC_LINE	DEBUG_EVENT("[%s]: %s:%d: (%d)\n",	\
	       		__FILE__,__FUNCTION__,__LINE__,(unsigned int)SYSTEM_TICKS);

#define BRI_FUNC()	if(0)DEBUG_EVENT("%s(): line:%d\n", __FUNCTION__, __LINE__)
#else
# define BRI_FUNC()
#endif /* WAN_DEBUG_FUNC */

#define WAN_ASSERT(val) if (val){					\
	DEBUG_EVENT("************** ASSERT FAILED **************\n");	\
	DEBUG_EVENT("%s:%d - Critical error\n",__FILE__,__LINE__);	\
	WAN_KRN_BREAK_POINT();	\
	return -EINVAL;							\
			}
#define WAN_ASSERT_EINVAL(val) WAN_ASSERT(val)

#define WAN_ASSERT1(val) if (val){					\
	DEBUG_EVENT("************** ASSERT FAILED **************\n");	\
	DEBUG_EVENT("%s:%d - Critical error\n",__FILE__,__LINE__);	\
	return;								\
			}
#define WAN_ASSERT_VOID(val) WAN_ASSERT1(val)

#define WAN_ASSERT2(val, ret) if (val){					\
	DEBUG_EVENT("************** ASSERT FAILED **************\n");	\
	DEBUG_EVENT("%s:%d - Critical error\n",__FILE__,__LINE__);	\
	return ret;							\
			}
#define WAN_ASSERT_RC(val,ret) WAN_ASSERT2(val, ret)

#define WAN_MEM_ASSERT(str) {if (str){					\
		DEBUG_EVENT("%s: Error: No memory in %s:%d\n",		\
					str,__FILE__,__LINE__);		\
	}else{								\
		DEBUG_EVENT("wanpipe: Error: No memory in %s:%d\n",	\
					__FILE__,__LINE__);		\
	}								\
	}

#define WAN_OPP_FLAG_ASSERT(val,cmd) if (val){				\
	DEBUG_EVENT("%s:%d - Critical error: Opp Flag Set Cmd=0x%x!\n",	\
					__FILE__,__LINE__,cmd);		\
	}


#if defined(__FreeBSD__)
# ifndef WAN_SKBDEBUG
#  define WAN_SKBDEBUG 0
# endif
# define WAN_SKBCRITASSERT(mm) if (WAN_SKBDEBUG){			\
	if ((mm) == NULL){							\
		panic("%s:%d: MBUF is NULL!\n", 				\
					__FUNCTION__,__LINE__);		\
	}									\
	if (((mm)->m_flags & (M_PKTHDR|M_EXT)) != (M_PKTHDR|M_EXT)){	\
		panic("%s:%d: Invalid MBUF m_flags=%X (m=%p)\n",	\
					__FUNCTION__,__LINE__,		\
					(mm)->m_flags,(mm));			\
	}									\
	if ((unsigned long)(mm)->m_data < 0x100){				\
		panic("%s:%d: Invalid MBUF m_data=%p (m=%p)\n",		\
					__FUNCTION__,__LINE__,		\
					(mm)->m_data,(mm));			\
	}									\
}
#else
# define WAN_SKBCRITASSERT(mm)
#endif

#define WAN_MEM_INIT(id)	unsigned long mem_in_used_##id = 0x0l
#define WAN_MEM_INC(id,size)	mem_in_used_##id += size
#define WAN_MEM_DEC(id,size)	mem_in_used_##id -= size

/* WANPIPE debugging states */
#define WAN_DEBUGGING_NONE		0x00
#define WAN_DEBUGGING_AGAIN		0x01
#define WAN_DEBUGGING_START		0x02
#define WAN_DEBUGGING_CONT		0x03
#define WAN_DEBUGGING_PROTOCOL		0x04
#define WAN_DEBUGGING_END		0x05

/* WANPIPE debugging delay */
#define WAN_DEBUGGING_DELAY		60

/* WANPIPE debugging messages */
#define WAN_DEBUG_NONE_MSG		0x00
#define WAN_DEBUG_ALARM_MSG		0x01
#define WAN_DEBUG_TE1_MSG		0x02
#define WAN_DEBUG_TE3_MSG		0x02
#define WAN_DEBUG_LINERROR_MSG		0x03
#define WAN_DEBUG_CLK_MSG		0x04
#define WAN_DEBUG_TX_MSG		0x05
#define WAN_DEBUG_FR_CPE_MSG		0x06
#define WAN_DEBUG_FR_NODE_MSG		0x07
#define WAN_DEBUG_PPP_LCP_MSG		0x08
#define WAN_DEBUG_PPP_NAK_MSG		0x09
#define WAN_DEBUG_PPP_NEG_MSG		0x0A
#define WAN_DEBUG_CHDLC_KPLV_MSG	0x0B	
#define WAN_DEBUG_CHDLC_UNKNWN_MSG	0x0C	

/* WAN DEBUG timer */
#define WAN_DEBUG_INIT(card){						\
	wan_tasklet_t* debug_task = &card->debug_task;			\
	WAN_TASKLET_INIT(debug_task, 0, &wanpipe_debugging, card);	\
	wan_clear_bit(0, (unsigned long*)&card->debug_running);		\
	wanpipe_debug_timer_init(card);					\
	}
#define WAN_DEBUG_END(card){						\
	wan_del_timer(&card->debug_timer);				\
	WAN_TASKLET_KILL(&card->debug_task);				\
	}
#define WAN_DEBUG_STOP(card)	wan_clear_bit(0, &card->debug_running)

#if defined(__NetBSD__) || defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__WINDOWS__)
# define WAN_DEBUG_START(card)						\
		if (!wan_test_bit(0, &card->debug_running)){		\
			wan_set_bit(0, &card->debug_running);		\
			wan_add_timer(&card->debug_timer, 5*HZ);	\
		}
#elif defined(__LINUX__)
# define WAN_DEBUG_START(card)						\
		if (!wan_test_and_set_bit(0, &card->debug_running)){	\
			wan_add_timer(&card->debug_timer, 5*HZ);	\
		}
#else
# error "Undefined WAN_DEBUG_START macro!"
#endif

#if defined(__OpenBSD__) && (OpenBSD >= 200611)
# define WP_READ_LOCK(lock,flag)   {					\
	DEBUG_TEST("%s:%d: RLock %u\n",__FILE__,__LINE__,(u32)lock);	\
	flag = splnet(); }
				     
# define WP_READ_UNLOCK(lock,flag) {					\
	DEBUG_TEST("%s:%d: RULock %u\n",__FILE__,__LINE__,(u32)lock);	\
	splx(flag);}

# define WP_WRITE_LOCK(lock,flag) {					\
	DEBUG_TEST("%s:%d: WLock %u\n",__FILE__,__LINE__,(u32)lock); 	\
	flag = splnet(); }

# define WP_WRITE_UNLOCK(lock,flag) {					\
	DEBUG_TEST("%s:%d: WULock %u\n",__FILE__,__LINE__,(u32)lock); 	\
	splx(flag); }

#elif defined(__NetBSD__) || defined(__FreeBSD__) || defined(__OpenBSD__)
# define WP_READ_LOCK(lock,flag)   {					\
	DEBUG_TEST("%s:%d: RLock %u\n",__FILE__,__LINE__,(u32)lock);	\
	flag = splimp(); }
				     
# define WP_READ_UNLOCK(lock,flag) {					\
	DEBUG_TEST("%s:%d: RULock %u\n",__FILE__,__LINE__,(u32)lock);	\
	splx(flag);}

# define WP_WRITE_LOCK(lock,flag) {					\
	DEBUG_TEST("%s:%d: WLock %u\n",__FILE__,__LINE__,(u32)lock); 	\
	flag = splimp(); }

# define WP_WRITE_UNLOCK(lock,flag) {					\
	DEBUG_TEST("%s:%d: WULock %u\n",__FILE__,__LINE__,(u32)lock); 	\
	splx(flag); }

#elif defined(__WINDOWS__)

# define WP_READ_LOCK(lock,flag)   {							\
	DEBUG_TEST("%s:%d: RLock 0x%p\n",__FILE__,__LINE__,lock);	\
	flag = splimp(); }
				     
# define WP_READ_UNLOCK(lock,flag) {							\
	DEBUG_TEST("%s:%d: RULock 0x%p\n",__FILE__,__LINE__,lock);	\
	splx(flag);}

# define WP_WRITE_LOCK(lock,flag) {								\
	DEBUG_TEST("%s:%d: WLock 0x%p\n",__FILE__,__LINE__,lock); 	\
	flag = splimp(); }

# define WP_WRITE_UNLOCK(lock,flag) {							\
	DEBUG_TEST("%s:%d: WULock 0x%p\n",__FILE__,__LINE__,lock); \
	splx(flag); }

#elif defined(__LINUX__)

# define WAN_TIMEOUT(sec)  { unsigned long timeout; \
	                     timeout=jiffies; \
			     while ((jiffies-timeout)<sec*HZ){ \
				     schedule(); \
			     }\
			   }

# define WP_READ_LOCK(lock,flag)   {  DEBUG_TEST("%s:%d: RLock %u\n",__FILE__,__LINE__,(u32)lock);\
	                             read_lock((lock)); flag=0; }
				     
# define WP_READ_UNLOCK(lock,flag) {  DEBUG_TEST("%s:%d: RULock %u\n",__FILE__,__LINE__,(u32)lock);\
					read_unlock((lock)); flag=0; }

# define WP_WRITE_LOCK(lock,flag) {  DEBUG_TEST("%s:%d: WLock %u\n",__FILE__,__LINE__,(u32)lock); \
	                            write_lock_irqsave((lock),flag); }

# define WP_WRITE_UNLOCK(lock,flag) { DEBUG_TEST("%s:%d: WULock %u\n",__FILE__,__LINE__,(u32)lock); \
	                             write_unlock_irqrestore((lock),flag); }

#else
# error "Undefined WAN_DEBUG_START macro!"
#endif

#if defined(__LINUX__) && defined(WP_FUNC_DEBUG)

#define WP_USEC_DEFINE()            unsigned long wptimeout; struct timeval  wptv1,wptv2;
#define WP_START_TIMING()           wptimeout=jiffies; do_gettimeofday(&wptv1);
#define WP_STOP_TIMING_TEST(label,usec) { do_gettimeofday(&wptv2);\
                                    wptimeout=jiffies-wptimeout; \
		                    if (wptimeout >= 2){ \
                        	 	DEBUG_EVENT("%s:%u %s Jiffies=%lu\n",  \
                                        	__FUNCTION__,__LINE__,label,wptimeout);  \
		                    }\
				     \
                		    wptimeout=wptv2.tv_usec - wptv1.tv_usec; \
                                    if (wptimeout >= usec){ \
					DEBUG_EVENT("%s:%u %s:%s Usec=%lu\n",  \
                                                __FUNCTION__,__LINE__,card->devname,label,wptimeout);  \
                		    }\
                                  }

#else

#define WP_USEC_DEFINE()              
#define WP_START_TIMING()             
#define WP_STOP_TIMING_TEST(label,usec) 

#endif


static __inline void debug_print_skb_pkt(unsigned char *name, unsigned char *data, int len, int direction)
{
#if defined(__LINUX__) && defined(__KERNEL__)
	int i;
	printk(KERN_INFO "%s: PKT Len(%i) Dir(%s)\n",name,len,direction?"RX":"TX");
	printk(KERN_INFO "%s: DATA: ",name);
	for (i=0;i<len;i++){
		printk("%02X ", data[i]);
	}
	printk("\n");
#endif
}


#if 0

static __inline void debug_print_udp_pkt(unsigned char *data,int len,char trc_enabled, char direction)
{
#if defined(__LINUX__) && defined(__KERNEL__)
	int i,res;
	DEBUG_EVENT("\n");
	DEBUG_EVENT("%s UDP PACKET: ",direction?"RX":"TX");
	for (i=0; i<sizeof(wan_udp_pkt_t); i++){
		if (i==0){
			DEBUG_EVENT("\n");
			DEBUG_EVENT("IP PKT: ");
		}
		if (i==sizeof(struct iphdr)){
			DEBUG_EVENT("\n");
			DEBUG_EVENT("UDP PKT: ");
		}
		if (i==sizeof(struct iphdr)+sizeof(struct udphdr)){
			DEBUG_EVENT("\n");
			DEBUG_EVENT("MGMT PKT: ");
		}
		if (i==sizeof(struct iphdr)+sizeof(struct udphdr)+sizeof(wan_mgmt_t)){
			DEBUG_EVENT("\n");
			DEBUG_EVENT("CMD PKT: ");
		}
		
		if (trc_enabled){
			if (i==sizeof(struct iphdr)+sizeof(struct udphdr)+
			       sizeof(wan_mgmt_t)+sizeof(wan_cmd_t)){
				DEBUG_EVENT("\n");
				DEBUG_EVENT("TRACE PKT: ");
			}
			if (i==sizeof(struct iphdr)+sizeof(struct udphdr)+
			       sizeof(wan_mgmt_t)+sizeof(wan_cmd_t)+
			       sizeof(wan_trace_info_t)){

				DEBUG_EVENT("\n");
				DEBUG_EVENT("DATA PKT: ");
			}

			res=len-(sizeof(struct iphdr)+sizeof(struct udphdr)+
			       sizeof(wan_mgmt_t)+sizeof(wan_cmd_t)+sizeof(wan_trace_info_t));
		
			res=(res>10)?10:res;

			if (i==sizeof(struct iphdr)+sizeof(struct udphdr)+
			       sizeof(wan_mgmt_t)+sizeof(wan_cmd_t)+sizeof(wan_trace_info_t)+res){
				break;
			}
			
		}else{
	
			if (i==sizeof(struct iphdr)+sizeof(struct udphdr)+sizeof(wan_mgmt_t)+sizeof(wan_cmd_t)){
				DEBUG_EVENT("\n");
				DEBUG_EVENT("DATA PKT: ");
			}

			res=len-(sizeof(struct iphdr)+sizeof(struct udphdr)+
			       sizeof(wan_mgmt_t)+sizeof(wan_cmd_t));
		
			res=(res>10)?10:res;

			if (i==sizeof(struct iphdr)+sizeof(struct udphdr)+
			       sizeof(wan_mgmt_t)+sizeof(wan_cmd_t)+res){
				break;
			}
		}

		DEBUG_EVENT("%02X ",*(data+i));
	}
	DEBUG_EVENT("\n");
#endif
}

#endif




typedef struct wanpipe_debug_hdr {
	unsigned long	magic;
	unsigned long	total_len;
} wanpipe_kernel_msg_info_t;

#define WAN_DEBUG_SET_TRIGGER	0x01
#define WAN_DEBUG_CLEAR_TRIGGER	0x02

#define WAN_DEBUG_READING	0x00
#define WAN_DEBUG_FULL		0x01
#define WAN_DEBUG_TRIGGER	0x02

extern void wan_debug_trigger(int);
extern void wan_debug_write(char*);
extern int wan_debug_read(void*, void*);

/* NC Added to debug function calls */
#if 0
extern void wp_debug_func_init(void);
extern void wp_debug_func_add(unsigned char *func);
extern void wp_debug_func_print(void);
#endif

#endif /* WAN_KERNEL */
#endif /* __WANPIPE_DEBUG_H */
