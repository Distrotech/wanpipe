/******************************************************************************
 * wanec_iface_api.h
 *
 * Author: 	Alex Feldman  <al.feldman@sangoma.com>
 *
 * Copyright:	(c) 1995-2001 Sangoma Technologies Inc.
 *
 *		This program is free software; you can redistribute it and/or
 *		modify it under the terms of the GNU General Public License
 *		as published by the Free Software Foundation; either version
 *		2 of the License, or (at your option) any later version.
 * ============================================================================
 ******************************************************************************
 */

#ifndef __WANEC_IFACE_API_H
# define __WANEC_IFACE_API_H

#if defined(__WINDOWS__)
# if defined(__KERNEL__)
#  define _DEBUG
#  include <DebugOut.h>
# else
#  include <windows.h>
# endif
#endif

#include "wanpipe_includes.h"
#include "wanpipe_defines.h"
#include "wanpipe_cfg.h"
#include "wanec_iface.h"
#include "wanpipe_events.h"

#if defined(__WINDOWS__)
# include "oct6100_api.h"
#else
# include "oct6100api/oct6100_api.h"
#endif


#define	WANEC_BYDEFAULT_NORMAL

/* A delay, in seconds, between "is configuration complete" polls */
#define WANEC_API_CONFIG_POLL_DELAY	1	
/* Maximum number of polls. If firmware is not in "ready" state
 * after the maximum number of polls, then "timeout" error declared 
 * for HWEC chip initialization. */
#define WANEC_API_MAX_CONFIG_POLL	WANEC_API_CONFIG_POLL_DELAY * 100

#define WANEC_DEV_DIR			"/dev/"
#define WANEC_DEV_NAME			"wanec"

/* WANPIPE EC API return code */
#define WAN_EC_API_RC_OK		0x0000
#define WAN_EC_API_RC_FAILED		0x0001
#define WAN_EC_API_RC_INVALID_CMD	0x0002
#define WAN_EC_API_RC_INVALID_STATE	0x0003
#define WAN_EC_API_RC_INVALID_DEV	0x0004
#define WAN_EC_API_RC_INVALID_CHANNELS	0x0005
#define WAN_EC_API_RC_BUSY		0x0006
#define WAN_EC_API_RC_NOACTION		0x0007
#define WAN_EC_API_RC_INVALID_PORT	0x0008
#define WAN_EC_API_RC_DECODE(err)					\
	(err == WAN_EC_API_RC_OK) 		? "OK" :		\
	(err == WAN_EC_API_RC_FAILED) 		? "Failed" :		\
	(err == WAN_EC_API_RC_INVALID_CMD) 	? "Invalid Cmd" :	\
	(err == WAN_EC_API_RC_INVALID_STATE) 	? "Invalid State" :	\
	(err == WAN_EC_API_RC_INVALID_DEV) 	? "Invalid Device" :	\
	(err == WAN_EC_API_RC_INVALID_CHANNELS) ? "Invalid Channels" :	\
	(err == WAN_EC_API_RC_BUSY) 		? "Busy" :		\
	(err == WAN_EC_API_RC_NOACTION) 	? "No action" :		\
	(err == WAN_EC_API_RC_INVALID_PORT)     ? "Invalid Port" :	\
							"Unknown RC"

