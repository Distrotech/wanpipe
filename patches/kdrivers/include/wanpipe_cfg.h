/*****************************************************************************
* wanpipe_cfg.h	WANPIPE(tm) Sangoma Device Driver.
******************************************************************************/

#ifndef _WANPIPE_CFG_H_
#define _WANPIPE_CFG_H_

# include "sdla_front_end.h"
# include "wanpipe_cfg_def.h"
# include "wanpipe_cfg_fr.h"
# include "wanpipe_cfg_sppp.h"
# include "wanpipe_cfg_hdlc.h"
# include "wanpipe_cfg_atm.h"
# include "wanpipe_cfg_lip.h"
# include "wanpipe_cfg_adsl.h"
# include "wanpipe_abstr_types.h"

/* The user space applications need the 
   wan_time_t define. However we cannot include
   wanpipe_defines.h in kernel mode due to
   binary only apps like adsl/fr/ppp */
#if !defined(__KERNEL__)
# include "wanpipe_defines.h"
#endif	

/* This is the maximum number of interfaces that any protocol may have.
For example: a number of DLCIs. */
#ifndef MAX_NUMBER_OF_PROTOCOL_INTERFACES
#define MAX_NUMBER_OF_PROTOCOL_INTERFACES	100
#endif


/* Miscellaneous */
#if defined(__WINDOWS__)

/* Frame Relay definitions for sprotocol.dll
(the Protocol Configurator in Device Manager) */
#define MIN_T391	5
#define MAX_T391	30
#define MIN_T392	5
#define MAX_T392	30
#define MIN_N391	1
#define MAX_N391	255
#define MIN_N392	1
#define MAX_N392	10
#define MIN_N393	1
#define MAX_N393	10

#define HIGHEST_VALID_DLCI	991
#define LOWEST_VALID_DLCI	16
/********** end of sprotocol.dll definitions ************/


/* string definitions shared with the Driver via Registry */
#define WP_REGSTR_USER_SPECIFIED_WANPIPE_NUMBER "UserWanpipeNumber"



#pragma warning( disable : 4200 )	/* zero-sized array in struct */
#endif/* __WINDOWS__ */

#define USED_BY_FIELD	128	/* max length of the used by field */


enum wan_codec_format{
	WP_NONE,
	WP_SLINEAR
};


/* Interface Operation Modes */
enum {
	WANPIPE,		/* TCP/IP over CHDLC, Frame Relay, PPP... */
	API,			/* Data OR Voice. Requires API Poll. */
	BRIDGE,
	BRIDGE_NODE,
	WP_SWITCH,
	STACK,
	ANNEXG,
	TTY,
	TDM_VOICE,		/* Asterisk/Zaptel support */
	TDM_VOICE_DCHAN, /* Depricated. */
	TDM_VOICE_API,	/* Linux: Sangoma Voice API support. Windows: Depricated. */
	TDM_API,
	WP_NETGRAPH,
	TRUNK,
	XMTP2_API,
	TDM_SPAN_VOICE_API,	/* Voice API support. Delivers Voice stream from a Span of channels. */
	TDM_CHAN_VOICE_API,	/* Voice API support. Delivers Voice stream from a single channel. */
	API_LEGACY,			/* Data OR Voice. Windows: Does NOT require API Poll. Linux: runs on Sangoma Socket interface. */
	MTP1_API
};


/* POS protocols */
enum {
	IBM4680,
	NCR2127,
	NCR2126,
	NCR1255,
	NCR7000,
	ICL
};

/*      Standard Mode                          */
enum {
        WANOPT_ADSL_T1_413      = 0,
        WANOPT_ADSL_G_LITE      = 1,
        WANOPT_ADSL_G_DMT       = 2,
        WANOPT_ADSL_ALCATEL_1_4 = 3,
        WANOPT_ADSL_MULTIMODE   = 4,
        WANOPT_ADSL_ADI         = 5,
        WANOPT_ADSL_ALCATEL     = 6,
        WANOPT_ADSL_T1_413_AUTO = 9
};

/*      Trellis  Modes                         */
#define WANOPT_ADSL_TRELLIS_DISABLE             0x0000
#define WANOPT_ADSL_TRELLIS_ENABLE              0x8000

#define WANOPT_ADSL_TRELLIS_LITE_ONLY_DISABLE   0xF000

#define WANOPT_ADSL_0DB_CODING_GAIN             0x0000
#define WANOPT_ADSL_1DB_CODING_GAIN             0x1000
#define WANOPT_ADSL_2DB_CODING_GAIN             0x2000
#define WANOPT_ADSL_3DB_CODING_GAIN             0x3000
#define WANOPT_ADSL_4DB_CODING_GAIN             0x4000
#define WANOPT_ADSL_5DB_CODING_GAIN             0x5000
#define WANOPT_ADSL_6DB_CODING_GAIN             0x6000
#define WANOPT_ADSL_7DB_CODING_GAIN             0x7000
#define WANOPT_ADSL_AUTO_CODING_GAIN            0xFF00

#define WANOPT_ADSL_RX_BIN_ENABLE               0x01
#define WANOPT_ADSL_RX_BIN_DISABLE              0x00


#define WANOPT_ADSL_FRAMING_TYPE_0              0x0000
#define WANOPT_ADSL_FRAMING_TYPE_1              0x0001
#define WANOPT_ADSL_FRAMING_TYPE_2              0x0002
#define WANOPT_ADSL_FRAMING_TYPE_3              0x0003

#define WANOPT_ADSL_EXPANDED_EXCHANGE           0x8000
#define WANOPT_ADSL_SHORT_EXCHANGE              0x0000

#define WANOPT_ADSL_CLOCK_OSCILLATOR            0x00
#define WANOPT_ADSL_CLOCK_CRYSTAL               0x04


#define SDLA_DECODE_CARDTYPE(card_type)				\
		(card_type == WANOPT_S50X) ?       "S50X" :		\
		(card_type == WANOPT_S51X) ?       "S51X" :		\
		(card_type == WANOPT_ADSL) ?       "ADSL" :		\
		(card_type == WANOPT_AFT)  ?	   "A101/2" :		\
		(card_type == WANOPT_AFT101 ||			\
		 card_type == WANOPT_AFT102 ||			\
		 card_type == WANOPT_AFT104 ||			\
		 card_type == WANOPT_AFT108 ||			\
		 card_type == WANOPT_AFT116) ?     "A101/1D/2/2D/4/4D/8/8D/16/16D" :	\
		(card_type == WANOPT_T116) ?     "T116"  :	\
		(card_type == WANOPT_AFT300) ?     "A300"  :	\
		(card_type == WANOPT_AFT_ANALOG) ? "A200/A400/B600/B700/B800/B610"  :	\
		(card_type == WANOPT_AFT_GSM) ?    "W400"  :	\
		(card_type == WANOPT_AFT_ISDN) ?   "A500/B700/B500"  :	\
		(card_type == WANOPT_AFT_56K) ?    "A056"  :	\
		(card_type == WANOPT_AFT_SERIAL) ? "A14x"  :	\
		(card_type == WANOPT_USB_ANALOG) ? "U100"  :	\
		(card_type == WANOPT_AFT600)   ?   "A600"  :	\
		(card_type == WANOPT_AFT610)   ?   "A610"  :	\
		(card_type == WANOPT_AFT601)   ?   "B601"  :	\
						   "Unknown"

