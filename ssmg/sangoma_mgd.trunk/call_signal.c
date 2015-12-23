/*****************************************************************************
 * call_signal.c -- Signal Specifics
 *
 * Author(s):	Anthony Minessale II <anthmct@yahoo.com>
 *              Nenad Corbic <ncorbic@sangoma.com>
 *
 * Copyright:	(c) 2005 Nenad Corbic 
 *		         Anthony Minessale II
 * 
 *		This program is free software; you can redistribute it and/or
 *		modify it under the terms of the GNU General Public License
 *		as published by the Free Software Foundation; either version
 *		2 of the License, or (at your option) any later version.
 * ============================================================================
 */

#include "call_signal.h"
#include "sangoma_mgd.h"

extern void __log_printf(int level, FILE *fp, char *file, const char *func, int line, char *fmt, ...);
#define clog_printf(level, fp, fmt, ...) __log_printf(level, fp, __FILE__, __FUNCTION__, __LINE__, fmt, ##__VA_ARGS__)

struct call_signal_map {
	uint32_t event_id;
	char *name;
};

static struct call_signal_map call_signal_table[] = {
	{SIGBOOST_EVENT_CALL_START, "CALL_START"},
	{SIGBOOST_EVENT_CALL_START_ACK, "CALL_START_ACK"},
	{SIGBOOST_EVENT_CALL_START_NACK, "CALL_START_NACK"},
	{SIGBOOST_EVENT_CALL_START_NACK_ACK, "CALL_START_NACK_ACK"},
	{SIGBOOST_EVENT_CALL_ANSWERED, "CALL_ANSWERED"},
	{SIGBOOST_EVENT_CALL_STOPPED, "CALL_STOPPED"},
	{SIGBOOST_EVENT_CALL_STOPPED_ACK, "CALL_STOPPED_ACK"},
	{SIGBOOST_EVENT_SYSTEM_RESTART, "SYSTEM_RESTART"},
	{SIGBOOST_EVENT_SYSTEM_RESTART_ACK, "SYSTEM_RESTART_ACK"},
	{SIGBOOST_EVENT_HEARTBEAT, "HEARTBEAT"},
	{SIGBOOST_EVENT_INSERT_CHECK_LOOP, "LOOP START"},
	{SIGBOOST_EVENT_REMOVE_CHECK_LOOP, "LOOP STOP"},
	{SIGBOOST_EVENT_DIGIT_IN, "DIGIT_IN"},
	{SIGBOOST_EVENT_CALL_PROGRESS, "CALL_PROGRESS"}
}; 

#define USE_SCTP 1

static int create_udp_socket(call_signal_connection_t *mcon, char *local_ip, int local_port, char *ip, int port)
{
	int rc;
	struct hostent *result, *local_result;
	char buf[512], local_buf[512];
	int err = 0;

	memset(&mcon->remote_hp, 0, sizeof(mcon->remote_hp));
	memset(&mcon->local_hp, 0, sizeof(mcon->local_hp));
#ifdef USE_SCTP
	mcon->socket = socket(AF_INET, SOCK_SEQPACKET, IPPROTO_SCTP);
#else
	mcon->socket = socket(AF_INET, SOCK_DGRAM, 0);
#endif
 
	clog_printf(SMG_LOG_DEBUG_MISC,mcon->log,"Creating L=%s:%d R=%s:%d\n",
			local_ip,local_port,ip,port);

	if (mcon->socket >= 0) {
		int flag=1;
		gethostbyname_r(ip, &mcon->remote_hp, buf, sizeof(buf), &result, &err);
		gethostbyname_r(local_ip, &mcon->local_hp, local_buf, sizeof(local_buf), &local_result, &err);
		if (result && local_result) {
			mcon->remote_addr.sin_family = mcon->remote_hp.h_addrtype;
			memcpy((char *) &mcon->remote_addr.sin_addr.s_addr, mcon->remote_hp.h_addr_list[0], mcon->remote_hp.h_length);
			mcon->remote_addr.sin_port = htons(port);

			mcon->local_addr.sin_family = mcon->local_hp.h_addrtype;
			memcpy((char *) &mcon->local_addr.sin_addr.s_addr, mcon->local_hp.h_addr_list[0], mcon->local_hp.h_length);
			mcon->local_addr.sin_port = htons(local_port);

#ifdef USE_SCTP
			setsockopt(mcon->socket, IPPROTO_SCTP, SCTP_NODELAY, 
				   (char *)&flag, sizeof(int));
#endif

		
			rc = fcntl(mcon->socket, F_GETFL);
			if ( rc < 0 ) {
				close(mcon->socket);
				mcon->socket = -1;
				return mcon->socket;
			}
			
			rc |= O_NONBLOCK;
			rc = fcntl(mcon->socket, F_SETFL, rc);
			if ( rc < 0 ) {
				close(mcon->socket);
				mcon->socket = -1;
				return mcon->socket;
			}

			if ((rc = bind(mcon->socket, 
				  (struct sockaddr *) &mcon->local_addr,
				   sizeof(mcon->local_addr))) < 0) {
				close(mcon->socket);
				mcon->socket = -1;
			} else {
#ifdef USE_SCTP
				rc=listen(mcon->socket,100);
				if (rc) {
					close(mcon->socket);
					mcon->socket = -1;
				}
#endif
			}
		}
	}

	return mcon->socket;
}

