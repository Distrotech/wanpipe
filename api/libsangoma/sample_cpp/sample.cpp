/*******************************************************************************//**
 * \file sample.cpp
 * \brief  WANPIPE(tm) API C++ Sample Code
 *
 * Author(s):	David Rokhvarg
 *				Nenad Corbic
 *
 * Copyright:	(c) 2005-2009  Sangoma Technologies
 *
 *
 * * Redistribution and use in source and binary forms, with or without
 *   modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the Sangoma Technologies nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY Sangoma Technologies ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL Sangoma Technologies BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * ===============================================================================
 *
 */

#include "sangoma_port.h"
#include "sangoma_port_configurator.h"	
#include "sangoma_interface.h"

#if defined(__LINUX__)
# include "sample_linux_compat.h"
#else
# include <conio.h>
#endif

#include <iostream>
#include <string>

using namespace std;

#ifndef MAX_PATH
#define MAX_PATH 100
#endif


/*****************************************************************
 * Global Variables
 *****************************************************************/

wp_program_settings_t	program_settings;
callback_functions_t 	callback_functions;


/*****************************************************************
 * Prototypes & Defines
 *****************************************************************/

static int got_rx_data(void *sang_if_ptr, void *rxhdr, void *rx_data);
static void got_tdm_api_event(void *sang_if_ptr, void *event_data);
#if USE_WP_LOGGER
static void got_logger_event(void *sang_if_ptr, wp_logger_event_t *logger_event);
#endif


#if USE_STELEPHONY_API
//Sangoma Telephony API (Stelephony.dll) provides the following telephony services:
//1. FSK Caller ID detection for Analog FXO.
//2. Software DTMF detection.
//3. Q931 decoding
static void FSKCallerIDEvent(void *callback_context, char * Name, char * CallerNumber, char * CalledNumber, char * DateTime);
static void DTMFEvent(void *callback_context, long Key);
static void Q931Event(void *callback_context, stelephony_q931_event *pQ931Event);
static void FSKCallerIDTransmit (void *callback_context, void* buffer);
static void SwDtmfTransmit (void *callback_context, void* buffer);
#endif

//critical section for synchronizing access to 'stdout' between the threads
CRITICAL_SECTION	PrintCriticalSection;
//critical section for TDM events
CRITICAL_SECTION	TdmEventCriticalSection;

/*****************************************************************
 * Debugging Macros
 *****************************************************************/

#define DBG_MAIN	if(1)printf
#define ERR_MAIN	printf("%s():line:%d:Error:", __FUNCTION__, __LINE__);printf
#define INFO_MAIN	if(1)printf

#define MAIN_FUNC() if(1)printf("%s():line:%d\n", __FUNCTION__, __LINE__)

static int set_port_configuration();

/*!
  \fn sangoma_interface* init(int wanpipe_number, int interface_number)
  \brief Create a sangoma_interface class and setup callback functions
  \param wanpipe_number wanpipe device number obtained from the hardware probe info, span
  \param interface_number wanpipe interface number, corresponds to a channel in a span
  \return sangoma_interface object pointer - ok  or NULL if error.
*/

sangoma_interface* init(int wanpipe_number, int interface_number)
{
	sangoma_interface	*sang_if = NULL;
	DBG_MAIN("%s()\n", __FUNCTION__);

	if(program_settings.use_ctrl_dev == 1){
		sang_if = new sangoma_api_ctrl_dev();
	}else if(program_settings.use_logger_dev == 1){
		sang_if = new sangoma_api_logger_dev();
	}else{
		sang_if = new sangoma_interface(wanpipe_number, interface_number);
	}

	if(sang_if->init(&callback_functions)){
		delete sang_if;
		return NULL;
	}

	DBG_MAIN("init(): OK\n");
	return sang_if;
}

/*!
  \fn void cleanup(sangoma_interface	*sang_if)
  \brief Free Sangoma Interface Object
  \param sang_if Sangoma interface object pointer
  \return void
*/
void cleanup(sangoma_interface	*sang_if)
{
	DBG_MAIN("cleanup()\n");
	if(sang_if){
		delete sang_if;
	}
}

/*!
  \fn int start(sangoma_interface	*sang_if)
  \brief Run the main sangoma interface hanlder code
  \param sang_if Sangoma interface object pointer
  \return 0 - ok  Non Zero - Error
*/
int start(sangoma_interface	*sang_if)
{
	DBG_MAIN("start()\n");
	return sang_if->run();
}

/*!
  \fn void stop(sangoma_interface	*sang_if)
  \brief Stop the Sangoma Interface Object
  \param sang_if Sangoma interface object pointer
  \return void
*/
void stop(sangoma_interface	*sang_if)
{
	DBG_MAIN("stop()\n");
	sang_if->stop();
}

/*!
  \fn void PrintRxData(wp_api_hdr_t *hdr, void *data)
  \brief Debug function used to print Rx Data
  \param pRx API data element strcutre containt header + data 
  \return void
*/
void PrintRxData(wp_api_hdr_t *hdr, void *pdata)
{
	unsigned short	datlen;
	unsigned char *	data;
	static unsigned int	rx_counter = 0;

	//NOTE: if running in BitStream mode, there will be TOO MUCH to print 
	datlen = hdr->data_length;
	data = (unsigned char*)pdata;

	rx_counter++;
	if(program_settings.silent){
		if((rx_counter % 1000) == 0){
			INFO_MAIN("Rx counter: %d, Rx datlen: %d\n", rx_counter, datlen);
#if 1
			INFO_MAIN("Timestamp: Seconds: %d, Microseconds: %d\n", 
				hdr->time_stamp_sec, hdr->time_stamp_usec);
#endif
		}
		return;
	}else{
		INFO_MAIN("Rx counter: %d, Rx datlen: %d. Data:\n", rx_counter, datlen);
	}

#if 1
	for(int ln = 0; ln < datlen; ln++){
		if((ln % 20 == 0)){
			if(ln){
				INFO_MAIN("\n");
			}
			INFO_MAIN("%04d ", ln/20);
		}
		INFO_MAIN("%02X ", data[ln]);
	}
	INFO_MAIN("\n");
#endif
}


/*!
  \fn static int got_rx_data(void *sang_if_ptr, void *rx_data)
  \brief Callback function indicating data rx is pending.
  \param sang_if_ptr sangoma interface pointer
  \param rx_data API data element strcutre containt header + data
  \return 0 - ok  non-zero - Error

  This function must return as fast as possible
  because it is called from real time receiver thread.
*/
static int got_rx_data(void *sang_if_ptr, void *rxhdr, void *rx_data)
{
	sangoma_interface *sang_if = (sangoma_interface*)sang_if_ptr;

#if 0
#ifdef __LINUX__
	static struct timeval tv_start;
	static int elapsed_b4=0;
	struct timeval last;
	int elapsed;

	last=tv_start;
	gettimeofday(&tv_start, NULL);
	elapsed = abs(elapsed_b4);
	elapsed_b4 = abs((((last.tv_sec * 1000) + last.tv_usec / 1000) - ((tv_start.tv_sec * 1000) + tv_start.tv_usec / 1000)));
	if (abs(elapsed - elapsed_b4) > 1) {
		INFO_MAIN("wanpipe%d: Elapsed %i %i diff=%i\n", program_settings.wanpipe_number, elapsed,elapsed_b4,abs(elapsed-elapsed_b4));
	}
#endif
#endif
	//Do something with data received from Sangoma interface.
	//Fore example, transimit back everything what was received:

	if(program_settings.Rx_to_Tx_loopback == 1){
		sang_if->transmit((wp_api_hdr_t*)rxhdr, rx_data);
	}

	EnterCriticalSection(&PrintCriticalSection);
	PrintRxData((wp_api_hdr_t*)rxhdr, rx_data);
	LeaveCriticalSection(&PrintCriticalSection);
	return 0;
}

