/*
 * These are the public elements of the Binary xmtp2 module.
 */

#ifndef	_WP_XMTP2_IFACE_H
#define	_WP_XMTP2_IFACE_H

/* Mandatory Define here */
#ifdef WANLIP_DRIVER

enum xmtp2_ioctl_cmds {
	SIOCG_XMTP2_CONF,
	SIOCG_XMTP2_STATUS,
};

extern void *wp_register_xmtp2_prot	(void *, 
				   	 char *, 
		                   	 void *, 
				   	 wplip_prot_reg_t *);

extern int wp_unregister_xmtp2_prot	(void *);

extern void *wp_register_xmtp2_chan	(void *if_ptr, 
			    		 void *prot_ptr, 
		            		 char *devname, 
			    		 void *cfg_ptr,
			    		 unsigned char type);

extern int wp_unregister_xmtp2_chan	(void *chan_ptr);

extern int wp_xmtp2_open	(void *prot_ptr);
extern int wp_xmtp2_close	(void *prot_ptr);
extern int wp_xmtp2_rx	(void *prot_ptr, void *skb);
extern int wp_xmtp2_bh	(void *prot_ptr);
extern int wp_xmtp2_tx	(void *prot_ptr, void *skb, int type);
extern int wp_xmtp2_timer	(void *prot_ptr, unsigned int *period, 
		             	 unsigned int carrier_reliable);
extern int wp_xmtp2_pipemon	(void *prot_ptr, int cmd, int addr, 
				 unsigned char* data, unsigned int *len);

#endif


/* Any public structures shared between LIP and
 * Object Code */




/* This code should go into wanpipe_cfg.h */
typedef struct wan_xmtp2_if_conf
{
	/* IMPLEMENT USER CONFIG OPTIONS HERE */
	unsigned char data;


}wan_xmtp2_if_conf_t;

#endif
