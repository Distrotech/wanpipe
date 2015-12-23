/*****************************************************************************
* aft_tdm_hp.c: Example code for HP TDM API Library
*
* Author(s):    Nenad Corbic <ncorbic@sangoma.com>
*
* Copyright:	(c) 2008 Sangoma Technologies Inc.
*
*		This program is free software; you can redistribute it and/or
*		modify it under the terms of the GNU General Public License
*		as published by the Free Software Foundation; either version
*		2 of the License, or (at your option) any later version.
* ============================================================================
* Description:
*
*/

#include "aft_tdm_hp.h"


/*
 * Global flags used by this program
 */
static int system_flag=0;
static int system_threads=0;
static int system_debug=10;

/*
 * Global defines for our program.  Create an array of spans and chans.
 */
span_idx_t span_list[MAX_SPANS];
chan_idx_t chan_list[MAX_SPANS][MAX_CHANS];

/*------------------------------------------
  Utilites
 *-----------------------------------------*/

/*
 * Utility used to print packets
 */
void print_packet(unsigned char *buf, int len)
{
	int x;
	printf("{  | ");
	for (x=0;x<len;x++){
		if (x && x%24 == 0){
			printf("\n  ");
		}
		if (x && x%8 == 0)
			printf(" | ");
		printf("%02x ",buf[x]);
	}
	printf("}\n");
}


/*
 * Utility used to implement print to syslog
 */
void __log_printf(int level, FILE *fp, char *file, const char *func, int line, char *fmt, ...)
{
    char *data;
    int ret = 0;
    va_list ap;

    if (socket < 0) {
		return;
    }

    if (level && level > system_debug) {
		return;
    }

    va_start(ap, fmt);
#ifdef SOLARIS
    data = (char *) smg_malloc(2048);
    vsnprintf(data, 2048, fmt, ap);
#else
    ret = vasprintf(&data, fmt, ap);
#endif
    va_end(ap);
    if (ret == -1) {
		fprintf(stderr, "Memory Error\n");
    } else {
		char date[80] = "";
		struct tm now;
		time_t epoch;

		if (time(&epoch) && localtime_r(&epoch, &now)) {
			strftime(date, sizeof(date), "%Y-%m-%d %T", &now);
		}

#ifdef USE_SYSLOG
		syslog(LOG_DEBUG | LOG_LOCAL2, data);
#else
		if (fp) {
		fprintf(fp, "[%d] %s %s:%d %s() %s", getpid(), date, file, line, func, data);
		}
#endif
		free(data);
    }
#ifndef USE_SYSLOG
    fflush(fp);
#endif
}

void sig_end(int sigid)
{
	printf("%d: Got Signal %i\n",getpid(),sigid);
	aft_clear_flag(system_flag,SYSTEM_RUNNING);
}



/*
 * Main pthread thread used to implement a span->run_span() thread.
 * All this thread does is call span->run_span() method.
 * span->run-span method is defined in libsangoma library and it
 * implements rx/tx/oob on span hw interface.
 */
static void *hp_tdmapi_span_run(void *obj)
{
	span_idx_t *span_idx = (span_idx_t*)obj;
	int err;
	sangoma_hptdm_span_t *span = span_idx->span;

	span_idx->init = 1;

	log_printf(0,NULL,"Starting span %i!\n",span->span_no+1);

	while(aft_test_flag(system_flag,SYSTEM_RUNNING)){

		if (!span->run_span) {
			break;
		}

		err = span->run_span(span);
		if (err) {
			log_printf(0,NULL,"Span %i run_span exited err=%i!\n",span->span_no+1,err);
			usleep(5000);
			continue;
		}

	}

	if (span->close_span) {
		span->close_span(span);
	}

	span_idx->init = 0;

	/* Arbitrary delay - implementation specific */
	sleep(3);
	log_printf(0,NULL,"Stopping span %i!\n",span->span_no+1);

	pthread_exit(NULL);
}

/*
 * Span Thrad Launcher.
 * Spawn a span thread, give the thread span object poiner.
 * This implementation should be user specific.
 */
