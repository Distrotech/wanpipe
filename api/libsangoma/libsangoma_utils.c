/*******************************************************************************//**
 * \file libsangoma.c
 * \brief Wanpipe API Code Library for Sangoma AFT T1/E1/Analog/BRI/Serial hardware
 *
 * Author(s):	Nenad Corbic <ncorbic@sangoma.com>
 *              David Rokhvarg <davidr@sangoma.com>
 *              Michael Jerris <mike@jerris.com>
 * 				Anthony Minessale II <anthmct@yahoo.com>
 *
 * Copyright:	(c) 2005-2008 Nenad Corbic <ncorbic@sangoma.com>
 *
 * * Redistribution and use in source and binary forms, with or without
 *   modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the Sangoma Technologies nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY Sangoma Technologies ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL Sangoma Technologies BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 *******************************************************************************
 */         

#include "libsangoma-pvt.h"
#include "libsangoma.h"
#include "g711.h"
#include "math.h"


#ifdef WP_API_FEATURE_DRIVER_GAIN
static __inline int sangoma_set_gain(sng_fd_t fd, wanpipe_api_t *tdm_api, int cmd, float gain_val)
{

    unsigned char *gain;
	int x;
	float gain_lin = powf(10.0, gain_val / 20.0);
	int i;
	int coding=-1;
	
	{
		wanpipe_api_t tmp_tdm_api;
		memset(&tmp_tdm_api,0,sizeof(wanpipe_api_t));
		coding = sangoma_get_hw_coding(fd,&tmp_tdm_api);
		if (coding < 0) {
         	return coding;
		}
    }
    

	WANPIPE_API_INIT_CHAN(tdm_api, 0);
	SANGOMA_INIT_TDM_API_CMD_RESULT(*tdm_api);  

	tdm_api->wp_cmd.cmd = cmd;
	
    gain = tdm_api->wp_cmd.data;
	tdm_api->wp_cmd.data_len=256;
	
	switch (coding) {
	
	case WP_MULAW:
		for (i = 0; i < 256; i++) {
			if (gain_val) {
				x = (int) (((float) ulaw_to_linear(i)) * gain_lin);
				if (x > 32767) x = 32767;
				if (x < -32767) x = -32767;
				gain[i] = linear_to_ulaw(x);
			} else {
				gain[i] = i;
			}
		}
		break;
	case WP_ALAW:
		for (i = 0; i < 256; i++) {
			if (gain_val) {
				x = (int) (((float) alaw_to_linear(i)) * gain_lin);
				if (x > 32767) x = 32767;
				if (x < -32767) x = -32767;
				gain[i] = linear_to_alaw(x);
			} else {
				gain[i] = i;
			}
		}
		break;

	default:
		return -1;
	}


#if 0
	{
		int i;
		printf("Data size = %i\n",sizeof(tdm_api->wp_cmd.data));
		for (i=0;i<256;i++) {
			printf("Set Gain Coding=%i: [%03i]=%03i\n", coding, i, gain[i]);
		}
	}
#endif

	return sangoma_cmd_exec(fd, tdm_api); 


}

int _LIBSNG_CALL sangoma_set_tx_gain(sng_fd_t fd, wanpipe_api_t *tdm_api, float gain_val)  
{
	return sangoma_set_gain(fd,tdm_api, WP_API_CMD_SET_TX_GAINS, gain_val); 

}

int _LIBSNG_CALL sangoma_set_rx_gain(sng_fd_t fd, wanpipe_api_t *tdm_api, float gain_val)  
{
	return sangoma_set_gain(fd,tdm_api,WP_API_CMD_SET_RX_GAINS,gain_val);

}

int _LIBSNG_CALL sangoma_clear_tx_gain(sng_fd_t fd, wanpipe_api_t *tdm_api)
{
	WANPIPE_API_INIT_CHAN(tdm_api, 0);
	SANGOMA_INIT_TDM_API_CMD_RESULT(*tdm_api);  
	tdm_api->wp_cmd.cmd = WP_API_CMD_CLEAR_TX_GAINS;
	return sangoma_cmd_exec(fd, tdm_api);
}

int _LIBSNG_CALL sangoma_clear_rx_gain(sng_fd_t fd, wanpipe_api_t *tdm_api)
{
	WANPIPE_API_INIT_CHAN(tdm_api, 0);
	SANGOMA_INIT_TDM_API_CMD_RESULT(*tdm_api);
	tdm_api->wp_cmd.cmd = WP_API_CMD_CLEAR_RX_GAINS;
	return sangoma_cmd_exec(fd, tdm_api);
}

#endif

