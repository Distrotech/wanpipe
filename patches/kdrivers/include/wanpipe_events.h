/******************************************************************************
 * wanpipe_events.h	
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

#ifndef __WANPIPE_EVENTS_H__
# define __WANPIPE_EVENTS_H__

/* DTMF event tone type: present or stop */
#define WAN_EC_TONE_PRESENT		0x01
#define WAN_EC_TONE_STOP		0x02
#define WAN_EC_DECODE_TONE_TYPE(type)					\
		(type == WAN_EC_TONE_PRESENT)	? "Present"	:	\
		(type == WAN_EC_TONE_STOP)	? "Stop" 	:	\
							"Unknown"

/* pcm law type (alaw or ulaw) */
#define WAN_EC_PCM_U_LAW		0x01
#define WAN_EC_PCM_A_LAW		0x02
#define WAN_EC_DECODE_PCM_LAW(pcmlaw)				\
		((pcmlaw) == WAN_EC_PCM_U_LAW)	? "ULAW" :	\
		((pcmlaw) == WAN_EC_PCM_A_LAW)	? "ALAW"  : "Unknown"

/* channel port (sout, rout, sin, rin) */
#define WAN_EC_CHANNEL_PORT_SOUT		0x01
#define WAN_EC_CHANNEL_PORT_SIN			0x02
#define WAN_EC_CHANNEL_PORT_ROUT		0x04
#define WAN_EC_CHANNEL_PORT_RIN			0x08
#define WAN_EC_DECODE_CHANNEL_PORT(port)				\
		((port) == WAN_EC_CHANNEL_PORT_SOUT)	? "SOUT" :	\
		((port) == WAN_EC_CHANNEL_PORT_SIN)	? "SIN"  :	\
		((port) == WAN_EC_CHANNEL_PORT_ROUT)	? "ROUT" :	\
		((port) == WAN_EC_CHANNEL_PORT_RIN)	? "RIN"  : "Unknown"

#define WAN_EVENT_RXHOOK_OFF			0x01
#define WAN_EVENT_RXHOOK_ON				0x02
#define WAN_EVENT_RXHOOK_FLASH		0x03
#define WAN_EVENT_RXHOOK_DECODE(hook)					\
		((hook) == WAN_EVENT_RXHOOK_OFF) ? "Off-hook" :		\
		((hook) == WAN_EVENT_RXHOOK_ON)  ? "On-hook" :		\
		((hook) == WAN_EVENT_RXHOOK_FLASH)  ? "Flash-hook" :		\
							"Unknown"

#define WAN_EVENT_RING_PRESENT		0x01
#define WAN_EVENT_RING_STOP		0x02
#define WAN_EVENT_RING_DECODE(ring)					\
		((ring) == WAN_EVENT_RING_PRESENT) ? "Ring Present" :	\
		((ring) == WAN_EVENT_RING_STOP)	   ? "Ring Stop" :	\
							"Unknown"
#define WAN_EVENT_RING_TRIP_PRESENT		0x01
#define WAN_EVENT_RING_TRIP_STOP		0x02
#define WAN_EVENT_RING_TRIP_DECODE(ring)					\
		((ring) == WAN_EVENT_RING_TRIP_PRESENT) ? "RingTrip Present" :	\
		((ring) == WAN_EVENT_RING_TRIP_STOP)	? "RingTrip Stop" :	\
							"Unknown"
/*Link Status */
#define WAN_EVENT_LINK_STATUS_CONNECTED		0x01
#define WAN_EVENT_LINK_STATUS_DISCONNECTED	0x02
#define WAN_EVENT_LINK_STATUS_DECODE(status)					\
		((status) == WAN_EVENT_LINK_STATUS_CONNECTED) ? "Connected" :		\
		((status) == WAN_EVENT_LINK_STATUS_DISCONNECTED)  ? "Disconnected" :		\
							"Unknown"