/*!
  \fn static void got_tdm_api_event(void *sang_if_ptr, void *event_data)
  \brief Callback function indicating Event is pending.
  \param sang_if_ptr sangoma interface pointer
  \param event_data  API event element strcutre containt header + data
  \return 0 - ok  non-zero - Error


  Handling of Events must be done OUTSIDE of the REAL-TIME Rx thread
  because it may make take a lot of time.
  Create a special thread for Event hadling Or push Event into
  a queue - implentation is left to the user.
  In this example event is handled directly.
*/
static void got_tdm_api_event(void *sang_if_ptr, void *event_data)
{
	sangoma_interface	*sang_if = (sangoma_interface *)sang_if_ptr;
	wp_api_event_t		*wp_tdm_api_event = (wp_api_event_t *)event_data;
	wan_time_t			wan_time;
	char				*timestamp_str;

	EnterCriticalSection(&TdmEventCriticalSection);

	/* Windows: wan_time is 64bit, time_stamp_sec is 32bit 
	 * Linux: wan_time and time_stamp_sec is 32bit */
	wan_time = wp_tdm_api_event->time_stamp_sec;
	timestamp_str = sangoma_ctime( &wan_time );

	/* Display Logger Event Timestamp as UNIX-style Date string. */
	/*DBG_MAIN("Time and Date:\t\t%s\n", (timestamp_str == NULL ? "Invalid Timestamp" : timestamp_str));*/

	DBG_MAIN("%s(): Span: %d, Channel: %d (Seconds:%u, Microseconds:%u)\n", __FUNCTION__,	
		wp_tdm_api_event->wp_api_event_span, wp_tdm_api_event->wp_api_event_channel,
		wp_tdm_api_event->time_stamp_sec, wp_tdm_api_event->time_stamp_usec);

	switch(wp_tdm_api_event->wp_api_event_type)
	{
	case WP_API_EVENT_DTMF:/* DTMF detected by Hardware */
		DBG_MAIN("DTMF Event: Digit: %c (Port: %s, Type:%s)!\n",
			wp_tdm_api_event->wp_api_event_dtmf_digit,
			WAN_EC_DECODE_CHANNEL_PORT(wp_tdm_api_event->wp_api_event_dtmf_port), /* sout, rout, sin, rin */
			WAN_EC_DECODE_TONE_TYPE(wp_tdm_api_event->wp_api_event_dtmf_type) /* present or stop */	);
		break;

	case WP_API_EVENT_RXHOOK:
		DBG_MAIN("RXHOOK Event: %s! (0x%X)\n", 
			WAN_EVENT_RXHOOK_DECODE(wp_tdm_api_event->wp_api_event_hook_state),
			wp_tdm_api_event->wp_api_event_hook_state);
		break;

	case WP_API_EVENT_RING_DETECT:
		DBG_MAIN("RING Event: %s! (0x%X)\n",
			WAN_EVENT_RING_DECODE(wp_tdm_api_event->wp_api_event_ring_state),
			wp_tdm_api_event->wp_api_event_ring_state);
		break;

	case WP_API_EVENT_RING_TRIP_DETECT:
		DBG_MAIN("RING TRIP Event: %s! (0x%X)\n", 
			WAN_EVENT_RING_TRIP_DECODE(wp_tdm_api_event->wp_api_event_ring_state),
			wp_tdm_api_event->wp_api_event_ring_state);
		break;

	case WP_API_EVENT_RBS:
		DBG_MAIN("RBS Event: New bits: 0x%X!\n", wp_tdm_api_event->wp_api_event_rbs_bits);
		DBG_MAIN("RX RBS/CAS: ");
		wp_print_rbs_cas_bits(wp_tdm_api_event->wp_api_event_rbs_bits);
		break;

	case WP_API_EVENT_LINK_STATUS:
		DBG_MAIN("Link Status Event: %s! (0x%X)\n", 
			WAN_EVENT_LINK_STATUS_DECODE(wp_tdm_api_event->wp_api_event_link_status),
			wp_tdm_api_event->wp_api_event_link_status);
		break;

	case WP_API_EVENT_ALARM:
		DBG_MAIN("New Alarm State: 0x%X\n", wp_tdm_api_event->wp_api_event_alarm);
		break;

	case WP_API_EVENT_POLARITY_REVERSE:
		/* This event may have different meaning on different Telco lines.
		 * For example, it indicates "Network Initiated Clearing", 
		 * on a British Telecom line. But on some lines it means
		 * "Start of Caller ID transmission". Please consult with your Telco
		 * for exact meaning of event. */
		DBG_MAIN("Polarity Reversal Event: %s\n",
			WP_API_EVENT_POLARITY_REVERSE_DECODE(wp_tdm_api_event->wp_api_event_polarity_reverse));
		break;

	default:
		ERR_MAIN("Unknown TDM API Event: %d\n", wp_tdm_api_event->wp_api_event_type);
		break;
	}

	LeaveCriticalSection(&TdmEventCriticalSection);
	return;
}

#if USE_WP_LOGGER
/*!
  \fn static void got_logger_event(void *sang_if_ptr, wp_logger_event_t *logger_event)
  \brief Callback function indicating Logger Event is pending.
  \param sang_if_ptr sangoma interface pointer
  \param logger_event API Logger Event structure
  \return 0 - ok  non-zero - Error
*/
static void got_logger_event(void *sang_if_ptr, wp_logger_event_t *logger_event)
{
	char *timestamp_str = sangoma_ctime( &logger_event->time_stamp_sec );

	INFO_MAIN("Logger Event Type:\t%s (Logger:%d BitMap: 0x%08X)\n",
		wp_decode_logger_event_type(logger_event->logger_type, logger_event->event_type),
		logger_event->logger_type, logger_event->event_type);
	/* Display Logger Event Timestamp as UNIX-style Date string. */
	INFO_MAIN("Time and Date:\t\t%s\n", 
		(timestamp_str == NULL ? "Invalid Timestamp" : timestamp_str));
	INFO_MAIN("Logger Event Data: %s\n\n", logger_event->data);
}
#endif

/*!
  \fn int tx_file(sangoma_interface *sang_if)
  \brief Transmit a file on a sangoma interface / device.
  \param sang_if_ptr sangoma interface pointer
  \return 0 - ok  non-zero - Error

*/
int tx_file(sangoma_interface *sang_if)
{
	FILE			*pFile;
	unsigned int	tx_counter=0, bytes_read_from_file, total_bytes_read_from_file=0;
	wp_api_hdr_t	hdr;
	unsigned char	local_tx_data[SAMPLE_CPP_MAX_DATA_LEN];

	pFile = fopen( program_settings.szTxFileName, "rb" );
	if( pFile == NULL){
		ERR_MAIN( "Can't open file: [%s]\n", program_settings.szTxFileName );
		return 1;
	}

	do
	{
		//read tx data from the file. if 'bytes_read_from_file != txlength', end of file is reached
		bytes_read_from_file = fread( local_tx_data, 1, program_settings.txlength /* MTU size */, pFile );
		total_bytes_read_from_file += bytes_read_from_file;

		hdr.data_length = program_settings.txlength;//ALWAYS transmit MTU size over the BitStream/Voice interface
		hdr.operation_status = SANG_STATUS_TX_TIMEOUT;
		
		if(SANG_STATUS_SUCCESS != sang_if->transmit(&hdr, local_tx_data)){
			//error
			break;
		}
		
		tx_counter++;
		
		//DBG_MAIN("tx_counter: %u\r",tx_counter);

	}while(bytes_read_from_file == program_settings.txlength);

	INFO_MAIN("%s: Finished transmitting file \"%s\" (tx_counter: %u, total_bytes_read_from_file: %d)\n",
		sang_if->device_name, program_settings.szTxFileName, tx_counter, total_bytes_read_from_file);

	fclose( pFile );
	return 0;
}

/*!
  \fn static int get_user_decimal_number()
  \brief Get user DECIMAL input in integer format
  \return user inputed integer
*/
static int get_user_decimal_number()
{
	int result = 1;
	int retry_counter = 0;

	while(scanf("%d", &result) == 0){
		fflush( stdin );
		INFO_MAIN("\nError: Not a numerical input!!\n");
		if(retry_counter++ > 10){
			INFO_MAIN("giving up...\n");
			result = 1;
			break;
		}
	}//while()

	INFO_MAIN("User input: %d\n", result);
	return result;
}

/*!
  \fn static int get_user_hex_number()
  \brief Get user HEX input in integer format
  \return user inputed integer
*/
static int get_user_hex_number()
{
	int result = 1;
	int retry_counter = 0;

	while(scanf("%x", &result) == 0){
		fflush( stdin );
		INFO_MAIN("\nError: Not a HEX input!!\n");
		if(retry_counter++ > 10){
			INFO_MAIN("giving up...\n");
			result = 1;
			break;
		}
	}//while()

	INFO_MAIN("User input: 0x%X\n", result);
	return result;
}

