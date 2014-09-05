/***************************************************************************
                   wanconfig.h  -  Definitions and structures for
					parsing Sangoma Configuration ('.conf') files.
                        -------------------
    begin                : Thu Dec 17 2009
    copyright            : (C) 2009 by David Rokhvarg
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

#ifndef WANCONFIG_H
#define WANCONFIG_H

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

#if defined(__LINUX__)
# include <linux/wanpipe_defines.h>
# include <linux/sdla_fr.h>
# include <linux/wanpipe_cfg.h>
# include <linux/sdla_te1.h>
# include <linux/sdlasfm.h>
# include <linux/sdla_front_end.h>
# include <linux/sdla_remora.h>
# include <linux/wanpipe_version.h>
# include <linux/wanpipe.h>
#elif defined(__FreeBSD__) || defined(__OpenBSD__)
# include <wanpipe_version.h>
# include <wanproc.h>
# include <wanpipe.h>
#elif defined(__WINDOWS__)
# include "wanpipe_includes.h"
# include "wanpipe_version.h"
# include "wanpipe_defines.h"
# include "wanpipe_cfg.h"
# include "sdlasfm.h"
# include "sdla_front_end.h"
# include "sdla_remora.h"
# include "wanpipe.h"

#ifdef __cplusplus
extern "C" {	/* for C++ users */
#endif

# define WAN_HWEC
#else
# include <sys/stat.h>
# include <sys/ioctl.h>
# include <sys/types.h>
# include <fcntl.h>
# include <dirent.h>
# include <unistd.h>
# include <sys/socket.h>
# include <sys/un.h>
# include <signal.h>
# include <time.h>
# include <sys/types.h>
# include <wanpipe_defines.h>
# include <sdla_fr.h>
# include <wanpipe_cfg.h>
# include <sdla_te1.h>
# include <sdlasfm.h>
# include <sdla_front_end.h>
# include <sdla_remora.h>
#endif

/****** Defines *************************************************************/

#define smemof(TYPE, MEMBER) offsetof(TYPE,MEMBER),(sizeof(((TYPE *)0)->MEMBER))

#ifndef	min
# define	min(a,b)	(((a)<(b))?(a):(b))
#endif

#define	is_digit(ch) (((ch)>=(unsigned)'0'&&(ch)<=(unsigned)'9')?1:0)

enum ErrCodes		/* Error codes */
{
	ERR_SYSTEM = 1,	/* system error */
	ERR_SYNTAX,		/* command line syntax error */
	ERR_CONFIG,		/* configuration file syntax error */
	ERR_MODULE,		/* module not loaded */
	ERR_LIMIT
};

/* Configuration stuff */
#define	MAX_CFGLINE	256
#define	MAX_CMDLINE	256
#define	MAX_TOKENS	32

/* Device directory and WAN device name */
#define WANDEV_NAME "/dev/wanrouter"

/****** Data Types **********************************************************/

typedef struct look_up		/* Look-up table entry */
{
	uint	val;		/* look-up value */
	void*	ptr;		/* pointer */
} look_up_t;

typedef struct key_word		/* Keyword table entry */
{
	char*	keyword;	/* -> keyword */
	uint	offset;		/* offset of the related parameter */
	int	size;		/* size of type */
	int	dtype;		/* data type */
} key_word_t;

typedef struct data_buf		/* General data buffer */
{
	unsigned size;
	void* data;
} data_buf_t;

/*
 * Data types for configuration structure description tables.
 */
#define	DTYPE_INT		1
#define	DTYPE_UINT		2
#define	DTYPE_LONG		3
#define	DTYPE_ULONG		4
#define	DTYPE_SHORT		5
#define	DTYPE_USHORT		6
#define	DTYPE_CHAR		7
#define	DTYPE_UCHAR		8
#define	DTYPE_PTR		9
#define	DTYPE_STR		10
#define	DTYPE_FILENAME		11
#define	DTYPE_OCT_FILENAME	12
#define DTYPE_OCT_CHAN_CONF	13

#define NO_ANNEXG	 0
#define ANNEXG_LAPB	 1
#define ANNEXG_X25	 2
#define ANNEXG_DSP	 3

#define ANNEXG_FR	 4
#define ANNEXG_PPP	 5
#define ANNEXG_CHDLC	 6
#define ANNEXG_LIP_LAPB  7
#define ANNEXG_LIP_XDLC  8
#define ANNEXG_LIP_TTY   9
#define ANNEXG_LIP_XMTP2 10
#define ANNEXG_LIP_X25   11
#define ANNEXG_LIP_ATM   12
#define ANNEXG_LIP_LAPD  13
#define ANNEXG_LIP_KATM  14
#define ANNEXG_LIP_HDLC  15

#define WANCONFIG_SOCKET "/etc/wanpipe/wanconfig_socket"
#define WANCONFIG_PID    "/var/run/wanconfig.pid"
#define WANCONFIG_PID_FILE "/proc/net/wanrouter/pid"

#define show_error(x) { show_error_dbg(x, __LINE__);}

/***************************************************************************/

#define MAX_PATH_LENGTH 4096
#define NO  1
#define YES 2

#if defined(__LINUX__)
# if !defined(strlcpy)
#  define strlcpy(d,s,l) strcpy((d),(s))
# endif
#elif defined(__FreeBSD__)
# if !defined(strlcpy)
#  define strlcpy(d,s,l) strcpy((d),(s))
# endif
#elif defined(__WINDOWS__)
# define strlcpy strncpy
# ifndef strdup
#  define strdup	_strdup
# endif
#endif

struct link_def;
typedef struct chan_def			/* Logical Channel definition */
{
	char name[WAN_IFNAME_SZ+1];	/* interface name for this channel */
	char* addr;			/* -> media address */
	char* conf;			/* -> configuration data */
	char* conf_profile;		/* -> protocol provile data */
	char* usedby;			/* used by WANPIPE or API */
	char  annexg;			/* -> anexg interface */
	char* label;			/* -> user defined label/comment */
	char* virtual_addr;		/* -> X.25 virtual addr */
	char* real_addr;		/* -> X.25 real addr */
	char* protocol;			/* -> protocol */
	char active_ch[50];		/* -> active channels */
	wanif_conf_t  *chanconf;	/* channel configuration structure */
	struct link_def	*link;		/* Point to WAN link definition */
	struct chan_def* next;		/* -> next channel definition */
} chan_def_t;

typedef struct link_def			/* WAN Link definition */
{
	char name[WAN_DRVNAME_SZ+1];	/* link device name */
	int config_id;			/* configuration ID */
	char* conf;			/* -> configuration data */
	char* descr;			/* -> description */
	unsigned long checksum;
	time_t modified;
	chan_def_t* chan;		/* list of channel definitions */
	wandev_conf_t *linkconf;	/* link configuration structure */
	struct link_def* next;		/* -> next link definition */
} link_def_t;


/****** Global Function prototypes ******************************************/
#if defined(WAN_HWEC)
int wanconfig_hwec(chan_def_t *def);
#endif

int get_active_channels_str(unsigned int chan_map, int start_channel, int stop_channel, char* chans_str);
unsigned int parse_active_channel(char* val);

/****** Global Data *********************************************************/

/*
 * Configuration structure description tables.
 * WARNING:	These tables MUST be kept in sync with corresponding data
 *		structures defined in linux/wanrouter.h
 */
