#ifndef __WAN_AFTUP_H
# define __WAN_AFTUP_H

#define MAX_IFNAME_LEN	40
#define MAX_HWINFO_LEN	100

#include "wanpipe_api.h"
#include "wan_aft_prg.h"

#if defined(__WINDOWS__)
#include "wanpipe_time.h"	/* usleep() */
#else
#define u32 	unsigned int
#define u8 	unsigned char
#define u16	unsigned short
#endif

#define s16	signed short


typedef struct wan_aftup_ {

	char		if_name[MAX_IFNAME_LEN];
	int		board_id;
	int		chip_id;
	unsigned char	core_rev;
	unsigned char	core_id;
	int		revision_id;
	char	flash_rev;
	char	flash_new;	
#if 0
	char	firmware[100];
	char	prefix_fw[100];
#endif
	char		hwinfo[MAX_HWINFO_LEN];

	wan_aft_cpld_t	cpld;

	WAN_LIST_ENTRY(wan_aftup_) next;
} wan_aftup_t;

#endif	/* __WAN_AFTUP_H */
