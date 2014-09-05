/* TTY Call backs */


#include "wanpipe_version.h"
#include "wanpipe_includes.h"
#include "wanpipe_defines.h"
#include "wanpipe_abstr.h"
#include "wanpipe.h"
#include "if_wanpipe_common.h"
#include "sdla_adsl.h"

#if defined(__LINUX__)
#include "if_wanpipe.h"
#include "wanpipe_syncppp.h"
#endif


#if defined(__WINDOWS__)
#include <sdladrv_private.h>
#endif


#if defined(__LINUX__)
# define  STATIC	static
#else
# define  STATIC
#endif

#define ADSL_WAN_TTY_VERSION	1

#ifndef ADSL_TTY_MAJOR
#define ADSL_TTY_MAJOR 		240
#endif

#define ADSL_WAN_MAX_CHANNELS   1
#define ADSL_WAN_TTY_NAME  	"ttyWP"

#ifndef ADSL_NR_PORTS
# define ADSL_NR_PORTS 		4
#else
# if ADSL_NR_PORTS > 16
#  undef ADSL_NR_PORTS
#  define ADSL_NR_PORTS 16
# endif
#endif

/* FIXME Redefined in gpwan.c */
#define GSI_WAN_MAX_CHANNELS 	1

#if defined(__LINUX__)
static int adsl_wan_tx(ttystruct_t*, int, const unsigned char*, int);
static int adsl_wan_open(ttystruct_t  *tp, struct file *pFile);
static void adsl_wan_close(ttystruct_t  *tp, struct file *pFile);

static ttydriver_t serial_driver;
static int serial_refcount=1;
static int tty_init_cnt=0;

static struct tty_struct *serial_table[ADSL_NR_PORTS];
static struct termios *serial_termios[ADSL_NR_PORTS];
static struct termios *serial_termios_locked[ADSL_NR_PORTS];
static void* tty_card_map[ADSL_NR_PORTS] = {NULL,NULL,NULL,NULL};

STATIC void adsl_wan_flush_chars(ttystruct_t *tp);
STATIC void adsl_wan_flush_buffer(ttystruct_t *tp);
STATIC int adsl_wan_chars_in_buffer(ttystruct_t  *tp);
STATIC void adsl_wan_set_termios(ttystruct_t *tp, struct termios *old_termios);
STATIC int adsl_wan_write_room(ttystruct_t *tp);

unsigned int  adsl_get_tty_minor(void *tty_ptr);
unsigned int adsl_get_tty_minor_start(void *tty_ptr);
void adsl_mod_inc_use_count (void);
void adsl_mod_dec_use_count (void);
void adsl_set_tty_driver_data(void *tty_ptr, void *ptr);
void adsl_set_low_latency(void *tty_ptr, unsigned char data);
void *adsl_get_tty_driver_state(void *tty_ptr);

STATIC void adsl_wan_flush_chars(ttystruct_t *tp)
{
	return;
}

STATIC void adsl_wan_flush_buffer(ttystruct_t *tp)
{
	if (!tp){
		return;
	}

	wake_up_interruptible(&tp->write_wait);
#if defined(SERIAL_HAVE_POLL_WAIT) || \
         (defined LINUX_2_1 && LINUX_VERSION_CODE >= KERNEL_VERSION(2,2,15))
	wake_up_interruptible(&tp->poll_wait);
#endif
	if ((tp->flags & (1 << TTY_DO_WRITE_WAKEUP)) &&
	    tp->ldisc.write_wakeup)
		(tp->ldisc.write_wakeup)(tp);
	return;
}

/*+F*************************************************************************
 * Function:
 *   adsl_wan_chars_in_buffer()
 *
 * Description:
 *   Return the number of bytes waiting to be transmitted out the adapter.
 *-F*************************************************************************/
STATIC int adsl_wan_chars_in_buffer(ttystruct_t  *tp)
{
	return 0;
}


STATIC void adsl_wan_set_termios(ttystruct_t *tp, struct termios *old_termios)
{
	return;
}

STATIC int adsl_wan_write_room(ttystruct_t *tp)
{
	if (!tp)
		return 0;

	if (!tp->driver_data)
		return 0;

	return GpWanWriteRoom(tp->driver_data);
}


/*+F*************************************************************************
 * Function:
 *   adsl_wan_open
 *
 * Description:
 *   Entry point for TTY device open request from the application.
 *-F*************************************************************************/
