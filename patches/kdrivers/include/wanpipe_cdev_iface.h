/* wanpipe_cdev_iface.h */

#ifndef __WANPIPE_CDEV_IFACE__
#define __WANPIPE_CDEV_IFACE__



#include "wanpipe_includes.h"
#include "wanpipe_defines.h"
#include "wanpipe_debug.h"
#include "wanpipe_common.h"
#include "wanpipe_cfg.h"
#include "wanpipe.h"
#include "wanpipe_api.h"

#if defined(WAN_KERNEL)

typedef struct wanpipe_cdev_ops
{
	int (*open)(void* dev_ptr);
	int (*close)(void* dev_ptr);
	int (*ioctl)(void* dev_ptr, int cmd, void *arg);

	u_int32_t (*poll)(void *dev_ptr);

//	int (*write)(void* dev_ptr, u_int8_t *hdr, u_int32_t hdr_len, u_int8_t *data, u_int32_t data_len);
//	int (*read)(void* dev_ptr, u_int8_t *hdr, u_int32_t hdr_len, u_int8_t *data, u_int32_t data_len);

	int (*write)(void* dev_ptr, netskb_t *skb,  wp_api_hdr_t *hdr);
	int (*read)(void* dev_ptr,  netskb_t **skb,  wp_api_hdr_t *hdr, int count);

	/* handle transmission time out */
	int (*tx_timeout)(void* dev_ptr);

}wanpipe_cdev_ops_t;



/*
#if defined(__WINDOWS__)
typedef struct file_operations
{
	int (*open)(void *inode, void *file);
	int (*wp_cdev_release)(void *inode, void *file);
	int (*ioctl)(void *inode, void *file, unsigned int cmd, unsigned long data);
	int (*read)(void *file, char *usrbuf, size_t count, void *ppos);
	int (*write)(void *file, const char *usrbuf, size_t count, void *ppos);
	int (*poll)(void *file, void *wait_table);

}file_operations_t;
#endif
*/

enum WP_TDM_OPMODE{
	WP_TDM_OPMODE_CHAN = 0,
	WP_TDM_OPMODE_SPAN,
	WP_TDM_OPMODE_MTP1
};

enum {
	WP_TDM_API_MODE_CURRENT = 0,
	WP_TDM_API_MODE_LEGACY_WIN_API
};

#define CDEV_WIN_LEGACY_MODE(cdev) (cdev->operation_mode==WP_TDM_API_MODE_LEGACY_WIN_API)

typedef struct wanpipe_cdev
{
	wan_bitmap_t init;
	wan_bitmap_t used;
	
	int32_t	 span;
	int32_t	 chan;
	u_int8_t name[WAN_IFNAME_SZ+1];

	void 	*dev_ptr;
	wanpipe_cdev_ops_t ops;

	u8		operation_mode;/* WP_TDM_OPMODE */

 	wan_timer_t	tx_timeout_timer;

	/* Private to cdev code - Do not use*/
	void *priv;

#if defined(__WINDOWS__)
	void *PnpFdoPdx;
#endif

} wanpipe_cdev_t;



#if defined (__LINUX__) && defined(__KERNEL__)

typedef struct wanpipe_cdev_priv
{
	int dev_minor;
	spinlock_t lock;
	wait_queue_head_t poll_wait;

}wanpipe_cdev_priv_t;

# define CPRIV(dev)  ((wanpipe_cdev_priv_t*)(dev->priv))

static __inline int  wanpipe_cdev_rx_wake(wanpipe_cdev_t *cdev)
{
	if (!cdev || !CPRIV(cdev)) {
		DEBUG_EVENT("%s(): Error cdev->dev_ptr not initialized!\n",__FUNCTION__);
		return -1;
	}

	if (waitqueue_active(&CPRIV(cdev)->poll_wait)){
		wake_up_interruptible(&CPRIV(cdev)->poll_wait);
	}

	return 0;
}


static __inline int wanpipe_cdev_tx_wake(wanpipe_cdev_t *cdev)
{
	if (!cdev || !CPRIV(cdev)) {
		DEBUG_EVENT("%s(): Error cdev->dev_ptr not initialized!\n",__FUNCTION__);
		return -1;
	}

	if (waitqueue_active(&CPRIV(cdev)->poll_wait)){
		wake_up_interruptible(&CPRIV(cdev)->poll_wait);
	}

	return 0;
}

static __inline int  wanpipe_cdev_event_wake(wanpipe_cdev_t *cdev)
{
	if (!cdev || !CPRIV(cdev)) {
		DEBUG_EVENT("%s(): Error cdev->dev_ptr not initialized!\n",__FUNCTION__);
		return -1;
	}

	if (waitqueue_active(&CPRIV(cdev)->poll_wait)){
		wake_up_interruptible(&CPRIV(cdev)->poll_wait);
	}

	return 0;
}
#else
int wanpipe_cdev_tx_wake(wanpipe_cdev_t *cdev);
int wanpipe_cdev_rx_wake(wanpipe_cdev_t *cdev);
int wanpipe_cdev_event_wake(wanpipe_cdev_t *cdev);
#endif

/*=================================================
 * Public Interface Functions
 *================================================*/
int wanpipe_global_cdev_init(void);
int wanpipe_global_cdev_free(void);

int wanpipe_cdev_tdm_create(wanpipe_cdev_t *cdev);
int wanpipe_cdev_free(wanpipe_cdev_t *cdev);
int wanpipe_cdev_lip_api_create(wanpipe_cdev_t *cdev);


int wanpipe_cdev_tdm_ctrl_create(wanpipe_cdev_t *cdev);
int wanpipe_cdev_cfg_ctrl_create(wanpipe_cdev_t *cdev);
int wanpipe_cdev_timer_create(wanpipe_cdev_t *cdev);
int wanpipe_cdev_logger_create(wanpipe_cdev_t *cdev);

int wp_ctrl_dev_create(void);
void wp_ctrl_dev_delete(void);

int wanpipe_sys_dev_add(struct device *dev, struct device *parent, char *name);
void wanpipe_sys_dev_del(struct device *dev);

#endif

#endif
