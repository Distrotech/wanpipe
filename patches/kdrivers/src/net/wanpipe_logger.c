/*****************************************************************************
* wanpipe_logger.c
*
* 		WANPIPE(tm) - Event Logger API Support
*
* Authors: 	David Rokhvarg <davidr@sangoma.com>
*
* Copyright:	(c) 2003-2009 Sangoma Technologies Inc.
*
* ============================================================================
* September 04, 2009	David Rokhvarg	Initial version.
*****************************************************************************/

#include "wanpipe_includes.h"
#include "wanpipe_cdev_iface.h"
#include "wanpipe_logger.h"

/*=================================================================
 * Debugging Definitions
 *================================================================*/

#if defined(__WINDOWS__)
# define DEBUG_LOGGER if(0)DbgPrint
extern void vOutputLogString(const char * pvFormat, va_list args);
#else
# define DEBUG_LOGGER if(0)printk
#endif

#undef LOGGER_FUNC_DEBUG
#if 0
# define LOGGER_FUNC_DEBUG()  DEBUG_LOGGER("%s():Line:%d\n", __FUNCTION__, __LINE__)
#else
# define LOGGER_FUNC_DEBUG()
#endif

#define LOGGER_QLEN_DBG(queue)	\
if(0){	\
	DEBUG_LOGGER("%s(): queue ptr: 0x%p (%s), qlen:%d\n",	\
		__FUNCTION__,	\
		queue, \
		(queue == &logger_api_dev.wp_event_list ? "wp_event_list":"wp_event_free_list"),	\
		wan_skb_queue_len(queue));	\
}

#define WAN_MESSAGE_DISCARD_COUNT 1000

/*=================================================================
 * Macro Definitions
 *================================================================*/


#define WAN_LOGGER_SET_DATA(logger_event, logger_type, evt_type, format, va_arg_list)	\
{	\
	\
	memset(logger_event, 0x00, sizeof(*logger_event));	\
	\
	logger_event->logger_type = logger_type;	\
	logger_event->event_type = evt_type;	\
	\
	wp_vsnprintf(logger_event->data, sizeof(logger_event->data), format, va_arg_list);	\
	\
	WAN_LOGGER_SET_TIME_STAMP(logger_event)	\
}

#define WAN_LOGGER_SET_TIME_STAMP(logger_event)	\
{	\
	struct timeval tv;	\
	\
	do_gettimeofday(&tv);	\
	\
	logger_event->time_stamp_sec = tv.tv_sec;	\
	logger_event->time_stamp_usec = tv.tv_usec;	\
}


/*=================================================================
 * Static Defines
 *================================================================*/

typedef struct wp_logger_api_dev {

	u32 	init;
	char 	name[WAN_IFNAME_SZ];

	wan_spinlock_t lock;

	u32	used, open_cnt;

	wanpipe_api_dev_cfg_t	cfg;

	wan_skb_queue_t 	wp_event_list;
	wan_skb_queue_t 	wp_event_free_list;

	void 			*cdev;
	u32				magic_no;

} wp_logger_api_dev_t;


/*=================================================================
 * Function Prototypes and Global Variables
 *================================================================*/

u_int32_t wp_logger_level_default = SANG_LOGGER_INFORMATION | SANG_LOGGER_WARNING | SANG_LOGGER_ERROR;
u_int32_t wp_logger_level_te1 = 0;
u_int32_t wp_logger_level_hwec = 0;
u_int32_t wp_logger_level_tdmapi = 0;
u_int32_t wp_logger_level_fe = 0;
u_int32_t wp_logger_level_bri = 0;

/* by default messages will be printed into Log file. */
static u_int32_t wp_logger_level_file = SANG_LOGGER_FILE_ON;


static wp_logger_api_dev_t	logger_api_dev;
static wanpipe_cdev_ops_t	wp_logger_api_fops;

static void __wp_logger_input(u_int32_t logger_type, u_int32_t evt_type, const char * fmt, ...);

/*=================================================================
 * Private Functions
 *================================================================*/

