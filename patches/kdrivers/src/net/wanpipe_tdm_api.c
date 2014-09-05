/*****************************************************************************
* wanpipe_tdm_api.c
*
* 		WANPIPE(tm) AFT TE1 Hardware Support
*
* Authors: 	Nenad Corbic <ncorbic@sangoma.com>
*			David Rokhvarg	<davidr@sangoma.com>
*
* Copyright:	(c) 2003-2010 Sangoma Technologies Inc.
*
*		This program is free software; you can redistribute it and/or
*		modify it under the terms of the GNU General Public License
*		as published by the Free Software Foundation; either version
*		2 of the License, or (at your option) any later version.
* ============================================================================
*
*
* Feb 16, 2011	David Rokhvarg	<davidr@sangoma.com>
*				Added interface into HWEC module via WP_API_CMD_EC_IOCTL.
*	
* Nov 16, 2009	David Rokhvarg	<davidr@sangoma.com>
*				Enabled support for WP_API_CMD_SET_RX_GAINS and
*				WP_API_CMD_SET_TX_GAINS commands in Sangoma Windows Driver.
*
* Nov 19, 2008	David Rokhvarg	<davidr@sangoma.com>
*				Added RBS Chan/Span poll.
*
* Mar 10, 2008	David Rokhvarg	<davidr@sangoma.com>
*				Added BRI LoopBack control.
*
* Jul 27, 2006	David Rokhvarg	<davidr@sangoma.com>
*				Ported to Windows.
*
* Oct 04, 2005	Nenad Corbic	Initial version.
*****************************************************************************/


#include "wanpipe_includes.h"
#include "wanpipe_defines.h"
#include "wanpipe_debug.h"
#include "wanpipe_common.h"
#include "wanpipe_cfg.h"
#include "wanpipe.h"
#include "wanpipe_tdm_api.h"
#include "wanpipe_cdev_iface.h"
#include "wanpipe_timer_iface.h"
#include "wanpipe_mtp1.h"


/*==============================================================
  Defines
 */

#if !defined(__WINDOWS__)
# if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
#  define BUILD_TDMV_API
# endif/* #if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0) */
#endif/* #if !defined(__WINDOWS__) */


#if defined(BUILD_TDMV_API)

#define WP_TDM_API_MAX_PERIOD 50 /* 50ms */

#define WP_TDM_MAX_RX_Q_LEN 10
#define WP_TDM_MAX_HDLC_RX_Q_LEN 40
#define WP_TDM_MAX_TX_Q_LEN 5
#define WP_TDM_MAX_HDLC_TX_Q_LEN 17

#define WP_TDM_MAX_EVENT_Q_TIMESLOT_MULTIPLE 6
#define WP_TDM_MIN_EVENT_Q_LEN 10

#define WP_TDM_MAX_CTRL_EVENT_Q_LEN 2000 /* 500 channels * 40 events (At same time) */
#define WP_TDM_MAX_RX_FREE_Q_LEN 10


#undef  DEBUG_API_WAKEUP
#undef  DEBUG_API_READ
#undef  DEBUG_API_WRITE
#undef  DEBUG_API_POLL

#define WP_TDM_DEF_FAKE_POLARITY_THRESHOLD 16000

#ifdef __WINDOWS__
# define wptdm_os_lock_irq(card,flags)   wan_spin_lock_irq(card,flags)
# define wptdm_os_unlock_irq(card,flags) wan_spin_unlock_irq(card,flags)
#else

#if 0
#warning "WANPIPE TDM API - OS LOCK DEFINED -- DEBUGGING"
#define wptdm_os_lock_irq(card,flags)	wan_spin_lock_irq(card,flags)
#define wptdm_os_unlock_irq(card,flags) wan_spin_unlock_irq(card,flags)
#else
#define wptdm_os_lock_irq(card,flags)	 
#define wptdm_os_unlock_irq(card,flags)  
#endif
#endif

#define MAX_TDM_API_CHANNELS 32

static int wp_tdmapi_global_cnt=0;
static wanpipe_tdm_api_dev_t tdmapi_ctrl;

/*==============================================================
  Prototypes
 */

static int wp_tdmapi_open(void *obj);
static int wp_tdmapi_read_msg(void *obj , netskb_t **skb,  wp_api_hdr_t *hdr, int count);
static int wp_tdmapi_write_msg(void *obj , netskb_t *skb, wp_api_hdr_t *hdr);
static int wp_tdmapi_release(void *obj);

static void wp_tdmapi_report_rbsbits(void* card_id, int channel, unsigned char rbsbits);
static void wp_tdmapi_report_alarms(void* card_id, uint32_t te_alarm);
static void wp_tdmapi_tone(void* card_id, wan_event_t*);
static void wp_tdmapi_hook(void* card_id, wan_event_t*);
static void wp_tdmapi_ringtrip(void* card_id, wan_event_t*);
static void wp_tdmapi_ringdetect (void* card_id, wan_event_t *event);
static void wp_tdmapi_linkstatus(void* card_id, wan_event_t*);
static void wp_tdmapi_polarityreverse(void* card_id, wan_event_t*);
static __inline int wp_tdmapi_check_fakepolarity(u8 *buf, int len, wanpipe_tdm_api_dev_t* tdm_api);
static void wp_tdmapi_report_fakepolarityreverse(void* card_id,u_int8_t channel);


static int store_tdm_api_pointer_in_card(sdla_t *card, wanpipe_tdm_api_dev_t *tdm_api);
static int remove_tdm_api_pointer_from_card(wanpipe_tdm_api_dev_t *tdm_api);


static int wanpipe_tdm_api_ioctl(void *obj, int cmd, void *udata);
static u_int32_t wp_tdmapi_poll(void *obj);
static int wanpipe_tdm_api_event_ioctl(wanpipe_tdm_api_dev_t*, wanpipe_api_cmd_t*);
static wanpipe_tdm_api_dev_t *wp_tdmapi_search(sdla_t *card, int fe_chan);

static int wanpipe_tdm_api_handle_event(wanpipe_tdm_api_dev_t *tdm_api, netskb_t *skb);
static int wp_tdmapi_unreg_notify_ctrl(wanpipe_tdm_api_dev_t *tdm_api);

#if defined(__LINUX__)
# if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,20))
static void wp_api_task_func (void * tdmapi_ptr);
# else
static void wp_api_task_func (struct work_struct *work);
# endif
#elif defined(__WINDOWS__)
static void wp_api_task_func (IN PKDPC Dpc, IN PVOID tdmapi_ptr, IN PVOID SystemArgument1, IN PVOID SystemArgument2);
#else
static void wp_api_task_func (void * tdmapi_ptr, int arg);
#endif


#if 0
static int wp_tdm_api_mtp1_rx_data (void *priv_ptr, u8 *rx_data, int rx_len);
static int wp_tdm_api_mtp1_rx_reject (void *priv_prt, char *reason);
static int wp_tdm_api_mtp1_rx_suerm (void *priv_ptr);
static int wp_tdm_api_tmp1_wakup(void *priv_ptr);
#endif
static int wp_tdmapi_tx_timeout(void *obj);

/*==============================================================
  Global Variables
 */

enum {
	WP_API_FLUSH_INVAL,
	WP_API_FLUSH_ALL,
	WP_API_FLUSH_RX,
	WP_API_FLUSH_TX,
	WP_API_FLUSH_EVENT,
	WP_API_FLUSH_LOCK,
	WP_API_FLUSH_NO_LOCK
};

static wanpipe_cdev_ops_t wp_tdmapi_fops;


static void wp_tdmapi_init_buffs(wanpipe_tdm_api_dev_t *tdm_api, int option)
{

#define wptdm_queue_lock_irq(card,flags)    if (lock) wan_spin_lock_irq(card,flags)
#define wptdm_queue_unlock_irq(card,flags)  if (lock) wan_spin_unlock_irq(card,flags)

	netskb_t *skb;
	int hdr_sz=sizeof(wp_api_hdr_t);
	sdla_t *card; 
	wan_smp_flag_t flags;
	int lock=1;

	card = (sdla_t*)tdm_api->card;
	if (!card || tdm_api == &tdmapi_ctrl) {
     	lock=0;
	}
	flags=0;

	wptdm_queue_lock_irq(&card->wandev.lock,&flags);

	if (option == WP_API_FLUSH_ALL || option == WP_API_FLUSH_RX) {

		if (WPTDM_SPAN_OP_MODE(tdm_api) || tdm_api->hdlc_framing) {
			do {
				skb=wan_skb_dequeue(&tdm_api->wp_rx_list);		
				if (skb) {
					wan_skb_queue_tail(&tdm_api->wp_dealloc_list,skb);
					continue;
				}
			} while(skb);

			do {
				skb=wan_skb_dequeue(&tdm_api->wp_rx_free_list);
				if (skb) {
					wan_skb_queue_tail(&tdm_api->wp_dealloc_list,skb);
					continue;
				}
			} while(skb);

			
			skb=tdm_api->rx_skb;
			tdm_api->rx_skb=NULL;
			if (skb) {
				wan_skb_queue_tail(&tdm_api->wp_dealloc_list,skb);
			}

		} else {
			 
			while((skb=wan_skb_dequeue(&tdm_api->wp_rx_list))) {
				wan_skb_init(skb,hdr_sz);
				wan_skb_trim(skb,0);
				wan_skb_queue_tail(&tdm_api->wp_rx_free_list,skb);
			}
			skb=tdm_api->rx_skb;
			tdm_api->rx_skb=NULL;
			if (skb) {
				wan_skb_init(skb,hdr_sz);
				wan_skb_trim(skb,0);
				wan_skb_queue_tail(&tdm_api->wp_rx_free_list,skb);
			}
			
		}
	}

	if (option == WP_API_FLUSH_ALL || option == WP_API_FLUSH_TX) {

		do {
			skb=wan_skb_dequeue(&tdm_api->wp_tx_free_list);
			if (skb) {
				wan_skb_queue_tail(&tdm_api->wp_dealloc_list,skb);
				continue;
			}
		} while(skb);

		do {
			skb=wan_skb_dequeue(&tdm_api->wp_tx_list);
			if (skb) {
				wan_skb_queue_tail(&tdm_api->wp_dealloc_list,skb);
				continue;
			}
		} while(skb);

		skb=tdm_api->tx_skb;
		tdm_api->tx_skb=NULL;
		if (skb){
			wan_skb_queue_tail(&tdm_api->wp_dealloc_list,skb);
		}

	}

	if (option == WP_API_FLUSH_ALL || option == WP_API_FLUSH_EVENT) {

		while((skb=wan_skb_dequeue(&tdm_api->wp_event_list))) {
			wan_skb_init(skb,hdr_sz);
			wan_skb_trim(skb,0);
			wan_skb_queue_tail(&tdm_api->wp_event_free_list,skb);
		}
	}

	wptdm_queue_unlock_irq(&card->wandev.lock,&flags);


	do {
		wptdm_queue_lock_irq(&card->wandev.lock,&flags);
		skb=wan_skb_dequeue(&tdm_api->wp_dealloc_list);
		wptdm_queue_unlock_irq(&card->wandev.lock,&flags);
		if (skb) {
			wan_skb_free(skb);
			continue;
		}
	} while(skb);
	


#undef wptdm_queue_lock_irq
#undef wptdm_queue_unlock_irq

}

static void wp_tdmapi_free_buffs(wanpipe_tdm_api_dev_t *tdm_api)
{
	wan_skb_queue_purge(&tdm_api->wp_rx_list);
	wan_skb_queue_purge(&tdm_api->wp_rx_free_list);
	wan_skb_queue_purge(&tdm_api->wp_tx_free_list);
	wan_skb_queue_purge(&tdm_api->wp_tx_list);
	wan_skb_queue_purge(&tdm_api->wp_event_list);
	wan_skb_queue_purge(&tdm_api->wp_event_bh_list);
	wan_skb_queue_purge(&tdm_api->wp_event_free_list);
	wan_skb_queue_purge(&tdm_api->wp_dealloc_list);

	if (tdm_api->rx_skb){
		wan_skb_free(tdm_api->rx_skb);
		tdm_api->rx_skb=NULL;
	}
	if (tdm_api->tx_skb){
		wan_skb_free(tdm_api->tx_skb);
		tdm_api->tx_skb=NULL;
	}

	if (tdm_api->rx_gain) {
		uint8_t *rx_gains=tdm_api->rx_gain;
		tdm_api->rx_gain=NULL;
		wan_free(rx_gains);
	}

	if (tdm_api->tx_gain){
		uint8_t *tx_gains=tdm_api->tx_gain;
		tdm_api->tx_gain=NULL;
		wan_free(tx_gains);
	}
}	



#ifdef DEBUG_API_WAKEUP
static int gwake_cnt=0;
#endif

static inline void wp_wakeup_rx_tdmapi(wanpipe_tdm_api_dev_t *tdm_api)
{
#ifdef DEBUG_API_WAKEUP
	if (tdm_api->tdm_span == 1 && tdm_api->tdm_chan == 1) {
		gwake_cnt++;
		if (gwake_cnt % 1000 == 0) {
			DEBUG_EVENT("%s: TDMAPI WAKING DEV \n",tdm_api->name);
		}
	}
#endif

	if (tdm_api->cdev) {
		wanpipe_cdev_rx_wake(tdm_api->cdev);
	}
}

static inline void wp_wakeup_event_tdmapi(wanpipe_tdm_api_dev_t *tdm_api)
{
	if (tdm_api->cdev) {
		wanpipe_cdev_event_wake(tdm_api->cdev);
	}
}

static inline void wp_wakeup_tx_tdmapi(wanpipe_tdm_api_dev_t *tdm_api)
{
#ifdef DEBUG_API_WAKEUP
	if (tdm_api->tdm_span == 1 && tdm_api->tdm_chan == 1) {
		gwake_cnt++;
		if (gwake_cnt % 1000 == 0) {
			DEBUG_EVENT("%s: TDMAPI WAKING DEV \n",tdm_api->name);
		}
	}
#endif

	if (tdm_api->cdev) {
		wanpipe_cdev_tx_wake(tdm_api->cdev);
	}
}


int wp_ctrl_dev_create(void)
{
	wanpipe_cdev_t *cdev;
	int err;
		
	cdev = wan_kmalloc(sizeof(wanpipe_cdev_t));
	if (!cdev) {
		return -ENOMEM;
	}
	
	memset(cdev, 0, sizeof(wanpipe_cdev_t));
	
	DEBUG_TDMAPI("%s(): Registering Wanpipe CTRL Device!\n",__FUNCTION__);
	
	memset(&tdmapi_ctrl, 0, sizeof(tdmapi_ctrl));
	
	wan_skb_queue_init(&tdmapi_ctrl.wp_rx_list);
	wan_skb_queue_init(&tdmapi_ctrl.wp_rx_free_list);
	wan_skb_queue_init(&tdmapi_ctrl.wp_tx_free_list);
	wan_skb_queue_init(&tdmapi_ctrl.wp_tx_list);
	wan_skb_queue_init(&tdmapi_ctrl.wp_event_list);
	wan_skb_queue_init(&tdmapi_ctrl.wp_event_bh_list);
	wan_skb_queue_init(&tdmapi_ctrl.wp_event_free_list);
	wan_skb_queue_init(&tdmapi_ctrl.wp_dealloc_list);
	
	WAN_TASKQ_INIT((&tdmapi_ctrl.wp_api_task), 0, wp_api_task_func, &tdmapi_ctrl);
	
	wan_set_bit(0,&tdmapi_ctrl.init);
	
	wan_mutex_lock_init(&tdmapi_ctrl.lock, "ctrldev_lock");
	wan_spin_lock_init(&tdmapi_ctrl.irq_lock, "ctrldev_irq_lock");
	
	cdev->dev_ptr=&tdmapi_ctrl;
	tdmapi_ctrl.cdev=cdev;
	
	err=wp_tdmapi_alloc_q(&tdmapi_ctrl, &tdmapi_ctrl.wp_event_free_list, WP_TDM_API_EVENT_MAX_LEN, WP_TDM_MAX_CTRL_EVENT_Q_LEN);
	if (err) {
		return err;
	}

	/*****************************************************************
	 Part of global initialization is setting the fops pointers
	 for ALL users of this file.
	******************************************************************/

	memset(&wp_tdmapi_fops, 0x00, sizeof(wp_tdmapi_fops));

	wp_tdmapi_fops.open		= wp_tdmapi_open;
	wp_tdmapi_fops.close	= wp_tdmapi_release;
	wp_tdmapi_fops.ioctl	= wanpipe_tdm_api_ioctl;
	wp_tdmapi_fops.poll		= wp_tdmapi_poll;
	wp_tdmapi_fops.read		= wp_tdmapi_read_msg;
	wp_tdmapi_fops.write	= wp_tdmapi_write_msg;
	/*****************************************************************/

	memcpy(&cdev->ops, &wp_tdmapi_fops, sizeof(wanpipe_cdev_ops_t));

	/* some functions are NOT valid for ctrl dev */
	cdev->ops.read	= NULL;
	cdev->ops.write	= NULL;
	
	err=wanpipe_cdev_tdm_ctrl_create(cdev);
	if(err){
		return err;
	}

	return 0;
}

static int wp_tdmapi_reg_globals(void)
{
	int err=0;

		/* Header Sanity Check */
	if ((sizeof(wp_api_hdr_t)!=WAN_MAX_HDR_SZ) ||
		(sizeof(wp_api_hdr_t)!=WAN_MAX_HDR_SZ)) {
		DEBUG_ERROR("%s: Internal Error: RxHDR=%i != Expected RxHDR=%i  TxHDR=%i != Expected TxHDR=%i\n",
				__FUNCTION__,sizeof(wp_api_hdr_t),WAN_MAX_HDR_SZ,sizeof(wp_api_hdr_t),WAN_MAX_HDR_SZ);
		return -EINVAL;
	}

	/* Internal Sanity Check */
	if (WANPIPE_API_CMD_SZ != sizeof(wanpipe_api_cmd_t)) {
		DEBUG_ERROR("%s: Internal Error: Wanpipe API CMD Structure Exceeds Limit=%i Size=%i!\n",
				__FUNCTION__, WANPIPE_API_CMD_SZ,sizeof(wanpipe_api_cmd_t));
		return -EINVAL;
	}

	/* Internal Sanity Check */
	if (WAN_MAX_EVENT_SZ != sizeof(wp_api_event_t)) {
		DEBUG_ERROR("%s: Internal Error: Wanpipe Event Structure Exceeds Limit=%i Size=%i!\n",
				__FUNCTION__, WAN_MAX_EVENT_SZ,sizeof(wp_api_event_t));
		return EINVAL;
	}

	/* Internal Sanity Check */
	if (WAN_MAX_HDR_SZ != sizeof(wp_api_hdr_t)) {
		DEBUG_ERROR("%s: Internal Error: Wanpipe Header Structure Exceeds Limit=%i Size=%i!\n",
				__FUNCTION__, WAN_MAX_HDR_SZ,sizeof(wp_api_event_t));
		return EINVAL;
	}

	/* Internal Sanity Check */
	if (WANPIPE_API_DEV_CFG_SZ != sizeof(wanpipe_api_dev_cfg_t)) {
		DEBUG_ERROR("%s: Internal Error: Wanpipe Dev Cfg Structure Exceeds Limit=%i Size=%i!\n",
				__FUNCTION__, WANPIPE_API_DEV_CFG_SZ,sizeof(wanpipe_api_dev_cfg_t));
		return EINVAL;
	}

	if (WANPIPE_API_DEV_CFG_MAX_SZ != sizeof(wanpipe_api_dev_cfg_t)) {
		DEBUG_ERROR("%s: Internal Error: Wanpipe Dev Cfg Structure Exceeds MAX Limit=%i Size=%i!\n",
				__FUNCTION__, WANPIPE_API_DEV_CFG_MAX_SZ,sizeof(wanpipe_api_dev_cfg_t));
		return EINVAL;
	}

	DEBUG_EVENT("WANPIPE API: HDR_SZ=%i CMD_SZ=%i EVENT_SZ=%i DEV_CFG_SZ=%i\n",
			sizeof(wp_api_hdr_t),sizeof(wanpipe_api_cmd_t),
			sizeof(wp_api_event_t),sizeof(wanpipe_api_dev_cfg_t));


#if 0
/*  FIXME 1: timer dev is not complete 
	FIXME 2: timer dev is global and should be created/deleted
			similary to ctrl dev, not a Span/Chan dev.
*/
	err=wanpipe_wandev_timer_create();
#endif

	return err;
}


void wp_ctrl_dev_delete(void)
{
	unsigned long timeout=SYSTEM_TICKS;

	DEBUG_TDMAPI("%s(): Un-Registering Wanpipe CTRL Device!\n",__FUNCTION__);

	wan_clear_bit(0,&tdmapi_ctrl.init);
	
	while(wan_test_bit(0,&tdmapi_ctrl.used)) {
		if (SYSTEM_TICKS - timeout > HZ*2) {
			if(wan_test_bit(0,&tdmapi_ctrl.used)){
				DEBUG_WARNING("wanpipe_tdm_api: Warning: Ctrl Device still in use on shutdown!\n");
			}
			break;	
		}
		WP_DELAY(1000);
		if(wan_test_bit(0,&tdmapi_ctrl.used)){
			wp_wakeup_rx_tdmapi(&tdmapi_ctrl);
			WP_SCHEDULE("tdmapi_ctlr",10);
		}
	}

	wp_tdmapi_free_buffs(&tdmapi_ctrl);

	if (tdmapi_ctrl.cdev) {
		wanpipe_cdev_free(tdmapi_ctrl.cdev);
		wan_free(tdmapi_ctrl.cdev);
		tdmapi_ctrl.cdev=NULL;
	}
}

static int wp_tdmapi_unreg_globals(void)
{
	WAN_DEBUG_FUNC_START;

	DEBUG_TEST("%s(): Unregistering Global API Devices!\n",__FUNCTION__);

#if 0
/*  FIXME timer dev is not complete */
	wanpipe_wandev_timer_free();
#endif

	WAN_DEBUG_FUNC_END;

	return 0;
}

