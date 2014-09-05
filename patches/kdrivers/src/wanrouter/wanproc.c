/*****************************************************************************
* wanproc.c	WAN Router Module. /proc filesystem interface.
*
*		This module is completely hardware-independent and provides
*		access to the router using Linux /proc filesystem.
*
* Author: 	Gideon Hack	
* 		Nenad Corbic
*
* Copyright:	(c) 1995-2004 Sangoma Technologies Inc.
*
*		This program is free software; you can redistribute it and/or
*		modify it under the terms of the GNU General Public License
*		as published by the Free Software Foundation; either version
*		2 of the License, or (at your option) any later version.
* ============================================================================
* Aug 20, 2001  Alex Feldman  	Support SNMP.
* May 25, 2001  Alex Feldman  	Added T1/E1 support (TE1).
* May 23, 2001  Nenad Corbic 	Bug fix supplied by Akash Jain. If the user
*                               copy fails free up allocated memory.
* Jun 02, 1999  Gideon Hack	Updates for Linux 2.2.X kernels.
* Jun 29, 1997	Alan Cox	Merged with 1.0.3 vendor code
* Jan 29, 1997	Gene Kozin	v1.0.1. Implemented /proc read routines
* Jan 30, 1997	Alan Cox	Hacked around for 2.1
* Dec 13, 1996	Gene Kozin	Initial version (based on Sangoma's WANPIPE)
*****************************************************************************/



#include "wanpipe_includes.h"
#include "wanpipe_version.h"
#include "wanpipe_defines.h"
#include "wanpipe_debug.h"
#include "wanpipe_common.h"
#include "wanpipe.h"
#include "sdladrv.h"
#include "wanproc.h"
#include "if_wanpipe_common.h"

#if !defined(__WINDOWS__)

#if defined(__NetBSD__) || defined(__FreeBSD__) || defined(__OpenBSD__)
# define STATIC	
# define CONFIG_PROC_FS
#else
# define STATIC	static
#endif

#if defined(__LINUX__)

# if defined(LINUX_2_1) || defined(LINUX_2_4)
#  ifndef proc_mkdir
#   define proc_mkdir(buf, usbdir) create_proc_entry(buf, S_IFDIR, usbdir)
#  endif
# endif

# if defined(LINUX_2_6)
#  define M_STOP_CNT(m) NULL
# else
#  define M_STOP_CNT(m) &m->stop_cnt
# endif

#endif

#define PROC_STATS_1_FORMAT "%25s: %10lu\n"
#define PROC_STATS_2_FORMAT "%25s: %10lu %25s: %10lu\n"
#define PROC_STATS_ALARM_FORMAT "%25s: %10s %25s: %10s\n"
#define PROC_STATS_STR_FORMAT "%25s: %10s\n"

/****** Defines and Macros **************************************************/

#ifndef	wp_min
#define wp_min(a,b) (((a)<(b))?(a):(b))
#endif
#ifndef	wp_max
#define wp_max(a,b) (((a)>(b))?(a):(b))
#endif

#define	PROC_BUFSZ	4000	/* buffer size for printing proc info */

#define PROT_UNKNOWN	"Unknown"
#define PROT_DECODE(prot,cap) ((prot == WANCONFIG_FR) ? ((cap)?"FR":"fr") :\
		                (prot == WANCONFIG_MFR) ? ((cap)?"FR":"fr") : \
			        (prot == WANCONFIG_X25) ? ((cap)?"X25":"x25") : \
			          (prot == WANCONFIG_PPP) ? ((cap)?"PPP":"ppp") : \
				    (prot == WANCONFIG_CHDLC) ? ((cap)?"CHDLC":"chdlc") :\
				     (prot == WANCONFIG_ADSL) ? ((cap)?"ADSL":"adsl") :\
				      (prot == WANCONFIG_MPPP) ? ((cap)?"S51X":"s51x") : \
				       (prot == WANCONFIG_SDLC) ? ((cap)?"SDLC":"sdlc") : \
				       (prot == WANCONFIG_ATM) ? ((cap)?"ATM":"atm") : \
				       (prot == WANCONFIG_AFT) ? ((cap)?"AFT TE1":"aft te1") : \
				       (prot == WANCONFIG_AFT_T116) ? ((cap)?"AFT TE1":"aft te1") : \
				       (prot == WANCONFIG_AFT_SERIAL) ? ((cap)?"A-SERIAL":"aft serial") : \
				       (prot == WANCONFIG_AFT_ANALOG) ? ((cap)?"A-ANALOG":"aft analog") : \
				       (prot == WANCONFIG_AFT_GSM) ? ((cap)?"AFT GSM":"aft gsm") : \
				       (prot == WANCONFIG_AFT_ISDN_BRI) ? ((cap)?"AFT ISDN":"aft isdn") : \
				       (prot == WANCONFIG_AFT_56K) ? ((cap)?"AFT 56K":"aft 56k") : \
				       (prot == WANCONFIG_AFT_TE1) ? ((cap)?"AFT TE1":"aft te1") : \
				       (prot == WANCONFIG_AFT_TE3) ? ((cap)?"AFT TE3":"aft te3") : \
				        PROT_UNKNOWN )
	
#define CARD_DECODE(wandev)    ((wandev->card_type == WANOPT_ADSL) ? "ADSL" : \
				(wandev->card_type == WANOPT_S50X) ? "S508" : \
				(wandev->card_type == WANOPT_AFT) ? "AFT TE1" : \
				(wandev->card_type == WANOPT_AFT_101) ? "A101" : \
				(wandev->card_type == WANOPT_AFT_102) ? "A102" : \
				(wandev->card_type == WANOPT_AFT_104) ? "A104" : \
				(wandev->card_type == WANOPT_AFT_108) ? "A108" : \
				(wandev->card_type == WANOPT_AFT_300) ? "A301" : \
				(wandev->card_type == WANOPT_AFT_300) ? "A301" : \
				(wandev->card_type == WANOPT_AFT_ANALOG) ? "A200/A400" : \
				(wandev->card_type == WANOPT_AFT_ISDN) ? "A500" : \
				(wandev->card_type == WANOPT_AFT_SERIAL) ? "A14X" : \
				(wandev->fe_iface.get_fe_media_string) ? 			\
		 		wandev->fe_iface.get_fe_media_string() : "S514")


/****** Data Types **********************************************************/
#if defined(__LINUX__)
typedef struct wan_stat_entry
{
	struct wan_stat_entry *next;
	char *description;		/* description string */
	void *data;			/* -> data */
	unsigned data_type;		/* data type */
} wan_stat_entry_t;

typedef struct wan_proc_entry
{
	struct proc_dir_entry *protocol_entry;
	int count;
} wan_proc_entry_t;
#endif
/****** Function Prototypes *************************************************/

extern wan_spinlock_t 		wan_devlist_lock;
extern struct wan_devlist_	wan_devlist;

#if defined(CONFIG_PROC_FS)

#define CONF_PCI_FRM	 "%-12s| %-13s| %-9s| %-4u| %-8u| %-5u| %-4s| %-10u|\n"
#define CONF_ISA_FRM	 "%-12s| %-13s| %-9s| %-4u| 0x%-6X| %-5u| %-4s| %-10u|\n"
#define MAP_FRM		 "%-12s| %-50s\n"
#define INTERFACES_FRM	 "%-15s| %-12s| %-6u| %-18s|\n"
#define PROC_FR_FRM	 "%-30s\n"
#define PROC_FR_CFG_FRM	 "%-15s| %-12s| %-5u|\n"
#define PROC_FR_STAT_FRM "%-15s| %-12s| %-14s|\n"

#ifdef CONFIG_PRODUCT_WANPIPE_ANNEXG
 #define STAT_FRM	 "%-12s| %-9s| %-8s| %-14s| %-3u | %-3u | %-3u | %-3u | %-3u | %-3u | %-3u | %-3u | %-3u | %-3u |\n"
#else
 #define STAT_FRM	 "%-12s| %-9s| %-8s| %-14s|\n"
#endif


/* NEW_PROC */
/* Strings */
static char conf_hdr[] =
	"Device name | Protocol Map | Adapter  | IRQ | Slot/IO "
	"| If's | CLK | Baud rate |\n";
#if defined(__LINUX__)
static char map_hdr[] =
	"Device name | Protocol Map \n";
#endif

#ifdef CONFIG_PRODUCT_WANPIPE_ANNEXG
static char stat_hdr[] =
	"Device name | Protocol | Station | Status        |  Wanpipe  |    Lapb   |  X25 Link |  X25 Svc  |    Dsp    |\n";
#else
static char stat_hdr[] =
	"Device name | Protocol | Station | Status        |\n";
#endif
	
static char interfaces_hdr[] =
	"Interface name | Device name | Media | Operational State |\n";

#if defined(__LINUX__)

/* Proc filesystem interface */
#ifndef LINUX_2_6
static int router_proc_perms(struct inode *, int);
static ssize_t router_proc_read(struct file*, char*, size_t,loff_t*);
static ssize_t router_proc_write(struct file*, const char*, size_t,loff_t*);
#endif

/* Methods for preparing data for reading proc entries */

#if defined(LINUX_2_6)
static int config_get_info(struct seq_file *m, void *v);
static int status_get_info(struct seq_file *m, void *v);
static int probe_get_info(struct seq_file *m, void *v);
static int probe_get_info_legacy(struct seq_file *m, void *v);
static int probe_get_info_verbose(struct seq_file *m, void *v);
static int probe_get_info_dump(struct seq_file *m, void *v);
static int wandev_get_info(struct seq_file *m, void *v);

