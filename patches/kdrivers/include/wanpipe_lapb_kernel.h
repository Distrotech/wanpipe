/*
 * These are the public elements of the Linux LAPB module.
 */

#include <linux/wanpipe_debug.h>
#include <linux/wanpipe_cfg.h>
#include <linux/if_wanpipe_common.h>
#include "wanpipe_x25_kernel.h"

#ifndef	LAPB_KERNEL_H
#define	LAPB_KERNEL_H

#define	LAPB_OK			0
#define	LAPB_BADTOKEN		1
#define	LAPB_INVALUE		2
#define	LAPB_CONNECTED		3
#define	LAPB_NOTCONNECTED	4
#define	LAPB_REFUSED		5
#define	LAPB_TIMEDOUT		6
#define	LAPB_NOMEM		7

#define	LAPB_STANDARD		0x00
#define	LAPB_EXTENDED		0x01

#define	LAPB_SLP		0x00
#define	LAPB_MLP		0x02

#define	LAPB_DTE		0x00
#define	LAPB_DCE		0x04

enum lapb_ioctl_cmds {
	SIOCC_PC_RESERVED = (SIOC_WANPIPE_DEVPRIVATE),
	SIOCG_LAPB_CONF,
	SIOCS_LAPB_NEW_X25_CONF,
	SIOCS_LAPB_DEL_X25_CONF,
	SIOCG_LAPB_STATUS,
};


#ifdef __KERNEL__

extern int register_lapb_x25_protocol (struct lapb_x25_register_struct * x25_reg);
extern void unregister_lapb_x25_protocol (void);


extern int lapb_x25_register(struct net_device *lapb_dev,
				      	     char *dev_name,
					     struct net_device **new_dev);

extern int lapb_x25_unregister(struct net_device *x25_dev);

extern int lapb_x25_setparms (struct net_device *lapb_dev, 
			struct net_device *x25_dev, 
			struct x25_parms_struct *x25_parms);


struct wanpipe_lapb_register_struct {
	
	unsigned long init;

	int (*lapb_register)(struct net_device *dev, 
	                     char *dev_name, char *hw_name, 
	  		     struct net_device **new_dev);
	int (*lapb_unregister)(struct net_device *dev);
	
	int (*lapb_getparms)(struct net_device *dev, struct lapb_parms_struct *parms);
	int (*lapb_setparms)(struct net_device *dev, struct lapb_parms_struct *parms);
	void (*lapb_mark_bh) (struct net_device *dev);
	void (*lapb_link_up) (struct net_device *dev);
	void (*lapb_link_down) (struct net_device *dev);
	int  (*lapb_rx) (struct net_device *dev, struct sk_buff *skb);
	
	int (*lapb_x25_register)(struct net_device *lapb_dev,
				 char *dev_name,
				 struct net_device **new_dev);

	int (*lapb_x25_unregister)(struct net_device *x25_dev);

	int (*lapb_x25_setparms) (struct net_device *lapb_dev, 
			struct net_device *x25_dev, 
			struct x25_parms_struct *x25_parms);
	int  (*lapb_get_map)(struct net_device*, struct seq_file *);
	void (*lapb_get_active_inactive)(struct net_device*, wp_stack_stats_t*);
};

extern int register_wanpipe_lapb_protocol (struct wanpipe_lapb_register_struct *lapb_reg);
extern void unregister_wanpipe_lapb_protocol (void);

#endif


#endif
