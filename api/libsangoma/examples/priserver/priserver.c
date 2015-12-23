/*****************************************************************************
 * priserver.c Refactoring of pritest.c
 *
 * Author(s):   Anthony Minessale II <anthmct@yahoo.com>
 *              Nenad Corbic <ncorbic@sangoma.com>
 *
 * Copyright:   (c) 2005 Anthony Minessale II
 *
 *      This program is free software; you can redistribute it and/or
 *      modify it under the terms of the GNU General Public License
 *      as published by the Free Software Foundation; either version
 *      2 of the License, or (at your option) any later version.
 * ============================================================================
 */

#include <libsangoma.h>
#include <sangoma_pri.h>


typedef struct {
	int pid;
	q931_call *call;
	void *pri;
}call_info_t;


#define SANGOMA_MAX_CHAN_PER_SPAN 32

static call_info_t pidmap[SANGOMA_MAX_CHAN_PER_SPAN];

/* Stupid runtime process to play a file to a b channel*/
#define BYTES 80
#define MAX_BYTES 1000
static void launch_channel(struct sangoma_pri *spri, int channo)
{
	pid_t pid;
	int fd = 0, file = 0, inlen = 0, outlen = 0;
	unsigned char inframe[MAX_BYTES], outframe[MAX_BYTES];
	sangoma_api_hdr_t hdrframe;
	sangoma_status_t status;
	sangoma_wait_obj_t *sng_wait;
	int mtu_mru=BYTES;
	wanpipe_tdm_api_t tdm_api;
	int err;
	uint32_t flagsout;

	pid = fork();

	if (pid) {
		pidmap[channo-1].pid = pid;
		printf("-- Launching process %d to handle channel %d\n", pid, channo);
	} else {
		memset(inframe, 0, MAX_BYTES);
		memset(outframe, 0, MAX_BYTES);

		if((fd = sangoma_open_tdmapi_span_chan(spri->span, channo)) < 0) {
			printf("DEBUG cant open fd!\n");
		}

		
		err=sangoma_tdm_set_codec(fd, &tdm_api, WP_SLINEAR);
		if (err < 0){
			printf("Critical Error: Failed to set driver codec!\n");
			sangoma_close(&fd);
			exit(-1);
		}

		mtu_mru = sangoma_tdm_get_usr_mtu_mru(fd,&tdm_api);	
		if (mtu_mru < 8){
			printf("Critical Error: Failed to obtain MTU/MRU from the card!\n");
			close (fd);
			exit(-1);
		}

		file = open("sound.raw", O_RDONLY);
		if (file < 0){
			printf("Critical Error: Failed to open sound file!\n");
			sangoma_close(&fd);
			exit(-1);
		}
		status = sangoma_wait_obj_create(&sng_wait, fd, SANGOMA_DEVICE_WAIT_OBJ);
		if (status != SANG_STATUS_SUCCESS) {
			printf("Critial Error: Failed to create sangoma waitable object\n");
			sangoma_close(&fd);
			exit(-1); 
		}
		printf("Playing sound.raw\n");
		for(;;) {

			status = sangoma_waitfor(sng_wait, POLLIN, &flagsout, SANGOMA_WAIT_INFINITE);
			if (status != SANG_STATUS_SUCCESS) {
				perror("sangoma_waitfor");
				printf("Error waiting on sangoma device for input, ret = %d\n", status);
				break;
			}
			
			if((outlen = read(file, outframe, mtu_mru)) != mtu_mru) {
				break;
			}

			if((inlen = sangoma_readmsg_tdm(fd, &hdrframe, sizeof(hdrframe), 
				                        inframe, MAX_BYTES, 0)) < 0) {
				break;
			}

			sangoma_writemsg_tdm(fd, &hdrframe, sizeof(hdrframe), outframe, outlen, 0);
		}

		sangoma_get_full_cfg(fd, &tdm_api);
		close(file);
		close(fd);
		sangoma_wait_obj_delete(&sng_wait);
		printf("Call Handler: Process Finished\n");
		exit(0);
	}
}


