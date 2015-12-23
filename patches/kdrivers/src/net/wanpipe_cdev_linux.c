/*****************************************************************************
* wanpipe_cdev_linux.c	 Linux CDEV Implementation
*
* Author:	Nenad Corbic
*
* Copyright:	(c) 2008 Sangoma Technologies Inc.
*
*		This program is free software; you can redistribute it and/or
*		modify it under the terms of the GNU General Public License
*		as published by the Free Software Foundation; either version
*		2 of the License, or (at your option) any later version.
* ============================================================================
*/


#include "wanpipe_cdev_iface.h"

#define WP_CDEV_MAJOR 241
#define WP_CDEV_MINOR_OFFSET 0
#define WP_CDEV_MAX_MINORS 5000

#define WP_CDEV_MAX_VOICE_MINOR

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)


# if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,26)
#  define class_device_destroy device_destroy
#  define WP_CLASS_DEV_CREATE(class, devt, device, priv_data, name) \
       device_create_drvdata(class, device, devt, priv_data, name)
# elif LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,15)
#  define WP_CLASS_DEV_CREATE(class, devt, device, priv_data, name) \
       class_device_create(class, NULL, devt, device, name)
# else
#  define WP_CLASS_DEV_CREATE(class, devt, device, priv_data, name) \
       class_device_create(class, devt, device, name)
# endif

# if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,13)
static struct class *wp_cdev_class = NULL;
# else
static struct class_simple *wp_cdev_class = NULL;
#  define class_create class_simple_create
#  define class_destroy class_simple_destroy
#  define class_device_create class_simple_device_add
#  define class_device_destroy(a, b) class_simple_device_remove(b)
# endif

# define	UNIT(file) MINOR(file->f_dentry->d_inode->i_rdev)

#endif/* #if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0) */


# define WP_CDEV_SPAN_MASK		0xFFFF
# define WP_CDEV_SPAN_SHIFT		5		//8
# define WP_CDEV_CHAN_MASK		0x1F	//0xFF

# define WP_CDEV_SET_MINOR(span,chan) ((((span-1)&WP_CDEV_SPAN_MASK)<<WP_CDEV_SPAN_SHIFT)|(chan&WP_CDEV_CHAN_MASK))
# define WP_CDEV_GET_SPAN_FROM_MINOR(minor) ((((minor)>>WP_CDEV_SPAN_SHIFT)&WP_CDEV_SPAN_MASK)+1)
# define WP_CDEV_GET_CHAN_FROM_MINOR(minor) ((minor)&WP_CDEV_CHAN_MASK)

# define WP_CDEV_MAX_SPANS 128
# define WP_CDEV_MAX_CHANS 31

# define WP_CDEV_MAX_SPAN_CHAN_MINOR (WP_CDEV_MAX_SPANS+1)

/* CFG Deivce is always 1 */
# define WP_CDEV_CFG_DEV_MOFFSET (WP_CDEV_MAX_SPANS+1)

/* CTRL Deivce is always 2 */
# define WP_CDEV_CTRL_DEV_MOFFSET (WP_CDEV_MAX_SPANS+2)

/* Logger Deivce is always 3 */
# define WP_CDEV_LOGGER_DEV_MOFFSET (WP_CDEV_MAX_SPANS+3)

/* Timer devices can range from WP_CDEV_TIMER_DEV_MOFFSET to WP_CDEV_TIMER_DEV_MOFFSET + WP_MAX_TIMER_DEV_CNT */
# define WP_CDEV_TIMER_DEV_MOFFSET (WP_CDEV_MAX_SPANS+10)
# define WP_MAX_TIMER_DEV_CNT 20

# define WP_CDEV_SET_OFFSET_MINOR(offset)  ((offset) << (WP_CDEV_SPAN_SHIFT))

# define WP_TIMER_DEV(minor) (minor >= WP_CDEV_TIMER_DEV_MOFFSET && minor <= (WP_CDEV_TIMER_DEV_MOFFSET+WP_MAX_TIMER_DEV_CNT))


# define DEBUG_CDEV DEBUG_TEST

