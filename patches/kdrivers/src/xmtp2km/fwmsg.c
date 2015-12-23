/****************************************************************************
 *
 * fwmsg.c : Fatal and Warning messages
 *
 * These functions are common to all protocol levels.
 *
 * Copyright (C) 2004  Xygnada Technology, Inc.
 *
 *		This program is free software; you can redistribute it and/or
 *		modify it under the terms of the GNU General Public License
 *		as published by the Free Software Foundation; either version
 *		2 of the License, or (at your option) any later version.
 ****************************************************************************/
#include <linux/kernel.h>
#include <linux/string.h>

#define I_FORMAT_S	"xmtp2km:I:%s:%s:%s:%s\n" 
#define I_FORMAT_2S	"xmtp2km:I:%s:%s:%s:%s:%s\n" 
#define I_FORMAT_2S_U	"xmtp2km:I:%s:%s:%s:%s:%s:%u\n" 
#define I_FORMAT_U	"xmtp2km:I:%s:%s:%s:%u\n"
#define I_FORMAT_2U	"xmtp2km:I:%s:%s:%s:%u:%u\n"
#define I_FORMAT_3U	"xmtp2km:I:%s:%s:%s:%u:%u:%u\n"
#define I_FORMAT_4U	"xmtp2km:I:%s:%s:%s:%u:%u:%u:%u\n"
#define I_FORMAT_6U	"xmtp2km:I:%s:%s:%s:%u:%u:%u:%u:%u:%u\n"

#define W_FORMAT_S	"xmtp2km:W:%s:%s:%s:%s\n" 
#define W_FORMAT_2S	"xmtp2km:W:%s:%s:%s:%s:%s\n" 
#define W_FORMAT_2S_U	"xmtp2km:W:%s:%s:%s:%s:%s:%u\n" 
#define W_FORMAT_3S	"xmtp2km:W:%s:%s:%s:%s:%s:%s\n" 
#define W_FORMAT_U	"xmtp2km:W:%s:%s:%s:%u\n"
#define W_FORMAT_2U	"xmtp2km:W:%s:%s:%s:%u:%u\n"
#define W_FORMAT_3U	"xmtp2km:W:%s:%s:%s:%u:%u:%u\n"
#define W_FORMAT_4U	"xmtp2km:W:%s:%s:%s:%u:%u:%u:%u\n"
#define W_FORMAT_6U	"xmtp2km:W:%s:%s:%s:%u:%u:%u:%u:%u:%u\n"

#define F_FORMAT_S	"xmtp2km:F:%s:%s:%s:%s\n" 
#define F_FORMAT_2S	"xmtp2km:F:%s:%s:%s:%s:%s\n" 
#define F_FORMAT_2S_U	"xmtp2km:F:%s:%s:%s:%s:%s:%u\n" 
#define F_FORMAT_2S_2U	"xmtp2km:F:%s:%s:%s:%s:%s:%u:%u\n" 
#define F_FORMAT_3S	"xmtp2km:F:%s:%s:%s:%s:%s:%s\n" 
#define F_FORMAT_U	"xmtp2km:F:%s:%s:%s:%u\n"
#define F_FORMAT_2U	"xmtp2km:F:%s:%s:%s:%u:%u\n"
#define F_FORMAT_3U	"xmtp2km:F:%s:%s:%s:%u:%u:%u\n"
#define F_FORMAT_4U	"xmtp2km:F:%s:%s:%s:%u:%u:%u:%u\n"
#define F_FORMAT_6U	"xmtp2km:F:%s:%s:%s:%u:%u:%u:%u:%u:%u\n"

char pline[1024];

int xmtp2km_fatal_error = 0;
int verbose_mode 	= -1;
int debug_mode 		= 0;

extern int		bpool_rdy;

void default_prg_end (void);
void (*pf_prgend)(void) = default_prg_end;

void fwmsg_init (
		const char * ps_prgname, 
		const int _print_syslog_, 
		const int _verbose_mode_,
		const int _debug_mode_,
		void (*_pf_prgend_)(void))
/******************************************************************************/
{
	verbose_mode = _verbose_mode_;
	debug_mode = _debug_mode_;
	if (_pf_prgend_ != NULL) pf_prgend = _pf_prgend_;
}

void stop_execution (void)
/******************************************************************************/
{
	xmtp2km_fatal_error = -1;	
	bpool_rdy = 0;
}

void default_prg_end (void)
/******************************************************************************/
{
	return;
}

void print_msg (void)
/******************************************************************************/
{
	printk ("%s", pline);
}

void	info_s (
		const char * object,
		const char * location, 
		const char * cause, 
		const char * detail
		)
/******************************************************************************/
{
	if (! verbose_mode) return;

	sprintf (pline,
		I_FORMAT_S,
		object,
		location,
		cause,
		detail
	);
	print_msg ();
}

void	info_2s (
		const char * object,
		const char * location, 
		const char * cause, 
		const char * detail1,
		const char * detail2
		)
