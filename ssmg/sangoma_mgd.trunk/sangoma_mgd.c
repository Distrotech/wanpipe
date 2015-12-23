/*********************************************************************************
 * sangoma_mgd.c --  Sangoma Media Gateway Daemon for Sangoma/Wanpipe Cards
 *
 * Copyright 05-10, Nenad Corbic <ncorbic@sangoma.com>
 *		    Anthony Minessale II <anthmct@yahoo.com>
 *
 * This program is free software, distributed under the terms of
 * the GNU General Public License
 * 
 * =============================================
 *
 * v1.72 Nenad Corbic <ncorbic@sangoma.com>
 * Sep 22 2010
 * Bug fix from 1.71, for BRI mode we must use the ec chip
 * check not ec channel check since BRI mode starts in non persist
 * state.
 *
 * v1.71 Nenad Corbic <ncorbic@sangoma.com>
 * Jun 21 2010
 * Added a hwec_chan_status check instead of hwec check.
 * This allows some ports to have hwec disabled and others enabled.
 *
 * v1.70 Nenad Corbic <ncorbic@sangoma.com>
 * May 07 2010
 * remove_end_of_digits_char was being run on custom_data.
 * thus cutting off the custom isup parameters.
 *
 * v1.69 David Yat Sin <dyatsin@sangoma.com>
 * May 03 2010
 * Added support for WOOMERA_CUSTOM variable on outgoing calls
 *  
 * v1.68 David Yat Sin <dyatsin@sangoma.com>
 * Mar 18 2010
 * Media and RING bits implemented
 *  
 * v1.67 David Yat Sin <dyatsin@sangoma.com>
 * Mar 16 2010
 * Added sanity check for span chan on call stopped and call answered
 *
 * v1.66 David Yat Sin <dyatsin@sangoma.com>
 * Mar 15 2010
 * Fix for WFLAG_SYSTEM_RESET being cleared if when
 * no boost msg was received from signalling daemon
 *  
 * v1.65 David Yat Sin <dyatsin@sangoma.com>
 * Mar 10 2010
 * Support for TON and NPI passthrough
 * 
 * v1.64 David Yat Sin <dyatsin@sangoma.com>
 * Feb 22 2010
 * Updated to sigboost 103
 * Added checks for hwec present
 *
 * v1.63 Nenad Corbic <ncorbic@sangoma.com>
 * Jan 27 2010
 * Enabled media pass through so that
 * two woomera servers pass media directly.
 *
 * v1.62 Konrad Hammel <konrad@sangoma.com>
 * Jan 26 2010
 * Added rbs relay code
 *
 * v1.61 Konrad Hammel <konrad@sangoma.com>
 * Jan 19 2010
 * changed all_ckt_busy to be per trunk group.
 *
 * v1.60 Nenad Corbic <ncorbic@sangoma.com>
 * Jan 14 2010
 * Added media sequencing option. 
 * Check if server has it enabled.
 *
 * v1.59 Nenad Corbic <ncorbic@sangoma.com>
 * Changed w1g1 to s1c1 
 *
 * v1.58 Nenad Corbic <ncorbic@sangoma.com>
 *  Added bridge tdm to ip functionality
 *
 * v1.57 Nenad Corbic <ncorbic@sangoma.com>
 *  Support for woomera multiple profiles
 *
 * v1.56 David Yat Sin <dyatsin@sangoma.com>
 *  Changed BRI to run with HWEC in non-persist mode
 *  
 * v1.55 Nenad Corbic <ncorbic@sangoma.com>
 *  Updated the base media port to 10000 and max media ports to 5000 
 *
 * v1.54 Nenad Corbic <ncorbic@sangoma.com>
 *  Bug added in 1.51 release causing call on channel 31 to fail.
 *
 * v1.53 Nenad Corbic <ncorbic@sangoma.com>
 *  Added progress message
 * 
 * v1.52 David Yat Sin <dyatsin@sangoma.com>
 *  Changed sangoma_open_span_chan to __sangoma_span_chan
 *  to enabled shared used of file descriptors when using 
 *  with PRI in NFAS mode
 *
 * v1.51 David Yat Sin <dyatsin@sangoma.com>
 *  MAX_SPANS increased to 32.
 *  Fix for server.process_table declared incorrectly
 *
 * v1.50 Nenad Corbic <ncorbic@sangoma.com>
 *  Logic to support multiple woomera clients hanging up the
 *  channel. This feature now supprorts woomera loadbalancing
 *  with extension check on the client end.  Example, two Asterisk
 *  systems setup where Asterisk with correct extension gets the
 *  call.
 *  Changed log levels:  Loglevel 1 = production
 *                       Loglevel 2 = Boost TX/RX EVENTS
 *                       Loglevel 3 = Woomera RX Messages
 *                       Loglevel 4 = call setup debugging
 *                       Loglevel 5 = extra debugging
 *                       Loglevel 10 = full debugging
 *
 * v1.49 Nenad Corbic <ncorbic@sangoma.com>
 *	Removed tv from sigboost to make it binary compatible
 *  Updated release cause on double use return code.
 *
 * v1.48 Nenad Corbic <ncorbic@sangoma.com>
 *       Konrad Hammel <konrad@sangoma.com>
 * Jun 30 2009
 *	Added feature to disable/enable HWEC on channels that are 
 *	passing a call with a "data" only transfer capability
 *
 * v1.47 Nenad Corbic <ncorbic@sangoma.com>
 * Apr 30 2009
 *	Updated hangup woomera events. Each event must
 *	be followed by a woomera error code 200=ok >299=no ok.
 *	This update fixes the chan_woomera socket write
 *	warnings.
 *
 * v1.46 Nenad Corbic <ncorbic@sangoma.com>
 * Mar 27 2009
 * 	Major updates on socket handling. A bug was introducted
 *      when cmm and trunk was merged. 
 * 	Added configuration print in the logs.
 *
 * v1.45 Nenad Corbic <ncorbic@sangoma.com>
 * Mar 19 2009
 * 	Outbound call timeout defaulted to 300 seconds.
 *      Timeout on answer instead of accept.
 *
 * v1.44 Nenad Corbic <ncorbic@sangoma.com>
 * Mar 19 2009
 *	Adjustable boost size. Added boost version check.	
 *
 * v1.43 David Yat Sin <dyatsin@sangoma.com>
 * Feb 25 2009
 *      Merged CMM and Trunk
 *
 * v1.42 Nenad Corbic  <ncorbic@sangoma.com>
 * Jan 26 2008
 *      Call Bearer Capability
 *	BugFix: Hangup NACK was sent out with invalid cause 0
 *
 * v1.41 Nenad Corbic  <ncorbic@sangoma.com>
 * Jan 12 2008
 * 	Fixed the NACK with cause 0 bug.
 * 	Added cause 19 on call timeout.
 *	Added configuration option call_timeout to timeout
 * 	outbound calls.
 *
 * v1.40 Nenad Corbic  <ncorbic@sangoma.com>
 * Dec 10 2008
 *	Check for Rx call overrun.
 *	In unlikely case sangoma_mgd goes out of 
 * 	sync with boost.
 *    
 * v1.39 Nenad Corbic  <ncorbic@sangoma.com>
 * Sep 17 2008
 *	Updated for Unit Test
 *	Bug fix in Loop Logic, possible race
 *	between media and woomera thread on loop end.
 *
 * v1.38 Nenad Corbic  <ncorbic@sangoma.com>
 * Sep 5 2008
 *	Added a Double use of setup id logic.
 *	Currently double use call gets dropped.
 *
 * v1.37 Nenad Corbic  <ncorbic@sangoma.com>
 * Sep 2 2008
 *	Bug fix in REMOVE LOOP logic continued
 *	The woomera->sock in loop logic was set to 0.
 * 	Closing it failed the next call.
 *
 * v1.36 Nenad Corbic  <ncorbic@sangoma.com>
 * Aug 28 2008
 *	Bug fix in REMOVE LOOP logic 
 *	Remove F from incoming calls by default
 *	Check for errno on poll()
 *	Do not delay in closing socket on media down.
 *
 * v1.35cmm Nenad Corbic  <ncorbic@sangoma.com>
 * Jul 28 2008
 * 	Bug fixes on ACK Timeout and trunk release
 *	Added a thread logger
 *	Increased max spans to 32
 *
 * v1.34cmm Nenad Corbic  <ncorbic@sangoma.com>
 * Jul 24 2008
 * 	Clean up the setup id on CALL STOP ACK
 *
 * v1.33cmm Nenad Corbic  <ncorbic@sangoma.com>
 * Jul 24 2008
 * 	The CALL STOP timeout function had a bug
 *      resulting in false blocking of tank ids 
 *      due to STOP ACK Timeouts.
 *
 * v1.33 Nenad Corbic <ncorbic@sangoma.com>
 * Jul 18 2008
 *	Added UDP Sequencing to check for dropped frames
 *	Should only be used for debugging.
 *
 * v1.32 David Yat Sin  <davidy@sangoma.com>
 * Jul 17 2008
 * 	Support for d-channel incoming digit 
 *      passthrough to Asterisk
 *      
 * v1.32cmm Nenad Corbic  <ncorbic@sangoma.com>
 * Jun 03 2008
 * 	Implemented new Restart ACK Policy
 *	Split the Event packet into 2 types
 *      	
 * v1.31cmm Nenad Corbic  <ncorbic@sangoma.com>
 * Apr 04 2008
 *	New CMM Restart procedure for boost
 *	Block TRUNK ID on ACK Timeout
 *
 * v1.31 David Yat Sin  <davidy@sangoma.com>
 * Apr 29 2008
 * 	Support for HW DTMF events.
 *
 * v1.30 Nenad Corbic  <ncorbic@sangoma.com>
 * Feb 08 2008
 *	The fix in v1.26 causes double stop.
 *      I took out the v1.26 fix and added
 *      a big warning if that condition is
 *      ever reached.
 *
 * v1.29 Nenad Corbic  <ncorbic@sangoma.com>
 * Feb 07 2008
 *	Added strip_cid_non_digits option
 *
 * v1.28 Nenad Corbic  <ncorbic@sangoma.com>
 * Feb 06 2008
 * 	Fixed a memory leak in clone event
 *      function. Added memory check subsystem  
 *
 * v1.27 Nenad Corbic  <ncorbic@sangoma.com>
 * Jan 24 2008
 *	Fixed a memory leak on incoming calls
 *	Removed the use of server listener which 
 *	was not used
 *
 * v1.26 Nenad Corbic  <ncorbic@sangoma.com>
 * Jan 18 2008
 *	Fixed hangup after invalid Answer or Ack Session 
 *	Can cause double use of setup id - now fixed
 *	Update on autoacm on accept check for acked.
 *
 * v1.25 Nenad Corbic  <ncorbic@sangoma.com>
 * Dec 31 2007
 *	Removed UDP Resync it can cause skb_over errors.
 * 	Moved RDNIS message to higher debug level
 *
 * v1.24 Nenad Corbic  <ncorbic@sangoma.com>
 * Nov 30 2007
 *	Bug fix on return code on ALL ckt busy
 *
 * v1.23 Nenad Corbic  <ncorbic@sangoma.com>
 *	Bug fix on socket open. Check for retun code >= 0
 *
 * v1.22 Nenad Corbic  <ncorbic@sangoma.com>
 * Nov 27 2007
 *	Updated DTMF Tx function
 *      Fixed - dtmf tx without voice
 *      Fxied - dtmf clipping.
 *
 * v1.21 Nenad Corbic  <ncorbic@sangoma.com>
 * Nov 25 2007
 *	Major unit testing of each state
 *      Numerous bug fixes for non autoacm mode.
 *      Changed "Channel-Name" to tg/cic
 *      Added compile option WANPIPE_CHAN_NAME to change Asterisk channel
 *      name of chan_woomera.so.  So one can use Dial(SS7/g1/${EXTE})
 *      instead of WOOMERA (for example) 
 *
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
#include "sangoma_mgd_memdbg.h"
#include "q931_cause.h"
#include "smg_capabilities.h"

int pipe_fd[2]; 

#ifdef CODEC_LAW_DEFAULT
static uint32_t codec_sample=8;
#else
static uint32_t codec_sample=16;
#endif

static char ps_progname[]="sangoma_mgd";

static struct woomera_interface woomera_dead_dev;


#ifdef BRI_PROT
static unsigned char tdmv_hwec_persist = 0;
#else
static unsigned char tdmv_hwec_persist = 1;
#endif
struct woomera_server server; 

struct smg_tdm_ip_bridge g_smg_ip_bridge_idx[MAX_SMG_BRIDGE];
pthread_mutex_t g_smg_ip_bridge_lock;

#if 0
#define DOTRACE
#endif


#define SMG_VERSION	"v1.72"

/* enable early media */
#if 1
#define WOOMERA_EARLY_MEDIA 1
#endif
  

#define SMG_DTMF_ON 	60
#define SMG_DTMF_OFF 	10
#define SMG_DTMF_RATE  	8000
#define SMG_DEFAULT_CALL_TIMEOUT 300

#define SANGOMA_USR_PERIOD 20

#if 0
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

#if 0
#define SMG_DROP_SEQ 1
#warning "SMG Debug feature Drop Seq Enabled"
static int drop_seq=0;
#else
#undef SMG_DROP_SEQ
#endif


#if 0
#define SMG_NO_MEDIA
#warning "SMG No Media Defined"
#else
#undef SMG_NO_MEDIA
#endif

const char WELCOME_TEXT[] =
"================================================================================\n"
"Sangoma Media Gateway Daemon v1.72 \n"
"\n"
"TDM Signal Media Gateway for Sangoma/Wanpipe Cards\n"
"Copyright 2005, 2006, 2007 \n"
"Nenad Corbic <ncorbic@sangoma.com>, Anthony Minessale II <anthmct@yahoo.com>\n"
"This program is free software, distributed under the terms of\n"
"the GNU General Public License\n"
"================================================================================\n"
"";

					

static int coredump=1;
static int autoacm=0;

/* FIXME: Should be done in cfg file */
#if defined(BRI_PROT)
int max_spans=WOOMERA_BRI_MAX_SPAN;
int max_chans=WOOMERA_BRI_MAX_CHAN;
#else
/* PRI_PROT uses these defines as well */
int max_spans=WOOMERA_MAX_SPAN;
int max_chans=WOOMERA_MAX_CHAN;
#endif

static int launch_media_tdm_thread(struct woomera_interface *woomera);
static int launch_woomera_thread(struct woomera_interface *woomera);
static void woomera_check_digits (struct woomera_interface *woomera); 
static struct woomera_interface *alloc_woomera(void);
static void handle_event_dtmf(struct woomera_interface *woomera, unsigned char dtmf_digit);
static int handle_event_rbs(struct woomera_interface *woomera, unsigned char rbs_digit);
static int handle_dequeue_and_woomera_tx_event_rbs(struct woomera_interface *woomera);

q931_cause_to_str_array_t q931_cause_to_str_array[255];
bearer_cap_to_str_array_t bearer_cap_to_str_array[255];
uil1p_to_str_array_t uil1p_to_str_array[255];

static int isup_exec_command(int span, int chan, int id, int cmd, int cause)
{
	short_signal_event_t oevent;
	int retry=5;
	
	call_signal_event_init((short_signal_event_t*)&oevent, cmd, chan, span);
	oevent.release_cause = cause;
	
	if (id >= 0) {
		oevent.call_setup_id = id;
	}
isup_exec_cmd_retry:
	if (call_signal_connection_write(&server.mcon, (call_signal_event_t*)&oevent) < 0){

		--retry;
		if (retry <= 0) {
			log_printf(SMG_LOG_ALL, server.log, 
			"Critical System Error: Failed to tx on ISUP socket: %s\n", 
				strerror(errno));
			return -1;
		} else {
			log_printf(SMG_LOG_ALL, server.log,
                        "System Warning: Failed to tx on ISUP socket: %s :retry %i\n",
                                strerror(errno),retry);
		}

		goto isup_exec_cmd_retry; 
	}
	
	return 0;
}

static int isup_exec_event(call_signal_event_t *event)
{
	int retry=5;
	
isup_exec_cmd_retry:
	if (call_signal_connection_write(&server.mcon, event) < 0){

		--retry;
		if (retry <= 0) {
			log_printf(SMG_LOG_ALL, server.log, 
					"Critical System Error: Failed to tx on ISUP socket: %s\n", 
					strerror(errno));
			return -1;
		} else {
			log_printf(SMG_LOG_ALL, server.log,
					"System Warning: Failed to tx on ISUP socket: %s :retry %i\n",
					strerror(errno),retry);
		}
	
		goto isup_exec_cmd_retry; 
	}
	
	return 0;
}


static int isup_exec_commandp(int span, int chan, int id, int cmd, int cause)
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
                        log_printf(SMG_LOG_ALL, server.log,
                        "Critical System Error: Failed to tx on ISUP socket: %s\n",
                                strerror(errno));
                        return -1;
                } else {
                        log_printf(SMG_LOG_ALL, server.log,
                        "System Warning: Failed to tx on ISUP socket: %s :retry %i\n",
                                strerror(errno),retry);
                }

                goto isup_exec_cmd_retry;
        }

        return 0;
}


static int get_span_chan_from_interface(char* interface, int *span_ptr, int *chan_ptr)
{
	int span, chan;
	int err;
	
	err=sscanf(interface, "s%dc%d", &span, &chan);
	if (err!=2) {
		err=sscanf(interface, "w%dg%d", &span, &chan);
	}

	if (err==2) {
		if (smg_validate_span_chan(span,chan) != 0) {
			log_printf(SMG_LOG_DEBUG_CALL, server.log, 
				"WOOMERA Warning invalid span chan in interface %s\n",
				interface);
			return -1;
		}

		*span_ptr = span;
		*chan_ptr = chan;
		return 0;
	}
	log_printf(SMG_LOG_ALL, server.log, "ERROR: Failed to get span chan from interface:[%s]\n", interface, span, chan);
	return -1;
}

static int socket_printf(int socket, char *fmt, ...)
{
    char *data;
    int ret = 0;
    va_list ap;

    if (socket < 0) {
		return -1;
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
		log_printf(SMG_LOG_ALL, server.log, "Crtical ERROR: Memory Error!\n");
    } else {
		int err;
		int len = strlen(data);
		err=send(socket, data, strlen(data), 0);
		if (err != strlen(data)) {
			log_printf(SMG_LOG_DEBUG_8, server.log, "ERROR: Failed to send data to woomera socket(%i): err=%i  len=%d %s\n",
					   socket,err,len,strerror(errno));
			ret = err;
		} else {
			ret = 0;
		}
		
		free(data);
    }

	return ret;
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
			log_printf(SMG_LOG_ALL, server.log, "Invalid syntax on %s: line %d\n", cfg->path, cfg->lineno);
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
		log_printf(SMG_LOG_ALL, server.log, "Memory Error queuing event!\n");
		return NULL;
    } else {
    		return NULL;
    }

    va_start(ap, fmt);
#ifdef SOLARIS
    event->data = (char *) smg_malloc(2048);
    vsnprintf(event->data, 2048, fmt, ap);
#else
    ret = smg_vasprintf(&event->data, fmt, ap);
#endif
    va_end(ap);
    if (ret == -1) {
		log_printf(SMG_LOG_ALL, server.log, "Memory Error queuing event!\n");
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

    /* This overwrites the MALLOC in clone causing a memory leak */	
    memcpy(clone, event, sizeof(*event));

    /* We must set the malloc flag back so that this event
     * will be deleted */
    _woomera_set_flag(clone, WFLAG_MALLOC);
    clone->next = NULL;
    clone->data = smg_strdup(event->data);

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
		log_printf(SMG_LOG_ALL, server.log, "Error Cloning Event\n");
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
    	smg_free(event->data);	
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
			smg_free(ptr);
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
	
    if ((new = smg_malloc(sizeof(*new)))) {
		memset(new, 0, sizeof(*new));
		new->woomera = woomera;
		new->next = server.listeners;
		server.listeners = new;
    } else {
		log_printf(SMG_LOG_ALL, server.log, "Memory Error adding listener!\n");
    }

    pthread_mutex_unlock(&server.listen_lock);
}

static int wanpipe_send_rbs(struct woomera_interface *woomera, char *digits)
{	
	struct media_session *ms = woomera_get_ms(woomera);
	int err;
	wanpipe_tdm_api_t tdm_api;
	unsigned int digit;

	memset(&tdm_api,0,sizeof(tdm_api));

	if (!ms) {
		log_printf(SMG_LOG_PROD, server.log, "[%s]: wanpipe_send_rbs (%X) Error no ms\n",
				woomera->interface,digit);
		return -EINVAL;
	}

	if ((woomera->span+1) <= 0 || !server.rbs_relay[woomera->span+1]) {
		log_printf(SMG_LOG_PROD, server.log, "[%s]: wanpipe_send_rbs (%X) Error rbs relay not enabled on span %i\n",
				woomera->interface,digit,woomera->span+1);
		return -EINVAL;
	}

	err=sscanf(digits,"%x",&digit);
	if (err == 1) {
		log_printf(SMG_LOG_PROD, server.log, "[%s]: Transmitting RBS 0x%X\n",
				woomera->interface,digit);

#ifdef LIBSANGOMA_VERSION
		err=sangoma_tdm_write_rbs(ms->sangoma_sock, &tdm_api, woomera->chan+1, digit);
#else
		err=sangoma_tdm_write_rbs(ms->sangoma_sock, &tdm_api, digit);
#endif
	}

	return err;
}


static int wanpipe_send_dtmf(struct woomera_interface *woomera, char *digits)
{	
	struct media_session *ms = woomera_get_ms(woomera);
	char *cur = NULL;
	int wrote = 0;
	int err;
	
	if (!ms) {
		return -EINVAL;
	}
	
	if (!ms->dtmf_buffer) {
		log_printf(SMG_LOG_DEBUG_MISC, woomera->log, "Allocate DTMF Buffer....");

		err=switch_buffer_create_dynamic(&ms->dtmf_buffer, 1024, server.dtmf_size, 0);

		if (err != 0) {
			log_printf(SMG_LOG_ALL, woomera->log, "Failed to allocate DTMF Buffer!\n");
			return -ENOMEM;
		} else {
			log_printf(SMG_LOG_DEBUG_MISC, woomera->log, "SUCCESS!\n");
		}
		
	}
	
	log_printf(SMG_LOG_DEBUG_MISC, woomera->log, "Sending DTMF %s\n",digits);
	for (cur = digits; *cur; cur++) {
		if ((wrote = teletone_mux_tones(&ms->tone_session,
					        &ms->tone_session.TONES[(int)*cur]))) {
			
			pthread_mutex_lock(&woomera->dtmf_lock);

			err=switch_buffer_write(ms->dtmf_buffer, ms->tone_session.buffer, wrote * 2);

			pthread_mutex_unlock(&woomera->dtmf_lock);

			log_printf(SMG_LOG_DEBUG_MISC, woomera->log, "Sending DTMF %s Wrote=%i (err=%i)\n",
					digits,wrote*2,err);
		} else {
			log_printf(SMG_LOG_ALL, woomera->log, "Error: Sending DTMF %s (err=%i)\n",
                                        digits,wrote);
		}
	}

	ms->skip_read_frames = 200;
	return 0;
}

static struct woomera_interface *alloc_woomera(void)
{
	struct woomera_interface *woomera = NULL;

