#if defined (__LINUX__)
# include <stdlib.h>
# include <string.h>
# include <memory.h>
#elif defined(__WINDOWS__)
#endif

#include "PToneDecoder.h"

#define DBG_FSK		if(0)printf
#define DBG_DTMF	if(0)printf  

#define MX_CID_LEN 15

PhoneToneDecoder::PhoneToneDecoder(void)
{
	rate = 8000;
	bufSize = 256;
	printf("Init FSK demodulator rate:%d buffer:%zu\n", rate, bufSize);

	fskInit = fskEnable = dtmfInit = dtmfEnable = false;

	memset(&sink_callback_functions, 0x00, sizeof(sink_callback_functions));

	fskData = (fsk_data_state_t*) stel_malloc(sizeof(fsk_data_state_t));
	if (fskData == NULL) {
		STEL_ERR("Failed to allocate memory (%s:%d)\n", __FUNCTION__,__LINE__);
		return;
	}

	fbuf = (unsigned char*)stel_malloc(bufSize);
	if (fbuf == NULL) {
		STEL_ERR("Failed to allocate memory (%s:%d)\n", __FUNCTION__,__LINE__);
		return;
	}

	dtmfData = (teletone_dtmf_detect_state_t*) stel_malloc(sizeof(teletone_dtmf_detect_state_t));
	if (dtmfData == NULL) {
		STEL_ERR("Failed to allocate memory (%s:%d)\n", __FUNCTION__,__LINE__);
		return;
	}
}

PhoneToneDecoder::~PhoneToneDecoder(void)
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
	if (dtmfData) {
		DBG_DTMF("Freeing dtmfData\n");
		free(dtmfData);
	}
	STEL_FUNC_END();
}

int PhoneToneDecoder::Init()
{	
	if (fskData) {
		memset(fskData, 0, sizeof(fsk_data_state_t));
	} else {
		STEL_ERR("Tone Decoder was not initialized (%s:%d)\n", __FUNCTION__,__LINE__);
		return -1;
	}
	if (fbuf) {
		memset(fbuf, 0, bufSize);
	} else {
		STEL_ERR("Tone Decoder was not initialized (%s:%d)\n", __FUNCTION__,__LINE__);
		return -1;
	}

	if (dtmfData) {
		memset(dtmfData, 0, sizeof(teletone_dtmf_detect_state_t));
	} else {
		STEL_ERR("Dtmf Decoder was not initialized (%s:%d)\n", __FUNCTION__,__LINE__);
	}

	return 0; 
}

int PhoneToneDecoder::WaveStreamInputExFSK(int16_t* slinData, int dataLength, int *retvalue)
{
	char CallerNumber[22], CallerName[52], DateTime[16];
	memset(CallerNumber,0,sizeof(CallerNumber));
	memset(CallerName,0,sizeof(CallerName));
	memset(DateTime,0,sizeof(DateTime));

	if (fsk_demod_feed(fskData, (int16_t*) slinData, dataLength)) {
		size_t type, mlen;
		char str[128], *sp;

		DBG_FSK("Got FSK Data\n");	
		while(fsk_data_parse(fskData, &type, &sp, &mlen) == 0) {
			*(str+mlen) = '\0';
			strncpy(str, sp, (++mlen)-1);

			my_clean_string(str);

			//DBG_FSK("FSK: TYPE:%d LEN:%d VAL [%s]\n", type, mlen, str);
			switch(type) {
				case MDMF_DATETIME:
					strncpy(DateTime, sp, mlen-1);
					break;
				case MDMF_DDN:
				case MDMF_PHONE_NUM:
					strncpy(CallerNumber, sp, mlen-1);
					break;
				case MDMF_NO_NUM:
					memset(CallerNumber, 0, sizeof(CallerNumber));
					break;
				case MDMF_PHONE_NAME:
					strncpy(CallerName, sp, mlen-1);
					break;
				case MDMF_NO_NAME:
					memset(CallerName, 0, sizeof(CallerName));
					break;
			}
		}
		
		DBG_FSK("Date:[%s] Name:[%s] Number:[%s]\n",  DateTime, CallerName, CallerNumber);	
		if (sink_callback_functions.OnCallerID) {
			sink_callback_functions.OnCallerID(this->callback_obj, CallerName, CallerNumber, "Private", DateTime);	
		} else {
			STEL_ERR("hello: No FSK callback function\n");
		}
	}
	return 0;
}

int PhoneToneDecoder::WaveStreamInputExDtmf(int16_t* slinData, int dataLength, int *retvalue)
{
	char digit_str[100] = "";

	teletone_dtmf_detect(dtmfData, slinData, dataLength);
	teletone_dtmf_get(dtmfData, digit_str, sizeof(digit_str));
	if(*digit_str) {
		char* p;
		int len = strlen(digit_str);
		p = digit_str;

		//DBG_DTMF("DTMF DETECTED %s\n", digit_str);
		while(len--) {
			DBG_DTMF("DTMF DIGIT %c\n", *p);
			if (sink_callback_functions.OnDTMF) {
				sink_callback_functions.OnDTMF(this->callback_obj, *p);	
			} else {
				DBG_DTMF("No DTMF callback function\n");
			}
			p++;
		}
	}

	return 0;
}

