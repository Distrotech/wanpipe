/*******************************************************************************//**
 * \file libsangoma_hwec.c
 * \brief Hardware Echo Canceller API Code Library for
 *		Sangoma AFT T1/E1/Analog/BRI hardware.
 *
 * Author(s):	David Rokhvarg <davidr@sangoma.com>
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

#include "libsangoma.h"
#include "libsangoma-pvt.h"
#include "wanpipe_includes.h"

#ifdef WP_API_FEATURE_LIBSNG_HWEC

#include "wanpipe_events.h"
#include "wanec_api.h"
#include "wanec_iface_api.h"

#if defined (__WINDOWS__)
# include "wanpipe_time.h"	/* wp_sleep() */
# pragma comment( lib, "waneclib" )	/* import functions from waneclib.dll */
#endif/* __WINDOWS__) */

static int libsng_hwec_verbosity_level = 0x00;

/************************************************************//**
 * Private Functions. (Not exported)
 ***************************************************************/ 

static sangoma_status_t sangoma_hwec_bypass(char *device_name, int enable, unsigned int fe_chan_map)
{
	sangoma_status_t rc;
	wanec_api_hwec_t hwec;

	memset(&hwec, 0, sizeof(wanec_api_hwec_t));

	hwec.enable = enable;
	hwec.fe_chan_map = fe_chan_map;

	/* WAN_EC_API_CMD_HWEC_ENABLE/WAN_EC_API_CMD_HWEC_DISABLE - Controls the "bypass" mode.)*/
	rc = wanec_api_hwec(device_name, libsng_hwec_verbosity_level, &hwec);

	return rc;
}

static int sangoma_hwec_is_numeric_parameter(char *parameter)
{
	int i;
	static char *WANEC_numeric_params[] = {
		"WANEC_TailDisplacement",
		"WANEC_MaxPlayoutBuffers",
		"WANEC_MaxConfBridges",
		"WANEC_EchoOperationMode",
		"WANEC_ComfortNoiseMode",
		"WANEC_NonLinearityBehaviorA",
		"WANEC_NonLinearityBehaviorB",
		"WANEC_DoubleTalkBehavior",
		"WANEC_RinLevelControlGainDb",
		"WANEC_SoutLevelControlGainDb",
		"WANEC_RinAutomaticLevelControlTargetDb",
		"WANEC_SoutAutomaticLevelControlTargetDb",
		"WANEC_RinHighLevelCompensationThresholdDb",
		"WANEC_AnrSnrEnhancementDb",
		"WANEC_AecTailLength",
		NULL
	};

	i = 0;
	while(WANEC_numeric_params[i]){
		if (!wp_strncasecmp(parameter, WANEC_numeric_params[i], strlen(parameter))) {
			return 1;/* this IS a numeric parameter */
		}
		i++;
	};

	return 0;/* NOT a numeric parameter */
}

static sangoma_status_t sangoma_hwec_ioctl(sng_fd_t fd, wan_ec_api_t *ec_api)
{
	wanpipe_api_t tdm_api;
	sangoma_status_t err;

	/* it is very important to zero out 'tdm_api' */
	memset(&tdm_api, 0x00, sizeof(tdm_api));

	WANPIPE_API_INIT_CHAN((&tdm_api), ec_api->fe_chan);
	SANGOMA_INIT_TDM_API_CMD_RESULT(tdm_api);

	tdm_api.wp_cmd.cmd = WP_API_CMD_EC_IOCTL;

	tdm_api.wp_cmd.iovec_list.iovec_list[0].iov_base = ec_api;
	tdm_api.wp_cmd.iovec_list.iovec_list[0].iov_len = sizeof(*ec_api);

	err = sangoma_cmd_exec(fd, &tdm_api);
	if (err) {
		DBG_HWEC("sangoma_cmd_exec() Failed: err %d !\n", err);
		return err;
	}

#if 0
	if (WAN_EC_API_RC_OK != ec_api->err) {

		/* IOCTL ok, but error in cmd - caller must check 'ec_api->err' */
		switch(ec_api->err){
		case WAN_EC_API_RC_INVALID_STATE:
			DBG_HWEC("WP_API_CMD_EC_IOCTL Failed: Invalid State: %s !\n", 
				WAN_EC_STATE_DECODE(ec_api->state));
			break;
		case WAN_EC_API_RC_FAILED:
		case WAN_EC_API_RC_INVALID_CMD:
		case WAN_EC_API_RC_INVALID_DEV:
		case WAN_EC_API_RC_BUSY:
		case WAN_EC_API_RC_INVALID_CHANNELS:
		case WAN_EC_API_RC_INVALID_PORT:
		default:
			DBG_HWEC("WP_API_CMD_EC_IOCTL Failed: %s !\n",
				WAN_EC_API_RC_DECODE(ec_api->err));
			break;
		}
	}
#endif

	return SANG_STATUS_SUCCESS;
}

