/*************************************************************************
* wanpipe_cfg_sppp.h							 *
*									 *
*	WANPIPE(tm)	Wanpipe SPPP Interface configuration	 *
*									 *
* Author:	Alex Feldman <al.feldman@sangoma.com>			 *
*========================================================================*
* Aug 27, 2008	Alex Feldman	Initial version				 *
*									 *
*************************************************************************/

#ifndef __WANPIPE_CFG_SPPP_H__
# define __WANPIPE_CFG_SPPP_H__

#define WAN_AUTHNAMELEN 64

/* PPP IP Mode Options */
#define	WANOPT_PPP_STATIC	0
#define	WANOPT_PPP_HOST		1
#define	WANOPT_PPP_PEER		2

/* used by both PPP and CHDLC in LIP layer */
typedef struct sppp_parms_struct {

	unsigned char dynamic_ip;/* Static/Host/Peer (the same as ip_mode) */
	unsigned int  local_ip;
	unsigned int  remote_ip;
	
	unsigned int  pp_auth_timer;
 	unsigned int  sppp_keepalive_timer;/* if 0, ignore keepalive for link status */
	unsigned int  pp_timer;

	unsigned char pap;
	unsigned char chap;
	unsigned char userid[WAN_AUTHNAMELEN];	
	unsigned char passwd[WAN_AUTHNAMELEN];	
#define SYSTEM_NAME_LEN	31
	unsigned char sysname[SYSTEM_NAME_LEN];
	
	unsigned int  gateway;
	unsigned char ppp_prot;

	/* CHDLC */
	unsigned int keepalive_err_margin;
	unsigned char disable_magic;
}wan_sppp_if_conf_t;


#endif /* __WANPIPE_CFG_SPPP_H__ */
