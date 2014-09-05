/*********************************************************************************
 * sangoma_mgd.h --  Sangoma Media Gateway Daemon for Sangoma/Wanpipe Cards
 *
 * Copyright 05-07, Nenad Corbic <ncorbic@sangoma.com>
 *		    Anthony Minessale II <anthmct@yahoo.com>
 *
 * This program is free software, distributed under the terms of
 * the GNU General Public License
 * 
 * =============================================*/

#ifndef __SANGOMA_MGD_H_
#define __SANGOMA_MGD_H_

#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stddef.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/time.h>
#include <poll.h>
#include <signal.h>
#include <pthread.h>
#include <netinet/tcp.h>
#include <assert.h>
#include <sys/mman.h>
#include <syslog.h> 
#include <libsangoma.h>

#include "sangoma_mgd_common.h"
#include "call_signal.h"
#include "sangoma_mgd_memdbg.h"

#ifdef __LINUX__
#include <sys/prctl.h>
#endif


#include "call_signal.h"
#include "g711.h"
#include "sangoma_mgd_memdbg.h"
#include "libteletone.h"
#include "switch_buffer.h"
#include "list.h"

#define USE_LOG_THREAD 1
#define USE_SYSLOG 1
#define CODEC_LAW_DEFAULT 1

#define WOOMERA_MAX_CHAN	31

#define SMG_MAX_TG	32

#define SMG_SESSION_NAME_SZ	100
#define SMG_CHAN_NAME_SZ	20

#define PIDFILE "/var/run/sangoma_mgd.pid"
#define PIDFILE_UNIT "/var/run/sangoma_mgd_unit.pid"

#define WOOMERA_MAX_MEDIA_PORTS 5000

#define CORE_EVENT_LEN 512
#define WOOMERA_STRLEN 256
#define WOOMERA_ARRAY_LEN 50
#define WOOMERA_BODYLEN 2048
#define WOOMERA_MIN_MEDIA_PORT 10000
#define WOOMERA_MAX_MEDIA_PORT (WOOMERA_MIN_MEDIA_PORT + WOOMERA_MAX_MEDIA_PORTS)
#define WOOMERA_HARD_TIMEOUT 0
#define WOOMERA_LINE_SEPERATOR "\r\n"
#define WOOMERA_RECORD_SEPERATOR "\r\n\r\n"
#define WOOMERA_DEBUG_PREFIX "[DEBUG] "
#define WOOMERA_DEBUG_LINE "------------------------------------------------------------------------------------------------"




#define STDERR fileno(stderr)
#define MAXPENDING 500
#define MGD_STACK_SIZE 1024 * 240

#define SMG_SOCKET_EVENT_TIMEOUT 0

typedef enum {
    WFLAG_RUNNING 		= (1 << 0),
    WFLAG_LISTENING 		= (1 << 1),
    WFLAG_MASTER_DEV 		= (1 << 2),
    WFLAG_EVENT 		= (1 << 3),
    WFLAG_MALLOC 		= (1 << 4),
    WFLAG_MEDIA_RUNNING 	= (1 << 5),
    WFLAG_MEDIA_END 		= (1 << 6),
    WFLAG_MONITOR_RUNNING 	= (1 << 7),
    WFLAG_HANGUP 		= (1 << 8),
    WFLAG_ANSWER 		= (1 << 9),
    WFLAG_MEDIA_TDM_RUNNING 	= (1 << 10),
    WFLAG_HANGUP_ACK 		= (1 << 11),
    WFLAG_HANGUP_NACK_ACK 	= (1 << 12),
    WFLAG_WAIT_FOR_NACK_ACK 	= (1 << 13),		/* Wait flag used to test reception of an NACK  */
    WFLAG_WAIT_FOR_STOPPED_ACK 	= (1 << 14),		/* Wait flag used to test reception of an ACK  */
    WFLAG_RAW_MEDIA_STARTED 	= (1 << 15),		/* Media has started on this channel */
    WFLAG_CALL_ACKED     	= (1 << 16),		/* Woomera side rx accept so CALL ACK was Sent */
    WFLAG_WAIT_FOR_NACK_ACK_SENT = (1 << 17),		/* Call START NACK was sent out on this channel */
    WFLAG_WAIT_FOR_STOPPED_ACK_SENT = (1 << 18),	/* Call STOP was sent out on this channel */
    WFLAG_SYSTEM_RESET 		= (1 << 19),		/* Initial System Reset Condition no calls allowed */
    WFLAG_SYSTEM_NEED_RESET_ACK = (1 << 20),		/* We sent a RESTART */
    WFLAG_WAIT_FOR_ACK_TIMEOUT 	= (1 << 21),		/* Timeout flag indicating that incoming ACK or NACK timedout */
    WFLAG_SYSTEM_NEED_IN_RESET_ACK = (1 << 22),		/* We got a RESTART */
} WFLAGS;

