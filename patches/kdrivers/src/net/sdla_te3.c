/******************************************************************************
 * 
 * sdla_te3.c	Sangoma T3/E3 front end.
 *
 * 		Alex Feldman <al.feldman@sangoma.com>
 * 		
 * 	Copyright Sangoma Technologies Inc. 1999, 2000,2001, 2002, 2003, 2004
 *
 * 	This program is provided subject to the Software License included in 
 * 	this package in the file license.txt. By using this program you agree
 * 	to be bound bythe terms of this license. 
 *
 * 	Should you not have a copy of the file license.txt, or wish to obtain
 * 	a hard copy of the Software License, please contact Sangoma
 * 	technologies Corporation.
 *
 * 	Contact: Sangoma Technologies Inc. 905-474-1990, info@sangoma.com
 *
 *****************************************************************************/


/******************************************************************************
**			   INCLUDE FILES
******************************************************************************/

# include "wanpipe_includes.h"
# include "wanpipe_defines.h"
# include "wanpipe_debug.h"
# include "wanproc.h"
# include "sdla_te3_reg.h"
# include "wanpipe.h"

/******************************************************************************
**			  DEFINES AND MACROS
******************************************************************************/

/******************************************************************************
**			  DEFINES AND MACROS
******************************************************************************/

#define WRITE_CPLD(reg,val)						\
		fe->write_cpld(fe->card, (reg), (val))

#define WRITE_EXAR_CPLD(reg,val)					\
		fe->write_fe_cpld(fe->card, (reg), (val))

#define WRITE_REG(reg,val)						\
	fe->write_fe_reg(						\
		((sdla_t*)fe->card)->hw,(int)(reg),(int)(val));
#define READ_REG(reg)							\
	fe->read_fe_reg(						\
		((sdla_t*)fe->card)->hw,(int)(reg));

#define WAN_FE_SWAP_BIT(value, mask)					\
	if ((value) & mask){						\
		(value) &= ~mask;					\
	}else{								\
		(value) |= mask;					\
	}

/* DS3: Define DS3 alarm states: LOS OOF YEL */
#define IS_DS3_ALARM(alarm)	((alarm) &				\
					(WAN_TE3_BIT_LOS_ALARM |	\
					 WAN_TE3_BIT_YEL_ALARM |	\
					 WAN_TE3_BIT_OOF_ALARM))

/* DS3: Define E3 alarm states: LOS OOF YEL */
#if 0
#define IS_E3_ALARM(alarm)	((alarm) &				\
					(WAN_TE3_BIT_LOS_ALARM |	\
					 WAN_TE3_BIT_YEL_ALARM |	\
					 WAN_TE3_BIT_OOF_ALARM))
#endif

#define IS_E3_ALARM(alarm)	((alarm) &				\
					(WAN_TE3_BIT_YEL_ALARM |	\
					 WAN_TE3_BIT_OOF_ALARM))

/******************************************************************************
**			  FUNCTION DEFINITIONS
******************************************************************************/

static int sdla_te3_config(void *p_fe);
static int sdla_te3_unconfig(void *p_fe);
static int sdla_te3_get_fe_status(sdla_fe_t *fe, unsigned char *status, int notused);
static int sdla_te3_polling(sdla_fe_t *fe);
static int sdla_ds3_isr(sdla_fe_t *fe);
static int sdla_e3_isr(sdla_fe_t *fe);
static int sdla_te3_isr(sdla_fe_t *fe);
static int sdla_te3_udp(sdla_fe_t *fe, void*, unsigned char*);
static unsigned int sdla_te3_read_alarms(sdla_fe_t *fe, int);
static int sdla_te3_read_pmon(sdla_fe_t *fe, int);
static int sdla_te3_flush_pmon(sdla_fe_t *fe);
static int sdla_te3_post_init(void* pfe);
static int sdla_te3_post_unconfig(void* pfe);

static int sdla_te3_update_alarm_info(sdla_fe_t* fe, struct seq_file* m, int* stop_cnt);
static int sdla_te3_update_pmon_info(sdla_fe_t* fe, struct seq_file* m, int* stop_cnt);

static int sdla_te3_old_set_lb_modes(sdla_fe_t *fe, unsigned char type, unsigned char mode);
static int sdla_te3_set_lb_modes(sdla_fe_t *fe, unsigned char type, unsigned char mode);

static int sdla_te3_set_lb_modes_all(sdla_fe_t *fe, unsigned char type, unsigned char mode)
{
	int err;
	sdla_t *card=fe->card;

	DEBUG_TEST("%s: NENAD %s LOOP\n",fe->name,
			mode==WAN_TE3_LB_DISABLE?"Disable":"Enable");	

	if (card->adptr_subtype == AFT_SUBTYPE_NORMAL){
	       	err = sdla_te3_old_set_lb_modes(fe, type,mode); 
	}else if (card->adptr_subtype == AFT_SUBTYPE_SHARK){
	      	err = sdla_te3_set_lb_modes(fe, type,mode); 
	}

	return 0;
}


/******************************************************************************
 *				sdla_te3_get_fe_status()	
 *
 * Description:
 * Arguments:	
 * Returns:
 ******************************************************************************
 */
static char* sdla_te3_get_fe_media_string(void)
{
	return ("AFT T3/E3");
}

/******************************************************************************
 *				sdla_te3_get_fe_status()	
 *
 * Description:
 * Arguments:	
 * Returns:
 ******************************************************************************
 */
static unsigned char sdla_te3_get_fe_media(sdla_fe_t *fe)
{
	return fe->fe_cfg.media;
}

/******************************************************************************
* sdla_te3_get_fe_status()	
*
* Description	: Get current FE line state - is it Connected or Disconnected
*
* Arguments	: fe - pointer to Front End structure.	
*		  status - pointer to location where the FE line state will
*			be stored. 
*		  notused - ignored 
*
* Returns	: always zero.
*******************************************************************************/
static int sdla_te3_get_fe_status(sdla_fe_t *fe, unsigned char *status, int notused)
{
	*status = fe->fe_status;
	return 0;
}

/******************************************************************************
 *				sdla_te3_polling()	
 *
 * Description:
 * Arguments:	
 * Returns:
 ******************************************************************************
 */
static int sdla_te3_polling(sdla_fe_t *fe)
{
       	int err;
	err=sdla_te3_read_alarms(fe, WAN_FE_ALARM_READ|WAN_FE_ALARM_UPDATE);
	return err;
}

/******************************************************************************
 *				sdla_te3_set_status()	
 *
 * Description:
 * Arguments:	
 * Returns:
 ******************************************************************************
 */
