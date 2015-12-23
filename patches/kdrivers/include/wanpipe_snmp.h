/*****************************************************************************
* wansnmp.h	Definitions for the WAN SNMP.
*
* Author: 	Alex Feldman <al.feldman@sangoma.com>
*
* ============================================================================
* Aug 20, 2001  Alex Feldman	Initial version.
* 				This version includes SNMP mibs for Frame Relay,
* 				PPP and X25 (v-1.0.1).			
* 	
* Apr 10, 2003  Alex Feldman	Add support SNMP mib for DS1 (v-1.0.4).
*
*/

#ifndef __WANSNMP_H
# define __WANSNMP_H

#define WANPIPE_SNMP_VERSION	"1.0.4"

#if defined(__NetBSD__) || defined(__FreeBSD__) || defined (__OpenBSD__)
# define SIOC_WANPIPE_SNMP		_IOWR('i', 100, struct ifreq)
# define SIOC_WANPIPE_SNMP_IFSPEED	_IOWR('i', 101, struct ifreq)
#endif

#define SNMP_READ	1
#define SNMP_WRITE	2

/*
 ********************************************************************
 *				X.25
 */
#define X25_STATION_DECODE(station)				\
		(station == WANOPT_DTE) ? "DTE" : 		\
		(station == WANOPT_DCE) ? "DCE" : "DXE"
		
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

#define SNMP_X25_MAX_CIRCUIT_NUM	4096
/*
 ********************************************************************
 *			Frame Relay
 */
#define FR_STATION_DECODE(station)				\
		(station == WANOPT_CPE) ? "CPE" : "Node"
		
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

				
/*
 ********************************************************************
 *			Point-to-Point
 */

#define SNMP_PPP_IPOPENED		1
#define SNMP_PPP_IPNOTOPENED		2

#define SNMP_PPP_COMPR_PROT_NONE	1
#define SNMP_PPP_COMPR_PROT_VJ_TCP	2


/*
 ********************************************************************
 *			T1/E1 
 */
#define SNMP_DSX1_NOALARM		1
#define SNMP_DSX1_RCVFARENDLOF		2
#define SNMP_DSX1_XMTFARENDLOF		4
#define SNMP_DSX1_RCVAIS		8
#define SNMP_DSX1_XMTAIS		16
#define SNMP_DSX1_LOSSOFFRAME		32
#define SNMP_DSX1_LOSSOFSIGNAL		64
#define SNMP_DSX1_LOOPBACKSTATE		128
#define SNMP_DSX1_T16AIS		2568
#define SNMP_DSX1_RCVFARENDLOMF		512
#define SNMP_DSX1_XMTFARENDLOMF		1024
#define SNMP_DSX1_RCVTESTCODE		2048
#define SNMP_DSX1_OTHERFAILURE		4096
#define SNMP_DSX1_UNAVAILSIGSTATE	8192
#define SNMP_DSX1_NETEUIPOOS		16384
#define SNMP_DSX1_RCVPAYLOADAIS		32768
#define SNMP_DSX1_DS2PERFTHRESHOLD	65536
				
/*
 ********************************************************************
 *			Common Defines
 */
#ifndef MAX_OID_LEN
# define MAX_OID_LEN	128
#endif

/*
*****************************************************************************
			TYPEDEF/STRUCTURES
******************************************************************************
*/
/* SNMP */
#define snmp_val	value.val
#define snmp_data	value.data
				
typedef struct wanpipe_snmp
{
	int		snmp_namelen;
	unsigned char	snmp_name[MAX_OID_LEN];
	int		snmp_cmd;
	int		snmp_magic;
	union {
		int		val;
		char	data[800];
	} value;
} wanpipe_snmp_t;


#if !defined(WANLIP_DRIVER)
		
#define SNMP_FR_SET_ERR(chan, type, data, len)	{struct timeval tv;	\
				do_gettimeofday(&tv);			\
				chan->err_type = type;			\
				memcpy(chan->err_data, data, len);	\
				chan->err_time = tv.tv_sec;		\
				chan->err_faults++;}

#endif


#endif /* __WANSNMP_H */