int wanpipe_tdm_api_reg(wanpipe_tdm_api_dev_t *tdm_api)
{
	sdla_t				*card = NULL;
	u8 					tmp_name[50];
	int 				err=-EINVAL;
	wanpipe_cdev_t 		*cdev=NULL;
	wanpipe_api_cmd_t 	wp_cmd;
	wanpipe_api_cmd_t 	*wp_cmd_ptr=NULL;
	sdla_fe_t			*fe =NULL;
	int 				tdm_api_queue_init=0;
	wanpipe_tdm_api_card_dev_t *tdm_card_dev=NULL;
	int					event_queue_len=0;

	wan_mutex_lock_init(&tdm_api->lock, "wan_tdmapi_lock");
	wan_spin_lock_init(&tdm_api->irq_lock, "wan_tdmapi_irq_lock");

	if (wp_tdmapi_global_cnt == 0){
		err=wp_tdmapi_reg_globals();
		if (err) {
			return err;
		}
	}
	wp_tdmapi_global_cnt++;

	cdev=wan_kmalloc(sizeof(wanpipe_cdev_t));
	if (!cdev) {
		err=-ENOMEM;
		goto tdm_api_reg_error_exit;
	}

	memset(cdev,0,sizeof(wanpipe_cdev_t));


	WAN_ASSERT(tdm_api == NULL);
	card = (sdla_t*)tdm_api->card;
	fe = &card->fe;

	if (!card->tdm_api_dev) {
		
		tdm_card_dev = wan_malloc(sizeof(wanpipe_tdm_api_card_dev_t));
		if (!tdm_card_dev) {
			goto tdm_api_reg_error_exit;
		}
		memset(tdm_card_dev,0,sizeof(wanpipe_tdm_api_card_dev_t));
		card->tdm_api_dev=tdm_card_dev;
		tdm_card_dev->ref_cnt++;

	} else {
		tdm_card_dev = card->tdm_api_dev;
	}

	if (!tdm_api->hdlc_framing) {
		unsigned int i,max_timeslot=0;
		if (!IS_E1_CARD(card)) {
			tdm_card_dev->sig_timeslot_map |= (tdm_api->active_ch >> 1);
		} else {
			tdm_card_dev->sig_timeslot_map |= tdm_api->active_ch;
		}

		for (i=0;i<32;i++) {
			if (wan_test_bit(i,&tdm_card_dev->sig_timeslot_map)) {
				max_timeslot=i+1;
			}
		}
		if (max_timeslot > tdm_card_dev->max_timeslots) {
			tdm_card_dev->max_timeslots=max_timeslot;
		}

		/* Calculate number of timeslots this channel has.
		   SPAN mode versus CHAN. We cannot assume 1 time slot */
		for (i=0;i<32;i++) {
			if (wan_test_bit(i,&tdm_api->active_ch)) {
				tdm_api->timeslots++;
			}
		}
	} else {
		/* Just initialize for hdlc channels */
		tdm_api->timeslots=1;
	}

	wan_skb_queue_init(&tdm_api->wp_rx_list);
	wan_skb_queue_init(&tdm_api->wp_rx_free_list);

	wan_skb_queue_init(&tdm_api->wp_tx_list);
	wan_skb_queue_init(&tdm_api->wp_tx_free_list);

	wan_skb_queue_init(&tdm_api->wp_event_list);
	wan_skb_queue_init(&tdm_api->wp_event_bh_list);
	wan_skb_queue_init(&tdm_api->wp_event_free_list);
	wan_skb_queue_init(&tdm_api->wp_dealloc_list);

	WAN_TASKQ_INIT((&tdm_api->wp_api_task),0,wp_api_task_func,tdm_api);

	tdm_api_queue_init=1;

	err=wp_tdmapi_alloc_q(tdm_api, &tdm_api->wp_rx_free_list, WP_TDM_API_MAX_LEN, 16);
	if (err) {
		goto tdm_api_reg_error_exit;
	}

	event_queue_len=tdm_api->timeslots * WP_TDM_MAX_EVENT_Q_TIMESLOT_MULTIPLE;
	if (event_queue_len < WP_TDM_MIN_EVENT_Q_LEN) {
    	event_queue_len=WP_TDM_MIN_EVENT_Q_LEN; 	
	}

	err=wp_tdmapi_alloc_q(tdm_api, &tdm_api->wp_event_free_list, WP_TDM_API_EVENT_MAX_LEN, event_queue_len);
	if (err) {
		goto tdm_api_reg_error_exit;
	}

	/* Default initialization */
	tdm_api->cfg.rx_queue_sz	=WP_TDM_MAX_RX_Q_LEN;
	tdm_api->tx_channelized		=0;
	tdm_api->buffer_multiplier = 1;

	DEBUG_TDMAPI("%s(): usr_period: %d, hw_mtu_mru: %d\n", __FUNCTION__, 
		tdm_api->cfg.usr_period, tdm_api->cfg.hw_mtu_mru);

	if (tdm_api->hdlc_framing) {

		if (IS_BRI_CARD(card)) {
			tdm_api->cfg.hw_mtu_mru		=300;
			tdm_api->cfg.usr_mtu_mru	=300;
			tdm_api->mtu_mru = 300;
		} else {
			tdm_api->cfg.hw_mtu_mru		=1500;
			tdm_api->cfg.usr_mtu_mru	=1500;
			tdm_api->mtu_mru = 1500;
		}

		tdm_api->cfg.usr_period		=0;
		tdm_api->cfg.tdm_codec		=WP_NONE;
		tdm_api->cfg.power_level	=0;
		tdm_api->cfg.rx_disable		=0;
		tdm_api->cfg.tx_disable		=0;
		tdm_api->cfg.ec_tap		=0;
		tdm_api->cfg.rbs_rx_bits	=-1;
		tdm_api->cfg.hdlc		=1;
		
		/* Rx queue size must be at least twice dma chain */
		tdm_api->cfg.rx_queue_sz 	= WP_TDM_MAX_HDLC_RX_Q_LEN;
		/* Tx queue size must be at least as bit as tx dma chan */
		tdm_api->cfg.tx_queue_sz	= WP_TDM_MAX_HDLC_TX_Q_LEN;

		/* We are expecting tx_q_len for hdlc
  		 * to be configured from upper layer */
		if (tdm_api->cfg.tx_queue_sz == 0) {
			DEBUG_ERROR("%s: Internal Error: Tx Q Len not specified by lower layer!\n",
					__FUNCTION__);
			err=-EINVAL;
			goto tdm_api_reg_error_exit;
		}

	} else {

		tdm_api->cfg.tdm_codec		=WP_NONE;
		tdm_api->cfg.power_level	=0;
		tdm_api->cfg.rx_disable		=0;
		tdm_api->cfg.tx_disable		=0;
		tdm_api->cfg.ec_tap			=0;
		tdm_api->cfg.rbs_rx_bits	=-1;
		tdm_api->cfg.hdlc			=0;
		if (!tdm_api->cfg.hw_mtu_mru ||
			!tdm_api->cfg.usr_period ||
			!tdm_api->cfg.usr_mtu_mru) {
			DEBUG_ERROR("%s: Internal Error: API hwmtu=%i period=%i usr_mtu_mru=%i!\n",
						__FUNCTION__, tdm_api->cfg.hw_mtu_mru, tdm_api->cfg.usr_period, tdm_api->cfg.usr_mtu_mru);
					
			err=-EINVAL;
			goto tdm_api_reg_error_exit;
		}

		if (WPTDM_CHAN_OP_MODE(tdm_api)) {
			
			tdm_api->cfg.rx_queue_sz 	= WP_TDM_MAX_RX_Q_LEN;
			tdm_api->cfg.tx_queue_sz	= WP_TDM_MAX_TX_Q_LEN;
			tdm_api->tx_channelized		=1;

			tdm_api->mtu_mru = tdm_api->cfg.usr_period*tdm_api->cfg.hw_mtu_mru+sizeof(wp_api_hdr_t);

		} else {

			/* We are expecting tx_q_len for hdlc
			* to be configured from upper layer */
			if (tdm_api->cfg.tx_queue_sz == 0) {
				DEBUG_ERROR("%s: Internal Error: Tx Q Len not specified by lower layer!\n",
						__FUNCTION__);
				err=-EINVAL;
				goto tdm_api_reg_error_exit;
			}
		
		}

		if (tdm_api->cfg.idle_flag == 0) {
			/* BRI card must always idle 0xFF regardless of coding */
			if (tdm_api->cfg.hw_tdm_coding == WP_MULAW || IS_BRI_CARD(card)) {
        		tdm_api->cfg.idle_flag=0xFF;
			} else {
				tdm_api->cfg.idle_flag=0xD5;	
			}
		}
	}


	tdm_api->critical=0;
	wan_clear_bit(0,&tdm_api->used);


	err=store_tdm_api_pointer_in_card(card, tdm_api);
	if (err){
		goto tdm_api_reg_error_exit;
	}
	
	sprintf(tmp_name,"wanpipe%d_if%d",tdm_api->tdm_span,tdm_api->tdm_chan);

	DEBUG_TDMAPI("%s: Configuring TDM API NAME=%s Qlen=%i TS=%i MTU=%i\n",
				 card->devname,tmp_name, tdm_api->cfg.tx_queue_sz, 
				 tdm_api->timeslots,tdm_api->mtu_mru);

	/* Initialize Event Callback functions */
	card->wandev.event_callback.rbsbits		= NULL; /*wp_tdmapi_rbsbits;*/
	card->wandev.event_callback.alarms		= NULL; /*wp_tdmapi_alarms;*/
	card->wandev.event_callback.hook		= wp_tdmapi_hook;
	card->wandev.event_callback.ringdetect	= wp_tdmapi_ringdetect;
	card->wandev.event_callback.ringtrip	= wp_tdmapi_ringtrip;
	card->wandev.event_callback.linkstatus	= wp_tdmapi_linkstatus;
	card->wandev.event_callback.polarityreverse	= wp_tdmapi_polarityreverse;

	card->wandev.te_report_rbsbits  = wp_tdmapi_report_rbsbits;
	card->wandev.te_report_alarms   = wp_tdmapi_report_alarms;

	/* Always initialize the callback pointer */
	card->wandev.event_callback.tone 		= wp_tdmapi_tone;

#if defined(__WINDOWS__)
	/* Analog always supports DTMF detection */
	tdm_api->dtmfsupport = WANOPT_YES;
	/* Need pointer to PnP Fdo Device Extension for 
	 * handling of PnP notifications. */
	cdev->PnpFdoPdx = card->level2_fdo_pdx;
#endif

	cdev->dev_ptr=tdm_api;
	tdm_api->cdev=cdev;

	cdev->span=tdm_api->tdm_span;
	cdev->chan=tdm_api->tdm_chan;
	cdev->operation_mode=tdm_api->api_mode;/* WP_TDM_OPMODE - Legacy...*/
	sprintf(cdev->name,tmp_name,sizeof(cdev->name));
	memcpy(&cdev->ops, &wp_tdmapi_fops, sizeof(wanpipe_cdev_ops_t));

#if 0
	if (tdm_api->operation_mode == WP_TDM_OPMODE_MTP1) {
		wp_mtp1_reg_t reg;
		
		reg.priv_ptr=tdm_api;
		reg.rx_data=wp_tdm_api_mtp1_rx_data;
		reg.rx_reject=wp_tdm_api_mtp1_rx_reject;
		reg.rx_suerm=wp_tdm_api_mtp1_rx_suerm;
		reg.wakeup=wp_tdm_api_tmp1_wakup;
		
		tdm_api->mtp1_dev = wp_mtp1_register(&reg);	
		if (tdm_api->mtp1_dev == NULL) {
			goto tdm_api_reg_error_exit;	
		}
	}
#endif
	
	if (tdm_api->hdlc_framing) {
		/* timeout may occur only on HDLC channels */
		cdev->ops.tx_timeout = wp_tdmapi_tx_timeout;
	}

	err=wanpipe_cdev_tdm_create(cdev);
	if (err) {
		goto tdm_api_reg_error_exit;
	}

	/* AT this point the registration should not fail */

	if (tdm_api->cfg.rbs_tx_bits) {
		DEBUG_EVENT("%s: Setting Tx RBS/CAS Idle Bits = 0x%02X\n",
			       	        tmp_name,
					tdm_api->cfg.rbs_tx_bits);
		tdm_api->write_rbs_bits(tdm_api->chan,
		                        tdm_api->tdm_chan,
					(u8)tdm_api->cfg.rbs_tx_bits);
	}

   /* Enable default events for analog */
	if (card->wandev.config_id == WANCONFIG_AFT_ANALOG) {
		memset(&wp_cmd, 0x00, sizeof(wp_cmd));
		wp_cmd.cmd = WP_API_CMD_SET_EVENT;
		wp_cmd.event.wp_api_event_mode = WP_API_EVENT_ENABLE;
		wp_cmd.chan = tdm_api->tdm_chan;
		wp_cmd_ptr=&wp_cmd;

		if (fe->rm_param.mod[(tdm_api->tdm_chan) - 1].type == MOD_TYPE_FXS) {
			/*Intialize timers for FXS Wink-Flash event*/
			fe->rm_param.mod[(tdm_api->tdm_chan) - 1].u.fxs.itimer=0;
			fe->rm_param.mod[(tdm_api->tdm_chan) - 1].u.fxs.rxflashtime=WP_RM_RXFLASHTIME;
			/*hook event*/
			wp_cmd.event.wp_api_event_type = WP_API_EVENT_RXHOOK;
			DEBUG_EVENT("%s: %s Loop Closure Event on Module %d\n",
				fe->name,
				WP_API_EVENT_MODE_DECODE(wp_cmd.event.wp_api_event_mode),
				tdm_api->tdm_chan);
			wanpipe_tdm_api_event_ioctl(tdm_api,wp_cmd_ptr);
			/*Ring/Trip event */
			wp_cmd.event.wp_api_event_type = WP_API_EVENT_RING_TRIP_DETECT;
			DEBUG_EVENT("%s: %s Ring Trip Detection Event on Module %d\n",
				fe->name,
				WP_API_EVENT_MODE_DECODE(wp_cmd.event.wp_api_event_mode),
				tdm_api->tdm_chan);
			wanpipe_tdm_api_event_ioctl(tdm_api,wp_cmd_ptr);
		} else if (fe->rm_param.mod[(tdm_api->tdm_chan) - 1].type == MOD_TYPE_FXO ) {
			/*ring detect event */
			wp_cmd.event.wp_api_event_type = WP_TDMAPI_EVENT_RING_DETECT;
			DEBUG_EVENT("%s: %s Ring Detection Event on module %d\n",
				fe->name,
				WP_API_EVENT_MODE_DECODE(wp_cmd.event.wp_api_event_mode),
				tdm_api->tdm_chan);
			wanpipe_tdm_api_event_ioctl(tdm_api,wp_cmd_ptr);
		}
	}

   	tdm_api->cfg.fe_alarms = 0;
	if (IS_BRI_CARD(card) || IS_TE1_CARD(card)) {
   		if (fe->fe_status == FE_DISCONNECTED) {
   			tdm_api->cfg.fe_alarms = (1 | WAN_TE_BIT_ALARM_RED);
   		}
	} 

	return err;


tdm_api_reg_error_exit:

	if (wp_tdmapi_global_cnt > 0) {
		wp_tdmapi_global_cnt--;
	}

	if (wp_tdmapi_global_cnt == 0){
		wp_tdmapi_unreg_globals();
	}

	if (card->tdm_api_dev) {
		wanpipe_tdm_api_card_dev_t *tdm_card_dev = card->tdm_api_dev;

		if (tdm_card_dev->ref_cnt > 0){
			tdm_card_dev->ref_cnt--;
		}
		if (tdm_card_dev->ref_cnt == 0) {
			wan_free(card->tdm_api_dev);
			card->tdm_api_dev=NULL;
		}
	}

	if (tdm_api_queue_init) {
		wp_tdmapi_free_buffs(tdm_api);
	}

	if (cdev) {
		wan_free(cdev);
	}

	return err;
}


int wanpipe_tdm_api_unreg(wanpipe_tdm_api_dev_t *tdm_api)
{
	sdla_t *card = tdm_api->card;
	int err;

	WAN_DEBUG_FUNC_START;

	wan_set_bit(WP_TDM_DOWN,&tdm_api->critical);

	if (wan_test_bit(0,&tdm_api->used)) {
#if defined(__WINDOWS__)
		/* During transition to System StandBy state an application may keep
		 * open file descriptors. Do NOT fail unregister in this case. */
		DEBUG_WARNING("%s: Warning: unregister request for Span=%i Chan=%i while Open Handles exist!\n",
			tdm_api->name, tdm_api->tdm_span, tdm_api->tdm_chan);
#else
		DEBUG_EVENT("%s Failed to unreg Span=%i Chan=%i: BUSY!\n",
			tdm_api->name,tdm_api->tdm_span,tdm_api->tdm_chan);
		return -EBUSY;
#endif
	}


#if defined(WAN_TASKQ_STOP)
	err=WAN_TASKQ_STOP((&tdm_api->wp_api_task));
	DEBUG_TDMAPI("%s: %s: %s\n",card->devname, tdm_api->name, err?"TASKQ Successfully Stopped":"TASKQ Not Running");
#endif

	wp_tdmapi_unreg_notify_ctrl(tdm_api);

	remove_tdm_api_pointer_from_card(tdm_api);

	wp_tdmapi_free_buffs(tdm_api);

	if (tdm_api->cdev) {
		wanpipe_cdev_free(tdm_api->cdev);
		wan_free(tdm_api->cdev);
		tdm_api->cdev=NULL;
	}

	if (card->tdm_api_dev) {
		wanpipe_tdm_api_card_dev_t *tdm_card_dev = card->tdm_api_dev;

		if (tdm_card_dev->ref_cnt > 0){
			tdm_card_dev->ref_cnt--;
		}
		if (tdm_card_dev->ref_cnt == 0) {

			if (tdm_card_dev->rbs_poll_timeout) {
				wan_smp_flag_t smp_flags1;
				card->hw_iface.hw_lock(card->hw,&smp_flags1);
				if (card->wandev.fe_iface.set_fe_sigctrl) {
					card->wandev.fe_iface.set_fe_sigctrl(
							&card->fe,
							WAN_TE_SIG_POLL,
							ENABLE_ALL_CHANNELS,
							WAN_DISABLE);
				}
				card->hw_iface.hw_unlock(card->hw,&smp_flags1);
			}

			card->tdm_api_dev=NULL;
			wan_free(tdm_card_dev);
		}
	}

	wp_tdmapi_global_cnt--;
	if (wp_tdmapi_global_cnt == 0) {
		wp_tdmapi_unreg_globals();
	}

	WAN_DEBUG_FUNC_END;

	return 0;
}

int wanpipe_tdm_api_kick(wanpipe_tdm_api_dev_t *tdm_api)
{
	int rc=0;

	if (tdm_api == NULL || !wan_test_bit(0,&tdm_api->init)){
		return -ENODEV;
	}

	if (tdm_api->tx_channelized) {
		if (is_tdm_api_stopped(tdm_api) || wan_skb_queue_len(&tdm_api->wp_tx_list) >= (int)tdm_api->cfg.tx_queue_sz){
			wp_tdm_api_start(tdm_api);
			if (wan_test_bit(0,&tdm_api->used)) {
				wp_wakeup_tx_tdmapi(tdm_api);
			}
		}
	} else {
			/* By default tdm_api->buffer_multiplier is set to 1 therefore we
               can use it to check it tx is available */
			if (tdm_api->write_hdlc_check) {
				rc=tdm_api->write_hdlc_check(tdm_api->chan, 0, tdm_api->buffer_multiplier);
			}
			if (rc == 0 ||
				wan_skb_queue_len(&tdm_api->wp_rx_list) >= tdm_api->buffer_multiplier ||
				wan_skb_queue_len(&tdm_api->wp_event_list)) {
				wp_tdm_api_start(tdm_api);
				if (wan_test_bit(0,&tdm_api->used)) {
					wp_wakeup_tx_tdmapi(tdm_api);
				}
			}
	}

	return 0;
}


int wanpipe_tdm_api_is_rbsbits(sdla_t *card)
{
	wanpipe_tdm_api_card_dev_t *tdm_card_dev;

	WAN_ASSERT(card == NULL);

	if (!card->tdm_api_dev) {
		return 0;
	}

	tdm_card_dev = card->tdm_api_dev;

	if (!tdm_card_dev || !tdm_card_dev->rbs_poll_timeout) {
		return 0;
	}

	tdm_card_dev->rbscount++;

	if ((SYSTEM_TICKS - tdm_card_dev->rbs_poll_cnt) < tdm_card_dev->rbs_poll_timeout) {
		return 0;
	}

	tdm_card_dev->rbs_poll_cnt=SYSTEM_TICKS;

	return 1;
}



/* Note: This functin is hw_locked from the core */
int wanpipe_tdm_api_rbsbits_poll(sdla_t * card)
{
	wanpipe_tdm_api_card_dev_t *tdm_card_dev;
	unsigned int x;

	WAN_ASSERT(card == NULL);

	if (!card->tdm_api_dev) {
		return 0;
	}

	tdm_card_dev = card->tdm_api_dev;

	if ((SYSTEM_TICKS - tdm_card_dev->rbs_long_poll_cnt) > HZ*2) {

		tdm_card_dev->rbs_long_poll_cnt=SYSTEM_TICKS;

		DEBUG_TEST("%s: RBS POLL MaxSlots=%i MAP=0x%08X\n",card->devname,tdm_card_dev->max_timeslots,tdm_card_dev->sig_timeslot_map);

		for(x = 0; x < tdm_card_dev->max_timeslots; x++){
			if (wan_test_bit(x, &tdm_card_dev->sig_timeslot_map)){
				if (card->wandev.fe_iface.read_rbsbits) {
					card->wandev.fe_iface.read_rbsbits(
						&card->fe,
						x,
						WAN_TE_RBS_UPDATE|WAN_TE_RBS_REPORT);
				} else {
					DEBUG_ERROR("%s: Internal Error [%s:%d]!\n", card->devname,	__FUNCTION__,__LINE__);
				}
			}
		}

	} else {

		if (card->wandev.fe_iface.check_rbsbits == NULL){
                	DEBUG_ERROR("%s: Internal Error [%s:%d]!\n",
					card->devname,
					__FUNCTION__,__LINE__); 
			return -EINVAL;
		}

		card->wandev.fe_iface.check_rbsbits(
				&card->fe, 
				1, tdm_card_dev->sig_timeslot_map, 1);
		card->wandev.fe_iface.check_rbsbits(
				&card->fe, 
				9, tdm_card_dev->sig_timeslot_map, 1);
		card->wandev.fe_iface.check_rbsbits(
				&card->fe, 
				17, tdm_card_dev->sig_timeslot_map, 1);
		if (IS_E1_CARD(card)){
			card->wandev.fe_iface.check_rbsbits(
					&card->fe, 
					25, tdm_card_dev->sig_timeslot_map, 1);
		}
	}

	return 0;
}


static int wp_tdmapi_open(void *obj)
{
	wanpipe_tdm_api_dev_t *tdm_api = (wanpipe_tdm_api_dev_t*)obj;
	int cnt;

	if (!tdm_api) {
		return -ENODEV;
	}

	if (wan_test_bit(WP_TDM_DOWN,&tdm_api->critical)) {
		return -ENODEV;
	}

	if (!tdm_api->card) {
		if(tdm_api != &tdmapi_ctrl){
			DEBUG_ERROR("%s: Error: Wanpipe API Device does not have a 'card' pointer! Internal error!\n",
				tdm_api->name);
			return -ENODEV;
		}
	}

	cnt = wp_tdm_inc_open_cnt(tdm_api);

	if (cnt == 1) {

		wp_tdmapi_init_buffs(tdm_api, WP_API_FLUSH_ALL);
	
		tdm_api->cfg.loop=0;
	}

	wan_set_bit(0,&tdm_api->used);
	wp_tdm_api_start(tdm_api);

	DEBUG_TDMAPI ("%s: DRIVER OPEN S/C(%i/%i) API Ptr=%p\n",
		__FUNCTION__, tdm_api->tdm_span, tdm_api->tdm_chan, tdm_api);

	return 0;
}

#ifdef DEBUG_API_READ
static int gread_cnt=0;
#endif

static __inline int wp_tdmapi_set_gain(netskb_t *skb, char *gain)
{
 	if (skb && gain && wan_skb_len(skb) > sizeof(wp_api_hdr_t)) {
		int i;
		int len = wan_skb_len(skb)- sizeof(wp_api_hdr_t);
		u8 *buf=(u8*)wan_skb_data(skb);
		buf=&buf[sizeof(wp_api_hdr_t)];
		for (i=0;i<len;i++) {
			buf[i]=gain[buf[i]];
		}
	} else {
     	return -1;
	}

	return 0;
}