static int sdla_te3_set_status(sdla_fe_t *fe)
{
	if (IS_DS3(&fe->fe_cfg)){
		if (IS_DS3_ALARM(fe->fe_alarm)){
			if (fe->fe_status != FE_DISCONNECTED){
				DEBUG_EVENT("%s: DS3 disconnected!\n",
						fe->name);
				fe->fe_status = FE_DISCONNECTED;
			}
		}else{
			if (fe->fe_status != FE_CONNECTED){
				DEBUG_EVENT("%s: DS3 connected!\n",
						fe->name);
				fe->fe_status = FE_CONNECTED;
			}
		}
	}else if (IS_E3(&fe->fe_cfg)){
		if (IS_E3_ALARM(fe->fe_alarm)){

			if (fe->fe_status != FE_DISCONNECTED){
				fe->fe_param.te3.e3_connect_delay=0;

				if (fe->fe_param.te3.e3_lb_ctrl == 1) {
					fe->fe_param.te3.e3_lb_ctrl=2;
				} else if (fe->fe_param.te3.e3_lb_ctrl == 3) {
					fe->fe_param.te3.e3_lb_ctrl=4;
				}

				DEBUG_EVENT("%s: E3 disconnected! State=%i\n",
						fe->name,fe->fe_param.te3.e3_lb_ctrl);
				fe->fe_status = FE_DISCONNECTED;
			} else {

				if (fe->fe_param.te3.e3_lb_ctrl==2) {
					fe->fe_param.te3.e3_connect_delay++;
					DEBUG_EVENT("%s: Waiting for link up %i/8...!\n", 
								fe->name,fe->fe_param.te3.e3_connect_delay);
					if (fe->fe_param.te3.e3_connect_delay >= 8) {
						DEBUG_EVENT("%s: Link Up Timeout - Link in loopback!\n", fe->name);
						sdla_te3_set_lb_modes_all(fe,WAN_TE3_LIU_LB_REMOTE,WAN_TE3_LB_DISABLE);
						fe->fe_param.te3.e3_lb_ctrl=4;
						fe->fe_param.te3.e3_connect_delay=0;
					}
				}
			}

		}else{

			if (fe->fe_status != FE_CONNECTED){
				fe->fe_param.te3.e3_connect_delay++;
			  	if (fe->fe_param.te3.e3_connect_delay >= 5) {
					fe->fe_param.te3.e3_connect_delay=0;

					if (fe->fe_param.te3.e3_lb_ctrl == 0) {
						fe->fe_param.te3.e3_lb_ctrl=1;
						sdla_te3_set_lb_modes_all(fe,WAN_TE3_LIU_LB_REMOTE,WAN_TE3_LB_ENABLE);
					} else if (fe->fe_param.te3.e3_lb_ctrl == 2) {
						fe->fe_param.te3.e3_lb_ctrl = 3;
						sdla_te3_set_lb_modes_all(fe,WAN_TE3_LIU_LB_REMOTE,WAN_TE3_LB_DISABLE);
					} else {
						fe->fe_param.te3.e3_lb_ctrl = 0;
					}

					DEBUG_EVENT("%s: E3 connected! State=%i\n",
							fe->name,fe->fe_param.te3.e3_lb_ctrl);
					fe->fe_status = FE_CONNECTED;
			   	} 
			} else { 
				if (fe->fe_param.te3.e3_lb_ctrl==1) {
					fe->fe_param.te3.e3_connect_delay++;
					DEBUG_EVENT("%s: Waiting for link down %i/10...!\n", 
						fe->name,fe->fe_param.te3.e3_connect_delay);
					if (fe->fe_param.te3.e3_connect_delay > 10) {
						sdla_te3_set_lb_modes_all(fe,WAN_TE3_LIU_LB_REMOTE,WAN_TE3_LB_DISABLE);
						fe->fe_param.te3.e3_lb_ctrl = 3;
						fe->fe_param.te3.e3_connect_delay=0;
					}
				}
		  	}
			
		}
	}else{
		return -EINVAL;
	}
	return 0;
}

/******************************************************************************
**				sdla_ds3_tx_isr()
**
** Description:
** Arguments:	
** Returns:
******************************************************************************/
static int sdla_ds3_tx_isr(sdla_fe_t *fe)
{
	unsigned char	value;
	
	WAN_ASSERT(fe->write_fe_reg == NULL);
	WAN_ASSERT(fe->read_fe_reg == NULL);
	value = READ_REG(REG_TxDS3_LAPD_STATUS);
	if (value & BIT_TxDS3_LAPD_STATUS_INT){
		DEBUG_EVENT("%s: LAPD Interrupt!\n",
				fe->name);
	}
	return 0;
}

/******************************************************************************
**				sdla_ds3_rx_isr()	
**
** Description:
** Arguments:	
** Returns:
******************************************************************************/
static int sdla_ds3_rx_isr(sdla_fe_t *fe)
{
	unsigned char	value, status;

	WAN_ASSERT(fe->write_fe_reg == NULL);
	WAN_ASSERT(fe->read_fe_reg == NULL);
	/* RxDS3 Interrupt status register (0x13) */
	value = READ_REG(REG_RxDS3_INT_STATUS);
	status = READ_REG(REG_RxDS3_CFG_STATUS);
	if (WAN_FE_FRAME(fe) == WAN_FR_DS3_Cbit && value & BIT_RxDS3_INT_STATUS_CPBIT_ERR){
		DEBUG_TE3("%s: CP Bit Error interrupt detected!\n",
						fe->name);
	}
	if (value & BIT_RxDS3_INT_STATUS_LOS){
		if (status & BIT_RxDS3_CFG_STATUS_RxLOS){
			DEBUG_EVENT("%s: LOS Status ON!\n",
						fe->name);
			fe->fe_alarm |= WAN_TE3_BIT_LOS_ALARM;
		}else{
			DEBUG_EVENT("%s: LOS Status OFF!\n",
						fe->name);
			fe->fe_alarm &= ~WAN_TE3_BIT_LOS_ALARM;
		}
	}
	if (value & BIT_RxDS3_INT_STATUS_AIS){
		DEBUG_EVENT("%s: AIS status %s!\n",
				fe->name,
				(status & BIT_RxDS3_CFG_STATUS_RxAIS) ? "ON" : "OFF");
	}
	if (value & BIT_RxDS3_INT_STATUS_IDLE){
		DEBUG_EVENT("%s: IDLE condition status %s!\n",
				fe->name,
				(status & BIT_RxDS3_CFG_STATUS_RxIDLE) ? "ON" : "OFF");
	}
	if (value & BIT_RxDS3_INT_STATUS_OOF){
		if (status & BIT_RxDS3_CFG_STATUS_RxLOS){
			DEBUG_EVENT("%s: OOF Alarm ON!\n",
						fe->name);
			fe->fe_alarm |= WAN_TE3_BIT_OOF_ALARM;
		}else{
			DEBUG_EVENT("%s: OOF Alarm OFF!\n",
						fe->name);
			fe->fe_alarm &= ~WAN_TE3_BIT_OOF_ALARM;
		}
	}
	status = READ_REG(REG_RxDS3_STATUS);
	if (value & BIT_RxDS3_INT_STATUS_FERF){
		if (status & BIT_RxDS3_STATUS_RxFERF){
			DEBUG_EVENT("%s: Rx FERF status is ON (YELLOW)!\n",
					fe->name);
			fe->fe_alarm |= WAN_TE3_BIT_YEL_ALARM;
		}else{
			DEBUG_EVENT("%s: Rx FERF status is OFF!\n",
					fe->name);
			fe->fe_alarm &= ~WAN_TE3_BIT_YEL_ALARM;
		}
	}
	if (WAN_FE_FRAME(fe) == WAN_FR_DS3_Cbit && value & BIT_RxDS3_INT_STATUS_AIC){
		DEBUG_TE3("%s: AIC bit-field status %s!\n",
				fe->name,
				(status & BIT_RxDS3_STATUS_RxAIC) ? "ON" : "OFF");
	}
	if (value & BIT_RxDS3_INT_STATUS_PBIT_ERR){
		DEBUG_TE3("%s: P-Bit error interrupt!\n",
					fe->name);
	}

	/* RxDS3 FEAC Interrupt (0x17) */
	value = READ_REG(REG_RxDS3_FEAC_INT);
	if (value & BIT_RxDS3_FEAC_REMOVE_INT_STATUS){
		DEBUG_EVENT("%s: RxFEAC Remove Interrupt!\n",
				fe->name);
	}
	if (value & BIT_RxDS3_FEAC_VALID_INT_STATUS){
		DEBUG_EVENT("%s: RxFEAC Valid Interrupt!\n",
				fe->name);
	}

	return 0;
}

