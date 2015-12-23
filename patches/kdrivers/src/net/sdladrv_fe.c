/*****************************************************************************
* sdladrv_fe.c	SDLA FE interface support Module.
*
*
* Author:	Alex Feldman
*
* Copyright:	(c) 2006 Sangoma Technologies Inc.
*
*		This program is free software; you can redistribute it and/or
*		modify it under the terms of the GNU General Public License
*		as published by the Free Software Foundation; either version
*		2 of the License, or (at your option) any later version.
* ============================================================================
* Aug 10, 2006	Alex Feldman	Initial version
*
* July 5, 2007	David Rokhvarg	Added support of A500 - ISDN BRI card.
*****************************************************************************/

/*****************************************************************************
 * Notes:
 * ------
 ****************************************************************************/


#define __SDLA_HW_LEVEL
#define __SDLADRV__

#define SDLADRV_NEW

/***************************************************************************
****		I N C L U D E  		F I L E S			****
***************************************************************************/
#if defined(__LINUX__)||defined(__KERNEL__)
# define _K22X_MODULE_FIX_
#endif

#include "wanpipe_includes.h"
#include "wanpipe_defines.h"
#include "wanpipe_debug.h"
#include "wanpipe_common.h"
#include "wanpipe.h"

#include "sdlasfm.h"	/* SDLA firmware module definitions */
#include "sdlapci.h"	/* SDLA PCI hardware definitions */
#include "sdladrv.h"	/* API definitions */


#if defined(WAN_DEBUG_FE)
# if defined(__WINDOWS__)
#  pragma message("WAN_DEBUG_FE - Debugging Enabled")
# else
#  warning "WAN_DEBUG_FE - Debugging Enabled"
# endif
#endif

#if defined(WAN_DEBUG_REG)
# if defined(__WINDOWS__)
#  pragma message("WAN_DEBUG_REG - Debugging Enabled")
# else
# warning "WAN_DEBUG_REG - Debugging Enabled"
# endif
#endif


#define AFT_A600_BASE_REG_OFF	0x1000
#define A600_REG_OFF(reg) reg+AFT_A600_BASE_REG_OFF


/***************************************************************************
****                     M A C R O S / D E F I N E S                    ****
***************************************************************************/
#define BIT_DEV_ADDR_CLEAR      0x600
#define BIT_DEV_ADDR_CPLD       0x200
#define XILINX_MCPU_INTERFACE           0x44
#define XILINX_MCPU_INTERFACE_ADDR      0x46

/***************************************************************************
****               F U N C T I O N   P R O T O T Y P E S                ****
***************************************************************************/
extern int sdla_cmd (void* phw, unsigned long offset, wan_mbox_t* mbox);
u_int8_t sdla_legacy_read_fe (void *phw, ...);
int sdla_legacy_write_fe (void *phw, ...);

int		sdla_te1_write_fe(void* phw, ...);
u_int8_t	sdla_te1_read_fe (void* phw, ...);

static int	__sdla_shark_te1_write_fe(void *phw, ...);
int		sdla_shark_te1_write_fe(void *phw, ...);
u_int8_t	__sdla_shark_te1_read_fe (void *phw, ...);
u_int8_t	sdla_shark_te1_read_fe (void *phw, ...);

static int     __sdla_shark_tap_write_fe(void *phw, ...);
int            sdla_shark_tap_write_fe(void *phw, ...);
u_int8_t       __sdla_shark_tap_read_fe (void *phw, ...);
u_int8_t       sdla_shark_tap_read_fe (void *phw, ...);

static int	__sdla_shark_rm_write_fe (void* phw, ...);
int		sdla_shark_rm_write_fe (void* phw, ...);
u_int8_t	__sdla_shark_rm_read_fe (void* phw, ...);
u_int8_t	sdla_shark_rm_read_fe (void* phw, ...);
void 		sdla_a200_reset_fe (void* fe);

static int	__sdla_shark_56k_write_fe(void *phw, ...);
int		sdla_shark_56k_write_fe(void *phw, ...);
u_int8_t	__sdla_shark_56k_read_fe (void *phw, ...);
u_int8_t	sdla_shark_56k_read_fe (void *phw, ...);

static int	__write_bri_fe_byte(void*,u_int8_t,u_int8_t,u_int8_t);
int		sdla_shark_bri_write_fe (void* phw, ...);
static u_int8_t __read_bri_fe_byte(void*,u_int8_t,u_int8_t,u_int8_t,u_int8_t); 
u_int8_t	sdla_shark_bri_read_fe (void* phw, ...);

static int	__sdla_shark_serial_write_fe(void *phw, ...);
int		sdla_shark_serial_write_fe(void *phw, ...);
u_int32_t	__sdla_shark_serial_read_fe (void *phw, ...);
u_int32_t	sdla_shark_serial_read_fe (void *phw, ...);

int		sdla_te3_write_fe(void *phw, ...);
u_int8_t	sdla_te3_read_fe(void *phw, ...);


static int	__sdla_a600_write_fe(void *phw, ...);
int		sdla_a600_write_fe(void *phw, ...);
u_int8_t	__sdla_a600_read_fe (void *phw, ...);
u_int8_t	sdla_a600_read_fe (void *phw, ...);
void		sdla_a600_reset_fe (void *fe);

void		sdla_a700_reset_fe (void *fe);


extern int sdla_bus_write_1(void* phw, unsigned int offset, u8 value);
extern int sdla_bus_read_1(void* phw, unsigned int offset, u8* value);
extern int sdla_bus_write_2(void* phw, unsigned int offset, u16 value);
extern int sdla_bus_read_2(void* phw, unsigned int offset, u16* value);
extern int sdla_bus_write_4(void* phw, unsigned int offset, u32 value);
extern int sdla_bus_read_4(void* phw, unsigned int offset, u32* value);

extern int sdla_hw_fe_test_and_set_bit(void *phw, int value);
extern int sdla_hw_fe_test_bit(void *phw, int value);
extern int sdla_hw_fe_clear_bit(void *phw, int value);
extern int sdla_hw_fe_set_bit(void *phw, int value);


#define SDLA_HW_T1E1_FE_ACCESS_BLOCK { u32 breg; sdla_bus_read_4(hw, 0x40, &breg); \
								  if (breg == (u32)-1) { \
									if (WAN_NET_RATELIMIT()) { \
										DEBUG_ERROR("%s:%d: wanpipe PCI Error: Illegal Register read: 0x40 = 0xFFFFFFFF\n", \
											__FUNCTION__,__LINE__);  \
									} \
								 } \
}


#define SDLA_HW_T1E1_FE_ACCESS_BLOCK_A600 { u32 breg; sdla_bus_read_4(hw, 0x1040, &breg); \
								  if (breg == (u32)-1) { \
									if (WAN_NET_RATELIMIT()) { \
										DEBUG_ERROR("%s:%d: wanpipe PCI Error: Illegal Register read: 0x1040 = 0xFFFFFFFF\n", \
											__FUNCTION__,__LINE__);  \
									} \
								 } \
}

/***************************************************************************
****                      G L O B A L  D A T A                          ****
***************************************************************************/

/***************************************************************************
****               F U N C T I O N   D E F I N I T I O N                ****
***************************************************************************/


/***************************************************************************
	Front End DS31/E3 interface
***************************************************************************/
int sdla_te3_write_fe(void *phw, ...)
{
	sdlahw_t*	hw = (sdlahw_t*)phw;
	va_list		args;
	int		off, value;

	WAN_ASSERT(phw == NULL);
	va_start(args, phw);
	off	= va_arg(args, int);
	value	= va_arg(args, int);
	va_end(args);
	
        off &= ~AFT_BIT_DEV_ADDR_CLEAR;

	DEBUG_TEST("%s: WRITE FRAMER OFFSET=0x%02X DATA=0x%02X\n",
			hw->devname, off,value);

       	sdla_bus_write_2(hw, AFT_MCPU_INTERFACE_ADDR, (u16)off);
       	sdla_bus_write_2(hw, AFT_MCPU_INTERFACE, (u16)value);
        return 0;
}

u_int8_t sdla_te3_read_fe(void *phw, ...)
{
	sdlahw_t*	hw = (sdlahw_t*)phw;
	va_list		args;
	int		off;
	u_int8_t	value;

	WAN_ASSERT(phw == NULL);
	va_start(args, phw);
	off	= va_arg(args, int);
	va_end(args);
	
        off &= ~AFT_BIT_DEV_ADDR_CLEAR;
       	sdla_bus_write_2(hw, AFT_MCPU_INTERFACE_ADDR, (u16)off);	
        sdla_bus_read_1(hw,AFT_MCPU_INTERFACE, &value);
	
	DEBUG_TEST("%s: READ FRAMER OFFSET=0x%02X DATA=0x%02X\n",
			hw->devname, off, value);
        return value;
}

/***************************************************************************
**	Front End T1/E1 interface for S-Series cards
***************************************************************************/
typedef struct {
	unsigned short 	register_number;
	unsigned char		register_value;
} sdla_legacy_fe_t;
wan_mbox_t	wan_legacy_mbox;

static int sdla_legacy_fe_error (sdlahw_t *hw, int err, u_int8_t cmd)
{
	switch (err) {
	case WAN_CMD_TIMEOUT:
		DEBUG_EVENT("%s: command 0x%02X timed out!\n",
					hw->devname, cmd);
		break;

	default:
		DEBUG_EVENT("%s: command 0x%02X returned 0x%02X!\n",
					hw->devname, cmd, err);
	}
	return 0;
}

u_int8_t sdla_legacy_read_fe (void *phw, ...)
{
	va_list	args;
	sdlahw_t	*hw = (sdlahw_t*)phw;
       wan_mbox_t	*mb = &wan_legacy_mbox;
	char		*data = mb->wan_data;
	int		qaccess, reg, line_no;
	int		err;

	va_start(args, phw);
	qaccess = va_arg(args, int);
	line_no = va_arg(args, int);
	reg	 = va_arg(args, int);
	va_end(args);

	((sdla_legacy_fe_t*)data)->register_number = (unsigned short)reg;
	mb->wan_data_len = sizeof(sdla_legacy_fe_t);
       mb->wan_command = 0x90;
	/* Mailbox address 0xE000 */
       err = sdla_cmd(hw, 0xE000, mb);
       if (err){
		sdla_legacy_fe_error(hw,err,0x90);
	}

	return(((sdla_legacy_fe_t*)data)->register_value);
}

/*============================================================================
 * Write to TE1/56K Front end registers  
 */
int sdla_legacy_write_fe (void *phw, ...)
{
	va_list	args;
	sdlahw_t	*hw = (sdlahw_t*)phw;
       wan_mbox_t	*mb = &wan_legacy_mbox;
	char		*data = mb->wan_data;
	int		qaccess, reg, line_no, value;
	int		err, retry=15;

	va_start(args, phw);
	qaccess = va_arg(args, int);
	line_no = va_arg(args, int);
	reg	 = va_arg(args, int);
	value	 = va_arg(args, int);
	va_end(args);
	
	do {
		((sdla_legacy_fe_t*)data)->register_number = (unsigned short)reg;
		((sdla_legacy_fe_t*)data)->register_value = (unsigned char)value;
		mb->wan_data_len = sizeof(sdla_legacy_fe_t);
		mb->wan_command = 0x91;
		err = sdla_cmd(hw, 0xE000, mb);
		if (err){
			sdla_legacy_fe_error(hw,err,0x91);
		}
	}while(err && --retry);
	return err;
}

/***************************************************************************
	Front End T1/E1 interface for Normal cards
***************************************************************************/
int sdla_te1_write_fe(void* phw, ...)
{
	sdlahw_t*	hw = (sdlahw_t*)phw;
	va_list		args;
	int		qaccess, off, line_no, value;

//	u8	qaccess = card->wandev.state == WAN_CONNECTED ? 1 : 0;
	       	 
	va_start(args, phw);
	qaccess	= va_arg(args, int);
	line_no = va_arg(args, int);
	off	= va_arg(args, int);
	value	= va_arg(args, int);
	va_end(args);

	SDLA_HW_T1E1_FE_ACCESS_BLOCK;

	off &= ~BIT_DEV_ADDR_CLEAR;
    sdla_bus_write_2(hw, XILINX_MCPU_INTERFACE_ADDR, (u16)off);
	
	/* AF: Sep 10, 2003
	 * IMPORTANT
	 * This delays are required to avoid bridge optimization 
	 * (combining two writes together)
	 */
	SDLA_HW_T1E1_FE_ACCESS_BLOCK;

    sdla_bus_write_1(hw, XILINX_MCPU_INTERFACE, (u8)value);
        
	SDLA_HW_T1E1_FE_ACCESS_BLOCK;

	return 0;
}