static int wp_tdmapi_read_msg(void *obj, netskb_t **skb_ptr, wp_api_hdr_t *hdr, int count)
{
	wanpipe_tdm_api_dev_t *tdm_api = (wanpipe_tdm_api_dev_t*)obj;
	netskb_t *skb=NULL;
	wan_smp_flag_t irq_flags;
	sdla_t *card;
	int rx_q_len=0;
	u8 *buf;

	
	if (tdm_api == NULL || !wan_test_bit(0,&tdm_api->init) || !wan_test_bit(0,&tdm_api->used) || !hdr) {
		if (hdr) {
			hdr->wp_api_hdr_operation_status = SANG_STATUS_GENERAL_ERROR;	
		}
		return -ENODEV;
	}

	if (tdm_api == &tdmapi_ctrl) {
		hdr->wp_api_hdr_operation_status = SANG_STATUS_INVALID_DEVICE;
		return -EINVAL;
	}

	if (wan_test_bit(WP_TDM_DOWN,&tdm_api->critical)) {
		hdr->wp_api_hdr_operation_status = SANG_STATUS_GENERAL_ERROR;
		return -ENODEV;
	}

	card=(sdla_t*)tdm_api->card;

	if (!card) {
		hdr->wp_api_hdr_operation_status = SANG_STATUS_GENERAL_ERROR;
		return -ENODEV;
	}

	irq_flags=0;

	if (tdm_api->buffer_multiplier > 1) {
		netskb_t *nskb;
		netskb_t *skb_to_free;
		char * buf, *u_buf;
		int single_len;
		int multiplier=tdm_api->buffer_multiplier;
		int i;

		wptdm_os_lock_irq(&card->wandev.lock,&irq_flags);

		if (wan_skb_queue_len(&tdm_api->wp_rx_list) < multiplier) {
			wptdm_os_unlock_irq(&card->wandev.lock,&irq_flags);
			DEBUG_ERROR("%s: %s:%d: Error: Rx queue len %i: multiplier %i mismatch\n",
				tdm_api->name,__FUNCTION__,__LINE__,wan_skb_queue_len(&tdm_api->wp_rx_list),multiplier);
			hdr->wp_api_hdr_operation_status = SANG_STATUS_NO_DATA_AVAILABLE;
			return -ENOBUFS;
		}

		skb=wan_skb_dequeue(&tdm_api->wp_rx_list);
		if (!skb) {
			wptdm_os_unlock_irq(&card->wandev.lock,&irq_flags);
			DEBUG_ERROR("%s: %s:%d: Error rx queue emtpy on first skb: multiplier %i\n",
				tdm_api->name,__FUNCTION__,__LINE__,multiplier);
			hdr->wp_api_hdr_operation_status = SANG_STATUS_NO_DATA_AVAILABLE;
			return -ENOBUFS;
		}

		wptdm_os_unlock_irq(&card->wandev.lock,&irq_flags);

		/* This section must be unlocked because we are allocating memory */
		single_len = (wan_skb_len(skb)-sizeof(wp_api_hdr_t));
		nskb=wan_skb_kalloc(wan_skb_len(skb)+(single_len*multiplier));
		if (!nskb) {
			hdr->wp_api_hdr_operation_status = SANG_STATUS_NO_DATA_AVAILABLE;
			return -ENOMEM;
		}

		buf=wan_skb_put(nskb,sizeof(wp_api_hdr_t)+single_len);
		u_buf=wan_skb_data(skb);
		memcpy(buf,u_buf,sizeof(wp_api_hdr_t)+single_len);

		wptdm_os_lock_irq(&card->wandev.lock,&irq_flags);

		wan_skb_queue_tail(&tdm_api->wp_dealloc_list,skb);
		skb=NULL;

		/* Count 1 less then multiplier because we already pulled one buffer above */
		for (i=1;i<multiplier;i++) {
			skb=wan_skb_dequeue(&tdm_api->wp_rx_list);
			if (!skb) {
				wptdm_os_unlock_irq(&card->wandev.lock,&irq_flags);
				DEBUG_ERROR("%s:%d: Error rx queue emtpy on %i skb - multiplier %i\n",
					__FUNCTION__,__LINE__,i+1,multiplier);
				wan_skb_free(nskb);
				wan_skb_free(skb);
				hdr->wp_api_hdr_operation_status = SANG_STATUS_NO_DATA_AVAILABLE;
				return -ENOBUFS;
			}

			u_buf=wan_skb_data(skb);
			u_buf = &u_buf[sizeof(wp_api_hdr_t)];

			buf=wan_skb_put(nskb,single_len);

			memcpy(buf,u_buf,single_len);

			wan_skb_queue_tail(&tdm_api->wp_dealloc_list,skb);
			skb=NULL;
		}

		skb=nskb;
		rx_q_len = wan_skb_queue_len(&tdm_api->wp_rx_list);
		wptdm_os_unlock_irq(&card->wandev.lock,&irq_flags);

		do {

			wptdm_os_lock_irq(&card->wandev.lock,&irq_flags);

			skb_to_free = wan_skb_dequeue(&tdm_api->wp_dealloc_list);

			wptdm_os_unlock_irq(&card->wandev.lock,&irq_flags);

			if (skb_to_free) {
				wan_skb_free(skb_to_free);
			}

		} while(skb_to_free);

	} else {

		wptdm_os_lock_irq(&card->wandev.lock,&irq_flags);
		skb=wan_skb_dequeue(&tdm_api->wp_rx_list);
		rx_q_len = wan_skb_queue_len(&tdm_api->wp_rx_list);
		wptdm_os_unlock_irq(&card->wandev.lock,&irq_flags);
	}

	if (!skb){
		hdr->wp_api_hdr_operation_status = SANG_STATUS_NO_DATA_AVAILABLE;
		return -ENOBUFS;
	}
		
	if (tdm_api->rx_gain) {
		wp_tdmapi_set_gain(skb,tdm_api->rx_gain);
	}

#ifdef DEBUG_API_READ
	if (tdm_api->tdm_span == 1 &&
	    tdm_api->tdm_chan == 1) {
	    gread_cnt++;
	    if (gread_cnt &&
	        gread_cnt % 1000 == 0) {
		DEBUG_EVENT("%s: WP_TDM API READ %i Len=%i\n",
				tdm_api->name, gread_cnt,
				wan_skb_len(skb)-sizeof(wp_api_hdr_t));
	    }
	}
#endif

	if (count < (int)(wan_skb_len(skb)) ||
		wan_skb_len(skb) < sizeof(wp_api_hdr_t)){

		DEBUG_ERROR("%s:%d User API Error: User Rx Len=%i < Driver Rx Len=%i (hdr=%i). User API must increase expected rx length!\n",
			__FUNCTION__,__LINE__,count,wan_skb_len(skb),sizeof(wp_api_hdr_t));
		wan_skb_free(skb);
		hdr->wp_api_hdr_operation_status = SANG_STATUS_BUFFER_TOO_SMALL;
		return -EFAULT;
	}

	buf=(u8*)wan_skb_data(skb);

	/* copy header info (such as time stamp) */
	memcpy(hdr, buf, sizeof(wp_api_hdr_t));
		
	if (WPTDM_SPAN_OP_MODE(tdm_api) || tdm_api->hdlc_framing) {

		/* do nothing */
		
	} else {
		netskb_t *tmp_skb;

		/* Allocate another free skb for RX */
		tmp_skb=wan_skb_kalloc(WP_TDM_API_MAX_LEN);
		if (tmp_skb) {
			wan_skb_init(tmp_skb,sizeof(wp_api_hdr_t));
			wan_skb_trim(tmp_skb,0);
			wptdm_os_lock_irq(&card->wandev.lock,&irq_flags);
			wan_skb_queue_tail(&tdm_api->wp_rx_free_list,tmp_skb);
			wptdm_os_unlock_irq(&card->wandev.lock,&irq_flags);
		}
	}

	hdr->wp_api_rx_hdr_number_of_frames_in_queue = (u8)rx_q_len;
	hdr->wp_api_rx_hdr_max_queue_length = (u8)tdm_api->cfg.rx_queue_sz;

	hdr->wp_api_hdr_operation_status = SANG_STATUS_RX_DATA_AVAILABLE;
	hdr->wp_api_hdr_data_length = wan_skb_len(skb)-sizeof(wp_api_hdr_t);

	if (wan_skb_len(skb) >= sizeof(wp_api_hdr_t)) {
		/* Copy back the updated header info into the skb header
           section. */
		memcpy(buf,hdr,sizeof(wp_api_hdr_t));
	} else {
		DEBUG_ERROR("%s: Internal Error: Rx Data Invalid %i\n",tdm_api->name,wan_skb_len(skb));
	}

	*skb_ptr = skb;

//NENAD
	DEBUG_TEST("%s: RX Q=%i  RX Q MAX=%i SIZE=%i %p\n",
				tdm_api->name,
				hdr->wp_api_rx_hdr_number_of_frames_in_queue,
				hdr->wp_api_rx_hdr_max_queue_length,
				wan_skb_len(skb), skb);

	return 0;
}


static int wp_tdmapi_tx (wanpipe_tdm_api_dev_t *tdm_api, netskb_t *skb,  wp_api_hdr_t *hdr)
{
	int err=-EINVAL;

	if (wan_test_and_set_bit(WP_TDM_HDLC_TX,&tdm_api->critical)){
		DEBUG_ERROR("%s: Error: %s() WP_TDM_HDLC_TX critical\n",
			tdm_api->name, __FUNCTION__);
		hdr->wp_api_hdr_operation_status = SANG_STATUS_GENERAL_ERROR;
		return -EINVAL;
	}

	if (tdm_api->buffer_multiplier > 1) {

			netskb_t *nskb;
			int len,i;
			char *buf;
			char *u_buf=wan_skb_data(skb);

	
			if (tdm_api->write_hdlc_check) {
				int rc=tdm_api->write_hdlc_check(tdm_api->chan, 0, tdm_api->buffer_multiplier);
				if (rc) {
					DEBUG_ERROR("%s: Error: %s() Write check failed for multiplier %i\n",
						tdm_api->name, __FUNCTION__,tdm_api->buffer_multiplier);
					err=-EINVAL;
					goto wp_tdmapi_tx_exit;
				}
			}

			if ((wan_skb_len(skb) % tdm_api->buffer_multiplier) != 0) {
				DEBUG_ERROR("%s: Error: %s() buf size %i is not divisable by multiplier %i\n",
						tdm_api->name, __FUNCTION__,wan_skb_len(skb),tdm_api->buffer_multiplier);
				err=-EINVAL;
				goto wp_tdmapi_tx_exit;
			}

			len = wan_skb_len(skb) / tdm_api->buffer_multiplier;

			for (i=0;i<tdm_api->buffer_multiplier;i++) {

				nskb=wan_skb_kalloc(len+sizeof(wp_api_hdr_t));
				if (!nskb) {
					err = -ENOMEM;
					break;
				}

				buf=wan_skb_put(nskb,len);
				memcpy(buf,&u_buf[i*len],len);

				if (tdm_api->write_hdlc_frame) {
					err=tdm_api->write_hdlc_frame(tdm_api->chan,nskb,hdr);
					if (err) {
						if (WAN_NET_RATELIMIT()) {
						DEBUG_ERROR("%s: Error: %s() write_hdlc_frame failed on multiple %i/%i len=%i\n",
									tdm_api->name, __FUNCTION__,i+1,tdm_api->buffer_multiplier,len);
						}
						wan_skb_free(nskb);
						break;
					}	
				} else {
					wan_skb_free(nskb);
					DEBUG_ERROR("%s: Error: %s() tdm_api->write_hdlc_frame is NULL: critical\n",
						tdm_api->name, __FUNCTION__);
					err=-EINVAL;
					break;
				}
				nskb=NULL;
			}

			/* Free the orignal copy of the skb */
			if (err==0) {
				wan_skb_free(skb);
				skb=NULL;
			}

	} else {

		if (tdm_api->write_hdlc_frame) {
			err=tdm_api->write_hdlc_frame(tdm_api->chan,skb, hdr);
		} else {
			DEBUG_ERROR("%s: Error: %s() tdm_api->write_hdlc_frame is NULL: critical\n",
				tdm_api->name, __FUNCTION__);
			err=-EINVAL;
		}
	}

wp_tdmapi_tx_exit:

	if (err == -EBUSY) {
		/* Busy condition */
		hdr->wp_api_hdr_operation_status = SANG_STATUS_DEVICE_BUSY;
		wp_tdm_api_stop(tdm_api);
		err=1;
	} else if (err == 1) {
		/* Successful tx but now queue is full.
		 * Note that BRI D-channel will always return 1 after accepting a frame for transmission.
		 */
		wp_tdm_api_stop(tdm_api);
		err=0;
	} else if (err == 0) {
		/* Successful tx queue is still NOT full */
		wp_tdm_api_start(tdm_api);
		err=0;
	} else {
		wp_tdm_api_start(tdm_api);
		hdr->wp_api_hdr_operation_status = SANG_STATUS_IO_ERROR;
		WP_AFT_CHAN_ERROR_STATS(tdm_api->cfg.stats,tx_errors);
	}

#if 0
	while ((skb=wan_skb_dequeue(&tdm_api->wp_tx_list)) != NULL) {
		err=tdm_api->write_hdlc_frame(tdm_api->chan,skb);
		if (err) {
			wan_skb_queue_head(&tdm_api->wp_tx_list, skb);
			break;
		}
		tdm_api->cfg.stats.tx_packets++;
		skb=NULL;
	}
#endif

	wan_clear_bit(WP_TDM_HDLC_TX,&tdm_api->critical);
	return err;
}

#ifdef DEBUG_API_WRITE
static int gwrite_cnt=0;
#endif
static int wp_tdmapi_write_msg(void *obj, netskb_t *skb, wp_api_hdr_t *hdr)
{
	wanpipe_tdm_api_dev_t *tdm_api = (wanpipe_tdm_api_dev_t*)obj;
	wan_smp_flag_t irq_flags;
	int err=-EINVAL;
	sdla_t *card;

	if (tdm_api == NULL || !wan_test_bit(0,&tdm_api->init) || !wan_test_bit(0,&tdm_api->used)){
		return -ENODEV;
	}

	if (tdm_api == &tdmapi_ctrl) {
		hdr->wp_api_hdr_operation_status = SANG_STATUS_INVALID_DEVICE;
		WP_AFT_CHAN_ERROR_STATS(tdm_api->cfg.stats,tx_errors);
		if (WAN_NET_RATELIMIT()) {
			DEBUG_ERROR("Error: Wanpipe API: wp_tdmapi_write_msg() on ctrl device!\n");
		}
		return -EINVAL;
	}

	card=tdm_api->card;
	irq_flags=0;

	if (wan_test_bit(WP_TDM_DOWN,&tdm_api->critical)) {
		hdr->wp_api_hdr_operation_status = SANG_STATUS_GENERAL_ERROR;
		WP_AFT_CHAN_ERROR_STATS(tdm_api->cfg.stats,tx_errors);
		if (WAN_NET_RATELIMIT()) {
			DEBUG_ERROR("Error: Wanpipe API: wp_tdmapi_write_msg() WP_TDM_DOWN\n");
		}
		return -ENODEV;
	}

	if (wan_skb_len(skb) <= sizeof(wp_api_hdr_t)) {
		hdr->wp_api_hdr_operation_status = SANG_STATUS_BUFFER_TOO_SMALL;
		WP_AFT_CHAN_ERROR_STATS(tdm_api->cfg.stats,tx_errors);
		if (WAN_NET_RATELIMIT()) {
			DEBUG_ERROR("Error: Wanpipe API: wp_tdmapi_write_msg() datalen=%i < hdrlen=%i\n",
				wan_skb_len(skb),sizeof(wp_api_hdr_t));
		}
		return -EINVAL;
	}

	if (tdm_api->buffer_multiplier > 1) {

		if ((wan_skb_len(skb) - sizeof(wp_api_hdr_t)) % tdm_api->buffer_multiplier != 0 ) {
			DEBUG_ERROR("%s: Error: TDM API Tx packet len %i not divisible by multiple %i\n",
					tdm_api->name, wan_skb_len(skb), tdm_api->buffer_multiplier);
			hdr->wp_api_hdr_operation_status =SANG_STATUS_TX_DATA_TOO_LONG;
			WP_AFT_CHAN_ERROR_STATS(tdm_api->cfg.stats,tx_errors);
			return -EFBIG;
		}

	} else if (WPTDM_SPAN_OP_MODE(tdm_api) || tdm_api->hdlc_framing) {
		if (wan_skb_len(skb) > (int)(tdm_api->cfg.usr_mtu_mru + sizeof(wp_api_hdr_t))) {
			DEBUG_ERROR("%s: Error: TDM API Tx packet too big %d Max=%d\n",
					tdm_api->name,wan_skb_len(skb), (tdm_api->cfg.usr_mtu_mru + sizeof(wp_api_hdr_t)));
			
			hdr->wp_api_hdr_operation_status =SANG_STATUS_TX_DATA_TOO_LONG;
			WP_AFT_CHAN_ERROR_STATS(tdm_api->cfg.stats,tx_errors);
			return -EFBIG;
		}
	} else {
		if (wan_skb_len(skb) > (WP_TDM_API_MAX_LEN+sizeof(wp_api_hdr_t))) {
			if (WAN_NET_RATELIMIT()) {
				DEBUG_ERROR("%s: Error: TDM API Tx packet too big %d\n",
					tdm_api->name, wan_skb_len(skb));
			}
			hdr->wp_api_hdr_operation_status =SANG_STATUS_TX_DATA_TOO_LONG;
			WP_AFT_CHAN_ERROR_STATS(tdm_api->cfg.stats,tx_errors);
			return -EFBIG;
		}
	}

	hdr->wp_api_hdr_data_length = wan_skb_len(skb)-sizeof(wp_api_hdr_t);

	if (WPTDM_SPAN_OP_MODE(tdm_api) || tdm_api->hdlc_framing) {

		if (tdm_api->tx_gain && !tdm_api->hdlc_framing) {
			wp_tdmapi_set_gain(skb,tdm_api->tx_gain);
		}
		
		wan_skb_pull(skb,sizeof(wp_api_hdr_t));

		err=wp_tdmapi_tx(tdm_api, skb, hdr);
		if (err) {
			wan_skb_push(skb,sizeof(wp_api_hdr_t));
			goto wp_tdmapi_write_msg_exit;
		} else {
			tdm_api->cfg.stats.tx_packets++;
			hdr->wp_api_hdr_operation_status=SANG_STATUS_SUCCESS;
		}

	} else {

		if (wan_skb_queue_len(&tdm_api->wp_tx_list) >= (int)tdm_api->cfg.tx_queue_sz){
			wp_tdm_api_stop(tdm_api);
            hdr->tx_h.current_number_of_frames_in_tx_queue = (u8)wan_skb_queue_len(&tdm_api->wp_tx_list);
	   		hdr->tx_h.tx_idle_packets = tdm_api->cfg.stats.tx_idle_packets;   
			hdr->tx_h.max_tx_queue_length = (u8)tdm_api->cfg.tx_queue_sz;
			err=1;
			DEBUG_TDMAPI("Error: Wanpipe API: wp_tdmapi_write_msg() TxList=%i >= MaxTxList=%i\n",
				wan_skb_queue_len(&tdm_api->wp_tx_list), tdm_api->cfg.tx_queue_sz);
			goto wp_tdmapi_write_msg_exit;
		}

		/* The code below implements the channelized tx mode */
	
#ifdef DEBUG_API_WRITE
		if (tdm_api->tdm_span == 1 &&
			tdm_api->tdm_chan == 1) {
			gwrite_cnt++;
			if (gwrite_cnt &&
				gwrite_cnt % 1000 == 0) {
				DEBUG_EVENT("%s: WP_TDM API WRITE CODEC %i Len=%i\n",
						tdm_api->name, gwrite_cnt,
						wan_skb_len(skb)-sizeof(wp_api_hdr_t));
			}
		}
#endif
	
		card = tdm_api->card;

		if (tdm_api->tx_gain) {
			wp_tdmapi_set_gain(skb,tdm_api->tx_gain);
		}

		wptdm_os_lock_irq(&card->wandev.lock,&irq_flags);
		wan_skb_queue_tail(&tdm_api->wp_tx_list, skb);
		hdr->tx_h.current_number_of_frames_in_tx_queue = (u8)wan_skb_queue_len(&tdm_api->wp_tx_list);
		hdr->tx_h.tx_idle_packets = tdm_api->cfg.stats.tx_idle_packets;
		wptdm_os_unlock_irq(&card->wandev.lock,&irq_flags);

		hdr->tx_h.max_tx_queue_length = (u8)tdm_api->cfg.tx_queue_sz;
		
		
		/* Free up previously used tx buffers */
		skb=NULL;
		if (wan_skb_queue_len(&tdm_api->wp_tx_free_list)) {
			do {
				wptdm_os_lock_irq(&card->wandev.lock,&irq_flags);
				skb=wan_skb_dequeue(&tdm_api->wp_tx_free_list);
				wptdm_os_unlock_irq(&card->wandev.lock,&irq_flags);
				if (skb) {
					wan_skb_free(skb);
					continue;
				}

			}while(skb);
		}

		hdr->wp_api_hdr_operation_status=SANG_STATUS_SUCCESS;
		err=0;
	}

	DEBUG_TEST("%s: TX Q=%i  TX Q MAX=%i\n",
				tdm_api->name,
				hdr->tx_h.current_number_of_frames_in_tx_queue,hdr->tx_h.max_tx_queue_length);

wp_tdmapi_write_msg_exit:

	return err;
}


static int wp_tdmapi_tx_timeout(void *obj)
{
	wanpipe_tdm_api_dev_t *tdm_api = (wanpipe_tdm_api_dev_t*)obj;
	int err=-EINVAL;
			
	if (tdm_api->write_hdlc_timeout) {
		/* Call timeout function with lock parameter set */
		err=tdm_api->write_hdlc_timeout(tdm_api->chan, 1);
	}

	DEBUG_TDMAPI("%s(): %s\n", __FUNCTION__, tdm_api->name);

	return err;
}

static int wp_tdmapi_release(void *obj)
{
	wanpipe_tdm_api_dev_t *tdm_api = (wanpipe_tdm_api_dev_t*)obj;
	wanpipe_api_cmd_t usr_tdm_api;
	wan_smp_flag_t flag;
	u32 cnt;

	if (tdm_api == NULL || !wan_test_bit(0,&tdm_api->init)) {
		if (tdm_api) {
			wan_clear_bit(0,&tdm_api->used);
		}
		return -ENODEV;
	}

	cnt = wp_tdm_dec_open_cnt(tdm_api);
	if (cnt == 0) {
		wan_clear_bit(0,&tdm_api->used);
		wp_wakeup_rx_tdmapi(tdm_api);

		wan_mutex_lock(&tdm_api->lock,&flag);
	
		tdm_api->cfg.rbs_rx_bits=-1;
		tdm_api->cfg.rbs_tx_bits=-1;
	
		wan_mutex_unlock(&tdm_api->lock,&flag);
	
		if (tdm_api != &tdmapi_ctrl && tdm_api->driver_ctrl) {
			tdm_api->driver_ctrl(tdm_api->chan,WP_API_CMD_FLUSH_BUFFERS,&usr_tdm_api);
		}
	}

	return 0;
}


