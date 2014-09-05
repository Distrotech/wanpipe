//////////////////////////////////////////////////////////////////////
// sangoma_cthread.h: generic C++ Execution Thread.
//
// Author	:	David Rokhvarg
//////////////////////////////////////////////////////////////////////

#ifndef _SANGOMA_CTHREAD_H
#define _SANGOMA_CTHREAD_H

struct ThreadParam;

class sangoma_cthread {

public:
	sangoma_cthread();
	virtual ~sangoma_cthread();
	bool CreateThread (int iParam);

protected:
	virtual unsigned long threadFunction(struct ThreadParam& thParam) = 0;
#if defined (__WINDOWS__)
	DWORD	dwThreadId;
	HANDLE	hThread;
#endif

private:
#if defined(__WINDOWS__)
	static int runThread (void *p);
#else
	static void *runThread (void *p);
#endif
};

struct ThreadParam {
	sangoma_cthread*	pThread;
	int					iParam;
};

#endif//_SANGOMA_CTHREAD_H

