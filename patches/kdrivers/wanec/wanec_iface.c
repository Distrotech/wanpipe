/********************************************************************
 * wanec_iface.c   WANPIPE Echo Canceller Layer (WANEC)
 *
 *
 *
 * ==================================================================
 *
 * May	10	2006	Alex Feldman
 *			Initial Version
 *
 * January 9	2008	David Rokhvarg
 *			Added support for Sangoma MS Windows Driver
 *
 ********************************************************************/

/*=============================================================
 * Includes
 */
#undef WAN_DEBUG_FUNC

#include "wanpipe_includes.h"
#include "wanpipe_defines.h"
#include "wanpipe_debug.h"
#include "wanpipe.h"
#include "wanpipe_ec_kernel.h"

#if defined(__FreeBSD__) || defined(__OpenBSD__)
# include <sdla_cdev.h>
#elif defined(__LINUX__)
# include "if_wanpipe.h"
# include "wanpipe_tdm_api.h"
#elif defined(__WINDOWS__)
# include "wanpipe_tdm_api.h"
#endif

#include "wanec_iface_api.h"

/*=============================================================
 * Definitions
 */
#define WAN_OCT6100_ENABLE_INTR_POLL

#define WAN_OCT6100_READ_LIMIT		0x10000

#if 0
# define WANEC_DEBUG
#endif

/*=============================================================
 * Global Parameters
 */
#if defined(WANEC_DEBUG)
static int	wanec_verbose = WAN_EC_VERBOSE_EXTRA2;
#else
static int	wanec_verbose = WAN_EC_VERBOSE_NONE;
#endif

WAN_LIST_HEAD(wan_ec_head_, wan_ec_) wan_ec_head =
		WAN_LIST_HEAD_INITIALIZER(wan_ec_head);

wanec_iface_t	wanec_iface =
{
	0,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL
};

static unsigned char wpec_fullname[]="WANPIPE(tm) WANEC Layer";
static unsigned char wpec_copyright[]="(c) 1995-2006 Sangoma Technologies Inc.";

/*=============================================================
 * Function definition
 */

#if defined(__LINUX__)
extern int wanec_create_dev(void);
extern int wanec_remove_dev(void);
#elif defined(__WINDOWS__)
extern int wanec_create_dev(void);
extern int wanec_remove_dev(void);
extern void* get_wan_ec_ptr(sdla_t *card);
extern void set_wan_ec_ptr(sdla_t *card, IN void *wan_ec_ptr);
#endif

int register_wanec_iface (wanec_iface_t *iface);
void unregister_wanec_iface (void);

extern int wanec_fe2ec_channel(wan_ec_dev_t*, int);

extern int wanec_ChipOpenPrep(wan_ec_dev_t *ec_dev, char *devname, wanec_config_t *config, int);
extern int wanec_ChipOpen(wan_ec_dev_t*, int verbose);
extern int wanec_ChipClose(wan_ec_dev_t*, int verbose);
extern int wanec_ChipStats(wan_ec_dev_t *ec_dev, wanec_chip_stats_t *chip_stats, int reset, int verbose);
extern int wanec_ChipImage(wan_ec_dev_t *ec_dev, wanec_chip_image_t *chip_image, int verbose);

extern int wanec_ChannelOpen(wan_ec_dev_t*, INT, int);
extern int wanec_ChannelClose(wan_ec_dev_t*, INT, int);
extern int wanec_ChannelModifyOpmode(wan_ec_dev_t*, INT, UINT32, int verbose);
extern int wanec_ChannelModifyCustom(wan_ec_dev_t*, INT, wanec_chan_custom_t*, int verbose);
extern int wanec_ChannelStats(wan_ec_dev_t*, INT ec_chan, wanec_chan_stats_t *chan_stats, int reset);

extern int wanec_ChannelMute(wan_ec_dev_t*, INT ec_chan, wanec_chan_mute_t*, int);
extern int wanec_ChannelUnMute(wan_ec_dev_t*, INT ec_chan, wanec_chan_mute_t*, int);

extern int wanec_TonesCtrl(wan_ec_t*, int, int, wanec_tone_config_t*, int);

extern int wanec_DtmfRemoval(wan_ec_dev_t *ec_dev, int channel, int enable, int verbose);

extern int wanec_DebugChannel(wan_ec_dev_t*, INT, int);
extern int wanec_DebugGetData(wan_ec_dev_t*, wanec_chan_monitor_t*, int);

extern int wanec_BufferLoad(wan_ec_dev_t *ec_dev, wanec_buffer_config_t *buffer_config, int verbose);
extern int wanec_BufferUnload(wan_ec_dev_t *ec_dev, wanec_buffer_config_t *buffer_config, int verbose);
extern int wanec_BufferPlayoutAdd(wan_ec_t *ec, int ec_chan, wanec_playout_t *playout, int verbose);
extern int wanec_BufferPlayoutStart(wan_ec_t *ec, int ec_chan, wanec_playout_t *playout, int verbose);
extern int wanec_BufferPlayoutStop(wan_ec_t *ec, int ec_chan, wanec_playout_t *playout, int verbose);

extern int wanec_EventTone(wan_ec_t *ec, int verbose);
extern int wanec_ISR(wan_ec_t *ec, int verbose);

static int wanec_channel_opmode_modify(wan_ec_dev_t *ec_dev, int fe_chan, UINT32 opmode, int verbose);
static int wanec_channel_tone(wan_ec_dev_t*, int, int, wanec_tone_config_t*, int);

static int wanec_bypass(wan_ec_dev_t *ec_dev, int fe_chan, int enable, int verbose);

static int wanec_api_config(wan_ec_dev_t *ec_dev, wan_ec_api_t *ec_api);
static int wanec_api_release(wan_ec_dev_t *ec_dev, wan_ec_api_t *ec_api, int verbose);
static int wanec_api_channel_open(wan_ec_dev_t *ec_dev, wan_ec_api_t *ec_api);
static int wanec_api_modify(wan_ec_dev_t *ec_dev, wan_ec_api_t *ec_api);
static int wanec_api_chan_opmode(wan_ec_dev_t *ec_dev, wan_ec_api_t *ec_api);
static int wanec_api_chan_custom(wan_ec_dev_t *ec_dev, wan_ec_api_t *ec_api);
static int wanec_api_modify_bypass(wan_ec_dev_t *ec_dev, wan_ec_api_t *ec_api);
static int wanec_api_tone(wan_ec_dev_t *ec_dev, wan_ec_api_t *ec_api);
static int wanec_api_stats(wan_ec_dev_t *ec_dev, wan_ec_api_t *ec_api);
static int wanec_api_stats_image(wan_ec_dev_t *ec_dev, wan_ec_api_t *ec_api);
static int wanec_api_buffer(wan_ec_dev_t *ec_dev, wan_ec_api_t *ec_api);
static int wanec_api_playout(wan_ec_dev_t *ec_dev, wan_ec_api_t *ec_api);
static int wanec_api_monitor(wan_ec_dev_t *ec_dev, wan_ec_api_t *ec_api);
static int wanec_api_chan_dtmf_removal(wan_ec_dev_t *ec_dev, wan_ec_api_t *ec_api);

static wan_ec_dev_t *wanec_search(char *devname);

static int wanec_enable(void *pcard, int enable, int fe_chan);
static int __wanec_enable(void *pcard, int enable, int fe_chan);

static int wanec_poll(void *arg, void *pcard);

#if defined(__FreeBSD__) || defined(__OpenBSD__)
int wanec_ioctl(void *sc, void *data);
#elif defined(__LINUX__)
WAN_IOCTL_RET_TYPE wanec_ioctl(unsigned int cmd, void *data);
#endif

static void __wanec_ioctl(wan_ec_dev_t *ec_dev, wan_ec_api_t *ec_api);

int wan_ec_write_internal_dword(wan_ec_dev_t *ec_dev, u32 addr1, u32 data);
int wan_ec_read_internal_dword(wan_ec_dev_t *ec_dev, u32 addr1, u32 *data);

int wanec_init(void*);
int wanec_exit(void*);
#if defined(__FreeBSD__)
int wanec_shutdown(void*);
int wanec_ready_unload(void*);
#endif

/*****************************************************************************/

static u32 convert_addr(u32 addr)
{
	addr &= 0xFFFF;
	switch(addr){
	case 0x0000:
		return 0x60;
	case 0x0002:
		return 0x64;
	case 0x0004:
		return 0x68;
	case 0x0008:
		return 0x70;
	case 0x000A:
		return 0x74;
	}
	return 0x00;
}

/*===========================================================================*\
  ReadInternalDword
\*===========================================================================*/
int wan_ec_read_internal_dword(wan_ec_dev_t *ec_dev, u32 addr1, u32 *data)
{
	sdla_t	*card = NULL;
	u32	addr;
	int err;

	WAN_ASSERT(ec_dev == NULL);
	WAN_ASSERT(ec_dev->card == NULL);
	card = ec_dev->card;
	addr = convert_addr(addr1);
	if (addr == 0x00){
		DEBUG_ERROR("%s: %s:%d: Internal Error (EC off %X)\n",
				card->devname,
				__FUNCTION__,__LINE__,
				addr1);
		return -EINVAL;
	}

	if (IS_A600_CARD(card) || IS_B601_CARD(card)) {
		addr+=0x1000;
	}

	err = card->hw_iface.bus_read_4(card->hw, addr, data);

	WP_DELAY(5);
	return err;
}


/*===========================================================================*\
  WriteInternalDword
\*===========================================================================*/
int wan_ec_write_internal_dword(wan_ec_dev_t *ec_dev, u32 addr1, u32 data)
{
	sdla_t	*card = NULL;
	u32	addr;
	int	err;

	WAN_ASSERT(ec_dev == NULL);
	WAN_ASSERT(ec_dev->card == NULL);
	card = ec_dev->card;
	addr = convert_addr(addr1);
	if (addr == 0x00){
		DEBUG_ERROR("%s: %s:%d: Internal Error (EC off %X)\n",
				card->devname,
				__FUNCTION__,__LINE__,
				addr1);
		return -EINVAL;
	}

 	if (IS_A600_CARD(card) || IS_B601_CARD(card)) {
		addr+=0x1000;
	}

	err = card->hw_iface.bus_write_4(card->hw, addr, data);

	WP_DELAY(5);
	return err;
}


static int wanec_reset(wan_ec_dev_t *ec_dev, int reset)
{
	sdla_t		*card;
	wan_ec_t	*ec;
	int		err = -EINVAL;

	WAN_ASSERT(ec_dev == NULL);
	WAN_ASSERT(ec_dev->ec == NULL);
	WAN_ASSERT(ec_dev->card == NULL);
	ec	= ec_dev->ec;
	card	= ec_dev->card;
	if (card->wandev.hwec_reset){
		/* reset=1 - Set HW EC reset
		** reset=0 - Clear HW EC reset */
		err = card->wandev.hwec_reset(card, reset);
		if (!err){
			if (reset){
				ec->state = WAN_EC_STATE_RESET;
			}else{
				ec->state = WAN_EC_STATE_READY;
			}
		}
	}
	return err;
}


static int wanec_state(void *pcard, wan_hwec_dev_state_t *ecdev_state)
{
	sdla_t		*card = (sdla_t*)pcard;
	wan_ec_dev_t	*ec_dev = NULL;
	wan_ec_t	*ec;

	ec_dev = card->wandev.ec_dev;

	WAN_ASSERT(ec_dev == NULL);
	WAN_ASSERT(ec_dev->ec == NULL);
	ec = ec_dev->ec;
	
	if (ec->state == WAN_EC_STATE_CHIP_OPEN && ec_dev->state == WAN_EC_STATE_CHAN_READY) {
		ec_dev->ecdev_state.ec_state=1;
	} else {
		ec_dev->ecdev_state.ec_state=0;
	}

	memcpy(ecdev_state,&ec_dev->ecdev_state,sizeof(wan_hwec_dev_state_t));

	DEBUG_HWEC("%s: wanecdev_state  ec_state=%i, ec_mode_map=0x%08X, dtmf_map=0x%08X, fax_called=0x%08X, fax_calling=0x%08X\n",
			ec_dev->name,
			ecdev_state->ec_state,
    		ecdev_state->ec_mode_map,
			ecdev_state->dtmf_map,
			ecdev_state->fax_called_map,
			ecdev_state->fax_calling_map);

	return 0;
}

