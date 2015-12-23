/*
 * Asterisk -- A telephony toolkit for Linux.
 *
 * Woomera Channel Driver
 * 
 * Copyright (C) 05-09 Nenad Corbic 
 *		       David Yat Sin
 * 		       Anthony Minessale II 
 *
 * Nenad Corbic <ncorbic@sangoma.com>
 * David Yat Sin <davidy@sangoma.com>
 * Anthony Minessale II <anthmct@yahoo.com>
 *
 * This program is free software, distributed under the terms of
 * the GNU General Public License
 * =============================================
 *
 * v1.72 Nenad Corbic <ncorbic@sangoma.com>
 * Jan 4 2011
 * Updated for Asterisk 1.8
 *
 * v1.71 Konrad Hammel <konrad@sangoma.com>
 * Jul 15 2010
 * Added "cid_pres" option to woomera.conf to hard code
 * the caller-id presentation value for all outgoing calls
 * 
 * v1.70 David Yat Sin <dyatsin@sangoma.com>
 * May 11 2010
 * Added check for invalid hangup causes
 *
 * v1.69 David Yat Sin <dyatsin@sangoma.com>
 * May 03 2010
 * Added support for WOOMERA_CUSTOM variable on outgoing calls
 * 
 * v1.68 David Yat Sin <dyatsin@sangoma.com>
 * Mar 19 2010
 * MEDIA and RING flags set on answer, based on tech_indicate
 * 
 * v1.67 David Yat Sin <dyatsin@sangoma.com>
 * Mar 15 2010
 * Fix cause code for rejected outbound calls
 * when sangoma_mgd not running.
 *
 * v1.66 David Yat Sin <dyatsin@sangoma.com>
 * Mar 10 2010
 * Support for TON and NPI passthrough
 *
 * v1.65 David Yat Sin <dyatsin@sangoma.com>
 * Feb 22 2010
 * RDnis parameter only contains RDNIS
 * Additional parameters now transferred as xCustom parameter
 * WOOMERA_CUSTOM variable exposed to user
 *
 * v1.64 Nenad Corbic <ncorbic@sangoma.com>
 * Jan 27 2010
 * Enabled media pass through so that
 * two woomera servers pass media directly.
 *
 * v1.63 Nenad Corbic <ncorbic@sangoma.com>
 * Jan 26 2010
 * Added bridge code and rbs relay
 *
 * v1.62 Nenad Corbic <ncorbic@sangoma.com>
 * Jan 24 2010
 * Added woomer called blacklist
 *
 * v1.61 Nenad Corbic <ncorbic@sangoma.com>
 * Jan 14 2010
 * Added media sequencing.
 * Enabled only if server HELLO message contains
 * sequence enable status.
 *
 * v1.60 Nenad Corbic <ncorbic@sangoma.com>
 * Nov 22 2009
 * Added Woomera No Answer feature for calling cards.
 *
 * v1.59 Nenad Corbic <ncorbic@sangoma.com>
 * Nov 16 2009
 * Bug fix in woomera profile verbose introduced in 1.58
 *
 * v1.58 Nenad Corbic <ncorbic@sangoma.com>
 * Nov 10 2009
 *  Added verbosity per profile.
 *  Updated cli commands
 *
 * v1.57 Nenad Corbic <ncorbic@sangoma.com>
 * Nov 10 2009
 *  Fixed global set flag. When one profile goes down it shuts down
 *  all calls.
 *
 * v1.56 Nenad Corbic <ncorbic@sangoma.com>
 * Oct 29 2009
 *  Fixed the call incoming call problem on second profile  
 *
 * v1.55 Nenad Corbic <ncorbic@sangoma.com>
 * Sep 22 2009
 *  Updated udp base port to 20000 and numbe of ports to 5000
 *
 * v1.54 Nenad Corbic <ncorbic@sangoma.com>
 * Sep 16 2009
 *  Added Progress Messages
 *
 * v1.53 Nenad Corbic <ncorbic@sangoma.com>
 * Jul 16 2009
 *	Updated for Asterisk load balancing and well
 *  as one to many call calling based on valid extension.
 *
 * v1.52 Konrad Hammel <konrad@sangoma.com>
 * Jun 25 2009
 * 	Bug fix for tg_context in multiple profiles
 *
 * v1.51 Nenad Corbic <ncorbic@sangoma.com>
 * Jun 06 2009
 * 	Updated for Asterisk 1.6.1
 *
 * v1.50 Nenad Corbic <ncorbic@sangoma.com>
 * Apr 24 2009
 * 	Bug fix on write socket. Check that write woomera socket failed.
 *	This update prevents socket warning messages on call congestion.
 *
 * v1.49 Nenad Corbic <ncorbic@sangoma.com>
 * Apr 08 2009
 * 	Bug fix on transfer. The owner was not
 * 	properly updated causing unpredictable behaviour.
 *
 * v1.48 Nenad Corbic <ncorbic@sangoma.com>
 * Apr 05 2009
 * 	Updated locking on pbx_start
 *	
 * v1.47 Nenad Corbic <ncorbic@sangoma.com>
 * Apr 03 2009
 *      Added BLOCKER sanity check.
 *
 * v1.46 Nenad Corbic <ncorbic@sangoma.com>
 * Mar 29 2009
 *	Added hup_pending stat to call_status
 *	Let tech_hangup destroy softhungup channel
 *
 * v1.45 Nenad Corbic <ncorbic@sangoma.com>
 * Mar 27 2009
 *	Major updates on channel locking.
 *	This update fixes potential crashing issues
 *	under heavy load. Stress tested in 500+ call 
 *      environment. 
 *
 * v1.44 Nenad Corbic <ncorbic@sangoma.com>
 * Mar 55 2009
 *	Updated woomera channel locking using 
 *      DEADLOCK_AVOIDANCE. Fixed RDNIS transmission issue.
 *
 * v1.43 David Yat Sin <dyatsin@sangoma.com>
 * Mar 01 2009
 *	Fix to support Callweaver svn trunk
 *	
 * v1.42 David Yat Sin <dyatsin@sangoma.com>
 * Feb 25 2009
 *	Fix to support Callweaver svn trunk
 *
 * v1.41 Nenad Corbic <ncorbic@sangoma.com>
 * Jan 29 2008
 *	Bug introduced in 1.40 if WOOMERA uil1p parameter
 *	was not there transcoding was misconfigued.
 *
 * v1.40 Nenad Corbic <ncorbic@sangoma.com>
 * Jan 26 2008
 *	Call Bearer Capability Feature
 *	Updated for Callweaver
 * 
 * v1.39 David Yat Sin <dyatsin@sangoma.com>
 * Dec 19 2008
 * 	Support for Asterisk 1.6
 *
 * v1.38 David Yat Sin <dyatsin@sangoma.com>
 * Dec 05 2008
 *	Support for fax_detect using Asterisk software DSP
 *
 * v1.37 Nenad Corbic <ncorbic@sangoma.com>
 * Nov 26 2008
 *	Bug Fix: tech_read try again now checks for hangup
 *
 * v1.36 David Yat Sin <dyatsin@sangoma.com>
 * Oct 14 2008
 *  Bug Fix: Call hangup on call park
 *
 * v1.35 Nenad Corbic <ncorbic@sangoma.com>
 * Jul 23 2008
 *	Bug Fix: Check for cid_name.
 *
 * v1.34 Nenad Corbic <ncorbic@sangoma.com>
 * Jul 23 2008
 *	Added udp tagging and rx/tx sync options for
 *	voice streams debugging. Not for production.
 *
 * v1.33 Nenad Corbic <ncorbic@sangoma.com>
 * Jul 18 2008
 *	Added UDP Sequencing to check for dropped frames
 *	Should only be used for debugging.
 *
 * v1.32 David Yat Sin <davidy@sangoma.com>
 * Jun 3 2008
 *	Updated for callweaver v1.2.0
 *	
 * v1.31 Nenad Corbic <ncorbic@sangoma.com>
 * v1.30 Nenad Corbic <ncorbic@sangoma.com>
 * Jun 2 2008
 *	Added AST_CTONROL_RING event on outgoing call.
 *	Updated for CallWeaver 1.2 SVN
 *
 * v1.29 David Yat Sin <davidy@sangoma.com>
 * Apr 30 2008
 *	Added AST_CONTROL_SRCUPDATE in tech_indicate
 *
 * v1.29 David Yat Sin <davidy@sangoma.com>
 * April 29 2008
 *	Support for HW DTMF
 *
 * v1.28 David Yat Sin <davidy@sangoma.com>
 * Apr 29 2008
 *	Fix for compilation issues with Callweaver v1.99
 *
 * v1.27 David Yat Sin <davidy@sangoma.com>
 * Feb 13 2008
 *	Fix for ast_channel type not defined on 
 * 	outgoing calls, causing PHP agi scripts
 *	to fail
 *
 * v1.26 Nenad Corbic <ncorbic@sangoma.com>
 * Feb 13 2008
 *	Compilation Update for callweaver 1.2-rc5
 *
 * v1.25 Nenad Corbic <ncorbic@sangoma.com>
 * Feb 06 2008
 *	Bug fix in woomera message declaration
 *      Possible memory overflow
 *
 * v1.24 Nenad Corbic <ncorbic@sangoma.com>
 * Jan 23 2008
 *	Removed LISTEN on every woomera channel. Listen
 *      only on master. Fixed jitterbuffer support on AST1.4
 *
 * v1.23 Nenad Corbic <ncorbic@sangoma.com>
 * Jan 22 2008
 *	Implemented Music on Hold.
 *
 * v1.22 David Yat Sin <davidy@sangoma.com>
 * Jan 11 2008
 * 	rxgain and txgain configuration parameters 
 *	are ignored if coding is not specified in 
 *	woomera.conf	
 *
 * v1.21 David Yat Sin <davidy@sangoma.com>
 * Dec 27 2007
 * 	Support for language  
 *
 * v1.20 David Yat Sin <davidy@sangoma.com>
 * Dec 20 2007
 *	 Support for call confirmation
 * 	 Support for default context
 *
 * v1.19 Nenad Corbic <ncorbic@sangoma.com>
 * Nov 30 2007
 * 	 Updated for latest CallWeaver
 *	 Updated smgversion update on master socket
 *       restart.
 *
 * v1.18 Nenad Corbic <ncorbic@sangoma.com>
 * 	 Updated Channel-Name on outbound call
 *       Check queued events on ABORT
 *       Major Unit Testing done
 *       Ability to change chan name from Makefile
 *
 * v1.17 Nenad Corbic <ncorbic@sangoma.com>
 *	 Updates for Asterisk 1.4
 *	 Updated the release causes
 * 	 Updated for tech_indication
 *	 
 * v1.16 Nenad Corbic <ncorbic@sangoma.com>
 *	 Added support for Asterisk 1.4
 *	 Updated support for Callweaver
 *
 * v1.15 Nenad Corbic <ncorbic@sangoma.com>
 * 	 Added PRI_CAUSE and Q931-Cause-Code 
 *	 in woomera protocol.
 *
 * v1.14 Nenad Corbic <ncorbic@sangoma.com>
 *	 Updated for session support
 *
 * v1.13 Nenad Corbic <ncorbic@sangoma.com>
 *	 Added CallWeaver Support
 *	 |->(thanks to Andre Schwaller)
 *	 Updated codec negotiation for 
 *	 mutliple profiles.
 *
 * v1.12 Nenad Corbic <ncorbic@sangoma.com>
 *	 Updated DTMF locking
 *
 * v1.11 Nenad Corbic <ncorbic@sangoma.com>
 *       Updated multiple profiles 
 *       Updated Dialect for OPAL Woomera
 *       Added call logging/debugging
 *
 * v1.10 Nenad Corbic <ncorbic@sangoma.com>
 *       Bug fix in incoming hangup 
 * 
 * v1.9 Nenad Corbic <ncorbic@sangoma.com>
 *	Fixed remote asterisk/woomera
 *      setup.
 *
 * v1.8 Nenad Corbic <ncorbic@sangoma.com>
 *	Added Woomera OPAL dialect.
 *      Code cleanup.
 *	Added cli call_status
 *
 * v1.7 Nenad Corbic <ncorbic@sangoma.com>
 *	Added smgdebug to enable smg debugging
 *	Added rdnis
 *
 * v1.6 Nenad Corbic <ncorbic@sangoma.com>
 *      Added incoming trunk group context 
 *      The trunk number will be added to the
 *      profile context name. 
 *      Added presentation feature.
 *
 * v1.5	Nenad Corbic <ncorbic@sangoma.com>
 *      Use only ALAW and MLAW not SLIN.
 *      This reduces the load quite a bit.
 *      Autodetect Format from HELLO Message.
 *      RxTx Gain supported in woomera.conf as well
 *      from CLI.
 */

#if defined(AST18)
#define W_SUBCLASS_INT  	subclass.integer
#define W_SUBCLASS_CODEC	subclass.codec
#define W_CID_NAME  		caller.id.name.str
#define W_CID_NAME_PRES   	caller.id.name.presentation
#define W_CID_NUM   		caller.id.number.str
#define W_CID_NUM_PRES   	caller.id.number.presentation
#define W_CID_FROM_RDNIS	redirecting.from.number.str		
#define W_CID_SET_FROM_RDNIS(self,_value) self->redirecting.from.number.str = _value; self->redirecting.from.number.valid=1 
#else
#define W_SUBCLASS_CODEC	subclass
#define W_SUBCLASS_INT  	subclass
#define W_CID_NAME  		cid.cid_name
#define W_CID_NAME_PRES 	cid.cid_pres
#define W_CID_NUM   		cid.cid_num
#define W_CID_NUM_PRES 		cid.cid_pres
#define W_CID_FROM_RDNIS	cid.cid_rdnis
#define W_CID_SET_FROM_RDNIS(self,_value) self->cid.cid_rdnis = _value 
#endif

#if !defined(CALLWEAVER)
#include "asterisk.h"
#endif

#if defined(CALLWEAVER) && defined(HAVE_CONFIG_H)
#include "confdefs.h"
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <math.h>
#include <netinet/tcp.h>


#ifndef CALLWEAVER

#include "asterisk.h"
#include "asterisk/lock.h"
#include "asterisk/channel.h"
#include "asterisk/config.h"
#include "asterisk/module.h"
#include "asterisk/astobj.h"
#include "asterisk/sched.h"


#if defined(AST16) 
#include "asterisk/linkedlists.h"
#include "asterisk/channel.h"
#endif
#if !defined (AST14) && !defined (AST16)
#include "asterisk/options.h"
#endif
#include "asterisk/manager.h"
#include "asterisk/pbx.h"
#include "asterisk/cli.h"
#include "asterisk/logger.h"
#include "asterisk/frame.h"
#include "asterisk/config.h"
#include "asterisk/module.h"
#include "asterisk/lock.h"
#include "asterisk/translate.h"
#include "asterisk/causes.h"
#include "asterisk/dsp.h"
#include "asterisk/musiconhold.h"
#include "asterisk/transcap.h"

ASTERISK_FILE_VERSION(__FILE__, "$Revision: 1.72 $")

#else

#include "callweaver.h"
#include "callweaver/sched.h"
#ifdef CALLWEAVER_CWOBJ
#include "callweaver/cwobj.h"
#else
#include "callweaver/astobj.h"
#endif
#include "callweaver/lock.h"
#include "callweaver/manager.h"
#include "callweaver/options.h"
#include "callweaver/channel.h"
#include "callweaver/pbx.h"
#include "callweaver/cli.h"
#include "callweaver/logger.h"
#include "callweaver/frame.h"
#include "callweaver/config.h"
#include "callweaver/module.h"
#include "callweaver/lock.h"
#include "callweaver/translate.h"
#include "callweaver/causes.h"
#include "callweaver/dsp.h"
#include "callweaver/transcap.h"
#include "callweaver.h"
#include "confdefs.h"


#ifdef CALLWEAVER_CWOBJ
#define ASTOBJ_COMPONENTS 		CWOBJ_COMPONENTS
#define ASTOBJ_CONTAINER_INIT		CWOBJ_CONTAINER_INIT
#define ASTOBJ_CONTAINER_COMPONENTS 	CWOBJ_CONTAINER_COMPONENTS
#define ASTOBJ_CONTAINER_DESTROY 	CWOBJ_CONTAINER_DESTROY
#define ASTOBJ_CONTAINER_DESTROYALL 	CWOBJ_CONTAINER_DESTROYALL
#define ASTOBJ_UNLOCK			CWOBJ_UNLOCK
#define ASTOBJ_RDLOCK			CWOBJ_RDLOCK
#define ASTOBJ_CONTAINER_UNLINK 	CWOBJ_CONTAINER_UNLINK
#define ASTOBJ_CONTAINER_TRAVERSE	CWOBJ_CONTAINER_TRAVERSE
#define ASTOBJ_CONTAINER_WRLOCK		CWOBJ_CONTAINER_WRLOCK
#define ASTOBJ_CONTAINER_UNLOCK		CWOBJ_CONTAINER_UNLOCK
#define ASTOBJ_CONTAINER_FIND		CWOBJ_CONTAINER_FIND
#define ASTOBJ_CONTAINER_LINK		CWOBJ_CONTAINER_LINK
#endif


#if defined (CW_CONTROL_RINGING) 
#define CALLWEAVER_19 1
#endif

CALLWEAVER_FILE_VERSION(__FILE__, "$Revision: 1.72 $")

#if defined(DSP_FEATURE_FAX_CNG_DETECT)
#undef		DSP_FEATURE_FAX_DETECT
#define		DSP_FEATURE_FAX_DETECT DSP_FEATURE_FAX_CNG_DETECT
#endif

#if !defined(macrocontext)
#undef		macrocontext
#define		macrocontext	proc_context
#endif

#if !defined(ast_tv)
#undef		ast_tv
#define		ast_tv		opbx_tv
#endif

/* CALLWEAVER v1.9 and later */
#if defined (CALLWEAVER_19) 
#define		ast_transfercapability2str			cw_transfercapability2str
#define		ast_config							cw_config
#define 	AST_CONTROL_RINGING					CW_CONTROL_RINGING
#define 	AST_CONTROL_BUSY					CW_CONTROL_BUSY
#define 	AST_CONTROL_CONGESTION				CW_CONTROL_CONGESTION
#define 	AST_CONTROL_PROCEEDING				CW_CONTROL_PROCEEDING
#define 	AST_CONTROL_PROGRESS				CW_CONTROL_PROGRESS
#define 	AST_CONTROL_HOLD					CW_CONTROL_HOLD
#define 	AST_CONTROL_UNHOLD					CW_CONTROL_UNHOLD
#define		AST_CONTROL_VIDUPDATE				CW_CONTROL_VIDUPDATE

#define 	AST_TRANS_CAP_SPEECH 				CW_TRANS_CAP_SPEECH
#define		AST_TRANS_CAP_3_1K_AUDIO			CW_TRANS_CAP_3_1K_AUDIO
#define		AST_TRANS_CAP_DIGITAL				CW_TRANS_CAP_DIGITAL
#define		AST_TRANS_CAP_RESTRICTED_DIGITAL 	CW_TRANS_CAP_RESTRICTED_DIGITAL
#define		AST_TRANS_CAP_DIGITAL_W_TONES		CW_TRANS_CAP_DIGITAL_W_TONES
#define		AST_TRANS_CAP_VIDEO					CW_TRANS_CAP_VIDEO

#define		AST_FORMAT_ADPCM					CW_FORMAT_DVI_ADPCM

#ifndef LOG_NOTICE
#define		LOG_NOTICE		CW_LOG_NOTICE
#endif

#ifndef LOG_DEBUG
#define		LOG_DEBUG		CW_LOG_DEBUG
#endif

#ifndef LOG_ERROR
#define		LOG_ERROR		CW_LOG_ERROR
#endif

#ifndef LOG_WARNING
#define		LOG_WARNING		CW_LOG_WARNING
#endif

#define 	AST_FORMAT_SLINEAR 	CW_FORMAT_SLINEAR
#define 	AST_FORMAT_ULAW		CW_FORMAT_ULAW
#define 	AST_FORMAT_ALAW 	CW_FORMAT_ALAW
#define 	ast_mutex_t 		cw_mutex_t
#define 	ast_frame 		cw_frame
#define 	ast_verbose 		cw_verbose
#define 	AST_FRIENDLY_OFFSET 	CW_FRIENDLY_OFFSET
#define 	AST_MUTEX_DEFINE_STATIC 	CW_MUTEX_DEFINE_STATIC
#define 	AST_CONTROL_PROGRESS 		CW_CONTROL_PROGRESS
#define 	AST_CAUSE_REQUESTED_CHAN_UNAVAIL 	CW_CAUSE_REQUESTED_CHAN_UNAVAIL
#define 	AST_CAUSE_NORMAL_CIRCUIT_CONGESTION 	CW_CAUSE_NORMAL_CIRCUIT_CONGESTION
#define 	AST_CAUSE_USER_BUSY 		CW_CAUSE_USER_BUSY
#define 	AST_CAUSE_NO_ANSWER 		CW_CAUSE_NO_ANSWER
#define 	AST_CAUSE_NORMAL_CLEARING 	CW_CAUSE_NORMAL_CLEARING
#define 	AST_SOFTHANGUP_EXPLICIT 	CW_SOFTHANGUP_EXPLICIT
#define 	AST_SOFTHANGUP_DEV 		CW_SOFTHANGUP_DEV
#define		AST_CAUSE_NORMAL_CLEARING 	CW_CAUSE_NORMAL_CLEARING
#define 	AST_FRAME_DTMF 			CW_FRAME_DTMF
#define 	AST_FRAME_CONTROL 		CW_FRAME_CONTROL
#define 	AST_CONTROL_ANSWER 		CW_CONTROL_ANSWER
#define 	AST_STATE_UP 			CW_STATE_UP
#define 	AST_STATE_RINGING 		CW_STATE_RINGING
#define 	AST_STATE_DOWN 			CW_STATE_DOWN
#define 	AST_FLAGS_ALL 			CW_FLAGS_ALL
#define 	AST_FRAME_VOICE 		CW_FRAME_VOICE
#define 	ASTERISK_GPL_KEY 		0
#define 	ast_channel_tech 		cw_channel_tech
#define 	ast_test_flag  			cw_test_flag
#define 	ast_queue_frame 		cw_queue_frame
#define		ast_frdup 			cw_frdup
#define 	ast_channel 			cw_channel
#define 	ast_exists_extension 		cw_exists_extension
#define 	ast_hostent 		cw_hostent
#define         ast_clear_flag          cw_clear_flag
#define         ast_log                 cw_log
#define         ast_set_flag            cw_set_flag
#define         ast_copy_string         cw_copy_string
#define         ast_set_flag            cw_set_flag
#define         ast_set2_flag           cw_set2_flag
#define         ast_setstate            cw_setstate
#define         ast_test_flag           cw_test_flag
#define         ast_softhangup          cw_softhangup
#define         ast_softhangup_nolock   cw_softhangup_nolock
#define         ast_true                cw_true
#define         ast_false               cw_false
#define         ast_strlen_zero         cw_strlen_zero
#define         ast_exists_extension    cw_exists_extension
#define         ast_frame               cw_frame
#define         ast_jb_conf             cw_jb_conf
#define         ast_carefulwrite        cw_carefulwrite
#define         ast_channel_unregister  cw_channel_unregister
#define         ast_cli                 cw_cli
#define         ast_cli_register        cw_cli_register
#define         ast_cli_unregister      cw_cli_unregister
#define         ast_jb_read_conf        cw_jb_read_conf
#define         ast_mutex_destroy       cw_mutex_destroy
#define         ast_mutex_init          cw_mutex_init
#define         ast_mutex_lock          cw_mutex_lock
#define         ast_mutex_unlock        cw_mutex_unlock
#define         ast_mutex_t             cw_mutex_t
#define         ast_queue_control       cw_queue_control
#define         ast_queue_frame         cw_queue_frame
#define         ast_queue_hangup        cw_queue_hangup
#define         ast_set_callerid        cw_set_callerid
#define         ast_variable            cw_variable
#define         ast_pthread_create      cw_pthread_create
#define         ast_cli_entry           cw_cli_entry
#define         ast_channel_register    cw_channel_register
#define         ast_config_load         cw_config_load
#define         ast_config_destroy      cw_config_destroy
#define         ast_category_browse     cw_category_browse
#define         ast_variable_browse     cw_variable_browse
#define         ast_gethostbyname       cw_gethostbyname
#define         ast_channel_alloc       cw_channel_alloc
#define         ast_dsp_new                     cw_dsp_new
#define         ast_dsp                         cw_dsp
#define         ast_dsp_set_features            cw_dsp_set_features
#define         ast_dsp_digitmode               cw_dsp_digitmode
#define         ast_dsp_set_call_progress_zone  cw_dsp_set_call_progress_zone
#define         ast_dsp_set_busy_count          cw_dsp_set_busy_count
#define         ast_dsp_set_busy_pattern        cw_dsp_set_busy_pattern
#define         ast_dsp_process                 cw_dsp_process
#define         ast_strdupa             cw_strdupa
#define         ast_mutex_trylock       cw_mutex_trylock
#define         ast_cause2str           cw_cause2str
#define         ast_pbx_start           cw_pbx_start
#define         ast_hangup	 	cw_hangup
#if !defined(ast_async_goto)
#undef		ast_async_goto 
#define		ast_async_goto		cw_async_goto_n
#endif
#else /* CALLWEAVER prior to v1.9 */

#define ast_transfercapability2str opbx_transfercapability2str

#define		ast_config				opbx_config
#define 	AST_CONTROL_RINGING		OPBX_CONTROL_RINGING
#define 	AST_CONTROL_BUSY		OPBX_CONTROL_BUSY
#define 	AST_CONTROL_CONGESTION	OPBX_CONTROL_CONGESTION
#define 	AST_CONTROL_PROCEEDING	OPBX_CONTROL_PROCEEDING
#define 	AST_CONTROL_PROGRESS	OPBX_CONTROL_PROGRESS
#define 	AST_CONTROL_HOLD		OPBX_CONTROL_HOLD
#define 	AST_CONTROL_UNHOLD		OPBX_CONTROL_UNHOLD
#define		AST_CONTROL_VIDUPDATE	OPBX_CONTROL_VIDUPDATE

#define 	AST_FORMAT_SLINEAR 		OPBX_FORMAT_SLINEAR
#define 	AST_FORMAT_ULAW			OPBX_FORMAT_ULAW
#define 	AST_FORMAT_ALAW 		OPBX_FORMAT_ALAW

#define 	AST_TRANS_CAP_SPEECH 				OPBX_TRANS_CAP_SPEECH
#define		AST_TRANS_CAP_3_1K_AUDIO			OPBX_TRANS_CAP_3_1K_AUDIO
#define		AST_TRANS_CAP_DIGITAL				OPBX_TRANS_CAP_DIGITAL
#define		AST_TRANS_CAP_RESTRICTED_DIGITAL 	OPBX_TRANS_CAP_RESTRICTED_DIGITAL
#define		AST_TRANS_CAP_DIGITAL_W_TONES		OPBX_TRANS_CAP_DIGITAL_W_TONES
#define		AST_TRANS_CAP_VIDEO					OPBX_TRANS_CAP_VIDEO
		
#ifndef LOG_NOTICE
#define		LOG_NOTICE		OPBX_LOG_NOTICE
#endif

#ifndef LOG_DEBUG
#define		LOG_DEBUG		OPBX_LOG_DEBUG
#endif

#ifndef LOG_ERROR
#define		LOG_ERROR		OPBX_LOG_ERROR
#endif

#ifndef LOG_WARNING
#define		LOG_WARNING		OPBX_LOG_WARNING
#endif


#define 	AST_FORMAT_SLINEAR 	OPBX_FORMAT_SLINEAR
#define 	AST_FORMAT_ULAW		OPBX_FORMAT_ULAW
#define 	AST_FORMAT_ALAW 	OPBX_FORMAT_ALAW
#define		AST_FORMAT_ADPCM	OPBX_FORMAT_DVI_ADPCM