/*FXO polarity Reversal */
#define WAN_EVENT_POLARITY_REV_POSITIVE_NEGATIVE	0x01
#define WAN_EVENT_POLARITY_REV_NEGATIVE_POSITIVE	0x02
#define WAN_EVENT_POLARITY_REV_DECODE(status)					\
		((status) == WAN_EVENT_POLARITY_REV_POSITIVE_NEGATIVE) ? "Positive to Negative" :		\
		((status) == WAN_EVENT_POLARITY_REV_NEGATIVE_POSITIVE)  ? "Negative to Positive" :		\
							"Unknown"
	
#if defined(WAN_KERNEL)

#include "wanpipe_debug.h"
#include "wanpipe_defines.h"
#include "wanpipe_common.h"

/* Global Event defines 			*/
#define WAN_EVENT_ENABLE	0x01
#define WAN_EVENT_DISABLE	0x02
#define WAN_EVENT_MODE_DECODE(mode)					\
		((mode) == WAN_EVENT_ENABLE) ? "Enable" :		\
		((mode) == WAN_EVENT_DISABLE) ? "Disable" :		\
						"(Unknown mode)"

/* Event type list */
enum {
	WAN_EVENT_EC_DTMF			= 1, 	/* WAN_EVENT_EC_TONE_DTMF */
	WAN_EVENT_RM_POWER			,	
	WAN_EVENT_RM_LC				,
	WAN_EVENT_RM_RING_TRIP		,
	WAN_EVENT_RM_DTMF			,
	WAN_EVENT_TE_RBS			,
	WAN_EVENT_RM_RING			,
	WAN_EVENT_RM_TONE			,
	WAN_EVENT_RM_RING_DETECT	,
	WAN_EVENT_RM_TXSIG_START	,
	WAN_EVENT_RM_TXSIG_OFFHOOK	,
	WAN_EVENT_RM_TXSIG_ONHOOK	,
	WAN_EVENT_RM_TXSIG_KEWL		,
	WAN_EVENT_RM_ONHOOKTRANSFER	,
	WAN_EVENT_RM_SETPOLARITY	,
	WAN_EVENT_RM_SET_ECHOTUNE	,
	WAN_EVENT_EC_CHAN_MODIFY	,
	WAN_EVENT_EC_H100_REPORT	,
	WAN_EVENT_BRI_CHAN_LOOPBACK	,
	WAN_EVENT_LINK_STATUS		,
	WAN_EVENT_RM_POLARITY_REVERSE, 	
	WAN_EVENT_EC_FAX_DETECT		,
	WAN_EVENT_RM_SET_TX_GAIN	,
	WAN_EVENT_RM_SET_RX_GAIN	,
	WAN_EVENT_EC_FAX_1100		,
	WAN_EVENT_EC_FAX_2100		,
	WAN_EVENT_EC_FAX_2100_WSPR		
};
	