static int map_get_info(struct seq_file *m, void *v);
static int interfaces_get_info(struct seq_file *m, void *v);

static int get_dev_config_info(struct seq_file *m, void *v);
static int get_dev_status_info(struct seq_file *m, void *v);

static int wandev_mapdir_get_info(struct seq_file *m, void *v);

#elif defined(LINUX_2_4)
static int config_get_info(char* buf, char** start, off_t offs, int len);
static int status_get_info(char* buf, char** start, off_t offs, int len);
static int probe_get_info(char* buf, char** start, off_t offs, int len);
static int probe_get_info_legacy(char* buf, char** start, off_t offs, int len);
static int probe_get_info_verbose(char* buf, char** start, off_t offs, int len);
static int probe_get_info_dump(char* buf, char** start, off_t offs, int len);
static int wandev_get_info(char* buf, char** start, off_t offs, int len);

static int map_get_info(char* buf, char** start, off_t offs, int len);
static int interfaces_get_info(char* buf, char** start, off_t offs, int len);

static int get_dev_config_info(char* buf, char** start, off_t offs, int len);
static int get_dev_status_info(char* buf, char** start, off_t offs, int len);

static int wandev_mapdir_get_info(char* buf, char** start, off_t offs, int len);

#else
static int config_get_info(char* buf, char** start, off_t offs, int len, int dummy);
static int status_get_info(char* buf, char** start, off_t offs, int len, int dummy);
static int probe_get_info(char* buf, char** start, off_t offs, int len, int dummy);
static int probe_get_info_legacy(char* buf, char** start, off_t offs, int len, int dummy);
static int probe_get_info_verbose(char* buf, char** start, off_t offs, int len, int dummy);
static int probe_get_info_dump(char* buf, char** start, off_t offs, int len, int dummy);
static int wandev_get_info(char* buf, char** start, off_t offs, int len, int dummy);

static int map_get_info(char* buf, char** start, off_t offs, int len, int dummy);
static int interfaces_get_info(char* buf, char** start, off_t offs, int len, int dummy);

static int get_dev_config_info(char* buf, char** start, off_t offs, int len, int dummy);
static int get_dev_status_info(char* buf, char** start, off_t offs, int len, int dummy);
#endif


/* Miscellaneous */

/*
 *	Structures for interfacing with the /proc filesystem.
 *	Router creates its own directory /proc/net/wanrouter with the folowing
 *	entries:
 *	config		device configuration
 *	status		global device statistics
 *	<device>	entry for each WAN device
 */

/*
 *	Generic /proc/net/wanrouter/<file> file and inode operations 
 */



struct proc_dir_entry *proc_router;
static struct proc_dir_entry *map_dir;

#ifdef LINUX_2_6

static int config_open(struct inode *inode, struct file *file)
{
	return single_open(file, config_get_info, WP_PDE_DATA(inode));
}

static int status_open(struct inode *inode, struct file *file)
{
	return single_open(file, status_get_info, WP_PDE_DATA(inode));
}

static struct file_operations config_fops = {
	.owner	 = THIS_MODULE,
	.open	 = config_open,
	.read	 = seq_read,
	.llseek	 = seq_lseek,
	.release = single_release,
};

static struct file_operations status_fops = {
	.owner	 = THIS_MODULE,
	.open	 = status_open,
	.read	 = seq_read,
	.llseek	 = seq_lseek,
	.release = single_release,
};

static int wandev_open(struct inode *inode, struct file *file)
{
	return single_open(file, wandev_get_info, WP_PDE_DATA(inode));
}

static struct file_operations wandev_fops = {
	.owner	 = THIS_MODULE,
	.open	 = wandev_open,
	.read	 = seq_read,
	.llseek	 = seq_lseek,
	.release = single_release,
	.WAN_IOCTL	 = wanrouter_ioctl,
};

static int wp_hwprobe_open(struct inode *inode, struct file *file)
{
 	return single_open(file, probe_get_info, WP_PDE_DATA(inode));
}

static struct file_operations wp_hwprobe_fops = {
	.owner	 = THIS_MODULE,
	.open	 = wp_hwprobe_open,
	.read	 = seq_read,
	.llseek	 = seq_lseek,
	.release = single_release,
};

static int wp_hwprobe_legacy_open(struct inode *inode, struct file *file)
{
 	return single_open(file, probe_get_info_legacy, WP_PDE_DATA(inode));
}

static struct file_operations wp_hwprobe_legacy_fops = {
	.owner	 = THIS_MODULE,
	.open	 = wp_hwprobe_legacy_open,
	.read	 = seq_read,
	.llseek	 = seq_lseek,
	.release = single_release,
};

static int wp_hwprobe_verbose_open(struct inode *inode, struct file *file)
{
 	return single_open(file, probe_get_info_verbose, WP_PDE_DATA(inode));
}



static struct file_operations wp_hwprobe_verbose_fops = {
	.owner	 = THIS_MODULE,
	.open	 = wp_hwprobe_verbose_open,
	.read	 = seq_read,
	.llseek	 = seq_lseek,
	.release = single_release,
};

static int wp_hwprobe_dump_open(struct inode *inode, struct file *file)
{
 	return single_open(file, probe_get_info_dump, WP_PDE_DATA(inode));
}

static struct file_operations wp_hwprobe_dump_fops = {
	.owner	 = THIS_MODULE,
	.open	 = wp_hwprobe_dump_open,
	.read	 = seq_read,
	.llseek	 = seq_lseek,
	.release = single_release,
};

static int wp_map_open(struct inode *inode, struct file *file)
{
 	return single_open(file, map_get_info, WP_PDE_DATA(inode));
}

static struct file_operations wp_map_fops = {
	.owner	 = THIS_MODULE,
	.open	 = wp_map_open,
	.read	 = seq_read,
	.llseek	 = seq_lseek,
	.release = single_release,
};

static int wp_iface_open(struct inode *inode, struct file *file)
{
 	return single_open(file, interfaces_get_info, WP_PDE_DATA(inode));
}

static struct file_operations wp_iface_fops = {
	.owner	 = THIS_MODULE,
	.open	 = wp_iface_open,
	.read	 = seq_read,
	.llseek	 = seq_lseek,
	.release = single_release,
};

static int wandev_mapdir_open(struct inode *inode, struct file *file)
{
 	return single_open(file, wandev_mapdir_get_info, WP_PDE_DATA(inode));
}

static struct file_operations wandev_mapdir_fops = {
	.owner	 = THIS_MODULE,
	.open	 = wandev_mapdir_open,
	.read	 = seq_read,
	.llseek	 = seq_lseek,
	.release = single_release,
};

static int  wp_get_dev_config_open(struct inode *inode, struct file *file)
{
 	return single_open(file, get_dev_config_info, WP_PDE_DATA(inode));
}

static struct file_operations wp_get_dev_config_fops = {
	.owner	 = THIS_MODULE,
	.open	 =  wp_get_dev_config_open,
	.read	 = seq_read,
	.llseek	 = seq_lseek,
	.release = single_release,
};

static int wp_get_dev_status_open(struct inode *inode, struct file *file)
{
 	return single_open(file, get_dev_status_info, WP_PDE_DATA(inode));
}

static struct file_operations wp_get_dev_status_fops = {
	.owner	 = THIS_MODULE,
	.open	 =  wp_get_dev_status_open,
	.read	 = seq_read,
	.llseek	 = seq_lseek,
	.release = single_release,
};


#if 0
static int  wp_prot_dev_config_open(struct inode *inode, struct file *file)
{
	return 0;
 	//return single_open(file, get_dev_status_info, WP_PDE_DATA(inode));
}

static struct file_operations wp_prot_dev_config_fops = {
	.owner	 = THIS_MODULE,
	.open	 =  wp_prot_dev_config_open,
	.read	 = seq_read,
	.llseek	 = seq_lseek,
	.release = single_release,
};
#endif

#elif defined(LINUX_2_4)

static struct file_operations router_fops =
{
	read:		router_proc_read,
	write:		router_proc_write,
};

static struct inode_operations router_inode =
{
	permission:	router_proc_perms,
};

/*
 *	/proc/net/wanrouter/<device> file operations
 */

static struct file_operations wandev_fops =
{
	read:		router_proc_read,
	WAN_IOCTL:		wanrouter_ioctl,
};

/*
 *	/proc/net/wanrouter 
 */


#else

static struct file_operations router_fops =
{
	NULL,			/* lseek   */
	router_proc_read,	/* read	   */
	router_proc_write,	/* write   */
	NULL,			/* readdir */
	NULL,			/* select  */
	NULL,			/* ioctl   */
	NULL,			/* mmap	   */
	NULL,			/* no special open code	   */
	NULL,			/* flush */
	NULL,			/* no special release code */
	NULL			/* can't fsync */
};

static struct inode_operations router_inode =
{
	&router_fops,
	NULL,			/* create */
	NULL,			/* lookup */
	NULL,			/* link */
	NULL,			/* unlink */
	NULL,			/* symlink */
	NULL,			/* mkdir */
	NULL,			/* rmdir */
	NULL,			/* mknod */
	NULL,			/* rename */
	NULL,			/* follow link */
	NULL,			/* readlink */
	NULL,			/* readpage */
	NULL,			/* writepage */
	NULL,			/* bmap */
	NULL,			/* truncate */
	router_proc_perms
};