static int wan_skb_push_to_ctrl_event (netskb_t *skb)
{
	netskb_t *ctrl_skb;
	wan_smp_flag_t flags;

	if (!skb) {
		DEBUG_ERROR("TDMAPI: Error: Null skb argument!\n");
		return -ENODEV;
	}

	if (!wan_test_bit(0,&tdmapi_ctrl.used)) {
		return -EBUSY;
	}
		
	wan_spin_lock_irq(&tdmapi_ctrl.irq_lock, &flags);
	ctrl_skb=wan_skb_dequeue(&tdmapi_ctrl.wp_event_free_list);
	wan_spin_unlock_irq(&tdmapi_ctrl.irq_lock, &flags);

	if (ctrl_skb){

		int len = wan_skb_len(skb);
		u8 *buf;
		if (len > wan_skb_tailroom(ctrl_skb)) {

			DEBUG_ERROR("TDMAPI: Error: TDM API CTRL Event Buffer Overflow Elen=%i Max=%i!\n",
					len,wan_skb_tailroom(ctrl_skb));

			WP_AFT_CHAN_ERROR_STATS(tdmapi_ctrl.cfg.stats, rx_events_dropped);
			return -EFAULT;
		}

		buf=wan_skb_put(ctrl_skb,len);
		memcpy(buf,wan_skb_data(skb),len);

		wan_spin_lock_irq(&tdmapi_ctrl.irq_lock, &flags);
		wanpipe_tdm_api_handle_event(&tdmapi_ctrl, ctrl_skb);
		wan_spin_unlock_irq(&tdmapi_ctrl.irq_lock, &flags);

	} else {
		WP_AFT_CHAN_ERROR_STATS(tdmapi_ctrl.cfg.stats, rx_events_dropped);
	}

	return 0;
}


#ifdef DEBUG_API_POLL
# if defined(__WINDOWS__)
#  pragma message ("POLL Debugging Enabled")
# else
#  warning "POLL Debugging Enabled"
# endif
static int gpoll_cnt=0;
#endif

static unsigned int wp_tdmapi_poll(void *obj)
{
	int ret=0,rc;
	wanpipe_tdm_api_dev_t *tdm_api = (wanpipe_tdm_api_dev_t*)obj;
	sdla_t *card;
	wan_smp_flag_t irq_flags;	

	irq_flags=0;

	if (tdm_api == NULL || !wan_test_bit(0,&tdm_api->init) || !wan_test_bit(0,&tdm_api->used)){
		return -ENODEV;
	}

	if (wan_test_bit(WP_TDM_DOWN,&tdm_api->critical)) {
		return -ENODEV;
	}

	if (tdm_api == &tdmapi_ctrl) {
		/* intentionally NOT locking! */
		if (wan_skb_queue_len(&tdm_api->wp_event_list)) {
			/* Indicate an Exception or an Event */
			ret |= POLLPRI;
		}
		return ret;	
	}


	card = tdm_api->card;
	if (!card) {
		return -ENODEV;
	}

#ifdef DEBUG_API_POLL
	if (tdm_api->tdm_span == 1 && tdm_api->tdm_chan == 1) {
		gpoll_cnt++;
		if (gpoll_cnt && gpoll_cnt % 1000 == 0) {
			DEBUG_EVENT("%s: WP_TDM POLL WAKEUP %i RxF=%i TxF=%i TxCE=%i TxE=%i TxP=%i\n",
				tdm_api->name, gpoll_cnt,
				wan_skb_queue_len(&tdm_api->wp_tx_free_list),
				wan_skb_queue_len(&tdm_api->wp_rx_free_list),
				tdm_api->cfg.stats.tx_idle_packets,
				tdm_api->cfg.stats.tx_errors,
				tdm_api->cfg.stats.tx_packets);
		}
	}
#endif

	wan_spin_lock_irq(&card->wandev.lock,&irq_flags);

	/* Tx Poll */
	if (!wan_test_bit(0,&tdm_api->cfg.tx_disable)) {
		if (tdm_api->tx_channelized) {
			if (wan_skb_queue_len(&tdm_api->wp_tx_list) < (int)tdm_api->cfg.tx_queue_sz) {
				wp_tdm_api_start(tdm_api);
				ret |= POLLOUT | POLLWRNORM;
			}
		} else {

			rc=1;
			/* tdm_api->buffer_multiplir is by default 1 so we can use it to check
			   for available buffers */
			if (tdm_api->write_hdlc_check) {
				rc=tdm_api->write_hdlc_check(tdm_api->chan, 0, tdm_api->buffer_multiplier);
			}

			if (rc == 0) {
				/* Wake up the user application tx available */
				wp_tdm_api_start(tdm_api);
				ret |= POLLOUT | POLLWRNORM;
			}
		}
	} 

	/* Rx Poll */
	if (!wan_test_bit(0,&tdm_api->cfg.rx_disable)) {
		/* tdm_api->buffer_multiplir is by default 1 so we can use it to check
		   for available buffers */
		if (wan_skb_queue_len(&tdm_api->wp_rx_list) >= tdm_api->buffer_multiplier) {
			ret |= POLLIN | POLLRDNORM;
		}
	}

	if (wan_skb_queue_len(&tdm_api->wp_event_list)) {
		/* Indicate an exception */
		ret |= POLLPRI;
	}

	wan_spin_unlock_irq(&card->wandev.lock,&irq_flags);

	return ret;
}

static int wanpipe_tdm_api_handle_event(wanpipe_tdm_api_dev_t *tdm_api, netskb_t *skb)
{
	wp_api_event_t  *p_tdmapi_event = NULL;
	wan_time_t sec;	/* windows note: must use wan_time_t when calling wan_get_timestamp()
					 * because it take 64 bit paramter 1. */
	wan_suseconds_t	usec;

	p_tdmapi_event = (wp_api_event_t*)wan_skb_data(skb);

	wan_get_timestamp(&sec, &usec);

	p_tdmapi_event->time_stamp_sec = sec;
	p_tdmapi_event->time_stamp_usec = usec;

	wan_skb_queue_tail(&tdm_api->wp_event_list,skb);
	tdm_api->cfg.stats.rx_events++;
	wp_wakeup_event_tdmapi(tdm_api);
	return 0;
}

