/***************************************************************************
 * sdla_ec.c	WANPIPE(tm) Multiprotocol WAN Link Driver. 
 *				TDM voice hardware echo cancler.
 *
 * Author: 	Alex Feldman   <al.feldman@sangoma.com>
 *
 *
 * Copyright:	(c) 1995-2005 Sangoma Technologies Inc.
 *
 *		This program is free software; you can redistribute it and/or
 *		modify it under the terms of the GNU General Public License
 *		as published by the Free Software Foundation; either version
 *		2 of the License, or (at your option) any later version.
 * ============================================================================
 * Aug 25, 2005	Alex Feldman	Initial version.
 ******************************************************************************
 */
/*******************************************************************************
**			  INCLUDE FILES
*******************************************************************************/


#include "wanpipe_includes.h"
#include "wanpipe_defines.h"
#include "wanpipe_debug.h"
#include "wanpipe_common.h"
#include "wanpipe.h"

#if defined(WAN_OCT6100_DAEMON)
# include "sdla_ec.h"
#endif

#if defined (__FreeBSD__) || defined(__OpenBSD__)
# include <sdla_cdev.h>
#endif

#define WAN_OCT6100_MOD

extern void aft_fe_intr_ctrl(sdla_t *card, int status);

/*******************************************************************************
**			  DEFINES AND MACROS
*******************************************************************************/
#define WAN_OCT6100_READ_LIMIT	0x10000


/*****************************************************************************
**			STRUCTURES AND TYPEDEFS
*****************************************************************************/



/*****************************************************************************
**			   GLOBAL VARIABLES
*****************************************************************************/
#if defined(WAN_OCT6100_DAEMON)
static int wan_ec_no = 0;
WAN_LIST_HEAD(wan_ec_head_, wan_ec_) wan_ec_head = 
		WAN_LIST_HEAD_INITIALIZER(wan_ec_head);
#endif

/*****************************************************************************
**			  FUNCTION PROTOTYPES
*****************************************************************************/
#if defined(WAN_OCT6100_DAEMON)
static int wan_ec_reset(wan_ec_dev_t*, sdla_t*, int);
static int wan_ec_action(void *pcard, int enable, int channel);
#endif

extern unsigned char	aft_read_cpld(sdla_t*, unsigned short);
extern int		aft_write_cpld(void*, unsigned short,unsigned char);

extern void *wanpipe_eclip_register(void*, int);
extern int wanpipe_eclip_unregister(void*,void*);

/*****************************************************************************
**			  FUNCTION DEFINITIONS
*****************************************************************************/
#if defined(WAN_OCT6100_DAEMON)
#define my_isdigit(c)	((c) >= '0' && (c) <= '9')
static int wan_ec_devnum(char *ptr)
{
	int	num = 0, base = 1;
	int	i = 0;

	while(!my_isdigit(*ptr)) ptr++;
	while(my_isdigit(ptr[i])){
		i++;
		base = base * 10;
	}
	if (i){
		i=0;
		do {
			base = base / 10;
			num = num + (ptr[i]-'0') * base;
			i++;
		}while(my_isdigit(ptr[i]));
	}
	return num;
}
#endif