/*============================================================================
 * Read TE1/56K Front end registers
 */
u_int8_t sdla_te1_read_fe (void* phw, ...)
{
	sdlahw_t*	hw = (sdlahw_t*)phw;
	va_list		args;
	int		qaccess, line_no, off;
	u_int8_t	tmp;
//	u8	qaccess = card->wandev.state == WAN_CONNECTED ? 1 : 0;

	va_start(args, phw);
	qaccess = va_arg(args, int);
	line_no = va_arg(args, int);
	off	= va_arg(args, int);
	va_end(args);

	SDLA_HW_T1E1_FE_ACCESS_BLOCK;
    
	off &= ~BIT_DEV_ADDR_CLEAR;
    sdla_bus_write_2(hw, XILINX_MCPU_INTERFACE_ADDR, (u16)off);
	
	SDLA_HW_T1E1_FE_ACCESS_BLOCK;
    
	sdla_bus_read_1(hw,XILINX_MCPU_INTERFACE, &tmp);

	SDLA_HW_T1E1_FE_ACCESS_BLOCK;
	
    return tmp;
}

/***************************************************************************
	Front End T1/E1 interface for Shark subtype cards
***************************************************************************/
static int __sdla_shark_te1_write_fe (void *phw, ...)
{
	sdlahw_t*	hw = (sdlahw_t*)phw;
	sdlahw_cpu_t	*hwcpu;
	sdlahw_card_t	*hwcard;
	va_list		args;
	int		org_off=0, qaccess=0, line_no=0, off=0, value=0;

	WAN_ASSERT(hw == NULL);
	WAN_ASSERT(hw->hwcpu == NULL);
	WAN_ASSERT(hw->hwcpu->hwcard == NULL);
	hwcpu = hw->hwcpu;
	hwcard = hwcpu->hwcard;
	va_start(args, phw);
	qaccess = (u_int16_t)va_arg(args, int);
	line_no = (u_int16_t)va_arg(args, int);
	off	= (u_int16_t)va_arg(args, int);
	value	= (u_int8_t)va_arg(args, int);
	va_end(args);
	WAN_ASSERT(qaccess != 0 && qaccess != 1);

	if (hwcard->core_id == AFT_PMC_FE_CORE_ID){
       		off &= ~AFT4_BIT_DEV_ADDR_CLEAR;	
	}else if (hwcard->core_id == AFT_DS_FE_CORE_ID){
		if (off & 0x800)  off |= 0x2000;
		if (off & 0x1000) off |= 0x4000;
		off &= ~AFT8_BIT_DEV_ADDR_CLEAR;
		if ((hwcard->adptr_type == A101_ADPTR_2TE1 || 
		     hwcard->adptr_type == A101_ADPTR_1TE1) && line_no == 1){
			off |= AFT8_BIT_DEV_MAXIM_ADDR_CPLD;
		}
	}


	SDLA_HW_T1E1_FE_ACCESS_BLOCK;

	sdla_bus_read_2(hw, AFT_MCPU_INTERFACE_ADDR, (u16*)&org_off);
	
	SDLA_HW_T1E1_FE_ACCESS_BLOCK;

    sdla_bus_write_2(hw,AFT_MCPU_INTERFACE_ADDR, (u16)off);
	
	/* AF: Sep 10, 2003
	 * IMPORTANT
	 * This delays are required to avoid bridge optimization 
	 * (combining two writes together)
	 */
	SDLA_HW_T1E1_FE_ACCESS_BLOCK;

	sdla_bus_write_1(hw, AFT_MCPU_INTERFACE, (u8)value);

	SDLA_HW_T1E1_FE_ACCESS_BLOCK;

	sdla_bus_write_2(hw, AFT_MCPU_INTERFACE_ADDR, (u16)org_off);	

	SDLA_HW_T1E1_FE_ACCESS_BLOCK;

    return 0;
}

static int __sdla_shark_tap_write_fe (void *phw, ...)
{
	sdlahw_t*       hw = (sdlahw_t*)phw;
	sdlahw_cpu_t    *hwcpu;
	sdlahw_card_t   *hwcard;
	va_list         args;
	int             qaccess=0, line_no=0, off=0, value=0;

	WAN_ASSERT(hw == NULL);
	WAN_ASSERT(hw->hwcpu == NULL);
	WAN_ASSERT(hw->hwcpu->hwcard == NULL);
	hwcpu = hw->hwcpu;
	hwcard = hwcpu->hwcard;
	va_start(args, phw);
	qaccess = (u_int16_t)va_arg(args, int);
	line_no = (u_int16_t)va_arg(args, int);
	off     = (u_int16_t)va_arg(args, int);
	value   = (u_int8_t)va_arg(args, int);
	va_end(args);
	WAN_ASSERT(qaccess != 0 && qaccess != 1);

	if (hwcard->core_id == AFT_PMC_FE_CORE_ID){
		off &= ~AFT4_BIT_DEV_ADDR_CLEAR;
	}else if (hwcard->core_id == AFT_DS_FE_CORE_ID){
		off &= 0X3FFF;
		if ((hwcard->adptr_type == A101_ADPTR_2TE1 ||
			hwcard->adptr_type == A101_ADPTR_1TE1) && line_no == 1){
				off |= AFT8_BIT_DEV_MAXIM_ADDR_CPLD;
		}
	}

	sdla_bus_write_2(hw,AFT_MCPU_INTERFACE, (u16)off);
	SDLA_HW_T1E1_FE_ACCESS_BLOCK;

	value = (value << 16) & 0x00FF0000;
	DEBUG_TEST("VALUE in __sdla_shark_tap_write_fe = 0x%x\n",value);
	sdla_bus_write_4(hw,AFT_MCPU_INTERFACE, (u32)value);
	SDLA_HW_T1E1_FE_ACCESS_BLOCK;

	return 0;
}

int sdla_shark_te1_write_fe (void *phw, ...)
{
	sdlahw_t*	hw = (sdlahw_t*)phw;
	va_list		args;
	int		qaccess=0, line_no=0, off=0, value=0;

	WAN_ASSERT(hw->magic != SDLADRV_MAGIC);
	if (sdla_hw_fe_test_and_set_bit(hw,0)){
		if (WAN_NET_RATELIMIT()){
			DEBUG_ERROR("%s: %s:%d: Critical Error: Re-entry in FE!\n",
					hw->devname, __FUNCTION__,__LINE__);
		}
		return -EINVAL;
	}
	va_start(args, phw);
	qaccess = va_arg(args, int);
	line_no = va_arg(args, int);
	off	= va_arg(args, int);
	value	= va_arg(args, int);
	va_end(args);

	DEBUG_REG("%s: Writting T1/E1 Reg: Line:%d: %02X=%02X\n", 
					hw->devname, line_no, off, value);
	__sdla_shark_te1_write_fe(hw, qaccess, line_no, off, value);
	sdla_hw_fe_clear_bit(hw,0);
	return 0;
}

int sdla_shark_tap_write_fe (void *phw, ...)
{
	sdlahw_t*       hw = (sdlahw_t*)phw;
	va_list         args;
	int             qaccess=0, line_no=0, off=0, value=0;

	WAN_ASSERT(hw->magic != SDLADRV_MAGIC);
	if (sdla_hw_fe_test_and_set_bit(hw,0)){
		if (WAN_NET_RATELIMIT()){
			DEBUG_ERROR("%s: %s:%d: Critical Error: Re-entry in FE!\n",
					hw->devname, __FUNCTION__,__LINE__);
		}
		return -EINVAL;
	}
	va_start(args, phw);
	qaccess = va_arg(args, int);
	line_no = va_arg(args, int);
	off     = va_arg(args, int);
	value   = va_arg(args, int);
	va_end(args);

	DEBUG_TEST("%s:IN TAP Writting T1/E1 Reg: Line:%d: %02X=%02X\n",
			hw->devname, line_no, off, value);
	__sdla_shark_tap_write_fe(hw, qaccess, line_no, off, value);
	sdla_hw_fe_clear_bit(hw,0);
	return 0;
}

/*============================================================================
 * Read TE1 Front end registers
 */
u_int8_t __sdla_shark_te1_read_fe (void *phw, ...)
{
	sdlahw_t*	hw = (sdlahw_t*)phw;
	sdlahw_cpu_t	*hwcpu;
	sdlahw_card_t	*hwcard;
	va_list		args;
	int     	org_off=0, qaccess=0, line_no=0, off=0, tmp=0;

	WAN_ASSERT(hw == NULL);
	WAN_ASSERT(hw->hwcpu == NULL);
	WAN_ASSERT(hw->hwcpu->hwcard == NULL);
	hwcpu = hw->hwcpu;
	hwcard = hwcpu->hwcard;
	va_start(args, phw);
	qaccess = (u_int16_t)va_arg(args, int);
	line_no = (u_int16_t)va_arg(args, int);
	off	= (u_int16_t)va_arg(args, int);
	va_end(args);
	WAN_ASSERT(qaccess != 0 && qaccess != 1);

	if (hwcard->core_id == AFT_PMC_FE_CORE_ID){
       		off &= ~AFT4_BIT_DEV_ADDR_CLEAR;	
	}else if (hwcard->core_id == AFT_DS_FE_CORE_ID){
		if (off & 0x800)  off |= 0x2000;
		if (off & 0x1000) off |= 0x4000;
		off &= ~AFT8_BIT_DEV_ADDR_CLEAR;	
		if ((hwcard->adptr_type == A101_ADPTR_1TE1 || 
		     hwcard->adptr_type == A101_ADPTR_2TE1) && line_no == 1){
			off |= AFT8_BIT_DEV_MAXIM_ADDR_CPLD;
		}
	}
	

	SDLA_HW_T1E1_FE_ACCESS_BLOCK;

	sdla_bus_read_2(hw, AFT_MCPU_INTERFACE_ADDR, (u16*)&org_off);
	
	SDLA_HW_T1E1_FE_ACCESS_BLOCK;
	
	sdla_bus_write_2(hw, AFT_MCPU_INTERFACE_ADDR, (u16)off);
	
	SDLA_HW_T1E1_FE_ACCESS_BLOCK;

	sdla_bus_read_1(hw,AFT_MCPU_INTERFACE, (u8*)&tmp);
	
	SDLA_HW_T1E1_FE_ACCESS_BLOCK;
	
	sdla_bus_write_2(hw, AFT_MCPU_INTERFACE_ADDR, (u16)org_off);	
	
	SDLA_HW_T1E1_FE_ACCESS_BLOCK;

    return (u8)tmp;
}

u_int8_t __sdla_shark_tap_read_fe (void *phw, ...)
{
	sdlahw_t*       hw = (sdlahw_t*)phw;
	sdlahw_cpu_t    *hwcpu;
	sdlahw_card_t   *hwcard;
	va_list         args;
	int             qaccess=0, line_no=0, off=0, tmp=0;

	WAN_ASSERT(hw == NULL);
	WAN_ASSERT(hw->hwcpu == NULL);
	WAN_ASSERT(hw->hwcpu->hwcard == NULL);
	hwcpu = hw->hwcpu;
	hwcard = hwcpu->hwcard;
	va_start(args, phw);
	qaccess = (u_int16_t)va_arg(args, int);
	line_no = (u_int16_t)va_arg(args, int);
	off     = (u_int16_t)va_arg(args, int);
	va_end(args);
	WAN_ASSERT(qaccess != 0 && qaccess != 1);

	if (hwcard->core_id == AFT_PMC_FE_CORE_ID){
		off &= ~AFT4_BIT_DEV_ADDR_CLEAR;
	}else if (hwcard->core_id == AFT_DS_FE_CORE_ID){
		off &= 0x3FFF;
		if ((hwcard->adptr_type == A101_ADPTR_1TE1 ||
			hwcard->adptr_type == A101_ADPTR_2TE1) && line_no == 1){
			off |= AFT8_BIT_DEV_MAXIM_ADDR_CPLD;
		}
	}

	sdla_bus_write_2(hw, AFT_MCPU_INTERFACE, (u16)off);
	
	SDLA_HW_T1E1_FE_ACCESS_BLOCK;

	sdla_bus_read_4(hw,AFT_MCPU_INTERFACE, (u32*)&tmp);
	tmp = (tmp >> 16) & 0xFF;

	SDLA_HW_T1E1_FE_ACCESS_BLOCK;

	return (u8)tmp;
}

