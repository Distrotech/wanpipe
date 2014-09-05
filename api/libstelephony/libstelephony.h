////////////////////////////////////////////////////////////////////////////////////
// libstelephony.h - declarations of function, structures and variables
//				exported by the Stelephony.dll. 
//
// Author	:	David Rokhvarg	<davidr@sangoma.com>
//
// Versions:
//	v 1,0,0,1	May  29		2008		David Rokhvarg	Initial Version
//		Implemented FSK Caller ID detection for Analog FXO.
//
//	v 1,0,0,2	June 12		2008		David Rokhvarg
//		Software DTMF detection
//
//	v 1,0,0,3	July		30	2008	Konrad Hammel
//		Q931 event detection
//
//	v 1,0,0,4	October		22	2008	David Rokhvarg
//		Updated declaration for proper "C" function exporting.
//
//	v 1,0,0,5	November	19	2008	David Rokhvarg
//		All exported function declared as WINAPI (__stdcall).
//
//	v 1,0,0,6	January		12	2009	David Rokhvarg
//		All exported function declared as __cdecl.
//
//	v 1,0,0,7	June		9	2009	David Rokhvarg
//		FSK CID and DTMF generation on Windows.
////////////////////////////////////////////////////////////////////////////////////

#ifndef __LIBSTELEPHONY_H_
#define __LIBSTELEPHONY_H_

#pragma once

#if defined (__WINDOWS__)
#ifdef STELEPHONY_EXPORTS
# define STELAPI_CALL __declspec(dllexport) __cdecl
#else
# define STELAPI_CALL __declspec(dllimport) __cdecl
#endif

# include <Windows.h>
#elif defined (__LINUX__)

# define STELAPI_CALL

/* Include headers */
# include <stddef.h>
# include <errno.h>
# include <fcntl.h>
# include <string.h>
# include <ctype.h>
# include <sys/stat.h>
# include <sys/ioctl.h>
# include <sys/types.h>
# include <dirent.h>
# include <unistd.h>
# include <sys/socket.h>
# include <netdb.h>
# include <sys/un.h>
# include <sys/wait.h>
# include <unistd.h>
# include <signal.h>
# include <time.h>
#endif

