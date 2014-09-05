/***************************************************************************
 * sdla_remora_analog.c	 WANPIPE(tm) Multiprotocol WAN Link Driver. 
 *				AFT REMORA and FXO/FXS support module.
 *
 * Author: 	Jignesh Patel   <jpatel@sangoma.com>
 *
 * Copyright:	(c) 2005 Sangoma Technologies Inc.
 *
 *		This program is free software; you can redistribute it and/or
 *		modify it under the terms of the GNU General Public License
 *		as published by the Free Software Foundation; either version
 *		2 of the License, or (at your option) any later version.
 * ============================================================================
 * Oct 20, 2008	Jignesh Patel	Initial version based on sdla_remora_tdmv.c
 * Sep 22, 2009	Moises Silva    Added TDMV Alarm reporting on line disconnect
 ******************************************************************************
 */


/*******************************************************************************
**			   INCLUDE FILES
*******************************************************************************/

# include	"wanpipe_includes.h"
# include	"wanpipe_defines.h"
# include	"wanpipe.h"
# include 	"wanpipe_debug.h"
# include	"wanpipe_events.h"
# include	"sdla_remora.h"
# include	"sdla_remora_analog.h"
#if defined(CONFIG_PRODUCT_WANPIPE_TDM_VOICE)
#if !defined (__WINDOWS__)
# include 	"zapcompat.h"
#endif
# include "sdla_remora_tdmv.h"
#endif

#define AFT_TDM_API_SUPPORT 1
#if 1

extern int wp_init_voicedaa(sdla_fe_t *fe, int mod_no, int fast, int sane);
extern int wp_init_proslic(sdla_fe_t *fe, int mod_no, int fast, int sane);

#if defined(AFT_TDM_API_SUPPORT) || defined(AFT_API_SUPPORT)
static void wp_remora_voicedaa_tapper_check_hook(sdla_fe_t *fe, int mod_no);
#endif

static int ohdebounce = 16;
#define DEFAULT_CURRENT_THRESH 5  /*Anything under this is considered "on-hook" for active call on FXO */


/* January 13 2011 - David R - fixed ring debounce */
#define RING_DEBOUNCE_FIX	1

int
wp_tdmv_remora_proslic_recheck_sanity(sdla_fe_t	*fe, int mod_no)
{
 	sdla_t			*card = fe->card;
	
	wp_remora_fxs_t		*fxs = NULL;	
	
	int res;

#if defined(CONFIG_PRODUCT_WANPIPE_TDM_VOICE)
	wan_tdmv_t			*wan_tdmv = NULL;
	wp_tdmv_remora_t	*wr = NULL;
#endif


	if (card->u.aft.tdmv_zaptel_cfg) {
#if defined(CONFIG_PRODUCT_WANPIPE_TDM_VOICE)
		wan_tdmv = &card->wan_tdmv;
		WAN_ASSERT(wan_tdmv->sc == NULL);
		wr	= wan_tdmv->sc;	
		fxs = &wr->mod[mod_no].fxs;
#endif
	} else {
		fxs = &fe->rm_param.mod[mod_no].u.fxs;
		
	}

	/* Check loopback */
#if 0
#if defined(REG_SHADOW)
	res = wr-> fe->rm_param.reg2shadow[mod_no];
#else
        res = READ_RM_REG(mod_no, 8);
#endif
        if (res) {
                DEBUG_EVENT(
		"%s: Module %d: Ouch, part reset, quickly restoring reality (%02X) -- Comment out\n",
				wr->devname, mod_no, res);
                wp_init_proslic(fe, mod_no, 1, 1);
		return;
        }
#endif

#if defined(REG_SHADOW)
	res = fe->rm_param.reg1shadow[mod_no];
#else
	res = READ_RM_REG(mod_no, 64);
#endif
	if (!res && (res != fxs->lasttxhook)) {
#if defined(REG_SHADOW)
		res = fe->rm_param.reg2shadow[mod_no];
#else
		res = READ_RM_REG(mod_no, 8);
#endif
		if (res) {
			DEBUG_EVENT("%s:%d\n",__FUNCTION__,__LINE__);
			DEBUG_EVENT(
			"%s: Module %d: Ouch, part reset, quickly restoring reality\n",
					fe->name, mod_no+1);
			wp_init_proslic(fe, mod_no, 1, 1);
		} else {
			//DEBUG_EVENT("%s: mod %d Alarm %d intcout:%d \n", wr->devname,mod_no+1,wr->mod[mod_no].fxs.palarms,wr->intcount);
			if (fxs->palarms++ < MAX_ALARMS) {
				DEBUG_EVENT(
				"%s: Module %d: Power alarm, resetting!\n",
					fe->name, mod_no + 1);
				if (fxs->lasttxhook == 4)
					fxs->lasttxhook = 1;
				WRITE_RM_REG(mod_no, 64, fxs->lasttxhook);
			} else {
				if (fxs->palarms == MAX_ALARMS)
					DEBUG_EVENT(
					"%s: Module %d: Too many power alarms, NOT resetting!\n",
						fe->name, mod_no + 1);
			}
		}
	}
	return 0;
}
void
wp_tdmv_remora_voicedaa_recheck_sanity(sdla_fe_t *fe, int mod_no)
{
	int res;

	/* Check loopback */
#if defined(REG_SHADOW)
	res =fe->rm_param.reg2shadow[mod_no];
#else
        res = READ_RM_REG(mod_no, 34);
#endif
        if (!res) {
		DEBUG_EVENT("%s:%d\n",__FUNCTION__,__LINE__);
		DEBUG_EVENT(
		"%s: Module %d: Ouch, part reset, quickly restoring reality\n",
					fe->name, mod_no+1);
		wp_init_voicedaa(fe, mod_no, 1, 1);
	}
	return ;	
}

