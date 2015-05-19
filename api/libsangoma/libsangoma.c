/*******************************************************************************//**
 * \file libsangoma.c
 * \brief Wanpipe API Code Library for Sangoma AFT T1/E1/Analog/BRI/Serial hardware
 *
 * Author(s):	Nenad Corbic <ncorbic@sangoma.com>
 *              David Rokhvarg <davidr@sangoma.com>
 *              Michael Jerris <mike@jerris.com>
 * 				Anthony Minessale II <anthmct@yahoo.com>
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
 *******************************************************************************
 */

#include "libsangoma-pvt.h"

#if defined(__WINDOWS__)
# include <setupapi.h>	/* SetupDiXXX() functions */
# include <initguid.h>	/* GUID instantination */
# include <devguid.h>	/* DEFINE_GUID() */
# include "public.h"	/* GUID_DEVCLASS_SANGOMA_ADAPTER */

# define MAX_COMP_INSTID	2096
# define MAX_COMP_DESC		2096
# define MAX_FRIENDLY		2096	
# define TMP_BUFFER_LEN		256

/*!+! jpboily used to tell get_out_flags no objects were signaled */
# define INVALID_INDEX (uint32_t) -1 
# define WP_ALL_BITS_SET ((uint32_t)-1)
#endif

int libsng_dbg_level = 0;

/*********************************************************************//**
 * WINDOWS Only Section
 *************************************************************************/

#if defined(__WINDOWS__)
#define	DBG_REGISTRY	if(libsng_dbg_level)libsng_dbg

/*
  \fn static void DecodeLastError(LPSTR lpszFunction)
  \brief Decodes the Error in radable format.
  \param lpszFunction error string

  Private Windows Only Function
 */
static void LibSangomaDecodeLastError(LPSTR lpszFunction) 
{ 
	LPVOID lpMsgBuf;
	DWORD dwLastErr = GetLastError();
	FormatMessage( 
		FORMAT_MESSAGE_ALLOCATE_BUFFER | 
		FORMAT_MESSAGE_FROM_SYSTEM | 
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		dwLastErr,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), /* Default language */
		(LPTSTR) &lpMsgBuf,
		0,
		NULL 
	);
	/* Display the string. */
	DBG_POLL("Last Error in %s(): %s (%d)\n", lpszFunction, (char*)lpMsgBuf, dwLastErr);
	/* Free the buffer. */
	LocalFree( lpMsgBuf );
} 

/*
  \fn static int handle_device_ioctl_result(int bResult)
  \brief Checks result code of ioctl
  \param bResult result of ioctl call

  Private Windows Only Function
 */
static u16 handle_device_ioctl_result(int bResult, char *caller_name)
{
	if(bResult == 0){
		/*error*/
		LibSangomaDecodeLastError(caller_name);
		return SANG_STATUS_IO_ERROR;
	}else{
		return SANG_STATUS_SUCCESS;
	}
}

/*
  \fn static int UdpManagementCommand(sng_fd_t fd, wan_udp_hdr_t* wan_udp)
  \brief Executes Driver Management Command
  \param fd device file descriptor
  \param wan_udp managemet cmd structure

  Private Windows Function
 */
int UdpManagementCommand(sng_fd_t fd, wan_udp_hdr_t* wan_udp)
{
	DWORD ln, bIoResult;
	unsigned char id = 0;

	wan_udp->wan_udphdr_request_reply = 0x01;
	wan_udp->wan_udphdr_id = id;
 	wan_udp->wan_udphdr_return_code = WAN_UDP_TIMEOUT_CMD;

	bIoResult = DeviceIoControl(
			fd,
			IoctlManagementCommand,
			(LPVOID)wan_udp,
			sizeof(wan_udp_hdr_t),
			(LPVOID)wan_udp,
			sizeof(wan_udp_hdr_t),
			(LPDWORD)(&ln),
			(LPOVERLAPPED)NULL
			);

	return handle_device_ioctl_result(bIoResult, __FUNCTION__);
}

/*
  \fn static int tdmv_api_ioctl(sng_fd_t fd, wanpipe_tdm_api_cmd_t *api_cmd)
  \brief Executes Driver TDM API Command Wrapper Function
  \param fd device file descriptor
  \param api_cmd tdm_api managemet cmd structure

  Private Windows Function
 */
static int tdmv_api_ioctl(sng_fd_t fd, wanpipe_tdm_api_cmd_t *api_cmd)
{
	DWORD ln, bIoResult;

	bIoResult = DeviceIoControl(
			fd,
			IoctlTdmApiCommand,
			(LPVOID)api_cmd,
			sizeof(wanpipe_tdm_api_cmd_t),
			(LPVOID)api_cmd,
			sizeof(wanpipe_tdm_api_cmd_t),
			(LPDWORD)(&ln),
			(LPOVERLAPPED)NULL
			);

	return handle_device_ioctl_result(bIoResult, __FUNCTION__);
}

/*!
  \fn static USHORT DoReadCommand(sng_fd_t fd, wan_iovec_list_t *p_iovec_list)
  \brief  API READ Function
  \param drv device file descriptor
  \param p_iovec_list a list of memory descriptors

  Private Function.
  A non-blocking function.
 */
static USHORT DoReadCommand(sng_fd_t fd, wan_iovec_list_t *p_iovec_list)
{
	DWORD ln, bIoResult;

	bIoResult = DeviceIoControl(
			fd,
			IoctlReadCommandNonBlocking,
			(LPVOID)NULL,	/*NO input buffer!*/
			0,
			(LPVOID)p_iovec_list,
			sizeof(wan_iovec_list_t),
			(LPDWORD)(&ln),
			(LPOVERLAPPED)NULL);

	return handle_device_ioctl_result(bIoResult, __FUNCTION__);
}

/*!
  \fn static UCHAR DoWriteCommand(sng_fd_t fd, wan_iovec_list_t *p_iovec_list)
  \brief API Write Function
  \param drv device file descriptor
  \param p_iovec_list a list of memory descriptors

  Private Function.
  A non-blocking function.
 */
static UCHAR DoWriteCommand(sng_fd_t fd, wan_iovec_list_t *p_iovec_list)
{
	DWORD BytesReturned, bIoResult;

	bIoResult = DeviceIoControl(
			fd,
			IoctlWriteCommandNonBlocking,
			(LPVOID)p_iovec_list,	//input buffer
			sizeof(*p_iovec_list),	//size of input buffer
			(LPVOID)p_iovec_list,	//output buffer
			sizeof(*p_iovec_list),	//size of output buffer
			(LPDWORD)(&BytesReturned),
			(LPOVERLAPPED)NULL);

	return (UCHAR)handle_device_ioctl_result(bIoResult, __FUNCTION__);	
}

/*
  \fn static USHORT sangoma_poll_fd(sng_fd_t fd, API_POLL_STRUCT *api_poll_ptr)
  \brief Non Blocking API Poll function used to find out if Rx Data, Events or
			Free Tx buffer available
  \param drv device file descriptor
  \param api_poll_ptr poll device that stores polling information read/write/event
  \param overlapped pointer to system overlapped io structure.

  Private Windows Function
 */
static USHORT sangoma_poll_fd(sng_fd_t fd, API_POLL_STRUCT *api_poll_ptr)
{
	DWORD ln, bIoResult;

	bIoResult = DeviceIoControl(
			fd,
			IoctlApiPoll,
			(LPVOID)NULL,
			0L,
			(LPVOID)api_poll_ptr,
			sizeof(API_POLL_STRUCT),
			(LPDWORD)(&ln),
			(LPOVERLAPPED)NULL);

	return handle_device_ioctl_result(bIoResult, __FUNCTION__);
}

/*
  \fn static int logger_api_ioctl(sng_fd_t fd, wp_logger_cmd_t *logger_cmd)
  \brief Executes Logger API IOCTL
  \param fd device file descriptor
  \param logger_cmd Logger API command structure

  Private Windows Function
 */
static sangoma_status_t logger_api_ioctl(sng_fd_t fd, wp_logger_cmd_t *logger_cmd)
{
	DWORD ln, bIoResult;

	bIoResult = DeviceIoControl(
			fd,
			IoctlLoggerApiCommand,
			(LPVOID)logger_cmd,
			sizeof(wp_logger_cmd_t),
			(LPVOID)logger_cmd,
			sizeof(wp_logger_cmd_t),
			(LPDWORD)(&ln),
			(LPOVERLAPPED)NULL	);

	return handle_device_ioctl_result(bIoResult, __FUNCTION__);
}

/* This function is exported only for debugging purposes and NOT a part of API. */
sangoma_status_t _LIBSNG_CALL sangoma_cdev_ctrl_cmd(sng_fd_t fd, wanpipe_tdm_api_cmd_t *api_cmd)
{
	DWORD ln, bIoResult;

	bIoResult = DeviceIoControl(
			fd,
			IoctlCdevControlCommand,
			(LPVOID)api_cmd,
			sizeof(wanpipe_tdm_api_cmd_t),
			(LPVOID)api_cmd,
			sizeof(wanpipe_tdm_api_cmd_t),
			(LPDWORD)(&ln),
			(LPOVERLAPPED)NULL
			);

	return handle_device_ioctl_result(bIoResult, __FUNCTION__);
}

static sangoma_status_t init_sangoma_event_object(sangoma_wait_obj_t *sng_wait_obj, sng_fd_t fd)
{
	wanpipe_tdm_api_cmd_t tdm_api_cmd;
	sangoma_status_t sng_status;
   	char event_name[200];

	memset(&tdm_api_cmd, 0x00, sizeof(tdm_api_cmd));

	tdm_api_cmd.cmd = WP_CDEV_CMD_GET_INTERFACE_NAME;
	sng_status = sangoma_cdev_ctrl_cmd(fd, &tdm_api_cmd);
	if(SANG_ERROR(sng_status)){
		DBG_ERR("sangoma_cdev_ctrl_cmd() failed!\n");
		return sng_status;
	}

	DBG_EVNT("%s(): interface name: %s\n", __FUNCTION__, tdm_api_cmd.data);

	/* 1. The Sangoma Device Driver creates Notification Events in "\\BaseNamedObjects\\Global\\" 
	 * when first CreateFile() was called by a process. That means the Events will inherit
	 * the security attributes of the calling process, so the calling process will have
	 * the permissions to open the Events by calling OpenEvent(). Since Events are created
	 * "Global" subdirectory, the calling process does NOT need Administrator priveleges.
	 * The example name of the signaling object is: \\BaseNamedObjects\\Global\\wanpipe1_if1_signal.
	 * 2. The Events are deleted when the last HANDLE for a device is closed by CloseHandle()
	 * or automatically by the system when calling process exits. */
	_snprintf(event_name, sizeof(event_name), "Global\\%s_signal", tdm_api_cmd.data);

	sng_wait_obj->signal_object = OpenEvent(EVENT_ALL_ACCESS, TRUE, event_name);
	if(NULL == sng_wait_obj->signal_object){
		/* error */
		LibSangomaDecodeLastError(__FUNCTION__);
		return SANG_STATUS_GENERAL_ERROR;
	}

	return SANG_STATUS_SUCCESS;
}

static sangoma_status_t sangoma_wait_obj_poll(sangoma_wait_obj_t *sangoma_wait_object, int flags_in, uint32_t *flags_out)
{
	int err;
	sangoma_wait_obj_t *sng_wait_obj = sangoma_wait_object;
	 /*! api structure used by windows IoctlApiPoll call */
	API_POLL_STRUCT	api_poll;

	*flags_out = 0;

	memset(&api_poll, 0x00, sizeof(api_poll));
	api_poll.user_flags_bitmap = flags_in;

	/* This call is non-blocking - it will return immediatly. */
	if(sangoma_poll_fd(sng_wait_obj->fd, &api_poll)){
		/* error - ioctl failed */
		return SANG_STATUS_IO_ERROR;
	}

	if(api_poll.operation_status == SANG_STATUS_SUCCESS){
		*flags_out = api_poll.poll_events_bitmap;
		err = 0;
	}else{
		/* error - command failed */
		err = api_poll.operation_status;
	}

	if(*flags_out == 0){
		/*DBG_POLL("======%s(): *flags_out: 0x%X, flags_in: 0x%X\n", __FUNCTION__, *flags_out, flags_in);*/
	}
	return err;
}

static int sangoma_check_number_of_wait_objects(uint32_t number_of_objects)
{
	if(number_of_objects > MAXIMUM_WAIT_OBJECTS){
		DBG_ERR("'number_of_objects': %d is greater than the Maximum of: %d\n", 
			number_of_objects, MAXIMUM_WAIT_OBJECTS);
		return 1;
	}

	if(number_of_objects < 1){
		DBG_ERR("'number_of_objects': %d is less than the Minimum of: 1\n", 
			number_of_objects);
		return 1;
	}

	return 0;
}

static sangoma_status_t get_out_flags(IN sangoma_wait_obj_t *sng_wait_objects[],
									  IN uint32_t first_signaled_obj_index,
									  IN uint32_t in_flags[], OUT uint32_t out_flags[],
									  IN uint32_t number_of_sangoma_wait_objects,
									  OUT BOOL	*at_least_one_poll_set_flags_out)
{
	uint32_t i;

	if(at_least_one_poll_set_flags_out){
		*at_least_one_poll_set_flags_out = FALSE;
	}

	for(i = 0; i < number_of_sangoma_wait_objects; i++) {

		sangoma_wait_obj_t *sangoma_wait_object = sng_wait_objects[i];

		if (!sangoma_wait_object->signal_object) {
			return SANG_STATUS_DEV_INIT_INCOMPLETE;
		}

		if (!SANGOMA_OBJ_HAS_DEVICE(sangoma_wait_object)) {

			/* This object does not has a device, but, may have been signaled via sangoma_wait_obj_signal()
			 * test if the object is signaled, if it is, set SANG_WAIT_OBJ_IS_SIGNALED */

			if((i == first_signaled_obj_index)	||	/* !+! jpboily : Since WaitForMultipleObjects cleared the status
													 * of the FIRST signaled object, we make sure that the out_flag
													 * for this object is set.
													 * !+! davidr: Status of all OTHER objects will be checked
													 * by WaitForSingleObject(). */
				(WaitForSingleObject(sangoma_wait_object->signal_object, 0) == WAIT_OBJECT_0)) {
				out_flags[i] |= SANG_WAIT_OBJ_IS_SIGNALED;
			}
			continue;
		}

		if(sangoma_wait_obj_poll(sangoma_wait_object, in_flags[i], &out_flags[i])){
			return SANG_STATUS_GENERAL_ERROR;
		}

		if (out_flags[i] & in_flags[i]) {
			if (at_least_one_poll_set_flags_out) {
				*at_least_one_poll_set_flags_out = TRUE;
			}
		}

	}/* for(i = 0; i < number_of_sangoma_wait_objects; i++) */

	return SANG_STATUS_SUCCESS;
}

/*!
  \brief Brief description
 */
static LONG registry_get_string_value(HKEY hkey, LPTSTR szKeyname, OUT LPSTR szValue, OUT DWORD *pdwSize)
{
	LONG	iReturnCode;

	/* reading twice to set pdwSize to needed value */
	RegQueryValueEx(hkey, szKeyname, NULL, NULL, (unsigned char *)szValue, pdwSize);
	
	iReturnCode = RegQueryValueEx(hkey, szKeyname, NULL, NULL, (unsigned char *)szValue, pdwSize);
	if(ERROR_SUCCESS == iReturnCode){
		iReturnCode = 0;
	}
	DBG_REGISTRY("%s(): %s: %s: iReturnCode: %d\n", __FUNCTION__, szKeyname, szValue, iReturnCode);
	return iReturnCode;
}

/*!
  \brief Brief description
 */
LONG registry_set_string_value(HKEY hkey, LPTSTR szKeyname, IN LPSTR szValue)
{
	DWORD	dwSize;
	LONG	iReturnCode;

	dwSize = (DWORD)strlen(szValue) + 1;

    iReturnCode = RegSetValueEx(hkey, szKeyname, 0, REG_SZ, (unsigned char *)szValue, dwSize);
	DBG_REGISTRY("%s(): %s: %s\n", __FUNCTION__, szKeyname, szValue);

	if(ERROR_SUCCESS == iReturnCode){
		iReturnCode = 0;
	}
	return iReturnCode;
}

/*!
  \brief Convert an integer (iValue) to string and write it to registry
 */
LONG registry_set_integer_value(HKEY hkey, LPTSTR szKeyname, IN int iValue)
{
	DWORD	dwSize;
	char	szTemp[TMP_BUFFER_LEN];
	LONG	iReturnCode;

	sprintf(szTemp, "%u", iValue);

	dwSize = (DWORD)strlen(szTemp) + 1;
    iReturnCode = RegSetValueEx(hkey, szKeyname, 0, REG_SZ, (unsigned char *)szTemp, dwSize);

	if(ERROR_SUCCESS == iReturnCode){
		iReturnCode = 0;
	}

	DBG_REGISTRY("%s(): %s: %d: iReturnCode: %d\n", __FUNCTION__, szKeyname, iValue, iReturnCode);
	return iReturnCode;
}