#define COMPORT_DECODE(port)	(port == WANOPT_PRI) ? "PRI" : "SEC"

#define STATE_DECODE(state)					\
		((state == WAN_UNCONFIGURED) ? "Unconfigured" :	\
		 (state == WAN_DISCONNECTED) ? "Disconnected" : \
         (state == WAN_CONNECTING) ? "Connecting" : 	\
		 (state == WAN_CONNECTED) ? "Connected": 	\
		 (state == WAN_LIMIT) ? "Limit": 		\
		 (state == WAN_DUALPORT) ? "DualPort": 		\
		 (state == WAN_DISCONNECTING) ? "Disconnecting": \
		 (state == WAN_FT1_READY) ? "FT1 Ready": "Invalid")

#define CFG_DECODE(value)	(value) ? "YES" : "NO"

#define CLK_DECODE(clocking)	(clocking) ? "INT" : "EXT"

#define INT_DECODE(interface)					\
		(interface == WANOPT_RS232) ? "RS232" :	\
		(interface == WANOPT_V35)	? "V35" :	\
		"Invalid"

#define SIGNALLING_DECODE(sig)					\
		(sig == WANOPT_FR_ANSI) ? "ANSI" :	\
		(sig == WANOPT_FR_Q933) ? "Q333" :	\
		(sig == WANOPT_FR_LMI) ? "LMI" : "NO"

#define COMPORT_DECODE(port)	(port == WANOPT_PRI) ? "PRI" : "SEC"

#define IP_MODE_DECODE(ip_mode)					\
		(ip_mode == WANOPT_PPP_STATIC) ? "STATIC" :	\
		(ip_mode == WANOPT_PPP_PEER) ? "PEER" : "HOST"

#define X25_STATION_DECODE(station)				\
		(station == WANOPT_DTE) ? "DTE" : 		\
		(station == WANOPT_DCE) ? "DCE" : "DXE"

#define FR_STATION_DECODE(station)				\
		(station == WANOPT_CPE) ? "CPE" : "Node"

#define DEFAULT_TE_RX_SLEVEL 120

typedef char devname_t[WAN_DRVNAME_SZ+1];

/* Return:
 *	sub-media as string or
 * 'N/A' if sub-media does not exist or
 * 'InvalidSubMedia' if error. */
static __inline const char* wp_decode_submedia(int media, int sub_media)
{
	switch(media)
	{
	case WAN_MEDIA_FXOFXS:
		switch(sub_media)
		{
		case MOD_TYPE_FXS:
			return "FXS";
		case MOD_TYPE_FXO:
			return "FXO";
		default:
			return "InvalidSubMedia";
		}
		break;/* WAN_MEDIA_FXOFXS */

	case WAN_MEDIA_BRI:
		switch(sub_media)
		{
		case MOD_TYPE_NT:
			return "ISDN BRI NT";
		case MOD_TYPE_TE:
			return "ISDN BRI TE";
		default:
			return "InvalidSubMedia";
		}
		break;/* WAN_MEDIA_BRI */

	case WAN_MEDIA_SERIAL:
		return INT_DECODE(sub_media);

	default:
		return "N/A";/* this media does not have any sub-media (e.g. T1) */
	}
}

#define SDLA_DECODE_WAN_MEDIA(wan_media)	\
(wan_media == WAN_MEDIA_NONE)	? "MEDIA_NONE":	\
(wan_media == WAN_MEDIA_T1)		? "MEDIA_T1":	\
(wan_media == WAN_MEDIA_E1)		? "MEDIA_E1":	\
(wan_media == WAN_MEDIA_56K)	? "MEDIA_56K":	\
(wan_media == WAN_MEDIA_DS3)	? "MEDIA_DS3":	\
(wan_media == WAN_MEDIA_E3)		? "MEDIA_E3":	\
(wan_media == WAN_MEDIA_STS1)	? "MEDIA_STS1":	\
(wan_media == WAN_MEDIA_J1)		? "MEDIA_J1":	\
(wan_media == WAN_MEDIA_FXOFXS)	? "MEDIA_FXOFXS":	\
(wan_media == WAN_MEDIA_BRI)	? "MEDIA_BRI":	\
(wan_media == WAN_MEDIA_SERIAL)	? "MEDIA_SERIAL":	"Media Unknown"

typedef struct wan_atm_conf
{
	unsigned char	atm_sync_mode;
	unsigned short	atm_sync_data;
	unsigned char	atm_sync_offset;
	unsigned short  atm_hunt_timer;

	unsigned char	atm_cell_cfg;
	unsigned char	atm_cell_pt;
	unsigned char	atm_cell_clp;
	unsigned char	atm_cell_payload;
}wan_atm_conf_t;


typedef struct wan_bitstrm_conf{
	/* Bitstreaming options */
	unsigned short sync_options;
	unsigned short rx_sync_char;
	unsigned char monosync_tx_time_fill_char;
	unsigned int  max_length_tx_data_block;
	unsigned int  rx_complete_length;
	unsigned int  rx_complete_timer;

	unsigned int  rbs_map;

}wan_bitstrm_conf_t;


typedef struct wan_bitstrm_conf_if
{
	unsigned int	max_tx_queue_size;
	unsigned int	max_tx_up_size;
	unsigned char 	seven_bit_hdlc;
}wan_bitstrm_conf_if_t;

/*----------------------------------------------------------------------------
 * X.25-specific link-level configuration.
 */