/*!
  \fn static int parse_command_line_args(int argc, char* argv[])
  \brief Parse input arguments
  \param argc number of arguments
  \param argv argument list
  \return 0 - ok  non-zero - Error
*/
static int parse_command_line_args(int argc, char* argv[])
{
	int i;
	const char *USAGE_STR =
"\n"
"Usage: sample [-c] [-i] [options]\n"
"\n"
"\t-c		number	Wanpipe (Port/Span) number: 1,2,3...\n"
"\t-i		number	Interface number 1,2,3,....\n"
"options:\n"
"\t-silent			Disable display of Rx data\n"
"\t-driver_config	\tStop/Set Configuration/Start a Port....\n"
"\t-rx2tx			All received data automatically transmitted on\n"
"\t       			the SAME interface\n"
"\t-txlength\tnumber\tLength of data frames to be transmitted when 't'\n"
"\t         \t      \tkey is pressed\n"
"\t-txcount\tnumber	Number of test data frames to be transmitted when 't'\n"
"\t        \t       \tkey is pressed\n"
"\t-tx_file_name\tstring\tFile to be transmitted when 't' key is pressed\n"
#if USE_STELEPHONY_API
"\t-decode_fsk_cid\t\tDecode FSK Caller ID on an Analog line.\n"
"\t               \t\tFor Voice data only.\n"
"\t-encode_fsk_cid\t\tEncode FSK Caller ID on an Analog line.\n"
"\t               \t\tFor Voice data only.\n"
"\t-encode_sw_dtmf\t\tEncode SW DTMF on an line. For Voice data only.\n"
"\t-sw_dtmf		Enable Sangoma Software DTMF decoder. For Voice data only.\n"
"\t-decode_q931		Enable Sangoma Q931 decoder. For HDLC (Dchannel) data only.\n"
"\t-alaw\t\t	Use Alaw codec instead of default MuLaw codec for Voice data.\n"
"\t-rm_txgain\t	Set txgain for FXS/FXO modules.\n"
"\t          \t\tFXO range from -150 to 120, FXS 35 or -35\n"
"\t-rm_rxgain\t	Set txgain for FXS/FXO modules.\n"
"\t          \t\tFXO range from -150 to 120, FXS 35 or -35\n"
#endif
#if 0
"\t-use_ctrl_dev	\tUse the global 'wptdm_ctrl' device to Get events from\n"
"\t				\tall API devices.\n"
#endif
"\t-use_logger_dev	\tUse the global Logger device to Get Log Messages\n"
"\t                 \tfrom API driver.\n"
#ifdef WP_API_FEATURE_LIBSNG_HWEC
"\t-use_hwec	\tInitialize/Configure/Use the Hardware Echo Canceller\n"
"\t-hwec_chan	\tA 1-based channel number to be used by per-channel (vs. global)\n"
"\t         	\tHWEC functions. Valid values: 1-31\n"
#endif
#if USE_BUFFER_MULTIPLIER
"\t-buff_mult\tnumber\tBuffer Multiplier coefficient\n"
#endif
"\t-real_time	\tRun the Program at real-time priority. This maybe\n"
"\t             \t\timportant when Audio stream is used for timing.\n"
"\n"
"Example: sample -c 1 -i 1\n";

	memset(&program_settings, 0, sizeof(wp_program_settings_t));
	program_settings.wanpipe_number = 1;
	program_settings.interface_number = 1;
	program_settings.txlength = 80;
	program_settings.txcount = 1;
	program_settings.rxgain = 0xFFFF;	//FXO/FXS rx gain unchanged	
	program_settings.txgain = 0xFFFF;	//FXO/FXS tx gain unchanged
	program_settings.hwec_channel = 1;
#if USE_BUFFER_MULTIPLIER
	program_settings.buffer_multiplier = MIN_BUFFER_MULTIPLIER_FACTOR; //1
#endif

	for(i = 1; i < argc;){
	
		if(_stricmp(argv[i], "-silent") == 0){
			INFO_MAIN("disabling Rx data display...\n");
			program_settings.silent = 1;
		}else if(_stricmp(argv[i], "help") == 0 || _stricmp(argv[i], "?") == 0 || _stricmp(argv[i], "/?") == 0){
			INFO_MAIN(USAGE_STR);
			return 1;
		}else if(_stricmp(argv[i], "-c") == 0){
			if (i+1 > argc-1){
				INFO_MAIN("No Wanpipe number was provided!\n");
				return 1;
			}
			program_settings.wanpipe_number = (uint16_t)atoi(argv[i+1]);
			INFO_MAIN("Using wanpipe number %d\n", program_settings.wanpipe_number);
			i++;
		}else if(_stricmp(argv[i], "-i") == 0){
			if (i+1 > argc-1){
				INFO_MAIN("No Interface number was provided!\n");
				return 1;
			}
			program_settings.interface_number = (uint16_t)atoi(argv[i+1]);
			INFO_MAIN("Using interface number %d\n", program_settings.interface_number);
			if(program_settings.interface_number < 1){
				ERR_MAIN("Invalid interface number %d!!\n", program_settings.interface_number);
				return 1;
			}
			i++;
		}else if(strcmp(argv[i], "-rx2tx") == 0){
			INFO_MAIN("enabling Rx to Tx loopback...\n");
			program_settings.Rx_to_Tx_loopback = 1;
		}else if(strcmp(argv[i], "-driver_config") == 0){
			INFO_MAIN("enabling driver config start/stop\n");
			program_settings.driver_config = 1;

		}else if(_stricmp(argv[i], "-txlength") == 0){
			if (i+1 > argc-1){
				INFO_MAIN("No txlength provided!\n");
				return 1;
			}
			program_settings.txlength = (uint16_t)atoi(argv[i+1]);
			INFO_MAIN("Setting txlength to %d bytes.\n", program_settings.txlength);
			i++;
		}else if(_stricmp(argv[i], "-txcount") == 0){
			if (i+1 > argc-1){
				INFO_MAIN("No txcount provided!\n");
				return 1;
			}
			program_settings.txcount = atoi(argv[i+1]);
			i++;
			INFO_MAIN("Setting txcount to %d.\n", program_settings.txcount);
#if USE_STELEPHONY_API
		}else if(_stricmp(argv[i], "-decode_fsk_cid") == 0){
			INFO_MAIN("enabling FSK Caller ID decoder...\n");
			program_settings.decode_fsk_cid = 1;
			callback_functions.FSKCallerIDEvent = FSKCallerIDEvent;
		}else if(_stricmp(argv[i], "-sw_dtmf") == 0){
			INFO_MAIN("enabling Software DTMF decoder...\n");
			program_settings.sw_dtmf = 1;
			callback_functions.DTMFEvent = DTMFEvent;
		}else if(_stricmp(argv[i], "-decode_q931") == 0){
			INFO_MAIN("enabling Q931 decoder...\n");
			program_settings.decode_q931 = 1;
			callback_functions.Q931Event = Q931Event;
		}else if(_stricmp(argv[i], "-encode_fsk_cid") == 0){
			INFO_MAIN("enabling FSK Caller ID encoder...\n");
			program_settings.encode_fsk_cid = 1;
			callback_functions.FSKCallerIDTransmit = FSKCallerIDTransmit;
		}else if(_stricmp(argv[i], "-encode_sw_dtmf") == 0){
			INFO_MAIN("enabling Software DTMF encoder...\n");
			program_settings.encode_sw_dtmf = 1;
			callback_functions.SwDtmfTransmit = SwDtmfTransmit;
		}else if(_stricmp(argv[i], "-alaw") == 0){
			INFO_MAIN("enabling ALaw codec...\n");
			program_settings.voice_codec_alaw = 1;
#endif//USE_STELEPHONY_API
		}else if(_stricmp(argv[i], "-tx_file_name") == 0){
			if (i+1 > argc-1){
				INFO_MAIN("No TxFileName provided!\n");
				return 1;
			}
			strcpy(program_settings.szTxFileName, argv[i+1]);
			i++;
			INFO_MAIN("Setting szTxFileName to '%s'.\n", program_settings.szTxFileName);
		}else if(_stricmp(argv[i], "-use_ctrl_dev") == 0){
			INFO_MAIN("Using ctrl_dev...\n");
			program_settings.use_ctrl_dev = 1;
		}else if(_stricmp(argv[i], "-use_logger_dev") == 0){
			INFO_MAIN("Using logger_dev...\n");
			program_settings.use_logger_dev = 1;
		}else if(_stricmp(argv[i], "-rm_txgain") == 0){
			if (i+1 > argc-1){
				INFO_MAIN("No Tx gain provided!\n");
				return 1;
			}
			program_settings.txgain = atoi(argv[i+1]);
			i++;
			INFO_MAIN("Setting Tx gain to %d.\n", program_settings.txgain);
		}else if(_stricmp(argv[i], "-rm_rxgain") == 0){
			if (i+1 > argc-1){
				INFO_MAIN("No Rx gain provided!\n");
				return 1;
			}
			program_settings.rxgain = atoi(argv[i+1]);
			i++;
			INFO_MAIN("Setting Rx gain to %d.\n", program_settings.txgain);
		}else if(_stricmp(argv[i], "-use_hwec") == 0){
			INFO_MAIN("Using hardware echo canceller...\n");
			program_settings.use_hardware_echo_canceller = 1;
		}else if(_stricmp(argv[i], "-hwec_chan") == 0){
			if (i+1 > argc-1){
				INFO_MAIN("No hwec_chan provided!\n");
				return 1;
			}
			program_settings.hwec_channel = atoi(argv[i+1]);
			i++;
			INFO_MAIN("Setting hwec_chan to %d.\n", program_settings.hwec_channel);
		}else if(_stricmp(argv[i], "-real_time") == 0){
			INFO_MAIN("Will be running at real-time priority...\n");
			program_settings.real_time = 1;
#if USE_BUFFER_MULTIPLIER
		}else if(_stricmp(argv[i], "-buff_mult") == 0){
			if (i+1 > argc-1){
				INFO_MAIN("No buff_mult provided!\n");
				return 1;
			}
			program_settings.buffer_multiplier = atoi(argv[i+1]);
			i++;
			INFO_MAIN("Setting buff_mult to %d.\n", program_settings.buffer_multiplier);
#endif
		}else{
			INFO_MAIN("Error: Invalid Argument %s\n",argv[i]);
			return 1;
		}
		i++;
	}
	return 0;
}


