/*
 * Asterisk -- A telephony toolkit for Linux.
 *
 * This program is free software, distributed under the terms of
 * the GNU General Public License
 * app_changrab.c 
 * Copyright Anthony C Minessale II <anthmct@yahoo.com>
 * 
 * Thanks to Claude Patry <cpatry@gmail.com> for his help.
 */

/*uncomment below or build with -DAST_10_COMPAT for 1.0 */ 
//#define AST_10_COMPAT
#include <stdio.h> 
#include <asterisk/file.h>
#include <asterisk/logger.h>
#include <asterisk/channel.h>
#include <asterisk/pbx.h>
#include <asterisk/utils.h>
#include <asterisk/musiconhold.h>
#include <asterisk/module.h>
#include <asterisk/features.h>
#include <asterisk/cli.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>

#include "asterisk.h"

ASTERISK_FILE_VERSION(__FILE__, "$Revision: 1.48 $")

static char *tdesc = "Take over an existing channel and bridge to it.";
static char *app = "ChanGrab";
static char *synopsis = "ChanGrab(<Channel Name>|[flags])\n"
"\nTake over the specified channel (ending any call it is currently\n"
"involved in.) and bridge that channel to the caller.\n\n"
"Flags:\n\n"
"   -- 'b' Indicates that you want to grab the channel that the\n"
"          specified channel is Bridged to.\n\n"
"   -- 'r' Only incercept the channel if the channel has not\n"
"          been answered yet\n";

STANDARD_LOCAL_USER;
LOCAL_USER_DECL;


static struct ast_channel *my_ast_get_channel_by_name_locked(char *channame) {
	struct ast_channel *chan;
	chan = ast_channel_walk_locked(NULL);
	while(chan) {
		if (!strncasecmp(chan->name, channame, strlen(channame)))
			return chan;
		ast_mutex_unlock(&chan->lock);
		chan = ast_channel_walk_locked(chan);
	}
	return NULL;
}

static int changrab_exec(struct ast_channel *chan, void *data)
{
	int res=0;
	struct localuser *u;
	struct ast_channel *newchan;
	struct ast_channel *oldchan;
    struct ast_frame *f;
	struct ast_bridge_config config;
	char *info=NULL;
	char *flags=NULL;

	if (!data || ast_strlen_zero(data)) {
		ast_log(LOG_WARNING, "changrab requires an argument (chan)\n");
		return -1;
	}

	if((info = ast_strdupa((char *)data))) {
		if((flags = strchr(info,'|'))) {

			*flags = '\0';
			flags++;
		}
	}
	
	if((oldchan = my_ast_get_channel_by_name_locked(info))) {
		ast_mutex_unlock(&oldchan->lock);
	} else {
		ast_log(LOG_WARNING, "No Such Channel: %s\n",(char *) data);
		return -1;
	}
	
	if(flags && oldchan->_bridge && strchr(flags,'b')) {
		oldchan = oldchan->_bridge;
	}

	if(flags && strchr(flags,'r') && oldchan->_state == AST_STATE_UP) {
		return -1;
	}
	
	LOCAL_USER_ADD(u);
	newchan = ast_channel_alloc(0);
	snprintf(newchan->name, sizeof (newchan->name), "ChanGrab/%s",oldchan->name);
	newchan->readformat = oldchan->readformat;
	newchan->writeformat = oldchan->writeformat;
	ast_channel_masquerade(newchan, oldchan);
	if((f = ast_read(newchan))) {
		ast_frfree(f);
		memset(&config,0,sizeof(struct ast_bridge_config));
		ast_set_flag(&(config.features_callee), AST_FEATURE_REDIRECT);
		ast_set_flag(&(config.features_caller), AST_FEATURE_REDIRECT);

		if(newchan->_state != AST_STATE_UP) {
			ast_answer(newchan);
		}
		
		chan->appl = "Bridged Call";
		res = ast_bridge_call(chan, newchan, &config);
		ast_hangup(newchan);
	}

	LOCAL_USER_REMOVE(u);
	return res ? 0 : -1;
}


struct ast_bridge_thread_obj 
{
	struct ast_bridge_config bconfig;
	struct ast_channel *chan;
	struct ast_channel *peer;
};