static int wanpipe_create_cdev(wanpipe_cdev_t *cdev, int minor, int *counter);
static int wanpipe_free_cdev(wanpipe_cdev_t *cdev, int minor, int *counter);
static int wp_cdev_open(struct inode *inode, struct file *file);
static int wp_cdev_release(struct inode *inode, struct file *file);

static int wan_verify_iovec(wan_msghdr_t *m, wan_iovec_t *iov, char *address, int mode);
static int wan_memcpy_fromiovec(unsigned char *kdata, wan_iovec_t *iov, int len);
static int wan_memcpy_toiovec(wan_iovec_t *iov, unsigned char *kdata, int len);

static ssize_t wp_cdev_read(struct file *file, char *usrbuf, size_t count, loff_t *ppos);
static ssize_t wp_cdev_write(struct file *file, const char *usrbuf, size_t count, loff_t *ppos);
static WAN_IOCTL_RET_TYPE WANDEF_IOCTL_FUNC(wp_cdev_ioctl, struct file *file, unsigned int cmd, unsigned long data);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,18)
static long wp_cdev_compat_ioctl(struct file *file, unsigned int cmd, unsigned long data);
#endif
static unsigned int wp_cdev_poll(struct file *file, struct poll_table_struct *wait_table);

/*=========================================================
 * Type Defines
 *=========================================================*/

typedef struct wanpipe_cdev_device
{
	int init;
	int dev_cnt;
	int timer_dev_cnt;
	spinlock_t lock;
	void *idx[WP_CDEV_MAX_MINORS];

}wanpipe_cdev_device_t;

;

/*=========================================================
 * Static Defines
 *=========================================================*/

static struct file_operations wp_cdev_fops = {
	owner: THIS_MODULE,
	llseek: NULL,
	open: wp_cdev_open,
	release: wp_cdev_release,
	WAN_IOCTL: wp_cdev_ioctl,
	read: wp_cdev_read,
	write: wp_cdev_write,
	poll: wp_cdev_poll,
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,18)
	compat_ioctl: wp_cdev_compat_ioctl,
#endif
	mmap: NULL,
	flush: NULL,
	fsync: NULL,
	fasync: NULL,
};


static struct cdev wp_cdev_dev = {
#ifndef LINUX_FEAT_2624
	.kobj	=	{.name = "wanpipe", },
#endif
	.owner	=	THIS_MODULE,
};

/* Global WANDEV Structure */
static wanpipe_cdev_device_t wandev;


static struct device_attribute wanpipe_device_attrs[] = {
	__ATTR_NULL,
};

static struct bus_type wanpipe_device_bus = {
	.name = "wanpipe_devices",
	.dev_attrs = wanpipe_device_attrs,
};


/*=========================================================
 * PUBLIC FUNCTIONS
 *========================================================*/


/*=========================================================
 * wanpipe_global_cdev_init
 *
 *=========================================================*/

int wanpipe_global_cdev_init(void)
{
	int err;
#ifdef LINUX_2_4
	if ((err = register_chrdev(WP_CDEV_MAJOR, "wanpipe", &wp_cdev_fops))) {
		DEBUG_ERROR("%s(): Error unable to register device!\n",__FUNCTION__);
		return err;
	}

	wp_cdev_class = class_create(THIS_MODULE, "wanpipe");

#else
	dev_t dev = MKDEV(WP_CDEV_MAJOR, 0);

	if ((err=register_chrdev_region(dev, WP_CDEV_MAX_MINORS, "wanpipe"))) {
		DEBUG_ERROR("%s(): Error unable to register device!\n",__FUNCTION__);
		return err;
	}

	cdev_init(&wp_cdev_dev, &wp_cdev_fops);
	if (cdev_add(&wp_cdev_dev, dev, WP_CDEV_MAX_MINORS)) {
		kobject_put(&wp_cdev_dev.kobj);
		unregister_chrdev_region(dev, WP_CDEV_MAX_MINORS);
		DEBUG_ERROR("%s(): Error cdev_add!\n",__FUNCTION__);
		return -EINVAL;
	}

	wp_cdev_class = class_create(THIS_MODULE, "wanpipe");
	if (IS_ERR(wp_cdev_class)) {
		DEBUG_ERROR("%s(): Error creating class!\n",__FUNCTION__);
		cdev_del(&wp_cdev_dev);
		unregister_chrdev_region(dev, WP_CDEV_MAX_MINORS);
		return -EINVAL;
	}
		
	err = bus_register(&wanpipe_device_bus);
    if (err) {
		DEBUG_ERROR("%s(): Error registering bus!\n",__FUNCTION__);
       	return err;
	}
#endif

	memset(&wandev,0,sizeof(wanpipe_cdev_device_t));

	wan_spin_lock_init(&wandev.lock, "wp_cdev_lock");

	wan_set_bit(0,&wandev.init);

	return 0;	

}

