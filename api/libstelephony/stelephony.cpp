////////////////////////////////////////////////////////////////////////////////////
//stelephony.cpp - Definitions of the main class of the Sangoma Telephony Library
//					(CStelephony). 
//
// Author	:	David Rokhvarg	<davidr@sangoma.com>
//
// Versions:
//	v 1,0,0,1	May  29	2008	David Rokhvarg	Initial Version
//		Implemented FSK Caller ID detection for Analog FXO.
//
////////////////////////////////////////////////////////////////////////////////////

#include <string.h>
#include <stdio.h>

#include "stelephony.h"

#define DBG_STEL if(0)printf

CStelephony::CStelephony()
{ 
	STEL_FUNC();

	memset(&callback_functions, 0x00, sizeof(stelephony_callback_functions_t));
	InitializeCriticalSection(&m_CriticalSection);
	return; 
}

CStelephony::~CStelephony()
{ 
	STEL_FUNC();
	return; 
}

void CStelephony::SetCallbackFunctions(IN stelephony_callback_functions_t *cbf)
{
	memcpy(&callback_functions, cbf, sizeof(stelephony_callback_functions_t));
	
	if(cbf->Q931Event){
		DBG_STEL("%s(): cbf->Q931Event: 0x%p\n", __FUNCTION__, cbf->Q931Event);
		q931_event_decoder_callback_functions_t q931_cbf;
		q931_cbf.OnQ931Event = cbf->Q931Event;
		Q931EventsDecoderObj.SetCallbackFunctions(&q931_cbf);
	}
}

void CStelephony::GetCallbackFunctions(OUT stelephony_callback_functions_t *cbf)
{
	memcpy(cbf, &callback_functions, sizeof(stelephony_callback_functions_t));
}

void CStelephony::SetCallbackContext(void *cbc)
{
	callback_context = cbc;
	Q931EventsDecoderObj.SetCallbackObject(cbc);
}

void* CStelephony::GetCallbackContext(void)
{
	return callback_context;
}

stelephony_status_t CStelephony::Init()
{ 
	STEL_FUNC();
	stelephony_status_t status = STEL_STATUS_SUCCESS;

	status = InitDecoderLib();
	if (status != STEL_STATUS_SUCCESS) return status;

	status = InitEncoderLib();
	if (status != STEL_STATUS_SUCCESS) return status;

	return STEL_STATUS_SUCCESS;
}

stelephony_status_t CStelephony::Cleanup()
{ 
	STEL_FUNC();
	stelephony_status_t status = STEL_STATUS_SUCCESS;

	status = CleanupDecoderLib();
	if (status != STEL_STATUS_SUCCESS) return status;

	status = CleanupEncoderLib();
	if (status != STEL_STATUS_SUCCESS) return status;
	return STEL_STATUS_SUCCESS;
}

stelephony_status_t CStelephony::InitDecoderLib()
{
	if (ToneDecoderObj.Init()) {
		return STEL_STATUS_INVALID_INPUT_ERROR;
	}	
	return STEL_STATUS_SUCCESS;
}

stelephony_status_t CStelephony::InitEncoderLib()
{
	if (ToneEncoderObj.Init()) {
		return STEL_STATUS_INVALID_INPUT_ERROR;
	}	
	return STEL_STATUS_SUCCESS;
}

stelephony_status_t CStelephony::CleanupDecoderLib()
{
	ToneDecoderObj.WaveStreamEnd();
	return STEL_STATUS_SUCCESS;
}

stelephony_status_t CStelephony::CleanupEncoderLib()
{
	ToneEncoderObj.WaveStreamEnd();
	return STEL_STATUS_SUCCESS;
}

int CStelephony::DecoderLibStreamInput(void *data, int dataLength)
{
	//If user enabled FSK CallerID or SoftwareDTMF, call ToneDecoder with (voice) StreamInput.
	if(callback_functions.FSKCallerIDEvent || callback_functions.DTMFEvent){
		int retvalue, rc;
		rc = ToneDecoderObj.WaveStreamInputEx((char*)data, dataLength, &retvalue);
		//printf("retvalue: %d, rc: %d\n", retvalue, rc);
	}

	if(callback_functions.Q931Event){
		Q931EventsDecoderObj.Input(data,	dataLength);
	}

	return 0;
}

stelephony_status_t CStelephony::EncoderLibGenerateFSKCallerID(stelephony_caller_id_t* cid_info)
{
	int retValue, rc;

	rc = ToneEncoderObj.BufferGetFSKCallerID(cid_info, &retValue);
	if (rc) {
		return STEL_STATUS_ENCODER_LIB_ERROR;
	}
	return STEL_STATUS_SUCCESS;
}