int call_signal_connection_close(call_signal_connection_t *mcon)
{
	close(mcon->socket);
	mcon->socket = -1;
	memset(mcon, 0, sizeof(*mcon));

	return 0;
}

int call_signal_connection_open(call_signal_connection_t *mcon, char *local_ip, int local_port, char *ip, int port)
{
	create_udp_socket(mcon, local_ip, local_port, ip, port);
	return mcon->socket;
}


static int smg_event_dbg=SMG_LOG_BOOST;
static void clog_print_event_call(call_signal_connection_t *mcon,call_signal_event_t *event, int priority, int dir)
{
	clog_printf((event->event_id==SIGBOOST_EVENT_HEARTBEAT)?SMG_LOG_DEBUG_CALL:smg_event_dbg, mcon->log,
                           "%s EVENT (%s): %s:(%X) [s%dc%d] Tg=%i CSid=%i Seq=%i Cn=[%s] Cd=[%s] Ci=[%s]\n",
			   dir ? "TX":"RX",
			   priority ? "P":"N",	
                           call_signal_event_id_name(event->event_id),
                           event->event_id,
                           event->span+1,
                           event->chan+1,
						   event->trunk_group,
                           event->call_setup_id,
                           event->fseqno,
			   			   strlen(event->calling_name)?event->calling_name:"N/A",
                           (event->called_number_digits_count ? (char *) event->called_number_digits : "N/A"),
                           (event->calling_number_digits_count ? (char *) event->calling_number_digits : "N/A"));
}
static void clog_print_event_short(call_signal_connection_t *mcon,short_signal_event_t *event, int priority, int dir)
{
	clog_printf((event->event_id==SIGBOOST_EVENT_HEARTBEAT)?SMG_LOG_DEBUG_CALL:smg_event_dbg, mcon->log,
                           "%s EVENT (%s): %s:(%X) [s%dc%d] Rc=%i CSid=%i Seq=%i \n",
			   dir ? "TX":"RX",
			   priority ? "P":"N",	
                           call_signal_event_id_name(event->event_id),
                           event->event_id,
                           event->span+1,
                           event->chan+1,
                           event->release_cause,
                           event->call_setup_id,
                           event->fseqno);
}


