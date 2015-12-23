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
#include "q931_cause.h"
#include "unit.h"

#define USE_SYSLOG 1
#define CODEC_LAW_DEFAULT 1


static char ps_progname[]="sangoma_mgd_unit";

static struct woomera_interface woomera_dead_dev;

#if 0
#define DOTRACE
#endif

struct woomera_server server; 

#if defined(BRI_PROT)
int max_spans=WOOMERA_BRI_MAX_SPAN;
int max_chans=WOOMERA_BRI_MAX_CHAN;
#else
int max_spans=WOOMERA_MAX_SPAN;
int max_chans=WOOMERA_MAX_CHAN;
#endif


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

const char WELCOME_TEXT[] =
"================================================================================\n"
"Sangoma Media Gateway Daemon v1.20 \n"
"TDM Signal Media Gateway for Sangoma/Wanpipe Cards\n"
"Copyright 2005, 2006, 2007 \n"
"Nenad Corbic <ncorbic@sangoma.com>, Anthony Minessale II <anthmct@yahoo.com>\n"
"This program is free software, distributed under the terms of\n"
"the GNU General Public License\n"
"================================================================================\n"
"";


static int coredump=1;
static int autoacm=1;

static int gcnt=0;

static struct woomera_interface *alloc_woomera(void);

q931_cause_to_str_array_t q931_cause_to_str_array[255];



void __log_printf(int level, FILE *fp, char *file, const char *func, int line, char *fmt, ...)
{
    char *data;
    int ret = 0;
    va_list ap;

    if (socket < 0) {
		return;
    }

    if (level && level > server.debug) {
		return;
    }

    va_start(ap, fmt);
#ifdef SOLARIS
    data = (char *) malloc(2048);
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
		fprintf(fp, "[%d] %s %s:%d %s() %s", getpid(), date, file, line, func, data);
#endif
		free(data);
    }
#ifndef USE_SYSLOG
    fflush(fp);
#endif
}



int isup_exec_command(int span, int chan, int id, int cmd, int cause)
{
	short_signal_event_t oevent;
	int retry=5;
	
	call_signal_event_init(&oevent, cmd, chan, span);
	oevent.release_cause = cause;
	
	if (id >= 0) {
		oevent.call_setup_id = id;
	}
isup_exec_cmd_retry:
	if (call_signal_connection_write(&server.mcon, (call_signal_event_t*)&oevent) < 0){

		--retry;
		if (retry <= 0) {
			log_printf(0, server.log, 
			"Critical System Error: Failed to tx on ISUP socket: %s\n", 
				strerror(errno));
			return -1;
		} else {
			log_printf(0, server.log,
                        "System Warning: Failed to tx on ISUP socket: %s :retry %i\n",
                                strerror(errno),retry);
		}

		goto isup_exec_cmd_retry; 
	}
	
	return 0;
}


int isup_exec_commandp(int span, int chan, int id, int cmd, int cause)
{
        short_signal_event_t oevent;
        int retry=5;

        call_signal_event_init(&oevent, cmd, chan, span);
        oevent.release_cause = cause;

        if (id >= 0) {
                oevent.call_setup_id = id;
        }
isup_exec_cmd_retry:
        if (call_signal_connection_writep(&server.mconp, (call_signal_event_t*)&oevent) < 0){

                --retry;
                if (retry <= 0) {
                        log_printf(0, server.log,
                        "Critical System Error: Failed to tx on ISUP socket: %s\n",
                                strerror(errno));
                        return -1;
                } else {
                        log_printf(0, server.log,
                        "System Warning: Failed to tx on ISUP socket: %s :retry %i\n",
                                strerror(errno),retry);
                }

                goto isup_exec_cmd_retry;
        }

        return 0;
}



int isup_exec_event(call_signal_event_t *oevent)
{
	int retry=5;
	

isup_exec_cmd_retry:
	if (call_signal_connection_write(&server.mcon, oevent) < 0){

		--retry;
		if (retry < 0) {
			log_printf(0, server.log, 
			"Critical System Error: Failed to tx on ISUP socket: %s\n", 
				strerror(errno));
			return -1;
		} else {
			log_printf(0, server.log,
                        "System Warning: Failed to tx on ISUP socket: %s :retry %i\n",
                                strerror(errno),retry);
		}

		goto isup_exec_cmd_retry; 
	}
	
	return 0;
}



int isup_exec_eventp(call_signal_event_t *oevent)
{
	int retry=5;
	

isup_exec_cmd_retry:
	if (call_signal_connection_writep(&server.mconp, oevent) < 0){

		--retry;
		if (retry < 0) {
			log_printf(0, server.log, 
			"Critical System Error: Failed to tx on ISUP socket: %s\n", 
				strerror(errno));
			return -1;
		} else {
			log_printf(0, server.log,
                        "System Warning: Failed to tx on ISUP socket: %s :retry %i\n",
                                strerror(errno),retry);
		}

		goto isup_exec_cmd_retry; 
	}
	
	return 0;
}


static int woomera_next_pair(struct woomera_config *cfg, char **var, char **val)
{
    int ret = 0;
    char *p, *end;

    *var = *val = NULL;
	
    for(;;) {
		cfg->lineno++;

		if (!fgets(cfg->buf, sizeof(cfg->buf), cfg->file)) {
			ret = 0;
			break;
		}
		
		*var = cfg->buf;

		if (**var == '[' && (end = strchr(*var, ']'))) {
			*end = '\0';
			(*var)++;
			strncpy(cfg->category, *var, sizeof(cfg->category) - 1);
			continue;
		}
		
		if (**var == '#' || **var == '\n' || **var == '\r') {
			continue;
		}

		if ((end = strchr(*var, '#'))) {
			*end = '\0';
			end--;
		} else if ((end = strchr(*var, '\n'))) {
			if (*end - 1 == '\r') {
				end--;
			}
			*end = '\0';
		}
	
		p = *var;
		while ((*p == ' ' || *p == '\t') && p != end) {
			*p = '\0';
			p++;
		}
		*var = p;

		if (!(*val = strchr(*var, '='))) {
			ret = -1;
			log_printf(0, server.log, "Invalid syntax on %s: line %d\n", cfg->path, cfg->lineno);
			continue;
		} else {
			p = *val - 1;
			*(*val) = '\0';
			(*val)++;
			if (*(*val) == '>') {
				*(*val) = '\0';
				(*val)++;
			}

			while ((*p == ' ' || *p == '\t') && p != *var) {
				*p = '\0';
				p--;
			}

			p = *val;
			while ((*p == ' ' || *p == '\t') && p != end) {
				*p = '\0';
				p++;
			}
			*val = p;
			ret = 1;
			break;
		}
    }

    return ret;

}


