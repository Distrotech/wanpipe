/*
 ************************************************************************
 * wanpipe_includes.h													*
 *		WANPIPE(tm) 	Global includes for Sangoma drivers				*
 *																		*
 * Author:	Alex Feldman <al.feldman@sangoma.com>						*
 *======================================================================*
 *																		*
 * Nov 27, 2007	David Rokhvarg	Added header files needed to compile	*
 *                              Sangoma MS Windows Driver and API.		*
 *																		*
 * Aug 10, 2002	Alex Feldman	Initial version							*
 *																		*
 ************************************************************************
 */

#ifndef __WANPIPE_INCLUDES_H
# define __WANPIPE_INCLUDES_H

#if !defined(__NetBSD__) && !defined(__FreeBSD__) && !defined (__OpenBSD__) && !defined(__WINDOWS__) && !defined(__SOLARIS__) && !defined(__LINUX__)
# if defined(__KERNEL__)
#  define __LINUX__
# endif
#endif

#if defined (__KERNEL__) || defined (KERNEL) || defined (_KERNEL)
# ifndef WAN_KERNEL
# define WAN_KERNEL
# endif
#endif


#if defined(__FreeBSD__)
/*
**		***	F R E E B S D	***
# include <stddef.h>
*/
# include <sys/param.h>
# if (__FreeBSD_version > 600000)
#  include <gnu/fs/ext2fs/i386-bitops.h>
# else
#  include <gnu/ext2fs/i386-bitops.h>
# endif
# include <sys/types.h>
# include <sys/systm.h>
# include <sys/endian.h>
# include <sys/syslog.h>
# include <sys/conf.h>
# include <sys/errno.h>
# if (__FreeBSD_version > 400000)
#  include <sys/ioccom.h>
# else
#  include <i386/isa/isa_device.h> 
# endif
# if (__FreeBSD_version >= 410000)
#  include <sys/taskqueue.h>
# endif
# include <sys/malloc.h>
# include <sys/errno.h>
# include <sys/mbuf.h>
# include <sys/sockio.h>
# include <sys/ioctl_compat.h>
# include <sys/socket.h>
# include <sys/callout.h>
# include <sys/kernel.h>
# include <sys/time.h>
# include <sys/random.h>
# include <sys/module.h>
# ifdef __SDLA_HW_LEVEL
#  include <machine/bus.h>
#  include <machine/resource.h>
#  include <sys/bus.h>
#  include <sys/rman.h>
#  include <sys/interrupt.h>
# endif
# include <sys/pciio.h>
# include <sys/filio.h>
# include <sys/uio.h>
# include <sys/tty.h>
# include <sys/ttycom.h>
# include <sys/proc.h>
# include <net/bpf.h>
# include <net/bpfdesc.h>
# include <net/if_dl.h>
# include <net/if_types.h>
# include <net/if.h>
# include <net/if_media.h>
# include <net/if_ppp.h>
# include <net/if_sppp.h>
# include <net/netisr.h>
# include <net/route.h>
# if (__FreeBSD_version >= 501000)
#  include <net/netisr.h>
# elif (__FreeBSD_version > 400000)
#  include <net/intrq.h>
# endif
# include <netinet/in_systm.h>
# include <netinet/in.h>
# include <netinet/in_var.h>
# include <netinet/udp.h>
# include <netinet/ip.h>
# include <netinet/if_ether.h>
# include <netipx/ipx.h>
# include <netipx/ipx_if.h>
#ifdef NETGRAPH
# include <netgraph/ng_message.h>
# include <netgraph/netgraph.h>
# include <netgraph/ng_parse.h>
#endif /* NETGRAPH */
# include <machine/param.h>
# if (__FreeBSD_version < 500000)
#  include <machine/types.h>
# endif
# include <machine/clock.h>
# include <machine/stdarg.h>
# include <machine/atomic.h>
# include <machine/bus.h>
# include <machine/md_var.h>
# include <vm/vm.h>
# include <vm/vm_param.h>
# include <vm/pmap.h>
# include <vm/vm_extern.h>
# include <vm/vm_kern.h>
#elif defined(__NetBSD__)
/*
**		***	N E T B S D	***
*/
# include </usr/include/bitstring.h>
# include <sys/types.h>
# include <sys/param.h>
# include <sys/systm.h>
# include <sys/syslog.h>
# include <sys/ioccom.h>
# include <sys/conf.h>
# include <sys/malloc.h>
# include <sys/errno.h>
# include <sys/exec.h>
# include <sys/lkm.h>
# include <sys/mbuf.h>
# include <sys/sockio.h>
# include <sys/socket.h>
# include <sys/kernel.h>
# include <sys/device.h>
# include <sys/time.h>
# include <sys/callout.h>
# include <sys/tty.h>
# include <sys/ttycom.h>
# include <machine/types.h>
# include <machine/param.h>
# include <machine/cpufunc.h>
# include <machine/bus.h>
# include <machine/stdarg.h>
# include <machine/intr.h>
# include <net/bpf.h>
# include <net/bpfdesc.h>
# include <net/if_dl.h>
# include <net/if_types.h>
# include <net/if.h>
# include <net/if_ether.h>
# include <net/netisr.h>
# include <net/route.h>
# include <net/if_media.h>
# include <net/ppp_defs.h>
# include <net/if_ppp.h>
# include <net/if_sppp.h>
# include <net/if_spppvar.h>
# include <netinet/in_systm.h>
# include <netinet/in.h>
# include <netinet/in_var.h>
# include <netinet/udp.h>
# include <netinet/ip.h>
# include <uvm/uvm_extern.h>
#elif defined(__OpenBSD__)
/*
**		***	O P E N B S D	***
*/
# include </usr/include/bitstring.h>
# include <sys/types.h>
# include <sys/systm.h>
# include <sys/param.h>
# include <sys/syslog.h>
# include <sys/ioccom.h>
# include <sys/conf.h>
# include <sys/malloc.h>
# include <sys/errno.h>
# include <sys/exec.h>
# include <sys/lkm.h>
# include <sys/mbuf.h>
# include <sys/mutex.h>
# include <sys/sockio.h>
# include <sys/socket.h>
# include <sys/kernel.h>
# include <sys/device.h>
# include <sys/time.h>
# include <sys/timeout.h>
# include <sys/tty.h>
# include <sys/ttycom.h>
# include <i386/bus.h>
# if (OpenBSD < 200605)
#  include <machine/types.h>
# endif
# include <machine/param.h>
# include <machine/cpufunc.h>
# include <machine/bus.h>
/*# include <machine/stdarg.h>*/
# include <net/bpf.h>
# include <net/bpfdesc.h>
# include <net/if_dl.h>
# include <net/if_types.h>
# include <net/if.h>
# include <net/netisr.h>
# include <net/route.h>
# include <net/if_media.h>
# include <net/ppp_defs.h>
# include <net/if_ppp.h>
# include <net/if_sppp.h>
# include <netinet/in_systm.h>
# include <netinet/in.h>
# include <netinet/in_var.h>
# include <netinet/if_ether.h>
# include <netinet/udp.h>
# include <netinet/ip.h>
# include <netipx/ipx.h>
# include <netipx/ipx_if.h>
# include <uvm/uvm_extern.h>
#elif defined(__LINUX__)
# ifdef __KERNEL__
/*
**		***	L I N U X	***
*/
#  include <linux/init.h>
#  include <linux/version.h>	/**/