#if defined(__WINDOWS__)
# define WAN_EC_API_CMD_NONE			0
# define WAN_EC_API_CMD_GETINFO			1
# define WAN_EC_API_CMD_CONFIG			2
# define WAN_EC_API_CMD_CHANNEL_OPEN		3
# define WAN_EC_API_CMD_RELEASE			4
# define WAN_EC_API_CMD_ENABLE			5
# define WAN_EC_API_CMD_DISABLE			6
# define WAN_EC_API_CMD_BYPASS_ENABLE		7
# define WAN_EC_API_CMD_BYPASS_DISABLE		8
# define WAN_EC_API_CMD_OPMODE			9
# define WAN_EC_API_CMD_MODIFY_CHANNEL		10
# define WAN_EC_API_CMD_TONE_ENABLE		11
# define WAN_EC_API_CMD_TONE_DISABLE		12
# define WAN_EC_API_CMD_STATS			13
# define WAN_EC_API_CMD_STATS_FULL		14
# define WAN_EC_API_CMD_STATS_IMAGE		15
# define WAN_EC_API_CMD_BUFFER_LOAD		16
# define WAN_EC_API_CMD_BUFFER_UNLOAD		17
# define WAN_EC_API_CMD_PLAYOUT_START		18
# define WAN_EC_API_CMD_PLAYOUT_STOP		19
# define WAN_EC_API_CMD_MONITOR			20
# define WAN_EC_API_CMD_RELEASE_ALL		21
# define WAN_EC_API_CMD_CONFIG_POLL		22
# define WAN_EC_API_CMD_CHANNEL_MUTE		23
# define WAN_EC_API_CMD_CHANNEL_UNMUTE		24
# define WAN_EC_API_CMD_HWDTMF_REMOVAL_ENABLE	25
# define WAN_EC_API_CMD_HWDTMF_REMOVAL_DISABLE	26
#else
# define WAN_EC_API_CMD_NONE			_IOWR('E', 0, struct wan_ec_api_)
# define WAN_EC_API_CMD_GETINFO			_IOWR('E', 1, wan_ec_api_t)
# define WAN_EC_API_CMD_CONFIG			_IOWR('E', 2, struct wan_ec_api_)
# define WAN_EC_API_CMD_CHANNEL_OPEN		_IOWR('E', 3, struct wan_ec_api_)
# define WAN_EC_API_CMD_RELEASE			_IOWR('E', 4, struct wan_ec_api_)
# define WAN_EC_API_CMD_ENABLE			_IOWR('E', 5, struct wan_ec_api_)
# define WAN_EC_API_CMD_DISABLE			_IOWR('E', 6, struct wan_ec_api_)
# define WAN_EC_API_CMD_BYPASS_ENABLE		_IOWR('E', 7, struct wan_ec_api_)
# define WAN_EC_API_CMD_BYPASS_DISABLE		_IOWR('E', 8, struct wan_ec_api_)
# define WAN_EC_API_CMD_OPMODE			_IOWR('E', 9, struct wan_ec_api_)
# define WAN_EC_API_CMD_MODIFY_CHANNEL		_IOWR('E', 10, struct wan_ec_api_)
# define WAN_EC_API_CMD_TONE_ENABLE		_IOWR('E', 11, struct wan_ec_api_)
# define WAN_EC_API_CMD_TONE_DISABLE		_IOWR('E', 12, struct wan_ec_api_)
# define WAN_EC_API_CMD_STATS			_IOWR('E', 13, struct wan_ec_api_)
# define WAN_EC_API_CMD_STATS_FULL		_IOWR('E', 14, struct wan_ec_api_)
# define WAN_EC_API_CMD_STATS_IMAGE		_IOWR('E', 15, struct wan_ec_api_)
# define WAN_EC_API_CMD_BUFFER_LOAD		_IOWR('E', 16, struct wan_ec_api_)
# define WAN_EC_API_CMD_BUFFER_UNLOAD		_IOWR('E', 17, struct wan_ec_api_)
# define WAN_EC_API_CMD_PLAYOUT_START		_IOWR('E', 18, struct wan_ec_api_)
# define WAN_EC_API_CMD_PLAYOUT_STOP		_IOWR('E', 19, struct wan_ec_api_)
# define WAN_EC_API_CMD_MONITOR			_IOWR('E', 20, struct wan_ec_api_)
# define WAN_EC_API_CMD_RELEASE_ALL		_IOWR('E', 21, struct wan_ec_api_)
# define WAN_EC_API_CMD_CONFIG_POLL		_IOWR('E', 22, struct wan_ec_api_)
# define WAN_EC_API_CMD_CHANNEL_MUTE		_IOWR('E', 23, struct wan_ec_api_)
# define WAN_EC_API_CMD_CHANNEL_UNMUTE		_IOWR('E', 24, struct wan_ec_api_)
# define WAN_EC_API_CMD_HWDTMF_REMOVAL_ENABLE	_IOWR('E', 25, struct wan_ec_api_)
# define WAN_EC_API_CMD_HWDTMF_REMOVAL_DISABLE	_IOWR('E', 26, struct wan_ec_api_)
#endif