static int wanpipe_tdm_api_ioctl_handle_tdm_api_cmd(wanpipe_tdm_api_dev_t *tdm_api, void *udata)
{
	wanpipe_api_cmd_t usr_tdm_api, *utdmapi;
	int err=0, cmd;
	wanpipe_codec_ops_t *wp_codec_ops;
	netskb_t *skb;
	sdla_fe_t			*fe = NULL;
	sdla_t *card=NULL;
	wanpipe_tdm_api_card_dev_t *tdm_card_dev=NULL;
	wan_smp_flag_t flags,irq_flags;

	int channel=tdm_api->tdm_chan;
	int span=0;

	flags=0;
	irq_flags=0;

	/* This is a klooge because global ctrl device does not have card pointer */
    card = (sdla_t*)tdm_api->card;  
	if (card) {
		tdm_card_dev=card->tdm_api_dev;
		fe = &card->fe;
	}

	utdmapi = (wanpipe_api_cmd_t*)udata;

#if defined(__WINDOWS__)
	/* udata is a pointer to wanpipe_api_cmd_t */
	memcpy(&usr_tdm_api, udata, sizeof(wanpipe_api_cmd_t));
#else
	if (WAN_COPY_FROM_USER(&usr_tdm_api,
			       utdmapi,
			       sizeof(wanpipe_api_cmd_t))){
	       return -EFAULT;
	}
#endif
	cmd=usr_tdm_api.cmd;

	
	if (WPTDM_SPAN_OP_MODE(tdm_api)) {
	
		if (usr_tdm_api.span) {
			span=usr_tdm_api.span;
		}

		if (usr_tdm_api.chan) {
			channel=usr_tdm_api.chan;
		}
	}

	if (channel >= MAX_TDM_API_CHANNELS) {
		DEBUG_EVENT("%s: %s(): Error: Invalid channel selected %i  (Max=%i)\n",
					tdm_api->name, __FUNCTION__, channel, MAX_TDM_API_CHANNELS);
		return -EINVAL;
	}

	usr_tdm_api.result=SANG_STATUS_SUCCESS;

	DEBUG_TDMAPI("%s: TDM API CMD: %i CMD Size=%i HDR Size=%i Even Size=%i\n",
			tdm_api->name,cmd,sizeof(wanpipe_api_cmd_t),sizeof(wp_api_hdr_t), sizeof(wp_api_event_t));

	wan_mutex_lock(&tdm_api->lock,&flags);


	/* Set the span/channel so that user knows which channel its using */
    usr_tdm_api.chan=channel;

	if (card) {
		/* must NOT do it for ctrl device! */
		usr_tdm_api.span=wp_tdmapi_get_span(card);
	}

	/* Commands for HDLC Device */

	if (tdm_api->hdlc_framing) {
		switch (cmd) {
		case WP_API_CMD_OPEN_CNT:
		case WP_API_CMD_GET_USR_MTU_MRU:
		case WP_API_CMD_GET_STATS:
		case WP_API_CMD_GET_FULL_CFG:
		case WP_API_CMD_GET_FE_STATUS:
		case WP_API_CMD_SET_FE_STATUS:
       	case WP_API_CMD_READ_EVENT:
       	case WP_API_CMD_GET_FE_ALARMS:
		case WP_API_CMD_RESET_STATS:
		case WP_API_CMD_SET_EVENT:
		case WP_API_CMD_SET_RM_RXFLASHTIME:
		case WP_API_CMD_DRIVER_VERSION:
		case WP_API_CMD_FIRMWARE_VERSION:
		case WP_API_CMD_CPLD_VERSION:
		case WP_API_CMD_FLUSH_BUFFERS:
		case WP_API_CMD_FLUSH_RX_BUFFERS:
		case WP_API_CMD_FLUSH_TX_BUFFERS:
		case WP_API_CMD_FLUSH_EVENT_BUFFERS:
		case WP_API_CMD_SET_TX_Q_SIZE:
		case WP_API_CMD_GET_TX_Q_SIZE:
		case WP_API_CMD_GET_RX_Q_SIZE:
		case WP_API_CMD_SET_RX_Q_SIZE:
		case WP_API_CMD_SS7_FORCE_RX:
		case WP_API_CMD_SS7_GET_CFG_STATUS:
			break;
		default:
			DEBUG_WARNING("%s: Warning: Invalid TDM API HDLC CMD %i\n", tdm_api->name,cmd);
			usr_tdm_api.result=SANG_STATUS_OPTION_NOT_SUPPORTED;
			err=-EOPNOTSUPP;
			goto tdm_api_exit;
		}
	}

	/* Commands for CTRL Device */

	if (tdm_api == &tdmapi_ctrl) {

		wan_smp_flag_t irq_flags;

		/* Note that in case of tdmapi_ctrl the lock has already
		   been taken above so no need to lock here */
		err=-EOPNOTSUPP;

		switch (cmd) {
		case WP_API_CMD_READ_EVENT:
		
			wan_spin_lock_irq(&tdm_api->irq_lock,&irq_flags);
			skb=wan_skb_dequeue(&tdm_api->wp_event_list);
			wan_spin_unlock_irq(&tdm_api->irq_lock,&irq_flags);
			if (!skb){
				usr_tdm_api.result=SANG_STATUS_NO_DATA_AVAILABLE;
				err=-ENOBUFS;
				break;
			}

			memcpy(&usr_tdm_api.event, wan_skb_data(skb), sizeof(wp_api_event_t));

			wan_skb_init(skb,sizeof(wp_api_hdr_t));
			wan_skb_trim(skb,0);

			wan_spin_lock_irq(&tdm_api->irq_lock,&irq_flags);
			wan_skb_queue_tail(&tdm_api->wp_event_free_list,skb);
			wan_spin_unlock_irq(&tdm_api->irq_lock,&irq_flags);

			err=0;
			break;

		default:
			DEBUG_WARNING("%s: Warning: Invalid TDM API cmd=%d for CTRL DEVICE\n", tdm_api->name, cmd);
			usr_tdm_api.result=SANG_STATUS_OPTION_NOT_SUPPORTED;
			err=-EOPNOTSUPP;
			break;
		}
	
		goto tdm_api_exit;
	}

	/* at this point 'card' must NOT be null */
	if (!card) {
		DEBUG_ERROR("%s: Error: %s(): 'card' pointer is null!\n", tdm_api->name, __FUNCTION__);
		err=-EOPNOTSUPP;
		goto tdm_api_exit;
	}

	/* Commands for TDM API Device */

	switch (cmd) {

	case WP_API_CMD_OPEN_CNT:
		usr_tdm_api.open_cnt = __wp_tdm_get_open_cnt(tdm_api);
		break;

	case WP_API_CMD_SET_HW_MTU_MRU:
		usr_tdm_api.result=SANG_STATUS_OPTION_NOT_SUPPORTED;
		err=-EOPNOTSUPP;
		break;

	case WP_API_CMD_GET_HW_MTU_MRU:
		usr_tdm_api.hw_mtu_mru = tdm_api->cfg.hw_mtu_mru;
		break;

	case WP_API_CMD_SET_USR_PERIOD:
		
		if (!WPTDM_CHAN_OP_MODE(tdm_api)) {
			err=-EOPNOTSUPP;
			goto tdm_api_exit;
		}

		if (usr_tdm_api.usr_period >= 10 &&
		    (usr_tdm_api.usr_period % 10) == 0 &&
		    usr_tdm_api.usr_period <= 1000) {

			usr_tdm_api.usr_mtu_mru = usr_tdm_api.usr_period*tdm_api->cfg.hw_mtu_mru*tdm_api->buffer_multiplier*tdm_api->timeslots;

		} else {
			usr_tdm_api.result=SANG_STATUS_INVALID_PARAMETER;
			err = -EINVAL;
			DEBUG_EVENT("%s: TDM API: Invalid HW Period %i!\n",
					tdm_api->name,usr_tdm_api.usr_period);
			goto tdm_api_exit;
		}

		usr_tdm_api.usr_mtu_mru = wanpipe_codec_calc_new_mtu(tdm_api->cfg.tdm_codec, usr_tdm_api.usr_mtu_mru); 

		tdm_api->cfg.usr_period = usr_tdm_api.usr_period;
		tdm_api->cfg.usr_mtu_mru = usr_tdm_api.usr_mtu_mru;

		tdm_api->mtu_mru = tdm_api->cfg.usr_period*tdm_api->cfg.hw_mtu_mru*tdm_api->timeslots+sizeof(wp_api_hdr_t);

		break;

	case WP_API_CMD_GET_USR_PERIOD:

		usr_tdm_api.usr_period = tdm_api->cfg.usr_period;
		break;

	case WP_API_CMD_GET_USR_MTU_MRU:

		usr_tdm_api.usr_mtu_mru = tdm_api->cfg.usr_mtu_mru;
		break;

	case WP_API_CMD_SET_CODEC:

		if (WPTDM_SPAN_OP_MODE(tdm_api)) {
			usr_tdm_api.result=SANG_STATUS_OPTION_NOT_SUPPORTED;
			err=-EOPNOTSUPP;
			goto tdm_api_exit;
		}

		if (usr_tdm_api.tdm_codec == tdm_api->cfg.tdm_codec){
			err = 0;
			goto tdm_api_exit;
		}

		if (usr_tdm_api.tdm_codec >= WP_TDM_CODEC_MAX){
			usr_tdm_api.result=SANG_STATUS_INVALID_PARAMETER;
			err = -EINVAL;
			goto tdm_api_exit;
		}

		if (usr_tdm_api.tdm_codec == WP_NONE) {

			usr_tdm_api.usr_mtu_mru = tdm_api->cfg.hw_mtu_mru * tdm_api->cfg.usr_period * tdm_api->buffer_multiplier * tdm_api->timeslots;

		} else {
			wp_codec_ops = WANPIPE_CODEC_OPS[tdm_api->cfg.hw_tdm_coding][usr_tdm_api.tdm_codec];
			if (!wp_codec_ops || !wp_codec_ops->init){

				usr_tdm_api.result=SANG_STATUS_INVALID_PARAMETER;
				err = -EINVAL;
				DEBUG_EVENT("%s: TDM API: Codec %i is unsupported!\n",
						tdm_api->name,usr_tdm_api.tdm_codec);
				goto tdm_api_exit;
			}
			usr_tdm_api.usr_mtu_mru = tdm_api->cfg.hw_mtu_mru * tdm_api->cfg.usr_period * tdm_api->buffer_multiplier * tdm_api->timeslots;
			usr_tdm_api.usr_mtu_mru = wanpipe_codec_calc_new_mtu(usr_tdm_api.tdm_codec, tdm_api->cfg.usr_mtu_mru);
		}

		tdm_api->cfg.usr_mtu_mru=usr_tdm_api.usr_mtu_mru;
		tdm_api->cfg.tdm_codec = usr_tdm_api.tdm_codec;
		break;

	case WP_API_CMD_GET_CODEC:
		usr_tdm_api.tdm_codec = tdm_api->cfg.tdm_codec;
		break;

	case WP_API_CMD_SET_POWER_LEVEL:
		tdm_api->cfg.power_level = usr_tdm_api.power_level;
		break;

	case WP_API_CMD_GET_POWER_LEVEL:
		usr_tdm_api.power_level = tdm_api->cfg.power_level;
		break;

	case WP_API_CMD_TOGGLE_RX:
		if (tdm_api->cfg.tx_disable){
			wan_clear_bit(0,&tdm_api->cfg.rx_disable);
		}else{
			wan_set_bit(0,&tdm_api->cfg.rx_disable);
		}
		break;

	case WP_API_CMD_TOGGLE_TX:
		if (tdm_api->cfg.tx_disable){
			wan_clear_bit(0,&tdm_api->cfg.tx_disable);
		}else{
			wan_set_bit(0,&tdm_api->cfg.tx_disable);
		}
		break;

	case WP_API_CMD_SET_EC_TAP:

		switch (usr_tdm_api.ec_tap){
			case 0:
			case 32:
			case 64:
			case 128:
				tdm_api->cfg.ec_tap = usr_tdm_api.ec_tap;
				break;

			default:
				usr_tdm_api.result=SANG_STATUS_INVALID_PARAMETER;
				DEBUG_EVENT("%s: Illegal Echo Cancellation Tap \n",
						tdm_api->name);
				err = -EINVAL;
				goto tdm_api_exit;
		}
		break;

	case WP_API_CMD_GET_EC_TAP:
		usr_tdm_api.ec_tap = tdm_api->cfg.ec_tap;
		break;

	case WP_API_CMD_ENABLE_HWEC:
		if (card && card->wandev.ec_enable) {
			err=card->wandev.ec_enable(card, 1, channel);
		} else{
			usr_tdm_api.result=SANG_STATUS_OPTION_NOT_SUPPORTED;
			err = -EOPNOTSUPP;
		}
		break;

	case WP_API_CMD_DISABLE_HWEC:
		if (card && card->wandev.ec_enable) {
			err=card->wandev.ec_enable(card, 0, channel);
		} else {
			usr_tdm_api.result=SANG_STATUS_OPTION_NOT_SUPPORTED;
			err = -EOPNOTSUPP;
		}
		break;

	case WP_API_CMD_GET_HW_EC:
		if (card && card->wandev.ec_state) {
			wan_hwec_dev_state_t ecdev_state = {0};
			card->wandev.ec_state(card,&ecdev_state);
			if (ecdev_state.ec_state) {
				usr_tdm_api.hw_ec = WANOPT_YES;
			} else {
				usr_tdm_api.hw_ec = WANOPT_NO;
			}
		} else {
			usr_tdm_api.hw_ec = WANOPT_NO;
		}
		break;

	case WP_API_CMD_GET_HW_EC_PERSIST:
		if (card->hwec_conf.persist_disable) {
			usr_tdm_api.hw_ec = WANOPT_NO;
		} else {
			usr_tdm_api.hw_ec = WANOPT_YES;
		}
		break;
		
	case WP_API_CMD_GET_HW_EC_CHAN:
		if (wan_test_bit(channel, &card->wandev.fe_ec_map)) {
			usr_tdm_api.hw_ec = WANOPT_YES;
		} else {
			usr_tdm_api.hw_ec = WANOPT_NO;
		}
		break;

	case WP_API_CMD_GET_HW_DTMF:
		if (card && card->wandev.ec_state) {
			wan_hwec_dev_state_t ecdev_state = {0};
			card->wandev.ec_state(card,&ecdev_state);
			if (wan_test_bit(channel,&ecdev_state.dtmf_map)) {
				usr_tdm_api.hw_dtmf = WANOPT_YES;
			} else {
				usr_tdm_api.hw_dtmf = WANOPT_NO;
			}
		} else {
			usr_tdm_api.hw_dtmf = WANOPT_NO;
		}
		break;

	case WP_API_CMD_GET_HW_FAX_DETECT:
		if (card && card->wandev.ec_state) {
			wan_hwec_dev_state_t ecdev_state = {0};
			card->wandev.ec_state(card,&ecdev_state);
			if (wan_test_bit(channel,&ecdev_state.fax_calling_map)) {
				usr_tdm_api.hw_fax = WANOPT_YES;
			} else {
				usr_tdm_api.hw_fax = WANOPT_NO;
			}
		} else {
			usr_tdm_api.hw_fax = WANOPT_NO;
		}
		break;

	case WP_API_CMD_EC_IOCTL:

		if (!card->wandev.ec_dev || !card->wandev.ec_tdmapi_ioctl) {
			DEBUG_WARNING("%s: Warning: HWEC dev OR ec_tdmapi_ioctl() is NULL!\n",
				card->devname);
			err = -EOPNOTSUPP;
			break;
		}

		err = card->wandev.ec_tdmapi_ioctl(card->wandev.ec_dev, &usr_tdm_api.iovec_list);

		break;

	case WP_API_CMD_GET_STATS:
		
		if (WPTDM_SPAN_OP_MODE(tdm_api) || tdm_api->hdlc_framing) {
			if (tdm_api->driver_ctrl) {
					err = tdm_api->driver_ctrl(tdm_api->chan,usr_tdm_api.cmd,&usr_tdm_api);

					/* Overwrite with TDM API rx/tx queue size.
					   In this case we hide the low level core buffering.
					   FIXME: Create new stats for low level core buffering */
					if (err == 0) {
						usr_tdm_api.stats.max_rx_queue_length =  (u8)tdm_api->cfg.rx_queue_sz;
						wptdm_os_lock_irq(&card->wandev.lock,&irq_flags);
						usr_tdm_api.stats.current_number_of_frames_in_rx_queue =  wan_skb_queue_len(&tdm_api->wp_rx_list);
						wptdm_os_unlock_irq(&card->wandev.lock,&irq_flags);
						
					}
			} else {
				usr_tdm_api.result=SANG_STATUS_OPTION_NOT_SUPPORTED;
				err = -EOPNOTSUPP;
			}
		} else {
			int rx_fifo=0, rx_over=0, tx_fifo=0;
			if (tdm_api->driver_ctrl) {
				err = tdm_api->driver_ctrl(tdm_api->chan,usr_tdm_api.cmd,&usr_tdm_api);
				if (err == 0) {
					rx_fifo=usr_tdm_api.stats.rx_fifo_errors;
					rx_over=usr_tdm_api.stats.rx_over_errors;
					tx_fifo=usr_tdm_api.stats.tx_fifo_errors;
				} else {
					DEBUG_ERROR("Error: Failed to exec driver_ctrl\n");
				}
			}
			memcpy(&usr_tdm_api.stats,&tdm_api->cfg.stats,sizeof(tdm_api->cfg.stats));
			usr_tdm_api.stats.max_rx_queue_length =  (u8)tdm_api->cfg.rx_queue_sz; 
			usr_tdm_api.stats.max_tx_queue_length = (u8)tdm_api->cfg.tx_queue_sz;
			usr_tdm_api.stats.rx_fifo_errors = rx_fifo;
			usr_tdm_api.stats.rx_over_errors = rx_over;
			usr_tdm_api.stats.tx_fifo_errors = tx_fifo;
			wptdm_os_lock_irq(&card->wandev.lock,&irq_flags);
			usr_tdm_api.stats.current_number_of_frames_in_tx_queue = wan_skb_queue_len(&tdm_api->wp_tx_list);
			usr_tdm_api.stats.current_number_of_frames_in_rx_queue = wan_skb_queue_len(&tdm_api->wp_rx_list);
			wptdm_os_unlock_irq(&card->wandev.lock,&irq_flags);
		}

		if (err == 0) {
			wptdm_os_lock_irq(&card->wandev.lock,&irq_flags);
			usr_tdm_api.stats.max_event_queue_length = wan_skb_queue_len(&tdm_api->wp_event_list)+wan_skb_queue_len(&tdm_api->wp_event_free_list);
			usr_tdm_api.stats.current_number_of_events_in_event_queue = wan_skb_queue_len(&tdm_api->wp_event_list);
			wptdm_os_unlock_irq(&card->wandev.lock,&irq_flags);

			usr_tdm_api.stats.rx_events_dropped	= tdm_api->cfg.stats.rx_events_dropped;
			usr_tdm_api.stats.rx_events 		= tdm_api->cfg.stats.rx_events;
			usr_tdm_api.stats.rx_events_tone 	= tdm_api->cfg.stats.rx_events_tone;
		}
		break;

	case WP_API_CMD_RESET_STATS:

		/* Always reset both set of stats for SPAN & CHAN mode, because
  		   chan mode uses fifo errors stats from lower layer */
		if (tdm_api->driver_ctrl) {
				err = tdm_api->driver_ctrl(tdm_api->chan,usr_tdm_api.cmd,&usr_tdm_api);
		} else {
			usr_tdm_api.result=SANG_STATUS_OPTION_NOT_SUPPORTED;
			err = -EOPNOTSUPP;
		}
		/* Alwasy flush the tdm_api stats SPAN or CHAN mode*/
		memset(&tdm_api->cfg.stats, 0x00, sizeof(tdm_api->cfg.stats));
		break;

	case WP_API_CMD_SET_IDLE_FLAG:
		if (WPTDM_SPAN_OP_MODE(tdm_api) || tdm_api->hdlc_framing) {
			if (tdm_api->driver_ctrl) {
				err = tdm_api->driver_ctrl(tdm_api->chan, usr_tdm_api.cmd, &usr_tdm_api);
			} else {
				usr_tdm_api.result = SANG_STATUS_OPTION_NOT_SUPPORTED;
				err = -EOPNOTSUPP;
			}
		}
		/* Set the new idle_flag, for both SPAN or CHAN mode. */
		tdm_api->cfg.idle_flag = usr_tdm_api.idle_flag;
		break;
	
	case WP_API_CMD_SS7_FORCE_RX:
		if (tdm_api->hdlc_framing && tdm_api->driver_ctrl) {
			err = tdm_api->driver_ctrl(tdm_api->chan, usr_tdm_api.cmd, &usr_tdm_api);
		} else {
			usr_tdm_api.result = SANG_STATUS_OPTION_NOT_SUPPORTED;
			err = -EOPNOTSUPP;
		}
		break;

	case WP_API_CMD_GET_FULL_CFG:
		memcpy(&usr_tdm_api.data[0],&tdm_api->cfg,sizeof(wanpipe_api_dev_cfg_t));
		break;

	case WP_API_CMD_ENABLE_RBS_EVENTS:
		/* 'usr_tdm_api.rbs_poll' is the user provided 'number of polls per second' */
		if (usr_tdm_api.rbs_poll < 20 || usr_tdm_api.rbs_poll > 100) {
			DEBUG_ERROR("%s: Error: Invalid RBS Poll Count Min=20 Max=100\n",
					tdm_api->name);
			
			usr_tdm_api.result=SANG_STATUS_INVALID_PARAMETER;
			err=-EINVAL;
		} else {

			err=-EOPNOTSUPP;
			usr_tdm_api.result=SANG_STATUS_OPTION_NOT_SUPPORTED;

			if (tdm_card_dev->rbs_enable_cnt == 0) {
				if (card && card->wandev.fe_iface.set_fe_sigctrl){
						wan_smp_flag_t smp_flags1;
						card->hw_iface.hw_lock(card->hw,&smp_flags1);
						err=card->wandev.fe_iface.set_fe_sigctrl(
								&card->fe,
								WAN_TE_SIG_POLL,
								ENABLE_ALL_CHANNELS,
								WAN_ENABLE);
						card->hw_iface.hw_unlock(card->hw,&smp_flags1);
				}

				if (err == 0) {
					tdm_card_dev->rbs_enable_cnt++;
					tdm_card_dev->rbs_poll_timeout =  (HZ * usr_tdm_api.rbs_poll) / 1000;
					tdm_api->cfg.rbs_poll = usr_tdm_api.rbs_poll;
					usr_tdm_api.result=SANG_STATUS_SUCCESS;
					err=0;
				}

			} else {
				tdm_card_dev->rbs_enable_cnt++;
				usr_tdm_api.result=SANG_STATUS_SUCCESS;
				err=0;
			}

		}
		break;

	case WP_API_CMD_DISABLE_RBS_EVENTS:

		if (tdm_card_dev->rbs_enable_cnt == 1) {

			tdm_card_dev->rbs_poll_timeout=0;
			tdm_api->cfg.rbs_poll=0;
	
			if (card && card->wandev.fe_iface.set_fe_sigctrl){
				wan_smp_flag_t smp_flags1;
				card->hw_iface.hw_lock(card->hw,&smp_flags1);
				err=card->wandev.fe_iface.set_fe_sigctrl(
							&card->fe,
							WAN_TE_SIG_POLL,
							ENABLE_ALL_CHANNELS,
							WAN_DISABLE);
				card->hw_iface.hw_unlock(card->hw,&smp_flags1);
			}else{
				usr_tdm_api.result=SANG_STATUS_OPTION_NOT_SUPPORTED;
				err=-EOPNOTSUPP;
			}
		}

		if (tdm_card_dev->rbs_enable_cnt > 0) {
			tdm_card_dev->rbs_enable_cnt--;
		}

		break;

	case WP_API_CMD_WRITE_RBS_BITS:

		if (tdm_api->write_rbs_bits) {
			err=tdm_api->write_rbs_bits(
							tdm_api->chan,
							usr_tdm_api.chan,
							(u8)usr_tdm_api.rbs_tx_bits);
			if (err) {
				usr_tdm_api.result=SANG_STATUS_IO_ERROR;
				DEBUG_ERROR("%s: WRITE RBS Error (%i)\n",tdm_api->name,err);
			}
		} else {
			usr_tdm_api.result=SANG_STATUS_OPTION_NOT_SUPPORTED;
			err=-EOPNOTSUPP;
		}
		break;

	case WP_API_CMD_READ_RBS_BITS:

		if (tdm_api->read_rbs_bits) {
			err=tdm_api->read_rbs_bits(
							tdm_api->chan,
							usr_tdm_api.chan,
							(u8*)&usr_tdm_api.rbs_rx_bits);
			if (err) {
				usr_tdm_api.result=SANG_STATUS_IO_ERROR;
				DEBUG_ERROR("%s: READ RBS Error (%d)\n",tdm_api->name,err);
			}
			DEBUG_TEST("%s: READ RBS  (0x%X)\n",tdm_api->name,usr_tdm_api.rbs_rx_bits);
		} else {
			usr_tdm_api.result=SANG_STATUS_OPTION_NOT_SUPPORTED;
			err=-EOPNOTSUPP;
		}
		break;

	case WP_API_CMD_GET_FE_STATUS:
		if (card && card->wandev.fe_iface.get_fe_status){
			wan_smp_flag_t smp_flags1;
			card->hw_iface.hw_lock(card->hw,&smp_flags1);
			card->wandev.fe_iface.get_fe_status(
					&card->fe, &usr_tdm_api.fe_status,tdm_api->tdm_chan);
			card->hw_iface.hw_unlock(card->hw,&smp_flags1);
		}else{
			usr_tdm_api.result=SANG_STATUS_OPTION_NOT_SUPPORTED;
			err=-EOPNOTSUPP;
			DEBUG_WARNING("%s: Warning: WP_API_CMD_GET_FE_STATUS request is not supported!\n",
						tdm_api->name);
		}
		break;

	case WP_API_CMD_SET_FE_STATUS:
		if (card && card->wandev.fe_iface.set_fe_status){
			wan_smp_flag_t smp_flags1;
			card->hw_iface.hw_lock(card->hw,&smp_flags1);
                 	card->wandev.fe_iface.set_fe_status(
					&card->fe, usr_tdm_api.fe_status);
			card->hw_iface.hw_unlock(card->hw,&smp_flags1);
		}else{
			usr_tdm_api.result=SANG_STATUS_OPTION_NOT_SUPPORTED;
			err=-EOPNOTSUPP;
			DEBUG_WARNING("%s: Warning: WP_API_CMD_SET_FE_STATUS request is not supported!\n",
						tdm_api->name);
		}
		break;

	case WP_API_CMD_SET_EVENT:
		err = wanpipe_tdm_api_event_ioctl(tdm_api, &usr_tdm_api);
		if (err) {
			if (err==-EBUSY) {
				usr_tdm_api.result=SANG_STATUS_DEVICE_BUSY;
			} else if (err==-EINVAL) {
				usr_tdm_api.result=SANG_STATUS_INVALID_PARAMETER;
			} else {
				usr_tdm_api.result=SANG_STATUS_IO_ERROR;
			}
		}
		break;

	case WP_API_CMD_READ_EVENT:
		wptdm_os_lock_irq(&card->wandev.lock,&irq_flags);
		skb=wan_skb_dequeue(&tdm_api->wp_event_list);
		wptdm_os_unlock_irq(&card->wandev.lock,&irq_flags);
		if (!skb){
			usr_tdm_api.result=SANG_STATUS_NO_DATA_AVAILABLE;
			err=-ENOBUFS;
			break;
		}

		err = 0;
		memcpy(&usr_tdm_api.event,wan_skb_data(skb),sizeof(wp_api_event_t));

#if DBG_FALSE_RING2
		{
			wp_api_event_t *event_ptr = &usr_tdm_api.event;

			switch(event_ptr->wp_api_event_type)
			{
			case WP_API_EVENT_RING_DETECT:
				DEBUG_FALSE_RING("%s():%s: Received WP_API_EVENT_RING_DETECT at channel: %d!\n",
					__FUNCTION__, tdm_api->name, event_ptr->channel);
				break;
			case WP_API_EVENT_RING_TRIP_DETECT:
				DEBUG_FALSE_RING("%s():%s: Received WP_API_EVENT_RING_TRIP_DETECT at channel: %d!\n",
					__FUNCTION__, tdm_api->name, event_ptr->channel);
				break;
			}
		}
#endif
		wptdm_os_lock_irq(&card->wandev.lock,&irq_flags);
		wan_skb_init(skb,sizeof(wp_api_hdr_t));
		wan_skb_trim(skb,0);
		wan_skb_queue_tail(&tdm_api->wp_event_free_list,skb);
		wptdm_os_unlock_irq(&card->wandev.lock,&irq_flags);
		break;

	case WP_API_CMD_GET_HW_CODING:
		usr_tdm_api.hw_tdm_coding = tdm_api->cfg.hw_tdm_coding;
		break;

	case WP_API_CMD_SET_RX_GAINS:

		if (usr_tdm_api.data_len && utdmapi->data) {
			uint8_t *rx_gains;
			
			if (usr_tdm_api.data_len != 256) {
				usr_tdm_api.result=SANG_STATUS_INVALID_PARAMETER;
				err=-EINVAL;
				break;
			}

			rx_gains = tdm_api->rx_gain;
					
			wan_mutex_unlock(&tdm_api->lock, &flags); 	

			if (!rx_gains) {
				rx_gains = wan_malloc(usr_tdm_api.data_len);
				if (!rx_gains) {
					usr_tdm_api.result=SANG_STATUS_FAILED_ALLOCATE_MEMORY;
					err=-ENOMEM;
					goto tdm_api_unlocked_exit;
				}
			}

			if (WAN_COPY_FROM_USER(rx_gains,
							utdmapi->data,
							usr_tdm_api.data_len)){
					usr_tdm_api.result=SANG_STATUS_FAILED_TO_LOCK_USER_MEMORY;
	       			err=-EFAULT;
				goto tdm_api_unlocked_exit;
			}

			wan_mutex_lock(&tdm_api->lock,&flags);
			
			tdm_api->rx_gain = rx_gains;

		} else {
			usr_tdm_api.result=SANG_STATUS_FAILED_TO_LOCK_USER_MEMORY;
			err=-EFAULT;
		}

		break;

	case WP_API_CMD_GET_FE_ALARMS:
		usr_tdm_api.fe_alarms = tdm_api->cfg.fe_alarms;
		break;

	case WP_API_CMD_SET_TX_GAINS:

		if (usr_tdm_api.data_len && utdmapi->data) {
			uint8_t *tx_gains;
			
			if (usr_tdm_api.data_len != 256) {
				usr_tdm_api.result=SANG_STATUS_INVALID_PARAMETER;
				err=-EINVAL;
				break;
			}

			tx_gains = tdm_api->tx_gain;
			
			wan_mutex_unlock(&tdm_api->lock, &flags);

			if (!tx_gains) {
				tx_gains = wan_malloc(usr_tdm_api.data_len);
				if (!tx_gains) {
					usr_tdm_api.result=SANG_STATUS_FAILED_ALLOCATE_MEMORY;
					err=-ENOMEM;
					goto tdm_api_unlocked_exit;
				}
			}

			if (WAN_COPY_FROM_USER(tx_gains,
							utdmapi->data,
							usr_tdm_api.data_len)){
							usr_tdm_api.result=SANG_STATUS_FAILED_TO_LOCK_USER_MEMORY;
				err=-EFAULT;
				goto tdm_api_unlocked_exit;
			}
			wan_mutex_lock(&tdm_api->lock,&flags);
			
			tdm_api->tx_gain = tx_gains;

		} else {
				usr_tdm_api.result=SANG_STATUS_FAILED_TO_LOCK_USER_MEMORY;
				err=-EFAULT;
		}

		break;



	case WP_API_CMD_CLEAR_RX_GAINS:
		if (tdm_api->rx_gain) {
			int i;
			for (i=0;i<256;i++) {
				tdm_api->rx_gain[i]=i;
			}
		}
		break;

	case WP_API_CMD_CLEAR_TX_GAINS:
		if (tdm_api->tx_gain) {
			int i;
			for (i=0;i<256;i++) {
				tdm_api->tx_gain[i]=i;
			}
		}
		break;

	case WP_API_CMD_FLUSH_BUFFERS:
		wp_tdmapi_init_buffs(tdm_api, WP_API_FLUSH_ALL);
		break;

	case WP_API_CMD_FLUSH_RX_BUFFERS:
		wp_tdmapi_init_buffs(tdm_api, WP_API_FLUSH_RX);
		break;

	case WP_API_CMD_FLUSH_TX_BUFFERS:
		wp_tdmapi_init_buffs(tdm_api, WP_API_FLUSH_TX);
		break;

	case WP_API_CMD_FLUSH_EVENT_BUFFERS:
		wp_tdmapi_init_buffs(tdm_api, WP_API_FLUSH_EVENT);
		break;

	case WP_API_CMD_SET_TX_Q_SIZE:

		if (usr_tdm_api.tx_queue_sz) {
			if (WPTDM_SPAN_OP_MODE(tdm_api) || tdm_api->hdlc_framing) {
				if (tdm_api->driver_ctrl) {
					err = tdm_api->driver_ctrl(tdm_api->chan,usr_tdm_api.cmd,&usr_tdm_api);
				} else {
					err=-EFAULT;
				}
			} else {
				tdm_api->cfg.tx_queue_sz =usr_tdm_api.tx_queue_sz;
			}
		} else {
			err=-EINVAL;
		}

		break;

	case WP_API_CMD_GET_TX_Q_SIZE:

		if (WPTDM_SPAN_OP_MODE(tdm_api) || tdm_api->hdlc_framing) {
			if (tdm_api->driver_ctrl) {
				err = tdm_api->driver_ctrl(tdm_api->chan,usr_tdm_api.cmd,&usr_tdm_api);
			} else {
				err=-EFAULT;
			}
		} else {
			usr_tdm_api.tx_queue_sz = tdm_api->cfg.tx_queue_sz;
		}

		break;

	case WP_API_CMD_SET_RX_Q_SIZE:

		if (usr_tdm_api.rx_queue_sz) {
			tdm_api->cfg.rx_queue_sz =usr_tdm_api.rx_queue_sz;
		} else {
			err=-EINVAL;
		}
		break;

	case WP_API_CMD_GET_RX_Q_SIZE:
		usr_tdm_api.rx_queue_sz = tdm_api->cfg.rx_queue_sz;
		break;

	case WP_API_CMD_SET_RM_RXFLASHTIME:
		if (card->wandev.config_id == WANCONFIG_AFT_ANALOG) {
			if (fe->rm_param.mod[(tdm_api->tdm_chan) - 1].type == MOD_TYPE_FXS) {
				DEBUG_EVENT("%s: Module %d: rxflashtime changed (%d -> %d)\n",
					fe->name,
					tdm_api->tdm_chan,
					fe->rm_param.mod[(tdm_api->tdm_chan) - 1].u.fxs.rxflashtime,
					usr_tdm_api.rxflashtime);
				fe->rm_param.mod[(tdm_api->tdm_chan) - 1].u.fxs.rxflashtime=usr_tdm_api.rxflashtime;						
			} else {
				DEBUG_WARNING("Warning %s: Module %d: WP_API_CMD_SET_RM_RXFLASHTIME is only valid for FXS module!\n", 
					fe->name,tdm_api->tdm_chan);
				err=-EOPNOTSUPP;
			}
		} else {
				DEBUG_WARNING("Warning %s : Unsupported Protocol/Card %s WP_API_CMD_SET_RM_RXFLASHTIME is only valid for analog!\n", 
					fe->name,SDLA_DECODE_PROTOCOL(card->wandev.config_id));
				err=-EOPNOTSUPP;
		}
		break;

	case WP_API_CMD_ENABLE_LOOP:

		if (WPTDM_CHAN_OP_MODE(tdm_api)) {
			if (wan_test_bit(channel, &card->wandev.fe_ec_map)) {				
				if (card && card->wandev.ec_enable) {
					int terr=card->wandev.ec_enable(card, 0, channel);
					if (terr == 0) {
						tdm_api->loop_reenable_hwec=1;
					}
				}
			}

			tdm_api->cfg.loop=1;

		} else {
			err=-EOPNOTSUPP;
		}
			
		
		break;

	case WP_API_CMD_DISABLE_LOOP:

		if (WPTDM_CHAN_OP_MODE(tdm_api)) {
			if (tdm_api->loop_reenable_hwec) {
				if (card && card->wandev.ec_enable) {
					card->wandev.ec_enable(card, 1, channel);
				}
			}

			tdm_api->cfg.loop=0;
		} else {
			err=-EOPNOTSUPP;
		}
		break;

	case WP_API_CMD_DRIVER_VERSION:
	case WP_API_CMD_FIRMWARE_VERSION:
	case WP_API_CMD_CPLD_VERSION:
	case WP_API_CMD_GEN_FIFO_ERR_TX:
	case WP_API_CMD_GEN_FIFO_ERR_RX:
	case WP_API_CMD_SS7_GET_CFG_STATUS:
		if (tdm_api->driver_ctrl) {
			err = tdm_api->driver_ctrl(tdm_api->chan,usr_tdm_api.cmd,&usr_tdm_api);
		} else {
			err=-EFAULT;
		}
		break;

	case WP_API_CMD_BUFFER_MULTIPLIER:

		if (WPTDM_SPAN_OP_MODE(tdm_api) && !tdm_api->hdlc_framing) {
			int mult = *(uint32_t*)&usr_tdm_api.data[0];
			if (mult > TDMAPI_MAX_BUFFER_MULTIPLIER) {
				DEBUG_ERROR("%s: Error: Inavlid buffer multiplier %i\n",tdm_api->name,mult);
				err=-EINVAL;
				goto tdm_api_exit;
			}
			if (mult < 2) {
				tdm_api->buffer_multiplier = 1;
			} else {
				tdm_api->buffer_multiplier = mult;
			}
			tdm_api->cfg.usr_mtu_mru = tdm_api->cfg.usr_period*tdm_api->cfg.hw_mtu_mru*tdm_api->buffer_multiplier *tdm_api->timeslots;
			DEBUG_TDMAPI("%s: Setting Multiplier %i, New MTU %i (p=%i,hwmtu=%i)\n",
				tdm_api->name,tdm_api->buffer_multiplier,tdm_api->cfg.usr_mtu_mru,tdm_api->cfg.usr_period,tdm_api->cfg.hw_mtu_mru);

		} else {
			DEBUG_WARNING("Warning: %s: WP_API_CMD_BUFFER_MULTIPLIER not supported for device mode\n", tdm_api->name);
			err=-EOPNOTSUPP;
		}

		break;

	default:
		DEBUG_EVENT("%s: Invalid TDM API CMD %i\n", tdm_api->name,cmd);
		usr_tdm_api.result=SANG_STATUS_OPTION_NOT_SUPPORTED;
		err=-EOPNOTSUPP;
		break;
	}

tdm_api_exit:
	wan_mutex_unlock(&tdm_api->lock, &flags);

tdm_api_unlocked_exit:

	usr_tdm_api.result = err;


#if defined(__WINDOWS__)
	/* udata is a pointer to wanpipe_api_cmd_t */
	memcpy(udata, &usr_tdm_api, sizeof(wanpipe_api_cmd_t));
#else
	if (WAN_COPY_TO_USER(udata,
			     &usr_tdm_api,
			     sizeof(wanpipe_api_cmd_t))){
		usr_tdm_api.result=SANG_STATUS_FAILED_TO_LOCK_USER_MEMORY;
	    return -EFAULT;
	}
#endif

	return err;

}

static int wanpipe_tdm_api_ioctl(void *obj, int cmd, void *udata)
{
	wanpipe_tdm_api_dev_t *tdm_api = (wanpipe_tdm_api_dev_t*)obj;
	int err=0;

	if (!tdm_api->chan && tdm_api != &tdmapi_ctrl){
		DEBUG_ERROR("%s:%d Error: TDM API Not initialized! chan=NULL!\n",
			__FUNCTION__,__LINE__);
		return -EFAULT;
	}

	if (!udata){
		return -EINVAL;
	}

	switch (cmd) {

#if defined (__LINUX__)
	case SIOC_WANPIPE_TDM_API:
#endif
	case WANPIPE_IOCTL_API_CMD:
		err=wanpipe_tdm_api_ioctl_handle_tdm_api_cmd(tdm_api,udata);
		break;

	case WANPIPE_IOCTL_PIPEMON:
		err=-EINVAL;
		if (tdm_api->pipemon) {
			wan_smp_flag_t flag;
			wan_mutex_lock(&tdm_api->lock,&flag);
			err=tdm_api->pipemon(tdm_api->card,tdm_api->chan,udata);
			wan_mutex_unlock(&tdm_api->lock,&flag);
		}
		break;

	default:
		err=-EINVAL;
		break;
	}	

	return err;
}

