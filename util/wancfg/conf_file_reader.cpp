/***************************************************************************
                          conf_file_reader.cpp  -  description
                             -------------------
    begin                : Thu Mar 4 2004
    copyright            : (C) 2004 by David Rokhvarg
    email                : davidr@sangoma.com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "list_element_chan_def.h"
#include "conf_file_reader.h"
#include "text_box_help.h"
#include "wancfg.h"

#define DBG_CONF_FILE_READER	  1
#define DBG_CONF_FILE_READER_AFT  1

#define smemof(TYPE, MEMBER) offsetof(TYPE,MEMBER),(sizeof(((TYPE *)0)->MEMBER))

static void SIZEOFASSERT (key_word_t* dtab, unsigned int type_size)
{
      if (dtab->size != type_size) {
		fprintf(stderr,"\n==========CRITICAL ERROR============\n\n");
		fprintf(stderr,"Size Mismatch: Type Size %i != %i\n",dtab->size, type_size);
		fprintf(stderr,"======================================\n\n");
		fprintf(stderr,"Plese email /var/log/wanrouter file to Sangoma Support\n");
		fprintf(stderr,"Email to techdesk@sangoma.com\n");
		fprintf(stderr,"======================================\n\n");
      }
}


//////////////////////////////////////////////////////////////////////////////////////////

/****** Global Data *********************************************************/

/*
 * Configuration structure description tables.
 * WARNING:	These tables MUST be kept in sync with corresponding data
 *		structures defined in linux/wanrouter.h
 */
key_word_t common_conftab[] =	/* Common configuration parameters */
{
  { "IOPORT",     smemof(wandev_conf_t, ioport),     DTYPE_UINT },
  { "MEMADDR",    smemof(wandev_conf_t, maddr),       DTYPE_UINT },
  { "MEMSIZE",    smemof(wandev_conf_t, msize),       DTYPE_UINT },
  { "IRQ",        smemof(wandev_conf_t, irq),         DTYPE_UINT },
  { "DMA",        smemof(wandev_conf_t, dma),         DTYPE_UINT },
  { "CARD_TYPE",  smemof(wandev_conf_t, card_type),	DTYPE_UINT },
  { "S514CPU",    smemof(wandev_conf_t, S514_CPU_no), DTYPE_STR },
  { "PCISLOT",    smemof(wandev_conf_t, PCI_slot_no), DTYPE_UINT },
  { "PCIBUS", 	  smemof(wandev_conf_t, pci_bus_no),	DTYPE_UINT },
  { "AUTO_PCISLOT",smemof(wandev_conf_t, auto_hw_detect), DTYPE_UCHAR },
  { "AUTO_DETECT",smemof(wandev_conf_t, auto_hw_detect), DTYPE_UCHAR },
  { "COMMPORT",   smemof(wandev_conf_t, comm_port),   DTYPE_UINT },

  /* Front-End parameters */
  { "FE_MEDIA",    offsetof(wandev_conf_t, fe_cfg)+smemof(sdla_fe_cfg_t, media), DTYPE_UCHAR },
  { "FE_LCODE",    offsetof(wandev_conf_t, fe_cfg)+smemof(sdla_fe_cfg_t, lcode), DTYPE_UCHAR },
  { "FE_FRAME",    offsetof(wandev_conf_t, fe_cfg)+smemof(sdla_fe_cfg_t, frame), DTYPE_UCHAR },
  { "FE_LINE",     offsetof(wandev_conf_t, fe_cfg)+smemof(sdla_fe_cfg_t, line_no),  DTYPE_UINT },
  { "FE_TXTRISTATE", offsetof(wandev_conf_t, fe_cfg)+smemof(sdla_fe_cfg_t, tx_tristate_mode),  DTYPE_UCHAR },

  /* Front-End parameters (old style) */
  { "MEDIA",    offsetof(wandev_conf_t, fe_cfg)+smemof(sdla_fe_cfg_t, media), DTYPE_UCHAR },
  { "LCODE",    offsetof(wandev_conf_t, fe_cfg)+smemof(sdla_fe_cfg_t, lcode), DTYPE_UCHAR },
  { "FRAME",    offsetof(wandev_conf_t, fe_cfg)+smemof(sdla_fe_cfg_t, frame), DTYPE_UCHAR },
  /* T1/E1 Front-End parameters */
  { "TE_LBO", offsetof(wandev_conf_t, fe_cfg)+offsetof(sdla_fe_cfg_t, cfg) + smemof(sdla_te_cfg_t, lbo), DTYPE_UCHAR },
  { "TE_CLOCK",offsetof(wandev_conf_t, fe_cfg)+offsetof(sdla_fe_cfg_t, cfg) + smemof(sdla_te_cfg_t, te_clock),
	 DTYPE_UCHAR },
  { "TE_ACTIVE_CH",offsetof(wandev_conf_t, fe_cfg)+offsetof(sdla_fe_cfg_t, cfg) + smemof(sdla_te_cfg_t, active_ch),
	 DTYPE_UINT },
  { "TE_HIGHIMPEDANCE", offsetof(wandev_conf_t, fe_cfg)+offsetof(sdla_fe_cfg_t, cfg) + smemof(sdla_te_cfg_t, high_impedance_mode), DTYPE_UCHAR },
  { "TE_RX_SLEVEL", offsetof(wandev_conf_t, fe_cfg)+offsetof(sdla_fe_cfg_t, cfg) + smemof(sdla_te_cfg_t, rx_slevel), DTYPE_INT },

  { "TE_REF_CLOCK",offsetof(wandev_conf_t, fe_cfg)+offsetof(sdla_fe_cfg_t, cfg) + smemof(sdla_te_cfg_t, te_ref_clock),
	 DTYPE_UCHAR },
  { "TE_SIG_MODE",     offsetof(wandev_conf_t, fe_cfg)+offsetof(sdla_fe_cfg_t, cfg) + smemof(sdla_te_cfg_t, sig_mode), DTYPE_UINT },
  /* T1/E1 Front-End parameters (old style) */
  { "LBO",           offsetof(wandev_conf_t, fe_cfg)+offsetof(sdla_fe_cfg_t, cfg) + smemof(sdla_te_cfg_t, lbo), DTYPE_UCHAR },
  { "ACTIVE_CH",	offsetof(wandev_conf_t, fe_cfg)+offsetof(sdla_fe_cfg_t, cfg) + smemof(sdla_te_cfg_t, active_ch), DTYPE_UINT },
  { "HIGHIMPEDANCE", offsetof(wandev_conf_t, fe_cfg)+offsetof(sdla_fe_cfg_t, cfg) + smemof(sdla_te_cfg_t, high_impedance_mode), DTYPE_UCHAR },
  /* T3/E3 Front-End parameters */
  { "TE3_FRACTIONAL",offsetof(wandev_conf_t, fe_cfg)+offsetof(sdla_fe_cfg_t, cfg) + smemof(sdla_te3_cfg_t, fractional), DTYPE_UCHAR },
  { "TE3_RDEVICE",	offsetof(wandev_conf_t, fe_cfg)+offsetof(sdla_fe_cfg_t, cfg) + smemof(sdla_te3_cfg_t, rdevice_type), DTYPE_UCHAR },
  { "TE3_FCS",	offsetof(wandev_conf_t, fe_cfg)+offsetof(sdla_fe_cfg_t, cfg) + smemof(sdla_te3_cfg_t, fcs), DTYPE_UINT },

  { "TE3_RXEQ",	offsetof(wandev_conf_t, fe_cfg)+offsetof(sdla_fe_cfg_t, cfg) + offsetof(sdla_te3_cfg_t, liu_cfg) + smemof(sdla_te3_liu_cfg_t, rx_equal), DTYPE_UINT },

  { "TE3_TAOS",	offsetof(wandev_conf_t, fe_cfg)+offsetof(sdla_fe_cfg_t, cfg) + offsetof(sdla_te3_cfg_t, liu_cfg) + smemof(sdla_te3_liu_cfg_t, taos), DTYPE_UINT },
  
  { "TE3_LBMODE",	offsetof(wandev_conf_t, fe_cfg)+offsetof(sdla_fe_cfg_t, cfg) + offsetof(sdla_te3_cfg_t, liu_cfg) + smemof(sdla_te3_liu_cfg_t, lb_mode), DTYPE_UINT },
  
  { "TE3_TXLBO",	offsetof(wandev_conf_t, fe_cfg)+offsetof(sdla_fe_cfg_t, cfg) + offsetof(sdla_te3_cfg_t, liu_cfg) + smemof(sdla_te3_liu_cfg_t, tx_lbo), DTYPE_UINT },

   { "TE3_CLOCK",       offsetof(wandev_conf_t, fe_cfg)+offsetof(sdla_fe_cfg_t, cfg) + smemof(sdla_te3_cfg_t, clock), DTYPE_UCHAR },
   
  { "TDMV_LAW",    offsetof(wandev_conf_t, fe_cfg)+smemof(sdla_fe_cfg_t, tdmv_law), DTYPE_UINT },

  { "TDMV_OPERMODE",    offsetof(wandev_conf_t, fe_cfg)+offsetof(sdla_fe_cfg_t, cfg) + smemof(sdla_remora_cfg_t, opermode_name), DTYPE_STR },
  { "RM_BATTTHRESH",    offsetof(wandev_conf_t, fe_cfg)+offsetof(sdla_fe_cfg_t, cfg) + smemof(sdla_remora_cfg_t, battthresh), DTYPE_UINT },
  { "RM_BATTDEBOUNCE",  offsetof(wandev_conf_t, fe_cfg)+offsetof(sdla_fe_cfg_t, cfg) + smemof(sdla_remora_cfg_t, battdebounce), DTYPE_UINT },

//  { "RM_NETWORK_SYNC",  offsetof(wandev_conf_t, fe_cfg)+offsetof(sdla_fe_cfg_t, cfg) + smemof(sdla_remora_cfg_t, network_sync), DTYPE_UINT },
  { "RM_NETWORK_SYNC",  offsetof(wandev_conf_t, fe_cfg)+smemof(sdla_fe_cfg_t, network_sync), DTYPE_UINT },
  { "FE_NETWORK_SYNC",  offsetof(wandev_conf_t, fe_cfg)+smemof(sdla_fe_cfg_t, network_sync), DTYPE_UINT },

  { "RM_BRI_CLOCK_MASTER",  offsetof(wandev_conf_t, fe_cfg)+offsetof(sdla_fe_cfg_t, cfg) + smemof(sdla_bri_cfg_t, clock_mode), DTYPE_UCHAR },

    /* TDMV parameters */
  { "TDMV_SPAN",     offsetof(wandev_conf_t, tdmv_conf)+smemof(wan_tdmv_conf_t, span_no), DTYPE_UINT},
  { "TDMV_DCHAN",    offsetof(wandev_conf_t, tdmv_conf)+smemof(wan_tdmv_conf_t, dchan),   DTYPE_UINT},
  { "TDMV_HW_DTMF",  offsetof(wandev_conf_t, tdmv_conf)+smemof(wan_tdmv_conf_t, hw_dtmf),    DTYPE_UCHAR},
          

  { "BAUDRATE",   smemof(wandev_conf_t, bps),         DTYPE_UINT },
  { "MTU",        smemof(wandev_conf_t, mtu),         DTYPE_UINT },
  { "UDPPORT",    smemof(wandev_conf_t, udp_port),    DTYPE_UINT },
  { "TTL",	  smemof(wandev_conf_t, ttl),		DTYPE_UCHAR },
  { "INTERFACE",  smemof(wandev_conf_t, electrical_interface),   DTYPE_UCHAR },
  { "CLOCKING",   smemof(wandev_conf_t, clocking),    DTYPE_UCHAR },
  { "LINECODING", smemof(wandev_conf_t, line_coding), DTYPE_UCHAR },
  { "CONNECTION", smemof(wandev_conf_t, connection),  DTYPE_UCHAR },
  { "LINEIDLE",   smemof(wandev_conf_t, line_idle),   DTYPE_UCHAR },
  { "OPTION1",    smemof(wandev_conf_t, hw_opt[0]),   DTYPE_UINT },
  { "OPTION2",    smemof(wandev_conf_t, hw_opt[1]),   DTYPE_UINT },
  { "OPTION3",    smemof(wandev_conf_t, hw_opt[2]),   DTYPE_UINT },
  { "OPTION4",    smemof(wandev_conf_t, hw_opt[3]),   DTYPE_UINT },
  { "FIRMWARE",   smemof(wandev_conf_t, data_size),   DTYPE_FILENAME },
  { "RXMODE",     smemof(wandev_conf_t, read_mode),   DTYPE_CHAR },
  { "RECEIVE_ONLY", smemof(wandev_conf_t, receive_only), DTYPE_UCHAR},	  
  { "BACKUP",     smemof(wandev_conf_t, backup), DTYPE_UCHAR},
  { "TTY",        smemof(wandev_conf_t, tty), DTYPE_UCHAR},
  { "TTY_MAJOR",  smemof(wandev_conf_t, tty_major), DTYPE_UINT},
  { "TTY_MINOR",  smemof(wandev_conf_t, tty_minor), DTYPE_UINT},
  { "TTY_MODE",   smemof(wandev_conf_t, tty_mode), DTYPE_UINT},
  { "IGNORE_FRONT_END",  smemof(wandev_conf_t, ignore_front_end_status), DTYPE_UCHAR},


  { "MAX_RX_QUEUE", smemof(wandev_conf_t, max_rx_queue), DTYPE_UINT},
  { "LMI_TRACE_QUEUE", smemof(wandev_conf_t, max_rx_queue), DTYPE_UINT},
  { NULL, 0, 0 }
};

/*
key_word_t ppp_conftab[] =	// PPP-specific configuration (on Firmware PPP)
 {
  { "RESTARTTIMER",   smemof(wan_ppp_conf_t, restart_tmr),   DTYPE_UINT },
  { "AUTHRESTART",    smemof(wan_ppp_conf_t, auth_rsrt_tmr), DTYPE_UINT },
  { "AUTHWAIT",       smemof(wan_ppp_conf_t, auth_wait_tmr), DTYPE_UINT },

  { "DTRDROPTIMER",   smemof(wan_ppp_conf_t, dtr_drop_tmr),  DTYPE_UINT },
  { "CONNECTTIMEOUT", smemof(wan_ppp_conf_t, connect_tmout), DTYPE_UINT },
  { "CONFIGURERETRY", smemof(wan_ppp_conf_t, conf_retry),    DTYPE_UINT },
  { "TERMINATERETRY", smemof(wan_ppp_conf_t, term_retry),    DTYPE_UINT },
  { "MAXCONFREJECT",  smemof(wan_ppp_conf_t, fail_retry),    DTYPE_UINT },
  { "AUTHRETRY",      smemof(wan_ppp_conf_t, auth_retry),    DTYPE_UINT },
  { "AUTHENTICATOR",  smemof(wan_ppp_conf_t, authenticator), DTYPE_UCHAR},
  { "IP_MODE",        smemof(wan_ppp_conf_t, ip_mode),       DTYPE_UCHAR},
  { NULL, 0, 0 }
};
*/

#if 0
key_word_t synch_ppp_conftab[] =	/* PPP-on LIP layer configuration */
 {
  { "IP_MODE",        	smemof(wan_sppp_if_conf_t, dynamic_ip),       DTYPE_UCHAR},
  { "IP_MODE",		smemof(wan_sppp_if_conf_t, dynamic_ip),       DTYPE_UCHAR},
  { "PAP",   	     	smemof(wan_sppp_if_conf_t, pap),		DTYPE_UCHAR},
  { "CHAP",          	smemof(wan_sppp_if_conf_t, chap),		DTYPE_UCHAR},
  { "USERID",        	smemof(wan_sppp_if_conf_t, userid),	 	DTYPE_STR},
  { "PASSWD",        	smemof(wan_sppp_if_conf_t, passwd),		DTYPE_STR},
  { "SYSNAME",       	smemof(wan_sppp_if_conf_t, sysname),		DTYPE_STR},
  { NULL, 0, 0 }
};
#endif

key_word_t sppp_conftab[] =	/* PPP-CHDLC (in LIP layer!!!) specific configuration */
 {
  { "IP_MODE",        smemof(wan_sppp_if_conf_t, dynamic_ip),       DTYPE_UCHAR},
	
  { "AUTH_TIMER",     smemof(wan_sppp_if_conf_t, pp_auth_timer),    DTYPE_UINT},
  { "KEEPALIVE_TIMER",smemof(wan_sppp_if_conf_t, sppp_keepalive_timer),    DTYPE_UINT},
  { "PPP_TIMER",      smemof(wan_sppp_if_conf_t, pp_timer),    DTYPE_UINT},
 
  { "PAP",	      smemof(wan_sppp_if_conf_t, pap),    DTYPE_UCHAR},		
  { "CHAP",	      smemof(wan_sppp_if_conf_t, chap),    DTYPE_UCHAR},		
  { "CHAP",	      smemof(wan_sppp_if_conf_t, chap),    DTYPE_UCHAR},		

  { "USERID",         smemof(wan_sppp_if_conf_t, userid), 	DTYPE_STR},
  { "PASSWD",         smemof(wan_sppp_if_conf_t, passwd),	DTYPE_STR},
  { "SYSNAME",        smemof(wan_sppp_if_conf_t, sysname),	DTYPE_STR},

  { "KEEPALIVE_ERROR_MARGIN", smemof(wan_sppp_if_conf_t, keepalive_err_margin),    DTYPE_UINT },

  { NULL, 0, 0 }
};