# define WAN_EC_API_CMD_DECODE(cmd)					\
	(cmd == WAN_EC_API_CMD_GETINFO)		? "Get Info" :		\
	(cmd == WAN_EC_API_CMD_CONFIG)		? "Config" :		\
	(cmd == WAN_EC_API_CMD_CONFIG_POLL)	? "Config Poll" :	\
	(cmd == WAN_EC_API_CMD_CHANNEL_OPEN)	? "Channel Open" :	\
	(cmd == WAN_EC_API_CMD_ENABLE)		? "Enable" :		\
	(cmd == WAN_EC_API_CMD_DISABLE)		? "Disable" :		\
	(cmd == WAN_EC_API_CMD_BYPASS_ENABLE)	? "Enable bypass" :	\
	(cmd == WAN_EC_API_CMD_BYPASS_DISABLE)	? "Disable bypass" :	\
	(cmd == WAN_EC_API_CMD_OPMODE)		? "Modify EC OPMODE" :	\
	(cmd == WAN_EC_API_CMD_STATS)		? "Get stats" :		\
	(cmd == WAN_EC_API_CMD_STATS_FULL)	? "Get stats" :		\
	(cmd == WAN_EC_API_CMD_STATS_IMAGE)	? "Get Image stats" :		\
	(cmd == WAN_EC_API_CMD_BUFFER_LOAD)	? "Buffer load" :		\
	(cmd == WAN_EC_API_CMD_BUFFER_UNLOAD)	? "Buffer unload" :	\
	(cmd == WAN_EC_API_CMD_PLAYOUT_START)	? "Playout start" :	\
	(cmd == WAN_EC_API_CMD_PLAYOUT_STOP)	? "Playout stop" :	\
	(cmd == WAN_EC_API_CMD_RELEASE)		? "Release" :		\
	(cmd == WAN_EC_API_CMD_RELEASE_ALL)		? "Release all" :	\
	(cmd == WAN_EC_API_CMD_MONITOR)		? "MONITOR" :		\
	(cmd == WAN_EC_API_CMD_MODIFY_CHANNEL)	? "MODIFY" :		\
	(cmd == WAN_EC_API_CMD_TONE_ENABLE)		? "Enable TONE" :	\
	(cmd == WAN_EC_API_CMD_TONE_DISABLE)		? "Disable TONE" :	\
	(cmd == WAN_EC_API_CMD_CHANNEL_MUTE)	? "Channel Mute" :	\
	(cmd == WAN_EC_API_CMD_CHANNEL_UNMUTE)	? "Channel Un-mute" :	\
	(cmd == WAN_EC_API_CMD_HWDTMF_REMOVAL_ENABLE)	? "DTMF Removal enable" :	\
	(cmd == WAN_EC_API_CMD_HWDTMF_REMOVAL_DISABLE)	? "DTMF Removal disable" :	\
					"Unknown"

#define WAN_

#pragma pack(1)

typedef struct wan_ec_api_ {
	char		devname[WAN_DRVNAME_SZ+1];
	unsigned long	cmd;
#if defined(__GNUC__) && !defined(__x86_64__)
	unsigned int reserved_cmd;	/* configuration structure identifier */
#endif
	unsigned int	type;
	int		err;
	int		verbose;
	int		state;

	int		fe_chan;
	unsigned long	fe_chan_map;
#if defined(__GNUC__) && !defined(__x86_64__)
	unsigned int reserved_fe_chan_map;	/* configuration structure identifier */
#endif

	union {
#define u_info		u_ec.info
#define u_config	u_ec.config
#define u_config_poll	u_ec.config_poll
#define u_chip_stats	u_ec.chip_stats
#define u_chip_image	u_ec.chip_image
#define u_chan_opmode	u_ec.chan_opmode
#define u_chan_mute	u_ec.chan_mute
#define u_chan_custom	u_ec.chan_custom
#define u_chan_stats	u_ec.chan_stats
#define u_chan_monitor	u_ec.chan_monitor
#define u_buffer_config	u_ec.buffer_config
#define u_playout	u_ec.playout
#define u_tone_config	u_ec.tone_config
		struct info_ {
			u_int16_t	max_channels;
		} info;
		wanec_config_t		config;
		wanec_config_poll_t	config_poll;
		wanec_chip_stats_t	chip_stats;
		wanec_chip_image_t	chip_image;
		wanec_chan_opmode_t	chan_opmode;
		wanec_chan_mute_t	chan_mute;
		wanec_chan_custom_t	chan_custom;
		wanec_chan_stats_t	chan_stats;
		wanec_chan_monitor_t	chan_monitor;
		wanec_buffer_config_t	buffer_config;
		wanec_playout_t		playout;
		wanec_tone_config_t	tone_config;
	} u_ec;

	wan_custom_conf_t	custom_conf;
} wan_ec_api_t;

#pragma pack()

#endif /* __WANEC_IFACE_API_H */