static u_int32_t* wp_logger_type_to_variable_ptr(u_int32_t logger_type)
{
	switch(logger_type)
	{
	case WAN_LOGGER_DEFAULT:
		return &wp_logger_level_default;
	case WAN_LOGGER_TE1:
		return &wp_logger_level_te1;
	case WAN_LOGGER_HWEC:
		return &wp_logger_level_hwec;
	case WAN_LOGGER_TDMAPI:
		return &wp_logger_level_tdmapi;
	case WAN_LOGGER_FE:
		return &wp_logger_level_fe;
	case WAN_LOGGER_BRI:
		return &wp_logger_level_bri;
	case WAN_LOGGER_FILE:
		return &wp_logger_level_file;
	default:
		return NULL;
	}
}

static void wp_logger_spin_lock_irq(wan_smp_flag_t *flags)
{
	wan_spin_lock_irq(&logger_api_dev.lock, flags);
}

static void wp_logger_spin_unlock_irq(wan_smp_flag_t *flags)
{
	wan_spin_unlock_irq(&logger_api_dev.lock, flags);
}

static int wp_logger_increment_open_cnt(void)
{
	logger_api_dev.open_cnt++;
	return logger_api_dev.open_cnt;
}

static int wp_logger_decrement_open_cnt(void)
{
	logger_api_dev.open_cnt--;
	return logger_api_dev.open_cnt;
}

static int wp_logger_get_open_cnt(void)
{
	return logger_api_dev.open_cnt;
}

/* set logger level bitmap */
static void wp_logger_set_level(u_int32_t logger_type, u_int32_t new_level)
{
	u_int32_t *logger_var_ptr = wp_logger_type_to_variable_ptr(logger_type);

	DEBUG_LOGGER("%s(): logger_type: %d(%s), new_level: 0x%08X\n",
		__FUNCTION__, logger_type, SANG_DECODE_LOGGER_TYPE(logger_type),
		new_level);

	if(logger_var_ptr){
		*logger_var_ptr = new_level;
	}else{
		DEBUG_WARNING("Warning: %s(): unknown logger_type: %d\n",
			__FUNCTION__, logger_type);
	}
}


/* return current logger level bitmap to the caller */
static u_int32_t wp_logger_get_level(u_int32_t logger_type)
{
	u_int32_t *logger_var_ptr = wp_logger_type_to_variable_ptr(logger_type);

	if(logger_var_ptr){
		DEBUG_LOGGER("%s(): logger_type: %d, current level: 0x%08X\n", 
			__FUNCTION__, logger_type, *logger_var_ptr);

		return *logger_var_ptr;
	}else{
		DEBUG_WARNING("Warning: %s(): unknown logger_type: %d\n",
			__FUNCTION__, logger_type);
		return 0;
	}
}

static void wp_logger_init_buffs(void)
{
	netskb_t *skb;
	wan_smp_flag_t irq_flag;

	wp_logger_spin_lock_irq(&irq_flag);

	while((skb = wan_skb_dequeue(&logger_api_dev.wp_event_list))) {
		wan_skb_init(skb,0);
		wan_skb_trim(skb,0);
		wan_skb_queue_tail(&logger_api_dev.wp_event_free_list, skb);
	}

	wp_logger_spin_unlock_irq(&irq_flag);
}

static int wp_logger_api_open(void *not_used)
{
	if (wp_logger_increment_open_cnt() == 1) {
		wp_logger_init_buffs();
	}

	wan_set_bit(0,&logger_api_dev.used);

	DEBUG_LOGGER ("%s(): OpenCounter: %d\n",
		__FUNCTION__, wp_logger_get_open_cnt());
	return 0;
}

static void wp_logger_wakeup_api(void)
{
	if (logger_api_dev.cdev) {
		wanpipe_cdev_event_wake(logger_api_dev.cdev);
	}
}

