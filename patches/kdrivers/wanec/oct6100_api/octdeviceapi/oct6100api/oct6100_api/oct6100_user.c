/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*\

File: oct6100_user.c

	Copyright (c) 2001 Octasic Inc. All rights reserved.
    
Description: 

	This file contains the functions provided by the user.

This source code is Octasic Confidential. Use of and access to this code
is covered by the Octasic Device Enabling Software License Agreement. 
Acknowledgement of the Octasic Device Enabling Software License was 
required for access to this code. A copy was also provided with the release.


$Octasic_Release: OCT612xAPI-01.00-PR37 $

$Octasic_Revision: 25 $

\*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/


/*****************************  INCLUDE FILES  *******************************/

/* System specific includes */
#if defined(WAN_EC_USER)
# include <unistd.h>
# include <stdio.h>
# include <stdlib.h>
# include <time.h>
# include <sys/fcntl.h>
# include <sys/time.h>
# include <sys/stat.h>
# include <sys/types.h>
# include <sys/ipc.h>
# include <sys/sem.h>
# include <sys/queue.h>
# include <getopt.h>
# include <limits.h>
# include <errno.h>
# include <memory.h>
# include <signal.h>
# include <semaphore.h>
# include <sys/ioctl.h>
# if defined(__LINUX__)
#  include <linux/wanpipe_defines.h>
#  include <linux/wanpipe_cfg.h>
# elif defined(__FreeBSD__) || defined(__OpenBSD__)
#  include <wanpipe_defines.h>
#  include <wanpipe_cfg.h>
# endif
# include "oct6100api/oct6100_apiud.h"
# include "oct6100api/oct6100_errors.h"
# include "oct6100api/oct6100_api.h"
# include "oct6100_version.h"
# include "wanec_iface.h"
#else
# if defined(__LINUX__)
#  include <linux/wanpipe_includes.h>
#  include <linux/wanpipe_defines.h>
#  include <linux/wanpipe_debug.h>
#  include <linux/wanpipe_common.h>
#  include <linux/wanpipe_cfg.h>
#  include <linux/if_wanpipe.h>
#  include <linux/wanpipe.h>
# else
#  include <wanpipe_includes.h>
#  include <wanpipe_defines.h>
#  include <wanpipe_debug.h>
#  include <wanpipe_common.h>
#  include <wanpipe.h>
#  include <wanpipe_cfg.h>
# endif
# include "oct6100api/oct6100_apiud.h"
# include "oct6100api/oct6100_errors.h"
# include "oct6100api/oct6100_api.h"
# include "oct6100_version.h"
# include "wanec_iface.h"
#endif

extern u_int32_t wanec_req_write(void*, u_int32_t write_addr, u_int16_t write_data);
extern u_int32_t wanec_req_write_smear(void*, u_int32_t addr, u_int16_t data, u_int32_t len);
extern u_int32_t wanec_req_write_burst(void*, u_int32_t addr, u_int16_t *data, u_int32_t len);
extern u_int32_t wanec_req_read(void*, u_int32_t addr, u_int16_t *data);
extern u_int32_t wanec_req_read_burst(void*, u_int32_t addr, u_int16_t *data, u_int32_t len);

/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*\

Function:		Oct6100UserGetTime

Description:	Returns the system time in us.

-------------------------------------------------------------------------------
|	Argument		|	Description
-------------------------------------------------------------------------------
IN OUT f_pTime		Pointer to structure in which the time is returned.
 
\*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/

UINT32 Oct6100UserGetTime(

			IN OUT tPOCT6100_GET_TIME	f_pTime )
{
	clock_t		ulClockTicks;
	struct timeval	TimeVal;
	static UINT32	ulWallTimeUsHigh = 0;
	static UINT32	ulWallTimeUsLow = 0;

	if( !f_pTime )
		return cOCT6100_GET_TIME_FAILED_0;

#if !defined(__WINDOWS__)
	/* Retrieve clock tick */
# if defined(WAN_KERNEL)
	wan_getcurrenttime( &TimeVal.tv_sec, &TimeVal.tv_usec );
# else
	gettimeofday( &TimeVal, NULL );
# endif
	/* ulClockTicks = ( TimeVal.tv_sec * 1000000 ) + ( TimeVal.tv_usec ); */
	/* Create a value im ms (as clock does) */
	ulClockTicks = ( TimeVal.tv_sec * 1000 ) + ( TimeVal.tv_usec /1000 );
#else
	ulClockTicks = wpabs_get_systemticks();
#endif

	/* move to micro sec */
	ulClockTicks *= 1000;

	/* Did it wrap ? */
	if ( (UINT32)ulClockTicks < ulWallTimeUsLow )
	{
		/* Yes, so increment MSB */
		ulWallTimeUsHigh++;
	}

	f_pTime->aulWallTimeUs[ 0 ] = ulClockTicks;
	f_pTime->aulWallTimeUs[ 1 ] = ulWallTimeUsHigh;
	ulWallTimeUsLow = f_pTime->aulWallTimeUs[ 0 ];

	return cOCT6100_ERR_OK;
}