#if 0
static void woomera_set_span_chan(struct woomera_interface *woomera, int span, int chan)
{
    pthread_mutex_lock(&woomera->vlock);
    woomera->span = span;
    woomera->chan = chan;
    pthread_mutex_unlock(&woomera->vlock);

}
#endif


static struct woomera_event *new_woomera_event_printf(struct woomera_event *ebuf, char *fmt, ...)
{
    struct woomera_event *event = NULL;
    int ret = 0;
    va_list ap;

    if (ebuf) {
		event = ebuf;
    } else if (!(event = new_woomera_event())) {
		log_printf(0, server.log, "Memory Error queuing event!\n");
		return NULL;
    } else {
    		return NULL;
    }

    va_start(ap, fmt);
#ifdef SOLARIS
    event->data = (char *) malloc(2048);
    vsnprintf(event->data, 2048, fmt, ap);
#else
    ret = vasprintf(&event->data, fmt, ap);
#endif
    va_end(ap);
    if (ret == -1) {
		log_printf(0, server.log, "Memory Error queuing event!\n");
		destroy_woomera_event(&event, EVENT_FREE_DATA);
		return NULL;
    }

    return event;
	
}

static struct woomera_event *woomera_clone_event(struct woomera_event *event)
{
    struct woomera_event *clone;

    if (!(clone = new_woomera_event())) {
		return NULL;
    }

    memcpy(clone, event, sizeof(*event));
    clone->next = NULL;
    clone->data = strdup(event->data);

    return clone;
}

static void enqueue_event(struct woomera_interface *woomera, 
                          struct woomera_event *event, 
			  event_args free_data)
{
    struct woomera_event *ptr, *clone = NULL;
	
    assert(woomera != NULL);
    assert(event != NULL);

    if (!(clone = woomera_clone_event(event))) {
		log_printf(0, server.log, "Error Cloning Event\n");
		return;
    }

    pthread_mutex_lock(&woomera->queue_lock);
	
    for (ptr = woomera->event_queue; ptr && ptr->next ; ptr = ptr->next);
	
    if (ptr) {
	ptr->next = clone;
    } else {
	woomera->event_queue = clone;
    }

    pthread_mutex_unlock(&woomera->queue_lock);

    woomera_set_flag(woomera, WFLAG_EVENT);
    
    if (free_data && event->data) {
    	/* The event has been duplicated, the original data
	 * should be freed */
    	free(event->data);	
	event->data=NULL;
    }
}

static char *dequeue_event(struct woomera_interface *woomera) 
{
    struct woomera_event *event;
    char *data = NULL;

    if (!woomera) {
		return NULL;
    }
	
    pthread_mutex_lock(&woomera->queue_lock);
    if (woomera->event_queue) {
		event = woomera->event_queue;
		woomera->event_queue = event->next;
		data = event->data;
		pthread_mutex_unlock(&woomera->queue_lock);
	
		destroy_woomera_event(&event, EVENT_KEEP_DATA);
		return data;
    }
    pthread_mutex_unlock(&woomera->queue_lock);

    return data;
}


static int enqueue_event_on_listeners(struct woomera_event *event) 
{
    struct woomera_listener *ptr;
    int x = 0;

    assert(event != NULL);

    pthread_mutex_lock(&server.listen_lock);
    for (ptr = server.listeners ; ptr ; ptr = ptr->next) {
		enqueue_event(ptr->woomera, event, EVENT_KEEP_DATA);
		x++;
    }
    pthread_mutex_unlock(&server.listen_lock);
	
    return x;
}


static void del_listener(struct woomera_interface *woomera) 
{
    struct woomera_listener *ptr, *last = NULL;

    pthread_mutex_lock(&server.listen_lock);
    for (ptr = server.listeners ; ptr ; ptr = ptr->next) {
		if (ptr->woomera == woomera) {
			if (last) {
				last->next = ptr->next;
			} else {
				server.listeners = ptr->next;
			}
			free(ptr);
			break;
		}
		last = ptr;
    }
    pthread_mutex_unlock(&server.listen_lock);
}

static void add_listener(struct woomera_interface *woomera) 
{
    struct woomera_listener *new;

    pthread_mutex_lock(&server.listen_lock);
	
    if ((new = malloc(sizeof(*new)))) {
		memset(new, 0, sizeof(*new));
		new->woomera = woomera;
		new->next = server.listeners;
		server.listeners = new;
    } else {
		log_printf(0, server.log, "Memory Error adding listener!\n");
    }

    pthread_mutex_unlock(&server.listen_lock);
}

static struct woomera_interface *alloc_woomera(void)
{
	struct woomera_interface *woomera = NULL;

	if ((woomera = malloc(sizeof(struct woomera_interface)))) {

		memset(woomera, 0, sizeof(struct woomera_interface));
		
		woomera->chan = -1;
		woomera->span = -1;
		woomera->log = server.log;
		woomera->debug = server.debug;
		woomera->call_id = 1;
		woomera->event_queue = NULL;
		woomera->q931_rel_cause_topbx=SIGBOOST_RELEASE_CAUSE_NORMAL;
		woomera->q931_rel_cause_tosig=SIGBOOST_RELEASE_CAUSE_NORMAL;
    
		woomera_set_interface(woomera, "w-1g-1");

		

	}

	return woomera;

}


static struct woomera_interface *new_woomera_interface(int socket, struct sockaddr_in *sock_addr, int len) 
{
    	struct woomera_interface *woomera = NULL;
	
    	if (socket < 0) {
		log_printf(0, server.log, "Critical: Invalid Socket on new interface!\n");
		return NULL;
    	}

    	if ((woomera = alloc_woomera())) {
		if (socket >= 0) {
			no_nagle(socket);
			woomera->socket = socket;
		}

		if (sock_addr && len) {
			memcpy(&woomera->addr, sock_addr, len);
		}
	}

	return woomera;

}

static char *woomera_message_header(struct woomera_message *wmsg, char *key) 
{
    int x = 0;
    char *value = NULL;

    for (x = 0 ; x < wmsg->last ; x++) {
		if (!strcasecmp(wmsg->names[x], key)) {
			value = wmsg->values[x];
			break;
		}
    }

    return value;
}


#if 1

static int waitfor_socket(int fd, int timeout, int flags)
{
    struct pollfd pfds[1];
    int res;
 
    memset(&pfds[0], 0, sizeof(pfds[0]));
    pfds[0].fd = fd;
    pfds[0].events = flags;
    res = poll(pfds, 1, timeout);

    if (res > 0) {
	if(pfds[0].revents & POLLIN) {
		res = 1;
	} else if ((pfds[0].revents & POLLERR)) {
		res = -1;
	} else if ((pfds[0].revents & POLLNVAL)) {
		res = -2;
#if 0
		log_printf(0, server.log,"System Warning: Poll Event NVAL (0x%X) (fd=%i)!\n",
				pfds[0].revents, fd);
#endif
    	} else {
#if 0
		log_printf(0, server.log,"System Error: Poll Event Error no event (0x%X) (fd=%i)!\n",
				pfds[0].revents,fd);
#endif
		res = -1;
	}
    }

    return res;
}

