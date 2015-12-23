/*
	sample.h - common definitions used in this application.
*/
#ifndef _SAMPLE_H
#define _SAMPLE_H

#include <libsangoma.h>
#if USE_STELEPHONY_API
#include <libstelephony.h>
#endif

#define USE_WP_LOGGER 1
#define SAMPLE_CPP_MAX_PATH 1024

#define USE_BUFFER_MULTIPLIER 0


typedef struct{
	unsigned int	wanpipe_number;	
	unsigned int	interface_number;	
	unsigned char	silent;
	uint16_t		txlength;
	unsigned char	Rx_to_Tx_loopback;
	unsigned char	decode_fsk_cid;
	unsigned char 	encode_fsk_cid;
	unsigned char	sw_dtmf;
	unsigned char 	encode_sw_dtmf;
	unsigned char	voice_codec_alaw;
	unsigned char	decode_q931;
	char			szTxFileName[SAMPLE_CPP_MAX_PATH];
	unsigned int	txcount;
	unsigned char   driver_config;
	unsigned char	use_ctrl_dev;
	unsigned char	use_logger_dev;
	int				txgain;
	int				rxgain;
	unsigned char	use_hardware_echo_canceller;
	unsigned char	real_time;
	unsigned char	hwec_channel;
#if USE_BUFFER_MULTIPLIER
	unsigned int	buffer_multiplier;
#endif
}wp_program_settings_t;

#define DEV_NAME_LEN			100

extern void cli_out(unsigned int dbg_flag, void *pszFormat, ...);
extern unsigned int verbosity_level;

typedef struct{
	//Recieved data
	int		(*got_rx_data)(void *sang_if_ptr, void *rxhdr, void *rx_data);
	//TDM events
	void	(*got_tdm_api_event)(void *sang_if_ptr, void *event_data);
#if USE_STELEPHONY_API
	//FSK Caller ID detected
	void	(*FSKCallerIDEvent)(void *sang_if_ptr, char * Name, char * CallerNumber, char * CalledNumber, char * DateTime);
	//DTMF detected in SOFTWARE
	void	(*DTMFEvent)(void *sang_if_ptr, long Key);
	//Q931 decoder events
	void	(*Q931Event)(void *callback_context, stelephony_q931_event *pQ931Event);
	//FSK Caller ID buffer ready for transmission events
	void	(*FSKCallerIDTransmit)(void *callback_context, void* buffer);
	//DTMF buffer ready for transmission events
	void	(*SwDtmfTransmit)(void *callback_context, void* buffer);
#endif

#if USE_WP_LOGGER
	void	(*got_logger_event)(void *sang_if_ptr, wp_logger_event_t *logger_event);
#endif

}callback_functions_t;

#if USE_BUFFER_MULTIPLIER
# define SAMPLE_CPP_MAX_DATA_LEN	(MAX_BUFFER_MULTIPLIER_FACTOR*MAX_NO_DATA_BYTES_IN_FRAME)
#else
# define SAMPLE_CPP_MAX_DATA_LEN	MAX_NO_DATA_BYTES_IN_FRAME
#endif

static void wp_print_rbs_cas_bits(unsigned int abcd)
{
	printf("A:%1d B:%1d C:%1d D:%1d\n",
		(abcd & WAN_RBS_SIG_A) ? 1 : 0,
		(abcd & WAN_RBS_SIG_B) ? 1 : 0,
		(abcd & WAN_RBS_SIG_C) ? 1 : 0,
		(abcd & WAN_RBS_SIG_D) ? 1 : 0);
}

#if defined (__WINDOWS__)
static void DecodeLastError(LPSTR lpszFunction) 
{
	LPVOID lpMsgBuf;
	DWORD dwLastErr = GetLastError();
	FormatMessage( 
		FORMAT_MESSAGE_ALLOCATE_BUFFER | 
		FORMAT_MESSAGE_FROM_SYSTEM | 
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		dwLastErr,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
		(LPTSTR) &lpMsgBuf,
		0,
		NULL 
	);
	// Display the string.
	printf("Last Error: %s (GetLastError() returned: %d)\n", (char*)lpMsgBuf, dwLastErr);
	// Free the buffer.
	LocalFree( lpMsgBuf );
} 
#endif

