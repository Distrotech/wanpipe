/*************************************************************************
* wanpipe_cfg_def.h														*
*																		*
*	WANPIPE(tm)	Wanpipe Global configuration defines 					*
*																		*
* Author:	Alex Feldman <al.feldman@sangoma.com>						*
*=======================================================================*
* Aug 27, 2008	Alex Feldman	Initial version							*
*																		*
*************************************************************************/

#ifndef __WANPIPE_CFG_DEF_H__
# define __WANPIPE_CFG_DEF_H__

#if defined(__WINDOWS__)
 #define	WAN_IFNAME_SZ	128		/* max length of the interface name */
#else
 #define	WAN_IFNAME_SZ	15		/* max length of the interface name */
#endif

#define	WAN_DRVNAME_SZ	WAN_IFNAME_SZ	/* max length of the link driver name */
#define	WAN_ADDRESS_SZ	31		/* max length of the WAN media address */


typedef enum {
    RFC_MODE_BRIDGED_ETH_LLC    = 0,
    RFC_MODE_BRIDGED_ETH_VC     = 1,
    RFC_MODE_ROUTED_IP_LLC      = 2,
    RFC_MODE_ROUTED_IP_VC       = 3,
    RFC_MODE_RFC1577_ENCAP      = 4,
    RFC_MODE_PPP_LLC 	    	= 5,
    RFC_MODE_PPP_VC		= 6,
    RFC_MODE_STACK_VC		= 7
} RFC_MODE;

/* 'state' defines */
enum wan_states
{
	WAN_UNCONFIGURED,	/* link/channel is not configured */
	WAN_DISCONNECTED,	/* link/channel is disconnected */
	WAN_CONNECTING,		/* connection is in progress */
	WAN_CONNECTED,		/* link/channel is operational */
	WAN_LIMIT,		/* for verification only */
	WAN_DUALPORT,		/* for Dual Port cards */
	WAN_DISCONNECTING,
	WAN_FT1_READY		/* FT1 Configurator Ready */
};

enum {
	WAN_LOCAL_IP,
	WAN_POINTOPOINT_IP,
	WAN_NETMASK_IP,
	WAN_BROADCAST_IP
};

/* Defines for UDP PACKET TYPE */
#define UDP_PTPIPE_TYPE 	0x01
#define UDP_FPIPE_TYPE		0x02
#define UDP_CPIPE_TYPE		0x03
#define UDP_DRVSTATS_TYPE 	0x04
#define UDP_INVALID_TYPE  	0x05

#define UDPMGMT_UDP_PROTOCOL	0x11

/* Command return code */
#define WAN_CMD_OK		0	/* normal firmware return code */
#define WAN_CMD_TIMEOUT		0xFF	/* firmware command timed out */
/* FIXME: Remove these 2 defines (use WAN_x) */
#define CMD_OK		0	/* normal firmware return code */
#define CMD_TIMEOUT		0xFF	/* firmware command timed out */

/* UDP Packet Management */
#define UDP_PKT_FRM_STACK	0x00
#define UDP_PKT_FRM_NETWORK	0x01

#define WAN_UDP_FAILED_CMD  	0xCF
#define WAN_UDP_INVALID_CMD 	0xCE 
#define WAN_UDP_TIMEOUT_CMD 	0xAA 
#define WAN_UDP_INVALID_NET_CMD     0xCD

/* Maximum interrupt test counter */
#define MAX_INTR_TEST_COUNTER	100
#define MAX_NEW_INTR_TEST_COUNTER	5

/* Critical Values for RACE conditions*/
#define CRITICAL_IN_ISR		0xA1
#define CRITICAL_INTR_HANDLED	0xB1

/* Card Types */
#define WANOPT_S50X		1
#define WANOPT_S51X		2
#define WANOPT_ADSL		3
#define WANOPT_AFT		4
#define WANOPT_AFT104		5
#define WANOPT_AFT300		6
#define WANOPT_AFT_ANALOG	7
#define WANOPT_AFT108		8
#define WANOPT_AFT_X		9
#define WANOPT_AFT102		10
#define WANOPT_AFT_ISDN		11
#define WANOPT_AFT_56K		12
#define WANOPT_AFT101		13
#define WANOPT_AFT_SERIAL	14
#define WANOPT_USB_ANALOG       15
#define WANOPT_AFT600           16
#define WANOPT_AFT601           17
#define WANOPT_AFT_GSM          18
#define WANOPT_AFT610           19
#define WANOPT_AFT116           20
#define WANOPT_T116		21

