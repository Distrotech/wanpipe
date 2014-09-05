

#include "wanpipe_includes.h"
#include "wanpipe_api.h"
#include "wan_aften.h"

#if 0
# define DEBUG_REG
#endif

#define WAN_AFTEN_FE_RW 0

#if WAN_AFTEN_FE_RW
#ifndef DEFINE_MUTEX
# define DEFINE_MUTEX
#endif

# include "wanpipe_common.h"
# define DEBUG_FE_EXPORT	if(0)DEBUG_EVENT

static wan_mutexlock_t rw_mutex;
int wan_aftup_te1_reg_write(void *card, unsigned short reg, unsigned short in_data);
int wan_aftup_te1_reg_read(void *card, unsigned short reg, unsigned char *out_data);

/* Maxim chip definitions */
#define REG_IDR				0xF8
#define DEVICE_ID_DS26519   0xD9
#define DEVICE_ID_DS26528   0x0B
#define DEVICE_ID_DS26524   0x0C
#define DEVICE_ID_DS26522   0x0D
#define DEVICE_ID_DS26521   0x0E
#define DEVICE_ID_BAD       0x00

#define DEVICE_ID_SHIFT     3
#define DEVICE_ID_MASK      0x1F
#define DEVICE_ID_DS(dev_id)    ((dev_id) >> DEVICE_ID_SHIFT) & DEVICE_ID_MASK
#define DECODE_CHIPID(chip_id)                  \
        (chip_id == DEVICE_ID_DS26519) ? "DS26519" :    \
        (chip_id == DEVICE_ID_DS26528) ? "DS26528" :    \
        (chip_id == DEVICE_ID_DS26524) ? "DS26524" :    \
        (chip_id == DEVICE_ID_DS26522) ? "DS26522" :    \
        (chip_id == DEVICE_ID_DS26521) ? "DS26521" : "Unknown"

#endif

static char	*wan_drvname = "wan_aften";
static char wan_fullname[]	= "WANPIPE(tm) Sangoma AFT (lite) Driver";
static char wan_copyright[]	= "(c) 1995-2008 Sangoma Technologies Inc.";
static int	ncards; 

static sdla_t* card_list = NULL;	/* adapter data space */
extern wan_iface_t wan_iface;

#if defined(CONFIG_PRODUCT_WANPIPE_USB)
static int wan_aften_callback_add_device(void);
static int wan_aften_callback_delete_device(char *);
extern sdladrv_callback_t sdladrv_callback;

extern int sdla_get_hw_usb_adptr_cnt (void);

#endif

static int wan_aften_init(void*);
static int wan_aften_exit(void*);
#if 0
static int wan_aften_shutdown(void*);
static int wan_aften_ready_unload(void*);
#endif
static int wan_aften_add_device(int index);
static int wan_aften_delete_device(sdla_t *card);

static int wan_aften_setup(sdla_t *card, netdevice_t *dev);
static int wan_aften_release(sdla_t *card);
static int wan_aften_update_ports(void);
static int wan_aften_open(netdevice_t*);
static int wan_aften_close(netdevice_t*);
static int wan_aften_ioctl (netdevice_t*, struct ifreq*, wan_ioctl_cmd_t);

#if defined(__OpenBSD__)
struct cdevsw wan_aften_devsw = {
	NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL};
WAN_MODULE_DEFINE(
	wan_aften,"wan_aften", 
	"Alex Feldman <al.feldman@sangoma.com>",
	"WAN AFT Enable", 
	"GPL",
	wan_aften_init, wan_aften_exit,/* wan_aften_shutdown, wan_aften_ready_unload,*/
	&wan_aften_devsw);
#else
WAN_MODULE_DEFINE(
	wan_aften,"wan_aften", 
	"Alex Feldman <al.feldman@sangoma.com>",
	"WAN AFT Enable", 
	"GPL",
	wan_aften_init, wan_aften_exit,/*wan_aften_shutdown, wan_aften_ready_unload,*/
	NULL);
#endif
WAN_MODULE_DEPEND(wan_aften, sdladrv, 1, SDLADRV_MAJOR_VER, SDLADRV_MAJOR_VER);
WAN_MODULE_VERSION(wan_aften, WAN_AFTEN_VER);

static int wan_aften_init(void *arg)
{
	int 	err = 0, i, extra_ncards = 0;

#if defined(CONFIG_PRODUCT_WANPIPE_USB)
	sdladrv_callback.add_device	= wan_aften_callback_add_device;
	sdladrv_callback.delete_device	= wan_aften_callback_delete_device;
#endif

#if defined(__OpenBSD__) || defined(__NetBSD__)
	sdladrv_init();
#endif

	sdladrv_hw_mode(SDLADRV_MODE_LIMITED);

        DEBUG_EVENT("%s v%d %s\n", 
				wan_fullname, 
				WAN_AFTEN_VER, 
				wan_copyright); 
	
	ncards = sdla_hw_probe();
#if defined(CONFIG_PRODUCT_WANPIPE_USB)
	ncards += sdla_get_hw_usb_adptr_cnt();
#endif
	if (!(ncards+extra_ncards)){
		DEBUG_EVENT("No Sangoma cards found, unloading modules!\n");
		return -ENODEV;
	}

	for (i=0;i<ncards;i++){
		if (wan_aften_add_device(i)){
			goto wan_aften_init_done;
		}
	}
	wan_aften_update_ports();

wan_aften_init_done:
	if (err) wan_aften_exit(arg);
	return err;
}

static int wan_aften_exit(void *arg)
{
	sdla_t			*card;
	int			err = 0;

	for (card=card_list;card_list;){
		DEBUG_EVENT("%s: Unregistering device\n",
				 card->devname);
		wan_aften_delete_device(card);
		card_list = card->list;
		wan_free(card);
		card = card_list;
	}

	card_list=NULL;
#if defined(__OpenBSD__) || defined(__NetBSD__)
	sdladrv_exit();
#endif
	DEBUG_EVENT("\n");
	DEBUG_EVENT("%s Unloaded.\n", wan_fullname);

	return err;
}

#if 0
int wan_aften_shutdown(void *arg)
{
	DEBUG_EVENT("Shutting down WAN_AFTEN module ...\n");
	return 0;
}

