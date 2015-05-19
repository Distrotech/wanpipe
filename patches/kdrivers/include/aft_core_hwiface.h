/*****************************************************************************
* sdla_aft_te1_iface.h	WANPIPE(tm) AFT Hardware Support
* 		
* Authors: 	Nenad Corbic <ncorbic@sangoma.com>
*
* Copyright:	(c) 2003 Sangoma Technologies Inc.
*
*		This program is free software; you can redistribute it and/or
*		modify it under the terms of the GNU General Public License
*		as published by the Free Software Foundation; either version
*		2 of the License, or (at your option) any later version.
* ============================================================================
* Sep 27, 2005	Nenad Corbic	Initial version.
*****************************************************************************/


typedef struct {

	u32 init;
	
	int (*hw_global_cfg)	(sdla_t *card, wandev_conf_t* conf);
	int (*hw_cfg)		(sdla_t *card, wandev_conf_t* conf);
	int (*hw_chan_cfg) 	(sdla_t *card, wandev_conf_t* conf);

	int (*hw_global_uncfg)	(sdla_t *card);
	int (*hw_uncfg)		(sdla_t *card);
	int (*hw_chan_uncfg)	(sdla_t *card, private_area_t *chan);

} aft_hw_iface_t;

