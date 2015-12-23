#ifndef  __LINUX_COMPAT_H__
#define __LINUX_COMPAT_H__

#include <sys/time.h>
#include <pthread.h>

#define SYSTEMTIME  struct tm

#define wMonth			tm_mon
#define wDay			tm_mday
#define wYear			tm_year
#define wHour			tm_hour
#define wMinute			tm_min
#define wSecond			tm_sec

#define LPCTSTR char *
#define STELAPI_CALL

#ifndef BOOL
#define BOOL bool
#endif

#ifndef CHAR
#define CHAR char
#endif

#define IN	
#define OUT


#ifndef FALSE
#define FALSE   0
#ifndef TRUE
#define TRUE    (!FALSE)
#endif
#endif

static __inline int GetLocalTime(SYSTEMTIME *tv)
{
	time_t now = time(NULL);
	*tv = *localtime(&now);
	return 0;
}

#define CRITICAL_SECTION 	pthread_mutex_t

#define EnterCriticalSection(arg) 	pthread_mutex_lock(arg)
#define LeaveCriticalSection(arg) 	pthread_mutex_unlock(arg)
#define InitializeCriticalSection(arg) pthread_mutex_init(arg, NULL);

typedef struct _variant
{
		int vt;
		int intVal; 
}variant_t;

#define VARIANT variant_t
#define VT_UI4						0
#define WFI_CCITT_ALaw_8kHzMono 	1 
#define WFI_CCITT_uLaw_8kHzMono 	2

#endif /* __LINUX_COMPAT_H__ */