int wan_aften_ready_unload(void *arg)
{
	DEBUG_EVENT("Is WAN_AFTEN module ready to unload...\n");
	return 0;
}
#endif

static int wan_aften_add_device(int index)
{
	struct wan_aften_priv	*priv = NULL;
	struct wan_dev_le	*devle;
	sdla_t			*card, *tmp;
	wan_device_t		*wandev;
	netdevice_t		*dev = NULL;
	static int		if_index = 0;

	card=wan_malloc(sizeof(sdla_t));
	if (!card){
		DEBUG_EVENT("%s: Failed allocate new card!\n", 
				wan_drvname);
		return -EINVAL;
	}
	memset(card, 0, sizeof(sdla_t));
	/* Allocate HDLC device */
	if (wan_iface.alloc)
		dev = wan_iface.alloc(1,WAN_IFT_OTHER);
	if (dev == NULL){
		wan_free(card);
		return -EINVAL;
	}

	devle = wan_malloc(sizeof(struct wan_dev_le));
	if (devle == NULL){
		DEBUG_EVENT("%s: Failed allocate memory!\n",
				wan_drvname);
		wan_free(card);
		return -EINVAL;
	}
	if ((priv = wan_malloc(sizeof(struct wan_aften_priv))) == NULL){
		DEBUG_EVENT("%s: Failed to allocate priv structure!\n",
			wan_drvname);
		wan_free(card);
		wan_free(devle);
		return -EINVAL;
	}
	priv->common.is_netdev		= 1;
	priv->common.iface.open		= &wan_aften_open;
	priv->common.iface.close	= &wan_aften_close;
	priv->common.iface.ioctl	= &wan_aften_ioctl;

	priv->common.card		= card;
	wan_netif_set_priv(dev, priv);

#if defined(__LINUX__)
	/*sprintf(card->devname, "hdlc%d", if_index++);*/
	sprintf(card->devname, "w%dg1", ++if_index);
#else
	sprintf(card->devname, "w%cg1", 
		'a' + if_index++);

#endif

	/* Register in HDLC device */
	if (!wan_iface.attach || wan_iface.attach(dev, card->devname, 1)){
		wan_free(devle);
		if (wan_iface.free) wan_iface.free(dev);
		return -EINVAL;
	}
	wandev 		= &card->wandev;
	wandev->magic  	= ROUTER_MAGIC;
	wandev->name   	= card->devname;
	wandev->priv 	= card;
	devle->dev	= dev;
	/* Set device pointer */
	WAN_LIST_INIT(&wandev->dev_head);
	WAN_LIST_INSERT_HEAD(&wandev->dev_head, devle, dev_link);
	card->list = NULL;
	if (card_list){
		for(tmp = card_list;tmp->list;tmp = tmp->list);
		tmp->list = card;
	}else{
		card_list = card;
	}
#if 0
	card->list	= card_list;
	card_list	= card;
#endif
	if (wan_aften_setup(card, dev)){
		DEBUG_EVENT("%s: Failed setup new device!\n", 
				card->devname);
		WAN_LIST_REMOVE(devle, dev_link);
		wan_free(devle);
		if (wan_iface.detach) wan_iface.detach(dev, 1);
		if (wan_iface.free) wan_iface.free(dev);
		if (card_list == card){
			card_list = NULL;
		}else{
			for(tmp = card_list;
				tmp->list != card; tmp = tmp->list);
			tmp->list = card->list;
			card->list = NULL;
		}
		wan_free(card);
		return -EINVAL;
	}

	return 0;
}

static int wan_aften_delete_device(sdla_t *card)
{
	struct wan_dev_le	*devle;

	devle = WAN_LIST_FIRST(&card->wandev.dev_head);
	if (devle && devle->dev){
		struct wan_aften_priv	*priv = NULL;
		priv = wan_netif_priv(devle->dev);
		DEBUG_EVENT("%s: Unregistering interface...\n", 
				wan_netif_name(devle->dev));
		if (wan_iface.detach)
			wan_iface.detach(devle->dev, 1);
		wan_free(priv);
		if (wan_iface.free)
			wan_iface.free(devle->dev);
		WAN_LIST_REMOVE(devle, dev_link);
		wan_free(devle);
	}
	DEBUG_EVENT("%s: Release device (%p)\n",
			card->devname, card);
	wan_aften_release(card);
	return 0;
}