/******************************************************************************/
{
	if (! verbose_mode) return;

	sprintf (pline,
		I_FORMAT_2S,
		object,
		location,
		cause,
		detail1,
		detail2
	);
	print_msg ();
}

void	info_2s_u (
		const char * object,
		const char * location, 
		const char * cause, 
		const char * detail1,
		const char * detail2,
		unsigned int detail3
		)
/******************************************************************************/
{
	if (! verbose_mode) return;

	sprintf (pline,
		I_FORMAT_2S_U,
		object,
		location,
		cause,
		detail1,
		detail2,
		detail3
	);
	print_msg ();
}

void	info_u (
		const char * object,
		const char * location, 
		const char * cause, 
		const unsigned int detail
		)
/******************************************************************************/
{
	if (! verbose_mode) return;

	sprintf (pline,
		I_FORMAT_U,
		object,
		location,
		cause,
		detail
	);
	print_msg ();
}

void	info_2u (
		const char * object,
		const char * location, 
		const char * cause, 
		const unsigned int detail1,
		const unsigned int detail2
		)
/******************************************************************************/
{
	if (! verbose_mode) return;

	sprintf (pline,
		I_FORMAT_2U,
		object,
		location,
		cause,
		detail1,
		detail2
	);
	print_msg ();
}

void	info_3u (
		const char * object,
		const char * location, 
		const char * cause, 
		const unsigned int detail1,
		const unsigned int detail2,
		const unsigned int detail3
		)
/******************************************************************************/
{
	if (! verbose_mode) return;

	sprintf (pline,
		I_FORMAT_3U,
		object,
		location,
		cause,
		detail1,
		detail2,
		detail3
	);
	print_msg ();
}

void	info_4u (
		const char * object,
		const char * location, 
		const char * cause, 
		const unsigned int detail1,
		const unsigned int detail2,
		const unsigned int detail3,
		const unsigned int detail4
		)
/******************************************************************************/
{
	if (! verbose_mode) return;

	sprintf (pline,
		I_FORMAT_4U,
		object,
		location,
		cause,
		detail1,
		detail2,
		detail3,
		detail4
	);
	print_msg ();
}

void	report_6u (
		const char * object,
		const char * location, 
		const char * cause, 
		const unsigned int detail1,
		const unsigned int detail2,
		const unsigned int detail3,
		const unsigned int detail4,
		const unsigned int detail5,
		const unsigned int detail6
		)
/******************************************************************************/
{
	//if (! verbose_mode) return;

	sprintf (pline,
		I_FORMAT_6U,
		object,
		location,
		cause,
		detail1,
		detail2,
		detail3,
		detail4,
		detail5,
		detail6
	);
	print_msg ();
}

void	warning_s (
		const char * object,
		const char * location, 
		const char * cause, 
		const char * detail
		)
/******************************************************************************/
{
	sprintf (pline,
		W_FORMAT_S,
		object,
		location,
		cause,
		detail
	);
	print_msg ();
}

void	warning_2s (
		const char * object,
		const char * location, 
		const char * cause, 
		const char * detail1,
		const char * detail2
		)
/******************************************************************************/
{
	sprintf (pline,
		W_FORMAT_2S,
		object,
		location,
		cause,
		detail1,
		detail2
	);
	print_msg ();
}

void	warning_2s_u (
		const char * object,
		const char * location, 
		const char * cause, 
		const char * detail1,
		const char * detail2,
		unsigned int detail3
		)
/******************************************************************************/
{
	sprintf (pline,
		W_FORMAT_2S_U,
		object,
		location,
		cause,
		detail1,
		detail2,
		detail3
	);
	print_msg ();
}

void	warning_3s (
		const char * object,
		const char * location, 
		const char * cause, 
		const char * detail1,
		const char * detail2,
		const char * detail3
		)
/******************************************************************************/
{
	sprintf (pline,
		W_FORMAT_3S,
		object,
		location,
		cause,
		detail1,
		detail2,
		detail3
	);
	print_msg ();
}

void	warning_u (
		const char * object,
		const char * location, 
		const char * cause, 
		const unsigned int detail
		)
/******************************************************************************/
{
	sprintf (pline,
		W_FORMAT_U,
		object,
		location,
		cause,
		detail
	);
	print_msg ();
}

void	warning_2u (
		const char * object,
		const char * location, 
		const char * cause, 
		const unsigned int detail1,
		const unsigned int detail2
		)
/******************************************************************************/
{
	sprintf (pline,
		W_FORMAT_2U,
		object,
		location,
		cause,
		detail1,
		detail2
	);
	print_msg ();
}

void	warning_3u (
		const char * object,
		const char * location, 
		const char * cause, 
		const unsigned int detail1,
		const unsigned int detail2,
		const unsigned int detail3
		)
/******************************************************************************/
{
	sprintf (pline,
		W_FORMAT_3U,
		object,
		location,
		cause,
		detail1,
		detail2,
		detail3
	);
	print_msg ();
}

void	warning_4u (
		const char * object,
		const char * location, 
		const char * cause, 
		const unsigned int detail1,
		const unsigned int detail2,
		const unsigned int detail3,
		const unsigned int detail4
		)
