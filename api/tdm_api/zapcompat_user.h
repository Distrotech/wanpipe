/******************************************************************************
 * zapcompat.h	
 *
 * Author: 	Moises Silva <moises.silva@gmail.com>
 *
 * Copyright:	(c) 2008 Sangoma Technologies Inc.
 *
 *		This program is free software; you can redistribute it and/or
 *		modify it under the terms of the GNU General Public License
 *		as published by the Free Software Foundation; either version
 *		2 of the License, or (at your option) any later version.
 * ============================================================================
 * Sep 06,  2008 Moises Silva   Initial Version
 ******************************************************************************
 */

// Simple compat header to compile with Zaptel or DAHDI
#ifndef __ZAPCOMPAT_H
# define __ZAPCOMPAT_H

// for DAHDI we need to map values and functions from user ZT_XX to DAHDI_XX
#if defined (DAHDI_ISSUES)
#include <sys/types.h>
#include <dahdi/user.h> 

#define ZT_LAW_MULAW DAHDI_LAW_MULAW
#define ZT_LAW_ALAW DAHDI_LAW_ALAW
#define ZT_LAW_DEFAULT DAHDI_LAW_DEFAULT

#define ZT_MAX_SPANS DAHDI_MAX_SPANS
#define ZT_MAX_CHANNELS DAHDI_MAX_CHANNELS

#define ZT_SIG_EM DAHDI_SIG_EM
#define ZT_SIG_EM_E1 DAHDI_SIG_EM_E1
#define ZT_SIG_FXSLS DAHDI_SIG_FXSLS
#define ZT_SIG_FXSGS DAHDI_SIG_FXSGS
#define ZT_SIG_FXSKS DAHDI_SIG_FXSKS
#define ZT_SIG_FXOLS DAHDI_SIG_FXOLS
#define ZT_SIG_FXOGS DAHDI_SIG_FXOGS
#define ZT_SIG_FXOKS DAHDI_SIG_FXOKS
#define ZT_SIG_CAS DAHDI_SIG_CAS
#define ZT_SIG_DACS DAHDI_SIG_DACS
#define __ZT_SIG_DACS __DAHDI_SIG_DACS
#define ZT_SIG_DACS_RBS DAHDI_SIG_DACS_RBS
#define ZT_SIG_CLEAR DAHDI_SIG_CLEAR
#define ZT_SIG_SLAVE DAHDI_SIG_SLAVE
#define ZT_SIG_HDLCRAW DAHDI_SIG_HDLCRAW
#define ZT_SIG_HDLCFCS DAHDI_SIG_HDLCFCS
#define ZT_SIG_HDLCNET DAHDI_SIG_HDLCNET
#define ZT_SIG_MTP2 DAHDI_SIG_MPT2

#define ZT_ABIT DAHDI_ABIT
#define ZT_BBIT DAHDI_BBIT
#define ZT_CBIT DAHDI_CBIT
#define ZT_DBIT DAHDI_DBIT

#define ZT_CONFIG_NOTOPEN DAHDI_CONFIG_NOTOPEN
#define ZT_CONFIG_HDB3 DAHDI_CONFIG_HDB3
#define ZT_CONFIG_CCS DAHDI_CONFIG_CCS
#define ZT_CONFIG_CRC4 DAHDI_CONFIG_CRC4
#define ZT_CONFIG_AMI DAHDI_CONFIG_AMI
#define ZT_CONFIG_B8ZS DAHDI_CONFIG_B8ZS
#define ZT_CONFIG_D4 DAHDI_CONFIG_D4
#define ZT_CONFIG_ESF DAHDI_CONFIG_ESF

#define ZT_PARAMS struct dahdi_params
#define ZT_BUFFERINFO struct dahdi_bufferinfo

#define ZT_SPECIFY DAHDI_SPECIFY
#define ZT_SIG_HARDHDLC DAHDI_SIG_HARDHDLC
#define ZT_SET_BLOCKSIZE DAHDI_SET_BLOCKSIZE 
#define ZT_GET_PARAMS DAHDI_GET_PARAMS
#define ZT_POLICY_IMMEDIATE DAHDI_POLICY_IMMEDIATE
#define ZT_SET_BUFINFO DAHDI_SET_BUFINFO
#define ZT_GETEVENT DAHDI_GETEVENT
#define ZT_FLUSH_ALL DAHDI_FLUSH_ALL
#define ZT_FLUSH DAHDI_FLUSH

// data types
#define zt_lineconfig dahdi_lineconfig
#define zt_dynamic_span dahdi_dynamic_span
#define zt_chanconfig dahdi_chanconfig

#else
// zaptel is present
// we will keep the same old names in wanpipe code, I thought of changing them
// to something like WP_XX instead of ZT_XX, but I don't see any benefit on it
// and would make this file bigger 
#include <zaptel.h>
#endif

#endif	/* __ZAPCOMPAT_H */
