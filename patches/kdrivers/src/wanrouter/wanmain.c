/*****************************************************************************
* wanmain.c	WAN Multiprotocol Router Module. Main code.
*
*		This module is completely hardware-independent and provides
*		the following common services for the WAN Link Drivers:
*		 o WAN device managenment (registering, unregistering)
*		 o Network interface management
*		 o Physical connection management (dial-up, incoming calls)
*		 o Logical connection management (switched virtual circuits)
*		 o Protocol encapsulation/decapsulation

* Author:	Gideon Hack	
* 		Nenad Corbic
*
* Copyright:	(c) 1995-2003 Sangoma Technologies Inc.
*
*		This program is free software; you can redistribute it and/or
*		modify it under the terms of the GNU General Public License
*		as published by the Free Software Foundation; either version
*		2 of the License, or (at your option) any later version.
* ============================================================================
* May 13, 2002	Nenad Corbic	Updated for ADSL drivers
* Nov 24, 2000  Nenad Corbic	Updated for 2.4.X kernels 
* Nov 07, 2000  Nenad Corbic	Fixed the Mulit-Port PPP for kernels 2.2.16 and
*  				greater.
* Aug 2,  2000  Nenad Corbic	Block the Multi-Port PPP from running on
*  			        kernels 2.2.16 or greater.  The SyncPPP 
*  			        has changed.
* Jul 13, 2000  Nenad Corbic	Added SyncPPP support
* 				Added extra debugging in wan_device_setup().
* Oct 01, 1999  Gideon Hack     Update for s514 PCI card
* Dec 27, 1996	Gene Kozin	Initial version (based on Sangoma's WANPIPE)
* Jan 16, 1997	Gene Kozin	router_devlist made public
* Jan 31, 1997  Alan Cox	Hacked it about a bit for 2.1
* Jun 27, 1997  Alan Cox	realigned with vendor code
* Oct 15, 1997  Farhan Thawar   changed wan_encapsulate to add a pad byte of 0
* Apr 20, 1998	Alan Cox	Fixed 2.1 symbols
* May 17, 1998  K. Baranowski	Fixed SNAP encapsulation in wan_encapsulate
* Dec 15, 1998  Arnaldo Melo    support for firmwares of up to 128000 bytes
*                               check wandev->setup return value
* Dec 22, 1998  Arnaldo Melo    vmalloc/vfree used in wan_device_setup to allocate
*                               kernel memory and copy configuration data to
*                               kernel space (for big firmwares)
* Jun 02, 1999  Gideon Hack	Updates for Linux 2.0.X and 2.2.X kernels.	
*****************************************************************************/

#define _K22X_MODULE_FIX_

#include "wanpipe_includes.h"
#include "wanpipe_defines.h"
#include "wanpipe_debug.h"
#include "wanpipe_common.h"
#include "wanpipe_iface.h"
#include "wanpipe_wanrouter.h"	/* WAN router API definitions */
#include "wanpipe.h"	/* WAN router API definitions */
#include "if_wanpipe.h"

#include <linux/wanpipe_lapb_kernel.h>
#include <linux/wanpipe_x25_kernel.h>
#include <linux/wanpipe_dsp_kernel.h>
#include <linux/wanpipe_lip_kernel.h>
#include <linux/wanpipe_ec_kernel.h>

#define KMEM_SAFETYZONE 8

/***********FOR DEBUGGING PURPOSES*********************************************
static void * dbg_kmalloc(unsigned int size, int prio, int line) {
	int i = 0;
	void * v = kmalloc(size+sizeof(unsigned int)+2*KMEM_SAFETYZONE*8,prio);
	char * c1 = v;	
	c1 += sizeof(unsigned int);
	*((unsigned int *)v) = size;

	for (i = 0; i < KMEM_SAFETYZONE; i++) {
		c1[0] = 'D'; c1[1] = 'E'; c1[2] = 'A'; c1[3] = 'D';
		c1[4] = 'B'; c1[5] = 'E'; c1[6] = 'E'; c1[7] = 'F';
		c1 += 8;
	}
	c1 += size;
	for (i = 0; i < KMEM_SAFETYZONE; i++) {
		c1[0] = 'M'; c1[1] = 'U'; c1[2] = 'N'; c1[3] = 'G';
		c1[4] = 'W'; c1[5] = 'A'; c1[6] = 'L'; c1[7] = 'L';
		c1 += 8;
	}
	v = ((char *)v) + sizeof(unsigned int) + KMEM_SAFETYZONE*8;
	printk(KERN_INFO "line %d  kmalloc(%d,%d) = %p\n",line,size,prio,v);
	return v;
}
static void dbg_kfree(void * v, int line) {
	unsigned int * sp = (unsigned int *)(((char *)v) - (sizeof(unsigned int) + KMEM_SAFETYZONE*8));
	unsigned int size = *sp;
	char * c1 = ((char *)v) - KMEM_SAFETYZONE*8;
	int i = 0;
	for (i = 0; i < KMEM_SAFETYZONE; i++) {
		if (   c1[0] != 'D' || c1[1] != 'E' || c1[2] != 'A' || c1[3] != 'D'
		    || c1[4] != 'B' || c1[5] != 'E' || c1[6] != 'E' || c1[7] != 'F') {
			printk(KERN_INFO "kmalloced block at %p has been corrupted (underrun)!\n",v);
			printk(KERN_INFO " %4x: %2x %2x %2x %2x %2x %2x %2x %2x\n", i*8,
			                c1[0],c1[1],c1[2],c1[3],c1[4],c1[5],c1[6],c1[7] );
		}
		c1 += 8;
	}
	c1 += size;
	for (i = 0; i < KMEM_SAFETYZONE; i++) {
		if (   c1[0] != 'M' || c1[1] != 'U' || c1[2] != 'N' || c1[3] != 'G'
		    || c1[4] != 'W' || c1[5] != 'A' || c1[6] != 'L' || c1[7] != 'L'
		   ) {
			printk(KERN_INFO "kmalloced block at %p has been corrupted (overrun):\n",v);
			printk(KERN_INFO " %4x: %2x %2x %2x %2x %2x %2x %2x %2x\n", i*8,
			                c1[0],c1[1],c1[2],c1[3],c1[4],c1[5],c1[6],c1[7] );
		}
		c1 += 8;
	}
	printk(KERN_INFO "line %d  kfree(%p)\n",line,v);
	v = ((char *)v) - (sizeof(unsigned int) + KMEM_SAFETYZONE*8);
	kfree(v);
}

#define kmalloc(x,y) dbg_kmalloc(x,y,__LINE__)
#define kfree(x) dbg_kfree(x,__LINE__)
*****************************************************************************/


/*
 * 	Defines and Macros 
 */

#ifndef	wp_min
#define wp_min(a,b) (((a)<(b))?(a):(b))
#endif
#ifndef	wp_max
#define wp_max(a,b) (((a)>(b))?(a):(b))
#endif

/*
 * 	Function Prototypes 
 */

/* 
 * 	Kernel loadable module interface.
 */
#ifdef MODULE
MODULE_AUTHOR ("Nenad Corbic <ncorbic@sangoma.com>");
MODULE_DESCRIPTION ("Sangoma WANPIPE: Proc & User Interface");

#if LINUX_VERSION_CODE > KERNEL_VERSION(2,4,9) 
MODULE_LICENSE("GPL");
#endif
#endif

/* 
 *	WAN device IOCTL handlers 
 */

int wan_device_setup(wan_device_t *wandev, wandev_conf_t *u_conf, int user);
static int wan_device_stat(wan_device_t *wandev, wandev_stat_t *u_stat);
int wan_device_shutdown(wan_device_t *wandev, wandev_conf_t *u_conf);
int wan_device_new_if(wan_device_t *wandev, wanif_conf_t *u_conf, int user);
static int wan_device_del_if(wan_device_t *wandev, char *u_name);
static int wan_device_debugging(wan_device_t *wandev);

static int wan_device_new_if_lip(wan_device_t *wandev, wanif_conf_t *u_conf);
static int wan_device_del_if_lip(wan_device_t *wandev, netdevice_t *dev);

/* WAN Annexg IOCLT handlers */

#ifdef CONFIG_PRODUCT_WANPIPE_ANNEXG
static int wan_device_new_if_lapb(wan_device_t *wandev, wanif_conf_t *u_conf);
static int wan_device_del_if_lapb(wan_device_t *wandev, netdevice_t *dev);

static int wan_device_new_if_x25(wan_device_t *wandev, wanif_conf_t *u_conf);
static int wan_device_del_if_x25(wan_device_t *wandev, netdevice_t *dev);

static int wan_device_new_if_dsp(wan_device_t *wandev, wanif_conf_t *u_conf);
static int wan_device_del_if_dsp(wan_device_t *wandev, netdevice_t *dev);
#endif

static int wan_device_unreg_lip(netdevice_t *dev);

/* 
 *	Miscellaneous 
 */

wan_device_t *wan_find_wandev_device (char *name);
static int delete_interface (wan_device_t *wandev, netdevice_t	*dev, int force);

/*
 *	Global Data
 */

static char fullname[]		= "WANPIPE(tm) Interface Support Module";
static char modname[]		= ROUTER_NAME;	/* short module name */
wan_spinlock_t          wan_devlist_lock;
struct wan_devlist_	wan_devlist = 
	WAN_LIST_HEAD_INITIALIZER(wan_devlist); 