void *wan_ec_config(void *pcard, int max_channels)
{
	sdla_t		*card = (sdla_t*)pcard;
	

#if defined(WAN_OCT6100_DAEMON)
	wan_ec_t	*ec = NULL;
	wan_ec_dev_t	*ec_dev = NULL, *ec_dev_new = NULL;
	int		err;
	
	WAN_LIST_FOREACH(ec, &wan_ec_head, next){
		WAN_LIST_FOREACH(ec_dev, &ec->ec_dev, next){
			if (ec_dev->card == NULL || ec_dev->card == card){
				DEBUG_ERROR("%s: Internal Error (%s:%d)\n",
						card->devname,
						__FUNCTION__,__LINE__);
				return NULL;
			}
		}
	}
	WAN_LIST_FOREACH(ec, &wan_ec_head, next){
		WAN_LIST_FOREACH(ec_dev, &ec->ec_dev, next){
			if (card->hw_iface.hw_same(ec_dev->card->hw, card->hw)){
				/* Current OCT6100 chip is already in use */
				break;
			}
		}
		if (ec_dev){
			break;
		}
	}
	ec_dev_new = wan_malloc(sizeof(wan_ec_dev_t));
	if (ec_dev_new == NULL){
		DEBUG_ERROR("%s: ERROR: Failed to allocate memory (%s:%d)!\n",
					card->devname,
					__FUNCTION__,__LINE__);
		return NULL;
	}
	memset(ec_dev_new, 0, sizeof(wan_ec_dev_t));

	if (ec_dev == NULL){
		
		/* First device for current Oct6100 chip */
		ec = wan_malloc(sizeof(wan_ec_t));
		if (ec == NULL){
			DEBUG_ERROR("%s: ERROR: Failed to allocate memory (%s:%d)!\n",
						card->devname,
						__FUNCTION__,__LINE__);
			return NULL;
		}
		memset(ec, 0, sizeof(wan_ec_t));
		ec->chip_no		= ++wan_ec_no;
		ec->state		= WAN_OCT6100_STATE_RESET;
		ec->ec_channels_no	= 0;
		ec->max_channels	= max_channels;
		sprintf(ec->name, "wp_ec%d", ec->chip_no);

		WAN_LIST_INIT(&ec->ec_dev);

		WAN_LIST_INSERT_HEAD(&wan_ec_head, ec, next);

	}else{
		ec = ec_dev->ec;
	}
	ec->usage++;
	ec_dev_new->ecdev_no = wan_ec_devnum(card->devname);
	ec_dev_new->ec		= ec;
	ec_dev_new->name	= ec->name;
	ec_dev_new->card	= card;
	/* Initialize hwec_bypass pointer */
	card->wandev.ec_action	= wan_ec_action;
	card->wandev.ec_map		= 0;
	memcpy(ec_dev_new->devname, card->devname, sizeof(card->devname));
	sprintf(ec_dev_new->ecdev_name, "wp%dec", ec_dev_new->ecdev_no);
#if defined(__LINUX__)
	err=wanpipe_ecdev_reg(ec, ec_dev_new->ecdev_name, ec_dev_new->ecdev_no);
	if (err){
		DEBUG_EVENT(
		"%s: ERROR: Failed to create device (%s:%d)!\n",
					card->devname,
					__FUNCTION__,__LINE__);
		return NULL;
	}
#else
	err=wp_cdev_reg(ec, ec_dev_new->ecdev_name, wan_ec_dev_ioctl);
	if (err){
		DEBUG_EVENT(
		"%s: ERROR: Failed to create device (%s:%d)!\n",
					card->devname,
					__FUNCTION__,__LINE__);
		return NULL;
	}
#endif
	WAN_LIST_INSERT_HEAD(&ec->ec_dev, ec_dev_new, next);

	DEBUG_EVENT("%s: Register EC interface %s (usage %d, max ec channels %d)!\n",
					ec_dev_new->devname,
					ec->name,
					ec->usage,
					ec->max_channels);
	return (void*)ec_dev_new;
#else
	return wanpipe_eclip_register(card, max_channels);
#endif
}