/*=========================================================
 * wanpipe_global_cdev_free
 *========================================================*/

int wanpipe_global_cdev_free(void)
{
	if (wandev.dev_cnt) {
		DEBUG_ERROR("%s: Error: Wanpipe CDEV Busy - failed to free!\n",__FUNCTION__);
		return 1;
	}

	wan_clear_bit(0,&wandev.init);

	class_destroy(wp_cdev_class);

#ifdef LINUX_2_4
	unregister_chrdev(WP_CDEV_MAJOR, "wanpipe");
#else
	cdev_del(&wp_cdev_dev);
  	unregister_chrdev_region(MKDEV(WP_CDEV_MAJOR, 0), WP_CDEV_MAX_MINORS);
	
	bus_unregister(&wanpipe_device_bus);
#endif

	return 0;	
}


/*=========================================================
 * wanpipe_cdev_wake
 *========================================================*/



/*=========================================================
 * wanpipe_cdev_tdm_create
 *========================================================*/

int wanpipe_cdev_tdm_create(wanpipe_cdev_t *cdev)
{
	
	int minor=-1;

	if (!wan_test_bit(0,&wandev.init)) {
		DEBUG_ERROR("%s(): Error global device not initialized!\n",__FUNCTION__);
		return -1;
	}

	if (!cdev->dev_ptr) {
		DEBUG_ERROR("%s(): Error cdev->dev_ptr not initialized!\n",__FUNCTION__);
		return -1;
	}

	if (cdev->span < 0 || cdev->span > WP_CDEV_MAX_SPANS) {
		DEBUG_ERROR("%s(): Error span out of range %i!\n",__FUNCTION__,cdev->span);
		return -1;
	}
	if (cdev->chan < 0 || cdev->chan > WP_CDEV_MAX_CHANS) {
		DEBUG_ERROR("%s(): Error chan out of range %i!\n",__FUNCTION__,cdev->chan);
		return -1;
	}
	minor=WP_CDEV_SET_MINOR(cdev->span,cdev->chan);

	cdev->name[WAN_IFNAME_SZ]=0;

	if (strlen(cdev->name) == 0) {
		return -1;
	}

	return  wanpipe_create_cdev(cdev, minor, NULL);
	
}


/*=========================================================
 * wanpipe_cdev_tdm_ctrl_create
 *========================================================*/

int wanpipe_cdev_tdm_ctrl_create(wanpipe_cdev_t *cdev)
{
	
	int minor=-1;

	if (!wan_test_bit(0,&wandev.init)) {
		DEBUG_ERROR("%s(): Error global device not initialized!\n",__FUNCTION__);
		return -1;
	}

	if (!cdev->dev_ptr) {
		DEBUG_ERROR("%s(): Error cdev->dev_ptr not initialized!\n",__FUNCTION__);
		return -1;
	}

	minor=WP_CDEV_SET_OFFSET_MINOR(WP_CDEV_CTRL_DEV_MOFFSET);

	cdev->name[WAN_IFNAME_SZ]=0;
	sprintf(cdev->name, "wanpipe_ctrl");

	return wanpipe_create_cdev(cdev, minor, NULL);
	
}


/*=========================================================
 * wanpipe_cdev_cfg_ctrl_create
 *========================================================*/