#define 	ast_mutex_t 		opbx_mutex_t
#define 	ast_frame 		opbx_frame
#define 	ast_verbose 		opbx_verbose
#define 	AST_FRIENDLY_OFFSET 	OPBX_FRIENDLY_OFFSET
#define 	AST_MUTEX_DEFINE_STATIC 	OPBX_MUTEX_DEFINE_STATIC
#define 	AST_CONTROL_PROGRESS 		OPBX_CONTROL_PROGRESS
#define 	AST_CAUSE_REQUESTED_CHAN_UNAVAIL 	OPBX_CAUSE_REQUESTED_CHAN_UNAVAIL
#define 	AST_CAUSE_NORMAL_CIRCUIT_CONGESTION 	OPBX_CAUSE_NORMAL_CIRCUIT_CONGESTION
#define 	AST_CAUSE_USER_BUSY 		OPBX_CAUSE_USER_BUSY
#define 	AST_CAUSE_NO_ANSWER 		OPBX_CAUSE_NO_ANSWER
#define 	AST_CAUSE_NORMAL_CLEARING 	OPBX_CAUSE_NORMAL_CLEARING
#define 	AST_SOFTHANGUP_EXPLICIT 	OPBX_SOFTHANGUP_EXPLICIT
#define 	AST_SOFTHANGUP_DEV 		OPBX_SOFTHANGUP_DEV
#define		AST_CAUSE_NORMAL_CLEARING 	OPBX_CAUSE_NORMAL_CLEARING
#define 	AST_FRAME_DTMF 			OPBX_FRAME_DTMF
#define 	AST_FRAME_CONTROL 		OPBX_FRAME_CONTROL
#define 	AST_CONTROL_ANSWER 		OPBX_CONTROL_ANSWER
#define 	AST_STATE_UP 			OPBX_STATE_UP
#define 	AST_STATE_RINGING 		OPBX_STATE_RINGING
#define 	AST_STATE_DOWN 			OPBX_STATE_DOWN
#define 	AST_FLAGS_ALL 			OPBX_FLAGS_ALL
#define 	AST_FRAME_VOICE 		OPBX_FRAME_VOICE
#define 	AST_FRAME_NULL			OPBX_FRAME_NULL
#define 	ASTERISK_GPL_KEY 		0
#define 	ast_channel_tech 		opbx_channel_tech
#define 	ast_test_flag  			opbx_test_flag
#define 	ast_queue_frame 		opbx_queue_frame
#define		ast_frdup 			opbx_frdup
#define 	ast_channel 			opbx_channel
#define 	ast_exists_extension 		opbx_exists_extension
#define 	ast_hostent 		opbx_hostent
#define         ast_clear_flag          opbx_clear_flag
#define         ast_log                 opbx_log
#define         ast_set_flag            opbx_set_flag
#define         ast_copy_string         opbx_copy_string
#define         ast_set_flag            opbx_set_flag
#define         ast_set2_flag           opbx_set2_flag
#define         ast_setstate            opbx_setstate
#define         ast_test_flag           opbx_test_flag
#define         ast_softhangup          opbx_softhangup
#define         ast_softhangup_nolock   opbx_softhangup_nolock
#define         ast_true                opbx_true
#define         ast_false               opbx_false
#define         ast_strlen_zero         opbx_strlen_zero
#define         ast_exists_extension    opbx_exists_extension
#define         ast_frame               opbx_frame
#define         ast_jb_conf             opbx_jb_conf
#define         ast_carefulwrite        opbx_carefulwrite
#define         ast_channel_unregister  opbx_channel_unregister
#define         ast_cli                 opbx_cli
#define         ast_cli_register        opbx_cli_register
#define         ast_cli_unregister      opbx_cli_unregister
#define         ast_jb_read_conf        opbx_jb_read_conf
#define         ast_mutex_destroy       opbx_mutex_destroy
#define         ast_mutex_init          opbx_mutex_init
#define         ast_mutex_lock          opbx_mutex_lock
#define         ast_mutex_unlock        opbx_mutex_unlock
#define         ast_mutex_t             opbx_mutex_t
#define         ast_queue_control       opbx_queue_control
#define         ast_queue_frame         opbx_queue_frame
#define         ast_queue_hangup        opbx_queue_hangup
#define         ast_set_callerid        opbx_set_callerid
#define         ast_variable            opbx_variable
#define         ast_pthread_create      opbx_pthread_create
#define         ast_cli_entry           opbx_cli_entry
#define         ast_channel_register    opbx_channel_register
#define         ast_config_load         opbx_config_load
#define         ast_config_destroy      opbx_config_destroy
#define         ast_category_browse     opbx_category_browse
#define         ast_variable_browse     opbx_variable_browse
#define         ast_gethostbyname       opbx_gethostbyname
#define         ast_channel_alloc       opbx_channel_alloc
#define         ast_dsp_new                     opbx_dsp_new
#define         ast_dsp                         opbx_dsp
#define         ast_dsp_set_features            opbx_dsp_set_features
#define         ast_dsp_digitmode               opbx_dsp_digitmode
#define         ast_dsp_set_call_progress_zone  opbx_dsp_set_call_progress_zone
#define         ast_dsp_set_busy_count          opbx_dsp_set_busy_count
#define         ast_dsp_set_busy_pattern        opbx_dsp_set_busy_pattern
#define         ast_dsp_process                 opbx_dsp_process
#define         ast_strdupa             opbx_strdupa
#define         ast_mutex_trylock       opbx_mutex_trylock
#define         ast_cause2str           opbx_cause2str
#define         ast_pbx_start           opbx_pbx_start
#define         ast_hangup	 	opbx_hangup

#if !defined(ast_async_goto)
#undef		ast_async_goto 
#define		ast_async_goto		opbx_async_goto
#endif
		
#endif /* CALLWEAVER_19 */
#endif

#include "g711.h"
#include <errno.h>

#define USE_TECH_INDICATE 1
#ifdef  USE_TECH_INDICATE
	#define MEDIA_ANSWER "MEDIA"
#else
	#define MEDIA_ANSWER "ACCEPT"
#endif

#define USE_ANSWER 1
 
#ifndef ast_free
#define ast_free(x) free(x)
#define ast_malloc(x) malloc(x)
#endif


extern int option_verbose;

#define WOOMERA_VERSION "v1.72"
#ifndef WOOMERA_CHAN_NAME
#define WOOMERA_CHAN_NAME "SS7"
#endif

#if 1
#undef WOOMERA_PRINTF_DEBUG
#else
#warning "WOOMERA_PRINTF_DEBUG defined"
#define WOOMERA_PRINTF_DEBUG
#endif
#ifdef WOOMERA_PRINTF_DEBUG
#define woomera_printf(a,b,c,msg...)	__woomera_printf(__FUNCTION__,__LINE__,a,b,c,##msg)
#else
#define woomera_printf(a,b,c,msg...) __woomera_printf(a,b,c,##msg)
#endif

#define WOOMERA_MAX_CALLED_IGNORE 32 

static int tech_count = 0;

static const char desc[] = "Woomera Channel Driver";
//static const char type[] = "WOOMERA";
static const char tdesc[] = "Woomera Channel Driver";
static char configfile[] = "woomera.conf";

static char mohinterpret[MAX_MUSICCLASS] = "default";
static char mohsuggest[MAX_MUSICCLASS] = "";


/* Used to debug a specific channel */
static void *debug_tech_pvt=NULL;


#if !defined (AST14) && !defined (AST16)
struct ast_frame ast_null_frame;
#endif

#if defined (AST14) || defined (AST16)
 #if !defined (AST_JB)
  #define AST_JB 1
 #endif
#endif

#if !defined(DSP_FEATURE_DTMF_DETECT) && defined(DSP_FEATURE_DIGIT_DETECT) 
#define DSP_FEATURE_DTMF_DETECT DSP_FEATURE_DIGIT_DETECT
#define ast_dsp_digitmode ast_dsp_set_digitmode
#define woo_ast_data_ptr data.ptr
#else
#define woo_ast_data_ptr data
#endif


#if defined (AST_JB)
#include "asterisk/abstract_jb.h"
/* Global jitterbuffer configuration - by default, jb is disabled */
static struct ast_jb_conf default_jbconf =
{
        .flags = 0,
        .max_size = -1,
        .resync_threshold = -1,
        .impl = ""
};
static struct ast_jb_conf global_jbconf;
#endif /* AST_JB */


#define WOOMERA_SLINEAR 0
#define WOOMERA_ULAW	1
#define WOOMERA_ALAW	2

#define WOOMERA_MAX_MEDIA_PORTS 5000

#define WOOMERA_STRLEN 256
#define WOOMERA_ARRAY_LEN 50
#define WOOMERA_MIN_PORT  20000
#define WOOMERA_MAX_PORT WOOMERA_MIN_PORT+WOOMERA_MAX_MEDIA_PORTS
#define WOOMERA_BODYLEN 2048
#define WOOMERA_LINE_SEPARATOR "\r\n"
#define WOOMERA_RECORD_SEPARATOR "\r\n\r\n"
#define WOOMERA_DEBUG_PREFIX "**[WOOMERA]** "
#define WOOMERA_DEBUG_LINE "--------------------------------------------------------------------------------" 
#define WOOMERA_HARD_TIMEOUT -2000
#define WOOMERA_QLEN 10
#define WOOMERA_MAX_TRUNKGROUPS 64

static int woomera_base_media_port = WOOMERA_MIN_PORT;
static int woomera_max_media_port  = WOOMERA_MAX_PORT;

/* this macro is not in all versions of asterisk */
#ifdef OLDERAST
#define ASTOBJ_CONTAINER_UNLINK(container,obj) \
        ({ \
                typeof((container)->head) found = NULL; \
                typeof((container)->head) prev = NULL; \
                ASTOBJ_CONTAINER_TRAVERSE(container, !found, do { \
                        if (iterator== obj) { \
                                found = iterator; \
                                found->next[0] = NULL; \
                                ASTOBJ_CONTAINER_WRLOCK(container); \
                                if (prev) \
                                        prev->next[0] = next; \
                                else \
                                        (container)->head = next; \
                                ASTOBJ_CONTAINER_UNLOCK(container); \
                        } \
                        prev = iterator; \
                } while (0)); \
                found; \
        }) 
#endif

#define FRAME_LEN 480

#if 0
static int WFORMAT = AST_FORMAT_ALAW; //AST_FORMAT_SLINEAR;
#else
static int WFORMAT = AST_FORMAT_SLINEAR;
#endif

typedef enum {
	WFLAG_EXISTS = (1 << 0),
	WFLAG_EVENT = (1 << 1),
	WFLAG_CONTENT = (1 << 2),
} WFLAGS;


typedef enum {
	WCFLAG_NOWAIT = (1 << 0)
} WCFLAGS;


typedef enum {
	PFLAG_INBOUND = (1 << 0),
	PFLAG_OUTBOUND = (1 << 1),
	PFLAG_DYNAMIC = (1 << 2),
	PFLAG_DISABLED = (1 << 3)
} PFLAGS;

typedef enum {
	TFLAG_MEDIA = (1 << 0),
	TFLAG_INBOUND = (1 << 1),
	TFLAG_OUTBOUND = (1 << 2),
	TFLAG_INCOMING = (1 << 3),
	TFLAG_PARSE_INCOMING = (1 << 4),
	TFLAG_ACTIVATE = (1 << 5),
	TFLAG_DTMF = (1 << 6),
	TFLAG_DESTROY = (1 << 7),
	TFLAG_ABORT = (1 << 8),
	TFLAG_PBX = (1 << 9),
	TFLAG_ANSWER = (1 << 10),
	TFLAG_INTHREAD = (1 << 11),
	TFLAG_TECHHANGUP = (1 << 12),
	TFLAG_DESTROYED = (1 << 13),
	TFLAG_UP = (1 << 14),
	TFLAG_ACCEPT = (1 << 15),
	TFLAG_ACCEPTED = (1 << 16),
	TFLAG_ANSWER_RECEIVED = (1 << 17),
	TFLAG_CONFIRM_ANSWER = (1 << 18),
	TFLAG_CONFIRM_ANSWER_ENABLED = (1 << 19),
	TFLAG_AST_HANGUP = (1 << 20),
	TFLAG_PROGRESS = (1 << 21),
	TFLAG_RBS	= (1 << 22),
	TFLAG_MEDIA_RELAY	= (1 << 23),
	TFLAG_MEDIA_AVAIL = (1 << 24),
	TFLAG_RING_AVAIL = (1 << 25),
} TFLAGS;

static int usecnt = 0;

struct woomera_message {
	char callid[WOOMERA_STRLEN];
	int mval;
	char command[WOOMERA_STRLEN];
	char command_args[WOOMERA_STRLEN];
	char names[WOOMERA_ARRAY_LEN][WOOMERA_STRLEN];
	char values[WOOMERA_ARRAY_LEN][WOOMERA_STRLEN];
	char body[WOOMERA_BODYLEN];
	char cause[WOOMERA_STRLEN]; 
	unsigned int flags;
	int last;
	unsigned int queue_id;
	struct woomera_message *next;
};


static struct {
	int next_woomera_port;	
	int debug;
	int panic;
	int more_threads;
	ast_mutex_t woomera_port_lock;
} globals;

struct woomera_event_queue {
	struct woomera_message *head;
	ast_mutex_t lock;
};

struct woomera_profile {
	ASTOBJ_COMPONENTS(struct woomera_profile);
	ast_mutex_t iolock;
	ast_mutex_t call_count_lock;
	char woomera_host[WOOMERA_STRLEN];
	int max_calls;
	int call_count;
	int woomera_port;
	char audio_ip[WOOMERA_STRLEN];
	char context[WOOMERA_STRLEN];
	struct {
		int pres;
		int val;
	} cid_pres;
	pthread_t thread;
	unsigned int flags;
	int thread_running;
	int dtmf_enable;
	int faxdetect;
	int woomera_socket;
	struct woomera_event_queue event_queue;
	int jb_enable;
	int progress_enable;
	int progress_on_accept;
	int coding;
	float rxgain_val;
	float txgain_val;
	unsigned char rxgain[256];
	unsigned char txgain[256];
	unsigned int call_out;
	unsigned int call_in;
	unsigned int call_ok;
	unsigned int call_end;
	unsigned int call_abort;
	unsigned int call_ast_hungup;
	unsigned int media_rx_seq_err;
	unsigned int media_tx_seq_err;
	char default_context[WOOMERA_STRLEN];
	char* tg_context [WOOMERA_MAX_TRUNKGROUPS+1];
	char language[WOOMERA_STRLEN];
	char* tg_language [WOOMERA_MAX_TRUNKGROUPS+1];
	int udp_seq;
	int rx_sync_check_opt;
	int tx_sync_check_opt;
	int tx_sync_gen_opt;
	unsigned int verbose;
	char called_ignore[WOOMERA_MAX_CALLED_IGNORE][WOOMERA_STRLEN];
	int called_ignore_idx;
	unsigned char rbs_relay;
	unsigned char bridge_disable;
	unsigned char media_pass_through;
	char smgversion[100];
};


struct private_object {
	ASTOBJ_COMPONENTS(struct private_object);
	ast_mutex_t iolock;
	struct ast_channel *owner;
	struct sockaddr_in udpread;
	struct sockaddr_in udpwrite;
	int command_channel;
	int udp_socket;
	unsigned int flags;
	struct ast_frame frame;
	short fdata[FRAME_LEN + AST_FRIENDLY_OFFSET];
	struct woomera_message call_info;
	struct woomera_message media_info;
	char *woomera_relay;
	struct woomera_profile *profile;
	char dest[WOOMERA_STRLEN]; 
	char proto[WOOMERA_STRLEN];
	int port;
	struct timeval started;
	int timeout;
	char dtmfbuf[WOOMERA_STRLEN];
	unsigned char rbsbuf;
	char cid_name[WOOMERA_STRLEN];
	char cid_num[WOOMERA_STRLEN];
	char mohinterpret[MAX_MUSICCLASS];
	char mohsuggest[MAX_MUSICCLASS];
	char *cid_rdnis;
	int  cid_pres;
	char ds[WOOMERA_STRLEN];
	struct ast_dsp *dsp;  
	int ast_dsp;
	int dsp_features;
	int faxdetect;
	int faxhandled;
	int call_count;
	char callid[WOOMERA_STRLEN];
	pthread_t thread;
	unsigned int callno;
	int refcnt;
	struct woomera_event_queue event_queue;
	int coding;
	int pri_cause;
	unsigned int rx_udp_seq;
	unsigned int tx_udp_seq;
#ifdef AST_JB
        struct ast_jb_conf jbconf;
#endif /* AST_JB */

	int sync_r;
	int sync_w;
	unsigned char sync_data_w;
	unsigned char sync_data_r;
	int capability;
	int bridge;
	int ignore_dtmf;
	struct ast_frame rbs_frame;

};

typedef struct private_object private_object;
typedef struct woomera_message woomera_message;
typedef struct woomera_profile woomera_profile;
typedef struct woomera_event_queue woomera_event_queue;

#ifndef DEADLOCK_AVOIDANCE 
#define ast_find_lock_info(a,b,c,d,e,f,g,h) -1
#define DEADLOCK_AVOIDANCE(lock) \
        do { \
                ast_mutex_unlock(lock); \
                usleep(1); \
                ast_mutex_lock(lock); \
        } while (0)
#endif

static inline int validate_number(char *s)
{
	char *p;
	for (p = s; *p; p++) {
		if (*p < 48 || *p > 57) {
			return -1;
		}
	}
	return 0;
}



static inline int woomera_profile_verbose(woomera_profile *profile)
{
	if (!profile) {
		/* If profile does not exists then log by default */
		return 100;
	}

	return profile->verbose;
}


static int my_ast_channel_trylock(struct ast_channel *chan)
{
#if defined (AST14) || defined (AST16)
	return ast_channel_trylock(chan);
#else
	return ast_mutex_trylock(&chan->lock);
#endif
}

#if !defined (CALLWEAVER)
#if 0
static int my_ast_channel_lock(struct ast_channel *chan)
{
#if defined (AST14) || defined (AST16)
	return ast_channel_lock(chan);
#else
	return ast_mutex_lock(&chan->lock);
#endif
}
#endif
#endif

static int my_ast_channel_unlock(struct ast_channel *chan)
{
#if defined (AST14) || defined (AST16)
	return ast_channel_unlock(chan);
#else
	return ast_mutex_unlock(&chan->lock);
#endif
}

static int my_tech_pvt_and_owner_lock(private_object *tech_pvt)
{
	ast_mutex_lock(&tech_pvt->iolock);
        while(tech_pvt->owner && my_ast_channel_trylock(tech_pvt->owner)) {
        	if (globals.debug > 2) {
                	ast_log(LOG_NOTICE,"Tech Thrad - Hanging up channel - deadlock avoidance\n");
		}
                DEADLOCK_AVOIDANCE(&tech_pvt->iolock);
        }
	return 0;
}

static int my_tech_pvt_and_owner_unlock(private_object *tech_pvt)
{

	if (tech_pvt->owner) {
		my_ast_channel_unlock(tech_pvt->owner);	
	}
	ast_mutex_unlock(&tech_pvt->iolock);
	return 0;
}

static void my_ast_softhangup(struct ast_channel *chan, private_object *tech_pvt, int cause)
{
#if 1
	ast_queue_hangup(chan);
	//ast_softhangup(chan,  AST_SOFTHANGUP_EXPLICIT);
#else
	struct ast_frame f = { AST_FRAME_NULL };

	if (chan) {
		chan->_softhangup |= cause;
		ast_queue_frame(chan, &f);
	}
	
	ast_set_flag(tech_pvt, TFLAG_ABORT);
#endif

#if 0
	if (tech_pvt->dsp) {
		tech_pvt->dsp_features &= ~DSP_FEATURE_DTMF_DETECT;
		ast_dsp_set_features(tech_pvt->dsp, tech_pvt->dsp_features); 
		tech_pvt->ast_dsp=0;
	}
#endif

}

//static struct sched_context *sched;

static struct private_object_container {
    ASTOBJ_CONTAINER_COMPONENTS(private_object);
} private_object_list;

static struct woomera_profile_container {
    ASTOBJ_CONTAINER_COMPONENTS(woomera_profile);
} woomera_profile_list;

static woomera_profile default_profile;

/* some locks you will use for use count and for exclusive access to the main linked-list of private objects */
AST_MUTEX_DEFINE_STATIC(usecnt_lock);
AST_MUTEX_DEFINE_STATIC(lock);

/* local prototypes */
static void woomera_close_socket(int *socket);
static void global_set_flag(woomera_profile *profile,int flags);
#ifdef WOOMERA_PRINTF_DEBUG
static int __woomera_printf(const char* file, int line, woomera_profile *profile, int fd, char *fmt, ...);
#else
static int __woomera_printf(woomera_profile *profile, int fd, char *fmt, ...);
#endif
static char *woomera_message_header(woomera_message *wmsg, char *key);
static int woomera_enqueue_event(woomera_event_queue *event_queue, woomera_message *wmsg);
static int woomera_dequeue_event(woomera_event_queue *event_queue, woomera_message *wmsg);
static int woomera_message_parse(int fd, woomera_message *wmsg, int timeout, woomera_profile *profile, woomera_event_queue *event_queue);
static int woomera_message_parse_wait(private_object *tech_pvt,woomera_message *wmsg);
static int waitfor_socket(int fd, int timeout);
static int woomera_profile_thread_running(woomera_profile *profile, int set, int new);
static int woomera_locate_socket(woomera_profile *profile, int *woomera_socket);
static void *woomera_thread_run(void *obj); 
static void launch_woomera_thread(woomera_profile *profile);
static void destroy_woomera_profile(woomera_profile *profile); 
static woomera_profile *clone_woomera_profile(woomera_profile *new_profile, woomera_profile *default_profile);
static woomera_profile *create_woomera_profile(woomera_profile *default_profile);
static int config_woomera(void);
static int create_udp_socket(char *ip, int port, struct sockaddr_in *sockaddr, int client);
static int connect_woomera(int *new_socket, woomera_profile *profile, int flags);
static int init_woomera(void);
#if defined (AST16)
static char *ast16_woomera_cli(struct ast_cli_entry *e, int cmd, struct ast_cli_args *a);
#endif
static int woomera_cli(int fd, int argc, char *argv[]);
static void tech_destroy(private_object *tech_pvt, struct ast_channel *owner);
static struct ast_channel *woomera_new(const char *type, int format, void *data, int *cause, woomera_profile *profile);
static int launch_tech_thread(private_object *tech_pvt);
static int tech_create_read_socket(private_object *tech_pvt);
static int tech_activate(private_object *tech_pvt);
static int tech_init(private_object *tech_pvt, woomera_profile *profile, int flags);
static void *tech_monitor_thread(void *obj);
static void tech_monitor_in_one_thread(void);
static struct ast_channel *  tech_get_owner( private_object *tech_pvt);
int usecount(void);
#if 0
static char *key(void);
static char *description(void);
#endif
int load_module(void);
int unload_module(void);
int reload(void);


/********************CHANNEL METHOD PROTOTYPES*******************
 * You may or may not need all of these methods, remove any unnecessary functions/protos/mappings as needed.
 *
 */

#if defined (AST18)
static struct ast_channel *tech_requester(const char *type, format_t format, const struct ast_channel *requestor, void *data, int *cause);
#else
static struct ast_channel *tech_requester(const char *type, int format, void *data, int *cause);
#endif

static int tech_send_digit(struct ast_channel *self, char digit);
#if defined (AST14) || defined (AST16)
static int tech_digit_end(struct ast_channel *ast, char digit, unsigned int duration);
#endif
static int tech_call(struct ast_channel *self, char *dest, int timeout);
static int tech_hangup(struct ast_channel *self);
static int tech_answer(struct ast_channel *self);
static struct ast_frame *tech_read(struct ast_channel *self);
static struct ast_frame *tech_exception(struct ast_channel *self);
static int tech_write(struct ast_channel *self, struct ast_frame *frame);
#ifdef  USE_TECH_INDICATE
# if defined (AST14) || defined (AST16)
static int tech_indicate(struct ast_channel *self, int condition, const void *data, size_t datalen);
# else
static int tech_indicate(struct ast_channel *self, int condition);
# endif
#endif
static int tech_fixup(struct ast_channel *oldchan, struct ast_channel *newchan);
static int tech_send_html(struct ast_channel *self, int subclass, const char *data, int datalen);
static int tech_send_text(struct ast_channel *self, const char *text);
static int tech_send_image(struct ast_channel *self, struct ast_frame *frame);
static int tech_setoption(struct ast_channel *self, int option, void *data, int datalen);
static int tech_queryoption(struct ast_channel *self, int option, void *data, int *datalen);
static enum ast_bridge_result tech_bridge(struct ast_channel *c0, struct ast_channel *c1, int flags, struct ast_frame **fo, struct ast_channel **rc, int timeoutms);
static int tech_transfer(struct ast_channel *self, const char *newdest);
static int tech_write_video(struct ast_channel *self, struct ast_frame *frame);
//static struct ast_channel *tech_bridged_channel(struct ast_channel *self, struct ast_channel *bridge);

static int woomera_event_incoming (private_object *tech_pvt);
static int woomera_event_media (private_object *tech_pvt, woomera_message *wmsg);
static void woomera_check_event (private_object *tech_pvt, int res, woomera_message *wmsg);

/********************************************************************************
 * Constant structure for mapping local methods to the core interface.
 * This structure only needs to contain the methods the channel requires to operate
 * Not every channel needs all of them defined.
 */

static const struct ast_channel_tech technology = {
	.type = WOOMERA_CHAN_NAME,
	.description = tdesc,
	.capabilities = (AST_FORMAT_SLINEAR | AST_FORMAT_ULAW | AST_FORMAT_ALAW),
	.requester = tech_requester,
#if defined (AST14) || defined (AST16)
	.send_digit_begin = tech_send_digit,
	.send_digit_end = tech_digit_end,
#else
	.send_digit = tech_send_digit,
#endif
	.call = tech_call,
	.bridge = tech_bridge,
	.hangup = tech_hangup,
	.answer = tech_answer,
	.transfer = tech_transfer,
	.write_video = tech_write_video,
	.read = tech_read,
	.write = tech_write,
	.exception = tech_exception,
#ifdef  USE_TECH_INDICATE
	.indicate = tech_indicate,
#endif
	.fixup = tech_fixup,
	.send_html = tech_send_html,
	.send_text = tech_send_text,
	.send_image = tech_send_image,
	.setoption = tech_setoption,
	.queryoption = tech_queryoption,
	//.bridged_channel = tech_bridged_channel,
	.transfer = tech_transfer,
};



static void woomera_close_socket(int *socket) 
{

	if (*socket > -1) {
		close(*socket);
	}
	*socket = -1;
}

static int woomera_message_reply_ok(woomera_message *wmsg) 
{

       if (!(wmsg->mval >= 200 && wmsg->mval <= 299)) {
               return -1;
       }

       return 0;
}


static void global_set_flag(woomera_profile * profile, int flags)
{
	private_object *tech_pvt;

	ASTOBJ_CONTAINER_TRAVERSE(&private_object_list, 1, do {
		ASTOBJ_RDLOCK(iterator);
        	tech_pvt = iterator;
		if (profile == NULL || tech_pvt->profile == profile ) {
			ast_set_flag(tech_pvt, flags);
		}
		ASTOBJ_UNLOCK(iterator);
    } while(0));
} 

static void woomera_send_progress(private_object *tech_pvt)
{
	struct ast_channel *owner = tech_pvt->owner;

 	if (tech_pvt->profile->progress_enable && owner){
        	if (globals.debug > 2) {
                	ast_log(LOG_NOTICE, "Sending Progress %s\n",tech_pvt->callid);
                }

              	ast_queue_control(owner,
                                AST_CONTROL_PROGRESS);
        }
}


static int  woomera_coding_to_ast(char *suil1p)
{
	if (suil1p) {
		if (!strcasecmp(suil1p, "G_711_ULAW")) {
			return AST_FORMAT_ULAW;
		}
		if (!strcasecmp(suil1p, "G_711_ALAW")) {
			return AST_FORMAT_ALAW;
		}
		if (!strcasecmp(suil1p, "G_721")) {
			return AST_FORMAT_ADPCM;
		}
	}
	return -1;
}

static char* woomera_ast_coding_to_string(int coding)
{
	switch(coding) {
		case AST_FORMAT_ALAW:
			return "G_711_ALAW";
		case AST_FORMAT_ULAW:
			return "G_711_ULAW";
		case AST_FORMAT_ADPCM:
			return "G_721";
	}
	return "G_711_ALAW";
}

static uint32_t woomera_capability_to_ast(char *sbearer_cap)
{
	if (sbearer_cap) {
		if (!strcasecmp(sbearer_cap, "SPEECH")) {
			return AST_TRANS_CAP_SPEECH;
		}
		if (!strcasecmp(sbearer_cap, "3_1KHZ_AUDIO")) {
			return AST_TRANS_CAP_3_1K_AUDIO;
		} 
		if (!strcasecmp(sbearer_cap, "64K_UNRESTRICTED") ||
		    !strcasecmp(sbearer_cap, "ALTERNATE_SPEECH") ||
		    !strcasecmp(sbearer_cap, "ALTERNATE_64K") ||
		    !strcasecmp(sbearer_cap, "64K_PREFERRED") ||
		    !strcasecmp(sbearer_cap, "384K_UNRESTRICTED") ||
		    !strcasecmp(sbearer_cap, "2x64K_PREFERRED") ||
		    !strcasecmp(sbearer_cap, "1536K_UNRESTRICTED") ||
		    !strcasecmp(sbearer_cap, "1920K_UNRESTRICTED")) {
		
			return AST_TRANS_CAP_DIGITAL;
		}
	}
	return AST_TRANS_CAP_SPEECH;
}

static char* woomera_ast_transfercap_to_string(int transfercap)
{
	switch(transfercap) {
		case AST_TRANS_CAP_SPEECH:
			return "SPEECH";
		case AST_TRANS_CAP_3_1K_AUDIO:
			return "3_1KHZ_AUDIO";
		case AST_TRANS_CAP_DIGITAL:
		case AST_TRANS_CAP_RESTRICTED_DIGITAL:
		case AST_TRANS_CAP_DIGITAL_W_TONES:
		case AST_TRANS_CAP_VIDEO:      
			return "64K_UNRESTRICTED";
	}

	return "SPEECH";
}