enum {

	SMG_LOG_ALL  		= 0,
	SMG_LOG_PROD  		= 1,
	SMG_LOG_BOOST 		= 2,
	SMG_LOG_WOOMERA 	= 3,
	SMG_LOG_DEBUG_CALL  = 4,
	SMG_LOG_DEBUG_MISC	= 5,
	SMG_LOG_DEBUG_6		= 6,
	SMG_LOG_DEBUG_7		= 7,
	SMG_LOG_DEBUG_8		= 8,
	SMG_LOG_DEBUG_9		= 9,
	SMG_LOG_DEBUG_10  	= 10
};


#define woomera_print_flags(woomera,level) \
    log_printf(level,woomera->log,"%s: WFLAG_RUNNING                    = %i\n",woomera->interface, woomera_test_flag(woomera,WFLAG_RUNNING));\
	log_printf(level,woomera->log,"%s: WFLAG_LISTENING                  = %i\n",woomera->interface, woomera_test_flag(woomera,WFLAG_LISTENING));\
	log_printf(level,woomera->log,"%s: WFLAG_MASTER_DEV                 = %i\n",woomera->interface, woomera_test_flag(woomera,WFLAG_MASTER_DEV));\
	log_printf(level,woomera->log,"%s: WFLAG_EVENT                      = %i\n",woomera->interface, woomera_test_flag(woomera,WFLAG_EVENT));\
	log_printf(level,woomera->log,"%s: WFLAG_MALLOC                     = %i\n",woomera->interface, woomera_test_flag(woomera,WFLAG_MALLOC));\
	log_printf(level,woomera->log,"%s: WFLAG_MEDIA_RUNNING              = %i\n",woomera->interface, woomera_test_flag(woomera,WFLAG_MEDIA_RUNNING));\
	log_printf(level,woomera->log,"%s: WFLAG_MEDIA_END                  = %i\n",woomera->interface, woomera_test_flag(woomera,WFLAG_MEDIA_END));\
	log_printf(level,woomera->log,"%s: WFLAG_MONITOR_RUNNING            = %i\n",woomera->interface, woomera_test_flag(woomera,WFLAG_MONITOR_RUNNING));\
	log_printf(level,woomera->log,"%s: WFLAG_HANGUP                     = %i\n",woomera->interface, woomera_test_flag(woomera,WFLAG_HANGUP));\
	log_printf(level,woomera->log,"%s: WFLAG_ANSWER                     = %i\n",woomera->interface, woomera_test_flag(woomera,WFLAG_ANSWER));\
	log_printf(level,woomera->log,"%s: WFLAG_MEDIA_TDM_RUNNING          = %i\n",woomera->interface, woomera_test_flag(woomera,WFLAG_MEDIA_TDM_RUNNING));\
	log_printf(level,woomera->log,"%s: WFLAG_HANGUP_ACK                 = %i\n",woomera->interface, woomera_test_flag(woomera,WFLAG_HANGUP_ACK));\
	log_printf(level,woomera->log,"%s: WFLAG_HANGUP_NACK_ACK            = %i\n",woomera->interface, woomera_test_flag(woomera,WFLAG_HANGUP_NACK_ACK));\
	log_printf(level,woomera->log,"%s: WFLAG_WAIT_FOR_NACK_ACK          = %i\n",woomera->interface, woomera_test_flag(woomera,WFLAG_WAIT_FOR_NACK_ACK));\
	log_printf(level,woomera->log,"%s: WFLAG_WAIT_FOR_STOPPED_ACK       = %i\n",woomera->interface, woomera_test_flag(woomera,WFLAG_WAIT_FOR_STOPPED_ACK));\
	log_printf(level,woomera->log,"%s: WFLAG_RAW_MEDIA_STARTED          = %i\n",woomera->interface, woomera_test_flag(woomera,WFLAG_RAW_MEDIA_STARTED));\
	log_printf(level,woomera->log,"%s: WFLAG_CALL_ACKED                 = %i\n",woomera->interface, woomera_test_flag(woomera,WFLAG_CALL_ACKED));\
	log_printf(level,woomera->log,"%s: WFLAG_WAIT_FOR_NACK_ACK_SENT     = %i\n",woomera->interface, woomera_test_flag(woomera,WFLAG_WAIT_FOR_NACK_ACK_SENT));\
	log_printf(level,woomera->log,"%s: WFLAG_WAIT_FOR_STOPPED_ACK_SENT  = %i\n",woomera->interface, woomera_test_flag(woomera,WFLAG_WAIT_FOR_STOPPED_ACK_SENT));\
	log_printf(level,woomera->log,"%s: WFLAG_SYSTEM_RESET               = %i\n",woomera->interface, woomera_test_flag(woomera,WFLAG_SYSTEM_RESET));\
	log_printf(level,woomera->log,"%s: WFLAG_WAIT_FOR_ACK_TIMEOUT       = %i\n",woomera->interface, woomera_test_flag(woomera,WFLAG_WAIT_FOR_ACK_TIMEOUT));\


