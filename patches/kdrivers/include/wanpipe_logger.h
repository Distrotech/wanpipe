/******************************************************************************//**
 * \file wanpipe_logger.h
 * \brief WANPIPE(tm) Wanpipe Logger API Headers and Defines
 *
 * Authors: David Rokhvarg <davidr@sangoma.com>
 *
 * Copyright (c) 2007 - 09, Sangoma Technologies
 * All rights reserved.
 *
 * * Redistribution and use in source and binary forms, with or without
 *   modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the Sangoma Technologies nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY Sangoma Technologies ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL Sangoma Technologies BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * ===============================================================================
 */

#ifndef __WANPIPE_LOGGER_API_HDR__
#define __WANPIPE_LOGGER_API_HDR__

#include "wanpipe_abstr_types.h"

/***********************************************//**
 Wanpipe Logger Event Structure
****************************************************/

/*!
  \def WP_MAX_NO_BYTES_IN_LOGGER_EVENT
  \brief Maximum length of event string
 */
#define WP_MAX_NO_BYTES_IN_LOGGER_EVENT	256

/*!
  \def WP_MAX_NO_LOGGER_EVENTS
  \brief Maximum number of logger events kept by API driver
 */
#define WP_MAX_NO_LOGGER_EVENTS	512

/*!
 \struct wp_logger_event
 \brief Wanpipe API Logger Event contains event type,
		timestamp and data.
 
 \typedef wp_logger_event_t
 \brief Wanpipe API Logger Event contains event type,
		timestamp and data.
*/
typedef struct wp_logger_event{

	u_int32_t	logger_type;		/*!< Type of logger which produced the event. One of the types in wan_logger_type. */
	u_int32_t	event_type;			/*!< A bitmap indicating type of the event. Only a single bit will be set by API. */
	wan_time_t	time_stamp_sec;		/*!< Timestamp in seconds (Since January 1, 1970). */
	wan_suseconds_t	time_stamp_usec;/*!< microseconds portion of the timestamp */
	unsigned char	data[WP_MAX_NO_BYTES_IN_LOGGER_EVENT];	/*!< data area containing the logger event null-terminated string */

}wp_logger_event_t;


/* Definitions for 'logger_type' in wp_logger_event_t: */
enum wan_logger_type{
	WAN_LOGGER_DEFAULT, /* controled by bitmaps in wp_default_logger_level */
	WAN_LOGGER_TE1,		/* controled by bitmaps in wp_te1_logger_level */
	WAN_LOGGER_HWEC,	/* controled by bitmaps in wp_hwec_logger_level */
	WAN_LOGGER_TDMAPI,	/* controled by bitmaps in wp_tdmapi_logger_level */
	WAN_LOGGER_FE,		/* controled by bitmaps in wp_fe_logger_level */
	WAN_LOGGER_BRI,		/* controled by bitmaps in wp_bri_logger_level */
	WAN_LOGGER_FILE		/* controled by bitmaps in wp_file_logger_level */
};

#define SANG_DECODE_LOGGER_TYPE(wplg_type)	\
	(wplg_type == WAN_LOGGER_DEFAULT)	? "WAN_LOGGER_DEFAULT": \
	(wplg_type == WAN_LOGGER_TE1)	? "WAN_LOGGER_TE1": \
	(wplg_type == WAN_LOGGER_HWEC)	? "WAN_LOGGER_HWEC": \
	(wplg_type == WAN_LOGGER_TDMAPI)	? "WAN_LOGGER_TDMAPI": \
	(wplg_type == WAN_LOGGER_FE)	? "WAN_LOGGER_FE": \
	(wplg_type == WAN_LOGGER_BRI)	? "WAN_LOGGER_BRI": \
	(wplg_type == WAN_LOGGER_FILE)	? "WAN_LOGGER_FILE": \
	"Invalid Logger type"

/* Definitions for 'event_type' in wp_logger_event_t: */