	if ((woomera = smg_malloc(sizeof(struct woomera_interface)))) {

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
		log_printf(SMG_LOG_ALL, server.log, "Critical: Invalid Socket on new interface!\n");
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


#define SMG_DECODE_POLL_ERRORS(err) \
    (err & POLLERR) ? "POLLERR" : \
    (err & POLLHUP) ? "POLLHUP" : \
    (err & POLLNVAL) ? "POLLNVAL" : "UNKNOWN"

static int waitfor_socket(int fd, int timeout, int flags, int *flags_out)
{
    struct pollfd pfds[1];
    int res;
	int errflags = (POLLERR | POLLHUP | POLLNVAL);

waitfor_socket_tryagain:

    memset(&pfds[0], 0, sizeof(pfds[0]));

    pfds[0].fd = fd;
    pfds[0].events = flags | errflags;

    res = poll(pfds, 1, timeout);

	if (res == 0) {
		return res;
	}

    if (res > 0) {
		res=-1;
		*flags_out = pfds[0].revents;
		if (pfds[0].revents & errflags) {
			res=-1;
#if 0
			log_printf(SMG_LOG_DEBUG_10,server.log, "Wait for socket Error in revents 0x%X %s Error=%s!\n",
				pfds[0].revents,strerror(errno),SMG_DECODE_POLL_ERRORS(pfds[0].revents));
#endif
			return res;
		} else {
			if (pfds[0].revents & flags) {
				res=1;
			} else {
				log_printf(SMG_LOG_ALL,server.log, "Wait for socket invalid poll event in revents 0x%X  Error=%s Errno=%s !\n",
						pfds[0].revents,SMG_DECODE_POLL_ERRORS(pfds[0].revents),strerror(errno));
			}
		}
    } else {
		if ((errno == EINTR || errno == EAGAIN)) {
			goto waitfor_socket_tryagain;
		}
		log_printf(SMG_LOG_ALL,server.log, "Wait for socket error!\n");
	}

    return res;
}


int waitfor_2sockets(int fda, int fdb, char *a, char *b, int timeout)
{
    struct pollfd pfds[2];
    int res = 0;
    int errflags = (POLLERR | POLLHUP | POLLNVAL);

    if (fda < 0 || fdb < 0) {
		return -1;
    }


waitfor_2sockets_tryagain:

    *a=0;
    *b=0;


    memset(pfds, 0, sizeof(pfds));

    pfds[0].fd = fda;
    pfds[1].fd = fdb;
    pfds[0].events = POLLIN | errflags;
    pfds[1].events = POLLIN | errflags;

    res = poll(pfds, 2, timeout); 

    if (res > 0) {
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
    } else if (res < 0) {
	
		if (errno == EINTR || errno == EAGAIN) {
			goto waitfor_2sockets_tryagain;
		}

    }
	
    return res;
}

static int woomera_raw_to_ip(struct media_session *ms, char *raw)
{
	int x;
	char *p;
	
	for(x = 0; x < strlen(raw) ; x++) {
		if (raw[x] == ':') {
			break;
		}
		if (raw[x] == '/') {
			break;
		}
	}	
	
	media_set_raw(ms,raw);
	
	if (ms->ip) {
		smg_free(ms->ip);
	}
	ms->ip = smg_strndup(raw, x);
	p = raw + (x+1);
	ms->port = atoi(p);
	
	return 0;
}

static struct media_session *media_session_new(struct woomera_interface *woomera)
{
	struct media_session *ms = NULL;
	int span,chan;

	span=woomera->span;
	chan=woomera->chan;

	log_printf(SMG_LOG_DEBUG_CALL, server.log,"Starting new MEDIA session [%s] [%s]\n",
			woomera->interface,woomera->raw?woomera->raw:"N/A");

	if ((ms = smg_malloc(sizeof(struct media_session)))) {
		memset(ms, 0, sizeof(struct media_session));
		
		if (woomera->loop_tdm != 1) {
			
			woomera_raw_to_ip(ms,woomera->raw);
			time(&ms->started);
			
		} 
		
		time(&ms->started);
		ms->woomera = woomera;
		
		/* Setup artificial DTMF stuff */
		memset(&ms->tone_session, 0, sizeof(ms->tone_session));
		if (teletone_init_session(&ms->tone_session, 0, NULL, NULL)) {
			log_printf(SMG_LOG_ALL, server.log, "ERROR: Failed to initialize TONE [s%ic%i]!\n",
			span+1,chan+1);			
		}
	
		ms->tone_session.rate = SMG_DTMF_RATE;
		ms->tone_session.duration = server.dtmf_on * (ms->tone_session.rate / 1000);
		ms->tone_session.wait = server.dtmf_off * (ms->tone_session.rate / 1000);
	
		teletone_dtmf_detect_init (&ms->dtmf_detect, SMG_DTMF_RATE);

		woomera_set_ms(woomera,ms);
		
	} else {
		log_printf(SMG_LOG_ALL, server.log, "ERROR: Memory Alloc Failed [s%ic%i]!\n",
			span+1,chan+1);	
	}

   	return ms;
}

static void media_session_free(struct media_session *ms) 
{
    if (ms->ip) {
		smg_free(ms->ip);
    }
	if (ms->raw) {
		smg_free(ms->raw);	
	}
	
    teletone_destroy_session(&ms->tone_session);
    switch_buffer_destroy(&ms->dtmf_buffer);

    ms->woomera = NULL;

    smg_free(ms);
}

static int update_udp_socket(struct media_session *ms, char *ip, int port)
{
	struct hostent *result;
	char buf[512];
	int err = 0;
		
	memset(&ms->remote_hp, 0, sizeof(ms->remote_hp));
	gethostbyname_r(ip, &ms->remote_hp, buf, sizeof(buf), &result, &err);
	ms->remote_addr.sin_family = ms->remote_hp.h_addrtype;
	memcpy((char *) &ms->remote_addr.sin_addr.s_addr, ms->remote_hp.h_addr_list[0], ms->remote_hp.h_length);
	ms->remote_addr.sin_port = htons(port);
	
	return 0;
}

static int create_udp_socket(struct media_session *ms, char *local_ip, int local_port, char *ip, int port)
{
    int rc;
    struct hostent *result, *local_result;
    char buf[512], local_buf[512];
    int err = 0;

    log_printf(SMG_LOG_DEBUG_9,server.log,"LocalIP %s:%d IP %s:%d \n",local_ip, local_port, ip, port);

    memset(&ms->remote_hp, 0, sizeof(ms->remote_hp));
    memset(&ms->local_hp, 0, sizeof(ms->local_hp));
    if ((ms->socket = socket(AF_INET, SOCK_DGRAM, 0)) >= 0) {
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
    			
				log_printf(SMG_LOG_DEBUG_9,server.log,
					"Failed to bind LocalIP %s:%d IP %s:%d (%s)\n",
						local_ip, local_port, ip, port,strerror(errno));
			} 

			/* OK */

		} else {
    			log_printf(SMG_LOG_ALL,server.log,
				"Failed to get hostbyname LocalIP %s:%d IP %s:%d (%s)\n",
					local_ip, local_port, ip, port,strerror(errno));
		}
    } else {
    	log_printf(SMG_LOG_ALL,server.log,
		"Failed to create/allocate UDP socket\n");
    }

    return ms->socket;
}

static int next_media_port(void)
{
    int port;
    
    pthread_mutex_lock(&server.media_udp_port_lock);
    port = ++server.next_media_port;
    if (port > server.max_media_port) {
    	server.next_media_port = server.base_media_port;
		port = server.base_media_port;
    }
    pthread_mutex_unlock(&server.media_udp_port_lock);
    
    return port;
}



static int woomera_dtmf_transmit(struct media_session *ms, int mtu)
{
	struct woomera_interface *woomera = ms->woomera;
	int bread;
	unsigned char dtmf[1024];
	unsigned char dtmf_law[1024];
	sangoma_api_hdr_t hdrframe;
	int i;
	int slin_len = mtu*2;
	short *data;
	int used;
	int res;
	int err;
	int txdtmf=0;
	int flags_out;
	memset(&hdrframe,0,sizeof(hdrframe));

	if (!ms->dtmf_buffer) {
		return -1;
	}
	
	for (;;) {
		
		if ((used=switch_buffer_inuse(ms->dtmf_buffer)) <= 0) {
			break;
		}

		res = waitfor_socket(ms->sangoma_sock, -1,  POLLOUT, &flags_out);
		if (res <= 0) {
			break;
		}

#ifdef CODEC_LAW_DEFAULT

		pthread_mutex_lock(&woomera->dtmf_lock);
		if ((used=switch_buffer_inuse(ms->dtmf_buffer)) <= 0) {
			pthread_mutex_unlock(&woomera->dtmf_lock);
			break;
		}

		bread = switch_buffer_read(ms->dtmf_buffer, dtmf, slin_len);
		pthread_mutex_unlock(&woomera->dtmf_lock);

		if (bread <= 0) {
			break;
		}
		
		log_printf(SMG_LOG_DEBUG_MISC,woomera->log,"%s: Write DTMF Got %d bytes MTU=%i Coding=%i Used=%i\n",
				woomera->interface,bread,mtu,ms->hw_coding,used);
		
		data=(short*)dtmf;
		for (i=0;i<mtu;i++) {
			if (ms->hw_coding) {
				/* ALAW */
				dtmf_law[i] = linear_to_alaw((int)data[i]);
			} else {
				/* ULAW */
				dtmf_law[i] = linear_to_ulaw((int)data[i]);
			}			
		}
	
		err=sangoma_sendmsg_socket(ms->sangoma_sock,
					 &hdrframe, 
					 sizeof(hdrframe), 
					 dtmf_law, mtu, 0);
				
		if (err != mtu) {
			log_printf(SMG_LOG_ALL, woomera->log, "Error: Failed to TX to TDM API on DTMF (err=%i mtu=%i)!\n",err,mtu);
		}

		txdtmf++;	 
		ms->skip_write_frames++;
#else
...
		pthread_mutex_lock(&woomera->dtmf_lock);		
		bread = switch_buffer_read(ms->dtmf_buffer, dtmf, mtu);
		pthread_mutex_unlock(&woomera->dtmf_lock);

		log_printf(SMG_LOG_DEBUG_MISC,woomera->log,"%s: Write DTMF Got %d bytes\n",
				woomera->interface,bread);

		sangoma_sendmsg_socket(ms->sangoma_sock,
					 &hdrframe, 
					 sizeof(hdrframe), 
					 dtmf, mtu, 0);
		txdtmf++;	 
		ms->skip_write_frames++;
#endif
		
	} 

	if (txdtmf) {
		return 0;
	} else {	
		return -1;
	}
}
		
static void media_loop_run(struct media_session *ms)
{
	struct woomera_interface *woomera = ms->woomera;
	int sangoma_frame_len = 160;
	int errs=0;
	int res=0;
	wanpipe_tdm_api_t tdm_api;
	unsigned char circuit_frame[1024];
	char filename[100];
	FILE *filed=NULL;
	int loops=0,flags_out=0;
	int open_cnt = 0;

	open_cnt=0;
	
	sangoma_api_hdr_t hdrframe;
	memset(&hdrframe,0,sizeof(hdrframe));
	memset(circuit_frame,0,sizeof(circuit_frame));

retry_loop:
	ms->sangoma_sock = open_span_chan(woomera->span+1, woomera->chan+1);
	
	log_printf(SMG_LOG_PROD, server.log, "Media Loop Started %s fd=%i\n", 
			woomera->interface,ms->sangoma_sock);

	if (ms->sangoma_sock < 0) {
		errs++;
		if (errs < 5) {
			usleep(500000);
			goto retry_loop;
		}
		log_printf(SMG_LOG_ALL, server.log, "WANPIPE MEDIA Socket Error (%s) if=[%s]  [s%ic%i]\n", 
			strerror(errno), woomera->interface, woomera->span+1, woomera->chan+1);

	} else {
		errs=0;	

		if (sangoma_tdm_set_codec(ms->sangoma_sock, &tdm_api, WP_NONE) < 0 ) {
			errs++;	
		}

		if (sangoma_tdm_flush_bufs(ms->sangoma_sock, &tdm_api)) {	
			errs++;
		}
			
		if (sangoma_tdm_set_usr_period(ms->sangoma_sock, &tdm_api, SANGOMA_USR_PERIOD) < 0 ) {
			errs++;	
		}

		sangoma_frame_len = sangoma_tdm_get_usr_mtu_mru(ms->sangoma_sock,&tdm_api);

#ifdef LIBSANGOMA_VERSION
#ifdef WP_API_FEATURE_EC_CHAN_STAT  
		/* Since we are in loop mode we want to make sure that hwec is disabled
		   before we start doing the loop test. */
		ms->has_hwec = sangoma_tdm_get_hwec_chan_status(ms->sangoma_sock, &tdm_api);
#else		
		ms->has_hwec = sangoma_tdm_get_hw_ec(ms->sangoma_sock, &tdm_api);
#endif
		if (ms->has_hwec > 0) {
			sangoma_tdm_disable_hwec(ms->sangoma_sock,&tdm_api);
		} else {
         	ms->has_hwec=0;
		}
#else
		ms->has_hwec=1;
		sangoma_tdm_disable_hwec(ms->sangoma_sock,&tdm_api);
#endif
		ms->oob_disable = 0;
#ifdef LIBSANGOMA_VERSION
		open_cnt = sangoma_get_open_cnt(ms->sangoma_sock, &tdm_api);
		if (open_cnt > 1) {
			ms->oob_disable = 1;
		}
#endif
	}

	if (errs) {
		log_printf(SMG_LOG_ALL, server.log, "Media Loop: failed to open tdm device %s\n", 
					woomera->interface);
		return;
	}

	if (server.loop_trace) {
		sprintf(filename,"/smg/s%ic%i-loop.trace",woomera->span+1,woomera->chan+1);	
		unlink(filename);
		filed = safe_fopen(filename, "w");
	}
	
	while ( woomera_test_flag(&server.master_connection, WFLAG_RUNNING) &&
		!woomera_test_flag(woomera, WFLAG_MEDIA_END) && 
		((res = waitfor_socket(ms->sangoma_sock, 1000,  POLLIN, &flags_out)) >= 0)) {
		
		if (res == SMG_SOCKET_EVENT_TIMEOUT) {
			//log_printf(SMG_LOG_DEBUG_8, server.log, "%s: TDM UDP Timeout !!!\n",
			//		woomera->interface);
			/* NENAD Timeout thus just continue */
			continue; 
		}

		res = sangoma_readmsg_socket(ms->sangoma_sock, 
		                             &hdrframe, 
					     sizeof(hdrframe), 
					     circuit_frame, 
					     sizeof(circuit_frame), 0);
		if (res < 0) {
			log_printf(SMG_LOG_ALL, server.log, "TDM Loop ReadMsg Error: %s\n", 
				strerror(errno), woomera->interface);
			break;
		}

		if (server.loop_trace && filed != NULL) {
			int i;
			for (i=0;i<res;i++) {
				fprintf(filed,"%02X ", circuit_frame[i]);
				if (i && (i % 16) == 0) {
					fprintf(filed,"\n");
				}
			}
		      	fprintf(filed,"\n");
		}
			
		sangoma_sendmsg_socket(ms->sangoma_sock, 
		                             &hdrframe, 
					     sizeof(hdrframe), 
					     circuit_frame, 
					     res, 0);
		
		res=0;

		loops++;			     
	} 

	
	if (res < 0) {
		if (!woomera_test_flag(woomera, WFLAG_MEDIA_END)) {
			log_printf(SMG_LOG_ALL, server.log, "Media Loop: socket error %s (fd=%i) %s\n",
					woomera->interface, ms->sangoma_sock, strerror(errno));
		}
	}


	if (server.loop_trace && filed != NULL) {
		fclose(filed);
	}

	if (ms->has_hwec) {
		sangoma_tdm_enable_hwec(ms->sangoma_sock,&tdm_api);
	}

	close_span_chan(&ms->sangoma_sock, woomera->span+1, woomera->chan+1);

	if (loops < 1) {
		log_printf(SMG_LOG_ALL, server.log, "Media Loop FAILED %s Master=%i MediaEnd=%i Loops=%i\n",
				woomera->interface, 
				woomera_test_flag(&server.master_connection, WFLAG_RUNNING), 
				woomera_test_flag(woomera, WFLAG_MEDIA_END),loops); 
	} else {
		log_printf(SMG_LOG_PROD, server.log, "Media Loop PASSED %s Master=%i MediaEnd=%i Loops=%i\n",
				woomera->interface, 
				woomera_test_flag(&server.master_connection, WFLAG_RUNNING), 
				woomera_test_flag(woomera, WFLAG_MEDIA_END),loops); 
	}

	return;
	
}

		
		
		
#ifdef WP_HPTDM_API
static int media_rx_ready(void *p, unsigned char *data, int len)
{
	struct media_session *ms = (struct media_session *)p;
		
	if (ms->udp_sock < 0) {
		return -1;
	}
	
	return sendto(ms->udp_sock, 
	       	      data,len, 0, 
	      	      (struct sockaddr *) &ms->remote_addr, 
	       	       sizeof(ms->remote_addr));
	       
	
}
#endif

static void *media_thread_run(void *obj)
{
	struct media_session *ms = obj;
	struct woomera_interface *woomera = ms->woomera;
	int sangoma_frame_len = 160;
	int local_port, x = 0, errs = 0, res = 0, packet_len = 0;
	//int udp_cnt=0;
	struct woomera_event wevent;
	wanpipe_tdm_api_t tdm_api;
	FILE *tx_fd=NULL;
	int sock_timeout=200;

	int hwec_enabled=0, hwec_reenable=0;

	int open_cnt = 0;

	open_cnt=0;

	if (woomera_test_flag(woomera, WFLAG_MEDIA_END) || 
	    !woomera->interface ||
	    woomera_test_flag(woomera, WFLAG_HANGUP)) {
		log_printf(SMG_LOG_DEBUG_CALL, server.log, 
			"MEDIA session for [%s] Cancelled! (ptr=%p)\n", 
				woomera->interface,woomera);
		/* In this case the call will be closed via woomera_thread_run
		* function. And the process table will be cleard there */
		woomera_set_flag(woomera, WFLAG_MEDIA_END);
		woomera_clear_flag(woomera, WFLAG_MEDIA_RUNNING);
		media_session_free(ms);
		pthread_exit(NULL);
		return NULL;
	}


	log_printf(SMG_LOG_DEBUG_CALL, server.log, "MEDIA session for [%s] started (ptr=%p loop=%i)\n",
		woomera->interface,woomera,woomera->loop_tdm);

	if (woomera->loop_tdm) {   
		media_loop_run(ms);
		ms->udp_sock=-1;
		woomera_set_flag(woomera, WFLAG_HANGUP);
		woomera_clear_flag(woomera, WFLAG_MEDIA_TDM_RUNNING);
		goto media_thread_exit;	
	} 	

	for(x = 0; x < 1000 ; x++) {
		local_port = next_media_port();
		if ((ms->udp_sock = create_udp_socket(ms, server.media_ip, local_port, ms->ip, ms->port)) > -1) {
			break;
		}
	}

	/* Normal Temporary Failure */
	woomera->q931_rel_cause_topbx = 41;

	if (ms->udp_sock < 0) {
 		log_printf(SMG_LOG_ALL, server.log, "UDP Socket Error (%s) [%s] LocalPort=%d\n", 
				strerror(errno), woomera->interface, local_port);

		errs++;
	} else {
		int media_retry_cnt=0;
#ifdef WP_HPTDM_API
		hp_tdm_api_span_t *span=hptdmspan[woomera->span+1];
		if (!span || !span->init) {
			errs++;
		} else {
			hp_tdm_api_usr_callback_t usr_callback;
			memset(&usr_callback,0,sizeof(usr_callback));
			usr_callback.p = ms;
			usr_callback.rx_avail = media_rx_ready;
			if (span->open_chan(span, &usr_callback, &ms->tdmchan,woomera->chan+1)) {
				errs++;
				/* Switch Congestion */
				woomera->q931_rel_cause_topbx = 42;
			}
		}
#else
media_retry:
		ms->sangoma_sock = open_span_chan(woomera->span+1, woomera->chan+1);
		if (ms->sangoma_sock < 0) {

			if (!woomera_test_flag(woomera, WFLAG_MEDIA_END)) {
				media_retry_cnt++;
				if (media_retry_cnt < 5) {
					log_printf(SMG_LOG_ALL, server.log, "WANPIPE Socket Retry  [s%ic%i]\n",
							 woomera->span+1, woomera->chan+1);
					usleep(100000);
					goto media_retry;
				}

				log_printf(SMG_LOG_ALL, server.log, "WANPIPE Socket Error (%s) if=[%s]  [s%ic%i]\n",
					strerror(errno), woomera->interface, woomera->span+1, woomera->chan+1);

				/* Switch Congestion */
				woomera->q931_rel_cause_topbx = 42;
			}

			errs++;
		} else {
			
# ifdef CODEC_LAW_DEFAULT
			if (sangoma_tdm_set_codec(ms->sangoma_sock, &tdm_api, WP_NONE) < 0 ) {
				errs++;	
			}
# else
			if (sangoma_tdm_set_codec(ms->sangoma_sock, &tdm_api, WP_SLINEAR) < 0 ) {
				errs++;	
			}
# endif
			if (sangoma_tdm_flush_bufs(ms->sangoma_sock, &tdm_api)) {	
				errs++;
			}
			
			if (sangoma_tdm_set_usr_period(ms->sangoma_sock, &tdm_api, SANGOMA_USR_PERIOD) < 0 ) {
				errs++;	
			}

#ifdef LIBSANGOMA_VERSION
#ifdef WP_API_FEATURE_EC_CHAN_STAT  
			/* check if hwec is available */
			if (tdmv_hwec_persist) {
				/* For T1/E1 we run with ec on. If ec is off then it was meant to be off
				   thus use this as indication of whether hwec is on or off */
				ms->has_hwec = sangoma_tdm_get_hwec_chan_status(ms->sangoma_sock, &tdm_api);
			} else {
				/* On BRI we run with ec off by default. Therefore we must check
				   for ec chip being enabled not the channel */
				ms->has_hwec = sangoma_tdm_get_hw_ec(ms->sangoma_sock, &tdm_api);
			}
#else
			ms->has_hwec = sangoma_tdm_get_hw_ec(ms->sangoma_sock, &tdm_api);
#endif
			if (ms->has_hwec < 0) {
             	 ms->has_hwec=0;
			}
#else
			ms->has_hwec = 1;
#endif

# ifdef CODEC_LAW_DEFAULT			
#  ifdef LIBSANGOMA_GET_HWCODING
			ms->hw_coding=sangoma_tdm_get_hw_coding(ms->sangoma_sock, &tdm_api);
			if (ms->hw_coding < 0) {
				errs++;
			}
#  else
#   error "libsangoma missing hwcoding feature: not up to date!"
#  endif
# endif


# ifdef LIBSANGOMA_GET_HWDTMF

		if (ms->has_hwec) {
			ms->hw_dtmf=sangoma_tdm_get_hw_dtmf(ms->sangoma_sock, &tdm_api);
			if (ms->hw_dtmf) {
				log_printf(SMG_LOG_DEBUG_9, server.log, "HW DTMF Supported  [s%ic%i]\n", 
					   strerror(errno), woomera->interface, woomera->span+1, woomera->chan+1);
			} else {
				log_printf(SMG_LOG_DEBUG_9, server.log, "HW DTMF Not Supported  [s%ic%i]\n", 
					   strerror(errno), woomera->interface, woomera->span+1, woomera->chan+1);
			}
		}

#else
			ms->hw_dtmf=0;
			log_printf(SMG_LOG_DEBUG_9, server.log, "HW DTMF Not Supported  [s%ic%i]\n", 
					   strerror(errno), woomera->interface, woomera->span+1, woomera->chan+1);
# warning "libsangoma missing hwdtmf feature: not up to date!"
			
#endif

			ms->oob_disable = 0;
#ifdef LIBSANGOMA_VERSION
			open_cnt = sangoma_get_open_cnt(ms->sangoma_sock, &tdm_api);
			if (open_cnt > 1) {
				ms->oob_disable = 1;
			}
#endif


			if (ms->has_hwec) {
				if (!tdmv_hwec_persist) {
					// BRI cards start with HWEC in bypass disable state 
					if (bearer_cap_is_audio(woomera->bearer_cap)) {
						int err;
						err=sangoma_tdm_enable_hwec(ms->sangoma_sock, &tdm_api);
						if (err == 0) {
							hwec_enabled=1;
							log_printf(SMG_LOG_DEBUG_8, server.log, "MEDIA [%s] Enabling hwec Ok\n",woomera->interface);
						} else {
							log_printf(SMG_LOG_PROD, server.log, "MEDIA [%s] Enabling hwec Failed (%s)\n",woomera->interface, strerror(errno));
						}
					}
				} else {
					if (!bearer_cap_is_audio(woomera->bearer_cap)) {
						int err;
						err=sangoma_tdm_disable_hwec(ms->sangoma_sock, &tdm_api);
						if (err == 0) {
							hwec_reenable=1;
							log_printf(SMG_LOG_DEBUG_8, server.log, "MEDIA [%s] Disabling hwec Ok\n",woomera->interface);
						} else {
							log_printf(SMG_LOG_PROD, server.log, "MEDIA [%s] Disabling hwec Failed (%s)\n",woomera->interface, strerror(errno));
						}
					}
				}
			}

			sangoma_frame_len = sangoma_tdm_get_usr_mtu_mru(ms->sangoma_sock,&tdm_api);

			if (server.rbs_relay[woomera->span+1]) {
				sangoma_tdm_enable_rbs_events(ms->sangoma_sock, &tdm_api, 20);
			}
		}
#endif	 	
	}

		

#ifdef WP_HPTDM_API
	/* No tdm thread */
#else
#ifndef SMG_NO_MEDIA
	if (!errs && 
	    launch_media_tdm_thread(woomera)) {
		errs++;
	}
#endif
#endif

    if (errs) {

		woomera->q931_rel_cause_topbx = 42;
		
	} else {

		unsigned char udp_frame[4096];
		unsigned int fromlen = sizeof(struct sockaddr_in);
		int flags_out=0;
		int rc;
		unsigned int carrier=0,carrier_diff=0;

		sangoma_api_hdr_t hdrframe;
		memset(&hdrframe,0,sizeof(hdrframe));
		memset(udp_frame,0,sizeof(udp_frame));
#ifdef DOTRACE
		int fdin, fdout;
		char path_in[512], path_out[512];
#endif

		new_woomera_event_printf(&wevent, 
			"EVENT MEDIA %s AUDIO%s"
			"Unique-Call-Id: %s%s"
			"Raw-Audio: %s:%d%s"
			"Call-ID: %s%s"
			"Raw-Format: PCM-16%s"
			"DTMF: %s%s"
			,
			woomera->interface,
			WOOMERA_LINE_SEPERATOR,
			woomera->session,
			WOOMERA_LINE_SEPERATOR,
			server.media_ip,
			local_port,
			WOOMERA_LINE_SEPERATOR,
			woomera->interface,
								WOOMERA_LINE_SEPERATOR,
			WOOMERA_LINE_SEPERATOR,
				(ms->hw_dtmf)? "OutofBand": "InBand",

			WOOMERA_RECORD_SEPERATOR
			);


		enqueue_event(woomera, &wevent, EVENT_FREE_DATA);
		
#ifdef DOTRACE
		sprintf(path_in, "/tmp/debug-in.%d.raw", tc);
		sprintf(path_out, "/tmp/debug-out.%d.raw", tc++);
		fdin = open(path_in, O_WRONLY | O_CREAT, O_TRUNC, 0600);
		fdout = open(path_out, O_WRONLY | O_CREAT, O_TRUNC, 0600);
#endif

		if (server.out_tx_test) {
			tx_fd=fopen("/smg/sound.raw","rb");
			if (!tx_fd) {
				log_printf(SMG_LOG_ALL,server.log, "FAILED TO OPEN Sound file!\n");
			}	
		}

		for (;;) {
			if ((packet_len = recvfrom(ms->udp_sock, udp_frame, sizeof(udp_frame), 
				MSG_DONTWAIT, (struct sockaddr *) &ms->local_addr, &fromlen)) < 1) {
				break;
			}
		}

		
		for (;;) {

			if (!woomera_test_flag(&server.master_connection, WFLAG_RUNNING) ||
				woomera_test_flag(&server.master_connection, WFLAG_SYSTEM_RESET) || 
				woomera_test_flag(woomera, WFLAG_MEDIA_END) ||
				woomera_test_flag(woomera, WFLAG_HANGUP)) {
				res=0;
				break;
			}

			
			res = waitfor_socket(ms->udp_sock, sock_timeout, POLLIN, &flags_out);

			if (res < 0) {
				break;
			}

#ifdef SMG_NO_MEDIA
			continue;
#endif

			if (res == 0) {
				
				if (woomera_dtmf_transmit(ms,sangoma_frame_len) == 0) {
					sock_timeout=(sangoma_frame_len/codec_sample);
				} else {
					sock_timeout=200;
				}
		
				if (ms->skip_write_frames > 0) {
					ms->skip_write_frames--;
				}

				log_printf(SMG_LOG_DEBUG_8, server.log, "%s: UDP Sock Timeout !!!\n",
						woomera->interface);
				/* NENAD Timeout thus just continue */
				continue;
			}
	
			if ((packet_len = recvfrom(ms->udp_sock, udp_frame, sizeof(udp_frame), 
				MSG_DONTWAIT, (struct sockaddr *) &ms->local_addr, &fromlen)) < 1) {
				log_printf(SMG_LOG_DEBUG_CALL, server.log, "UDP Recv Error: %s\n",strerror(errno));
				break;
			}


#ifdef SMG_DROP_SEQ
			if (drop_seq) {
				log_printf(SMG_LOG_ALL,server.log,"Dropping TX Sequence! %i\n",drop_seq);
				drop_seq--;
				continue;
			}
#endif

#if 0	
			log_printf(SMG_LOG_DEBUG_10, server.log, "%s: UDP Receive %i !!!\n",
						woomera->interface,packet_len);
#endif
			
			if (packet_len > 0) {

				if (server.udp_seq && packet_len > 4) {
                    packet_len-=4;
					if ( woomera->rx_udp_seq != *(unsigned int*)&udp_frame[packet_len]) {
						 woomera->rx_udp_seq = *(unsigned int*)&udp_frame[packet_len];
						 log_printf(SMG_LOG_WOOMERA,server.log,"RX UDP SEQ=%i Expected=%i\n",
						 		*(unsigned int*)&udp_frame[packet_len],woomera->rx_udp_seq);

						pthread_mutex_lock(&server.thread_count_lock);
						server.media_rx_seq_err++;
						pthread_mutex_unlock(&server.thread_count_lock);
					} else {
                     	 woomera->rx_udp_seq++;
					}
				}     

#if 0
/* NC: This can cause skb_over panic must be retested */
				if (packet_len != sangoma_frame_len && ms->udp_sync_cnt <= 5) {
					/* Assume that we will always receive SLINEAR here */
					sangoma_tdm_set_usr_period(ms->sangoma_sock, 
								   &tdm_api, packet_len/codec_sample);
					sangoma_frame_len =
						 sangoma_tdm_get_usr_mtu_mru(ms->sangoma_sock,&tdm_api);
				
					log_printf(SMG_LOG_DEBUG_MISC, server.log, 
						"%s: UDP TDM Period ReSync to Len=%i %ims (udp=%i) \n",
						woomera->interface,sangoma_frame_len,
						sangoma_frame_len/codec_sample,packet_len);


					if (++ms->udp_sync_cnt >= 6) {
				       		sangoma_tdm_set_usr_period(ms->sangoma_sock,
				       			&tdm_api, 20);
				       		sangoma_frame_len =
				       			sangoma_tdm_get_usr_mtu_mru(ms->sangoma_sock,&tdm_api);
						log_printf(SMG_LOG_ALL, server.log, 
								"%s: UDP TDM Period Force ReSync to 20ms \n", 
								woomera->interface); 
					}
					  
				}
#endif				
				if (!server.out_tx_test) {

					if (woomera_dtmf_transmit(ms,sangoma_frame_len) == 0) {
						sock_timeout=(sangoma_frame_len/codec_sample);
						/* For sanity sake if we are doing the out test
						* dont take any chances force tx udp data */
						if (ms->skip_write_frames > 0) {
							ms->skip_write_frames--;
						}
						continue;
					} else {
						sock_timeout=200;
					}
	
					if (ms->skip_write_frames > 0) {
						ms->skip_write_frames--;
						continue;
					}

				} 		

				if (server.out_tx_test && tx_fd && 
				    fread((void*)udp_frame,
				                   sizeof(char),
					           packet_len,tx_fd) <= 0) {
						   
					sangoma_get_full_cfg(ms->sangoma_sock,&tdm_api);		   
					fclose(tx_fd);
					tx_fd=NULL;
				}


#ifdef WP_HPTDM_API
				if (ms->tdmchan->push) {
					ms->tdmchan->push(ms->tdmchan,udp_frame,packet_len);
				}
#else
	
				rc=sangoma_sendmsg_socket(ms->sangoma_sock, 
							&hdrframe, 
							sizeof(hdrframe), 
							udp_frame, 
							packet_len, 0);
                if (rc != packet_len) {
					log_printf(SMG_LOG_PROD, server.log, "Error: Sangoma Tx error [%s] len=%i  (%s)\n",woomera->interface, packet_len, strerror(errno));
				   	pthread_mutex_lock(&server.thread_count_lock);
				   	server.media_rx_seq_err++;
				   	pthread_mutex_unlock(&server.thread_count_lock);
				}
				
				sangoma_get_full_cfg(ms->sangoma_sock,&tdm_api);		   
				if (carrier == 0) {
					carrier=tdm_api.wp_tdm_cmd.stats.tx_carrier_errors;
				}
				if (carrier != tdm_api.wp_tdm_cmd.stats.tx_carrier_errors) {
					carrier_diff+=tdm_api.wp_tdm_cmd.stats.tx_carrier_errors-carrier;
					carrier=tdm_api.wp_tdm_cmd.stats.tx_carrier_errors;
					log_printf(SMG_LOG_WOOMERA, server.log, "Error: Sangoma Tx carrier error [%s] cnt=%i\n",
						woomera->interface, carrier_diff);
					pthread_mutex_lock(&server.thread_count_lock);
					server.media_rx_seq_err++;
					pthread_mutex_unlock(&server.thread_count_lock);
				
					sangoma_sendmsg_socket(ms->sangoma_sock, 
							&hdrframe, 
							sizeof(hdrframe), 
							udp_frame, 
							packet_len, 0);
				}
				
#endif

			}

#if 0					
			if (woomera->span == 1 && woomera->chan == 1) {
				udp_cnt++;
				if (udp_cnt && udp_cnt % 1000 == 0) { 
					log_printf(SMG_LOG_ALL, server.log, "%s: MEDIA UDP TX RX CNT %i %i\n",
						woomera->interface,udp_cnt,packet_len);
				}
			}
#endif			
		}
	
		if (res < 0) {
			if (!woomera_test_flag(&server.master_connection, WFLAG_RUNNING) ||
					woomera_test_flag(&server.master_connection, WFLAG_SYSTEM_RESET) || 
					woomera_test_flag(woomera, WFLAG_MEDIA_END) ||
					woomera_test_flag(woomera, WFLAG_HANGUP)) {
					res=0;
			} else {
				log_printf(SMG_LOG_ALL, server.log, "Media Thread: socket error %s SockID=%i %s Poll=%s!\n",
							woomera->interface,ms->sangoma_sock,strerror(errno),SMG_DECODE_POLL_ERRORS(flags_out));
			}
		}
	}

	new_woomera_event_printf(&wevent,
					"EVENT HANGUP %s%s"
					"Unique-Call-Id: %s%s"
					"Start-Time: %ld%s"
					"End-Time: %ld%s"
					"Answer-Time: %ld%s"
					"Call-ID: %s%s"
					"Cause: %s%s"
					"Q931-Cause-Code: %d%s",
					woomera->interface,
					WOOMERA_LINE_SEPERATOR,
				
					woomera->session,
					WOOMERA_LINE_SEPERATOR,

					time(&ms->started),
					WOOMERA_LINE_SEPERATOR,

					time(NULL),
					WOOMERA_LINE_SEPERATOR,

					time(&ms->answered),
					WOOMERA_LINE_SEPERATOR,

					woomera->interface,
					WOOMERA_LINE_SEPERATOR,
					
					q931_rel_to_str(woomera->q931_rel_cause_topbx),
					WOOMERA_LINE_SEPERATOR,

					woomera->q931_rel_cause_topbx,
					WOOMERA_RECORD_SEPERATOR
					);

    	enqueue_event(woomera, &wevent,EVENT_FREE_DATA);

	
media_thread_exit:

	if (ms->has_hwec) {

		if (!tdmv_hwec_persist) {
			if (hwec_enabled) {
				int err;
				err=sangoma_tdm_disable_hwec(ms->sangoma_sock, &tdm_api);
				if (err==0) {
					log_printf(SMG_LOG_DEBUG_8, server.log, "MEDIA [%s] disabling hwec ok\n",woomera->interface);
				} else {
					log_printf(SMG_LOG_PROD, server.log, "MEDIA [%s] disabling hwec Failed (%s)\n",woomera->interface, strerror(errno));
				}
			}
		} else {
			if (hwec_reenable) {
				int err;
				err=sangoma_tdm_enable_hwec(ms->sangoma_sock, &tdm_api);
				if (err==0) {
					log_printf(SMG_LOG_DEBUG_8, server.log, "MEDIA [%s] Re-enabling hwec ok\n",woomera->interface);
				} else {
					log_printf(SMG_LOG_PROD, server.log, "MEDIA [%s] Re-enabling hwec Failed (%s)\n",woomera->interface, strerror(errno));
				}
			}
		}

	}

 	if (woomera_test_flag(woomera, WFLAG_MEDIA_TDM_RUNNING)) {
		woomera_set_flag(woomera, WFLAG_MEDIA_END);

		/* Dont wait for the other thread */
		close_socket(&ms->udp_sock);
		close_span_chan(&ms->sangoma_sock, woomera->span+1, woomera->chan+1);		
		while(woomera_test_flag(woomera, WFLAG_MEDIA_TDM_RUNNING)) {
			usleep(1000);
			sched_yield();
		}
	}

	
	close_socket(&ms->udp_sock);
	close_span_chan(&ms->sangoma_sock, woomera->span+1, woomera->chan+1);

	if (tx_fd){
		fclose(tx_fd);
		tx_fd=NULL;
	}
	
	
	woomera_set_flag(woomera, WFLAG_MEDIA_END);
	
	woomera_set_ms(woomera,NULL);	
	woomera_clear_flag(woomera, WFLAG_MEDIA_RUNNING);

	media_session_free(ms);
	
	log_printf(SMG_LOG_DEBUG_CALL, server.log, "MEDIA session for [%s] ended (ptr=%p)\n", 
			woomera->interface,woomera);
			
	
	pthread_exit(NULL);		
	return NULL;
}




