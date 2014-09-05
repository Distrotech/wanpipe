/*****************************************************************************
* sdladrv_usb.c 
* 		
* 		SDLA USB Support Interface
*
* Authors: 	Alex Feldman <alex@sangoma.com>
*
* Copyright:	(c) 2003-2006 Sangoma Technologies Inc.
*
*		This program is free software; you can redistribute it and/or
*		modify it under the terms of the GNU General Public License
*		as published by the Free Software Foundation; either version
*		2 of the License, or (at your option) any later version.
* ============================================================================
* Jan 30, 2008	Alex Feldman	Initial version.
*****************************************************************************/

/***************************************************************************
****		I N C L U D E  		F I L E S			****
***************************************************************************/
#if defined(__NetBSD__) || defined(__FreeBSD__) || defined(__OpenBSD__)
# include <wanpipe_includes.h>
# include <wanpipe.h>
# include <wanpipe_abstr.h> 
# include <wanpipe_snmp.h> 
# include <if_wanpipe_common.h>    /* Socket Driver common area */
# include <sdlapci.h>
# include <aft_core.h>
# include <wanpipe_iface.h>
#else
# include <linux/wanpipe_includes.h>
# include <linux/tty.h>
# include <linux/usb.h>
# include "wanpipe.h"
# include "sdlapci.h"
# include "sdladrv_usb.h"
#endif

#if defined(CONFIG_PRODUCT_WANPIPE_USB) 

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,10)
#  include <linux/usb/serial.h>
#else
#  define MAX_NUM_PORTS 8  //This is normally defined in /linux/usb/serial.h
#endif
/***************************************************************************
****                   M A C R O S     D E F I N E S                    ****
***************************************************************************/

#if LINUX_VERSION_CODE > KERNEL_VERSION(2,4,19)
#define USB2420
#endif

#define LINUX26

#define SDLA_USBFXO_BL_DELAY		(10)	// ms 

#define WP_USB_RXTX_DATA_LEN		8
#define WP_USB_RXTX_DATA_COUNT		2	// Original 2
#define WP_USB_MAX_RW_COUNT		100

#define SDLA_USB_NAME			"sdlausb"

#define SDLA_USBFXO_VID			0x10C4
#define SDLA_USBFXO_PID			0x8461

#define SDLA_USBFXO_SYNC_DELAY		10
#define SDLA_USBFXO_SYNC_RX_RETRIES	100
#define SDLA_USBFXO_SYNC_TX_RETRIES	400
#define SDLA_USBFXO_READ_DELAY		10
#define SDLA_USBFXO_READ_RETRIES	100
#define SDLA_USBFXO_WRITE_DELAY		10

#define WP_USB_BAUD_RATE		500000

#define WP_USB_IDLE_PATTERN		0x7E
#define WP_USB_CTRL_IDLE_PATTERN	0x7E

#define WP_USB_MAX_RX_CMD_QLEN		20	// Original 10
#define WP_USB_MAX_TX_CMD_QLEN		20	

unsigned char readFXOcmd[]= { 0x70, 0xC1, 0xC8 , 0x88 , 0x12 , 0x03 , 0x48 , 0x08 , 0x14 , 0x05 , 0x88 , 0x08 , 0x56 , 0xC7 , 0xC8 , 0x88 , 0x58 , 0xC9 , 0xC8 , 0x88 , 0x5A , 0xCB , 0xC8 , 0x88 , 0x5C , 0xCD , 0xC8 , 0x88 , 0x5E , 0xCF , 0xC8 , 0x88 };

unsigned char idlebuf[]= 
	{
		0x77, 0xCF, 0xCF, 0x8E,
		0x57, 0xCF, 0xCF, 0x8E,
		0x57, 0xCF, 0xCF, 0x8E,
		0x57, 0xCF, 0xCF, 0x8E,
		0x57, 0xCF, 0xCF, 0x8E,
		0x57, 0xCF, 0xCF, 0x8E,
		0x57, 0xCF, 0xCF, 0x8E,
		0x57, 0xCF, 0xCF, 0x8E,
		0x77, 0xCF, 0xCF, 0x8E,
		0x57, 0xCF, 0xCF, 0x8E,
		0x57, 0xCF, 0xCF, 0x8E,
		0x57, 0xCF, 0xCF, 0x8E,
		0x57, 0xCF, 0xCF, 0x8E,
		0x57, 0xCF, 0xCF, 0x8E,
		0x57, 0xCF, 0xCF, 0x8E,
		0x57, 0xCF, 0xCF, 0x8E
	};

#define WP_USB_STATUS_ATTACHED		1
#define WP_USB_STATUS_READY		2
#define WP_USB_STATUS_TX_READY		3
#define WP_USB_STATUS_RX_EVENT1_READY	4
#define WP_USB_STATUS_RX_EVENT2_READY	5
#define WP_USB_STATUS_RX_DATA_READY	6
#define WP_USB_STATUS_BH		7
#define WP_USB_STATUS_TX_CMD		8

#define WP_USB_CMD_TYPE_MASK		0x1C
#define WP_USB_CMD_TYPE_SHIFT		2
#define WP_USB_CMD_TYPE_READ_FXO	0x01
#define WP_USB_CMD_TYPE_WRITE_FXO	0x02
#define WP_USB_CMD_TYPE_EVENT		0x03
#define WP_USB_CMD_TYPE_READ_CPU	0x04
#define WP_USB_CMD_TYPE_WRITE_CPU	0x05
#define WP_USB_CMD_TYPE_DECODE(ctrl)	(((ctrl) & WP_USB_CMD_TYPE_MASK) >> WP_USB_CMD_TYPE_SHIFT)
#define WP_USB_CMD_TYPE_ENCODE(cmd)	((cmd) << WP_USB_CMD_TYPE_SHIFT)

#define WP_USB_CTRL_DECODE(buf)							\
		((buf)[0] & 0xC0) | (((buf)[1] & 0xC0) >> 2) | 			\
		(((buf)[2] & 0xC0) >> 4) | (((buf)[3] & 0xC0) >> 6)

#define WP_USB_CTRL_ENCODE(buf, ind)							\
	if (ind % 4 == 0 && ind % 32 == 0) *((buf)+ind) = (*((buf)+ind) & 0xCF) | 0x30;	\
	else if (ind % 4 == 0)  *((buf)+ind) = (*((buf)+ind) & 0xCF) | 0x10;

#define WP_USB_CTRL_ENCODE1(buf, ctrl)						\
	*(buf)     = (*(buf) & 0xCF)     | (ctrl);

#define WP_USB_DATA_ENCODE(buf, data)						\
	*(buf)     = (*(buf) & 0xF0)     | ((data) & 0x0F);			\
	*((buf)+2) = (*((buf)+2) & 0xF0) | (((data) >> 4) & 0x0F);
	
#define WP_USB_CMD_ENCODE(buf, cmd)						\
	*(buf)     = (*(buf) & 0x3F)     | ((cmd) & 0xC0);			\
	*((buf)+1) = (*((buf)+1) & 0x3F) | (((cmd) & 0x30) << 2);		\
	*((buf)+2) = (*((buf)+2) & 0x3F) | (((cmd) & 0x0C) << 4);		\
	*((buf)+3) = (*((buf)+3) & 0x3F) | (((cmd) & 0x03) << 6); 
#if 0
#define WP_USB_CTRL_ENCODE(buf, ctrl)						\
	*(buf) |= (ctrl) & 0xC0;						\
	*((buf)+1) |= ((ctrl) & 0x30) << 2;					\
	*((buf)+2) |= ((ctrl) & 0x0C) << 4;					\
	*((buf)+3) |= ((ctrl) & 0x03) << 6; 
#endif

#define WP_USB_GUARD1_CHECK(hwcard) if(hwcard){	\
	if (hwcard->u_usb.guard1[0]){\
	 DEBUG_EVENT("%s: Memory problem guard1 %d (%s:%d)!\n",\
		hwcard->name, 0, __FUNCTION__,__LINE__);\
	hwcard->u_usb.guard1[0]=0x00;\
	}\
	}

#define WP_USB_GUARD1(hwcard) if(hwcard){	\
	int i=0;\
	for(i=0;i<1000;i++){\
	if (hwcard->u_usb.guard1[i]){\
	 DEBUG_EVENT("%s: Memory problem guard1 %d (%s:%d)!\n",\
		hwcard->name, i, __FUNCTION__,__LINE__);\
	}\
	}\
	}

#define WP_USB_GUARD2(hwcard) if (hwcard){	\
	int i=0;\
	for(i=0;i<1000;i++){\
	if (hwcard->u_usb.guard2[i]){\
	 DEBUG_EVENT("%s: Memory problem guard2 %d (%s:%d)!\n",\
		hwcard->name, i, __FUNCTION__,__LINE__);\
	}\
	}\
	}

/***************************************************************************
****               S T R U C T U R E S   T Y P E D E F S                ****
***************************************************************************/
struct sdla_usb_desc {
        char	*name;
        int	adptr_type;
};

/***************************************************************************
****               F U N C T I O N   P R O T O T Y P E S                ****
***************************************************************************/
extern sdlahw_t* sdla_find_adapter(wandev_conf_t* conf, char* devname);
extern sdlahw_cpu_t* sdla_hwcpu_search(u8, int, int, int, int);
extern sdlahw_t* sdla_hw_search(sdlahw_cpu_t*, int);
extern int sdla_hw_fe_test_and_set_bit(void *phw,int value);
extern int sdla_hw_fe_test_bit(void *phw,int value);
extern int sdla_hw_fe_set_bit(void *phw,int value);
extern int sdla_hw_fe_clear_bit(void *phw,int value);
extern int sdla_usb_create(struct usb_interface*, int);
extern int sdla_usb_remove(struct usb_interface*, int);

static int sdla_usb_probe(struct usb_interface*, const struct usb_device_id*);
static void sdla_usb_disconnect(struct usb_interface*);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,10) 
static int sdla_usb_suspend (struct usb_interface*, pm_message_t);
#else
static int sdla_usb_suspend (struct usb_interface*, u32);
#endif
static int sdla_usb_resume (struct usb_interface*);
#if 0
static void sdla_usb_prereset (struct usb_interface*);
static void sdla_usb_postreset (struct usb_interface*);
#endif

u_int8_t sdla_usb_fxo_read(void *phw, ...);
int sdla_usb_fxo_write(void *phw, ...);
int sdla_usb_cpu_read(void *phw, unsigned char off, unsigned char *data);
int sdla_usb_cpu_write(void *phw, unsigned char off, unsigned char data);
int sdla_usb_write_poll(void *phw, unsigned char off, unsigned char data);
int sdla_usb_read_poll(void *phw, unsigned char off, unsigned char *data);
int sdla_usb_hwec_enable(void *phw, int mod_no, int enable);
int sdla_usb_rxevent_enable(void *phw, int mod_no, int enable);
int sdla_usb_rxevent(void *phw, int mod_no, u8 *regs, int);
int sdla_usb_rxtx_data_init(void *phw, int, unsigned char **, unsigned char **);
int sdla_usb_rxdata_enable(void *phw, int enable);
int sdla_usb_set_intrhand(void* phw, wan_pci_ifunc_t *isr_func, void* arg, int notused);
int sdla_usb_restore_intrhand(void* phw, int notused);
int sdla_usb_err_stats(void*,void*,int);

#if defined(__LINUX__)
static void 	sdla_usb_bh (unsigned long);
#else
static void 	sdla_usb_bh (void*,int);
#endif

static int sdla_usb_rxdata_get(sdlahw_card_t*, unsigned char*, int);
static int sdla_usb_txdata_prepare(sdlahw_card_t*, char*, int);
static int wp_usb_start_transfer(struct wan_urb*);
static int sdla_usb_rxurb_reset(sdlahw_card_t *hwcard);
static int sdla_usb_txurb_reset(sdlahw_card_t *hwcard);

/***************************************************************************
****                   G L O B A L   V A R I A B L E                    ****
***************************************************************************/
static struct sdla_usb_desc sdlausb = { "Sangoma Wanpipe USB-FXO 2 Interfaces", U100_ADPTR };

static struct usb_device_id sdla_usb_dev_ids[] = {
          /* This needs to be a USB audio device, and it needs to be made by us and have the right device ID */
	{ 
		match_flags:		(USB_DEVICE_ID_MATCH_VENDOR|USB_DEVICE_ID_MATCH_DEVICE),
		bInterfaceClass:	USB_CLASS_AUDIO,
		bInterfaceSubClass:	1,
		idVendor:		SDLA_USBFXO_VID,
		idProduct:		SDLA_USBFXO_PID,
		driver_info:		(unsigned long)&sdlausb,
	},
	{ }     /* Terminating Entry */
};

static struct usb_driver sdla_usb_driver =
{
	name:		SDLA_USB_NAME,
	probe:		sdla_usb_probe,
	disconnect:	sdla_usb_disconnect,
	suspend:	sdla_usb_suspend,
	resume:		sdla_usb_resume,
#if 0
	pre_reset:	sdla_usb_prereset,
	post_reset:	sdla_usb_postreset,
#endif
	id_table:	sdla_usb_dev_ids,
};

int gl_usb_rw_fast = 0;
EXPORT_SYMBOL(gl_usb_rw_fast);