/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*\

Function:		Oct6100UserMemSet

Description:	Sets f_ulLength bytes pointed to by f_pAddress to f_ulPattern.

-------------------------------------------------------------------------------
|	Argument		|	Description
-------------------------------------------------------------------------------
 
\*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/
UINT32 Oct6100UserMemSet(
			IN	PVOID	f_pAddress,
			IN	UINT32	f_ulPattern,
			IN	UINT32	f_ulLength )
{

	memset( f_pAddress, f_ulPattern, f_ulLength );

	return cOCT6100_ERR_OK;
}


/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*\

Function:		Oct6100UserMemCopy

Description:	Copy f_ulLength bytes from f_pSource to f_pDestination.

-------------------------------------------------------------------------------
|	Argument		|	Description
-------------------------------------------------------------------------------
 
\*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/
UINT32 Oct6100UserMemCopy(
			IN	PVOID	f_pDestination,
			IN	PVOID	f_pSource,
			IN	UINT32	f_ulLength )
{

	memcpy( f_pDestination, f_pSource, f_ulLength );

	return cOCT6100_ERR_OK;
}


#if !defined(WAN_KERNEL)
static void SigArlmHandler()
{
}
#endif

#if defined(__GNU_LIBRARY__) && !defined(_SEM_SEMUN_UNDEFINED) || defined(__FreeBSD__) || defined(__OpenBSD__) || defined(WAN_KERNEL)
/* union semun is defined by including <sys/sem.h> */
#else
/* according to X/OPEN we have to define it ourselves */
union semun {
	int val;                  /* value for SETVAL */
	struct semid_ds *buf;     /* buffer for IPC_STAT, IPC_SET */
	unsigned short *array;    /* array for GETALL, SETALL */
                              /* Linux specific part: */
	struct seminfo *__buf;    /* buffer for IPC_INFO */
};
#endif

#if defined(WAN_KERNEL)

#else
typedef struct _SEM_FILE_INF_
{
	UINT32		ulMainProcessId;
	UINT32		ulUsageCount;

} tSEM_FILE_INF, *tPSEM_FILE_INF;

typedef struct _SEM_INF_
{
	INT			SemId;
	CHAR		szFileName[PATH_MAX];

} tSEM_INF, *tPSEM_INF;
#endif

/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*\

Function:		Oct6100UserCreateSerializeObject

Description:	Creates a serialization object. The serialization object is
				seized via the Oct6100UserSeizeSerializeObject function.

-------------------------------------------------------------------------------
|	Argument		|	Description
-------------------------------------------------------------------------------
IN OUT f_pCreate		Pointer to structure in which the serialization object's
				handle is returned.
 