static key_word_t common_conftab[] =	/* Common configuration parameters */
{
  { "IOPORT",     smemof(wandev_conf_t, ioport),     DTYPE_UINT },
  { "MEMADDR",    smemof(wandev_conf_t, maddr),       DTYPE_UINT  },
  { "MEMSIZE",    smemof(wandev_conf_t, msize),       DTYPE_UINT },
  { "IRQ",        smemof(wandev_conf_t, irq),         DTYPE_UINT },
  { "DMA",        smemof(wandev_conf_t, dma),         DTYPE_UINT },
  { "CARD_TYPE",  smemof(wandev_conf_t, card_type),	DTYPE_UINT },
  { "S514CPU",    smemof(wandev_conf_t, S514_CPU_no), DTYPE_STR },
  { "PCISLOT",    smemof(wandev_conf_t, PCI_slot_no), DTYPE_UINT },
  { "PCIBUS", 	  smemof(wandev_conf_t, pci_bus_no),	DTYPE_UINT },
  { "AUTO_PCISLOT",smemof(wandev_conf_t, auto_hw_detect), DTYPE_UCHAR },
  { "AUTO_DETECT",smemof(wandev_conf_t, auto_hw_detect), DTYPE_UCHAR },
  { "USB_BUSID",	  smemof(wandev_conf_t, usb_busid), DTYPE_STR },
  { "COMMPORT",   smemof(wandev_conf_t, comm_port),   DTYPE_UINT },

  /* TE1 New hardware parameters for T1/E1 board */
//  { "MEDIA",    offsetof(wandev_conf_t, te_cfg)+smemof(sdla_te_cfg_t, media), DTYPE_UCHAR },
//  { "LCODE",    offsetof(wandev_conf_t, te_cfg)+smemof(sdla_te_cfg_t, lcode), DTYPE_UCHAR },
//  { "FRAME",    offsetof(wandev_conf_t, te_cfg)+smemof(sdla_te_cfg_t, frame), DTYPE_UCHAR },

  /* Front-End parameters */
  { "FE_MEDIA",    offsetof(wandev_conf_t, fe_cfg)+smemof(sdla_fe_cfg_t, media), DTYPE_UCHAR },
  { "FE_SUBMEDIA",    offsetof(wandev_conf_t, fe_cfg)+smemof(sdla_fe_cfg_t, sub_media), DTYPE_UCHAR },
  { "FE_LCODE",    offsetof(wandev_conf_t, fe_cfg)+smemof(sdla_fe_cfg_t, lcode), DTYPE_UCHAR },
  { "FE_FRAME",    offsetof(wandev_conf_t, fe_cfg)+smemof(sdla_fe_cfg_t, frame), DTYPE_UCHAR },
  { "FE_LINE",    offsetof(wandev_conf_t, fe_cfg)+smemof(sdla_fe_cfg_t, line_no),  DTYPE_UINT },
  { "FE_POLL",    offsetof(wandev_conf_t, fe_cfg)+smemof(sdla_fe_cfg_t, poll_mode),  DTYPE_UCHAR },
  { "FE_TXTRISTATE",    offsetof(wandev_conf_t, fe_cfg)+smemof(sdla_fe_cfg_t, tx_tristate_mode),  DTYPE_UCHAR },
  { "FE_NETWORK_SYNC",  offsetof(wandev_conf_t, fe_cfg)+smemof(sdla_fe_cfg_t, network_sync), DTYPE_UINT },

  /* Front-End parameters (old style) */
  /* Front-End parameters (old style) */
  { "MEDIA",    offsetof(wandev_conf_t, fe_cfg)+smemof(sdla_fe_cfg_t, media), DTYPE_UCHAR },
  { "LCODE",    offsetof(wandev_conf_t, fe_cfg)+smemof(sdla_fe_cfg_t, lcode), DTYPE_UCHAR },
  { "FRAME",    offsetof(wandev_conf_t, fe_cfg)+smemof(sdla_fe_cfg_t, frame), DTYPE_UCHAR },
  /* T1/E1 Front-End parameters */
  { "TE_LBO",           offsetof(wandev_conf_t, fe_cfg)+offsetof(sdla_fe_cfg_t, cfg) + smemof(sdla_te_cfg_t, lbo), DTYPE_UCHAR },
  { "TE_CLOCK",         offsetof(wandev_conf_t, fe_cfg)+offsetof(sdla_fe_cfg_t, cfg) + smemof(sdla_te_cfg_t, te_clock), DTYPE_UCHAR },
  { "TE_ACTIVE_CH",	offsetof(wandev_conf_t, fe_cfg)+offsetof(sdla_fe_cfg_t, cfg) + smemof(sdla_te_cfg_t, active_ch), DTYPE_UINT },
  { "TE_RBS_CH",	offsetof(wandev_conf_t, fe_cfg)+offsetof(sdla_fe_cfg_t, cfg) + smemof(sdla_te_cfg_t, te_rbs_ch), DTYPE_UINT },
  { "TE_HIGHIMPEDANCE", offsetof(wandev_conf_t, fe_cfg)+offsetof(sdla_fe_cfg_t, cfg) + smemof(sdla_te_cfg_t, high_impedance_mode), DTYPE_UCHAR },
  { "TE_RX_SLEVEL", offsetof(wandev_conf_t, fe_cfg)+offsetof(sdla_fe_cfg_t, cfg) + smemof(sdla_te_cfg_t, rx_slevel), DTYPE_INT },
  { "TE_REF_CLOCK",     offsetof(wandev_conf_t, fe_cfg)+offsetof(sdla_fe_cfg_t, cfg) + smemof(sdla_te_cfg_t, te_ref_clock), DTYPE_UCHAR },
  { "TE_SIG_MODE",     offsetof(wandev_conf_t, fe_cfg)+offsetof(sdla_fe_cfg_t, cfg) + smemof(sdla_te_cfg_t, sig_mode), DTYPE_UCHAR },
  { "TE_IGNORE_YEL", offsetof(wandev_conf_t, fe_cfg)+offsetof(sdla_fe_cfg_t, cfg) + smemof(sdla_te_cfg_t, ignore_yel_alarm), DTYPE_UCHAR },
  { "TE_AIS_MAINTENANCE", offsetof(wandev_conf_t, fe_cfg)+offsetof(sdla_fe_cfg_t, cfg) + smemof(sdla_te_cfg_t, ais_maintenance), DTYPE_UCHAR },
  { "TE_AIS_AUTO_ON_LOS", offsetof(wandev_conf_t, fe_cfg)+offsetof(sdla_fe_cfg_t, cfg) + smemof(sdla_te_cfg_t, ais_auto_on_los), DTYPE_UCHAR },
  /* T1/E1 Front-End parameters (old style) */
  { "LBO",           offsetof(wandev_conf_t, fe_cfg)+offsetof(sdla_fe_cfg_t, cfg) + smemof(sdla_te_cfg_t, lbo), DTYPE_UCHAR },
  { "ACTIVE_CH",	offsetof(wandev_conf_t, fe_cfg)+offsetof(sdla_fe_cfg_t, cfg) + smemof(sdla_te_cfg_t, active_ch), DTYPE_UINT },
  { "HIGHIMPEDANCE", offsetof(wandev_conf_t, fe_cfg)+offsetof(sdla_fe_cfg_t, cfg) + smemof(sdla_te_cfg_t, high_impedance_mode), DTYPE_UCHAR },
  /* T3/E3 Front-End parameters */
  { "TE3_FRACTIONAL",offsetof(wandev_conf_t, fe_cfg)+offsetof(sdla_fe_cfg_t, cfg) + smemof(sdla_te3_cfg_t, fractional), DTYPE_UINT },
  { "TE3_RDEVICE",	offsetof(wandev_conf_t, fe_cfg)+offsetof(sdla_fe_cfg_t, cfg) + smemof(sdla_te3_cfg_t, rdevice_type), DTYPE_UINT },
  { "TE3_FCS",	offsetof(wandev_conf_t, fe_cfg)+offsetof(sdla_fe_cfg_t, cfg) + smemof(sdla_te3_cfg_t, fcs), DTYPE_UINT },
  { "TE3_RXEQ",	offsetof(wandev_conf_t, fe_cfg)+offsetof(sdla_fe_cfg_t, cfg) + offsetof(sdla_te3_cfg_t, liu_cfg) + smemof(sdla_te3_liu_cfg_t, rx_equal), DTYPE_UINT },
  { "TE3_TAOS",	offsetof(wandev_conf_t, fe_cfg)+offsetof(sdla_fe_cfg_t, cfg) + offsetof(sdla_te3_cfg_t, liu_cfg) + smemof(sdla_te3_liu_cfg_t, taos), DTYPE_UINT },
  { "TE3_LBMODE",	offsetof(wandev_conf_t, fe_cfg)+offsetof(sdla_fe_cfg_t, cfg) + offsetof(sdla_te3_cfg_t, liu_cfg) + smemof(sdla_te3_liu_cfg_t, lb_mode), DTYPE_UINT },
  { "TE3_TXLBO",	offsetof(wandev_conf_t, fe_cfg)+offsetof(sdla_fe_cfg_t, cfg) + offsetof(sdla_te3_cfg_t, liu_cfg) + smemof(sdla_te3_liu_cfg_t, tx_lbo), DTYPE_UINT },
  { "TE3_CLOCK",         offsetof(wandev_conf_t, fe_cfg)+offsetof(sdla_fe_cfg_t, cfg) + smemof(sdla_te3_cfg_t, clock), DTYPE_UINT },
  { "TDMV_LAW",         offsetof(wandev_conf_t, fe_cfg)+smemof(sdla_fe_cfg_t, tdmv_law), DTYPE_UINT },
  { "TDMV_OPERMODE",    offsetof(wandev_conf_t, fe_cfg)+offsetof(sdla_fe_cfg_t, cfg) + smemof(sdla_remora_cfg_t, opermode_name), DTYPE_STR },
  { "RM_OHTHRESH",    offsetof(wandev_conf_t, fe_cfg)+offsetof(sdla_fe_cfg_t, cfg) + smemof(sdla_remora_cfg_t, ohthresh), DTYPE_UINT },
  { "RM_BATTTHRESH",    offsetof(wandev_conf_t, fe_cfg)+offsetof(sdla_fe_cfg_t, cfg) + smemof(sdla_remora_cfg_t, battthresh), DTYPE_UINT },
  { "RM_BATTDEBOUNCE",  offsetof(wandev_conf_t, fe_cfg)+offsetof(sdla_fe_cfg_t, cfg) + smemof(sdla_remora_cfg_t, battdebounce), DTYPE_UINT },
  { "RM_MODE",    offsetof(wandev_conf_t, fe_cfg)+offsetof(sdla_fe_cfg_t, cfg) + smemof(sdla_remora_cfg_t, rm_mode), DTYPE_UCHAR },
  { "RM_LCM",    offsetof(wandev_conf_t, fe_cfg)+offsetof(sdla_fe_cfg_t, cfg) + smemof(sdla_remora_cfg_t, rm_lcm), DTYPE_UCHAR },
  
  { "RM_BRI_CLOCK_MASTER",  offsetof(wandev_conf_t, fe_cfg)+offsetof(sdla_fe_cfg_t, cfg) + smemof(sdla_bri_cfg_t, clock_mode), DTYPE_UCHAR },
  { "RM_BRI_CLOCK",  offsetof(wandev_conf_t, fe_cfg)+offsetof(sdla_fe_cfg_t, cfg) + smemof(sdla_bri_cfg_t, clock_mode), DTYPE_UCHAR },
  { "RM_NETWORK_SYNC",  offsetof(wandev_conf_t, fe_cfg)+smemof(sdla_fe_cfg_t, network_sync), DTYPE_UINT },
  { "RM_FASTRINGER",  offsetof(wandev_conf_t, fe_cfg)+offsetof(sdla_fe_cfg_t, cfg) + smemof(sdla_remora_cfg_t, fxs_fastringer), DTYPE_UCHAR },
  { "RM_LOWPOWER",  offsetof(wandev_conf_t, fe_cfg)+offsetof(sdla_fe_cfg_t, cfg) + smemof(sdla_remora_cfg_t, fxs_lowpower), DTYPE_UCHAR },
  { "RM_FXSTXGAIN",  offsetof(wandev_conf_t, fe_cfg)+offsetof(sdla_fe_cfg_t, cfg) + smemof(sdla_remora_cfg_t, fxs_txgain), DTYPE_INT },
  { "RM_FXSRXGAIN",  offsetof(wandev_conf_t, fe_cfg)+offsetof(sdla_fe_cfg_t, cfg) + smemof(sdla_remora_cfg_t, fxs_rxgain), DTYPE_INT },
  { "RM_FXOTXGAIN",  offsetof(wandev_conf_t, fe_cfg)+offsetof(sdla_fe_cfg_t, cfg) + smemof(sdla_remora_cfg_t, fxo_txgain), DTYPE_INT },
  { "RM_FXORXGAIN",  offsetof(wandev_conf_t, fe_cfg)+offsetof(sdla_fe_cfg_t, cfg) + smemof(sdla_remora_cfg_t, fxo_rxgain), DTYPE_INT },
  { "RM_PULSEDIALING",  offsetof(wandev_conf_t, fe_cfg)+offsetof(sdla_fe_cfg_t, cfg) + smemof(sdla_remora_cfg_t, fxs_pulsedialing), DTYPE_UCHAR },
  { "RM_RINGAMPL",  offsetof(wandev_conf_t, fe_cfg)+offsetof(sdla_fe_cfg_t, cfg) + smemof(sdla_remora_cfg_t, fxs_ringampl), DTYPE_INT },
  { "RM_RELAXCFG",  offsetof(wandev_conf_t, fe_cfg)+offsetof(sdla_fe_cfg_t, cfg) + smemof(sdla_remora_cfg_t, relaxcfg), DTYPE_UCHAR },
  { "RM_FAKE_POLARITY",  offsetof(wandev_conf_t, fe_cfg)+offsetof(sdla_fe_cfg_t, cfg) + smemof(sdla_remora_cfg_t, fake_polarity), DTYPE_UCHAR },
  { "RM_FAKE_POLARITY_THRESHOLD",  offsetof(wandev_conf_t, fe_cfg)+offsetof(sdla_fe_cfg_t, cfg) + smemof(sdla_remora_cfg_t, fake_polarity_thres), DTYPE_INT },
  
  /* TDMV parameters */
  { "TDMV_SPAN",     offsetof(wandev_conf_t, tdmv_conf)+smemof(wan_tdmv_conf_t, span_no), DTYPE_UCHAR},
  { "TDMV_DCHAN",    offsetof(wandev_conf_t, tdmv_conf)+smemof(wan_tdmv_conf_t, dchan),   DTYPE_UINT},
  { "TDMV_HW_DTMF",  offsetof(wandev_conf_t, tdmv_conf)+smemof(wan_tdmv_conf_t, hw_dtmf), DTYPE_UCHAR},
  { "TDMV_HW_FAX_DETECT",  offsetof(wandev_conf_t, tdmv_conf)+smemof(wan_tdmv_conf_t, hw_fax_detect), DTYPE_UCHAR},
  { "TDMV_HW_FAXCALLED",  offsetof(wandev_conf_t, tdmv_conf)+smemof(wan_tdmv_conf_t, hw_faxcalled), DTYPE_UCHAR},

  { "TDMV_DUMMY_REF",  offsetof(wandev_conf_t, tdmv_conf)+smemof(wan_tdmv_conf_t, sdla_tdmv_dummy_enable), DTYPE_UCHAR},
  { "TDMV_EC_OFF_ON_FAX",  offsetof(wandev_conf_t, tdmv_conf)+smemof(wan_tdmv_conf_t, ec_off_on_fax), DTYPE_UINT},

  { "RTP_TAP_IP",   offsetof(wandev_conf_t, rtp_conf)+smemof(wan_rtp_conf_t, rtp_ip), DTYPE_UINT},
  { "RTP_TAP_PORT",  offsetof(wandev_conf_t, rtp_conf)+smemof(wan_rtp_conf_t, rtp_port), DTYPE_USHORT},
  { "RTP_TAP_SAMPLE",  offsetof(wandev_conf_t, rtp_conf)+smemof(wan_rtp_conf_t, rtp_sample), DTYPE_USHORT},
  { "RTP_TAP_DEV", offsetof(wandev_conf_t, rtp_conf)+smemof(wan_rtp_conf_t, rtp_devname), DTYPE_STR},
  { "RTP_TAP_MAC",  offsetof(wandev_conf_t, rtp_conf)+smemof(wan_rtp_conf_t, rtp_mac), DTYPE_UINT},           
  
  { "HWEC_CLKSRC",   offsetof(wandev_conf_t, hwec_conf)+smemof(wan_hwec_conf_t, clk_src), DTYPE_UINT},  
  { "HWEC_PERSIST",  offsetof(wandev_conf_t, hwec_conf)+smemof(wan_hwec_conf_t, persist_disable), DTYPE_UINT},  
   /* Keep backward compatibility */
  { "TDMV_HWEC_PERSIST_DISABLE",  offsetof(wandev_conf_t, hwec_conf)+smemof(wan_hwec_conf_t, persist_disable), DTYPE_UINT},  

  /* hwec parameters */
  { "HWEC_NOISE_REDUCTION",  offsetof(wandev_conf_t, hwec_conf)+smemof(wan_hwec_conf_t, noise_reduction), DTYPE_UCHAR},  
  { "HWEC_NOISE_REDUCTION_DISABLE",  offsetof(wandev_conf_t, hwec_conf)+smemof(wan_hwec_conf_t, noise_reduction_disable), DTYPE_UCHAR},  
  { "HWEC_TONEDISABLERDELAY",  offsetof(wandev_conf_t, hwec_conf)+smemof(wan_hwec_conf_t, tone_disabler_delay), DTYPE_UINT},
  { "HWEC_DTMF_REMOVAL",  offsetof(wandev_conf_t, hwec_conf)+smemof(wan_hwec_conf_t, dtmf_removal), DTYPE_UCHAR},
  { "HWEC_OPERATION_MODE",  offsetof(wandev_conf_t, hwec_conf)+smemof(wan_hwec_conf_t, operation_mode), DTYPE_UCHAR},
  { "HWEC_ACUSTIC_ECHO",  offsetof(wandev_conf_t, hwec_conf)+smemof(wan_hwec_conf_t, acustic_echo), DTYPE_UCHAR},
  { "HWEC_NLP_DISABLE",  offsetof(wandev_conf_t, hwec_conf)+smemof(wan_hwec_conf_t, nlp_disable), DTYPE_UCHAR},
  { "HWEC_TX_AUTO_GAIN",  offsetof(wandev_conf_t, hwec_conf)+smemof(wan_hwec_conf_t, tx_auto_gain), DTYPE_INT},
  { "HWEC_RX_AUTO_GAIN",  offsetof(wandev_conf_t, hwec_conf)+smemof(wan_hwec_conf_t, rx_auto_gain), DTYPE_INT},
  { "HWEC_TX_GAIN",  offsetof(wandev_conf_t, hwec_conf)+smemof(wan_hwec_conf_t, tx_gain), DTYPE_INT},
  { "HWEC_RX_GAIN",  offsetof(wandev_conf_t, hwec_conf)+smemof(wan_hwec_conf_t, rx_gain), DTYPE_INT},

  { "OCT_CHIP_CONF",	smemof(wandev_conf_t, oct_conf), DTYPE_OCT_FILENAME }, 
  { "OCT_ECHOOPERATIONMODE",	smemof(wandev_conf_t, oct_conf), DTYPE_OCT_CHAN_CONF},

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
  
  /* ATM can be in the LIP layer but hardware setting stay in hardware section,
     so this the copy */
#if 0
  {"ATM_SYNC_MODE", offsetof(wandev_conf_t, u) + smemof(wan_atm_conf_t, atm_sync_mode), DTYPE_USHORT },
  {"ATM_SYNC_DATA", offsetof(wandev_conf_t, u) + smemof(wan_atm_conf_t,atm_sync_data), DTYPE_USHORT },
  {"ATM_SYNC_OFFSET", offsetof(wandev_conf_t, u) + smemof(wan_atm_conf_t,atm_sync_offset), DTYPE_USHORT },
  {"ATM_HUNT_TIMER", offsetof(wandev_conf_t, u) + smemof(wan_atm_conf_t,atm_hunt_timer), DTYPE_USHORT },
  {"ATM_CELL_CFG", offsetof(wandev_conf_t, u) + smemof(wan_atm_conf_t,atm_cell_cfg), DTYPE_UCHAR },
  {"ATM_CELL_PT", offsetof(wandev_conf_t, u) + smemof(wan_atm_conf_t,atm_cell_pt), DTYPE_UCHAR },
  {"ATM_CELL_CLP", offsetof(wandev_conf_t, u) + smemof(wan_atm_conf_t,atm_cell_clp), DTYPE_UCHAR },
  {"ATM_CELL_PAYLOAD", offsetof(wandev_conf_t, u) + smemof(wan_atm_conf_t,atm_cell_payload), DTYPE_UCHAR },
#endif

  { NULL, 0, 0 }
};