static int wp_tdmv_remora_voicedaa_check_hook(sdla_fe_t *fe, int mod_no)
{	

	sdla_t			*card = fe->card;
#ifdef AFT_TDM_API_SUPPORT
	wan_event_t	event;
#endif
#ifndef AUDIO_RINGCHECK
	unsigned char res;
#endif	
	signed char b;
	signed char lc;	
	int poopy = 0;
	wp_remora_fxo_t 	*fxo = NULL;
#if defined(CONFIG_PRODUCT_WANPIPE_TDM_VOICE)
	wan_tdmv_t		*wan_tdmv = NULL;
	wp_tdmv_remora_t	*wr = NULL;
#endif


	if (card->u.aft.tdmv_zaptel_cfg) {
#if defined(CONFIG_PRODUCT_WANPIPE_TDM_VOICE)
		wan_tdmv = &card->wan_tdmv;
		WAN_ASSERT(wan_tdmv->sc == NULL);
		wr	= wan_tdmv->sc;	
		fxo = &wr->mod[mod_no].fxo;
#endif
	} else {
		fxo = &fe->rm_param.mod[mod_no].u.fxo;
		
	}


	/* Try to track issues that plague slot one FXO's */
#if defined(REG_SHADOW)
	b = fe->rm_param.reg0shadow[mod_no];
#else
	b = READ_RM_REG(mod_no, 5);
#endif
	if ((b & 0x2) || !(b & 0x8)) {
		/* Not good -- don't look at anything else */
		DEBUG_TDMV("%s: Module %d: Poopy (%02x)!\n",
				fe->name, mod_no + 1, b); 
		poopy++;
	}
	b &= 0x9b;
	if (fxo->offhook) {
		if (b != 0x9){
			WRITE_RM_REG(mod_no, 5, 0x9);
		}
	} else {
		if (b != 0x8){
			WRITE_RM_REG(mod_no, 5, 0x8);
		}
	}
	if (poopy)
		return 0;
#ifndef AUDIO_RINGCHECK
	if (!fxo->offhook) {
#if defined(REG_SHADOW)
		res = fe->rm_param.reg0shadow[mod_no];
#else
		res = READ_RM_REG(mod_no, 5);
#endif
		if ((res & 0x60) && fxo->battery) {

#if !RING_DEBOUNCE_FIX
			fxo->ringdebounce += (fe->rm_param.wp_rm_chunk_size * 16);
#else
			fxo->ringdebounce += (fe->rm_param.wp_rm_chunk_size * 8);
#endif

			/*DEBUG_FALSE_RING("RING ON: (res & 0x60): 0x%X, fxo->battery: %d, fxo->ringdebounce: %d, fe->rm_param.wp_rm_chunk_size: %d, fxo->wasringing: %d!\n",
				(res & 0x60), fxo->battery, fxo->ringdebounce, fe->rm_param.wp_rm_chunk_size, fxo->wasringing);*/

#if !RING_DEBOUNCE_FIX
# define RING_DEBOUNCE_COUNTER	64
#else
# define RING_DEBOUNCE_COUNTER	56
#endif

			if (fxo->ringdebounce >= fe->rm_param.wp_rm_chunk_size * RING_DEBOUNCE_COUNTER) {
				if (!fxo->wasringing) {
					fxo->wasringing = 1;
					if (card->u.aft.tdmv_zaptel_cfg) {
#if defined(CONFIG_PRODUCT_WANPIPE_TDM_VOICE)	
						zt_hooksig(&wr->chans[mod_no], ZT_RXSIG_RING);
						DEBUG_TDMV("%s: Module %d: RING on span %d!\n",
								fe->name,
								mod_no + 1,
								wr->span.spanno);
#endif					
					} else {
#ifdef AFT_TDM_API_SUPPORT
						DEBUG_RM("%s: Module %d: RING!\n",
							fe->name,
								mod_no + 1);
						
						event.type	= WAN_EVENT_RM_RING_DETECT;
						event.channel	= mod_no+1;
						event.ring_mode	= WAN_EVENT_RING_PRESENT;
						if (card->wandev.event_callback.ringdetect){
							card->wandev.event_callback.ringdetect(card, &event);
						}	
#endif					
					}

				}
				fxo->ringdebounce = fe->rm_param.wp_rm_chunk_size * RING_DEBOUNCE_COUNTER;
			}
		} else {

			fxo->ringdebounce -= fe->rm_param.wp_rm_chunk_size * 4;

			if (fxo->ringdebounce <= 0) {
				if (fxo->wasringing) {
					/*DEBUG_FALSE_RING("RING OFF: (res & 0x60): 0x%X, fxo->battery: %d, fxo->ringdebounce: %d, fe->rm_param.wp_rm_chunk_size: %d, fxo->wasringing: %d!\n",
						(res & 0x60), fxo->battery, fxo->ringdebounce, fe->rm_param.wp_rm_chunk_size, fxo->wasringing);*/

					fxo->wasringing = 0;
					if (card->u.aft.tdmv_zaptel_cfg) {
#if defined(CONFIG_PRODUCT_WANPIPE_TDM_VOICE)
						zt_hooksig(&wr->chans[mod_no], ZT_RXSIG_OFFHOOK);
						DEBUG_TDMV("%s: Module %d: NO RING on span %d!\n",
								fe->name,
								mod_no + 1,
								wr->span.spanno);
#endif
					} else {
#ifdef AFT_TDM_API_SUPPORT
						DEBUG_RM("%s: Module %d: NO RING!\n",
							fe->name,
								mod_no + 1);
						
						event.type	= WAN_EVENT_RM_RING_DETECT;
						event.channel	= mod_no+1;
						event.ring_mode	= WAN_EVENT_RING_STOP;
						if (card->wandev.event_callback.ringdetect){
							card->wandev.event_callback.ringdetect(card, &event);
						}
#endif
					}
				}
				fxo->ringdebounce = 0;
			}
		}
	} /* if (!fxo->offhook) */
#endif
#if defined(REG_SHADOW)
	b = fe->rm_param.reg1shadow[mod_no];
#else
	b = READ_RM_REG(mod_no, 29);
#endif
#if 0 
	{
		static int count = 0;
		if (!(count++ % 100)) {
			printk("mod_no %d: Voltage: %d  Debounce %d\n", mod_no + 1, 
			       b, wr->mod[mod_no].fxo.battdebounce);
		}
	}
#endif	

	if (abs(b) <= 1){
		fe->rm_param.mod[mod_no].u.fxo.statusdebounce ++;
		if (fe->rm_param.mod[mod_no].u.fxo.statusdebounce >= FXO_LINK_DEBOUNCE){
			if (fe->rm_param.mod[mod_no].u.fxo.status != FE_DISCONNECTED){
				DEBUG_EVENT(
				"%s: Module %d: FXO Line is disconnected!\n",
								fe->name,
								mod_no + 1);
				fe->rm_param.mod[mod_no].u.fxo.status = FE_DISCONNECTED;
#ifdef AFT_TDM_API_SUPPORT
				if (!card->u.aft.tdmv_zaptel_cfg) {
					event.type	= WAN_EVENT_LINK_STATUS;
					event.channel	= mod_no+1;
					event.link_status= WAN_EVENT_LINK_STATUS_DISCONNECTED;
		
					if (card->wandev.event_callback.linkstatus) {
						card->wandev.event_callback.linkstatus(card, &event);
					}	
				}
#endif
#if defined(CONFIG_PRODUCT_WANPIPE_TDM_VOICE)
				if (card->u.aft.tdmv_zaptel_cfg) {
					zt_alarm_channel(&wr->chans[mod_no], ZT_ALARM_RED);
					DEBUG_TDMV("%s: Module %d: TDMV RED ALARM on span %d!\n",
							fe->name,
							mod_no + 1,
							wr->span.spanno);
				}
#endif

			}
			fe->rm_param.mod[mod_no].u.fxo.statusdebounce = FXO_LINK_DEBOUNCE;
		}
	}else{
		fe->rm_param.mod[mod_no].u.fxo.statusdebounce--;
		if (fe->rm_param.mod[mod_no].u.fxo.statusdebounce <= 0) {
			if (fe->rm_param.mod[mod_no].u.fxo.status != FE_CONNECTED){
				DEBUG_EVENT(
				"%s: Module %d: FXO Line is connected!\n",
								fe->name,
								mod_no + 1);
				fe->rm_param.mod[mod_no].u.fxo.status = FE_CONNECTED;
#ifdef AFT_TDM_API_SUPPORT
				if (!card->u.aft.tdmv_zaptel_cfg) {				
					event.type	= WAN_EVENT_LINK_STATUS;
					event.channel	= mod_no+1;
					event.link_status= WAN_EVENT_LINK_STATUS_CONNECTED;
	
					if (card->wandev.event_callback.linkstatus) {					
						card->wandev.event_callback.linkstatus(card, &event);				
					}	
				}
#endif
#if defined(CONFIG_PRODUCT_WANPIPE_TDM_VOICE)
				if (card->u.aft.tdmv_zaptel_cfg) {
					zt_alarm_channel(&wr->chans[mod_no], ZT_ALARM_NONE);
					DEBUG_TDMV("%s: Module %d: TDMV NONE ALARM on span %d!\n",
							fe->name,
							mod_no + 1,
							wr->span.spanno);
				}
#endif
			}
			fe->rm_param.mod[mod_no].u.fxo.statusdebounce = 0;
		}
	}
	/*If current measure is enabled check measure once we are Off-hook */
	if (fe->fe_cfg.cfg.remora.rm_lcm == WANOPT_YES) {
#if defined(REG_SHADOW)
		lc= fe->rm_param.reg3shadow[mod_no];
#else
		lc= READ_RM_REG(mod_no, 28);
#endif
		lc=(11*lc)/10; /* lc=current on the line, equivalent of lc*1.1*/
		if  (DEFAULT_CURRENT_THRESH > lc){
			if(fxo->offhook && !(fxo->i_debounce)){
				if (card->u.aft.tdmv_zaptel_cfg) {
#if defined(CONFIG_PRODUCT_WANPIPE_TDM_VOICE)
					DEBUG_TDMV(
					"%s: Module %d: FXO Line current %d !!\n",
									fe->name,								
									mod_no + 1,
									lc);
					
					DEBUG_TDMV(
					"%s: Module %d: Signalled On Hook span %d (%u) using LCM)\n",
									fe->name,
									mod_no + 1,
									wr->span.spanno,(u32)SYSTEM_TICKS);
					zt_hooksig(&wr->chans[mod_no], ZT_RXSIG_ONHOOK);
#endif
				} else {
#ifdef AFT_TDM_API_SUPPORT
					DEBUG_RM(
					"%s: Module %d: FXO Line current %d  (%u)!!\n",
									fe->name,								
									mod_no + 1,
									lc,(u32)SYSTEM_TICKS);
					DEBUG_RM("%s: Module %d: On-Hook status using LCM!\n",
								fe->name,
								mod_no + 1);
					event.type	= WAN_EVENT_RM_LC;
					event.channel	= mod_no+1;
					event.rxhook	= WAN_EVENT_RXHOOK_ON;
					if (card->wandev.event_callback.hook){
						card->wandev.event_callback.hook(
									card, &event);
					}
#endif
				}
				fxo->i_debounce=fe->rm_param.battdebounce; /*current debounce same as battry debounce*/
			}
			
		}else{
			fxo->i_debounce=fe->rm_param.battdebounce;
		}
		if(fxo->offhook) /*only during off-hook */{
			if (fxo->i_debounce)
				fxo->i_debounce--;
		}			
	}
	
	if (abs(b) < fe->rm_param.battthresh) {  /*Check for fe */
		fxo->nobatttimer++;
#if 0
		if (wr->mod[mod_no].fxo.battery)
			printk("Battery loss: %d (%d debounce)\n", 
				b, wr->mod[mod_no].fxo.battdebounce);
#endif
		if (fxo->battery && !fxo->battdebounce) {
			if (card->u.aft.tdmv_zaptel_cfg) {
#if defined(CONFIG_PRODUCT_WANPIPE_TDM_VOICE)
			DEBUG_TDMV(
			"%s: Module %d: NO BATTERY on span %d!\n",
						fe->name,
						mod_no + 1,
						wr->span.spanno);
#endif
			} else {
#ifdef AFT_TDM_API_SUPPORT
			DEBUG_RM(
			"%s: Module %d: NO BATTERY!\n",
						fe->name,
						mod_no + 1);
#endif
			}		
			fxo->battery =  0;
#ifdef	JAPAN
			if ((!fxo->ohdebounce) &&
		            fxo->offhook) {
					if (fe->fe_cfg.cfg.remora.rm_lcm != WANOPT_YES) {
						if (card->u.aft.tdmv_zaptel_cfg) {
#if defined(CONFIG_PRODUCT_WANPIPE_TDM_VOICE)
							zt_hooksig(&wr->chans[mod_no], ZT_RXSIG_ONHOOK);
							DEBUG_TDMV(
							"%s: Module %d: Signalled On Hook span %d\n",
										fe->name,
										mod_no + 1,
										wr->span.spanno);
#endif
						} else {
#ifdef AFT_TDM_API_SUPPORT
							DEBUG_RM("%s: Module %d: On-Hook status!\n",
									fe->name,
									mod_no + 1);
							event.type	= WAN_EVENT_RM_LC;
							event.channel	= mod_no+1;
							event.rxhook	= WAN_EVENT_RXHOOK_ON;
							if (card->wandev.event_callback.hook){
								card->wandev.event_callback.hook(
										card, &event);
							}
#endif
						}
				}
				
#ifdef	ZERO_BATT_RING
				fxo->onhook++;
#endif
			}
#else
			if (fe->fe_cfg.cfg.remora.rm_lcm != WANOPT_YES) {
				if (card->u.aft.tdmv_zaptel_cfg) {
#if defined(CONFIG_PRODUCT_WANPIPE_TDM_VOICE)
					DEBUG_TDMV(
					"%s: Module %d: Signalled On Hook span %d\n",
									fe->name,
									mod_no + 1,
									wr->span.spanno);
					zt_hooksig(&wr->chans[mod_no], ZT_RXSIG_ONHOOK);
#endif
				} else {
#ifdef AFT_TDM_API_SUPPORT
					DEBUG_RM("%s: Module %d: On-Hook status!\n",
								fe->name,
								mod_no + 1);
					event.type	= WAN_EVENT_RM_LC;
					event.channel	= mod_no+1;
					event.rxhook	= WAN_EVENT_RXHOOK_ON;
					if (card->wandev.event_callback.hook){
						card->wandev.event_callback.hook(
									card, &event);
					}
#endif
				}
			}
#endif
			fxo->battdebounce = fe->rm_param.battdebounce;
		} else if (!fxo->battery)
			fxo->battdebounce = fe->rm_param.battdebounce;
	} else if (abs(b) > fe->rm_param.battthresh) {
		if (!fxo->battery && !fxo->battdebounce) {
			if (card->u.aft.tdmv_zaptel_cfg) {
#if defined(CONFIG_PRODUCT_WANPIPE_TDM_VOICE)
			DEBUG_TDMV(
			"%s: Module %d: BATTERY on span %d (%s)!\n",
						fe->name,
						mod_no + 1,
						wr->span.spanno,
						(b < 0) ? "-" : "+");	
#endif
			} else {
#ifdef AFT_TDM_API_SUPPORT
			DEBUG_RM(
			"%s: Module %d: BATTERY (%s)!\n",
						fe->name,
						mod_no + 1,
						(b < 0) ? "-" : "+");
#endif
			}	
#ifdef	ZERO_BATT_RING
			if (fxo->onhook) {
				fxo->onhook = 0;
				if (card->u.aft.tdmv_zaptel_cfg) {
#if defined(CONFIG_PRODUCT_WANPIPE_TDM_VOICE)
					zt_hooksig(&wr->chans[mod_no], ZT_RXSIG_OFFHOOK);
					DEBUG_TDMV(
					"%s: Module %d: Signalled Off Hook span %d\n",
								fe->name,
								mod_no + 1,
								wr->span.spanno);
	
	
#endif
				} else {
#ifdef AFT_TDM_API_SUPPORT
					DEBUG_RM("%s: Module %d: Off-Hook status!\n",
								fe->name,
								mod_no + 1);
					event.type	= WAN_EVENT_RM_LC;
					event.channel	= mod_no+1;
					event.rxhook	= WAN_EVENT_RXHOOK_OFF;
					if (card->wandev.event_callback.hook){
						card->wandev.event_callback.hook(card, &event);
				
#endif
						}
				}
				
			}
#else
			if (card->u.aft.tdmv_zaptel_cfg) {
#if defined(CONFIG_PRODUCT_WANPIPE_TDM_VOICE)
				zt_hooksig(&wr->chans[mod_no], ZT_RXSIG_OFFHOOK);
				DEBUG_TDMV(
				"%s: Module %d: Signalled Off Hook span %d\n",
								fe->name,
								mod_no + 1,
								wr->span.spanno);

#endif
			} else {
#ifdef AFT_TDM_API_SUPPORT
				DEBUG_RM("%s: Module %d: Off-Hook status!\n",
						fe->name,
						mod_no + 1);
				event.type	= WAN_EVENT_RM_LC;
				event.channel	= mod_no+1;
				event.rxhook	= WAN_EVENT_RXHOOK_OFF;
				if (card->wandev.event_callback.hook){
					card->wandev.event_callback.hook(card, &event);
				}

#endif
			}			
#endif
			fxo->battery = 1;
			fxo->nobatttimer = 0;
			fxo->battdebounce = fe->rm_param.battdebounce;
		} else if (fxo->battery)
			fxo->battdebounce = fe->rm_param.battdebounce;

		if (fxo->lastpol >= 0) {
			if (b < 0) {
				fxo->lastpol = -1;
				fxo->polaritydebounce = POLARITY_DEBOUNCE;
			}
		} 
		if (fxo->lastpol <= 0) {
			if (b > 0) {
				fxo->lastpol = 1;
				fxo->polaritydebounce = POLARITY_DEBOUNCE;
			}
		}
	} else {
		/* It's something else... */
		fxo->battdebounce = fe->rm_param.battdebounce;
	}

	if (fxo->battdebounce)
		fxo->battdebounce--;
	if (fxo->polaritydebounce) {
	        fxo->polaritydebounce--;
		if (fxo->polaritydebounce < 1) {
			if (fxo->lastpol != fxo->polarity) {
				if (fxo->polarity){
					if (card->u.aft.tdmv_zaptel_cfg) {
#if defined(CONFIG_PRODUCT_WANPIPE_TDM_VOICE)
						DEBUG_TDMV(
						"%s: Module %d: Polarity reversed %d -> %d (%u)\n",
								fe->name,
								mod_no + 1,
								fxo->polarity, 
								fxo->lastpol,
								(u32)SYSTEM_TICKS);
						zt_qevent_lock(&wr->chans[mod_no],
									ZT_EVENT_POLARITY);
#endif
					} else {
#ifdef AFT_TDM_API_SUPPORT
						DEBUG_RM(
						"%s: Module %d: Polarity reversed (%d -> %d) (%ul)\n",
								fe->name, mod_no + 1,
								fe->rm_param.mod[mod_no].u.fxo.polarity, 
								fe->rm_param.mod[mod_no].u.fxo.lastpol,
								(unsigned int)SYSTEM_TICKS);
						event.type      = WAN_EVENT_RM_POLARITY_REVERSE;
						event.channel   = mod_no+1;

						if (fxo->polarity == 1 &&  fxo->lastpol == -1 ) {
							event.polarity_reverse  = WAN_EVENT_POLARITY_REV_POSITIVE_NEGATIVE;
						}else if (fxo->polarity == -1 &&  fxo->lastpol == 1 ){
							event.polarity_reverse  = WAN_EVENT_POLARITY_REV_NEGATIVE_POSITIVE;	
						}
						
						if (card->wandev.event_callback.polarityreverse){
								card->wandev.event_callback.polarityreverse(card, &event);
						}

#endif
					}
				}
				fxo->polarity =	fxo->lastpol;
			}
		}
	}
	
	return 0;
}
static int wp_tdmv_remora_proslic_check_hook(sdla_fe_t *fe, int mod_no)
{
	
	
	int		hook;
	char		res;
#ifdef AFT_TDM_API_SUPPORT	
	wan_event_t	event;
	unsigned char	stat2 = 0x00, stat3 = 0x00;
	unsigned char 	status;
#endif
	sdla_t			*card = fe->card;
	wp_remora_fxo_t 	*fxo = NULL;
	wp_remora_fxs_t		*fxs = NULL;

#if defined(CONFIG_PRODUCT_WANPIPE_TDM_VOICE)
	wan_tdmv_t		*wan_tdmv = NULL;
	wp_tdmv_remora_t	*wr = NULL;
#endif


	if (card->u.aft.tdmv_zaptel_cfg) {
#if defined(CONFIG_PRODUCT_WANPIPE_TDM_VOICE)
		wan_tdmv = &card->wan_tdmv;
		WAN_ASSERT(wan_tdmv->sc == NULL);
		wr	= wan_tdmv->sc;	
		fxo = &wr->mod[mod_no].fxo;
		fxs = &wr->mod[mod_no].fxs;
#endif
	} else {
		fxo = &fe->rm_param.mod[mod_no].u.fxo;
		fxs = &fe->rm_param.mod[mod_no].u.fxs;
		
	}
	
	/* For some reason we have to debounce the
	   hook detector.  */

#if defined(REG_SHADOW)
	res = fe->rm_param.reg0shadow[mod_no];
#ifdef AFT_TDM_API_SUPPORT
	if (!card->u.aft.tdmv_zaptel_cfg) {
		stat2 = fe->rm_param.reg3shadow[mod_no];
		stat3 = fe->rm_param.reg4shadow[mod_no];
	}
#endif
#else
	res = READ_RM_REG(mod_no, 68);
#ifdef AFT_TDM_API_SUPPORT
	if (!card->u.aft.tdmv_zaptel_cfg) {
		stat2 = READ_RM_REG(mod_no, 19);
		stat3 = READ_RM_REG(mod_no, 20);
	}
#endif
#endif
#ifdef AFT_TDM_API_SUPPORT
	status=res;
#endif

	hook = (res & 1);
	if (hook != fxs->lastrxhook) {
		/* Reset the debounce (must be multiple of 4ms) */
		fxs->debounce = 4 * (4 * 8);
		DEBUG_TDMV(
		"%s: Module %d: Resetting debounce hook %d, %d\n",
				fe->name, mod_no + 1, hook,
				fxs->debounce);
	} else {
		if (fxs->debounce > 0) {
			fxs->debounce-= 16 * fe->rm_param.wp_rm_chunk_size;
			DEBUG_TDMV(
			"%s: Module %d: Sustaining hook %d, %d\n",
					fe->name, mod_no + 1,
					hook, fxs->debounce);
			if (!fxs->debounce) {
				DEBUG_TDMV(
				"%s: Module %d: Counted down debounce, newhook: %d\n",
							fe->name,
							mod_no + 1,
							hook);
				fxs->debouncehook = hook;
			}
			if (!fxs->oldrxhook && fxs->debouncehook) {
				/* Off hook */
				

				if (card->u.aft.tdmv_zaptel_cfg) {
#if defined(CONFIG_PRODUCT_WANPIPE_TDM_VOICE)
					DEBUG_TDMV(
					"%s: Module %d: Going off hook\n",
								fe->name, mod_no + 1);
					zt_hooksig(&wr->chans[mod_no], ZT_RXSIG_OFFHOOK);

#endif
				} else {
#ifdef AFT_TDM_API_SUPPORT
						if(fxs->itimer) {
							fxs->itimer=0;
							DEBUG_RM(
							"RM: %s: Module %d: Flash status!\n",
									fe->name, mod_no+1);
							event.type	= WAN_EVENT_RM_LC;
							event.channel	= mod_no+1;
							event.rxhook	= WAN_EVENT_RXHOOK_FLASH;
							if (card->wandev.event_callback.hook){
								card->wandev.event_callback.hook(
											card,
											&event);
							}
						} else {
								DEBUG_RM(
								"RM: %s: Module %d: Off-Hook  status!\n",
										fe->name, mod_no+1);
								fxs->itimer=0;
								event.type	= WAN_EVENT_RM_LC;
								event.channel	= mod_no+1;
								event.rxhook	= WAN_EVENT_RXHOOK_OFF;
								if (card->wandev.event_callback.hook){
									card->wandev.event_callback.hook(
												card,
												&event);
								}
						}
#endif	
				}
				
#if 0
				if (robust)
					wp_init_proslic(wc, card, 1, 0, 1);
#endif
				fxs->oldrxhook = 1;
			
			} else if (fxs->oldrxhook && !fxs->debouncehook) {
				/* On hook */

				if (card->u.aft.tdmv_zaptel_cfg) {
#if defined(CONFIG_PRODUCT_WANPIPE_TDM_VOICE)
					DEBUG_TDMV(
					"%s: Module %d: Going on hook\n",
							fe->name, mod_no + 1);
					zt_hooksig(&wr->chans[mod_no], ZT_RXSIG_ONHOOK);
#endif
				} else {
#ifdef AFT_TDM_API_SUPPORT
							if(!fxs->itimer && fxs->rxflashtime) {
								fxs->itimer=(fxs->rxflashtime)*(fe->rm_param.wp_rm_chunk_size);
							} else {
									DEBUG_RM(
									"RM: %s: Module %d: On-hook status!\n",
										fe->name, mod_no+1);
									fxs->itimer = 0;
									event.type	= WAN_EVENT_RM_LC;
									event.channel	= mod_no+1;
									event.rxhook	= WAN_EVENT_RXHOOK_ON;
									if (card->wandev.event_callback.hook){
										card->wandev.event_callback.hook(
													card,
													&event);
									}
									/*need for mwi tranfer in TDMAPI mode */
									fxs->lasttxhook=fxs->idletxhookstate;
									fxs->lasttxhook_update=1;


							}


					
#endif
				}
				
				fxs->oldrxhook = 0;
			}
		}
	}
#ifdef AFT_TDM_API_SUPPORT
	/* RING-TRIP is special event passed if FXS goes off during ringing state */
	if (!card->u.aft.tdmv_zaptel_cfg) {

		if (stat2) {
			if (stat2 & 0x01){
					DEBUG_RM(
					"%s: Module %d: Ring TRIP interrupt pending!\n",
						fe->name, mod_no+1);
#if 0
					if (card->wandev.fe_notify_iface.hook_state){
						card->wandev.fe_notify_iface.hook_state(
								fe, mod_no, status & 0x01);
					}
#endif
					/*
					A ring trip event signals that the terminal equipment has gone
					off-hook during the ringing state.
					At this point the application should stop the ring because the
					call was answered.
					*/
					event.type	= WAN_EVENT_RM_RING_TRIP;
					event.channel	= mod_no+1;
					if (status & 0x02){
					DEBUG_RM(
						"%s: Module %d: Ring Trip detect occured!\n",
								fe->name, mod_no+1);
						event.ring_mode	= WAN_EVENT_RING_TRIP_PRESENT;
					}else{
					DEBUG_RM(
						"%s: Module %d: Ring Trip detect not occured!\n",
								fe->name, mod_no+1);
						event.ring_mode	= WAN_EVENT_RING_TRIP_STOP;
					}
					if (card->wandev.event_callback.ringtrip){
						card->wandev.event_callback.ringtrip(card, &event);
					}
				}
			if (stat2 & 0x80) {
				DEBUG_EVENT("%s: Module %d: Power Alarm Q6!\n",
						fe->name, mod_no+1);
			}
			if (stat2 & 0x40) {
				DEBUG_EVENT("%s: Module %d: Power Alarm Q5!\n",
						fe->name, mod_no+1);
			}
			if (stat2 & 0x20) {
				DEBUG_EVENT("%s: Module %d: Power Alarm Q4!\n",
						fe->name, mod_no+1);
			}
			if (stat2 & 0x10) {
				DEBUG_EVENT("%s: Module %d: Power Alarm Q3!\n",
						fe->name, mod_no+1);
			}
			if (stat2 & 0x08) {
				DEBUG_EVENT("%s: Module %d: Power Alarm Q2!\n",
						fe->name, mod_no+1);
			}
			if (stat2 & 0x04) {
				DEBUG_EVENT("%s: Module %d: Power Alarm Q1!\n",
						fe->name, mod_no+1);
			}
		DEBUG_RM(
		"%s: Module %d: Reg[64]=%02X Reg[68]=%02X\n",
			fe->name, mod_no+1,
			READ_RM_REG(mod_no,64),
			status);
		WRITE_RM_REG(mod_no, 19, stat2);
		}

		if (stat3){
			if (stat3 & 0x1){
				wp_remora_read_dtmf(fe, mod_no);
			}
			WRITE_RM_REG(mod_no, 20, stat3);
		}

	}
#endif

	fxs->lastrxhook = hook;
	return 0;
}