/******************************************************************************
**				sdla_ds3_isr()	
**
** Description:
** Arguments:	
** Returns:
******************************************************************************/
static int sdla_ds3_isr(sdla_fe_t *fe)
{
	unsigned char	value;
	
	WAN_ASSERT(fe->write_fe_reg == NULL);
	WAN_ASSERT(fe->read_fe_reg == NULL);
	value = READ_REG(REG_BLOCK_INT_STATUS);
	if (value & BIT_BLOCK_INT_STATUS_RxDS3_E3){
		sdla_ds3_rx_isr(fe);
	}
	if (value & BIT_BLOCK_INT_STATUS_TxDS3_E3){
		sdla_ds3_tx_isr(fe);
	}
		
	return 0;
}

/******************************************************************************
**				sdla_e3_tx_isr()
**
** Description:
** Arguments:	
** Returns:
******************************************************************************/
static int sdla_e3_tx_isr(sdla_fe_t *fe)
{
	DEBUG_EVENT("%s: %s: This function is still not supported!\n",
			fe->name, __FUNCTION__);

	return 0;
}


/******************************************************************************
**				sdla_e3_rx_isr()
**
** Description:
** Arguments:	
** Returns:
******************************************************************************/
static int sdla_e3_rx_isr(sdla_fe_t *fe)
{
	unsigned char	int_status1, int_status2;
	unsigned char	status;

	WAN_ASSERT(fe->write_fe_reg == NULL);
	WAN_ASSERT(fe->read_fe_reg == NULL);

	int_status1 = READ_REG(REG_RxE3_INT_STATUS_1);
	int_status2 = READ_REG(REG_RxE3_INT_STATUS_2);
	status = READ_REG(REG_RxE3_CFG_STATUS_2);
	if (int_status1 & BIT_RxE3_INT_STATUS_OOF){
		if (status & BIT_RxE3_CFG_STATUS_RxOOF){
			DEBUG_EVENT("%s: OOF Alarm ON!\n",
						fe->name);
			fe->fe_alarm |= WAN_TE3_BIT_OOF_ALARM;
		}else{
			DEBUG_EVENT("%s: OOF Alarm OFF!\n",
						fe->name);
			fe->fe_alarm &= ~WAN_TE3_BIT_OOF_ALARM;
		}
	}

	if (int_status1 & BIT_RxE3_INT_STATUS_LOF){
		if (status & BIT_RxE3_CFG_STATUS_RxLOF){
			DEBUG_EVENT("%s: LOF Alarm ON!\n",
						fe->name);
			fe->fe_alarm |= WAN_TE3_BIT_LOF_ALARM;
		}else{
			DEBUG_EVENT("%s: LOF Alarm OFF!\n",
						fe->name);
			fe->fe_alarm &= ~WAN_TE3_BIT_LOF_ALARM;
		}
	}

	if (int_status1 & BIT_RxE3_INT_STATUS_LOS){
		if (status & BIT_RxE3_CFG_STATUS_RxLOS){
			DEBUG_EVENT("%s: LOS Alarm ON!\n",
						fe->name);
			fe->fe_alarm |= WAN_TE3_BIT_LOS_ALARM;
		}else{
			DEBUG_EVENT("%s: LOS Alarm OFF!\n",
						fe->name);
			fe->fe_alarm &= ~WAN_TE3_BIT_LOS_ALARM;
		}
	}
	
	if (int_status1 & BIT_RxE3_INT_STATUS_AIS){
		if (status & BIT_RxE3_CFG_STATUS_RxAIS){
			DEBUG_EVENT("%s: AIS Alarm ON!\n",
						fe->name);
			fe->fe_alarm |= WAN_TE3_BIT_AIS_ALARM;
		}else{
			DEBUG_EVENT("%s: AIS Alarm OFF!\n",
						fe->name);
			fe->fe_alarm &= ~WAN_TE3_BIT_AIS_ALARM;
		}
	}
	
	if (int_status2 & BIT_RxE3_INT_STATUS_FERF){
		if (status & BIT_RxE3_CFG_STATUS_RxFERF){
			DEBUG_EVENT("%s: Rx FERF status is ON (YELLOW)!\n",
					fe->name);
			fe->fe_alarm |= WAN_TE3_BIT_YEL_ALARM;
		}else{
			DEBUG_EVENT("%s: Rx FERF status is OFF!\n",
					fe->name);
			fe->fe_alarm &= ~WAN_TE3_BIT_YEL_ALARM;
		}
	}

	return 0;
}

/******************************************************************************
 *				sdla_e3_isr()	
 *
 * Description:
 * Arguments:	
 * Returns:
 ******************************************************************************
 */
static int sdla_e3_isr(sdla_fe_t *fe)
{
	unsigned char	value;
	
	WAN_ASSERT(fe->write_fe_reg == NULL);
	WAN_ASSERT(fe->read_fe_reg == NULL);

	value = READ_REG(REG_BLOCK_INT_STATUS);
	if (value & BIT_BLOCK_INT_STATUS_RxDS3_E3){
		sdla_e3_rx_isr(fe);
	}
	if (value & BIT_BLOCK_INT_STATUS_TxDS3_E3){
		sdla_e3_tx_isr(fe);
	}
	return -EINVAL;
}
 
/******************************************************************************
 *				sdla_te3_isr()	
 *
 * Description:
 * Arguments:	
 * Returns:
 ******************************************************************************
 */
static int sdla_te3_isr(sdla_fe_t *fe)
{
	sdla_fe_cfg_t	*fe_cfg = &fe->fe_cfg;
	int		err = 0;
	
	switch(fe_cfg->media){
	case WAN_MEDIA_DS3:
		err = sdla_ds3_isr(fe);
		break;
	case WAN_MEDIA_E3:
		err = sdla_e3_isr(fe);
		break;
	}
	sdla_te3_set_status(fe);
	return err;
}

/******************************************************************************
 *				sdla_te3_read_alarms()	
 *
 * Description:
 * Arguments:	
 * Returns:
 ******************************************************************************
 */
static unsigned int sdla_te3_read_framer_alarms(sdla_fe_t *fe)
{
	sdla_fe_cfg_t	*fe_cfg = &fe->fe_cfg;
	unsigned int	alarms = 0;
	unsigned char	value;

	WAN_ASSERT(fe->write_fe_reg == NULL);
	WAN_ASSERT(fe->read_fe_reg == NULL);

	if (fe_cfg->media == WAN_MEDIA_DS3){
		value = READ_REG(REG_RxDS3_CFG_STATUS);
		if (value & BIT_RxDS3_CFG_STATUS_RxAIS){
			alarms |= WAN_TE3_BIT_AIS_ALARM;
		}else{
			alarms &= ~WAN_TE3_BIT_AIS_ALARM;
		}	
		if (value & BIT_RxDS3_CFG_STATUS_RxLOS){
			alarms |= WAN_TE3_BIT_LOS_ALARM;
		}else{
			alarms &= ~WAN_TE3_BIT_LOS_ALARM;
		}
		if (value & BIT_RxDS3_CFG_STATUS_RxOOF){
			alarms |= WAN_TE3_BIT_OOF_ALARM;
		}else{
			alarms &= ~WAN_TE3_BIT_OOF_ALARM;
		}
		value = READ_REG(REG_RxDS3_STATUS);
		if (value & BIT_RxDS3_STATUS_RxFERF){
			alarms |= WAN_TE3_BIT_YEL_ALARM;
		}else{
			alarms &= ~WAN_TE3_BIT_YEL_ALARM;
		}
	}else{
		value = READ_REG(REG_RxE3_CFG_STATUS_2);
		if (value & BIT_RxE3_CFG_STATUS_RxOOF){
			alarms |= WAN_TE3_BIT_OOF_ALARM;
		}else{
			alarms &= ~WAN_TE3_BIT_OOF_ALARM;
		}

		if (value & BIT_RxE3_CFG_STATUS_RxLOF){
			alarms |= WAN_TE3_BIT_LOF_ALARM;
		}else{
			alarms &= ~WAN_TE3_BIT_LOF_ALARM;
		}

		if (value & BIT_RxE3_CFG_STATUS_RxLOS){
			alarms |= WAN_TE3_BIT_LOS_ALARM;
		}else{
			alarms &= ~WAN_TE3_BIT_LOS_ALARM;
		}
	
		if (value & BIT_RxE3_CFG_STATUS_RxAIS){
			alarms |= WAN_TE3_BIT_AIS_ALARM;
		}else{
			alarms &= ~WAN_TE3_BIT_AIS_ALARM;
		}
	
		if (value & BIT_RxE3_CFG_STATUS_RxFERF){
			alarms |= WAN_TE3_BIT_YEL_ALARM;
		}else{
			alarms &= ~WAN_TE3_BIT_YEL_ALARM;
		}
	}
	return alarms;
}