static const unsigned char MULAW_1[992] = {
22,44,153,136,137,163,23,10,14,44,167,161,199,45,71,158,142,146,223,15,6,13,65,147,141,155,
90,43,98,167,163,55,16,10,21,170,139,135,150,53,19,23,52,183,231,32,24,40,157,137,137,
158,27,9,12,36,168,156,175,55,59,166,144,146,189,18,7,11,49,148,140,150,206,40,54,178,
165,74,21,11,20,181,140,134,145,67,18,19,40,188,190,43,28,37,163,139,136,154,30,10,10,
29,171,153,164,75,54,176,149,147,176,22,7,10,40,150,139,145,185,38,40,206,169,226,26,13,
18,202,141,134,142,235,18,15,31,200,178,60,31,35,172,141,137,151,36,10,8,25,176,150,157,
207,52,197,155,149,170,26,9,9,33,152,138,142,172,38,31,66,174,195,31,15,18,87,143,134,
140,190,19,13,26,251,172,223,40,36,184,144,138,149,44,11,7,20,186,148,152,181,54,88,161,
152,166,30,10,9,29,156,138,140,163,40,27,45,184,186,40,18,18,59,146,134,139,175,20,12,
21,68,169,183,51,37,205,149,139,147,56,12,6,16,206,147,147,169,59,59,172,155,163,38,12,
9,25,159,138,138,157,43,24,34,209,182,50,22,20,47,150,135,138,167,22,11,16,51,168,169,
78,41,90,154,140,145,82,14,6,14,82,147,143,159,71,48,190,159,162,46,14,9,23,166,138,
136,153,48,21,28,72,182,72,27,22,42,154,136,137,160,25,10,13,41,168,160,194,46,65,159,
142,145,203,16,6,12,57,148,141,154,121,43,86,168,162,60,17,10,20,174,139,135,148,59,20,
23,49,184,215,34,24,38,159,138,136,156,28,10,11,33,170,156,173,57,57,168,145,145,183,19,
7,11,45,149,140,149,197,40,52,180,165,86,22,11,18,188,140,134,144,81,19,18,38,190,188,
44,28,35,166,140,136,153,32,10,9,28,173,153,163,81,53,179,150,147,173,23,8,10,37,151,
139,144,179,39,40,216,169,209,27,13,17,223,142,134,142,206,19,15,30,207,177,62,32,34,175,
142,137,150,40,11,8,23,181,150,156,200,53,203,155,149,168,28,9,9,30,154,138,141,169,40,
31,62,174,191,32,15,17,70,144,134,140,183,20,13,25,92,172,215,41,35,189,145,137,147,48,
12,7,19,192,149,151,177,55,79,162,152,164,32,11,9,27,157,138,139,161,42,27,44,187,185,
41,18,18,53,147,134,138,172,21,12,20,62,170,180,54,37,220,150,139,145,64,13,6,15,236,
148,146,167,61,57,173,155,161,41,13,9,24,162,138,137,156,45,24,33,221,182,53,23,19,44,
152,135,137,164,24,11,15,47,169,169,89,41,78,155,140,144,243,14,6,13,67,148,143,158,77,
47,194,160,161,49,15,9,21,170,139,136,151,53,22,27,66,182,78,28,21,39,156,137,136,158,
27,10,13,39,170,160,190,46,62,160,142,144,191,17,6,12,50,149,141,153,223,43,77,169,162,
65,18,10,19,179,140,135,147,67,20,22,47,185,206,35,24,36,161,138,136,154,30,10,11,31,
172,156,172,59,56,170,146,145,177,21,7,10,41,151,140,148,190,40,49,183,164,113,23,11,17,
198,141,134,143,254,20,17,37,193,186,46,28,34,169,140,136,151,36,10,9,26,176,153,161,89,
52,183,151,146,170,25,8,9,33,153,139,143,175,40,39,226,169,201,28,13,16,93,142,134,141,
192,20,14,29,217,176,67,33,33,179,142,137,148,43,11,8,22,186,151,155,195,53,211,156,149,
165,29,9,9,29,156,138,141,167,41,31,59,175,188,34,15,16,60,145,134,139,176,21,13,24,
79,172,206,42,34,197,146,138,146,55,12,7,17,206,149,150,175,56,74,163,152,162,35,11,8,
25,159,138,139,159,44,27,42,188,183,43,19,17,47,149,135,138,168,23,12,19,57,170,179,55,
37,118,151,139,144,77,14,6,14,86,149,146,165,64,56,174,155,160,44,13,9,22,165,138,137,
154,48,24,31,236,181,57,24,19,41,153,136,137,160,25,11,15,44,170,168,101,41,71,156,141,
143,206,15,6,13,59,149,143,157,86,47,199,160,160,55,15,9,19,173,139,136,150,58,22,26,
62,182,87,29,21,37,157,137,136,156,28,10,12,36,171,159,187,47,59,162,143,143,184,19,6,
11,45,150,141,152,207,43,72,170,161,75,19,10,17,186,140,135,145,78,21,21,45,187,202,36,
24,33,164,139,136,152,32,10,10,29,174,156,170,61,54,172/*,146,144,173,22,7,10,37,152*/
};


/***************************************************************************
****               F U N C T I O N   D E F I N I T I N S                ****
***************************************************************************/

/***************************************************************************
***************************************************************************/
/*****************************************************************************
*****************************************************************************/
int sdla_usb_init(void)
{
	int	ret;

	/* USB-FXO */
	ret = usb_register(&sdla_usb_driver);
	if (ret){
		DEBUG_EVENT("%s: Failed to register Sangoma USB driver!\n",
				SDLA_USB_NAME);
		return ret;
	}
	return 0;
}

int sdla_usb_exit(void)
{
	usb_deregister(&sdla_usb_driver);
	return 0;
}


static int sdla_usb_probe(struct usb_interface *intf, const struct usb_device_id *id)
{
	struct usb_device	*udev = interface_to_usbdev(intf);
	struct sdla_usb_desc 	*desc = (struct sdla_usb_desc *)id->driver_info;

	DEBUG_EVENT("%s: Probing %s (%X) on %d ...\n",
				SDLA_USB_NAME, desc->name, desc->adptr_type, udev->devnum);

	if (sdla_usb_create(intf, desc->adptr_type)){
		DEBUG_ERROR("ERROR: %s: Failed to probe new device on %d!\n",
				SDLA_USB_NAME, udev->devnum);
		return -ENODEV;
	}
	return 0;
}

static void sdla_usb_disconnect(struct usb_interface *intf)
{
	struct usb_device	*dev = interface_to_usbdev(intf);
	sdlahw_card_t		*hwcard = NULL;
	int			force = 0;

	DEBUG_EVENT("%s: Disconnect USB from %d ...\n", 
				SDLA_USB_NAME, dev->devnum);
	if ((hwcard = usb_get_intfdata(intf)) != NULL){
		wan_clear_bit(WP_USB_STATUS_READY, &hwcard->u_usb.status);
		force = 1;
	}

	sdla_usb_remove(intf, force);
	return;
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,10) 
static int sdla_usb_suspend (struct usb_interface *intf, pm_message_t message)
#else
static int sdla_usb_suspend (struct usb_interface *intf, u32 message)
#endif
{
	struct usb_device	*dev = interface_to_usbdev(intf);
	DEBUG_EVENT("%s: Suspend USB device on %d (not implemented)!\n", 
				SDLA_USB_NAME, dev->devnum);
	return 0;
}

static int sdla_usb_resume (struct usb_interface *intf)
{
	struct usb_device	*dev = interface_to_usbdev(intf);
	DEBUG_EVENT("%s: Resume USB device on %d (not implemented)!\n", 
				SDLA_USB_NAME, dev->devnum);
	return 0;
}

#if 0
static void sdla_usb_prereset (struct usb_interface *intf)
{
	struct usb_device	*dev = interface_to_usbdev(intf);
	DEBUG_EVENT("%s: Pre-reset USB device on %d (not implemented)!\n",
				SDLA_USB_NAME, dev->devnum);
}


static void sdla_usb_postreset (struct usb_interface *intf)
{
	struct usb_device	*dev = interface_to_usbdev(intf);
	DEBUG_EVENT("%s: Post-Reset USB device on %d (not implemented)!\n",
				SDLA_USB_NAME, dev->devnum);
}
#endif

/***************************************************************************
***************************************************************************/
static void wait_just_a_bit(int ms, int fast)
{

#if defined(__FreeBSD__) || defined(__OpenBSD__)
	WP_SCHEDULE(foo, "A-USB");
#else
	wan_ticks_t	start_ticks = SYSTEM_TICKS;
	int		delay_ticks = (HZ*ms)/1000;
	while((SYSTEM_TICKS-start_ticks) < delay_ticks){
		WP_DELAY(1);
# if defined(__LINUX__)
		if (!fast) WP_SCHEDULE(foo, "A-USB");
# endif
	}
#endif
}

static u_int8_t __sdla_usb_fxo_read(sdlahw_card_t *hwcard, int mod_no, unsigned char off)
{ 
	sdlahw_usb_t	*hwusb = NULL;
	netskb_t	*skb;
	int		retry = 0;	
	u8		data = 0xFF, *cmd_data;
	wan_smp_flag_t	flags;

	data = 0xFF;
	hwusb = &hwcard->u_usb;
	wan_spin_lock(&hwusb->cmd_lock, &flags);
	DEBUG_TX("%s: Tx Read FXO register (%02X:%d)!\n", 
			hwcard->name,
			WP_USB_CMD_TYPE_ENCODE(WP_USB_CMD_TYPE_READ_FXO) | mod_no, 
			(unsigned char)off);
	if (wan_test_and_set_bit(WP_USB_STATUS_TX_CMD, &hwusb->status)){
		DEBUG_USB("%s: WARNING: USB FXO Read Command Overrun (Read command in process)!\n",
					hwcard->name);
		hwusb->stats.cmd_overrun++;
		goto fxo_read_done;
	}
	
	if (!wan_skb_queue_len(&hwusb->tx_cmd_free_list)){
		DEBUG_USB("%s: WARNING: USB FXO Read Command Overrun (%d commands in process)!\n",
					hwcard->name, wan_skb_queue_len(&hwusb->tx_cmd_list));
		hwusb->stats.cmd_overrun++;
		goto fxo_read_done;
	}	
	skb = wan_skb_dequeue(&hwusb->tx_cmd_free_list);
	cmd_data = wan_skb_put(skb, 2);
	cmd_data[0] = WP_USB_CMD_TYPE_ENCODE(WP_USB_CMD_TYPE_READ_FXO) | mod_no;
	cmd_data[1] = off;
	wan_skb_queue_tail(&hwusb->tx_cmd_list, skb);

	do {
		wait_just_a_bit(SDLA_USBFXO_READ_DELAY, 1);	//WP_DELAY(10000);
		if (++retry > SDLA_USBFXO_READ_RETRIES) break;
	} while(!wan_skb_queue_len(&hwusb->rx_cmd_list));
	if (!wan_skb_queue_len(&hwusb->rx_cmd_list)){
		DEBUG_USB("%s: WARNING: Timeout on Read USB-FXO Reg!\n",
				hwcard->name);
		hwusb->stats.cmd_timeout++;
		goto fxo_read_done;
	}

	skb = wan_skb_dequeue(&hwusb->rx_cmd_list);
	cmd_data = wan_skb_data(skb);
	if (cmd_data[1] != off){
		DEBUG_USB("%s: USB FXO Read response is out of order (%02X:%02X)!\n",
				hwcard->name, cmd_data[1], off);
		hwusb->stats.cmd_invalid++;
		goto fxo_read_done;
	}
	data = (unsigned char)cmd_data[2];
	wan_skb_init(skb, 0);
	wan_skb_queue_tail(&hwusb->rx_cmd_free_list, skb);

fxo_read_done:
	wan_clear_bit(WP_USB_STATUS_TX_CMD, &hwusb->status);
	wan_spin_unlock(&hwusb->cmd_lock, &flags);
	return data;
}

u_int8_t sdla_usb_fxo_read(void *phw, ...)
{
	sdlahw_t	*hw = (sdlahw_t*)phw;
	sdlahw_card_t	*hwcard = NULL;
	va_list		args;
	int 		mod_no, off, data;

	WAN_ASSERT(hw == NULL);
	SDLA_MAGIC(hw);
	WAN_ASSERT(hw->hwcpu == NULL);
	WAN_ASSERT(hw->hwcpu->hwcard == NULL);
	hwcard = hw->hwcpu->hwcard;
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
	mod_no = va_arg(args, int);
	off	= va_arg(args, int);
	data	= va_arg(args, int);
	va_end(args);

	data = __sdla_usb_fxo_read(hwcard, mod_no, (unsigned char)off);
	sdla_hw_fe_clear_bit(hw,0);
	return data;
}

static int __sdla_usb_fxo_write(sdlahw_card_t *hwcard, int mod_no, unsigned char off, unsigned char data)
{
	sdlahw_usb_t	*hwusb;
	netskb_t	*skb;
	u8		*cmd_data;
	wan_smp_flag_t	flags; 

	hwusb = &hwcard->u_usb;
	wan_spin_lock(&hwusb->cmd_lock, &flags);
	DEBUG_TX("%s: Tx Write FXO register (%02X: %d <- 0x%02X)!\n", 
			hwcard->name, 
			WP_USB_CMD_TYPE_ENCODE(WP_USB_CMD_TYPE_WRITE_FXO) | mod_no, 
			(unsigned char)off, (unsigned char)data);
	if (!wan_skb_queue_len(&hwusb->tx_cmd_free_list)){
		DEBUG_USB("%s: WARNING: USB FXO Write Command Overrun (%d commands in process)!\n",
					hwcard->name, wan_skb_queue_len(&hwusb->tx_cmd_list));
		hwusb->stats.cmd_overrun++;
		wan_spin_unlock(&hwusb->cmd_lock, &flags);
		return 0;
	}	
	skb = wan_skb_dequeue(&hwusb->tx_cmd_free_list);
	cmd_data = wan_skb_put(skb, 3);
	cmd_data[0] = WP_USB_CMD_TYPE_ENCODE(WP_USB_CMD_TYPE_WRITE_FXO) | mod_no;
	cmd_data[1] = off; 
	cmd_data[2] = data; 
	wan_skb_queue_tail(&hwusb->tx_cmd_list, skb);

	/* update mirror registers */
	hwusb->regs[mod_no][off]  = data;

	wan_spin_unlock(&hwusb->cmd_lock, &flags);
	return 0;
}
int sdla_usb_fxo_write(void *phw, ...)
{
	sdlahw_t	*hw = (sdlahw_t*)phw;
	sdlahw_card_t	*hwcard = NULL;
	va_list		args;
	int 		mod_no, off, err, data;

	WAN_ASSERT(hw == NULL);
	SDLA_MAGIC(hw);
	WAN_ASSERT(hw->hwcpu == NULL);
	WAN_ASSERT(hw->hwcpu->hwcard == NULL);
	hwcard = hw->hwcpu->hwcard;
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
	mod_no = va_arg(args, int);
	off	= va_arg(args, int);
	data	= va_arg(args, int);
	va_end(args);

	err = __sdla_usb_fxo_write(hwcard, mod_no, (unsigned char)off, (unsigned char)data);
	sdla_hw_fe_clear_bit(hw,0);
	return err;
}