int wp_tdmv_remora_check_hook(sdla_fe_t *fe, int mod_no)
{
	sdla_t			*card = fe->card;
	
#if defined(CONFIG_PRODUCT_WANPIPE_TDM_VOICE)
	wan_tdmv_t		*wan_tdmv = NULL;	
	wp_tdmv_remora_t	*wr = NULL;
#endif
	
#if defined(CONFIG_PRODUCT_WANPIPE_TDM_VOICE)
	if (card->u.aft.tdmv_zaptel_cfg) {
		wan_tdmv = &card->wan_tdmv;
		WAN_ASSERT(wan_tdmv->sc == NULL);
		wr	= wan_tdmv->sc;
	}
#endif

	if (fe->rm_param.mod[mod_no].type == MOD_TYPE_FXS) {
		wp_tdmv_remora_proslic_check_hook(fe, mod_no);
	} else if (fe->rm_param.mod[mod_no].type == MOD_TYPE_FXO) {
		if (card->u.aft.tdmv_zaptel_cfg) {
			wp_tdmv_remora_voicedaa_check_hook(fe, mod_no);
		}else{ 
			if (fe->fe_cfg.cfg.remora.rm_mode == WAN_RM_TAPPING) {
				
				wp_remora_voicedaa_tapper_check_hook(fe, mod_no);	
			} else {
				wp_tdmv_remora_voicedaa_check_hook(fe, mod_no);	
			}
		}
	}
	
	return 0;
}