//wan_device_t* router_devlist 	= NULL;	/* list of registered devices */
static int devcnt 		= 0;
static unsigned char wan_version[100]; 


/* 
 *	Organize Unique Identifiers for encapsulation/decapsulation 
 */

static unsigned char oui_ether[] = { 0x00, 0x00, 0x00 };
#if 0
static unsigned char oui_802_2[] = { 0x00, 0x80, 0xC2 };
#endif


struct wanpipe_api_register_struct api_socket;
struct wanpipe_lapb_register_struct lapb_protocol;
struct wanpipe_x25_register_struct x25_protocol;
struct wanpipe_dsp_register_struct dsp_protocol;
struct wanpipe_fw_register_struct  wp_fw_protocol;
struct wplip_reg		   wplip_protocol;

#if defined(CONFIG_WANPIPE_HWEC)
struct wanec_iface_		   wanec_iface;
#endif

/*
 *	Kernel Loadable Module Entry Points
 */

/*
 * 	Module 'insert' entry point.
 * 	o print announcement
 * 	o initialize static data
 * 	o create /proc/net/wanrouter directory and static entries
 *
 * 	Return:	0	Ok
 *		< 0	error.
 * 	Context:	process
 */

int __init wanrouter_init (void)
{
	int err;

   	DEBUG_EVENT("%s %s.%s %s %s\n",
			fullname, WANPIPE_VERSION, WANPIPE_SUB_VERSION,
			WANPIPE_COPYRIGHT_DATES,WANPIPE_COMPANY);
   	sprintf(wan_version,"%s.%s",
				WANPIPE_VERSION, WANPIPE_SUB_VERSION);

	WAN_LIST_INIT(&wan_devlist);
        wan_spin_lock_init(&wan_devlist_lock,"wan_devlist_lock");

	err = wanrouter_proc_init();
	
	if (err){ 
		printk(KERN_INFO
		"%s: can't create entry in proc filesystem!\n", modname);
	}

	wanpipe_wandev_create();

#ifdef CONFIG_PRODUCT_WANPIPE_ANNEXG
	UNREG_PROTOCOL_FUNC(dsp_protocol);
	UNREG_PROTOCOL_FUNC(x25_protocol);
	UNREG_PROTOCOL_FUNC(api_socket);
	UNREG_PROTOCOL_FUNC(lapb_protocol);
#endif
	UNREG_PROTOCOL_FUNC(wplip_protocol);
#if defined(CONFIG_WANPIPE_HWEC)
	UNREG_PROTOCOL_FUNC(wanec_iface);
#endif
	
	return err;
}

/*
 * 	Module 'remove' entry point.
 * 	o delete /proc/net/wanrouter directory and static entries.
 */

void __exit wanrouter_exit (void)
{
#if defined(CONFIG_WANPIPE_HWEC)
	UNREG_PROTOCOL_FUNC(wanec_iface);
#endif

#ifdef CONFIG_PRODUCT_WANPIPE_ANNEXG
	UNREG_PROTOCOL_FUNC(dsp_protocol);
	UNREG_PROTOCOL_FUNC(x25_protocol);
	UNREG_PROTOCOL_FUNC(api_socket);
	UNREG_PROTOCOL_FUNC(lapb_protocol);
#endif
	UNREG_PROTOCOL_FUNC(wp_fw_protocol);
	wanrouter_proc_cleanup();
	wanpipe_wandev_free();
}

module_init(wanrouter_init);
module_exit(wanrouter_exit);

/*
 * 	Kernel APIs
 */

/*
 * 	Register WAN device.
 * 	o verify device credentials
 * 	o create an entry for the device in the /proc/net/wanrouter directory
 * 	o initialize internally maintained fields of the wan_device structure
 * 	o link device data space to a singly-linked list
 * 	o if it's the first device, then start kernel 'thread'
 * 	o increment module use count
 *
 * 	Return:
 *	0	Ok
 *	< 0	error.
 *
 * 	Context:	process
 */


int register_wan_device(wan_device_t *wandev)
{
	int err, namelen;
	wan_smp_flag_t smp_flags;

	if ((wandev == NULL) || (wandev->magic != ROUTER_MAGIC) ||
	    (wandev->name == NULL))
		return -EINVAL;
 		
	namelen = strlen(wandev->name);
	if (!namelen || (namelen > WAN_DRVNAME_SZ))
		return -EINVAL;
		
	if (wan_find_wandev_device(wandev->name) != NULL)
		return -EEXIST;

#ifdef WANDEBUG		
	printk(KERN_INFO "%s: registering WAN device %s\n",
		modname, wandev->name);
#endif

	/*
	 *	Register /proc directory entry 
	 */
	err = wanrouter_proc_add(wandev);
	if (err) {
		printk(KERN_INFO
			"%s: can't create /proc/net/wanrouter/%s entry!\n",
			modname, wandev->name);
		return err;
	}

	/*
	 *	Initialize fields of the wan_device structure maintained by the
	 *	router and update local data.
	 */
	 
	wandev->ndev = 0;
	WAN_LIST_INIT(&wandev->dev_head);
	wan_spin_lock_init(&wandev->dev_head_lock, "wan_dev_head_lock");

	wan_spin_lock(&wan_devlist_lock,&smp_flags);
	WAN_LIST_INSERT_HEAD(&wan_devlist, wandev, next);
	wan_spin_unlock(&wan_devlist_lock,&smp_flags);

	++devcnt;

    MOD_INC_USE_COUNT;	/* prevent module from unloading */
	return 0;
}

/*
 *	Unregister WAN device.
 *	o shut down device
 *	o unlink device data space from the linked list
 *	o delete device entry in the /proc/net/wanrouter directory
 *	o decrement module use count
 *
 *	Return:		0	Ok
 *			<0	error.
 *	Context:	process
 */


int unregister_wan_device(char *name)
{
	int err;

	wan_device_t *wandev;
	wan_smp_flag_t smp_flags;

	if (name == NULL)
		return -EINVAL;

	
	WAN_LIST_FOREACH(wandev, &wan_devlist, next){
		if (!strcmp(wandev->name, name)){
			break;
		}
	}

	if (wandev == NULL)
		return -ENODEV;

#ifdef WANDEBUG		
	printk(KERN_INFO "%s: unregistering WAN device %s\n", modname, name);
#endif

	if (wandev->state != WAN_UNCONFIGURED) {
		err = wan_device_shutdown(wandev, NULL);
		if (err) return err;
	}
	
	wan_spin_lock(&wan_devlist_lock,&smp_flags);
	WAN_LIST_REMOVE(wandev, next);
	wan_spin_unlock(&wan_devlist_lock,&smp_flags);

	if (wandev->port_cfg) {
		wan_free(wandev->port_cfg);
		wandev->port_cfg=NULL;
	}

	--devcnt;
	wanrouter_proc_delete(wandev);
    MOD_DEC_USE_COUNT;
	return 0;
}

/*
 *	Encapsulate packet.
 *
 *	Return:	encapsulation header size
 *		< 0	- unsupported Ethertype
 *
 *	Notes:
 *	1. This function may be called on interrupt context.
 */


int wanrouter_encapsulate (struct sk_buff *skb, netdevice_t *dev,
	unsigned short type)
{
	int hdr_len = 0;

	switch (type) {
	case ETH_P_IP:		/* IP datagram encapsulation */
		hdr_len += 1;
		skb_push(skb, 1);
		skb->data[0] = NLPID_IP;
		break;

	case ETH_P_IPX:		/* SNAP encapsulation */
	case ETH_P_ARP:
		hdr_len += 7;
		skb_push(skb, 7);
		skb->data[0] = 0;
		skb->data[1] = NLPID_SNAP;
		memcpy(&skb->data[2], oui_ether, sizeof(oui_ether));
		*((unsigned short*)&skb->data[5]) = htons(type);
		break;

	default:		/* Unknown packet type */
		printk(KERN_INFO
			"%s: unsupported Ethertype 0x%04X on interface %s!\n",
			modname, type, dev->name);
		hdr_len = -EINVAL;
	}
	return hdr_len;
}



/*
 *	Decapsulate packet.
 *
 *	Return:	Ethertype (in network order)
 *			0	unknown encapsulation
 *
 *	Notes:
 *	1. This function may be called on interrupt context.
 */


unsigned short wanrouter_type_trans (struct sk_buff *skb, netdevice_t *dev)
{
	int cnt = skb->data[0] ? 0 : 1;	/* there may be a pad present */
	unsigned short ethertype;

	switch (skb->data[cnt]) {
	
	case CISCO_IP:
	case NLPID_IP:		/* IP datagramm */
		ethertype = htons(ETH_P_IP);
		cnt += 1;
		break;

        case NLPID_SNAP:	/* SNAP encapsulation */
		if (memcmp(&skb->data[cnt + 1], oui_ether, sizeof(oui_ether))){
          		printk(KERN_INFO
				"%s: unsupported SNAP OUI %02X-%02X-%02X "
				"on interface %s!\n", modname,
				skb->data[cnt+1], skb->data[cnt+2],
				skb->data[cnt+3], dev->name);
			return 0;
		}	
		ethertype = *((unsigned short*)&skb->data[cnt+4]);
		cnt += 6;
		break;

	/* add other protocols, e.g. CLNP, ESIS, ISIS, if needed */

	default:
		printk(KERN_INFO
			"%s: unsupported NLPID 0x%02X on interface %s!\n",
			modname, skb->data[cnt], dev->name);
		return 0;
	}
	skb->protocol = ethertype;
	skb->pkt_type = PACKET_HOST;	/*	Physically point to point */
	skb_pull(skb, cnt);
	wan_skb_reset_mac_header(skb);
	return ethertype;
}


