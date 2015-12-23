/*************************************************************************
* wanpipe_cfg_atm.h							 *
*									 *
*	WANPIPE(tm)	Wanpipe ATM Interface configuration	 *
*									 *
* Author:	Alex Feldman <al.feldman@sangoma.com>			 *
*========================================================================*
* Aug 27, 2008	Alex Feldman	Initial version				 *
*									 *
*************************************************************************/

#ifndef __WANPIPE_CFG_ATM_H__
# define __WANPIPE_CFG_ATM_H__

#define ATM_CELL_SIZE 	53

enum {
	ATM_CONNECTED,
	ATM_DISCONNECTED,
	ATM_AIS
};

typedef struct atm_stats {
	unsigned int	rx_valid;
	unsigned int	rx_empty;
	unsigned int	rx_invalid_atm_hdr;	
	unsigned int	rx_invalid_prot_hdr;
	unsigned int	rx_atm_pdu_size;
	unsigned int	rx_chip;
	unsigned int	tx_valid;
	unsigned int	tx_chip;
	unsigned int	rx_congestion;
	unsigned int    rx_clp;
} atm_stats_t;

typedef struct wan_atm_conf_if
{
	unsigned char     encap_mode;
	unsigned short    vci;
	unsigned short    vpi;

	unsigned char	  atm_oam_loopback;
	unsigned char	  atm_oam_loopback_intr;
	unsigned char	  atm_oam_continuity;
	unsigned char	  atm_oam_continuity_intr;
	unsigned char	  atm_arp;
	unsigned char	  atm_arp_intr;

	unsigned short	  mtu;

	unsigned char	 atm_sync_mode;
	unsigned short	 atm_sync_data;
	unsigned char	 atm_sync_offset;
	unsigned short   atm_hunt_timer;

	unsigned char	 atm_cell_cfg;
	unsigned char	 atm_cell_pt;
	unsigned char	 atm_cell_clp;
	unsigned char	 atm_cell_payload;

}wan_atm_conf_if_t;

#endif /* __WANPIPE_CFG_ATM_H__ */