static void *ast_bridge_call_thread(void *data) 
{
	struct ast_bridge_thread_obj *tobj = data;
	tobj->chan->appl = "Redirected Call";
	tobj->chan->data = tobj->peer->name;
	tobj->peer->appl = "Redirected Call";
	tobj->peer->data = tobj->chan->name;
	if (tobj->chan->cdr) {
		ast_cdr_reset(tobj->chan->cdr,0);
		ast_cdr_setdestchan(tobj->chan->cdr, tobj->peer->name);
	}
	if (tobj->peer->cdr) {
		ast_cdr_reset(tobj->peer->cdr,0);
		ast_cdr_setdestchan(tobj->peer->cdr, tobj->chan->name);
	}


	ast_bridge_call(tobj->peer, tobj->chan, &tobj->bconfig);
	ast_hangup(tobj->chan);
	ast_hangup(tobj->peer);
	tobj->chan = tobj->peer = NULL;
	free(tobj);
	tobj=NULL;
	return NULL;
}

static void ast_bridge_call_thread_launch(struct ast_channel *chan, struct ast_channel *peer) 
{
	pthread_t thread;
	pthread_attr_t attr;
	int result;
	struct ast_bridge_thread_obj *tobj;
	
	if((tobj = malloc(sizeof(struct ast_bridge_thread_obj)))) {
		memset(tobj,0,sizeof(struct ast_bridge_thread_obj));
		tobj->chan = chan;
		tobj->peer = peer;
		

		result = pthread_attr_init(&attr);
		pthread_attr_setschedpolicy(&attr, SCHED_RR);
		pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
		result = ast_pthread_create(&thread, &attr,ast_bridge_call_thread, tobj);
		result = pthread_attr_destroy(&attr);
	}
}


#define CGUSAGE "Usage: changrab [-[bB]] <channel> <exten>@<context> [pri]\n"
static int changrab_cli(int fd, int argc, char *argv[]) {
	char *chan_name_1, *chan_name_2 = NULL, *context,*exten,*flags=NULL;
	char *pria = NULL;
    struct ast_channel *xferchan_1, *xferchan_2;
	int pri=0,x=1;

	if(argc < 3) {
		ast_cli(fd,CGUSAGE);
		return -1;
	}
	chan_name_1 = argv[x++];
	if(chan_name_1[0] == '-') {
		flags = ast_strdupa(chan_name_1);
		if(flags && (strchr(flags,'h'))) {
			chan_name_1 = argv[x++];
			if((xferchan_1 = my_ast_get_channel_by_name_locked(chan_name_1))) {
				ast_mutex_unlock(&xferchan_1->lock);
				ast_hangup(xferchan_1);
				ast_verbose("OK, good luck!\n");
				return 0;
			} else 
				return -1;
		} else if(flags && (strchr(flags,'m') || strchr(flags,'M'))) {
			chan_name_1 = argv[x++];
			if((xferchan_1 = my_ast_get_channel_by_name_locked(chan_name_1))) {
				ast_mutex_unlock(&xferchan_1->lock);
				strchr(flags,'m') ? ast_moh_start(xferchan_1,NULL) : ast_moh_stop(xferchan_1);
			} else 
				return 1;
			return 0;
		}
		if(argc < 4) {
			ast_cli(fd,CGUSAGE);
			return -1;
		}
		chan_name_1 = argv[x++];
	}

	exten = ast_strdupa(argv[x++]);
	if((context = strchr(exten,'@'))) {
		*context = 0;
		context++;
		if(!(context && exten)) {
			ast_cli(fd,CGUSAGE);
			return -1;
		}
		if((pria = strchr(context,':'))) {
			*pria = '\0';
			pria++;
			pri = atoi(pria);
		} else {
			pri = argv[x] ? atoi(argv[x++]) : 1;
		}
		if(!pri)
			pri = 1;
	} else if (strchr(exten,'/')) {
		chan_name_2 = exten;
	}

	
	xferchan_1 = my_ast_get_channel_by_name_locked(chan_name_1);

	if(!xferchan_1) {
		ast_log(LOG_WARNING, "No Such Channel: %s\n",chan_name_1);
		return -1;
	} 

	ast_mutex_unlock(&xferchan_1->lock);
	if(flags && strchr(flags,'b')) {
		if(ast_bridged_channel(xferchan_1)) {
			xferchan_1 = ast_bridged_channel(xferchan_1);
		}
	}

	if(chan_name_2) {
		struct ast_frame *f;
		struct ast_channel *newchan_1, *newchan_2;
		
		if (!(newchan_1 = ast_channel_alloc(0))) {
			ast_log(LOG_WARNING, "Memory Error!\n");
			ast_hangup(newchan_1);
			return -1;
		} else {
			snprintf(newchan_1->name, sizeof (newchan_1->name), "ChanGrab/%s", xferchan_1->name);
			newchan_1->readformat = xferchan_1->readformat;
			newchan_1->writeformat = xferchan_1->writeformat;
			ast_channel_masquerade(newchan_1, xferchan_1);
			if ((f = ast_read(newchan_1))) {
				ast_frfree(f);
			} else {
				ast_hangup(newchan_1);
				return -1;
			}
		}

		if(!(xferchan_2 = my_ast_get_channel_by_name_locked(chan_name_2))) {
			ast_log(LOG_WARNING, "No Such Channel: %s\n",chan_name_2);
			ast_hangup(newchan_1);
			return -1;
		}

		ast_mutex_unlock(&xferchan_2->lock);		

		if(flags && strchr(flags, 'B')) {
			if(ast_bridged_channel(xferchan_2)) {
				xferchan_2 = ast_bridged_channel(xferchan_2);
			}
		}

		if(!(newchan_2 = ast_channel_alloc(0))) {
			ast_log(LOG_WARNING, "Memory Error!\n");
			ast_hangup(newchan_1);
			return -1;
		} else {
			snprintf(newchan_2->name, sizeof (newchan_2->name), "ChanGrab/%s", xferchan_2->name);
			newchan_2->readformat = xferchan_2->readformat;
			newchan_2->writeformat = xferchan_2->writeformat;
			ast_channel_masquerade(newchan_2, xferchan_2);

			if ((f = ast_read(newchan_2))) {
				ast_frfree(f);
			} else {
				ast_hangup(newchan_1);
				ast_hangup(newchan_2);
				return -1;
			}
		}
		
		ast_bridge_call_thread_launch(newchan_1, newchan_2);
		
	} else {
		ast_verbose("Transferring_to context %s, extension %s, priority %d\n", context, exten, pri);
		ast_async_goto(xferchan_1, context, exten, pri);

		if(xferchan_1)
			ast_mutex_unlock(&xferchan_1->lock);
	}
	return 0;
}