static int __wanec_enable(void *pcard, int enable, int fe_chan)
{
	sdla_t		*card = (sdla_t*)pcard;
	wan_ec_t	*ec = NULL;
	wan_ec_dev_t	*ec_dev = NULL;
	int err;

	ec_dev = card->wandev.ec_dev;

	WAN_ASSERT(ec_dev == NULL);
	WAN_ASSERT(ec_dev->ec == NULL);
	ec = ec_dev->ec;

	if (ec->state != WAN_EC_STATE_CHIP_OPEN){
   		return -EINVAL; 		
	}

	if (ec_dev->state != WAN_EC_STATE_CHAN_READY) {
		return -EINVAL;
	}


	WAN_ASSERT(ec_dev->ec == NULL);
	err = wanec_bypass(ec_dev, fe_chan, enable, 0);

	return err;

}

static int wanec_enable(void *pcard, int enable, int fe_chan)
{
	sdla_t		*card = (sdla_t*)pcard;
	wan_ec_t	*ec = NULL;
	wan_ec_dev_t	*ec_dev = NULL;
	wan_smp_flag_t flags;
	int err;

	ec_dev = card->wandev.ec_dev;

	WAN_ASSERT(ec_dev == NULL);
	WAN_ASSERT(ec_dev->ec == NULL);
	ec = ec_dev->ec;

	if (ec->state != WAN_EC_STATE_CHIP_OPEN){
   		return -EINVAL; 		
	}

	if (ec_dev->state != WAN_EC_STATE_CHAN_READY) {
		return -EINVAL;
	}


#if defined(WANEC_BYDEFAULT_NORMAL)
	WAN_ASSERT(ec_dev->ec == NULL);
	wan_mutex_lock(&ec_dev->ec->lock,&flags);
	err=__wanec_enable(pcard, enable, fe_chan); 
	wan_mutex_unlock(&ec_dev->ec->lock,&flags);

	return err;
#else
	return wanec_channel_opmode_modify(
			ec_dev,
			fe_chan,
			(enable) ? cOCT6100_ECHO_OP_MODE_SPEECH_RECOGNITION : cOCT6100_ECHO_OP_MODE_POWER_DOWN,
			0);
#endif
}

static int
wanec_bypass(wan_ec_dev_t *ec_dev, int fe_chan, int enable, int verbose)
{
	wan_ec_t	*ec = NULL;
	sdla_t		*card = NULL;
	int		ec_chan = 0, err = -ENODEV;

	WAN_ASSERT(ec_dev == NULL);
	WAN_ASSERT(ec_dev->ec == NULL);
	WAN_ASSERT(ec_dev->card == NULL);
	ec = ec_dev->ec;
	card = ec_dev->card;

	PRINT1(verbose,
	"%s: %s bypass mode for fe_chan:%d (ec map:%lX, fe map:%X)!\n",
				card->devname,
				(enable) ? "Enable" : "Disable",
				fe_chan,
				card->wandev.fe_ec_map, ec_dev->fe_channel_map);

	if (card->wandev.hwec_enable == NULL){
		DEBUG_EVENT( "%s: Undefined HW EC Bypass callback function!\n",
					ec->name);
		return -ENODEV;
	}
	if (!wan_test_bit(fe_chan, &ec_dev->fe_channel_map)){
		PRINT1(verbose, "%s: FE channel %d is not available (fe_chan_map=%X)!\n",
					ec->name, fe_chan, ec_dev->fe_channel_map);
		return -ENODEV;
	}
	
	ec_chan = wanec_fe2ec_channel(ec_dev, fe_chan);
	if (enable){
		if (wan_test_bit(fe_chan, &card->wandev.fe_ec_map)){
			/* Already enabled */
                        PRINT2(verbose,
			"%s: Enable bypass mode overrun detected for ec_chan %d!\n",
                                card->devname, ec_chan);
			return 0;
		}
	}else{
		if (!wan_test_bit(fe_chan, &card->wandev.fe_ec_map)){
			/* Already disabled */
                        PRINT2(verbose,
			"%s: Disble bypass mode overrun detected for ec_channel %d!\n",
                                card->devname, ec_chan);
			return 0;
		}
	}

	err=wan_ec_update_and_check(ec,enable);
        if (err) {
                DEBUG_ERROR("%s: Error: Maximum EC Channels Reached! MaxEC=%i\n",
                                ec->name,ec->max_ec_chans);
                return err;
        }

	err = card->wandev.hwec_enable(card, enable, fe_chan);
	if (!err){
		if (enable){
			wan_set_bit(fe_chan, &card->wandev.fe_ec_map);
		}else{
			wan_clear_bit(fe_chan, &card->wandev.fe_ec_map);
		}
	}else if (err < 0){

		/* If ec hwec_enable function fails, undo the action of above
		   wan_ec_update_and_check fucntion by passing in an inverted
		   enable variable */
		wan_ec_update_and_check(ec,!enable);

		DEBUG_EVENT("ERROR: %s: Failed to %s Bypass mode on fe_chan:%d!\n",
				ec->name,
				(enable) ? "Enable" : "Disable",
				fe_chan);
		return err;
	}else{
		/* no action made */
		err = 0;
	}
	return err;
}

#if defined(__NetBSD__) || defined(__FreeBSD__) || defined(__OpenBSD__)
static void wanec_timer(void* p_ec_dev)
#elif defined(__WINDOWS__)
static void wanec_timer(IN PKDPC Dpc, void* p_ec_dev, void* arg2, void* arg3)
#else
static void wanec_timer(unsigned long p_ec_dev)
#endif
{
	wan_ec_dev_t	*ec_dev = (wan_ec_dev_t*)p_ec_dev;
	sdla_t		*card = NULL;

	WAN_ASSERT1(ec_dev == NULL);
	WAN_ASSERT1(ec_dev->card == NULL);
	card = (sdla_t*)ec_dev->card;

	if (wan_test_bit(WAN_EC_BIT_TIMER_KILL,(void*)&ec_dev->critical)){
		wan_clear_bit(WAN_EC_BIT_TIMER_RUNNING,(void*)&ec_dev->critical);
		return;
	}
	/*WAN_ASSERT1(wandev->te_enable_timer == NULL); */
	/* Enable hardware interrupt for TE1 */

	if (card->wandev.ec_enable_timer){
		card->wandev.ec_enable_timer(card);
	}else{
		wanec_poll(ec_dev, card);
	}
	return;
}

/*
 ******************************************************************************
 *				wanec_enable_timer()
 *
 * Description: Enable software timer interrupt in delay ms.
 * Arguments:
 * Returns:
 ******************************************************************************
 */
static void wanec_enable_timer(wan_ec_dev_t* ec_dev, u_int8_t cmd, u_int32_t delay)
{
	sdla_t	*card = NULL;

	WAN_ASSERT1(ec_dev == NULL);
	WAN_ASSERT1(ec_dev->card == NULL);
	card = (sdla_t*)ec_dev->card;

	if (wan_test_bit(WAN_EC_BIT_TIMER_KILL,(void*)&ec_dev->critical)){
		wan_clear_bit(WAN_EC_BIT_TIMER_RUNNING, (void*)&ec_dev->critical);
		return;
	}

	if (wan_test_bit(WAN_EC_BIT_TIMER_RUNNING,(void*)&ec_dev->critical)){
		if (ec_dev->poll_cmd == cmd){
			/* Just ignore current request */
			return;
		}
		DEBUG_EVENT("%s: WAN_EC_BIT_TIMER_RUNNING: new_cmd=%X curr_cmd=%X\n",
					ec_dev->name,
					cmd,
					ec_dev->poll_cmd);
		return;
	}

	wan_set_bit(WAN_EC_BIT_TIMER_RUNNING,(void*)&ec_dev->critical);
	ec_dev->poll_cmd=cmd;
	wan_add_timer(&ec_dev->timer, delay * HZ / 1000);
	return;
}

wan_ec_dev_t *wanec_search(char *devname)
{
	wan_ec_t	*ec;
	wan_ec_dev_t	*ec_dev = NULL;

	WAN_LIST_FOREACH(ec, &wan_ec_head, next){
		WAN_LIST_FOREACH(ec_dev, &ec->ec_dev_head, next){
			if (strcmp(ec_dev->devname, devname) == 0){
				return ec_dev;
			}
		}
	}
	return NULL;
}


static int wanec_channel_opmode_modify(wan_ec_dev_t *ec_dev, int fe_chan, UINT32 opmode, int verbose)
{
	wan_ec_t	*ec = NULL;
	u_int32_t	ec_chan = 0;
	int 		err=0;

	WAN_ASSERT(ec_dev == NULL);
	WAN_ASSERT(ec_dev->ec == NULL);
	WAN_ASSERT(ec_dev->card == NULL);
	ec = ec_dev->ec;

	if (ec_dev->state != WAN_EC_STATE_CHAN_READY){
		DEBUG_EVENT(
		"ERROR: %s: Invalid Echo Canceller %s API state (%s)\n",
				ec_dev->devname,
				ec->name,
				WAN_EC_STATE_DECODE(ec->state));
		return -EINVAL;
	}

	switch(opmode){
	case cOCT6100_ECHO_OP_MODE_NORMAL:
	case cOCT6100_ECHO_OP_MODE_HT_FREEZE:
	case cOCT6100_ECHO_OP_MODE_HT_RESET:
	case cOCT6100_ECHO_OP_MODE_NO_ECHO:
	case cOCT6100_ECHO_OP_MODE_POWER_DOWN:
	case cOCT6100_ECHO_OP_MODE_SPEECH_RECOGNITION:
		break;
	default:
		DEBUG_EVENT(
		"%s: Invalid Echo Channel Operation Mode (opmode=%X)\n",
				ec_dev->devname, opmode);
		return -EINVAL;
	}

	/* Enable Echo cancelation on Oct6100 */
	PRINT1(verbose,
	"%s: Modify Echo Channel OpMode %s on fe_chan:%d ...\n",
			ec_dev->devname,
			(opmode == cOCT6100_ECHO_OP_MODE_NORMAL) ? "Normal" :
			(opmode == cOCT6100_ECHO_OP_MODE_POWER_DOWN) ? "Power Down" :
			(opmode == cOCT6100_ECHO_OP_MODE_HT_FREEZE) ? "HT Freeze" :
			(opmode == cOCT6100_ECHO_OP_MODE_HT_RESET) ? "HT Reset" :
			(opmode == cOCT6100_ECHO_OP_MODE_NO_ECHO) ? "NO Echo" :
			(opmode == cOCT6100_ECHO_OP_MODE_SPEECH_RECOGNITION) ? "Speech Recognition" : "Unknown",
			fe_chan);
	ec_chan = wanec_fe2ec_channel(ec_dev, fe_chan);
	err=wanec_ChannelModifyOpmode(ec_dev, ec_chan, opmode, verbose);
	
	if (err == 0) {
    	if (opmode == cOCT6100_ECHO_OP_MODE_POWER_DOWN) {
         	wan_clear_bit(fe_chan,&ec_dev->ecdev_state.ec_mode_map);
		} else {
         	wan_set_bit(fe_chan,&ec_dev->ecdev_state.ec_mode_map);
		}
	}

	return err;
}
static int wanec_update_tone_status (wan_ec_dev_t *ec_dev, 
								     int fe_chan,
									 int	cmd,
									 wanec_tone_config_t	*tone)
{
	if (tone->id == WP_API_EVENT_TONE_DTMF) {
		if (cmd == WAN_TRUE) {      
        	wan_set_bit(fe_chan,&ec_dev->ecdev_state.dtmf_map);
		} else {
        	wan_clear_bit(fe_chan,&ec_dev->ecdev_state.dtmf_map);
		}
	} else if (tone->id == WP_API_EVENT_TONE_FAXCALLING) {
		if (cmd == WAN_TRUE) {      
        	wan_set_bit(fe_chan,&ec_dev->ecdev_state.fax_calling_map);
		} else {
        	wan_clear_bit(fe_chan,&ec_dev->ecdev_state.fax_calling_map);
		}
	} else if (tone->id == WP_API_EVENT_TONE_FAXCALLED) {
		if (cmd == WAN_TRUE) {      
        	wan_set_bit(fe_chan,&ec_dev->ecdev_state.fax_called_map);
		} else {
        	wan_clear_bit(fe_chan,&ec_dev->ecdev_state.fax_called_map);
		}
	}

	return 0;
}

