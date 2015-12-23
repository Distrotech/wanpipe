/******************************************************************************
 * wanec_iface.h
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

#ifndef __WANEC_IFACE_H
# define __WANEC_IFACE_H

#if defined(__WINDOWS__)
# if defined(__KERNEL__)
# define _DEBUG
# include <DebugOut.h>
# else
#  include <windows.h>
# endif
#endif

#include "wanpipe_includes.h"
#include "wanpipe_defines.h"
#include "wanpipe_cfg.h"
#include "wanpipe_events.h"

#if defined(__WINDOWS__)
# include "oct6100_api.h"
#else
# include "oct6100api/oct6100_api.h"
#endif

#define WAN_EC_VERBOSE_NONE		0x00
#define WAN_EC_VERBOSE_EXTRA1		0x01
#define WAN_EC_VERBOSE_MASK_EXTRA1	0x01
#define WAN_EC_VERBOSE_EXTRA2		0x02
#define WAN_EC_VERBOSE_MASK_EXTRA2	0x03
#define WAN_EC_VERBOSE(ec_api)	(ec_api)?ec_api->verbose:WAN_EC_VERBOSE_NONE

#define WAN_EC_RC_OK				0x0000
#define WAN_EC_RC_CPU_INTERFACE_NO_RESPONSE	0x0001
#define WAN_EC_RC_MEMORY			0x0002


/* Internal EC chip state machine */
enum {
	WAN_EC_STATE_NONE = 0x00,
	WAN_EC_STATE_RESET,
	WAN_EC_STATE_READY,
	WAN_EC_STATE_CHIP_OPEN_PENDING,
	WAN_EC_STATE_CHIP_OPEN,
	WAN_EC_STATE_CHAN_READY
};
#define WAN_EC_STATE_DECODE(state)						\
	(state == WAN_EC_STATE_RESET)			? "Reset" :		\
	(state == WAN_EC_STATE_READY)			? "Ready" :		\
	(state == WAN_EC_STATE_CHIP_OPEN_PENDING)	? "Chip Open Pending" :	\
	(state == WAN_EC_STATE_CHIP_OPEN)		? "Chip Open" :		\
	(state == WAN_EC_STATE_CHAN_READY)		? "Channels Ready" :	\
					"Unknown"

#define MAX_EC_CHANS		256

#define WAN_NUM_DTMF_TONES	16
#define WAN_NUM_PLAYOUT_TONES	16
#define WAN_MAX_TONE_LEN	100

#pragma pack(1)

typedef struct wanec_config_
{
	u_int16_t		max_channels;
	int			memory_chip_size;
	UINT32			debug_data_mode;
	UINT8 *WP_POINTER_64	imageData;
#if defined(__GNUC__) && !defined(__x86_64__)
    unsigned int reserved;  
#endif

	UINT32			imageSize;
	int			imageLast;

	wan_custom_conf_t	custom_conf;
} wanec_config_t;

typedef struct wanec_config_poll_
{
	int			cnt;
} wanec_config_poll_t;

typedef struct wanec_chip_stats_
{
	int				reset;
	tOCT6100_CHIP_STATS		f_ChipStats;
} wanec_chip_stats_t;

typedef struct wanec_chip_image_
{
	tOCT6100_CHIP_IMAGE_INFO *WP_POINTER_64 f_ChipImageInfo;
#if defined(__GNUC__) && !defined(__x86_64__)
    unsigned int reserved;  
#endif
} wanec_chip_image_t;

typedef struct wanec_chan_opmode_
{
	UINT32			opmode;
} wanec_chan_opmode_t;

typedef struct wanec_chan_mute_
{
	unsigned char		port_map;

} wanec_chan_mute_t;

typedef struct wanec_chan_custom_
{
	int			custom;
	wan_custom_conf_t	custom_conf;
} wanec_chan_custom_t;

typedef struct wanec_chan_stats_
{
	int				reset;
	tOCT6100_CHANNEL_STATS		f_ChannelStats;
} wanec_chan_stats_t;

#define MAX_MONITOR_DATA_LEN	1024
typedef struct wanec_chan_monitor_
{
	int			fe_chan;
	int			data_mode;
	UINT32			remain_len;
	UINT32			data_len;
	UINT32			max_len;
	UINT8			data[MAX_MONITOR_DATA_LEN+1];
} wanec_chan_monitor_t;

