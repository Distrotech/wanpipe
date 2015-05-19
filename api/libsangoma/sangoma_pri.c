/*****************************************************************************
 * sangoma_pri.c libpri Sangoma integration
 *
 * Author(s):	Anthony Minessale II <anthmct@yahoo.com>
 *              Nenad Corbic <ncorbic@sangoma.com>
 *
 * Copyright:	(c) 2005 Anthony Minessale II
 *
 *		This program is free software; you can redistribute it and/or
 *		modify it under the terms of the GNU General Public License
 *		as published by the Free Software Foundation; either version
 *		2 of the License, or (at your option) any later version.
 * ============================================================================
 */
#include <libsangoma.h>
#include <sangoma_pri.h>
#ifndef HAVE_GETTIMEOFDAY

#ifdef WIN32
#include <mmsystem.h>

static __inline int gettimeofday(struct timeval *tp, void *nothing)
{
#ifdef WITHOUT_MM_LIB
	SYSTEMTIME st;
	time_t tt;
	struct tm tmtm;
	/* mktime converts local to UTC */
	GetLocalTime (&st);
	tmtm.tm_sec = st.wSecond;
	tmtm.tm_min = st.wMinute;
	tmtm.tm_hour = st.wHour;
	tmtm.tm_mday = st.wDay;
	tmtm.tm_mon = st.wMonth - 1;
	tmtm.tm_year = st.wYear - 1900;  tmtm.tm_isdst = -1;
	tt = mktime (&tmtm);
	tp->tv_sec = tt;
	tp->tv_usec = st.wMilliseconds * 1000;
#else
	/**
	** The earlier time calculations using GetLocalTime
	** had a time resolution of 10ms.The timeGetTime, part
	** of multimedia apis offer a better time resolution
	** of 1ms.Need to link against winmm.lib for this
	**/
	unsigned long Ticks = 0;
	unsigned long Sec =0;
	unsigned long Usec = 0;
	Ticks = timeGetTime();

	Sec = Ticks/1000;
	Usec = (Ticks - (Sec*1000))*1000;
	tp->tv_sec = Sec;
	tp->tv_usec = Usec;
#endif /* WITHOUT_MM_LIB */
	(void)nothing;
	return 0;
}
#endif /* WIN32 */
#endif /* HAVE_GETTIMEOFDAY */

static struct sangoma_pri_event_list SANGOMA_PRI_EVENT_LIST[] = {
	{0, SANGOMA_PRI_EVENT_ANY, "ANY"},
	{1, SANGOMA_PRI_EVENT_DCHAN_UP, "DCHAN_UP"},
	{2, SANGOMA_PRI_EVENT_DCHAN_DOWN, "DCHAN_DOWN"},
	{3, SANGOMA_PRI_EVENT_RESTART, "RESTART"},
	{4, SANGOMA_PRI_EVENT_CONFIG_ERR, "CONFIG_ERR"},
	{5, SANGOMA_PRI_EVENT_RING, "RING"},
	{6, SANGOMA_PRI_EVENT_HANGUP, "HANGUP"},
	{7, SANGOMA_PRI_EVENT_RINGING, "RINGING"},
	{8, SANGOMA_PRI_EVENT_ANSWER, "ANSWER"},
	{9, SANGOMA_PRI_EVENT_HANGUP_ACK, "HANGUP_ACK"},
	{10, SANGOMA_PRI_EVENT_RESTART_ACK, "RESTART_ACK"},
	{11, SANGOMA_PRI_EVENT_FACNAME, "FACNAME"},
	{12, SANGOMA_PRI_EVENT_INFO_RECEIVED, "INFO_RECEIVED"},
	{13, SANGOMA_PRI_EVENT_PROCEEDING, "PROCEEDING"},
	{14, SANGOMA_PRI_EVENT_SETUP_ACK, "SETUP_ACK"},
	{15, SANGOMA_PRI_EVENT_HANGUP_REQ, "HANGUP_REQ"},
	{16, SANGOMA_PRI_EVENT_NOTIFY, "NOTIFY"},
	{17, SANGOMA_PRI_EVENT_PROGRESS, "PROGRESS"},
	{18, SANGOMA_PRI_EVENT_KEYPAD_DIGIT, "KEYPAD_DIGIT"}
};


const char *sangoma_pri_event_str(sangoma_pri_event_t event_id)
{ 
	return SANGOMA_PRI_EVENT_LIST[event_id].name;
}

