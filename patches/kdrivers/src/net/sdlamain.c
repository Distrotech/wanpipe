/****************************************************************************
* sdlamain.c	WANPIPE(tm) Multiprotocol WAN Link Driver.  Main module.
*
* Author:	Nenad Corbic	<ncorbic@sangoma.com>
*		Gideon Hack	
*
* Copyright:	(c) 1995-2002 Sangoma Technologies Inc.
*
*		This program is free software; you can redistribute it and/or
*		modify it under the terms of the GNU General Public License
*		as published by the Free Software Foundation; either version
*		2 of the License, or (at your option) any later version.
* ============================================================================
* Jul 29, 2002  Nenad Corbic	Addes support for SDLC driver
* May 13, 2002  Nenad Corbic	Added the support for ADSL drivers
* Jan 2,  2002  Nenad Corbic	Updated the request_region() function call.
* 				Due to the change in function interface.
* Nov 5,  2001  Nenad Corbic	Changed the card_array into a card_list.
* 				Kernel limitation does not allow a large
* 				contigous memory allocation. This restricted
* 				the wanpipe drivers to 20 devices. 
* May 8,  2001  Nenad Corbic	Fixed the Piggiback bug introducted by
*                               the auto_pci_cfg update.  Thus, do not use
*                               the slot number in piggyback testing, if
*                               the auto_pci_cfg option is enabled.
* Dec 22, 2000  Nenad Corbic	Updated for 2.4.X kernels.
* 				Removed the polling routine.
* Nov 13, 2000  Nenad Corbic	Added hw probing on module load and dynamic
* 				device allocation. 
* Nov 7,  2000  Nenad Corbic	Fixed the Multi-Port PPP for kernels
*                               2.2.16 and above.
* Aug 2,  2000  Nenad Corbic	Block the Multi-Port PPP from running on
*  			        kernels 2.2.16 or greater.  The SyncPPP 
*  			        has changed.
* Jul 25, 2000  Nenad Corbic	Updated the Piggiback support for MultPPPP.
* Jul 13, 2000	Nenad Corbic	Added Multi-PPP support.
* Feb 02, 2000  Nenad Corbic    Fixed up piggyback probing and selection.
* Sep 23, 1999  Nenad Corbic    Added support for SMP
* Sep 13, 1999  Nenad Corbic	Each port is treated as a separate device.
* Jun 02, 1999  Gideon Hack     Added support for the S514 adapter.
*				Updates for Linux 2.2.X kernels.
* Sep 17, 1998	Jaspreet Singh	Updated for 2.1.121+ kernel
* Nov 28, 1997	Jaspreet Singh	Changed DRV_RELEASE to 1
* Nov 10, 1997	Jaspreet Singh	Changed sti() to restore_flags();
* Nov 06, 1997 	Jaspreet Singh	Changed DRV_VERSION to 4 and DRV_RELEASE to 0
* Oct 20, 1997 	Jaspreet Singh	Modified sdla_isr routine so that card->in_isr
*				assignments are taken out and placed in the
*				sdla_ppp.c, sdla_fr.c and sdla_x25.c isr
*				routines. Took out 'wandev->tx_int_enabled' and
*				replaced it with 'wandev->enable_tx_int'. 
* May 29, 1997	Jaspreet Singh	Flow Control Problem
*				added "wandev->tx_int_enabled=1" line in the
*				init module. This line intializes the flag for 
*				preventing Interrupt disabled with device set to
*				busy
* Jan 15, 1997	Gene Kozin	Version 3.1.0
*				 o added UDP management stuff
* Jan 02, 1997	Gene Kozin	Initial version.
*****************************************************************************/

#define _K22X_MODULE_FIX_

#include <linux/wanpipe_includes.h>
#include <linux/wanpipe_defines.h>
#include <linux/wanpipe_debug.h>
#include <linux/wanpipe_common.h>
#include <linux/wanpipe_events.h>
#include <linux/wanpipe_cfg.h>
#include <linux/wanpipe_wanrouter.h>	/* WAN router definitions */
#include <linux/sdladrv.h>
#include <linux/wanpipe.h>	/* WANPIPE common user API definitions */
#include <linux/sdlapci.h>
#include <linux/if_wanpipe.h>
#include <linux/if_wanpipe_common.h>
#include <linux/wanproc.h>
#include <linux/wanpipe_codec_iface.h>

#define KMEM_SAFETYZONE 8

#ifndef CONFIG_PRODUCT_WANPIPE_BASE

  #ifndef CONFIG_WANPIPE_FR
    #define wpf_init(a,b) (-EPROTONOSUPPORT) 
  #endif

  #ifndef CONFIG_WANPIPE_CHDLC
    #define wpc_init(a,b) (-EPROTONOSUPPORT) 
  #endif

  #ifndef CONFIG_WANPIPE_X25
   #define wpx_init(a,b) (-EPROTONOSUPPORT) 
  #endif
 
  #ifndef CONFIG_WANPIPE_PPP
   #define wpp_init(a,b) (-EPROTONOSUPPORT) 
  #endif

  #ifndef CONFIG_WANPIPE_MULTPROT 
   #define wp_mprot_init(a,b) (-EPROTONOSUPPORT) 
  #endif

  #ifndef CONFIG_WANPIPE_LIP_ATM 
   #define wp_lip_atm_init(a,b) (-EPROTONOSUPPORT) 
  #endif

  #ifndef CONFIG_WANPIPE_MULTFR
   #define wp_hdlc_fr_init(a,b) (-EPROTONOSUPPORT)	
  #endif

#else

  #ifndef CONFIG_PRODUCT_WANPIPE_FR
    #define wpf_init(a,b) (-EPROTONOSUPPORT) 
  #endif

  #ifndef CONFIG_PRODUCT_WANPIPE_CHDLC
    #define wpc_init(a,b) (-EPROTONOSUPPORT) 
  #endif

  #ifndef CONFIG_PRODUCT_WANPIPE_X25
   #define wpx_init(a,b) (-EPROTONOSUPPORT) 
  #endif
 
  #ifndef CONFIG_PRODUCT_WANPIPE_PPP
   #define wpp_init(a,b) (-EPROTONOSUPPORT) 
  #endif

  #ifndef CONFIG_PRODUCT_WANPIPE_MULTPROT 
   #define wp_mprot_init(a,b) (-EPROTONOSUPPORT) 
  #endif

  #ifndef CONFIG_WANPIPE_LIP_ATM 
   #define wp_lip_atm_init(a,b) (-EPROTONOSUPPORT) 
  #endif

  #ifndef CONFIG_PRODUCT_WANPIPE_MULTFR
   #define wp_hdlc_fr_init(a,b) (-EPROTONOSUPPORT)	
  #endif

#endif


#ifndef CONFIG_PRODUCT_WANPIPE_ASYHDLC
 #define wp_asyhdlc_init(a,b) (-EPROTONOSUPPORT)
#endif

#ifndef CONFIG_PRODUCT_WANPIPE_SDLC
 #define wp_sdlc_init(a,b) (-EPROTONOSUPPORT)
#endif

#ifndef CONFIG_PRODUCT_WANPIPE_EDU
 #define wpedu_init(a,b) (-EPROTONOSUPPORT) 
#endif

#ifndef CONFIG_PRODUCT_WANPIPE_BSC
 #define wpbsc_init(a,b) (-EPROTONOSUPPORT)
#endif
 
#ifndef CONFIG_PRODUCT_WANPIPE_SS7
  #define wpss7_init(a,b) (-EPROTONOSUPPORT) 
#endif

#ifndef CONFIG_PRODUCT_WANPIPE_BSCSTRM
 #define wp_bscstrm_init(a,b) (-EPROTONOSUPPORT)
#endif

#ifndef CONFIG_PRODUCT_WANPIPE_BITSTRM
 #define wpbit_init(a,b) (-EPROTONOSUPPORT)
#endif

#ifndef CONFIG_PRODUCT_WANPIPE_ADSL
 #define wp_adsl_init(card,conf) (-EPROTONOSUPPORT)
#endif

#ifndef CONFIG_PRODUCT_WANPIPE_POS
 #define wp_pos_init(card,conf) (-EPROTONOSUPPORT)
#endif

#ifndef CONFIG_PRODUCT_WANPIPE_ATM
 #define wp_atm_init(card,conf) (-EPROTONOSUPPORT)
#endif

#ifndef CONFIG_PRODUCT_WANPIPE_AFT
 #define wp_xilinx_init(card,conf) (-EPROTONOSUPPORT)
#endif

#ifndef CONFIG_PRODUCT_WANPIPE_AFT_BRI
 #define wp_aft_bri_init(card,conf) (-EPROTONOSUPPORT)
#endif

#ifndef CONFIG_PRODUCT_WANPIPE_AFT_SERIAL
 #define wp_aft_serial_init(card,conf) (-EPROTONOSUPPORT)
#endif

#ifndef CONFIG_PRODUCT_WANPIPE_AFT_TE1
 #define wp_aft_te1_init(card,conf) (-EPROTONOSUPPORT)