typedef enum {
    MFLAG_EXISTS = (1 << 0),
    MFLAG_CONTENT = (1 << 1),
} MFLAGS;

typedef enum {
    EVENT_FREE_DATA = 1,
    EVENT_KEEP_DATA = 0
} event_args;


void __log_printf(int level, FILE *fp, char *file, const char *func, int line, char *fmt, ...);

#define ysleep(usec) sched_yield() ; usleep(usec);
#define log_printf(level, fp, fmt, ...) __log_printf(level, fp, __FILE__, __FUNCTION__, __LINE__, fmt, ##__VA_ARGS__)


#define woomera_test_flag(p,flag)		({ \
					((p)->flags & (flag)); \
					})

#define _woomera_set_flag(p,flag)		do { \
					((p)->flags |= (flag)); \
					} while (0)

#define _woomera_clear_flag(p,flag)		do { \
					((p)->flags &= ~(flag)); \
					} while (0)					
					
#define woomera_set_flag(p,flag)		do { \
    					pthread_mutex_lock(&(p)->flags_lock); \
					((p)->flags |= (flag)); \
    					pthread_mutex_unlock(&(p)->flags_lock); \
					} while (0)

#define woomera_clear_flag(p,flag)		do { \
    					pthread_mutex_lock(&(p)->flags_lock); \
					((p)->flags &= ~(flag)); \
    					pthread_mutex_unlock(&(p)->flags_lock); \
					} while (0)

#define woomera_copy_flags(dest,src,flagz)	do { \
    					pthread_mutex_lock(&(p)->flags_lock); \
					(dest)->flags &= ~(flagz); \
					(dest)->flags |= ((src)->flags & (flagz)); \
    					pthread_mutex_unlock(&(p)->flags_lock); \
					} while (0)


struct media_session {
    int udp_sock;
    int sangoma_sock;
    char *ip;
    int port;
	char *raw;
    time_t started;
    time_t answered;
    pthread_t thread;
    int socket;
    struct woomera_interface *woomera;
    struct sockaddr_in local_addr;
    struct sockaddr_in remote_addr;
    struct hostent remote_hp;
    struct hostent local_hp;
    int skip_read_frames;
    int skip_write_frames;
    int hw_coding;
    int hw_dtmf;
    int has_hwec;

    int udp_sync_cnt;
    
#ifdef WP_HPTDM_API
    hp_tdm_api_chan_t *tdmchan;
#endif
    
    teletone_dtmf_detect_state_t dtmf_detect;
    teletone_generation_session_t tone_session;
    switch_buffer_t *dtmf_buffer;
	unsigned char oob_disable;
};

