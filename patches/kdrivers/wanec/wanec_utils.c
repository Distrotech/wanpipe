/******************************************************************************
** Copyright (c) 2005
**	Alex Feldman <al.feldman@sangoma.com>.  All rights reserved.
**
** ============================================================================
** Aprl 29, 2001	Alex Feldma	Initial version.
******************************************************************************/

/******************************************************************************
**			   INCLUDE FILES
******************************************************************************/


#include "wanpipe_includes.h"
#include "wanpipe_defines.h"
#include "wanpipe_debug.h"
#include "wanpipe.h"
#if defined(__LINUX__)
# include "if_wanpipe.h"
#endif

#include "oct6100_api.h"
#include "oct6100_version.h"

#include "wanec_iface.h"
#include "wanec_iface_api.h"

/******************************************************************************
**			  DEFINES AND MACROS
******************************************************************************/
#define	DTYPE_BOOL	1
#define	DTYPE_UINT32	2
#define	DTYPE_INT32	3

#if !defined(offsetof)
# define offsetof(s, e) ((size_t)&((s*)0)->e)
#endif
/******************************************************************************
**			STRUCTURES AND TYPEDEFS
******************************************************************************/
typedef struct key_word		/* Keyword table entry */
{
	u_int8_t	*key;		/* -> keyword */
	u_int16_t	offset;		/* offset of the related parameter */
	u_int16_t	dtype;		/* data type */
} key_word_t;

typedef struct value_word		/* Keyword table entry */
{
	u_int8_t	*sValue;	/* -> keyword */
	u_int32_t	dValue;		/* offset of the related parameter */
} value_word_t;

/******************************************************************************
**			   GLOBAL VARIABLES
******************************************************************************/
/* Driver version string */
extern int	verbose;

key_word_t chip_param[] =
{
	{ "WANEC_TailDisplacement",
	  offsetof(tOCT6100_CHIP_OPEN, ulTailDisplacement),
	  DTYPE_UINT32 },
	{ "WANEC_MaxPlayoutBuffers",
	  offsetof(tOCT6100_CHIP_OPEN, ulMaxPlayoutBuffers),
	  DTYPE_UINT32 },
	{ "WANEC_MaxConfBridges",
	  offsetof(tOCT6100_CHIP_OPEN, ulMaxConfBridges),
	  DTYPE_UINT32 },
	{ "WANEC_EnableExtToneDetection",
	  offsetof(tOCT6100_CHIP_OPEN, fEnableExtToneDetection),
	  DTYPE_BOOL },
	{ "WANEC_EnableAcousticEcho",
	  offsetof(tOCT6100_CHIP_OPEN, fEnableAcousticEcho),
	  DTYPE_BOOL },
	{ NULL, 0, 0 }
};

