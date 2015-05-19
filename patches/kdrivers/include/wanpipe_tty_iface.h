
#ifndef	_WP_TTY_IFACE_H
#define	_WP_TTY_IFACE_H

#ifdef WANLIP_DRIVER

typedef struct wp_tty_reg
{
	int (*prot_set_state) (void *, int, unsigned char *, int);
	int (*chan_set_state) (void *, int, unsigned char *, int);
	int (*tx_down)		 (void *, void *);
	int (*rx_up) 	    	 (void *, void *);
	int mtu;
}wp_tty_reg_t;

extern void *wp_register_tty_prot(void *, char *, 
		                   wan_tty_conf_t *, 
				   wp_tty_reg_t *);

extern int wp_unregister_tty_prot(void *);

#endif