static key_word_t sppp_conftab[] =	/* PPP-CHDLC (in LIP layer!!!) specific configuration */
 {
  { "IP_MODE",        smemof(wan_sppp_if_conf_t, dynamic_ip),       DTYPE_UCHAR},
	
  { "AUTH_TIMER",     smemof(wan_sppp_if_conf_t, pp_auth_timer),    DTYPE_UINT},
  { "KEEPALIVE_TIMER",smemof(wan_sppp_if_conf_t, sppp_keepalive_timer),    DTYPE_UINT},
  { "PPP_TIMER",      smemof(wan_sppp_if_conf_t, pp_timer),    DTYPE_UINT},
 
  { "PAP",	      smemof(wan_sppp_if_conf_t, pap),    DTYPE_UCHAR},		
  { "CHAP",	      smemof(wan_sppp_if_conf_t, chap),    DTYPE_UCHAR},		

  { "USERID",         smemof(wan_sppp_if_conf_t, userid), 	DTYPE_STR},
  { "PASSWD",         smemof(wan_sppp_if_conf_t, passwd),	DTYPE_STR},
 
  /* note, the name is NOT "KEEPALIVE_ERR_MARGIN" intentionally, couse this name is defined
     inside wanif_conf_t, and data gets to wrong place */
  { "KEEPALIVE_ERROR_MARGIN", smemof(wan_sppp_if_conf_t, keepalive_err_margin),    DTYPE_UINT },

  { "MAGIC_DISABLE",      smemof(wan_sppp_if_conf_t, disable_magic),    DTYPE_UCHAR},

  { NULL, 0, 0 }
};