static int sdla_te3_print_alarms(sdla_fe_t *fe, unsigned int alarms)
{
	
	DEBUG_EVENT("%s: %s Framer Alarms status (%X):\n",
			fe->name,
			FE_MEDIA_DECODE(fe),
			alarms);

	if (!alarms){
		DEBUG_EVENT("%s: %s Alarms status: No alarms detected!\n",
				fe->name,
				FE_MEDIA_DECODE(fe));
		return 0;
	}
	if (alarms & WAN_TE3_BIT_AIS_ALARM){
		DEBUG_EVENT("%s:    AIS Alarm is ON\n", fe->name);
	}
	if (alarms & WAN_TE3_BIT_LOS_ALARM){
		DEBUG_EVENT("%s:    LOS Alarm is ON\n", fe->name);
	}
	if (alarms & WAN_TE3_BIT_OOF_ALARM){
		DEBUG_EVENT("%s:    OOF Alarm is ON\n", fe->name);
	}
	if (alarms & WAN_TE3_BIT_YEL_ALARM){
		DEBUG_EVENT("%s:    YEL Alarm is ON\n", fe->name);
	}
	if (alarms & WAN_TE3_BIT_LOF_ALARM){
		DEBUG_EVENT("%s:    LOF Alarm OFF!\n", fe->name);
	}
	
	return 0;
}

static unsigned int sdla_te3_read_alarms(sdla_fe_t *fe, int action)
{
	unsigned int	alarms = 0;
	
	if (IS_FE_ALARM_READ(action)){
		alarms = sdla_te3_read_framer_alarms(fe);
	}

	if (IS_FE_ALARM_PRINT(action)){
		sdla_te3_print_alarms(fe, alarms);
	}

	if (IS_FE_ALARM_UPDATE(action)){
		fe->fe_alarm = alarms;
		sdla_te3_set_status(fe);
	} 
	return alarms;
}


/******************************************************************************
 *				sdla_te3_set_alarm()	
 *
 * Description:
 * Arguments:	
 * Returns:
 ******************************************************************************
 */
static int sdla_te3_set_alarm(sdla_fe_t *fe, unsigned int alarm)
{
	DEBUG_EVENT("%s: %s: This function is still not supported!\n",
			fe->name, __FUNCTION__);
	return -EINVAL;
}

/******************************************************************************
*				sdla_te3_read_pmon()	
*
* Description:
* Arguments:	
* Returns:
******************************************************************************/
static int sdla_te3_read_pmon(sdla_fe_t *fe, int action)
{
	sdla_te3_pmon_t	*pmon = (sdla_te3_pmon_t*)&fe->fe_stats.u.te3_pmon;
	unsigned char value_msb, value_lsb;

	WAN_ASSERT(fe->write_fe_reg == NULL);
	WAN_ASSERT(fe->read_fe_reg == NULL);

	value_msb = READ_REG(REG_PMON_LCV_MSB);
	value_lsb = READ_REG(REG_PMON_LCV_LSB);
	pmon->pmon_lcv += ((value_msb << 8) | value_lsb);

	value_msb = READ_REG(REG_PMON_FRAMING_ERR_CNT_MSB);
	value_lsb = READ_REG(REG_PMON_FRAMING_ERR_CNT_LSB);
	pmon->pmon_framing += ((value_msb << 8) | value_lsb); 

	value_msb = READ_REG(REG_PMON_PARITY_ERR_CNT_MSB);
	value_lsb = READ_REG(REG_PMON_PARITY_ERR_CNT_LSB);
	pmon->pmon_parity += ((value_msb << 8) | value_lsb); 

	value_msb = READ_REG(REG_PMON_FEBE_EVENT_CNT_MSB);
	value_lsb = READ_REG(REG_PMON_FEBE_EVENT_CNT_LSB);
	pmon->pmon_febe += ((value_msb << 8) | value_lsb); 

	value_msb = READ_REG(REG_PMON_CPBIT_ERROR_CNT_MSB);
	value_lsb = READ_REG(REG_PMON_CPBIT_ERROR_CNT_LSB);
	pmon->pmon_cpbit += ((value_msb << 8) | value_lsb); 
	return 0;
}

/******************************************************************************
*				sdla_te3_flush_pmon()	
*
* Description:
* Arguments:
* Returns:
******************************************************************************/
static int sdla_te3_flush_pmon(sdla_fe_t *fe)
{
	sdla_te3_pmon_t	*pmon = (sdla_te3_pmon_t*)&fe->fe_stats.u.te3_pmon;

	pmon->pmon_lcv		= 0;
	pmon->pmon_framing	= 0; 
	pmon->pmon_parity	= 0; 
	pmon->pmon_febe		= 0; 
	pmon->pmon_cpbit  	= 0; 
	return 0;
}

/******************************************************************************
*				sdla_te3_old_set_lb_modes()	
*
* Description:
* Arguments:
* Returns:
******************************************************************************/
static int 
sdla_te3_old_set_lb_modes(sdla_fe_t *fe, unsigned char type, unsigned char mode)
{

	WAN_ASSERT(fe->write_cpld == NULL);
	DEBUG_EVENT("%s: %s %s mode...\n",
			fe->name,
			WAN_TE3_LB_ACTION_DECODE(mode),
			WAN_TE3_LB_TYPE_DECODE(type));

	if (mode == WAN_TE3_LB_DISABLE){
		fe->te3_param.cpld_status &= ~BIT_CPLD_STATUS_LLB;
		fe->te3_param.cpld_status &= ~BIT_CPLD_STATUS_RLB;
	}else{
		switch(type){
		case WAN_TE3_LIU_LB_ANALOG:
			fe->te3_param.cpld_status |= BIT_CPLD_STATUS_LLB;
			fe->te3_param.cpld_status &= ~BIT_CPLD_STATUS_RLB;
			break;
		case WAN_TE3_LIU_LB_REMOTE:
			fe->te3_param.cpld_status &= ~BIT_CPLD_STATUS_LLB;
			fe->te3_param.cpld_status |= BIT_CPLD_STATUS_RLB;
			break;
		case WAN_TE3_LIU_LB_DIGITAL:
			fe->te3_param.cpld_status |= BIT_CPLD_STATUS_LLB;
			fe->te3_param.cpld_status |= BIT_CPLD_STATUS_RLB;
			break;
		default :
			DEBUG_EVENT("%s: (T3/E3) Unknown loopback mode!\n",
					fe->name);
			break;
		}		
	}
	/* Write value to CPLD Status/Control register */
	WRITE_CPLD(REG_CPLD_STATUS, fe->te3_param.cpld_status);
	return 0;
}
 
