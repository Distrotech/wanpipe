#if defined (__LINUX__)
# include <stdlib.h>
# include <string.h>
# include <memory.h>
#endif
#include "PToneEncoder.h"

#define DBG_FSK			if(0)printf
#define DBG_DTMF		if(0)printf("%s():line:%d: ", __FUNCTION__, __LINE__);if(0)printf

#define MX_CID_LEN 15

#define SMG_DTMF_ON 	250
#define SMG_DTMF_OFF 	50

PhoneToneEncoder::PhoneToneEncoder(void)
{
	bufSize = 256;
	rate = 8000;

	fskInit = fskEnable = dtmfInit = dtmfEnable = false;

	memset(&sink_callback_functions, 0x00, sizeof(sink_callback_functions));

	fskData = (fsk_data_state_t*) stel_malloc(sizeof(fsk_data_state_t));
	if (fskData == NULL) {
		STEL_ERR("Failed to allocate memory (%s:%d)\n", __FUNCTION__,__LINE__);
		return;
	}

	fbuf = (unsigned char*) stel_malloc(bufSize);
	if (fbuf == NULL) {
		STEL_ERR("Failed to allocate memory (%s:%d)\n", __FUNCTION__,__LINE__);
		return;
	}

	fskTrans = (fsk_modulator_t*) stel_malloc(sizeof(fsk_modulator_t));
	if (fskTrans == NULL) {
		STEL_ERR("Failed to allocate memory (%s:%d)\n", __FUNCTION__,__LINE__);
		return;
	}

	userData = (helper_t*) stel_malloc(sizeof(helper_t));
	if (userData == NULL) {
		STEL_ERR("Failed to allocate memory (%s:%d)\n", __FUNCTION__,__LINE__);
		return;
	}

	dtmfSession = (teletone_generation_session_t*) stel_malloc(sizeof(teletone_generation_session_t));
	if (dtmfSession == NULL) {
		STEL_ERR("Failed to allocate memory (%s:%d)\n", __FUNCTION__,__LINE__);
		return;
	}
}

PhoneToneEncoder::~PhoneToneEncoder(void)
{
	STEL_FUNC_START();

	if (fskData) {
		DBG_FSK("Freeing fskData\n");
		free(fskData);
	}
	if (fbuf) {
		DBG_FSK("Freeing fbuf\n");
		free(fbuf);
	}

	if (userData) {
		DBG_FSK("Freeing userData\n");
		free(userData);
	}

	if (fskTrans) {
		DBG_FSK("Freeing fskTrans\n");
		free(fskTrans);
	}

	if (dtmfSession) {
		DBG_DTMF("Freeing dtmfSession\n");
		free(dtmfSession);
	}
	STEL_FUNC_END();
}

int PhoneToneEncoder::Init(void) 
{
	if (fskData) {
		memset(fskData, 0, sizeof(fsk_data_state_t));
	} else {
		STEL_ERR("Tone Encoder was not initialized (%s:%d)\n", __FUNCTION__,__LINE__);
		return -1;
	}
	if (fbuf) {
		memset(fbuf, 0, bufSize);
	} else {
		STEL_ERR("Tone Encoder was not initialized (%s:%d)\n", __FUNCTION__,__LINE__);
		return -1;
	}

	fsk_data_init(fskData, fbuf, bufSize);
	fskInit = true;

	if (dtmfSession) {
		memset(dtmfSession, 0, sizeof(teletone_generation_session_t));
	}
	return 0;
}

void PhoneToneEncoder::WaveStreamStart(void)
{
	int dtmf_size;

	dtmfInit = true;
	if (teletone_init_session(dtmfSession, 0, NULL, NULL)) {
		STEL_ERR("Failed to initialize SW DTMF Tone session\n");
		return;
	}

	dtmfSession->rate = rate;
	dtmfSession->duration = SMG_DTMF_ON * (dtmfSession->rate/1000);
	dtmfSession->wait = SMG_DTMF_OFF * (dtmfSession->rate/1000);

	dtmf_size=(SMG_DTMF_ON+SMG_DTMF_OFF)*10*2;

	buffer_create_dynamic(&(userData->dtmf_buffer), 1024, dtmf_size, 0);
}

void PhoneToneEncoder::WaveStreamEnd(void)
{
	dtmfInit = false;
	if (userData->dtmf_buffer) {
		buffer_zero(userData->dtmf_buffer);
	}
	return;
}