#else

static int waitfor_socket(int fd, int timeout, int flags)
{
    struct pollfd pfds[1];
    int res;
    int errflags = (POLLERR | POLLHUP | POLLNVAL);

    memset(&pfds[0], 0, sizeof(pfds[0]));
    pfds[0].fd = fd;
    pfds[0].events = flags | errflags;
    res = poll(pfds, 1, timeout);

    if (res > 0) {
	if(pfds[0].revents & POLLIN) {
		res = 1;
	} else if ((pfds[0].revents & errflags)) {
		res = -1;
    	} else {
#if 0
		log_printf(0, server.log,"System Error: Poll Event Error no event (0x%X)!\n",
				pfds[0].revents);
#endif
		res = -1;
	}
    }

    return res;
}

#endif

#if 1
static int waitfor_2sockets(int fda, int fdb, char *a, char *b, int timeout) 
{
    struct pollfd pfds[2];
    int res = 0;
    int errflags = (POLLERR | POLLHUP | POLLNVAL);

    if (fda < 0 || fdb < 0) {
		return -1;
    }

    *a=0;
    *b=0;

    memset(pfds, 0, sizeof(pfds));
    pfds[0].fd = fda;
    pfds[1].fd = fdb;
    pfds[0].events = POLLIN | errflags;
    pfds[1].events = POLLIN | errflags;
    if ((res = poll(pfds, 2, timeout)) > 0) {
		res = 1;
		if ((pfds[0].revents & errflags) || (pfds[1].revents & errflags)) {
			res = -1;
		} else { 
			if ((pfds[0].revents & POLLIN)) {
				*a=1;
				res++;
			}
			if ((pfds[1].revents & POLLIN)) {
				*b=1;		
				res++;
			}
		}

		if (res == 1) {
			/* No event found what to do */
			res=-1;
		}
    }
	
    return res;
}
#endif


static int create_udp_socket(struct media_session *ms, char *local_ip, int local_port, char *ip, int port)
{
    int rc;
    struct hostent *result, *local_result;
    char buf[512], local_buf[512];
    int err = 0;

    log_printf(5,server.log,"LocalIP %s:%d IP %s:%d \n",local_ip, local_port, ip, port);

    memset(&ms->remote_hp, 0, sizeof(ms->remote_hp));
    memset(&ms->local_hp, 0, sizeof(ms->local_hp));
    if ((ms->socket = socket(AF_INET, SOCK_DGRAM, 0))) {
		gethostbyname_r(ip, &ms->remote_hp, buf, sizeof(buf), &result, &err);
		gethostbyname_r(local_ip, &ms->local_hp, local_buf, sizeof(local_buf), &local_result, &err);
		if (result && local_result) {
			ms->remote_addr.sin_family = ms->remote_hp.h_addrtype;
			memcpy((char *) &ms->remote_addr.sin_addr.s_addr, ms->remote_hp.h_addr_list[0], ms->remote_hp.h_length);
			ms->remote_addr.sin_port = htons(port);

			ms->local_addr.sin_family = ms->local_hp.h_addrtype;
			memcpy((char *) &ms->local_addr.sin_addr.s_addr, ms->local_hp.h_addr_list[0], ms->local_hp.h_length);
			ms->local_addr.sin_port = htons(local_port);
    
			rc = bind(ms->socket, (struct sockaddr *) &ms->local_addr, sizeof(ms->local_addr));
			if (rc < 0) {
				close(ms->socket);
				ms->socket = -1;
    			
				log_printf(5,server.log,
					"Failed to bind LocalIP %s:%d IP %s:%d (%s)\n",
						local_ip, local_port, ip, port,strerror(errno));
			} 

			/* OK */

		} else {
    			log_printf(0,server.log,
				"Failed to get hostbyname LocalIP %s:%d IP %s:%d (%s)\n",
					local_ip, local_port, ip, port,strerror(errno));
		}
    } else {
    	log_printf(0,server.log,
		"Failed to create/allocate UDP socket\n");
    }

    return ms->socket;
}

static int next_media_port(void)
{
    int port;
    
    pthread_mutex_lock(&server.media_udp_port_lock);
    port = ++server.next_media_port;
    if (port > WOOMERA_MAX_MEDIA_PORT) {
    		server.next_media_port = WOOMERA_MIN_MEDIA_PORT;
		port = WOOMERA_MIN_MEDIA_PORT;
    }
    pthread_mutex_unlock(&server.media_udp_port_lock);
    
    return port;
}