static struct file_operations wandev_fops =
{
	NULL,			/* lseek   */
	router_proc_read,	/* read	   */
	NULL,			/* write   */
	NULL,			/* readdir */
	NULL,			/* select  */
	wanrouter_ioctl,	/* ioctl   */
	NULL,			/* mmap	   */
	NULL,			/* no special open code	   */
	NULL,			/* flush */
	NULL,			/* no special release code */
	NULL			/* can't fsync */
};

static struct inode_operations wandev_inode =
{
	&wandev_fops,
	NULL,			/* create */
	NULL,			/* lookup */
	NULL,			/* link */
	NULL,			/* unlink */
	NULL,			/* symlink */
	NULL,			/* mkdir */
	NULL,			/* rmdir */
	NULL,			/* mknod */
	NULL,			/* rename */
	NULL,			/* readlink */
	NULL,			/* follow_link */
	NULL,			/* readpage */
	NULL,			/* writepage */
	NULL,			/* bmap */
	NULL,			/* truncate */
	router_proc_perms
};

#endif


/*
 *	/proc/net/wanrouter/<protocol>
 */
wan_proc_entry_t proc_router_fr;
wan_proc_entry_t proc_router_chdlc;
wan_proc_entry_t proc_router_ppp;
wan_proc_entry_t proc_router_x25;

#endif /* __LINUX__ */

/*
 *	Interface functions
 */

/*
 *	Prepare data for reading 'Config' entry.
 *	Return length of data.
 */
#if defined(__NetBSD__) || defined(__FreeBSD__) || defined(__OpenBSD__) || defined(LINUX_2_4)
STATIC int config_get_info(char* buf, char** start, off_t offs, int len)
#else
#if defined(LINUX_2_6)
static int config_get_info(struct seq_file *m, void *v)
#elif defined(LINUX_2_4)
static int config_get_info(char* buf, char** start, off_t offs, int len)
#else
static int config_get_info(char* buf, char** start, off_t offs, int len, int dummy)
#endif
#endif
{
	wan_device_t* wandev = NULL;
	wan_smp_flag_t flags;
	PROC_ADD_DECL(m);
	PROC_ADD_INIT(m, buf, offs, len);

	PROC_ADD_LINE(m, "%s", conf_hdr);

	wan_spin_lock(&wan_devlist_lock,&flags);  
	WAN_LIST_FOREACH(wandev, &wan_devlist, next){
		/*for (wandev = router_devlist; wandev; wandev = wandev->next){*/
		sdla_t*	card = (sdla_t*)wandev->priv;
		u16 arg = 0;

		if (!wandev->state) continue;
		
		if (card && card->hw_iface.getcfg){
			card->hw_iface.getcfg(card->hw,
					(wandev->card_type==WANOPT_S50X) ? 
						SDLA_IOPORT : SDLA_SLOT,
					&arg);
		}
		if (wandev->state){ 
			PROC_ADD_LINE(m ,
			       (wandev->card_type==WANOPT_S50X)?CONF_ISA_FRM:CONF_PCI_FRM,
				wandev->name,
				"N/A",	/* FIXME */
				SDLA_DECODE_CARDTYPE(wandev->card_type),
				wandev->irq,
				arg,
				wandev->ndev,
				(wandev->card_type==WANOPT_S51X || wandev->card_type==WANOPT_AFT_SERIAL) ? CLK_DECODE(wandev->clocking) : "N/A",
				wandev->bps);
		}
	}
	wan_spin_unlock(&wan_devlist_lock,&flags);  

	PROC_ADD_RET(m);
}


/*
 *	Prepare data for reading 'Status' entry.
 *	Return length of data.
 */

#if defined(__NetBSD__) || defined(__FreeBSD__) || defined(__OpenBSD__) || defined(LINUX_2_4)
STATIC int status_get_info(char* buf, char** start, off_t offs, int len)
#else
#if defined(LINUX_2_6)
static int status_get_info(struct seq_file *m, void *v)
#elif defined(LINUX_2_4)
static int status_get_info(char* buf, char** start, off_t offs, int len)
#else
static int status_get_info(char* buf, char** start, off_t offs, int len, int dummy)
#endif
#endif
{
	struct wan_dev_le	*devle;
	wan_device_t*	wandev = NULL;
	wan_smp_flag_t flags;

#ifdef CONFIG_PRODUCT_WANPIPE_ANNEXG
	netdevice_t *dev;
	wp_stack_stats_t wp_stats;
#endif
	PROC_ADD_DECL(m);
	PROC_ADD_INIT(m, buf, offs, len);

	PROC_ADD_LINE(m, "%s",  stat_hdr);

	devle=NULL;

	wan_spin_lock(&wan_devlist_lock,&flags);  
	WAN_LIST_FOREACH(wandev, &wan_devlist, next){
		
		if (!wandev->state) continue;

#ifdef CONFIG_PRODUCT_WANPIPE_ANNEXG
		memset(&wp_stats,0,sizeof(wp_stack_stats_t));

		if (wandev->get_active_inactive){
			wan_spin_lock(&wandev->dev_head_lock,&flags);
			WAN_LIST_FOREACH(devle, &wandev->dev_head, dev_link){
				dev = WAN_DEVLE2DEV(devle);
				if (!dev || !(dev->flags&IFF_UP) || !wan_netif_priv(dev)){ 
			   	     continue;
				}  
				wandev->get_active_inactive(wandev,dev,&wp_stats);
			}
			wan_spin_unlock(&wandev->dev_head_lock,&flags);
		}
	
		PROC_ADD_LINE(m, 
			      
			      STAT_FRM,
			      wandev->name,
			      PROT_DECODE(wandev->config_id,1),
			      wandev->config_id == WANCONFIG_FR ?  
			      		FR_STATION_DECODE(wandev->station) :
			       wandev->config_id == WANCONFIG_MFR ?  
			       		FR_STATION_DECODE(wandev->station) :
			      wandev->config_id == WANCONFIG_ADSL ? 
			      		ADSL_STATION_DECODE(wandev->station) :
			      wandev->config_id == WANCONFIG_X25 ? 
			      		X25_STATION_DECODE(wandev->station) :
			      wandev->config_id == WANCONFIG_AFT ? 
			      		"A1/2 TE1":
			       wandev->config_id == WANCONFIG_AFT_TE1 ?	
			       		"A104 TE1":
				wandev->config_id == WANCONFIG_AFT_TE3 ?
					"A300 TE3" :
				wandev->config_id == WANCONFIG_AFT_ANALOG ?
					"A200 RM" :
					("N/A"),
			       STATE_DECODE(wandev->state),
			       wp_stats.fr_active,wp_stats.fr_inactive,
			       wp_stats.lapb_active,wp_stats.lapb_inactive,
			       wp_stats.x25_link_active,wp_stats.x25_link_inactive,
			       wp_stats.x25_active,wp_stats.x25_inactive,
			       wp_stats.dsp_active,wp_stats.dsp_inactive);

#else		
		PROC_ADD_LINE(m, 
			      
			      STAT_FRM,
			      wandev->name,
			      PROT_DECODE(wandev->config_id,1),
			      wandev->config_id == WANCONFIG_FR ?
			      		FR_STATION_DECODE(wandev->station) :
			      wandev->config_id == WANCONFIG_MFR ?
			      		FR_STATION_DECODE(wandev->station) :
			      wandev->config_id == WANCONFIG_ADSL ?
			      		ADSL_STATION_DECODE(wandev->station) :
			      wandev->config_id == WANCONFIG_X25 ?
			      		X25_STATION_DECODE(wandev->station) :
					("N/A"),
				STATE_DECODE(wandev->state));
#endif
	}
	wan_spin_unlock(&wan_devlist_lock,&flags);  
	
	PROC_ADD_RET(m);
}


/*
 *	Prepare data for reading 'Interfaces' entry.
 *	Return length of data.
 */

#if defined(__NetBSD__) || defined(__FreeBSD__) || defined(__OpenBSD__) || defined(LINUX_2_4)
STATIC int interfaces_get_info(char* buf, char** start, off_t offs, int len)
#else
#if defined(LINUX_2_6)
static int interfaces_get_info(struct seq_file *m, void *v)
#elif defined(LINUX_2_4)
static int interfaces_get_info(char* buf, char** start, off_t offs, int len)
#else
static int interfaces_get_info(char* buf, char** start, off_t offs, int len, int dummy)
#endif
#endif
{
	struct wan_dev_le	*devle;
	wan_device_t*	wandev = NULL;
	netdevice_t*	dev=NULL;
	wan_smp_flag_t flags;
	PROC_ADD_DECL(m);
	PROC_ADD_INIT(m, buf, offs, len);

	PROC_ADD_LINE(m, "%s", interfaces_hdr);

	wan_spin_lock(&wan_devlist_lock,&flags);  
	WAN_LIST_FOREACH(wandev, &wan_devlist, next){
		wanpipe_common_t *dev_priv;
		if (!(m->count < (m->size - 80))) break;
		if (!wandev->state) continue;

		wan_spin_lock(&wandev->dev_head_lock,&flags);
		WAN_LIST_FOREACH(devle, &wandev->dev_head, dev_link){
			dev = WAN_DEVLE2DEV(devle);
			
			if (!dev || !WAN_NETIF_UP(dev) || !wan_netif_priv(dev)){ 
				continue;
			}

			dev_priv = wan_netif_priv(dev);
			PROC_ADD_LINE(m, 
				INTERFACES_FRM,
				wan_netif_name(dev), wandev->name, 
				dev_priv->lcn,
				dev_priv?STATE_DECODE(dev_priv->state):"N/A");

			if (dev_priv->usedby == STACK){
				wanpipe_lip_get_if_status(dev_priv,m);	
			}
		}
		wan_spin_unlock(&wandev->dev_head_lock,&flags);
	}
	wan_spin_unlock(&wan_devlist_lock,&flags);  

	PROC_ADD_RET(m);
}

