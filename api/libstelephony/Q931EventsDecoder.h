
#pragma once

#include <libstelephony.h>
#include "libstelephony_linux_compat.h"

#include <stdio.h>
#define DBG_Q931	if(0)printf

typedef struct{
	void	(*OnQ931Event)(void *callback_obj, stelephony_q931_event *pEvent);
}q931_event_decoder_callback_functions_t;

class Q931EventsDecoder
{
	q931_event_decoder_callback_functions_t	q931_event_decoder_callback_functions;
	void						*callback_obj;

	BOOL	m_IsEnabled;/* TRUE-Enabled, FALSE-Disabled */

public:
	Q931EventsDecoder(void);
	~Q931EventsDecoder(void);

	void SetCallbackFunctions(IN q931_event_decoder_callback_functions_t *cf);
	void SetCallbackObject(IN void *cbo);
	void GetCallbackFunctions(OUT q931_event_decoder_callback_functions_t *cf);

	stelephony_status_t EventControl(stelephony_event_t Event, stelephony_control_code_t ControlCode, void *optionalData);

	int Input(void *data, int dataLength);
};
