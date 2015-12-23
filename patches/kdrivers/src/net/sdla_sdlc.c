/*****************************************************************************
* wanpipe_sdlc.c SDLC driver module.
*
* Authors: 	Nenad Corbic <ncorbic@sangoma.com>
*
* Copyright:	(c) 2002 Sangoma Technologies Inc.
*
*		This program is free software; you can redistribute it and/or
*		modify it under the terms of the GNU General Public License
*		as published by the Free Software Foundation; either version
*		2 of the License, or (at your option) any later version.
* ============================================================================
* Jul 24 2002	Nenad Corbic    Initial Version
*****************************************************************************/

#include <linux/wanpipe_includes.h>
#include <linux/wanpipe_defines.h>
#include <linux/wanpipe.h>	/* WANPIPE common user API definitions */
#include <linux/wanpipe_wanrouter.h>	/* WAN router definitions */
#include <linux/sdlapci.h>
#include <linux/wanproc.h>	
#include <linux/if_wanpipe_common.h>    /* Socket Driver common area */
#include <linux/if_wanpipe.h>		
#include <linux/sdla_sdlc.h>		/* CHDLC firmware API definitions */

/****** Defines & Macros ****************************************************/

/* Private critical flags */
enum { 
	REG_CRIT = PRIV_CRIT 
};

/* reasons for enabling the timer interrupt on the adapter */
#define TMR_INT_ENABLED_UDP   	0x01
#define TMR_INT_ENABLED_UPDATE	0x02
 
#define	CHDLC_DFLT_DATA_LEN	1500		/* default MTU */
#define CHDLC_HDR_LEN		1

#define IFF_POINTTOPOINT 0x10

#define CHDLC_API 0x01

#define PORT(x)   (x == 0 ? "PRIMARY" : "SECONDARY" )
#define MAX_BH_BUFF	10

#define CRC_LENGTH 	2 
#define PPP_HEADER_LEN 	4
#define MAX_TRACE_LEN   25 
#define MAX_TRACE_ASCII_LEN MAX_TRACE_LEN*3+5
#define	MAX_CMD_RETRY	10		/* max number of firmware retries */

#ifndef ARPHRD_SDLC
#define ARPHRD_SDLC 514
#endif
/******Data Structures*****************************************************/

/* This structure is placed in the private data area of the device structure.
 * The card structure used to occupy the private area but now the following 
 * structure will incorporate the card structure along with CHDLC specific data
 */

typedef struct sdlc_private_area
{
	wanpipe_common_t common;
	sdla_t		*card;
	int 		TracingEnabled;		/* For enabling Tracing */
	unsigned long 	curr_trace_addr;	/* Used for Tracing */
	unsigned long 	start_trace_addr;
	unsigned long 	end_trace_addr;
	unsigned long 	base_addr_trace_buffer;
	unsigned long 	end_addr_trace_buffer;
	unsigned short 	number_trace_elements;
	unsigned  	available_buffer_space;
	unsigned long 	router_start_time;
	unsigned char 	route_status;
	unsigned char 	route_removed;
	unsigned long 	tick_counter;		/* For 5s timeout counter */
	unsigned long 	router_up_time;
        u32             IP_address;		/* IP addressing */
        u32             IP_netmask;
	unsigned char  mc;			/* Mulitcast support on/off */
	unsigned short udp_pkt_lgth;		/* udp packet processing */
	char udp_pkt_src;
	unsigned short timer_int_enabled;
	char update_comms_stats;		/* updating comms stats */

	unsigned long trace_state;

	/* Entry in proc fs per each interface */
	struct proc_dir_entry* 	dent;
	unsigned char ignore_modem;

	unsigned char udp_pkt_data[sizeof(wan_udp_pkt_t)+10];
	atomic_t udp_pkt_len;

	wp_sdlc_reg_t wp_sdlc_register;	

} sdlc_private_area_t;

/* Route Status options */
#define NO_ROUTE	0x00
#define ADD_ROUTE	0x01
#define ROUTE_ADDED	0x02
#define REMOVE_ROUTE	0x03


/****** Function Prototypes *************************************************/
/* WAN link driver entry points. These are called by the WAN router module. */
static int update (wan_device_t* wandev);
static int new_if (wan_device_t* wandev, netdevice_t* dev,
	wanif_conf_t* conf);
static int del_if (wan_device_t* wandev, netdevice_t* dev);

/* Network device interface */
static int if_init   (netdevice_t* dev);
static int if_open   (netdevice_t* dev);
static int if_close  (netdevice_t* dev);
static int if_send (struct sk_buff* skb, netdevice_t* dev);
static struct net_device_stats* if_stats (netdevice_t* dev);

/* CHDLC Firmware interface functions */
static int sdlc_configure 	(sdla_t* card, void* data);
static int sdlc_comm_enable 	(sdla_t* card);
static int sdlc_comm_disable 	(sdla_t* card);
static int sdlc_read_version 	(sdla_t* card, char* str);
static int sdlc_set_intr_mode 	(sdla_t* card, unsigned mode);
static int sdlc_send (sdla_t* card, void* data, unsigned len);
static int sdlc_list_stations_with_ifrms (sdla_t* card);

static int sdlc_read_comm_err_stats (sdla_t* card);
static int sdlc_read_op_stats (sdla_t* card);
static int config_sdlc (sdla_t *card);


/* Miscellaneous CHDLC Functions */
static int set_sdlc_config (sdla_t* card);
static int sdlc_error (sdla_t *card, int err, wan_mbox_t *mb);
static int process_sdlc_exception(sdla_t *card);
static int update_comms_stats(sdla_t* card,
        sdlc_private_area_t* sdlc_priv_area);
static void port_set_state (sdla_t *card, int);

/* Interrupt handlers */
static WAN_IRQ_RETVAL wp_sdlc_isr (sdla_t* card);
static void rx_intr (sdla_t* card);
static void tx_intr(sdla_t *card);
static void timer_intr(sdla_t *);

/* Miscellaneous functions */
static int intr_test( sdla_t* card);
static int process_udp_mgmt_pkt(sdla_t* card, netdevice_t* dev,  
				sdlc_private_area_t* sdlc_priv_area,int local_dev);

static int chdlc_get_config_info(void* priv, struct seq_file* m, int*);
static int chdlc_get_status_info(void* priv, struct seq_file* m, int*);
static int chdlc_set_dev_config(struct file*, const char*, unsigned long, void *);
static int chdlc_set_if_info(struct file*, const char*, unsigned long, void *);

void send_oob_msg(sdla_t *card, wan_mbox_t *mb);




/****** Public Functions ****************************************************/

/*============================================================================
 * Cisco HDLC protocol initialization routine.
 *
 * This routine is called by the main WANPIPE module during setup.  At this
 * point adapter is completely initialized and firmware is running.
 *  o read firmware version (to make sure it's alive)
 *  o configure adapter
 *  o initialize protocol-specific fields of the adapter data space.
 *
 * Return:	0	o.k.
 *		< 0	failure.
 */
int wp_sdlc_init (sdla_t* card, wandev_conf_t* conf)
{
	unsigned char port_num;
	int err;
	union{
	char str[80];
	} u;

	volatile wan_mbox_t* mb;
	wan_mbox_t* mb1;

	/* Verify configuration ID */
	if (conf->config_id != WANCONFIG_SDLC) {
		printk(KERN_INFO "%s: invalid configuration ID %u!\n",
				  card->devname, conf->config_id);
		return -EINVAL;
	}

	/* Find out which Port to use */
	if (conf->comm_port == WANOPT_PRI){
		card->u.sdlc.comm_port = conf->comm_port;
	}else{
		printk(KERN_INFO "%s: ERROR - Invalid Port Selected!\n",
                			card->wandev.name);
		return -EINVAL;
	}
	

	/* Initialize protocol-specific fields */
	/* Set a pointer to the actual mailbox in the allocated virtual 
	 * memory area */
	/* Alex Apr 8 2004 Sangoma ISA card */
	card->mbox_off = BASE_ADDR_OF_MB_STRUCTS;
	card->rxmb_off = 0xE230;

	mb = &card->wan_mbox;
	mb1 = &card->wan_mbox;

	if (!card->configured){
		/* Wait for the board to initialize. */
		udelay(500);
	}

	
        /* TE1 and 56K boards are not supported by this firmware */
        if (IS_TE1_MEDIA(&conf->fe_cfg) || IS_56K_MEDIA(&conf->fe_cfg)) {
		printk(KERN_INFO "%s: SDLC protocol doesn't support TE1 or 56K cards\n",
				card->devname);
		return -EINVAL;
	}

	if (sdlc_read_version(card, u.str))
		return -EIO;

	printk(KERN_INFO "%s: Running SDLC firmware v%s\n", 
			card->devname,u.str); 

	card->isr			= &wp_sdlc_isr;
	card->poll			= NULL;
	card->exec			= NULL;
	card->wandev.update		= &update;
 	card->wandev.new_if		= &new_if;
	card->wandev.del_if		= &del_if;
	card->wandev.udp_port   	= conf->udp_port;

	card->wandev.new_if_cnt = 0;

	// Proc fs functions
	card->wandev.get_config_info 	= &chdlc_get_config_info;
	card->wandev.get_status_info 	= &chdlc_get_status_info;
	card->wandev.set_dev_config    	= &chdlc_set_dev_config;
	card->wandev.set_if_info     	= &chdlc_set_if_info;

	card->wandev.ttl = conf->ttl;
	card->wandev.electrical_interface = conf->electrical_interface; 
	card->wandev.clocking = conf->clocking;

	port_num = card->u.sdlc.comm_port;

	/* Setup Port Bps */
	if(card->wandev.clocking) {
		
		if(conf->bps > MAX_PERMITTED_BAUD_RATE) {
			conf->bps = MAX_PERMITTED_BAUD_RATE;
			printk(KERN_INFO "%s: Baud too high!\n",
				card->wandev.name);
			printk(KERN_INFO "%s: Baud rate set to %u bps\n", 
				card->wandev.name, MAX_PERMITTED_BAUD_RATE);
		}
		     
		card->wandev.bps = conf->bps;
	}else{
        	card->wandev.bps = 0;
  	}

	/* For Primary Port 0 */
	card->wandev.mtu = (conf->mtu >= MIN_PERMITTED_I_FIELD_LENGTH) ?
			    wp_min(conf->mtu, MAX_PERMITTED_I_FIELD_LENGTH) :
			    MAX_PERMITTED_I_FIELD_LENGTH;


	/* Alex Apr 8 2004 Sangoma ISA card */
	card->flags_off = ADDR_INTERRUPT_REPORT_INTERFACE_BYTE;
    	card->intr_type_off = 
			card->flags_off +
			offsetof(INTERRUPT_INFORMATION_STRUCT, interrupt_type);
	card->intr_perm_off = 
			card->flags_off +
			offsetof(INTERRUPT_INFORMATION_STRUCT, interrupt_permission);

	/* This is for the ports link state */
	card->wandev.state = WAN_DUALPORT;
	card->u.sdlc.state = WAN_DISCONNECTED;

	if (card->u.sdlc.comm_enabled){
		sdlc_comm_disable(card);
		port_set_state(card, WAN_DISCONNECTED);
	}

	memcpy(&card->wandev.sdlc_cfg,&conf->u.sdlc,sizeof(wan_sdlc_conf_t));
	
	if (set_sdlc_config(card)) {
		printk(KERN_INFO "%s: SDLC Configuration Failed: Off=%i\n",
				card->devname,mb->wan_data_len);
		return -EINVAL;
	}

	if (!card->wandev.piggyback){
		err = intr_test(card);

		if(err || (card->timer_int_enabled < MAX_INTR_TEST_COUNTER)) { 
			printk(KERN_INFO "%s: Interrupt test failed (%i)\n",
					card->devname, card->timer_int_enabled);
			printk(KERN_INFO "%s: Please choose another interrupt\n",
					card->devname);
			return  -EIO;
		}
			
		printk(KERN_INFO "%s: Interrupt test passed (%i)\n", 
				card->devname, card->timer_int_enabled);
	}

	err=config_sdlc(card);
	if (err!=0){
		return err;
	}

	printk(KERN_INFO "\n");

	return 0;
}