#define SANGOMA_COUNT_MESSAGE "\nSangoma Card Count: "	
#if defined(__NetBSD__) || defined(__FreeBSD__) || defined(__OpenBSD__) || defined(LINUX_2_4)
STATIC int probe_get_info(char* buf, char** start, off_t offs, int len)
#else
#if defined(LINUX_2_6)
static int probe_get_info(struct seq_file *m, void *v)
#elif defined(LINUX_2_4)
static int probe_get_info(char* buf, char** start, off_t offs, int len)
#else
static int probe_get_info(char* buf, char** start, off_t offs, int len, int dummy)
#endif
#endif
{
	int i=0;
	sdla_hw_probe_t* hw_probe;
	sdla_hw_type_cnt_t *hw_cnt;
	PROC_ADD_DECL(m);
	PROC_ADD_INIT(m, buf, offs, len);

	PROC_ADD_LINE(m,  "-------------------------------\n");
	PROC_ADD_LINE(m,  "| Wanpipe Hardware Probe Info |\n");
	PROC_ADD_LINE(m,  "-------------------------------\n");

	hw_probe = (sdla_hw_probe_t *)sdla_get_hw_probe();	
	
	for (;
	     hw_probe;
	     hw_probe = WAN_LIST_NEXT(hw_probe, next)) {

		if (hw_probe->index) {
			if (hw_probe->index > i) {
             	i=hw_probe->index;
			}
		PROC_ADD_LINE(m, 
		       "%-2d. %s\n", hw_probe->index, hw_probe->hw_info);
		}
	}
	
	hw_probe = (sdla_hw_probe_t *)sdla_get_hw_probe();	
	for (;
	     hw_probe;
	     hw_probe = WAN_LIST_NEXT(hw_probe, next)) {

		if (!hw_probe->index) {
		i++;
		PROC_ADD_LINE(m, 
		       "%-2d. %s\n", i, hw_probe->hw_info);
		}
	}

	hw_cnt=(sdla_hw_type_cnt_t*)sdla_get_hw_adptr_cnt();	
	PROC_ADD_LINE(m, SANGOMA_COUNT_MESSAGE);
	if (hw_cnt->s508_adapters){
		PROC_ADD_LINE(m, "S508=%d ", hw_cnt->s508_adapters);
	}
	if (hw_cnt->s514x_adapters){
		PROC_ADD_LINE(m, "S514X=%d ", hw_cnt->s514x_adapters);
	}
	if (hw_cnt->s518_adapters){
		PROC_ADD_LINE(m, "S518=%d ", hw_cnt->s518_adapters);
	}
	if (hw_cnt->aft101_adapters){
		PROC_ADD_LINE(m, "A101-2=%d ", hw_cnt->aft101_adapters);
	}
	if (hw_cnt->aft104_adapters){
		PROC_ADD_LINE(m, "A104=%d ", hw_cnt->aft104_adapters);
	}
	if (hw_cnt->aft108_adapters){
		PROC_ADD_LINE(m, "A108=%d ", hw_cnt->aft108_adapters);
	}
	if (hw_cnt->aft200_adapters){
		PROC_ADD_LINE(m, "A200=%d ", hw_cnt->aft200_adapters);
	}
	if (hw_cnt->aft_56k_adapters){
		PROC_ADD_LINE(m, "A056=%d ", hw_cnt->aft_56k_adapters);
	}
	if (hw_cnt->aft_isdn_adapters){
		PROC_ADD_LINE(m, "A500=%d ", hw_cnt->aft_isdn_adapters);
	}
	if (hw_cnt->aft_b500_adapters){
		PROC_ADD_LINE(m, "B500=%d ", hw_cnt->aft_b500_adapters);
	}
	if (hw_cnt->aft_a700_adapters){
		PROC_ADD_LINE(m, "B700=%d ", hw_cnt->aft_a700_adapters);
	}
	if (hw_cnt->aft_serial_adapters){
		PROC_ADD_LINE(m, "A14x=%d ", hw_cnt->aft_serial_adapters);
	}
	if (hw_cnt->aft300_adapters){
		PROC_ADD_LINE(m, "A300=%d ", hw_cnt->aft300_adapters);
	}
	if (hw_cnt->usb_adapters){
		PROC_ADD_LINE(m, "U100=%d ", hw_cnt->usb_adapters);
	}
	if (hw_cnt->aft_a600_adapters){
		PROC_ADD_LINE(m, "B600=%d ", hw_cnt->aft_a600_adapters);
	}
	if (hw_cnt->aft_b601_adapters){
		PROC_ADD_LINE(m, "B601=%d ", hw_cnt->aft_b601_adapters);
	}
	if (hw_cnt->aft_b610_adapters){
		PROC_ADD_LINE(m, "B610=%d ", hw_cnt->aft_b610_adapters);
	}
	if (hw_cnt->aft_b800_adapters){
			PROC_ADD_LINE(m, "B800=%d ", hw_cnt->aft_b800_adapters);
	}
	if (hw_cnt->aft_w400_adapters){
			PROC_ADD_LINE(m, "W400=%d ", hw_cnt->aft_w400_adapters);
	}
	if (hw_cnt->aft116_adapters){
		PROC_ADD_LINE(m, "A116=%d ", hw_cnt->aft116_adapters);
	}
	if (hw_cnt->aft_t116_adapters){
		PROC_ADD_LINE(m, "T116=%d ", hw_cnt->aft_t116_adapters);
	}
	PROC_ADD_LINE(m, "\n");

	PROC_ADD_RET(m);
}

#if defined(__NetBSD__) || defined(__FreeBSD__) || defined(__OpenBSD__) || defined(LINUX_2_4)
STATIC int probe_get_info_legacy(char* buf, char** start, off_t offs, int len)
#else
#if defined(LINUX_2_6)
static int probe_get_info_legacy(struct seq_file *m, void *v)
#elif defined(LINUX_2_4)
static int probe_get_info_legacy(char* buf, char** start, off_t offs, int len)
#else
static int probe_get_info_legacy(char* buf, char** start, off_t offs, int len, int dummy)
#endif
#endif
{
	int i=0;
	sdla_hw_probe_t* hw_probe;
	sdla_hw_type_cnt_t *hw_cnt;
	PROC_ADD_DECL(m);
	PROC_ADD_INIT(m, buf, offs, len);

	PROC_ADD_LINE(m,  "----------------------------------------\n");
	PROC_ADD_LINE(m,  "| Wanpipe Hardware Probe Info (Legacy) |\n");
	PROC_ADD_LINE(m,  "----------------------------------------\n");

	hw_probe = (sdla_hw_probe_t *)sdla_get_hw_probe();	
	
	for (;
	     hw_probe;
	     hw_probe = WAN_LIST_NEXT(hw_probe, next)) {

		i++;
		PROC_ADD_LINE(m, 
		       "%-2d. %s\n", i, hw_probe->hw_info);
	}

	hw_cnt=(sdla_hw_type_cnt_t*)sdla_get_hw_adptr_cnt();	
	
	PROC_ADD_LINE(m,
		"\nCard Cnt: S508=%d S514X=%d S518=%d A101-2=%d A104=%d A300=%d A200=%d A108=%d A056=%d\n          A500=%d A14x=%d A600=%d B601=%d A116=%d\n",
		hw_cnt->s508_adapters,
		hw_cnt->s514x_adapters,
		hw_cnt->s518_adapters,
		hw_cnt->aft101_adapters,
		hw_cnt->aft104_adapters,
		hw_cnt->aft300_adapters,
		hw_cnt->aft200_adapters,
		hw_cnt->aft108_adapters,
		hw_cnt->aft_56k_adapters,
		hw_cnt->aft_isdn_adapters,
		hw_cnt->aft_serial_adapters,
		hw_cnt->aft_a600_adapters,
		hw_cnt->aft_b601_adapters,
		hw_cnt->aft116_adapters
		);

	PROC_ADD_RET(m);
}