static uint32_t string_to_release(char *code)
{
	if (code) {
		if (!strcasecmp(code, "CHANUNAVAIL")) {
			return AST_CAUSE_REQUESTED_CHAN_UNAVAIL;
		}

		if (!strcasecmp(code, "INVALID")) {
			return AST_CAUSE_NORMAL_CIRCUIT_CONGESTION;
		}

		if (!strcasecmp(code, "ERROR")) {
			return AST_CAUSE_NORMAL_CIRCUIT_CONGESTION;
		}

		if (!strcasecmp(code, "CONGESTION")) {
			return AST_CAUSE_NORMAL_CIRCUIT_CONGESTION;
		}

		if (!strcasecmp(code, "BUSY")) {
			return AST_CAUSE_USER_BUSY;
		}

		if (!strcasecmp(code, "NOANSWER")) {
			return AST_CAUSE_NO_ANSWER;
		}

		if (!strcasecmp(code, "ANSWER")) {
			return AST_CAUSE_NORMAL_CLEARING;
		}

		if (!strcasecmp(code, "CANCEL")) {
			return AST_CAUSE_NORMAL_CIRCUIT_CONGESTION;
		}

		if (!strcasecmp(code, "UNKNOWN")) {
			return AST_CAUSE_NORMAL_CIRCUIT_CONGESTION;
		} 
	}
	return AST_CAUSE_NORMAL_CLEARING;
}

#ifdef WOOMERA_PRINTF_DEBUG
static int __woomera_printf(const char* file, int line, woomera_profile *profile, int fd, char *fmt, ...)
#else
static int __woomera_printf(woomera_profile *profile, int fd, char *fmt, ...)
#endif
{
    char *stuff;
    int res = 0;

	if (fd < 0) {
		if (globals.debug > 4) {
			ast_log(LOG_ERROR, "Not gonna write to fd %d\n", fd);
		}
		return -1;
	}
	
    va_list ap;
    va_start(ap, fmt);
#ifdef SOLARIS
	stuff = (char *)ast_malloc(10240);
	vsnprintf(stuff, 10240, fmt, ap);
#else
    res = vasprintf(&stuff, fmt, ap);
#endif
    va_end(ap);
    if (res == -1) {
        ast_log(LOG_ERROR, "Out of memory\n");
    } else {
		res=0;
		if (profile && globals.debug) {
			if (option_verbose > 2 && woomera_profile_verbose(profile) > 2) {
			ast_verbose(WOOMERA_DEBUG_PREFIX "Send Message: {%s} [%s/%d]\n%s\n%s", profile->name, profile->woomera_host, profile->woomera_port, WOOMERA_DEBUG_LINE, stuff);
			}
		}
        res=ast_carefulwrite(fd, stuff, strlen(stuff), 100);
#ifdef WOOMERA_PRINTF_DEBUG
		if (res < 0) {
			ast_log(LOG_ERROR,"%s:%d FAILED fd=%i %s\n",file,line,fd,strerror(errno));
		}
#endif
        ast_free(stuff);
    }



	return res;
}

static char *woomera_message_header(woomera_message *wmsg, char *key) 
{
	int x = 0;
	char *value = NULL;


#if 0
	if (!strcasecmp(wmsg->command,"HANGUP")) {
	ast_log(LOG_NOTICE, "Message Header for HANGUP\n");
	for (x = 0 ; x < wmsg->last ; x++) {
		ast_log(LOG_NOTICE, "Name=%s Value=%s\n",
				wmsg->names[x],wmsg->values[x]);
		if (!strcasecmp(wmsg->names[x], key)) {
			value = wmsg->values[x];
			break;
		}
	}
	}
#endif

	for (x = 0 ; x < wmsg->last ; x++) {
		if (!strcasecmp(wmsg->names[x], key)) {
			value = wmsg->values[x];
			break;
		}
	}

	return value;
}

static int woomera_enqueue_event(woomera_event_queue *event_queue, woomera_message *wmsg)
{
	woomera_message *new, *mptr;

	if ((new = ast_malloc(sizeof(woomera_message)))) {
		ast_mutex_lock(&event_queue->lock);
		memcpy(new, wmsg, sizeof(woomera_message));
		new->next = NULL;
		if (!event_queue->head) {
			event_queue->head = new;
		} else {
			for (mptr = event_queue->head; mptr && mptr->next ; mptr = mptr->next);
			mptr->next = new;
		}
		ast_mutex_unlock(&event_queue->lock);
		return 1;
	} else {
		ast_log(LOG_ERROR, "Memory Allocation Error!\n");
	}

	return 0;
}

static int woomera_dequeue_event(woomera_event_queue *event_queue, woomera_message *wmsg)
{
	woomera_message *mptr = NULL;
	
	ast_mutex_lock(&event_queue->lock);
	if (event_queue->head) {
		mptr = event_queue->head;
		event_queue->head = mptr->next;
	}

	if (mptr) {
		memcpy(wmsg, mptr, sizeof(woomera_message));
	}
	ast_mutex_unlock(&event_queue->lock);

	if (mptr){
		ast_free(mptr);
		return 1;
	} else {
		memset(wmsg, 0, sizeof(woomera_message));
	}
	
	return 0;
}

#if 0
#include <execinfo.h>
static void print_trace(void)
{
  int i;
  void *stacktrace[100];
  size_t size;
  char **symbols;

  size = backtrace(stacktrace, sizeof(stacktrace)/sizeof(stacktrace[1]));
  symbols = backtrace_symbols(stacktrace, size);

  for (i=0;i<size;i++) {
     ast_log(LOG_ERROR,"%lu, %s\n", pthread_self(), symbols[i]);

  }

  if (symbols)
        free(symbols);

}
#endif


static int woomera_message_parse(int fd, woomera_message *wmsg, int timeout, 
                                 woomera_profile *profile, woomera_event_queue *event_queue) 
{
	char *cur, *cr, *next = NULL, *eor = NULL;
	char buf[2048];
	int res = 0, bytes = 0, sanity = 0;
	struct timeval started, ended;
	int elapsed, loops = 0;
	int failto = 0;

	memset(wmsg, 0, sizeof(woomera_message));

	if (fd < 0) {
		return -1;
	}

	gettimeofday(&started, NULL);
	memset(buf, 0, sizeof(buf));

	if (timeout < 0) {
		timeout = abs(timeout);
		failto = 1;
	} else if(timeout == 0) {
		timeout = -1;
	}

	while (!(eor = strstr(buf, WOOMERA_RECORD_SEPARATOR))) {

		if (!profile->thread_running) {
			return -1;
		}

		if (globals.panic > 2) {
			return -1;
		}

		/* Keep things moving.
		   Stupid Sockets -Homer Simpson */
		res=woomera_printf(NULL, fd, "%s", WOOMERA_RECORD_SEPARATOR);
		if (res < 0) {
			return -1;
		}

		if((res = waitfor_socket(fd, (timeout > 0 ? timeout : 100)) > 0)) {
			res = recv(fd, buf, sizeof(buf), MSG_PEEK);
			if (res == 0) {
				sanity++;
			} else if (res < 0) {
				if (option_verbose > 2 && woomera_profile_verbose(profile) > 2) {
				ast_verbose(WOOMERA_DEBUG_PREFIX "{%s} error during packet retry #%d\n", profile->name, loops);
				}
				return res;
			} else if (loops && globals.debug) {
				//ast_verbose(WOOMERA_DEBUG_PREFIX "{%s} Didnt get complete packet retry #%d\n", profile->name, loops);
				res=woomera_printf(NULL, fd, "%s", WOOMERA_RECORD_SEPARATOR);
				if (res < 0) {
					return res;
				}
				usleep(100);
			}

			if (res > 0) {
				sanity=0;
			}

		}

		gettimeofday(&ended, NULL);
		elapsed = (((ended.tv_sec * 1000) + ended.tv_usec / 1000) - ((started.tv_sec * 1000) + started.tv_usec / 1000));

		if (res < 0) {
			return res;
		}

		if (sanity > 1000) {
			if (globals.debug > 2) {
				ast_log(LOG_ERROR, "{%s} Failed Sanity Check! [errors] chfd=%d\n", 
					profile->name,fd);
			}
			return -100;
		}

		if (timeout > 0 && (elapsed > timeout)) {
			return failto ? -1 : 0;
		}
		
		loops++;
	}
	*eor = '\0';
	bytes = strlen(buf) + 4;
	
	memset(buf, 0, sizeof(buf));
	res = read(fd, buf, bytes);
	next = buf;

	if (globals.debug) {
		if (option_verbose > 2 && woomera_profile_verbose(profile) > 2) {
		ast_verbose(WOOMERA_DEBUG_PREFIX "Receive Message: {%s} [%s/%d]\n%s\n%s", profile->name, profile->woomera_host, profile->woomera_port, WOOMERA_DEBUG_LINE, buf);
		}
	}

	while((cur = next)) {
		if ((cr = strstr(cur, WOOMERA_LINE_SEPARATOR))) {
			*cr = '\0';
			next = cr + (sizeof(WOOMERA_LINE_SEPARATOR) - 1);
			if (!strcmp(next, WOOMERA_RECORD_SEPARATOR)) {
				break;
			}
		} 

		if (ast_strlen_zero(cur)) {
			break;
		}

		if (!wmsg->last) {
			ast_set_flag(wmsg, WFLAG_EXISTS);
			if (!strncasecmp(cur, "EVENT", 5)) {
				cur += 6;
				ast_set_flag(wmsg, WFLAG_EVENT);

				if (cur && (cr = strchr(cur, ' '))) {
					char *id;

					*cr = '\0';
					cr++;
					id = cr;
					if (cr && (cr = strchr(cr, ' '))) {
						*cr = '\0';
						cr++;
						strncpy(wmsg->command_args, cr, WOOMERA_STRLEN);
					}
					if(id) {
						ast_copy_string(wmsg->callid, id, sizeof(wmsg->callid));
					}
				}
			} else {
				if (cur && (cur = strchr(cur, ' '))) {
					*cur = '\0';
					cur++;
					wmsg->mval = atoi(buf);
				} else {
					ast_log(LOG_NOTICE, "Malformed Message!\n");
					break;
				}
			}
			if (cur) {
				strncpy(wmsg->command, cur, WOOMERA_STRLEN);
			} else {
				ast_log(LOG_NOTICE, "Malformed Message!\n");
				break;
			}
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
				ast_set_flag(wmsg, WFLAG_CONTENT);
				bytes = atoi(val);
			}

			if (name && val && !strcasecmp(name, "content-length")) {
                                ast_set_flag(wmsg, WFLAG_CONTENT);
                                bytes = atoi(val);
                        }


		}
		wmsg->last++;

		if (wmsg->last >= WOOMERA_ARRAY_LEN) {
			ast_log(LOG_NOTICE, "Woomera parse error: index overflow!\n");
			break;
		}
	}

	wmsg->last--;

	if (bytes && ast_test_flag(wmsg, WFLAG_CONTENT)) {
		int terr;
		terr=read(fd, wmsg->body, (bytes > sizeof(wmsg->body)) ? sizeof(wmsg->body) : bytes);
		if (globals.debug) {
			if (option_verbose > 2 && woomera_profile_verbose(profile) > 2) {
				ast_verbose("%s\n", wmsg->body);
			}
		}
	}

	if (event_queue && ast_test_flag(wmsg, WFLAG_EVENT)) {
		if (globals.debug) {
			if (option_verbose > 2 && woomera_profile_verbose(profile) > 2) {
				ast_verbose(WOOMERA_DEBUG_PREFIX "Queue Event: {%s} [%s]\n", profile->name, wmsg->command);
			}
		}
		/* we don't want events we want a reply so we will stash them for later */
		woomera_enqueue_event(event_queue, wmsg);

		/* call ourself recursively to find the reply. we'll keep doing this as long we get events.
		 * wmsg will be overwritten but it's ok we just queued it.
		 */
		return woomera_message_parse(fd, wmsg, timeout, profile, event_queue);
		
	}else if (wmsg->mval > 99 && wmsg->mval < 200) {
		/* reply in the 100's are nice but we need to wait for another reply 
		   call ourself recursively to find the reply > 199 and forget this reply.
		*/
		return woomera_message_parse(fd, wmsg, timeout, profile, event_queue);
	} else {
		return ast_test_flag(wmsg, WFLAG_EXISTS);
	}


}

static int woomera_message_parse_wait(private_object *tech_pvt, woomera_message *wmsg)
{
	int err=0;

	for (;;) {
                
		if (ast_test_flag(tech_pvt, TFLAG_ABORT)){
         		return -1;
		} 
		
 		err=woomera_message_parse(tech_pvt->command_channel,
                                           wmsg,
                                           100,
                                           tech_pvt->profile,
                                           &tech_pvt->event_queue);

		if (err == 0) {
			/* This is a timeout */
			continue;
		}  

	        break;
	}
        
	return err;	
}



static int tech_create_read_socket(private_object *tech_pvt)
{
	int retry=0;
	int ports=0;

retry_udp:

	ast_mutex_lock(&globals.woomera_port_lock);
	globals.next_woomera_port++;
	if (globals.next_woomera_port >= woomera_max_media_port) {
		globals.next_woomera_port = woomera_base_media_port;
	}
	tech_pvt->port = globals.next_woomera_port;
	ports++;
	ast_mutex_unlock(&globals.woomera_port_lock);
	
	if ((tech_pvt->udp_socket = create_udp_socket(tech_pvt->profile->audio_ip, tech_pvt->port, &tech_pvt->udpread, 0)) > -1) {
		struct ast_channel *owner = tech_get_owner(tech_pvt);
		if (owner) {
			owner->fds[0] = tech_pvt->udp_socket;
		} else {
			ast_log(LOG_ERROR, "Tech_pvt has no OWNER! %i\n",__LINE__);
		}

	} else {

		retry++;
		if (retry <= 10 || errno == EADDRINUSE) {
			if (ports < (woomera_max_media_port - woomera_base_media_port)) {
				goto retry_udp;
			}
		}

		
		if (globals.debug) {
			ast_log(LOG_ERROR,
			"Error Creating udp socket  %s/%i (%p) %s %s %s\n",
				tech_pvt->profile->audio_ip,
				tech_pvt->port,
				tech_pvt,
				tech_pvt->callid,
				ast_test_flag(tech_pvt, TFLAG_OUTBOUND) ? "OUT":"IN",
				strerror(errno));
		}
	}
	return tech_pvt->udp_socket;
}

static unsigned char get_digits_val_from_pbx_var(struct ast_channel *owner, char* var_name, unsigned char default_val)
{
	const char* pbx_var = NULL;
	pbx_var = pbx_builtin_getvar_helper(owner, var_name);
	if (pbx_var && (atoi(pbx_var) >= 0)) {
		return atoi(pbx_var);
	}
	return default_val;
}

#define WOOMERA_MAX_CALLS 600
static struct private_object *tech_pvt_idx[WOOMERA_MAX_CALLS];
static ast_mutex_t tech_pvt_idx_lock[WOOMERA_MAX_CALLS];

static int tech_activate(private_object *tech_pvt) 
{
	int retry_activate_call=0;
	woomera_message wmsg;
	char *callid;
	char *rdnis = NULL;
	int err=0;

	tech_pvt->owner->hangupcause = 34;
	memset(&wmsg,0,sizeof(wmsg));

retry_activate_again:
	
	if (!tech_pvt) {
		ast_log(LOG_ERROR, "Critical Error: Where's my tech_pvt?\n");
		return -1;
	}

	if((connect_woomera(&tech_pvt->command_channel, tech_pvt->profile, 0)) > -1) {
		if (globals.debug > 2) {
			ast_log(LOG_NOTICE, 
			"Connected to woomera! chfd=%i port=%i dir=%s callid=%s Count=%i tpvt=%p\n",
				tech_pvt->command_channel,
				tech_pvt->port,
				ast_test_flag(tech_pvt, TFLAG_OUTBOUND)?"OUT":"IN",
				ast_test_flag(tech_pvt, TFLAG_OUTBOUND)?"N/A" : 
					tech_pvt->callid,
					tech_pvt->call_count,
				tech_pvt);
		}
	} else {

		if (retry_activate_call <= 3) {
			retry_activate_call++;
			goto retry_activate_again;
		}
		
				
		if (globals.debug > 1 && option_verbose > 1 && woomera_profile_verbose(tech_pvt->profile) > 1) {
			ast_log(LOG_ERROR, "Error: %s call connect to TCP/Woomera Server! tpvt=%p: %s\n",
					ast_test_flag(tech_pvt, TFLAG_OUTBOUND)?"Out":"In",
					tech_pvt,strerror(errno));
		}
		goto tech_activate_failed;
	}

	if (tech_pvt->cid_rdnis) {
		if (validate_number(tech_pvt->cid_rdnis) < 0) {
			/* cid_rdnis should only contain digits */
			ast_log(LOG_ERROR, "Error, invalid RDNIS value %s - ignoring\n", tech_pvt->cid_rdnis);
		} else {
			rdnis = tech_pvt->cid_rdnis;
		}
	} 
	
	retry_activate_call=0;
	const char *custom = pbx_builtin_getvar_helper(tech_pvt->owner, "WOOMERA_CUSTOM");

	if (ast_test_flag(tech_pvt, TFLAG_OUTBOUND)) {
	
		if (strlen(tech_pvt->proto) > 1) {
			err=woomera_printf(tech_pvt->profile,
				 tech_pvt->command_channel, 
				 "CALL %s:%s%s"
				 "Raw-Audio: %s:%d%s"
				 "Local-Name: %s!%s%s"
				 "Local-Number:%s%s"
				 "Presentation:%d%s"
				 "Screening:%d%s"
				 "Bearer-Cap:%s%s"
				 "uil1p:%s%s"
				 "RDNIS:%s%s"
				 "xCalledTon:%d%s"
				 "xCalledNpi:%d%s"
				 "xCallingTon:%d%s"
				 "xCallingNpi:%d%s"
				 "xRdnisTon:%d%s"
				 "xRdnisNpi:%d%s"
				 "xCustom:%s%s",
				 tech_pvt->proto,
				 tech_pvt->dest, 
				 WOOMERA_LINE_SEPARATOR,
				 tech_pvt->profile->audio_ip,
				 tech_pvt->port,
				 WOOMERA_LINE_SEPARATOR,
				 tech_pvt->cid_name,
				 tech_pvt->cid_num,
				 WOOMERA_LINE_SEPARATOR,
				 tech_pvt->cid_num,
				 WOOMERA_LINE_SEPARATOR,
				 (tech_pvt->cid_pres>>5)&0x7,
				 WOOMERA_LINE_SEPARATOR,
				 tech_pvt->cid_pres&0xF,
				 WOOMERA_LINE_SEPARATOR,
				 woomera_ast_transfercap_to_string(tech_pvt->capability),
				 WOOMERA_LINE_SEPARATOR,
				 woomera_ast_coding_to_string(tech_pvt->coding),
				 WOOMERA_LINE_SEPARATOR,
				 rdnis?rdnis:"",
				 WOOMERA_LINE_SEPARATOR,
				 get_digits_val_from_pbx_var(tech_pvt->owner, "OUTBOUND_CALLED_TON", 255),
				 WOOMERA_LINE_SEPARATOR,
				 get_digits_val_from_pbx_var(tech_pvt->owner, "OUTBOUND_CALLED_NPI", 255),
				 WOOMERA_LINE_SEPARATOR,
				 get_digits_val_from_pbx_var(tech_pvt->owner, "OUTBOUND_CALLING_TON", 255),
				 WOOMERA_LINE_SEPARATOR,
				 get_digits_val_from_pbx_var(tech_pvt->owner, "OUTBOUND_CALLING_NPI", 255),
				 WOOMERA_LINE_SEPARATOR,
				 get_digits_val_from_pbx_var(tech_pvt->owner, "OUTBOUND_RDNIS_TON", 255),
				 WOOMERA_LINE_SEPARATOR,
				 get_digits_val_from_pbx_var(tech_pvt->owner, "OUTBOUND_RDNIS_NPI", 255),
				 WOOMERA_LINE_SEPARATOR,
				 custom?custom:"",
				 WOOMERA_RECORD_SEPARATOR
				 );

		 } else {
			err=woomera_printf(tech_pvt->profile,
				 tech_pvt->command_channel, 
				 "CALL %s%s"
				 "Raw-Audio: %s:%d%s"
				 "Local-Name: %s!%s%s"
				 "Local-Number:%s%s"
				 "Presentation:%d%s"
				 "Screening:%d%s"
				 "Bearer-Cap:%s%s"
				 "uil1p:%s%s"
				 "RDNIS:%s%s"
				 "xCalledTon:%d%s"
				 "xCalledNpi:%d%s"
				 "xCallingTon:%d%s"
				 "xCallingNpi:%d%s"
				 "xRdnisTon:%d%s"
				 "xRdnisNpi:%d%s"
				 "xCustom:%s%s",
				 tech_pvt->dest, 
				 WOOMERA_LINE_SEPARATOR,
				 tech_pvt->profile->audio_ip,
				 tech_pvt->port,
				 WOOMERA_LINE_SEPARATOR,
				 tech_pvt->cid_name,
				 tech_pvt->cid_num,
				 WOOMERA_LINE_SEPARATOR,
				 tech_pvt->cid_num,
				 WOOMERA_LINE_SEPARATOR,
				 (tech_pvt->cid_pres>>5)&0x7,
				 WOOMERA_LINE_SEPARATOR,
				 tech_pvt->cid_pres&0xF,
				 WOOMERA_LINE_SEPARATOR,
				 woomera_ast_transfercap_to_string(tech_pvt->capability),
				 WOOMERA_LINE_SEPARATOR,
				 woomera_ast_coding_to_string(tech_pvt->coding),
				 WOOMERA_LINE_SEPARATOR,
				 rdnis?rdnis:"",
				 WOOMERA_LINE_SEPARATOR,
				 get_digits_val_from_pbx_var(tech_pvt->owner, "OUTBOUND_CALLED_TON", 255),
				 WOOMERA_LINE_SEPARATOR,
				 get_digits_val_from_pbx_var(tech_pvt->owner, "OUTBOUND_CALLED_NPI", 255),
				 WOOMERA_LINE_SEPARATOR,
				 get_digits_val_from_pbx_var(tech_pvt->owner, "OUTBOUND_CALLING_TON", 255),
				 WOOMERA_LINE_SEPARATOR,
				 get_digits_val_from_pbx_var(tech_pvt->owner, "OUTBOUND_CALLING_NPI", 255),
				 WOOMERA_LINE_SEPARATOR,
				 get_digits_val_from_pbx_var(tech_pvt->owner, "OUTBOUND_RDNIS_TON", 255),
				 WOOMERA_LINE_SEPARATOR,
				 get_digits_val_from_pbx_var(tech_pvt->owner, "OUTBOUND_RDNIS_NPI", 255),
				 WOOMERA_LINE_SEPARATOR,
				 custom?custom:"",
				 WOOMERA_RECORD_SEPARATOR
				 );
		 }

		 if (err < 0) {
			if (globals.debug > 2) {
			ast_log(LOG_NOTICE, "Outboud call failed -write msg Call %s tpvt=%p\n",
							tech_pvt->callid,tech_pvt);
			}
			ast_set_flag(tech_pvt, TFLAG_ABORT);
			goto tech_activate_failed;
		 }

		 err=woomera_message_parse_wait(tech_pvt,&wmsg);
		 if (err < 0) {
			if (globals.debug > 2) {
			ast_log(LOG_NOTICE, "Outboud call failed -wait parse Call %s tpvt=%p\n",
							tech_pvt->callid,tech_pvt);
			}
			ast_set_flag(tech_pvt, TFLAG_ABORT);
			goto tech_activate_failed;
  	 	 }

		 if (woomera_message_reply_ok(&wmsg) != 0) {
			if (globals.debug > 2) {
			ast_log(LOG_NOTICE, "Outboud call failed reply failed Call %s tpvt=%p\n",
				tech_pvt->callid,tech_pvt);
			}
			ast_set_flag(tech_pvt, TFLAG_ABORT);
		 }
		
		 callid = woomera_message_header(&wmsg, "Unique-Call-Id");
		 if (callid) {
			ast_copy_string(tech_pvt->callid,callid,sizeof(wmsg.callid));
		 }
		 
	} else {
		ast_set_flag(tech_pvt, TFLAG_PARSE_INCOMING);
		if (globals.debug > 2) {
			ast_log(LOG_NOTICE, "Incoming Call %s tpvt=%p\n",
				tech_pvt->callid,tech_pvt);
		}
		

#if 0
		/* NC: Took this out becuase its not needed any more.
		       It was a kluge to get load balancing to work
			   but now it works properly so it should be removed.
			   I am keeping it here as depricated */
		err=woomera_printf(tech_pvt->profile,
				 tech_pvt->command_channel, 
				 "PROCEED %s%s"
				 "Unique-Call-Id: %s%s",
				 tech_pvt->callid,
				 WOOMERA_LINE_SEPARATOR,
				 tech_pvt->callid,
				 WOOMERA_RECORD_SEPARATOR);

		 if (err < 0) {
			ast_set_flag(tech_pvt, TFLAG_ABORT);
			goto tech_activate_failed;
		 }

		 err=woomera_message_parse_wait(tech_pvt,&wmsg);
		 if (err < 0) {
			ast_set_flag(tech_pvt, TFLAG_ABORT);
			goto tech_activate_failed;
		 }

		 if (woomera_message_reply_ok(&wmsg) != 0) {
			ast_set_flag(tech_pvt, TFLAG_ABORT);
			/* Do not hangup on main because
			 * socket connection has been
			 * established */
	     }
#endif
	}
	

	if (globals.debug > 2) {
		ast_log(LOG_NOTICE, "TECH ACTIVATE OK tech_pvt=%p\n",tech_pvt);
	}
	return 0;

tech_activate_failed:
	if (globals.debug > 2) {
		ast_log(LOG_NOTICE, "TECH ACTIVATE FAILED tech_pvt=%p\n",tech_pvt);
	}
					
	woomera_close_socket(&tech_pvt->command_channel);
	ast_set_flag(tech_pvt, TFLAG_ABORT);

	/* At this point we cannot estabilsh a woomer
 	 * socket to the smg.  The SMG still doesnt know
 	 * about the incoming call that is now pending.
 	 * We must send a message to SMG to hangup the call */


	
	if (globals.debug > 2) {
  	ast_log(LOG_NOTICE, "Error: %s Call %s tpvt=%p Failed!\n",
                                ast_test_flag(tech_pvt, TFLAG_OUTBOUND) ? "OUT":"IN",
				tech_pvt->callid,tech_pvt);
	}

	return -1;

}

