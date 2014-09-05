/*
 * main2_6.c
 *
 * Derivation: Copyright (C) 2005 Xygnada Technology, Inc.
 * Copyright (C) 2001 Alessandro Rubini and Jonathan Corbet
 * Copyright (C) 2001 O'Reilly & Associates
 *
 * The source code in this file can be freely used, adapted,
 * and redistributed in source or binary form, so long as an
 * acknowledgment appears in derived source files.  The citation
 * should list that the code comes from the book "Linux Device
 * Drivers" by Alessandro Rubini and Jonathan Corbet, published
 * by O'Reilly & Associates.   No warranty is attached;
 * we cannot take responsibility for errors or fitness for use.
 *
 */

#include <linux/version.h>
# if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,20)
	# include <linux/config.h>	/* OS configuration options */
# endif
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>

#include <linux/kernel.h>	/* printk() */
#include <linux/slab.h>		/* kmalloc() */
#include <linux/fs.h>		/* everything... */
#include <linux/errno.h>	/* error codes */
#include <linux/types.h>	/* size_t */
#include <linux/proc_fs.h>
#include <linux/fcntl.h>	/* O_ACCMODE */
#include <linux/seq_file.h>
#include <linux/cdev.h>
#include <linux/net.h>

#include <asm/system.h>		/* cli(), *_flags */
#include <asm/uaccess.h>	/* copy_*_user */

#include "_xmtp2km.h"
#include "kern_if.h"
#include "fwmsg.h"

/* parameters which can be set at load time */

int xmtp2km_major =   XMTP2KM_MAJOR;
int xmtp2km_minor =   0;
int xmtp2km_nr_devs = XMTP2KM_NR_DEVS;	/* number of bare xmtp2km devices */
module_param(xmtp2km_major, int, S_IRUGO);
module_param(xmtp2km_minor, int, S_IRUGO);
module_param(xmtp2km_nr_devs, int, S_IRUGO);

MODULE_AUTHOR("Michael Mueller/Xygnada Technology, Inc.");
MODULE_LICENSE("Dual BSD/GPL");

struct xmtp2km_dev *xmtp2km_devices;	/* allocated in xmtp2km_init_module */
/* extern global vars */
extern int	module_use_count;

static spinlock_t xmtp2km_spinlock; 

/* prototypes for non-open parts of xmtp2km kernel modules */
/* prototypes for functions used by the Sangoma xmtp2 LIP device driver */
#if 0
void xmtp2km_bs_handler (int fi, int len, uint8_t * p_rxbs, uint8_t * p_txbs);
void xmtp2km_facility_state_change (int card_iface_id, int state);
int xmtp2km_unregister (int fi);
int xmtp2km_register (
	void * p_instance_data, 
	char * ps_ifname, 
	int (*sangoma_drvr_cb)(void*, unsigned char*, int));
int xmtp2km_tap_disable(int card_iface_id);
int xmtp2km_tap_enable(
		int card_iface_id, 
		void *dev, 
		int (*frame)(void *ptr, int slot, int dir, unsigned char *data, int len));
#else
	#include "xmtp2km_kiface.h"
#endif
/* prototypes for functions used by the xmtp2km kernel module ioctl function */
int xmtp2km_ioctl_binit (void);
int xmtp2km_ioctl_opsparms (unsigned int cmd, unsigned long arg);
int xmtp2km_ioctl_pwr_on (unsigned int cmd, unsigned long arg);
int xmtp2km_ioctl_emergency (unsigned int cmd, unsigned long arg);
int xmtp2km_ioctl_stoplink(unsigned int cmd, unsigned long arg);
int xmtp2km_ioctl_startlink(unsigned int cmd, unsigned long arg);
int xmtp2km_ioctl_getmsu (unsigned int cmd, unsigned long arg);
int xmtp2km_ioctl_putmsu (unsigned int cmd, unsigned long arg);
int xmtp2km_ioctl_getbsnt (unsigned int cmd, unsigned long arg);
int xmtp2km_ioctl_gettbq (unsigned int cmd, unsigned long arg);
int xmtp2km_ioctl_lnkrcvy (unsigned int cmd, unsigned long arg);
int xmtp2km_ioctl_getopm (unsigned int cmd, unsigned long arg);
int xmtp2km_ioctl_close(void);
int xmtp2km_ioctl_stop_fac (unsigned int cmd, unsigned long arg);
/* protoypes for functions used by the xmtp2km kernel module startup and shutdown functions */
void xmtp2km_init (void);
void xmtp2km_shutdown (void);



