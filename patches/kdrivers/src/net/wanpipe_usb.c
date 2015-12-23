/*****************************************************************************
* wanpipe_usb.c 
* 		
* 		WANPIPE(tm) USB Interface Support
*
* Authors: 	Alex Feldman <alex@sangoma.com>
*
* Copyright:	(c) 2008 Sangoma Technologies Inc.
*
*		This program is free software; you can redistribute it and/or
*		modify it under the terms of the GNU General Public License
*		as published by the Free Software Foundation; either version
*		2 of the License, or (at your option) any later version.
*
******************************************************************************
* Apr 01,2008	Alex Feldman	Initial version.
*****************************************************************************/


/*****************************************************************************
*                             INCLUDE FILES
*****************************************************************************/


# include "wanpipe_includes.h"
# include "wanpipe_defines.h"
# include "wanpipe.h"
# include "wanproc.h"
# include "wanpipe_abstr.h"
# include "if_wanpipe_common.h"    /* Socket Driver common area */
# include "if_wanpipe.h"
# include "sdlapci.h"
# include "wanpipe_iface.h"
# include "wanpipe_usb.h"
# include "sdla_usb_remora.h"

#if defined(CONFIG_PRODUCT_WANPIPE_USB)

/*****************************************************************************
*                         DEFINES/MACROS
*****************************************************************************/
#if 1
# define WP_USB_FUNC_DEBUG()
#else
# define WP_USB_FUNC_DEBUG()  DEBUG_EVENT("%s:%d\n",__FUNCTION__,__LINE__)
#endif

WAN_DECLARE_NETDEV_OPS(wan_netdev_ops)

/* Private critical flags */
enum {
	CARD_DOWN = 0x01,
	CARD_DISCONNECT 
};

enum {
	WP_USB_TASK_UNKNOWN = 0x00,
	WP_USB_TASK_RXENABLE,
	WP_USB_TASK_FE_POLL
};

/*****************************************************************************
*                        GLOBAL STRUCTTURE/TYPEDEF 
*****************************************************************************/
typedef struct wp_usb_softc {

	wanpipe_common_t 	common;
	sdla_t			*card;
	char 			if_name[WAN_IFNAME_SZ+1];

	wan_usb_conf_if_t 	cfg;

	u32			time_slot_map;

	int			channelized_cfg;
	int			tdmv_chan;
	u8			regs_mirror[5];

	wan_taskq_t		port_task;

	int			mtu, mru;
	unsigned long		start_time;
	unsigned long		current_time;

	wan_skb_queue_t 	wp_tx_pending_list;
	wan_skb_queue_t 	wp_tx_complete_list;

	wan_skb_queue_t 	wp_rx_free_list;
	wan_skb_queue_t 	wp_rx_complete_list;

	unsigned long		interface_down;
	unsigned long		up;

	unsigned char		*rxdata;
	unsigned char		*txdata;

	unsigned char 		udp_pkt_data[sizeof(wan_udp_pkt_t)+10];
	atomic_t 		udp_pkt_len;

	wp_usb_op_stats_t	opstats;

	wan_hwec_if_conf_t	hwec;

	struct wp_usb_softc	*next;

} wp_usb_softc_t;

/*****************************************************************************
*                         GLOBAL VARIABLES
*****************************************************************************/
/* Function interface between WANPIPE layer and kernel */
extern wan_iface_t wan_iface;

extern sdla_t	*card_list;

/*****************************************************************************
*                        FUNCTION PROTOTYPES
*****************************************************************************/
static int	wp_usb_chip_config(sdla_t *card);

static int	wp_usb_add_device(char *devname, void*);
static int	wp_usb_delete_device(char *devname);

static void	wp_usb_disable_comm (sdla_t *card);
static void	wp_usb_enable_timer (void* card_id);

static int	wp_usb_devel_ioctl(sdla_t *card, struct ifreq *ifr);
static int 	wp_usb_new_if (wan_device_t* wandev, netdevice_t* dev, wanif_conf_t* conf);
static int 	wp_usb_del_if(wan_device_t *wandev, netdevice_t *dev);

/* Network device interface */
#if defined(__LINUX__)
static int 	wp_usb_if_init   (netdevice_t* dev);
#endif
static int 	wp_usb_if_open   (netdevice_t* dev);
static int 	wp_usb_if_close  (netdevice_t* dev);
static int 	wp_usb_if_do_ioctl(netdevice_t*, struct ifreq*, wan_ioctl_cmd_t);
static void	wp_usb_if_tx_timeout (netdevice_t *dev);
static int	wp_usb_process_udp(sdla_t*, netdevice_t*, wp_usb_softc_t*);

#if defined(__LINUX__)
static int 	wp_usb_if_send (netskb_t* skb, netdevice_t* dev);
static struct net_device_stats* wp_usb_if_stats (netdevice_t* dev);
#else
static int	wp_usb_if_send(netdevice_t*, netskb_t*, struct sockaddr*,struct rtentry*);
#endif 

static int wp_usb_set_chan_state(sdla_t* card, netdevice_t* dev, int state);
static void wp_usb_port_set_state (sdla_t *card, int state);

#if defined(__LINUX__)
static void 	wp_usb_bh (unsigned long);
#else
static void 	wp_usb_bh (void*,int);
#endif

#if defined(__LINUX__)
# if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,20))     
static void wp_usb_task (void * card_ptr);
# else
static void wp_usb_task (struct work_struct *work);	
# endif
#else
static void wp_usb__task (void * card_ptr, int arg);
#endif  
static void wp_usb_isr(void *arg);


/*****************************************************************************
*                       FUNCTION DEFINITIONS
*****************************************************************************/
int wp_usb_init(sdla_t *card, wandev_conf_t *conf)
{

	/* Verify configuration ID */
	if (card->wandev.config_id != WANCONFIG_USB_ANALOG) {
		DEBUG_EVENT( "%s: invalid configuration ID %u!\n",
				  card->devname, card->wandev.config_id);
		return -EINVAL;
	}

	memset(card->u.usb.dev_to_ch_map,0,sizeof(card->u.usb.dev_to_ch_map));
	memcpy(&card->fe.fe_cfg, &conf->fe_cfg, sizeof(sdla_fe_cfg_t));
	wp_usb_remora_iface_init(&card->fe, &card->wandev.fe_iface);
	memcpy(&card->tdmv_conf,&conf->tdmv_conf,sizeof(wan_tdmv_conf_t));
	memcpy(&card->hwec_conf,&conf->hwec_conf,sizeof(wan_hwec_conf_t));

	card->fe.name		= card->devname;
	card->fe.card		= card;
	card->fe.write_fe_reg	= card->hw_iface.fe_write;
	card->fe.read_fe_reg	= card->hw_iface.fe_read;
	card->fe.__read_fe_reg	= NULL;	//card->hw_iface.__fe_read;

	card->wandev.fe_enable_timer	= wp_usb_enable_timer;
	card->wandev.ec_enable_timer	= NULL;		// wp_usb_enable_ec_timer;
	card->wandev.te_link_state	= NULL;		// callback_front_end_state;

        card->wandev.new_if             = &wp_usb_new_if;
        card->wandev.del_if             = &wp_usb_del_if;
        card->disable_comm              = wp_usb_disable_comm;

	card->u.usb.num_of_time_slots	= 0x02;

	wp_usb_port_set_state(card,WAN_CONNECTING);
	WAN_TASKQ_INIT((&card->u.usb.port_task),0,wp_usb_task,card);

	wan_clear_bit(CARD_DOWN,&card->wandev.critical);

	DEBUG_EVENT("%s: Configuring Device   :%s  Addr=%d\n",
			card->devname,card->devname,conf->pci_bus_no);

	if (wp_usb_chip_config(card)){
		return -EINVAL;
	}
	
	wp_usb_port_set_state(card,WAN_CONNECTED);
	return 0;
}