int wan_ec_remove(void *arg, void *pcard)
{
#if defined(WAN_OCT6100_DAEMON)
	sdla_t		*card = (sdla_t*)pcard;
	wan_ec_t	*ec = NULL;
	wan_ec_dev_t	*ec_dev = (wan_ec_dev_t*)arg;

	WAN_ASSERT(ec_dev == NULL);
	ec = ec_dev->ec;
	DEBUG_EVENT("%s: Unregister EC interface %s (chip id %d, usage %d)!\n",
					card->devname,
					ec->name,
					ec->chip_no,
					ec->usage);
	ec_dev->card = NULL;
	ec->usage--;
	if (WAN_LIST_FIRST(&ec->ec_dev) == ec_dev){
		WAN_LIST_FIRST(&ec->ec_dev) =
				WAN_LIST_NEXT(ec_dev, next);
		WAN_LIST_NEXT(ec_dev, next) = NULL;
	}else{
		WAN_LIST_REMOVE(ec_dev, next);
	}
#if defined(__LINUX__)
	wanpipe_ecdev_unreg(ec_dev->ecdev_no);
#else
	wp_cdev_unreg(ec_dev->ecdev_name);
#endif
	if (!ec->usage){
		ec_dev->ec = NULL;
		wan_free(ec_dev);
		if (WAN_LIST_FIRST(&wan_ec_head) == ec){
			WAN_LIST_FIRST(&wan_ec_head) =
					WAN_LIST_NEXT(ec, next);
			WAN_LIST_NEXT(ec, next) = NULL;
		}else{
			WAN_LIST_REMOVE(ec, next);
		}
		wan_free(ec);
	}else{
		ec_dev->ec = NULL;
		wan_free(ec_dev);
	}
	return 0;
#else
	return wanpipe_eclip_unregister(arg, pcard);
#endif
}
 
#if defined(WAN_OCT6100_DAEMON)
static wan_ec_dev_t *wan_ec_search(char *devname)
{
	wan_ec_t	*ec;
	wan_ec_dev_t	*ec_dev = NULL;

	WAN_LIST_FOREACH(ec, &wan_ec_head, next){
		WAN_LIST_FOREACH(ec_dev, &ec->ec_dev, next){
			if (strcmp(ec_dev->devname, devname) == 0){
				return ec_dev;
			}
		}
	}
	return NULL;
}

static int
wan_ec_reset(wan_ec_dev_t *ec_dev, sdla_t *card, int cmd)
{
	wan_ec_t	*ec;
	int		err = -EINVAL;

	WAN_ASSERT(ec_dev == NULL);
	ec = ec_dev->ec;

	if (card->wandev.hwec_reset){
		err = card->wandev.hwec_reset(
					card,
					(cmd==WAN_OCT6100_CMD_SET_RESET) ?
						AFT_HWEC_SET_RESET :
						AFT_HWEC_CLEAR_RESET);
		if (!err){
			if (cmd == WAN_OCT6100_CMD_SET_RESET){
				ec->state = WAN_OCT6100_STATE_RESET;
			}else{
				ec->state = WAN_OCT6100_STATE_READY;
			}
		}
	}
	return err;
}

static int wan_ec_action(void *pcard, int enable, int channel)
{
	sdla_t		*card = (sdla_t*)pcard;
	wan_ec_t	*ec = NULL;
	wan_ec_dev_t	*ec_dev = NULL;
	int		err = -ENODEV;

	ec_dev = wan_ec_search(card->devname);
	WAN_ASSERT(ec_dev == NULL);
	WAN_ASSERT(ec_dev->ec == NULL);
	ec = ec_dev->ec;	

	DEBUG_HWEC("[HWEC]: %s: %s bypass mode for channel %d!\n",
			card->devname,
			(enable) ? "Enable" : "Disable",
			channel);

	if (card->wandev.hwec_enable){
		if (enable){
			if (ec->ec_channels_no >= ec->max_channels){
				DEBUG_EVENT("%s: Exceeded maximum number of Echo Canceller channels (max=%d)!\n",
						ec->name,
						ec->max_channels);
				return -ENODEV;
			}
		}
		err = card->wandev.hwec_enable(card, enable, channel);
		if (!err){
			if (enable){
				ec->ec_channels_no++;
			}else{
				ec->ec_channels_no--;
			}
		}
	}else{
		DEBUG_HWEC("[HWEC]: %s: %s(): card->wandev.hwec_enable == NULL!\n",
			card->devname, __FUNCTION__);
	}
	return err;
}

static int wan_ec_tone_detection(void *pcard, wan_ec_api_t *ec_api)
{
	sdla_t		*card = (sdla_t*)pcard;

	WAN_ASSERT(card == NULL);

	DEBUG_EVENT("[HWEC]: %s: Received tone event on channel %d: %s %s!\n",
			card->devname,
			ec_api->u_tone.fe_chan,
			WAN_EC_DECODE_TONE_TYPE(ec_api->u_tone.type),
			WAN_EC_DECODE_DTMF_ID(ec_api->u_tone.id));

	return 0;
}

