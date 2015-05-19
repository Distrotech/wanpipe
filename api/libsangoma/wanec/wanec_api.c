/******************************************************************************
** Copyright (c) 2005 Sangoma Technologies.  All rights reserved.
**
** ============================================================================
**
** May 21, 2010		Moises Silva    <moy@sangoma.com>	Thread safety (no globals!)
**
** May 12, 2010		David Rokhvarg 	added verbosity control (WANEC_API_PRINT()).
**
** Jul 26, 2006		David Rokhvarg	<davidr@sangoma.com>	Ported to Windows.
**
** Oct 13, 2005		Alex Feldman	Initial version.
******************************************************************************/

/******************************************************************************
**			   INCLUDE FILES
******************************************************************************/
#if !defined(__WINDOWS__)
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <fcntl.h>
#include <time.h>
#include <syslog.h>
#include <sys/param.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/queue.h>
#include <sys/un.h>
#include <netinet/in.h>
#if defined(__LINUX__)
# include <linux/if.h>
# include <linux/types.h>
# include <linux/if_packet.h>
#endif
#endif

#if defined(__LINUX__)
# include <linux/wanpipe_defines.h>
# include <linux/wanpipe_cfg.h>
# include <wanec_iface_api.h>
#elif defined(__WINDOWS__)
# include "wanpipe_includes.h"
# include "sang_api.h"
# include "wanpipe_defines.h"
# include "wanpipe_cfg.h"
# include "wanec_iface_api.h"
#else
# include <wanpipe_defines.h>
# include <wanpipe_cfg.h>
# include <wanec_iface_api.h>
#endif

#include "wanec_api.h"

/******************************************************************************
**			  DEFINES AND MACROS
******************************************************************************/
#ifdef __WINDOWS__
#define WANEC_API_PRINT(...)	if(ec_library_verbose)printf(## __VA_ARGS__)
#else
#define WANEC_API_PRINT(...)	if(ec_library_verbose)printf(__VA_ARGS__)
#endif

/******************************************************************************
**			STRUCTURES AND TYPEDEFS
******************************************************************************/

/******************************************************************************
**			   GLOBAL VARIABLES
******************************************************************************/
int    ec_library_verbose = 1; /* by default, the library will print to stdout */

/******************************************************************************
** 			FUNCTION PROTOTYPES
******************************************************************************/
extern int wanec_api_lib_config(wan_ec_api_t *ec_api, int verbose);
extern int wanec_api_lib_bufferload(wan_ec_api_t *ec_api);
extern int wanec_api_lib_membufferload(wan_ec_api_t *ec_api);
extern int wanec_api_lib_monitor(wan_ec_api_t *ec_api);
extern int wanec_api_lib_cmd(wan_ec_api_t *ec_api);



/******************************************************************************
** 			FUNCTION DEFINITIONS
******************************************************************************/

static int wanec_api_print_chip_stats(wan_ec_api_t *ec_api)
{
	tPOCT6100_CHIP_STATS		pChipStats;

	pChipStats = &ec_api->u_chip_stats.f_ChipStats;
	WANEC_API_PRINT("****** Echo Canceller Chip Get Stats %s ******\n",
			ec_api->devname);
	WANEC_API_PRINT("%10s: Number of channels currently open\t\t\t%d\n", 
			ec_api->devname,
			pChipStats->ulNumberChannels);
	WANEC_API_PRINT("%10s: Number of conference bridges currently open\t\t%d\n", 
			ec_api->devname,
			pChipStats->ulNumberConfBridges);
	WANEC_API_PRINT("%10s: Number of playout buffers currently loaded\t\t%d\n", 
			ec_api->devname,
			pChipStats->ulNumberPlayoutBuffers);
	WANEC_API_PRINT("%10s: Number of framing error on H.100 bus\t\t%d\n", 
			ec_api->devname,
			pChipStats->ulH100OutOfSynchCount);
	WANEC_API_PRINT("%10s: Number of errors on H.100 clock CT_C8_A\t\t%d\n", 
			ec_api->devname,
			pChipStats->ulH100ClockABadCount);
	WANEC_API_PRINT("%10s: Number of errors on H.100 frame CT_FRAME_A\t\t%d\n", 
			ec_api->devname,
			pChipStats->ulH100FrameABadCount);
	WANEC_API_PRINT("%10s: Number of errors on H.100 clock CT_C8_B\t\t%d\n", 
			ec_api->devname,
			pChipStats->ulH100ClockBBadCount);
	WANEC_API_PRINT("%10s: Number of internal read timeout errors\t\t%d\n",
			ec_api->devname,
			pChipStats->ulInternalReadTimeoutCount);
	WANEC_API_PRINT("%10s: Number of SDRAM refresh too late errors\t\t%d\n",
			ec_api->devname,
			pChipStats->ulSdramRefreshTooLateCount);
	WANEC_API_PRINT("%10s: Number of PLL jitter errors\t\t\t\t%d\n",
			ec_api->devname,
			pChipStats->ulPllJitterErrorCount);
	WANEC_API_PRINT("%10s: Number of HW tone event buffer has overflowed\t%d\n",
			ec_api->devname,
			pChipStats->ulOverflowToneEventsCount);
	WANEC_API_PRINT("%10s: Number of SW tone event buffer has overflowed\t%d\n",
			ec_api->devname,
			pChipStats->ulSoftOverflowToneEventsCount);
	WANEC_API_PRINT("%10s: Number of SW Playout event buffer has overflowed\t%d\n",
			ec_api->devname,
			pChipStats->ulSoftOverflowBufferPlayoutEventsCount);
	WANEC_API_PRINT("\n");
	
	return 0;
}

static int wanec_api_print_full_chip_stats(wan_ec_api_t *ec_api)
{
	tPOCT6100_CHIP_STATS		pChipStats;

	pChipStats = &ec_api->u_chip_stats.f_ChipStats;
	WANEC_API_PRINT("****** Echo Canceller Chip Get Stats %s ******\n",
			ec_api->devname);

	WANEC_API_PRINT("%10s: Number of channels currently open\t\t\t%d\n", 
			ec_api->devname,
			pChipStats->ulNumberChannels);
	WANEC_API_PRINT("%10s: Number of TSI connections currently open\t\t%d\n", 
			ec_api->devname,
			pChipStats->ulNumberTsiCncts);
	WANEC_API_PRINT("%10s: Number of conference bridges currently open\t\t%d\n", 
			ec_api->devname,
			pChipStats->ulNumberConfBridges);
	WANEC_API_PRINT("%10s: Number of playout buffers currently loaded\t\t%d\n", 
			ec_api->devname,
			pChipStats->ulNumberPlayoutBuffers);
	WANEC_API_PRINT("%10s: Number of framing error on H.100 bus\t\t%d\n", 
			ec_api->devname,
			pChipStats->ulH100OutOfSynchCount);
	WANEC_API_PRINT("%10s: Number of errors on H.100 clock CT_C8_A\t\t%d\n", 
			ec_api->devname,
			pChipStats->ulH100ClockABadCount);
	WANEC_API_PRINT("%10s: Number of errors on H.100 frame CT_FRAME_A\t\t%d\n", 
			ec_api->devname,
			pChipStats->ulH100FrameABadCount);
	WANEC_API_PRINT("%10s: Number of errors on H.100 clock CT_C8_B\t\t%d\n", 
			ec_api->devname,
			pChipStats->ulH100ClockBBadCount);
	WANEC_API_PRINT("%10s: Number of internal read timeout errors\t\t%d\n",
			ec_api->devname,
			pChipStats->ulInternalReadTimeoutCount);
	WANEC_API_PRINT("%10s: Number of SDRAM refresh too late errors\t\t%d\n",
			ec_api->devname,
			pChipStats->ulSdramRefreshTooLateCount);
	WANEC_API_PRINT("%10s: Number of PLL jitter errors\t\t\t\t%d\n",
			ec_api->devname,
			pChipStats->ulPllJitterErrorCount);
	WANEC_API_PRINT("%10s: Number of HW tone event buffer has overflowed\t%d\n",
			ec_api->devname,
			pChipStats->ulOverflowToneEventsCount);
	WANEC_API_PRINT("%10s: Number of SW tone event buffer has overflowed\t%d\n",
			ec_api->devname,
			pChipStats->ulSoftOverflowToneEventsCount);
	WANEC_API_PRINT("%10s: Number of SW Playout event buffer has overflowed\t%d\n",
			ec_api->devname,
			pChipStats->ulSoftOverflowBufferPlayoutEventsCount);
	WANEC_API_PRINT("\n");
	
	return 0;
}

