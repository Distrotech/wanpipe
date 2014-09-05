/*********************************************************************************
 * sangoma_mgd.c --  Sangoma Media Gateway Daemon for Sangoma/Wanpipe Cards
 *
 * Copyright 05-07, Nenad Corbic <ncorbic@sangoma.com>
 *		    Anthony Minessale II <anthmct@yahoo.com>
 *
 * This program is free software, distributed under the terms of
 * the GNU General Public License
 * 
 * =============================================
 * v1.20 Nenad Corbic <ncorbic@sangoma.com>
 *	Added option for Auto ACM response mode.
 *
 * v1.19 Nenad Corbic <ncorbic@sangoma.com>
 *	Configurable DTMF
 *      Bug fix in release codes (all ckt busy)
 *
 * v1.18 Nenad Corbic <ncorbic@sangoma.com>
 *	Added new rel cause support based on
 *	digits instead of strings.
 *
 * v1.17 Nenad Corbic <ncorbic@sangoma.com>
 *	Added session support
 *
 * v1.16 Nenad Corbic <ncorbic@sangoma.com>
 *      Added hwec disable on loop ccr 
 *
 * v1.15 Nenad Corbic <ncorbic@sangoma.com>
 *      Updated DTMF Locking 
 *	Added delay between digits
 *
 * v1.14 Nenad Corbic <ncorbic@sangoma.com>
 *	Updated DTMF Library
 *	Fixed DTMF synchronization
 *
 * v1.13 Nenad Corbic <ncorbic@sangoma.com>
 *	Woomera OPAL Dialect
 *	Added Congestion control
 *	Added SCTP
 *	Added priority ISUP queue
 *	Fixed presentation
 *
 * v1.12 Nenad Corbic <ncorbic@sangoma.com>
 *	Fixed CCR 
 *	Removed socket shutdown on end call.
 *	Let Media thread shutodwn sockets.
 *
 * v1.11 Nenad Corbic <ncorbic@sangoma.com>
 * 	Fixed Remote asterisk/woomera connection
 *	Increased socket timeouts
 *
 * v1.10 Nenad Corbic <ncorbic@sangoma.com>
 *	Added Woomera OPAL dialect.
 *	Start montor thread in priority
 *
 * v1.9 Nenad Corbic <ncorbic@sangoma.com>
 * 	Added Loop mode for ccr
 *	Added remote debug enable
 *	Fixed syslog logging.
 *
 * v1.8 Nenad Corbic <ncorbic@sangoma.com>
 *	Added a ccr loop mode for each channel.
 *	Boost can set any channel in loop mode
 *
 * v1.7 Nenad Corbic <ncorbic@sangoma.com>
 *      Pass trunk group number to incoming call
 *      chan woomera will use it to append to context
 *      name. Added presentation feature.
 *
 * v1.6	Nenad Corbic <ncorbic@sangoma.com>
 *      Use only ALAW and MLAW not SLIN.
 *      This reduces the load quite a bit.
 *      Send out ALAW/ULAW format on HELLO message.
 *      RxTx Gain is done now in chan_woomera.
 *
 * v1.5	Nenad Corbic <ncorbic@sangoma.com>
 * 	Bug fix in START_NACK_ACK handling.
 * 	When we receive START_NACK we must alwasy pull tank before
 *      we send out NACK_ACK this way we will not try to send NACK 
 *      ourself.
 *********************************************************************************/

#include "sangoma_mgd.h"
#include "unit.h"


#define USE_SYSLOG 1
#define CODEC_LAW_DEFAULT 1


static struct woomera_interface woomera_dead_dev;

#if 0
#define DOTRACE
#endif

struct woomera_server server; 

#define SMG_VERSION	"v1.20"

/* enable early media */
#if 1
#define WOOMERA_EARLY_MEDIA 1
#endif

#define SMG_DTMF_ON 	60
#define SMG_DTMF_OFF 	10
#define SMG_DTMF_RATE  	8000

#if 0
#warning "NENAD: MEDIA SHUTDOWN"
#define MEDIA_SOCK_SHUTDOWN 1	
#endif


#ifdef DOTRACE
static int tc = 0;
#endif

#if 0
#warning "NENAD: HPTDM API"
#define WP_HPTDM_API 1
#else
#undef WP_HPTDM_API 
#endif

#ifdef  WP_HPTDM_API
hp_tdm_api_span_t *hptdmspan[WOOMERA_MAX_SPAN];
#endif

#undef SMG_CALLING_NAME

static int gcnt=0;


void handle_call_answer(short_signal_event_t *event)
{
    	

}

void handle_call_start_ack(short_signal_event_t *event)
{
    	

}

