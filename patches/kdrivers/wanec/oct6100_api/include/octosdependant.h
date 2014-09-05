/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*\

File: octosdependant.h

    Copyright (c) 2001-2008 Octasic Inc.

Description:

	This file is included to set target-specific constants.

This file is part of the Octasic OCT6100 GPL API . The OCT6100 GPL API  is 
free software; you can redistribute it and/or modify it under the terms of 
the GNU General Public License as published by the Free Software Foundation; 
either version 2 of the License, or (at your option) any later version.

The OCT6100 GPL API is distributed in the hope that it will be useful, but 
WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY 
or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License 
for more details. 

You should have received a copy of the GNU General Public License 
along with the OCT6100 GPL API; if not, write to the Free Software 
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA.

$Octasic_Release: OCT612xAPI-01.01.01 $

$Octasic_Revision: 20 $

\*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/

#ifndef __OCTOSDEPENDANT_H__
#define __OCTOSDEPENDANT_H__


/*--------------------------------------------------------------------------
	C language
----------------------------------------------------------------------------*/

#ifdef __cplusplus
extern "C" {
#endif



/*****************************************************************************

  Known define values

	MSDEV:
			WIN32		==	WINDOWS 32 bit app
			__WIN32__	==	WINDOWS 32 bit app
			_Windows	==	WINDOWS 16 bit app

			_WINDOWS	==	Windows application .. not console
			_DLL		==	Dll Application
			_CONSOLE	==	Console Application .. no windows

	BORLANDC
			__TURBOC__		== Turbo Compiler
			__BORLANDC__	== Borland compiler
			__OS2__			== Borland OS2 compiler
			_Windows		== Windows 16 bit app

	GCC Compiler
			__GNUC__		== GCC Compiler
			__unix__		== Unix system
			__vax__			== Unix system
			unix			== Unix system
			vax				== vax system

	TORNADO
			_VXWORKS_		==	VXWORK

	ECOS/CYGWIN
			_ECOS_			== eCos

  	SOLARIS
			_SOLARIS_		== Solaris

*****************************************************************************/

/* Machine endian type */

#define OCT_MACH_LITTLE_ENDIAN		1
#define OCT_MACH_BIG_ENDIAN			2


/* Try to find current OCT_MACH_ENDIAN from compiler define values */
#if !defined( MACH_TYPE_BIG_ENDIAN ) && !defined( MACH_TYPE_LITTLE_ENDIAN )
	/* Does GNU defines the endian ? */
	#if defined(__GNU_C__)
		#if defined(_BIG_ENDIAN) || defined(__BIG_ENDIAN__)
			#define OCT_MACH_ENDIAN		OCT_MACH_BIG_ENDIAN
		#elif defined(_LITTLE_ENDIAN) || defined(__LITTLE_ENDIAN__)
			#define OCT_MACH_ENDIAN		OCT_MACH_LITTLE_ENDIAN
		#endif
	#endif

	/* Try with cpu type */
	#if !defined(OCT_MACH_ENDIAN)
		/* Look for intel */
		/* Sangoma for AMD x64:
		 * Visual Studio 2008 defines _M_X64 (but not _WIN64 !)
		 * Windows Driver Kit (WDK) defines AMD64	*/
		#if defined( _M_IX86 ) || defined( __i386__ ) || defined(__x86_64__) || defined(_M_X64) || defined(AMD64)
			#define OCT_MACH_ENDIAN		OCT_MACH_LITTLE_ENDIAN
		/* Look for PowerPC */
		#elif defined( _M_MPPC  ) || defined( _M_PPC ) || defined(PPC) || defined(__PPC) || defined(_ARCH_PPC)
			#define OCT_MACH_ENDIAN		OCT_MACH_BIG_ENDIAN
		#elif defined(__arm__)
			#define OCT_MACH_ENDIAN		OCT_MACH_LITTLE_ENDIAN
		#elif defined( CPU )
			#if CPU==PPC860
				#define OCT_MACH_ENDIAN		OCT_MACH_BIG_ENDIAN
			#endif
		#endif
	#endif
#else
	#if defined( MACH_TYPE_BIG_ENDIAN )
		#define OCT_MACH_ENDIAN		OCT_MACH_BIG_ENDIAN
	#else
		#define OCT_MACH_ENDIAN		OCT_MACH_LITTLE_ENDIAN
	#endif
#endif

/* Have we reached a conclusion ? */
#if OCT_MACH_ENDIAN != OCT_MACH_LITTLE_ENDIAN && OCT_MACH_ENDIAN != OCT_MACH_BIG_ENDIAN
#error Endanness cannot be detected; Please specify in Makefile.pre
#endif

/* Find system type if not already defined */
#if !defined( OCT_NTDRVENV ) && !defined( OCT_VXENV ) && !defined( OCT_WINENV )

#if defined( WIN32 ) || defined( __WIN32__ ) ||	defined( _WIN32_ ) || defined( WIN32S )
	/* Verif if building a win32 driver */
	/* #if ( defined( WIN32 ) && WIN32==100 ) */	/* Sangoma: "WIN32==100" produces error C1017 on Visual Studio 2008,
													 * but no such problem when compiler is WDK. */
	#if ( defined( WIN32 ) && defined(__KERNEL__) ) /* __KERNEL__ is defined for Sangoma Windows Device Driver */
		#define OCT_NTDRVENV
	#else
		#define OCT_WINENV
	#endif
#elif defined( _VXWORKS_ )
	#define OCT_VXENV
#elif defined( _ECOS_ )
#ifndef OCT_ECOSENV
	#define OCT_ECOSENV
#endif /* OCT_ECOSENV */
#elif defined( _SOLARIS_ )
	#define OCT_SOLARISENV
#elif defined( _LINUX_ )
	#define OCT_LINUXENV
#else
	/* Unknown environment */
	#define OCT_UNKNOWNENV
#endif	/* WIN env */

#endif /* Already defined */

#if defined( __KERNEL__ ) && defined( OCT_LINUXENV )
#define OCT_LINUXDRVENV
#endif

#ifdef _DEBUG
#define OCT_OPT_USER_DEBUG
#endif

/*--------------------------------------------------------------------------
	C language
----------------------------------------------------------------------------*/

#ifdef __cplusplus
}
#endif

#endif	/* __OCTOSDEPENDANT_H__ */