static void *media_tdm_thread_run(void *obj)
{
	wanpipe_tdm_api_t tdm_api;
	wp_tdm_api_event_t *rx_event;
	
	struct media_session *ms = obj;
	struct woomera_interface *woomera = ms->woomera;
	int res = 0;
	unsigned char circuit_frame[1024];
	sangoma_api_hdr_t hdrframe;
	int flags_out;
	int poll_opt = POLLIN | POLLPRI;

	memset(&hdrframe,0,sizeof(hdrframe));
	memset(circuit_frame,0,sizeof(circuit_frame));
    
	memset(&tdm_api, 0, sizeof(wanpipe_tdm_api_t));
	
	if (woomera_test_flag(woomera, WFLAG_MEDIA_END) || !woomera->interface) {
		log_printf(SMG_LOG_DEBUG_CALL, server.log, "MEDIA TDM session for [%s] Cancelled! (ptr=%p)\n",
			 woomera->interface,woomera);
		/* In this case the call will be closed via woomera_thread_run
		* function. And the process table will be cleard there */
		woomera_set_flag(woomera, WFLAG_MEDIA_END);
		woomera_clear_flag(woomera, WFLAG_MEDIA_TDM_RUNNING);
		pthread_exit(NULL);
		return NULL;
	}

   	log_printf(SMG_LOG_DEBUG_CALL, server.log, "MEDIA TDM session for [%s] started (ptr=%p)\n",
	 		woomera->interface,woomera);

	for (;;) {
		res = sangoma_readmsg_socket(ms->sangoma_sock,
					&hdrframe,
					sizeof(hdrframe),
					circuit_frame,
					sizeof(circuit_frame), 0);
		if (res < 0) {
			break;
		}
	}

	for (;;) {

		if (!woomera_test_flag(&server.master_connection, WFLAG_RUNNING) ||
			woomera_test_flag(&server.master_connection, WFLAG_SYSTEM_RESET) ||
			woomera_test_flag(woomera, WFLAG_MEDIA_END)) {
			res=0;
			break;
		}
	

		if (ms->oob_disable) {
			poll_opt = POLLIN;
		} else {
			poll_opt = POLLIN | POLLPRI;
		}
		res = waitfor_socket(ms->sangoma_sock, 1000, poll_opt, &flags_out);


		if (res < 0) {
			break;
		}

		if (res == 0) {
#if 0
				log_printf(SMG_LOG_DEBUG_8, server.log, "%s: TDM UDP Timeout !!!\n",
					woomera->interface);
				/* NENAD Timeout thus just continue */
#endif
			continue;
		}


		if (flags_out & POLLIN) {

			if (server.rbs_relay[woomera->span+1]) {
				handle_dequeue_and_woomera_tx_event_rbs(woomera);
			}

			res = sangoma_readmsg_socket(ms->sangoma_sock,
					&hdrframe,
					sizeof(hdrframe),
					circuit_frame,
					sizeof(circuit_frame), 0);

			if (res < 0) {
				if (!woomera_test_flag(woomera, WFLAG_MEDIA_END)) {
					log_printf(SMG_LOG_ALL, server.log, "TDM Read Data Error: %s  %s  Sockid=%i\n",
						 woomera->interface,
						 strerror(errno),ms->sangoma_sock);
				}
				break;
			}


			if (server.udp_seq) {
            		*(unsigned int*)&circuit_frame[res] = woomera->tx_udp_seq;
                    woomera->tx_udp_seq++;
                    res+=4;
			}

			res = sendto(ms->udp_sock,
					circuit_frame,
					res, 0,
					(struct sockaddr *) &ms->remote_addr,
						sizeof(ms->remote_addr));

			if (res < 0) {
				log_printf(SMG_LOG_DEBUG_CALL, server.log, "UDP Sento Error: %s\n", strerror(errno));
			}
		}

		if (flags_out & POLLPRI) {

			res = sangoma_tdm_read_event(ms->sangoma_sock, &tdm_api);
			if (res < 0) {
				log_printf(SMG_LOG_ALL, server.log, "TDM Read Event Error: %s  %s  Sockid=%i\n",
						 woomera->interface,
						 strerror(errno),ms->sangoma_sock);
				break;
			}

			rx_event = &tdm_api.wp_tdm_cmd.event;

			switch (rx_event->wp_tdm_api_event_type){
# ifdef LIBSANGOMA_GET_HWDTMF
				case WP_TDMAPI_EVENT_DTMF:

					/* Only handle hw dtmf if hw_dtmf option is enabled */
					if (!ms->hw_dtmf) {
						break;
					}

					/* PORT_SOUT = 1 */
					if (rx_event->wp_tdm_api_event_dtmf_port == WAN_EC_CHANNEL_PORT_SOUT &&
									rx_event->wp_tdm_api_event_dtmf_type == WAN_EC_TONE_PRESENT) {

						handle_event_dtmf(woomera, rx_event->wp_tdm_api_event_dtmf_digit);
					}
					break;
#endif
				case WP_TDMAPI_EVENT_RBS:
				   	log_printf(SMG_LOG_WOOMERA, server.log, "[%s] Rx RBS Event Bits=0x%02X\n",
					                               woomera->interface, rx_event->wp_tdm_api_event_rbs_bits);
					handle_event_rbs(woomera, rx_event->wp_tdm_api_event_rbs_bits);
					break;

				default:
					log_printf(SMG_LOG_ALL, server.log, "TDM API Unknown OOB Event %i\n",
									rx_event->wp_tdm_api_event_type);
					break;
			}
		}
		
#if 0
		if (woomera->span == 1 && woomera->chan == 1) {
			tdm_cnt++;
			if (tdm_cnt && tdm_cnt % 1000 == 0) { 
				log_printf(SMG_LOG_ALL, server.log, "%s: MEDIA TDM TX RX CNT %i %i\n",
					woomera->interface,tdm_cnt,res);
			}
		}
#endif

	}

	if (res < 0) {

		if (!woomera_test_flag(&server.master_connection, WFLAG_RUNNING) ||
			woomera_test_flag(&server.master_connection, WFLAG_SYSTEM_RESET) ||
			woomera_test_flag(woomera, WFLAG_MEDIA_END)) {

			/* Good reason to exit */

		} else {

			log_printf(SMG_LOG_ALL, server.log, "Media TDM Thread: socket error %s Sockid=%i %s Woomera Flags=0x%08X Poll=%s!\n",
							woomera->interface,ms->sangoma_sock,strerror(errno),woomera->flags,SMG_DECODE_POLL_ERRORS(flags_out));
			woomera_print_flags(woomera,0);
		}
	}

	log_printf(SMG_LOG_DEBUG_CALL, server.log, "MEDIA TDM session for [%s] ended (ptr=%p)\n",
		    woomera->interface,woomera);
    
	woomera_set_flag(woomera, WFLAG_MEDIA_END);
	woomera_clear_flag(woomera, WFLAG_MEDIA_TDM_RUNNING);
	
	pthread_exit(NULL);
	return NULL;
	
}


/* This function must be called with process_lock 
 * because it modifies shared process_table */

static int launch_media_thread(struct woomera_interface *woomera) 
{
    pthread_attr_t attr;
    int result = -1;
    struct media_session *ms;

    if ((ms = media_session_new(woomera))) {
		result = pthread_attr_init(&attr);
    		//pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);
		//pthread_attr_setschedpolicy(&attr, SCHED_RR);
		pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
		pthread_attr_setstacksize(&attr, MGD_STACK_SIZE);

		woomera_set_flag(woomera, WFLAG_MEDIA_RUNNING);
		result = pthread_create(&ms->thread, &attr, media_thread_run, ms);
		if (result) {
			log_printf(SMG_LOG_ALL, server.log, "%s: Error: Creating Thread! %s\n",
				 __FUNCTION__,strerror(errno));
			woomera_clear_flag(woomera, WFLAG_MEDIA_RUNNING);
			media_session_free(woomera->ms);
			
    		} 
		pthread_attr_destroy(&attr);
	
    } else {
		log_printf(SMG_LOG_ALL, server.log, "Failed to start new media session\n");
    }
    
    return result;

}

static int launch_media_tdm_thread(struct woomera_interface *woomera) 
{
	pthread_attr_t attr;
	int result = -1;
	struct media_session *ms = woomera_get_ms(woomera);

	if (!ms) {
		return result;
	}
	
	result = pthread_attr_init(&attr);
        //pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);
	//pthread_attr_setschedpolicy(&attr, SCHED_RR);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	pthread_attr_setstacksize(&attr, MGD_STACK_SIZE);

	woomera_set_flag(woomera, WFLAG_MEDIA_TDM_RUNNING);
	result = pthread_create(&ms->thread, &attr, media_tdm_thread_run, ms);
	if (result) {
		log_printf(SMG_LOG_ALL, server.log, "%s: Error: Creating Thread! %s\n",
				 __FUNCTION__,strerror(errno));
		woomera_clear_flag(woomera, WFLAG_MEDIA_TDM_RUNNING);
    	} 
	pthread_attr_destroy(&attr);

   	return result;
}


static struct woomera_interface * launch_woomera_loop_thread(short_signal_event_t *event)
{

	struct woomera_interface *woomera = NULL;
	char callid[20];
	
	sprintf(callid, "s%dc%d", event->span+1,event->chan+1);
	
	if ((woomera = alloc_woomera())) {
		
		woomera->chan = event->chan;
		woomera->span = event->span;
		woomera->log = server.log;
		woomera->debug = server.debug;
		woomera->call_id = 1;
		woomera->event_queue = NULL;
		woomera->loop_tdm=1;
		woomera->socket=-1;

	} else {
		log_printf(SMG_LOG_ALL, server.log, "Critical ERROR: memory/socket error\n");
		return NULL;
	}

	woomera_set_interface(woomera,callid);

	pthread_mutex_lock(&server.process_lock);
        server.process_table[event->span][event->chan].dev = woomera;
        pthread_mutex_unlock(&server.process_lock);
    		
	if (launch_woomera_thread(woomera)) {
		pthread_mutex_lock(&server.process_lock);
		server.process_table[event->span][event->chan].dev = NULL;
		memset(server.process_table[event->span][event->chan].session,0,SMG_SESSION_NAME_SZ);
    	pthread_mutex_unlock(&server.process_lock); 
		smg_free(woomera);
		log_printf(SMG_LOG_ALL, server.log, "Critical ERROR: memory/socket error\n");
		return NULL;
	}
	
	return woomera;
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
	int flags_out = 0;

    memset(wmsg, 0, sizeof(*wmsg));

    if (woomera->socket < 0 ) {
   	 log_printf(SMG_LOG_DEBUG_CALL, woomera->log, WOOMERA_DEBUG_PREFIX "%s Invalid Socket! %d\n", 
			 woomera->interface,woomera->socket);
		return -1;
    }

	if (woomera_test_flag(woomera, WFLAG_MEDIA_END) || 
		woomera_test_flag(woomera, WFLAG_HANGUP)) {
			log_printf(SMG_LOG_DEBUG_9, woomera->log, WOOMERA_DEBUG_PREFIX 
				"%s Woomera Message parse: Call Hangup - skipping message parse !\n", 
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
			log_printf(SMG_LOG_DEBUG_CALL, woomera->log, WOOMERA_DEBUG_PREFIX "%s Failed Sanity Check!\n[%s]\n\n", woomera->interface, buf);
			return -1;
		}

		if ((res = waitfor_socket(woomera->socket, 1000, POLLIN, &flags_out) > 0)) {

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
					log_printf(SMG_LOG_DEBUG_9, woomera->log, WOOMERA_DEBUG_PREFIX 
						"%s MEDIA END or HANGUP \n", woomera->interface);
					return -1;
				}
				ysleep(1000);
				continue;

			} else if (res < 0) {
				log_printf(SMG_LOG_DEBUG_MISC, woomera->log, WOOMERA_DEBUG_PREFIX 
					"%s error during packet retry (err=%i) Loops#%d (%s)\n", 
						woomera->interface, res, loops,
						strerror(errno));
				return res;
			} else if (loops) {
				ysleep(100000);
			}
		}
		
		gettimeofday(&ended, NULL);
		elapsed = (((ended.tv_sec * 1000) + ended.tv_usec / 1000) - ((started.tv_sec * 1000) + started.tv_usec / 1000));

		if (res < 0) {
			log_printf(SMG_LOG_DEBUG_CALL, woomera->log, WOOMERA_DEBUG_PREFIX "%s Bad RECV\n", 
				woomera->interface);
			return res;
		} else if (res == 0) {
			sanity++;
			/* Looks Like it's time to go! */
			if (!woomera_test_flag(&server.master_connection, WFLAG_RUNNING) ||
			    woomera_test_flag(woomera, WFLAG_MEDIA_END) || 
			    woomera_test_flag(woomera, WFLAG_HANGUP)) {
				log_printf(SMG_LOG_DEBUG_9, woomera->log, WOOMERA_DEBUG_PREFIX 
					"%s MEDIA END or HANGUP \n", woomera->interface);
					return -1;
			}
			ysleep(1000);
			continue;
		}

		if (packet && loops > 150) {
			log_printf(SMG_LOG_PROD, woomera->log, WOOMERA_DEBUG_PREFIX 
					"%s Timeout waiting for packet.\n", 
						woomera->interface);
			return -1;
		}

		if (timeout > 0 && (elapsed > timeout)) {
			log_printf(SMG_LOG_PROD, woomera->log, WOOMERA_DEBUG_PREFIX 
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
			log_printf(SMG_LOG_DEBUG_CALL, woomera->log, WOOMERA_DEBUG_PREFIX 
				"%s Woomera Message Parse: Server or Woomera not Running\n", woomera->interface);
			return -1;
		}
		loops++;
    }

    *eor = '\0';
    bytes = strlen(buf) + 4;
	
    memset(buf, 0, sizeof(buf));
    res = read(woomera->socket, buf, bytes);
    next = buf;

	log_printf(SMG_LOG_WOOMERA, woomera->log, "%s:WOOMERA RX MSG: %s\n",woomera->interface,buf);
	
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
	int terr;
		terr=read(woomera->socket, wmsg->body, 
		     (bytes > sizeof(wmsg->body)) ? sizeof(wmsg->body) : bytes);
    }

    return woomera_test_flag(wmsg, MFLAG_EXISTS);

}

static struct woomera_interface *pull_from_holding_tank(int index, int span , int chan)
{
	struct woomera_interface *woomera = NULL;
	
	if (index < 1 || index >= CORE_TANK_LEN) {
		if (index != 0) {
			log_printf(SMG_LOG_ALL, server.log, "%s Error on invalid TANK INDEX = %i\n",
				__FUNCTION__,index);
		}
		return NULL;
	}

	pthread_mutex_lock(&server.ht_lock);
	if (server.holding_tank[index] && 
	    server.holding_tank[index] != &woomera_dead_dev) {
			woomera = server.holding_tank[index];
	
			/* Block this index until the call is completed */
			server.holding_tank[index] = &woomera_dead_dev;
			
			woomera->index = 0;
			woomera->span=span;
			woomera->chan=chan;
	}
	pthread_mutex_unlock(&server.ht_lock);
	
	return woomera;
}

static void clear_all_holding_tank(void)
{
	int i;
	pthread_mutex_lock(&server.ht_lock);
	for (i=0;i<CORE_TANK_LEN;i++){
		server.holding_tank[i] = NULL;
	}
	pthread_mutex_unlock(&server.ht_lock);
}


static struct woomera_interface *check_tank_index(int index)
{
	struct woomera_interface *woomera = NULL;

	if (index < 1 || index >= CORE_TANK_LEN) {
		if (index != 0) {
			log_printf(SMG_LOG_ALL, server.log, "%s Error on invalid TANK INDEX = %i\n",
				__FUNCTION__,index);
		}
		return NULL;
	}

	pthread_mutex_lock(&server.ht_lock);
	woomera = server.holding_tank[index];
	pthread_mutex_unlock(&server.ht_lock);

	return woomera;
}


static void clear_from_holding_tank(int index, struct woomera_interface *woomera)
{

	if (index < 1 || index >= CORE_TANK_LEN) {
		if (index != 0) {
			log_printf(SMG_LOG_ALL, server.log, "%s Error on invalid TANK INDEX = %i\n",
				__FUNCTION__,index);
		}
		return;
	}
	
	pthread_mutex_lock(&server.ht_lock);
        if (server.holding_tank[index] == &woomera_dead_dev) {
#if 0
                log_printf(SMG_LOG_ALL,server.log, "%s Clearing DEAD id=%i OK\n",
                                        __FUNCTION__,index);
#endif
                server.holding_tank[index] = NULL;
        } else if (woomera && server.holding_tank[index] == woomera) {
#if 0
                log_printf(SMG_LOG_ALL,server.log, "%s Clearing ACTIVE Woomera id=%i OK\n",
                                        __FUNCTION__,index);
#endif
                server.holding_tank[index] = NULL;
        } else if (server.holding_tank[index]) {
                log_printf(SMG_LOG_ALL, server.log, "Critical Error: Holding tank index %i not cleared %p !\n",
                                index, server.holding_tank[index]);
        }
	pthread_mutex_unlock(&server.ht_lock);
	
	return;
}

static struct woomera_interface *peek_from_holding_tank(int index)
{
	struct woomera_interface *woomera = NULL;
	
	if (index < 1 || index >= CORE_TANK_LEN) {
		if (index != 0) {
			log_printf(SMG_LOG_ALL, server.log, "%s Error on invalid TANK INDEX = %i\n",
				__FUNCTION__,index);
		}
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
			log_printf(SMG_LOG_ALL, server.log, "\nCritical Error on TANK INDEX == 0\n");
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
		log_printf(SMG_LOG_ALL, server.log, "\nCritical Error failed to obtain a TANK INDEX\n");
		return 0;
	}