static int wanec_channel_tone(	wan_ec_dev_t		*ec_dev,
				int			fe_chan,
				int			cmd,
				wanec_tone_config_t	*tone,
				int			verbose)
{
	wan_ec_t	*ec = NULL;
	int		ec_chan, err;

	WAN_ASSERT(ec_dev == NULL);
	WAN_ASSERT(ec_dev->ec == NULL);
	ec = ec_dev->ec;
	if (ec_dev->state != WAN_EC_STATE_CHAN_READY){
		DEBUG_EVENT(
		"WARNING: %s: Invalid Echo Canceller %s API state (%s)\n",
				ec_dev->devname,
				ec->name,
				WAN_EC_STATE_DECODE(ec->state));
		return -EINVAL;
	}

	if (tone){
		if ((tone->port_map & (WAN_EC_CHANNEL_PORT_SOUT|WAN_EC_CHANNEL_PORT_ROUT)) != tone->port_map){

			DEBUG_EVENT(
			"ERROR: %s: Invalid Echo Canceller port value (%X)!\n",
					ec_dev->devname,
					tone->port_map);
			return -EINVAL;
		}
	}
	/* Enable/Disable Normal mode on Oct6100 */
	PRINT1(verbose, "%s: %s EC Tone detection on fe_chan:%d ...\n",
			ec_dev->devname, (cmd==WAN_TRUE) ? "Enable" : "Disable",
			fe_chan);
	ec_chan = wanec_fe2ec_channel(ec_dev, fe_chan);
	err = wanec_TonesCtrl(ec, cmd, ec_chan, tone, verbose);
	if (err == WAN_EC_API_RC_OK){

		wanec_update_tone_status(ec_dev, fe_chan, cmd, tone);

		if (cmd == WAN_TRUE){
			wan_set_bit(WAN_EC_BIT_EVENT_TONE, &ec_dev->events);
			ec->tone_verbose = verbose;
		}else{
			/* FIXME: Once the Tone event was enabled, do not
			** disable it otherwise tone events for other
			** channels will be delayed
			** wan_clear_bit(WAN_EC_BIT_EVENT_TONE, &ec_dev->events);
			** ec->tone_verbose = 0;	*/
		}
	}
	return err;
}


static int wanec_channel_mute(	wan_ec_dev_t		*ec_dev,
				int			fe_chan,
				int			cmd,
				wanec_chan_mute_t 	*mute,
				int 			verbose)
{
	wan_ec_t	*ec = NULL;
	int		ec_chan, err;

	WAN_ASSERT(ec_dev == NULL);
	WAN_ASSERT(ec_dev->ec == NULL);
	ec = ec_dev->ec;

	if (ec_dev->state != WAN_EC_STATE_CHAN_READY){
		DEBUG_EVENT(
		"ERROR: %s: Invalid Echo Canceller %s API state (%s)\n",
				ec_dev->devname,
				ec->name,
				WAN_EC_STATE_DECODE(ec->state));
		return -EINVAL;
	}

	/* Enable/Disable Normal mode on Oct6100 */
	PRINT1(verbose,
	"%s: %s EC channel on fe_chan:%d ...\n",
			ec_dev->devname,
			(cmd == WAN_TRUE) ? "Mute" : "Un-mute",
			fe_chan);

	ec_chan = wanec_fe2ec_channel(ec_dev, fe_chan);
	if (cmd == WAN_TRUE){
		err = wanec_ChannelMute(ec_dev, ec_chan, mute, verbose);
	}else{
		err = wanec_ChannelUnMute(ec_dev, ec_chan, mute, verbose);
	}
	return err;
}


/******************************************************************************
**		WANPIPE EC API INTERFACE FUNCTION
******************************************************************************/

static int wanec_api_config(wan_ec_dev_t *ec_dev, wan_ec_api_t *ec_api)
{
	wan_ec_t	*ec = NULL;
	int		err;

	WAN_ASSERT(ec_dev == NULL);
	WAN_ASSERT(ec_dev->ec == NULL);
	ec = ec_dev->ec;
	switch(ec->state){
	case WAN_EC_STATE_RESET:
	case WAN_EC_STATE_READY:
		break;
	case WAN_EC_STATE_CHIP_OPEN_PENDING:
	case WAN_EC_STATE_CHIP_OPEN:
		DEBUG_HWEC(
		"%s: Echo Canceller %s chip is %s!\n",
				ec_api->devname, ec->name,
				WAN_EC_STATE_DECODE(ec->state));
		break;
	default:
		DEBUG_EVENT(
		"ERROR: %s: Invalid Echo Canceller %s API state (%s)\n",
				ec_api->devname, ec->name,
				WAN_EC_STATE_DECODE(ec->state));
		ec_api->err	= WAN_EC_API_RC_INVALID_STATE;
		return 0;
	}

	if (ec->state == WAN_EC_STATE_RESET){
		err = wanec_reset(ec_dev, 0);
		if (err) return err;
	}

	if (ec->state == WAN_EC_STATE_READY){

		ec->pImageData = wan_vmalloc(ec_api->u_config.imageSize * sizeof(UINT8));
		if (ec->pImageData == NULL){
			DEBUG_EVENT(
			"ERROR: Failed to allocate memory for EC image %ld bytes [%s:%d]!\n",
					(unsigned long)ec_api->u_config.imageSize*sizeof(UINT8),
					__FUNCTION__,__LINE__);
			err = wanec_reset(ec_dev, 0);
			return -EINVAL;
		}
		err = WAN_COPY_FROM_USER(
					ec->pImageData,
					ec_api->u_config.imageData,
					ec_api->u_config.imageSize * sizeof(UINT8));
		if (err){
			DEBUG_EVENT(
			"ERROR: Failed to copy EC image from user space [%s:%d]!\n",
					__FUNCTION__,__LINE__);
			wan_vfree(ec->pImageData);
			err = wanec_reset(ec_dev, 0);
			return -EINVAL;
		}
		ec->ImageSize = ec_api->u_config.imageSize;

		/* Copying custom configuration (if exists) */
		if (ec_api->custom_conf.param_no){
			ec_api->u_config.custom_conf.params =
					wan_malloc(ec_api->custom_conf.param_no * sizeof(wan_custom_param_t));
			if (ec_api->u_config.custom_conf.params){
				int	err = 0;
				err = WAN_COPY_FROM_USER(
						ec_api->u_config.custom_conf.params,
						ec_api->custom_conf.params,
						ec_api->custom_conf.param_no * sizeof(wan_custom_param_t));
				if (err){
					DEBUG_ERROR(
					"ERROR: Failed to copy Custom Parameters from user space [%s():%d]!\n",
							__FUNCTION__,__LINE__);
				}
				
				ec_api->u_config.custom_conf.param_no = ec_api->custom_conf.param_no;
			}
		}

		if (wanec_ChipOpenPrep(ec_dev, ec_api->devname, &ec_api->u_config, ec_api->verbose)){
			wan_vfree(ec->pImageData);
			if (ec_api->u_config.custom_conf.params){
				wan_free(ec_api->u_config.custom_conf.params);
			}
			wanec_reset(ec_dev, 1);
			return -EINVAL;
		}
		if (ec_api->u_config.custom_conf.params){
			wan_free(ec_api->u_config.custom_conf.params);
		}
		ec->imageLast	= ec_api->u_config.imageLast;
		ec->state	= WAN_EC_STATE_CHIP_OPEN_PENDING;
		wanec_enable_timer(ec_dev, WAN_EC_POLL_CHIPOPENPENDING, 10);
	}
	ec_dev->state = ec->state;
	return 0;
}

static int wanec_api_channel_open(wan_ec_dev_t *ec_dev, wan_ec_api_t *ec_api)
{
	wan_ec_t	*ec = NULL;
	int		ec_chan, fe_chan;

	WAN_ASSERT(ec_dev == NULL);
	WAN_ASSERT(ec_dev->ec == NULL);
	ec = ec_dev->ec;
	switch(ec->state){
	case WAN_EC_STATE_CHIP_OPEN:
		break;
	default:
		DEBUG_EVENT(
		"ERROR: %s: Invalid Echo Canceller %s API state (%s)\n",
				ec_api->devname, ec->name,
				WAN_EC_STATE_DECODE(ec->state));
		ec_api->err	= WAN_EC_API_RC_INVALID_STATE;
		return 0;
	}

	if (ec_dev->state == WAN_EC_STATE_CHAN_READY){
		PRINT1(ec_api->verbose,
		"%s: Echo Canceller %s chip is %s!\n",
				ec_api->devname, ec->name,
				WAN_EC_STATE_DECODE(ec->state));
		return 0;
	}

	if (ec->state == WAN_EC_STATE_CHIP_OPEN){

		if (ec_api->fe_chan_map == 0xFFFFFFFF){
			/* All channels selected */
			ec_api->fe_chan_map = ec_dev->fe_channel_map;
		}
		for(fe_chan = ec_dev->fe_start_chan; fe_chan <= ec_dev->fe_stop_chan; fe_chan++){
			if (!(ec_api->fe_chan_map & (1 << fe_chan))){
				continue;
			}
			if (ec_dev->fe_media == WAN_MEDIA_E1 && fe_chan == 0) continue;
			ec_chan = wanec_fe2ec_channel(ec_dev, fe_chan);
			wanec_ChannelOpen(ec_dev, ec_chan, ec_api->verbose);
		}
		ec_dev->state = WAN_EC_STATE_CHAN_READY;
	}

	/* EC_DEV_MAP */
	for(fe_chan = ec_dev->fe_start_chan; fe_chan <= ec_dev->fe_stop_chan; fe_chan++){
		ec_chan = wanec_fe2ec_channel(ec_dev, fe_chan);
		ec->pEcDevMap[ec_chan] = ec_dev;
	}
	return 0;
}