void* xmtp2_memset(void *b, int c, int len)
{
	return memset(b,c,len);
}

void xmtp2_printk(const char * fmt, ...)
{
	va_list args;
	char buf[1024];
	va_start(args, fmt);
	vsnprintf(buf, sizeof(buf), fmt, args);
	printk(KERN_INFO "%s", buf);
	va_end(args);
}

void * xmtp2km_kmalloc (const unsigned int bsize)
/***************************************************************************************/
{
	return kmalloc (bsize, GFP_ATOMIC);
}

void xmtp2km_kfree (void * p_buffer)
/***************************************************************************************/
{
	kfree (p_buffer);
}

void xmtp2km_lock(unsigned long *flags)
{
	spin_lock_irqsave(&xmtp2km_spinlock,*flags);
}

void xmtp2km_unlock(unsigned long *flags)
{
    spin_unlock_irqrestore(&xmtp2km_spinlock,*flags);
}

int xmtp2km_ratelimit(void)
{
    return net_ratelimit();
}

int xmtp2km_access_ok   (int type, const void *p_addr, unsigned long n)
/***************************************************************************************/
{
	switch (type) 
	{
		case XMTP2_VERIFY_WRITE:
			return access_ok (VERIFY_WRITE, p_addr, n);
			break;
		case XMTP2_VERIFY_READ:
			return access_ok (VERIFY_READ, p_addr, n);
			break;
		default:
			break;
	}
	return 0;
}

unsigned long xmtp2km_copy_to_user   (void *p_to, const void *p_from, unsigned long n)
/***************************************************************************************/
{
	return __copy_to_user (p_to, p_from, n);
}

unsigned long xmtp2km_copy_from_user (void *p_to, const void *p_from, unsigned long n)
/***************************************************************************************/
{
	return __copy_from_user (p_to, p_from, n);
}

int xmtp2km_strncmp (const char *p_s1, const char *p_s2, size_t n)
/***************************************************************************************/
{
	return strncmp (p_s1, p_s2, n);
}

void * xmtp2km_memset(void * p_mb, const int init_value, const unsigned int n)
/***************************************************************************************/
{
	return memset (p_mb, init_value, n);
}

void * xmtp2km_memcpy(void * p_to, const void * p_from, const unsigned int n)
/***************************************************************************************/
{
	return memcpy (p_to, p_from, n);
}

int xmtp2km_open(struct inode *inode, struct file *filp)
/***************************************************************************************/
{
	struct xmtp2km_dev *dev; /* device information */

	module_use_count++;

	dev = container_of(inode->i_cdev, struct xmtp2km_dev, cdev);
	filp->private_data = dev; /* for other methods */

	return 0;          /* success */
}

int xmtp2km_release(struct inode *inode, struct file *filp)
/***************************************************************************************/
{
	if (module_use_count > 0) module_use_count--;
	
	xmtp2km_ioctl_close();
	
	return 0;
}

int xmtp2km_ioctl(struct inode *inode, struct file *filp,
                 unsigned int cmd, unsigned long arg)