static int launch_hptdm_api_span_thread(pthread_t *threadid, void *span)
{
	pthread_attr_t attr;
	int result = 0;
	struct sched_param param;

	param.sched_priority = 5;
	result = pthread_attr_init(&attr);
	pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);
	pthread_attr_setschedpolicy(&attr, SCHED_RR);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	pthread_attr_setstacksize(&attr, MGD_STACK_SIZE);

	result = pthread_attr_setschedparam (&attr, &param);

	result = pthread_create(threadid, &attr, hp_tdmapi_span_run, span);
	if (result) {
		log_printf(0, NULL, "%s: Error: Creating Thread! %s\n",
			   __FUNCTION__,strerror(errno));
	}
	pthread_attr_destroy(&attr);

	return result;
}


/*
 * Channel callback function. When channel has a chunk of data
 * ready for us, this function will be called to deliver the
 * rx chunk.  The user should implement the logic here of
 * what to do with that rx chunk.
 *
 * If we return 0 - channel will continue to operate normally.
 * If we return -1 - channel will get closed and we will not
 *                  receive any more data on this channel until
 *                  channel is opened again.
 */

static int media_rx_ready(void *p, char *data, int len)
{
	chan_idx_t *chan_idx = (chan_idx_t *)p;

	if (!chan_idx->init) {
		return -1;
	}

	log_printf(15,NULL,"Chan s%ic%i Rx Data Len %i \n",
				chan_idx->span_no+1,chan_idx->chan_no+1,len);

#if 0
	print_packet((unsigned char*)data,len);
#endif

	/* FIXME: ADD CODE HERE
	Do something with rx chunk of data for this channel
	*/


	/* If we return -1 here the library will close the channel */

	return 0;
}


/*
 * Main pthread thread used to implement a channel thread.
 * Since this is a channel TX thread, user would wait on
 * some UDP socket for data to be transmitted to this channel.
 *
 * The channel thread opens the channel on a span using
 * the library span->open_chan method.  Once the channel
 * object is obtained, this thread can start pushing
 * arbitrary data chunks into the channel.
 *
 */
static void *hp_tdmapi_chan_run(void *obj)
{
	chan_idx_t *chan_idx = (chan_idx_t*)obj;
	int err;
	char data[1024];
	int len=160; /* 20ms worth of ulaw/alaw */

	sangoma_hptdm_span_t *span;
	sangoma_hptdm_chan_reg_t channel_reg;

	/* Grab a span based on the span_no - integer starting from 0 */
	span_idx_t *span_idx = &span_list[chan_idx->span_no];
	span = span_idx->span;

	log_printf(0,NULL,"Starting chan s%ic%i ...\n",
				chan_idx->span_no+1,chan_idx->chan_no+1);

	/* Configure channel registration structure */
	memset(&channel_reg,0,sizeof(channel_reg));
	channel_reg.p = (void*)chan_idx;
	channel_reg.rx_data = media_rx_ready;

	/* Set the local channel index to configured */
	chan_idx->init = 1;

	/* Open a channel based on chan_no integer starting from 0, on a specific span */
	err = span->open_chan(span, &channel_reg, chan_idx->chan_no, &chan_idx->chan);
	if (err){
		log_printf(0,NULL,"Error openeing chan s%ic%i ...\n",
				chan_idx->span_no+1,chan_idx->chan_no+1);
		pthread_exit(NULL);
	}


	memset(data,0,sizeof(data));


	while (aft_test_flag(system_flag,SYSTEM_RUNNING)) {

		/* FIXME: Wait for AUDIO from a UDP socket here
		          once you receive data to tx to a channel
		          use the push() funciton to pass the chunk
		          into the channel.  The size can be of any length
		          up to the MAX CHUNK SIZE */

		/* In this example we dont have a socket to wait on
		   so we just use a delay */
		usleep(1000);

		err=chan_idx->chan->push(chan_idx->chan,
					 data,len);
		/*
		-1 = packet too large, -2 = channel closed, 1 = busy, 0 = tx ok
		*/
		switch (err) {
		case 0:
			/* Data tx ok */
			break;
		case -1:
			/* packet too large */
			break;
		case -2:
			/* channel closed */
			break;
		case 1:
			/* failed to tx, channel busy - try again later - in 20ms or so */
			break;
		}
	}

	/* Once we are done, close the channel */
	span->close_chan(chan_idx->chan);

	/* Set the local channel index to free */
	chan_idx->init = 0;

	pthread_exit(NULL);
}

/*
 * Channel Thrad Launcher.
 * Spawn a channel thread, give the thread chan object poiner.
 * This implementation should be user specific.
 */