/******* WAN Device Driver Entry Points *************************************/

/*============================================================================
 * Update device status & statistics
 * This procedure is called when updating the PROC file system and returns
 * various communications statistics. These statistics are accumulated from 3 
 * different locations:
 * 	1) The 'if_stats' recorded for the device.
 * 	2) Communication error statistics on the adapter.
 *      3) CHDLC operational statistics on the adapter.
 * The board level statistics are read during a timer interrupt. Note that we 
 * read the error and operational statistics during consecitive timer ticks so
 * as to minimize the time that we are inside the interrupt handler.
 *
 */
static int update (wan_device_t* wandev)
{
	sdla_t* card = wandev->priv;
 	netdevice_t* dev;
        volatile sdlc_private_area_t* sdlc_priv_area;
	unsigned long timeout;

	/* sanity checks */
	if((wandev == NULL) || (wandev->priv == NULL))
		return -EFAULT;
	
	if(wandev->state == WAN_UNCONFIGURED)
		return -ENODEV;

	/* more sanity checks */
        if(!card->flags_off)
                return -ENODEV;

	dev = WAN_DEVLE2DEV(WAN_LIST_FIRST(&card->wandev.dev_head));
	if(dev == NULL)
		return -ENODEV;

	if((sdlc_priv_area=dev->priv) == NULL)
		return -ENODEV;

       	if(sdlc_priv_area->update_comms_stats){
		return -EAGAIN;
	}
			
	/* we will need 2 timer interrupts to complete the */
	/* reading of the statistics */
	sdlc_priv_area->update_comms_stats = 2;
	card->hw_iface.set_bit(card->hw, card->intr_perm_off, INTERRUPT_ON_TIMER);
	sdlc_priv_area->timer_int_enabled = TMR_INT_ENABLED_UPDATE;
  
	/* wait a maximum of 1 second for the statistics to be updated */ 
        timeout = jiffies;
        for(;;) {
		if(sdlc_priv_area->update_comms_stats == 0)
			break;
                if ((jiffies - timeout) > (1 * HZ)){
    			sdlc_priv_area->update_comms_stats = 0;
 			sdlc_priv_area->timer_int_enabled &=
				~TMR_INT_ENABLED_UPDATE; 
 			return -EAGAIN;
		}
        }

	return 0;
}


/*============================================================================
 * Create new logical channel.
 * This routine is called by the router when ROUTER_IFNEW IOCTL is being
 * handled.
 * o parse media- and hardware-specific configuration
 * o make sure that a new channel can be created
 * o allocate resources, if necessary
 * o prepare network device structure for registaration.
 *
 * Return:	0	o.k.
 *		< 0	failure (channel will not be created)
 */
static int new_if (wan_device_t* wandev, netdevice_t* dev, wanif_conf_t* conf)
{

	sdla_t* card = wandev->priv;
	sdlc_private_area_t* sdlc_priv_area;
	int err = 0;
	
	if ((conf->name[0] == '\0') || (strlen(conf->name) > WAN_IFNAME_SZ)) {
		printk(KERN_INFO "%s: invalid interface name!\n",
			card->devname);
		return -EINVAL;
	}
		
	/* allocate and initialize private data */
	sdlc_priv_area = kmalloc(sizeof(sdlc_private_area_t), GFP_KERNEL);
	if(sdlc_priv_area == NULL){ 
		return -ENOMEM;
	}

	memset(sdlc_priv_area, 0, sizeof(sdlc_private_area_t));

	sdlc_priv_area->card = card; 

	/* initialize data */
	strcpy(card->u.sdlc.if_name, conf->name);

	sdlc_priv_area->common.sk = NULL;
	sdlc_priv_area->common.state = WAN_CONNECTING;
	sdlc_priv_area->common.dev = dev;
	
	if(card->wandev.new_if_cnt > 0) {
		err = -EEXIST;
		goto new_if_error;
	}

	card->wandev.new_if_cnt++;

	sdlc_priv_area->TracingEnabled = 0;

	printk(KERN_INFO "%s: Firmware running in SDLC Mode\n",
		wandev->name);
	
	if(strcmp(conf->usedby, "STACK") == 0) {
		printk(KERN_INFO "%s: Driver running in API STACK mode!\n",
			wandev->name);
		card->u.sdlc.usedby = STACK;
		sdlc_priv_area->common.usedby=STACK;
	}else{
		printk(KERN_INFO "%s: Driver running in API mode!\n",
			wandev->name);
		card->u.sdlc.usedby = API;
		sdlc_priv_area->common.usedby =API;
	}

	/* Get Multicast Information */
	sdlc_priv_area->mc = conf->mc;

	dev->init = &if_init;
	dev->priv = sdlc_priv_area;

#if 0
	/*
	 * Create interface file in proc fs.
	 */
	err = wanrouter_proc_add_interface(wandev, 
					   &sdlc_priv_area->dent, 
					   card->u.sdlc.if_name, 
					   dev);
	if (err){	
		printk(KERN_INFO
			"%s: can't create /proc/net/router/sdlc/%s entry!\n",
			card->devname, card->u.sdlc.if_name);
		goto new_if_error;
	}
#endif

	return 0;

new_if_error:

	if (sdlc_priv_area){
		kfree(sdlc_priv_area);
	}

	dev->priv=NULL;

	return err;

}



/*============================================================================
 * Delete logical channel.
 */
static int del_if (wan_device_t* wandev, netdevice_t* dev)
{
	sdlc_private_area_t *sdlc_priv_area = dev->priv;
	sdla_t *card = sdlc_priv_area->card;
	unsigned long smp_lock;
	
	/* Delete interface name from proc fs. */
	//wanrouter_proc_delete_interface(wandev, card->u.sdlc.if_name);

	wan_spin_lock_irq(&wandev->lock,&smp_lock);

	/* Bug Fix: Kernel 2.2.X
	 * We must manually remove the ioctl call binding
	 * since in some cases (mrouted) daemons continue
	 * to call ioctl() after the device has gone down */
	dev->do_ioctl = NULL;
	
	sdlc_set_intr_mode(card, 0);
	if (card->u.sdlc.comm_enabled){
		sdlc_comm_disable(card);
	}

	wan_spin_unlock_irq(&wandev->lock,&smp_lock);
	
	port_set_state(card, WAN_DISCONNECTED);

	return 0;
}

/**
 *	if_do_ioctl - Ioctl handler for fr
 *	@dev: Device subject to ioctl
 *	@ifr: Interface request block from the user
 *	@cmd: Command that is being issued
 *	
 *	This function handles the ioctls that may be issued by the user
 *	to control the settings of a FR. It does both busy
 *	and security checks. This function is intended to be wrapped by
 *	callers who wish to add additional ioctl calls of their own.
 */