#endif


#if defined(AFT_TDM_API_SUPPORT) || defined(AFT_API_SUPPORT)

static void wp_remora_voicedaa_tapper_check_hook(sdla_fe_t *fe, int mod_no)
{
	
	sdla_t		*card = NULL;
	wan_event_t	event;
	
	unsigned char res;

	signed char b;
	
	int poopy = 0;
	
	WAN_ASSERT1(fe->card == NULL);
	card	= (sdla_t*)fe->card;

	/* Try to track issues that plague slot one FXO's */
	//b = READ_RM_REG(mod_no, 5);
#if defined(REG_SHADOW)
	b = fe->rm_param.reg0shadow[mod_no];
#else
	b = READ_RM_REG(mod_no, 5);
#endif

	if ((b & 0x2) || !(b & 0x8)) {
		/* Not good -- don't look at anything else */
		DEBUG_RM("%s: Module %d: Poopy (%02x)!\n",
			 fe->name, mod_no + 1, b); 
		poopy++;
	}
	
	if (poopy)
		return;

	if (!fe->rm_param.mod[mod_no].u.fxo.offhook) {
	//	res = READ_RM_REG(mod_no, 5);
#if defined(REG_SHADOW)
	res = fe->rm_param.reg0shadow[mod_no];
#else
	res = READ_RM_REG(mod_no, 5);
#endif
		if ((res & 0x60)) {
			fe->rm_param.mod[mod_no].u.fxo.ringdebounce += (fe->rm_param.wp_rm_chunk_size * 4);
			if (fe->rm_param.mod[mod_no].u.fxo.ringdebounce >= fe->rm_param.wp_rm_chunk_size * 16) {	
	
				if (!fe->rm_param.mod[mod_no].u.fxo.wasringing) {
					fe->rm_param.mod[mod_no].u.fxo.wasringing = 1;
					
					DEBUG_RM("%s: Module %d: RING!\n",
						fe->name,
							mod_no + 1);
					
					event.type	= WAN_EVENT_RM_RING_DETECT;
					event.channel	= mod_no+1;
					event.ring_mode	= WAN_EVENT_RING_PRESENT;
					if (card->wandev.event_callback.ringdetect){
						card->wandev.event_callback.ringdetect(card, &event);
					}	
				}
				
				fe->rm_param.mod[mod_no].u.fxo.ringdebounce = fe->rm_param.wp_rm_chunk_size * 16;
	
			}
		} else {
	
			fe->rm_param.mod[mod_no].u.fxo.ringdebounce -= fe->rm_param.wp_rm_chunk_size * 1;
			if (fe->rm_param.mod[mod_no].u.fxo.ringdebounce <= 0) {
				if (fe->rm_param.mod[mod_no].u.fxo.wasringing) {
				
					fe->rm_param.mod[mod_no].u.fxo.wasringing = 0;
					
					DEBUG_RM("%s: Module %d: NO RING!\n",
						fe->name,
							mod_no + 1);
					
					event.type	= WAN_EVENT_RM_RING_DETECT;
					event.channel	= mod_no+1;
					event.ring_mode	= WAN_EVENT_RING_STOP;
					if (card->wandev.event_callback.ringdetect){
						card->wandev.event_callback.ringdetect(card, &event);
					}	
					
				}
				fe->rm_param.mod[mod_no].u.fxo.ringdebounce = 0;
			}
		}
	}						    
	

#if defined(REG_SHADOW)
	b = fe->rm_param.reg1shadow[mod_no];
#else
	b = READ_RM_REG(mod_no, 29);
#endif
	
	if (abs(b) <= FXO_LINK_THRESH) {
		
		fe->rm_param.mod[mod_no].u.fxo.statusdebounce ++;
		if (fe->rm_param.mod[mod_no].u.fxo.statusdebounce >= FXO_LINK_DEBOUNCE) {
			if (fe->rm_param.mod[mod_no].u.fxo.status != FE_DISCONNECTED) {
				DEBUG_EVENT(
				"%s: Module %d: FXO Line is disconnected (tapper)!\n",
								fe->name,
								mod_no + 1);
				fe->rm_param.mod[mod_no].u.fxo.status = FE_DISCONNECTED;
				
				event.type	= WAN_EVENT_LINK_STATUS;
				event.channel	= mod_no+1;
				event.link_status= WAN_EVENT_LINK_STATUS_DISCONNECTED;
	
				if (card->wandev.event_callback.linkstatus) {
					card->wandev.event_callback.linkstatus(card, &event);
				}	
			}		
			
			fe->rm_param.mod[mod_no].u.fxo.statusdebounce = FXO_LINK_DEBOUNCE;
		}
	} else {
			
		fe->rm_param.mod[mod_no].u.fxo.statusdebounce--;
		if (fe->rm_param.mod[mod_no].u.fxo.statusdebounce <= 0) {
			if (fe->rm_param.mod[mod_no].u.fxo.status != FE_CONNECTED) {
				DEBUG_EVENT("%s: Module %d: FXO Line is connected!\n",
								fe->name,
								mod_no + 1);
				fe->rm_param.mod[mod_no].u.fxo.status = FE_CONNECTED;

				event.type	= WAN_EVENT_LINK_STATUS;
				event.channel	= mod_no+1;
				event.link_status= WAN_EVENT_LINK_STATUS_CONNECTED;

				if (card->wandev.event_callback.linkstatus) {					
					card->wandev.event_callback.linkstatus(card, &event);				
				}	

			}
			fe->rm_param.mod[mod_no].u.fxo.statusdebounce = 0;
		}
	}
	


	if (abs(b) < fe->fe_cfg.cfg.remora.ohthresh) {
		if (!fe->rm_param.mod[mod_no].u.fxo.going_offhook) {
			/* if we were debouncing towards on_hook, reset timer */
			
			fe->rm_param.mod[mod_no].u.fxo.going_offhook = 1;
			fe->rm_param.mod[mod_no].u.fxo.ohdebounce = ohdebounce;	
			
		}
		if (!fe->rm_param.mod[mod_no].u.fxo.offhook) {
			fe->rm_param.mod[mod_no].u.fxo.ohdebounce--;
			
			if (!fe->rm_param.mod[mod_no].u.fxo.ohdebounce) {
				DEBUG_RM("%s: Module %d: OFF-HOOK!\n",
						fe->name,
						mod_no + 1);

				event.type	= WAN_EVENT_RM_LC;
				event.channel	= mod_no+1;
				event.rxhook	= WAN_EVENT_RXHOOK_OFF;
				if (card->wandev.event_callback.hook) {
					card->wandev.event_callback.hook(card, &event);
				}	
				
				fe->rm_param.mod[mod_no].u.fxo.offhook = 1;
				fe->rm_param.mod[mod_no].u.fxo.ohdebounce = ohdebounce;
			}
		} 
	} else {
		if (fe->rm_param.mod[mod_no].u.fxo.going_offhook) {
			/* if we were debouncing towards off_hook, reset timer */
			
			fe->rm_param.mod[mod_no].u.fxo.going_offhook = 0;
			fe->rm_param.mod[mod_no].u.fxo.ohdebounce = ohdebounce;	
			
		}

		
		if (fe->rm_param.mod[mod_no].u.fxo.offhook) {
			fe->rm_param.mod[mod_no].u.fxo.ohdebounce--;

			if (!fe->rm_param.mod[mod_no].u.fxo.ohdebounce) {
			/*For the first On-hook event after line connected, pass connected event before ON-hook !*/

				if (fe->rm_param.mod[mod_no].u.fxo.status != FE_CONNECTED) {
					DEBUG_EVENT("%s: Module %d: FXO Line is connected!\n",
									fe->name,
									mod_no + 1);
					fe->rm_param.mod[mod_no].u.fxo.status = FE_CONNECTED;
	
					event.type	= WAN_EVENT_LINK_STATUS;
					event.channel	= mod_no+1;
					event.link_status= WAN_EVENT_LINK_STATUS_CONNECTED;
	
					if (card->wandev.event_callback.linkstatus) {					
						card->wandev.event_callback.linkstatus(card, &event);				
					}	

				}
			

				DEBUG_RM("%s: Module %d: ON-HOOK!\n",
					    	fe->name,
	 					mod_no + 1);

				event.type	= WAN_EVENT_RM_LC;
				event.channel	= mod_no+1;
				event.rxhook	= WAN_EVENT_RXHOOK_ON;
				if (card->wandev.event_callback.hook){
					card->wandev.event_callback.hook(
							card, &event);
				}
				fe->rm_param.mod[mod_no].u.fxo.offhook = 0;
				fe->rm_param.mod[mod_no].u.fxo.ohdebounce = ohdebounce;
			}
		} 

	}

	
	/*
	DEBUG_EVENT("%s: oh_debounce:%d ring_debounce:%d\n", 
			    fe->name,
	      		  fe->rm_param.mod[mod_no].u.fxo.tapper_ohdebounce,
      				fe->rm_param.mod[mod_no].u.fxo.ringdebounce);
	*/

	return;
}
#endif