int my_getch()
{
	string str_input = "";

	std::getline(cin, str_input);

	return str_input.c_str()[0];
}


/*!
  \fn int main(int argc, char* argv[])
  \brief Main function start of the applicatoin
  \param argc number of arguments
  \param argv argument list
  \return 0 - ok  non-zero - Error

  Get user Input
  Set program settings based on user input
  Create SangomaInterface Class based on user span/chan input.
  Bind callback functions read/event to SangomaInteface class.
  Execute the SangomaInterface handling function -> start()
  The start function will read/write/event data.
  In Main thread prompt the user for commands.
*/
int __cdecl main(int argc, char* argv[])
{
	int		rc, user_selection,err;
	sangoma_interface	*sang_if = NULL;
	wp_api_hdr_t		hdr;
	unsigned char		local_tx_data[SAMPLE_CPP_MAX_DATA_LEN];
	UCHAR				tx_test_byte = 0;

	////////////////////////////////////////////////////////////////////////////
	memset(&callback_functions, 0x00, sizeof(callback_functions));
	callback_functions.got_rx_data = got_rx_data;
	callback_functions.got_tdm_api_event = got_tdm_api_event;
#if USE_WP_LOGGER
	callback_functions.got_logger_event = got_logger_event;
#endif
	////////////////////////////////////////////////////////////////////////////
	if(parse_command_line_args(argc, argv)){
		return 1;
	}

	////////////////////////////////////////////////////////////////////////////
	//An OPTIONAL step of setting the port configuration to different values from
	//what is set in "Device Manager"-->"Sangoma Hardware Abstraction Driver".

	//set port configration and exit
	if (program_settings.driver_config) {
		err=set_port_configuration();
		if (err) {
			return err;
		}
	}

	////////////////////////////////////////////////////////////////////////////
	//initialize critical section objects
	InitializeCriticalSection(&PrintCriticalSection);
	InitializeCriticalSection(&TdmEventCriticalSection);

	if (program_settings.real_time) {
		sng_set_process_priority_to_real_time();
	}

	////////////////////////////////////////////////////////////////////////////
	//User may provide Wanpipe Number and Interface Number as a command line arguments:
	INFO_MAIN("Using wanpipe_number: %d, interface_number: %d\n", program_settings.wanpipe_number, program_settings.interface_number);

	sang_if = init(program_settings.wanpipe_number, program_settings.interface_number);

	if(sang_if == NULL){
		return 1;
	}

	rc = start(sang_if);
	if(rc){
		cleanup(sang_if);
		return rc;
	}

	if(program_settings.txgain !=  0xFFFF) {
		INFO_MAIN("Applying  txgain...\n");
		if (sang_if->tdm_control_rm_txgain(program_settings.txgain)){
			INFO_MAIN("Failed to apply txgain!\n");
		}
	}
	
	/* FXS cannot detect if phone is connected or not when the card is started
		therefore tranmit following two events for Set Polarity to work */
	if(sang_if->get_adapter_type() == WAN_MEDIA_FXOFXS && sang_if->get_sub_media() == MOD_TYPE_FXS) {
			INFO_MAIN("Setting Proper hookstates on FXS\n");
			sang_if->tdm_txsig_offhook();
			sang_if->tdm_txsig_onhook();
	}

	if(program_settings.rxgain !=  0xFFFF) {
		INFO_MAIN("Applying rxgain...\n");
		if (sang_if->tdm_control_rm_rxgain(program_settings.rxgain)){
			INFO_MAIN("Failed to apply rxgain!\n");
		}
	}
	do{
		EnterCriticalSection(&PrintCriticalSection);
		
		INFO_MAIN("Press 'q' to quit the program.\n");
		INFO_MAIN("Press 's' to get Operational Statistics.\n");
		INFO_MAIN("Press 'f' to reset (flush) Operational Statistics.\n");

		if(program_settings.use_logger_dev != 1){
			/* these options valid for non-logger api devices */
			INFO_MAIN("Press 't' to transmit data.\n");
			INFO_MAIN("Press 'v' to get API driver version.\n");

			if(sang_if->get_adapter_type() == WAN_MEDIA_T1 || sang_if->get_adapter_type() == WAN_MEDIA_E1){
				INFO_MAIN("Press 'a' to get T1/E1 alarms.\n");
				//RBS (CAS) commands
				INFO_MAIN("Press 'g' to get RBS bits.\n");
				INFO_MAIN("Press 'r' to set RBS bits.\n");
				INFO_MAIN("Press '1' to read FE register. Warning: used by Sangoma Techsupport only!\n");
				INFO_MAIN("Press '2' to write FE register.  Warning: used by Sangoma Techsupport only!\n");
			}
			INFO_MAIN("Press 'i' to set Tx idle data buffer (BitStream only).\n");
			switch(sang_if->get_adapter_type())
			{
			case WAN_MEDIA_T1:
				//those commands valid only for T1
				INFO_MAIN("Press 'l' to send 'activate remote loop back' signal.\n");
				INFO_MAIN("Press 'd' to send 'deactivate remote loop back' signal.\n");
				break;
			case WAN_MEDIA_FXOFXS:
				switch(sang_if->get_sub_media())
				{
				case MOD_TYPE_FXS:
					INFO_MAIN("Press 'e' to listen to test tones on a phone connected to the A200-FXS\n");
					INFO_MAIN("Press 'c' to ring/stop ring phone connected to the A200-FXS\n");
					INFO_MAIN("Press 'n' to enable/disable reception of ON/OFF Hook events on A200-FXS\n");
					INFO_MAIN("Press 'm' to enable DTMF events (on SLIC chip) on A200-FXS\n");
					INFO_MAIN("Press 'j 'to enable/disable reception of Ring Trip events on A200-FXS\n");
					INFO_MAIN("Press 'k' to transmit kewl - drop line voltage on the line connected to the A200-FXS\n");
					INFO_MAIN("Press 'h' to set polarity on the line connected to the A200-fXS\n");
					INFO_MAIN("Press 'u' to transmit onhooktransfer on the line connected to the A200-FXS\n");
					break;

				case MOD_TYPE_FXO:
					INFO_MAIN("Press 'u' to enable/disable reception of Ring Detect events on A200-FXO\n");
					INFO_MAIN("Press 'h' to transmit ON/OFF hook signals on A200-FXO\n");
					INFO_MAIN("Press 'a' to get Line Status (Connected/Disconnected)\n");
					break;
				}
				break;
			case WAN_MEDIA_BRI:
				INFO_MAIN("Press 'k' to Activate/Deactivate ISDN BRI line\n");
				INFO_MAIN("Press 'l' to enable	bri bchan loopback\n");
				INFO_MAIN("Press 'd' to disable bri bchan loopback\n");
				break;
			}
			INFO_MAIN("Press 'o' to control DTMF events on DSP (Octasic)\n");
			if (program_settings.encode_sw_dtmf) {
				INFO_MAIN("Press 'x' to send software DTMF\n");
			}
			if (program_settings.encode_fsk_cid) {
				INFO_MAIN("Press 'z' to send software FSK Caller ID\n");
			}
		}//if(program_settings.use_logger_dev != 1)

		LeaveCriticalSection(&PrintCriticalSection);
		user_selection = tolower(my_getch());
		switch(user_selection)
		{
		case 'q':
			break;
		case 't':
			for(u_int32_t cnt = 0; cnt < program_settings.txcount; cnt++){
				if(program_settings.szTxFileName[0]){
					tx_file(sang_if);
				}else{
					hdr.data_length = program_settings.txlength;
					hdr.operation_status = SANG_STATUS_TX_TIMEOUT;
					//set the actual data for transmission
					memset(local_tx_data, tx_test_byte, program_settings.txlength);
					sang_if->transmit(&hdr, local_tx_data);
					tx_test_byte++;
				}
			}
			break;
		case 's':
			if(program_settings.use_logger_dev == 1){
				wp_logger_stats_t stats;
				/* Warning: for demonstration purposes only, it is assumed,
				 *			that 'sang_if' is 'sangoma_api_logger_dev'. */
				((sangoma_api_logger_dev*)sang_if)->get_logger_dev_operational_stats(&stats);
			}else{
				wanpipe_chan_stats_t stats;
				sang_if->get_operational_stats(&stats);
			}
			break;
		case 'f':
			sang_if->flush_operational_stats();
			break;
		case 'v':
			{
				DRIVER_VERSION version;
				//read API driver version
				sang_if->get_api_driver_version(&version);
				INFO_MAIN("\nAPI version\t: %d,%d,%d,%d\n",
					version.major, version.minor, version.minor1, version.minor2);

				u_int8_t customer_id = 0;
				sang_if->get_card_customer_id(&customer_id);
				INFO_MAIN("\ncustomer_id\t: 0x%02X\n", customer_id);
			}
			break;
		case 'a':
			unsigned char cFeStatus;

			switch(sang_if->get_adapter_type())
			{
			case WAN_MEDIA_T1:
			case WAN_MEDIA_E1:
				//read T1/E1/56k alarms
				sang_if->get_te1_56k_stat();
				break;

			case WAN_MEDIA_FXOFXS:
				switch(sang_if->get_sub_media())
				{
				case MOD_TYPE_FXO:
					cFeStatus = 0;
					sang_if->tdm_get_front_end_status(&cFeStatus);
					INFO_MAIN("cFeStatus: %s (%d)\n", FE_STATUS_DECODE(cFeStatus), cFeStatus);
					break;
				}
			}
			break;
		case 'l':
			switch(sang_if->get_adapter_type())
			{
			case WAN_MEDIA_T1:
			case WAN_MEDIA_E1:
				//Activate Line/Remote Loopback mode:
				sang_if->set_lb_modes(WAN_TE1_LINELB_MODE, WAN_TE1_LB_ENABLE);
				//Activate Diagnostic Digital Loopback mode:
				//sang_if->set_lb_modes(WAN_TE1_DDLB_MODE, WAN_TE1_LB_ENABLE);
				//sang_if->set_lb_modes(WAN_TE1_PAYLB_MODE, WAN_TE1_LB_ENABLE);
				break;
			case WAN_MEDIA_BRI:
				sang_if->tdm_enable_bri_bchan_loopback(WAN_BRI_BCHAN1);
				break;
			}
			break;
		case 'd':
			switch(sang_if->get_adapter_type())
			{
			case WAN_MEDIA_T1:
			case WAN_MEDIA_E1:
				//Deactivate Line/Remote Loopback mode:
				sang_if->set_lb_modes(WAN_TE1_LINELB_MODE, WAN_TE1_LB_DISABLE);
				//Deactivate Diagnostic Digital Loopback mode:
				//sang_if->set_lb_modes(WAN_TE1_DDLB_MODE, WAN_TE1_LB_DISABLE);
				//sang_if->set_lb_modes(WAN_TE1_PAYLB_MODE, WAN_TE1_LB_DISABLE);
				break;
			case WAN_MEDIA_BRI:
				sang_if->tdm_disable_bri_bchan_loopback(WAN_BRI_BCHAN1);
				break;
			}
			break;
		case 'g'://read RBS bits
			switch(sang_if->get_adapter_type())
			{
			case WAN_MEDIA_T1:
			case WAN_MEDIA_E1:
				{
					rbs_management_t rbs_management_struct = {0,0};

					sang_if->enable_rbs_monitoring();

					INFO_MAIN("Type Channel number and press <Enter>:\n");
					rbs_management_struct.channel = get_user_decimal_number();//channels (Time Slots). Valid values: 1 to 24.

					if(WAN_MEDIA_T1 == sang_if->get_adapter_type()){
						if(rbs_management_struct.channel < 1 || rbs_management_struct.channel > 24){
							INFO_MAIN("Invalid T1 RBS Channel number!\n");
							break;
						}
					}

					if(WAN_MEDIA_E1 == sang_if->get_adapter_type()){
						if(rbs_management_struct.channel < 1 || rbs_management_struct.channel > 31){
							INFO_MAIN("Invalid E1 CAS Channel number!\n");
							break;
						}
					}

					sang_if->get_rbs(&rbs_management_struct);
				}
				break;
			default:
				INFO_MAIN("Command invalid for card type\n");
				break;
			}//switch()
			break;
		case 'r'://set RBS bits
			switch(sang_if->get_adapter_type())
			{
			case WAN_MEDIA_T1:
			case WAN_MEDIA_E1:
				{
					static rbs_management_t rbs_management_struct = {0,0};
					int chan_no;

					sang_if->enable_rbs_monitoring();

					INFO_MAIN("Type Channel number and press <Enter>:\n");
					rbs_management_struct.channel = chan_no = get_user_decimal_number();//channels (Time Slots). Valid values: T1: 1 to 24; E1: 1-15 and 17-31.

					if(WAN_MEDIA_T1 == sang_if->get_adapter_type()){
						if(rbs_management_struct.channel < 1 || rbs_management_struct.channel > 24){
							INFO_MAIN("Invalid T1 RBS Channel number!\n");
							break;
						}
					}

					if(WAN_MEDIA_E1 == sang_if->get_adapter_type()){
						if(rbs_management_struct.channel < 1 || rbs_management_struct.channel > 31){
							INFO_MAIN("Invalid E1 CAS Channel number!\n");
							break;
						}
					}

					/*	bitmap - set as needed: WAN_RBS_SIG_A |	WAN_RBS_SIG_B | WAN_RBS_SIG_C | WAN_RBS_SIG_D;

					In this example make bits A and B to change each time,
					so it's easy to see the change on the receiving side.
					*/
					if(rbs_management_struct.ABCD_bits == WAN_RBS_SIG_A){
						rbs_management_struct.ABCD_bits = WAN_RBS_SIG_B;
					}else{
						rbs_management_struct.ABCD_bits = WAN_RBS_SIG_A;
					}
					sang_if->set_rbs(&rbs_management_struct);

#if 0
#define RBS_CAS_TX_AND_WAIT(chan, abcd)	\
{	\
	rbs_management_struct.channel = chan;	\
	rbs_management_struct.ABCD_bits = abcd;		\
	INFO_MAIN("Press any key to transmit bits:");	\
	wp_print_rbs_cas_bits(abcd);	\
	my_getch();	\
	sang_if->set_rbs(&rbs_management_struct);	\
}

					//for ESF lines test all all combinations of 4 bits
					RBS_CAS_TX_AND_WAIT(chan_no, 0x00);

					RBS_CAS_TX_AND_WAIT(chan_no, WAN_RBS_SIG_A);

					RBS_CAS_TX_AND_WAIT(chan_no, WAN_RBS_SIG_B);

					RBS_CAS_TX_AND_WAIT(chan_no, WAN_RBS_SIG_A | WAN_RBS_SIG_B);

					RBS_CAS_TX_AND_WAIT(chan_no, WAN_RBS_SIG_C);

					RBS_CAS_TX_AND_WAIT(chan_no, WAN_RBS_SIG_A | WAN_RBS_SIG_C);

					RBS_CAS_TX_AND_WAIT(chan_no, WAN_RBS_SIG_B | WAN_RBS_SIG_C);

					RBS_CAS_TX_AND_WAIT(chan_no, WAN_RBS_SIG_A | WAN_RBS_SIG_B | WAN_RBS_SIG_C);

					RBS_CAS_TX_AND_WAIT(chan_no, WAN_RBS_SIG_D);

					RBS_CAS_TX_AND_WAIT(chan_no, WAN_RBS_SIG_A | WAN_RBS_SIG_D);

					RBS_CAS_TX_AND_WAIT(chan_no, WAN_RBS_SIG_B | WAN_RBS_SIG_D);

					RBS_CAS_TX_AND_WAIT(chan_no, WAN_RBS_SIG_A | WAN_RBS_SIG_B | WAN_RBS_SIG_D);

					RBS_CAS_TX_AND_WAIT(chan_no, WAN_RBS_SIG_C | WAN_RBS_SIG_D);

					RBS_CAS_TX_AND_WAIT(chan_no, WAN_RBS_SIG_A | WAN_RBS_SIG_C | WAN_RBS_SIG_D);

					RBS_CAS_TX_AND_WAIT(chan_no, WAN_RBS_SIG_B | WAN_RBS_SIG_C | WAN_RBS_SIG_D);

					RBS_CAS_TX_AND_WAIT(chan_no, WAN_RBS_SIG_A | WAN_RBS_SIG_B | WAN_RBS_SIG_C | WAN_RBS_SIG_D);
#endif
				}
				break;
			default:
				INFO_MAIN("Command invalid for card type\n");
				break;
			}//switch()
			break;
		case 'i':
			{
				INFO_MAIN("Type Idle Flag (HEX, for example: FE) and press <Enter>:\n");
				unsigned char new_idle_flag = (unsigned char)get_user_hex_number();
				sang_if->set_tx_idle_flag(new_idle_flag);
			}
			break;
		case 'c':
user_retry_ring_e_d:
			INFO_MAIN("Press 'e' to START ring, 'd' to STOP ring, 't' to Toggle\n");
			INFO_MAIN("\n");
			user_selection = tolower(my_getch());
			switch(user_selection)
			{
			case 'e':
				INFO_MAIN("Starting Ring ...%c\n",user_selection);
				sang_if->start_ringing_phone();//start
				break;
			case 'd':
				INFO_MAIN("Stopping Ring ... %c\n",user_selection);
				sang_if->stop_ringing_phone();//stop
				break;
			case 't':
				{
					int x;
					for (x=0;x<500;x++) {
						sang_if->start_ringing_phone();
						sang_if->start_ringing_phone();
						//sangoma_msleep(500);
						sang_if->stop_ringing_phone();//stop
						sang_if->stop_ringing_phone();//stop
						//sangoma_msleep(500);
						sang_if->start_busy_tone();
						sangoma_msleep(50);
						sang_if->stop_all_tones();
						sangoma_msleep(50);
					}
				}
				break;
			default:
				goto user_retry_ring_e_d;
				break;
			}
			break;
		case 'e':
			INFO_MAIN("Press 'e' to START a Tone, 'd' to STOP a Tone.\n");
			INFO_MAIN("\n");

			switch(tolower(my_getch()))
			{
			case 'e':
				INFO_MAIN("Press 'r' for Ring Tone, 'd' for Dial Tone, 'b' for Busy Tone, 'c' for Congestion Tone.\n");
				INFO_MAIN("\n");
				switch(tolower(my_getch()))
				{
				case 'r':
					sang_if->start_ring_tone();
					break;
				case 'd':
					sang_if->start_dial_tone();
					break;
				case 'b':
					sang_if->start_busy_tone();
					break;
				case 'c':
				default:
					sang_if->start_congestion_tone();
					break;
				}
				break;

			case 'd':
			default:
				sang_if->stop_all_tones();//stop all tones
			}
			break;
		case 'n':
			INFO_MAIN("Press 'e' to ENABLE Rx Hook Events, 'd' to DISABLE Rx Hook Events.\n");
			INFO_MAIN("\n");
			switch(tolower(my_getch()))
			{
			case 'e':
				sang_if->tdm_enable_rxhook_events();
				break;
			case 'd':
			default:
				sang_if->tdm_disable_rxhook_events();
			}
			break;
		case 'm':
			//Enable/Disable DTMF events on SLIC chip.
			//On Analog (A200) card only.
			INFO_MAIN("Press 'e' to ENABLE Remora DTMF Events, 'd' to DISABLE Remora DTMF Events.\n");
			INFO_MAIN("\n");
			switch(tolower(my_getch()))
			{
			case 'e':
				sang_if->tdm_enable_rm_dtmf_events();
				break;
			case 'd':
			default:
				sang_if->tdm_disable_rm_dtmf_events();
			}
			break;
		case 'o':
			{
				//Enable DTMF events on Octasic chip. 
				//For both Analog (A200) and T1/E1 (A104D) cards, but only if the chip is present.
				INFO_MAIN("Press 'e' to ENABLE Octasic DTMF Events, 'd' to DISABLE Octasic DTMF Events.\n");
				uint8_t channel;

				INFO_MAIN("\n");
				switch(tolower(my_getch()))
				{
				case 'e':
					INFO_MAIN("Type Channel number and press <Enter>:\n");
					channel = (uint8_t)get_user_decimal_number();//channels (Time Slots). Valid values: 1 to 31.

					sang_if->tdm_enable_dtmf_events(channel);
					break;
				case 'd':
				default:
					INFO_MAIN("Type Channel number and press <Enter>:\n");
					channel = (uint8_t)get_user_decimal_number();//channels (Time Slots). Valid values: 1 to 31.

					sang_if->tdm_disable_dtmf_events(channel);
				}
			}
			break;
		case 'u':
			//Enable/Disable Ring Detect events on FXO. 
			if(sang_if->get_sub_media()==MOD_TYPE_FXO){
				INFO_MAIN("Press 'e' to ENABLE Rx Ring Detect Events, 'd' to DISABLE Rx Ring Detect Events.\n");
				INFO_MAIN("\n");
				switch(tolower(my_getch()))
				{
				case 'e':
					sang_if->tdm_enable_ring_detect_events();
					break;
				case 'd':
				default:
					sang_if->tdm_disable_ring_detect_events();
				}
			}else if(sang_if->get_sub_media()==MOD_TYPE_FXS){
					sang_if->tdm_txsig_onhooktransfer();
			}
			break;
		case 'j':
			//Enable/Disable Ring Trip events on FXS. 
			INFO_MAIN("Press 'e' to ENABLE Rx Ring Trip Events, 'd' to DISABLE Rx Ring Trip Events.\n");
			INFO_MAIN("\n");
			switch(tolower(my_getch()))
			{
			case 'e':
				sang_if->tdm_enable_ring_trip_detect_events();
				break;
			case 'd':
			default:
				sang_if->tdm_disable_ring_trip_detect_events();
			}
			break;
		case 'h':
			if(sang_if->get_sub_media()==MOD_TYPE_FXO) {
				INFO_MAIN("Press 'e' to transmit OFF hook signal, 'd' to transmit ON hook signal.\n");
				INFO_MAIN("\n");
				switch(tolower(my_getch()))
				{
				case 'e':
					sang_if->fxo_go_off_hook();
					break;
				case 'd':
				default:
					sang_if->fxo_go_on_hook();
				}
			}else if(sang_if->get_sub_media() == MOD_TYPE_FXS) {
				INFO_MAIN("Press 'f' for forward, 'r' to for reverse.\n");
				INFO_MAIN("\n");
				switch(tolower(my_getch()))
				{
				case 'f':
					sang_if->tdm_set_rm_polarity(0);
					break;
				case 'r':
					sang_if->tdm_set_rm_polarity(1);
					break;
				default:
					//toggle polarity 
					int polarity_wait, two_second_ring_repetion_counter;

					INFO_MAIN("Type Polarity Reverse/Forward delay and press <Enter>:\n");
					polarity_wait = get_user_decimal_number();
					if (!polarity_wait) {
						ERR_MAIN("Invalid User-specified Polarity Reverse/Forward delay: %d!\n", polarity_wait);
						polarity_wait = 400;
					}
					/* polarity_wait of 30 ms will ring Analog phone or FXO
					 * polarity_wait of 400 ms will cause Polarity reversal event on FXO */
					INFO_MAIN("User specified Polarity Reverse/Forward delay: %d. Press <Enter> to continue.\n", polarity_wait);
					my_getch();

					/* calculate number of iterations for a 2-second test */
					two_second_ring_repetion_counter = 2000 / (polarity_wait * 2);
					INFO_MAIN("two_second_ring_repetion_counter: %d. Press <Enter> to continue.\n", two_second_ring_repetion_counter);
					my_getch();

					for (int ii = 0; ii < two_second_ring_repetion_counter; ii++) {
						INFO_MAIN("Reversing polarity %d...\n", ii);
						sang_if->tdm_set_rm_polarity(1);
						sangoma_msleep(polarity_wait);
						INFO_MAIN("Forwarding polarity %d...\n", ii);
						sang_if->tdm_set_rm_polarity(0);
						sangoma_msleep(polarity_wait);
					}

					//sleep 4 seconds between the rings
					INFO_MAIN("4 seconds between the rings...\n");
					sangoma_msleep(4000);

					for (int ii = 0; ii < two_second_ring_repetion_counter; ii++) {
						INFO_MAIN("Reversing polarity %d...\n", ii);
						sang_if->tdm_set_rm_polarity(1);
						sangoma_msleep(polarity_wait);
						INFO_MAIN("Forwarding polarity %d...\n", ii);
						sang_if->tdm_set_rm_polarity(0);
						sangoma_msleep(polarity_wait);
					}//for()
				}//switch()
			}
			break;
		case 'k':
			if( sang_if->get_adapter_type() == WAN_MEDIA_BRI ) {
				INFO_MAIN("Press 'e' to Activate, 'd' to De-Activate line.\n");
				INFO_MAIN("\n");
				switch(tolower(my_getch()))
				{
				case 'e':
					sang_if->tdm_front_end_activate();
					break;
				case 'd':
				default:
					sang_if->tdm_front_end_deactivate();
				}
			}else if(sang_if->get_adapter_type()== WAN_MEDIA_FXOFXS) {
				if(sang_if->get_sub_media()==MOD_TYPE_FXS) {
					printf("calling tdm_txsig_kewl()...\n");
					sang_if->tdm_txsig_kewl();
					sangoma_msleep(5000);
					//to restore line current after txsig kewl
					printf("restoring line current after txsig kewl...\n");
					sang_if->tdm_txsig_offhook();
				}
			}
			break;
		case 'p':
			{
				int user_period;//Milliseconds interval between receive of Voice Data
				INFO_MAIN("Type User Period and press <Enter>. Valid values are: 10, 20, 40.\n");
				user_period = get_user_decimal_number();
				switch(user_period)
				{
				case 10:
				case 20:
				case 40:
					sang_if->tdm_set_user_period(user_period);
					break;
				default:
					INFO_MAIN("Invalid User Period value! Valid values are: 10, 20, 40.\n");
					break;
				}
			}
			break;
#if USE_STELEPHONY_API
		case 'x':
			{	
				INFO_MAIN("Press a key. Valid values are 0-9, A-C\n");
				int user_char = my_getch();
 				switch(tolower(user_char)) {
					case '1': case '2': case '3':
					case '4': case '5': case '6':
					case '7': case '8': case '9':
					case '0': case 'a': case 'b':
					case 'c':
						INFO_MAIN("Sending DTMF (%c).\n", user_char);
						sang_if->sendSwDTMF((char)user_char);
						break;
					default:
						INFO_MAIN("Invalid DTMF Char! Valid values are: 0-9, A-C\n");
					break;
				}
			}
			break;
		case 'z':
			{
				if(WAN_MEDIA_FXOFXS == sang_if->get_adapter_type() &&  MOD_TYPE_FXS == sang_if->get_sub_media() ){
					//Ring the line
					sang_if->start_ringing_phone();
					sangoma_msleep(2000);
					//txsig offhook
					sang_if->fxs_txsig_offhook();
					INFO_MAIN("Sending CallerID.\n");
					sang_if->sendCallerID("Sangoma Rocks", "9054741990");
				}else{
					INFO_MAIN("Sending CallerID.\n");
					sang_if->sendCallerID("Sangoma Rocks", "9054741990");
				}				
			}
			break;
#endif
		case '1':/* read FE register */
			{
				int	value;
				sdla_fe_debug_t fe_debug;

				fe_debug.type = WAN_FE_DEBUG_REG;

				printf("Type Register number (hex) i.g. F8 and press Enter:");
				value = get_user_hex_number();

				fe_debug.fe_debug_reg.reg  = value;
				fe_debug.fe_debug_reg.read = 1;

				sang_if->set_fe_debug_mode(&fe_debug);
			}
			break;

		case '2':/* write FE register */
			{
				int	value;
				sdla_fe_debug_t fe_debug;
				fe_debug.type = WAN_FE_DEBUG_REG;

				printf("WRITE: Type Register number (hex) i.g. F8 and press Enter:");
				value = get_user_hex_number();

				fe_debug.fe_debug_reg.reg  = value;
				fe_debug.fe_debug_reg.read = 1;

				printf("WRITE: Type value (hex) i.g. 1A and press Enter:");
				value = get_user_hex_number();

				fe_debug.fe_debug_reg.read = 0;
				fe_debug.fe_debug_reg.value = (unsigned char)value;

				sang_if->set_fe_debug_mode(&fe_debug);
			}
			break;

		default:
			INFO_MAIN("Invalid command.\n");
		}		
	}while(user_selection != 'q');

	stop(sang_if);
	cleanup(sang_if);

	return 0;
}//main()