/* SNMP */ 
static int if_do_ioctl(netdevice_t *dev, struct ifreq *ifr, int cmd)
{
	sdlc_private_area_t* chan= (sdlc_private_area_t*)dev->priv;
	unsigned long smp_flags;
	sdla_t *card;
	wan_udp_pkt_t *wan_udp_pkt;
	wan_mbox_t *usr_mb, *mb;
	int err=0;
	char usedby=API;

	if (!chan){
		return -ENODEV;
	}
	card=chan->card;
	mb=&card->wan_mbox;

	if (chan->common.usedby == STACK){
		//if (ifr->ifr_flags != STACK_IF_REQ){
		//	printk(KERN_INFO 
		//	    "%s: Stack if cmd request with invalid flag %i, expecting %i\n",
		//			card->devname,ifr->ifr_flags,STACK_IF_REQ);
		//	return -EINVAL;
		//}
		usedby=STACK;
	}
	

	switch(cmd){

	case SIOC_WANPIPE_DEV_STATE:
		
		err = chan->common.state;
		break;
		
	case SIOC_WANPIPE_BIND_SK:
		
		if (!ifr){
			err= -EINVAL;
			break;
		}
		
		DEBUG_TEST("%s: SIOC_WANPIPE_BIND_SK \n",__FUNCTION__);
		spin_lock_irqsave(&card->wandev.lock,smp_flags);
		err=wan_bind_api_to_svc(chan,ifr->ifr_data);
		spin_unlock_irqrestore(&card->wandev.lock,smp_flags);
		break;

	case SIOC_WANPIPE_UNBIND_SK:
		
		if (!ifr){
			err= -EINVAL;
			break;
		}

		DEBUG_TEST("%s: SIOC_WANPIPE_UNBIND_SK \n",__FUNCTION__);
		
		spin_lock_irqsave(&card->wandev.lock,smp_flags);
		err=wan_unbind_api_from_svc(chan,ifr->ifr_data);
		spin_unlock_irqrestore(&card->wandev.lock,smp_flags);
		break;

		
	case SIOC_WANPIPE_PIPEMON: 

		NET_ADMIN_CHECK();
		
		if (atomic_read(&chan->udp_pkt_len) != 0){
			return -EBUSY;
		}

		atomic_set(&chan->udp_pkt_len,MAX_LGTH_UDP_MGNT_PKT);
		
		/* For performance reasons test the critical
		 * here before spin lock */
		if (test_bit(0,&card->in_isr)){
			atomic_set(&chan->udp_pkt_len,0);
			return -EBUSY;
		}
	
		
		wan_udp_pkt=(wan_udp_pkt_t*)chan->udp_pkt_data;
		if (copy_from_user(&wan_udp_pkt->wan_udp_hdr,
				   ifr->ifr_data,
				   sizeof(wan_udp_hdr_t))){
			atomic_set(&chan->udp_pkt_len,0);
			return -EFAULT;
		}

		spin_lock_irqsave(&card->wandev.lock, smp_flags);

		/* We have to check here again because we don't know
		 * what happened during spin_lock */
		if (test_bit(0,&card->in_isr)) {
			printk(KERN_INFO "%s:%s Pipemon command failed, Driver busy: try again.\n",
					card->devname,dev->name);
			atomic_set(&chan->udp_pkt_len,0);
			spin_unlock_irqrestore(&card->wandev.lock, smp_flags);
			return -EBUSY;
		}
		
		process_udp_mgmt_pkt(card,dev,chan,1);
		
		spin_unlock_irqrestore(&card->wandev.lock, smp_flags);

		/* This area will still be critical to other
		 * PIPEMON commands due to udp_pkt_len
		 * thus we can release the irq */
		
		if (atomic_read(&chan->udp_pkt_len) > sizeof(wan_udp_pkt_t)){
			printk(KERN_INFO "%s: Error: Pipemon buf too bit on the way up! %i\n",
					card->devname,atomic_read(&chan->udp_pkt_len));
			atomic_set(&chan->udp_pkt_len,0);
			return -EINVAL;
		}

		if (copy_to_user(ifr->ifr_data,&wan_udp_pkt->wan_udp_hdr,sizeof(wan_udp_hdr_t))){
			atomic_set(&chan->udp_pkt_len,0);
			return -EFAULT;
		}
		
		atomic_set(&chan->udp_pkt_len,0);
		return 0;

	case SIOC_WANPIPE_EXEC_CMD:
	
		usr_mb=(wan_mbox_t*)ifr->ifr_ifru.ifru_data;
		if (!usr_mb){
			printk (KERN_INFO 
			"%s: Ioctl command %x, has no Mbox attached\n",
			card->devname, cmd);
			return -EINVAL;
		}

		if (usedby == STACK){
			memcpy(&mb->wan_command,&usr_mb->wan_command,sizeof(wan_cmd_t)-1);
		}else{
			if (copy_from_user(&mb->wan_command, &usr_mb->wan_command,sizeof(wan_cmd_t)-1)){
				printk(KERN_INFO "%s: SDLC Cmd: Failed to copy mb \n",card->devname);
				err = -EFAULT;
				break;
			}
		}
		
		if (mb->wan_data_len){
			if (usedby == STACK){
				memcpy(&mb->wan_data[0], &usr_mb->wan_data[0] ,mb->wan_data_len);
			}else{
				if (copy_from_user(&mb->wan_data[0], &usr_mb->wan_data[0] ,mb->wan_data_len)){
					printk(KERN_INFO "%s: SDLC Cmd: Failed to copy mb data: len= %i\n",
							card->devname, mb->wan_data_len);
					err = -EFAULT;
					break;
				}
			}
		}

		spin_lock_irqsave(&card->wandev.lock,smp_flags);
		
		err = card->hw_iface.cmd(card->hw, card->mbox_off, mb);
		if (err != COMMAND_OK) {
			spin_unlock_irqrestore(&card->wandev.lock,smp_flags);
			break;
		}
	
		spin_unlock_irqrestore(&card->wandev.lock,smp_flags);

		DEBUG_TEST("%s: CMD=0x%x  Err=0x%X\n",
				card->devname,
				mb->wan_command,mb->wan_return_code);
		
		/* copy the result back to our buffer */
		
		if (usedby == STACK){
			memcpy(&usr_mb->wan_command, &mb->wan_command,sizeof(wan_cmd_t)-1);
		}else{
			if (copy_to_user(&usr_mb->wan_command, &mb->wan_command,sizeof(wan_cmd_t)-1)){
				err= -EFAULT;
				break;
			}
		}
		
		if (mb->wan_data_len) {
			if (usedby == STACK){
				memcpy(&usr_mb->wan_data[0], &mb->wan_data[0], mb->wan_data_len);
			}else{
				if (copy_to_user(&usr_mb->wan_data[0], &mb->wan_data[0], mb->wan_data_len)){
					err= -EFAULT;
					break;
				}
			}
		}

		break;

		
	default:
		return -EOPNOTSUPP;
	}
	return err;
}



/****** Network Device Interface ********************************************/

/*============================================================================
 * Initialize Linux network interface.
 *
 * This routine is called only once for each interface, during Linux network
 * interface registration.  Returning anything but zero will fail interface
 * registration.
 */
static int if_init (netdevice_t* dev)
{
	sdlc_private_area_t* sdlc_priv_area = dev->priv;
	sdla_t* card = sdlc_priv_area->card;
	wan_device_t* wandev = &card->wandev;
	
	/* NOTE: Most of the dev initialization was
         *       done in sppp_attach(), called by new_if() 
         *       function. All we have to do here is
         *       to link four major routines below. 
         */

	/* Initialize device driver entry points */
	dev->open		= &if_open;
	dev->stop		= &if_close;
	dev->hard_start_xmit	= &if_send;
	dev->get_stats		= &if_stats;
#if defined(LINUX_2_4)||defined(LINUX_2_6)
	dev->tx_timeout		= NULL;
	dev->watchdog_timeo	= 0;
#endif
	dev->do_ioctl		= if_do_ioctl;

	dev->type		= ARPHRD_SDLC; /* This breaks the tcpdump */

	dev->mtu	= card->wandev.mtu+sizeof(wan_api_hdr_t);

	/* Initialize hardware parameters */
	dev->irq	= wandev->irq;
	dev->dma	= wandev->dma;
	dev->base_addr	= wandev->ioport;
	card->hw_iface.getcfg(card->hw, SDLA_MEMBASE, &dev->mem_start); //ALEX_TODAY wandev->maddr;
	card->hw_iface.getcfg(card->hw, SDLA_MEMEND, &dev->mem_end); //ALEX_TODAY wandev->maddr + wandev->msize - 1;

	/* Set transmit buffer queue length 
         * If we over fill this queue the packets will
         * be droped by the kernel.
         * sppp_attach() sets this to 10, but
         * 100 will give us more room at low speeds.
	 */
        dev->tx_queue_len = 100;
   
	return 0;
}


/*============================================================================
 * Open network interface.
 * o enable communications and interrupts.
 * o prevent module from unloading by incrementing use count
 *
 * Return 0 if O.k. or errno.
 */
static int if_open (netdevice_t* dev)
{
	sdlc_private_area_t* sdlc_priv_area = dev->priv;
	sdla_t* card = sdlc_priv_area->card;
	struct timeval tv;

	/* Only one open per interface is allowed */

	if (open_dev_check(dev))
		return -EBUSY;

	do_gettimeofday(&tv);
	sdlc_priv_area->router_start_time = tv.tv_sec;
 
	netif_start_queue(dev);
	
	wanpipe_open(card);

	port_set_state(card, WAN_CONNECTED);

	if (sdlc_priv_area->common.usedby==API){
		wan_wakeup_api(sdlc_priv_area);
	}
	return 0;
}

/*============================================================================
 * Close network interface.
 * o if this is the last close, then disable communications and interrupts.
 * o reset flags.
 */
static int if_close (netdevice_t* dev)
{
	sdlc_private_area_t* sdlc_priv_area = dev->priv;
	sdla_t* card = sdlc_priv_area->card;

	stop_net_queue(dev);

#if defined(LINUX_2_1)
	dev->start=0;
#endif

	wanpipe_close(card);

	port_set_state(card, WAN_DISCONNECTED);
	return 0;
}

/*============================================================================
 * Send a packet on a network interface.
 * o set tbusy flag (marks start of the transmission) to block a timer-based
 *   transmit from overlapping.
 * o check link state. If link is not up, then drop the packet.
 * o execute adapter send command.
 * o free socket buffer
 *
 * Return:	0	complete (socket buffer must be freed)
 *		non-0	packet may be re-transmitted (tbusy must be set)
 *
 * Notes:
 * 1. This routine is called either by the protocol stack or by the "net
 *    bottom half" (with interrupts enabled).
 * 2. Setting tbusy flag will inhibit further transmit requests from the
 *    protocol stack and can be used for flow control with protocol layer.
 */