struct woomera_message {
    char callid[WOOMERA_STRLEN];
    char command[WOOMERA_STRLEN];
    char command_args[WOOMERA_STRLEN];
    char names[WOOMERA_STRLEN][WOOMERA_ARRAY_LEN];
    char values[WOOMERA_STRLEN][WOOMERA_ARRAY_LEN];
    char body[WOOMERA_BODYLEN];
    uint32_t flags;
    pthread_mutex_t flags_lock;
    int last;
    struct woomera_message *next;
};

struct woomera_event {
    char *data;
    uint32_t flags;
    struct woomera_event *next;
};

struct woomera_listener {
    struct woomera_interface *woomera;
    struct woomera_listener *next;
};

struct woomera_interface {
	int socket;
	char *raw;
	char *interface;
	time_t timeout;
	struct sockaddr_in addr;
	struct media_session *ms;
	pthread_mutex_t queue_lock;
	pthread_mutex_t ms_lock;
	pthread_mutex_t dtmf_lock;
	struct woomera_event *event_queue;
	struct woomera_event *incoming_event_queue;
	pthread_t thread;
	uint32_t flags;
	pthread_mutex_t flags_lock;
	int debug;
	int call_id;
	FILE *log;
	pthread_mutex_t vlock;
	int index;
	int index_hold;
	int span;
	int chan;
	int trunk_group;
	int call_count;
	int q931_rel_cause_tosig;
	int q931_rel_cause_topbx;
	int loop_tdm;
	char session[SMG_SESSION_NAME_SZ];
	int check_digits;	/* set to 1 when session comes up */
	int bearer_cap;
	unsigned int rx_udp_seq;
	unsigned int tx_udp_seq;
	struct woomera_interface *next;
};

#define WOOMERA_MAX_RBS_BITS 4

typedef struct woomera_rbs_bits
{
	int init;
	unsigned char abcd;
}woomera_rbs_bits_t;

typedef struct woomera_rbs_relay
{
	int init;
	woomera_rbs_bits_t rbs_bits[WOOMERA_MAX_RBS_BITS];
	int rx_idx;
	int tx_idx;
} woomera_rbs_relay_t;

struct  woomera_session {
	struct woomera_interface *dev;
	char session[SMG_SESSION_NAME_SZ];
	char digits[MAX_DIALED_DIGITS+1];
	int  digits_len;
	int bearer_cap;
	int clients;
	unsigned char media_used;
	pthread_mutex_t media_lock;
	woomera_rbs_relay_t rbs_relay;
	int sangoma_fd;
	int sangoma_fd_usage;
};

struct smg_tdm_ip_bridge {
	int init;
	int end;
	int span;
	int chan;
#if 0
	int port;
	char local_ip[25];
	char remote_ip[25];
#endif
	int period;
	int tdm_fd;
	call_signal_connection_t mcon;
	pthread_t thread;
};

extern struct smg_tdm_ip_bridge g_smg_ip_bridge_idx[];
extern pthread_mutex_t g_smg_ip_bridge_lock;




#define MAX_SMG_RBS_RELAY 32
#define MAX_SMG_BRIDGE 32
#define CORE_TANK_LEN CORE_MAX_CHAN_PER_SPAN*CORE_MAX_SPANS