static int woomera_message_parse(struct woomera_interface *woomera, struct woomera_message *wmsg, int timeout) 
{
    char *cur, *cr, *next = NULL, *eor = NULL;
    char buf[2048];
    int res = 0, bytes = 0, sanity = 0;
    struct timeval started, ended;
    int elapsed, loops = 0;
    int failto = 0;
    int packet = 0;

    memset(wmsg, 0, sizeof(*wmsg));

    if (woomera->socket < 0 ) {
   	 log_printf(2, woomera->log, WOOMERA_DEBUG_PREFIX "%s Invalid Socket! %d\n", 
			 woomera->interface,woomera->socket);
		return -1;
    }

	if (woomera_test_flag(woomera, WFLAG_MEDIA_END) || 
            woomera_test_flag(woomera, WFLAG_HANGUP)) {
		log_printf(5, woomera->log, WOOMERA_DEBUG_PREFIX 
			"%s MEDIA END or HANGUP !\n", 
			woomera->interface);
		return -1;
	}

    gettimeofday(&started, NULL);
    memset(buf, 0, sizeof(buf));

    if (timeout < 0) {
		timeout = abs(timeout);
		failto = 1;
    } else if (timeout == 0) {
		timeout = -1;
    }
	

    while (!(eor = strstr(buf, WOOMERA_RECORD_SEPERATOR))) {
		if (sanity > 1000) {
			log_printf(2, woomera->log, WOOMERA_DEBUG_PREFIX "%s Failed Sanity Check!\n[%s]\n\n", woomera->interface, buf);
			return -1;
		}

		if ((res = waitfor_socket(woomera->socket, 1000, POLLERR | POLLIN) > 0)) {
			res = recv(woomera->socket, buf, sizeof(buf), MSG_PEEK);

			if (res > 1) {
				packet++;
			}
			if (!strncmp(buf, WOOMERA_LINE_SEPERATOR, 2)) {
				res = read(woomera->socket, buf, 2);
				return 0;
			}
			if (res == 0) {
				sanity++;
				/* Looks Like it's time to go! */
				if (!woomera_test_flag(&server.master_connection, WFLAG_RUNNING) ||
				    woomera_test_flag(woomera, WFLAG_MEDIA_END) || 
				    woomera_test_flag(woomera, WFLAG_HANGUP)) {
					log_printf(5, woomera->log, WOOMERA_DEBUG_PREFIX 
						"%s MEDIA END or HANGUP \n", woomera->interface);
					return -1;
				}
				ysleep(1000);
				continue;
			} else if (res < 0) {
				log_printf(3, woomera->log, WOOMERA_DEBUG_PREFIX 
					"%s error during packet retry #%d\n", 
						woomera->interface, loops);
				return res;
			} else if (loops) {
				ysleep(100000);
			}
		}
		
		gettimeofday(&ended, NULL);
		elapsed = (((ended.tv_sec * 1000) + ended.tv_usec / 1000) - ((started.tv_sec * 1000) + started.tv_usec / 1000));

		if (res < 0) {
			log_printf(2, woomera->log, WOOMERA_DEBUG_PREFIX "%s Bad RECV\n", 
				woomera->interface);
			return res;
		} else if (res == 0) {
			sanity++;
			/* Looks Like it's time to go! */
			if (!woomera_test_flag(&server.master_connection, WFLAG_RUNNING) ||
			    woomera_test_flag(woomera, WFLAG_MEDIA_END) || 
			    woomera_test_flag(woomera, WFLAG_HANGUP)) {
				log_printf(5, woomera->log, WOOMERA_DEBUG_PREFIX 
					"%s MEDIA END or HANGUP \n", woomera->interface);
					return -1;
			}
			ysleep(1000);
			continue;
		}

		if (packet && loops > 150) {
			log_printf(1, woomera->log, WOOMERA_DEBUG_PREFIX 
					"%s Timeout waiting for packet.\n", 
						woomera->interface);
			return -1;
		}

		if (timeout > 0 && (elapsed > timeout)) {
			log_printf(1, woomera->log, WOOMERA_DEBUG_PREFIX 
					"%s Timeout [%d] reached\n", 
						woomera->interface, timeout);
			return failto ? -1 : 0;
		}

		if (woomera_test_flag(woomera, WFLAG_EVENT)) {
			/* BRB! we have an Event to deliver....*/
			return 0;
		}

		/* what're we still doing here? */
		if (!woomera_test_flag(&server.master_connection, WFLAG_RUNNING) || 
		    !woomera_test_flag(woomera, WFLAG_RUNNING)) {
			log_printf(2, woomera->log, WOOMERA_DEBUG_PREFIX 
				"%s MASTER RUNNING or RUNNING!\n", woomera->interface);
			return -1;
		}
		loops++;
    }

    *eor = '\0';
    bytes = strlen(buf) + 4;
	
    memset(buf, 0, sizeof(buf));
    res = read(woomera->socket, buf, bytes);
    next = buf;

    if (woomera->debug > 1) {
		log_printf(3, woomera->log, "%s:WOOMERA RX MSG: %s\n",woomera->interface,buf);

    }
	
    while ((cur = next)) {

		if ((cr = strstr(cur, WOOMERA_LINE_SEPERATOR))) {
			*cr = '\0';
			next = cr + (sizeof(WOOMERA_LINE_SEPERATOR) - 1);
			if (!strcmp(next, WOOMERA_RECORD_SEPERATOR)) {
				break;
			}
		} 
		if (!cur || !*cur) {
			break;
		}

		if (!wmsg->last) {
			char *cmd, *id, *args;
			woomera_set_flag(wmsg, MFLAG_EXISTS);
			cmd = cur;

			if ((id = strchr(cmd, ' '))) {
				*id = '\0';
				id++;
				if ((args = strchr(id, ' '))) {
					*args = '\0';
					args++;
					strncpy(wmsg->command_args, args, sizeof(wmsg->command_args)-1);
				}
				strncpy(wmsg->callid, id, sizeof(wmsg->callid)-1);
			}

			strncpy(wmsg->command, cmd, sizeof(wmsg->command)-1);
		} else {
			char *name, *val;
			name = cur;

			if ((val = strchr(name, ':'))) {
				*val = '\0';
				val++;
				while (*val == ' ') {
					*val = '\0';
					val++;
				}
				strncpy(wmsg->values[wmsg->last-1], val, WOOMERA_STRLEN);
			}
			strncpy(wmsg->names[wmsg->last-1], name, WOOMERA_STRLEN);
			if (name && val && !strcasecmp(name, "content-type")) {
				woomera_set_flag(wmsg, MFLAG_CONTENT);
				bytes = atoi(val);
			}
			if (name && val && !strcasecmp(name, "content-length")) {
				woomera_set_flag(wmsg, MFLAG_CONTENT);
				bytes = atoi(val);
			}


		}
		wmsg->last++;
    }

    wmsg->last--;

    if (bytes && woomera_test_flag(wmsg, MFLAG_CONTENT)) {
		read(woomera->socket, wmsg->body, 
		     (bytes > sizeof(wmsg->body)) ? sizeof(wmsg->body) : bytes);
    }

    return woomera_test_flag(wmsg, MFLAG_EXISTS);

}

static struct woomera_interface *pull_from_holding_tank(int index, int span , int chan)
{
	struct woomera_interface *woomera = NULL;
	
	if (index < 1 || index >= CORE_TANK_LEN) {
		log_printf(0, server.log, "%s Error on invalid TANK INDEX = %i\n",
			__FUNCTION__,index);
		return NULL;
	}

	pthread_mutex_lock(&server.ht_lock);
	if (server.holding_tank[index] && 
	    server.holding_tank[index] != &woomera_dead_dev) {
			woomera = server.holding_tank[index];
	
			/* Block this index until the call is completed */
			server.holding_tank[index] = &woomera_dead_dev;
			
			woomera->timeout = 0;
			woomera->index = 0;
			woomera->span=span;
			woomera->chan=chan;
	}
	pthread_mutex_unlock(&server.ht_lock);
	
	return woomera;
}

static void clear_from_holding_tank(int index, struct woomera_interface *woomera)
{

	if (index < 1 || index >= CORE_TANK_LEN) {
		log_printf(0, server.log, "%s Error on invalid TANK INDEX = %i\n",
			__FUNCTION__,index);
		return;
	}
	
	pthread_mutex_lock(&server.ht_lock);
	if (server.holding_tank[index] == &woomera_dead_dev) {
		server.holding_tank[index] = NULL;
	} else if (woomera && server.holding_tank[index] == woomera) {
		server.holding_tank[index] = NULL;
	} else if (server.holding_tank[index]) {
		log_printf(0, server.log, "Error: Holding tank index %i not cleared %p !\n",
				index, server.holding_tank[index]);
	}
	pthread_mutex_unlock(&server.ht_lock);
	
	return;
}