\*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/
UINT32 Oct6100UserCreateSerializeObject(

		IN OUT tPOCT6100_CREATE_SERIALIZE_OBJECT	f_pCreate )
{

#if !defined(WAN_KERNEL)

	UINT32					ulRc = cOCT6100_ERR_OK;
	INT					SemId;
	FILE *					hSemFile = NULL;
	BOOL					fFileCreated = FALSE;
	key_t					Key;
	int					iSemCnt = 1;
	tPSEM_INF				pSemInf = NULL;
	union semun 				SemArg;
	tSEM_FILE_INF				SemFileInf;
	struct stat				FileStat;
	tPOCTPCIDRV_USER_PROCESS_CONTEXT	pContext;

	pContext = (tPOCTPCIDRV_USER_PROCESS_CONTEXT)f_pCreate->pProcessContext;

	if ( ( f_pCreate == NULL ) || ( f_pCreate->pszSerialObjName == NULL )  )
		return cOCT6100_CREATE_SERIAL_FAILED_0;

	/* Alloc a sem inf structure */
	pSemInf = (tPSEM_INF)malloc( sizeof(tSEM_INF) );

	/* Check if malloc failed!!! */
	if ( pSemInf == NULL )
		return cOCT6100_CREATE_SERIAL_FAILED_0; /* No memory. */

	snprintf( pSemInf->szFileName, PATH_MAX, "/tmp/%s", f_pCreate->pszSerialObjName );

	/* File exist? */
	if( stat( pSemInf->szFileName, &FileStat ) )
	{
		/* Not main process, this file must exist */
		if ( pContext->fMainProcess == FALSE )
		{
			/* The file does not exist, and we are not the main process. */
			ulRc = cOCT6100_CREATE_SERIAL_FAILED_1;
		}
		else
		{
			/* create the file */
			hSemFile = fopen( pSemInf->szFileName, "w+b" );
			if( !hSemFile )
			{
				ulRc = cOCT6100_CREATE_SERIAL_FAILED_2;
			}
			else
			{
				fFileCreated = TRUE;
				memset( &SemFileInf, 0x0, sizeof(SemFileInf) );
			}
		}
		SemFileInf.ulMainProcessId = getpid();
	}
	else
	{
		/* The semaphore exists.  Open it. */
		hSemFile = fopen( pSemInf->szFileName, "r+b" );
		/* Retrieve info from file */
		if ( sizeof(SemFileInf) != fread( &SemFileInf, 1, sizeof(SemFileInf), hSemFile ) )
		{
			ulRc = cOCT6100_CREATE_SERIAL_FAILED_2;
		}
		/* rewind file */
		rewind( hSemFile );
	}

	if ( cOCT6100_ERR_OK == ulRc )
	{
		/* This will create a key with upper bits set to 'K' ( 0x4b ) */
		Key = ftok( pSemInf->szFileName, 'K' );

		if ( -1 == Key )
		{
			ulRc = cOCT6100_CREATE_SERIAL_FAILED_2;
		}
	}

	if ( cOCT6100_ERR_OK == ulRc )
	{
		/* Get the semaphore or create it */
		SemId = semget( Key, iSemCnt, IPC_CREAT | 0666 /* | IPC_EXCL */ );

		if ( -1 == SemId )
		{
			ulRc = cOCT6100_CREATE_SERIAL_FAILED_3;
		}
	}

	if ( cOCT6100_ERR_OK == ulRc )
	{
		SemArg.val = 1;
		iSemCnt = 0;
		if ( 0 == semctl( SemId, iSemCnt, SETVAL, SemArg ))
		{
			/* Store handle and id */
			pSemInf->SemId = SemId;

			/* increment usage count */
			SemFileInf.ulUsageCount++;
			/* write file */
			if ( sizeof(SemFileInf) != fwrite( &SemFileInf, 1, sizeof(SemFileInf), hSemFile ) )
			{
				ulRc = cOCT6100_CREATE_SERIAL_FAILED_4;
			}
			else
			{
				/* commit write */
				fflush( hSemFile );
				fclose( hSemFile );
				signal( SIGALRM, SigArlmHandler );
				
				/* Keep pointer to semaphore information. */
				f_pCreate->ulSerialObjHndl = (UINT32)pSemInf;	
			}
		}
		else
		{
			ulRc = cOCT6100_CREATE_SERIAL_FAILED_4;
		}
	}
	
	/* Any errors ? */
	if ( cOCT6100_ERR_OK != ulRc )
	{
		if ( -1 != SemId )
		{
			/* remove semaphore */
			semctl( SemId, 0, IPC_RMID, SemArg );    
		}
		if ( NULL != hSemFile )
		{
			fclose( hSemFile );
		}

		if ( pSemInf )
		{
			if ( fFileCreated )
			{
				unlink( pSemInf->szFileName );
			}

			free( pSemInf );
		}
	}

	return ulRc;
#else

	wan_mutexlock_t				*pLockInf;
	tPOCTPCIDRV_USER_PROCESS_CONTEXT	pContext;

	pContext = (tPOCTPCIDRV_USER_PROCESS_CONTEXT)f_pCreate->pProcessContext;

	if ( ( f_pCreate == NULL ) || ( f_pCreate->pszSerialObjName == NULL )  )
		return cOCT6100_CREATE_SERIAL_FAILED_0;

	/* Alloc a sem inf structure */
	pLockInf = (wan_mutexlock_t*)wan_malloc( sizeof(wan_mutexlock_t) );

	/* Check if malloc failed!!! */
	if ( pLockInf == NULL )
		return cOCT6100_CREATE_SERIAL_FAILED_0; /* No memory. */

	wan_mutex_lock_init(pLockInf, "wan_ecapi_mutex");
	/* Keep pointer to semaphore information. */
	f_pCreate->ulSerialObjHndl = (PVOID)pLockInf;	

	return cOCT6100_ERR_OK;
#endif
}

/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*\

Function:		Oct6100UserDestroySerializeObject

Description:	Destroys the indicated serialization object.