struct woomera_server {
	struct  woomera_session process_table[CORE_MAX_SPANS][CORE_MAX_CHAN_PER_SPAN+1];
	struct woomera_interface *holding_tank[CORE_TANK_LEN];
	int holding_tank_index;
	struct woomera_interface master_connection;
	pthread_mutex_t listen_lock;
	pthread_mutex_t ht_lock;
	pthread_mutex_t process_lock;
	pthread_mutex_t digits_lock;
	pthread_mutex_t media_udp_port_lock;
	pthread_mutex_t thread_count_lock;
	call_signal_connection_t mcon;
	call_signal_connection_t mconp;
	struct woomera_listener *listeners;
	char media_ip[WOOMERA_STRLEN];
	int port;
	int next_media_port;
	int base_media_port;
	int max_media_port;
	int debug;
	int panic;
	int thread_count;
	char *logfile_path;
	FILE *log;
	char boost_local_ip[25];
	int boost_local_port;
	char boost_remote_ip[25];
	int boost_remote_port;
	pthread_t monitor_thread;
	char *config_file;
	int max_calls;
	int call_count;
	uint32_t out_tx_test;
	uint32_t rxgain;
	uint32_t txgain;
	uint32_t hw_coding;
	uint32_t loop_trace;
	uint32_t hungup_waiting;
	int all_ckt_gap[SMG_MAX_TG+1];
	int all_ckt_busy[SMG_MAX_TG+1];
	struct timeval all_ckt_busy_time[SMG_MAX_TG+1];
	struct timeval restart_timeout;
	int dtmf_on; 
	int dtmf_off;
	int dtmf_intr_ch;
	int dtmf_size;
	int strip_cid_non_digits;
	int call_timeout;
	struct smg_tdm_ip_bridge ip_bridge_idx[MAX_SMG_BRIDGE];
	int udp_seq; 
	unsigned int media_rx_seq_err;
	unsigned char rbs_relay[MAX_SMG_RBS_RELAY];
	unsigned char media_pass_through;
};

extern struct woomera_server server;

struct woomera_config {
    FILE *file;
    char *path;
    char category[256];
    char buf[1024];
    int lineno;
};

static inline int smg_get_ip_bridge_session(struct smg_tdm_ip_bridge **ip_bridge)
{
	int i;
	for (i=0;i<MAX_SMG_BRIDGE;i++) {
		if (g_smg_ip_bridge_idx[i].init) {
			continue;
		}
		g_smg_ip_bridge_idx[i].init=1;
		*ip_bridge=&g_smg_ip_bridge_idx[i];
		return 0;
	}

	*ip_bridge=NULL;	
	return -1;

}

static inline int smg_free_ip_bridge_session(struct smg_tdm_ip_bridge *ip_bridge)
{
	memset(ip_bridge,0,sizeof(struct smg_tdm_ip_bridge));
	return 0;
}

static inline void smg_get_current_priority(int *policy, int *priority)
{
    struct sched_param param;
    pthread_getschedparam(pthread_self(), policy, &param);
    *priority = param.sched_priority;
    return;
}

static inline int smg_calc_elapsed(struct timeval *started, struct timeval *ended)
{
	return (((ended->tv_sec * 1000) + ended->tv_usec / 1000) - 
		((started->tv_sec * 1000) + started->tv_usec / 1000));
}

static inline int smg_check_all_busy(int tg)
{
	struct timeval ended;
	int elapsed;

	if (server.all_ckt_gap[tg]) {
		return server.all_ckt_gap[tg];
	}
 
	if (server.all_ckt_busy[tg]==0) {
		return 0;
	}
 
	gettimeofday(&ended,NULL);
	elapsed = smg_calc_elapsed(&server.all_ckt_busy_time[tg],&ended);
	
	/* seconds elapsed */
	if (elapsed > server.all_ckt_busy[tg]) {
		server.all_ckt_busy[tg]=0;
		return 0;
	} else {
		return 1;
	}

#if 0

	if (server.all_ckt_busy[tg] > 50) {
		/* When in GAP mode wait 10s */
		return server.all_ckt_busy[tg];
	}
	
	--server.all_ckt_busy[tg];
	if (server.all_ckt_busy[tg] < 0) {
		server.all_ckt_busy[tg]=0;
	}
	
	return server.all_ckt_busy[tg];
#endif
}

 
static inline void smg_all_ckt_busy(int tg)
{

	if (server.call_count*10 < 1500) {
		server.all_ckt_busy[tg]+=1500;
	} else {
		server.all_ckt_busy[tg]+=server.call_count*15;
	}

	if (server.all_ckt_busy[tg] > 10000) {
		server.all_ckt_busy[tg] = 10000;
	}

#if 0	
	if (server.all_ckt_busy[tg] >= 5) {
		server.all_ckt_busy[tg]=10;
	} else if (server.all_ckt_busy[tg] >= 10) {
		server.all_ckt_busy[tg]=15;
	} else if (server.all_ckt_busy[tg] == 0) {
		server.all_ckt_busy[tg]=5;
	}
#endif
	gettimeofday(&server.all_ckt_busy_time[tg],NULL);
}