typedef struct wan_x25_conf
{
	unsigned int lo_pvc;	/* lowest permanent circuit number */
	unsigned int hi_pvc;	/* highest permanent circuit number */
	unsigned int lo_svc;	/* lowest switched circuit number */
	unsigned int hi_svc;	/* highest switched circuit number */
	unsigned int hdlc_window; /* HDLC window size (1..7) */
	unsigned int pkt_window;  /* X.25 packet window size (1..7) */
	unsigned int t1;	  /* HDLC timer T1, sec (1..30) */
	unsigned int t2;	  /* HDLC timer T2, sec (0..29) */
	unsigned int t4;	/* HDLC supervisory frame timer = T4 * T1 */
	unsigned int n2;	/* HDLC retransmission limit (1..30) */
	unsigned int t10_t20;	/* X.25 RESTART timeout, sec (1..255) */
	unsigned int t11_t21;	/* X.25 CALL timeout, sec (1..255) */
	unsigned int t12_t22;	/* X.25 RESET timeout, sec (1..255) */
	unsigned int t13_t23;	/* X.25 CLEAR timeout, sec (1..255) */
	unsigned int t16_t26;	/* X.25 INTERRUPT timeout, sec (1..255) */
	unsigned int t28;	/* X.25 REGISTRATION timeout, sec (1..255) */
	unsigned int r10_r20;	/* RESTART retransmission limit (0..250) */
	unsigned int r12_r22;	/* RESET retransmission limit (0..250) */
	unsigned int r13_r23;	/* CLEAR retransmission limit (0..250) */
	unsigned int ccitt_compat;   /* compatibility mode: 1988/1984/1980 */
	unsigned int x25_conf_opt;   /* User defined x25 config optoins */
	unsigned char LAPB_hdlc_only; /* Run in HDLC only mode */
	unsigned char logging;        /* Control connection logging */
	unsigned char oob_on_modem; /* Whether to send modem
				       status to the user app */
	unsigned char local_station_address;  /*Local Station address */
	unsigned short defPktSize;
	unsigned short pktMTU;
	unsigned char cmd_retry_timeout; /* Value is seconds */
	unsigned char station;
} wan_x25_conf_t;

typedef struct wan_rtp_conf
{
   	unsigned int	rtp_ip;
	unsigned int	rtp_local_ip;
	unsigned short	rtp_port;
	unsigned short	rtp_sample;
	unsigned char	rtp_devname[WAN_IFNAME_SZ+1];
	unsigned char	rtp_mac[WAN_IFNAME_SZ+1];
	unsigned char	rtp_local_mac[WAN_IFNAME_SZ+1];
}wan_rtp_conf_t;

typedef struct wan_xilinx_conf
{
	unsigned short	dma_per_ch; 	/* DMA buffers per logic channel */
	unsigned short	mru;		/* MRU of transparent channels */
	unsigned int	rbs;		/* Robbit signalling support */
	unsigned int	data_mux_map;	/* Data mux map */
	unsigned int	rx_crc_bytes;
	unsigned char	span_tx_only_irq; /* SPAN mode API to use rx irq only */
	unsigned char   global_poll_irq;
	unsigned char	hw_port_map; 	/* A108 hw port map: Default or Linear */
	unsigned short	rx_fifo_trigger;
	unsigned short	tx_fifo_trigger;
} wan_xilinx_conf_t;

typedef struct wan_xilinx_conf_if
{
	unsigned char 	station;   	/* Node or CPE */
	unsigned int  	signalling; 	/* local in-channel signalling type */
	unsigned char 	seven_bit_hdlc;
	unsigned int  	mru;
	unsigned int  	mtu;
	unsigned char  	idle_flag;
	unsigned char  	data_mux;
	unsigned char	ss7_enable;
	unsigned char 	ss7_mode;
	unsigned char	ss7_lssu_size;
	unsigned char	tdmv_master_if;
	unsigned char   rbs_cas_idle;		/* Initial RBS/CAS value */
	unsigned char	hdlc_repeat;
	unsigned int	mtu_idle;
	unsigned char 	sw_hdlc;
	unsigned short  mtp1_filter;
}wan_xilinx_conf_if_t;

#if defined(CONFIG_PRODUCT_WANPIPE_USB)
typedef struct wan_usb_conf_if
{
	int	notused;

}wan_usb_conf_if_t;
#endif


typedef struct wan_ss7_conf
{
	unsigned int line_cfg_opt;
	unsigned int modem_cfg_opt;
	unsigned int modem_status_timer;
	unsigned int api_options;
	unsigned int protocol_options;
	unsigned int protocol_specification;
	unsigned int stats_history_options;
	unsigned int max_length_msu_sif;
	unsigned int max_unacked_tx_msus;
	unsigned int link_inactivity_timer;
	unsigned int t1_timer;
	unsigned int t2_timer;
	unsigned int t3_timer;
	unsigned int t4_timer_emergency;
	unsigned int t4_timer_normal;
	unsigned int t5_timer;
	unsigned int t6_timer;
	unsigned int t7_timer;
	unsigned int t8_timer;
	unsigned int n1;
	unsigned int n2;
	unsigned int tin;
	unsigned int tie;
	unsigned int suerm_error_threshold;
	unsigned int suerm_number_octets;
	unsigned int suerm_number_sus;
	unsigned int sie_interval_timer;
	unsigned int sio_interval_timer;
	unsigned int sios_interval_timer;
	unsigned int fisu_interval_timer;
} wan_ss7_conf_t;


#define WANOPT_TWO_WAY_ALTERNATE 	0
#define WANOPT_TWO_WAY_SIMULTANEOUS	1

#define WANOPT_PRI_DISC_ON_NO_RESP	0
#define WANOPT_PRI_SNRM_ON_NO_RESP	1

typedef struct wan_xdlc_conf {

	unsigned char  station;
	unsigned int   address;
	unsigned int   max_I_field_length;

	unsigned int protocol_config;
	unsigned int error_response_config;
	unsigned int TWS_max_ack_count;

	unsigned int pri_slow_poll_timer;
	unsigned int pri_normal_poll_timer;
	unsigned int pri_frame_response_timer;
	unsigned int sec_nrm_timer;

	unsigned int max_no_response_cnt;
	unsigned int max_frame_retransmit_cnt;
	unsigned int max_rnr_cnt;

	unsigned int window;

}wan_xdlc_conf_t;


typedef struct wan_sdlc_conf {

	unsigned char  station_configuration;
	unsigned int   baud_rate;
	unsigned short max_I_field_length;
	unsigned short general_operational_config_bits;
	unsigned short protocol_config_bits;
	unsigned short exception_condition_reporting_config;
	unsigned short modem_config_bits;
	unsigned short statistics_format;
	unsigned short pri_station_slow_poll_interval;
	unsigned short permitted_sec_station_response_TO;
	unsigned short no_consec_sec_TOs_in_NRM_before_SNRM_issued;
	unsigned short max_lgth_I_fld_pri_XID_frame;
	unsigned short opening_flag_bit_delay_count;
	unsigned short RTS_bit_delay_count;
	unsigned short CTS_timeout_1000ths_sec;
	unsigned char  SDLA_configuration;

}wan_sdlc_conf_t;