u_int8_t sdla_shark_te1_read_fe (void *phw, ...)
{
	sdlahw_t*	hw = (sdlahw_t*)phw;
	va_list		args;
	int		qaccess=0, line_no=0, off=0;
	u_int8_t	tmp;

	WAN_ASSERT(hw->magic != SDLADRV_MAGIC);
	if (sdla_hw_fe_test_and_set_bit(hw,0)){
		if (WAN_NET_RATELIMIT()){
		DEBUG_ERROR("%s: %s:%d: Critical Error: Re-entry in FE!\n",
			hw->devname, __FUNCTION__,__LINE__);
		}
		return 0x00;
	}

	va_start(args, phw);
	qaccess = va_arg(args, int);
	line_no = va_arg(args, int);
	off	= va_arg(args, int);
	va_end(args);

	tmp = __sdla_shark_te1_read_fe(hw, qaccess, line_no, off);
	sdla_hw_fe_clear_bit(hw,0);
	DEBUG_REG("%s: Reading T1/E1 Reg: Line:%d: %02X=%02X\n", 
					hw->devname, line_no, off, tmp);
        return tmp;
}

u_int8_t sdla_shark_tap_read_fe (void *phw, ...)
{
	sdlahw_t*       hw = (sdlahw_t*)phw;
	va_list         args;
	int             qaccess=0, line_no=0, off=0;
	u_int8_t        tmp;

	WAN_ASSERT(hw->magic != SDLADRV_MAGIC);
	if (sdla_hw_fe_test_and_set_bit(hw,0)){
		if (WAN_NET_RATELIMIT()){
			DEBUG_ERROR("%s: %s:%d: Critical Error: Re-entry in FE!\n",
					hw->devname, __FUNCTION__,__LINE__);
		}
		return 0x00;
	}

	va_start(args, phw);
	qaccess = va_arg(args, int);
	line_no = va_arg(args, int);
	off     = va_arg(args, int);
	va_end(args);

	tmp = __sdla_shark_tap_read_fe(hw, qaccess, line_no, off);
	sdla_hw_fe_clear_bit(hw,0);
	DEBUG_TEST("%s: IN TAP Reading T1/E1 Reg: Line:%d: %02X=%02X\n",
			hw->devname, line_no, off, tmp);
	return tmp;
}


/***************************************************************************
	56K Front End interface for Shark subtype cards
***************************************************************************/

/*============================================================================
 * Write 56k Front end registers
 */
static int __sdla_shark_56k_write_fe (void *phw, ...)
{
	sdlahw_t*	hw = (sdlahw_t*)phw;
	va_list		args;
	int		qaccess, line_no, off, value;

	va_start(args, phw);
	qaccess = va_arg(args, int);
	line_no = va_arg(args, int);
	off	= va_arg(args, int);
	value	= va_arg(args, int);
	va_end(args);

	off &= ~AFT8_BIT_DEV_ADDR_CLEAR;	
	
	SDLA_HW_T1E1_FE_ACCESS_BLOCK;

   	sdla_bus_write_2(hw,0x46, (u16)off);
	
	SDLA_HW_T1E1_FE_ACCESS_BLOCK;

	sdla_bus_write_2(hw,0x44, (u16)value);
	
	SDLA_HW_T1E1_FE_ACCESS_BLOCK;

	return 0;
}

int sdla_shark_56k_write_fe (void *phw, ...)
{
	sdlahw_t*	hw = (sdlahw_t*)phw;
	va_list		args;
	int		qaccess, line_no, off, value;

	WAN_ASSERT(hw->magic != SDLADRV_MAGIC);
	if (sdla_hw_fe_test_and_set_bit(hw,0)){
		if (WAN_NET_RATELIMIT()){
			DEBUG_EVENT(
			"%s: %s:%d: Critical Error: Re-entry in FE!\n",
					hw->devname,
					__FUNCTION__,__LINE__);
		}
		return -EINVAL;
	}

	va_start(args, phw);
	qaccess = va_arg(args, int);
	line_no = va_arg(args, int);
	off	= va_arg(args, int);
	value	= va_arg(args, int);
	va_end(args);

	__sdla_shark_56k_write_fe(hw, qaccess, line_no, off, value);

	sdla_hw_fe_clear_bit(hw,0);
	return 0;
}

/*============================================================================
 * Read 56k Front end registers
 */
u_int8_t __sdla_shark_56k_read_fe (void *phw, ...)
{
	sdlahw_t*	hw = (sdlahw_t*)phw;
	va_list		args;
	int		qaccess, line_no, off, tmp;

	va_start(args, phw);
	qaccess = va_arg(args, int);
	line_no = va_arg(args, int);
	off	= va_arg(args, int);
	va_end(args);

	off &= ~AFT8_BIT_DEV_ADDR_CLEAR;	
	
	SDLA_HW_T1E1_FE_ACCESS_BLOCK;

   	sdla_bus_write_2(hw, AFT56K_MCPU_INTERFACE_ADDR, (u16)off);
	
	SDLA_HW_T1E1_FE_ACCESS_BLOCK;

   	sdla_bus_read_4(hw, AFT56K_MCPU_INTERFACE, &tmp);
	
	SDLA_HW_T1E1_FE_ACCESS_BLOCK;

	return (u_int8_t)tmp;
}

u_int8_t sdla_shark_56k_read_fe (void *phw, ...)
{
	sdlahw_t*	hw = (sdlahw_t*)phw;
	va_list		args;
	int		qaccess, line_no, off, tmp;

	WAN_ASSERT(hw->magic != SDLADRV_MAGIC);
	if (sdla_hw_fe_test_and_set_bit(hw,0)){
		if (WAN_NET_RATELIMIT()){
		DEBUG_ERROR("%s: %s:%d: Critical Error: Re-entry in FE!\n",
			hw->devname, __FUNCTION__,__LINE__);
		}
		return 0x00;
	}

	va_start(args, phw);
	qaccess = va_arg(args, int);
	line_no = va_arg(args, int);
	off	= va_arg(args, int);
	va_end(args);

	tmp = __sdla_shark_56k_read_fe(hw, qaccess, line_no, off);

	sdla_hw_fe_clear_bit(hw,0);
        return (u8)tmp;
}


static int __sdla_a600_write_fe(void *phw, ...)
{
	sdlahw_t	*hw = (sdlahw_t*)phw;
	va_list		args;
	int		mod_no, type, chain;
	int		reg, value;
	u32		data = 0;
	//unsigned char	cs = 0x00, ctrl_byte = 0x00;
	int		i;

	WAN_ASSERT(hw == NULL);
	WAN_ASSERT(hw->hwcpu == NULL);
	WAN_ASSERT(hw->hwcpu->hwcard == NULL);

	va_start(args, phw);
	mod_no	= va_arg(args, int);
	type	= va_arg(args, int);
	chain	= va_arg(args, int);
	reg	= va_arg(args, int);
	value	= va_arg(args, int);
	va_end(args);
	
	if (chain) DEBUG_ERROR ("%s :%d Error: chain mode not supported on A600 (%s:%d)\n",
					hw->devname, mod_no, __FUNCTION__,__LINE__);
				
		    
	if (type == MOD_TYPE_FXO){
		data |= (mod_no & 0x3) << 24;
		data &= 0xFF000000;
			
		/* Clear data bits */
		data |= (value & 0xFF);
	
		reg = reg & 0x7F;
		data |= (reg & 0xFF) << 8;
			
	}else if (type == MOD_TYPE_FXS){
		wan_set_bit(A600_SPI_REG_CHAN_TYPE_FXS_BIT, &data); /* 1 << 28 */
		
		/* Clear data bits */
		data |= (value & 0xFF);
		
		reg = reg & 0x7F;
		data |= (reg & 0xFF) << 8;
		
	}else{
		DEBUG_EVENT("%s: Module %d: Unsupported module type %d!\n",
					hw->devname, mod_no, type);
					return -EINVAL;
	}
	
	sdla_bus_write_4(hw, A600_REG_OFF(SPI_INTERFACE_REG), data);	
		
	WP_DELAY(10);
		
	wan_set_bit(A600_SPI_REG_START_BIT, &data);
	sdla_bus_write_4(hw, A600_REG_OFF(SPI_INTERFACE_REG), data);
	
	for (i=0;i<10;i++) {	
		WP_DELAY(10);
		sdla_bus_read_4(hw, A600_REG_OFF(SPI_INTERFACE_REG),&data);
		if (!(wan_test_bit(A600_SPI_REG_SPI_BUSY_BIT, &data))) {
			goto spi_write_done;
		}
	}
			
	if (wan_test_bit(A600_SPI_REG_SPI_BUSY_BIT, &data)) {
		DEBUG_ERROR("%s: ERROR:SPI Iface not ready\n", hw->devname);
		return -EINVAL;
	}
		
spi_write_done:
	return 0;
}

int sdla_a600_write_fe(void *phw, ...)
{
	sdlahw_t	*hw = (sdlahw_t*)phw;
	va_list		args;
	int		mod_no, type, chain, reg, value;
#if defined(WAN_DEBUG_FE)
	char		*fname;	
	int		fline;
#endif

	WAN_ASSERT(hw->magic != SDLADRV_MAGIC);
	va_start(args, phw);
	mod_no	= va_arg(args, int);
	type	= va_arg(args, int);
	chain	= va_arg(args, int);
	reg	= va_arg(args, int);
	value	= va_arg(args, int);
	va_end(args);

	if (sdla_hw_fe_test_and_set_bit(hw,0)){
		DEBUG_ERROR("%s: %s:%d: Critical Error: Re-entry in FE!\n",
			    hw->devname, __FUNCTION__,__LINE__);
		return -EINVAL;
	}
	
	__sdla_a600_write_fe(hw, mod_no, type, chain, reg, value);

	sdla_hw_fe_clear_bit(hw,0);
	return 0;
}



u_int8_t __sdla_a600_read_fe (void *phw, ...)
{
	sdlahw_t	*hw = (sdlahw_t*)phw;
	va_list		args;
	int		mod_no, type, chain, reg;
	u32		data = 0;
	int		i;

	WAN_ASSERT(hw == NULL);
	WAN_ASSERT(hw->hwcpu == NULL);
	WAN_ASSERT(hw->hwcpu->hwcard == NULL);

	va_start(args, phw);
	mod_no	= va_arg(args, int);
	type	= va_arg(args, int);
	chain	= va_arg(args, int);
	reg	= va_arg(args, int);
	va_end(args);

	if (chain) DEBUG_ERROR ("%s :%d Error: chain mode not supported on A600 (%s:%d)\n",
	    		hw->devname, mod_no, __FUNCTION__,__LINE__);
	

	wan_set_bit(A600_SPI_REG_READ_ENABLE_BIT, &data);
	
	if (type == MOD_TYPE_FXO){
		data |= (mod_no & 0x3) << 24;
		data &= 0xFF000000;
	
		reg = reg & 0xFF;
		data |= (reg & 0xFF) << 8;
		data &= 0xFFFFFF00;
   
	}else if (type == MOD_TYPE_FXS){
		wan_set_bit(A600_SPI_REG_CHAN_TYPE_FXS_BIT, &data);
		
		reg = reg & 0x7F;
		data |= (reg & 0xFF) << 8;
		data &= 0xFFFFFF00;
   
	} else {
		DEBUG_EVENT("%s: Module %d: Unsupported module type %d!\n",
					hw->devname, mod_no, type);
		return 0xFF;
	}
	
	sdla_bus_write_4(hw, A600_REG_OFF(SPI_INTERFACE_REG), data);
	WP_DELAY(10);

	wan_set_bit(A600_SPI_REG_START_BIT, &data);
	sdla_bus_write_4(hw, A600_REG_OFF(SPI_INTERFACE_REG), data);
	
		
	for (i=0;i<10;i++) {
		WP_DELAY(10);
		sdla_bus_read_4(hw, A600_REG_OFF(SPI_INTERFACE_REG),&data);
			
		if (!(wan_test_bit(A600_SPI_REG_SPI_BUSY_BIT, &data))) {
			goto spi_read_done;
		}
	}
		
spi_read_done:
	if (wan_test_bit(A600_SPI_REG_SPI_BUSY_BIT, &data)) {
		DEBUG_ERROR("%s: ERROR:SPI Iface not ready\n", hw->devname);
		data = 0xFF;
	}
	
	return (u8) data;
}