/*!
 * \brief Go through the list of ALL "Sangoma Hardware Abstraction Driver" ports installed on the system.
 * Read Bus/Slot/Port information for a port and compare with what is searched for.
 */
HKEY registry_open_port_key(hardware_info_t *hardware_info)
{
	int				i, iRegistryReturnCode;
	SP_DEVINFO_DATA deid={sizeof(SP_DEVINFO_DATA)};
	HDEVINFO		hdi = SetupDiGetClassDevs((struct _GUID *)&GUID_DEVCLASS_SANGOMA_ADAPTER, NULL,NULL, DIGCF_PRESENT);
	char			DeviceName[TMP_BUFFER_LEN], PCI_Bus[TMP_BUFFER_LEN], PCI_Slot[TMP_BUFFER_LEN], Port_Number[TMP_BUFFER_LEN];
	DWORD			dwTemp;
	HKEY			hKeyTmp = (struct HKEY__ *)INVALID_HANDLE_VALUE;
	HKEY			hPortRegistryKey = (struct HKEY__ *)INVALID_HANDLE_VALUE;

	char    szCompInstanceId[MAX_COMP_INSTID];
	TCHAR   szCompDescription[MAX_COMP_DESC];
	DWORD   dwRegType;
	/* Possible Sangoma Port Names (refer to sdladrv.inf):
	Sangoma Hardware Abstraction Driver (Port 1)
	Sangoma Hardware Abstraction Driver (Port 2)
	Sangoma Hardware Abstraction Driver (Port 3)
	Sangoma Hardware Abstraction Driver (Port 4)
	Sangoma Hardware Abstraction Driver (Port 5)
	Sangoma Hardware Abstraction Driver (Port 6)
	Sangoma Hardware Abstraction Driver (Port 7)
	Sangoma Hardware Abstraction Driver (Port 8)
	Sangoma Hardware Abstraction Driver (Analog)
	Sangoma Hardware Abstraction Driver (ISDN BRI)	*/
	const TCHAR	*search_SubStr = "Sangoma Hardware Abstraction Driver";

	/* search for all (AFT) ports: */
	for (i = 0; SetupDiEnumDeviceInfo(hdi, i, &deid); i++){

		BOOL fSuccess = SetupDiGetDeviceInstanceId(hdi, &deid, (PSTR)szCompInstanceId, MAX_COMP_INSTID, NULL);
		if (!fSuccess){
			continue;
		}

		fSuccess = SetupDiGetDeviceRegistryProperty(hdi, &deid, SPDRP_DEVICEDESC, &dwRegType, (BYTE*)szCompDescription, MAX_COMP_DESC, NULL);
		if (!fSuccess){
			continue;
		}

		if (!strstr(szCompDescription, search_SubStr)) {	/* Windows can add #2 etc - do NOT consider */
			/* This is a "Sangoma Card" device, we are interested only in "Sangoma Port" devices. */
			continue;
		}

		DBG_REGISTRY("* %s\n", szCompDescription);

		hKeyTmp = SetupDiOpenDevRegKey(hdi, &deid, DICS_FLAG_GLOBAL, 0, DIREG_DRV, KEY_READ | KEY_SET_VALUE );
		if(hKeyTmp == INVALID_HANDLE_VALUE){
			DBG_REGISTRY("Error: Failed to open Registry key!!\n");
			continue;
		}

		PCI_Bus[0] = '\0';
		iRegistryReturnCode = registry_get_string_value(hKeyTmp, "PCI_Bus", PCI_Bus, &dwTemp);
		if(iRegistryReturnCode){
			continue;
		}

		PCI_Slot[0] = '\0';
		iRegistryReturnCode = registry_get_string_value(hKeyTmp, "PCI_Slot", PCI_Slot, &dwTemp);
		if(iRegistryReturnCode){
			continue;
		}

		Port_Number[0] = '\0';
		iRegistryReturnCode = registry_get_string_value(hKeyTmp, "Port_Number", Port_Number, &dwTemp);
		if(iRegistryReturnCode){
			continue;
		}

		if(	atoi(PCI_Bus)		== hardware_info->pci_bus_number	&&
			atoi(PCI_Slot)		== hardware_info->pci_slot_number	&&
			atoi(Port_Number)	== hardware_info->port_number		){

			hPortRegistryKey = hKeyTmp;

			/* read device name for debugging only */
			DeviceName[0] = '\0';
			registry_get_string_value(hPortRegistryKey, "DeviceName", DeviceName, &dwTemp);

			DBG_REGISTRY("Found Port's Registry key: DeviceName: %s, PCI_Bus: %s, PCI_Slot: %s, Port_Number: %s\n",
				DeviceName, PCI_Bus, PCI_Slot, Port_Number);
			break;
		}/* if() */
	}/* for() */

	SetupDiDestroyDeviceInfoList(hdi);

	DBG_REGISTRY("hPortRegistryKey: 0x%X\n", hPortRegistryKey);

	return hPortRegistryKey;
}

static char* timeslot_bitmap_to_string(int bitmap)
{
	int i = 0, range_counter = 0;
	int start_channel = -1, stop_channel = -1;
	static char tmp_string[256];

	if( WP_ALL_BITS_SET == bitmap ){
		return "ALL"; /* If all bits are set, use the "ALL" keyword. */
	}
	
	tmp_string[0] = '\0';

	/* all ranges between two zeros is what we will look for */
	for(i = 0; i < sizeof(bitmap) * 8; i++){

		if(start_channel < 0){
			/* found a starting one of a range */
			if(bitmap & (1 << i)){
				start_channel = i + 1;
			}
		}

		if(start_channel >= 0){
			if((bitmap & (1 << i)) == 0){
				
				/* we hit a zero, that means the previous channel is one */
				stop_channel = i;

			}else if(i == (sizeof(bitmap) * 8 - 1)){
				/* The most significant bit is set - there will be no delimiting zero.
				* It will also take care of "all channels" which is a special
				* case - there is a start but no stop channel because all bits
				* are set. result will be 1-32 */
				stop_channel = (sizeof(bitmap) * 8);
			}
		}

		if(start_channel >= 0 && stop_channel >= 0){

			if(range_counter){
				/* put '.' separator from the previous range */
				_snprintf(&tmp_string[strlen(tmp_string)], sizeof(tmp_string) - strlen(tmp_string), ".");
			}
							
			if(start_channel == stop_channel){
				/* the range contains a SINGLE channel */
				_snprintf(&tmp_string[strlen(tmp_string)], sizeof(tmp_string) - strlen(tmp_string),
					"%d", start_channel);
			}else{
				/* the range contains MULTIPLE channels */
				_snprintf(&tmp_string[strlen(tmp_string)], sizeof(tmp_string) - strlen(tmp_string),
					"%d-%d", start_channel, stop_channel);
			}

			start_channel = stop_channel = -1;
			range_counter++;
		}
	}/* for() */

	return tmp_string;
}

int registry_write_front_end_cfg(HKEY hPortRegistryKey, port_cfg_t *port_cfg)
{
	wandev_conf_t	*wandev_conf	= &port_cfg->wandev_conf;
	sdla_fe_cfg_t	*sdla_fe_cfg	= &wandev_conf->fe_cfg;
	sdla_te_cfg_t	*te_cfg			= &sdla_fe_cfg->cfg.te_cfg;
	sdla_remora_cfg_t *analog_cfg	= &sdla_fe_cfg->cfg.remora;
	sdla_bri_cfg_t	*bri_cfg		= &sdla_fe_cfg->cfg.bri;
	int iReturnCode = 0;

	/* Set Card/Port-wide values, which are independent of Media type, and
	 * stored in sdla_fe_cfg_t. */

	iReturnCode = registry_set_integer_value(hPortRegistryKey, "tdmv_law", FE_TDMV_LAW(sdla_fe_cfg) /* WAN_TDMV_MULAW/WAN_TDMV_ALAW */);
	if(iReturnCode){
		return iReturnCode;
	}

	iReturnCode = registry_set_integer_value(hPortRegistryKey, "ExternalSynchronization", FE_NETWORK_SYNC(sdla_fe_cfg) /* WANOPT_NO/WANOPT_YES */);
	if(iReturnCode){
		return iReturnCode;
	}

	iReturnCode = registry_set_integer_value(hPortRegistryKey, "FE_TXTRISTATE", FE_TXTRISTATE(sdla_fe_cfg) /* WANOPT_NO/WANOPT_YES */);
	if(iReturnCode){
		return iReturnCode;
	}

	iReturnCode = registry_set_integer_value(hPortRegistryKey, "HWEC_CLKSRC", wandev_conf->hwec_conf.clk_src /* is this port the HWEC clock source - WANOPT_NO/WANOPT_YES */);
	if(iReturnCode){
		return iReturnCode;
	}


	/* set Media specific values. */
	switch(sdla_fe_cfg->media)
	{
	case WAN_MEDIA_T1:
	case WAN_MEDIA_J1: /* the same as T1 */
	case WAN_MEDIA_E1:
		/* only T1/E1 Port can change the Media, all other ports ignore this parameter. */
		iReturnCode = registry_set_integer_value(hPortRegistryKey, "Media",		sdla_fe_cfg->media	/*WAN_MEDIA_T1*/);
		if(iReturnCode){
			break;
		}

		iReturnCode = registry_set_integer_value(hPortRegistryKey, "LDecoding",	sdla_fe_cfg->lcode	/*WAN_LCODE_B8ZS*/);
		if(iReturnCode){
			break;
		}

		iReturnCode = registry_set_integer_value(hPortRegistryKey, "Framing",		sdla_fe_cfg->frame	/*WAN_FR_ESF*/);
		if(iReturnCode){
			break;
		}

		iReturnCode = registry_set_integer_value(hPortRegistryKey, "ClkRefPort",		te_cfg->te_ref_clock);
		if(iReturnCode){
			break;
		}

		iReturnCode = registry_set_integer_value(hPortRegistryKey, "TE_IGNORE_YEL",	te_cfg->ignore_yel_alarm);
		if(iReturnCode){
			break;
		}

		iReturnCode = registry_set_integer_value(hPortRegistryKey, "ACTIVE_CH",		ENABLE_ALL_CHANNELS	/*must be hardcoded*/);
		if(iReturnCode){
			break;
		}

		iReturnCode = registry_set_integer_value(hPortRegistryKey, "TE_RBS_CH",		te_cfg->te_rbs_ch);	/*not used by DS chip code, only by PMC */
		if(iReturnCode){
			break;
		}

		iReturnCode = registry_set_integer_value(hPortRegistryKey, "LBO",				te_cfg->lbo			/*WAN_T1_LBO_0_DB*/);
		if(iReturnCode){
			break;
		}

		iReturnCode = registry_set_integer_value(hPortRegistryKey, "ClkMode",			te_cfg->te_clock	/*WAN_NORMAL_CLK*/);
		if(iReturnCode){
			break;
		}

		iReturnCode = registry_set_integer_value(hPortRegistryKey, "HighImpedanceMode",te_cfg->high_impedance_mode /*WANOPT_NO*/);
		if(iReturnCode){
			break;
		}

		iReturnCode = registry_set_integer_value(hPortRegistryKey, "TE_RX_SLEVEL",	te_cfg->rx_slevel	/*WAN_TE1_RX_SLEVEL_12_DB*/);
		if(iReturnCode){
			break;
		}

		/* write E1 signalling for both T1 and E1 */
		iReturnCode = registry_set_integer_value(hPortRegistryKey, "E1Signalling",	te_cfg->sig_mode	/*WAN_TE1_SIG_CCS*/);
		if(iReturnCode){
			break;
		}
		break;

	case WAN_MEDIA_56K:
		/* do nothing */
		iReturnCode = 0;
		break;

	case WAN_MEDIA_FXOFXS:
		/* Analog global (per-card) settings */
		iReturnCode = registry_set_string_value(hPortRegistryKey, "remora_fxo_operation_mode_name",	analog_cfg->opermode_name);
		if(iReturnCode){
			break;
		}

		iReturnCode = registry_set_integer_value(hPortRegistryKey, "RM_BATTTHRESH",		analog_cfg->battthresh);
		if(iReturnCode){
			break;
		}

		iReturnCode = registry_set_integer_value(hPortRegistryKey, "RM_BATTDEBOUNCE",	analog_cfg->battdebounce);
		if(iReturnCode){
			break;
		}

		iReturnCode = registry_set_integer_value(hPortRegistryKey, "RM_FXO_TAPPING",	analog_cfg->rm_mode);
		if(iReturnCode){
			break;
		}
		
		iReturnCode = registry_set_integer_value(hPortRegistryKey, "RM_FXO_TAPPING_OFF_HOOK_THRESHOLD",analog_cfg->ohthresh);
		if(iReturnCode){
			break;
		}

		iReturnCode = registry_set_integer_value(hPortRegistryKey, "RM_LCM", analog_cfg->rm_lcm);
		if(iReturnCode){
			break;
		}

		iReturnCode = registry_set_integer_value(hPortRegistryKey, "RM_FXSTXGAIN", analog_cfg->fxs_txgain);
		if(iReturnCode){
			break;
		}

		iReturnCode = registry_set_integer_value(hPortRegistryKey, "RM_FXSRXGAIN", analog_cfg->fxs_rxgain);
		if(iReturnCode){
			break;
		}

		iReturnCode = registry_set_integer_value(hPortRegistryKey, "RM_FXOTXGAIN", analog_cfg->fxo_txgain);
		if(iReturnCode){
			break;
		}

		iReturnCode = registry_set_integer_value(hPortRegistryKey, "RM_FXORXGAIN", analog_cfg->fxo_rxgain);
		if(iReturnCode){
			break;
		}
		break;

	case WAN_MEDIA_BRI:
		iReturnCode = registry_set_integer_value(hPortRegistryKey, "aft_bri_clock_mode", bri_cfg->clock_mode);
		if(iReturnCode){
			break;
		}
		break;

	case WAN_MEDIA_SERIAL:
		iReturnCode = registry_set_integer_value(hPortRegistryKey, "clock_source",			wandev_conf->clocking);
		if(iReturnCode){
			break;
		}

		iReturnCode = registry_set_integer_value(hPortRegistryKey, "BaudRate",				wandev_conf->bps);
		if(iReturnCode){
			break;
		}

		iReturnCode = registry_set_integer_value(hPortRegistryKey, "serial_connection_type",	wandev_conf->connection);
		if(iReturnCode){
			break;
		}

		iReturnCode = registry_set_integer_value(hPortRegistryKey, "serial_line_coding",		wandev_conf->line_coding);
		if(iReturnCode){
			break;
		}

		iReturnCode = registry_set_integer_value(hPortRegistryKey, "serial_line_idle",		wandev_conf->line_idle);
		if(iReturnCode){
			break;
		}
		break;

	default:
		DBG_ERR("Invalid Media Type %d!\n", sdla_fe_cfg->media);
		iReturnCode = 1;
	}

	return iReturnCode;
}

int registry_write_wan_tdmv_conf(HKEY hPortRegistryKey, port_cfg_t *port_cfg)
{
	wandev_conf_t	*wandev_conf	= &port_cfg->wandev_conf;
	sdla_fe_cfg_t	*sdla_fe_cfg	= &wandev_conf->fe_cfg;
	wan_tdmv_conf_t	*tdmv_conf		= &wandev_conf->tdmv_conf;
	int	iReturnCode = 1;

	/* set Media specific values. */
	switch(sdla_fe_cfg->media)
	{
	case WAN_MEDIA_T1:
	case WAN_MEDIA_J1: /* the same as T1 */
	case WAN_MEDIA_E1:
		/* write dchannel bitmap as a string (the same way as active channels) */
		iReturnCode = registry_set_string_value(hPortRegistryKey, "TDMV_DCHAN", timeslot_bitmap_to_string(tdmv_conf->dchan));
		break;

	case WAN_MEDIA_56K:
	case WAN_MEDIA_FXOFXS:
	case WAN_MEDIA_BRI:
	case WAN_MEDIA_SERIAL:
		/* do nothing */
		iReturnCode = 0;
		break;

	default:
		DBG_ERR("Invalid Media Type %d!\n", sdla_fe_cfg->media);
		iReturnCode = 2;
		break;
	}

	return iReturnCode;
}

