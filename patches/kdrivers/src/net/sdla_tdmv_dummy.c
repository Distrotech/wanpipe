/*****************************************************************************
* sdla_tdmv_dummy.c	WANPIPE(tm) Multiprotocol WAN Link Driver.
*		Dummy Zaptel timer.
*
* Author: 	David Yat Sin <david.yatsin@sangoma.com>
*
* Copyright:	(c) 2008 Sangoma Technologies Inc.
*
*		This program is free software; you can redistribute it and/or
*		modify it under the terms of the GNU General Public License
*		as published by the Free Software Foundation; either version
*		2 of the License, or (at your option) any later version.
* ============================================================================
* Mar 12,  2008 David Yat Sin   Initial Version
* Sep 06,  2008 Moises Silva    DAHDI support 
*****************************************************************************/

#if defined(__FreeBSD__) || defined(__OpenBSD__)
# include <wanpipe_includes.h>
# include <wanpipe_debug.h>
# include <wanpipe_defines.h>
# include <wanpipe_abstr.h>
# include <wanpipe_common.h>
# include <wanpipe.h>
# include <wanpipe_events.h>
# include <sdla_tdmv.h>	/* WANPIPE TDM Voice definitions */
# include <sdla_tdmv_dummy.h>
# include <zapcompat.h> /* Map of Zaptel -> DAHDI definitions */
#elif (defined __WINDOWS__)
# include <wanpipe\csu_dsu.h>
#else
# include <linux/wanpipe_includes.h>
# include <linux/wanpipe_defines.h>
# include <linux/wanpipe.h>
# include <linux/wanpipe_events.h>
# include <linux/sdla_tdmv.h>	/* WANPIPE TDM Voice definitions */
# include <linux/sdla_tdmv_dummy.h>
# include <zapcompat.h> /* Map of Zaptel -> DAHDI definitions */
#endif

typedef struct sdla_tdmv_dummy
{
   struct zt_span span;
#ifdef DAHDI_ISSUES
   struct zt_chan *chan_ptr;
#endif
   struct zt_chan chan;

}sdla_tdmv_dummy_t;   



int sdla_tdmv_dummy_get_zt_chunksize(void)
{	
   return ZT_CHUNKSIZE;
}

int sdla_tdmv_dummy_tick(void *wpd_ptr)
{
   sdla_tdmv_dummy_t *wpd;

   WAN_ASSERT(wpd_ptr == NULL);
   wpd=(sdla_tdmv_dummy_t *)wpd_ptr;
   
   zt_receive(&wpd->span);
   zt_transmit(&wpd->span);

   return 0;
}

#ifdef DAHDI_24
static const struct WP_DAHDI_SPAN_OPS dummy_ops = {
	.owner = THIS_MODULE,
};
#endif

void* sdla_tdmv_dummy_register(void) 
{ 
   sdla_tdmv_dummy_t *wpd = wan_kmalloc(sizeof(sdla_tdmv_dummy_t));
   if (wpd == NULL) {
      DEBUG_EVENT( "Failed to allocate memory (%s)!\n",__FUNCTION__);
      return NULL;
   }
   memset(wpd, 0x0, sizeof(sdla_tdmv_dummy_t));

   sprintf(wpd->span.name, "SDLA_DUMMY");
   snprintf(wpd->span.desc, sizeof(wpd->span.desc) - 1, "%s (source: AFT-HW) %d", wpd->span.name, 1);
#ifdef DAHDI_ISSUES
   wpd->chan_ptr   = &wpd->chan;
   wpd->span.chans = &wpd->chan_ptr;
#ifdef DAHDI_24
	wpd->span.ops = &dummy_ops;
#endif
#else
   wpd->span.chans = &wpd->chan;
#endif
   wpd->span.channels = 0;	/* no channels */
   wpd->span.deflaw = ZT_LAW_MULAW;

#ifndef DAHDI_24
   wpd->span.pvt = wpd;
   wpd->chan.pvt = wpd;
#endif

#if defined(DAHDI_26)
	return NULL;
#else
   if (zt_register(&wpd->span, 0)) {
      DEBUG_EVENT( "Failed to register Zaptel span (%s)!\n",__FUNCTION__);
      wan_free(wpd);
      return NULL;
   }
#endif
   
   return wpd;
}


int sdla_tdmv_dummy_unregister(void *wpd_ptr)
{
   sdla_tdmv_dummy_t *wpd;

   WAN_ASSERT(wpd_ptr == NULL);

   wpd=(sdla_tdmv_dummy_t *)wpd_ptr;

#if !defined(DAHDI_26)
   zt_unregister(&wpd->span);
#endif
   wan_free(wpd);
   return 0;
}