void handle_call_start_nack(short_signal_event_t *event)
{
 
	if (event->call_setup_id == 0) {
		log_printf(0,server.log, "FATAL: ERROR NACK Incoming id == 0\n");
		woomera_clear_flag(&server.master_connection, WFLAG_RUNNING);
		exit(0);
	}

	if (server.holding_tank[event->call_setup_id] == NULL) {
		log_printf(0,server.log, "FATAL: ERROR NACK Incoming id %i already empty\n",
			event->call_setup_id);
		woomera_clear_flag(&server.master_connection, WFLAG_RUNNING);
		exit(0);
	}

	gcnt=0;
	server.holding_tank[event->call_setup_id]=NULL;

#if 1
	isup_exec_command(0, 
			  0, 
			  event->call_setup_id,
			  SIGBOOST_EVENT_CALL_START_NACK_ACK,
			  0);
#endif

}


void handle_remove_loop(short_signal_event_t *event)
{
	


}

void handle_call_loop_start(call_signal_event_t *event)
{


}

void handle_call_start(call_signal_event_t *oevent)
{
	short_signal_event_t sevent_s;
	short_signal_event_t *event = &sevent_s;

	memset(event,0,sizeof(short_signal_event_t));
	memcpy(event,oevent,sizeof(short_signal_event_t));

	if (event->call_setup_id == 0) {
		log_printf(0,server.log, "FATAL: ERROR Incoming id == 0\n");
		woomera_clear_flag(&server.master_connection, WFLAG_RUNNING);
		exit(0);
	}

	if (server.holding_tank[event->call_setup_id]) {
		log_printf(0,server.log, "FATAL: ERROR Incoming id %i already used\n",
			event->call_setup_id);
		woomera_clear_flag(&server.master_connection, WFLAG_RUNNING);
		exit(0);
	}

gcnt++;
if (gcnt > 80) {
	log_printf(0,server.log, "TIMING OUT THE CALL START :) with setup id %i \n",
			event->call_setup_id);
	return;
}


#if 1

	server.holding_tank[event->call_setup_id] = &woomera_dead_dev;

	event->event_id=SIGBOOST_EVENT_CALL_START_NACK;
	event->release_cause=17;//SIGBOOST_CALL_SETUP_NACK_ALL_CKTS_BUSY;
	isup_exec_event((call_signal_event_t*)event);
#endif

#if 0
	//sleep(20);
	event->span=0;
	event->chan=0;
	event->event_id=SIGBOOST_EVENT_CALL_START_ACK;
	isup_exec_event(event);
#endif

#if 0
	event->span=0;
	event->chan=0;
	event->event_id=SIGBOOST_EVENT_CALL_STOPPED;
	event->release_cause=17;
	isup_exec_event(event);
#endif

#if 0
	sleep(20);
	event->event_id=SIGBOOST_EVENT_CALL_ANSWERED;
	isup_exec_event(event);
#endif



}

void handle_gap_abate(short_signal_event_t *event)
{
	log_printf(0, server.log, "NOTICE: GAP Cleared!\n", 
			event->span+1, event->chan+1);
	smg_clear_ckt_gap();
}


void handle_restart(call_signal_connection_t *mcon, short_signal_event_t *event)
{
	int i;
	for (i=0;i<CORE_TANK_LEN;i++) {
		server.holding_tank[event->call_setup_id]=NULL;
	}
	gcnt=0;

	isup_exec_commandp(event->span,
                          event->chan,
                        -1,
			SIGBOOST_EVENT_SYSTEM_RESTART_ACK,
                        0);
	
	mcon->rxseq=1;

	usleep(500000);
	
	isup_exec_commandp(event->span,
                          event->chan,
                        -1,
			SIGBOOST_EVENT_SYSTEM_RESTART,
                        0);

	return;
}

void handle_restart_ack(call_signal_connection_t *mcon, short_signal_event_t *event)
{
	int sp,ch,retry=0;
	mcon->rxseq=1;

#if 0
	initiate_call_unit_start();
#endif

#if 1
retry_loop:
	for (sp=0;sp<16;sp++) {

		for (ch=0;ch<31;ch++) {

			if (sp==0 && ch==15) {
				continue;
			}

			initiate_loop_unit_start(sp,ch);
		}
	}

	
	sleep(5);

	for (sp=0;sp<16;sp++) {

		for (ch=0;ch<31;ch++) {

			if (sp==0 && ch==15) {
				continue;
			}

			isup_exec_command(sp,
					ch,
					-1,
					SIGBOOST_EVENT_REMOVE_CHECK_LOOP,
					0);
		}
	}

#if 0
	for (sp=0;sp<16;sp++) {

		for (ch=0;ch<31;ch++) {

			if (sp==0 && ch==15) {
				continue;
			}

			initiate_call_unit_start(sp,ch);
		}
	}
#endif
	retry++;
	if (retry < 10) {
		goto retry_loop;
	}


#if 0
	initiate_loop_unit_start();
	sleep(5);

	isup_exec_command(0,
                          0,
                          -1,
			  SIGBOOST_EVENT_REMOVE_CHECK_LOOP,
                          0);
#endif
#endif

#if 0
	sleep(1);
	isup_exec_command(0,
                          0,
                          -1,
			  SIGBOOST_EVENT_CALL_STOPPED,
			  //SIGBOOST_EVENT_CALL_START_NACK,
			  //SIGBOOST_EVENT_INSERT_CHECK_LOOP,
                          17);
#endif


#if 0
	isup_exec_command(0,
                          0,
                          -1,
                          //SIGBOOST_EVENT_CALL_STOPPED,
                          //SIGBOOST_EVENT_CALL_START_NACK,
                          SIGBOOST_EVENT_INSERT_REMOVE_LOOP,
                          17);
#endif
}