int registry_write_channel_group_cfg(HKEY hPortRegistryKey, port_cfg_t *port_cfg, int interface_index, wanif_conf_t wanif_conf)
{
	char	szTemp[TMP_BUFFER_LEN];
	int		iReturnCode = 0;

	do{
		// Line mode
		_snprintf(szTemp, TMP_BUFFER_LEN, "aft_logic_channel_%d_line_mode", interface_index);
		iReturnCode = registry_set_string_value(hPortRegistryKey, szTemp, 
			(wanif_conf.hdlc_streaming == WANOPT_YES ? MODE_OPTION_HDLC : MODE_OPTION_BITSTRM));
		if(iReturnCode){
			break;
		}

		// MTU
		_snprintf(szTemp, TMP_BUFFER_LEN, "aft_logic_channel_%d_mtu", interface_index);
		iReturnCode = registry_set_integer_value(hPortRegistryKey, szTemp, wanif_conf.mtu);
		if(iReturnCode){
			break;
		}

		// Operational mode
		_snprintf(szTemp, TMP_BUFFER_LEN, "aft_logic_channel_%d_operational_mode", interface_index);
		iReturnCode = registry_set_string_value(hPortRegistryKey, szTemp, wanif_conf.usedby);
		if(iReturnCode){
			break;
		}

		// Active Timeslots for the Group
		_snprintf(szTemp, TMP_BUFFER_LEN, "aft_logic_channel_%d_active_ch", interface_index);
		iReturnCode = registry_set_string_value(hPortRegistryKey, szTemp, timeslot_bitmap_to_string(wanif_conf.active_ch));
		if(iReturnCode){
			break;
		}

		// Idle char.
		_snprintf(szTemp, TMP_BUFFER_LEN, "aft_logic_channel_%d_idle_char", interface_index);
		iReturnCode = registry_set_integer_value(hPortRegistryKey, szTemp, wanif_conf.u.aft.idle_flag);
		if(iReturnCode){
			break;
		}

	}while(0);

	return iReturnCode;
}

/*!
 * \brief Go through the list of ALL Sangoma PCI Adapters (Cards) on the system.
 * Enable or Disable each one of them (state change will be visible in the Device Manager).
 */
static sangoma_status_t sangoma_control_cards(DWORD StateChange)
{
	sangoma_status_t rc;
	int		i;
	SP_DEVINFO_DATA deid = {sizeof(SP_DEVINFO_DATA)};
	HDEVINFO	hdi;
	TCHAR	szInstanceId[MAX_COMP_INSTID];

	const TCHAR *szAftSearchSubStr = "PCI\\VEN_1923"; /* Sangoma PCI Vendor ID is 1923 */
	/*const TCHAR *szS518AdslSearchSubStr = "PCI\\VEN_14BC&DEV_D002";*/ /* Note: S518 is end-of-life */

	SP_PROPCHANGE_PARAMS	pcp;

	hdi = SetupDiGetClassDevs((struct _GUID *)&GUID_DEVCLASS_SANGOMA_ADAPTER, NULL, NULL, DIGCF_PRESENT);
	if(INVALID_HANDLE_VALUE == hdi){
		/* no sangoma cards installed on the system */
		return SANG_STATUS_INVALID_DEVICE;
	}

	/* search for all AFT Cards: */
	for (i = 0; SetupDiEnumDeviceInfo(hdi, i, &deid); i++){

		BOOL fSuccess = SetupDiGetDeviceInstanceId(hdi, &deid, (PSTR)szInstanceId, sizeof(szInstanceId), NULL);
		if (!fSuccess){
			rc = SANG_STATUS_REGISTRY_ERROR;
			break;
		}
		if(!strstr(szInstanceId, szAftSearchSubStr)){
			/* Found Sangoma Port, but we are interested only Cards. */
			continue;
		}

		memset(&pcp, 0x00, sizeof(pcp));
		//Set the PropChangeParams structure.
		pcp.ClassInstallHeader.cbSize = sizeof(SP_CLASSINSTALL_HEADER);
		pcp.ClassInstallHeader.InstallFunction = DIF_PROPERTYCHANGE;
		pcp.Scope = DICS_FLAG_GLOBAL; /* DICS_FLAG_CONFIGSPECIFIC will enable ALL cards, including
									   * those explicetly DISABLED by the user in the Device Manager,
									   * and we don't want to force anything on the user, hence we
									   * use DICS_FLAG_GLOBAL. */
		pcp.StateChange = StateChange;

		if(!SetupDiSetClassInstallParams(hdi, &deid, (SP_CLASSINSTALL_HEADER *)&pcp, sizeof(pcp))){
			rc = SANG_STATUS_REGISTRY_ERROR;
			break;
		}

		if(!SetupDiCallClassInstaller(DIF_PROPERTYCHANGE, hdi, &deid)){
			rc = SANG_STATUS_REGISTRY_ERROR;
			break;
		}

	}/* for() */

	SetupDiDestroyDeviceInfoList(hdi);

	return SANG_STATUS_SUCCESS;
}

sangoma_status_t _LIBSNG_CALL sangoma_unload_driver()
{
	return sangoma_control_cards(DICS_DISABLE);
}

sangoma_status_t _LIBSNG_CALL sangoma_load_driver()
{
	return sangoma_control_cards(DICS_ENABLE);
}

/*!
 * \brief Go through the list of ALL "Sangoma Hardware Abstraction Driver" ports installed on the system,
 * and assign sequential numbers to the Ports, starting from 1. This will override the Automatic Wanpipe
 * Span numbering.
 */
void _LIBSNG_CALL sangoma_reset_port_numbers()
{
	int				i, iRegistryReturnCode, iPortCounter = 0;
	SP_DEVINFO_DATA deid={sizeof(SP_DEVINFO_DATA)};
	HDEVINFO		hdi = SetupDiGetClassDevs((struct _GUID *)&GUID_DEVCLASS_SANGOMA_ADAPTER, NULL,NULL, DIGCF_PRESENT);
	HKEY			hKeyTmp = (struct HKEY__ *)INVALID_HANDLE_VALUE;

	char    szCompInstanceId[MAX_COMP_INSTID];
	TCHAR   szCompDescription[MAX_COMP_DESC];
	DWORD   dwRegType;
	/* Possible Sangoma Port Names (refer to sdladrv.inf):
	Sangoma Hardware Abstraction Driver (Port 1)
	Sangoma Hardware Abstraction Driver (Port 2)
	Sangoma Hardware Abstraction Driver (Port 3)
	Sangoma Hardware Abstraction Driver (Port 4)
	Sangoma Hardware Abstraction Driver (Port 5)
	Sangoma Hardware Abstraction Driver (Port 6)
	Sangoma Hardware Abstraction Driver (Port 7)
	Sangoma Hardware Abstraction Driver (Port 8)
	Sangoma Hardware Abstraction Driver (Analog)
	Sangoma Hardware Abstraction Driver (ISDN BRI)	*/
	const TCHAR	*search_SubStr = "Sangoma Hardware Abstraction Driver";

	/* search for all (AFT) ports: */
	for (i = 0; SetupDiEnumDeviceInfo(hdi, i, &deid); i++){

		BOOL fSuccess = SetupDiGetDeviceInstanceId(hdi, &deid, (PSTR)szCompInstanceId, MAX_COMP_INSTID, NULL);
		if (!fSuccess){
			continue;
		}

		fSuccess = SetupDiGetDeviceRegistryProperty(hdi, &deid, SPDRP_DEVICEDESC, &dwRegType, (BYTE*)szCompDescription, MAX_COMP_DESC, NULL);
		if (!fSuccess){
			continue;
		}

		if (!strstr(szCompDescription, search_SubStr)) {	/* Windows can add #2 etc - do NOT consider */
			/* This is a "Sangoma Card" device, we are interested only in "Sangoma Port" devices. */
			continue;
		}

		hKeyTmp = SetupDiOpenDevRegKey(hdi, &deid, DICS_FLAG_GLOBAL, 0, DIREG_DRV, KEY_READ | KEY_SET_VALUE );
		if(hKeyTmp == INVALID_HANDLE_VALUE){
			DBG_REGISTRY("Error: Failed to open Registry key!!\n");
			continue;
		}

		iPortCounter++;

		printf("* %s -> Setting Span/Port number: %d\n", szCompDescription, iPortCounter);

		/* TODO: 1. search for installed Sangoma Cards
				2. for each Card, search Ports (Bus/Slot must match)
				3. based on number of installed Ports, set SerialNumbersRange of the Card
				
				This way the WP_REGSTR_USER_SPECIFIED_WANPIPE_NUMBER can be set to zero and Span numbers will be
				sequencial automatically.
		*/
		iRegistryReturnCode = registry_set_integer_value(hKeyTmp, WP_REGSTR_USER_SPECIFIED_WANPIPE_NUMBER, iPortCounter);
		if(iRegistryReturnCode){
			continue;
		}

		RegCloseKey(hKeyTmp);

	}/* for() */

	SetupDiDestroyDeviceInfoList(hdi);

	return;
}


/*!
 * \brief Get version of driver from the Windows Registry, as it was written by .inf parser.
 *		Note that there is a possibility that currently loaded driver has a different
 *		version (if the original binary file was overwritten).
 */
sangoma_status_t _LIBSNG_CALL sangoma_get_driver_version_from_registry(char *out_buffer, int out_buffer_length)
{
	int				i, iRegistryReturnCode;
	SP_DEVINFO_DATA deid = {sizeof(SP_DEVINFO_DATA)};
	HDEVINFO		hdi = SetupDiGetClassDevs((struct _GUID *)&GUID_DEVCLASS_SANGOMA_ADAPTER, NULL, NULL, DIGCF_PRESENT);
	DWORD			dwTemp;
	HKEY			hKeyTmp = (struct HKEY__ *)INVALID_HANDLE_VALUE;

	TCHAR	szTmp[MAX_COMP_DESC];
	BOOL	fSuccess = FALSE;

	/* search for ANY Sangoma device of class GUID_DEVCLASS_SANGOMA_ADAPTER */
	for (i = 0; SetupDiEnumDeviceInfo(hdi, i, &deid); i++){

		fSuccess = SetupDiGetDeviceInstanceId(hdi, &deid, (PSTR)szTmp, sizeof(szTmp), NULL);
		if (!fSuccess){
			break;
		}
		DBG_REGISTRY("%s(): Device Instance Id: %s\n", __FUNCTION__, szTmp);
		/*	Device Instance Id: COMMSADAPTER\SANGOMAADAPTER_AFT_LINE8\5&32E1B75F&0&13
			Device Instance Id: PCI\VEN_1923&DEV_0100&SUBSYS_4113A114&REV_00\4&31B6CD7&0&10F0 	*/

		hKeyTmp = SetupDiOpenDevRegKey( hdi, &deid, DICS_FLAG_GLOBAL, 0, DIREG_DRV, KEY_READ | KEY_SET_VALUE );
		if(hKeyTmp == INVALID_HANDLE_VALUE){
			DBG_REGISTRY("Error: Failed to open Registry key!!\n");
			fSuccess = FALSE;
			break;
		}

		iRegistryReturnCode = registry_get_string_value(hKeyTmp, "DriverVersion", szTmp, &dwTemp);
		if(iRegistryReturnCode){
			fSuccess = FALSE;
			break;
		}

		wp_snprintf(out_buffer, out_buffer_length, szTmp);

		RegCloseKey(hKeyTmp);

		/* it is enough to read the version only once, so break here */
		break; /* breaking here will generate "warning C4702: unreachable code" - it is ok */
	}/* for() */

	SetupDiDestroyDeviceInfoList(hdi);

	if (!fSuccess){
		return SANG_STATUS_REGISTRY_ERROR;
	}else{
		return SANG_STATUS_SUCCESS;
	}
}

/*!
 * \brief Set the Driver Mode of all Hardware (not virtual) devices.
 *		The modes are:	Normal
 *						Firmware Update
 */
sangoma_status_t _LIBSNG_CALL sangoma_set_driver_mode_of_all_hw_devices(int driver_mode)
{
	sangoma_status_t rc;
	int		i, iRegistryReturnCode;
	SP_DEVINFO_DATA deid = {sizeof(SP_DEVINFO_DATA)};
	HDEVINFO	hdi;
	TCHAR	szInstanceId[MAX_COMP_INSTID];
	HKEY	hKeyTmp = (struct HKEY__ *)INVALID_HANDLE_VALUE;

	const TCHAR *szAftSearchSubStr = "PCI\\VEN_1923"; /* Sangoma PCI Vendor ID is 1923 */

	hdi = SetupDiGetClassDevs((struct _GUID *)&GUID_DEVCLASS_SANGOMA_ADAPTER, NULL, NULL, DIGCF_PRESENT);
	if(INVALID_HANDLE_VALUE == hdi){
		/* no sangoma cards installed on the system */
		return SANG_STATUS_INVALID_DEVICE;
	}

	/* search for all AFT Cards: */
	for (i = 0; SetupDiEnumDeviceInfo(hdi, i, &deid); i++){

		BOOL fSuccess = SetupDiGetDeviceInstanceId(hdi, &deid, (PSTR)szInstanceId, sizeof(szInstanceId), NULL);
		if (!fSuccess){
			rc = SANG_STATUS_REGISTRY_ERROR;
			break;
		}
		if(!strstr(szInstanceId, szAftSearchSubStr)){
			/* Found Sangoma Port, but we are interested only Cards. */
			continue;
		}

		hKeyTmp = SetupDiOpenDevRegKey(hdi, &deid, DICS_FLAG_GLOBAL, 0, DIREG_DRV, KEY_READ | KEY_SET_VALUE );
		if(hKeyTmp == INVALID_HANDLE_VALUE){
			rc = SANG_STATUS_REGISTRY_ERROR;
			break;
		}

		iRegistryReturnCode = registry_set_integer_value(hKeyTmp, "driver_mode", driver_mode);
		if(iRegistryReturnCode){
			rc = SANG_STATUS_REGISTRY_ERROR;
			break;
		}

		RegCloseKey(hKeyTmp);

	}/* for() */

	SetupDiDestroyDeviceInfoList(hdi);

	return SANG_STATUS_SUCCESS;
}


#endif	/* __WINDOWS__ */


/*********************************************************************//**
 * Common Linux & Windows Code
 *************************************************************************/

/*************************************************/
/* private functions for accessing wan_udp_hdr_t */
/*************************************************/
/* return POINTER to DATA at offset 'off' */
static unsigned char* sangoma_get_wan_udphdr_data_ptr(wan_udp_hdr_t *wan_udp_ptr, unsigned char off)
{
	unsigned char *p_data = &wan_udp_ptr->wan_udphdr_data[0];
	p_data += off;
	return p_data;
}

/* set a single byte of DATA at offset 'off' */
static unsigned char sangoma_set_wan_udphdr_data_byte(wan_udp_hdr_t *wan_udp_ptr, unsigned char off, unsigned char data)
{
	unsigned char *p_data = &wan_udp_ptr->wan_udphdr_data[0];
	p_data[off] = data;
	return 0;
}

/* return a single byte of DATA at offset 'off' */
static unsigned char sangoma_get_wan_udphdr_data_byte(wan_udp_hdr_t *wan_udp_ptr, unsigned char off)
{
	unsigned char *p_data = &wan_udp_ptr->wan_udphdr_data[0];
	return p_data[off];
}
/*************************************************/


/************************************************************//**
 * Device POLL Functions
 ***************************************************************/ 

/*!
  \fn sangoma_status_t sangoma_wait_obj_create(sangoma_wait_obj_t **sangoma_wait_object, sng_fd_t fd, sangoma_wait_obj_type_t object_type)
  \brief Create a wait object that will be used with sangoma_waitfor_many() API
  \param sangoma_wait_object pointer a single device object 
  \param fd device file descriptor
  \param object_type type of the wait object. see sangoma_wait_obj_type_t for types
  \see sangoma_wait_obj_type_t
  \return SANG_STATUS_SUCCESS: success, or error status
*/
sangoma_status_t _LIBSNG_CALL sangoma_wait_obj_create(sangoma_wait_obj_t **sangoma_wait_object, sng_fd_t fd, sangoma_wait_obj_type_t object_type)
{
	int err = 0;
	sangoma_wait_obj_t *sng_wait_obj = NULL;

	if (!sangoma_wait_object) { 
		return SANG_STATUS_INVALID_PARAMETER;
	}
	*sangoma_wait_object = NULL;
	sng_wait_obj = malloc(sizeof(**sangoma_wait_object));
	if (!sng_wait_obj) {
		return SANG_STATUS_FAILED_ALLOCATE_MEMORY;
	}

	memset(sng_wait_obj, 0x00, sizeof(*sng_wait_obj));
	/* it is a first initialization of the object */
	sng_wait_obj->init_flag = LIBSNG_MAGIC_NO;

	sng_wait_obj->fd			= fd;
	sng_wait_obj->object_type	= object_type;

#if defined(__WINDOWS__)
	if (!SANGOMA_OBJ_HAS_DEVICE(sng_wait_obj)) {
		sng_wait_obj->signal_object = CreateEvent(NULL, FALSE, FALSE, NULL);
		if(!sng_wait_obj->signal_object){
			err = SANG_STATUS_GENERAL_ERROR;
			goto failed;
		}
		err = SANG_STATUS_SUCCESS;
	} else {
		err = init_sangoma_event_object(sng_wait_obj, fd);
		if(SANG_STATUS_SUCCESS != err){
			goto failed;
		}
	}
#else
	int flags;
	int filedes[2];
	if (SANGOMA_OBJ_IS_SIGNALABLE(sng_wait_obj)) {
		sng_wait_obj->signal_read_fd = INVALID_HANDLE_VALUE;
		sng_wait_obj->signal_write_fd = INVALID_HANDLE_VALUE;
		/* if we want cross-process event notification we can use a named pipe with mkfifo() */
		if (pipe(filedes)) {
			err = SANG_STATUS_GENERAL_ERROR;
			goto failed;
		}
		sng_wait_obj->signal_read_fd = filedes[0];
		sng_wait_obj->signal_write_fd = filedes[1];
		/* make the read fd non-blocking, multiple threads can wait for it but just one
		 * wil read the dummy notify character, the others would block otherwise and that's
		 * not what we want */
		flags = fcntl(sng_wait_obj->signal_read_fd, F_GETFL, 0);
		if (flags < 0) {
			err = SANG_STATUS_GENERAL_ERROR;
			goto failed;
		}
		flags |= O_NONBLOCK;
		fcntl(sng_wait_obj->signal_read_fd, F_SETFL, flags);
		flags = fcntl(sng_wait_obj->signal_read_fd, F_GETFL, 0);
		if (flags < 0 || !(flags & O_NONBLOCK)) {
			err = SANG_STATUS_GENERAL_ERROR;
			goto failed;
		}
	}
#endif
	*sangoma_wait_object = sng_wait_obj;
	return err;

failed:
	if (sng_wait_obj) {
		sangoma_wait_obj_delete(&sng_wait_obj);
	}
	return err;
}