static int adsl_wan_open(ttystruct_t  *tp, struct file *pFile)
{
	int        status = 0;
	int        line;

	DEBUG_CFG("wanpipe: WanOpen\n");

	line = MINOR(tp->device) - tp->driver.minor_start;
	if ((line < 0) || (line >= ADSL_NR_PORTS)){
		DEBUG_ERROR("wanpipe: Error: Invalid tty line %d!\n", line);
		status = -ENODEV;
		goto adsl_wan_open_exit;
	}

	if (!tty_card_map[line]){
		DEBUG_ERROR("wanpipe: Error: No adapter in tty map for minor %d!\n", line);
		return -ENODEV;
	}

	status = GpWanOpen(tty_card_map[line], line, tp, &tp->driver_data); 

adsl_wan_open_exit:
	return status;
}

/*+F*************************************************************************
 * Function:
 *   GpWanClose
 *
 * Description:
 *   Entry point for TTY device close request from the application.
 *-F*************************************************************************/
static void adsl_wan_close(ttystruct_t  *tp, struct file *pFile)
{
	int line;
	DEBUG_CFG("wanpipe: GpWanClose\n");

	line = MINOR(tp->device) - tp->driver.minor_start;
	if ((line < 0) || (line >= ADSL_NR_PORTS)){
		DEBUG_ERROR("wanpipe: Error: Invalid tty line %d!\n", line);
		return;
	}

	if (!tty_card_map[line]){
		DEBUG_ERROR("wanpipe: Error: No adapter in tty map for minor %d!\n", line);
		return;
	}
	
	GpWanClose(tty_card_map[line],tp->driver_data);

	if (tp->driver.flush_buffer){
	 	tp->driver.flush_buffer(tp);
	}
	    
	if (tp->ldisc.flush_buffer){
	 	tp->ldisc.flush_buffer(tp);
	}
}

/*+F*************************************************************************
 * Function:
 *   GpWanTx
 *
 * Description:
 *   Send a buffer to the adapter. It is assumed that the data has already
 *   been formatted as a synchronous PPP packet. This can be assured by
 *   setting the sync flag to PPP. Since the adapter does not need the PPP
 *   Address/Control bytes, they are stripped before the packet is sent down
 *   to the adapter.
 *-F*************************************************************************/
static int 
adsl_wan_tx(ttystruct_t* tp, int fromUser, const unsigned char* buffer, int bufferLen)
{
	if (tp->driver_data == NULL){
		return -EINVAL;
	}
	
	return GpWanTx(tp->driver_data, fromUser, buffer, bufferLen);
}


unsigned int  adsl_get_tty_minor(void *tty_ptr)
{ 
	ttystruct_t  *tp = (ttystruct_t  *)tty_ptr;
	return MINOR(tp->device);
}

unsigned int adsl_get_tty_minor_start(void *tty_ptr)
{ 
	ttystruct_t  *tp = (ttystruct_t  *)tty_ptr;
	return tp->driver.minor_start;
}

void adsl_mod_inc_use_count (void)
{
	MOD_INC_USE_COUNT;
}

void adsl_mod_dec_use_count (void)
{
	MOD_DEC_USE_COUNT;
}

void adsl_set_tty_driver_data(void *tty_ptr, void *ptr)
{
	ttystruct_t  *tp = (ttystruct_t  *)tty_ptr;
	tp->driver_data = ptr;
}
void adsl_set_low_latency(void *tty_ptr, unsigned char data)
{
	ttystruct_t  *tp = (ttystruct_t  *)tty_ptr;
	tp->low_latency = data;
}
void *adsl_get_tty_driver_state(void *tty_ptr)
{
	ttystruct_t  *tp = (ttystruct_t  *)tty_ptr;
	WAN_ASSERT2((tp==NULL),NULL);

	return (void*)tp->driver.driver_state;		
}


void adsl_tty_hangup(void *tty_ptr)
{
	ttystruct_t  *tp = (ttystruct_t  *)tty_ptr;
        WAN_ASSERT1((tp==NULL));
	DEBUG_EVENT("Hanging up TTY\n");
	tty_hangup(tp);
	schedule();
}
#endif

void* adsl_termios_alloc(void)
{
	return wan_malloc(sizeof(termios_t));
}

/*+F*************************************************************************
 * Function:
 *   GpWanRegister
 *
 * Description:
 *   Register the WAN interface of the adapter as a TTY device with the
 *   operating system.
 *-F*************************************************************************/