static int wp_logger_api_release(void *not_used)
{
	u32 cnt;

	if (!wan_test_bit(0, &logger_api_dev.init)) {
		wan_clear_bit(0, &logger_api_dev.used);
		return -ENODEV;
	}

	cnt = wp_logger_decrement_open_cnt();

	if (cnt == 0) {

		wan_clear_bit(0, &logger_api_dev.used);

		wp_logger_wakeup_api();
	}

	return 0;
}

static int wp_logger_alloc_q(wan_skb_queue_t *queue)
{
	netskb_t *skb;
	int i;
	wan_smp_flag_t irq_flag;

	for (i = 0; i < WP_MAX_NO_LOGGER_EVENTS; i++) {

		skb = wan_skb_kalloc(sizeof(wp_logger_event_t));

		if (!skb) {
			if (WAN_NET_RATELIMIT()) {
				DEBUG_ERROR("Error: %s(): %d: Failed to Allocate Memory!\n",
					__FUNCTION__, __LINE__);
			}
			return -1;
		}

		/*wan_skb_init(skb, sizeof(wp_logger_event_t));*/
		wan_skb_init(skb, 0);
		wan_skb_trim(skb, 0);

		wp_logger_spin_lock_irq(&irq_flag);	
		wan_skb_queue_tail(queue, skb);
		wp_logger_spin_unlock_irq(&irq_flag);
	}

	LOGGER_QLEN_DBG(queue);
	return 0;
}

static void wp_logger_free_q(wan_skb_queue_t *queue)
{
	netskb_t *skb;
	wan_smp_flag_t irq_flag;

	LOGGER_QLEN_DBG(queue);

	do {
		wp_logger_spin_lock_irq(&irq_flag);

		skb = wan_skb_dequeue(queue);

		wp_logger_spin_unlock_irq(&irq_flag);

		if(skb){
			wan_skb_free(skb);
		}
	}while(skb);

}

/* Note that there is no need to check for queue length exceeding
 * some maximum because it is done automatically, when wp_logger_get_free_skb()
 * returns NULL. */
static void wp_logger_enqueue_skb(wan_skb_queue_t *queue, netskb_t *skb)
{
	wan_smp_flag_t irq_flag;

	wp_logger_spin_lock_irq(&irq_flag);
	wan_skb_queue_tail(queue, skb);
	wp_logger_spin_unlock_irq(&irq_flag);
}

static netskb_t* wp_logger_dequeue_skb(wan_skb_queue_t *queue)
{
	netskb_t *skb;
	wan_smp_flag_t irq_flag;

	wp_logger_spin_lock_irq(&irq_flag);
	skb = wan_skb_dequeue(queue);
	wp_logger_spin_unlock_irq(&irq_flag);

	return skb;
}

static unsigned int wp_logger_api_poll(void *not_used)
{
	int ret = 0, qlength;
	wan_smp_flag_t irq_flag;	


	if (!wan_test_bit(0, &logger_api_dev.init) || !wan_test_bit(0, &logger_api_dev.used)){
		return -ENODEV;
	}

	wp_logger_spin_lock_irq(&irq_flag);

	qlength = wan_skb_queue_len(&logger_api_dev.wp_event_list);

	wp_logger_spin_unlock_irq(&irq_flag);

	if (qlength) {
		/* Indicate an exception */
		ret = POLLPRI;
	}

	return ret;	
}