static int wp_usb_chip_config(sdla_t *card)
{
	int	err = 0;

	DEBUG_EVENT("%s: Global Front End Configuraton!\n",card->devname);
	err = -EINVAL;
	if (card->wandev.fe_iface.config){
		err = card->wandev.fe_iface.config(&card->fe);
	}
	if (err){
		DEBUG_EVENT("%s: Failed Front End configuration!\n",
					card->devname);
		return -EINVAL;
	}

	/* Run rest of initialization not from lock */
	err = -EINVAL;
	if (card->wandev.fe_iface.post_init){
		err=card->wandev.fe_iface.post_init(&card->fe);
	}
	if (err){
		DEBUG_EVENT("%s: Failed Post-Init Front End configuration!\n",
					card->devname);
		card->wandev.fe_iface.unconfig(&card->fe);
		return -EINVAL;
	}
		
	DEBUG_EVENT("%s: Set USB softirq handler ...\n", card->devname);
	err = -EINVAL;
	if (card->hw_iface.set_intrhand){
		err = card->hw_iface.set_intrhand(card->hw, wp_usb_isr, (void*)card, 0);
	}
	if (err){
		DEBUG_EVENT("%s: Failed to set softirq!\n", card->devname);
		card->wandev.fe_iface.unconfig(&card->fe);
		return -EINVAL;
	}
	DEBUG_EVENT("%s: USB Remora config done!\n",card->devname);
	return 0;
}

static int
wp_usb_new_if_private (wan_device_t* wandev, netdevice_t* dev, wanif_conf_t* conf, int channelized, int silent)
{
	sdla_t		*card = wandev->priv;
	wp_usb_softc_t	*chan;

	WP_USB_FUNC_DEBUG();

	chan = wan_kmalloc(sizeof(wp_usb_softc_t));
	if(chan == NULL){
		WAN_MEM_ASSERT(card->devname);
		return -ENOMEM;
	}
	
	memset(chan, 0, sizeof(wp_usb_softc_t));
	strncpy(chan->if_name, wan_netif_name(dev), WAN_IFNAME_SZ);
	chan->card		= card;
	chan->common.card	= card;

	WAN_IFQ_INIT(&chan->wp_tx_pending_list,0);
	WAN_IFQ_INIT(&chan->wp_tx_complete_list,0);
	WAN_IFQ_INIT(&chan->wp_rx_free_list,0);
	WAN_IFQ_INIT(&chan->wp_rx_complete_list,0);

	/* Initialize the socket binding information
	 * These hooks are used by the API sockets to
	 * bind into the network interface */
	WAN_TASKLET_INIT((&chan->common.bh_task),0,wp_usb_bh,chan);
	chan->common.dev = dev;


	if (strcmp(conf->usedby, "TDM_VOICE") == 0){

#ifdef CONFIG_PRODUCT_WANPIPE_TDM_VOICE
		int	channel;
		int err;

		chan->common.usedby = TDM_VOICE;
		WAN_TDMV_CALL(check_mtu, (card, conf->active_ch, &chan->mtu), err);
		if (err){
			DEBUG_ERROR("Error: TMDV mtu check failed!");
			return -EINVAL;
		}

		WAN_TDMV_CALL(reg, 
				(card, 
				&conf->tdmv, 
				conf->active_ch, 
				conf->hwec.enable, 
				chan->common.dev), channel);

		if (channel < 0){
			DEBUG_ERROR("%s: Error: Failed to register TDMV channel!\n",
					chan->if_name);

			return -EINVAL;
		}
		chan->tdmv_chan=channel;
		card->u.usb.dev_to_ch_map[channel] = chan;
		wan_set_bit(channel,&card->u.usb.tdm_logic_ch_map);
		DEBUG_EVENT( "%s:%s: Running in TDM Voice Zaptel Mode (channel=%d:%08X).\n",
				card->devname,chan->if_name,channel, card->u.usb.tdm_logic_ch_map);
		card->hw_iface.usb_rxtx_data_init(card->hw, channel, &chan->rxdata, &chan->txdata);

		memcpy(&chan->hwec, &conf->hwec, sizeof(wan_hwec_if_conf_t));
		if (conf->hwec.enable){
			card->wandev.ec_enable_map = 0x03;
		}

#else
		DEBUG_ERROR("%s: Error: TDMV_VOICE Zaptel Option not compiled into the driver!\n",
					card->devname);
		return -EINVAL;
#endif

	}else if (strcmp(conf->usedby, "API") == 0) {

		chan->common.usedby = API;
		DEBUG_EVENT( "%s:%s: Running in API mode\n",
			wandev->name,chan->if_name);
		wan_reg_api(chan, dev, card->devname);
	}else{
		wan_netif_set_priv(dev, NULL);
		return -EINVAL;
	}

	if (channelized) { 
		chan->channelized_cfg=1;
		if (wan_netif_priv(dev)) { 
			wp_usb_softc_t *cptr;
			for (cptr=wan_netif_priv(dev);cptr->next!=NULL;cptr=cptr->next);
			cptr->next=chan;
			chan->next=NULL;
		} else {
			wan_netif_set_priv(dev, chan);
		}
	} else {
		chan->channelized_cfg=0;
		wan_netif_set_priv(dev, chan);
	}
	chan->time_slot_map=conf->active_ch;

#if defined(__LINUX__)
	WAN_NETDEV_OPS_BIND(dev,wan_netdev_ops);
	WAN_NETDEV_OPS_INIT(dev,wan_netdev_ops,&wp_usb_if_init);
	WAN_NETDEV_OPS_OPEN(dev,wan_netdev_ops,&wp_usb_if_open);
	WAN_NETDEV_OPS_STOP(dev,wan_netdev_ops,&wp_usb_if_close);
	WAN_NETDEV_OPS_XMIT(dev,wan_netdev_ops,&wp_usb_if_send);
	WAN_NETDEV_OPS_STATS(dev,wan_netdev_ops,&wp_usb_if_stats);
	WAN_NETDEV_OPS_TIMEOUT(dev,wan_netdev_ops,&wp_usb_if_tx_timeout);
	WAN_NETDEV_OPS_IOCTL(dev,wan_netdev_ops,&wp_usb_if_do_ioctl);
# if defined(CONFIG_PRODUCT_WANPIPE_GENERIC)
	wp_usb_if_init(dev);
# endif
#else
	if (chan->common.usedby != TDM_VOICE && 
	    chan->common.usedby != TDM_VOICE_API){
		chan->common.is_netdev = 1;
	}
	chan->common.iface.open      = &wp_usb_if_open;
        chan->common.iface.close     = &wp_usb_if_close;
        chan->common.iface.output    = &wp_usb_if_send;
        chan->common.iface.ioctl     = &wp_usb_if_do_ioctl;
        chan->common.iface.tx_timeout= &wp_usb_if_tx_timeout;
	if (wan_iface.attach){
		if (!ifunit(wan_netif_name(dev))){
			wan_iface.attach(dev, NULL, chan->common.is_netdev);
		}
	}else{
		DEBUG_EVENT("%s: Failed to attach interface %s!\n",
				card->devname, wan_netif_name(dev));
		wan_netif_set_priv(dev, NULL);
		return -EINVAL;
	}
	wan_netif_set_mtu(dev, chan->mtu);
#endif
	chan->mru = chan->mtu;

	wan_set_bit(0,&chan->up);

	return 0;
}