static key_word_t fr_conftab[] =	/* Frame relay-specific configuration */
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
  { "EEK_CFG",       smemof(wan_fr_conf_t, eek_cfg),     DTYPE_UINT },
  { "EEK_TIMER",     smemof(wan_fr_conf_t, eek_timer),     DTYPE_UINT },   
  { NULL, 0, 0 }
};

static key_word_t xilinx_conftab[] =	/* Xilinx specific configuration */
{
  { "MRU",     	     smemof(wan_xilinx_conf_t, mru),          DTYPE_USHORT },
  { "DMA_PER_CH",    smemof(wan_xilinx_conf_t, dma_per_ch),   DTYPE_USHORT },
  { "RBS",    	     smemof(wan_xilinx_conf_t, rbs),          DTYPE_UCHAR },
  { "DATA_MUX_MAP",  smemof(wan_xilinx_conf_t, data_mux_map), DTYPE_UINT },
  { "RX_CRC_BYTES",  smemof(wan_xilinx_conf_t, rx_crc_bytes), DTYPE_UINT},
  { "SPAN_TX_ONLY_IRQ",  smemof(wan_xilinx_conf_t, span_tx_only_irq), DTYPE_UCHAR},
  { "HW_RJ45_PORT_MAP",   smemof(wan_xilinx_conf_t, hw_port_map), DTYPE_UCHAR },
  { "RX_FIFO_TRIGGER",  smemof(wan_xilinx_conf_t, rx_fifo_trigger), DTYPE_USHORT },
  { "TX_FIFO_TRIGGER",  smemof(wan_xilinx_conf_t, tx_fifo_trigger), DTYPE_USHORT },
  { NULL, 0, 0 }
};

static key_word_t edukit_conftab[] =	
{
	{ NULL, 0, 0 }
};

static key_word_t bitstrm_conftab[] =	/* Bitstreaming specific configuration */
{
 /* Bit strm options */
  { "SYNC_OPTIONS",	       smemof(wan_bitstrm_conf_t, sync_options), DTYPE_USHORT },
  { "RX_SYNC_CHAR",	       smemof(wan_bitstrm_conf_t, rx_sync_char), DTYPE_USHORT},
  { "MONOSYNC_TX_TIME_FILL_CHAR", smemof(wan_bitstrm_conf_t, monosync_tx_time_fill_char), DTYPE_UCHAR},
  { "MAX_LENGTH_TX_DATA_BLOCK", smemof(wan_bitstrm_conf_t,max_length_tx_data_block), DTYPE_UINT},
  { "RX_COMPLETE_LENGTH",       smemof(wan_bitstrm_conf_t,rx_complete_length), DTYPE_UINT},
  { "RX_COMPLETE_TIMER",        smemof(wan_bitstrm_conf_t,rx_complete_timer), DTYPE_UINT},	
  { "RBS_CH_MAP",		smemof(wan_bitstrm_conf_t,rbs_map), DTYPE_UINT},
  { NULL, 0, 0 }
};

/*
static key_word_t atm_conftab[] =
{
  {"ATM_SYNC_MODE", smemof(wanif_conf_t, u) + offsetof(wan_atm_conf_t, atm_sync_mode), DTYPE_USHORT },
  {"ATM_SYNC_DATA", offsetof(wanif_conf_t, u) + offsetof(wan_atm_conf_t,atm_sync_data), DTYPE_USHORT },
  {"ATM_SYNC_OFFSET", offsetof(wanif_conf_t, u) + offsetof(wan_atm_conf_t,atm_sync_offset), DTYPE_USHORT },
  {"ATM_HUNT_TIMER", offsetof(wanif_conf_t, u) + offsetof(wan_atm_conf_t,atm_hunt_timer), DTYPE_USHORT },
  {"ATM_CELL_CFG", offsetof(wanif_conf_t, u) + offsetof(wan_atm_conf_t,atm_cell_cfg), DTYPE_UCHAR },
  {"ATM_CELL_PT", offsetof(wanif_conf_t, u) + offsetof(wan_atm_conf_t,atm_cell_pt), DTYPE_UCHAR },
  {"ATM_CELL_CLP", offsetof(wanif_conf_t, u) + offsetof(wan_atm_conf_t,atm_cell_clp), DTYPE_UCHAR },
  {"ATM_CELL_PAYLOAD", offsetof(wanif_conf_t, u) + offsetof(wan_atm_conf_t,atm_cell_payload), DTYPE_UCHAR },
  { NULL, 0, 0 }
};
*/

static key_word_t atm_conftab[] =
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

static key_word_t atm_if_conftab[] =
{
  /* Per Interface Configuration */
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

  /* Profile Section */
  {"ATM_SYNC_MODE",	       smemof(wan_atm_conf_if_t,atm_sync_mode), DTYPE_USHORT },
  {"ATM_SYNC_DATA",	       smemof(wan_atm_conf_if_t,atm_sync_data), DTYPE_USHORT },
  {"ATM_SYNC_OFFSET", 	       smemof(wan_atm_conf_if_t,atm_sync_offset), DTYPE_USHORT },
  {"ATM_HUNT_TIMER", 	       smemof(wan_atm_conf_if_t,atm_hunt_timer), DTYPE_USHORT },
  {"ATM_CELL_CFG", 	       smemof(wan_atm_conf_if_t,atm_cell_cfg), DTYPE_UCHAR },
  {"ATM_CELL_PT", 	       smemof(wan_atm_conf_if_t,atm_cell_pt), DTYPE_UCHAR },
  {"ATM_CELL_CLP", 	       smemof(wan_atm_conf_if_t,atm_cell_clp), DTYPE_UCHAR },
  {"ATM_CELL_PAYLOAD", 	       smemof(wan_atm_conf_if_t,atm_cell_payload), DTYPE_UCHAR },

  { NULL, 0, 0 }
};

static key_word_t xilinx_if_conftab[] =
{
  { "SIGNALLING",     smemof(wan_xilinx_conf_if_t, signalling),  DTYPE_UINT },
  { "STATION",        smemof(wan_xilinx_conf_if_t, station),     DTYPE_UCHAR },
  { "SEVEN_BIT_HDLC", smemof(wan_xilinx_conf_if_t, seven_bit_hdlc), DTYPE_CHAR },
  { "SW_HDLC", smemof(wan_xilinx_conf_if_t, sw_hdlc), DTYPE_CHAR },
  { "MRU",     	smemof(wan_xilinx_conf_if_t, mru),  DTYPE_UINT },
  { "MTU",     	smemof(wan_xilinx_conf_if_t, mtu),  DTYPE_UINT },
  { "IDLE_MTU",     	smemof(wan_xilinx_conf_if_t, mtu_idle),  DTYPE_UINT },
  { "IDLE_FLAG",     smemof(wan_xilinx_conf_if_t, idle_flag),  DTYPE_UCHAR},
  { "DATA_MUX",    smemof(wan_xilinx_conf_if_t, data_mux),  DTYPE_UCHAR}, 
  { "SS7_ENABLE",  smemof(wan_xilinx_conf_if_t, ss7_enable),  DTYPE_UCHAR},
  { "SS7_MODE",  smemof(wan_xilinx_conf_if_t, ss7_mode),  DTYPE_UCHAR},
  { "SS7_LSSU_SZ",  smemof(wan_xilinx_conf_if_t, ss7_lssu_size),  DTYPE_UCHAR},
  { "RBS_CAS_IDLE",  smemof(wan_xilinx_conf_if_t, rbs_cas_idle), DTYPE_UCHAR },
  { "HDLC_REPEAT",  smemof(wan_xilinx_conf_if_t, hdlc_repeat), DTYPE_UCHAR },
  { "MTP1_FILTER",  smemof(wan_xilinx_conf_if_t, mtp1_filter), DTYPE_USHORT },
  { NULL, 0, 0 }
};

static key_word_t bitstrm_if_conftab[] =
{
  {"MAX_TX_QUEUE", smemof(wan_bitstrm_conf_if_t,max_tx_queue_size), DTYPE_UINT },
  {"MAX_TX_UP_SIZE", smemof(wan_bitstrm_conf_if_t,max_tx_up_size), DTYPE_UINT },
  {"SEVEN_BIT_HDLC", smemof(wan_bitstrm_conf_if_t,seven_bit_hdlc), DTYPE_CHAR },
  { NULL, 0, 0 }
};

static key_word_t adsl_conftab[] =
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

static key_word_t bscstrm_conftab[]=
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

static key_word_t ss7_conftab[] =
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