#endif

#ifndef CONFIG_PRODUCT_WANPIPE_AFT_RM
 #define wp_aft_analog_init(card,conf) (-EPROTONOSUPPORT)
#endif

#ifndef CONFIG_PRODUCT_WANPIPE_AFT_A600
 #define wp_aft_a600_init(card, conf) (-EPROTONOSUPPORT)
#endif

#ifndef CONFIG_PRODUCT_WANPIPE_AFT_56K
 #define wp_aft_56k_init(card,conf) (-EPROTONOSUPPORT)
#endif

#ifndef CONFIG_PRODUCT_WANPIPE_AFT_TE3
 #define wp_aft_te3_init(card,conf) (-EPROTONOSUPPORT)
#endif

#ifndef CONFIG_PRODUCT_WANPIPE_ADCCP
  #define wp_adccp_init(card,conf) (-EPROTONOSUPPORT)
#endif

#ifndef CONFIG_PRODUCT_WANPIPE_USB
  #define wp_usb_init(card,conf) (-EPROTONOSUPPORT)
#endif

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
******************************************************************************/



/****** Defines & Macros ****************************************************/

#ifdef	_DEBUG_
#define	STATIC
#else
#define	STATIC		static
#endif

#ifndef	CONFIG_WANPIPE_CARDS		/* configurable option */
#define	CONFIG_WANPIPE_CARDS 1
#endif

#define	CMD_OK		0		/* normal firmware return code */
#define	CMD_TIMEOUT	0xFF		/* firmware command timed out */
#define	MAX_CMD_RETRY	10		/* max number of firmware retries */

#define INVALID_ADAPTER_CHECK(card,cfg) \
	{									\
	  u16	adapter_type = 0x00;						\
	  card->hw_iface.getcfg(card->hw, SDLA_ADAPTERTYPE, &adapter_type);	\
	  if (adapter_type == S5144_ADPTR_1_CPU_T1E1 || 			\
	      IS_TE1_MEDIA(&(cfg))){					\
	      	DEBUG_EVENT("%s: Error: Protocol not supported on S514-4 T1/E1 Adapter\n", \
			    	card->devname); 				\
		err = -EPFNOSUPPORT; 						\
		break; 								\
	  }else if (adapter_type == S5145_ADPTR_1_CPU_56K || 			\
	            IS_56K_MEDIA(&(cfg))){ 						\
	        DEBUG_EVENT("%s: Error: Protocol not supported on S514-5 56K Adapter\n", \
			    	card->devname); 				\
		err = -EPFNOSUPPORT; 						\
		break; 								\
          } \
       }

/****** Function Prototypes *************************************************/

extern void disable_irq(unsigned int);
extern void enable_irq(unsigned int);
 
/* Module entry points */
int init_module (void);
void cleanup_module (void);

/* WAN link driver entry points */
static int setup    (wan_device_t* wandev, wandev_conf_t* conf);
static int shutdown (wan_device_t* wandev, wandev_conf_t* conf);
static int ioctl    (wan_device_t* wandev, unsigned cmd, unsigned long arg);
static int debugging (wan_device_t* wandev);

/* IOCTL handlers */
static int ioctl_dump	(sdla_t* card, sdla_dump_t* u_dump);
static int ioctl_exec	(sdla_t* card, sdla_exec_t* u_exec, int);

/* Miscellaneous functions */
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,20)  
STATIC WAN_IRQ_RETVAL sdla_isr (int irq, void* dev_id, struct pt_regs *regs);
#else
STATIC WAN_IRQ_RETVAL sdla_isr (int irq, void* dev_id);
#endif

static void release_hw  (sdla_t *card);

static int check_s508_conflicts (sdla_t* card,wandev_conf_t* conf, int*);
static int check_s514_conflicts (sdla_t* card,wandev_conf_t* conf, int*);
static int check_adsl_conflicts (sdla_t* card,wandev_conf_t* conf, int*);
static int check_aft_conflicts (sdla_t* card,wandev_conf_t* conf, int*);

static int wanpipe_register_fw_to_api(void);
static int wanpipe_unregister_fw_from_api(void);

static int wan_add_device(void);
static int wan_delete_device(char*);

/****** Global Data **********************************************************
 * Note: All data must be explicitly initialized!!!
 */

/* private data */
static char drvname[]	= "wanpipe";
static char fullname[]	= "WANPIPE(tm) Multi-Protocol WAN Driver Module";
static int ncards; 

sdla_t* card_list;	/* adapter data space */
sdla_t*  wanpipe_debug;


typedef struct{
	unsigned char name [100];
}func_debug_t;

static int DBG_ARRAY_CNT;
func_debug_t DEBUG_ARRAY[100];

extern sdladrv_callback_t sdladrv_callback;

/******* Kernel Loadable Module Entry Points ********************************/

/*============================================================================
 * Module 'insert' entry point.
 * o print announcement
 * o allocate adapter data space
 * o initialize static data
 * o register all cards with WAN router
 * o calibrate SDLA shared memory access delay.
 *
 * Return:	0	Ok
 *		< 0	error.
 * Context:	process
 */
 
MODULE_AUTHOR ("Nenad Corbic <ncorbic@sangoma.com>");
MODULE_DESCRIPTION ("Sangoma WANPIPE: WAN Multi-Protocol Driver");
MODULE_LICENSE("GPL");


int __init wanpipe_init(void)
{
	int i, cnt, err = 0;

	ncards=0;

	/* Hot-plug is not supported yet */
	sdladrv_callback.add_device = wan_add_device;
	sdladrv_callback.delete_device = wan_delete_device;

  	DEBUG_EVENT("%s %s.%s %s %s\n",
			fullname, WANPIPE_VERSION, WANPIPE_SUB_VERSION,
			WANPIPE_COPYRIGHT_DATES,WANPIPE_COMPANY);
	
	/* Probe for wanpipe cards and return the number found */
	DEBUG_EVENT("wanpipe: Probing for WANPIPE hardware.\n");
	cnt = sdla_hw_probe();
#if defined(CONFIG_PRODUCT_WANPIPE_USB)
	cnt += sdla_get_hw_usb_adptr_cnt();
#endif


#if defined(WANPIPE_DEVICE_ALLOC_CNT)
	if (WANPIPE_DEVICE_ALLOC_CNT > cnt) {
		cnt=WANPIPE_DEVICE_ALLOC_CNT;
	}
#endif
	if (cnt){
		DEBUG_EVENT("wanpipe: Allocating maximum %i devices: wanpipe%i - wanpipe%i.\n",
					cnt,1,cnt);
	}else{
		DEBUG_EVENT("wanpipe: No AFT/S514/S508 cards found, unloading modules!\n");
		return -ENODEV;
	}

	card_list=NULL;
	wanpipe_debug=NULL;
	
	for (i = 0; i < cnt; i++){
		if (wan_add_device()) break;
	}
	if (ncards == 0){
		DEBUG_EVENT("IN Init Module: NO Cards registered\n");
		return -ENODEV;
	}
	
	err=wanpipe_register_fw_to_api();
	if (err){
		return err;
	}

	err=wanpipe_codec_init();
	if (err){
		return err;
	}
	err=wp_ctrl_dev_create();
	if (err) {
		return err;
	}

	wanpipe_globals_util_init();

#if defined(CONFIG_PRODUCT_WANPIPE_AFT_CORE)
	aft_global_hw_device_init();
#endif

#if 0
	wp_tasklet_per_cpu_init();
#endif

	
	return err;
}

/*============================================================================
 * Module 'remove' entry point.
 * o unregister all adapters from the WAN router
 * o release all remaining system resources
 */
void __exit wanpipe_exit(void)
{

	wanpipe_unregister_fw_from_api();

	if (!card_list){
		return;
	}
		
	sdladrv_callback.add_device = NULL;
	sdladrv_callback.delete_device = NULL;

	/* Remove all devices */
	while(card_list){
		wan_delete_device(card_list->devname);
	}
	card_list=NULL;

	wanpipe_codec_free();
	wp_ctrl_dev_delete();

	DEBUG_EVENT("\n");
	DEBUG_EVENT("wanpipe: WANPIPE Modules Unloaded.\n");

}

module_init(wanpipe_init);
module_exit(wanpipe_exit);

/******* WAN Device Driver Entry Points *************************************/

static int wan_add_device(void)
{
	sdla_t		*card;
	wan_device_t	*wandev;
	int		err = 0;
 
	card = wan_kmalloc(sizeof(sdla_t));
	if (card == NULL){
		return -ENOMEM;
	}
		
	memset(card, 0, sizeof(sdla_t));
	card->next = NULL;
	sprintf(card->devname, "%s%d", drvname, ncards+1);
	wandev = &card->wandev;
	wandev->magic    	= ROUTER_MAGIC;
	wandev->name		= card->devname;
	wandev->priv  		= card;
	wandev->enable_tx_int 	= 0;
	wandev->setup    	= &setup;
	wandev->shutdown 	= &shutdown;
	wandev->ioctl    	= &ioctl;
	wandev->debugging	= &debugging;
	card->card_no		= ncards+1;
	err = register_wan_device(wandev);
	if (err) {
		DEBUG_EVENT("%s: %s registration failed with error %d!\n",
					drvname, card->devname, err);
		wan_free(card);
		return -EINVAL;
	}

	card->list=card_list;
	card_list=card;
	ncards++;
	return 0;
}