#if defined(__NetBSD__) || defined(__FreeBSD__) || defined(__OpenBSD__) || defined(LINUX_2_4)
STATIC int probe_get_info_verbose(char* buf, char** start, off_t offs, int len)
#else
# if defined(LINUX_2_6)
static int probe_get_info_verbose(struct seq_file *m, void *v)
# elif defined(LINUX_2_4)
static int probe_get_info_verbose(char* buf, char** start, off_t offs, int len)
# else
static int probe_get_info_verbose(char* buf, char** start, off_t offs, int len, int dummy)
# endif
#endif
{
	int i=0;
	sdla_hw_probe_t* hw_probe;
	sdla_hw_type_cnt_t *hw_cnt;
	PROC_ADD_DECL(m);
	PROC_ADD_INIT(m, buf, offs, len);

	PROC_ADD_LINE(m,  "-----------------------------------------\n");
	PROC_ADD_LINE(m,  "| Wanpipe Hardware Probe Info (verbose) |\n");
	PROC_ADD_LINE(m,  "-----------------------------------------\n");

	hw_probe = (sdla_hw_probe_t *)sdla_get_hw_probe();	
	
	for (;
	     hw_probe;
	     hw_probe = WAN_LIST_NEXT(hw_probe, next)) {

		if (hw_probe->index) {
			if (hw_probe->index > i) {
             	i=hw_probe->index;
			}
			PROC_ADD_LINE(m, 
				   "%-2d. %s", hw_probe->index, hw_probe->hw_info);
			PROC_ADD_LINE(m, 
				   "%s\n", hw_probe->hw_info_verbose); 
		}
	}
	
	hw_probe = (sdla_hw_probe_t *)sdla_get_hw_probe();	
	
	for (;
	     hw_probe;
	     hw_probe = WAN_LIST_NEXT(hw_probe, next)) {

		if (!hw_probe->index) {
			i++;

			PROC_ADD_LINE(m, 
		       "%-2d. %s", i, hw_probe->hw_info);
			PROC_ADD_LINE(m, 
		       "%s\n", hw_probe->hw_info_verbose); 
		}
	}
	
	hw_cnt=(sdla_hw_type_cnt_t*)sdla_get_hw_adptr_cnt();	
	
	PROC_ADD_LINE(m, SANGOMA_COUNT_MESSAGE);
	if (hw_cnt->s508_adapters){
		PROC_ADD_LINE(m, "S508=%d ", hw_cnt->s508_adapters);
	}
	if (hw_cnt->s514x_adapters){
		PROC_ADD_LINE(m, "S514X=%d ", hw_cnt->s514x_adapters);
	}
	if (hw_cnt->s518_adapters){
		PROC_ADD_LINE(m, "S518=%d ", hw_cnt->s518_adapters);
	}
	if (hw_cnt->aft101_adapters){
		PROC_ADD_LINE(m, "A101-2=%d ", hw_cnt->aft101_adapters);
	}
	if (hw_cnt->aft104_adapters){
		PROC_ADD_LINE(m, "A104=%d ", hw_cnt->aft104_adapters);
	}
	if (hw_cnt->aft108_adapters){
		PROC_ADD_LINE(m, "A108=%d ", hw_cnt->aft108_adapters);
	}
	if (hw_cnt->aft200_adapters){
		PROC_ADD_LINE(m, "A200=%d ", hw_cnt->aft200_adapters);
	}
	if (hw_cnt->aft_56k_adapters){
		PROC_ADD_LINE(m, "A056=%d ", hw_cnt->aft_56k_adapters);
	}
	if (hw_cnt->aft_isdn_adapters){
		PROC_ADD_LINE(m, "A500=%d ", hw_cnt->aft_isdn_adapters);
	}
	if (hw_cnt->aft_a700_adapters){
		PROC_ADD_LINE(m, "B700=%d ", hw_cnt->aft_a700_adapters);
	}
	if (hw_cnt->aft_serial_adapters){
		PROC_ADD_LINE(m, "A14x=%d ", hw_cnt->aft_serial_adapters);
	}
	if (hw_cnt->aft300_adapters){
		PROC_ADD_LINE(m, "A300=%d ", hw_cnt->aft300_adapters);
	}
	if (hw_cnt->usb_adapters){
		PROC_ADD_LINE(m, "U100=%d ", hw_cnt->usb_adapters);
	}
	if (hw_cnt->aft_a600_adapters){
		PROC_ADD_LINE(m, "B600=%d ", hw_cnt->aft_a600_adapters);
	}
	if (hw_cnt->aft_b601_adapters){
		PROC_ADD_LINE(m, "B601=%d ", hw_cnt->aft_b601_adapters);
	}
	if (hw_cnt->aft_b800_adapters){
			PROC_ADD_LINE(m, "B800=%d ", hw_cnt->aft_b800_adapters);
	}
	if (hw_cnt->aft_w400_adapters){
			PROC_ADD_LINE(m, "W400=%d ", hw_cnt->aft_w400_adapters);
	}
	if (hw_cnt->aft116_adapters){
		PROC_ADD_LINE(m, "A116=%d ", hw_cnt->aft116_adapters);
	}
	if (hw_cnt->aft_t116_adapters){
		PROC_ADD_LINE(m, "T116=%d ", hw_cnt->aft_t116_adapters);
	}
	PROC_ADD_LINE(m, "\n");

	PROC_ADD_RET(m);
}


#if defined(__NetBSD__) || defined(__FreeBSD__) || defined(__OpenBSD__) || defined(LINUX_2_4)
STATIC int probe_get_info_dump(char* buf, char** start, off_t offs, int len)
#else
# if defined(LINUX_2_6)
static int probe_get_info_dump(struct seq_file *m, void *v)
# elif defined(LINUX_2_4)
static int probe_get_info_dump(char* buf, char** start, off_t offs, int len)
# else
static int probe_get_info_dump(char* buf, char** start, off_t offs, int len, int dummy)
# endif
#endif
{
	int i=0;
	sdla_hw_probe_t* hw_probe;
	sdla_hw_type_cnt_t *hw_cnt;
	PROC_ADD_DECL(m);
	PROC_ADD_INIT(m, buf, offs, len);

	hw_probe = (sdla_hw_probe_t *)sdla_get_hw_probe();	
	
	for (;
	     hw_probe;
	     hw_probe = WAN_LIST_NEXT(hw_probe, next)) {

		if (hw_probe->index) {
			if (hw_probe->index > i) {
             	i=hw_probe->index;
			}
		PROC_ADD_LINE(m, 
		       "|%d%s\n", hw_probe->index, hw_probe->hw_info_dump);
		}
	}
	hw_probe = (sdla_hw_probe_t *)sdla_get_hw_probe();	
	for (;
	     hw_probe;
	     hw_probe = WAN_LIST_NEXT(hw_probe, next)) {

		if (!hw_probe->index) {
		i++;
		PROC_ADD_LINE(m, 
		       "|%d%s\n", i, hw_probe->hw_info_dump);
		}
	}

	hw_cnt=(sdla_hw_type_cnt_t*)sdla_get_hw_adptr_cnt();	
	
	PROC_ADD_LINE(m,
		"|Card Cnt|S508=%d|S514X=%d|S518=%d|A101-2=%d|A104=%d|A300=%d|A200=%d|A108=%d|A056=%d|A500=%d|B700=%d|B600=%d|B601=%d|A14x=%d|A116=%d|T116=%d|W400=%d|\n",
		hw_cnt->s508_adapters,
		hw_cnt->s514x_adapters,
		hw_cnt->s518_adapters,
		hw_cnt->aft101_adapters,
		hw_cnt->aft104_adapters,
		hw_cnt->aft300_adapters,
		hw_cnt->aft200_adapters,
		hw_cnt->aft108_adapters,
		hw_cnt->aft_56k_adapters,
		hw_cnt->aft_isdn_adapters,
		hw_cnt->aft_a700_adapters,
		hw_cnt->aft_a600_adapters,
		hw_cnt->aft_b601_adapters,
		hw_cnt->aft_serial_adapters,
		hw_cnt->aft116_adapters,
		hw_cnt->aft_t116_adapters,
		hw_cnt->aft_w400_adapters);

	PROC_ADD_RET(m);
}


	
/*
 *	Prepare data for reading <device> entry.
 *	Return length of data.
 *
 *	On entry, the 'start' argument will contain a pointer to WAN device
 *	data space.
 */

#if defined(__NetBSD__) || defined(__FreeBSD__) || defined(__OpenBSD__) || defined(LINUX_2_4)
STATIC int wandev_get_info(char* buf, char** start, off_t offs, int len)
#else
#if defined(LINUX_2_6)
static int wandev_get_info(struct seq_file *m, void *v)
#elif defined(LINUX_2_4)
static int wandev_get_info(char* buf, char** start, off_t offs, int len)
#else
static int wandev_get_info(char* buf, char** start, off_t offs, int len, int dummy)
#endif
#endif
{
	wan_device_t* wandev = PROC_GET_DATA();
	int rslt = 0;
	PROC_ADD_DECL(m);

	if ((wandev == NULL) || (wandev->magic != ROUTER_MAGIC))
		return 0;
	
	PROC_ADD_INIT(m, buf, offs, len);
	if (!wandev->state){
		PROC_ADD_LINE(m,
			"device is not configured!\n");
		goto wandev_get_info_end;
	}

	/* Update device statistics */
	if (wandev->update && !m->from){

		rslt = wandev->update(wandev);
		if(rslt) {
			switch (rslt) {
			case -EAGAIN:
				PROC_ADD_LINE(m, 
					
					"Device is busy!\n");
				break;
					

			default:
				PROC_ADD_LINE(m,
					
					"Device is not configured!\n");
				break;
			}
			goto wandev_get_info_end;
		}
	}

	PROC_ADD_LINE(m, 
		
		PROC_STATS_2_FORMAT,
		"total rx packets", wandev->stats.rx_packets,
		"total tx packets", wandev->stats.tx_packets);
	PROC_ADD_LINE(m,
		
		PROC_STATS_2_FORMAT,
		"total rx bytes", wandev->stats.rx_bytes, 
		"total tx bytes", wandev->stats.tx_bytes);
	PROC_ADD_LINE(m,
		
		PROC_STATS_2_FORMAT,
		"bad rx packets", wandev->stats.rx_errors, 
		"packet tx problems", wandev->stats.tx_errors);
	PROC_ADD_LINE(m,
		 
		PROC_STATS_2_FORMAT,
		"rx frames dropped", wandev->stats.rx_dropped,
		"tx frames dropped", wandev->stats.tx_dropped);
	PROC_ADD_LINE(m,
		
		PROC_STATS_2_FORMAT,
		"multicast rx packets", wandev->stats.multicast,
		"tx collisions", wandev->stats.collisions);
	PROC_ADD_LINE(m,
		
		PROC_STATS_2_FORMAT,
		"rx length errors", wandev->stats.rx_length_errors,
		"rx overrun errors", wandev->stats.rx_over_errors);
	PROC_ADD_LINE(m,
		
		PROC_STATS_2_FORMAT,
		"CRC errors", wandev->stats.rx_crc_errors,
		"abort frames", wandev->stats.rx_frame_errors);
	PROC_ADD_LINE(m,
		
		PROC_STATS_2_FORMAT,
		"rx fifo overrun", wandev->stats.rx_fifo_errors,
		"rx missed packet", wandev->stats.rx_missed_errors);
	PROC_ADD_LINE(m,
		
		PROC_STATS_1_FORMAT,
		"aborted tx frames", wandev->stats.tx_aborted_errors);

	/* Update Front-End information (alarms, performance monitor counters */
	if (wandev->get_info){
		m->count = wandev->get_info(
				wandev->priv, 
				m, 
				M_STOP_CNT(m)); 
	}

wandev_get_info_end:
	PROC_ADD_RET(m);
}