static int __sdla_usb_cpu_read(sdlahw_card_t *hwcard, unsigned char off, unsigned char *data)
{
	sdlahw_usb_t	*hwusb;
	netskb_t 	*skb;
	int		err = 0, retry = 0;
	u8		*cmd_data;
	wan_smp_flag_t	flags; 

	WAN_ASSERT(hwcard == NULL);
	hwusb = &hwcard->u_usb;
	*data = 0xFF;
	if (!wan_test_bit(WP_USB_STATUS_READY, &hwusb->status)){
		DEBUG_USB("%s: WARNING: RX USB core is not ready (%ld)!\n",
				hwcard->name,
				(unsigned long)SYSTEM_TICKS);
		hwusb->stats.core_notready_cnt++;
		return -ENODEV;
	}
	wan_spin_lock(&hwusb->cmd_lock, &flags);
	DEBUG_TX("%s: Tx Read CPU register (0x%02X:ticks=%ld)!\n", 
			hwcard->name,(unsigned char)off,SYSTEM_TICKS);
	if (wan_test_and_set_bit(WP_USB_STATUS_TX_CMD, &hwusb->status)){
		DEBUG_USB("%s: WARNING: USB CPU Read Command Overrun (Read command in process)!\n",
					hwcard->name);
		hwusb->stats.cmd_overrun++;
		err = -EBUSY;
		goto cpu_read_done;
	}
	
	if (!wan_skb_queue_len(&hwusb->tx_cmd_free_list)){
		DEBUG_USB("%s: WARNING: USB CPU Read Command Overrun (%d commands in process)!\n",
					hwcard->name, wan_skb_queue_len(&hwusb->tx_cmd_list));
		hwusb->stats.cmd_overrun++;
		err = -EBUSY;
		goto cpu_read_done;
	}	
	skb = wan_skb_dequeue(&hwusb->tx_cmd_free_list);
	cmd_data = wan_skb_put(skb, 2);
	cmd_data[0] = WP_USB_CMD_TYPE_ENCODE(WP_USB_CMD_TYPE_READ_CPU);
	cmd_data[1] = off;
	wan_skb_queue_tail(&hwusb->tx_cmd_list, skb);
	hwusb->tx_cmd_start = SYSTEM_TICKS;

	//WP_DELAY(20000);
	do {
		wait_just_a_bit(SDLA_USBFXO_READ_DELAY, 1);	//WP_DELAY(10000);
		if (++retry > SDLA_USBFXO_READ_RETRIES) break;
	} while(!wan_skb_queue_len(&hwusb->rx_cmd_list));
	if (!wan_skb_queue_len(&hwusb->rx_cmd_list)){
		DEBUG_USB("%s: WARNING: Timeout on Read USB-CPU Reg (%d:%02X:%ld:%ld:%ld)!\n",
					hwcard->name, retry,off,
					(unsigned long)(SYSTEM_TICKS-hwusb->tx_cmd_start),
					(unsigned long)hwusb->tx_cmd_start,
					(unsigned long)SYSTEM_TICKS);
		hwusb->stats.cmd_timeout++;
		err = -EINVAL;
		goto cpu_read_done;
	}
	skb = wan_skb_dequeue(&hwusb->rx_cmd_list);
	cmd_data = wan_skb_data(skb);
	if (cmd_data[1] != off){
		DEBUG_USB("%s: USB Read response is out of order (%02X:%02X)!\n",
				hwcard->name, cmd_data[1], off);
		hwusb->stats.cmd_invalid++;
		wan_skb_init(skb, 0);
		wan_skb_queue_tail(&hwusb->rx_cmd_free_list, skb);
		err = -EINVAL;
		goto cpu_read_done;
	}
	*data = (unsigned char)cmd_data[2];

	wan_skb_init(skb, 0);
	wan_skb_queue_tail(&hwusb->rx_cmd_free_list, skb);

cpu_read_done: 
	wan_clear_bit(WP_USB_STATUS_TX_CMD, &hwusb->status);
	wan_spin_unlock(&hwusb->cmd_lock, &flags);
	return err;
}

int sdla_usb_cpu_read(void *phw, unsigned char off, unsigned char *data)
{
	sdlahw_t	*hw = (sdlahw_t*)phw;
	sdlahw_card_t	*hwcard = NULL;
//	wan_smp_flag_t	flags; 

	WAN_ASSERT(hw == NULL);
	SDLA_MAGIC(hw);
	WAN_ASSERT(hw->hwcpu == NULL);
	WAN_ASSERT(hw->hwcpu->hwcard == NULL);

	hwcard = hw->hwcpu->hwcard;
	return __sdla_usb_cpu_read(hwcard, off, data);
}

static int __sdla_usb_cpu_write(sdlahw_card_t *hwcard, unsigned char off, unsigned char data)
{
	sdlahw_usb_t	*hwusb;
	netskb_t	*skb;
	u8		*cmd_data;
	wan_smp_flag_t	flags; 

	WAN_ASSERT(hwcard == NULL);
	hwusb = &hwcard->u_usb;
	wan_spin_lock(&hwusb->cmd_lock, &flags);

	DEBUG_TX("%s: Tx Write CPU register (0x%02X <- 0x%02X)!\n", 
			hw->devname, (unsigned char)off, (unsigned char)data);
	if (!wan_skb_queue_len(&hwusb->tx_cmd_free_list)){
		DEBUG_USB("%s: WARNING: USB CPU Write Command Overrun (%d commands in process)!\n",
					hwcard->name, wan_skb_queue_len(&hwusb->tx_cmd_list));
		hwusb->stats.cmd_overrun++;
		wan_spin_unlock(&hwusb->cmd_lock, &flags);
		return -EINVAL;
	}	
	skb = wan_skb_dequeue(&hwusb->tx_cmd_free_list);
	cmd_data = wan_skb_put(skb, 3);
	cmd_data[0] = WP_USB_CMD_TYPE_ENCODE(WP_USB_CMD_TYPE_WRITE_CPU);
	cmd_data[1] = off; 
	cmd_data[2] = data; 
	wan_skb_queue_tail(&hwusb->tx_cmd_list, skb);

	wan_spin_unlock(&hwusb->cmd_lock, &flags);
	return 0;
}

int sdla_usb_cpu_write(void *phw, unsigned char off, unsigned char data)
{
	sdlahw_t	*hw = (sdlahw_t*)phw;
	sdlahw_card_t	*hwcard = NULL;

	WAN_ASSERT(hw == NULL);
	SDLA_MAGIC(hw);
	WAN_ASSERT(hw->hwcpu == NULL);
	WAN_ASSERT(hw->hwcpu->hwcard == NULL);
	hwcard = hw->hwcpu->hwcard;

	return __sdla_usb_cpu_write(hwcard, off, data);
}

int sdla_usb_write_poll(void *phw, unsigned char off, unsigned char data)
{
	return 0;
}

int sdla_usb_read_poll(void *phw, unsigned char off, unsigned char *data)
{
	sdlahw_usb_t	*hwusb = NULL;
	sdlahw_t	*hw = (sdlahw_t*)phw;
	sdlahw_card_t	*hwcard = NULL;
	netskb_t	*skb;
	u8		*cmd_data;

	WAN_ASSERT(hw == NULL);
	SDLA_MAGIC(hw);
	WAN_ASSERT(hw->hwcpu == NULL);
	WAN_ASSERT(hw->hwcpu->hwcard == NULL);
	hwcard = hw->hwcpu->hwcard;
	hwusb = &hwcard->u_usb;
	if (!wan_skb_queue_len(&hwusb->rx_cmd_list)){
		DEBUG_EVENT("WARNING: %s: Timeout on Read USB Reg!\n",
					hw->devname);
		return -EBUSY;
	}	
	skb = wan_skb_dequeue(&hwusb->rx_cmd_list);
	cmd_data = wan_skb_data(skb);
	*data = (unsigned char)cmd_data[2];
	wan_skb_init(skb, 0);
	wan_skb_queue_tail(&hwusb->rx_cmd_free_list, skb);
	return 0;
}

int sdla_usb_rxevent_enable(void *phw, int mod_no, int enable)
{
	sdlahw_usb_t	*hwusb = NULL;
	sdlahw_t	*hw = (sdlahw_t*)phw;
	sdlahw_card_t	*hwcard = NULL;
	int		event_bit;
	u8		mask;

	WAN_ASSERT(hw == NULL);
	SDLA_MAGIC(hw);
	WAN_ASSERT(hw->hwcpu == NULL);
	WAN_ASSERT(hw->hwcpu->hwcard == NULL);
	hwcard = hw->hwcpu->hwcard;
	hwusb = &hwcard->u_usb;
	if (mod_no) mask = SDLA_USB_CPU_BIT_CTRL_TS1_EVENT_EN;
	else mask = SDLA_USB_CPU_BIT_CTRL_TS0_EVENT_EN;
	if (enable) hwusb->reg_cpu_ctrl |= mask;
	else hwusb->reg_cpu_ctrl &= ~mask; 
	sdla_usb_cpu_write(hw, SDLA_USB_CPU_REG_CTRL, hwusb->reg_cpu_ctrl);

	event_bit = (mod_no) ? WP_USB_STATUS_RX_EVENT2_READY : WP_USB_STATUS_RX_EVENT1_READY;
	wan_set_bit(event_bit, &hwusb->status);
	DEBUG_USB("%s: Module %d: %s RX Events (%02X:%ld)!\n",
				hw->devname, mod_no+1, 
				(enable) ? "Enable" : "Disable", hwusb->reg_cpu_ctrl,
				(unsigned long)SYSTEM_TICKS);
	return 0;
}

int sdla_usb_hwec_enable(void *phw, int mod_no, int enable)
{
	sdlahw_usb_t	*hwusb = NULL;
	sdlahw_t	*hw = (sdlahw_t*)phw;
	sdlahw_card_t	*hwcard = NULL;
	u8		mask;

	WAN_ASSERT(hw == NULL);
	SDLA_MAGIC(hw);
	WAN_ASSERT(hw->hwcpu == NULL);
	WAN_ASSERT(hw->hwcpu->hwcard == NULL);
	hwcard = hw->hwcpu->hwcard;
	hwusb = &hwcard->u_usb;
	if (mod_no) mask = SDLA_USB_CPU_BIT_CTRL_TS1_HWEC_EN;
	else mask = SDLA_USB_CPU_BIT_CTRL_TS0_HWEC_EN;
	if (enable) hwusb->reg_cpu_ctrl |= mask;
	else hwusb->reg_cpu_ctrl &= ~mask; 
	sdla_usb_cpu_write(hw, SDLA_USB_CPU_REG_CTRL, hwusb->reg_cpu_ctrl);

	DEBUG_USB("%s: Module %d: %s hw echo canceller!\n",
				hw->devname, mod_no+1, 
				(enable) ? "Enable" : "Disable");
	return 0;
}
int sdla_usb_rxevent(void *phw, int mod_no, u8 *regs, int force)
{
	sdlahw_t	*hw = (sdlahw_t*)phw;
	sdlahw_usb_t	*hwusb = NULL;
	sdlahw_card_t	*hwcard = NULL;
	int		event_bit;

	WAN_ASSERT(hw == NULL);
	SDLA_MAGIC(hw);
	WAN_ASSERT(hw->hwcpu == NULL);
	WAN_ASSERT(hw->hwcpu->hwcard == NULL);
	hwcard = hw->hwcpu->hwcard;
	hwusb = &hwcard->u_usb;
	event_bit = (mod_no) ? WP_USB_STATUS_RX_EVENT2_READY : WP_USB_STATUS_RX_EVENT1_READY;
	if(!wan_test_bit(event_bit, &hwusb->status)){
		return -EINVAL;
	}
	if (force){
		hwusb->regs[mod_no][5]  = __sdla_usb_fxo_read(hwcard, mod_no, 5);
		hwusb->regs[mod_no][29] = __sdla_usb_fxo_read(hwcard, mod_no, 29);
		hwusb->regs[mod_no][34] = __sdla_usb_fxo_read(hwcard, mod_no, 34);
		hwusb->regs[mod_no][4]  = __sdla_usb_fxo_read(hwcard, mod_no, 4);
		DEBUG_USB("%s: Module %d: RX Event Init (%02X:%02X:%02X:%02X)...\n",
				hw->devname, mod_no+1,
				hwusb->regs[mod_no][5], hwusb->regs[mod_no][29],
				hwusb->regs[mod_no][34], hwusb->regs[mod_no][4]);
	}
	regs[0] = hwusb->regs[mod_no][5];
	regs[1] = hwusb->regs[mod_no][29];
	regs[2] = hwusb->regs[mod_no][34];
	regs[3] = hwusb->regs[mod_no][4];
	return 0;
}

int sdla_usb_rxtx_data_init(void *phw, int mod_no, unsigned char ** rxdata, unsigned char **txdata)
{
	sdlahw_t	*hw = (sdlahw_t*)phw;
	sdlahw_card_t	*hwcard = NULL;

	WAN_ASSERT(hw == NULL);
	SDLA_MAGIC(hw);
	WAN_ASSERT(hw->hwcpu == NULL);
	WAN_ASSERT(hw->hwcpu->hwcard == NULL);
	hwcard = hw->hwcpu->hwcard;

	*rxdata = &hwcard->u_usb.readchunk[mod_no][0];
	*txdata = &hwcard->u_usb.writechunk[mod_no][0];
	return 0;
}

