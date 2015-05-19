/*****************************************************************************
* wandev.c	WAN Multiprotocol Interface Module. Main code.
*
* Author:	Nenad Corbic
*
* Copyright:	(c) 2008 Sangoma Technologies Inc.
*
*		This program is free software; you can redistribute it and/or
*		modify it under the terms of the GNU General Public License
*		as published by the Free Software Foundation; either version
*		2 of the License, or (at your option) any later version.
* ============================================================================
*/

#include "wanpipe_includes.h"
#include "wanpipe_defines.h"
#include "wanpipe_debug.h"
#include "wanpipe_common.h"
#include "wanpipe_iface.h"
#include "wanpipe.h"
#include "wanpipe_api.h"
#include "if_wanpipe.h"
#include "wanpipe_cdev_iface.h"
#include "sdladrv.h"


/*=================================================
 * Type Defines
 *================================================*/

#define DEBUG_WANDEV DEBUG_TEST


typedef struct wanpipe_wandev
{
	int init;
	int used;
	wanpipe_cdev_t *cdev;
	wan_mutexlock_t lock;
}wanpipe_wandev_t;



/*=================================================
 * Prototypes 
 *================================================*/

static int wp_wandev_open(void *obj);
static int wp_wandev_close(void *obj);
static int wp_wandev_ioctl(void *obj, int cmd, void *data);
static int wanpipe_port_cfg(wanpipe_wandev_t *wdev, void *data);

static int wanpipe_port_management(wanpipe_wandev_t *wdev, void *data);
static int wanpipe_mgmnt_get_hardware_info(wanpipe_wandev_t *wdev, wan_device_t *wandev, port_management_struct_t *usr_port_mgmnt);
static int wanpipe_mgmnt_stop_port(wanpipe_wandev_t *wdev, wan_device_t *wandev, port_management_struct_t *usr_port_mgmnt);
static int wanpipe_mgmnt_start_port(wanpipe_wandev_t *wdev, wan_device_t *wandev, port_management_struct_t *usr_port_mgmnt);
static int wanpipe_mgmnt_get_driver_version(wanpipe_wandev_t *wdev, wan_device_t *wandev, port_management_struct_t *usr_port_mgmnt);


/*=================================================
 * Global Defines 
 *================================================*/


static wanpipe_wandev_t wandev;


static wanpipe_cdev_ops_t wandev_fops = {
	open: wp_wandev_open,
	close: wp_wandev_close,
	ioctl: wp_wandev_ioctl,
};


/*=================================================
 * Functions 
 *================================================*/


int wanpipe_wandev_create(void)
{
	int err=-EINVAL;
	wanpipe_cdev_t *cdev = wan_kmalloc(sizeof(wanpipe_cdev_t));
	if (!cdev) {
		return -ENOMEM;
	}

	memset(cdev,0,sizeof(wanpipe_cdev_t));
	
	memset(&wandev,0,sizeof(wanpipe_wandev_t));

	wan_mutex_lock_init(&wandev.lock, "wandev_mutex_lock");

	cdev->dev_ptr=&wandev;
	wandev.cdev=cdev;
	memcpy(&cdev->ops,&wandev_fops,sizeof(wanpipe_cdev_ops_t));
	
	err=wanpipe_cdev_cfg_ctrl_create(cdev);
	if (err) {
		wan_free(cdev);
		memset(&wandev,0,sizeof(wanpipe_wandev_t));
	}


	DEBUG_WANDEV("%s: WANDEV CREATE \n",__FUNCTION__);

	return err;

}


int wanpipe_wandev_free()
{
	if (wandev.cdev) {
			wanpipe_cdev_free(wandev.cdev);
			wan_free(wandev.cdev);
			wandev.cdev=NULL;
	}

	DEBUG_WANDEV("%s: WANDEV FREE \n",__FUNCTION__);

	return 0;
}

static int wp_wandev_open(void *obj)
{
#if 0
	wanpipe_wandev_t *wdev=(wanpipe_wandev_t*)obj;

	if (wan_test_and_set_bit(0,&wdev->used)) {
		return -EBUSY;
	}
#endif

	DEBUG_WANDEV("%s: WANDEV OPEN \n",__FUNCTION__);

	return 0;
}