/* General purpose Wanpipe Logger event types. */
enum wp_default_logger_level{
	SANG_LOGGER_INFORMATION	= (1),
	SANG_LOGGER_WARNING		= (1 << 1),
	SANG_LOGGER_ERROR		= (1 << 2)
};

#define SANG_DECODE_DEFAULT_LOGGER_EVENT_TYPE(bit)	\
	(bit & SANG_LOGGER_INFORMATION) ?	"SANG_LOGGER_INFORMATION":	\
	(bit & SANG_LOGGER_WARNING)		?	"SANG_LOGGER_WARNING"	:	\
	(bit & SANG_LOGGER_ERROR)		?	"SANG_LOGGER_ERROR"	: \
	"Invalid Bit for Default Logger"


/* Task-specific Wanpipe Logger event types - for advanced debugging only. */
/* T1/E1 */
enum wp_te1_logger_level{
	SANG_LOGGER_TE1_DEFAULT = (1)
};

#define SANG_DECODE_TE1_LOGGER_EVENT_TYPE(bit)	\
	(bit & SANG_LOGGER_TE1_DEFAULT) ?	"SANG_LOGGER_TE1_DEFAULT":	\
	"Invalid Bit for TE1 Logger"

/* HWEC */
enum wp_hwec_logger_level{
	SANG_LOGGER_HWEC_DEFAULT = (1)
};

#define SANG_DECODE_HWEC_LOGGER_EVENT_TYPE(bit)	\
	(bit & SANG_LOGGER_HWEC_DEFAULT) ?	"SANG_LOGGER_HWEC_DEFAULT":	\
	"Invalid Bit for HWEC Logger"


/* TDMAPI */
enum wp_tdmapi_logger_level{
	SANG_LOGGER_TDMAPI_DEFAULT = (1)
};

#define SANG_DECODE_TDMAPI_LOGGER_EVENT_TYPE(bit)	\
	(bit & SANG_LOGGER_TDMAPI_DEFAULT) ? "SANG_LOGGER_TDMAPI_DEFAULT":	\
	"Invalid Bit for TDMAPI Logger"

/* FE */
enum wp_fe_logger_level{
	SANG_LOGGER_FE_DEFAULT = (1)
};

#define SANG_DECODE_FE_LOGGER_EVENT_TYPE(bit)	\
	(bit & SANG_LOGGER_FE_DEFAULT) ? "SANG_LOGGER_FE_DEFAULT":	\
	"Invalid Bit for FE Logger"

/* BRI */
enum wp_bri_logger_level{
	SANG_LOGGER_BRI_DEFAULT = (1),
	SANG_LOGGER_BRI_HFC_S0_STATES = (1 << 1),
	SANG_LOGGER_BRI_L2_TO_L1_ACTIVATION = (1 << 2)
};

#define SANG_DECODE_BRI_LOGGER_EVENT_TYPE(bit)	\
	(bit & SANG_LOGGER_BRI_DEFAULT) ? "SANG_LOGGER_BRI_DEFAULT":	\
	(bit & SANG_LOGGER_BRI_HFC_S0_STATES) ?	"SANG_LOGGER_BRI_HFC_S0_STATES":	\
	(bit & SANG_LOGGER_BRI_L2_TO_L1_ACTIVATION) ? "SANG_LOGGER_BRI_L2_TO_L1_ACTIVATION":	\
	"Invalid Bit for BRI Logger"

/* File Logger. Controls printing of messages to Wanpipe Log file.
 * Does *not* produce any Events in Wanpipe Logger queue.
 * Windows: Windows\System32\drivers\wanpipelog.txt
 * Linux: /var/log/messages */
enum wp_file_logger_level{
	SANG_LOGGER_FILE_ON = (1),
	SANG_LOGGER_FILE_OFF = (1 << 1)
};