#if 0
//not used by MP_CHDLC!! - it is all in 'wanif_conf_t'
key_word_t chdlc_conftab[] =	/* Cisco HDLC-specific configuration - in FIRMWARE!!*/
 {
  { "IGNORE_DCD", smemof(wan_chdlc_conf_t, ignore_dcd), DTYPE_UCHAR},
  { "IGNORE_CTS", smemof(wan_chdlc_conf_t, ignore_cts), DTYPE_UCHAR},
  { "IGNORE_KEEPALIVE", smemof(wan_chdlc_conf_t, ignore_keepalive), DTYPE_UCHAR},
  { "HDLC_STREAMING", smemof(wan_chdlc_conf_t, hdlc_streaming), DTYPE_UCHAR},
  { "KEEPALIVE_TX_TIMER", smemof(wan_chdlc_conf_t, keepalive_tx_tmr), DTYPE_UINT },
  { "KEEPALIVE_RX_TIMER", smemof(wan_chdlc_conf_t, keepalive_rx_tmr), DTYPE_UINT },
  { "KEEPALIVE_ERR_MARGIN", smemof(wan_chdlc_conf_t, keepalive_err_margin),    DTYPE_UINT },
  { "SLARP_TIMER", smemof(wan_chdlc_conf_t, slarp_timer),    DTYPE_UINT },
  { "FAST_ISR", smemof(wan_chdlc_conf_t,fast_isr), DTYPE_UCHAR },
  { NULL, 0, 0 }
};
#endif

key_word_t fr_conftab[] =	/* Frame relay-specific configuration */
{
  { "SIGNALLING",    smemof(wan_fr_conf_t, signalling),     DTYPE_UINT },
  { "T391", 	     smemof(wan_fr_conf_t, t391),           DTYPE_UINT },
  { "T392",          smemof(wan_fr_conf_t, t392),           DTYPE_UINT },
  { "N391",	     smemof(wan_fr_conf_t, n391),           DTYPE_UINT },
  { "N392",          smemof(wan_fr_conf_t, n392),           DTYPE_UINT },
  { "N393",          smemof(wan_fr_conf_t, n393),           DTYPE_UINT },
  { "FR_ISSUE_FS",   smemof(wan_fr_conf_t, issue_fs_on_startup),DTYPE_UCHAR },
  { "NUMBER_OF_DLCI",    smemof(wan_fr_conf_t, dlci_num),       DTYPE_UINT },
  { "STATION",       smemof(wan_fr_conf_t, station),     DTYPE_UCHAR },
  { NULL, 0, 0 }
};

key_word_t xilinx_conftab[] =	/* Xilinx specific configuration */
{
  { "MRU",     	     smemof(wan_xilinx_conf_t, mru),       DTYPE_USHORT },
  { "DMA_PER_CH",    smemof(wan_xilinx_conf_t, dma_per_ch),      DTYPE_USHORT },
  { "RBS",    	     smemof(wan_xilinx_conf_t, rbs),      DTYPE_UCHAR },
  { "DATA_MUX_MAP",  smemof(wan_xilinx_conf_t, data_mux_map),      DTYPE_UINT },
  { "RX_CRC_BYTES",  smemof(wan_xilinx_conf_t, rx_crc_bytes), DTYPE_UINT},
  { NULL, 0, 0 }
};

key_word_t edukit_conftab[] =	
{
  { NULL, 0, 0 }
};

key_word_t bitstrm_conftab[] =	/* Bitstreaming specific configuration */
{
 /* Bit strm options */
  { "SYNC_OPTIONS",	       smemof(wan_bitstrm_conf_t, sync_options), DTYPE_UINT },
  { "RX_SYNC_CHAR",	       smemof(wan_bitstrm_conf_t, rx_sync_char), DTYPE_UCHAR},
  { "MONOSYNC_TX_TIME_FILL_CHAR", smemof(wan_bitstrm_conf_t, monosync_tx_time_fill_char), DTYPE_UCHAR},
  { "MAX_LENGTH_TX_DATA_BLOCK", smemof(wan_bitstrm_conf_t,max_length_tx_data_block), DTYPE_UINT},
  { "RX_COMPLETE_LENGTH",       smemof(wan_bitstrm_conf_t,rx_complete_length), DTYPE_UINT},
  { "RX_COMPLETE_TIMER",        smemof(wan_bitstrm_conf_t,rx_complete_timer), DTYPE_UINT},	
  { "RBS_CH_MAP",		smemof(wan_bitstrm_conf_t,rbs_map), DTYPE_UINT},
  { NULL, 0, 0 }
};

key_word_t atm_conftab[] =
{
  {"ATM_SYNC_MODE",	       smemof(wan_atm_conf_t,atm_sync_mode), DTYPE_USHORT },
  {"ATM_SYNC_DATA",	       smemof(wan_atm_conf_t,atm_sync_data), DTYPE_USHORT },
  {"ATM_SYNC_OFFSET", 	       smemof(wan_atm_conf_t,atm_sync_offset), DTYPE_USHORT },
  {"ATM_HUNT_TIMER", 	       smemof(wan_atm_conf_t,atm_hunt_timer), DTYPE_USHORT },
  {"ATM_CELL_CFG", 	       smemof(wan_atm_conf_t,atm_cell_cfg), DTYPE_UCHAR },
  {"ATM_CELL_PT", 	       smemof(wan_atm_conf_t,atm_cell_pt), DTYPE_UCHAR },
  {"ATM_CELL_CLP", 	       smemof(wan_atm_conf_t,atm_cell_clp), DTYPE_UCHAR },
  {"ATM_CELL_PAYLOAD", 	       smemof(wan_atm_conf_t,atm_cell_payload), DTYPE_UCHAR },
  { NULL, 0, 0 }
};

key_word_t atm_if_conftab[] =
{
  {"ENCAPMODE",        smemof(wan_atm_conf_if_t, encap_mode), DTYPE_UCHAR },
  {"VCI", 	       smemof(wan_atm_conf_if_t, vci), DTYPE_USHORT },
  {"VPI", 	       smemof(wan_atm_conf_if_t, vpi), DTYPE_USHORT },

  {"OAM_LOOPBACK",	smemof(wan_atm_conf_if_t,atm_oam_loopback),  DTYPE_UCHAR },
  {"OAM_LOOPBACK_INT",  smemof(wan_atm_conf_if_t,atm_oam_loopback_intr),  DTYPE_UCHAR },
  {"OAM_CC_CHECK",	smemof(wan_atm_conf_if_t,atm_oam_continuity),  DTYPE_UCHAR },
  {"OAM_CC_CHECK_INT",	smemof(wan_atm_conf_if_t,atm_oam_continuity_intr),  DTYPE_UCHAR },
  {"ATMARP",		smemof(wan_atm_conf_if_t,atm_arp),  DTYPE_UCHAR },
  {"ATMARP_INT",	smemof(wan_atm_conf_if_t,atm_arp_intr),  DTYPE_UCHAR },
  {"MTU",		smemof(wan_atm_conf_if_t,mtu),  DTYPE_USHORT },
  { NULL, 0, 0 }
};

key_word_t xilinx_if_conftab[] =
{
  { "SIGNALLING",     smemof(wan_xilinx_conf_if_t, signalling),  DTYPE_UINT },
  { "STATION",        smemof(wan_xilinx_conf_if_t, station),     DTYPE_UCHAR },
  { "SEVEN_BIT_HDLC", smemof(wan_xilinx_conf_if_t, seven_bit_hdlc), DTYPE_CHAR },
  { "MRU",     	smemof(wan_xilinx_conf_if_t, mru),  DTYPE_UINT },
  { "MTU",     	smemof(wan_xilinx_conf_if_t, mtu),  DTYPE_UINT },
  { "IDLE_FLAG",     smemof(wan_xilinx_conf_if_t, idle_flag),  DTYPE_UCHAR},
  { "DATA_MUX",    smemof(wan_xilinx_conf_if_t, data_mux),  DTYPE_UCHAR},
  { NULL, 0, 0 }
};

key_word_t bitstrm_if_conftab[] =
{
  {"MAX_TX_QUEUE", smemof(wan_bitstrm_conf_if_t,max_tx_queue_size), DTYPE_UINT },
  {"MAX_TX_UP_SIZE", smemof(wan_bitstrm_conf_if_t,max_tx_up_size), DTYPE_UINT },
  {"SEVEN_BIT_HDLC", smemof(wan_bitstrm_conf_if_t,seven_bit_hdlc), DTYPE_CHAR },
  { NULL, 0, 0 }
};

key_word_t adsl_conftab[] =
{
  {"ENCAPMODE", 	smemof(wan_adsl_conf_t,EncapMode), DTYPE_UCHAR },
  /*Backward compatibility*/
  {"RFC1483MODE", 	smemof(wan_adsl_conf_t,EncapMode), DTYPE_UCHAR },
  {"RFC2364MODE", 	smemof(wan_adsl_conf_t,EncapMode), DTYPE_UCHAR },

  {"VCI", 		smemof(wan_adsl_conf_t,Vci), DTYPE_USHORT },
  /*Backward compatibility*/
  {"RFC1483VCI",	smemof(wan_adsl_conf_t,Vci), DTYPE_USHORT },
  {"RFC2364VCI",	smemof(wan_adsl_conf_t,Vci), DTYPE_USHORT },

  {"VPI", 		smemof(wan_adsl_conf_t,Vpi), DTYPE_USHORT },
  /*Backward compatibility*/
  {"RFC1483VPI", 	smemof(wan_adsl_conf_t,Vpi), DTYPE_USHORT },
  {"RFC2364VPI", 	smemof(wan_adsl_conf_t,Vpi), DTYPE_USHORT },

  {"VERBOSE", 		smemof(wan_adsl_conf_t,Verbose), DTYPE_UCHAR },
  /*Backward compatibility*/
  {"DSL_INTERFACE", 	smemof(wan_adsl_conf_t,Verbose), DTYPE_UCHAR },

  {"RXBUFFERCOUNT", 	smemof(wan_adsl_conf_t,RxBufferCount), DTYPE_USHORT },
  {"TXBUFFERCOUNT", 	smemof(wan_adsl_conf_t,TxBufferCount), DTYPE_USHORT },

  {"ADSLSTANDARD",      smemof(wan_adsl_conf_t,Standard), DTYPE_USHORT },
  {"ADSLTRELLIS",       smemof(wan_adsl_conf_t,Trellis), DTYPE_USHORT },
  {"ADSLTXPOWERATTEN",  smemof(wan_adsl_conf_t,TxPowerAtten), DTYPE_USHORT },
  {"ADSLCODINGGAIN",    smemof(wan_adsl_conf_t,CodingGain), DTYPE_USHORT },
  {"ADSLMAXBITSPERBIN", smemof(wan_adsl_conf_t,MaxBitsPerBin), DTYPE_USHORT },
  {"ADSLTXSTARTBIN",    smemof(wan_adsl_conf_t,TxStartBin), DTYPE_USHORT },
  {"ADSLTXENDBIN",      smemof(wan_adsl_conf_t,TxEndBin), DTYPE_USHORT },
  {"ADSLRXSTARTBIN",    smemof(wan_adsl_conf_t,RxStartBin), DTYPE_USHORT },
  {"ADSLRXENDBIN",      smemof(wan_adsl_conf_t,RxEndBin), DTYPE_USHORT },
  {"ADSLRXBINADJUST",   smemof(wan_adsl_conf_t,RxBinAdjust), DTYPE_USHORT },
  {"ADSLFRAMINGSTRUCT", smemof(wan_adsl_conf_t,FramingStruct), DTYPE_USHORT },
  {"ADSLEXPANDEDEXCHANGE",      smemof(wan_adsl_conf_t,ExpandedExchange), DTYPE_USHORT },
  {"ADSLCLOCKTYPE",     smemof(wan_adsl_conf_t,ClockType), DTYPE_USHORT },
  {"ADSLMAXDOWNRATE",   smemof(wan_adsl_conf_t,MaxDownRate), DTYPE_USHORT },

  /*Backward compatibility*/
  {"GTISTANDARD",      smemof(wan_adsl_conf_t,Standard), DTYPE_USHORT },
  {"GTITRELLIS",       smemof(wan_adsl_conf_t,Trellis), DTYPE_USHORT },
  {"GTITXPOWERATTEN",  smemof(wan_adsl_conf_t,TxPowerAtten), DTYPE_USHORT },
  {"GTICODINGGAIN",    smemof(wan_adsl_conf_t,CodingGain), DTYPE_USHORT },
  {"GTIMAXBITSPERBIN", smemof(wan_adsl_conf_t,MaxBitsPerBin), DTYPE_USHORT },
  {"GTITXSTARTBIN",    smemof(wan_adsl_conf_t,TxStartBin), DTYPE_USHORT },
  {"GTITXENDBIN",      smemof(wan_adsl_conf_t,TxEndBin), DTYPE_USHORT },
  {"GTIRXSTARTBIN",    smemof(wan_adsl_conf_t,RxStartBin), DTYPE_USHORT },
  {"GTIRXENDBIN",      smemof(wan_adsl_conf_t,RxEndBin), DTYPE_USHORT },
  {"GTIRXBINADJUST",   smemof(wan_adsl_conf_t,RxBinAdjust), DTYPE_USHORT },
  {"GTIFRAMINGSTRUCT", smemof(wan_adsl_conf_t,FramingStruct), DTYPE_USHORT },
  {"GTIEXPANDEDEXCHANGE",      smemof(wan_adsl_conf_t,ExpandedExchange), DTYPE_USHORT },
  {"GTICLOCKTYPE",     smemof(wan_adsl_conf_t,ClockType), DTYPE_USHORT },
  {"GTIMAXDOWNRATE",   smemof(wan_adsl_conf_t,MaxDownRate), DTYPE_USHORT },

  {"ATM_AUTOCFG", 	smemof(wan_adsl_conf_t,atm_autocfg), DTYPE_UCHAR },
  {"ADSL_WATCHDOG",	smemof(wan_adsl_conf_t,atm_watchdog), DTYPE_UCHAR },
  { NULL, 0, 0 }
};

key_word_t bscstrm_conftab[]=
{
  {"BSCSTRM_ADAPTER_FR" , smemof(wan_bscstrm_conf_t,adapter_frequency),DTYPE_UINT},
  {"BSCSTRM_MTU", 	smemof(wan_bscstrm_conf_t,max_data_length),DTYPE_USHORT},
  {"BSCSTRM_EBCDIC" ,  smemof(wan_bscstrm_conf_t,EBCDIC_encoding),DTYPE_USHORT},
  {"BSCSTRM_RB_BLOCK_TYPE", smemof(wan_bscstrm_conf_t,Rx_block_type),DTYPE_USHORT},
  {"BSCSTRM_NO_CONSEC_PAD_EOB", smemof(wan_bscstrm_conf_t,no_consec_PADs_EOB), DTYPE_USHORT},
  {"BSCSTRM_NO_ADD_LEAD_TX_SYN_CHARS", smemof(wan_bscstrm_conf_t,no_add_lead_Tx_SYN_chars),DTYPE_USHORT},
  {"BSCSTRM_NO_BITS_PER_CHAR", smemof(wan_bscstrm_conf_t,no_bits_per_char),DTYPE_USHORT},
  {"BSCSTRM_PARITY", smemof(wan_bscstrm_conf_t,parity),DTYPE_USHORT},
  {"BSCSTRM_MISC_CONFIG_OPTIONS",  smemof(wan_bscstrm_conf_t,misc_config_options),DTYPE_USHORT},
  {"BSCSTRM_STATISTICS_OPTIONS", smemof(wan_bscstrm_conf_t,statistics_options),  DTYPE_USHORT},
  {"BSCSTRM_MODEM_CONFIG_OPTIONS", smemof(wan_bscstrm_conf_t,modem_config_options), DTYPE_USHORT},
  { NULL, 0, 0 }
};