static int wp_logger_handle_api_cmd(void *user_data)
{
	wp_logger_cmd_t tmp_usr_logger_api, *user_logger_api_ptr;
	int err;
	netskb_t *skb;
	wan_smp_flag_t irq_flag;	

	user_logger_api_ptr = (wp_logger_cmd_t*)user_data;

#if defined(__WINDOWS__)
	/* user_data IS a kernel-space pointer to wp_logger_cmd_t */
	memcpy(&tmp_usr_logger_api, user_data, sizeof(wp_logger_cmd_t));
#else
	if(WAN_COPY_FROM_USER(&tmp_usr_logger_api, user_logger_api_ptr, sizeof(wp_logger_cmd_t))){
		return -EFAULT;
	}
#endif

	err = tmp_usr_logger_api.result = SANG_STATUS_SUCCESS;

	DEBUG_LOGGER("%s: Logger CMD: %i, sizeof(wp_logger_cmd_t)=%d\n",
			logger_api_dev.name, tmp_usr_logger_api.cmd, (unsigned int)sizeof(wp_logger_cmd_t));

	switch (tmp_usr_logger_api.cmd) 
	{
	case WP_API_LOGGER_CMD_READ_EVENT:

		skb = wp_logger_dequeue_skb(&logger_api_dev.wp_event_list);
		if (!skb){
			err = SANG_STATUS_NO_FREE_BUFFERS;
			break;
		}
		
		memcpy(&tmp_usr_logger_api.logger_event, wan_skb_data(skb), sizeof(wp_logger_event_t));
	
		/* return the buffer into free skb list (do NOT deallocate it!) */
		wp_logger_enqueue_skb(&logger_api_dev.wp_event_free_list, skb);
		break;
		
	case WP_API_LOGGER_CMD_GET_LOGGER_LEVEL:
		tmp_usr_logger_api.logger_level_ctrl.logger_level =
			wp_logger_get_level(tmp_usr_logger_api.logger_level_ctrl.logger_type);
		break;
		
	case WP_API_LOGGER_CMD_SET_LOGGER_LEVEL:
		wp_logger_set_level(tmp_usr_logger_api.logger_level_ctrl.logger_type, 
						tmp_usr_logger_api.logger_level_ctrl.logger_level);
		break;
		
	case WP_API_LOGGER_CMD_OPEN_CNT:
		tmp_usr_logger_api.open_cnt = wp_logger_get_open_cnt();
		break;

	case WP_API_LOGGER_CMD_GET_STATS:

		wp_logger_spin_lock_irq(&irq_flag);

		tmp_usr_logger_api.stats.max_event_queue_length = wan_skb_queue_len(&logger_api_dev.wp_event_list) + wan_skb_queue_len(&logger_api_dev.wp_event_free_list);
		tmp_usr_logger_api.stats.current_number_of_events_in_event_queue = wan_skb_queue_len(&logger_api_dev.wp_event_list);

		wp_logger_spin_unlock_irq(&irq_flag);

		tmp_usr_logger_api.stats.rx_events_dropped	= logger_api_dev.cfg.stats.rx_events_dropped;
		tmp_usr_logger_api.stats.rx_events 		= logger_api_dev.cfg.stats.rx_events;
		break;

	case WP_API_LOGGER_CMD_RESET_STATS:
		memset(&logger_api_dev.cfg.stats, 0x00, sizeof(logger_api_dev.cfg.stats));
		break;

	case WP_API_LOGGER_CMD_FLUSH_BUFFERS:
		wp_logger_init_buffs();
		break;

	default:
		DEBUG_WARNING("%s: Warning: Invalid Wanpipe Logger API Command: %i\n",
			logger_api_dev.name, tmp_usr_logger_api.cmd);
		err = SANG_STATUS_OPTION_NOT_SUPPORTED;
		break;

	}/* switch (tmp_usr_logger_api.cmd)  */

	tmp_usr_logger_api.result = err;

#if defined(__WINDOWS__)
	/* user_data IS a kernel-space pointer to wp_logger_cmd_t */
	memcpy(user_data, &tmp_usr_logger_api, sizeof(wp_logger_cmd_t));
#else
	if(WAN_COPY_TO_USER(user_data, &tmp_usr_logger_api, sizeof(wp_logger_cmd_t))){
	    err = -EFAULT;
	}
#endif

	return err;
}

static int wp_logger_api_ioctl(void *not_used, int ioctl_cmd, void *user_data)
{
	int err = 0;

	if (!wan_test_bit(0,&logger_api_dev.init) || WANPIPE_MAGIC != logger_api_dev.magic_no){
		DEBUG_ERROR("Error: %s(): line: %d: Logger API Not initialized!\n",
			__FUNCTION__, __LINE__);
		return -EFAULT;
	}

	if (!user_data){
		return -EINVAL;
	}

	switch (ioctl_cmd) 
	{
	case WANPIPE_IOCTL_LOGGER_CMD:
		err = wp_logger_handle_api_cmd(user_data);
		break;

	default:
		DEBUG_ERROR("Error: %s(): line: %d: Invalid IOCTL CMD: %d!\n",
			__FUNCTION__, __LINE__, ioctl_cmd);
		err = -EINVAL;
		break;
	}	

	return err;
}