//This Program interfaces directly to hardware, it should be real time.
static void sng_set_process_priority_to_real_time()
{
#if defined (__WINDOWS__)
	if (!SetPriorityClass(GetCurrentProcess(), REALTIME_PRIORITY_CLASS)) {
		printf("Failed to set program priority!\n");
	}
#else
	//TODO: implement for Linux
#endif
}

//This Execution Thread interfaces directly to hardware, it should be real time.
static void sng_set_thread_priority_to_real_time()
{
#if defined (__WINDOWS__)
	if (!SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL)) {
		printf("Failed to set thread priority!\n");
	}
#else
	//TODO: implement for Linux
#endif
}

/* This flag controls debugging of time difference between 
 * data receive indications. Disabled by default. */
#define DBG_TIMING 0

#if DBG_TIMING

#define DEBUG_HI_RES_PERF if(0)printf

typedef struct _wan_debug{

	LARGE_INTEGER	LastHiResolutionCounter;
	int				high_resolution_timediff_value;
	unsigned int	allowed_deviation_of_timediff_value;
	unsigned int	timediff_deviation_counter;
	int				highest_timediff;
	int				lowest_timediff;
	int				latest_timediff;
	char			*debug_name;//optional
}wan_debug_t;


/*	Print contents of wan_debug_t structure:
	1. TimeDiff info
	2. More info will be added in future
*/
static
void
debug_print_dbg_struct(
	wan_debug_t	*wan_debug_ptr,
	const char *caller_name
	)
{
	printf("timediff_deviation_counter: %d\n\tTimeDiff (ms) between '%s()':\n\tHighest:%d  Lowest:%d  Expected: %d  Latest: %d allowed deviation:%d\n", 
		wan_debug_ptr->timediff_deviation_counter,
		caller_name,
		wan_debug_ptr->highest_timediff,
		wan_debug_ptr->lowest_timediff,
		wan_debug_ptr->high_resolution_timediff_value,
		wan_debug_ptr->latest_timediff,
		wan_debug_ptr->allowed_deviation_of_timediff_value);
}