#if defined(__LINUX__)

#ifdef LINUX_2_6
#define FPTR(st) (&st)
#else
#define FPTR(st) NULL
#endif

static inline struct proc_dir_entry *wp_proc_create(const char *name,
		umode_t mode,
		struct proc_dir_entry *parent,
		struct file_operations *proc_fops)
{
	struct proc_dir_entry *p = NULL;
#ifdef LINUX_3_0
	p = proc_create(name, mode, parent, proc_fops);
#elif defined(LINUX_2_6)
	p = create_proc_entry(name, mode, parent);
	if (!p) {
		return NULL;
	}
	p->proc_fops = proc_fops;
#elif defined(LINUX_2_4)
	p = create_proc_entry(name, mode, parent);
	if (!p) {
		return NULL;
	}
	/* In 2.4 we always default to router_fops and router_inode */
	if (!proc_fops) {
		p->proc_fops = &router_fops;
	}
	p->proc_iops = &router_inode;
#else
	p = create_proc_entry(name, mode, parent);
	if (!p) {
		return NULL;
	}
	/* In Pre-2.4 we always default to router_inode */
	p->ops = &router_inode;
	p->nlink = 1;
#endif
	return p;
}

static inline struct proc_dir_entry *wp_proc_create_data(const char *name,
		umode_t mode,
		struct proc_dir_entry *parent,
		struct file_operations *proc_fops, void *data)
{
	struct proc_dir_entry *p = NULL;
#ifdef LINUX_3_0
	p = proc_create_data(name, mode, parent, proc_fops, data);
#else
	p = wp_proc_create(name, mode, parent, proc_fops);
	if (p) {
		p->data = data;
	}
#endif
	return p;
}

/*
 *	Initialize router proc interface.
 */

int wanrouter_proc_init (void)
{
	struct proc_dir_entry *p;
	proc_router = proc_mkdir(ROUTER_NAME, wan_init_net(proc_net));
	if (!proc_router)
		goto fail;
	
	p = wp_proc_create("config",S_IRUGO,proc_router,FPTR(config_fops));
	if (!p)
		goto fail_config;
#ifndef LINUX_2_6
	p->get_info = config_get_info;
#endif
	
	p = wp_proc_create("status",S_IRUGO,proc_router,FPTR(status_fops));
	if (!p)
		goto fail_stat;
#ifndef LINUX_2_6
	p->get_info = status_get_info;
#endif
	
	p = wp_proc_create("hwprobe",0,proc_router,FPTR(wp_hwprobe_fops));
	if (!p)
		goto fail_probe;
#ifndef LINUX_2_6
	p->get_info = probe_get_info;
#endif

	p = wp_proc_create("hwprobe_legacy",0,proc_router,FPTR(wp_hwprobe_legacy_fops));
	if (!p)
		goto fail_probe_legacy;
#ifndef LINUX_2_6
	p->get_info = probe_get_info_legacy;
#endif

	p = wp_proc_create("hwprobe_verbose",0,proc_router,FPTR(wp_hwprobe_verbose_fops));
	if (!p)
		goto fail_probe_verbose;
#ifndef LINUX_2_6
	p->get_info = probe_get_info_verbose;
#endif

	p = wp_proc_create("hwprobe_dump",0,proc_router,FPTR(wp_hwprobe_dump_fops));
	if (!p)
		goto fail_probe_dump;
#ifndef LINUX_2_6
	p->get_info = probe_get_info_dump;
#endif

	p = wp_proc_create("map",0,proc_router,FPTR(wp_map_fops));
	if (!p)
		goto fail_map;
#ifndef LINUX_2_6
	p->get_info = map_get_info;
#endif

	p = wp_proc_create("interfaces",0,proc_router,FPTR(wp_iface_fops));
	if (!p)
		goto fail_interfaces;
#ifndef LINUX_2_6
	p->get_info = interfaces_get_info;
#endif

	map_dir = proc_mkdir("dev_map", proc_router);
	if (!map_dir)
		goto fail_dev_map;

	/* Initialize protocol proc fs. */
	proc_router_chdlc.count = 0;
	proc_router_chdlc.protocol_entry = NULL;
	proc_router_fr.count = 0;
	proc_router_fr.protocol_entry = NULL;
	proc_router_ppp.count = 0;
	proc_router_ppp.protocol_entry = NULL;
	proc_router_x25.count = 0;
	proc_router_x25.protocol_entry = NULL;
	return 0;

fail_dev_map:
	remove_proc_entry("interfaces", proc_router);	
fail_interfaces:	
	remove_proc_entry("map", proc_router);
fail_map:
	remove_proc_entry("hwprobe_dump", proc_router);
fail_probe_dump:
	remove_proc_entry("hwprobe_verbose", proc_router);
fail_probe_verbose:	
	remove_proc_entry("hwprobe_legacy", proc_router);
fail_probe_legacy:
	remove_proc_entry("hwprobe", proc_router);
fail_probe:
	remove_proc_entry("status", proc_router);
fail_stat:
	remove_proc_entry("config", proc_router);
fail_config:
	remove_proc_entry(ROUTER_NAME, wan_init_net(proc_net));
fail:
	return -ENOMEM;
}

/*
 *	Clean up router proc interface.
 */
void wanrouter_proc_cleanup (void)
{
	remove_proc_entry("config", proc_router);
	remove_proc_entry("status", proc_router);
	remove_proc_entry("hwprobe", proc_router);
	remove_proc_entry("hwprobe_legacy", proc_router);
	remove_proc_entry("hwprobe_verbose", proc_router);
	remove_proc_entry("hwprobe_dump", proc_router);
	remove_proc_entry("map", proc_router);
	remove_proc_entry("interfaces", proc_router);
	remove_proc_entry("dev_map",proc_router);
	remove_proc_entry(ROUTER_NAME,wan_init_net(proc_net));
}

/*
 *	Add directory entry for WAN device.
 */

int wanrouter_proc_add (wan_device_t* wandev)
{	
	int err=0;
	struct proc_dir_entry *p;	

	wan_spin_lock_init(&wandev->get_map_lock, "wan_proc_lock");
	
	if (wandev->magic != ROUTER_MAGIC)
		return -EINVAL;


	wandev->dent = wp_proc_create_data(wandev->name, S_IRUGO, proc_router, FPTR(wandev_fops), wandev);
	if (!wandev->dent)
		return -ENOMEM;

#if !defined(LINUX_2_6)
	wandev->dent->get_info	= wandev_get_info;
#endif

	p = wp_proc_create_data(wandev->name, 0, map_dir, FPTR(wandev_mapdir_fops), wandev);
	if (!p){
		remove_proc_entry(wandev->name, proc_router);
		wandev->dent=NULL;
		return -ENOMEM;
	}

#if !defined(LINUX_2_6)
#if defined(LINUX_2_4)
	p->proc_fops	= &wandev_fops;
	p->proc_iops	= &router_inode;
	p->get_info	= wandev_mapdir_get_info;
#else
	p->ops = &wandev_inode;
	p->get_info	= wandev_mapdir_get_info;
#endif
#endif
	return err;
}

/*
 *	Delete directory entry for WAN device.
 */
 
int wanrouter_proc_delete(wan_device_t* wandev)
{
	if (wandev->magic != ROUTER_MAGIC)
		return -EINVAL;

	remove_proc_entry(wandev->name, proc_router);
	remove_proc_entry(wandev->name, map_dir);
	return 0;
}
/*
 *	Add directory entry for Protocol.
 */