#define WAN_EVENT_TYPE_DECODE(type)					\
		((type) == WAN_EVENT_EC_DTMF)		? "EC DTMF"  :	\
		((type) == WAN_EVENT_RM_POWER)		? "RM Power Alarm" :	\
		((type) == WAN_EVENT_RM_LC)		? "RM Loop Closure" :	\
		((type) == WAN_EVENT_RM_RING_TRIP)	? "RM Ring Trip" :	\
		((type) == WAN_EVENT_RM_DTMF)		? "RM DTMF" :		\
		((type) == WAN_EVENT_TE_RBS)		? "TE RBS" :		\
		((type) == WAN_EVENT_RM_RING)		? "RM Ring" :		\
		((type) == WAN_EVENT_RM_TONE)		? "RM Tone" :		\
		((type) == WAN_EVENT_RM_RING_DETECT)	? "RM Ring Detect" :	\
		((type) == WAN_EVENT_RM_TXSIG_START)	? "RM TXSIG Start" :	\
		((type) == WAN_EVENT_RM_TXSIG_OFFHOOK)	? "RM TXSIG Off-hook" :	\
		((type) == WAN_EVENT_RM_TXSIG_ONHOOK)	? "RM TXSIG On-hook" :	\
		((type) == WAN_EVENT_RM_TXSIG_KEWL)	? "RM TXSIG kewlfs" :	\
		((type) == WAN_EVENT_RM_ONHOOKTRANSFER)	? "RM On-hook transfer" :	\
		((type) == WAN_EVENT_RM_SETPOLARITY)	? "RM Set polarity" :	\
		((type) == WAN_EVENT_RM_SET_ECHOTUNE)	? "RM Set echotune" :	\
		((type) == WAN_EVENT_EC_CHAN_MODIFY)	? "EC Chan Modify" :	\
		((type) == WAN_EVENT_BRI_CHAN_LOOPBACK)	? "BRI B-Chan Loopback" :	\
		((type) == WAN_EVENT_LINK_STATUS)	? "Link Status" :	\
		((type) == WAN_EVENT_RM_POLARITY_REVERSE)	? "RM Polarity Reverse" :	\
		((type) == WAN_EVENT_EC_FAX_DETECT)	? "EC FAX Detect" :	\
		((type) == WAN_EVENT_RM_SET_TX_GAIN)	? "RM Set Tx Gain" :	\
		((type) == WAN_EVENT_RM_SET_TX_GAIN)	? "RM Set Rx Gain" :	\
		((type) == WAN_EVENT_EC_FAX_1100)	? "EC FAX 1100" :	\
		((type) == WAN_EVENT_EC_FAX_2100)	? "EC FAX 2100" :	\
		((type) == WAN_EVENT_EC_FAX_2100_WSPR)	? "EC FAX 2100 WSPR" :	\
							"(Unknown type)"

/* tone type list */						
#define	WAN_EVENT_RM_TONE_TYPE_DIAL		0x01
#define	WAN_EVENT_RM_TONE_TYPE_BUSY		0x02
#define	WAN_EVENT_RM_TONE_TYPE_RING		0x03
#define	WAN_EVENT_RM_TONE_TYPE_CONGESTION	0x04
#define WAN_EVENT_TONE_DECODE(tone)					\
		((tone) == WAN_EVENT_RM_TONE_TYPE_DIAL)		? "Dial tone" :	\
		((tone) == WAN_EVENT_RM_TONE_TYPE_BUSY)		? "Busy tone" :	\
		((tone) == WAN_EVENT_RM_TONE_TYPE_RING)		? "Ring tone" :	\
		((tone) == WAN_EVENT_RM_TONE_TYPE_CONGESTION)	? "Congestion tone" :	\
						"(Unknown tone)"

/* Event information			*/
typedef struct wan_event_
{
	u_int16_t	type;
	u_int8_t	mode;		/* Enable/Disable */
	u_int8_t	channel;	/* A200-mod_no, T1/E1-fe chan  */
	unsigned char	digit;		/* TONE: digit, 'f' -  for fax  */
	unsigned char	tone_type;	/* TONE: PRESETN/STOP */
	unsigned char	tone_port;	/* TONE: ROUT/SOUT */

	unsigned char	rxhook;		/* LC: OFF-HOOK or ON-HOOK */

	unsigned char	ring_mode;	/* RingDetect: Present/Stop */
	unsigned char	link_status;    /* Link Status */
	unsigned char   polarity_reverse; /*Polarity Reverse detection */
	unsigned int	alarms;
		
} wan_event_t;

/* Event control 			*/
typedef struct wan_event_ctrl_
{
	u_int16_t	type;
	u_int8_t	mode;
	int		mod_no;		/* A200-Remora */
	int		channel;
	unsigned char	ec_tone_port;	/* EC Tone: SOUT or ROUT */
	unsigned long	ts_map;
	u_int8_t	tone;
	int		ohttimer;	/* On-hook transfer */
	int		polarity;	/* SETPOLARITY */
	signed int	rm_gain; /* RM GAIN VALUE */
#if !defined(__WINDOWS__)
	WAN_LIST_ENTRY(wan_event_ctrl_)	next;
#endif
} wan_event_ctrl_t;

#endif	/* WAN_KERNEL */

#endif	/* __WANPIPE_EVENTS_H__ */