static int 
sdla_te3_set_lb_modes(sdla_fe_t *fe, unsigned char type, unsigned char mode) 
{
	unsigned char	data = 0x00;

	WAN_ASSERT(fe->write_fe_reg == NULL);
	WAN_ASSERT(fe->read_fe_reg == NULL);
	DEBUG_EVENT("%s: %s %s mode...\n",
			fe->name,
			WAN_TE3_LB_ACTION_DECODE(mode),
			WAN_TE3_LB_TYPE_DECODE(type));

	data = READ_REG(REG_LINE_INTERFACE_DRIVE);
	if (mode == WAN_TE3_LB_DISABLE){
		data &= ~BIT_LINE_INTERFACE_DRIVE_LLOOP;
		data &= ~BIT_LINE_INTERFACE_DRIVE_RLOOP;
	}else{
		switch(type){
		case WAN_TE3_LIU_LB_NORMAL:
			break;
		case WAN_TE3_LIU_LB_ANALOG:
			data |= BIT_LINE_INTERFACE_DRIVE_LLOOP;
			data &= ~BIT_LINE_INTERFACE_DRIVE_RLOOP;
			break;
		case WAN_TE3_LIU_LB_REMOTE:
			data &= ~BIT_LINE_INTERFACE_DRIVE_LLOOP;
			data |= BIT_LINE_INTERFACE_DRIVE_RLOOP;
			break;
		case WAN_TE3_LIU_LB_DIGITAL:
			data |= BIT_LINE_INTERFACE_DRIVE_LLOOP;
			data |= BIT_LINE_INTERFACE_DRIVE_RLOOP;
			break;
		default :
			DEBUG_EVENT("%s: (T3/E3) Unknown loopback mode!\n",
					fe->name);
			break;
		}		
	}
	WRITE_REG(REG_LINE_INTERFACE_DRIVE, data);
	return 0;
}

/******************************************************************************
 *				sdla_te3_old_get_lb()	
 *
 * Description:
 * Arguments:
 * Returns:
 *****************************************************************************/
static u32 sdla_te3_old_get_lb(sdla_fe_t *fe) 
{
	u32	type = 0;

	WAN_ASSERT(fe->write_fe_reg == NULL);
	WAN_ASSERT(fe->read_fe_reg == NULL);

	if ((fe->te3_param.cpld_status & BIT_CPLD_STATUS_LLB) && 
	    (fe->te3_param.cpld_status & BIT_CPLD_STATUS_RLB)){
		wan_set_bit(WAN_TE3_LIU_LB_DIGITAL, &type);
	}else if (fe->te3_param.cpld_status & BIT_CPLD_STATUS_LLB){
		wan_set_bit(WAN_TE3_LIU_LB_ANALOG, &type);
	}else if (fe->te3_param.cpld_status & BIT_CPLD_STATUS_RLB){
		wan_set_bit(WAN_TE3_LIU_LB_REMOTE, &type);
	}
	return type;
}

/******************************************************************************
 *				sdla_tee_get_lb()	
 *
 * Description:
 * Arguments:
 * Returns:
 *****************************************************************************/
static u32 sdla_te3_get_lb(sdla_fe_t *fe) 
{
	u32	type = 0;
	u8	data;

	WAN_ASSERT(fe->write_fe_reg == NULL);
	WAN_ASSERT(fe->read_fe_reg == NULL);

	data = READ_REG(REG_LINE_INTERFACE_DRIVE);
	if ((data & BIT_LINE_INTERFACE_DRIVE_LLOOP) && (data & BIT_LINE_INTERFACE_DRIVE_RLOOP)){
		wan_set_bit(WAN_TE3_LIU_LB_DIGITAL, &type);
	}else if (data & BIT_LINE_INTERFACE_DRIVE_LLOOP){
		wan_set_bit(WAN_TE3_LIU_LB_ANALOG, &type);
	}else if (data & BIT_LINE_INTERFACE_DRIVE_RLOOP){
		wan_set_bit(WAN_TE3_LIU_LB_REMOTE, &type);
	}
	return type;
}

/******************************************************************************
*				sdla_te3_udp_lb()	
*
* Description:
* Arguments:	
* Returns:
******************************************************************************/
static int sdla_te3_udp_lb(sdla_fe_t *fe, unsigned char *data)
{
	sdla_t			*card = (sdla_t*)fe->card;
	sdla_fe_lbmode_t	*lb = (sdla_fe_lbmode_t*)data;
	int			err = 0;

	WAN_ASSERT(card == NULL);
	if (lb->cmd == WAN_FE_LBMODE_CMD_SET){

		/* Activate/Deactivate Line Loopback modes */
		if (card->adptr_subtype == AFT_SUBTYPE_NORMAL){
			err = sdla_te3_old_set_lb_modes(fe, lb->type, lb->mode); 
		}else if (card->adptr_subtype == AFT_SUBTYPE_SHARK){
			err = sdla_te3_set_lb_modes(fe, lb->type, lb->mode); 
		}

	}else if (lb->cmd == WAN_FE_LBMODE_CMD_SET){

		if (card->adptr_subtype == AFT_SUBTYPE_NORMAL){
			lb->type_map = sdla_te3_old_get_lb(fe);
		}else if (card->adptr_subtype == AFT_SUBTYPE_SHARK){
			lb->type_map= sdla_te3_get_lb(fe);
		}
	}else{
		return -EINVAL;
	}
	return err;
}

/******************************************************************************
 *				sdla_te3_udp()	
 *
 * Description:
 * Arguments:	
 * Returns:
 ******************************************************************************
 */
static int sdla_te3_udp(sdla_fe_t *fe, void *pudp_cmd, unsigned char *data)
{
	wan_femedia_t		*fe_media = NULL;
	sdla_fe_debug_t	*fe_debug = NULL;
	wan_cmd_t		*udp_cmd = (wan_cmd_t*)pudp_cmd;

	switch(udp_cmd->wan_cmd_command){
	case WAN_GET_MEDIA_TYPE:
		fe_media = (wan_femedia_t*)data;
		memset(fe_media, 0, sizeof(wan_femedia_t));
		fe_media->media	= fe->fe_cfg.media;
		udp_cmd->wan_cmd_return_code = WAN_CMD_OK;
		udp_cmd->wan_cmd_data_len = sizeof(wan_femedia_t); 
		break;

	case WAN_FE_LB_MODE:
		if (sdla_te3_udp_lb(fe, data)){
		    	udp_cmd->wan_cmd_return_code	= WAN_UDP_FAILED_CMD;
	    		udp_cmd->wan_cmd_data_len	= 0x00;
			break;
		}
	    	udp_cmd->wan_cmd_return_code = WAN_CMD_OK;
	    	udp_cmd->wan_cmd_data_len = sizeof(sdla_fe_lbmode_t);
		break;

	case WAN_FE_GET_STAT:
 	        /* TE1_56K Read T1/E1/56K alarms */
    		sdla_te3_read_pmon(fe, 0);
		if (udp_cmd->wan_cmd_fe_force){
			/* force to read FE alarms */
			DEBUG_EVENT("%s: Force to read Front-End alarms\n",
						fe->name);
			fe->fe_stats.alarms = 
				sdla_te3_read_alarms(fe, WAN_FE_ALARM_READ|WAN_FE_ALARM_UPDATE);
		}
	        memcpy(&data[0], &fe->fe_stats, sizeof(sdla_fe_stats_t));
	        udp_cmd->wan_cmd_return_code = WAN_CMD_OK;
	    	udp_cmd->wan_cmd_data_len = sizeof(sdla_fe_stats_t); 
		break;

 	case WAN_FE_FLUSH_PMON:
		/* TE1 Flush T1/E1 pmon counters */
		sdla_te3_flush_pmon(fe);
	        udp_cmd->wan_cmd_return_code = WAN_CMD_OK;
		break;
 
	case WAN_FE_GET_CFG:
		/* Read T1/E1 configuration */
	       	memcpy(&data[0],
	    		&fe->fe_cfg,
		      	sizeof(sdla_fe_cfg_t));
	    	udp_cmd->wan_cmd_return_code = WAN_CMD_OK;
    	    	udp_cmd->wan_cmd_data_len = sizeof(sdla_te_cfg_t);
	    	break;

	case WAN_FE_SET_DEBUG_MODE:
		fe_debug = (sdla_fe_debug_t*)&data[0];
		switch(fe_debug->type){
		case WAN_FE_DEBUG_REG:
			if (fe_debug->fe_debug_reg.read){
				fe_debug->fe_debug_reg.value = 
						READ_REG(fe_debug->fe_debug_reg.reg);
			}else{
				WRITE_REG(fe_debug->fe_debug_reg.reg, fe_debug->fe_debug_reg.value);
			}
			udp_cmd->wan_cmd_return_code = WAN_CMD_OK;
			break;
		
		default:
			udp_cmd->wan_cmd_return_code = WAN_UDP_INVALID_CMD;
			break;
		}
    	    	udp_cmd->wan_cmd_data_len = 0;
		break;

	default:
		udp_cmd->wan_cmd_return_code = WAN_UDP_INVALID_CMD;
	    	udp_cmd->wan_cmd_data_len = 0;
		break;
	}
	return 0;
}