	server.holding_tank[next] = woomera;
	woomera->timeout = time(NULL) + server.call_timeout;
		
	pthread_mutex_unlock(&server.ht_lock);
	return next;
}


static int handle_event_rbs(struct woomera_interface *woomera, unsigned char rbs_digit)
{
	woomera_rbs_relay_t *rbsrelay;

	if (smg_validate_span_chan(woomera->span,woomera->chan) != 0) {
		log_printf(SMG_LOG_ALL, server.log, "[%s] handle_event_rbs invalid span chan\n",
			woomera->interface);
		return -1;
	}

	if (!server.rbs_relay[woomera->span+1]) {
		log_printf(SMG_LOG_ALL, server.log, "[%s] handle_event_rbs rbs_relan not enabled\n",
			woomera->interface);
		return -1;	
	}
	
	rbsrelay=&server.process_table[woomera->span][woomera->chan].rbs_relay;

	if (rbsrelay->rbs_bits[rbsrelay->rx_idx].init) {
		log_printf(SMG_LOG_ALL, server.log, "[%s] Critical Error Rx RBS Overrun\n",
				  	woomera->interface);
		return -1;
	}

	log_printf(SMG_LOG_ALL, server.log, "[%s] woomera rx queue rbs %X idx %i\n",
			woomera->interface,rbs_digit, rbsrelay->rx_idx);

	rbsrelay->rbs_bits[rbsrelay->rx_idx].abcd=rbs_digit;
	rbsrelay->rbs_bits[rbsrelay->rx_idx].init=500/SANGOMA_USR_PERIOD;
	rbsrelay->rx_idx = ((rbsrelay->rx_idx + 1) % WOOMERA_MAX_RBS_BITS);
	
	return 0;
}

static int handle_dequeue_and_woomera_tx_event_rbs(struct woomera_interface *woomera)
{
	struct woomera_event wevent;
	woomera_rbs_relay_t *rbsrelay;
	unsigned char abcd;

	if (smg_validate_span_chan(woomera->span,woomera->chan) != 0) {
		log_printf(SMG_LOG_ALL, server.log, "[%s] handle_dequeue_and_woomera_tx_event_rbs invalid span chan\n",
			woomera->interface);
		return -1;
	}

	if (!server.rbs_relay[woomera->span+1]) {
		log_printf(SMG_LOG_ALL, server.log, "[%s] handle_dequeue_and_woomera_tx_event_rbs rbs_relan not enabled\n",
			woomera->interface);
		return -1;	
	}
	
	if (!woomera_test_flag(woomera,WFLAG_ANSWER)) {
		return 0;
	}
	
	rbsrelay=&server.process_table[woomera->span][woomera->chan].rbs_relay;

	if (rbsrelay->rbs_bits[rbsrelay->tx_idx].init == 0) {
		return -1;
	}

	if (rbsrelay->rbs_bits[rbsrelay->tx_idx].init > 1) {
		rbsrelay->rbs_bits[rbsrelay->tx_idx].init--;
		return -1;
	}

	
	abcd = rbsrelay->rbs_bits[rbsrelay->tx_idx].abcd;
	rbsrelay->rbs_bits[rbsrelay->tx_idx].init=0;
	

	log_printf(SMG_LOG_ALL, server.log, "[%s] woomera tx rbs %X idx %i\n",
			woomera->interface,abcd, rbsrelay->tx_idx);

	rbsrelay->tx_idx = ((rbsrelay->tx_idx + 1) % WOOMERA_MAX_RBS_BITS);

	memset(&wevent, 0, sizeof(struct woomera_event));
	
	new_woomera_event_printf(&wevent, "EVENT RBS %sUnique-Call-Id:%s%sContent-Length:%d%s%s%X%s",
							  WOOMERA_LINE_SEPERATOR,
							  woomera->session,
							  WOOMERA_LINE_SEPERATOR,
							  1,
							  WOOMERA_LINE_SEPERATOR,
							  WOOMERA_LINE_SEPERATOR,
							  abcd,
							  WOOMERA_RECORD_SEPERATOR);			
	

	enqueue_event(woomera, &wevent,EVENT_FREE_DATA);
	
	return 0;
}
	
static void handle_event_dtmf(struct woomera_interface *woomera, unsigned char dtmf_digit)
{
	struct woomera_event wevent;
	
	memset(&wevent, 0, sizeof(struct woomera_event));
	
	new_woomera_event_printf(&wevent, "EVENT DTMF %sUnique-Call-Id:%s%sContent-Length:%d%s%s%c%s",
					  	WOOMERA_LINE_SEPERATOR,
       						woomera->session,
       						WOOMERA_LINE_SEPERATOR,
       						1,
	      					WOOMERA_LINE_SEPERATOR,
       						WOOMERA_LINE_SEPERATOR,
       						dtmf_digit,
       						WOOMERA_RECORD_SEPERATOR);			
	

	enqueue_event(woomera, &wevent,EVENT_FREE_DATA);		
	return;
}
 
static int handle_woomera_progress(struct woomera_interface *woomera, 
								   struct woomera_message *wmsg)
{ 
	call_signal_event_t event;
	int err=-1;
	
	memset(&event, 0, sizeof(event));

	call_signal_event_init((short_signal_event_t*)&event, SIGBOOST_EVENT_CALL_PROGRESS, woomera->chan, woomera->span);
	sprintf(event.isup_in_rdnis,"SMG003-EVI-2");
	event.isup_in_rdnis_size=strlen(event.isup_in_rdnis);
	if (woomera->index >= 0) {
		event.call_setup_id = woomera->index;
	}
	
	log_printf(SMG_LOG_WOOMERA, woomera->log, "WOOMERA CMD: %s [%s]\n",
				wmsg->command, woomera->interface);				
		
	if (!woomera_check_running(woomera)) {
		socket_printf(woomera->socket, "405 PROGRESS Channel already hungup%s"
				"Unique-Call-Id: %s%s",
				WOOMERA_LINE_SEPERATOR, 
				woomera->session,
				WOOMERA_RECORD_SEPERATOR);
		return -1;
	}
			
	if (!woomera_test_flag(woomera,WFLAG_CALL_ACKED)) {
		
		socket_printf(woomera->socket, "405 PROGRESS Channel not aceked%s"
				"Unique-Call-Id: %s%s",
				WOOMERA_LINE_SEPERATOR, 
				woomera->session,
				WOOMERA_RECORD_SEPERATOR);
		return -1;
	}
	
	err=isup_exec_event(&event);
	if (err == 0) {
		socket_printf(woomera->socket, 
					"200 %s PROGRESS OK%s"
							"Unique-Call-Id: %s%s", 
					wmsg->callid, 
					WOOMERA_LINE_SEPERATOR,
					woomera->session,
					WOOMERA_RECORD_SEPERATOR);	
	} else {
		socket_printf(woomera->socket, "405 PROGRESS Boost failure%s"
				"Unique-Call-Id: %s%s",
				WOOMERA_LINE_SEPERATOR, 
				woomera->session,
				WOOMERA_RECORD_SEPERATOR);
	}
	
	return err;
}

static int handle_woomera_media_accept_answer(struct woomera_interface *woomera, 
				      struct woomera_message *wmsg, 
				      int media, int answer, int accept)
{
	char *raw = woomera_message_header(wmsg, "raw-audio");
	char *media_available = woomera_message_header(wmsg, "xMedia");
	char *ring_available = woomera_message_header(wmsg, "xRing");
	struct woomera_event wevent;
	
	log_printf(SMG_LOG_WOOMERA, woomera->log, "WOOMERA CMD: %s [%s] raw=%s\n",
			   wmsg->command, woomera->interface, raw?raw:"N/A");	
		
	if (!woomera_check_running(woomera)) {
	
		log_printf(SMG_LOG_DEBUG_CALL, server.log,
			"ERROR! call was cancelled MEDIA on HANGUP or MEDIA END!\n");
			
		new_woomera_event_printf(&wevent, "EVENT HANGUP %s%s"
								"Unique-Call-Id: %s%s"
								"Cause: %s%s"
								"Q931-Cause-Code: %d%s",
								wmsg->callid,
								WOOMERA_LINE_SEPERATOR,
								woomera->session,
								WOOMERA_LINE_SEPERATOR,
							 	q931_rel_to_str(woomera->q931_rel_cause_topbx),
								WOOMERA_LINE_SEPERATOR,
							 	woomera->q931_rel_cause_topbx,
								WOOMERA_RECORD_SEPERATOR
								);
			
		enqueue_event(woomera, &wevent,EVENT_FREE_DATA);
				
		new_woomera_event_printf(&wevent, "501 call already hungup!%s"
										"Unique-Call-Id: %s%s",
										WOOMERA_LINE_SEPERATOR,
										woomera->session,
										WOOMERA_RECORD_SEPERATOR);
		
		enqueue_event(woomera, &wevent,EVENT_FREE_DATA);

		woomera->timeout=0;
		return -1;
	} 
		
	log_printf(SMG_LOG_DEBUG_MISC, server.log,"WOOMERA: GOT %s EVENT: [%s]  RAW=%s\n",
			wmsg->command,wmsg->callid,raw);

			
	if (raw &&
	    woomera->raw == NULL && 
	    !woomera_test_flag(woomera, WFLAG_RAW_MEDIA_STARTED)) {
		
		woomera_set_flag(woomera, WFLAG_RAW_MEDIA_STARTED);
		
		woomera_set_raw(woomera, raw);
				
		if (launch_media_thread(woomera)) {
			
			log_printf(SMG_LOG_DEBUG_8, server.log,"ERROR: Failed to Launch Call [%s]\n",
				woomera->interface);

			
			new_woomera_event_printf(&wevent, "EVENT HANGUP %s%s"
								"Unique-Call-Id: %s%s"
								"Cause: %s%s"
								"Q931-Cause-Code: %d%s",
								wmsg->callid,
								WOOMERA_LINE_SEPERATOR,
								woomera->session,
								WOOMERA_LINE_SEPERATOR,
								q931_rel_to_str(21),
								WOOMERA_LINE_SEPERATOR,
								21,
								WOOMERA_RECORD_SEPERATOR
								);
			
			enqueue_event(woomera, &wevent,EVENT_FREE_DATA);
				
			new_woomera_event_printf(&wevent, "501 call was cancelled!%s"
										"Unique-Call-Id: %s%s",
										WOOMERA_LINE_SEPERATOR,
										woomera->session,
										WOOMERA_RECORD_SEPERATOR);
			
			enqueue_event(woomera, &wevent,EVENT_FREE_DATA);
			
			woomera_set_flag(woomera, WFLAG_MEDIA_END);
			woomera_clear_flag(woomera, WFLAG_RUNNING);
			
			woomera->timeout=0;
			return -1;
		}
	} else if (media && raw && woomera->ms) {
		if (strncmp(raw,woomera->raw,20) != 0) {
			log_printf(SMG_LOG_WOOMERA, server.log, "[%s] MEDIA Address Change from %s to %s\n",
					   woomera->interface,woomera->raw, raw); 
			
			pthread_mutex_lock(&woomera->ms_lock);
			woomera_set_raw(woomera,raw);
			woomera_raw_to_ip(woomera->ms,woomera->raw);		
			update_udp_socket(woomera->ms,woomera->ms->ip, woomera->ms->port);
	
			pthread_mutex_unlock(&woomera->ms_lock);
		}
	}

	if (!woomera_test_flag(&server.master_connection, WFLAG_MONITOR_RUNNING)) {
		log_printf(SMG_LOG_ALL, server.log,"ERROR! Monitor Thread not running!\n");
		new_woomera_event_printf(&wevent, "501 call was cancelled!%s"
				"Unique-Call-Id: %s%s",
				WOOMERA_LINE_SEPERATOR,
				woomera->session,
				WOOMERA_RECORD_SEPERATOR);
	
		enqueue_event(woomera, &wevent,EVENT_FREE_DATA);
		return -1;
		
	} else {

		if (accept) {
			if (!autoacm && !woomera_test_flag(woomera,WFLAG_CALL_ACKED)) {
				short_signal_event_t event;
				memset(&event, 0, sizeof(event));
				call_signal_event_init(&event, SIGBOOST_EVENT_CALL_START_ACK, woomera->chan, woomera->span);
				if (media_available && (atoi(media_available) == 1)) {
					event.flags |= SIGBOOST_PROGRESS_MEDIA;
				}

				if (ring_available && (atoi(ring_available) == 1)) {
					event.flags |= SIGBOOST_PROGRESS_RING;
				}
				woomera_set_flag(woomera,WFLAG_CALL_ACKED);

				isup_exec_event((call_signal_event_t*)&event);
			}
		}

		if (answer) {
			struct media_session *ms;
			
			pthread_mutex_lock(&woomera->ms_lock);
			if ((ms=woomera->ms)) {
				time(&woomera->ms->answered);
			}
			pthread_mutex_unlock(&woomera->ms_lock);
			
			if (ms) {

				if (!autoacm && !woomera_test_flag(woomera,WFLAG_CALL_ACKED)) {
					short_signal_event_t event;
					memset(&event, 0, sizeof(event));
					call_signal_event_init(&event, SIGBOOST_EVENT_CALL_START_ACK, woomera->chan, woomera->span);
					if (media_available && (atoi(media_available) == 1)) {
						event.flags |= SIGBOOST_PROGRESS_MEDIA;
					}

					if (ring_available && (atoi(ring_available) == 1)) {
						event.flags |= SIGBOOST_PROGRESS_RING;
					}
					woomera_set_flag(woomera,WFLAG_CALL_ACKED);
					
					isup_exec_event((call_signal_event_t*)&event);
				}

				woomera_set_flag(woomera, WFLAG_ANSWER);
				
				isup_exec_command(woomera->span, 
						  woomera->chan, 
						  -1,
						  SIGBOOST_EVENT_CALL_ANSWERED,
						  0);
				log_printf(SMG_LOG_DEBUG_CALL, server.log,
				"Sent SIGBOOST_EVENT_CALL_ANSWERED [s%dc%d]\n",
					woomera->span+1,woomera->chan+1);
			} else {
				
				struct woomera_event wevent;
				log_printf(SMG_LOG_ALL, server.log,
				  "WOOMERA ANSWER: FAILED [%s] no Media \n",
					wmsg->command,wmsg->callid);	


				new_woomera_event_printf(&wevent, "EVENT HANGUP %s%s"
								  "Unique-Call-Id: %s%s"
							  	  "Cause: %s%s"
								  "Q931-Cause-Code: %d%s",
								wmsg->callid,
								WOOMERA_LINE_SEPERATOR,
								woomera->session,
								WOOMERA_LINE_SEPERATOR,
								q931_rel_to_str(21),
								WOOMERA_LINE_SEPERATOR,
								21,
								WOOMERA_RECORD_SEPERATOR
								);
			
				enqueue_event(woomera, &wevent,EVENT_FREE_DATA);
					
	    		new_woomera_event_printf(&wevent, "501 call was cancelled!%s"
						"Unique-Call-Id: %s%s",
						WOOMERA_LINE_SEPERATOR,
						woomera->session,
						WOOMERA_RECORD_SEPERATOR);
			
				enqueue_event(woomera, &wevent,EVENT_FREE_DATA);
				
				woomera_set_flag(woomera, WFLAG_MEDIA_END);
				woomera_clear_flag(woomera, WFLAG_RUNNING);
				woomera->timeout=0;
				return -1;
			}
		}
	} 

	new_woomera_event_printf(&wevent, "200 %s OK%s"
					"Unique-Call-Id: %s%s", 
			answer ? "ANSWER" : 
			accept ? "ACCEPT" : "MEDIA", 
			WOOMERA_LINE_SEPERATOR,
			woomera->session,
			WOOMERA_RECORD_SEPERATOR);

	enqueue_event(woomera, &wevent,EVENT_FREE_DATA);

	return 0;
}

static int handle_woomera_call_start (struct woomera_interface *woomera,
                                     struct woomera_message *wmsg)
{
	char *raw = woomera_message_header(wmsg, "raw-audio");
	call_signal_event_t event;
	char *calling = woomera_message_header(wmsg, "local-number");
#ifdef SMG_CALLING_NAME
	char *calling_name = woomera_message_header(wmsg, "local-name");
#endif
	char *presentation = woomera_message_header(wmsg, "Presentation");
	char *screening = woomera_message_header(wmsg, "Screening");
	char *rdnis = woomera_message_header(wmsg, "RDNIS");
	char *bearer_cap = woomera_message_header(wmsg, "Bearer-Cap");
	char *uil1p = woomera_message_header(wmsg, "uil1p");
	char *custom_data = woomera_message_header(wmsg, "xCustom");

	char *called = wmsg->callid;
	char *grp = wmsg->callid;
	char *profile;
	char *p;
	int cause = 34;
	int tg = 0;
	int huntgroup = SIGBOOST_HUNTGRP_SEQ_ASC;

	/* Remove profile name out of destiantion */
	if ((profile = strchr(called, ':'))) {
		profile++; 
		called=profile;
		grp=profile;
	}

	log_printf(SMG_LOG_DEBUG_CALL, woomera->log, "New Call %d/%d  origdest=%s  newdest=%s\n",
		 server.call_count, server.max_calls, wmsg->callid, called);

	switch(called[0]) {
		case 'g':
			huntgroup = SIGBOOST_HUNTGRP_SEQ_ASC;
			break;
		case 'G':
			huntgroup = SIGBOOST_HUNTGRP_SEQ_DESC;
			break;
		case 'r':
			huntgroup = SIGBOOST_HUNTGRP_RR_ASC;
			break;
		case 'R':
			huntgroup = SIGBOOST_HUNTGRP_RR_DESC;
			break;
		default:
			log_printf(SMG_LOG_DEBUG_CALL, woomera->log, 
					"Warning: Failed to determine huntgroup (%s)\n",
							called);
			huntgroup = SIGBOOST_HUNTGRP_SEQ_ASC;
	}

	if ((p = strchr(called, '/'))) {
		*p = '\0';
		called = p+1;
		tg = atoi(grp+1) - 1;
		if (tg < 0) {
			tg=0;
		}
	}

	woomera->trunk_group=tg;

	if ( woomera->trunk_group > SMG_MAX_TG ) {

		socket_printf(woomera->socket, "EVENT HANGUP %s"
										"Cause: %s%s"
										"Q931-Cause-Code: %d%s",
										WOOMERA_LINE_SEPERATOR,
										q931_rel_to_str(cause),
										WOOMERA_LINE_SEPERATOR,
										cause,
										WOOMERA_RECORD_SEPERATOR);

		socket_printf(woomera->socket, 
			"405 SMG Server: trunk group value not valid!%s", WOOMERA_RECORD_SEPERATOR);

		log_printf(SMG_LOG_DEBUG_MISC, woomera->log, "SMG Server: trunk group value not valid %d\n", 
			woomera->trunk_group);

		return -1;
	}

	if (smg_check_all_busy(woomera->trunk_group) || 
	    woomera_test_flag(&server.master_connection, WFLAG_SYSTEM_RESET)){


		socket_printf(woomera->socket, "EVENT HANGUP %s"
										"Cause: %s%s"
										"Q931-Cause-Code: %d%s",
										WOOMERA_LINE_SEPERATOR,
										q931_rel_to_str(cause),
										WOOMERA_LINE_SEPERATOR,
										cause,
										WOOMERA_RECORD_SEPERATOR);

		socket_printf(woomera->socket, 
			"405 SMG Server All Ckt Busy!%s", WOOMERA_RECORD_SEPERATOR);

		log_printf(SMG_LOG_DEBUG_MISC, woomera->log, "SMG Server Full %d (ckt busy cnt=%i on tg=%d)\n", 
			server.call_count, server.all_ckt_busy[woomera->trunk_group],woomera->trunk_group);
		return -1;
	}



	if (raw) {
		woomera_set_raw(woomera, raw);
	}

	woomera->index =  add_to_holding_tank(woomera);
	if (woomera->index < 1) {
		 socket_printf(woomera->socket, "EVENT HANGUP %s"
										"Cause: %s%s"
										"Q931-Cause-Code: %d%s",
										WOOMERA_LINE_SEPERATOR,
										q931_rel_to_str(cause),
										WOOMERA_LINE_SEPERATOR,
										cause,
										WOOMERA_RECORD_SEPERATOR);
		socket_printf(woomera->socket, 
				"405 SMG Server All Tanks Busy!%s", 
				WOOMERA_RECORD_SEPERATOR);
		log_printf(SMG_LOG_ALL, woomera->log, "Error: Call Tank Full (Call Cnt=%i)\n", 
			server.call_count);
		return -1;				
	} 


	woomera->index_hold = woomera->index;
	
	call_signal_call_init(&event, calling, called, woomera->index);

	if (presentation) {
		event.calling_number_presentation = atoi(presentation);
	} else {
		event.calling_number_presentation = 0;
	}

	if (screening) {
		event.calling_number_screening_ind = atoi(screening);
	} else {
		event.calling_number_screening_ind = 0;
	}

	event.isup_in_rdnis_size=0;
	event.isup_in_rdnis[0]=0;

	if (rdnis && strlen(rdnis) ) {
		if (strlen(rdnis) > sizeof(event.rdnis.digits)){ 
			log_printf(SMG_LOG_ALL,server.log,"Error: RDNIS Overflow (in size=%i max=%i)\n",
					strlen(rdnis), sizeof(event.rdnis.digits));
			
		} else {
			strncpy(event.rdnis.digits,rdnis, sizeof(event.rdnis.digits)-1);
			event.rdnis.digits_count=strlen(rdnis)+1;
			log_printf(SMG_LOG_DEBUG_MISC,server.log,"RDNIS %s\n", rdnis);
		}
	}

	if (custom_data && strlen(custom_data)) {
		if (strlen(custom_data) > sizeof(event.custom_data)){
			log_printf(SMG_LOG_ALL,server.log,"Error: CUSTOM Overflow (in size=%i max=%i)\n",
					strlen(custom_data), sizeof(event.custom_data));
		} else {
			strncpy(event.custom_data, custom_data, sizeof(event.custom_data)-1);
			event.custom_data_size=strlen(custom_data)+1;
			log_printf(SMG_LOG_DEBUG_MISC, server.log, "CUSTOM %s\n", custom_data);
		}
	}

	if (bearer_cap && strlen(bearer_cap)) {
		event.bearer.capability = bearer_cap_to_code(bearer_cap);
		woomera->bearer_cap = event.bearer.capability;
	}

	if (uil1p && strlen(uil1p)) {
		event.bearer.uil1p = uil1p_to_code(uil1p);
	}
	
#ifdef SMG_CALLING_NAME
	if (calling_name) {
		strncpy((char*)event.calling_name,calling_name,
				sizeof(event.calling_name)-1);
	}
#endif

	event.trunk_group = tg;
	event.hunt_group = huntgroup;

	set_digits_info(&event.calling.ton, woomera_message_header(wmsg, "xCallingTon"));
	set_digits_info(&event.calling.npi, woomera_message_header(wmsg, "xCallingNpi"));
	set_digits_info(&event.called.ton,  woomera_message_header(wmsg, "xCalledTon"));
	set_digits_info(&event.called.npi, woomera_message_header(wmsg, "xCalledNpi"));
	set_digits_info(&event.rdnis.ton, woomera_message_header(wmsg, "xRdnisTon"));
	set_digits_info(&event.rdnis.npi, woomera_message_header(wmsg, "xRdnisNpi"));

	if (call_signal_connection_write(&server.mcon, &event) < 0) {
		log_printf(SMG_LOG_ALL, server.log, 
		"Critical System Error: Failed to tx on ISUP socket [%s]: %s\n", 
			strerror(errno));

		 socket_printf(woomera->socket, "EVENT HANGUP %s"
                                                  "Cause: %s%s"
                                                  "Q931-Cause-Code: %d%s",
                                                 WOOMERA_LINE_SEPERATOR,
                                                 q931_rel_to_str(cause),
                                                 WOOMERA_LINE_SEPERATOR,
                                                 cause,
                                                 WOOMERA_RECORD_SEPERATOR);

		socket_printf(woomera->socket, 
				"405 SMG Signalling Contestion!%s", 
				WOOMERA_RECORD_SEPERATOR);
		return -1;
	}

	socket_printf(woomera->socket, "100 Trying%s", WOOMERA_RECORD_SEPERATOR);

	log_printf(SMG_LOG_DEBUG_CALL, server.log, "Call Called Event [Setup ID: %d] TG=%d\n",
					woomera->index,tg);
	
	return 0;
}