u_int8_t sdla_a600_read_fe (void *phw, ...)
{
	sdlahw_t	*hw = (sdlahw_t*)phw;
	va_list		args;
	int		mod_no, type, chain, reg;
	unsigned char	data = 0;

	WAN_ASSERT(hw->magic != SDLADRV_MAGIC);
	va_start(args, phw);
	mod_no	= va_arg(args, int);
	type	= va_arg(args, int);
	chain	= va_arg(args, int);
	reg	= va_arg(args, int);
	va_end(args);

	if (sdla_hw_fe_test_and_set_bit(hw,0)){
		DEBUG_ERROR("%s: %s:%d: Critical Error: Re-entry in FE!\n",
			    hw->devname, __FUNCTION__,__LINE__);
		return 0x00;
	}
	
	data = __sdla_a600_read_fe (hw, mod_no, type, chain, reg);

	sdla_hw_fe_clear_bit(hw,0);
	return data;
}




/***************************************************************************
	Front End FXS/FXO interface for A700
***************************************************************************/

static int __sdla_a700_analog_write_fe (void* phw, ...)
{
	sdlahw_t	*hw = (sdlahw_t*)phw;
	va_list		args;
	int		mod_no, type, chain;
	int		reg, value;
	u32		data = 0;
	unsigned char	cs = 0x00, ctrl_byte = 0x00;
	int		i;

	WAN_ASSERT(hw == NULL);
	WAN_ASSERT(hw->hwcpu == NULL);
	WAN_ASSERT(hw->hwcpu->hwcard == NULL);

	va_start(args, phw);
	mod_no	= va_arg(args, int);
	type	= va_arg(args, int);
	chain	= va_arg(args, int);
	reg	= va_arg(args, int);
	value	= va_arg(args, int);
	va_end(args);
#if 0
	if (!wan_test_bit(mod_no, card->fe.fe_param.remora.module_map)){
		DEBUG_ERROR("%s: %s:%d: Internal Error: Module %d\n",
			card->devname, __FUNCTION__,__LINE__,mod_no);
		return -EINVAL;
	}
#endif
	if(0)DEBUG_RM("%s:%d: Module %d: Write RM FE code (reg %d, value %02X)!\n",
				__FUNCTION__,__LINE__,
				/*FIXME: hw->devname,*/mod_no, reg, (u8)value);
	
	mod_no+=4;

	/* bit 0-7: data byte */
	data = value & 0xFF;
	if (type == MOD_TYPE_FXO){
		/* bit 8-15: register number */
		data |= (reg & 0xFF) << 8;

		/* bit 16-23: chip select byte */
		cs = 0x20;
		cs |= MOD_SPI_CS_FXO_WRITE;
		if (mod_no % 2 == 1){
			/* Select second chip in a chain */
			cs |= MOD_SPI_CS_FXO_CHIP_1;
		}

		data |= (cs & 0xFF) << 16;

		/* bit 24-31: ctrl byte */
		ctrl_byte = mod_no / 2;

		ctrl_byte |= MOD_SPI_CTRL_CHAIN;	/* always chain */
		data |= ctrl_byte << 24;

	}else if (type == MOD_TYPE_FXS || type == MOD_TYPE_TEST){
		/* bit 8-15: register byte */
		reg = reg & 0x7F;
		reg |= MOD_SPI_ADDR_FXS_WRITE; 
		data |= (reg & 0xFF) << 8;
		
		/* bit 16-23: chip select byte */
		if (mod_no % 2 == 0){
			/* Select first chip in a chain */
			cs = A700_ANALOG_SPI_CS_FXS_CHIP_0;
		}else {
			/* Select second chip in a chain */
			cs = A700_ANALOG_SPI_CS_FXS_CHIP_1;
		}
		data |= cs << 16;

		/* bit 24-31: ctrl byte */
		ctrl_byte = mod_no / 2;

		ctrl_byte |= MOD_SPI_CTRL_FXS;
		if (chain){
			ctrl_byte |= MOD_SPI_CTRL_CHAIN;
		}
		data |= ctrl_byte << 24;

	}else{
		DEBUG_EVENT("%s: Module %d: Unsupported module type %d!\n",
				hw->devname, mod_no, type);
		return -EINVAL;
	}

	sdla_bus_write_4(hw, A700_ANALOG_SPI_INTERFACE_REG, data);	

	WP_DELAY(10);
	data |= A700_ANALOG_SPI_MOD_START_BIT;
	
	sdla_bus_write_4(hw, A700_ANALOG_SPI_INTERFACE_REG, data);	

	for (i=0;i<10;i++){	
		WP_DELAY(10);
		sdla_bus_read_4(hw, A700_ANALOG_SPI_INTERFACE_REG, &data);

		if (data & MOD_SPI_BUSY){
			continue;
		}
	}

	if (data & MOD_SPI_BUSY) {
		DEBUG_ERROR("%s: Module %d: Critical Error (%s:%d)!\n",
					hw->devname, mod_no,
					__FUNCTION__,__LINE__);
		return -EINVAL;
	}
        return 0;
}

int sdla_a700_analog_write_fe (void* phw, ...)
{
	sdlahw_t	*hw = (sdlahw_t*)phw;
	va_list		args;
	int		mod_no, type, chain, reg, value;
#if defined(WAN_DEBUG_FE)
	char		*fname;	
	int		fline;
#endif

	WAN_ASSERT(hw->magic != SDLADRV_MAGIC);
	va_start(args, phw);
	mod_no	= va_arg(args, int);
	type	= va_arg(args, int);
	chain	= va_arg(args, int);
	reg	= va_arg(args, int);
	value	= va_arg(args, int);
#if defined(WAN_DEBUG_FE)
	fname	= va_arg(args, char*);
	fline	= va_arg(args, int);
#endif
	va_end(args);

	if (sdla_hw_fe_test_and_set_bit(hw,0)){
#if defined(WAN_DEBUG_FE)
		DEBUG_ERROR("%s: %s:%d: Critical Error: Re-entry in FE (%s:%d)!\n",
			hw->devname, __FUNCTION__,__LINE__, fname, fline);
#else
		DEBUG_ERROR("%s: %s:%d: Critical Error: Re-entry in FE!\n",
			hw->devname, __FUNCTION__,__LINE__);
#endif			
		return -EINVAL;
	}

	DEBUG_REG("%s: Remora Direct Register %d = %02X\n",
			hw->devname, reg, value);
	__sdla_a700_analog_write_fe(hw, (mod_no-4), type, chain, reg, value);

	sdla_hw_fe_clear_bit(hw,0);
        return 0;
}

u_int8_t __sdla_a700_analog_read_fe (void* phw, ...)
{
	sdlahw_t	*hw = (sdlahw_t*)phw;
	va_list		args;
	int		mod_no, type, chain, reg;
	u32		data = 0;
	unsigned char	cs = 0x00, ctrl_byte = 0x00;
	int		i;

	WAN_ASSERT(hw == NULL);
	WAN_ASSERT(hw->hwcpu == NULL);
	WAN_ASSERT(hw->hwcpu->hwcard == NULL);

	va_start(args, phw);
	mod_no	= va_arg(args, int);
	type	= va_arg(args, int);
	chain	= va_arg(args, int);
	reg	= va_arg(args, int);
	va_end(args);
	
	if(0)DEBUG_RM("%s:%d: Module %d: Read RM FE code (reg %d)!\n",
				__FUNCTION__,__LINE__,
				/*FIXME: hw->devname, */mod_no, reg);

	mod_no+=4;

	/* bit 0-7: data byte */
	data = 0x00;
	if (type == MOD_TYPE_FXO){

		/* bit 8-15: register byte */
		data |= (reg & 0xFF) << 8;

		/* bit 16-23: chip select byte */
		cs = 0x20;
		cs |= MOD_SPI_CS_FXO_READ;

		if (mod_no % 2 == 1) {
			/* Select second chip in a chain */
			cs |= MOD_SPI_CS_FXO_CHIP_1;
		}

		data |= (cs & 0xFF) << 16;

		/* bit 24-31: ctrl byte	*/
		ctrl_byte = mod_no / 2;
		ctrl_byte |= MOD_SPI_CTRL_CHAIN;	/* always chain */
		data |= ctrl_byte << 24;

	}else if (type == MOD_TYPE_FXS){

		/* bit 8-15: register byte */
		reg = reg & 0x7F;
		reg |= MOD_SPI_ADDR_FXS_READ; 
		data |= (reg & 0xFF) << 8;
		
		/* bit 16-23: chip select byte */
		if (mod_no % 2 == 0){
			/* Select first chip in a chain */
			cs = A700_ANALOG_SPI_CS_FXS_CHIP_0;
		}else{
			/* Select second chip in a chain */
			cs = A700_ANALOG_SPI_CS_FXS_CHIP_1;
		}
		data |= cs << 16;

		/* bit 24-31: ctrl byte */
		ctrl_byte = mod_no / 2;
		ctrl_byte |= MOD_SPI_CTRL_FXS;
		if (chain){
			ctrl_byte |= MOD_SPI_CTRL_CHAIN;
		} else {
		}
		data |= ctrl_byte << 24;

	}else{
		DEBUG_EVENT("%s: Module %d: Unsupported module type %d!\n",
				hw->devname, mod_no, type);
		return -EINVAL;
	}

	sdla_bus_write_4(hw, A700_ANALOG_SPI_INTERFACE_REG, data);	

	WP_DELAY(10);
	data |= A700_ANALOG_SPI_MOD_START_BIT;
	sdla_bus_write_4(hw, A700_ANALOG_SPI_INTERFACE_REG, data);	

	for (i=0;i<10;i++){
		WP_DELAY(10);
		sdla_bus_read_4(hw, A700_ANALOG_SPI_INTERFACE_REG, &data);
		if (data & MOD_SPI_BUSY) {
			continue;
		}
	}

	if (data & MOD_SPI_BUSY){
		DEBUG_ERROR("%s: Module %d: Critical Error (%s:%d)!\n",
					hw->devname, mod_no,
					__FUNCTION__,__LINE__);
		return 0xFF;
	}

	return (u8)(data & 0xFF);
}

u_int8_t sdla_a700_analog_read_fe (void* phw, ...)
{
	sdlahw_t	*hw = (sdlahw_t*)phw;
	va_list		args;
	int		mod_no, type, chain, reg;
	unsigned char	data = 0;
#if defined(WAN_DEBUG_FE)
	char		*fname;
	int		fline;
#endif

	WAN_ASSERT(hw->magic != SDLADRV_MAGIC);
	va_start(args, phw);
	mod_no	= va_arg(args, int);
	type	= va_arg(args, int);
	chain	= va_arg(args, int);
	reg	= va_arg(args, int);
#if defined(WAN_DEBUG_FE)
	fname	= va_arg(args, char*);
	fline	= va_arg(args, int);
#endif
	va_end(args);

	if (sdla_hw_fe_test_and_set_bit(hw,0)){
#if defined(WAN_DEBUG_FE)
		DEBUG_ERROR("%s: %s:%d: Critical Error: Re-entry in FE (%s:%d)!\n",
			hw->devname, __FUNCTION__,__LINE__,fname,fline);
#else
		DEBUG_ERROR("%s: %s:%d: Critical Error: Re-entry in FE!\n",
			hw->devname, __FUNCTION__,__LINE__);
#endif		
		return 0x00;
	}
	data = __sdla_a700_analog_read_fe (hw, (mod_no-4), type, chain, reg);

	sdla_hw_fe_clear_bit(hw,0);
	return data;
}



/***************************************************************************
	Front End FXS/FXO interface for Shark subtype cards
***************************************************************************/


