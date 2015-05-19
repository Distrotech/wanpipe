#ifndef __FSK_TONE_DECODER_H__
#define __FSK_TONE_DECODER_H__


#pragma once
#include <stdio.h>
#include <libstelephony.h>

#include "Sink.h"
#include "wp_g711.h"
#include "wp_libteletone.h"

class PhoneToneDecoder
{
	/* common variables */
	variant_t			variant;	
	sink_callback_functions_t sink_callback_functions;
	void				*callback_obj;

	/* fsk variables */
	bool 				fskInit;
	bool				fskEnable;
	fsk_data_state_t 	*fskData;
	unsigned char 		*fbuf;
	size_t 				bufSize;
	int 				rate;
	int WaveStreamInputExFSK(int16_t* slin_data, int dataLength, int *retvalue);
	
	/* dtmf variables */
	bool				dtmfInit;
	bool 				dtmfEnable;
	teletone_dtmf_detect_state_t *dtmfData;
	
	int WaveStreamInputExDtmf(int16_t* slin_data, int dataLength, int *retvalue);

public:
	PhoneToneDecoder(void);
	~PhoneToneDecoder(void);
		
	int  Init(void);
	int  WaveStreamInputEx(char* data,	int dataLength, int *retvalue);
	void SetCallbackFunctions(sink_callback_functions_t *cbf);
	void GetCallbackFunctions(sink_callback_functions_t *cbf);
	void SetCallbackFunctionsAndObject(sink_callback_functions_t* cbf, void *cbo);
	void WaveStreamStart(void);
	void WaveStreamEnd(void);
	void put_WaveFormatID(variant_t var);
	void put_MonitorDTMF(bool val);
	void put_MonitorCallerID(bool val);
};

#endif/* __FSK_TONE_DECODER_H__*/