int wanec_api_release(wan_ec_dev_t *ec_dev, wan_ec_api_t *ec_api, int verbose)
{
	wan_ec_t	*ec = NULL;
	wan_ec_dev_t	*ec_dev_tmp = NULL;
	int		fe_chan, ec_chan, err = 0;

	WAN_ASSERT(ec_dev == NULL);
	WAN_ASSERT(ec_dev->ec == NULL);
	ec = ec_dev->ec;

	switch(ec->state){
	case WAN_EC_STATE_READY:
	case WAN_EC_STATE_CHIP_OPEN_PENDING:
	case WAN_EC_STATE_CHIP_OPEN:
		break;
	case WAN_EC_STATE_RESET:
		return 0;
	default:
		PRINT1(verbose,
		"%s: WARNING: Echo Canceller %s API state (%s)\n",
				ec_dev->devname,
				ec->name,
				WAN_EC_STATE_DECODE(ec->state));
		return 0;
	}

	WAN_LIST_FOREACH(ec_dev_tmp, &ec->ec_dev_head, next){
		if (ec_dev_tmp != ec_dev){
			if (ec_dev_tmp->state == WAN_EC_STATE_CHAN_READY){
				/* This EC device is still connected */
				ec->f_Context.ec_dev	= ec_dev_tmp;
				wp_strlcpy(ec->f_Context.devname, ec_dev_tmp->devname, WAN_DRVNAME_SZ);
				break;
			}
		}
	}

	if (ec_dev->state == WAN_EC_STATE_CHAN_READY || ec->state == WAN_EC_STATE_CHIP_OPEN){
		for(fe_chan = ec_dev->fe_start_chan; fe_chan <= ec_dev->fe_stop_chan; fe_chan++){
			ec_chan = wanec_fe2ec_channel(ec_dev, fe_chan);
			if (ec->pEcDevMap){
				ec->pEcDevMap[ec_chan] = NULL;
			}
			if (!(ec_dev->fe_channel_map & (1 << fe_chan))){
				continue;
			}
			ec_chan = wanec_fe2ec_channel(ec_dev, fe_chan);

			wanec_bypass(ec_dev, fe_chan, 0, 0);

			if (ec_dev->fe_media == WAN_MEDIA_E1 && fe_chan == 0) continue;
 			wanec_ChannelClose(ec_dev, ec_chan, verbose);
		}
	}

	memset(&ec_dev->ecdev_state,0,sizeof(ec_dev->ecdev_state));

	ec_dev->state = WAN_EC_STATE_RESET;
	if (ec_dev_tmp){
		/* EC device is still in use ("configure" was done).
		 * This may happen only on multi-port cards, but
		 * never on Analog or single-port digital. */
		PRINT2(verbose,	"%s: %s(): this EC still used by another port\n", 
			ec->name, __FUNCTION__);
		return 0;
	}

	if (ec->state == WAN_EC_STATE_CHIP_OPEN){

		PRINT2(verbose,	"%s: %s(): calling wanec_ChipClose()\n", 
			ec->name, __FUNCTION__);

		if (wanec_ChipClose(ec_dev, verbose)){
			return EINVAL;
		}

		ec->state = WAN_EC_STATE_READY;
	}

	if (ec->state == WAN_EC_STATE_CHIP_OPEN_PENDING){
		ec->state = WAN_EC_STATE_READY;
	}

	if (ec->state == WAN_EC_STATE_READY){

		PRINT2(verbose, "%s: %s(): calling wanec_reset()\n", 
			ec->name, __FUNCTION__);

		err = wanec_reset(ec_dev, 1);
		if (err){
			return EINVAL;
		}
	}
	return 0;
}


static int wanec_api_modify(wan_ec_dev_t *ec_dev, wan_ec_api_t *ec_api)
{
	wan_ec_t	*ec = NULL;
	u_int32_t	cmd = ec_api->cmd;
	int		fe_chan = 0;
#if 0
	u_int32_t	ec_chan = 0;
#endif
	int		err;

	WAN_ASSERT(ec_dev == NULL);
	WAN_ASSERT(ec_dev->ec == NULL);
	ec = ec_dev->ec;

	if (ec_dev->state != WAN_EC_STATE_CHAN_READY){
		DEBUG_EVENT(
		"ERROR: %s: Invalid Echo Canceller %s API state (%s)\n",
				ec_dev->devname,
				ec->name,
				WAN_EC_STATE_DECODE(ec->state));
		return WAN_EC_API_RC_INVALID_STATE;
	}

	if (ec_api->fe_chan_map == 0xFFFFFFFF){
		/* All channels selected */
		ec_api->fe_chan_map = ec_dev->fe_channel_map;
	}

	/* Enable Echo cancelation on Oct6100 */
	PRINT1(ec_api->verbose,
	"%s: %s Echo Canceller on channel(s) chan_map=0x%08lX ...\n",
			ec_dev->devname,
			(cmd == WAN_EC_API_CMD_ENABLE) ? "Enable" : "Disable",
			ec_api->fe_chan_map);
	/*for(chan = fe_first; chan <= fe_last; chan++){*/
	for(fe_chan = ec_dev->fe_start_chan; fe_chan <= ec_dev->fe_stop_chan; fe_chan++){
		if (!(ec_api->fe_chan_map & (1 << fe_chan))){
			continue;
		}
		if (ec_dev->fe_media == WAN_MEDIA_E1 && fe_chan == 0) continue;
		if (cmd == WAN_EC_API_CMD_ENABLE){
			err = wanec_channel_opmode_modify(
					ec_dev, fe_chan,
					cOCT6100_ECHO_OP_MODE_SPEECH_RECOGNITION,
					ec_api->verbose);
			if (err) {
				return WAN_EC_API_RC_FAILED;
			}
			err = wanec_bypass(ec_dev, fe_chan, 1, ec_api->verbose);
		}else{
			wanec_bypass(ec_dev, fe_chan, 0, ec_api->verbose);
			err = wanec_channel_opmode_modify(
					ec_dev, fe_chan,
					cOCT6100_ECHO_OP_MODE_POWER_DOWN,
					ec_api->verbose);
		}
		if (err){
			return WAN_EC_API_RC_FAILED;
		}
	}

	return 0;
}

static int wanec_api_chan_opmode(wan_ec_dev_t *ec_dev, wan_ec_api_t *ec_api)
{
	wan_ec_t	*ec = NULL;
	int		fe_chan, err;

	WAN_ASSERT(ec_dev == NULL);
	WAN_ASSERT(ec_dev->ec == NULL);
	ec = ec_dev->ec;
	if (ec_dev->state != WAN_EC_STATE_CHAN_READY){
		DEBUG_EVENT(
		"ERROR: %s: Invalid Echo Canceller %s API state (%s)\n",
				ec_dev->devname,
				ec->name,
				WAN_EC_STATE_DECODE(ec->state));
		return WAN_EC_API_RC_INVALID_STATE;
	}

	if (ec_api->fe_chan_map == 0xFFFFFFFF){
		/* All channels selected */
		ec_api->fe_chan_map = ec_dev->fe_channel_map;
	}
	/* Enable/Disable Normal mode on Oct6100 */
	PRINT1(ec_api->verbose,
	"%s: Modify Echo Canceller opmode on channel(s) chan_map=0x%08lX ...\n",
			ec_dev->devname, ec_api->fe_chan_map);
	for(fe_chan = ec_dev->fe_start_chan; fe_chan <= ec_dev->fe_stop_chan; fe_chan++){
		if (!(ec_api->fe_chan_map & (1 << fe_chan))){
			continue;
		}
		if (ec_dev->fe_media == WAN_MEDIA_E1 && fe_chan == 0) continue;
		err = wanec_channel_opmode_modify(
				ec_dev, fe_chan, ec_api->u_chan_opmode.opmode, ec_api->verbose);
		if (err){
			return WAN_EC_API_RC_FAILED;
		}
	}
	return 0;
}

static int wanec_api_chan_custom(wan_ec_dev_t *ec_dev, wan_ec_api_t *ec_api)
{
	wan_ec_t		*ec = NULL;
	wanec_chan_custom_t	*chan_custom;
	int			fe_chan, ec_chan, err;

	WAN_ASSERT(ec_dev == NULL);
	WAN_ASSERT(ec_dev->ec == NULL);
	ec = ec_dev->ec;
	if (ec_dev->state != WAN_EC_STATE_CHAN_READY){
		DEBUG_EVENT(
		"ERROR: %s: Invalid Echo Canceller %s API state (%s)\n",
				ec_dev->devname,
				ec->name,
				WAN_EC_STATE_DECODE(ec->state));
		return WAN_EC_API_RC_INVALID_STATE;
	}

	if (ec_api->fe_chan_map == 0xFFFFFFFF){
		/* All channels selected */
		ec_api->fe_chan_map = ec_dev->fe_channel_map;
	}
	/* Enable/Disable Normal mode on Oct6100 */
	PRINT1(ec_api->verbose,
	"%s: Modify EC Channel config (parms:%d) on channel(s) chan_map=0x%08lX ...\n",
			ec_dev->devname, ec_api->custom_conf.param_no,
			ec_api->fe_chan_map);
	if (ec_api->custom_conf.param_no == 0){
		/* nothing to do */
		return 0;
	}
	chan_custom = &ec_api->u_chan_custom;
	chan_custom->custom_conf.params =
			wan_malloc(ec_api->custom_conf.param_no * sizeof(wan_custom_param_t));
	if (chan_custom->custom_conf.params){
		int	err = 0;
		err = WAN_COPY_FROM_USER(
				chan_custom->custom_conf.params,
				ec_api->custom_conf.params,
				ec_api->custom_conf.param_no * sizeof(wan_custom_param_t));
		chan_custom->custom_conf.param_no = ec_api->custom_conf.param_no;
	}else{
		DEBUG_EVENT(
		"%s: WARNING: Skipping custom OCT6100 configuration (allocation failed)!\n",
					ec_dev->devname);
		return WAN_EC_API_RC_FAILED;
	}

	for(fe_chan = ec_dev->fe_start_chan; fe_chan <= ec_dev->fe_stop_chan; fe_chan++){
		if (!(ec_api->fe_chan_map & (1 << fe_chan))){
			continue;
		}
		if (ec_dev->fe_media == WAN_MEDIA_E1 && fe_chan == 0) continue;
		ec_chan = wanec_fe2ec_channel(ec_dev, fe_chan);
		err = wanec_ChannelModifyCustom(ec_dev, ec_chan, chan_custom, ec_api->verbose);
		if (err){
			wan_free(chan_custom->custom_conf.params);
			chan_custom->custom_conf.params = NULL;
			return WAN_EC_API_RC_FAILED;
		}
	}
	wan_free(chan_custom->custom_conf.params);
	chan_custom->custom_conf.params = NULL;
	return 0;
}

static int wanec_api_modify_bypass(wan_ec_dev_t *ec_dev, wan_ec_api_t *ec_api)
{
	wan_ec_t	*ec = NULL;
	unsigned int	fe_chan = 0;

	WAN_ASSERT(ec_dev == NULL);
	WAN_ASSERT(ec_dev->ec == NULL);
	ec = ec_dev->ec;

	if (ec_dev->state != WAN_EC_STATE_CHAN_READY){
		DEBUG_EVENT(
		"ERROR: %s: Invalid Echo Canceller %s API state (%s)\n",
				ec_dev->devname,
				ec->name,
				WAN_EC_STATE_DECODE(ec->state));
		return WAN_EC_API_RC_INVALID_STATE;
	}

	if (ec_api->fe_chan_map == 0xFFFFFFFF){
		/* All channels selected */
		ec_api->fe_chan_map = ec_dev->fe_channel_map;
	}

	/* Enable/Disable bypass mode on Oct6100 */
	PRINT1(ec_api->verbose,
	"%s: %s Bypass mode on channel(s) chan_map=0x%08lX ...\n",
			ec_dev->devname,
			(ec_api->cmd == WAN_EC_API_CMD_BYPASS_ENABLE) ? "Enable" : "Disable",
			ec_api->fe_chan_map);
	for(fe_chan = ec_dev->fe_start_chan; fe_chan <= (unsigned int)ec_dev->fe_stop_chan; fe_chan++){
		if (!(ec_api->fe_chan_map & (1 << fe_chan))){
			continue;
		}
		if (ec_api->cmd == WAN_EC_API_CMD_BYPASS_ENABLE){
			/* Change rx/tx traffic through Oct6100. */
			if (wanec_bypass(ec_dev, fe_chan, 1, ec_api->verbose)){
				return WAN_EC_API_RC_FAILED;
			}
		}else{
			/* The rx/tx traffic will NOT go through Oct6100. */
			if (wanec_bypass(ec_dev, fe_chan, 0, ec_api->verbose)){
				return WAN_EC_API_RC_FAILED;
			}
		}
	}
	return 0;
}