int sdla_usb_rxdata_enable(void *phw, int enable)
{
	sdlahw_t	*hw = (sdlahw_t*)phw;
	sdlahw_card_t	*hwcard = NULL;

	WAN_ASSERT(hw == NULL);
	SDLA_MAGIC(hw);
	WAN_ASSERT(hw->hwcpu == NULL);
	WAN_ASSERT(hw->hwcpu->hwcard == NULL);
	hwcard = hw->hwcpu->hwcard;

	if (enable){
		wan_set_bit(WP_USB_STATUS_RX_DATA_READY, &hwcard->u_usb.status);
	}else{
		wan_clear_bit(WP_USB_STATUS_RX_DATA_READY, &hwcard->u_usb.status);
	}
	return 0;
}

int sdla_usb_fwupdate_enable(void *phw)
{
	sdlahw_usb_t	*hwusb = NULL;
	sdlahw_t	*hw = (sdlahw_t*)phw;
	sdlahw_card_t	*hwcard = NULL;
	unsigned char	data[1];

	WAN_ASSERT(hw == NULL);
	SDLA_MAGIC(hw);
	WAN_ASSERT(hw->hwcpu == NULL);
	WAN_ASSERT(hw->hwcpu->hwcard == NULL);
	hwcard = hw->hwcpu->hwcard;
	hwusb = &hwcard->u_usb;

	/* Send Firmware update command */
	data[0] = SDLA_USB_CPU_BITS_FWUPDATE_MAGIC;
	sdla_usb_cpu_write(phw, SDLA_USB_CPU_REG_FWUPDATE_MAGIC, SDLA_USB_CPU_BITS_FWUPDATE_MAGIC);
	data[0] = SDLA_USB_CPU_BIT_CTRL_FWUPDATE;
	sdla_usb_cpu_write(phw, SDLA_USB_CPU_REG_CTRL, SDLA_USB_CPU_BIT_CTRL_FWUPDATE);
	wait_just_a_bit(SDLA_USBFXO_WRITE_DELAY, 0);
	DEBUG_EVENT("%s: Enabling Firmware Update mode...\n",
				  hwcard->name);
	hwusb->opmode = SDLA_USB_OPMODE_API;

	/* Initialize rx urb */
	sdla_usb_rxurb_reset(hwcard);
	sdla_usb_txurb_reset(hwcard);

	return 0;
}

int sdla_usb_txdata_raw(void *phw, unsigned char *data, int max_len)
{
	sdlahw_t	*hw = (sdlahw_t*)phw;
	sdlahw_card_t	*hwcard = NULL;
	int		err = 0;

	WAN_ASSERT(hw == NULL);
	SDLA_MAGIC(hw);
	WAN_ASSERT(hw->hwcpu == NULL);
	WAN_ASSERT(hw->hwcpu->hwcard == NULL);
	hwcard = hw->hwcpu->hwcard;

	if (max_len > MAX_USB_TX_LEN){
		max_len = MAX_USB_TX_LEN;
	}
	err = sdla_usb_txdata_prepare(hwcard, data, max_len);
	return (!err) ? max_len : 0;
}

int sdla_usb_txdata_raw_ready(void *phw)
{
	sdlahw_t			*hw = (sdlahw_t*)phw;
	sdlahw_usb_t			*hwusb = NULL;
	sdlahw_card_t			*hwcard = NULL;

	WAN_ASSERT(hw == NULL);
	SDLA_MAGIC(hw);
	WAN_ASSERT(hw->hwcpu == NULL);
	WAN_ASSERT(hw->hwcpu->hwcard == NULL);
	hwcard = hw->hwcpu->hwcard;
	hwusb = &hwcard->u_usb;

	if (!wan_test_bit(WP_USB_STATUS_TX_READY, &hwusb->status)){
		DEBUG_TX("%s:%d: Tx Data is busy (%ld)!\n",
				hw->devname, hwcard->u_usb.datawrite[urb_write_ind].id,
				(unsigned long)SYSTEM_TICKS);
		return -EBUSY;
	}
	return 0;
}

int sdla_usb_rxdata_raw(void *phw, unsigned char *data, int max_len)
{
	sdlahw_t	*hw = (sdlahw_t*)phw;
	sdlahw_card_t	*hwcard = NULL;
	int		rxlen = 0;

	WAN_ASSERT(hw == NULL);
	SDLA_MAGIC(hw);
	WAN_ASSERT(hw->hwcpu == NULL);
	WAN_ASSERT(hw->hwcpu->hwcard == NULL);
	hwcard = hw->hwcpu->hwcard;

	if (max_len > MAX_USB_RX_LEN){
		max_len = MAX_USB_RX_LEN;
	}
	rxlen = sdla_usb_rxdata_get(hwcard, data, max_len);
	return rxlen;
}


int sdla_usb_set_intrhand(void* phw, wan_pci_ifunc_t *isr_func, void* arg, int notused)
{
	sdlahw_t	*hw = (sdlahw_t*)phw;
	sdlahw_card_t	*hwcard = NULL;

	WAN_ASSERT(hw == NULL);
	SDLA_MAGIC(hw);
	WAN_ASSERT(hw->hwcpu == NULL);
	WAN_ASSERT(hw->hwcpu->hwcard == NULL);
	hwcard = hw->hwcpu->hwcard;

	hwcard->u_usb.isr_func	= isr_func;
	hwcard->u_usb.isr_arg	= arg;
	return 0;
}

int sdla_usb_restore_intrhand(void* phw, int notused)
{
	sdlahw_t	*hw = (sdlahw_t*)phw;
	sdlahw_card_t	*hwcard = NULL;

	WAN_ASSERT(hw == NULL);
	SDLA_MAGIC(hw);
	WAN_ASSERT(hw->hwcpu == NULL);
	WAN_ASSERT(hw->hwcpu->hwcard == NULL);
	hwcard = hw->hwcpu->hwcard;

	hwcard->u_usb.isr_func	= NULL;
	hwcard->u_usb.isr_arg	= NULL;
	return 0;
}

int sdla_usb_err_stats(void *phw,void *buf,int len)
{
	sdlahw_t			*hw = (sdlahw_t*)phw;
	sdlahw_usb_t			*hwusb = NULL;
	sdlahw_card_t			*hwcard = NULL;
	sdla_usb_comm_err_stats_t	*stats;

	WAN_ASSERT(hw == NULL);
	SDLA_MAGIC(hw);
	WAN_ASSERT(hw->hwcpu == NULL);
	WAN_ASSERT(hw->hwcpu->hwcard == NULL);
	hwcard = hw->hwcpu->hwcard;
	hwusb = &hwcard->u_usb;
	if (len != sizeof(sdla_usb_comm_err_stats_t)){
		DEBUG_EVENT("%s: Invalid stats structure (%d:%d)!\n",
				hw->devname, len, (int)sizeof(sdla_usb_comm_err_stats_t)); 
		return -EINVAL;
	}
	stats = &hwusb->stats;
	sdla_usb_cpu_read(hw, SDLA_USB_CPU_REG_FIFO_STATUS, &stats->dev_fifo_status);
	sdla_usb_cpu_read(hw, SDLA_USB_CPU_REG_UART_STATUS, &stats->dev_uart_status);
	sdla_usb_cpu_read(hw, SDLA_USB_CPU_REG_HOSTIF_STATUS, &stats->dev_hostif_status);

	memcpy(buf, stats,sizeof(sdla_usb_comm_err_stats_t));
	return 0; 
}

int sdla_usb_flush_err_stats(void *phw)
{
	sdlahw_t	*hw = (sdlahw_t*)phw;
	sdlahw_card_t	*hwcard = NULL;

	WAN_ASSERT(hw == NULL);
	SDLA_MAGIC(hw);
	WAN_ASSERT(hw->hwcpu == NULL);
	WAN_ASSERT(hw->hwcpu->hwcard == NULL);
	hwcard = hw->hwcpu->hwcard;

	memset(&hwcard->u_usb.stats,0,sizeof(sdla_usb_comm_err_stats_t));
	return 0; 
}

static unsigned int	bhcount=0, rxcount=0, txcount=0;

static int sdla_usb_rxcmd_decode(sdlahw_card_t *hwcard, u8 *rx_cmd, int rx_cmd_len)
{
	sdlahw_usb_t	*hwusb = NULL;
	netskb_t	*skb;
	u_int8_t	*cmd;
	u_int8_t	reg_no;
	int		mod_no;

	WAN_ASSERT(hwcard == NULL);
	hwusb = &hwcard->u_usb;
	switch(WP_USB_CMD_TYPE_DECODE(rx_cmd[0])){
	case WP_USB_CMD_TYPE_WRITE_CPU:
		DEBUG_RX("%s: Rx Write CPU register (%02X:%02X)!\n", 
					hwcard->name,
					(unsigned char)rx_cmd[1],
					(unsigned char)rx_cmd[2]);
		break;			
	case WP_USB_CMD_TYPE_READ_CPU:
		DEBUG_RX("%s: Rx Read CPU register (%02X:%02X)!\n", 
					hwcard->name,
					(unsigned char)rx_cmd[1],
					(unsigned char)rx_cmd[2]);
		break;
	case WP_USB_CMD_TYPE_WRITE_FXO:
		DEBUG_RX("%s: Rx Write FXO register (%02X:%02X)!\n", 
					hwcard->name,
					(unsigned char)rx_cmd[1],
					(unsigned char)rx_cmd[2]);
		break;
	case WP_USB_CMD_TYPE_READ_FXO:
		DEBUG_RX("%s: Rx Read FXO register (%d:%02X)!\n", 
					hwcard->name,
					(unsigned char)rx_cmd[1],
					(unsigned char)rx_cmd[2]);
		break;
	case WP_USB_CMD_TYPE_EVENT:
		mod_no = rx_cmd[0] & 0x01;
		reg_no = rx_cmd[1];
		DEBUG_RX("%s: Module %d: Rx Event Info (Reg:%d = %02X:%ld)!\n", 
					hwcard->name, mod_no+1, 
					reg_no, rx_cmd[2],
					(unsigned long)SYSTEM_TICKS);
		hwusb->regs[mod_no][reg_no] = rx_cmd[2];
		return 0;
	default:
		DEBUG_USB("%s: Unknown command %X : %02X!\n",
				hwcard->name,
				WP_USB_CMD_TYPE_DECODE(rx_cmd[0]), rx_cmd[0]);
		hwusb->stats.rx_cmd_unknown++;
		return -EINVAL;
	}
	skb = wan_skb_dequeue(&hwusb->rx_cmd_free_list);
	cmd = wan_skb_put(skb, rx_cmd_len);
	memcpy(cmd, &rx_cmd[0], rx_cmd_len);
	wan_skb_queue_tail(&hwusb->rx_cmd_list, skb);
	return 0;
}

static int 
sdla_usb_rxdata_get(sdlahw_card_t *hwcard, unsigned char *rxdata, int max_len)
{
	sdlahw_usb_t	*hwusb = NULL;
	int		rx_len, next_rx_ind = 0, next_read_ind;
	
	WAN_ASSERT(hwcard == NULL);
	hwusb = &hwcard->u_usb;

	next_rx_ind	= hwusb->next_rx_ind;
	next_read_ind	= hwusb->next_read_ind;
	if (next_rx_ind > next_read_ind){
		rx_len = next_rx_ind - next_read_ind; 
	}else if (next_rx_ind < next_read_ind){
		rx_len = MAX_READ_BUF_LEN - next_read_ind + next_rx_ind;
	}else{
		return 0;
	}
	if (rxdata == NULL){
		/* ignore previously received data */
		hwusb->next_read_ind = (next_read_ind + rx_len) % MAX_READ_BUF_LEN;
		return rx_len;
	}
	if (rx_len > max_len) rx_len = max_len;
	if (rx_len > MAX_USB_RX_LEN) rx_len = MAX_USB_RX_LEN;

	if (next_read_ind + rx_len < MAX_READ_BUF_LEN){
		memcpy(rxdata, &hwusb->readbuf[next_read_ind], rx_len);
	}else{
		int len = MAX_READ_BUF_LEN - next_read_ind;
		memcpy(rxdata, &hwusb->readbuf[next_read_ind], len);
		memcpy(&rxdata[len], &hwusb->readbuf[0], rx_len-len);
	}
	hwusb->next_read_ind = (next_read_ind + rx_len) % MAX_READ_BUF_LEN;
	return rx_len;
}