#  if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,18)
#   include <linux/config.h>	/* OS configuration options */
#  endif

#  if LINUX_VERSION_CODE < KERNEL_VERSION(2,3,0)
#   if !(defined __NO_VERSION__) && !defined(_K22X_MODULE_FIX_)
#    define __NO_VERSION__	
#   endif
#  endif
#  if LINUX_VERSION_CODE < KERNEL_VERSION(2,5,0)
  /* Remove experimental SEQ support */
#   define _LINUX_SEQ_FILE_H
# endif
#  include <linux/module.h>
#  include <linux/types.h>
#  include <linux/sched.h>
#  include <linux/mm.h>
#  include <linux/slab.h>
#  include <linux/stddef.h>	/* offsetof, etc. */
#  include <linux/errno.h>	/* returns codes */
#  include <linux/string.h>	/* inline memset, etc */
#  include <linux/ctype.h>
#  include <linux/kernel.h>	/* printk()m and other usefull stuff */
#  include <linux/timer.h>
#  include <linux/kmod.h>
#  include <net/ip.h>
#  include <net/protocol.h>
#  include <net/sock.h>
#  include <net/route.h>
#  include <linux/fcntl.h>
#  include <linux/skbuff.h>
#  include <linux/socket.h>
#  include <linux/poll.h>
#  include <linux/wireless.h>
#  include <linux/in.h>
#  include <linux/inet.h>
#  include <linux/netdevice.h>
#  include <linux/list.h>
#  include <asm/io.h>		/* phys_to_virt() */
#  if LINUX_VERSION_CODE < KERNEL_VERSION(3,4,0)
#   include <asm/system.h>
#  endif
#  include <asm/byteorder.h>
#  include <asm/delay.h>
#  include <linux/pci.h>
# if defined(CONFIG_PRODUCT_WANPIPE_USB)
#  if defined(CONFIG_USB) || defined (CONFIG_USB_SUPPORT) 
#   include <linux/usb.h>
#  else
#   warning "USB Kernel support not found... disabling usb support"
#   undef CONFIG_PRODUCT_WANPIPE_USB
#  endif
# endif
#  include <linux/if.h>
#  include <linux/if_arp.h>
#  include <linux/tcp.h>
#  include <linux/ip.h>
#  include <linux/udp.h>
#  include <linux/ioport.h>
#  include <linux/init.h>
#  include <linux/pkt_sched.h>
#  include <linux/etherdevice.h>
#  include <linux/random.h>
#  include <asm/uaccess.h>
#  include <linux/inetdevice.h>
#  include <linux/vmalloc.h>     /* vmalloc, vfree */
#  include <asm/uaccess.h>        /* copy_to/from_user */
#  include <linux/init.h>         /* __initfunc et al. */
#  if LINUX_VERSION_CODE >= KERNEL_VERSION(2,5,0)
#   include <linux/seq_file.h>
#  endif
#  ifdef CONFIG_INET
#   include <net/inet_common.h>
#  endif
# endif
#elif defined(__SOLARIS__)
/*
**		***	S O L A R I S	***
*/
#include <sys/types.h>
#include <sys/systm.h>
#include <sys/cmn_err.h>
#include <sys/stream.h>
#include <sys/stropts.h>
#include <sys/strsun.h>
#include <sys/kmem.h>
#include <sys/stat.h>
#include <sys/modctl.h>
#include <sys/dditypes.h>
#include <sys/ddi.h>
#include <sys/conf.h>
#include <sys/ethernet.h>
#include <sys/sunddi.h>
#include <sys/ddidmareq.h>
#include <sys/ddimapreq.h>
#include <sys/ddipropdefs.h>
#include <sys/ddidevmap.h>
#include <sys/devops.h>
#include <sys/pci.h>
#include <sys/dlpi.h>
#include <sys/gld.h>