#ifdef CONFIG_PRODUCT_WANPIPE_TDM_VOICE
extern int wp_usb_rm_tdmv_init(wan_tdmv_iface_t *iface);

static int
wp_usb_if_tdmv_init(wan_device_t* wandev, netdevice_t* dev, wanif_conf_t* conf)
{
	sdla_t	*card = wandev->priv;
	int	err = 0;

	WP_USB_FUNC_DEBUG();
	if (card->tdmv_conf.span_no){

		int i=0;
		int if_cnt=0;
		u32 active_ch=conf->active_ch;

		switch(card->wandev.config_id){
		case WANCONFIG_USB_ANALOG:
			err = wp_usb_rm_tdmv_init(&card->tdmv_iface);
			break;
		}
		if (err){
			DEBUG_ERROR("%s: Error: Failed to initialize tdmv functions!\n",
					card->devname);
			return -EINVAL;
		}
		WAN_TDMV_CALL(create, (card, &card->tdmv_conf), err);
		if (err){
			DEBUG_ERROR("%s: Error: Failed to create tdmv span!\n",
					card->devname);
			return err;
		}

		if (card->wandev.fe_iface.active_map){
			conf->active_ch = card->wandev.fe_iface.active_map(&card->fe, 0);
			active_ch=conf->active_ch;
		}

		for (i=0;i<card->u.usb.num_of_time_slots;i++){
		
			if (wan_test_bit(i,&active_ch)){
				conf->active_ch=0;
				wan_set_bit(i,&conf->active_ch);
				err = wp_usb_new_if_private(wandev,dev,conf,1,if_cnt);
				if (err){
					break;
				}
				if_cnt++;
			}
		}
		WAN_TDMV_CALL(software_init, (&card->wan_tdmv), err);
		WAN_TDMV_CALL(state, (card, WAN_CONNECTED), err);
		wan_set_bit(WP_USB_TASK_RXENABLE,&card->u.usb.port_task_cmd);
		WAN_TASKQ_SCHEDULE(&card->u.usb.port_task);
	}

	return 0;
}
#endif


static int
wp_usb_if_api_init(wan_device_t* wandev, netdevice_t* dev, wanif_conf_t* conf)
{
	int	err;


	//card->wandev.event_callback.dtmf	= wan_aft_api_dtmf; 
	//card->wandev.event_callback.hook	= wan_aft_api_hook; 
	//card->wandev.event_callback.ringtrip	= wan_aft_api_ringtrip; 
	//card->wandev.event_callback.ringdetect	= wan_aft_api_ringdetect; 

	err = wp_usb_new_if_private(wandev,dev,conf,0,0);

	return err;
}



static int
wp_usb_new_if(wan_device_t* wandev, netdevice_t* dev, wanif_conf_t* conf)
{
	sdla_t		*card = wandev->priv;
	int		err;

	err=0;
	WP_USB_FUNC_DEBUG();

        wan_netif_set_priv(dev, NULL); 
	if (strcmp(conf->usedby, "TDM_VOICE") == 0){

#ifdef CONFIG_PRODUCT_WANPIPE_TDM_VOICE
		err = wp_usb_if_tdmv_init(wandev, dev, conf);
#endif

	}else if (strcmp(conf->usedby, "API") == 0) {

		err = wp_usb_if_api_init(wandev, dev, conf);

	}else{
		DEBUG_ERROR( "%s: Error: Invalid IF operation mode %s\n",
				card->devname,conf->usedby);
		err=-EINVAL;
		goto wp_usb_new_if_error;
	}
	if (err){
		DEBUG_ERROR( "%s: Error: Failed initialize for %s operation mode!\n",
				card->devname,conf->usedby);
		err=-EINVAL;
		goto wp_usb_new_if_error;
	}

	/* Increment the number of network interfaces configured on this card. */
	wan_atomic_inc(&card->wandev.if_cnt);

	wp_usb_set_chan_state(card, dev, WAN_CONNECTING);

	DEBUG_EVENT( "%s: New interface is ready!\n", wan_netif_name(dev));

	return 0;

wp_usb_new_if_error:

	return -EINVAL;

}


static int wp_usb_del_if_private (wan_device_t* wandev, netdevice_t* dev)
{
	wp_usb_softc_t* 	chan = wan_netif_priv(dev);
	sdla_t*			card;
	wan_smp_flag_t		flags;
	int			err;

	if (!chan){
		DEBUG_ERROR("%s: Critical Error del_if_private() chan=NULL!\n",
			  wan_netif_name(dev));	
		return 0;
	}

	card = chan->card;
	if (!card){
		DEBUG_ERROR("%s: Critical Error del_if_private() chan=NULL!\n",
			  wan_netif_name(dev));
		return 0;
	}

	WP_USB_FUNC_DEBUG();
	wan_clear_bit(0,&chan->up);

	WAN_IFQ_DMA_PURGE(&chan->wp_rx_free_list);
	WAN_IFQ_DESTROY(&chan->wp_rx_free_list);

	WAN_IFQ_PURGE(&chan->wp_rx_complete_list);
	WAN_IFQ_DESTROY(&chan->wp_rx_complete_list);
	
	WAN_IFQ_PURGE(&chan->wp_tx_pending_list);
	WAN_IFQ_DESTROY(&chan->wp_tx_pending_list);
	
	WAN_IFQ_PURGE(&chan->wp_tx_complete_list);
	WAN_IFQ_DESTROY(&chan->wp_tx_complete_list);

	//WAN_TASKLET_KILL(&chan->common.bh_task);
	
	if (chan->common.usedby == API){
		wan_unreg_api(chan, card->devname);
	}

#ifdef CONFIG_PRODUCT_WANPIPE_TDM_VOICE
	if (chan->common.usedby == TDM_VOICE){
		wan_spin_lock_irq(&card->wandev.lock,&flags);
		WAN_TDMV_CALL(unreg, (card, chan->time_slot_map), err);
		wan_spin_unlock_irq(&card->wandev.lock,&flags);
	}
#endif

#if defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__)
	if (wan_iface.detach){
		wan_iface.detach(dev, chan->common.is_netdev);
	}
#endif

	wan_atomic_dec(&card->wandev.if_cnt);
	DEBUG_SUB_MEM(sizeof(private_area_t));
	return 0;
}

