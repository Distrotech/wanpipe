/*****************************************************************************
* sdladump.c	WANPIPE(tm) Adapter Memeory Dump Utility.
*
* Author:	Gene Kozin	<genek@compuserve.com>
*
* Copyright:	(c) 1995-1996 Sangoma Technologies Inc.
*
*		This program is free software; you can redistribute it and/or
*		modify it under the terms of the GNU General Public License
*		as published by the Free Software Foundation; either version
*		2 of the License, or (at your option) any later version.
* ----------------------------------------------------------------------------
* Nov 29, 1996	Gene Kozin	Initial version based on WANPIPE configurator.
*****************************************************************************/

/*****************************************************************************
* Usage:
*   sdladump [{switches}] {device} [{offset} [{length}]]
*
* Where:
*   {device}	WANPIPE device name in /proc/net/wanrouter directory
*   {offset}	address of adapter's on-board memory. Default is 0.
*   {length}	size of memory to dump. Default is 0x100 (256 bytes).
*   {switches}	one of the following:
*			-v	verbose mode
*			-h | -?	show help
*****************************************************************************/

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <fcntl.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#if defined(CONFIG_PRODUCT_WANPIPE_GENERIC)
# include <sys/socket.h>
# include <netinet/in.h>
# if defined(__NetBSD__) || defined(__FreeBSD__) || defined(__OpenBSD__)
#  include <net/if.h>
# else
#  include <linux/if.h>
#  include <linux/if_wanpipe.h>
# endif
#endif

#if defined(__LINUX__)
# include <linux/wanpipe_defines.h>
# include <linux/wanpipe_cfg.h>
# include <linux/wanpipe.h>	/* WANPIPE user API definitions */
#else
# include <wanpipe_defines.h>
# include <wanpipe_cfg.h>
# include <wanpipe.h>	/* WANPIPE user API definitions */
#endif

/****** Defines *************************************************************/

#ifndef	min
#define	min(a,b)	(((a)<(b))?(a):(b))
#endif

/* Error exit codes */
enum ErrCodes
{
	ERR_SYSTEM = 1,		/* System error */
	ERR_SYNTAX,		/* Command line syntax error */
	ERR_LIMIT
};

/* Command line parsing stuff */
#define ARG_SWITCH	'-'	/* switch follows */
#define	SWITCH_GETHELP	'h'	/* show help screen */
#define	SWITCH_ALTHELP	'?'	/* same */
#define	SWITCH_VERBOSE	'v'	/* enable verbose output */
#define	SWITCH_DEBUG	'd'	/* read debug messages */

/* Defaults */
#define	DEFAULT_OFFSET	0
#define	DEFAULT_LENGTH	0x100

/* FreeBSD/OpenBSD */
#define WANDEV_NAME "/dev/wanrouter"

/****** Data Types **********************************************************/

/****** Function Prototypes *************************************************/

int arg_proc	(int argc, char* argv[]);
int do_dump	(int argc, char* argv[]);
int do_debug(int argc, char *argv[]);
void show_dump	(char* buf, unsigned long len, unsigned long addr);
void show_error	(int err);
int hexdump	(char* str, int, char* data, int length, int limit);

extern	int close (int);

/****** Global Data *********************************************************/

/*
 * Strings & message tables.
 */
char progname[] = "sdladump";
char wandev_dir[] = "/proc/net/wanrouter";	/* location of WAN devices */
#if defined(__NetBSD__) || defined(__FreeBSD__) || defined(__OpenBSD__)
wan_conf_t config;
#endif

char banner[] =
	"WANPIPE Memory Dump Utility. v3.0.0 "
	"(c) 1995-1996 Sangoma Technologies Inc."
;
char helptext[] =
	"\n"
	"sdladump: Wanpipe Memory Dump Utility\n\n"
#if defined(CONFIG_PRODUCT_WANPIPE_GENERIC)
	"Usage:\tsdladump [{switches}] {ifname} [{offset} [{length}]]\n"
#else
	"Usage:\tsdladump [{switches}] {device} [{offset} [{length}]]\n"
#endif
	"\nWhere:"
#if defined(CONFIG_PRODUCT_WANPIPE_GENERIC)
	"\t{ifname}\tWANPIPE (LITE) interface name (hdlcN, where N=0,1..)\n"
#else
	"\t{device}\tWANPIPE device name from /proc/net/wanrouter directory\n"
#endif
	"\t{offset}\taddress of adapter's on-board memory (default is 0)\n"
	"\t{length}\tsize of memory to dump (default is 256 bytes)\n"
	"\t{switches}\tone of the following:\n"
	"\t\t\t-v\tverbose mode\n"
	"\t\t\t-h|?\tshow this help\n"
;

char* err_messages[] =		/* Error messages */
{
	"Invalid command line syntax",	/* ERR_SYNTAX */
	"Unknown error code",		/* ERR_LIMIT */
};

enum				/* execution modes */
{
	DO_DUMP,
	DO_DEBUG,
	DO_HELP
} action;
int verbose;			/* verbosity level */

/****** Entry Point *********************************************************/