static int wanec_api_print_chip_image(wan_ec_api_t *ec_api, int full)
{
	tPOCT6100_CHIP_IMAGE_INFO	pChipImageInfo;
	tPOCT6100_CHIP_TONE_INFO	pChipToneInfo;
	unsigned int			i;

	if (ec_api->u_chip_image.f_ChipImageInfo){
		pChipImageInfo = ec_api->u_chip_image.f_ChipImageInfo;
		WANEC_API_PRINT("****** Echo Canceller Chip Image Info %s ******\n",
					ec_api->devname);
		WANEC_API_PRINT("%10s: Echo Canceller chip image build description:\n%s\n",
					ec_api->devname,
					pChipImageInfo->szVersionNumber);
		WANEC_API_PRINT("%10s: Echo Canceller chip build ID\t\t\t%d\n",
					ec_api->devname,
					pChipImageInfo->ulBuildId);
		WANEC_API_PRINT("%10s: Echo Canceller image type\t\t\t\t%d\n",
					ec_api->devname,
					pChipImageInfo->ulImageType);
		WANEC_API_PRINT("%10s: Maximum number of channels supported by the image\t%d\n",
					ec_api->devname,
					pChipImageInfo->ulMaxChannels);
		if (full) WANEC_API_PRINT("%10s: Support Acoustic echo cancellation\t%s\n",
					ec_api->devname,
					(pChipImageInfo->fAcousticEcho == TRUE) ?
								"TRUE" : "FALSE");
		if (full) WANEC_API_PRINT("%10s: Support configurable tail length for Aec\t%s\n",
					ec_api->devname,
					(pChipImageInfo->fAecTailLength == TRUE) ?
								"TRUE" : "FALSE");
		if (full) WANEC_API_PRINT("%10s: Support configurable default ERL\t%s\n",
					ec_api->devname,
					(pChipImageInfo->fDefaultErl == TRUE) ?
								"TRUE" : "FALSE");
		if (full) WANEC_API_PRINT("%10s: Support configurable non-linearity A\t%s\n",
					ec_api->devname,
					(pChipImageInfo->fNonLinearityBehaviorA == TRUE) ?
								"TRUE" : "FALSE");
		if (full) WANEC_API_PRINT("%10s: Support configurable non-linearity B\t%s\n",
					ec_api->devname,
					(pChipImageInfo->fNonLinearityBehaviorB == TRUE) ?
								"TRUE" : "FALSE");
		if (full) WANEC_API_PRINT("%10s: Tone profile number built in the image\t%d\n",
					ec_api->devname,
					pChipImageInfo->ulToneProfileNumber);
		if (full) WANEC_API_PRINT("%10s: Number of tone available in the image\t%d\n",
					ec_api->devname,
					pChipImageInfo->ulNumTonesAvailable);
		if (full) {
			for(i = 0; i < pChipImageInfo->ulNumTonesAvailable; i++){
				pChipToneInfo = &pChipImageInfo->aToneInfo[i];
				WANEC_API_PRINT("%10s: \tDetection Port: %s\tToneId: 0x%X\n",
					ec_api->devname,
					(pChipToneInfo->ulDetectionPort == cOCT6100_CHANNEL_PORT_SIN)?"SIN":
					(pChipToneInfo->ulDetectionPort == cOCT6100_CHANNEL_PORT_ROUT)?"ROUT":
					(pChipToneInfo->ulDetectionPort == cOCT6100_CHANNEL_PORT_ROUT)?"SOUT":
					(pChipToneInfo->ulDetectionPort == cOCT6100_CHANNEL_PORT_ROUT_SOUT)?"ROUT/SOUT":
												"INV",
					pChipToneInfo->ulToneID);
			}
			WANEC_API_PRINT("\n");
		}
	}
	return 0;
}


