/***************************************************************************
 * sdla_edac.c	WANPIPE(tm) Multiprotocol WAN Link Driver. 
 *				Implementation of EchoMaster algorithm.
 *
 * Author: 	David Rokhvarg  <davidr@sangoma.com>
 *
 * Copyright:	(c) 1995-2005 Sangoma Technologies Inc.
 *
 * ============================================================================
 * Jul 07, 2005	David Rokhvarg	Initial version.
 * Jul 26, 2005	David Rokhvarg	V1.00
 *		Added ALAW support.
 ******************************************************************************
 */

/*
 ******************************************************************************
			   INCLUDE FILES
 ******************************************************************************
*/


#include "wanpipe_includes.h"
#include "wanpipe_defines.h"
#include "wanpipe_debug.h"
#include "wanpipe_common.h"
#include "wanpipe.h"
#include "sdla_tdmv.h"
#include "zaptel.h"
#include "wanpipe_edac_iface.h"

/*
 ******************************************************************************
			  DEFINES AND MACROS
 ******************************************************************************
*/
#define DEBUG_ECHO	if(0) DEBUG_EVENT


/******************************************************************************
** wp_tdmv_echo_check() - check the channel for echo
**
*/
int wp_tdmv_echo_check(wan_tdmv_t *wan_tdmv, void *current_ztchan, int channo)
{
        struct zt_chan *thechan = (struct zt_chan *)current_ztchan;
        wan_tdmv_rxtx_pwr_t *pwr_rxtx = &wan_tdmv->chan_pwr[channo];

        if(thechan->echo_detect_struct.echo_detection_state !=
           thechan->echo_detect_struct.echo_detection_state_old){

                DEBUG_ECHO("%s(): chan:%d, new ed_state: %d\n", __FUNCTION__, channo,
                        thechan->echo_detect_struct.echo_detection_state);

                /* there was a state change */
                switch(thechan->echo_detect_struct.echo_detection_state)
                {
                case ECHO_DETECT_ON:
                case ECHO_DETECT_OFF:
                        init_ed_state(pwr_rxtx, channo);
                        thechan->echo_detect_struct.echo_detection_state_old =
                                thechan->echo_detect_struct.echo_detection_state;
                        break;
                default:
                        DEBUG_EVENT("%s(): channo:%d: Invalid echo_detection_state: %d\n", __FUNCTION__,
                                channo, thechan->echo_detect_struct.echo_detection_state);
                        return 1;
                }
        }

        if(thechan->echo_detect_struct.echo_detection_state != ECHO_DETECT_ON){
                return 0;
        }

        /* As soon is Echo state is known, do NOT run the ED algorithm. */
        if(pwr_rxtx->current_state != INDETERMINATE){
                return 0;
        }

	wp_tdmv_calc_echo(pwr_rxtx, (thechan->xlaw == __zt_mulaw), 
			  channo, thechan->readchunk, thechan->writechunk,
			  ZT_CHUNKSIZE);

        return 0;
}