struct fast_originate_helper {
	char tech[256];
	char data[256];
	int timeout;
	char app[256];
	char appdata[256];
	char cid_name[256];
	char cid_num[256];
	char context[256];
	char exten[256];
	char idtext[256];
	int priority;
	struct ast_variable *vars;
};



#define USAGE "Usage: originate <channel> <exten>@<context> [pri] [callerid] [timeout]\n"

static void *originate(void *arg) {
	struct fast_originate_helper *in = arg;
	int reason=0;
	struct ast_channel *chan = NULL;

	ast_pbx_outgoing_exten(in->tech, AST_FORMAT_SLINEAR, in->data, in->timeout, in->context, in->exten, in->priority, &reason, 1, !ast_strlen_zero(in->cid_num) ? in->cid_num : NULL, !ast_strlen_zero(in->cid_name) ? in->cid_name : NULL, NULL, NULL, &chan);

	/* Locked by ast_pbx_outgoing_exten or ast_pbx_outgoing_app */
	if (chan) {
		ast_mutex_unlock(&chan->lock);
	}
	free(in);
	return NULL;
}


static int originate_cli(int fd, int argc, char *argv[]) {
	char *chan_name_1,*context,*exten,*tech,*data,*callerid;
	int pri=0,to=60000;
	struct fast_originate_helper *in;
	pthread_t thread;
	pthread_attr_t attr;
	int result;
	char *num = NULL;

	if(argc < 3) {
		ast_cli(fd,USAGE);
		return -1;
	}
	chan_name_1 = argv[1];

	exten = ast_strdupa(argv[2]);
	if((context = strchr(exten,'@'))) {
		*context = 0;
		context++;
	}
	if(! (context && exten)) {
		ast_cli(fd,CGUSAGE);
        return -1;
	}


	pri = argv[3] ? atoi(argv[3]) : 1;
	if(!pri)
		pri = 1;

	tech = ast_strdupa(chan_name_1);
	if((data = strchr(tech,'/'))) {
		*data = '\0';
		data++;
	}
	if(!(tech && data)) {
		ast_cli(fd,USAGE);
        return -1;
	}
	in = malloc(sizeof(struct fast_originate_helper));
	if(!in) {
		ast_cli(fd,"No Memory!\n");
        return -1;
	}
	memset(in,0,sizeof(struct fast_originate_helper));
	
	callerid = (argc > 4)  ? argv[4] : NULL;
	to = (argc > 5) ? atoi(argv[5]) : 60000;

	strncpy(in->tech,tech,sizeof(in->tech));
	strncpy(in->data,data,sizeof(in->data));
	in->timeout=to;
	if(callerid) {
		if((num = strchr(callerid,':'))) {
			*num = '\0';
			num++;
			strncpy(in->cid_num,num,sizeof(in->cid_num));
		}
		strncpy(in->cid_name,callerid,sizeof(in->cid_name));
	}
	strncpy(in->context,context,sizeof(in->context));
	strncpy(in->exten,exten,sizeof(in->exten));
	in->priority = pri;

	ast_cli(fd,"Originating Call %s/%s %s %s %d\n",in->tech,in->data,in->context,in->exten,in->priority);


	result = pthread_attr_init(&attr);
	pthread_attr_setschedpolicy(&attr, SCHED_RR);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	result = ast_pthread_create(&thread, &attr,originate,in);
	result = pthread_attr_destroy(&attr);	
	return 0;
}