static key_word_t x25_conftab[] =	/* X.25-specific configuration */
{
  { "LOWESTPVC",    smemof(wan_x25_conf_t, lo_pvc),       DTYPE_UINT },
  { "HIGHESTPVC",   smemof(wan_x25_conf_t, hi_pvc),       DTYPE_UINT },
  { "LOWESTSVC",    smemof(wan_x25_conf_t, lo_svc),       DTYPE_UINT },
  { "HIGHESTSVC",   smemof(wan_x25_conf_t, hi_svc),       DTYPE_UINT },
  { "HDLCWINDOW",   smemof(wan_x25_conf_t, hdlc_window),  DTYPE_UINT },
  { "PACKETWINDOW", smemof(wan_x25_conf_t, pkt_window),   DTYPE_UINT },
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
  { "CCITTCOMPAT",  smemof(wan_x25_conf_t, ccitt_compat), DTYPE_UINT },
  { "X25CONFIG",    smemof(wan_x25_conf_t, x25_conf_opt), DTYPE_UINT }, 	
  { "LAPB_HDLC_ONLY", smemof(wan_x25_conf_t, LAPB_hdlc_only), DTYPE_UCHAR },
  { "CALL_SETUP_LOG", smemof(wan_x25_conf_t, logging), DTYPE_UCHAR },
  { "OOB_ON_MODEM",   smemof(wan_x25_conf_t, oob_on_modem), DTYPE_UCHAR},
  { "STATION_ADDR", smemof(wan_x25_conf_t, local_station_address), DTYPE_UCHAR},
  { "DEF_PKT_SIZE", smemof(wan_x25_conf_t, defPktSize),  DTYPE_USHORT },
  { "MAX_PKT_SIZE", smemof(wan_x25_conf_t, pktMTU),  DTYPE_USHORT },
  { "STATION",      smemof(wan_x25_conf_t, station),  DTYPE_UCHAR },
  { NULL, 0, 0 }
};

static key_word_t lapb_annexg_conftab[] =
{
  //LAPB STUFF
  { "T1", smemof(wan_lapb_if_conf_t, t1),    DTYPE_UINT },
  { "T2", smemof(wan_lapb_if_conf_t, t2),    DTYPE_UINT },
  { "T3", smemof(wan_lapb_if_conf_t, t3),    DTYPE_UINT },
  { "T4", smemof(wan_lapb_if_conf_t, t4),    DTYPE_UINT },
  { "N2", smemof(wan_lapb_if_conf_t, n2),    DTYPE_UINT },
  { "LAPB_MODE", 	smemof(wan_lapb_if_conf_t, mode),    DTYPE_UINT },
  { "LAP_MODE", 	smemof(wan_lapb_if_conf_t, mode),    DTYPE_UINT },
  { "MODE", 		smemof(wan_lapb_if_conf_t, mode),    DTYPE_UINT },
  { "LAPB_WINDOW", 	smemof(wan_lapb_if_conf_t, window),    DTYPE_UINT },
  { "LAP_WINDOW", 	smemof(wan_lapb_if_conf_t, window),    DTYPE_UINT },
  { "WINDOW", 		smemof(wan_lapb_if_conf_t, window),    DTYPE_UINT },
  { "LAP_STATION",     	smemof(wan_lapb_if_conf_t, station),    DTYPE_UINT },

  { "LABEL",		smemof(wan_lapb_if_conf_t,label), DTYPE_STR},
  { "VIRTUAL_ADDR",     smemof(wan_lapb_if_conf_t,virtual_addr), DTYPE_STR},
  { "REAL_ADDR",        smemof(wan_lapb_if_conf_t,real_addr), DTYPE_STR},

  { "MAX_PKT_SIZE", 	smemof(wan_lapb_if_conf_t,mtu), DTYPE_UINT},

  { NULL, 0, 0, 0 }
};

static key_word_t x25_annexg_conftab[] =
{
 //X25 STUFF
  { "PACKETWINDOW", smemof(wan_x25_if_conf_t, packet_window),   DTYPE_USHORT },
  { "CCITTCOMPAT",  smemof(wan_x25_if_conf_t, CCITT_compatibility), DTYPE_USHORT },
  { "T10_T20", 	    smemof(wan_x25_if_conf_t, T10_T20), DTYPE_UINT },
  { "T11_T21", 	    smemof(wan_x25_if_conf_t, T11_T21), DTYPE_UINT },	
  { "T12_T22", 	    smemof(wan_x25_if_conf_t, T12_T22), DTYPE_UINT },
  { "T13_T23", 	    smemof(wan_x25_if_conf_t, T13_T23), DTYPE_UINT },
  { "T16_T26", 	    smemof(wan_x25_if_conf_t, T16_T26), DTYPE_UINT },
  { "T28", 	    smemof(wan_x25_if_conf_t, T28),     DTYPE_UINT },
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
  
  { NULL, 0, 0, 0 }
};

static key_word_t dsp_annexg_conftab[] =
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
  { NULL, 0, 0, 0 }
};

static key_word_t chan_conftab[] =	/* Channel configuration parameters */
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
  { "PAP",     	     	smemof(wanif_conf_t, pap),		DTYPE_UCHAR},
  { "CHAP",          	smemof(wanif_conf_t, chap),		DTYPE_UCHAR},
  { "USERID",        	smemof(wanif_conf_t, userid),	 	DTYPE_STR},
  { "PASSWD",        	smemof(wanif_conf_t, passwd),		DTYPE_STR},
  { "SYSNAME",       	smemof(wanif_conf_t, sysname),		DTYPE_STR},
  { "INARP", 	     	smemof(wanif_conf_t, inarp),          	DTYPE_UCHAR},
  { "INARPINTERVAL", 	smemof(wanif_conf_t, inarp_interval), 	DTYPE_UINT },
  { "INARP_RX",      	smemof(wanif_conf_t, inarp_rx),          	DTYPE_UCHAR},
  { "IGNORE_DCD",  	smemof(wanif_conf_t, ignore_dcd),        	DTYPE_UCHAR},
  { "IGNORE_CTS",    	smemof(wanif_conf_t, ignore_cts),        	DTYPE_UCHAR},
  { "IGNORE_KEEPALIVE", smemof(wanif_conf_t, ignore_keepalive), 	DTYPE_UCHAR},
  { "HDLC_STREAMING", 	smemof(wanif_conf_t, hdlc_streaming), 	DTYPE_UCHAR},

  { "KEEPALIVE_TX_TIMER",	smemof(wanif_conf_t, keepalive_tx_tmr), 	DTYPE_UINT },
  { "KEEPALIVE_RX_TIMER",	smemof(wanif_conf_t, keepalive_rx_tmr), 	DTYPE_UINT },
  { "KEEPALIVE_ERR_MARGIN",	smemof(wanif_conf_t, keepalive_err_margin),	DTYPE_UINT },
  { "SLARP_TIMER", 	smemof(wanif_conf_t, slarp_timer),    		DTYPE_UINT },

  { "TTL",        	smemof(wanif_conf_t, ttl),         DTYPE_UCHAR },

 // { "INTERFACE",  	smemof(wanif_conf_t, interface),   DTYPE_UCHAR },
 // { "CLOCKING",   	smemof(wanif_conf_t, clocking),    DTYPE_UCHAR },
 // { "BAUDRATE",   	smemof(wanif_conf_t, bps),         DTYPE_UINT },
 
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
  { "MPPP_PROT",	       smemof(wanif_conf_t, protocol),  DTYPE_UCHAR},
  { "PROTOCOL",	       	       smemof(wanif_conf_t, protocol),  DTYPE_UCHAR},
  { "ACTIVE_CH",	       smemof(wanif_conf_t, active_ch),  DTYPE_UINT},
  { "SW_DEV_NAME",	       smemof(wanif_conf_t, sw_dev_name),  DTYPE_STR},

  { "LIP_MTU",        	       smemof(wanif_conf_t, mtu),  DTYPE_UINT},

  { "DLCI_TRACE_QUEUE", smemof(wanif_conf_t, max_trace_queue), DTYPE_UINT},
  { "MAX_TRACE_QUEUE", smemof(wanif_conf_t, max_trace_queue), DTYPE_UINT},

  { "TDMV_ECHO_OFF", offsetof(wanif_conf_t, tdmv)+smemof(wan_tdmv_if_conf_t, tdmv_echo_off), DTYPE_UCHAR},
  { "TDMV_CODEC",    offsetof(wanif_conf_t, tdmv)+smemof(wan_tdmv_if_conf_t, tdmv_codec),    DTYPE_UCHAR},
  { "TDMV_HWEC",     offsetof(wanif_conf_t, hwec)+smemof(wan_hwec_if_conf_t, enable),    DTYPE_UCHAR},
//  { "TDMV_ECHO_OFF",	smemof(wanif_conf_t, tdmv_echo_off), DTYPE_UCHAR},
//  { "TDMV_CODEC",	smemof(wanif_conf_t, tdmv_codec), DTYPE_UCHAR},

  { "OCT_ECHOOPERATIONMODE",	smemof(wanif_conf_t, ec_conf), DTYPE_OCT_CHAN_CONF},

  { "SINGLE_TX_BUF",    smemof(wanif_conf_t, single_tx_buf), DTYPE_UCHAR},

  { NULL, 0, 0, 0 }
};

static key_word_t ppp_conftab[] =	/* PPP-specific configuration */
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

static key_word_t chdlc_conftab[] =	/* Cisco HDLC-specific configuration */
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
  { "PROTOCOL_OPTIONS",	  smemof(wan_chdlc_conf_t,protocol_options), DTYPE_UINT },
  { NULL, 0, 0 }
};