static int wanec_api_channel_mute(wan_ec_dev_t *ec_dev, wan_ec_api_t *ec_api)
{
	wan_ec_t	*ec = NULL;
	int		fe_chan;
	int		err = WAN_EC_API_RC_OK;

	WAN_ASSERT(ec_dev == NULL);
	WAN_ASSERT(ec_dev->ec == NULL);
	ec = ec_dev->ec;
	if (ec_dev->state != WAN_EC_STATE_CHAN_READY){
		DEBUG_EVENT(
		"ERROR: %s: Invalid Echo Canceller %s API state (%s)\n",
				ec_dev->devname,
				ec->name,
				WAN_EC_STATE_DECODE(ec->state));
		return WAN_EC_API_RC_INVALID_STATE;
	}
	
	if (ec_api->fe_chan_map == 0xFFFFFFFF){
		/* All channels selected */
		ec_api->fe_chan_map = ec_dev->fe_channel_map;
	}

	if (!ec_api->fe_chan_map){
		return WAN_EC_API_RC_NOACTION;
	}
	for(fe_chan = ec_dev->fe_start_chan; fe_chan <= ec_dev->fe_stop_chan; fe_chan++){
		if (!(ec_api->fe_chan_map & (1 << fe_chan))){
			continue;
		}
		if (ec_dev->fe_media == WAN_MEDIA_E1 && fe_chan == 0) continue;
		err = wanec_channel_mute(
					ec_dev,
					fe_chan,
					(ec_api->cmd == WAN_EC_API_CMD_CHANNEL_MUTE) ? WAN_TRUE: WAN_FALSE,
					&ec_api->u_chan_mute,
					ec_api->verbose);
		if (err){
			return WAN_EC_API_RC_FAILED;
		}
	}
	return err;
}

static int wanec_api_chan_dtmf_removal(wan_ec_dev_t *ec_dev, wan_ec_api_t *ec_api)
{
	wan_ec_t	*ec = NULL;
	int		fe_chan, ec_chan;
	int		err = WAN_EC_API_RC_OK;

	WAN_ASSERT(ec_dev == NULL);
	WAN_ASSERT(ec_dev->ec == NULL);
	ec = ec_dev->ec;

	if (ec_dev->state != WAN_EC_STATE_CHAN_READY){
		DEBUG_EVENT("ERROR: %s: Invalid Echo Canceller %s API state (%s)\n",
					ec_dev->devname,
					ec->name,
					WAN_EC_STATE_DECODE(ec->state));

		return WAN_EC_API_RC_INVALID_STATE;
	}
	
	if (ec_api->fe_chan_map == 0xFFFFFFFF){
		/* All channels selected */
		ec_api->fe_chan_map = ec_dev->fe_channel_map;
	}

	if (!ec_api->fe_chan_map){
		return WAN_EC_API_RC_NOACTION;
	}
	for(fe_chan = ec_dev->fe_start_chan; fe_chan <= ec_dev->fe_stop_chan; fe_chan++){
		if (!(ec_api->fe_chan_map & (1 << fe_chan))){
			continue;
		}
		if (ec_dev->fe_media == WAN_MEDIA_E1 && fe_chan == 0) continue;

		ec_chan = wanec_fe2ec_channel(ec_dev, fe_chan);

		err = wanec_DtmfRemoval(ec_dev, ec_chan, (ec_api->cmd == WAN_EC_API_CMD_HWDTMF_REMOVAL_ENABLE) ? 1 : 0, ec_api->verbose);
		if (err){
			return WAN_EC_API_RC_FAILED;
		}
	}
	return err;
}

		
static int wanec_api_tone(wan_ec_dev_t *ec_dev, wan_ec_api_t *ec_api)
{
	wan_ec_t	*ec = NULL;
	int		fe_chan, err = WAN_EC_API_RC_OK;

	WAN_ASSERT(ec_dev == NULL);
	WAN_ASSERT(ec_dev->ec == NULL);
	ec = ec_dev->ec;
	if (ec_dev->state != WAN_EC_STATE_CHAN_READY){
		DEBUG_EVENT(
		"ERROR: %s: Invalid Echo Canceller %s API state (%s)\n",
				ec_dev->devname,
				ec->name,
				WAN_EC_STATE_DECODE(ec->state));
		return WAN_EC_API_RC_INVALID_STATE;
	}

	if (ec_api->fe_chan_map == 0xFFFFFFFF){
		/* All channels selected */
		ec_api->fe_chan_map = ec_dev->fe_channel_map;
	}

	if (!ec_api->fe_chan_map){
		return WAN_EC_API_RC_NOACTION;
	}
	/* Enable/Disable Normal mode on Oct6100 */
	PRINT1(ec_api->verbose,
	"%s: %s Echo Canceller Tone on channel(s) chan_map=0x%08lX ...\n",
			ec_dev->devname,
			(ec_api->cmd == WAN_EC_API_CMD_TONE_ENABLE) ? "Enable" :
			(ec_api->cmd == WAN_EC_API_CMD_TONE_DISABLE) ? "Disable" :
								"Unknown",
			ec_api->fe_chan_map);
	for(fe_chan = ec_dev->fe_start_chan; fe_chan <= ec_dev->fe_stop_chan; fe_chan++){
		if (!(ec_api->fe_chan_map & (1 << fe_chan))){
			continue;
		}
		if (ec_dev->fe_media == WAN_MEDIA_E1 && fe_chan == 0) continue;
		err = wanec_channel_tone(
				ec_dev,
				fe_chan,
				(ec_api->cmd == WAN_EC_API_CMD_TONE_ENABLE) ? WAN_TRUE : WAN_FALSE,
				&ec_api->u_tone_config,
				ec_api->verbose);
		if (err){
			return WAN_EC_API_RC_FAILED;
		}
	}
	return err;
}

static int wanec_api_stats(wan_ec_dev_t *ec_dev, wan_ec_api_t *ec_api)
{
	wan_ec_t	*ec = NULL;
	int		fe_chan, ec_chan, err = 0;

	WAN_ASSERT(ec_dev == NULL);
	WAN_ASSERT(ec_dev->ec == NULL);
	ec = ec_dev->ec;

	if (ec_dev->state != WAN_EC_STATE_CHAN_READY){
		DEBUG_EVENT(
		"ERROR: %s: Invalid Echo Canceller %s API state (%s)\n",
				ec_dev->devname,
				ec->name,
				WAN_EC_STATE_DECODE(ec->state));
		return WAN_EC_API_RC_INVALID_STATE;
	}

	PRINT1(ec_api->verbose,
	"%s: Read EC stats on channel(s) chan_map=0x%08lX reset:%d...\n",
			ec_dev->devname,
			ec_api->fe_chan_map,
			(ec_api->fe_chan_map) ?
				ec_api->u_chan_stats.reset:ec_api->u_chip_stats.reset);
	if (wanec_ISR(ec, ec_api->verbose)){
		return WAN_EC_API_RC_FAILED;
	}

	if (ec_api->fe_chan_map){
		int	ready = 0;

		for(fe_chan = ec_dev->fe_start_chan; fe_chan <= ec_dev->fe_stop_chan; fe_chan++){
			if (!(ec_api->fe_chan_map & (1 << fe_chan))){
				continue;
			}
			if (!wan_test_bit(fe_chan, &ec_dev->fe_channel_map)){
				continue;
			}
			if (ec_dev->fe_media == WAN_MEDIA_E1 && fe_chan == 0){
				continue;
			}
			ec_chan = wanec_fe2ec_channel(ec_dev, fe_chan);
			err = wanec_ChannelStats(
						ec_dev,
						ec_chan,
						&ec_api->u_chan_stats,
						ec_api->verbose);
			if (err){
				return WAN_EC_API_RC_FAILED;
			}
			ready = 1;
			break;
		}
		if (!ready){
			return WAN_EC_API_RC_INVALID_CHANNELS;
		}
	}else{
		wanec_ChipStats(ec_dev, &ec_api->u_chip_stats, ec_api->u_chip_stats.reset, ec_api->verbose);
	}

	return 0;
}

static int wanec_api_stats_image(wan_ec_dev_t *ec_dev, wan_ec_api_t *ec_api)
{
	wan_ec_t	*ec = NULL;

	WAN_ASSERT(ec_dev == NULL);
	WAN_ASSERT(ec_dev->ec == NULL);
	ec = ec_dev->ec;
	if (ec_dev->state != WAN_EC_STATE_CHAN_READY){
		DEBUG_EVENT(
		"ERROR: %s: Invalid Echo Canceller %s API state (%s)\n",
				ec_dev->devname,
				ec->name,
				WAN_EC_STATE_DECODE(ec->state));
		return WAN_EC_API_RC_INVALID_STATE;
	}

	PRINT1(ec_api->verbose,
	"%s: Read EC Image stats ...\n",
			ec_dev->devname);
	wanec_ChipImage(ec_dev, &ec_api->u_chip_image, ec_api->verbose);
	return 0;
}

static int wanec_api_monitor(wan_ec_dev_t *ec_dev, wan_ec_api_t *ec_api)
{
	wan_ec_t	*ec = NULL;
	int		fe_chan = ec_api->fe_chan,
			ec_chan = 0;

	WAN_ASSERT(ec_dev == NULL);
	WAN_ASSERT(ec_dev->ec == NULL);
	ec = ec_dev->ec;
	if (ec_dev->state != WAN_EC_STATE_CHAN_READY){
		DEBUG_EVENT(
		"ERROR: %s: Invalid Echo Canceller %s API state (%s)\n",
				ec_dev->devname,
				ec->name,
				WAN_EC_STATE_DECODE(ec->state));
		return WAN_EC_API_RC_INVALID_STATE;
	}

	/* Sanity check from channel selection */
	if (fe_chan > ec_dev->fe_stop_chan){
		DEBUG_EVENT(
		"ERROR: %s: Front-End channel number %d is out of range!\n",
				ec_dev->devname, fe_chan);
		return WAN_EC_API_RC_INVALID_CHANNELS;
	}

	if (fe_chan){
		if (!(ec_dev->fe_channel_map & (1 << fe_chan))){
			return WAN_EC_API_RC_INVALID_CHANNELS;
		}
		ec_chan = wanec_fe2ec_channel(ec_dev, fe_chan);
		if (wanec_DebugChannel(ec_dev, ec_chan, ec_api->verbose)){
			return WAN_EC_API_RC_FAILED;
		}
	}else{
		if (wanec_DebugGetData(ec_dev, &ec_api->u_chan_monitor, ec_api->verbose)){
			return WAN_EC_API_RC_FAILED;
		}
		ec_api->fe_chan = ec_api->u_chan_monitor.fe_chan;
	}
	return 0;
}

static int wanec_api_buffer(wan_ec_dev_t *ec_dev, wan_ec_api_t *ec_api)
{
	wan_ec_t	*ec = NULL;
	int		err;

	WAN_ASSERT(ec_dev == NULL);
	WAN_ASSERT(ec_dev->ec == NULL);
	ec = ec_dev->ec;
	if (ec_dev->state != WAN_EC_STATE_CHAN_READY){
		DEBUG_EVENT(
		"ERROR: %s: Invalid Echo Canceller %s API state (%s)\n",
				ec_dev->devname,
				ec->name,
				WAN_EC_STATE_DECODE(ec->state));
		return WAN_EC_API_RC_INVALID_STATE;
	}
	if (ec_api->cmd == WAN_EC_API_CMD_BUFFER_LOAD){
		err = wanec_BufferLoad(ec_dev,  &ec_api->u_buffer_config, ec_api->verbose);
	}else{
		err = wanec_BufferUnload(ec_dev, &ec_api->u_buffer_config, ec_api->verbose);
	}
	if (err){
		return WAN_EC_API_RC_FAILED;
	}
	return 0;
}