/*
 *	WAN device IOCTL.
 *	o find WAN device associated with this node
 *	o execute requested action or pass command to the device driver
 */
WAN_IOCTL_RET_TYPE WANDEF_IOCTL_FUNC(wanrouter_ioctl, struct file *file, unsigned int cmd, unsigned long arg)
{
	WAN_IOCTL_RET_TYPE err = 0;
	wan_device_t *wandev;

#ifdef HAVE_UNLOCKED_IOCTL
	struct inode *inode = file->f_dentry->d_inode;
#endif
	
	if ((cmd>>8) != ROUTER_IOCTL){
		DEBUG_EVENT("%s: Invalid CMD=0x%X ROUTER_IOCTL=0x%X\n",
				__FUNCTION__,cmd>>8,ROUTER_IOCTL);
		return -EINVAL;
	}
		
	wandev = WP_PDE_DATA(inode);
	if (!wandev) {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,18)
		DEBUG_EVENT("%s: Invalid inode data\n",
				__FUNCTION__);
#else
		DEBUG_EVENT("%s: Invalid inode data %p\n",
				__FUNCTION__,inode->u.generic_ip);
#endif		
		return -EINVAL;
	}
	if (wandev->magic != ROUTER_MAGIC){
		DEBUG_EVENT("%s: Invalid wandev Magic Number\n",
				__FUNCTION__);
		return -EINVAL;
	}
	
	if (wandev->config_id != WANCONFIG_EDUKIT){
		ADMIN_CHECK();
	}

	switch (cmd) {
	case ROUTER_SETUP:
		ADMIN_CHECK();
		err = wan_device_setup(wandev, (void*)arg, 1);
		break;

	case ROUTER_DOWN:
		ADMIN_CHECK();
		err = wan_device_shutdown(wandev, (void*)arg);
		break;

	case ROUTER_STAT:
		ADMIN_CHECK();		
		err = wan_device_stat(wandev, (void*)arg);
		break;

	case ROUTER_IFNEW:
		ADMIN_CHECK();
		err = wan_device_new_if(wandev, (void*)arg, 1);
		break;

	case ROUTER_IFDEL:
		ADMIN_CHECK();
		err = wan_device_del_if(wandev, (void*)arg);
		break;
		
#ifdef CONFIG_PRODUCT_WANPIPE_ANNEXG
	case ROUTER_IFNEW_LAPB:
		err = wan_device_new_if_lapb(wandev,(void*)arg);
		break;
		
	case ROUTER_IFNEW_X25:
		err = wan_device_new_if_x25(wandev,(void*)arg);
		break;

	case ROUTER_IFNEW_DSP:
		err = wan_device_new_if_dsp(wandev,(void*)arg);
		break;
#endif

	case ROUTER_IFNEW_LIP:
		err = wan_device_new_if_lip(wandev,(void*)arg);
		break;

	case ROUTER_IFDEL_LIP:
		err = wan_device_del_if_lip(wandev,(void*)arg);
		break;
		
	case ROUTER_IFSTAT:
		break;

	case ROUTER_VER:
		err=copy_to_user((char*)arg,wan_version,strlen(wan_version));
		break;

	case ROUTER_DEBUGGING:
		ADMIN_CHECK();
		err = wan_device_debugging(wandev);
		break;

	case ROUTER_DEBUG_READ:
		ADMIN_CHECK();
		if (wandev->debug_read){
			err = wandev->debug_read(wandev, (void*)arg);
		}else{
			err = -EINVAL;
		}
		break;

	default:
		if ((cmd >= ROUTER_USER) &&
		    (cmd <= ROUTER_USER_MAX) &&
		    wandev->ioctl)
			err = wandev->ioctl(wandev, cmd, arg);
		else err = -EINVAL;
	}
	return err;
}

/*
 *	WAN Driver IOCTL Handlers
 */

/*
 *	Setup WAN link device.
 *	o verify user address space
 *	o allocate kernel memory and copy configuration data to kernel space
 *	o if configuration data includes extension, copy it to kernel space too
 *	o call driver's setup() entry point
 */

int wan_device_setup (wan_device_t *wandev, wandev_conf_t *u_conf, int user)
{
	void *data = NULL;
	wandev_conf_t *conf;
	int err = -EINVAL;

	if (wandev->setup == NULL){	/* Nothing to do ? */
		printk(KERN_INFO "%s: ERROR, No setup script: wandev->setup()\n",
				wandev->name);
		return 0;
	}

	conf = wan_malloc(sizeof(wandev_conf_t));
	if (conf == NULL){
		printk(KERN_INFO "%s: ERROR, Failed to allocate kernel memory !\n",
				wandev->name);
		return -ENOBUFS;
	}

	if (user) {
		if(copy_from_user(conf, u_conf, sizeof(wandev_conf_t))) {
			printk(KERN_INFO "%s: Failed to copy user config data to kernel space!\n",
					wandev->name);
	
			DEBUG_SUB_MEM(sizeof(wandev_conf_t));
			wan_free(conf);
			return -EFAULT;
		}
	} else {
		memcpy(conf,u_conf,sizeof(wandev_conf_t));
	}

	if (!wandev->port_cfg) {
		wandev->port_cfg=wan_kmalloc(sizeof(wanpipe_port_cfg_t));
		if (!wandev->port_cfg) {
			return -ENOMEM;
		}
		memset(wandev->port_cfg,0,sizeof(wanpipe_port_cfg_t));
	}

	memcpy(&((wanpipe_port_cfg_t*)wandev->port_cfg)->wandev_conf, conf, sizeof(wandev_conf_t));
	
	if (conf->magic != ROUTER_MAGIC){
		DEBUG_SUB_MEM(sizeof(wandev_conf_t));
		wan_free(conf);
		printk(KERN_INFO "%s: ERROR, Invalid MAGIC Number\n",
				wandev->name);
	        return -EINVAL; 
	}

	if (conf->card_type == WANOPT_ADSL ||
 	    conf->card_type == WANOPT_AFT  || 
 	    conf->card_type == WANOPT_AFT_GSM  || 
 	    conf->card_type == WANOPT_T116  || 
 	    conf->card_type == WANOPT_AFT104  || 
 	    conf->card_type == WANOPT_AFT300  || 
	    conf->config_id == WANCONFIG_DEBUG){
		conf->data=NULL;
		conf->data_size=0;
		err = wandev->setup(wandev, conf);
		
	}else if (conf->data_size && conf->data){
		if(conf->data_size > 128000 || conf->data_size < 0) {
			DEBUG_SUB_MEM(sizeof(wandev_conf_t));
			wan_free(conf);
			printk(KERN_INFO 
			    "%s: ERROR, Invalid firmware data size %i !\n",
					wandev->name, conf->data_size);
		        return -EINVAL;;
		}

		data = vmalloc(conf->data_size);
		if (data) {
			if(!copy_from_user(data, conf->data, conf->data_size)){
				conf->data=data;
				err = wandev->setup(wandev,conf);
			}else{ 
				printk(KERN_INFO 
				     "%s: ERROR, Faild to copy from user data !\n",
				       wandev->name);
				err = -EFAULT;
			}
		}else{ 
			printk(KERN_INFO 
			 	"%s: ERROR, Faild allocate kernel memory !\n",
				wandev->name);
			err = -ENOBUFS;
		}
			
		if (data){
			vfree(data);
		}
	}else{
		/* No Firmware found. This is ok for ADSL cards or when we are
		 * operating in DEBUG mode */
		
		printk(KERN_INFO 
			    "%s: ERROR, No firmware found ! Firmware size = %i !\n",
					wandev->name, conf->data_size);
	}

	DEBUG_SUB_MEM(sizeof(wandev_conf_t));
	wan_free(conf);
	return err;
}

/*
 *	Shutdown WAN device.
 *	o delete all not opened logical channels for this device
 *	o call driver's shutdown() entry point
 */
 
int wan_device_shutdown (wan_device_t *wandev, wandev_conf_t *u_conf)
{
	struct wan_dev_le	*devle;
	netdevice_t		*dev;
	wandev_conf_t *conf = NULL;
	int err=0;
	int force=0;
	wan_smp_flag_t smp_flags;
		
	if (wandev->state == WAN_UNCONFIGURED){
		return 0;
	}
	
	if (u_conf){
		conf = wan_malloc(sizeof(wandev_conf_t));
		if (conf == NULL){
			printk(KERN_INFO "%s: ERROR, Failed to allocate kernel memory !\n",
					wandev->name);
			return -ENOBUFS;
		}
		
		if(copy_from_user(conf, u_conf, sizeof(wandev_conf_t))) {
			printk(KERN_INFO "%s: Failed to copy user config data to kernel space!\n",
					wandev->name);
			DEBUG_SUB_MEM(sizeof(wandev_conf_t));
			wan_free(conf);
			return -EFAULT;
		}
	}else{
		/* Module is being unloaded we have no choice here */
		force=1;
	}
	
	printk(KERN_INFO "\n");
	printk(KERN_INFO "%s: Shutting Down!\n",wandev->name);
		
	devle = WAN_LIST_FIRST(&wandev->dev_head);
	
	while(devle){

		dev = WAN_DEVLE2DEV(devle);
		if ((err=delete_interface(wandev, dev, force)) != 0){
			printk(KERN_ERR "shutdown failed with error %d\n", err);
			return err;
		}

		wan_spin_lock(&wandev->dev_head_lock,&smp_flags);
		WAN_LIST_REMOVE(devle, dev_link);
		--wandev->ndev;
		wan_spin_unlock(&wandev->dev_head_lock,&smp_flags);

		wan_free(devle);
		/* the next interface is alwast first */
		devle = WAN_LIST_FIRST(&wandev->dev_head);

	}
	
	if (wandev->ndev){
		if (conf){
			DEBUG_SUB_MEM(sizeof(wandev_conf_t));
			wan_free(conf);
		}
		printk(KERN_ERR "cannot shutdown: one or more interfaces are still open\n");

		return -EBUSY;	/* there are opened interfaces  */
	}	
	
	if (wandev->shutdown)
		err=wandev->shutdown(wandev, conf);

	if (u_conf){
		if(copy_to_user(u_conf, conf, sizeof(wandev_conf_t)))
			return -EFAULT;

		DEBUG_SUB_MEM(sizeof(wandev_conf_t));
		wan_free(conf);
	}
	return err;
}