/* Call this function to mesure TimeDiff between some function call. */
static
void
debug_update_timediff(
	wan_debug_t	*wan_debug_ptr,
	const char *caller_name
	)
{
	LARGE_INTEGER	CurrentHiResolutionCounter;
	LARGE_INTEGER	PerformanceFrequency;
	unsigned int	number_of_HiResCounterTicks_BETWEEN_function_calls;
	int				timediff_between_function_calls;

	if(!wan_debug_ptr->high_resolution_timediff_value){
		//TimeDiff debugging is disabled.
		return;
	}

	QueryPerformanceFrequency(&PerformanceFrequency);
	QueryPerformanceCounter(&CurrentHiResolutionCounter);

	if(	wan_debug_ptr->LastHiResolutionCounter.QuadPart == 0 ){
		//It is the 1st time this function is running after TimeDiff debugging was enabled.
		//Initialize wan_debug_ptr->LastHiResolutionCounter!
		QueryPerformanceCounter(&wan_debug_ptr->LastHiResolutionCounter);
		return;
	}

#if 0
	DEBUG_HI_RES_PERF("Frequency: %I64u\n", PerformanceFrequency);
	DEBUG_HI_RES_PERF("hi res cnt difference: %I64u\n", 
		CurrentHiResolutionCounter.QuadPart - wan_debug_ptr->LastHiResolutionCounter.QuadPart);
#endif
	////////////////////////////////////////////////////////////////
	//Frequency: 1600000000
	//hi res cnt difference: 32038864
	//1600000000 / 32038864 = 49.939348661051153374227001306913
	////////////////////////////////////////////////////////////////
	if((CurrentHiResolutionCounter.QuadPart - wan_debug_ptr->LastHiResolutionCounter.QuadPart)){//check for division by zero
		number_of_HiResCounterTicks_BETWEEN_function_calls = (unsigned int)
			(PerformanceFrequency.QuadPart / (CurrentHiResolutionCounter.QuadPart - wan_debug_ptr->LastHiResolutionCounter.QuadPart));
	}
	//note that floating point calculations are not allowed in kernel!
	if(number_of_HiResCounterTicks_BETWEEN_function_calls){//check for division by zero
		timediff_between_function_calls = 1000 / number_of_HiResCounterTicks_BETWEEN_function_calls;
	}
#if 0
	if(number_of_HiResCounterTicks_BETWEEN_function_calls < 49 || number_of_HiResCounterTicks_BETWEEN_function_calls > 51){
		printf("Warning: Invalid Number of SetEvent() calls per second %d!\n", number_of_HiResCounterTicks_BETWEEN_function_calls);
	}
#endif
	//if(timediff_between_function_calls < 19 || timediff_between_function_calls > 21){
	if(	timediff_between_function_calls < 
		(int)(wan_debug_ptr->high_resolution_timediff_value - wan_debug_ptr->allowed_deviation_of_timediff_value) ||
		timediff_between_function_calls >
		(int)(wan_debug_ptr->high_resolution_timediff_value + wan_debug_ptr->allowed_deviation_of_timediff_value)){

			wan_debug_ptr->timediff_deviation_counter++;

			//			DEBUG_HI_RES_PERF("diff: %d, hi: %d, low: %d\n", timediff_between_function_calls, 
			//				wan_debug_ptr->highest_timediff, wan_debug_ptr->lowest_timediff);

			if(timediff_between_function_calls > 0){//because the FIRST timediff is always negative!

				int current_time_diff_delta = 
					timediff_between_function_calls - wan_debug_ptr->high_resolution_timediff_value;

				//negative (example: 10 - 20 = -10)
				int lowest_time_diff_delta = 
					wan_debug_ptr->lowest_timediff - wan_debug_ptr->high_resolution_timediff_value;

				//positive (example: 30 - 20 = +10)
				int highest_time_diff_delta = 
					wan_debug_ptr->highest_timediff - wan_debug_ptr->high_resolution_timediff_value;

				DEBUG_HI_RES_PERF("curr delta: %d, hi delta: %d, low delta: %d\n",
					current_time_diff_delta, highest_time_diff_delta, lowest_time_diff_delta);

				if(current_time_diff_delta < 0){
					//TimeDiff is BELLOW expected
					if(	current_time_diff_delta < lowest_time_diff_delta ||
						wan_debug_ptr->lowest_timediff == 0){
							wan_debug_ptr->lowest_timediff = timediff_between_function_calls;
					}
				}

				if(current_time_diff_delta > 0){
					//TimeDiff is ABOVE expected
					if(current_time_diff_delta > highest_time_diff_delta ||
						wan_debug_ptr->highest_timediff == 0){
							wan_debug_ptr->highest_timediff = timediff_between_function_calls;
					}
				}
			}//if(timediff_between_function_calls > 0)

			if(!(wan_debug_ptr->timediff_deviation_counter % 10)){
				debug_print_dbg_struct(wan_debug_ptr, caller_name);
			}
	}//if(	timediff_between_function_calls ...)

	wan_debug_ptr->latest_timediff = timediff_between_function_calls;

	//prepare for the NEXT call by storing CURRENT counter
	QueryPerformanceCounter(&wan_debug_ptr->LastHiResolutionCounter);
}

/* Set (initialize) the TimeDiff part of wan_debug_t structure. */
static
void
debug_set_timing_info(
	wan_debug_t	*wan_debug_ptr,
	int usr_period,
	int allowed_deviation_of_timediff_value
	)
{
	wan_debug_ptr->high_resolution_timediff_value = usr_period;//in milliseconds. 0 - disable TimeDiff debugging.

	if(wan_debug_ptr->high_resolution_timediff_value == 0){
		wan_debug_ptr->LastHiResolutionCounter.QuadPart = 0;
	}

	wan_debug_ptr->highest_timediff = wan_debug_ptr->lowest_timediff = 0;

	wan_debug_ptr->allowed_deviation_of_timediff_value = allowed_deviation_of_timediff_value;//milliseconds. 
																							//for example: if 2 and high_resolution_timediff_value is 20,
																							//will result 18 < 20 < 22
}

/////////////////////////////////////////////////////////////////////////////////////
#endif//DBG_TIMING

#endif//_SAMPLE_H