static int wp_wandev_close(void *obj)
{
#if 0
	wanpipe_wandev_t *wdev=(wanpipe_wandev_t*)obj;


	wan_clear_bit(0,&wdev->used);
#endif

	DEBUG_WANDEV("%s: WANDEV CLOSE \n",__FUNCTION__);

	return 0;
}


static int wp_wandev_ioctl(void *obj, int cmd, void *data)
{
	wanpipe_wandev_t *wdev=(wanpipe_wandev_t*)obj;
	wan_smp_flag_t flag;
	int err=-EINVAL;

	wan_mutex_lock(&wdev->lock,&flag);

	switch (cmd) {

	case WANPIPE_IOCTL_WRITE:
	case WANPIPE_IOCTL_READ:
		break;

	case WANPIPE_IOCTL_MGMT:
	case WANPIPE_IOCTL_SET_IDLE_TX_BUFFER:
	case WANPIPE_IOCTL_API_POLL:
	case WANPIPE_IOCTL_SET_SHARED_EVENT:
		break;

	case WANPIPE_IOCTL_PORT_MGMT:
		err=wanpipe_port_management(wdev,data);
		break;

	case WANPIPE_IOCTL_PORT_CONFIG:
		err=wanpipe_port_cfg(wdev,data);
		break;
	}
	
	wan_mutex_unlock(&wdev->lock,&flag);

	return err;

}

static int wanpipe_port_management(wanpipe_wandev_t *wdev, void *data)
{
	port_management_struct_t *usr_port_mgmnt=NULL;
	int err=-EINVAL;
	char card_name[100];
	wan_device_t *wandev;
	int cnt;

	DEBUG_WANDEV("%s: WPCTRL PORT MANAGMENT IOCTL \n",__FUNCTION__);

	usr_port_mgmnt = wan_kmalloc(sizeof(port_management_struct_t));
	if (!usr_port_mgmnt) {
		err=-ENOMEM;
		goto wanpipe_port_management_exit;
	}

	err=WAN_COPY_FROM_USER(usr_port_mgmnt, data, sizeof(port_management_struct_t));
	if (err) {
		goto wanpipe_port_management_exit;
	}

	usr_port_mgmnt->operation_status = SANG_STATUS_INVALID_DEVICE;

	if (!usr_port_mgmnt->port_no) {
		err=-ENODEV;
		goto wanpipe_port_management_exit;
	}

	sprintf(card_name,"wanpipe%d",usr_port_mgmnt->port_no);
	wandev=wan_find_wandev_device(card_name);
	if (!wandev) {
		err=-ENODEV;
		goto wanpipe_port_management_exit;
	}	

	switch (usr_port_mgmnt->command_code) {

	case GET_HARDWARE_INFO:
		/* Fill in "hardware_info_t" structure */
		err=wanpipe_mgmnt_get_hardware_info(wdev, wandev, usr_port_mgmnt);
		if (err != 0) {
			err=-ENODEV;
		}
		break;

	case STOP_PORT:
		/* Stop Port. Prior to calling this command all 'handles'
		   to communication interfaces such as WANPIPE1_IF0 must
		   be closed using CloseHandle() system call. */
		err=wanpipe_mgmnt_stop_port(wdev, wandev, usr_port_mgmnt);
		break;

	case START_PORT_VOLATILE_CONFIG:
		/* Start Port. Use configuration stored in a Port's Driver memory buffer.
		   This command runs faster than START_PORT_REGISTRY_CFG.
		   Recommended for use if Port is restarted *a lot* */
		err=wanpipe_mgmnt_start_port(wdev, wandev, usr_port_mgmnt);
		break;

	case START_PORT_REGISTRY_CONFIG:
		/* Start Port. Use configuration stored in the Port's Registry key.
			This command runs slower than START_PORT_VOLATILE_CFG.
			Recommended for use if Port is *not* restarted often (most cases) */

		/* Not supported */
		break;

	case GET_DRIVER_VERSION:
		/* Fill in "DRIVER_VERSION" structure. */
		err = wanpipe_mgmnt_get_driver_version(wdev, wandev, usr_port_mgmnt);
		break;

	case GET_PORT_OPERATIONAL_STATS:
		/* Fill in "port_stats_t" structure. */

		break;

	case FLUSH_PORT_OPERATIONAL_STATS:
		/* Reset port's statistics counters in API driver */
	
		break;

	case WANPIPE_HARDWARE_RESCAN:
        {
		int new_cards=0;

		/* Detect only new cards */
		new_cards = sdla_hw_probe(); 
#if defined(CONFIG_PRODUCT_WANPIPE_USB)       		
		new_cards += sdla_get_hw_usb_adptr_cnt();
#endif
		
		/* Free all so that we erase any hw that is not there
		   any more. And rescan from begining. */
		sdla_hw_probe_free();
		cnt = sdla_hw_probe(); 
#if defined(CONFIG_PRODUCT_WANPIPE_USB)       		
		cnt += sdla_get_hw_usb_adptr_cnt();
#endif
		DEBUG_EVENT("wandev: hardware rescan: total=%i new=%i\n",cnt,new_cards);

		
#if defined(WANPIPE_DEVICE_ALLOC_CNT)
		if (WANPIPE_DEVICE_ALLOC_CNT > cnt) {
			/* The pre defined number of cards still exceed
			 * the newly detected cards, thus no need to 
			 * allocate more */
			new_cards=0;
		}	
#endif

		/* Only add new cards */
		if (new_cards) {
			int i;
			for (i = 0; i < new_cards; i++){
				 if (sdladrv_callback.add_device){
					sdladrv_callback.add_device();
				}
			}
		}
		usr_port_mgmnt->port_no=cnt;
		
		err=0;
		}
		break;
	}



wanpipe_port_management_exit:

	if (err==0) {
		usr_port_mgmnt->operation_status = SANG_STATUS_SUCCESS;
	}

	if (usr_port_mgmnt) {
		int res=WAN_COPY_TO_USER(data, usr_port_mgmnt, sizeof(port_management_struct_t));
		if (err==0 && res) {
			err=res;
		}

		wan_free(usr_port_mgmnt);
		usr_port_mgmnt=NULL;
	}

	return err;



}