/*
 *	Get WAN device status & statistics.
 */

static int wan_device_stat (wan_device_t *wandev, wandev_stat_t *u_stat)
{
	wandev_stat_t stat;

	memset(&stat, 0, sizeof(stat));

	/* Ask device driver to update device statistics */
	if ((wandev->state != WAN_UNCONFIGURED) && wandev->update)
		wandev->update(wandev);

	/* Fill out structure */
	stat.ndev  = wandev->ndev;
	stat.state = wandev->state;

	if(copy_to_user(u_stat, &stat, sizeof(stat)))
		return -EFAULT;

	return 0;
}

/*
 *	Create new WAN interface.
 *	o verify user address space
 *	o copy configuration data to kernel address space
 *	o allocate network interface data space
 *	o call driver's new_if() entry point
 *	o make sure there is no interface name conflict
 *	o register network interface
 */
int wan_device_new_if (wan_device_t *wandev, wanif_conf_t *u_conf, int user)
{
	struct wan_dev_le	*devle;	
	wanif_conf_t *conf=NULL;
	netdevice_t  *dev=NULL;
	int err=-EINVAL;
	wan_smp_flag_t smp_flags;

	if ((wandev->state == WAN_UNCONFIGURED) || (wandev->new_if == NULL)) {
		DEBUG_EVENT("%s: Error wandev state is not configured or new_if=NULL!\n",__FUNCTION__);
		return -ENODEV;
	}

	conf = wan_malloc(sizeof(wanif_conf_t));
	if (!conf){
		DEBUG_EVENT("%s: Error: Failed to alloc memory\n",__FUNCTION__);
		return -ENOMEM;
	}

	if (user) {
		if(copy_from_user(conf, u_conf, sizeof(wanif_conf_t))){
			err=-EFAULT;
			DEBUG_EVENT("%s: Error: Failed to copy from user\n",__FUNCTION__);
			goto wan_device_new_if_exit;
		}
	} else {
		memcpy(conf, u_conf, sizeof(wanif_conf_t));
	}

	if (conf->magic != ROUTER_MAGIC){
		err=-EINVAL;
		DEBUG_EVENT("%s: Error: Config magic is wrong!\n",__FUNCTION__);
		goto wan_device_new_if_exit;
	}
       

	if ((dev=wan_dev_get_by_name(conf->name))){
		dev_put(dev);
		dev=NULL;
	       	err = -EEXIST;	/* name already exists */
		DEBUG_EVENT("%s: Device %s already exists\n",
				wandev->name,conf->name);
		goto wan_device_new_if_exit;
	}
	
	dev=wan_netif_alloc(conf->name, WAN_IFT_OTHER, &err);
	if (dev == NULL){
		DEBUG_EVENT("%s: Error: failed to allocate net device!\n",__FUNCTION__);
		goto wan_device_new_if_exit;
	}
	wan_netif_set_priv(dev, NULL);

	devle = wan_malloc(sizeof(struct wan_dev_le));
	if (devle == NULL){
		wan_free(dev);
		DEBUG_EVENT("%s: Error: failed to allocate dev element!\n",__FUNCTION__);
		goto wan_device_new_if_exit;
	}
	
	err = wandev->new_if(wandev, dev, conf);
	
	if (!err) {
		netdevice_t *tmp_dev;
		/* Register network interface. This will invoke init()
		 * function supplied by the driver.  If device registered
		 * successfully, add it to the interface list.
		 */

		if (dev->name == NULL){
			DEBUG_EVENT("%s: Error: device name is null!\n",__FUNCTION__);
			err = -EINVAL;
		}else if ((tmp_dev=wan_dev_get_by_name(dev->name))){
			DEBUG_EVENT("%s: Error: device name %s alrady exists!\n",__FUNCTION__,dev->name);
			dev_put(tmp_dev);
			err = -EEXIST;	/* name already exists */
		}else if (wan_netif_priv(dev)){
			err = register_netdev(dev);
			if (!err) {
				devle->dev = dev;
				
				wan_spin_lock(&wandev->dev_head_lock,&smp_flags);
				WAN_LIST_INSERT_HEAD(&wandev->dev_head, devle, dev_link);
				++wandev->ndev;
				wan_spin_unlock(&wandev->dev_head_lock,&smp_flags);
				err = 0;

				wan_free(conf);
				return 0;
			} else {
				DEBUG_EVENT("%s: Error: failed to register net device!\n",__FUNCTION__);
			}
		}

		if (wandev->del_if)
			wandev->del_if(wandev, dev);
	}

	/* This code has moved from del_if() function */
	if (wan_netif_priv(dev)){
		wan_free(wan_netif_priv(dev));
		wan_netif_set_priv(dev,NULL);
	}

wan_device_new_if_exit:

	if (dev){
		DEBUG_SUB_MEM(sizeof(netdevice_t));
		wan_netif_free(dev);
	}
	if (conf){
		DEBUG_SUB_MEM(sizeof(conf));
		wan_free(conf);
	}

	return err;
}


/*
 *	Delete WAN logical channel.
 *	 o verify user address space
 *	 o copy configuration data to kernel address space
 */

static int wan_device_del_if (wan_device_t *wandev, char *u_name)
{
	struct wan_dev_le	*devle;
	netdevice_t		*dev=NULL;
	char name[WAN_IFNAME_SZ + 1];
    int err = 0;
	wan_smp_flag_t smp_flags;

	if (wandev->state == WAN_UNCONFIGURED)
		return -ENODEV;
	
	memset(name, 0, sizeof(name));

	if(copy_from_user(name, u_name, WAN_IFNAME_SZ))
		return -EFAULT;

	WAN_LIST_FOREACH(devle, &wandev->dev_head, dev_link){
		dev = WAN_DEVLE2DEV(devle);
		if (dev && !strcmp(name, wan_netif_name(dev))){
			break;
		}
        }

	if (devle == NULL || dev == NULL){
		if ((dev = wan_dev_get_by_name(name)) == NULL){
			printk(KERN_INFO "%s: wan_dev_get_by_name failed\n", name);
			return err;
		}

		dev_put(dev);
#ifdef CONFIG_PRODUCT_WANPIPE_ANNEXG		
		if (dev->type == ARPHRD_LAPB){
			printk(KERN_INFO "%s: Unregistering LAPB interface\n", dev->name);
			return wan_device_del_if_lapb(wandev,dev);
		}else if (dev->type == ARPHRD_X25){
			printk(KERN_INFO "%s: Unregistering X25 interface\n", dev->name);
			return wan_device_del_if_x25(wandev,dev);
		}else if (dev->type == ARPHRD_VOID){
			printk(KERN_INFO "%s: Unregistering DSP interface\n", dev->name);
			return wan_device_del_if_dsp(wandev,dev);
		}else
#endif		
		{
			printk(KERN_INFO "%s: Unregistering LIP interface\n", dev->name);
			return wan_device_del_if_lip(wandev,dev);
		}
		
#if 0
		FIXME: Find a better way to distinguish between 
		       interfaces
		printk(KERN_INFO "%s: Unknown interface dev type %d\n",
					dev->name,dev->type);
			err=-EINVAL;		
#endif
		return -ENODEV;
	}

		
	err = delete_interface(wandev, dev,0);
	if (err){
		return(err);
	}

       	wan_spin_lock(&wandev->dev_head_lock,&smp_flags);
	WAN_LIST_REMOVE(devle, dev_link);
	--wandev->ndev;
       	wan_spin_unlock(&wandev->dev_head_lock,&smp_flags);

	wan_free(devle);

		
	/* If last interface being deleted, shutdown card
	 * This helps with administration at leaf nodes
	 * (You can tell if the person at the other end of the phone 
	 * has an interface configured) and avoids DoS vulnerabilities
	 * in binary driver files - this fixes a problem with the current
	 * Sangoma driver going into strange states when all the network
	 * interfaces are deleted and the link irrecoverably disconnected.
	 */ 

        if (!wandev->ndev && wandev->shutdown){
                err = wandev->shutdown(wandev, NULL);
	}
	return err;
}


/*
 *	Miscellaneous Functions
 */

/*
 *	Find WAN device by name.
 *	Return pointer to the WAN device data space or NULL if device not found.
 */