/*!
  \fn sangoma_status_t sangoma_wait_obj_delete(sangoma_wait_obj_t **sangoma_wait_object)
  \brief De-allocate all resources in a wait object
  \param sangoma_wait_object pointer to a pointer to a single device object
  \return SANG_STATUS_SUCCESS on success or some sangoma status error
*/
sangoma_status_t _LIBSNG_CALL sangoma_wait_obj_delete(sangoma_wait_obj_t **sangoma_wait_object)
{
	sangoma_wait_obj_t *sng_wait_obj = *sangoma_wait_object;

	if(sng_wait_obj->init_flag != LIBSNG_MAGIC_NO){
		/* error. object was not initialized by sangoma_wait_obj_init() */
		return SANG_STATUS_INVALID_DEVICE;
	}

#if defined(__WINDOWS__)
	if (sng_wait_obj->signal_object &&
		sng_wait_obj->signal_object != INVALID_HANDLE_VALUE) {
		sangoma_close(&sng_wait_obj->signal_object);
	}
#else
	if (SANGOMA_OBJ_IS_SIGNALABLE(sng_wait_obj)) {
		sangoma_close(&sng_wait_obj->signal_read_fd);
		sangoma_close(&sng_wait_obj->signal_write_fd);
	}
#endif
	sng_wait_obj->init_flag = 0;
	sng_wait_obj->object_type = UNKNOWN_WAIT_OBJ;
	free(sng_wait_obj);
	*sangoma_wait_object = NULL;
	return SANG_STATUS_SUCCESS;
}

/*!
  \fn void sangoma_wait_obj_signal(void *sangoma_wait_object)
  \brief Set wait object to a signaled state
  \param sangoma_wait_object pointer a single device object 
  \return 0 on success, non-zero on error
*/
int _LIBSNG_CALL sangoma_wait_obj_signal(sangoma_wait_obj_t *sng_wait_obj)
{
	if (!SANGOMA_OBJ_IS_SIGNALABLE(sng_wait_obj)) {
		/* even when Windows objects are always signalable for the sake of providing
		 * a consistent interface to the user we downgrade the capabilities of Windows
		 * objects unless the sangoma wait object is explicitly initialized as signalable */
		return SANG_STATUS_INVALID_DEVICE;
	}
#if defined(__WINDOWS__)
	if(sng_wait_obj->signal_object){
		if (!SetEvent(sng_wait_obj->signal_object)) {
			return SANG_STATUS_GENERAL_ERROR;
		}
	}
#else
	/* at this point we know is a signalable object and has a signal_write_fd */
	if (write(sng_wait_obj->signal_write_fd, "s", 1) < 1) {
		return SANG_STATUS_GENERAL_ERROR;
	}
#endif
	return SANG_STATUS_SUCCESS;
}

/*!
  \fn sng_fd_t sangoma_wait_obj_get_fd(void *sangoma_wait_object)
  \brief Retrieve fd device file descriptor which was the 'fd' parameter for sangoma_wait_obj_init()
  \param sangoma_wait_object pointer a single device object 
  \return fd - device file descriptor
*/
sng_fd_t _LIBSNG_CALL sangoma_wait_obj_get_fd(sangoma_wait_obj_t *sng_wait_obj)
{
	return sng_wait_obj->fd;
}

/*!
  \fn void sangoma_wait_obj_set_context(sangoma_wait_obj_t *sangoma_wait_object)
  \brief Store the given context into provided sangoma wait object.
  \brief This function is useful to associate a context with a sangoma wait object.
  \param sangoma_wait_object pointer a single device object 
  \param context void pointer to user context
  \return void
*/
void _LIBSNG_CALL sangoma_wait_obj_set_context(sangoma_wait_obj_t *sng_wait_obj, void *context)
{
	sng_wait_obj->context = context;
}

/*!
  \fn PVOID sangoma_wait_obj_get_context(sangoma_wait_obj_t *sangoma_wait_object)
  \brief Retrieve the user context (if any) that was set via sangoma_wait_obj_set_context.
  \param sangoma_wait_object pointer a single device object 
  \return void*
*/
PVOID _LIBSNG_CALL sangoma_wait_obj_get_context(sangoma_wait_obj_t *sng_wait_obj)
{
	return sng_wait_obj->context;
}

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
sangoma_status_t _LIBSNG_CALL sangoma_waitfor_many(sangoma_wait_obj_t *sng_wait_objects[], uint32_t in_flags[], uint32_t out_flags[],
		uint32_t number_of_sangoma_wait_objects, int32_t system_wait_timeout)
{
#if defined(__WINDOWS__)
	HANDLE hEvents[MAXIMUM_WAIT_OBJECTS];
	DWORD dwResult;
	int at_least_one_poll_set_flags_out, err;
	sangoma_wait_obj_t *sangoma_wait_object;
#else
	uint32_t j = 0;
#endif
	uint32_t i = 0;

	memset(out_flags, 0x00, number_of_sangoma_wait_objects * sizeof(out_flags[0]));
#if defined(__WINDOWS__)

	if(sangoma_check_number_of_wait_objects(number_of_sangoma_wait_objects)){
		/* error - most likely the user did not initialize sng_wait_objects[] */
		return SANG_STATUS_INVALID_PARAMETER;
	}

	for(i = 0; i < MAXIMUM_WAIT_OBJECTS; i++){
		hEvents[i] = INVALID_HANDLE_VALUE;
	}

	/* This loop will initialize 'hEvents[]' based on
	 * 'number_of_sangoma_wait_objects' and sng_wait_objects[].  */
	for(i = 0; i < number_of_sangoma_wait_objects; i++){

		sangoma_wait_object = sng_wait_objects[i];

		if(LIBSNG_MAGIC_NO != sangoma_wait_object->init_flag){
			return SANG_STATUS_DEV_INIT_INCOMPLETE;
		}

		if(sangoma_wait_object->signal_object){
			hEvents[i] = sangoma_wait_object->signal_object;
		}

	}/* for() */

	at_least_one_poll_set_flags_out = FALSE;

	/* It is important to get 'out flags' BEFORE the WaitForMultipleObjects()
	 * because it allows to keep API driver's TRANSMIT queue full. */
	err = get_out_flags(sng_wait_objects, INVALID_INDEX, in_flags, out_flags, number_of_sangoma_wait_objects, &at_least_one_poll_set_flags_out);
	if(SANG_ERROR(err)){
		return err;
	}

	if(TRUE == at_least_one_poll_set_flags_out){
		return SANG_STATUS_SUCCESS;
	}

	/* wait untill at least one of the events is signaled OR a 'system_wait_timeout' occured */
	dwResult = WaitForMultipleObjects(number_of_sangoma_wait_objects, &hEvents[0], FALSE, system_wait_timeout);
	if (WAIT_TIMEOUT == dwResult){
		return SANG_STATUS_APIPOLL_TIMEOUT;
	}

	if( dwResult >= (DWORD)number_of_sangoma_wait_objects ) {
		return SANG_STATUS_GENERAL_ERROR;
	}

	/* WaitForMultipleObjects() was waken by a Sangoma or by a non-Sangoma wait object. */
	err = get_out_flags(sng_wait_objects,
						dwResult, /* Array index of the signalled object with the smallest index
								   * value of all the signalled objects, from array. */ 
						in_flags, out_flags, number_of_sangoma_wait_objects, NULL);
	if(SANG_ERROR(err)){
		return err;
	}

	return SANG_STATUS_SUCCESS;
#else
	struct pollfd pfds[number_of_sangoma_wait_objects*2]; /* we need twice as many polls because of the sangoma signalable objects */
	char dummy_buf[1];
	int res;
	j = 0;

	memset(pfds, 0, sizeof(pfds));

	for(i = 0; i < number_of_sangoma_wait_objects; i++){

		if (SANGOMA_OBJ_HAS_DEVICE(sng_wait_objects[i])) {
			pfds[i].fd = sng_wait_objects[i]->fd;
			pfds[i].events = in_flags[i];
		}

		if (SANGOMA_OBJ_IS_SIGNALABLE(sng_wait_objects[i])) {
			pfds[number_of_sangoma_wait_objects+j].fd = sng_wait_objects[i]->signal_read_fd;
			pfds[number_of_sangoma_wait_objects+j].events = POLLIN;
			j++;
		}
	}

	poll_try_again:

	res = poll(pfds, (number_of_sangoma_wait_objects + j), system_wait_timeout);
	if (res > 0) {
		for(i = 0; i < number_of_sangoma_wait_objects; i++){
			out_flags[i] = pfds[i].revents;
      /* POLLIN, POLLOUT, POLLPRI have same values as SANG_WAIT_OBJ_ HAS_INPUT, CAN_OUTPUT and HAS_EVENTS, no need to set them */
		}
		for(i = 0; i < j; i++){
			if (pfds[number_of_sangoma_wait_objects+i].revents & POLLIN) {
				/* read and discard the signal byte  */
				read(pfds[number_of_sangoma_wait_objects+i].fd, &dummy_buf, 1);
				/* set the proper flag so users may know this object was signaled */
				out_flags[i] |= SANG_WAIT_OBJ_IS_SIGNALED;
			}
		}
	} else if (res < 0 && errno == EINTR) {
		/* TODO: decrement system_wait_timeout */
		goto poll_try_again;
	}
	if (res < 0) {
		return SANG_STATUS_GENERAL_ERROR;
	}
	if (res == 0) {
		return SANG_STATUS_APIPOLL_TIMEOUT;
	}
	return SANG_STATUS_SUCCESS;
#endif
}


/*!
  \fn sangoma_status_t _LIBSNG_CALL sangoma_waitfor(sangoma_wait_obj_t *sangoma_wait_obj, int32_t inflags, int32_t *outflags, int32_t timeout)
  \brief Wait for a single waitable object
  \param sangoma_wait_obj pointer to a wait object previously created with sangoma_wait_obj_create
  \param timeout timeout in miliseconds in case of no event
  \return SANG_STATUS_APIPOLL_TIMEOUT: timeout, SANG_STATUS_SUCCESS: event occured use sangoma_wait_obj_get_out_flags to check or error status
*/
sangoma_status_t _LIBSNG_CALL sangoma_waitfor(sangoma_wait_obj_t *sangoma_wait_obj, uint32_t inflags, uint32_t *outflags, int32_t timeout)
{
	return sangoma_waitfor_many(&sangoma_wait_obj, &inflags, outflags, 1, timeout);
}


/************************************************************//**
 * Device OPEN / CLOSE Functions
 ***************************************************************/


int _LIBSNG_CALL sangoma_span_chan_toif(int span, int chan, char *interface_name)
{
#if defined(__WINDOWS__)
	/* Form the Interface Name from span and chan number (i.e. wanpipe1_if1). */
	sprintf(interface_name, WP_INTERFACE_NAME_FORM, span, chan);
#else
 	sprintf(interface_name,"s%ic%i",span,chan);
#endif
	return 0;
}

int _LIBSNG_CALL sangoma_interface_toi(char *interface_name, int *span, int *chan)
{
	char *p=NULL, *sp = NULL, *ch = NULL;
	int ret = 0;
	char data[FNAME_LEN];

	strncpy(data, interface_name, FNAME_LEN);
	if ((data[0])) {
		for (p = data; *p; p++) {
			if (sp && *p == 'g') {
				*p = '\0';
				ch = (p + 1);
				break;
			} else if (*p == 'w') {
				sp = (p + 1);
			}
		}

		if(ch && sp) {
			*span = atoi(sp);
			*chan = atoi(ch);
			ret = 1;
		} else {
			*span = -1;
			*chan = -1;
		}
	}

	return ret;
}

int _LIBSNG_CALL sangoma_interface_wait_up(int span, int chan, int sectimeout)
{
#if defined(__WINDOWS__)
  /* Windows does not need to wait for interfaces to come up */
  return 0;
#else
	char interface_name[FNAME_LEN];
  struct stat statbuf;
  struct timeval endtime = {0,0};
  struct timeval curtime = {0,0};
  int counter;
  int rc;
  if (sectimeout >= 0 && gettimeofday(&endtime, NULL)) {
    return -1;
  }
  snprintf(interface_name, sizeof(interface_name), "/dev/" WP_INTERFACE_NAME_FORM, span, chan);
  endtime.tv_sec += sectimeout;
  do {
    counter = 0;
    while ((rc = stat(interface_name, &statbuf)) && errno == ENOENT && counter != 10) {
      poll(0, 0, 100); // test in 100ms increments
      counter++;
    }
    if (!rc || errno != ENOENT) break;
    if (gettimeofday(&curtime, NULL)) {
      return -1;
    }
  } while (sectimeout < 0 || timercmp(&endtime, &curtime,>));
  return rc;
#endif
}

