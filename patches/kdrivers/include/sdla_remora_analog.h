/*******************************************************************************
** sdla_remora_analog.h	
**
** Author: 		Jignesh Patel <jpatel@sangoma.com>
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

/*******************************************************************************
**			  DEFINES and MACROS
*******************************************************************************/

#define OHT_TIMER		6000
#define FXO_LINK_DEBOUNCE	200
#define POLARITY_DEBOUNCE       16     /* Polarity debounce (64 ms) */
#define MAX_ALARMS		10
#define FXO_LINK_THRESH		1      /* fxo link threshold */


#define REG_SHADOW
#define REG_WRITE_SHADOW
#define NEW_PULSE_DIALING
#undef  PULSE_DIALING


/*******************************************************************************
**			  FUNCTION PROTOTYPES
*******************************************************************************/
int wp_tdmv_remora_rx_tx_span_common(void *pcard );
int wp_tdmv_remora_proslic_recheck_sanity(sdla_fe_t *fe, int mod_no);
void wp_tdmv_remora_voicedaa_recheck_sanity(sdla_fe_t *fe, int mod_no);
int wp_tdmv_remora_check_hook(sdla_fe_t *fe, int mod_no);
int wp_remora_read_dtmf(sdla_fe_t *fe, int mod_no);



