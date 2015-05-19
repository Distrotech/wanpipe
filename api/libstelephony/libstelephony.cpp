////////////////////////////////////////////////////////////////////////////////////
//libstelephony.cpp - Defines the entry points for the Sangoma Telephony Library (Stelephony.dll). 
//			These are the functions exported for library users.
//
// Author	:	David Rokhvarg	<davidr@sangoma.com>
//
// Versions:
//	v 1,0,0,1	May  29	2008	David Rokhvarg	Initial Version
//		Implemented FSK Caller ID detection for Analog FXO.
//
//	v 1,0,0,2	June 12	2008	David Rokhvarg
//		Software DTMF detection
////////////////////////////////////////////////////////////////////////////////////

#include "stelephony.h"

#ifdef __cplusplus
extern "C" {	/* for C++ users */
#endif

size_t stelephony_buffer_inuse(void *pbuffer);
size_t stelephony_buffer_read(void *pbuffer, void *data, size_t datalen);
size_t stelephony_buffer_read_ulaw(void *pbuffer, unsigned char *data, int* dlen, int max);
size_t stelephony_buffer_read_alaw(void *buffer, unsigned char *data, int* dlen, int max);

#ifdef __cplusplus
}
#endif

#define DBG_MAIN if(0)printf

#if defined(__WINDOWS__)
BOOL APIENTRY DllMain( HANDLE hModule, 
                       DWORD  ul_reason_for_call, 
                       LPVOID lpReserved
					 )
{
	STEL_FUNC_START();
    switch (ul_reason_for_call)
	{
		case DLL_PROCESS_ATTACH:
			DBG_MAIN("DLL_PROCESS_ATTACH\n");
			break;
		case DLL_PROCESS_DETACH:
			DBG_MAIN("DLL_PROCESS_DETACH\n");
			break;
		case DLL_THREAD_ATTACH:
			//DBG_MAIN("DLL_THREAD_ATTACH\n");
			break;
		case DLL_THREAD_DETACH:
			//DBG_MAIN("DLL_THREAD_DETACH\n");
			break;
		default:
			break;
    }
	STEL_FUNC_END();
    return TRUE;
}
#endif

////////////////////////////////////////////////////////////////////////////////////
//Functions exported by the stelephony.dll.
//	These functions are thread safe on per CStelephony object basis.
////////////////////////////////////////////////////////////////////////////////////
stelephony_status_t STELAPI_CALL StelSetup(void **stelObjPtr, void *callback_context, stelephony_callback_functions_t *cbf)
{
	CStelephony			*stelObj;
	stelephony_status_t	rc;

	STEL_FUNC_START();
	do{
		stelObj = new CStelephony();
		if(stelObj == NULL){
			STEL_FUNC();
			rc = STEL_STATUS_MEMORY_ALLOCATION_ERROR;
			break;
		}

		if((rc = stelObj->Init()) != STEL_STATUS_SUCCESS){
			STEL_FUNC();
			delete stelObj;
			stelObj = NULL;
			break;
		}
		stelObj->EnterStelCriticalSection();
		stelObj->SetCallbackFunctions(cbf);
		stelObj->SetCallbackContext(callback_context);
		stelObj->LeaveStelCriticalSection();

	}while(0);

	DBG_MAIN("%s(): returning stelObj: 0x%p\n", __FUNCTION__, stelObj);

	*stelObjPtr = stelObj;
	return rc;
}

stelephony_status_t STELAPI_CALL StelCleanup(void *stelObjPtr)
{
	CStelephony* stelObj = (CStelephony*)stelObjPtr;
		
	DBG_MAIN("%s(): stelObj: 0x%p\n", __FUNCTION__, stelObj);

	if(stelObj){
		stelObj->EnterStelCriticalSection();
		stelObj->Cleanup();
		stelObj->LeaveStelCriticalSection();
		delete stelObj;
		return STEL_STATUS_SUCCESS;
	}else{
		return STEL_STATUS_INVALID_INPUT_ERROR;
	}
}

stelephony_status_t STELAPI_CALL StelStreamInput(void *stelObjPtr, void *data, int dataLength)
{
	CStelephony* stelObj = (CStelephony*)stelObjPtr;
		
	if(!stelObj){
		return STEL_STATUS_INVALID_INPUT_ERROR;
	}
	if(!data){
		return STEL_STATUS_INVALID_INPUT_ERROR;
	}
	if(dataLength < 4){
		return STEL_STATUS_INVALID_INPUT_ERROR;
	}

	stelObj->EnterStelCriticalSection(); 
	stelObj->DecoderLibStreamInput(data, dataLength);
	stelObj->LeaveStelCriticalSection();

	return STEL_STATUS_SUCCESS; 
}