static int wanec_api_print_chan_stats(wan_ec_api_t *ec_api, int fe_chan)
{
	tPOCT6100_CHANNEL_STATS		pChannelStats;
	tPOCT6100_CHANNEL_STATS_TDM	pChannelStatsTdm;
	tPOCT6100_CHANNEL_STATS_VQE	pChannelStatsVqe;

	pChannelStats = (tPOCT6100_CHANNEL_STATS)&ec_api->u_chan_stats.f_ChannelStats;
	WANEC_API_PRINT("%10s:%2d: Echo Channel Operation Mode\t\t\t: %s\n",
					ec_api->devname,
					fe_chan,
		(pChannelStats->ulEchoOperationMode==cOCT6100_ECHO_OP_MODE_NORMAL)?
								"NORMAL":
		(pChannelStats->ulEchoOperationMode==cOCT6100_ECHO_OP_MODE_HT_FREEZE)?
								"HT FREEZE":
		(pChannelStats->ulEchoOperationMode==cOCT6100_ECHO_OP_MODE_HT_RESET)?
								"HT RESET":
		(pChannelStats->ulEchoOperationMode==cOCT6100_ECHO_OP_MODE_POWER_DOWN)?
								"POWER DOWN":
		(pChannelStats->ulEchoOperationMode==cOCT6100_ECHO_OP_MODE_NO_ECHO)?
								"NO ECHO":
		(pChannelStats->ulEchoOperationMode==cOCT6100_ECHO_OP_MODE_SPEECH_RECOGNITION)?
								"SPEECH RECOGNITION":
								"Unknown");
	WANEC_API_PRINT("%10s:%2d: Enable Tone Disabler\t\t\t\t: %s\n",
		ec_api->devname, fe_chan,
		(pChannelStats->fEnableToneDisabler==TRUE) ? "TRUE" : "FALSE");
	WANEC_API_PRINT("%10s:%2d: Mute Ports\t\t\t\t\t: %s\n",
					ec_api->devname, fe_chan,
		(pChannelStats->ulMutePortsMask==cOCT6100_CHANNEL_MUTE_PORT_RIN) ?
							"RIN" :
		(pChannelStats->ulMutePortsMask==cOCT6100_CHANNEL_MUTE_PORT_ROUT) ?
							"ROUT" :
		(pChannelStats->ulMutePortsMask==cOCT6100_CHANNEL_MUTE_PORT_SIN) ?
							"SIN" :
		(pChannelStats->ulMutePortsMask==cOCT6100_CHANNEL_MUTE_PORT_SOUT) ?
							"SOUT" :
		(pChannelStats->ulMutePortsMask==cOCT6100_CHANNEL_MUTE_PORT_NONE) ?
							"NONE" : "Unknown");
	WANEC_API_PRINT("%10s:%2d: Enable Extended Tone Detection\t\t\t: %s\n",
		ec_api->devname, fe_chan,
		(pChannelStats->fEnableExtToneDetection==TRUE) ? "TRUE" : "FALSE");

	if (pChannelStats->ulToneDisablerStatus == cOCT6100_INVALID_STAT){
		WANEC_API_PRINT("%10s:%2d: Tone Disabler Status\t\t\t\t: Invalid\n",
					ec_api->devname, fe_chan);
	}else{
		WANEC_API_PRINT("%10s:%2d: Tone Disabler Status\t\t\t\t: %s\n",
			ec_api->devname, fe_chan,
			(pChannelStats->ulToneDisablerStatus==cOCT6100_TONE_DISABLER_EC_DISABLED)?
								"Disabled":"Enabled"); 
	}
	WANEC_API_PRINT("%10s:%2d: Voice activity is detected on SIN port\t\t: %s\n",
			ec_api->devname, fe_chan,
			(pChannelStats->fSinVoiceDetected==TRUE)? "TRUE":
			(pChannelStats->fSinVoiceDetected==FALSE)? "FALSE": "Unknown");
	WANEC_API_PRINT("%10s:%2d: Echo canceller has detected and converged\t: %s\n",
			ec_api->devname, fe_chan,
			(pChannelStats->fEchoCancellerConverged==TRUE)? "TRUE":
			(pChannelStats->fEchoCancellerConverged==FALSE)? "FALSE": "Unknown");
	if (pChannelStats->lRinLevel == cOCT6100_INVALID_SIGNED_STAT){
		WANEC_API_PRINT("%10s:%2d: Average power of signal level on RIN\t\t: Invalid\n",
				ec_api->devname, fe_chan);
	}else{
		WANEC_API_PRINT("%10s:%2d: Average power of signal level on RIN\t\t: %d dBm0\n",
				ec_api->devname, fe_chan,
				pChannelStats->lRinLevel);
	}
	if (pChannelStats->lSinLevel == cOCT6100_INVALID_SIGNED_STAT){
		WANEC_API_PRINT("%10s:%2d: Average power of signal level on SIN\t\t: Invalid\n",
				ec_api->devname, fe_chan);
	}else{
		WANEC_API_PRINT("%10s:%2d: Average power of signal level on SIN\t\t: %d dBm0\n",
				ec_api->devname, fe_chan,
				pChannelStats->lSinLevel);
	}
	if (pChannelStats->lRinAppliedGain == cOCT6100_INVALID_SIGNED_STAT){
		WANEC_API_PRINT("%10s:%2d: Current gain applied to signal level on RIN\t: Invalid\n",
				ec_api->devname, fe_chan);
	}else{
		WANEC_API_PRINT("%10s:%2d: Current gain applied to signal level on RIN\t: %d dB\n",
				ec_api->devname, fe_chan,
				pChannelStats->lRinAppliedGain);
	}
	if (pChannelStats->lSoutAppliedGain == cOCT6100_INVALID_SIGNED_STAT){
		WANEC_API_PRINT("%10s:%2d: Current gain applied to signal level on SOUT\t: Invalid\n",
				ec_api->devname, fe_chan);
	}else{
		WANEC_API_PRINT("%10s:%2d: Current gain applied to signal level on SOUT\t: %d dB\n",
				ec_api->devname, fe_chan,
				pChannelStats->lSoutAppliedGain);
	}
	if (pChannelStats->lComfortNoiseLevel == cOCT6100_INVALID_SIGNED_STAT){
		WANEC_API_PRINT("%10s:%2d: Average power of the comfort noise injected\t: Invalid\n",
				ec_api->devname, fe_chan);
	}else{
		WANEC_API_PRINT("%10s:%2d: Average power of the comfort noise injected\t: %d dBm0\n",
				ec_api->devname, fe_chan,
				pChannelStats->lComfortNoiseLevel);
	}

	pChannelStatsTdm = &pChannelStats->TdmConfig;
	WANEC_API_PRINT("%10s:%2d: (TDM) PCM Law type on SIN\t\t\t: %s\n",
				ec_api->devname, fe_chan,
				(pChannelStatsTdm->ulSinPcmLaw == cOCT6100_PCM_U_LAW) ? "ULAW" : "ALAW");
	WANEC_API_PRINT("%10s:%2d: (TDM) TDM timeslot on SIN port\t\t\t: %d\n",
				ec_api->devname, fe_chan,
				pChannelStatsTdm->ulSinTimeslot);
	WANEC_API_PRINT("%10s:%2d: (TDM) TDM stream on SIN port\t\t\t: %d\n",
				ec_api->devname, fe_chan,
				pChannelStatsTdm->ulSinStream);
	WANEC_API_PRINT("%10s:%2d: (TDM) PCM Law type on RIN\t\t\t: %s\n",
				ec_api->devname, fe_chan,
				(pChannelStatsTdm->ulRinPcmLaw == cOCT6100_PCM_U_LAW) ? "ULAW" : "ALAW");
	WANEC_API_PRINT("%10s:%2d: (TDM) TDM timeslot on RIN port\t\t\t: %d\n",
				ec_api->devname, fe_chan,
				pChannelStatsTdm->ulRinTimeslot);
	WANEC_API_PRINT("%10s:%2d: (TDM) TDM stream on RIN port\t\t\t: %d\n",
				ec_api->devname, fe_chan,
				pChannelStatsTdm->ulRinStream);
	WANEC_API_PRINT("%10s:%2d: (TDM) PCM Law type on SOUT\t\t\t: %s\n",
				ec_api->devname, fe_chan,
				(pChannelStatsTdm->ulSoutPcmLaw == cOCT6100_PCM_U_LAW) ? "ULAW" : "ALAW");
	WANEC_API_PRINT("%10s:%2d: (TDM) TDM timeslot on SOUT port\t\t\t: %d\n",
				ec_api->devname, fe_chan,
				pChannelStatsTdm->ulSoutTimeslot);
	WANEC_API_PRINT("%10s:%2d: (TDM) TDM stream on SOUT port\t\t\t: %d\n",
				ec_api->devname, fe_chan,
				pChannelStatsTdm->ulSoutStream);
	WANEC_API_PRINT("%10s:%2d: (TDM) PCM Law type on ROUT\t\t\t: %s\n",
				ec_api->devname, fe_chan,
				(pChannelStatsTdm->ulRoutPcmLaw == cOCT6100_PCM_U_LAW) ? "ULAW" : "ALAW");
	WANEC_API_PRINT("%10s:%2d: (TDM) TDM timeslot on ROUT port\t\t\t: %d\n",
				ec_api->devname, fe_chan,
				pChannelStatsTdm->ulRoutTimeslot);
	WANEC_API_PRINT("%10s:%2d: (TDM) TDM stream on ROUT port\t\t\t: %d\n",
				ec_api->devname, fe_chan,
				pChannelStatsTdm->ulRoutStream);

	pChannelStatsVqe = &pChannelStats->VqeConfig;
	WANEC_API_PRINT("%10s:%2d: (VQE) NLP status\t\t\t\t\t: %s\n",
				ec_api->devname, fe_chan,
				(pChannelStatsVqe->fEnableNlp == TRUE) ? "TRUE" : "FALSE");
	WANEC_API_PRINT("%10s:%2d: (VQE) Tail Displacement Status\t\t\t: %s\n",
				ec_api->devname, fe_chan,
				(pChannelStatsVqe->fEnableTailDisplacement == TRUE) ? "TRUE" : "FALSE");
	WANEC_API_PRINT("%10s:%2d: (VQE) Tail Displacement (ms)\t\t\t: %d\n",
				ec_api->devname, fe_chan,
				pChannelStatsVqe->ulTailDisplacement);
	WANEC_API_PRINT("%10s:%2d: (VQE) Tail Length (ms)\t\t\t\t: %d\n",
				ec_api->devname, fe_chan,
				pChannelStatsVqe->ulTailLength);
	WANEC_API_PRINT("%10s:%2d: (VQE) Comfort noise mode\t\t\t\t: %s\n",
			ec_api->devname, fe_chan,
			(pChannelStatsVqe->ulComfortNoiseMode == cOCT6100_COMFORT_NOISE_NORMAL) ? "NORMAL" :
			(pChannelStatsVqe->ulComfortNoiseMode == cOCT6100_COMFORT_NOISE_FAST_LATCH) ? "FAST LATCH" :
			(pChannelStatsVqe->ulComfortNoiseMode == cOCT6100_COMFORT_NOISE_EXTENDED) ? "EXTENDED" :
			(pChannelStatsVqe->ulComfortNoiseMode == cOCT6100_COMFORT_NOISE_OFF) ? "OFF" : "UNKNOWN");
	WANEC_API_PRINT("%10s:%2d: (VQE) Acoustic Echo\t\t\t\t: %s\n",
				ec_api->devname, fe_chan,
				(pChannelStatsVqe->fAcousticEcho == TRUE) ? "TRUE" : "FALSE");

	/* Out means: from Driver to FPGA to HWEC
	 * In  means: from HWEC to FPGA to Driver. */
	WANEC_API_PRINT("%10s:%2d: (VQE) Out Automatic Level Control\t\t: %s\n",
				ec_api->devname, fe_chan,
				(pChannelStatsVqe->fRinAutomaticLevelControl == TRUE) ? "TRUE" : "FALSE");
	WANEC_API_PRINT("%10s:%2d: (VQE) Out Automatic Level Control Target\t\t: %d dB\n",
				ec_api->devname, fe_chan,
				pChannelStatsVqe->lRinAutomaticLevelControlTargetDb);
	WANEC_API_PRINT("%10s:%2d: (VQE) In  Automatic Level Control\t\t: %s\n",
				ec_api->devname, fe_chan,
				(pChannelStatsVqe->fSoutAutomaticLevelControl == TRUE) ? "TRUE" : "FALSE");
	WANEC_API_PRINT("%10s:%2d: (VQE) In  Automatic Level Control Target\t\t: %d dB\n",
				ec_api->devname, fe_chan,
				pChannelStatsVqe->lSoutAutomaticLevelControlTargetDb);
	WANEC_API_PRINT("%10s:%2d: (VQE) Noise Reduction\t\t\t\t: %s\n",
				ec_api->devname, fe_chan,
				(pChannelStatsVqe->fSoutAdaptiveNoiseReduction == TRUE) ? "TRUE" : "FALSE");
	WANEC_API_PRINT("%10s:%2d: (VQE) Tone Removal\t\t\t\t: %s\n",
				ec_api->devname, fe_chan,
				(pChannelStatsVqe->fDtmfToneRemoval == TRUE) ? "TRUE" : "FALSE");
	WANEC_API_PRINT("%10s:%2d: (VQE) Activation Delay (ms)\t\t\t: %d\n",
				ec_api->devname, fe_chan,
				pChannelStatsVqe->ulToneDisablerVqeActivationDelay);


	WANEC_API_PRINT("\n");

	return 0;
}