static int tech_init(private_object *tech_pvt, woomera_profile *profile, int flags) 
{
	struct ast_channel *self = tech_get_owner(tech_pvt);

	gettimeofday(&tech_pvt->started, NULL);

	if (profile) {
		tech_pvt->profile = profile;
	} else {
		ast_log(LOG_ERROR, "ERROR: No Tech profile on init!\n");
		ast_set_flag(tech_pvt, TFLAG_ABORT);
		return -1;
	}

	ast_set_flag(tech_pvt, flags);
	
	if (tech_pvt->udp_socket < 0) {
		int rc;
		rc=tech_create_read_socket(tech_pvt);
		if (rc < 0){
			ast_log(LOG_ERROR, "ERROR: Failed to create UDP Socket (%p)! %s\n",
						tech_pvt,strerror(errno));
			ast_set_flag(tech_pvt, TFLAG_ABORT);
			return -1;
		}
	}

	ast_set_flag(tech_pvt, flags);

	tech_pvt->capability = self->transfercapability;
	
	tech_pvt->coding = profile->coding;
	self->nativeformats = tech_pvt->coding;	
	self->writeformat = self->rawwriteformat = self->rawreadformat = self->readformat = tech_pvt->coding;
	tech_pvt->frame.W_SUBCLASS_CODEC = tech_pvt->coding;

	ast_clear_flag(tech_pvt, TFLAG_CONFIRM_ANSWER);
	ast_clear_flag(tech_pvt, TFLAG_CONFIRM_ANSWER_ENABLED);
	ast_clear_flag(tech_pvt, TFLAG_ANSWER_RECEIVED);

	if (profile && profile->faxdetect) {
		tech_pvt->faxdetect=1;
	}
	
	if (profile->dtmf_enable) {
			
		tech_pvt->dsp_features=0;
		tech_pvt->dsp = ast_dsp_new();
		if (tech_pvt->dsp) {
#if 0
			i->dsp_features = features & ~DSP_PROGRESS_TALK;
	
			/* We cannot do progress detection until receives PROGRESS message */
			if (i->outgoing && (i->sig == SIG_PRI)) {
				/* Remember requested DSP features, don't treat
				talking as ANSWER */
				features = 0;
			}
#endif
			tech_pvt->dsp_features |= DSP_FEATURE_DTMF_DETECT;
			//tech_pvt->dsp_features |= DSP_FEATURE_BUSY_DETECT;
			//tech_pvt->dsp_features |= DSP_FEATURE_CALL_PROGRESS;
			if (tech_pvt->faxdetect) {
				tech_pvt->dsp_features |= DSP_FEATURE_FAX_DETECT;
			} 
			ast_dsp_set_features(tech_pvt->dsp, tech_pvt->dsp_features);
			ast_dsp_digitmode(tech_pvt->dsp, DSP_DIGITMODE_DTMF | DSP_DIGITMODE_RELAXDTMF);
			tech_pvt->ast_dsp=1;
#if 0
			if (!ast_strlen_zero(progzone))
				ast_dsp_set_call_progress_zone(tech_pvt->dsp, progzone);
			if (i->busydetect && CANBUSYDETECT(i)) {
				ast_dsp_set_busy_count(tech_pvt->dsp, i->busycount);
				ast_dsp_set_busy_pattern(tech_pvt->dsp, i->busy_tonelength, ->busy_quietlength);
			}             	
#endif
		}
	}

	if (profile->jb_enable) {
#if defined AST_JB
		/* Assign default jb conf to the new zt_pvt */
		memcpy(&tech_pvt->jbconf, &global_jbconf, sizeof(struct ast_jb_conf));
		ast_jb_configure(self, &tech_pvt->jbconf);
		
		if (globals.debug > 1 && option_verbose > 10 && woomera_profile_verbose(profile) > 10) {
			ast_log(LOG_NOTICE, "%s: Cfg JitterBuffer (F=%i MS=%li Rs=%li Impl=%s)\n",
					self->name,
					tech_pvt->jbconf.flags,
					tech_pvt->jbconf.max_size,
					tech_pvt->jbconf.resync_threshold,
					tech_pvt->jbconf.impl);
		}
#else
		ast_log(LOG_ERROR, "Asterisk Jitter Buffer Not Compiled!\n");
#endif
	} 


	/* Asterisk being asterisk and all allows approx 1 nanosecond 
	 * to try and establish a connetion here before it starts crying.
	 * Now asterisk, being unsure of it's self will not enforce a lock while we work
	 * and after even a 1 second delay it will give up on the lock and mess everything up
	 * This stems from the fact that asterisk will scan it's list of channels constantly for 
	 * silly reasons like tab completion and cli output.
	 *
	 * Anyway, since we've already spent that nanosecond with the previous line of code
	 * tech_create_read_socket(tech_pvt); to setup a read socket
	 * which, by the way, asterisk insists we have before going any furthur.  
	 * So, in short, we are between a rock and a hard place and asterisk wants us to open a socket here
	 * but it too impatient to wait for us to make sure it's ready so in the case of outgoing calls
	 * we will defer the rest of the socket establishment process to the monitor thread.  This is, of course, common 
	 * knowledge since asterisk abounds in documentation right?, sorry to bother you with all this!
	 */
	if (globals.more_threads) {
		int err;
		ast_set_flag(tech_pvt, TFLAG_ACTIVATE);
		/* we're gonna try "wasting" a thread to do a better realtime monitoring */
		err=launch_tech_thread(tech_pvt);
		if (err) {
			ast_log(LOG_ERROR, "Error: Failed to lauch tech control thread\n");
			ast_clear_flag(tech_pvt, TFLAG_ACTIVATE);
			ast_set_flag(tech_pvt, TFLAG_ABORT);
			return -1;
		} 
		
	} else {
		if (ast_test_flag(tech_pvt, TFLAG_OUTBOUND)) {
			ast_set_flag(tech_pvt, TFLAG_ACTIVATE);
		} else {
			tech_activate(tech_pvt);
		}
	}
	
	
	if (globals.debug > 2) {
		ast_log(LOG_NOTICE, "TECH INIT tech_pvt=%p c=%p (use=%i)\n",
			tech_pvt,tech_pvt->owner,usecount());
	}
	
	return 0;
}



static void tech_destroy(private_object *tech_pvt, struct ast_channel *owner) 
{

	ASTOBJ_CONTAINER_UNLINK(&private_object_list, tech_pvt);

	ast_set_flag(tech_pvt, TFLAG_DESTROY);
	ast_set_flag(tech_pvt, TFLAG_ABORT);


	if (globals.debug > 2) {
		ast_log(LOG_NOTICE, "Tech Destroy callid=%s tpvt=%p %s/%d\n",
						tech_pvt->callid,
						tech_pvt,
						tech_pvt->profile ? tech_pvt->profile->audio_ip : "NA",
						tech_pvt->port);
	}
	
	
	if (tech_pvt->profile && tech_pvt->command_channel > -1) {

		if (globals.debug > 1 && option_verbose > 1 && woomera_profile_verbose(tech_pvt->profile) > 1) {
			ast_log(LOG_NOTICE, "+++DESTROY sent HANGUP %s\n",
				tech_pvt->callid);
		}
		woomera_printf(tech_pvt->profile, tech_pvt->command_channel,
					   "hangup %s%scause: %s%sQ931-Cause-Code: %d%sUnique-Call-Id: %s%s", 
					   tech_pvt->callid,
					   WOOMERA_LINE_SEPARATOR, 
					   tech_pvt->ds, 
					   WOOMERA_LINE_SEPARATOR,
					   tech_pvt->pri_cause, 
					   WOOMERA_LINE_SEPARATOR,
					   tech_pvt->callid,
					   WOOMERA_RECORD_SEPARATOR);
		woomera_printf(tech_pvt->profile, tech_pvt->command_channel, 
						"bye%s" 
						"Unique-Call-Id: %s%s",
						WOOMERA_LINE_SEPARATOR,
						tech_pvt->callid,
						WOOMERA_RECORD_SEPARATOR);
		woomera_close_socket(&tech_pvt->command_channel);
	}

	woomera_close_socket(&tech_pvt->command_channel);
	woomera_close_socket(&tech_pvt->udp_socket);


	/* Tech profile is allowed to be null in case the call
	 * is blocked from the call_count */
	
#if 0
	ast_log(LOG_NOTICE, "---- Call END %p %s ----------------------------\n",
		tech_pvt,tech_pvt->callid);		
#endif

	if (owner) {
		if (globals.debug > 2) {
			ast_log(LOG_NOTICE, "Tech Thread - Tech Destroy doing AST HANGUP!\n");
		}
		owner->tech_pvt = NULL;
		tech_pvt->owner=NULL;
		ast_hangup(owner);
	}
	tech_pvt->owner=NULL;

	
	tech_count--;
	if (tech_pvt->dsp){
		tech_pvt->dsp_features &= ~DSP_FEATURE_DTMF_DETECT;
                ast_dsp_set_features(tech_pvt->dsp, tech_pvt->dsp_features);
                tech_pvt->ast_dsp=0;

		ast_free(tech_pvt->dsp);
		tech_pvt->dsp=NULL;
	}

	if (globals.debug > 2) {
		ast_log(LOG_NOTICE, "DESTROY Exit tech_pvt=%p  (use=%i)\n",
			tech_pvt,usecount());
	}
	
	ast_mutex_destroy(&tech_pvt->iolock);
	ast_mutex_destroy(&tech_pvt->event_queue.lock);

	if (tech_pvt->cid_rdnis) { 
		ast_free(tech_pvt->cid_rdnis);
		tech_pvt->cid_rdnis=NULL;
	}

	if (debug_tech_pvt == tech_pvt) {
     	debug_tech_pvt=NULL;
	}

	ast_free(tech_pvt);	
	ast_mutex_lock(&usecnt_lock);
	usecnt--;
	ast_mutex_unlock(&usecnt_lock);
}

#if 0

static int waitfor_socket(int fd, int timeout) 
{
	struct pollfd pfds[1];
	int res;
	int errflags = (POLLERR | POLLHUP | POLLNVAL);
	
	if (fd < 0) {
		return -1;
	}

	memset(&pfds[0], 0, sizeof(pfds[0]));
	pfds[0].fd = fd;
	pfds[0].events = POLLIN | errflags;
	res = poll(pfds, 1, timeout);
	if (res > 0) {
		if ((pfds[0].revents & errflags)) {
			res = -1;
		} else if ((pfds[0].revents & POLLIN)) {
			res = 1;
		} else {
			ast_log(LOG_ERROR, "System Error: Poll Event Error no event!\n");
			res = -1;
		}
	}
	
	return res;
}

#else

static int waitfor_socket(int fd, int timeout) 
{
	struct pollfd pfds[1];
	int res;

	if (fd < 0) {
		return -1;
	}

	memset(&pfds[0], 0, sizeof(pfds[0]));
	pfds[0].fd = fd;
	pfds[0].events = POLLIN | POLLERR;
	res = poll(pfds, 1, timeout);
	if (res > 0) {
		if ((pfds[0].revents & POLLERR)) {
			res = -1;
		} else if((pfds[0].revents & POLLIN)) {
			res = 1;
		} else {
			res = -1;
		}
	}
	
	return res;
}

#endif


static void *tech_monitor_thread(void *obj) 
{
	private_object *tech_pvt;
	woomera_message wmsg;
	char tcallid[WOOMERA_STRLEN];
	int aborted=0;

	int res = 0;

	tech_pvt = obj;
		
	if (ast_test_flag(tech_pvt, TFLAG_TECHHANGUP)) {
		ast_log(LOG_NOTICE, "Tech Monitor: Call stopped before thread up!\n");
		return NULL;
	}	

	memset(tcallid,0,sizeof(tcallid));
	memset(&wmsg,0,sizeof(wmsg));

	if (globals.debug > 2) {
		ast_log(LOG_NOTICE, "IN THREAD %s  rxgain=%f txtain=%f\n",
				tech_pvt->callid,
				tech_pvt->profile->rxgain_val,
				tech_pvt->profile->txgain_val);
	}
	ast_mutex_lock(&tech_pvt->profile->call_count_lock);
	if (ast_test_flag(tech_pvt, TFLAG_OUTBOUND)) {
		tech_pvt->profile->call_out++;
	} else {
		tech_pvt->profile->call_in++;
	}
	tech_pvt->profile->call_count++;
	ast_mutex_unlock(&tech_pvt->profile->call_count_lock);

	for(;;) {
		
		if (globals.panic) {
			ast_set_flag(tech_pvt, TFLAG_ABORT);
		}

		if (!tech_pvt->owner) {
			ast_set_flag(tech_pvt, TFLAG_ABORT);
			if (globals.debug > 2) {
			ast_log(LOG_NOTICE, "Thread lost Owner Chan %s %p\n",
						tech_pvt->callid,
							tech_pvt);
			}
		}
	

		/* finish the deferred crap asterisk won't allow us to do live */
		if (ast_test_flag(tech_pvt, TFLAG_ABORT)) {
			int ast_hangup=0;
			struct woomera_profile *profile = tech_pvt->profile;;

			if (globals.debug > 2) {
				ast_log(LOG_NOTICE, "ABORT GOT HANGUP CmdCh=%i %s %s/%i\n",	
					tech_pvt->command_channel, tech_pvt->callid,
					tech_pvt->profile ? tech_pvt->profile->audio_ip : "N/A",
					tech_pvt->port);
			}

			aborted|=1;

			/* Check for queued events, looking for HANGUP messages,
			   so we can return proper hangup cause */
			for (;;) {
				if ((res = woomera_dequeue_event(&tech_pvt->event_queue, &wmsg))) {
					woomera_check_event (tech_pvt, res, &wmsg);			
				} else {
					break;
				}
			}
		
			ast_mutex_lock(&tech_pvt->profile->call_count_lock);
			tech_pvt->profile->call_count--;
			ast_mutex_unlock(&tech_pvt->profile->call_count_lock);

			if (tech_pvt->profile && tech_pvt->command_channel > -1) {

				if (globals.debug > 2) {
				ast_log(LOG_NOTICE, "ABORT sent HANGUP on %s %p\n",
							tech_pvt->callid,
								tech_pvt);
				}
				aborted|=2;
		                woomera_printf(tech_pvt->profile, tech_pvt->command_channel, 
							"hangup %s%scause: %s%sQ931-Cause-Code: %d%sUnique-Call-Id: %s%s",
							tech_pvt->callid,
							WOOMERA_LINE_SEPARATOR, 
							tech_pvt->ds, 
							WOOMERA_LINE_SEPARATOR, 
							tech_pvt->pri_cause, 
							WOOMERA_LINE_SEPARATOR, 
							tech_pvt->callid,
							WOOMERA_RECORD_SEPARATOR);
		                woomera_printf(tech_pvt->profile, tech_pvt->command_channel, 
							"bye%s" 
							"Unique-Call-Id: %s%s",
							WOOMERA_LINE_SEPARATOR,
							tech_pvt->callid,
							WOOMERA_RECORD_SEPARATOR);
				woomera_close_socket(&tech_pvt->command_channel);
			}

			if (tech_pvt->udp_socket > -1) {
				woomera_close_socket(&tech_pvt->udp_socket);
			}

			if (globals.debug > 2) {
				ast_log(LOG_NOTICE,"Tech Thread - Hanging up channel\n");
			}
			ast_mutex_lock(&tech_pvt->iolock);
			while(tech_pvt->owner && my_ast_channel_trylock(tech_pvt->owner)) {
				if (globals.debug > 2) {
					ast_log(LOG_NOTICE,"Tech Thrad - Hanging up channel - deadlock avoidance\n");
				}
				DEADLOCK_AVOIDANCE(&tech_pvt->iolock);
			} 

			if (tech_pvt->owner) {
				struct ast_channel *owner = tech_pvt->owner;
				if (ast_test_flag(tech_pvt, TFLAG_PBX) || owner->pbx) {
					aborted|=4;

					if (globals.debug > 2) {
						ast_log(LOG_NOTICE, "ABORT calling hangup on %s t=%p c=%p UP=%d\n",
											tech_pvt->callid,
											tech_pvt,
											owner,
											ast_test_flag(tech_pvt, TFLAG_UP));
					}

					/* Issue a softhangup */
					if (globals.debug > 2) {
						ast_log(LOG_NOTICE,"Tech Thread - Hanging up channel - soft hangup\n");
					}

#if defined (AST14) || defined (AST16)
					if (ast_test_flag(owner, AST_FLAG_BLOCKING)) {
						int cnt=0;
						while (!owner->blocker) {
							ast_log(LOG_ERROR, "Woomera: BLOCKER Set but no blocker!\n");
							usleep(50);
							cnt++;
						}
						if (cnt) {
							ast_log(LOG_ERROR, "Woomera: BLOCKER Set Got Blocker %s!\n",
									owner->blockproc);
						}
					}
#endif

					ast_softhangup_nolock(owner, AST_SOFTHANGUP_DEV);
					ast_clear_flag(tech_pvt, TFLAG_INTHREAD);
					ast_set_flag(tech_pvt, TFLAG_AST_HANGUP);
					tech_pvt->owner=NULL;

					/* After this point tech_pvt point should not be touched */
					ast_hangup=1;

				} else {
					if (globals.debug > 2) {
						ast_log(LOG_NOTICE,"Tech Thread - Hanging up channel - owner=%p pbx=%i \n",
							owner,ast_test_flag(tech_pvt, TFLAG_PBX));
					}
				}
				my_ast_channel_unlock(owner);
			}


			ast_mutex_unlock(&tech_pvt->iolock);
				
			ast_mutex_lock(&profile->call_count_lock);
			profile->call_end++;
			ast_mutex_unlock(&profile->call_count_lock);

			/* Wait for tech_hangup to set this, so there is on
  		         * race condition with asterisk */
			//ast_set_flag(tech_pvt, TFLAG_DESTROY);

			if (ast_hangup) {
				ast_mutex_lock(&profile->call_count_lock);
				profile->call_ast_hungup++;
				ast_mutex_unlock(&profile->call_count_lock);

				/* Let Asterisk tech_hangup destroy tech_pvt */
				goto tech_thread_exit;

			} else {
				ast_mutex_lock(&tech_pvt->profile->call_count_lock);
				tech_pvt->profile->call_abort++;
				ast_mutex_unlock(&tech_pvt->profile->call_count_lock);
		
				if (globals.debug > 2) {
				ast_log(LOG_NOTICE, "NOTE: Skipping Wait on destroy timedout! %s tech_pvt=%p\n",
                                                        tech_pvt->callid, tech_pvt);
				}
				ast_set_flag(tech_pvt, TFLAG_DESTROY);
			}

			aborted|=8;
			tech_destroy(tech_pvt,tech_get_owner(tech_pvt));
			tech_pvt = NULL;
			break;
		}


		if (ast_test_flag(tech_pvt, TFLAG_TECHHANGUP) || !tech_pvt->owner) {
			ast_set_flag(tech_pvt, TFLAG_DESTROY);
			ast_set_flag(tech_pvt, TFLAG_ABORT);
			if (globals.debug > 2) {
			ast_log(LOG_NOTICE, "Thread got HANGUP or no owner %s  %p tpvt=%p\n",
					tech_pvt->callid,tech_pvt,tech_pvt->owner);
			}
			goto tech_thread_continue;
		}

		if (ast_test_flag(tech_pvt, TFLAG_ACTIVATE)) {
		
			struct ast_channel *owner;
			int err;
		
			if (globals.debug > 2) {
			ast_log(LOG_NOTICE, "ACTIVATE %s tpvt=%p\n",
					tech_pvt->callid,tech_pvt);
			}
			ast_clear_flag(tech_pvt, TFLAG_ACTIVATE);
			err=tech_activate(tech_pvt);
			if (err < 0 || ast_test_flag(tech_pvt, TFLAG_ABORT)) {
				if (globals.debug > 2) {
					ast_log(LOG_NOTICE, "ACTIVATE ABORT Ch=%d\n",
						tech_pvt->command_channel);
				}
				ast_set_flag(tech_pvt, TFLAG_ABORT);
				goto tech_thread_continue;
			}

			

			my_tech_pvt_and_owner_lock(tech_pvt);
			owner = tech_pvt->owner;
			if (owner) {
				owner->hangupcause = AST_CAUSE_NORMAL_CLEARING;
			}
			my_tech_pvt_and_owner_unlock(tech_pvt);

			if (globals.debug > 2) {
				ast_log(LOG_NOTICE, "ACTIVATE DONE %s tpvt=%p\n",
					tech_pvt->callid,tech_pvt);
			}
		}

		if (ast_test_flag(tech_pvt, TFLAG_PARSE_INCOMING)) {
			int err;

			ast_clear_flag(tech_pvt, TFLAG_PARSE_INCOMING);
			ast_set_flag(tech_pvt, TFLAG_INCOMING);

			my_tech_pvt_and_owner_lock(tech_pvt);
			err=woomera_event_incoming (tech_pvt);
			my_tech_pvt_and_owner_unlock(tech_pvt);

			if (err != 0) {
				
				err=woomera_printf(tech_pvt->profile, tech_pvt->command_channel, 
						"hangup %s%s"
						"cause: INVALID_CALL_REFERENCE%s" 
						"Q931-Cause-Code: 81%s"
						"Unique-Call-Id: %s%s",
						tech_pvt->callid, 
						WOOMERA_LINE_SEPARATOR, 
						WOOMERA_LINE_SEPARATOR, 
						WOOMERA_LINE_SEPARATOR, 
						tech_pvt->callid,
						WOOMERA_RECORD_SEPARATOR);

				/* Wait for Ack */
				if (err >= 0) {
					woomera_message_parse_wait(tech_pvt,&wmsg);
				}

				woomera_close_socket(&tech_pvt->command_channel);

				ast_set_flag(tech_pvt, TFLAG_ABORT);
				goto tech_thread_continue;
		
			} else {
			
				err=woomera_printf(tech_pvt->profile, tech_pvt->command_channel, 
							"%s %s%s"
							"Raw-Audio: %s:%d%s"
							"Request-Audio: Raw%s"
							"Unique-Call-Id: %s%s",
							MEDIA_ANSWER,
							tech_pvt->callid,
							WOOMERA_LINE_SEPARATOR,
							tech_pvt->profile->audio_ip,
							tech_pvt->port,
							WOOMERA_LINE_SEPARATOR,
							WOOMERA_LINE_SEPARATOR,
							tech_pvt->callid,
							WOOMERA_RECORD_SEPARATOR);

			}

			/* Wait for Ack */
			if (err< 0 || woomera_message_parse_wait(tech_pvt,&wmsg) < 0) {
				ast_set_flag(tech_pvt, TFLAG_ABORT);
				ast_log(LOG_NOTICE, "MEDIA ANSWER ABORT Ch=%d\n",
					tech_pvt->command_channel);
				ast_copy_string(tech_pvt->ds, "PROTOCOL_ERROR", sizeof(tech_pvt->ds));
                		tech_pvt->pri_cause=111;
				goto tech_thread_continue;
			} 

 			/* Confirm that the Ack is OK otherwise
			 * hangup */
			if (woomera_message_reply_ok(&wmsg) != 0) {
				ast_set_flag(tech_pvt, TFLAG_ABORT);
				goto tech_thread_continue;
			}
			

			/* It is possible for ACCEPT to have media info
			 * This is how Early Media is started */
			err=woomera_event_media (tech_pvt, &wmsg);
			if (err < 0) {
				ast_set_flag(tech_pvt, TFLAG_ABORT);
				goto tech_thread_continue;
			} 
		}

		if (ast_test_flag(tech_pvt, TFLAG_ACCEPT) &&
		    ast_test_flag(tech_pvt, TFLAG_INCOMING)) {
			int err;
 
			ast_set_flag(tech_pvt,TFLAG_ACCEPTED);
			ast_clear_flag(tech_pvt,TFLAG_ACCEPT);
			
			if (tech_pvt->profile->progress_on_accept) {
				ast_set_flag(tech_pvt, TFLAG_PROGRESS);
			}

			err=woomera_printf(tech_pvt->profile, tech_pvt->command_channel,
								"ACCEPT %s%s"
								"Raw-Audio: %s:%d%s"
								"Request-Audio: Raw%s"
								"Unique-Call-Id: %s%s"
								"xMedia:%s%s"
								"xRing:%s%s",
								tech_pvt->callid,
								WOOMERA_LINE_SEPARATOR,
								tech_pvt->profile->audio_ip,
								tech_pvt->port,
								WOOMERA_LINE_SEPARATOR,
								WOOMERA_LINE_SEPARATOR,
								tech_pvt->callid,
								WOOMERA_LINE_SEPARATOR,
								ast_test_flag(tech_pvt, TFLAG_MEDIA_AVAIL)? "1":"0",
								WOOMERA_LINE_SEPARATOR,
								ast_test_flag(tech_pvt, TFLAG_RING_AVAIL)? "1":"0",
								WOOMERA_RECORD_SEPARATOR);

			if(err < 0 || woomera_message_parse_wait(tech_pvt,&wmsg) < 0) {
					ast_set_flag(tech_pvt, TFLAG_ABORT);
					if (globals.debug > 2) {
						ast_log(LOG_NOTICE, "ACCEPT ABORT Ch=%d\n",
										tech_pvt->command_channel);
					}
					goto tech_thread_continue;
					continue;
			}
		}

		if (ast_test_flag(tech_pvt, TFLAG_PROGRESS)) {
			int err;
			ast_clear_flag(tech_pvt, TFLAG_PROGRESS);
			err=woomera_printf(tech_pvt->profile, tech_pvt->command_channel,
							   "PROGRESS %s%s"
									   "Unique-Call-Id: %s%s",
							   tech_pvt->callid,
							   WOOMERA_LINE_SEPARATOR,
							   tech_pvt->callid,
							   WOOMERA_RECORD_SEPARATOR);

			if(err < 0 || woomera_message_parse_wait(tech_pvt,&wmsg) < 0) {
				ast_set_flag(tech_pvt, TFLAG_ABORT);
				if (globals.debug > 2) {
					ast_log(LOG_NOTICE, "PROGRESS ABORT Ch=%d\n",
							tech_pvt->command_channel);
				}
				goto tech_thread_continue;
				continue;
			}
		}
		
		if (ast_test_flag(tech_pvt, TFLAG_ANSWER)) {
			int err;

			if (globals.debug > 2) {
			ast_log(LOG_NOTICE, "ANSWER %s tpvt=%p\n",
					tech_pvt->callid,tech_pvt);
			}
			ast_clear_flag(tech_pvt, TFLAG_ANSWER);
			
			if (ast_test_flag(tech_pvt, TFLAG_OUTBOUND)) {
				ast_log(LOG_ERROR,"Error: ANSWER on OUTBOUND Call! (skipped) %s\n",
						tech_pvt->callid);
			} else {	
#ifdef USE_ANSWER
				err=woomera_printf(tech_pvt->profile, tech_pvt->command_channel, 
						"ANSWER %s%s"
						"Unique-Call-Id: %s%s",
						tech_pvt->callid, 
						WOOMERA_LINE_SEPARATOR,
						tech_pvt->callid,
						WOOMERA_RECORD_SEPARATOR);
				
				if(err<0 || woomera_message_parse_wait(tech_pvt,&wmsg) < 0) {
					ast_set_flag(tech_pvt, TFLAG_ABORT);
					if (globals.debug > 2) {
						ast_log(LOG_NOTICE, "ANSWER ABORT Ch=%d\n",
							tech_pvt->command_channel);
					}
					goto tech_thread_continue;
					continue;
				}
			
				ast_mutex_lock(&tech_pvt->profile->call_count_lock);
				tech_pvt->profile->call_ok++;
				ast_mutex_unlock(&tech_pvt->profile->call_count_lock);
#endif
			}
		}
		
		if (ast_test_flag(tech_pvt, TFLAG_DTMF)) {
			int err;
			if (globals.debug > 2) {
			ast_log(LOG_NOTICE, "DTMF %s tpvt=%p %s\n",
					tech_pvt->callid,tech_pvt,tech_pvt->dtmfbuf);
			}

			//DIALECT
			ast_mutex_lock(&tech_pvt->iolock);
#if 0
			woomera_printf(tech_pvt->profile, tech_pvt->command_channel, 
						"DTMF %s %s%s",
						tech_pvt->callid, 
						tech_pvt->dtmfbuf, 
						WOOMERA_LINE_SEPARATOR);
#else
			err=woomera_printf(tech_pvt->profile, tech_pvt->command_channel, 
						"DTMF %sUnique-Call-Id:%s%sContent-Length:%d%s%s%s%s",
						WOOMERA_LINE_SEPARATOR,
						tech_pvt->callid,
						WOOMERA_LINE_SEPARATOR,
						strlen(tech_pvt->dtmfbuf),
						WOOMERA_LINE_SEPARATOR,
						WOOMERA_LINE_SEPARATOR,
						tech_pvt->dtmfbuf,
						WOOMERA_RECORD_SEPARATOR);
#endif
			
			ast_clear_flag(tech_pvt, TFLAG_DTMF);
			memset(tech_pvt->dtmfbuf, 0, sizeof(tech_pvt->dtmfbuf));
			ast_mutex_unlock(&tech_pvt->iolock);

			if (err<0 || woomera_message_parse_wait(tech_pvt,&wmsg) < 0) {
				ast_set_flag(tech_pvt, TFLAG_ABORT);
				ast_log(LOG_NOTICE, "DTMF ABORT Ch=%d\n",
						tech_pvt->command_channel);
				ast_copy_string(tech_pvt->ds, "PROTOCOL_ERROR", sizeof(tech_pvt->ds));
                		tech_pvt->pri_cause=111;
				goto tech_thread_continue;
				continue;
			}
		}

		if (ast_test_flag(tech_pvt, TFLAG_RBS)) {
			int err;
			if (globals.debug > 2) {
			ast_log(LOG_NOTICE, "Woomera Tx RBS %s tpvt=%p %X\n",
					tech_pvt->callid,tech_pvt,tech_pvt->rbsbuf);
			}

			ast_mutex_lock(&tech_pvt->iolock);

			err=woomera_printf(tech_pvt->profile, tech_pvt->command_channel, 
						"RBS %sUnique-Call-Id:%s%sContent-Length:%d%s%s%X%s",
						WOOMERA_LINE_SEPARATOR,
						tech_pvt->callid,
						WOOMERA_LINE_SEPARATOR,
						1,
						WOOMERA_LINE_SEPARATOR,
						WOOMERA_LINE_SEPARATOR,
						tech_pvt->rbsbuf,
						WOOMERA_RECORD_SEPARATOR);
			
			ast_clear_flag(tech_pvt, TFLAG_RBS);
			tech_pvt->rbsbuf=0;
			ast_mutex_unlock(&tech_pvt->iolock);

			if (err<0 || woomera_message_parse_wait(tech_pvt,&wmsg) < 0) {
				ast_set_flag(tech_pvt, TFLAG_ABORT);
				ast_log(LOG_NOTICE, "RBS ABORT Ch=%d\n",
						tech_pvt->command_channel);
				ast_copy_string(tech_pvt->ds, "PROTOCOL_ERROR", sizeof(tech_pvt->ds));
                		tech_pvt->pri_cause=111;
				goto tech_thread_continue;
				continue;
			}
		}
		
		if (ast_test_flag(tech_pvt, TFLAG_MEDIA_RELAY)) {
			ast_clear_flag(tech_pvt, TFLAG_MEDIA_RELAY);
			int err;
			err=woomera_printf(tech_pvt->profile, tech_pvt->command_channel, tech_pvt->woomera_relay);
			
			ast_free(tech_pvt->woomera_relay);
			tech_pvt->woomera_relay=NULL;
			
			if (err<0 || woomera_message_parse_wait(tech_pvt,&wmsg) < 0) {
				ast_set_flag(tech_pvt, TFLAG_ABORT);
				ast_log(LOG_NOTICE, "MEDIA RELAY ABORT Ch=%d\n",
						tech_pvt->command_channel);
				ast_copy_string(tech_pvt->ds, "PROTOCOL_ERROR", sizeof(tech_pvt->ds));
				tech_pvt->pri_cause=111;
				goto tech_thread_continue;
				continue;
			}
	
		}

		if(tech_pvt->timeout) {
			struct timeval now;
			int elapsed;
			gettimeofday(&now, NULL);
			elapsed = (((now.tv_sec * 1000) + now.tv_usec / 1000) - 
			((tech_pvt->started.tv_sec * 1000) + tech_pvt->started.tv_usec / 1000));
			if (elapsed > tech_pvt->timeout) {
				/* call timed out! */
				if (globals.debug > 2) {
				ast_log(LOG_NOTICE, "CALL TIMED OUT %s tpvt=%p\n",
					tech_pvt->callid,tech_pvt);
				}
				ast_set_flag(tech_pvt, TFLAG_ABORT);
				ast_copy_string(tech_pvt->ds, "RECOVERY_ON_TIMER_EXPIRE", sizeof(tech_pvt->ds));
                		tech_pvt->pri_cause=102;
			}
		}
		
		if (globals.debug > 2) {
			if (tcallid[0] == 0) {
				strncpy(tcallid,tech_pvt->callid,sizeof(tcallid)-1);
			}
		}
		
		if (tech_pvt->command_channel < 0) {
			if (!globals.more_threads) {
				goto tech_thread_continue;
				continue;
			} else {
				if (globals.debug > 2) {
				ast_log(LOG_NOTICE, "No Command Channel %s tpvt=%p\n",
					tech_pvt->callid,tech_pvt);
				}
				ast_copy_string(tech_pvt->ds, "REQUESTED_CHAN_UNAVAIL", sizeof(tech_pvt->ds));
                		tech_pvt->pri_cause=44;
				ast_set_flag(tech_pvt, TFLAG_ABORT);
				goto tech_thread_continue;
				continue;
			}
		}
		/* Check for events */
		if((res = woomera_dequeue_event(&tech_pvt->event_queue, &wmsg)) ||
		   (res = woomera_message_parse(tech_pvt->command_channel,
						&wmsg,
						100,
						tech_pvt->profile,
						NULL
						))) {

			woomera_check_event (tech_pvt, res, &wmsg);			
			if (ast_test_flag(tech_pvt, TFLAG_ABORT)) {
				continue;
			}

		}
		if (globals.debug > 4) {
			if (option_verbose > 2 && woomera_profile_verbose(tech_pvt->profile) > 2) {
				ast_verbose(WOOMERA_DEBUG_PREFIX "CHECK {%s} (%d) %s\n", 
						tech_pvt->profile->name,  
						res,tech_pvt->callid);
			}
		}
		
tech_thread_continue:
		
		if (!globals.more_threads) {
			ast_log(LOG_NOTICE, "EXITING THREAD on more threads %s\n",
						tcallid);
			break;
		}
		
	}

tech_thread_exit:

	if (globals.debug > 2) {
		ast_log(LOG_NOTICE, "OUT THREAD %s 0x%X\n",tcallid,aborted);
	}
	
	return NULL;
}