static int if_send (struct sk_buff* skb, netdevice_t* dev)
{
	sdlc_private_area_t *sdlc_priv_area = dev->priv;
	sdla_t *card = sdlc_priv_area->card;
	unsigned long smp_flags;
	int err=0;

	if (skb == NULL){
		/* If we get here, some higher layer thinks we've missed an
		 * tx-done interrupt.
		 */
		printk(KERN_INFO "%s: Received NULL skb buffer! interface %s got kicked!\n",
			card->devname, dev->name);

		wake_net_dev(dev);
		return 0;
	}

#if defined(LINUX_2_1)
	if (dev->tbusy){

		/* If our device stays busy for at least 5 seconds then we will
		 * kick start the device by making dev->tbusy = 0.  We expect 
		 * that our device never stays busy more than 5 seconds. So this
		 * is only used as a last resort. 
		 */
		if((jiffies - sdlc_priv_area->tick_counter) < (5 * HZ)) {
			return 1;
		}

		++card->wandev.stats.collisions;

		printk (KERN_INFO "%s: Transmit timed out on %s\n", card->devname,dev->name);

		netif_wake_queue (dev);
	}
#endif

	sdlc_priv_area->tick_counter=jiffies;
	err=0;

    	if (test_and_set_bit(SEND_CRIT, (void*)&card->wandev.critical)){
	
		printk(KERN_INFO "%s: Critical in if_send: %lx\n",
					card->wandev.name,card->wandev.critical);
                ++card->wandev.stats.tx_dropped;
		goto if_send_crit_exit;
	}

	if (card->wandev.state != WAN_CONNECTED){
		++card->wandev.stats.tx_dropped;
		goto if_send_crit_exit;
	}

	wan_spin_lock_irq(&card->wandev.lock,&smp_flags);
	err=sdlc_send(card, skb->data, skb->len);
	wan_spin_unlock_irq(&card->wandev.lock,&smp_flags);

	if (err){
		err=1;
	}else{
		++card->wandev.stats.tx_packets;
       		card->wandev.stats.tx_bytes += skb->len;
#if defined(LINUX_2_4)||defined(LINUX_2_6)
		dev->trans_start = jiffies;
#endif

		if (test_bit(0,&sdlc_priv_area->trace_state)){
			int i;	
			DEBUG_EVENT("%s:%s: OUTPUT:\n",card->devname,dev->name);
			for (i=0; i < skb->len; i++){		
				printk("%02X ",skb->data[i]);
			}
			printk("\n");
		}
		
	}	

if_send_crit_exit:
	if (err==0){
                dev_kfree_skb_any(skb);
	}

	start_net_queue(dev);
	clear_bit(SEND_CRIT, (void*)&card->wandev.critical);

	return err;
}

/*============================================================================
 * Get ethernet-style interface statistics.
 * Return a pointer to struct enet_statistics.
 */
static struct net_device_stats* if_stats (netdevice_t* dev)
{
	sdla_t *my_card;
	sdlc_private_area_t* sdlc_priv_area;

	/* Shutdown bug fix. In del_if() we kill
         * dev->priv pointer. This function, gets
         * called after del_if(), thus check
         * if pointer has been deleted */
	if ((sdlc_priv_area=dev->priv) == NULL)
		return NULL;

	my_card = sdlc_priv_area->card;
	return &my_card->wandev.stats; 
}

/****** Cisco HDLC Firmware Interface Functions *******************************/

/*============================================================================
 * Read firmware code version.
 *	Put code version as ASCII string in str. 
 */
static int sdlc_read_version (sdla_t* card, char* str)
{
	wan_mbox_t* mb = &card->wan_mbox;
	int len;
	char err;
	mb->wan_data_len = 0;
	mb->wan_command = READ_CODE_VERSION;
	err = card->hw_iface.cmd(card->hw, card->mbox_off, mb);

	if(err != COMMAND_OK) {
		sdlc_error(card,err,mb);
	
	}else if (str) {  /* is not null */
		len = mb->wan_data_len;
		memcpy(str, mb->wan_data, len);
		str[len] = '\0';
	}
	return (err);
}

/*-----------------------------------------------------------------------------
 *  Configure CHDLC firmware.
 */
static int sdlc_configure (sdla_t* card, void* data)
{
	int err;
	wan_mbox_t *mb = &card->wan_mbox;
	int data_length = sizeof(SDLC_CONFIGURATION_STRUCT);
	int retry=10;

	do {
		mb->wan_data_len = data_length;  
		memcpy(mb->wan_data, data, data_length);
		mb->wan_command = SET_SDLC_CONFIGURATION;
		err = card->hw_iface.cmd(card->hw, card->mbox_off, mb);
		
		if (err != COMMAND_OK) 
			sdlc_error (card, err, mb);

		if (err != 0x30){
			break;
		}
		    
	}while((err!=COMMAND_OK) && --retry);

	return err;
}

/*-----------------------------------------------------------------------------
 *  Configure CHDLC firmware.
 */
static int sdlc_read_config (sdla_t* card)
{
	int err;
	wan_mbox_t *mb = &card->wan_mbox;
	
	mb->wan_data_len = 0;
	mb->wan_command = READ_SDLC_CONFIGURATION;
	err = card->hw_iface.cmd(card->hw, card->mbox_off, mb);
	
	if (err != COMMAND_OK) sdlc_error (card, err, mb);
                           
	return err;
}


/*============================================================================
 * Set interrupt mode -- HDLC Version.
 */

static int sdlc_set_intr_mode (sdla_t* card, unsigned mode)
{
	wan_mbox_t* mb = &card->wan_mbox;
	SDLC_INT_TRIGGERS_STRUCT* int_data =
		 (SDLC_INT_TRIGGERS_STRUCT *)mb->wan_data;
	int err;

	int_data->CHDLC_interrupt_triggers 	= mode;
	int_data->IRQ				= card->wandev.irq; // ALEX_TODAY card->hw.irq;
	int_data->interrupt_timer               = 1;
	int_data->misc_interrupt_bits		= 0;
   
	mb->wan_data_len = sizeof(SDLC_INT_TRIGGERS_STRUCT);
	mb->wan_command = SET_INTERRUPT_TRIGGERS;
	err = card->hw_iface.cmd(card->hw, card->mbox_off, mb);
	if (err != COMMAND_OK)
		sdlc_error (card, err, mb);
	return err;
}


/*============================================================================
 * Enable communications.
 */

static int sdlc_comm_enable (sdla_t* card)
{
	int err;
	wan_mbox_t* mb = &card->wan_mbox;
	
	mb->wan_data_len = 0;
	mb->wan_command = ENABLE_COMMUNICATIONS;
	err = card->hw_iface.cmd(card->hw, card->mbox_off, mb);
	if (err != COMMAND_OK)
		sdlc_error(card, err, mb);
	else
		card->u.sdlc.comm_enabled=1;

	return err;
}

/*============================================================================
 * Disable communications and Drop the Modem lines (DCD and RTS).
 */
static int sdlc_comm_disable (sdla_t* card)
{
	int err;
	wan_mbox_t* mb = &card->wan_mbox;

	mb->wan_data_len = 0;
	mb->wan_command = DISABLE_COMMUNICATIONS;
	err = card->hw_iface.cmd(card->hw, card->mbox_off, mb);
	if (err != COMMAND_OK)
		sdlc_error(card,err,mb);

	return err;
}

/*============================================================================
 * Read communication error statistics.
 */
static int sdlc_read_comm_err_stats (sdla_t* card)
{
        int err;
        wan_mbox_t* mb = &card->wan_mbox;

        mb->wan_data_len = 0;
        mb->wan_command = READ_COMMS_ERR_STATISTICS;
        err = card->hw_iface.cmd(card->hw, card->mbox_off, mb);
        if (err != COMMAND_OK)
                sdlc_error(card,err,mb);
        return err;
}


/*============================================================================
 * Read CHDLC operational statistics.
 */
static int sdlc_read_op_stats (sdla_t* card)
{
        int err;
        wan_mbox_t* mb = &card->wan_mbox;

        mb->wan_data_len = 0;
        mb->wan_command = READ_OPERATIONAL_STATISTICS;
        err = card->hw_iface.cmd(card->hw, card->mbox_off, mb);
        if (err != COMMAND_OK)
                sdlc_error(card,err,mb);
        return err;
}


/*============================================================================
 * Update communications error and general packet statistics.
 */
static int update_comms_stats(sdla_t* card, sdlc_private_area_t* sdlc_priv_area)
{
        wan_mbox_t* mb = &card->wan_mbox;
  	COMMS_ERROR_STATS_STRUCT *err_stats;
        SDLC_OPERATIONAL_STATS_STRUCT *op_stats;

	/* on the first timer interrupt, read the comms error statistics */
	if(sdlc_priv_area->update_comms_stats == 2) {
		if(sdlc_read_comm_err_stats(card))
			return 1;
		err_stats = (COMMS_ERROR_STATS_STRUCT *)mb->wan_data;
		card->wandev.stats.rx_over_errors = 
				err_stats->Rx_overrun_err_cnt;
		card->wandev.stats.rx_crc_errors = 
				err_stats->CRC_err_cnt;
		card->wandev.stats.rx_frame_errors = 
				err_stats->Rx_abort_cnt;
		card->wandev.stats.rx_fifo_errors = 
				err_stats->Rx_frm_lgth_err_cnt; 
		card->wandev.stats.rx_missed_errors =
				card->wandev.stats.rx_fifo_errors;
		card->wandev.stats.tx_aborted_errors =
				err_stats->msd_Tx_und_int_cnt;
	}

        /* on the second timer interrupt, read the operational statistics */
	else {
        	if(sdlc_read_op_stats(card))
                	return 1;
		op_stats = (SDLC_OPERATIONAL_STATS_STRUCT *)mb->wan_data;
		//FIXME
		card->wandev.stats.rx_length_errors =
			(op_stats->no_short_frames_Rx +
			op_stats->no_short_frames_Rx);
	}

	return 0;
}
/*============================================================================
 * Send packet.
 *	Return:	0 - o.k.
 *		1 - no transmit buffers available
 */
static int sdlc_list_stations_with_ifrms (sdla_t* card)
{
	wan_mbox_t *mb = &card->wan_mbox;
	int retry=MAX_CMD_RETRY;
	int err;

	do {
		mb->wan_command=LIST_STATIONS_WITH_I_FRMS_AVAILABLE;
		mb->wan_return_code=0;	
		mb->wan_data_len=0;
		err = card->hw_iface.cmd(card->hw, card->mbox_off, mb);
		if (err){
			sdlc_error(card,err,mb);
		}
	}while (err && --retry);

	return err;
}

static int sdlc_read (sdla_t* card,unsigned char station)
{
	wan_mbox_t *mb = &card->wan_mbox;
	volatile int retry=MAX_CMD_RETRY;
	volatile int err;

	do {
		mb->wan_command=SDLC_READ;
		mb->wan_return_code=0;	
		mb->wan_data_len=0;
		mb->wan_sdlc_address=station;
		err = card->hw_iface.cmd(card->hw, card->rxmb_off, mb);
		if (err){
			sdlc_error(card,err,mb);
		}
	}while (err && --retry);

	return err;
}