-------------------------------------------------------------------------------
|	Argument		|	Description
-------------------------------------------------------------------------------
IN f_pDestroy			Pointer to structure containing the handle of the
				serialization object.
 
\*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/
UINT32 Oct6100UserDestroySerializeObject(

			IN tPOCT6100_DESTROY_SERIALIZE_OBJECT		f_pDestroy )
{	

#if !defined(WAN_KERNEL)
	UINT32		ulRc = cOCT6100_ERR_OK;
	tPSEM_INF	pSemInf;

	if ( ( f_pDestroy == NULL ) || ( f_pDestroy->ulSerialObjHndl == 0x0 ) )
		return cOCT6100_DESTROY_SERIAL_FAILED_0;

	pSemInf = (tPSEM_INF)(f_pDestroy->ulSerialObjHndl);

	/* Check mutex handle */
	if ( -1 != pSemInf->SemId )
	{
		FILE *			hSemFile = NULL;
		tSEM_FILE_INF	SemFileInf;

		/* Default is failure */
		ulRc = cOCT6100_DESTROY_SERIAL_FAILED_0;

		hSemFile = fopen( pSemInf->szFileName, "rb" );

		if ( hSemFile )
		{
			/* read sem file inf */
			if ( sizeof(SemFileInf) == fread( &SemFileInf, 1, sizeof(SemFileInf), hSemFile ) )
			{
				/* remove a link on that sem */
				if ( SemFileInf.ulUsageCount > 0 )
				{
					SemFileInf.ulUsageCount--;
				}

				/* Last reference gone ? */
				if( !SemFileInf.ulUsageCount )
				{
					union semun 	SemArg;

					/* Close semaphore, this must be called when no one is waiting on sem */
					if( semctl( pSemInf->SemId, 0, IPC_RMID, SemArg ) != -1 )
					{
						/* Close file */
						fclose( hSemFile );
						/* delete file */
						unlink( pSemInf->szFileName );
						/* release memory */
						free( pSemInf );

						/* return done */
						f_pDestroy->ulSerialObjHndl = 0x0;
						ulRc = cOCT6100_ERR_OK;
					}
				}
				else
				{
					/* commit change */
					rewind( hSemFile );
					fwrite( &SemFileInf, 1, sizeof(SemFileInf), hSemFile );
					/* commit write */
					fflush( hSemFile );
					fclose( hSemFile );
					free( pSemInf );
					/* If not in the main process, nothing much to do ... */
					f_pDestroy->ulSerialObjHndl = 0x0;
					ulRc = cOCT6100_ERR_OK;
				}
			}
		}
	}

	return ulRc;
#else

	wan_mutexlock_t		*pLockInf;

	if ( ( f_pDestroy == NULL ) || ( f_pDestroy->ulSerialObjHndl == 0x0 ) )
		return cOCT6100_DESTROY_SERIAL_FAILED_0;

	pLockInf = (wan_mutexlock_t*)(f_pDestroy->ulSerialObjHndl);

	if (wan_mutex_is_locked(pLockInf)){
		return cOCT6100_DESTROY_SERIAL_FAILED_0;
	}

	wan_free( pLockInf );
	return cOCT6100_ERR_OK;
#endif
}

/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*\

Function:		Oct6100UserSeizeSerializeObject

Description:	Seizes the indicated serialization object.

-------------------------------------------------------------------------------
|	Argument		|	Description
-------------------------------------------------------------------------------
IN f_pSeize			Pointer to structure containing the handle of the
				serialization object.
 