key_word_t ChannelModifyParam[] =
{
	{ "WANEC_EchoOperationMode",
	  offsetof(tOCT6100_CHANNEL_MODIFY, ulEchoOperationMode),
	  DTYPE_UINT32 },
	{ "WANEC_EnableToneDisabler",
	  offsetof(tOCT6100_CHANNEL_MODIFY, fEnableToneDisabler),
	  DTYPE_BOOL },
	{ "WANEC_ApplyToAllChannels",
	  offsetof(tOCT6100_CHANNEL_MODIFY, fApplyToAllChannels),
	  DTYPE_BOOL },
	{ "WANEC_DisableToneDetection",
	  offsetof(tOCT6100_CHANNEL_MODIFY, fDisableToneDetection),
	  DTYPE_BOOL },
	/****************  tOCT6100_CHANNEL_MODIFY_TDM  **********************/
	/****************  tOCT6100_CHANNEL_MODIFY_VQE  **********************/
	{ "WANEC_EnableNlp",
	  offsetof(tOCT6100_CHANNEL_MODIFY, VqeConfig)+offsetof(tOCT6100_CHANNEL_MODIFY_VQE,fEnableNlp),
	  DTYPE_BOOL },
	{ "WANEC_EnableTailDisplacement",
	  offsetof(tOCT6100_CHANNEL_MODIFY, VqeConfig)+offsetof(tOCT6100_CHANNEL_MODIFY_VQE,fEnableTailDisplacement),
	  DTYPE_BOOL },
	{ "WANEC_TailDisplacement",
	  offsetof(tOCT6100_CHANNEL_MODIFY, VqeConfig)+offsetof(tOCT6100_CHANNEL_MODIFY_VQE,ulTailDisplacement),
	  DTYPE_UINT32 },
	{ "WANEC_RinLevelControl",
	  offsetof(tOCT6100_CHANNEL_MODIFY, VqeConfig)+offsetof(tOCT6100_CHANNEL_MODIFY_VQE,fRinLevelControl),
	  DTYPE_BOOL },
	{ "WANEC_RinLevelControlGainDb",
	  offsetof(tOCT6100_CHANNEL_MODIFY, VqeConfig)+offsetof(tOCT6100_CHANNEL_MODIFY_VQE,lRinLevelControlGainDb),
	  DTYPE_INT32 },
	{ "WANEC_SoutLevelControl",
	  offsetof(tOCT6100_CHANNEL_MODIFY, VqeConfig)+offsetof(tOCT6100_CHANNEL_MODIFY_VQE,fSoutLevelControl),
	  DTYPE_BOOL },
	{ "WANEC_SoutLevelControlGainDb",
	  offsetof(tOCT6100_CHANNEL_MODIFY, VqeConfig)+offsetof(tOCT6100_CHANNEL_MODIFY_VQE,lSoutLevelControlGainDb),
	  DTYPE_INT32 },
	{ "WANEC_RinAutomaticLevelControl",
	  offsetof(tOCT6100_CHANNEL_MODIFY, VqeConfig)+offsetof(tOCT6100_CHANNEL_MODIFY_VQE,fRinAutomaticLevelControl),
	  DTYPE_BOOL },
	{ "WANEC_RinAutomaticLevelControlTargetDb",
	  offsetof(tOCT6100_CHANNEL_MODIFY, VqeConfig)+offsetof(tOCT6100_CHANNEL_MODIFY_VQE,lRinAutomaticLevelControlTargetDb),
	  DTYPE_INT32 },
	{ "WANEC_SoutAutomaticLevelControl",
	  offsetof(tOCT6100_CHANNEL_MODIFY, VqeConfig)+offsetof(tOCT6100_CHANNEL_MODIFY_VQE,fSoutAutomaticLevelControl),
	  DTYPE_BOOL },
	{ "WANEC_SoutAutomaticLevelControlTargetDb",
	  offsetof(tOCT6100_CHANNEL_MODIFY, VqeConfig)+offsetof(tOCT6100_CHANNEL_MODIFY_VQE,lSoutAutomaticLevelControlTargetDb),
	  DTYPE_INT32 },
#if 0
	{ "WANEC_AlcNoiseBleedOutTime",
	  offsetof(tOCT6100_CHANNEL_MODIFY, VqeConfig)+offsetof(tOCT6100_CHANNEL_MODIFY_VQE,ulAlcNoiseBleedOutTime),
	  DTYPE_UINT32 },
#endif
	{ "WANEC_RinHighLevelCompensation",
	  offsetof(tOCT6100_CHANNEL_MODIFY, VqeConfig)+offsetof(tOCT6100_CHANNEL_MODIFY_VQE,fRinHighLevelCompensation),
	  DTYPE_BOOL },
	{ "WANEC_RinHighLevelCompensationThresholdDb",
	  offsetof(tOCT6100_CHANNEL_MODIFY, VqeConfig)+offsetof(tOCT6100_CHANNEL_MODIFY_VQE,lRinHighLevelCompensationThresholdDb),
	  DTYPE_INT32 },
	{ "WANEC_SoutAdaptiveNoiseReduction",
	  offsetof(tOCT6100_CHANNEL_MODIFY, VqeConfig)+offsetof(tOCT6100_CHANNEL_MODIFY_VQE,fSoutAdaptiveNoiseReduction),
	  DTYPE_BOOL },
	{ "WANEC_RoutNoiseReduction",
	  offsetof(tOCT6100_CHANNEL_MODIFY, VqeConfig)+offsetof(tOCT6100_CHANNEL_MODIFY_VQE,fRoutNoiseReduction),
	  DTYPE_BOOL },
	{ "WANEC_ComfortNoiseMode",
	  offsetof(tOCT6100_CHANNEL_MODIFY, VqeConfig)+offsetof(tOCT6100_CHANNEL_MODIFY_VQE,ulComfortNoiseMode),
	  DTYPE_UINT32 },
	{ "WANEC_DtmfToneRemoval",
	  offsetof(tOCT6100_CHANNEL_MODIFY, VqeConfig)+offsetof(tOCT6100_CHANNEL_MODIFY_VQE,fDtmfToneRemoval),
	  DTYPE_BOOL },
	{ "WANEC_AcousticEcho",
	  offsetof(tOCT6100_CHANNEL_MODIFY, VqeConfig)+offsetof(tOCT6100_CHANNEL_MODIFY_VQE,fAcousticEcho),
	  DTYPE_BOOL },
	{ "WANEC_SoutNoiseBleaching",
	  offsetof(tOCT6100_CHANNEL_MODIFY, VqeConfig)+offsetof(tOCT6100_CHANNEL_MODIFY_VQE,fSoutNoiseBleaching),
	  DTYPE_BOOL },
	{ "WANEC_NonLinearityBehaviorA",
	  offsetof(tOCT6100_CHANNEL_MODIFY, VqeConfig)+offsetof(tOCT6100_CHANNEL_MODIFY_VQE,ulNonLinearityBehaviorA),
	  DTYPE_UINT32 },
	{ "WANEC_NonLinearityBehaviorB",
	  offsetof(tOCT6100_CHANNEL_MODIFY, VqeConfig)+offsetof(tOCT6100_CHANNEL_MODIFY_VQE,ulNonLinearityBehaviorB),
	  DTYPE_UINT32 },
	{ "WANEC_DoubleTalkBehavior",
	  offsetof(tOCT6100_CHANNEL_MODIFY, VqeConfig)+offsetof(tOCT6100_CHANNEL_MODIFY_VQE,ulDoubleTalkBehavior),
	  DTYPE_UINT32 },
	{ "WANEC_AnrSnrEnhancementDb",
	  offsetof(tOCT6100_CHANNEL_MODIFY, VqeConfig)+offsetof(tOCT6100_CHANNEL_MODIFY_VQE,lAnrSnrEnhancementDb),
	  DTYPE_INT32 },

	{ "WANEC_AecTailLength",
	  offsetof(tOCT6100_CHANNEL_MODIFY, VqeConfig)+offsetof(tOCT6100_CHANNEL_MODIFY_VQE, ulAecTailLength),
	  DTYPE_INT32 },

	{ NULL, 0, 0 }
};