static void interpret_command(struct woomera_interface *woomera, struct woomera_message *wmsg)
{
    	int answer = 0, media = 0, accept=0;
	char *unique_id;
	int cause=0;



	if (!strcasecmp(wmsg->command, "call")) {
		int err;

		if (strlen(woomera->session) != 0) {
			/* Call has already been placed */
			socket_printf(woomera->socket, "400 Error Call already in progress %s",
						WOOMERA_RECORD_SEPERATOR);
			log_printf(SMG_LOG_ALL,server.log,"Woomera RX Call Even while call in progress!\n");
			woomera_set_flag(woomera, WFLAG_HANGUP);
			return;
		}
		
		if (woomera_test_flag(&server.master_connection, WFLAG_SYSTEM_RESET)){
			socket_printf(woomera->socket, "EVENT HANGUP %s"
                                                  "Cause: %s%s"
                                                  "Q931-Cause-Code: %d%s",
                                                 WOOMERA_LINE_SEPERATOR,
                                                 q931_rel_to_str(34),
                                                 WOOMERA_LINE_SEPERATOR,
                                                 34,
                                                 WOOMERA_RECORD_SEPERATOR);
	                socket_printf(woomera->socket,
        	                "405 SMG Server All Ckt Reset!%s", WOOMERA_RECORD_SEPERATOR);

			woomera_set_flag(woomera, WFLAG_HANGUP);
			return;
		} 
	
		err=handle_woomera_call_start(woomera,wmsg);
		if (err) {
			woomera_set_flag(woomera, WFLAG_HANGUP);
		}

		return;

	} else if (!strcasecmp(wmsg->command, "bye") || !strcasecmp(wmsg->command, "quit")) {
		char *cause = woomera_message_header(wmsg, "cause");
		char *q931cause = woomera_message_header(wmsg, "Q931-Cause-Code");
		
		if (cause) {
			log_printf(SMG_LOG_DEBUG_MISC, woomera->log, "Bye Cause Received: [%s]\n", cause);
		}
		if (q931cause && atoi(q931cause)) {
			woomera_set_cause_tosig(woomera,atoi(q931cause));
		}

		log_printf(SMG_LOG_WOOMERA, woomera->log, "WOOMERA CMD: Bye Received: [%s]\n", woomera->interface);
		
		woomera_clear_flag(woomera, WFLAG_RUNNING);
		socket_printf(woomera->socket, "200 Connection closed%s"
						"Unique-Call-Id: %s%s",
						WOOMERA_LINE_SEPERATOR, 
						woomera->session,
						WOOMERA_RECORD_SEPERATOR);
		return;				
	
	} else if (!strcasecmp(wmsg->command, "listen")) {

		if (woomera_test_flag(woomera, WFLAG_LISTENING)) {
			socket_printf(woomera->socket, "405 Listener already started%s"
							"Unique-Call-Id: %s%s",
							WOOMERA_LINE_SEPERATOR, 
							woomera->session,
					 		WOOMERA_RECORD_SEPERATOR);
		} else {
		
			woomera_set_flag(woomera, WFLAG_LISTENING);
			add_listener(woomera);
			log_printf(SMG_LOG_ALL,woomera->log, "Starting Listen Device!\n");

			socket_printf(woomera->socket, "%s", 
					WOOMERA_RECORD_SEPERATOR);
			
			socket_printf(woomera->socket, "200 Listener enabled%s"
						       "Unique-Call-Id: %s%s",
							WOOMERA_LINE_SEPERATOR, 
							woomera->session,
							WOOMERA_RECORD_SEPERATOR);
		}
		return;

	} else if ((media = !strcasecmp(wmsg->command, "debug"))) {

		int debug_level=atoi(wmsg->callid);

		if (debug_level < 10) {
			server.debug=debug_level;
			log_printf(SMG_LOG_ALL,server.log,"SMG Debugging set to %i (window=%i)\n",server.debug,server.mcon.txwindow);
		}

		return;
	}

	if (!strcasecmp(wmsg->command, "hangup")) {
		char *q931cause = woomera_message_header(wmsg, "Q931-Cause-Code");


		if (q931cause && atoi(q931cause)) {
			log_printf(SMG_LOG_DEBUG_8,server.log,"Woomera Hangup setting cause to %s %i\n",
							q931cause,atoi(q931cause));
							woomera_set_cause_tosig(woomera,atoi(q931cause));
		}

		/* Continue Through */
	}



	unique_id = woomera_message_header(wmsg, "Unique-Call-Id");	
    	if (!unique_id) {

		cause=111;
                socket_printf(woomera->socket, "EVENT HANGUP %s"
                                                  "Cause: %s%s"
                                                  "Q931-Cause-Code: %d%s",
                                                 WOOMERA_LINE_SEPERATOR,
                                                 q931_rel_to_str(cause),
                                                 WOOMERA_LINE_SEPERATOR,
                                                 cause,
                                                 WOOMERA_RECORD_SEPERATOR);
		socket_printf(woomera->socket, "400 Woomera cmd without uniquie id%s",
						WOOMERA_RECORD_SEPERATOR);

		log_printf(SMG_LOG_DEBUG_CALL,server.log,"Woomera RX Event (%s) without unique id!\n",wmsg->command);
		woomera_set_flag(woomera, WFLAG_HANGUP);
		return;
	}

	if (strlen(woomera->session) == 0) {
		struct woomera_interface *session_woomera=NULL;
		char *session=NULL;
		int span, chan;
		char ifname[100];
		int err;

		/* If session does not exist this is an incoming call */
		err=sscanf(unique_id, "s%dc%d",  &span, &chan);
		if (err!=2) {
			err=sscanf(unique_id, "w%dg%d",  &span, &chan);
		}
		span--;
		chan--;
	
		log_printf(SMG_LOG_DEBUG_MISC, woomera->log, 
			"WOOMERA Got CMD %s Span=%d Chan=%d from session %s\n", 	
				wmsg->command,span,chan,unique_id);

		if (smg_validate_span_chan(span,chan) != 0) {

			cause=81;
            socket_printf(woomera->socket, "EVENT HANGUP %s"
                                                  "Cause: %s%s"
                                                  "Q931-Cause-Code: %d%s",
                                                 WOOMERA_LINE_SEPERATOR,
                                                 q931_rel_to_str(cause),
                                                 WOOMERA_LINE_SEPERATOR,
                                                 cause,
                                                 WOOMERA_RECORD_SEPERATOR);
			socket_printf(woomera->socket, "404 Invalid span/chan in session%s",
						WOOMERA_RECORD_SEPERATOR);

			log_printf(SMG_LOG_DEBUG_CALL, woomera->log, 
				"WOOMERA Warning invalid span chan in session %s %s\n",
				wmsg->command,unique_id);
			woomera_set_flag(woomera, WFLAG_HANGUP);
			return;
		}

		pthread_mutex_lock(&server.process_lock);
		session = server.process_table[span][chan].session;
		session_woomera = server.process_table[span][chan].dev;

		/* This scenario is very common when we have multile clients
		   where multiple clients race get the incoming call */
		if (session_woomera) {
			pthread_mutex_unlock(&server.process_lock);

			cause=81;
            socket_printf(woomera->socket, "EVENT HANGUP %s"
                                                  "Cause: %s%s"
                                                  "Q931-Cause-Code: %d%s",
                                                 WOOMERA_LINE_SEPERATOR,
                                                 q931_rel_to_str(cause),
                                                 WOOMERA_LINE_SEPERATOR,
                                                 cause,
                                                 WOOMERA_RECORD_SEPERATOR);
			socket_printf(woomera->socket, "404 Session not found%s",
						WOOMERA_RECORD_SEPERATOR);

			
			log_printf(SMG_LOG_DEBUG_8, woomera->log, "WOOMERA Error channel in use %s %s\n",
				wmsg->command,unique_id);
			woomera_set_flag(woomera, WFLAG_HANGUP);
			return;
		}

		if (!session || strlen(session) == 0 ||
		    strncmp(session,unique_id,sizeof(woomera->session))){
			pthread_mutex_unlock(&server.process_lock);

			/* Invalid call reference */
			cause=81;
			socket_printf(woomera->socket, "event hangup %s"
                                                  "cause: %s%s"
                                                  "q931-cause-code: %d%s",
                                                 WOOMERA_LINE_SEPERATOR,
                                                 q931_rel_to_str(cause),
                                                 WOOMERA_LINE_SEPERATOR,
                                                 cause,
                                                 WOOMERA_RECORD_SEPERATOR);

			socket_printf(woomera->socket, "404 Invalid/Expired Session%s",
						WOOMERA_RECORD_SEPERATOR);

			log_printf(SMG_LOG_DEBUG_MISC, woomera->log, 
				"WOOMERA Warning: Cmd=%s with invalid session %s (orig=%s)\n", 	
				wmsg->command,unique_id,session?session:"N/A");
			woomera_set_flag(woomera, WFLAG_HANGUP);
			return;
		}

#if 1
		if (!strcasecmp(wmsg->command, "hangup")) {
			int clients=server.process_table[span][chan].clients;
			if (server.process_table[span][chan].clients < 0) {

				log_printf(SMG_LOG_ALL, woomera->log, "WOOMERA CMD: Warning Clients (%i) Race condition on Hangup  [s%dc%d]\n",
						clients, span+1,chan+1);

			} else if (server.process_table[span][chan].clients > 1) {
				server.process_table[span][chan].clients--;
				pthread_mutex_unlock(&server.process_lock);
			
				log_printf(SMG_LOG_DEBUG_MISC, woomera->log, "WOOMERA CMD: Got Hungup on Multiple Clients %i, skipping hangup [s%dc%d]\n",
						clients, span+1,chan+1);

				cause=16;
				socket_printf(woomera->socket, "event hangup %s"
                                                  "cause: %s%s"
                                                  "q931-cause-code: %d%s",
                                                 WOOMERA_LINE_SEPERATOR,
                                                 q931_rel_to_str(cause),
                                                 WOOMERA_LINE_SEPERATOR,
                                                 cause,
                                                 WOOMERA_RECORD_SEPERATOR);

				socket_printf(woomera->socket, "404 Hangup on multiple session%s",
						WOOMERA_RECORD_SEPERATOR);

				woomera_set_flag(woomera, WFLAG_HANGUP);
                return;
			}
			log_printf(SMG_LOG_DEBUG_MISC, woomera->log, "WOOMERA CMD: Hanging up channel [s%dc%d]\n",
						span+1,chan+1);
		}
#endif

		server.process_table[span][chan].dev=woomera;
		strncpy(woomera->session,unique_id,sizeof(woomera->session));
		sprintf(ifname,"s%dc%d",span+1,chan+1);
		woomera_set_interface(woomera, ifname);

		woomera->span=span;
		woomera->chan=chan;

		/* Save bearer cap that came in on incoming call event */
		woomera->bearer_cap = server.process_table[span][chan].bearer_cap;

		/* set it to 1 so that queued digits are checked on proceed message */
		woomera->check_digits = 1;
		
		pthread_mutex_unlock(&server.process_lock);
		
		log_printf(SMG_LOG_DEBUG_MISC, woomera->log, "WOOMERA Got New If=%s Session %s\n", 	
			woomera->interface, woomera->session);
	
	
	} else if (strncmp(woomera->session,unique_id,sizeof(woomera->session))) {

		cause=81;
		socket_printf(woomera->socket, "EVENT HANGUP %s"
										"Cause: %s%s"
										"Q931-Cause-Code: %d%s",
										WOOMERA_LINE_SEPERATOR,
										q931_rel_to_str(cause),
										WOOMERA_LINE_SEPERATOR,
										cause,
										WOOMERA_RECORD_SEPERATOR);

		socket_printf(woomera->socket, "404 Session Mis-match%s",
						WOOMERA_RECORD_SEPERATOR);
		woomera_set_flag(woomera, WFLAG_HANGUP);
		return;
	}

	if (!strcasecmp(wmsg->command, "rbs")) {
		
		log_printf(SMG_LOG_PROD, server.log,
				"WOOMERA CMD: RBS Received: [%s]  Digit %s Body %s\n",
					woomera->interface, wmsg->command_args, wmsg->body);
	
		wanpipe_send_rbs(woomera,wmsg->body);
	
		socket_printf(woomera->socket, "200 RBS OK%s"
						"Unique-Call-Id: %s%s",
						WOOMERA_LINE_SEPERATOR, 
						woomera->session,
						WOOMERA_RECORD_SEPERATOR);

	} else if (!strcasecmp(wmsg->command, "dtmf")) {
		
		log_printf(SMG_LOG_WOOMERA, woomera->log, 
				"WOOMERA CMD: DTMF Received: [%s]  Digit %s Body %s\n", 	
					woomera->interface, wmsg->command_args, wmsg->body);
	
		wanpipe_send_dtmf(woomera,wmsg->body);
	
		socket_printf(woomera->socket, "200 DTMF OK%s"
						"Unique-Call-Id: %s%s",
						WOOMERA_LINE_SEPERATOR, 
						woomera->session,
						WOOMERA_RECORD_SEPERATOR);

		
    	} else if (!strcasecmp(wmsg->command, "hangup")) {

		int chan = -1, span = -1;
		char *cause = woomera_message_header(wmsg, "cause");
		char *q931cause = woomera_message_header(wmsg, "Q931-Cause-Code");

		if (q931cause && atoi(q931cause)) {
			woomera_set_cause_tosig(woomera,atoi(q931cause));
		}
	
		span=woomera->span;
		chan=woomera->chan;
			
		log_printf(SMG_LOG_DEBUG_MISC, woomera->log, "WOOMERA CMD: Hangup Received: [%s] MEDIA EXIST Cause=%s\n",
				 	woomera->interface,cause);
		
		if (smg_validate_span_chan(span,chan) != 0) {
			
			socket_printf(woomera->socket, "405 No Such Channel%s"
							"Unique-Call-Id: %s%s",
							WOOMERA_LINE_SEPERATOR, 
							woomera->session,
							WOOMERA_RECORD_SEPERATOR);
			return;
		}

	
		log_printf(SMG_LOG_DEBUG_CALL, woomera->log, "Hangup Received: [s%dc%d]\n", 	
				span+1,chan+1);	

		
		woomera_set_flag(woomera, WFLAG_MEDIA_END);
		woomera_clear_flag(woomera, WFLAG_RUNNING);

			
		socket_printf(woomera->socket, "200 HANGUP OK%s"
						"Unique-Call-Id: %s%s",
						WOOMERA_LINE_SEPERATOR, 
						woomera->session,
						WOOMERA_RECORD_SEPERATOR);
		
     } else if (!strcasecmp(wmsg->command, "proceed")) {
		
		log_printf(SMG_LOG_WOOMERA, woomera->log, "WOOMERA CMD: %s [%s]\n",
			wmsg->command, woomera->interface);
		
		socket_printf(woomera->socket, 
				"200 %s PROCEED OK%s"
				"Unique-Call-Id: %s%s", 
				wmsg->callid, 
				WOOMERA_LINE_SEPERATOR,
				woomera->session,
				WOOMERA_RECORD_SEPERATOR);
		
	 } else if (!strcasecmp(wmsg->command, "progress")) {
			
		handle_woomera_progress(woomera,wmsg);

		 
    } else if ((media = !strcasecmp(wmsg->command, "media")) || 
               (answer = !strcasecmp(wmsg->command, "answer")) ||
	       (accept = !strcasecmp(wmsg->command, "accept"))) {
	       
		handle_woomera_media_accept_answer(woomera, wmsg, media,answer,accept);
    

    } else {
	    log_printf(SMG_LOG_ALL, server.log,"WOOMERA INVALID EVENT:  %s  [%s] \n",
					wmsg->command,wmsg->callid);
		socket_printf(woomera->socket, "501 Command '%s' not implemented%s", 
				wmsg->command, WOOMERA_RECORD_SEPERATOR);
    }
}


/*
  EVENT INCOMING 1
  Remote-Address: 10.3.3.104
  Remote-Number:
  Remote-Name: Anthony Minessale!8668630501
  Protocol: H.323
  User-Agent: Post Increment Woomera		1.0alpha1 (OpenH323 v1.17.2)	9/61
  H323-Call-Id: 887b1ff8-bb1f-da11-85c0-0007e98988c4
  Local-Number: 996
  Start-Time: Fri, 09 Sep 2005 12:25:14 -0400
  Local-Name: root
*/


static void handle_call_answer(short_signal_event_t *event)
{
		struct woomera_interface *woomera = NULL;
		int kill = 0;

		if (smg_validate_span_chan(event->span,event->chan)) {
			log_printf(0,server.log, "Error: invalid span=% chan=%i on call answer [s%dc%d]!\n",
				event->span+1, event->chan+1, event->span+1,event->chan+1);
			return;
		}
		
		pthread_mutex_lock(&server.process_lock);
		woomera = server.process_table[event->span][event->chan].dev;
		pthread_mutex_unlock(&server.process_lock);	
    
    	if (woomera) {
		char callid[80];
		struct woomera_event wevent;
			
		woomera->timeout = 0;

		if (!woomera->raw) {
			log_printf(SMG_LOG_PROD, server.log, "Refusing to answer call with no media!\n");
			kill++;
			goto handle_call_answer_end;
		}

		if (woomera_test_flag(woomera, WFLAG_ANSWER)) {
			log_printf(SMG_LOG_PROD, server.log, "Refusing to double-answer a call!\n");
			kill++;
			goto handle_call_answer_end;
		}

		woomera_set_flag(woomera, WFLAG_ANSWER);

		if (woomera->span != event->span || woomera->chan != event->chan) {
			log_printf(SMG_LOG_PROD, server.log, "Refusing to start media on a different channel from the one we agreed on.!\n");
			kill++;
			goto handle_call_answer_end;
		}

		if (woomera_test_flag(woomera, WFLAG_HANGUP)) {
			log_printf(SMG_LOG_PROD, server.log, "Refusing to answer a dead call!\n");
			kill++;
		} else {
			int err;
			err=0;
			sprintf(callid, "s%dc%d", event->span + 1, event->chan + 1);
			woomera_set_interface(woomera, callid);
#ifndef WOOMERA_EARLY_MEDIA
			err=launch_media_thread(woomera);
			if (err) {
				log_printf(SMG_LOG_ALL, server.log,"ERROR: Failed to Launch Call [%s]\n",
					woomera->interface);

							 
				new_woomera_event_printf(&wevent, "EVENT HANGUP %s%s"
									"Unique-Call-Id: %s%s"
									"Q931-Cause-Code: %d%s"
									"Cause: %s%s",
						 	 woomera->interface,
							 WOOMERA_LINE_SEPERATOR,
							 woomera->session,
							 WOOMERA_LINE_SEPERATOR,
							 21,
							 WOOMERA_LINE_SEPERATOR,
							 q931_rel_to_str(21),
							 WOOMERA_RECORD_SEPERATOR
							 );
					
				enqueue_event(woomera, &wevent,EVENT_FREE_DATA);
				
				new_woomera_event_printf(&wevent, "501 call was cancelled!%s"
						"Unique-Call-Id: %s%s",
						WOOMERA_LINE_SEPERATOR,
						woomera->session,
						WOOMERA_RECORD_SEPERATOR);
			
				enqueue_event(woomera, &wevent,EVENT_FREE_DATA);
				
				kill++;
			} 
#endif

			if (!kill) {
				new_woomera_event_printf(&wevent, "EVENT CONNECT s%dc%d%s"
								  "Unique-Call-Id: %s%s",
									 event->span+1,
									 event->chan+1,
									 WOOMERA_LINE_SEPERATOR,
									 woomera->session,
									 WOOMERA_RECORD_SEPERATOR
									 );
				enqueue_event(woomera, &wevent,EVENT_FREE_DATA);
			}
		}
        } else {
		log_printf(SMG_LOG_PROD, server.log, "Answer requested on non-existant session. [s%dc%d]\n",
					event->span+1, event->chan+1);
		kill++;
	}

handle_call_answer_end:

	if (kill) {
		if (woomera) {
			woomera_set_flag(woomera,WFLAG_MEDIA_END);
		} else {

/* This can casuse a double STOP 
   must be debugged further */
#if 0
			isup_exec_command(event->span, 
					event->chan, 
					-1,
					SIGBOOST_EVENT_CALL_STOPPED,
					SIGBOOST_RELEASE_CAUSE_NORMAL);
									
			log_printf(SMG_LOG_PROD, server.log, "Sent CALL STOP to Answer without session [s%dc%d]\n", 
					event->span+1, event->chan+1);
#endif
			log_printf(SMG_LOG_ALL, server.log, "WARNING: Received Answer with no session [s%dc%d]\n", 
					event->span+1, event->chan+1);
		}
	}
}

static void handle_call_start_ack(short_signal_event_t *event)
{
    	struct woomera_interface *woomera = NULL;
    	struct woomera_event wevent;
	int kill = 0;

   	if ((woomera = peek_from_holding_tank(event->call_setup_id))) {
		char callid[80];
			
		if (woomera_test_flag(woomera, WFLAG_HANGUP)) {
			log_printf(SMG_LOG_PROD, server.log, "Refusing to ack a dead call!\n");
			kill++;
		} else {
			int err, span, chan;
			
			pull_from_holding_tank(event->call_setup_id,event->span,event->chan);
			sprintf(callid, "s%dc%d", event->span + 1, event->chan + 1);
			
			span = event->span;
			chan = event->chan;

			woomera_set_flag(woomera,WFLAG_CALL_ACKED);
			woomera_set_interface(woomera, callid);

			pthread_mutex_lock(&server.process_lock);

			if (server.process_table[span][chan].dev) {
    			struct woomera_interface *tmp_woomera = server.process_table[span][chan].dev;
		 		woomera_set_flag(tmp_woomera,WFLAG_HANGUP);
				woomera_set_flag(tmp_woomera,WFLAG_MEDIA_END);
				log_printf(SMG_LOG_ALL,server.log,"Call Overrun on [s%dc%d] - Call ACK!\n", event->span+1, event->chan+1);
				kill++;
			} 

			server.process_table[span][chan].dev = woomera;
			sprintf(woomera->session,"%s-%i-%i",callid,rand(),rand());
			sprintf(server.process_table[span][chan].session,"%s-%s",
							callid,woomera->session);
			
			
			memset(server.process_table[span][chan].digits, 0, 
			       sizeof(server.process_table[span][chan].digits));
			
			server.process_table[span][chan].digits_len = 0;
			memset(&server.process_table[event->span][event->chan].rbs_relay,0,
					sizeof(server.process_table[event->span][event->chan].rbs_relay));
			pthread_mutex_unlock(&server.process_lock);
			
			if (kill) {
				goto woomera_call_ack_skip;
			}	
			
#ifdef WOOMERA_EARLY_MEDIA
			err=launch_media_thread(woomera);
			if (err) {
				log_printf(SMG_LOG_ALL, server.log,"ERROR: Failed to Launch Call [%s]\n",
					woomera->interface);
					
	 
				new_woomera_event_printf(&wevent, "EVENT HANGUP %s%s"
								  "Unique-Call-Id:%s%s"
							      "Q931-Cause-Code: %d%s"
								  "Cause: %s%s",
								 woomera->interface,
								 WOOMERA_LINE_SEPERATOR,
								 woomera->session,
								 WOOMERA_LINE_SEPERATOR,
								 21,
								 WOOMERA_LINE_SEPERATOR,
								 q931_rel_to_str(21),
								 WOOMERA_RECORD_SEPERATOR
								 );
					
				enqueue_event(woomera, &wevent,EVENT_FREE_DATA);
				
				new_woomera_event_printf(&wevent, "501 call was cancelled!%s"
						"Unique-Call-Id: %s%s",
						WOOMERA_LINE_SEPERATOR,
						woomera->session,
						WOOMERA_RECORD_SEPERATOR);
			
				enqueue_event(woomera, &wevent,EVENT_FREE_DATA);
				
			
				kill++;
			}
#endif
			if (!kill) {
				
				new_woomera_event_printf(&wevent, "EVENT PROCEED s%dc%d%s"
								  "Channel-Name: g%d/%d%s"
								  "Unique-Call-Id: %s%s",
								event->span+1,
								event->chan+1,
								WOOMERA_LINE_SEPERATOR,
								woomera->trunk_group+1,
								(event->span*max_chans)+event->chan+1,
								WOOMERA_LINE_SEPERATOR,
								woomera->session,
								WOOMERA_RECORD_SEPERATOR
								);
								
				enqueue_event(woomera, &wevent,EVENT_FREE_DATA);
				
				new_woomera_event_printf(&wevent, "201 Accepted%s"
						"Unique-Call-Id: %s%s",
						WOOMERA_LINE_SEPERATOR,
						woomera->session,
						WOOMERA_RECORD_SEPERATOR);

				enqueue_event(woomera, &wevent,EVENT_FREE_DATA);
	
				log_printf(SMG_LOG_DEBUG_MISC, server.log, "Call Answered Event ID = %d  Device = s%dc%d!\n",
						event->call_setup_id,woomera->span+1,woomera->chan+1);
			}
		}
	} else {
		log_printf(SMG_LOG_PROD, server.log, 
			"Event (START ACK) %d referrs to a non-existant session (%d) [s%dc%d]!\n",
				event->event_id, event->call_setup_id,event->span+1, event->chan+1);
		kill++;
	}

woomera_call_ack_skip:
	if (kill) {
		if (woomera) {
			woomera_set_flag(woomera,WFLAG_MEDIA_END);
		} else {
			isup_exec_command(event->span, 
					event->chan, 
					-1,
					SIGBOOST_EVENT_CALL_STOPPED,
					SIGBOOST_RELEASE_CAUSE_NORMAL);
							
			log_printf(SMG_LOG_PROD, server.log, "Sent CALL STOP to CALL START ACK without session [s%dc%d]\n", 
					event->span+1, event->chan+1);
		}
	}
}

static void handle_call_start_nack(short_signal_event_t *event)
{
   	struct woomera_interface *woomera = NULL;
	int span=-1, chan=-1;
	int ack=0;
	
	/* Always ACK the incoming NACK 
	 * Send out the NACK ACK before pulling the TANK, because
	 * if we send after the pull, the outgoing call could send
	 * a message to boost with the pulled TANK value before
	 * we send a NACK ACK */
	 
	if (smg_validate_span_chan(event->span,event->chan) == 0) {
		span=event->span;
		chan=event->chan;
	}

	if (event->call_setup_id > 0) {
		woomera=peek_from_holding_tank(event->call_setup_id);
	}
	
	if (woomera) {
	
		struct woomera_event wevent;

		if (woomera_test_flag(woomera, WFLAG_HANGUP)) {
			log_printf(SMG_LOG_ALL, server.log, "Event CALL START NACK on hungup call [%d]!\n",
				event->call_setup_id);
			ack++;
		} else {

			woomera_set_cause_topbx(woomera,event->release_cause);	
			woomera_set_flag(woomera, (WFLAG_HANGUP|WFLAG_HANGUP_NACK_ACK)); 

			new_woomera_event_printf(&wevent, "EVENT HANGUP s%dc%d%s"
							  "Unique-Call-Id: %s%s"
							  "Cause: %s%s"
							  "Q931-Cause-Code: %d%s", 
							 event->span+1,
							 event->chan+1,
							 WOOMERA_LINE_SEPERATOR,
							 woomera->session,
							 WOOMERA_LINE_SEPERATOR,
							 q931_rel_to_str(woomera->q931_rel_cause_topbx),
							 WOOMERA_LINE_SEPERATOR,
							 woomera->q931_rel_cause_topbx,
							 WOOMERA_RECORD_SEPERATOR
								 );
			enqueue_event(woomera, &wevent,EVENT_FREE_DATA);
			
			new_woomera_event_printf(&wevent, "501 call was cancelled!%s"
					"Unique-Call-Id: %s%s",
					WOOMERA_LINE_SEPERATOR,
					woomera->session,
					WOOMERA_RECORD_SEPERATOR);
			
			enqueue_event(woomera, &wevent,EVENT_FREE_DATA);
			
			woomera_set_flag(woomera, WFLAG_HANGUP);
			woomera_clear_flag(woomera, WFLAG_RUNNING);
			
			/* Do not ack here, let woomera thread ack it */
			ack=0;
		}
	
		
	} else if (event->call_setup_id == 0 && 
		   smg_validate_span_chan(event->span,event->chan) == 0) {
	
		pthread_mutex_lock(&server.process_lock);
		woomera = server.process_table[event->span][event->chan].dev;

		if (woomera) {
			if (!woomera_test_flag(woomera, WFLAG_WAIT_FOR_NACK_ACK_SENT) && 
		            !woomera_test_flag(woomera, WFLAG_WAIT_FOR_STOPPED_ACK_SENT)) {
				/* Only if we are not already waiting for hangup */
				server.process_table[event->span][event->chan].dev=NULL;
			} else if (woomera_test_flag(woomera, WFLAG_HANGUP)) {
				/* At this point call is already hang up */
				woomera=NULL;
			} else {
				/* At this point call is already hang up */
				woomera=NULL;
			}
		}

		memset(server.process_table[event->span][event->chan].session,
			0,SMG_SESSION_NAME_SZ);			
		pthread_mutex_unlock(&server.process_lock);

		
		if (woomera) {
			
			log_printf(SMG_LOG_DEBUG_CALL, server.log, "Event START NACK on s%dc%d ptr=%p ms=%p\n",
					woomera->span+1,woomera->chan+1,woomera,woomera->ms);

			if (!woomera_test_flag(woomera,WFLAG_HANGUP)){
				if (event->release_cause == SIGBOOST_CALL_SETUP_CSUPID_DBL_USE ||
				    event->release_cause == SIGBOOST_CALL_SETUP_NACK_ALL_CKTS_BUSY) {
					woomera_set_cause_topbx(woomera,17);
				} else {
					woomera_set_cause_topbx(woomera,event->release_cause);
				}
			}

			woomera_set_flag(woomera, 
				(WFLAG_HANGUP|WFLAG_MEDIA_END|WFLAG_HANGUP_NACK_ACK)); 

			/* Nack Ack will be sent by the woomera thread */
			ack=0;		
		} else {
			/* Valid state when we are not in autoacm mode */
			ack++;
			log_printf(SMG_LOG_DEBUG_MISC, server.log, "Event: NACK no woomera on span chan [s%dc%d]!\n",
				event->span+1, event->chan+1);
		}
  				
	} else {
		log_printf(SMG_LOG_ALL, server.log, 
			"Error: Start Nack Invalid State Should not happen [%d] [s%dc%d]!\n",
				event->call_setup_id, event->span+1, event->chan+1);
		ack++;
	}

	if (event->release_cause == SIGBOOST_CALL_SETUP_NACK_ALL_CKTS_BUSY) {
		smg_all_ckt_busy(woomera->trunk_group);
		log_printf(SMG_LOG_ALL, server.log, "WARNING: All ckt busy Timeout=%i on tg=%i!\n",
											 server.all_ckt_busy[woomera->trunk_group], woomera->trunk_group);	
	} 
	if (event->release_cause == SIGBOOST_CALL_SETUP_CSUPID_DBL_USE) {
		log_printf(SMG_LOG_ALL, server.log, "WARNING: Double use on [s%ic%i] setup id %i!\n",
			event->span+1,event->chan+1,event->call_setup_id);	
	}

#warning "Ignoring CALL GAP"
#if 0
	if (event->release_cause == SIGBOOST_CALL_SETUP_NACK_AUTO_CALL_GAP) {
		log_printf(SMG_LOG_ALL, server.log, "WARNING: Call Gapping Detected!\n");	
		smg_all_ckt_gap(woomera->trunk_group);
 	} 
#endif
	
	if (ack) {
		span=0;
		chan=0;
		if (smg_validate_span_chan(event->span,event->chan) == 0) {
			span=event->span;
			chan=event->chan;	
		}

		isup_exec_command(span,
				  chan,
				  event->call_setup_id,
				  SIGBOOST_EVENT_CALL_START_NACK_ACK,
				  0);
		
		if (!woomera) {	
		log_printf(SMG_LOG_DEBUG_CALL, server.log, "Event (NACK ACK) %d referrs to a non-existant session (%d) [s%dc%d]!\n",
				event->event_id,event->call_setup_id, event->span+1, event->chan+1);
		}
	}
}