\*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/
UINT32 Oct6100UserSeizeSerializeObject(

			IN tPOCT6100_SEIZE_SERIALIZE_OBJECT			f_pSeize )
{

#if !defined(WAN_KERNEL)
	UINT32			ulRc = cOCT6100_ERR_OK;
	tPSEM_INF		pSemInf;

	if( f_pSeize == NULL )
		return cOCT6100_SEIZE_SERIAL_FAILED_0;

	if ( f_pSeize->ulSerialObjHndl == 0 )
		return cOCT6100_SEIZE_SERIAL_FAILED_0;

	pSemInf = (tPSEM_INF)f_pSeize->ulSerialObjHndl;

	/* Check mutex handle */
	if ( pSemInf )
	{
		INT			iRes = 0;
		struct itimerval	TimerSetting;

		ulRc = cOCT6100_SEIZE_SERIAL_FAILED_1;
		
		/* Check if must create a timer. */
		if( cOCT6100_WAIT_INFINITELY != f_pSeize->ulTryTimeMs )
		{
			/* Set the alarm */
			TimerSetting.it_value.tv_sec = ( f_pSeize->ulTryTimeMs / 1000 );
			TimerSetting.it_value.tv_usec = ( f_pSeize->ulTryTimeMs % 1000 ) * 1000;
			/* No repeat please! */
			TimerSetting.it_interval.tv_sec = 0;
			TimerSetting.it_interval.tv_usec = 0;

			iRes = setitimer( ITIMER_REAL, &TimerSetting, NULL );
		}
		
		if( iRes != -1 )
		{
			/* Timer is armed! */
			struct sembuf LockSEM[1] = { { 0, -1, SEM_UNDO } };

			iRes = semop( pSemInf->SemId, LockSEM, 1 );

			if( iRes == -1 )
			{
				switch( errno )
				{
					case EINTR:
						ulRc = cOCT6100_SEIZE_SERIAL_FAILED_2;
						break;
					default:
						break;
				}
			}
			else
			{
				ulRc = cOCT6100_ERR_OK;
			}
			
			if( cOCT6100_WAIT_INFINITELY != f_pSeize->ulTryTimeMs )
			{
				/* Disarm timer */
				TimerSetting.it_value.tv_sec = 0;
				TimerSetting.it_value.tv_usec = 0;

				setitimer( ITIMER_REAL, &TimerSetting, NULL );
			}
		}
	}
	return( ulRc );
#else
	wan_mutexlock_t		*pLockInf;

	if( f_pSeize == NULL )
		return cOCT6100_SEIZE_SERIAL_FAILED_0;

	if ( f_pSeize->ulSerialObjHndl == 0 )
		return cOCT6100_SEIZE_SERIAL_FAILED_0;

	pLockInf = (wan_mutexlock_t*)f_pSeize->ulSerialObjHndl;

	/* Check mutex handle */
	if ( pLockInf ){
		if (wan_mutex_trylock(pLockInf, NULL)){/* FIXME: NULL is ignored, but should be removed completely */
			return cOCT6100_ERR_OK;
		}
		return cOCT6100_SEIZE_SERIAL_FAILED_1;
	}
	return cOCT6100_ERR_OK;
#endif

}

/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*\

Function:		Oct6100UserReleaseSerializeObject

Description:	Releases the indicated serialization object.

-------------------------------------------------------------------------------
|	Argument		|	Description
-------------------------------------------------------------------------------
IN f_pRelease			Pointer to structure containing the handle of the
				serialization object.
 
\*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/
UINT32 Oct6100UserReleaseSerializeObject(

			IN tPOCT6100_RELEASE_SERIALIZE_OBJECT		f_pRelease )
{

#if !defined(WAN_KERNEL)
	UINT32		ulRc = cOCT6100_ERR_OK;
	tPSEM_INF	pSemInf;

	if( f_pRelease == NULL )
		return cOCT6100_RELEASE_SERIAL_FAILED_0;

	if ( f_pRelease->ulSerialObjHndl == 0 )
		return cOCT6100_RELEASE_SERIAL_FAILED_0;

	pSemInf = (tPSEM_INF)f_pRelease->ulSerialObjHndl;

	/* Check mutex handle */
	if ( pSemInf )
	{
		struct sembuf UnlockSEM[1] = { { 0, 1, SEM_UNDO } };
		int iRes;
		
		ulRc = cOCT6100_ERR_OK;

		iRes = semop( pSemInf->SemId, UnlockSEM, 1 );

		if( iRes == -1 )
		{
			ulRc = cOCT6100_RELEASE_SERIAL_FAILED_1;
		}
	}

	return( ulRc );
#else
	wan_mutexlock_t		*pLockInf;

	if( f_pRelease == NULL )
		return cOCT6100_RELEASE_SERIAL_FAILED_0;

	if ( f_pRelease->ulSerialObjHndl == 0 )
		return cOCT6100_RELEASE_SERIAL_FAILED_0;

	pLockInf = (wan_mutexlock_t*)f_pRelease->ulSerialObjHndl;

	/* Check mutex handle */
	if ( pLockInf ){
		wan_mutex_unlock(pLockInf, NULL);/* FIXME: no local 'flags' - NULL will cause the use of the internal 'flags' in the wan_mutexlock_t */
		return cOCT6100_ERR_OK;
	}
	return cOCT6100_ERR_OK;
#endif

}



/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*\

Function:		Oct6100UserDriverWriteApi

Description:    Performs a write access to the chip. This function is
		accessible only from the API code entity (i.e. not from the
		APIMI code entity).

-------------------------------------------------------------------------------
|	Argument		|	Description
-------------------------------------------------------------------------------
IN f_pWriteParams		Pointer to structure containing the Params to the
				write function.
 