typedef struct wan_bscstrm_conf {
   unsigned int baud_rate;                 /* the baud rate */
   unsigned int adapter_frequency;         /* the adapter frequecy */
   unsigned short max_data_length;          /* the maximum length of a BSC data block */
   unsigned short EBCDIC_encoding;          /* EBCDIC/ASCII encoding */
   unsigned short Rx_block_type;            /* the type of BSC block to be received */
   unsigned short no_consec_PADs_EOB;       /* the number of consecutive PADs indicating the end of the block */
   unsigned short no_add_lead_Tx_SYN_chars; /* the number of additional leading transmit SYN characters */
   unsigned short no_bits_per_char;         /* the number of bits per character */
   unsigned short parity;                   /* parity */
   unsigned short misc_config_options;      /* miscellaneous configuration options */
   unsigned short statistics_options;       /* statistic options */
   unsigned short modem_config_options;     /* modem configuration options */
}wan_bscstrm_conf_t;


/*----------------------------------------------------------------------------
 * PPP-specific link-level configuration.
 */
typedef struct wan_ppp_conf
{
	unsigned restart_tmr;	/* restart timer */
	unsigned auth_rsrt_tmr;	/* authentication timer */
	unsigned auth_wait_tmr;	/* authentication timer */
	unsigned mdm_fail_tmr;	/* modem failure timer */
	unsigned dtr_drop_tmr;	/* DTR drop timer */
	unsigned connect_tmout;	/* connection timeout */
	unsigned conf_retry;	/* max. retry */
	unsigned term_retry;	/* max. retry */
	unsigned fail_retry;	/* max. retry */
	unsigned auth_retry;	/* max. retry */
	unsigned auth_options;	/* authentication opt. */
	unsigned ip_options;	/* IP options */
	char	authenticator;	/* AUTHENTICATOR or not */
	char	ip_mode;	/* Static/Host/Peer */
} wan_ppp_conf_t;

/*----------------------------------------------------------------------------
 * CHDLC-specific link-level configuration.
 */
typedef struct wan_chdlc_conf
{
	unsigned char ignore_dcd;	/* Protocol options:		*/
	unsigned char ignore_cts;	/* Ignore these to determine	*/
	unsigned char ignore_keepalive;	/* link status (Yes or No)	*/
	unsigned char hdlc_streaming;	/* hdlc_streaming mode (Y/N) */
	unsigned char receive_only;	/* no transmit buffering (Y/N) */
	unsigned keepalive_tx_tmr;	/* transmit keepalive timer */
	unsigned keepalive_rx_tmr;	/* receive  keepalive timer */
	unsigned keepalive_err_margin;	/* keepalive_error_tolerance */
	unsigned slarp_timer;		/* SLARP request timer */
	unsigned char fast_isr;		/* Fast interrupt option */
	unsigned int protocol_options;
} wan_chdlc_conf_t;



#define X25_CALL_STR_SZ 512
#define WAN_IF_LABEL_SZ 15

typedef struct lapb_parms_struct {
	unsigned int t1;
	unsigned int t1timer;
	unsigned int t2;
	unsigned int t2timer;
	unsigned int n2;
	unsigned int n2count;
	unsigned int t3;
	unsigned int t4;
	unsigned int window;
	unsigned int state;
	unsigned int mode;
	unsigned int mtu;
	unsigned int station;
	unsigned char label[WAN_IF_LABEL_SZ+1];
	unsigned char virtual_addr[WAN_ADDRESS_SZ+1];
	unsigned char real_addr[WAN_ADDRESS_SZ+1];
}wan_lapb_if_conf_t;



typedef struct x25_parms_struct {

	unsigned short X25_API_options;
	unsigned short X25_protocol_options;
	unsigned short X25_response_options;
	unsigned short X25_statistics_options;
	unsigned short packet_window;
	unsigned short default_packet_size;
	unsigned short maximum_packet_size;
	unsigned short lowest_PVC;
	unsigned short highest_PVC;
	unsigned short lowest_incoming_channel;
	unsigned short highest_incoming_channel;
	unsigned short lowest_two_way_channel;
	unsigned short highest_two_way_channel;
	unsigned short lowest_outgoing_channel;
	unsigned short highest_outgoing_channel;
	unsigned short genl_facilities_supported_1;
	unsigned short genl_facilities_supported_2;
	unsigned short CCITT_facilities_supported;
	unsigned short non_X25_facilities_supported;
	unsigned short CCITT_compatibility;
	unsigned int T10_T20;
	unsigned int T11_T21;
	unsigned int T12_T22;
	unsigned int T13_T23;
	unsigned int T16_T26;
	unsigned int T28;
	unsigned short R10_R20;
	unsigned short R12_R22;
	unsigned short R13_R23;
	unsigned char  dte;
	unsigned char  mode;

	unsigned char call_string   [X25_CALL_STR_SZ+1];

	/* Accept Call Information */
	unsigned char accept_called [WAN_ADDRESS_SZ+1];
	unsigned char accept_calling[WAN_ADDRESS_SZ+1];
	unsigned char accept_facil  [WAN_ADDRESS_SZ+1];
	unsigned char accept_udata  [WAN_ADDRESS_SZ+1];

	unsigned char label [WAN_IF_LABEL_SZ+1];

	unsigned int  call_backoff_timeout;
	unsigned char call_logging;

	/* X25_SW X.25 switching */
	unsigned char virtual_addr[WAN_ADDRESS_SZ+1];
	unsigned char real_addr[WAN_ADDRESS_SZ+1];

	unsigned char addr[WAN_ADDRESS_SZ+1];	/* PVC LCN number */

} wan_x25_if_conf_t;

typedef struct dsp_parms {

	unsigned char	pad_type;	/* PAD type: HOST or TERM */
	unsigned int T1;			/* Service Timeout perioud */
	unsigned int T2;			/* PAD protocol timeout period */
	unsigned int T3;			/* Timeout between two packets
					 * of the same M_DATA */
	unsigned char auto_ce;		/* Automaticaly send Circuit Enabled
					 * with ACCEPT CALL packet */
	unsigned char auto_call_req;	/* Automaticaly re-send CALL REQUEST
					 * if it was failed before (T1) */
	unsigned char auto_ack;		/* Automaticaly send ACK for data */

	unsigned short dsp_mtu;		/* MTU size for DSP level */

}wan_dsp_if_conf_t;


/*----------------------------------------------------------------------------
 * TDMV configuration structures.
 */

typedef struct wan_tdmv_conf_ {

	unsigned char  span_no;
	unsigned int   dchan;		/* hwHDLC: PRI SIG */
	unsigned char  hw_dtmf;		/* TDMV Enable/Disable HW DTMF */
	unsigned char  hw_fax_detect;	/* TDMV Enable/Disable HW FAX Calling */
	unsigned char  hw_faxcalled;	/* TDMV Enable/Disable HW FAX Called */
	unsigned char  sdla_tdmv_dummy_enable;
	unsigned char  ec_off_on_fax;	/* Disable hwec on fax event */
} wan_tdmv_conf_t;