/*============================================================================
 * Send packet.
 *	Return:	0 - o.k.
 *		1 - no transmit buffers available
 */
static int sdlc_send (sdla_t* card, void* data, unsigned len)
{
	wan_mbox_t *mb = &card->wan_mbox;
	wan_api_t* wan_api;

	if (mb->wan_opp_flag){
		printk(KERN_INFO "%s: Critical in sdlc_send() opp flag set!",
				card->devname);
		return 1;
	}

	wan_api = (wan_api_t*)data;
	len -= sizeof(wan_api_hdr_t);
		
	mb->wan_command              =SDLC_WRITE;
	mb->wan_return_code          =0;	
	mb->wan_data_len 	     = len;
	mb->wan_sdlc_address 	     = wan_api->wan_api_sdlc_station;
	mb->wan_sdlc_poll_interval   = wan_api->wan_api_sdlc_poll_interval;
	mb->wan_sdlc_pf	     	     = wan_api->wan_api_sdlc_pf;
	mb->wan_sdlc_general_mb_byte = wan_api->wan_api_sdlc_general_mb_byte;
			     
	memcpy(mb->wan_data,wan_api->data,len);

	return card->hw_iface.cmd(card->hw, card->mbox_off, mb);
}
/****** Firmware Error Handler **********************************************/

/*============================================================================
 * Firmware error handler.
 *	This routine is called whenever firmware command returns non-zero
 *	return code.
 *
 * Return zero if previous command has to be cancelled.
 */
static int sdlc_error (sdla_t *card, int err, wan_mbox_t *mb)
{
	unsigned cmd = mb->wan_command;

	switch (err) {

	case CMD_TIMEOUT:
		printk(KERN_INFO "%s: command 0x%02X timed out!\n",
			card->devname, cmd);
		break;
	
	case BUSY_WITH_BAUD_CALIBRATION:
		/* Do not print here, because we have to wait in
		 * this contiditon */
		break;
		
	case BAUD_CALIBRATION_FAILED:
		printk(KERN_INFO "%s: Baud calibration failed!\n",
				card->devname);
		break;

	default:
		printk(KERN_INFO "%s: command 0x%02X returned 0x%02X!\n",
			card->devname, cmd, err);
	}

	return 0;
}

/****** Interrupt Handlers **************************************************/

/*============================================================================
 * Cisco HDLC interrupt service routine.
 */
static WAN_IRQ_RETVAL wp_sdlc_isr (sdla_t* card)
{
	netdevice_t* dev;
	INTERRUPT_INFORMATION_STRUCT flags;

	dev = WAN_DEVLE2DEV(WAN_LIST_FIRST(&card->wandev.dev_head));
	card->in_isr = 1;
	card->hw_iface.peek(card->hw, card->flags_off, &flags, sizeof(flags));
		
	/* If we get an interrupt with no network device, stop the interrupts
	 * and issue an error */
	if ((!dev || !dev->priv) && flags.interrupt_type != COMMAND_COMPLETE_INTERRUPT_PENDING){
		goto isr_done;
	}

	/* if critical due to peripheral operations
	 * ie. update() or getstats() then reset the interrupt and
	 * wait for the board to retrigger.
	 */
	if(test_bit(PERI_CRIT, (void*)&card->wandev.critical)) {
		goto isr_done;
	}


	/* On a 508 Card, if critical due to if_send 
         * Major Error !!!
	 */
	if(card->type != SDLA_S514) {
		if(test_bit(0, (void*)&card->wandev.critical)) {
			printk(KERN_INFO "%s: Critical while in ISR: %lx\n",
				card->devname, card->wandev.critical);
			goto isr_done;
		}
	}


	switch(flags.interrupt_type) {

		case RX_INTERRUPT_PENDING:	/* 0x01: receive interrupt */
			rx_intr(card);
			break;

		case TX_INTERRUPT_PENDING:	/* 0x02: transmit interrupt */

			/* Not used, because, tx interrupt
			 * is not supported by Gideon */
			tx_intr(card);
			card->hw_iface.clear_bit(card->hw, card->intr_perm_off, INTERRUPT_ON_TX_FRAME);
			break;

		case COMMAND_COMPLETE_INTERRUPT_PENDING:/* 0x04: cmd cplt */
			++ card->timer_int_enabled;
			break;

		case EXCEPTION_CONDITION_INTERRUPT_PENDING:	/* 0x20 */
			process_sdlc_exception(card);
			break;

		case TIMER_INTERRUPT_PENDING:
			timer_intr(card);
			break;

		default:
			printk(KERN_INFO "%s: spurious interrupt 0x%02X!\n", 
				card->devname,
				flags.interrupt_type);
			break;
	}

isr_done:
	card->hw_iface.poke_byte(card->hw, card->intr_type_off, 0x00);
	card->in_isr = 0;
	WAN_IRQ_RETURN(WAN_IRQ_HANDLED);
}

static unsigned char station_list[500];

/*============================================================================
 * Receive interrupt handler.
 */
static void rx_intr (sdla_t* card)
{
	netdevice_t *dev;
	sdlc_private_area_t *chan;
	wan_mbox_t *mb    = &card->wan_mbox;
	wan_mbox_t *rxmb = &card->wan_rxmb;
	struct sk_buff *skb;
	unsigned len,station_cnt;
	unsigned char *buf;
	int i,udp_type;
	
	dev = WAN_DEVLE2DEV(WAN_LIST_FIRST(&card->wandev.dev_head));
	if (!dev){ 
		goto rx_exit;
	}

#if defined(LINUX_2_4)||defined(LINUX_2_6)
	if (!netif_running(dev)){
		goto rx_exit;
	}
#else
	if (!dev->start){ 
		goto rx_exit;
	}
#endif

	chan = dev->priv;

	
	if (sdlc_list_stations_with_ifrms(card) != 0){
		goto rx_exit;
	}

	station_cnt=mb->wan_data_len;
	memcpy(&station_list,mb->wan_data,station_cnt);

	for (i=0; i < station_cnt; i++){
		int err;

		err = sdlc_read(card,station_list[i]);

		if (err != 0){
			DEBUG_EVENT("%s: Receive failed for station %i\n",
					card->devname, mb->wan_data[i]);
			continue;
		}
		
		card->hw_iface.peek(card->hw, card->rxmb_off, rxmb, sizeof(wan_cmd_t));
		len=rxmb->wan_data_len;
		if (len){
			card->hw_iface.peek(card->hw, 
					    card->rxmb_off+offsetof(wan_mbox_t, wan_data), 
					    rxmb->wan_data, len);
		}
		
		if (len > card->wandev.mtu){
			printk(KERN_INFO "%s: Rx packet too big len %i > MTU %i\n",
					card->devname,len,card->wandev.mtu);
			continue;
		}
		
		/* Allocate socket buffer */
		skb = dev_alloc_skb(len+sizeof(wan_api_hdr_t)+2);
		if (skb == NULL){
			if (net_ratelimit()){
			printk(KERN_INFO "%s: no socket buffers available!\n",
							card->devname);
			}
			++card->wandev.stats.rx_dropped;
			goto rx_exit;
		}

		/* Align to 16 byte boundary */
		skb_reserve(skb,2);

		buf=skb_put(skb,len);
		memcpy(buf,rxmb->wan_data,len);

		skb->protocol = htons(ETH_P_IP);

		card->wandev.stats.rx_packets++;
		card->wandev.stats.rx_bytes += skb->len;

		udp_type = wan_udp_pkt_type(card,skb->data);

		if (chan->common.usedby == API){
	
			wan_api_hdr_t	*api_hdr;
			skb_push(skb,sizeof(wan_api_hdr_t));
			api_hdr = (wan_api_hdr_t*)&skb->data[0];
			api_hdr->wan_apihdr_sdlc_station	=mb->wan_sdlc_address;
			api_hdr->wan_apihdr_sdlc_pf		=mb->wan_sdlc_pf;
			api_hdr->wan_apihdr_sdlc_poll_interval	=mb->wan_sdlc_poll_interval;
			api_hdr->wan_apihdr_sdlc_general_mb_byte=mb->wan_sdlc_general_mb_byte;
			
			skb->pkt_type = WAN_PACKET_DATA;

			if (chan->common.sk == NULL){
				++card->wandev.stats.rx_dropped;

				card->wandev.stats.rx_packets --;
				card->wandev.stats.rx_bytes -= skb->len;

				dev_kfree_skb_any(skb);
				continue;
			}

			if (wan_api_rx(chan,skb) != 0){
				++card->wandev.stats.rx_dropped;
				
				card->wandev.stats.rx_packets --;
				card->wandev.stats.rx_bytes -= skb->len;
				
				dev_kfree_skb_any(skb);
			}
		
		}else if (chan->common.usedby == STACK) {

			wan_api_hdr_t	*api_hdr;
			skb_push(skb,sizeof(wan_api_hdr_t));
			api_hdr = (wan_api_hdr_t*)&skb->data[0];
			api_hdr->wan_apihdr_sdlc_station=mb->wan_data[i];

			skb->pkt_type = WAN_PACKET_DATA;

			if (!test_bit(REG_CRIT,&card->wandev.critical) ||
			    !chan->wp_sdlc_register.sdlc_stack_rx){

				++card->wandev.stats.rx_dropped;

				card->wandev.stats.rx_packets --;
				card->wandev.stats.rx_bytes -= skb->len;

				dev_kfree_skb_any(skb);
				continue;
			}

			skb->dev = dev;
			wan_skb_reset_mac_header(skb);
					
			if (chan->wp_sdlc_register.sdlc_stack_rx(skb) != 0){
				++card->wandev.stats.rx_dropped;
				
				card->wandev.stats.rx_packets --;
				card->wandev.stats.rx_bytes -= skb->len;
				
				dev_kfree_skb_any(skb);
			}
		
		}else{
			/* Pass it up the protocol stack */
			skb->dev = dev;
			wan_skb_reset_mac_header(skb);
			netif_rx(skb);
		}
	}

rx_exit:
	return;
}