static int sdla_usb_rxdata_decode (sdlahw_card_t *hwcard, unsigned char *rxdata, int len)
{
	sdlahw_usb_t	*hwusb = NULL;
	int		x = 0, mod_no=0, ind=0, high=0, shift=0;
	unsigned char	data, cmd, start_bit, start_fr_bit;
	u_int8_t	*rx_data[2];
	u_int8_t	rx_cmd[10];
	int		rx_cmd_len= 0;

	WAN_ASSERT(hwcard == NULL);
	hwusb = &hwcard->u_usb;

	rx_data[0] = hwusb->readchunk[0];
	rx_data[1] = hwusb->readchunk[1];

	//memset(&hwusb->readchunk[0][0], 0, WP_USB_MAX_CHUNKSIZE);
	//memset(&hwusb->readchunk[1][0], 0, WP_USB_MAX_CHUNKSIZE);
	memset(&rx_cmd[0],0, 10);
	x = 0;
	while(x < MAX_USB_RX_LEN){

		DEBUG_RX("  RX: %02X\n", (unsigned char)rxdata[x]);
		data		= rxdata[x] & 0x0F;
		start_bit	= rxdata[x] & 0x10;
		start_fr_bit	= rxdata[x] & 0x20;

		ind	= x / 4;
		mod_no	= x % 2;
		high	= (x % 4) / 2;
		/* decode voice traffic */
		if (mod_no == 0 && high == 0 && (ind % WP_USB_CHUNKSIZE) == 0){
			if (!start_fr_bit){
				DEBUG_USB(
				"%s: ERROR:%d: Start Frame bit missing (%d:%d:%02X:%ld)\n",
						hwcard->name,
						bhcount,x,ind,
						(unsigned char)rxdata[x],
						(unsigned long)SYSTEM_TICKS);
				hwusb->stats.rx_start_fr_err_cnt++;
				break;
			}
		}
		if (x % 4 == 0){
			if (!start_bit){
				DEBUG_USB(
				"%s: ERROR:%d: Start bit missing (%d:%02X:%ld)\n",
						hwcard->name,
						bhcount,x,
						(unsigned char)rxdata[x],
						(unsigned long)SYSTEM_TICKS);
				hwusb->stats.rx_start_err_cnt++;
				break;
			}
		}
		DEBUG_RX("x:%d mod:%d ind:%d high:%d data:%02X\n",
					x, mod_no, ind, high,
					(unsigned char)rxdata[x]);
		if (high){
			rx_data[mod_no][ind] |= (data << 4);	//sc->readchunk[mod_no][ind] |= (data << 4);
		}else{
			rx_data[mod_no][ind] = data;	//sc->readchunk[mod_no][ind] = data;
		}

		/* decode control command (2 bits) */
		cmd = rxdata[x] & 0xC0;

		ind = x / 4;
		shift = (x % 4) * 2;
		cmd = cmd >> shift;
		if (ind && shift == 0){
			if (rx_cmd[rx_cmd_len] == hwusb->ctrl_idle_pattern){
				DEBUG_RX("RX_CMD: x:%d ind:%d shift:%d len:%d ctrl_idle_pattern\n",
							x, ind, shift, rx_cmd_len);
				rx_cmd[rx_cmd_len] = 0x00;
				if (rx_cmd_len){
					if (!wan_skb_queue_len(&hwusb->rx_cmd_free_list)){
						DEBUG_USB("%s: INTERNAL ERROR:%d: Received too many commands!\n",
								hwcard->name, (u32)SYSTEM_TICKS);
						hwusb->stats.rx_cmd_drop_cnt++;
						rx_cmd_len = 0;
					}
					if (rx_cmd_len >= 3){
						sdla_usb_rxcmd_decode(hwcard, rx_cmd, rx_cmd_len);
					}else{
						hwusb->stats.rx_cmd_reset_cnt++;
						if (rx_cmd_len == 1){
							DEBUG_USB("%s: Reset RX Cmd (%d:%02X:%02X)!\n",	
								hwcard->name, rx_cmd_len,
								WP_USB_CMD_TYPE_DECODE(rx_cmd[0]), rx_cmd[0]);
						}else{
							DEBUG_USB("%s: Reset RX Cmd (%d:%02X:%02X:%02X)!\n",	
								hwcard->name, rx_cmd_len,
								WP_USB_CMD_TYPE_DECODE(rx_cmd[0]), rx_cmd[0],rx_cmd[1]);
						}
					}
					memset(&rx_cmd[0],0,rx_cmd_len);
					rx_cmd_len = 0;
				}
			}else{
				DEBUG_RX("RX_CMD: x:%d ind:%d shift:%d cmd:%02X\n",
					x, ind, shift, (unsigned char)rx_cmd[rx_cmd_len]);
				rx_cmd_len++;
			}
		}
		rx_cmd[rx_cmd_len] |= cmd;
		DEBUG_RX("RX_CMD: x:%d ind:%d shift:%d cmd:%02X:%02X\n",
					x, ind, shift, (unsigned char)cmd,
					(unsigned char)rx_cmd[rx_cmd_len]);
		x++;
	}
	return 0;
}

static int
sdla_usb_txdata_prepare(sdlahw_card_t *hwcard, char *writebuf, int len)
{
	sdlahw_usb_t	*hwusb = NULL;
	int		next_write_ind, urb_write_ind; 
	int		err;

	WAN_ASSERT(hwcard == NULL);
	WAN_ASSERT(len > MAX_USB_TX_LEN);
	hwusb = &hwcard->u_usb;
	
	next_write_ind = hwusb->next_write_ind;
	memcpy(&hwusb->writebuf[next_write_ind], writebuf, len); 

	next_write_ind = (next_write_ind + len) % MAX_WRITE_BUF_LEN;  
	if (next_write_ind == hwusb->next_tx_ind){
		DEBUG_USB("ERROR:%d: TX BH is too fast (%d:%ld)\n",
					bhcount,
					next_write_ind, 
					(unsigned long)SYSTEM_TICKS);
		hwusb->stats.tx_overrun_cnt++;
	}else{
		hwusb->next_write_ind = next_write_ind;
	}

	urb_write_ind = (hwusb->urb_write_ind + 1) % MAX_WRITE_URB_COUNT;
#if 1
	if (!wan_test_bit(SDLA_URB_STATUS_READY, &hwusb->datawrite[urb_write_ind].ready)){
		/* Use previous urb for now and increment errors */
		DEBUG_TX("%s: [BH:%d:%d:%d]: TX is not ready (%ld)!\n",
				hwcard->name, 
				bhcount,rxcount,txcount,
				(unsigned long)SYSTEM_TICKS);
		hwusb->stats.tx_overrun_cnt++;
		return -EINVAL;
	}
	/* Update tx urb */
	hwusb->datawrite[urb_write_ind].urb.transfer_buffer = &hwusb->writebuf[hwusb->next_tx_ind];
	hwusb->datawrite[urb_write_ind].urb.transfer_buffer_length = len;
	hwusb->next_tx_ind += len;
	hwusb->next_tx_ind = hwusb->next_tx_ind % MAX_WRITE_BUF_LEN;
	wan_clear_bit(WP_USB_STATUS_TX_READY, &hwusb->status);	//sc->tx_ready = 0;
	if ((err = wp_usb_start_transfer(&hwusb->datawrite[urb_write_ind]))){
		if (err != -ENOENT){
			DEBUG_EVENT("%s: Failed to program transmitter (%ld)!\n",
					hwcard->name,
					(unsigned long)SYSTEM_TICKS);
		}
		hwusb->stats.tx_notready_cnt++;
		return -EINVAL;
	}
	hwusb->urb_write_ind = urb_write_ind;

#else
	if (wan_test_bit(1, &hwusb->datawrite[urb_write_ind].ready)){
		/* Update tx urb */
		hwusb->datawrite[urb_write_ind].urb.transfer_buffer = &hwusb->writebuf[hwusb->next_tx_ind];
		hwusb->next_tx_ind += len;
		hwusb->next_tx_ind = hwusb->next_tx_ind % MAX_WRITE_BUF_LEN;
	}else{
		/* Use previous urb for now and increment errors */
		DEBUG_USB("%s: [BH:%d:%d:%d]: TX is not ready (%ld)!\n",
				hwcard->name, 
				bhcount,rxcount,txcount,
				(unsigned long)SYSTEM_TICKS);
		hwusb->stats.tx_overrun_cnt++;
	}
	wan_clear_bit(WP_USB_STATUS_TX_READY, &hwusb->status);	//sc->tx_ready = 0;
	if ((err = wp_usb_start_transfer(&hwusb->datawrite[urb_write_ind]))){
		if (err != -ENOENT){
			DEBUG_EVENT("%s: Failed to program transmitter (%ld)!\n",
					hwcard->name,
					(unsigned long)SYSTEM_TICKS);
		}
		hwusb->stats.tx_notready_cnt++;
		return -EINVAL;
	}
	hwusb->urb_write_ind = urb_write_ind;
#endif
	return 0;
}

static int
sdla_usb_txdata_encode (sdlahw_card_t *hwcard, unsigned char *writebuf, int len)
{
	sdlahw_usb_t	*hwusb = NULL;
	int		off, x = 0, ind, mod_no, tx_idle = 0;
	unsigned char	data, start_bit;
	unsigned char	*txdata[2];

	WAN_ASSERT(hwcard == NULL);
	hwusb = &hwcard->u_usb;

	txdata[0] = hwusb->writechunk[0];
	txdata[1] = hwusb->writechunk[1];
	memcpy(writebuf, &hwusb->idlebuf[0], MAX_USB_TX_LEN); 
	for (x = 0; x < WP_USB_RXTX_CHUNKSIZE; x++) {
		for (mod_no = 0; mod_no < 2; mod_no++) {
			start_bit = 0x00;
			if (tx_idle){
				data = WP_USB_IDLE_PATTERN;
			}else{
				data = *(txdata[mod_no] + x);
			}
	
			ind = x * 4 + mod_no;
			WP_USB_CTRL_ENCODE(writebuf, ind);
			/* write loq nibble */
			WP_USB_DATA_ENCODE(&writebuf[ind], data);
			DEBUG_TX("TX: x:%d mod:%d ind:%d:%d data:%02X:%02X:%02X\n",
						x, mod_no, ind, ind+2,
						(unsigned char)data,
						(unsigned char)writebuf[ind],
						(unsigned char)writebuf[ind+2]);
		}
	}	

	if (wan_skb_queue_len(&hwusb->tx_cmd_list)){
		u8		*cmd;
		int		cmd_len;
		netskb_t	*skb;
	
		skb = wan_skb_dequeue(&hwusb->tx_cmd_list);
		cmd = wan_skb_data(skb);
		cmd_len = wan_skb_len(skb);
		off = 0;

		off++;
		for (x=0; x < cmd_len; x++,off++) {
			WP_USB_CMD_ENCODE(&writebuf[off*4], cmd[x]);
			DEBUG_TX("TX_CMD: off:%d byte:%02X %02X:%02X:%02X:%02X\n",
					off, cmd[x],
					(unsigned char)writebuf[off*4],
					(unsigned char)writebuf[off*4+1],
					(unsigned char)writebuf[off*4+2],
					(unsigned char)writebuf[off*4+3]);
		}
		wan_skb_init(skb, 0);
		wan_skb_queue_tail(&hwusb->tx_cmd_free_list, skb);
	}
	return 0;
}

static int
sdla_usb_bh_rx (sdlahw_card_t *hwcard)
{
	sdlahw_usb_t	*hwusb = NULL;
	int		rx_len;
	u_int8_t	readbuf[MAX_USB_RX_LEN];

	WAN_ASSERT(hwcard == NULL);
	hwusb = &hwcard->u_usb;

	DEBUG_USB("[BH-RX] %s:%d: RX:%d (%d:%d)\n",
			__FUNCTION__,__LINE__,rx_len, 
			hwusb->next_read_ind, hwusb->next_rx_ind);

	rx_len = sdla_usb_rxdata_get(hwcard, &readbuf[0], MAX_USB_RX_LEN);
	if (!rx_len){
		DEBUG_USB("%s: INTERNAL ERROR:%d: No data available!\n",
					hwcard->name, (u32)SYSTEM_TICKS);
		hwusb->stats.rx_underrun_cnt++;
		return -EINVAL;
	}

	if (rx_len < MAX_USB_RX_LEN){
		DEBUG_USB("%s: ERROR: Not enough data (%d:%d:%d:%d)!\n",
				hwcard->name, rx_len,
				hwusb->next_read_ind,
				hwusb->next_rx_ind,
				MAX_READ_BUF_LEN);
		hwusb->stats.rx_underrun_cnt++;
		return 0;
	}

	sdla_usb_rxdata_decode(hwcard, readbuf, MAX_USB_RX_LEN);
	if (wan_test_bit(WP_USB_STATUS_RX_DATA_READY, &hwusb->status) && hwusb->isr_func){
		hwusb->isr_func(hwusb->isr_arg);
	}	
	return 0;
}


static int sdla_usb_bh_tx (sdlahw_card_t *hwcard)
{	
	sdlahw_usb_t	*hwusb = NULL;
	unsigned char	writebuf[MAX_USB_TX_LEN];

	WAN_ASSERT(hwcard == NULL);
	hwusb = &hwcard->u_usb;

	if (hwusb->opmode == SDLA_USB_OPMODE_VOICE){
		DEBUG_USB("[BH-TX] %s:%d: TX:%d (%d:%d)\n",
			__FUNCTION__,__LINE__,MAX_USB_TX_LEN,
			hwusb->next_tx_ind, hwusb->next_write_ind);

		sdla_usb_txdata_encode(hwcard, writebuf, MAX_USB_TX_LEN);

 		if (sdla_usb_txdata_prepare(hwcard, writebuf, MAX_USB_TX_LEN)){
			return -EINVAL;
		}
	}
	return 0;
}

#if defined(__LINUX__)
static void sdla_usb_bh (unsigned long data)
#else
static void sdla_usb_bh (void *data, int pending)
#endif
{
	sdlahw_card_t	*hwcard = (sdlahw_card_t*)data;
	sdlahw_usb_t	*hwusb = NULL;
//	wan_smp_flag_t flags;
	
	WAN_ASSERT_VOID(hwcard == NULL);
	hwusb = &hwcard->u_usb;
 	DEBUG_TEST("%s:%d: ------------ BEGIN --------------: %ld\n",
			__FUNCTION__,__LINE__,(unsigned long)SYSTEM_TICKS);
	if (!wan_test_bit(WP_USB_STATUS_READY, &hwusb->status)){
		goto usb_bh_exit;
	}
	//wan_spin_lock_irq(&hwusb->lock,&flags);
	if (wan_test_and_set_bit(WP_USB_STATUS_BH, &hwusb->status)){
		DEBUG_EVENT("%s: [BH:%ld]: Re-entry in USB BH!\n",
				hwcard->name, (unsigned long)SYSTEM_TICKS);
		goto usb_bh_done;
	}
	bhcount++;

	/* receive path */
	sdla_usb_bh_rx(hwcard);
	/* transmit path */
	sdla_usb_bh_tx(hwcard);

	wan_clear_bit(WP_USB_STATUS_BH, &hwusb->status);

usb_bh_done:
	//wan_spin_unlock_irq(&hwusb->lock,&flags);

usb_bh_exit:
	WAN_TASKLET_END((&hwusb->bh_task));

	DEBUG_TEST("%s: ------------ END -----------------: %ld\n",
                        __FUNCTION__,(unsigned long)SYSTEM_TICKS);
	return;
}

