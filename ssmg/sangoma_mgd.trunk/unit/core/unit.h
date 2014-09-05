
#ifndef __TEST_H__
#define __TEST_H__

#include "sangoma_mgd.h"


extern void handle_call_answer(short_signal_event_t *event);
extern void handle_call_start_ack(short_signal_event_t *event);
extern void handle_call_start_nack(short_signal_event_t *event);
extern void handle_remove_loop(short_signal_event_t *event);
extern void handle_call_loop_start(call_signal_event_t *event);
extern void handle_call_start(call_signal_event_t *oevent);
extern void handle_gap_abate(short_signal_event_t *event);
extern void handle_restart(call_signal_connection_t *mcon, short_signal_event_t *event);
extern void handle_restart_ack(call_signal_connection_t *mcon, short_signal_event_t *event);
extern void handle_call_stop(short_signal_event_t *event);
extern void handle_heartbeat(short_signal_event_t *event);
extern void handle_call_stop_ack(short_signal_event_t *event);
extern void handle_call_start_nack_ack(short_signal_event_t *event);

extern int isup_exec_command(int span, int chan, int id, int cmd, int cause);
extern int isup_exec_commandp(int span, int chan, int id, int cmd, int cause);
extern int isup_exec_event(call_signal_event_t *oevent);
extern int isup_exec_eventp(call_signal_event_t *oevent);
extern int initiate_loop_unit_start (int span, int chan);
extern int initiate_call_unit_start (int span, int chan);

#endif

