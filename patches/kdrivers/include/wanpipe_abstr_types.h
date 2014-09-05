/************************************************************************
* wanpipe_abstr_types.h			WANPIPE(tm)								*
*																		*
*	Wanpipe Kernel Abstraction type definitions	 						*
*																		*
*																		*
* Authors:	Alex Feldman <al.feldman@sangoma.com>						*
			David Rokhvarg <davidr@sangoma.com>							*
*=======================================================================*
*																		*
* Nov 02, 2009	David Rokhvarg											*
*				Moved definitions for Sangoma MS Windows driver from	*
*				wanpipe_ctypes.h to this file. The wanpipe_ctypes.h is	*
*				not removed, but it will include this file for backward	*
*				compatibility with existing source code.				*
*																		*
* Jan 24, 2008	Alex Feldman											*
*				Initial version			 								*
*************************************************************************/

#ifndef __WANPIPE_ABSTR_TYPES_H
# define __WANPIPE_ABSTR_TYPES_H



#if defined(__FreeBSD__)
/******************* F R E E B S D ******************************/
typedef int					wan_ticks_t; 
typedef unsigned long		ulong_ptr_t;
typedef long				long_t;	
typedef unsigned long 		ulong_t;	
#define wan_timeval			timeval
#define wan_timeval_t		struct timeval
#elif defined(__OpenBSD__)
/******************* O P E N B S D ******************************/
typedef int					wan_ticks_t; 
typedef unsigned long		ulong_ptr_t;
typedef long				long_t;	
typedef unsigned long 		ulong_t;	
#define wan_timeval			timeval
#define wan_timeval_t		struct timeval
#elif defined(__NetBSD__)
/******************* N E T B S D ******************************/
typedef int					wan_ticks_t; 
typedef unsigned long		ulong_ptr_t;
typedef long				long_t ; 
typedef unsigned long 		ulong_t;	
#define wan_timeval			timeval
#define wan_timeval_t		struct timeval
#elif defined(__LINUX__)
/*********************** L I N U X ******************************/
typedef unsigned long		wan_ticks_t; 
typedef unsigned long		ulong_ptr_t;
typedef unsigned long		wan_time_t; 
typedef unsigned long		wan_suseconds_t; 
typedef long				long_t;	
typedef unsigned long 		ulong_t;
#define wan_timeval			timeval
#define wan_timeval_t		struct timeval
typedef unsigned int 		wan_bitmap_t; /* 32 bit-wide on both 32 and 64 bit systems */
#elif defined(__WINDOWS__)
/******************* W I N D O W S ******************************/

/************* Basic data types **********/

/* signed types */
#define int8_t		INT8		
#define int16_t		INT16
#define int32_t		INT32		
#define int64_t		INT64

typedef int8_t		int8;
typedef int16_t		int16;
typedef int32_t		int32;
typedef int64_t		int64;

/* unsigned types */
#define u_int8_t	UINT8
#define u_int16_t	UINT16
#define u_int32_t	UINT32
#define u_int64_t	UINT64

typedef unsigned int	wan_ticks_t; 
typedef __time64_t		wan_time_t; 
typedef unsigned int	wan_suseconds_t; 

struct wan_timeval
{ 
	wan_time_t		tv_sec;
	wan_suseconds_t	tv_usec;
};

typedef	struct wan_timeval wan_timeval_t;

#if defined(WAN_KERNEL)
 typedef LONG		long_t;
 typedef ULONG		ulong_t;

# if defined(_WIN64)
   typedef unsigned __int64	ulong_ptr_t;
# else
   typedef unsigned __int32	ulong_ptr_t;
# endif

# define timeval wan_timeval/* kernel-mode only - will NOT conflict with winsock.h */

# else/* (WAN_KERNEL) */
   typedef long				long_t;
   typedef unsigned long	ulong_t;
#endif 

typedef ulong_t		u_long;

typedef u_int8_t	uint8_t;
typedef u_int16_t	uint16_t;
typedef u_int32_t	uint32_t;
typedef u_int64_t	uint64_t;

typedef uint8_t		u8;
typedef uint16_t	u16;
typedef uint32_t	u32;
typedef uint64_t	u64;

typedef uint8_t		__u8;
typedef uint16_t	__u16;
typedef uint32_t	__u32;
typedef uint64_t	__u64;

typedef unsigned int uint;
typedef unsigned short ushort;

#if defined(WAN_KERNEL)
typedef u_int8_t		u_char;
typedef unsigned int	UINT,	*PUINT;

/* AFT DMA addresses are MAXIMUM 32-bit wide!!!
 * This is why this declaration is safe: */
typedef u32		dma_addr_t;
/*typedef PHYSICAL_ADDRESS	dma_addr_t;*/
/*typedef LONGLONG		dma_addr_t;*/
#endif

typedef char*		caddr_t;
typedef struct { long_t counter; } atomic_t;	/* Interlocked functions require LONG parameter */
typedef ulong_t		wan_bitmap_t;	/* atomic bit-manupulation require LONG. 
									 * 32 bit-wide on both 32 and 64 bit systems */

/******** End of Basic data types ********/

typedef u32	gfp_t;

#if !defined(WAN_KERNEL)
#if defined(_WIN64)
 #define DWL_MSGRESULT	DWLP_MSGRESULT
 #define DWL_USER		DWLP_USER
 #define DLGPROC_RETURN_TYPE INT_PTR
#else
 #define DLGPROC_RETURN_TYPE BOOL
 #endif
#endif/* !defined(WAN_KERNEL) */

#endif /* __WINDOWS__ */

#if defined(__WINDOWS__)
/*
 *   We support 32 bit applications running on 64 bit machines.
 * This requires pointers used by WAN_COPY_FROM_USER() and
 * WAN_COPY_TO_USER() to be 64 bit on both platforms.
 * The WP_POINTER_64 will be used for that.
 *
 *   Do NOT use "POINTER_64" - some versions of MS compiler 
 * will expand it incorrectly to __ptr32.
 */
# define WP_POINTER_64	__ptr64
#else
# define WP_POINTER_64
#endif

#endif	/* __WANPIPE_ABSTR_TYPES_H */