/***************************************************************
 * Public Functions. (Exported)
 ***************************************************************/ 

/*!
  \fn void _LIBSNG_CALL sangoma_hwec_initialize_custom_parameter_structure(wan_custom_param_t *custom_param, char *parameter_name, char *parameter_value)

  \brief Initialize Custom Paramter structure.

  \param parameter_name  Parameter Name

  \param parameter_value Parameter Value

  \return None
*/
void _LIBSNG_CALL sangoma_hwec_initialize_custom_parameter_structure(wan_custom_param_t *custom_param, char *parameter_name, char *parameter_value)
{
	memset(custom_param, 0x00, sizeof(*custom_param));

	strncpy( custom_param->name, parameter_name, sizeof(custom_param->name) );

	if (sangoma_hwec_is_numeric_parameter(parameter_name)) {
		custom_param->dValue = atoi(parameter_value);
	} else {
		strncpy(custom_param->sValue, parameter_value, sizeof(custom_param->sValue));
	}
}

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
sangoma_status_t _LIBSNG_CALL sangoma_hwec_config_init(char *device_name, wan_custom_param_t custom_params[], unsigned int number_of_custom_params)
{
	sangoma_status_t rc = SANG_STATUS_SUCCESS;
	wanec_api_config_t config;

	memset(&config, 0x00, sizeof(config));

	if (number_of_custom_params >= 1 && number_of_custom_params <= 4) {

		wan_custom_param_t *custom_parms_ptr;
		unsigned int i, custom_params_memory_size;

		custom_params_memory_size = sizeof(wan_custom_param_t) * number_of_custom_params;

		/* Do NOT change memory at custom_params[] (it belongs to the caller).
		 * Instead allocate temporary buffer, and use information in custom_params[]
		 * for proper initialization the temproary buffer and
		 * and send if down to API driver. */
		custom_parms_ptr = malloc(custom_params_memory_size);
		if (!custom_parms_ptr) {
			return SANG_STATUS_FAILED_ALLOCATE_MEMORY;
		}

		memset(custom_parms_ptr, 0x00, custom_params_memory_size);

		for (i = 0; i < number_of_custom_params; i++) {

			strcpy( custom_parms_ptr[i].name, custom_params[i].name );

			if (sangoma_hwec_is_numeric_parameter(custom_params[i].name)) {
				custom_parms_ptr[i].dValue = atoi(custom_params[i].sValue);
			} else {
				strcpy(custom_parms_ptr[i].sValue, custom_params[i].sValue);
			}
		} /* for() */

		config.conf.param_no = number_of_custom_params;
		config.conf.params = custom_parms_ptr;

	}/* if() */

	/* Load firmware on EC chip AND apply configuration, if any. */
	rc = wanec_api_config( device_name, libsng_hwec_verbosity_level, &config );

	if (config.conf.params) {
		free(config.conf.params);
	}

	return rc;
}


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
sangoma_status_t _LIBSNG_CALL sangoma_hwec_config_release(char *device_name)
{
	sangoma_status_t rc;
	wanec_api_release_t	release;

	memset(&release, 0, sizeof(wanec_api_release_t));

	rc = wanec_api_release( device_name, libsng_hwec_verbosity_level, &release );

	return rc;
}

