////////////////////////////////////////////////////////////////////////////////////
//stelephony.h - internal definitions of stelephony.dll
////////////////////////////////////////////////////////////////////////////////////

#include <stdio.h>

#include "libstelephony.h"
#include "Q931EventsDecoder.h" 
#include "Sink.h"

#include "PToneDecoder.h"
#include "PToneEncoder.h"

/*!
  \def STELEPHONY_VERSION
  \brief stelephony Version Number String Format
  \def STELEPHONY_VERSION_CODE
  \brief stelephony Version Number in HEX/Integer Format
  \def STELEPHONY_VERSION_TO_CODE
  \brief stelephony Version Macro used to convert version numbers to hex code
*/
#define STELEPHONY_VERSION "2.0.1"
#define STELEPHONY_VERSION_TO_CODE(a,b,c) (((a) << 16) + ((b) << 8) + (c))
#define STELEPHONY_VERSION_CODE  STELEPHONY_VERSION_TO_CODE(2,0,0) 

// This class will provide telepony services for a SINGLE thread
class CStelephony 
{
	CRITICAL_SECTION	m_CriticalSection;

	PhoneToneDecoder 	ToneDecoderObj;
	PhoneToneEncoder 	ToneEncoderObj;

	Q931EventsDecoder	Q931EventsDecoderObj;

	stelephony_status_t InitDecoderLib();
	stelephony_status_t CleanupDecoderLib();

	stelephony_status_t InitEncoderLib();
	stelephony_status_t CleanupEncoderLib();

	stelephony_callback_functions_t callback_functions;
	void *callback_context;

	stelephony_option_t m_ToneDecoderCodec;
	stelephony_option_t m_ToneEncoderCodec;

	stelephony_status_t ToneDecoderEventControl(stelephony_event_t Event, stelephony_control_code_t ControlCode, void *optionalData);
	stelephony_status_t ToneEncoderEventControl(stelephony_event_t Event, stelephony_control_code_t ControlCode, void *optionalData);

public:
	CStelephony();
	~CStelephony();

	void EnterStelCriticalSection();
	void LeaveStelCriticalSection();

	stelephony_status_t Init();
	stelephony_status_t Cleanup();

	void SetCallbackFunctions(IN stelephony_callback_functions_t *cbf);
	void GetCallbackFunctions(OUT stelephony_callback_functions_t *cbf);
	void SetCallbackContext(void *cbc);
	void *GetCallbackContext(void);

	int DecoderLibStreamInput(void *data, int dataLength);

	stelephony_status_t EncoderLibGenerateFSKCallerID(stelephony_caller_id_t *cid_info);

	stelephony_status_t EncoderLibGenerateSwDTMF(char dtmfChar);

	stelephony_status_t EventControl(stelephony_event_t Event, stelephony_control_code_t ControlCode, void *optionalData);
};
