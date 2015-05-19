/******************************************************************************
 * sdla_gsm_inline.h	
 *
 * Author: 	Moises Silva <moy@sangoma.com>
 *
 * Copyright:	(c) 2011 Sangoma Technologies Inc.
 *
 *		This program is free software; you can redistribute it and/or
 *		modify it under the terms of the GNU General Public License
 *		as published by the Free Software Foundation; either version
 *		2 of the License, or (at your option) any later version.
 * ============================================================================
 * Dec 09, 2011 Moises Silva   Initial Version
 ******************************************************************************
 */

#ifndef __SDLA_GSM_INLINE_H
#define __SDLA_GSM_INLINE_H

/* XXX This mess was brought to you by <moy@sangoma.com>  ... XXX
 * I could not find a way to abstract the module power on/off code in a way could be used
 * by sdladrv.c for hardware probe, aft_gsm.c for port start/stop, and sdla_gsm.c
 * for wanpipemon handling of fixed start/stop of the modules without messing with the build
 * in such a painful way that I'd rather die first, so I added this function here,
 * in a way can be included by each file that uses it ... */
static __inline int wp_gsm_toggle_power(void *phw, int mod_map, int turn_on)
{
	u32 reg[MAX_GSM_MODULES+1]; /* mod_no is not zero-based */
	int timeout_loops = 0;
	int mod_no = 1;
	sdlahw_t *hw = phw;
	const char *devname = hw->devname;

	/* 
	 * Power toggle sequence as described by the Telit documentation
	 * We have a power monitor pin that tells us whether the module is on/off
	 * We have a power pin that is equivalent to the power on/off button on your cell phone
	 * In order to turn on/off we hold high the power pin for at least one second and then
	 * set it low. Then we monitor the power monitor pin until goes high/low depending on
	 * whether we're turning on or off the module
	 * 
	 * Note that in the Telit documentation you will see the high/low order inversed, there
	 * is an inversor in our hardware doing that, ask our hw engineers why? :-)
	 */
	for (mod_no = 1; mod_no <= MAX_GSM_MODULES; mod_no++) {
		if (!wan_test_bit(AFT_GSM_MOD_BIT(mod_no), &mod_map)) {
			continue;
		}

		reg[mod_no] = 0;
		__sdla_bus_read_4(hw, AFT_GSM_MOD_REG(mod_no, AFT_GSM_CONFIG_REG), &reg[mod_no]);
		/* if we were asked to turn the module on and is on, we're done */
		if (turn_on && wan_test_bit(AFT_GSM_MOD_POWER_MONITOR_BIT, &reg[mod_no])) {
			DEBUG_EVENT("%s: GSM module %d is already on, skipping power toggle ...\n", devname, mod_no);
			wan_clear_bit(AFT_GSM_MOD_BIT(mod_no), &mod_map);
			continue;
		}
		/* if we were asked to turn the module off and is off, we're done */
		if (!turn_on && !wan_test_bit(AFT_GSM_MOD_POWER_MONITOR_BIT, &reg[mod_no])) {
			DEBUG_EVENT("%s: GSM module %d is already off, skipping power toggle ...\n", devname, mod_no);
			wan_clear_bit(AFT_GSM_MOD_BIT(mod_no), &mod_map);
			continue;
		}
		DEBUG_EVENT("%s: Turning GSM module %d %s ...\n", devname, mod_no, turn_on ? "on" : "off");
		wan_set_bit(AFT_GSM_MOD_POWER_BIT, &reg[mod_no]);
		__sdla_bus_write_4(hw, AFT_GSM_MOD_REG(mod_no, AFT_GSM_CONFIG_REG), reg[mod_no]);
	}

	/* no modules to toggle */
	if (!mod_map) {
		return 0;
	}

	WP_DELAY(AFT_GSM_MODULE_POWER_TOGGLE_DELAY_MS * 1000);

	/* restore the power bit in all modules */
	for (mod_no = 1; mod_no <= MAX_GSM_MODULES; mod_no++) {
		if (!wan_test_bit(AFT_GSM_MOD_BIT(mod_no), &mod_map)) {
			continue;
		}
		wan_clear_bit(AFT_GSM_MOD_POWER_BIT, &reg[mod_no]);
		__sdla_bus_write_4(hw, AFT_GSM_MOD_REG(mod_no, AFT_GSM_CONFIG_REG), reg[mod_no]);
	}

	/* monitor the modules to see if they power on/off */
	for (timeout_loops = (AFT_GSM_MODULE_POWER_TOGGLE_TIMEOUT_MS / AFT_GSM_MODULE_POWER_TOGGLE_CHECK_INTERVAL_MS); 
	     (timeout_loops && mod_map); 
	     timeout_loops--) {
		WP_DELAY(AFT_GSM_MODULE_POWER_TOGGLE_CHECK_INTERVAL_MS * 1000);
		for (mod_no = 1; mod_no <= MAX_GSM_MODULES; mod_no++) {
			if (!wan_test_bit(AFT_GSM_MOD_BIT(mod_no), &mod_map)) {
				continue;
			}
			reg[mod_no] = 0;
			__sdla_bus_read_4(hw, AFT_GSM_MOD_REG(mod_no, AFT_GSM_CONFIG_REG), &reg[mod_no]);
			/* if we were asked to turn the module on and is on, we're done */
			if (turn_on && wan_test_bit(AFT_GSM_MOD_POWER_MONITOR_BIT, &reg[mod_no])) {
				DEBUG_EVENT("%s: GSM module %d is now on ...\n", devname, mod_no);
				wan_clear_bit(AFT_GSM_MOD_BIT(mod_no), &mod_map);
			}
			/* if we were asked to turn the module off and is off, we're done */
			if (!turn_on && !wan_test_bit(AFT_GSM_MOD_POWER_MONITOR_BIT, &reg[mod_no])) {
				DEBUG_EVENT("%s: GSM module %d is now off ...\n", devname, mod_no);
				wan_clear_bit(AFT_GSM_MOD_BIT(mod_no), &mod_map);
			}
		}
	}
	/* return a map of the modules that failed to turn on/off (if any) */
	return mod_map;
}

#endif	/* __SDLA_GSM_INLINE_H */