key_word_t ss7_conftab[] =
{
  {"LINE_CONFIG_OPTIONS", smemof(wan_ss7_conf_t,line_cfg_opt),  DTYPE_UINT },
  {"MODEM_CONFIG_OPTIONS",smemof(wan_ss7_conf_t,modem_cfg_opt), DTYPE_UINT },
  {"MODEM_STATUS_TIMER",  smemof(wan_ss7_conf_t,modem_status_timer), DTYPE_UINT },
  {"API_OPTIONS",	  smemof(wan_ss7_conf_t,api_options),	  DTYPE_UINT },
  {"PROTOCOL_OPTIONS",	  smemof(wan_ss7_conf_t,protocol_options), DTYPE_UINT },
  {"PROTOCOL_SPECIFICATION", smemof(wan_ss7_conf_t,protocol_specification), DTYPE_UINT },
  {"STATS_HISTORY_OPTIONS", smemof(wan_ss7_conf_t,stats_history_options), DTYPE_UINT },
  {"MAX_LENGTH_MSU_SIF", smemof(wan_ss7_conf_t,max_length_msu_sif), DTYPE_UINT },
  {"MAX_UNACKED_TX_MSUS", smemof(wan_ss7_conf_t,max_unacked_tx_msus), DTYPE_UINT },
  {"LINK_INACTIVITY_TIMER", smemof(wan_ss7_conf_t,link_inactivity_timer), DTYPE_UINT },
  {"T1_TIMER",		  smemof(wan_ss7_conf_t,t1_timer),	  DTYPE_UINT },
  {"T2_TIMER",		  smemof(wan_ss7_conf_t,t2_timer),	  DTYPE_UINT },
  {"T3_TIMER",		  smemof(wan_ss7_conf_t,t3_timer),	  DTYPE_UINT },
  {"T4_TIMER_EMERGENCY",  smemof(wan_ss7_conf_t,t4_timer_emergency), DTYPE_UINT },
  {"T4_TIMER_NORMAL",  smemof(wan_ss7_conf_t,t4_timer_normal), DTYPE_UINT },
  {"T5_TIMER",		  smemof(wan_ss7_conf_t,t5_timer),	  DTYPE_UINT },
  {"T6_TIMER",		  smemof(wan_ss7_conf_t,t6_timer),	  DTYPE_UINT },
  {"T7_TIMER",		  smemof(wan_ss7_conf_t,t7_timer),	  DTYPE_UINT },
  {"T8_TIMER",		  smemof(wan_ss7_conf_t,t8_timer),	  DTYPE_UINT },
  {"N1",		  smemof(wan_ss7_conf_t,n1),	  DTYPE_UINT },
  {"N2",		  smemof(wan_ss7_conf_t,n2),	  DTYPE_UINT },
  {"TIN",		  smemof(wan_ss7_conf_t,tin),	  DTYPE_UINT },
  {"TIE",		  smemof(wan_ss7_conf_t,tie),	  DTYPE_UINT },
  {"SUERM_ERROR_THRESHOLD", smemof(wan_ss7_conf_t,suerm_error_threshold), DTYPE_UINT},
  {"SUERM_NUMBER_OCTETS", smemof(wan_ss7_conf_t,suerm_number_octets), DTYPE_UINT},
  {"SUERM_NUMBER_SUS", smemof(wan_ss7_conf_t,suerm_number_sus), DTYPE_UINT},
  {"SIE_INTERVAL_TIMER", smemof(wan_ss7_conf_t,sie_interval_timer), DTYPE_UINT},
  {"SIO_INTERVAL_TIMER", smemof(wan_ss7_conf_t,sio_interval_timer), DTYPE_UINT},
  {"SIOS_INTERVAL_TIMER", smemof(wan_ss7_conf_t,sios_interval_timer), DTYPE_UINT},
  {"FISU_INTERVAL_TIMER", smemof(wan_ss7_conf_t,fisu_interval_timer), DTYPE_UINT},
  { NULL, 0, 0 }
};

key_word_t x25_conftab[] =	/* X.25-specific configuration */
{
  { "LOWESTPVC",    smemof(wan_x25_conf_t, lo_pvc),       DTYPE_UINT },
  { "HIGHESTPVC",   smemof(wan_x25_conf_t, hi_pvc),       DTYPE_UINT },
  { "LOWESTSVC",    smemof(wan_x25_conf_t, lo_svc),       DTYPE_UINT },
  { "HIGHESTSVC",   smemof(wan_x25_conf_t, hi_svc),       DTYPE_UINT },
  { "HDLCWINDOW",   smemof(wan_x25_conf_t, hdlc_window),  DTYPE_UINT },
  { "PACKETWINDOW", smemof(wan_x25_conf_t, pkt_window),   DTYPE_UINT },
  { "CCITTCOMPAT",  smemof(wan_x25_conf_t, ccitt_compat), DTYPE_UINT },
  { "X25CONFIG",    smemof(wan_x25_conf_t, x25_conf_opt), DTYPE_UINT }, 	
  { "LAPB_HDLC_ONLY", smemof(wan_x25_conf_t, LAPB_hdlc_only), DTYPE_UCHAR },
  { "CALL_SETUP_LOG", smemof(wan_x25_conf_t, logging), DTYPE_UCHAR },
  { "OOB_ON_MODEM",   smemof(wan_x25_conf_t, oob_on_modem), DTYPE_UCHAR},
  { "T1", 	    smemof(wan_x25_conf_t, t1), DTYPE_UINT },
  { "T2", 	    smemof(wan_x25_conf_t, t2), DTYPE_UINT },	
  { "T4", 	    smemof(wan_x25_conf_t, t4), DTYPE_UINT },
  { "N2", 	    smemof(wan_x25_conf_t, n2), DTYPE_UINT },
  { "T10_T20", 	    smemof(wan_x25_conf_t, t10_t20), DTYPE_UINT },
  { "T11_T21", 	    smemof(wan_x25_conf_t, t11_t21), DTYPE_UINT },	
  { "T12_T22", 	    smemof(wan_x25_conf_t, t12_t22), DTYPE_UINT },
  { "T13_T23", 	    smemof(wan_x25_conf_t, t13_t23), DTYPE_UINT },
  { "T16_T26", 	    smemof(wan_x25_conf_t, t16_t26), DTYPE_UINT },
  { "T28", 	    smemof(wan_x25_conf_t, t28), DTYPE_UINT },
  { "R10_R20", 	    smemof(wan_x25_conf_t, r10_r20), DTYPE_UINT },
  { "R12_R22", 	    smemof(wan_x25_conf_t, r12_r22), DTYPE_UINT },
  { "R13_R23", 	    smemof(wan_x25_conf_t, r13_r23), DTYPE_UINT },
  { "STATION_ADDR", smemof(wan_x25_conf_t, local_station_address), DTYPE_UCHAR},
  { "DEF_PKT_SIZE", smemof(wan_x25_conf_t, defPktSize),  DTYPE_UINT },
  { "MAX_PKT_SIZE", smemof(wan_x25_conf_t, pktMTU),  DTYPE_UINT },
  { NULL, 0, 0 }
};

key_word_t lapb_annexg_conftab[] =
{
  //LAPB STUFF
  { "T1", smemof(wan_lapb_if_conf_t, t1),    DTYPE_UINT },
  { "T2", smemof(wan_lapb_if_conf_t, t2),    DTYPE_UINT },
  { "T3", smemof(wan_lapb_if_conf_t, t3),    DTYPE_UINT },
  { "T4", smemof(wan_lapb_if_conf_t, t4),    DTYPE_UINT },
  { "N2", smemof(wan_lapb_if_conf_t, n2),    DTYPE_UINT },
  { "LAPB_MODE", 	smemof(wan_lapb_if_conf_t, mode),    DTYPE_UINT },
  { "LAPB_WINDOW", 	smemof(wan_lapb_if_conf_t, window),    DTYPE_UINT },

  { "LABEL",		smemof(wan_lapb_if_conf_t,label), DTYPE_STR},
  { "VIRTUAL_ADDR",     smemof(wan_lapb_if_conf_t,virtual_addr), DTYPE_STR},
  { "REAL_ADDR",        smemof(wan_lapb_if_conf_t,real_addr), DTYPE_STR},

  { "MAX_PKT_SIZE", 	smemof(wan_lapb_if_conf_t,mtu), DTYPE_UINT},

  { "STATION" ,          smemof(wan_lapb_if_conf_t, station),     DTYPE_UCHAR },
  
  { NULL, 0, 0 }
};


key_word_t x25_annexg_conftab[] =
{
 //X25 STUFF
  { "PACKETWINDOW", smemof(wan_x25_if_conf_t, packet_window),   DTYPE_USHORT },
  { "CCITTCOMPAT",  smemof(wan_x25_if_conf_t, CCITT_compatibility), DTYPE_USHORT },
  { "T10_T20", 	    smemof(wan_x25_if_conf_t, T10_T20), DTYPE_USHORT },
  { "T11_T21", 	    smemof(wan_x25_if_conf_t, T11_T21), DTYPE_USHORT },	
  { "T12_T22", 	    smemof(wan_x25_if_conf_t, T12_T22), DTYPE_USHORT },
  { "T13_T23", 	    smemof(wan_x25_if_conf_t, T13_T23), DTYPE_USHORT },
  { "T16_T26", 	    smemof(wan_x25_if_conf_t, T16_T26), DTYPE_USHORT },
  { "T28", 	    smemof(wan_x25_if_conf_t, T28),     DTYPE_USHORT },
  { "R10_R20", 	    smemof(wan_x25_if_conf_t, R10_R20), DTYPE_USHORT },
  { "R12_R22", 	    smemof(wan_x25_if_conf_t, R12_R22), DTYPE_USHORT },
  { "R13_R23", 	    smemof(wan_x25_if_conf_t, R13_R23), DTYPE_USHORT },

  { "X25_API_OPTIONS", smemof(wan_x25_if_conf_t, X25_API_options), DTYPE_USHORT },
  { "X25_PROTOCOL_OPTIONS", smemof(wan_x25_if_conf_t, X25_protocol_options), DTYPE_USHORT },
  { "X25_RESPONSE_OPTIONS", smemof(wan_x25_if_conf_t, X25_response_options), DTYPE_USHORT },
  { "X25_STATISTICS_OPTIONS", smemof(wan_x25_if_conf_t, X25_statistics_options), DTYPE_USHORT },

  { "GEN_FACILITY_1", smemof(wan_x25_if_conf_t, genl_facilities_supported_1), DTYPE_USHORT },
  { "GEN_FACILITY_2", smemof(wan_x25_if_conf_t, genl_facilities_supported_2), DTYPE_USHORT },
  { "CCITT_FACILITY", smemof(wan_x25_if_conf_t, CCITT_facilities_supported), DTYPE_USHORT },
  { "NON_X25_FACILITY",	smemof(wan_x25_if_conf_t,non_X25_facilities_supported),DTYPE_USHORT },

  { "DFLT_PKT_SIZE", smemof(wan_x25_if_conf_t,default_packet_size), DTYPE_USHORT },
  { "MAX_PKT_SIZE",  smemof(wan_x25_if_conf_t,maximum_packet_size), DTYPE_USHORT },

  { "LOWESTSVC",   smemof(wan_x25_if_conf_t,lowest_two_way_channel), DTYPE_USHORT },
  { "HIGHESTSVC",  smemof(wan_x25_if_conf_t,highest_two_way_channel), DTYPE_USHORT},

  { "LOWESTPVC",   smemof(wan_x25_if_conf_t,lowest_PVC), DTYPE_USHORT },
  { "HIGHESTPVC",  smemof(wan_x25_if_conf_t,highest_PVC), DTYPE_USHORT},

  { "X25_MODE", smemof(wan_x25_if_conf_t, mode), DTYPE_UCHAR},
  { "CALL_BACKOFF", smemof(wan_x25_if_conf_t, call_backoff_timeout), DTYPE_UINT },
  { "CALL_LOGGING", smemof(wan_x25_if_conf_t, call_logging), DTYPE_UCHAR },

  { "X25_CALL_STRING",      smemof(wan_x25_if_conf_t, call_string),     DTYPE_STR},
  { "X25_ACCEPT_DST_ADDR",  smemof(wan_x25_if_conf_t, accept_called),   DTYPE_STR},
  { "X25_ACCEPT_SRC_ADDR",  smemof(wan_x25_if_conf_t, accept_calling),  DTYPE_STR},
  { "X25_ACCEPT_FCL_DATA",  smemof(wan_x25_if_conf_t, accept_facil),    DTYPE_STR},
  { "X25_ACCEPT_USR_DATA",  smemof(wan_x25_if_conf_t, accept_udata),    DTYPE_STR},

  { "LABEL",		smemof(wan_x25_if_conf_t,label),        DTYPE_STR},
  { "VIRTUAL_ADDR",     smemof(wan_x25_if_conf_t,virtual_addr), DTYPE_STR},
  { "REAL_ADDR",        smemof(wan_x25_if_conf_t,real_addr),    DTYPE_STR},

  { NULL, 0, 0 }
};

key_word_t dsp_annexg_conftab[] =
{
  //DSP_20 DSP STUFF
  { "PAD",		smemof(wan_dsp_if_conf_t, pad_type),		DTYPE_UCHAR },
  { "T1_DSP",  		smemof(wan_dsp_if_conf_t, T1), 		DTYPE_UINT },
  { "T2_DSP",  		smemof(wan_dsp_if_conf_t, T2), 		DTYPE_UINT },
  { "T3_DSP",  		smemof(wan_dsp_if_conf_t, T3), 		DTYPE_UINT },
  { "DSP_AUTO_CE",  	smemof(wan_dsp_if_conf_t, auto_ce), 		DTYPE_UCHAR },
  { "DSP_AUTO_CALL_REQ",smemof(wan_dsp_if_conf_t, auto_call_req), 	DTYPE_UCHAR },
  { "DSP_AUTO_ACK",  	smemof(wan_dsp_if_conf_t, auto_ack), 		DTYPE_UCHAR },
  { "DSP_MTU",  	smemof(wan_dsp_if_conf_t, dsp_mtu), 		DTYPE_USHORT },
  { NULL, 0, 0 }
};

key_word_t chan_conftab[] =	/* Channel configuration parameters */
{
  { "IDLETIMEOUT",   	smemof(wanif_conf_t, idle_timeout), 	DTYPE_UINT },
  { "HOLDTIMEOUT",   	smemof(wanif_conf_t, hold_timeout), 	DTYPE_UINT },
  { "X25_SRC_ADDR",   	smemof(wanif_conf_t, x25_src_addr), 	DTYPE_STR},
  { "X25_ACCEPT_DST_ADDR",  smemof(wanif_conf_t, accept_dest_addr), DTYPE_STR},
  { "X25_ACCEPT_SRC_ADDR",  smemof(wanif_conf_t, accept_src_addr),  DTYPE_STR},
  { "X25_ACCEPT_USR_DATA",  smemof(wanif_conf_t, accept_usr_data),  DTYPE_STR},
  { "CIR",           	smemof(wanif_conf_t, cir), 	   	DTYPE_UINT },
  { "BC",            	smemof(wanif_conf_t, bc),		DTYPE_UINT },
  { "BE", 	     	smemof(wanif_conf_t, be),		DTYPE_UINT },
  { "MULTICAST",     	smemof(wanif_conf_t, mc),		DTYPE_UCHAR},
  { "IPX",	     	smemof(wanif_conf_t, enable_IPX),	DTYPE_UCHAR},
  { "NETWORK",       	smemof(wanif_conf_t, network_number),	DTYPE_UINT},
  
 // { "PAP",   	     	smemof(wanif_conf_t, pap),		DTYPE_UCHAR},
 // { "CHAP",          	smemof(wanif_conf_t, chap),		DTYPE_UCHAR},
 // { "USERID",        	smemof(wanif_conf_t, userid),	 	DTYPE_STR},
 // { "PASSWD",        	smemof(wanif_conf_t, passwd),		DTYPE_STR},
 // { "SYSNAME",       	smemof(wanif_conf_t, sysname),		DTYPE_STR},
  //PPP profile kept in "wanif_conf_t.u.ppp"
 // { "IP_MODE",       	offsetof(wanif_conf_t, u) + smemof(wan_sppp_if_conf_t, dynamic_ip),	DTYPE_UINT},
  
  { "INARP", 	     	smemof(wanif_conf_t, inarp),          	DTYPE_UCHAR},
  { "INARPINTERVAL", 	smemof(wanif_conf_t, inarp_interval), 	DTYPE_UINT },
  { "INARP_RX",      	smemof(wanif_conf_t, inarp_rx),          	DTYPE_UCHAR},
  { "IGNORE_DCD",  	smemof(wanif_conf_t, ignore_dcd),        	DTYPE_UCHAR},
  { "IGNORE_CTS",    	smemof(wanif_conf_t, ignore_cts),        	DTYPE_UCHAR},
  { "IGNORE_KEEPALIVE", smemof(wanif_conf_t, ignore_keepalive), 	DTYPE_UCHAR},
  { "HDLC_STREAMING", 	smemof(wanif_conf_t, hdlc_streaming), 	DTYPE_UCHAR},
  //{ "KEEPALIVE_TX_TIMER",	smemof(wanif_conf_t, keepalive_tx_tmr), 	DTYPE_UINT },
  //{ "KEEPALIVE_RX_TIMER",	smemof(wanif_conf_t, keepalive_rx_tmr), 	DTYPE_UINT },
  //{ "KEEPALIVE_ERR_MARGIN",	smemof(wanif_conf_t, keepalive_err_margin),	DTYPE_UINT },
  //{ "SLARP_TIMER", 	smemof(wanif_conf_t, slarp_timer),    DTYPE_UINT },
  { "TTL",        	smemof(wanif_conf_t, ttl),         DTYPE_UCHAR },

  { "STATION" ,          smemof(wanif_conf_t, station),     DTYPE_UCHAR },
  { "DYN_INTR_CFG",  	smemof(wanif_conf_t, if_down),     DTYPE_UCHAR },
  { "GATEWAY",  	smemof(wanif_conf_t, gateway),     DTYPE_UCHAR },
  { "TRUE_ENCODING_TYPE", smemof(wanif_conf_t,true_if_encoding), DTYPE_UCHAR },

  /* ASYNC Options */
  { "ASYNC_MODE",    	       smemof(wanif_conf_t, async_mode), DTYPE_UCHAR},	
  { "ASY_DATA_TRANSPARENT",    smemof(wanif_conf_t, asy_data_trans), DTYPE_UCHAR},
  { "RTS_HS_FOR_RECEIVE",      smemof(wanif_conf_t, rts_hs_for_receive), DTYPE_UCHAR},
  { "XON_XOFF_HS_FOR_RECEIVE", smemof(wanif_conf_t, xon_xoff_hs_for_receive), DTYPE_UCHAR},
  { "XON_XOFF_HS_FOR_TRANSMIT",smemof(wanif_conf_t, xon_xoff_hs_for_transmit), DTYPE_UCHAR},
  { "DCD_HS_FOR_TRANSMIT",     smemof(wanif_conf_t, dcd_hs_for_transmit), DTYPE_UCHAR},	
  { "CTS_HS_FOR_TRANSMIT",     smemof(wanif_conf_t, cts_hs_for_transmit), DTYPE_UCHAR},
  { "TX_BITS_PER_CHAR",        smemof(wanif_conf_t, tx_bits_per_char),    DTYPE_UINT },
  { "RX_BITS_PER_CHAR",        smemof(wanif_conf_t, rx_bits_per_char),    DTYPE_UINT },
  { "STOP_BITS",               smemof(wanif_conf_t, stop_bits),    DTYPE_UINT },
  { "PARITY",                  smemof(wanif_conf_t, parity),    DTYPE_UCHAR },
  { "BREAK_TIMER",             smemof(wanif_conf_t, break_timer),    DTYPE_UINT },	
  { "INTER_CHAR_TIMER",        smemof(wanif_conf_t, inter_char_timer),    DTYPE_UINT },
  { "RX_COMPLETE_LENGTH",      smemof(wanif_conf_t, rx_complete_length),    DTYPE_UINT },
  { "XON_CHAR",                smemof(wanif_conf_t, xon_char),    DTYPE_UINT },	
  { "XOFF_CHAR",               smemof(wanif_conf_t, xoff_char),    DTYPE_UINT },	
  //{ "MPPP_PROT",	       smemof(wanif_conf_t, protocol),  DTYPE_UCHAR},
  { "PROTOCOL",      	       smemof(wanif_conf_t, protocol),  DTYPE_UCHAR},	//note!! it is read, ignored and
  										//NOT written for everything but 
										//LIP ATM!
  
  { "ACTIVE_CH",	       	smemof(wanif_conf_t, active_ch),  	DTYPE_UINT},
  { "SW_DEV_NAME",	       	smemof(wanif_conf_t, sw_dev_name),  	DTYPE_STR},

  { "DLCI_TRACE_QUEUE",		smemof(wanif_conf_t, max_trace_queue), DTYPE_UINT},
  { "MAX_TRACE_QUEUE",		smemof(wanif_conf_t, max_trace_queue), DTYPE_UINT},

  { "TDMV_ECHO_OFF", offsetof(wanif_conf_t, tdmv)+smemof(wan_tdmv_if_conf_t, tdmv_echo_off), DTYPE_UCHAR},
  { "TDMV_CODEC",    offsetof(wanif_conf_t, tdmv)+smemof(wan_tdmv_if_conf_t, tdmv_codec),    DTYPE_UCHAR},
  { "TDMV_HWEC",     offsetof(wanif_conf_t, hwec)+smemof(wan_hwec_if_conf_t, enable),    DTYPE_UCHAR}, 

  { NULL, 0, 0 }
};