static netskb_t* wp_logger_get_free_skb(void)
{
	netskb_t *skb;

	skb = wp_logger_dequeue_skb(&logger_api_dev.wp_event_free_list);

	if (skb == NULL) {
		DEBUG_ERROR("Error: %s(): no free buffers. Dropping Logger Event!\n",
			__FUNCTION__);
		WP_AFT_CHAN_ERROR_STATS(logger_api_dev.cfg.stats, rx_events_dropped);
		return NULL;
	}

	return skb;
}

static int wp_logger_push_event(netskb_t *skb)
{
	if (!wan_test_bit(0,&logger_api_dev.init) || WANPIPE_MAGIC != logger_api_dev.magic_no){
		DEBUG_TEST("%s(): Initialization incomplete. Dropping event.\n", __FUNCTION__);
		return -ENODEV;
	}

	if (!skb) {
		DEBUG_ERROR("Error: Logger: Null skb argument in %s()!\n", __FUNCTION__);
		return -ENODEV;
	}

	if (!wan_test_bit(0,&logger_api_dev.used)) {
		DEBUG_TEST("%s(): Not in use. Dropping event.\n", __FUNCTION__);
		return -EBUSY;
	}

	wp_logger_enqueue_skb(&logger_api_dev.wp_event_list, skb);

	logger_api_dev.cfg.stats.rx_events++;

	return 0;
}

static char previous_error_message[WP_MAX_NO_BYTES_IN_LOGGER_EVENT];
static u32	repeating_error_message_counter = 0;


static int wp_logger_repeating_message_filter(u_int32_t logger_type, u_int32_t evt_type, const char * fmt, va_list *va_arg_list)
{
	char current_error_message[WP_MAX_NO_BYTES_IN_LOGGER_EVENT];
	char tmp_message_buf[WP_MAX_NO_BYTES_IN_LOGGER_EVENT];

	/* Filter out repeating messages. */
	u8	apply_filter = 0;

	/* Filter only ERROR message from Default logger. 
	 * Other types of message can be filtered too. */
	if(WAN_LOGGER_DEFAULT == logger_type){
 		if(SANG_LOGGER_ERROR == evt_type){
			apply_filter = 1;
		}
	}

	if(!apply_filter){
		/* consider it as NOT a repeating message */
		previous_error_message[0]=0;
		repeating_error_message_counter = 0;
		return 0;
	}

	memset(current_error_message, 0x00, sizeof(current_error_message));

	wp_vsnprintf(current_error_message, sizeof(current_error_message) - 1,
		(const char *)fmt, *va_arg_list);

	if(!memcmp(previous_error_message, current_error_message, sizeof(current_error_message))){
		/* Every WAN_MESSAGE_DISCARD_COUNT messages print the repeating message
		 * and the counter how many times it was repeated. */

		if(!(repeating_error_message_counter % WAN_MESSAGE_DISCARD_COUNT)){
		
			if (repeating_error_message_counter) {

				/* print the repeating message */
				__wp_logger_input(logger_type, evt_type, "%s", current_error_message);

				/* and say how many times it was repeated. */
				wp_snprintf(tmp_message_buf, sizeof(tmp_message_buf),
					"* Message repeated %d times.\n", repeating_error_message_counter);
				__wp_logger_input(logger_type, evt_type, "%s", tmp_message_buf);
			}
		}

		++repeating_error_message_counter;

		/* is IS a repeating message */
		return 1;

	}else{

		if(repeating_error_message_counter){

			/* this message broke a sequence of repeating messages */
			wp_snprintf(tmp_message_buf, sizeof(tmp_message_buf),
				"* Message repeated %d times.\n", repeating_error_message_counter);
			__wp_logger_input(logger_type, evt_type, "%s", tmp_message_buf);
			repeating_error_message_counter = 0;
		}

		/* store current message for comparison with a future message */
		memcpy(previous_error_message, current_error_message, sizeof(current_error_message));

		/* it is NOT a repeating message */
		return 0;
	}
}

