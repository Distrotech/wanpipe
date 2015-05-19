/*
 * Written by Gabor Egressy 
 * gabor@vmunix.com
 * Original    : January 1995
 * Last update : November 1997
 *
 * The functions contained herein have been tested on FreeBSD and LINUX.
 * The operating system used must be POSIX compliant UNIX to utilize
 * getkey() and kbdhit(). you might need to include some of the if lines
 * on some compilers with broken selects and headers. eg. on hp-ux you
 * might need to change #if 0 to #if 1
 *
 * all of the functions in this file return the constant IOERROR when an
 * error occurs; make sure to check for this return value.
 * make sure you include unixio.h in necessary headers
 *
 * Make sure to link in termcap/terminfo library
 * You might need to change the line
 * #include <termcap.h> to #include <terminfo.h> in the unixio.c file
 */

#include "unixio.h"

#include <stdlib.h>
#include <ctype.h>
#include <termcap.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>

#if 0
typedef int fd_set;
#endif

#if 0
struct timeval {
    long tv_sec;
    long tv_usec;
};
#endif

#if 0
#define _INCLUDE_POSIX_SOURCE
#endif

#include <unistd.h>

#include <signal.h>
#include <termios.h>

static enum {g_RAW, g_CBREAK, g_RESET} tty_status = g_RESET;
static int no_block = 0;

/* 
 * signal handling functions; these are not interface functions -
 * termSetup() is - do not call these functions
 */
void saveMode(int);
void resetMode(int);
void dieOnSignal(int);
void cleanup(void);

static int timeout(void);

#define ESC 0X1B

/*
 * kbdhit :
 * return non-zero if a key on the keyboard was hit; 0 otherwise
 * the key that was typed will be stored in input iff it is a valid character
 * otherwise -1 is stored in input: valid characters are those that fit
 * into one char; if input is null nothing is stored in input
 * might be a good idea to turn off echo when using this function; you can 
 * turn echo back on when you need to
 * IN  : pointer to variable to store input
 * OUT : non-zero if keyboard was hit, 0 if not; IOERROR on error
 */
int
kbdhit(int *input)
{
	unsigned char ch;
	int n, retval;
	struct termios oattr, attr;


    if(tty_status == g_RESET) {
        if((tcgetattr)(STDIN_FILENO,&oattr) < 0)
            return IOERROR;
        attr = oattr;
        attr.c_lflag &= ~(ICANON | ECHO);
        attr.c_cc[VMIN] = 1;
        attr.c_cc[VTIME] = 0;
        if((tcsetattr)(STDIN_FILENO,TCSAFLUSH,&attr) < 0)
            return IOERROR;
    } else if(!(isatty)(STDIN_FILENO))
        return IOERROR;

    no_block = 1;
	if((n = timeout()) < 0)
		return IOERROR;
    retval = n;
	if(retval) { 
		if((read)(STDIN_FILENO,&ch,1) != 1)
			return IOERROR;
        if(input)
            *input = ch;
		if(ch == ESC) {
			if((n = timeout()) < 0)
				return IOERROR;
			if(n) {
				do {
					if((read)(STDIN_FILENO,&ch,1) != 1)
						return IOERROR;
					if((n = timeout()) < 0)
						return IOERROR;
				} while(n);
				if(input)
					*input = (-1);
			}
		}
	}
    if(tty_status == g_RESET)
        if((tcsetattr)(STDIN_FILENO,TCSAFLUSH,&oattr) < 0)
            return IOERROR;

	return retval;
}

static int
timeout(void)
{
	struct timeval tv;
	fd_set readfds;
	int n;


    if(!(isatty)(STDIN_FILENO))
        return IOERROR;

	FD_ZERO(&readfds);
	FD_SET(STDIN_FILENO,&readfds);

	tv.tv_sec = TIMEOUT_SEC;
	tv.tv_usec = TIMEOUT_USEC + no_block;

	if((n = (select)(1,&readfds,(fd_set*)NULL,(fd_set*)NULL,&tv)) < 0)
		return IOERROR;

	return n;
}
