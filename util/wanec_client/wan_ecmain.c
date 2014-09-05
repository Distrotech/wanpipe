/******************************************************************************
** Copyright (c) 2005
**	Alex Feldman <al.feldman@sangoma.com>.  All rights reserved.
**
** ============================================================================
** Sep 1, 2005		Alex Feldman	Initial version.
** Sep 2, 2005		Alex Feldman	Add option 'all' for channel 
**					selection.
******************************************************************************/

/******************************************************************************
**			   INCLUDE FILES
******************************************************************************/

#if !defined(__WINDOWS__)
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <fcntl.h>
#include <syslog.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <sys/queue.h>
#include <netinet/in.h>
#if defined(__LINUX__)
# include <linux/if.h>
# include <linux/types.h>
# include <linux/if_packet.h>
#endif
#endif

#include "wan_ecmain.h"
#include <wanpipe_events.h>
#include "wanec_api.h"

/******************************************************************************
**			  DEFINES AND MACROS
******************************************************************************/

/******************************************************************************
**			STRUCTURES AND TYPEDEFS
******************************************************************************/

/******************************************************************************
**			   GLOBAL VARIABLES
******************************************************************************/
wanec_client_t	ec_client;

/******************************************************************************
** 			FUNCTION PROTOTYPES
******************************************************************************/

/******************************************************************************
** 			FUNCTION DEFINITIONS
******************************************************************************/

int main(int argc, char *argv[])
{
	int	err;

	memset(&ec_client, 0, sizeof(wanec_client_t));
	err = wan_ec_args_parse_and_run(argc, argv);
	if (!err){
		printf("\n\n");
	}
	if (ec_client.conf.param_no){
		free(ec_client.conf.params);
		ec_client.conf.params = NULL;
	}
	return err;
}

int wanec_client_config(void)
{
	wanec_api_config_t	conf;
	int 			err;

	memset(&conf, 0, sizeof(wanec_api_config_t));
	if (ec_client.conf.param_no){
		memcpy(&conf.conf, &ec_client.conf, sizeof(wan_custom_conf_t));
	}
	err = wanec_api_config(
			ec_client.devname,
			ec_client.verbose,
			&conf);
	return err;
}

int wanec_client_release(void)
{
	wanec_api_release_t	release;
	int			err;

	memset(&release, 0, sizeof(wanec_api_release_t));
	err = wanec_api_release(
			ec_client.devname,
			ec_client.verbose,
			&release);
	return err;
}

int wanec_client_mode(int enable)
{
	wanec_api_mode_t	mode;
	int			err;

	memset(&mode, 0, sizeof(wanec_api_mode_t));
	mode.enable	= enable;
	mode.fe_chan_map= ec_client.fe_chan_map;
	err = wanec_api_mode(
			ec_client.devname,
			ec_client.verbose,
			&mode);
	return err;
}

int wanec_client_bypass(int enable)
{
	wanec_api_bypass_t	bypass;
	int			err;

	memset(&bypass, 0, sizeof(wanec_api_bypass_t));
	bypass.enable		= enable;
	bypass.fe_chan_map	= ec_client.fe_chan_map;
	err = wanec_api_bypass(
			ec_client.devname,
			ec_client.verbose,
			&bypass);
	return err;
}

int wanec_client_opmode(int mode)
{
	wanec_api_opmode_t	opmode;
	int			err;

	memset(&opmode, 0, sizeof(wanec_api_opmode_t));
	opmode.mode		= mode;
	opmode.fe_chan_map	= ec_client.fe_chan_map;
	err = wanec_api_opmode(
			ec_client.devname,
			ec_client.verbose,
			&opmode);
	return err;
}


int wanec_client_modify()
{
	wanec_api_modify_t	modify;
	int			err;

	memset(&modify, 0, sizeof(wanec_api_modify_t));
	modify.fe_chan_map	= ec_client.fe_chan_map;
	if (ec_client.conf.param_no){
		memcpy(&modify.conf, &ec_client.conf, sizeof(wan_custom_conf_t));
	}
	err = wanec_api_modify(
			ec_client.devname,
			ec_client.verbose,
			&modify);
	return err;	
}