\*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/
UINT32 Oct6100UserDriverWriteApi(
			IN	tPOCT6100_WRITE_PARAMS	f_pWriteParams )
{
	tPOCTPCIDRV_USER_PROCESS_CONTEXT	pContext;
	unsigned int				err;

	/*  The pProcessContext is there in case the user needs context information
	in order to perform the access. Note that if it is used, the memory pointed 
	by this pointer	must be allocated by the user beforehand. */
	pContext = (tPOCTPCIDRV_USER_PROCESS_CONTEXT)f_pWriteParams->pProcessContext;

	/* Check if driver context exists. In this implementation, the context */
	/* contains driver and semaphore information to perform I/O. */
	if ( pContext == NULL )
		return cOCT6100_DRIVER_WRITE_FAILED_0;

	err = wanec_req_write(
				pContext->ec_dev,
				f_pWriteParams->ulWriteAddress,
				f_pWriteParams->usWriteData);
	if (err){
		return cOCT6100_DRIVER_WRITE_FAILED_1;
	}
	return cOCT6100_ERR_OK;
}

/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*\

Function:		Oct6100UserDriverWriteOs

Description:    Performs a write access to the chip. This function is
		accessible only from the APIMI code entity (i.e. not from the
		API code entity).

-------------------------------------------------------------------------------
|	Argument		|	Description
-------------------------------------------------------------------------------
IN f_pWriteParams		Pointer to structure containing the Params to the
				write function.
 
\*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/
UINT32 Oct6100UserDriverWriteOs(
			IN	tPOCT6100_WRITE_PARAMS	f_pWriteParams )
{

	/* We assume that the interrupts are blocked during the 
	Oct6100UserDriverWriteApi calls and that the Oct6100UserDriverWriteApi 
	function is accessible in an interrupt context */

	return Oct6100UserDriverWriteApi( f_pWriteParams );
}


/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*\

Function:		Oct6100UserDriverWriteSmearApi

Description:    Performs a series of write accesses to the chip. The same data
		word is written to a series of addresses. The writes begin at
		the start address, and the address is incremented by the
		indicated amount for each subsequent write. This function is
		accessible only from the API code entity (i.e. not from the
		APIMI code entity).

-------------------------------------------------------------------------------
|	Argument		|	Description
-------------------------------------------------------------------------------
IN f_pSmearParams		Pointer to structure containing the Params to the
				write function.
 
\*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/
UINT32 Oct6100UserDriverWriteSmearApi(
			IN	tPOCT6100_WRITE_SMEAR_PARAMS	f_pSmearParams )
{

	tPOCTPCIDRV_USER_PROCESS_CONTEXT	pContext;
	unsigned int				err;

	/*  The pProcessContext is there in case the user needs context information
	in order to perform the access. Note that if it is used, the memory pointed 
	by this pointer	must be allocated by the user beforehand. */
	pContext = (tPOCTPCIDRV_USER_PROCESS_CONTEXT)f_pSmearParams->pProcessContext;

	/* Check if driver context exists. In this implementation, the context */
	/* contains driver and semaphore information to perform I/O. */
	if ( pContext == NULL )
		return cOCT6100_DRIVER_WSMEAR_FAILED_0;

	err = wanec_req_write_smear(
				pContext->ec_dev,
				f_pSmearParams->ulWriteAddress,
				f_pSmearParams->usWriteData,
				f_pSmearParams->ulWriteLength);
	if (err){
		return cOCT6100_DRIVER_WSMEAR_FAILED_1;
	}

	return cOCT6100_ERR_OK;
}

/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*\

Function:		Oct6100UserDriverWriteSmearOs

Description:    Performs a series of write accesses to the chip. The same data
		word is written to a series of addresses. The writes begin at
		the start address, and the address is incremented by the
		indicated amount for each subsequent write. This function is
		accessible only from the APIMI code entity (i.e. not from the
		API code entity).

-------------------------------------------------------------------------------
|	Argument		|	Description
-------------------------------------------------------------------------------
IN f_pSmearParams		Pointer to structure containing the Params to the
				write function.
 
\*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/
UINT32 Oct6100UserDriverWriteSmearOs(

			IN	tPOCT6100_WRITE_SMEAR_PARAMS			f_pSmearParams )
{
	/* We assume that the interrupts are blocked during the 
	Oct6100UserDriverWriteApi calls and that the Oct6100UserDriverWriteSmearApi 
	function is accessible in an interrupt context */

	return Oct6100UserDriverWriteSmearApi( f_pSmearParams );
}