static int wan_aften_setup(sdla_t *card, netdevice_t *dev)
{
	struct wan_aften_priv	*priv = wan_netif_priv(dev);
	int			err;

	card->hw = sdla_register(&card->hw_iface, NULL, card->devname);
	if (card->hw == NULL){
		DEBUG_EVENT("%s: Failed to register hw device\n",
				card->devname);
		goto wan_aften_setup_error;
	}

	err = card->hw_iface.setup(card->hw, NULL);
	if (err){
		DEBUG_EVENT("%s: Hardware setup Failed %d\n",
				card->devname,err);
		sdla_unregister(&card->hw, card->devname);
		goto wan_aften_setup_error;
	}

	WAN_HWCALL(getcfg, (card->hw, SDLA_CARDTYPE, &card->type));
	if (card->type != SDLA_USB){
		WAN_HWCALL(getcfg, (card->hw, SDLA_BUS, &priv->bus));
		WAN_HWCALL(getcfg, (card->hw, SDLA_SLOT, &priv->slot));
		WAN_HWCALL(getcfg, (card->hw, SDLA_IRQ, &priv->irq));
		WAN_HWCALL(pci_read_config_dword, 
			(card->hw, 0x04, &priv->base_class));
		WAN_HWCALL(pci_read_config_dword, 
			(card->hw, PCI_IO_BASE_DWORD, &priv->base_addr0));
		WAN_HWCALL(pci_read_config_dword, 
			(card->hw, PCI_MEM_BASE0_DWORD, &priv->base_addr1));

		DEBUG_EVENT("%s: BaseClass %X BaseAddr %X:%X IRQ %d\n", 
				wan_netif_name(dev),
				priv->base_class,
				priv->base_addr0,
				priv->base_addr1,
				priv->irq);

		/* Save pci bridge config (if needed) */
		WAN_HWCALL(getcfg, 
			(card->hw, SDLA_PCIEXPRESS, &priv->pci_express_bridge));
		if (priv->pci_express_bridge){
			int	off = 0;

			WAN_HWCALL(pci_bridge_read_config_dword, 
				(card->hw, 0x04, &priv->pci_bridge_base_class));
			WAN_HWCALL(pci_bridge_read_config_dword, 
				(card->hw, PCI_IO_BASE_DWORD, &priv->pci_bridge_base_addr0));
			WAN_HWCALL(pci_bridge_read_config_dword, 
				(card->hw, PCI_MEM_BASE0_DWORD, &priv->pci_bridge_base_addr1));
			WAN_HWCALL(pci_bridge_read_config_byte, 
				(card->hw, PCI_INT_LINE_BYTE, &priv->pci_bridge_irq));
			for(off=0;off<=15;off++){
				WAN_HWCALL(pci_bridge_read_config_dword, 
					(card->hw, off*4, &priv->pci_bridge_cfg[off]));
			}		
			DEBUG_EVENT("%s: PCI_ExpressBridge: BaseClass %X BaseAddr %X:%X IRQ %d\n", 
					wan_netif_name(dev),
					priv->pci_bridge_base_class,
					priv->pci_bridge_base_addr0,
					priv->pci_bridge_base_addr1,
					priv->pci_bridge_irq);
		}

#if defined(ENABLED_IRQ)
		if(request_irq(irq, wan_aften_isr, SA_SHIRQ, card->devname, card)){
			DEBUG_EVENT("%s: Can't reserve IRQ %d!\n", 
					card->devname, irq);
			sdla_unregister(&card->hw, card->devname);
			goto wan_aften_setup_error;			
		}
#endif
	}else{
		DEBUG_EVENT("%s: USB device is ready (%p)\n",
				card->devname, card);
	}
	return 0;

wan_aften_setup_error:
	return -EINVAL;
}

static int wan_aften_release(sdla_t *card)
{
	if (card->type != SDLA_USB){
#if defined(ENABLED_IRQ)
		struct wan_aften_priv	*priv = wan_netif_priv(dev);
		free_irq(priv->irq, card);
#endif
	}
	if (card->hw_iface.hw_down){
		card->hw_iface.hw_down(card->hw);
	}
	if (card->hw){
		sdla_unregister(&card->hw, card->devname);
	}else{
		DEBUG_EVENT("%s: ERROR: HW device pointer is NULL!\n",
				card->devname);
	}
	return 0;
}

#if defined(CONFIG_PRODUCT_WANPIPE_USB)
static int wan_aften_callback_add_device(void)
{
	int	err = 0;

	err = wan_aften_add_device(0);
	if (err){
		DEBUG_EVENT("%s: ERROR: Failed to add new device!\n",
					__FUNCTION__);
		return -EINVAL;
	}
	return 0;
}
static int wan_aften_callback_delete_device(char *devname)
{
	sdla_t	*card, *card_prev = NULL;
	int	err = -ENODEV;

	for (card=card_list;card;){
		if (strcmp(card->devname, devname) == 0){
			wan_aften_delete_device(card);
			if (card_list == card){
				card_list = card->list;
			}else{
				card_prev->list = card->list;
			}
			wan_free(card);
			return 0;
		}
		card_prev = card;
		card = card->list;
	}
	return err;
}
#endif

static int wan_aften_update_ports(void)
{
	sdla_t		*card, *prev = NULL;
#if 0
	unsigned char	*hwprobe;
	int		err;
#endif
	
	DEBUG_TEST("\n\nList of available Sangoma devices:\n");
	
	for (card=card_list; card; card = card->list){
		netdevice_t	*dev;
		dev = WAN_DEVLE2DEV(WAN_LIST_FIRST(&card->wandev.dev_head));
		if (dev){
			if (prev && prev->hw == card->hw){
				card->wandev.comm_port = 
					prev->wandev.comm_port + 1;
			}
			
			DEBUG_TEST("dev name: %s\n", wan_netif_name(dev));
			
#if 0
			err = card->hw_iface.get_hwprobe(
						card->hw, 
						card->wandev.comm_port, 
						(void**)&hwprobe); 
			if (!err){
				hwprobe[strlen(hwprobe)] = '\0';
				DEBUG_EVENT("%s: %s\n",
						wan_netif_name(dev),
						hwprobe);
			}
#endif
		}
		prev = card;
	}
	return 0;
}


static int wan_aften_read_reg(sdla_t *card, wan_cmd_api_t *api_cmd, int idata)
{

	if (api_cmd->len == 1){
		if (api_cmd->offset <= 0x3C){
			card->hw_iface.pci_read_config_byte(
					card->hw,
					api_cmd->offset,
					(unsigned char*)&api_cmd->data[idata]);
		}else{
			card->hw_iface.bus_read_1(
					card->hw,
				       	api_cmd->offset,
			       		(unsigned char*)&api_cmd->data[idata]);
		}
		DEBUG_TEST("%s: Reading Off=0x%08X Len=%i Data=0x%02X\n",
				card->devname,
				api_cmd->offset,
				api_cmd->len,
				*(unsigned char*)&api_cmd->data[idata]);
	}else if (api_cmd->len == 2){
		if (api_cmd->offset <= 0x3C){
			card->hw_iface.pci_read_config_word(
					card->hw,
					api_cmd->offset,
					(unsigned short*)&api_cmd->data[idata]);
		}else{
			card->hw_iface.bus_read_2(
					card->hw,
			       		api_cmd->offset,
				       	(unsigned short*)&api_cmd->data[idata]);
		}
		DEBUG_TEST("%s: Reading Off=0x%08X Len=%i Data=0x%04X\n",
				card->devname,
				api_cmd->offset,
				api_cmd->len,
				*(unsigned short*)&api_cmd->data[idata]);
	}else if (api_cmd->len == 4){
		if (api_cmd->offset <= 0x3C){
			card->hw_iface.pci_read_config_dword(
					card->hw,
					api_cmd->offset,
					(unsigned int*)&api_cmd->data[idata]);
		}else{
			card->hw_iface.bus_read_4(
					card->hw,
				       	api_cmd->offset,
				       	(unsigned int*)&api_cmd->data[idata]);
		}
		DEBUG_TEST("ADEBUG: %s: Reading Off=0x%08X Len=%i Data=0x%04X (idata=%d)\n",
				card->devname,
				api_cmd->offset,
				api_cmd->len,
				*(unsigned int*)&api_cmd->data[idata],
				idata);
	}else{
		card->hw_iface.peek(
				card->hw,
				api_cmd->offset,
				(unsigned char*)&api_cmd->data[idata],
				api_cmd->len);
#if 0
		memcpy_fromio((unsigned char*)&api_cmd.data[0],
				(unsigned char*)vector, api_cmd.len);
#endif

		DEBUG_TEST("%s: Reading Off=0x%08X Len=%i\n",
				card->devname,
				api_cmd->offset,
				api_cmd->len);
	}
	
	return 0;	
}