static int wanec_api_print_full_chan_stats(wan_ec_api_t *ec_api, int fe_chan)
{
	tPOCT6100_CHANNEL_STATS		pChannelStats;
	tPOCT6100_CHANNEL_STATS_TDM	pChannelStatsTdm;
	tPOCT6100_CHANNEL_STATS_VQE	pChannelStatsVqe;
	tPOCT6100_CHANNEL_STATS_CODEC	pChannelStatsCodec;

	pChannelStats = (tPOCT6100_CHANNEL_STATS)&ec_api->u_chan_stats.f_ChannelStats;
	WANEC_API_PRINT("%10s:%2d: Echo Channel Operation Mode\t\t\t: %s\n",
					ec_api->devname,
					fe_chan,
		(pChannelStats->ulEchoOperationMode==cOCT6100_ECHO_OP_MODE_NORMAL)?
								"NORMAL":
		(pChannelStats->ulEchoOperationMode==cOCT6100_ECHO_OP_MODE_HT_FREEZE)?
								"HT FREEZE":
		(pChannelStats->ulEchoOperationMode==cOCT6100_ECHO_OP_MODE_HT_RESET)?
								"HT RESET":
		(pChannelStats->ulEchoOperationMode==cOCT6100_ECHO_OP_MODE_POWER_DOWN)?
								"POWER DOWN":
		(pChannelStats->ulEchoOperationMode==cOCT6100_ECHO_OP_MODE_NO_ECHO)?
								"NO ECHO":
		(pChannelStats->ulEchoOperationMode==cOCT6100_ECHO_OP_MODE_SPEECH_RECOGNITION)?
								"SPEECH RECOGNITION":
								"Unknown");
	WANEC_API_PRINT("%10s:%2d: Enable Tone Disabler\t\t\t\t\t: %s\n",
		ec_api->devname, fe_chan,
		(pChannelStats->fEnableToneDisabler==TRUE) ? "TRUE" : "FALSE");
	WANEC_API_PRINT("%10s:%2d: Mute Ports\t\t\t\t\t: %s\n",
					ec_api->devname, fe_chan,
		(pChannelStats->ulMutePortsMask==cOCT6100_CHANNEL_MUTE_PORT_RIN) ?
							"RIN" :
		(pChannelStats->ulMutePortsMask==cOCT6100_CHANNEL_MUTE_PORT_ROUT) ?
							"ROUT" :
		(pChannelStats->ulMutePortsMask==cOCT6100_CHANNEL_MUTE_PORT_SIN) ?
							"SIN" :
		(pChannelStats->ulMutePortsMask==cOCT6100_CHANNEL_MUTE_PORT_SOUT) ?
							"SOUT" :
		(pChannelStats->ulMutePortsMask==cOCT6100_CHANNEL_MUTE_PORT_NONE) ?
							"NONE" : "Unknown");
	WANEC_API_PRINT("%10s:%2d: Enable Extended Tone Detection\t\t\t\t: %s\n",
		ec_api->devname, fe_chan,
		(pChannelStats->fEnableExtToneDetection==TRUE) ? "TRUE" : "FALSE");
	if (pChannelStats->lCurrentERL == cOCT6100_INVALID_SIGNED_STAT){
		WANEC_API_PRINT("%10s:%2d: Current Echo Return Loss\t\t\t\t: Invalid\n",
					ec_api->devname, fe_chan);
	}else{
		WANEC_API_PRINT("%10s:%2d: Current Echo Return Loss\t\t\t\t: %d dB\n",
					ec_api->devname, fe_chan,
					pChannelStats->lCurrentERL);
	}
	if (pChannelStats->lCurrentERLE == cOCT6100_INVALID_SIGNED_STAT){
		WANEC_API_PRINT("%10s:%2d: Current Echo Return Loss Enhancement\t\t: Invalid\n",
					ec_api->devname, fe_chan);
	}else{
		WANEC_API_PRINT("%10s:%2d: Current Echo Return Loss Enhancement\t\t: %d dB\n",
					ec_api->devname, fe_chan,
					pChannelStats->lCurrentERLE);
	}
	if (pChannelStats->lMaxERL == cOCT6100_INVALID_SIGNED_STAT){
		WANEC_API_PRINT("%10s:%2d: Maximum value of the ERL\t\t\t\t: Invalid\n",
					ec_api->devname, fe_chan);
	}else{
		WANEC_API_PRINT("%10s:%2d: Maximum value of the ERL\t\t\t\t: %d dB\n",
					ec_api->devname, fe_chan,
					pChannelStats->lMaxERL);
	}
	if (pChannelStats->lMaxERLE == cOCT6100_INVALID_SIGNED_STAT){
		WANEC_API_PRINT("%10s:%2d: Maximum value of the ERLE\t\t\t: Invalid\n",
					ec_api->devname, fe_chan);
	}else{
		WANEC_API_PRINT("%10s:%2d: Maximum value of the ERLE\t\t\t: %d dB\n",
					ec_api->devname, fe_chan,
					pChannelStats->lMaxERLE);
	}
	if (pChannelStats->ulNumEchoPathChanges == cOCT6100_INVALID_STAT){
		WANEC_API_PRINT("%10s:%2d: Number of Echo Path changes\t\t\t: Invalid\n",
					ec_api->devname, fe_chan);
	}else{
		WANEC_API_PRINT("%10s:%2d: Number of Echo Path changes\t\t\t: %d\n",
					ec_api->devname, fe_chan,
					pChannelStats->ulNumEchoPathChanges);
	}
	if (pChannelStats->ulCurrentEchoDelay == cOCT6100_INVALID_STAT){
		WANEC_API_PRINT("%10s:%2d: Current Echo Delay\t\t\t\t: Invalid\n",
					ec_api->devname, fe_chan);
	}else{
		WANEC_API_PRINT("%10s:%2d: Current Echo Delay\t\t\t\t: %d\n",
					ec_api->devname, fe_chan,
					pChannelStats->ulCurrentEchoDelay);
	}
	if (pChannelStats->ulMaxEchoDelay == cOCT6100_INVALID_STAT){
		WANEC_API_PRINT("%10s:%2d: Maximum Echo Delay\t\t\t\t: Invalid\n",
					ec_api->devname, fe_chan);
	}else{
		WANEC_API_PRINT("%10s:%2d: Maximum Echo Delay\t\t\t\t: %d\n",
					ec_api->devname, fe_chan,
					pChannelStats->ulMaxEchoDelay);
	}
	if (pChannelStats->ulToneDisablerStatus == cOCT6100_INVALID_STAT){
		WANEC_API_PRINT("%10s:%2d: Tone Disabler Status\t\t\t\t: Invalid\n",
					ec_api->devname, fe_chan);
	}else{
		WANEC_API_PRINT("%10s:%2d: Tone Disabler Status\t\t\t\t: %s\n",
					ec_api->devname, fe_chan,
			(pChannelStats->ulToneDisablerStatus==cOCT6100_TONE_DISABLER_EC_DISABLED)?
								"Disabled":"Enabled"); 
	}
	WANEC_API_PRINT("%10s:%2d: Voice activity is detected on SIN port\t\t: %s\n",
					ec_api->devname, fe_chan,
		(pChannelStats->fSinVoiceDetected==TRUE)? "TRUE":
		(pChannelStats->fSinVoiceDetected==FALSE)? "FALSE": "Unknown");
	WANEC_API_PRINT("%10s:%2d: Echo canceller has detected and converged\t: %s\n",
					ec_api->devname, fe_chan,
		(pChannelStats->fEchoCancellerConverged==TRUE)? "TRUE":
		(pChannelStats->fEchoCancellerConverged==FALSE)? "FALSE": "Unknown");
	if (pChannelStats->lRinLevel == cOCT6100_INVALID_SIGNED_STAT){
		WANEC_API_PRINT("%10s:%2d: Average power of signal level on RIN\t\t: Invalid\n",
					ec_api->devname, fe_chan);
	}else{
		WANEC_API_PRINT("%10s:%2d: Average power of signal level on RIN\t\t: %d dBm0\n",
					ec_api->devname, fe_chan,
					pChannelStats->lRinLevel);
	}
	if (pChannelStats->lSinLevel == cOCT6100_INVALID_SIGNED_STAT){
		WANEC_API_PRINT("%10s:%2d: Average power of signal level on SIN\t\t: Invalid\n",
					ec_api->devname, fe_chan);
	}else{
		WANEC_API_PRINT("%10s:%2d: Average power of signal level on SIN\t\t: %d dBm0\n",
					ec_api->devname, fe_chan,
					pChannelStats->lSinLevel);
	}
	if (pChannelStats->lRinAppliedGain == cOCT6100_INVALID_SIGNED_STAT){
		WANEC_API_PRINT("%10s:%2d: Current gain applied to signal level on RIN\t: Invalid\n",
					ec_api->devname, fe_chan);
	}else{
		WANEC_API_PRINT("%10s:%2d: Current gain applied to signal level on RIN\t: %d dB\n",
					ec_api->devname, fe_chan,
					pChannelStats->lRinAppliedGain);
	}
	if (pChannelStats->lSoutAppliedGain == cOCT6100_INVALID_SIGNED_STAT){
		WANEC_API_PRINT("%10s:%2d: Current gain applied to signal level on SOUT\t: Invalid\n",
					ec_api->devname, fe_chan);
	}else{
		WANEC_API_PRINT("%10s:%2d: Current gain applied to signal level on SOUT\t: %d dB\n",
					ec_api->devname, fe_chan,
					pChannelStats->lSoutAppliedGain);
	}
	if (pChannelStats->lComfortNoiseLevel == cOCT6100_INVALID_SIGNED_STAT){
		WANEC_API_PRINT("%10s:%2d: Average power of the comfort noise injected\t: Invalid\n",
					ec_api->devname, fe_chan);
	}else{
		WANEC_API_PRINT("%10s:%2d: Average power of the comfort noise injected\t: %d dBm0\n",
					ec_api->devname, fe_chan,
					pChannelStats->lComfortNoiseLevel);
	}

	pChannelStatsTdm = &pChannelStats->TdmConfig;
	WANEC_API_PRINT("%10s:%2d: (TDM) PCM Law type on SIN\t\t\t: %s\n",
				ec_api->devname, fe_chan,
				(pChannelStatsTdm->ulSinPcmLaw == cOCT6100_PCM_U_LAW) ? "ULAW" : "ALAW");
	WANEC_API_PRINT("%10s:%2d: (TDM) TDM timeslot on SIN port\t\t\t: %d\n",
				ec_api->devname, fe_chan,
				pChannelStatsTdm->ulSinTimeslot);
	WANEC_API_PRINT("%10s:%2d: (TDM) TDM stream on SIN port\t\t\t: %d\n",
				ec_api->devname, fe_chan,
				pChannelStatsTdm->ulSinStream);
	WANEC_API_PRINT("%10s:%2d: (TDM) PCM Law type on RIN\t\t\t: %s\n",
				ec_api->devname, fe_chan,
				(pChannelStatsTdm->ulRinPcmLaw == cOCT6100_PCM_U_LAW) ? "ULAW" : "ALAW");
	WANEC_API_PRINT("%10s:%2d: (TDM) TDM timeslot on RIN port\t\t\t: %d\n",
				ec_api->devname, fe_chan,
				pChannelStatsTdm->ulRinTimeslot);
	WANEC_API_PRINT("%10s:%2d: (TDM) TDM stream on RIN port\t\t\t: %d\n",
				ec_api->devname, fe_chan,
				pChannelStatsTdm->ulRinStream);
	WANEC_API_PRINT("%10s:%2d: (TDM) PCM Law type on SOUT\t\t\t: %s\n",
				ec_api->devname, fe_chan,
				(pChannelStatsTdm->ulSoutPcmLaw == cOCT6100_PCM_U_LAW) ? "ULAW" : "ALAW");
	WANEC_API_PRINT("%10s:%2d: (TDM) TDM timeslot on SOUT port\t\t\t: %d\n",
				ec_api->devname, fe_chan,
				pChannelStatsTdm->ulSoutTimeslot);
	WANEC_API_PRINT("%10s:%2d: (TDM) TDM stream on SOUT port\t\t\t: %d\n",
				ec_api->devname, fe_chan,
				pChannelStatsTdm->ulSoutStream);
	WANEC_API_PRINT("%10s:%2d: (TDM) PCM Law type on ROUT\t\t\t: %s\n",
				ec_api->devname, fe_chan,
				(pChannelStatsTdm->ulRoutPcmLaw == cOCT6100_PCM_U_LAW) ? "ULAW" : "ALAW");
	WANEC_API_PRINT("%10s:%2d: (TDM) TDM timeslot on ROUT port\t\t\t: %d\n",
				ec_api->devname, fe_chan,
				pChannelStatsTdm->ulRoutTimeslot);
	WANEC_API_PRINT("%10s:%2d: (TDM) TDM stream on ROUT port\t\t\t: %d\n",
				ec_api->devname, fe_chan,
				pChannelStatsTdm->ulRoutStream);

	pChannelStatsVqe = &pChannelStats->VqeConfig;
	WANEC_API_PRINT("%10s:%2d: (VQE) NLP status\t\t\t\t\t: %s\n",
				ec_api->devname, fe_chan,
				(pChannelStatsVqe->fEnableNlp == TRUE) ? "TRUE" : "FALSE");
	WANEC_API_PRINT("%10s:%2d: (VQE) Enable Tail Displacement\t\t\t: %s\n",
				ec_api->devname, fe_chan,
				(pChannelStatsVqe->fEnableTailDisplacement == TRUE) ? "TRUE" : "FALSE");
	WANEC_API_PRINT("%10s:%2d: (VQE) Offset of the Echo Cancellation window (ms)\t: %d\n",
				ec_api->devname, fe_chan,
				pChannelStatsVqe->ulTailDisplacement);
	WANEC_API_PRINT("%10s:%2d: (VQE) Maximum tail length\t\t\t: %d ms\n",
				ec_api->devname, fe_chan,
				pChannelStatsVqe->ulTailLength);
	WANEC_API_PRINT("%10s:%2d: (VQE) Rin Level control mode\t\t\t: %s\n",
				ec_api->devname, fe_chan,
				(pChannelStatsVqe->fRinLevelControl == TRUE) ? "Enable" : "TRUE");
	WANEC_API_PRINT("%10s:%2d: (VQE) Rin Control Signal gain\t\t\t: %d dB\n",
				ec_api->devname, fe_chan,
				pChannelStatsVqe->lRinLevelControlGainDb);
	WANEC_API_PRINT("%10s:%2d: (VQE) Sout Level control mode\t\t\t: %s\n",
				ec_api->devname, fe_chan,
				(pChannelStatsVqe->fSoutLevelControl == TRUE) ? "Enable" : "TRUE");
	WANEC_API_PRINT("%10s:%2d: (VQE) Sout Control Signal gain\t\t\t: %d dB\n",
				ec_api->devname, fe_chan,
				pChannelStatsVqe->lSoutLevelControlGainDb);
	WANEC_API_PRINT("%10s:%2d: (VQE) RIN Automatic Level Control\t\t: %s\n",
				ec_api->devname, fe_chan,
				(pChannelStatsVqe->fRinAutomaticLevelControl == TRUE) ? "TRUE" : "FALSE");
	WANEC_API_PRINT("%10s:%2d: (VQE) RIN Target Level Control\t\t\t: %d dBm0\n",
				ec_api->devname, fe_chan,
				pChannelStatsVqe->lRinAutomaticLevelControlTargetDb);
	WANEC_API_PRINT("%10s:%2d: (VQE) SOUT Automatic Level Control\t\t: %s\n",
				ec_api->devname, fe_chan,
				(pChannelStatsVqe->fSoutAutomaticLevelControl == TRUE) ? "TRUE" : "FALSE");
	WANEC_API_PRINT("%10s:%2d: (VQE) SOUT Target Level Control\t\t\t: %d dBm0\n",
				ec_api->devname, fe_chan,
				pChannelStatsVqe->lSoutAutomaticLevelControlTargetDb);
	WANEC_API_PRINT("%10s:%2d: (VQE) Comfort noise mode\t\t\t\t: %s\n",
			ec_api->devname, fe_chan,
			(pChannelStatsVqe->ulComfortNoiseMode == cOCT6100_COMFORT_NOISE_NORMAL) ? "NORMAL" :
			(pChannelStatsVqe->ulComfortNoiseMode == cOCT6100_COMFORT_NOISE_FAST_LATCH) ? "FAST LATCH" :
			(pChannelStatsVqe->ulComfortNoiseMode == cOCT6100_COMFORT_NOISE_EXTENDED) ? "EXTENDED" :
			(pChannelStatsVqe->ulComfortNoiseMode == cOCT6100_COMFORT_NOISE_OFF) ? "OFF" : "UNKNOWN");
	WANEC_API_PRINT("%10s:%2d: (VQE) Remove any DTMF tone detection on SIN port\t: %s\n",
			ec_api->devname, fe_chan,
			(pChannelStatsVqe->fDtmfToneRemoval == TRUE) ? "TRUE" : "FALSE");
	WANEC_API_PRINT("%10s:%2d: (VQE) Acoustic Echo\t\t\t\t: %s\n",
			ec_api->devname, fe_chan,
			(pChannelStatsVqe->fAcousticEcho == TRUE) ? "TRUE" : "FALSE");

	WANEC_API_PRINT("%10s:%2d: (VQE) Non Linearity Behavior A\t\t\t: %d\n",
			ec_api->devname, fe_chan,
			pChannelStatsVqe->ulNonLinearityBehaviorA);
	WANEC_API_PRINT("%10s:%2d: (VQE) Non Linearity Behavior B\t\t\t: %d\n",
			ec_api->devname, fe_chan,
			pChannelStatsVqe->ulNonLinearityBehaviorB);
	WANEC_API_PRINT("%10s:%2d: (VQE) Double Talk algorithm\t\t\t: %s\n",
			ec_api->devname, fe_chan,
			(pChannelStatsVqe->ulDoubleTalkBehavior==cOCT6100_DOUBLE_TALK_BEH_NORMAL)?"NORMAL":
									"LESS AGGRESSIVE");
	WANEC_API_PRINT("%10s:%2d: (VQE) Default ERL (not converged)\t\t: %d dB\n",
			ec_api->devname, fe_chan,
			pChannelStatsVqe->lDefaultErlDb);
	WANEC_API_PRINT("%10s:%2d: (VQE) Acoustic Echo Cancellation default ERL\t: %d dB\n",
			ec_api->devname, fe_chan,
			pChannelStatsVqe->lAecDefaultErlDb);
	WANEC_API_PRINT("%10s:%2d: (VQE) Maximum Acoustic Echo tail length\t\t: %d ms\n",
			ec_api->devname, fe_chan,
			pChannelStatsVqe->ulAecTailLength);
	WANEC_API_PRINT("%10s:%2d: (VQE) Attenuation Level applied to the noise signal\t: %d dB\n",
			ec_api->devname, fe_chan,
			pChannelStatsVqe->lAnrSnrEnhancementDb);
	WANEC_API_PRINT("%10s:%2d: (VQE) Silence period before the re-activation of VQE features\t: %d ms\n",
			ec_api->devname, fe_chan,
			pChannelStatsVqe->ulToneDisablerVqeActivationDelay);

	pChannelStatsCodec = &pChannelStats->CodecConfig;
	WANEC_API_PRINT("%10s:%2d: (CODEC) Encoder channel port\t\t\t: %s\n",
			ec_api->devname, fe_chan,
			(pChannelStatsCodec->ulEncoderPort == cOCT6100_CHANNEL_PORT_ROUT) ? "ROUT":
			(pChannelStatsCodec->ulEncoderPort == cOCT6100_CHANNEL_PORT_SOUT) ? "SOUT":"NO ENCODING");
	WANEC_API_PRINT("%10s:%2d: (CODEC) Encoder rate\t\t\t\t: %s\n",
			ec_api->devname, fe_chan,
			(pChannelStatsCodec->ulEncodingRate == cOCT6100_G711_64KBPS) ? "G.711 64 kBps":
			(pChannelStatsCodec->ulEncodingRate == cOCT6100_G726_40KBPS) ? "G.726 40 kBps":
			(pChannelStatsCodec->ulEncodingRate == cOCT6100_G726_32KBPS) ? "G.726 32 kBps":
			(pChannelStatsCodec->ulEncodingRate == cOCT6100_G726_24KBPS) ? "G.726 24 kBps":
			(pChannelStatsCodec->ulEncodingRate == cOCT6100_G726_16KBPS) ? "G.726 16 kBps":
			(pChannelStatsCodec->ulEncodingRate == cOCT6100_G727_40KBPS_4_1) ? "G.727 40 kBps (4:1)":
			(pChannelStatsCodec->ulEncodingRate == cOCT6100_G727_40KBPS_3_2) ? "G.727 40 kBps (3:2)":
			(pChannelStatsCodec->ulEncodingRate == cOCT6100_G727_40KBPS_2_3) ? "G.727 40 kBps (2:3)":
			(pChannelStatsCodec->ulEncodingRate == cOCT6100_G727_32KBPS_4_0) ? "G.727 32 kBps (4:0)":
			(pChannelStatsCodec->ulEncodingRate == cOCT6100_G727_32KBPS_3_1) ? "G.727 32 kBps (3:1)":
			(pChannelStatsCodec->ulEncodingRate == cOCT6100_G727_32KBPS_2_2) ? "G.727 32 kBps (2:2)":
			(pChannelStatsCodec->ulEncodingRate == cOCT6100_G727_24KBPS_3_0) ? "G.727 24 kBps (3:0)":
			(pChannelStatsCodec->ulEncodingRate == cOCT6100_G727_24KBPS_2_1) ? "G.727 24 kBps (2:1)":
			(pChannelStatsCodec->ulEncodingRate == cOCT6100_G727_16KBPS_2_0) ? "G.727 16 kBps (2:0)":
											"UNKNOWN");
	WANEC_API_PRINT("%10s:%2d: (CODEC) Decoder channel port\t\t\t: %s\n",
			ec_api->devname, fe_chan,
			(pChannelStatsCodec->ulDecoderPort == cOCT6100_CHANNEL_PORT_RIN) ? "RIN":
			(pChannelStatsCodec->ulDecoderPort == cOCT6100_CHANNEL_PORT_SIN) ? "SIN":"NO DECODING");
	WANEC_API_PRINT("%10s:%2d: (CODEC) Decoder rate\t\t\t\t: %s\n",
			ec_api->devname, fe_chan,
			(pChannelStatsCodec->ulDecodingRate == cOCT6100_G711_64KBPS) ? "G.711 64 kBps":
			(pChannelStatsCodec->ulDecodingRate == cOCT6100_G726_40KBPS) ? "G.726 40 kBps":
			(pChannelStatsCodec->ulDecodingRate == cOCT6100_G726_32KBPS) ? "G.726 32 kBps":
			(pChannelStatsCodec->ulDecodingRate == cOCT6100_G726_24KBPS) ? "G.726 24 kBps":
			(pChannelStatsCodec->ulDecodingRate == cOCT6100_G726_16KBPS) ? "G.726 16 kBps":
			(pChannelStatsCodec->ulDecodingRate == cOCT6100_G727_2C_ENCODED) ? "G.727 2C Encoded":
			(pChannelStatsCodec->ulDecodingRate == cOCT6100_G727_3C_ENCODED) ? "G.727 3C Encoded":
			(pChannelStatsCodec->ulDecodingRate == cOCT6100_G727_4C_ENCODED) ? "G.727 4C Encoded":
			(pChannelStatsCodec->ulDecodingRate == cOCT6100_G726_ENCODED) ? "G.726 Encoded":
			(pChannelStatsCodec->ulDecodingRate == cOCT6100_G711_G727_2C_ENCODED) ? "G.727 2C Encoded":
			(pChannelStatsCodec->ulDecodingRate == cOCT6100_G711_G727_3C_ENCODED) ? "G.727 3C Encoded":
			(pChannelStatsCodec->ulDecodingRate == cOCT6100_G711_G727_4C_ENCODED) ? "G.727 4C Encoded":
											"UNKNOWN");
	WANEC_API_PRINT("\n");
	return 0;
}