static int wan_delete_device(char *devname)
{
	sdla_t	*card = NULL, *tmp;

	if (strcmp(card_list->devname, devname) == 0){
		card = card_list;
		card_list = card_list->list;
	}else{
		for (tmp = card_list; tmp; tmp = tmp->list){
			if (tmp->list && strcmp(tmp->list->devname, devname) == 0){
				card = tmp->list;
				tmp->list = card->list;
				card->list = NULL;
				break;
			}
		}
		if (card == NULL){
			DEBUG_EVENT("%s: Failed to find device in a list!\n",
					devname);
			return -EINVAL;
		}
	}
	if (card->wandev.state != WAN_UNCONFIGURED){
		DEBUG_EVENT("%s: Device is still running!\n",
				card->devname);
		return -EINVAL;
	}

	unregister_wan_device(card->devname);
	wan_free(card);
	return 0;
}

/*============================================================================
 * Setup/configure WAN link driver.
 * o check adapter state
 * o make sure firmware is present in configuration
 * o make sure I/O port and IRQ are specified
 * o make sure I/O region is available
 * o allocate interrupt vector
 * o setup SDLA hardware
 * o call appropriate routine to perform protocol-specific initialization
 * o mark I/O region as used
 * o if this is the first active card, then schedule background task
 *
 * This function is called when router handles ROUTER_SETUP IOCTL. The
 * configuration structure is in kernel memory (including extended data, if
 * any).
 */
 