int main (int argc, char *argv[])
{
	int err = 0;	/* return code */
	int skip;

	/* Process command line switches */
	for (skip = 0, --argc, ++argv;
	     argc && (**argv == ARG_SWITCH);
	     argc -= skip, argv += skip)
	{
		skip = arg_proc(argc, argv);
		if (skip == 0)
		{
			err = ERR_SYNTAX;
			show_error(err);
			break;
		}
	}

	/* Perform requested action */
	if (verbose) puts(banner);
	if (!err) switch (action)
	{
	case DO_DUMP:
		err = do_dump(argc, argv);
		break;

	case DO_DEBUG:
		err = do_debug(argc, argv);
		break;

	default:
		err = ERR_SYNTAX;
	}
	if (err == ERR_SYNTAX) puts(helptext);
	return err;
}

/*============================================================================
 * Process command line.
 *	Return number of successfully processed arguments, or 0 in case of
 *	syntax error.
 */
int arg_proc (int argc, char *argv[])
{
	int cnt = 0;

	switch (argv[0][1])
	{
	case SWITCH_GETHELP:
	case SWITCH_ALTHELP:
		action = DO_HELP;
		cnt = 1;
		break;

	case SWITCH_DEBUG:
		action = DO_DEBUG;
		cnt = 1;
		break;

	case SWITCH_VERBOSE:
	  	verbose = 1;
		cnt = 1;
		break;
	}
	return cnt;
}

/*============================================================================
 * Dump adapter memory.
 *	argv[0]: device name
 *	argv[1]: offset
 *	argv[2]: length
 */
int do_dump (int argc, char *argv[])
{
	int err = 0;
	int dev;
#if defined(CONFIG_PRODUCT_WANPIPE_GENERIC)
	struct ifreq	req;
#else
	int	max_flen = sizeof(wandev_dir) + WAN_DRVNAME_SZ;
	char	filename[max_flen + 2];
#endif
	sdla_dump_t dump;

	if (!argc || (strlen(argv[0]) > WAN_DRVNAME_SZ))
	{
		show_error(ERR_SYNTAX);
		return ERR_SYNTAX;
	}

	dump.magic  = WANPIPE_MAGIC;
	dump.offset = (argc > 1) ? strtoul(argv[1], NULL, 0) : DEFAULT_OFFSET;
	dump.length = (argc > 2) ? strtoul(argv[2], NULL, 0) : DEFAULT_LENGTH;
	if (!dump.length)
	{
		show_error(ERR_SYNTAX);
		return ERR_SYNTAX;
	}

	if (verbose) printf(
		"Dumping %lu bytes from offset %lu from device %s ...\n",
		dump.length, dump.offset, argv[0])
	;

	dump.ptr = malloc(dump.length);
	if (dump.ptr == NULL)
	{
		show_error(ERR_SYSTEM);
		return ERR_SYSTEM;
	}

#if defined(CONFIG_PRODUCT_WANPIPE_GENERIC)
	if ((dev = socket(PF_INET, SOCK_DGRAM, IPPROTO_IP)) < 0){
		err = ERR_SYSTEM;
		show_error(err);
		goto do_dump_done;
	}
	strcpy(req.ifr_name, argv[0]);
	req.ifr_data = (void*)&dump;
	err = ioctl(dev, SIOC_WANPIPE_DUMP, &req);
#else
# if defined(__LINUX__)
	snprintf(filename, max_flen, "%s/%s", wandev_dir, argv[0]);
# else
	snprintf(filename, max_flen, "%s", WANDEV_NAME);
# endif
	if ((dev = open(filename, O_RDONLY)) < 0){
		err = ERR_SYSTEM;
		show_error(err);
		goto do_dump_done;
	}
# if defined(__LINUX__)
	err = ioctl(dev, WANPIPE_DUMP, &dump);
# else
  	strlcpy(config.devname, argv[0], WAN_DRVNAME_SZ);
 	config.arg = &dump;
	err = ioctl(dev, WANPIPE_DUMP, &config);
# endif
#endif
	if (err < 0){
		err = ERR_SYSTEM;
		show_error(err);
	}
	else show_dump(dump.ptr, dump.length, dump.offset);
do_dump_done:
	if (dev >= 0) close(dev);
	free(dump.ptr);
	return err;
}

/*============================================================================
 * Print hex memory dump to standard output.
 *	argv[0]: device name
 *	argv[1]: offset
 *	argv[2]: length
 */
void show_dump (char* buf, unsigned long len, unsigned long addr)
{
	char str[80];
	int cnt;

	memset(str, 0, 80);
	if (len > 16)
	{
		/* see if ajustment is needed and adjust to 16-byte
		 * boundary, if necessary.
		 */ 
		cnt = 16 - addr % 16;
		if (cnt)
		{
			printf("%05lX: ", addr);
			hexdump(str, 80, buf, cnt, 16);
			puts(str);
		}
	}
	else cnt = 0;

	while (cnt < len)
	{
		memset(str, 0, 80);
		printf("%05lX: ", addr + cnt);
		cnt += hexdump(str, 80, &buf[cnt], min(16, len - cnt), 16);
		puts(str);
	}
}

