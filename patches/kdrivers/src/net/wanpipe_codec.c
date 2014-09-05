/**************************************************************************
 * wanpipe_codec.c	WANPIPE(tm) Multiprotocol WAN Link Driver. 
 *				TDM voice board configuration.
 *
 * Author: 	Nenad Corbic  <ncorbic@sangoma.com>
 *
 * Copyright:	(c) 1995-2005 Sangoma Technologies Inc.
 *
 *		This program is free software; you can redistribute it and/or
 *		modify it under the terms of the GNU General Public License
 *		as published by the Free Software Foundation; either version
 *		2 of the License, or (at your option) any later version.
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
#include "wanpipe_codec.h"


wanpipe_codec_ops_t *WANPIPE_CODEC_OPS[WP_TDM_HW_CODING_MAX][WP_TDM_CODEC_MAX];


#ifdef __LINUX__ 
__init int wanpipe_codec_init(void)
#elif defined(__WINDOWS__)
int wanpipe_codec_init(void)
#else
__init int wanpipe_codec_init(void)
#endif
{

#ifdef CONFIG_PRODUCT_WANPIPE_CODEC_SLINEAR_LAW
	wanpipe_codec_ops_t *wp_codec_ops;	

	wanpipe_codec_law_init();

	{
		int i, j;
		for (i = 0; i < WP_TDM_HW_CODING_MAX; i++){
			for (j = 0; j < WP_TDM_CODEC_MAX; j++){
				WANPIPE_CODEC_OPS[i][j]=NULL;
			}
		}
	}


	wp_codec_ops = wan_malloc(sizeof(wanpipe_codec_ops_t));
	if (!wp_codec_ops){
		return -ENOMEM;
	}

	memset(wp_codec_ops,0,sizeof(wanpipe_codec_ops_t));

	wp_codec_ops->init = 1;
	wp_codec_ops->encode = wanpipe_codec_convert_ulaw_2_s;
	wp_codec_ops->decode = wanpipe_codec_convert_s_2_ulaw;

	WANPIPE_CODEC_OPS[WP_MULAW][WP_SLINEAR] = wp_codec_ops;

	wp_codec_ops = wan_malloc(sizeof(wanpipe_codec_ops_t));
	if (!wp_codec_ops){
		return -ENOMEM;
	}

	memset(wp_codec_ops,0,sizeof(wanpipe_codec_ops_t));

	wp_codec_ops->init = 1;
	wp_codec_ops->encode = wanpipe_codec_convert_alaw_2_s;
	wp_codec_ops->decode = wanpipe_codec_convert_s_2_alaw;

	WANPIPE_CODEC_OPS[WP_ALAW][WP_SLINEAR] = wp_codec_ops;
#endif

	DEBUG_EVENT("WANPIPE: TDM Codecs Initialized\n");

	return 0;
}

#ifdef __LINUX__
__exit int wanpipe_codec_free(void)
#else
int wanpipe_codec_free(void)
#endif
{
	int i,j;
	for (i = 0; i < WP_TDM_HW_CODING_MAX; i++){
		for (j=0;j < WP_TDM_CODEC_MAX; j++){
			if (WANPIPE_CODEC_OPS[i][j]){
				wan_free(WANPIPE_CODEC_OPS[i][j]);
				WANPIPE_CODEC_OPS[i][j]=NULL;
			}
		}
	}

	DEBUG_EVENT("WANPIPE: TDM Codecs unloaded.\n");
	return 0;
}

int wanpipe_codec_convert_to_linear(u8 pcm_value,u8 codecType)
{
	int ret = 0;
	switch(codecType)
	{
		case WP_ALAW:
			{
				ret = wanpipe_codec_get_alaw_to_linear(pcm_value);
				break;
			}
		case WP_MULAW:
			{
				ret = wanpipe_codec_get_ulaw_to_linear(pcm_value);
				break;
			}
		default:
			{
				DEBUG_ERROR("%s(): Invalid CodecType[%d]\n",__FUNCTION__,codecType);
				break;
			}
	}

	return ret;
}
