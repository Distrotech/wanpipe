/*******************************************************************************//**
 * \file libsangoma_config.c
 * \brief Sangoma Driver and Hardware Configuration, Operation
 *
 * Author(s):	Nenad Corbic, David Rokhvarg
 *
 * Copyright:	(c) 2005-2011 Sangoma Technologies Corporation
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
#include "libsangoma.h"

#ifndef LIBSANGOMA_LIGHT 

/************************************************************//**
 * Device PORT Control Functions
 ***************************************************************/

int sangoma_port_mgmnt_ioctl(sng_fd_t fd, port_management_struct_t *port_management)
{
	int err = 0;
#if defined(__WINDOWS__)
	DWORD ln;
	if(DeviceIoControl(
			fd,
			IoctlPortManagementCommand,
			(LPVOID)port_management,
			sizeof(port_management_struct_t),
			(LPVOID)port_management,
			sizeof(port_management_struct_t),
			(LPDWORD)(&ln),
			(LPOVERLAPPED)NULL
						) == FALSE){
		/* Call OS specific code to find cause of the error and check messages log. */
		DBG_ERR("%s():Error: IoctlPortManagementCommand failed!!\n", __FUNCTION__);
		err = -1;
	}
#else
	err=ioctl(fd,WANPIPE_IOCTL_PORT_MGMT,port_management);
	if (err) {
		err = -1;
	}
#endif
	if(err){
		port_management->operation_status = SANG_STATUS_INVALID_DEVICE;
	}

	return err;
}

int sangoma_port_cfg_ioctl(sng_fd_t fd, port_cfg_t *port_cfg)
{
	int err = 0;
#if defined(__WINDOWS__)
	DWORD ln;
	if(DeviceIoControl(
			fd,
			IoctlPortConfigurationCommand,
			(LPVOID)port_cfg,
			sizeof(port_cfg_t),
			(LPVOID)port_cfg,
			sizeof(port_cfg_t),
			(LPDWORD)(&ln),
			(LPOVERLAPPED)NULL
						) == FALSE){
		/* Call OS specific code to find cause of the error and check messages log. */
		DBG_ERR("%s():Error: IoctlPortConfigurationCommand failed!!\n", __FUNCTION__);
		err = -1;
	}
#else
	err=ioctl(fd,WANPIPE_IOCTL_PORT_CONFIG,port_cfg);
	if (err) {
		err = -1;
	}
#endif
	if(err){
		port_cfg->operation_status = SANG_STATUS_INVALID_DEVICE;
	}

	return err;
}

/* open wanpipe configuration device */
sng_fd_t _LIBSNG_CALL sangoma_open_driver_ctrl(int port_no)
{
	char tmp_fname[FNAME_LEN];

#if defined(__WINDOWS__)
	/* Form the Config Device Name (i.e. wanpipe1, wanpipe2,...). */
	_snprintf(tmp_fname, DEV_NAME_LEN, WP_PORT_NAME_FORM, port_no);
#else
	/* Form the Config Device Name. ("/dev/wanpipe") */
	_snprintf(tmp_fname, DEV_NAME_LEN, WP_CONFIG_DEV_NAME);
#endif
	return sangoma_open_dev_by_name(tmp_fname);
}


int _LIBSNG_CALL sangoma_mgmt_cmd(sng_fd_t fd, wan_udp_hdr_t* wan_udp)
{
	int err=0;
#if defined(__WINDOWS__)
	if(UdpManagementCommand(fd, wan_udp)){
		err = 1;
	}
#else
	unsigned char id = 0;

	wan_udp->wan_udphdr_request_reply = 0x01;
	wan_udp->wan_udphdr_id = id;
	wan_udp->wan_udphdr_return_code = WAN_UDP_TIMEOUT_CMD;

	err=ioctl(fd,WANPIPE_IOCTL_PIPEMON,wan_udp);
	if (err < 0) {
		err = 1;
	}
#endif
	if(err){
		/* The ioctl failed. */
		return err;
	}

	/* The ioctl was successfull. The caller must check
	 * value of wan_udp->wan_udphdr_return_code. */
	return 0;
}

int _LIBSNG_CALL sangoma_driver_port_start(sng_fd_t fd, port_management_struct_t *port_mgmnt, unsigned short port_no)
{
	int err;
	port_mgmnt->operation_status = SANG_STATUS_GENERAL_ERROR;
	port_mgmnt->command_code = START_PORT_VOLATILE_CONFIG;
	port_mgmnt->port_no	= port_no;

	err = sangoma_port_mgmnt_ioctl(fd, port_mgmnt);
	if (err) {
		/* ioctl failed */
		return err;
	}

	return port_mgmnt->operation_status;
}