static int set_port_configuration()
{
	int		rc = 0, user_selection;
	/* On hybrid cards such as B700, ports can be of different types -
	 * some are Analog some Digital.
	 * On non-hybrid cards such as A108, all ports are of the same
	 * type - T1/E1. */
	int		is_te1_port = 0, is_analog_port = 0, is_bri_port = 0, is_serial_port = 0, is_gsm_port=0;
	hardware_info_t	hardware_info;
	port_cfg_t		port_cfg;

	sangoma_port_configurator *sng_port_cfg_obj;

	sng_port_cfg_obj = new sangoma_port_configurator();
	if(sng_port_cfg_obj == NULL || sng_port_cfg_obj->init((unsigned short)program_settings.wanpipe_number)){
		ERR_MAIN("Failed to initialize 'sangoma_port_configurator'\n");
		return 2;
	}

	rc = sng_port_cfg_obj->get_hardware_info(&hardware_info);

	if(rc == SANG_STATUS_SUCCESS){

		INFO_MAIN("card_model		: %s (%d)\n",
			SDLA_ADPTR_NAME(hardware_info.card_model), hardware_info.card_model);
		INFO_MAIN("firmware_version\t: 0x%02X\n", hardware_info.firmware_version);
		INFO_MAIN("pci_bus_number\t\t: %d\n", hardware_info.pci_bus_number);
		INFO_MAIN("pci_slot_number\t\t: %d\n", hardware_info.pci_slot_number);
		INFO_MAIN("max_hw_ec_chans\t\t: %d\n", hardware_info.max_hw_ec_chans);
		INFO_MAIN("port_number\t\t: %d\n", hardware_info.port_number);

	}else{
		ERR_MAIN("Failed to get hardware information\n");
		delete sng_port_cfg_obj;
		return 3;
	}

	// very important to zero out the configuration structure
	memset(&port_cfg, 0x00, sizeof(port_cfg_t));

	switch(hardware_info.card_model)
	{
	case A101_ADPTR_1TE1:
	case A101_ADPTR_2TE1:
	case A104_ADPTR_4TE1:
	case A108_ADPTR_8TE1:
		is_te1_port = 1;
		INFO_MAIN("T1/E1 Port on non-Hybrid Card (A10[1/2/4/8]).\n");
		break;
	case A200_ADPTR_ANALOG:
	case A400_ADPTR_ANALOG:
	case AFT_ADPTR_A600: //B600
		is_analog_port = 1;
		INFO_MAIN("Analog Port on non-Hybrid Card (A200/A400).\n");
		break;
	case AFT_ADPTR_ISDN:
		is_bri_port = 1;
		INFO_MAIN("BRI Port on non-Hybrid Card (A500).\n");
		break;
	case AFT_ADPTR_FLEXBRI:
		//B700, a hybrid card - may have both ISDN BRI and Analog ports
		if (hardware_info.bri_modtype == MOD_TYPE_NT ||
			hardware_info.bri_modtype == MOD_TYPE_TE) {
			is_bri_port = 1;
			INFO_MAIN("BRI Port on Hybrid Card.\n");
		} else {
			is_analog_port = 1;
			INFO_MAIN("Analog Port on Hybrid Card.\n");
		}
		break;
	case AFT_ADPTR_2SERIAL_V35X21:	/* AFT-A142 2 Port V.35/X.21 board */
	case AFT_ADPTR_4SERIAL_V35X21:	/* AFT-A144 4 Port V.35/X.21 board */
	case AFT_ADPTR_2SERIAL_RS232:	/* AFT-A142 2 Port RS232 board */
	case AFT_ADPTR_4SERIAL_RS232:	/* AFT-A144 4 Port RS232 board */
		is_serial_port = 1;
		INFO_MAIN("Serial Port on non-Hybrid Card (A14[2/4]).\n");
		break;
	case AFT_ADPTR_W400:
		is_gsm_port = 1;
		INFO_MAIN("GSM Port on GSM Card (W400).\n");
		break;
	default:
		INFO_MAIN("Warning: configuration of card model 0x%08X can not be changed!\n",
			hardware_info.card_model);
		break;
	}


	if (is_te1_port) {
		INFO_MAIN("\n");
		INFO_MAIN("Press 't' to set T1 configration.\n");
		INFO_MAIN("Press 'e' to set E1 configration.\n");
try_again:
		user_selection = tolower(my_getch());
		switch(user_selection)
		{
		case 't'://T1
			rc=sng_port_cfg_obj->initialize_t1_tdm_span_voice_api_configration_structure(&port_cfg,&hardware_info,program_settings.wanpipe_number);
			break;

		case 'e'://E1
			rc=sng_port_cfg_obj->initialize_e1_tdm_span_voice_api_configration_structure(&port_cfg,&hardware_info,program_settings.wanpipe_number);
			break;

		case 'q'://quit the application
			rc = 1;
			break;

		default:
			INFO_MAIN("Invalid command %c.\n",user_selection);
			goto try_again;

		}//switch(user_selection)
	}//if(is_te1_port)

	if (is_analog_port) {
		//read current configuration:
		if(sng_port_cfg_obj->get_configration(&port_cfg)){
			rc = 1;
		}else{
			//print the current configuration:
			sng_port_cfg_obj->print_port_cfg_structure(&port_cfg);
#if 0
			sng_port_cfg_obj->initialize_interface_mtu_mru(&port_cfg, 16, 16);
#endif
#if 0
			//as an EXAMPLE, enable Loop Current Monitoring for Analog FXO:
			rc=sng_port_cfg_obj->control_analog_rm_lcm(&port_cfg, 1);
#endif
#if 0
			//as an EXAMPLE, set Operation mode for FXO:
			rc=sng_port_cfg_obj->set_analog_opermode(&port_cfg, "TBR21");
#endif
		}
	}//if(is_analog_port)

	if (is_bri_port) {
		rc=sng_port_cfg_obj->initialize_bri_tdm_span_voice_api_configration_structure(&port_cfg,&hardware_info,program_settings.wanpipe_number);
	}

	if (is_serial_port) {
		rc=sng_port_cfg_obj->initialize_serial_api_configration_structure(&port_cfg,&hardware_info,program_settings.wanpipe_number);
	}

  if (is_gsm_port) {
    rc=sng_port_cfg_obj->initialize_gsm_tdm_span_voice_api_configration_structure(&port_cfg,&hardware_info,program_settings.wanpipe_number);
  }

	if(!is_te1_port && !is_analog_port && !is_bri_port && !is_serial_port && !is_gsm_port){
		INFO_MAIN("Unsupported Card %d\n", hardware_info.card_model);
		rc = 1;
	}

	do{
		if (rc) {
			ERR_MAIN("Failed to Initialize Port Configuratoin structure!\n");
			break;
		}
#if 1
		INFO_MAIN("Stopping PORT for re-configuration!\n");
		if ((rc = sng_port_cfg_obj->stop_port())) {
			ERR_MAIN("Failed to Stop Port! rc: %d\n", rc);
			break;
		}

		INFO_MAIN("Configuring PORT!\n");
		if ((rc = sng_port_cfg_obj->set_volatile_configration(&port_cfg))) {
			ERR_MAIN("Failed to Configure Port! rc: %d\n", rc);
			break;
		}

		INFO_MAIN("Starting PORT!\n");
		if ((rc = sng_port_cfg_obj->start_port())) {
			ERR_MAIN("Failed to Start Port! rc: %d\n", rc);
			break;
		}
#endif
#if 0
		//Optional step: the volatile configuration can be stored on the hard drive.
		if (rc = sng_port_cfg_obj->write_configration_on_persistent_storage(
			&port_cfg, &hardware_info, program_settings.wanpipe_number)) {
			ERR_MAIN("Failed to write configuration on persistant storage! rc: %d\n", rc);
			break;
		}
#endif
	}while(0);

	if(sng_port_cfg_obj != NULL){
		delete sng_port_cfg_obj;
	}

	sangoma_msleep(2000);//wait 2 seconds for initialization to complete

	return rc;
}


