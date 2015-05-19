/*
 * These are the public elements of the Binary katm module.
 */

#ifndef	_WP_KATM_IFACE_H
#define	_WP_KATM_IFACE_H

/* Mandatory Define here */
#ifdef WANLIP_DRIVER

#define ETH_P_LAPD 0x0030

enum katm_ioctl_cmds {
	SIOCG_LAPD_CONF,
	SIOCG_LAPD_STATUS,
};

extern void *wp_register_katm_prot	(void *, 
				   	 char *, 
		                   	 void *, 
				   	 wplip_prot_reg_t *);

extern int wp_unregister_katm_prot	(void *);

extern void *wp_register_katm_chan	(void *if_ptr, 
			    		 void *prot_ptr, 
		            		 char *devname, 
			    		 void *cfg_ptr,
			    		 unsigned char type);

extern int wp_unregister_katm_chan	(void *chan_ptr);

extern int wp_katm_open	(void *prot_ptr);
extern int wp_katm_close	(void *prot_ptr);
extern int wp_katm_rx	(void *prot_ptr, void *skb);
extern int wp_katm_bh	(void *prot_ptr);
extern int wp_katm_tx	(void *prot_ptr, void *skb, int type);
extern int wp_katm_timer	(void *prot_ptr, unsigned int *period, 
		             	 unsigned int carrier_reliable);
extern int wp_katm_pipemon	(void *prot_ptr, int cmd, int addr, 
				 unsigned char* data, unsigned int *len);
				 
extern int wpkatm_bh (void *prot_ptr);

#endif


/* Any public structures shared between LIP and
 * Object Code */




/* This code should go into wanpipe_cfg.h */
typedef struct wan_katm_if_conf
{
	/* IMPLEMENT USER CONFIG OPTIONS HERE */
	unsigned char data;


}wan_katm_if_conf_t;

#endif