static u32 convert_addr(u32 addr)
{
	addr &= 0xFFFF;
	switch(addr){
	case 0x0000:
		return 0x60;
	case 0x0002:
		return 0x64;
	case 0x0004:
		return 0x68;
	case 0x0008:
		return 0x70;
	case 0x000A:
		return 0x74;
	}
	return 0x00;
}

/*===========================================================================*\
  ReadInternalDword
\*===========================================================================*/
static int
wan_ec_read_internal_dword(wan_ec_dev_t *ec_dev, u32 addr1, u32 *data)
{
	sdla_t	*card = ec_dev->card;
	u32	addr;
	int err;

	WAN_ASSERT(card == NULL);
	addr = convert_addr(addr1);
	if (addr == 0x00){
		DEBUG_ERROR("%s: %s:%d: Internal Error (EC off %X)\n",
				card->devname,
				__FUNCTION__,__LINE__,
				addr1);
		return -EINVAL;
	}

	err = card->hw_iface.bus_read_4(card->hw, addr, data);
	
	WP_DELAY(5);
	return err;
}


/*===========================================================================*\
  WriteInternalDword
\*===========================================================================*/
static int
wan_ec_write_internal_dword(wan_ec_dev_t *ec_dev, u32 addr1, u32 data)
{
	sdla_t	*card = ec_dev->card;
	u32	addr;
	int	err;

	WAN_ASSERT(card == NULL);
	addr = convert_addr(addr1);
	if (addr == 0x00){
		DEBUG_ERROR("%s: %s:%d: Internal Error (EC off %X)\n",
				card->devname,
				__FUNCTION__,__LINE__,
				addr1);
		return -EINVAL;
	}


	err = card->hw_iface.bus_write_4(card->hw, addr, data);	

	WP_DELAY(5);
	return err;
}

/*===========================================================================*\
  Oct6100Read
\*===========================================================================*/

static int
wan_ec_read(wan_ec_dev_t *ec_dev, u32 read_addr, u16 *read_data)
{
	u32	data;
	wan_ec_read_internal_dword(ec_dev, read_addr, &data);
	*read_data = (u16)(data & 0xFFFF);
	return 0;
}

/*===========================================================================*\
  Oct6100Write
\*===========================================================================*/

static int
wan_ec_write(wan_ec_dev_t *ec_dev, u32 write_addr, u32 write_data)
{
	return wan_ec_write_internal_dword(ec_dev, write_addr, write_data);
}


/*===========================================================================*\
  Oct6100WriteSequenceSub
\*===========================================================================*/
static u32
wan_ec_write_seq(wan_ec_dev_t *ec_dev, u32 write_addr, u16 write_data)
{
	u32	ulResult;
	u32	ulData;
	u16	usData;
	u32	ulWordAddress;
	u32	i;

	/* Create the word address from the provided byte address. */
	ulWordAddress = write_addr >> 1;

	/* 16-bit indirect access. */

	/* First write to the address indirection registers. */
	ulData = ( ulWordAddress >> 19 ) & 0x1FFF;
	ulResult = wan_ec_write( ec_dev, 0x8, ulData );
	if (ulResult) 
		return ulResult;

	ulData = ( ulWordAddress >> 3 ) & 0xFFFF;
	ulResult = wan_ec_write( ec_dev, 0xA, ulData );
	if (ulResult) 
		return ulResult;

	/* Next, write data word to read/write data registers. */
	ulData = write_data & 0xFFFF;
	ulResult = wan_ec_write( ec_dev, 0x4, ulData );
	if ( ulResult ) 
		return ulResult;


	/* Write the parities and write enables, as well as last three bits
	** of wadd and request the write access. */
	ulData = ( ( 0x0 & 0x3 ) << 14 ) | ( ( 0x3 & 0x3 ) << 12 ) | ( ( ulWordAddress & 0x7 ) << 9 ) | 0x0100;
	ulResult = wan_ec_write( ec_dev, 0x0, ulData );
	if ( ulResult ) 
		return ulResult;

	/* Keep polling register contol0 for the access_req bit to go low. */
	for ( i = 0; i < WAN_OCT6100_READ_LIMIT; i++ )
	{
		ulResult = wan_ec_read( ec_dev, 0, &usData );
		if ( ulResult ) 
			return ulResult;

		if ( ( ( usData >> 8 ) & 0x1 ) == 0x0 ) 
			break;
	}

	if ( i == WAN_OCT6100_READ_LIMIT ){
		DEBUG_EVENT("%s: EC write command reached limit!\n",
				ec_dev->name);
		return WAN_OCT6100_RC_CPU_INTERFACE_NO_RESPONSE;
	}
	return 0;
}