/******************************************************************************/
{
	sprintf (pline,
		W_FORMAT_4U,
		object,
		location,
		cause,
		detail1,
		detail2,
		detail3,
		detail4
	);
	print_msg ();
}

void	fatal_s (
		const char * object,
		const char * location, 
		const char * cause, 
		const char * detail
		)
/******************************************************************************/
{
	sprintf (pline,
		F_FORMAT_S,
		object,
		location,
		cause,
		detail
	);
	print_msg ();
	(*pf_prgend) ();
	stop_execution ();
}

void	fatal_2s (
		const char * object,
		const char * location, 
		const char * cause, 
		const char * detail1,
		const char * detail2
		)
/******************************************************************************/
{
	sprintf (pline,
		F_FORMAT_2S,
		object,
		location,
		cause,
		detail1,
		detail2
	);
	print_msg ();
	(*pf_prgend) ();
	stop_execution ();
}

void	fatal_3s (
		const char * object,
		const char * location, 
		const char * cause, 
		const char * detail1,
		const char * detail2,
		const char * detail3
		)
/******************************************************************************/
{
	sprintf (pline,
		F_FORMAT_3S,
		object,
		location,
		cause,
		detail1,
		detail2,
		detail3
	);
	print_msg ();
	(*pf_prgend) ();
	stop_execution ();
}

void	fatal_u (
		const char * object,
		const char * location, 
		const char * cause, 
		const unsigned int detail
		)
/******************************************************************************/
{
	sprintf (pline,
		F_FORMAT_U,
		object,
		location,
		cause,
		detail
	);
	print_msg ();
	(*pf_prgend) ();
	stop_execution ();
}

void	fatal_2u (
		const char * object,
		const char * location, 
		const char * cause, 
		const unsigned int detail1,
		const unsigned int detail2
		)
/******************************************************************************/
{
	sprintf (pline,
		F_FORMAT_2U,
		object,
		location,
		cause,
		detail1,
		detail2
	);
	print_msg ();
	(*pf_prgend) ();
	stop_execution ();
}

void	fatal_3u (
		const char * object,
		const char * location, 
		const char * cause, 
		const unsigned int detail1,
		const unsigned int detail2,
		const unsigned int detail3
		)
/******************************************************************************/
{
	sprintf (pline,
		F_FORMAT_3U,
		object,
		location,
		cause,
		detail1,
		detail2,
		detail3
	);
	print_msg ();
	(*pf_prgend) ();
	stop_execution ();
}

void	fatal_4u (
		const char * object,
		const char * location, 
		const char * cause, 
		const unsigned int detail1,
		const unsigned int detail2,
		const unsigned int detail3,
		const unsigned int detail4
		)
/******************************************************************************/
{
	sprintf (pline,
		F_FORMAT_4U,
		object,
		location,
		cause,
		detail1,
		detail2,
		detail3,
		detail4
	);
	print_msg ();
	(*pf_prgend) ();
	stop_execution ();
}

void	fatal_2s_2u (
		const char * object,
		const char * location, 
		const char * cause, 
		const char * detail1, 
		const char * detail2, 
		const unsigned int detail3,
		const unsigned int detail4
		)
/******************************************************************************/
{
	sprintf (pline,
		F_FORMAT_2S_2U,
		object,
		location,
		cause,
		detail1,
		detail2,
		detail3,
		detail4
	);
	print_msg ();
	(*pf_prgend) ();
	stop_execution ();
}

void debug_init (const int _debug_mode_)
/******************************************************************************/
{
	debug_mode = _debug_mode_;
}

void	mtp2_debug_u (
		const char * object,
		const char * location, 
		const char * cause, 
		const unsigned int detail
		)
/******************************************************************************/
{
	if (! debug_mode) return;

	printk (
"D:MTP2:%s:%s:%s:%u\n", 
		object,
		location,
		cause,
		detail
	);
}

void dbg_state_cause (
//		t_mtp2_link * p_lnk, 
		void * p_lnk,
		const unsigned int called, 
		const unsigned int cause, 
		const unsigned int caller)
/******************************************************************************/
{
	if (! debug_mode) return;
	
#if 0
	printk (
"D:MTP2: %s:%s:states/cause:\
\nfac %u slot %u ls %u l %u:\
\ncalled %u:\
\ncaller %u:\
\ncause %u:\
\nLSC %u:\
\nIAC %u:\
\nRXC %u:\
\nTXC %u:\
\nDAEDR %u:\
\nDAEDT %u:\
\nSUERM %u:\
\nAERM %u:\n",
		__FILE__,
		__FUNCTION__,
		p_lnk->facility_index,
		p_lnk->slot,
		p_lnk->ls,
		p_lnk->link,
		called,
		caller,
		cause,
		p_lnk->state_lsc,
		p_lnk->state_iac,
		p_lnk->state_rxc,
		p_lnk->state_txc,
		p_lnk->state_daedr,
		p_lnk->state_daedt,
		p_lnk->state_suerm,
		p_lnk->state_aerm
	       );
#endif
}