static int launch_hptdm_api_chan_thread(pthread_t *threadid, void *chan)
{
    pthread_attr_t attr;
    int result = 0;
    struct sched_param param;

    param.sched_priority = 1;
    result = pthread_attr_init(&attr);
    pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);
    pthread_attr_setschedpolicy(&attr, SCHED_RR);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    pthread_attr_setstacksize(&attr, MGD_STACK_SIZE);

    result = pthread_attr_setschedparam (&attr, &param);

    result = pthread_create(threadid, &attr, hp_tdmapi_chan_run, chan);
    if (result) {
	 log_printf(0, NULL, "%s: Error: Creating Thread! %s\n",
			 __FUNCTION__,strerror(result));
    }
    pthread_attr_destroy(&attr);

    return result;
}


int hptdm_rx_event(void *p, hp_tdmapi_rx_event_t *event)
{
	span_idx_t *span_idx = (span_idx_t*)p;
	int err;
	sangoma_hptdm_span_t *span = span_idx->span;

	if (!span || !span_idx->init) {
		log_printf(0,NULL,"%s: Error Span %i not configured\n",span->span_no);
		return -1;
	} 
#if 0
	switch (event->event_type) {

	case WP_API_EVENT_NONE:

	case WP_API_EVENT_DTMF:

	case WP_API_EVENT_RM_DTMF:

	case WP_API_EVENT_RXHOOK:

	case WP_API_EVENT_RING:

	case WP_API_EVENT_TONE:

	case WP_API_EVENT_RING_DETECT:

	case WP_API_EVENT_ONHOOKTRANSFER:

	case WP_API_EVENT_SETPOLARITY:

	case WP_API_EVENT_BRI_CHAN_LOOPBACK:

	}
#endif

}



/*
 * Start all spans and channels.
 *
 * This exmaple codes automatically starts all spans and channels together.
 * The real world application would start all spans on startup.
 * Jowever the channels would only get started once the signalling stack indicates
 * a call on that specific channel.
 *
 * This implementation should be user specific.
 */
int smg_init_spans(void)
{
	int span,i;
	int err=-1;
	sangoma_hptdm_span_reg_t lib_callback;
	lib_callback.log=__log_printf;

	for (span=0;span<1;span++) {
		span_list[span].span_no=span;

		lib_callback.rx_event = hptdm_rx_event;
		lib_callback.p = (void*)&span_list[span]; 

		span_list[span].span = sangoma_hptdm_api_span_init(span, &lib_callback);
		if (!span_list[span].span) {
			log_printf(0, NULL, "Error: Failed to initialize span %i\n",
					span+1);
			break;
		} else {
			log_printf(0, NULL, "HP TDM API Span: %d configured...\n",
					span+1);

			err=launch_hptdm_api_span_thread(&span_list[span].thread,&span_list[span]);
			if (err) {
				return err;
			}

			for (i=0;i<31;i++) {
				chan_list[span][i].chan_no=i;
				chan_list[span][i].span_no=span;

				err=launch_hptdm_api_chan_thread(&chan_list[span][i].thread,
								 &chan_list[span][i]);
				if (err) {
					return err;
				}
			}
		}
	}

	return err;
}

/*
 * Applicatoin Main Function
 *
 * This implementation should be user specific!
 *
 * Configure and initialize application
 * Set a global sytem flag indicating that app is running.
 *
 * Start all SPANS by calling span init function and
 * launching a thread per span. In span thread calling run_span().
 *
 * Start all channels by launching a thread per channel and
 * calling span->chan_open.  Every 20ms send chunk down each channel.
 *
 * This implementation should be user specific!
 */

int main(int argc, char* argv[])
{
	int err=0;

	nice(-10);

	signal(SIGINT,&sig_end);
	signal(SIGTERM,&sig_end);

	aft_set_flag(system_flag,SYSTEM_RUNNING);

	log_printf(0, NULL, "HP TDM API MAIN Process Starting\n");

	err=smg_init_spans();
	if (err) {
		aft_clear_flag(system_flag,SYSTEM_RUNNING);
	}

	while(aft_test_flag(system_flag,SYSTEM_RUNNING)){
		sleep(1);
	}

	sleep(5);
	log_printf(0, NULL, "HP TDM API MAIN Process Exiting\n");

	return 0;
};