void sdla_a700_reset_fe (void *fe)
{
	sdla_t  *card;
	
	WAN_ASSERT1(fe == NULL);

	card = (sdla_t*)((sdla_fe_t*)fe)->card;
	
	WAN_ASSERT1(card == NULL);
	card->hw_iface.bus_write_4(card->hw,
								A700_ANALOG_SPI_INTERFACE_REG,
								MOD_SPI_RESET);
	WP_DELAY(1000);
	card->hw_iface.bus_write_4(card->hw,
								A700_ANALOG_SPI_INTERFACE_REG,
								0x00000000);
	WP_DELAY(1000);
}

void sdla_a600_reset_fe (void *fe)  
{
	u32 reg;
	sdla_t *card;
	
	WAN_ASSERT1(fe == NULL);

	card = (sdla_t*)((sdla_fe_t*)fe)->card;

	WAN_ASSERT1(card == NULL);
	
	DEBUG_RM("%s: Resetting SPI\n", ((sdla_fe_t*)fe)->name);
	
	/* Reset FXO */
	reg = 0x00;
	
	/* set reset */
	wan_set_bit(A600_SPI_REG_FX0_RESET_BIT, &reg);
	card->hw_iface.bus_write_4(card->hw,
				   A600_REG_OFF(SPI_INTERFACE_REG),
						   reg);
	
	WP_DELAY(1000);
	
	/* clear reset */
	wan_clear_bit(A600_SPI_REG_FX0_RESET_BIT, &reg);
	card->hw_iface.bus_write_4(card->hw,
				   A600_REG_OFF(SPI_INTERFACE_REG),
						   reg);
	
	WP_DELAY(1000);
	
	/* Reset FXS */
	reg = 0x00;
	wan_set_bit(A600_SPI_REG_FXS_RESET_BIT, &reg);
	card->hw_iface.bus_write_4(card->hw,
				   A600_REG_OFF(SPI_INTERFACE_REG),
						   reg);
	
	WP_DELAY(1000);
	
	/* clear reset */
	wan_clear_bit(A600_SPI_REG_FXS_RESET_BIT, &reg);
	card->hw_iface.bus_write_4(card->hw,
				   A600_REG_OFF(SPI_INTERFACE_REG),
						   reg);
	
	WP_DELAY(1000);
}

void sdla_b800_reset_module(sdla_t *card, int mod_no)
{
	u32 reg;
	u8 remora_no, module_no;
	
	remora_no = mod_no/4;
	module_no = mod_no%4;

	reg = 0x00;
	reg |= (module_no & 0x3) <<  24;
	reg |= (remora_no & 0x3) <<  26;
	reg |= 1<< B800_SPI_REG_RESET_BIT;
	
	card->hw_iface.bus_write_4(card->hw, SPI_INTERFACE_REG, reg);
	
	WP_DELAY(1000);
	card->hw_iface.bus_write_4(card->hw, SPI_INTERFACE_REG,	0x00000000);
	WP_DELAY(1000);
}

void sdla_b800_reset_fe (void *fe)
{
	int i;
	sdla_t *card;

	WAN_ASSERT1(fe == NULL);
	card = (sdla_t*)((sdla_fe_t*)fe)->card;

	WAN_ASSERT1(card == NULL);

	/* reset all modules together for now until we have the ability/need
		reset 1 module at a time from driver core */
	for (i=0;i<NUM_B800_ANALOG_PORTS;i++) {
		sdla_b800_reset_module(card, i);
	}
}


void sdla_a200_reset_fe (void *fe)
{
	sdla_t *card;

	WAN_ASSERT1(fe == NULL);
	card = (sdla_t*)((sdla_fe_t*)fe)->card;

	WAN_ASSERT1(card == NULL);
	
	card->hw_iface.bus_write_4(card->hw,
					SPI_INTERFACE_REG,
     					MOD_SPI_RESET);
	
	WP_DELAY(1000);
	card->hw_iface.bus_write_4(card->hw,
					SPI_INTERFACE_REG,
     					0x00000000);
	WP_DELAY(1000);
}


/*============================================================================
 * Read TE1/56K Front end registers
 */

static int __sdla_b800_write_fe (void* phw, ...)
{
	sdlahw_t	*hw = (sdlahw_t*)phw;
	va_list		args;
	int		mod_no, type, chain;
	u8 module_no, remora_no;
	int		reg, value;
	u32		data = 0;
	//unsigned char	cs = 0x00, ctrl_byte = 0x00;
	int		i;

	WAN_ASSERT(hw == NULL);
	WAN_ASSERT(hw->hwcpu == NULL);
	WAN_ASSERT(hw->hwcpu->hwcard == NULL);

	va_start(args, phw);
	mod_no	= va_arg(args, int);
	type	= va_arg(args, int);
	chain	= va_arg(args, int);
	reg	= va_arg(args, int);
	value	= va_arg(args, int);
	va_end(args);

	if (chain) DEBUG_ERROR ("%s :%d Error: chain mode not supported on A600 (%s:%d)\n",
					hw->devname, mod_no, __FUNCTION__,__LINE__);

	remora_no = mod_no/4;
	module_no = mod_no%4;

	data |= (module_no & 0x3) << 24;
	data |= (remora_no & 0x3) << 26;
	data &= 0xFF000000;

	if (type == MOD_TYPE_FXO) {
		/* Clear data bits */
		data |= (value & 0xFF);
		data |= (reg & 0xFF) << 8;
	} else if (type == MOD_TYPE_FXS) {
		wan_set_bit(B800_SPI_REG_CHAN_TYPE_FXS_BIT, &data);
		/* Clear data bits */
		data |= (value & 0xFF);
		data |= (reg & 0x7F) << 8;
	} else {
		DEBUG_EVENT("%s: Module %d: Unsupported module type %d!\n",
					hw->devname, mod_no, type);
					return -EINVAL;
	}
	
	sdla_bus_write_4(hw, SPI_INTERFACE_REG, data);
		
	WP_DELAY(10);
		
	wan_set_bit(B800_SPI_REG_START_BIT, &data);
	sdla_bus_write_4(hw, SPI_INTERFACE_REG, data);
	
	for (i=0;i<10;i++) {	
		WP_DELAY(10);
		sdla_bus_read_4(hw, SPI_INTERFACE_REG,&data);
		if (!(wan_test_bit(B800_SPI_REG_SPI_BUSY_BIT, &data))) {
			goto spi_write_done;
		}
	}
			
	if (wan_test_bit(B800_SPI_REG_SPI_BUSY_BIT, &data)) {
		DEBUG_ERROR("%s: ERROR:SPI Iface not ready\n", hw->devname);
		return -EINVAL;
	}
		
spi_write_done:
	return 0;
}


int sdla_b800_write_fe(void* phw, ...)
{
	sdlahw_t	*hw = (sdlahw_t*)phw;
	va_list		args;
	int		mod_no, type, chain, reg, value;
#if defined(WAN_DEBUG_FE)
	char		*fname;	
	int		fline;
#endif

	WAN_ASSERT(hw->magic != SDLADRV_MAGIC);
	va_start(args, phw);
	mod_no	= va_arg(args, int);
	type	= va_arg(args, int);
	chain	= va_arg(args, int);
	reg	= va_arg(args, int);
	value	= va_arg(args, int);
#if defined(WAN_DEBUG_FE)
	fname	= va_arg(args, char*);
	fline	= va_arg(args, int);
#endif
	va_end(args);

	if (sdla_hw_fe_test_and_set_bit(hw,0)){
#if defined(WAN_DEBUG_FE)
		DEBUG_ERROR("%s: %s:%d: Critical Error: Re-entry in FE (%s:%d)!\n",
			hw->devname, __FUNCTION__,__LINE__, fname, fline);
#else
		DEBUG_ERROR("%s: %s:%d: Critical Error: Re-entry in FE!\n",
			hw->devname, __FUNCTION__,__LINE__);
#endif			
		return -EINVAL;
	}
	
	DEBUG_REG("%s: Remora Direct Register %d = %02X\n",
			hw->devname, reg, value);

	__sdla_b800_write_fe(hw, mod_no, type, chain, reg, value);

	sdla_hw_fe_clear_bit(hw,0);
	return 0;
}

static int __sdla_shark_rm_write_fe (void* phw, ...)
{
	sdlahw_t	*hw = (sdlahw_t*)phw;
	va_list		args;
	int		mod_no, type, chain;
	int		reg, value;
	u32		data = 0;
	unsigned char	cs = 0x00, ctrl_byte = 0x00;
	int		i;

	WAN_ASSERT(hw == NULL);
	WAN_ASSERT(hw->hwcpu == NULL);
	WAN_ASSERT(hw->hwcpu->hwcard == NULL);

	va_start(args, phw);
	mod_no	= va_arg(args, int);
	type	= va_arg(args, int);
	chain	= va_arg(args, int);
	reg	= va_arg(args, int);
	value	= va_arg(args, int);
	va_end(args);
#if 0
	if (!wan_test_bit(mod_no, card->fe.fe_param.remora.module_map)){
		DEBUG_ERROR("%s: %s:%d: Internal Error: Module %d\n",
			card->devname, __FUNCTION__,__LINE__,mod_no);
		return -EINVAL;
	}
#endif
	if(0)DEBUG_RM("%s:%d: Module %d: Write RM FE code (reg %d, value %02X)!\n",
				__FUNCTION__,__LINE__,
				/*FIXME: hw->devname,*/mod_no, reg, (u8)value);
	
	/* bit 0-7: data byte */
	data = value & 0xFF;
	if (type == MOD_TYPE_FXO){

		/* bit 8-15: register number */
		data |= (reg & 0xFF) << 8;

		/* bit 16-23: chip select byte
		** bit 16
		**
		**
		**			*/
		cs = 0x20;
		cs |= MOD_SPI_CS_FXO_WRITE;
		if (mod_no % 2 == 0){
			/* Select second chip in a chain */
			cs |= MOD_SPI_CS_FXO_CHIP_1;
		}
		data |= (cs & 0xFF) << 16;

		/* bit 24-31: ctrl byte
		** bit 24
		**
		**
		**			*/
		ctrl_byte = mod_no / 2;
#if !defined(SPI2STEP)
		if (hw->hwcpu->hwcard->core_rev > 3){
			ctrl_byte |= MOD_SPI_CTRL_START;
		}else{
			ctrl_byte |= MOD_SPI_CTRL_V3_START;
		}
#endif
		ctrl_byte |= MOD_SPI_CTRL_CHAIN;	/* always chain */
		data |= ctrl_byte << 24;

	}else if (type == MOD_TYPE_FXS){

		/* bit 8-15: register byte */
		reg = reg & 0x7F;
		reg |= MOD_SPI_ADDR_FXS_WRITE; 
		data |= (reg & 0xFF) << 8;
		
		/* bit 16-23: chip select byte
		** bit 16
		**
		**
		**			*/
		if (mod_no % 2){
			/* Select first chip in a chain */
			cs = MOD_SPI_CS_FXS_CHIP_0;
		}else{
			/* Select second chip in a chain */
			cs = MOD_SPI_CS_FXS_CHIP_1;
		}
		data |= cs << 16;

		/* bit 24-31: ctrl byte
		** bit 24
		**
		**
		**			*/
		ctrl_byte = mod_no / 2;
#if !defined(SPI2STEP)
		if (hw->hwcpu->hwcard->core_rev > 3){
			ctrl_byte |= MOD_SPI_CTRL_START;
		}else{
			ctrl_byte |= MOD_SPI_CTRL_V3_START;
		}
#endif
		ctrl_byte |= MOD_SPI_CTRL_FXS;
		if (chain){
			ctrl_byte |= MOD_SPI_CTRL_CHAIN;
		}
		data |= ctrl_byte << 24;

	}else{
		DEBUG_EVENT("%s: Module %d: Unsupported module type %d!\n",
				hw->devname, mod_no, type);
		return -EINVAL;
	}

	sdla_bus_write_4(hw, SPI_INTERFACE_REG, data);	
#if defined(SPI2STEP)
	WP_DELAY(1);
	if (hw->hwcpu->hwcard->core_rev > 3){
		data |= MOD_SPI_START;
	}else{
		data |= MOD_SPI_V3_START;
	}
	sdla_bus_write_4(hw, SPI_INTERFACE_REG, data);	
#endif
#if 0
	DEBUG_EVENT("%s: %s: Module %d - Execute SPI command %08X\n",
					card->fe.name,
					__FUNCTION__,
					mod_no,
					data);
#endif

	for (i=0;i<10;i++){	
		WP_DELAY(10);
		sdla_bus_read_4(hw, SPI_INTERFACE_REG, &data);

		if (data & MOD_SPI_BUSY){
			continue;
		}
	}

	if (data & MOD_SPI_BUSY) {
		DEBUG_ERROR("%s: Module %d: Critical Error (%s:%d)!\n",
					hw->devname, mod_no,
					__FUNCTION__,__LINE__);
		return -EINVAL;
	}
        return 0;
}