wan_device_t *wan_find_wandev_device(char *name)
{
	wan_device_t *wandev;
	wan_smp_flag_t smp_flags;

	wan_spin_lock(&wan_devlist_lock,&smp_flags);
	WAN_LIST_FOREACH(wandev, &wan_devlist, next){
		if (!strcmp(wandev->name, name)){
			break;
		}
	}
	wan_spin_unlock(&wan_devlist_lock,&smp_flags);

        if (wandev == NULL) {
		DEBUG_TEST("wan_find_wandev_device() Name=%s not found!\n",
			name);
  	}

	return wandev;
}

/*
 *	Delete WAN logical channel identified by its name.
 *	o find logical channel by its name
 *	o call driver's del_if() entry point
 *	o unregister network interface
 *	o unlink channel data space from linked list of channels
 *	o release channel data space
 *
 *	Return:	0		success
 *		-ENODEV		channel not found.
 *		-EBUSY		interface is open
 *
 *	Note: If (force != 0), then device will be destroyed even if interface
 *	associated with it is open. It's caller's responsibility to make
 *	sure that opened interfaces are not removed!
 */

static int delete_interface (wan_device_t *wandev, netdevice_t *dev, int force)
{
	int err;
	
	if (dev == NULL){
		return -ENODEV;	/* interface not found */
	}

	if (netif_running(dev)){
		if (force) {
			rtnl_lock();
			err=dev_change_flags(dev,(dev->flags&~(IFF_UP)));
			rtnl_unlock();
			if (err) {
				return err;
			}

			if (netif_running(dev)){
				DEBUG_EVENT("%s: Error Device Busy - Force down failed!\n",dev->name);
				return -EBUSY;
			}

		} else {
			return -EBUSY;	/* interface in use */
		}
	}
	/* Unregister the lip interface attached
	 * to this interace */
	err=wan_device_unreg_lip(dev);
	if (err){
		return err;
	}
	
	if (wandev->del_if){
		int err;
		if ((err=wandev->del_if(wandev, dev))){
			return err;
		}
	}

	
	printk(KERN_INFO "%s: unregistering '%s'\n", wandev->name, dev->name); 

	
	/* Due to new interface linking method using wan_netif_priv(dev),
	 * this code has moved from del_if() function.*/
	if (wan_netif_priv(dev)){
		wan_free(wan_netif_priv(dev));
		wan_netif_set_priv(dev,NULL);
	}

	unregister_netdev(dev);
	
	DEBUG_SUB_MEM(sizeof(netdevice_t));

#ifdef LINUX_2_4
	return 0;
#else
	/* Changed behaviour for 2.6 kernels */
	wan_netif_free(dev);
	return 0;
#endif

}

/* 
 * ============================================================================
 *	Debugging WAN device.
 */
 
static int wan_device_debugging (wan_device_t *wandev)
{
	int 			error = 0;

	if (wandev->state == WAN_UNCONFIGURED){
		return 0;
	}

	if (wandev->debugging){
		error = wandev->debugging(wandev);
	}

	return 0;
}

unsigned long wan_set_ip_address (netdevice_t *dev, int option, unsigned long ip)
{

	struct sockaddr_in *if_data;
	struct ifreq if_info;	
	int err=0;
	mm_segment_t fs;
	
	 /* Setup a structure for adding/removing routes */
        memset(&if_info, 0, sizeof(if_info));
        strcpy(if_info.ifr_name, dev->name);

	switch (option){

	case WAN_LOCAL_IP:

		if_data = (struct sockaddr_in *)&if_info.ifr_addr;
		if_data->sin_addr.s_addr = ip;
		if_data->sin_family = AF_INET;

		fs = get_fs();                  /* Save file system  */
       		set_fs(get_ds());    
		err = wp_devinet_ioctl(SIOCSIFADDR, &if_info);
		set_fs(fs);
		break;
	
	case WAN_POINTOPOINT_IP:

		if_data = (struct sockaddr_in *)&if_info.ifr_dstaddr;
		if_data->sin_addr.s_addr = ip;
		if_data->sin_family = AF_INET;

		fs = get_fs();                  /* Save file system  */
       		set_fs(get_ds());  
		err = wp_devinet_ioctl(SIOCSIFDSTADDR, &if_info);
		set_fs(fs);
		break;	

	case WAN_NETMASK_IP:
		break;

	case WAN_BROADCAST_IP:
		break;
	}

	return err;
}	

unsigned long wan_get_ip_address (netdevice_t *dev, int option)
{
	
#if defined(LINUX_2_4) || defined(LINUX_2_6)
	struct in_ifaddr *ifaddr;
	struct in_device *in_dev;

	if ((in_dev = in_dev_get(dev)) == NULL){
		return 0;
	}
#else
	struct in_ifaddr *ifaddr;
	struct in_device *in_dev;
	
	if ((in_dev = dev->ip_ptr) == NULL){
		return 0;
	}
#endif

#if defined(LINUX_2_4) || defined(LINUX_2_6)
	wp_rcu_read_lock(in_dev);
	for (ifaddr=in_dev->ifa_list; ifaddr != NULL;
		ifaddr=ifaddr->ifa_next) {
		if (strcmp(dev->name, ifaddr->ifa_label) == 0){
			break;
		}
	}
	wp_rcu_read_unlock(in_dev);
	in_dev_put(in_dev);
#else
	for (ifaddr=in_dev->ifa_list; ifaddr != NULL;
		ifaddr=ifaddr->ifa_next){
		if (strcmp(dev->name, ifaddr->ifa_label) == 0) 
		{
			break;
		}
	}
#endif

	if (ifaddr == NULL ){
		return 0;
	}
	
	switch (option){

	case WAN_LOCAL_IP:
		return ifaddr->ifa_local;
		break;
	
	case WAN_POINTOPOINT_IP:
		return ifaddr->ifa_address;
		break;	

	case WAN_NETMASK_IP:
		return ifaddr->ifa_mask;
		break;

	case WAN_BROADCAST_IP:
		return ifaddr->ifa_broadcast;
		break;
	default:
		return 0;
	}

	return 0;
}	

static inline void set_sockaddr(struct sockaddr_in *sin, u32 addr, u16 port)
{
	sin->sin_family = AF_INET;
	sin->sin_addr.s_addr = addr;
	sin->sin_port = port;
}

void wan_add_gateway(netdevice_t *dev)
{
	mm_segment_t oldfs;
	struct rtentry route;
	int res;
#if 0
	unsigned long local_ip;
	unsigned long remote_ip;
	unsigned long netmask_ip;

	local_ip=wan_get_ip_address(dev, WAN_LOCAL_IP);
	remote_ip=wan_get_ip_address(dev, WAN_POINTOPOINT_IP);
	netmask_ip=wan_get_ip_address(dev, WAN_NETMASK_IP);
#endif		
	memset((char*)&route,0,sizeof(struct rtentry));

#if 0
	if (local_ip && remote_ip && local_ip != remote_ip){
		set_sockaddr((struct sockaddr_in *) &route.rt_gateway, remote_ip, 0);
	}
#endif
	route.rt_dev = dev->name;
	set_sockaddr((struct sockaddr_in *) &route.rt_dst, 0, 0);
	set_sockaddr((struct sockaddr_in *) &route.rt_genmask, 0, 0);
	route.rt_flags = 0;  

	oldfs = get_fs();
	set_fs(get_ds());
	res = wp_ip_rt_ioctl(SIOCADDRT,&route);
	set_fs(oldfs);

	if (res == 0){
		DEBUG_EVENT("%s: IP default route added for: %s\n",
			dev->name,dev->name);
		DEBUG_TX("%s: IP default route added : %u.%u.%u.%u\n",
			dev->name,NIPQUAD(remote_ip));
	}else{
		DEBUG_EVENT("%s: Failed to set IP default route for: %s :Rc=%i\n",
			dev->name,dev->name,res);
		DEBUG_TX("%s: Failed to set IP default route : %u.%u.%u.%u :Rc=%i\n",
			dev->name,NIPQUAD(remote_ip),res);
	}

	return;
}



#ifdef CONFIG_PRODUCT_WANPIPE_ANNEXG

/******************************************************************
 * API,DSP,X25,LAP Layer registration
 *
 * Each protocol registers its functions to wanrouter.o
 * which is the base module needed by all upper layers.
 * This way, upper modules are not deppendent on one
 * another during module load time.
 *
 * eg: TCP->WANPIPE->WANROUTER
 *     API->WANPIPE->WANROUTER
 *     TCP->LAPB->WANPIPE->WANROUTER
 *     API->LAPB->WANPIPE->WANROUTER
 *     TCP->X25->LAPB->WANPIPE->WANROUTER
 *     API->X25->LAPB->WANPIPE->WANROUTER
 *     API->DSP->X25->LAPB->WANPIPE->WANROUTER
 *
 * Any combination of above layers is possible.
 * 
 * (WANPIPE: Frame Relay, Chdlc, PPP etc...)
 * 
 * 
 */


/*=================================================================
 * register_wanpipe_lapb_protocol
 *
 * The lapb protocol layer binds its function to the wanrouter.o
 * module during lapb module load, so they can be 
 * used by the higher layers such as x25,dsp,socket.
 * 
 */


int register_wanpipe_lapb_protocol (struct wanpipe_lapb_register_struct *lapb_reg)
{
	memcpy(&lapb_protocol,lapb_reg,sizeof(struct wanpipe_lapb_register_struct));
	REG_PROTOCOL_FUNC(lapb_protocol);
	return 0;
}