int wanpipe_cdev_cfg_ctrl_create(wanpipe_cdev_t *cdev)
{
	
	int minor=-1;

	if (!wan_test_bit(0,&wandev.init)) {
		DEBUG_ERROR("%s(): Error global device not initialized!\n",__FUNCTION__);
		return -1;
	}

	if (!cdev->dev_ptr) {
		DEBUG_ERROR("%s(): Error cdev->dev_ptr not initialized!\n",__FUNCTION__);
		return -1;
	}

	minor=WP_CDEV_SET_OFFSET_MINOR(WP_CDEV_CFG_DEV_MOFFSET);

	cdev->name[WAN_IFNAME_SZ]=0;
	sprintf(cdev->name, "wanpipe");

	return wanpipe_create_cdev(cdev, minor, NULL);
	
}

/*=========================================================
 * wanpipe_cdev_timer_create
 *========================================================*/
int wanpipe_cdev_logger_create(wanpipe_cdev_t *cdev)
{
	int minor=-1;

	if (!wan_test_bit(0,&wandev.init)) {
		DEBUG_ERROR("%s(): Error global device not initialized!\n",__FUNCTION__);
		return -1;
	}

	if (!cdev->dev_ptr) {
		DEBUG_ERROR("%s(): Error cdev->dev_ptr not initialized!\n",__FUNCTION__);
		return -1;
	}

	minor=WP_CDEV_SET_OFFSET_MINOR(WP_CDEV_LOGGER_DEV_MOFFSET);

	cdev->name[WAN_IFNAME_SZ]=0;
	sprintf(cdev->name, "wanpipe_logger");

	return wanpipe_create_cdev(cdev, minor, NULL);
}


/*=========================================================
 * wanpipe_cdev_timer_create
 *========================================================*/

int wanpipe_cdev_timer_create(wanpipe_cdev_t *cdev)
{
	
	int minor=-1;

	if (!wan_test_bit(0,&wandev.init)) {
		DEBUG_ERROR("%s(): Error global device not initialized!\n",__FUNCTION__);
		return -1;
	}

	if (!cdev->dev_ptr) {
		DEBUG_ERROR("%s(): Error cdev->dev_ptr not initialized!\n",__FUNCTION__);
		return -1;
	}

	if (wandev.timer_dev_cnt >= WP_MAX_TIMER_DEV_CNT) {
		DEBUG_ERROR("%s(): Error timer dev cnt limit %i!\n",__FUNCTION__,wandev.timer_dev_cnt);
		return -1;
	}

	minor=WP_CDEV_SET_OFFSET_MINOR(WP_CDEV_TIMER_DEV_MOFFSET+wandev.timer_dev_cnt);

	cdev->name[WAN_IFNAME_SZ]=0;
	sprintf(cdev->name, "wanpipe_timer%i",wandev.timer_dev_cnt);
	DEBUG_CDEV ("%s:%d TIMER DEV %s minor=%i\n",__FUNCTION__,__LINE__,cdev->name,minor);

	return wanpipe_create_cdev(cdev, minor, &wandev.timer_dev_cnt);
	
}


/*=========================================================
 * wanpipe_cdev_tdm_free
 *========================================================*/

int wanpipe_cdev_free(wanpipe_cdev_t *cdev)
{
	int minor;
	int *counter=NULL;

	DEBUG_CDEV ("%s:%d\n",__FUNCTION__,__LINE__);

	if (!CPRIV(cdev)) {
		DEBUG_ERROR("%s(): Error cdev priv not initialized!\n",__FUNCTION__); 
		return -ENODEV;
	}

	minor=CPRIV(cdev)->dev_minor;

	if (!wan_test_bit(0,&wandev.init)) {
		DEBUG_ERROR("%s(): Error global device not initialized!\n",__FUNCTION__);
		return -ENODEV;
	}

	if (minor < 0 || minor > WP_CDEV_MAX_MINORS) {
		DEBUG_ERROR("%s(): Error MINOR out of range %i!\n",__FUNCTION__,minor);
		return -EINVAL;
	}
	
	if (!wan_test_bit(0,&cdev->init)) {
		DEBUG_ERROR("%s(): Error cdev device not initialized!\n",__FUNCTION__);
		return -ENODEV;
	}

	if (WP_TIMER_DEV(minor)) {
		counter=&wandev.timer_dev_cnt;
	}

	DEBUG_CDEV ("%s:%d\n",__FUNCTION__,__LINE__);

	return  wanpipe_free_cdev(cdev, minor,counter);
}




