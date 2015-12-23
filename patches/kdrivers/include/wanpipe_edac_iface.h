/* wanpipe_edac_iface.h */
#ifndef __WANPIPE_EDAC_IFACE_H
#define __WANPIPE_EDAC_IFACE_H


#define WAN_TDMV_RX     0
#define WAN_TDMV_TX     1

typedef struct wan_tdmv_pwr{
	unsigned int sum;
	unsigned int tap_depth;
	unsigned int tap_debug_counter;
}wan_tdmv_pwr_t;

typedef struct _sample_state_t{
	int state;
	int forced_state;
}sample_state_t;
#define SAMPLE_STATE_HISTORY_LEN	3

typedef struct wan_tdmv_rxtx_pwr{
	wan_tdmv_pwr_t direction[2];
	/* of type ED_STATE */
	int sample_state[SAMPLE_STATE_HISTORY_LEN];
	//sample_state_t sample_state[SAMPLE_STATE_HISTORY_LEN];

	unsigned int current_sample_number;

	/* of type ED_STATE */
	int current_state;

	/* see comment in ED code */
	int complete_samples_counter;

	/* debugging stuff */
	unsigned int total_samples_number;
	/* current counters */
	unsigned int echo_present_samples_number;
	unsigned int echo_absent_samples_number;
	/* history counters */
	unsigned int echo_present_samples_number_history;
	unsigned int echo_absent_samples_number_history;
}wan_tdmv_rxtx_pwr_t;


typedef enum { ECHO_PRESENT, ECHO_ABSENT, INDETERMINATE, DOUBLE_TALK, NOT_USED } ED_STATE;
 
typedef enum { ECHO_DETECT_OFF, ECHO_DETECT_ON } ED_CONTROL_STATE;

typedef struct _echo_detect_struct{
 	/* Used for reporting Echo Presence/Absence on a Asterisk CLI request. */
 	ED_STATE echo_state;
 
 	/* Controls start/stop of ED algorithm. */
 	ED_CONTROL_STATE echo_detection_state;
 	ED_CONTROL_STATE echo_detection_state_old;
 
	 /* if 1 ED algorithm enabled for the channel */
 	int ed_enabled;
	 
 	/* debugging stuff */
 	int echo_absent_samples_number;
 	int echo_present_samples_number;
 
 	unsigned int last_rx_power;
 	unsigned int last_tx_power;
 
}echo_detect_struct_t;
 
/*
#define TDMV_SAMPLE_STATE_DECODE(state)				\
	((state == ECHO_PRESENT) ? "ECHO_PRESENT" :		\
	 (state == ECHO_ABSENT) ? "ECHO_ABSENT" : 		\
         (state == INDETERMINATE) ? "INDETERMINATE" : "Invalid")
*/
#define TDMV_SAMPLE_STATE_DECODE(state)			\
	((state == ECHO_PRESENT)  ? "P" :		\
	 (state == ECHO_ABSENT)   ? "A" : 		\
	 (state == DOUBLE_TALK)   ? "D" : 		\
         (state == INDETERMINATE) ? "I" : "Invalid")

extern int wp_tdmv_calc_echo  (wan_tdmv_rxtx_pwr_t *pwr_rxtx, 
   		  	int is_mlaw,
			int channo,
			unsigned char* rxdata, unsigned char *txdata,
			int len);
extern void init_ed_state(wan_tdmv_rxtx_pwr_t *pwr_rxtx, int echo_detect_chan);

#endif
