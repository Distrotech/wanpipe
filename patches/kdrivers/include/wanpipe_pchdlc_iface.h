
#ifndef __PCHDLC_H__
#define __PCHDLC_H__



void *wp_register_sppp_prot(void *link_ptr, char *devname, 
			      wan_sppp_if_conf_t *cfg, 
			      wp_sppp_reg_t *prot_reg);



extern void wp_sppp_attach (struct ppp_device *pd);
extern void wp_sppp_detach (struct net_device *dev);
extern void wp_sppp_input (struct net_device *dev, struct sk_buff *m);
extern int  wp_sppp_do_ioctl (struct net_device *dev, struct ifreq *ifr, int cmd);
extern struct sk_buff *sppp_dequeue (struct net_device *dev);
extern int sppp_isempty (struct net_device *dev);
extern void sppp_flush (struct net_device *dev);
extern int wp_sppp_open (struct net_device *dev);
extern int wp_sppp_reopen (struct net_device *dev);
extern int wp_sppp_close (struct net_device *dev);









#endif