int wanec_client_mute(int mode)
{
	wanec_api_mute_t	mute;
	int			err;

	memset(&mute, 0, sizeof(wanec_api_mute_t));
	mute.mode		= mode;
	mute.fe_chan_map	= ec_client.fe_chan_map;
	mute.port_map		= ec_client.port_map;
	err = wanec_api_mute(
			ec_client.devname,
			ec_client.verbose,
			&mute);
	return err;
	
}

int wanec_client_tone(int id, int enable)
{
	wanec_api_tone_t	tone;
	int			err;

	memset(&tone, 0, sizeof(wanec_api_tone_t));
	tone.id			= id;
	tone.enable		= enable;
	tone.fe_chan_map	= ec_client.fe_chan_map;
	tone.port_map		= ec_client.port_map;
	tone.type_map		= WAN_EC_TONE_PRESENT;
	err = wanec_api_tone(
			ec_client.devname,
			ec_client.verbose,
			&tone);
	return err;
}

int wanec_client_stats(int full)
{
	wanec_api_stats_t	stats;
	int			err;

	memset(&stats, 0, sizeof(wanec_api_stats_t));
	stats.full	= full;
	stats.fe_chan	= ec_client.fe_chan;
	stats.reset	= 0;	//reset
	err = wanec_api_stats(
			ec_client.devname,
			ec_client.verbose,
			&stats);
	return err;
}

int wanec_client_hwimage(void)
{
	wanec_api_image_t	image;
	int			err;

	memset(&image, 0, sizeof(wanec_api_image_t));
	err = wanec_api_hwimage(
			ec_client.devname,
			ec_client.verbose,
			&image);
	return err;
}

int wanec_client_bufferload(void)
{
	wanec_api_bufferload_t	bufferload;
	int			err;

	memset(&bufferload, 0, sizeof(wanec_api_bufferload_t));
	bufferload.buffer	= &ec_client.filename[0];
	bufferload.pcmlaw	= ec_client.pcmlaw;
	err = wanec_api_buffer_load(	
				ec_client.devname,
				ec_client.verbose,
				&bufferload);
	if (!err){
		printf("Buffer index is %d!\n", bufferload.buffer_id);
	}
	return err;
}

int wanec_client_bufferunload(unsigned long buffer_id)
{
	wanec_api_bufferunload_t	bufferunload;
	int				err;

	memset(&bufferunload, 0, sizeof(wanec_api_bufferunload_t));
	bufferunload.buffer_id	= (unsigned int)buffer_id;
	err = wanec_api_buffer_unload(	
				ec_client.devname,
				ec_client.verbose,
				&bufferunload);
	return err;
}

int wanec_client_playout(int start)
{
	wanec_api_playout_t	playout;
	int			err;

	memset(&playout, 0, sizeof(wanec_api_playout_t));
	playout.start		= start;
	playout.fe_chan		= ec_client.fe_chan;
	playout.buffer_id	= ec_client.buffer_id;
	playout.port		= ec_client.port;
	playout.notifyonstop	= 1;
	playout.user_event_id	= 0xA5;		/* dummy value */
	playout.repeat_cnt	= ec_client.repeat_cnt;
	playout.duration	= (ec_client.duration) ? ec_client.duration : 5000;	// default 5s
	err = wanec_api_playout(	
				ec_client.devname,
				ec_client.verbose,
				&playout);
	return err;
}

int wanec_client_monitor(int data_mode)
{
	wanec_api_monitor_t	monitor;
	int			err;

	memset(&monitor, 0, sizeof(wanec_api_monitor_t));
	monitor.fe_chan		= ec_client.fe_chan;
	monitor.data_mode	= data_mode;
	err = wanec_api_monitor(	
				ec_client.devname,
				ec_client.verbose,
				&monitor);
	return err;
}