int _LIBSNG_CALL sangoma_span_chan_fromif(char *interface_name, int *span, int *chan)
{
	char *p = NULL, *sp = NULL, *ch = NULL;
	int ret = 0;
	char data[FNAME_LEN];

	/* Windows: Accept WANPIPEx_IFy or wanpipex_ify
	 * where x is the span and y is the chan. */

	strncpy(data, interface_name, FNAME_LEN);
	if ((data[0])) {
		for (p = data; *p; p++) {
#if defined(__WINDOWS__)
			if (sp && (*p == 'F'||*p == 'f')) {
#else
			if (sp && *p == 'c') {
#endif
				*p = '\0';
				ch = (p + 1);
				break;
#if defined(__WINDOWS__)
			} else if (*p == 'E'||*p == 'e') {
#else
			} else if (*p == 's') {
#endif
				sp = (p + 1);
			}
		}

		if(ch && sp) {
			*span = atoi(sp);
			*chan = atoi(ch);
			ret = 1;
		} else {
			*span = -1;
			*chan = -1;
		}
	}

	return ret;
}

sng_fd_t _LIBSNG_CALL sangoma_open_api_span_chan(int span, int chan) 
{
	sng_fd_t fd = INVALID_HANDLE_VALUE;
	wanpipe_api_t tdm_api;
	int err;

	fd = __sangoma_open_api_span_chan(span, chan);

#if defined(__WINDOWS__)
	if(fd == INVALID_HANDLE_VALUE){
		return fd;
	}
#else
	if (fd < 0) {
		return fd;
	}
#endif

	memset(&tdm_api,0,sizeof(tdm_api));
	tdm_api.wp_cmd.cmd = WP_API_CMD_OPEN_CNT;
	err=sangoma_cmd_exec(fd,&tdm_api);
	if (err){
		sangoma_close(&fd);
		return fd;
	}

	if (tdm_api.wp_cmd.open_cnt > 1) {
		/* this is NOT the first open request for this span/chan */
		sangoma_close(&fd);
		fd = INVALID_HANDLE_VALUE;/* fd is NOT valid anymore */
	}

	return fd;
}            

sng_fd_t _LIBSNG_CALL sangoma_open_dev_by_name(const char *dev_name)
{
   	char fname[FNAME_LEN];

#if defined(__WINDOWS__)
	_snprintf(fname , FNAME_LEN, "\\\\.\\%s", dev_name);

	return CreateFile(	fname, 
						GENERIC_READ | GENERIC_WRITE, 
						FILE_SHARE_READ | FILE_SHARE_WRITE,
						(LPSECURITY_ATTRIBUTES)NULL, 
						OPEN_EXISTING,
						FILE_FLAG_NO_BUFFERING | FILE_FLAG_WRITE_THROUGH,
						(HANDLE)NULL
						);
#else
	sprintf(fname,"/dev/%s", dev_name);

	return open(fname, O_RDWR);
#endif
}

/* no checks done for multiple open */
sng_fd_t _LIBSNG_CALL __sangoma_open_api_span_chan(int span, int chan)
{
   	char tmp_fname[FNAME_LEN];

	/* Form the Interface Name from span and chan number (i.e. wanpipe1_if1). */
	_snprintf(tmp_fname, DEV_NAME_LEN, WP_INTERFACE_NAME_FORM, span, chan);

	return sangoma_open_dev_by_name(tmp_fname);
}            

sng_fd_t _LIBSNG_CALL sangoma_open_api_ctrl(void)
{
	return sangoma_open_dev_by_name(WP_CTRL_DEV_NAME);
}

#ifdef WP_API_FEATURE_LOGGER
sng_fd_t _LIBSNG_CALL sangoma_logger_open(void)
{
	return sangoma_open_dev_by_name(WP_LOGGER_DEV_NAME);
}
#endif

int _LIBSNG_CALL sangoma_get_open_cnt(sng_fd_t fd, wanpipe_api_t *tdm_api)
{
	int err;

	WANPIPE_API_INIT_CHAN(tdm_api, 0);
	SANGOMA_INIT_TDM_API_CMD_RESULT(*tdm_api);
	tdm_api->wp_cmd.cmd = WP_API_CMD_OPEN_CNT;

	err=sangoma_cmd_exec(fd,tdm_api);
	if (err){
		return -1;
	}

	return tdm_api->wp_cmd.open_cnt;
}

sng_fd_t _LIBSNG_CALL sangoma_create_socket_by_name(char *device, char *card) 
{
	int span,chan;
	sangoma_interface_toi(device,&span,&chan);
	
	return sangoma_open_api_span_chan(span,chan);
}

          
sng_fd_t _LIBSNG_CALL sangoma_open_api_span(int span) 
{
    int i=0;
	sng_fd_t fd = INVALID_HANDLE_VALUE;

	for(i = 1; i < 32; i++){

		fd = sangoma_open_api_span_chan(span, i);

#if defined(__WINDOWS__)
		if(fd != INVALID_HANDLE_VALUE){
#else
		if (fd >= 0) {
#endif

			/* found free chan */
			break;
		}

	}/*for()*/

    return fd;  
}


/*!
  \fn void sangoma_close(sng_fd_t *fd)
  \brief Close device file descriptor
  \param fd device file descriptor
  \return void
*/
void _LIBSNG_CALL sangoma_close(sng_fd_t *fd)
{
	if (!fd) {
		return;
	}
#if defined(__WINDOWS__)
	if (*fd != INVALID_HANDLE_VALUE){
		CloseHandle(*fd);
		*fd = INVALID_HANDLE_VALUE;
	}
#else
	if (*fd >= 0) {
		close(*fd);
		*fd = -1;
	}
#endif
}



/************************************************************//**
 * Device READ / WRITE Functions
 ***************************************************************/ 

int _LIBSNG_CALL sangoma_readmsg(sng_fd_t fd, void *hdrbuf, int hdrlen, void *databuf, int datalen, int flag)
{
	int rx_len=0;

#if defined(__WINDOWS__)
	wp_api_hdr_t	*rx_hdr = (wp_api_hdr_t*)hdrbuf;
	wan_iovec_list_t iovec_list;

	if (hdrlen != sizeof(wp_api_hdr_t)) {
		/*error*/
		DBG_ERR("hdrlen (%i) != sizeof(wp_api_hdr_t) (%i)\n", hdrlen, sizeof(wp_api_hdr_t));
		return -1;
	}

	memset(&iovec_list, 0x00, sizeof(iovec_list));

	iovec_list.iovec_list[0].iov_base = hdrbuf;
	iovec_list.iovec_list[0].iov_len = hdrlen;

	iovec_list.iovec_list[1].iov_base = databuf;
	iovec_list.iovec_list[1].iov_len = datalen;

	if (DoReadCommand(fd, &iovec_list)) {
		/*error*/
		DBG_ERR("DoReadCommand() failed! Check messages log.\n");
		return -4;
	}

	switch(rx_hdr->operation_status)
	{
	case SANG_STATUS_RX_DATA_AVAILABLE:
		/* ok */
		break;
	case SANG_STATUS_NO_DATA_AVAILABLE:
		/* Note that SANG_STATUS_NO_DATA_AVAILABLE is NOT an error becase
		 * read() is non-blocking and can be called at any time (by some polling code)*/
		return 1; /* return positive value to indicate IOCTL success, user must check 'rx_hdr->operation_status' */
	default:
		if(libsng_dbg_level)DBG_ERR("Operation Status: %s(%d)\n",
			SDLA_DECODE_SANG_STATUS(rx_hdr->operation_status), rx_hdr->operation_status);
		return -5;
	}

	rx_len = rx_hdr->data_length;
#else
	wan_msghdr_t msg;
	wan_iovec_t iov[2];

	memset(&msg,0,sizeof(msg));
	memset(&iov[0],0,sizeof(iov[0])*2);

	iov[0].iov_len=hdrlen;
	iov[0].iov_base=hdrbuf;

	iov[1].iov_len=datalen;
	iov[1].iov_base=databuf;

	msg.msg_iovlen=2;
	msg.msg_iov=iov;

	rx_len = read(fd,&msg,sizeof(msg));

	if (rx_len <= sizeof(wp_api_hdr_t)){
		return -EINVAL;
	}

	rx_len -= sizeof(wp_api_hdr_t);
#endif
    return rx_len;
}                    

int _LIBSNG_CALL sangoma_writemsg(sng_fd_t fd, void *hdrbuf, int hdrlen, void *databuf, unsigned short datalen, int flag)
{
	int bsent=-1;
	wp_api_hdr_t *wp_api_hdr = hdrbuf;
#if defined(__WINDOWS__)
	wan_iovec_list_t iovec_list;
#endif

	if (hdrlen != sizeof(wp_api_hdr_t)) {
		/* error. Possible cause is a mismatch between versions of API header files. */
		DBG_ERR("hdrlen (%i) != sizeof(wp_api_hdr_t) (%i)\n", hdrlen, sizeof(wp_api_hdr_t));
		return -1;
	}

#if defined(__WINDOWS__)

	wp_api_hdr->data_length = datalen;

	memset(&iovec_list, 0x00, sizeof(iovec_list));

	iovec_list.iovec_list[0].iov_base = hdrbuf;
	iovec_list.iovec_list[0].iov_len = hdrlen;

	iovec_list.iovec_list[1].iov_base = databuf;
	iovec_list.iovec_list[1].iov_len = datalen;

	/* queue data for transmission */
	if (DoWriteCommand(fd, &iovec_list)) {
		/*error*/
		DBG_ERR("DoWriteCommand() failed!! Check messages log.\n");
		return -1;
	}

	bsent=0;
	/*check that frame was transmitted*/
	switch(wp_api_hdr->operation_status)
	{
	case SANG_STATUS_SUCCESS:
		bsent = datalen;
		break;
	default:
		DBG_ERR("Operation Status: %s(%d)\n",
			SDLA_DECODE_SANG_STATUS(wp_api_hdr->operation_status), wp_api_hdr->operation_status);
		break;
	}/*switch()*/
#else
	wan_msghdr_t msg;
	wan_iovec_t iov[2];
	
	memset(&msg,0,sizeof(msg));
	memset(&iov[0],0,sizeof(iov[0])*2);

	iov[0].iov_len=hdrlen;
	iov[0].iov_base=hdrbuf;

	iov[1].iov_len=datalen;
	iov[1].iov_base=databuf;

	msg.msg_iovlen=2;
	msg.msg_iov=iov;

	bsent = write(fd,&msg,sizeof(msg));

	if (bsent == (datalen+hdrlen)){
		wp_api_hdr->wp_api_hdr_operation_status=SANG_STATUS_SUCCESS;
		bsent-=sizeof(wp_api_hdr_t);
	} else if (errno == EBUSY){
		wp_api_hdr->wp_api_hdr_operation_status=SANG_STATUS_DEVICE_BUSY;
	} else {
		wp_api_hdr->wp_api_hdr_operation_status=SANG_STATUS_IO_ERROR;
	}
	wp_api_hdr->wp_api_hdr_data_length=bsent;

#endif
	return bsent;
}


#ifdef WANPIPE_TDM_API


/************************************************************//**
 * Device API COMMAND Functions
 ***************************************************************/ 



/*========================================================
 * Execute TDM command
 *
 */
int _LIBSNG_CALL sangoma_cmd_exec(sng_fd_t fd, wanpipe_api_t *tdm_api)
{
	int err;

#if defined(__WINDOWS__)
	err = tdmv_api_ioctl(fd, &tdm_api->wp_cmd);
#else
	err = ioctl(fd,WANPIPE_IOCTL_API_CMD,&tdm_api->wp_cmd);
	if (err < 0){
		char tmp[50];
		sprintf(tmp,"TDM API: CMD: %i\n",tdm_api->wp_cmd.cmd);
		perror(tmp);
		return -1;
	}
#endif
	return err;
}

/*========================================================
 * Get Full TDM API configuration per channel
 *
 */
int _LIBSNG_CALL sangoma_get_full_cfg(sng_fd_t fd, wanpipe_api_t *tdm_api)
{
	int err;

	WANPIPE_API_INIT_CHAN(tdm_api, 0);
	SANGOMA_INIT_TDM_API_CMD_RESULT(*tdm_api);
	tdm_api->wp_cmd.cmd = WP_API_CMD_GET_FULL_CFG;

	err=sangoma_cmd_exec(fd,tdm_api);
	if (err){
		return err;
	}

#if 0
	printf("TDM API CFG:\n");
	printf("\thw_tdm_coding:\t%d\n",tdm_api->wp_cmd.hw_tdm_coding);
	printf("\thw_mtu_mru:\t%d\n",tdm_api->wp_cmd.hw_mtu_mru);
	printf("\tusr_period:\t%d\n",tdm_api->wp_cmd.usr_period);
	printf("\ttdm_codec:\t%d\n",tdm_api->wp_cmd.tdm_codec);
	printf("\tpower_level:\t%d\n",tdm_api->wp_cmd.power_level);
	printf("\trx_disable:\t%d\n",tdm_api->wp_cmd.rx_disable);
	printf("\ttx_disable:\t%d\n",tdm_api->wp_cmd.tx_disable);
	printf("\tusr_mtu_mru:\t%d\n",tdm_api->wp_cmd.usr_mtu_mru);
	printf("\tidle flag:\t0x%02X\n",tdm_api->wp_cmd.idle_flag);

#ifdef WP_API_FEATURE_FE_ALARM
	printf("\tfe alarms:\t0x%02X\n",tdm_api->wp_cmd.fe_alarms);
#endif
	
	printf("\trx pkt\t%d\ttx pkt\t%d\n",tdm_api->wp_cmd.stats.rx_packets,
				tdm_api->wp_cmd.stats.tx_packets);
	printf("\trx err\t%d\ttx err\t%d\n",
				tdm_api->wp_cmd.stats.rx_errors,
				tdm_api->wp_cmd.stats.tx_errors);
#ifndef __WINDOWS__
	printf("\trx ovr\t%d\ttx idl\t%d\n",
				tdm_api->wp_cmd.stats.rx_fifo_errors,
				tdm_api->wp_cmd.stats.tx_carrier_errors);
#endif		
#endif		
	
	return 0;
}

/*========================================================
 * SET Codec on a particular Channel.
 * 
 * Available codecs are defined in 
 * /usr/src/linux/include/linux/wanpipe_cfg.h
 * 
 * enum wan_codec_format {
 *  	WP_NONE,
 *	WP_SLINEAR
 * }
 *
 */
int _LIBSNG_CALL sangoma_tdm_set_codec(sng_fd_t fd, wanpipe_api_t *tdm_api, int codec)
{
	WANPIPE_API_INIT_CHAN(tdm_api, 0);
	SANGOMA_INIT_TDM_API_CMD_RESULT(*tdm_api);
	tdm_api->wp_cmd.cmd = WP_API_CMD_SET_CODEC;
	tdm_api->wp_cmd.tdm_codec = codec;
	return sangoma_cmd_exec(fd,tdm_api);
}

/*========================================================
 * GET Codec from a particular Channel.
 * 
 * Available codecs are defined in 
 * /usr/src/linux/include/linux/wanpipe_cfg.h
 * 
 * enum wan_codec_format {
 *  	WP_NONE,
 *	WP_SLINEAR
 * }
 *
 */
int _LIBSNG_CALL sangoma_tdm_get_codec(sng_fd_t fd, wanpipe_api_t *tdm_api)
{
	int err;

	WANPIPE_API_INIT_CHAN(tdm_api, 0);
	SANGOMA_INIT_TDM_API_CMD_RESULT(*tdm_api);
	tdm_api->wp_cmd.cmd = WP_API_CMD_GET_CODEC;

	err=sangoma_cmd_exec(fd,tdm_api);
	if (err){
		return err;
	}

	return tdm_api->wp_cmd.tdm_codec;	
}

/*========================================================
 * SET Rx/Tx Hardware Period in milliseconds.
 * 
 * Available options are:
 *  10,20,30,40,50 ms      
 *
 */
int _LIBSNG_CALL sangoma_tdm_set_usr_period(sng_fd_t fd, wanpipe_api_t *tdm_api, int period)
{
	WANPIPE_API_INIT_CHAN(tdm_api, 0);
	SANGOMA_INIT_TDM_API_CMD_RESULT(*tdm_api);
	tdm_api->wp_cmd.cmd = WP_API_CMD_SET_USR_PERIOD;
	tdm_api->wp_cmd.usr_period = period;
	return sangoma_cmd_exec(fd,tdm_api);
}

/*========================================================
 * GET Rx/Tx Hardware Period in milliseconds.
 * 
 * Available options are:
 *  10,20,30,40,50 ms      
 *
 */
int _LIBSNG_CALL sangoma_tdm_get_usr_period(sng_fd_t fd, wanpipe_api_t *tdm_api)
{
	int err;

	WANPIPE_API_INIT_CHAN(tdm_api, 0);
	SANGOMA_INIT_TDM_API_CMD_RESULT(*tdm_api);
	tdm_api->wp_cmd.cmd = WP_API_CMD_GET_USR_PERIOD;

	err=sangoma_cmd_exec(fd,tdm_api);
	if (err){
		return err;
	}

	return tdm_api->wp_cmd.usr_period;
}

/*========================================================
 * GET Current User Hardware Coding Format
 *
 * Coding Format will be ULAW/ALAW based on T1/E1 
 */

int _LIBSNG_CALL sangoma_get_hw_coding(sng_fd_t fd, wanpipe_api_t *tdm_api)
{
	int err;

	WANPIPE_API_INIT_CHAN(tdm_api, 0);
	SANGOMA_INIT_TDM_API_CMD_RESULT(*tdm_api);
	tdm_api->wp_cmd.cmd = WP_API_CMD_GET_HW_CODING;
	err=sangoma_cmd_exec(fd,tdm_api);
	if (err){
		return err;
	}
	return tdm_api->wp_cmd.hw_tdm_coding;
}

#ifdef WP_API_FEATURE_DTMF_EVENTS
/*========================================================
 * GET Current User Hardware DTMF Enabled/Disabled
 *
 * Will return true if HW DTMF is enabled on Octasic
 */

int _LIBSNG_CALL sangoma_tdm_get_hw_dtmf(sng_fd_t fd, wanpipe_api_t *tdm_api)
{
	int err;

	WANPIPE_API_INIT_CHAN(tdm_api, 0);
	SANGOMA_INIT_TDM_API_CMD_RESULT(*tdm_api);
	tdm_api->wp_cmd.cmd = WP_API_CMD_GET_HW_DTMF;
	err=sangoma_cmd_exec(fd,tdm_api);
	if (err){
		return err;
	}
	return tdm_api->wp_cmd.hw_dtmf;
}

/*========================================================
 * GET status of echo canceler chip. 
 *
 * Will return true if HW EC is available 
 */

int _LIBSNG_CALL sangoma_tdm_get_hw_ec(sng_fd_t fd, wanpipe_api_t *tdm_api)
{
	int err;

	WANPIPE_API_INIT_CHAN(tdm_api, 0);
	SANGOMA_INIT_TDM_API_CMD_RESULT(*tdm_api);
	tdm_api->wp_cmd.cmd = WP_API_CMD_GET_HW_EC;
	err=sangoma_cmd_exec(fd,tdm_api);
	if (err){
		return err;
	}
	return tdm_api->wp_cmd.hw_ec;
}
#endif


#ifdef WP_API_FEATURE_EC_CHAN_STAT
/*========================================================
 * GET status of the echo canceler for current channel.
 *
 * Will return true if HW EC is enabled 
 */

int _LIBSNG_CALL sangoma_tdm_get_hwec_chan_status(sng_fd_t fd, wanpipe_api_t *tdm_api)
{
	int err;

	WANPIPE_API_INIT_CHAN(tdm_api, 0);
	SANGOMA_INIT_TDM_API_CMD_RESULT(*tdm_api);
	tdm_api->wp_cmd.cmd = WP_API_CMD_GET_HW_EC_CHAN;
	err=sangoma_cmd_exec(fd,tdm_api);
	if (err){
		return err;
	}
	return tdm_api->wp_cmd.hw_ec;
}

#endif

#ifdef WP_API_FEATURE_HWEC_PERSIST
/*========================================================
 * Check if hwec persist is enabled or disabled.
 * If persist is on: then hwec will always stay enabled for
 *                   all channes.
 * If persist is off: hwec will not be enabled by default.
 *
 * Will return true if HW EC is persist mode is on 
 */

int _LIBSNG_CALL sangoma_tdm_get_hwec_persist_status(sng_fd_t fd, wanpipe_api_t *tdm_api)
{
	int err;

	WANPIPE_API_INIT_CHAN(tdm_api, 0);
	SANGOMA_INIT_TDM_API_CMD_RESULT(*tdm_api);
	tdm_api->wp_cmd.cmd = WP_API_CMD_GET_HW_EC_PERSIST;
	err=sangoma_cmd_exec(fd,tdm_api);
	if (err){
		return err;
	}
	return tdm_api->wp_cmd.hw_ec;
}     
#endif


/*========================================================
 * GET Current User MTU/MRU values in bytes.
 * 
 * The USER MTU/MRU values will change each time a PERIOD
 * or CODEC is adjusted.
 */
int _LIBSNG_CALL sangoma_tdm_get_usr_mtu_mru(sng_fd_t fd, wanpipe_api_t *tdm_api)
{
	int err;

	WANPIPE_API_INIT_CHAN(tdm_api, 0);
	SANGOMA_INIT_TDM_API_CMD_RESULT(*tdm_api);
	tdm_api->wp_cmd.cmd = WP_API_CMD_GET_USR_MTU_MRU;

	err=sangoma_cmd_exec(fd,tdm_api);
	if (err){
		return err;
	}

	return tdm_api->wp_cmd.usr_mtu_mru;
}

/*========================================================
 * SET TDM Power Level
 * 
 * This option is not implemented yet
 *
 */
int _LIBSNG_CALL sangoma_tdm_set_power_level(sng_fd_t fd, wanpipe_api_t *tdm_api, int power)
{
	WANPIPE_API_INIT_CHAN(tdm_api, 0);
	SANGOMA_INIT_TDM_API_CMD_RESULT(*tdm_api);
	tdm_api->wp_cmd.cmd = WP_API_CMD_SET_POWER_LEVEL;
	tdm_api->wp_cmd.power_level = power;
	return sangoma_cmd_exec(fd,tdm_api);
}

/*========================================================
 * GET TDM Power Level
 * 
 * This option is not implemented yet
 *
 */
int _LIBSNG_CALL sangoma_tdm_get_power_level(sng_fd_t fd, wanpipe_api_t *tdm_api)
{
	int err;

	WANPIPE_API_INIT_CHAN(tdm_api, 0);
	SANGOMA_INIT_TDM_API_CMD_RESULT(*tdm_api);
	tdm_api->wp_cmd.cmd = WP_API_CMD_GET_POWER_LEVEL;

	err=sangoma_cmd_exec(fd,tdm_api);
	if (err){
		return err;
	}

	return tdm_api->wp_cmd.power_level;
}

int _LIBSNG_CALL sangoma_flush_bufs(sng_fd_t fd, wanpipe_api_t *tdm_api)
{
	WANPIPE_API_INIT_CHAN(tdm_api, 0);
	SANGOMA_INIT_TDM_API_CMD_RESULT(*tdm_api);
	tdm_api->wp_cmd.cmd = WP_API_CMD_FLUSH_BUFFERS;
	return sangoma_cmd_exec(fd,tdm_api);
}

int _LIBSNG_CALL sangoma_flush_rx_bufs(sng_fd_t fd, wanpipe_api_t *tdm_api)
{
	WANPIPE_API_INIT_CHAN(tdm_api, 0);
	SANGOMA_INIT_TDM_API_CMD_RESULT(*tdm_api);
	tdm_api->wp_cmd.cmd = WP_API_CMD_FLUSH_RX_BUFFERS;
	return sangoma_cmd_exec(fd,tdm_api);
}

int _LIBSNG_CALL sangoma_flush_tx_bufs(sng_fd_t fd, wanpipe_api_t *tdm_api)
{
	WANPIPE_API_INIT_CHAN(tdm_api, 0);
	SANGOMA_INIT_TDM_API_CMD_RESULT(*tdm_api);
	tdm_api->wp_cmd.cmd = WP_API_CMD_FLUSH_TX_BUFFERS;
	return sangoma_cmd_exec(fd,tdm_api);
} 

int _LIBSNG_CALL sangoma_flush_event_bufs(sng_fd_t fd, wanpipe_api_t *tdm_api)
{
	WANPIPE_API_INIT_CHAN(tdm_api, 0);
	SANGOMA_INIT_TDM_API_CMD_RESULT(*tdm_api);
	tdm_api->wp_cmd.cmd = WP_API_CMD_FLUSH_EVENT_BUFFERS;
	return sangoma_cmd_exec(fd,tdm_api);
}  

int _LIBSNG_CALL sangoma_tdm_enable_rbs_events(sng_fd_t fd, wanpipe_api_t *tdm_api, int poll_in_sec) 
{
	WANPIPE_API_INIT_CHAN(tdm_api, 0);
	SANGOMA_INIT_TDM_API_CMD_RESULT(*tdm_api);
	tdm_api->wp_cmd.cmd = WP_API_CMD_ENABLE_RBS_EVENTS;
	tdm_api->wp_cmd.rbs_poll = poll_in_sec;
	return sangoma_cmd_exec(fd,tdm_api);
}

int _LIBSNG_CALL sangoma_tdm_disable_rbs_events(sng_fd_t fd, wanpipe_api_t *tdm_api) {

	WANPIPE_API_INIT_CHAN(tdm_api, 0);
	SANGOMA_INIT_TDM_API_CMD_RESULT(*tdm_api);
	tdm_api->wp_cmd.cmd = WP_API_CMD_DISABLE_RBS_EVENTS;
	return sangoma_cmd_exec(fd,tdm_api);
}

int _LIBSNG_CALL sangoma_tdm_write_rbs(sng_fd_t fd, wanpipe_api_t *tdm_api, int channel, unsigned char rbs)
{
	WANPIPE_API_INIT_CHAN(tdm_api, channel);
	SANGOMA_INIT_TDM_API_CMD_RESULT(*tdm_api);
	tdm_api->wp_cmd.cmd = WP_API_CMD_WRITE_RBS_BITS;
	tdm_api->wp_cmd.rbs_tx_bits=rbs;
	return sangoma_cmd_exec(fd,tdm_api);
}        

int _LIBSNG_CALL sangoma_tdm_read_rbs(sng_fd_t fd, wanpipe_api_t *tdm_api, int channel, unsigned char *rbs)
{
	int err;
	WANPIPE_API_INIT_CHAN(tdm_api, channel);
	SANGOMA_INIT_TDM_API_CMD_RESULT(*tdm_api);
	tdm_api->wp_cmd.cmd = WP_API_CMD_READ_RBS_BITS;
	tdm_api->wp_cmd.rbs_tx_bits=0;
	
	err=sangoma_cmd_exec(fd,tdm_api);
	if (err){
		return err;
	}

	*rbs=(unsigned char)tdm_api->wp_cmd.rbs_rx_bits;
	return 0;
}

#ifdef WP_API_FEATURE_BUFFER_MULT

int _LIBSNG_CALL sangoma_tdm_set_buffer_multiplier(sng_fd_t fd, wanpipe_api_t *tdm_api, unsigned int multiplier)
{
	
	int err;
	WANPIPE_API_INIT_CHAN(tdm_api, 0);
	SANGOMA_INIT_TDM_API_CMD_RESULT(*tdm_api);
	tdm_api->wp_cmd.cmd = WP_API_CMD_BUFFER_MULTIPLIER;
	*((unsigned int*)&tdm_api->wp_cmd.data[0]) = multiplier;
	
	err=sangoma_cmd_exec(fd,tdm_api);
	if (err){
		return err;
	}

	return 0;
}

#endif


int _LIBSNG_CALL sangoma_read_event(sng_fd_t fd, wanpipe_api_t *tdm_api)
{

#ifdef WP_API_FEATURE_EVENTS
	wp_api_event_t *rx_event;
	int err;

	WANPIPE_API_INIT_CHAN(tdm_api, 0);
	SANGOMA_INIT_TDM_API_CMD_RESULT(*tdm_api);
	tdm_api->wp_cmd.cmd = WP_API_CMD_READ_EVENT;
	
	err=sangoma_cmd_exec(fd,tdm_api);
	if (err){
		return err;
	}

	rx_event = &tdm_api->wp_cmd.event;

#ifdef WP_API_DEPRECATED_FEATURE_READ_CALLBACK_FUNCTIONS
   	/*
	 The use of callbacks here is purely optional and is left
     here for backward compatibility purposes.  By default user
 	 should handle events outside this funciton. This function
	 should only be used to read the event
    */

	switch (rx_event->wp_api_event_type){
	
	case WP_API_EVENT_RBS:
		if (tdm_api->wp_callback.wp_rbs_event) {
			tdm_api->wp_callback.wp_rbs_event(fd,rx_event->wp_api_event_rbs_bits);
		}
		break;
	
#ifdef WP_API_FEATURE_DTMF_EVENTS
	case WP_API_EVENT_DTMF:
		if (tdm_api->wp_callback.wp_dtmf_event) {
			tdm_api->wp_callback.wp_dtmf_event(fd,
						rx_event->wp_api_event_dtmf_digit,
						rx_event->wp_api_event_dtmf_type,
						rx_event->wp_api_event_dtmf_port);
		}
		break;
#endif
		
	case WP_API_EVENT_RXHOOK:
		if (tdm_api->wp_callback.wp_rxhook_event) {
			tdm_api->wp_callback.wp_rxhook_event(fd,
						rx_event->wp_api_event_hook_state);
		}
		break;

	case WP_API_EVENT_RING_DETECT:
		if (tdm_api->wp_callback.wp_ring_detect_event) {
			tdm_api->wp_callback.wp_ring_detect_event(fd,
						rx_event->wp_api_event_ring_state);
		}
		break;

	case WP_API_EVENT_RING_TRIP_DETECT:
		if (tdm_api->wp_callback.wp_ring_trip_detect_event) {
			tdm_api->wp_callback.wp_ring_trip_detect_event(fd,
						rx_event->wp_api_event_ring_state);
		}
		break;

#ifdef WP_API_FEATURE_FE_ALARM
	case WP_API_EVENT_ALARM:
		if (tdm_api->wp_callback.wp_fe_alarm_event) {
			tdm_api->wp_callback.wp_fe_alarm_event(fd,
						rx_event->wp_api_event_alarm);
		}   
		break; 
#endif

#ifdef WP_API_FEATURE_LINK_STATUS
	/* Link Status */	
	case WP_API_EVENT_LINK_STATUS:
		if(tdm_api->wp_callback.wp_link_status_event){
			tdm_api->wp_callback.wp_link_status_event(fd,
						rx_event->wp_api_event_link_status);
		}
		
		break;
#endif

#ifdef WP_API_FEATURE_POL_REV
    case WP_API_EVENT_POLARITY_REVERSE:
        break;
#endif	
	default:
#ifdef __WINDOWS__
		if(0)printf("Warning: libsangoma: %s fd=0x%p: Unknown TDM event!", __FUNCTION__,fd);
#else
		if(0)printf("Warning: libsangoma: %s fd=%d: Unknown TDM event!", __FUNCTION__, fd);
#endif
		break;
	}

#endif

	
	return 0;
#else
	printf("Error: Read Event not supported!\n");
	return -1;
#endif
}  


#ifdef WP_API_FEATURE_LOGGER
/*========================================================
 * Execute Wanpipe Logger command
 */
sangoma_status_t _LIBSNG_CALL sangoma_logger_cmd_exec(sng_fd_t fd, wp_logger_cmd_t *logger_cmd)
{
#if defined(__WINDOWS__)
	return logger_api_ioctl(fd, logger_cmd);
#else
	int err = ioctl(fd,WANPIPE_IOCTL_LOGGER_CMD,logger_cmd);
	if (err < 0){
		char tmp[50];
		sprintf(tmp,"Logger CMD: %i\n",logger_cmd->cmd);
		perror(tmp);
		return -1;
	}
	return err;
#endif
}

sangoma_status_t _LIBSNG_CALL sangoma_logger_read_event(sng_fd_t fd, wp_logger_cmd_t *logger_cmd)
{
	logger_cmd->cmd = WP_API_LOGGER_CMD_READ_EVENT;
	return sangoma_logger_cmd_exec(fd, logger_cmd);
}

sangoma_status_t _LIBSNG_CALL sangoma_logger_flush_buffers(sng_fd_t fd, wp_logger_cmd_t *logger_cmd)
{
	logger_cmd->cmd = WP_API_LOGGER_CMD_FLUSH_BUFFERS;
	return sangoma_logger_cmd_exec(fd, logger_cmd);
}

sangoma_status_t _LIBSNG_CALL sangoma_logger_get_statistics(sng_fd_t fd, wp_logger_cmd_t *logger_cmd)
{
	logger_cmd->cmd = WP_API_LOGGER_CMD_GET_STATS;
	return sangoma_logger_cmd_exec(fd, logger_cmd);
}

sangoma_status_t _LIBSNG_CALL sangoma_logger_reset_statistics(sng_fd_t fd, wp_logger_cmd_t *logger_cmd)
{
	logger_cmd->cmd = WP_API_LOGGER_CMD_RESET_STATS;
	return sangoma_logger_cmd_exec(fd, logger_cmd);
}

sangoma_status_t _LIBSNG_CALL sangoma_logger_get_open_handle_counter(sng_fd_t fd, wp_logger_cmd_t *logger_cmd)
{
	logger_cmd->cmd = WP_API_LOGGER_CMD_OPEN_CNT;
	return sangoma_logger_cmd_exec(fd, logger_cmd);
}

sangoma_status_t _LIBSNG_CALL sangoma_logger_get_logger_level(sng_fd_t fd, wp_logger_cmd_t *logger_cmd)
{
	logger_cmd->cmd = WP_API_LOGGER_CMD_GET_LOGGER_LEVEL;
	return sangoma_logger_cmd_exec(fd, logger_cmd);
}

sangoma_status_t _LIBSNG_CALL sangoma_logger_set_logger_level(sng_fd_t fd, wp_logger_cmd_t *logger_cmd)
{
	logger_cmd->cmd = WP_API_LOGGER_CMD_SET_LOGGER_LEVEL;
	return sangoma_logger_cmd_exec(fd, logger_cmd);
}

#endif /* WP_API_FEATURE_LOGGER */

#ifdef WP_API_FEATURE_FAX_EVENTS
int _LIBSNG_CALL sangoma_tdm_enable_fax_events(sng_fd_t fd, wanpipe_api_t *tdm_api)
{
	WANPIPE_API_INIT_CHAN(tdm_api, 0);	
	SANGOMA_INIT_TDM_API_CMD_RESULT(*tdm_api);
	tdm_api->wp_cmd.cmd = WP_API_CMD_SET_EVENT;
	tdm_api->wp_cmd.event.wp_api_event_type = WP_API_EVENT_FAX_DETECT;
	tdm_api->wp_cmd.event.wp_api_event_mode = WP_API_EVENT_ENABLE;
	return sangoma_cmd_exec(fd,tdm_api);
}

int _LIBSNG_CALL sangoma_tdm_disable_fax_events(sng_fd_t fd, wanpipe_api_t *tdm_api)
{
	WANPIPE_API_INIT_CHAN(tdm_api, 0);
	SANGOMA_INIT_TDM_API_CMD_RESULT(*tdm_api);
	tdm_api->wp_cmd.cmd = WP_API_CMD_SET_EVENT;
	tdm_api->wp_cmd.event.wp_api_event_type = WP_API_EVENT_FAX_DETECT;
	tdm_api->wp_cmd.event.wp_api_event_mode = WP_API_EVENT_DISABLE;
	return sangoma_cmd_exec(fd,tdm_api);
}

int _LIBSNG_CALL sangoma_tdm_get_hw_fax(sng_fd_t fd, wanpipe_api_t *tdm_api)
{
	int err;
	WANPIPE_API_INIT_CHAN(tdm_api, 0);
	SANGOMA_INIT_TDM_API_CMD_RESULT(*tdm_api);
	tdm_api->wp_cmd.cmd = WP_API_CMD_GET_HW_FAX_DETECT;
	err=sangoma_cmd_exec(fd,tdm_api);
	if (err){
		return err;
	}
	return tdm_api->wp_cmd.hw_fax;
}
#endif

#ifdef WP_API_FEATURE_DTMF_EVENTS
int _LIBSNG_CALL sangoma_tdm_enable_dtmf_events(sng_fd_t fd, wanpipe_api_t *tdm_api)
{
	WANPIPE_API_INIT_CHAN(tdm_api, 0);	
	SANGOMA_INIT_TDM_API_CMD_RESULT(*tdm_api);
	tdm_api->wp_cmd.cmd = WP_API_CMD_SET_EVENT;
	tdm_api->wp_cmd.event.wp_api_event_type = WP_API_EVENT_DTMF;
	tdm_api->wp_cmd.event.wp_api_event_mode = WP_API_EVENT_ENABLE;
	return sangoma_cmd_exec(fd,tdm_api);
}

int _LIBSNG_CALL sangoma_tdm_disable_dtmf_events(sng_fd_t fd, wanpipe_api_t *tdm_api)
{
	WANPIPE_API_INIT_CHAN(tdm_api, 0);
	SANGOMA_INIT_TDM_API_CMD_RESULT(*tdm_api);
	tdm_api->wp_cmd.cmd = WP_API_CMD_SET_EVENT;
	tdm_api->wp_cmd.event.wp_api_event_type = WP_API_EVENT_DTMF;
	tdm_api->wp_cmd.event.wp_api_event_mode = WP_API_EVENT_DISABLE;
	return sangoma_cmd_exec(fd,tdm_api);
}

int _LIBSNG_CALL sangoma_tdm_enable_rm_dtmf_events(sng_fd_t fd, wanpipe_api_t *tdm_api)
{
	WANPIPE_API_INIT_CHAN(tdm_api, 0);
	SANGOMA_INIT_TDM_API_CMD_RESULT(*tdm_api);
	tdm_api->wp_cmd.cmd = WP_API_CMD_SET_EVENT;
	tdm_api->wp_cmd.event.wp_api_event_type = WP_API_EVENT_RM_DTMF;
	tdm_api->wp_cmd.event.wp_api_event_mode = WP_API_EVENT_ENABLE;
	return sangoma_cmd_exec(fd,tdm_api);
}

int _LIBSNG_CALL sangoma_tdm_disable_rm_dtmf_events(sng_fd_t fd, wanpipe_api_t *tdm_api)
{
	WANPIPE_API_INIT_CHAN(tdm_api, 0);
	SANGOMA_INIT_TDM_API_CMD_RESULT(*tdm_api);
	tdm_api->wp_cmd.cmd = WP_API_CMD_SET_EVENT;
	tdm_api->wp_cmd.event.wp_api_event_type = WP_API_EVENT_RM_DTMF;
	tdm_api->wp_cmd.event.wp_api_event_mode = WP_API_EVENT_DISABLE;
	return sangoma_cmd_exec(fd,tdm_api);
}

int _LIBSNG_CALL sangoma_tdm_enable_rxhook_events(sng_fd_t fd, wanpipe_api_t *tdm_api)
{
	WANPIPE_API_INIT_CHAN(tdm_api, 0);
	SANGOMA_INIT_TDM_API_CMD_RESULT(*tdm_api);
	tdm_api->wp_cmd.cmd = WP_API_CMD_SET_EVENT;
	tdm_api->wp_cmd.event.wp_api_event_type = WP_API_EVENT_RXHOOK;
	tdm_api->wp_cmd.event.wp_api_event_mode = WP_API_EVENT_ENABLE;
	return sangoma_cmd_exec(fd,tdm_api);
}

int _LIBSNG_CALL sangoma_tdm_disable_rxhook_events(sng_fd_t fd, wanpipe_api_t *tdm_api)
{
	WANPIPE_API_INIT_CHAN(tdm_api, 0);
	SANGOMA_INIT_TDM_API_CMD_RESULT(*tdm_api);
	tdm_api->wp_cmd.cmd = WP_API_CMD_SET_EVENT;
	tdm_api->wp_cmd.event.wp_api_event_type = WP_API_EVENT_RXHOOK;
	tdm_api->wp_cmd.event.wp_api_event_mode = WP_API_EVENT_DISABLE;
	return sangoma_cmd_exec(fd,tdm_api);
}

int _LIBSNG_CALL sangoma_tdm_enable_ring_events(sng_fd_t fd, wanpipe_api_t *tdm_api) 
{
	WANPIPE_API_INIT_CHAN(tdm_api, 0);
	SANGOMA_INIT_TDM_API_CMD_RESULT(*tdm_api);
	tdm_api->wp_cmd.cmd = WP_API_CMD_SET_EVENT;
	tdm_api->wp_cmd.event.wp_api_event_type = WP_API_EVENT_RING;
	tdm_api->wp_cmd.event.wp_api_event_mode = WP_API_EVENT_ENABLE;
	return sangoma_cmd_exec(fd,tdm_api);
}

int _LIBSNG_CALL sangoma_tdm_disable_ring_events(sng_fd_t fd, wanpipe_api_t *tdm_api) 
{
	WANPIPE_API_INIT_CHAN(tdm_api, 0);
	SANGOMA_INIT_TDM_API_CMD_RESULT(*tdm_api);
	tdm_api->wp_cmd.cmd = WP_API_CMD_SET_EVENT;
	tdm_api->wp_cmd.event.wp_api_event_type = WP_API_EVENT_RING;
	tdm_api->wp_cmd.event.wp_api_event_mode = WP_API_EVENT_DISABLE;
	return sangoma_cmd_exec(fd,tdm_api);
}

int _LIBSNG_CALL sangoma_tdm_enable_ring_detect_events(sng_fd_t fd, wanpipe_api_t *tdm_api) 
{
	WANPIPE_API_INIT_CHAN(tdm_api, 0);	
	SANGOMA_INIT_TDM_API_CMD_RESULT(*tdm_api);
	tdm_api->wp_cmd.cmd = WP_API_CMD_SET_EVENT;
	tdm_api->wp_cmd.event.wp_api_event_type = WP_API_EVENT_RING_DETECT;
	tdm_api->wp_cmd.event.wp_api_event_mode = WP_API_EVENT_ENABLE;
	return sangoma_cmd_exec(fd,tdm_api);
}

int _LIBSNG_CALL sangoma_tdm_disable_ring_detect_events(sng_fd_t fd, wanpipe_api_t *tdm_api) 
{
	WANPIPE_API_INIT_CHAN(tdm_api, 0);
	SANGOMA_INIT_TDM_API_CMD_RESULT(*tdm_api);
	tdm_api->wp_cmd.cmd = WP_API_CMD_SET_EVENT;
	tdm_api->wp_cmd.event.wp_api_event_type = WP_API_EVENT_RING_DETECT;
	tdm_api->wp_cmd.event.wp_api_event_mode = WP_API_EVENT_DISABLE;
	return sangoma_cmd_exec(fd,tdm_api);
}

int _LIBSNG_CALL sangoma_tdm_enable_ring_trip_detect_events(sng_fd_t fd, wanpipe_api_t *tdm_api) 
{
	WANPIPE_API_INIT_CHAN(tdm_api, 0);
	SANGOMA_INIT_TDM_API_CMD_RESULT(*tdm_api);
	tdm_api->wp_cmd.cmd = WP_API_CMD_SET_EVENT;
	tdm_api->wp_cmd.event.wp_api_event_type = WP_API_EVENT_RING_TRIP_DETECT;
	tdm_api->wp_cmd.event.wp_api_event_mode = WP_API_EVENT_ENABLE;
	return sangoma_cmd_exec(fd,tdm_api);
}

int _LIBSNG_CALL sangoma_tdm_disable_ring_trip_detect_events(sng_fd_t fd, wanpipe_api_t *tdm_api) 
{
	WANPIPE_API_INIT_CHAN(tdm_api, 0);
	SANGOMA_INIT_TDM_API_CMD_RESULT(*tdm_api);
	tdm_api->wp_cmd.cmd = WP_API_CMD_SET_EVENT;
	tdm_api->wp_cmd.event.wp_api_event_type = WP_API_EVENT_RING_TRIP_DETECT;
	tdm_api->wp_cmd.event.wp_api_event_mode = WP_API_EVENT_DISABLE;
	return sangoma_cmd_exec(fd,tdm_api);
}

int _LIBSNG_CALL sangoma_tdm_txsig_kewl(sng_fd_t fd, wanpipe_api_t *tdm_api) 
{
	WANPIPE_API_INIT_CHAN(tdm_api, 0);
	SANGOMA_INIT_TDM_API_CMD_RESULT(*tdm_api);
	tdm_api->wp_cmd.cmd = WP_API_CMD_SET_EVENT;
	tdm_api->wp_cmd.event.wp_api_event_type = WP_API_EVENT_TXSIG_KEWL;
	tdm_api->wp_cmd.event.wp_api_event_mode = WP_API_EVENT_ENABLE;
	return sangoma_cmd_exec(fd,tdm_api);
}

int _LIBSNG_CALL sangoma_tdm_txsig_start(sng_fd_t fd, wanpipe_api_t *tdm_api) 
{
	WANPIPE_API_INIT_CHAN(tdm_api, 0);	
	SANGOMA_INIT_TDM_API_CMD_RESULT(*tdm_api);
	tdm_api->wp_cmd.cmd = WP_API_CMD_SET_EVENT;
	tdm_api->wp_cmd.event.wp_api_event_type = WP_API_EVENT_TXSIG_START;
	tdm_api->wp_cmd.event.wp_api_event_mode = WP_API_EVENT_ENABLE;
	return sangoma_cmd_exec(fd,tdm_api);
}

int _LIBSNG_CALL sangoma_tdm_txsig_onhook(sng_fd_t fd, wanpipe_api_t *tdm_api) 
{
	WANPIPE_API_INIT_CHAN(tdm_api, 0);
	SANGOMA_INIT_TDM_API_CMD_RESULT(*tdm_api);
	tdm_api->wp_cmd.cmd = WP_API_CMD_SET_EVENT;
	tdm_api->wp_cmd.event.wp_api_event_type = WP_API_EVENT_TXSIG_ONHOOK;
	tdm_api->wp_cmd.event.wp_api_event_mode = WP_API_EVENT_ENABLE;
	return sangoma_cmd_exec(fd,tdm_api);
}

int _LIBSNG_CALL sangoma_tdm_txsig_offhook(sng_fd_t fd, wanpipe_api_t *tdm_api) 
{
	WANPIPE_API_INIT_CHAN(tdm_api, 0);	
	SANGOMA_INIT_TDM_API_CMD_RESULT(*tdm_api);
	tdm_api->wp_cmd.cmd = WP_API_CMD_SET_EVENT;
	tdm_api->wp_cmd.event.wp_api_event_type = WP_API_EVENT_TXSIG_OFFHOOK;
	tdm_api->wp_cmd.event.wp_api_event_mode = WP_API_EVENT_ENABLE;
	return sangoma_cmd_exec(fd,tdm_api);
}

int _LIBSNG_CALL sangoma_tdm_enable_tone_events(sng_fd_t fd, wanpipe_api_t *tdm_api, uint16_t tone_id) 
{
	WANPIPE_API_INIT_CHAN(tdm_api, 0);
	SANGOMA_INIT_TDM_API_CMD_RESULT(*tdm_api);
	tdm_api->wp_cmd.cmd = WP_API_CMD_SET_EVENT;
	tdm_api->wp_cmd.event.wp_api_event_type = WP_API_EVENT_TONE;
	tdm_api->wp_cmd.event.wp_api_event_mode = WP_API_EVENT_ENABLE;
	tdm_api->wp_cmd.event.wp_api_event_tone_type = tone_id;
	return sangoma_cmd_exec(fd,tdm_api);
}

int _LIBSNG_CALL sangoma_tdm_disable_tone_events(sng_fd_t fd, wanpipe_api_t *tdm_api) 
{
	WANPIPE_API_INIT_CHAN(tdm_api, 0);
	SANGOMA_INIT_TDM_API_CMD_RESULT(*tdm_api);
	tdm_api->wp_cmd.cmd = WP_API_CMD_SET_EVENT;
	tdm_api->wp_cmd.event.wp_api_event_type = WP_API_EVENT_TONE;
	tdm_api->wp_cmd.event.wp_api_event_mode = WP_API_EVENT_DISABLE;
	tdm_api->wp_cmd.event.wp_api_event_tone_type = 0x00;
	return sangoma_cmd_exec(fd,tdm_api);
}
#endif

int _LIBSNG_CALL sangoma_tdm_enable_hwec(sng_fd_t fd, wanpipe_api_t *tdm_api)
{
	/* intentionally NOT initializing chan - caller must do it */
	SANGOMA_INIT_TDM_API_CMD_RESULT(*tdm_api);
	tdm_api->wp_cmd.cmd = WP_API_CMD_ENABLE_HWEC;
	return sangoma_cmd_exec(fd,tdm_api);
}

int _LIBSNG_CALL sangoma_tdm_disable_hwec(sng_fd_t fd, wanpipe_api_t *tdm_api)
{
	/* intentionally NOT initializing chan - caller must do it */
	SANGOMA_INIT_TDM_API_CMD_RESULT(*tdm_api);
	tdm_api->wp_cmd.cmd = WP_API_CMD_DISABLE_HWEC;
	return sangoma_cmd_exec(fd,tdm_api);
}


/*========================================================
 * GET Front End Alarms
 * 
 */                  
#ifdef WP_API_FEATURE_FE_ALARM
int _LIBSNG_CALL sangoma_tdm_get_fe_alarms(sng_fd_t fd, wanpipe_api_t *tdm_api, unsigned int *alarms)
{
	int err;

	WANPIPE_API_INIT_CHAN(tdm_api, 0);
	SANGOMA_INIT_TDM_API_CMD_RESULT(*tdm_api);
	tdm_api->wp_cmd.cmd = WP_API_CMD_GET_FE_ALARMS;

	err=sangoma_cmd_exec(fd,tdm_api);
	if (err){
		return err;
	}

	*alarms=tdm_api->wp_cmd.fe_alarms;

	return 0;
}         

/* get current Line Connection state - Connected/Disconnected */
int _LIBSNG_CALL sangoma_get_fe_status(sng_fd_t fd, wanpipe_api_t *tdm_api, unsigned char *current_status)
{
	int err;

	WANPIPE_API_INIT_CHAN(tdm_api, 0);
	SANGOMA_INIT_TDM_API_CMD_RESULT(*tdm_api);
	tdm_api->wp_cmd.cmd = WP_API_CMD_GET_FE_STATUS;
	err = sangoma_cmd_exec(fd, tdm_api);
	*current_status = tdm_api->wp_cmd.fe_status;

	return err;
}
#endif

/* get current Line Connection state - Connected/Disconnected */
#ifdef WP_API_FEATURE_LINK_STATUS
int _LIBSNG_CALL sangoma_get_link_status(sng_fd_t fd, wanpipe_api_t *tdm_api, unsigned char *current_status)
{
	int err;

	WANPIPE_API_INIT_CHAN(tdm_api, 0);
	SANGOMA_INIT_TDM_API_CMD_RESULT(*tdm_api);
	tdm_api->wp_cmd.cmd = WP_API_CMD_GET_FE_STATUS;
	err = sangoma_cmd_exec(fd, tdm_api);
	*current_status = tdm_api->wp_cmd.fe_status;

	return err;
}

/* set current Line Connection state - Connected/Disconnected. valid only for ISDN BRI */
int _LIBSNG_CALL sangoma_set_fe_status(sng_fd_t fd, wanpipe_api_t *tdm_api, unsigned char new_status)
{
	WANPIPE_API_INIT_CHAN(tdm_api, 0);
	SANGOMA_INIT_TDM_API_CMD_RESULT(*tdm_api);
	tdm_api->wp_cmd.cmd = WP_API_CMD_SET_FE_STATUS;
	tdm_api->wp_cmd.fe_status = new_status;
	return sangoma_cmd_exec(fd, tdm_api);
}
#endif

int _LIBSNG_CALL sangoma_disable_bri_bchan_loopback(sng_fd_t fd, wanpipe_api_t *tdm_api, int channel)
{
	WANPIPE_API_INIT_CHAN(tdm_api, 0);
	SANGOMA_INIT_TDM_API_CMD_RESULT(*tdm_api);
	tdm_api->wp_cmd.cmd = WP_API_CMD_SET_EVENT;
	tdm_api->wp_cmd.event.channel	= (unsigned char)channel;
	tdm_api->wp_cmd.event.wp_api_event_type = WP_API_EVENT_BRI_CHAN_LOOPBACK;
	tdm_api->wp_cmd.event.wp_api_event_mode = WP_API_EVENT_DISABLE;
	return sangoma_cmd_exec(fd, tdm_api);
}

int _LIBSNG_CALL sangoma_enable_bri_bchan_loopback(sng_fd_t fd, wanpipe_api_t *tdm_api, int channel)
{
	WANPIPE_API_INIT_CHAN(tdm_api, 0);
	SANGOMA_INIT_TDM_API_CMD_RESULT(*tdm_api);
	tdm_api->wp_cmd.cmd = WP_API_CMD_SET_EVENT;
	tdm_api->wp_cmd.event.channel	= (unsigned char)channel;
	tdm_api->wp_cmd.event.wp_api_event_type = WP_API_EVENT_BRI_CHAN_LOOPBACK;
	tdm_api->wp_cmd.event.wp_api_event_mode = WP_API_EVENT_ENABLE;
	return sangoma_cmd_exec(fd, tdm_api);
}

int _LIBSNG_CALL sangoma_get_tx_queue_sz(sng_fd_t fd, wanpipe_api_t *tdm_api)
{
	int err;

	WANPIPE_API_INIT_CHAN(tdm_api, 0);
	SANGOMA_INIT_TDM_API_CMD_RESULT(*tdm_api);
	tdm_api->wp_cmd.cmd = WP_API_CMD_GET_TX_Q_SIZE;
	tdm_api->wp_cmd.tx_queue_sz = 0;
	
	err=sangoma_cmd_exec(fd, tdm_api);
	if (err < 0) {
		return err;
	}

	return tdm_api->wp_cmd.tx_queue_sz;
}

int _LIBSNG_CALL sangoma_set_tx_queue_sz(sng_fd_t fd, wanpipe_api_t *tdm_api, int size)
{
	if (size < 0) {
		return -1;
	}
	WANPIPE_API_INIT_CHAN(tdm_api, 0);
	SANGOMA_INIT_TDM_API_CMD_RESULT(*tdm_api);
	tdm_api->wp_cmd.cmd = WP_API_CMD_SET_TX_Q_SIZE;
	tdm_api->wp_cmd.tx_queue_sz = size;
	return sangoma_cmd_exec(fd, tdm_api);
}

int _LIBSNG_CALL sangoma_get_rx_queue_sz(sng_fd_t fd, wanpipe_api_t *tdm_api)
{
	int err;

	WANPIPE_API_INIT_CHAN(tdm_api, 0);
	SANGOMA_INIT_TDM_API_CMD_RESULT(*tdm_api);
	tdm_api->wp_cmd.cmd = WP_API_CMD_GET_RX_Q_SIZE;
	tdm_api->wp_cmd.rx_queue_sz = 0;
	
	err=sangoma_cmd_exec(fd, tdm_api);
	if (err < 0) {
		return err;
	}

	return tdm_api->wp_cmd.rx_queue_sz;

}

int _LIBSNG_CALL sangoma_set_rx_queue_sz(sng_fd_t fd, wanpipe_api_t *tdm_api, int size)
{
	if (size < 0) {
		return -1;
	}

	WANPIPE_API_INIT_CHAN(tdm_api, 0);
	SANGOMA_INIT_TDM_API_CMD_RESULT(*tdm_api);
	tdm_api->wp_cmd.cmd = WP_API_CMD_SET_RX_Q_SIZE;
	tdm_api->wp_cmd.rx_queue_sz = size;
	return sangoma_cmd_exec(fd, tdm_api);
}

int _LIBSNG_CALL sangoma_get_driver_version(sng_fd_t fd, wanpipe_api_t *tdm_api, wan_driver_version_t *drv_ver)
{
	int err;

	WANPIPE_API_INIT_CHAN(tdm_api, 0);
	SANGOMA_INIT_TDM_API_CMD_RESULT(*tdm_api);
	tdm_api->wp_cmd.cmd = WP_API_CMD_DRIVER_VERSION;

	err = sangoma_cmd_exec(fd, tdm_api);
	if (err == 0) {
		if (tdm_api->wp_cmd.data_len == sizeof(wan_driver_version_t)) {
			if (drv_ver) {
				memcpy(drv_ver,&tdm_api->wp_cmd.version,sizeof(wan_driver_version_t));
			}
		} else {
			return -1;
		}
	}

	return err;
}

int _LIBSNG_CALL sangoma_get_firmware_version(sng_fd_t fd, wanpipe_api_t *tdm_api, unsigned char *ver)
{
	int err;

	WANPIPE_API_INIT_CHAN(tdm_api, 0);
	SANGOMA_INIT_TDM_API_CMD_RESULT(*tdm_api);
	tdm_api->wp_cmd.cmd = WP_API_CMD_FIRMWARE_VERSION;

	err = sangoma_cmd_exec(fd, tdm_api);
	if (err == 0) {
		if (tdm_api->wp_cmd.data_len == sizeof(unsigned char)) {
			*ver = tdm_api->wp_cmd.data[0];
		} else {
			return -1;
		}
	}

	return err;
}

int _LIBSNG_CALL sangoma_get_cpld_version(sng_fd_t fd, wanpipe_api_t *tdm_api, unsigned char *ver)
{
	int err;

	WANPIPE_API_INIT_CHAN(tdm_api, 0);
	SANGOMA_INIT_TDM_API_CMD_RESULT(*tdm_api);
	tdm_api->wp_cmd.cmd = WP_API_CMD_CPLD_VERSION;

	err = sangoma_cmd_exec(fd, tdm_api);
	if (err == 0) {
		if (tdm_api->wp_cmd.data_len == sizeof(unsigned char)) {
			*ver = tdm_api->wp_cmd.data[0];
		} else {
			return -1;
		}
	}

	return err;
}

int _LIBSNG_CALL sangoma_get_aft_customer_id(sng_fd_t fd, unsigned char *out_customer_id)
{
	wan_udp_hdr_t wan_udp;

	memset(&wan_udp, 0x00, sizeof(wan_udp));

	wan_udp.wan_udphdr_command = WANPIPEMON_AFT_CUSTOMER_ID;
	wan_udp.wan_udphdr_return_code = SANG_STATUS_UNSUPPORTED_FUNCTION;
	wan_udp.wan_udphdr_data_len = 0;

	if (sangoma_mgmt_cmd(fd, &wan_udp)) {
		return SANG_STATUS_IO_ERROR;
	}

	if (wan_udp.wan_udphdr_return_code) {
		return SANG_STATUS_UNSUPPORTED_FUNCTION;
	}

	*out_customer_id = sangoma_get_wan_udphdr_data_byte(&wan_udp, 0);

	return 0;
}

#ifdef WP_API_FEATURE_LED_CTRL
int _LIBSNG_CALL sangoma_port_led_ctrl(sng_fd_t fd, unsigned char led_ctrl)
{
	wan_udp_hdr_t wan_udp;

	memset(&wan_udp, 0x00, sizeof(wan_udp));

	wan_udp.wan_udphdr_command = WANPIPEMON_LED_CTRL;
	wan_udp.wan_udphdr_return_code = SANG_STATUS_UNSUPPORTED_FUNCTION;
	wan_udp.wan_udphdr_data_len = 1;
	wan_udp.wan_udphdr_data[0] = led_ctrl;

	if (sangoma_mgmt_cmd(fd, &wan_udp)) {
		return SANG_STATUS_IO_ERROR;
	}

	if (wan_udp.wan_udphdr_return_code) {
		return SANG_STATUS_UNSUPPORTED_FUNCTION;
	}

	return 0;
}
#endif

#ifdef  WP_API_FEATURE_FE_RW
int _LIBSNG_CALL sangoma_fe_reg_write(sng_fd_t fd, uint32_t offset, uint8_t data)
{
	int chan=0;
	wan_udp_hdr_t wan_udp;
	sdla_fe_debug_t *fe_debug; 
	memset(&wan_udp, 0x00, sizeof(wan_udp));
	
	{
		int err;
		wanpipe_api_t tdm_api;
		memset(&tdm_api,0,sizeof(tdm_api));
		err=sangoma_get_full_cfg(fd, &tdm_api);
		if (err) {
			return err;
		}
		chan=tdm_api.wp_cmd.chan;
		if (chan) 
			chan--;
	}

	wan_udp.wan_udphdr_command	= WAN_FE_SET_DEBUG_MODE;
	wan_udp.wan_udphdr_data_len	= sizeof(sdla_fe_debug_t);
	wan_udp.wan_udphdr_return_code	= 0xaa;
	fe_debug = (sdla_fe_debug_t*)wan_udp.wan_udphdr_data;

	fe_debug->type = WAN_FE_DEBUG_REG;
	fe_debug->mod_no = chan;
	fe_debug->fe_debug_reg.reg  = offset;
	fe_debug->fe_debug_reg.value  = data;
	fe_debug->fe_debug_reg.read = 0;
	
	if (sangoma_mgmt_cmd(fd, &wan_udp)) {
		return SANG_STATUS_IO_ERROR;
	}

	if (wan_udp.wan_udphdr_return_code) {
		return SANG_STATUS_UNSUPPORTED_FUNCTION;
	}

	return 0;
}

int _LIBSNG_CALL sangoma_fe_reg_read(sng_fd_t fd, uint32_t offset, uint8_t *data)
{
	int chan=0;
	wan_udp_hdr_t wan_udp;
	sdla_fe_debug_t *fe_debug;
	memset(&wan_udp, 0x00, sizeof(wan_udp));

	{
		int err;
		wanpipe_api_t tdm_api;
		memset(&tdm_api,0,sizeof(tdm_api));
		err=sangoma_get_full_cfg(fd, &tdm_api);
		if (err) {
			return err;
		}
		chan=tdm_api.wp_cmd.chan;
		if (chan) 
			chan--;
	}

	wan_udp.wan_udphdr_command	= WAN_FE_SET_DEBUG_MODE;
	wan_udp.wan_udphdr_data_len	= sizeof(sdla_fe_debug_t);
	wan_udp.wan_udphdr_return_code	= 0xaa;
	fe_debug = (sdla_fe_debug_t*)wan_udp.wan_udphdr_data;

	fe_debug->type = WAN_FE_DEBUG_REG;
	fe_debug->mod_no = chan;
	fe_debug->fe_debug_reg.reg  = offset;
	fe_debug->fe_debug_reg.read = 1;
	
	if (sangoma_mgmt_cmd(fd, &wan_udp)) {
		return SANG_STATUS_IO_ERROR;
	}

	if (wan_udp.wan_udphdr_return_code) {
		return SANG_STATUS_UNSUPPORTED_FUNCTION;
	}

	*data = fe_debug->fe_debug_reg.value;
	
	return 0;
}
#endif

int _LIBSNG_CALL sangoma_get_stats(sng_fd_t fd, wanpipe_api_t *tdm_api, wanpipe_chan_stats_t *stats)
{
	int err;

	WANPIPE_API_INIT_CHAN(tdm_api, 0);
	SANGOMA_INIT_TDM_API_CMD_RESULT(*tdm_api);
	tdm_api->wp_cmd.cmd = WP_API_CMD_GET_STATS;

	err = sangoma_cmd_exec(fd, tdm_api);
	if (err == 0) {
		if (stats) {
			memcpy(stats, &tdm_api->wp_cmd.stats, sizeof(wanpipe_chan_stats_t));
		}
	}

	return err;
}

int _LIBSNG_CALL sangoma_flush_stats(sng_fd_t fd, wanpipe_api_t *tdm_api)
{
	WANPIPE_API_INIT_CHAN(tdm_api, 0);
	SANGOMA_INIT_TDM_API_CMD_RESULT(*tdm_api);
	tdm_api->wp_cmd.cmd = WP_API_CMD_RESET_STATS;
	return sangoma_cmd_exec(fd, tdm_api);
}

int _LIBSNG_CALL sangoma_set_rm_rxflashtime(sng_fd_t fd, wanpipe_api_t *tdm_api, int rxflashtime)
{
	WANPIPE_API_INIT_CHAN(tdm_api, 0);
	SANGOMA_INIT_TDM_API_CMD_RESULT(*tdm_api);
	tdm_api->wp_cmd.cmd = WP_API_CMD_SET_RM_RXFLASHTIME;
	tdm_api->wp_cmd.rxflashtime=rxflashtime;
	return sangoma_cmd_exec(fd, tdm_api);
}

#ifdef WP_API_FEATURE_RM_GAIN
int _LIBSNG_CALL sangoma_set_rm_tx_gain(sng_fd_t fd, wanpipe_api_t *tdm_api, int value)
{
	WANPIPE_API_INIT_CHAN(tdm_api, 0);
	SANGOMA_INIT_TDM_API_CMD_RESULT(*tdm_api);
	tdm_api->wp_cmd.cmd = WP_API_CMD_SET_EVENT;
	tdm_api->wp_cmd.event.wp_api_event_type = WP_API_EVENT_SET_RM_TX_GAIN;
	tdm_api->wp_cmd.event.wp_api_event_mode = WP_API_EVENT_ENABLE;
	tdm_api->wp_cmd.event.wp_api_event_gain_value = value;
	return sangoma_cmd_exec(fd, tdm_api);
}

int _LIBSNG_CALL sangoma_set_rm_rx_gain(sng_fd_t fd, wanpipe_api_t *tdm_api, int value)
{
	WANPIPE_API_INIT_CHAN(tdm_api, 0);
	SANGOMA_INIT_TDM_API_CMD_RESULT(*tdm_api);
	tdm_api->wp_cmd.cmd = WP_API_CMD_SET_EVENT;
	tdm_api->wp_cmd.event.wp_api_event_type = WP_API_EVENT_SET_RM_RX_GAIN;
	tdm_api->wp_cmd.event.wp_api_event_mode = WP_API_EVENT_ENABLE;
	tdm_api->wp_cmd.event.wp_api_event_gain_value = value;
	return sangoma_cmd_exec(fd, tdm_api);
}
#endif /* WP_API_FEATURE_RM_GAIN */

int _LIBSNG_CALL sangoma_tdm_set_polarity(sng_fd_t fd, wanpipe_api_t *tdm_api, int polarity)
{
	int err;

	WANPIPE_API_INIT_CHAN(tdm_api, 0);
	SANGOMA_INIT_TDM_API_CMD_RESULT(*tdm_api);
	tdm_api->wp_cmd.cmd = WP_API_CMD_SET_EVENT;
	tdm_api->wp_cmd.event.wp_api_event_type = WP_API_EVENT_SETPOLARITY;
	tdm_api->wp_cmd.event.wp_api_event_polarity = polarity;
	err = sangoma_cmd_exec(fd, tdm_api);
	return err;

}


int _LIBSNG_CALL sangoma_tdm_txsig_onhooktransfer(sng_fd_t fd, wanpipe_api_t *tdm_api) 
{
	WANPIPE_API_INIT_CHAN(tdm_api, 0);
	SANGOMA_INIT_TDM_API_CMD_RESULT(*tdm_api);
	tdm_api->wp_cmd.cmd = WP_API_CMD_SET_EVENT;
	tdm_api->wp_cmd.event.wp_api_event_type = WP_API_EVENT_ONHOOKTRANSFER;
	tdm_api->wp_cmd.event.wp_api_event_mode = WP_API_EVENT_ENABLE;
	return sangoma_cmd_exec(fd,tdm_api);
}

#ifdef WP_API_FEATURE_LOOP

int _LIBSNG_CALL sangoma_tdm_enable_loop(sng_fd_t fd, wanpipe_api_t *tdm_api)
{
	int err;

	WANPIPE_API_INIT_CHAN(tdm_api, 0);
	SANGOMA_INIT_TDM_API_CMD_RESULT(*tdm_api);
	tdm_api->wp_cmd.cmd = WP_API_CMD_ENABLE_LOOP;
	err = sangoma_cmd_exec(fd, tdm_api);
	return err;
}      

int _LIBSNG_CALL sangoma_tdm_disable_loop(sng_fd_t fd, wanpipe_api_t *tdm_api)
{
	int err;

	WANPIPE_API_INIT_CHAN(tdm_api, 0);
	SANGOMA_INIT_TDM_API_CMD_RESULT(*tdm_api);
	tdm_api->wp_cmd.cmd = WP_API_CMD_DISABLE_LOOP;
	err = sangoma_cmd_exec(fd, tdm_api);
	return err;
}      

#endif

#ifdef WP_API_FEATURE_HWEC_PERSIST




#endif


#ifdef WP_API_FEATURE_SS7_FORCE_RX
int _LIBSNG_CALL sangoma_ss7_force_rx(sng_fd_t fd, wanpipe_api_t *tdm_api)
{
	WANPIPE_API_INIT_CHAN(tdm_api, 0);
	SANGOMA_INIT_TDM_API_CMD_RESULT(*tdm_api);
	tdm_api->wp_cmd.cmd = WP_API_CMD_SS7_FORCE_RX;
	return sangoma_cmd_exec(fd, tdm_api);
}
#endif

#ifdef WP_API_FEATURE_SS7_CFG_STATUS
int _LIBSNG_CALL sangoma_ss7_get_cfg_status(sng_fd_t fd, wanpipe_api_t *tdm_api, wan_api_ss7_cfg_status_t *ss7_cfg_status)
{
	int err;

	WANPIPE_API_INIT_CHAN(tdm_api, 0);
	SANGOMA_INIT_TDM_API_CMD_RESULT(*tdm_api);
	tdm_api->wp_cmd.cmd = WP_API_CMD_SS7_GET_CFG_STATUS;

	err = sangoma_cmd_exec(fd, tdm_api);
	if (err == 0) {
		if (ss7_cfg_status) {
			memcpy(ss7_cfg_status, &tdm_api->wp_cmd.ss7_cfg_status, sizeof(wan_api_ss7_cfg_status_t));
		}
	}

	return err;
}
#endif


#endif /* WANPIPE_TDM_API */