int sdlausb_syncverify(sdlahw_card_t *hwcard, int len)
{
	sdlahw_usb_t	*hwusb = NULL;
	unsigned char	data;

	WAN_ASSERT(hwcard == NULL);
	hwusb = &hwcard->u_usb;
	if (!hwusb->rx_sync){
	
		DEBUG_USB("%s: Searching for sync from %d...\n", 
					hwcard->name, hwusb->next_read_ind);
		do {
			if ((hwusb->readbuf[hwusb->next_read_ind] & 0x30) != 0x30){
				hwusb->next_read_ind = (hwusb->next_read_ind+1) % MAX_READ_BUF_LEN;
				len--;
				continue;
			}
			/* Found first sync condition */
			DEBUG_USB("%s: Got first sync condition at %d:%d!\n",
						hwcard->name,
						hwusb->next_read_ind,
						hwusb->next_rx_ind);
			if (len < 4) break;
			data = WP_USB_CTRL_DECODE(&hwusb->readbuf[hwusb->next_read_ind]);
			if (data == 0x7E){
				/* Found second sync condition */
				DEBUG_EVENT("%s: USB device is connected!\n",
							hwcard->name);
				DEBUG_USB("%s: Got in sync at %d:%d!\n",
							hwcard->name,
							hwusb->next_read_ind,
							hwusb->next_rx_ind);
				hwusb->rx_sync = 1;
				break;
			}
			hwusb->next_read_ind = (hwusb->next_read_ind+4) % MAX_READ_BUF_LEN;
			len -= 4;
		}while(len);
		//if (sc->next_read_ind >= MAX_READ_BUF_LEN){
		//	sc->next_read_ind = sc->next_read_ind % MAX_READ_BUF_LEN;
		//}
	}else{
		if ((hwusb->readbuf[hwusb->next_read_ind] & 0x30) != 0x30){
			DEBUG_EVENT("%s: USB device is disconnected!\n",
						hwcard->name);
			DEBUG_USB("%s: WARNING: Got out of sync 1 at %d:%d (%02X)!\n",
						hwcard->name,
						hwusb->next_read_ind, hwusb->next_rx_ind,
						(unsigned char)hwusb->readbuf[hwusb->next_read_ind]);
			hwusb->stats.rx_sync_err_cnt++;
	
			hwusb->rx_sync = 0;
			//sc->next_read_ind++;
			hwusb->next_read_ind = (hwusb->next_read_ind+1) % MAX_READ_BUF_LEN;
			return 0;
		}
		data = WP_USB_CTRL_DECODE(&hwusb->readbuf[hwusb->next_read_ind]);
		if (data != 0x7E){
			DEBUG_EVENT("%s: USB device is disconnected!\n",
						hwcard->name);
			DEBUG_USB("%s: WARNING: Got out of sync 2 at %d:%d!\n",
						hwcard->name,
						hwusb->next_read_ind,
						hwusb->next_rx_ind);
			hwusb->stats.rx_sync_err_cnt++;
			hwusb->rx_sync = 0;
			//sc->next_read_ind++;
			hwusb->next_read_ind = (hwusb->next_read_ind+1) % MAX_READ_BUF_LEN;
			return 0;
		}

		if (hwcard->core_rev >= 0x20){

			if ((hwusb->readbuf[hwusb->next_read_ind+1] & 0x20) == 0x20){
				if (hwcard->core_rev != 0xFF && !hwusb->tx_sync){
					DEBUG_USB("%s: USB device is in sync!\n",
							hwcard->name);
				}
				hwusb->tx_sync = 1;
			}else{
				if (hwcard->core_rev != 0xFF && hwusb->tx_sync){
					DEBUG_USB("%s: USB device is out of sync (%02X)!\n",
						hwcard->name,
						(unsigned char)hwusb->readbuf[hwusb->next_read_ind+1]);
					hwusb->stats.dev_sync_err_cnt++;
				}
				hwusb->tx_sync = 0;
			}
		}
	}
	return 1;
}

#if defined(LINUX26)  && (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,19))
void sdlausb_read_complete(struct urb *q, struct pt_regs *regs)
#else
void sdlausb_read_complete(struct urb *q)
#endif
{
	struct wan_urb	*wurb = (struct wan_urb*)q->context;
	sdlahw_card_t	*hwcard = (sdlahw_card_t*)wurb->pvt;
	sdlahw_usb_t	*hwusb = NULL;
	int		next_rx_ind = 0, next_rx_off = 0;
	int		err, len = 0;	// actual_length = MAX_USB_RX_LEN;
//	wan_smp_flag_t	flags; 

	WAN_ASSERT_VOID(hwcard == NULL);
	hwusb = &hwcard->u_usb;
	if (q->status){
		if (!(q->status == -ENOENT ||q->status == -ECONNRESET || q->status == -ESHUTDOWN)){
			DEBUG_EVENT("%s: ERROR: RX transaction failed (%d)!\n",
					hwcard->name, q->status);
		}
		return;
	}
	//wan_spin_lock_irq(&hwusb->lock,&flags);
	rxcount++;
	if (!wan_test_bit(WP_USB_STATUS_READY, &hwusb->status)){
		DEBUG_RX("%s: WARNING: RX USB core is not ready (%d:%ld)!\n",
					hwcard->name, rxcount,
					(unsigned long)SYSTEM_TICKS);
		hwusb->stats.core_notready_cnt++;
	//	wan_spin_unlock_irq(&hwusb->lock,&flags);
		return;
	}
#if 0
	if (q->actual_length < MAX_USB_RX_LEN){
		DEBUG_EVENT("%s: [RX:%d]: WARNING: Invalid Rx length (%d:%d:%d)\n",
					hwcard->name, rxcount,
					q->actual_length,q->transfer_buffer_length, MAX_USB_RX_LEN);
	}
#endif

	DEBUG_RX("%s:%d: [RX:%d]: RX:%d:%02X %d:%02X %d:%02X (%ld)\n", 
			hwcard->name, wurb->id, rxcount, 
			q->actual_length, ((unsigned char*)q->transfer_buffer)[0], 
			hwusb->next_read_ind, (unsigned char)hwusb->readbuf[hwusb->next_read_ind],
			hwusb->next_rx_ind, (unsigned char)hwusb->readbuf[hwusb->next_rx_ind],
			(unsigned long)SYSTEM_TICKS);

	if (!q->actual_length) goto read_complete_done;

	next_rx_ind = hwusb->next_rx_ind + q->actual_length;
	next_rx_off = hwusb->next_rx_ind + q->actual_length * hwusb->urbcount_read;

	if (hwusb->next_rx_ind < hwusb->next_read_ind){

		if (next_rx_off > hwusb->next_read_ind){
			DEBUG_USB("%s: ERROR:%ld:%d: RX BH is too slow %d:%d:%d (Reset:1)\n",
					hwcard->name,		
					(unsigned long)SYSTEM_TICKS, rxcount,
					hwusb->next_read_ind,hwusb->next_rx_ind,
					q->actual_length*hwusb->urbcount_read);
			hwusb->stats.rx_overrun_cnt++;
			WAN_TASKLET_SCHEDULE((&hwusb->bh_task));
			goto read_complete_done;
		}
		next_rx_ind = next_rx_ind % MAX_READ_BUF_LEN;
		next_rx_off = next_rx_off % MAX_READ_BUF_LEN;

	}else if (hwusb->next_rx_ind > hwusb->next_read_ind){

		if (next_rx_ind > MAX_READ_BUF_LEN){

			memcpy(	&hwusb->readbuf[0], 
				&hwusb->readbuf[MAX_READ_BUF_LEN],
				next_rx_ind - MAX_READ_BUF_LEN);
			next_rx_off = next_rx_off % MAX_READ_BUF_LEN;
			if (next_rx_off > hwusb->next_read_ind){
				DEBUG_USB("%s: ERROR:%ld:%d: RX BH is too slow %d:%d:%d (Reset:2)\n",
						hwcard->name,
						(unsigned long)SYSTEM_TICKS, rxcount,
						hwusb->next_read_ind,hwusb->next_rx_ind,
						q->actual_length*hwusb->urbcount_read);
				hwusb->stats.rx_overrun_cnt++;
				WAN_TASKLET_SCHEDULE((&hwusb->bh_task));
				goto read_complete_done;
			}
			next_rx_ind = next_rx_ind % MAX_READ_BUF_LEN;
		}else{
			next_rx_ind = next_rx_ind % MAX_READ_BUF_LEN;
			next_rx_off = next_rx_off % MAX_READ_BUF_LEN;
		}
	}else{
		next_rx_ind = next_rx_ind % MAX_READ_BUF_LEN;
		next_rx_off = next_rx_off % MAX_READ_BUF_LEN;
	}

	hwusb->next_rx_ind = next_rx_ind;
	wurb->next_off = next_rx_off;	

	/* analyze data */
	if (hwusb->next_rx_ind >= hwusb->next_read_ind){
		len = hwusb->next_rx_ind - hwusb->next_read_ind; 
	}else{
		len = MAX_READ_BUF_LEN - hwusb->next_read_ind + hwusb->next_rx_ind;
	}

	if (hwusb->opmode == SDLA_USB_OPMODE_VOICE){
		DEBUG_RX("[RX-] %s: RX:%d %d:%d\n", 
				hwcard->name, len, hwusb->next_read_ind, hwusb->next_rx_ind);
		if (len > 4){
			sdlausb_syncverify(hwcard, len);
		}
	
		if (hwusb->rx_sync && len >= MAX_USB_RX_LEN){
			WAN_TASKLET_SCHEDULE((&hwusb->bh_task));
		}	
	}

read_complete_done:
	DEBUG_TEST("%s: q->dev=%p buf:%p\n", 
				SDLA_USB_NAME, q->dev, q->transfer_buffer);
	//q->dev = sc->dev;
	
	q->transfer_buffer = &hwusb->readbuf[wurb->next_off];
	if ((err = wp_usb_start_transfer(wurb))){
		if (err != -ENOENT){
			DEBUG_EVENT("%s: Failed to program receiver (%ld)!\n", 
					hwcard->name,
					(unsigned long)SYSTEM_TICKS);
		}
	}

	DEBUG_RX("%s: [RX:%d:%d]: %s: %d:%d:%d!\n",
			hw->devname,wurb->id,rxcount,
			hwusb->next_read_ind, hwusb->next_rx_ind, next_rx_off);
	//wan_spin_unlock_irq(&hwusb->lock,&flags);
	return;
}


#if defined(LINUX26) && (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,19))
void sdlausb_write_complete(struct urb *q, struct pt_regs *regs)
#else
void sdlausb_write_complete(struct urb *q)
#endif
{
	struct wan_urb	*wurb = (struct wan_urb*)q->context;
	sdlahw_card_t	*hwcard = (sdlahw_card_t*)wurb->pvt;
	sdlahw_usb_t	*hwusb = NULL;
	int		actual_length = MAX_USB_TX_LEN;
//	wan_smp_flag_t	flags;

	WAN_ASSERT_VOID(hwcard == NULL);
	hwusb = &hwcard->u_usb;
	txcount++;
	if (q->status){
		if (!(q->status == -ENOENT ||q->status == -ECONNRESET || q->status == -ESHUTDOWN)){
			DEBUG_EVENT("%s: ERROR: RX transaction failed (%d)!\n",
					hwcard->name, q->status);
		}
		return;
	}
	//wan_spin_lock_irq(&hwusb->lock,&flags);
	if (!wan_test_bit(WP_USB_STATUS_READY, &hwusb->status)){
		DEBUG_USB("%s: WARNING: TX USB core is not ready (%d:%ld)!\n",
					hwcard->name, txcount,
					(unsigned long)SYSTEM_TICKS);
		hwusb->stats.core_notready_cnt++;
	//	wan_spin_unlock_irq(&hwusb->lock,&flags);
		return;
	}

	if (hwusb->opmode == SDLA_USB_OPMODE_VOICE){
		if (q->actual_length < MAX_USB_TX_LEN){
			DEBUG_TEST("[TX]: WARNING: Invalid Tx length (%d:%d)\n",
						q->actual_length,MAX_USB_TX_LEN);
			actual_length = q->actual_length; 
		}
	}

	/* Mark this urb as ready */
	wan_set_bit(SDLA_URB_STATUS_READY, &wurb->ready);
	wan_set_bit(WP_USB_STATUS_TX_READY, &hwusb->status);	//sc->tx_ready = 1;
	DEBUG_TX("%s:%d: [TX:%d:%d] %d:%d (%d:%d)\n", 
			hwcard->name, wurb->id, txcount, txcount,
			MAX_USB_TX_LEN,q->transfer_buffer_length,
			hwusb->next_tx_ind, hwusb->next_write_ind);
	//wan_spin_unlock_irq(&hwusb->lock,&flags);
	return;
}


#define usb_endpoint_dir_in(epd)					\
		(((epd)->bEndpointAddress & USB_ENDPOINT_DIR_MASK) == USB_DIR_IN)
#define usb_endpoint_dir_out(epd)					\
		(((epd)->bEndpointAddress & USB_ENDPOINT_DIR_MASK) == USB_DIR_OUT)
#define usb_endpoint_xfer_bulk(epd)					\
		(((epd)->bmAttributes & USB_ENDPOINT_XFERTYPE_MASK) ==	\
		USB_ENDPOINT_XFER_BULK)
#define usb_endpoint_is_bulk_in(epd)					\
		(usb_endpoint_xfer_bulk((epd)) && usb_endpoint_dir_in(epd))