/* Event Handlers */

static int on_info(struct sangoma_pri *spri, sangoma_pri_event_t event_type, pri_event *event) 
{
	printf( "number is: %s\n", event->ring.callednum);
	if(strlen(event->ring.callednum) > 3) {
		printf( "final number is: %s\n", event->ring.callednum);
		pri_answer(spri->pri, event->ring.call, 0, 1);
	}
	return 0;
}

static int on_hangup(struct sangoma_pri *spri, sangoma_pri_event_t event_type, pri_event *event) 
{
	pri_hangup(spri->pri, event->hangup.call, event->hangup.cause);
	printf("-- Hanging up channel %d\n", event->hangup.channel);
	if(pidmap[event->hangup.channel-1].pid) {
		kill(pidmap[event->hangup.channel-1].pid, SIGINT);
		pidmap[event->hangup.channel-1].pid = 0;
	}
	return 0;
}

static int on_ring(struct sangoma_pri *spri, sangoma_pri_event_t event_type, pri_event *event) 
{
	printf("-- Ring on channel %d (from %s to %s), answering...\n", event->ring.channel, event->ring.callingnum, event->ring.callednum);
	pri_answer(spri->pri, event->ring.call, event->ring.channel, 1);
	pidmap[event->ring.channel-1].call = event->ring.call;
	//memcpy(&pidmap[event->ring.channel-1].call, event->ring.call, sizeof(q931_call));
	pidmap[event->ring.channel-1].pri = spri->pri;
	launch_channel(spri, event->ring.channel);
	return 0;
}

static int on_restart(struct sangoma_pri *spri, sangoma_pri_event_t event_type, pri_event *event)
{
	printf("-- Restarting channel %d\n", event->restart.channel);
	return 0;
}

static int on_anything(struct sangoma_pri *spri, sangoma_pri_event_t event_type, pri_event *event) 
{
	printf("%s: Caught Event %d (%s)\n", __FUNCTION__, event_type, sangoma_pri_event_str(event_type));
	return 0;
}

/* Generic Reaper */
static void chan_ended(int sig)
{
	int status;
	int x;
	struct rusage rusage;
	pid_t pid;
	pid = wait4(-1, &status, WNOHANG, &rusage);

	printf("-- PID %d ended\n", pid);

	for (x=0;x<SANGOMA_MAX_CHAN_PER_SPAN;x++) {
		if (pid == pidmap[x].pid) {
			pidmap[x].pid = 0;
			if (pidmap[x].pri){
				int err=pri_hangup(pidmap[x].pri, pidmap[x].call, -1);
				printf("Hanging up on PID %i Err=%i\n",pid,err);
			}

			pidmap[x].pri=NULL;
			return;
		}
	}

	if (pid > -1) {
		fprintf(stderr, "--!! Unknown PID %d exited\n", pid);
		return;
	}
}

/* Our Program */
int main(int argc, char *argv[])
{
	struct sangoma_pri spri;
	int debug = 0;
	if (argv[1]) {
		debug = atoi(argv[1]);
	}

	if (sangoma_init_pri(&spri,
						 1,  // span
						 24, // dchan
						 SANGOMA_PRI_SWITCH_DMS100,
						 SANGOMA_PRI_CPE,
						 debug) < 0) {
		return -1;
	}

	SANGOMA_MAP_PRI_EVENT(spri, SANGOMA_PRI_EVENT_ANY, on_anything);
	SANGOMA_MAP_PRI_EVENT(spri, SANGOMA_PRI_EVENT_RING, on_ring);
	SANGOMA_MAP_PRI_EVENT(spri, SANGOMA_PRI_EVENT_HANGUP_REQ, on_hangup);
	SANGOMA_MAP_PRI_EVENT(spri, SANGOMA_PRI_EVENT_INFO_RECEIVED, on_info);
	SANGOMA_MAP_PRI_EVENT(spri, SANGOMA_PRI_EVENT_RESTART, on_restart);

	signal(SIGCHLD, chan_ended);
	sangoma_run_pri(&spri);
	return 0;
}