static struct woomera_interface *peek_from_holding_tank(int index)
{
	struct woomera_interface *woomera = NULL;
	
	if (index < 1 || index >= CORE_TANK_LEN) {
		log_printf(0, server.log, "%s Error on invalid TANK INDEX = %i\n",
			__FUNCTION__,index);
		return NULL;
	}
	
	pthread_mutex_lock(&server.ht_lock);
	if (server.holding_tank[index] && 
		server.holding_tank[index] != &woomera_dead_dev) {
			woomera = server.holding_tank[index];
	}
	pthread_mutex_unlock(&server.ht_lock);
	
	return woomera;
}

static int add_to_holding_tank(struct woomera_interface *woomera)
{
	int next, i, found=0;
	
	pthread_mutex_lock(&server.ht_lock);
	
	for (i=0;i<CORE_TANK_LEN;i++) {    
		next = ++server.holding_tank_index;
		if (server.holding_tank_index >= CORE_TANK_LEN) {
			next = server.holding_tank_index = 1;
		} 
	
		if (next == 0) {
			log_printf(0, server.log, "\nCritical Error on TANK INDEX == 0\n");
			continue;
		}
	
		if (server.holding_tank[next]) {
			continue;
		}
	
		found=1;
		break;
	}
	
	if (!found) {
		/* This means all tank vales are busy
		* should never happend */
		pthread_mutex_unlock(&server.ht_lock);
		log_printf(0, server.log, "\nCritical Error failed to obtain a TANK INDEX\n");
		return 0;
	}
	
	server.holding_tank[next] = woomera;
	woomera->timeout = time(NULL) + 100;
		
	pthread_mutex_unlock(&server.ht_lock);
	return next;
}


int initiate_call_unit_start (int span, int chan)
{
	int tg = 0;
	char rdnis[100];
	char calling[100];
	char called[100];
	call_signal_event_t event;

	sprintf(calling,"9054741990");
	sprintf(called, "4168162992");

	memset(rdnis,0,sizeof(rdnis));
	
	call_signal_call_init(&event, calling, called, 0);
	
	event.calling_number_presentation = 0;
	
	event.calling_number_screening_ind = 0;
	
	event.span=span;
	event.chan=chan;

#if 0
	if (strlen(rdnis)) {
		strncpy((char*)event.isup_in_rdnis,rdnis,
				sizeof(event.isup_in_rdnis)-1);
		log_printf(0,server.log,"RDNIS %s\n", rdnis);
	}
#endif

#ifdef SMG_CALLING_NAME
	if (calling_name) {
		strncpy((char*)event.calling_name,calling_name,
				sizeof(event.calling_name)-1);
	}
#endif

	event.trunk_group = tg;

	if (call_signal_connection_write(&server.mcon, &event) < 0) {
		log_printf(0, server.log, 
		"Critical System Error: Failed to tx on ISUP socket [%s]: %s\n", 
			strerror(errno));
		return -1;
	}
	
	return 0;
}


int initiate_loop_unit_start (int span, int chan)
{
	int tg = 0;
	char rdnis[100];
	char calling[100];
	char called[100];
	call_signal_event_t event;

	sprintf(calling,"9054741990");
	sprintf(called, "4168162992");

	memset(rdnis,0,sizeof(rdnis));
	
	call_signal_call_init(&event, calling, called, 0);
	
	event.calling_number_presentation = 0;
	
	event.calling_number_screening_ind = 0;
	
	event.span=span;
	event.chan=chan;

	event.event_id = SIGBOOST_EVENT_INSERT_CHECK_LOOP;

	if (strlen(rdnis)) {
		strncpy((char*)event.isup_in_rdnis,rdnis,
				sizeof(event.isup_in_rdnis)-1);
		log_printf(0,server.log,"RDNIS %s\n", rdnis);
	}

#ifdef SMG_CALLING_NAME
	if (calling_name) {
		strncpy((char*)event.calling_name,calling_name,
				sizeof(event.calling_name)-1);
	}
#endif

	event.trunk_group = tg;

	if (call_signal_connection_write(&server.mcon, &event) < 0) {
		log_printf(0, server.log, 
		"Critical System Error: Failed to tx on ISUP socket %s\n",
			strerror(errno));
		return -1;
	}
	
	return 0;
}



static int parse_ss7_event(call_signal_connection_t *mcon, short_signal_event_t *event)
{
    	int ret = 0;

	switch(event->event_id) {
	
	case SIGBOOST_EVENT_CALL_START:
			handle_call_start((call_signal_event_t*)event);
			break;
	case SIGBOOST_EVENT_CALL_STOPPED:
			handle_call_stop(event);
			break;
	case SIGBOOST_EVENT_CALL_START_ACK:
			handle_call_start_ack(event);
			break;
	case SIGBOOST_EVENT_CALL_START_NACK:
			handle_call_start_nack(event);
			break;
	case SIGBOOST_EVENT_CALL_ANSWERED:
			handle_call_answer(event);
			break;
	case SIGBOOST_EVENT_HEARTBEAT:
			handle_heartbeat(event);
			break;
	case SIGBOOST_EVENT_CALL_START_NACK_ACK:
			handle_call_start_nack_ack(event);
			break;
	case SIGBOOST_EVENT_CALL_STOPPED_ACK:
			handle_call_stop_ack(event);
			break;
	case SIGBOOST_EVENT_INSERT_CHECK_LOOP:
			handle_call_loop_start((call_signal_event_t*)event);
			break;
	case SIGBOOST_EVENT_SYSTEM_RESTART:
                        handle_restart(mcon,event);
			break;
	case SIGBOOST_EVENT_REMOVE_CHECK_LOOP:
			handle_remove_loop(event);
			break;
	case SIGBOOST_EVENT_SYSTEM_RESTART_ACK:
			handle_restart_ack(mcon,event);
			break;
	case SIGBOOST_EVENT_AUTO_CALL_GAP_ABATE:
			handle_gap_abate(event);
			break;
	default:
			log_printf(0, server.log, "Warning no handler implemented for [%s]\n", 
				call_signal_event_id_name(event->event_id));
			break;
	}
		
	return ret;
}


