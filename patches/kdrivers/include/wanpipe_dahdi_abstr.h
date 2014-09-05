/******************************************************************************
 * sdla_tdmv.h	
 *
 * Author: 	Alex Feldman  <al.feldman@sangoma.com>
 *
 * Copyright:	(c) 1995-2001 Sangoma Technologies Inc.
 *
 *		This program is free software; you can redistribute it and/or
 *		modify it under the terms of the GNU General Public License
 *		as published by the Free Software Foundation; either version
 *		2 of the License, or (at your option) any later version.
 * ============================================================================
 ******************************************************************************
 */

#ifndef __WANPIPE_DAHDI_ABSTR
# define __WANPIPE_DAHDI_ABSTR

#ifdef __SDLA_TDMV_SRC
# define WP_EXTERN
#else
# define WP_EXTERN extern
#endif


/******************************************************************************
**			  DEFINES and MACROS
******************************************************************************/

#if defined(WAN_KERNEL)

#include "zapcompat.h"
#include "wanpipe_cdev_iface.h"


#ifdef DAHDI_27

# define wp_dahdi_chan_set_rxdisable(chan,val) 
# define wp_dahdi_chan_rxdisable(chan) 0

#else

# define wp_dahdi_chan_set_rxdisable(chan,val) chan->rxdisable=val
# define wp_dahdi_chan_rxdisable(chan) chan->rxdisable 
# define SPANTYPE_DIGITAL_E1 "E1"
# define SPANTYPE_DIGITAL_T1 "T1"

#endif


#ifdef DAHDI_26

#define wp_dahdi_create_device(card, wp) __wp_dahdi_create_device(card,&wp->ddev,&wp->dev)
static __inline int __wp_dahdi_create_device(sdla_t *card, struct dahdi_device **ddev, struct device *dev)
{
    struct pci_dev *pdev;
	int err;

    *ddev = dahdi_create_device();
    if (!*ddev) {
        return -ENOMEM;
    }

    card->hw_iface.getcfg(card->hw, SDLA_PCI_DEV, &pdev);
    err=wanpipe_sys_dev_add(dev, &pdev->dev, card->devname);
    if (err) {
        DEBUG_ERROR("%s: Error Failed to add wanpipe device \n",card->devname);
        dahdi_free_device(*ddev);
        return -EINVAL;
    }

    (*ddev)->manufacturer = kmalloc(WP_DAHDI_MAX_STR_SZ,GFP_KERNEL);
    (*ddev)->hardware_id = kmalloc(WP_DAHDI_MAX_STR_SZ,GFP_KERNEL);
    (*ddev)->devicetype = kmalloc(WP_DAHDI_MAX_STR_SZ,GFP_KERNEL);
    (*ddev)->location = kmalloc(WP_DAHDI_MAX_STR_SZ,GFP_KERNEL);

    return 0;
}

#define wp_dahdi_free_device(wp) __wp_dahdi_free_device(wp->ddev, &wp->dev)
static __inline int __wp_dahdi_free_device(struct dahdi_device *ddev, struct device *dev)
{
	wanpipe_sys_dev_del(dev);

    kfree(ddev->manufacturer);
    kfree(ddev->hardware_id);
    kfree(ddev->devicetype);
    kfree(ddev->location);
    dahdi_free_device(ddev);

    return 0;
}

#define wp_dahdi_register_device(wp) __wp_dahdi_register_device(wp->ddev, &wp->dev,&wp->span)
static __inline int __wp_dahdi_register_device(struct dahdi_device *ddev, struct device *dev, struct dahdi_span *span)
{
    list_add_tail(&span->device_node,
              &ddev->spans);
    return dahdi_register_device(ddev, dev);
}

#define wp_dahdi_unregister_device(wp) __wp_dahdi_unregister_device(wp->ddev)
static __inline void __wp_dahdi_unregister_device(struct dahdi_device *ddev)
{
    dahdi_unregister_device(ddev);
}

#else

#define wp_dahdi_create_device(card,wp) __wp_dahdi_create_device(card,&wp->span)
static __inline int __wp_dahdi_create_device(sdla_t *card, struct zt_span *span)
{
	span->manufacturer = kmalloc(WP_DAHDI_MAX_STR_SZ,GFP_KERNEL);
	return 0;
}
#define wp_dahdi_free_device(wp) __wp_dahdi_free_device(&wp->span)
static __inline int __wp_dahdi_free_device(struct zt_span *span)
{
	kfree(span->manufacturer);
	span->manufacturer=NULL;
	return 0;
}

#define wp_dahdi_register_device(wp) __wp_dahdi_register_device(&wp->span)
static __inline int __wp_dahdi_register_device(struct zt_span *span)
{
    return zt_register(span, 0);
}

#define wp_dahdi_unregister_device(wp) __wp_dahdi_unregister_device(&wp->span)
static __inline void __wp_dahdi_unregister_device(struct zt_span *span)
{
    zt_unregister(span);
}

#endif


#endif /* WAN_KERNEL */

#undef WP_EXTERN

#endif	/* __SDLA_VOIP_H */