int sdla_shark_rm_write_fe (void* phw, ...)
{
	sdlahw_t	*hw = (sdlahw_t*)phw;
	va_list		args;
	int		mod_no, type, chain, reg, value;
#if defined(WAN_DEBUG_FE)
	char		*fname;	
	int		fline;
#endif

	WAN_ASSERT(hw->magic != SDLADRV_MAGIC);
	va_start(args, phw);
	mod_no	= va_arg(args, int);
	type	= va_arg(args, int);
	chain	= va_arg(args, int);
	reg	= va_arg(args, int);
	value	= va_arg(args, int);
#if defined(WAN_DEBUG_FE)
	fname	= va_arg(args, char*);
	fline	= va_arg(args, int);
#endif
	va_end(args);

	if (sdla_hw_fe_test_and_set_bit(hw,0)){
#if defined(WAN_DEBUG_FE)
		DEBUG_ERROR("%s: %s:%d: Critical Error: Re-entry in FE (%s:%d)!\n",
			hw->devname, __FUNCTION__,__LINE__, fname, fline);
#else
		DEBUG_ERROR("%s: %s:%d: Critical Error: Re-entry in FE!\n",
			hw->devname, __FUNCTION__,__LINE__);
#endif			
		return -EINVAL;
	}

	DEBUG_REG("%s: Remora Direct Register %d = %02X\n",
			hw->devname, reg, value);
	__sdla_shark_rm_write_fe(hw, mod_no, type, chain, reg, value);

	sdla_hw_fe_clear_bit(hw,0);
        return 0;
}

u_int8_t __sdla_b800_read_fe (void* phw, ...)
{
	sdlahw_t	*hw = (sdlahw_t*)phw;
	va_list		args;
	int		mod_no, type, chain, reg;
	u8		module_no, remora_no;
	u32		data = 0;
	int		i;

	WAN_ASSERT(hw == NULL);
	WAN_ASSERT(hw->hwcpu == NULL);
	WAN_ASSERT(hw->hwcpu->hwcard == NULL);

	va_start(args, phw);
	mod_no	= va_arg(args, int);
	type	= va_arg(args, int);
	chain	= va_arg(args, int);
	reg	= va_arg(args, int);
	va_end(args);

	remora_no = mod_no/4;
	module_no = mod_no%4;

	if (chain) {
			DEBUG_ERROR ("%s :%d Error: chain mode not supported on B800 (%s:%d)\n", hw->devname, mod_no, __FUNCTION__,__LINE__);
			WARN_ON(1);
	}
	
	wan_set_bit(B800_SPI_REG_READ_ENABLE_BIT, &data);

	data |= (module_no & 0x3) << 24;
	data |= (remora_no & 0x3) << 26;
	data &= 0xFF000000;
	
	if (type == MOD_TYPE_FXO) {
		data |= (reg & 0xFF) << 8;
		data &= 0xFFFFFF00;
	} else if (type == MOD_TYPE_FXS) {
		wan_set_bit(B800_SPI_REG_CHAN_TYPE_FXS_BIT, &data);			
		data |= (reg & 0x7F) << 8;
		data &= 0xFFFFFF00;
	} else {
		DEBUG_EVENT("%s: Module %d: Unsupported module type %d!\n",
					hw->devname, mod_no, type);
		return 0xFF;
	}
	
	sdla_bus_write_4(hw, SPI_INTERFACE_REG, data);
	WP_DELAY(10);

	wan_set_bit(B800_SPI_REG_START_BIT, &data);
	sdla_bus_write_4(hw, SPI_INTERFACE_REG, data);
	
		
	for (i=0;i<10;i++) {
		WP_DELAY(10);
		sdla_bus_read_4(hw, SPI_INTERFACE_REG,&data);
			
		if (!(wan_test_bit(B800_SPI_REG_SPI_BUSY_BIT, &data))) {
			goto spi_read_done;
		}
	}
		
spi_read_done:
	if (wan_test_bit(B800_SPI_REG_SPI_BUSY_BIT, &data)) {
		DEBUG_ERROR("%s: ERROR:SPI Iface not ready\n", hw->devname);
		data = 0xFF;
	}
		
	return (u8)(data & 0xFF);
}

u_int8_t sdla_b800_read_fe (void* phw, ...)
{
	sdlahw_t	*hw = (sdlahw_t*)phw;
	va_list		args;
	int		mod_no, type, chain, reg;
	unsigned char	data = 0;
#if defined(WAN_DEBUG_FE)
	char		*fname;
	int		fline;
#endif

	WAN_ASSERT(hw->magic != SDLADRV_MAGIC);
	va_start(args, phw);
	mod_no	= va_arg(args, int);
	type	= va_arg(args, int);
	chain	= va_arg(args, int);
	reg	= va_arg(args, int);
#if defined(WAN_DEBUG_FE)
	fname	= va_arg(args, char*);
	fline	= va_arg(args, int);
#endif
	va_end(args);

	if (sdla_hw_fe_test_and_set_bit(hw,0)){
#if defined(WAN_DEBUG_FE)
		DEBUG_ERROR("%s: %s:%d: Critical Error: Re-entry in FE (%s:%d)!\n",
			hw->devname, __FUNCTION__,__LINE__,fname,fline);
#else
		DEBUG_ERROR("%s: %s:%d: Critical Error: Re-entry in FE!\n",
			hw->devname, __FUNCTION__,__LINE__);
#endif		
		return 0x00;
	}
	data = __sdla_b800_read_fe (hw, mod_no, type, chain, reg);
	//DEBUG_REG("%s: Remora Read Reg %X = %02X\n",
	//		hw->devname, reg, data);

	sdla_hw_fe_clear_bit(hw,0);
	return data;
}

u_int8_t __sdla_shark_rm_read_fe (void* phw, ...)
{
	sdlahw_t	*hw = (sdlahw_t*)phw;
	va_list		args;
	int		mod_no, type, chain, reg;
	u32		data = 0;
	unsigned char	cs = 0x00, ctrl_byte = 0x00;
	int		i;

	WAN_ASSERT(hw == NULL);
	WAN_ASSERT(hw->hwcpu == NULL);
	WAN_ASSERT(hw->hwcpu->hwcard == NULL);

	va_start(args, phw);
	mod_no	= va_arg(args, int);
	type	= va_arg(args, int);
	chain	= va_arg(args, int);
	reg	= va_arg(args, int);
	va_end(args);
#if 0
	if (!wan_test_bit(mod_no, card->fe.fe_param.remora.module_map)){
		DEBUG_ERROR("%s: %s:%d: Internal Error: Module %d\n",
			card->devname, __FUNCTION__,__LINE__,mod_no);
		return 0x00;
	}
#endif
	if(0)DEBUG_RM("%s:%d: Module %d: Read RM FE code (reg %d)!\n",
				__FUNCTION__,__LINE__,
				/*FIXME: hw->devname, */mod_no, reg);

	/* bit 0-7: data byte */
	data = 0x00;
	if (type == MOD_TYPE_FXO){

		/* bit 8-15: register byte */
		data |= (reg & 0xFF) << 8;

		/* bit 16-23: chip select byte
		** bit 16
		**
		**
		**			*/
		cs = 0x20;
		cs |= MOD_SPI_CS_FXO_READ;
		if (mod_no % 2 == 0){
			/* Select second chip in a chain */
			cs |= MOD_SPI_CS_FXO_CHIP_1;
		}
		data |= (cs & 0xFF) << 16;

		/* bit 24-31: ctrl byte
		** bit 24
		**
		**
		**			*/
		ctrl_byte = mod_no / 2;
#if !defined(SPI2STEP)
		if (hw->hwcpu->hwcard->core_rev > 3){
			ctrl_byte |= MOD_SPI_CTRL_START;
		}else{
			ctrl_byte |= MOD_SPI_CTRL_V3_START;
		}
#endif
		ctrl_byte |= MOD_SPI_CTRL_CHAIN;	/* always chain */
		data |= ctrl_byte << 24;

	}else if (type == MOD_TYPE_FXS){

		/* bit 8-15: register byte */
		reg = reg & 0x7F;
		reg |= MOD_SPI_ADDR_FXS_READ; 
		data |= (reg & 0xFF) << 8;
		
		/* bit 16-23: chip select byte
		** bit 16
		**
		**
		**			*/
		if (mod_no % 2){
			/* Select first chip in a chain */
			cs = MOD_SPI_CS_FXS_CHIP_0;
		}else{
			/* Select second chip in a chain */
			cs = MOD_SPI_CS_FXS_CHIP_1;
		}
		data |= cs << 16;

		/* bit 24-31: ctrl byte
		** bit 24
		**
		**
		**			*/
		ctrl_byte = mod_no / 2;
#if !defined(SPI2STEP)
		if (hw->hwcpu->hwcard->core_rev > 3){
			ctrl_byte |= MOD_SPI_CTRL_START;
		}else{
			ctrl_byte |= MOD_SPI_CTRL_V3_START;
		}
#endif
		ctrl_byte |= MOD_SPI_CTRL_FXS;
		if (chain){
			ctrl_byte |= MOD_SPI_CTRL_CHAIN;
		}
		data |= ctrl_byte << 24;

	}else{
		DEBUG_EVENT("%s: Module %d: Unsupported module type %d!\n",
				hw->devname, mod_no, type);
		return -EINVAL;
	}

	sdla_bus_write_4(hw, SPI_INTERFACE_REG, data);	
#if defined(SPI2STEP)
	WP_DELAY(1);
	if (hw->hwcpu->hwcard->core_rev > 3){
		data |= MOD_SPI_START;
	}else{
		data |= MOD_SPI_V3_START;
	}
	sdla_bus_write_4(hw, SPI_INTERFACE_REG, data);	
#endif
#if 0
	DEBUG_EVENT("%s: %s: Module %d - Execute SPI command %08X\n",
					hw->devname,
					__FUNCTION__,
					mod_no,
					data);
#endif
	for (i=0;i<10;i++){
		WP_DELAY(10);
		sdla_bus_read_4(hw, SPI_INTERFACE_REG, &data);
		if (data & MOD_SPI_BUSY) {
			continue;
		}
	}

	if (data & MOD_SPI_BUSY){
		DEBUG_ERROR("%s: Module %d: Critical Error (%s:%d)!\n",
					hw->devname, mod_no,
					__FUNCTION__,__LINE__);
		return 0xFF;
	}

	return (u8)(data & 0xFF);
}

u_int8_t sdla_shark_rm_read_fe (void* phw, ...)
{
	sdlahw_t	*hw = (sdlahw_t*)phw;
	va_list		args;
	int		mod_no, type, chain, reg;
	unsigned char	data = 0;
#if defined(WAN_DEBUG_FE)
	char		*fname;
	int		fline;
#endif

	WAN_ASSERT(hw->magic != SDLADRV_MAGIC);
	va_start(args, phw);
	mod_no	= va_arg(args, int);
	type	= va_arg(args, int);
	chain	= va_arg(args, int);
	reg	= va_arg(args, int);
#if defined(WAN_DEBUG_FE)
	fname	= va_arg(args, char*);
	fline	= va_arg(args, int);
#endif
	va_end(args);

	if (sdla_hw_fe_test_and_set_bit(hw,0)){
#if defined(WAN_DEBUG_FE)
		DEBUG_ERROR("%s: %s:%d: Critical Error: Re-entry in FE (%s:%d)!\n",
			hw->devname, __FUNCTION__,__LINE__,fname,fline);
#else
		DEBUG_ERROR("%s: %s:%d: Critical Error: Re-entry in FE!\n",
			hw->devname, __FUNCTION__,__LINE__);
#endif		
		return 0x00;
	}
	data = __sdla_shark_rm_read_fe (hw, mod_no, type, chain, reg);
	//DEBUG_REG("%s: Remora Read Reg %X = %02X\n",
	//		hw->devname, reg, data);

	sdla_hw_fe_clear_bit(hw,0);
	return data;
}