/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*\

Function:		Oct6100UserDriverWriteBurstApi

Description:    Performs a series of write accesses to the chip. An array of
		data words is written to a series of consecutive addresses.
		The writes begin at the start address with element 0 of the
		provided array as the data word. The address is incremented by
		two for each subsequent write. This function is accessible only
		from the API code entity (i.e. not from the APIMI code entity).

-------------------------------------------------------------------------------
|	Argument		|	Description
-------------------------------------------------------------------------------
IN f_pSmearParams		Pointer to structure containing the Params to the
				write function.
 
\*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/
UINT32 Oct6100UserDriverWriteBurstApi(
			IN	tPOCT6100_WRITE_BURST_PARAMS	f_pBurstParams )
{

	tPOCTPCIDRV_USER_PROCESS_CONTEXT	pContext;
	unsigned int	i, err;
	unsigned short	*data;

	/*  The pProcessContext is there in case the user needs context information
	in order to perform the access. Note that if it is used, the memory pointed 
	by this pointer	must be allocated by the user beforehand. */
	pContext = (tPOCTPCIDRV_USER_PROCESS_CONTEXT)f_pBurstParams->pProcessContext;

	/* Check if driver context exists. In this implementation, the context */
	/* contains driver and semaphore information to perform I/O. */
	if ( pContext == NULL )
		return cOCT6100_DRIVER_WBURST_FAILED_0;

	/* In this case, the message is allocated dynamically since we do not know */
	/* in advance the size of the array to be passed down to the driver. */
#if defined(WAN_KERNEL)
	data = wan_malloc(f_pBurstParams->ulWriteLength * sizeof(unsigned short));
#else
	data = malloc(f_pBurstParams->ulWriteLength * sizeof(unsigned short));
#endif
	if (data == NULL){
		return cOCT6100_DRIVER_WBURST_FAILED_1;
	}

	/* Copy payload to be transported to driver. */
	for(i = 0; i < f_pBurstParams->ulWriteLength; i++ )
		data[i] = f_pBurstParams->pusWriteData[i];

	err = wanec_req_write_burst(
				pContext->ec_dev,
				f_pBurstParams->ulWriteAddress,
				data,
				f_pBurstParams->ulWriteLength);
	/* Cleanup allocated memory. */
#if defined(WAN_KERNEL)
	wan_free( data );
#else
	free( data );
#endif

	if (err){
		return cOCT6100_DRIVER_WBURST_FAILED_2;
	}

	return cOCT6100_ERR_OK;
}

/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*\

Function:		Oct6100UserDriverWriteBurstOs

Description:    Performs a series of write accesses to the chip. An array of
		data words is written to a series of consecutive addresses.
		The writes begin at the start address with element 0 of the
		provided array as the data word. The address is incremented by
		two for each subsequent write. This function is accessible only
		from the API code entity (i.e. not from the APIMI code entity).

-------------------------------------------------------------------------------
|	Argument		|	Description
-------------------------------------------------------------------------------
IN f_pBurstParams		Pointer to structure containing the Params to the
				write function.
 
\*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/
UINT32 Oct6100UserDriverWriteBurstOs(
			IN	tPOCT6100_WRITE_BURST_PARAMS	f_pBurstParams )
{

	/* We assume that the interrupts are blocked during the 
	Oct6100UserDriverWriteApi calls and that the Oct6100UserDriverWriteBurstApi 
	function is accessible in an interrupt context */
	
	return Oct6100UserDriverWriteBurstApi( f_pBurstParams );
}




/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*\

Function:		Oct6100UserDriverReadApi

Description:    Performs a read access to the chip. This function is accessible
		only from the API code entity (i.e. not from the APIMI code
		entity).

-------------------------------------------------------------------------------
|	Argument		|	Description
-------------------------------------------------------------------------------
IN OUT f_pReadParams		Pointer to structure containing the Params to the
				read function.
 
\*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/
UINT32 Oct6100UserDriverReadApi(
			IN OUT	tPOCT6100_READ_PARAMS	f_pReadParams )
{
	tPOCTPCIDRV_USER_PROCESS_CONTEXT	pContext;
	unsigned int				err;

	/*  The pProcessContext is there in case the user needs context information
	in order to perform the access. Note that if it is used, the memory pointed 
	by this pointer	must be allocated by the user beforehand. */
	
	pContext = (tPOCTPCIDRV_USER_PROCESS_CONTEXT)f_pReadParams->pProcessContext;

	/* Check if driver context exists. In this implementation, the context */
	/* contains driver and semaphore information to perform I/O. */
	if ( pContext == NULL )
		return cOCT6100_DRIVER_READ_FAILED_0;

	err = wanec_req_read(
				pContext->ec_dev,
				f_pReadParams->ulReadAddress,
				f_pReadParams->pusReadData);
	if (err){
		return cOCT6100_DRIVER_READ_FAILED_1;
	}

	return cOCT6100_ERR_OK;
}