/*
 * Configuration options defines.
 */
/* general options */
#define	WANOPT_OFF	0
#define	WANOPT_ON	1
#define	WANOPT_NO	0
#define	WANOPT_YES	1

#define	WANOPT_SIM	2

/* interface options */
#define	WANOPT_RS232	0
#define	WANOPT_V35	1
#define WANOPT_X21      3

/* data encoding options */
#define	WANOPT_NRZ	0
#define	WANOPT_NRZI	1
#define	WANOPT_FM0	2
#define	WANOPT_FM1	3

/* line idle option */
#define WANOPT_IDLE_FLAG 0
#define WANOPT_IDLE_MARK 1

/* link type options */
#define	WANOPT_POINTTOPOINT	0	/* RTS always active */
#define	WANOPT_MULTIDROP	1	/* RTS is active when transmitting */

/* clocking options */
#define	WANOPT_EXTERNAL	0
#define	WANOPT_INTERNAL	1
#define WANOPT_RECOVERY 2		/*Uses another oscillator(to be put by hw guys) to */
					/*generate almost exactly baud rate 64333 and  dividor 2*/

/* station options */
#define	WANOPT_DTE		0
#define	WANOPT_DCE		1
#define	WANOPT_SECONDARY	0
#define	WANOPT_PRIMARY		1

/* connection options */
#define	WANOPT_PERMANENT	0	/* DTR always active */
#define	WANOPT_SWITCHED		1	/* use DTR to setup link (dial-up) */
#define	WANOPT_ONDEMAND		2	/* activate DTR only before sending */

/* ASY Mode Options */
#define WANOPT_ONE 		1
#define WANOPT_TWO		2
#define WANOPT_ONE_AND_HALF	3

#define WANOPT_NONE	0
#define WANOPT_ODD      1
#define WANOPT_EVEN	2

/* ATM sync options */
#define WANOPT_AUTO	0
#define WANOPT_MANUAL	1

#define WANOPT_DSP_HPAD	0
#define WANOPT_DSP_TPAD	1


/* SS7 options */
#define WANOPT_SS7_FISU 0
#define WANOPT_SS7_LSSU 1

#define WANOPT_SS7_MODE_128 	0
#define WANOPT_SS7_MODE_4096	1

#define WANOPT_SS7_FISU_128_SZ  3
#define WANOPT_SS7_FISU_4096_SZ 6


/* CHDLC Protocol Options */
/* DF Commmented out for now.

#define WANOPT_CHDLC_NO_DCD		IGNORE_DCD_FOR_LINK_STAT
#define WANOPT_CHDLC_NO_CTS		IGNORE_CTS_FOR_LINK_STAT
#define WANOPT_CHDLC_NO_KEEPALIVE	IGNORE_KPALV_FOR_LINK_STAT
*/



/* SS7 options */
#define WANOPT_SS7_ANSI		1
#define WANOPT_SS7_ITU		2	
#define WANOPT_SS7_NTT		3	


/* Port options */
#define WANOPT_PRI 0
#define WANOPT_SEC 1
/* read mode */
#define	WANOPT_INTR	0
#define WANOPT_POLL	1


#define WANOPT_TTY_SYNC  0
#define WANOPT_TTY_ASYNC 1

/* RBS Signalling Options */
#define WAN_RBS_SIG_A	0x01
#define WAN_RBS_SIG_B	0x02
#define WAN_RBS_SIG_C	0x04
#define WAN_RBS_SIG_D	0x08

/* Front End Ref Clock Options */

#define WANOPT_FE_OSC_CLOCK 	0x00
#define WANOPT_FE_LINE_CLOCK 	0x01

#define WANOPT_NETWORK_SYNC_OUT	0x00
#define WANOPT_NETWORK_SYNC_IN	0x01

#define WAN_CLK_OUT_OSC		0x03
#define WAN_CLK_OUT_LINE	0x04

#define WAN_CLK_IN_8000HZ  0x05
#define WAN_CLK_IN_2000HZ  0x06
#define WAN_CLK_IN_1500HZ  0x07

#define WANOPT_HW_PORT_MAP_DEFAULT 0
#define WANOPT_HW_PORT_MAP_LINEAR  1


enum {
 	WANOPT_OCT_CHAN_OPERMODE_NORMAL,
 	WANOPT_OCT_CHAN_OPERMODE_SPEECH,
 	WANOPT_OCT_CHAN_OPERMODE_NO_ECHO,
};

#endif /* __WANPIPE_CFG_DEF_H__ */