#if 0
/* NC: Interrupt mode is not used in T3 
   We run only poll. This function is still here for
   historical reasons */

static int sdla_te3_set_intr(sdla_fe_t *fe)
{
	sdla_fe_cfg_t	*fe_cfg = &fe->fe_cfg;

	WAN_ASSERT(fe->write_fe_reg == NULL);
	WAN_ASSERT(fe->read_fe_reg == NULL);

	DEBUG_EVENT("%s: Enabling interrupts for %s (%d)!\n",
					fe->name, FE_MEDIA_DECODE(fe), IS_DS3(fe_cfg));
	/* Enable Framer Interrupts */
	/* 1. Block Interrupt Enable */
	WRITE_REG(REG_BLOCK_INT_ENABLE,
			BIT_BLOCK_INT_ENABLE_RxDS3_E3 |
			BIT_BLOCK_INT_ENABLE_TxDS3_E3);
	if (IS_DS3(fe_cfg)){
		/* 1. RxDS3 Interrupt Enable */
		WRITE_REG(REG_RxDS3_INT_ENABLE, 
				BIT_RxDS3_INT_ENABLE_CPBIT_ERR	|
				BIT_RxDS3_INT_ENABLE_LOS 	|
				BIT_RxDS3_INT_ENABLE_OOF	|
				BIT_RxDS3_INT_ENABLE_AIS	|
				BIT_RxDS3_INT_ENABLE_IDLE	|
				BIT_RxDS3_INT_ENABLE_FERF	|
				BIT_RxDS3_INT_ENABLE_AIC	|
				BIT_RxDS3_INT_ENABLE_PBIT_ERR	);

		/* RxDS3 FEAC */
		WRITE_REG(REG_RxDS3_FEAC_INT, 
				BIT_RxDS3_FEAC_REMOVE_INT_EN	|
				BIT_RxDS3_FEAC_VALID_INT_EN);

		/* RxDS3 LAPD */
		WRITE_REG(REG_TxDS3_LAPD_STATUS, 
				BIT_TxDS3_LAPD_STATUS_INT_EN);

	}else if (IS_E3(fe_cfg)){
		/* RxE3 Interrupt Enable 1 (0x12) */
		WRITE_REG(REG_RxE3_INT_ENABLE_1,
				BIT_RxE3_INT_ENABLE_OOF	|
				BIT_RxE3_INT_ENABLE_LOS	|
				BIT_RxE3_INT_ENABLE_LOF	|
				BIT_RxE3_INT_ENABLE_AIS);

		/* RxE3 Interrupt Enable 2 (0x13) */
		WRITE_REG(REG_RxE3_INT_ENABLE_2,
				BIT_RxE3_INT_ENABLE_FERF	|
				BIT_RxE3_INT_ENABLE_FRAMING);
	}else{
		return -EINVAL;
	}

	return 0;
}
#endif


/******************************************************************************
 *				sdla_te3_liu_config()	
 *
 * Description: Configure Sangoma TE3 board
 * Arguments:	
 * Returns:	WAN_TRUE - TE3 configred successfully, otherwise WAN_FALSE.
 ******************************************************************************
 */
static int 
sdla_te3_liu_config(sdla_fe_t *fe, sdla_te3_liu_cfg_t *liu, char *name)
{
	sdla_fe_cfg_t	*fe_cfg = &fe->fe_cfg;
	unsigned char	data = 0x00;

	WAN_ASSERT(fe->write_cpld == NULL);
	if (fe_cfg->media == WAN_MEDIA_E3){
		data |= BIT_CPLD_CNTRL_E3;
	}
	if (liu->rx_equal == WAN_TRUE){
		DEBUG_TE3("%s: (T3/E3) Enable Receive Equalizer\n",
				name);
		data |= BIT_CPLD_CNTRL_REQEN;
	}else{
		DEBUG_TE3("%s: (T3/E3) Disable Receive Equalizer\n",
				name);
		data &= ~BIT_CPLD_CNTRL_REQEN;
	}
	if (liu->tx_lbo == WAN_TRUE){
		DEBUG_TE3("%s: (T3/E2) Enable Transmit Build-out\n",
				name);
		data |= BIT_CPLD_CNTRL_TxLEV;
	}else{
		DEBUG_TE3("%s: (T3/E3) Disable Transmit Build-out\n",
				name);
		data &= ~BIT_CPLD_CNTRL_TxLEV;
	}
	/* Write value to CPLD Control register */
	WRITE_CPLD(REG_CPLD_CNTRL, data);
	
	data = 0x00;
	if (liu->taos == WAN_TRUE){
		DEBUG_TE3("%s: (T3/E3) Enable Transmit All Ones\n",
				name);
		data |= BIT_CPLD_STATUS_TAOS;
	}else{
		DEBUG_TE3("%s: (T3/E3) Disable Transmit All Ones\n",
				name);
		data &= ~BIT_CPLD_STATUS_TAOS;
	}
	/* Write value to CPLD Status register */
	WRITE_CPLD(REG_CPLD_STATUS, data);
	return 0;
}

/******************************************************************************
 *				sdla_te3_shark_liu_config()	
 *
 * Description: Configure Sangoma TE3 Shark board
 * Arguments:	
 * Returns:	WAN_TRUE - TE3 configred successfully, otherwise WAN_FALSE.
 ******************************************************************************
 */
static int 
sdla_te3_shark_liu_config(sdla_fe_t *fe, sdla_te3_liu_cfg_t *liu, char *name)
{
	sdla_fe_cfg_t	*fe_cfg = &fe->fe_cfg;
	unsigned char	data = 0x00;

	WAN_ASSERT(fe->write_fe_cpld == NULL);
	fe->te3_param.cpld_cntrl = 0x00;
	if (fe_cfg->media == WAN_MEDIA_E3){
		fe->te3_param.cpld_cntrl |= BIT_EXAR_CPLD_CNTRL_E3;
	}
	/* Write value to CPLD Control register */
	WRITE_EXAR_CPLD(REG_EXAR_CPLD_CNTRL, fe->te3_param.cpld_cntrl);
	
	data = 0x00;
	if (liu->rx_equal == WAN_TRUE){
		DEBUG_TE3("%s: (T3/E3) Enable Receive Equalizer\n",
				name);
		data |= BIT_LINE_INTERFACE_DRIVE_REQB;
	}else{
		DEBUG_TE3("%s: (T3/E3) Disable Receive Equalizer\n",
				name);
		data &= ~BIT_LINE_INTERFACE_DRIVE_REQB;
	}
	if (liu->tx_lbo == WAN_TRUE){
		DEBUG_TE3("%s: (T3/E2) Enable Transmit Build-out\n",
				name);
		data |= BIT_LINE_INTERFACE_DRIVE_TxLEV;
	}else{
		DEBUG_TE3("%s: (T3/E3) Disable Transmit Build-out\n",
				name);
		data &= ~BIT_LINE_INTERFACE_DRIVE_TxLEV;
	}
	if (liu->taos == WAN_TRUE){
		DEBUG_TE3("%s: (T3/E3) Enable Transmit All Ones\n",
				name);
		data |= BIT_LINE_INTERFACE_DRIVE_TAOS;
	}else{
		DEBUG_TE3("%s: (T3/E3) Disable Transmit All Ones\n",
				name);
		data &= ~BIT_LINE_INTERFACE_DRIVE_TAOS;
	}
	
	
	return 0;
}