/***************************************************************************
	ISDN BRI Front End interface
***************************************************************************/
#define SPI_DELAY if(1)WP_DELAY(10)

#undef SPI_MAX_RETRY_COUNT
#define SPI_MAX_RETRY_COUNT 10000

#define SPI_BREAK	if(1)break
#define FAST_SPI	0
/*============================================================================
 * Write ISDN BRI Front end registers
 */
static 
int 
__write_bri_fe_byte (
	void* phw, 
	u_int8_t mod_no, 
	u_int8_t addr, 
	u_int8_t value)
{
	sdlahw_t	*hw = (sdlahw_t*)phw;
	bri_reg_t	data, dummy;
	u_int32_t	*data_ptr = (u_int32_t*)&data;
	u_int32_t	*dummy_ptr = (u_int32_t*)&dummy;
	u_int32_t	retry_counter;
	u_int8_t	rm_no=0xFF;

	WAN_ASSERT(hw == NULL);

	DEBUG_REG("%s():%s: mod_no:%d addr=0x%X (%d) value=0x%02X\n",
		__FUNCTION__,
		hw->devname,
		mod_no,
		addr,
		addr,
		value);

	/*	Input mod_no is an even number between 0 and 22 (including).
		Calculate rm_no - should be between 0 and 3 (including). 
	*/
	rm_no = mod_no / (MAX_BRI_MODULES_PER_REMORA*2);
#if 0
	DEBUG_BRI("%s(input): rm_no: %d, mod_no: %d\n", __FUNCTION__, rm_no, mod_no);
#endif
	/* Translate mod_no to be between 0 and 2 (including) */
	mod_no = (mod_no / 2) % MAX_BRI_MODULES_PER_REMORA;
#if 0
	DEBUG_BRI("%s(updated): rm_no: %d, mod_no: %d\n", __FUNCTION__, rm_no, mod_no);
#endif
	if(rm_no > MAX_BRI_REMORAS - 1){
		DEBUG_EVENT("%s:%s(): Line:%d: invalid rm_no: %d!!(mod_no: %d)\n", 
			hw->devname, __FUNCTION__, __LINE__, rm_no, mod_no);	
		return 0;
	}

	/* setup address offset for fe */
	data.reset = 0;
	data.start = 1;
	data.reserv1 = 0;

	data.remora_addr = rm_no;
	data.mod_addr = mod_no;

	data.data = addr;
	data.contrl = 0;
	data.contrl |= ADDR_BIT;
	
	/* DEBUG_BRI("1. data: 0x%08X\n", *data_ptr); */
	/* check spi not busy */
	for (retry_counter = 0; retry_counter < SPI_MAX_RETRY_COUNT; retry_counter++){
		sdla_bus_read_4(hw, SPI_INTERFACE_REG, dummy_ptr);
		if(dummy.start == 1){
			SPI_DELAY;
		} else {
			SPI_BREAK;
		}
	}
	SPI_DELAY;
	if(dummy.start == 1){
		DEBUG_EVENT("%s:%s(): Line:%d: SPI TIMEOUT!!\n", hw->devname, __FUNCTION__, __LINE__);	
		return 0;
	}	

	sdla_bus_write_4(hw, SPI_INTERFACE_REG, *data_ptr);

	/* start read spi operation */
	data.reset=0;
	data.start=1;
	data.reserv1=0;

	data.remora_addr = rm_no;
	data.mod_addr = mod_no;

	data.data = value;
	data.contrl = 0;
	
	/* DEBUG_BRI("2. data: 0x%08X\n", *data_ptr); */
#if FAST_SPI
	SPI_DELAY;
#else
	/* wait for end of spi operation */
	for (retry_counter = 0; retry_counter < SPI_MAX_RETRY_COUNT; retry_counter++){
		sdla_bus_read_4(hw, SPI_INTERFACE_REG, dummy_ptr);
		if(dummy.start == 1){SPI_DELAY;}else{SPI_BREAK;}
	}
	SPI_DELAY;
	if(dummy.start == 1){
		DEBUG_EVENT("%s:%s(): Line:%d: SPI TIMEOUT!!\n", hw->devname, __FUNCTION__, __LINE__);	
		return 0;
	}	
#endif

	/* write the actual data */
	sdla_bus_write_4(hw, SPI_INTERFACE_REG, *data_ptr);
	return 0;
}

int sdla_shark_bri_write_fe (void* phw, ...)
{
	sdlahw_t	*hw = (sdlahw_t*)phw;
	va_list		args;
	int		mod_no, reg, value;
#if defined(WAN_DEBUG_FE)
	char		*fname;	
	int		fline;
#endif
	va_start(args, phw);
	mod_no	= va_arg(args, int);
	reg	= va_arg(args, int);
	value	= va_arg(args, int);
#if defined(WAN_DEBUG_FE)
	fname	= va_arg(args, char*);
	fline	= va_arg(args, int);
#endif
	va_end(args);

	if (sdla_hw_fe_test_and_set_bit(hw,0)){
#if defined(WAN_DEBUG_FE)
		DEBUG_ERROR("%s: %s:%d: Critical Error: Re-entry in FE (%s:%d)!\n",
			hw->devname, __FUNCTION__,__LINE__, fname, fline);
#else
		DEBUG_ERROR("%s: %s:%d: Critical Error: Re-entry in FE!\n",
			hw->devname, __FUNCTION__,__LINE__);
#endif			
		return -EINVAL;
	}

	__write_bri_fe_byte(hw, (u8)mod_no, (u8)reg, (u8)value);

	sdla_hw_fe_clear_bit(hw,0);
	return 0;
}

/*============================================================================
 * Read ISDN BRI Front end registers
 */
static 
u_int8_t
__read_bri_fe_byte(
	void* phw, 
	u_int8_t mod_no, 
	u_int8_t reg,
	u_int8_t type, 
	u_int8_t optional_arg)
{
	sdlahw_t	*hw = (sdlahw_t*)phw;
	bri_reg_t	data, dummy;
	u_int32_t	*data_ptr = (u_int32_t*)&data;
	u_int32_t	*dummy_ptr = (u_int32_t*)&dummy;
	u_int32_t	retry_counter;
	u_int8_t	rm_no=0xFF;
	u_int8_t	value;
	
	WAN_ASSERT(hw == NULL);

	DEBUG_REG("%s():%s\n", __FUNCTION__, hw->devname);

	if(type != MOD_TYPE_NT && type != MOD_TYPE_TE){
		DEBUG_BRI("%s(): Warning: unknown module type! (%d)\n", __FUNCTION__, type);
	}

	/* setup address offset for fe */
	data.reset  = 0;
	data.start  = 1;
	data.reserv1= 0;

	if(type == MOD_TYPE_NONE){
		/* the only case we get here is if running module detection code. */
		rm_no = optional_arg;
	}else{
		/* Input mod_no is an even number between 0 and 22 (including).
		   Calculate rm_no - should be between 0 and 3 (including). */
		if(mod_no % 2){
			DEBUG_BRI("%s(): Warning: module number (%d) is not even!!\n",
				__FUNCTION__, mod_no);
		}
		rm_no = mod_no / (MAX_BRI_MODULES_PER_REMORA*2);
#if 0
		DEBUG_BRI("%s(input): rm_no: %d, mod_no: %d\n", __FUNCTION__, rm_no, mod_no);
#endif
		/* Translate mod_no to be between 0 and 2 (including) */
		mod_no = (mod_no / 2) % MAX_BRI_MODULES_PER_REMORA;
	}
#if 0
	DEBUG_BRI("%s(updated): rm_no: %d, mod_no: %d\n", __FUNCTION__, rm_no, mod_no);
#endif
	if(rm_no > MAX_BRI_REMORAS - 1){
		DEBUG_EVENT("%s:%s(): Line:%d: invalid rm_no: %d!!(mod_no: %d)\n",
			hw->devname, __FUNCTION__, __LINE__,	rm_no, mod_no);	
		return 0;
	}

	data.remora_addr = rm_no;
	data.mod_addr = mod_no;

	data.data = reg;
	data.contrl = 0;
	data.contrl |= ADDR_BIT;

	if(type == MOD_TYPE_NONE){
		/* DavidR (April 10, 2008): for module detection set 512 khz bit */
		data.contrl |= CPLD_USE_512KHZ_RECOVERY_CLOCK_BIT;
	}

	/* check spi not busy */
	for (retry_counter = 0; retry_counter < SPI_MAX_RETRY_COUNT; retry_counter++){
		sdla_bus_read_4(hw, SPI_INTERFACE_REG, dummy_ptr);
		if(dummy.start == 1){SPI_DELAY;}else{SPI_BREAK;}
	}
	if(dummy.start == 1){
		DEBUG_EVENT("%s:%s(): Line:%d: SPI TIMEOUT!!\n", hw->devname, __FUNCTION__, __LINE__);	
		return 0;
	}	

	/* DEBUG_BRI("1. data: 0x%08X\n", *data_ptr); */	
	/* setup the address */
	sdla_bus_write_4(hw, SPI_INTERFACE_REG, *data_ptr);	

	/* DEBUG_BRI("2. data: 0x%08X\n", *data_ptr); */
	/* wait for end of spi operation */
#if !FAST_SPI
	for (retry_counter = 0; retry_counter < SPI_MAX_RETRY_COUNT; retry_counter++){
		sdla_bus_read_4(hw, SPI_INTERFACE_REG, dummy_ptr);
		if(dummy.start == 1){SPI_DELAY;}else{SPI_BREAK;}
	}
	SPI_DELAY;
	if(dummy.start == 1){
		DEBUG_EVENT("%s:%s(): Line:%d: SPI TIMEOUT!!\n", hw->devname, __FUNCTION__, __LINE__);	
		return 0;
	}	
#endif

	/* setup data for read spi operation  */
	data.reset=0;
	data.start=1;
	data.reserv1=0;

	data.remora_addr = rm_no;
	data.mod_addr = mod_no;

	data.data = 0;
	data.contrl = 0;
	data.contrl |= READ_BIT;

	if(type == MOD_TYPE_NONE){
		/* DavidR (April 10, 2008): for module detection set 512 khz bit */
		data.contrl |= CPLD_USE_512KHZ_RECOVERY_CLOCK_BIT;
	}

	DEBUG_REG("%s(Line: %i): (data: 0x%08X) reset: 0x%X, start: 0x%X, reserv1: 0x%X, remora_addr: 0x%X, mod_addr: 0x%X, data: 0x%X, contrl: 0x%X\n",
		__FUNCTION__, __LINE__, *((u32*)&data), data.reset, data.start, data.reserv1, data.remora_addr, data.mod_addr, data.data, data.contrl);

#if FAST_SPI
	SPI_DELAY;
#else
	for (retry_counter = 0; retry_counter < SPI_MAX_RETRY_COUNT; retry_counter++){
		sdla_bus_read_4(hw, SPI_INTERFACE_REG, dummy_ptr);
		if(dummy.start == 1){SPI_DELAY;}else{SPI_BREAK;}
	}
	SPI_DELAY;
	if(dummy.start == 1){
		DEBUG_EVENT("%s:%s(): Line:%d: SPI TIMEOUT!!\n", hw->devname, __FUNCTION__, __LINE__);	
		return 0;
	}	
#endif
	/*  start read spi operation */
	sdla_bus_write_4(hw, SPI_INTERFACE_REG, *data_ptr);
	
#if FAST_SPI
	SPI_DELAY;
#else
	/* wait for end of spi operation */	
	for (retry_counter = 0; retry_counter < SPI_MAX_RETRY_COUNT; retry_counter++){
		sdla_bus_read_4(hw, SPI_INTERFACE_REG, data_ptr);
		if(data.start == 1){ SPI_DELAY; }else{ SPI_BREAK; }
	}
	SPI_DELAY;
	if(data.start == 1){
		DEBUG_EVENT("%s:%s(): Line:%d: SPI TIMEOUT!!\n", hw->devname, __FUNCTION__, __LINE__);	
		return 0;
	}
#endif	
	//DEBUG_BRI("3. data: 0x%08X\n", *data_ptr);

	value = (u_int8_t)data.data;

	DEBUG_REG("%s(Line: %i): (data: 0x%08X) reset: 0x%X, start: 0x%X, reserv1: 0x%X, remora_addr: 0x%X, mod_addr: 0x%X, data: 0x%X, contrl: 0x%X\n",
		__FUNCTION__, __LINE__, *((u32*)&data), data.reset, data.start, data.reserv1, data.remora_addr, data.mod_addr, data.data, data.contrl);

	DEBUG_REG("%s():%s: mod_no:%d reg=0x%X (%d) value=0x%02X\n",
		__FUNCTION__,
		hw->devname,
		mod_no,
		reg,
		reg,
		value);
	return value;
}