typedef struct wan_hwec_conf_
{
	unsigned int	clk_src;		/* Octasic Clock Source Port */
	unsigned int	persist_disable;	/* HW EC Persist */
	unsigned char	noise_reduction;			/* Noise Reduction control */
	unsigned char	noise_reduction_disable;	/* Noise Reduction control - because now its on by default */
	unsigned int	tone_disabler_delay;	/* delay in a fax mode */
	unsigned char	dtmf_removal;		/* remove dtmf */
	unsigned char 	operation_mode;
	unsigned char 	acustic_echo;
	unsigned char 	nlp_disable;
	int 			rx_auto_gain;
	int 			tx_auto_gain;
	int				rx_gain;
	int				tx_gain;
	unsigned char	hw_fax_detect_cnt;	
} wan_hwec_conf_t;

typedef struct wan_hwec_dev_state
{
	u_int32_t	ec_state;
    u_int32_t	ec_mode_map;
	u_int32_t	dtmf_map;
	u_int32_t	fax_called_map;
	u_int32_t	fax_calling_map;
}wan_hwec_dev_state_t;

#define MAX_PARAM_LEN	50
#define MAX_VALUE_LEN	50
typedef struct wan_custom_param_
{
	char		name[MAX_PARAM_LEN+1];
	char		sValue[MAX_VALUE_LEN+1];
	unsigned int	dValue;
} wan_custom_param_t;

typedef struct wan_custom_conf_
{
	unsigned int		param_no;
	wan_custom_param_t	*WP_POINTER_64 params;
} wan_custom_conf_t;


/*----------------------------------------------------------------------------
 * WAN device configuration. Passed to ROUTER_SETUP IOCTL.
 */
typedef struct wandev_conf
{
	unsigned magic;		/* magic number (for verification) */
	unsigned long config_id;	/* configuration structure identifier */
				/****** hardware configuration ******/
	unsigned ioport;	/* adapter I/O port base */
	unsigned long maddr;	/* dual-port memory address */
	unsigned msize;		/* dual-port memory size */
	int irq;		/* interrupt request level */
	int dma;		/* DMA request level */
	char S514_CPU_no[1];	/* S514 PCI adapter CPU number ('A' or 'B') */
	unsigned PCI_slot_no;	/* S514 PCI adapter slot number */
	char auto_hw_detect;	/* S515 PCI automatic slot detection */
	char	usb_busid[30];	/* USB bus id string (FIXME: length is set to 30) (for USB devices) */
	unsigned int comm_port;	/* Communication Port (PRI=0, SEC=1) */
	unsigned int bps;	/* data transfer rate */
	unsigned int mtu;	/* maximum transmit unit size */
    unsigned udp_port;      /* UDP port for management */
	unsigned char ttl;	/* Time To Live for UDP security */
	unsigned char ft1;	/* FT1 Configurator Option */
    char electrical_interface;		/* RS-232/V.35, etc. */
	char clocking;		/* external/internal */
	char line_coding;	/* NRZ/NRZI/FM0/FM1, etc. */
	char connection;	/* permanent/switched/on-demand */
	char read_mode;		/* read mode: Polling or interrupt */
	char receive_only;	/* disable tx buffers */
	char tty;		/* Create a fake tty device */
	unsigned tty_major;	/* Major number for wanpipe tty device */
	unsigned tty_minor; 	/* Minor number for wanpipe tty device */
	unsigned tty_mode;	/* TTY operation mode SYNC or ASYNC */
	char backup;		/* Backup Mode */
	unsigned hw_opt[4];	/* other hardware options */
	unsigned reserved[4];

				/****** arbitrary data ***************/
	unsigned data_size;	/* data buffer size */

	void *WP_POINTER_64 data;		/* data buffer, e.g. firmware */

	union{			/****** protocol-specific ************/
		wan_x25_conf_t 		x25;	/* X.25 configuration */
		wan_ppp_conf_t 		ppp;	/* PPP configuration */
		wan_fr_conf_t 		fr;	/* frame relay configuration */
		wan_chdlc_conf_t	chdlc;	/* Cisco HDLC configuration */
		wan_ss7_conf_t 		ss7;	/* SS7 Configuration */
		wan_sdlc_conf_t 	sdlc;	/* SDLC COnfiguration */
		wan_bscstrm_conf_t 	bscstrm; /* Bisync Streaming Configuration */
		wan_adsl_conf_t 	adsl;	/* ADSL Configuration */
		wan_atm_conf_t 		atm;
		wan_xilinx_conf_t 	aft;
		wan_bitstrm_conf_t	bitstrm;
		wan_xdlc_conf_t 	xdlc;
	} u;

	/* No new variables are allowed above */
	unsigned int card_type;	/* Supported Sangoma Card type */
	unsigned int pci_bus_no;	/* S514 PCI bus number */

	sdla_fe_cfg_t	fe_cfg;	/* Front end configurations */

	wan_tdmv_conf_t		tdmv_conf;
	wan_hwec_conf_t		hwec_conf;
	wan_rtp_conf_t  	rtp_conf;
	wan_custom_conf_t	oct_conf;

	unsigned char line_idle; /* IDLE FLAG/ IDLE MARK */
	unsigned char ignore_front_end_status;
	unsigned int  max_trace_queue;
	unsigned int  max_rx_queue;

#if 0
	/* Bitstreaming options */
	unsigned int  sync_options;
	unsigned char rx_sync_char;
	unsigned char monosync_tx_time_fill_char;
	unsigned int  max_length_tx_data_block;
	unsigned int  rx_complete_length;
	unsigned int  rx_complete_timer;

	unsigned int  max_trace_queue;
	unsigned int  max_rx_queue;
#endif

} wandev_conf_t;

#if defined(__WINDOWS__)
#define WANCONFIG_AFT_FIRMWARE_UPDATE	89
#endif