static int setup (wan_device_t* wandev, wandev_conf_t* conf)
{
	sdla_t* card;
	int err = 0;
	int irq=0;

	/* Sanity checks */
	if ((wandev == NULL) || (wandev->priv == NULL) || (conf == NULL)){
		DEBUG_EVENT("%s: Failed Sdlamain Setup wandev %p, card %p, conf %p !\n",
		      wandev->name,
		      wandev,wandev->priv,
		      conf); 
		return -EFAULT;
	}

	DEBUG_EVENT("%s: Starting WAN Setup\n", wandev->name);

	card = wandev->priv;
	if (wandev->state != WAN_UNCONFIGURED){
		DEBUG_EVENT("%s: Device already configured\n",
			wandev->name);
		return -EBUSY;		/* already configured */
	}
	card->wandev.piggyback = 0;

	DEBUG_EVENT("\n");
	DEBUG_EVENT("Processing WAN device %s...\n", wandev->name);

	/* Initialize the counters for each wandev 
	 * Used for counting number of times new_if and 
         * del_if get called.
	 */
	//conf->config_id = WANCONFIG_AFT_T116;
	wandev->del_if_cnt = 0;
	wandev->new_if_cnt = 0;
	wandev->config_id  = conf->config_id;

	switch(conf->config_id){
	case WANCONFIG_AFT:
		conf->card_type = WANOPT_AFT;
		break;
	case WANCONFIG_AFT_TE1:
		conf->card_type = WANOPT_AFT104;
		conf->S514_CPU_no[0] = 'A';
		break;
	case WANCONFIG_AFT_ANALOG:
		conf->card_type = WANOPT_AFT_ANALOG;
		conf->S514_CPU_no[0] = 'A';
		break;
	case WANCONFIG_AFT_TE3:
		conf->card_type = WANOPT_AFT300;
		conf->S514_CPU_no[0] = 'A';
		break;
	case WANCONFIG_AFT_ISDN_BRI:
		conf->card_type = WANOPT_AFT_ISDN;
		conf->S514_CPU_no[0] = 'A';
		break;	
	case WANCONFIG_AFT_56K:
		conf->card_type = WANOPT_AFT_56K;
		conf->S514_CPU_no[0] = 'A';
		break;
	case WANCONFIG_AFT_SERIAL:
		conf->card_type = WANOPT_AFT_SERIAL;
		conf->S514_CPU_no[0] = 'A';
		break;
	case WANCONFIG_AFT_T116:
		conf->card_type = WANOPT_T116;
		conf->S514_CPU_no[0] = 'A';
		break;
#if defined(CONFIG_PRODUCT_WANPIPE_USB)
	case WANCONFIG_USB_ANALOG:
		conf->card_type = WANOPT_USB_ANALOG;
		break;
#endif
	}

	wandev->card_type  = conf->card_type;
	
	card->hw = sdla_register(&card->hw_iface, conf, card->devname);
	if (card->hw == NULL){
		return -EINVAL;
	}
	/* Reset the wandev confid, because sdla_register
         * could have changed our config_id, in order to
         * support A102 config file for A102-SH */	 
	wandev->card_type  = conf->card_type;
	wandev->config_id  = conf->config_id;
	
	/* Check for resource conflicts and setup the
	 * card for piggibacking if necessary */
	switch (conf->card_type){
	
		case WANOPT_S50X:

			if (!conf->data_size || (conf->data == NULL)) {
				DEBUG_EVENT("%s: firmware not found in configuration data!\n",
							wandev->name);
				sdla_unregister(&card->hw, card->devname);
				return -EINVAL;
			}

			if ((err=check_s508_conflicts(card,conf,&irq)) != 0){
				sdla_unregister(&card->hw, card->devname);
				return err;
			}
			break;
			
		case WANOPT_S51X:

			if ((!conf->data_size || (conf->data == NULL)) && 
			    (conf->config_id != WANCONFIG_DEBUG)){
				DEBUG_EVENT("%s: firmware not found in configuration data!\n",
							wandev->name);
				sdla_unregister(&card->hw, card->devname);
				return -EINVAL;
			}
			if ((err=check_s514_conflicts(card,conf,&irq)) != 0){
				sdla_unregister(&card->hw, card->devname);
				return err;
			}
			break;
		
		case WANOPT_ADSL:
		
			if ((err=check_adsl_conflicts(card,conf,&irq)) != 0){
				sdla_unregister(&card->hw, card->devname);
				return err;
			}
			break;

		case WANOPT_AFT:
		case WANOPT_AFT104:
		case WANOPT_AFT300:
		case WANOPT_AFT_ISDN:	
		case WANOPT_AFT_ANALOG:
		case WANOPT_AFT_56K:
		case WANOPT_AFT_SERIAL:
		case WANOPT_AFT_GSM:
		case WANOPT_T116:

			err=0;
			if ((err=check_aft_conflicts(card,conf,&irq)) != 0){
				sdla_unregister(&card->hw, card->devname);
				return err;
			}
			break;
#if defined(CONFIG_PRODUCT_WANPIPE_USB)
		case WANOPT_USB_ANALOG:
			break;
#endif
		default:
			DEBUG_EVENT("%s: (1) ERROR, invalid card type 0x%0X!\n",
					card->devname,conf->card_type);
			sdla_unregister(&card->hw, card->devname);
			return -EINVAL;
	}
	
	card->hw_iface.getcfg(card->hw, SDLA_CARDTYPE, &card->type);
	card->hw_iface.getcfg(card->hw, SDLA_ADAPTERTYPE, &card->adptr_type);
	card->hw_iface.getcfg(card->hw, SDLA_ADAPTERSUBTYPE, &card->adptr_subtype);
	card->isr=NULL;

	/* If the current card has already been configured
         * or its a piggyback card, do not try to allocate
         * resources.
	 */
	if (!card->wandev.piggyback && !card->configured){

		err = card->hw_iface.setup(card->hw, conf);
		if (err){
			DEBUG_EVENT("%s: Hardware setup Failed %i\n",
					card->devname,err);
			card->hw_iface.hw_down(card->hw);
			sdla_unregister(&card->hw, card->devname);
			return err;
		}

	     	if (conf->config_id == WANCONFIG_DEBUG){
			if (wanpipe_debug){
				DEBUG_EVENT("%s: More than 2 debugging cards!\n",
						card->devname);
				card->hw_iface.hw_down(card->hw);
				sdla_unregister(&card->hw, card->devname);
				return -EINVAL;
			}
			wanpipe_debug = card;
			wanpipe_debug->u.debug.total_len = 0;
			wanpipe_debug->u.debug.total_num = 0;
			wanpipe_debug->u.debug.current_offset = 
					sizeof(wanpipe_kernel_msg_hdr_t);
			wanpipe_debug->configured = 1;
			wanpipe_debug->wandev.state = WAN_CONNECTED;
			wanpipe_debug->wandev.debug_read = wan_debug_read;
			//wanpipe_debug->adapter_type = card->hw.pci_adapter_type;
			spin_lock_init(&wanpipe_debug->wandev.lock);
			return 0;
		}
		if(conf->card_type == WANOPT_S50X){
			irq = (conf->irq == 2) ? 9 : conf->irq; /* IRQ2 -> IRQ9 */
		}else{
			irq = conf->irq;
		}
               	spin_lock_init(&card->wandev.lock);

		/* request an interrupt vector - note that interrupts may be shared */
		/* when using the S514 PCI adapter */
		if (card->wandev.config_id != WANCONFIG_BSC && 
#if defined(CONFIG_PRODUCT_WANPIPE_USB)
		    card->wandev.config_id != WANCONFIG_USB_ANALOG && 
#endif 
		    card->wandev.config_id != WANCONFIG_POS){ 
			if(request_irq(irq, sdla_isr, 
			      (card->type == SDLA_S508) ? 0: IRQF_SHARED, 
			       wandev->name, card)){

				DEBUG_EVENT("%s: Can't reserve IRQ %d!\n", 
						wandev->name, irq);
				card->hw_iface.hw_down(card->hw);
				sdla_unregister(&card->hw, card->devname);
				return -EINVAL;
			}
		}

	}else{
		DEBUG_EVENT("%s: Card Configured %li or Piggybacking %i!\n",
			wandev->name,card->configured,card->wandev.piggyback);
               	spin_lock_init(&card->wandev.lock);
	} 

	if (!card->configured){

		/* Intialize WAN device data space */
		wandev->irq       = irq;
		wandev->dma       = 0;
		if(conf->card_type == WANOPT_S50X){ 
			card->hw_iface.getcfg(card->hw, SDLA_IOPORT, &wandev->ioport);
		}else{
			card->hw_iface.getcfg(card->hw, SDLA_CPU, &wandev->S514_cpu_no[0]);
			card->hw_iface.getcfg(card->hw, SDLA_SLOT, &wandev->S514_slot_no);
			card->hw_iface.getcfg(card->hw, SDLA_BUS, &wandev->S514_bus_no);
		}
#if 0
		// ALEX_TODAY
		wandev->maddr     = (unsigned long)card->hw.dpmbase;
		wandev->msize     = card->hw.dpmsize;
		wandev->hw_opt[0] = card->hw.type;
		wandev->hw_opt[1] = card->hw.pclk;
		wandev->hw_opt[2] = card->hw.memory;
		wandev->hw_opt[3] = card->hw.fwid;
#endif
	}

	/* Debugging: Read PCI adapter type */
	//card->adapter_type = card->hw.pci_adapter_type;
	card->hw_iface.getcfg(card->hw, SDLA_ADAPTERTYPE, &card->adptr_type);

	/* Protocol-specific initialization */
	switch (conf->config_id) {

	case WANCONFIG_X25:
		DEBUG_EVENT("%s: Starting x25 Protocol Init.\n",
				card->devname);

		INVALID_ADAPTER_CHECK(card,conf->fe_cfg);
		err = wpx_init(card, conf);
		break;
		
	case WANCONFIG_FR:
		DEBUG_EVENT("%s: Starting Frame Relay Protocol Init.\n",
				card->devname);
		err = wpf_init(card, conf);
		break;
		
	case WANCONFIG_PPP:
		DEBUG_EVENT("%s: Starting PPP Protocol Init.\n",
				card->devname);
		err = wpp_init(card, conf);
		break;
	
	case WANCONFIG_SDLC:
		DEBUG_EVENT("%s: Starting SDLC Protocol Init.\n",
				card->devname);
		err = wp_sdlc_init(card, conf);
		break;

	case WANCONFIG_ASYHDLC:
		DEBUG_EVENT("%s: Starting ASY HDLC Protocol Init.\n",
				card->devname);
		err = wp_asyhdlc_init(card, conf);
		break;
		
	case WANCONFIG_CHDLC:
		if (conf->ft1){		
			DEBUG_EVENT("%s: Starting FT1 CSU/DSU Config Driver.\n",
				card->devname);
			err = wpft1_init(card, conf);
			break;
			
		}else{
			DEBUG_EVENT("%s: Starting CHDLC Protocol Init.\n",
					card->devname);
			err = wpc_init(card, conf);
			break;
		}
		break;
		
	case WANCONFIG_MPROT:
			
		DEBUG_EVENT("%s: Starting Multi Protocol Driver Init.\n",
					card->devname);
		err = wp_mprot_init(card,conf);
		break;

	case WANCONFIG_MFR:
		DEBUG_EVENT("%s: Starting Multi-Port Frame Relay Protocol Init.\n",
					card->devname);
		err = wp_hdlc_fr_init(card,conf);
		break;
		
	/* Extra, non-standard WANPIPE Protocols */
	case WANCONFIG_BITSTRM:
		DEBUG_EVENT("%s: Starting Bit Stream Protocol Init.\n",
					card->devname);
		err = wpbit_init(card, conf);
		break;


	case WANCONFIG_EDUKIT:
		DEBUG_EVENT("%s: Starting Educational mode.\n",
								card->devname);
		err = wpedu_init(card, conf);
		break;

	case WANCONFIG_BSC:
		DEBUG_EVENT("%s: Starting Bisync API Protocol Init.\n",
								card->devname);
		INVALID_ADAPTER_CHECK(card,conf->fe_cfg);	
		err = wpbsc_init(card,conf);
		break;
	
	case WANCONFIG_SS7:
		DEBUG_EVENT("%s: Starting SS7 API Protocol Init.\n",
								card->devname);
		err = wpss7_init(card,conf);
		break;

	case WANCONFIG_BSCSTRM:
		DEBUG_EVENT("%s: Starting BiSync Streaming API Init.\n",
				card->devname);
		INVALID_ADAPTER_CHECK(card,conf->fe_cfg);	
		err = wp_bscstrm_init(card,conf);
		break;
	
	case WANCONFIG_ADSL:
		DEBUG_EVENT("%s: Starting ADSL Device Init.\n",
					card->devname);
		err = wp_adsl_init(card,conf);
		break;

	case WANCONFIG_ATM:
		DEBUG_EVENT("%s: Starting ATM Protocol Init.\n",
					card->devname);
		err = wp_atm_init(card,conf);
		break;

	case WANCONFIG_POS:
		DEBUG_EVENT("%s: Starting POS Protocol Init.\n",
					card->devname);
		err = wp_pos_init(card,conf);
		break;

	case WANCONFIG_ADCCP:
		DEBUG_EVENT("%s: Starting ADCCP Protocol Init.\n",
					card->devname);
		err = wp_adccp_init(card,conf);
		break;
		
	case WANCONFIG_AFT:
		DEBUG_EVENT("%s: Starting AFT Hardware Init.\n",
					card->devname);
		err = wp_xilinx_init(card,conf);
		break;

	case WANCONFIG_AFT_TE1:
		DEBUG_EVENT("%s: Starting AFT 2/4/8 Hardware Init.\n",
					card->devname);
		err = wp_aft_te1_init(card,conf);
		break;

	case WANCONFIG_AFT_T116:
		DEBUG_EVENT("%s: Starting Tapping Hardware Init.\n",
					card->devname);
		err = wp_aft_te1_init(card,conf);
		break;

	case WANCONFIG_AFT_56K:
		DEBUG_EVENT("%s: Starting AFT 56K Hardware Init.\n",
					card->devname);
		err = wp_aft_56k_init(card,conf);
		break;

	case WANCONFIG_AFT_ANALOG:
		if (card->adptr_type == AFT_ADPTR_A600) {
			DEBUG_EVENT("%s: Starting AFT B600 Hardware Init.\n",
						card->devname);
			err = wp_aft_a600_init(card,conf);
		} else if (card->adptr_type == AFT_ADPTR_B601) {
			DEBUG_EVENT("%s: Starting AFT B601 Hardware Init.\n",
						card->devname);
			err = wp_aft_a600_init(card,conf);
		} else if (card->adptr_type == AFT_ADPTR_B610) {
			DEBUG_EVENT("%s: Starting AFT B610 Hardware Init.\n",
						card->devname);
			err = wp_aft_a600_init(card,conf);
		} else {
			DEBUG_EVENT("%s: Starting AFT Analog Hardware Init.\n",
						card->devname);
			err = wp_aft_analog_init(card,conf);
		}
		break;

	case WANCONFIG_AFT_ISDN_BRI:
		DEBUG_EVENT("%s: Starting AFT ISDN BRI Hardware Init.\n",
					card->devname);
		err = wp_aft_bri_init(card,conf);
		break;

	case WANCONFIG_AFT_SERIAL:
		DEBUG_EVENT("%s: Starting AFT Serial (V32/RS232) Hardware Init.\n",
					card->devname);
		err = wp_aft_serial_init(card,conf);
		break;

	case WANCONFIG_AFT_TE3:
		DEBUG_EVENT("%s: Starting AFT TE3 Hardware Init.\n",
					card->devname);
		err = wp_aft_te3_init(card,conf);
		break;

#if 0
	case WANCONFIG_LIP_ATM:
		DEBUG_EVENT("%s: Starting ATM MultiProtocol Driver Init.\n",
					card->devname);
		err = wp_lip_atm_init(card,conf);
		break;
#endif		

#if defined(CONFIG_PRODUCT_WANPIPE_USB)
	case WANCONFIG_USB_ANALOG:
		DEBUG_EVENT("%s: Starting USB Hardware Init.\n",
					card->devname);		
		err = wp_usb_init(card,conf);
		break;
#endif
	case WANCONFIG_AFT_GSM:
		DEBUG_EVENT("%s: Starting GSM Hardware Init.\n", card->devname);
		err = wp_aft_w400_init(card, conf);
		break;
	default:
		DEBUG_EVENT("%s: Error, Protocol is not supported %u!\n",
			wandev->name, conf->config_id);

		err = -EPROTONOSUPPORT;
	}

	if (err != 0){
		if (err == -EPROTONOSUPPORT){
			DEBUG_EVENT("%s: Error, Protocol selected has not been compiled!\n",
					card->devname);
			DEBUG_EVENT("%s:      Re-configure the kernel and re-build the modules!\n",
					card->devname);
		}

		if (card->disable_comm){
			card->disable_comm(card);
		}

		release_hw(card);
		wandev->state = WAN_UNCONFIGURED;
		
		card->configured=0;
		sdla_unregister(&card->hw, card->devname);
		if (card->next){
			card->next->next=NULL;
			card->next=NULL;
		}
		return err;
	}

	/*
	 *	Register Protocol proc directory entry 
	 */
	err = wanrouter_proc_add_protocol(wandev);
	
	if (err) {
		DEBUG_EVENT("%s: can't create /proc/net/wanrouter/<protocol> entry!\n",
			card->devname);
		if (card->disable_comm){
			card->disable_comm(card);
		}
		release_hw(card);
		wandev->state = WAN_UNCONFIGURED;

		card->configured=0;
		sdla_unregister(&card->hw, card->devname);
		if (card->next){
			card->next->next=NULL;
			card->next=NULL;
		}

		return err;
	}

  	/* Reserve I/O region and schedule background task */
        if(conf->card_type == WANOPT_S50X && !card->wandev.piggyback){
		u16	io_range;
		card->hw_iface.getcfg(card->hw, SDLA_IORANGE, &io_range);
#if defined(LINUX_2_4) || defined(LINUX_2_6)
		if (!request_region(wandev->ioport, io_range, wandev->name)){
			DEBUG_EVENT("%s: Failed to reserve IO region!\n",
					card->devname);
			wanrouter_proc_delete_protocol(wandev);
			if (card->disable_comm){
				card->disable_comm(card);
			}
			release_hw(card);
			wandev->state = WAN_UNCONFIGURED;

			card->configured=0;
			sdla_unregister(&card->hw, card->devname);
			if (card->next){
				card->next->next=NULL;
				card->next=NULL;
			}
			return -EBUSY;
		}
#else
		request_region(wandev->ioport, io_range, wandev->name);
#endif
	}
	
	/* Only use the polling routine for the X25 protocol */
	WAN_DEBUG_INIT(card);

	card->wandev.critical=0;
	return 0;
}