call_signal_event_t *call_signal_connection_read(call_signal_connection_t *mcon, int iteration)
{
	unsigned int fromlen = sizeof(struct sockaddr_in);
#if 0
	call_signal_event_t *event = &mcon->event;
#endif
	int bytes = 0;
	int msg_ok=0;

	bytes = recvfrom(mcon->socket, &mcon->event, sizeof(mcon->event), MSG_DONTWAIT, 
			(struct sockaddr *) &mcon->local_addr, &fromlen);

	clog_printf(SMG_LOG_DEBUG_8,mcon->log,"RX EVENT len=%i sock=%i\n",bytes,mcon->socket);

	/* Must check for < 0 cannot rely on bytes > MIN_SIZE_... compiler issue */
	if (bytes < 0) {
		msg_ok=0;

	} else if ((bytes >= MIN_SIZE_CALLSTART_MSG) && boost_full_event(mcon->event.event_id)) {
		msg_ok=1;
		
	} else if (bytes == sizeof(short_signal_event_t)) {
		msg_ok=1;

	} else {
		msg_ok=0;
	}

	if (msg_ok){

#if 0
		if (mcon->event.event_id == SIGBOOST_EVENT_SYSTEM_RESTART) {
			clog_printf(SMG_LOG_ALL,mcon->log,"Rx sync ok\n");
			woomera_set_flag(&server.master_connection, WFLAG_SYSTEM_RESET);
                        mcon->rxseq=mcon->event.fseqno;
                        return &mcon->event;
		}
		if (woomera_test_flag(&server.master_connection, WFLAG_SYSTEM_RESET)){
			if (mcon->event.event_id == SIGBOOST_EVENT_SYSTEM_RESTART_ACK) {
				clog_printf(SMG_LOG_ALL,mcon->log,"Rx sync ok\n");
				mcon->rxseq=mcon->event.fseqno;
				return &mcon->event;
			}
			errno=EAGAIN;
			clog_printf(SMG_LOG_ALL,mcon->log,"Waiting for rx sync...\n");
			return NULL;
		}
#endif
		mcon->txwindow = mcon->txseq - mcon->event.bseqno;

#ifndef SANGOMA_UNIT_TESTER		
		mcon->rxseq++;
#endif
	
		clog_printf(SMG_LOG_DEBUG_6, mcon->log, "RX (N) CMD %i Exp_RX = %i Got_RX =%i",
				mcon->event.event_id,mcon->rxseq,mcon->event.fseqno);

		if (mcon->rxseq != mcon->event.fseqno) {
			clog_printf(SMG_LOG_ALL, mcon->log, 
				"------------------------------------------\n");
			clog_printf(SMG_LOG_ALL, mcon->log, 
				"Critical Error: Invalid Sequence Number Event=%i Rx(N) Expect=%i Rx=%i\n",
				mcon->event.event_id,mcon->rxseq,mcon->event.fseqno);
			clog_printf(SMG_LOG_ALL, mcon->log, 
				"------------------------------------------\n");

			/* NC: Recover from a sequence error */
            mcon->rxseq = mcon->event.fseqno;
		}

#ifdef SANGOMA_UNIT_TESTER
		if (mcon->event.event_id == SIGBOOST_EVENT_SYSTEM_RESTART_ACK) {
			mcon->rxseq=0;
		} else {
			mcon->rxseq++;
		}
#endif

		if  (mcon->event.event_id == SIGBOOST_EVENT_CALL_START ||
			 mcon->event.event_id == SIGBOOST_EVENT_DIGIT_IN) {
			clog_print_event_call(mcon,&mcon->event, 0, 0);
		} else {
			clog_print_event_short(mcon,(short_signal_event_t*)&mcon->event, 0, 0);
		}

#if 0
/* Debugging only not to be used in production because span/chan can be invalid */
	   	if (mcon->event.span < 0 || mcon->event.chan < 0 || mcon->event.span > max_spans || mcon->event.chan > max_spans) {
                	clog_printf(SMG_LOG_ALL, mcon->log,
                        	"------------------------------------------\n");
                	clog_printf(SMG_LOG_ALL, mcon->log,
                        	"Critical Error: RX Cmd=%s Invalid Span=%i Chan=%i\n",
                        	call_signal_event_id_name(event->event_id), event->span,event->chan);
                	clog_printf(SMG_LOG_ALL, mcon->log,
                        	"------------------------------------------\n");
		
			errno=EAGAIN;
                	return NULL;
        	}
#endif

		return &mcon->event;
	} else {
		if (iteration == 0) {
                	clog_printf(SMG_LOG_ALL, mcon->log,
                        	"------------------------------------------\n");
                	clog_printf(SMG_LOG_ALL, mcon->log,
                        	"Critical Error: Invalid Event length from boost rxlen=%i expected=%i\n",
					bytes, sizeof(mcon->event));
                	clog_printf(SMG_LOG_ALL, mcon->log,
                        	"------------------------------------------\n");
		}
	}

	return NULL;
}