look_up_t conf_def_tables[] =
{
	{ WANCONFIG_FR,		fr_conftab	},
	{ WANCONFIG_X25,	x25_conftab	},
	{ WANCONFIG_ADCCP,	x25_conftab	},
	{ WANCONFIG_MPCHDLC,	sppp_conftab},//chdlc_conftab	},
	{ WANCONFIG_MPPP,	sppp_conftab},//chdlc_conftab	},
	{ WANCONFIG_MFR,	fr_conftab	},
	{ WANCONFIG_SS7,	ss7_conftab 	},
	{ WANCONFIG_ADSL,	adsl_conftab 	},
	{ WANCONFIG_BSCSTRM,	bscstrm_conftab	},
	//{ WANCONFIG_ATM,	atm_conftab	},
	//{ WANCONFIG_LIP_ATM,	atm_conftab	},//not used
	//{ WANCONFIG_MLINK_PPP,	chdlc_conftab	},
	{ WANCONFIG_AFT,        xilinx_conftab  },
	{ WANCONFIG_EDUKIT,     edukit_conftab  },
	{ WANCONFIG_AFT_TE3,    xilinx_conftab  },
	{ WANCONFIG_BITSTRM,    bitstrm_conftab },
	{ WANCONFIG_LAPB,       lapb_annexg_conftab},
	{ 0,			NULL		}
};

look_up_t conf_if_def_tables[] =
{
	{ WANCONFIG_ATM,	atm_if_conftab	},
	{ WANCONFIG_LIP_ATM,	atm_if_conftab	},
	{ WANCONFIG_BITSTRM,    bitstrm_if_conftab },
	{ WANCONFIG_AFT,	xilinx_if_conftab },
	{ WANCONFIG_AFT_TE3,    xilinx_if_conftab },
	{ WANCONFIG_AFT_GSM,    xilinx_if_conftab },
	{ 0,			NULL		}
};

look_up_t conf_annexg_def_tables[] =
{
	{ ANNEXG_LAPB,		lapb_annexg_conftab	},
	{ ANNEXG_X25,		x25_annexg_conftab	},
	{ ANNEXG_DSP,		dsp_annexg_conftab	},
	{ 0,			NULL		}
};

look_up_t	config_id_str[] =
{
	{ WANCONFIG_PPP,	(void*)"WAN_PPP"	},
	{ WANCONFIG_FR,		(void*)"WAN_FR"		},
	{ WANCONFIG_LIP_ATM,    (void*)"WAN_LIP_ATM"   	},
	{ WANCONFIG_X25,	(void*)"WAN_X25"	},
	{ WANCONFIG_CHDLC,	(void*)"WAN_CHDLC"	},
  	{ WANCONFIG_BSC,        (void*)"WAN_BSC"       	},
  	{ WANCONFIG_HDLC,       (void*)"WAN_HDLC"      	},
	{ WANCONFIG_MPROT,      (void*)"WAN_MULTPROT"  	},
	{ WANCONFIG_MPPP,       (void*)"WAN_MULTPPP"   	},
	{ WANCONFIG_BITSTRM, 	(void*)"WAN_BITSTRM"	},
	{ WANCONFIG_EDUKIT, 	(void*)"WAN_EDU_KIT"	},
	{ WANCONFIG_SS7,        (void*)"WAN_SS7"       	},
	{ WANCONFIG_BSCSTRM,    (void*)"WAN_BSCSTRM"   	},
	{ WANCONFIG_ADSL,	(void*)"WAN_ADSL"	},
	{ WANCONFIG_ADSL,	(void*)"WAN_ETH"	},
	{ WANCONFIG_SDLC,	(void*)"WAN_SDLC"	},
	{ WANCONFIG_ATM,	(void*)"WAN_ATM"	},
	{ WANCONFIG_POS,	(void*)"WAN_POS"	},
	{ WANCONFIG_AFT,	(void*)"WAN_AFT"	},
	{ WANCONFIG_AFT_TE1,	(void*)"WAN_AFT_TE1"	},
	{ WANCONFIG_AFT_56K,	(void*)"WAN_AFT_56K"	},
	{ WANCONFIG_AFT_TE3,	(void*)"WAN_AFT_TE3"	},
	{ WANCONFIG_AFT,	(void*)"WAN_XILINX"	},
	{ WANCONFIG_MFR,    	(void*)"WAN_MFR"   	},
	{ WANCONFIG_LAPB,	(void*)"WAN_LAPB"	},
	{ WANCONFIG_DEBUG,    	(void*)"WAN_DEBUG"   	},
	{ WANCONFIG_ADCCP,    	(void*)"WAN_ADCCP"   	},
	{ WANCONFIG_MLINK_PPP, 	(void*)"WAN_MLINK_PPP"  },
	{ WANCONFIG_TTY,	(void*)"WAN_TTY"	}, //????
	{ WANCONFIG_AFT_ANALOG,	(void*)"WAN_AFT_ANALOG" },
	{ WANCONFIG_AFT_GSM,	(void*)"WAN_AFT_GSM"    },
	{ WANCONFIG_AFT_ISDN_BRI,	(void*)"WAN_AFT_ISDN_BRI" },
	{ WANCONFIG_AFT_SERIAL, (void*)"WAN_AFT_SERIAL" },
	{ 0,			NULL,			}
};


/*
 * Configuration options values and their symbolic names.
 */
look_up_t	sym_table[] =
{
	/*----- General -----------------------*/
	{ WANOPT_OFF,		(void*)"OFF"		},
	{ WANOPT_ON,		(void*)"ON"		},
	{ WANOPT_NO,		(void*)"NO"		},
	{ WANOPT_YES,		(void*)"YES"		},
	/*----- Interface type ----------------*/
	{ WANOPT_RS232,	(void*)"RS232"		},
	{ WANOPT_V35,		(void*)"V35"		},
	/*----- Data encoding -----------------*/
	{ WANOPT_NRZ,		(void*)"NRZ"		},
	{ WANOPT_NRZI,	(void*)"NRZI"		},
	{ WANOPT_FM0,		(void*)"FM0"		},
	{ WANOPT_FM1,		(void*)"FM1"		},

	/*----- Idle Line ----------------------*/
	{ WANOPT_IDLE_FLAG,	(void*)"FLAG"	},
	{ WANOPT_IDLE_MARK,	(void*)"MARK"	},

	/*----- Link type ---------------------*/
	{ WANOPT_POINTTOPOINT,	(void*)"POINTTOPOINT"	},
	{ WANOPT_MULTIDROP,	(void*)"MULTIDROP"	},
	/*----- Clocking ----------------------*/
	{ WANOPT_EXTERNAL,	(void*)"EXTERNAL"	},
	{ WANOPT_INTERNAL,	(void*)"INTERNAL"	},
	/*----- Station -----------------------*/
	{ WANOPT_DTE,		(void*)"DTE"		},
	{ WANOPT_DCE,		(void*)"DCE"		},
	{ WANOPT_CPE,		(void*)"CPE"		},
	{ WANOPT_NODE,		(void*)"NODE"		},
	{ WANOPT_SECONDARY,	(void*)"SECONDARY"	},
	{ WANOPT_PRIMARY,	(void*)"PRIMARY"	},
	/*----- Connection options ------------*/
	{ WANOPT_PERMANENT,	(void*)"PERMANENT"	},
	{ WANOPT_SWITCHED,	(void*)"SWITCHED"	},
	{ WANOPT_ONDEMAND,	(void*)"ONDEMAND"	},
	/*----- Frame relay in-channel signalling */
	{ WANOPT_FR_ANSI,	(void*)"ANSI"		},
	{ WANOPT_FR_Q933,	(void*)"Q933"		},
	{ WANOPT_FR_LMI,	(void*)"LMI"		},
	/*----- PPP IP Mode Options -----------*/
	{ WANOPT_PPP_STATIC,	(void*)"STATIC"	},
	{ WANOPT_PPP_HOST,	(void*)"HOST"		},
	{ WANOPT_PPP_PEER,	(void*)"PEER"		},
	/*----- CHDLC Protocol Options --------*/
/* DF for now	{ WANOPT_CHDLC_NO_DCD,	(void*)"IGNORE_DCD"	},
	{ WANOPT_CHDLC_NO_CTS,	(void*)"IGNORE_CTS"	},
	{ WANOPT_CHDLC_NO_KEEP,	(void*)"IGNORE_KEEPALIVE"},
*/
	{ WANOPT_PRI,           (void*)"PRI"           },
	{ WANOPT_SEC,           (void*)"SEC"           },
	
	/*------Read Mode ---------------------*/
	{ WANOPT_INTR,          (void*)"INT"           },
	{ WANOPT_POLL,          (void*)"POLL"          },
	/*------- Async Options ---------------*/
	{ WANOPT_ONE,           (void*)"ONE"          	},
	{ WANOPT_TWO,           (void*)"TWO"          	},
	{ WANOPT_ONE_AND_HALF,  (void*)"ONE_AND_HALF" 	},
	{ WANOPT_NONE,          (void*)"NONE"    	},
	{ WANOPT_EVEN,          (void*)"EVEN"    	},
	{ WANOPT_ODD,           (void*)"ODD"    	},

	{ WANOPT_TTY_SYNC,	(void*)"SYNC"		},
	{ WANOPT_TTY_ASYNC,     (void*)"ASYNC"		},


	/* TE1 T1/E1 defines */
  /*------TE Cofngiuration --------------*/

  { WAN_MEDIA_56K,		(void*)"56K"           },
  { WAN_MEDIA_T1,		(void*)"T1"            },
  { WAN_MEDIA_E1,		(void*)"E1"            },
  { WAN_MEDIA_DS3,		(void*)"DS3"           },
  { WAN_MEDIA_STS1,     (void*)"STS-1"         },
  { WAN_MEDIA_E3,       (void*)"E3"            },
  { WAN_MEDIA_FXOFXS,   (void*)"FXO/FXS"       },
  { WAN_MEDIA_GSM,   (void*)"GSM"           },
  { WAN_MEDIA_BRI,		(void*)"BRI"           },

  { WAN_LCODE_AMI, 	(void*)"AMI"           },
  { WAN_LCODE_B8ZS,     (void*)"B8ZS"          },
  { WAN_LCODE_HDB3,     (void*)"HDB3"          },
  { WAN_LCODE_B3ZS,     (void*)"B3ZS"          },
  
  { WAN_FR_D4,         (void*)"D4"          },
  { WAN_FR_ESF,        (void*)"ESF"         },
  { WAN_FR_NCRC4,      (void*)"NCRC4"       },
  { WAN_FR_CRC4,       (void*)"CRC4"        },
  { WAN_FR_UNFRAMED,   (void*)"UNFRAMED"    },
  { WAN_FR_E3_G751,       (void*)"G.751"       },
  { WAN_FR_E3_G832,       (void*)"G.832"       },
  { WAN_FR_DS3_Cbit,      (void*)"C-BIT"       },
  { WAN_FR_DS3_M13,       (void*)"M13"         },
  
  { WAN_T1_LBO_0_DB,  	(void*)"0DB"     },
  { WAN_T1_LBO_75_DB,  (void*)"7.5DB"         },
  { WAN_T1_LBO_15_DB,  (void*)"15DB"          },
  { WAN_T1_LBO_225_DB, (void*)"22.5DB"        },
  { WAN_T1_0_110,  	  (void*)"0-110FT"       },
  { WAN_T1_110_220,  	(void*)"110-220FT"     },
  { WAN_T1_220_330,  	(void*)"220-330FT"     },
  { WAN_T1_330_440,  	(void*)"330-440FT"     },
  { WAN_T1_440_550,  	(void*)"440-550FT"     },
  { WAN_T1_550_660,  	(void*)"550-660FT"     },
  { WAN_E1_120,  	(void*)"120OH"     },
  { WAN_E1_75,  	(void*)"75OH"     },
  { WAN_NORMAL_CLK,   	(void*)"NORMAL"        },
  { WAN_MASTER_CLK,   	(void*)"MASTER"        },
  { WAN_TE1_SIG_CAS,	(void*)"CAS"           },
  { WAN_TE1_SIG_CCS,	(void*)"CCS"		},


  	/* T3/E3 configuration */
        { WAN_TE3_RDEVICE_ADTRAN,	(void*)"ADTRAN"        },
        { WAN_TE3_RDEVICE_DIGITALLINK,	(void*)"DIGITAL-LINK"  },
        { WAN_TE3_RDEVICE_KENTROX,	(void*)"KENTROX"       },
        { WAN_TE3_RDEVICE_LARSCOM,	(void*)"LARSCOM"       },
        { WAN_TE3_RDEVICE_VERILINK,	(void*)"VERILINK"      },
        { WAN_TE3_LIU_LB_NORMAL,	(void*)"LB_NORMAL"     },
        { WAN_TE3_LIU_LB_ANALOG,	(void*)"LB_ANALOG"     },
        { WAN_TE3_LIU_LB_REMOTE,	(void*)"LB_REMOTE"     },
        { WAN_TE3_LIU_LB_DIGITAL,	(void*)"LB_DIGITAL"    },

/*
	{ WANCONFIG_PPP,	  (void*)"MP_PPP"	},
	{ WANCONFIG_CHDLC,	(void*)"MP_CHDLC"	},
	{ WANCONFIG_FR, 	  (void*)"MP_FR"		},
	{ WANOPT_NO,		    (void*)"RAW"		},
	{ WANOPT_NO,		    (void*)"HDLC"		},
	{ WANCONFIG_PPP,	  (void*)"PPP"		},
	{ WANCONFIG_CHDLC,	(void*)"CHDLC"		},
*/
  //David
	{ WANCONFIG_TTY,	(void*)"TTY"		},
	{ WANCONFIG_MPPP,	(void*)"MP_PPP"		},
	{ WANCONFIG_MPCHDLC,	(void*)"MP_CHDLC"	},
	{ WANCONFIG_MFR, 	(void*)"MP_FR"		},
	{ WANCONFIG_LAPB,	(void*)"MP_LAPB"	},
	{ WANOPT_NO,		(void*)"RAW"		},
	{ WANCONFIG_HDLC,	(void*)"HDLC"		},
	{ WANCONFIG_PPP,	(void*)"PPP"		},
	{ WANCONFIG_CHDLC,	(void*)"CHDLC"		},
	{ WANCONFIG_EDUKIT,	(void*)"EDUKIT"		},
	{ WANCONFIG_LIP_ATM, 	(void*)"MP_ATM"		},

	/*-------------SS7 options--------------*/
	{ WANOPT_SS7_ANSI,      (void*)"ANSI" },
	{ WANOPT_SS7_ITU,       (void*)"ITU"  },
	{ WANOPT_SS7_NTT,       (void*)"NTT"  },
	
	{ WANOPT_S50X,		(void*)"S50X"		},
	{ WANOPT_S51X,		(void*)"S51X"		},
	{ WANOPT_ADSL,		(void*)"ADSL"		},
	{ WANOPT_ADSL,		(void*)"S518"		},
	{ WANOPT_AFT,     (void*)"AFT"    },

	/*-------------ADSL options--------------*/
	{ RFC_MODE_BRIDGED_ETH_LLC,	(void*)"ETH_LLC_OA" },
	{ RFC_MODE_BRIDGED_ETH_VC,	(void*)"ETH_VC_OA"  },
	{ RFC_MODE_ROUTED_IP_LLC,	  (void*)"IP_LLC_OA"  },
	{ RFC_MODE_ROUTED_IP_VC,	  (void*)"IP_VC_OA"   },
	{ RFC_MODE_PPP_LLC,		      (void*)"PPP_LLC_OA" },
	{ RFC_MODE_PPP_VC,		      (void*)"PPP_VC_OA"  },
	
	/* Backward compatible */
	{ RFC_MODE_BRIDGED_ETH_LLC,	(void*)"BLLC_OA" },
	{ RFC_MODE_ROUTED_IP_LLC,	  (void*)"CIP_OA" },
	{ LAN_INTERFACE,		        (void*)"LAN" },
	{ WAN_INTERFACE,		        (void*)"WAN" },

	{ WANOPT_ADSL_T1_413,                   (void*)"ADSL_T1_413"                   },
	{ WANOPT_ADSL_G_LITE,                   (void*)"ADSL_G_LITE"                   },
	{ WANOPT_ADSL_G_DMT,                    (void*)"ADSL_G_DMT"                    },
	{ WANOPT_ADSL_ALCATEL_1_4,              (void*)"ADSL_ALCATEL_1_4"              },
	{ WANOPT_ADSL_ALCATEL,                  (void*)"ADSL_ALCATEL"                  },
	{ WANOPT_ADSL_MULTIMODE,                (void*)"ADSL_MULTIMODE"                },
	{ WANOPT_ADSL_T1_413_AUTO,              (void*)"ADSL_T1_413_AUTO"              },
	
	{ WANOPT_ADSL_TRELLIS_ENABLE,           (void*)"ADSL_TRELLIS_ENABLE"           },
	{ WANOPT_ADSL_TRELLIS_DISABLE,          (void*)"ADSL_TRELLIS_DISABLE"          },
	{ WANOPT_ADSL_TRELLIS_LITE_ONLY_DISABLE,(void*)"ADSL_TRELLIS_LITE_ONLY_DISABLE"},
	
	{ WANOPT_ADSL_0DB_CODING_GAIN,          (void*)"ADSL_0DB_CODING_GAIN"          },
	{ WANOPT_ADSL_1DB_CODING_GAIN,          (void*)"ADSL_1DB_CODING_GAIN"          },
	{ WANOPT_ADSL_2DB_CODING_GAIN,          (void*)"ADSL_2DB_CODING_GAIN"          },
	{ WANOPT_ADSL_3DB_CODING_GAIN,          (void*)"ADSL_3DB_CODING_GAIN"          },
	{ WANOPT_ADSL_4DB_CODING_GAIN,          (void*)"ADSL_4DB_CODING_GAIN"          },
	{ WANOPT_ADSL_5DB_CODING_GAIN,          (void*)"ADSL_5DB_CODING_GAIN"          },
	{ WANOPT_ADSL_6DB_CODING_GAIN,          (void*)"ADSL_6DB_CODING_GAIN"          },
	{ WANOPT_ADSL_7DB_CODING_GAIN,          (void*)"ADSL_7DB_CODING_GAIN"          },
	{ WANOPT_ADSL_AUTO_CODING_GAIN,         (void*)"ADSL_AUTO_CODING_GAIN"         },
	
	{ WANOPT_ADSL_RX_BIN_DISABLE,           (void*)"ADSL_RX_BIN_DISABLE"           },
	{ WANOPT_ADSL_RX_BIN_ENABLE,            (void*)"ADSL_RX_BIN_ENABLE"            },
	
	{ WANOPT_ADSL_FRAMING_TYPE_0,           (void*)"ADSL_FRAMING_TYPE_0"           },
	{ WANOPT_ADSL_FRAMING_TYPE_1,           (void*)"ADSL_FRAMING_TYPE_1"           },
	{ WANOPT_ADSL_FRAMING_TYPE_2,           (void*)"ADSL_FRAMING_TYPE_2"           },
	{ WANOPT_ADSL_FRAMING_TYPE_3,           (void*)"ADSL_FRAMING_TYPE_3"           },
	
	{ WANOPT_ADSL_EXPANDED_EXCHANGE,        (void*)"ADSL_EXPANDED_EXCHANGE"        },
	{ WANOPT_ADSL_SHORT_EXCHANGE,           (void*)"ADSL_SHORT_EXCHANGE"           },

	{ WANOPT_ADSL_CLOCK_CRYSTAL,            (void*)"ADSL_CLOCK_CRYSTAL"            },
	{ WANOPT_ADSL_CLOCK_OSCILLATOR,         (void*)"ADSL_CLOCK_OSCILLATOR"         },

	{ IBM4680,	(void*)"IBM4680" },
	{ IBM4680,	(void*)"IBM4680" },
  	{ NCR2126,	(void*)"NCR2126" },
	{ NCR2127,	(void*)"NCR2127" },
	{ NCR1255,	(void*)"NCR1255" },
	{ NCR7000,	(void*)"NCR7000" },
	{ ICL,		(void*)"ICL" },

	// DSP_20
	/*------- DSP Options -----------------*/
	{ WANOPT_DSP_HPAD,      (void*)"HPAD"         	},
	{ WANOPT_DSP_TPAD,      (void*)"TPAD"         	},

	{ WANOPT_AUTO,  (void*)"AUTO" },
	{ WANOPT_MANUAL,(void*)"MANUAL" },

	{ WAN_TDMV_ALAW, (void*)"ALAW" },
	{ WAN_TDMV_MULAW,(void*)"MULAW" },

	/*----- End ---------------------------*/
	{ 0,			NULL		},
};