/*================================================================== 
 * configure_s508_card
 * 
 * For a S508 adapter, check for a possible configuration error in that
 * we are loading an adapter in the same IO port as a previously loaded S508
 * card.
 */ 

static int check_s508_conflicts (sdla_t* card,wandev_conf_t* conf, int *irq)
{
	unsigned long	smp_flags;
	sdla_t*		nxt_card;
	
	if (conf->ioport <= 0) {
		DEBUG_EVENT("%s: can't configure without I/O port address!\n",
					card->wandev.name);
		return -EINVAL;
	}

	if (conf->irq <= 0) {
		DEBUG_EVENT("%s: can't configure without IRQ!\n",
					card->wandev.name);
		return -EINVAL;
	}

	if (test_bit(0,&card->configured))
		return 0;


	/* Check for already loaded card with the same IO port and IRQ 
	 * If found, copy its hardware configuration and use its
	 * resources (i.e. piggybacking)
	 */
	
	for (nxt_card=card_list;nxt_card;nxt_card=nxt_card->list) {

		/* Skip the current card ptr */
		if (nxt_card == card)	
			continue;

		/* Find a card that is already configured with the
		 * same IO Port */
		if (nxt_card->hw == card->hw){

			/* We found a card the card that has same configuration
			 * as us. This means, that we must setup this card in 
			 * piggibacking mode. However, only CHDLC and MPPP protocol
			 * support this setup */
		
			if ((nxt_card->next == NULL) &&
			    ((conf->config_id == WANCONFIG_CHDLC && 
			      nxt_card->wandev.config_id == WANCONFIG_CHDLC) ||
		             (conf->config_id == WANCONFIG_MPPP && 
			      nxt_card->wandev.config_id == WANCONFIG_MPPP)  ||
			     (conf->config_id == WANCONFIG_ASYHDLC && 
			      nxt_card->wandev.config_id == WANCONFIG_ASYHDLC)  ||
			     (conf->config_id == WANCONFIG_BSCSTRM && 
			      nxt_card->wandev.config_id == WANCONFIG_BSCSTRM))){
				
				*irq = nxt_card->wandev.irq;	//ALEX_TODAY nxt_card->hw.irq;
			
				/* The master could already be running, we must
				 * set this as a critical area */
				wan_spin_lock_irq(&nxt_card->wandev.lock, &smp_flags);

				nxt_card->next = card;
				card->next = nxt_card;

				card->wandev.piggyback = WANOPT_YES;

				/* We must initialise the piggiback spin lock here
				 * since isr will try to lock card->next if it
				 * exists */
				spin_lock_init(&card->wandev.lock);
				
				wan_spin_unlock_irq(&nxt_card->wandev.lock, &smp_flags);
				break;
			}else{
				/* Trying to run piggibacking with a wrong protocol */
				DEBUG_EVENT("%s: ERROR: Resource busy, ioport: 0x%x\n"
						 "%s:        This protocol doesn't support\n"
						 "%s:        multi-port operation!\n",
						 card->devname, nxt_card->wandev.ioport,
						 card->devname,card->devname);
				DEBUG_EVENT("%s: Pri Prot = 0x%X  Sec Prot = 0x%X\n",
						card->devname,nxt_card->wandev.config_id,
						conf->config_id);
				return -EEXIST;
			}
		}
	}
	

	/* Make sure I/O port region is available only if we are the
	 * master device.  If we are running in piggibacking mode, 
	 * we will use the resources of the master card */
#ifndef LINUX_2_6
	if (check_region(conf->ioport, SDLA_MAXIORANGE) && 
	    !card->wandev.piggyback) {
		DEBUG_EVENT("%s: I/O region 0x%X - 0x%X is in use!\n",
			card->wandev.name, conf->ioport,
			conf->ioport + SDLA_MAXIORANGE);
		return -EINVAL;
	}
#endif

	return 0;
}

/*================================================================== 
 * configure_s514_card
 * 
 * For a S514 adapter, check for a possible configuration error in that
 * we are loading an adapter in the same slot as a previously loaded S514
 * card.
 */ 


static int check_s514_conflicts(sdla_t* card,wandev_conf_t* conf, int *irq)
{
	unsigned long	smp_flags;
	sdla_t*		nxt_card;

	if (test_bit(0,&card->configured))
		return 0;
	
	/* Check for already loaded card with the same IO port, Bus no and IRQ 
	 * If found, copy its hardware configuration and use its
	 * resources (i.e. piggybacking)
	 */
	
	for (nxt_card=card_list;nxt_card;nxt_card=nxt_card->list) {

		if (nxt_card == card)
			continue;

		if (nxt_card->wandev.state == WAN_UNCONFIGURED)
			continue;

		/* Bug Fix:
		 * If we are using auto PCI slot detection:
		 *   assume that new pci slot is the same as 
		 *   the pci slot of the already configured card
		 *   ie. (nxt_card).
		 * 
		 * The reason for this is, when the pci slot is found
		 * the card->hw.PCI_slot_no is updated with the
		 * found slot number 
		 *
		 * Thus, do not use the PCI_slot_no to detect a
		 * piggyback condition if the auto_pci_cfg option
		 * is enabled.
		 */
		if (nxt_card->hw == card->hw){
		 
			if ((nxt_card->next == NULL) &&
			    ((conf->config_id == WANCONFIG_CHDLC && 
			      nxt_card->wandev.config_id == WANCONFIG_CHDLC) ||
		             (conf->config_id == WANCONFIG_MPPP && 
			      nxt_card->wandev.config_id == WANCONFIG_MPPP)  ||
			     (conf->config_id == WANCONFIG_ASYHDLC && 
			      nxt_card->wandev.config_id == WANCONFIG_ASYHDLC)  ||
			     (conf->config_id == WANCONFIG_BSCSTRM && 
			      nxt_card->wandev.config_id == WANCONFIG_BSCSTRM))){

				*irq = nxt_card->wandev.irq;	//ALEX_TODAY nxt_card->hw.irq;
	
				/* The master could already be running, we must
				 * set this as a critical area */
				wan_spin_lock_irq(&nxt_card->wandev.lock,&smp_flags);
				nxt_card->next = card;
				card->next = nxt_card;

				card->wandev.piggyback = WANOPT_YES;

				/* We must initialise the piggiback spin lock here
				 * since isr will try to lock card->next if it
				 * exists */
				spin_lock_init(&card->wandev.lock);

				wan_spin_unlock_irq(&nxt_card->wandev.lock,&smp_flags);

			}else{
				/* Trying to run piggibacking with a wrong protocol */
				DEBUG_EVENT("%s: ERROR: Resource busy: CPU %c PCISLOT %i\n"
						 "%s:        This protocol doesn't support\n"
						 "%s:        multi-port operation!\n",
						 card->devname,
						 conf->S514_CPU_no[0],conf->PCI_slot_no,
						 card->devname,card->devname);
				DEBUG_EVENT("%s: Pri Prot = 0x%X  Sec Prot = 0x%X\n",
						card->devname,nxt_card->wandev.config_id,
						conf->config_id);
				return -EEXIST;
			}
		}
	}

	return 0;
}