static key_word_t sdlc_conftab[] =
{
  { "STATION_CONFIG",	smemof(wan_sdlc_conf_t,station_configuration), DTYPE_UCHAR},
  { "BAUD_RATE",	smemof(wan_sdlc_conf_t,baud_rate),  DTYPE_UINT},
  { "MAX_I_FIELD_LEN",	smemof(wan_sdlc_conf_t,max_I_field_length),  DTYPE_USHORT},
  { "GEN_OP_CFG_BITS",	smemof(wan_sdlc_conf_t,general_operational_config_bits),  DTYPE_USHORT},

  { "PROT_CFG_BITS",	smemof(wan_sdlc_conf_t,protocol_config_bits),  DTYPE_USHORT},
  { "EXCEP_REPORTING",	smemof(wan_sdlc_conf_t,exception_condition_reporting_config),  DTYPE_USHORT},
  { "MODEM_CFG_BITS",	smemof(wan_sdlc_conf_t,modem_config_bits),  DTYPE_USHORT},
  { "STATISTICS_FRMT",	smemof(wan_sdlc_conf_t,statistics_format),  DTYPE_USHORT},
  { "PRI_ST_SLOW_POLL",	smemof(wan_sdlc_conf_t,pri_station_slow_poll_interval),  DTYPE_USHORT},

  { "SEC_ST_RESP_TO",	smemof(wan_sdlc_conf_t,permitted_sec_station_response_TO),  DTYPE_USHORT},
  { "CNSC_NRM_B4_SNRM",	smemof(wan_sdlc_conf_t,no_consec_sec_TOs_in_NRM_before_SNRM_issued),  DTYPE_USHORT},
  { "MX_LN_IFLD_P_XID",	smemof(wan_sdlc_conf_t,max_lgth_I_fld_pri_XID_frame),  DTYPE_USHORT},
  { "O_FLG_BIT_DELAY",	smemof(wan_sdlc_conf_t,opening_flag_bit_delay_count),  DTYPE_USHORT},
  { "RTS_BIT_DELAY",	smemof(wan_sdlc_conf_t,RTS_bit_delay_count),  DTYPE_USHORT},
  { "CTS_TIMEOUT",	smemof(wan_sdlc_conf_t,CTS_timeout_1000ths_sec),  DTYPE_USHORT},
  
  { "SDLC_CONFIG",	smemof(wan_sdlc_conf_t,SDLA_configuration),  DTYPE_UCHAR},

  { NULL, 0, 0 }
};

static key_word_t xdlc_conftab[] =
{

  { "STATION",		smemof(wan_xdlc_conf_t,station), DTYPE_UCHAR},
  { "MAX_I_FIELD_LEN",	smemof(wan_xdlc_conf_t,max_I_field_length),  DTYPE_UINT},
  
  { "WINDOW",		smemof(wan_xdlc_conf_t,window), 	 DTYPE_UINT},
  { "PROT_CONFIG",	smemof(wan_xdlc_conf_t,protocol_config),  DTYPE_UINT},
  { "ERROR_RESP_CONFIG", smemof(wan_xdlc_conf_t,error_response_config),  DTYPE_UINT},
  
  { "TWS_MAX_ACK_CNT",	smemof(wan_xdlc_conf_t,TWS_max_ack_count),  DTYPE_UINT},

  { "PRI_SLOW_POLL_TIMER",      smemof(wan_xdlc_conf_t,pri_slow_poll_timer),  DTYPE_UINT},
  { "PRI_NORMAL_POLL_TIMER",    smemof(wan_xdlc_conf_t,pri_normal_poll_timer),  DTYPE_UINT},
  { "PRI_FRAME_RESPONSE_TIMER", smemof(wan_xdlc_conf_t,pri_frame_response_timer),  DTYPE_UINT},
 
  { "MAX_NO_RESPONSE_CNT",	smemof(wan_xdlc_conf_t,max_no_response_cnt),  DTYPE_UINT},	
  { "MAX_FRAME_RETRANSMIT_CNT",	smemof(wan_xdlc_conf_t,max_frame_retransmit_cnt),  DTYPE_UINT},	
  { "MAX_RNR_CNT",	smemof(wan_xdlc_conf_t,max_rnr_cnt),  DTYPE_UINT},	
  
  { "SEC_NRM_TIMER",	smemof(wan_xdlc_conf_t,sec_nrm_timer),  DTYPE_UINT},
  
  { NULL, 0, 0 }
};

static key_word_t tty_conftab[] =
{
  { NULL, 0, 0 }
};

static key_word_t xmtp2_conftab[] =
{
  { NULL, 0, 0 }
};

static key_word_t lapd_conftab[] =
{
  { NULL, 0, 0 }
};

static key_word_t lip_hdlc_annexg_conftab[] =
{
	
  { "SEVEN_BIT_HDLC", smemof(wan_lip_hdlc_if_conf_t, seven_bit_hdlc), DTYPE_CHAR },
  { "RX_CRC_BYTES", smemof(wan_lip_hdlc_if_conf_t, rx_crc_bytes), DTYPE_CHAR },
  { "LINEIDLE", smemof(wan_lip_hdlc_if_conf_t, lineidle), DTYPE_CHAR },
  { NULL, 0, 0 }

};

static look_up_t conf_def_tables[] =
{
	{ WANCONFIG_PPP,	ppp_conftab	},
	{ WANCONFIG_FR,		fr_conftab	},
	{ WANCONFIG_X25,	x25_conftab	},
	{ WANCONFIG_ADCCP,	x25_conftab	},
	{ WANCONFIG_CHDLC,	chdlc_conftab	},
	{ WANCONFIG_ASYHDLC,	chdlc_conftab	},
	{ WANCONFIG_MPPP,	chdlc_conftab	},
	{ WANCONFIG_LIP_ATM,	atm_if_conftab	},
	{ WANCONFIG_LIP_KATM,	atm_if_conftab	},
	{ WANCONFIG_MFR,	fr_conftab	},
	{ WANCONFIG_SS7,	ss7_conftab 	},
	{ WANCONFIG_ADSL,	adsl_conftab 	},
	{ WANCONFIG_BSCSTRM,	bscstrm_conftab	},
	{ WANCONFIG_ATM,	atm_conftab	},
	{ WANCONFIG_MLINK_PPP,	chdlc_conftab	},
	{ WANCONFIG_AFT,        xilinx_conftab  },
	{ WANCONFIG_AFT_TE1,    xilinx_conftab  },
	{ WANCONFIG_AFT_ANALOG, xilinx_conftab  },
	{ WANCONFIG_AFT_ISDN_BRI, xilinx_conftab  },
	{ WANCONFIG_AFT_SERIAL, xilinx_conftab  },
	{ WANCONFIG_AFT_56K,    xilinx_conftab  },
	{ WANCONFIG_AFT_TE3,    xilinx_conftab  },
	{ WANCONFIG_BITSTRM,    bitstrm_conftab },
	{ WANCONFIG_SDLC,	sdlc_conftab 	},
	{ 0,			NULL		}
};

static look_up_t conf_if_def_tables[] =
{
	{ WANCONFIG_ATM,	atm_if_conftab	},
	{ WANCONFIG_LIP_ATM,	atm_if_conftab	},
	{ WANCONFIG_LIP_KATM,	atm_if_conftab	},
	{ WANCONFIG_BITSTRM,    bitstrm_if_conftab },
	{ WANCONFIG_AFT,	xilinx_if_conftab },
	{ WANCONFIG_AFT_TE1,	xilinx_if_conftab },
	{ WANCONFIG_AFT_TE3,    xilinx_if_conftab },
	{ WANCONFIG_AFT_ANALOG, xilinx_if_conftab },
	{ WANCONFIG_AFT_GSM,    xilinx_if_conftab },
	{ WANCONFIG_AFT_ISDN_BRI, xilinx_if_conftab },
	{ WANCONFIG_AFT_SERIAL, xilinx_if_conftab },
	{ WANCONFIG_AFT_56K,   xilinx_if_conftab },
	{ WANCONFIG_ASYHDLC,	chdlc_conftab	},
	{ 0,			NULL		}
};

static look_up_t conf_annexg_def_tables[] =
{
	{ ANNEXG_LAPB,		lapb_annexg_conftab	},
	{ ANNEXG_LIP_LAPB,	lapb_annexg_conftab	},
	{ ANNEXG_LIP_XDLC,	xdlc_conftab	},
	{ ANNEXG_LIP_TTY,	tty_conftab	},
	{ ANNEXG_LIP_XMTP2,	xmtp2_conftab	},
	{ ANNEXG_LIP_LAPD,	lapb_annexg_conftab	},
	{ ANNEXG_LIP_HDLC,	lip_hdlc_annexg_conftab	},
	{ ANNEXG_X25,		x25_annexg_conftab	},
	{ ANNEXG_DSP,		dsp_annexg_conftab	},
	{ ANNEXG_FR,		fr_conftab	},
	{ ANNEXG_CHDLC,		sppp_conftab	},
	{ ANNEXG_PPP,		sppp_conftab	},
	{ ANNEXG_LIP_X25,	x25_annexg_conftab	},
	{ ANNEXG_LIP_ATM,	atm_if_conftab	},
	{ ANNEXG_LIP_KATM,	atm_if_conftab	},
	{ 0,			NULL		}
};