static int wanec_api_playout(wan_ec_dev_t *ec_dev, wan_ec_api_t *ec_api)
{
	wan_ec_t	*ec = NULL;
	int		fe_chan = ec_api->fe_chan,
			ec_chan = 0;

	WAN_ASSERT(ec_dev == NULL);
	WAN_ASSERT(ec_dev->ec == NULL);
	ec = ec_dev->ec;
	if (ec_dev->state != WAN_EC_STATE_CHAN_READY){
		DEBUG_EVENT(
		"ERROR: %s: Invalid Echo Canceller %s API state (%s)\n",
				ec_dev->devname,
				ec->name,
				WAN_EC_STATE_DECODE(ec->state));
		return WAN_EC_API_RC_INVALID_STATE;
	}

	if (ec_dev->fe_media == WAN_MEDIA_E1 && ec_api->fe_chan == 0){
		return WAN_EC_API_RC_NOACTION;
	}
	if (fe_chan > ec_dev->fe_stop_chan){
		DEBUG_EVENT(
		"ERROR: %s: Front-End channel number %d is out of range!\n",
				ec_dev->devname, fe_chan);
		return WAN_EC_API_RC_INVALID_CHANNELS;
	}
	if (ec_api->u_playout.port != WAN_EC_CHANNEL_PORT_SOUT &&
	    ec_api->u_playout.port != WAN_EC_CHANNEL_PORT_ROUT){
		DEBUG_EVENT(
		"ERROR: %s: Invalid Echo Canceller port value (%s)!\n",
				ec_dev->devname,
				WAN_EC_DECODE_CHANNEL_PORT(ec_api->u_playout.port));
		return WAN_EC_API_RC_INVALID_PORT;
	}
	ec_chan = wanec_fe2ec_channel(ec_dev, fe_chan);

	PRINT1(ec_api->verbose,
	"%s: Buffer Playout %s on fe_chan:%d ...\n",
				ec_dev->devname,
				(ec_api->cmd == WAN_EC_API_CMD_PLAYOUT_START) ?
							"Start" : "Stop",
				fe_chan);
	switch(ec_api->cmd){
	case WAN_EC_API_CMD_PLAYOUT_START:
		if (wanec_BufferPlayoutAdd(ec, ec_chan, &ec_api->u_playout, ec_api->verbose)){
			return WAN_EC_API_RC_FAILED;
		}
		if (wanec_BufferPlayoutStart(ec, ec_chan, &ec_api->u_playout, ec_api->verbose)){
			wanec_BufferPlayoutStop(ec, ec_chan, &ec_api->u_playout, ec_api->verbose);
			return WAN_EC_API_RC_FAILED;
		}
		ec->playout_verbose = ec_api->verbose;
		if (ec_api->u_playout.notifyonstop){
			wan_set_bit(WAN_EC_BIT_EVENT_PLAYOUT, &ec_dev->events);
		}
		break;
	case WAN_EC_API_CMD_PLAYOUT_STOP:
		wanec_BufferPlayoutStop(ec, ec_chan, &ec_api->u_playout, ec_api->verbose);
		/* FIXME: Once the Playout event was enabled, do not
		** disable it otherwise playout events for other
		** channels will be delayed
		**ec->playout_verbose = 0;
		**wan_clear_bit(WAN_EC_BIT_EVENT_PLAYOUT, &ec_dev->events);*/
		break;
	}

	return 0;
}

int wanpipe_ec_tdm_api_ioctl(void *p_ec_dev, wan_iovec_list_t *iovec_list) 
{
	wan_ec_api_t *ec_api;
	void *user_buffer;
	int err, user_buffer_len;
	wan_ec_dev_t *ec_dev = (wan_ec_dev_t*)p_ec_dev;
	
	user_buffer = iovec_list->iovec_list[0].iov_base;

	if (!user_buffer) {
		DEBUG_ERROR("Error: %s: IO memory buffer is NULL!\n", ec_dev->devname);
		return SANG_STATUS_INVALID_PARAMETER;
	}

	user_buffer_len = iovec_list->iovec_list[0].iov_len;

	if (sizeof(wan_ec_api_t) != user_buffer_len) {
		DEBUG_ERROR(
			"Error: %s: IO memory buffer length %d not equal sizeof(wan_ec_api_t):%d!\n",
			ec_dev->devname, user_buffer_len, sizeof(wan_ec_api_t));
		return SANG_STATUS_INVALID_PARAMETER;
	}

	ec_api = wan_malloc(sizeof(wan_ec_api_t));
	if (ec_api == NULL){
		DEBUG_ERROR(
    		"Error: %s: Failed to allocate memory (%d) [%s():%d]!\n",
				ec_dev->devname, (int)sizeof(wan_ec_api_t),
				__FUNCTION__, __LINE__);
		return SANG_STATUS_FAILED_ALLOCATE_MEMORY;
	}

	err = WAN_COPY_FROM_USER(ec_api, user_buffer, sizeof(*ec_api));
	if (err){
		DEBUG_ERROR(
       		"Error: %s: Failed to copy data from user space [%s():%d]!\n",
				ec_dev->devname, __FUNCTION__,__LINE__);
		wan_free(ec_api);
		return SANG_STATUS_IO_ERROR;
	}

	__wanec_ioctl(ec_dev, ec_api);

	err = WAN_COPY_TO_USER(user_buffer, ec_api, sizeof(*ec_api));
	if (err){
		DEBUG_ERROR(
			"Error: %s: Failed to copy data to user space [%s():%d]!\n",
				ec_dev->devname, __FUNCTION__,__LINE__);
		wan_free(ec_api);
		return SANG_STATUS_IO_ERROR;
	}

	wan_free(ec_api);

	return SANG_STATUS_SUCCESS;
}

static void __wanec_ioctl(wan_ec_dev_t *ec_dev, wan_ec_api_t *ec_api)
{
	wan_ec_t	*ec = NULL;
	WAN_IOCTL_RET_TYPE	err = 0;
	wan_smp_flag_t flags;

	WAN_DEBUG_FUNC_START;

#if defined(WANEC_DEBUG)
	ec_api->verbose |= (WAN_EC_VERBOSE_EXTRA1|WAN_EC_VERBOSE_EXTRA2);
#endif

	if (ec_dev->ec == NULL) {
		DEBUG_ERROR("Error: %s: 'ec_dev->ec' is NULL!\n",
			ec_dev->devname);
		return;
	}

	ec = ec_dev->ec;

	PRINT2(ec_api->verbose,	"%s(): %s: cmd: %s\n",
		__FUNCTION__, ec_api->devname, WAN_EC_API_CMD_DECODE(ec_api->cmd));

	ec_api->err = WAN_EC_API_RC_OK;
	if (ec_api->cmd == WAN_EC_API_CMD_GETINFO){
		ec_api->u_info.max_channels	= ec->max_ec_chans;
		ec_api->state			= ec->state;
		goto wanec_ioctl_exit;
	}else if (ec_api->cmd == WAN_EC_API_CMD_CONFIG_POLL){
		ec_api->state			= ec->state;
		goto wanec_ioctl_exit;
	}

	wan_mutex_lock(&ec->lock, &flags);

	if (wan_test_bit(WAN_EC_BIT_CRIT_DOWN, &ec_dev->critical)){
		DEBUG_EVENT("%s: Echo Canceller device is down!\n",
			ec_api->devname);
		ec_api->err = WAN_EC_API_RC_INVALID_DEV;
		goto wanec_ioctl_done;
	}

	if (wan_test_and_set_bit(WAN_EC_BIT_CRIT_CMD, &ec->critical)){
		DEBUG_EVENT("%s: Echo Canceller is busy!\n",
			ec_api->devname);
		ec_api->err = WAN_EC_API_RC_BUSY;
		goto wanec_ioctl_done;
	}

	switch(ec_api->cmd){
	case WAN_EC_API_CMD_GETINFO:
		ec_api->u_info.max_channels	= ec->max_ec_chans;
		ec_api->state			= ec->state;
		break;
	case WAN_EC_API_CMD_CONFIG:
		err = wanec_api_config(ec_dev, ec_api);
		break;
	case WAN_EC_API_CMD_CONFIG_POLL:
		ec_api->state			= ec->state;
		break;
	case WAN_EC_API_CMD_RELEASE:
		err = wanec_api_release(ec_dev, ec_api, ec_api->verbose);
		break;
	case WAN_EC_API_CMD_CHANNEL_OPEN:
		err = wanec_api_channel_open(ec_dev, ec_api);
		break;
	case WAN_EC_API_CMD_ENABLE:
	case WAN_EC_API_CMD_DISABLE:
		err = wanec_api_modify(ec_dev, ec_api);
		break;
	case WAN_EC_API_CMD_BYPASS_ENABLE:
	case WAN_EC_API_CMD_BYPASS_DISABLE:
		err = wanec_api_modify_bypass(ec_dev, ec_api);
		break;
	case WAN_EC_API_CMD_OPMODE:
		err = wanec_api_chan_opmode(ec_dev, ec_api);
		break;
	case WAN_EC_API_CMD_CHANNEL_MUTE:
	case WAN_EC_API_CMD_CHANNEL_UNMUTE:
		err = wanec_api_channel_mute(ec_dev, ec_api);
		break;
	case WAN_EC_API_CMD_MODIFY_CHANNEL:
		err = wanec_api_chan_custom(ec_dev, ec_api);
		break;
	case WAN_EC_API_CMD_TONE_ENABLE:
	case WAN_EC_API_CMD_TONE_DISABLE:
		ec_api->err = wanec_api_tone(ec_dev, ec_api);
		break;
	case WAN_EC_API_CMD_STATS:
	case WAN_EC_API_CMD_STATS_FULL:
		err = wanec_api_stats(ec_dev, ec_api);
		break;
	case WAN_EC_API_CMD_STATS_IMAGE:
		err = wanec_api_stats_image(ec_dev, ec_api);
		break;
	case WAN_EC_API_CMD_BUFFER_LOAD:
	case WAN_EC_API_CMD_BUFFER_UNLOAD:
		err = wanec_api_buffer(ec_dev, ec_api);
		break;
	case WAN_EC_API_CMD_PLAYOUT_START:
	case WAN_EC_API_CMD_PLAYOUT_STOP:
		err = wanec_api_playout(ec_dev, ec_api);
		break;
	case WAN_EC_API_CMD_MONITOR:
		err = wanec_api_monitor(ec_dev, ec_api);
		break;
	case WAN_EC_API_CMD_RELEASE_ALL:
		break;
	case WAN_EC_API_CMD_HWDTMF_REMOVAL_ENABLE:
	case WAN_EC_API_CMD_HWDTMF_REMOVAL_DISABLE:
		err = wanec_api_chan_dtmf_removal(ec_dev, ec_api);
		break;
	}
	
	if (err){
		PRINT2(ec_api->verbose,	"%s(): %s returns error! (Command: %s)\n",
			__FUNCTION__, ec_api->devname, WAN_EC_API_CMD_DECODE(ec_api->cmd));
		ec_api->err = err;
	}

	if (ec_api->err == WAN_EC_API_RC_INVALID_STATE){
		ec_api->state = ec->state;
	}

	wan_clear_bit(WAN_EC_BIT_CRIT_CMD, &ec->critical);

wanec_ioctl_done:
	wan_mutex_unlock(&ec->lock,&flags);

wanec_ioctl_exit:
	PRINT2(ec_api->verbose, "%s(): %s: cmd %s returns %d\n", __FUNCTION__,
		ec_api->devname, WAN_EC_API_CMD_DECODE(ec_api->cmd), ec_api->err);

	WAN_DEBUG_FUNC_END;
}