/* no Filtering will be applied to the message */
static void wp_logger_vInput(u_int32_t logger_type, u_int32_t evt_type, const char * fmt, va_list *va_arg_list)
{
	/************************************************************************
	  Independently of Logger Device state, write the message into log file. 
	*************************************************************************/

	if (!(wp_logger_level_file & SANG_LOGGER_FILE_OFF)) {

 #ifdef __WINDOWS__
		/* Write the message to wanpipelog.txt */
		vOutputLogString(fmt, *va_arg_list);
 #else
 		{
			char msg[WP_MAX_NO_BYTES_IN_LOGGER_EVENT];
			wp_vsnprintf(msg, sizeof(msg) - 1,(const char *)fmt, *va_arg_list);
			switch (evt_type) {
			case SANG_LOGGER_ERROR:
				printk(KERN_ERR "%s", msg);
				break;
			case SANG_LOGGER_WARNING:
				printk(KERN_WARNING "%s", msg);
				break;
			case SANG_LOGGER_INFORMATION:
			default:
#ifdef WAN_DEBUG_EVENT_AS_KERN_DEBUG
				printk(KERN_DEBUG "%s", msg);
#else
				printk(KERN_INFO "%s", msg);
#endif
				break;
			}
		}
 #endif
	}

	/************************************************************************
	 Push the message into logger queue.
	*************************************************************************/

	if (WANPIPE_MAGIC == logger_api_dev.magic_no	&&
		wan_test_bit(0, &logger_api_dev.used)		&&
		wan_test_bit(0, &logger_api_dev.init)){

		wp_logger_event_t *p_wp_logger_event;
		netskb_t *skb;

		skb = wp_logger_get_free_skb();
		if(skb){
			wan_skb_init(skb,0);
			wan_skb_trim(skb,0);
			
			/* since we have the free skb buffer we should copy data into it
			* and push it into event queue. */
			p_wp_logger_event = (wp_logger_event_t*)wan_skb_put(skb, sizeof(wp_logger_event_t));
			
			memset(p_wp_logger_event, 0, sizeof(wp_logger_event_t));
			
			WAN_LOGGER_SET_DATA(p_wp_logger_event, logger_type, evt_type, fmt, *va_arg_list);
			
			wp_logger_push_event(skb);

			wp_logger_wakeup_api();

#ifdef AFT_TASKQ_DEBUG
			DEBUG_TASKQ("%s():%d trigger executed!\n",
					__FUNCTION__,__LINE__);
#endif
		}
	}
}

static void __wp_logger_input(u_int32_t logger_type, u_int32_t evt_type, const char * fmt, ...)
{
    va_list	va_arg_list;

	/* the paramter list must start at fmt, not at evt_type */
    va_start (va_arg_list, fmt);

	wp_logger_vInput(logger_type, evt_type, fmt, &va_arg_list);

    va_end (va_arg_list);
}

/*=================================================================
 * Public Functions
 *================================================================*/

/* Windows note: sngbus.sys, sprotocol.sys and wanpipe.sys 
 * will NOT initialize/use Logger Device, but writing to wanpipelog.txt
 * file is still possible by calling this function.*/
void wp_logger_input(u_int32_t logger_type, u_int32_t evt_type, const char * fmt, ...)
{
	va_list	va_arg_list;

	/* Linux 64bit: Note that va_arg_list is started/ended twice -
	 * before each call to functions which take it as a parameter.
	 */

	/* the paramter list must start at fmt, not at evt_type */
	va_start (va_arg_list, fmt);
	if (wp_logger_repeating_message_filter(logger_type, evt_type, fmt, &va_arg_list)) {
		va_end (va_arg_list);
		return;
	}
	va_end (va_arg_list);

    va_start (va_arg_list, fmt);
    wp_logger_vInput(logger_type, evt_type, fmt, &va_arg_list);
	va_end (va_arg_list);
}