static void tx_intr(sdla_t *card)
{
	netdevice_t *dev;
	sdlc_private_area_t* sdlc_priv_area;

	dev = WAN_DEVLE2DEV(WAN_LIST_FIRST(&card->wandev.dev_head));
	if (dev == NULL){
		return;
	}

	if ((sdlc_priv_area=dev->priv) == NULL){
		return;
	}

	if (sdlc_priv_area->common.usedby == API){
		start_net_queue(dev);
		wan_wakeup_api(sdlc_priv_area);
	}else{
		wake_net_dev(dev);
	}
}

/*============================================================================
 * Timer interrupt handler.
 * The timer interrupt is used for two purposes:
 *    1) Processing udp calls from 'cpipemon'.
 *    2) Reading board-level statistics for updating the proc file system.
 */
void timer_intr(sdla_t *card)
{
        netdevice_t* dev;
        sdlc_private_area_t* sdlc_priv_area = NULL;

        dev = WAN_DEVLE2DEV(WAN_LIST_FIRST(&card->wandev.dev_head)); 
	if (!dev) return;
        sdlc_priv_area = dev->priv;

	/* read the communications statistics if required */
	if(sdlc_priv_area->timer_int_enabled & TMR_INT_ENABLED_UPDATE){
		update_comms_stats(card, sdlc_priv_area);
                if (IS_TE1_CARD(card)) {
                        /* TE1 Update T1/E1 alarms */
                        card->wandev.fe_iface.read_alarm(&card->fe, 0);
                        /* TE1 Update T1/E1 perfomance counters */
                        card->wandev.fe_iface.read_pmon(&card->fe, 0);

                }else if (IS_56K_CARD(card)) {
			/* 56K Update CSU/DSU alarms */
                        card->wandev.fe_iface.read_alarm(&card->fe, 1);
		}	
		
                if(!(-- sdlc_priv_area->update_comms_stats)) {
			sdlc_priv_area->timer_int_enabled &= 
				~TMR_INT_ENABLED_UPDATE;
		}
        }

	/* only disable the timer interrupt if there are no udp or statistic */
	/* updates pending */
        if(!sdlc_priv_area->timer_int_enabled) {
		card->hw_iface.clear_bit(card->hw, card->intr_perm_off, INTERRUPT_ON_TIMER);
        }
}

/*------------------------------------------------------------------------------
  Miscellaneous Functions
	- set_sdlc_config() used to set configuration options on the board
------------------------------------------------------------------------------*/

static int set_sdlc_config(sdla_t* card)
{

	SDLC_CONFIGURATION_STRUCT cfg;

	memset(&cfg, 0, sizeof(SDLC_CONFIGURATION_STRUCT));

	/* Generate an interrupt on an exception condition,
	 * do not give it to me on a command */
	cfg.general_operational_config_bits |= APP_REQUESTS_PASSING_EXCEPT_COND;
	
	if(card->wandev.clocking){
		cfg.baud_rate = card->wandev.bps;
	}
	
	/* the station configuration (primary or secondary) */
	cfg.station_configuration=card->wandev.sdlc_cfg.station_configuration;		
	
	/* the maximum permitted Information field length supported */
	if (card->wandev.sdlc_cfg.max_I_field_length){
		cfg.max_I_field_length	= card->wandev.mtu = card->wandev.sdlc_cfg.max_I_field_length;
	}else{
		cfg.max_I_field_length	= card->wandev.mtu;
	}
	
	cfg.general_operational_config_bits=card->wandev.sdlc_cfg.general_operational_config_bits;

	if (card->wandev.electrical_interface != WANOPT_RS232){
		cfg.general_operational_config_bits |= INTERFACE_LEVEL_V35;
	}

	
	/* miscellaneous protocol configuration bits */
	cfg.protocol_config_bits=card->wandev.sdlc_cfg.protocol_config_bits;
	
	/* bits to specify which exception conditions are reported to the application */
	cfg.exception_condition_reporting_config=card->wandev.sdlc_cfg.exception_condition_reporting_config;
	
	/* modem operation configuration bits */
	cfg.modem_config_bits=card->wandev.sdlc_cfg.modem_config_bits;
	
	/* the statistics format byte */
	cfg.statistics_format=card->wandev.sdlc_cfg.statistics_format;
	
	/* the slow poll interval for a primary station (milliseconds) */
	cfg.pri_station_slow_poll_interval=card->wandev.sdlc_cfg.pri_station_slow_poll_interval;
	
	/* the permitted secondary station_response timeout (milliseconds) */
	cfg.permitted_sec_station_response_TO=card->wandev.sdlc_cfg.permitted_sec_station_response_TO;
	
	/* the number of consecutive secondary timeouts while */
	/* in the NRM before a SNRM is issued */
	cfg.no_consec_sec_TOs_in_NRM_before_SNRM_issued=card->wandev.sdlc_cfg.no_consec_sec_TOs_in_NRM_before_SNRM_issued;
	
	/* the maximum Information field length permitted in an XID */
	/* frame sent by a primary station */
	cfg.max_lgth_I_fld_pri_XID_frame=card->wandev.sdlc_cfg.max_lgth_I_fld_pri_XID_frame;		
	
	/* the additional bit delay before transmitting the opening */
	/* character in a frame */
	cfg.opening_flag_bit_delay_count=card->wandev.sdlc_cfg.opening_flag_bit_delay_count;		
	
	/* the additional bit delay before dropping RTS */
	cfg.RTS_bit_delay_count=card->wandev.sdlc_cfg.RTS_bit_delay_count;
	
	/* the permitted CTS timeout for switched CTS/RTS configurations */
	/* (in 1000ths/second) */
	cfg.CTS_timeout_1000ths_sec=card->wandev.sdlc_cfg.CTS_timeout_1000ths_sec;	
	
	/* the adapter type and the CPU speed */
	cfg.SDLA_configuration=card->wandev.sdlc_cfg.SDLA_configuration;			

	DEBUG_EVENT("\n");

	DEBUG_EVENT("%s: SDLC Config:\n",card->devname);
	
	DEBUG_EVENT("%s:   Station Cfg           : 0x%X\n",
			card->devname,cfg.station_configuration);		
	
	DEBUG_EVENT("%s:   Baud Rate             : 0x%lX\n",
			card->devname,cfg.baud_rate);
	
	DEBUG_EVENT("%s:   Max I Frame Len       : %i\n",
			card->devname,cfg.max_I_field_length);

	DEBUG_EVENT("%s:   Misc Oper. Cfg        : 0x%X\n",
			card->devname,cfg.general_operational_config_bits);

	DEBUG_EVENT("%s:   Misc Protocol Cfg     : 0x%X\n",
			card->devname,cfg.protocol_config_bits);

	DEBUG_EVENT("%s:   Exception Reporting   : 0x%X\n",
			card->devname,cfg.exception_condition_reporting_config);

	DEBUG_EVENT("%s:   Modem Op Cfg          : 0x%X\n",
			card->devname,cfg.modem_config_bits);
		
	DEBUG_EVENT("%s:   Statistics Format     : 0x%X\n",
			card->devname,cfg.statistics_format);

	DEBUG_EVENT("%s:   Slow poll Inter Pri   : %i\n",
			card->devname,cfg.pri_station_slow_poll_interval);

	DEBUG_EVENT("%s:   Sec Station Resp      : %i\n",
			card->devname,cfg.permitted_sec_station_response_TO);

	DEBUG_EVENT("%s:   No Tout NRM b4 SNRM   : %i\n",
			card->devname,cfg.no_consec_sec_TOs_in_NRM_before_SNRM_issued);
	
	DEBUG_EVENT("%s:   Max I Frm in XID  Pri : %i\n",
			card->devname,cfg.max_lgth_I_fld_pri_XID_frame);

	DEBUG_EVENT("%s:   Bit delay b4 Tx Ch    : %i\n",
			card->devname,cfg.opening_flag_bit_delay_count);

	DEBUG_EVENT("%s:   Bit delay b4 drop RTS : %i\n",
			card->devname,cfg.RTS_bit_delay_count);
	
	DEBUG_EVENT("%s:   CTS Timeout Sw CTS/RTS: %i\n",
			card->devname,cfg.CTS_timeout_1000ths_sec);
	
	DEBUG_EVENT("%s:   Adapter Type CPU Speed: 0x%X\n",
			card->devname,cfg.SDLA_configuration);

	DEBUG_EVENT("\n");
	
	return sdlc_configure(card, &cfg);
}


static void sdlc_modem_change(sdla_t *card, wan_mbox_t *mb)
{

#if 0
	if (IS_56K_CARD(card)) {
		INTERRUPT_INFORMATION_STRUCT *flags = card->u.sdlc.flags;
		FRONT_END_STATUS_STRUCT *FE_status =
		(FRONT_END_STATUS_STRUCT *)&flags->FT1_info_struct.parallel_port_A_input;
		card->wandev.RR8_reg_56k = 
			FE_status->FE_U.stat_56k.RR8_56k;	
		card->wandev.RRA_reg_56k = 
			FE_status->FE_U.stat_56k.RRA_56k;	
		card->wandev.RRC_reg_56k = 
			FE_status->FE_U.stat_56k.RRC_56k;	
                card->wandev.fe_iface.read_alarm(&card->fe, 0);
		return;
	}

	if (IS_TE1_CARD(card)) {
		/* TE_INTR */
		card->wandev.fe_iface.isr(&card->fe);
		return;
	}	
#endif

	printk(KERN_INFO "%s: Modem status change\n",
		card->devname);

	switch(mb->wan_data[0] & (DCD_HIGH | CTS_HIGH)) {
		case (DCD_HIGH):
			printk(KERN_INFO "%s: DCD high, CTS low\n",card->devname);
			break;
		case (CTS_HIGH):
			printk(KERN_INFO "%s: DCD low, CTS high\n",card->devname);
			break;
		case ((DCD_HIGH | CTS_HIGH)):
			printk(KERN_INFO "%s: DCD high, CTS high\n",card->devname);
			break;
		default:
			printk(KERN_INFO "%s: DCD low, CTS low\n",card->devname);
			break;
	}

	return;
}