stelephony_status_t STELAPI_CALL StelGenerateFSKCallerID(void *stelObjPtr, stelephony_caller_id_t* cid_info)
{
	CStelephony* stelObj = (CStelephony*)stelObjPtr;

	if(!stelObj){
		return STEL_STATUS_INVALID_INPUT_ERROR;
	}

	stelObj->EnterStelCriticalSection(); 
	stelObj->EncoderLibGenerateFSKCallerID(cid_info);
	stelObj->LeaveStelCriticalSection();
	return STEL_STATUS_SUCCESS; 
}

stelephony_status_t STELAPI_CALL StelGenerateSwDTMF(void *stelObjPtr, char dtmf_char)
{
	CStelephony* stelObj = (CStelephony*)stelObjPtr;

	if(!stelObj){
		return STEL_STATUS_INVALID_INPUT_ERROR;
	}

	stelObj->EnterStelCriticalSection(); 
	stelObj->EncoderLibGenerateSwDTMF(dtmf_char);
	stelObj->LeaveStelCriticalSection();
	return STEL_STATUS_SUCCESS; 
}

stelephony_status_t STELAPI_CALL StelEventControl(void *stelObjPtr, stelephony_event_t Event, stelephony_control_code_t ControlCode, void *optionalData)
{
	CStelephony* stelObj = (CStelephony*)stelObjPtr;
	stelephony_status_t rc = STEL_STATUS_SUCCESS;

	switch(ControlCode)
	{
	case STEL_CTRL_CODE_ENABLE:
	case STEL_CTRL_CODE_DISABLE:
		break;
	default:
		return STEL_STATUS_INVALID_EVENT_CONTROL_CODE_ERROR;
	}

	switch(Event)
	{
	case STEL_EVENT_FSK_CALLER_ID:
	case STEL_EVENT_DTMF:
	case STEL_EVENT_Q931:
	case STEL_FEATURE_FSK_CALLER_ID:
	case STEL_FEATURE_SW_DTMF:
		stelObj->EnterStelCriticalSection();
		rc = stelObj->EventControl(Event, ControlCode, optionalData);
		stelObj->LeaveStelCriticalSection();
		return rc;

	default:
		return STEL_STATUS_INVALID_EVENT_ERROR;
	}
}

//////////////////////////////////////////////////////////

size_t STELAPI_CALL StelBufferInuse(void *stelObjPtr, void *pbuffer)
{
	size_t sizeTmp;
	CStelephony* stelObj = (CStelephony*)stelObjPtr;

	stelObj->EnterStelCriticalSection();
	sizeTmp = stelephony_buffer_inuse(pbuffer);
	stelObj->LeaveStelCriticalSection();
	return sizeTmp;
}

size_t STELAPI_CALL StelBufferRead(void *stelObjPtr, void *pbuffer, void *data, size_t datalen)
{
	size_t sizeTmp;
	CStelephony* stelObj = (CStelephony*)stelObjPtr;

	stelObj->EnterStelCriticalSection();
	sizeTmp = stelephony_buffer_read(pbuffer, data, datalen);
	stelObj->LeaveStelCriticalSection();
	return sizeTmp;
}

size_t STELAPI_CALL StelBufferReadUlaw(void *stelObjPtr, void *pbuffer, unsigned char *data, int* dlen, int max)
{
	size_t sizeTmp;
	CStelephony* stelObj = (CStelephony*)stelObjPtr;

	stelObj->EnterStelCriticalSection();
	sizeTmp = stelephony_buffer_read_ulaw(pbuffer, data, dlen, max);
	stelObj->LeaveStelCriticalSection();
	return sizeTmp;
}

size_t STELAPI_CALL StelBufferReadAlaw(void *stelObjPtr, void *buffer, unsigned char *data, int* dlen, int max)
{
	size_t sizeTmp;
	CStelephony* stelObj = (CStelephony*)stelObjPtr;

	stelObj->EnterStelCriticalSection();
	sizeTmp = stelephony_buffer_read_alaw(buffer, data, dlen, max);
	stelObj->LeaveStelCriticalSection();
	return sizeTmp;
}

//////////////////////////////////////////////////////////
// Private global functions - Start

/*!
	\fn		void *stel_malloc(int size)
	\brief	allocate memory AND zero it
	\param	size	size of memory in bytes to allocate
	\return	pointer to allocated memory or NULL if allocation failed
*/
void *stel_malloc(int size)
{
	void *pMem = malloc(size);
	if(pMem){
		memset(pMem, 0x00, size);
	}
	return pMem;
}

// Private global functions - End
//////////////////////////////////////////////////////////