/* Note: WAN_LOGGER_FILE does not produce events. */
static __inline const char* wp_decode_logger_event_type(u_int32_t logger_type, u_int32_t evt_type)
{
	switch(logger_type)
	{
	case WAN_LOGGER_DEFAULT:
		return SANG_DECODE_DEFAULT_LOGGER_EVENT_TYPE(evt_type);
	case WAN_LOGGER_TE1:
		return SANG_DECODE_TE1_LOGGER_EVENT_TYPE(evt_type);
	case WAN_LOGGER_HWEC:
		return SANG_DECODE_HWEC_LOGGER_EVENT_TYPE(evt_type);
	case WAN_LOGGER_TDMAPI:
		return SANG_DECODE_TDMAPI_LOGGER_EVENT_TYPE(evt_type);
	case WAN_LOGGER_FE:
		return SANG_DECODE_FE_LOGGER_EVENT_TYPE(evt_type);
	case WAN_LOGGER_BRI:
		return SANG_DECODE_BRI_LOGGER_EVENT_TYPE(evt_type);
	default:
		return "Error: unknown logger type";
	}
}


/*!
  \struct wp_logger_stats
  \brief Wanpipe Logger API Statistics Structure. Used with WP_API_LOGGER_CMD_GET_STATS command.

  \typedef wp_logger_stats_t
 */
typedef struct wp_logger_stats
{
	ulong_t rx_events;			/*!< total Events received    */
	ulong_t rx_events_dropped;	/*!<  number of Events discarded because no free buffer available */
	ulong_t	max_event_queue_length;/*!<  maximum number of Events which can be stored in API queue */
	ulong_t	current_number_of_events_in_event_queue;/*!< number of Events currently in API queue */

}wp_logger_stats_t;

/*!
  \struct wp_logger_level_control
  \brief Wanpipe Logger API Levle Control Structure.
		Used with WP_API_LOGGER_CMD_GET_LOGGER_LEVEL and WP_API_LOGGER_CMD_SET_LOGGER_LEVEL commands.

  \typedef wp_logger_level_control_t
 */
typedef struct wp_logger_level_control
{
	u_int32_t	logger_type;	/*!< Type of logger which level is being controlled. One of the types in wan_logger_type. */
	u_int32_t	logger_level;	/*!< A bitmap indicating types of the events logged by Wanpipe Logger. */

}wp_logger_level_control_t;

/*!
  \struct wp_logger_cmd
  \brief Wanpipe Logger API Command Structure used with WANPIPE_IOCTL_LOGGER_CMD

  Wanpipe Logger API Command structure used to execute WANPIPE_IOCTL_LOGGER_CMD commands
  All commands are defined in: 
  enum wp_logger_cmds

  \typedef wp_logger_cmd_t
 */
typedef struct wp_logger_cmd
{
	unsigned int cmd;		/*!< Command defined in enum wp_logger_cmds */
	unsigned int result;	/*!< Result defined in: enum SANG_STATUS or SANG_STATUS_T */

	union {
		
		wp_logger_event_t logger_event;	

		wp_logger_level_control_t logger_level_ctrl; 

		u_int32_t	open_cnt;	/*!< Number of Open file descriptors for the device.
									Used with WP_API_LOGGER_CMD_OPEN_CNT command. */
		wp_logger_stats_t stats;
	};

}wp_logger_cmd_t;

/*!
  \enum wanpipe_api_cmds
  \brief Commands used with WANPIPE_IOCTL_LOGGER_CMD IOCTL
 */
enum wp_logger_cmds
{
	WP_API_LOGGER_CMD_FLUSH_BUFFERS,	/*!< Flush Buffers */
	WP_API_LOGGER_CMD_READ_EVENT,		/*!< Read Logger Event */

	WP_API_LOGGER_CMD_GET_STATS,		/*!< Get device statistics */
	WP_API_LOGGER_CMD_RESET_STATS,		/*!< Reset device statistics */
	WP_API_LOGGER_CMD_OPEN_CNT,			/*!< Get Number of Open file descriptors for the device */