static char *complete_exten_at_context(char *line, char *word, int pos,
	int state)
{
	char *ret = NULL;
	int which = 0;

#ifdef BROKEN_READLINE
	/*
	 * Fix arguments, *word is a new allocated structure, REMEMBER to
	 * free *word when you want to return from this function ...
	 */
	if (fix_complete_args(line, &word, &pos)) {
		ast_log(LOG_ERROR, "Out of free memory\n");
		return NULL;
	}
#endif

	/*
	 * exten@context completion ... 
	 */
	if (pos == 2) {
		struct ast_context *c;
		struct ast_exten *e;
		char *context = NULL, *exten = NULL, *delim = NULL;

		/* now, parse values from word = exten@context */
		if ((delim = strchr(word, (int)'@'))) {
			/* check for duplicity ... */
			if (delim != strrchr(word, (int)'@')) {
#ifdef BROKEN_READLINE
				free(word);
#endif
				return NULL;
			}

			*delim = '\0';
			exten = strdup(word);
			context = strdup(delim + 1);
			*delim = '@';
		} else {
			exten = strdup(word);
		}
#ifdef BROKEN_READLINE
		free(word);
#endif

		if (ast_lock_contexts()) {
			ast_log(LOG_ERROR, "Failed to lock context list\n");
			free(context); free(exten);
			return NULL;
		}

		/* find our context ... */
		c = ast_walk_contexts(NULL); 
		while (c) {
			/* our context? */
			if ( (!context || !strlen(context)) || /* if no input, all contexts ... */
				 (context && !strncmp(ast_get_context_name(c),
				              context, strlen(context))) ) { /* if input, compare ... */
				/* try to complete extensions ... */
				e = ast_walk_context_extensions(c, NULL);
				while (e) {

					if(!strncasecmp(ast_get_context_name(c),"macro-",6) || !strncasecmp(ast_get_extension_name(e),"_",1)) {
						e = ast_walk_context_extensions(c, e);
						continue;
					}

					/* our extension? */
					if ( (!exten || !strlen(exten)) ||  /* if not input, all extensions ... */
						 (exten && !strncmp(ast_get_extension_name(e), exten,
						                    strlen(exten))) ) { /* if input, compare ... */
								
						if (++which > state) {
							/* If there is an extension then return
							 * exten@context.
							 */


							if (exten) {
								ret = malloc(strlen(ast_get_extension_name(e)) +
									strlen(ast_get_context_name(c)) + 2);
								if (ret)
									sprintf(ret, "%s@%s", ast_get_extension_name(e),
											ast_get_context_name(c));
								
							}
							free(exten); free(context);

							ast_unlock_contexts();
	
							return ret;
						}
					}
					e = ast_walk_context_extensions(c, e);
				}
			}
			c = ast_walk_contexts(c);
		}

		ast_unlock_contexts();

		free(exten); free(context);

		return NULL;
	}

	/*
	 * Complete priority ...
	 */
	if (pos == 3) {
		char *delim, *exten, *context, *dupline, *duplinet, *ec;
		struct ast_context *c;

		dupline = strdup(line);
		if (!dupline) {
#ifdef BROKEN_READLINE
			free(word);
#endif
			return NULL;
		}
		duplinet = dupline;

		strsep(&duplinet, " "); /* skip 'remove' */
		strsep(&duplinet, " "); /* skip 'extension */

		if (!(ec = strsep(&duplinet, " "))) {
			free(dupline);
#ifdef BROKEN_READLINE
			free(word);
#endif
			return NULL;
		}

		/* wrong exten@context format? */
		if (!(delim = strchr(ec, (int)'@')) ||
			(strchr(ec, (int)'@') != strrchr(ec, (int)'@'))) {
#ifdef BROKEN_READLINE
			free(word);
#endif
			free(dupline);
			return NULL;
		}

		/* check if there is exten and context too ... */
		*delim = '\0';
		if ((!strlen(ec)) || (!strlen(delim + 1))) {
#ifdef BROKEN_READLINE
			free(word);
#endif
			free(dupline);
			return NULL;
		}

		exten = strdup(ec);
		context = strdup(delim + 1);
		free(dupline);

		if (ast_lock_contexts()) {
			ast_log(LOG_ERROR, "Failed to lock context list\n");
#ifdef BROKEN_READLINE
			free(word);
#endif
			free(exten); free(context);
			return NULL;
		}

		/* walk contexts */
		c = ast_walk_contexts(NULL); 
		while (c) {
			if (!strcmp(ast_get_context_name(c), context)) {
				struct ast_exten *e;

				/* walk extensions */
				free(context);
				e = ast_walk_context_extensions(c, NULL); 
				while (e) {
					if (!strcmp(ast_get_extension_name(e), exten)) {
						struct ast_exten *priority;
						char buffer[10];
					
						free(exten);
						priority = ast_walk_extension_priorities(e, NULL);
						/* serve priorities */
						do {
							snprintf(buffer, 10, "%u",
								ast_get_extension_priority(priority));
							if (!strncmp(word, buffer, strlen(word))) {
								if (++which > state) {
#ifdef BROKEN_READLINE
									free(word);
#endif
									ast_unlock_contexts();
									return strdup(buffer);
								}
							}
							priority = ast_walk_extension_priorities(e,
								priority);
						} while (priority);

#ifdef BROKEN_READLINE
						free(word);
#endif
						ast_unlock_contexts();
						return NULL;			
					}
					e = ast_walk_context_extensions(c, e);
				}
#ifdef BROKEN_READLINE
				free(word);
#endif
				free(exten);
				ast_unlock_contexts();
				return NULL;
			}
			c = ast_walk_contexts(c);
		}

#ifdef BROKEN_READLINE
		free(word);
#endif
		free(exten); free(context);

		ast_unlock_contexts();
		return NULL;
	}

#ifdef BROKEN_READLINE
	free(word);
#endif
	return NULL; 
}