#if defined(__FreeBSD__) || defined(__OpenBSD__)
int wanec_ioctl(void *sc, void *data)
#elif defined(__LINUX__)
WAN_IOCTL_RET_TYPE wanec_ioctl(unsigned int cmd, void *data)
#elif defined(__WINDOWS__)
int wanec_ioctl(void *data)
#endif
{
	wan_ec_api_t	*ec_api = NULL;
	wan_ec_dev_t	*ec_dev = NULL;
	WAN_IOCTL_RET_TYPE err = 0;


	WAN_DEBUG_FUNC_START;

#if defined(__LINUX__)
	ec_api = wan_malloc(sizeof(wan_ec_api_t));
	if (ec_api == NULL){
		DEBUG_ERROR(
    		"Error: wanec: Failed to allocate memory (%d) [%s():%d]!\n",
				(int)sizeof(wan_ec_api_t), __FUNCTION__, __LINE__);
		return -EINVAL;
	}

	err = WAN_COPY_FROM_USER(
				ec_api,
				data,
				sizeof(wan_ec_api_t));
	if (err){
		DEBUG_ERROR(
       		"Error: wanec: Failed to copy data from user space [%s():%d]!\n",
				__FUNCTION__,__LINE__);
		wan_free(ec_api);
		return -EINVAL;
	}
#elif defined(__WINDOWS__)
	ec_api = (wan_ec_api_t*)data;
#else
# error "Unsupported OS"
#endif

#if defined(WANEC_DEBUG)
	ec_api->verbose |= (WAN_EC_VERBOSE_EXTRA1|WAN_EC_VERBOSE_EXTRA2);
#endif

	ec_dev = wanec_search(ec_api->devname);

	if (ec_dev == NULL){
		DEBUG_ERROR("Error: %s: Failed to find device [%s():%d]!\n",
			ec_api->devname, __FUNCTION__, __LINE__);
		ec_api->err = WAN_EC_API_RC_INVALID_DEV;
		goto wanec_ioctl_exit;
	}

	/* note that __wanec_ioctl() is a void-return function because
	 * the return code will be stored in 'ec_api->err'*/
	__wanec_ioctl(ec_dev, ec_api);

wanec_ioctl_exit:

#if defined(__LINUX__)
	err = WAN_COPY_TO_USER(
			data,
			ec_api,
			sizeof(wan_ec_api_t));
	if (err){
		DEBUG_ERROR(
		"Error: %s: Failed to copy data to user space [%s():%d]!\n",
			ec_api->devname, __FUNCTION__,__LINE__);
		wan_free(ec_api);
		return -EINVAL;
	}

	wan_free(ec_api);
#endif

	WAN_DEBUG_FUNC_END;
	return 0;
}


/*****************************************************************************/
#define my_isdigit(c)	((c) >= '0' && (c) <= '9')
static int wan_ec_devnum(char *ptr)
{
	int	num = 0, base = 1;
	int	i = 0;

	while(!my_isdigit(*ptr)) ptr++;
	while(my_isdigit(ptr[i])){
		i++;
		base = base * 10;
	}
	if (i){
		i=0;
		do {
			base = base / 10;
			num = num + (ptr[i]-'0') * base;
			i++;
		}while(my_isdigit(ptr[i]));
	}
	return num;
}



static void*
wanec_register(void *pcard, u_int32_t fe_port_mask, int max_fe_chans, int max_ec_chans, void *pconf)
{
	sdla_t			*card = (sdla_t*)pcard;
	wan_custom_conf_t	*conf = (wan_custom_conf_t*)pconf;
	wan_ec_t		*ec = NULL;
	wan_ec_dev_t		*ec_dev = NULL, *ec_dev_new = NULL;

	WAN_DEBUG_FUNC_START;

	WAN_LIST_FOREACH(ec, &wan_ec_head, next){
		WAN_LIST_FOREACH(ec_dev, &ec->ec_dev_head, next){
			if (ec_dev->card == NULL || ec_dev->card == card){
				DEBUG_ERROR("%s: Internal Error (%s:%d)\n",
					card->devname, __FUNCTION__,__LINE__);
				return NULL;
			}
			if (card->hw_iface.hw_same(ec_dev->card->hw, card->hw)){
				/* Current OCT6100 chip is already in use */
				break;
			}
		}
		if (ec_dev){
			break;
		}
	}

	if (ec && ec_dev){
		if(wan_test_bit(WAN_EC_BIT_CRIT_ERROR, &ec_dev->ec->critical)){
			DEBUG_ERROR("%s: Echo Canceller chip has Critical Error flag set!\n",
					card->devname);
			return NULL;
		}
	}

	ec_dev_new = wan_malloc(sizeof(wan_ec_dev_t));
	if (ec_dev_new == NULL){
		DEBUG_EVENT("%s: ERROR: Failed to allocate memory (%s:%d)!\n",
					card->devname,
					__FUNCTION__,__LINE__);
		return NULL;
	}
	memset(ec_dev_new, 0, sizeof(wan_ec_dev_t));

	if (ec_dev == NULL){

		/* First device for current Oct6100 chip */
		ec = wan_malloc(sizeof(wan_ec_t));
		if (ec == NULL){
			DEBUG_EVENT("%s: ERROR: Failed to allocate memory (%s:%d)!\n",
						card->devname,
						__FUNCTION__,__LINE__);
			return NULL;
		}
		memset(ec, 0, sizeof(wan_ec_t));

		ec->chip_no	= card->hw_iface.get_hwec_index(card->hw);
		if (ec->chip_no < 0){
			DEBUG_EVENT("%s: ERROR: Failed to get EC chip number!\n",
						card->devname);
			wan_free(ec);
			return NULL;
		}
		ec->state		= WAN_EC_STATE_RESET;
		ec->max_ec_chans	= (u_int16_t)max_ec_chans;
		wan_mutex_lock_init(&ec->lock, "wan_ec_mutex");
		sprintf(ec->name, "%s%d", WANEC_DEV_NAME, ec->chip_no);

		/* Copy EC chip custom configuration */
		if (conf->param_no){
			/* Copy custom oct parameter from user space */
			ec->custom_conf.params = wan_malloc(conf->param_no * sizeof(wan_custom_param_t));
			if (ec->custom_conf.params){
				int	err = 0;
				err = WAN_COPY_FROM_USER(
							ec->custom_conf.params,
							conf->params,
							conf->param_no * sizeof(wan_custom_param_t));
				ec->custom_conf.param_no = conf->param_no;
			}else{
				DEBUG_EVENT(
				"%s: WARNING: Skipping custom OCT6100 configuration (allocation failed)!\n",
							card->devname);
			}
		}

		Oct6100InterruptServiceRoutineDef(&ec->f_InterruptFlag);

		WAN_LIST_INIT(&ec->ec_dev_head);
		WAN_LIST_INIT(&ec->ec_confbridge_head);
		WAN_LIST_INSERT_HEAD(&wan_ec_head, ec, next);

	}else{
		ec = ec_dev->ec;
	}
	ec->usage++;
	ec_dev_new->ecdev_no		= wan_ec_devnum(card->devname);
	ec_dev_new->ec			= ec;
	ec_dev_new->name		= ec->name;
	ec_dev_new->card		= card;

	ec_dev_new->fe_media		= WAN_FE_MEDIA(&card->fe);
	ec_dev_new->fe_lineno		= WAN_FE_LINENO(&card->fe);
	ec_dev_new->fe_start_chan	= WAN_FE_START_CHANNEL(&card->fe);
	
	/** A600 - fe_max_chans 5
	 ** A700 - fe_max_chans 2	*/

#if 1
	if (IS_A600_CARD(card)) {
		ec_dev_new->fe_max_chans	= 5;
	} else if (IS_B601_CARD(card)) {
		if (IS_TE1_CARD(card)) {
			ec_dev_new->fe_max_chans  = WAN_FE_MAX_CHANNELS(&card->fe);
		} else {
			ec_dev_new->fe_max_chans	= 5;
		}
	} else if (IS_A700_CARD(card)) {
		ec_dev_new->fe_max_chans	= 2;	//max_line_no;	//
	} else {
		ec_dev_new->fe_max_chans	= WAN_FE_MAX_CHANNELS(&card->fe);	//max_line_no;	//
	}
#else
	/* FIXME: Should be removed */
	ec_dev_new->fe_max_chans	= max_fe_chans;
#endif
	ec_dev_new->fe_stop_chan	= ec_dev_new->fe_start_chan + ec_dev_new->fe_max_chans - 1;

	/* Feb 14, 2008
	** Ignore fe_port_mask for BRI cards. fe_port_mask is for full card,
	** but ec_dev created per module. In this case, we have always
	** 2 channels (1 and 2). Create fe_channel_map manually */

	if (fe_port_mask && ec_dev_new->fe_media != WAN_MEDIA_BRI){
		ec_dev_new->fe_channel_map	= fe_port_mask;
	}else{
		int	fe_chan = 0;
		for (fe_chan = ec_dev_new->fe_start_chan; fe_chan <= ec_dev_new->fe_stop_chan; fe_chan++){
			ec_dev_new->fe_channel_map |= (1 << fe_chan);
		}
	}

	if (!WAN_FE_TDMV_LAW(&card->fe)){
		if (WAN_FE_MEDIA(&card->fe) == WAN_MEDIA_T1){
			WAN_FE_TDMV_LAW(&card->fe) = WAN_TDMV_MULAW;
		}else if (WAN_FE_MEDIA(&card->fe) == WAN_MEDIA_E1){
			WAN_FE_TDMV_LAW(&card->fe) = WAN_TDMV_ALAW;
		}else{
			DEBUG_EVENT("%s: ERROR: Undefines MULAW/ALAW type!\n",
						card->devname);
		}
	}
	ec_dev_new->fe_tdmv_law		= WAN_FE_TDMV_LAW(&card->fe);
	ec_dev_new->state		= WAN_EC_STATE_RESET;

	/* Initialize hwec_bypass pointer */
	card->wandev.ec_enable	= wanec_enable;
	card->wandev.__ec_enable= __wanec_enable;
	card->wandev.ec_state   = wanec_state;
	card->wandev.ec_tdmapi_ioctl   = wanpipe_ec_tdm_api_ioctl;
	card->wandev.fe_ec_map	= 0;

	memcpy(ec_dev_new->devname, card->devname, sizeof(card->devname));
	sprintf(ec_dev_new->ecdev_name, "wp%dec", ec_dev_new->ecdev_no);

	wan_init_timer(	&ec_dev_new->timer,
			wanec_timer,
			(wan_timer_arg_t)ec_dev_new);

	WAN_LIST_INSERT_HEAD(&ec->ec_dev_head, ec_dev_new, next);

	DEBUG_EVENT("%s: Register EC interface %s (usage %d, max ec chans %d)!\n",
					ec_dev_new->devname,
					ec->name,
					ec->usage,
					ec->max_ec_chans);
	return (void*)ec_dev_new;
}