/*!
	Modify channel operation mode.
*/
sangoma_status_t _LIBSNG_CALL sangoma_hwec_config_operation_mode(char *device_name, int mode, unsigned int fe_chan_map)
{
	sangoma_status_t rc;
	wanec_api_opmode_t	opmode;

	memset(&opmode, 0, sizeof(wanec_api_opmode_t));

	opmode.mode = mode;
	opmode.fe_chan_map = fe_chan_map;

	/* modes are:
	WANEC_API_OPMODE_NORMAL,
	WANEC_API_OPMODE_HT_FREEZE,
	WANEC_API_OPMODE_HT_RESET, 
	WANEC_API_OPMODE_POWER_DOWN,
	WANEC_API_OPMODE_NO_ECHO,
	WANEC_API_OPMODE_SPEECH_RECOGNITION.
	*/
	rc = wanec_api_opmode(device_name, libsng_hwec_verbosity_level, &opmode);

	return rc;
}

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
sangoma_status_t _LIBSNG_CALL sangoma_hwec_config_power_on(char *device_name,  unsigned int fe_chan_map)
{
	return sangoma_hwec_config_operation_mode(device_name, WANEC_API_OPMODE_NORMAL, fe_chan_map);
}

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
sangoma_status_t _LIBSNG_CALL sangoma_hwec_config_power_off(char *device_name,  unsigned int fe_chan_map)
{
	return sangoma_hwec_config_operation_mode(device_name, WANEC_API_OPMODE_POWER_DOWN, fe_chan_map);
}

/*!
  \fn sangoma_status_t _LIBSNG_CALL sangoma_hwec_enable(char *device_name,  unsigned int fe_chan_map)

  \brief Redirect audio stream from AFT FPGA to EC chip. 
        This command effectively enables echo cancellation since
		data is now forced through the EC chip by the FPGA. 
		Data will be modified by the echo canceller.
		This command is recommened for fast enabling of Echo Cancellation.
        Note 1: Chip must be configured and in POWER ON state for echo
				Chancellation to take place.
		Note 2: sangoma_tdm_enable_hwec() function can be used to achive
				the same funcitnality based on file descriptor versus
				channel map.

  \param device_name Sangoma API device name. 
		Windows: wanpipe1_if1, wanpipe2_if1...
		Linux: wanpipe1, wanpipe2...

  \param fe_chan_map Bitmap of channels (timeslots for Digital, lines for Analog) where 
		the call will take effect.

  \return SANG_STATUS_SUCCESS: success, or error status
*/
sangoma_status_t _LIBSNG_CALL sangoma_hwec_enable(char *device_name,  unsigned int fe_chan_map)
{
	return sangoma_hwec_bypass(device_name, 1 /* enable */, fe_chan_map);
}


/*!
  \fn sangoma_status_t _LIBSNG_CALL sangoma_hwec_disable(char *device_name, unsigned int fe_chan_map)

  \brief Force AFT FPGA to bypass the echo canceller.  
         This command effectively disables echo cancellation since
		 data will not flowing through the ec chip. 
         Data will not be modified by the echo canceller.
         This command is recommened for fast disabling of Echo Cancelation.
		 Note: sangoma_tdm_disable_hwec() function can be use to achive
				the same functionality based on file descriptor versus
				channel map.

  \param device_name Sangoma API device name. 
		Windows: wanpipe1_if1, wanpipe2_if1...
		Linux: wanpipe1, wanpipe2...

  \param fe_chan_map Bitmap of channels (timeslots for Digital, lines for Analog) where 
		the call will take effect.

  \return SANG_STATUS_SUCCESS: success, or error status
*/
sangoma_status_t _LIBSNG_CALL sangoma_hwec_disable(char *device_name, unsigned int fe_chan_map)
{
	return sangoma_hwec_bypass(device_name, 0 /* disable */, fe_chan_map);;
}