//////////////////////////////////////////////////////////////////////////////////////////

/****** Global Function Prototypes *************************************************/

int tokenize (char* str, char **tokens);
char* str_strip (char* str, char* s);
char* strupcase	(char* str);
void* lookup (int val, look_up_t* table);
int name2val (char* name, look_up_t* table);

unsigned int dec_to_uint (unsigned char* str, int len);

void read_adsl_vci_vpi_list(wan_adsl_vcivpi_t* vcivpi_list, unsigned short* vcivpi_num);

extern	int close (int);

//////////////////////////////////////////////////////////////////////////////////////////

conf_file_reader::conf_file_reader(int wanpipe_number)
{
  Debug(DBG_CONF_FILE_READER, ("conf_file_reader::conf_file_reader(%d)\n",
				  wanpipe_number));

  //wanpipe number will be needed in many cases
  this->wanpipe_number = wanpipe_number;

  snprintf(conf_file_full_path, MAX_PATH_LENGTH, "%swanpipe%d.conf", 
				  wanpipe_cfg_dir, wanpipe_number);

  /////////////////////////////////////////////////////////////
  verbose = 1;	    /* verbosity level */
  link_defs = NULL;
  conf_file_ptr = NULL;
}

int conf_file_reader::init()
{
  wandev_conf_t* linkconf;
  sdla_fe_cfg_t*  fe_cfg;
  sdla_te_cfg_t*  te_cfg;
  wan_xilinx_conf_t* wan_xilinx_conf;

  Debug(DBG_CONF_FILE_READER, ("conf_file_reader::init(%d)\n", wanpipe_number));

  main_obj_list = new objects_list();
  if(main_obj_list == NULL){
    ERR_DBG_OUT(("Memory allocation failed for 'main_obj_list'!!\n"));
    return 1;
  }

  /////////////////////////////////////////////////////////////
  link_defs = (link_def_t*)malloc(sizeof(link_def_t));
  if (link_defs == NULL) {
    ERR_DBG_OUT(("Memory allocation failed for 'link_defs'!!\n"));
    return 1;
  }
  memset(link_defs, 0, sizeof(link_def_t));

  snprintf(link_defs->name, WAN_DRVNAME_SZ, "wanpipe%d", wanpipe_number);
  
  /////////////////////////////////////////////////////////////
  link_defs->linkconf = (wandev_conf_t*)malloc(sizeof(wandev_conf_t));
  if (link_defs->linkconf == NULL){
    ERR_DBG_OUT(("Memory allocation failed for 'link_defs->linkconf'!!\n"));
  }
  memset(link_defs->linkconf, 0, sizeof(wandev_conf_t));
  
  /////////////////////////////////////////////////////////////
  linkconf = link_defs->linkconf;
  
  linkconf->card_type = NOT_SET;
  linkconf->config_id = PROTOCOL_NOT_SET;
  linkconf->magic = ROUTER_MAGIC;

  /////////////////////////////////////////////////////////////
  linkconf->mtu = 1500;
  linkconf->udp_port = 9000;
  linkconf->ttl = 255;
  linkconf->ignore_front_end_status = 0;

  wan_xilinx_conf = &linkconf->u.aft;
  //wan_xilinx_conf->tdmv_span_no = 0;
  linkconf->tdmv_conf.dchan = 0;//by default NOT used or DISABLED
				  //OR
				  //the actual value will be read from conf file
  /*
  linkconf->tdmv_conf.dchan = 24;//when media type will be known,
  				//must be set to a default value: 24-T1, 16-E1
				//OR
				//the actual value will be read from conf file
  */
  //at this point hardware type is not known, so initialize the firmware to most common.
  //good for all except EduKit.
  snprintf(link_defs->firmware_file_path, MAX_PATH_LENGTH, "%sfirmware/%s", 
		 wanpipe_cfg_dir, "cdual514.sfm");
  
  link_defs->start_script = NO;
  link_defs->stop_script = NO;
  /////////////////////////////////////////////////////////////
 
  //PCI config
  linkconf->S514_CPU_no[0] = 'A';
  linkconf->comm_port = WANOPT_PRI;
  linkconf->auto_hw_detect = WANOPT_YES;
  //serial config
  linkconf->electrical_interface = WANOPT_V35;

  //front end config
  fe_cfg = &linkconf->fe_cfg;
  te_cfg = &fe_cfg->cfg.te_cfg;
  set_default_t1_configuration(fe_cfg);
 
  //this is only for S514, for AFT it *must* always be "ALL" - no option to change it
  snprintf(link_defs->active_channels_string, MAX_LEN_OF_ACTIVE_CHANNELS_STRING, "ALL");
  return 0;
}

conf_file_reader::~conf_file_reader()
{
  Debug(DBG_CONF_FILE_READER, ("conf_file_reader::~conf_file_reader()\n"));
  
  if(main_obj_list != NULL){
    delete main_obj_list;
  }
  
  if(link_defs != NULL){
    if(link_defs->linkconf != NULL){
      free(link_defs->linkconf);
    }
    free(link_defs);
  }

  if(conf_file_ptr != NULL){
    fclose(conf_file_ptr);
  }
}

int conf_file_reader::read_conf_file()
{
  int err = 0;

  Debug(DBG_CONF_FILE_READER, ("conf_file_reader::read_conf_file()\n"));

  conf_file_ptr = fopen(conf_file_full_path, "r");
  if (conf_file_ptr == NULL) {
    ERR_DBG_OUT(("Failed to open file: %s!!\n", conf_file_full_path));
  }

  err = read_devices_section(conf_file_ptr);
  if (err){
    return err;
  }

  err = read_wanpipe_section(conf_file_ptr);
  if (err){
    return err;
  }

  /////////////////////////////////////////////////////////////////////////////////////////////
  sdla_fe_cfg_t*  fe_cfg = &link_defs->linkconf->fe_cfg;
  //sdla_te_cfg_t*  te_cfg = &fe_cfg->cfg.te_cfg;
#if DBG_FLAG
  sdla_te3_cfg_t* te3_cfg= &fe_cfg->cfg.te3_cfg; 
#endif

  switch(link_defs->card_version)
	{
  case AFT_ADPTR_2SERIAL_V35X21:	/* AFT (shark) Serial card */
		fe_cfg->media = WAN_MEDIA_SERIAL;
		break;
	}

  ///////////////////////////////////////////////////////////////////////////////////////////// 
  Debug(DBG_CONF_FILE_READER, ("%s(): fe_cfg->media: 0x%x\n",__FUNCTION__,
		fe_cfg->media));

  Debug(DBG_CONF_FILE_READER, ("%s(): fe_cfg->line_no: %d\n",__FUNCTION__,
		fe_cfg->line_no));

  Debug(DBG_CONF_FILE_READER, ("%s(): te3_cfg->fcs: %d\n",__FUNCTION__,
		te3_cfg->fcs));

  Debug(DBG_CONF_FILE_READER, ("%s(): te3_cfg->liu_cfg.rx_equal: %d\n",__FUNCTION__,
		te3_cfg->liu_cfg.rx_equal));

  Debug(DBG_CONF_FILE_READER, ("%s(): te3_cfg->liu_cfg.taos: %d\n",__FUNCTION__,
 		te3_cfg->liu_cfg.taos));

  /////////////////////////////////////////////////////////////////////////////////////////////
  //if S514X card, get the version (S514-1/2/3...7)
  if(link_defs->linkconf->card_type == WANOPT_S51X){

    if(get_pci_adapter_type_from_wanpipe_conf_file( conf_file_full_path,
                                                    &link_defs->card_version) == NO){
      return ERR_CONFIG;
    }else{
      Debug(DBG_CONF_FILE_READER,
        ("conf_file_reader: S514 card_version 0x%04X\n", link_defs->card_version));
    }
  }
 
  /////////////////////////////////////////////////////////////////////////////////////////////
  if(link_defs->linkconf->card_type == WANOPT_ADSL){

    wandev_conf_t *linkconf = link_defs->linkconf;
    wan_adsl_conf_t* adsl_cfg = &linkconf->u.adsl;
	
    //change the 'sub_config_id' accordingly
    switch(adsl_cfg->EncapMode)
    {
    case RFC_MODE_BRIDGED_ETH_LLC:
    case RFC_MODE_BRIDGED_ETH_VC:
      link_defs->sub_config_id = PROTOCOL_ETHERNET;
      break;
    
    case RFC_MODE_ROUTED_IP_LLC:
    case RFC_MODE_ROUTED_IP_VC:
    case RFC_MODE_RFC1577_ENCAP:
      link_defs->sub_config_id = PROTOCOL_IP;
      break;
    
    case RFC_MODE_PPP_LLC:
    case RFC_MODE_PPP_VC:
      link_defs->sub_config_id = WANCONFIG_MPPP;
      break;
    }//switch(adsl_cfg->EncapMode)
  }//if()

  /////////////////////////////////////////////////////////////////////////////////////////////
  int current_line_number = 0;
  err = read_interfaces_section(
		  conf_file_ptr,      //FILE* file,
		  main_obj_list,      //objects_list* parent_objects_list,
		  NULL,		      //char* parent_dev_name,
		  &current_line_number//int* current_line_number
		  );

  if (err){
    return err;
  }

 
  return err;
}

/*============================================================================
 * 
 */
