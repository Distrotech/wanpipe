/*************************************************************************
* wanpipe_cfg_hdlc.h							 *
*									 *
*	WANPIPE(tm)	Wanpipe HDLC Interface configuration	 *
*									 *
* Author:	Alex Feldman <al.feldman@sangoma.com>			 *
*========================================================================*
* Aug 27, 2008	Alex Feldman	Initial version				 *
*									 *
*************************************************************************/

#ifndef __WANPIPE_CFG_HDLC_H__
# define __WANPIPE_CFG_HDLC_H__


typedef struct wan_lip_hdlc_if_conf
{
	/* IMPLEMENT USER CONFIG OPTIONS HERE */
	unsigned char seven_bit_hdlc;
	unsigned char rx_crc_bytes;
	unsigned char lineidle;

}wan_lip_hdlc_if_conf_t;

#endif /* __WANPIPE_CFG_HDLC_H__ */
