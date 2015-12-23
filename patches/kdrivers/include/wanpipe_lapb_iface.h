/*
 * These are the public elements of the Linux LAPB module.
 */

#ifndef	_WP_LAPB_IFACE_H
#define	_WP_LAPB_IFACE_H


enum lapb_ioctl_cmds {
	SIOCG_LAPB_CONF,
	SIOCS_LAPB_NEW_X25_CONF,
	SIOCS_LAPB_DEL_X25_CONF,
	SIOCG_LAPB_STATUS,
};


#if 0
typedef struct wp_lapb_reg
{
	int (*prot_state_change) (void *, int, unsigned char *, int);
	int (*chan_state_change) (void *, int, unsigned char *, int);
	int (*tx_down)		 (void *, void *);
	int (*rx_up) 	    	 (void *, void *);
}wp_lapb_reg_t;
#endif

extern void *wp_register_lapb_prot(void *, char *, 
		                   void *, 
				   wplip_prot_reg_t *);
extern void *wp_register_lapd_prot(void *, char *, 
		                   void *, 
				   wplip_prot_reg_t *);
extern int wp_unregister_lapb_prot(void *);

extern void *wp_register_lapb_chan_prot(void *callback_dev_ptr, 
		       		 void *link_ptr,
		       		 char *devname, 
		       		 void *cfg_ptr,
		       		 unsigned char type);

extern int wp_unregister_lapb_chan_prot(void *dev_ptr);


extern int wp_lapb_open(void *lapb_ptr);
extern int wp_lapb_close(void *lapb_ptr);
extern int wp_lapb_rx(void *lapb_ptr, void *skb);
extern int wp_lapb_bh(void *lapb_ptr);
extern int wp_lapb_tx(void *lapb_ptr, void *skb, int type);
extern int wp_lapb_timer(void *lapb_ptr, unsigned int *period, unsigned int);

#endif