static int wanec_unregister(void *arg, void *pcard)
{
	sdla_t		*card = (sdla_t*)pcard;
	wan_ec_t	*ec = NULL;
	wan_ec_dev_t	*ec_dev = (wan_ec_dev_t*)arg;
	wan_smp_flag_t flags;

	WAN_ASSERT(ec_dev == NULL);
	WAN_ASSERT(ec_dev->ec == NULL);
	WAN_DEBUG_FUNC_START;

	ec = ec_dev->ec;
	wan_mutex_lock(&ec->lock,&flags);
	DEBUG_EVENT("%s: Unregister interface from %s (usage %d)!\n",
					card->devname,
					ec->name,
					ec->usage);

	wan_set_bit(WAN_EC_BIT_TIMER_KILL,(void*)&ec_dev->critical);
	wan_set_bit(WAN_EC_BIT_CRIT_DOWN,(void*)&ec_dev->critical);
	wan_set_bit(WAN_EC_BIT_CRIT,(void*)&ec_dev->critical);
	wan_del_timer(&ec_dev->timer);

	if (ec_dev->state != WAN_EC_STATE_RESET){
		DEBUG_EVENT(
		"%s: Releasing EC device (force)!\n",
					card->devname);
		wanec_api_release(ec_dev, NULL, wanec_verbose);
	}
	ec_dev->card = NULL;
	ec->usage--;

	if (WAN_LIST_FIRST(&ec->ec_dev_head) == ec_dev){
		WAN_LIST_FIRST(&ec->ec_dev_head) =
				WAN_LIST_NEXT(ec_dev, next);
		WAN_LIST_NEXT(ec_dev, next) = NULL;
	}else{
		WAN_LIST_REMOVE(ec_dev, next);
	}

	card->wandev.ec_enable	= NULL;
	card->wandev.__ec_enable	= NULL;

	ec_dev->ec = NULL;
	wan_free(ec_dev);
	wan_mutex_unlock(&ec->lock,&flags);

	/* FIXME: Remove character device */
	if (!ec->usage){

		if (WAN_LIST_FIRST(&wan_ec_head) == ec){
			WAN_LIST_FIRST(&wan_ec_head) =
					WAN_LIST_NEXT(ec, next);
			WAN_LIST_NEXT(ec, next) = NULL;
		}else{
			WAN_LIST_REMOVE(ec, next);
		}

		/* Free all custom configuration parameters */
		if (ec->custom_conf.params){
			wan_free(ec->custom_conf.params);
		}

		wan_free(ec);
	}
	WAN_DEBUG_FUNC_END;
	return 0;
}

#define WAN_EC_IRQ_TIMEOUT		(HZ*60)
#define WAN_EC_TONE_IRQ_TIMEOUT		(HZ/100)
#define WAN_EC_PLAYOUT_IRQ_TIMEOUT	(HZ/100)
static int wanec_isr(void *arg)
{
	wan_ec_dev_t	*ec_dev = (wan_ec_dev_t*)arg;

	if (ec_dev == NULL || ec_dev->ec == NULL) return 0;
	if (ec_dev->ec->state != WAN_EC_STATE_CHIP_OPEN){
		return 0;
	}
	if (wan_test_bit(WAN_EC_BIT_EVENT_TONE, &ec_dev->events)){
		/* Tone event is enabled */
		if ((SYSTEM_TICKS - ec_dev->ec->lastint_ticks) >= WAN_EC_TONE_IRQ_TIMEOUT){
			ec_dev->ec->lastint_ticks = SYSTEM_TICKS;
			return 1;
		}
		return 0;
	}
	if (wan_test_bit(WAN_EC_BIT_EVENT_PLAYOUT, &ec_dev->events)){
		/* Playout event is enabled */
		if ((SYSTEM_TICKS - ec_dev->ec->lastint_ticks) >= WAN_EC_PLAYOUT_IRQ_TIMEOUT){
			ec_dev->ec->lastint_ticks = SYSTEM_TICKS;
			return 1;
		}
		return 0;
	}

	if ((SYSTEM_TICKS - ec_dev->ec->lastint_ticks) > WAN_EC_IRQ_TIMEOUT){
		ec_dev->ec->lastint_ticks = SYSTEM_TICKS;
		return 1;
	}
	return 0;
}

static int wanec_poll(void *arg, void *pcard)
{
	wan_ec_t	*ec = NULL;
	wan_ec_dev_t	*ec_dev = (wan_ec_dev_t*)arg;
	int		err = 0;
	wan_smp_flag_t flags;

	WAN_ASSERT(ec_dev == NULL);
	WAN_ASSERT(ec_dev->ec == NULL);
	ec = ec_dev->ec;

	WAN_DEBUG_FUNC_START;

	if (!wan_mutex_trylock(&ec->lock,&flags)){
		return -EBUSY;
	}

	wan_clear_bit(WAN_EC_BIT_TIMER_RUNNING,(void*)&ec_dev->critical);
	if (wan_test_bit(WAN_EC_BIT_CRIT_DOWN, &ec_dev->critical) ||
	    wan_test_bit(WAN_EC_BIT_CRIT_ERROR, &ec_dev->critical)){
		ec_dev->poll_cmd = WAN_EC_POLL_NONE;
		wan_mutex_unlock(&ec->lock,&flags);
		return -EINVAL;
	}
	switch(ec_dev->poll_cmd){
	case WAN_EC_POLL_CHIPOPENPENDING:
		/* Chip open */
		if (ec->state != WAN_EC_STATE_CHIP_OPEN_PENDING){
			DEBUG_EVENT(
			"%s: Invalid EC state at ChipOpenPending poll command (%02X)\n",
						ec->name, ec->state);
			ec->state = WAN_EC_STATE_READY;
			ec_dev->state = ec->state;
			ec_dev->poll_cmd = WAN_EC_POLL_NONE;
			err = -EINVAL;
			goto wanec_poll_done;
		}
		if (wanec_ChipOpen(ec_dev, WAN_EC_VERBOSE_NONE)){
			ec->f_OpenChip.pbyImageFile = NULL;
			if (ec->pImageData) wan_vfree(ec->pImageData);
			ec->pImageData = NULL;
			/* Chip state is Ready state */
			ec->state = WAN_EC_STATE_READY;
			ec_dev->state = ec->state;
			ec_dev->poll_cmd = WAN_EC_POLL_NONE;
			err = -EINVAL;
			goto wanec_poll_done;
		}
		ec->state = WAN_EC_STATE_CHIP_OPEN;
		ec_dev->state = ec->state;

		ec->f_OpenChip.pbyImageFile = NULL;
		if (ec->pImageData) wan_vfree(ec->pImageData);
		ec->pImageData = NULL;
		break;

	case WAN_EC_POLL_INTR:
	default:	/* by default, can be only schedule from interrupt */
		if (ec->state != WAN_EC_STATE_CHIP_OPEN){
			break;
		}

		if (wan_test_bit(WAN_EC_BIT_CRIT_CMD, &ec->critical)) {
			err = -EINVAL;
			break;
		}

		/* Execute interrupt routine */
		err = wanec_ISR(ec, wanec_verbose);
		if (err < 0){
			wan_set_bit(WAN_EC_BIT_CRIT_ERROR, &ec->critical);
			wan_set_bit(WAN_EC_BIT_CRIT,(void*)&ec_dev->critical);
			err = -EINVAL;
			break;
		}
		break;
	}
	ec_dev->poll_cmd = WAN_EC_POLL_NONE;

wanec_poll_done:
	wan_mutex_unlock(&ec->lock,&flags);
	WAN_DEBUG_FUNC_END;
	return err;
}


static int wanec_event_ctrl(void *arg, void *pcard, wan_event_ctrl_t *event_ctrl)
{
	wan_ec_t	*ec = NULL;
	wan_ec_dev_t	*ec_dev = (wan_ec_dev_t*)arg;
	int		err = 0, enable = 0;
	wan_smp_flag_t flags;

	WAN_ASSERT(ec_dev == NULL);
	WAN_ASSERT(ec_dev->ec == NULL);
	WAN_ASSERT(event_ctrl  == NULL);
	ec = ec_dev->ec;

	wan_mutex_lock(&ec->lock,&flags);
	if (wan_test_and_set_bit(WAN_EC_BIT_CRIT_CMD, &ec->critical)){
		wan_mutex_unlock(&ec->lock,&flags);
		return -EBUSY;
	}

	switch(event_ctrl->type){
	case WAN_EVENT_EC_DTMF:
		{
			wanec_tone_config_t	tone;
			tone.id		= WP_API_EVENT_TONE_DTMF;
			tone.port_map	= WAN_EC_CHANNEL_PORT_SOUT;
			tone.type	= WAN_EC_TONE_PRESENT;
			enable = (event_ctrl->mode == WAN_EVENT_ENABLE) ? WAN_TRUE : WAN_FALSE;
			err=wanec_channel_tone(ec_dev, event_ctrl->channel, enable, &tone, wanec_verbose);
		}
		break;

	case WAN_EVENT_EC_FAX_DETECT:
		{
			wanec_tone_config_t	tone;
			tone.id		= WP_API_EVENT_TONE_FAXCALLING;
			tone.port_map	= WAN_EC_CHANNEL_PORT_SOUT;
			tone.type	= WAN_EC_TONE_PRESENT;
			enable = (event_ctrl->mode == WAN_EVENT_ENABLE) ? WAN_TRUE : WAN_FALSE;
			err=wanec_channel_tone(ec_dev, event_ctrl->channel, enable, &tone, wanec_verbose);
		}
		break;

	case WAN_EVENT_EC_H100_REPORT:
		if (event_ctrl->mode == WAN_EVENT_DISABLE){
			ec->ignore_H100 = 1;
		}else{
			ec->ignore_H100 = 0;
		}
		break;
	default:
		err = -EINVAL;
		break;
	}
	if (!err){
		wan_free(event_ctrl);
	}
	wan_clear_bit(WAN_EC_BIT_CRIT_CMD, &ec->critical);
	wan_mutex_unlock(&ec->lock,&flags);
	return err;
}

int wanec_init(void *arg)
{
#if defined(__WINDOWS__)
	if(wanec_iface.reg != NULL){
		DEBUG_WARNING("%s(): Warning: Initialization already done!\n", __FUNCTION__);
		return 0;
	}
#endif

		DEBUG_EVENT("%s %s.%s %s\n",
				wpec_fullname,
				WANPIPE_VERSION,
				WANPIPE_SUB_VERSION,
				wpec_copyright);

	/* Initialize WAN EC lip interface */
	wanec_iface.reg		= wanec_register;
	wanec_iface.unreg	= wanec_unregister;
	wanec_iface.ioctl	= NULL;
	wanec_iface.isr		= wanec_isr;
	wanec_iface.poll	= wanec_poll;
	wanec_iface.event_ctrl	= wanec_event_ctrl;

#if defined(CONFIG_WANPIPE_HWEC)
	register_wanec_iface (&wanec_iface);
# if defined(__FreeBSD__) || defined(__OpenBSD__)
	wp_cdev_reg(NULL, WANEC_DEV_NAME, wanec_ioctl);
# elif defined(__LINUX__) || defined(__WINDOWS__)
	wanec_create_dev();
# endif
#endif
	return 0;
}

int wanec_exit (void *arg)
{
#if defined(CONFIG_WANPIPE_HWEC)
# if defined(__FreeBSD__) || defined(__OpenBSD__)
	wp_cdev_unreg(WANEC_DEV_NAME);
# elif defined(__LINUX__) || defined(__WINDOWS__)
	wanec_remove_dev();
# endif
	unregister_wanec_iface();
#endif

	DEBUG_EVENT("WANEC Layer: Unloaded\n");
	return 0;
}

#if 0
int wanec_shutdown(void *arg)
{
	DEBUG_EVENT("Shutting down WANEC module ...\n");
	return 0;
}

int wanec_ready_unload(void *arg)
{
	DEBUG_EVENT("Is WANEC module ready to unload...\n");
	return 0;
}
#endif

#if !defined(__WINDOWS__)
WAN_MODULE_DEFINE(
		wanec,"wanec",
		"Alex Feldman <al.feldman@sangoma.com>",
		"Wanpipe Echo Canceller Layer - Sangoma Tech. Copyright 2006",
		"GPL",
		wanec_init, wanec_exit, /*wanec_shutdown, wanec_ready_unload,*/
		NULL);

WAN_MODULE_DEPEND(wanec, wanrouter, 1,
			WANROUTER_MAJOR_VER, WANROUTER_MAJOR_VER);

WAN_MODULE_DEPEND(wanec, wanpipe, 1,
			WANPIPE_MAJOR_VER, WANPIPE_MAJOR_VER);
#endif
