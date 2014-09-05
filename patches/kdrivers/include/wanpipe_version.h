/*****************************************************************************
* wanpipe_version.h	WANPIPE(tm) Sangoma Device Driver.
******************************************************************************/

#ifndef __WANPIPE_VERSION__
#define __WANPIPE_VERSION__


#define WANPIPE_COPYRIGHT_DATES "(c) 1994-2013"
#define WANPIPE_COMPANY         "Sangoma Technologies Inc"

/********** LINUX **********/
#define WANPIPE_VERSION			"7.0.10"
#define WANPIPE_SUB_VERSION				"0"
#define WANPIPE_LITE_VERSION			"1.1.1"

#if defined(__LINUX__)
#define WANPIPE_VERSION_MAJOR			7
#define WANPIPE_VERSION_MINOR			0
#define WANPIPE_VERSION_MINOR1			10
#define WANPIPE_VERSION_MINOR2			0
#endif

/********** FreeBSD **********/
#define WANPIPE_VERSION_FreeBSD			"3.2"
#define WANPIPE_SUB_VERSION_FreeBSD		"2"
#define WANPIPE_VERSION_BETA_FreeBSD	1
#define WANPIPE_LITE_VERSION_FreeBSD	"1.1.1"

/********** OpenBSD **********/
#define WANPIPE_VERSION_OpenBSD			"1.6.5"
#define WANPIPE_SUB_VERSION_OpenBSD		"8"
#define WANPIPE_VERSION_BETA_OpenBSD	1
#define WANPIPE_LITE_VERSION_OpenBSD	"1.1.1"

/********** NetBSD **********/
#define WANPIPE_VERSION_NetBSD			"1.1.1"
#define WANPIPE_SUB_VERSION_NetBSD		"5"
#define WANPIPE_VERSION_BETA_NetBSD		1

#if defined(__WINDOWS__)

# define WP_BUILD_NBE41_HPBOA 0

# define	WANPIPE_VERSION_MAJOR	7
# define	WANPIPE_VERSION_MINOR	0

# if WP_BUILD_NBE41_HPBOA
#  define	WANPIPE_VERSION_MINOR1	0
#  define	WANPIPE_VERSION_MINOR2	0
# else
#  define	WANPIPE_VERSION_MINOR1	0
#  define	WANPIPE_VERSION_MINOR2	0
# endif

# undef	VER_PRODUCTVERSION
# undef VER_PRODUCTVERSION_STR
# undef VER_PRODUCTNAME_STR
# undef VER_COMPANYNAME_STR

# if WP_BUILD_NBE41_HPBOA
#  define VER_PRODUCTVERSION	7,0,0,0
#  define VER_PRODUCTVERSION_STR	"7.0.0.0"
# else
#  define VER_PRODUCTVERSION	7,0,0,0
#  define VER_PRODUCTVERSION_STR	"7.0.0.0"
# endif

# define __BUILDDATE__				Dec 24, 2012

# define VER_COMPANYNAME_STR		"Sangoma Technologies Corporation"
# define VER_LEGALCOPYRIGHT_YEARS	"1984-2012"
# define VER_LEGALCOPYRIGHT_STR		"Copyright (c) Sangoma Technologies Corp."
# define VER_PRODUCTNAME_STR		"Sangoma WANPIPE (TM)"

# undef WANPIPE_VERSION
# undef WANPIPE_VERSION_BETA
# undef WANPIPE_SUB_VERSION

# if WP_BUILD_NBE41_HPBOA
#  define WANPIPE_VERSION_Windows	"7.0.0"
#  define WANPIPE_SUB_VERSION_Windows	"0"
# else
#  define WANPIPE_VERSION_Windows	"7.0.0"
#  define WANPIPE_SUB_VERSION_Windows	"0"
# endif

# define WANPIPE_VERSION_BETA_Windows	0

# define WANPIPE_VERSION		WANPIPE_VERSION_Windows
# define WANPIPE_VERSION_BETA	WANPIPE_VERSION_BETA_Windows
# define WANPIPE_SUB_VERSION	WANPIPE_SUB_VERSION_Windows
#endif /* __WINDOWS__ */

#endif /* __WANPIPE_VERSION__ */