static void handle_call_loop_start(short_signal_event_t *event)
{
	struct woomera_interface *woomera;
    	char callid[20];
	char *session;
	
	pthread_mutex_lock(&server.process_lock);
    	if (server.process_table[event->span][event->chan].dev) {

		isup_exec_command(event->span, 
				  event->chan, 
				  -1,
				  SIGBOOST_EVENT_CALL_START_NACK,
				  17);

	
		log_printf(SMG_LOG_PROD, server.log, 
			"Sent (From Handle Loop START) Call Busy SIGBOOST_EVENT_CALL_START_NACK  [s%dc%d] ptr=%d\n", 
				event->span+1, event->chan+1, server.process_table[event->span][event->chan].dev);
	
		pthread_mutex_unlock(&server.process_lock);
		return;
				
    	}	

 	sprintf(callid, "s%dc%d", event->span+1,event->chan+1);
        sprintf(server.process_table[event->span][event->chan].session,
                                         "%s-%i-%i",callid,rand(),rand());
        session=server.process_table[event->span][event->chan].session;
        server.process_table[event->span][event->chan].dev = NULL;
        pthread_mutex_unlock(&server.process_lock);


	woomera=launch_woomera_loop_thread(event);
	if (woomera == NULL) {
		isup_exec_command(event->span, 
			  event->chan, 
			  -1,
			  SIGBOOST_EVENT_CALL_START_NACK,
			  17);
		log_printf(SMG_LOG_PROD, server.log, 
		"Sent (From Handle Loop START) Call Busy SIGBOOST_EVENT_CALL_START_NACK  [s%dc%d] ptr=%d\n", 
			event->span+1, event->chan+1, server.process_table[event->span][event->chan].dev);
	}
		
	woomera_set_flag(woomera,WFLAG_CALL_ACKED);

	return;
}


static void handle_call_start(call_signal_event_t *event)
{
	struct woomera_event wevent;
	char callid[20];
	char *session;
	struct woomera_interface *tmp_woomera=NULL;
	int clients;

	remove_end_of_digits_char((unsigned char*)event->called.digits);
	remove_end_of_digits_char((unsigned char*)event->calling.digits);
	remove_end_of_digits_char((unsigned char*)event->rdnis.digits);

	if (server.strip_cid_non_digits) {	
		validate_number((unsigned char*)event->called.digits);
		validate_number((unsigned char*)event->calling.digits);
		validate_number((unsigned char*)event->rdnis.digits);
	}

	if (smg_validate_span_chan(event->span,event->chan)) {
		log_printf(0,server.log,
			"Error: invalid span=% chan=%i on incoming call [s%dc%d] - Call START!\n", 
				event->span+1, event->chan+1, event->span+1,event->chan+1);
		return;	
	}

	pthread_mutex_lock(&server.process_lock);

	if ((tmp_woomera=server.process_table[event->span][event->chan].dev)) {

		woomera_set_flag(tmp_woomera,WFLAG_HANGUP);
		woomera_set_flag(tmp_woomera,WFLAG_MEDIA_END);
		log_printf(SMG_LOG_ALL,server.log,"Call Overrun on [s%dc%d] - Call START!\n", event->span+1, event->chan+1);

		isup_exec_command(event->span,
				event->chan,
				-1,
				SIGBOOST_EVENT_CALL_START_NACK,
				17);

		log_printf(SMG_LOG_ALL, server.log,
			"Sent (From Handle START) Call Busy SIGBOOST_EVENT_CALL_START_NACK  [s%dc%d]\n",
				event->span+1, event->chan+1);

		pthread_mutex_unlock(&server.process_lock);
		return;
	}
	
	sprintf(callid, "s%dc%d", event->span+1,event->chan+1);
	sprintf(server.process_table[event->span][event->chan].session,
					 "%s-%i-%i",callid,rand(),rand()); 
	session=server.process_table[event->span][event->chan].session;
	server.process_table[event->span][event->chan].dev = NULL;
	memset(server.process_table[event->span][event->chan].digits, 0, sizeof(server.process_table[event->span][event->chan].digits));
	server.process_table[event->span][event->chan].digits_len = 0;
	server.process_table[event->span][event->chan].bearer_cap = event->bearer.capability;
    server.process_table[event->span][event->chan].clients=0;
 	memset(&server.process_table[event->span][event->chan].rbs_relay,0,
			sizeof(server.process_table[event->span][event->chan].rbs_relay));
	pthread_mutex_unlock(&server.process_lock);
    	
	if (autoacm) {
		isup_exec_command(event->span, 
				event->chan, 
				-1,
				SIGBOOST_EVENT_CALL_START_ACK,
				0);
	}	
    

	log_printf(SMG_LOG_DEBUG_8,server.log,"BEARER %i , UIL1P = %i\n",
		event->bearer.capability,event->bearer.uil1p);

	if (event->bearer.uil1p == 0) {
	 	if (server.hw_coding) {
			event->bearer.uil1p=BC_IE_UIL1P_G711_ALAW;
		} else {
			event->bearer.uil1p=BC_IE_UIL1P_G711_ULAW;
		}
	}

    	new_woomera_event_printf(&wevent, "EVENT INCOMING s%dc%d%s"
					  		 "Unique-Call-Id: %s%s"
							 "Remote-Number: %s%s"
							 "Remote-Name: %s%s"
#if defined(BRI_PROT)
							 "Protocol: BRI%s"
#else
#if defined(PRI_PROT)
							 "Protocol: PRI%s"
#else
							 "Protocol: SS7%s"
#endif
#endif
							 "User-Agent: sangoma_mgd%s"
							 "Local-Number: %s%s"
							 "Channel-Name: g%d/%d%s"
							 "Trunk-Group: %d%s"
							 "Presentation: %d%s"
							 "Screening: %d%s"
							 "RDNIS: %s%s"
							 "Bearer-Cap: %s%s"
							 "uil1p: %s%s"
							 "xCustom: %s%s"
							 ,
							 event->span+1,
							 event->chan+1,
							 WOOMERA_LINE_SEPERATOR,
							 session,
							 WOOMERA_LINE_SEPERATOR,
							 event->calling.digits,
							 WOOMERA_LINE_SEPERATOR,
							 event->calling_name,
							 WOOMERA_LINE_SEPERATOR,
							 WOOMERA_LINE_SEPERATOR,
							 WOOMERA_LINE_SEPERATOR,
							 event->called.digits,
							 WOOMERA_LINE_SEPERATOR,
							 event->trunk_group+1,
							 (event->span*max_chans)+event->chan+1,
							 WOOMERA_LINE_SEPERATOR,
							 event->trunk_group+1,
							 WOOMERA_LINE_SEPERATOR,
							 event->calling_number_presentation,
							 WOOMERA_LINE_SEPERATOR,
							 event->calling_number_screening_ind,
							 WOOMERA_LINE_SEPERATOR,
							 event->rdnis.digits,
							 WOOMERA_LINE_SEPERATOR,
							 bearer_cap_to_str(event->bearer.capability), 
							 WOOMERA_LINE_SEPERATOR,
							 uil1p_to_str(event->bearer.uil1p),
							 WOOMERA_LINE_SEPERATOR,
							 event->custom_data,
							 WOOMERA_RECORD_SEPERATOR
						 );

	clients=enqueue_event_on_listeners(&wevent);
	if (!clients) {
		
		pthread_mutex_lock(&server.process_lock);
		server.process_table[event->span][event->chan].dev = NULL;
		memset(server.process_table[event->span][event->chan].session,0,SMG_SESSION_NAME_SZ);
		pthread_mutex_unlock(&server.process_lock);  
	
		if (autoacm) {
			isup_exec_command(event->span, 
					event->chan, 
					-1,
					SIGBOOST_EVENT_CALL_STOPPED,
					17);
			log_printf(SMG_LOG_ALL, server.log, 
				"CALL INCOMING: Enqueue Error Sent SIGBOOST_EVENT_CALL_STOPPED  [s%dc%d]\n", 
					event->span+1, event->chan+1);

		} else {
			isup_exec_command(event->span, 
					  event->chan, 
					  -1,
					  SIGBOOST_EVENT_CALL_START_NACK,
					  17);
			log_printf(SMG_LOG_ALL, server.log, 
				"CALL INCOMING: Enqueue Error Sent SIGBOOST_EVENT_CALL_START_NACK  [s%dc%d]\n", 
					event->span+1, event->chan+1);

		}
	
	} else {
		//pthread_mutex_lock(&server.process_lock);
		server.process_table[event->span][event->chan].clients = clients;
		//pthread_mutex_unlock(&server.process_lock);
	}
	
	destroy_woomera_event_data(&wevent);
       
}

static void handle_incoming_digit(call_signal_event_t *event)
{
	struct woomera_interface *woomera;
	int digits_len;
	
	if (smg_validate_span_chan(event->span,event->chan)) {
		log_printf(SMG_LOG_DEBUG_CALL, server.log, "Event DTMF on invalid span chan [s%dc%d] !\n",
			   event->span+1, event->chan+1);
		return;
	} 
	
	
	if (!event->called_number_digits_count) {
		log_printf(SMG_LOG_ALL, server.log, "Error Incoming digit with len %s %d [s%dc%d]\n",
			   	event->called_number_digits,
			   	event->called_number_digits_count,
			   	event->span+1, event->chan+1);
	}
	
	log_printf(SMG_LOG_DEBUG_9, server.log, "Queuing incoming digits %s [s%dc%d]\n", 
		   			event->called_number_digits,
     					event->span+1, event->chan+1);
		
	pthread_mutex_lock(&server.digits_lock);
		
	digits_len = server.process_table[event->span][event->chan].digits_len;
		
	strncpy(&server.process_table[event->span][event->chan].digits[digits_len],
			event->called_number_digits,
			event->called_number_digits_count);

	server.process_table[event->span][event->chan].digits_len += event->called_number_digits_count;
	
	pthread_mutex_unlock(&server.digits_lock);
	
	pthread_mutex_lock(&server.process_lock);
	woomera = server.process_table[event->span][event->chan].dev;
	pthread_mutex_unlock(&server.process_lock);
	if (!woomera || !strlen(woomera->session)) {
		return;	
	}
	woomera_check_digits(woomera);
}

static void handle_gap_abate(short_signal_event_t *event)
{
	log_printf(SMG_LOG_ALL, server.log, "NOTICE: GAP Cleared!\n", 
			event->span+1, event->chan+1);
	smg_clear_ckt_gap(event->trunk_group+1);
}

static void handle_restart(call_signal_connection_t *mcon, short_signal_event_t *event)
{
     	if (!woomera_test_flag(&server.master_connection, WFLAG_MONITOR_RUNNING)) {
            log_printf(SMG_LOG_ALL, server.log,"ERROR! Monitor Thread not running!\n");
		} else if (woomera_test_flag(&server.master_connection, WFLAG_SYSTEM_NEED_RESET_ACK)){
			log_printf(SMG_LOG_ALL, server.log,
									   "RESTART Ignored: Outgoing RESTART ACK not received\n");  
        } else {
			/* Clear Reset */	
			/* Tell all threads to go down */

			log_printf(SMG_LOG_ALL, server.log,
									   "RESTART Received: resetting all threads\n");

			int i=0;
			for ( i =0 ; i < SMG_MAX_TG ; i++ ){
				smg_all_ckt_busy(i);
			}

			gettimeofday(&server.restart_timeout,NULL);
			woomera_set_flag(&server.master_connection, WFLAG_SYSTEM_NEED_IN_RESET_ACK); 
			woomera_set_flag(&server.master_connection, WFLAG_SYSTEM_RESET); 

#if 0
			sleep(5);

			clear_all_holding_tank();
			
			sleep(2);
			
			event->event_id = SIGBOOST_EVENT_SYSTEM_RESTART_ACK;	
			err=call_signal_connection_write(&server.mcon, (call_signal_event_t*)event);
					if (err < 0) {
							log_printf(SMG_LOG_ALL, server.log,
									   "Critical System Error: Failed to tx on ISUP socket [%s]: %s\n",
											 strerror(errno));
					}
			
			smg_all_ckt_busy();
			woomera_clear_flag(&server.master_connection, WFLAG_SYSTEM_RESET); 
#endif
        }

}

static void handle_restart_ack(call_signal_connection_t *mcon, short_signal_event_t *event)
{
	woomera_clear_flag(&server.master_connection, WFLAG_SYSTEM_NEED_RESET_ACK);
	/* Do not reset WFLAG_SYSTEM_RESET flag here!. The flag should
           only be reset on restart from boost */
	//woomera_clear_flag(&server.master_connection, WFLAG_SYSTEM_RESET); 
}



static void handle_remove_loop(short_signal_event_t *event)
{
        struct woomera_interface *woomera;

        pthread_mutex_lock(&server.process_lock);
        woomera = server.process_table[event->span][event->chan].dev;
        memset(server.process_table[event->span][event->chan].session,0,SMG_SESSION_NAME_SZ);
	server.process_table[event->span][event->chan].dev=NULL;
        pthread_mutex_unlock(&server.process_lock);

        if (woomera) {

                woomera_set_flag(woomera,
                                (WFLAG_MEDIA_END|WFLAG_HANGUP));
	
                /* We have to close the socket because
                   At this point we are release span chan */

                log_printf(SMG_LOG_DEBUG_MISC, server.log, "Event REMOVE LOOP on s%dc%d ptr=%p ms=%p\n",
                                woomera->span+1,woomera->chan+1,woomera,woomera->ms);

	}
}


static void handle_call_stop(short_signal_event_t *event)
{
    struct woomera_interface *woomera;
	int ack=0;

	woomera = NULL;
    pthread_mutex_lock(&server.process_lock);
    woomera = server.process_table[event->span][event->chan].dev;
	if (woomera) {

		if (!woomera_test_flag(woomera, WFLAG_WAIT_FOR_NACK_ACK_SENT) && 
            	    !woomera_test_flag(woomera, WFLAG_WAIT_FOR_STOPPED_ACK_SENT)) {
			/* Only if we are not already waiting for hangup */
			//server.process_table[event->span][event->chan].dev=NULL;

		} else if (woomera_test_flag(woomera, WFLAG_HANGUP)) {
			/* At this point call is already hangup */
			woomera=NULL;
		} else {
			/* At this point call is already hangup */
			woomera=NULL;
		}
	}
	memset(server.process_table[event->span][event->chan].session,0,SMG_SESSION_NAME_SZ);
	pthread_mutex_unlock(&server.process_lock);
    
	if (woomera) {
	
		woomera_set_cause_topbx(woomera,event->release_cause);
	
		woomera_set_flag(woomera,
				(WFLAG_MEDIA_END|WFLAG_HANGUP|WFLAG_HANGUP_ACK));

		/* We have to close the socket because
	           At this point we are release span chan */

		log_printf(SMG_LOG_DEBUG_MISC, server.log, "Event CALL STOP on s%dc%d ptr=%p ms=%p\n",
				woomera->span+1,woomera->chan+1,woomera,woomera->ms);

	} else {
		ack++;
	}


	if (ack) {	
		/* At this point we have already sent our STOP so its safe to ACK */
		isup_exec_command(event->span, 
		  	  event->chan, 
		  	  -1,
		  	  SIGBOOST_EVENT_CALL_STOPPED_ACK,
		  	  0);
	}
		
	if (!woomera){
		/* This is allowed on incoming call if remote app does not answer it */
		log_printf(SMG_LOG_DEBUG_MISC, server.log, "Event CALL STOP referrs to a non-existant session [s%dc%d]!\n",
			event->span+1, event->chan+1);
	}
}

static void handle_heartbeat(short_signal_event_t *event)
{
	if (!woomera_test_flag(&server.master_connection, WFLAG_MONITOR_RUNNING)) {
		log_printf(SMG_LOG_ALL, server.log,"ERROR! Monitor Thread not running!\n");
	} else {	
		int err=call_signal_connection_writep(&server.mconp, (call_signal_event_t*)event);
		if (err < 0) {
			log_printf(SMG_LOG_ALL, server.log,
                                   "Critical System Error: Failed to tx on ISUP socket [%s]: %s\n",
                                         strerror(errno));
		}
	}
	return;
}


static void handle_call_stop_ack(short_signal_event_t *event)
{

	struct woomera_interface *woomera = NULL;


	if (smg_validate_span_chan(event->span,event->chan)) {
		log_printf(0,server.log, "Error: invalid span=% chan=%i on call stopped ack [s%dc%d]\n",
				event->span+1, event->chan+1, event->span+1,event->chan+1);
		return;
	}
	pthread_mutex_lock(&server.process_lock);
    	woomera = server.process_table[event->span][event->chan].dev;
	server.process_table[event->span][event->chan].dev=NULL;
	memset(server.process_table[event->span][event->chan].session,0,SMG_SESSION_NAME_SZ);
    	pthread_mutex_unlock(&server.process_lock);

	
	if (woomera) {
		log_printf(SMG_LOG_DEBUG_CALL, server.log, "Stop Ack on [s%dc%d] [Setup ID: %d] [%s]!\n",
				event->span+1, event->chan+1, event->call_setup_id,
				woomera->interface);

		woomera_clear_flag(woomera, WFLAG_WAIT_FOR_STOPPED_ACK);

	} else {
		log_printf(SMG_LOG_DEBUG_CALL, server.log, "Event CALL_STOP_ACK(%d) referrs to a non-existant session [s%dc%d] [Setup ID: %d]!\n",
				event->event_id, event->span+1, event->chan+1, event->call_setup_id);
	}
	
	if (event->call_setup_id > 0) {
		clear_from_holding_tank(event->call_setup_id, NULL);	
	}

	/* No need for us to do any thing here */
	return;
}


static void handle_call_start_nack_ack(short_signal_event_t *event)
{

   	struct woomera_interface *woomera = NULL;
	
	if ((woomera=pull_from_holding_tank(event->call_setup_id,-1,-1))) {
	
		woomera_clear_flag(woomera, WFLAG_WAIT_FOR_NACK_ACK);

	} else if (event->call_setup_id == 0 &&
		   smg_validate_span_chan(event->span,event->chan) == 0) {

		pthread_mutex_lock(&server.process_lock);
		woomera = server.process_table[event->span][event->chan].dev;
		server.process_table[event->span][event->chan].dev=NULL;
		memset(server.process_table[event->span][event->chan].session,
			0,SMG_SESSION_NAME_SZ);			
		pthread_mutex_unlock(&server.process_lock);

		if (woomera) {
			woomera_clear_flag(woomera, WFLAG_WAIT_FOR_NACK_ACK);
		} else {
			/* Possible if incoming call is NACKed due to
			 * woomera client being down, in this case no
		         * woomera thread is created */
			log_printf(SMG_LOG_DEBUG_8, server.log, 
			"Event NACK ACK [s%dc%d] with valid span/chan no dev!\n",
				event->span+1, event->chan+1);
		}

	} else {
		log_printf(SMG_LOG_DEBUG_CALL, server.log, 
			"Event NACK ACK referrs to a non-existant session [s%dc%d] [Setup ID: %d]!\n",
				event->span+1, event->chan+1, event->call_setup_id);
	}
	
	if (event->call_setup_id > 0) {
		clear_from_holding_tank(event->call_setup_id, NULL);	
	}

	/* No need for us to do any thing here */
	return;
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
			handle_call_loop_start(event);
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
	case SIGBOOST_EVENT_DIGIT_IN:
			handle_incoming_digit((call_signal_event_t*)event);
			break;
	case SIGBOOST_EVENT_CALL_PROGRESS:
			/* We always assume early media, so there is no need to do anything on progress */
			break;
	default:
			log_printf(SMG_LOG_ALL, server.log, "Warning no handler implemented for [%s val:%d]\n", 
				call_signal_event_id_name(event->event_id), event->event_id);
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
			log_printf(SMG_LOG_ALL, server.log, "Error: Opening MCON Socket [%d] %s\n", 
					mcon->socket,strerror(errno));
			exit(-1);
	} 

	if (call_signal_connection_open(mconp,
					mconp->cfg.local_ip, 
					mconp->cfg.local_port,
					mconp->cfg.remote_ip,
					mconp->cfg.remote_port) < 0) {
			log_printf(SMG_LOG_ALL, server.log, "Error: Opening MCONP Socket [%d] %s\n", 
					mconp->socket,strerror(errno));
			exit(-1);
	}
	
	mcon->log = server.log;
	mconp->log = server.log;
	
	isup_exec_commandp(0, 
			  0, 
			  -1,
			  SIGBOOST_EVENT_SYSTEM_RESTART,
			  0);

	gettimeofday(&server.restart_timeout,NULL);
	woomera_set_flag(&server.master_connection, WFLAG_SYSTEM_NEED_RESET_ACK);
	woomera_set_flag(&server.master_connection, WFLAG_SYSTEM_RESET); 

	smg_get_current_priority(&policy,&priority);
	
	log_printf(SMG_LOG_PROD, server.log, "Open udp socket [%d] [%d]\n",
				 mcon->socket,mconp->socket);
	log_printf(SMG_LOG_PROD, server.log, "Monitor Thread Started (%i:%i)\n",policy,priority);
				
	
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
					log_printf(SMG_LOG_DEBUG_9, server.log, "Socket Event P [%s] \n",
									call_signal_event_id_name(event->event_id));
					parse_ss7_event(mconp,(short_signal_event_t*)event);
					if (++i < 10) {
						goto mcon_retry_priority;
					}
				
			    } else if (errno != EAGAIN) {
					ss=-1;
					log_printf(SMG_LOG_ALL, server.log, 
						"Error: Reading from Boost P Socket! (%i) %s\n",
						errno,strerror(errno));
					break;
			    }
			}
		
			i=0;

			if (a) {
			    if ((event = call_signal_connection_read(mcon,i))) {
                   	log_printf(SMG_LOG_DEBUG_9, server.log, "Socket Event [%s]\n",
                               	   	call_signal_event_id_name(event->event_id));
                   	parse_ss7_event(mcon,(short_signal_event_t*)event);

			    } else if (errno != EAGAIN) {
					ss=-1;
					log_printf(SMG_LOG_ALL, server.log, 
						"Error: Reading from Boost Socket! (%i) %s\n",
						errno,strerror(errno));
					break;
			    }
			} 

		} 

		if (ss < 0){
			log_printf(SMG_LOG_ALL, server.log, "Thread Run: Select Socket Error!\n");
			break;
		}
			
		if (woomera_test_flag(&server.master_connection, WFLAG_SYSTEM_RESET)) {
			short_signal_event_t event;
			struct timeval current;
			int elapsed;
			gettimeofday(&current,NULL);

			elapsed=smg_calc_elapsed(&server.restart_timeout, &current);
			if (elapsed > 5000) {
				int err;

                if (woomera_test_flag(&server.master_connection, WFLAG_SYSTEM_NEED_RESET_ACK)) {
                    log_printf(SMG_LOG_ALL, server.log, "Reset Timeout: Tx RESTART Elapsed=%i!\n",elapsed);     
					gettimeofday(&server.restart_timeout,NULL);
                    woomera_clear_flag(&server.master_connection, WFLAG_SYSTEM_NEED_IN_RESET_ACK);
					isup_exec_commandp(0, 
						  0, 
						  -1,
						  SIGBOOST_EVENT_SYSTEM_RESTART,
						  0);


				} else if (woomera_test_flag(&server.master_connection, WFLAG_SYSTEM_NEED_IN_RESET_ACK)) {

					log_printf(SMG_LOG_ALL, server.log, "Reset Condition Cleared Elapsed=%i!\n",elapsed);
					clear_all_holding_tank();
					memset(&event,0,sizeof(event));
					event.event_id = SIGBOOST_EVENT_SYSTEM_RESTART_ACK;	
					err=call_signal_connection_write(&server.mcon, (call_signal_event_t*)&event);
					if (err < 0) {
						log_printf(SMG_LOG_ALL, server.log,
							   "Critical System Error: Failed to tx on ISUP socket [%s]: %s\n",
								 strerror(errno));
					}
					int i=0;
					for ( i =0 ; i < SMG_MAX_TG ; i++ ){
						smg_all_ckt_busy(i);
					}  
					woomera_clear_flag(&server.master_connection, WFLAG_SYSTEM_RESET); 
				} else {
					gettimeofday(&server.restart_timeout,NULL);
				}
			}
		}
		
	}
 
	log_printf(SMG_LOG_PROD, server.log, "Close udp socket [%d] [%d]\n", 
					mcon->socket,mconp->socket);
	call_signal_connection_close(&server.mcon);
	call_signal_connection_close(&server.mconp);

    	pthread_mutex_lock(&server.thread_count_lock);
	server.thread_count--;
    	pthread_mutex_unlock(&server.thread_count_lock);
	
	woomera_clear_flag(&server.master_connection, WFLAG_MONITOR_RUNNING);
	log_printf(SMG_LOG_ALL, server.log, "Monitor Thread Ended\n");
	
	return NULL;
}

