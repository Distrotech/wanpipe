/****************************************************************************
 *
 * fwmsg.h : Fatal and Warning messages
 * 
 * These function prototypes are common to all protocol levels.
 * 
 * Copyright (C) 2004  Xygnada Technology, Inc.
 * 
 *		This program is free software; you can redistribute it and/or
 *		modify it under the terms of the GNU General Public License
 *		as published by the Free Software Foundation; either version
 *		2 of the License, or (at your option) any later version.
****************************************************************************/
//#include <string.h>
//#include <errno.h>

#ifdef __cplusplus
extern "C" {
#endif

/* warning and fatal action prototypes */

void fwmsg_init (
		const char * ps_prgname, 
		const int _print_syslog_, 
		const int _verbose_mode_,
		const int _debug_mode_,
		void (*pf_prgend)(void));

void	info_s (
		const char * object,
		const char * location, 
		const char * cause, 
		const char * detail
		);
void	info_2s (
		const char * object,
		const char * location, 
		const char * cause, 
		const char * detail1,
		const char * detail2
		);
void	info_2s_u (
		const char * object,
		const char * location, 
		const char * cause, 
		const char * detail1,
		const char * detail2,
		unsigned int detail3
		);
void	info_u (
		const char * object,
		const char * location, 
		const char * cause, 
		const unsigned int detail
		);
void	info_2u (
		const char * object,
		const char * location, 
		const char * cause, 
		const unsigned int detail1,
		const unsigned int detail2
		);
void	info_3u (
		const char * object,
		const char * location, 
		const char * cause, 
		const unsigned int detail1,
		const unsigned int detail2,
		const unsigned int detail3
		);
void	info_4u (
		const char * object,
		const char * location, 
		const char * cause, 
		const unsigned int detail1,
		const unsigned int detail2,
		const unsigned int detail3,
		const unsigned int detail4
		);
void	warning_s (
		const char * object,
		const char * location, 
		const char * cause, 
		const char * detail
		);
void	warning_2s (
		const char * object,
		const char * location, 
		const char * cause, 
		const char * detail1,
		const char * detail2
		);
void	warning_3s (
		const char * object,
		const char * location, 
		const char * cause, 
		const char * detail1,
		const char * detail2,
		const char * detail3
		);
void	warning_u (
		const char * object,
		const char * location, 
		const char * cause, 
		const unsigned int detail
		);
void	warning_2u (
		const char * object,
		const char * location, 
		const char * cause, 
		const unsigned int detail1,
		const unsigned int detail2
		);
void	warning_2s_u (
		const char * object,
		const char * location, 
		const char * cause, 
		const char * detail1,
		const char * detail2,
		unsigned int detail3
		);
void	warning_3u (
		const char * object,
		const char * location, 
		const char * cause, 
		const unsigned int detail1,
		const unsigned int detail2,
		const unsigned int detail3
		);
void	warning_4u (
		const char * object,
		const char * location, 
		const char * cause, 
		const unsigned int detail1,
		const unsigned int detail2,
		const unsigned int detail3,
		const unsigned int detail4
		);
void	fatal_s (
		const char * object,
		const char * location, 
		const char * cause, 
		const char * detail
		);
void	fatal_2s (
		const char * object,
		const char * location, 
		const char * cause, 
		const char * detail1,
		const char * detail2
		);
void	fatal_3s (
		const char * object,
		const char * location, 
		const char * cause, 
		const char * detail1,
		const char * detail2,
		const char * detail3
		);
void	fatal_u (
		const char * object,
		const char * location, 
		const char * cause, 
		const unsigned int detail
		);
void	fatal_2u (
		const char * object,
		const char * location, 
		const char * cause, 
		const unsigned int detail1,
		const unsigned int detail2
		);
void	fatal_3u (
		const char * object,
		const char * location, 
		const char * cause, 
		const unsigned int detail1,
		const unsigned int detail2,
		const unsigned int detail3
		);
void	fatal_4u (
		const char * object,
		const char * location, 
		const char * cause, 
		const unsigned int detail1,
		const unsigned int detail2,
		const unsigned int detail3,
		const unsigned int detail4
		);
void	fatal_2s_2u (
		const char * object,
		const char * location, 
		const char * cause, 
		const char * detail1, 
		const char * detail2, 
		const unsigned int detail3,
		const unsigned int detail4
		);

#ifdef __cplusplus
}
#endif