/*=========================================================
 * PRIVATE CREATE/FREE FUNCTIONS
 *========================================================*/

static int wanpipe_create_cdev(wanpipe_cdev_t *cdev, int minor, int *counter)
{
	wan_smp_flag_t flags;
	wanpipe_cdev_priv_t *cdev_priv;
	char lname[100];

	cdev_priv = wan_kmalloc(sizeof(wanpipe_cdev_priv_t));
	if (!cdev_priv) {
		DEBUG_ERROR("%s(): Error unable to alloc cdev_priv mem!\n",__FUNCTION__);
		return -ENOMEM;
	}

	memset(cdev_priv,0,sizeof(wanpipe_cdev_priv_t));

	cdev->priv=cdev_priv;

	wan_spin_lock_irq(&wandev.lock,&flags);

	if (wandev.idx[minor] != NULL) {
		wan_spin_unlock_irq(&wandev.lock,&flags);
		wan_free(cdev_priv);
		/* Busy */
		DEBUG_ERROR("%s(): Error MINOR device busy %i!\n",__FUNCTION__,minor);
		return 1;
	}

	wandev.idx[minor] = cdev;


	wan_set_bit(0,&cdev->init);
	cdev_priv->dev_minor = minor;

	wandev.dev_cnt++;

	if (counter) {
		*counter = *counter + 1;
	}

	sprintf(lname,"wp_cdev_wandev_lock%d",wandev.dev_cnt);

	wan_spin_unlock_irq(&wandev.lock,&flags);

	wan_spin_lock_init(&cdev_priv->lock, lname);
	init_waitqueue_head(&cdev_priv->poll_wait);

	WP_CLASS_DEV_CREATE(wp_cdev_class,
			    MKDEV(WP_CDEV_MAJOR, minor), NULL, NULL,cdev->name);

	DEBUG_CDEV("%s(): CREATING CDEV DEVICE MINOR 0x%X!  cdev=%p idx=%p\n",
			__FUNCTION__,minor,cdev,wandev.idx[minor]);
	

	return 0;	
}


static int wanpipe_free_cdev(wanpipe_cdev_t *cdev, int minor, int *counter)
{
	wan_smp_flag_t flags;
	wanpipe_cdev_priv_t *cdev_priv = (wanpipe_cdev_priv_t *)cdev->priv;

	wan_spin_lock_irq(&wandev.lock,&flags);

	if (wandev.idx[minor] != cdev || !cdev_priv) {
		wan_spin_unlock_irq(&wandev.lock,&flags);
		/* Busy */
		DEBUG_ERROR("%s(): Error MINOR device busy 0x%X! cdev=%p  idx=%p cdev_priv=%p\n",
			__FUNCTION__,minor,cdev,wandev.idx[minor],cdev_priv);
		return 1;
	}

	wan_clear_bit(0,&cdev->init);

	wandev.idx[minor] = NULL;

	wandev.dev_cnt--;

	if (counter) {
		*counter = *counter - 1;
	}

	wan_spin_unlock_irq(&wandev.lock,&flags);

	cdev->priv=NULL;
	wan_free(cdev_priv);

	class_device_destroy(wp_cdev_class,
			     MKDEV(WP_CDEV_MAJOR, minor));

	DEBUG_CDEV("%s(): FREEING CDEV DEVICE MINOR 0x%X!\n",__FUNCTION__,minor);

	return 0;	
}





/*=============================================================
 * Private IO calls
 *============================================================*/