/*
 ******************************************************************************
 *				wp_tdmv_remora_rx_tx_span_common()	
 *
 * Description:
 * Arguments:
 * Returns:
 ******************************************************************************
 */


int wp_tdmv_remora_rx_tx_span_common(void *pcard )
{
	sdla_t				*card= (sdla_t*)pcard;
	sdla_fe_t			*fe = &card->fe;
#if defined(CONFIG_PRODUCT_WANPIPE_TDM_VOICE)		
	wan_tdmv_t			*wan_tdmv = &card->wan_tdmv;
    wp_tdmv_remora_t	*wr = NULL;
#endif
#ifdef AFT_TDM_API_SUPPORT
	wan_event_t	event;
#endif
	wp_remora_fxo_t 	*fxo = NULL;
	wp_remora_fxs_t		*fxs = NULL;
	u_int16_t			 x = 0;
#if defined(CONFIG_PRODUCT_WANPIPE_TDM_VOICE)
	if (card->u.aft.tdmv_zaptel_cfg) {	
		WAN_ASSERT(wan_tdmv->sc == NULL);
		wr = wan_tdmv->sc;
	}
#endif
	

#if 1
	
	for (x = 0; x < fe->rm_param.max_fe_channels; x++) {
		if (!wan_test_bit(x, &fe->rm_param.module_map)){ /* should be ok confirm with Nenad*/
			continue;
		}
		if (card->u.aft.tdmv_zaptel_cfg) {
#if defined(CONFIG_PRODUCT_WANPIPE_TDM_VOICE)		
			fxo = &wr->mod[x].fxo;
			fxs = &wr->mod[x].fxs;
#endif
		} else {
			fxo = &fe->rm_param.mod[x].u.fxo;
			fxs = &fe->rm_param.mod[x].u.fxs;
		}


		if (fe->rm_param.mod[x].type == MOD_TYPE_FXS){
#if defined(REG_WRITE_SHADOW)
			if (fxs->lasttxhook_update){

				DBG_BATTERY_REMOVAL("%s():line:%d: fxs ptr: 0x%p, fxs->lasttxhook: %d (0x%x)\n",
						__FUNCTION__, __LINE__, fxs, fxs->lasttxhook, fxs->lasttxhook);

				WRITE_RM_REG(x, 64, fxs->lasttxhook);
				fxs->lasttxhook_update = 0;
				continue;
			}
#endif
#ifdef AFT_TDM_API_SUPPORT
		if (!card->u.aft.tdmv_zaptel_cfg && fxs->itimer) {
			fxs->itimer-=fe->rm_param.wp_rm_chunk_size;
			if(fxs->itimer==0){
				/*definately on-hook as itimer is expired*/
				DEBUG_TDMAPI("%s: Module %d: On-hook itimer expired!\n",
						fe->name,
						x + 1);
				event.type	= WAN_EVENT_RM_LC;
				event.channel	= x+1;
				event.rxhook	= WAN_EVENT_RXHOOK_ON;
				if (card->wandev.event_callback.hook){
					card->wandev.event_callback.hook(
							card, &event);
				}
				fxs->lasttxhook =fxs->idletxhookstate;
				fxs->lasttxhook_update=1;
			}

		}
#endif

			if (fxs->lasttxhook == 0x4) {
				/* RINGing, prepare for OHT */
				fxs->ohttimer = OHT_TIMER << 3;
				if (fe->fe_cfg.cfg.remora.reversepolarity){
					/* OHT mode when idle */
					fe->rm_param.mod[x].u.fxs.idletxhookstate = 0x6;
				}else{
					fe->rm_param.mod[x].u.fxs.idletxhookstate = 0x2; 
				}
			} else {
				if (fxs->ohttimer) {
					fxs->ohttimer-= fe->rm_param.wp_rm_chunk_size;
					if (!fxs->ohttimer) {
						if (fe->fe_cfg.cfg.remora.reversepolarity){
							/* Switch to active */
							fe->rm_param.mod[x].u.fxs.idletxhookstate = 0x5;
						}else{
							fe->rm_param.mod[x].u.fxs.idletxhookstate = 0x1;
						}
						if ((fxs->lasttxhook == 0x2) || (fxs->lasttxhook == 0x6)) {
							/* Apply the change if appropriate */
							if (fe->fe_cfg.cfg.remora.reversepolarity){ 
								fxs->lasttxhook = 0x5;
							}else{
								fxs->lasttxhook = 0x1;
							}
				
							DBG_BATTERY_REMOVAL("%s():line:%d: fxs ptr: 0x%p, fxs->lasttxhook: %d (0x%x)\n",
								__FUNCTION__, __LINE__, fxs, fxs->lasttxhook, fxs->lasttxhook);

							WRITE_RM_REG(x, 64, fxs->lasttxhook);
						}
					}
				}
			}

		} else if (fe->rm_param.mod[x].type == MOD_TYPE_FXO ) {
#if defined(CONFIG_PRODUCT_WANPIPE_TDM_VOICE)
			if (fxo->echotune && card->u.aft.tdmv_zaptel_cfg){
				DEBUG_RM("%s: Module %d: Setting echo registers: \n",
							fe->name, x);

				/* Set the ACIM register */
				WRITE_RM_REG(x, 30, fxo->echoregs.acim);

				/* Set the digital echo canceller registers */
				WRITE_RM_REG(x, 45, fxo->echoregs.coef1);
				WRITE_RM_REG(x, 46, fxo->echoregs.coef2);
				WRITE_RM_REG(x, 47, fxo->echoregs.coef3);
				WRITE_RM_REG(x, 48, fxo->echoregs.coef4);
				WRITE_RM_REG(x, 49, fxo->echoregs.coef5);
				WRITE_RM_REG(x, 50, fxo->echoregs.coef6);
				WRITE_RM_REG(x, 51, fxo->echoregs.coef7);
				WRITE_RM_REG(x, 52, fxo->echoregs.coef8);

				DEBUG_RM("%s: Module %d: Set echo registers successfully\n",
						fe->name, x);
				fxo->echotune = 0;
			}
#endif
#if defined(REG_WRITE_SHADOW)
			if (fe->rm_param.reg0shadow_update[x]){
				/* Read first shadow reg */
				WRITE_RM_REG(x, 5, fe->rm_param.reg0shadow[x]);
				fe->rm_param.reg0shadow_update[x] = 0;
			}
#endif
		}

#if defined(NEW_PULSE_DIALING)
		if (fe->fe_cfg.cfg.remora.fxs_pulsedialing == WANOPT_YES){
			/*
			** Alex 31 Mar, 2006
			** Check for HOOK status every interrupt
			** (in pulse mode is very critical) */
			wp_tdmv_remora_check_hook(fe, x);
		}
#else
#ifdef PULSE_DIALING
		/*
		** Alex 31 Mar, 2006
		** Check for HOOK status every interrupt
		** (in pulse mode is very critical) */
		wp_tdmv_remora_check_hook(fe, x);
#endif
#endif
	}

	x = fe->rm_param.intcount % MAX_REMORA_MODULES;
	if (wan_test_bit(x, &fe->rm_param.module_map)) {
#if defined(REG_SHADOW)
		if (fe->rm_param.mod[x].type == MOD_TYPE_FXS) {
			/* Read first shadow reg */
			fe->rm_param.reg0shadow[x] = READ_RM_REG(x, 68);
			/* Read second shadow reg */
			fe->rm_param.reg1shadow[x] = READ_RM_REG(x, 64);
			/* Read third shadow reg */
			fe->rm_param.reg2shadow[x] = READ_RM_REG(x, 8);
			if (!card->u.aft.tdmv_zaptel_cfg) {
				/* Read fourth shadow reg */
				fe->rm_param.reg3shadow[x] = READ_RM_REG(x, 19);
				/* Read fifth shadow reg */
				fe->rm_param.reg4shadow[x] = READ_RM_REG(x, 20);
			}

		
		}else if (fe->rm_param.mod[x].type == MOD_TYPE_FXO) {
			/* Read first shadow reg */
			fe->rm_param.reg0shadow[x] = READ_RM_REG(x, 5);
			/* Read second shadow reg */
			fe->rm_param.reg1shadow[x] = READ_RM_REG(x, 29);
			/* Read third shadow reg */
			fe->rm_param.reg2shadow[x] = READ_RM_REG(x, 34);
			/* Read fourth shadow reg */
			fe->rm_param.reg3shadow[x] = READ_RM_REG(x, 28);
		}
#endif

#if defined(NEW_PULSE_DIALING)

		if (fe->fe_cfg.cfg.remora.fxs_pulsedialing != WANOPT_YES){
			wp_tdmv_remora_check_hook(fe, x);
		}
#else
#ifndef PULSE_DIALING

		wp_tdmv_remora_check_hook(fe, x);
#endif
#endif
		if (!(fe->rm_param.intcount & 0xf0)){
			if (fe->rm_param.mod[x].type == MOD_TYPE_FXS) {
				wp_tdmv_remora_proslic_recheck_sanity(fe, x);
			}else if (fe->rm_param.mod[x].type == MOD_TYPE_FXO) {
				wp_tdmv_remora_voicedaa_recheck_sanity(fe, x);
			}
		}
	}
	
	if (!(fe->rm_param.intcount % 10000)) {
		/* Accept an alarm once per 10 seconds */
	
		for (x = 0; x < fe->rm_param.max_fe_channels; x++) {

			if (!wan_test_bit(x, &fe->rm_param.module_map)){ /* should be ok confirm with Nenad*/
                                       continue;
             }

                       if (card->u.aft.tdmv_zaptel_cfg) {
#if defined(CONFIG_PRODUCT_WANPIPE_TDM_VOICE)          
                               fxs = &wr->mod[x].fxs;
#endif
                       } else {
                               fxs = &fe->rm_param.mod[x].u.fxs;
                       }
	                	
					  	if (fe->rm_param.mod[x].type == MOD_TYPE_FXS) {
							if (fxs->palarms){
								fxs->palarms--;
							}
						}
		}
	}
	

#endif
	return 0;
}