static void *monitor_thread_run(void *obj)
{
	int ss = 0;
	int policy=0,priority=0;
	char a=0,b=0;
	call_signal_connection_t *mcon=&server.mcon;
	call_signal_connection_t *mconp=&server.mconp;

    	pthread_mutex_lock(&server.thread_count_lock);
	server.thread_count++;
    	pthread_mutex_unlock(&server.thread_count_lock);
	
	woomera_set_flag(&server.master_connection, WFLAG_MONITOR_RUNNING);
	
	if (call_signal_connection_open(mcon, 
					mcon->cfg.local_ip, 
					mcon->cfg.local_port,
					mcon->cfg.remote_ip,
					mcon->cfg.remote_port) < 0) {
			log_printf(0, server.log, "Error: Opening MCON Socket [%d] %s\n", 
					mcon->socket,strerror(errno));
			exit(-1);
	}

	if (call_signal_connection_open(mconp,
					mconp->cfg.local_ip, 
					mconp->cfg.local_port,
					mconp->cfg.remote_ip,
					mconp->cfg.remote_port) < 0) {
			log_printf(0, server.log, "Error: Opening MCONP Socket [%d] %s\n", 
					mconp->socket,strerror(errno));
			exit(-1);
	}
	
	mcon->log = server.log;
	mconp->log = server.log;
	
	smg_get_current_priority(&policy,&priority);
	
	log_printf(1, server.log, "Open udp socket [%d] [%d]\n",
				 mcon->socket,mconp->socket);
	log_printf(1, server.log, "Monitor Thread Started (%i:%i)\n",policy,priority);
				
	
	while (woomera_test_flag(&server.master_connection, WFLAG_RUNNING) &&
		woomera_test_flag(&server.master_connection, WFLAG_MONITOR_RUNNING)) {
#if 0
		ss = waitfor_socket(server.mcon.socket, 1000, POLLERR | POLLIN);
#else
		ss = waitfor_2sockets(mcon->socket,
				      mconp->socket,
				      &a, &b, 1000);
#endif	
		
		if (ss > 0) {

			call_signal_event_t *event=NULL;
			int i=0;
		
			if (b) { 
mcon_retry_priority:
			    if ((event = call_signal_connection_readp(mconp,i))) {
				struct timeval current;
                               	struct timeval difftime;
                               	gettimeofday(&current,NULL);
                               	timersub (&current, &event->tv, &difftime);
#if 0
                               	log_printf(6, server.log, "Socket Event P [%s] T=%d:%d\n",
                               	call_signal_event_id_name(event->event_id),
                                       	difftime.tv_sec, difftime.tv_usec);
#endif
                               	parse_ss7_event(mconp,(short_signal_event_t*)event);
				if (++i < 10) {
					goto mcon_retry_priority;
				}
				
			    } else if (errno != EAGAIN) {
				ss=-1;
				log_printf(0, server.log, 
					"Error: Reading from Boost P Socket! (%i) %s\n",
					errno,strerror(errno));
				break;
			    }
			}
		
			i=0;

			if (a) {
mcon_retry:
			    if ((event = call_signal_connection_read(mcon,i))) {
      				struct timeval current;
                               	struct timeval difftime;
                               	gettimeofday(&current,NULL);
                               	timersub (&current, &event->tv, &difftime);
#if 0
                               	log_printf(3, server.log, "Socket Event [%s] T=%d:%d\n",
                               			call_signal_event_id_name(event->event_id),
                                       		difftime.tv_sec, difftime.tv_usec);
#endif
                               	parse_ss7_event(mcon,(short_signal_event_t*)event);

				if (++i < 50) {
					goto mcon_retry;
				}
			    } else if (errno != EAGAIN) {
				ss=-1;
				log_printf(0, server.log, 
					"Error: Reading from Boost Socket! (%i) %s\n",
					errno,strerror(errno));
				break;
			    }
			} 

		} 

		if (ss < 0){
			log_printf(0, server.log, "Thread Run: Select Socket Error!\n");
			break;
		}
		
	}
 
	log_printf(1, server.log, "Close udp socket [%d] [%d]\n", 
					mcon->socket,mconp->socket);
	call_signal_connection_close(&server.mcon);
	call_signal_connection_close(&server.mconp);

    	pthread_mutex_lock(&server.thread_count_lock);
	server.thread_count--;
    	pthread_mutex_unlock(&server.thread_count_lock);
	
	woomera_clear_flag(&server.master_connection, WFLAG_MONITOR_RUNNING);
	log_printf(0, server.log, "Monitor Thread Ended\n");
	
	return NULL;
}



static int launch_monitor_thread(void) 
{
    pthread_attr_t attr;
    int result = 0;
    struct sched_param param;

    param.sched_priority = 10;
    result = pthread_attr_init(&attr);
    pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);
    pthread_attr_setschedpolicy(&attr, SCHED_RR);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    pthread_attr_setstacksize(&attr, MGD_STACK_SIZE);

    result = pthread_attr_setschedparam (&attr, &param);
    
    log_printf(0,server.log,"%s: Old Priority =%i res=%i \n",__FUNCTION__,
			 param.sched_priority,result);


    woomera_set_flag(&server.master_connection, WFLAG_MONITOR_RUNNING);
    result = pthread_create(&server.monitor_thread, &attr, monitor_thread_run, NULL);
    if (result) {
	log_printf(0, server.log, "%s: Error: Creating Thread! %s\n",
			 __FUNCTION__,strerror(errno));
	 woomera_clear_flag(&server.master_connection, WFLAG_MONITOR_RUNNING);
    } 
    pthread_attr_destroy(&attr);

    return result;
}


#ifdef WP_HPTDM_API
static void *hp_tdmapi_span_run(void *obj)
{
    hp_tdm_api_span_t *span = obj;
    int err; 
    
    log_printf(0,server.log,"Starting %s span!\n",span->ifname);
    
    while (woomera_test_flag(&server.master_connection, WFLAG_RUNNING) &&
           woomera_test_flag(&server.master_connection, WFLAG_MONITOR_RUNNING)) {
    	
	if (!span->run_span) {
		break;
	}
	
	err = span->run_span(span);
	if (err) {
		break;
	}
	
    }

    if (span->close_span) {
    	span->close_span(span);
    }
        
    sleep(3);
    log_printf(0,server.log,"Stopping %s span!\n",span->ifname);
    
    pthread_exit(NULL);	    
}


static int launch_hptdm_api_span_thread(int span)
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
    
    result = pthread_create(&server.monitor_thread, &attr, hp_tdmapi_span_run, &hptdmspan[span]);
    if (result) {
	 log_printf(0, server.log, "%s: Error: Creating Thread! %s\n",
			 __FUNCTION__,strerror(errno));
    } 
    pthread_attr_destroy(&attr);

    return result;
}
#endif

