/*****************************************************************************
* wanproc.h	Definitions for the WAN PROC fs.
*
* Author: 	Alex Feldman <al.feldman@sangoma.com>
*
* Copyright:	(c) 1995-2000 Sangoma Technologies Inc.
*
*		This program is free software; you can redistribute it and/or
*		modify it under the terms of the GNU General Public License
*		as published by the Free Software Foundation; either version
*		2 of the License, or (at your option) any later version.
* ============================================================================
* Aug 20, 2001  Alex Feldman	Initial version.
*/

#if !defined(__WANPROC_H) && !defined(__WINDOWS__)
# define __WANPROC_H

# if defined(__NetBSD__) || defined(__FreeBSD__) || defined(__OpenBSD__)
enum {
	WANPIPE_PROCFS_CONFIG	= 0x1,
	WANPIPE_PROCFS_HWPROBE,
	WANPIPE_PROCFS_HWPROBE_LEGACY,
	WANPIPE_PROCFS_HWPROBE_VERBOSE,
	WANPIPE_PROCFS_STATUS,
	WANPIPE_PROCFS_INTERFACES,
	WANPIPE_PROCFS_DEV
};
# endif

# if defined(WAN_KERNEL)

# define PROC_STATS_ALARM_FORMAT "%25s: %10s %25s: %10s\n"
# define PROC_STATS_PMON_FORMAT "%25s: %10u %25s: %10u\n"

/*
 * ******************************************************************
 *
 *
 * ******************************************************************
 */
#define PROC_BUF_CONT	0
#define PROC_BUF_EXIT	1

#if defined(LINUX_2_6)
# define PROC_ADD_DECL(m)	
# define PROC_ADD_INIT(m,buf,offs,len)
# define PROC_ADD_LINE(m,frm,msg...)	seq_printf(m,frm,##msg)
# define PROC_ADD_RET(m)		return 0
# define PROC_GET_DATA()		m->private
#else
/* For BSDs and not Linux-2.6 */
# define PROC_GET_DATA()		(void*)start
# define PROC_ADD_DECL(m)		static struct seq_file mfile;	\
					struct seq_file *m=&mfile;
# define PROC_ADD_INIT(m,buf,offs,len)					\
		if (offs == 0) m->stop_cnt = 0;				\
		m->buf = buf;						\
		m->size = len;						\
		m->from = offs;						\
		m->count = 0;						\
		m->index = 0;			