#if USE_STELEPHONY_API
static void FSKCallerIDEvent(void *callback_context,
							 char * Name, char * CallerNumber,
							 char * CalledNumber, char * DateTime)
{
	//The "sangoma_interface" object was registered as the callback context in StelSetup() call.
	sangoma_interface	*sang_if = (sangoma_interface*)callback_context;

	INFO_MAIN("\n%s: %s() - Start\n", sang_if->device_name, __FUNCTION__);

	if(Name){
		INFO_MAIN("Name: %s\n", Name);
#if 0
		printf("caller name in SINGLE byte hex:\n");
		for(unsigned int ind = 0; ind < strlen(Name); ind++){
			printf("Name[%02d]: 0x%02X\n", ind, Name[ind]);
		}
		printf("\n");

		printf("caller name in DOUBLE byte (unicode) hex:\n");
		for(unsigned int ind = 0; ind < strlen(Name); ind += 2){
			printf("Name[%02d]: 0x%04X\n", ind, *(unsigned short*)&Name[ind]);
		}
#endif
		printf("\n");
	}


	if(CallerNumber){
		INFO_MAIN("CallerNumber: %s\n", CallerNumber);
	}
	if(CalledNumber){
		INFO_MAIN("CalledNumber: %s\n", CalledNumber);
	}
	if(DateTime){
		INFO_MAIN("DateTime: %s\n", DateTime);
	}

	INFO_MAIN("Resetting FSK Caller ID\n");
	sang_if->resetFSKCID();//prepare for next FSK CID detection

	INFO_MAIN("%s() - End\n\n", __FUNCTION__);
}