stelephony_status_t CStelephony::EncoderLibGenerateSwDTMF(char dtmf_char)
{
	int retValue, rc;

	rc = ToneEncoderObj.BufferGetSwDTMF(dtmf_char, &retValue);
	if (rc) {
		return STEL_STATUS_ENCODER_LIB_ERROR;
	}
	return STEL_STATUS_SUCCESS;
}

void OnCallerID(void *callback_obj, char *Name, char *CallerNumber, char *CalledNumber, char *DateTime)
{
	CStelephony* stelObj = (CStelephony*)callback_obj;
	stelephony_callback_functions_t cbf;

	DBG_STEL("%s(): Name: %s(%zu), CallerNumber: %s(%zu), CalledNumber: %s, DateTime: %s\r\n",
		__FUNCTION__, Name, strlen(Name), CallerNumber, strlen(CallerNumber), CalledNumber, DateTime);

	stelObj->GetCallbackFunctions(&cbf);

	if(cbf.FSKCallerIDEvent){
		cbf.FSKCallerIDEvent(stelObj->GetCallbackContext(), Name, CallerNumber, CalledNumber, DateTime);
	}
}

void OnDTMF(void *callback_obj, long Key)
{
	CStelephony* stelObj = (CStelephony*)callback_obj;
	stelephony_callback_functions_t cbf;

	DBG_STEL("%s(): Key: %c\r\n", __FUNCTION__, (int) Key);

	stelObj->GetCallbackFunctions(&cbf);

	if(cbf.DTMFEvent){
		cbf.DTMFEvent(stelObj->GetCallbackContext(), Key);
	}
}

void OnFSKCallerIDTransmit(void *callback_obj, buffer_t* buffer)
{
	CStelephony* stelObj = (CStelephony*)callback_obj;
	stelephony_callback_functions_t cbf;
	stelObj->GetCallbackFunctions(&cbf);

	if(cbf.FSKCallerIDTransmit){
		cbf.FSKCallerIDTransmit(stelObj->GetCallbackContext(), buffer);
	} else {
		STEL_ERR("No CallerID Transmit callback function\n");
	}
}

void OnSwDtmfTransmit(void *callback_obj, buffer_t* buffer)
{
	CStelephony* stelObj = (CStelephony*)callback_obj;
	stelephony_callback_functions_t cbf;
	stelObj->GetCallbackFunctions(&cbf);

	STEL_FUNC();

	//call DTMF generation callback so user will transmit it.
	if (cbf.SwDTMFBuffer) {
		cbf.SwDTMFBuffer(stelObj->GetCallbackContext(), buffer);
	} else {
		STEL_ERR("No OnSwDtmfTransmit() callback function\n");
	}
}

stelephony_status_t CStelephony::ToneEncoderEventControl(stelephony_event_t Event, stelephony_control_code_t ControlCode, void *optionalData)
{
	//get current callback functions
	sink_callback_functions_t scf;

	ToneEncoderObj.GetCallbackFunctions(&scf);

	switch(Event)
	{
		case STEL_FEATURE_FSK_CALLER_ID:
		case STEL_FEATURE_SW_DTMF:
			switch(ControlCode)
			{
				case STEL_CTRL_CODE_ENABLE:
					switch(Event)
					{
						case STEL_FEATURE_FSK_CALLER_ID:
							scf.OnFSKCallerIDTransmit = OnFSKCallerIDTransmit;
							break;
						case STEL_FEATURE_SW_DTMF:
							scf.OnSwDtmfTransmit = OnSwDtmfTransmit;
							break;
						default:
							return STEL_STATUS_INVALID_EVENT_ERROR;
					}
					m_ToneEncoderCodec = *(stelephony_option_t*)optionalData;

					VARIANT var;
					var.vt=VT_UI4;
					if(m_ToneEncoderCodec == STEL_OPTION_ALAW){
						var.intVal=WFI_CCITT_ALaw_8kHzMono;
					}else{
						var.intVal=WFI_CCITT_uLaw_8kHzMono;
					}
					ToneEncoderObj.WaveStreamEnd(); 
					ToneEncoderObj.put_WaveFormatID(var);
					ToneEncoderObj.put_FeatureFSKCallerID(TRUE);
					ToneEncoderObj.put_FeatureSwDTMF(TRUE);
					ToneEncoderObj.SetCallbackFunctionsAndObject(&scf, this);
					ToneEncoderObj.WaveStreamStart(); 
					break;
				case STEL_CTRL_CODE_DISABLE:
					switch(Event)
					{
						case STEL_FEATURE_FSK_CALLER_ID:
							scf.OnFSKCallerIDTransmit = NULL;
							break;
	
						case STEL_FEATURE_SW_DTMF:
							scf.OnSwDtmfTransmit = NULL;
							break;
						default:
							return STEL_STATUS_INVALID_EVENT_ERROR;
					}
					ToneEncoderObj.WaveStreamEnd(); 
					ToneEncoderObj.SetCallbackFunctionsAndObject(&scf, this);
					break;
			}
			break;
		default:
			return STEL_STATUS_INVALID_EVENT_ERROR;
	}
	return STEL_STATUS_SUCCESS;
}