int sdla_te3_iface_init(void *p_fe_iface)
{
	sdla_fe_iface_t	*fe_iface = (sdla_fe_iface_t*)p_fe_iface;

	/* Inialize Front-End interface functions */
	fe_iface->config		= &sdla_te3_config;
	fe_iface->unconfig		= &sdla_te3_unconfig;
	fe_iface->post_init		= &sdla_te3_post_init;
	fe_iface->post_unconfig	= &sdla_te3_post_unconfig;
	fe_iface->polling		= &sdla_te3_polling;
	fe_iface->isr			= &sdla_te3_isr;
	fe_iface->process_udp		= &sdla_te3_udp;
	fe_iface->read_alarm		= &sdla_te3_read_alarms;
	fe_iface->read_pmon		= &sdla_te3_read_pmon;
	fe_iface->flush_pmon		= &sdla_te3_flush_pmon;
	fe_iface->set_fe_alarm		= &sdla_te3_set_alarm;
	fe_iface->get_fe_status		= &sdla_te3_get_fe_status;
	fe_iface->get_fe_media		= &sdla_te3_get_fe_media;
	fe_iface->get_fe_media_string	= &sdla_te3_get_fe_media_string;
	fe_iface->update_alarm_info	= &sdla_te3_update_alarm_info;
	fe_iface->update_pmon_info	= &sdla_te3_update_pmon_info;

	return 0;
}
static int sdla_te3_config(void *p_fe)
{
	sdla_fe_t	*fe = (sdla_fe_t*)p_fe;
	sdla_t*		card = (sdla_t*)fe->card;
	sdla_fe_cfg_t	*fe_cfg = &fe->fe_cfg;
	sdla_te3_cfg_t	*te3_cfg = &fe_cfg->cfg.te3_cfg;
	u16		adptr_subtype;
	unsigned char	data = 0x00;
	
	WAN_ASSERT(fe->write_fe_reg == NULL);
	WAN_ASSERT(fe->read_fe_reg == NULL);

	card->hw_iface.getcfg(card->hw, SDLA_ADAPTERSUBTYPE, &adptr_subtype);
	
	data = READ_REG(0x02);


	data=0x00;
	
	/* configure Line Interface Unit */
	if (card->adptr_subtype == AFT_SUBTYPE_NORMAL){
		sdla_te3_liu_config(fe, &te3_cfg->liu_cfg, fe->name);
	}else if (card->adptr_subtype == AFT_SUBTYPE_SHARK){
		sdla_te3_shark_liu_config(fe, &te3_cfg->liu_cfg, fe->name);	
	}else{
		DEBUG_EVENT("%s: Unknown Adapter Subtype (%X)\n",
				fe->name, card->adptr_subtype);
		return -EINVAL;
	}

	switch(fe_cfg->media){
	case WAN_MEDIA_DS3:
		DEBUG_TE3("%s: (T3/E3) Media type DS3\n",
				fe->name);
		data |= BIT_OPMODE_DS3;
		break;
	case WAN_MEDIA_E3:
		DEBUG_TE3("%s: (T3/E3) Media type E3\n",
				fe->name);
		data &= ~BIT_OPMODE_DS3;
		break;
	default:
		DEBUG_EVENT("%s: Invalid Media type 0x%X\n",
				fe->name,fe_cfg->media);
		return -EINVAL;
	}

	switch(WAN_FE_FRAME(fe)){
	case WAN_FR_E3_G751:
		if (fe_cfg->media != WAN_MEDIA_E3){
			DEBUG_EVENT("%s: (T3/E3) Invalid Frame Format!\n",
					fe->name);
			return -EINVAL;
		}
		DEBUG_TE3("%s: (T3/E3) Frame type G.751\n",
				fe->name);
		data &= ~BIT_OPMODE_FRAME_FRMT;
		break;
	case WAN_FR_E3_G832:
		if (fe_cfg->media != WAN_MEDIA_E3){
			DEBUG_EVENT("%s: (T3/E3) Invalid Frame Format!\n",
					fe->name);
			return -EINVAL;
		}
		DEBUG_TE3("%s: (T3/E3) Frame type G.832\n",
				fe->name);
		data |= BIT_OPMODE_FRAME_FRMT;
		break;
	case WAN_FR_DS3_Cbit:
		if (fe_cfg->media != WAN_MEDIA_DS3){
			DEBUG_EVENT("%s: (T3/E3) Invalid Frame Format!\n",
					fe->name);
			return -EINVAL;
		}
		DEBUG_TE3("%s: (T3/E3) Frame type C-bit Parity\n",
				fe->name);
		data &= ~BIT_OPMODE_FRAME_FRMT;
		break;
	case WAN_FR_DS3_M13:
		if (fe_cfg->media != WAN_MEDIA_DS3){
			DEBUG_EVENT("%s: (T3/E3) Invalid Frame Format!\n",
					fe->name);
			return -EINVAL;
		}
		DEBUG_TE3("%s: (T3/E3) Frame type M13\n",
				fe->name);
		data |= BIT_OPMODE_FRAME_FRMT;
		break;
	default:
		DEBUG_EVENT("%s: Invalid Frame type 0x%X\n",
				fe->name,fe_cfg->frame);
		return -EINVAL;
	}
	data |= (BIT_OPMODE_TIMREFSEL1 | BIT_OPMODE_TIMREFSEL0);
	WRITE_REG(REG_OPMODE, data);

	data = 0x00;
	switch(WAN_FE_LCODE(fe)){
	case WAN_LCODE_AMI: 
		DEBUG_TE3("%s: (T3/E3) Line code AMI\n",
				fe->name);
		data |= BIT_IO_CONTROL_AMI;
		break;
	case WAN_LCODE_HDB3:
		DEBUG_TE3("%s: (T3/E3) Line code HDB3\n",
				fe->name);
		data &= ~BIT_IO_CONTROL_AMI;
		break;
	case WAN_LCODE_B3ZS:
		DEBUG_TE3("%s: (T3/E3) Line code B3ZS\n",
				fe->name);
		data &= ~BIT_IO_CONTROL_AMI;
		break;
	default:
		DEBUG_EVENT("%s: Invalid Lcode 0x%X\n",
				fe->name,fe_cfg->lcode);
		return -EINVAL;
	}
	data |= BIT_IO_CONTROL_DISABLE_TXLOC;
	data |= BIT_IO_CONTROL_DISABLE_RXLOC;
	data |= BIT_IO_CONTROL_RxLINECLK;
	WRITE_REG(REG_IO_CONTROL, data);

	fe->fe_param.te3.e3_lb_ctrl=0;
	fe->fe_param.te3.e3_connect_delay=10;

	/* Initialize Front-End parameters */
	fe->fe_status	= FE_DISCONNECTED;

	DEBUG_EVENT("%s: %s disconnected!\n",
					fe->name, FE_MEDIA_DECODE(fe));
	sdla_te3_read_alarms(fe, 1);

	//sdla_te3_set_intr(fe);
	return 0;
}