static int woomera_profile_thread_running(woomera_profile *profile, int set, int new) 
{
	int running = 0;

	ast_mutex_lock(&profile->iolock);
	if (set) {
		profile->thread_running = new;
	}
	running = profile->thread_running;
	ast_mutex_unlock(&profile->iolock);
	return running;
	
}

static int woomera_locate_socket(woomera_profile *profile, int *woomera_socket) 
{
	woomera_message wmsg;

	memset(&wmsg,0,sizeof(wmsg));	
	
	for (;;) {

		while (connect_woomera(woomera_socket, profile, 0) < 0) {
			if(!woomera_profile_thread_running(profile, 0, 0)) {
				break;
			}
			if (globals.panic > 2) {
				break;
			}
			ast_log(LOG_NOTICE, 
				"Woomera {%s} Cannot Reconnect! retry in 5 seconds...\n", 
				profile->name);

			/* When we establish connection update smg version */
			profile->smgversion[0]='\0';
			sleep(5);
		}

		if (*woomera_socket > -1) {
			if (ast_test_flag(profile, PFLAG_INBOUND)) {
				int err;
				if (globals.debug > 2) {
				ast_log(LOG_NOTICE, "Woomera Master Socket \n");
				}

				err=woomera_printf(profile, *woomera_socket, "LISTEN %s", WOOMERA_RECORD_SEPARATOR);
				if (err<0) {
					if (*woomera_socket > -1) {
						woomera_close_socket(woomera_socket);
					}
					continue;
				}

				if (woomera_message_parse(*woomera_socket,
										  &wmsg,
										  WOOMERA_HARD_TIMEOUT,
										  profile,
										  &profile->event_queue
										  ) < 0) {
					ast_log(LOG_ERROR, "{%s} %s:%d HELP! Woomera is broken!\n", 
							profile->name,__FUNCTION__,__LINE__);
					if (*woomera_socket > -1) {
						woomera_close_socket(woomera_socket);
						profile->smgversion[0]='\0';
					}
					continue;
				}
			}

		}
		usleep(100);
		break;
	}	
	return *woomera_socket;
}

static void tech_monitor_in_one_thread(void) 
{
	private_object *tech_pvt;

	ASTOBJ_CONTAINER_TRAVERSE(&private_object_list, 1, do {
		ASTOBJ_RDLOCK(iterator);
        tech_pvt = iterator;
		tech_monitor_thread(tech_pvt);
		ASTOBJ_UNLOCK(iterator);
    } while(0));
}

static void *woomera_thread_run(void *obj) 
{

	int woomera_socket = -1, res = 0, res2=0;
	woomera_message wmsg;
	woomera_profile *profile;

	memset(&wmsg,0,sizeof(wmsg));

	profile = obj;
	ast_log(LOG_NOTICE, "Started Woomera Thread {%s}.\n", profile->name);
 
	profile->thread_running = 1;


	while(woomera_profile_thread_running(profile, 0, 0)) {
		/* listen on socket and handle events */

		if (globals.panic > 2) {
			break;
		}

		if (globals.panic == 2) {
			ast_log(LOG_NOTICE, "Woomera is disabled!\n");
			sleep(5);
			continue;
		}

		if (woomera_socket < 0) {
			if (woomera_locate_socket(profile, &woomera_socket)) {
				globals.panic = 0;
			}
			if (!woomera_profile_thread_running(profile, 0, 0)) {
				break;
			}
			profile->woomera_socket=woomera_socket;
			ast_log(LOG_NOTICE, "Woomera Thread Up {%s} %s/%d\n", 
					profile->name, profile->woomera_host, profile->woomera_port);

		}

		if (globals.panic) {
			if (globals.panic != 2) {
				ast_log(LOG_ERROR, "Help I'm in a state of panic!\n");
			}
			if (woomera_socket > -1) {
				woomera_close_socket(&woomera_socket);
				profile->woomera_socket=-1;
			}
			continue;
		}
		if (!globals.more_threads) {
			if (woomera_socket > -1) {
				tech_monitor_in_one_thread();
			}
		}

		if ((res = woomera_dequeue_event(&profile->event_queue, &wmsg) ||
		    (res2 = woomera_message_parse(woomera_socket,
						  &wmsg,
					  /* if we are not stingy with threads we can block forever */
						  globals.more_threads ? 0 : 500,
						  profile,
						  NULL
						  )))) {


			if (res2 < 0) {
				ast_log(LOG_ERROR, "{%s} HELP! I lost my connection to woomera!\n", profile->name);
				if (woomera_socket > -1) {
					woomera_close_socket(&woomera_socket);
					profile->woomera_socket=-1;
				}
				global_set_flag(profile,TFLAG_ABORT);
				if (globals.panic > 2) {
					break;
				}

				continue;

				if (woomera_socket > -1) {
					if (ast_test_flag(profile, PFLAG_INBOUND)) {
						if (globals.debug > 2) {
						ast_log(LOG_NOTICE, "%s:%d Incoming Call \n",__FUNCTION__,__LINE__);
						}

#if 0
/* We only want a single listener */
						woomera_printf(profile, woomera_socket, "LISTEN%s", WOOMERA_RECORD_SEPARATOR);
						if(woomera_message_parse(woomera_socket,
												 &wmsg,
												 WOOMERA_HARD_TIMEOUT,
												 profile,
												 &profile->event_queue
												 ) < 0) {
							ast_log(LOG_ERROR, "{%s} %s:%d HELP! Woomera is broken!\n", profile->name,__FUNCTION__,__LINE__);
							woomera_close_socket(&woomera_socket);
							profile->woomera_socket=-1;
						}
#endif 
					}
					if (woomera_socket > -1) {
						ast_log(LOG_NOTICE, "Woomera Thread Up {%s} %s/%d\n", profile->name, profile->woomera_host, profile->woomera_port);
					}
				}
				continue;
			}

			if (!strcasecmp(wmsg.command, "INCOMING")) {
			
				int err=1;
				int cause = 0;
				struct ast_channel *inchan;
				char *name = "Woomera";

				if (!(name = woomera_message_header(&wmsg, "Channel-Name"))) {
					name = woomera_message_header(&wmsg,"Remote-Address");
				}

				if (!name) {
					name=wmsg.callid;
				}

				if (!name) {
					name="smg";
				}

				if (globals.debug > 2) {	
					ast_log(LOG_NOTICE, "NEW INBOUND CALL %s!\n",wmsg.callid);
				}

				if ((inchan = woomera_new(WOOMERA_CHAN_NAME, profile->coding, name, &cause, profile))) {
					private_object *tech_pvt;
					char *callid;
					tech_pvt = inchan->tech_pvt;
					
					/* Save the call id */
					tech_pvt->call_info = wmsg;
					memcpy(tech_pvt->callid,wmsg.callid,sizeof(tech_pvt->callid));
					
					callid = woomera_message_header(&wmsg, "Unique-Call-Id");
					if (callid) {
						ast_copy_string(tech_pvt->callid,
							callid,sizeof(wmsg.callid));
					}
					
					err=tech_init(tech_pvt, profile, TFLAG_INBOUND);
					if (err) {
						if(globals.debug > 2) {
							ast_log(LOG_ERROR, "Error: Inbound Call Failed %s %p\n",
									wmsg.callid,
									tech_pvt);
						}
						tech_destroy(tech_pvt,inchan);
					}
				} else {
					ast_log(LOG_ERROR, "Cannot Create new Inbound Channel!\n");
				}
			
				/* It is the job of the server to timeout on this call
                                   if the call is not started */
			}
		}
		if(globals.debug > 4) {
			if (option_verbose > 2 && woomera_profile_verbose(profile) > 2) {
				ast_verbose(WOOMERA_DEBUG_PREFIX "Main Thread {%s} Select Return %d\n", profile->name, res);
			}
		}
		usleep(100);
	}
	

	if (woomera_socket > -1) {
		int err;
		err=woomera_printf(profile, woomera_socket, "BYE%s", WOOMERA_RECORD_SEPARATOR);
		if(err<0 || woomera_message_parse(woomera_socket,
								 &wmsg,
								 WOOMERA_HARD_TIMEOUT,
								 profile,
								 &profile->event_queue
								 ) < 0) {
		}
		woomera_close_socket(&woomera_socket);
		profile->woomera_socket=-1;
	}

	ast_set_flag(profile, PFLAG_DISABLED);

	ast_log(LOG_NOTICE, "Ended Woomera Thread {%s}.\n", profile->name);
	woomera_profile_thread_running(profile, 1, -1);
	return NULL;
}

static void launch_woomera_thread(woomera_profile *profile) 
{
	pthread_attr_t attr;
	int result = 0;

	result = pthread_attr_init(&attr);
	pthread_attr_setschedpolicy(&attr, SCHED_RR);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	result = ast_pthread_create(&profile->thread, &attr, woomera_thread_run, profile);
	result = pthread_attr_destroy(&attr);
}


static int launch_tech_thread(private_object *tech_pvt) 
{
	pthread_attr_t attr;
	int result = 0;

	if (globals.debug > 2) {
		if (option_verbose > 2 && woomera_profile_verbose(tech_pvt->profile) > 2) {
			ast_verbose(WOOMERA_DEBUG_PREFIX "+++LAUCN TECH THREAD\n");
		}
	}

	if (ast_test_flag(tech_pvt, TFLAG_TECHHANGUP)) {
		/* Sanity check should never happen */
		ast_log(LOG_NOTICE,"Tech Thread failed call already hangup!\n");
		return -1;
	} 

	result = pthread_attr_init(&attr);
	pthread_attr_setschedpolicy(&attr, SCHED_RR);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	ast_set_flag(tech_pvt, TFLAG_INTHREAD);
	result = ast_pthread_create(&tech_pvt->thread, &attr, tech_monitor_thread, tech_pvt);
	if (result) {
		ast_clear_flag(tech_pvt, TFLAG_INTHREAD);
		ast_log(LOG_ERROR, "Error: Failed to launch tech thread %s\n",
				strerror(errno));
	}
	pthread_attr_destroy(&attr);
	
	return result;
}

static void woomera_config_gain(woomera_profile *profile, float gain_val, int rx) 
{
	int j;
	int k;
	float linear_gain = pow(10.0, gain_val / 20.0);
	unsigned char *gain;

	if (profile->coding == AST_FORMAT_SLINEAR){
		ast_log(LOG_WARNING, "Coding not specified, %s value ignored\n", (rx)? "rxgain":"txgain");
		return;
	}
	
		
	if (gain_val == 0) {
		goto woomera_config_gain_skip;
	}
	if (rx) {
		gain = profile->rxgain;
	} else {
		gain = profile->txgain;
	}
	
	switch (profile->coding) {
	
	case AST_FORMAT_ALAW:
		for (j = 0; j < 256; j++) {
			if (gain_val) {
				k = (int) (((float) alaw_to_linear(j)) * linear_gain);
				if (k > 32767) k = 32767;
				if (k < -32767) k = -32767;
				gain[j] = linear_to_alaw(k);
			} else {
				gain[j] = j;
			}
		}
		break;
	case AST_FORMAT_ULAW:
		for (j = 0; j < 256; j++) {
			if (gain_val) {
				k = (int) (((float) ulaw_to_linear(j)) * linear_gain);
				if (k > 32767) k = 32767;
				if (k < -32767) k = -32767;
				gain[j] = linear_to_ulaw(k);
			} else {
				gain[j] = j;
			}
		}
		break;
	}	

woomera_config_gain_skip:

	if (rx) {
		profile->rxgain_val=gain_val;
	} else {
		profile->txgain_val=gain_val;
	}

}

static void destroy_woomera_profile(woomera_profile *profile) 
{
	int i;
	if (profile && ast_test_flag(profile, PFLAG_DYNAMIC)) {
		for ( i = 0; i <= WOOMERA_MAX_TRUNKGROUPS; i++){
			if(profile->tg_context[i] != NULL){
				ast_free(profile->tg_context[i]);
			}
			if(profile->tg_language[i] != NULL){
				ast_free(profile->tg_language[i]);
			}
		} 
		ast_mutex_destroy(&profile->iolock);
		ast_mutex_destroy(&profile->call_count_lock);
		ast_mutex_destroy(&profile->event_queue.lock);
		ast_free(profile);
	}
}

static woomera_profile *clone_woomera_profile(woomera_profile *new_profile, woomera_profile *default_profile) 
{
	memcpy(new_profile, default_profile, sizeof(woomera_profile));
        memset(new_profile->tg_context, 0,sizeof(new_profile->tg_context));
        return new_profile;
}

static woomera_profile *create_woomera_profile(woomera_profile *default_profile) 
{
	woomera_profile *profile;

	if((profile = ast_malloc(sizeof(woomera_profile)))) {
		clone_woomera_profile(profile, default_profile);
		ast_mutex_init(&profile->iolock);
		ast_mutex_init(&profile->call_count_lock);
		ast_mutex_init(&profile->event_queue.lock);
		ast_set_flag(profile, PFLAG_DYNAMIC);
	}
	return profile;
}

