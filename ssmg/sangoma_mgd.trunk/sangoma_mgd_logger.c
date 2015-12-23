/*********************************************************************************
 * sangoma_mgd_logger.c --  Sangoma Media Gateway Daemon for Sangoma/Wanpipe Cards
 *
 * Copyright 05-08, Nenad Corbic <ncorbic@sangoma.com>
 *		    
 *
 * This program is free software, distributed under the terms of
 * the GNU General Public License
 */


#include "sangoma_mgd.h"
#include "sangoma_mgd_memdbg.h"


extern int pipe_fd;
extern struct woomera_server server; 


LIST_HEAD(smg_log_list);
static pthread_t logger_thread;
static pthread_mutex_t smg_log_element_lock;
static int smg_log_thread_running;
static int  smg_log_thread_up=0;

typedef struct smg_log_element 
{
	char *data;
	int level;
	struct list_head clist; 
}smg_log_element_t;


void __log_printf(int level, FILE *fp, char *file, const char *func, int line, char *fmt, ...)
{
	char *data;
	int ret = 0;
	va_list ap;

#ifdef USE_LOG_THREAD
	smg_log_element_t *log_el;
#endif

	if (socket < 0) {
		return;
	}
	
	if (fp == NULL) {
		fp = server.log;
	} 
	
	if (level && level > server.debug && level < 100) {
		return;
	}

#ifdef 	USE_LOG_THREAD
	log_el = smg_malloc(sizeof(smg_log_element_t));
	if (!log_el) {
		return;
	}
	memset(log_el,0,sizeof(log_el));
#endif
	
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
#ifdef 	USE_LOG_THREAD
			free(log_el);
#endif			
	} else {
			char date[80] = "";
			struct tm now;
			time_t epoch;
	
			if (time(&epoch) && localtime_r(&epoch, &now)) {
				strftime(date, sizeof(date), "%Y-%m-%d %T", &now);
			}
	
#ifdef USE_LOG_THREAD
			log_el->data=data;
			log_el->level = level;
			pthread_mutex_lock(&smg_log_element_lock);
			list_add_tail(&log_el->clist, &smg_log_list);
			pthread_mutex_unlock(&smg_log_element_lock);
#else
# ifdef USE_SYSLOG 
			syslog(LOG_DEBUG | LOG_LOCAL2, data);
# else
			fprintf(fp, "[%d] %s %s:%d %s() %s", getpid(), date, file, line, func, data);
			fflush(fp);
# endif
			free(data);
#endif
	
	}
}



static void *logger_thread_run(void *obj)
{
	smg_log_element_t *log_el;
	char *data;

	printf("SMG Log Thread Running ...\n");

	smg_log_thread_up = 1;

	while (smg_log_thread_running) {

		
		
		if (list_empty(&smg_log_list)) {
			usleep(5000);
			sched_yield();
			continue;
		}	

		log_el=NULL;
		data=NULL;
	
		pthread_mutex_lock(&smg_log_element_lock);
		log_el = list_first_entry(&smg_log_list, smg_log_element_t, clist);
		if (log_el) {
			data=log_el->data;
			list_del(&log_el->clist);
		}
		pthread_mutex_unlock(&smg_log_element_lock);

		if (data) {
			if (log_el->level == 100) {
				syslog(LOG_DEBUG | LOG_LOCAL3, "%s", data);
			} else {
				syslog(LOG_DEBUG | LOG_LOCAL2, "%s", data);
			}
			free(data);
		}
		if (log_el){
			free(log_el);
		}
	}

	printf("SMG Log Thread Exiting ...\n");

	pthread_exit(NULL);
	return NULL;
}


static int launch_logger_thread(void) 
{
	pthread_attr_t attr;
	int result = -1;

	printf("SMG Log Thread Starting ...\n");

	result = pthread_attr_init(&attr);
        //pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);
	//pthread_attr_setschedpolicy(&attr, SCHED_RR);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	pthread_attr_setstacksize(&attr, MGD_STACK_SIZE);
	smg_log_thread_running=1;

	result = pthread_create(&logger_thread, &attr, logger_thread_run, NULL);
	if (result) {
		log_printf(0, server.log, "%s: Error: Creating Thread! %s\n",
				 __FUNCTION__,strerror(errno));
		smg_log_thread_running=0;
    	} 
	pthread_attr_destroy(&attr);

   	return result;
}

static int smg_log_ready(void)
{
	int timeout=5;
	while (timeout && !smg_log_thread_up) {
		sleep(1);
		timeout--;
	}
	if (smg_log_thread_up) {
		return 0;
	}

	return 1;
}

int smg_log_init(void)
{
	int err;
	pthread_mutex_init(&smg_log_element_lock, NULL);

	INIT_LIST_HEAD(&smg_log_list);
	
	err=launch_logger_thread();
	if (err) {
		return err;
	}

	return smg_log_ready();
}

void smg_log_cleanup(void)
{
	
	smg_log_thread_running=0;
 
	usleep(50000);

	printf("SMG Log Clenaup!\n");	

	pthread_mutex_destroy(&smg_log_element_lock);
}

 