#if defined(CONFIG_PRODUCT_WANPIPE_TDM_VOICE)
static int wp_usb_del_if_tdmv (wan_device_t* wandev, netdevice_t* dev)
{
	wp_usb_softc_t	*chan = wan_netif_priv(dev);
	sdla_t 		*card = chan->card;
	wan_smp_flag_t	flags;
	int		err = 0;

	WAN_TDMV_CALL(running, (card), err);
	if (err){
		return -EBUSY;
	}

        wan_spin_lock_irq(&card->wandev.lock,&flags);
	if (card->wan_tdmv.sc) {
		WAN_TDMV_CALL(state, (card, WAN_DISCONNECTED), err);
	}
       	wan_spin_unlock_irq(&card->wandev.lock,&flags);

	if (chan->channelized_cfg) {

		sdla_t *card=chan->card;

		card->hw_iface.usb_rxdata_enable(card->hw, 0);
		while(chan){

			/* Disable fxo event from usbfxo device */
			card->hw_iface.usb_rxevent_enable(card->hw, chan->tdmv_chan, 0);
			chan->rxdata = NULL;
			chan->txdata = NULL;
			card->u.usb.dev_to_ch_map[chan->tdmv_chan] = NULL;
			err = wp_usb_del_if_private(wandev,dev);
			if (err) {
				return err;
			}	
			if (chan->next) {
				wan_netif_set_priv(dev, chan->next);
				wan_free(chan);
				chan = wan_netif_priv(dev);
			} else {
				/* Leave the last chan dev
				 * in dev->priv.  It will get
				 * deallocated normally */
				break;
			}
		}
	}else{

		err = wp_usb_del_if_private(wandev,dev);

	}

	if (card->wan_tdmv.sc) {
		if (card->tdmv_conf.span_no && card->wan_tdmv.sc){
			WAN_TDMV_CALL(remove, (card), err);
		}
	}

	return err;
}
#endif


static int wp_usb_del_if_api (wan_device_t* wandev, netdevice_t* dev)
{
//	wp_usb_softc_t	*chan = wan_netif_priv(dev);
	int		err;

	err = wp_usb_del_if_private(wandev,dev);

	return err;
}


static int
wp_usb_del_if (wan_device_t* wandev, netdevice_t* dev)
{
	wp_usb_softc_t	*chan=wan_netif_priv(dev);
	sdla_t		*card;	
	int		card_use_cnt=0;
	int		err=0;

	WP_USB_FUNC_DEBUG();

	if (!chan){
		DEBUG_ERROR("%s: Critical Error del_if() chan=NULL!\n",
			  wan_netif_name(dev));	
		return 0;
	}

	if (!(card=chan->card)){
		DEBUG_ERROR("%s: Critical Error del_if() chan=NULL!\n",
			  wan_netif_name(dev));	
		return 0;
	}

	wan_set_bit(CARD_DOWN,&card->wandev.critical);
	card->hw_iface.getcfg(card->hw, SDLA_HWCPU_USEDCNT, &card_use_cnt);

	if (chan->common.usedby == API){
		
		err = wp_usb_del_if_api (wandev, dev);

#if defined(CONFIG_PRODUCT_WANPIPE_TDM_VOICE)
	}else if (chan->common.usedby == TDM_VOICE){
		
		err = wp_usb_del_if_tdmv (wandev, dev);
#endif
	}

	return err;
}

static int wp_usb_add_device(char *devname, void *hw)
{
	sdla_t	*card = NULL;

	for (card = card_list; card; card = card->list) {
		if (strcmp(card->devname, devname) == 0){
			break;
		}	
	}
	if (card == NULL){
		DEBUG_ERROR("%s: INTERNAL ERROR: Failed to find the device in a list!\n",
					devname);
		return -EINVAL;
	}

	if (wp_usb_chip_config(card)){
		return -EINVAL;
	}

	/* Enable interface with hardware */
	card->hw = hw;
	wan_clear_bit(CARD_DISCONNECT,&card->wandev.critical);

	return 0;
}

static int wp_usb_delete_device(char *devname)
{
	sdla_t	*card = NULL;

	for (card = card_list; card; card = card->list) {
		if (strcmp(card->devname, devname) == 0){
			break;
		}	
	}
	if (card == NULL){
		DEBUG_ERROR("%s: INTERNAL ERROR: Failed to find the device in a list!\n",
					devname);
		return -EINVAL;
	}

	DEBUG_EVENT("%s: Disable USB device...\n", card->devname); 
	/* Disable all interface with hardware */
	wan_set_bit(CARD_DISCONNECT,&card->wandev.critical);

	DEBUG_EVENT("%s: Disable USB softirq handler ...\n", card->devname);
	if (card->hw_iface.restore_intrhand){
		card->hw_iface.restore_intrhand(card->hw, 0);
	}
	card->hw = NULL;
	return 0;
}

static void wp_usb_disable_comm (sdla_t *card)
{
//	wan_smp_flag_t smp_flags,smp_flags1;

	DEBUG_EVENT("%s: Disable USB softirq handler ...\n", card->devname);
	if (card->hw_iface.restore_intrhand){
		card->hw_iface.restore_intrhand(card->hw, 0);
	}

	wan_set_bit(CARD_DOWN,&card->wandev.critical);
   	/* Unconfiging, only on shutdown */
	if (card->wandev.fe_iface.unconfig){
		card->wandev.fe_iface.unconfig(&card->fe);
	}
	if (card->wandev.fe_iface.post_unconfig){
		card->wandev.fe_iface.post_unconfig(&card->fe);
	}
	return;
}


#if defined(__LINUX__)
static int wp_usb_if_init (netdevice_t* dev)
{
	wp_usb_softc_t	*chan = wan_netif_priv(dev);
	
	/* Initialize device driver entry points */
	WAN_NETDEV_OPS_OPEN(dev,wan_netdev_ops,&wp_usb_if_open);
	WAN_NETDEV_OPS_STOP(dev,wan_netdev_ops,&wp_usb_if_close);
	WAN_NETDEV_OPS_XMIT(dev,wan_netdev_ops,&wp_usb_if_send);
	WAN_NETDEV_OPS_STATS(dev,wan_netdev_ops,&wp_usb_if_stats);

	if (chan->common.usedby == TDM_VOICE || 
	    chan->common.usedby == TDM_VOICE_API){
		WAN_NETDEV_OPS_TIMEOUT(dev,wan_netdev_ops,NULL);
	}else{
		WAN_NETDEV_OPS_TIMEOUT(dev,wan_netdev_ops,&wp_usb_if_tx_timeout);
	}
	dev->watchdog_timeo	= 2*HZ;
		
	WAN_NETDEV_OPS_IOCTL(dev,wan_netdev_ops,&wp_usb_if_do_ioctl);
	dev->flags     |= IFF_POINTOPOINT;
	dev->flags     |= IFF_NOARP;
	dev->type	= ARPHRD_PPP;
	dev->mtu		= chan->mtu;
	dev->hard_header_len	= 0;

	/* Set transmit buffer queue length
	 * If too low packets will not be retransmitted
         * by stack.
	 */
        dev->tx_queue_len = 100;
	return 0;
}
#endif