/*!
  \fn sangoma_status_t _LIBSNG_CALL sangoma_hwec_config_channel_parameter(char *device_name,	char *parameter, char *parameter_value, unsigned int channel_map)

  \brief Modify channel configuration parameters.

  \param device_name Sangoma API device name. 
		Windows: wanpipe1_if1, wanpipe2_if1...
		Linux: wanpipe1, wanpipe2...

  \param parameter Echo Cancellation channel parameter

	This channel parameters are listed under "Channel parameter":

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

  \param parameter_value channel parameter value, listed under "Channel parameter value"

  \param fe_chan_map Bitmap of channels (timeslots for Digital, lines for Analog) where 
		the call will take effect.

  \return SANG_STATUS_SUCCESS: success, or error status
*/
sangoma_status_t _LIBSNG_CALL sangoma_hwec_config_channel_parameter(char *device_name,	char *parameter, char *parameter_value, unsigned int channel_map)
{
	sangoma_status_t rc;
	wanec_api_modify_t channelModify;
	wan_custom_param_t custom_param;

	memset(&channelModify, 0x00, sizeof(channelModify));

	sangoma_hwec_initialize_custom_parameter_structure(&custom_param, parameter, parameter_value);

	channelModify.fe_chan_map = channel_map;
	channelModify.conf.param_no = 1;
	channelModify.conf.params = &custom_param;

	rc = wanec_api_modify( device_name, libsng_hwec_verbosity_level, &channelModify );

	return rc;
}

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
sangoma_status_t _LIBSNG_CALL sangoma_hwec_config_tone_detection(char *device_name, int tone_id, int enable, unsigned int fe_chan_map, unsigned char port_map)
{
	sangoma_status_t rc;
	wanec_api_tone_t tone;

	memset(&tone, 0, sizeof(wanec_api_tone_t));

	tone.id		= tone_id;
	tone.enable	= enable;
	tone.fe_chan_map = fe_chan_map;
	tone.port_map	= port_map;
	tone.type_map	= WAN_EC_TONE_PRESENT | WAN_EC_TONE_STOP;

	rc = wanec_api_tone( device_name, libsng_hwec_verbosity_level, &tone );

	return rc;
}

/*!
  \fn sangoma_status_t _LIBSNG_CALL sangoma_hwec_print_statistics(char *device_name, int full, unsigned int fe_chan)

  \brief Read and print Chip/Channel statistics from EC chip. 

  \param device_name Sangoma API device name. 
		Windows: wanpipe1_if1, wanpipe2_if1...
		Linux: wanpipe1, wanpipe2...

  \param full Flag to read full statistics, if set to 1.

  \param fe_chan Channel number (a timeslot for Digital, a line for Analog) where 
		the call will read statistics. Values from 1 to 31 will indicate the 
		channel nuber. A value of zero will indicate request to print global chip statistics,
		not per-channel statistics.

  \return SANG_STATUS_SUCCESS: success, or error status
*/
sangoma_status_t _LIBSNG_CALL sangoma_hwec_print_statistics(char *device_name, int full, unsigned int fe_chan)
{
	sangoma_status_t rc;
	wanec_api_stats_t stats;

	memset(&stats, 0, sizeof(wanec_api_stats_t));

	stats.full	= full;
	stats.fe_chan = fe_chan;
	stats.reset = 0;	/* do not reset */

	rc = wanec_api_stats( device_name, libsng_hwec_verbosity_level, &stats );

	return rc;
}

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
sangoma_status_t _LIBSNG_CALL sangoma_hwec_audio_buffer_load(char *device_name, char *filename, char pcmlaw, int *out_buffer_id)
{
	sangoma_status_t rc;
	wanec_api_bufferload_t bufferload;

	memset(&bufferload, 0, sizeof(wanec_api_bufferload_t));
	*out_buffer_id = -1;

	bufferload.buffer = filename;
	bufferload.pcmlaw = pcmlaw;

	rc = wanec_api_buffer_load( device_name, libsng_hwec_verbosity_level, &bufferload );
	if( rc ) {
		return rc;
	}

	*out_buffer_id = bufferload.buffer_id;

	return SANG_STATUS_SUCCESS;
}