#elif defined(__WINDOWS__)
/*
**		***	W I N D O W S	***
*/

#if defined(WAN_KERNEL) || defined(__KERNEL__)
# include <stdlib.h>
# include <time.h>	/* clock_t */

# if !defined(NDIS_MINIPORT_DRIVER)
#  include <ntddk.h>
#  include "wanpipe_abstr_types.h"
#  include "wanpipe_kernel_types.h"
#  include "aft_core_options.h"
#  include "wanpipe_debug.h"
#  include "wanpipe_kernel.h"
#  include "wanpipe_skb.h"
#  include "wanpipe_defines.h"
#  include "wanpipe_common.h"
#  include "wanpipe_cfg.h"
#  include "sdladrv.h"	/* API definitions */
#  include "wanpipe_abstr.h"
# endif

# if defined( NDIS_MINIPORT_DRIVER )
#  undef BINARY_COMPATIBLE	
#  define BINARY_COMPATIBLE 0 /* Windows 2000 and later (no Win98 support) */
/*#  define NDIS51_MINIPORT   1 */
#  include <ntddk.h>
#  include <ndis.h>
#  include "wanpipe_abstr_types.h"
#  include "wanpipe_kernel_types.h"
#  include "wanpipe_debug.h"
# endif

# else /* WAN_KERNEL */
#  include <windows.h>
#  include "wanpipe_abstr_types.h"
# endif

# include <stdarg.h>
# include <stdio.h>
# include <stddef.h>	/* offsetof, etc. */

#elif defined (__SOLARIS__)
#  include <sys/types.h>
#  include <sys/systm.h>
#  include <sys/cmn_err.h>
#  include <sys/kmem.h>
#  include <sys/stat.h>
#  include <sys/modctl.h>
#  include <sys/ddi.h>
#  include <sys/conf.h>
#  include <sys/sunddi.h>
#  include <sys/ethernet.h>
#  include <sys/dditypes.h>
#  include <sys/ddidmareq.h>
#  include <sys/ddimapreq.h>
#  include <sys/ddipropdefs.h>
#  include <sys/ddidevmap.h>
#  include <sys/devops.h>
#  include <sys/pci.h>
#  include <sys/gld.h>
#  include <netinet/in.h>
#else
# error "Unsupported Operating System!";
#endif

#endif	/* __WANPIPE_INCLUDES_H	*/