int adsl_wan_register(void *driver_data, 
		      char *devname,
		      unsigned char minor_no)
{
    	int     status = 0;

#if defined(__LINUX__)
    	ttydriver_t *tp = &serial_driver;

	if (minor_no >= ADSL_NR_PORTS){
		DEBUG_ERROR("%s: Error Illegal Minor TTY number (0-%d): %i\n",
				devname,ADSL_NR_PORTS,minor_no);
		return -EINVAL;
	}

	if (tty_card_map[minor_no] != NULL){
		DEBUG_ERROR("%s: Error TTY Minor %i, already in use\n",
				devname,minor_no);
		return -EBUSY;	
	}

	DEBUG_EVENT ("%s: ADSL Registering %s%d  Major=%d Minor=%i Channels=(0-%d)\n",
		    devname,ADSL_WAN_TTY_NAME,minor_no,ADSL_TTY_MAJOR,minor_no,ADSL_NR_PORTS-1);
	
    	if (tty_init_cnt==0){
	    
	    	memset(tp,0,sizeof(ttydriver_t));

		tp->magic           = TTY_DRIVER_MAGIC;
		tp->driver_name     = "wanpipe";
		tp->name            = ADSL_WAN_TTY_NAME;
		tp->major           = ADSL_TTY_MAJOR;
		tp->minor_start     = 0;
		tp->num             = ADSL_NR_PORTS;
		tp->type            = TTY_DRIVER_TYPE_SERIAL;
		tp->subtype         = SERIAL_TYPE_NORMAL;
		tp->init_termios    = tty_std_termios;
		tp->init_termios.c_cflag = B9600 | CS8 | CREAD | HUPCL | CLOCAL;
		tp->flags           = TTY_DRIVER_REAL_RAW;
		tp->refcount        = &serial_refcount;
		tp->table           = serial_table;
		tp->termios         = serial_termios;
		tp->termios_locked  = serial_termios_locked;

		tp->open            = adsl_wan_open;
		tp->close           = adsl_wan_close;
		tp->write           = adsl_wan_tx;
		tp->write_room      = adsl_wan_write_room;
		tp->chars_in_buffer = adsl_wan_chars_in_buffer;
		tp->flush_chars     = adsl_wan_flush_chars;
		tp->flush_buffer    = adsl_wan_flush_buffer;
		tp->ioctl           = NULL;// GpWanIoctl;
		tp->set_termios     = adsl_wan_set_termios;
		tp->stop            = NULL;
		tp->start           = NULL;
		tp->throttle        = NULL;
		tp->unthrottle      = NULL;
		tp->hangup          = NULL;
		tp->break_ctl       = NULL;
		tp->send_xchar      = NULL;
		tp->wait_until_sent = NULL;
		tp->read_proc       = NULL;

		if (tty_register_driver(tp)){
			DEBUG_EVENT("Unabled to register TTY interface!\n");
			return -ENODEV;
		}
   	}
    
   	tty_init_cnt++;
	tty_card_map[minor_no] = driver_data; 
	
#endif
	return status;
}

void adsl_wan_unregister(unsigned char minor_no)
{
#if defined(__LINUX__)

	if ((--tty_init_cnt) == 0){
		*serial_driver.refcount=0;
		tty_unregister_driver(&serial_driver);
	}
	tty_card_map[minor_no]=NULL;

#endif
}

/*+F*************************************************************************
 * Function:
 *   adsl_wan_soft_intr
 *
 * Description:
 *   Routine to actually process queued channel events.
 *-F*************************************************************************/
void adsl_wan_soft_intr(void *tty_ptr, unsigned int bit, unsigned long *event)
{

#if defined(__LINUX__)
	ttystruct_t  *tp = (ttystruct_t  *)tty_ptr;
	DEBUG_TX("In adsl_wan_soft_intr\n");
	if (test_and_clear_bit(bit, event)){
        	if ((tp->flags & (1 << TTY_DO_WRITE_WAKEUP)) &&
                    (tp->ldisc.write_wakeup != NULL)){
			(tp->ldisc.write_wakeup)(tp);
		}
		wake_up_interruptible(&(tp->write_wait));
# if defined(SERIAL_HAVE_POLL_WAIT) || \
	 (defined LINUX_2_1 && LINUX_VERSION_CODE >= KERNEL_VERSION(2,2,15))
		wake_up_interruptible(&tp->poll_wait);
# endif	
	}
#endif	
}

void adsl_tty_receive(void *tty_ptr, unsigned char *pData, 
		      unsigned char *pFlags,unsigned int dataLen)
{	
#if defined(__LINUX__)
	ttystruct_t  *tp = (ttystruct_t  *)tty_ptr;
	WAN_ASSERT1((tp==NULL));
	tp->ldisc.receive_buf(tp, pData, NULL, dataLen);
#endif	
}