static int wp_usb_if_open (netdevice_t* dev)
{
	wp_usb_softc_t	*chan = wan_netif_priv(dev);
	sdla_t		*card = chan->card;

#if defined(__LINUX__)
	/* Only one open per interface is allowed */
	if (open_dev_check(dev)){
		DEBUG_EVENT("%s: Open dev check failed!\n",
				wan_netif_name(dev));
		return -EBUSY;
	}
#endif

	if (wan_test_bit(CARD_DOWN,&card->wandev.critical)){
		DEBUG_EVENT("%s:%s: Card down: Failed to open interface!\n",
			card->devname,chan->if_name);	
		return -EINVAL;
	}

	/* Initialize the router start time.
	 * Used by wanpipemon debugger to indicate
	 * how long has the interface been up */
	wan_getcurrenttime(&chan->start_time, NULL);

#if !defined(WANPIPE_IFNET_QUEUE_POLICY_INIT_OFF)
	WAN_NETIF_START_QUEUE(dev);
#endif
	
	if (card->wandev.state == WAN_CONNECTED){
		wp_usb_set_chan_state(card, dev, WAN_CONNECTED);
       		WAN_NETIF_CARRIER_ON(dev);
	} else {
       		WAN_NETIF_CARRIER_OFF(dev);
	}

	/* Increment the module usage count */
	wanpipe_open(card);


	/* Wait for the front end interrupt 
	 * before enabling the card */
	return 0;
}

static int wp_usb_if_close (netdevice_t* dev)
{
	wp_usb_softc_t	*chan = wan_netif_priv(dev);
	sdla_t		*card = chan->card;

	WAN_NETIF_STOP_QUEUE(dev);

#if defined(LINUX_2_1)
	dev->start=0;
#endif

	wanpipe_close(card);
	
	return 0;
}

#if defined(__LINUX__)
static struct net_device_stats gstats;
static struct net_device_stats* wp_usb_if_stats (netdevice_t* dev)
{
	sdla_t		*card = NULL;
	wp_usb_softc_t	*chan = wan_netif_priv(dev);

	if ((chan = wan_netif_priv(dev)) == NULL){
		return &gstats;
	}

	card=chan->card;

	if (card) {
#if defined(CONFIG_PRODUCT_WANPIPE_TDM_VOICE)
      		if (card->wan_tdmv.sc &&
		    card->wandev.state == WAN_CONNECTED && 
		    chan->common.usedby == TDM_VOICE) {
			chan->common.if_stats.rx_packets = card->wandev.stats.rx_packets;
			chan->common.if_stats.tx_packets = card->wandev.stats.tx_packets;
		}
#endif
	}

	return &chan->common.if_stats;
}
#endif

static void wp_usb_if_tx_timeout (netdevice_t *dev)
{
	WP_USB_FUNC_DEBUG();

	return;
}

#if defined(__LINUX__)
static int wp_usb_if_send (netskb_t* skb, netdevice_t* dev)
#else
static int wp_usb_if_send(netdevice_t *dev, netskb_t *skb, struct sockaddr *dst,struct rtentry *rt)
#endif
{
	WP_USB_FUNC_DEBUG();

	return -EINVAL;
}

static int
wp_usb_if_do_ioctl(netdevice_t *dev, struct ifreq *ifr, wan_ioctl_cmd_t cmd)
{
	wp_usb_softc_t	*chan = NULL;
	wan_udp_pkt_t	*wan_udp_pkt;
	sdla_t		*card;
	int 		err = 0;

	WAN_ASSERT(dev == NULL);
	chan= (wp_usb_softc_t*)wan_netif_priv(dev);
	WAN_ASSERT(chan == NULL);
	if (!chan->card){
		DEBUG_EVENT("%s:%d: No Chan of card ptr\n",
				__FUNCTION__,__LINE__);
		return -ENODEV;
	}
	card=chan->card;

	if (wan_test_bit(CARD_DOWN,&card->wandev.critical)){
		DEBUG_EVENT("%s: Card down: Ignoring Ioctl call!\n",
			card->devname);	
		return -ENODEV;
	}

	switch(cmd){

	case SIOC_WAN_DEVEL_IOCTL:
		err = wp_usb_devel_ioctl(card, ifr);
		break;

	case SIOC_WANPIPE_PIPEMON:
			
		if (wan_atomic_read(&chan->udp_pkt_len) != 0){
			return -EBUSY;
		}

		wan_atomic_set(&chan->udp_pkt_len,MAX_LGTH_UDP_MGNT_PKT);

		wan_udp_pkt=(wan_udp_pkt_t*)chan->udp_pkt_data;
		if (WAN_COPY_FROM_USER(
				&wan_udp_pkt->wan_udp_hdr,
				ifr->ifr_data,
				sizeof(wan_udp_hdr_t))){
			wan_atomic_set(&chan->udp_pkt_len,0);
			return -EFAULT;
		}

		wp_usb_process_udp(card,dev,chan);
		/* This area will still be critical to other
		 * PIPEMON commands due to udp_pkt_len
		 * thus we can release the irq */
		if (wan_atomic_read(&chan->udp_pkt_len) > sizeof(wan_udp_pkt_t)){
			DEBUG_ERROR( "%s: Error: Pipemon buf too bit on the way up! %d\n",
					card->devname,wan_atomic_read(&chan->udp_pkt_len));
			wan_atomic_set(&chan->udp_pkt_len,0);
			return -EINVAL;
		}

		if (WAN_COPY_TO_USER(
				ifr->ifr_data,
				&wan_udp_pkt->wan_udp_hdr,
				sizeof(wan_udp_hdr_t))){
			wan_atomic_set(&chan->udp_pkt_len,0);
			return -EFAULT;
		}

		wan_atomic_set(&chan->udp_pkt_len,0);
		break;

	default:
		DEBUG_TEST("%s: Command %x not supported!\n",
			card->devname,cmd);
		err = -EOPNOTSUPP;
		break;
	}

	return err;
}

/*=============================================================================
 * process_udp_mgmt_pkt
 *
 * Process all "wanpipemon" debugger commands.  This function
 * performs all debugging tasks:
 *
 * 	Line Tracing
 * 	Line/Hardware Statistics
 * 	Protocol Statistics
 *
 * "wanpipemon" utility is a user-space program that
 * is used to debug the WANPIPE product.
 *
 */