#ifdef __cplusplus
extern "C" {	/* for C++ users */
#endif

typedef enum _stelephony_status{

	STEL_STATUS_SUCCESS=0,
	STEL_STATUS_MEMORY_ALLOCATION_ERROR,
	STEL_STATUS_INVALID_INPUT_ERROR,
	STEL_STATUS_REQUEST_INVALID_FOR_CURRENT_OBJECT_STATE_ERROR,
	STEL_STATUS_DECODER_LIB_CO_CREATE_INSTANCE_REGDB_E_CLASSNOTREG_ERROR,
	STEL_STATUS_DECODER_LIB_CO_CREATE_INSTANCE_CLASS_E_NOAGGREGATION_ERROR,
	STEL_STATUS_DECODER_LIB_CO_CREATE_INSTANCE_E_NOINTERFACE_ERROR,
	STEL_STATUS_DECODER_LIB_CO_CREATE_INSTANCE_UNKNOWN_ERROR,
	STEL_STATUS_DECODER_LIB_QUERY_INTERFACE_ERROR,
	STEL_STATUS_DECODER_LIB_FIND_CONNECTION_POINT_ERROR,
	STEL_STATUS_DECODER_LIB_MEMORY_ALLOCATION_ERROR,
	STEL_STATUS_INVALID_EVENT_ERROR,
	STEL_STATUS_INVALID_EVENT_CONTROL_CODE_ERROR,
	STEL_STATUS_OBJECT_DISABLED_ERROR,
	STEL_STATUS_ENCODER_LIB_MEMORY_ALLOCATION_ERROR,
	STEL_STATUS_ENCODER_LIB_ERROR

}stelephony_status_t;

typedef enum _stelephony_event{

	STEL_EVENT_FSK_CALLER_ID=1,
	STEL_EVENT_DTMF,
	STEL_EVENT_Q931,
	STEL_FEATURE_FSK_CALLER_ID,
	STEL_FEATURE_SW_DTMF
}stelephony_event_t;


typedef enum _stelephony_control_code{

	STEL_CTRL_CODE_ENABLE=1,
	STEL_CTRL_CODE_DISABLE

}stelephony_control_code_t;

typedef enum _stelephony_option{

	STEL_OPTION_MULAW=1,
	STEL_OPTION_ALAW

}stelephony_option_t;

#ifdef WIN32
#define ST_SYSTEMTIME SYSTEMTIME 
#else
#define ST_SYSTEMTIME struct tm 
#endif



typedef struct {
	int				len_callRef;						//Length of call reference field
	char			callRef[6];							//call reference value, hex
	char			msg_type[25];						//Null terminated string containing the Q931 message type
	int				chan;								//b-channel call we be using
	int				called_num_digits_count;			//Number of Digits in the called number
	char			called_num_digits[32];				//Null terminated string containing the called number
	int				calling_num_digits_count;			//Number of digits in the calling number
	char			calling_num_digits[32];				//Null terminated string containing the calling number
	int				cause_code;							//the release cause code
	ST_SYSTEMTIME	tv;									//time value that message was received by Q931 decoder
	int				calling_num_screening_ind;
	int				calling_num_presentation;
	int				rdnis_digits_count;					//Number of RDNIS digits found
	char			rdnis_string[81];					//RDNIS value
	unsigned char	data[8188];							//the entire data
	int		dataLength;									//the length of the data

}stelephony_q931_event;

typedef struct {
	unsigned char	auto_datetime;			/* if set to non-zero library will adjust date to system time */
	char		datetime[16];
	char		calling_number[22];
	char		calling_name[52];
}stelephony_caller_id_t;


#ifdef WIN32
#define ST_STR char *
#else
#define ST_STR char * 
#endif

typedef struct{
	void	(*FSKCallerIDEvent)(void *callback_context, ST_STR Name, ST_STR CallerNumber, ST_STR CalledNumber, ST_STR DateTime);
	void	(*DTMFEvent)(void *callback_context, long Key);
	void	(*Q931Event)(void *callback_context, stelephony_q931_event *pEvent);
	void	(*FSKCallerIDTransmit)(void *callback_context, void* buffer);
	void	(*SwDTMFBuffer)(void *callback_context, void* buffer);
}stelephony_callback_functions_t;



stelephony_status_t STELAPI_CALL StelSetup(void **stelObj, void *callback_context, stelephony_callback_functions_t* scf);
stelephony_status_t STELAPI_CALL StelCleanup(void *stelObj);
stelephony_status_t STELAPI_CALL StelStreamInput(void *stelObj, void *data, int dataLength);
stelephony_status_t STELAPI_CALL StelEventControl(void *stelObjPtr, stelephony_event_t Event, stelephony_control_code_t ControlCode, void *optionalData);

stelephony_status_t STELAPI_CALL StelGenerateFSKCallerID(void *stelObj, stelephony_caller_id_t* cid_info);
stelephony_status_t STELAPI_CALL StelGenerateSwDTMF(void *stelObjPtr, char dtmf_char);

size_t STELAPI_CALL StelBufferInuse(void *stelObjPtr, void *buffer);
size_t STELAPI_CALL StelBufferReadUlaw(void *stelObjPtr, void *buffer, unsigned char *data, int* dlen, int max);
size_t STELAPI_CALL StelBufferReadAlaw(void *stelObjPtr, void *buffer, unsigned char *data, int* dlen, int max);
size_t STELAPI_CALL StelBufferRead(void *stelObjPtr, void *buffer, void *data, size_t datalen);

#ifdef __cplusplus
}
#endif


#endif