static char *complete_ch_helper(char *line, char *word, int pos, int state)
{
    struct ast_channel *c;
    int which=0;
    char *ret;

    c = ast_channel_walk_locked(NULL);
    while(c) {
        if (!strncasecmp(word, c->name, strlen(word))) {
            if (++which > state)
                break;
        }
        ast_mutex_unlock(&c->lock);
        c = ast_channel_walk_locked(c);
    }
    if (c) {
        ret = strdup(c->name);
        ast_mutex_unlock(&c->lock);
    } else
        ret = NULL;
    return ret;
}

static char *complete_cg(char *line, char *word, int pos, int state)
{

	if(pos == 1) {
		return complete_ch_helper(line, word, pos, state);
	}
	else if(pos >= 2) {
		return complete_exten_at_context(line, word, pos, state);
	}
	return NULL;

}

static char *complete_org(char *line, char *word, int pos, int state)
{

	if(pos >= 2) {
		return complete_exten_at_context(line, word, pos, state);
	}
	return NULL;

}


static struct ast_cli_entry  cli_changrab = { { "changrab", NULL}, changrab_cli, "ChanGrab", "ChanGrab", complete_cg };
static struct ast_cli_entry  cli_originate = { { "cgoriginate", NULL }, originate_cli, "Originate", "Originate", complete_org};

int unload_module(void)
{
	STANDARD_HANGUP_LOCALUSERS;
	ast_cli_unregister(&cli_changrab);
	ast_cli_unregister(&cli_originate);
	return ast_unregister_application(app);
}

int load_module(void)
{
	ast_cli_register(&cli_changrab);
	ast_cli_register(&cli_originate);
	return ast_register_application(app, changrab_exec, tdesc, synopsis);
}

char *description(void)
{
	return tdesc;
}

int usecount(void)
{
	int res;
	STANDARD_USECOUNT(res);
	return res;
}

char *key()
{
	return ASTERISK_GPL_KEY;
}