static int wanpipe_port_cfg(wanpipe_wandev_t *wdev, void *data)
{
	wanpipe_port_cfg_t *usr_port_cfg=NULL;
	int err=-EINVAL;
	wan_device_t *wandev;
	char card_name[100];

	if (!wdev || !data) {
		DEBUG_ERROR("%s: Error: Invalid wdev or data ptrs \n",__FUNCTION__);
		return -EFAULT;
	}

	usr_port_cfg = wan_kmalloc(sizeof(wanpipe_port_cfg_t));
	if (!usr_port_cfg) {
		err = -ENOMEM;
		goto wanpipe_port_cfg_exit;
	}

	err=WAN_COPY_FROM_USER(usr_port_cfg,
							data,
							sizeof(wanpipe_port_cfg_t));

	if (err) {
		goto wanpipe_port_cfg_exit;
	}
	
	usr_port_cfg->operation_status = SANG_STATUS_INVALID_DEVICE;

	if (!usr_port_cfg->port_no) {
		goto wanpipe_port_cfg_exit;
	}

	sprintf(card_name,"wanpipe%d",usr_port_cfg->port_no);


	wandev=wan_find_wandev_device(card_name);
	if (!wandev) {
		err=-ENODEV;
		goto wanpipe_port_cfg_exit;
	}	

	switch (usr_port_cfg->command_code) {

	case GET_PORT_VOLATILE_CONFIG:

		if (!wandev->port_cfg) {
			err = -ENODEV;
			goto wanpipe_port_cfg_exit;
		}

		err=0;
		memcpy(usr_port_cfg,wandev->port_cfg, sizeof(wanpipe_port_cfg_t));


		/* Get current Port configuration stored in a Port's Driver memory buffer */
		break;

	case SET_PORT_VOLATILE_CONFIG:
		/* Set new Port configuration in a Port's Driver memory buffer.
		   Prior to using this command Port must be stopped with STOP_PORT command.
		   This command will not update Port's Property Pages in the Device Manager.
		   Recommended for use if Port is restarted *a lot*. */

		if (!wandev->port_cfg) {
			wandev->port_cfg = wan_kmalloc(sizeof(wanpipe_port_cfg_t));
			if (!wandev->port_cfg) {
				err = -ENOMEM;
				goto wanpipe_port_cfg_exit;
			}
		}

		err=0;
		memcpy(wandev->port_cfg,usr_port_cfg,sizeof(wanpipe_port_cfg_t));

		break;

	}

	if (err == 0) {
       	 usr_port_cfg->operation_status = SANG_STATUS_SUCCESS;   
	}

wanpipe_port_cfg_exit:

	if (usr_port_cfg) {
	
		err= WAN_COPY_TO_USER(data,
						 usr_port_cfg,
						 sizeof(wanpipe_port_cfg_t));

		wan_free(usr_port_cfg);
		usr_port_cfg=NULL;
	}

	return err;


}


