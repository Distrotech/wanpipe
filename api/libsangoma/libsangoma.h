/*******************************************************************************//**
 * \file libsangoma.h
 * \brief Wanpipe API Library header for Sangoma AFT T1/E1/Analog/BRI/Serial Hardware -
 * \brief Provides User a Unified/OS Agnostic API to Wanpipe/Sangoma Drivers and Hardware
 *
 * Author(s):   Nenad Corbic <ncorbic@sangoma.com>
 *              David Rokhvarg <davidr@sangoma.com>
 *              Michael Jerris <mike@jerris.com>
 *              Anthony Minessale II <anthmct@yahoo.com>
 *
 * Copyright:	(c) 2005-2008 Nenad Corbic <ncorbic@sangoma.com>
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
 * v.2.0.0 Nenad Corbic
 * Jan 30 2009
 *		Added sangoma_get_driver_version, sangoma_get_firmware_version,
 *      sangoma_get_cpld_version functions,sangoma_get_stats,sangoma_flush_stats
 */
#ifndef _LIBSANGOMA_H
#define _LIBSANGOMA_H

#ifdef __linux__
#ifndef __LINUX__
/* most wanpipe driver headers require __LINUX__ to be defined */
#define __LINUX__
#endif
#endif

#ifdef __cplusplus
extern "C" {	/* for C++ users */
#endif

#include <stdio.h>


/*!
  \def WANPIPE_TDM_API
  \brief Used by compiler and driver to enable TDM API
*/
#define WANPIPE_TDM_API 1

/*TODO: LIBSANGOMA_VERSION_CODE should be generated out of LIBSANGOMA_LT_CURRENT and friends in configure.in */

/*!
  \def LIBSANGOMA_VERSION
  \brief LibSangoma Macro to check the Version Number
*/
#define LIBSANGOMA_VERSION(a,b,c) (((a) << 16) + ((b) << 8) + (c))

/*!
  \def LIBSANGOMA_VERSION_CODE
  \brief LibSangoma Current Version Number to be checked against the LIBSANGOMA_VERSION Macro 
*/
#define LIBSANGOMA_VERSION_CODE LIBSANGOMA_VERSION(3,3,0)

/*!
  \def LIBSANGOMA_VERSION_STR
  \brief LibSangoma Version in string format
*/
#define LIBSANGOMA_VERSION_STR "3.3.0"

struct sangoma_wait_obj;
#define sangoma_wait_obj_t struct sangoma_wait_obj


/*!
   \def	SANGOMA_DECLARE_TDM_API_CMD
   \brief Instantiate/Declare a tdm api cmd strucure
   \def SANGOMA_INIT_TDM_API_CMD
   \brief Initialize the tdm api cmd structure. Set to 0.
   \def SANGOMA_INIT_TDM_API_CMD_RESULT
   \brief Initialize the 'result' in wanpipe_api_t to SANG_STATUS_GENERAL_ERROR.
   \def SANGOMA_DECLARE_INIT_TDM_API_CMD
   \brief Declare and initialize the tdm api cmd structure.
*/
#define SANGOMA_DECLARE_TDM_API_CMD(_name_)  		wanpipe_api_t _name_

#define SANGOMA_INIT_TDM_API_CMD(_name_) 			memset(&_name_,0,sizeof(_name_)); \
														SANGOMA_INIT_TDM_API_CMD_RESULT(_name_)

#define SANGOMA_INIT_TDM_API_CMD_RESULT(_name_)		(_name_).wp_cmd.result = SANG_STATUS_GENERAL_ERROR 

#define SANGOMA_DECLARE_INIT_TDM_API_CMD(_name_)  	wanpipe_tdm_api_t _name_; SANGOMA_INIT_TDM_API_CMD(_name_);


#if defined(WIN32) || defined(WIN64)
#ifndef __WINDOWS__
#define __WINDOWS__
#endif
#include <windows.h>
#include <winioctl.h>
#include <conio.h>
#include <stddef.h>	
#include <stdlib.h>	
#include <time.h>

/*!
  \def _LIBSNG_CALL
  \brief libsangoma.dll functions exported as __cdecl calling convention
*/
#ifdef __COMPILING_LIBSANGOMA__
# define _LIBSNG_CALL __declspec(dllexport) __cdecl
#else
# ifdef __LIBSANGOMA_IS_STATIC_LIB__
/* libsangoma is static lib OR 'compiled in' */
#  define _LIBSNG_CALL __cdecl
# else
/* compiling an application which will use libsangoma.dll */
#  define _LIBSNG_CALL __declspec(dllimport) __cdecl
# endif
#endif /* __COMPILING_LIBSANGOMA__ */

/*!
  \def SANGOMA_INFINITE_API_POLL_WAIT (deprecated, use SANGOMA_WAIT_INFINITE instead)
  \brief Infinite poll timeout value 
*/
#define SANGOMA_INFINITE_API_POLL_WAIT INFINITE
#define SANGOMA_WAIT_INFINITE INFINITE

/*!
  \def sangoma_msleep(x)
  \brief milisecond sleep function
*/
#define sangoma_msleep(x)	Sleep(x)

/*!
  \def sangoma_ctime(time_val)
  \brief Convert a time value to a string
  \param time_val pointer to 64-bit time value
*/
#define sangoma_ctime(time)	_ctime64(time)

#else
/* L I N U X */
#include <stdlib.h>
#include <stdarg.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/signal.h>
#include <sys/select.h>
#include <sys/wait.h>
#include <sys/resource.h>
#include <netinet/in.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <net/if.h>
#include <poll.h>
#include <signal.h>
#include <pthread.h>
#include <stdint.h>

/*!
  \def _LIBSNG_CALL
  \brief Not used in Linux
*/
#define _LIBSNG_CALL

/*!
  \def INVALID_HANDLE_VALUE
  \brief Invalid file handle value -1, Ported from Windows
*/
#define INVALID_HANDLE_VALUE -1

/*!
  \def SANGOMA_INFINITE_API_POLL_WAIT (deprecated, use SANGOMA_WAIT_INFINITE instead)
  \brief Infinite poll timeout value -1, Ported from Windows
*/
#define SANGOMA_INFINITE_API_POLL_WAIT -1
#define SANGOMA_WAIT_INFINITE -1

/*!
  \def __cdecl
  \brief Ported from Windows
  \typedef BOOL
  \brief Boolean type int, Ported from Windows
  \typedef DWORD
  \brief DWORD type is int, Ported from Windows
  \def FALSE
  \brief FALSE value is 0, Ported from Windows
  \def TRUE
  \brief TRUE value is 1, Ported from Windows
  \def sangoma_msleep(x)
  \brief milisecond sleep function
  \def _getch
  \brief get character, Ported from Windows
  \def Sleep
  \brief milisecond sleep function
  \typedef HANDLE
  \brief file handle type int, Ported from Windows
  \typedef TCHAR
  \brief TCHAN type mapped to char, Ported from Windows
  \typedef ULONG
  \brief ULONG type mapped to unsigned long, Ported from Windows
  \typedef UCHAR
  \brief ULONG type mapped to unsigned char, Ported from Windows
  \typedef USHORT
  \brief USHORT type mapped to unsigned short, Ported from Windows
  \typedef LPSTR
  \brief LPSTR type mapped to unsigned char *, Ported from Windows
  \typedef PUCHAR
  \brief PUCHAR type mapped to unsigned char *, Ported from Windows
  \typedef PVOID
  \brief PVOID type mapped to void *, Ported from Windows
  \typedef LPTHREAD_START_ROUTINE
  \brief LPTHREAD_START_ROUTINE type mapped to unsigned char *, Ported from Windows
  \def _stricmp
  \brief _stricmp type mapped to _stricmp, Ported from Windows
  \def _snprintf
  \brief _snprintf type mapped to snprintf, Ported from Windows
*/

#define __cdecl

#ifndef FALSE
#define FALSE		0
#endif

#ifndef TRUE
#define TRUE		1
#endif

#define sangoma_msleep(x) usleep(x*1000)
#define _getch		getchar
#define Sleep 		sangoma_msleep
#define _stricmp   	strcmp
#define _snprintf	snprintf
#define _vsnprintf  	vsnprintf

typedef int HANDLE;
typedef int DWORD;
typedef char TCHAR;


#if 0
typedef int BOOL;
typedef unsigned char UCHAR;
typedef unsigned long ULONG;
typedef unsigned short USHORT;
typedef unsigned char * LPSTR;
typedef unsigned char * PUCHAR;
typedef void * PVOID;
typedef unsigned int UINT;
#endif

typedef void * LPTHREAD_START_ROUTINE;
typedef pthread_mutex_t CRITICAL_SECTION;

#define EnterCriticalSection(arg) 	pthread_mutex_lock(arg)
/* On success, pthread_mutex_trylock() returns 0. On error, non-zero value is returned. */
#define TryEnterCriticalSection(arg)	(pthread_mutex_trylock(arg)==0 ? 1 : 0)
#define LeaveCriticalSection(arg) 	pthread_mutex_unlock(arg)
#define InitializeCriticalSection(arg)	pthread_mutex_init(arg, NULL);

typedef struct tm SYSTEMTIME;
typedef char * LPCTSTR;

/*!
  \def sangoma_ctime(time_val)
  \brief Convert a time value to a string
  \param time_val pointer to time value
*/
#define sangoma_ctime(time)	ctime((time_t*)time)

#endif/* WIN32 */


/*!
  LIBSANGOMA_LIGHT can be used to enable only IO and EVENT
  libsangoma functions.  The DRIVER configuration/start/stop
  functions are not compiled.

  LIBSANGOMA_LIGHT depends only on 3 header files. Instead
  of all wanpipe header files needed for DRIVER management

  LIBSANGMOA_LIGHT is NOT enabled by default.
*/

#ifdef LIBSANGOMA_LIGHT
#include "wanpipe_api_iface.h"
#include "wanpipe_api_hdr.h"
#include "sdla_te1.h"
#include "wanpipe_events.h"
#include "wanpipe_api_deprecated.h"
#else
#include "wanpipe_api.h"

#ifdef WP_API_FEATURE_LIBSNG_HWEC
# include "wanpipe_events.h"
# include "wanec_api.h"
# include "wanec_iface_api.h"
#endif
#endif


#ifdef __LINUX__
#include "wanpipe_kernel.h"
#endif

/*!
 * As of now this typedef maps exactly to SANG_STATUS_T, however that
 * is a kernel type, ugly, ugly, uglyyyyy, we should have strictly
 * minimum set of shared data structures between kernel and user
 * many return codes specified in SANG_STATUS_T are kernel specific
 * like FAILED_TO_LOCK_USER_MEMORY or INVALID_IRQL, the libsangoma
 * user does not need that much information, and even if ever needs
 * it we should provide simpler defaults
 * \brief return status from sangoma APIs
 */
typedef int32_t sangoma_status_t;

/*!
  \def FNAME_LEN
  \brief string length of a file name
  \def FUNC_DBG(x)
  \brief function debug print function
  \def DBG_PRINT
  \brief debug print function
*/
#define FNAME_LEN		100
#define LIBSNG_FUNC_DBG()	if(0)printf("%s(): line:%d\n", __FUNCTION__, __LINE__)
#define LIBSNG_DBG_PRINT	if(0)printf

/*!
  \typedef sangoma_api_hdr_t
  \brief Backward comaptible define of wp_api_hdr_t
*/
typedef wp_api_hdr_t sangoma_api_hdr_t;

/*!
  \enum _sangoma_wait_obj_type_t
  \brief Wait object type definition
  \typedef sangoma_wait_obj_type_t
  \brief Wait object type definition
*/
typedef enum _sangoma_wait_obj_type
{
	/*! \brief deprecated, use SANGOMA_GENERIC_WAIT_OBJ */
	UNKNOWN_WAIT_OBJ = 0, 
	/*! \brief Generic object that can be signaled but is not associated to any sangoma device */
	SANGOMA_GENERIC_WAIT_OBJ = 0, 
	/*!  \brief Sangoma object associated to some device which cannot be signaled (cannot call sangoma_wait_obj_signal on it) */
	SANGOMA_DEVICE_WAIT_OBJ, 
	/*!  \brief Sangoma object that is associated to a device AND can be signaled */
	SANGOMA_DEVICE_WAIT_OBJ_SIG, 
} sangoma_wait_obj_type_t;

#define DECODE_SANGOMA_WAIT_OBJECT_TYPE(type)\
	type == SANGOMA_GENERIC_WAIT_OBJ	? "SANGOMA_GENERIC_WAIT_OBJ"	:\
	type == SANGOMA_DEVICE_WAIT_OBJ		? "SANGOMA_DEVICE_WAIT_OBJ"		:\
	type == SANGOMA_DEVICE_WAIT_OBJ_SIG ? "SANGOMA_DEVICE_WAIT_OBJ_SIG"	:\
	"Invalid Wait Object type!"

/* 
 * Possible flags for sangoma waitable objects 
 * this definitions MUST match POLLIN, POLLOUT and POLLPRI 
 * SANG_WAIT_OBJ_IS_SIGNALED has no posix equivalent though
 * Users are encouraged to use this flags instead of the system ones
 */
typedef enum _sangoma_wait_obj_flags {
  SANG_WAIT_OBJ_HAS_INPUT = WP_POLLIN,
  SANG_WAIT_OBJ_HAS_OUTPUT = WP_POLLOUT,
  SANG_WAIT_OBJ_HAS_EVENTS = WP_POLLPRI,
  SANG_WAIT_OBJ_IS_SIGNALED = 0x400 /* matches GNU extension POLLMSG */
} sangoma_wait_obj_flags_t;

/*
 * Note for data format of Input/Output and Bit-order in libsangoma API.
 * The bit-order depends on Span/Channel configuration.
 *
 * TDM_CHAN_VOICE_API - expected input is ulaw/alaw, output is ulaw/alaw,
 * TDM_SPAN_VOICE_API - expected input is ulaw/alaw, output is ulaw/alaw
 *
 * DATA_API - data can be in any format, not bitswapped by AFT hardware
 * API - data can be in any format, not bitswapped by AFT hardware
*/

/************************************************************//**
 * Device OPEN / CLOSE Functions
 ***************************************************************/

/*!
  \fn sng_fd_t sangoma_open_api_span_chan(int span, int chan)
  \brief Open a Device based on Span/Chan values
  \param span span number starting from 1 to 255
  \param chan chan number starting from 1 to 32
  \return File Descriptor: -1 error, 0 or positive integer: valid file descriptor

  Restriced open, device will allowed to be open only once.
*/
sng_fd_t _LIBSNG_CALL sangoma_open_api_span_chan(int span, int chan);


/*!
  \fn sng_fd_t __sangoma_open_api_span_chan(int span, int chan)
  \brief Open a Device based on Span/Chan values
  \param span span number starting from 1 to 255
  \param chan chan number starting from 1 to 32
  \return File Descriptor: -1 error, 0 or positive integer: valid file descriptor

  Unrestriced open, allows mutiple open calls on a single device 
*/
sng_fd_t _LIBSNG_CALL __sangoma_open_api_span_chan(int span, int chan);
#define __sangoma_open_tdmapi_span_chan __sangoma_open_api_span_chan

/*!
  \fn sng_fd_t sangoma_open_tdmapi_span(int span)
  \brief Open a first available device on a Span
  \param span span number starting from 1 to 255
  \return File Descriptor: -1 error, 0 or positive integer: valid file descriptor

  Unrestriced open, allows mutiple open calls on a single device 
*/
sng_fd_t _LIBSNG_CALL sangoma_open_api_span(int span);

/*!
  \fn sng_fd_t sangoma_open_dev_by_name(const char *dev_name)
  \brief Open API device using it's name. For example: Linux: w1g1, Windows wanpipe1_if1.
  \param dev_name  API device name
  \return File Descriptor: -1 error, 0 or positive integer: valid file descriptor
*/
sng_fd_t _LIBSNG_CALL sangoma_open_dev_by_name(const char *dev_name);


/*!
  \def sangoma_create_socket_intr
  \brief Backward compatible open span chan call
*/



/*!
  \def LIBSANGOMA_TDMAPI_CTRL
  \brief Global control device feature
*/
#ifndef LIBSANGOMA_TDMAPI_CTRL
#define LIBSANGOMA_TDMAPI_CTRL 1
#endif

/*!
  \fn sng_fd_t sangoma_open_api_ctrl(void)
  \brief Open a Global Control Device 
  \return File Descriptor - negative=error  0 or greater = fd

  The global control device receives events for all devices
  configured.
*/
sng_fd_t _LIBSNG_CALL sangoma_open_api_ctrl(void);

/*!
  \fn sng_fd_t sangoma_logger_open(void)
  \brief Open a Global Logger Device 
  \return File Descriptor - negative=error  0 or greater = fd

  The global Logger device receives Logger Events for all devices
  configured.
*/
sng_fd_t _LIBSNG_CALL sangoma_logger_open(void);

/*!
  \fn sng_fd_t sangoma_open_driver_ctrl(int port_no)
  \brief Open a Global Driver Control Device
  \return File Descriptor - negative=error  0 or greater = fd

  The global control device receives events for all devices
  configured.
*/
sng_fd_t _LIBSNG_CALL sangoma_open_driver_ctrl(int port_no);



/*!
  \fn void sangoma_close(sng_fd_t *fd)
  \brief Close device file descriptor
  \param fd device file descriptor
  \return void

*/
void _LIBSNG_CALL sangoma_close(sng_fd_t *fd);



/*!
  \fn int sangoma_get_open_cnt(sng_fd_t fd, wanpipe_api_t *tdm_api)
  \brief Get device open count
  \param fd device file descriptor
  \param tdm_api tdm api command structure
  \return negative or 0: error,  greater than 1 : open count
*/

int _LIBSNG_CALL sangoma_get_open_cnt(sng_fd_t fd, wanpipe_api_t *tdm_api);


/************************************************************//**
 * Device READ / WRITE Functions
 ***************************************************************/

/*!
  \fn int sangoma_writemsg(sng_fd_t fd, void *hdrbuf, int hdrlen, void *databuf, unsigned short datalen, int flag)
  \brief Write Data to device
  \param fd device file descriptor 
  \param hdrbuf pointer to header structure wp_api_hdr_t 
  \param hdrlen size of wp_api_hdr_t
  \param databuf pointer to data buffer to be transmitted
  \param datalen length of data buffer
  \param flag currently not used, set to 0
  \return transmit size, must be equal to datalen, anything else is error

   In case of error return code, one must check the header operation_status
   variable to identify the reason of an error. Please refer to the error codes.
*/

int _LIBSNG_CALL sangoma_writemsg(sng_fd_t fd, void *hdrbuf, int hdrlen,
						 void *databuf, unsigned short datalen, int flag);


/*!
  \fn int sangoma_readmsg(sng_fd_t fd, void *hdrbuf, int hdrlen, void *databuf, int datalen, int flag)
  \brief Read Data from device
  \param fd device file descriptor 
  \param hdrbuf pointer to header structure wp_api_hdr_t 
  \param hdrlen size of wp_api_hdr_t
  \param databuf pointer to data buffer to be received
  \param datalen length of data buffer
  \param flag currently not used, set to 0
  \return received size, must be equal to datalen, anything else is error or busy
   
  In case of error return code, one must check the header operation_status
  variable to identify the reason of error. Please refer to the error codes.
*/
int _LIBSNG_CALL sangoma_readmsg(sng_fd_t fd, void *hdrbuf, int hdrlen,
						void *databuf, int datalen, int flag);




/************************************************************//**
 * Device POLL Functions
 ***************************************************************/ 

/*!
  \fn sangoma_status_t _LIBSNG_CALL sangoma_waitfor(sangoma_wait_obj_t *sangoma_wait_obj, int32_t inflags, int32_t *outflags, int32_t timeout)
  \brief Wait for a single waitable object
  \param sangoma_wait_obj pointer to a wait object previously created with sangoma_wait_obj_create
  \param timeout timeout in miliseconds in case of no event
  \return SANG_STATUS_APIPOLL_TIMEOUT: timeout, SANG_STATUS_SUCCESS: event occured use sangoma_wait_obj_get_out_flags to check or error status
*/
sangoma_status_t _LIBSNG_CALL sangoma_waitfor(sangoma_wait_obj_t *sangoma_wait_obj, uint32_t inflags, uint32_t *outflags, int32_t timeout);

/*!
  \fn sangoma_status_t _LIBSNG_CALL sangoma_waitfor_many(sangoma_wait_obj_t *sangoma_wait_objects[], int32_t in_flags[], int32_t out_flags[] uint32_t number_of_sangoma_wait_objects, int32_t system_wait_timeout);
  \brief Wait for multiple sangoma waitable objects 
  \param sangoma_wait_objects pointer to array of wait objects previously created with sangoma_wait_obj_create
  \param in_flags array of flags corresponding to the flags the user is interested on for each waitable object
  \param out_flags array of flags corresponding to the flags that are ready in the waitable objects 
  \param number_of_sangoma_wait_objects size of the array of waitable objects and flags
  \param system_wait_timeout timeout in miliseconds in case of no event
  \return negative: SANG_STATUS_APIPOLL_TIMEOUT: timeout, SANG_STATUS_SUCCESS: event occured check in sangoma_wait_objects, any other return code is some error
*/
sangoma_status_t _LIBSNG_CALL sangoma_waitfor_many(sangoma_wait_obj_t *sangoma_wait_objects[], uint32_t in_flags[], uint32_t out_flags[],
		uint32_t number_of_sangoma_wait_objects, int32_t system_wait_timeout);

/*!
  \fn sangoma_status_t sangoma_wait_obj_create(sangoma_wait_obj_t **sangoma_wait_object, sng_fd_t fd, sangoma_wait_obj_type_t object_type)
  \brief Create a wait object that will be used with sangoma_waitfor_many() API
  \param sangoma_wait_object pointer a single device object 
  \param fd device file descriptor
  \param object_type type of the wait object. see sangoma_wait_obj_type_t for types
  \see sangoma_wait_obj_type_t
  \return SANG_STATUS_SUCCESS: success, or error status
*/
sangoma_status_t _LIBSNG_CALL sangoma_wait_obj_create(sangoma_wait_obj_t **sangoma_wait_object, sng_fd_t fd, sangoma_wait_obj_type_t object_type);

/*!
  \fn sangoma_status_t sangoma_wait_obj_delete(sangoma_wait_obj_t **sangoma_wait_object)
  \brief De-allocate all resources inside a wait object which were allocated by sangoma_wait_obj_init().
  \param sangoma_wait_object pointer to a pointer to a single device object
  \return SANG_STATUS_SUCCESS on success or some sangoma status error
*/
sangoma_status_t _LIBSNG_CALL sangoma_wait_obj_delete(sangoma_wait_obj_t **sangoma_wait_object);

/*!
  \fn void sangoma_wait_obj_signal(sangoma_wait_obj_t *sangoma_wait_object)
  \brief Set wait object to a signaled state
  \param sangoma_wait_object pointer a single device object that can be signaled
  \return sangoma_status_t
*/
sangoma_status_t _LIBSNG_CALL sangoma_wait_obj_signal(sangoma_wait_obj_t *sangoma_wait_object);

/*!
  \fn sng_fd_t sangoma_wait_obj_get_fd(sangoma_wait_obj_t *sangoma_wait_object)
  \brief Get fd device file descriptor which was the 'fd' parameter for sangoma_wait_obj_create(), not useful for generic objects
  \param sangoma_wait_object pointer a single device object 
  \return sng_fd_t - device file descriptor
*/
sng_fd_t _LIBSNG_CALL sangoma_wait_obj_get_fd(sangoma_wait_obj_t *sangoma_wait_object);

/*!
  \fn void sangoma_wait_obj_set_context(sangoma_wait_obj_t *sangoma_wait_object)
  \brief Store the given context into provided sangoma wait object.
  \brief This function is useful to associate a context with a sangoma wait object.
  \param sangoma_wait_object pointer a single device object 
  \param context void pointer to user context
  \return void
*/
void _LIBSNG_CALL sangoma_wait_obj_set_context(sangoma_wait_obj_t *sangoma_wait_object, void *context);

/*!
  \fn PVOID sangoma_wait_obj_get_context(sangoma_wait_obj_t *sangoma_wait_object)
  \brief Retrieve the user context (if any) that was set via sangoma_wait_obj_set_context.
  \brief Windows note: must use return type PVOID instead of void* to satisfy WDK compiler.
  \param sangoma_wait_object pointer a single device object 
  \return void*
*/
PVOID _LIBSNG_CALL sangoma_wait_obj_get_context(sangoma_wait_obj_t *sangoma_wait_object);


/************************************************************//**
 * Device API COMMAND Functions
 ***************************************************************/ 

/*!
  \fn int sangoma_cmd_exec(sng_fd_t fd, wanpipe_api_t *tdm_api)
  \brief Execute Sangoma API Command
  \param fd device file descriptor
  \param tdm_api tdm api command structure
  \return non-zero: error, 0: ok
*/
int _LIBSNG_CALL sangoma_cmd_exec(sng_fd_t fd, wanpipe_api_t *tdm_api);


/*!
  \fn int sangoma_get_full_cfg(sng_fd_t fd, wanpipe_api_t *tdm_api)
  \brief Read tdm api device configuration
  \param fd device file descriptor
  \param tdm_api tdm api command structure
  \return non-zero: error, 0: ok
*/
int _LIBSNG_CALL sangoma_get_full_cfg(sng_fd_t fd, wanpipe_api_t *tdm_api);

/*!
  \fn int sangoma_tdm_set_usr_period(sng_fd_t fd, wanpipe_api_t *tdm_api, int period)
  \brief Set Tx/Rx Period in Milliseconds 
  \param fd device file descriptor
  \param tdm_api tdm api command structure
  \param period value in miliseconds (1,2,5,10)
  \return non-zero: error, 0: ok
  
  Only valid in CHAN Operation Mode
*/
int _LIBSNG_CALL sangoma_tdm_set_usr_period(sng_fd_t fd, wanpipe_api_t *tdm_api, int period);

/*!
  \fn int sangoma_tdm_get_usr_period(sng_fd_t fd, wanpipe_api_t *tdm_api)
  \brief Get Tx/Rx Period in Milliseconds
  \param fd device file descriptor
  \param tdm_api tdm api command structure
  \return negative: error or configured period value
*/
int _LIBSNG_CALL sangoma_tdm_get_usr_period(sng_fd_t fd, wanpipe_api_t *tdm_api);

/*!
  \fn int sangoma_tdm_get_usr_mtu_mru(sng_fd_t fd, wanpipe_api_t *tdm_api)
  \brief Get Tx/Rx MTU/MRU in bytes
  \param fd device file descriptor
  \param tdm_api tdm api command structure
  \return negative: error or configured mtu/mru in bytes
*/
int _LIBSNG_CALL sangoma_tdm_get_usr_mtu_mru(sng_fd_t fd, wanpipe_api_t *tdm_api);


/*!
  \fn int sangoma_flush_bufs(sng_fd_t fd, wanpipe_api_t *tdm_api)
  \brief Flush all (tx/rx/event) buffers from current channel
  \param fd device file descriptor
  \param tdm_api tdm api command structure
  \return non-zero: error, 0: ok
  
*/
int _LIBSNG_CALL sangoma_flush_bufs(sng_fd_t fd, wanpipe_api_t *tdm_api);

/*!
  \fn int sangoma_flush_rx_bufs(sng_fd_t fd, wanpipe_api_t *tdm_api)
  \brief Flush only rx buffers from current channel
  \param fd device file descriptor
  \param tdm_api tdm api command structure
  \return non-zero: error, 0: ok
  
*/
int _LIBSNG_CALL sangoma_flush_rx_bufs(sng_fd_t fd, wanpipe_api_t *tdm_api);
/*!
  \fn int sangoma_flush_tx_bufs(sng_fd_t fd, wanpipe_api_t *tdm_api)
  \brief Flush only tx buffers from current channel
  \param fd device file descriptor
  \param tdm_api tdm api command structure
  \return non-zero: error, 0: ok
  
*/
int _LIBSNG_CALL sangoma_flush_tx_bufs(sng_fd_t fd, wanpipe_api_t *tdm_api);

/*!
  \fn int sangoma_flush_event_bufs(sng_fd_t fd, wanpipe_api_t *tdm_api)
  \brief Flush only event buffers from current channel
  \param fd device file descriptor
  \param tdm_api tdm api command structure
  \return non-zero: error, 0: ok
  
*/
int _LIBSNG_CALL sangoma_flush_event_bufs(sng_fd_t fd, wanpipe_api_t *tdm_api);


/*!
  \fn int sangoma_tdm_enable_rbs_events(sng_fd_t fd, wanpipe_api_t *tdm_api, int poll_in_sec)
  \brief Enable RBS Events on a device
  \param fd device file descriptor
  \param tdm_api tdm api command structure
  \param poll_in_sec driver poll period for rbs events
  \return non-zero: error, 0: ok
*/
int _LIBSNG_CALL sangoma_tdm_enable_rbs_events(sng_fd_t fd, wanpipe_api_t *tdm_api, int poll_in_sec);

/*!
  \fn int sangoma_tdm_disable_rbs_events(sng_fd_t fd, wanpipe_api_t *tdm_api)
  \brief Disable RBS Events for a device
  \param fd device file descriptor
  \param tdm_api tdm api command structure
  \return non-zero: error, 0: ok
*/
int _LIBSNG_CALL sangoma_tdm_disable_rbs_events(sng_fd_t fd, wanpipe_api_t *tdm_api);

/*!
  \fn int sangoma_tdm_write_rbs(sng_fd_t fd, wanpipe_api_t *tdm_api, int channel, unsigned char rbs)
  \brief Write RBS Bits on a device
  \param fd device file descriptor
  \param tdm_api tdm api command structure
  \param channel t1/e1 timeslot
  \param rbs rbs bits (ABCD)
  \return non-zero: error, 0: ok
*/
int _LIBSNG_CALL sangoma_tdm_write_rbs(sng_fd_t fd, wanpipe_api_t *tdm_api, int channel, unsigned char rbs);

/*!
  \fn int sangoma_tdm_read_rbs(sng_fd_t fd, wanpipe_api_t *tdm_api, int channel, unsigned char *rbs)
  \brief Read RBS Bits on a device
  \param fd device file descriptor
  \param tdm_api tdm api command structure
  \param channel t1/e1 timeslot
  \param rbs pointer to rbs bits (ABCD) 
  \return non-zero: error, 0: ok
*/

int _LIBSNG_CALL sangoma_tdm_read_rbs(sng_fd_t fd, wanpipe_api_t *tdm_api, int channel, unsigned char *rbs);

/*!
  \fn int sangoma_tdm_enable_dtmf_events(sng_fd_t fd, wanpipe_api_t *tdm_api)
  \brief Enable DTMF Detection on Octasic chip (if hw supports it)
  \param fd device file descriptor
  \param tdm_api tdm api command structure
  \return non-zero: error, 0: ok
  
  Supported only on cards that have HWEC
*/
int _LIBSNG_CALL sangoma_tdm_enable_dtmf_events(sng_fd_t fd, wanpipe_api_t *tdm_api);

/*!
  \fn int sangoma_tdm_disable_dtmf_events(sng_fd_t fd, wanpipe_api_t *tdm_api)
  \brief Disable DTMF Detection on Octasic chip (if hw supports it)
  \param fd device file descriptor
  \param tdm_api tdm api command structure
  \return non-zero: error, 0: ok
  
  Supported only on cards that have HWEC
*/
int _LIBSNG_CALL sangoma_tdm_disable_dtmf_events(sng_fd_t fd, wanpipe_api_t *tdm_api);


#ifdef WP_API_FEATURE_FAX_EVENTS
/*!
  \fn int sangoma_tdm_enable_fax_events(sng_fd_t fd, wanpipe_api_t *tdm_api)
  \brief Enable FAX Detection on Octasic chip (if hw supports it)
  \param fd device file descriptor
  \param tdm_api tdm api command structure
  \return non-zero: error, 0: ok
  
  Supported only on cards that have HWEC
*/
int _LIBSNG_CALL sangoma_tdm_enable_fax_events(sng_fd_t fd, wanpipe_api_t *tdm_api);

/*!
  \fn int sangoma_tdm_disable_fax_events(sng_fd_t fd, wanpipe_api_t *tdm_api)
  \brief Disable FAX Detection on Octasic chip (if hw supports it)
  \param fd device file descriptor
  \param tdm_api tdm api command structure
  \return non-zero: error, 0: ok
  
  Supported only on cards that have HWEC
*/
int _LIBSNG_CALL sangoma_tdm_disable_fax_events(sng_fd_t fd, wanpipe_api_t *tdm_api);

/*!
  \fn int sangoma_tdm_get_hw_fax(sng_fd_t fd, wanpipe_api_t *tdm_api)
  \brief Get HW FAX Detection State (Enable or Disabled) on Octasic chip (if hw supports it)
  \param fd device file descriptor
  \param tdm_api tdm api command structure
  \return non-zero: error, 0: Disabled, 1: Enabled
  
  Supported only on cards that have HWEC
*/
int _LIBSNG_CALL sangoma_tdm_get_hw_fax(sng_fd_t fd, wanpipe_api_t *tdm_api);

#endif


/*!
  \fn int sangoma_tdm_enable_rm_dtmf_events(sng_fd_t fd, wanpipe_api_t *tdm_api)
  \brief Enable DTMF Detection on Analog/Remora SLIC Chip
  \param fd device file descriptor
  \param tdm_api tdm api command structure
  \return non-zero: error, 0: ok

  Supported only on Analog Cards
*/
int _LIBSNG_CALL sangoma_tdm_enable_rm_dtmf_events(sng_fd_t fd, wanpipe_api_t *tdm_api);

/*!
  \fn int sangoma_tdm_disable_rm_dtmf_events(sng_fd_t fd, wanpipe_api_t *tdm_api)
  \brief Disable DTMF Detection on Analog/Remora SLIC Chip
  \param fd device file descriptor
  \param tdm_api tdm api command structure
  \return non-zero: error, 0: ok
  
  Supported only on Analog Cards
*/
int _LIBSNG_CALL sangoma_tdm_disable_rm_dtmf_events(sng_fd_t fd, wanpipe_api_t *tdm_api);

/*!
  \fn int sangoma_tdm_enable_rxhook_events(sng_fd_t fd, wanpipe_api_t *tdm_api)
  \brief Enable RX HOOK Events (Analog Only)
  \param fd device file descriptor
  \param tdm_api tdm api command structure
  \return non-zero: error, 0: ok
  
  Supported only on Analog Cards
*/
int _LIBSNG_CALL sangoma_tdm_enable_rxhook_events(sng_fd_t fd, wanpipe_api_t *tdm_api);

/*!
  \fn int sangoma_tdm_disable_rxhook_events(sng_fd_t fd, wanpipe_api_t *tdm_api)
  \brief Disable RX HOOK Events (Analog Only)
  \param fd device file descriptor
  \param tdm_api tdm api command structure
  \return non-zero: error, 0: ok
  
  Supported only on Analog Cards
*/
int _LIBSNG_CALL sangoma_tdm_disable_rxhook_events(sng_fd_t fd, wanpipe_api_t *tdm_api);

/*!
  \fn int sangoma_tdm_enable_ring_events(sng_fd_t fd, wanpipe_api_t *tdm_api)
  \brief Enable RING Events (Analog Only)
  \param fd device file descriptor
  \param tdm_api tdm api command structure
  \return non-zero: error, 0: ok
  
  Supported only on Analog Cards
*/
int _LIBSNG_CALL sangoma_tdm_enable_ring_events(sng_fd_t fd, wanpipe_api_t *tdm_api);

/*!
  \fn int sangoma_tdm_disable_ring_events(sng_fd_t fd, wanpipe_api_t *tdm_api)
  \brief Disable RING Events (Analog Only)
  \param fd device file descriptor
  \param tdm_api tdm api command structure
  \return non-zero: error, 0: ok
  
  Supported only on Analog Cards
*/
int _LIBSNG_CALL sangoma_tdm_disable_ring_events(sng_fd_t fd, wanpipe_api_t *tdm_api);


/*!
  \fn int sangoma_tdm_enable_ring_detect_events(sng_fd_t fd, wanpipe_api_t *tdm_api)
  \brief Enable RING DETECT Events (Analog Only)
  \param fd device file descriptor
  \param tdm_api tdm api command structure
  \return non-zero: error, 0: ok
  
  Supported only on Analog Cards
*/
int _LIBSNG_CALL sangoma_tdm_enable_ring_detect_events(sng_fd_t fd, wanpipe_api_t *tdm_api);

/*!
  \fn int sangoma_tdm_disable_ring_detect_events(sng_fd_t fd, wanpipe_api_t *tdm_api)
  \brief Disable RING DETECT Events (Analog Only)
  \param fd device file descriptor
  \param tdm_api tdm api command structure
  \return non-zero: error, 0: ok
  
  Supported only on Analog Cards
*/
int _LIBSNG_CALL sangoma_tdm_disable_ring_detect_events(sng_fd_t fd, wanpipe_api_t *tdm_api);

/*!
  \fn int sangoma_tdm_enable_ring_trip_detect_events(sng_fd_t fd, wanpipe_api_t *tdm_api)
  \brief Enable RING TRIP Events (Analog Only)
  \param fd device file descriptor
  \param tdm_api tdm api command structure
  \return non-zero: error, 0: ok
  
  Supported only on Analog Cards
*/
int _LIBSNG_CALL sangoma_tdm_enable_ring_trip_detect_events(sng_fd_t fd, wanpipe_api_t *tdm_api);

/*!
  \fn int sangoma_tdm_disable_ring_trip_detect_events(sng_fd_t fd, wanpipe_api_t *tdm_api)
  \brief Disable RING TRIP Events (Analog Only)
  \param fd device file descriptor
  \param tdm_api tdm api command structure
  \return non-zero: error, 0: ok
  
  Supported only on Analog Cards
*/
int _LIBSNG_CALL sangoma_tdm_disable_ring_trip_detect_events(sng_fd_t fd, wanpipe_api_t *tdm_api);

/*!
  \fn int sangoma_tdm_enable_tone_events(sng_fd_t fd, wanpipe_api_t *tdm_api, uint16_t tone_id)
  \brief Transmit a TONE on this device (Analog Only)
  \param fd device file descriptor
  \param tdm_api tdm api command structure
  \param tone_id tone type to transmit 
  \return non-zero: error, 0: ok
  
  Supported only on Analog Cards
*/
int _LIBSNG_CALL sangoma_tdm_enable_tone_events(sng_fd_t fd, wanpipe_api_t *tdm_api, uint16_t tone_id);

/*!
  \fn int sangoma_tdm_disable_tone_events(sng_fd_t fd, wanpipe_api_t *tdm_api)
  \brief Enable TONE Events (Analog Only)
  \param fd device file descriptor
  \param tdm_api tdm api command structure
  \return non-zero: error, 0: ok
  
  Supported only on Analog Cards
*/
int _LIBSNG_CALL sangoma_tdm_disable_tone_events(sng_fd_t fd, wanpipe_api_t *tdm_api);

/*!
  \fn int sangoma_tdm_txsig_onhook(sng_fd_t fd, wanpipe_api_t *tdm_api)
  \brief Tranmsmit TX SIG ON HOOK (Analog Only)
  \param fd device file descriptor
  \param tdm_api tdm api command structure
  \return non-zero: error, 0: ok
  
  Supported only on Analog Cards
*/
int _LIBSNG_CALL sangoma_tdm_txsig_onhook(sng_fd_t fd, wanpipe_api_t *tdm_api);

/*!
  \fn int sangoma_tdm_txsig_offhook(sng_fd_t fd, wanpipe_api_t *tdm_api)
  \brief Tranmsmit TX SIG OFF HOOK (Analog Only)
  \param fd device file descriptor
  \param tdm_api tdm api command structure
  \return non-zero: error, 0: ok
  
  Supported only on Analog Cards
*/
int _LIBSNG_CALL sangoma_tdm_txsig_offhook(sng_fd_t fd, wanpipe_api_t *tdm_api);

/*!
  \fn int sangoma_tdm_txsig_start(sng_fd_t fd, wanpipe_api_t *tdm_api)
  \brief Tranmsmit TX SIG START (Analog Only)
  \param fd device file descriptor
  \param tdm_api tdm api command structure
  \return non-zero: error, 0: ok
  
  Supported only on Analog Cards
*/
int _LIBSNG_CALL sangoma_tdm_txsig_start(sng_fd_t fd, wanpipe_api_t *tdm_api);

/*!
  \fn int sangoma_tdm_txsig_kewl(sng_fd_t fd, wanpipe_api_t *tdm_api)
  \brief Tranmsmit TX SIG KEWL START (Analog Only)
  \param fd device file descriptor
  \param tdm_api tdm api command structure
  \return non-zero: error, 0: ok
  
  Supported only on Analog Cards
*/
int _LIBSNG_CALL sangoma_tdm_txsig_kewl(sng_fd_t fd, wanpipe_api_t *tdm_api);

/*!
  \fn int sangoma_tdm_enable_hwec(sng_fd_t fd, wanpipe_api_t *tdm_api)
  \brief Enable HWEC on this channel 
  \param fd device file descriptor
  \param tdm_api tdm api command structure
  \return non-zero: error, 0: ok
  
  Supported only on cards that have HWEC
*/
int _LIBSNG_CALL sangoma_tdm_enable_hwec(sng_fd_t fd, wanpipe_api_t *tdm_api);

/*!
  \fn int sangoma_tdm_disable_hwec(sng_fd_t fd, wanpipe_api_t *tdm_api)
  \brief Disable HWEC on this channel
  \param fd device file descriptor
  \param tdm_api tdm api command structure
  \return non-zero: error, 0: ok
  
  Supported only on cards that have HWEC
*/
int _LIBSNG_CALL sangoma_tdm_disable_hwec(sng_fd_t fd, wanpipe_api_t *tdm_api);

/*!
  \fn int _LIBSNG_CALL sangoma_tdm_get_fe_alarms(sng_fd_t fd, wanpipe_api_t *tdm_api, unsigned int *alarms);
  \brief Get Front End Alarms (T1/E1 Only)
  \param fd device file descriptor
  \param tdm_api tdm api command structure
  \param alarms bit map status of T1/E1 alarms 
  \return non-zero: error, 0: ok
  
  Supported only on T1/E1 Cards
*/
int _LIBSNG_CALL sangoma_tdm_get_fe_alarms(sng_fd_t fd, wanpipe_api_t *tdm_api, unsigned int *alarms);



#ifdef WP_API_FEATURE_LINK_STATUS
# ifndef LIBSANGOMA_GET_LINKSTATUS
/*!
  \def LIBSANGOMA_GET_LINKSTATUS
  \brief Get Link Status feature 
*/
# define LIBSANGOMA_GET_LINKSTATUS 1
# endif

/*!
  \fn int sangoma_get_link_status(sng_fd_t fd, wanpipe_api_t *tdm_api, unsigned char *current_status)
  \brief Get Device Link Status (Connected/Disconnected)
  \param fd device file descriptor
  \param tdm_api tdm api command structure
  \param current_status pointer where result will be filled: 0=Link UP 1=Link Down
  \return non-zero: error, 0: ok -> check current_status 

*/
int _LIBSNG_CALL sangoma_get_link_status(sng_fd_t fd, wanpipe_api_t *tdm_api, unsigned char *current_status);

#endif

/* set current Line Connection state - Connected/Disconnected */
#ifndef LIBSANGOMA_GET_FESTATUS
/*!
  \def LIBSANGOMA_GET_FESTATUS
  \brief Get Front End Status feature
*/
#define LIBSANGOMA_GET_FESTATUS 1
#endif

/*!
  \fn int sangoma_set_fe_status(sng_fd_t fd, wanpipe_api_t *tdm_api, unsigned char new_status)
  \brief Set Device Link Status (Connected/Disconnected) 
  \param fd device file descriptor
  \param tdm_api tdm api command structure
  \param new_status new status  0=Link UP  1=Link Down
  \return non-zero: error, 0: ok
*/
int _LIBSNG_CALL sangoma_set_fe_status(sng_fd_t fd, wanpipe_api_t *tdm_api, unsigned char new_status);



#ifdef WP_API_FEATURE_BUFFER_MULT
/*!
  \fn int sangoma_tdm_set_buffer_multiplier(sng_fd_t fd, wanpipe_api_t *tdm_api, unsigned int multiplier)
  \brief Set voice tx/rx buffer multiplier. 
  \param fd device file descriptor
  \param tdm_api tdm api command structure
  \param multiplier buffer multiplier value 0-disable or 1 to TDMAPI_MAX_BUFFER_MULTIPLIER
  \return non-zero: error, 0: ok
*/
int _LIBSNG_CALL sangoma_tdm_set_buffer_multiplier(sng_fd_t fd, wanpipe_api_t *tdm_api, unsigned int multiplier);

#endif


/*!
  \fn int _LIBSNG_CALL sangoma_enable_bri_bchan_loopback(sng_fd_t fd, wanpipe_api_t *tdm_api, int channel)
  \brief Enable BRI Bchannel loopback - used when debugging bri device
  \param fd device file descriptor
  \param tdm_api tdm api command structure
  \param channel bri bchannel 1 or 2
  \return non-zero: error, 0: ok
 
*/
int _LIBSNG_CALL sangoma_enable_bri_bchan_loopback(sng_fd_t fd, wanpipe_api_t *tdm_api, int channel);

/*!
  \fn int _LIBSNG_CALL sangoma_disable_bri_bchan_loopback(sng_fd_t fd, wanpipe_api_t *tdm_api, int channel)
  \brief Disable BRI Bchannel loopback - used when debugging bri device
  \param fd device file descriptor
  \param tdm_api tdm api command structure
  \param channel bri bchannel 1 or 2
  \return non-zero: error, 0: ok
 
*/
int _LIBSNG_CALL sangoma_disable_bri_bchan_loopback(sng_fd_t fd, wanpipe_api_t *tdm_api, int channel);


/*!
  \fn int sangoma_get_tx_queue_sz(sng_fd_t fd, wanpipe_api_t *tdm_api)
  \brief Get Tx Queue Size for this channel
  \param fd device file descriptor
  \param tdm_api tdm api command structure
  \return non-zero: error, 0: ok
*/
int _LIBSNG_CALL sangoma_get_tx_queue_sz(sng_fd_t fd, wanpipe_api_t *tdm_api);


/*!
  \fn int sangoma_set_tx_queue_sz(sng_fd_t fd, wanpipe_api_t *tdm_api, int size)
  \brief Get Tx Queue Size for this channel
  \param fd device file descriptor
  \param tdm_api tdm api command structure
  \param size tx queue size (minimum value of 1) 
  \return non-zero: error, 0: ok
*/
int _LIBSNG_CALL sangoma_set_tx_queue_sz(sng_fd_t fd, wanpipe_api_t *tdm_api, int size);

/*!
  \fn int sangoma_get_rx_queue_sz(sng_fd_t fd, wanpipe_api_t *tdm_api)
  \brief Get Rx Queue Size for this channel
  \param fd device file descriptor
  \param tdm_api tdm api command structure
  \return non-zero: error, 0: ok
*/
int _LIBSNG_CALL sangoma_get_rx_queue_sz(sng_fd_t fd, wanpipe_api_t *tdm_api);

/*!
  \fn int sangoma_set_rx_queue_sz(sng_fd_t fd, wanpipe_api_t *tdm_api, int size)
  \brief Get Tx Queue Size for this channel
  \param fd device file descriptor
  \param tdm_api tdm api command structure
  \param size rx queue size (minimum value of 1)
  \return non-zero: error, 0: ok
*/
int _LIBSNG_CALL sangoma_set_rx_queue_sz(sng_fd_t fd, wanpipe_api_t *tdm_api, int size);


#ifndef LIBSANGOMA_GET_HWCODING
/*!
  \def LIBSANGOMA_GET_HWCODING
  \brief Get HW Coding Feature
*/
#define LIBSANGOMA_GET_HWCODING 1
#endif

/*!
  \fn int sangoma_get_hw_coding(sng_fd_t fd, wanpipe_api_t *tdm_api)
  \brief Get HW Voice Coding (ulaw/alaw)
  \param fd device file descriptor
  \param tdm_api tdm api command structure
  \return the low level voice coding, depending on configuration.  (WP_MULAW or WP_ALAW)
*/
int _LIBSNG_CALL sangoma_get_hw_coding(sng_fd_t fd, wanpipe_api_t *tdm_api);



#ifndef LIBSANGOMA_GET_HWDTMF
/*!
  \def LIBSANGOMA_GET_HWDTMF
  \brief HW DTMF Feature
*/
#define LIBSANGOMA_GET_HWDTMF 1
#endif
/*!
  \fn int sangoma_tdm_get_hw_dtmf(sng_fd_t fd, wanpipe_api_t *tdm_api)
  \brief Check if hwdtmf support is available
  \param fd device file descriptor
  \param tdm_api tdm api command structure
  \return non-zero: error, 0: ok

  This function will check if hw supports HW DTMF.
*/
int _LIBSNG_CALL sangoma_tdm_get_hw_dtmf(sng_fd_t fd, wanpipe_api_t *tdm_api);

/*!
  \fn int sangoma_tdm_get_hw_ec(sng_fd_t fd, wanpipe_api_t *tdm_api)
  \brief Check if hw echo cancelation support is available
  \param fd device file descriptor
  \param tdm_api tdm api command structure
  \return non-zero: error, 0: disable, >0:enabled

  This function will check if hw supports HW EC.
*/
int _LIBSNG_CALL sangoma_tdm_get_hw_ec(sng_fd_t fd, wanpipe_api_t *tdm_api);


#ifdef WP_API_FEATURE_EC_CHAN_STAT
/*!
  \fn int sangoma_tdm_get_hwec_chan_status(sng_fd_t fd, wanpipe_api_t *tdm_api)
  \brief Check if hw echo cancelation is enabled on current timeslot
  \param fd device file descriptor
  \param tdm_api tdm api command structure
  \return non-zero: error, 0: disabled, >0: enabled

  This function will check if hw echo cancelation is enable
  on current timeslot.
*/

int _LIBSNG_CALL sangoma_tdm_get_hwec_chan_status(sng_fd_t fd, wanpipe_api_t *tdm_api);

#endif

#ifdef WP_API_FEATURE_HWEC_PERSIST    
/*!
  \fn int sangoma_tdm_get_hwec_persist_status(sng_fd_t fd, wanpipe_api_t *tdm_api) 
  \brief Check if hwec persis mode is on: On persist mode hwec is always enabled.
  \param fd device file descriptor
  \param tdm_api tdm api command structure
  \return non-zero: error, 0: disabled, >0: enabled

  This function will check if hw persist mode is enabled.
*/

int _LIBSNG_CALL sangoma_tdm_get_hwec_persist_status(sng_fd_t fd, wanpipe_api_t *tdm_api);   
#endif

/*!
  \fn int sangoma_span_chan_toif(int span, int chan, char *interface_name)
  \brief Convert Span & Chan to interface name
  \param span span number starting from 1 to 255
  \param chan chan number starting from 1 to 32
  \param interface_name pointer to string where interface name will be written
  \return non-zero = error, 0 = ok
*/
int _LIBSNG_CALL sangoma_span_chan_toif(int span, int chan, char *interface_name);

/*!
  \fn int sangoma_span_chan_fromif(char *interface_name, int *span, int *chan)
  \brief Convert Interace Name to Span & Chan 
  \param interface_name pointer to string containing interface name
  \param span integer pointer where to write span value
  \param chan integer pointer where to write chan value
  \return non-zero = error, 0 = ok
*/
int _LIBSNG_CALL sangoma_span_chan_fromif(char *interface_name, int *span, int *chan);


/*!
  \fn int sangoma_interface_wait_up(int span, int chan, int sectimeout)
  \brief Wait for a sangoma device to come up (ie: Linux wait for /dev/wanpipex_1 to come up)
  \param span span number of the device to wait
  \param chan chan number of the device to wait
  \param sectimeout how many seconds to wait for the device to come up, -1 to wait forever
  \return non-zero = error, 0 = ok
*/
int _LIBSNG_CALL sangoma_interface_wait_up(int span, int chan, int sectimeout);

/*!
  \fn int sangoma_get_driver_version(sng_fd_t fd, wanpipe_api_t *tdm_api, wan_driver_version_t *drv_ver)
  \brief Get Device Driver Version Number
  \param fd device file descriptor
  \param tdm_api tdm api command structure
  \param drv_ver driver version structure that will contain the driver version
  \return non-zero = error, 0 = ok
*/
int _LIBSNG_CALL sangoma_get_driver_version(sng_fd_t fd, wanpipe_api_t *tdm_api, wan_driver_version_t *drv_ver);

/*!
  \fn int sangoma_get_firmware_version(sng_fd_t fd, wanpipe_api_t *tdm_api, unsigned char *ver)
  \brief Get Hardware/Firmware Version
  \param fd device file descriptor
  \param tdm_api tdm api command structure
  \param ver hardware/firmware version number
  \return non-zero = error, 0 = ok
*/
int _LIBSNG_CALL sangoma_get_firmware_version(sng_fd_t fd, wanpipe_api_t *tdm_api, unsigned char *ver);

/*!
  \fn int sangoma_get_cpld_version(sng_fd_t fd, wanpipe_api_t *tdm_api, unsigned char *ver)
  \brief Get AFT CPLD Version
  \param fd device file descriptor
  \param tdm_api tdm api command structure
  \param ver AFT CPLD version number
  \return non-zero = error, 0 = ok
*/
int _LIBSNG_CALL sangoma_get_cpld_version(sng_fd_t fd, wanpipe_api_t *tdm_api, unsigned char *ver);

/*!
  \fn int _LIBSNG_CALL sangoma_get_aft_customer_id(sng_fd_t fd, unsigned char *out_customer_id)
  \brief Get Customer-specific ID from AFT hardware, the default value is 0xFF, any change
			requires special arrangement with Sangoma Technologies.
  \param fd device file descriptor
  \param out_customer_id AFT Customer ID
  \return non-zero = error, 0 = ok
*/

int _LIBSNG_CALL sangoma_get_aft_customer_id(sng_fd_t fd, unsigned char *out_customer_id);

#ifdef WP_API_FEATURE_LED_CTRL
/*!
  \fn int _LIBSNG_CALL sangoma_port_led_ctrl(sng_fd_t fd, unsigned char led_state)
  \brief Control the LED ligths of the TDM port. On (led set based on link status) Off (turn off all led). 
         Used to visually identify a phisical port from software. 
  \param fd device file descriptor
  \param led_state 0=off 1=on
  \return non-zero = error, 0 = ok
*/

int _LIBSNG_CALL sangoma_port_led_ctrl(sng_fd_t fd, unsigned char led_state);   

#endif


#ifdef  WP_API_FEATURE_FE_RW
/*!
  \fn int sangoma_fe_reg_write(sng_fd_t fd, uint32_t offset, uint8_t data)
  \brief Write to a front end register
  \param fd device file descriptor
  \param offset offset of front end register
  \param data value to write
  \return non-zero = error, 0 = ok
 */


int _LIBSNG_CALL sangoma_fe_reg_write(sng_fd_t fd, uint32_t offset, uint8_t data);

/*!
  \fn int sangoma_fe_reg_read(sng_fd_t fd, uint32_t offset, uint8_t *data)
  \brief Read front end register
  \param fd device file descriptor
  \param offset offset of front end register
  \param data value of the read register
  \return non-zero = error, 0 = ok
 */
int _LIBSNG_CALL sangoma_fe_reg_read(sng_fd_t fd, uint32_t offset, uint8_t *data);
#endif

/*!
  \fn int sangoma_get_stats(sng_fd_t fd, wanpipe_api_t *tdm_api, wanpipe_chan_stats_t *stats)
  \brief Get Device Statistics. Statistics will be available in tdm_api->wp_cmd.stats structure.
  \param fd device file descriptor
  \param tdm_api tdm api command structure
  \param stats stats structure will be filled with device stats. (Optional, can be left NULL)
  \return non-zero = error, 0 = ok
 */
int _LIBSNG_CALL sangoma_get_stats(sng_fd_t fd, wanpipe_api_t *tdm_api, wanpipe_chan_stats_t *stats);

/*!
  \fn int _LIBSNG_CALL sangoma_flush_stats(sng_fd_t fd, wanpipe_api_t *tdm_api)
  \brief Flush/Reset device statistics
  \param fd device file descriptor
  \param tdm_api tdm api command structure
  \return non-zero = error, 0 = ok
*/
int _LIBSNG_CALL sangoma_flush_stats(sng_fd_t fd, wanpipe_api_t *tdm_api);

/*!
  \fn int sangoma_set_rm_rxflashtime(sng_fd_t fd, wanpipe_api_t *tdm_api, int rxflashtime)
  \brief Set rxflashtime for FXS module Wink-Flash Event
  \param fd device file descriptor
  \param tdm_api tdm api command structure
  \param rxflashtime time value 
  \return non-zero = error, 0 = ok
*/
int _LIBSNG_CALL sangoma_set_rm_rxflashtime(sng_fd_t fd, wanpipe_api_t *tdm_api, int rxflashtime);

#ifdef WP_API_FEATURE_RM_GAIN
/*!
  \fn int sangoma_set_rm_tx_gain(sng_fd_t fd, wanpipe_api_t *tdm_api, int value)
  \brief set tx gain for FXO/FXS module
  \param fd device file descriptor
  \param tdm_api tdm api command structure
  \param value txgain (FXO - txgain value ranges from -150 to 120 , FXS - txgain value 35,-35)
  \return non-zero = error, 0 = ok
*/
int _LIBSNG_CALL sangoma_set_rm_tx_gain(sng_fd_t fd, wanpipe_api_t *tdm_api, int value);


/*!
  \fn int sangoma_set_rm_rx_gain(sng_fd_t fd, wanpipe_api_t *tdm_api, int value)
  \brief set rx gain for FXO/FXS module
  \param fd device file descriptor
  \param tdm_api tdm api command structure
  \param value rxgain (FXO - rxgain value ranges from -150 to 120 , FXS -rxgain value 35,-35) 
  \return non-zero = error, 0 = ok
*/
int _LIBSNG_CALL sangoma_set_rm_rx_gain(sng_fd_t fd, wanpipe_api_t *tdm_api, int value);

#endif /* WP RM GAIN feature */

/*!
  \fn int sangoma_set_polarity(sng_fd_t fd, wanpipe_api_t *tdm_api, int value)
  \brief set polarity on FXS (Analog only)
  \param fd device file descriptor
  \param tdm_api tdm api command structure
  \param value 0 fwd, 1 rev
  \return non-zero = error, 0 = ok
*/
int _LIBSNG_CALL sangoma_tdm_set_polarity(sng_fd_t fd, wanpipe_api_t *tdm_api, int polarity);

/*!
  \fn int sangoma_tdm_txsig_onhooktranfer(sng_fd_t fd, wanpipe_api_t *tdm_api)
  \brief Tranmsmit TX SIG ON HOOK Tranfer (Analog Only)
  \param fd device file descriptor
  \param tdm_api tdm api command structure
  \return non-zero: error, 0: ok
  
  Supported only on Analog Cards
*/
int _LIBSNG_CALL sangoma_tdm_txsig_onhooktransfer(sng_fd_t fd, wanpipe_api_t *tdm_api);


#ifdef WP_API_FEATURE_LOOP
/*!
  \fn int sangoma_tdm_enable_loop(sng_fd_t fd, wanpipe_api_t *tdm_api)
  \brief Enable channel loop: All rx data will be transmitted back out.
  \param fd device file descriptor
  \param tdm_api tdm api command structure
  \return non-zero = error, 0 = ok
*/ 
int _LIBSNG_CALL sangoma_tdm_enable_loop(sng_fd_t fd, wanpipe_api_t *tdm_api);

/*!
  \fn int sangoma_tdm_disable_loop(sng_fd_t fd, wanpipe_api_t *tdm_api)
  \brief Disable channel loop
  \param fd device file descriptor
  \param tdm_api tdm api command structure
  \return non-zero = error, 0 = ok
*/
int _LIBSNG_CALL sangoma_tdm_disable_loop(sng_fd_t fd, wanpipe_api_t *tdm_api);    
#endif

/************************************************************//**
 * Device EVENT Function
 ***************************************************************/ 


/*!
  \fn int sangoma_read_event(sng_fd_t fd, wanpipe_api_t *tdm_api)
  \brief Read API Events
  \param fd device file descriptor
  \param tdm_api tdm api command structure
  \return non-zero: error, 0: ok
  
  The TDM API structure will be populated with a TDM API or WAN Event.
  This function usually used after wait() function indicated that event
  has occured.
*/
int _LIBSNG_CALL sangoma_read_event(sng_fd_t fd, wanpipe_api_t *tdm_api);

#ifdef WP_API_FEATURE_LOGGER

sangoma_status_t _LIBSNG_CALL sangoma_logger_cmd_exec(sng_fd_t fd, wp_logger_cmd_t *logger_cmd);

/*!
  \fn sangoma_status_t sangoma_logger_read_event(sng_fd_t fd, wp_logger_cmd_t *logger_cmd)
  \brief Read Wanpipe Logger Events
  \param fd device file descriptor
  \param logger_cmd Logger API command structure
  \return SANG_STATUS_SUCCESS: ok, else: error
  
  The Logger API structure will be populated with a Logger Event.
  This function usually used after wait() function indicated that
  an event has occured.
*/
sangoma_status_t _LIBSNG_CALL sangoma_logger_read_event(sng_fd_t fd, wp_logger_cmd_t *logger_cmd);

/*!
  \fn sangoma_status_t sangoma_logger_flush_buffers(sng_fd_t fd, wp_logger_cmd_t *logger_cmd)
  \brief Flush Wanpipe Logger internal buffers
  \param fd device file descriptor
  \param logger_cmd Logger API command structure
  \return SANG_STATUS_SUCCESS: ok, else: error
*/
sangoma_status_t _LIBSNG_CALL sangoma_logger_flush_buffers(sng_fd_t fd, wp_logger_cmd_t *logger_cmd);

/*!
  \fn sangoma_status_t sangoma_logger_get_statistics(sng_fd_t fd, wp_logger_cmd_t *logger_cmd)
  \brief Get Wanpipe Logger statistics
  \param fd device file descriptor
  \param logger_cmd Logger API command structure
  \return SANG_STATUS_SUCCESS: ok, else: error
*/
sangoma_status_t _LIBSNG_CALL sangoma_logger_get_statistics(sng_fd_t fd, wp_logger_cmd_t *logger_cmd);

/*!
  \fn sangoma_status_t sangoma_logger_reset_statistics(sng_fd_t fd, wp_logger_cmd_t *logger_cmd)
  \brief Reset Wanpipe Logger statistics
  \param fd device file descriptor
  \param logger_cmd Logger API command structure
  \return SANG_STATUS_SUCCESS: ok, else: error
*/
sangoma_status_t _LIBSNG_CALL sangoma_logger_reset_statistics(sng_fd_t fd, wp_logger_cmd_t *logger_cmd);

/*!
  \fn sangoma_status_t sangoma_logger_get_open_handle_counter(sng_fd_t fd, wp_logger_cmd_t *logger_cmd)
  \brief Get Counter of open Handles/File Descriptors of Wanpipe Logger
  \param fd device file descriptor
  \param logger_cmd Logger API command structure
  \return SANG_STATUS_SUCCESS: ok, else: error
*/
sangoma_status_t _LIBSNG_CALL sangoma_logger_get_open_handle_counter(sng_fd_t fd, wp_logger_cmd_t *logger_cmd);

/*!
  \fn sangoma_status_t sangoma_logger_get_logger_level(sng_fd_t fd, wp_logger_cmd_t *logger_cmd)
  \brief Get current level (types of events) of Wanpipe Logger
  \param fd device file descriptor
  \param logger_cmd Logger API command structure
  \return SANG_STATUS_SUCCESS: ok, else: error
*/
sangoma_status_t _LIBSNG_CALL sangoma_logger_get_logger_level(sng_fd_t fd, wp_logger_cmd_t *logger_cmd);

/*!
  \fn sangoma_status_t sangoma_logger_set_logger_level(sng_fd_t fd, wp_logger_cmd_t *logger_cmd)
  \brief Set current level (types of events) of Wanpipe Logger
  \param fd device file descriptor
  \param logger_cmd Logger API command structure
  \return SANG_STATUS_SUCCESS: ok, else: error
*/
sangoma_status_t _LIBSNG_CALL sangoma_logger_set_logger_level(sng_fd_t fd, wp_logger_cmd_t *logger_cmd);

#endif /* WP LOGGER FEATURE */


#ifdef WP_API_FEATURE_DRIVER_GAIN
/*!
  \fn int sangoma_set_tx_gain(sng_fd_t fd, wanpipe_api_t *tdm_api, floag gain_val)
  \brief set driver tx gain for this tdm device
  \param fd device file descriptor
  \param tdm_api tdm api command structure
  \param value txgain (0.0 - 10.0) db
  \return non-zero = error, 0 = ok
*/ 
int _LIBSNG_CALL sangoma_set_tx_gain(sng_fd_t fd, wanpipe_api_t *tdm_api, float gain_val);

/*!
  \fn int sangoma_set_rx_gain(sng_fd_t fd, wanpipe_api_t *tdm_api, floag gain_val)
  \brief set driver rx gain for this tdm device
  \param fd device file descriptor
  \param tdm_api tdm api command structure
  \param value rxgain (0.0 - 10.0) db
  \return non-zero = error, 0 = ok
*/
int _LIBSNG_CALL sangoma_set_rx_gain(sng_fd_t fd, wanpipe_api_t *tdm_api, float gain_val);

/*!
  \fn int sangoma_clear_tx_gain(sng_fd_t fd, wanpipe_api_t *tdm_api)
  \brief Clear tx gain from device, set it to 0.0
  \param fd device file descriptor
  \param tdm_api tdm api command structure
  \return non-zero = error, 0 = ok
 */
int _LIBSNG_CALL sangoma_clear_tx_gain(sng_fd_t fd, wanpipe_api_t *tdm_api);

/*!
  \fn int sangoma_clear_rx_gain(sng_fd_t fd, wanpipe_api_t *tdm_api)
  \brief Clear rx gain from device, set it to 0.0
  \param fd device file descriptor
  \param tdm_api tdm api command structure
  \return non-zero = error, 0 = ok
 */
int _LIBSNG_CALL sangoma_clear_rx_gain(sng_fd_t fd, wanpipe_api_t *tdm_api);
#endif

#ifndef LIBSANGOMA_LIGHT

/************************************************************//**
 * Device PORT Control Functions
 ***************************************************************/ 

/*!
  \fn int sangoma_driver_port_start(sng_fd_t fd, port_management_struct_t *port_mgmnt, unsigned short port_no)
  \brief Start a Port, create Sangoma Communication interfaces.
  \param[in] fd				Port Device file descriptor
  \param[out] port_mgmnt	pointer to a port_management_struct_t structure.
							On return, sangoma_driver_port_start() updates operation_status field
							of this structure.
  \param[in] port_no		1-based Port Number. Port numbers correspond to Port Names.
							For example, a 2-Port card will have ports named WANPIPE1 and WANPIPE2.
  \return	non-zero:		system error. Call OS specific code to find cause of the error.
							Linux example: strerror(errno)
							Windows example: combination of GetLastError()/FormatMessage()
				zero:		no system error. Check port_mgmt->operation_status.
*/
int _LIBSNG_CALL sangoma_driver_port_start(sng_fd_t fd, port_management_struct_t *port_mgmnt, unsigned short port_no);


/*!
  \fn int sangoma_driver_port_stop(sng_fd_t fd, port_management_struct_t *port_mgmnt, unsigned short port_no)
  \brief Start a Port, create Sangoma Communication interfaces.
  \param[in] fd				Port Device file descriptor
  \param[out] port_mgmnt	pointer to a port_management_struct_t structure.
							On return, sangoma_driver_port_stop() updates operation_status field
							of this structure.
  \param[in] port_no		1-based Port Number. Port numbers correspond to Port Names.
							For example, a 2-Port card will have ports named WANPIPE1 and WANPIPE2.
  \return	non-zero:		system error. Call OS specific code to find cause of the error.
							Linux example: strerror(errno)
							Windows example: combination of GetLastError()/FormatMessage()
				zero:		no system error. Check port_mgmt->operation_status.
*/
int _LIBSNG_CALL sangoma_driver_port_stop(sng_fd_t fd, port_management_struct_t *port_mgmnt, unsigned short port_no);


/*!
  \fn int sangoma_driver_port_set_config(sng_fd_t fd, port_cfg_t *port_cfg, unsigned short port_no)
  \brief Set Port's "Volatile" configuration. The configuration will not persist between system restarts.
			Before calling this function please stop the port by calling sangoma_driver_port_stop().
			After calling this function please start the port by calling sangoma_driver_port_start().
  \param[in] fd				Port Device file descriptor
  \param[in, out] port_cfg	pointer to port_cfg_t structure that specifies complete Port configuration.
							On return, sangoma_driver_port_set_config() updates operation_status field
							of this structure.
  \param[in] port_no		1-based Port Number. Port numbers correspond to Port Names.
							For example, a 2-Port card will have ports named WANPIPE1 and WANPIPE2.
  \return	non-zero:		system error. Call OS specific code to find cause of the error.
							Linux example: strerror(errno)
							Windows example: combination of GetLastError()/FormatMessage()
				zero:		no system error. Check port_cfg->operation_status.
*/
int _LIBSNG_CALL sangoma_driver_port_set_config(sng_fd_t fd, port_cfg_t *port_cfg, unsigned short port_no);


/*!
  \fn int sangoma_driver_port_get_config(sng_fd_t fd, port_cfg_t *port_cfg, unsigned short port_no)
  \brief Retrieve Port's "Volatile" configuration.
  \param[in] fd				Port Device file descriptor
  \param[out]	port_cfg	pointer to port_cfg_t structure. 
							On return, sangoma_driver_port_get_config() will copy current Port configuration
							into this structure.
  \param[in]	port_no		please see comment of sangoma_driver_port_set_config()
  \return	non-zero:		system error. Call OS specific code to find cause of the error.
							Linux example: strerror(errno)
							Windows example: combination of GetLastError()/FormatMessage()
				zero:		no system error. Check port_cfg->operation_status.
*/
int _LIBSNG_CALL sangoma_driver_port_get_config(sng_fd_t fd, port_cfg_t *port_cfg, unsigned short port_no);


/*!
  \fn int sangoma_driver_get_hw_info(sng_fd_t fd, port_management_struct_t *port_mgmnt, unsigned short port_no)
  \brief Retrieve information about a single instance of Sangoma hardware.
  \param[in] fd				Port Device file descriptor
  \param[out]	port_mgmnt	pointer to port_management_struct_t structure which will contain hardware_info_t at
							it's "data" field, when this function returns.
  \param[in]	port_no		please see comment of sangoma_driver_port_set_config()
  \return	non-zero:		system error. Call OS specific code to find cause of the error.
							Linux example: strerror(errno)
							Windows example: combination of GetLastError()/FormatMessage()
				zero:		no system error. Check port_mgmt->operation_status.
*/
int _LIBSNG_CALL sangoma_driver_get_hw_info(sng_fd_t fd, port_management_struct_t *port_mgmnt, unsigned short port_no);


/*!
  \fn int sangoma_driver_get_version(sng_fd_t fd, port_management_struct_t *port_mgmnt, unsigned short port_no)
  \brief Retrieve Driver Version BEFORE any communication interface is configured and sangoma_get_driver_version()
				can not be called.
  \param[in] fd                         Port Device file descriptor
  \param[out]   port_mgmnt      pointer to port_management_struct_t structure which will contain wan_driver_version_t at
                                                        it's "data" field, when this function returns.
  \param[in]    port_no         please see comment of sangoma_driver_port_set_config()
  \return       non-zero:               system error. Call OS specific code to find cause of the error.
                                                        Linux example: strerror(errno)
                                                        Windows example: combination of GetLastError()/FormatMessage()
                                zero:           no system error. Check port_mgmt->operation_status.
*/
int _LIBSNG_CALL sangoma_driver_get_version(sng_fd_t fd, port_management_struct_t *port_mgmnt, unsigned short port_no);




#ifdef  WP_API_FEATURE_HARDWARE_RESCAN
/*!
  \fn int sangoma_driver_hw_rescan(sng_fd_t fd, port_management_struct_t *port_mgmnt, int *detected_port_cnt)
  \brief Rescan the pci and usb bus for newly added hardware
  \param[in]    fd          	Port Device file descriptor
  \param[out]   port_mgmnt      pointer to port_management_struct_t structure which will contain wan_driver_version_t at
                                it's "data" field, when this function returns.
  \param[out]   detected_port_cnt     newly detected ports.
  \return       less than zero:       system error. Call OS specific code to find cause of the error.
                                                        Linux example: strerror(errno)
                                                        Windows example: combination of GetLastError()/FormatMessage()
                zero or greater: number of new ports found.       
*/
int _LIBSNG_CALL sangoma_driver_hw_rescan(sng_fd_t fd, port_management_struct_t *port_mgmnt, int *detected_port_cnt);
#endif

/*!
  \fn int sangoma_write_port_config_on_persistent_storage(hardware_info_t *hardware_info, port_cfg_t *port_cfg)
  \brief Write Port's configuration on the hard disk. 
		Linux Specific: the "Persistent" configuration of a Port N (e.g. WANPIPE1) is stored in
						/etc/wanpipe/wanpipeN.conf (e.g. wanpipe1.conf).
						Configuration can be manualy viewed/changed by editing the ".conf" file.
						Currently this functionality is not implemented.
		Windows Specific: the "Persistent" configuration of a Port (e.g. WANPIPE1) is stored in
						Windows Registry.
						Configuration can be manualy viewed/changed in the Device Manager.
  \param[in] hardware_info	pointer to hardware_info_t structure containing information about a
							single instance of Sangoma hardware.
  \param[in] port_cfg		pointer to structure containing complete Port configuration.
  \param[in] port_no	please see comment of sangoma_driver_port_set_config()
  \return non-zero: error, 0: ok
*/
int _LIBSNG_CALL sangoma_write_port_config_on_persistent_storage(hardware_info_t *hardware_info, port_cfg_t *port_cfg, unsigned short port_no);


/************************************************************//**
 * Device MANAGEMENT Functions
 ***************************************************************/ 

/*!
  \fn int sangoma_mgmt_cmd(sng_fd_t fd, wan_udp_hdr_t* wan_udp)
  \brief Execute Sangoma Management Command
  \param fd device file descriptor
  \param wan_udp management command structure
  \return non-zero: error, 0: ok
*/
int _LIBSNG_CALL sangoma_mgmt_cmd(sng_fd_t fd, wan_udp_hdr_t* wan_udp);


#endif  /* LIBSANGOMA_LIGHT */


/*================================================================
 * DEPRECATED Function Calls - Not to be used any more
 * Here for backward compatibility 
 *================================================================*/


#ifndef LIBSANGOMA_SET_FESTATUS
/*!
  \def LIBSANGOMA_SET_FESTATUS
  \brief Set Front End Status Feature 
*/
#define LIBSANGOMA_SET_FESTATUS 1
#endif

/*!
  \fn int sangoma_get_fe_status(sng_fd_t fd, wanpipe_api_t *tdm_api, unsigned char *current_status)
  \brief Get Device Link Status (Connected/Disconnected)
  \param fd device file descriptor
  \param tdm_api tdm api command structure
  \param current_status pointer where result will be filled: 0=Link UP 1=Link Down
  \return non-zero: error, 0: ok -> check current_status 

  Deprecated - replaced by sangoma_tdm_get_link_status function 
*/
int _LIBSNG_CALL sangoma_get_fe_status(sng_fd_t fd, wanpipe_api_t *tdm_api, unsigned char *current_status);



/*!
  \fn int sangoma_tdm_set_codec(sng_fd_t fd, wanpipe_api_t *tdm_api, int codec)
  \brief Set TDM Codec per chan
  \param fd device file descriptor
  \param tdm_api tdm api command structure
  \param codec codec to set (ulaw/alaw/slinear)
  \return non-zero: error, 0: ok
  
  Deprecated Function - Here for backward compatibility
  Only valid in CHAN Operation Mode
*/
int _LIBSNG_CALL sangoma_tdm_set_codec(sng_fd_t fd, wanpipe_api_t *tdm_api, int codec);

/*!
  \fn int sangoma_tdm_get_codec(sng_fd_t fd, wanpipe_api_t *tdm_api)
  \brief Get Configured TDM Codec per chan
  \param fd device file descriptor
  \param tdm_api tdm api command structure
  \return negative: error or configured codec value
  
  Deprecated Function - Here for backward compatibility
  Only valid in CHAN Operation Mode
*/
int _LIBSNG_CALL sangoma_tdm_get_codec(sng_fd_t fd, wanpipe_api_t *tdm_api);


/*!
  \fn sng_fd_t sangoma_create_socket_by_name(char *device, char *card)
  \brief Open a device based on a interface and card name
  \param device interface name
  \param card card name
  \return File Descriptor: -1 error, 0 or positive integer: valid file descriptor

   Deprecated - here for backward compatibility
*/
sng_fd_t _LIBSNG_CALL sangoma_create_socket_by_name(char *device, char *card);

/*!
  \fn int sangoma_interface_toi(char *interface_name, int *span, int *chan)
  \brief Convert Span & Chan to interface name
  \param interface_name pointer to string where interface name will be written
  \param span span number starting from 1 to 255
  \param chan chan number starting from 1 to 32
  \return non-zero = error, 0 = ok
  Deprecated - here for backward compatibility
*/
int _LIBSNG_CALL sangoma_interface_toi(char *interface_name, int *span, int *chan);


/*!
  \fn int sangoma_tdm_set_power_level(sng_fd_t fd, wanpipe_api_t *tdm_api, int power)
  \brief Set Power Level - so only data matching the power level would be passed up.
  \param fd device file descriptor
  \param tdm_api tdm api command structure
  \param power value of power 
  \return non-zero: error, 0: ok
  
  Deprecated - not used/implemented
*/
int _LIBSNG_CALL sangoma_tdm_set_power_level(sng_fd_t fd, wanpipe_api_t *tdm_api, int power);

/*!
  \fn int sangoma_tdm_get_power_level(sng_fd_t fd, wanpipe_api_t *tdm_api)
  \brief Get Configured Power Level
  \param fd device file descriptor
  \param tdm_api tdm api command structure
  \return negative: error or configured power level
  
  Deprecated - not used/implemented
*/
int _LIBSNG_CALL sangoma_tdm_get_power_level(sng_fd_t fd, wanpipe_api_t *tdm_api);




#ifdef WP_API_FEATURE_LIBSNG_HWEC

/*!
  \fn sangoma_status_t _LIBSNG_CALL sangoma_hwec_config_init(char *device_name, wan_custom_param_t custom_params[], unsigned int number_of_custom_params)

  \brief Load Firmware image onto EC chip and allocated per-port resources in HWEC API.
			All chip-wide configuration paramters, if any, must be specified at the time of chip initialization.
			  Note that Analog card is considered a "single-port" card by HWEC API. That means for Analog cards and
			for single-port digital cards only a single sangoma_hwec_config_init() call is required, all subsequent
			calls will have no effect.
			  For multi-port cards, such as A102/A104/A108/A500, the sangoma_hwec_config_init() must be called
			for each port, at least one time. Only the first call will actually load the Firmware image onto 
			EC chip, all subsequent calls (for other ports) will only add the Port to list of ports which use
			the HWEC API.
			  Actions of sangoma_hwec_config_init() can be reversed by calling sangoma_hwec_config_release().
			When Port is stopped, the HWEC API automatically releases per-port resources and removes the Port
			from list ports which use HWEC API.

  \param device_name Sangoma API device name. 
		Windows: wanpipe1_if1, wanpipe2_if1... Note that wanpipe1_if1 and wanpipe1_if2 will access the same Port - wanpipe1.
		Linux: wanpipe1, wanpipe2...

  \param custom_params[] - (optional) array of custom paramter structures.

		This is list of Echo Cancellation chip parameters:

		Chip parameter					Chip parameter value
		=================				=======================
		WANEC_TailDisplacement			0-896
		WANEC_MaxPlayoutBuffers			0-4678
		WANEC_EnableExtToneDetection	TRUE | FALSE
		WANEC_EnableAcousticEcho		TRUE | FALSE

  \param number_of_custom_params - (optional) number of structures in custom_params[]. Minimum value is 1, maximum is 4,
		if any other value the custom_params[] will be ignored.

  \return SANG_STATUS_SUCCESS: success, or error status
*/
sangoma_status_t _LIBSNG_CALL sangoma_hwec_config_init(char *device_name, wan_custom_param_t custom_params[], unsigned int number_of_custom_params);

/*!
  \fn sangoma_status_t _LIBSNG_CALL sangoma_hwec_config_release(char *device_name)

  \brief Release resources allocated by sangoma_hwec_config_init().
			  For single-port cards, such as A101 and A200 (A200 is an Analog card and considered
			sinle-port by HWEC API), a single call to sangoma_hwec_config_release()	will free the per-chip resources.
			  For multi-port cards, such as A102/A104/A108/A500, sangoma_hwec_config_release() can be called
			for each port to remove it from list Ports which are using HWEC API. When sangoma_hwec_config_release()
			is called for the last Port which was "configured/initialized by HWEC API", the per-chip resources will be freed.

  \param device_name Sangoma API device name. 
		Windows: wanpipe1_if1, wanpipe2_if1... Note that wanpipe1_if1 and wanpipe1_if2 will access the same Port - wanpipe1.
		Linux: wanpipe1, wanpipe2...

  \return SANG_STATUS_SUCCESS: success, or error status
*/
sangoma_status_t _LIBSNG_CALL sangoma_hwec_config_release(char *device_name);

/*!
  \fn sangoma_status_t _LIBSNG_CALL sangoma_hwec_config_operation_mode(char *device_name, int mode, unsigned int fe_chan_map)

  \brief	Modify channel operation mode.

  \param	mode	One of WANEC_API_OPMODE_? values defined in wanpipe_api_iface.h:
					WANEC_API_OPMODE_NORMAL,
					WANEC_API_OPMODE_HT_FREEZE,
					WANEC_API_OPMODE_HT_RESET, 
					WANEC_API_OPMODE_POWER_DOWN,
					WANEC_API_OPMODE_NO_ECHO,
					WANEC_API_OPMODE_SPEECH_RECOGNITION.

  \param fe_chan_map Bitmap of channels (timeslots for Digital,
				lines for Analog) where the call will take effect.

  \return SANG_STATUS_SUCCESS: success, or error status
*/
sangoma_status_t _LIBSNG_CALL sangoma_hwec_config_operation_mode(char *device_name, int mode, unsigned int fe_chan_map);

/*!
  \fn sangoma_status_t _LIBSNG_CALL sangoma_hwec_config_power_on (char *device_name,  unsigned int fe_chan_map)

  \brief Set the channel state in the echo canceller to NORMAL/POWER ON.
		 This enables echo cancelation logic inside the chip. 
         The action is internal to EC chip itself, not related to AFT FPGA.
		 This call is slow and should be used only on startup.

  \param device_name Sangoma API device name. 
		Windows: wanpipe1_if1, wanpipe2_if1...
		Linux: wanpipe1, wanpipe2...

  \param fe_chan_map Bitmap of channels (timeslots for Digital,
				lines for Analog) where the call will take effect.

  \return SANG_STATUS_SUCCESS: success, or error status
*/
sangoma_status_t _LIBSNG_CALL sangoma_hwec_config_power_on(char *device_name,  unsigned int fe_chan_map);

/*!
  \fn sangoma_status_t _LIBSNG_CALL sangoma_hwec_config_power_off (char *device_name,  unsigned int fe_chan_map)

  \brief  Set the channel state in the echo canceller to POWER OFF.
          This disables echo cancellatio logic inside the chip and
          data passes unmodified through the ec chip.
          The action is internal to EC chip itself, not related
		  to AFT FPGA. This call is slow and should be used only on startup.

  \param device_name Sangoma API device name. 
		Windows: wanpipe1_if1, wanpipe2_if1...
		Linux: wanpipe1, wanpipe2...

  \param fe_chan_map Bitmap of channels (timeslots for Digital,
				lines for Analog) where the call will take effect.

  \return SANG_STATUS_SUCCESS: success, or error status
*/
sangoma_status_t _LIBSNG_CALL sangoma_hwec_config_power_off(char *device_name,  unsigned int fe_chan_map);

/*!
  \fn sangoma_status_t _LIBSNG_CALL sangoma_hwec_enable(char *device_name,  unsigned int fe_chan_map)

  \brief Redirect audio stream from AFT FPGA to EC chip. 
        This command effectively enables echo cancellation since
		data is now forced through the EC chip by the FPGA. 
		Data will be modified by the echo canceller.
		This command is recommened for fast enabling of Echo Cancellation.
        Note 1: Chip must be configured and in POWER ON state for echo
				Chancellation to take place.
		Note 2: sangoma_tdm_enable_hwec() function can be use to achive
				the same funcitnality based on file descriptor versus
				channel map.

  \param device_name Sangoma API device name. 
		Windows: wanpipe1_if1, wanpipe2_if1...
		Linux: wanpipe1, wanpipe2...

  \param fe_chan_map Bitmap of channels (timeslots for Digital, lines for Analog) where 
		the call will take effect.

  \return SANG_STATUS_SUCCESS: success, or error status
*/
sangoma_status_t _LIBSNG_CALL sangoma_hwec_enable(char *device_name,  unsigned int fe_chan_map);

/*!
  \fn sangoma_status_t _LIBSNG_CALL sangoma_hwec_disable(char *device_name, unsigned int fe_chan_map)

  \brief Force AFT FPGA to bypass the echo canceller.  
         This command effectively disables echo cancellation since
		 data will not be flowing through the ec chip. 
         Data will not be modified by the echo canceller.
         This command is recommened for fast disabling of Echo Cancelation.
		 Note: sangoma_tdm_disable_hwec() function can be used to achive
				the same functionality based on file descriptor versus
				channel map.

  \param device_name Sangoma API device name. 
		Windows: wanpipe1_if1, wanpipe2_if1...
		Linux: wanpipe1, wanpipe2...

  \param fe_chan_map Bitmap of channels (timeslots for Digital, lines for Analog) where 
		the call will take effect.

  \return SANG_STATUS_SUCCESS: success, or error status
*/
sangoma_status_t _LIBSNG_CALL sangoma_hwec_disable(char *device_name, unsigned int fe_chan_map);

/*!
  \fn sangoma_status_t _LIBSNG_CALL sangoma_hwec_config_channel_parameter(char *device_name, char *parameter, char *parameter_value, unsigned int channel_map)

  \brief Modify channel configuration parameters.

  \param device_name Sangoma API device name. 
		Windows: wanpipe1_if1, wanpipe2_if1...
		Linux: wanpipe1, wanpipe2...

  \param parameter Echo Cancellation channel parameter

	This is list of Echo Cancellation channel parameters:

		Channel parameter					Channel parameter value
		=================					=======================
		WANEC_EnableNlp						TRUE | FALSE
		WANEC_EnableTailDisplacement		TRUE | FALSE
		WANEC_TailDisplacement				0-896
		WANEC_SoutLevelControl				TRUE | FALSE
		WANEC_RinAutomaticLevelControl		TRUE | FALSE
		WANEC_SoutAutomaticLevelControl		TRUE | FALSE
		WANEC_SoutAdaptiveNoiseReduction	TRUE | FALSE
		WANEC_RoutNoiseReduction			TRUE | FALSE
		WANEC_ComfortNoiseMode				COMFORT_NOISE_NORMAL
											COMFORT_NOISE_FAST_LATCH
											COMFORT_NOISE_EXTENDED
											COMFORT_NOISE_OFF
		WANEC_DtmfToneRemoval				TRUE | FALSE
		WANEC_AcousticEcho					TRUE | FALSE
		WANEC_NonLinearityBehaviorA			0-13
		WANEC_NonLinearityBehaviorB			0-8
		WANEC_DoubleTalkBehavior			DT_BEH_NORMAL
											DT_BEH_LESS_AGGRESSIVE
		WANEC_AecTailLength					128 (default), 256, 512 or 1024 
		WANEC_EnableToneDisabler			TRUE | FALSE

  \param fe_chan_map Bitmap of channels (timeslots for Digital, lines for Analog) where 
		the call will take effect.

  \return SANG_STATUS_SUCCESS: success, or error status
*/
sangoma_status_t _LIBSNG_CALL sangoma_hwec_config_channel_parameter(char *device_name, char *parameter, char *parameter_value, unsigned int channel_map);

/*!
  \fn sangoma_status_t _LIBSNG_CALL sangoma_hwec_config_tone_detection(char *device_name, int tone_id, int enable, unsigned int fe_chan_map, unsigned char port_map)
  
  \brief Enable/Disable tone detection (such as DTMF) of channels from channel map. 

  \param device_name Sangoma API device name. 
		Windows: wanpipe1_if1, wanpipe2_if1...
		Linux: wanpipe1, wanpipe2...

  \param tone_id See wanpipe_api_iface.h for list of valid tones

  \param enable A flag, if 1 - the specified tone will be detected,
				if 0 - specified tone will not be detected.

  \param fe_chan_map Bitmap of channels (timeslots for Digital, lines for Analog) where 
		the call will take effect.

  \param port_map Port\Direction of tone detection - Rx, Tx. See wanpipe_events.h for
			list of valid ports (WAN_EC_CHANNEL_PORT_SOUT...).

  \return SANG_STATUS_SUCCESS: success, or error status
*/
sangoma_status_t _LIBSNG_CALL sangoma_hwec_config_tone_detection(char *device_name, int tone_id, int enable, unsigned int fe_chan_map, unsigned char port_map);

/*!
  \fn sangoma_status_t _LIBSNG_CALL sangoma_hwec_print_statistics(char *device_name, int full, unsigned int fe_chan_map)

  \brief Read and print Chip/Channel statistics from EC chip. 

  \param device_name Sangoma API device name. 
		Windows: wanpipe1_if1, wanpipe2_if1...
		Linux: wanpipe1, wanpipe2...

  \param full Flag to read full statistics, if set to 1.

  \param fe_chan_map Bitmap of channels (timeslots for Digital, lines for Analog) where 
		the call will read statistics.

  \return SANG_STATUS_SUCCESS: success, or error status
*/
sangoma_status_t _LIBSNG_CALL sangoma_hwec_print_statistics(char *device_name, int full, unsigned int fe_chan_map);

/*!
  \fn sangoma_status_t _LIBSNG_CALL sangoma_hwec_audio_buffer_load(char *device_name, char *filename, char pcmlaw, int *out_buffer_id)

  \brief Load audio buffer to EC chip. The buffer can be played out using the sangoma_hwec_audio_buffer_playout() function. 

  \param filename name of the audio file (without the extension). 
				Actual file must have .pcm extension.
				Location:
					Windows: %SystemRoot%\sang_ec_files (ex: c:\WINDOWS\sang_ec_files)
				Linux: /etc/wanpipe/buffers

 \param out_buffer_id when the buffer is loaded on the chip, it is assigned an ID. This ID should
			be used when requesting to play out the buffer.
 
  \return SANG_STATUS_SUCCESS: success, or error status

 */
sangoma_status_t _LIBSNG_CALL sangoma_hwec_audio_buffer_load(char *device_name, char *filename, char pcmlaw, int *out_buffer_id);

/*!
  \fn sangoma_status_t _LIBSNG_CALL sangoma_hwec_audio_buffer_load(char *device_name, char *filename, char pcmlaw, int *out_buffer_id)

  \brief Load audio buffer to EC chip. The buffer can be played out using the sangoma_hwec_audio_buffer_playout() function. 

  \param buffer Pointer to in memory buffer to be loaded on the chip.

  \param size Size of buffer.

  \param out_buffer_id when the buffer is loaded on the chip, it is assigned an ID. This ID should
		be used when requesting to play out the buffer.

  \return SANG_STATUS_SUCCESS: success, or error status
*/
 sangoma_status_t _LIBSNG_CALL sangoma_hwec_audio_mem_buffer_load(char *device_name, unsigned char *buffer, unsigned int in_size, char pcmlaw, int *out_buffer_id);

/*!
  \fn sangoma_status_t _LIBSNG_CALL sangoma_hwec_audio_bufferunload(char *device_name, int in_buffer_id)

  \brief Unload/remove an audio buffer from the HWEC chip.

  \param device_name Sangoma wanpipe device name. (ex: wanpipe1 - Linux; wanpipe1_if1 - Windows).

  \param in_buffer_id	ID of the buffer which will be unloaded. The ID must be initialized by sangoma_hwec_audio_bufferload().

  \return SANG_STATUS_SUCCESS - buffer was successfully unloaded/removed, SANG_STATUS_GENERAL_ERROR - error occured
*/
sangoma_status_t _LIBSNG_CALL sangoma_hwec_audio_buffer_unload(char *device_name, int in_buffer_id);

/*!
  \fn sangoma_status_t _LIBSNG_CALL sangoma_hwec_audio_buffer_playout(char *device_name, unsigned int fe_chan_map, 
										unsigned char port_map, int buffer_id, int start, int repeat_cnt, int duration)

  \brief Start\Stop playing out an audio buffer previously loaded by sangoma_hwec_audio_buffer_load().

  \param fe_chan_map Bitmap of channels (timeslots for Digital,
				lines for Analog) where the call will take effect.

  \param port_map Port\Direction where the buffer will be played out.
					This is the channel port on which the buffer will be
					played (WAN_EC_CHANNEL_PORT_SOUT or WAN_EC_CHANNEL_PORT_ROUT)
  
  \param in_buffer_id	ID of the buffer which will be unloaded. The ID must be initialized by sangoma_hwec_audio_bufferload().
  
  \param start	If 1 - start the play out, 0 - stop the play out
		
  \param repeat_cnt Number of times to play out the same buffer
  
  \param duration	Maximum duration of the playout, in milliseconds. If it takes less then 'duration' to
					play out the whole buffer, it will be repeated to fill 'duration' amount of time.
					If 'duration' is set to non-zero, this parameter overrides the repeat_cnt flag.

  \return SANG_STATUS_SUCCESS: success, or error status
*/
sangoma_status_t _LIBSNG_CALL sangoma_hwec_audio_buffer_playout(char *device_name, unsigned int fe_chan_map, 
																unsigned char port, int in_buffer_id, int start,
																int repeat_cnt,	int duration);

/*!
  \fn void _LIBSNG_CALL sangoma_hwec_config_verbosity(int verbosity_level)

  \brief Set Verbosity level of EC API. The level controls amount of data 
			printed to stdout and wanpipelog.txt for diagnostic purposes.

  \param verbosity_level Valid values are from 0 to 3.

  \return	SANG_STATUS_SUCCESS: success - the level was changed to 'verbosity_level', 
			SANG_STATUS_INVALID_PARAMETER: error - the level was not changed because new level is invalid
*/
sangoma_status_t _LIBSNG_CALL sangoma_hwec_config_verbosity(int verbosity_level);

/*!
  \fn void _LIBSNG_CALL sangoma_hwec_initialize_custom_parameter_structure(wan_custom_param_t *custom_param, char *parameter_name, char *parameter_value)

  \brief Initialize Custom Paramter structure.

  \param parameter_name  Parameter Name

  \param parameter_value Parameter Value

  \return None
*/
void _LIBSNG_CALL sangoma_hwec_initialize_custom_parameter_structure(wan_custom_param_t *custom_param, char *parameter_name, char *parameter_value);


/*!
  \fn sangoma_status_t _LIBSNG_CALL sangoma_hwec_get_channel_statistics(sng_fd_t fd, unsigned int fe_chan, 
								int *hwec_api_return_code, wanec_chan_stats_t *wanec_chan_stats, int reset)

  \brief Get Channel statistics from EC chip. 

  \param fd device file descriptor

  \param fe_chan Channel number (a timeslot for Digital, a line for Analog) where 
		the call will read statistics. Valid values are from 1 to 31.

  \param hwec_api_return_code	will contain one of WAN_EC_API_RC_x codes which are defined in wanec_iface_api.h

  \param wanec_chip_stats	structure will be filled with HWEC channel statistics.

  \param verbose Flag indicating to the Driver to print additional information about the command into Wanpipe Log file.

  \param reset Flag to reset (flush) channel statistics to zero, if set to 1.

  \return SANG_STATUS_SUCCESS: success, or error status of IOCTL
*/
sangoma_status_t _LIBSNG_CALL sangoma_hwec_get_channel_statistics(sng_fd_t fd, unsigned int fe_chan, 
				int *hwec_api_return_code, wanec_chan_stats_t *wanec_chan_stats, int verbose, int reset);


/*!
  \fn sangoma_status_t _LIBSNG_CALL sangoma_hwec_get_global_chip_statistics(sng_fd_t fd,
				int *hwec_api_return_code, wanec_chip_stats_t *wanec_chip_stats, int verbose, int reset)

  \brief Get Global statistics from EC chip. 

  \param fd device file descriptor

  \param hwec_api_return_code	will contain one of WAN_EC_API_RC_x codes which are defined in wanec_iface_api.h

  \param wanec_chip_stats	structure will be filled with HWEC channel statistics.

  \param verbose Flag indicating to the Driver to print additional information about the command into Wanpipe Log file.

  \param reset Flag to reset (flush) global statistics to zero, if set to 1.

  \return SANG_STATUS_SUCCESS: success, or error status of IOCTL
*/
sangoma_status_t _LIBSNG_CALL sangoma_hwec_get_global_chip_statistics(sng_fd_t fd,
			int *hwec_api_return_code, wanec_chip_stats_t *wanec_chip_stats, int verbose, int reset);


/*!
  \fn sangoma_status_t _LIBSNG_CALL sangoma_hwec_get_chip_image_info(sng_fd_t fd,
						int *hwec_api_return_code, wanec_chip_image_t *wanec_chip_image, int verbose)

  \brief Get information about Firmware Image of EC chip. 

  \param fd device file descriptor

  \param hwec_api_return_code	will contain one of WAN_EC_API_RC_x codes which are defined in wanec_iface_api.h

  \param wanec_chip_image_t		structure will be filled with HWEC channel statistics. 

  \param verbose Flag indicating to the Driver to print additional information about the command into Wanpipe Log file.

  \return SANG_STATUS_SUCCESS: success, or error status of IOCTL
*/
sangoma_status_t _LIBSNG_CALL sangoma_hwec_get_chip_image_info(sng_fd_t fd,
			int *hwec_api_return_code, wanec_chip_image_t *wanec_chip_image, int verbose);

#ifdef WP_API_FEATURE_LIBSNG_HWEC_DTMF_REMOVAL
/*!
  \fn sangoma_status_t _LIBSNG_CALL sangoma_hwec_set_hwdtmf_removal(sng_fd_t fd, unsigned int fe_chan,
		int *hwec_api_return_code, int enable)

  \brief Enable/Disable HW DTMF removal. 

  \param fd device file descriptor

  \param fe_chan Channel number (a timeslot for Digital, a line for Analog) where 
		the call will read statistics. Valid values are from 1 to 31.

  \param hwec_api_return_code	will contain one of WAN_EC_API_RC_x codes which are defined in wanec_iface_api.h

  \param verbose Flag indicating to the Driver to print additional information about the command into Wanpipe Log file.

  \param enable Flag to remove DTMF tones, if set to 1.

  \return SANG_STATUS_SUCCESS: success, or error status of IOCTL
 */
sangoma_status_t _LIBSNG_CALL sangoma_hwec_set_hwdtmf_removal(sng_fd_t fd, unsigned int fe_chan,
		int *hwec_api_return_code, int enable, int verbose);
#endif /* WP_API_FEATURE_LIBSNG_HWEC */
#endif



#ifdef WP_API_FEATURE_SS7_FORCE_RX
/*!
  \fn int _LIBSNG_CALL sangoma_ss7_force_rx(sng_fd_t fd, wanpipe_api_t *tdm_api)
  \brief Force the firmware to pass up a repeating frame
  \param fd device file descriptor
  \param tdm_api tdm api command structure
  \return SANG_STATUS_SUCCESS: success, or error status of IOCTL
*/
int _LIBSNG_CALL sangoma_ss7_force_rx(sng_fd_t fd, wanpipe_api_t *tdm_api);
#endif

#ifdef WP_API_FEATURE_SS7_CFG_STATUS
/*!
  \fn int _LIBSNG_CALL sangoma_ss7_get_cfg_status(sng_fd_t fd, wanpipe_api_t *tdm_api, wan_api_ss7_cfg_status_t *ss7_cfg_status)
  \brief Get current ss7 hw configuration
  \param fd device file descriptor
  \param tdm_api tdm api command structure
  \param ss7_cfg_status ss7 configuration status structure
  \return SANG_STATUS_SUCCESS: success, or error status of IOCTL
*/
int _LIBSNG_CALL sangoma_ss7_get_cfg_status(sng_fd_t fd, wanpipe_api_t *tdm_api, wan_api_ss7_cfg_status_t *ss7_cfg_status);
#endif


#ifdef __cplusplus
}
#endif/* __WINDOWS__ */


