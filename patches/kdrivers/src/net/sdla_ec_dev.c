/*****************************************************************************
* sdla_ec_dev.c 
* 		
* 		WANPIPE(tm) EC Device
*
* Authors: 	Nenad Corbic <ncorbic@sangoma.com>
*
* Copyright:	(c) 2003-2006 Sangoma Technologies Inc.
*
*		This program is free software; you can redistribute it and/or
*		modify it under the terms of the GNU General Public License
*		as published by the Free Software Foundation; either version
*		2 of the License, or (at your option) any later version.
* ============================================================================
* Jan 16, 2006	Nenad Corbic	Initial version.
*****************************************************************************/

#include "wanpipe_includes.h"
#include "wanpipe_defines.h"
#include "wanpipe_debug.h"
#include "wanpipe_common.h"
#include "wanpipe.h"

#if defined (__LINUX__)
# include "if_wanpipe.h"
#endif

#include "sdla_ec.h"
#include <linux/smp_lock.h>
#if defined(__x86_64__)
#if defined(CONFIG_COMPAT) && defined(WP_CONFIG_COMPAT)
# include <linux/ioctl32.h>
#else
# warning "Wanpipe Warning: Kernel IOCTL32 Not supported!"
# warning "Wanpipe A104D Hardware Echo Cancellation will not be supported!"
#endif
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
# define WP_ECDEV_UDEV 1
# undef WP_CONFIG_DEVFS_FS
#else
# undef WP_ECDEV_UDEV
# ifdef CONFIG_DEVFS_FS
#  include <linux/devfs_fs_kernel.h>
#  define WP_CONFIG_DEVFS_FS
# else
#  undef WP_CONFIG_DEVFS_FS
#  warning "Error: Hardware EC Device requires DEVFS: HWEC Will not be supported!"
# endif
#endif

#define WP_ECDEV_MAJOR 242
#define WP_ECDEV_MINOR_OFFSET 0

#ifdef WP_ECDEV_UDEV

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,15)
#define WP_CLASS_DEV_CREATE(class, devt, device, name) \
        class_device_create(class, NULL, devt, device, name)
#else
#define WP_CLASS_DEV_CREATE(class, devt, device, name) \
        class_device_create(class, devt, device, name)
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,13)
static struct class *wp_ecdev_class = NULL;
#else
static struct class_simple *wp_ecdev_class = NULL;
#define class_create class_simple_create
#define class_destroy class_simple_destroy
#define class_device_create class_simple_device_add
#define class_device_destroy(a, b) class_simple_device_remove(b)
#endif

#endif

#define	UNIT(file) MINOR(file->f_dentry->d_inode->i_rdev)

#define WP_ECDEV_MAX_CHIPS 255

typedef struct ecdev_element {
	void *ec;
	int  open;
#ifdef WP_CONFIG_DEVFS_FS 
	devfs_handle_t devfs_handle;
#endif
} ecdev_element_t;

static ecdev_element_t wp_ecdev_hash[WP_ECDEV_MAX_CHIPS];
static wan_spinlock_t wp_ecdev_hash_lock;
static int wp_ecdev_global_cnt=0;


static int wp_ecdev_open(struct inode*, struct file*);
static int wp_ecdev_release(struct inode*, struct file*);
static WAN_IOCTL_RET_TYPE WANDEF_IOCTL_FUNC(wp_ecdev_ioctl, struct file*, unsigned int, unsigned long);
#if defined(__x86_64__) && defined(CONFIG_COMPAT) && defined(WP_CONFIG_COMPAT)
static int wp_ecdev_ioctl32(unsigned int, unsigned int, unsigned long,struct file*);
#endif

/*==============================================================
  Global Variables
 */
static struct file_operations wp_ecdev_fops = {
	owner: THIS_MODULE,
	llseek: NULL,
	open: wp_ecdev_open,
	release: wp_ecdev_release,
	WAN_IOCTL: wp_ecdev_ioctl,
	read: NULL,
	write: NULL,
	poll: NULL,
	mmap: NULL,
	flush: NULL,
	fsync: NULL,
	fasync: NULL,
};