static int configure_server(void)
{
    struct woomera_config cfg;
    char *var, *val;
    int cnt = 0;

    server.dtmf_intr_ch = -1;

    if (!woomera_open_file(&cfg, server.config_file)) {
		log_printf(0, server.log, "open of %s failed\n", server.config_file);
		return 0;
    }
	
    while (woomera_next_pair(&cfg, &var, &val)) {
		if (!strcasecmp(var, "boost_local_ip")) {
			strncpy(server.mcon.cfg.local_ip, val,
				 sizeof(server.mcon.cfg.local_ip) -1);
			strncpy(server.mconp.cfg.local_ip, val,
				 sizeof(server.mconp.cfg.local_ip) -1);
			cnt++;
		} else if (!strcasecmp(var, "boost_local_port")) {
			server.mcon.cfg.local_port = atoi(val);
			server.mconp.cfg.local_port = 
				server.mcon.cfg.local_port+1;
			cnt++;
		} else if (!strcasecmp(var, "boost_remote_ip")) {
			strncpy(server.mcon.cfg.remote_ip, val,
				 sizeof(server.mcon.cfg.remote_ip) -1);
			strncpy(server.mconp.cfg.remote_ip, val,
				 sizeof(server.mconp.cfg.remote_ip) -1);
			cnt++;
		} else if (!strcasecmp(var, "boost_remote_port")) {
			server.mcon.cfg.remote_port = atoi(val);
			server.mconp.cfg.remote_port = 
				server.mcon.cfg.remote_port+1;
			cnt++;
		} else if (!strcasecmp(var, "logfile_path")) {
			if (!server.logfile_path) {
				server.logfile_path = strdup(val);
			}
		} else if (!strcasecmp(var, "woomera_port")) {
			server.port = atoi(val);
		} else if (!strcasecmp(var, "debug_level")) {
			server.debug = atoi(val);
		} else if (!strcasecmp(var, "out_tx_test")) {
		        server.out_tx_test = atoi(val);
		} else if (!strcasecmp(var, "loop_trace")) {
		        server.loop_trace = atoi(val);
		} else if (!strcasecmp(var, "rxgain")) {
		        server.rxgain = atoi(val);
		} else if (!strcasecmp(var, "txgain")) {
		        server.txgain = atoi(val);
		} else if (!strcasecmp(var, "dtmf_on_duration")){
			server.dtmf_on = atoi(val);
		} else if (!strcasecmp(var, "dtmf_off_duration")){
			server.dtmf_off = atoi(val);
		} else if (!strcasecmp(var, "dtmf_inter_ch_duration")){
			server.dtmf_intr_ch = atoi(val);
		} else if (!strcasecmp(var, "max_calls")) {
			int max = atoi(val);
			if (max > 0) {
				server.max_calls = max;
			}
		} else if (!strcasecmp(var, "autoacm")) {
			int max = atoi(val);
			if (max >= 0) {
				autoacm=max;
			}			

		} else if (!strcasecmp(var, "media_ip")) {
			strncpy(server.media_ip, val, sizeof(server.media_ip) -1);
		} else {
			log_printf(0, server.log, "Invalid Option %s at line %d!\n", var, cfg.lineno);
		}
    }

    /* Post initialize */
    if (server.dtmf_on == 0){ 
	server.dtmf_on=SMG_DTMF_ON;
    }
    if (server.dtmf_off == 0) {
	server.dtmf_off=SMG_DTMF_OFF;
    }
    if (server.dtmf_intr_ch == -1) {
	server.dtmf_intr_ch = server.dtmf_on/server.dtmf_off;	
    }
    server.dtmf_size=(server.dtmf_on+server.dtmf_off)*10*2;

    log_printf(0,server.log, "DTMF On=%i Off=%i IntrCh=%i Size=%i\n",
		server.dtmf_on,server.dtmf_off,server.dtmf_intr_ch,server.dtmf_size);

    woomera_close_file(&cfg);
    return cnt == 4 ? 1 : 0;
}



static int main_thread(void)
{
	struct sockaddr_in sock_addr, client_addr;
	struct woomera_interface *new_woomera;
	int pid = 0;
	FILE *tmp;

	if ((pid = get_pid_from_file(PIDFILE_UNIT))) {
		log_printf(0, stderr, "pid %d already exists.\n", pid);
		exit(0);
	}
	
	if (!(tmp = safe_fopen(PIDFILE_UNIT, "w"))) {
			log_printf(0, stderr, "Error creating pidfile %s\n", PIDFILE_UNIT);
			return 1;
	} else {
			fprintf(tmp, "%d", getpid());
			fclose(tmp);
			tmp = NULL;
	}


 	woomera_set_flag(&server.master_connection, WFLAG_RUNNING);
	monitor_thread_run(NULL);
    	return 0;
}

static int do_ignore(int sig)
{
    return 0;
}

static int do_shut(int sig)
{
    woomera_clear_flag(&server.master_connection, WFLAG_RUNNING);
    close_socket(&server.master_connection.socket);
    log_printf(1, server.log, "Caught SIG %d, Closing Master Socket!\n", sig);
    return 0;
}

static int sangoma_tdm_init (int span)
{
#ifdef LIBSANGOMA_GET_HWCODING
    wanpipe_tdm_api_t tdm_api;
    int fd=sangoma_open_tdmapi_span(span);
    if (fd < 0 ){
        return -1;
    } else {
        server.hw_coding=sangoma_tdm_get_hw_coding(fd,&tdm_api);
        close_socket(&fd);
    }
#else
#error "libsangoma missing hwcoding feature: not up to date!"
#endif
    return 0;
}