int PhoneToneDecoder::WaveStreamInputEx(char* data, int dataLength, int *retValue)
{
	int16_t *slinData = NULL;
	int i;

	//DBG_FSK("RX data len:%d\n", dataLength);
	//STEL_FUNC_START();

	if (fskEnable == false && dtmfEnable == false) {
		return 0;
	}

	if (variant.intVal == WFI_CCITT_uLaw_8kHzMono || 
		variant.intVal == WFI_CCITT_ALaw_8kHzMono) {

		slinData = (int16_t*) stel_malloc(dataLength*2);
		if (slinData == NULL) {
			STEL_ERR("Failed to alloc mem (%s:%d)\n", __FUNCTION__,__LINE__);
			return -1;
		}
		memset( slinData, 0, dataLength*2 );
		if (variant.intVal==WFI_CCITT_uLaw_8kHzMono) {
			for(i=0; i<dataLength; i++) {
				/* Convert to linear data */
				slinData[i] = ulaw_to_linear(data[i]);
			}
		} else {
			/* Convert to linear data */
			for(i=0; i<dataLength; i++) {
				slinData[i] = alaw_to_linear(data[i]);
			}
		}
	} else {
		slinData = (int16_t*) data;
	}

	if (fskInit == true && fskEnable == true) {
		WaveStreamInputExFSK(slinData, dataLength, retValue);
	}

	if (dtmfInit == true && dtmfEnable == true) {
		WaveStreamInputExDtmf(slinData, dataLength, retValue);
	}
	
	if (variant.intVal == WFI_CCITT_uLaw_8kHzMono || 
		variant.intVal == WFI_CCITT_ALaw_8kHzMono) {

		free(slinData);
	}
	//STEL_FUNC_END();
	return 0;
}

void PhoneToneDecoder::SetCallbackFunctions(sink_callback_functions_t *cbf)
{
	DBG_FSK("%s(): cbf->OnCallerID: 0x%p\n", __FUNCTION__, cbf->OnCallerID);
	DBG_DTMF("%s(): cbf->OnDTMF: 0x%p\n", __FUNCTION__, cbf->OnDTMF);

	memcpy(&sink_callback_functions, cbf, sizeof(sink_callback_functions_t));
	return;
}

void PhoneToneDecoder::GetCallbackFunctions(sink_callback_functions_t *cbf)
{
	memcpy(cbf, &sink_callback_functions, sizeof(sink_callback_functions_t));
	return;
}

void PhoneToneDecoder::SetCallbackFunctionsAndObject(sink_callback_functions_t* cbf, void *cbo)
{
	DBG_FSK("%s(): cbf->OnCallerID: 0x%p\n", __FUNCTION__, cbf->OnCallerID);
	DBG_DTMF("%s(): cbf->OnDTMF: 0x%p\n", __FUNCTION__, cbf->OnDTMF);

	memcpy(&sink_callback_functions, cbf, sizeof(sink_callback_functions_t));

	callback_obj = cbo;
	return;
}

void PhoneToneDecoder::WaveStreamStart(void)
{	
	if (fsk_demod_init(fskData, rate, fbuf, bufSize)) {
		STEL_ERR("Failed to initialize FSK demodulator\n");
	} else {
		fskInit = true;
	}

	teletone_dtmf_detect_init(dtmfData, rate);

	dtmfInit = true;	
	return;
}

void PhoneToneDecoder::WaveStreamEnd(void)
{
	if (fskInit == true) {
		fsk_demod_destroy(fskData);	
	}
	fskInit = false;

	dtmfInit = false;
	return;
}
void PhoneToneDecoder::put_WaveFormatID(variant_t _var)
{
	DBG_FSK("Wave format:%s\n", 
				(_var.intVal==WFI_CCITT_uLaw_8kHzMono) ?"Alaw" :
				(_var.intVal==WFI_CCITT_uLaw_8kHzMono) ?"Ulaw" :
				"slinear");
	memcpy(&variant, &_var, sizeof(variant_t));
	return;
}
void PhoneToneDecoder::put_MonitorDTMF(bool val)
{
	DBG_DTMF("DTMF Decoding: %s\n", (val==true ? "enabled" : "disabled"));
	dtmfEnable = val;
	return;
}	
void PhoneToneDecoder::put_MonitorCallerID(bool val)
{	
	DBG_FSK("FSK Decoding: %s\n", (val==true? "enabled" : "disabled"));
	fskEnable = val;
	return;
}