static int wp_ecdev_reg_globals(void)
{
	int err;
	
	wan_spin_lock_init(&wp_ecdev_hash_lock, "wan_ecdev_lock");
	memset(wp_ecdev_hash,0,sizeof(wp_ecdev_hash));
	
	DEBUG_EVENT("%s: Registering Wanpipe ECDEV Device!\n",__FUNCTION__);
#ifdef WP_ECDEV_UDEV  
	wp_ecdev_class = class_create(THIS_MODULE, "wp_ec");
#endif
	
#ifdef WP_CONFIG_DEVFS_FS
	err=devfs_register_chrdev(WP_ECDEV_MAJOR, "wp_ec", &wp_ecdev_fops);  
	if (err) {
              	DEBUG_EVENT("Unable to register tor device on %d\n", WP_ECDEV_MAJOR);
	    	return err;	
	}
#else
	if ((err = register_chrdev(WP_ECDEV_MAJOR, "wp_ec", &wp_ecdev_fops))) {
		DEBUG_EVENT("Unable to register tor device on %d\n", WP_ECDEV_MAJOR);
		return err;
	}
#endif
	
#if defined(__x86_64__) && defined(CONFIG_COMPAT) && defined(WP_CONFIG_COMPAT)   
	register_ioctl32_conversion(WAN_OCT6100_CMD_GET_INFO, wp_ecdev_ioctl32);
	register_ioctl32_conversion(WAN_OCT6100_CMD_SET_INFO, wp_ecdev_ioctl32);
	register_ioctl32_conversion(WAN_OCT6100_CMD_CLEAR_RESET, wp_ecdev_ioctl32);
	register_ioctl32_conversion(WAN_OCT6100_CMD_SET_RESET, wp_ecdev_ioctl32);
	register_ioctl32_conversion(WAN_OCT6100_CMD_BYPASS_ENABLE, wp_ecdev_ioctl32);
	register_ioctl32_conversion(WAN_OCT6100_CMD_BYPASS_DISABLE, wp_ecdev_ioctl32);
	register_ioctl32_conversion(WAN_OCT6100_CMD_API_WRITE, wp_ecdev_ioctl32);
	register_ioctl32_conversion(WAN_OCT6100_CMD_API_WRITE_SMEAR, wp_ecdev_ioctl32);
	register_ioctl32_conversion(WAN_OCT6100_CMD_API_WRITE_BURST, wp_ecdev_ioctl32);
	register_ioctl32_conversion(WAN_OCT6100_CMD_API_READ, wp_ecdev_ioctl32);
	register_ioctl32_conversion(WAN_OCT6100_CMD_API_READ_BURST, wp_ecdev_ioctl32);
#endif
	return err;
}
 
static int wp_ecdev_unreg_globals(void)
{
	DEBUG_EVENT("%s: Unregistering Wanpipe ECDEV Device!\n",__FUNCTION__);
#if defined(__x86_64__) && defined(CONFIG_COMPAT) && defined(WP_CONFIG_COMPAT)
	unregister_ioctl32_conversion(WAN_OCT6100_CMD_GET_INFO);
	unregister_ioctl32_conversion(WAN_OCT6100_CMD_SET_INFO);
	unregister_ioctl32_conversion(WAN_OCT6100_CMD_CLEAR_RESET);
	unregister_ioctl32_conversion(WAN_OCT6100_CMD_SET_RESET);
	unregister_ioctl32_conversion(WAN_OCT6100_CMD_BYPASS_ENABLE);
	unregister_ioctl32_conversion(WAN_OCT6100_CMD_BYPASS_DISABLE);
	unregister_ioctl32_conversion(WAN_OCT6100_CMD_API_WRITE);
	unregister_ioctl32_conversion(WAN_OCT6100_CMD_API_WRITE_SMEAR);
	unregister_ioctl32_conversion(WAN_OCT6100_CMD_API_WRITE_BURST);
	unregister_ioctl32_conversion(WAN_OCT6100_CMD_API_READ);
	unregister_ioctl32_conversion(WAN_OCT6100_CMD_API_READ_BURST);
#endif

#ifdef WP_ECDEV_UDEV
	class_destroy(wp_ecdev_class);
#endif

#ifdef WP_CONFIG_DEVFS_FS           
        devfs_unregister_chrdev(WP_ECDEV_MAJOR, "wp_ec");    
#else
	unregister_chrdev(WP_ECDEV_MAJOR, "wp_ec");
#endif
	return 0;
}

