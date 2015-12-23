/*****************************************************************************
* sdla_tdmv_dummy.h	WANPIPE(tm) Multiprotocol WAN Link Driver.
*		Dummy Zaptel timer definitions.
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
*****************************************************************************/

#ifndef	_SDLA_TDMV_DUMMY_H
#define	_SDLA_TDMV_DUMMY_H

int sdla_tdmv_dummy_get_zt_chunksize(void);
void* sdla_tdmv_dummy_register(void);
int sdla_tdmv_dummy_tick(void *wpd_ptr);
int sdla_tdmv_dummy_unregister(void *wpd_ptr);
#endif