static int
wp_usb_process_udp(sdla_t* card, netdevice_t* dev, wp_usb_softc_t *chan)
{
//	unsigned short buffer_length;
	wan_udp_pkt_t *wan_udp_pkt;
//	wan_trace_t *trace_info=NULL;
	int	err;

	wan_udp_pkt = (wan_udp_pkt_t *)chan->udp_pkt_data;

	if (wan_atomic_read(&chan->udp_pkt_len) == 0){
		return -ENODEV;
	}

	wan_udp_pkt = (wan_udp_pkt_t *)chan->udp_pkt_data;

   	{

		wan_udp_pkt->wan_udp_opp_flag = 0;

		switch(wan_udp_pkt->wan_udp_command) {

 /* Added during merge */
#if 0
		case READ_CODE_VERSION:
			wan_udp_pkt->wan_udp_return_code = 0;
			wan_udp_pkt->wan_udp_data[0]=card->u.usb.firm_ver;
			wan_udp_pkt->wan_udp_data_len=1;
			break;
	
		case AFT_LINK_STATUS:
			wan_udp_pkt->wan_udp_return_code = 0;
			if (card->wandev.state == WAN_CONNECTED){
				wan_udp_pkt->wan_udp_data[0]=1;
			}else{
				wan_udp_pkt->wan_udp_data[0]=0;
			}
			wan_udp_pkt->wan_udp_data_len=1;
			break;

		case ENABLE_TRACING:
	
			wan_udp_pkt->wan_udp_return_code = WAN_CMD_OK;
			wan_udp_pkt->wan_udp_data_len = 0;
			
			if (!wan_test_bit(0,&trace_info->tracing_enabled)){
						
				trace_info->trace_timeout = SYSTEM_TICKS;
					
				wan_trace_purge(trace_info);
					
				if (wan_udp_pkt->wan_udp_data[0] == 0){
					wan_clear_bit(1,&trace_info->tracing_enabled);
					DEBUG_UDP("%s: ADSL L3 trace enabled!\n",
						card->devname);
				}else if (wan_udp_pkt->wan_udp_data[0] == 1){
					wan_clear_bit(2,&trace_info->tracing_enabled);
					wan_set_bit(1,&trace_info->tracing_enabled);
					DEBUG_UDP("%s: ADSL L2 trace enabled!\n",
							card->devname);
				}else{
					wan_clear_bit(1,&trace_info->tracing_enabled);
					wan_set_bit(2,&trace_info->tracing_enabled);
					DEBUG_UDP("%s: ADSL L1 trace enabled!\n",
							card->devname);
				}
				wan_set_bit (0,&trace_info->tracing_enabled);

			}else{
				DEBUG_ERROR("%s: Error: ATM trace running!\n",
						card->devname);
				wan_udp_pkt->wan_udp_return_code = 2;
			}
					
			break;

		case DISABLE_TRACING:
			
			wan_udp_pkt->wan_udp_return_code = WAN_CMD_OK;
			
			if(wan_test_bit(0,&trace_info->tracing_enabled)) {
					
				wan_clear_bit(0,&trace_info->tracing_enabled);
				wan_clear_bit(1,&trace_info->tracing_enabled);
				wan_clear_bit(2,&trace_info->tracing_enabled);
				
				wan_trace_purge(trace_info);
				
				DEBUG_UDP("%s: Disabling AFT trace\n",
							card->devname);
					
			}else{
				/* set return code to line trace already 
				   disabled */
				wan_udp_pkt->wan_udp_return_code = 1;
			}

			break;

	        case GET_TRACE_INFO:

			if(wan_test_bit(0,&trace_info->tracing_enabled)){
				trace_info->trace_timeout = SYSTEM_TICKS;
			}else{
				DEBUG_ERROR("%s: Error ATM trace not enabled\n",
						card->devname);
				/* set return code */
				wan_udp_pkt->wan_udp_return_code = 1;
				break;
			}

			buffer_length = 0;
			wan_udp_pkt->wan_udp_atm_num_frames = 0;	
			wan_udp_pkt->wan_udp_atm_ismoredata = 0;
					
#if defined(__FreeBSD__) || defined(__OpenBSD__)
			while (wan_skb_queue_len(&trace_info->trace_queue)){
				WAN_IFQ_POLL(&trace_info->trace_queue, skb);
				if (skb == NULL){	
					DEBUG_EVENT("%s: No more trace packets in trace queue!\n",
								card->devname);
					break;
				}
				if ((WAN_MAX_DATA_SIZE - buffer_length) < skb->m_pkthdr.len){
					/* indicate there are more frames on board & exit */
					wan_udp_pkt->wan_udp_atm_ismoredata = 0x01;
					break;
				}

				m_copydata(skb, 
					   0, 
					   skb->m_pkthdr.len, 
					   &wan_udp_pkt->wan_udp_data[buffer_length]);
				buffer_length += skb->m_pkthdr.len;
				WAN_IFQ_DEQUEUE(&trace_info->trace_queue, skb);
				if (skb){
					wan_skb_free(skb);
				}
				wan_udp_pkt->wan_udp_atm_num_frames++;
			}
#elif defined(__LINUX__)
			while ((skb=skb_dequeue(&trace_info->trace_queue)) != NULL){

				if((MAX_TRACE_BUFFER - buffer_length) < wan_skb_len(skb)){
					/* indicate there are more frames on board & exit */
					wan_udp_pkt->wan_udp_atm_ismoredata = 0x01;
					if (buffer_length != 0){
						wan_skb_queue_head(&trace_info->trace_queue, skb);
					}else{
						/* If rx buffer length is greater than the
						 * whole udp buffer copy only the trace
						 * header and drop the trace packet */

						memcpy(&wan_udp_pkt->wan_udp_atm_data[buffer_length], 
							wan_skb_data(skb),
							sizeof(wan_trace_pkt_t));

						buffer_length = sizeof(wan_trace_pkt_t);
						wan_udp_pkt->wan_udp_atm_num_frames++;
						wan_skb_free(skb);	
					}
					break;
				}

				memcpy(&wan_udp_pkt->wan_udp_atm_data[buffer_length], 
				       wan_skb_data(skb),
				       wan_skb_len(skb));
		     
				buffer_length += wan_skb_len(skb);
				wan_skb_free(skb);
				wan_udp_pkt->wan_udp_atm_num_frames++;
			}
#endif                      
			/* set the data length and return code */
			wan_udp_pkt->wan_udp_data_len = buffer_length;
			wan_udp_pkt->wan_udp_return_code = WAN_CMD_OK;
			break;

#endif  /* Added during merge */

		case ROUTER_UP_TIME:
			wan_getcurrenttime(&chan->current_time, NULL);
			chan->current_time -= chan->start_time;
			*(unsigned long *)&wan_udp_pkt->wan_udp_data = 
					chan->current_time;	
			wan_udp_pkt->wan_udp_data_len = sizeof(unsigned long);
			wan_udp_pkt->wan_udp_return_code = 0;
			break;

		case READ_OPERATIONAL_STATS:
			wan_udp_pkt->wan_udp_return_code = 0;
			memcpy(wan_udp_pkt->wan_udp_data,&chan->opstats,sizeof(wp_usb_op_stats_t));
			wan_udp_pkt->wan_udp_data_len=sizeof(wp_usb_op_stats_t);
			break;

		case FLUSH_OPERATIONAL_STATS:
			wan_udp_pkt->wan_udp_return_code = 0;
			memset(&chan->opstats,0,sizeof(wp_usb_op_stats_t));
			wan_udp_pkt->wan_udp_data_len=0;
			break;

		case READ_COMMS_ERROR_STATS:
			wan_udp_pkt->wan_udp_data_len = 0;
			err = card->hw_iface.usb_err_stats(card->hw, wan_udp_pkt->wan_udp_data, sizeof(wp_usb_comm_err_stats_t));
			if (!err){
				wan_udp_pkt->wan_udp_data_len=sizeof(wp_usb_comm_err_stats_t);
				wan_udp_pkt->wan_udp_return_code = 0;
			}
			break;
	
		case FLUSH_COMMS_ERROR_STATS:
			wan_udp_pkt->wan_udp_return_code = 0;
			card->hw_iface.usb_flush_err_stats(card->hw);
			wan_udp_pkt->wan_udp_data_len=0;
			break;
	
		case WAN_GET_PROTOCOL:
			wan_udp_pkt->wan_udp_data[0] = (u8)card->wandev.config_id;
		    	wan_udp_pkt->wan_udp_return_code = CMD_OK;
		    	wan_udp_pkt->wan_udp_data_len = 1;
		    	break;

		case WAN_GET_PLATFORM:
		    	wan_udp_pkt->wan_udp_data[0] = WAN_PLATFORM_ID;
		    	wan_udp_pkt->wan_udp_return_code = CMD_OK;
		    	wan_udp_pkt->wan_udp_data_len = 1;
		    	break;

		case WAN_GET_MASTER_DEV_NAME:
			wan_udp_pkt->wan_udp_data_len = 0;
			wan_udp_pkt->wan_udp_return_code = 0xCD;
			break;
			
		case AFT_HWEC_STATUS:
			//*(u32*)&wan_udp_pkt->wan_udp_aft_num_frames = 
			//				card->wandev.fe_ec_map;
			*(u32*)&wan_udp_pkt->wan_udp_data[0] = card->wandev.fe_ec_map;
			wan_udp_pkt->wan_udp_data_len = sizeof(u32);
			wan_udp_pkt->wan_udp_return_code = CMD_OK;
			break;
	
		default:
			if ((wan_udp_pkt->wan_udp_command == WAN_GET_MEDIA_TYPE) ||
			    (wan_udp_pkt->wan_udp_command & 0xF0) == WAN_FE_UDP_CMD_START){
				/* FE udp calls */
				wan_smp_flag_t smp_flags,smp_flags1;
				
				card->hw_iface.hw_lock(card->hw,&smp_flags);
				wan_spin_lock_irq(&card->wandev.lock,&smp_flags1);
				card->wandev.fe_iface.process_udp(
						&card->fe, 
						&wan_udp_pkt->wan_udp_cmd,
						&wan_udp_pkt->wan_udp_data[0]);
				wan_spin_unlock_irq(&card->wandev.lock,&smp_flags1);
				card->hw_iface.hw_unlock(card->hw,&smp_flags);
				
				break;
			}
			wan_udp_pkt->wan_udp_data_len = 0;
			wan_udp_pkt->wan_udp_return_code = 0xCD;
	
			if (WAN_NET_RATELIMIT()){
				DEBUG_EVENT(
				"%s: Warning, Illegal UDP command attempted from network: %x\n",
				card->devname,wan_udp_pkt->wan_udp_command);
			}
			break;
		} /* end of switch */
     	} /* end of else */

     	/* Fill UDP TTL */
	wan_udp_pkt->wan_ip_ttl= card->wandev.ttl;

	wan_udp_pkt->wan_udp_request_reply = UDPMGMT_REPLY;
	return 0;

}