/***************************************************************************************/
{
	int err = 0;
	int ret = 0; /* default is success */
	unsigned long flags;

	/*
	 * extract the type and number bitfields, and don't decode
	 * wrong cmds: return ENOTTY (inappropriate ioctl) before access_ok()
	 */
	if (_IOC_TYPE(cmd) != XMTP2KM_IOC_MAGIC) return -ENOTTY;
	if (_IOC_NR(cmd) > XMTP2KM_IOC_MAXNR) return -ENOTTY;

	/*
	 * the direction is a bitmask, and VERIFY_WRITE catches R/W
	 * transfers. `Type' is user-oriented, while
	 * access_ok is kernel-oriented, so the concept of "read" and
	 * "write" is reversed
	 */
	if (_IOC_DIR(cmd) & _IOC_READ)
		err = !access_ok(VERIFY_WRITE, (void __user *)arg, _IOC_SIZE(cmd));
	else if (_IOC_DIR(cmd) & _IOC_WRITE)
		err =  !access_ok(VERIFY_READ, (void __user *)arg, _IOC_SIZE(cmd));

	if (err) return -EFAULT;


	switch(cmd) 
	{
		case XMTP2KM_IOCS_BINIT:
			ret = xmtp2km_ioctl_binit ();
			break;
		case XMTP2KM_IOCS_STOP_FAC:
			ret = xmtp2km_ioctl_stop_fac (cmd, arg);
			break;
		case XMTP2KM_IOCS_OPSPARMS:
			 ret = xmtp2km_ioctl_opsparms (cmd, arg);
			break;
		case XMTP2KM_IOCS_PWR_ON:
			//printk ("%s ptr %u size %u\n", __FUNCTION__, (unsigned int)((void __user *)arg), _IOC_SIZE(cmd));
			ret = xmtp2km_ioctl_pwr_on (cmd, arg);
			break;
		case XMTP2KM_IOCS_EMERGENCY:
		case XMTP2KM_IOCS_EMERGENCY_CEASES:
			//printk ("%s ptr %u size %u\n", __FUNCTION__, (unsigned int)((void __user *)arg), _IOC_SIZE(cmd));
			ret = xmtp2km_ioctl_emergency (cmd, arg);
			break;
		case XMTP2KM_IOCS_STARTLINK:
			//printk ("%s ptr %u size %u\n", __FUNCTION__, (unsigned int)((void __user *)arg), _IOC_SIZE(cmd));
			ret = xmtp2km_ioctl_startlink (cmd, arg);
			break;
		case XMTP2KM_IOCS_STOPLINK:
			//printk ("%s ptr %u size %u\n", __FUNCTION__, (unsigned int)((void __user *)arg), _IOC_SIZE(cmd));
			ret = xmtp2km_ioctl_stoplink (cmd, arg);
			break;
		case XMTP2KM_IOCG_GETMSU:
			ret = xmtp2km_ioctl_getmsu (cmd, arg);
			break;
		case XMTP2KM_IOCS_PUTMSU:
			ret = xmtp2km_ioctl_putmsu (cmd, arg);
			break;
		case XMTP2KM_IOCX_GETTBQ:
			ret = xmtp2km_ioctl_gettbq (cmd, arg);
			break;
		case XMTP2KM_IOCS_LNKRCVY:
			ret = xmtp2km_ioctl_lnkrcvy (cmd, arg);
			break;
		case XMTP2KM_IOCX_GETBSNT:
			ret = xmtp2km_ioctl_getbsnt (cmd, arg);
			break;
		case XMTP2KM_IOCG_GETOPM:
			ret = xmtp2km_ioctl_getopm (cmd, arg);
			break;
		default:  /* redundant, as cmd was checked against MAXNR */
			ret = -ENOTTY;
			break;
	}
 

	return ret;
}

struct file_operations xmtp2km_fops = {
	.owner =    THIS_MODULE,
	.ioctl =    xmtp2km_ioctl,
	.open =     xmtp2km_open,
	.release =  xmtp2km_release,
};