#if defined(__WINDOWS__)
/*! Windows specific API */
#ifdef __cplusplus
extern "C" {	/* for C++ users */
#endif

sangoma_status_t _LIBSNG_CALL sangoma_unload_driver();
sangoma_status_t _LIBSNG_CALL sangoma_load_driver();
void _LIBSNG_CALL sangoma_reset_port_numbers();
sangoma_status_t _LIBSNG_CALL sangoma_get_driver_version_from_registry(char *out_buffer, int out_buffer_length);
sangoma_status_t _LIBSNG_CALL sangoma_set_driver_mode_of_all_hw_devices(int driver_mode);

#ifdef __cplusplus
}
#endif/* __WINDOWS__ */

#else
/*! Backward compabile defines */
#define sangoma_open_tdmapi_span_chan sangoma_open_api_span_chan
#define sangoma_open_tdmapi_span sangoma_open_api_span
#define sangoma_open_tdmapi_ctrl sangoma_open_api_ctrl
#define sangoma_tdm_get_fe_status sangoma_get_fe_status
#define sangoma_socket_close sangoma_close
#define sangoma_tdm_get_hw_coding sangoma_get_hw_coding
#define sangoma_tdm_set_fe_status sangoma_set_fe_status
#define sangoma_tdm_get_link_status sangoma_get_link_status
#define sangoma_tdm_flush_bufs sangoma_flush_bufs
#define sangoma_tdm_cmd_exec sangoma_cmd_exec
#define sangoma_tdm_read_event sangoma_read_event
#define sangoma_readmsg_tdm sangoma_readmsg
#define sangoma_readmsg_socket sangoma_readmsg
#define sangoma_sendmsg_socket sangoma_writemsg
#define sangoma_writemsg_tdm sangoma_writemsg
#define sangoma_create_socket_intr sangoma_open_api_span_chan
#endif

#endif	/* _LIBSANGOMA_H */