call_signal_event_t *call_signal_connection_readp(call_signal_connection_t *mcon, int iteration)
{
	unsigned int fromlen = sizeof(struct sockaddr_in);
#if 0
	call_signal_event_t *event = &mcon->event;
#endif
	int bytes = 0;

	bytes = recvfrom(mcon->socket, &mcon->event, sizeof(mcon->event), MSG_DONTWAIT, 
			(struct sockaddr *) &mcon->local_addr, &fromlen);

	if (bytes) {
		if (mcon->event.version != SIGBOOST_VERSION) {
			clog_printf(SMG_LOG_ALL, mcon->log,
                        	"------------------------------------------\n");
                	clog_printf(SMG_LOG_ALL, mcon->log,
                        	"Critical Error: Invalid Boost Version number %i, Expecting %i\n",
								mcon->event.version,SIGBOOST_VERSION);
                	clog_printf(SMG_LOG_ALL, mcon->log,
                        	"------------------------------------------\n");
		}
	}

	if (bytes == sizeof(short_signal_event_t)) {

		clog_print_event_short(mcon,(short_signal_event_t*)&mcon->event, 1, 0);

#if 0
	/* Debugging only not to be used in production because span/chan can be invalid */
               if (mcon->event.span < 0 || mcon->event.chan < 0 || mcon->event.span > max_spans || mcon->event.chan > max_spans) {
                        clog_printf(SMG_LOG_ALL, mcon->log,
                                "------------------------------------------\n");
                        clog_printf(SMG_LOG_ALL, mcon->log,
                                "Critical Error: RXp Cmd=%s Invalid Span=%i Chan=%i\n",
                                call_signal_event_id_name(event->event_id), event->span,event->chan);
                        clog_printf(SMG_LOG_ALL, mcon->log,
                                "------------------------------------------\n");

                        errno=EAGAIN;
                        return NULL;
                }
#endif

		return &mcon->event;

	} else {
		if (iteration == 0) {
                	clog_printf(SMG_LOG_ALL, mcon->log,
                        	"------------------------------------------\n");
                	clog_printf(SMG_LOG_ALL, mcon->log,
                        	"Critical Error: PQ Invalid Event lenght from boost rxlen=%i evsz=%i\n",
					bytes, sizeof(mcon->event));
                	clog_printf(SMG_LOG_ALL, mcon->log,
                        	"------------------------------------------\n");
		}
	}

	return NULL;
}


int call_signal_connection_writep(call_signal_connection_t *mcon, call_signal_event_t *event)
{
	int err;
	int event_size=MIN_SIZE_CALLSTART_MSG+event->isup_in_rdnis_size;

	if (!event) {
		clog_printf(SMG_LOG_ALL, mcon->log, "Critical Error: No Event Device\n");
		return -EINVAL;
	}

	if (!boost_full_event(event->event_id)) {
		event_size=sizeof(short_signal_event_t);
	}	
#if 0
	if (event->span < 0 || event->chan < 0 || event->span > max_spans || event->chan > max_chans) {
		clog_printf(SMG_LOG_ALL, mcon->log, 
			"------------------------------------------\n");
		clog_printf(SMG_LOG_ALL, mcon->log, 
			"Critical Error: TX Cmd=%s Invalid Span=%i Chan=%i\n",
			call_signal_event_id_name(event->event_id), event->span,event->chan);
		clog_printf(SMG_LOG_ALL, mcon->log, 
			"------------------------------------------\n");

		return -1;
	}
#endif

	event->version=SIGBOOST_VERSION;
	/* Sendto is thread safe so no lock needed */
	err=sendto(mcon->socket, event, event_size, 0, 
		   (struct sockaddr *) &mcon->remote_addr, sizeof(mcon->remote_addr));

	if (err != event_size) {
		err = -1;
	} else {
		err = 0;
	}

	if (boost_full_event(event->event_id)) {
		clog_print_event_call(mcon,event, 1, 1);
	} else {
		clog_print_event_short(mcon,(short_signal_event_t*)event, 1, 1);
	}
	return err;
}