int conf_file_reader::read_devices_section(FILE* file)
{
  int err = 0;
  char* conf;		  /* -> section buffer */
  char* key;		  /* -> current configuration record */
  int len;		  /* record length */
  char *token[MAX_TOKENS];

  int toknum;
  int config_id = 0;

  Debug(DBG_CONF_FILE_READER, ("conf_file_reader::read_devices_section()\n"));

  /* Read [devices] section */
  conf = read_conf_section(file, "devices");
  if (conf == NULL){
    return ERR_CONFIG;
  }

  //Debug(DBG_CONF_FILE_READER, ("conf:%s\n", conf));

  /* For the record in [devices] section create a link definition
   * structure and link it to the list of link definitions.
   */
  //Current wanpipe#.conf may have only *one* device in the [devices] section.
  key = conf;
  len = strlen(key) + 1;

  toknum = tokenize(key, token);
  if(toknum < 2){
    ERR_DBG_OUT(("Invalid number of tokens (%d) in [devices] section!!\n", toknum));
    err = ERR_CONFIG;
    goto done;
  }

  Debug(DBG_CONF_FILE_READER, ("token[1]:%s\n", token[1]));

  strupcase(token[1]);
  config_id = name2val(token[1], config_id_str);
  if (!config_id) {
    ERR_DBG_OUT((" * Device config_id %s is invalid!\n", token[1]));
    err = ERR_CONFIG;
    goto done;
  }
  //store the real config_id
  link_defs->linkconf->config_id = config_id;

  Debug(DBG_CONF_FILE_READER, ("Original link_defs->linkconf->config_id: %d\n",
			 link_defs->linkconf->config_id));
	
  switch(link_defs->linkconf->config_id)
  {
  case WANCONFIG_AFT_TE3:
    //aways keep it as just AFT to allow to LIP configuration
    link_defs->linkconf->config_id = WANCONFIG_AFT;
    link_defs->card_version = A300_ADPTR_U_1TE3;
    break;
		
  case WANCONFIG_AFT:	/* AFT Original Hardware Support */
    link_defs->card_version = A101_ADPTR_1TE1;
    break;

  case WANCONFIG_AFT_TE1:	/* AFT Quad Hardware Support */
    link_defs->linkconf->config_id = WANCONFIG_AFT;
    link_defs->card_version = A104_ADPTR_4TE1;
    break;

  case WANCONFIG_AFT_ANALOG:	/* AFT (shark) analog card */
    link_defs->linkconf->config_id = WANCONFIG_AFT;
    link_defs->card_version = A200_ADPTR_ANALOG;
    break;

  case WANCONFIG_AFT_56K:	/* AFT 56k DDS */
    link_defs->linkconf->config_id = WANCONFIG_AFT;//WANCONFIG_AFT_56K;
    link_defs->card_version = AFT_ADPTR_56K;
    break;

  case WANCONFIG_AFT_ISDN_BRI:	/* AFT (shark) ISDN BRI card */
    link_defs->linkconf->config_id = WANCONFIG_AFT;
    link_defs->card_version = AFT_ADPTR_ISDN;
    break;

  case WANCONFIG_AFT_SERIAL:	/* AFT (shark) Serial card */
    link_defs->linkconf->config_id = WANCONFIG_AFT;
    link_defs->card_version = AFT_ADPTR_2SERIAL_V35X21;//this will indicate "Serial" card, number of ports is not important.
    break;
  }


  Debug(DBG_CONF_FILE_READER, ("Updated link_defs->linkconf->config_id: %d\n",
			 link_defs->linkconf->config_id)); 
 
  Debug(DBG_CONF_FILE_READER, ("token[0]:%s\n", token[0]));
  //store the section name i.g. [wanpipe2]
  strncpy(link_defs->name, token[0], WAN_DRVNAME_SZ);
	
  //check the start/stop scripts exist for the wanpipe#
  link_defs->start_script = check_wanpipe_start_script_exist(link_defs->name);
  link_defs->stop_script = check_wanpipe_stop_script_exist(link_defs->name);

  Debug(DBG_CONF_FILE_READER, ("link_defs->start_script:%d\n", link_defs->start_script));
  Debug(DBG_CONF_FILE_READER, ("link_defs->stop_script:%d\n", link_defs->stop_script));
	
  //this is for the 'Comment'
  if ((toknum > 2) && token[2]){
    link_defs->descr = strdup(token[2]);
    Debug(DBG_CONF_FILE_READER, ("Comment for the wanpipe:%s\n", link_defs->descr));
  }
  
done:

  Debug(DBG_CONF_FILE_READER, ("read_devices_section() - End (err:%d)\n", err));

  free(conf);
  return err;
}


/*============================================================================
 * 
 */
int conf_file_reader::read_wanpipe_section(FILE* file)
{
  int err = 0;
  int len = 0;
  key_word_t* conf_table = (key_word_t*)lookup(link_defs->linkconf->config_id, conf_def_tables);
  char* conf_rec;
  char *token[MAX_TOKENS];

  Debug(DBG_CONF_FILE_READER, ("conf_file_reader::read_wanpipe_section()\n"));

  link_defs->conf = read_conf_section(file, link_defs->name);
  if(link_defs->conf == NULL){
    ERR_DBG_OUT(("Failed to read '%s' section!!\n", link_defs->name));
    err = ERR_CONFIG;
  }
	
  /* Parse link configuration */
  for (conf_rec = link_defs->conf; *conf_rec; conf_rec += len) {

    int toknum;

    len = strlen(conf_rec) + 1;
    toknum = tokenize(conf_rec, token);

    Debug(DBG_CONF_FILE_READER, ("toknum: %d\n", toknum));
   
    if(toknum < 2) {
      ERR_DBG_OUT(("Invalid number of tokens (%d) in 'wanpipe' section!!\n", toknum));
      return ERR_CONFIG;
    }
    Debug(DBG_CONF_FILE_READER, ("token[0]: %s\n", token[0]));
    Debug(DBG_CONF_FILE_READER, ("token[1]: %s\n", token[1]));

    /* Look up a keyword first in common, then in media-specific
     * configuration definition tables.
     */
    strupcase(token[0]);
    //common		  key	    value
    err = set_conf_param(token[0], token[1], 
    		common_conftab, link_defs->linkconf, sizeof(wandev_conf_t),link_defs, NULL);

    // 'config_id' specific
    if ((err < 0) && (conf_table != NULL)){
      err = set_conf_param(token[0], token[1], 
      		conf_table, &link_defs->linkconf->u, sizeof(link_defs->linkconf->u), link_defs, NULL);
    }

    if(err < 0) {
      ERR_DBG_OUT((" * Unknown parameter %s!\n", token[0]));
      return ERR_CONFIG;
    }

    if (err){
      ERR_DBG_OUT(("Error parsing configuration file!\n"));
      return err;
    }
  }

  return err;
}

/*============================================================================
 * Read configuration file section.
 *	Return a pointer to memory filled with key entries read from given
 *	section or NULL if section is not found. Each key is a zero-terminated
 *	ASCII string with no leading spaces and comments. The last key is
 *	followed by an empty string.
 *	Note:	memory is allocated dynamically and must be released by the
 *		caller.
 */
char* conf_file_reader::read_conf_section (FILE* file, char* section)
{
  char key[MAX_CFGLINE];		/* key buffer */
  char* buf = NULL;
  int found = 0, offs = 0, len, buf_len = 0;
  int dbg_len = 0;

  Debug(DBG_CONF_FILE_READER, ("conf_file_reader::read_conf_section ():%s\n", section));
	
  rewind(file);

  while ((len = read_conf_record(file, key, MAX_CFGLINE)) > 0) {
    char* tmp;

    dbg_len += len;

    if (found) {

      if (*key == '['){
	break;	/* end of section */
      }

      if (buf){
      	buf_len = offs+len+1;
        tmp = (char*)realloc(buf, offs + len + 1);
      }else{
        tmp = (char*)malloc(len + 1);
      	buf_len = len+1;
      }

      if (tmp) {
	buf = tmp;
	strlcpy(&buf[offs], key, buf_len-offs);
	offs += len;
	buf[offs] = '\0';
      }else{
	/* allocation failed */
	ERR_DBG_OUT(("Failed to allocate memory!\n"));
	break;
      }
      
    }else if (*key == '[') {
      //getting here on the 1-st pass	    
      tmp = strchr(key, ']');
      if (tmp != NULL) {
	*tmp = '\0';
	if (strcmp(&key[1], section) == 0) {
	  if (verbose){
	    printf(" ****** Reading section [%s]...\n", section);
	    found = 1;
	  }
	}
      }
    }
    
    if (verbose && !found){
      //printf(" * section [%s] not found!\n", section);
    }
  }//while()

/*
  char* dbg_ptr;
  int counter=0;
  Debug(DBG_CONF_FILE_READER, ("\nprinting [%s] section buffer:\n", section));
  for(dbg_ptr=buf; counter++ < dbg_len; dbg_ptr++){
    //Debug(DBG_CONF_FILE_READER, ("%c", *dbg_ptr));
    if(isprint(*dbg_ptr) || isspace(*dbg_ptr)){
      Debug(DBG_CONF_FILE_READER, ("%c", *dbg_ptr));
    }
  }
  Debug(DBG_CONF_FILE_READER, ("\n\n"));
*/
  
  //Debug(DBG_CONF_FILE_READER, ("returning buf: %p\n", buf));
  
  return buf;
}

/*============================================================================
 * Read a single record from the configuration file.
 *	Read configuration file stripping comments and leading spaces until
 *	non-empty line is read.  Copy it to the destination buffer.
 *
 * Return string length (incl. terminating zero) or 0 if end of file has been
 * reached.
 */
int conf_file_reader::read_conf_record (FILE* file, char* key, int max_len)
{
  char buf[MAX_CFGLINE];		/* line buffer */

  //Debug(DBG_CONF_FILE_READER, ("conf_file_reader::read_conf_record():key: %s\n", key));

  while (fgets(buf, MAX_CFGLINE, file)) {

    char* str;
    int len;

    /* Strip leading spaces and tabs */
    for (str = buf; *str && strchr(" \t", *str); ++str){
      ;
    }

    /* Skip comments and empty lines */
    len = strcspn(str, "#;\n\r");
    if (len) {
      str[len] = '\0';

      //Debug(DBG_CONF_FILE_READER, ("read_conf_record():str: %s\n", str));

      strlcpy(key, str, max_len);
      return len + 1;
    }
  }
  return 0;
}

/*============================================================================
 * Build a list of channel definitions.
 */
/*
 * sample output
key: w3g1 = wanpipe3,  , STACK, Comment
key: w3g1ppp = wanpipe3, , WANPIPE, ppp, ppp.ppp1
key: w3g2 = wanpipe3,  , STACK, Comment
key: w3g2f16 = wanpipe3, 16, WANPIPE, fr, fr.fr2
key: w3g2f17 = wanpipe3, 17, API, fr, fr.fr2
key: w3g3 = wanpipe3,  , API, Comment

w5g1 = wanpipe5,  , STACK, Comment
w5g1tty0 = wanpipe5, 0, TTY, tty, tty.tty1


key: w3g1 = wanpipe3,  , STACK, Comment
token[0]: w3g1
token[1]: wanpipe3
token[2]:
token[3]: STACK
token[4]: Comment

 =========================
key: w3g1ppp = wanpipe3, , WANPIPE, ppp, ppp.ppp1
token[0]: w3g1ppp
token[1]: wanpipe3
token[2]:
token[3]: WANPIPE
token[4]: ppp
token[5]: ppp.ppp1

 =========================
key: w3g2 = wanpipe3,  , STACK, Comment
token[0]: w3g2
token[1]: wanpipe3
token[2]:
token[3]: STACK
token[4]: Comment

 =========================
key: w3g2f16 = wanpipe3, 16, WANPIPE, fr, fr.fr2
token[0]: w3g2f16
token[1]: wanpipe3
token[2]: 16
token[3]: WANPIPE
token[4]: fr
token[5]: fr.fr2

 =========================
key: w3g2f17 = wanpipe3, 17, API, fr, fr.fr2
token[0]: w3g2f17
token[1]: wanpipe3
token[2]: 17
token[3]: API
token[4]: fr
token[5]: fr.fr2

 =========================
key: w3g3 = wanpipe3,  , API, Comment
token[0]: w3g3
token[1]: wanpipe3
token[2]:
token[3]: API
token[4]: Comment

 =========================
*/
int conf_file_reader::read_interfaces_section(
		FILE* file,
	       	objects_list* parent_objects_list,
		char* parent_dev_name,
	       	int* current_line_number
		)
{
  int err = 0;
  char* conf;		/* -> section buffer */
  char* key;		/* -> current configuration record */
  int len = 0;		/* record length */
  char *token[MAX_TOKENS];
  char dev_name_backup[LEN_OF_DBG_BUFF]; 
  int local_line_number = 0;
  
  Debug(DBG_CONF_FILE_READER, ("conf_file_reader::read_interfaces_section()\n"));

  if(parent_dev_name != NULL){
    Debug(DBG_CONF_FILE_READER, (" 'parent_dev_name': %s\n",  parent_dev_name));
  }else{
    Debug(DBG_CONF_FILE_READER, (" 'parent_dev_name' is NULL\n"));
  }
  
  /* Read [interfaces] section */
  conf = read_conf_section(file, "interfaces");
  if (conf == NULL){
    ERR_DBG_OUT(("Failed to read '[interfaces]' section!\n"));
    return ERR_CONFIG;
  }
  
  /* For each record in [interfaces] section create channel definition structure.
   * If channel is usedby STACK, read channels belonging to this stack.
   */
  for (key = conf; !err && *key; key += len) {
    int toknum;
    char *channel_sect;
    list_element_chan_def* list_el_chan_def = NULL;
    chan_def_t* chandef = NULL;
    wanif_conf_t* chanconf = NULL;

    len = strlen(key) + 1;

    Debug(DBG_CONF_FILE_READER, ("key: %s\n", key));

    //have to start parsing at the line passed to this function,
    //all the preceeding lines were already parsed
    if(local_line_number != *current_line_number){
      Debug(DBG_CONF_FILE_READER, ("skipping line: %d, key: %s\n", local_line_number, key));
      local_line_number++;
      //key += len;  
      continue;
    }
    
    toknum = tokenize(key, token);
    if (toknum < 4){
      ERR_DBG_OUT(("Invalid number of tokens: %d, in key:%s. Expected at least 4!\n",
			     toknum, key));
      return ERR_CONFIG;
      //continue;
    }
/*
    for(indx=0; indx < toknum; indx++){
      Debug(DBG_CONF_FILE_READER_AFT, ("token[%d]: %s\n", indx, token[indx]));
    }
*/
    Debug(DBG_CONF_FILE_READER_AFT, ("\n ========================= \n"));

    //use copy of the name because 'strstr()' inserts nulls and it
    //will break things.
    snprintf(dev_name_backup, LEN_OF_DBG_BUFF, token[0]); 
    
    if(parent_dev_name != NULL){
	    
      char parent_dev_name_backup[LEN_OF_DBG_BUFF]; 
      
#if defined(__LINUX__) && !BSD_DEBG
      snprintf(parent_dev_name_backup, WAN_IFNAME_SZ, "%s",
	  parent_dev_name);
#elif (defined __FreeBSD__) || (defined __OpenBSD__) || defined(__NetBSD__) || BSD_DEBG
      
      //on BSD parent device name ends with a digit, but digits are illigal in
      //the middle of upper (or any) layer names
      snprintf(parent_dev_name_backup, WAN_IFNAME_SZ, "%s",
	  replace_numeric_with_char(parent_dev_name));
#endif
      
      Debug(DBG_CONF_FILE_READER, ("searching '%s' in '%s'\n",
		         parent_dev_name_backup, dev_name_backup));
      
      //check this line belongs to the parent device
      if(strstr(dev_name_backup, parent_dev_name_backup) == NULL){
	//if not, return from the recursive call.
	Debug(DBG_CONF_FILE_READER, ("this line does NOT belong to parent device: %s\n",
			        parent_dev_name));
	return 0;
      }else{
	Debug(DBG_CONF_FILE_READER, ("this line does BELONGS to parent device: %s\n",
			        parent_dev_name));
      }
    }  
   
    //allocate and initialize channel definition structure
    list_el_chan_def = new list_element_chan_def();
    chandef = &list_el_chan_def->data;
    chanconf = chandef->chanconf;

    snprintf(chandef->name, WAN_IFNAME_SZ, token[0]);
    snprintf(chandef->addr, MAX_ADDR_STR_LEN, token[2]);
   
    //NOTE: 'atoi()' returns zero if it failes AND if number
    //is actually zero.
    if(atoi(chandef->addr) == 0){
      
      Debug(DBG_CONF_FILE_READER, ("no valid 'chandef->addr', using 'list->get_size()': %d\n",
		        parent_objects_list->get_size()));

      //1. There is nothing at all (blank space instead of a number).
      //   It is possible for a STACK or PPP interface where
      //   there is no "address" or "dlci number".
      //OR
      //2. For TTY zero is a *valid* number.
      //OR
      //3. For WANCONFIG_MFR there can be "auto" instead of the "dlci number".
      //   In this case there must be only one dlci/interface and '1' will mean "auto".
      //
      //   Current 'size' of the list will be actually the sequence number. Start from 1.
      //   									0+1	
      snprintf(chandef->addr, MAX_ADDR_STR_LEN, "%d", parent_objects_list->get_size() + 1);
    }

    if(toknum == 6){
      if(strcmp(token[4],"tty") == 0){
	strlcpy(chandef->addr, token[2], MAX_ADDR_STR_LEN);
      }
    }
    //print 'addr' after all the updates    
    Debug(DBG_CONF_FILE_READER, ("Final 'chandef->addr': %s\n", chandef->addr));
	
    if (toknum > 5){
      chanconf->config_id = get_config_id_from_profile(token[4]);
      if(chanconf->config_id == PROTOCOL_NOT_SET){
	ERR_DBG_OUT(("Invalid 'config_id': %s in profile!\n", token[4]));
	return ERR_CONFIG;
      }
    }
    
    chandef->usedby = get_used_by_integer_value(token[3]); 
    switch(chandef->usedby)
    {
    case WANPIPE:
    case API:
    case TDM_API:
    case BRIDGE:
    case BRIDGE_NODE:
    //case ANNEXG:
    case TDM_VOICE:
    case TDM_VOICE_API:
    case TTY:
    case PPPoE:
    case WP_NETGRAPH:
      //do nothing special
      break;

    case STACK:
      //allocate new object list for THIS stack
      list_el_chan_def->next_objects_list = new objects_list();
      if(list_el_chan_def->next_objects_list == NULL){
	ERR_DBG_OUT(("Memory allocation failed for 'next_objects_list'!!\n"));
	return ERR_CONFIG;
      }
      break;

    default:
      ERR_DBG_OUT(("Unsupported 'usedby' string: '%s' for interface: %s!\n",
						  token[3], token[0]));
      return ERR_CONFIG;
    }
    
    channel_sect=strdup(token[0]);
		
    if (toknum > 5){
      //there is LIP layer
      char *section=strdup(token[5]);

      /* X25_SW */
      if (toknum > 7){
	if (toknum > 6)
	  chandef->virtual_addr = strdup(token[6]);
	if (toknum > 7)
	  chandef->real_addr = strdup(token[7]);
	if (toknum > 8)
	  chandef->label = strdup(token[8]);
      }else{
	if (toknum > 6)
	  chandef->label = strdup(token[6]);
      }

      Debug(DBG_CONF_FILE_READER_AFT,
        ("calling 'chandef->conf_profile = read_conf_section(file, section: %s)'\n",
	  section));

      //read the profile
      chandef->conf_profile = read_conf_section(file, section);
      free(section);

    }else{
      chandef->conf_profile = NULL;
    }//if(toknum > 5)

    chandef->conf = read_conf_section(file, channel_sect);	
    free(channel_sect);

    //If 'chanconf->config_id' is 0, it means it is NOT a LIP Interface,
    //it is Hardware Interface, so it will have the same config_id as
    //the actual Hardware which is WANCONFIG_HDLC.
    //Note, that 'link_defs->linkconf->config_id' can NOT always be used because
    //for example, on S514 with LIP layer it will be set to WAN_MULTPROT or WAN_MULTPPP,
    //which indicates PPP!!! Inconsistency of conf file again.
    
    if(chanconf->config_id == 0){
      //Right now 'WANCONFIG_MPPP' is the same as 'WANCONFIG_MPROT',
      //but in case it changed one day, check for both.
      if(link_defs->linkconf->config_id == WANCONFIG_MPPP ||
	 link_defs->linkconf->config_id ==  WANCONFIG_MPROT){

        chanconf->config_id = WANCONFIG_HDLC;
      }else{
        chanconf->config_id = link_defs->linkconf->config_id;
      }
    }

    /*
    if(chanconf->config_id == WANCONFIG_AFT || chanconf->config_id == WANCONFIG_AFT_TE3){
      chanconf->config_id = WANCONFIG_HDLC;
    }
    */
    
    //now, when configuration was read to a buffer, set all structures
    //to real values 
    err = configure_chan(chandef, chanconf->config_id);
    if (err){
      return ERR_CONFIG;
    }
    
    if(parent_objects_list != NULL){

      Debug(DBG_CONF_FILE_READER, ("inserting interface: %s.\n", token[0]));
	    
      if(parent_objects_list->insert(list_el_chan_def) != LIST_ACTION_OK){
	ERR_DBG_OUT(("Failed to insert interface %s to list!\n", token[0]));
	return ERR_CONFIG;
      }

      //advance the global 'current_line_number' for the next iteration
      Debug(DBG_CONF_FILE_READER, ("old current_line_number: %d\n", *current_line_number));
      (*current_line_number)++;
      Debug(DBG_CONF_FILE_READER, ("new current_line_number: %d\n", *current_line_number));
      
      //update the local one in order not skip the next line.
      local_line_number = *current_line_number;
    }else{
      ERR_DBG_OUT(("Error: channels list pointer is NULL!!\n"));
      return ERR_CONFIG;
    }    

    if(chandef->usedby == STACK){
      //the next interface (line in the file) we read belongs to this STACK,
      //not to the original 'parent_objects_list'.
      //NOTE: recursive call!!
      err = read_interfaces_section(
		      file,
		      (objects_list*)list_el_chan_def->next_objects_list,
		      token[0],
		      current_line_number
		      );
    }//if()
    
  }//for()

  return 0;
}