stelephony_status_t CStelephony::ToneDecoderEventControl(stelephony_event_t Event, stelephony_control_code_t ControlCode, void *optionalData)
{
	//get current callback functions
	sink_callback_functions_t scf;

	ToneDecoderObj.GetCallbackFunctions(&scf);
	
	//add/remove a function
	switch(Event)
	{
	case STEL_EVENT_FSK_CALLER_ID:
	case STEL_EVENT_DTMF:
		switch(ControlCode)
		{
		case STEL_CTRL_CODE_ENABLE:
			switch(Event)
			{
			case STEL_EVENT_Q931:
				STEL_ERR("Q931 event not handled\n");
				break;
			case STEL_EVENT_FSK_CALLER_ID:
				scf.OnCallerID = OnCallerID;
				ToneDecoderObj.put_MonitorCallerID(TRUE);
				break;
			case STEL_EVENT_DTMF:
				scf.OnDTMF = OnDTMF;
				ToneDecoderObj.put_MonitorDTMF(TRUE);
				break;
			default:
				return STEL_STATUS_INVALID_EVENT_ERROR;
			}
			m_ToneDecoderCodec = *(stelephony_option_t*)optionalData;

			ToneDecoderObj.WaveStreamEnd();//stop stream, just in case it was running before.
			
			VARIANT var;
			var.vt=VT_UI4;
			if(m_ToneDecoderCodec == STEL_OPTION_ALAW){
				var.intVal=WFI_CCITT_ALaw_8kHzMono;
			}else{
				var.intVal=WFI_CCITT_uLaw_8kHzMono;
			}
			ToneDecoderObj.put_WaveFormatID(var);
			ToneDecoderObj.WaveStreamStart();
			break;

		case STEL_CTRL_CODE_DISABLE:
			switch(Event)
			{
			case STEL_EVENT_Q931:
				STEL_ERR("Q931 event not handled\n");
				break;
			case STEL_EVENT_FSK_CALLER_ID:
				scf.OnCallerID = NULL;
				ToneDecoderObj.put_MonitorCallerID(FALSE);
				break;
			case STEL_EVENT_DTMF:
				scf.OnDTMF = NULL;
				ToneDecoderObj.put_MonitorDTMF(FALSE);
				break;
			default:
				return STEL_STATUS_INVALID_EVENT_ERROR;
			}
			break;
		}
		break;
	default:
		return STEL_STATUS_INVALID_EVENT_ERROR;
	}

	ToneDecoderObj.SetCallbackFunctionsAndObject(&scf, this);

	return STEL_STATUS_SUCCESS;
}

stelephony_status_t CStelephony::EventControl(stelephony_event_t Event, stelephony_control_code_t ControlCode, void *optionalData)
{
	switch(Event)
	{
	case STEL_EVENT_FSK_CALLER_ID:
	case STEL_EVENT_DTMF:
		return ToneDecoderEventControl(Event, ControlCode, optionalData);

	case STEL_FEATURE_FSK_CALLER_ID:
	case STEL_FEATURE_SW_DTMF:
		return ToneEncoderEventControl(Event, ControlCode, optionalData);

	case STEL_EVENT_Q931:
		return Q931EventsDecoderObj.EventControl(Event, ControlCode, optionalData);

	default:
		return STEL_STATUS_INVALID_EVENT_ERROR;
	}
}

void CStelephony::EnterStelCriticalSection()
{
	EnterCriticalSection(&m_CriticalSection);
}

void CStelephony::LeaveStelCriticalSection()
{
	LeaveCriticalSection(&m_CriticalSection);
}

/*
///////////////////////////////////////////////////
FSK Decoder test results:
fsk file name: test-caller-id-stel.wav

old FSK CID library:
wanpipe1_if3: FSKCallerIDEvent() - Start
Name: DAVID
CallerNumber: 98765555
DateTime: 03-04 13:53
FSKCallerIDEvent() - End

new (open source) FSK CID library:
wanpipe22_if1: FSKCallerIDEvent() - Start
Name: DAVID
CallerNumber: 98765555
CalledNumber: Private
DateTime: 03041353
FSKCallerIDEvent() - End
///////////////////////////////////////////////////
*/
