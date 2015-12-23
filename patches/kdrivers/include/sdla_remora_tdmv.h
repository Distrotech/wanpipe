/*******************************************************************************
** sdla_remora_tdmv.h	
**
** Author: 	Jignesh Patel <jpatel@sangoma.com>
**
** Copyright:	(c) 2008 Sangoma Technologies Inc.
**
**		This program is free software; you can redistribute it and/or
**		modify it under the terms of the GNU General Public License
**		as published by the Free Software Foundation; either version
**		2 of the License, or (at your option) any later version.
** ============================================================================
** Oct 20, 2008	  		 Initial version.
*******************************************************************************/
#include "wanpipe_defines.h"
#include "sdla_tdmv.h"
#include "zapcompat.h" /* Map of Zaptel -> DAHDI definitions */


typedef struct wp_tdmv_remora_ {
	void		*card;
	char		*devname;
	int		num;
	int		flags;
	wan_spinlock_t	lockirq;
	wan_spinlock_t	tx_rx_lockirq;
	union {
		wp_remora_fxo_t	fxo;
		wp_remora_fxs_t	fxs;
	} mod[MAX_REMORA_MODULES];

	int		spanno;
#if defined(CONFIG_PRODUCT_WANPIPE_TDM_VOICE)
	struct zt_span	span;
#ifdef DAHDI_ISSUES
#ifdef DAHDI_22
	struct dahdi_echocan_state ec[MAX_REMORA_MODULES];		/* echocan state for each channel */
#endif
	struct zt_chan	*chans_ptrs[MAX_REMORA_MODULES];

#ifdef DAHDI_26
   struct dahdi_device *ddev;
   struct device dev;
#endif

#endif
	struct zt_chan	chans[MAX_REMORA_MODULES];
#endif
	unsigned long	reg_module_map;	/* Registered modules */
#if 0
	unsigned char	reg0shadow[MAX_REMORA_MODULES];	/* read> fxs: 68 fxo: 5 */
	unsigned char	reg1shadow[MAX_REMORA_MODULES];	/* read> fxs: 64 fxo: 29 */
	unsigned char	reg2shadow[MAX_REMORA_MODULES];	/* read> fxs: 64 fxo: 29 */

	unsigned char	reg0shadow_write[MAX_REMORA_MODULES];	/* write> fxs: 68 fxo: 5 */
	int		reg0shadow_update[MAX_REMORA_MODULES];
#endif	
	/* Global configuration */

/*	u32		intcount;*/
	int		pollcount;
	unsigned char	ec_chunk1[31][ZT_CHUNKSIZE];
	unsigned char	ec_chunk2[31][ZT_CHUNKSIZE];
	int		usecount;
	u16		max_timeslots;		/* up to MAX_REMORA_MODULES */
	int		max_rxtx_len;
	int		channelized;
	unsigned char	hwec;
	unsigned long	echo_off_map;
#if 0	
	int		battdebounce;		/* global for FXO */
	int		battthresh;		/* global for FXO */
#endif
	u_int8_t	tonesupport;
	unsigned int	toneactive;
	unsigned int	tonemask;
	unsigned int	tonemutemask;

	unsigned long	ec_fax_detect_timeout[MAX_REMORA_MODULES+1];
	unsigned int	ec_off_on_fax;
	
} wp_tdmv_remora_t;