/*===========================================================================*\
  HandleReqWriteOct6100
\*===========================================================================*/
EXPORT_SYMBOL(wan_ec_req_write);
u32 wan_ec_req_write(void *arg, u32 write_addr, u16 write_data)
{
	wan_ec_dev_t	*ec_dev = (wan_ec_dev_t*)arg;
	u32		ulResult;

	DEBUG_TEST(": %s: EC WRITE API addr=%X data=%X\n",
				ec_dev->ec->name, write_addr, write_data);
	ulResult = wan_ec_write_seq(ec_dev, write_addr, write_data);
	if (ulResult){
		DEBUG_EVENT("%s: Failed to write %04X to addr %08X\n",
						ec_dev->name,
						write_addr,
						write_data);
	}
	return ulResult;
}


/*===========================================================================*\
  HandleReqWriteSmearOct6100
\*===========================================================================*/
EXPORT_SYMBOL(wan_ec_req_write_smear);
u32 wan_ec_req_write_smear(void *arg, u32 addr, u16 data, u32 len)
{
	wan_ec_dev_t	*ec_dev = (wan_ec_dev_t*)arg;
	u32		i, ulResult = WAN_OCT6100_RC_OK;

	WAN_ASSERT(ec_dev == NULL);
	for ( i = 0; i < len; i++ ){
		ulResult = wan_ec_write_seq(ec_dev, addr + (i*2), data);
		if (ulResult){
			DEBUG_EVENT("%s: Failed to write %04X to addr %08X\n",
						ec_dev->name,
						addr + (i*2),
						data);
			break;
		}
	}
	return ulResult;
}


/*===========================================================================*\
  HandleReqWriteBurstOct6100
\*===========================================================================*/
EXPORT_SYMBOL(wan_ec_req_write_burst);
u32 wan_ec_req_write_burst(void *arg, u32 addr, u16 *data, u32 len)
{
	wan_ec_dev_t	*ec_dev = (wan_ec_dev_t*)arg;
	u32		i, ulResult = WAN_OCT6100_RC_OK;

	WAN_ASSERT(ec_dev == NULL);

	for ( i = 0; i < len; i++ ){
		ulResult = wan_ec_write_seq(ec_dev, addr + (i * 2), data[i]);
		if (ulResult){
			DEBUG_EVENT("%s: Failed to write %04X to addr %08X\n",
						ec_dev->name,
						addr + (i*2),
						data[i]);
			break;
		}
	}
	return ulResult;
}