static int sdla_te3_add_timer(sdla_fe_t* fe, unsigned long delay)
{
	int	err=0;

	if (wan_test_bit(TE_TIMER_KILL,(void*)&fe->te3_param.critical)) {
		DEBUG_EVENT("WARNING: %s: %s() Timer has been killed!\n", 
					fe->name,__FUNCTION__);
		return 0;
	}
	
	if (wan_test_bit(TE_TIMER_RUNNING,(void*)&fe->te3_param.critical)) {
		DEBUG_ERROR("Error: %s: %s() Timer already running!\n", 
					fe->name,__FUNCTION__);
		return 0;
	}
	
	wan_set_bit(TE_TIMER_RUNNING,(void*)&fe->te3_param.critical);


	err = wan_add_timer(&fe->timer, delay * HZ / 1000);

	if (err){
		wan_clear_bit(TE_TIMER_RUNNING,(void*)&fe->te3_param.critical);
		DEBUG_ERROR("Error: %s: %s() Failed to add Timer err=%i!\n", 
					fe->name,__FUNCTION__,err);
		/* Failed to add timer */
		return -EINVAL;
	}
	return 0;	
}



#if defined(__NetBSD__) || defined(__FreeBSD__) || defined(__OpenBSD__)
static void sdla_te3_timer(void* pfe)
#elif defined(__WINDOWS__)
static void sdla_te3_timer(IN PKDPC Dpc, void* pfe, void* arg2, void* arg3)
#else
static void sdla_te3_timer(unsigned long pfe)
#endif
{
	sdla_fe_t	*fe = (sdla_fe_t*)pfe;
	sdla_t 		*card = (sdla_t*)fe->card;
	wan_device_t	*wandev = &card->wandev;

	DEBUG_TEST("[TE3] %s: TE3 timer!\n", fe->name);
	if (wan_test_bit(TE_TIMER_KILL,(void*)&fe->te3_param.critical)){
		wan_clear_bit(TE_TIMER_RUNNING,(void*)&fe->te3_param.critical);
		DEBUG_EVENT("WARNING: %s: Timer has been killed!\n", 
					fe->name);
		return;
	}
	if (!wan_test_bit(TE_TIMER_RUNNING,(void*)&fe->te3_param.critical)){
		/* Somebody clear this bit */
		DEBUG_EVENT("WARNING: %s: Timer bit is cleared (should never happened)!\n", 
					fe->name);
		return;
	}
	wan_clear_bit(TE_TIMER_RUNNING,(void*)&fe->te3_param.critical);

	if (wandev->fe_enable_timer){
		wandev->fe_enable_timer(fe->card);
	}

	sdla_te3_add_timer(fe, 1000);

	return;
}
 

static int sdla_te3_post_init(void* pfe)
{
	sdla_fe_t		*fe = (sdla_fe_t*)pfe;


	/* Initialize and start T1/E1 timer */
	wan_set_bit(TE_TIMER_KILL,(void*)&fe->te3_param.critical);
	
	wan_init_timer(
		&fe->timer, 
		sdla_te3_timer,
	       	(wan_timer_arg_t)fe);

	/* Initialize T1/E1 timer */
	wan_clear_bit(TE_TIMER_KILL,(void*)&fe->te3_param.critical);

	/* Start T1/E1 timer */
	if (IS_E3(&fe->fe_cfg)){
		sdla_te3_add_timer(fe, 5000);
	} else {
		sdla_te3_add_timer(fe, 1000);
	}
	return 0;
}

static int sdla_te3_post_unconfig(void* pfe)
{
	sdla_fe_t		*fe = (sdla_fe_t*)pfe;


	/* Kill TE timer poll command */
	wan_set_bit(TE_TIMER_KILL,(void*)&fe->te3_param.critical);
	if (wan_test_bit(TE_TIMER_RUNNING,(void*)&fe->te3_param.critical)){
		wan_del_timer(&fe->timer);
	}
	wan_clear_bit(TE_TIMER_RUNNING,(void*)&fe->te3_param.critical);

	return 0;
}

static int sdla_te3_unconfig(void *p_fe)
{
	sdla_fe_t	*fe = (sdla_fe_t*)p_fe;
	
	wan_set_bit(TE_TIMER_KILL,(void*)&fe->te3_param.critical);

	DEBUG_EVENT("%s: Unconfiguring T3/E3 interface\n",
			fe->name);
	return 0;
}

static int
sdla_te3_update_alarm_info(sdla_fe_t* fe, struct seq_file* m, int* stop_cnt)
{
	if (IS_DS3(&fe->fe_cfg)){
		PROC_ADD_LINE(m,
			"=============================== DS3 Alarms ===============================\n");
		PROC_ADD_LINE(m,
			PROC_STATS_ALARM_FORMAT,
			"AIS", WAN_TE3_AIS_ALARM(fe->fe_alarm), 
			"LOS", WAN_TE3_LOS_ALARM(fe->fe_alarm));
		PROC_ADD_LINE(m,
			PROC_STATS_ALARM_FORMAT,
			"OOF", WAN_TE3_OOF_ALARM(fe->fe_alarm), 
			"YEL", WAN_TE3_YEL_ALARM(fe->fe_alarm));
	}else{
		PROC_ADD_LINE(m,
			"=============================== E3 Alarms ===============================\n");
		PROC_ADD_LINE(m,
			PROC_STATS_ALARM_FORMAT,
			"AIS", WAN_TE3_AIS_ALARM(fe->fe_alarm), 
			"LOS", WAN_TE3_LOS_ALARM(fe->fe_alarm));
		PROC_ADD_LINE(m,
			PROC_STATS_ALARM_FORMAT,
			"OFF", WAN_TE3_OOF_ALARM(fe->fe_alarm), 
			"YEL", WAN_TE3_YEL_ALARM(fe->fe_alarm));
		PROC_ADD_LINE(m,
			PROC_STATS_ALARM_FORMAT,
			"LOF", WAN_TE3_LOF_ALARM(fe->fe_alarm), 
			"", "");
	}
		
	return m->count;
}

static int sdla_te3_update_pmon_info(sdla_fe_t* fe, struct seq_file* m, int* stop_cnt)
{
	PROC_ADD_LINE(m,
		 "=========================== %s PMON counters ============================\n",
		 (IS_DS3(&fe->fe_cfg)) ? "DS3" : "E3");
	PROC_ADD_LINE(m,
		PROC_STATS_PMON_FORMAT,
		"Line Code Violation", fe->fe_stats.u.te3_pmon.pmon_lcv,
		"Framing Bit/Byte Error", fe->fe_stats.u.te3_pmon.pmon_framing);
	if (IS_DS3(&fe->fe_cfg)){
		if (WAN_FE_FRAME(fe) == WAN_FR_DS3_Cbit){
			PROC_ADD_LINE(m,
				PROC_STATS_PMON_FORMAT,
				"Parity Error", fe->fe_stats.u.te3_pmon.pmon_parity,
				"CP-Bit Error Event", fe->fe_stats.u.te3_pmon.pmon_cpbit);
		}else{
			PROC_ADD_LINE(m,
				PROC_STATS_PMON_FORMAT,
				"Parity Error", fe->fe_stats.u.te3_pmon.pmon_parity,
				"FEBE Event", fe->fe_stats.u.te3_pmon.pmon_febe);

		}
	}else{
		PROC_ADD_LINE(m,
			PROC_STATS_PMON_FORMAT,
			"Parity Error", fe->fe_stats.u.te3_pmon.pmon_parity,
			"FEBE Event", fe->fe_stats.u.te3_pmon.pmon_febe);
	}
	
	return m->count;
}
