#ifndef __FSK_TONE_ENCODER_H__
#define __FSK_TONE_ENCODER_H__

#pragma once
#include <stdio.h>
#include <time.h>
#include <libstelephony.h>

#include "Sink.h"
#include "wp_g711.h"
#include "wp_libteletone.h"

class PhoneToneEncoder
{
	/* common variables */
	variant_t			variant;	
	sink_callback_functions_t sink_callback_functions;
	void				*callback_obj;
	helper_t			*userData;

	/* fsk variables */
	BOOL 				fskInit;
	BOOL				fskEnable;
	fsk_data_state_t 	*fskData;
	unsigned char 		*fbuf;
	size_t 				bufSize;
	int 				rate;
	
	fsk_modulator_t 	*fskTrans;

	/* dtmf variables */
	BOOL				dtmfInit;
	BOOL 				dtmfEnable;
	teletone_generation_session_t *dtmfSession;
	
public:
	PhoneToneEncoder(void);
	~PhoneToneEncoder(void);
		
	int Init(void);
	int BufferGetFSKCallerID(stelephony_caller_id_t *cid_info, int *retValue);
	int BufferGetSwDTMF(char dtmfChar, int *retValue);

	void SetCallbackFunctions(sink_callback_functions_t *cbf);
	void GetCallbackFunctions(sink_callback_functions_t *cbf);
	void SetCallbackFunctionsAndObject(sink_callback_functions_t* cbf, void *cbo);
		
	void WaveStreamStart(void);
	void WaveStreamEnd(void);
	void put_WaveFormatID(variant_t var);
	void put_FeatureFSKCallerID(int val);
	void put_FeatureSwDTMF(int val);
};

#endif /*__FSK_TONE_ENCODER_H__*/