/*===========================================================================*\
  Oct6100ReadSequenceSub
\*===========================================================================*/
static u32
wan_ec_read_seq(wan_ec_dev_t *ec_dev, u32 read_addr, u16 *read_data, u32 read_len)
{
	u32	ulResult;
	u32	ulData;
	u32	ulWordAddress;
	u32	ulReadBurstLength;
	u16	usData;
	u32	i;

	/* Create the word address from the provided byte address. */
	ulWordAddress = read_addr >> 1;

	/* Indirect accesses. */

	/* First write to the address indirection registers. */
	ulData = ( ulWordAddress >> 19 ) & 0x1FFF;
	ulResult = wan_ec_write( ec_dev, 0x8, ulData );
	if (ulResult) 
		return ulResult;

	ulData = ( ulWordAddress >> 3 ) & 0xFFFF;
	ulResult = wan_ec_write( ec_dev, 0xA, ulData );
	if (ulResult)
		return ulResult;

	/* Request access. */
	if ( read_len >= 128 )
	{
		ulData = 0x100 | ( ( ulWordAddress & 0x7 ) << 9);
		ulReadBurstLength = 0;
	}
	else
	{
		ulData = 0x100 | ( ( ulWordAddress & 0x7 ) << 9) | read_len;
		ulReadBurstLength = read_len;
	}
	ulResult = wan_ec_write( ec_dev, 0x0, ulData );
	if (ulResult)
		return ulResult;

	/* Keep polling register contol0 for the access_req bit to go low. */
	for ( i = 0; i < WAN_OCT6100_READ_LIMIT; i++ )
	{
		ulResult = wan_ec_read( ec_dev, 0x0, &usData );
		if (ulResult) 
			return ulResult;

		if ( ( ( usData >> 8 ) & 0x1 ) == 0x0 )
			break;
	}
	if ( i == WAN_OCT6100_READ_LIMIT ){
		DEBUG_EVENT("%s: EC read command reached limit!\n",
				ec_dev->name);
		return WAN_OCT6100_RC_CPU_INTERFACE_NO_RESPONSE;
	}

	if ( ( usData & 0xFF ) == 0x1 )
	{
		i = 0;
	}

	/* Retrieve read data. */
	ulResult = wan_ec_read( ec_dev, 0x4, &usData );
	if (ulResult)
		return ulResult;

	if ( ( usData & 0xFF ) == 0x1 )
	{
		i = 0;
	}

	*read_data = usData;
	return 0;
}

EXPORT_SYMBOL(wan_ec_req_read);
u32 wan_ec_req_read(void *arg, u32 addr, u16 *data)
{
	wan_ec_dev_t	*ec_dev = (wan_ec_dev_t*)arg;
	u32		ulResult;

	WAN_ASSERT(ec_dev == NULL);
	DEBUG_TEST("%s: EC READ API addr=%X data=????\n",
			ec_dev->ec->name, addr);
	ulResult = wan_ec_read_seq(
				ec_dev,
				addr,
				data,
				1);
	if (ulResult){
		DEBUG_EVENT("%s: Failed to read data from addr %08X\n",
					ec_dev->name,
					addr);
	}
	DEBUG_TEST("%s: EC READ API addr=%X data=%X\n",
			ec_dev->ec->name,
			addr, *data);
	return ulResult;
}


EXPORT_SYMBOL(wan_ec_req_read_burst);
u32 wan_ec_req_read_burst(void *arg, u32 addr, u16 *data, u32 len)
{
	wan_ec_dev_t	*ec_dev = (wan_ec_dev_t*)arg;
	u32		i, ulResult = WAN_OCT6100_RC_OK;
	u16		read_data;

	for ( i = 0; i < len; i++ ){
		ulResult = wan_ec_read_seq(ec_dev, addr, &read_data, 1);
		if (ulResult){
			DEBUG_EVENT("%s: Failed to read from addr %X\n",
						ec_dev->name,
						addr);
			break;
		}
		data[i]  = (u16)read_data;
		addr += 2;
	}
	return ulResult;
}

static int
wan_ec_req_write_burst_prepare(wan_ec_dev_t *ec_dev, wan_ec_api_t *ec_api)
{
	u32	len = ec_api->u_oct6100_write_burst.ulWriteLength;
	u16	*user_space = (u16*)ec_api->u_oct6100_write_burst.pWriteData,
		*data = NULL;
	u32	ulResult = WAN_OCT6100_RC_OK;
	int	err = 0;

	WAN_ASSERT(ec_dev == NULL);
	data = wan_malloc(len * sizeof(u16));
	if (data == NULL){
		DEBUG_EVENT("%s: Failed to allocate memory space [%s:%d]!\n",
				ec_dev->name,
				__FUNCTION__,__LINE__);
		return WAN_OCT6100_RC_MEMORY;
	}
	err = WAN_COPY_FROM_USER(data, user_space, len * sizeof(u16));
	if (err){
		DEBUG_EVENT("%s: Failed to copy data from user space [%s:%d]!\n",
				ec_dev->ec->name,
				__FUNCTION__,__LINE__);
		wan_free(data);
		return WAN_OCT6100_RC_MEMORY;
	}
	ulResult = wan_ec_req_write_burst(
				ec_dev,
				ec_api->u_oct6100_write_burst.ulWriteAddress,
				data,
				ec_api->u_oct6100_write_burst.ulWriteLength);

	if (data) wan_free(data);
	return ulResult;
}