/* This function runs during module load - the 'logger_api_dev' structure
 * is initialized for the first time. */
int wp_logger_create(void)
{
	wanpipe_cdev_t *cdev;
	int err;
		
	if(WANPIPE_MAGIC == logger_api_dev.magic_no){
		DEBUG_LOGGER("%s(): Logger Dev already exist - not creating\n", __FUNCTION__);
		/* Already initialized. */
		return 0;
	}

	memset(&logger_api_dev, 0, sizeof(wp_logger_api_dev_t));
	logger_api_dev.magic_no = WANPIPE_MAGIC;

	snprintf(logger_api_dev.name, sizeof(logger_api_dev.name), "WP Logger");

	wp_logger_api_fops.open		= wp_logger_api_open;
	wp_logger_api_fops.close	= wp_logger_api_release;
	wp_logger_api_fops.ioctl	= wp_logger_api_ioctl;
	wp_logger_api_fops.read		= NULL;
	wp_logger_api_fops.write	= NULL;
	wp_logger_api_fops.poll		= wp_logger_api_poll;
	
	cdev = wan_kmalloc(sizeof(wanpipe_cdev_t));
	if (!cdev) {
		return -ENOMEM;
	}
	memset(cdev, 0, sizeof(wanpipe_cdev_t));
	
	DEBUG_LOGGER("%s(): Registering Wanpipe Logger Device!\n",__FUNCTION__);
	
	wan_skb_queue_init(&logger_api_dev.wp_event_list);
	wan_skb_queue_init(&logger_api_dev.wp_event_free_list);
	
	wan_spin_lock_init(&logger_api_dev.lock, "wp_logger_lock");

	if(wp_logger_alloc_q(&logger_api_dev.wp_event_free_list)){
		logger_api_dev.magic_no = 0;
		return -ENOMEM;
	}
		
	cdev->dev_ptr = &logger_api_dev;

	logger_api_dev.cdev = cdev;
	
	memcpy(&cdev->ops, &wp_logger_api_fops, sizeof(cdev->ops));
	
	err = wanpipe_cdev_logger_create(cdev);

	if(err){
		logger_api_dev.magic_no = 0;
		return err;
	}

	wan_set_bit(0,&logger_api_dev.init);

	return 0;
}

/* This function called during module unload. */
void wp_logger_delete(void)
{
	if(wan_test_bit(0,&logger_api_dev.used)){
		DEBUG_LOGGER("%s(): Logger Dev in use - not deleting\n", __FUNCTION__);
		/* If open file descriptor exist, deletion of Logger Device
		 * will fail (under Windows). */
		return;
	}

	if(WANPIPE_MAGIC != logger_api_dev.magic_no){
		/* make sure logger deleted exactly one time */
		return;
	}

	logger_api_dev.magic_no = 0;
	wan_clear_bit(0, &logger_api_dev.init);
	
	wp_logger_free_q(&logger_api_dev.wp_event_list);
	wp_logger_free_q(&logger_api_dev.wp_event_free_list);

	if (logger_api_dev.cdev) {
		wanpipe_cdev_free(logger_api_dev.cdev);
		wan_free(logger_api_dev.cdev);
		logger_api_dev.cdev=NULL;
	}
}

#if defined(__LINUX__)
EXPORT_SYMBOL(wp_logger_level_default);
EXPORT_SYMBOL(wp_logger_level_te1);
EXPORT_SYMBOL(wp_logger_level_hwec);
EXPORT_SYMBOL(wp_logger_level_tdmapi);
EXPORT_SYMBOL(wp_logger_level_fe);
EXPORT_SYMBOL(wp_logger_level_bri);
EXPORT_SYMBOL(wp_logger_input);
#endif
/****** End ****************************************************************/