static int wp_usb_set_chan_state(sdla_t* card, netdevice_t* dev, int state)
{
	wp_usb_softc_t	*chan= (wp_usb_softc_t*)wan_netif_priv(dev);

	if (!chan){
                if (WAN_NET_RATELIMIT()){
                DEBUG_EVENT("%s: %s:%d No chan ptr!\n",
                               card->devname,__FUNCTION__,__LINE__);
                }	
		return -EINVAL;
	}

       	chan->common.state = state;

	return 0;
}

/*============================================================================
** port_set_state
*============================================================================*/
static void wp_usb_port_set_state (sdla_t *card, int state)
{
	struct wan_dev_le	*devle;
	netdevice_t		*dev;

        if (card->wandev.state != state){
                card->wandev.state = state;
		WAN_LIST_FOREACH(devle, &card->wandev.dev_head, dev_link){
			dev = WAN_DEVLE2DEV(devle);
			if (!dev) continue;
			wp_usb_set_chan_state(card, dev, state);
		}
        }
}


/*============================================================================
 * Enable timer interrupt
 */
static void wp_usb_enable_timer (void* card_id)
{
	sdla_t*	card = (sdla_t*)card_id;

	if (wan_test_bit(CARD_DOWN,&card->wandev.critical)){
		DEBUG_EVENT("%s: Card down: Ignoring enable_timer!\n",
			card->devname);	
		return;
	}

	DEBUG_EVENT("%s: Enable USB-FXO FE POll...\n", card->devname); 
	wan_set_bit(WP_USB_TASK_FE_POLL,&card->u.usb.port_task_cmd);
	WAN_TASKQ_SCHEDULE(&card->u.usb.port_task);
	return;
}


extern int gl_usb_rw_fast;

#if defined(__LINUX__)
static void wp_usb_bh (unsigned long data)
#else
static void wp_usb_bh (void *data, int pending)
#endif
{
//	wp_usb_softc_t	*gchan = (wp_usb_softc_t *)data;

	/* Add code here */

//	WAN_TASKLET_END((&gchan->common.bh_task));

//	WAN_TASKLET_SCHEDULE((&gchan->common.bh_task));	

	return;
}


static int wp_usb_task_rxenable (sdla_t *card)
{
	wp_usb_softc_t	*chan = NULL;
	int	i;

	for (i=0; i<card->u.usb.num_of_time_slots;i++){

		if (!wan_test_bit(i,&card->u.usb.tdm_logic_ch_map)){
			continue;
		}

		chan=(wp_usb_softc_t*)card->u.usb.dev_to_ch_map[i];
		if (!chan){
			DEBUG_ERROR("%s: Error: No Dev for Rx logical ch=%d\n",
					card->devname,i);
			continue;
		}
		if (!wan_test_bit(0,&chan->up)){
			continue;
		}
		card->hw_iface.usb_rxevent(card->hw, chan->tdmv_chan, &chan->regs_mirror[0], 1);
		card->hw_iface.usb_rxevent_enable(card->hw, chan->tdmv_chan, 1);
	}
	card->hw_iface.usb_rxdata_enable(card->hw, 1);
	return 0;
}


#if defined(__LINUX__)
# if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,20))     
static void wp_usb_task (void * data)
# else
static void wp_usb_task (struct work_struct *work)	
# endif
#else
static void wp_usb_task (void * data, int arg)
#endif
{
#if defined(__LINUX__)
# if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,20))   
        sdla_t		*card = (sdla_t*)container_of(work, sdla_t, u.usb.port_task);
# else
	sdla_t		*card = (sdla_t*)data;
# endif
#else
	sdla_t		*card = (sdla_t*)data;
#endif 
	WAN_ASSERT_VOID(card == NULL);

	if (wan_test_bit(CARD_DOWN,&card->wandev.critical)){
		return;
	}

	if (wan_test_bit(WP_USB_TASK_FE_POLL,&card->u.usb.port_task_cmd)){
		if (card->wandev.fe_iface.polling){
			DEBUG_EVENT("%s: Calling USB-FXO FE Polling...\n", card->devname); 
			card->wandev.fe_iface.polling(&card->fe);
		}
		wan_clear_bit(WP_USB_TASK_FE_POLL,&card->u.usb.port_task_cmd);
	}
	if (wan_test_bit(WP_USB_TASK_RXENABLE,&card->u.usb.port_task_cmd)){
		wp_usb_task_rxenable(card);
		wan_clear_bit(WP_USB_TASK_RXENABLE,&card->u.usb.port_task_cmd);
	}

	if (card->u.usb.port_task_cmd){
		WAN_TASKQ_SCHEDULE(&card->u.usb.port_task);
	}
	
	return;
}


#if 0
static int wp_usb_rx_tdmv(sdla_t *card, wp_usb_softc_t *chan)
{
	return 0;
}
#endif