int call_signal_connection_write(call_signal_connection_t *mcon, call_signal_event_t *event)
{
	int err;
	int event_size=sizeof(call_signal_event_t);


	if (!event) {
		clog_printf(SMG_LOG_ALL, mcon->log, "Critical Error: No Event Device\n");
		return -EINVAL;
	}

	/* span chan are unsigned no point of checking if < 0 */
	if (event->span > max_spans || event->chan > max_chans) {
		clog_printf(SMG_LOG_ALL, mcon->log, 
			"------------------------------------------\n");
		clog_printf(SMG_LOG_ALL, mcon->log, 
			"Critical Error: TX Cmd=%s Invalid Span=%i Chan=%i\n",
			call_signal_event_id_name(event->event_id), event->span,event->chan);
		clog_printf(SMG_LOG_ALL, mcon->log, 
			"------------------------------------------\n");

		return -1;
	}


	if (!boost_full_event(event->event_id)) {
		event_size=sizeof(short_signal_event_t);
	}	

	if (woomera_test_flag(&server.master_connection, WFLAG_SYSTEM_RESET) && event->event_id != SIGBOOST_EVENT_SYSTEM_RESTART_ACK) { 
		clog_printf(SMG_LOG_ALL, mcon->log, "Error: Boost connection in reset ignoring packet\n");
		return 0;
	}

	pthread_mutex_lock(&mcon->lock);

	if (event->event_id == SIGBOOST_EVENT_SYSTEM_RESTART_ACK) {
		mcon->txseq=0;
		mcon->rxseq=0;
		event->fseqno=0;
	} else {
#ifdef SANGOMA_UNIT_TESTER
		event->fseqno=++mcon->txseq;
#else
		event->fseqno=mcon->txseq++;
#endif
	}

	event->bseqno=mcon->rxseq;
	event->version=SIGBOOST_VERSION;
	
	err=sendto(mcon->socket, event, event_size, 0, 
		   (struct sockaddr *) &mcon->remote_addr, sizeof(mcon->remote_addr));
	
	if (err != event_size) {
		mcon->txseq--;	
	}
	pthread_mutex_unlock(&mcon->lock);

	if (err != event_size) {
		err = -1;
	} else {
		err = 0;
	}
	
	if (boost_full_event(event->event_id)) {
		clog_print_event_call(mcon,event, 0, 1);
	} else {
		clog_print_event_short(mcon,(short_signal_event_t*)event, 0, 1);
	}

	return err;
}

void call_signal_call_init(call_signal_event_t *event, char *calling, char *called, int setup_id)
{
	memset(event, 0, sizeof(*event));
	event->event_id = SIGBOOST_EVENT_CALL_START;

	if (calling) {
		strncpy((char*)event->calling_number_digits, calling, sizeof(event->calling_number_digits)-1);
		event->calling_number_digits_count = strlen(calling);
	}

	if (called) {
		strncpy((char*)event->called_number_digits, called, sizeof(event->called_number_digits)-1);
		event->called_number_digits_count = strlen(called);
	}
		
	event->call_setup_id = setup_id;
	
}

void call_signal_event_init(short_signal_event_t *event, call_signal_event_id_t event_id, int chan, int span) 
{
	memset(event, 0, sizeof(*event));
	event->event_id = event_id;
	event->chan = chan;
	event->span = span;
}

char *call_signal_event_id_name(uint32_t event_id)
{
	int x;
	char *ret = NULL;

	for (x = 0 ; x < sizeof(call_signal_table)/sizeof(struct call_signal_map); x++) {
		if (call_signal_table[x].event_id == event_id) {
			ret = call_signal_table[x].name;
			break;
		}
	}

	return ret;
}