static int wp_cdev_open(struct inode *inode, struct file *file)
{
	wanpipe_cdev_t *cdev;
	wan_smp_flag_t flags;
	int minor = UNIT(file);
	int err;

	if (!wan_test_bit(0,&wandev.init)) {
		return -1;
	}

	wan_spin_lock_irq(&wandev.lock,&flags);
	cdev=wandev.idx[minor];
	
	if (cdev == NULL) {
		wan_spin_unlock_irq(&wandev.lock,&flags);
		/* No Dev */
		DEBUG_ERROR("%s(): Error cdev is null!\n",__FUNCTION__);
		return -ENODEV;
	}

	if (!wan_test_bit(0,&cdev->init)) {
		wan_spin_unlock_irq(&wandev.lock,&flags);
		/* Dev not initialized */
		DEBUG_ERROR("%s(): Error cdev is not initialized!\n",__FUNCTION__);
		return -ENODEV;
	}

	file->private_data = cdev;

	wan_spin_unlock_irq(&wandev.lock,&flags);

	wan_spin_lock(&CPRIV(cdev)->lock,&flags);

	if (cdev->ops.open) {
		err=cdev->ops.open(cdev->dev_ptr);
	} else {
		err=-EINVAL;
	}

	wan_spin_unlock(&CPRIV(cdev)->lock,&flags);


	DEBUG_CDEV ("%s: OPEN  S/C(%i/%i) Minor=0x%X\n",
		__FUNCTION__, cdev->span, cdev->chan, CPRIV(cdev)->dev_minor);

	return err;

}


static int wp_cdev_release(struct inode *inode, struct file *file)
{
	wan_smp_flag_t flag;
	wanpipe_cdev_t *cdev = file->private_data;
	int minor,err;

	if (!cdev || !CPRIV(cdev)) {
		return -ENODEV;
	}
	minor=CPRIV(cdev)->dev_minor;
	if (minor > WP_CDEV_MAX_MINORS) {
		DEBUG_ERROR("%s(): Error MINOR is out of range %i !\n",__FUNCTION__,minor);
		return -ENODEV;
	}

	if (wandev.idx[minor] != cdev) {
		DEBUG_ERROR("%s(): Error cdev does not match minor index ptr!\n",__FUNCTION__);
		return -ENODEV;
	}

	wan_spin_lock(&CPRIV(cdev)->lock,&flag);
	if (cdev->ops.close) {
		err=cdev->ops.close(cdev->dev_ptr);
	}
	wan_spin_unlock(&CPRIV(cdev)->lock,&flag);

	DEBUG_CDEV ("%s: CLOSE  S/C(%i/%i) Minor=0x%X\n",
		__FUNCTION__, cdev->span, cdev->chan, CPRIV(cdev)->dev_minor);

	return 0;
}

#define WP_UIO_MAX_SZ 5

static ssize_t wp_cdev_read(struct file *file, char *usrbuf, size_t count, loff_t *ppos)
{
	wanpipe_cdev_t *cdev;
	wan_iovec_t iovstack[WP_UIO_MAX_SZ];
	wan_iovec_t *iov=iovstack;
	wan_msghdr_t msg_sys;
	wan_msghdr_t *msg = (wan_msghdr_t*)usrbuf;
	netskb_t *skb=NULL;
	int err=-EINVAL;
	wp_api_hdr_t hdr;

	memset(&hdr,0,sizeof(wp_api_hdr_t));

	WAN_ASSERT((file==NULL));
	WAN_ASSERT((usrbuf==NULL));

	cdev = file->private_data;
	if (!cdev || !CPRIV(cdev)) {
		return -ENODEV;
	}

	if (count < sizeof(wan_msghdr_t)) {
		DEBUG_ERROR("%s:%d Error: Invalid read buffer size %i\n",__FUNCTION__,__LINE__,count);
		return -EINVAL;
	}

	if (copy_from_user(&msg_sys,msg,sizeof(wan_msghdr_t)))
		return -EFAULT;

	if (msg_sys.msg_iovlen == 0 || msg_sys.msg_iovlen > WP_UIO_MAX_SZ) {
		DEBUG_ERROR("%s:%d Error: Invalid read buffer msg_iovlen %i\n",__FUNCTION__,__LINE__,msg_sys.msg_iovlen);
		return -EFAULT;
	}

	err=wan_verify_iovec(&msg_sys, iov, NULL, 0);
	if (err < 0) {
		return err;
	}

	/* Update the count with length obtained from verify */
	count = err;

	if (cdev->ops.read) {
		err=cdev->ops.read(cdev->dev_ptr, &skb, &hdr, count);
	}

	if (!skb) {
		err = wan_memcpy_toiovec(msg_sys.msg_iov,
					 			(void*)&hdr,
					 			sizeof(hdr));
		return -ENOBUFS;
	}

	if (err == 0) {
		err = wan_memcpy_toiovec(msg_sys.msg_iov,
					 wan_skb_data(skb),
					 wan_skb_len(skb));

		if (err == 0) {
			err=wan_skb_len(skb);
		}

		wan_skb_free(skb);
	} else {
		err = wan_memcpy_toiovec(msg_sys.msg_iov,
					 			(void*)&hdr,
					 			sizeof(hdr));
	}

	return err;
}