/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*\

Function:		Oct6100UserDriverReadOs

Description:    Performs a read access to the chip. This function is accessible
		only from the APIMI code entity (i.e. not from the API code
		entity).

-------------------------------------------------------------------------------
|	Argument		|	Description
-------------------------------------------------------------------------------
IN OUT f_pReadParams		Pointer to structure containing the Params to the
				read function.
 
\*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/
UINT32 Oct6100UserDriverReadOs(
			IN OUT	tPOCT6100_READ_PARAMS	f_pReadParams )
{

	/* We assume that the interrupts are blocked during the 
	Oct6100UserDriverReadApi calls and that the Oct6100UserDriverReadApi 
	function is accessible in an interrupt context */
	return Oct6100UserDriverReadApi( f_pReadParams );
}


/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*\

Function:		Oct6100UserDriverReadBurstApi

Description:    Performs a burst of read accesses to the chip. The first read
		is performed at the start address, and the address is
		incremented by two for each subsequent read. The data is
		retunred in an array provided by the user. This function is
		accessible only from the API code entity (i.e. not from the
		APIMI code entity).

-------------------------------------------------------------------------------
|	Argument		|	Description
-------------------------------------------------------------------------------
IN OUT f_pBurstParams		Pointer to structure containing the Params to the
				read function.
 
\*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/
UINT32 Oct6100UserDriverReadBurstApi(
			IN OUT	tPOCT6100_READ_BURST_PARAMS	f_pBurstParams )
{
	tPOCTPCIDRV_USER_PROCESS_CONTEXT	pContext;
	unsigned int	i, err;
	unsigned short	*data;

	/*  The pProcessContext is there in case the user needs context information
	in order to perform the access. Note that if it is used, the memory pointed 
	by this pointer	must be allocated by the user beforehand. */
	pContext = (tPOCTPCIDRV_USER_PROCESS_CONTEXT)f_pBurstParams->pProcessContext;

	/* Check if driver context exists. In this implementation, the context */
	/* contains driver and semaphore information to perform I/O. */
	if ( pContext == NULL )
		return cOCT6100_DRIVER_RBURST_FAILED_0;

	/* In this case, the message is allocated dynamically since we do not know */
	/* in advance the size of the array to be passed down to the driver. */
#if defined(WAN_KERNEL)
	data = (unsigned short*)wan_malloc(f_pBurstParams->ulReadLength * sizeof(unsigned short));
#else
	data = (unsigned short*)malloc(f_pBurstParams->ulReadLength * sizeof(unsigned short));
#endif
	if (data == NULL){
		return cOCT6100_DRIVER_RBURST_FAILED_1;
	}
	err = wanec_req_read_burst(
				pContext->ec_dev,
				f_pBurstParams->ulReadAddress,
				data,
				f_pBurstParams->ulReadLength);	

	/* Copy result to the user. */
	if (!err){
		for(i = 0; i < f_pBurstParams->ulReadLength; i++){
			f_pBurstParams->pusReadData[i] = data[i];
		}
	}
	
	/* Cleanup allocated memory. */
#if defined(WAN_KERNEL)
	wan_free(data);
#else
	free(data);
#endif

	if (err){
		return cOCT6100_DRIVER_RBURST_FAILED_2;
	}

	return cOCT6100_ERR_OK;
}

/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*\

Function:		Oct6100UserDriverReadBurstOs

Description:    Performs a burst of read accesses to the chip. The first read
		is performed at the start address, and the address is
		incremented by two for each subsequent read. The data is
		retunred in an array provided by the user. This function is
		accessible only from the APIMI code entity (i.e. not from the
		API code entity).

-------------------------------------------------------------------------------
|	Argument		|	Description
-------------------------------------------------------------------------------
IN OUT f_pBurstParams		Pointer to structure containing the Params to the
				read function.
 
\*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/
UINT32 Oct6100UserDriverReadBurstOs(
			IN OUT	tPOCT6100_READ_BURST_PARAMS	f_pBurstParams )
{
	/* We assume that the interrupts are blocked during the 
	Oct6100UserDriverReadApi calls and that the  
	Oct6100UserDriverReadBurstApi function is accessible 
	in an interrupt context */
	return Oct6100UserDriverReadBurstApi(f_pBurstParams);
}