static inline void smg_all_ckt_gap(int tg)
{
	server.all_ckt_gap[tg]=1;		
}

static inline void smg_clear_ckt_gap(int tg)
{
	server.all_ckt_gap[tg]=0;		
	gettimeofday(&server.all_ckt_busy_time[tg],NULL);
}


static inline int smg_validate_span_chan(int span, int chan)
{
	if (span < 0 || span > max_spans) {
		return -1;
	}

	if (chan < 0 || chan > WOOMERA_MAX_CHAN) {
		return -1;
	}
	
	return 0;
}


static inline void close_socket(int *sp) 
{
    if (*sp > -1) {
		close(*sp);
		*sp = -1;
    }
}

static inline FILE *safe_fopen(char *path, char *flags)
{
    char buf[512] = "";

    if (readlink(path, buf, sizeof(buf)) > 0) {
		fprintf(stderr, "Symlinks not allowed! [%s] != [%s]\n", buf, path);
		return NULL;
    }
	
    return fopen(path, flags);
	
}

static inline int get_pid_from_file(char *path)
{
    FILE *tmp;
    int pid;
    int err;

    if (!(tmp = safe_fopen(path, "r"))) {
		return 0;
    } else {
		err=fscanf(tmp, "%d", &pid);
		fclose(tmp);
		tmp = NULL;
    }

    return pid;
}


static inline int woomera_open_file(struct woomera_config *cfg, char *path) 
{
    FILE *f;

    if (!(f = fopen(path, "r"))) {
		log_printf(SMG_LOG_ALL, stderr, "Cannot open file %s\n", path);
		return 0;
    }

    memset(cfg, 0, sizeof(*cfg));
    cfg->file = f;
    cfg->path = path;
    return 1;

}


static inline void woomera_close_file(struct woomera_config *cfg)
{
	
    if (cfg->file) {
		fclose(cfg->file);
    }

    memset(cfg, 0, sizeof(*cfg));
}


static inline void woomera_message_init (struct woomera_message *wmsg)
{
	memset (wmsg,0,sizeof(struct woomera_message));
	pthread_mutex_init(&wmsg->flags_lock, NULL);	
}

static inline void woomera_message_clear (struct woomera_message *wmsg)
{
	pthread_mutex_destroy(&wmsg->flags_lock);	
}


static inline void woomera_set_raw(struct woomera_interface *woomera, char *raw)
{
	char *oldraw=woomera->raw;

	if (raw) {
		woomera->raw = smg_strdup(raw);
    	} else {
		woomera->raw = NULL;
	}

   	if (oldraw) {
		smg_free(oldraw);
    	}
}

static inline void media_set_raw(struct media_session *ms, char *raw)
{
	char *oldraw=ms->raw;

	if (raw) {
		ms->raw = smg_strdup(raw);
	} else {
		ms->raw = NULL;
	}

	if (oldraw) {
		smg_free(oldraw);
	}
}

static inline struct media_session * woomera_get_ms(struct woomera_interface *woomera)
{
	struct media_session *ms;
	pthread_mutex_lock(&woomera->ms_lock);
	ms=woomera->ms;
	pthread_mutex_unlock(&woomera->ms_lock);
	return ms;
}

static inline void woomera_set_ms(struct woomera_interface *woomera,struct media_session *ms)
{
	pthread_mutex_lock(&woomera->ms_lock);
	woomera->ms=ms;
	pthread_mutex_unlock(&woomera->ms_lock);
	return;
}

static inline void woomera_set_interface(struct woomera_interface *woomera, char *interface)
{
	char *iface = woomera->interface;
	
	if (interface) {
		woomera->interface = smg_strdup(interface);
    	} else {
		woomera->interface = NULL;
	}
	
    	if (iface) {
		smg_free(iface);
    	}
}

static inline void woomera_set_cause_tosig(struct woomera_interface *woomera, int cause)
{
	if (!cause) {
		cause=SIGBOOST_RELEASE_CAUSE_NORMAL;
	}

	woomera->q931_rel_cause_tosig=cause;
}