/*============================================================================
 * Dump data into a string in the following format:
 *	XX XX XX XX XX XX XX XX XX XX XX XX XX XX XX XX AAAAAAAAAAAAAAAA
 *
 * Return number of bytes dumped.
 * NOTE: string buffer must be at least (limit * 4 + 2) bytes long.
 */
int hexdump (char* str, int max_len, char* data, int length, int limit)
{
	int i, n, off = 0;

	n = min(limit, length);
	if (n)
	{
		for (i = 0; i < n; ++i){
			off += snprintf(&str[off], max_len-off, "%02X ", data[i]);
		}
		for (i = 0; i < limit - n; ++i){
			off += snprintf(&str[off], max_len-off, "   ");
		}
		off += snprintf(&str[off], max_len-off, " ");
		for (i = 0; i < n; ++i){
			off += snprintf(&str[off], max_len-off, 
				"%c", (isprint(data[i])) ? data[i] : '.');
		}
		for (i = 0; i < limit - n; ++i){
			off += snprintf(&str[off], max_len-off, " ");
		}
	}
	*str = '\0';
	return n;
}


/*============================================================================
 * Get debuggin messages.
 */
int do_debug(int argc, char *argv[])
{
    	int				dev;
	wan_kernel_msg_t		wan_debug;
	wanpipe_kernel_msg_hdr_t*	dbg_msg;
	struct tm*			time_tm;
	char				buf[200], tmp_time[50];
	int 				offset = 0, len = 0, is_more = 1,
					max_flen = sizeof(wandev_dir) + WAN_DRVNAME_SZ;
	char				filename[max_flen + 2];

	if (!argc || (strlen(argv[0]) > WAN_DRVNAME_SZ))
	{
		show_error(ERR_SYNTAX);
		return ERR_SYNTAX;
	}

#if defined(__LINUX__)
	snprintf(filename, max_flen, "%s/%s", wandev_dir, argv[0]);
#else
	snprintf(filename, max_flen, "%s", WANDEV_NAME);
#endif
	
        dev = open(filename, O_RDONLY); 
        if (dev == -1){
		fprintf(stderr, "\n\n\tFAILED open device %s!\n", 
					filename);
		show_error(ERR_SYSTEM);
		return ERR_SYNTAX;	
	}
 	/* Clear configuration structure */
	is_more = 1;
	while(is_more){
		wan_debug.magic = ROUTER_MAGIC;
		wan_debug.len = 0;
		wan_debug.is_more = 0;
		wan_debug.max_len = 1024;
		wan_debug.data	= malloc(1024);
		if (wan_debug.data == NULL){
			show_error(ERR_SYSTEM);
			if (dev >= 0) close(dev);
			return ERR_SYSTEM;
		}

#if defined(__NetBSD__) || defined(__FreeBSD__) || defined(__OpenBSD__)
	  	strlcpy(config.devname, argv[0], WAN_DRVNAME_SZ);
    		config.arg = &wan_debug;
		if (ioctl(dev, ROUTER_DEBUG_READ, &config) < 0){ 
#else
		if (ioctl(dev, ROUTER_DEBUG_READ, &wan_debug)){
#endif
			fprintf(stderr, "\n\n\tROUTER DEBUG READ failed!!\n");
			show_error(ERR_SYSTEM);
			if (dev >= 0) close(dev);
			return ERR_SYSTEM;
	        }
		is_more = wan_debug.is_more;
		len = wan_debug.len;
		offset = 0;
		while(offset < len){
			dbg_msg = (wanpipe_kernel_msg_hdr_t*)&wan_debug.data[offset];
			offset += sizeof(wanpipe_kernel_msg_hdr_t);
			if (dbg_msg->len == 0) continue;
			memcpy(buf, &wan_debug.data[offset], dbg_msg->len); 
			offset += dbg_msg->len;
			buf[dbg_msg->len] = '\0';
			time_tm = localtime((time_t*)&dbg_msg->time);
			strftime(tmp_time, sizeof(tmp_time),"%a",time_tm);
			printf("%s ",tmp_time);
			strftime(tmp_time, sizeof(tmp_time),"%b",time_tm);
			printf("%s ",tmp_time);
			strftime(tmp_time, sizeof(tmp_time),"%d",time_tm);
			printf("%s ",tmp_time);
			strftime(tmp_time, sizeof(tmp_time),"%H",time_tm);
			printf("%s:",tmp_time);
			strftime(tmp_time, sizeof(tmp_time),"%M",time_tm);
			printf("%s:",tmp_time);
			strftime(tmp_time, sizeof(tmp_time),"%S",time_tm);
			printf("%s ",tmp_time);
			printf("%s", buf);
		}
	}
	if (dev >= 0){ 
		close(dev);
       	}
	return 0;
}



/*============================================================================
 * Show error message.
 */
void show_error (int err)
{
	if (err == ERR_SYSTEM) fprintf(stderr, "%s: SYSTEM ERROR %d: %s!\n",
		progname, errno, strerror(errno))
	;
	else fprintf(stderr, "%s: %s!\n", progname,
		err_messages[min(err, ERR_LIMIT) - 2])
	;
}
/****** End *****************************************************************/