int _LIBSNG_CALL sangoma_driver_port_stop(sng_fd_t fd, port_management_struct_t *port_mgmnt, unsigned short port_no)
{
	int err;
	port_mgmnt->operation_status = SANG_STATUS_GENERAL_ERROR;
	port_mgmnt->command_code = STOP_PORT;
	port_mgmnt->port_no	= port_no;

	err = sangoma_port_mgmnt_ioctl(fd, port_mgmnt);
	if (err) {
		/* ioctl failed */
		return err;
	}

	switch(port_mgmnt->operation_status)
	{
	case SANG_STATUS_CAN_NOT_STOP_DEVICE_WHEN_ALREADY_STOPPED:
		/* This is not an error, rather a state indication.
		 * Return SANG_STATUS_SUCCESS, but real return code will be available
		 * for the caller at port_mgmnt->operation_status. */
		err = SANG_STATUS_SUCCESS;
		break;
	default:
		err = port_mgmnt->operation_status;
		break;
	}

	return err;
}

int _LIBSNG_CALL sangoma_driver_get_hw_info(sng_fd_t fd, port_management_struct_t *port_mgmnt, unsigned short port_no)
{
	int err;
	port_mgmnt->operation_status = SANG_STATUS_GENERAL_ERROR;
	port_mgmnt->command_code = GET_HARDWARE_INFO;
	port_mgmnt->port_no     = port_no;

	err = sangoma_port_mgmnt_ioctl(fd, port_mgmnt);
	if (err) {
		return err;
	}

	return port_mgmnt->operation_status;
}

int _LIBSNG_CALL sangoma_driver_get_version(sng_fd_t fd, port_management_struct_t *port_mgmnt, unsigned short port_no)
{
        int err;
        port_mgmnt->command_code = GET_DRIVER_VERSION;
        port_mgmnt->port_no     = port_no;

        err = sangoma_port_mgmnt_ioctl(fd, port_mgmnt);
        if (err) {
                return err;
        }

        return port_mgmnt->operation_status;
}

#ifdef  WP_API_FEATURE_HARDWARE_RESCAN
int _LIBSNG_CALL sangoma_driver_hw_rescan(sng_fd_t fd, port_management_struct_t *port_mgmnt, int *cnt)
{
    int err;
    port_mgmnt->command_code = WANPIPE_HARDWARE_RESCAN;
    port_mgmnt->port_no     = 1;

    err = sangoma_port_mgmnt_ioctl(fd, port_mgmnt);
    if (err < 0) {
    	return err;
    }

	*cnt=port_mgmnt->port_no;
     
    return port_mgmnt->operation_status;   
}
#endif


int _LIBSNG_CALL sangoma_driver_port_set_config(sng_fd_t fd, port_cfg_t *port_cfg, unsigned short port_no)
{
	port_cfg->operation_status = SANG_STATUS_GENERAL_ERROR;
	port_cfg->command_code = SET_PORT_VOLATILE_CONFIG;
	port_cfg->port_no	= port_no;

	return sangoma_port_cfg_ioctl(fd, port_cfg);
}

int _LIBSNG_CALL sangoma_driver_port_get_config(sng_fd_t fd, port_cfg_t *port_cfg, unsigned short port_no)
{
	port_cfg->operation_status = SANG_STATUS_GENERAL_ERROR;
	port_cfg->command_code = GET_PORT_VOLATILE_CONFIG;
	port_cfg->port_no = port_no;
	return sangoma_port_cfg_ioctl(fd, port_cfg);
}

int _LIBSNG_CALL sangoma_write_port_config_on_persistent_storage(hardware_info_t *hardware_info, port_cfg_t *port_cfg, unsigned short port_no)
{
	int err = 0;
#if defined(__WINDOWS__)
	HKEY		hPortRegistryKey	= registry_open_port_key(hardware_info);
/*	wandev_conf_t	*wandev_conf	= &port_cfg->wandev_conf;
	sdla_fe_cfg_t	*sdla_fe_cfg	= &wandev_conf->fe_cfg;*/
	unsigned int	ind;

	if(hPortRegistryKey == INVALID_HANDLE_VALUE){
		return 1;
	}

	/* write T1/E1/BRI/Analog configuration */
	if(registry_write_front_end_cfg(hPortRegistryKey, port_cfg)){
		return 2;
	}

	/* write TDM Voice configuration */
	if(registry_write_wan_tdmv_conf(hPortRegistryKey, port_cfg)){
		return 3;
	}

	/* write number of groups */
	err = registry_set_integer_value(hPortRegistryKey, "aft_number_of_logic_channels", port_cfg->num_of_ifs);
	if(err){
		return err;
	}
	
	/* write configuration of each group */
	for(ind = 0; ind < port_cfg->num_of_ifs; ind++){
		registry_write_channel_group_cfg(hPortRegistryKey, port_cfg, ind, port_cfg->if_cfg[ind]);
	}

#else
	printf("%s(): Warning: function not implemented\n", __FUNCTION__);
	err = 1;
#endif
	return err;
}




#endif /* #ifndef LIBSANGOMA_LIGHT */   