static look_up_t	config_id_str[] =
{
	{ WANCONFIG_PPP,	"WAN_PPP"	},
	{ WANCONFIG_FR,		"WAN_FR"	},
	{ WANCONFIG_X25,	"WAN_X25"	},
	{ WANCONFIG_CHDLC,	"WAN_CHDLC"	},
	{ WANCONFIG_ASYHDLC,	"WAN_ASYHDLC"	},
    { WANCONFIG_BSC,        "WAN_BSC"       },
    { WANCONFIG_HDLC,       "WAN_HDLC"      },
	{ WANCONFIG_MPPP,       "WAN_MULTPPP"   },
	{ WANCONFIG_MPROT,      "WAN_MULTPROT"  },
	{ WANCONFIG_LIP_ATM,    "WAN_LIP_ATM"   },
	{ WANCONFIG_LIP_KATM,   "WAN_LIP_KATM"   },
	{ WANCONFIG_BITSTRM, 	"WAN_BITSTRM"	},
	{ WANCONFIG_EDUKIT, 	"WAN_EDU_KIT"	},
	{ WANCONFIG_SS7,        "WAN_SS7"       },
	{ WANCONFIG_BSCSTRM,    "WAN_BSCSTRM"   },
	{ WANCONFIG_ADSL,	"WAN_ADSL"	},
	{ WANCONFIG_ADSL,	"WAN_ETH"	},
	{ WANCONFIG_SDLC,	"WAN_SDLC"	},
	{ WANCONFIG_ATM,	"WAN_ATM"	},
	{ WANCONFIG_POS,	"WAN_POS"	},
	{ WANCONFIG_AFT,	"WAN_AFT"	},
	{ WANCONFIG_AFT_TE1,	"WAN_AFT_TE1"	},
	{ WANCONFIG_AFT_ANALOG,	"WAN_AFT_ANALOG" },
	{ WANCONFIG_AFT_ISDN_BRI, "WAN_AFT_ISDN_BRI" },
	{ WANCONFIG_AFT_SERIAL, "WAN_AFT_SERIAL" },
  	{ WANCONFIG_AFT_56K,    "WAN_AFT_56K"   },
	{ WANCONFIG_AFT_TE3,	"WAN_AFT_TE3"	},
	{ WANCONFIG_AFT,	"WAN_XILINX"	},
	{ WANCONFIG_MFR,    	"WAN_MFR"   	},
	{ WANCONFIG_DEBUG,    	"WAN_DEBUG"   	},
	{ WANCONFIG_ADCCP,    	"WAN_ADCCP"   	},
	{ WANCONFIG_MLINK_PPP, 	"WAN_MLINK_PPP" },
	{ WANCONFIG_USB_ANALOG,	"WAN_USB_ANALOG"	},
	{ WANCONFIG_AFT_GSM,	"WAN_AFT_GSM" },
	{ 0,			NULL,		}
};