void unregister_wanpipe_lapb_protocol (void)
{
	UNREG_PROTOCOL_FUNC(lapb_protocol);
	return;
}

/*=================================================================
 * register_wanpipe_x25_protocol
 *
 * The x25 protocol layer binds its function to the wanrouter.o
 * module during x25 module load, so they can be 
 * used by the higher layers such as dsp and socket.
 * 
 */

int register_wanpipe_x25_protocol (struct wanpipe_x25_register_struct *x25_api_reg)
{
	memcpy(&x25_protocol,x25_api_reg,sizeof(struct wanpipe_x25_register_struct));
	REG_PROTOCOL_FUNC(x25_protocol);
	return 0;
}

void unregister_wanpipe_x25_protocol (void)
{
	UNREG_PROTOCOL_FUNC(x25_protocol);
	return;
}


/*=================================================================
 * register_wanpipe_dsp_protocol
 *
 * The dsp protocol layer binds its function to the wanrouter.o
 * module during dsp module load, so they can be 
 * used by the api socket.
 * 
 */

int register_wanpipe_dsp_protocol (struct wanpipe_dsp_register_struct *dsp_api_reg)
{
	memcpy(&dsp_protocol, dsp_api_reg, sizeof(struct wanpipe_dsp_register_struct));
	REG_PROTOCOL_FUNC(dsp_protocol);
	return 0;
}

void unregister_wanpipe_dsp_protocol (void)
{
	UNREG_PROTOCOL_FUNC(dsp_protocol);
	return;
}

#endif

/*=================================================================
 * register_wanpipe_api_socket
 *
 * The api socket binds its function to the wanrouter.o
 * module during af_wanpipe_api module load, so they can be 
 * used by lower protocols, such as x25, frame relay ...
 * 
 */

int register_wanpipe_api_socket (struct wanpipe_api_register_struct *wan_api_reg)
{
	memcpy(&api_socket, wan_api_reg, sizeof(struct wanpipe_api_register_struct)); 
	REG_PROTOCOL_FUNC(api_socket);
	++devcnt;
    MOD_INC_USE_COUNT;
	return 0;
}

void unregister_wanpipe_api_socket (void)
{

	UNREG_PROTOCOL_FUNC(api_socket);
	--devcnt;
	MOD_DEC_USE_COUNT;
}


/*=================================================================
 * register_wanpipe_fw_protocol
 *
 */

int register_wanpipe_fw_protocol (struct wanpipe_fw_register_struct *wp_fw_reg)
{
	memcpy(&wp_fw_protocol, wp_fw_reg, sizeof(struct wanpipe_fw_register_struct));
	REG_PROTOCOL_FUNC(wp_fw_protocol);
	return 0;
}

void unregister_wanpipe_fw_protocol (void)
{
	UNREG_PROTOCOL_FUNC(wp_fw_protocol);
	return;
}



#ifdef CONFIG_PRODUCT_WANPIPE_ANNEXG
static int wan_device_new_if_lapb (wan_device_t *wandev, wanif_conf_t *u_conf)
{
	struct wan_dev_le	*devle;
	wanif_conf_t *conf=NULL;
	netdevice_t *dev=NULL;
	netdevice_t *annexg_dev=NULL;
	netdevice_t *tmp_dev;
	int err=-EINVAL;

	if ((wandev->state == WAN_UNCONFIGURED) || (wandev->new_if == NULL))
		return -ENODEV;

	conf=wan_malloc(sizeof(wanif_conf_t));
	if (!conf){
		return -ENOMEM;
	}
	
	if(copy_from_user(conf, u_conf, sizeof(wanif_conf_t))){
		err = -EFAULT;
		goto wan_device_new_if_lapb_exit;
	}
		
	if (conf->magic != ROUTER_MAGIC){
		err = -EINVAL;
		goto wan_device_new_if_lapb_exit;
	}

	if (!test_bit(0,&lapb_protocol.init)){
		err = -EPROTONOSUPPORT;
		goto wan_device_new_if_lapb_exit;
	}
		
	err = -EPROTONOSUPPORT;

	if (!conf->name){
		printk(KERN_INFO "wanpipe: NEW_IF lapb no interface name!\n");
		err = -EINVAL;	
		goto wan_device_new_if_lapb_exit;
	}
	
	if ((tmp_dev=wan_dev_get_by_name(conf->name)) != NULL){
		printk(KERN_INFO "%s: Device already exists!\n",
				conf->name);
		dev_put(tmp_dev);
		err = -EEXIST;
		goto wan_device_new_if_lapb_exit;
	}
		
	printk(KERN_INFO "%s: Registering Lapb device %s -> %s\n",
			wandev->name, conf->name, conf->master);

	/* Find the Frame Relay DLCI to bind to the LAPB device */
	WAN_LIST_FOREACH(devle, &wandev->dev_head, dev_link){
		dev = WAN_DEVLE2DEV(devle);
		if (dev && !strcmp(dev->name, conf->master)){
			break;
		}
	}
	
	if (devle == NULL || dev == NULL){
		printk(KERN_INFO "%s: Master device %s used by Lapb %s not found !\n",
				wandev->name, conf->master,conf->name);
		err = -ENODEV;
		goto wan_device_new_if_lapb_exit;
	}
	dev = WAN_DEVLE2DEV(devle);
	if (!IS_FUNC_CALL(lapb_protocol,lapb_register)){
		goto wan_device_new_if_lapb_exit;
	}
	
	if ((err=lapb_protocol.lapb_register(dev,conf->name,wandev->name,&annexg_dev)) != 0){
		printk(KERN_INFO "%s: Failed to register Lapb Protocol\n",wandev->name);
		goto wan_device_new_if_lapb_exit;
	}

	if (conf->station == WANOPT_DCE){
		conf->u.lapb.mode |= 0x04;
	}

	if (lapb_protocol.lapb_setparms(annexg_dev,&conf->u.lapb)){
		lapb_protocol.lapb_unregister(annexg_dev);
		printk(KERN_INFO "%s: Failed to setup Lapb Protocol\n",wandev->name);
		err= -EINVAL;
		goto wan_device_new_if_lapb_exit;
	}

	if (!wandev->bind_annexg){
		printk(KERN_INFO "%s: Device %s doesn't support ANNEXG\n",
				wandev->name,dev->name);
		lapb_protocol.lapb_unregister(annexg_dev);
		err = 	-EPROTONOSUPPORT;
		goto wan_device_new_if_lapb_exit;
	}
	
	if (wandev->bind_annexg(dev, annexg_dev)){
		lapb_protocol.lapb_unregister(annexg_dev);
		printk(KERN_INFO "%s: Failed to bind %s to %s\n",
				wandev->name,annexg_dev->name,conf->master);
		err = -EINVAL;
		goto wan_device_new_if_lapb_exit;
	}

wan_device_new_if_lapb_exit:

	if (conf){
		wan_free(conf);
	}
	
	return 0;
}

static int wan_device_del_if_lapb(wan_device_t *wandev, netdevice_t *dev)
{
	int err;
	netdevice_t *master_dev=NULL;
	
	if (dev->flags & IFF_UP){
		printk(KERN_INFO "%s: Failed to del interface: Device UP!\n",
				dev->name);
		return -EBUSY;
	}

	if (wandev->un_bind_annexg){
		master_dev=wandev->un_bind_annexg(wandev,dev); 
	}
	
	if (IS_FUNC_CALL(lapb_protocol,lapb_unregister)){
		if ((err=lapb_protocol.lapb_unregister(dev)) != 0){
			printk(KERN_INFO "%s: Lapb unregister failed rc=%i\n",dev->name,err);
			if (wandev->bind_annexg && master_dev){
				wandev->bind_annexg(master_dev,dev);
			}
			return err;
		}
	}

	return 0;
}

static int wan_device_new_if_x25 (wan_device_t *wandev, wanif_conf_t *u_conf)
{
	wanif_conf_t *conf=NULL;
	netdevice_t *dev=NULL;
	netdevice_t *annexg_dev=NULL;
	netdevice_t *tmp_dev;
	int err=-EINVAL;

	if ((wandev->state == WAN_UNCONFIGURED) || (wandev->new_if == NULL))
		return -ENODEV;
	
	conf=wan_malloc(sizeof(wanif_conf_t));
	if (!conf){
		return -ENOMEM;
	}
	
	if(copy_from_user(conf, u_conf, sizeof(wanif_conf_t))){
		err = -EFAULT;
		goto wan_device_new_if_x25_exit;
	}
		
	if (conf->magic != ROUTER_MAGIC){
		err = -EINVAL;
		goto wan_device_new_if_x25_exit;
	}

	if (!test_bit(0,&lapb_protocol.init)){
		printk(KERN_INFO "%s: Lapb protocol not initialized!\n",
				wandev->name);
		err = -EPROTONOSUPPORT;
		goto wan_device_new_if_x25_exit;
	}

	err = -EPROTONOSUPPORT;

	if (!conf->name){
		printk(KERN_INFO "wanpipe: NEW_IF x25 no interface name!\n");
		err = -EINVAL;	
		goto wan_device_new_if_x25_exit;
	}

	if ((tmp_dev=wan_dev_get_by_name(conf->name)) != NULL){
		printk(KERN_INFO "%s: Device already exists!\n",
				conf->name);
		dev_put(tmp_dev);
		err = -EEXIST;
		goto wan_device_new_if_x25_exit;
	}

	printk(KERN_INFO "%s: Registering X25 SVC %s -> %s\n",
			wandev->name, conf->name, conf->master);

	if (conf->station == WANOPT_DTE){
		conf->u.x25.dte=1;
	}else{
		conf->u.x25.dte=0;
	}

	if (!conf->master){
		printk(KERN_INFO "wanpipe: NEW_IF x25 no master interface name!\n");
		err = -EINVAL;	
		goto wan_device_new_if_x25_exit;
	}
		
	//Find a master device for our x25 lcn
	if ((dev = wan_dev_get_by_name(conf->master)) == NULL){
		printk(KERN_INFO "%s: Master device %s used by X25 SVC %s no found!\n",
				wandev->name,conf->master,conf->name);
		goto wan_device_new_if_x25_exit;
	}
	dev_put(dev);

	if ((err=lapb_protocol.lapb_x25_register (dev,conf->name,&annexg_dev)) != 0){
		printk(KERN_INFO "%s: Failed to register x25 device %s\n",
				wandev->name,conf->name);
		goto wan_device_new_if_x25_exit;
	}

	memcpy(&conf->u.x25.addr[0],&conf->addr[0],WAN_ADDRESS_SZ);
	
	if (lapb_protocol.lapb_x25_setparms(dev,annexg_dev,&conf->u.x25)){
		lapb_protocol.lapb_x25_unregister(annexg_dev);
		printk(KERN_INFO "%s: Failed to setup X25 Protocol\n",wandev->name);
		err = -EINVAL;
		goto wan_device_new_if_x25_exit;
	}	

wan_device_new_if_x25_exit:

	if (conf){
		wan_free(conf);
	}
	
	return err;
}