static void DTMFEvent(void *callback_context, long Key)
{
	//The "sangoma_interface" object was registered as the callback context in StelSetup() call.
	sangoma_interface	*sang_if = (sangoma_interface*)callback_context;

	INFO_MAIN("\n%s: %s() - Start\n", sang_if->device_name, __FUNCTION__);

	INFO_MAIN("Key: %c\n", (char) Key);

	INFO_MAIN("%s() - End\n\n", __FUNCTION__);
}

static void Q931Event(void *callback_context, stelephony_q931_event *pQ931Event)
{
	//The "sangoma_interface" object was registered as the callback context in StelSetup() call.
	sangoma_interface	*sang_if = (sangoma_interface*)callback_context;
	
	INFO_MAIN("\n%s: %s() - Start\n", sang_if->device_name, __FUNCTION__);
#if 0
	INFO_MAIN("\nFound %d bytes of data: ", pQ931Event->dataLength);
	for (int i=0; i < pQ931Event->dataLength;i++){
		INFO_MAIN("%02X ",pQ931Event->data[i]);
	}
	INFO_MAIN("\n");
#endif


	INFO_MAIN("Message Received on: %02d/%02d/%02d @ %02d:%02d:%02d\n",pQ931Event->tv.wMonth,pQ931Event->tv.wDay,pQ931Event->tv.wYear,
		pQ931Event->tv.wHour,pQ931Event->tv.wMinute,pQ931Event->tv.wSecond);
	
	INFO_MAIN("Message Type is: %s\n",pQ931Event->msg_type);
	INFO_MAIN("Length of Call Reference Field is: %d\n", pQ931Event->len_callRef);
	INFO_MAIN("Message Call Reference is : 0X%s\n",pQ931Event->callRef);

	if (pQ931Event->cause_code > 0){
		INFO_MAIN("Cause code found = %d \n", pQ931Event->cause_code);
	}

	if (pQ931Event->chan > 0){
		INFO_MAIN("B-channel used = %d \n", pQ931Event->chan);
	}

	if (pQ931Event->calling_num_digits_count > 0 ){
		INFO_MAIN("Found %d digits for calling number \n", pQ931Event->calling_num_digits_count);
		INFO_MAIN("Presentation indicator is = %d \n",pQ931Event->calling_num_presentation);
		INFO_MAIN("Screening indicator is = %d \n",pQ931Event->calling_num_screening_ind);
		INFO_MAIN("Calling number is = %s\n",pQ931Event->calling_num_digits);
	}

	if (pQ931Event->called_num_digits_count > 0 ){
		INFO_MAIN("Found %d digits for called number \n", pQ931Event->called_num_digits_count);
		INFO_MAIN("Called number is = %s\n",pQ931Event->called_num_digits);
	}

	if (pQ931Event->rdnis_digits_count > 0 ){
		INFO_MAIN("Found %d digits for RDNIS\n", pQ931Event->rdnis_digits_count);
		INFO_MAIN("RDNIS is = %s\n",pQ931Event->rdnis_string);
	}
	//INFO_MAIN("%s() - End\n\n", __FUNCTION__);
}