/*
* Configuration options values and their symbolic names.
*/
static look_up_t	sym_table[] =
{
	/*----- General -----------------------*/
	{ WANOPT_OFF,		"OFF"		}, 
	{ WANOPT_ON,		"ON"		}, 
	{ WANOPT_NO,		"NO"		}, 
	{ WANOPT_YES,		"YES"		}, 
	/*----- Interface type ----------------*/
	{ WANOPT_RS232,		"RS232"		},
	{ WANOPT_V35,		"V35"		},
	/*----- Data encoding -----------------*/
	{ WANOPT_NRZ,		"NRZ"		}, 
	{ WANOPT_NRZI,		"NRZI"		}, 
	{ WANOPT_FM0,		"FM0"		}, 
	{ WANOPT_FM1,		"FM1"		}, 

	/*----- Idle Line ----------------------*/
	{ WANOPT_IDLE_FLAG,	"FLAG"	}, 
	{ WANOPT_IDLE_MARK,	"MARK"	},

	/*----- Link type ---------------------*/
	{ WANOPT_POINTTOPOINT,	"POINTTOPOINT"	},
	{ WANOPT_MULTIDROP,	"MULTIDROP"	},
	/*----- Clocking ----------------------*/
	{ WANOPT_EXTERNAL,	"EXTERNAL"	}, 
	{ WANOPT_INTERNAL,	"INTERNAL"	}, 
	{ WANOPT_RECOVERY,      "RECOVERY"      },
	/*----- Station -----------------------*/
	{ WANOPT_DTE,		"DTE"		}, 
	{ WANOPT_DCE,		"DCE"		}, 
	{ WANOPT_CPE,		"CPE"		}, 
	{ WANOPT_NODE,		"NODE"		}, 
	{ WANOPT_SECONDARY,	"SECONDARY"	}, 
	{ WANOPT_PRIMARY,	"PRIMARY"	}, 
	/*----- Connection options ------------*/
	{ WANOPT_PERMANENT,	"PERMANENT"	}, 
	{ WANOPT_SWITCHED,	"SWITCHED"	}, 
	{ WANOPT_ONDEMAND,	"ONDEMAND"	}, 
	/*----- Frame relay in-channel signalling */
	{ WANOPT_FR_ANSI,	"ANSI"		}, 
	{ WANOPT_FR_Q933,	"Q933"		}, 
	{ WANOPT_FR_LMI,	"LMI"		}, 
	{ WANOPT_FR_NO_LMI,	"NOLMI"		}, 
	/*----- PPP IP Mode Options -----------*/
	{ WANOPT_PPP_STATIC,	"STATIC"	}, 
	{ WANOPT_PPP_HOST,	"HOST"		}, 
	{ WANOPT_PPP_PEER,	"PEER"		}, 
	/*----- CHDLC Protocol Options --------*/
/* DF for now	{ WANOPT_CHDLC_NO_DCD,	"IGNORE_DCD"	},
	{ WANOPT_CHDLC_NO_CTS,	"IGNORE_CTS"	}, 
	{ WANOPT_CHDLC_NO_KEEP,	"IGNORE_KEEPALIVE"}, 
*/
	{ WANOPT_PRI,           "PRI"           },
	{ WANOPT_SEC,           "SEC"           },
	
        /*------Read Mode ---------------------*/
        { WANOPT_INTR,          "INT"           },
        { WANOPT_POLL,          "POLL"          },
	/*------- Async Options ---------------*/
	{ WANOPT_ONE,           "ONE"          	},
	{ WANOPT_TWO,           "TWO"          	},
	{ WANOPT_ONE_AND_HALF,  "ONE_AND_HALF" 	},
	{ WANOPT_NONE,          "NONE"    	},
	{ WANOPT_EVEN,          "EVEN"    	},
	{ WANOPT_ODD,           "ODD"    	},

	{ WANOPT_TTY_SYNC,	"SYNC"		},
	{ WANOPT_TTY_ASYNC,     "ASYNC"		},

	
	{ WANOPT_FR_EEK_REQUEST, "EEK_REQ"	},
	{ WANOPT_FR_EEK_REPLY,	"EEK_REPLY"	}, 
                                                         


	/* TE1 T1/E1 defines */
        /*------TE Cofngiuration --------------*/
        { WAN_MEDIA_T1,      "T1"            },
        { WAN_MEDIA_E1,      "E1"            },
        { WAN_MEDIA_J1,      "J1"            },
	{ WAN_MEDIA_56K,     "56K"           },
	{ WAN_MEDIA_DS3,     "DS3"           },
	{ WAN_MEDIA_STS1,    "STS-1"         },
	{ WAN_MEDIA_E3,      "E3"            },
	{ WAN_MEDIA_FXOFXS,  "FXO/FXS"       },
	{ WAN_MEDIA_GSM,     "GSM"           },
	{ WAN_MEDIA_BRI,  "BRI"       },
        { WAN_LCODE_AMI, 	"AMI"           },
        { WAN_LCODE_B8ZS,       "B8ZS"          },
        { WAN_LCODE_HDB3,       "HDB3"          },
        { WAN_LCODE_B3ZS,       "B3ZS"          },
        { WAN_FR_D4,         "D4"            },
        { WAN_FR_ESF,        "ESF"           },
        { WAN_FR_SLC96,      "SLC-96"        },
        { WAN_FR_NCRC4,      "NCRC4"         },
        { WAN_FR_CRC4,       "CRC4"          },
        { WAN_FR_UNFRAMED,   "UNFRAMED"      },
        { WAN_FR_E3_G751,    "G.751"         },
        { WAN_FR_E3_G832,    "G.832"         },
        { WAN_FR_DS3_Cbit,   "C-BIT"  	},
        { WAN_FR_DS3_M13,    "M13"           },
        { WAN_T1_LBO_0_DB,  	"0DB"           },
        { WAN_T1_LBO_75_DB,  "7.5DB"         },
        { WAN_T1_LBO_15_DB,  "15DB"          },
        { WAN_T1_LBO_225_DB, "22.5DB"        },
        { WAN_T1_0_110,  	"0-110FT"       },
        { WAN_T1_110_220,  	"110-220FT"     },
        { WAN_T1_220_330,  	"220-330FT"     },
        { WAN_T1_330_440,  	"330-440FT"     },
        { WAN_T1_440_550,  	"440-550FT"     },
        { WAN_T1_550_660,  	"550-660FT"     },
        { WAN_T1_0_133,  	"0-133FT"       },
        { WAN_T1_133_266,  	"133-266FT"     },
        { WAN_T1_266_399,  	"266-399FT"     },
        { WAN_T1_399_533,  	"399-533FT"     },
        { WAN_T1_533_655,  	"533-655FT"     },
        { WAN_E1_120,  		"120OH"     	},
        { WAN_E1_75,  		"75OH"     	},
        { WAN_NORMAL_CLK,   	"NORMAL"        },
        { WAN_MASTER_CLK,   	"MASTER"        },
        { WAN_NORMAL_CLK,	"LINE"          },
	{ WAN_MASTER_CLK, 	"OSC"     	},
	{ WAN_CLK_OUT_OSC,	"OUT_OSC"	},
	{ WAN_CLK_OUT_LINE,	"OUT_LINE"	},
	{ WANOPT_NETWORK_SYNC_IN,   "IN" },
	{ WANOPT_NETWORK_SYNC_OUT,  "OUT" },
        { WAN_TE1_SIG_CAS,	"CAS"           },
        { WAN_TE1_SIG_CCS,	"CCS"		},
	
	/* A200/A400 configuration */
	{ WAN_RM_DEFAULT,   	"RM_DEFAULT"        },
        { WAN_RM_TAPPING,   	"RM_TAPPING"        },

	/* T3/E3 configuration */
        { WAN_TE3_RDEVICE_ADTRAN,	"ADTRAN"        },
        { WAN_TE3_RDEVICE_DIGITALLINK,	"DIGITAL-LINK"  },
        { WAN_TE3_RDEVICE_KENTROX,	"KENTROX"       },
        { WAN_TE3_RDEVICE_LARSCOM,	"LARSCOM"       },
        { WAN_TE3_RDEVICE_VERILINK,	"VERILINK"      },
        { WAN_TE3_LIU_LB_NORMAL,	"LB_NORMAL"     },
        { WAN_TE3_LIU_LB_ANALOG,	"LB_ANALOG"     },
        { WAN_TE3_LIU_LB_REMOTE,	"LB_REMOTE"     },
        { WAN_TE3_LIU_LB_DIGITAL,	"LB_DIGITAL"    },
	
	{ WANCONFIG_PPP,	"MP_PPP"	}, 
	{ WANCONFIG_CHDLC,	"MP_CHDLC"	},
	{ WANCONFIG_FR, 	"MP_FR"		},
	{ WANCONFIG_LAPB, 	"MP_LAPB"	},
	{ WANCONFIG_XDLC, 	"MP_XDLC"	},
	{ WANCONFIG_TTY, 	"MP_TTY"	},
	{ WANCONFIG_X25, 	"MP_X25"	},
	{ WANCONFIG_LIP_ATM, 	"MP_ATM"	},
	{ WANCONFIG_LIP_KATM, 	"MP_KATM"	},
	{ WANCONFIG_XMTP2, 	"MP_XMTP2"	},
	{ WANCONFIG_LAPD, 	"MP_LAPD"	},
	{ WANCONFIG_LIP_HDLC,	"MP_HDLC"	},
	{ WANOPT_NO,		"RAW"		},
	{ WANOPT_NO,		"HDLC"		},
	{ WANCONFIG_PPP,	"PPP"		}, 
	{ WANCONFIG_CHDLC,	"CHDLC"		},
	{ WANCONFIG_ASYHDLC,	"ASYHDLC"	},
	
	/*-------------SS7 options--------------*/
	{ WANOPT_SS7_ANSI,      "ANSI"          },
	{ WANOPT_SS7_ITU,       "ITU"           },
	{ WANOPT_SS7_NTT,       "NTT"           },

	{ WANOPT_SS7_FISU,	"FISU"		},
	{ WANOPT_SS7_LSSU,	"LSSU"		},

	{ WANOPT_SS7_MODE_128, 	"SS7_128"	},
	{ WANOPT_SS7_MODE_4096,	"SS7_4096"	},
	
	{ WANOPT_S50X,		"S50X"		},
	{ WANOPT_S51X,		"S51X"		},
	{ WANOPT_ADSL,		"ADSL"		},
	{ WANOPT_ADSL,		"S518"		},
	{ WANOPT_AFT,           "AFT"           },
	{ WANOPT_AFT,           "USB"           },
	{ WANOPT_AFT_GSM,       "AFT_GSM"       },

	/*-------------ADSL options--------------*/
	{ RFC_MODE_BRIDGED_ETH_LLC,	"ETH_LLC_OA" },
	{ RFC_MODE_BRIDGED_ETH_VC,	"ETH_VC_OA"  },
	{ RFC_MODE_ROUTED_IP_LLC,	"IP_LLC_OA"  },//ok
	{ RFC_MODE_ROUTED_IP_VC,	"IP_VC_OA"   },
	{ RFC_MODE_PPP_LLC,		"PPP_LLC_OA" },
	{ RFC_MODE_PPP_VC,		"PPP_VC_OA"  },
	{ RFC_MODE_STACK_VC,		"STACK_VC_OA"  },
	
	/* Backward compatible */
	{ RFC_MODE_BRIDGED_ETH_LLC,	"BLLC_OA" },
	{ RFC_MODE_ROUTED_IP_LLC,	"CIP_OA" },
	{ LAN_INTERFACE,		"LAN" },
	{ WAN_INTERFACE,		"WAN" },

        { WANOPT_ADSL_T1_413,                   "ADSL_T1_413"                   },
        { WANOPT_ADSL_G_LITE,                   "ADSL_G_LITE"                   },
	{ WANOPT_ADSL_G_DMT,                    "ADSL_G_DMT"                    },
	{ WANOPT_ADSL_ALCATEL_1_4,              "ADSL_ALCATEL_1_4"              },
	{ WANOPT_ADSL_ALCATEL,                  "ADSL_ALCATEL"                  },
	{ WANOPT_ADSL_MULTIMODE,                "ADSL_MULTIMODE"                },
	{ WANOPT_ADSL_T1_413_AUTO,              "ADSL_T1_413_AUTO"              },
	{ WANOPT_ADSL_TRELLIS_ENABLE,           "ADSL_TRELLIS_ENABLE"           },
	{ WANOPT_ADSL_TRELLIS_DISABLE,          "ADSL_TRELLIS_DISABLE"          },
	{ WANOPT_ADSL_TRELLIS_LITE_ONLY_DISABLE,"ADSL_TRELLIS_LITE_ONLY_DISABLE"},
	{ WANOPT_ADSL_0DB_CODING_GAIN,          "ADSL_0DB_CODING_GAIN"          },
	{ WANOPT_ADSL_1DB_CODING_GAIN,          "ADSL_1DB_CODING_GAIN"          },
	{ WANOPT_ADSL_2DB_CODING_GAIN,          "ADSL_2DB_CODING_GAIN"          },
	{ WANOPT_ADSL_3DB_CODING_GAIN,          "ADSL_3DB_CODING_GAIN"          },
	{ WANOPT_ADSL_4DB_CODING_GAIN,          "ADSL_4DB_CODING_GAIN"          },
	{ WANOPT_ADSL_5DB_CODING_GAIN,          "ADSL_5DB_CODING_GAIN"          },
	{ WANOPT_ADSL_6DB_CODING_GAIN,          "ADSL_6DB_CODING_GAIN"          },
	{ WANOPT_ADSL_7DB_CODING_GAIN,          "ADSL_7DB_CODING_GAIN"          },
	{ WANOPT_ADSL_AUTO_CODING_GAIN,         "ADSL_AUTO_CODING_GAIN"         },
	{ WANOPT_ADSL_RX_BIN_DISABLE,           "ADSL_RX_BIN_DISABLE"           },
	{ WANOPT_ADSL_RX_BIN_ENABLE,            "ADSL_RX_BIN_ENABLE"            },
	{ WANOPT_ADSL_FRAMING_TYPE_0,           "ADSL_FRAMING_TYPE_0"           },
        { WANOPT_ADSL_FRAMING_TYPE_1,           "ADSL_FRAMING_TYPE_1"           },
	{ WANOPT_ADSL_FRAMING_TYPE_2,           "ADSL_FRAMING_TYPE_2"           },
	{ WANOPT_ADSL_FRAMING_TYPE_3,           "ADSL_FRAMING_TYPE_3"           },
	{ WANOPT_ADSL_EXPANDED_EXCHANGE,        "ADSL_EXPANDED_EXCHANGE"        },
	{ WANOPT_ADSL_SHORT_EXCHANGE,           "ADSL_SHORT_EXCHANGE"           },
	{ WANOPT_ADSL_CLOCK_CRYSTAL,            "ADSL_CLOCK_CRYSTAL"            },
	{ WANOPT_ADSL_CLOCK_OSCILLATOR,         "ADSL_CLOCK_OSCILLATOR"         },

#if 0
	{ WANOPT_OCT_CHAN_OPERMODE_NORMAL,	"OCT_OP_MODE_NORMAL"		},
	{ WANOPT_OCT_CHAN_OPERMODE_POWERDOWN,	"OCT_OP_MODE_POWERDOWN"		},
	{ WANOPT_OCT_CHAN_OPERMODE_NO_ECHO,	"OCT_OP_MODE_NO_ECHO"		},

	{ WANOPT_OCT_CHAN_OPERMODE_NORMAL,	"OCT_OP_MODE_NORMAL"		},
	{ WANOPT_OCT_CHAN_OPERMODE_NORMAL,	"OCT_OP_MODE_NORMAL"		},
	
#endif
	{ IBM4680,	"IBM4680" },
	{ IBM4680,	"IBM4680" },
  	{ NCR2126,	"NCR2126" },
	{ NCR2127,	"NCR2127" },
	{ NCR1255,	"NCR1255" },
	{ NCR7000,	"NCR7000" },
	{ ICL,		"ICL" },

	
	// DSP_20
	/*------- DSP Options -----------------*/
	{ WANOPT_DSP_HPAD,      "HPAD"         	},
	{ WANOPT_DSP_TPAD,      "TPAD"         	},

	{ WANOPT_AUTO,  "AUTO" },
	{ WANOPT_MANUAL,"MANUAL" },

	/* XDLC Options -----------------------*/
	{ WANOPT_TWO_WAY_ALTERNATE, "TWO_WAY_ALTERNATE" },
	{ WANOPT_TWO_WAY_SIMULTANEOUS, "TWO_WAY_SIMULTANEOUS" },
	{ WANOPT_PRI_DISC_ON_NO_RESP,     "PRI_DISC_ON_NO_RESP" },
	{ WANOPT_PRI_SNRM_ON_NO_RESP,	"PRI_SNRM_ON_NO_RESP" },

	{ WP_NONE,	"WP_NONE" },
	{ WP_SLINEAR,   "WP_SLINEAR" },
		
	{ WAN_TDMV_ALAW,	"ALAW" },
	{ WAN_TDMV_MULAW,	"MULAW" },
		
	{ WANOPT_SIM,	"SIMULATE" },

	{ WANOPT_OCT_CHAN_OPERMODE_NORMAL, "OCT_NORMAL" },
	{ WANOPT_OCT_CHAN_OPERMODE_SPEECH, "OCT_SPEECH" },
	{ WANOPT_OCT_CHAN_OPERMODE_NO_ECHO, "OCT_NO_ECHO" },

	{ WANOPT_HW_PORT_MAP_DEFAULT,"DEFAULT" },
	{ WANOPT_HW_PORT_MAP_LINEAR, "LINEAR" },
	
	/*----- End ---------------------------*/
	{ 0,			NULL		}, 
};


#if defined(__WINDOWS__)
#ifdef __cplusplus
}	/* for C++ users */
#endif
#endif

#endif/* WANCONFIG_H */