static int
wan_ec_req_read_burst_prepare(wan_ec_dev_t *ec_dev, wan_ec_api_t *ec_api)
{
	u32	len = ec_api->u_oct6100_read_burst.ulReadLength;
	u16	*user_space = (u16*)ec_api->u_oct6100_read_burst.pReadData,
		*data = NULL;
	u32	ulResult = WAN_OCT6100_RC_OK;
	int	err;

	WAN_ASSERT(ec_dev == NULL);
	data = wan_malloc(len * sizeof(u16));
	if (data == NULL){
		DEBUG_EVENT("%s: Failed allocate memory (%d bytes)\n",
				ec_dev->name,
				len * sizeof(u16));
		return WAN_OCT6100_RC_MEMORY;
	}

	ulResult = wan_ec_req_read_burst(
				ec_dev,
				ec_api->u_oct6100_read_burst.ulReadAddress,
				data,
				ec_api->u_oct6100_read_burst.ulReadLength);	
	if (ulResult == WAN_OCT6100_RC_OK){
		err = WAN_COPY_TO_USER(	user_space,
				data,
				len * sizeof(u16));
		if (err){
			DEBUG_EVENT("%s: Failed to copy data to user space [%s:%d]!\n",
				ec_dev->ec->name,
				__FUNCTION__,__LINE__);
			ulResult = WAN_OCT6100_RC_MEMORY;
		}
	}
	if (data) wan_free(data);
	return ulResult;
}

static int wan_ec_getinfo(wan_ec_dev_t *ec_dev, wan_ec_api_t *ec_api)
{
	sdla_t		*card = ec_dev->card;
	wan_ec_t	*ec = NULL;

	if (strcmp(card->devname, ec_api->devname)){
		DEBUG_EVENT("%s:%s: Failed to find %s device!\n",
				card->devname, ec_api->if_name,
				ec_api->devname);
		return -EINVAL;
	}
	ec = ec_dev->ec;
	
	memcpy(ec_api->name, ec->name, strlen(ec->name));
	ec_api->u_ecd.state		= ec->state;
	ec_api->u_ecd.chip_no		= ec->chip_no;
	ec_api->u_ecd.fe_media		= WAN_FE_MEDIA(&card->fe);
	ec_api->u_ecd.fe_lineno		= WAN_FE_LINENO(&card->fe);
	ec_api->u_ecd.fe_max_channels	= WAN_FE_MAX_CHANNELS(&card->fe);
	ec_api->u_ecd.fe_tdmv_law	= WAN_FE_TDMV_LAW(&card->fe);
	ec_api->u_ecd.max_channels	= ec->max_channels;
	ec_api->u_ecd.max_ec_channels	= ec->max_ec_channels;
	ec_api->u_ecd.ulApiInstanceSize = ec->ulApiInstanceSize;

	return 0;
}
#endif 