void handle_call_stop(short_signal_event_t *event)
{
	
	/* At this point we have already sent our STOP so its safe to ACK */
#if 0
	isup_exec_command(event->span, 
	  	  event->chan, 
	  	  -1,
	  	  SIGBOOST_EVENT_CALL_STOPPED_ACK,
	  	  0);
#endif

#if 0
	//sleep(1);

	 isup_exec_command(event->span,
                          event->chan,
                          -1,
                          //SIGBOOST_EVENT_CALL_STOPPED,
                          //SIGBOOST_EVENT_CALL_START_NACK,
                          SIGBOOST_EVENT_INSERT_CHECK_LOOP,
                          0);

	sleep(10);

	isup_exec_command(event->span,
                          event->chan,
                          -1,
                          //SIGBOOST_EVENT_CALL_STOPPED,
                          //SIGBOOST_EVENT_CALL_START_NACK,
                          SIGBOOST_EVENT_REMOVE_CHECK_LOOP,
                          0);
#endif
}

void handle_heartbeat(short_signal_event_t *event)
{
	if (!woomera_test_flag(&server.master_connection, WFLAG_MONITOR_RUNNING)) {
		log_printf(0, server.log,"ERROR! Monitor Thread not running!\n");
	} else {	
		int err=call_signal_connection_write(&server.mcon, (call_signal_event_t*)event);
		if (err <= 0) {
			log_printf(0, server.log,
                                   "Critical System Error: Failed to tx on ISUP socket [%s]: %s\n",
                                         strerror(errno));
		}
	}
	return;
}

void handle_call_stop_ack(short_signal_event_t *event)
{
	/* No need for us to do any thing here */

#if 0	
	initiate_call_unit_start();

	sleep(5);

	event->span=0;
	event->chan=0;
	isup_exec_command(event->span,
                          event->chan,
                          -1,
                          //SIGBOOST_EVENT_CALL_STOPPED,
                          SIGBOOST_EVENT_CALL_START_NACK,
                          //SIGBOOST_EVENT_INSERT_CHECK_LOOP,
                          0);
#endif

#if 0
       isup_exec_command(event->span,
                          event->chan,
                          -1,
                          //SIGBOOST_EVENT_CALL_STOPPED,
                          //SIGBOOST_EVENT_CALL_START_NACK,
                          SIGBOOST_EVENT_INSERT_CHECK_LOOP,
                          0);

        sleep(10);

        isup_exec_command(event->span,
                          event->chan,
                          -1,
                          //SIGBOOST_EVENT_CALL_STOPPED,
                          //SIGBOOST_EVENT_CALL_START_NACK,
                          SIGBOOST_EVENT_REMOVE_CHECK_LOOP,
                          0);
#endif
	return;
}

void handle_call_start_nack_ack(short_signal_event_t *event)
{

	if (event->call_setup_id == 0) {
		log_printf(0,server.log, "FATAL: ERROR NACK ACK Incoming id == 0\n");
		woomera_clear_flag(&server.master_connection, WFLAG_RUNNING);
		exit(0);
	}

	if (server.holding_tank[event->call_setup_id] == NULL) {
		log_printf(0,server.log, "FATAL: ERROR NACK ACK Incoming id %i already empty\n",
			event->call_setup_id);
		woomera_clear_flag(&server.master_connection, WFLAG_RUNNING);
		exit(0);
	}

	server.holding_tank[event->call_setup_id]=NULL;


#if 0
    	initiate_call_unit_start();

        sleep(5);

        event->span=0;
        event->chan=0;
        isup_exec_command(event->span,
                          event->chan,
                          -1,
                          SIGBOOST_EVENT_CALL_STOPPED,
                          //SIGBOOST_EVENT_CALL_START_NACK,
                          //SIGBOOST_EVENT_INSERT_CHECK_LOOP,
                          0);

#endif
	/* No need for us to do any thing here */
	return;
}