	WP_API_LOGGER_CMD_GET_LOGGER_LEVEL,	/*!< Get current level (types of events) of Wanpipe Logger */
	WP_API_LOGGER_CMD_SET_LOGGER_LEVEL	/*!< Set current level (types of events) of Wanpipe Logger */
};


#if defined(__KERNEL__)

#if defined(__WINDOWS__)
# if defined(BUSENUM_DRV)
#  define WPLOGGER_CALL
# else
#  define WPLOGGER_CALL	DECLSPEC_IMPORT
# endif
#elif defined(__LINUX__)
# define WPLOGGER_CALL
# define EXTERN_C extern
#endif	/* __LINUX__ */

/* Wanpipe Logger functions and variables */
EXTERN_C WPLOGGER_CALL int wp_logger_create(void);
EXTERN_C WPLOGGER_CALL void wp_logger_delete(void);
EXTERN_C WPLOGGER_CALL void wp_logger_input(u_int32_t logger_type, u_int32_t evt_type, const char * fmt, ...);

EXTERN_C WPLOGGER_CALL u_int32_t wp_logger_level_default;
EXTERN_C WPLOGGER_CALL u_int32_t wp_logger_level_te1;
EXTERN_C WPLOGGER_CALL u_int32_t wp_logger_level_hwec;
EXTERN_C WPLOGGER_CALL u_int32_t wp_logger_level_tdmapi;
EXTERN_C WPLOGGER_CALL u_int32_t wp_logger_level_fe;
EXTERN_C WPLOGGER_CALL u_int32_t wp_logger_level_bri;

/* macros for checking if a level of debugging is enabled */
#define WAN_LOGGER_TEST_LEVEL_DEFAULT(level)	(level & wp_logger_level_default)
#define WAN_LOGGER_TEST_LEVEL_TE1(level)		(level & wp_logger_level_te1)
#define WAN_LOGGER_TEST_LEVEL_HWEC(level)		(level & wp_logger_level_hwec)
#define WAN_LOGGER_TEST_LEVEL_TDMAPI(level)		(level & wp_logger_level_tdmapi)
#define WAN_LOGGER_TEST_LEVEL_FE(level)			(level & wp_logger_level_fe)
#define WAN_LOGGER_TEST_LEVEL_BRI(level)		(level & wp_logger_level_bri)


#define WP_DEBUG(logger_type, logger_level, ...)	\
{	\
	char is_level_on = 0;	\
	\
	switch(logger_type)	\
	{	\
	case WAN_LOGGER_DEFAULT:	\
		is_level_on = WAN_LOGGER_TEST_LEVEL_DEFAULT(logger_level);	\
		break;	\
	case WAN_LOGGER_TE1:	\
		is_level_on = WAN_LOGGER_TEST_LEVEL_TE1(logger_level);	\
		break;	\
	case WAN_LOGGER_HWEC:	\
		is_level_on = WAN_LOGGER_TEST_LEVEL_HWEC(logger_level);	\
		break;	\
	case WAN_LOGGER_TDMAPI:	\
		is_level_on = WAN_LOGGER_TEST_LEVEL_TDMAPI(logger_level);	\
		break;	\
	case WAN_LOGGER_FE:	\
		is_level_on = WAN_LOGGER_TEST_LEVEL_FE(logger_level);	\
		break;	\
	case WAN_LOGGER_BRI:	\
		is_level_on = WAN_LOGGER_TEST_LEVEL_BRI(logger_level);	\
		break;	\
	}/* switch(type) */	\
	\
	if (is_level_on ) {	\
		wp_logger_input(logger_type, logger_level, ## __VA_ARGS__);	\
	}	\
}

#endif/* __KERNEL__ */

#endif/* __WANPIPE_LOGGER_API_HDR__ */