int wanrouter_proc_add_protocol(wan_device_t* wandev)
{
	struct proc_dir_entry*	p = NULL;
	//struct proc_dir_entry**	proc_protocol;
	wan_proc_entry_t*	proc_protocol;

	if (wandev->magic != ROUTER_MAGIC)
		return -EINVAL;

	switch(wandev->config_id){
		case WANCONFIG_FR:
		case WANCONFIG_MFR:
			proc_protocol = &proc_router_fr;
			break;
			
		case WANCONFIG_CHDLC:
			proc_protocol = &proc_router_chdlc;
			break;
			
		case WANCONFIG_PPP:
			proc_protocol = &proc_router_ppp;
			break;
			
		case WANCONFIG_X25:
			proc_protocol = &proc_router_x25;
			break;
			
		default:
			proc_protocol = NULL;
			return 0;
	}

	if (proc_protocol->protocol_entry == NULL){

		proc_protocol->count=0;
		
		/* Create /proc/net/wanrouter/<protocol> directory */
		proc_protocol->protocol_entry = 
			proc_mkdir(PROT_DECODE(wandev->config_id,0), proc_router);
		
		if (!proc_protocol->protocol_entry)
			goto fail;
	
		/* Create /proc/net/wanrouter/<protocol>/config directory */
		p = wp_proc_create_data("config",0,proc_protocol->protocol_entry,FPTR(wp_get_dev_config_fops),((void *)(long)wandev->config_id));
		if (!p)
			goto fail_config;
#if !defined(LINUX_2_6)
		p->get_info 	= get_dev_config_info;
#endif
	
		/* Create /proc/net/wanrouter/<protocol>/status directory */
		p = wp_proc_create_data("status",0,proc_protocol->protocol_entry,FPTR(wp_get_dev_status_fops),((void *)(long)wandev->config_id));
		if (!p)
			goto fail_stat;
#if !defined(LINUX_2_6)
		p->get_info 	= get_dev_status_info;
#endif
	}	
	
	/* Create /proc/net/wanrouter/<protocol>/<wanpipe#> directory */
	wandev->link = proc_mkdir(wandev->name, proc_protocol->protocol_entry);
	if (!wandev->link)
		goto fail_link;

	/* Create /proc/net/wanrouter/<protocol>/<wanpipe#>config directory */
	p = wp_proc_create_data("config",0,wandev->link,NULL,wandev);
	if (!p)
		goto fail_link_config;
#if !defined(LINUX_2_6)
	p->get_info 	= wandev->get_dev_config_info;
	p->write_proc 	= wandev->set_dev_config;
#endif

	proc_protocol->count ++;
	return 0;

fail_link_config:
	remove_proc_entry(wandev->name, proc_protocol->protocol_entry);
fail_link:
	if (proc_protocol->count){
		/* Do not remove /proc/net/wanrouter/<protocol>... because 
		 * another device is still using this entry.
		 */
		return -ENOMEM;
	}
	remove_proc_entry("status", proc_protocol->protocol_entry);
	return -ENOMEM;

fail_stat:
	remove_proc_entry("config", proc_protocol->protocol_entry);
fail_config:
	remove_proc_entry(PROT_DECODE(wandev->config_id,0), proc_router);
	proc_protocol->protocol_entry = NULL;
fail:
	return -ENOMEM;
}

/*
 *	Delete directory entry for Protocol.
 */
 
int wanrouter_proc_delete_protocol(wan_device_t* wandev)
{
	wan_proc_entry_t*	proc_protocol = NULL;

	if (wandev->magic != ROUTER_MAGIC)
		return -EINVAL;

	switch(wandev->config_id){
		case WANCONFIG_FR:
		case WANCONFIG_MFR:
			proc_protocol = &proc_router_fr;
			break;
			
		case WANCONFIG_CHDLC:
			proc_protocol = &proc_router_chdlc;
			break;
			
		case WANCONFIG_PPP:
			proc_protocol = &proc_router_ppp;
			break;
			
		case WANCONFIG_X25:
			proc_protocol = &proc_router_x25;
			break;
			
		default:
			proc_protocol = NULL;
			return 0;
			break;
	}

	remove_proc_entry("config", wandev->link);
	remove_proc_entry(wandev->name, proc_protocol->protocol_entry);
	proc_protocol->count --;
	if (!proc_protocol->count){
		remove_proc_entry("config", proc_protocol->protocol_entry);
		remove_proc_entry("status", proc_protocol->protocol_entry);
		remove_proc_entry(PROT_DECODE(wandev->config_id,0), proc_router);
		proc_protocol->protocol_entry = NULL;
	}
	return 0;
}

/*
 *	Add directory entry for interface.
 */

int wanrouter_proc_add_interface(wan_device_t* wandev, 
				 struct proc_dir_entry** dent,
				 char* if_name,
				 void* priv)
{

#if 0
	if (wandev->magic != ROUTER_MAGIC)
		return -EINVAL;
	if (wandev->link == NULL || wandev->get_if_info == NULL)
		return -ENODEV;

	*dent = create_proc_entry(if_name, 0, wandev->link);
	if (!*dent)
		return -ENOMEM;
#ifdef LINUX_2_4
	(*dent)->proc_fops	= &router_fops;
	(*dent)->proc_iops	= &router_inode;
#else
	(*dent)->ops = &router_inode;
	(*dent)->nlink = 1;
#endif
	(*dent)->get_info	= wandev->get_if_info;
	(*dent)->write_proc	= wandev->set_if_info;
	(*dent)->data		= priv;
#endif
	return 0;
}

/*
 *	Delete directory entry for interface.
 */
 
int wanrouter_proc_delete_interface(wan_device_t* wandev, char* if_name)
{
#if 0
	if (wandev->magic != ROUTER_MAGIC)
		return -EINVAL;
	if (wandev->link == NULL)
		return -ENODEV;

	remove_proc_entry(if_name, wandev->link);
#endif
	return 0;
}

/****** Proc filesystem entry points ****************************************/

/*
 *	Verify access rights.
 */

#ifndef LINUX_2_6
static int router_proc_perms (struct inode* inode, int op)
{
	return 0;
}

/*
 *	Read router proc directory entry.
 *	This is universal routine for reading all entries in /proc/net/wanrouter
 *	directory.  Each directory entry contains a pointer to the 'method' for
 *	preparing data for that entry.
 *	o verify arguments
 *	o allocate kernel buffer
 *	o call get_info() to prepare data
 *	o copy data to user space
 *	o release kernel buffer
 *
 *	Return:	number of bytes copied to user space (0, if no data)
 *		<0	error
 */

static ssize_t router_proc_read(struct file* file, char* buf, size_t count,
				loff_t *ppos)
{
	struct inode *inode = file->f_dentry->d_inode;
	struct proc_dir_entry* dent;
	char* page;
	int len;

	if (count <= 0)
		return 0;
	
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,18)
	dent = inode->i_private;
#else
	dent = inode->u.generic_ip;
#endif
	if ((dent == NULL) || (dent->get_info == NULL))
		return 0;
		
	page = wan_malloc(count);
	if (page == NULL)
		return -ENOBUFS;
#ifdef LINUX_2_4
	len = dent->get_info(page, dent->data, file->f_pos, count);
#else
	len = dent->get_info(page, dent->data, file->f_pos, count, 0);
#endif
	if (len) {
		if(copy_to_user(buf, page, len)){
			DEBUG_SUB_MEM(count);
			wan_free(page);
			return -EFAULT;
		}
		file->f_pos += len;
	}
	wan_free(page);
	return len;
}

/*
 *	Write router proc directory entry.
 *	This is universal routine for writing all entries in /proc/net/wanrouter
 *	directory.  Each directory entry contains a pointer to the 'method' for
 *	preparing data for that entry.
 *	o verify arguments
 *	o allocate kernel buffer
 *	o copy data from user space
 *	o call write_info()
 *	o release kernel buffer
 *
 *	Return:	number of bytes copied to user space (0, if no data)
 *		<0	error
 */
static ssize_t router_proc_write (struct file *file, const char *buf, size_t count, 
					loff_t *ppos)
{
	struct inode*		inode = file->f_dentry->d_inode;
	struct proc_dir_entry* 	dent = NULL;
	char* 			page = NULL;
#ifdef WAN_DEBUG_MEM
	unsigned int 		ocount=count;
#endif

	if (count <= 0)
		return 0;
		
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,18)
        dent = inode->i_private;
#else
        dent = inode->u.generic_ip;
#endif

	if ((dent == NULL) || (dent->write_proc == NULL))
	return count;
		
	page = wan_malloc(count);
	if (page == NULL)
		return -ENOBUFS;
	if (copy_from_user(page, buf, count)){
		DEBUG_SUB_MEM(count);
		wan_free(page);
		return -EINVAL;
	}
	page[count-1] = '\0';
		
	/* Add supporting Write to proc fs */
	count = dent->write_proc(file, page, count, dent->data);

	DEBUG_SUB_MEM(ocount);
	wan_free(page);
	return count;
}

#endif


/*
 *	Prepare data for reading 'MAP' entry.
 *	Return length of data.
 */

#if defined(LINUX_2_6)
static int map_get_info(struct seq_file *m, void *v)
#elif defined(LINUX_2_4)
static int map_get_info(char* buf, char** start, off_t offs, int len)
#else
static int map_get_info(char* buf, char** start, off_t offs, int len, int dummy)	
#endif
{
	wan_device_t*	wandev = NULL;
	struct wan_dev_le	*devle;
	netdevice_t*	dev=NULL;
	wan_smp_flag_t flags;

	PROC_ADD_DECL(m);
	PROC_ADD_INIT(m, buf, offs, len);

	PROC_ADD_LINE(m, "%s", map_hdr);

	wan_spin_lock(&wan_devlist_lock,&flags);  
	WAN_LIST_FOREACH(wandev, &wan_devlist, next){
		
		if (!wandev->state){ 
			continue;
		}
		if (!wandev->get_map){
			continue;
		}

		wan_spin_lock(&wandev->dev_head_lock,&flags);
		WAN_LIST_FOREACH(devle, &wandev->dev_head, dev_link){
			dev = WAN_DEVLE2DEV(devle);
			if (!dev || !(dev->flags&IFF_UP) || !wan_netif_priv(dev)){ 
				continue;
			}

			m->count = wandev->get_map(wandev, dev, m, M_STOP_CNT(m));
		}
		wan_spin_unlock(&wandev->dev_head_lock,&flags);
	}
	wan_spin_unlock(&wan_devlist_lock,&flags);  

	PROC_ADD_RET(m);
}



