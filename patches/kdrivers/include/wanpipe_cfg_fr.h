/*************************************************************************
* wanpipe_cfg_fr.h							 *
*									 *
*	WANPIPE(tm)	Wanpipe Frame Relay Interface configuration	 *
*									 *
* Author:	Alex Feldman <al.feldman@sangoma.com>			 *
*========================================================================*
* Aug 27, 2008	Alex Feldman	Initial version				 *
*									 *
*************************************************************************/

#ifndef __WANPIPE_CFG_FR_H__
# define __WANPIPE_CFG_FR_H__

/* frame relay in-channel signalling */
#define WANOPT_FR_AUTO_SIG	0	/* Automatically find singalling */
#define	WANOPT_FR_ANSI		1	/* ANSI T1.617 Annex D */
#define	WANOPT_FR_Q933		2	/* ITU Q.933A */
#define	WANOPT_FR_LMI		3	/* LMI */
#define	WANOPT_FR_NO_LMI	4	/* NO LMI */

#define WANOPT_FR_EEK_OFF	0	/* Frame Relay EEK Disabled */
#define WANOPT_FR_EEK_REQUEST	1	/* Frame Relay EEK Request Mode */
#define WANOPT_FR_EEK_REPLY	2	/* Frame Relay EEK Reply Mode */

#define	WANOPT_CPE		0
#define	WANOPT_NODE		1

/*----------------------------------------------------------------------------
 * Frame relay specific link-level configuration.
 */

#ifndef MAX_NUMBER_OF_PROTOCOL_INTERFACES
#define MAX_NUMBER_OF_PROTOCOL_INTERFACES 100
#endif

# define DLCI_LIST_LEN MAX_NUMBER_OF_PROTOCOL_INTERFACES

#pragma pack(1)

typedef struct wan_fr_conf
{
	unsigned int signalling;	/* local in-channel signalling type */
	unsigned int t391;		/* link integrity verification timer */
	unsigned int t392;		/* polling verification timer */
	unsigned int n391;		/* full status polling cycle counter */
	unsigned int n392;		/* error threshold counter */
	unsigned int n393;		/* monitored events counter */
	unsigned int dlci_num;		/* number of DLCs (access node) */
	unsigned int dlci[DLCI_LIST_LEN];/* List of all DLCIs */
	unsigned char issue_fs_on_startup;
	unsigned char station;  	/* Node or CPE */
	unsigned int eek_cfg;		/* EEK Request Reply Mode */
	unsigned int eek_timer;		/* EEK Request Reply Timer */
	unsigned char auto_dlci;	/* 1 - yes, 0 - no */
} wan_fr_conf_t;

/* used by wanpipemon to get DLCI status */
#define DLCI_NAME_LEN	20
typedef struct wan_lip_fr_dlci
{
	unsigned short 	dlci;
	unsigned int	dlci_type;
	unsigned char	dlci_state;
	unsigned char	name[20];
	unsigned int	down;
	unsigned char 	type;
} wan_fr_dlci_t;

#pragma pack()

#endif /* __WANPIPE_CFG_FR_H__ */