typedef struct wanec_buffer_config_
{
	UINT8	buffer[WAN_MAX_TONE_LEN];
	UINT8 *WP_POINTER_64	data;
#if defined(__GNUC__) && !defined(__x86_64__)
    unsigned int reserved;  
#endif
	UINT32	size;
	UINT32	pcmlaw;
	UINT32	buffer_index;		/* value return by ec */
} wanec_buffer_config_t;

#define MAX_EC_PLAYOUT_LEN	20
typedef struct wanec_playout_
{
	UINT32	index;
	UINT8	port;
	UINT32	duration;
	BOOL	repeat;
	UINT32	repeat_cnt;
	UINT32	buffer_length;
	INT32	gaindb;
	BOOL	notifyonstop;
	UINT32	user_event_id;

	CHAR	str[MAX_EC_PLAYOUT_LEN];
	UINT32	delay;
} wanec_playout_t;

typedef struct wanec_tone_config_
{
	u_int8_t	id;		/* Tone id */
	u_int8_t	port_map;	/* SOUT/ROUT */
	u_int8_t	type;		/* PRESENT or STOP */
} wanec_tone_config_t;


/*===========================================================================*\
  User process context - This would probably have to be defined elsewhere.
  This structure must be allocated by the calling process and the parameters
  should be defined prior to open the OCT6100 chip.
\*===========================================================================*/
typedef struct _OCTPCIDRV_USER_PROCESS_CONTEXT_
{
#if defined(WAN_KERNEL)
	/* Interface name to driver (copied by calling process) */
	unsigned char	devname[WAN_DRVNAME_SZ+1];
	/*unsigned char	ifname[WAN_IFNAME_SZ+1];*/

	/* Board index. */
	unsigned int	ulBoardId;

	void		*ec_dev;
#else
	/* Is main process */
	unsigned int	fMainProcess;

	/* Handle to driver (opened by calling process) */
	void *ec_dev;

	/* Interface name to driver (copied by calling process) */
	unsigned char	devname[WAN_DRVNAME_SZ+1];
	unsigned char	ifname[WAN_IFNAME_SZ+1];

	/* Handle to serialization object used for read and writes */
	unsigned int 	ulUserReadWriteSerObj;

	/* Board index. */
	unsigned int	ulBoardId;

	/* Board type. */
	unsigned int	ulBoardType;
#endif
} tOCTPCIDRV_USER_PROCESS_CONTEXT, *tPOCTPCIDRV_USER_PROCESS_CONTEXT;

#pragma pack()


#if defined(WAN_KERNEL)

