//////////////////////////////////////////////////////////////////////
// Sink.h: interface for the CSink class.
//
// Author	:	David Rokhvarg	<davidr@sangoma.com>
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_SINK_H__3618F855_A175_41A3_AF69_3C0DEA0152A7__INCLUDED_)
#define AFX_SINK_H__3618F855_A175_41A3_AF69_3C0DEA0152A7__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifdef __WINDOWS__
# include <windows.h>
#endif

#include <stdio.h>

#include "libstelephony_linux_compat.h"
#include "libstelephony_tone.h"

#define DBG_SINK	if(0)printf

#define STEL_FUNC()			if(0)printf("%s(): Line: %d\n", __FUNCTION__, __LINE__)
#define STEL_FUNC_START()	if(0)printf("%s(): Start\n", __FUNCTION__)
#define STEL_FUNC_END()		if(0)printf("%s(): End\n", __FUNCTION__)
#define NOT_IMPL()			printf("FUNCTION_NOT_IMPLEMENTED (%s:%d)\n",__FUNCTION__,__LINE__)

#define STEL_ERR	printf("%s():line:%d:Error: ", __FUNCTION__, __LINE__);printf

typedef struct{
	void	(*OnCallerID)(void *callback_obj, char *Name, char *CallerNumber, char *CalledNumber, char *DateTime);
	void	(*OnDTMF)(void *callback_obj, long Key);
	void 	(*OnFSKCallerIDTransmit)(void *callback_obj, buffer_t* buffer);
	void 	(*OnSwDtmfTransmit)(void *callback_obj, buffer_t* buffer);
}sink_callback_functions_t;

extern void *stel_malloc(int size);

#endif // !defined(AFX_SINK_H__3618F855_A175_41A3_AF69_3C0DEA0152A7__INCLUDED_)