static int wan_device_del_if_x25(wan_device_t *wandev, netdevice_t *dev)
{
	if (dev->flags & IFF_UP){
		printk(KERN_INFO "%s: Failed to del interface: Device UP!\n",
				dev->name);
		return -EBUSY;
	}
	
	if (!IS_FUNC_CALL(lapb_protocol,lapb_x25_unregister))
		return 0;
	
	return lapb_protocol.lapb_x25_unregister(dev);
}


static int wan_device_new_if_dsp (wan_device_t *wandev, wanif_conf_t *u_conf)
{
	wanif_conf_t *conf=NULL;
	netdevice_t *dev=NULL;
	netdevice_t *annexg_dev=NULL;
	netdevice_t *tmp_dev;
	int err=-EINVAL;

	if ((wandev->state == WAN_UNCONFIGURED) || (wandev->new_if == NULL))
		return -ENODEV;

	conf=wan_malloc(sizeof(wanif_conf_t));
	if (!conf){
		return -ENOMEM;
	}
	
	if (copy_from_user(conf, u_conf, sizeof(wanif_conf_t))){
		err = -EFAULT;
		goto wan_device_new_if_dsp_exit;
	}
		
	if (conf->magic != ROUTER_MAGIC){
		err = -EINVAL;
		goto wan_device_new_if_dsp_exit;
	}

	if (!IS_FUNC_CALL(x25_protocol,x25_dsp_register)){
		printk(KERN_INFO "%s: X25 protocol not initialized.\n",wandev->name);
		err = -EPROTONOSUPPORT;
		goto wan_device_new_if_dsp_exit;
	}

	err = -EPROTONOSUPPORT;

	if (!conf->name){
		printk(KERN_INFO "wanpipe: NEW_IF dsp no interface name!\n");
		err= -EINVAL;	
		goto wan_device_new_if_dsp_exit;
	}

	if ((tmp_dev=wan_dev_get_by_name(conf->name)) != NULL){
		printk(KERN_INFO "%s: Device already exists!\n",
				conf->name);
		dev_put(tmp_dev);
		err = -EEXIST;
		goto wan_device_new_if_dsp_exit;
	}

	
	printk(KERN_INFO "%s: Registering DSP device %s -> %s\n",
			wandev->name, conf->name, conf->master);

	if (!conf->master){
		printk(KERN_INFO "wanpipe: NEW_IF lapb no master interface name!\n");
		err = -EINVAL;	
		goto wan_device_new_if_dsp_exit;
	}
	
	//Find a master device for our x25 lcn
	if ((dev = wan_dev_get_by_name(conf->master)) == NULL){
		printk(KERN_INFO "%s: Master device %s, no found for %s\n",
				wandev->name, conf->master,conf->name);
		goto wan_device_new_if_dsp_exit;
	}
	dev_put(dev);

	if ((err=x25_protocol.x25_dsp_register (dev, conf->name,&annexg_dev)) != 0){
		goto wan_device_new_if_dsp_exit;
	}

	if (IS_FUNC_CALL(x25_protocol,x25_dsp_setparms)){
		if (x25_protocol.x25_dsp_setparms(dev, annexg_dev, &conf->u.dsp)){
			x25_protocol.x25_dsp_unregister(annexg_dev);
			printk(KERN_INFO "%s: Failed to setup DSP Protocol\n",wandev->name);
			err = -EINVAL;
			goto wan_device_new_if_dsp_exit;
		}	
	}
	
wan_device_new_if_dsp_exit:

	if (conf){
		wan_free(conf);
	}

	return err;
}


static int wan_device_del_if_dsp(wan_device_t *wandev, netdevice_t *dev)
{
	printk(KERN_INFO "%s: Unregistering DSP Device %s\n", 
			wandev->name, dev->name);

	if (!IS_FUNC_CALL(x25_protocol,x25_dsp_unregister)){
		return 0;
	}

	return x25_protocol.x25_dsp_unregister(dev);
}


/* CONFIG_PRODUCT_WANPIPE_ANNEXG */
#endif



#if 1

int register_wanpipe_lip_protocol (wplip_reg_t *lip_reg)
{
	memcpy(&wplip_protocol,lip_reg,sizeof(wplip_reg_t));
	REG_PROTOCOL_FUNC(wplip_protocol);
	return 0;
}

void unregister_wanpipe_lip_protocol (void)
{
	UNREG_PROTOCOL_FUNC(wplip_protocol);
	return;
}

static int wan_device_unreg_lip(netdevice_t *dev)
{
	void *lip_link = wan_get_lip_ptr(dev);
	int err;

	if (!IS_PROTOCOL_FUNC(wplip_protocol)) return 0;

	if (!IS_FUNC_CALL(wplip_protocol,wplip_if_unreg))
		return 0;

	if (lip_link){
		err=wplip_protocol.wplip_unreg(lip_link);
		if (err){
			return err;
		}
		wan_set_lip_ptr(dev,NULL);
		wan_set_lip_prot(dev,0);
	}
	return 0;
}

static int wan_device_new_if_lip (wan_device_t *wandev, wanif_conf_t *u_conf)
{
	wanif_conf_t *conf=NULL;
	netdevice_t *dev=NULL;
	netdevice_t *tmp_dev;
	void *lip_link;
	int err=-EINVAL, init_if=0;

	if ((wandev->state == WAN_UNCONFIGURED) || (wandev->new_if == NULL))
		return -ENODEV;
	
	conf=wan_malloc(sizeof(wanif_conf_t));
	if (!conf){
		return -ENOMEM;
	}
	
	if(copy_from_user(conf, u_conf, sizeof(wanif_conf_t))){
		err = -EFAULT;
		goto wan_device_new_if_lip_exit;
	}
		
	if (conf->magic != ROUTER_MAGIC){
		err = -EINVAL;
		goto wan_device_new_if_lip_exit;
	}

	if (!test_bit(0,&wplip_protocol.init)){
		printk(KERN_INFO "%s: LIP protocol not initialized!\n",
				wandev->name);
		err = -EPROTONOSUPPORT;
		goto wan_device_new_if_lip_exit;
	}


	err = -EPROTONOSUPPORT;

	if (!conf->name){
		printk(KERN_INFO "wanpipe: NEW_IF LIP no interface name!\n");
		err = -EINVAL;	
		goto wan_device_new_if_lip_exit;
	}

	if ((tmp_dev=wan_dev_get_by_name(conf->name)) != NULL){
		printk(KERN_INFO "%s: Device already exists!\n",
				conf->name);
		dev_put(tmp_dev);
		err = -EEXIST;
		goto wan_device_new_if_lip_exit;
	}

	DEBUG_EVENT("%s: Registering LIP %s -> %s (prot = %i)\n",
			wandev->name, conf->name, conf->master, conf->protocol);

	if (!conf->master){
		printk(KERN_INFO "wanpipe: NEW_IF LIP no master interface name!\n");
		err = -EINVAL;	
		goto wan_device_new_if_lip_exit;
	}
		
	//Find a master device lip device
	if ((dev = wan_dev_get_by_name(conf->master)) == NULL){
		printk(KERN_INFO "%s: Master device %s used by LIP %s no found!\n",
				wandev->name,conf->master,conf->name);
		goto wan_device_new_if_lip_exit;
	}
	dev_put(dev);

	lip_link = (wplip_reg_t*)wan_get_lip_ptr(dev); 
	if (!lip_link){

		err=wplip_protocol.wplip_register(&lip_link,conf,dev->name);
		if (err!=0){
			DEBUG_EVENT("%s: Failed to register LIP Link device %s\n",
					wandev->name,conf->name);
			goto wan_device_new_if_lip_exit;
		}
	
	
		err=wplip_protocol.wplip_bind_link(lip_link,dev);
		if (err!=0){
			wplip_protocol.wplip_unreg(lip_link);
			wan_set_lip_ptr(dev,NULL);
			wan_set_lip_prot(dev,0);

			DEBUG_EVENT("%s: Failed to bind master dev(%s) to LIP Link device\n",
					wandev->name,dev->name);
			goto wan_device_new_if_lip_exit;
		}
		
		wan_set_lip_ptr(dev,lip_link);
		wan_set_lip_prot(dev,conf->protocol);
		init_if=1;
	}

	err=wplip_protocol.wplip_if_reg(lip_link,conf->name,conf);
	if (err != 0){	
		printk(KERN_INFO "%s: Failed to register LIP device %s\n",
				wandev->name,conf->name);

		if (init_if){
			wplip_protocol.wplip_unreg(lip_link);
			wan_set_lip_ptr(dev,NULL);
			wan_set_lip_prot(dev,0);
		}
		
		goto wan_device_new_if_lip_exit;
	}

wan_device_new_if_lip_exit:

	if (conf){
		wan_free(conf);
	}
	
	return err;
}