int wanpipe_ecdev_reg(void *ec, char *ec_name, int ec_chip_no)
{
	
	if (wp_ecdev_global_cnt == 0){
		wp_ecdev_reg_globals();
	}
	wp_ecdev_global_cnt++;
	
	if (ec_chip_no > WP_ECDEV_MAX_CHIPS){
		return -EINVAL;
	}
	
	if (wp_ecdev_hash[ec_chip_no].ec){
		wp_ecdev_global_cnt--;
		if (wp_ecdev_global_cnt == 0){
			wp_ecdev_unreg_globals();
		}
		return -EBUSY;
	}
	
	wp_ecdev_hash[ec_chip_no].ec=ec;
	wp_ecdev_hash[ec_chip_no].open=0;

#ifdef WP_ECDEV_UDEV	
	WP_CLASS_DEV_CREATE(	wp_ecdev_class,
				MKDEV(WP_ECDEV_MAJOR, ec_chip_no),
				NULL,
				ec_name);
#endif
	
#ifdef WP_CONFIG_DEVFS_FS 
	{
	umode_t mode = S_IFCHR|S_IRUGO|S_IWUGO;    
	wp_ecdev_hash[ec_chip_no].devfs_handle = 
		devfs_register(NULL, ec_name, DEVFS_FL_DEFAULT, WP_ECDEV_MAJOR, 
			       ec_chip_no, mode, &wp_ecdev_fops, NULL);
	}
#endif	
	
	return 0;
}

int wanpipe_ecdev_unreg(int ec_chip_no)
{

	if (ec_chip_no > WP_ECDEV_MAX_CHIPS){
		return -EINVAL;
	}
	
	if (wp_ecdev_hash[ec_chip_no].ec){
		wp_ecdev_hash[ec_chip_no].ec=NULL;
	}
	wan_clear_bit(0,&wp_ecdev_hash[ec_chip_no].open);

#ifdef WP_ECDEV_UDEV	
	class_device_destroy(	wp_ecdev_class,
				MKDEV(WP_ECDEV_MAJOR,
				ec_chip_no)); 
#endif

#ifdef WP_CONFIG_DEVFS_FS
	devfs_unregister(wp_ecdev_hash[ec_chip_no].devfs_handle);
#endif 

	wp_ecdev_global_cnt--;
	if (wp_ecdev_global_cnt == 0){
		wp_ecdev_unreg_globals();
	}

	return 0;
}

static int wp_ecdev_open(struct inode *inode, struct file *file)
{
	wan_smp_flag_t	flags;
	void		*ec;
	u32		minor = UNIT(file);
	
 	DEBUG_TEST ("%s: GOT Index %i\n",__FUNCTION__, minor);
	
	if (minor > WP_ECDEV_MAX_CHIPS){
		return -EINVAL;
	}
	
	wan_spin_lock_irq(&wp_ecdev_hash_lock,&flags);
	if (wan_test_and_set_bit(0,&wp_ecdev_hash[minor].open)){
		wan_spin_unlock_irq(&wp_ecdev_hash_lock,&flags);
		return -EBUSY;
	}
	ec=wp_ecdev_hash[minor].ec;
	wan_spin_unlock_irq(&wp_ecdev_hash_lock,&flags);
	
	
	file->private_data = ec;   
	
	DEBUG_TEST ("%s: DRIVER OPEN Chip %i %p\n",
		__FUNCTION__, minor, ec);
	
	return 0;
}


static int wp_ecdev_release(struct inode *inode, struct file *file)
{
	wan_smp_flag_t	flags;
	void		*ec = file->private_data;
	u32		minor = UNIT(file);

	DEBUG_EVENT ("%s: GOT Chip=%i Ptr=%p\n",
		__FUNCTION__, minor, ec);

	if (ec == NULL){
		return -ENODEV;
	}
	
			
	wan_spin_lock_irq(&wp_ecdev_hash_lock,&flags);
	wan_clear_bit(0,&wp_ecdev_hash[minor].open);
	wan_spin_unlock_irq(&wp_ecdev_hash_lock,&flags);
	
	file->private_data=NULL;
	return 0;
}

extern int wan_ec_dev_ioctl(void*, void*);
static WAN_IOCTL_RET_TYPE WANDEF_IOCTL_FUNC(wp_ecdev_ioctl, struct file *file, unsigned int cmd, unsigned long data)
{
	void	*ec = file->private_data;
	long ret;

	if (ec == NULL){
		return -ENODEV;
	}
	if (data == 0){
		return -EINVAL;
	}
	
	ret =  wan_ec_dev_ioctl(ec,(void*)data);
	return ret;
}

#if defined(__x86_64__) && defined(CONFIG_COMPAT) && defined(WP_CONFIG_COMPAT)
static int
wp_ecdev_ioctl32(unsigned int fd, unsigned int cmd, unsigned long data, struct file *file)
{
	void	*ec = file->private_data;

	if (ec == NULL){
		return -ENODEV;
	}
	if (data == 0){
		return -EINVAL;
	}
	
	return wan_ec_dev_ioctl(ec,(void*)data);
}
#endif