/*!
  \fn sangoma_status_t _LIBSNG_CALL sangoma_hwec_audio_mem_buffer_load(char *device_name, char *filename, char pcmlaw, int *out_buffer_id)

  \brief Load audio buffer to EC chip. The buffer can be played out using the sangoma_hwec_audio_buffer_playout() function. 

  \param out_buffer_id when the buffer is loaded on the chip, it is assigned an ID. This ID should
			be used when requesting to play out the buffer.
 
  \return SANG_STATUS_SUCCESS: success, or error status
 */
sangoma_status_t _LIBSNG_CALL sangoma_hwec_audio_mem_buffer_load(char *device_name, unsigned char *buffer, unsigned int in_size, char pcmlaw, int *out_buffer_id)
{
	sangoma_status_t rc;
	wanec_api_membufferload_t bufferload;

	memset(&bufferload, 0, sizeof(bufferload));
	*out_buffer_id = -1;

	bufferload.buffer = buffer;
	bufferload.size = in_size;
	bufferload.pcmlaw = pcmlaw;

	rc = wanec_api_mem_buffer_load( device_name, libsng_hwec_verbosity_level, &bufferload );
	if( rc ) {
          return rc;
	}

	*out_buffer_id = bufferload.buffer_id;

	return SANG_STATUS_SUCCESS;
}

/*!
  \fn sangoma_status_t _LIBSNG_CALL sangoma_hwec_audio_bufferunload(char *device_name, int in_buffer_id)

  \brief Unload/remove an audio buffer from the HWEC chip.

  \param device_name Sangoma wanpipe device name. (ex: wanpipe1 - Linux; wanpipe1_if1 - Windows).

  \param in_buffer_id	ID of the buffer which will be unloaded. The ID must be initialized by sangoma_hwec_audio_bufferload().

  \return SANG_STATUS_SUCCESS - buffer was successfully unloaded/removed, SANG_STATUS_GENERAL_ERROR - error occured
*/
sangoma_status_t _LIBSNG_CALL sangoma_hwec_audio_buffer_unload(char *device_name, int in_buffer_id)
{
	sangoma_status_t rc;
	wanec_api_bufferunload_t bufferunload;

	memset(&bufferunload, 0, sizeof(wanec_api_bufferunload_t));

	bufferunload.buffer_id	= (unsigned int)in_buffer_id;

	rc = wanec_api_buffer_unload( device_name, libsng_hwec_verbosity_level, &bufferunload);

	return rc;
}

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
																int repeat_cnt,	int duration)
{
	sangoma_status_t rc;
	wanec_api_playout_t	playout;

	memset(&playout, 0, sizeof(wanec_api_playout_t));

	playout.start		= start;
	playout.fe_chan		= fe_chan_map;
	playout.buffer_id	= in_buffer_id;
	playout.port		= port;
	playout.notifyonstop	= 1;
	playout.user_event_id	= 0xA5;	/* dummy value */
	playout.repeat_cnt	= repeat_cnt;
	playout.duration	= (duration) ? duration : cOCT6100_INVALID_VALUE;	/* default is no duration */

	rc = wanec_api_playout( device_name, libsng_hwec_verbosity_level, &playout);

	return rc;
}