static int wan_device_del_if_lip(wan_device_t *wandev, netdevice_t *dev)
{
	int err;
	
	if (dev->flags & IFF_UP){
		printk(KERN_INFO "%s: Failed to del interface: Device UP!\n",
				dev->name);
		return -EBUSY;
	}
	
	if (!IS_PROTOCOL_FUNC(wplip_protocol)) return 0;

	if (!IS_FUNC_CALL(wplip_protocol,wplip_if_unreg))
		return 0;

	err=wan_device_unreg_lip(dev);
	if (err){
		return err;
	}
	
	return wplip_protocol.wplip_if_unreg(dev);
}


#endif

#if defined(CONFIG_WANPIPE_HWEC)
int register_wanec_iface (wanec_iface_t *iface)
{
	memcpy(&wanec_iface, iface, sizeof(wanec_iface_t));
	REG_PROTOCOL_FUNC(wanec_iface);
	return 0;
}

void unregister_wanec_iface (void)
{
	UNREG_PROTOCOL_FUNC(wanec_iface);
	return;
}

void *wanpipe_ec_register(void *pcard, u_int32_t fe_port_mask, int max_line_no, int max_channels, void *conf)
{
	if (!IS_PROTOCOL_FUNC(wanec_iface)) return NULL;

	if (wanec_iface.reg){
		return wanec_iface.reg(pcard, fe_port_mask, max_line_no, max_channels, conf);
	}
	return NULL;
}
int wanpipe_ec_unregister(void *arg, void *pcard)
{
	if (!IS_PROTOCOL_FUNC(wanec_iface)) return 0;
	if (wanec_iface.unreg){
		return wanec_iface.unreg(arg, pcard);
	}
	return -EINVAL;
}

int wanpipe_ec_event_ctrl(void *arg, void *pcard, wan_event_ctrl_t *event_ctrl)	
{
	if (!IS_PROTOCOL_FUNC(wanec_iface)) return 0;
	if (wanec_iface.event_ctrl){
		return wanec_iface.event_ctrl(arg, pcard, event_ctrl);
	}
	return 0;
}

int wanpipe_ec_isr(void *arg)
{
	if (!IS_PROTOCOL_FUNC(wanec_iface)) return 0;
	if (wanec_iface.isr){
		return wanec_iface.isr(arg);
	}
	return 0;
}

int wanpipe_ec_poll(void *arg, void *pcard)
{
	if (!IS_PROTOCOL_FUNC(wanec_iface)) return 0;
	if (wanec_iface.poll){
		return wanec_iface.poll(arg, pcard);
	}
	return -EINVAL;
}
#endif

void wan_skb_destructor (struct sk_buff *skb)
{
	if (skb->dev){
		struct net_device *dev=skb->dev;
		dev_put(dev);
		//printk(KERN_INFO "%s: Skb destructor: put dev: refcnt=%i\n",
		//		dev->name,atomic_read(&dev->refcnt));
	}	
}

/**
 * @path: pathname for the application
 * @argv: null-terminated argument list
 * @envp: null-terminated environment list
 *
 * Runs a user-space application.  The application is started asynchronously.  It
 * runs as a child of keventd.  It runs with full root capabilities.  keventd silently
 * reaps the child when it exits.
 *
 * Must be called from process context.  Returns zero on success, else a negative
 * error code.
 */

char wanrouter_path[256] = "/usr/sbin/wanrouter";

int wan_run_wanrouter(char * hwdevname, char *devname, char *action)
{
	char *argv [3], **envp;
	char wan_action[20], wan_device[20], wan_if[20];
	int i = 0, value;


	if (!wanrouter_path [0]){
		return -ENODEV;
	}

	if (in_interrupt ()) {
		DEBUG_EVENT("%s: Error: Interrupt mode! Exiting...",
				__FUNCTION__);
		return -EINVAL;
	}

	
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,24) 
	if (!current->fs->root) {
		/* statically linked USB is initted rather early */
		DEBUG_EVENT ("%s: Error: no FS yet",__FUNCTION__);
		return -ENODEV;
	}
#endif
	
	if (!(envp = (char **) kmalloc (20 * sizeof (char *), GFP_KERNEL))) {
		DEBUG_EVENT ("%s: Error: no memory!",__FUNCTION__);
		return -ENOMEM;
	}
	
	/* only one standardized param to hotplug command: type */
	argv [0] = wanrouter_path;
	argv [1] = "script";
	argv [2] = 0;

	/* minimal command environment */
	envp [i++] = "HOME=/";
	envp [i++] = "PATH=/sbin:/bin:/usr/sbin:/usr/bin";

#if 0
	/* hint that policy agent should enter no-stdout debug mode */
	envp [i++] = "DEBUG=kernel";
#endif

	sprintf(wan_action, "WAN_ACTION=start");
	envp[i++] = wan_action;
		
	if (hwdevname){
		sprintf(wan_device, "WAN_DEVICE=%s",hwdevname);
		envp[i++] = wan_device;

		if (devname){
			sprintf(wan_if, "WAN_INTERFACE=%s", devname);
			envp[i++] = wan_if;
		}
	}

	envp [i++] = 0;

	/* NOTE: user mode daemons can call the agents too */

	DEBUG_EVENT ("%s: Call User Mode %s %s",__FUNCTION__, argv [0], argv[1]);
	value = wan_call_usermodehelper (argv [0], argv, envp);
	kfree (envp);

	if (value != 0){
		DEBUG_EVENT ("%s: Error Call User Mode failed 0x%x",__FUNCTION__,value);
	}

	return value;
}

extern struct proc_dir_entry *proc_router;
EXPORT_SYMBOL(proc_router);
extern int proc_add_line(struct seq_file* m, char* frm, ...);
EXPORT_SYMBOL(proc_add_line);

EXPORT_SYMBOL(register_wan_device);
EXPORT_SYMBOL(unregister_wan_device);
EXPORT_SYMBOL(wanrouter_encapsulate);
EXPORT_SYMBOL(wanrouter_type_trans);


/* From wanproc.c */
EXPORT_SYMBOL(wanrouter_proc_add_protocol);
EXPORT_SYMBOL(wanrouter_proc_delete_protocol);
EXPORT_SYMBOL(wanrouter_proc_add_interface);
EXPORT_SYMBOL(wanrouter_proc_delete_interface);


EXPORT_SYMBOL(wan_get_ip_address);
EXPORT_SYMBOL(wan_set_ip_address);
EXPORT_SYMBOL(wan_add_gateway);
EXPORT_SYMBOL(wan_run_wanrouter);


#ifdef CONFIG_PRODUCT_WANPIPE_ANNEXG
/* From waniface.c */
EXPORT_SYMBOL(register_wanpipe_lapb_protocol);
EXPORT_SYMBOL(unregister_wanpipe_lapb_protocol);
EXPORT_SYMBOL(lapb_protocol);

EXPORT_SYMBOL(register_wanpipe_x25_protocol);
EXPORT_SYMBOL(unregister_wanpipe_x25_protocol);

EXPORT_SYMBOL(register_wanpipe_dsp_protocol);
EXPORT_SYMBOL(unregister_wanpipe_dsp_protocol);
#endif

EXPORT_SYMBOL(register_wanpipe_lip_protocol);
EXPORT_SYMBOL(unregister_wanpipe_lip_protocol);

EXPORT_SYMBOL(wanpipe_lip_rx);
EXPORT_SYMBOL(wanpipe_lip_connect);
EXPORT_SYMBOL(wanpipe_lip_disconnect);
EXPORT_SYMBOL(wanpipe_lip_kick);

EXPORT_SYMBOL(wan_skb_destructor);


EXPORT_SYMBOL(register_wanpipe_api_socket);
EXPORT_SYMBOL(unregister_wanpipe_api_socket);


EXPORT_SYMBOL(register_wanpipe_fw_protocol);
EXPORT_SYMBOL(unregister_wanpipe_fw_protocol);

#if defined(CONFIG_WANPIPE_HWEC)
EXPORT_SYMBOL(register_wanec_iface);
EXPORT_SYMBOL(unregister_wanec_iface);
EXPORT_SYMBOL(wanpipe_ec_register);
EXPORT_SYMBOL(wanpipe_ec_unregister);
EXPORT_SYMBOL(wanpipe_ec_event_ctrl);
EXPORT_SYMBOL(wanpipe_ec_isr);
EXPORT_SYMBOL(wanpipe_ec_poll);
#endif

/*
 *	End
 */