/*
 *	Prepare data for reading FR 'Config' entry.
 *	Return length of data.
 */
#if defined(LINUX_2_6)
static int get_dev_config_info(struct seq_file *m, void *v)
#elif defined(LINUX_2_4)
static int get_dev_config_info(char* buf, char** start, off_t offs, int len)
#else
static int get_dev_config_info(char* buf, char** start, off_t offs, int len,int dummy)
#endif
{
	wan_device_t*	wandev = NULL;
	netdevice_t*	dev = NULL;
	wan_smp_flag_t flags;
	struct wan_dev_le	*devle;
	PROC_ADD_DECL(m);
	PROC_ADD_INIT(m, buf, offs, len);

	wan_spin_lock(&wan_devlist_lock,&flags);  
	WAN_LIST_FOREACH(wandev, &wan_devlist, next){
	     if (!(m->count < (PROC_BUFSZ - 80))) break;
		if (!wandev->get_config_info)
			continue;

#ifndef LINUX_2_6
		if ((wandev->config_id != (unsigned)start) || !wandev->state) 
			continue;
#else
		if (!wandev->state) 
			continue;
#endif

	       	wan_spin_lock(&wandev->dev_head_lock,&flags);
		WAN_LIST_FOREACH(devle, &wandev->dev_head, dev_link){
			dev = WAN_DEVLE2DEV(devle);
			if (!dev || !(dev->flags&IFF_UP) || !wan_netif_priv(dev)){ 
				continue;
			}
			m->count = wandev->get_config_info(wan_netif_priv(dev),
							   m, 
							   M_STOP_CNT(m)); 
		}
	       	wan_spin_unlock(&wandev->dev_head_lock,&flags);
	}
	wan_spin_unlock(&wan_devlist_lock,&flags);  

	PROC_ADD_RET(m);
}	

/*
 *	Prepare data for reading FR 'Status' entry.
 *	Return length of data.
 */

#if defined(LINUX_2_6)
static int get_dev_status_info(struct seq_file *m, void *v)
#elif defined(LINUX_2_4)
static int get_dev_status_info(char* buf, char** start, off_t offs, int len)
#else
static int get_dev_status_info(char* buf, char** start, off_t offs, int len, int dummy)	
#endif
{
	wan_device_t*	wandev = NULL;
	struct wan_dev_le	*devle;
	netdevice_t*	dev = NULL;
	int		cnt = 0;
	wan_smp_flag_t flags;
	PROC_ADD_DECL(m);
	
	PROC_ADD_INIT(m, buf, offs, len);

	wan_spin_lock(&wan_devlist_lock,&flags);  
	WAN_LIST_FOREACH(wandev, &wan_devlist, next){
	     if (!(cnt < (PROC_BUFSZ - 80))) break;

		if (!wandev->get_status_info)
			continue;
	
#ifndef LINUX_2_6
		if ((wandev->config_id != (unsigned)start) || !wandev->state) 
			continue;
#else
		if (!wandev->state) 
			continue;
#endif

	       	wan_spin_lock(&wandev->dev_head_lock,&flags);
		WAN_LIST_FOREACH(devle, &wandev->dev_head, dev_link){
			dev = WAN_DEVLE2DEV(devle);
			if (!dev || !(dev->flags&IFF_UP) || !wan_netif_priv(dev)){ 
				continue;
			}
			m->count = wandev->get_status_info(wan_netif_priv(dev), m, M_STOP_CNT(m));
		}
	       	wan_spin_unlock(&wandev->dev_head_lock,&flags);
	}
	wan_spin_unlock(&wan_devlist_lock,&flags);  
	

	PROC_ADD_RET(m);
}


#if defined(LINUX_2_6)
static int wandev_mapdir_get_info(struct seq_file *m, void *v)
#elif defined(LINUX_2_4)
static int wandev_mapdir_get_info(char* buf, char** start, off_t offs, int len)
#else
static int wandev_mapdir_get_info(char* buf, char** start, off_t offs, int len, int dummy)
#endif
{
	wan_device_t* wandev = PROC_GET_DATA();
	struct wan_dev_le	*devle;
	netdevice_t*	dev = NULL;
	wan_smp_flag_t flags;
	PROC_ADD_DECL(m);

	if (!wandev){
		return -ENODEV;
	}

	PROC_ADD_INIT(m, buf, offs, len);

	PROC_ADD_LINE(m, "%s", map_hdr);

	if (!wandev->state){ 
		goto wandev_mapdir_get_info_exit;
	}
	if (!wandev->get_map){
		goto wandev_mapdir_get_info_exit;
	}

	wan_spin_lock(&wandev->dev_head_lock,&flags);
	WAN_LIST_FOREACH(devle, &wandev->dev_head, dev_link){
		dev = WAN_DEVLE2DEV(devle);
		if (!dev || !(dev->flags&IFF_UP) || !wan_netif_priv(dev)){ 
		       	continue;
		}   
		m->count = wandev->get_map(wandev, dev, m, M_STOP_CNT(m));
	}
        wan_spin_unlock(&wandev->dev_head_lock,&flags);
	
wandev_mapdir_get_info_exit:
	PROC_ADD_RET(m);
}



#endif /* __LINUX__ */

#else

/*
 *	No /proc - output stubs
 */

int wanrouter_proc_init(void)
{
	return 0;
}

void wanrouter_proc_cleanup(void)
{
	return;
}

int wanrouter_proc_add(wan_device_t *wandev)
{
	return 0;
}

int wanrouter_proc_delete(wan_device_t *wandev)
{
	return 0;
}

int wanrouter_proc_add_protocol(wan_device_t *wandev)
{
	return 0;
}

int wanrouter_proc_delete_protocol(wan_device_t *wandev)
{
	return 0;
}

int wanrouter_proc_add_interface(wan_device_t* wandev, 
				 struct proc_dir_entry** dent,
				 char* if_name,
				 void* priv)
{
	return 0;
}

int wanrouter_proc_delete_interface(wan_device_t* wandev, char* if_name)
{
	return 0;
}

#endif

/*============================================================================
 * Write WAN device ???.
 * o Find WAN device associated with this node
 */
#ifdef LINUX_2_0
static int device_write(
        struct inode* inode, struct file* file, const char* buf, int count)
{
        int err = verify_area(VERIFY_READ, buf, count);
        struct proc_dir_entry* dent;
        wan_device_t* wandev;

        if (err) return err;

#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,18)
        dent = inode->i_private;
#else
        dent = inode->u.generic_ip;
#endif

        if ((dent == NULL) || (dent->data == NULL))
                return -ENODATA;

        wandev = dent->data;

        DEBUG_TEST("%s: writing %d bytes to %s...\n",
                name_root, count, dent->name);
        
	return 0;
}
#endif


 #if defined(__NetBSD__) || defined(__FreeBSD__) || defined(__OpenBSD__)
int proc_add_line(struct seq_file* m, char* frm, ...)
{
	char 	tmp[400];
	int 	ret = PROC_BUF_CONT;
	int 	size = 0;
	va_list	arg;
	
	va_start(arg, frm);
	if (m->count && m->stop_cnt){
		va_end(arg);
		return PROC_BUF_EXIT;
	}
	size = vsprintf(tmp, frm, arg);
	if (m->stop_cnt){
		if (m->stop_cnt < size){
			DEBUG_ERROR("!!! Error in writting in proc buffer !!!\n");
			m->stop_cnt = size;
		}
		m->stop_cnt -= size;
	}else{
		if (size < m->size - m->count){
			/*vsprintf(&m->buf[m->count], frm, arg);*/
			memcpy(&m->buf[m->count], tmp, size);
			m->count += size;
			/* *cnt += vsprintf(&buf[*cnt], frm, arg); */
		}else{
			m->stop_cnt = m->from + m->count;
			ret = PROC_BUF_EXIT;
		}
	}
	va_end(arg);
	return ret;
}
#endif


#if defined(__NetBSD__) || defined(__FreeBSD__) || defined(__OpenBSD__)
#else
int proc_add_line(struct seq_file* m, char* frm, ...)
{
#if defined(LINUX_2_6)
	return 0;
#else
	char 	tmp[400];
	int 	ret = PROC_BUF_CONT;
	int 	size = 0;
	va_list	arg;
	
	va_start(arg, frm);
	if (m->count && m->stop_cnt){
		va_end(arg);
		return PROC_BUF_EXIT;
	}
	size = vsprintf(tmp, frm, arg);
	if (m->stop_cnt){
		if (m->stop_cnt < size){
			DEBUG_ERROR("!!! Error in writting in proc buffer !!!\n");
			m->stop_cnt = size;
		}
		m->stop_cnt -= size;
	}else{
		if (size < m->size - m->count){
			/*vsprintf(&m->buf[m->count], frm, arg);*/
			memcpy(&m->buf[m->count], tmp, size);
			m->count += size;
			/* *cnt += vsprintf(&buf[*cnt], frm, arg); */
		}else{
			m->stop_cnt = m->from + m->count;
			ret = PROC_BUF_EXIT;
		}
	}
	va_end(arg);
	return ret;
#endif
}
#endif   

#endif /* !__WINDOWS__ */

/*
 *	End
 */