/*============================================================================
 * Set configuration parameter.
 *	Look up a keyword in configuration description table to find offset
 *	and data type of the configuration parameter into configuration data
 *	buffer.  Convert parameter to apporriate data type and store it the
 *	configuration data buffer.
 *
 *	Return:	0 - success
 *		1 - error
 *		-1 - not found
 */
int conf_file_reader::set_conf_param (char* key, char* val, key_word_t* dtab, 
			void* conf, int size, link_def_t* lnks_def, chan_def_t* chan_def)
{
  unsigned int tmp = 0;

//  Debug(DBG_CONF_FILE_READER, ("conf_file_reader::set_conf_param()\n"));
//  Debug(DBG_CONF_FILE_READER, ("key ::::%s\n", key));

  /* Search a keyword in configuration definition table */
  for (; dtab->keyword && strcmp(dtab->keyword, key); ++dtab){
    ;
  }

  if(dtab->keyword == NULL){
    //searched to end of the table.
    Debug(DBG_CONF_FILE_READER, ("key not found in the table!! key: %s\n", key));
    return -1;	/* keyword not found */
  }

  /* Interpert parameter value according to its data type */
  if (dtab->dtype == DTYPE_FILENAME && lnks_def != NULL) {

    if(strcmp(key, "FIRMWARE") == 0){
      Debug(DBG_CONF_FILE_READER, ("file name: %s\n", val));
      snprintf(lnks_def->firmware_file_path, MAX_PATH_LENGTH, val);
    }
    return 0;
  }

  if (verbose){
    printf(" * Setting %s to %s\n", key, val);
  }

  if (dtab->dtype == DTYPE_STR) {
    strlcpy((char*)conf + dtab->offset, val, size-dtab->offset);
    return 0;
  }

  if( !isdigit(*val) ||
      strcmp(key, "ACTIVE_CH") == 0 ||
      strcmp(key, "LBO") == 0 ||
      strcmp(key, "MEDIA") == 0 || /* old style for FE_MEDIA */
      strcmp(key, "FE_MEDIA") == 0 ||
      strcmp(key, "TDMV_HWEC_MAP") == 0 ) {
	  
    look_up_t* sym;
    unsigned int tmp_ch;

	FUNC_DBG();

    strupcase(val);
    for (sym = sym_table; sym->ptr && strcmp((char*)sym->ptr, val); ++sym){
      ;
    }
	
    if (sym->ptr == NULL) {
      /* TE1 */
      if(strcmp(key, "TDMV_HWEC_MAP")){
        if (strcmp(key, "ACTIVE_CH")) {		
	  ERR_DBG_OUT((" * invalid term %s ...\n", val));
	  return -1;
        }
	
        if(lnks_def != NULL){
          /* TE1 Convert active channel string to ULONG */
          tmp_ch = parse_active_channel(val, lnks_def->linkconf->fe_cfg.media);	
          if (tmp_ch == 0){
	    ERR_DBG_OUT(("Illegal active channel range for media type ! (%s).\n", val));
	    return -1;
	  }
	  tmp = (unsigned int)tmp_ch;
        }
      }

      Debug(DBG_CONF_FILE_READER, ("Active Channels val: ")); Debug(DBG_CONF_FILE_READER, ("%s\n", val));
      Debug(DBG_CONF_FILE_READER, ("Active Channels int tmp: ")); Debug(DBG_CONF_FILE_READER, ("0x%X\n", tmp));
      
      //Active Channels can be passed for both the 'device' and the 'channel'
      //Store the active channels string for display and modification by the user.
      if(lnks_def != NULL){
        //time slots for this 'device'
        snprintf(lnks_def->active_channels_string, MAX_LEN_OF_ACTIVE_CHANNELS_STRING, "%s", val);
        //lnks_def->chanconf->active_ch = tmp;
      }else if(conf != NULL && chan_def != NULL){
        //time slots for this 'channel'
        if(strcmp(key, "TDMV_HWEC_MAP") == 0){
          snprintf(chan_def->active_hwec_channels_string, MAX_LEN_OF_ACTIVE_CHANNELS_STRING, "%s", val);
	  //chan_def->hwec_flag = WANOPT_YES;

          Debug(DBG_CONF_FILE_READER, ("HWEC Channels: %s\n", 
		chan_def->active_hwec_channels_string));
	}else{
          snprintf(chan_def->active_channels_string, MAX_LEN_OF_ACTIVE_CHANNELS_STRING, "%s", val);
	}
        //chan_def->chanconf->active_ch = tmp;
      }
      
    }else{
      tmp = sym->val;
    }
    
  }else{
	FUNC_DBG();
    tmp = strtoul(val, NULL, 0);
  }//if()
  
  switch (dtab->dtype) 
  {
  case DTYPE_INT:
    SIZEOFASSERT(dtab, sizeof(int));     
    *(int*)((char*)conf + dtab->offset) = tmp;
    break;

  case DTYPE_UINT:
    SIZEOFASSERT(dtab, sizeof(unsigned int));     
    *(uint*)((char*)conf + dtab->offset) = tmp;
    break;

  case DTYPE_LONG:
    SIZEOFASSERT(dtab, sizeof(long));     
    *(long*)((char*)conf + dtab->offset) = tmp;
    break;

  case DTYPE_ULONG:
    SIZEOFASSERT(dtab, sizeof(unsigned long));     
    *(unsigned long*)((char*)conf + dtab->offset) = tmp;
    break;

  case DTYPE_SHORT:
    SIZEOFASSERT(dtab, sizeof(short));     
    *(short*)((char*)conf + dtab->offset) = tmp;
    break;

  case DTYPE_USHORT:
    SIZEOFASSERT(dtab, sizeof(unsigned short));     
    *(ushort*)((char*)conf + dtab->offset) = tmp;
    break;

  case DTYPE_CHAR:
    SIZEOFASSERT(dtab, sizeof(char));     
    *(char*)((char*)conf + dtab->offset) = tmp;
    break;

  case DTYPE_UCHAR:
    SIZEOFASSERT(dtab, sizeof(unsigned char));     
    *(unsigned char*)((char*)conf + dtab->offset) = tmp;
    break;

  case DTYPE_PTR:
    *(void**)((char*)conf + dtab->offset) = (void*)tmp;
    break;
  }
  return 0;
}

/*============================================================================
 * Configure WAN logical channel.
 */
#if 0
int conf_file_reader::configure_chan (chan_def_t* chandef, int id)
{
  int err = 0;
  int len = 0;
  char* conf_rec;
  char *token[MAX_TOKENS];
  key_word_t* conf_table = (key_word_t*)lookup(id, conf_if_def_tables);
#if DBG_FLAG
  wan_fr_conf_t* fr = &chandef->chanconf->u.fr;
#endif

  Debug(DBG_CONF_FILE_READER, ("conf_file_reader::configure_chan(): id: %d\n", id));
	
  if(!conf_table){
    Debug(DBG_CONF_FILE_READER, ("conf_table is NULL!!\n"));
  }

  /* Prepare configuration data */
  strncpy(chandef->chanconf->name, chandef->name, WAN_IFNAME_SZ);
  if (chandef->addr){
    strncpy(chandef->chanconf->addr, chandef->addr, WAN_ADDRESS_SZ);
  }
	
  if (chandef->usedby){
    strncpy(chandef->chanconf->usedby, get_used_by_string(chandef->usedby), USED_BY_FIELD);
  }
	
  if(chandef->label){
    Debug(DBG_CONF_FILE_READER, ("configure_chan(): chandef->label: %s\n", chandef->label));
    memcpy(chandef->chanconf->label, chandef->label, WAN_IF_LABEL_SZ);
  }else{
    Debug(DBG_CONF_FILE_READER, ("configure_chan(): no 'chandef->label'\n"));	
  }
  
  if (chandef->conf){
    
    for (conf_rec = chandef->conf; *conf_rec; conf_rec += len) {
      int toknum;
		
      len = strlen(conf_rec) + 1;
      toknum = tokenize(conf_rec, token);
      if (toknum < 2) {
        return -EINVAL;
      }

      //
      // Look up a keyword first in common, then media-specific
      // configuration definition tables.
      //      
      strupcase(token[0]);
      if (set_conf_param(token[0], token[1], chan_conftab, chandef->chanconf, NULL, chandef)) {

        if (!conf_table ||
	    set_conf_param(token[0], token[1], conf_table, &chandef->chanconf->u, NULL, chandef)) {

	  //not found in 'conf_if_def_tables' too !!
	  ERR_DBG_OUT(("Invalid parameter %s\n", token[0]));
	  return ERR_CONFIG;
	}
      }
    }

    Debug(DBG_CONF_FILE_READER, ("Current 'chandef->chanconf->config_id': %d\n", 
	chandef->chanconf->config_id));

    if(chandef->usedby == TDM_VOICE){
    	Debug(DBG_CONF_FILE_READER, ("TDM_VOICE channel!!\n"));
	chandef->chanconf->config_id = PROTOCOL_TDM_VOICE;
    }
    if(chandef->usedby == TDM_VOICE_API){
    	Debug(DBG_CONF_FILE_READER, ("TDM_VOICE_API channel!!\n"));
	chandef->chanconf->config_id = PROTOCOL_TDM_VOICE_API;
    }

    Debug(DBG_CONF_FILE_READER, ("Final 'chandef->chanconf->config_id': %d.\n",
			    chandef->chanconf->config_id));

    len=0;
      
    if (chandef->conf_profile){
	      
      for(conf_rec = chandef->conf_profile; *conf_rec; conf_rec += len) {
	      
	int toknum;
	wanif_conf_t* chanconf = chandef->chanconf;
	     
	len = strlen(conf_rec) + 1;
	toknum = tokenize(conf_rec, token);

	Debug(DBG_CONF_FILE_READER_AFT, ("toknum: %d\n", toknum));	

	if (toknum != 2) {
	  return -EINVAL;
	}

	strupcase(token[0]);

	Debug(DBG_CONF_FILE_READER_AFT, ("chandef->addr: %s\n", chandef->addr));	
	Debug(DBG_CONF_FILE_READER_AFT, ("0x%p, chanconf->config_id: %d (%s)\n",
		    chanconf, chanconf->config_id, get_protocol_string(chanconf->config_id)));	

	switch(chanconf->config_id)
	{
	case WANCONFIG_MFR:
	  Debug(DBG_CONF_FILE_READER_AFT, ("WANCONFIG_MFR\n"));	

	  if(set_conf_param(token[0], token[1], fr_conftab, &chandef->chanconf->u.fr,
			  NULL, chandef)) {
	    ERR_DBG_OUT(("Invalid parameter %s\n", token[0]));
	    return ERR_CONFIG;
	  }//if()

	  Debug(DBG_CONF_FILE_READER_AFT, ("fr->signalling: %d\n", fr->signalling));	
	  Debug(DBG_CONF_FILE_READER_AFT, ("fr->station   : %d\n", fr->station));	
	  break;

	case WANCONFIG_TTY:
	  //do nothing - everything was read already from : 'chan_conftab'
	  //and/or  'conf_table'
	  Debug(DBG_CONF_FILE_READER_AFT, ("WANCONFIG_TTY\n"));	
	  Debug(DBG_CONF_FILE_READER_AFT, ("token[0]: %s\n", token[0]));
	  Debug(DBG_CONF_FILE_READER_AFT, ("token[1]: %s\n", token[1]));
	  break;
	  
	case WANCONFIG_MPPP:
	  Debug(DBG_CONF_FILE_READER_AFT, ("WANCONFIG_MPPP\n"));	
	  Debug(DBG_CONF_FILE_READER_AFT, ("token[0]: %s\n", token[0]));
	  Debug(DBG_CONF_FILE_READER_AFT, ("token[1]: %s\n", token[1]));

	  if(set_conf_param(token[0], token[1], sppp_conftab, &chandef->chanconf->u.ppp,
			  NULL, chandef)) {
	    ERR_DBG_OUT(("Invalid parameter %s\n", token[0]));
	    return ERR_CONFIG;
	  }//if()

	  //Debug(DBG_CONF_FILE_READER_AFT, ("link_defs->linkconf->u.ppp.ip_mode: %d\n",
	  //			 link_defs->linkconf->u.ppp.ip_mode));
	  break;

	case WANCONFIG_MPCHDLC:
	  //profile for this protocol is empty, so we never get here.
	  Debug(DBG_CONF_FILE_READER_AFT, ("WANCONFIG_MPCHDLC\n"));	
	  Debug(DBG_CONF_FILE_READER_AFT, ("token[0]: %s\n", token[0]));
	  Debug(DBG_CONF_FILE_READER_AFT, ("token[1]: %s\n", token[1]));

	  break;
	
	case WANCONFIG_LAPB:
	  Debug(DBG_CONF_FILE_READER_AFT, ("WANCONFIG_LAPB\n"));	
	  Debug(DBG_CONF_FILE_READER_AFT, ("token[0]: %s\n", token[0]));
	  Debug(DBG_CONF_FILE_READER_AFT, ("token[1]: %s\n", token[1]));

	  if(set_conf_param(token[0], token[1], lapb_annexg_conftab, &chandef->chanconf->u.lapb,
			  NULL, chandef)) {
	    ERR_DBG_OUT(("Invalid parameter %s\n", token[0]));
	    return ERR_CONFIG;
	  }//if()
	  break;
	  
	default:
	  //it actually means that this protocol MUST NOT have a profile section
	  ERR_DBG_OUT(("Protocol %d(%s) MUST NOT have a profile section!\n", 
	    	chanconf->config_id, get_protocol_string(chanconf->config_id)));
	  return ERR_CONFIG;
	}//switch()

      }//for()
    }//if(chandef->conf_profile)
    {
      Debug(DBG_CONF_FILE_READER, ("chandef->conf_profile == NULL!!\n"));
    }
  }//if(chandef->conf)
  else
  {  Debug(DBG_CONF_FILE_READER, ("chandef->conf == NULL!!\n"));	  
  }	
  return err;
}
#endif