static void sdlc_state_change(sdla_t *card, wan_mbox_t *mb)
{
	int i;
	netdevice_t *dev;
	sdlc_private_area_t *sdlc_chan;
	SDLC_STATE_STRUCT *sdlc_state=(SDLC_STATE_STRUCT *)mb->wan_data;
	
	dev = WAN_DEVLE2DEV(WAN_LIST_FIRST(&card->wandev.dev_head));
	if (!dev)
		return;

	if ((sdlc_chan=dev->priv) == NULL)
		return;
	
	for (i=0;i<mb->wan_data_len;i+=sizeof(SDLC_STATE_STRUCT)){
		printk(KERN_INFO "%s: Station %X State = %s\n",
				card->devname,
				sdlc_state->station,
				sdlc_state->state?"Connected (NRM)":"Disconnected (NDM)");
		sdlc_state++;
	}

	if (sdlc_chan->common.usedby != WANPIPE){
		send_oob_msg(card,mb);
	}
	return;
}

/*============================================================================
 * Process chdlc exception condition
 */
static int process_sdlc_exception(sdla_t *card)
{
	wan_mbox_t* mb = &card->wan_mbox;
	int err;

	mb->wan_data_len = 0;
	mb->wan_command = READ_EXCEPTION_CONDITION;
	err = card->hw_iface.cmd(card->hw, card->mbox_off, mb);
	if(err != CMD_TIMEOUT) {
	
		switch (err) {

		case EXCEP_COND_STATE_CHANGE:
			/* the SDLC station changed state */
			sdlc_state_change(card,mb);
			break;

		case EXCEP_COND_TIMEOUT_NRM:
			/* a timeout occurred while the stations was in the NRM */
			
			break;

		case EXCEP_COND_RD_FRM_RX_WHILE_IN_NRM:
			/* a RD was received while the link was in the NRM */

			break;
		case EXCEP_COND_DM_FRM_RX_WHILE_IN_NRM:
			/* a DM was received while the link was in the NRM */
			
			break;

		case EXCEP_COND_SNRM_FRM_RX_WHILE_IN_NRM:
			/* a SNRM was received while the link was in the NRM */

			break;

		case EXCEP_COND_RIM_FRM_RX:
			/* a RIM was received */

			break;

		case EXCEP_COND_XID_FRM_RX:
			/* an XID frame was received */
			
			break;

		case EXCEP_COND_TEST_FRM_RX:
			/* a TEST frame was received */

			break;

		case EXCEP_COND_FRMR_FRM_RX_TX:
			/* a FRMR condition occurred */

			break;

		case EXCEP_COND_CHANGE_IN_MODEM_STATUS:
			/* a modem status change occurred */
			sdlc_modem_change(card,mb);
			break;

		default:
			printk(KERN_INFO "%s: Exception Condition %x!\n",
					card->devname,err);
			break;
		}

	}
	return 0;
}

/*=============================================================================
 * Process UDP management packet.
 */

static int process_udp_mgmt_pkt(sdla_t* card, netdevice_t* dev,
				sdlc_private_area_t* sdlc_priv_area,int local_dev) 
{
	unsigned char *buf;
	unsigned int len;
	struct sk_buff *new_skb;
	int udp_mgmt_req_valid = 1;
	wan_mbox_t *mb = &card->wan_mbox;
	wan_udp_pkt_t *wan_udp_pkt;
	struct timeval tv;
	int err;

	wan_udp_pkt = (wan_udp_pkt_t *) sdlc_priv_area->udp_pkt_data;


	if(sdlc_priv_area->udp_pkt_src == UDP_PKT_FRM_NETWORK) {

		switch(wan_udp_pkt->wan_udp_command) {
			case READ_MODEM_STATUS:  
			case READ_STATION_STATUS:
			case CPIPE_ROUTER_UP_TIME:
			case READ_COMMS_ERR_STATISTICS:
			case READ_OPERATIONAL_STATISTICS:

			/* These two commands are executed for
			 * each request */
			case READ_SDLC_CONFIGURATION:
			case READ_CODE_VERSION:
				udp_mgmt_req_valid = 1;
				break;
			default:
				udp_mgmt_req_valid = 0;
				break;
		} 
	}
	
  	if(!udp_mgmt_req_valid) {

		/* set length to 0 */
		wan_udp_pkt->wan_udp_data_len = 0;

    		/* set return code */
		wan_udp_pkt->wan_udp_return_code = 0xCD;

		if (net_ratelimit()){	
			printk(KERN_INFO 
			"%s: Warning, Illegal UDP command attempted from network: %x\n",
			card->devname,wan_udp_pkt->wan_udp_command);
		}

   	} else {
		switch(wan_udp_pkt->wan_udp_command) {

			
		case CPIPE_ROUTER_UP_TIME:
			do_gettimeofday( &tv );
			sdlc_priv_area->router_up_time = tv.tv_sec - 
					sdlc_priv_area->router_start_time;
			*(unsigned long *)&wan_udp_pkt->wan_udp_data = 
					sdlc_priv_area->router_up_time;	
			mb->wan_data_len = sizeof(unsigned long);
			wan_udp_pkt->wan_udp_return_code = COMMAND_OK;
			break;

		default:
			/* it's a board command */
			mb->wan_command = wan_udp_pkt->wan_udp_command;
			mb->wan_data_len = wan_udp_pkt->wan_udp_data_len;
			if (mb->wan_data_len) {
				memcpy(&mb->wan_data, (unsigned char *) wan_udp_pkt->
							wan_udp_data, mb->wan_data_len);
	      		} 
			/* run the command on the board */
			err = card->hw_iface.cmd(card->hw, card->mbox_off, mb);
			if (err != COMMAND_OK) {
				break;
			}

			/* copy the result back to our buffer */
	         	memcpy(&wan_udp_pkt->wan_udp_hdr.wan_cmd, mb, sizeof(wan_cmd_t)); 
			
			if (mb->wan_data_len) {
	         		memcpy(&wan_udp_pkt->wan_udp_data, &mb->wan_data, 
								mb->wan_data_len); 
	      		}

		} /* end of switch */
     	} /* end of else */

     	/* Fill UDP TTL */
	wan_udp_pkt->wan_ip_ttl = card->wandev.ttl; 

	if (local_dev){
		
	}
	
	len = wan_reply_udp(card, sdlc_priv_area->udp_pkt_data, mb->wan_data_len);
	
     	if(sdlc_priv_area->udp_pkt_src == UDP_PKT_FRM_NETWORK) {
		if(!sdlc_send(card, sdlc_priv_area->udp_pkt_data, len)) {
			++ card->wandev.stats.tx_packets;
			card->wandev.stats.tx_bytes += len;
		}
	} else {	
	
		/* Pass it up the stack
    		   Allocate socket buffer */
		if ((new_skb = dev_alloc_skb(len)) != NULL) {
			/* copy data into new_skb */

 	    		buf = skb_put(new_skb, len);
  	    		memcpy(buf, sdlc_priv_area->udp_pkt_data, len);

            		/* Decapsulate pkt and pass it up the protocol stack */
	    		new_skb->protocol = htons(ETH_P_IP);
            		new_skb->dev = dev;
	    		wan_skb_reset_mac_header(new_skb);
	
			netif_rx(new_skb);
		} else {
	    	
			printk(KERN_INFO "%s: no socket buffers available!\n",
					card->devname);
  		}
    	}
 
	sdlc_priv_area->udp_pkt_lgth = 0;
 	
	return 0;
}

/*=============================================================================
 * Perform Interrupt Test by running READ_CHDLC_CODE_VERSION command MAX_INTR
 * _TEST_COUNTER times.
 */
static int intr_test( sdla_t* card)
{
	wan_mbox_t* mb = &card->wan_mbox;
	int err,i;

	card->timer_int_enabled = 0;

	/* The critical flag is unset because during intialization (if_open) 
	 * we want the interrupts to be enabled so that when the wpc_isr is
	 * called it does not exit due to critical flag set.
	 */ 

	err = sdlc_set_intr_mode(card, INTERRUPT_ON_COMMAND_COMPLETE);

	if (err == CMD_OK) { 
		for (i = 0; i < MAX_INTR_TEST_COUNTER; i ++) {	
			mb->wan_data_len  = 0;
			mb->wan_command = READ_CODE_VERSION;
			err = card->hw_iface.cmd(card->hw, card->mbox_off, mb);
		}
	}
	else {
		return err;
	}

	err = sdlc_set_intr_mode(card, 0);

	if (err != CMD_OK)
		return err;

	return 0;
}

/*============================================================================
 * Set PORT state.
 */
static void port_set_state (sdla_t *card, int state)
{
	netdevice_t *dev;
	sdlc_private_area_t *sdlc_priv_area;

	dev = WAN_DEVLE2DEV(WAN_LIST_FIRST(&card->wandev.dev_head));
	if (!dev || !dev->priv){
		return;
	}
	
	sdlc_priv_area=dev->priv;
		
        if (card->u.sdlc.state != state)
        {
                switch (state)
                {
                case WAN_CONNECTED:
                        printk (KERN_INFO "%s: SDLC comms enabled!\n",
                                card->devname);
                      break;

                case WAN_CONNECTING:
                        printk (KERN_INFO "%s: SDLC comms disabled!\n",
                                card->devname);
                        break;

                case WAN_DISCONNECTED:
                        printk (KERN_INFO "%s: SDLC comms disabled!\n",
                                card->devname);
                        break;
                }

                card->wandev.state = card->u.sdlc.state = state;
		sdlc_priv_area->common.state = state;
		wan_update_api_state(sdlc_priv_area);
        }
}

/*===========================================================================
 * config_sdlc
 *
 *	Configure the chdlc protocol and enable communications.		
 *
 *   	The if_open() function binds this function to the poll routine.
 *      Therefore, this function will run every time the chdlc interface
 *      is brought up. We cannot run this function from the if_open 
 *      because if_open does not have access to the remote IP address.
 *      
 *	If the communications are not enabled, proceed to configure
 *      the card and enable communications.
 *
 *      If the communications are enabled, it means that the interface
 *      was shutdown by ether the user or driver. In this case, we 
 *      have to check that the IP addresses have not changed.  If
 *      the IP addresses have changed, we have to reconfigure the firmware
 *      and update the changed IP addresses.  Otherwise, just exit.
 *
 */