#define usb_endpoint_is_bulk_out(epd)					\
		(usb_endpoint_xfer_bulk((epd)) && usb_endpoint_dir_out(epd))

static int
sdla_prepare_transfer_urbs(sdlahw_card_t *hwcard, struct usb_interface *intf)
{
	struct usb_host_interface	*iface_desc;
	struct usb_endpoint_descriptor	*endpoint;
	sdlahw_usb_t			*hwusb = NULL;
	int				x, i = 0;

	WAN_ASSERT(hwcard == NULL);
	hwusb = &hwcard->u_usb;
	DEBUG_USB("%s: Preparing Transfer URBs...\n", hwcard->name); 
	/* descriptor matches, let's find the endpoints needed */
	/* check out the endpoints */
	iface_desc = intf->cur_altsetting;
	
#if 1
	for (i = 0; i < iface_desc->desc.bNumEndpoints; ++i) {
		endpoint = &iface_desc->endpoint[i].desc;

		if (usb_endpoint_is_bulk_in(endpoint)) {
			/* we found a bulk in endpoint */
			DEBUG_USB("%s: Found bulk in on endpoint %d\n",
					hwcard->name, i);

			for (x = 0; x < hwusb->urbcount_read; x++) {
				usb_init_urb(&hwusb->dataread[x].urb);
				usb_fill_bulk_urb (
						&hwusb->dataread[x].urb,
						hwusb->usb_dev,
						usb_rcvbulkpipe (hwusb->usb_dev,
								endpoint->bEndpointAddress),
						&hwusb->readbuf[MAX_USB_RX_LEN*x], 
						endpoint->wMaxPacketSize,
						sdlausb_read_complete,
						&hwusb->dataread[x]);	//p
				hwusb->dataread[x].next_off = MAX_USB_RX_LEN*x;
				DEBUG_USB("%s: %d: Bulk In ENAddress:%X Len:%d (%d:%p)\n",
						hwcard->name, x,
						endpoint->bEndpointAddress,
						endpoint->wMaxPacketSize,
						hwusb->dataread[x].next_off,
						&hwusb->readbuf[MAX_USB_RX_LEN*x]);
			}
		}

		if (usb_endpoint_is_bulk_out(endpoint)) {
			/* we found a bulk out endpoint */
			DEBUG_USB("%s: Found bulk out on endpoint %d\n",
					 hwcard->name, i);
			for (x = 0; x < hwusb->urbcount_write; x++) {
				usb_init_urb(&hwusb->datawrite[x].urb);
				usb_fill_bulk_urb (
						&hwusb->datawrite[x].urb,
						hwusb->usb_dev,
						usb_sndbulkpipe (hwusb->usb_dev,
								endpoint->bEndpointAddress),
						&hwusb->writebuf[MAX_USB_TX_LEN*x], 
						MAX_USB_TX_LEN,
						sdlausb_write_complete,
						&hwusb->datawrite[x]);	//p
				hwusb->datawrite[x].next_off = MAX_USB_TX_LEN*x;
				DEBUG_USB("%s: %d: Bulk Out ENAddress:%X Len:%d (%d:%p)\n",
						hwcard->name, x,
						endpoint->bEndpointAddress,
						MAX_USB_TX_LEN,
						hwusb->datawrite[x].next_off,
						&hwusb->writebuf[MAX_USB_TX_LEN*x]);
			}
		}
	}

#else
	for (i = 0; i < iface_desc->desc.bNumEndpoints; ++i) {
		endpoint = &iface_desc->endpoint[i].desc;

		if (usb_endpoint_is_bulk_in(endpoint)) {
			/* we found a bulk in endpoint */
			DEBUG_USB("%s: Found bulk in on endpoint %d\n",
					hwcard->name, i);
			bulk_in_endpoint[num_bulk_in] = endpoint;
			++num_bulk_in;
		}

		if (usb_endpoint_is_bulk_out(endpoint)) {
			/* we found a bulk out endpoint */
			DEBUG_USB("%s: Found bulk out on endpoint %d\n",
					 hwcard->name, i);
			bulk_out_endpoint[num_bulk_out] = endpoint;
			++num_bulk_out;
		}
	}

	for (x = 0; x < hwusb->urbcount_read; x++) {
		/* set up the endpoint information */
		for (i = 0; i < num_bulk_in; ++i) {
			endpoint = bulk_in_endpoint[i];
			usb_init_urb(&hwusb->dataread[x].urb);
			usb_fill_bulk_urb (
					&hwusb->dataread[x].urb,
					hwusb->usb_dev,
					usb_rcvbulkpipe (hwusb->usb_dev,
							   endpoint->bEndpointAddress),
					&hwusb->readbuf[MAX_USB_RX_LEN*x], 
					endpoint->wMaxPacketSize,
					sdlausb_read_complete,
					&hwusb->dataread[x]);	//p
			hwusb->dataread[x].next_off = MAX_USB_RX_LEN*x;
			DEBUG_USB("%s: %d: Bulk In ENAddress:%X Len:%d (%d:%p)\n",
					hwcard->name, x,
					endpoint->bEndpointAddress,
					endpoint->wMaxPacketSize,
					hwusb->dataread[x].next_off,
					&hwusb->readbuf[MAX_USB_RX_LEN*x]);
		}
	}
	
	for (x = 0; x < hwusb->urbcount_write; x++) {
		for (i = 0; i < num_bulk_out; ++i) {
			endpoint = bulk_out_endpoint[i];
			usb_init_urb(&hwusb->datawrite[x].urb);
			usb_fill_bulk_urb (
					&hwusb->datawrite[x].urb,
					hwusb->usb_dev,
					usb_sndbulkpipe (hwusb->usb_dev,
							endpoint->bEndpointAddress),
					&hwusb->writebuf[MAX_USB_TX_LEN*x], 
					MAX_USB_TX_LEN,
					sdlausb_write_complete,
					&hwusb->datawrite[x]);	//p
			hwusb->datawrite[x].next_off = MAX_USB_TX_LEN*x;
			DEBUG_USB("%s: %d: Bulk Out ENAddress:%X Len:%d (%d:%p)\n",
					hwcard->name, x,
					endpoint->bEndpointAddress,
					MAX_USB_TX_LEN,
					hwusb->datawrite[x].next_off,
					&hwusb->writebuf[MAX_USB_TX_LEN*x]);
		}
	}	
#endif
	return 0;
}

static int wp_usb_start_transfer(struct wan_urb *wurb)
{
	int	ret;

#ifdef LINUX26
	ret = usb_submit_urb(&wurb->urb, GFP_ATOMIC); 
#else
	ret = usb_submit_urb(q); 
#endif
	if (ret){
		DEBUG_USB("%s: usb_submit %p returns %d (%s)\n",
			__FUNCTION__, wurb, ret,
			wan_test_bit(SDLA_URB_STATUS_READY, &wurb->ready)?"Ready":"Busy");
		return ret;
	}
	return 0;
}

#define REQTYPE_HOST_TO_DEVICE	0x41
#define REQTYPE_DEVICE_TO_HOST	0xc1

/* Config SET requests. To GET, add 1 to the request number */
#define CP2101_UART 		0x00	/* Enable / Disable */
#define CP2101_BAUDRATE		0x01	/* (BAUD_RATE_GEN_FREQ / baudrate) */
#define CP2101_BITS		0x03	/* 0x(0)(databits)(parity)(stopbits) */
#define CP2101_BREAK		0x05	/* On / Off */
#define CP2101_CONTROL		0x07	/* Flow control line states */
#define CP2101_MODEMCTL		0x13	/* Modem controls */
#define CP2101_CONFIG_6		0x19	/* 6 bytes of config data ??? */

/* CP2101_UART */
#define UART_ENABLE		0x0001
#define UART_DISABLE		0x0000

/* CP2101_BAUDRATE */
#define BAUD_RATE_GEN_FREQ	0x384000

/* CP2101_BITS */
#define BITS_DATA_MASK		0X0f00
#define BITS_DATA_5		0X0500
#define BITS_DATA_6		0X0600
#define BITS_DATA_7		0X0700
#define BITS_DATA_8		0X0800
#define BITS_DATA_9		0X0900

#define BITS_PARITY_MASK	0x00f0
#define BITS_PARITY_NONE	0x0000
#define BITS_PARITY_ODD		0x0010
#define BITS_PARITY_EVEN	0x0020
#define BITS_PARITY_MARK	0x0030
#define BITS_PARITY_SPACE	0x0040

#define BITS_STOP_MASK		0x000f
#define BITS_STOP_1		0x0000
#define BITS_STOP_1_5		0x0001
#define BITS_STOP_2		0x0002


static int 
sdla_usb_set_config(sdlahw_card_t *hwcard, u8 request, unsigned int *data, int size)
{
	__le32		*buf;
	int		result, i, length;

	WAN_ASSERT(hwcard == NULL);
	WAN_ASSERT(hwcard->u_usb.usb_dev == NULL);

	/* Number of integers required to contain the array */
	length = (((size - 1) | 3) + 1)/4;

	buf = kmalloc(length * sizeof(__le32), GFP_KERNEL);
	if (!buf) {
		DEBUG_ERROR("ERROR [%s:%d]: Out of memory!\n",
				__FUNCTION__,__LINE__);
		return -ENOMEM;
	}

	/* Array of integers into bytes */
	for (i = 0; i < length; i++)
		buf[i] = cpu_to_le32(data[i]);

	if (size > 2) {
		result = usb_control_msg (hwcard->u_usb.usb_dev,
				usb_sndctrlpipe(hwcard->u_usb.usb_dev, 0),
				request, REQTYPE_HOST_TO_DEVICE, 0x0000,
				0, buf, size, 300);
	} else {
		result = usb_control_msg (hwcard->u_usb.usb_dev,
				usb_sndctrlpipe(hwcard->u_usb.usb_dev, 0),
				request, REQTYPE_HOST_TO_DEVICE, data[0],
				0, NULL, 0, 300);
	}

	kfree(buf);

	if ((size > 2 && result != size) || result < 0) {
		DEBUG_ERROR("ERROR [%s:%d]: Unable to send request: request=0x%x size=%d result=%d!\n",
				__FUNCTION__,__LINE__, request, size, result);
		return -EPROTO;
	}

	/* Single data value */
	result = usb_control_msg(	hwcard->u_usb.usb_dev,
					usb_sndctrlpipe(hwcard->u_usb.usb_dev, 0),
					request, REQTYPE_HOST_TO_DEVICE, data[0],
					0, NULL, 0, 300);
	return 0;
}


/*
 * cp2101_get_config
 * Reads from the CP2101 configuration registers
 * 'size' is specified in bytes.
 * 'data' is a pointer to a pre-allocated array of integers large
 * enough to hold 'size' bytes (with 4 bytes to each integer)
 */
static int 
sdla_usb_get_config(sdlahw_card_t *hwcard, u8 request, unsigned int *data, int size)
{
	__le32 *buf;
	int result, i, length;

	WAN_ASSERT(hwcard == NULL);
	WAN_ASSERT(hwcard->u_usb.usb_dev == NULL);

	/* Number of integers required to contain the array */
	length = (((size - 1) | 3) + 1)/4;

	buf = kcalloc(length, sizeof(__le32), GFP_KERNEL);
	if (!buf) {
		DEBUG_ERROR("ERROR [%s:%d]: Out of memory.\n", __FUNCTION__,__LINE__);
		return -ENOMEM;
	}

	/* For get requests, the request number must be incremented */
	request++;

	/* Issue the request, attempting to read 'size' bytes */
	result = usb_control_msg (	hwcard->u_usb.usb_dev,
					usb_rcvctrlpipe (hwcard->u_usb.usb_dev, 0),
					request, REQTYPE_DEVICE_TO_HOST, 0x0000,
					0, buf, size, 300);

	/* Convert data into an array of integers */
	for (i=0; i<length; i++)
		data[i] = le32_to_cpu(buf[i]);

	kfree(buf);

	if (result != size) {
		DEBUG_EVENT(
		"ERROR [%s:%d]: Unable to send config request, request=0x%x size=%d result=%d\n",
				__FUNCTION__, __LINE__, request, size, result);
		return -EPROTO;
	}

	return 0;
}

/*
 * cp2101_set_config_single
 * Convenience function for calling cp2101_set_config on single data values
 * without requiring an integer pointer
 */
static inline int 
sdla_usb_set_config_single(sdlahw_card_t *hwcard, u8 request, unsigned int data)
{
	return sdla_usb_set_config(hwcard, request, &data, 2);
}


static int sdla_usb_search_rxsync(sdlahw_card_t *hwcard)
{
	int	retry = 0;

	wait_just_a_bit(5*HZ, 0);
	do{
		if (retry++ > SDLA_USBFXO_SYNC_RX_RETRIES) return -EINVAL;
		wait_just_a_bit(SDLA_USBFXO_SYNC_DELAY, 0);
	} while(!hwcard->u_usb.rx_sync);
	return 0;
}

static int sdla_usb_search_txsync(sdlahw_card_t *hwcard)
{
	int		retry = 0;
	do{
		if (retry++ > SDLA_USBFXO_SYNC_TX_RETRIES) return -EINVAL;
		wait_just_a_bit(SDLA_USBFXO_SYNC_DELAY, 0);
	} while(!hwcard->u_usb.tx_sync);
	return 0;
}


static int sdla_usb_rxurb_reset(sdlahw_card_t *hwcard)
{
	sdlahw_usb_t	*hwusb = NULL;
	int		x=0;

	WAN_ASSERT(hwcard == NULL);
	hwusb = &hwcard->u_usb;

	for (x = 0; x < hwusb->urbcount_read; x++){
		usb_unlink_urb(&hwusb->dataread[x].urb);
		usb_kill_urb(&hwusb->dataread[x].urb);
	}
	hwusb->next_rx_ind	= 0;
	hwusb->next_read_ind	= 0;
	for (x = 0; x < hwusb->urbcount_read; x++){
		hwusb->dataread[x].urb.transfer_buffer = &hwusb->readbuf[hwusb->next_rx_ind];
		if (wp_usb_start_transfer(&hwusb->dataread[x])) {
			DEBUG_EVENT("%s: Failed to start RX:%d transfer!\n", 
					hwcard->name, x);
			return -EINVAL;
		}
	}
	return 0;
}