#if defined(__WINDOWS__)
# define PRINT1(v,...)	\
	if (v & WAN_EC_VERBOSE_EXTRA1) DEBUG_EVENT(## __VA_ARGS__)
# define PRINT2(v,...)	\
	if (v & WAN_EC_VERBOSE_EXTRA2) DEBUG_EVENT(## __VA_ARGS__)
#else
# define PRINT1(v,format,msg...)	\
	if (v & WAN_EC_VERBOSE_EXTRA1) DEBUG_EVENT(format,##msg)
# define PRINT2(v,format,msg...)	\
	if (v & WAN_EC_VERBOSE_EXTRA2) DEBUG_EVENT(format,##msg)
#endif


#define WANEC_IGNORE	(TRUE+1)

#define	WANEC_BYDEFAULT_NORMAL

/* Critical bit map */
#define WAN_EC_BIT_TIMER_RUNNING	1
#define WAN_EC_BIT_TIMER_KILL 		2
#define WAN_EC_BIT_CRIT_CMD 		3
#define WAN_EC_BIT_CRIT_DOWN 		4
#define WAN_EC_BIT_CRIT_ERROR 	       	5
#define WAN_EC_BIT_CRIT 	       	6

#define WAN_EC_BIT_EVENT_TONE 		4
#define WAN_EC_BIT_EVENT_PLAYOUT	5

#define WAN_EC_POLL_NONE		0x00
#define WAN_EC_POLL_INTR		0x01
#define WAN_EC_POLL_CHIPOPENPENDING	0x02
#define WAN_EC_POLL_DTMF_MUTE_ON	0x03
#define WAN_EC_POLL_DTMF_MUTE_OFF	0x04

#pragma pack(1)

typedef
struct wan_ec_confbridge_
{
	UINT32	ulHndl;

	WAN_LIST_ENTRY(wan_ec_confbridge_)	next;
} wan_ec_confbridge_t;


struct wan_ec_;
typedef struct wan_ec_dev_
{
	char		*name;
	char		devname[WAN_DRVNAME_SZ+1];
	char		ecdev_name[WAN_DRVNAME_SZ+1];
	int			ecdev_no;
	sdla_t		*card;

	u_int8_t	fe_media;
	u_int32_t	fe_lineno;
	int			fe_start_chan, fe_stop_chan;
	int			fe_max_chans;
	u_int32_t	fe_channel_map;
	u_int32_t   fe_dtmf_removal_map; /* Bitmap of channels that have dtmf removal enabled already */
	u_int32_t	fe_ec_map;
	u_int32_t	fe_tdmv_law;
	u_int32_t	channel;
	int			state;

	u_int32_t	critical;
	wan_timer_t	timer;

	u_int8_t	poll_cmd;
	int	   		poll_channel;

	u_int32_t	events;			/* enable events map */

	wan_hwec_dev_state_t ecdev_state;

	struct wan_ec_	*ec;
	WAN_LIST_ENTRY(wan_ec_dev_)	next;

#if 0
	/* Not using fax debouncing for now */
   	wan_ticks_t	fax_detect_timeout;
	u_int16_t	fax_detect_cnt;
#endif

} wan_ec_dev_t;

typedef struct wan_ec_
{
	char		name[WAN_DRVNAME_SZ+1];
	int		usage;
	int		chip_no;
	int		state;
	int		ec_active;
	u_int16_t	max_ec_chans;		/* max number of ec channels (security) */
	int		confbridges_no;		/* number of opened conf bridges */
	void		*ec_dev;
	u_int32_t	intcount;
	u_int32_t	critical;

	int		ignore_H100;		/* Temporary for BRI card */

	wan_mutexlock_t	lock;
	u_int32_t	events;			/* enable events map */
	int		tone_verbose;		/* verbose mode for tone events */
	int		playout_verbose;	/* verbose mode for playout events */
	wan_ticks_t	lastint_ticks;

	PUINT8				pImageData;
	UINT32				ImageSize;
	int				imageLast;
	wan_custom_conf_t		custom_conf;
	tOCT6100_CHIP_OPEN		f_OpenChip;

	tOCTPCIDRV_USER_PROCESS_CONTEXT	f_Context;
	tPOCT6100_INSTANCE_API		pChipInstance;
	tOCT6100_INTERRUPT_FLAGS	f_InterruptFlag;
	UINT32				*pEchoChannelHndl;
	PUINT32				pToneBufferIndexes;
	UINT32				ulDebugChannelHndl;
	INT				DebugChannel;
	UINT32				ulDebugDataMode;
	struct wan_ec_dev_		**pEcDevMap;
	WAN_LIST_HEAD(wan_ec_dev_head_, wan_ec_dev_)			ec_dev_head;
	WAN_LIST_HEAD(wan_ec_confbridge_head_, wan_ec_confbridge_)	ec_confbridge_head;
	WAN_LIST_ENTRY(wan_ec_)				next;
} wan_ec_t;

#pragma pack()

#if 0
typedef struct wanec_lip_reg
{
	unsigned long init;

	void* (*reg) 	(void*, int);
	int (*unreg)	(void*, void*);

	int (*ioctl)	(void);
	int (*isr)	(void);

}wanec_lip_reg_t;
#endif

static __inline int wan_ec_update_and_check(wan_ec_t *ec, int enable)
{
        if (!enable) {
                if (ec->ec_active > 0){
                        ec->ec_active--;
                }
                return 0;
        }

        if (ec->ec_active >= ec->max_ec_chans) {
                return -EINVAL;
        }

        ec->ec_active++;

        return 0;
}

/* global interface functions */
void *wan_ec_config (void *pcard, int max_channels);
int wan_ec_remove(void*, void *pcard);
int wan_ec_ioctl(void*, struct ifreq*, void *pcard);
int wan_ec_dev_ioctl(void*, void *pcard);

u32 wan_ec_req_write(void*, u32 write_addr, u16 write_data);
u32 wan_ec_req_write_smear(void*, u32 addr, u16 data, u32 len);
u32 wan_ec_req_write_burst(void*, u32 addr, u16 *data, u32 len);
u32 wan_ec_req_read(void*, u32 addr, u16 *data);
u32 wan_ec_req_read_burst(void*, u32 addr, u16 *data, u32 len);

#endif

#endif /* __WANEC_IFACE_H */