/*================================================================== 
 * configure_s514_card
 * 
 * For a S514 adapter, check for a possible configuration error in that
 * we are loading an adapter in the same slot as a previously loaded S514
 * card.
 */ 


static int check_adsl_conflicts(sdla_t* card,wandev_conf_t* conf, int *irq)
{
	sdla_t*	nxt_card;

	if (test_bit(0,&card->configured))
		return 0;

	
	/* Check for already loaded card with the same IO port, Bus no and IRQ 
	 * If found, copy its hardware configuration and use its
	 * resources (i.e. piggybacking)
	 */
	
	for (nxt_card=card_list;nxt_card;nxt_card=nxt_card->list) {

		if (nxt_card == card)
			continue;

		if (nxt_card->wandev.state == WAN_UNCONFIGURED)
			continue;

		/* Bug Fix:
		 * If we are using auto PCI slot detection:
		 *   assume that new pci slot is the same as 
		 *   the pci slot of the already configured card
		 *   ie. (nxt_card).
		 * 
		 * The reason for this is, when the pci slot is found
		 * the card->hw.PCI_slot_no is updated with the
		 * found slot number 
		 *
		 * Thus, do not use the PCI_slot_no to detect a
		 * piggyback condition if the auto_pci_cfg option
		 * is enabled.
		 */
		if (nxt_card->hw == card->hw){
			/* Trying to run piggibacking with a wrong protocol */
			DEBUG_EVENT("%s: ERROR: Resource busy: CPU %c PCISLOT %i\n"
					 "%s:        This protocol doesn't support\n"
					 "%s:        multi-port operation!\n",
					 card->devname,
					 conf->S514_CPU_no[0],conf->PCI_slot_no,
					 card->devname,card->devname);
			return -EEXIST;
		}
	}

	return 0;
}


/*================================================================== 
 * configure_s514_card
 * 
 * For a S514 adapter, check for a possible configuration error in that
 * we are loading an adapter in the same slot as a previously loaded S514
 * card.
 */ 


static int check_aft_conflicts(sdla_t* card,wandev_conf_t* conf, int *irq)
{
	sdla_t*	nxt_card;

	if (test_bit(0,&card->configured))
		return 0;

	
	/* Check for already loaded card with the same IO port, Bus no and IRQ 
	 * If found, copy its hardware configuration and use its
	 * resources (i.e. piggybacking)
	 */
	
	for (nxt_card=card_list;nxt_card;nxt_card=nxt_card->list) {

		if (nxt_card == card)
			continue;

		if (nxt_card->wandev.state == WAN_UNCONFIGURED)
			continue;

		/* Bug Fix:
		 * If we are using auto PCI slot detection:
		 *   assume that new pci slot is the same as 
		 *   the pci slot of the already configured card
		 *   ie. (nxt_card).
		 * 
		 * The reason for this is, when the pci slot is found
		 * the card->hw.PCI_slot_no is updated with the
		 * found slot number 
		 *
		 * Thus, do not use the PCI_slot_no to detect a
		 * piggyback condition if the auto_pci_cfg option
		 * is enabled.
		 */
		if (nxt_card->hw == card->hw){
                	u16 CPU_no;

                	card->hw_iface.getcfg(card->hw, SDLA_CPU, &CPU_no);
			if (CPU_no == conf->S514_CPU_no[0]){
				/* Trying to run piggibacking with a 
				 * wrong protocol */
				DEBUG_EVENT("%s: ERROR: Resource busy: CPU %c PCISLOT %i\n"
					 "%s:        This protocol doesn't support\n"
					 "%s:        multi-port operation!\n",
					 card->devname,
					 conf->S514_CPU_no[0],conf->PCI_slot_no,
					 card->devname,card->devname);
				return -EEXIST;
			}
		}
	}

	return 0;
}


/*============================================================================
 * Shut down WAN link driver. 
 * o shut down adapter hardware
 * o release system resources.
 *
 * This function is called by the router when device is being unregistered or
 * when it handles ROUTER_DOWN IOCTL.
 */
static int shutdown (wan_device_t* wandev, wandev_conf_t* conf)
{
	sdla_t*		card = NULL;
	int err=0;

	/* sanity checks */
	if ((wandev == NULL) || (wandev->priv == NULL)){
		return -EFAULT;
	}
		
	if (wandev->state == WAN_UNCONFIGURED){
		return 0;
	}

	card = wandev->priv;

	if (card->tty_opt){
		if (card->tty_open){
			DEBUG_EVENT("%s: Shutdown Failed: TTY is still open\n",
				  card->devname);
			return -EBUSY;
		}
	}
	
	wandev->state = WAN_UNCONFIGURED;

	if (wandev->config_id == WANCONFIG_DEBUG){
		wanpipe_debug = NULL;
	}

#ifdef CONFIG_PRODUCT_WANPIPE_ADSL
	if (wandev->config_id == WANCONFIG_ADSL && conf != NULL){
		/* Update ADSL VCI/VPI standart list
		 */
		adsl_vcivpi_update(card, conf);
		conf->config_id = wandev->config_id;
	}
#endif
	set_bit(PERI_CRIT,(void*)&wandev->critical);
	
	/* Stop debugging sequence: debugging task and timer */
	WAN_DEBUG_END(card);

	/* In case of piggibacking, make sure that 
         * we never try to shutdown both devices at the same
         * time, because they depend on one another */

	if (card->disable_comm){
		card->disable_comm(card);
	}

	wandev->state = WAN_UNCONFIGURED;

	/* Release Resources */
	release_hw(card);

	wandev->state = WAN_UNCONFIGURED;

        /* only free the allocated I/O range if not an S514 adapter */
	if (card->type != SDLA_S514 && 
	    card->type != SDLA_ADSL && 
	    card->type != SDLA_AFT &&	
	    wandev->config_id != WANCONFIG_DEBUG &&	
#if defined(CONFIG_PRODUCT_WANPIPE_USB)
	    card->type != SDLA_USB &&
#endif
	    !card->configured){
		u16	io_range;
		card->hw_iface.getcfg(card->hw, SDLA_IORANGE, &io_range);
              	release_region(wandev->ioport, io_range);
	}

	if (!card->configured){
		sdla_unregister(&card->hw, card->devname);
	      	if (card->next){
			sdla_unregister(&card->next->hw, card->next->devname);
			card->next->next=NULL;
			card->next=NULL;
		}
	}
	
	wanrouter_proc_delete_protocol(wandev);

	clear_bit(PERI_CRIT,(void*)&wandev->critical);
	
	return err;
}

static void release_hw (sdla_t *card)
{
	sdla_t *nxt_card;

	/* Check if next device exists */
	if (card->next){
		nxt_card = card->next;
		/* If next device is down then release resources */
		if (nxt_card->wandev.state == WAN_UNCONFIGURED){
			if (card->wandev.piggyback){
				/* If this device is piggyback then use
                                 * information of the master device 
				 */
				DEBUG_EVENT("%s: Piggyback shutting down\n",card->devname);
				if (card->hw_iface.hw_down){
					card->hw_iface.hw_down(card->next->hw);
				}
				if (card->wandev.config_id != WANCONFIG_BSC && 
#if defined(CONFIG_PRODUCT_WANPIPE_USB)
				    card->wandev.config_id != WANCONFIG_USB_ANALOG && 
#endif 
				    card->wandev.config_id != WANCONFIG_POS){ 
       					free_irq(card->wandev.irq, card->next);
				}
				card->configured = 0;
				card->next->configured = 0;
				card->wandev.piggyback = 0;

			}else{
				/* Master device shutting down */
				DEBUG_EVENT("%s: Master shutting down\n",card->devname);
				if (card->hw_iface.hw_down){
					card->hw_iface.hw_down(card->hw);
				}
				if (card->wandev.config_id != WANCONFIG_BSC && 
#if defined(CONFIG_PRODUCT_WANPIPE_USB)
				    card->wandev.config_id != WANCONFIG_USB_ANALOG && 
#endif 
				    card->wandev.config_id != WANCONFIG_POS){ 
					free_irq(card->wandev.irq, card);
				}
				card->configured = 0;
				card->next->configured = 0;
			}

		}else{
			DEBUG_EVENT("%s: Device still running state=%i\n",
				nxt_card->devname,nxt_card->wandev.state);

			card->configured = 1;
		}
	}else{
		DEBUG_EVENT("%s: Master shutting down\n",card->devname);

		if (card->hw_iface.hw_down){
			card->hw_iface.hw_down(card->hw);
		}
		if (card->wandev.config_id != WANCONFIG_BSC && 
#if defined(CONFIG_PRODUCT_WANPIPE_USB)
		    card->wandev.config_id != WANCONFIG_USB_ANALOG && 
#endif 
		    card->wandev.config_id != WANCONFIG_POS &&
		    card->wandev.config_id != WANCONFIG_DEBUG){
       			free_irq(card->wandev.irq, card);
		}
		card->configured = 0;
	}
	return;
}