static int sdla_usb_txurb_reset(sdlahw_card_t *hwcard)
{
	sdlahw_usb_t	*hwusb = NULL;
	int		x=0;

	WAN_ASSERT(hwcard == NULL);
	hwusb = &hwcard->u_usb;

	for (x = 0; x < hwusb->urbcount_write; x++){
		usb_unlink_urb(&hwusb->datawrite[x].urb);
		usb_kill_urb(&hwusb->datawrite[x].urb);
	}
	hwusb->next_tx_ind	= 0;
	hwusb->next_write_ind	= 0;
	return 0;
}

int sdla_usb_setup(sdlahw_card_t *hwcard, int new_usbdev)
{
	sdlahw_usb_t	*hwusb = NULL;
	netskb_t	*skb;
	int		err, bits, x = 0, mod_no = 0;
	u8		data8;

	WAN_ASSERT(hwcard == NULL);
	hwusb = &hwcard->u_usb;
	DEBUG_USB("%s: Creating private data for usb%s...\n", 
				hwcard->name,hwusb->bus_id);

	if (!new_usbdev){
		DEBUG_EVENT("%s: Loading USB-FXO firmware code...",
					hwcard->name);
		wait_just_a_bit(SDLA_USBFXO_BL_DELAY, 0);
	}
    
	WAN_TASKLET_INIT((&hwusb->bh_task), 0, sdla_usb_bh, hwcard);

	hwusb->writebuf = wan_malloc(MAX_READ_BUF_LEN+MAX_USB_TX_LEN+1);
	if (hwusb->writebuf == NULL){
		DEBUG_EVENT("%s: Failed to allocate TX buffer!\n",
				hwcard->name);
		return -ENOMEM;
	}
	memset(hwusb->writebuf,0x00,MAX_READ_BUF_LEN+MAX_USB_TX_LEN+1);

	hwusb->readbuf = wan_malloc(MAX_READ_BUF_LEN+MAX_USB_RX_LEN+1);
	if (hwusb->readbuf == NULL){
		DEBUG_EVENT("%s: Failed to allocate RX buffer!\n",
				hwcard->name);
		return -ENOMEM;
	}
	memset(hwusb->readbuf,0x00,MAX_READ_BUF_LEN+MAX_USB_RX_LEN+1);
	//memset(&hwusb->guard1[0],0x00,1000);
	//memset(&hwusb->guard2[0],0x00,1000);
	WAN_IFQ_INIT(&hwusb->rx_cmd_free_list, WP_USB_MAX_RX_CMD_QLEN);
	WAN_IFQ_INIT(&hwusb->rx_cmd_list, WP_USB_MAX_RX_CMD_QLEN);
	for(x = 0; x < WP_USB_MAX_RX_CMD_QLEN; x++){
		skb = wan_skb_alloc(10);
		if (!skb){
			DEBUG_ERROR("%s: ERROR: Failed to allocate RX cmd buffer!\n",
						hwcard->name);
			goto cleanup;
		}
		wan_skb_queue_tail(&hwusb->rx_cmd_free_list, skb);
	}
	WAN_IFQ_INIT(&hwusb->tx_cmd_free_list, WP_USB_MAX_TX_CMD_QLEN);
	WAN_IFQ_INIT(&hwusb->tx_cmd_list, WP_USB_MAX_TX_CMD_QLEN);
	for(x = 0; x < WP_USB_MAX_TX_CMD_QLEN; x++){
		skb = wan_skb_alloc(10);
		if (!skb){
			DEBUG_ERROR("%s: ERROR: Failed to allocate TX cmd buffer!\n",
						hwcard->name);
			goto cleanup;
		}
		wan_skb_queue_tail(&hwusb->tx_cmd_free_list, skb);
	}

	hwusb->rxtx_len		= WP_USB_RXTX_DATA_LEN;
	hwusb->rxtx_count	= WP_USB_RXTX_DATA_COUNT;
	hwusb->rxtx_total_len	= hwusb->rxtx_len * hwusb->rxtx_count;
 	
	hwusb->rxtx_buf_len	= hwusb->rxtx_total_len * 4;
	hwusb->read_buf_len	= hwusb->rxtx_buf_len * (WP_USB_MAX_RW_COUNT + 2);
	hwusb->write_buf_len	= hwusb->rxtx_buf_len * (WP_USB_MAX_RW_COUNT + 2);
 
	wan_spin_lock_init(&hwusb->cmd_lock,"usb_cmd");
	wan_spin_lock_irq_init(&hwusb->lock,"usb_bh_lock");
	hwusb->ctrl_idle_pattern  = WP_USB_CTRL_IDLE_PATTERN;
	if (sdla_usb_set_config_single(hwcard, CP2101_UART, UART_ENABLE)) {
		DEBUG_ERROR("%s: ERROR: Unable to enable UART!\n",
					hwcard->name);
		goto cleanup;
	}

	if (sdla_usb_set_config_single(hwcard, CP2101_BAUDRATE, (BAUD_RATE_GEN_FREQ / WP_USB_BAUD_RATE))){
		DEBUG_EVENT(
		"%s: ERROR: Baud rate requested not supported by device (%d bps)!\n",
				hwcard->name, WP_USB_BAUD_RATE);
		goto cleanup;
	}

	sdla_usb_get_config(hwcard, CP2101_BITS, &bits, 2);
	bits &= ~BITS_DATA_MASK;
	bits |= BITS_DATA_8;
	if (sdla_usb_set_config(hwcard, CP2101_BITS, &bits, 2)){
		DEBUG_EVENT(
		"%s: ERROR: Number of data bits requested not supported by device (%02X)!\n",
				hwcard->name, bits);
		goto cleanup;
	}

	hwusb->urbcount_read = MAX_READ_URB_COUNT;	//4
	hwusb->urb_read_ind = 0;
	for (x = 0; x < hwusb->urbcount_read; x++){
		hwusb->dataread[x].id = x;
		hwusb->dataread[x].pvt = hwcard;
		wan_set_bit(SDLA_URB_STATUS_READY, &hwusb->datawrite[x].ready);
	}

	hwusb->urbcount_write = MAX_WRITE_URB_COUNT;	//4
	hwusb->urb_write_ind = 0;
	for (x = 0; x < hwusb->urbcount_write; x++){
		hwusb->datawrite[x].id = x;
		hwusb->datawrite[x].pvt = hwcard;
		wan_set_bit(SDLA_URB_STATUS_READY, &hwusb->datawrite[x].ready);
	}
	if (sdla_prepare_transfer_urbs(hwcard, hwusb->usb_intf)) {
		DEBUG_EVENT(
		"%s: Failed to prepare the urbs for transfer!\n",
				hwcard->name);
		goto cleanup;
	}

	hwusb->next_rx_ind	= 0;
	hwusb->next_read_ind	= 0;
	hwusb->next_tx_ind	= 0;
	hwusb->next_write_ind	= 0;

	/* Create dummy voice */
	for(mod_no = 0; mod_no < 2; mod_no++){
		memset(&hwusb->regs[mod_no][0], 0xFF, 110); 
	}

	/* init initial and idle buffer */
	for (x=0; x < MAX_USB_TX_LEN;x++){
		hwusb->writebuf[x] = idlebuf[x];
		hwusb->idlebuf[x] = idlebuf[x];
	}
	
	usb_set_intfdata(hwusb->usb_intf, hwcard);
	hwcard->internal_used++;
	hwcard->core_rev = 0xFF;
	wan_set_bit(WP_USB_STATUS_READY, &hwusb->status);

	/* Verifing sync with USB-FXO device */
	hwusb->opmode = SDLA_USB_OPMODE_VOICE;
	for (x = 0; x < hwusb->urbcount_read; x++){
		if (wp_usb_start_transfer(&hwusb->dataread[x])) {
			DEBUG_EVENT(
			"%s: Failed to start RX:%d transfer!\n", 
					hwcard->name, x);
			goto cleanup;
		}
	}

	err = sdla_usb_search_rxsync(hwcard);
	if (!err && hwusb->rx_sync){

		hwusb->opmode = SDLA_USB_OPMODE_VOICE;

        	/* Waiting for USB device to sync */
		sdla_usb_search_txsync(hwcard);
				
		/* Read hardware ID */
		err = __sdla_usb_cpu_read(hwcard, SDLA_USB_CPU_REG_DEVICEID, &data8);
		if (data8 != SDLA_USB_DEVICEID){
			DEBUG_EVENT("%s: Failed to read USB Device ID (%02X:err=%d)!\n",
					hwcard->name, data8, err);
			goto cleanup;
		}
		DEBUG_USB("%s: Detected USB-FXO interface!\n",
					hwcard->name);

		/* Read hardware version */ 
		err = __sdla_usb_cpu_read(hwcard, SDLA_USB_CPU_REG_HARDWAREVER, &data8);
		if (err){
			DEBUG_EVENT("%s: Failed to read USB Hardware Version (err=%d)!\n",
					hwcard->name, err);
			goto cleanup;
		}
		hwusb->hw_rev = data8;
	
		/* Read firmware version */
		err = __sdla_usb_cpu_read(hwcard, SDLA_USB_CPU_REG_FIRMWAREVER, &data8);
		if (err){
			DEBUG_EVENT("%s: Failed to read USB Firmware Version (err=%d)!\n",
					hwcard->name, err);
			goto cleanup;
		}
		hwcard->core_rev = data8;

		/* Read EC channels number */
		__sdla_usb_cpu_read(hwcard, SDLA_USB_CPU_REG_EC_NUM, &data8);
		hwcard->hwec_chan_no = data8;
		if (hwcard->core_rev < 0x20){
			/* Undefined register before ver 2.0 */
			hwcard->hwec_chan_no = 0x00;
			/* TX Sync is not implemented in early version */
			hwusb->tx_sync = 1;
		}else{
			if (!hwusb->tx_sync){
    				DEBUG_EVENT("%s: USB device is out of sync!\n",
                                			hwcard->name);
                		goto cleanup;
            		}
        	}

		/* Reset register to default values */
		__sdla_usb_cpu_write(hwcard, SDLA_USB_CPU_REG_CTRL, SDLA_USB_CPU_BIT_CTRL_RESET);
		hwusb->reg_cpu_ctrl = 0x00;

		__sdla_usb_cpu_read(hwcard, SDLA_USB_CPU_REG_FIFO_STATUS, &data8);
		__sdla_usb_cpu_read(hwcard, SDLA_USB_CPU_REG_UART_STATUS, &data8);
		__sdla_usb_cpu_read(hwcard, SDLA_USB_CPU_REG_HOSTIF_STATUS, &data8);
	}else{
		DEBUG_EVENT("%s: USB-FXO Device in Firmware Update mode...\n",
				  hwcard->name);
		hwusb->opmode = SDLA_USB_OPMODE_API;
		hwusb->hw_rev	= 0xFF;
		hwcard->core_rev= 0xFF;
	}

	return 0;

cleanup:

	wan_clear_bit(WP_USB_STATUS_READY, &hwusb->status);
	WAN_IFQ_PURGE(&hwusb->tx_cmd_free_list);
	WAN_IFQ_DESTROY(&hwusb->tx_cmd_free_list);
	WAN_IFQ_PURGE(&hwusb->rx_cmd_free_list);
	WAN_IFQ_DESTROY(&hwusb->rx_cmd_free_list);
	WAN_TASKLET_KILL(&hwusb->bh_task);
	if (usb_get_intfdata(hwusb->usb_intf)){
		hwcard->internal_used--;
		usb_set_intfdata(hwusb->usb_intf, NULL);
	}
	return -ENODEV;
}

int sdla_usb_down(sdlahw_card_t *hwcard, int force)
{
	sdlahw_usb_t	*hwusb = NULL;
	int		x = 0;

	WAN_ASSERT(hwcard == NULL);
	hwusb = &hwcard->u_usb;
	DEBUG_EVENT("%s: Releasing private data...\n", hwcard->name);

	wan_clear_bit(WP_USB_STATUS_READY, &hwusb->status);
	WAN_TASKLET_KILL(&hwusb->bh_task);

	/* Do not clear the structures if force=1 */
	WAN_IFQ_PURGE(&hwusb->tx_cmd_free_list);
	WAN_IFQ_DESTROY(&hwusb->tx_cmd_free_list);
	WAN_IFQ_PURGE(&hwusb->rx_cmd_free_list);
	WAN_IFQ_DESTROY(&hwusb->rx_cmd_free_list);
	WAN_IFQ_PURGE(&hwusb->tx_cmd_list);
	WAN_IFQ_DESTROY(&hwusb->tx_cmd_list);
	WAN_IFQ_PURGE(&hwusb->rx_cmd_list);
	WAN_IFQ_DESTROY(&hwusb->rx_cmd_list);

	for (x = 0; x < hwusb->urbcount_read; x++){
		usb_unlink_urb(&hwusb->dataread[x].urb);
		usb_kill_urb(&hwusb->dataread[x].urb);
	}
	for (x = 0; x < hwusb->urbcount_write; x++){
		usb_unlink_urb(&hwusb->datawrite[x].urb);
		usb_kill_urb(&hwusb->datawrite[x].urb);
	}
	if (hwusb->writebuf) wan_free(hwusb->writebuf);
	if (hwusb->readbuf) wan_free(hwusb->readbuf);
	if (usb_get_intfdata(hwusb->usb_intf)){
		hwcard->internal_used--;
		usb_set_intfdata(hwusb->usb_intf, NULL);
	}
	return 0;
}

#endif   /* #if defined(CONFIG_PRODUCT_WANPIPE_USB)        */