int wan_ec_dev_ioctl(void *ec_arg, void *data)
{
#if defined(WAN_OCT6100_DAEMON)
	sdla_t		*card = NULL;
	wan_ec_t	*ec = (wan_ec_t*)ec_arg;
	wan_ec_dev_t	*ec_dev = NULL;
	wan_ec_api_t	*ec_api = NULL;
	int		err = 0;

	ec_dev = WAN_LIST_FIRST(&ec->ec_dev);
	if (ec_dev == NULL){
		DEBUG_ERROR("%s: Internal Error (%s:%d)\n",
					ec->name,
					__FUNCTION__,__LINE__);
		return -EINVAL;
	}
	card = ec_dev->card;
#if defined(__LINUX__)
	ec_api = wan_malloc(sizeof(wan_ec_api_t));
	if (ec_api == NULL){
		DEBUG_EVENT("%s: Failed allocate memory (%d) [%s:%d]!\n",
				ec->name,
				sizeof(wan_ec_api_t),
				__FUNCTION__,__LINE__);
		return -EINVAL;
	}
	err = WAN_COPY_FROM_USER(
				ec_api,
				data,
				sizeof(wan_ec_api_t));
	if (err){
		DEBUG_EVENT("%s: Failed to copy data from user space [%s:%d]!\n",
				ec->name,
				__FUNCTION__,__LINE__);
		wan_free(ec_api);
		return -EINVAL;
	}
#else
	ec_api = (wan_ec_api_t*)data;
#endif

	DEBUG_TEST("%s: EC DEV cmd %s\n",
				ec->name,
			WAN_OCT6100_CMD_DECODE(ec_api->cmd)); 
	switch(ec_api->cmd){
	case WAN_OCT6100_CMD_GET_INFO:
		err = wan_ec_getinfo(ec_dev, ec_api);
		break;

	case WAN_OCT6100_CMD_CLEAR_RESET:
	case WAN_OCT6100_CMD_SET_RESET:
		err = wan_ec_reset(ec_dev, card, ec_api->cmd);
		break;

	case WAN_OCT6100_CMD_BYPASS_ENABLE:
		err = wan_ec_action(card, 1, ec_api->u_ecd.channel);
		break;

	case WAN_OCT6100_CMD_BYPASS_DISABLE:
		err = wan_ec_action(card, 0, ec_api->u_ecd.channel);
		break;

	case WAN_OCT6100_CMD_API_WRITE:
		ec_api->ulResult = 
			wan_ec_req_write(
				ec_dev,
				ec_api->u_oct6100_write.ulWriteAddress,
				ec_api->u_oct6100_write.usWriteData);
		break;

	case WAN_OCT6100_CMD_API_WRITE_SMEAR:
		ec_api->ulResult = 
			wan_ec_req_write_smear(
				ec_dev,
				ec_api->u_oct6100_write_swear.ulWriteAddress,
				ec_api->u_oct6100_write_swear.usWriteData,
				ec_api->u_oct6100_write_swear.ulWriteLength);
		break;

	case WAN_OCT6100_CMD_API_WRITE_BURST:
		ec_api->ulResult =
			wan_ec_req_write_burst_prepare(ec_dev, ec_api);
		break;

	case WAN_OCT6100_CMD_API_READ:
		ec_api->ulResult =
			wan_ec_req_read(
				ec_dev,
				ec_api->u_oct6100_read.ulReadAddress,
				&ec_api->u_oct6100_read.usReadData);
		break;

	case WAN_OCT6100_CMD_API_READ_BURST:
		ec_api->ulResult =
			wan_ec_req_read_burst_prepare(ec_dev, ec_api);
		break;

	case WAN_OCT6100_CMD_TONE_DETECTION:
		err = wan_ec_tone_detection(card, ec_api);
		break;

	default:
		DEBUG_EVENT("%s: Unsupported EC DEV IOCTL command for %s (%X)\n",
				card->devname, ec->name, ec_api->cmd);
		err = -EINVAL;
		break;
	}
#if 0
	aft_fe_intr_ctrl(card, 1);
	card->hw_iface.hw_unlock(card->hw,&smp_flags);
#endif
	
#if defined(__LINUX__)
	err = WAN_COPY_TO_USER(
				data,
				ec_api,
				sizeof(wan_ec_api_t));
	if (err){
		DEBUG_EVENT("%s: Failed to copy data to user space [%s:%d]!\n",
				ec->name,
				__FUNCTION__,__LINE__);
		wan_free(ec_api);
		return -EINVAL;
	}
	wan_free(ec_api);
#endif
#endif
	return 0;
}