static int wan_aften_all_read_reg(sdla_t *card_head, wan_cmd_api_t *api_cmd)
{
	sdla_t		*card, *prev = NULL;
	int 		idata = 0;
	
	for (card=card_list; card; card = card->list){
#if 0
		if (prev == NULL || prev->hw != card->hw){
#endif
		if (prev == NULL||!card->hw_iface.hw_same(prev->hw, card->hw)){
			wan_aften_read_reg(card, api_cmd, idata);
			idata += api_cmd->len;
		}
		prev = card;

	}
	
	/*Update api_cmd->len to the new length of data to copy up
	  as the ioctl call will copy up only api_cmd->len to increase
	  the speed of a firmware update*/
	api_cmd->len=idata;

	return 0;	
}

static int wan_aften_write_reg(sdla_t *card, wan_cmd_api_t *api_cmd)
{
	if (api_cmd->len == 1){
		card->hw_iface.bus_write_1(
				card->hw,
				api_cmd->offset,
				*(unsigned char*)&api_cmd->data[0]);
		DEBUG_TEST("%s: Write  Offset=0x%08X Data=0x%02X\n",
			card->devname,api_cmd->offset,
			*(unsigned char*)&api_cmd->data[0]);
	}else if (api_cmd->len == 2){
		card->hw_iface.bus_write_2(
				card->hw,
				api_cmd->offset,
				*(unsigned short*)&api_cmd->data[0]);
		DEBUG_TEST("%s: Write  Offset=0x%08X Data=0x%04X\n",
			card->devname,api_cmd->offset,
			*(unsigned short*)&api_cmd->data[0]);
	}else if (api_cmd->len == 4){
		card->hw_iface.bus_write_4(
				card->hw,
				api_cmd->offset,
				*(unsigned int*)&api_cmd->data[0]);
		DEBUG_TEST("ADEBUG: %s: Write  Offset=0x%08X Data=0x%08X\n",
			card->devname,api_cmd->offset,
			*(unsigned int*)&api_cmd->data[0]);
	}else{
		card->hw_iface.poke(
				card->hw,
				api_cmd->offset,
				(unsigned char*)&api_cmd->data[0],
				api_cmd->len);
#if 0
		memcpy_toio((unsigned char*)vector,
			(unsigned char*)&api_cmd->data[0], api_cmd->len);
#endif
	}
	return 0;
}

static int wan_aften_all_write_reg(sdla_t *card_head, wan_cmd_api_t *api_cmd)
{
	sdla_t		*card, *prev = NULL;

	for (card=card_list; card; card = card->list){
#if 0
		if (prev == NULL || prev->hw != card->hw){
#endif
		if (prev == NULL||!card->hw_iface.hw_same(prev->hw, card->hw)){
			wan_aften_write_reg(card, api_cmd);
		}
		prev = card;
	}

	return 0;	
}


static int wan_aften_set_pci_bios(sdla_t *card)
{
	netdevice_t		*dev = NULL;
	struct wan_aften_priv	*priv;

	dev = WAN_DEVLE2DEV(WAN_LIST_FIRST(&card->wandev.dev_head));
	if (dev == NULL){
		return -EINVAL;
	}
	priv= wan_netif_priv(dev);
	DEBUG_TEST("%s: Set PCI BaseClass %X BaseAddr0 %X BaseAddr1 %X IRQ %d\n",
				wan_netif_name(dev),
			       	priv->base_class,
			       	priv->base_addr0,
			       	priv->base_addr1,
				priv->irq);

	if (priv->pci_express_bridge){
		int off = 0;
		WAN_HWCALL(pci_bridge_write_config_dword, 
			(card->hw, 0x04, priv->pci_bridge_base_class));
		WAN_HWCALL(pci_bridge_write_config_dword, 
			(card->hw, PCI_IO_BASE_DWORD, priv->pci_bridge_base_addr0));
		WAN_HWCALL(pci_bridge_write_config_dword, 
			(card->hw, PCI_MEM_BASE0_DWORD, priv->pci_bridge_base_addr1));
		WAN_HWCALL(pci_bridge_write_config_byte, 
			(card->hw, PCI_INT_LINE_BYTE, priv->pci_bridge_irq));
		for(off=0;off<=15;off++){
			WAN_HWCALL(pci_bridge_write_config_dword, 
				(card->hw, off*4, priv->pci_bridge_cfg[off]));
		}		
	}				
	WP_DELAY(200);
	card->hw_iface.pci_write_config_dword(
			card->hw, 0x04, priv->base_class);
	card->hw_iface.pci_write_config_dword(
			card->hw, PCI_IO_BASE_DWORD, priv->base_addr0);
	card->hw_iface.pci_write_config_dword(
			card->hw, PCI_MEM_BASE0_DWORD, priv->base_addr1);
	card->hw_iface.pci_write_config_byte(
			card->hw, PCI_INT_LINE_BYTE, priv->irq);
	return 0;
}

static int wan_aften_all_set_pci_bios(sdla_t *card_head)
{
	sdla_t		*card, *prev = NULL;

	for (card=card_list; card; card = card->list){
#if 0
		if (prev == NULL || prev->hw != card->hw){
#endif
		if (prev == NULL||!card->hw_iface.hw_same(prev->hw, card->hw)){
			if (wan_aften_set_pci_bios(card)){
				return -EINVAL;
			}
		}
		prev = card;
	}

	return 0;	
}

static int wan_aften_hwprobe(sdla_t *card, wan_cmd_api_t *api_cmd)
{
	netdevice_t	*dev;
	unsigned char	*str;
	int		err;

	dev = WAN_DEVLE2DEV(WAN_LIST_FIRST(&card->wandev.dev_head));
	if (dev && card->hw_iface.get_hwprobe && card->hw_iface.getcfg){
		char	ver;
		memcpy(&api_cmd->data[api_cmd->len], 
				wan_netif_name(dev), 
				strlen(wan_netif_name(dev)));
		api_cmd->len += strlen(wan_netif_name(dev));
		api_cmd->data[api_cmd->len++] = ':';
		api_cmd->data[api_cmd->len++] = ' ';
		err = card->hw_iface.get_hwprobe(
				card->hw, card->wandev.comm_port, (void**)&str); 
		if (err){
			return err;
		}
		str[strlen(str)] = '\0';
		memcpy(&api_cmd->data[api_cmd->len], str, strlen(str));
		api_cmd->len += strlen(str);	/* set to number of cards */
		api_cmd->data[api_cmd->len++] = ' ';
		api_cmd->data[api_cmd->len++] = '(';
		api_cmd->data[api_cmd->len++] = 'V';
		api_cmd->data[api_cmd->len++] = 'e';
		api_cmd->data[api_cmd->len++] = 'r';
		api_cmd->data[api_cmd->len++] = '.';
		err = card->hw_iface.getcfg(
				card->hw, SDLA_COREREV, &ver);
		if (err){
			return err;
		}
		sprintf(&api_cmd->data[api_cmd->len], "%02X", ver);
		api_cmd->len += 2;
		api_cmd->data[api_cmd->len++] = ')';
		api_cmd->data[api_cmd->len++] = '\n';
		DEBUG_TEST("%s: Read card hwprobe (%s)!\n",
					wan_netif_name(dev),
					api_cmd->data);
	}else{
		return -EINVAL;
	}
	return 0;
}

static int wan_aften_all_hwprobe(sdla_t *card_head, wan_cmd_api_t *api_cmd)
{
	sdla_t	*card, *prev=NULL;
#if 0
	int 	bus = -1, slot = -1;
#endif
	int	err = 0;

	for (card=card_list; card; card = card->list){
		if (prev == NULL||!card->hw_iface.hw_same(prev->hw, card->hw)){
			if ((err = wan_aften_hwprobe(card, api_cmd))){
				return err;
			}
		}
		prev = card;
#if 0
		netdevice_t		*dev = WAN_DEVLE2DEV(WAN_LIST_FIRST(&card->wandev.dev_head));
		struct wan_aften_priv	*priv = wan_netif_priv(dev);
		if (bus == -1 || 
		    !(bus == priv->bus && slot == priv->slot)){
			if ((err = wan_aften_hwprobe(card, api_cmd))){
				return err;
			}
		}
		bus = priv->bus;
		slot = priv->slot;
#endif
	}

	return 0;	
}

static int
wan_aften_read_pcibridge_reg(sdla_t *card, wan_cmd_api_t *api_cmd, int idata)
{
	if (card->hw_iface.pci_bridge_read_config_dword == NULL){
		return -EINVAL;
	}
	card->hw_iface.pci_bridge_read_config_dword(
				card->hw,
				api_cmd->offset,
				(u32*)&api_cmd->data[idata]);	
	DEBUG_TEST("%s: Reading value from %X: %X\n",
				card->devname,
				api_cmd->offset,
				api_cmd->data[idata]);		
	return 0;	
}

static int 
wan_aften_all_read_pcibridge_reg(sdla_t *card_head, wan_cmd_api_t *api_cmd)
{
	sdla_t		*card, *prev = NULL;
	int 		idata = 0;
	
	for (card=card_list; card; card = card->list){
		if (prev == NULL||!card->hw_iface.hw_same(prev->hw, card->hw)){
			wan_aften_read_pcibridge_reg(card, api_cmd, idata);
			idata += api_cmd->len;
		}
		prev = card;
	}

	return 0;	
}

static int wan_aften_write_pcibridge_reg(sdla_t *card, wan_cmd_api_t *api_cmd)
{
	if (card->hw_iface.pci_bridge_write_config_dword == NULL){
		return -EINVAL;
	}
	card->hw_iface.pci_bridge_write_config_dword(
				card->hw,
				api_cmd->offset,
				*(unsigned int*)&api_cmd->data[0]);
	DEBUG_TEST("%s: Writing value %X to %X\n",
				card->devname,
				*(unsigned int*)&api_cmd->data[0],
				api_cmd->offset);
						
	return 0;
}

static int wan_aften_all_write_pcibridge_reg(sdla_t *card_head, wan_cmd_api_t *api_cmd)
{
	sdla_t		*card, *prev = NULL;

	for (card=card_list; card; card = card->list){
		if (prev == NULL||!card->hw_iface.hw_same(prev->hw, card->hw)){
			wan_aften_write_pcibridge_reg(card, api_cmd);
		}
		prev = card;
	}

	return 0;	
}



static int wan_aften_open(netdevice_t *dev)
{
	WAN_NETIF_START_QUEUE(dev);
	return 0;
}

static int wan_aften_close(netdevice_t *dev)
{
	WAN_NETIF_STOP_QUEUE(dev);
	return 0;
}

#if defined(CONFIG_PRODUCT_WANPIPE_USB)

static int wan_aften_usb_write_access(sdla_t *card, wan_cmd_api_t *api_cmd)
{

	if (card->type != SDLA_USB){
		DEBUG_EVENT("%s: Unsupported command for current device!\n",
					card->devname);
		return -EINVAL;
	}

	switch(api_cmd->cmd){
	case SIOC_WAN_USB_CPU_WRITE_REG:
		DEBUG_TEST("%s: Write USB-CPU CMD: Reg:0x%02X <- 0x%02X\n",
				card->devname, 
				(unsigned char)api_cmd->offset,
				(unsigned char)api_cmd->data[0]);
		api_cmd->ret = card->hw_iface.usb_cpu_write(	
						card->hw, 
						(u_int8_t)api_cmd->offset, 
						(u_int8_t)api_cmd->data[0]);
		break;

	case SIOC_WAN_USB_FW_DATA_WRITE:
		if (api_cmd->len){
			DEBUG_TEST("%s: Write USB-FXO DATA: %d (%d) bytes\n",
					card->devname, api_cmd->len, api_cmd->offset);
			api_cmd->len = card->hw_iface.usb_txdata_raw(
						card->hw,
						&api_cmd->data[0],
						api_cmd->len);
			api_cmd->ret = 0;
		}else{
			DEBUG_TEST("%s: Write USB-FXO DATA Ready?\n",
					card->devname);
			api_cmd->ret = card->hw_iface.usb_txdata_raw_ready(card->hw);
		}
		break;

	case SIOC_WAN_USB_FE_WRITE_REG:
		DEBUG_TEST("%s: Write USB-FXO CMD: Module:%d Reg:%d <- 0x%02X\n",
				card->devname, api_cmd->bar,
				(unsigned char)api_cmd->offset,
				(unsigned char)api_cmd->data[0]);
		api_cmd->ret = card->hw_iface.fe_write(card->hw,
						api_cmd->bar,
						(u_int8_t)api_cmd->offset,
						(u_int8_t)api_cmd->data[0]);
		break;	

	default:
		DEBUG_EVENT("%s: %s:%d: Invalid command\n", card->devname,__FUNCTION__,__LINE__);
		api_cmd->ret = -EBUSY;
		break;
	}	

	return 0;
}

static int wan_aften_usb_read_access(sdla_t *card, wan_cmd_api_t *api_cmd)
{
	int	err = 0;

	if (card->type != SDLA_USB){
		DEBUG_EVENT("%s: Unsupported command for current device!\n",
					card->devname);
		return -EINVAL;
	}

	switch(api_cmd->cmd){
	case SIOC_WAN_USB_CPU_READ_REG:
		api_cmd->data[0] = 0xFF;
		/* Add function here */
		err = card->hw_iface.usb_cpu_read(
						card->hw,
						(u_int8_t)api_cmd->offset,
						(u_int8_t*)&api_cmd->data[0]);
		if (err){
			api_cmd->ret = -EBUSY;
			return 0;
		}
		DEBUG_TEST("%s: Read USB-CPU CMD: Reg:0x%02x -> 0x%02X\n",
				card->devname, 
				(unsigned char)api_cmd->offset,
				(unsigned char)api_cmd->data[0]);
		api_cmd->ret = 0;
		api_cmd->len = 1;
		break;

	case SIOC_WAN_USB_FW_DATA_READ:
		api_cmd->len = 
			card->hw_iface.usb_rxdata_raw(	card->hw,
							&api_cmd->data[0],
							api_cmd->len); 
		DEBUG_TEST("%s: Read USB-FXO Data: %d (%d) bytes\n",
				card->devname, api_cmd->ret, api_cmd->len);
		api_cmd->ret = 0;
		break;

	case SIOC_WAN_USB_FE_READ_REG:
		/* Add function here */
		api_cmd->data[0] = card->hw_iface.fe_read(
						card->hw, 
						api_cmd->bar, 
						(u_int8_t)api_cmd->offset);

		DEBUG_TEST("%s: Read USB-FXO CMD: Mod:%d Reg:%d -> 0x%02X\n",
				card->devname,  api_cmd->bar,
				(unsigned char)api_cmd->offset,
				(unsigned char)api_cmd->data[0]);
		api_cmd->ret = 0;
		api_cmd->len = 1;
		break;	

	default:
		DEBUG_EVENT("%s: %s:%d: Invalid command\n", card->devname,__FUNCTION__,__LINE__);
		api_cmd->ret = -EBUSY;
		break;
	}	
	return 0;
}

static int wan_aften_usb_fwupdate_enable(sdla_t *card)
{
	if (card->type != SDLA_USB){
		DEBUG_EVENT("%s: Unsupported command for current device!\n",
					card->devname);
		return -EINVAL;
	}
	return card->hw_iface.usb_fwupdate_enable(card->hw); 
}                

#endif

static int
wan_aften_ioctl (netdevice_t *dev, struct ifreq *ifr, wan_ioctl_cmd_t cmd)
{
	sdla_t			*card;
	struct wan_aften_priv	*priv= wan_netif_priv(dev);
	wan_cmd_api_t		*api_cmd;
	wan_cmd_api_t		*usr_api_cmd;
	wan_cmd_api_t		api_cmd_struct;
	int			err=-EINVAL;

	api_cmd=&api_cmd_struct;

	if (!priv || !priv->common.card){
		DEBUG_EVENT("%s: Invalid structures!\n", wan_netif_name(dev));
		return -ENODEV;
	}

	DEBUG_TEST("%s: CMD=0x%X\n",__FUNCTION__,cmd);

	if (cmd != SIOC_WAN_DEVEL_IOCTL){
		DEBUG_IOCTL("%s: Unsupported IOCTL call!\n", 
					wan_netif_name(dev));
		return -EINVAL;
	}

	if (!ifr->ifr_data){
		DEBUG_EVENT("%s: No API data!\n", wan_netif_name(dev));
		return -EINVAL;
	}

	memset(api_cmd,0,sizeof(wan_cmd_api_t));
	
	card = priv->common.card;
	usr_api_cmd = (wan_cmd_api_t*)ifr->ifr_data;

	/* Only copy the struct header + 8 bytes of data */
	if (WAN_COPY_FROM_USER(api_cmd,usr_api_cmd,sizeof(wan_cmd_api_t)-WAN_MAX_CMD_DATA_SIZE+8)){
		return -EFAULT;
	}

	/* If there is any length in data copy the data structure */
	if (api_cmd->len) {
		if (WAN_COPY_FROM_USER(api_cmd->data,usr_api_cmd->data,api_cmd->len)){
	                return -EFAULT;
        	}
	}

	/* Hardcode bar access FELD */
	switch (api_cmd->cmd){
	case SIOC_WAN_READ_REG:
		err = wan_aften_read_reg(card, api_cmd, 0);
		break;
	
	case SIOC_WAN_ALL_READ_REG:
		err = wan_aften_all_read_reg(card_list, api_cmd);
		break;

	case SIOC_WAN_WRITE_REG:
		err = wan_aften_write_reg(card, api_cmd);
		break;

	case SIOC_WAN_ALL_WRITE_REG:
		err = wan_aften_all_write_reg(card_list, api_cmd);
		break;
		
	case SIOC_WAN_SET_PCI_BIOS:
		err = wan_aften_set_pci_bios(card);
		break;

	case SIOC_WAN_ALL_SET_PCI_BIOS:
		err = wan_aften_all_set_pci_bios(card_list);
		break;
		
	case SIOC_WAN_HWPROBE:
		DEBUG_TEST("%s: Read Sangoma device hwprobe!\n",
					wan_netif_name(dev));
		memset(&api_cmd->data[0], 0, WAN_MAX_CMD_DATA_SIZE);
		api_cmd->len = 0;
		err = wan_aften_hwprobe(card, api_cmd);
		break;
		
	case SIOC_WAN_ALL_HWPROBE:
		DEBUG_TEST("%s: Read list of Sangoma devices!\n",
					wan_netif_name(dev));
		memset(&api_cmd->data[0], 0, WAN_MAX_CMD_DATA_SIZE);
		api_cmd->len = 0;
		err = wan_aften_all_hwprobe(card, api_cmd);
		break;

	case SIOC_WAN_COREREV:
		if (card->hw_iface.getcfg){
			err = card->hw_iface.getcfg(
					card->hw,
				       	SDLA_COREREV,
					&api_cmd->data[0]);
			api_cmd->len = 1;
		}
		DEBUG_TEST("%s: Get core revision (rev %X)!\n", 
				wan_netif_name(dev), api_cmd->data[0]);
		break;
	case SIOC_WAN_READ_PCIBRIDGE_REG:
		err = wan_aften_read_pcibridge_reg(card, api_cmd, 0);
		break;
	case SIOC_WAN_ALL_READ_PCIBRIDGE_REG:
		err = wan_aften_all_read_pcibridge_reg(card, api_cmd);
		break;
	case SIOC_WAN_WRITE_PCIBRIDGE_REG:
		err = wan_aften_write_pcibridge_reg(card, api_cmd);
		break;
	case SIOC_WAN_ALL_WRITE_PCIBRIDGE_REG:
		err = wan_aften_all_write_pcibridge_reg(card, api_cmd);
		break;
#if defined(CONFIG_PRODUCT_WANPIPE_USB)
	case SIOC_WAN_USB_CPU_READ_REG:
	case SIOC_WAN_USB_FW_DATA_READ:
	case SIOC_WAN_USB_FE_READ_REG:
		err = wan_aften_usb_read_access(card, api_cmd);
		break;
	case SIOC_WAN_USB_CPU_WRITE_REG:
	case SIOC_WAN_USB_FW_DATA_WRITE:
	case SIOC_WAN_USB_FE_WRITE_REG:
		err = wan_aften_usb_write_access(card, api_cmd);
		break;
	case SIOC_WAN_USB_FWUPDATE_ENABLE:
		err = wan_aften_usb_fwupdate_enable(card);
		break;
#endif                 
	default:
		DEBUG_EVENT("%s: Unknown WAN_AFTEN command %X\n",
				card->devname, api_cmd->cmd);
	}


	DEBUG_TEST("%s: CMD Executed err= %d len=%i\n",card->devname,api_cmd->cmd,api_cmd->len);

	if (WAN_COPY_TO_USER(usr_api_cmd,api_cmd,sizeof(wan_cmd_api_t)-WAN_MAX_CMD_DATA_SIZE)){
		return -EFAULT;
	}	
	
	if (api_cmd->len) {

		if (api_cmd->len >= WAN_MAX_CMD_DATA_SIZE) {
			DEBUG_EVENT("%s: Error API CMD exceeded available data space!\n",card->devname);
			return -EFAULT;
		}

		if (WAN_COPY_TO_USER(usr_api_cmd->data,api_cmd->data,api_cmd->len)){
	                return -EFAULT;
        	}
	}
	return err;
}

#if WAN_AFTEN_FE_RW

/********* Private Functions **********/
    
static void wan_aftup_init_critical_section(void *pmutex)
{
	wan_mutex_lock_init(pmutex, "fe_rw_mutex");
}

static void wan_aftup_enter_critical_section(void *pmutex)
{
	wan_mutex_lock(pmutex, NULL);
}

static void wan_aftup_leave_critical_section(void *pmutex)
{
	wan_mutex_unlock(pmutex, NULL);
}

static unsigned int wan_aft_read_maxim(void *card, int offset, int size, void *out_buffer)
{
	int err;
	wan_cmd_api_t api_cmd;

	DEBUG_FE_EXPORT("%s()\n", __FUNCTION__);

	memset(&api_cmd, 0x00, sizeof(api_cmd));

	api_cmd.len = size;
	api_cmd.offset = offset;

	err = wan_aften_read_reg(card, &api_cmd, 0);

	if (!err) {
		memcpy(out_buffer, api_cmd.data, size);
	}

    return err;
}


static unsigned int wan_aft_write_maxim(void *card, int offset, int size, void *in_buffer)
{
	int err;
	wan_cmd_api_t api_cmd;

	DEBUG_FE_EXPORT("%s()\n", __FUNCTION__);

	memset(&api_cmd, 0x00, sizeof(api_cmd));

	api_cmd.len = size;
	api_cmd.offset = offset;
	memcpy(api_cmd.data, in_buffer, size);

	err = wan_aften_write_reg(card, &api_cmd);

    return err;
}

unsigned int write_cpld(sdla_t *card, unsigned short cpld_off, unsigned short cpld_data)
{
    cpld_off &= ~AFT8_BIT_DEV_ADDR_CLEAR;
    cpld_off |= AFT8_BIT_DEV_ADDR_CPLD;

    wan_aft_write_maxim(card, 0x46, 2, &cpld_off);
    wan_aft_write_maxim(card, 0x44, 2, &cpld_data);
    return 0;
}

static int wan_aften_te1_disable_global_chip_reset(sdla_t *card)
{
    unsigned int data;
    int err, i;

    for (i = 0; i < 2; i++) {
        /* put in reset */
        write_cpld(card, 0x00, 0x06);

        /* sangoma_msleep(50); */

        /* Disable Global Chip/FrontEnd/CPLD Reset */
        err = wan_aft_read_maxim(card, 0x40, 4, &data);
        if (err){
            return 1;
        }

        data &= ~0x6;

        err = wan_aft_write_maxim(card, 0x40, 4, &data);
        if (err){
            return 1;
        }

        /* sangoma_msleep(50); */
    }

    return 0;
}

static int wan_aften_check_id_reg(sdla_t *card)
{
    unsigned char val, fe_id;

	wan_aftup_te1_reg_read(card, REG_IDR, &val);

	DEBUG_FE_EXPORT("val = 0x%X\n", val);

	fe_id = DEVICE_ID_DS(val);

	switch(fe_id)
    {
    case DEVICE_ID_DS26528:
        DEBUG_EVENT("Device is DS26528\n");
        break;
    case DEVICE_ID_DS26524:
        DEBUG_EVENT("Device is DS26524\n");
        break;
/*
    case DEVICE_ID_DS26522: //not installed on Sangoma HW
        DEBUG_EVENT("Device is DS26522!\n");
        break;
*/
    case DEVICE_ID_DS26521:
    	DEBUG_EVENT("Device is DS26521\n");
        break;
    default:
        DEBUG_ERROR("Failed to read Maxim ID (%02X)\n", val);
        return 1;
    }

    return 0;
}


/************ Public Functions ************/

void *wan_aftup_te1_global_init(int card_number)
{
	sdla_t *card = NULL, *found_card = NULL;
	int card_counter = 0;
			
	DEBUG_FE_EXPORT("%s()\n", __FUNCTION__);
		
	wan_aftup_init_critical_section(&rw_mutex);

	for (card = card_list; card; card = card->list){
		
		if (card_counter == card_number){
			/* found the card */
			found_card = card;
			DEBUG_FE_EXPORT("card name: %s\n", card->devname);
			break;
		}

		card_counter++;
	}
	
	if (!found_card) {
		DEBUG_ERROR("Error: failed to find card_number %d",
			card_number);
		return NULL;
	}

    if (wan_aften_te1_disable_global_chip_reset(found_card)) {
        return NULL;
    }

    /* read the Chip ID and check is it valid */
    if (wan_aften_check_id_reg(found_card)) {
		return NULL;
	}

	return found_card;
}

int wan_aftup_te1_reg_write(void *card, unsigned short reg, unsigned short in_data)
{
	unsigned int tmp;

	DEBUG_FE_EXPORT("%s()\n", __FUNCTION__);

    wan_aftup_enter_critical_section(&rw_mutex);

    if (reg & 0x800)  reg |= 0x2000;
    if (reg & 0x1000) reg |= 0x4000;

    reg &= ~AFT8_BIT_DEV_ADDR_CLEAR;
    //reg |= AFT8_BIT_DEV_MAXIM_ADDR_CPLD;

    DEBUG_FE_EXPORT("%s(): Reg 0x%04X Data 0x%04X\n", __FUNCTION__, reg, in_data);

    /* write the register where the data will be written to */
	if (wan_aft_write_maxim(card, AFT_MCPU_INTERFACE_ADDR, 2, &reg)) {
        DEBUG_ERROR("%s(): Failed to write to AFT_MCPU_INTERFACE_ADDR!\n",
			__FUNCTION__);
    	wan_aftup_leave_critical_section(&rw_mutex);
		/* return error */
		return 1;
	}

    /* read to avoid bridge optimization of two writes by PCI */
	if (wan_aft_read_maxim(card, AFT_MCPU_INTERFACE, 4, &tmp)) {
        DEBUG_ERROR("%s(): Failed to read from AFT_MCPU_INTERFACE!\n",
			__FUNCTION__);
    	wan_aftup_leave_critical_section(&rw_mutex);
		/* return error */
		return 1;
	}

    /* write the data to the register */
	if (wan_aft_write_maxim(card, AFT_MCPU_INTERFACE, 2, &in_data)) {
        DEBUG_ERROR("%s(): Failed to write to AFT_MCPU_INTERFACE!\n",
			__FUNCTION__);
    	wan_aftup_leave_critical_section(&rw_mutex);
		/* return error */
		return 1;
	}

    wan_aftup_leave_critical_section(&rw_mutex);

	/* return success */
	return 0;
}

int wan_aftup_te1_reg_read(void *card, unsigned short reg, unsigned char *out_data)
{
    unsigned int data;

	DEBUG_FE_EXPORT("%s()\n", __FUNCTION__);

    wan_aftup_enter_critical_section(&rw_mutex);

    if (reg & 0x800)  reg |= 0x2000;
    if (reg & 0x1000) reg |= 0x4000;

    reg &= ~AFT8_BIT_DEV_ADDR_CLEAR;
    //reg |= AFT8_BIT_DEV_MAXIM_ADDR_CPLD;

    /* write the register where data will be read from */
    if (wan_aft_write_maxim(card, AFT_MCPU_INTERFACE_ADDR, 2, &reg)) {
        DEBUG_ERROR("%s(): Failed to write to AFT_MCPU_INTERFACE_ADDR!\n",
			__FUNCTION__);
    	wan_aftup_leave_critical_section(&rw_mutex);
		/* return error */
        return 1;
    }

    if (wan_aft_read_maxim(card, AFT_MCPU_INTERFACE, 4,	&data)) {
        DEBUG_ERROR("%s(): Failed to read from AFT_MCPU_INTERFACE!\n",
			__FUNCTION__);
    	wan_aftup_leave_critical_section(&rw_mutex);
		/* return error */
        return 1;
    }

    DEBUG_FE_EXPORT("%s(): Reg 0x%04X Data 0x%08X\n", __FUNCTION__, reg, data);

	/* store data in user buffer*/
	*out_data = (unsigned char)data;

    wan_aftup_leave_critical_section(&rw_mutex);

	/* return success */
    return 0;
}

/* exported functions to be called by other modules */
EXPORT_SYMBOL(wan_aftup_te1_global_init);
EXPORT_SYMBOL(wan_aftup_te1_reg_write);
EXPORT_SYMBOL(wan_aftup_te1_reg_read);

#endif /* WAN_AFTEN_FE_RW */