static int wanec_api_verbose(int verbose)
{
	int	ec_driver_verbose;
		
	ec_driver_verbose = (verbose == 0) ? WAN_EC_VERBOSE_NONE :
			  (verbose == 1) ? WAN_EC_VERBOSE_EXTRA1 :
						(WAN_EC_VERBOSE_EXTRA1|WAN_EC_VERBOSE_EXTRA2);

	return ec_driver_verbose;
}

/*!
  \fn void _WANEC_API_CALL wanec_api_set_lib_verbosity(int verbose)

  \brief Set Verbosity level of the Library.
		The level controls amount of data printed to stdout
		for diagnostic purposes.

  \param verbosity_level 0 - do NOT print to stdout, else - print to stdout.

  \return None
*/
void _WANEC_API_CALL wanec_api_set_lib_verbosity(int verbosity_level)
{
	/* user can control output on per-function basis */
	ec_library_verbose = verbosity_level;
}

int _WANEC_API_CALL wanec_api_init(void)
{
	/* No op (a global wan_ec_api_t used to be initialized here, which is of course non-thread-safe) */
	return 0;
}

int _WANEC_API_CALL wanec_api_config(	char			*devname,
			int			verbose,
			wanec_api_config_t	*conf)
{
	wan_ec_api_t		ec_api;
	memset(&ec_api, 0, sizeof(ec_api));

	memcpy(ec_api.devname, devname, strlen(devname));
	ec_api.verbose	= wanec_api_verbose(verbose);
	ec_api.cmd	= WAN_EC_API_CMD_CONFIG;
	if (conf && conf->conf.param_no){
		memcpy(	&ec_api.custom_conf,
			&conf->conf,
			sizeof(wan_custom_conf_t));
	}
	return wanec_api_lib_cmd(&ec_api);	//return wanec_api_lib_config(&ec_api, 1);
}