/*!
  \fn void _LIBSNG_CALL sangoma_hwec_config_verbosity(int verbosity_level)

  \brief Set Verbosity level of EC API Driver and Library.
		The level controls amount of data 
		printed to stdout and to wanpipelog.txt (Windows) or
		/var/log/messages (Linux) for diagnostic purposes.

  \param verbosity_level Valid values are from 0 to 3.

  \return SANG_STATUS_SUCCESS: success, or error status
*/
sangoma_status_t _LIBSNG_CALL sangoma_hwec_config_verbosity(int verbosity_level)
{
	if (verbosity_level >= 0 || verbosity_level <= 3) {
		libsng_hwec_verbosity_level = verbosity_level;
		wanec_api_set_lib_verbosity(verbosity_level);
		return SANG_STATUS_SUCCESS;
	}
	return SANG_STATUS_INVALID_PARAMETER;
}

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
			int *hwec_api_return_code, wanec_chan_stats_t *wanec_chan_stats, int verbose, int reset)
{
	sangoma_status_t err;
	wan_ec_api_t ec_api;

	memset(&ec_api, 0x00, sizeof(ec_api));

	ec_api.cmd = WAN_EC_API_CMD_STATS_FULL;

	ec_api.verbose = verbose;

	/* translate channel number into "single bit" bitmap */
	ec_api.fe_chan_map = (1 << fe_chan);

	/* user may want to reset (clear) Channel statistics */
	ec_api.u_chan_stats.reset = reset;

	err = sangoma_hwec_ioctl(fd, &ec_api);
	if (err) {
		/* error in IOCTL */
		return err;
	}
		
	/* copy stats from driver buffer to user buffer */
	memcpy(wanec_chan_stats, &ec_api.u_chan_stats, sizeof(*wanec_chan_stats));

	*hwec_api_return_code = ec_api.err;

	return SANG_STATUS_SUCCESS;
}

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
			int *hwec_api_return_code, wanec_chip_stats_t *wanec_chip_stats, int verbose, int reset)
{

	sangoma_status_t err;
	wan_ec_api_t ec_api;

	memset(&ec_api, 0x00, sizeof(ec_api));

	ec_api.cmd = WAN_EC_API_CMD_STATS_FULL;

	ec_api.verbose = verbose;

	/* indicate to Driver to get chip stats, not channel stats, by setting fe_chan_map to zero */
	ec_api.fe_chan_map = 0;

	/* user may want to reset (clear) Chip statistics */
	ec_api.u_chip_stats.reset = reset;

	err = sangoma_hwec_ioctl(fd, &ec_api);
	if (err) {
		/* error in IOCTL */
		return err;
	}

	/* copy stats from driver buffer to user buffer */
	memcpy(wanec_chip_stats, &ec_api.u_chip_stats, sizeof(*wanec_chip_stats));

	*hwec_api_return_code = ec_api.err;

	return SANG_STATUS_SUCCESS;

}

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
			int *hwec_api_return_code, wanec_chip_image_t *wanec_chip_image, int verbose)
{
	sangoma_status_t err;
	wan_ec_api_t ec_api;

	memset(&ec_api, 0x00, sizeof(ec_api));

	ec_api.cmd = WAN_EC_API_CMD_STATS_IMAGE;

	ec_api.verbose = verbose;

	/* driver will copy image information into wanec_chip_image->f_ChipImageInfo */
	ec_api.u_chip_image.f_ChipImageInfo = wanec_chip_image->f_ChipImageInfo;

	err = sangoma_hwec_ioctl(fd, &ec_api);
	if (err) {
		/* error in IOCTL */
		return err;
	}

	*hwec_api_return_code = ec_api.err;

	return SANG_STATUS_SUCCESS;
}


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
		int *hwec_api_return_code, int enable, int verbose)
{
	sangoma_status_t err;
	wan_ec_api_t ec_api;

	memset(&ec_api, 0x00, sizeof(ec_api));

	if (enable) {
		ec_api.cmd = WAN_EC_API_CMD_HWDTMF_REMOVAL_ENABLE;
	} else {
		ec_api.cmd = WAN_EC_API_CMD_HWDTMF_REMOVAL_DISABLE;
	}

	ec_api.verbose = verbose;

	/* translate channel number into "single bit" bitmap */
	ec_api.fe_chan_map = (1 << fe_chan);

	err = sangoma_hwec_ioctl(fd, &ec_api);
	if (err) {
		/* error in IOCTL */
		return err;
	}

	*hwec_api_return_code = ec_api.err;

	return SANG_STATUS_SUCCESS;
}

#endif /* WP_API_FEATURE_LIBSNG_HWEC */