int PhoneToneEncoder::BufferGetSwDTMF(char dtmf_char, int *retValue)
{
	int wrote;
	int buffer_size;

	if (dtmfInit == false && dtmfEnable == false) {
		STEL_ERR("%s(): DTMF Encoder not ready\n", __FUNCTION__);
		return -1;
	}

	DBG_DTMF("Generating DTMF %c\n", dtmf_char);

	if (dtmf_char >= (int) (sizeof(dtmfSession->TONES)/sizeof(dtmfSession->TONES[0]))) {
		STEL_ERR("Unable to generate DTMF for %c\n", dtmf_char);
		return -1;
	}

	wrote = teletone_mux_tones(dtmfSession, &dtmfSession->TONES[(int) dtmf_char]);
	if (wrote) {
		buffer_size = buffer_write(userData->dtmf_buffer, (char*) dtmfSession->buffer, wrote); 
		if(buffer_size){
			//call DTMF generation callback so user will transmit it.
			if (sink_callback_functions.OnSwDtmfTransmit) {
				sink_callback_functions.OnSwDtmfTransmit(this->callback_obj, userData->dtmf_buffer);
			} else {
				STEL_ERR("No SwDtmfEconderCallback() function\n");
			}
		}else{
			STEL_ERR("buffer_write() returned %d!\n", buffer_size);
		}
	} else {
		STEL_ERR("Failed to generate tones\n");
		return -1;
	}
	
	return 0;
}


int PhoneToneEncoder::BufferGetFSKCallerID(stelephony_caller_id_t *cid_info, int *retValue)
{
	int nameStrLen;
	int numberStrLen;
	int dateStrLen;

	if (fskInit == false && fskEnable == false) {
		STEL_ERR("FSK Encoder not ready\n");
		return -1;
	}
	
	nameStrLen = strlen(cid_info->calling_name);
	numberStrLen = strlen(cid_info->calling_number);

	if (cid_info->auto_datetime) {
#if defined(__WINDOWS__)
		struct tm *tm;
		__time64_t now;

		_time64( &now );
		tm = _localtime64( &now );
		dateStrLen = strftime(cid_info->datetime, sizeof(cid_info->datetime), "%m%d%H%M", tm);
#else
		struct tm tm;
        time_t now;
	
		time(&now);

        localtime_r(&now, &tm);
		dateStrLen = strftime(cid_info->datetime, sizeof(cid_info->datetime), "%m%d%H%M", &tm);
#endif

	} else {
		dateStrLen = strlen(cid_info->datetime);
	}

	DBG_FSK("Generating CallerID Name:[%s] Number:[%s] DateTime:[%s]\n", cid_info->calling_name, cid_info->calling_number, cid_info->datetime);

	if (dateStrLen) {
		fsk_data_add_mdmf(fskData, MDMF_DATETIME, (uint8_t *)cid_info->datetime, dateStrLen);
	}

	if (numberStrLen) {
		fsk_data_add_mdmf(fskData, MDMF_PHONE_NUM, (uint8_t *)cid_info->calling_number, numberStrLen);
	}
	
	if (nameStrLen) {
		fsk_data_add_mdmf(fskData, MDMF_PHONE_NAME, (uint8_t *)cid_info->calling_name, nameStrLen);
	}
	
	buffer_create(&(userData->fsk_buffer), 128, 128, 0);

	fsk_data_add_checksum(fskData);

	fsk_modulator_init(fskTrans, FSK_BELL202, rate, fskData, -14, 180, 5, 300, fsk_write_sample, (void*) userData);

	fsk_modulator_send_all(fskTrans);
 	
	if (sink_callback_functions.OnFSKCallerIDTransmit) {
		sink_callback_functions.OnFSKCallerIDTransmit(this->callback_obj, userData->fsk_buffer);	
	} else {
		STEL_ERR("No OnFSKCallerIDTransmit callback function\n");
	}

	fsk_data_init(fskData, fbuf, bufSize);
	return 0;
}


void PhoneToneEncoder::SetCallbackFunctions(sink_callback_functions_t *cbf)
{
	memcpy(&sink_callback_functions, cbf, sizeof(sink_callback_functions_t));
	return;
}

void PhoneToneEncoder::GetCallbackFunctions(sink_callback_functions_t *cbf)
{
	memcpy(cbf, &sink_callback_functions, sizeof(sink_callback_functions_t));
	return;
}

void PhoneToneEncoder::SetCallbackFunctionsAndObject(sink_callback_functions_t* cbf, void *cbo)
{
	memcpy(&sink_callback_functions, cbf, sizeof(sink_callback_functions_t));

	callback_obj = cbo;
	return;
}

void PhoneToneEncoder::put_WaveFormatID(variant_t _var)
{
	DBG_FSK("Wave format:%s\n", 
				(_var.intVal==WFI_CCITT_uLaw_8kHzMono) ?"Alaw" :
				(_var.intVal==WFI_CCITT_uLaw_8kHzMono) ?"Ulaw" :
				"slinear");
	memcpy(&variant, &_var, sizeof(variant_t));
	return;
}

void PhoneToneEncoder::put_FeatureFSKCallerID(int val)
{
	DBG_FSK("FSK enabled\n");
	fskEnable = true;
}

void PhoneToneEncoder::put_FeatureSwDTMF(int val)
{
	if (val == false) {
		DBG_DTMF("DTMF disabled\n");
		dtmfEnable = false;
	} else {
		DBG_DTMF("DTMF enabled\n");
		dtmfEnable = true;
	}
}