int _WANEC_API_CALL wanec_api_release(	char			*devname,
			int			verbose,
			wanec_api_release_t	*release)
{
	wan_ec_api_t		ec_api;
	memset(&ec_api, 0, sizeof(ec_api));

	memcpy(ec_api.devname, devname, strlen(devname));
	ec_api.verbose	= wanec_api_verbose(verbose);
	ec_api.cmd	= WAN_EC_API_CMD_RELEASE;
	return wanec_api_lib_cmd(&ec_api);
}

int _WANEC_API_CALL wanec_api_mode(	char			*devname,
			int			verbose,
			wanec_api_mode_t	*mode)
{
	wan_ec_api_t		ec_api;
	memset(&ec_api, 0, sizeof(ec_api));
	memcpy(ec_api.devname, devname, strlen(devname));
	ec_api.verbose		= wanec_api_verbose(verbose);
	ec_api.cmd		= (mode->enable) ? 
					WAN_EC_API_CMD_ENABLE :
					WAN_EC_API_CMD_DISABLE;
	ec_api.fe_chan_map	= mode->fe_chan_map;
	return wanec_api_lib_cmd(&ec_api);
}

int _WANEC_API_CALL wanec_api_bypass(	char			*devname,
			int			verbose,
			wanec_api_bypass_t	*bypass)
{
	wan_ec_api_t		ec_api;
	memset(&ec_api, 0, sizeof(ec_api));
	memcpy(ec_api.devname, devname, strlen(devname));
	ec_api.verbose	= wanec_api_verbose(verbose);
	ec_api.cmd		= (bypass->enable) ?
					WAN_EC_API_CMD_BYPASS_ENABLE : 
					WAN_EC_API_CMD_BYPASS_DISABLE;
	ec_api.fe_chan_map	= bypass->fe_chan_map;
	return wanec_api_lib_cmd(&ec_api);
}