static int __pri_sangoma_read(struct pri *pri, void *buf, int buflen)
{
	unsigned char tmpbuf[sizeof(wp_tdm_api_rx_hdr_t)];
	

	/* NOTE: This code will be different for A104 
	 *       A104   receives data + 2byte CRC + 1 byte flag 
	 *       A101/2 receives data only */ 
	
	int res = sangoma_readmsg_socket((sng_fd_t)pri_fd(pri), 
					 tmpbuf, sizeof(wp_tdm_api_rx_hdr_t), 
					 buf, buflen, 
					 0);
	if (res > 0){
#ifdef WANPIPE_LEGACY_A104
		/* Prior 2.3.4 release A104 API passed up
                 * 3 extra bytes: 2 CRC + 1 FLAG,
                 * PRI is expecting only 2 CRC bytes, thus we
                 * must remove 1 flag byte */
		res--;
#else
		/* Add 2 byte CRC and set it to ZERO */ 
		memset(&((unsigned char*)buf)[res],0,2);
		res+=2;
#endif
	}else{
		res=0;
	}

	return res;
}

static int __pri_sangoma_write(struct pri *pri, void *buf, int buflen)
{
	unsigned char tmpbuf[sizeof(wp_tdm_api_rx_hdr_t)];
	int res;


	memset(&tmpbuf[0],0,sizeof(wp_tdm_api_rx_hdr_t));	

	if (buflen < 1){
		/* HDLC Frame must be greater than 2byte CRC */
		fprintf(stderr,"%s: Got short frame %i\n",__FUNCTION__,buflen);
		return 0;
	}

	/* FIXME: This might cause problems with other libraries
	 * We must remove 2 bytes from buflen because
	 * libpri sends 2 fake CRC bytes */
	res=sangoma_sendmsg_socket((sng_fd_t)pri_fd(pri),
				   tmpbuf, sizeof(wp_tdm_api_rx_hdr_t),
				   buf, (unsigned short)buflen-2,
				   0);	
	if (res > 0){
		res=buflen;
	}
	
	return res;
}

int sangoma_init_pri(struct sangoma_pri *spri, int span, int dchan, int swtype, int node, int debug)
{
	int ret = -1;
	sng_fd_t dfd = 0;

	memset(spri, 0, sizeof(struct sangoma_pri));

	if((dfd = sangoma_open_api_span_chan(span, dchan)) < 0) {
		fprintf(stderr, "Unable to open DCHAN %d for span %d (%s)\n", dchan, span, strerror(errno));

	} else {
		sangoma_status_t status = sangoma_wait_obj_create(&spri->dchan_wait, dfd, SANGOMA_DEVICE_WAIT_OBJ);
		if (status != SANG_STATUS_SUCCESS) {
			fprintf(stderr, "Unable to create sangoma waitable object\n");
			return -1;
		}
		if ((spri->pri = pri_new_cb((int)dfd, node, swtype, __pri_sangoma_read, __pri_sangoma_write, NULL))){
			spri->span = span;
			pri_set_debug(spri->pri, debug);
			ret = 0;
		} else {
			fprintf(stderr, "Unable to create PRI\n");
		}
	}
	return ret;
}


int sangoma_one_loop(struct sangoma_pri *spri)
{
	sangoma_status_t status;
	struct timeval now = {0,0}, *next;
	uint32_t outflags = 0;
	pri_event *event;
	long waitms = 0;
	
	if (spri->on_loop) {
		spri->on_loop(spri);
	}

	if ((next = pri_schedule_next(spri->pri))) {
		gettimeofday(&now, NULL);
		now.tv_sec = next->tv_sec - now.tv_sec;
		now.tv_usec = next->tv_usec - now.tv_usec;
		if (now.tv_usec < 0) {
			now.tv_usec += 1000000;
			now.tv_sec -= 1;
		}
		if (now.tv_sec < 0) {
			now.tv_sec = 0;
			now.tv_usec = 0;
		}
	}
	waitms = next ? (now.tv_sec * 1000) + (now.tv_usec / 1000) : SANGOMA_WAIT_INFINITE;
	status = sangoma_waitfor(spri->dchan_wait, POLLIN, &outflags, waitms);
	event = NULL;

	if (status == SANG_STATUS_APIPOLL_TIMEOUT) {
		event = pri_schedule_run(spri->pri);
	} else if (status == SANG_STATUS_SUCCESS) {
		event = pri_check_event(spri->pri);
	} else {
		return -1;
	}

	if (event) {
		event_handler handler;
		/* 0 is catchall event handler */
		if ((handler = spri->eventmap[event->e] ? spri->eventmap[event->e] : spri->eventmap[0] ? spri->eventmap[0] : NULL)) {
			handler(spri, event->e, event);
		} else {
			fprintf(stderr,"No event handler found for event %d.\n", event->e);
		}
	}

	return 0;
}

int sangoma_run_pri(struct sangoma_pri *spri)
{
	int ret = 0;

	for (;;){
		ret = sangoma_one_loop(spri);
		if (ret < 0){
			/* the printer error may not be accurate if sangoma_waitfor does anything that clears up errno */
			perror("Sangoma Run Pri");
			break;		
		}
	}
	sangoma_wait_obj_delete(&spri->dchan_wait);
	return ret;

}