/* 'config_id' definitions */
#define	WANCONFIG_X25		101	/* X.25 link */
#define	WANCONFIG_FR		102	/* frame relay link */
#define	WANCONFIG_PPP		103	/* synchronous PPP link */
#define WANCONFIG_CHDLC		104	/* Cisco HDLC Link */
#define WANCONFIG_BSC		105	/* BiSync Streaming */
#define WANCONFIG_HDLC		106	/* HDLC Support */
#define WANCONFIG_MPPP  	107	/* Multi Port PPP over RAW CHDLC */
#define WANCONFIG_MPROT 	WANCONFIG_MPPP	/* Multi Port Driver HDLC,PPP,CHDLC over RAW CHDLC */
#define WANCONFIG_BITSTRM 	108	/* Bit-Stream protocol */
#define WANCONFIG_EDUKIT 	109	/* Educational Kit support */
#define WANCONFIG_SS7	 	110	/* SS7 Support */
#define WANCONFIG_BSCSTRM 	111	/* Bisync Streaming Nasdaq */
#define WANCONFIG_MFR    	112	/* Mulit-Port Frame Relay over RAW HDLC */
#define WANCONFIG_ADSL	 	113	/* LLC Ethernet Support */
#define WANCONFIG_SDLC	 	114	/* SDLC Support */
#define WANCONFIG_ATM	 	115	/* ATM Supprot */
#define WANCONFIG_POS	 	116	/* POS Support */
#define WANCONFIG_AFT    	117	/* AFT Original Hardware Support */
#define WANCONFIG_DEBUG  	118	/* Real Time Debugging */
#define WANCONFIG_ADCCP	 	119	/* Special HDLC LAPB Driver */
#define WANCONFIG_MLINK_PPP 	120	/* Multi-Link PPP */
#define WANCONFIG_GENERIC   	121	/* WANPIPE Generic driver */
#define WANCONFIG_AFT_TE3   	122 	/* AFT TE3 Hardware Support */
#define WANCONFIG_MPCHDLC   	123 	/* Multi Port CHDLC */
#define WANCONFIG_AFT_TE1_SS7   124 	/* AFT TE1 SS7 Hardware Support */
#define WANCONFIG_LAPB		125	/* LIP LAPB Protocol Support */
#define WANCONFIG_XDLC		126	/* LIP XDLC Protocol Support */
#define WANCONFIG_TTY		127	/* LIP TTY Support */
#define WANCONFIG_AFT_TE1    	128	/* AFT A1/2/4/8 Hardware Support */
#define WANCONFIG_XMTP2    	129	/* LIP XMTP2 Protocol Support */
#define WANCONFIG_ASYHDLC	130	/* S514 ASY HDLC API Support */
#define WANCONFIG_LIP_ATM	131	/* ATM in LIP layer */
#define WANCONFIG_AFT_ANALOG	132	/* AFT Analog Driver */
#define WANCONFIG_ZAP		133	/* Used in wanpipemon when working with Zaptel driver */
#define WANCONFIG_LAPD    	134	/* LIP LAPD Q921 Protocol Support */
#define WANCONFIG_LIP_KATM	135	/* Kernel ATM Stack Support */
#define WANCONFIG_AFT_56K  	136	/* AFT 56K Support */
#define WANCONFIG_AFT_ISDN_BRI	137	/* AFT ISDN BRI Driver */
#define WANCONFIG_AFT_SERIAL	138	/* AFT Serial V32/RS232 Driver */
#define WANCONFIG_LIP_HDLC	139	/* LIP HDLC protocol */
#define WANCONFIG_USB_ANALOG	140	/* Wanpipe USB Driver */
#define WANCONFIG_AFT_GSM       141     /* Wanpipe GSM Driver */
#define WANCONFIG_AFT_T116      142     /* Wanpipe T116 Driver */

/*FIXME: This should be taken out, I just
//used it so I don't break the apps that are
//using the WANCONFIG_ETH. Once those apps are
//changed, remove this definition
*/
#define WANCONFIG_ETH WANCONFIG_ADSL

#define SDLA_DECODE_PROTOCOL(protocol)			\
	(protocol ==  WANCONFIG_X25) 	? "X25" :	\
	(protocol ==  WANCONFIG_FR)	? "Frame Relay" : \
	(protocol ==  WANCONFIG_PPP)	? "PPP" : \
	(protocol ==  WANCONFIG_CHDLC)	? "CHDLC" : \
	(protocol ==  WANCONFIG_BSC)	? "BiSync Streaming": \
	(protocol ==  WANCONFIG_HDLC)	? "HDLC Streaming": \
	(protocol ==  WANCONFIG_MPPP)	? "PPP": \
	(protocol ==  WANCONFIG_BITSTRM) ? "Bit Stream": \
	(protocol ==  WANCONFIG_EDUKIT)	? "WAN EduKit": \
	(protocol ==  WANCONFIG_SS7)	? "SS7": \
	(protocol ==  WANCONFIG_BSCSTRM) ? "Bisync Streaming Nasdaq": \
	(protocol ==  WANCONFIG_MFR)	? "Frame Relay": \
	(protocol ==  WANCONFIG_ADSL)	? "LLC Ethernet (ADSL)": \
	(protocol ==  WANCONFIG_SDLC)	? "SDLC": \
	(protocol ==  WANCONFIG_ATM)	? "ATM": \
	(protocol ==  WANCONFIG_LIP_ATM)? "LIP_ATM": \
	(protocol ==  WANCONFIG_LIP_KATM)? "LIP_KATM": \
	(protocol ==  WANCONFIG_LAPB)? "LIP_LAPB": \
	(protocol ==  WANCONFIG_LAPD)? "LIP_LAPD": \
	(protocol ==  WANCONFIG_POS)	? "Point-of-Sale": \
	(protocol ==  WANCONFIG_AFT)	? "AFT": \
	(protocol ==  WANCONFIG_AFT_TE3) ? "AFT TE3": \
	(protocol ==  WANCONFIG_DEBUG)	 ? "Real Time Debugging": \
	(protocol ==  WANCONFIG_ADCCP)	 ? "Special HDLC LAPB": \
	(protocol ==  WANCONFIG_MLINK_PPP) ? "Multi-Link PPP": \
	(protocol ==  WANCONFIG_GENERIC)   ? "WANPIPE Generic driver": \
	(protocol ==  WANCONFIG_MPCHDLC)   ? "CHDLC": \
	(protocol ==  WANCONFIG_ZAP)   	   ? "ZAP": \
	(protocol ==  WANCONFIG_AFT_56K)   ? "AFT 56K": \
	(protocol ==  WANCONFIG_AFT_ISDN_BRI) ? "ISDN BRI": \
	(protocol ==  WANCONFIG_AFT_SERIAL) ? "Serial V35/RS232": \
	(protocol ==  WANCONFIG_AFT_ANALOG) ? "Analog FXO/FXS": \
	(protocol ==  WANCONFIG_AFT_TE1)    ? "AFT A1/2/4/8" : \
	(protocol ==  WANCONFIG_LIP_HDLC)   ? "LIP HDLC" : \
	(protocol ==  WANCONFIG_AFT_SERIAL)  ? "AFT Serial" : \
	(protocol ==  WANCONFIG_AFT_ISDN_BRI)  ? "AFT BRI" : \
	(protocol ==  WANCONFIG_TTY)	    ? "TTY" : \
	(protocol ==  WANCONFIG_USB_ANALOG)  ? "USB Analog FXO/FXS" : \
	(protocol ==  WANCONFIG_AFT_GSM)  ? "AFT GSM " : \
						"Unknown Protocol"


typedef struct wan_tdmv_if_conf
{
	unsigned char	tdmv_echo_off;  /* TDMV echo disable */
	unsigned char	tdmv_codec;     /* TDMV codec */

} wan_tdmv_if_conf_t;