/*	A buffer containing DTMF digit was initialized. In this callback transmit the buffer
	by starting the SwDtmfTxThread */
static void SwDtmfTransmit (void *callback_context, void *DtmfBuffer)
{	
	sangoma_interface	*sang_if = (sangoma_interface*)callback_context;
	DBG_MAIN("%s(): %s:\n", __FUNCTION__, sang_if->device_name);

	/*	DTMF buffer can be very big (long DTMF is not uncommon), we don't want to
		block the calling thread, so start a new thread to transmit SW-DTMF. */
	sang_if->CreateSwDtmfTxThread(DtmfBuffer);
}

/* A buffer containing FSK CID was initialized. In this callback transmit the buffer. */
static void FSKCallerIDTransmit (void *callback_context, void *FskCidBuffer)
{
	sangoma_interface	*sang_if = (sangoma_interface*)callback_context;
	DBG_MAIN("%s(): %s:\n", __FUNCTION__, sang_if->device_name);

	/*	FSK CID buffer can be big (~8000 bytes), we don't want to block the calling thread,
		so start a new thread to transmit FSK CID. */
	sang_if->CreateFskCidTxThread(FskCidBuffer);
}

#if 0
#warning "REMOVE LATER"
int slin2ulaw(void* data, size_t max, size_t *datalen)
{
	int16_t sln_buf[512] = {0}, *sln = sln_buf;
	uint8_t *lp = (uint8_t*)data;
	uint32_t i;
	size_t len = *datalen;

	if (max > len) {
		max = len;
	}

	memcpy(sln, data, max);
	
	for(i = 0; i < max; i++) {
		*lp++ = linear_to_ulaw(*sln++);
	}

	*datalen = max / 2;

	return 0;
}
#endif

#endif /* USE_STELEPHONY_API */

#if 0
LONG Win32FaultHandler(struct _EXCEPTION_POINTERS *  ExInfo)

{   
	char  *FaultTx = "";
    switch(ExInfo->ExceptionRecord->ExceptionCode)
    {
	case EXCEPTION_ACCESS_VIOLATION: 
		FaultTx = "ACCESS VIOLATION";
		break;
	case EXCEPTION_DATATYPE_MISALIGNMENT: 
		FaultTx = "DATATYPE MISALIGNMENT";
		break;
	case EXCEPTION_FLT_DIVIDE_BY_ZERO: 
		FaultTx = "FLT DIVIDE BY ZERO";
		break;
	default: FaultTx = "(unknown)";           
		break;
    }

    FILE *sgLogFile = fopen("Win32Fault.log", "w");
    int    wsFault    = ExInfo->ExceptionRecord->ExceptionCode;
    PVOID  CodeAddress = ExInfo->ExceptionRecord->ExceptionAddress;

    sgLogFile = fopen("Win32Fault.log", "w");
	if(sgLogFile != NULL)
	{
		fprintf(sgLogFile, "****************************************************\n");
		fprintf(sgLogFile, "*** A Program Fault occurred:\n");
		fprintf(sgLogFile, "*** Error code %08X: %s\n", wsFault, FaultTx);
		fprintf(sgLogFile, "****************************************************\n");
		fprintf(sgLogFile, "***   Address: %08X\n", (int)CodeAdress);
		fprintf(sgLogFile, "***     Flags: %08X\n", 
			ExInfo->ExceptionRecord->ExceptionFlags);
		LogStackFrames(CodeAddress, (char *)ExInfo->ContextRecord->Ebp);
		fclose(sgLogFile);
	}
	/*if(want to continue)
	{
		ExInfo->ContextRecord->Eip++;
		return EXCEPTION_CONTINUE_EXECUTION;
	}
	*/ 
	return EXCEPTION_EXECUTE_HANDLER;
}
#endif