static ssize_t wp_cdev_write(struct file *file, const char *usrbuf, size_t count, loff_t *ppos)
{
	wanpipe_cdev_t *cdev;
	wan_iovec_t iovstack[WP_UIO_MAX_SZ];
	wan_iovec_t *iov=iovstack;
	wan_msghdr_t msg_sys;
	wan_msghdr_t msg_sys1;
	wan_msghdr_t *msg = (wan_msghdr_t*)usrbuf;
	netskb_t *skb=NULL;
	unsigned char* buf;
	int err=-EINVAL;
	wp_api_hdr_t hdr;

	memset(&hdr,0,sizeof(wp_api_hdr_t));

	WAN_ASSERT((file==NULL));
	WAN_ASSERT((usrbuf==NULL));
	

	cdev = file->private_data;
	if (!cdev || !CPRIV(cdev)) {
		return -ENODEV;
	}

	if (copy_from_user(&msg_sys,msg,sizeof(wan_msghdr_t)))
		return -EFAULT;

	if (copy_from_user(&msg_sys1,msg,sizeof(wan_msghdr_t)))
		return -EFAULT;

	if (msg_sys.msg_iovlen > WP_UIO_MAX_SZ)
		return -EFAULT;


	err=wan_verify_iovec(&msg_sys, iov, NULL, 0);
	if (err < 0) {
		return err;
	}

	
	/* Update the count with length obtained from verify */
	count = err;

	skb=wan_skb_alloc(count+128);
	if (!skb) {
		return -ENOMEM;
	}

	buf = skb_put(skb,count);
	err = wan_memcpy_fromiovec(buf, msg_sys1.msg_iov, count);
	if (err){
		wan_skb_free(skb);
		return -ENOMEM;
	}

	if (cdev->ops.write) {

		memcpy(&hdr,buf,sizeof(hdr));

		DEBUG_TEST("B4 CDEV WRITE TXDATA=%i  STATUS=%i Size=%i IOVEC ERR=%i\n",
				hdr.wp_api_hdr_data_length,hdr.wp_api_hdr_operation_status,sizeof(hdr),err);

		err=cdev->ops.write(cdev->dev_ptr, skb, &hdr);
		if (err != 0) {
			wan_skb_free(skb);
			if (err == 1) {
				err = -EBUSY;
			}
		} else {
			err=count;
		}

		/* Copy the header back to the user */
		wan_memcpy_toiovec(msg_sys.msg_iov, (void*)&hdr, sizeof(hdr));

		DEBUG_TEST("CDEV WRITE TXDATA=%i  STATUS=%i Size=%i IOVEC ERR=%i\n",
				hdr.wp_api_hdr_data_length,hdr.wp_api_hdr_operation_status,sizeof(hdr),err);

	}

	return err;
}


static WAN_IOCTL_RET_TYPE WANDEF_IOCTL_FUNC(wp_cdev_ioctl, struct file *file, unsigned int cmd, unsigned long data)
{
	wanpipe_cdev_t *cdev;
	WAN_IOCTL_RET_TYPE err=-EINVAL;

	WAN_ASSERT((file==NULL));

	cdev = file->private_data;
	if (!cdev || !CPRIV(cdev)) {
		return -ENODEV;
	}

	if (cdev->ops.ioctl) {
		err=cdev->ops.ioctl(cdev->dev_ptr,cmd, (void*)data);
	}

	return err;
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,18) 
static long wp_cdev_compat_ioctl(struct file *file, unsigned int cmd, unsigned long data)
{
#ifdef HAVE_UNLOCKED_IOCTL
    long err = (long) wp_cdev_ioctl(file, cmd, data);
#else
    long err = (long) wp_cdev_ioctl(NULL, file, cmd, data);
#endif
	return err;
}
#endif