static int
wanpipe_tdm_api_event_ioctl(wanpipe_tdm_api_dev_t *tdm_api, wanpipe_api_cmd_t *tdm_cmd)
{
	wp_api_event_t	*tdm_event;
	wan_event_ctrl_t	event_ctrl;
	sdla_t *card=tdm_api->card;
	int channel = tdm_api->tdm_chan;

	if (WPTDM_SPAN_OP_MODE(tdm_api)) {
		if (tdm_cmd->chan) {
			channel = tdm_cmd->chan;
		}
	}

	if (channel >= MAX_TDM_API_CHANNELS) {
		DEBUG_ERROR("%s: %s(): Error: Invalid channel selected %i  (Max=%i)\n",
					tdm_api->name, __FUNCTION__, channel, MAX_TDM_API_CHANNELS);
		return -EINVAL;
	}

	if (tdm_api->event_ctrl == NULL){
		DEBUG_ERROR("%s: Error: Event control interface doesn't initialized!\n",
				tdm_api->name);
		return -EINVAL;
	}

	tdm_event = &tdm_cmd->event;
	memset(&event_ctrl, 0, sizeof(wan_event_ctrl_t));

	switch(tdm_event->wp_api_event_type){


	case WP_API_EVENT_DTMF:

		event_ctrl.type = WAN_EVENT_EC_DTMF;

		if (card && card->wandev.ec_state) {
			wan_hwec_dev_state_t ecdev_state = {0};
			card->wandev.ec_state(card,&ecdev_state);
			if (ecdev_state.ec_state == 0) {
				return -EINVAL;
			}
		} else {
			return -EINVAL;
		}

		DEBUG_TEST("%s: %s HW EC DTMF event %X %p !\n",
					tdm_api->name,
					WP_API_EVENT_MODE_DECODE(tdm_event->wp_api_event_mode),
					tdm_api->active_ch,card->wandev.ec_enable);

		// Octasic DTMF event
		if (tdm_event->wp_api_event_mode == WP_API_EVENT_ENABLE){
				event_ctrl.mode = WAN_EVENT_ENABLE;
		}else{
				event_ctrl.mode = WAN_EVENT_DISABLE;
		}

		event_ctrl.channel      = channel;
		break;

	case WP_API_EVENT_FAX_DETECT:

		event_ctrl.type = WAN_EVENT_EC_FAX_DETECT;

		if (card && card->wandev.ec_state) {
			wan_hwec_dev_state_t ecdev_state = {0};
			card->wandev.ec_state(card,&ecdev_state);
			if (ecdev_state.ec_state == 0) {
				return -EINVAL;
			}
		} else {
			return -EINVAL;
		}

		DEBUG_TEST("%s: %s HW EC FAX Detect event %X %p !\n",
					tdm_api->name,
					WP_API_EVENT_MODE_DECODE(tdm_event->wp_api_event_mode),
					tdm_api->active_ch,card->wandev.ec_enable);

		// Octasic DTMF event
		if (tdm_event->wp_api_event_mode == WP_API_EVENT_ENABLE){
				event_ctrl.mode = WAN_EVENT_ENABLE;
		}else{
				event_ctrl.mode = WAN_EVENT_DISABLE;
		}

		event_ctrl.channel      = channel;

		break;

	case WP_API_EVENT_RM_DTMF:
		// A200-Remora DTMF event
		DEBUG_TDMAPI("%s: %s A200-Remora DTMF event!\n",
			tdm_api->name,
			WP_API_EVENT_MODE_DECODE(tdm_event->wp_api_event_mode));
		event_ctrl.type		= WAN_EVENT_RM_DTMF;
		if (tdm_event->wp_api_event_mode == WP_API_EVENT_ENABLE){
			event_ctrl.mode = WAN_EVENT_ENABLE;
		}else{
			event_ctrl.mode = WAN_EVENT_DISABLE;
		}
		event_ctrl.mod_no	= channel;
		break;

	case WP_API_EVENT_RXHOOK:
		DEBUG_TDMAPI("%s: %s A200-Remora Loop Closure event!\n",
			tdm_api->name,
			WP_API_EVENT_MODE_DECODE(tdm_event->wp_api_event_mode));
		event_ctrl.type		= WAN_EVENT_RM_LC;
		if (tdm_event->wp_api_event_mode == WP_API_EVENT_ENABLE){
			event_ctrl.mode = WAN_EVENT_ENABLE;
		}else{
			event_ctrl.mode = WAN_EVENT_DISABLE;
		}
		event_ctrl.mod_no	= channel;
		break;

	case WP_API_EVENT_RING:
		DEBUG_TDMAPI("%s: %s Ring Event on module %d!\n",
			tdm_api->name,
			WP_API_EVENT_MODE_DECODE(tdm_event->wp_api_event_mode),
						channel);
		event_ctrl.type   	= WAN_EVENT_RM_RING;
		if (tdm_event->wp_api_event_mode == WP_API_EVENT_ENABLE){
			event_ctrl.mode = WAN_EVENT_ENABLE;
		}else{
			event_ctrl.mode = WAN_EVENT_DISABLE;
		}
		event_ctrl.mod_no	= channel;
		break;

	case WP_API_EVENT_RING_DETECT:
		DEBUG_TDMAPI("%s: %s Ring Detection Event on module %d!\n",
			tdm_api->name,
			WP_API_EVENT_MODE_DECODE(tdm_event->wp_api_event_mode),
					channel);
		event_ctrl.type   	= WAN_EVENT_RM_RING_DETECT;
		if (tdm_event->wp_api_event_mode == WP_API_EVENT_ENABLE){
			event_ctrl.mode = WAN_EVENT_ENABLE;
		}else{
			event_ctrl.mode = WAN_EVENT_DISABLE;
		}
		event_ctrl.mod_no	= channel;
		break;

	case WP_API_EVENT_RING_TRIP_DETECT:
		DEBUG_TDMAPI("%s: %s Ring Trip Detection Event on module %d!\n",
			tdm_api->name,
			WP_API_EVENT_MODE_DECODE(tdm_event->wp_api_event_mode),
						channel);
		event_ctrl.type   	= WAN_EVENT_RM_RING_TRIP;
		if (tdm_event->wp_api_event_mode == WP_API_EVENT_ENABLE){
			event_ctrl.mode = WAN_EVENT_ENABLE;
		}else{
			event_ctrl.mode = WAN_EVENT_DISABLE;
		}
		event_ctrl.mod_no	= channel;
		break;

	case WP_API_EVENT_TONE:

		DEBUG_TDMAPI("%s: %s Tone Event (%d)on module %d!\n",
			tdm_api->name,
			WP_API_EVENT_MODE_DECODE(tdm_event->wp_api_event_mode),
			tdm_event->wp_api_event_tone_type,
						channel);
		event_ctrl.type   	= WAN_EVENT_RM_TONE;
		if (tdm_event->wp_api_event_mode == WP_API_EVENT_ENABLE){
			event_ctrl.mode = WAN_EVENT_ENABLE;
			switch(tdm_event->wp_api_event_tone_type){
			case WP_API_EVENT_TONE_DIAL:
				event_ctrl.tone	= WAN_EVENT_RM_TONE_TYPE_DIAL;
				break;
			case WP_API_EVENT_TONE_BUSY:
				event_ctrl.tone	= WAN_EVENT_RM_TONE_TYPE_BUSY;
				break;
			case WP_API_EVENT_TONE_RING:
				event_ctrl.tone	= WAN_EVENT_RM_TONE_TYPE_RING;
				break;
			case WP_API_EVENT_TONE_CONGESTION:
				event_ctrl.tone	= WAN_EVENT_RM_TONE_TYPE_CONGESTION;
				break;
			default:
				DEBUG_EVENT("%s: Unsupported TDM API Tone Type  %d!\n",
						tdm_api->name,
						tdm_event->wp_api_event_tone_type);
				return -EINVAL;
			}
		}else{
			event_ctrl.mode = WAN_EVENT_DISABLE;
		}
		event_ctrl.mod_no	= channel;
		break;

	case WP_API_EVENT_TXSIG_KEWL:
		DEBUG_TDMAPI("%s: TX Signalling KEWL on module %d!\n",
				tdm_api->name, channel);
		event_ctrl.type   	= WAN_EVENT_RM_TXSIG_KEWL;
		event_ctrl.mod_no	= channel;
		break;

	case WP_API_EVENT_TXSIG_START:
		DEBUG_TDMAPI("%s: TX Signalling START for module %d!\n",
				tdm_api->name, channel);
		event_ctrl.type		= WAN_EVENT_RM_TXSIG_START;
		event_ctrl.mod_no	= channel;
		break;

	case WP_API_EVENT_TXSIG_OFFHOOK:
		DEBUG_TDMAPI("%s: TX Signalling OFFHOOK for module %d!\n",
				tdm_api->name, channel);
		event_ctrl.type		= WAN_EVENT_RM_TXSIG_OFFHOOK;
		event_ctrl.mod_no	= channel;
		break;

	case WP_API_EVENT_TXSIG_ONHOOK:
		DEBUG_TDMAPI("%s: TX Signalling ONHOOK for module %d!\n",
				tdm_api->name, channel);
		event_ctrl.type		= WAN_EVENT_RM_TXSIG_ONHOOK;
		event_ctrl.mod_no	= channel;
		break;

	case WP_API_EVENT_ONHOOKTRANSFER:
		DEBUG_TDMAPI("%s: RM ONHOOKTRANSFER for module %d!\n",
				tdm_api->name, channel);
		event_ctrl.type		= WAN_EVENT_RM_ONHOOKTRANSFER;
		event_ctrl.mod_no	= channel;
		event_ctrl.ohttimer	= tdm_event->wp_api_event_ohttimer;
		break;

	case WP_API_EVENT_SETPOLARITY:
		DEBUG_TDMAPI("%s: RM SETPOLARITY(%s) for module %d!\n",
				tdm_api->name, tdm_event->wp_api_event_polarity == 0 ? "Forward" : "Reverse" ,channel);
		event_ctrl.type		= WAN_EVENT_RM_SETPOLARITY;
		event_ctrl.mod_no	= channel;
		event_ctrl.polarity	= tdm_event->wp_api_event_polarity;
		break;

	case WP_API_EVENT_BRI_CHAN_LOOPBACK:
		event_ctrl.type		= WAN_EVENT_BRI_CHAN_LOOPBACK;
		event_ctrl.channel	= tdm_event->channel;

		if (tdm_event->wp_api_event_mode == WP_API_EVENT_ENABLE){
			event_ctrl.mode = WAN_EVENT_ENABLE;
		}else{
			event_ctrl.mode = WAN_EVENT_DISABLE;
		}

		DEBUG_TDMAPI("%s: BRI_BCHAN_LOOPBACK: %s for channel %d!\n",
			tdm_api->name,
			(event_ctrl.mode == WAN_EVENT_ENABLE ? "Enable" : "Disable"),
			event_ctrl.channel);
		break;

	case WP_API_EVENT_SET_RM_TX_GAIN:
		DEBUG_TDMAPI("%s: Setting TX gain %d on  %d!\n",
				tdm_api->name, tdm_event->wp_api_event_gain_value ,channel);
		event_ctrl.type		= WAN_EVENT_RM_SET_TX_GAIN;
		event_ctrl.rm_gain	= (int)tdm_event->wp_api_event_gain_value;
		event_ctrl.mod_no	= channel;
		break;

	case WP_API_EVENT_SET_RM_RX_GAIN:
		DEBUG_TDMAPI("%s: Setting RX gains %d on  %d!\n",
				tdm_api->name, tdm_event->wp_api_event_gain_value,channel);
		event_ctrl.type		= WAN_EVENT_RM_SET_RX_GAIN;
		event_ctrl.rm_gain	= tdm_event->wp_api_event_gain_value;
		event_ctrl.mod_no	= channel;
		break;
		
	default:
		DEBUG_EVENT("%s: Unknown TDM API Event Type %02X!\n",
				tdm_api->name,
				tdm_event->wp_api_event_type);
		return -EINVAL;
	}

	return tdm_api->event_ctrl(tdm_api->chan, &event_ctrl);
}

static int wanpipe_tdm_api_channelized_tx (wanpipe_tdm_api_dev_t *tdm_api, u8 *tx_data, int len)
{
	u8 *buf;
	wp_api_hdr_t *tx_hdr;

	if (wan_test_bit(0,&tdm_api->cfg.tx_disable)){
		return 0;
	}

	if (!tdm_api->tx_skb) {
		tdm_api->tx_skb=wan_skb_dequeue(&tdm_api->wp_tx_list);
		if (!tdm_api->tx_skb){
			memset(tx_data,tdm_api->cfg.idle_flag,len);
			tdm_api->cfg.stats.tx_carrier_errors++;
			tdm_api->cfg.stats.tx_idle_packets++;
			return -ENOBUFS;
		}

		wp_wakeup_tx_tdmapi(tdm_api);
		buf=wan_skb_pull(tdm_api->tx_skb,sizeof(wp_api_hdr_t));
		memcpy(&tdm_api->tx_hdr, buf, sizeof(wp_api_hdr_t));
		tdm_api->cfg.stats.tx_bytes+=wan_skb_len(tdm_api->tx_skb);

		if (wan_skb_len(tdm_api->tx_skb) % len) {
			WP_AFT_CHAN_ERROR_STATS(tdm_api->cfg.stats,tx_errors);
			goto wanpipe_tdm_api_tx_error;
		}
	}

	tx_hdr=&tdm_api->tx_hdr;
	
	if (wan_skb_len(tdm_api->tx_skb) < len) {
#if 0
			if (WAN_NET_RATELIMIT()) {
				DEBUG_EVENT("%s: TX LEN %i Less then Expecting %i\n",
						tdm_api->name, wan_skb_len(tdm_api->tx_skb) , len);
			}
#endif
		WP_AFT_CHAN_ERROR_STATS(tdm_api->cfg.stats,tx_errors);
		goto wanpipe_tdm_api_tx_error;
	}

	buf=(u8*)wan_skb_data(tdm_api->tx_skb);
	memcpy(tx_data,buf,len);
	wan_skb_pull(tdm_api->tx_skb,len);

	if (wan_skb_len(tdm_api->tx_skb) <= 0) {
		tdm_api->cfg.stats.tx_packets++;
		wan_skb_init(tdm_api->tx_skb,sizeof(wp_api_hdr_t));
		wan_skb_trim(tdm_api->tx_skb,0);
		wan_skb_queue_tail(&tdm_api->wp_tx_free_list,tdm_api->tx_skb);
		tdm_api->tx_skb=NULL;
	}

	return 0;

wanpipe_tdm_api_tx_error:
			
	memset(tx_data,tdm_api->cfg.idle_flag,len);
	tdm_api->cfg.stats.tx_carrier_errors++;
	tdm_api->cfg.stats.tx_idle_packets++;

	if (tdm_api->tx_skb) {
		wan_skb_init(tdm_api->tx_skb,sizeof(wp_api_hdr_t));
		wan_skb_trim(tdm_api->tx_skb,0);
		wan_skb_queue_tail(&tdm_api->wp_tx_free_list,tdm_api->tx_skb);
		tdm_api->tx_skb=NULL;
	}

	tdm_api->tx_skb=NULL;
	return -1;

}

static void wanpipe_tdm_timestamp_hdr(wp_api_hdr_t *hdr)
{
	wan_time_t sec;
	wan_suseconds_t usec;
	
	wan_get_timestamp(&sec, &usec);

	hdr->wp_api_hdr_time_stamp_sec = (u32)sec;
	hdr->wp_api_hdr_time_stamp_use = usec;
	return;
}

static int wanpipe_tdm_api_channelized_rx (wanpipe_tdm_api_dev_t *tdm_api, u8 *rx_data, u8 *tx_data, int len)
{
	u8 *data_ptr;
	wp_api_hdr_t *rx_hdr;
	int err=-EINVAL;
	int skblen;
	int hdrsize = sizeof(wp_api_hdr_t);
	sdla_t  *card = (sdla_t*)tdm_api->card;

	if (wan_test_bit(0,&tdm_api->cfg.rx_disable)) {
		err=0;
		goto wanpipe_tdm_api_rx_error;
	}

	if (card && (WANCONFIG_AFT_ANALOG == card->wandev.config_id)) {
		/*Only for Analog card - check and report fake polarity event(If applicable)*/
		wp_tdmapi_check_fakepolarity(rx_data,len,tdm_api);
	}

	if (wan_skb_queue_len(&tdm_api->wp_rx_list) >= (int)tdm_api->cfg.rx_queue_sz) {
		WP_AFT_CHAN_ERROR_STATS(tdm_api->cfg.stats,rx_dropped);
		wp_wakeup_rx_tdmapi(tdm_api);
		err=-EBUSY;
		goto wanpipe_tdm_api_rx_error;
	}

	if (!tdm_api->rx_skb) {
		tdm_api->rx_skb=wan_skb_dequeue(&tdm_api->wp_rx_free_list);
		if (!tdm_api->rx_skb) {
			err=-ENOMEM;
			WP_AFT_CHAN_ERROR_STATS(tdm_api->cfg.stats,rx_errors);
			goto wanpipe_tdm_api_rx_error;
		}

#if 0
		if (wan_skb_headroom(tdm_api->rx_skb) != sizeof(wp_api_hdr_t)) {
#if 0
			if (WAN_NET_RATELIMIT()) {
				DEBUG_EVENT("%s: RX HEAD ROOM =%i Expecting %i\n",
						tdm_api->name, wan_skb_headroom(tdm_api->rx_skb), sizeof(wp_api_hdr_t));
			}
#endif
			WP_AFT_CHAN_ERROR_STATS(tdm_api->cfg.stats,rx_errors);
			goto wanpipe_tdm_api_rx_error;
		}
#endif

		data_ptr=wan_skb_put(tdm_api->rx_skb,hdrsize);
		memset(data_ptr,0,hdrsize);
	}

#if 0
	if (len > wan_skb_tailroom(tdm_api->rx_skb)) {
		err=-EINVAL;
		WP_AFT_CHAN_ERROR_STATS(tdm_api->cfg.stats,rx_errors);
#if 0
		if (WAN_NET_RATELIMIT()) {
			DEBUG_EVENT("%s: RX TAIL ROOM =%i Less than Len %i\n",
						tdm_api->name, wan_skb_tailroom(tdm_api->rx_skb), len);
		}
#endif
		goto wanpipe_tdm_api_rx_error;
	}
#endif

	rx_hdr=(wp_api_hdr_t*)wan_skb_data(tdm_api->rx_skb);

	data_ptr=wan_skb_put(tdm_api->rx_skb,len);
	memcpy((u8*)data_ptr,rx_data,len);

	skblen = wan_skb_len(tdm_api->rx_skb);
	if (skblen >= tdm_api->mtu_mru) {

		wanpipe_tdm_timestamp_hdr(rx_hdr);

		tdm_api->cfg.stats.rx_bytes+=skblen-hdrsize;
		wan_skb_queue_tail(&tdm_api->wp_rx_list,tdm_api->rx_skb);
		tdm_api->rx_skb=NULL;
		wp_wakeup_rx_tdmapi(tdm_api);
		tdm_api->cfg.stats.rx_packets++;
		
	}

	return 0;

wanpipe_tdm_api_rx_error:

	if (tdm_api->rx_skb) {
		netskb_t *skb = tdm_api->rx_skb;
		tdm_api->rx_skb = NULL;

		wan_skb_init(skb,hdrsize);
		wan_skb_trim(skb,0);
		wan_skb_queue_tail(&tdm_api->wp_rx_free_list,skb);
	}

	tdm_api->rx_skb=NULL;

	return err;
}

static __inline int wp_tdmapi_check_fakepolarity(u8 *buf, int len, wanpipe_tdm_api_dev_t* tdm_api)
{
	sdla_fe_t        *fe    	= NULL;
	wp_remora_fxo_t  *fxo   	= NULL;
	int              channo 	= 0;
	sdla_t          *card   	= NULL;
	int             upper_thres_val = 0;
	int             lower_thres_val = 0;
	int 		i		= 0;
	int 		linear_sample	= 0;
	int             fake_polarity_thres = 0;

	if(!buf || !tdm_api || !len || !tdm_api->card) {
		return 0 ;
	}

	card 	= (sdla_t*)tdm_api->card;
	fe 	= &card->fe;
	channo 	= tdm_api->tdm_chan;
	fxo 	= &fe->rm_param.mod[channo].u.fxo;

	if (WANOPT_NO == card->fe.fe_cfg.cfg.remora.fake_polarity) {
		/*RM_FAKE_POLARITY disabled */
		return 0;
	}

	/* only look for sound on the line if its an fxo card and state is onhook*/
	if (MOD_TYPE_FXO != fe->rm_param.mod[channo].type) {
			return 0;
	}

	if (fxo->offhook) {
		fxo->cidtimer = fe->rm_param.intcount;
		return 0;
	}

	/* If no possible CID has been reported yet, scan the signal looking for samples bigger than the user-specified threshold */
	if (!fxo->readcid && !fxo->wasringing && fe->rm_param.intcount > fxo->cidtimer + 400) {

		fake_polarity_thres = card->fe.fe_cfg.cfg.remora.fake_polarity_thres;

		if(0 == fake_polarity_thres) {
			/*threshold value not configured...but RM_FAKE_POLARITY enable , 
			 * hence considering default threshold value*/
			fake_polarity_thres = WP_TDM_DEF_FAKE_POLARITY_THRESHOLD;
		}
		upper_thres_val = fake_polarity_thres;
		lower_thres_val = -1 * fake_polarity_thres;

		for (i=0;i<len;i++) {
			linear_sample = wanpipe_codec_convert_to_linear(buf[i],tdm_api->cfg.hw_tdm_coding);

			if (linear_sample > upper_thres_val || linear_sample < lower_thres_val) {
				if (WAN_NET_RATELIMIT()) {
					DEBUG_EVENT("%s: Possible CID signal detected, faking polarity reverse event on module %d\n", card->devname, channo);
				}
				wp_tdmapi_report_fakepolarityreverse(card,channo);
				fxo->readcid = 1;
				fxo->cidtimer = fe->rm_param.intcount;
				return 1;
			}
		}
	} else if (fxo->readcid && fe->rm_param.intcount > fxo->cidtimer + 2000) {
		fxo->cidtimer = fe->rm_param.intcount;
		fxo->readcid = 0;
	}

	return 0;
}

static void wp_tdmapi_report_fakepolarityreverse(void* card_id,u_int8_t channel)
{
	wan_event_t event;

	memset(&event,0,sizeof(event));
	event.channel 		= channel;
	event.type      	= WAN_EVENT_RM_POLARITY_REVERSE;
	event.polarity_reverse 	= WAN_EVENT_POLARITY_REV_POSITIVE_NEGATIVE;

	wp_tdmapi_polarityreverse(card_id, &event);
	return;
}