typedef struct wan_hwec_if_conf
{
	unsigned char	enable;	/* Enable/Disable HW EC */

} wan_hwec_if_conf_t;


/*----------------------------------------------------------------------------
 * WAN interface (logical channel) configuration (for ROUTER_IFNEW IOCTL).
 */
typedef struct wanif_conf
{
	unsigned 	magic;			/* magic number */
	unsigned long 	config_id;		/* configuration identifier */
	char 		name[WAN_IFNAME_SZ+1];	/* interface name, ASCIIZ */
	char 		addr[WAN_ADDRESS_SZ+1];	/* media address, ASCIIZ */
	char 		usedby[USED_BY_FIELD+1];/* used by API or WANPIPE */
	unsigned 	idle_timeout;		/* sec, before disconnecting */
	unsigned 	hold_timeout;		/* sec, before re-connecting */
	unsigned 	cir;			/* Committed Information Rate fwd,bwd*/
	unsigned 	bc;			/* Committed Burst Size fwd, bwd */
	unsigned 	be;			/* Excess Burst Size fwd, bwd */
#ifdef ENABLE_IPV6
	unsigned char 	enable_IPv4;	/* Enable or Disable IPv4 */
	unsigned char 	enable_IPv6;	/* Enable or Disable IPv6 */
#endif
	unsigned char 	enable_IPX;	/* Enable or Disable IPX */
	unsigned char 	inarp;		/* Send Inverse ARP requests Y/N */
	unsigned 	inarp_interval;	/* sec, between InARP requests */
	unsigned int 	network_number;	/* Network Number for IPX */
	char 		mc;			/* Multicast on or off */
	char 		local_addr[WAN_ADDRESS_SZ+1];/* local media address, ASCIIZ */
        unsigned char 	port;             /* board port */
        unsigned char 	protocol;         /* prococol used in this channel (TCPOX25 or X25) */
	char 		pap;			/* PAP enabled or disabled */
	char 		chap;			/* CHAP enabled or disabled */
#ifdef ENABLE_IPV6
	unsigned char 	chap_userid[WAN_AUTHNAMELEN];	/* List of User Id */
	unsigned char 	chap_passwd[WAN_AUTHNAMELEN];	/* List of passwords */
	unsigned char 	pap_userid[WAN_AUTHNAMELEN];	/* List of User Id */
	unsigned char 	pap_passwd[WAN_AUTHNAMELEN];	/* List of passwords */
#else
	unsigned char 	userid[WAN_AUTHNAMELEN];	/* List of User Id */
	unsigned char 	passwd[WAN_AUTHNAMELEN];	/* List of passwords */
#endif
	unsigned char 	sysname[SYSTEM_NAME_LEN];		/* Name of the system */
	unsigned char 	ignore_dcd;		/* Protocol options: */
	unsigned char 	ignore_cts;	 	/*  Ignore these to determine */
	unsigned char 	ignore_keepalive; 	/*  link status (Yes or No) */
	unsigned char 	hdlc_streaming;	  	/*  Hdlc streaming mode (Y/N) */
	unsigned 	keepalive_tx_tmr;	/* transmit keepalive timer */
	unsigned 	keepalive_rx_tmr;	/* receive  keepalive timer */
	unsigned 	keepalive_err_margin;	/* keepalive_error_tolerance */
	unsigned 	slarp_timer;		/* SLARP request timer */
	unsigned char 	ttl;			/* Time To Live for UDP security */
	/* 'interface' is reserved for C++ compiler!! */
	char 		electrical_interface;		/* RS-232/V.35, etc. */
	char 		clocking;		/* external/internal */
	unsigned 	bps;			/* data transfer rate */
	unsigned 	mtu;			/* maximum transmit unit size */
	unsigned char 	if_down;	/* brind down interface when disconnected */
	unsigned char 	gateway;	/* Is this interface a gateway */
	unsigned char 	true_if_encoding; /* Set the dev->type to true board protocol */

	unsigned char 	asy_data_trans;     /* async API options */
    unsigned char 	rts_hs_for_receive; /* async Protocol options */
    unsigned char 	xon_xoff_hs_for_receive;
	unsigned char 	xon_xoff_hs_for_transmit;
	unsigned char 	dcd_hs_for_transmit;
	unsigned char 	cts_hs_for_transmit;
	unsigned char 	async_mode;
	unsigned 	tx_bits_per_char;
	unsigned 	rx_bits_per_char;
	unsigned 	stop_bits;
	unsigned char 	parity;
 	unsigned 	break_timer;
    unsigned 	inter_char_timer;
	unsigned 	rx_complete_length;
	unsigned 	xon_char;
	unsigned 	xoff_char;
	unsigned char 	receive_only;	/*  no transmit buffering (Y/N) */

	/* x25 media source address, ASCIIZ */
	unsigned char x25_src_addr[WAN_ADDRESS_SZ+1];
	/* pattern match string in -d<string>
	 * for accepting calls, ASCIIZ */
	unsigned char accept_dest_addr[WAN_ADDRESS_SZ+1];
	/* pattern match string in -s<string>
	 * for accepting calls, ASCIIZ */
	unsigned char accept_src_addr[WAN_ADDRESS_SZ+1];
	/* pattern match string in -u<string>
	 * for accepting calls, ASCIIZ */
	unsigned char accept_usr_data[WAN_ADDRESS_SZ+1];

	unsigned char inarp_rx;		/* Receive Inverse ARP requests Y/N */
	unsigned int  active_ch;
	unsigned int  cfg_active_ch;
	unsigned int  max_trace_queue;
	unsigned char sw_dev_name[WAN_IFNAME_SZ+1];
	unsigned char auto_cfg;

	unsigned char call_logging;
	unsigned char master[WAN_IFNAME_SZ+1];
	unsigned char station;
	unsigned char label[WAN_IF_LABEL_SZ+1];

	wan_tdmv_if_conf_t	tdmv;
	wan_hwec_if_conf_t	hwec;
	wan_custom_conf_t	ec_conf;

	/*unsigned char tdmv_echo_off;	*/	/* TDMV echo disable */
	/*unsigned char tdmv_codec;	*/	/* TDMV codec */
	unsigned char single_tx_buf; 	/* Used in low latency applications */

	unsigned char lip_prot;

	unsigned char line_mode[USED_BY_FIELD];
	unsigned char interface_number;/* from 0 to 31 */

	union {
		wan_atm_conf_if_t 	atm;  /* per interface configuration */
		wan_x25_if_conf_t 	x25;
		wan_lapb_if_conf_t 	lapb;
		wan_dsp_if_conf_t 	dsp;
		wan_fr_conf_t 		fr;
		wan_bitstrm_conf_if_t 	bitstrm;
		wan_xilinx_conf_if_t 	aft;
		wan_xdlc_conf_t 	xdlc;
		/* In LIP layer the same config structure 'wan_sppp_if_conf_t' used
		 * for both PPP and CHDLC. */
		wan_sppp_if_conf_t	ppp;
		wan_chdlc_conf_t	chdlc;
		wan_lip_hdlc_if_conf_t	lip_hdlc;
	}u;

} wanif_conf_t;