# define PROC_ADD_LINE(m,frm,msg...)					\
	{ if (proc_add_line(m,frm,##msg) == PROC_BUF_EXIT)		\
		return m->count;					\
	}
# define PROC_ADD_RET(m)						\
			if (m->count >= 0) {				\
				m->stop_cnt = m->from + m->count;	\
			}						\
			return m->count;
# define PROC_ADD_PREFIX(m,frm,msg...)					\
	if (proc_add_prefix(m, frm, ##msg) == PROC_BUF_EXIT)		\
		return m->count;
#endif

/*******************************************************************
*		TYPEDEF/STRUCTURE
********************************************************************/

/*******************************************************************
*		FUNCTION PROTOTYPES
********************************************************************/
#if defined(__NetBSD__) || defined(__FreeBSD__) || defined(__OpenBSD__)
int config_get_info(char* buf, char** start, off_t offs, int len);
int status_get_info(char* buf, char** start, off_t offs, int len);
int probe_get_info(char* buf, char** start, off_t offs, int len);
int probe_get_info_legacy(char* buf, char** start, off_t offs, int len);
int probe_get_info_verbose(char* buf, char** start, off_t offs, int len);
int wandev_get_info(char* buf, char** start, off_t offs, int len);
int interfaces_get_info(char* buf, char** start, off_t offs, int len);
int wanproc_active_dev(char* buf, char** start, off_t offs, int len);
#endif
extern int proc_add_line(struct seq_file* m, char* frm, ...);



/*
 * ******************************************************************
 */

/*
 ********************************************************************
 *				X.25
 */
/*
 * SNMP X.25 defines
 */
#define SNMP_X25_DTE		1
#define SNMP_X25_DCE		2
#define SNMP_X25_DXE		3

#define SNMP_X25_MODULO8	1
#define SNMP_X25_MODULO128	2

#define SNMP_X25_INCOMING	1
#define SNMP_X25_OUTGOING	2
#define SNMP_X25_PVC		3

/* x25CallParmAcceptReverseCharging */ 
#define SNMP_X25_ARC_DEFAULT		1
#define SNMP_X25_ARC_ACCEPT		2
#define SNMP_X25_ARC_REFUSE		3
#define SNMP_X25_ARC_NEVERACCEPT	4

/* x25CallParmProposeReverseCharging */
#define SNMP_X25_PRC_DEFAULT		1
#define SNMP_X25_PRC_REVERSE		2
#define SNMP_X25_PRC_LOCAL		3

/* x25CallParmFastSelecet */
#define SNMP_X25_FS_DEFAULT		1
#define SNMP_X25_FS_NOTSPECIFIED	2
#define SNMP_X25_FS_FASTSELECT		3
#define SNMP_X25_FS_RESTRICTEDFASTRESP	4
#define SNMP_X25_FS_NOFASTSELECT	5
#define SNMP_X25_FS_NORESTRICTEDFASTSEL	6

/* x25CallParmInThruPutClassSize */
/* x25CallParmOutThruPutClassSize */
#define SNMP_X25_THRUCLASS_TCRES1	1
#define SNMP_X25_THRUCLASS_TCRES2	2
#define SNMP_X25_THRUCLASS_TC75		3
#define SNMP_X25_THRUCLASS_TC150	4
#define SNMP_X25_THRUCLASS_TC300	5
#define SNMP_X25_THRUCLASS_TC600	6
#define SNMP_X25_THRUCLASS_TC1200	7
#define SNMP_X25_THRUCLASS_TC2400	8
#define SNMP_X25_THRUCLASS_TC4800	9
#define SNMP_X25_THRUCLASS_TC9600	10
#define SNMP_X25_THRUCLASS_TC19200	11
#define SNMP_X25_THRUCLASS_TC48000	12
#define SNMP_X25_THRUCLASS_TC64000	13
#define SNMP_X25_THRUCLASS_TCRES14	14
#define SNMP_X25_THRUCLASS_TCRES15	15
#define SNMP_X25_THRUCLASS_TCRES16	16
#define SNMP_X25_THRUCLASS_TCNONE	17
#define SNMP_X25_THRUCLASS_TCDEF	18

/* x25CallParmChargingInfo */
#define SNMP_X25_CHARGINGINFO_DEF		1
#define SNMP_X25_CHARGINGINFO_NOFACL		2
#define SNMP_X25_CHARGINGINFO_NOCHRGINFO	3
#define SNMP_X25_CHARGINGINFO_CHARGINFO		4

/* x25CallParmExptData */
#define SNMP_X25_EXPTDATA_DEFULT	1
#define SNMP_X25_EXPTDATA_NOEXPTDATA	2
#define SNMP_X25_EXPTDATA_EXPTDATA	3


/*
 ********************************************************************
 *			Frame Relay
 */
#define ADSL_STATION_DECODE(station)\
		(station == 0) ? "ETH_LLC" : \
		(station == 1) ? "ETH_VC" : \
		(station == 2) ? "CIP_LLC" : \
		(station == 3) ? "IP_VC" : \
		(station == 4) ? "PPP_LLC" : \
		(station == 5) ? "PPP_VC" : "N/A"
		
/*
 * SNMP defines
 */
#define SNMP_FR_UNICAST		1
#define SNMP_FR_ONEWAY		2
#define SNMP_FR_TWOWAY		3
#define SNMP_FR_NWAY		4

#define SNMP_FR_STATIC		1
#define SNMP_FR_DYNAMIC		2

#define SNMP_FR_INVALID		1
#define SNMP_FR_ACTIVE		2
#define SNMP_FR_INACTIVE	3

#define SNMP_FR_NOLMICONF	1
#define SNMP_FR_LMIREV		2
#define SNMP_FR_ANSIT1617D	3
#define SNMP_FR_ANSIT1617B	4
#define SNMP_FR_ITUT933A	5
#define SNMP_FR_ANSIT1617D1994	6

#define SNMP_FR_Q921_ADDR	1
#define SNMP_FR_Q922MARCH90	2
#define SNMP_FR_Q922NOV90	3
#define SNMP_FR_Q922		4

#define SNMP_FR_2BYTE_ADDR	2
#define SNMP_FR_3BYTE_ADDR	3
#define SNMP_FR_4BYTE_ADDR	4

#define SNMP_FR_NONBROADCAST	1
#define SNMP_FR_BROADCAST	2

#define SNMP_FR_RUNNING		1
#define SNMP_FR_FAULT		2
#define SNMP_FR_INITIALIZING	3

#define SNMP_FR_ENABLED		1
#define SNMP_FR_DISABLED	2

#define SNMP_FR_UNKNOWNERR	1
#define SNMP_FR_RECEIVESHORT	2
#define SNMP_FR_RECEIVELONG	3
#define SNMP_FR_ILLEGALADDR	4
#define SNMP_FR_UNKNOWNADDR	5
#define SNMP_FR_DLCMIPROTOERR	6
#define SNMP_FR_DLCMIUNKNOWNERR	7
#define SNMP_FR_DLCMISEQERR	8
#define SNMP_FR_DLCMIUNKNOWNRPT	9
#define SNMP_FR_NOERRSINCERESET	10

#define SNMP_FR_ERRDATA_LEN	1600

#define SNMP_FR_SET_ERR(chan, type, data, len)	{struct timeval tv;	\
				do_gettimeofday(&tv);			\
				chan->err_type = type;			\
				memcpy(chan->err_data, data, len);	\
				chan->err_time = tv.tv_sec;		\
				chan->err_faults++;}




#endif	/* WAN_KERNEL */

#endif 	/* __WANPROC_H */