int _WANEC_API_CALL wanec_api_opmode(	char			*devname,
			int			verbose,
			wanec_api_opmode_t	*opmode)
{
	wan_ec_api_t		ec_api;
	memset(&ec_api, 0, sizeof(ec_api));
		
	memcpy(ec_api.devname, devname, strlen(devname));
	ec_api.verbose		= wanec_api_verbose(verbose);
	ec_api.cmd		= WAN_EC_API_CMD_OPMODE;
	ec_api.fe_chan_map	= opmode->fe_chan_map;
	switch(opmode->mode){
	case WANEC_API_OPMODE_NORMAL:
		ec_api.u_chan_opmode.opmode = cOCT6100_ECHO_OP_MODE_NORMAL;
		break;
	case WANEC_API_OPMODE_HT_FREEZE:
		ec_api.u_chan_opmode.opmode = cOCT6100_ECHO_OP_MODE_HT_FREEZE;
		break;
	case WANEC_API_OPMODE_HT_RESET:
		ec_api.u_chan_opmode.opmode = cOCT6100_ECHO_OP_MODE_HT_RESET;
		break;
	case WANEC_API_OPMODE_POWER_DOWN:
		ec_api.u_chan_opmode.opmode = cOCT6100_ECHO_OP_MODE_POWER_DOWN;
		break;
	case WANEC_API_OPMODE_NO_ECHO:
		ec_api.u_chan_opmode.opmode = cOCT6100_ECHO_OP_MODE_NO_ECHO;
		break;
	case WANEC_API_OPMODE_SPEECH_RECOGNITION:
		ec_api.u_chan_opmode.opmode = cOCT6100_ECHO_OP_MODE_SPEECH_RECOGNITION;
		break;
	default:
		return -EINVAL;
	}
	return wanec_api_lib_cmd(&ec_api);
}

int _WANEC_API_CALL wanec_api_modify(	char			*devname,
			int			verbose,
			wanec_api_modify_t	*modify)
{
	wan_ec_api_t		ec_api;
	memset(&ec_api, 0, sizeof(ec_api));
	memcpy(ec_api.devname, devname, strlen(devname));
	ec_api.verbose	= wanec_api_verbose(verbose);
	ec_api.cmd = WAN_EC_API_CMD_MODIFY_CHANNEL;
	ec_api.fe_chan_map	= modify->fe_chan_map;
	if (modify->conf.param_no){
		memcpy(	&ec_api.custom_conf,
			&modify->conf,
			sizeof(wan_custom_conf_t));
	}
	return wanec_api_lib_cmd(&ec_api);
}

int _WANEC_API_CALL wanec_api_mute(	char			*devname,
			int			verbose,
			wanec_api_mute_t	*mute)
{
	wan_ec_api_t		ec_api;
	memset(&ec_api, 0, sizeof(ec_api));
	memcpy(ec_api.devname, devname, strlen(devname));
	ec_api.verbose	= wanec_api_verbose(verbose);
	switch(mute->mode){
	case 1:
		ec_api.cmd = WAN_EC_API_CMD_CHANNEL_MUTE;
		break;
	case 2:
		ec_api.cmd = WAN_EC_API_CMD_CHANNEL_UNMUTE;
		break;
	default:
		return -EINVAL;
	}
	ec_api.fe_chan_map		= mute->fe_chan_map;
	ec_api.u_chan_mute.port_map	= mute->port_map;
	return wanec_api_lib_cmd(&ec_api);
}