static unsigned int wp_cdev_poll(struct file *file, struct poll_table_struct *wait_table)
{
	int status=0;
	wanpipe_cdev_t *cdev;

	cdev = file->private_data;
	if (!cdev || !CPRIV(cdev)) {
		return -ENODEV;
	}

	poll_wait(file, &CPRIV(cdev)->poll_wait, wait_table);
	status = 0;

	if (cdev->ops.poll) {
		status=cdev->ops.poll(cdev->dev_ptr);
	}

	return status;
}

static int wan_verify_iovec(wan_msghdr_t *m, wan_iovec_t *iov, char *address, int mode)
{
	int size, err, ct;
	if (m->msg_iovlen == 0) {
		return -EMSGSIZE;
	}

	size = m->msg_iovlen * sizeof(wan_iovec_t);

	if (copy_from_user(iov, m->msg_iov, size))
		return -EFAULT;

	m->msg_iov = iov;
	err = 0;

	for (ct = 0; ct < m->msg_iovlen; ct++) {
		err += iov[ct].iov_len;
		/*
		* Goal is not to verify user data, but to prevent returning
		* negative value, which is interpreted as errno.
		* Overflow is still possible, but it is harmless.
		*/
		if (err < 0)
			return -EMSGSIZE;
	}
	
	return err;
}

static int wan_memcpy_fromiovec(unsigned char *kdata, wan_iovec_t *iov, int len)
{
	while (len > 0) {
		if (iov->iov_len) {
			int copy = min_t(unsigned int, len, iov->iov_len);
			if (copy_from_user(kdata, iov->iov_base, copy))
				return -EFAULT;
			len -= copy;
			kdata += copy;
			iov->iov_base += copy;
			iov->iov_len -= copy;
		}
		iov++;
	}
	return 0;
}

static int wan_memcpy_toiovec(wan_iovec_t *iov, unsigned char *kdata, int len)
{
	while (len > 0) {
		if (iov->iov_len) {
			int copy = min_t(unsigned int, iov->iov_len, len);
			if (copy_to_user(iov->iov_base, kdata, copy))
				return -EFAULT;
			kdata += copy;
			len -= copy;
			iov->iov_len -= copy;
			iov->iov_base += copy;
		}
		iov++;
	}
	return 0;
}

int wanpipe_sys_dev_add(struct device *dev, struct device *parent, char *name)
{
	device_initialize(dev);
	dev->parent = parent;
	dev->bus = &wanpipe_device_bus;
	wp_dev_set_name(dev,"%s",name);
	return device_add(dev);
}

void wanpipe_sys_dev_del(struct device *dev)
{
	device_del(dev);
}


EXPORT_SYMBOL(wanpipe_sys_dev_add);
EXPORT_SYMBOL(wanpipe_sys_dev_del);

EXPORT_SYMBOL(wanpipe_global_cdev_init);
EXPORT_SYMBOL(wanpipe_global_cdev_free);

EXPORT_SYMBOL(wanpipe_cdev_free);

EXPORT_SYMBOL(wanpipe_cdev_tx_wake);
EXPORT_SYMBOL(wanpipe_cdev_rx_wake);
EXPORT_SYMBOL(wanpipe_cdev_event_wake);
EXPORT_SYMBOL(wanpipe_cdev_tdm_create);
EXPORT_SYMBOL(wanpipe_cdev_tdm_ctrl_create);
EXPORT_SYMBOL(wanpipe_cdev_cfg_ctrl_create);
EXPORT_SYMBOL(wanpipe_cdev_timer_create);
EXPORT_SYMBOL(wanpipe_cdev_logger_create);