static int config_sdlc (sdla_t *card)
{
	wan_mbox_t *mb=&card->wan_mbox;
	unsigned long timeout;
	int err=0;
	
	/* Set interrupt mode and mask */
        if (sdlc_set_intr_mode(card, INTERRUPT_ON_RX_FRAME |
                		INTERRUPT_ON_EXCEPTION_CONDITION |
                		INTERRUPT_ON_TIMER)){
		printk (KERN_INFO "%s: Failed to set interrupt triggers!\n",
				card->devname);
		return -EINVAL;	
        }
	

	/* Mask the Transmit and Timer interrupt */
	card->hw_iface.clear_bit(card->hw, card->intr_perm_off, (INTERRUPT_ON_TIMER));

	timeout=jiffies;
	while ((jiffies-timeout) < 2*HZ){
		err = sdlc_comm_enable(card);

		if (err == BUSY_WITH_BAUD_CALIBRATION){
			udelay(100);
			schedule();
			continue;
		}else{
			break;
		}
	}

	if (err !=0){
		printk(KERN_INFO "%s: Failed to enable sdlc communications!\n",
					card->devname);
		card->hw_iface.poke_byte(card->hw, card->intr_perm_off, 0x00);
		card->u.sdlc.comm_enabled=0;
		sdlc_set_intr_mode(card,0);
		return -EINVAL;
	}

	err = sdlc_read_config(card);
	if (err==0){	
		SDLC_CONFIGURATION_STRUCT *sdla_cfg = 
			(SDLC_CONFIGURATION_STRUCT *)mb->wan_data;
		printk(KERN_INFO "%s: Baud rate calibrated at %li bps\n",
				card->devname,sdla_cfg->baud_rate);
	}else{
		printk(KERN_INFO "%s: Failed to read configuration!\n",
				card->devname);
		card->hw_iface.poke_byte(card->hw, card->intr_perm_off, 0x00);
		sdlc_set_intr_mode(card,0);
		sdlc_comm_disable(card);
		return -EINVAL;
	}
	
	return 0; 
}

/*
 * ******************************************************************
 * Proc FS function 
 */
#define PROC_CFG_FRM	"%-15s| %-12s|\n"
#define PROC_STAT_FRM	"%-15s| %-12s| %-14s|\n"
static char chdlc_config_hdr[] =
	"Interface name | Device name |\n";
static char chdlc_status_hdr[] =
	"Interface name | Device name | Status        |\n";

static int chdlc_get_config_info(void* priv, struct seq_file* m, int* stop_cnt) 
{
	sdlc_private_area_t*	sdlc_priv_area = priv;
	sdla_t*			card = NULL;

	if (sdlc_priv_area == NULL)
		return m->count;
	card = sdlc_priv_area->card;

	if ((m->from == 0 && m->count == 0) || (m->from && m->from == *stop_cnt)){
		PROC_ADD_LINE(m,
			"%s", chdlc_config_hdr);
	}

	PROC_ADD_LINE(m,
		PROC_CFG_FRM, card->u.sdlc.if_name, card->devname);
	return m->count;
}

static int chdlc_get_status_info(void* priv, struct seq_file* m, int* stop_cnt)
{
	sdlc_private_area_t*	sdlc_priv_area = priv;
	sdla_t*			card = NULL;

	if (sdlc_priv_area == NULL)
		return m->count;
	card = sdlc_priv_area->card;

	if ((m->from == 0 && m->count == 0) || (m->from && m->from == *stop_cnt)){
		PROC_ADD_LINE(m,
			"%s", chdlc_status_hdr);
	}

	PROC_ADD_LINE(m,
		PROC_STAT_FRM, 
		card->u.sdlc.if_name, card->devname, STATE_DECODE(sdlc_priv_area->common.state));
	return m->count;
}

#define PROC_DEV_FR_S_FRM	"%-20s| %-14s|\n"
#define PROC_DEV_FR_D_FRM	"%-20s| %-14d|\n"
#define PROC_DEV_SEPARATE	"=====================================\n"


static int chdlc_set_dev_config(struct file *file, 
			     	const char *buffer,
			     	unsigned long count, 
			     	void *data)
{
	int 		cnt = 0;
	wan_device_t*	wandev = (void*)data;
	sdla_t*		card = NULL;

	if (wandev == NULL)
		return cnt;

	card = (sdla_t*)wandev->priv;

	printk(KERN_INFO "%s: New device config (%s)\n",
			wandev->name, buffer);
	/* Parse string */

	return count;
}


#define PROC_IF_FR_S_FRM	"%-30s\t%-14s\n"
#define PROC_IF_FR_D_FRM	"%-30s\t%-14d\n"
#define PROC_IF_FR_L_FRM	"%-30s\t%-14ld\n"
#define PROC_IF_SEPARATE	"====================================================\n"
	
static int chdlc_set_if_info(struct file *file, 
			     const char *buffer,
			     unsigned long count, 
			     void *data)
{
	netdevice_t*		dev = (void*)data;
	sdlc_private_area_t* 	sdlc_priv_area = NULL;
	sdla_t*			card = NULL;

	if (dev == NULL || dev->priv == NULL)
		return count;
	sdlc_priv_area = (sdlc_private_area_t*)dev->priv;
	if (sdlc_priv_area->card == NULL)
		return count;
	card = sdlc_priv_area->card;

	printk(KERN_INFO "%s: New interface config (%s)\n",
			card->u.sdlc.if_name, buffer);
	/* Parse string */

	return count;
}

int wanpipe_sdlc_register(netdevice_t *dev, void *reg_data)
{
	netdevice_t	*dev1;
	wp_sdlc_reg_t *wp_sdlc_reg = (wp_sdlc_reg_t *)reg_data;
	sdlc_private_area_t *chan;
	sdla_t *card=NULL;
	
	if (!dev || !wp_sdlc_reg){
		return -ENODEV;
	}

	if (dev->type != ARPHRD_SDLC){
		printk(KERN_INFO "%s: Device %s type not SDLC! type=%i\n",
				card->devname,dev->name,dev->type);
		return -EINVAL;
	}
	
	if ((chan=dev->priv) == NULL){
		return -ENODEV;
	}

	if ((card=chan->card) == NULL){
		return -ENODEV;	
	}

	dev1 = WAN_DEVLE2DEV(WAN_LIST_FIRST(&card->wandev.dev_head));
	if (dev != dev1){
		return -EINVAL;
	}

	if (chan->common.usedby != STACK){
		printk(KERN_INFO "%s: Device %s doesn't support registration!\n",
				card->devname,dev->name);
		return -EINVAL;
	}
	
	if (test_bit(REG_CRIT,&card->wandev.critical)){
		printk(KERN_INFO "%s: Device %s already registered!\n",
				card->devname,dev->name);
		return -EBUSY;
	}

	memcpy(&chan->wp_sdlc_register,wp_sdlc_reg,sizeof(wp_sdlc_reg_t));
	set_bit(REG_CRIT,&card->wandev.critical);
	
	printk(KERN_INFO "%s: Registering sdlc device %s \n",
				card->devname,dev->name);
	
	return 0;
}

int wanpipe_sdlc_unregister(netdevice_t *dev)
{
	netdevice_t	*dev1;
	sdlc_private_area_t *chan;
	sdla_t *card;
	
	if (!dev){
		return -ENODEV;
	}

	if ((chan=dev->priv) == NULL){
		return -ENODEV;
	}

	if ((card=chan->card) == NULL){
		return -ENODEV;	
	}

	dev1 = WAN_DEVLE2DEV(WAN_LIST_FIRST(&card->wandev.dev_head));
	if (dev != dev1){
		return -EINVAL;
	}

	if (chan->common.usedby != STACK){
		printk(KERN_INFO "%s: Device %s doesn't support un-registration!\n",
				card->devname,dev->name);
		return -EINVAL;
	}
	
	if (test_bit(REG_CRIT,&card->wandev.critical)){
		clear_bit(REG_CRIT,&card->wandev.critical);

		printk(KERN_INFO "%s: Unregistering sdlc device %s \n",
				card->devname,dev->name);
	}

	return 0;
}

void send_oob_msg(sdla_t *card, wan_mbox_t *mb)
{	
	struct sk_buff *skb;
	netdevice_t *dev;
	sdlc_private_area_t *chan;
	wan_api_t *wan_api;
	
	dev = WAN_DEVLE2DEV(WAN_LIST_FIRST(&card->wandev.dev_head));
	if (!dev || !wan_netif_priv(dev))
		return;
	chan = wan_netif_priv(dev);
	if (chan->common.usedby == WANPIPE){
		return;
	}
	
	skb=dev_alloc_skb(sizeof(wan_api_hdr_t)+mb->wan_data_len+10);
	if (!skb){
		printk(KERN_INFO "%s: OOB msg failed: faild to allocate skb!\n",
				card->devname);
		return;
	}
	
	wan_api=(wan_api_t *)skb_put(skb,sizeof(wan_api_hdr_t)+mb->wan_data_len);
	wan_api->wan_api_pktType = mb->wan_command;
	wan_api->wan_api_length=mb->wan_data_len;
	wan_api->wan_api_sdlc_station = 0;

	memcpy(wan_api->data,mb->wan_data,mb->wan_data_len);

	skb->dev=dev;
	skb->pkt_type = WAN_PACKET_CMD;	
	
	if (chan->common.usedby == API){

		if (chan->common.sk == NULL){
			++card->wandev.stats.rx_dropped;
			dev_kfree_skb_any(skb);
		}

		if (wan_api_rx(chan,skb) != 0){
			++card->wandev.stats.rx_dropped;
			dev_kfree_skb_any(skb);
		}

	}else if (chan->common.usedby == STACK) {

		if (!test_bit(REG_CRIT,&card->wandev.critical) ||
		    !chan->wp_sdlc_register.sdlc_stack_rx){

			++card->wandev.stats.rx_dropped;
			dev_kfree_skb_any(skb);
		}

		skb->dev = dev;
		wan_skb_reset_mac_header(skb);
				
		if (chan->wp_sdlc_register.sdlc_stack_rx(skb) != 0){
			++card->wandev.stats.rx_dropped;
			dev_kfree_skb_any(skb);
		}
	}	
}

/****** End ****************************************************************/