static inline void woomera_set_cause_topbx(struct woomera_interface *woomera, int cause)
{

	if (!cause) {
                cause=SIGBOOST_RELEASE_CAUSE_NORMAL;
        }

	if (cause == SIGBOOST_CALL_SETUP_NACK_ALL_CKTS_BUSY) {
		cause=34;
	}	

        woomera->q931_rel_cause_topbx=cause;

}


static inline struct woomera_event *new_woomera_event(void)
{
    struct woomera_event *event = NULL;

    if ((event = smg_malloc(sizeof(*event)))) {
		memset(event, 0, sizeof(*event));
		_woomera_set_flag(event, WFLAG_MALLOC);
    }

    return event;
}


static inline void destroy_woomera_event_data(struct woomera_event *event)
{
    if (event->data) {
	smg_free (event->data);
	event->data=NULL;
    }
}

static inline void destroy_woomera_event(struct woomera_event **event, event_args free_data)
{
    if (free_data) {
		smg_free ((*event)->data);
    }

    (*event)->data=NULL;
    if (woomera_test_flag((*event), WFLAG_MALLOC)) {	
		smg_free (*event);
		*event = NULL;
    }
}


/* disable nagle's algorythm */
static inline void no_nagle(int socket)
{
    int flag = 1;
    setsockopt(socket, IPPROTO_TCP, TCP_NODELAY, (char *) &flag, sizeof(int));
}


static inline void remove_end_of_digits_char(unsigned char *s)
{
        unsigned char *p;
        for (p = s; *p; p++) {
                if (*p == 'F' || *p > 'f') {
                        log_printf(SMG_LOG_DEBUG_MISC, server.log, "Removing a non-numeric character [%c]!\n", *p);
                        *p = '\0';
                        break;
                }
        }
}

static inline void validate_number(unsigned char *s)
{
        unsigned char *p;
        for (p = s; *p; p++) {
                if (*p < 48 || *p > 57) {
                        log_printf(SMG_LOG_DEBUG_CALL, server.log, "Encountered a non-numeric character [%c]!\n", *p);
                        *p = '\0';
                        break;
                }
        }
}

static inline int woomera_check_running(struct woomera_interface *woomera) 
{
	if (woomera_test_flag(woomera, WFLAG_HANGUP) || 
		!woomera_test_flag(woomera, WFLAG_RUNNING) ||
		woomera_test_flag(woomera, WFLAG_MEDIA_END)) {
		
		return 0;
	}
		
	return 1;
}

static inline int open_span_chan (unsigned char span, unsigned char chan)
{
	int fd = -1;
#ifndef LIBSANGOMA_VERSION
	fd = sangoma_open_tdmapi_span_chan(span, chan);
#else
	if (chan == 24) {
		pthread_mutex_lock(&server.process_table[span][chan].media_lock);
		if(server.process_table[span][chan].media_used > 0) {
			log_printf(SMG_LOG_ALL, server.log, 
				"Critical Error: channel already opened [s%ic%i]\n", span, chan);
		} else {
			server.process_table[span][chan].media_used++;
	
			fd = __sangoma_open_api_span_chan(span, chan);
		}
		pthread_mutex_unlock(&server.process_table[span][chan].media_lock);
	} else {
		fd = sangoma_open_api_span_chan(span, chan);
	}
#endif
	return fd;
}

static inline void close_span_chan (int *socket, unsigned char span, unsigned char chan)
{
	if (chan == 24) {
		pthread_mutex_lock(&server.process_table[span][chan].media_lock);
		if(server.process_table[span][chan].media_used > 0) {
			server.process_table[span][chan].media_used--;
		}
		close_socket(socket);
		pthread_mutex_unlock(&server.process_table[span][chan].media_lock);
	} else {
		close_socket(socket);
	}
}

static inline void set_digits_info(unsigned char *target, char* string_val)
{
	if (string_val && (atoi(string_val) >= 0)) {
		*target = atoi(string_val);
		return;
	}
	*target = 255;
}


extern int smg_log_init(void);
extern void smg_log_cleanup(void);
extern int smg_ip_bridge_start(void);
extern int smg_ip_bridge_stop(void);
extern int waitfor_2sockets(int fda, int fdb, char *a, char *b, int timeout);
#endif