void xmtp2km_cleanup_module(void)
/***************************************************************************************/
{
	/*
	 * The cleanup function is used to handle initialization failures as well.
	 * Thefore, it must be careful to work correctly even if some of the items
	 * have not been initialized
	 */
	int i;
	dev_t devno;

	xmtp2km_shutdown ();
	devno = MKDEV(xmtp2km_major, xmtp2km_minor);

	/* Get rid of our char dev entries */
	if (xmtp2km_devices) 
	{
		for (i = 0; i < xmtp2km_nr_devs; i++) 
		{
			cdev_del(&xmtp2km_devices[i].cdev);
		}
		kfree(xmtp2km_devices);
	}

	/* cleanup_module is never called if registering failed */
	unregister_chrdev_region(devno, xmtp2km_nr_devs);
	printk ("xmtp2km %s:major %d minor %d\n", __FUNCTION__, xmtp2km_major, xmtp2km_minor);

	/* and call the cleanup functions for friend devices */
}


static void xmtp2km_setup_cdev(struct xmtp2km_dev *dev, int index)
/***************************************************************************************/
{
	/* Set up the char_dev structure for this device.  */
	int err, devno = MKDEV(xmtp2km_major, xmtp2km_minor + index);
    
	cdev_init(&dev->cdev, &xmtp2km_fops);
	dev->cdev.owner = THIS_MODULE;
	dev->cdev.ops = &xmtp2km_fops;
	err = cdev_add (&dev->cdev, devno, 1);
	/* Fail gracefully if need be */
	if (err)
		printk(KERN_NOTICE "Error %d adding xmtp2km%d", err, index);
	else
		printk ("xmtp2km %s:major %d minor %d\n", __FUNCTION__, xmtp2km_major, xmtp2km_minor + index);
}


int xmtp2km_init_module(void)
/***************************************************************************************/
{
	int result, i;
	dev_t dev = 0;

	printk ("MARK 0\n");
	info_u (__FILE__, __FUNCTION__,
"MARK", 0);
/*
 * Get a range of minor numbers to work with, asking for a dynamic
 * major unless directed otherwise at load time.
 */

 	spin_lock_init(&xmtp2km_spinlock);

	if (xmtp2km_major) 
	{
		dev = MKDEV(xmtp2km_major, xmtp2km_minor);
		result = register_chrdev_region(dev, xmtp2km_nr_devs, "xmtp2km");
	} 
	else 
	{
		result = alloc_chrdev_region(&dev, xmtp2km_minor, xmtp2km_nr_devs, "xmtp2km");
		xmtp2km_major = MAJOR(dev);
	}
	if (result < 0) 
	{
		printk(KERN_WARNING "xmtp2km: can't get major %d\n", xmtp2km_major);
		return result;
	}

        /* 
	 * allocate the devices -- we can't have them static, as the number
	 * can be specified at load time
	 */
	xmtp2km_devices = kmalloc(xmtp2km_nr_devs * sizeof(struct xmtp2km_dev), GFP_KERNEL);
	if (!xmtp2km_devices) 
	{
		result = -ENOMEM;
		goto fail;  /* Make this more graceful */
	}
	memset(xmtp2km_devices, 0, xmtp2km_nr_devs * sizeof(struct xmtp2km_dev));

        /* Initialize each device. */
	for (i = 0; i < xmtp2km_nr_devs; i++) 
	{
		init_MUTEX(&xmtp2km_devices[i].sem);
		xmtp2km_setup_cdev(&xmtp2km_devices[i], i);
	}

	xmtp2km_init ();

        /* At this point call the init function for any friend device; 
	 * xmtp2km has no friends */

	return 0; /* succeed */

  fail:
	xmtp2km_cleanup_module();
	return result;
}

module_init(xmtp2km_init_module);
module_exit(xmtp2km_cleanup_module);

EXPORT_SYMBOL (xmtp2km_register);
EXPORT_SYMBOL (xmtp2km_unregister);
EXPORT_SYMBOL (xmtp2km_bs_handler);
EXPORT_SYMBOL (xmtp2km_tap_enable);
EXPORT_SYMBOL (xmtp2km_tap_disable);
EXPORT_SYMBOL (xmtp2km_facility_state_change);
