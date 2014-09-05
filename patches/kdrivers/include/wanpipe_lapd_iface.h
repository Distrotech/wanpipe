/*
 * These are the public elements of the Binary lapd module.
 */

#ifndef	_WP_LAPD_IFACE_H
#define	_WP_LAPD_IFACE_H

/* Mandatory Define here */
#ifdef WANLIP_DRIVER

#define ETH_P_LAPD 0x0030

enum lapd_ioctl_cmds {
	SIOCG_LAPD_CONF,
	SIOCG_LAPD_STATUS,
};

extern void *wp_register_lapd_prot	(void *, 
				   	 char *, 
		                   	 void *, 
				   	 wplip_prot_reg_t *);

extern int wp_unregister_lapd_prot	(void *);

extern void *wp_register_lapd_chan	(void *if_ptr, 
			    		 void *prot_ptr, 
		            		 char *devname, 
			    		 void *cfg_ptr,
			    		 unsigned char type);

extern int wp_unregister_lapd_chan	(void *chan_ptr);

extern int wp_lapd_open	(void *prot_ptr);
extern int wp_lapd_close	(void *prot_ptr);
extern int wp_lapd_rx	(void *prot_ptr, void *skb);
extern int wp_lapd_bh	(void *prot_ptr);
extern int wp_lapd_tx	(void *prot_ptr, void *skb, int type);
extern int wp_lapd_timer	(void *prot_ptr, unsigned int *period, 
		             	 unsigned int carrier_reliable);
extern int wp_lapd_pipemon	(void *prot_ptr, int cmd, int addr, 
				 unsigned char* data, unsigned int *len);

#endif


/* Any public structures shared between LIP and
 * Object Code */




/* This code should go into wanpipe_cfg.h */
typedef struct wan_lapd_if_conf
{
	/* IMPLEMENT USER CONFIG OPTIONS HERE */
	unsigned char data;


}wan_lapd_if_conf_t;

#endif
