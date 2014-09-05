#ifndef __WAN_AFTEN__
#define __WAN_AFTEN__

#if defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__)
# include <wanpipe_includes.h>
# include <wanpipe_cfg.h>
# include <wanpipe_debug.h>
# include <wanpipe_defines.h>
# include <wanpipe_common.h>
# include <wanpipe_abstr.h>
# include <wanpipe_snmp.h>
# include <sdlapci.h>
# include <if_wanpipe_common.h>
# include <wanpipe_iface.h>
#else
# include <linux/wanpipe_includes.h>
# include <linux/wanpipe_debug.h>
# include <linux/wanpipe_defines.h>
# include <linux/wanpipe_common.h>
# include <linux/if_wanpipe.h>
# include <linux/sdlapci.h>
# include <linux/sdladrv.h>
# include <linux/wanpipe.h>
# include <linux/if_wanpipe_common.h>
//# include <linux/wanpipe_lip_kernel.h>
# include <linux/wanpipe_iface.h>
#endif

#define WAN_AFTEN_VER	1

struct wan_aften_priv {
	wanpipe_common_t	common;
	unsigned int		base_class;
	unsigned int		base_addr0;
	unsigned int		base_addr1;
	unsigned int		irq;
	unsigned int		slot;
	unsigned int		bus;
	
	u_int8_t		pci_express_bridge;
	
	unsigned int		pci_bridge_base_class;
	unsigned int		pci_bridge_base_addr0;
	unsigned int		pci_bridge_base_addr1;
	u_int8_t 		pci_bridge_irq;
	unsigned int		pci_bridge_slot;
	unsigned int		pci_bridge_bus;
	u_int32_t		pci_bridge_cfg[16];
	
};

#endif