u_int8_t sdla_shark_bri_read_fe (void* phw, ...)
{
	sdlahw_t	*hw = (sdlahw_t*)phw;
	va_list		args;
	int		mod_no, type, optional_arg, reg;
	u_int8_t	data = 0;
#if defined(WAN_DEBUG_FE)
	char		*fname;
	int		fline;
#endif

	va_start(args, phw);
	mod_no		= va_arg(args, int);
	type		= va_arg(args, int);
	optional_arg	= va_arg(args, int);
	reg		= va_arg(args, int);
#if defined(WAN_DEBUG_FE)
	fname		= va_arg(args, char*);
	fline		= va_arg(args, int);
#endif
	va_end(args);

	if (sdla_hw_fe_test_and_set_bit(hw,0)){
#if defined(WAN_DEBUG_FE)
		DEBUG_ERROR("%s: %s:%d: Critical Error: Re-entry in FE (%s:%d)!\n",
			hw->devname, __FUNCTION__,__LINE__,fname,fline);
#else
		DEBUG_ERROR("%s: %s:%d: Critical Error: Re-entry in FE!\n",
			hw->devname, __FUNCTION__,__LINE__);
#endif		
		return 0x00;
	}

	data = __read_bri_fe_byte (hw, (u8)mod_no, (u8)reg, (u8)type, (u8)optional_arg);

	sdla_hw_fe_clear_bit(hw,0);
	return data;
}

/***************************************************************************
	Front End AFT Serial interface for Shark subtype cards
***************************************************************************/
#define AFT_SERIAL_FE_INTERFACE_ADDR	0x210
#define AFT_SERIAL_PORT_REG(line,reg)	(reg+(0x4000*line))
/*============================================================================
 * Read AFT Serial Front end registers
 */
static int __sdla_shark_serial_write_fe (void *phw, ...)
{
	sdlahw_t*	hw = (sdlahw_t*)phw;
	va_list		args;
	unsigned int	addr;
	int		line_no, off, value;

	WAN_ASSERT(hw == NULL);
	WAN_ASSERT(hw->hwcpu == NULL);
	WAN_ASSERT(hw->hwcpu->hwcard == NULL);
	va_start(args, phw);
	line_no = (int)va_arg(args, int);
	off	= (int)va_arg(args, int);
	value	= (int)va_arg(args, int);
	va_end(args);

	addr = AFT_SERIAL_PORT_REG(line_no,AFT_SERIAL_FE_INTERFACE_ADDR);
	sdla_bus_write_4(hw, addr, value);
        return 0;
}

int sdla_shark_serial_write_fe (void *phw, ...)
{
	sdlahw_t*	hw = (sdlahw_t*)phw;
	va_list		args;
	int		line_no, off, value;

	if (sdla_hw_fe_test_and_set_bit(hw,0)){
		if (WAN_NET_RATELIMIT()){
			DEBUG_EVENT(
			"%s: %s:%d: Critical Error: Re-entry in FE!\n",
					hw->devname,
					__FUNCTION__,__LINE__);
		}
		return -EINVAL;
	}

	va_start(args, phw);
	line_no = (int)va_arg(args, int);
	off	= (int)va_arg(args, int);
	value	= (int)va_arg(args, int);
	va_end(args);

	__sdla_shark_serial_write_fe(hw, line_no, off, value);

	sdla_hw_fe_clear_bit(hw,0);
        return 0;
}

/*============================================================================
 * Read AFT Serial Front end registers
 */
u_int32_t __sdla_shark_serial_read_fe (void *phw, ...)
{
	sdlahw_t*	hw = (sdlahw_t*)phw;
	va_list		args;
	unsigned int	addr;
	int		off, line_no, value;

	WAN_ASSERT(hw == NULL);
	WAN_ASSERT(hw->hwcpu == NULL);
	WAN_ASSERT(hw->hwcpu->hwcard == NULL);
	va_start(args, phw);
	line_no = (int)va_arg(args, int);
	off	= (int)va_arg(args, int);
	va_end(args);

	addr = AFT_SERIAL_PORT_REG(line_no,AFT_SERIAL_FE_INTERFACE_ADDR);
	sdla_bus_read_4(hw, addr, (u32*)&value);
	return value;
}

u_int32_t sdla_shark_serial_read_fe(void *phw, ...)
{
	sdlahw_t*	hw = (sdlahw_t*)phw;
	va_list		args;
	int		line_no, off, value;

	if (sdla_hw_fe_test_and_set_bit(hw,0)){
		if (WAN_NET_RATELIMIT()){
		DEBUG_ERROR("%s: %s:%d: Critical Error: Re-entry in FE!\n",
			hw->devname, __FUNCTION__,__LINE__);
		}
		return 0x00;
	}

	va_start(args, phw);
	line_no = va_arg(args, int);
	off	= va_arg(args, int);
	va_end(args);

	value = __sdla_shark_serial_read_fe(hw, line_no, off);

	sdla_hw_fe_clear_bit(hw,0);
        return value;
}

#if defined(CONFIG_PRODUCT_WANPIPE_AFT_B601)
static int __sdla_b601_te1_write_fe(void *phw, ...)
{
    sdlahw_t*   hw = (sdlahw_t*)phw;
    sdlahw_cpu_t    *hwcpu;
    sdlahw_card_t   *hwcard;
    va_list     args;
    int     qaccess=0, line_no=0, off=0, value=0;
    u16 data_hi, data_lo;

    WAN_ASSERT(hw == NULL);
    WAN_ASSERT(hw->hwcpu == NULL);
    WAN_ASSERT(hw->hwcpu->hwcard == NULL);
    hwcpu = hw->hwcpu;
    hwcard = hwcpu->hwcard;
    va_start(args, phw);
    qaccess = (u_int16_t)va_arg(args, int);
    line_no = (u_int16_t)va_arg(args, int);
    off = (u_int16_t)va_arg(args, int);
    value   = (u_int8_t)va_arg(args, int);
    va_end(args);

    data_hi = 0x0000;
    data_lo = 0x0000;

    if (off & 0x800) {
        data_hi |= 0x2000;
    }
    if (off & 0x1000) {
        data_hi |= 0x4000;
    }

	SDLA_HW_T1E1_FE_ACCESS_BLOCK_A600;

    data_hi |= (off & 0x7FF);
    sdla_bus_write_2(hw, A600_MAXIM_INTERFACE_REG_ADD_HI, data_hi);
	
	SDLA_HW_T1E1_FE_ACCESS_BLOCK_A600;
    
	data_lo = (value & 0xFF);
    sdla_bus_write_2(hw, A600_MAXIM_INTERFACE_REG_ADD_LO, data_lo);
	
	SDLA_HW_T1E1_FE_ACCESS_BLOCK_A600;

    return 0;
}

int sdla_b601_te1_write_fe(void *phw, ...)
{
    sdlahw_t    *hw = (sdlahw_t*)phw;
    va_list     args;
    int     mod_no, type, chain, reg, value;
#if defined(WAN_DEBUG_FE)
    char        *fname;
    int     fline;
#endif

    WAN_ASSERT(hw->magic != SDLADRV_MAGIC);
    va_start(args, phw);
    mod_no  = va_arg(args, int);
    type    = va_arg(args, int);
    chain   = va_arg(args, int);
    reg = va_arg(args, int);
    value   = va_arg(args, int);
    va_end(args);

    if (sdla_hw_fe_test_and_set_bit(hw,0)){
        DEBUG_EVENT("%s: %s:%d: Critical Error: Re-entry in FE!\n",
                hw->devname, __FUNCTION__,__LINE__);
        return -EINVAL;
    }

    __sdla_b601_te1_write_fe(hw, mod_no, type, chain, reg, value);

    sdla_hw_fe_clear_bit(hw,0);
    return 0;
}

u_int8_t __sdla_b601_te1_read_fe (void *phw, ...)
{
    sdlahw_t*   hw = (sdlahw_t*)phw;
    sdlahw_cpu_t    *hwcpu;
    sdlahw_card_t   *hwcard;
    va_list     args;
    int     qaccess=0, line_no=0, off=0;
    u32     data_read;
    u16     data_hi;

    WAN_ASSERT(hw == NULL);
    WAN_ASSERT(hw->hwcpu == NULL);
    WAN_ASSERT(hw->hwcpu->hwcard == NULL);
    hwcpu = hw->hwcpu;
    hwcard = hwcpu->hwcard;
    va_start(args, phw);
    qaccess = (u_int16_t)va_arg(args, int);
    line_no = (u_int16_t)va_arg(args, int);
    off = (u_int16_t)va_arg(args, int);
    va_end(args);
    WAN_ASSERT(qaccess != 0 && qaccess != 1);

    data_hi = 0x00;
    if (off & 0x800) {
        data_hi |= 0x2000;
    }
    if (off & 0x1000) {
        data_hi |= 0x4000;
    }

	SDLA_HW_T1E1_FE_ACCESS_BLOCK_A600;
    
	data_hi |= (off & 0x7FF);
    sdla_bus_write_2(hw, A600_MAXIM_INTERFACE_REG_ADD_HI, data_hi);
	
	SDLA_HW_T1E1_FE_ACCESS_BLOCK_A600;
    
	data_read = 0x00;
    sdla_bus_read_4(hw, A600_MAXIM_INTERFACE_REG_ADD_LO, &data_read);
	
	SDLA_HW_T1E1_FE_ACCESS_BLOCK_A600;

    return (data_read & 0xFF);
}

u_int8_t sdla_b601_te1_read_fe (void *phw, ...)
{
    sdlahw_t    *hw = (sdlahw_t*)phw;
    va_list     args;
    int     mod_no, type, chain, reg;
    unsigned char   data = 0;

    WAN_ASSERT(hw->magic != SDLADRV_MAGIC);
    va_start(args, phw);
    mod_no  = va_arg(args, int);
    type    = va_arg(args, int);
    chain   = va_arg(args, int);
    reg = va_arg(args, int);
    va_end(args);

    if (sdla_hw_fe_test_and_set_bit(hw,0)){
        DEBUG_EVENT("%s: %s:%d: Critical Error: Re-entry in FE!\n",
                hw->devname, __FUNCTION__,__LINE__);
        return 0x00;
    }

    data = __sdla_b601_te1_read_fe (hw, mod_no, type, chain, reg);

    sdla_hw_fe_clear_bit(hw,0);
    return data;
}

#endif

void sdla_w400_reset_fe (void *fe)  
{
	sdla_t *card = NULL;
	card = (sdla_t*)((sdla_fe_t*)fe)->card;
	DEBUG_ERROR ("%s I should not be called! (%s:%d)\n", ((sdla_fe_t*)fe)->name, __FUNCTION__, __LINE__);
}

int sdla_w400_write_fe (void *phw, ...)
{
	sdlahw_t *hw = (sdlahw_t*)phw;
	DEBUG_ERROR ("%s I should not be called! (%s:%d)\n", hw->devname, __FUNCTION__, __LINE__);
	return 0xFF;
}

u_int8_t __sdla_w400_read_fe (void *phw, ...)
{
	sdlahw_t *hw = (sdlahw_t*)phw;
	DEBUG_ERROR ("%s I should not be called! (%s:%d)\n", hw->devname, __FUNCTION__, __LINE__);
	return 0xFF;
}

u_int8_t sdla_w400_read_fe (void *phw, ...)
{
	sdlahw_t *hw = (sdlahw_t*)phw;
	DEBUG_ERROR ("%s I should not be called! (%s:%d)\n", hw->devname, __FUNCTION__, __LINE__);
	return 0xFF;
}