int conf_file_reader::configure_chan (chan_def_t* chandef, int id)
{
  int err = 0;
  int len = 0;
  char* conf_rec;
  char *token[MAX_TOKENS];
  key_word_t* conf_table = (key_word_t*)lookup(id, conf_if_def_tables);
#if DBG_FLAG
  wan_fr_conf_t* fr = &chandef->chanconf->u.fr;
#endif

  Debug(DBG_CONF_FILE_READER, ("conf_file_reader::configure_chan(): id: %d\n", id));
	
  if(!conf_table){
    Debug(DBG_CONF_FILE_READER, ("conf_table is NULL!!\n"));
  }

  /* Prepare configuration data */
  strncpy(chandef->chanconf->name, chandef->name, WAN_IFNAME_SZ);
  if (chandef->addr){
    strncpy(chandef->chanconf->addr, chandef->addr, WAN_ADDRESS_SZ);
  }
	
  if (chandef->usedby){
    strncpy(chandef->chanconf->usedby, get_used_by_string(chandef->usedby), USED_BY_FIELD);
  }
	
  if(chandef->label){
    Debug(DBG_CONF_FILE_READER, ("configure_chan(): chandef->label: %s\n", chandef->label));
    memcpy(chandef->chanconf->label, chandef->label, WAN_IF_LABEL_SZ);
  }else{
    Debug(DBG_CONF_FILE_READER, ("configure_chan(): no 'chandef->label'\n"));	
  }
 
  //if there is any information in the [interface] section, get it.
  //some interfaces have no information.
  if (chandef->conf){
    for (conf_rec = chandef->conf; *conf_rec; conf_rec += len) {
      int toknum;
		
      len = strlen(conf_rec) + 1;
      toknum = tokenize(conf_rec, token);
      if (toknum < 2) {
        return -EINVAL;
      }

      // Look up a keyword first in common, then media-specific
      // configuration definition tables.
      strupcase(token[0]);
      if (set_conf_param(token[0], token[1], chan_conftab, chandef->chanconf, sizeof(wanif_conf_t), NULL, chandef)) {

        if (!conf_table ||
	    set_conf_param(token[0], token[1], conf_table, &chandef->chanconf->u, sizeof(chandef->chanconf->u), NULL, chandef)) {

          if(id == WANCONFIG_ADSL && link_defs->sub_config_id == WANCONFIG_MPPP){
            //on ADSL there can be PPP without the LIP layer!!
	    if(set_conf_param(token[0], token[1], 
	    	sppp_conftab, &chandef->chanconf->u.ppp,sizeof(wan_sppp_if_conf_t),NULL, chandef)) {
	      ERR_DBG_OUT(("Invalid parameter %s\n", token[0]));
	      return ERR_CONFIG;
	    }//if()
/*
Nov 21, 2006 - not done anymore - all configuration is in profile
          }else if(id == WANCONFIG_MPCHDLC){
	    //getting here because CHDLC configuration is kept in 2 places:
	    // 1. wanif conf  2. in u.ppp
	    if(set_conf_param(token[0], token[1], sppp_conftab, &chandef->chanconf->u.ppp,
			  NULL, chandef)) {
	      ERR_DBG_OUT(("Invalid parameter %s\n", token[0]));
	      return ERR_CONFIG;
	    }//if()
*/
	  }else{
	    //not found in 'conf_if_def_tables' too !!
	    ERR_DBG_OUT(("Invalid parameter %s\n", token[0]));
	    return ERR_CONFIG;
	  }
	}
      }//if()
    }//for()

    Debug(DBG_CONF_FILE_READER, ("Current 'chandef->chanconf->config_id': %d\n", 
	chandef->chanconf->config_id));

    if(chandef->usedby == TDM_VOICE){
    	Debug(DBG_CONF_FILE_READER, ("2. TDM_VOICE channel!!\n"));
	chandef->chanconf->config_id = PROTOCOL_TDM_VOICE;
    }
    if(chandef->usedby == TDM_VOICE_API){
    	Debug(DBG_CONF_FILE_READER, ("2. TDM_VOICE_API channel!!\n"));
	chandef->chanconf->config_id = PROTOCOL_TDM_VOICE_API;
    }

    Debug(DBG_CONF_FILE_READER, ("Final 'chandef->chanconf->config_id': %d.\n",
			    chandef->chanconf->config_id));

    len=0;
      
  }//if(chandef->conf)
  else
  {  Debug(DBG_CONF_FILE_READER, ("chandef->conf == NULL!!\n"));
  }	

  //if there is anything in [profile] section, get it
  if (chandef->conf_profile){
     
    for(conf_rec = chandef->conf_profile; *conf_rec; conf_rec += len) {
	      
      int toknum;
      wanif_conf_t* chanconf = chandef->chanconf;
	     
      len = strlen(conf_rec) + 1;
      toknum = tokenize(conf_rec, token);

      Debug(DBG_CONF_FILE_READER_AFT, ("toknum: %d\n", toknum));	

      if (toknum != 2) {
        return -EINVAL;
      }

      strupcase(token[0]);

      Debug(DBG_CONF_FILE_READER_AFT, ("chandef->addr: %s\n", chandef->addr));	
      Debug(DBG_CONF_FILE_READER_AFT, ("0x%p, chanconf->config_id: %d (%s)\n",
		    chanconf, chanconf->config_id, get_protocol_string(chanconf->config_id)));	

      switch(chanconf->config_id)
      {
      case WANCONFIG_MFR:
        Debug(DBG_CONF_FILE_READER_AFT, ("WANCONFIG_MFR\n"));	

	if(set_conf_param(token[0], token[1], 
		fr_conftab, &chandef->chanconf->u.fr, sizeof(wan_fr_conf_t),NULL, chandef)) {
	  ERR_DBG_OUT(("Invalid parameter %s\n", token[0]));
	  return ERR_CONFIG;
	}//if()

	Debug(DBG_CONF_FILE_READER_AFT, ("fr->signalling: %d\n", fr->signalling));	
	Debug(DBG_CONF_FILE_READER_AFT, ("fr->station   : %d\n", fr->station));	
	break;

      case WANCONFIG_TTY:
        //do nothing - everything was read already from : 'chan_conftab'
	//and/or  'conf_table'
	Debug(DBG_CONF_FILE_READER_AFT, ("WANCONFIG_TTY\n"));	
	Debug(DBG_CONF_FILE_READER_AFT, ("token[0]: %s\n", token[0]));
	Debug(DBG_CONF_FILE_READER_AFT, ("token[1]: %s\n", token[1]));
	break;
	  
      case WANCONFIG_MPPP:
	Debug(DBG_CONF_FILE_READER_AFT, ("WANCONFIG_MPPP\n"));	
	Debug(DBG_CONF_FILE_READER_AFT, ("token[0]: %s\n", token[0]));
	Debug(DBG_CONF_FILE_READER_AFT, ("token[1]: %s\n", token[1]));

	if(set_conf_param(token[0], token[1], 
		sppp_conftab, &chandef->chanconf->u.ppp,sizeof(wan_sppp_if_conf_t),NULL, chandef)) {
	  ERR_DBG_OUT(("Invalid parameter %s\n", token[0]));
	  return ERR_CONFIG;
	}//if()

	//Debug(DBG_CONF_FILE_READER_AFT, ("link_defs->linkconf->u.ppp.ip_mode: %d\n",
	//			 link_defs->linkconf->u.ppp.ip_mode));
	break;

      case WANCONFIG_CHDLC:
      case WANCONFIG_MPCHDLC:
        //profile for this protocol is empty, so we never get here.
	Debug(DBG_CONF_FILE_READER_AFT, ("WANCONFIG_MPCHDLC\n"));	
	Debug(DBG_CONF_FILE_READER_AFT, ("token[0]: %s\n", token[0]));
	Debug(DBG_CONF_FILE_READER_AFT, ("token[1]: %s\n", token[1]));

        if(set_conf_param(token[0], token[1], 
		sppp_conftab, &chandef->chanconf->u.ppp,sizeof(wan_sppp_if_conf_t), NULL, chandef)) {
	  ERR_DBG_OUT(("Invalid parameter %s\n", token[0]));
	  return ERR_CONFIG;
	}//if()

	break;
	
      case WANCONFIG_LAPB:
        Debug(DBG_CONF_FILE_READER_AFT, ("WANCONFIG_LAPB\n"));	
	Debug(DBG_CONF_FILE_READER_AFT, ("token[0]: %s\n", token[0]));
	Debug(DBG_CONF_FILE_READER_AFT, ("token[1]: %s\n", token[1]));

	if(set_conf_param(token[0], token[1], 
		lapb_annexg_conftab, &chandef->chanconf->u.lapb,sizeof(wan_lapb_if_conf_t), NULL, chandef)) {
	  ERR_DBG_OUT(("Invalid parameter %s\n", token[0]));
	  return ERR_CONFIG;
	}//if()
	break;
	  
      default:
        //it actually means that this protocol MUST NOT have a profile section
	ERR_DBG_OUT(("Protocol %d(%s) MUST NOT have a profile section!\n", 
	    	chanconf->config_id, get_protocol_string(chanconf->config_id)));
	return ERR_CONFIG;
      }//switch()
    }//for()
  }//if(chandef->conf_profile)
  {
    Debug(DBG_CONF_FILE_READER, ("chandef->conf_profile == NULL!!\n"));
  }
  
  return err;
}

void conf_file_reader::free_linkdefs(void)
{
  Debug(DBG_CONF_FILE_READER, ("conf_file_reader::free_linkdefs()\n"));	

  //David
  if(main_obj_list == NULL){
    printf("Error: free_linkdefs(): main_obj_list is NULL! \n");
    return;
  }

  Debug(DBG_CONF_FILE_READER, ("Freeing Link %s\n", link_defs->name));

  /* Clear (by deleteting) channel definition list */
  delete main_obj_list;
  main_obj_list = NULL;

  if(link_defs == NULL){
    Debug(DBG_CONF_FILE_READER, ("Freeing Link %s\n", link_defs->name));
    return;
  }
	
  if (link_defs->conf) free(link_defs->conf);
  if (link_defs->descr) free(link_defs->descr);
		
  if (link_defs->linkconf){
    if (link_defs->linkconf->data){
      free(link_defs->linkconf->data);
    }
    free(link_defs->linkconf);
  }

  free(link_defs);
  link_defs = NULL;
}

//this function extracts version of S514 (1/2/...7) card from the "conf" file.
//do not call it for other cards
int conf_file_reader::
  get_pci_adapter_type_from_wanpipe_conf_file(char * conf_file_name, unsigned int* card_version)
{
  FILE * conf_file = fopen(conf_file_name, "r+");
  char str_buff[MAX_PATH_LENGTH];

  if(conf_file == NULL){
    ERR_DBG_OUT(("Failed to open configuration file '%s'. File does not exist.\n",
      conf_file_name));
    return NO;
  }

  *card_version = INVALID_CARD_TYPE;
  /////////////////////////////////////////////////////
  Debug(DBG_CONF_FILE_READER, ("Conf file :\n"));
  do{
    fgets(str_buff, MAX_PATH_LENGTH, conf_file);

    if(!feof(conf_file)){

      //Debug(DBG_CONF_FILE_READER, (str_buff));

      if(strstr(str_buff, "CARD_TYPE") != NULL){
        if(get_card_type_from_str(str_buff) == NO){
          //Invalid card type or failed to parse the string.
          return NO;
        }
      }

      *card_version = S5141_ADPTR_1_CPU_SERIAL;

      //if 'MEDIA' exist it can be: 56K or TE1 card
      if(strstr(str_buff, "MEDIA") != NULL){
        if(get_media_type_from_str(str_buff, card_version) == NO){
          //Invalid media type or failed to parse the string.
          return NO;
        }else{
          return YES;
        }
      }//if()
    }//if()

  }while(!feof(conf_file));
  Debug(DBG_CONF_FILE_READER, ("\n"));

  if(*card_version == INVALID_CARD_TYPE){
    ERR_DBG_OUT(("Failed to get S514 card version from 'conf' file!!\n"));
    return NO;
  }else{
    return YES;
  }
}

//call this function
int conf_file_reader::get_card_type_from_str(char * line)
{
  //check it is S514X card
  if(strstr(line, "S51X") != NULL){
    return YES;
  }
  return NO;
}

int conf_file_reader::get_media_type_from_str(char * line, unsigned int* card_version)
{
  char * media_str;

  //check if this is a 56K card.
  media_str = strstr(line, "56K");
  if(media_str != NULL){
    printf("S514-5 56K card\n");
    *card_version = S5145_ADPTR_1_CPU_56K;
    return YES;
  }

  printf("Media type is ");

  //is it T1?
  media_str = strstr(line, "T1");
  if(media_str != NULL){
    printf("T1\n");
    *card_version = S5144_ADPTR_1_CPU_T1E1;
    return YES;
  }

  //is it E1?
  media_str = strstr(line, "E1");
  if(media_str != NULL){
    printf("E1\n");
    *card_version = S5144_ADPTR_1_CPU_T1E1;
    return YES;
  }

  return NO;
}


////////////////////////////////////////////////////////////////////////////////////////////////
//Global (non-member) functions.
void init_first_time_tokens(char **token){
	int i;

	for (i=0;i<MAX_TOKENS;i++){
		token[i]=NULL;
	}
}

/*============================================================================
 * Tokenize string.
 *	Parse a string of the following syntax:
 *		<tag>=<arg1>,<arg2>,...
 *	and fill array of tokens with pointers to string elements.
 *
 *	Return number of tokens.
 */
void init_tokens(char **token){
	int i;

	for (i=0;i<MAX_TOKENS;i++){
		token[i]=NULL;
	}
}

/* Bug Fix by Ren�Scharfe <l.s.r@web.de>
 * removed strtok
 */
int tokenize (char *str, char **tokens)
{
	int cnt = 0;
 	char *tok;

	init_tokens(tokens);

	if (!str){
		return 0;
	}

 	tok = strchr(str, '=');
	if (!tok)
		return 0;

	*tok='\0';

 	tokens[cnt] = str;
	str=++tok;
	
 	while (tokens[cnt] && (cnt < MAX_TOKENS-1)) {
	
		tokens[cnt] = str_strip(tokens[cnt], " \t");
		
		if ((tok = strchr(str, ',')) == NULL){
			tokens[++cnt] = str_strip(str, " \t");
			goto end_tokenize;
		}

		*tok='\0';

		tokens[++cnt] = str_strip(str, " \t");
		str=++tok;
 	}

end_tokenize:
 	return ++cnt;
}

/*============================================================================
 * Strip leading and trailing spaces off the string str.
 */
char* str_strip (char* str, char* s)
{
	char* eos = str + strlen(str);		/* -> end of string */

	while (*str && strchr(s, *str))
		++str;				/* strip leading spaces */
	while ((eos > str) && strchr(s, *(eos - 1)))
		--eos;				/* strip trailing spaces */
	*eos = '\0';
	return str;
}

/*============================================================================
 * Uppercase string.
 */
char* strupcase (char* str)
{
	char* s;

	for(s = str; *s; ++s) *s = toupper(*s);
	return str;
}

/*============================================================================
 * Get a pointer from a look-up table.
 */
void* lookup (int val, look_up_t* table)
{
	while(table->val && (table->val != (uint)val)){
    table++;
  }
	return table->ptr;
}

/*============================================================================
 * Look up a symbolic name in name table.
 *	Return a numeric value associated with that name or zero, if name was
 *	not found.
 */
int name2val (char* name, look_up_t* table)
{
	while (table->ptr && strcmp(name, (char*)table->ptr)) table++;
	return table->val;
}

/*============================================================================
 * Convert decimal string to unsigned integer.
 * If len != 0 then only 'len' characters of the string are converted.
 */
unsigned int dec_to_uint (unsigned char* str, int len)
{
	unsigned val;

	if (!len) len = strlen((char*)str);
	for (val = 0; len && is_digit(*str); ++str, --len)
		val = (val * 10) + (*str - (unsigned)'0')
	;
	return val;
}

//****** End *****************************************************************/