static int woomera_startup(int argc, char **argv)
{
    int x = 0, pid = 0, bg = 0;
    char *cfg=NULL, *debug=NULL, *arg=NULL;

    while((arg = argv[x++])) {

		if (!strcasecmp(arg, "-hup")) {
			if (! (pid = get_pid_from_file(PIDFILE_UNIT))) {
				log_printf(0, stderr, "Error reading pidfile %s\n", PIDFILE_UNIT);
				exit(1);
			} else {
				log_printf(0, stderr, "Killing PID %d\n", pid);
				kill(pid, SIGHUP);
				sleep(1);
				exit(0);
			}
		
		 } else if (!strcasecmp(arg, "-term") || !strcasecmp(arg, "--term")) {
                        if (! (pid = get_pid_from_file(PIDFILE_UNIT))) {
                                log_printf(0, stderr, "Error reading pidfile %s\n", PIDFILE_UNIT);
                                exit(1);
                        } else {
                                log_printf(0, stderr, "Killing PID %d\n", pid);
                                kill(pid, SIGTERM);
                                unlink(PIDFILE_UNIT);
				sleep(1);
                                exit(0);
                        }	

		} else if (!strcasecmp(arg, "-version")) {
                        fprintf(stdout, "\nSangoma Media Gateway: Version %s\n\n", SMG_VERSION);
			exit(0);
			
		} else if (!strcasecmp(arg, "-help")) {
			fprintf(stdout, "%s\n%s [-help] | [ -version] | [-hup] | [-wipe] | [[-bg] | [-debug <level>] | [-cfg <path>] | [-log <path>]]\n\n", WELCOME_TEXT, argv[0]);
			exit(0);
		} else if (!strcasecmp(arg, "-wipe")) {
			unlink(PIDFILE_UNIT);
		} else if (!strcasecmp(arg, "-bg")) {
			bg = 1;

		} else if (!strcasecmp(arg, "-g")) {
                        coredump = 1;

		} else if (!strcasecmp(arg, "-debug")) {
			if (argv[x] && *(argv[x]) != '-') {
				debug = argv[x++];
			}
		} else if (!strcasecmp(arg, "-cfg")) {
			if (argv[x] && *(argv[x]) != '-') {
				cfg = argv[x++];
			}
		} else if (!strcasecmp(arg, "-log")) {
			if (argv[x] && *(argv[x]) != '-') {
				server.logfile_path = strdup(argv[x++]);
			}
		} else if (*arg == '-') {
			log_printf(0, stderr, "Unknown Option %s\n", arg);
			fprintf(stdout, "%s\n%s [-help] | [-hup] | [-wipe] | [[-bg] | [-debug <level>] | [-cfg <path>] | [-log <path>]]\n\n", WELCOME_TEXT, argv[0]);
			exit(1);
		}
    }

    if (0){
	int spn; 
    	for (spn=1;spn<=WOOMERA_MAX_SPAN;spn++) {
    		if (sangoma_tdm_init(spn) == 0) {
			break;
		}
    	}
	if (spn>WOOMERA_MAX_SPAN) {
        	printf("\nError: Failed to access a channel on spans 1-16\n");
        	printf("         Please start Wanpipe TDM API drivers\n");
		return 0;
	}
    }

    if (bg && (pid = fork())) {
		log_printf(0, stderr, "Backgrounding!\n");
		return 0;
    }

    q931_cause_setup();

    server.port = 42420;
    server.debug = 0;
    strcpy(server.media_ip, "127.0.0.1");
    server.next_media_port = WOOMERA_MIN_MEDIA_PORT;
    server.log = stdout;
    server.master_connection.socket = -1;
    server.master_connection.event_queue = NULL;
    server.config_file = cfg ? cfg : "/etc/sangoma_mgd_unit.conf";
    pthread_mutex_init(&server.listen_lock, NULL);	
    pthread_mutex_init(&server.ht_lock, NULL);	
    pthread_mutex_init(&server.process_lock, NULL);
    pthread_mutex_init(&server.media_udp_port_lock, NULL);	
    pthread_mutex_init(&server.thread_count_lock, NULL);	
    pthread_mutex_init(&server.master_connection.queue_lock, NULL);	
    pthread_mutex_init(&server.master_connection.flags_lock, NULL);	
    pthread_mutex_init(&server.mcon.lock, NULL);	
	server.master_connection.chan = -1;
	server.master_connection.span = -1;

    if (!configure_server()) {
		log_printf(0, server.log, "configuration failed!\n");
		return 0;
    }

#ifndef USE_SYSLOG
    if (server.logfile_path) {
		if (!(server.log = safe_fopen(server.logfile_path, "a"))) {
			log_printf(0, stderr, "Error setting logfile %s!\n", server.logfile_path);
			server.log = stderr;
			return 0;
		}
    }
#endif
	
	
    if (debug) {
		server.debug = atoi(debug);
    }

    if (coredump) {
	struct rlimit l;
	memset(&l, 0, sizeof(l));
	l.rlim_cur = RLIM_INFINITY;
	l.rlim_max = RLIM_INFINITY;
	if (setrlimit(RLIMIT_CORE, &l)) {
		log_printf(0, stderr,  "Warning: Failed to disable core size limit: %s\n", 
			strerror(errno));
	}
    }

#ifdef __LINUX__
    if (geteuid() && coredump) {
    	if (prctl(PR_SET_DUMPABLE, 1, 0, 0, 0) < 0) {
		log_printf(0, stderr,  "Warning: Failed to disable core size limit for non-root: %s\n", 
			strerror(errno));
        }
    }
#endif



    (void) signal(SIGINT,(void *) do_shut);
    (void) signal(SIGPIPE,(void *) do_ignore);
    (void) signal(SIGHUP,(void *) do_shut);

   
   
	
    fprintf(stderr, "%s", WELCOME_TEXT);
    log_printf(0, stderr, "Woomera STARTUP Complete. [AutoACM=%i]\n",autoacm);

    return 1;
}

static int woomera_shutdown(void) 
{
    char *event_string;
    int told = 0, loops = 0;

    close_socket(&server.master_connection.socket);
    pthread_mutex_destroy(&server.listen_lock);
    pthread_mutex_destroy(&server.ht_lock);	
    pthread_mutex_destroy(&server.process_lock);
    pthread_mutex_destroy(&server.media_udp_port_lock);
    pthread_mutex_destroy(&server.thread_count_lock);
    pthread_mutex_destroy(&server.master_connection.queue_lock);
    pthread_mutex_destroy(&server.master_connection.flags_lock);
    pthread_mutex_destroy(&server.mcon.lock);
    woomera_clear_flag(&server.master_connection, WFLAG_RUNNING);


    if (server.logfile_path) {
		free(server.logfile_path);
		server.logfile_path = NULL;
    }

    /* delete queue */
    while ((event_string = dequeue_event(&server.master_connection))) {
		free(event_string);
    }

    while(server.thread_count > 0) {
		loops++;

		if (loops % 1000 == 0) {
			told = 0;
		}

		if (loops > 10000) {
			log_printf(0, server.log, "Red Alert! threads did not stop\n");
			assert(server.thread_count == 0);
		}

		if (told != server.thread_count) {
			log_printf(1, server.log, "Waiting For %d thread%s.\n", 
					server.thread_count, server.thread_count == 1 ? "" : "s");
			told = server.thread_count;
		}
		ysleep(10000);
    }
    unlink(PIDFILE_UNIT);
    log_printf(0, stderr, "Woomera SHUTDOWN Complete.\n");
    return 0;
}

int main(int argc, char *argv[]) 
{
    int ret = 0;
    
    mlockall(MCL_FUTURE);
    

    server.hw_coding=0;

    openlog (ps_progname ,LOG_PID, LOG_LOCAL2);  
	
    if (! (ret = woomera_startup(argc, argv))) {
		exit(0);
    }
    ret = main_thread();

    woomera_shutdown();

    return ret;
}

/** EMACS **
 * Local variables:
 * mode: C
 * c-basic-offset: 4
 * End:
 */