#undef AFT_TDM_ROTATE_DEBUG
#ifdef AFT_TDM_ROTATE_DEBUG
#warning "AFT_TDM_ROTATE_DEBUG Enabled"
#include "aft_core.h"
static int rotate_debug[32];
#endif
int wanpipe_tdm_api_span_rx_tx(sdla_t *card, wanpipe_tdm_api_span_t *tdm_span, u32 buf_sz, unsigned long mask, int circ_buf_len)
{
	int i,l_ch,tidx;
	wanpipe_tdm_api_dev_t *tdm_api;
	unsigned int rx_offset, tx_offset;
	void *ptr;

	if (!tdm_span || !card) {
		return -ENODEV;
	}

#ifdef AFT_TDM_ROTATE_DEBUG
	rotate_debug[card->tdmv_conf.span_no]++;
#endif

	for (i=0;i<32;i++) {
		if (wan_test_bit(i,&tdm_span->timeslot_map)) {
			tdm_api = &tdm_span->chans[i];

			if (IS_E1_CARD(card)) {
				tidx=i;
			} else {
				tidx=i-1;
			}
			
			l_ch=i-1;

#ifdef AFT_TDM_ROTATE_DEBUG
			if (1) { 
				u32 dma_descr, lo_reg, lo_reg_tx, lch;

				lch=i-1;

				dma_descr=(lch<<4) +
							AFT_PORT_REG(card,AFT_RX_DMA_LO_DESCR_BASE_REG);
				card->hw_iface.bus_read_4(card->hw,dma_descr,&lo_reg);
			
				dma_descr=(lch<<4) +
							AFT_PORT_REG(card,AFT_TX_DMA_LO_DESCR_BASE_REG);
				card->hw_iface.bus_read_4(card->hw,dma_descr,&lo_reg_tx);
			
				lo_reg=(lo_reg&AFT_TDMV_BUF_MASK)/AFT_TDMV_CIRC_BUF;
				lo_reg_tx=(lo_reg_tx&AFT_TDMV_BUF_MASK)/AFT_TDMV_CIRC_BUF;

				if (rotate_debug[card->tdmv_conf.span_no] < 5 ||
					abs(card->u.aft.tdm_rx_dma_toggle[tidx] - lo_reg) != 2 ||
					abs(card->u.aft.tdm_tx_dma_toggle[tidx] - lo_reg_tx) != 2) {
					DEBUG_EVENT("%s: Rotating buffer lcn=%i tdx=%i  rxdma=%i (hw=%i), txdma=%i (hw=%i)\n",
						card->devname, lch, tidx, card->u.aft.tdm_rx_dma_toggle[tidx], lo_reg,card->u.aft.tdm_tx_dma_toggle[tidx],lo_reg_tx);
				}
			}
#endif
			if (buf_sz) {
				rx_offset = buf_sz * card->u.aft.tdm_rx_dma_toggle[tidx];
				tx_offset = buf_sz * card->u.aft.tdm_tx_dma_toggle[tidx];
				card->u.aft.tdm_rx_dma_toggle[tidx]++;
				if (card->u.aft.tdm_rx_dma_toggle[tidx] >= circ_buf_len) {
					card->u.aft.tdm_rx_dma_toggle[tidx]=0;
				}
				card->u.aft.tdm_tx_dma_toggle[tidx]++;
				if (card->u.aft.tdm_tx_dma_toggle[tidx] >= circ_buf_len) {
					card->u.aft.tdm_tx_dma_toggle[tidx]=0;
				}

				ptr=(void*)((((ulong_ptr_t)tdm_api->rx_buf) & ~((ulong_ptr_t)mask)) + rx_offset);
				tdm_api->rx_buf = ptr;

				ptr=(void*)((((ulong_ptr_t)tdm_api->tx_buf) & ~((ulong_ptr_t)mask)) + tx_offset);
				tdm_api->tx_buf = ptr;
			}

			if (!tdm_api || !tdm_api->chan || !wan_test_bit(0,&tdm_api->init)){
				continue;
			}


#if 0
			if (tdm_api->tdm_span == 5 && tdm_api->tdm_chan == 15) {
				static unsigned char sync=0;
				int x;
				for (x=0;x<tdm_api->rx_buf_len;x++) {
					sync++;
					if (sync != tdm_api->rx_buf[x]) {
						DEBUG_ERROR("Error: RX Span 5 chan 15 expecting %02X got %02X diff=%i rlen=%i tlen=%i rxoff=%i tx_off=%i\n",
								sync,tdm_api->rx_buf[x],abs(sync-tdm_api->rx_buf[x]), tdm_api->rx_buf_len, tdm_api->tx_buf_len,
								card->u.aft.tdm_rx_dma_toggle[i],card->u.aft.tdm_tx_dma_toggle[i]);
						sync=tdm_api->rx_buf[x];
					}
				}
			}			
#endif

#if defined(__LINUX__)

			if (card->u.aft.tdm_rx_dma[l_ch]) {			
				wan_dma_descr_t *dma_descr = card->u.aft.tdm_rx_dma[l_ch]; 
				card->hw_iface.busdma_sync(card->hw, dma_descr, 1, 1, PCI_DMA_FROMDEVICE);
			}

			if (card->u.aft.tdm_tx_dma[l_ch]) {
				wan_dma_descr_t *dma_descr = card->u.aft.tdm_tx_dma[l_ch]; 
				card->hw_iface.busdma_sync(card->hw, dma_descr, 1, 1, PCI_DMA_TODEVICE);
			}
 
			prefetch(tdm_api->rx_buf);
			prefetch(tdm_api->tx_buf);
#endif
  
			if (!wan_test_bit(0,&tdm_api->used)) {
				/* Always use 'cfg.idle_flag' because it may be it was set by WP_API_CMD_SET_IDLE_FLAG */
				memset(tdm_api->tx_buf,tdm_api->cfg.idle_flag,tdm_api->tx_buf_len);
				continue;
			}

#if 0
			/* NC: Enable rtp tap for api */
			if (card->wandev.rtp_len && card->wandev.rtp_tap) {
				card->wandev.rtp_tap(card,tidx,
						     tdm_api->rx_buf,
						     tdm_api->tx_buf,
						     tdm_api->rx_buf_len);
			}
#endif

			wanpipe_tdm_api_channelized_rx (tdm_api, tdm_api->rx_buf, tdm_api->tx_buf, tdm_api->rx_buf_len);
			wanpipe_tdm_api_channelized_tx (tdm_api, tdm_api->tx_buf, tdm_api->tx_buf_len);


			if (tdm_api->cfg.loop) {
				memcpy(tdm_api->tx_buf, tdm_api->rx_buf, tdm_api->rx_buf_len);
			}

#if 0
			if (tdm_api->tdm_span == 5 && tdm_api->tdm_chan == 15) {
				static unsigned char tsync=0;
				int x;
				for (x=0;x<tdm_api->tx_buf_len;x++) {
					tsync++;
					if (tsync != tdm_api->tx_buf[x]) {
						DEBUG_ERROR("Error: TX Span 5 chan 15 expecting %02X got %02X diff=%i tx_offset\n",
								tsync,tdm_api->tx_buf[x],abs(tsync-tdm_api->tx_buf[x]));
						tsync=tdm_api->tx_buf[x];
					}
				}
			}
#endif
		}
	}
	
	return 0;
}


int wanpipe_tdm_api_rx_tx (wanpipe_tdm_api_dev_t *tdm_api, u8 *rx_data, u8 *tx_data, int len)
{

	if (!tdm_api || !rx_data || !tx_data || len == 0) {
		if (WAN_NET_RATELIMIT()) {
			DEBUG_ERROR("%s:%d Internal Error: tdm_api=%p rx_data=%p tx_data=%p len=%i\n",
					__FUNCTION__,__LINE__,tdm_api,rx_data,tx_data,len);
		}
		return 0;
	}

	if (!tdm_api->chan || !wan_test_bit(0,&tdm_api->init)){
		return 0;
	}

	tdm_api->rx_buf=rx_data;
	tdm_api->rx_buf_len=len;
	tdm_api->tx_buf=tx_data;
	tdm_api->tx_buf_len=len;

	if (!wan_test_bit(0,&tdm_api->used)) {
		/* Always use 'cfg.idle_flag' because it may be it was set by WP_API_CMD_SET_IDLE_FLAG */
		memset(tdm_api->tx_buf,tdm_api->cfg.idle_flag,len);
		return 0;
	}

	wanpipe_tdm_api_channelized_rx (tdm_api, tdm_api->rx_buf, tdm_api->tx_buf, tdm_api->rx_buf_len);
	wanpipe_tdm_api_channelized_tx (tdm_api, tdm_api->tx_buf, tdm_api->tx_buf_len);

	if (tdm_api->cfg.loop) {
		memcpy(tdm_api->tx_buf, tdm_api->rx_buf, tdm_api->rx_buf_len);
	}

	return 0;
}

int wanpipe_tdm_api_span_rx (wanpipe_tdm_api_dev_t *tdm_api, netskb_t *skb)
{
	sdla_t *card;
	wan_smp_flag_t flag;
	wp_api_hdr_t *rx_hdr;

	flag=0;
	
	if (!tdm_api->chan || !wan_test_bit(0,&tdm_api->init) || !tdm_api->card){
		return -ENODEV;
	}

	if (!wan_test_bit(0,&tdm_api->used)) {
		return -ENODEV;
	}

	card=tdm_api->card;

	if (wan_skb_len(skb) <= sizeof(wp_api_hdr_t)) {
		WP_AFT_CHAN_ERROR_STATS(tdm_api->cfg.stats,rx_errors);
		if (WAN_NET_RATELIMIT()) {
			DEBUG_ERROR("%s: Internal Error RX Data has no rx header!\n",
					__FUNCTION__);	
		}
		return -EINVAL;
	}

	wptdm_os_lock_irq(&card->wandev.lock,&flag);
	if (wan_skb_queue_len(&tdm_api->wp_rx_list) >= (int)tdm_api->cfg.rx_queue_sz) {
		wptdm_os_unlock_irq(&card->wandev.lock,&flag);
		wp_wakeup_rx_tdmapi(tdm_api);
		WP_AFT_CHAN_ERROR_STATS(tdm_api->cfg.stats,rx_dropped);
		DEBUG_TEST("%s: Error RX Buffer Overrun!\n",__FUNCTION__);
		return -EBUSY;
	}

	rx_hdr=(wp_api_hdr_t*)wan_skb_data(skb);

	wanpipe_tdm_timestamp_hdr(rx_hdr);

	wan_skb_queue_tail(&tdm_api->wp_rx_list,skb);
	tdm_api->cfg.stats.rx_packets++;
	wptdm_os_unlock_irq(&card->wandev.lock,&flag);


	/* Buffer multiplier is by default 1 so we can use it to check if there
	   is something in the rx_list */
	if (wan_skb_queue_len(&tdm_api->wp_rx_list) >= tdm_api->buffer_multiplier) {
		wp_wakeup_rx_tdmapi(tdm_api);
	}

	return 0;
}


static wanpipe_tdm_api_dev_t *wp_tdmapi_search(sdla_t *card, int fe_chan)
{
	wanpipe_tdm_api_dev_t	*tdm_api;

	if(fe_chan < 0 || fe_chan >= MAX_TDM_API_CHANNELS){
		DEBUG_ERROR("%s(): TDM API Error: Invalid Channel Number=%i!\n",
			__FUNCTION__, fe_chan);
		return NULL;
	}

	tdm_api = card->wp_tdmapi_hash[fe_chan];
	if(tdm_api == NULL){
		DEBUG_TEST("%s(): TDM API Warning: No TDM API registered for Channel Number=%i!\n",
			__FUNCTION__, fe_chan);
		return NULL;
	}


	if (!wan_test_bit(0,&tdm_api->init)) {
		return NULL;
	}

	return tdm_api;
}

static void wp_tdmapi_report_rbsbits(void* card_id, int channel, unsigned char rbsbits)
{
	netskb_t                *skb;
	wanpipe_tdm_api_dev_t   *tdm_api = NULL;
	sdla_t                  *card = (sdla_t*)card_id;
	wp_api_event_t  *p_tdmapi_event = NULL;
	wan_smp_flag_t irq_flags;

	irq_flags=0;

	tdm_api = wp_tdmapi_search(card, channel);
	if (tdm_api == NULL){
		return;
	}

	if (tdm_api->rbs_rx_bits[channel] == rbsbits) {
		return;
	}

	DEBUG_TEST("%s: Received RBS Bits Event at TDM_API channel=%d rbs=0x%02X!\n",
					card->devname,
					channel,rbsbits);

	tdm_api->rbs_rx_bits[channel] = rbsbits;

	wptdm_os_lock_irq(&card->wandev.lock,&irq_flags);

	skb=wan_skb_dequeue(&tdm_api->wp_event_free_list);
	if (skb == NULL) {
			wptdm_os_unlock_irq(&card->wandev.lock,&irq_flags);
			WP_AFT_CHAN_ERROR_STATS(tdm_api->cfg.stats,rx_events_dropped);
			return;
	}

	p_tdmapi_event = (wp_api_event_t*)wan_skb_put(skb,sizeof(wp_api_event_t));

	memset(p_tdmapi_event, 0, sizeof(wp_api_event_t));



	p_tdmapi_event->wp_api_event_type = WP_API_EVENT_RBS;
	p_tdmapi_event->wp_api_event_channel = (u8)channel;
	p_tdmapi_event->wp_api_event_span = wp_tdmapi_get_span(card);

	p_tdmapi_event->wp_api_event_rbs_bits	= (u8)rbsbits;

	wan_skb_push_to_ctrl_event(skb);
	
	if (!wan_test_bit(0,&tdm_api->used)) {
			wan_skb_init(skb,sizeof(wp_api_hdr_t));
			wan_skb_trim(skb,0);
			wan_skb_queue_tail(&tdm_api->wp_event_free_list,skb);
			wptdm_os_unlock_irq(&card->wandev.lock,&irq_flags);
			return;
	}

	wanpipe_tdm_api_handle_event(tdm_api,skb);

	wptdm_os_unlock_irq(&card->wandev.lock,&irq_flags);


	return;

}


static void wp_tdmapi_alarms(void* card_id, wan_event_t *event)
{
	netskb_t		*skb;
	wanpipe_tdm_api_dev_t	*tdm_api = NULL;
	sdla_t			*card = (sdla_t*)card_id;
	wp_api_event_t	*p_tdmapi_event = NULL;
	wan_smp_flag_t irq_flags;

	irq_flags=0;


	DEBUG_TEST("%s: Received ALARM Event at TDM_API channel=%d alarm=0x%X!\n",
			card->devname,
			event->channel,
			event->alarms);

	tdm_api = wp_tdmapi_search(card, event->channel);
	if (tdm_api == NULL){
		return;
	}

	wptdm_os_lock_irq(&card->wandev.lock,&irq_flags);
	skb=wan_skb_dequeue(&tdm_api->wp_event_free_list);
	if (skb == NULL) {
		wptdm_os_unlock_irq(&card->wandev.lock,&irq_flags);
		WP_AFT_CHAN_ERROR_STATS(tdm_api->cfg.stats,rx_events_dropped);
		return;
	}	

	p_tdmapi_event = (wp_api_event_t*)wan_skb_put(skb,sizeof(wp_api_event_t));

	memset(p_tdmapi_event, 0, sizeof(wp_api_event_t));

	p_tdmapi_event->wp_api_event_type = WP_API_EVENT_ALARM;
	p_tdmapi_event->wp_api_event_channel = event->channel;
	p_tdmapi_event->wp_api_event_span = wp_tdmapi_get_span(card);

	p_tdmapi_event->wp_api_event_alarm = event->alarms;			
	tdm_api->cfg.fe_alarms = event->alarms;

	wan_skb_push_to_ctrl_event(skb);

	if (!wan_test_bit(0,&tdm_api->used)) {
		wan_skb_init(skb,sizeof(wp_api_hdr_t));
		wan_skb_trim(skb,0);
		wan_skb_queue_tail(&tdm_api->wp_event_free_list,skb);
		wptdm_os_unlock_irq(&card->wandev.lock,&irq_flags);
		return;
	}

	wanpipe_tdm_api_handle_event(tdm_api,skb);

	wptdm_os_unlock_irq(&card->wandev.lock,&irq_flags);


	return;
}

static void wp_tdmapi_report_alarms(void* card_id, uint32_t te_alarm)
{
	wan_event_t event;
	u8 i;

	memset(&event,0,sizeof(event));

	event.alarms=te_alarm;

	for (i=0; i<NUM_OF_E1_CHANNELS; i++) {
		event.channel=i;
		wp_tdmapi_alarms(card_id, &event);
	}

	return;
}


static void wp_tdmapi_tone (void* card_id, wan_event_t *event)
{
	netskb_t		*skb = NULL;
	wanpipe_tdm_api_dev_t	*tdm_api = NULL;
	sdla_t			*card = (sdla_t*)card_id;
	wp_api_event_t	*p_tdmapi_event = NULL;
	wan_smp_flag_t irq_flags;
	int 			lock=1;

	irq_flags=0;

	switch(event->type) {
     	case WAN_EVENT_EC_DTMF:
		case WAN_EVENT_EC_FAX_1100:
		case WAN_EVENT_EC_FAX_2100:
		case WAN_EVENT_EC_FAX_2100_WSPR:
		case WAN_EVENT_RM_DTMF:
			break;
		default:
             DEBUG_ERROR("%s: %s() Error Invalid event type %X (%s)!\n",
				card->devname, __FUNCTION__, event->type, WAN_EVENT_TYPE_DECODE(event->type));       
			return;
	}

	if (event->type == WAN_EVENT_RM_DTMF){
		DEBUG_TDMAPI("%s: Received DTMF Event at TDM API (%d:%c)!\n",
			card->devname,
			event->channel,
			event->digit);
			lock=0;
			/* FIXME: Updated locking architecture */
	}else{
		DEBUG_TDMAPI("%s: Received Tone Event %s at TDM API (%d:%c:%s:%s)!\n",
			WAN_EVENT_TYPE_DECODE(event->type),
			card->devname,
			event->channel,
			event->digit,
			(event->tone_port == WAN_EC_CHANNEL_PORT_ROUT)?"ROUT":"SOUT",
			(event->tone_type == WAN_EC_TONE_PRESENT)?"PRESENT":"STOP");
	}

	tdm_api = wp_tdmapi_search(card, event->channel);
	if (tdm_api == NULL){
		return;
	}

	if (lock) wptdm_os_lock_irq(&card->wandev.lock,&irq_flags);
	skb=wan_skb_dequeue(&tdm_api->wp_event_free_list);
	if (skb == NULL) {
		if (lock) wptdm_os_unlock_irq(&card->wandev.lock,&irq_flags);
		WP_AFT_CHAN_ERROR_STATS(tdm_api->cfg.stats,rx_events_dropped);
		return;
	}
	
	p_tdmapi_event = (wp_api_event_t*)wan_skb_put(skb,sizeof(wp_api_event_t));

	memset(p_tdmapi_event,0,sizeof(wp_api_event_t));

    switch(event->type) {
     	case WAN_EVENT_EC_DTMF:
		case WAN_EVENT_RM_DTMF:
		  	 p_tdmapi_event->wp_api_event_type	= WP_API_EVENT_DTMF;
			 break;
		case WAN_EVENT_EC_FAX_1100:
		  	 p_tdmapi_event->wp_api_event_type	= WP_API_EVENT_FAX_1100;
			 break;
		case WAN_EVENT_EC_FAX_2100:
		  	 p_tdmapi_event->wp_api_event_type	= WP_API_EVENT_FAX_2100;
			 break;
		case WAN_EVENT_EC_FAX_2100_WSPR:
		  	 p_tdmapi_event->wp_api_event_type	= WP_API_EVENT_FAX_2100_WSPR;
			 break;
	}

	p_tdmapi_event->wp_api_event_dtmf_digit	= event->digit;
	p_tdmapi_event->wp_api_event_dtmf_type	= event->tone_type;
	p_tdmapi_event->wp_api_event_dtmf_port	= event->tone_port;

#ifdef WAN_TDMAPI_DTMF_TEST
#ifdef __LINUX__
#warning "Enabling DTMF TEST - Should only be done on debugging"
#endif
	{
		if (event->tone_type == WAN_EC_TONE_PRESENT) {
			u8 ntone=tdm_api->dtmf_check[event->channel].dtmf_tone_present;
			if (ntone != 0) {
				ntone = tdm_api->dtmf_check[event->channel].dtmf_tone_present + 1;
			
				if (ntone != event->digit) {
					DEBUG_ERROR("%s: Error: Digit mismatch Ch=%i Expecting %c  Received %c\n",
						tdm_api->name,event->channel, ntone, event->digit);
				}
			}
			tdm_api->dtmf_check[event->channel].dtmf_tone_present = event->digit;
		} else {
			u8 ntone=tdm_api->dtmf_check[event->channel].dtmf_tone_stop;

			if (ntone != 0) {
				ntone = tdm_api->dtmf_check[event->channel].dtmf_tone_stop + 1;
				
				if (ntone != event->digit) {
					DEBUG_ERROR("%s: Error: Digit mismatch Ch=%i Expecting %c  Received %c\n",
						tdm_api->name,event->channel, ntone, event->digit);
				}
			}
			tdm_api->dtmf_check[event->channel].dtmf_tone_stop = event->digit;
		}
	}
#endif

	p_tdmapi_event->channel = event->channel;
	p_tdmapi_event->span = wp_tdmapi_get_span(card);

#if 0
	rx_hdr->event_time_stamp = gettimeofday();
#endif

	wan_skb_push_to_ctrl_event(skb);

	if (!wan_test_bit(0,&tdm_api->used)) {
		wan_skb_init(skb,sizeof(wp_api_hdr_t));
		wan_skb_trim(skb,0);
		wan_skb_queue_tail(&tdm_api->wp_event_free_list,skb);
		if (lock)  wptdm_os_unlock_irq(&card->wandev.lock,&irq_flags);
		return;
	}

	tdm_api->cfg.stats.rx_events_tone++;
	wanpipe_tdm_api_handle_event(tdm_api,skb);

	if (lock) wptdm_os_unlock_irq(&card->wandev.lock,&irq_flags);


	return;
}


static void wp_tdmapi_hook (void* card_id, wan_event_t *event)
{
	netskb_t		*skb;
	wanpipe_tdm_api_dev_t	*tdm_api = NULL;
	sdla_t			*card = (sdla_t*)card_id;
	wp_api_event_t	*p_tdmapi_event = NULL;

	DEBUG_TDMAPI("%s: Received RM LC Event at TDM_API (%d:%s)!\n",
			card->devname,
			event->channel,
			(event->rxhook==WAN_EVENT_RXHOOK_OFF)?"OFF-HOOK":"ON-HOOK");

	tdm_api = wp_tdmapi_search(card, event->channel);
	if (tdm_api == NULL){
		DEBUG_ERROR("%s: Error: failed to find API device. Discarding RM LC Event at TDM_API (%d:%s)!\n",
			card->devname,
			event->channel,
			(event->rxhook==WAN_EVENT_RXHOOK_OFF)?"OFF-HOOK":"ON-HOOK");
		return;
	}

	skb=wan_skb_dequeue(&tdm_api->wp_event_free_list);
	if (skb == NULL) {
		WP_AFT_CHAN_ERROR_STATS(tdm_api->cfg.stats,rx_events_dropped);
		DEBUG_ERROR("%s: Error: no free buffers for API event. Discarding RM LC Event at TDM_API (%d:%s)!\n",
			card->devname,
			event->channel,
			(event->rxhook==WAN_EVENT_RXHOOK_OFF)?"OFF-HOOK":"ON-HOOK");
		return;
	}

	p_tdmapi_event = (wp_api_event_t*)wan_skb_put(skb,sizeof(wp_api_event_t));

	memset(p_tdmapi_event, 0, sizeof(wp_api_event_t));

	p_tdmapi_event->wp_api_event_type 		= WP_API_EVENT_RXHOOK;
	p_tdmapi_event->wp_api_event_channel	= event->channel;
	p_tdmapi_event->wp_api_event_span		= wp_tdmapi_get_span(card);

	switch(event->rxhook){
	case WAN_EVENT_RXHOOK_ON:
		p_tdmapi_event->wp_api_event_hook_state =
					WP_API_EVENT_RXHOOK_ON;
		break;
	case WAN_EVENT_RXHOOK_OFF:
		p_tdmapi_event->wp_api_event_hook_state =
					WP_API_EVENT_RXHOOK_OFF;
		break;
	case WAN_EVENT_RXHOOK_FLASH:
		p_tdmapi_event->wp_api_event_hook_state =
				WP_API_EVENT_RXHOOK_FLASH;
		break;
		}

#if 0
	rx_hdr->event_time_stamp = gettimeofday();
#endif

	wan_skb_push_to_ctrl_event(skb);

	if (!wan_test_bit(0,&tdm_api->used)) {
		wan_skb_init(skb,sizeof(wp_api_hdr_t));
		wan_skb_trim(skb,0);
		wan_skb_queue_tail(&tdm_api->wp_event_free_list,skb);
		return;
	}

	wanpipe_tdm_api_handle_event(tdm_api,skb);

	return;
}