/*============================================================================
 * Driver I/O control. 
 * o verify arguments
 * o perform requested action
 *
 * This function is called when router handles one of the reserved user
 * IOCTLs.  Note that 'arg' stil points to user address space.
 */
static int ioctl (wan_device_t* wandev, unsigned cmd, unsigned long arg)
{
	sdla_t* card;
	int err;

	/* sanity checks */
	if ((wandev == NULL) || (wandev->priv == NULL))
		return -EFAULT;
	//ALEX-HWABSTR
//	if (wandev->state == WAN_UNCONFIGURED)
//		return -ENODEV;

	card = wandev->priv;

	if (test_bit(SEND_CRIT, (void*)&wandev->critical)) {
		return -EAGAIN;
	}
	
	switch (cmd) {
	case WANPIPE_DUMP:
		err = ioctl_dump(wandev->priv, (void*)arg);
		break;

	case WANPIPE_EXEC:
		err = ioctl_exec(wandev->priv, (void*)arg, cmd);
		break;
	default:
		err = -EINVAL;
	}
 
	return err;
}

/****** Driver IOCTL Handlers ***********************************************/

/*============================================================================
 * Dump adapter memory to user buffer.
 * o verify request structure
 * o copy request structure to kernel data space
 * o verify length/offset
 * o verify user buffer
 * o copy adapter memory image to user buffer
 *
 * Note: when dumping memory, this routine switches curent dual-port memory
 *	 vector, so care must be taken to avoid racing conditions.
 */
static int ioctl_dump (sdla_t* card, sdla_dump_t* u_dump)
{
	sdla_dump_t dump;
	void* data;
	u32	memory;
	int err = 0;

	if(copy_from_user((void*)&dump, (void*)u_dump, sizeof(sdla_dump_t)))
		return -EFAULT;
		
	card->hw_iface.getcfg(card->hw, SDLA_MEMORY, &memory);
	if ((dump.magic != WANPIPE_MAGIC) ||
	    (dump.offset + dump.length > memory)){
		return -EINVAL;
	}
	
	data = wan_kmalloc(dump.length);
	if (data == NULL){
		return -ENOMEM;
	}

	card->hw_iface.peek(card->hw, dump.offset, data, dump.length);

	if(copy_to_user((void *)dump.ptr, data, dump.length)){
		err = -EFAULT;
	}
	wan_free(data);
	return err;
}

/*============================================================================
 * Execute adapter firmware command.
 * o verify request structure
 * o copy request structure to kernel data space
 * o call protocol-specific 'exec' function
 */
static int ioctl_exec (sdla_t* card, sdla_exec_t* u_exec, int cmd)
{
	sdla_exec_t exec;
	int err=0;

	if (card->exec == NULL && cmd == WANPIPE_EXEC){
		return -ENODEV;
	}

	if(copy_from_user((void*)&exec, (void*)u_exec, sizeof(sdla_exec_t)))
		return -EFAULT;

	if ((exec.magic != WANPIPE_MAGIC) || (exec.cmd == NULL))
		return -EINVAL;

	switch (cmd) {
		case WANPIPE_EXEC:	
			err = card->exec(card, exec.cmd, exec.data);
			break;
	}	
	return err;
}

/******* Miscellaneous ******************************************************/

/*============================================================================
 * SDLA Interrupt Service Routine.
 * o acknowledge SDLA hardware interrupt.
 * o call protocol-specific interrupt service routine, if any.
 */

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,20)  
STATIC WAN_IRQ_RETVAL sdla_isr (int irq, void* dev_id, struct pt_regs *regs)
#else
STATIC WAN_IRQ_RETVAL sdla_isr (int irq, void* dev_id)
#endif
{
#define	card	((sdla_t*)dev_id)
	
	if (card && card->type == SDLA_AFT){
		WAN_IRQ_RETVAL_DECL(val); 
		
		spin_lock(&card->wandev.lock);
		if (card->isr){
			WAN_IRQ_CALL(card->isr, (card), val);
		}

#ifdef CONFIG_SMP
		if (!spin_is_locked(&card->wandev.lock)) {
			if (WAN_NET_RATELIMIT()) {
         		DEBUG_ERROR("%s:%d Critical error: driver locking has been corrupted, isr lock left unlocked!\n",
					__FUNCTION__,__LINE__);
			}
		}
#endif

		spin_unlock(&card->wandev.lock);
	
		WAN_IRQ_RETURN(val);
	}

	if (card && card->type == SDLA_ADSL){
		unsigned long flags;
		WAN_IRQ_RETVAL_DECL(val); 
		spin_lock_irqsave(&card->wandev.lock,flags);
		if (card->isr){
			WAN_IRQ_CALL(card->isr, (card), val);
		}

		spin_unlock_irqrestore(&card->wandev.lock,flags);
		WAN_IRQ_RETURN(val);
	}

	if(card->type == SDLA_S514) {	/* handle interrrupt on S514 */
                u32 int_status;
                u16 CPU_no;
                unsigned char card_found_for_IRQ;

                card->hw_iface.getcfg(card->hw, SDLA_CPU, &CPU_no);
		if (card->hw_iface.read_int_stat){
			card->hw_iface.read_int_stat(card->hw, &int_status);
		}

		/* check if the interrupt is for this device */
		if(!((unsigned char)int_status & (IRQ_CPU_A | IRQ_CPU_B))){
			WAN_IRQ_RETURN(IRQ_NONE);
		}

		/* if the IRQ is for both CPUs on the same adapter, */
		/* then alter the interrupt status so as to handle */
		/* one CPU at a time */
		if(((unsigned char)int_status & (IRQ_CPU_A | IRQ_CPU_B))
			== (IRQ_CPU_A | IRQ_CPU_B)) {
			int_status &= (CPU_no == SDLA_CPU_A) ?
				~IRQ_CPU_B : ~IRQ_CPU_A;
		}

		card_found_for_IRQ = 0;
		/* check to see that the CPU number for this device */
		/* corresponds to the interrupt status read */
		switch (CPU_no) {
			case SDLA_CPU_A:
				if((unsigned char)int_status & IRQ_CPU_A){
					card_found_for_IRQ = 1;
				}
			break;

			case SDLA_CPU_B:
				if((unsigned char)int_status & IRQ_CPU_B){
					card_found_for_IRQ = 1;
				}
			break;
		}

		/* exit if the interrupt is for another CPU on the */
		/* same IRQ */
		if(!card_found_for_IRQ){
			WAN_IRQ_RETURN(IRQ_NONE);
		}

		if (!card || 
		   (card->wandev.state == WAN_UNCONFIGURED && !card->configured)){
				DEBUG_EVENT("Received IRQ %d for CPU #%c\n",
					irq, SDLA_GET_CPU(CPU_no));
				DEBUG_EVENT("IRQ for unconfigured adapter\n");
				if (card->hw_iface.intack){
					card->hw_iface.intack(card->hw, int_status);
				}
				WAN_IRQ_RETURN(IRQ_NONE);
		}

		if (card->in_isr) {
			DEBUG_EVENT("%s: interrupt re-entrancy on IRQ %d\n",
				card->devname, card->wandev.irq);
			if (card->hw_iface.intack){
				card->hw_iface.intack(card->hw, int_status);
			}
			WAN_IRQ_RETURN(IRQ_NONE);
		}

		spin_lock(&card->wandev.lock);
		if (card->next){
			spin_lock(&card->next->wandev.lock);
		}
				
		if (card->hw_iface.intack){
			card->hw_iface.intack(card->hw, int_status);
		}

		if (card->isr){
			card->isr(card);
			clear_bit(0,&card->spurious);
		}
		
		if (card->next && card->next->isr){
			card->next->isr(card->next);
			clear_bit(0,&card->next->spurious);
		}

		if (card->next){
			spin_unlock(&card->next->wandev.lock);
		}
		spin_unlock(&card->wandev.lock);
		
		WAN_IRQ_RETURN(WAN_IRQ_HANDLED);

	}else{			/* handle interrupt on S508 adapter */

		if (!card || ((card->wandev.state == WAN_UNCONFIGURED) && !card->configured))
			WAN_IRQ_RETURN(IRQ_NONE);

		if (card->in_isr) {
			DEBUG_EVENT("%s: interrupt re-entrancy on IRQ %d!\n",
				card->devname, card->wandev.irq);
			WAN_IRQ_RETURN(IRQ_NONE);
		}

		spin_lock(&card->wandev.lock);
		if (card->next){
			spin_lock(&card->next->wandev.lock);
		}
	
		if (card->hw_iface.intack){
			card->hw_iface.intack(card->hw, 0x00);
		}
		if (card->isr)
			card->isr(card);
		
		if (card->next){
			spin_unlock(&card->next->wandev.lock);
		}
		spin_unlock(&card->wandev.lock);
	
		WAN_IRQ_RETURN(WAN_IRQ_HANDLED);

	}

	WAN_IRQ_RETURN(WAN_IRQ_NONE);

#undef	card
}