static void woomera_loop_thread_run(struct woomera_interface *woomera)
{
	int err=launch_media_thread(woomera);
	if (err) {
		log_printf(SMG_LOG_ALL, server.log, "Failed to start loop media thread\n");
		woomera_set_flag(woomera, 
				(WFLAG_HANGUP|WFLAG_MEDIA_END));
		woomera_clear_flag(woomera, WFLAG_RUNNING);
		return;
	}

	while (woomera_test_flag(&server.master_connection, WFLAG_RUNNING) &&
               woomera_test_flag(&server.master_connection, WFLAG_MONITOR_RUNNING) &&
	       !woomera_test_flag(&server.master_connection, WFLAG_SYSTEM_RESET) &&
	       !woomera_test_flag(woomera, WFLAG_MEDIA_END) &&
	       !woomera_test_flag(woomera, WFLAG_HANGUP)) {
	
		usleep(300000);				
		continue;
		
	}
		
	woomera_clear_flag(woomera, WFLAG_RUNNING);

	log_printf(SMG_LOG_DEBUG_CALL, server.log, "Woomera Session: For Loop Test exiting %s\n",woomera->interface);

	return;
}

static void woomera_check_digits (struct woomera_interface *woomera) 
{
	int span, chan;
	
	if (!strlen(woomera->session)) {
		return;
	}
	
	if (get_span_chan_from_interface(woomera->interface, &span, &chan) == 0) {
		pthread_mutex_lock(&server.digits_lock);
		if (server.process_table[span-1][chan-1].digits_len > 0) {
			int i;
			unsigned char digit;

			for (i=0; i < server.process_table[span-1][chan-1].digits_len; i++) {
				digit = server.process_table[span-1][chan-1].digits[i];
					
				handle_event_dtmf(woomera, digit);
			}
			
			server.process_table[span-1][chan-1].digits_len = 0;
		} 
		pthread_mutex_unlock(&server.digits_lock);
	}
} 


static void *woomera_thread_run(void *obj)
{
    struct woomera_interface *woomera = obj;
    struct woomera_message wmsg;
    struct woomera_event wevent;
    char *event_string;
    int mwi;
    int err;
    int policy=0, priority=0;
    int span = -1, chan = -1;

    woomera_message_init(&wmsg);
	
    smg_get_current_priority(&policy,&priority);

    log_printf(SMG_LOG_DEBUG_CALL, server.log, "WOOMERA session started (ptr=%p : loop=%i)(%i:%i) Index=%i\n", 
    			woomera,woomera->loop_tdm,policy,priority, woomera->index);

    pthread_mutex_lock(&server.thread_count_lock);
    server.thread_count++;
    pthread_mutex_unlock(&server.thread_count_lock);

    pthread_mutex_init(&woomera->queue_lock, NULL);	
    pthread_mutex_init(&woomera->ms_lock, NULL);	
    pthread_mutex_init(&woomera->dtmf_lock, NULL);	
    pthread_mutex_init(&woomera->vlock, NULL);	
    pthread_mutex_init(&woomera->flags_lock, NULL);	

    if (woomera->loop_tdm) {
		/* We must set woomera socket to -1 otherwise
			a valid file descriptor will get closed */
		woomera->socket = -1;
    	woomera_loop_thread_run(woomera);
    	goto woomera_session_close;
    }
    
    err=socket_printf(woomera->socket,
				  "EVENT HELLO Sangoma Media Gateway%s"
				  "Supported-Protocols: TDM%s"
				  "Version: %s%s"
				  "Remote-Address: %s%s"
				  "Remote-Port: %d%s"
				  "Raw-Format: %s%s"
				  "xUDP-Seq: %s%s"
				  "xUDP-Seq-Err: %d%s"
				  "xNative-Bridge: %s%s",
				  WOOMERA_LINE_SEPERATOR,
				  WOOMERA_LINE_SEPERATOR,
				  SMG_VERSION, WOOMERA_LINE_SEPERATOR,
				  inet_ntoa(woomera->addr.sin_addr), WOOMERA_LINE_SEPERATOR,
				  ntohs(woomera->addr.sin_port), WOOMERA_LINE_SEPERATOR,
				  server.hw_coding?"ALAW":"ULAW", WOOMERA_LINE_SEPERATOR,
				  server.udp_seq?"Enabled":"Disabled", WOOMERA_LINE_SEPERATOR,
				  server.media_rx_seq_err, WOOMERA_LINE_SEPERATOR,
				  server.media_pass_through?"Enabled":"Disabled", WOOMERA_RECORD_SEPERATOR
				  );

    if (err) {
    	log_printf(SMG_LOG_ALL, server.log, "Woomera session socket failure! (ptr=%p)\n",
			woomera);
		woomera_clear_flag(woomera, WFLAG_RUNNING);
		goto woomera_session_close;
    }

    while (	woomera_test_flag(&server.master_connection, WFLAG_RUNNING) &&
			woomera_test_flag(&server.master_connection, WFLAG_MONITOR_RUNNING) &&
			woomera_test_flag(woomera, WFLAG_RUNNING) &&
			!woomera_test_flag(woomera, WFLAG_MEDIA_END) &&
			!woomera_test_flag(woomera, WFLAG_HANGUP)) {
					
		mwi = woomera_message_parse(woomera, &wmsg, WOOMERA_HARD_TIMEOUT);
		if (mwi >= 0) {
	
			if (mwi) {
				interpret_command(woomera, &wmsg);			
			} else if (woomera_test_flag(woomera, WFLAG_EVENT)){
				while ((event_string = dequeue_event(woomera))) {
					if (socket_printf(woomera->socket, "%s", event_string)) {
						woomera_set_flag(woomera, WFLAG_MEDIA_END);	
						woomera_clear_flag(woomera, WFLAG_RUNNING);
						smg_free(event_string);
						log_printf(SMG_LOG_DEBUG_8, server.log,
							"WOOMERA session (ptr=%p) print string error\n",
								woomera);
						break;
					}
					smg_free(event_string);
				}
				woomera_clear_flag(woomera, WFLAG_EVENT);
			}
	
			if(woomera->check_digits) {
				woomera_check_digits(woomera);
				woomera->check_digits = 0;
			}
			
			if (woomera->timeout > 0 && time(NULL) >= woomera->timeout) {
			
				/* Sent the hangup only after we sent a NACK */	
	
				log_printf(SMG_LOG_DEBUG_CALL, server.log, 
					"WOOMERA session Call Timedout ! [%s]\n",
						woomera->interface);
			
				/* Let the Index check below send a NACK */
				if (woomera->index) {
					woomera->q931_rel_cause_tosig=19;
					woomera_set_flag(woomera, WFLAG_HANGUP);
				}
				break;
			}

		} else  {
			log_printf(SMG_LOG_DEBUG_MISC, server.log, "WOOMERA session (ptr=%p) [%s] READ MSG Error %i \n",
				woomera,woomera->interface,mwi);
			break;
		}
		
	}	
    
woomera_session_close:
	
	log_printf(SMG_LOG_DEBUG_CALL, server.log, "WOOMERA session (ptr=%p) is dying [%s]:  SR=%d WR=%d WF=0x%04X\n",
			woomera,woomera->interface,
		woomera_test_flag(&server.master_connection, WFLAG_RUNNING),
		woomera_test_flag(woomera, WFLAG_RUNNING),
		woomera->flags);


	if (woomera_test_flag(woomera, WFLAG_MEDIA_RUNNING)) {
		woomera_set_flag(woomera, WFLAG_MEDIA_END);
		while(woomera_test_flag(woomera, WFLAG_MEDIA_RUNNING)) {
			usleep(100);
			sched_yield();
		}
	}


	/***********************************************
         * Identify the SPAN CHAN to be used below
	 ***********************************************/

	chan = woomera->chan;
	span = woomera->span;


	if (!woomera_test_flag(woomera, WFLAG_HANGUP)) {
		
		/* The call was not HUNGUP. This is the last check,
		   If the call is valid, hungup the call if the call
		   was never up the keep going */
		

		if (smg_validate_span_chan(span,chan) == 0) {	

			if (!woomera->index) {

				if (autoacm || woomera_test_flag(woomera,WFLAG_CALL_ACKED)) {

					woomera_set_flag(woomera, WFLAG_WAIT_FOR_STOPPED_ACK);
					isup_exec_command(span, 
							chan, 
							-1,
							SIGBOOST_EVENT_CALL_STOPPED,
							woomera->q931_rel_cause_tosig);
					woomera_set_flag(woomera, WFLAG_WAIT_FOR_STOPPED_ACK_SENT);
				
	
					log_printf(SMG_LOG_DEBUG_MISC, woomera->log, "Woomera Sent SIGBOOST_EVENT_CALL_STOPPED [s%dc%d] [%s] ptr=%p\n", 
							span+1, chan+1,woomera->interface,woomera);
				} else {
					
					woomera_set_flag(woomera, WFLAG_WAIT_FOR_NACK_ACK);
					isup_exec_command(span, 
							chan, 
							-1,
							SIGBOOST_EVENT_CALL_START_NACK,
							woomera->q931_rel_cause_tosig);
					woomera_set_flag(woomera, WFLAG_WAIT_FOR_NACK_ACK_SENT);
		
					log_printf(SMG_LOG_DEBUG_MISC, woomera->log, "Woomera Sent SIGBOOST_EVENT_CALL_START_NACK [s%dc%d] [%s] ptr=%p\n", 
							span+1, chan+1,woomera->interface,woomera);
				}
			} else {
				log_printf(SMG_LOG_ALL, woomera->log, "Woomera Not Sent CALL STOPPED - Instead NACK [s%dc%d] [%s] ptr=%p\n", 
						span+1, chan+1,woomera->interface,woomera);
				
			}
		}else{
			/* This can happend if an outgoing call times out
			   or gets hungup before it gets acked. Its not a
			   failure */
			if (!woomera->index) {
				/* In this case we really failed to tx STOP */
				log_printf(SMG_LOG_DEBUG_MISC, woomera->log, "FAILED: Woomera (R) SIGBOOST_EVENT_CALL_STOPPED [s%dc%d] [%s] Index=%d ptr=%p\n", 
					span+1, chan+1, woomera->interface, woomera->index, woomera);
			}
		}

		woomera_set_flag(woomera, WFLAG_HANGUP);

	}

woo_re_hangup:

	/* We must send a STOP ACK to boost telling it that we are done */
	if (woomera_test_flag(woomera, WFLAG_HANGUP_ACK)) {
	
		/* SMG received a HANGUP from boost.
 		   We must now send back the ACK to the HANGUP.
		   Boost will not release channel until we
		   ACK the hangup */
	
		if (smg_validate_span_chan(span,chan) == 0) {	

			isup_exec_command(span, 
			  		  chan, 
			  		  -1,
			  		  SIGBOOST_EVENT_CALL_STOPPED_ACK,
					  woomera->q931_rel_cause_tosig);
			
			log_printf(SMG_LOG_DEBUG_MISC, woomera->log, 
				"Sent (Ack) to SIGBOOST_EVENT_CALL_STOPPED_ACK [s%dc%d] [%s] ptr=%p\n", 
						span+1,chan+1,woomera->interface,woomera);

		}else{
			/* This should never happen! If it does
			   we broke protocol */
			log_printf(SMG_LOG_ALL, woomera->log, 
				"FAILED: Woomera (R) SIGBOOST_EVENT_CALL_STOPPED_ACK [s%dc%d] [%s] Index=%d ptr=%p\n", 
					span+1, chan+1, woomera->interface, woomera->index, woomera);
		}

		woomera_clear_flag(woomera, WFLAG_HANGUP_ACK);
		woomera_set_flag(woomera, WFLAG_HANGUP);
	}
	
	if (woomera_test_flag(woomera, WFLAG_HANGUP_NACK_ACK)) {
	
		/* SMG received a NACK from boost during call startup.
 		   We must now send back the ACK to the NACK.
		   Boost will not release channel until we
		   ACK the NACK */

		if (smg_validate_span_chan(span,chan) == 0) {	
		
			isup_exec_command(span, 
			  		  chan, 
			  		  -1,
			  		  SIGBOOST_EVENT_CALL_START_NACK_ACK,
					  woomera->q931_rel_cause_tosig);
			
			log_printf(SMG_LOG_DEBUG_MISC, woomera->log, 
				"Sent (Nack Ack) to SIGBOOST_EVENT_CALL_START_NACK_ACK [s%dc%d] [%s] ptr=%p\n", 
						span+1,chan+1,woomera->interface,woomera);

			woomera->index=0;

		} else if (woomera->index) {
			isup_exec_command(0, 
			  		  0, 
			  		  woomera->index,
			  		  SIGBOOST_EVENT_CALL_START_NACK_ACK,
					  woomera->q931_rel_cause_tosig);

			woomera->index=0;

		} else {
			log_printf(SMG_LOG_ALL, woomera->log, 
				"FAILED: Sent (Nack Ack) SIGBOOST_EVENT_CALL_START_NACK_ACK [s%dc%d] [%s] Index=%d ptr=%p\n", 
					span+1, chan+1, woomera->interface, woomera->index, woomera);
		}

		woomera_clear_flag(woomera, WFLAG_HANGUP_NACK_ACK);
		
	} 
	
	if (woomera->index) { 
	
		int index = woomera->index;
		
		new_woomera_event_printf(&wevent, "EVENT HANGUP %s%s"
										  "Unique-Call-Id: %s%s"
										  "Timeout: %ld%s"
										  "Cause: %s%s"
										  "Q931-Cause-Code: %d%s",
										woomera->interface,
										WOOMERA_LINE_SEPERATOR,
										woomera->session,
										WOOMERA_LINE_SEPERATOR,
										woomera->timeout,
										WOOMERA_LINE_SEPERATOR,
										q931_rel_to_str(18),
										WOOMERA_LINE_SEPERATOR,
										18,
										WOOMERA_RECORD_SEPERATOR
										);
		enqueue_event(woomera, &wevent,EVENT_FREE_DATA);
		
		new_woomera_event_printf(&wevent, "501 call was cancelled!%s"
				"Unique-Call-Id: %s%s",
				WOOMERA_LINE_SEPERATOR,
				woomera->session,
				WOOMERA_RECORD_SEPERATOR);

		enqueue_event(woomera, &wevent,EVENT_FREE_DATA);

		while ((event_string = dequeue_event(woomera))) {
				socket_printf(woomera->socket, "%s", event_string);
				smg_free(event_string);
		}

		if (peek_from_holding_tank(index)) {
			
			woomera_set_flag(woomera, WFLAG_WAIT_FOR_NACK_ACK);
			isup_exec_command(0, 
					0, 
					index,
					SIGBOOST_EVENT_CALL_START_NACK,
					woomera->q931_rel_cause_tosig);
			woomera_set_flag(woomera, WFLAG_WAIT_FOR_NACK_ACK_SENT);		
	
			log_printf(SMG_LOG_DEBUG_CALL, woomera->log, 
				"Sent SIGBOOST_EVENT_CALL_START_NACK [Setup ID: %d] .. WAITING FOR NACK ACK\n", 
			 	index);
		} else {
			log_printf(SMG_LOG_PROD, woomera->log, 
				"Error Failed to Sent SIGBOOST_EVENT_CALL_START_NACK [Setup ID: %d] - index stale!\n", 
			 	index);
		}
	}		
	
	if (woomera_test_flag(woomera, WFLAG_WAIT_FOR_NACK_ACK)) {
		int timeout_cnt=0;
		int overall_cnt=0;
		
		/* SMG sent NACK to boost, however we have to wait
		   for boost to give us the ACK back before we
		   release resources. */

		while (woomera_test_flag(woomera, WFLAG_WAIT_FOR_NACK_ACK)) {
			timeout_cnt++;
			if (timeout_cnt > 100) {  //5sec timeout
				timeout_cnt=0;
				overall_cnt++;
				
				log_printf((overall_cnt==1)?0:4, woomera->log, 
				"Waiting for NACK ACK [Setup ID: %d] ... \n", 
					woomera->index_hold);
			}		

			if (overall_cnt > 15) { //50sec timeout
				woomera_clear_flag(woomera, WFLAG_WAIT_FOR_NACK_ACK);
				woomera_set_flag(woomera, WFLAG_WAIT_FOR_ACK_TIMEOUT);
				break;
			}

			if (!woomera_test_flag(&server.master_connection, WFLAG_RUNNING)) {
				woomera_set_flag(woomera, WFLAG_WAIT_FOR_ACK_TIMEOUT);
				break;
			}
	   		if (woomera_test_flag(&server.master_connection, WFLAG_SYSTEM_RESET)){
				break;
			} 

			/* If ACK comes in while we wait for NACK ACK, the ACk will
			   clear the tank causing NACK ACK never to clear the waiting flag 
			   in this case we abort waiting for NACK ACK and we timeout 
			   the wait */
	   		if (woomera_test_flag(&server.master_connection, WFLAG_CALL_ACKED)){
				woomera_set_flag(woomera, WFLAG_WAIT_FOR_ACK_TIMEOUT);
				break;
			} 

			usleep(50000);
			sched_yield();
		}

		woomera_clear_flag(woomera, WFLAG_WAIT_FOR_NACK_ACK);

		/* If ACK came in while waiting for NACK ACK, we TIMEOUT on purpose.
		   The ACK has blocked the tank id and now ony NACK ACK can unblock it. 
		   THe only problem is that we blind and have to assume that NACK ACK 
		   will eventually come in :) */
		if (!woomera_test_flag(&server.master_connection, WFLAG_CALL_ACKED)) {
			if (woomera_test_flag(woomera, WFLAG_WAIT_FOR_ACK_TIMEOUT)) {
				log_printf(SMG_LOG_ALL, woomera->log, 
					"Waiting for NACK ACK [Setup ID: %d] .. TIMEOUT on NACK ACK\n", 
					woomera->index_hold);
		
			} else {
				log_printf(SMG_LOG_DEBUG_8, woomera->log, 
				"Waiting for NACK ACK [Setup ID: %d] .. GOT NACK ACK\n", 
				woomera->index_hold);
			}	
		}
	}
		
    	if (woomera_test_flag(woomera, WFLAG_EVENT)){
		while ((event_string = dequeue_event(woomera))) {
			socket_printf(woomera->socket, "%s", event_string);
			smg_free(event_string);
		}
		woomera_clear_flag(woomera, WFLAG_EVENT);
    	}


       if (woomera_test_flag(woomera, WFLAG_WAIT_FOR_STOPPED_ACK)) {
                int timeout_cnt=0;	
		int overall_cnt=0;

		/* SMG sent HANGUP to boost, however we have to wait
		   for boost to give us the ACK back before we
		   release resources. */

                log_printf(SMG_LOG_DEBUG_8, woomera->log,
                              "Waiting for STOPPED ACK [%s] [id=%i]... \n",
                                         woomera->interface,woomera->index_hold);

                while (woomera_test_flag(woomera, WFLAG_WAIT_FOR_STOPPED_ACK)) {
                        timeout_cnt++;
                        if (timeout_cnt > 100) {  //5sec timeout
				timeout_cnt=0;
				overall_cnt++;
				log_printf((overall_cnt==1)?0:4, woomera->log,
                                       "Waiting for STOPPED ACK [%s] [id=%i] %i... \n",
                                                woomera->interface,woomera->index_hold,overall_cnt);
                        }

			if (overall_cnt > 15) { //50sec 
                		woomera_clear_flag(woomera, WFLAG_WAIT_FOR_STOPPED_ACK);
				woomera_set_flag(woomera, WFLAG_WAIT_FOR_ACK_TIMEOUT);
				break;
			}


			if (!woomera_test_flag(&server.master_connection, WFLAG_RUNNING)) {
                		woomera_clear_flag(woomera, WFLAG_WAIT_FOR_STOPPED_ACK);
				woomera_set_flag(woomera, WFLAG_WAIT_FOR_ACK_TIMEOUT);
				break;
			}
	   		
			if (woomera_test_flag(&server.master_connection, WFLAG_SYSTEM_RESET)){
				break;
			} 
		
			usleep(50000);
                        sched_yield();
                }
                		
		woomera_clear_flag(woomera, WFLAG_WAIT_FOR_STOPPED_ACK);

		if (woomera_test_flag(woomera, WFLAG_WAIT_FOR_ACK_TIMEOUT)) {
			log_printf(SMG_LOG_ALL, woomera->log,
                             		"Wait TIMEDOUT on STOPPED ACK [%s] [id=%i]... \n",
                                       	 woomera->interface,woomera->index_hold);

		} else {
			log_printf(SMG_LOG_DEBUG_8, woomera->log,
                          		"Wait GOT STOPPED ACK [%s] [id=%i]... \n",
                                       	woomera->interface,woomera->index_hold);
		}
        }
	
	/***************************************************** 
	 * We must wait for WFLAG_WAIT_FOR_STOPPED_ACK here
  	 * so that STOP_ACK can access the woomera 
	 *****************************************************/
		
	if (smg_validate_span_chan(span,chan) == 0) {	
                log_printf(SMG_LOG_DEBUG_CALL, woomera->log,
                              "WOOMERA Clearing Processs Table ... \n",
                                         woomera->interface);
		pthread_mutex_lock(&server.process_lock);
    		if (server.process_table[span][chan].dev == woomera){
			server.process_table[span][chan].dev = NULL;
			memset(server.process_table[span][chan].session,0,SMG_SESSION_NAME_SZ);
			memset(server.process_table[span][chan].digits,0,
			       sizeof(server.process_table[span][chan].digits));
			server.process_table[span][chan].digits_len = 0;
		}
		pthread_mutex_unlock(&server.process_lock);
	}

#if 0	
//Used for testing
	if (1) {
		int chan = woomera->chan;
		int span = woomera->span;
		if (smg_validate_span_chan(span,chan) == 0) {	
			pthread_mutex_lock(&server.process_lock);
			/* This is possible in case media thread dies on startup */
			
    			if (server.process_table[span][chan]){
				log_printf(SMG_LOG_ALL, server.log, 
				"Sanity Span Chan Still in use: [s%dc%d] [%s] Index=%d ptr=%p\n", 
					span+1, chan+1, woomera->interface, woomera->index, woomera);
				//server.process_table[span][chan] = NULL;
			}
			pthread_mutex_unlock(&server.process_lock);
		}
	}
#endif
	
	usleep(3000000);
	
	/* Sanity Check */
	if (woomera_test_flag(woomera, WFLAG_HANGUP_ACK)) {
    		log_printf(SMG_LOG_ALL, woomera->log, 
			"Woomera MISSED HANGUP ACK: Retry HANGUP ACK\n");
		goto woo_re_hangup;
	}
	if (woomera_test_flag(woomera, WFLAG_HANGUP_NACK_ACK)) {
    		log_printf(SMG_LOG_ALL, woomera->log, 
			"Woomera MISSED HANGUP ACK: Retry HANGUP NACK ACK\n");
		goto woo_re_hangup;
	}


	/* This is where we actually pull the index
	 * out of the tank. We had to keep the tank
	 * value until the end of the call. Tank is only
	 * used on outgoing calls. */
	if (woomera->index_hold >= 1) {
		if (woomera_test_flag(woomera, WFLAG_WAIT_FOR_ACK_TIMEOUT)) {
			/* Replace real woomera interface with a dummy so 
			   the tank is blocked. The real woomera interface will be
			   deallocated */
			pull_from_holding_tank(woomera->index_hold,-1,-1);

			if (check_tank_index(woomera->index_hold) != NULL){
    			log_printf(SMG_LOG_PROD, woomera->log, "Woomera Thread: [%s] setup id %i blocked waiting for NACK or ACK\n", 
						woomera->interface ,woomera->index_hold);
			}
		} else {
			clear_from_holding_tank(woomera->index_hold, woomera);

		}
		woomera->index_hold=0;
	}

	log_printf(SMG_LOG_DEBUG_CALL, woomera->log, "Woomera Thread Finished %u\n", (unsigned long) woomera->thread);
   	close_socket(&woomera->socket);
   	woomera->socket=-1;

    	/* delete queue */
   	while ((event_string = dequeue_event(woomera))) {
		smg_free(event_string);
    	}
	
	if (woomera_test_flag(woomera, WFLAG_LISTENING)) {
		del_listener(woomera);
    	}

    	log_printf(SMG_LOG_DEBUG_CALL, server.log, "WOOMERA session for [%s] stopped (ptr=%p)\n", 
    			woomera->interface,woomera);

   	pthread_mutex_destroy(&woomera->queue_lock);
    	pthread_mutex_destroy(&woomera->ms_lock);
	pthread_mutex_destroy(&woomera->dtmf_lock);
	pthread_mutex_destroy(&woomera->vlock);
    	pthread_mutex_destroy(&woomera->flags_lock);
    	woomera_set_raw(woomera, NULL);
    	woomera_set_interface(woomera, NULL);

	woomera_message_clear(&wmsg);

    	smg_free(woomera);
    	pthread_mutex_lock(&server.thread_count_lock);
    	server.call_count--;
    	server.thread_count--;
    	pthread_mutex_unlock(&server.thread_count_lock);

	pthread_exit(NULL);
    	return NULL;
}


