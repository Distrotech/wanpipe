/******************************************************************************
 * 
 * sdla_te3.h	Sangoma T3/E3 front end definitions.
 * 	
 * 		Alex Feldman <al.feldman@sangoma.com>
 * 		
 * 	Copyright Sangoma Technologies Inc. 1999, 2000, 2001, 2002, 2003, 2004
 *
 * 	This program is provided subject to the Software License included in 
 * 	this package in the file license.txt. By using this program you agree
 * 	to be bound bythe terms of this license. 
 *
 * 	Should you not have a copy of the file license.txt, or wish to obtain
 * 	a hard copy of the Software License, please contact Sangoma
 * 	technologies Corporation.
 *
 * 	Contact: Sangoma Technologies Inc. 905-474-1990, info@sangoma.com
 *
 *****************************************************************************/

#ifndef __SDLA_TE3_H
# define __SDLA_TE3_H

#define WAN_TE3_LIU_LB_NORMAL	0x00
#define WAN_TE3_LIU_LB_ANALOG	0x01
#define WAN_TE3_LIU_LB_REMOTE	0x02
#define WAN_TE3_LIU_LB_DIGITAL	0x03
#define WAN_TE3_LB_TYPE_DECODE(type)					\
		((type) == WAN_TE3_LIU_LB_ANALOG) ? "Analog" :		\
		((type) == WAN_TE3_LIU_LB_REMOTE) ? "Remote" :		\
		((type) == WAN_TE3_LIU_LB_DIGITAL) ? "Digital Local" :	\
						"Unknown"

/* Line loopback activate/deactive modes */
#define WAN_TE3_LB_ENABLE	0x01
#define WAN_TE3_LB_DISABLE	0x02
#define WAN_TE3_LB_ACTION_DECODE(mode)				\
		((mode) == WAN_TE3_LB_ENABLE) ? "Enable" :	\
		((mode) == WAN_TE3_LB_DISABLE) ? "Disable" :\
						"Unknown"

#define WAN_TE3_RDEVICE_NONE		0x00
#define WAN_TE3_RDEVICE_ADTRAN		0x01
#define WAN_TE3_RDEVICE_DIGITALLINK	0x02
#define WAN_TE3_RDEVICE_KENTROX		0x03
#define WAN_TE3_RDEVICE_LARSCOM		0x04
#define WAN_TE3_RDEVICE_VERILINK	0x05	

#define WAN_TE3_BIT_LOS_ALARM		0x0001
#define WAN_TE3_BIT_OOF_ALARM		0x0002
#define WAN_TE3_BIT_AIS_ALARM		0x0004
#define WAN_TE3_BIT_AIC_ALARM		0x0008
#define WAN_TE3_BIT_YEL_ALARM		0x0010
#define WAN_TE3_BIT_LOF_ALARM		0x0020

#define RDEVICE_DECODE(rdevice)							\
		(rdevice == WAN_TE3_RDEVICE_NONE) 	 ? "None" :		\
		(rdevice == WAN_TE3_RDEVICE_ADTRAN) 	 ? "ADTRAN" :		\
		(rdevice == WAN_TE3_RDEVICE_DIGITALLINK) ? "DIGITALLINK" :	\
		(rdevice == WAN_TE3_RDEVICE_KENTROX)	 ? "KENTROX" :		\
		(rdevice == WAN_TE3_RDEVICE_LARSCOM)	 ? "LARSCOM" :		\
		(rdevice == WAN_TE3_RDEVICE_VERILINK)	 ? "VERLINK" :		\
						"Unknown"

typedef struct {
	int	rx_equal;	/* receive equalization enable (TRUE/FALSE) */
	int	taos;		/* transmit all ones select (TRUE/FALSE) */
	int	lb_mode;	/* loopback modes */
	int	tx_lbo;		/* transmit line build-out (TRUE/FALSE) */
} sdla_te3_liu_cfg_t;

typedef struct {
	sdla_te3_liu_cfg_t	liu_cfg;
	int			fractional;
	int			rdevice_type;
	int			fcs;
	int			clock;
	unsigned char		lcode;
} sdla_te3_cfg_t;

typedef struct {
	u_int32_t	pmon_lcv;
	u_int32_t	pmon_framing;
	u_int32_t	pmon_parity;
	u_int32_t	pmon_febe;
	u_int32_t	pmon_cpbit;
} sdla_te3_pmon_t;

#define IS_DS3(cfg)	((cfg)->media == WAN_MEDIA_DS3)
#define IS_E3(cfg)	((cfg)->media == WAN_MEDIA_E3)
#define IS_TE3(cfg)	(				\
		(cfg)->media == WAN_MEDIA_DS3 ||	\
		(cfg)->media == WAN_MEDIA_E3)


#define WAN_TE3_ALARM(alarm, bit)	((alarm) & (bit)) ? "ON" : "OFF"

#define WAN_TE3_LOS_ALARM(alarm)	WAN_TE3_ALARM(alarm, WAN_TE3_BIT_LOS_ALARM)
#define WAN_TE3_OOF_ALARM(alarm)	WAN_TE3_ALARM(alarm, WAN_TE3_BIT_OOF_ALARM)
#define WAN_TE3_AIS_ALARM(alarm)	WAN_TE3_ALARM(alarm, WAN_TE3_BIT_AIS_ALARM)
#define WAN_TE3_AIC_ALARM(alarm)	WAN_TE3_ALARM(alarm, WAN_TE3_BIT_AIC_ALARM)
#define WAN_TE3_YEL_ALARM(alarm)	WAN_TE3_ALARM(alarm, WAN_TE3_BIT_YEL_ALARM)
#define WAN_TE3_LOF_ALARM(alarm)	WAN_TE3_ALARM(alarm, WAN_TE3_BIT_LOF_ALARM)

#if defined(WAN_KERNEL)

typedef struct {
    unsigned long	critical;
	int dummy;
	u_int8_t	cpld_cntrl;
	u_int8_t	cpld_status;
	u_int8_t	e3_lb_ctrl;
	u_int8_t	e3_connect_delay;
} sdla_te3_param_t;

int sdla_te3_iface_init(void *p_fe_iface);
#endif /* WAN_KERNEL */

#endif /* __SDLA_TE3_H */
