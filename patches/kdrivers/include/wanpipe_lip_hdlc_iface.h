/*
 * These are the public elements of the Binary lip_hdlc module.
 */

#ifndef	_WP_LIP_HDLC_IFACE_H
#define	_WP_LIP_HDLC__IFACE_H

/* Mandatory Define here */
#ifdef WANLIP_DRIVER

extern void *wp_register_lip_hdlc_prot	(void *, 
				   	 char *, 
		                   	 void *, 
				   	 wplip_prot_reg_t *);

extern int wp_unregister_lip_hdlc_prot	(void *);

extern void *wp_register_lip_hdlc_chan	(void *if_ptr, 
			    		 void *prot_ptr, 
		            		 char *devname, 
			    		 void *cfg_ptr,
			    		 unsigned char type);

extern int wp_unregister_lip_hdlc_chan	(void *chan_ptr);

extern int wp_lip_hdlc_open	(void *prot_ptr);
extern int wp_lip_hdlc_close	(void *prot_ptr);
extern int wp_lip_hdlc_rx	(void *prot_ptr, void *skb);
extern int wp_lip_hdlc_bh	(void *prot_ptr);
extern int wp_lip_hdlc_tx	(void *prot_ptr, void *skb, int type);
extern int wp_lip_hdlc_timer	(void *prot_ptr, unsigned int *period, 
		             	 unsigned int carrier_reliable);
extern int wp_lip_hdlc_pipemon	(void *prot_ptr, int cmd, int addr, 
				 unsigned char* data, unsigned int *len);

#endif


#endif