value_word_t ValueParamList[] =
{
	{ "OPMODE_NORMAL",		cOCT6100_ECHO_OP_MODE_NORMAL },
	{ "OPMODE_HT_FREEZE",		cOCT6100_ECHO_OP_MODE_HT_FREEZE },
	{ "OPMODE_HT_RESET",		cOCT6100_ECHO_OP_MODE_HT_RESET },
	{ "OPMODE_POWER_DOWN",		cOCT6100_ECHO_OP_MODE_POWER_DOWN },
	{ "OPMODE_EXTERNAL",		cOCT6100_ECHO_OP_MODE_EXTERNAL },
	{ "OPMODE_NO_ECHO",		cOCT6100_ECHO_OP_MODE_NO_ECHO },
	{ "OPMODE_SPEECH_RECOGNITION",	cOCT6100_ECHO_OP_MODE_SPEECH_RECOGNITION },
	{ "OPMODE_G169_ALC",		cOCT6100_ECHO_OP_MODE_G169_ALC },
	{ "COMFORT_NOISE_NORMAL",	cOCT6100_COMFORT_NOISE_NORMAL },
	{ "COMFORT_NOISE_FAST_LATCH",	cOCT6100_COMFORT_NOISE_FAST_LATCH },
	{ "COMFORT_NOISE_EXTENDED",	cOCT6100_COMFORT_NOISE_EXTENDED },
	{ "COMFORT_NOISE_OFF",		cOCT6100_COMFORT_NOISE_OFF },
	{ "DT_BEH_NORMAL",		cOCT6100_DOUBLE_TALK_BEH_NORMAL },
	{ "DT_BEH_LESS_AGGRESSIVE",	cOCT6100_DOUBLE_TALK_BEH_LESS_AGGRESSIVE },

	{ NULL, 0, }
};

/******************************************************************************
** 			FUNCTION PROTOTYPES
******************************************************************************/
int wanec_ChipParam(wan_ec_t*,tPOCT6100_CHIP_OPEN, wan_custom_conf_t*, int);
int wanec_ChanParam(wan_ec_t*,tPOCT6100_CHANNEL_MODIFY, wan_custom_conf_t*, int);
int wanec_ChanParamList(wan_ec_t *ec);