static void __wp_tdmapi_linkstatus (void* card_id, wan_event_t *event, int lock)
{
#define wptdm_queue_lock_irq(card,flags)    if (lock) wptdm_os_lock_irq(card,flags)
#define wptdm_queue_unlock_irq(card,flags)  if (lock) wptdm_os_unlock_irq(card,flags)

	netskb_t		*skb;
	wanpipe_tdm_api_dev_t	*tdm_api = NULL;
	sdla_t			*card = (sdla_t*)card_id;
	wp_api_event_t	*p_tdmapi_event = NULL;
	wan_smp_flag_t 	flags;
	u8 i;

	flags=0;

	DEBUG_TEST("%s: Received Link Status Event at TDM_API (%d:%s)!\n",
			card->devname,
			event->channel,
			(event->link_status==WAN_EVENT_LINK_STATUS_DISCONNECTED)?"Disconnected":"Connected");

	tdm_api = wp_tdmapi_search(card, event->channel);
	if (tdm_api == NULL){
		return;
	}

	if (tdm_api->state == event->link_status) {
		return;
	}

   	wptdm_queue_lock_irq(&card->wandev.lock,&flags);
	tdm_api->state = event->link_status;

	if (event->link_status  == WAN_EVENT_LINK_STATUS_CONNECTED) {
		wp_tdm_api_start(tdm_api);		
	} else {
		wp_tdm_api_stop(tdm_api);
	}
	wanpipe_tdm_api_kick(tdm_api);

	for (i=0;i<NUM_OF_E1_CHANNELS;i++) {

		if (!wan_test_bit(i,&tdm_api->active_ch)) {
			continue;
		}

		skb=wan_skb_dequeue(&tdm_api->wp_event_free_list);
		if (skb == NULL) {
			WP_AFT_CHAN_ERROR_STATS(tdm_api->cfg.stats,rx_events_dropped);
			break;
		}
	
		p_tdmapi_event = (wp_api_event_t*)wan_skb_put(skb,sizeof(wp_api_event_t));
	
		memset(p_tdmapi_event, 0, sizeof(wp_api_event_t));
		p_tdmapi_event->wp_api_event_type		= WP_API_EVENT_LINK_STATUS;
		p_tdmapi_event->wp_api_event_channel	= i;
		p_tdmapi_event->wp_api_event_span		= wp_tdmapi_get_span(card);
	
		if (event->link_status  == WAN_EVENT_LINK_STATUS_CONNECTED) {
			p_tdmapi_event->wp_api_event_link_status =
						WP_API_EVENT_LINK_STATUS_CONNECTED;
		} else {
		
			p_tdmapi_event->wp_api_event_link_status =
						WP_API_EVENT_LINK_STATUS_DISCONNECTED;
		}
	
		wan_skb_push_to_ctrl_event(skb);
	
		if (!wan_test_bit(0,&tdm_api->used)) {
			wan_skb_init(skb,sizeof(wp_api_hdr_t));
			wan_skb_trim(skb,0);
			wan_skb_queue_tail(&tdm_api->wp_event_free_list,skb);
			continue;
		}
	
		wanpipe_tdm_api_handle_event(tdm_api,skb);
	}

	wptdm_queue_unlock_irq(&card->wandev.lock,&flags);

#if 0
	rx_hdr->event_time_stamp = gettimeofday();
#endif

	return;
#undef wptdm_queue_lock_irq
#undef wptdm_queue_unlock_irq
}

static void wp_tdmapi_linkstatus (void* card_id, wan_event_t *event)
{
	u8 i;
	sdla_t *card = (sdla_t*)card_id;
	
	if (card->wandev.config_id == WANCONFIG_AFT_ANALOG) {
		__wp_tdmapi_linkstatus(card_id, event, 0);
		return;
	}

	for (i=0; i<NUM_OF_E1_CHANNELS; i++) {
		event->channel=i;
		__wp_tdmapi_linkstatus(card_id, event, 1);
	}

	return;	

}

/*FXO
	Some analog line use reverse polarity to indicate line hangup 
*/


static void wp_tdmapi_polarityreverse (void* card_id, wan_event_t *event)
{
	netskb_t		*skb;
	wanpipe_tdm_api_dev_t	*tdm_api = NULL;
	sdla_t			*card = (sdla_t*)card_id;
	wp_api_event_t	*p_tdmapi_event = NULL;

	DEBUG_TDMAPI("%s: Received Polarity Reverse Event at TDM_API (%d: %s)!\n",
			card->devname,
			event->channel,(event->polarity_reverse==WAN_EVENT_POLARITY_REV_POSITIVE_NEGATIVE)?"+ve  to -ve":"-ve to +ve");

	tdm_api = wp_tdmapi_search(card, event->channel);
	if (tdm_api == NULL){
		return;
	}

	skb=wan_skb_dequeue(&tdm_api->wp_event_free_list);
	if (skb == NULL) {
		WP_AFT_CHAN_ERROR_STATS(tdm_api->cfg.stats,rx_events_dropped);
		return;
	}

	p_tdmapi_event = (wp_api_event_t*)wan_skb_put(skb,sizeof(wp_api_event_t));

	memset(p_tdmapi_event, 0, sizeof(wp_api_event_t));
	p_tdmapi_event->wp_api_event_type			= WP_API_EVENT_POLARITY_REVERSE;
	p_tdmapi_event->wp_api_event_channel		= event->channel;
	p_tdmapi_event->wp_api_event_span			= wp_tdmapi_get_span(card);
	
	switch(event->polarity_reverse){
	case WAN_EVENT_POLARITY_REV_POSITIVE_NEGATIVE:
		p_tdmapi_event->wp_api_event_polarity_reverse =
					WP_API_EVENT_POL_REV_POS_TO_NEG;
		wp_tdm_api_start(tdm_api);		

		break;
	case WAN_EVENT_POLARITY_REV_NEGATIVE_POSITIVE	:
		p_tdmapi_event->wp_api_event_polarity_reverse =
					WP_API_EVENT__POL_REV_NEG_TO_POS;
		wp_tdm_api_stop(tdm_api);
		break;
	}

	wan_skb_push_to_ctrl_event(skb);

	if (!wan_test_bit(0,&tdm_api->used)) {
		wan_skb_init(skb,sizeof(wp_api_hdr_t));
		wan_skb_trim(skb,0);
		wan_skb_queue_tail(&tdm_api->wp_event_free_list,skb);
		return;
	}

#if 0
	rx_hdr->event_time_stamp = gettimeofday();
#endif

	wanpipe_tdm_api_handle_event(tdm_api,skb);

	return;
}

/*
FXS:
A ring trip event signals that the terminal equipment has gone
off-hook during the ringing state.
At this point the application should stop the ring because the
call was answered.
*/
static void wp_tdmapi_ringtrip (void* card_id, wan_event_t *event)
{
	netskb_t		*skb;
	wanpipe_tdm_api_dev_t	*tdm_api = NULL;
	sdla_t			*card = (sdla_t*)card_id;
	wp_api_event_t	*p_tdmapi_event = NULL;

	DEBUG_TDMAPI("%s: Received RM RING TRIP Event at TDM_API (%d:%s)!\n",
			card->devname,
			event->channel,
			WAN_EVENT_RING_TRIP_DECODE(event->ring_mode));


	tdm_api = wp_tdmapi_search(card, event->channel);
	if (tdm_api == NULL){
		return;
	}

	skb=wan_skb_dequeue(&tdm_api->wp_event_free_list);
	if (skb == NULL) {
		WP_AFT_CHAN_ERROR_STATS(tdm_api->cfg.stats,rx_events_dropped);
		return;
	}

	p_tdmapi_event = (wp_api_event_t*)wan_skb_put(skb,sizeof(wp_api_event_t));

	memset(p_tdmapi_event, 0, sizeof(wp_api_event_t));
	
	p_tdmapi_event->wp_api_event_type = WP_API_EVENT_RING_TRIP_DETECT;
	p_tdmapi_event->wp_api_event_channel	= event->channel;
	p_tdmapi_event->wp_api_event_span = wp_tdmapi_get_span(card);

	
	if (event->ring_mode == WAN_EVENT_RING_TRIP_STOP){
		p_tdmapi_event->wp_api_event_ring_state =
				WP_API_EVENT_RING_TRIP_STOP;
	}else if (event->ring_mode == WAN_EVENT_RING_TRIP_PRESENT){
		p_tdmapi_event->wp_api_event_ring_state =
				WP_API_EVENT_RING_TRIP_PRESENT;
	}

#if 0
	rx_hdr->event_time_stamp = gettimeofday();
#endif

	wan_skb_push_to_ctrl_event(skb);

	if (!wan_test_bit(0,&tdm_api->used)) {
		wan_skb_init(skb,sizeof(wp_api_hdr_t));
		wan_skb_trim(skb,0);
		wan_skb_queue_tail(&tdm_api->wp_event_free_list,skb);
		return;
	}

	wanpipe_tdm_api_handle_event(tdm_api,skb);

	return;
}

/*
FXO:
received a ring Start/Stop from the other side.
*/
static void wp_tdmapi_ringdetect (void* card_id, wan_event_t *event)
{
	netskb_t		*skb;
	wanpipe_tdm_api_dev_t	*tdm_api = NULL;
	sdla_t			*card = (sdla_t*)card_id;
	wp_api_event_t	*p_tdmapi_event = NULL;

	DEBUG_TDMAPI("%s: Received RM RING DETECT Event at TDM_API (%d:%s)!\n",
			card->devname,
			event->channel,
			WAN_EVENT_RING_DECODE(event->ring_mode));


	tdm_api = wp_tdmapi_search(card, event->channel);
	if (tdm_api == NULL){
		return;
	}

	skb=wan_skb_dequeue(&tdm_api->wp_event_free_list);
	if (skb == NULL) {
		WP_AFT_CHAN_ERROR_STATS(tdm_api->cfg.stats,rx_events_dropped);
		return;
	}	

	p_tdmapi_event = (wp_api_event_t*)wan_skb_put(skb,sizeof(wp_api_event_t));

	memset(p_tdmapi_event, 0, sizeof(wp_api_event_t));

	p_tdmapi_event->wp_api_event_type = WP_API_EVENT_RING_DETECT;
	p_tdmapi_event->wp_api_event_channel	= event->channel;
	p_tdmapi_event->wp_api_event_span = wp_tdmapi_get_span(card);

	switch(event->ring_mode){
	case WAN_EVENT_RING_PRESENT:
		p_tdmapi_event->wp_api_event_ring_state =
					WP_API_EVENT_RING_PRESENT;
		break;
	case WAN_EVENT_RING_STOP:
		p_tdmapi_event->wp_api_event_ring_state =
					WP_API_EVENT_RING_STOP;
		break;
	}

#if 0
	rx_hdr->event_time_stamp = gettimeofday();
#endif

 	wan_skb_push_to_ctrl_event(skb);

	if (!wan_test_bit(0,&tdm_api->used)) {
		wan_skb_init(skb,sizeof(wp_api_hdr_t));
		wan_skb_trim(skb,0);
		wan_skb_queue_tail(&tdm_api->wp_event_free_list,skb);
		return;
	}

	wanpipe_tdm_api_handle_event(tdm_api,skb);

	return;
}


static int __store_tdm_api_pointer_in_card(sdla_t *card, wanpipe_tdm_api_dev_t *tdm_api, int fe_chan)
{

	if(fe_chan >= MAX_TDM_API_CHANNELS){
		DEBUG_ERROR("%s(): TDM API Error (TE1): Invalid Channel Number=%i (Span=%d)!\n",
				__FUNCTION__, fe_chan, tdm_api->tdm_span);
		return 1;
	}

	if(card->wp_tdmapi_hash[fe_chan] != NULL){
		DEBUG_ERROR("%s(): TDM API Error: device SPAN=%i CHAN=%i already in use!\n",
			__FUNCTION__, tdm_api->tdm_span, fe_chan);
		return 1;
	}

	card->wp_tdmapi_hash[fe_chan] = tdm_api;

#if 0
	DEBUG_TDMAPI("%s(): card->wp_tdmapi_hash[%d]==0x%p\n", __FUNCTION__, fe_chan, tdm_api);
#endif
	return 0;
}

static int store_tdm_api_pointer_in_card(sdla_t *card, wanpipe_tdm_api_dev_t *tdm_api)
{	
	int fe_chan=0, err;
#if 0
	DEBUG_TDMAPI("%s(): tdm_api ptr==0x%p, tdm_api->active_ch: 0x%X\n", __FUNCTION__, tdm_api, tdm_api->active_ch);
#endif

	/* Go through all timeslots belonging to this interface. VERY important for SPAN based API! */
	for(fe_chan = 0; fe_chan < NUM_OF_E1_CHANNELS; fe_chan++){

		/*	Note: 'tdm_api->active_ch' bitmap is 1-based so 'fe_chan' will start from 1.
			It is important because 'channo' in 'read_rbsbits(sdla_fe_t* fe, int channo, int mode)'
			is 1-based. */
		if (!wan_test_bit(fe_chan, &tdm_api->active_ch)) {
			/* timeslot does NOT belong to this interface */
			continue;
		}

		err=__store_tdm_api_pointer_in_card(card, tdm_api, fe_chan);
		if (err) {
			return err;
		}
	}

	return 0;
}


static int __remove_tdm_api_pointer_from_card(wanpipe_tdm_api_dev_t *tdm_api,int fe_chan)
{
	sdla_t	*card = NULL;

	WAN_ASSERT(tdm_api == NULL);
	card = (sdla_t*)tdm_api->card;

#if 1
	DEBUG_TDMAPI("%s(): tdm_api ptr==0x%p, fe_chan: %d\n", __FUNCTION__, tdm_api, fe_chan);
#endif

	if(card == NULL){
		DEBUG_ERROR("%s(): TDM API Error: Invalid 'card' pointer!\n", __FUNCTION__);
		return 1;
	}


	if(fe_chan >= MAX_TDM_API_CHANNELS){
		DEBUG_ERROR("%s(): TDM API Error (TE1): Invalid Channel Number=%i (Span=%d)!\n",
			__FUNCTION__, fe_chan, tdm_api->tdm_span);
		return 1;
	}

	if(card->wp_tdmapi_hash[fe_chan] == NULL){
		DEBUG_WARNING("%s: TDM API Warning: device SPAN=%i CHAN=%i was NOT initialized!\n",
			__FUNCTION__, tdm_api->tdm_span, fe_chan);
	}

	card->wp_tdmapi_hash[fe_chan] = NULL;

	return 0;
}


static int remove_tdm_api_pointer_from_card(wanpipe_tdm_api_dev_t *tdm_api)
{
	sdla_t	*card = NULL;
	int fe_chan=0, err;

	WAN_ASSERT(tdm_api == NULL);

	card = (sdla_t*)tdm_api->card;
	
	/* Go through all timeslots belonging to this interface. VERY important for SPAN based API! */
	for(fe_chan = 0; fe_chan < NUM_OF_E1_CHANNELS; fe_chan++){

		/*	Note: 'tdm_api->active_ch' bitmap is 1-based so 'fe_chan' will start from 1.
			It is important because 'channo' in 'read_rbsbits(sdla_fe_t* fe, int channo, int mode)'
			is 1-based. */
		if (!wan_test_bit(fe_chan, &tdm_api->active_ch)) {
			/* timeslot does NOT belong to this interface */
			continue;
		}

		err=__remove_tdm_api_pointer_from_card(tdm_api, fe_chan);
		if (err) {
			return err;
		}

	}

	return 0;
}

/* When a Port is unloaded tell ctrldev that the Port is 'disconnected' */
static int wp_tdmapi_unreg_notify_ctrl(wanpipe_tdm_api_dev_t *tdm_api)
{
	sdla_t *card = tdm_api->card;

	if (!card) {
		return 0;
	}

	if (wan_test_bit(0,&tdmapi_ctrl.used)) {

		wan_smp_flag_t flags;
		netskb_t *skb;
		u8 i;
		flags=0;

		for (i=0;i<NUM_OF_E1_CHANNELS;i++) {

			if (!wan_test_bit(i,&tdm_api->active_ch)) {
				continue;
			}

			wptdm_os_lock_irq(&card->wandev.lock,&flags);
			skb=wan_skb_dequeue(&tdm_api->wp_event_free_list);
			wptdm_os_unlock_irq(&card->wandev.lock,&flags);
	
			if (skb) {
				wp_api_event_t *	p_tdmapi_event;
				p_tdmapi_event = (wp_api_event_t*)wan_skb_put(skb,sizeof(wp_api_event_t));
		
				memset(p_tdmapi_event, 0, sizeof(wp_api_event_t));
				p_tdmapi_event->wp_api_event_type		= WP_API_EVENT_LINK_STATUS;
				p_tdmapi_event->wp_api_event_channel	= i;
				p_tdmapi_event->wp_api_event_span		= wp_tdmapi_get_span(card);
				p_tdmapi_event->wp_api_event_link_status =	WP_API_EVENT_LINK_STATUS_DISCONNECTED;
	
				wan_skb_push_to_ctrl_event(skb);
	
				wan_skb_init(skb,sizeof(wp_api_hdr_t));
				wan_skb_trim(skb,0);
	
				wptdm_os_lock_irq(&card->wandev.lock,&flags);
				wan_skb_queue_tail(&tdm_api->wp_event_free_list,skb);
				wptdm_os_unlock_irq(&card->wandev.lock,&flags);
			}
		}
	}

	return 0;
}


/* The api task funciton is not used right now. we are leaving
   the infrastructure here for future use */

#if defined(__LINUX__)
# if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,20))
static void wp_api_task_func (void * tdmapi_ptr)
# else
static void wp_api_task_func (struct work_struct *work)
# endif
#elif defined(__WINDOWS__)
static void wp_api_task_func (IN PKDPC Dpc, IN PVOID tdmapi_ptr, IN PVOID SystemArgument1, IN PVOID SystemArgument2)
#else
static void wp_api_task_func (void * tdmapi_ptr, int arg)
#endif
{
#if defined(__LINUX__)
# if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,20))
	wanpipe_tdm_api_dev_t 		*tdm_api = (wanpipe_tdm_api_dev_t *)container_of(work, wanpipe_tdm_api_dev_t, wp_api_task);
# else
	wanpipe_tdm_api_dev_t 		*tdm_api = (wanpipe_tdm_api_dev_t *)tdmapi_ptr;
# endif
#else
	wanpipe_tdm_api_dev_t 		*tdm_api = (wanpipe_tdm_api_dev_t *)tdmapi_ptr;
#endif
	sdla_t *card = tdm_api->card;

	if (!card) {
		return;
	}

	if (!wan_test_bit(0,&tdm_api->init)) {
		return;
	}

	return;
}

#if 0
static int wp_tdm_api_mtp1_rx_data (void *priv_ptr, u8 *rx_data, int rx_len)
{
	wanpipe_tdm_api_dev_t *tdm_api = (wanpipe_tdm_api_dev_t*)priv_ptr;
	sdla_t *card;
	netskb_t *skb;
	u8 *data;
	
	if (!tdm_api->chan || !wan_test_bit(0,&tdm_api->init) || !tdm_api->card){
		return -ENODEV;
	}

	if (!wan_test_bit(0,&tdm_api->used)) {
		return -ENODEV;
	}

	card=tdm_api->card;
	
	skb=wan_skb_alloc(sizeof(wp_api_hdr_t)+rx_len);
	if (!skb) {
		return -ENOBUFS;	
	}

	wan_skb_put(skb,sizeof(wp_api_hdr_t));
	data=wan_skb_put(skb,rx_len);
	memcpy(data,rx_data,rx_len);

	wptdm_os_lock_irq(&card->wandev.lock,&flag);
	if (wan_skb_queue_len(&tdm_api->wp_rx_list) >= (int)tdm_api->cfg.rx_queue_sz) {
		wptdm_os_unlock_irq(&card->wandev.lock,&flag);
		wp_wakeup_rx_tdmapi(tdm_api);
		WP_AFT_CHAN_ERROR_STATS(tdm_api->cfg.stats,rx_dropped);
		if (WAN_NET_RATELIMIT()) {
			DEBUG_TEST("%s: Error RX Buffer Overrun!\n",
					   __FUNCTION__);	
		}
		return -EBUSY;
	}

	wan_skb_queue_tail(&tdm_api->wp_rx_list,skb);
	tdm_api->cfg.stats.rx_packets++;
	wptdm_os_unlock_irq(&card->wandev.lock,&flag);

	wp_wakeup_rx_tdmapi(tdm_api);

	return 0;
}

static int wp_tdm_api_mtp1_rx_reject (void *priv_prt, char *reason)
{
	/* Do nothing */
	return 0;
	
}

static int wp_tdm_api_mtp1_rx_suerm (void *priv_ptr)
{
	/* Pass up suerm event */
	wanpipe_tdm_api_dev_t *tdm_api = (wanpipe_tdm_api_dev_t*)priv_ptr;
	wp_wakeup_rx_tdmapi(tdm_api);
	return 0;
}

static int wp_tdm_api_tmp1_wakup(void *priv_ptr)
{
	wanpipe_tdm_api_dev_t *tdm_api = (wanpipe_tdm_api_dev_t*)priv_ptr;
	wp_wakeup_rx_tdmapi(tdm_api);
	return 0;
}    
#endif

#else

int wanpipe_tdm_api_reg(wanpipe_tdm_api_dev_t *tdm_api)
{
	return -EINVAL;
}

int wanpipe_tdm_api_unreg(wanpipe_tdm_api_dev_t *tdm_api)
{
	return -EINVAL;
}

int wanpipe_tdm_api_kick(wanpipe_tdm_api_dev_t *tdm_api)
{
	return -EINVAL;
}

int wanpipe_tdm_api_rx_tx (wanpipe_tdm_api_dev_t *tdm_api, u8 *rx_data, u8 *tx_data, int len)
{
	return -EINVAL;
}


int wanpipe_tdm_api_span_rx (wanpipe_tdm_api_dev_t *tdm_api, netskb_t *skb)
{
	return -EINVAL;
}

#endif /* #if defined(BUILD_TDMV_API) */


