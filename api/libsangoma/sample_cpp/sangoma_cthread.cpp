//////////////////////////////////////////////////////////////////////
// sangoma_cthread.cpp: generic C++ Execution Thread.
//
// Author	:	David Rokhvarg
//////////////////////////////////////////////////////////////////////

#if defined(__WINDOWS__)
# include <windows.h>
#else
# include <string.h>
# include <stdarg.h>
# include <stdlib.h>
# include <sys/types.h>
# include <sys/socket.h>
# include <netinet/in.h>
# include <arpa/inet.h>
# include <netdb.h>
# include <stdio.h>
# include <unistd.h>
# include <fcntl.h>
# include <sys/time.h>
# include <poll.h>
# include <signal.h>
# include <pthread.h>
# define CTHREAD_STACK_SIZE 1024 * 240
#endif

# include "sangoma_cthread.h"

sangoma_cthread::sangoma_cthread ()
{
}

sangoma_cthread::~sangoma_cthread ()
{
}

bool sangoma_cthread::CreateThread (int iParam)
{

#if defined (__WINDOWS__)

	ThreadParam	*pParam = new ThreadParam;
		
	pParam->pThread	= this;
	pParam->iParam = iParam;

	hThread = ::CreateThread (NULL, 0, (unsigned long (__stdcall *)(void *))runThread, (void *)(pParam), 0, &dwThreadId);

	if(hThread){
		return true;
	}else{
		return false;
	}
#else
	pthread_t dwThreadId;
	pthread_attr_t attr;
	int result = -1;

	ThreadParam	*pParam = new ThreadParam;
		
	pParam->pThread	= this;
	pParam->iParam = iParam;

	result = pthread_attr_init(&attr);
   	//pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);
	//pthread_attr_setschedpolicy(&attr, SCHED_RR);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	pthread_attr_setstacksize(&attr, CTHREAD_STACK_SIZE);

	result = pthread_create(&dwThreadId, &attr, runThread, pParam);
	pthread_attr_destroy(&attr);
	if (result) {
		return false;
	}

	return true;
#endif
}

#if defined(__WINDOWS__)
int sangoma_cthread::runThread (void* Param)
#else
void *sangoma_cthread::runThread (void* Param)
#endif
{
	sangoma_cthread		*thread;
	int			tmp;
	ThreadParam	*pParam = (ThreadParam*)Param;
	ThreadParam	tmpParam;

	thread	= (sangoma_cthread*)pParam->pThread;
	tmp		= pParam->iParam;

	memcpy(&tmpParam, Param, sizeof(ThreadParam));

	delete	((ThreadParam*)Param);

#if defined(__WINDOWS__)
	return thread->threadFunction(tmpParam);
#else
	thread->threadFunction(tmpParam);
	pthread_exit(NULL);
#endif
}