/******************************************************************************
** 			FUNCTION DEFINITIONS
******************************************************************************/
static int
set_conf_param (	wan_ec_t		*ec,
			wan_custom_param_t	*param,
			key_word_t		*dtab,
			void			*conf,
			int			verbose)
{
	/* Search a keyword in configuration definition table */
	for (; dtab->key && wp_strncasecmp(dtab->key, param->name, strlen(param->name)); ++dtab);
	if (dtab->key == NULL){
		DEBUG_EVENT("%s: Failed to find EC parameter (name=%s)\n",
							ec->name, param->name);
		return -EINVAL;	/* keyword not found */
	}

	/* Search a keyword in configuration definition table */
	if (dtab->dtype != DTYPE_BOOL && strlen(param->sValue)){
		value_word_t *ValueParam = ValueParamList;
		for (; ValueParam->sValue && wp_strncasecmp(ValueParam->sValue, param->sValue, strlen(param->sValue)); ++ValueParam);
		if (ValueParam->sValue == NULL){
			DEBUG_EVENT("%s: Failed to find EC parameter value (sValue=%s)\n",
							ec->name, param->sValue);
			return -EINVAL;	/* keyword not found */
		}
		param->dValue = ValueParam->dValue;
	}

	switch (dtab->dtype) {
	case DTYPE_BOOL:
		if (memcmp(param->sValue, "TRUE", 4) == 0){
			*(BOOL*)((char*)conf + dtab->offset) = TRUE;
		}else if (memcmp(param->sValue, "FALSE", 5) == 0){
			*(BOOL*)((char*)conf + dtab->offset) = FALSE;
		}else{
			PRINT1(verbose, "%s: Invalid parameter type (%d)\n",
				ec->name, dtab->dtype);
			return -EINVAL;
		}
		PRINT1(verbose, "%s: * Setting %s to %s\n",
					ec->name, param->name, param->sValue);
		break;
	case DTYPE_UINT32:
		PRINT1(verbose, "%s: * Setting %s to %d\n",
					ec->name, param->name, param->dValue);
		*(UINT32*)((char*)conf + dtab->offset) = param->dValue;
		break;
	case DTYPE_INT32:
		PRINT1(verbose, "%s: * Setting %s to %d\n",
					ec->name, param->name, param->dValue);
		*(INT32*)((char*)conf + dtab->offset) = param->dValue;
		break;
	default:
		return -EINVAL;
	}
	return dtab->offset;
}

int wanec_ChipParam(	wan_ec_t		*ec,
			tPOCT6100_CHIP_OPEN	f_pOpenChip,
			wan_custom_conf_t	*custom_conf,
			int			verbose)
{
	unsigned int	iParam=0;

	PRINT1(verbose, "%s: Parsing advanced Chip params\n",
					ec->name);
	while(iParam < custom_conf->param_no){
		set_conf_param(	ec,
				&custom_conf->params[iParam],
				chip_param,
				f_pOpenChip,
				verbose);
		iParam++;
	}
	return 0;
}


int wanec_ChanParam(	wan_ec_t			*ec,
			tPOCT6100_CHANNEL_MODIFY	f_pChannelModify,
			wan_custom_conf_t		*custom_conf,
			int				verbose)
{
	unsigned int	iParam=0;
	int		err;

	PRINT1(verbose, "%s: Parsing advanced Channel params\n",
						ec->name);
	while(iParam < custom_conf->param_no){
		err = set_conf_param(	ec,
					&custom_conf->params[iParam],
					ChannelModifyParam,
					f_pChannelModify,
					verbose);
		if (err < 0) return -EINVAL;
		if (err < offsetof(tOCT6100_CHANNEL_MODIFY, TdmConfig)){
			/* no extra action */
		}else if (err < offsetof(tOCT6100_CHANNEL_MODIFY, VqeConfig)){
			f_pChannelModify->fTdmConfigModified = TRUE;
		}else if (err < offsetof(tOCT6100_CHANNEL_MODIFY, CodecConfig)){
			f_pChannelModify->fVqeConfigModified = TRUE;
		}else{
			f_pChannelModify->fCodecConfigModified = TRUE;
		}
		iParam++;
	}
	return 0;
}


int wanec_ChanParamList(wan_ec_t *ec)
{
	key_word_t	*dtab = ChannelModifyParam;

	for (; dtab->key; ++dtab){

		switch (dtab->dtype) {

		case DTYPE_UINT32:
			DEBUG_EVENT("%s: Channel paramater %s  = UINT32 value\n",
						ec->name, dtab->key);
			break;
		case DTYPE_INT32:
			DEBUG_EVENT("%s: Channel paramater %s  = INT32 value\n",
						ec->name, dtab->key);
			break;
		case DTYPE_BOOL:
			DEBUG_EVENT("%s: Channel paramater %s  = TRUE/FALSE value\n",
						ec->name, dtab->key);
			break;
		default:
			DEBUG_EVENT("%s: Channel paramater %s  = UNKNOWN value\n",
						ec->name, dtab->key);
			break;
		}
	}
	return 0;
}

