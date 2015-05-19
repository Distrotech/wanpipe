/*****************************************************************************
* wan_plxup.h 
*
* Copyright:    (c) 2006 SANGOMA CORP.
*
* ----------------------------------------------------------------------------
* Nov 1, 2006		Initial version.
*****************************************************************************/


#if !defined(__WAN_PLXUP_H__)
# define __WAN_PLXUP_H__

#define WAN_PLXUP_MAJOR_VERSION	1
#define WAN_PLXUP_MINOR_VERSION	2

#define MAX_IFNAME_LEN		20

#define WAN_PLXUP_NAME_LEN	50
#define WAN_PLXUP_MAX_ESIZE	0x7F

enum {
	WAN_PLXUP_WRITE_EFILE_CMD = 0x01,
	WAN_PLXUP_READ_EFILE_CMD,
	WAN_PLXUP_WRITE_E_CMD,
	WAN_PLXUP_READ_E_CMD
};	

 typedef struct {
 	int		sock;
	char		if_name[MAX_IFNAME_LEN];
	unsigned int	addr;
	unsigned int	value;
	char		*file;
} wan_plxup_t;

#endif /* __EX10CTRL_H__ */