static int launch_woomera_thread(struct woomera_interface *woomera) 
{
    int result = 0;
    pthread_attr_t attr;

    result = pthread_attr_init(&attr);
    //pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);
    //pthread_attr_setschedpolicy(&attr, SCHED_RR);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    pthread_attr_setstacksize(&attr, MGD_STACK_SIZE);

    woomera_set_flag(woomera, WFLAG_RUNNING);
    result = pthread_create(&woomera->thread, &attr, woomera_thread_run, woomera);
    if (result) {
		log_printf(SMG_LOG_ALL, server.log, "%s: Error: Creating Woomera Thread! (%i) %s\n",
			 __FUNCTION__,result,strerror(errno));
    	woomera_clear_flag(woomera, WFLAG_RUNNING);
    }
    pthread_attr_destroy(&attr);
    
    return result;
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
    
    log_printf(SMG_LOG_ALL,server.log,"%s: Old Priority =%i res=%i \n",__FUNCTION__,
			 param.sched_priority,result);


    woomera_set_flag(&server.master_connection, WFLAG_MONITOR_RUNNING);
    result = pthread_create(&server.monitor_thread, &attr, monitor_thread_run, NULL);
    if (result) {
	log_printf(SMG_LOG_ALL, server.log, "%s: Error: Creating Thread! %s\n",
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
    
    log_printf(SMG_LOG_ALL,server.log,"Starting %s span!\n",span->ifname);
    
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
    log_printf(SMG_LOG_ALL,server.log,"Stopping %s span!\n",span->ifname);
    
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
	 log_printf(SMG_LOG_ALL, server.log, "%s: Error: Creating Thread! %s\n",
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
	struct smg_tdm_ip_bridge *ip_bridge=NULL;

	
    server.dtmf_intr_ch = -1;

    if (!woomera_open_file(&cfg, server.config_file)) {
		log_printf(SMG_LOG_ALL, server.log, "open of %s failed\n", server.config_file);
		return 0;
    }

	log_printf(SMG_LOG_ALL,server.log, "\n======================= \n");
	log_printf(SMG_LOG_ALL,server.log, "Server - Configuration \n");
	log_printf(SMG_LOG_ALL,server.log, "======================= \n");
	
    while (woomera_next_pair(&cfg, &var, &val)) {
		if (!strcasecmp(var, "boost_local_ip")) {
			strncpy(server.mcon.cfg.local_ip, val,
				 sizeof(server.mcon.cfg.local_ip) -1);
			strncpy(server.mconp.cfg.local_ip, val,
				 sizeof(server.mconp.cfg.local_ip) -1);
			log_printf(SMG_LOG_ALL,server.log, "Server - Boost Local IP:    %s\n",val);

			cnt++;
		} else if (!strcasecmp(var, "boost_local_port")) {
			server.mcon.cfg.local_port = atoi(val);
			server.mconp.cfg.local_port = 
				server.mcon.cfg.local_port+1;
			log_printf(SMG_LOG_ALL,server.log, "Server - Boost Local Port:  %i\n",server.mcon.cfg.local_port);
			cnt++;
		} else if (!strcasecmp(var, "boost_remote_ip")) {
			strncpy(server.mcon.cfg.remote_ip, val,
				 sizeof(server.mcon.cfg.remote_ip) -1);
			strncpy(server.mconp.cfg.remote_ip, val,
				 sizeof(server.mconp.cfg.remote_ip) -1);
			log_printf(SMG_LOG_ALL,server.log, "Server - Boost Remote IP:   %s\n",server.mcon.cfg.remote_ip);
			cnt++;
		} else if (!strcasecmp(var, "boost_remote_port")) {
			server.mcon.cfg.remote_port = atoi(val);
			server.mconp.cfg.remote_port = 
				server.mcon.cfg.remote_port+1;
			log_printf(SMG_LOG_ALL,server.log, "Server - Boost Remote Port: %i\n",server.mcon.cfg.local_port);
			cnt++;
		} else if (!strcasecmp(var, "logfile_path")) {
			if (!server.logfile_path) {
				server.logfile_path = smg_strdup(val);
			}
		} else if (!strcasecmp(var, "woomera_port")) {
			server.port = atoi(val);
			log_printf(SMG_LOG_ALL,server.log, "Server - Woomera Port:      %i\n",server.port);
		} else if (!strcasecmp(var, "debug_level")) {
			server.debug = atoi(val);
			log_printf(SMG_LOG_ALL,server.log, "Server - Debug Level:       %i\n",server.debug);
		} else if (!strcasecmp(var, "out_tx_test")) {
		        server.out_tx_test = atoi(val);
			log_printf(SMG_LOG_ALL,server.log, "Server - Tx Media Dbg:      %s\n",server.out_tx_test?"On":"Off (Default)");
		} else if (!strcasecmp(var, "loop_trace")) {
		        server.loop_trace = atoi(val);
			log_printf(SMG_LOG_ALL,server.log, "Server - Media Loop Trace:  %s\n",server.loop_trace?"On":"Off (Default)");
		} else if (!strcasecmp(var, "rxgain")) {
		        server.rxgain = atoi(val);
			log_printf(SMG_LOG_ALL,server.log, "Server - Rx Gain:           %d\n",server.rxgain);
		} else if (!strcasecmp(var, "txgain")) {
		        server.txgain = atoi(val);
			log_printf(SMG_LOG_ALL,server.log, "Server - Tx Gain:           %d\n",server.txgain);
		} else if (!strcasecmp(var, "dtmf_on_duration")){
			server.dtmf_on = atoi(val);
			log_printf(SMG_LOG_ALL,server.log, "Server - DTMF ON Duration:  %d ms\n",server.dtmf_on);
		} else if (!strcasecmp(var, "dtmf_off_duration")){
			server.dtmf_off = atoi(val);
			log_printf(SMG_LOG_ALL,server.log, "Server - DTMF Off Duration: %d ms\n",server.dtmf_off);
		} else if (!strcasecmp(var, "dtmf_inter_ch_duration")){
			server.dtmf_intr_ch = atoi(val);
			log_printf(SMG_LOG_ALL,server.log, "Server - DTMF Spacing:      %d\n",server.dtmf_intr_ch);
		} else if (!strcasecmp(var, "strip_cid_non_digits")){
			server.strip_cid_non_digits = atoi(val);
			log_printf(SMG_LOG_ALL,server.log, "Server - Strip non digits:  %d\n",server.strip_cid_non_digits);
		} else if (!strcasecmp(var, "max_calls")) {
			int max = atoi(val);
			if (max > 0) {
				server.max_calls = max;
				log_printf(SMG_LOG_ALL,server.log, "Server - Max Active Calls:  %d\n",server.max_calls);
			}
		} else if (!strcasecmp(var, "autoacm")) {
			int max = atoi(val);
			if (max >= 0) {
				autoacm=max;
				log_printf(SMG_LOG_ALL,server.log, "Server - Auto ACM:          %s\n",autoacm?"On":"Off (Default)");
			}			
		} else if (!strcasecmp(var, "max_spans")) {
			int max = atoi(val);
			if (max > 0) {
				max_spans = max;
				log_printf(SMG_LOG_ALL,server.log, "Server - Max Spans:         %d\n",max_spans);
			}
		} else if (!strcasecmp(var, "call_timeout")) {
			int max = atoi(val);
			if (max >= 0) {
				server.call_timeout=max;
			} else {
				server.call_timeout=SMG_DEFAULT_CALL_TIMEOUT;
			}
			log_printf(SMG_LOG_ALL,server.log, "Server - Call Comp Timeout: %d s\n",server.call_timeout);

		} else if (!strcasecmp(var, "base_media_port")) {
			int base = atoi(val);
			if (base >= 0) {
				server.base_media_port = base;
				server.next_media_port = base;
				server.max_media_port =  server.base_media_port + 	WOOMERA_MAX_MEDIA_PORTS;
				log_printf(SMG_LOG_ALL,server.log, "Server - Base Media Port:   %d\n",server.base_media_port);
			}

		} else if (!strcasecmp(var, "max_media_ports")) {
			int max = atoi(val);
			if (max >= 0) {
				server.max_media_port = server.base_media_port+max;
				log_printf(SMG_LOG_ALL,server.log, "Server - Max Media Port:    %d\n",server.max_media_port);
			}
	
		} else if (!strcasecmp(var, "media_ip")) {
			strncpy(server.media_ip, val, sizeof(server.media_ip) -1);
			log_printf(SMG_LOG_ALL,server.log, "Server - Media IP:          %s\n",server.media_ip);
		
		} else if (!strcasecmp(var, "udp_seq")) {
			int max = atoi(val);
			if (max > 0) {
				server.udp_seq=max;
	   	         log_printf(SMG_LOG_ALL, server.log, "Server - UDP Seq:           %s\n",server.udp_seq?"Enabled":"Disabled");
			}

		} else if (!strcasecmp(var, "media_pass_through")) {
			int max = atoi(val);
			if (max > 0) {
				server.media_pass_through=max;
				log_printf(SMG_LOG_ALL, server.log, "Server - Media Pass Through: %s\n",server.media_pass_through?"Enabled":"Disabled");
			}

			
		} else if (!strcasecmp(var, "rbs_relay")) {
			int max = atoi(val);
			if (max > 0) {
				server.rbs_relay[max]=max;
	   	         log_printf(SMG_LOG_ALL, server.log, "Server - RBS Relay Span:    %d\n",max);
			} 

		} else if (!strcasecmp(var, "bridge_tdm_ip")) {
			int err=smg_get_ip_bridge_session(&ip_bridge);
			if (err) {
				log_printf(SMG_LOG_ALL, server.log, "Error failed to get free ip bridge %i!\n",err);
			} else {
				log_printf(SMG_LOG_ALL,server.log, "\n======================= \n");
				log_printf(SMG_LOG_ALL,server.log, "Bridge - Configuration \n");
				log_printf(SMG_LOG_ALL,server.log, "======================= \n");
			}		

		} else if (!strcasecmp(var, "bridge_span")) {
			int max = atoi(val);
			if (max > 0 && max <= 32 && ip_bridge) {
				ip_bridge->span=max;
				log_printf(SMG_LOG_ALL, server.log, "Bridge Span:                %i\n",ip_bridge->span);
			} else {
				log_printf(SMG_LOG_ALL, server.log, "Bridge Span:                ERROR: Invalid Value %s\n",val);
			}
		} else if (!strcasecmp(var, "bridge_chan")) {
			int max = atoi(val);
			if (max > 0 && max < MAX_SMG_BRIDGE && ip_bridge) {
				ip_bridge->chan=max;
				log_printf(SMG_LOG_ALL, server.log, "Bridge Chan:                %i\n",ip_bridge->chan);
			} else {
				log_printf(SMG_LOG_ALL, server.log, "Bridge Chan:                ERROR: Invalid Value %s\n",val);
			}
		} else if (!strcasecmp(var, "bridge_local_ip")) {
			if (ip_bridge) {
				strncpy(ip_bridge->mcon.cfg.local_ip, val,
				 	sizeof(ip_bridge->mcon.cfg.local_ip) -1);
				log_printf(SMG_LOG_ALL, server.log, "Bridge Local IP:            %s\n",ip_bridge->mcon.cfg.local_ip);
			}
		} else if (!strcasecmp(var, "bridge_remote_ip")) {
			if (ip_bridge) {
				strncpy(ip_bridge->mcon.cfg.remote_ip, val,
					sizeof(ip_bridge->mcon.cfg.remote_ip) -1);
				log_printf(SMG_LOG_ALL, server.log, "Bridge Remote IP:           %s\n",ip_bridge->mcon.cfg.remote_ip);
			}
		} else if (!strcasecmp(var, "bridge_port")) {
			int max = atoi(val);
			if (max > 0 && ip_bridge) {
				ip_bridge->mcon.cfg.local_port=max;
				ip_bridge->mcon.cfg.remote_port=max;
				log_printf(SMG_LOG_ALL, server.log, "Bridge Port:                %i\n",max);
			}
		} else if (!strcasecmp(var, "bridge_period")) {
			int max = atoi(val);
			if (max > 0 && ip_bridge) {
				ip_bridge->period=max;
				log_printf(SMG_LOG_ALL, server.log, "Bridge Period:              %ims\n",ip_bridge->period);
			}
		} else {
			log_printf(SMG_LOG_ALL, server.log, "Invalid Option %s at line %d!\n", var, cfg.lineno);
		}
    }

	log_printf(SMG_LOG_ALL,server.log, "======================= \n\n");

    /* Post initialize */
    if (server.dtmf_on == 0){ 
	server.dtmf_on=SMG_DTMF_ON;
    }
    if (server.dtmf_off == 0) {
	server.dtmf_off=SMG_DTMF_OFF;
    }
    if (server.dtmf_intr_ch == -1) {
	server.dtmf_intr_ch = 0;
    }
    server.dtmf_size=(server.dtmf_on+server.dtmf_off)*10*2;

    log_printf(SMG_LOG_ALL,server.log, "DTMF On=%i Off=%i IntrCh=%i Size=%i\n",
		server.dtmf_on,server.dtmf_off,server.dtmf_intr_ch,server.dtmf_size);

    woomera_close_file(&cfg);
    return cnt == 4 ? 1 : 0;
}



static int main_thread(void)
{

    struct sockaddr_in sock_addr, client_addr;
    struct woomera_interface *new_woomera;
    int client_sock = -1, pid = 0;
    unsigned int len = 0;
    FILE *tmp;

    if ((server.master_connection.socket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
	fprintf(stderr,"%s:%d socket() failed %s\n",__FUNCTION__,__LINE__,strerror(errno));
	return 1;
    }
	
    memset(&sock_addr, 0, sizeof(sock_addr));	/* Zero out structure */
    sock_addr.sin_family = AF_INET;				   /* Internet address family */
    sock_addr.sin_addr.s_addr = htonl(INADDR_ANY); /* Any incoming interface */
    sock_addr.sin_port = htons(server.port);	  /* Local port */

    /* Bind to the local address */
    if (bind(server.master_connection.socket, (struct sockaddr *) &sock_addr, sizeof(sock_addr)) < 0) {
		fprintf(stderr,"%s:%d socket() bind %i failed %s\n",__FUNCTION__,__LINE__,server.port,strerror(errno));
		log_printf(SMG_LOG_ALL, server.log, "bind(%d) failed\n", server.port);
		return 1;
    }
	 
    /* Mark the socket so it will listen for incoming connections */
    if (listen(server.master_connection.socket, MAXPENDING) < 0) {
	fprintf(stderr,"%s:%d socket() listen failed %s\n",__FUNCTION__,__LINE__,strerror(errno));
	log_printf(SMG_LOG_ALL, server.log, "listen() failed\n");
	return 1;
    }

    if ((pid = get_pid_from_file(PIDFILE))) {
	fprintf(stderr,"%s:%d get pid file failed %s\n",__FUNCTION__,__LINE__,strerror(errno));
	log_printf(SMG_LOG_ALL, stderr, "pid %d already exists.\n", pid);
	exit(0);
    }

    if (!(tmp = safe_fopen(PIDFILE, "w"))) {
	fprintf(stderr,"%s:%d open pid file failed %s\n",__FUNCTION__,__LINE__,strerror(errno));
	log_printf(SMG_LOG_ALL, stderr, "Error creating pidfile %s\n", PIDFILE);
	return 1;
    } else {
	fprintf(tmp, "%d", getpid());
	fclose(tmp);
	tmp = NULL;
    }

    no_nagle(server.master_connection.socket);

#if 0
    if (1) {
	int span,chan;
   	call_signal_event_t event;
#if 0	
	span=1;
	chan=30;
	event.span=span;
	event.chan=chan;
    	launch_woomera_loop_thread(&event);
#else
	for (span=0;span<8;span++) {
		for (chan=0;chan<31;chan++) {
			event.span=span;
			event.chan=chan;
    			launch_woomera_loop_thread(&event);
		}
	}
#endif
    }
#endif

#ifdef WP_HPTDM_API
    if (1) {
	int span;
	for (span=0;span<16;span++) {
		hptdmspan[span] = sangoma_hptdm_api_span_init(span);
		if (!hptdmspan[span]) {
			break;
		} else {
			log_printf(SMG_LOG_ALL, server.log, "HP TDM API Span: %d configured...\n", 
					span);
			launch_hptdm_api_span_thread(span);
		}
	}
    }	
#endif
    
    log_printf(SMG_LOG_PROD, server.log, "Main Process Started: Woomera Ready port: %d\n", server.port);

    while (woomera_test_flag(&server.master_connection, WFLAG_RUNNING) &&
           woomera_test_flag(&server.master_connection, WFLAG_MONITOR_RUNNING)) { 

		/* Set the size of the in-out parameter */
		len = sizeof(client_addr);

		/* Wait for a client to connect */
		if ((client_sock = accept(server.master_connection.socket, (struct sockaddr *) &client_addr, &len)) < 0) {
			/* Check if we are supposed to stop */
			if(!woomera_test_flag(&server.master_connection, WFLAG_RUNNING)) {
				log_printf(SMG_LOG_ALL, server.log, "accept() stopped\n");
				return 0;
			}
			log_printf(SMG_LOG_ALL, server.log, "accept() failed\n");
			return 1;
		}

		if ((new_woomera = new_woomera_interface(client_sock, &client_addr, len))) {
			log_printf(SMG_LOG_DEBUG_CALL, server.log, "Starting Thread for New Connection %s:%d Sock=%d\n",
					   inet_ntoa(new_woomera->addr.sin_addr),
					   ntohs(new_woomera->addr.sin_port),
					   client_sock);
			
			pthread_mutex_lock(&server.thread_count_lock);
			server.call_count++;
			pthread_mutex_unlock(&server.thread_count_lock);
			
			
			if (launch_woomera_thread(new_woomera)) {
				socket_printf(new_woomera->socket,
						"501 call was cancelled!%s", 
						WOOMERA_RECORD_SEPERATOR);

				close_socket(&new_woomera->socket);
				new_woomera->socket=-1;
				smg_free(new_woomera);
				log_printf(SMG_LOG_ALL, server.log, "ERROR: failed to launch woomera thread\n");
			}
		} else {
			log_printf(SMG_LOG_ALL, server.log, "Critical ERROR: memory/socket error\n");
		}
    }

    log_printf(SMG_LOG_PROD, server.log, "Main Process End\n");

    return 0;
}

static int do_ignore(int sig)

{
#ifdef SMG_DROP_SEQ
	drop_seq=4;
#endif
	sdla_memdbg_free(0);
    	return 0;
}

static int do_shut(int sig)
{
    woomera_clear_flag(&server.master_connection, WFLAG_RUNNING);
    close_socket(&server.master_connection.socket);
    log_printf(SMG_LOG_PROD, server.log, "Caught SIG %d, Closing Master Socket!\n", sig);
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
	int span_cnt, chan_cnt;
    char *cfg=NULL, *debug=NULL, *arg=NULL;

    while((arg = argv[x++])) {

		if (!strcasecmp(arg, "-hup")) {
			if (! (pid = get_pid_from_file(PIDFILE))) {
				log_printf(SMG_LOG_ALL, stderr, "Error reading pidfile %s\n", PIDFILE);
				exit(1);
			} else {
				log_printf(SMG_LOG_ALL, stderr, "Killing PID %d\n", pid);
				kill(pid, SIGHUP);
				sleep(1);
				exit(0);
			}
		
		 } else if (!strcasecmp(arg, "-term") || !strcasecmp(arg, "--term")) {
                        if (! (pid = get_pid_from_file(PIDFILE))) {
                                log_printf(SMG_LOG_ALL, stderr, "Error reading pidfile %s\n", PIDFILE);
                                exit(1);
                        } else {
                                log_printf(SMG_LOG_ALL, stderr, "Killing PID %d\n", pid);
                                kill(pid, SIGTERM);
                                unlink(PIDFILE);
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
			unlink(PIDFILE);
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
				server.logfile_path = smg_strdup(argv[x++]);
			}
		} else if (*arg == '-') {
			log_printf(SMG_LOG_ALL, stderr, "Unknown Option %s\n", arg);
			fprintf(stdout, "%s\n%s [-help] | [-hup] | [-wipe] | [[-bg] | [-debug <level>] | [-cfg <path>] | [-log <path>]]\n\n", WELCOME_TEXT, argv[0]);
			exit(1);
		}
    }

    if (1){
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
		log_printf(SMG_LOG_ALL, stderr, "Backgrounding!\n");
		return 0;
    }

	for	(span_cnt = 0; span_cnt < CORE_MAX_SPANS; span_cnt++) {
		for	(chan_cnt = 0; chan_cnt < CORE_MAX_CHAN_PER_SPAN; chan_cnt++) {
			pthread_mutex_init(&server.process_table[span_cnt][chan_cnt].media_lock, NULL);
		}
	}

    q931_cause_setup();
    bearer_cap_setup();
    uil1p_to_str_setup();

    server.port = 42420;
    server.debug = 0;
    strcpy(server.media_ip, "127.0.0.1");
	server.base_media_port = WOOMERA_MIN_MEDIA_PORT;
	server.max_media_port  = WOOMERA_MAX_MEDIA_PORT;
    server.next_media_port = server.base_media_port;
    server.log = stdout;
    server.master_connection.socket = -1;
    server.master_connection.event_queue = NULL;
    server.config_file = cfg ? cfg : "/etc/sangoma_mgd.conf";
    pthread_mutex_init(&server.listen_lock, NULL);	
    pthread_mutex_init(&server.ht_lock, NULL);	
    pthread_mutex_init(&server.process_lock, NULL);
    pthread_mutex_init(&server.digits_lock, NULL);
    pthread_mutex_init(&server.media_udp_port_lock, NULL);	
    pthread_mutex_init(&server.thread_count_lock, NULL);	
    pthread_mutex_init(&server.master_connection.queue_lock, NULL);	
    pthread_mutex_init(&server.master_connection.flags_lock, NULL);	
    pthread_mutex_init(&server.mcon.lock, NULL);	
	server.master_connection.chan = -1;
	server.master_connection.span = -1;
	server.call_timeout=SMG_DEFAULT_CALL_TIMEOUT;

	if (smg_log_init()) {
		printf("Error: Logger Launch Failed\n");
		fprintf(stderr, "Error: Logger Launch Failed\n");
		return 0;
    }

    if (!configure_server()) {
		log_printf(SMG_LOG_ALL, server.log, "configuration failed!\n");
		return 0;
    }

#ifndef USE_SYSLOG
    if (server.logfile_path) {
		if (!(server.log = safe_fopen(server.logfile_path, "a"))) {
			log_printf(SMG_LOG_ALL, stderr, "Error setting logfile %s!\n", server.logfile_path);
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
		log_printf(SMG_LOG_ALL, stderr,  "Warning: Failed to disable core size limit: %s\n", 
			strerror(errno));
	}
    }

#ifdef __LINUX__
    if (geteuid() && coredump) {
    	if (prctl(PR_SET_DUMPABLE, 1, 0, 0, 0) < 0) {
		log_printf(SMG_LOG_ALL, stderr,  "Warning: Failed to disable core size limit for non-root: %s\n", 
			strerror(errno));
        }
    }
#endif



	(void) signal(SIGINT,(void *) do_shut);
	(void) signal(SIGTERM,(void *) do_shut);
	(void) signal(SIGPIPE,(void *) do_ignore);
	(void) signal(SIGUSR1,(void *) do_ignore);	
	(void) signal(SIGHUP,(void *) do_shut);

    /* Wait for logger thread to start */
    usleep(5000);
   
    woomera_set_flag(&server.master_connection, WFLAG_RUNNING);

    if (launch_monitor_thread()) {
    	woomera_clear_flag(&server.master_connection, WFLAG_RUNNING);
		smg_log_cleanup();
		return 0;
    }
	
    fprintf(stderr, "%s", WELCOME_TEXT);

	log_printf(SMG_LOG_ALL, stderr, "Woomera STARTUP Complete. [AutoACM=%i SDigit=%i]\n",
			autoacm,server.strip_cid_non_digits);
    return 1;
}

static int woomera_shutdown(void) 
{
    char *event_string;
    int told = 0, loops = 0;
	int span_cnt, chan_cnt;

	for	(span_cnt = 0; span_cnt < CORE_MAX_SPANS; span_cnt++) {
		for	(chan_cnt = 0; chan_cnt < CORE_MAX_CHAN_PER_SPAN; chan_cnt++) {
			pthread_mutex_destroy(&server.process_table[span_cnt][chan_cnt].media_lock);
		}
	}

    close_socket(&server.master_connection.socket);
    pthread_mutex_destroy(&server.listen_lock);
    pthread_mutex_destroy(&server.ht_lock);	
    pthread_mutex_destroy(&server.process_lock);
    pthread_mutex_destroy(&server.digits_lock);
    pthread_mutex_destroy(&server.media_udp_port_lock);
    pthread_mutex_destroy(&server.thread_count_lock);
    pthread_mutex_destroy(&server.master_connection.queue_lock);
    pthread_mutex_destroy(&server.master_connection.flags_lock);
    pthread_mutex_destroy(&server.mcon.lock);
    woomera_clear_flag(&server.master_connection, WFLAG_RUNNING);


    if (server.logfile_path) {
		smg_free(server.logfile_path);
		server.logfile_path = NULL;
    }

    /* delete queue */
    while ((event_string = dequeue_event(&server.master_connection))) {
		smg_free(event_string);
    }    

    while(server.thread_count > 0) {
		loops++;

		if (loops % 1000 == 0) {
			told = 0;
		}

		if (loops > 10000) {
			log_printf(SMG_LOG_ALL, server.log, "Red Alert! threads did not stop\n");
			assert(server.thread_count == 0);
		}

		if (told != server.thread_count) {
			log_printf(SMG_LOG_PROD, server.log, "Waiting For %d thread%s.\n", 
					server.thread_count, server.thread_count == 1 ? "" : "s");
			told = server.thread_count;
		}
		ysleep(10000);
    }
    unlink(PIDFILE);
    log_printf(SMG_LOG_ALL, stderr, "Woomera SHUTDOWN Complete.\n");

    smg_log_cleanup();	
	
    return 0;
}

int main(int argc, char *argv[]) 
{
    int ret = 0;

	memset(&server,0,sizeof(server));
	memset(&woomera_dead_dev,0,sizeof(woomera_dead_dev));
    
    mlockall(MCL_FUTURE);
	memset(&server, 0, sizeof(server));
	memset(&woomera_dead_dev, 0, sizeof(woomera_dead_dev));
	memset(&g_smg_ip_bridge_idx,0, sizeof(g_smg_ip_bridge_idx));
	
    ret=nice(-5);
    
    sdla_memdbg_init();

    server.hw_coding=0;

    openlog (ps_progname ,LOG_PID, LOG_LOCAL2);  
	
    if (! (ret = woomera_startup(argc, argv))) {
		exit(0);
    }

	ret = smg_ip_bridge_start();
	if (ret == 0) {
    	ret = main_thread();
	}

    woomera_shutdown();

	smg_ip_bridge_stop();
    sdla_memdbg_free(1);

    return ret;
}

/** EMACS **
 * Local variables:
 * mode: C
 * c-basic-offset: 4
 * End:
 */