static void wp_usb_isr(void *arg)
{
	sdla_t		*card = (sdla_t*)arg;
	wp_usb_softc_t	*chan = NULL;
	int	 i, ii, off, err;

	err=0;

	WAN_ASSERT_VOID(card == NULL);

	for (ii = 0; ii < WP_USB_FRAMESCALE; ii++){

		off = ii * 8;
		for (i=0; i<card->u.usb.num_of_time_slots;i++){

			if (!wan_test_bit(i,&card->u.usb.tdm_logic_ch_map)){
				continue;
			}
			chan=(wp_usb_softc_t*)card->u.usb.dev_to_ch_map[i];
			if (!chan){
				DEBUG_ERROR("%s: Error: No Dev for Rx logical ch=%d\n",
						card->devname,i);
				continue;
			}
			if (!wan_test_bit(0,&chan->up)){
				continue;
			}
	
			chan->opstats.isr_no++;
			if (ii == 0 && chan->channelized_cfg){
#ifdef CONFIG_PRODUCT_WANPIPE_TDM_VOICE
				card->hw_iface.usb_rxevent(card->hw, chan->tdmv_chan, &chan->regs_mirror[0], 0);
				WAN_TDMV_CALL(update_regs, (card, chan->tdmv_chan, &chan->regs_mirror[0]), err);
#endif
			}
			if (chan->rxdata == NULL || chan->txdata == NULL){
				DEBUG_ERROR("%s: %s:%d ASSERT ERROR TxDma=%p RxDma=%p\n",
							card->devname,__FUNCTION__,__LINE__,
							chan->rxdata,chan->txdata);
				return;
			}
#ifdef CONFIG_PRODUCT_WANPIPE_TDM_VOICE
			WAN_TDMV_CALL(rx_chan,
					(&card->wan_tdmv,chan->tdmv_chan,
					&chan->rxdata[off],
					&chan->txdata[off]),
					err);
#endif
		}
		gl_usb_rw_fast = 1;
#ifdef CONFIG_PRODUCT_WANPIPE_TDM_VOICE
		WAN_TDMV_CALL(rx_tx_span, (card), err);
#endif
		gl_usb_rw_fast = 0;

		if (card->wandev.state == WAN_CONNECTED){
			card->wandev.stats.rx_packets++;
			card->wandev.stats.tx_packets++;
		}
	}
	return;
}


/******************************************************************************
**                  SIOC_WAN_DEVEL_IOCTL interface 
******************************************************************************/
static int wp_usb_write_access(sdla_t *card, wan_cmd_api_t *api_cmd)
{

	if (card->type != SDLA_USB){
		DEBUG_EVENT("%s: Unsupported command for current device!\n",
					card->devname);
		return -EINVAL;
	}

	switch(api_cmd->cmd){
	case SIOC_WAN_USB_CPU_WRITE_REG:
		DEBUG_EVENT("%s: Write USB-CPU CMD: Reg:%02X <- %02X\n",
				card->devname, 
				(unsigned char)api_cmd->offset,
				(unsigned char)api_cmd->data[0]);
		api_cmd->ret = card->hw_iface.usb_cpu_write(	
						card->hw, 
						(u_int8_t)api_cmd->offset, 
						(u_int8_t)api_cmd->data[0]);
		break;

	case SIOC_WAN_USB_FE_WRITE_REG:
		DEBUG_EVENT("%s: Write USB-FXO CMD: Module:%d Reg:%02X <- %02X\n",
				card->devname, api_cmd->bar,
				(unsigned char)api_cmd->offset,
				(unsigned char)api_cmd->data[0]);

		api_cmd->ret = card->hw_iface.fe_write(
						card->hw, api_cmd->bar, 
						(u_int8_t)api_cmd->offset, 
						(u_int8_t)api_cmd->data[0]);
		break;

	default:
		DEBUG_EVENT("%s: Invalid USB-FXO Write command (0x%08X)\n", 
				card->devname, api_cmd->cmd);
		api_cmd->ret = -EINVAL;
		break;
	}
	return 0;
}

static int wp_usb_read_access(sdla_t *card, wan_cmd_api_t *api_cmd)
{
	int	err = 0;

	if (card->type != SDLA_USB){
		DEBUG_EVENT("%s: Unsupported command for current device!\n",
					card->devname);
		api_cmd->ret = -EINVAL;
		return 0;
	}
	switch(api_cmd->cmd){
	case SIOC_WAN_USB_CPU_READ_REG:
		api_cmd->data[0] = 0xFF;
		/* Add function here */
		err = card->hw_iface.usb_cpu_read(
						card->hw, 
						(u_int8_t)api_cmd->offset,
						(u_int8_t*)&api_cmd->data[0]);
		if (err){
			api_cmd->ret = -EBUSY;
			return 0;
		}
		DEBUG_EVENT("%s: Read USB-CPU CMD: Reg:%02x -> %02X\n",
				card->devname, 
				(unsigned char)api_cmd->offset,
				(unsigned char)api_cmd->data[0]);
		api_cmd->ret = 0;
		api_cmd->len = 1;
		break;

	case SIOC_WAN_USB_FE_READ_REG:
		/* Add function here */
		api_cmd->data[0] = card->hw_iface.fe_read(
						card->hw, 
						api_cmd->bar, 
						(u_int8_t)api_cmd->offset);

		DEBUG_EVENT("%s: Read USB-FXO CMD: Mod:%d Reg:%02X -> %02X\n",
				card->devname,  api_cmd->bar,
				(unsigned char)api_cmd->offset,
				(unsigned char)api_cmd->data[0]);
		api_cmd->ret = 0;
		api_cmd->len = 1;
		break;

	default:
		DEBUG_EVENT("%s: Invalid USB-FXO Read command (0x%08X)\n", 
				card->devname, api_cmd->cmd);
		api_cmd->ret = -EINVAL;
		break;
	}
	return 0;
}

static int wp_usb_devel_ioctl(sdla_t *card, struct ifreq *ifr)
{
	wan_cmd_api_t	*api_cmd;
	int 		err = -EINVAL;

	if (!ifr || !ifr->ifr_data){
		DEBUG_ERROR("%s: Error: No ifr or ifr_data\n",__FUNCTION__);
		return -EFAULT;
	}
	api_cmd = wan_malloc(sizeof(wan_cmd_api_t));
	if (api_cmd == NULL) {
		DEBUG_EVENT("Failed to allocate memory (%s)\n", __FUNCTION__);
		return -EFAULT;
	}
	memset(api_cmd, 0, sizeof(wan_cmd_api_t));	

	if (WAN_COPY_FROM_USER(api_cmd,ifr->ifr_data,sizeof(wan_cmd_api_t))){
		return -EFAULT;
	}

	switch(api_cmd->cmd){
	case SIOC_WAN_USB_CPU_WRITE_REG:
	case SIOC_WAN_USB_FE_WRITE_REG:
		err = wp_usb_write_access(card, api_cmd);
		break;

	case SIOC_WAN_USB_CPU_READ_REG:
	case SIOC_WAN_USB_FE_READ_REG:
		err = wp_usb_read_access(card, api_cmd);
		break;
	}
	if (WAN_COPY_TO_USER(ifr->ifr_data,api_cmd,sizeof(wan_cmd_api_t))){
		return -EFAULT;
	}
	if (api_cmd != NULL) {
		wan_free(api_cmd);
	}
	return err;
}


#endif   /* #if defined(CONFIG_PRODUCT_WANPIPE_USB)        */