static int config_woomera(void) 
{
	struct ast_config *cfg;
#if defined (AST16)
	struct ast_flags config_flags = {0};
#endif
	char *entry;
	struct ast_variable *v;
	woomera_profile *profile;
	int default_context_set = 0;
	int count = 0;

	memset(&default_profile, 0, sizeof(default_profile));
#if defined (AST_JB)
	memcpy(&global_jbconf, &default_jbconf, sizeof(struct ast_jb_conf));
#endif
	
	default_profile.coding=0;

#if defined (AST16)
	if ((cfg = ast_config_load(configfile, config_flags))) {
#else
	if ((cfg = ast_config_load(configfile))) {
#endif
		for (entry = ast_category_browse(cfg, NULL); entry != NULL; entry = ast_category_browse(cfg, entry)) {
			if (strcmp(entry, "settings") == 0) {
				
				for (v = ast_variable_browse(cfg, entry); v ; v = v->next) {

					if (!strcmp(v->name, "debug")) {
						globals.debug = atoi(v->value);
					} else if (!strcmp(v->name, "more_threads")) {
						globals.more_threads = ast_true(v->value);
					}
#if defined (AST_JB)
					if (ast_jb_read_conf(&global_jbconf, v->name, v->value) == 0) {
						ast_log(LOG_NOTICE, "Woomera AST JB Opt %s = %s \n",	
								v->name,v->value);
						continue;
					}
#endif
				}



			} else {
				int new = 0;
				float gain;
				count++;
				if (!strcmp(entry, "default")) {
					profile = &default_profile;
				} else {
					if((profile = ASTOBJ_CONTAINER_FIND(&woomera_profile_list, entry))) {
						clone_woomera_profile(profile, &default_profile);
					} else {
						if((profile = create_woomera_profile(&default_profile))) {
							new = 1;
						} else {
							ast_log(LOG_ERROR, "Memory Error!\n");
						}
					}
				}
				default_context_set = 0;
				strncpy(profile->name, entry, sizeof(profile->name) - 1);
				profile->coding=AST_FORMAT_SLINEAR;
				profile->verbose=100;
				profile->smgversion[0]='\0';

				/*default is inbound and outbound enabled */
				ast_set_flag(profile, PFLAG_INBOUND | PFLAG_OUTBOUND);
				for (v = ast_variable_browse(cfg, entry); v ; v = v->next) {
					if (!strcmp(v->name, "audio_ip")) {
						strncpy(profile->audio_ip, v->value, sizeof(profile->audio_ip) - 1);
					} else if (!strcmp(v->name, "host")) {
						strncpy(profile->woomera_host, v->value, sizeof(profile->woomera_host) - 1);
					} else if (!strcmp(v->name, "max_calls")) {
						int max = atoi(v->value);
						if (max > 0) {
							profile->max_calls = max;
						}
					} else if (!strcmp(v->name, "port")) {
						profile->woomera_port = atoi(v->value);
					} else if (!strcmp(v->name, "disabled")) {
						ast_set2_flag(profile, ast_true(v->value), PFLAG_DISABLED);
					} else if (!strcmp(v->name, "inbound")) {
						if (ast_false(v->value)) {
							ast_clear_flag(profile, PFLAG_INBOUND);
						}
					} else if (!strcmp(v->name, "outbound")) {
						if (ast_false(v->value)) {
							ast_clear_flag(profile, PFLAG_OUTBOUND);
						}
					} else if (!strcmp(v->name, "context")) {
						if(!default_context_set){
							default_context_set=1;
							strncpy(profile->default_context, v->value, sizeof(profile->default_context) - 1);
						}
						strncpy(profile->context, v->value, sizeof(profile->context) - 1);
					} else if (!strcmp(v->name, "default_context")) {
						default_context_set=1;
						strncpy(profile->default_context, v->value, sizeof(profile->default_context) - 1);
					} else if (!strcmp(v->name, "group")) {
						int group_num = atoi(v->value);
						if (group_num < 0) {
							ast_log(LOG_ERROR, "Invalid group:%d (less than zero) - ignoring\n", group_num);	
						} else if (group_num > WOOMERA_MAX_TRUNKGROUPS){
							ast_log(LOG_ERROR, "Invalid trunkgroup:%d (exceeds max:%d) -ignoring\n", group_num, WOOMERA_MAX_TRUNKGROUPS);
						} else {
							if(profile->tg_context[group_num] != NULL){
								ast_free(profile->tg_context[group_num]);
							}
							profile->tg_context[group_num] =  strdup(profile->context);

							if(profile->tg_language[group_num] != NULL){
								ast_free(profile->tg_language[group_num]);
							}
							if(strlen(profile->language)){
								profile->tg_language[group_num] = strdup(profile->language);
							}
						}
					} else if (!strcmp(v->name, "cid_pres")) {
						/* check if the value is valid */
						if ( (atoi(v->value) >= 0) && (atoi(v->value) <= 3)) {
							profile->cid_pres.pres = 1;
							profile->cid_pres.val = atoi(v->value);
						} else {
							ast_log(LOG_ERROR, "Invalid cid_pres value:%s - must be 0,1,2,or 3\n", v->value);
						}
					} else if (!strcmp(v->name, "language")) {
						strncpy(profile->language, v->value, sizeof(profile->language) - 1);
						
					} else if (!strcmp(v->name, "exten_blacklist")) {
						if (profile->called_ignore_idx < WOOMERA_MAX_CALLED_IGNORE) {
							strncpy(profile->called_ignore[profile->called_ignore_idx], v->value, WOOMERA_STRLEN - 1);
							profile->called_ignore_idx++;
						}
						
					} else if (!strcmp(v->name, "dtmf_enable")) {
						profile->dtmf_enable = atoi(v->value);

					} else if (!strcmp(v->name, "bridge_disable")) {
						profile->bridge_disable = atoi(v->value);

					} else if (!strcmp(v->name, "rbs_relay")) {
						profile->rbs_relay = atoi(v->value);
				
					} else if (!strcmp(v->name, "fax_detect")) {
						profile->faxdetect = atoi(v->value);
						ast_log(LOG_NOTICE, "Profile {%s} Fax Detect %s %p \n",
							entry, profile->faxdetect?"Enabled":"Disabled", profile);

					} else if (!strcmp(v->name, "jb_enable")) {
						profile->jb_enable = atoi(v->value);
						ast_log(LOG_NOTICE, "Profile {%s} Jitter Buffer %s %p \n",
							 entry,profile->jb_enable?"Enabled":"Disabled",profile);

					} else if (!strcmp(v->name, "jbenable")) {
                                                profile->jb_enable = atoi(v->value);
						ast_log(LOG_NOTICE, "Profile {%s} Jitter Buffer %s %p\n",
							 entry,profile->jb_enable?"Enabled":"Disabled",profile);

					} else if (!strcmp(v->name, "progress_enable")) {
                         profile->progress_enable = atoi(v->value);

					} else if (!strcmp(v->name, "progress_on_accept")) {
                         profile->progress_on_accept = atoi(v->value);
						
					} else if (!strcmp(v->name, "coding")) {
						if (strcmp(v->value, "alaw") == 0) {
							profile->coding=AST_FORMAT_ALAW;
						}	
						if (strcmp(v->value, "ulaw") == 0) {
							profile->coding=AST_FORMAT_ULAW;
						}	
					} else if (!strcmp(v->name, "base_media_port")) {
						int base_port = atoi(v->value);
						if (base_port > 0) {
							woomera_base_media_port = base_port;
							woomera_max_media_port = woomera_base_media_port + WOOMERA_MAX_MEDIA_PORTS;
						}

					} else if (!strcmp(v->name, "max_media_ports")) {
						int max_ports = atoi(v->value);
						if (max_ports > 0 && max_ports > WOOMERA_MAX_MEDIA_PORTS) {
							woomera_max_media_port = woomera_base_media_port + max_ports;
						}

					} else if (!strcmp(v->name, "rxgain") && profile->coding) {
                                                if (sscanf(v->value, "%f", &gain) != 1) {
							ast_log(LOG_NOTICE, "Invalid rxgain: %s\n", v->value);
						} else {
							woomera_config_gain(profile,gain,1);
						}	
					} else if (!strcmp(v->name, "txgain") && profile->coding) {
						 if (sscanf(v->value, "%f", &gain) != 1) {
							ast_log(LOG_NOTICE, "Invalid txgain: %s\n", v->value);
						} else {
							woomera_config_gain(profile,gain,0);
						}	
					} 
				}

				ASTOBJ_CONTAINER_LINK(&woomera_profile_list, profile);
			}
		}
		ast_config_destroy(cfg);
	} else {
		return 0;
	}

	return count;

}

static int create_udp_socket(char *ip, int port, struct sockaddr_in *sockaddr, int client)
{
	int rc, sd = -1;
	struct sockaddr_in servAddr, *addr, cliAddr;
	struct hostent hps, *hp;
	struct hostent *result;
	char buf[512];
	int err=0;

	memset(&hps,0,sizeof(hps));
	hp=&hps;

	if(sockaddr) {
		addr = sockaddr;
	} else {
		addr = &servAddr;
	}
	
	if ((sd = socket(AF_INET, SOCK_DGRAM, 0)) > -1) {
	
		gethostbyname_r(ip, hp, buf, sizeof(buf), &result, &err);
		if (result) {
			
			addr->sin_family = hp->h_addrtype;
			memcpy((char *) &addr->sin_addr.s_addr, hp->h_addr_list[0], hp->h_length);
			addr->sin_port = htons(port);

			if (globals.debug > 4) {	
			ast_log(LOG_NOTICE,"MEDIA UdpRead IP=%s/%d len=%i %d.%d.%d.%d\n",
						ip,port, 
						hp->h_length,
						hp->h_addr_list[0][0],
						hp->h_addr_list[0][1],
						hp->h_addr_list[0][2],
						hp->h_addr_list[0][3]);
			}

			if (client) {
				cliAddr.sin_family = AF_INET;
				cliAddr.sin_addr.s_addr = htonl(INADDR_ANY);
				cliAddr.sin_port = htons(0);
  				rc = bind(sd, (struct sockaddr *) &cliAddr, sizeof(cliAddr));
			} else {
				rc = bind(sd, (struct sockaddr *) addr, sizeof(cliAddr));
			}
			if (rc < 0) {
				if (globals.debug > 4) {
					ast_log(LOG_ERROR,
						"Error failed to bind to udp socket  %s/%i %s\n",
								ip, port, strerror(errno));
				}
				
				woomera_close_socket(&sd);
				
			} else if (globals.debug > 5) {
				ast_log(LOG_NOTICE, "Socket Binded %s to %s/%d\n", 
							client ? "client" : "server", ip, port);
			}
			
		} else {
			ast_log(LOG_ERROR,
					"Error opening udp: gethostbyname failed  %s/%i %s\n",
					ip,port, strerror(errno));
					
			woomera_close_socket(&sd);
		}
	} else {
		ast_log(LOG_ERROR,
					"Error failed to create a socket! %s/%i %s\n",
					ip,port, strerror(errno));
	}

	return sd;
}


static int connect_woomera(int *new_socket, woomera_profile *profile, int flags) 
{
	struct sockaddr_in localAddr, remoteAddr;
	struct hostent *hp, *result;
	struct hostent ahp;
	int res = 0, err=0;
	hp=&ahp;
	char buf[512];

	*new_socket=-1;

	gethostbyname_r(profile->woomera_host, hp, buf, sizeof(buf), &result, &err);
	if (result) {
		remoteAddr.sin_family = hp->h_addrtype;
		memcpy((char *) &remoteAddr.sin_addr.s_addr, hp->h_addr_list[0], hp->h_length);
		remoteAddr.sin_port = htons(profile->woomera_port);
		do {
			/* create socket */
			*new_socket = socket(AF_INET, SOCK_STREAM, 0);
			if (*new_socket < 0) {
				ast_log(LOG_ERROR, "cannot open socket to %s/%d\n", profile->woomera_host, profile->woomera_port);
				res = 0;
				break;
			}
			
			/* bind any port number */
			localAddr.sin_family = AF_INET;
			localAddr.sin_addr.s_addr = htonl(INADDR_ANY);
			localAddr.sin_port = htons(0);
  
			res = bind(*new_socket, (struct sockaddr *) &localAddr, sizeof(localAddr));
			if (res < 0) {
				ast_log(LOG_ERROR, "cannot bind to %s/%d\n", profile->woomera_host, profile->woomera_port);
				woomera_close_socket(new_socket);
				res = 0;
				break;
			}
		
			/* connect to server */
			res = connect(*new_socket, (struct sockaddr *) &remoteAddr, sizeof(remoteAddr));
			if (res < 0) {
				ast_log(LOG_ERROR, "cannot connect to {%s} %s/%d\n", profile->name, profile->woomera_host, profile->woomera_port);
				res = 0;
				woomera_close_socket(new_socket);
				break;
			}
			res = 1;
		} while(0);
		
	} else {
		if (globals.debug > 2) {
			ast_log(LOG_ERROR, "gethost failed connect to {%s} %s/%d\n", 
				profile->name, profile->woomera_host, profile->woomera_port);
		}
		res = 0;
	}
	if (res > 0) {
		int flag = 1;
		woomera_message wmsg;
		memset(&wmsg,0,sizeof(wmsg));

		/* disable nagle's algorythm */
		res = setsockopt(*new_socket, IPPROTO_TCP, TCP_NODELAY, (char *) &flag, sizeof(int));

		
		if (!(flags & WCFLAG_NOWAIT)) {
			/* kickstart the session waiting for a HELLO */
			res=woomera_printf(NULL, *new_socket, "%s", WOOMERA_RECORD_SEPARATOR);
			if (res < 0 ){
				woomera_close_socket(new_socket);
				return *new_socket;

			} 

			if ((res = woomera_message_parse(*new_socket,
								&wmsg,
								WOOMERA_HARD_TIMEOUT,
								profile,
								NULL
								)) < 0) {
				ast_log(LOG_ERROR, "{%s} Timed out waiting for a hello from woomera!\n", profile->name);
				woomera_close_socket(new_socket);
				return *new_socket;
			}
			
			
			if (res > 0 && strcasecmp(wmsg.command, "HELLO")) {
				ast_log(LOG_ERROR, "{%s} unexpected reply [%s] while waiting for a hello from woomera!\n", profile->name, wmsg.command);
				woomera_close_socket(new_socket);
				return *new_socket;

			}else{
				char *audio_format,*udp_seq, *media_pass;
				if (globals.debug > 2) {
					ast_log(LOG_NOTICE, "{%s} Woomera Got HELLO on connect! SMG Version %s\n", 
							profile->name,woomera_message_header(&wmsg, "Version"));
				}
				if (profile->smgversion[0] == '\0' && woomera_message_header(&wmsg, "Version")) {
					strncpy(profile->smgversion,
						woomera_message_header(&wmsg, "Version"),
						sizeof(profile->smgversion)-1);
				}
				
				udp_seq = woomera_message_header(&wmsg, "xUDP-Seq");
				if (udp_seq && strncasecmp(udp_seq,"Enabled",20) == 0) {
					if (profile->udp_seq == 0) {
						ast_log(LOG_NOTICE, "{%s} Woomera UDP Sequencing Enabled\n", profile->name);
                 	 	profile->udp_seq=1;   
					}
					udp_seq = woomera_message_header(&wmsg, "xUDP-Seq-Err");
					if (udp_seq) {
                     	int seq_err=atoi(udp_seq);
						if (seq_err > 0) {
							ast_mutex_lock(&profile->call_count_lock);
                        	if (seq_err > profile->media_tx_seq_err) {
                            	profile->media_tx_seq_err=seq_err; 	
							}
							ast_mutex_unlock(&profile->call_count_lock);
						}
					}
				} else {
					if (profile->udp_seq == 1) {
						ast_log(LOG_NOTICE, "{%s} Woomera UDP Sequencing Disabled\n", profile->name);
                 	 	profile->udp_seq=0;   
					}
				}
				
				media_pass = woomera_message_header(&wmsg, "xNative-Bridge");
				if (media_pass && strncasecmp(media_pass,"Enabled",20) == 0) {
					if (profile->media_pass_through == 0) {
						ast_log(LOG_NOTICE, "{%s} Woomera Media Pass Through Enabled\n", profile->name);	
					}
					profile->media_pass_through=1;	
				} else {
					if (profile->media_pass_through) {
						ast_log(LOG_NOTICE, "{%s} Woomera Media Pass Through Disable\n", profile->name);		
					}
					profile->media_pass_through=0;	
				}
				
					
				audio_format = woomera_message_header(&wmsg, "Raw-Format");
				if (!audio_format) {
					audio_format = woomera_message_header(&wmsg, "Raw-Audio-Format");
				}
				if (audio_format) {
					
					profile->coding=AST_FORMAT_SLINEAR;
				
					if (strncasecmp(audio_format,"PCM-16",20) == 0){
						profile->coding=AST_FORMAT_SLINEAR;
					} else if (strncasecmp(audio_format,"ULAW",15) == 0) {
						profile->coding=AST_FORMAT_ULAW;
					} else if (strncasecmp(audio_format,"ALAW",15) == 0) {
						profile->coding=AST_FORMAT_ALAW;
					} else {
						ast_log(LOG_ERROR, "Error: Invalid Raw-Format %s\n",
							audio_format);
					}
					
					if (globals.debug > 2) {
#ifdef AST18
					ast_log(LOG_NOTICE, "Setting RAW Format to %s %i (p%llu:u%llu:a%llu)\n",
							audio_format, profile->coding,
							AST_FORMAT_SLINEAR,AST_FORMAT_ULAW,AST_FORMAT_ALAW);
#else
					ast_log(LOG_NOTICE, "Setting RAW Format to %s %i (p%i:u%i:a%i)\n",
							audio_format, profile->coding,
							AST_FORMAT_SLINEAR,AST_FORMAT_ULAW,AST_FORMAT_ALAW);
#endif
					}
				}
			}
		}

	} else {
		if (globals.debug > 2) {
			ast_log(LOG_ERROR, "Woomera {%s} connection failed: %s/%d\n", 
				profile->name, profile->woomera_host, profile->woomera_port);
			ast_log(LOG_ERROR, "Woomera {%s} connection failed: %s\n", 
				profile->name, strerror(errno));
		}
		woomera_close_socket(new_socket);
	}

	return *new_socket;
}


static struct ast_channel *woomera_new(const char *type, int format, 
				       void *data, int *cause, 
				       woomera_profile *parent_profile)
{
	private_object *tech_pvt;
	struct ast_channel *chan = NULL;
	char name[100];

	snprintf(name, sizeof(name), "%s/%s-%04x", type, (char *)data, rand() & 0xffff);

	if (!(tech_pvt = ast_malloc(sizeof(private_object)))) {
		ast_log(LOG_ERROR, "Memory Error!\n");
		return NULL;
	}
	memset(tech_pvt, 0, sizeof(private_object));

#if defined(AST18)
  	//chan =  ast_channel_alloc(0, state, i->cid_num, i->cid_name, i->accountcode, i->exten, i->context, linkedid, i->amaflags, "DAHDI/%s", ast_str_buffer(chan_name));
	chan =  ast_channel_alloc(0, AST_STATE_DOWN, "", "", "", "", "", "", 0, "%s", name);
#elif defined (AST14) || defined (AST16)
	chan =  ast_channel_alloc(0, AST_STATE_DOWN, "", "", "", "", "", 0, "%s", name);
#else
	chan = ast_channel_alloc(1);
#endif
	if (chan) {
		chan->nativeformats = WFORMAT;
#if !defined (AST14) && !defined (AST16)
		chan->type = type;
		snprintf(chan->name, sizeof(chan->name), "%s/%s-%04x", chan->type, (char *)data, rand() & 0xffff);
#endif
		
		chan->writeformat = chan->rawwriteformat = chan->rawreadformat = chan->readformat = WFORMAT;
		chan->_state = AST_STATE_DOWN;
		chan->_softhangup = 0;
		
		tech_count++;
		tech_pvt->coding=WFORMAT;
		
		ast_mutex_init(&tech_pvt->iolock);
		ast_mutex_init(&tech_pvt->event_queue.lock);
		tech_pvt->command_channel = -1;
		chan->tech_pvt = tech_pvt;
		chan->tech = &technology;
		tech_pvt->udp_socket = -1;

		ast_clear_flag(chan, AST_FLAGS_ALL);
		memset(&tech_pvt->frame, 0, sizeof(tech_pvt->frame));
		tech_pvt->frame.frametype = AST_FRAME_VOICE;
		tech_pvt->frame.W_SUBCLASS_CODEC = WFORMAT;
		tech_pvt->frame.offset = AST_FRIENDLY_OFFSET;

		tech_pvt->owner = chan;

		chan->nativeformats = tech_pvt->coding;
		chan->writeformat = chan->rawwriteformat = chan->rawreadformat = chan->readformat = tech_pvt->coding;
		tech_pvt->frame.W_SUBCLASS_CODEC = tech_pvt->coding;

		tech_pvt->pri_cause=AST_CAUSE_NORMAL_CLEARING;
	
		ast_copy_string(tech_pvt->mohinterpret,mohinterpret,sizeof(tech_pvt->mohinterpret));
		ast_copy_string(tech_pvt->mohsuggest,mohsuggest,sizeof(tech_pvt->mohsuggest));

		ASTOBJ_CONTAINER_LINK(&private_object_list, tech_pvt);
		
		ast_mutex_lock(&usecnt_lock);
		usecnt++;
		ast_mutex_unlock(&usecnt_lock);

	} else {
		ast_log(LOG_ERROR, "Can't allocate a channel\n");
	}
	

	return chan;
}




/********************CHANNEL METHOD LIBRARY********************
 * This is the actual functions described by the prototypes above.
 *
 */


/*--- tech_requester: parse 'data' a url-like destination string, allocate a channel and a private structure
 * and return the newly-setup channel.
 */
#if defined (AST18)
static struct ast_channel *tech_requester(const char *type, format_t format, const struct ast_channel *requestor, void *data, int *cause)
#else
static struct ast_channel *tech_requester(const char *type, int format, void *data, int *cause)
#endif
{
	struct ast_channel *chan = NULL;
			

	if (globals.panic) {
		return NULL;
	}

	if ((chan = woomera_new(type, format, data, cause, NULL))) {
		private_object *tech_pvt;
	
		tech_pvt = chan->tech_pvt;

		if (tech_pvt->owner) {
			tech_pvt->owner->hangupcause = AST_CAUSE_NORMAL_CLEARING;
		}

		ast_set_flag(tech_pvt, TFLAG_PBX); /* so we know we dont have to free the channel ourselves */
	
		if (globals.debug > 1 && option_verbose > 8) {
		   	ast_verbose(WOOMERA_DEBUG_PREFIX "+++REQ %s\n", chan->name);
		}

	} else {
		ast_log(LOG_ERROR, "Woomera: Can't allocate a channel\n");
	}


	return chan;
}

#if defined (AST14) || defined (AST16)
static int tech_digit_end(struct ast_channel *ast, char digit, unsigned int duration)
{
	return 0;
}
#endif

/*--- tech_senddigit: Send a DTMF character */
static int tech_send_digit(struct ast_channel *self, char digit)
{
	private_object *tech_pvt = self->tech_pvt;
	int res = 0;

	if (globals.debug > 1 && option_verbose > 2 && woomera_profile_verbose(tech_pvt->profile) > 2) {
	   	ast_verbose(WOOMERA_DEBUG_PREFIX "+++DIGIT %s '%c'\n",self->name, digit);
	}

	/* we don't have time to make sure the dtmf command is successful cos asterisk again 
	   is much too impaitent... so we will cache the digits so the monitor thread can send
	   it for us when it has time to actually wait.
	*/
	ast_mutex_lock(&tech_pvt->iolock);
	snprintf(tech_pvt->dtmfbuf + strlen(tech_pvt->dtmfbuf), sizeof(tech_pvt->dtmfbuf), "%c", digit);
	ast_set_flag(tech_pvt, TFLAG_DTMF);
	ast_mutex_unlock(&tech_pvt->iolock);

	return res;
}

/*--- tech_senddigit: Send a DTMF character */
static int tech_send_rbs(struct ast_channel *self, unsigned char digit)
{
	private_object *tech_pvt = self->tech_pvt;
	int res = 0;

	if (globals.debug > 1 && option_verbose > 2 && woomera_profile_verbose(tech_pvt->profile) > 2) {
	   	ast_verbose(WOOMERA_DEBUG_PREFIX "+++RBS %s '%X'\n",self->name, digit);
	}

	/* we don't have time to make sure the dtmf command is successful cos asterisk again 
	   is much too impaitent... so we will cache the digits so the monitor thread can send
	   it for us when it has time to actually wait.
	*/
	ast_mutex_lock(&tech_pvt->iolock);
	tech_pvt->rbsbuf=digit;
	ast_set_flag(tech_pvt, TFLAG_RBS);
	ast_mutex_unlock(&tech_pvt->iolock);

	return res;
}


/*--- tech_call: Initiate a call on my channel 
 * 'dest' has been passed telling you where to call
 * but you may already have that information from the requester method
 * not sure why it sends it twice, maybe it changed since then *shrug*
 * You also have timeout (in ms) so you can tell how long the caller
 * is willing to wait for the call to be complete.
 */

static int tech_call(struct ast_channel *self, char *dest, int timeout)
{
	private_object *tech_pvt = self->tech_pvt;
	char *workspace;
	char *p;
	char *c;

#if !defined (AST14) && !defined (AST16)
	self->type = WOOMERA_CHAN_NAME;	
#endif
	
 	self->hangupcause = AST_CAUSE_NORMAL_CIRCUIT_CONGESTION;

	if (globals.panic) {
		goto tech_call_failed;
	}
	if (globals.debug > 2) {
		ast_log(LOG_NOTICE, "TECH CALL %s (%s <%s>)  pres=0x%02X dest=%s\n",
			self->name, self->W_CID_NAME, 
			self->W_CID_NUM,
			self->W_CID_NUM_PRES,
			dest);
	}

	if (self->W_CID_NAME) {
		strncpy(tech_pvt->cid_name, self->W_CID_NAME, sizeof(tech_pvt->cid_name)-1);
	}
	if (self->W_CID_NUM) {
		strncpy(tech_pvt->cid_num, self->W_CID_NUM, sizeof(tech_pvt->cid_num)-1);
	}
	tech_pvt->cid_pres = self->W_CID_NUM_PRES;


	if (self->W_CID_FROM_RDNIS) {
		tech_pvt->cid_rdnis=strdup(self->W_CID_FROM_RDNIS);
	}

	if ((workspace = ast_strdupa(dest))) {
		char *addr, *profile_name, *proto=NULL;
		woomera_profile *profile;
		int err;
		

#if 0
		int isprofile = 0;


		if ((addr = strchr(workspace, ':'))) {
			char *tst;
			proto = workspace;
			if ((tst = strchr(proto, '/'))){
				proto=tst+1;
			}
			*addr = '\0';
			addr++;
		} else {
			proto = NULL;
			addr = workspace;
		}

			

	
		if ((profile_name = strchr(addr, ':'))) {
			*profile_name = '\0';
			profile_name++;
			isprofile = 1;
		} else {
			profile_name = "default";
		}
#else
		profile_name = "default";
                proto = NULL;
		if ((addr = strchr(workspace, ':'))) {
                        profile_name = workspace;
			proto=profile_name;
                        *addr = '\0';
                        addr++;
                } else {
                        addr = workspace;
                }

#endif
		
		if (! (profile = ASTOBJ_CONTAINER_FIND(&woomera_profile_list, profile_name))) {
			profile = ASTOBJ_CONTAINER_FIND(&woomera_profile_list, "default");
		}
		
		if (!profile) {
			ast_log(LOG_ERROR, "Unable to find profile! Call Aborted!\n");
			goto tech_call_failed;
		}

		/* if cid_pres.pres then the user wants to manually overwrite */
		if (profile->cid_pres.pres) {
			tech_pvt->cid_pres = profile->cid_pres.val;
			ast_log(LOG_DEBUG, "CID presentation override : was %d, now %d\n",
								self->W_CID_NUM_PRES,
								tech_pvt->cid_pres);
		}
 
		if (!ast_test_flag(profile, PFLAG_OUTBOUND)) {
			ast_log(LOG_ERROR, "This profile is not allowed to make outbound calls! Call Aborted!\n");
			goto tech_call_failed;
		}

		if (profile->max_calls) {
			if (profile->call_count >= profile->max_calls) {
				if (globals.debug > 1 && option_verbose > 2 && woomera_profile_verbose(profile) > 2) {
					ast_log(LOG_ERROR, "This profile is at call limit of %d\n",
						 profile->max_calls);
				}
				goto tech_call_failed;
			} 
		}

		snprintf(tech_pvt->dest, sizeof(tech_pvt->dest), "%s", addr ? addr : "");
		snprintf(tech_pvt->proto, sizeof(tech_pvt->proto), "%s", proto ? proto : "");

		if (globals.debug > 2) {
		ast_log(LOG_NOTICE, "TECH CALL: proto=%s addr=%s profile=%s Coding=%i\n",
					proto,addr,profile_name,profile->coding);
		}

		tech_pvt->timeout = timeout;
		err=tech_init(tech_pvt, profile, TFLAG_OUTBOUND);
		if (err) {	
			if (globals.debug > 2) {
				ast_log(LOG_ERROR, "Error: Outbound Call Failed \n");
			}
			goto tech_call_failed;
		}
		
#if 1		
		if ((p = strrchr(self->name, '/'))) {
			c = p-1;
			if(*c == 'c' || *c== 'C'){
				ast_set_flag(tech_pvt, TFLAG_CONFIRM_ANSWER_ENABLED);
				ast_set_flag(tech_pvt, TFLAG_CONFIRM_ANSWER);
			}
		}
#endif	
		
		woomera_send_progress(tech_pvt);
	}
	self->hangupcause = AST_CAUSE_NORMAL_CLEARING;


	return 0;

tech_call_failed:
	if (globals.debug > 1 && option_verbose > 1) {
		ast_log(LOG_ERROR, "Error: Outbound Call Failed %p \n",tech_pvt);
	}
 	self->hangupcause = AST_CAUSE_NORMAL_CIRCUIT_CONGESTION;
	my_ast_softhangup(self, tech_pvt, AST_SOFTHANGUP_EXPLICIT);
	return -1;
}


/*--- tech_hangup: end a call on my channel 
 * Now is your chance to tear down and free the private object
 * from the channel it's about to be freed so you must do so now
 * or the object is lost.  Well I guess you could tag it for reuse
 * or for destruction and let a monitor thread deal with it too.
 * during the load_module routine you have every right to start up
 * your own fancy schmancy bunch of threads or whatever else 
 * you want to do.
 */
static int tech_hangup(struct ast_channel *self)
{
	const char *ds;
	private_object *tech_pvt = self->tech_pvt;
	int res = 0;

	if (globals.debug > 2) {
		ast_log(LOG_NOTICE, "TECH HANGUP [%s] tech_pvt=%p c=%p\n",self->name,tech_pvt,self);
	}


	if (tech_pvt) {

		ast_mutex_lock(&tech_pvt->iolock);

		ast_set_flag(tech_pvt, TFLAG_TECHHANGUP);
		tech_pvt->owner=NULL;
		self->tech_pvt=NULL;



		if (!self || 
		   (!(ds = pbx_builtin_getvar_helper(self, "DIALSTATUS")) && 
		    !(ds = ast_cause2str(self->hangupcause)))) {
			ds = "NOEXIST";
		}

		ast_copy_string(tech_pvt->ds, ds, sizeof(tech_pvt->ds));

		ds=pbx_builtin_getvar_helper(self, "PRI_CAUSE");
		if (ds && atoi(ds)) {			
			tech_pvt->pri_cause=atoi(ds);
		} else if (self->hangupcause) {
			tech_pvt->pri_cause=self->hangupcause;
		} else {
			tech_pvt->pri_cause=AST_CAUSE_NORMAL_CLEARING;
		}

		if (tech_pvt->pri_cause > 127) {
			ast_log(LOG_NOTICE, "Invalid HangUp Cause %i\n", tech_pvt->pri_cause);
			tech_pvt->pri_cause = AST_CAUSE_NORMAL_UNSPECIFIED;
		}

		if (globals.debug > 2) {
			ast_log(LOG_NOTICE, "TECH HANGUP [%s] Cause=%i HangCause=%i ds=%s\n",
				self->name,tech_pvt->pri_cause, self->hangupcause,  ds?ds:"N/A");
		}

		if (tech_pvt->dsp) {
	                tech_pvt->dsp_features &= ~DSP_FEATURE_DTMF_DETECT;
        	        ast_dsp_set_features(tech_pvt->dsp, tech_pvt->dsp_features);
                	tech_pvt->ast_dsp=0;
        	}
		

		if (ast_test_flag(tech_pvt, TFLAG_INTHREAD)) {
			ast_set_flag(tech_pvt, TFLAG_ABORT);
			ast_set_flag(tech_pvt, TFLAG_DESTROY);

			if (globals.debug > 2) {
				ast_log(LOG_NOTICE, "TECH HANGUP IN THREAD! tpvt=%p\n",
						tech_pvt);
			}
	
			self->tech_pvt = NULL;
			ast_mutex_unlock(&tech_pvt->iolock);

		} else {
			int ast_hangup=0;

			if (globals.debug > 4) {
			ast_log(LOG_NOTICE, "TECH HANGUP:  Destroying tech not in thread! Callid=%s tech_pvt=%p Dir=%s\n",
							tech_pvt->callid, tech_pvt, 
							ast_test_flag(tech_pvt, TFLAG_OUTBOUND)?"OUT":"IN" );
			}
					
			if (ast_test_flag(tech_pvt, TFLAG_AST_HANGUP)) {
				ast_hangup=1;
			}
			self->tech_pvt = NULL;
			ast_mutex_unlock(&tech_pvt->iolock);

			if (!ast_test_flag(tech_pvt, TFLAG_DESTROY)) {

				if (ast_hangup && tech_pvt->profile) {
					ast_mutex_lock(&tech_pvt->profile->call_count_lock);
					tech_pvt->profile->call_ast_hungup--;
					ast_mutex_unlock(&tech_pvt->profile->call_count_lock);
				}

				tech_destroy(tech_pvt,NULL);
				if (globals.debug > 2) {
					ast_log(LOG_NOTICE, "TECH HANGUP NOT IN THREAD!\n");
				}
			}else{
				ast_log(LOG_ERROR, "Tech Hangup -> Device already destroyed. Should not happend! \n");
			}
		}
	} else {
		if (globals.debug > 2) {
			ast_log(LOG_NOTICE, "ERROR: NO TECH ON TECH HANGUP!\n");
		}
	}

	self->tech_pvt = NULL;
	
	return res;
}

/*--- tech_answer: answer a call on my channel
 * if being 'answered' means anything special to your channel
 * now is your chance to do it!
 */
static int tech_answer(struct ast_channel *self)
{
	private_object *tech_pvt;
	int res = 0;
	const char *noanswer;

	tech_pvt = self->tech_pvt;
	if (!tech_pvt) {
		return -1;
	}

	ast_mutex_lock(&tech_pvt->iolock);

	if (globals.debug > 1 && option_verbose > 2 && woomera_profile_verbose(tech_pvt->profile) > 2) {
	   	ast_verbose(WOOMERA_DEBUG_PREFIX "+++ANSWER %s\n",self->name);
	}
	
	if (!ast_test_flag(tech_pvt, TFLAG_OUTBOUND)) {
		/* Only answer the inbound calls */
		noanswer=pbx_builtin_getvar_helper(self, "WOOMERA_NO_ANSWER");
		if (noanswer && atoi(noanswer) == 1) {
			ast_clear_flag(tech_pvt, TFLAG_ANSWER);
		} else {
			ast_set_flag(tech_pvt, TFLAG_ANSWER);
		}
	} else {
		ast_log(LOG_ERROR, "Warning: AST trying to Answer OUTBOUND Call!\n");
	}
 
	ast_set_flag(tech_pvt, TFLAG_UP);
	ast_setstate(self, AST_STATE_UP);

	ast_mutex_unlock(&tech_pvt->iolock);

	return res;
}


static void handle_fax(private_object *tech_pvt)
{
	struct ast_channel *owner;
		
	owner = tech_pvt->owner;

	if (tech_pvt->owner) {
		ast_verbose(WOOMERA_DEBUG_PREFIX "FAX TONE %s\n", tech_pvt->callid);

		if (!tech_pvt->faxhandled) {
			tech_pvt->faxhandled++;
			if (strcmp(owner->exten, "fax")) {
#if defined (AST14) || defined (AST16)
				const char *target_context = S_OR(owner->macrocontext, owner->context);
#else
				const char *target_context = ast_strlen_zero(owner->macrocontext) ? owner->context : owner->macrocontext;
#endif
				if (ast_exists_extension(owner, target_context, "fax", 1, owner->W_CID_NUM)) {
						if (option_verbose > 2 && woomera_profile_verbose(tech_pvt->profile) > 2) {
								ast_verbose(VERBOSE_PREFIX_3 "Redirecting %s to fax extension\n", owner->name);
						}
	
						pbx_builtin_setvar_helper(owner, "FAXEXTEN", owner->exten);
						if (ast_async_goto(owner, target_context, "fax", 1))
								ast_log(LOG_WARNING, "Failed to async goto '%s' into fax of '%s'\n", owner->name, target_context);
				} else {
						ast_log(LOG_NOTICE, "Fax detected, but no fax extension\n");
				}
			}
		}
	}
}




/*--- tech_read: Read an audio frame from my channel.
 * You need to read data from your channel and convert/transfer the
 * data into a newly allocated struct ast_frame object
 */
static struct ast_frame  *tech_read(struct ast_channel *self)
{
	private_object *tech_pvt = self->tech_pvt;
	int res = 0;
	struct ast_frame *f;

tech_read_again:

	if (!tech_pvt || globals.panic || ast_test_flag(tech_pvt, TFLAG_ABORT)) {
		return NULL;
	}

	res = waitfor_socket(tech_pvt->udp_socket, 1000);

	if (res < 1) {
		return NULL;

	}else if (res == 0) {
		goto tech_read_again;
	}

	res = read(tech_pvt->udp_socket, tech_pvt->fdata + AST_FRIENDLY_OFFSET, FRAME_LEN);

	if (res < 1) {
		return NULL;
	}

	
#if defined(AST18)
	/* NC: Kludge Had to do this for Ast1.8 the nativeformats */
	if (self->nativeformats != tech_pvt->coding) {
        self->nativeformats = tech_pvt->coding;
    }
#endif

	/* Used for adding sequence numbers to udp packets.
 	   should only be used for debugging */
	if (tech_pvt->profile->udp_seq){
		char *rxdata=(char*)(tech_pvt->fdata + AST_FRIENDLY_OFFSET);
		res-=4;

		if (tech_pvt->rx_udp_seq == 0) { 
			tech_pvt->rx_udp_seq = *((unsigned int*)(&rxdata[res]));
			if (globals.debug > 2) {
				ast_log(LOG_NOTICE, "%s: Starting Rx Sequence %i Len=%i!\n", self->name,tech_pvt->rx_udp_seq, res);
			}
		} else {
			tech_pvt->rx_udp_seq++;
			if (tech_pvt->rx_udp_seq != *((unsigned int*)(&rxdata[res]))) {
				if (globals.debug > 2) {
					ast_log(LOG_NOTICE, "%s: Error: Missing Rx Sequence Expect %i Received %i!\n", 
						self->name,tech_pvt->rx_udp_seq, *((unsigned int*)(&rxdata[res])));
				}
				tech_pvt->rx_udp_seq = *((unsigned int*)(&rxdata[res]));
				ast_mutex_lock(&tech_pvt->profile->call_count_lock);
				tech_pvt->profile->media_rx_seq_err++;
				ast_mutex_unlock(&tech_pvt->profile->call_count_lock);
			}
		}
	}

	/* Used for checking incoming udp stream. Should only be used for debugging. */
	if (tech_pvt->profile->rx_sync_check_opt){
		int i;
		unsigned char *data = (unsigned char*)(tech_pvt->fdata + AST_FRIENDLY_OFFSET);
		for (i=0;i<res;i++) {
			if (tech_pvt->sync_r == 0) {
				if (data[i] == 0x01) {
					if (globals.debug > 2) {
						ast_log(LOG_NOTICE, "%s: R Sync Acheived Offset=%i\n", self->name,i);
					}
					tech_pvt->sync_r=1;
					tech_pvt->sync_data_r = data[i];
				}
			} else {
				tech_pvt->sync_data_r++;
				if (tech_pvt->sync_data_r != data[i]) {
					ast_log(LOG_NOTICE, "%s: R Sync Lost: Expected %i  Got %i  Offset=%i\n", 
							self->name,
							tech_pvt->sync_data_r, data[i],i);
					tech_pvt->sync_r=0;
				}
			}
		}
	}


	tech_pvt->frame.frametype = AST_FRAME_VOICE;
	tech_pvt->frame.W_SUBCLASS_CODEC = tech_pvt->coding;
	tech_pvt->frame.offset = AST_FRIENDLY_OFFSET;
	tech_pvt->frame.datalen = res;
	tech_pvt->frame.samples = res;
	tech_pvt->frame.woo_ast_data_ptr = tech_pvt->fdata + AST_FRIENDLY_OFFSET;

	f=&tech_pvt->frame;

	if (tech_pvt->profile->rxgain_val) {
		int i;
		unsigned char *data=tech_pvt->frame.woo_ast_data_ptr;
		for (i=0;i<tech_pvt->frame.datalen;i++) {
			data[i]=tech_pvt->profile->rxgain[data[i]];
		}
	} 	

	if (tech_pvt->owner && !tech_pvt->ignore_dtmf && (tech_pvt->faxdetect || tech_pvt->ast_dsp)) {
		f = ast_dsp_process(tech_pvt->owner, tech_pvt->dsp, &tech_pvt->frame);
		if (f && f->frametype == AST_FRAME_DTMF){
			int answer = 0;

			if(ast_test_flag(tech_pvt, TFLAG_CONFIRM_ANSWER_ENABLED)){
				ast_mutex_lock(&tech_pvt->iolock);
				if(ast_test_flag(tech_pvt, TFLAG_CONFIRM_ANSWER)){	
					ast_clear_flag(tech_pvt, TFLAG_CONFIRM_ANSWER);
					if(ast_test_flag(tech_pvt, TFLAG_ANSWER_RECEIVED)){
						answer = 1;		
					}
				} 
				ast_mutex_unlock(&tech_pvt->iolock);
				if(answer){
					struct ast_frame answer_frame = {AST_FRAME_CONTROL, .W_SUBCLASS_INT = AST_CONTROL_ANSWER};
					struct ast_channel *owner = tech_get_owner(tech_pvt);
					ast_log(LOG_DEBUG, "Confirm answer on %s!\n", self->name);
				
					if (owner) {
						ast_setstate(owner, AST_STATE_UP);
						ast_queue_frame(owner, &answer_frame);
						ast_set_flag(tech_pvt, TFLAG_UP);
					
						ast_mutex_lock(&tech_pvt->profile->call_count_lock);
						tech_pvt->profile->call_ok++;
						ast_mutex_unlock(&tech_pvt->profile->call_count_lock);
					} else {
						ast_copy_string(tech_pvt->ds, "REQUESTED_CHAN_UNAVAIL", sizeof(tech_pvt->ds));
						tech_pvt->pri_cause=44;
						ast_set_flag(tech_pvt, TFLAG_ABORT);
								
					}
				}
			}
			if (f->W_SUBCLASS_INT == 'f' && tech_pvt->faxdetect) {
				handle_fax(tech_pvt);
			}
			if (answer == 0 && globals.debug > 2) {
				ast_log(LOG_NOTICE, "%s: Detected inband DTMF digit: %c\n",
						self->name,
						f->W_SUBCLASS_INT);
			}
		}
		//woomera_tx2ast_frm(tech_pvt, tech_pvt->frame.data, tech_pvt->frame.datalen);
	}
	
	
	if (globals.debug > 4) {
		if (option_verbose > 2 && woomera_profile_verbose(tech_pvt->profile) > 2) {
			ast_verbose(WOOMERA_DEBUG_PREFIX "+++READ %s %d coding %d\n",self->name, res,
					tech_pvt->coding);
		}
	}


	return f;
}

/*--- tech_write: Write an audio frame to my channel
 * Yep, this is the opposite of tech_read, you need to examine
 * a frame and transfer the data to your technology's audio stream.
 * You do not have any responsibility to destroy this frame and you should
 * consider it to be read-only.
 */
static int tech_write(struct ast_channel *self, struct ast_frame *frame)
{
	private_object *tech_pvt = self->tech_pvt;
	int res = 0, i = 0;
	 
	if (!tech_pvt || globals.panic || ast_test_flag(tech_pvt, TFLAG_ABORT)) {
		return -1;
	} 

	/* Used for debugging only never in production */
	if (tech_pvt->profile->tx_sync_check_opt){
		unsigned char *data = frame->woo_ast_data_ptr;
		for (i=0;i<frame->datalen;i++) {
			if (tech_pvt->sync_w == 0) {
				if (data[i] == 0x01 && data[i+1] == 0x02) {
					ast_log(LOG_NOTICE, "%s: W Sync Acheived Offset=%i\n", self->name,i);
					tech_pvt->sync_w=1;
					tech_pvt->sync_data_w = data[i];
				}
			} else if (tech_pvt->sync_w == 1) {
				tech_pvt->sync_data_w++;
				if (tech_pvt->sync_data_w != data[i]) {
					ast_log(LOG_NOTICE, "%s: W Sync Lost: Expected %i  Got %i  Offset=%i\n", 
							self->name,
							tech_pvt->sync_data_w, data[i],i);
					tech_pvt->sync_w=0;
					if (0){
						int x;
						ast_log(LOG_NOTICE, "%s: PRINTING FRAME Len=%i\n", 
								self->name,frame->datalen);
						for (x=0;x<frame->datalen;x++) {
							ast_log(LOG_NOTICE, "%s: Off=%i Data %i\n", 
								self->name,x,data[x]);
						}
					}
				}
			}
		}

	/* Used for debugging only never in production */
	} else if (tech_pvt->profile->tx_sync_gen_opt){
		unsigned char *data = frame->woo_ast_data_ptr;
		int x;
		for (x=0;x<frame->datalen;x++) {
			data[x]=++tech_pvt->sync_data_w;
		}
	}


	if(ast_test_flag(tech_pvt, TFLAG_MEDIA) && frame->datalen) {
		if (frame->frametype == AST_FRAME_VOICE) {
		
			if (tech_pvt->profile->txgain_val) {
				unsigned char *data=frame->woo_ast_data_ptr;
				for (i=0;i<frame->datalen;i++) {
					data[i]=tech_pvt->profile->txgain[data[i]];
				}
			} 

			if (tech_pvt->profile->udp_seq){	
				unsigned char *txdata=frame->woo_ast_data_ptr;
				*((unsigned int*)&txdata[frame->datalen]) = tech_pvt->tx_udp_seq;
				tech_pvt->tx_udp_seq++;
				frame->datalen+=4;
			}
 
			i = sendto(tech_pvt->udp_socket, frame->woo_ast_data_ptr, frame->datalen, 0, 
				   (struct sockaddr *) &tech_pvt->udpwrite, sizeof(tech_pvt->udpwrite));
			if (i < 0) {
				return -1;
			}
			if (globals.debug > 4) {
				if (option_verbose > 9 && woomera_profile_verbose(tech_pvt->profile) > 9) {
					ast_verbose(WOOMERA_DEBUG_PREFIX "+++WRITE %s %d\n",self->name, i);
				}
			}


		} else {
			ast_log(LOG_NOTICE, "Invalid frame type %d sent\n", frame->frametype);
		}
	}
	
	return res;
}

/*--- tech_write_video: Write a video frame to my channel ---*/
static int tech_write_video(struct ast_channel *self, struct ast_frame *frame)
{
	private_object *tech_pvt;
	int res = 0;

	tech_pvt = self->tech_pvt;
	return res;
}

/*--- tech_exception: Read an exception audio frame from my channel ---*/
static struct ast_frame *tech_exception(struct ast_channel *self)
{
	private_object *tech_pvt;

	tech_pvt = self->tech_pvt;	
	if (globals.debug > 1 && option_verbose > 2 && woomera_profile_verbose(tech_pvt->profile) > 2) {
	   ast_verbose(WOOMERA_DEBUG_PREFIX "+++EXCEPT %s\n",self->name);
	}

	return &ast_null_frame;
}

/*--- tech_indicate: Indicaate a condition to my channel ---*/
#ifdef  USE_TECH_INDICATE
#if defined (AST14) || defined (AST16)
static int tech_indicate(struct ast_channel *self, int condition, const void *data, size_t datalen)
#else
static int tech_indicate(struct ast_channel *self, int condition)
#endif
{
	private_object *tech_pvt;
	int res = -1;

	
	tech_pvt = self->tech_pvt;
	if (!tech_pvt) {
		return res;
	}

	ast_mutex_lock(&tech_pvt->iolock);	

	switch(condition) {
	case AST_CONTROL_RINGING:
		if (globals.debug > 3) {
			ast_log(LOG_NOTICE, "TECH INDICATE: Ringing\n");
		}
		if (!ast_test_flag(tech_pvt,TFLAG_ACCEPTED)) {
			ast_set_flag(tech_pvt, TFLAG_ACCEPT);
		}
		ast_set_flag(tech_pvt, TFLAG_RING_AVAIL);
		if (tech_pvt->capability != AST_TRANS_CAP_DIGITAL) {
			ast_set_flag(tech_pvt, TFLAG_MEDIA_AVAIL);
		}
		break;
	case AST_CONTROL_BUSY:
		if (globals.debug > 3) {
			ast_log(LOG_NOTICE, "TECH INDICATE: Busy\n");
		}
		ast_copy_string(tech_pvt->ds, "BUSY", sizeof(tech_pvt->ds));
		tech_pvt->pri_cause=17;
		ast_set_flag(tech_pvt, TFLAG_ABORT);
		break;
	case AST_CONTROL_CONGESTION:
		if (globals.debug > 3) {
			ast_log(LOG_NOTICE, "TECH INDICATE: Congestion\n");
		}
		ast_copy_string(tech_pvt->ds, "BUSY", sizeof(tech_pvt->ds));
		tech_pvt->pri_cause=17;
		ast_set_flag(tech_pvt, TFLAG_ABORT);
		break;
	case AST_CONTROL_PROCEEDING:
		if (globals.debug > 3) {
			ast_log(LOG_NOTICE, "TECH INDICATE: Proceeding\n");
		}
		if (!ast_test_flag(tech_pvt,TFLAG_ACCEPTED)) {
			ast_set_flag(tech_pvt, TFLAG_ACCEPT);
		}
		if (tech_pvt->capability != AST_TRANS_CAP_DIGITAL) {
			ast_set_flag(tech_pvt, TFLAG_MEDIA_AVAIL);
		}
		break;
	case AST_CONTROL_PROGRESS:
		if (globals.debug > 3) {
			ast_log(LOG_NOTICE, "TECH INDICATE: Progress\n");
		}
		if (!ast_test_flag(tech_pvt,TFLAG_ACCEPTED)) {
			ast_set_flag(tech_pvt, TFLAG_ACCEPT);
		}
		if (tech_pvt->capability != AST_TRANS_CAP_DIGITAL) {
			ast_set_flag(tech_pvt, TFLAG_MEDIA_AVAIL);
		}
#if 0
		if (!tech_pvt->profile->progress_on_accept) {
			ast_set_flag(tech_pvt, TFLAG_PROGRESS);
		}
#endif

		break;
	case AST_CONTROL_HOLD:
		if (globals.debug > 3) {
			ast_log(LOG_NOTICE, "TECH INDICATE: Hold\n");
		}
		if (!ast_test_flag(tech_pvt,TFLAG_ACCEPTED)) {
			ast_set_flag(tech_pvt, TFLAG_ACCEPT);
		}
#if defined (AST14) || defined (AST16)
		ast_moh_start(self, data, tech_pvt->mohinterpret);
#endif
		break;
	case AST_CONTROL_UNHOLD:
		if (globals.debug > 3) {
			ast_log(LOG_NOTICE, "TECH INDICATE: UnHold\n");
		}
		if (!ast_test_flag(tech_pvt,TFLAG_ACCEPTED)) {
			ast_set_flag(tech_pvt, TFLAG_ACCEPT);
		}
#if defined (AST14) || defined (AST16)
		ast_moh_stop(self);
#endif
		break;
	case AST_CONTROL_VIDUPDATE: 
		if (globals.debug > 3) {
			ast_log(LOG_NOTICE, "TECH INDICATE: Vidupdate\n");
		}
		if (!ast_test_flag(tech_pvt,TFLAG_ACCEPTED)) {
			ast_set_flag(tech_pvt, TFLAG_ACCEPT);
		}
		break;
#ifdef WOO_CONTROL_SRC_FEATURE
	case AST_CONTROL_SRCUPDATE:
		res = 0;
		break;
#endif
	case -1:
                res = -1;
                break;
        default:
                ast_log(LOG_NOTICE, "Don't know how to indicate condition %d\n", condition);
                res = -1;
                break;
	}
	
	ast_mutex_unlock(&tech_pvt->iolock);	

	return res;
}
#endif

/*--- tech_fixup: add any finishing touches to my channel if it is masqueraded---*/
static int tech_fixup(struct ast_channel *oldchan, struct ast_channel *newchan)
{
	struct private_object *p;

	if (!oldchan || !newchan) {
			ast_log(LOG_ERROR, "Error: Invalid Pointers oldchan=%p newchan=%p\n",oldchan,newchan);
			return -1;
	}
	if (!newchan->tech_pvt) {
			ast_log(LOG_ERROR, "Error: Invalid Pointer newchan->tech_pvt=%p\n",
									newchan->tech_pvt);
			return -1;
	}

	p = newchan->tech_pvt;

	ast_mutex_lock(&p->iolock);
	if (p->owner == oldchan) {
		p->owner = newchan;
	} else {
		ast_log(LOG_ERROR, "Error: New p owner=%p instead of %p \n",
								p->owner, oldchan);
	}

#if 0
	if (newchan->_state == AST_STATE_RINGING) 
		dahdi_indicate(newchan, AST_CONTROL_RINGING, NULL, 0);
	update_conf(p);
#endif
	
    if (globals.debug > 1 && option_verbose > 9) {
			ast_verbose(WOOMERA_DEBUG_PREFIX "+++FIXUP ChOld=%s ChNew=%s\n",
                                                oldchan->name,newchan->name);
	}

	ast_mutex_unlock(&p->iolock);
	return 0;
}

/*--- tech_send_html: Send html data on my channel ---*/
static int tech_send_html(struct ast_channel *self, int subclass, const char *data, int datalen)
{
	private_object *tech_pvt;
	int res = 0;

	tech_pvt = self->tech_pvt;


	return res;
}

/*--- tech_send_text: Send plain text data on my channel ---*/
static int tech_send_text(struct ast_channel *self, const char *text)
{
	int res = 0;

	return res;
}

/*--- tech_send_image: Send image data on my channel ---*/
static int tech_send_image(struct ast_channel *self, struct ast_frame *frame) 
{
	int res = 0;

	return res;
}


/*--- tech_setoption: set options on my channel ---*/
static int tech_setoption(struct ast_channel *self, int option, void *data, int datalen)
{
	int res = 0;

	if (globals.debug > 1 && option_verbose > 9) {
	   	ast_verbose(WOOMERA_DEBUG_PREFIX "+++SETOPT %s\n",self->name);
	}
	return res;

}

/*--- tech_queryoption: get options from my channel ---*/
static int tech_queryoption(struct ast_channel *self, int option, void *data, int *datalen)
{
	int res = 0;

	if (globals.debug > 1 && option_verbose > 9) {
			ast_verbose(WOOMERA_DEBUG_PREFIX "+++GETOPT %s\n",self->name);
	}
	return res;
}

/*--- tech_bridged_channel: return a pointer to a channel that may be bridged to our channel. ---*/
#if 0
static struct ast_channel *tech_bridged_channel(struct ast_channel *self, struct ast_channel *bridge)
{
	struct ast_channel *chan = NULL;

	if (globals.debug > 1 && option_verbose > 2 && woomera_profile_verbose(profile) > 2) {
		ast_verbose(WOOMERA_DEBUG_PREFIX "+++BRIDGED %s\n",self->name);
	}
	return chan;
}
#endif


/*--- tech_transfer: Technology-specific code executed to peform a transfer. ---*/
static int tech_transfer(struct ast_channel *self, const char *newdest)
{
	int res = -1;

	if (globals.debug > 1 && option_verbose > 9) {
			ast_verbose(WOOMERA_DEBUG_PREFIX "+++TRANSFER %s\n",self->name);
	}
	return res;
}

		
static int woomera_rbs_relay(struct private_object *ch0, struct private_object *ch1, struct ast_channel *c1)
{

	if (ch0->profile->rbs_relay &&
		ch1->profile->rbs_relay &&
		ch0->rbs_frame.frametype == 99) {
			tech_send_rbs(c1, ch0->rbs_frame.W_SUBCLASS_INT);
			ch0->rbs_frame.frametype=0;
	}	
	
	return 0;
}

#define WOOMERA_RELAY_SIZE 4096
static int woomera_media_pass_through(struct private_object *ch0, struct private_object *ch1)
{
	
	if (ch0->woomera_relay) {
		ast_log(LOG_NOTICE,"%s: Error: 	woomera_media_pass_through relay used!\n",
			ch0->callid);	
		return -1;
	}
	
	if (woomera_message_header(&ch1->media_info, "Raw-Audio") == NULL) {
		ast_log(LOG_NOTICE,"%s: Error: 	woomera_media_pass_through media info not available!\n",
				ch1->callid);	
		return -1;	
	}
	
	ch0->woomera_relay = (char *)ast_malloc(WOOMERA_RELAY_SIZE);	
	if (!ch0->woomera_relay) {
		return -1;	
	}
	
	memset(ch0->woomera_relay,0,WOOMERA_RELAY_SIZE);
	
	sprintf(ch0->woomera_relay, 
				"MEDIA %s%s"
				"Raw-Audio: %s%s"
				"Request-Audio: Raw%s"
				"Unique-Call-Id: %s%s",
				ch0->callid,
				WOOMERA_LINE_SEPARATOR,
				woomera_message_header(&ch1->media_info, "Raw-Audio"),
				WOOMERA_LINE_SEPARATOR,
				WOOMERA_LINE_SEPARATOR,
				ch0->callid,
				WOOMERA_RECORD_SEPARATOR);
	
	ast_set_flag(ch0, TFLAG_MEDIA_RELAY);
		
	return 0;	
}


static enum ast_bridge_result  tech_bridge (struct ast_channel *c0,
				      struct ast_channel *c1, int flags,
				      struct ast_frame **fo,
				      struct ast_channel **rc,
				      int timeoutms)

{
	struct private_object *ch0, *ch1;
	struct ast_channel *carr[2], *who;
	int to = -1;
	struct ast_frame *f;
	int err=0;
	int media_pass_through=0;
	
	
	ch0 = c0->tech_pvt;
	ch1 = c1->tech_pvt;

	carr[0] = c0;
	carr[1] = c1;
  
	if (!ch0 || !ch0->profile || !ch1 || !ch1->profile) {
		return AST_BRIDGE_FAILED;
	}

	if (ch0->profile->bridge_disable) {
		return AST_BRIDGE_FAILED;
	}

	if (ch1->profile->bridge_disable) {
		return AST_BRIDGE_FAILED;
	}

	if (option_verbose > 5) {
		ast_verbose(VERBOSE_PREFIX_3 "Native bridging %s and %s\n", c0->name, c1->name);
	}

	//ast_log(LOG_NOTICE, "* Making Native Bridge between %s and %s\n", c0->name, c1->name);
 
	if (! (flags & AST_BRIDGE_DTMF_CHANNEL_0) )
		ch0->ignore_dtmf = 1;

	if (! (flags & AST_BRIDGE_DTMF_CHANNEL_1) )
		ch1->ignore_dtmf = 1;

	ch0->bridge=1;
	ch1->bridge=1;

	if (ch0->profile->media_pass_through &&
	    ch1->profile->media_pass_through) {
		
		/* Attempt */
		err=woomera_media_pass_through(ch0,ch1);
		if (err == 0) {
			err=woomera_media_pass_through(ch1,ch0);
		}
		
		if (err == 0) {
			ast_log(LOG_NOTICE, "woomera: Media pass throught complete %s <--> %s\n", c0->name, c1->name);
			media_pass_through=1;
			timeoutms=50;
			
		} else {
			ast_log(LOG_NOTICE, "woomera: Media pass throught failed, proceeding to bridge! %s <-!-> %s\n", c0->name, c1->name);
		}
	}
	
	
	for (;/*ever*/;) {
		to = timeoutms;
		who = ast_waitfor_n(carr, 2, &to);

		if (!who) {
			if (media_pass_through) {
				if (ast_test_flag(ch0, TFLAG_ABORT) || 
					ast_test_flag(ch1, TFLAG_ABORT)) {
					break;						
				}
				woomera_rbs_relay(ch0,ch1,c1);
				woomera_rbs_relay(ch1,ch0,c0);
				continue;
				
			}
			
			ast_log(LOG_NOTICE, "woomera: Bridge empty read, breaking out\n");
			break;
		}
		
		f = ast_read(who);

		if (!f || f->frametype == AST_FRAME_CONTROL) {
			/* got hangup .. */

			if (!f) {
				if (option_verbose > 10) {
					ast_log(LOG_NOTICE, "woomera: Bridge Read Null Frame\n");
				}
			} else {
				if (option_verbose > 10) {
					ast_log(LOG_NOTICE, "woomera: Bridge Read Frame Control class:%d\n", f->W_SUBCLASS_INT);
				}
			}

			*fo = f;
			*rc = who;
			break;
		}
		
		if (f->frametype == AST_FRAME_DTMF) {
			ast_log(LOG_NOTICE, "woomera: Bridge Read DTMF %d from %s\n", f->W_SUBCLASS_INT, who->exten);

			*fo = f;
			*rc = who;
			break;
		}

		
				  
#if 0
		if (f->frametype == AST_FRAME_VOICE) {
			chan_misdn_log(1, ch1->bc->port, "I SEND: Splitting conference with Number:%d\n", ch1->bc->pid +1);
	
			continue;
		}
#endif

		woomera_rbs_relay(ch0,ch1,c1);
		woomera_rbs_relay(ch1,ch0,c0);

		if (who == c0) {
			ast_write(c1, f);
		} else {
			ast_write(c0, f);
		}
	}
	
	ch0->bridge=0;
	ch1->bridge=0;

	return AST_BRIDGE_COMPLETE;
}


/*--- tech_bridge:  Technology-specific code executed to natively bridge 2 of our channels ---*/
#if 0
static enum ast_bridge_result tech_bridge(struct ast_channel *c0, struct ast_channel *c1, int flags, struct ast_frame **fo, struct ast_channel **rc, int timeoutms)
{
	int res = -1;

	if (globals.debug > 1) {
		ast_verbose(WOOMERA_DEBUG_PREFIX "+++BRIDGE %s\n",c0->name);
	}
	return res;
}
#endif

#if defined(AST16)
static char *ast16_woomera_cli(struct ast_cli_entry *e, int cmd, struct ast_cli_args *a)
{
	switch (cmd) {
		case CLI_INIT:
			e->command = "woomera";
			e->usage = "Usage: woomera <profile> <cmd> <option>\n";
			return NULL;
		case CLI_GENERATE:
			return NULL;
	}

	woomera_cli(a->fd, a->argc, a->argv);
	return CLI_SUCCESS;
}
#endif

static int woomera_full_status(int fd)
{
	woomera_profile *profile;
	ASTOBJ_CONTAINER_TRAVERSE(&woomera_profile_list, 1, do { 
		ASTOBJ_RDLOCK(iterator);
		profile = iterator;

		if (!ast_test_flag(profile, PFLAG_DISABLED)) {
            ast_cli(fd,"Profile: {%s} Host=%s WooVer=%s SmgVer=%s CurCalls=%i TotCalls=%i Verb=%i RxSeqErr=%i TxSeqErr=%i\n",
				profile->name, profile->woomera_host, 
				WOOMERA_VERSION, profile->smgversion[0] == '\0'?"N/A":profile->smgversion,
				profile->call_count,
				profile->call_out+profile->call_in,
				woomera_profile_verbose(profile),
				profile->media_rx_seq_err,
				profile->media_tx_seq_err);
		 }
		ASTOBJ_UNLOCK(iterator);
	} while(0));     

	return 0;
}


static int woomera_cli(int fd, int argc, char *argv[]) 
{
	struct woomera_profile *profile;
	char *profile_name="default";	

	if (argc > 1) {

		profile_name=argv[1];
		profile = ASTOBJ_CONTAINER_FIND(&woomera_profile_list, profile_name);
		if (!profile) {

		    if (strcmp(profile_name,"version") == 0) {
				ast_cli(fd, "Woomera version %s\n",
					WOOMERA_VERSION);
				return 0;
			}

            if (strcmp(profile_name, "debug") == 0) {
				if (argc > 2) {
					globals.debug = atoi(argv[2]);
		   	 	}
				ast_cli(fd, "Woomera debug=%d\n", globals.debug);  
            	return 0;
			}

			if (!strcmp(profile_name, "panic")) {
				if (argc > 2) {
					globals.panic = atoi(argv[2]);
				}
				ast_cli(fd, "Woomera panic=%d\n", globals.panic);
				return 0;
			}

			if (!strcmp(profile_name, "status")) {
				woomera_full_status(fd);
				return 0;
			}  

			ast_cli(fd, "Woomera: Invalid profile name %s\n", profile_name);
			ast_cli(fd, "Usage: woomera <profile> <cmd> <option>\n");
			return 0;
		}


		/* After this point we need at least two arguments */
		if (argc <= 2) {
         	  ast_cli(fd, "Usage: woomera <profile> <cmd> <option>\n");  
			  return 0;
		}

		if (argc == 2) {
			ast_cli(fd, "No command given. Need one of: debug, verbose, coding, call_status, version, panic, rxgain, txgain, media_pass_through, udp_seq, bridge_disable, rbs_relay, threads, smgdebug, abort\n");
		} else if (!strcmp(argv[2], "debug")) {
			if (argc > 3) {
				globals.debug = atoi(argv[3]);
			}
			ast_cli(fd, "Woomera debug=%d\n", globals.debug);

		} else if (!strcmp(argv[2], "verbose")) {
			if (argc > 3) {
				profile->verbose = atoi(argv[3]);
			}
			ast_cli(fd, "Woomera {%s} verbose=%d\n", profile_name, profile->verbose); 

		} else if (!strcmp(argv[2], "coding")) {

			switch (profile->coding) {
			case AST_FORMAT_ALAW:
				ast_cli(fd, " Woomera {%s} coding=ALAW\n",profile_name);
				break;
			case AST_FORMAT_ULAW:
				ast_cli(fd, " Woomera {%s} coding=ULAW\n",profile_name);
				break;
			case AST_FORMAT_SLINEAR:
				ast_cli(fd, " Woomera {%s} coding=PMC-16\n",profile_name);
				break;
			default:
				ast_cli(fd, " Woomera {%s} invalid coding=%d {internal error}",
					profile_name,profile->coding);
				break;
				
			}
			
		} else if (!strcmp(argv[2], "call_status") || !strcmp(argv[2], "callstatus")) {
			if (argc > 3) {
					if (!strcmp(argv[3], "clear")) {
						profile->call_out=0;
						profile->call_in=0;
						profile->call_out=0;
						profile->call_in=0;
						profile->call_ok=0;
						profile->call_end=0;
						profile->call_abort=0;
						profile->call_ast_hungup=0;
						profile->media_rx_seq_err=0;
						profile->media_tx_seq_err=0;
					}
			}

			ast_cli(fd, "Woomera {%s} calls=%d tcalls=%d (out=%d in=%d ok=%d end=%d abort=%d hup_pend=%d use=%d) rx_seq_err=%i tx_seq_err=%i\n", 
					profile->name,
					profile->call_count,
					profile->call_out+profile->call_in,
					profile->call_out,
					profile->call_in,
					profile->call_ok,
					profile->call_end,
					profile->call_abort,
					profile->call_ast_hungup,
					usecnt,
					profile->media_rx_seq_err,
					profile->media_tx_seq_err);

		} else if (!strcmp(argv[2], "version")) { 

			ast_cli(fd, "Woomera {%s} version %s : SMG Version %s  \n",
				 profile->name,WOOMERA_VERSION,profile->smgversion[0] == '\0'?"N/A":profile->smgversion);

			
		} else if (!strcmp(argv[2], "panic")) {
			if (argc > 3) {
				globals.panic = atoi(argv[3]);
			}
			ast_cli(fd, "Woomera panic=%d           \n", globals.panic);
			
		} else if (!strcmp(argv[2], "rxgain")) {
			float gain;
			if (argc > 3) {
				if (sscanf(argv[3], "%f", &gain) != 1) {
					ast_cli(fd, "Woomera Invalid rxgain: %s\n",argv[3]);
				} else {
					woomera_config_gain(profile,gain,1);
				}	
			}
			ast_cli(fd, "Woomera {%s} rxgain: %f\n",profile_name,profile->rxgain_val);
			
		} else if (!strcmp(argv[2], "txgain")) {
			float gain;
			if (argc > 3) {
				if (sscanf(argv[3], "%f", &gain) != 1) {
					ast_cli(fd, "Woomera Invalid txgain: %s\n",argv[3]);
				} else {
					woomera_config_gain(profile,gain,0);
				}	
			} 	
			ast_cli(fd, "Woomera {%s} txgain: %f\n",profile_name,profile->txgain_val);
			
		} else if (!strcmp(argv[2], "media_pass_through")) {
			
			ast_cli(fd, "Woomera {%s} media_pass_through: %d\n",profile_name,profile->media_pass_through);
			
		} else if (!strcmp(argv[2], "udp_seq")) {
			
			ast_cli(fd, "Woomera {%s} udp_seq: %d\n",profile_name,profile->udp_seq);
			
		} else if (!strcmp(argv[2], "bridge_disable")) {
			
			if (argc > 3) { 
				profile->bridge_disable = atoi(argv[3]);
			}
			
			ast_cli(fd, "Woomera {%s} bridge_disable: %d\n",profile_name,profile->bridge_disable);	
			
		} else if (!strcmp(argv[2], "rbs_relay")) {
			
			if (argc > 3) { 
				profile->rbs_relay = atoi(argv[3]);
			}
			
			ast_cli(fd, "Woomera {%s} rbs_relay: %d\n",profile_name,profile->rbs_relay);	
			
		} else if (!strcmp(argv[2], "threads")) {
			ast_cli(fd, "chan_woomera is using %s threads!\n", 
					globals.more_threads ? "more" : "less");

		} else if (!strcmp(argv[2], "smgdebug")) {

			if (argc > 3) {
				int smgdebug = atoi(argv[3]);
				if (smgdebug < 0) {
					ast_cli(fd, "Woomera Invalid smgdebug level: %s\n",argv[3]);
				} else {

					ast_cli(fd,"Woomera setting smg debug level to %i\n",smgdebug);
					
					woomera_printf(NULL, profile->woomera_socket , "debug %d%s", 
					   smgdebug,
					   WOOMERA_RECORD_SEPARATOR);
				}
			} else {
				ast_cli(fd, "Woomera Invalid smgdebug usage\n");
			}
		} else if (!strcmp(argv[2], "abort")) {
			global_set_flag(NULL, TFLAG_ABORT);
		}

	} else {
		ast_cli(fd, "Usage: woomera <profile> <cmd> <option>\n");
	}
	return 0;
}


#ifdef CALLWEAVER
#ifdef CALLWEAVER_OPBX_CLI_ENTRY
static struct opbx_cli_entry cli_woomera[] = {
# else
#ifdef CALLWEAVER_CW_CLI_ENTRY
static struct cw_cli_entry cli_woomera[] = {
#else
static struct cw_clicmd cli_woomera[] = {
#endif
#endif
	{
		.cmda = { "woomera", "default", "version", NULL },
		.handler = woomera_cli,
		.summary = "Woomera version",
		//.usage = pri_debug_help,
		//.generator = complete_span_4,
	},
};

#else
#if defined (AST16)
static struct ast_cli_entry cli_woomera[] = {
	AST_CLI_DEFINE(ast16_woomera_cli, "Prints Woomera options"),
};
#else
static struct ast_cli_entry  cli_woomera = { { "woomera", NULL }, woomera_cli, "Woomera", "Woomera" };
#endif
#endif

/******************************* CORE INTERFACE ********************************************
 * These are module-specific interface functions that are common to every module
 * To be used to initilize/de-initilize, reload and track the use count of a loadable module. 
 */

static void urg_handler(int num)
{
	signal(num, urg_handler);
        return;
}



int usecount(void)
{
	int res;
	ast_mutex_lock(&usecnt_lock);
	res = usecnt;
	ast_mutex_unlock(&usecnt_lock);
	return res;
}

#if 0
static char *key(void)
{
	return ASTERISK_GPL_KEY;
}

/* returns a descriptive string to the system console */
char *description(void)
{
	return (char *) desc;
}
#endif


static struct ast_channel * tech_get_owner( private_object *tech_pvt)
{
	struct ast_channel *owner=NULL;

	ast_mutex_lock(&tech_pvt->iolock);
	if (!ast_test_flag(tech_pvt, TFLAG_TECHHANGUP) && tech_pvt->owner) {
		owner=tech_pvt->owner;
	}
	ast_mutex_unlock(&tech_pvt->iolock);

	return owner;
}


static int woomera_event_media (private_object *tech_pvt, woomera_message *wmsg)
{

	char *raw_audio_header;
	char *hw_dtmf;
	char ip[25];
	char *ptr;
	int port = 0;
	struct hostent hps, *hp;
	struct hostent *result;
	struct ast_channel *owner;
	char buf[512];
	int err=0;

	memset(&hps,0,sizeof(hps));
	hp=&hps;
	
	raw_audio_header = woomera_message_header(wmsg, "Raw-Audio");
	if (raw_audio_header == NULL) {
		ast_copy_string(tech_pvt->ds, "PROTOCOL_ERROR", sizeof(tech_pvt->ds));
                tech_pvt->pri_cause=111;
		return 1;
	}

	strncpy(ip, raw_audio_header, sizeof(ip) - 1);
	if ((ptr=strchr(ip, '/'))) {
		*ptr = '\0';
		ptr++;
		port = atoi(ptr);
	} else if ((ptr=strchr(ip, ':'))) {
                *ptr = '\0';
                ptr++;
                port = atoi(ptr);
	}
	
#if 0	
	audio_codec = woomera_message_header(wmsg, "Receive-Audio-Codec");
	if (audio_codec) {
		if (!strcasecmp(audio_codec,"G.711-uLaw-64k")) {
			tech_pvt->coding=AST_FORMAT_ULAW;
			ast_log(LOG_NOTICE, "MEDIA GOT ULAW\n");
		} else if (!strcasecmp(audio_codec,"G.711-aLaw-64k")) {
			tech_pvt->coding=AST_FORMAT_ALAW;
			ast_log(LOG_NOTICE, "MEDIA GOT ALAW\n");
		} 
	}
#endif
	
	hw_dtmf = woomera_message_header(wmsg, "DTMF");
	if (hw_dtmf != NULL) {
		if (strncmp(hw_dtmf, "OutofBand" ,9) == 0) {
			if (option_verbose > 2 && woomera_profile_verbose(tech_pvt->profile) > 2) {
				ast_verbose(WOOMERA_DEBUG_PREFIX "HW DTMF supported %s\n", tech_pvt->callid);
			}
			
			if(tech_pvt->dsp) {	
				tech_pvt->dsp_features &= ~DSP_FEATURE_DTMF_DETECT;
				ast_dsp_set_features(tech_pvt->dsp, tech_pvt->dsp_features);	
			}
		} else {
			if (option_verbose > 2 && woomera_profile_verbose(tech_pvt->profile) > 2) {
				ast_verbose(WOOMERA_DEBUG_PREFIX "HW DTMF not supported %s\n", tech_pvt->callid);
			}
		}
	}

	/* If we are already in MEDIA mode then
	 * ignore this message */
	if (ast_test_flag(tech_pvt, TFLAG_MEDIA)) {
		return 0;
	}

	gethostbyname_r(ip, hp, buf, sizeof(buf), &result, &err);
	if (result) {
		
		tech_pvt->udpwrite.sin_family = hp->h_addrtype;
		memcpy((char *) &tech_pvt->udpwrite.sin_addr.s_addr, hp->h_addr_list[0], hp->h_length);
		tech_pvt->udpwrite.sin_port = htons(port);

		if (globals.debug > 4) {	
	 		ast_log(LOG_NOTICE,"MEDIA EVENT UdpWrite IP=%s/%d len=%i %d.%d.%d.%d\n",
                                                ip,port, 
                                                hp->h_length,
                                                hp->h_addr_list[0][0],
                                                hp->h_addr_list[0][1],
                                                hp->h_addr_list[0][2],
                                                hp->h_addr_list[0][3]);
		}
		
		ast_set_flag(tech_pvt, TFLAG_MEDIA);
		tech_pvt->timeout = 0;

		my_tech_pvt_and_owner_lock(tech_pvt);

		owner=tech_pvt->owner;
		if (!owner) {
                	tech_pvt->pri_cause=44;
			ast_copy_string(tech_pvt->ds, "REQUESTED_CHAN_UNAVAIL", sizeof(tech_pvt->ds));
			my_tech_pvt_and_owner_unlock(tech_pvt);
			return -1;
		}

		if (ast_test_flag(tech_pvt, TFLAG_INBOUND)) {
			int pbx_res;
#if defined (AST14) || defined (AST16)
			ast_setstate(owner, AST_STATE_RINGING);
#endif
			pbx_res=ast_pbx_start(owner);

			if (pbx_res) {

				my_tech_pvt_and_owner_unlock(tech_pvt);

				ast_log(LOG_NOTICE, "Failed to start PBX on %s \n", 
						tech_pvt->callid);
				ast_set_flag(tech_pvt, TFLAG_ABORT);
				ast_clear_flag(tech_pvt, TFLAG_PBX);
				owner->tech_pvt = NULL;
				tech_pvt->owner=NULL;
				ast_copy_string(tech_pvt->ds, "SWITCH_CONGESTION", sizeof(tech_pvt->ds));
                        	tech_pvt->pri_cause=42; 
				ast_hangup(owner);
				/* Let destroy hangup */
				return -1;
			} else {
#if !defined (AST14) && !defined (AST16)
				ast_setstate(owner, AST_STATE_RINGING);
#endif
				ast_set_flag(tech_pvt, TFLAG_PBX);
				owner->hangupcause = AST_CAUSE_NORMAL_CLEARING;

				woomera_send_progress(tech_pvt);
			}
		} else { 

#if 1
			ast_queue_control(owner, AST_CONTROL_RINGING);
			if (owner->_state != AST_STATE_UP) {
                                ast_setstate(owner, AST_STATE_RINGING);
                        }
#endif

			/* Do nothing for the outbound call
			 * The PBX flag was already set! */
		}

		my_tech_pvt_and_owner_unlock(tech_pvt);

		return 0;
		
	} else {
		if (globals.debug) {
			ast_log(LOG_ERROR, "{%s} Cannot resolve %s\n", tech_pvt->profile->name, ip);
		}
		ast_copy_string(tech_pvt->ds, "NO_ROUTE_DESTINATION", sizeof(tech_pvt->ds));
                tech_pvt->pri_cause=3;
		return -1;
	}

	return -1;
}

static int woomera_event_incoming (private_object *tech_pvt)
{
	char *exten;
	char *cid_name=NULL;
	char *cid_num;
	char *cid_rdnis;
	char *custom;
	char *tg_string="1";
	char *pres_string;
	char *screen_string;
	char *bearer_cap_string;
	char *uil1p_string;		/* User Layer 1 Info */
	int validext;
	int presentation=0;
	woomera_message wmsg;
	struct ast_channel *owner;


	wmsg = tech_pvt->call_info;
	
	if (globals.debug > 2) {
	ast_log(LOG_NOTICE, "WOOMERA EVENT %s : callid=%s tech_callid=%s tpvt=%p\n",
			wmsg.command,wmsg.callid,tech_pvt->callid,tech_pvt);
	}
	
	if (ast_strlen_zero(tech_pvt->profile->context)) {
		if (globals.debug > 2) {
		ast_log(LOG_NOTICE, "No Context Found %s tpvt=%p\n",
			tech_pvt->callid,tech_pvt);
		}
		ast_log(LOG_NOTICE, "No context configured for inbound calls aborting call!\n");
		ast_set_flag(tech_pvt, TFLAG_ABORT);
		return -1;
	}
	
	exten = woomera_message_header(&wmsg, "Local-Number");
	if (! exten || ast_strlen_zero(exten)) {
		exten = "s";
	}
	
	/* Check for list of blacklisted numbers */
	if (tech_pvt->profile->called_ignore_idx) {
		int i;
		for (i=0; i < tech_pvt->profile->called_ignore_idx; i++) {
			if (strcmp(tech_pvt->profile->called_ignore[i],exten) == 0) {
				if (option_verbose > 8 && woomera_profile_verbose(tech_pvt->profile) > 8) {
					ast_log(LOG_NOTICE, "Woomera {%s} called number %s blacklisted as per woomera.conf\n",
							tech_pvt->profile->name, exten);
				}
				return -1;	
			}
		}
	}
	
	tg_string = woomera_message_header(&wmsg, "Trunk-Group");
	if (!tg_string || ast_strlen_zero(tg_string)) {
		tg_string="1";	
	}

	pres_string = woomera_message_header(&wmsg, "Presentation");
	if (pres_string && !ast_strlen_zero(pres_string)) {
		presentation |= ( atoi(pres_string) << 5) & 0xF0;
	}

	screen_string = woomera_message_header(&wmsg, "Screening");
	if (screen_string && !ast_strlen_zero(screen_string)) {
		presentation |= atoi(screen_string) & 0x0F;
	}

	cid_name = woomera_message_header(&wmsg, "Remote-Name");
	if (cid_name) {
		cid_name = ast_strdupa(woomera_message_header(&wmsg, "Remote-Name"));
	}	

	if (cid_name && (cid_num = strchr(cid_name, '!'))) {
		*cid_num = '\0';
		cid_num++;
	} else {
		cid_num = woomera_message_header(&wmsg, "Remote-Number");
	}

	bearer_cap_string = woomera_message_header(&wmsg, "Bearer-Cap");
	if (bearer_cap_string) {
		tech_pvt->capability = woomera_capability_to_ast(bearer_cap_string);
#if 0
		ast_log(LOG_NOTICE,"Bearer-Cap NENAD %s %d %s\n",
				bearer_cap_string,woomera_capability_to_ast(bearer_cap_string), ast_transfercapability2str(tech_pvt->capability));
#endif
	}

	uil1p_string = woomera_message_header(&wmsg, "uil1p");
	tech_pvt->coding = woomera_coding_to_ast(uil1p_string);
	if (tech_pvt->coding < 0) {
		tech_pvt->coding = tech_pvt->profile->coding;
	}

	cid_rdnis = woomera_message_header(&wmsg, "RDNIS");
	custom = woomera_message_header(&wmsg, "xCustom");

	owner = tech_pvt->owner;
	if (owner) {
		int group = atoi(tg_string);
		if(group >= 0 && 
		   group <= WOOMERA_MAX_TRUNKGROUPS && 
		   tech_pvt->profile->tg_context[group] != NULL){

			strncpy(owner->context, tech_pvt->profile->tg_context[group], sizeof(owner->context) - 1);

			if(tech_pvt->profile->tg_language[group] != NULL &&
			   strlen(tech_pvt->profile->tg_language[group])){
				strncpy((char*)owner->language, (char*)tech_pvt->profile->tg_language[group], sizeof(owner->language) - 1);
			}
		}else {
			snprintf(owner->context, sizeof(owner->context) - 1,
					"%s%s", 
					tech_pvt->profile->default_context,
					tg_string);	
		}

		owner->transfercapability = tech_pvt->capability;
		pbx_builtin_setvar_helper(owner, "TRANSFERCAPABILITY", ast_transfercapability2str(tech_pvt->capability));
		pbx_builtin_setvar_helper(owner, "CALLTYPE", ast_transfercapability2str(tech_pvt->capability));

		owner->nativeformats = tech_pvt->coding;	
		owner->writeformat = owner->rawwriteformat = owner->rawreadformat = owner->readformat = tech_pvt->coding;
		tech_pvt->frame.W_SUBCLASS_CODEC = tech_pvt->coding;

		strncpy(owner->exten, exten, sizeof(owner->exten) - 1);
		ast_set_callerid(owner, cid_num, cid_name, cid_num);
		owner->W_CID_NUM_PRES=presentation;
		owner->W_CID_NAME_PRES=presentation;
		
		if (cid_rdnis) {
			W_CID_SET_FROM_RDNIS(owner,strdup(cid_rdnis));
			pbx_builtin_setvar_helper(owner, "RDNIS", cid_rdnis);	
		}

		if (custom) {
			pbx_builtin_setvar_helper(owner, "WOOMERA_CUSTOM", custom);
		}

		validext = ast_exists_extension(owner,
					owner->context,
					owner->exten,
					1,
					owner->W_CID_NUM);
					
		if (globals.debug > 2){			
		ast_log(LOG_NOTICE, "Incoming Call exten %s@%s called %s astpres = 0x%0X!\n", 
					exten, 
					owner->context,
					tech_pvt->callid,
					owner->W_CID_NUM_PRES);
		}
	
	} else {
		
		validext=0;
		
	}			


	if (validext == 0) {
               if (globals.debug > 1){
			if (option_verbose > 0) {
                               ast_log(LOG_ERROR, "Error: Invalid exten %s@%s called %s!\n",
                                       exten,
                                       owner->context,
                                       tech_pvt->callid);
			}
		}
		return -1;	
	}

	return 0;
}

static void woomera_check_event (private_object *tech_pvt, int res, woomera_message *wmsg)
{

	if (globals.debug > 2) {
			
		if (!strcasecmp(wmsg->command, "INCOMING") ) {
			/* Do not print it here */
		}else{
			ast_log(LOG_NOTICE, "WOOMERA EVENT %s : callid=%s tech_callid=%s tpvt=%p\n",
				wmsg->command,wmsg->callid,tech_pvt->callid,tech_pvt);
		}	
	}
	
	if (res < 0) {
		if (globals.debug > 2) {
			ast_log(LOG_NOTICE, "INVALID MESSAGE PARSE COMMAND %s tpvt=%p\n",
				tech_pvt->callid,tech_pvt);
		}
		ast_copy_string(tech_pvt->ds, "PROTOCOL_ERROR", sizeof(tech_pvt->ds));
		tech_pvt->pri_cause=111;
		ast_set_flag(tech_pvt, TFLAG_ABORT);
			
	} else if (!strcasecmp(wmsg->command, "HANGUP")) {
		char *cause;
		char *q931cause;
		struct ast_channel *owner;
			
            
		if (option_verbose > 2 && woomera_profile_verbose(tech_pvt->profile) > 2) {
			ast_verbose(WOOMERA_DEBUG_PREFIX "Hangup [%s] \n", tech_pvt->callid);
		}
		cause = woomera_message_header(wmsg, "Cause");
		q931cause = woomera_message_header(wmsg, "Q931-Cause-Code");
		
		my_tech_pvt_and_owner_lock(tech_pvt);
		owner = tech_pvt->owner;
		if (cause && owner) {
			owner->hangupcause = string_to_release(cause);
			if (globals.debug > 2) {
			ast_log(LOG_NOTICE, "EVENT HANGUP with OWNER with Cause %s  %s tpvt=%p\n",
					cause,tech_pvt->callid,tech_pvt);
			}
		}else{
			if (globals.debug > 2) {
			ast_log(LOG_NOTICE, "EVENT HANGUP with Cause %s  %s tpvt=%p\n",
				cause,tech_pvt->callid,tech_pvt);
			}
		}

		if (q931cause && atoi(q931cause) && owner) {
			owner->hangupcause = atoi(q931cause);
		}
		my_tech_pvt_and_owner_unlock(tech_pvt);

		woomera_close_socket(&tech_pvt->command_channel);
				
		ast_set_flag(tech_pvt, TFLAG_ABORT);

	} else if (ast_test_flag(tech_pvt, TFLAG_ABORT)) {

		/* If we are in abort we only care about HANGUP
	           so we can get the return code */
		return;		

	} else if (!strcasecmp(wmsg->command, "DTMF")) {
			
		my_tech_pvt_and_owner_lock(tech_pvt);

		char *content_len = woomera_message_header(wmsg, "Content-Length");
		if (tech_pvt->owner && content_len && atoi(content_len) > 0) {
			int clen=atoi(content_len);
			int x;
			for (x = 0; x < clen; x++) {
				struct ast_frame dtmf_frame = {AST_FRAME_DTMF};
				dtmf_frame.W_SUBCLASS_INT = wmsg->body[x];
				if(dtmf_frame.W_SUBCLASS_INT == 'f') {
					handle_fax(tech_pvt);
				} else {
					ast_queue_frame(tech_pvt->owner, &dtmf_frame);
					
					if (globals.debug > 1 && option_verbose > 2 && woomera_profile_verbose(tech_pvt->profile) > 2) {
						ast_verbose(WOOMERA_DEBUG_PREFIX "SEND DTMF [%c] to %s\n", dtmf_frame.W_SUBCLASS_INT,tech_pvt->callid);
					}
				}
			}
		}

		my_tech_pvt_and_owner_unlock(tech_pvt);

	} else if (!strcasecmp(wmsg->command, "RBS")) {

		if (tech_pvt->bridge && tech_pvt->profile->rbs_relay) {
			my_tech_pvt_and_owner_lock(tech_pvt);
	
			char *content_len = woomera_message_header(wmsg, "Content-Length");
			if (tech_pvt->owner && content_len && atoi(content_len) > 0) {
				int clen=atoi(content_len);
				int x;
				int err;
				for (x = 0; x < clen; x++) {
					if (tech_pvt->rbs_frame.frametype == 0) {
						err=sscanf(&wmsg->body[x], "%X", &tech_pvt->rbs_frame.W_SUBCLASS_INT);
						if (err==1) {
							tech_pvt->rbs_frame.frametype = 99;
						}
						if (globals.debug > 1 && option_verbose > 2 && woomera_profile_verbose(tech_pvt->profile) > 2) {
							ast_verbose(WOOMERA_DEBUG_PREFIX "SEND RBS [%X] to %s\n", tech_pvt->rbs_frame.W_SUBCLASS_INT,tech_pvt->callid);
						}
					}
				}
			}
			my_tech_pvt_and_owner_unlock(tech_pvt);
		} else {
			if (globals.debug > 1 && option_verbose > 2 && woomera_profile_verbose(tech_pvt->profile) > 2) {
					ast_verbose(WOOMERA_DEBUG_PREFIX "Ignoring RBS rbs_relay not configured: [%X] to %s\n", tech_pvt->rbs_frame.W_SUBCLASS_INT,tech_pvt->callid);
			}
		}
		

	} else if (!strcasecmp(wmsg->command, "PROCEED")) {
		/* This packet has lots of info so well keep it */
		char *chan_name = woomera_message_header(wmsg, "Channel-Name");
		if (chan_name && ast_test_flag(tech_pvt, TFLAG_OUTBOUND)) {
			
			my_tech_pvt_and_owner_lock(tech_pvt);
			if (tech_pvt->owner) {
#if defined (AST14) || defined (AST16)
				char newname[100];
                        	snprintf(newname, 100, "%s/%s", WOOMERA_CHAN_NAME, chan_name);
				ast_change_name(tech_pvt->owner,newname);
#else
                        	snprintf(tech_pvt->owner->name, 100, "%s/%s", WOOMERA_CHAN_NAME, chan_name);
#endif
                 	}
			my_tech_pvt_and_owner_unlock(tech_pvt);
		}
		memcpy(&tech_pvt->call_info,wmsg,sizeof(*wmsg));
		

	} else if (ast_test_flag(tech_pvt, TFLAG_PARSE_INCOMING) && 
			!strcasecmp(wmsg->command, "INCOMING")) {
			
			/* The INCOMING EVENT should never come here becuase
			* there should be only a single LISTEN via main thread */
			
			ast_log(LOG_ERROR, "INVALID WOOMERA EVENT %s :"
					"callid=%s tech_callid=%s tpvt=%p\n",
				wmsg->command,wmsg->callid,
				tech_pvt->callid,tech_pvt);

	} else if (!strcasecmp(wmsg->command, "CONNECT")) {
		struct ast_frame answer_frame = {AST_FRAME_CONTROL, .W_SUBCLASS_INT = AST_CONTROL_ANSWER};

		my_tech_pvt_and_owner_lock(tech_pvt);
		if (tech_pvt->owner) {
			int answer = 0;
			if(ast_test_flag(tech_pvt, TFLAG_CONFIRM_ANSWER_ENABLED)){
				ast_set_flag(tech_pvt, TFLAG_ANSWER_RECEIVED);
				if(ast_test_flag(tech_pvt, TFLAG_CONFIRM_ANSWER)){
					ast_log(LOG_DEBUG, "Waiting on answer confirmation on channel %s!\n", woomera_message_header(wmsg, "Channel-Name"));
				} else {
					answer = 1;
				}
			} else {
				answer = 1;
			}
			if(answer){
				ast_setstate(tech_pvt->owner, AST_STATE_UP);
				ast_queue_frame(tech_pvt->owner, &answer_frame);
				ast_set_flag(tech_pvt, TFLAG_UP);
			
				ast_mutex_lock(&tech_pvt->profile->call_count_lock);
				tech_pvt->profile->call_ok++;
				ast_mutex_unlock(&tech_pvt->profile->call_count_lock);
			}		
		}else{
			ast_copy_string(tech_pvt->ds, "REQUESTED_CHAN_UNAVAIL", sizeof(tech_pvt->ds));
			tech_pvt->pri_cause=44;
			ast_set_flag(tech_pvt, TFLAG_ABORT);
		}
		my_tech_pvt_and_owner_unlock(tech_pvt);


	} else if (!strcasecmp(wmsg->command, "MEDIA")) {
		int err;
		
		memcpy(&tech_pvt->media_info,wmsg,sizeof(tech_pvt->media_info));
	
		err=woomera_event_media (tech_pvt, wmsg);
		if (err != 0) {
			ast_set_flag(tech_pvt, TFLAG_ABORT);
		}

	} else {
		if (!strcasecmp(wmsg->command, "INCOMING") ) {
			/* Do not print it here */
		}else{
			ast_log(LOG_ERROR, "Woomera Invalid CMD %s %s\n", 
					wmsg->command,tech_pvt->callid);
		}
	}
	return;
}

/*==============================================================================
  Module Load Section

  =============================================================================*/

int load_module(void)
{
	int x;

	if (ast_channel_register(&technology)) {
		ast_log(LOG_ERROR, "Unable to register channel class %s\n", WOOMERA_CHAN_NAME);
		return -1;
	}
	memset(&globals, 0, sizeof(globals));
	globals.next_woomera_port = woomera_base_media_port;
	/* Use more threads for better timing this adds a dedicated monitor thread to
	   every channel you can disable it with more_threads => no in [settings] */
	globals.more_threads = 1;

	ast_mutex_init(&globals.woomera_port_lock);
	ast_mutex_init(&default_profile.iolock);
	ast_mutex_init(&default_profile.call_count_lock);
	ast_mutex_init(&default_profile.event_queue.lock);

	memset(tech_pvt_idx, 0, sizeof(tech_pvt_idx));

	for (x=0;x<WOOMERA_MAX_CALLS;x++){
		ast_mutex_init(&tech_pvt_idx_lock[x]);
	}
	
	if (!init_woomera()) {
		return -1;
	}

	signal(SIGURG, urg_handler);

	ASTOBJ_CONTAINER_INIT(&private_object_list);
	/*
	sched = sched_context_create();
    if (!sched) {
        ast_log(LOG_NOTICE, "Unable to create schedule context\n");
    }
	*/
	
#ifdef CALLWEAVER
#ifdef CALLWEAVER_19
	cw_cli_register_multiple(cli_woomera, sizeof(cli_woomera) / sizeof(cli_woomera[0]));
#else
	opbx_cli_register_multiple(cli_woomera, arraysize(cli_woomera));
#endif
#else
#if defined(AST16)
	ast_cli_register(cli_woomera);
#else
	ast_cli_register(&cli_woomera);
#endif
#endif

#if !defined (AST14) && !defined (AST16) && !defined (CALLWEAVER_19) 
	ast_null_frame.frametype = AST_FRAME_NULL;
	ast_null_frame.datalen = 0;
	ast_null_frame.samples = 0;
	ast_null_frame.mallocd = 0;
	ast_null_frame.offset = 0;
	ast_null_frame.subclass = 0;
	ast_null_frame.delivery = ast_tv(0,0);
	ast_null_frame.src = "zt_exception";
	ast_null_frame.woo_ast_data_ptr = NULL;
#endif

	return 0;
}

int reload(void)
{
	return 0;
}

int unload_module(void)
{
	time_t then, now;
	woomera_profile *profile = NULL;
	int x;

	globals.panic=10;
	
	ASTOBJ_CONTAINER_TRAVERSE(&woomera_profile_list, 1, do {
		ASTOBJ_RDLOCK(iterator);
		profile = iterator;

		time(&then);
		if (!ast_test_flag(profile, PFLAG_DISABLED)) {
			ast_log(LOG_NOTICE, "Shutting Down Thread. {%s}\n", profile->name);
			woomera_profile_thread_running(profile, 1, 0);
			
			while (!woomera_profile_thread_running(profile, 0, 0)) {
				time(&now);
				if (now - then > 30) {
					ast_log(LOG_NOTICE, "Timed out waiting for thread to exit\n");
					break;
				}
				usleep(100);
			}
		}
		ASTOBJ_UNLOCK(iterator);
	} while(0));

	sleep(1);
	
	ast_log(LOG_NOTICE, "WOOMERA Unload %i\n",
			usecount());

	ast_mutex_destroy(&default_profile.iolock);
	ast_mutex_destroy(&default_profile.call_count_lock);
	ast_mutex_destroy(&default_profile.event_queue.lock);
	ast_mutex_destroy(&globals.woomera_port_lock);

	for (x=0;x<WOOMERA_MAX_CALLS;x++){
		ast_mutex_destroy(&tech_pvt_idx_lock[x]);
	}
	
#if defined (CALLWEAVER)
#if defined (CALLWEAVER_19)
	cw_cli_unregister_multiple(cli_woomera, sizeof(cli_woomera) / sizeof(cli_woomera[0]));
#else
	opbx_cli_unregister_multiple(cli_woomera, sizeof(cli_woomera) / sizeof(cli_woomera[0]));
#endif
#else
#if defined(AST16)
	ast_cli_unregister(cli_woomera);
#else
	ast_cli_unregister(&cli_woomera);
#endif
#endif
	ASTOBJ_CONTAINER_DESTROY(&private_object_list);
	ASTOBJ_CONTAINER_DESTROYALL(&woomera_profile_list, destroy_woomera_profile);
	//sched_context_destroy(sched);

	ast_channel_unregister(&technology);
	return 0;
}

#ifdef CALLWEAVER

#ifdef CALLWEAVER_MODULE_INFO
MODULE_INFO(load_module, reload, unload_module, NULL, desc);
#else
char *description()
{
        return (char *) desc;
}
#endif

#else 


# if !defined (AST14) && !defined (AST16)

char *key()
{
        return ASTERISK_GPL_KEY;
}

/* returns a descriptive string to the system console */
char *description()
{
        return (char *) desc;
}

# else


AST_MODULE_INFO(ASTERISK_GPL_KEY, AST_MODFLAG_DEFAULT, "Woomera Protocol (WOOMERA)",
		.load = load_module,
		.unload = unload_module,
		.reload = reload,
	       );
# endif

#endif
 
static int init_woomera(void) 
{
	woomera_profile *profile;
	ast_mutex_lock(&lock);
	
	if (!config_woomera()) {
		ast_mutex_unlock(&lock);
		return 0;
	}
	
	ASTOBJ_CONTAINER_TRAVERSE(&woomera_profile_list, 1, do {
		ASTOBJ_RDLOCK(iterator);
		profile = iterator;
		if (!ast_test_flag(profile, PFLAG_DISABLED)) {
			launch_woomera_thread(profile);
		}
		ASTOBJ_UNLOCK(iterator);
	} while(0));

	ast_mutex_unlock(&lock);
	return 1;
}                        