typedef struct {
	unsigned short fr_active;
	unsigned short fr_inactive;
	unsigned short lapb_active;
	unsigned short lapb_inactive;
	unsigned short x25_link_active;
	unsigned short x25_link_inactive;
	unsigned short x25_active;
	unsigned short x25_inactive;
	unsigned short dsp_active;
	unsigned short dsp_inactive;
}wp_stack_stats_t;

/* ALEX_DEBUG*/
typedef struct wan_debug {
	unsigned long 	magic;	/* for verification */
	unsigned long	len;
	unsigned long	num;
	unsigned long	max_len;
	unsigned long	offs;
	int		is_more;
	char*		data;
} wan_kernel_msg_t;

typedef struct wanpipe_debug_msg_hdr_t {
	int len;
	wan_time_t time;
} wanpipe_kernel_msg_hdr_t;


#define TRC_INCOMING_FRM              0x00
#define TRC_OUTGOING_FRM              0x01
#pragma pack(1)
typedef struct {
	unsigned char	status;
	unsigned char	data_avail;
	unsigned short	real_length;
	unsigned short	time_stamp;
	unsigned char	channel;
	wan_time_t	sec;		/* unsigned long	sec; */
	wan_suseconds_t	usec;	/* unsigned long   usec; */
	unsigned char	data[0];
} wan_trace_pkt_t;
#pragma pack()


#define MODE_OPTION_HDLC		"HDLC"
#define MODE_OPTION_BITSTRM		"BitStream"
#define ISDN_BRI_DCHAN			"BRI D-chan"

/* Windows note: this macro is used as input to strcmp() ! */
#define SDLA_DECODE_USEDBY_FIELD(usedby)			\
	(usedby == WANPIPE)			? "WANPIPE"	:		\
	(usedby == API)				? "API"		:		\
	(usedby == BRIDGE)			? "BRIDGE"	:		\
	(usedby == BRIDGE_NODE)		? "BRIDGE_NODE" :	\
	(usedby == WP_SWITCH)			? "SWITCH"	:		\
	(usedby == STACK)			? "STACK"	:		\
	(usedby == TDM_VOICE_API)	? "TDM_VOICE_API" :	\
	(usedby == TDM_SPAN_VOICE_API)	? "TDM_SPAN_VOICE_API" :	\
	(usedby == TDM_CHAN_VOICE_API)	? "TDM_CHAN_VOICE_API" :	\
	(usedby == TDM_VOICE_DCHAN)	? "TDM_VOICE_DCHAN" :	\
	(usedby == API_LEGACY)	? "API_LEGACY" :	\
	(usedby == ANNEXG)		? "ANNEXG" :	"Unknown"

#define CARD_WANOPT_DECODE(cardtype)				\
	((cardtype == WANOPT_S50X) ? "WANOPT_S50X" :	\
	(cardtype == WANOPT_S51X) ? "WANOPT_S51X" :	\
	(cardtype == WANOPT_ADSL) ? "WANOPT_ADSL" : 	\
	(cardtype == WANOPT_AFT)  ? "WANOPT_AFT": 	\
	(cardtype == WANOPT_AFT102) ? "WANOPT_AFT102": 		\
	(cardtype == WANOPT_AFT104) ? "WANOPT_AFT104": 		\
	(cardtype == WANOPT_AFT108) ? "WANOPT_AFT108": 		\
	(cardtype == WANOPT_T116) ? "WANOPT_T116": 		\
	(cardtype == WANOPT_AFT_ANALOG) ? "WANOPT_AFT_ANALOG": 		\
	(cardtype == WANOPT_AFT_GSM) ? "WANOPT_AFT_GSM": 		\
	(cardtype == WANOPT_AFT_56K) ? "WANOPT_AFT_56K": 		\
	(cardtype == WANOPT_AFT300) ? "WANOPT_AFT300":	\
	(cardtype == WANOPT_AFT600) ? "WANOPT_AFT600":  \
	(cardtype == WANOPT_AFT610) ? "WANOPT_AFT610":  \
	(cardtype == WANOPT_AFT601) ? "WANOPT_AFT601":  \
	(cardtype == WANOPT_AFT_ISDN) ? "WANOPT_AFT_ISDN": "Invalid card")



typedef struct _buffer_settings{
	/* Number of received blocks of data before Receive event is indicated to API caller. */
	unsigned short buffer_multiplier_factor;

	/* Number of buffers for receiving or transmitting data on an API interface. */
	/* EACH buffer will contain 'buffer_multiplier_factor' blocks of data before
	   Receive event is indicated to API caller. */
	unsigned short number_of_buffers_per_api_interface;

}buffer_settings_t;

#define MIN_BUFFER_MULTIPLIER_FACTOR 1
#define MAX_BUFFER_MULTIPLIER_FACTOR 7

#define MIN_NUMBER_OF_BUFFERS_PER_API_INTERFACE	50
#define MAX_NUMBER_OF_BUFFERS_PER_API_INTERFACE	100

/* Structure used with START_PORT_VOLATILE_CONFIG and START_PORT_REGISTRY_CONFIG
   commands. The commands are defined in sang_api.h. */
typedef struct {
	unsigned int	command_code;		/* Configuration Command Code */
	unsigned int	operation_status;	/* operation completion status*/
	unsigned short	port_no;			/* Port No */

	/* Port configuration such T1 or E1... */
	wandev_conf_t wandev_conf;

	/* Number of interfaces in if_cfg array */
	unsigned int	num_of_ifs;

	/* Per-Interface configuration.
	   For AFT card maximum NUM_OF_E1_CHANNELS (32) logic channels.
	   Configuration of each logic channel ('active_ch', HDLC or Transparent...) */
	wanif_conf_t if_cfg[NUM_OF_E1_CHANNELS];

	buffer_settings_t buffer_settings;

}wanpipe_port_cfg_t;

#define port_cfg_t wanpipe_port_cfg_t

/* normal driver operation */
#define DRV_MODE_NORMAL	0
/* driver will run in 'AFT firmware update' mode */
#define DRV_MODE_AFTUP	1

/* cpecial id - when driver installed, it is "not configured".
user should select a protocol.*/
#define WANCONFIG_NONE			10
/* special id - for AFT firmware update */
/*#define WANCONFIG_AFT_FIRMWARE_UPDATE	11*/

#if defined(__WINDOWS__)
/* restore warning behavior to its default value*/
# pragma warning( default : 4200 )	/* zero-sized array in struct */
#endif

#endif