static int wanpipe_mgmnt_get_driver_version(wanpipe_wandev_t *wdev, wan_device_t *wandev, port_management_struct_t *usr_port_mgmnt)
{
	wan_driver_version_t *drv_ver = (wan_driver_version_t*)usr_port_mgmnt->data;

	drv_ver->major=WANPIPE_VERSION_MAJOR;
	drv_ver->minor=WANPIPE_VERSION_MINOR;
	drv_ver->minor1=WANPIPE_VERSION_MINOR1;
	drv_ver->minor2=WANPIPE_VERSION_MINOR2;

	usr_port_mgmnt->operation_status = 0;
        return 0;
}



static int wanpipe_mgmnt_get_hardware_info(wanpipe_wandev_t *wdev, wan_device_t *wandev, port_management_struct_t *usr_port_mgmnt)
{
	hardware_info_t *hw_info = (hardware_info_t*)usr_port_mgmnt->data;
	sdla_t *card=(sdla_t*)wandev->priv;

	if (!card) {
		DEBUG_EVENT("%s: Error: Card pointer not associated with Device %s!\n",
				__FUNCTION__,wandev->name);
		return -EINVAL;
	}

	return sdla_get_hwinfo(hw_info,usr_port_mgmnt->port_no);

}



static int wanpipe_mgmnt_stop_port(wanpipe_wandev_t *wdev,  wan_device_t *wandev, port_management_struct_t *usr_port_mgmnt)
{
	/* Bring down all network interfaces */
	return wan_device_shutdown(wandev, NULL);
}

static int wanpipe_mgmnt_start_port(wanpipe_wandev_t *wdev, wan_device_t *wandev, port_management_struct_t *usr_port_mgmnt)
{
	int err=-EINVAL;
	int i;
	wanpipe_port_cfg_t *port_cfg=wandev->port_cfg;

	if (!wandev->port_cfg) {
		/* No configuration present */
		return -EINVAL;
	}

	err=wan_device_setup(wandev, &port_cfg->wandev_conf, 0);
	if (err) {
		DEBUG_EVENT("%s: Error: Failed to configure device\n",
					__FUNCTION__);
		return err;
	}

	for (i = 0; i < port_cfg->num_of_ifs; i++) {
		err=wan_device_new_if (wandev, &port_cfg->if_cfg[i], 0);
		if (err) {
			DEBUG_EVENT("%s: Error: Failed to configure interface %i\n",
					__FUNCTION__,i);
			return err;
		}
	}

	err=-ENODEV;
	for (i = 0; i < port_cfg->num_of_ifs; i++) {
		netdevice_t *dev = wan_dev_get_by_name(port_cfg->if_cfg[i].name);
		if (dev) {
			dev_put(dev);
			rtnl_lock();
			err=dev_change_flags(dev,(dev->flags|IFF_UP));
			rtnl_unlock();
		} else {
			err=-ENODEV;
		}
		
		if (err) {
			break;
		}
	}

	/* Bring up all network interfaces */
	
	return err;
}





