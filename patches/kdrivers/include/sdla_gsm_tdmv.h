/******************************************************************************
 * sdla_gsm_tdmv.h	
 *
 * Author: 	Moises Silva <moy@sangoma.com>
 *
 * Copyright:	(c) 2011 Sangoma Technologies Inc.
 *
 *		This program is free software; you can redistribute it and/or
 *		modify it under the terms of the GNU General Public License
 *		as published by the Free Software Foundation; either version
 *		2 of the License, or (at your option) any later version.
 * ============================================================================
 * Oct 11, 2011 Moises Silva   Initial Version
 ******************************************************************************
 */
#include "wanpipe_defines.h"
#include "sdla_tdmv.h"
#include "zapcompat.h" /* Map of Zaptel -> DAHDI definitions */

typedef struct wp_tdmv_gsm_ {
	/*! Wanpipe card structre */
	sdla_t *card;
	/*! Shortcut to card->devname */
	char *devname;
	/*! Wanpipe GSM span number */
	int num;
	/*! Sanity check flags */
	int flags;
	/* DAHDI span number (as assigned in Wanpipe configuration) */
	int spanno;
	/*! DAHDI span and channels */
#if defined(CONFIG_PRODUCT_WANPIPE_TDM_VOICE)
	struct zt_span span;
#ifdef DAHDI_ISSUES
	struct zt_chan *chans_ptrs[MAX_GSM_CHANNELS];
#endif
	struct zt_chan chans[MAX_GSM_CHANNELS];

#ifdef DAHDI_26
        struct dahdi_device *ddev;
        struct device dev;
#endif
#endif
	/*! Number of users of this span (for sanity checks) */
	int usecount;
	/*! Debug audio buffer index */
	int audio_debug_i;
	unsigned char ec_chunk[ZT_CHUNKSIZE];

} wp_tdmv_gsm_t;