int _WANEC_API_CALL wanec_api_tone(	char			*devname,
			int			verbose,
			wanec_api_tone_t	*tone)
{
	wan_ec_api_t		ec_api;
	memset(&ec_api, 0, sizeof(ec_api));
	memcpy(ec_api.devname, devname, strlen(devname));
	ec_api.verbose			= wanec_api_verbose(verbose);
	ec_api.cmd			= (tone->enable) ?
						WAN_EC_API_CMD_TONE_ENABLE : 
						WAN_EC_API_CMD_TONE_DISABLE;
	ec_api.fe_chan_map		= tone->fe_chan_map;
	ec_api.u_tone_config.id		= (unsigned char)tone->id;
	ec_api.u_tone_config.port_map	= tone->port_map;
	ec_api.u_tone_config.type	= tone->type_map;
	return wanec_api_lib_cmd(&ec_api);
}

int _WANEC_API_CALL wanec_api_stats(	char			*devname,
			int			verbose,
			wanec_api_stats_t	*stats)
{
	int	err;
	wan_ec_api_t		ec_api;
	memset(&ec_api, 0, sizeof(ec_api));
	
	memcpy(ec_api.devname, devname, strlen(devname));
	ec_api.verbose	= wanec_api_verbose(verbose);
	ec_api.cmd	= (stats->full) ?
				WAN_EC_API_CMD_STATS_FULL : 
				WAN_EC_API_CMD_STATS;
	if (stats->fe_chan){
		ec_api.fe_chan_map		= (1 << stats->fe_chan);
		ec_api.u_chan_stats.reset 	= stats->reset;
	}else{
		ec_api.fe_chan_map		= 0;
		ec_api.u_chip_stats.reset 	= stats->reset;
#if 0
		if (full){
			ec_api.u_chip_stats.f_ChipImageInfo =
				malloc(sizeof(tOCT6100_CHIP_IMAGE_INFO));
			memset(ec_api.u_chip_stats.f_ChipImageInfo,
					0, sizeof(tOCT6100_CHIP_IMAGE_INFO));
		}
#endif		
	}
	err = wanec_api_lib_cmd(&ec_api);
	if (err) return err;
	if (ec_api.err) return 0;

	if (!stats->silent){
		if (stats->fe_chan){
			if (ec_api.cmd == WAN_EC_API_CMD_STATS){
				err = wanec_api_print_chan_stats(
						&ec_api, stats->fe_chan);
			}else{
				err = wanec_api_print_full_chan_stats(
						&ec_api, stats->fe_chan);
			}
		}else{
			if (ec_api.cmd == WAN_EC_API_CMD_STATS){
				err = wanec_api_print_chip_stats(&ec_api);
			}else{
				err = wanec_api_print_full_chip_stats(&ec_api);
			}
		}
	}
	return err;

}

int _WANEC_API_CALL wanec_api_hwimage(	char			*devname,
			int			verbose,
			wanec_api_image_t	*image)
{
	int	err;
	wan_ec_api_t		ec_api;
	memset(&ec_api, 0, sizeof(ec_api));
	
	memcpy(ec_api.devname, devname, strlen(devname));
	ec_api.verbose	= wanec_api_verbose(verbose);
	ec_api.cmd	= WAN_EC_API_CMD_STATS_IMAGE;
#if 0
	ec_api.u_chip_image.f_ChipImageInfo =
				malloc(sizeof(tOCT6100_CHIP_IMAGE_INFO));
	memset(ec_api.u_chip_image.f_ChipImageInfo,
				0, sizeof(tOCT6100_CHIP_IMAGE_INFO));
#endif
	err = wanec_api_lib_cmd(&ec_api);
	if (err) return err;
	if (ec_api.err) return 0;

	wanec_api_print_chip_image(&ec_api, 0);
	return err;

}

int _WANEC_API_CALL wanec_api_buffer_load(char			*devname,
			int			verbose,
			wanec_api_bufferload_t	*bufferload)
{
	int	err;
	wan_ec_api_t		ec_api;
	memset(&ec_api, 0, sizeof(ec_api));

	memcpy(ec_api.devname, devname, strlen(devname));
	ec_api.verbose	= wanec_api_verbose(verbose);
	ec_api.cmd	= WAN_EC_API_CMD_BUFFER_LOAD;

	memcpy(ec_api.u_buffer_config.buffer, bufferload->buffer, strlen(bufferload->buffer));
	ec_api.u_buffer_config.pcmlaw = (UINT32)bufferload->pcmlaw;	

	err = wanec_api_lib_bufferload(&ec_api);
	if (err) return err;
	if (ec_api.err) return -EINVAL;
	bufferload->buffer_id = ec_api.u_buffer_config.buffer_index;
	return 0;
}

int _WANEC_API_CALL wanec_api_buffer_unload(	char				*devname,
				int				verbose,
				wanec_api_bufferunload_t	*bufferunload)
{
	int	err;
	wan_ec_api_t		ec_api;
	memset(&ec_api, 0, sizeof(ec_api));

	memcpy(ec_api.devname, devname, strlen(devname));
	ec_api.verbose	= wanec_api_verbose(verbose);
	ec_api.cmd = WAN_EC_API_CMD_BUFFER_UNLOAD;
	ec_api.u_buffer_config.buffer_index	= (UINT32)bufferunload->buffer_id;
	err = wanec_api_lib_cmd(&ec_api);
	if (err) return err;
	if (ec_api.err) return 0;
	return 0;
}

int _WANEC_API_CALL wanec_api_playout(	char			*devname,
			int			verbose,
			wanec_api_playout_t	*playout)
{
	int	err;
	wan_ec_api_t		ec_api;
	memset(&ec_api, 0, sizeof(ec_api));

	memcpy(ec_api.devname, devname, strlen(devname));
	ec_api.verbose	= wanec_api_verbose(verbose);
	ec_api.cmd	= (playout->start) ? 
				WAN_EC_API_CMD_PLAYOUT_START :
				WAN_EC_API_CMD_PLAYOUT_STOP;
	ec_api.fe_chan			= playout->fe_chan;
	ec_api.fe_chan_map		= (1 << playout->fe_chan);
	ec_api.u_playout.index		= (UINT32)playout->buffer_id;
	ec_api.u_playout.port		= playout->port;
	ec_api.u_playout.repeat_cnt	= playout->repeat_cnt;
	ec_api.u_playout.duration	= playout->duration;
	ec_api.u_playout.notifyonstop	= playout->notifyonstop;
	ec_api.u_playout.user_event_id	= playout->user_event_id;
	err = wanec_api_lib_cmd(&ec_api);
	if (err) return err;
	if (ec_api.err) return 0;
	return 0;
}

int _WANEC_API_CALL wanec_api_monitor(	char			*devname,
			int			verbose,
			wanec_api_monitor_t	*monitor)
{
	int	err;
	wan_ec_api_t		ec_api;
	memset(&ec_api, 0, sizeof(ec_api));

	memcpy(ec_api.devname, devname, strlen(devname));
	ec_api.verbose	= wanec_api_verbose(verbose);
	ec_api.cmd			= WAN_EC_API_CMD_MONITOR;
	ec_api.fe_chan			= monitor->fe_chan;
	ec_api.fe_chan_map		= (1 << monitor->fe_chan);
	ec_api.u_chan_monitor.data_mode = 
		(monitor->data_mode == 120) ? 
				cOCT6100_DEBUG_GET_DATA_MODE_120S :
				cOCT6100_DEBUG_GET_DATA_MODE_16S;
 
	err = wanec_api_lib_cmd(&ec_api);	//err = wanec_api_lib_monitor(&ec_api);
	if (err) return err;
	if (ec_api.err) return 0;
	return 0;
}

int _WANEC_API_CALL wanec_api_mem_buffer_load(char			*devname,
			int			verbose,
			wanec_api_membufferload_t	*bufferload)
{
	int	err;
	wan_ec_api_t		ec_api;
	memset(&ec_api, 0, sizeof(ec_api));

	memcpy(ec_api.devname, devname, strlen(devname));

	ec_api.verbose	= wanec_api_verbose(verbose);
	ec_api.cmd	= WAN_EC_API_CMD_BUFFER_LOAD;

	ec_api.u_buffer_config.data = bufferload->buffer;
    ec_api.u_buffer_config.size = bufferload->size;
	ec_api.u_buffer_config.pcmlaw = (UINT32)bufferload->pcmlaw;	

	err = wanec_api_lib_membufferload(&ec_api);
	if (err) return err;
	if (ec_api.err) return -EINVAL;
	bufferload->buffer_id = ec_api.u_buffer_config.buffer_index;
	return 0;
}