/*============================================================================
 * This routine is called by the protocol-specific modules when network
 * interface is being open.  The only reason we need this, is because we
 * have to call MOD_INC_USE_COUNT, but cannot include 'module.h' where it's
 * defined more than once into the same kernel module.
 */
void wanpipe_open (sdla_t* card)
{
	++card->open_cnt;
	MOD_INC_USE_COUNT;
}

/*============================================================================
 * This routine is called by the protocol-specific modules when network
 * interface is being closed.  The only reason we need this, is because we
 * have to call MOD_DEC_USE_COUNT, but cannot include 'module.h' where it's
 * defined more than once into the same kernel module.
 */
void wanpipe_close (sdla_t* card)
{
	--card->open_cnt;
	MOD_DEC_USE_COUNT;
}

sdla_t * wanpipe_find_card_num (int num)
{
	sdla_t *card;
	
	if (num < 1 || num > ncards)
		return NULL;	

	for (card=card_list;card;card=card->list){
		--num;
		if (!num)
			break;
	}
	return card;
}


int wanpipe_queue_tq (wan_taskq_t *task)
{
	WAN_TASKQ_SCHEDULE(task);
	return 0;
}

int wanpipe_mark_bh (void)
{
	mark_bh(IMMEDIATE_BH);
	return 0;
} 


int change_dev_flags (netdevice_t *dev, unsigned flags)
{
	int err;

#ifdef LINUX_2_6
 	err=dev_change_flags(dev,flags);
#else
	struct ifreq if_info;
	mm_segment_t fs = get_fs();

	memset(&if_info, 0, sizeof(if_info));
	strcpy(if_info.ifr_name, dev->name);
	if_info.ifr_flags = flags;	

	set_fs(get_ds());     /* get user space block */ 
	err = wp_devinet_ioctl(SIOCSIFFLAGS, &if_info);
	set_fs(fs);
#endif
	return err;
}

unsigned long get_ip_address (netdevice_t *dev, int option)
{
	
	struct in_ifaddr *ifaddr;
	struct in_device *in_dev;
	unsigned long ip=0;

	if ((in_dev = in_dev_get(dev)) == NULL){
		return 0;
	}

	if ((ifaddr = in_dev->ifa_list)== NULL ){
		goto wp_get_ip_exit;
	}
	
	switch (option){

	case WAN_LOCAL_IP:
		ip = ifaddr->ifa_local;
		break;
	
	case WAN_POINTOPOINT_IP:
		ip = ifaddr->ifa_address;
		break;	

	case WAN_NETMASK_IP:
		ip = ifaddr->ifa_mask;
		break;

	case WAN_BROADCAST_IP:
		ip = ifaddr->ifa_broadcast;
		break;
	default:
		break;
	}

wp_get_ip_exit:

	in_dev_put(in_dev);

	return ip;
}	

void add_gateway(sdla_t *card, netdevice_t *dev)
{
	mm_segment_t oldfs;
	struct rtentry route;
	int res;

	memset((char*)&route,0,sizeof(struct rtentry));

	((struct sockaddr_in *)
		&(route.rt_dst))->sin_addr.s_addr = 0;
	((struct sockaddr_in *)
		&(route.rt_dst))->sin_family = AF_INET;

	((struct sockaddr_in *)
		&(route.rt_genmask))->sin_addr.s_addr = 0;
	((struct sockaddr_in *) 
		&(route.rt_genmask)) ->sin_family = AF_INET;


	route.rt_flags = 0;  
	route.rt_dev = dev->name;

	oldfs = get_fs();
	set_fs(get_ds());
	res = wp_ip_rt_ioctl(SIOCADDRT,&route);
	set_fs(oldfs);

	if (res == 0){
		DEBUG_EVENT("%s: Gateway added for %s\n",
			card->devname,dev->name);
	}

	return;
}


static int debugging (wan_device_t* wandev)
{
	sdla_t*			card = (sdla_t*)wandev->priv;

	if (wandev->state == WAN_UNCONFIGURED){
		return 0;
	}
	WAN_DEBUG_START(card);

	return 0;
}



static sdla_t * wanpipe_find_card (char *name)
{
	sdla_t *card;
	
	for (card=card_list;card;card=card->list) {
		if (!strcmp(card->devname,name) &&
		    card->wandev.state != WAN_UNCONFIGURED){
			return card;
		}
	}
	return NULL;
}

static sdla_t * wanpipe_find_card_skid (void *sk_id)
{
	sdla_t *card;
	
	for (card=card_list;card;card=card->list) {
		if (card->sk == sk_id)
			return card;
	}
	return NULL;
}


static int bind_api_to_svc (char *devname, void *sk_id)
{
	sdla_t *card;
	int err=-EOPNOTSUPP;

	WAN_ASSERT2((!devname),-EINVAL);
	
	card = wanpipe_find_card(devname);
	if (!card){
		return -ENODEV;
	}

	if (card->bind_api_to_svc){
		err=card->bind_api_to_svc(card,sk_id);
	}

	return err;
}

static int bind_listen_to_link (char *devname, void *sk_id, unsigned short protocol)
{
	sdla_t *card;
	unsigned long flags;

	WAN_ASSERT2((!devname),-EINVAL);
	
	card = wanpipe_find_card(devname);
	if (!card){
		DEBUG_EVENT("%s: Error: Wanpipe device not active! \n",devname);
		return -ENODEV;
	}

	if (card->sk == NULL){
		spin_lock_irqsave(&card->wandev.lock,flags);
		card->sk=sk_id;
		sock_hold(sk_id);
		spin_unlock_irqrestore(&card->wandev.lock,flags);
		return 0;
	}else{

		DEBUG_EVENT("%s: Error: Wanpipe device already bound for listen. Sk=%p\n",
			devname,card->sk);
	}
	
	return -EBUSY;
}

static int unbind_listen_from_link(void *sk_id,unsigned short protocol)
{
	sdla_t *card = wanpipe_find_card_skid(sk_id);	
	unsigned long flags;

	if (!card){
		DEBUG_TEST("%s: Error: No card ptr!\n",__FUNCTION__);
		return -ENODEV;
	}

	if (card->sk == sk_id){
		spin_lock_irqsave(&card->wandev.lock,flags);
		sock_put(card->sk);
		card->sk=NULL;
		spin_unlock_irqrestore(&card->wandev.lock,flags);
		DEBUG_TEST("%s: LISTEN UNBOUND OK\n",__FUNCTION__);
		return 0;
	}

	DEBUG_TEST("%s: Wanpipe listen device already unbounded!\n",
			card->devname);

	return -ENODEV;
}

static int wanpipe_register_fw_to_api(void)
{
	struct wanpipe_fw_register_struct wp_fw_reg;

	memset(&wp_fw_reg,0,sizeof(struct wanpipe_fw_register_struct));

	wp_fw_reg.bind_api_to_svc = bind_api_to_svc; 
	wp_fw_reg.bind_listen_to_link = bind_listen_to_link;
	wp_fw_reg.unbind_listen_from_link = unbind_listen_from_link;

	return register_wanpipe_fw_protocol(&wp_fw_reg); 
}

static int wanpipe_unregister_fw_from_api(void)
{
	unregister_wanpipe_fw_protocol();
	return 0;
}

/*====================================================
 * WANPIPE API COMMON CODE
 */

void wp_debug_func_add(unsigned char *func)
{
	if (DBG_ARRAY_CNT < 100){
		sprintf(DEBUG_ARRAY[DBG_ARRAY_CNT++].name,"%s",func);
	}else{
		DEBUG_EVENT("%s: Error: Max limit for DEBUG_ARRAY reached!\n",__FUNCTION__);
	}
}

void wp_debug_func_print(void)
{
	int i;

	DEBUG_EVENT("\n");
	DEBUG_EVENT("-----------------TRACE START------------------------\n");
	DEBUG_EVENT("\n");

	for (i=0;i<DBG_ARRAY_CNT;i++){
        	DEBUG_EVENT("%s: ---> \n",DEBUG_ARRAY[i].name);
	}

	DEBUG_EVENT("\n");
        DEBUG_EVENT("-----------------TRACE END------------------------\n");
        DEBUG_EVENT("\n");
}


/****** End *********************************************************/
