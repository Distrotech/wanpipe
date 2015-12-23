/*************************************************************************
* wanpipe_cfg_lip.h							 *
*									 *
*	WANPIPE(tm)	Wanpipe LIP Interface configuration	 *
*									 *
* Author:	Alex Feldman <al.feldman@sangoma.com>			 *
*========================================================================*
* Aug 27, 2008	Alex Feldman	Initial version				 *
*									 *
*************************************************************************/

#ifndef __WANPIPE_CFG_LIP_H__
# define __WANPIPE_CFG_LIP_H__


enum {
	WPLIP_RAW,
	WPLIP_IP,
	WPLIP_IPV6,
	WPLIP_IPX,
	WPLIP_FR_ARP,
	WPLIP_PPP,
	WPLIP_FR,
	WPLIP_ETH,
	WPLIP_LAPD
};


typedef struct wplip_prot_reg
{
	int (*prot_set_state) (void *, int, unsigned char *, int);
	int (*chan_set_state) (void *, int, unsigned char *, int);
	int (*tx_link_down)   (void *, void *);
	int (*tx_chan_down)   (void *, void *);
	int (*rx_up) 	      (void *, void *, int type);
	unsigned int (*get_ipv4_addr)(void *, int type);
	int (*set_ipv4_addr)(void *, 
			     unsigned int,
			     unsigned int,
			     unsigned int,
			     unsigned int);
	int (*kick_task)     (void *);
#if 0
	int (*set_hw_idle_frame) (void *, unsigned char *, int);
#endif
	int mtu;
} wplip_prot_reg_t;

#endif /* __WANPIPE_CFG_LIP_H__ */
