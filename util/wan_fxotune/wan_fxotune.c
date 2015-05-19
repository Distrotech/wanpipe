/* 
 * wan_fxotune.c -- A utility for tuning the various settings on the fxo
 * 		modules for the Sangoma analog cards.
 *
 * by Jim Chou <jchou@sangoma.com>
 * 
 * (C) 2010 Sangoma Technologies.
 *
 * Based on fxotune utility
 * by Matthew Fredrickson <creslin@digium.com>
 */

/*
 * This program is free software, distributed under the terms of
 * the GNU General Public License Version 2 as published by the
 * Free Software Foundation. See the LICENSE file included with
 * this program for more details.
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <math.h>

#ifndef __WINDOWS__
#	include <sys/time.h>
#endif

#ifdef __WINDOWS__
# include <io.h>
#define write	_write
#define open	_open
#define close	_close
#endif

#include <libsangoma.h>
#include "libteletone.h"
#include "g711.h"

#include "wan_fxotune.h"


// 16 A108 cards is a reasonable maximum
#define WP_MAX_CARDS		(16)
#define WP_MAX_CARD_PORTS	(8)
#define WP_MAX_PORTS		(WP_MAX_CARDS*WP_MAX_CARD_PORTS)

// Max number of analog lines in a card
#define MAX_REMORA_MODULES	24
#define MAX_FXOFXS_CHANNELS	MAX_REMORA_MODULES

// NOTE:  MTU must divide TEST_DURATION evenly, so that no data is missed.
#define USER_PERIOD (10)
#define MTU			(8*USER_PERIOD)

// Buffer size for all transmits and receives
#define MAX_TX_DATA	(10000)

#define TEST_DURATION 2000
#define BUFFER_LENGTH (2 * TEST_DURATION)
#define SKIP_SAMPLES 800
#define SINE_SAMPLES 8000

// Teletone parameters
#define DTMF_ON     80
#define DTMF_OFF    80
#define DTMF_RATE   8000
#define DTMF_SIZE		((DTMF_ON+DTMF_OFF)*10*2)
	

static float sintable[SINE_SAMPLES];

static const float amplitude = 16384.0;

#ifdef __WINDOWS__
#define CONFIG_FILE "wan_fxotune.conf"
#else
#define CONFIG_FILE "/etc/wan_fxotune.conf"
#endif

static int audio_dump_fd = -1;

static int printbest = 0;

// Bug 5730
static int use_reserved_open = 0;

#define MAX_RESULTS 	(5)
struct result_catalog {
	int 	idx;
	float 	echo;
	float 	freqres;
	struct stdm_echo_coefs settings;
};

struct {
	struct result_catalog results[MAX_RESULTS];
	int numactive;
}	topresults;

static char *usage =
"Usage: wan_fxotune [-v[vv] (-s | -i <options> | -d <options>)\n"
"\n"
"	-s : set previously calibrated echo settings\n"
"	-i : calibrate echo settings\n"
"		options : [<dialstring>] [-t <calibtype>]\n"
"		[-b <startdev>][-e <stopdev>]\n"
"		[-n <dialstring>][-l <delaytosilence>][-m <silencegoodfor>]\n"
" 	-d : dump input and output waveforms to ./fxotune_dump.vals\n"
"		options : [-b <device>][-w <waveform>]\n"
"		   [-n <dialstring>][-l <delaytosilence>][-m <silencegoodfor>]\n"
"	-v : more output (-vv, -vvv also)\n"
"	-p : print the 5 best candidates for acim and coefficients settings\n"
"	-x : Perform sin/cos functions using table lookup\n"
"	-r : Use the reserved version of sangoma_open_api_span_chan (allows opening a channel more than once)\n"
"	-o <path> : Write the received raw 16-bit signed linear audio that is\n"
"	            used in processing to the file specified by <path>\n"
"	-c <config_file>\n"
"\n"
"		<calibtype>      - type of calibration\n"
"		                   (default 2, old method 1)\n"
"		<startdev>\n"
"		<stopdev>        - defines a range of devices to test\n"
"		                   (default: s1c1-s128c24)\n"
"		<dialstring>     - string to dial to clear the line\n"
"		                   (default 5)\n"
"		<delaytosilence> - seconds to wait for line to clear (default 0)\n"
"		<silencegoodfor> - seconds before line will no longer be clear\n"
"		                   (default 18)\n"
"		<device>         - the device to perform waveform dump on\n"
"		                   (default s1c1)\n"
"		<waveform>       - -1 for multitone waveform, or frequency of\n"
"		                   single tone (default -1)\n"
"		<config_file>    - Alternative file to set from / calibrate to.\n"
"				   (Default: wan_fxotune.conf)\n"
;


#define OUT_OF_BOUNDS(x) ((x) < 0 || (x) > 255)

typedef struct s_device_info {

	/** fd of device we are working with */
	sng_fd_t fd; 
	int span;
	int chan;
	sangoma_wait_obj_t *swo;
	int mtu;
	wanpipe_tdm_api_t tdm_api;

} device_info_t;



typedef struct s_silence_info {
	char *dialstr;

	/** fd of device we are working with */
	device_info_t *device;

	/** seconds we should wait after dialing the dialstring before we know for sure we'll have silence */
	int initial_delay;
	/** seconds after which a reset should occur */
	int reset_after;

	/** time of last reset */
#ifdef __WINDOWS__
	FILETIME last_reset; 
#else
	struct timeval last_reset; 
#endif
} silence_info_t;


static short outbuf[TEST_DURATION];
static int debug = 0;

static FILE *debugoutfile = NULL;

static int use_table = 0;

int open_span_chan(device_info_t *dinfo)
{
	int rc = -1;

	if ( use_reserved_open ) {
		if (debug) printf( "Using reserved open span chan for s%dc%d.\n", dinfo->span, dinfo->chan);
		dinfo->fd = __sangoma_open_api_span_chan( dinfo->span, dinfo->chan );
	} else {
		dinfo->fd = sangoma_open_api_span_chan( dinfo->span, dinfo->chan );
	}

	if( dinfo->fd == INVALID_HANDLE_VALUE) {
		printf( "Failed to open span chan for s%dc%d.\n", dinfo->span, dinfo->chan);
		return -1;
	}

	rc = sangoma_wait_obj_create( &dinfo->swo, dinfo->fd, SANGOMA_DEVICE_WAIT_OBJ );
	if ( rc != SANG_STATUS_SUCCESS ) {
		printf( "Failed to create waitable object for s%dc%d.\n", dinfo->span, dinfo->chan);
		return rc;
	}

	memset( &dinfo->tdm_api, 0, sizeof(wanpipe_tdm_api_t) );

	return 0;
}


static int set_echo_registers( sng_fd_t fd, struct stdm_echo_coefs *echoregs )
{
	int rc = 0;

	/* Set the ACIM register */
	rc = sangoma_fe_reg_write(fd, 30, echoregs->acim);
	if (rc) goto err1;

	/* Set the digital echo canceller registers */
	rc = sangoma_fe_reg_write(fd, 45, echoregs->coef1);
	if (rc) goto err1;

	rc = sangoma_fe_reg_write(fd, 46, echoregs->coef2);
	if (rc) goto err1;

	rc = sangoma_fe_reg_write(fd, 47, echoregs->coef3);
	if (rc) goto err1;

	rc = sangoma_fe_reg_write(fd, 48, echoregs->coef4);
	if (rc) goto err1;

	rc = sangoma_fe_reg_write(fd, 49, echoregs->coef5);
	if (rc) goto err1;

	rc = sangoma_fe_reg_write(fd, 50, echoregs->coef6);
	if (rc) goto err1;

	rc = sangoma_fe_reg_write(fd, 51, echoregs->coef7);
	if (rc) goto err1;

	rc = sangoma_fe_reg_write(fd, 52, echoregs->coef8);
	if (rc) goto err1;

	return 0;

err1:
    fprintf(stderr, "Error, sangoma_fe_reg_write failed %i\n", rc);
	return rc;
}


int configure_channel(device_info_t *dinfo)
{
	int rc = -1;
	int queue_sz = 0;

	// SLINEAR not supported by Wanpipe driver -- only supports ULAW/ALAW
	rc = sangoma_tdm_set_codec(dinfo->fd, &dinfo->tdm_api, WP_MULAW);
	if ( rc ) {
		fprintf(stderr, "Unable to set channel codec to ulaw: rc = %d, %s\n", rc, strerror(errno));
		return -1;
	}

	rc = sangoma_tdm_set_usr_period(dinfo->fd, &dinfo->tdm_api, USER_PERIOD);
	if ( rc  ) {
	   fprintf(stderr, "Unable to set buffer information: rc = %d, %s\n", rc, strerror(errno));
	   return -1;
	}

	// queue the messages, so that we don't have to wait for transmit to finish
	dinfo->mtu = sangoma_tdm_get_usr_mtu_mru(dinfo->fd, &dinfo->tdm_api);
	queue_sz = MAX_TX_DATA / dinfo->mtu + 1;
	queue_sz *= 2; // to be extra safe that another transmits follows immediatley
	rc = sangoma_set_tx_queue_sz( dinfo->fd, &dinfo->tdm_api, queue_sz );
	if ( rc < 0 ) {
		printf( "Unable to set tx queue size: %d, %s\n", rc, strerror(errno));
		return rc;
	}

	return 0;
}


void delay ( int ms )
{
#ifdef __WINDOWS__
	Sleep( ms );
#else
	usleep( ms * 1000 );
#endif
}


// receive slinear; will convert ulaw data to slinear.
static int read_data(device_info_t *dinfo, void *buffer, int buf_len)
{
	wp_api_hdr_t hdr;
	int rc;

	uint32_t in_flags = POLLIN; 
	uint32_t out_flags = 0;
	int mtu, rx_remain = buf_len / 2;
	int rx_pos = 0;

	short *linear_data = buffer; // 16 bit elements
	static unsigned char ulaw_data[MAX_TX_DATA + MTU]; // 8 bit elements
	int samples = buf_len / 2;
	int i;
   
	mtu = dinfo->mtu;
	if ( debug ) printf( "read_data:  mtu = %d, samples = %d\n", mtu, samples );

	while(1) {

		if ( rx_remain <= 0 ) {
			rc = rx_pos * 2;
			break;
		}

		// The waitfor function must be used to implement flow control.
		rc = sangoma_waitfor( dinfo->swo, in_flags, &out_flags, 10000 );
		if (SANG_STATUS_APIPOLL_TIMEOUT == rc) {
			printf( "rx sangoma_waitfor timeout: %d, %s\n", rc, strerror(errno));
			return rc;
		} else if (SANG_STATUS_SUCCESS != rc) {
			printf( "rx sangoma_waitfor failed: %d, %s\n", rc, strerror(errno));
			return rc;
		}

		if (debug > 1) printf( "rx sangoma_waitfor: %d, %s, flag 0x%x \n", rc, strerror(errno), out_flags );
			
		// Check for rx packets
		if ( out_flags & POLLIN ) { 
			rc = sangoma_readmsg(dinfo->fd,
					 &hdr, sizeof(wp_api_hdr_t),
					 &ulaw_data[rx_pos], mtu, 0);

			// err indicates bytes received
			if (rc <= 0 || rc != mtu) {
				printf( "\nError receiving data:  fd = %d, rc = %d, %s\n", dinfo->fd, rc, strerror(errno));
				return rc;
			}
			rx_pos += rc;
			rx_remain -= rc;
		}
	}

	// convert to slinear
	linear_data = (short *)buffer;
	memset( linear_data, 0, buf_len );
	for( i = 0; i < samples; i++ ) {
		linear_data[i] = ulaw_to_linear( ulaw_data[i] );
	}

	return rc;
}

// transmit slinear data; will convert to ulaw first
static int write_data(device_info_t *dinfo, void *buffer, int buf_len)
{
	wp_api_hdr_t hdr;
	int rc;

	int mtu, tx_remain = buf_len / 2;
	int tx_pos = 0;

	short *linear_data = buffer; // 16 bit elements
	static unsigned char ulaw_data[MAX_TX_DATA + MTU]; // 8 bit elements
	int samples = buf_len / 2;
	int i;


	// convert to ulaw
	memset( ulaw_data, 0, sizeof(ulaw_data) );
	for (i = 0; i < samples; i++) {
		ulaw_data[i] = linear_to_ulaw((int)linear_data[i]);	
	}
		
	mtu = dinfo->mtu;
	if (debug) printf( "write_data:  mtu = %d, samples = %d\n", mtu, samples );
   
	while(1) {

		if ( tx_remain <= 0 ) {
			rc = tx_pos * 2;
			break;
		}

		rc = sangoma_writemsg(dinfo->fd,
				 &hdr, sizeof(wp_api_hdr_t),
				 &ulaw_data[tx_pos], mtu, 0);

		// err indicates bytes transmitted
		if (rc <= 0 || rc != mtu) {
			printf( "\nError transmitting data:  fd = %d, rc = %d, %s\n", dinfo->fd, rc, strerror(errno));
			return rc;
		}
		tx_pos += rc;
		tx_remain -= rc;

	}

	return rc;
}

// transmit a dial string 
int dtmf_transmit(device_info_t *dinfo, char *dialstr)
{
	int res;
	int err;
	char *dtmf;
	int slin_len;
	
	teletone_generation_session_t   tone_session;

	// init 
	memset(&tone_session, 0, sizeof(teletone_generation_session_t));
	if (teletone_init_session(&tone_session, 0, NULL, NULL)) {
		printf("Error: Failed to init teletone session!\n");
		return -1;
	} else {
		tone_session.rate = DTMF_RATE;
		tone_session.duration = DTMF_ON * (DTMF_RATE / 1000); // in samples
		tone_session.wait = DTMF_OFF * (DTMF_RATE / 1000); // in samples
	}

	for ( dtmf = dialstr; *dtmf; dtmf++ ) {

		// generate
		res = teletone_mux_tones(&tone_session, &tone_session.TONES[(int)*dtmf]);
		if ( res == 0 ) {
			printf("Error: Sending DTMF '%c' (err=%i)\n", *dtmf, res);
			return -1;
		}

		// transmit
		slin_len = res;
		err = write_data(dinfo, tone_session.buffer, slin_len * 2);
		if (err <= 0) {
			printf("Error: Failed to transmit DTMF '%c' (err=%i samples=%i)!\n", *dtmf, err, slin_len);
			return -1;
		}
		if (debug) printf( "Sent DTMF '%c' (with %d samples)\n", *dtmf, slin_len );

		fflush( stdout );
		delay( DTMF_ON + DTMF_OFF ); // wait for transmit to complete
	}

	return 0;
}


static int fxotune_read(device_info_t *dinfo, void *buffer, int len)
{
	int res;

	res = read_data(dinfo, buffer, len);
	if (debug) printf( "read_data(dinfo, buffer, len), res = %d\n", res );

	if ((res > 0) && (audio_dump_fd != -1)) {
		res = write(audio_dump_fd, buffer, len);
		printf( "write(audio_dump_fd, buffer, len), res = %d\n", res );
	}

	return res;
}


/**
 * Makes sure that the line is clear.
 * Right now, we do this by relying on the user to specify how long after dialing the
 * dialstring we can rely on the line being silent (before the telco complains about
 * the user not hitting the next digit).
 * 
 * A more robust way to do this would be to actually measure the sound levels on the line,
 * but that's a lot more complicated, and this should work.
 * 
 * @return 0 if succesful (no errors), 1 if unsuccesful
 */
static int ensure_silence(silence_info_t *info)
{

    // Computer elasped time in ms
#ifdef __WINDOWS__
   	FILETIME tv;
	DWORD64 elapsedms = 0;

	GetSystemTimeAsFileTime( &tv );
	if (info->last_reset.dwHighDateTime == 0) {
		/* this is the first request, we will force it to run */
		elapsedms = -1;
	} else {
		/* this is not the first request, we will compute elapsed time */
		elapsedms = (tv.dwHighDateTime - info->last_reset.dwHighDateTime) << 16 << 16;
		elapsedms |= (tv.dwLowDateTime - info->last_reset.dwLowDateTime);
		elapsedms /= 10000;
	}
#else
	struct timeval tv;
	long int elapsedms;

	gettimeofday(&tv, NULL);
	if (info->last_reset.tv_sec == 0) {
		/* this is the first request, we will force it to run */
		elapsedms = -1;
	} else {
		/* this is not the first request, we will compute elapsed time */
		elapsedms = ((tv.tv_sec - info->last_reset.tv_sec) * 1000L + (tv.tv_usec - info->last_reset.tv_usec) / 1000L);
	}
#endif

	if (debug > 4) {
		fprintf(stdout, "Reset line request received - elapsed ms = %li / reset after = %li\n", elapsedms, info->reset_after * 1000L);
	}

	if (elapsedms > 0 && elapsedms < info->reset_after * 1000L)
		return 0;
	
	if (debug > 1){
		fprintf(stdout, "Resetting line\n");
	}
	
	/* do a line reset */
	/* prepare line for silence */
	/* Do line hookstate reset */

	if (sangoma_tdm_txsig_onhook(info->device->fd, &info->device->tdm_api)) {
		fprintf(stderr, "Unable to hang up fd %d\n", info->device->fd);
		return -1;
	}

	delay( 2000 );
	if (sangoma_tdm_txsig_offhook(info->device->fd, &info->device->tdm_api)) {
		fprintf(stderr, "Cannot bring fd %d off hook\n", info->device->fd);
		return -1 ;
	}
	delay( 2000 ); /* Added to ensure that dial can actually takes place */

	// DTMF - send dialstr
	if( dtmf_transmit( info->device, info->dialstr ) ) {
		fprintf(stderr, "Unable to dial!\n");
	}
		
	delay( 1000 );
	delay( info->initial_delay * 1000 );  
	
#ifdef __WINDOWS__
	GetSystemTimeAsFileTime( &info->last_reset );
#else
	gettimeofday(&info->last_reset, NULL);
#endif
	
	return 0;
}

/**
 * Generates a tone of specified frequency.
 * 
 * @param hz the frequency of the tone to be generated
 * @param idx the current sample
 * 		to begenerated.  For a normal waveform you need to increment
 * 		this every time you execute the function.
 *
 * @return 16bit slinear sample for the specified index
 */
static short inline gentone(int hz, int idx)
{
	return amplitude * sin((idx * 2.0 * M_PI * hz)/8000);
}

/* Using DTMF tones for now since they provide good mid band testing 
 * while not being harmonics of each other */
static int freqs[] = {697, 770, 941, 1209, 1336, 1633};
static int freqcount = 6;

/**
 * Generates a waveform of several frequencies.
 * 
 * @param idx the current sample
 * 		to begenerated.  For a normal waveform you need to increment
 * 		this every time you execute the function.
 *
 * @return 16bit slinear sample for the specified index
 */
static short inline genwaveform(int idx)
{
	int i = 0;
	float response = (float)0;
	for (i = 0; i < freqcount; i++){
		response += sin((idx * 2.0 * M_PI * freqs[i])/8000);
	}
	

	return amplitude * response / freqcount;
}


/**
 *  Calculates the RMS of the waveform buffer of samples in 16bit slinear format.
 *  prebuf the buffer of either shorts or floats
 *  bufsize the number of elements in the prebuf buffer (not the number of bytes!)
 *  short_format 1 if prebuf points to an array of shorts, 0 if it points to an array of floats
 *  
 *  Formula for RMS (http://en.wikipedia.org/wiki/Root_mean_square): 
 *  
 *  Xrms = sqrt(1/N Sum(x1^2, x2^2, ..., xn^2))
 *  
 *  Note:  this isn't really a power calculation - but it gives a good measure of the level of the response
 *  
 *  @param prebuf the buffer containing the values to compute
 *  @param bufsize the size of the buffer
 *  @param short_format 1 if prebuf contains short values, 0 if it contains float values
 */
static float power_of(void *prebuf, int bufsize, int short_format)
{
	float sum_of_squares = 0;
	int numsamples = 0;
	float finalanswer = 0;
	short *sbuf = (short*)prebuf;
	float *fbuf = (float*)prebuf;
	int i = 0;

	if (short_format) {
		/* idiot proof checks */
		if (bufsize <= 0)
			return -1;

		numsamples = bufsize; /* Got rid of divide by 2 - the bufsize parameter should give the number of samples (that's what it does for the float computation, and it should do it here as well) */

		for (i = 0; i < numsamples; i++) {
			sum_of_squares += ((float)sbuf[i] * (float)sbuf[i]);
		}
	} else {
		/* Version for float inputs */
		for (i = 0; i < bufsize; i++) {
			sum_of_squares += (fbuf[i] * fbuf[i]);
		}
	}

	finalanswer = sum_of_squares/(float)bufsize; /* need to divide by the number of elements in the sample for RMS calc */

	if (finalanswer < 0) {
		fprintf(stderr, "Error: Final answer negative number %f\n", finalanswer);
		return -3;
	}

	return sqrtf(finalanswer);
}

/* 
 * In an effort to eliminate as much as possible the effect of outside noise, we use principles
 * from the Fourier Transform to attempt to calculate the return loss of our signal for each setting.
 *
 * To begin, we send our output signal out on the line.  We then receive back the reflected
 * response.  In the Fourier Transform, each evenly distributed frequency within the window
 * is correlated (multiplied against, then the resulting samples are added together) with
 * the real (cos) and imaginary (sin) portions of that frequency base to detect that frequency.
 * 
 * Instead of doing a complete Fourier Transform, we solve the transform for only our signal
 * by multiplying the received signal by the real and imaginary portions of our reference
 * signal.  This then gives us the real and imaginary values that we can use to calculate
 * the return loss of the sinusoids that we sent out on the line.  This is done by finding
 * the magnitude (think polar form) of the vector resulting from the real and imaginary
 * portions calculated above.
 *
 * This essentially filters out any other noise which maybe present on the line which is outside
 * the frequencies used in our test multi-tone.
 */

void init_sinetable(void)
{
	int i;
	if (debug) {
		fprintf(stdout, "Using sine tables with %d samples\n", SINE_SAMPLES);
	}
	for (i = 0; i < SINE_SAMPLES; i++) {
		sintable[i] = sin(((float)i * 2.0 * M_PI )/(float)(SINE_SAMPLES));
	}
}

/* Sine and cosine table lookup to use periodicity of the calculations being done */
float sin_tbl(int arg, int num_per_period)
{
	arg = arg % num_per_period;

	arg = (arg * SINE_SAMPLES)/num_per_period;

	return sintable[arg];
}

float cos_tbl(int arg, int num_per_period)
{
	arg = arg  % num_per_period;

	arg = (arg * SINE_SAMPLES)/num_per_period;

	arg = (arg + SINE_SAMPLES/4) % SINE_SAMPLES;  /* Pi/2 adjustment */

	return sintable[arg];
}


static float db_loss(float measured, float reference)
{
	double log1, log2;
	
	log1 = logf(measured/reference);
	log2 = logf(10);

	return 20 * (log1 / log2);
}

static void one_point_dft(const short *inbuf, int len, int frequency, float *real, float *imaginary)
{
	float myreal = 0, myimag = 0;
	int i;

	for (i = 0; i < len; i++) {
		if (use_table) {
			myreal += (float) inbuf[i] * cos_tbl(i*frequency, 8000);
			myimag += (float) inbuf[i] * sin_tbl(i*frequency, 8000);
		} else {
			myreal += (float) inbuf[i] * cos((i * 2.0 * M_PI * frequency)/8000);
			myimag += (float) inbuf[i] * sin((i * 2.0 * M_PI * frequency)/8000);
		}
	}

	myimag *= -1;

	*real = myreal / (float) len;
	*imaginary = myimag / (float) len;
}


static float calc_magnitude(short *inbuf, int insamps)
{
	float real, imaginary, magnitude;
	float totalmagnitude = 0;
	int i;

	for (i = 0; i < freqcount; i++) {
		one_point_dft(inbuf, insamps, freqs[i], &real, &imaginary);
		magnitude = sqrtf((real * real) + (imaginary * imaginary));
		totalmagnitude += magnitude;
	}

	return totalmagnitude;
}


/**
 *  dumps input and output buffer contents for the echo test - used to see exactly what's going on
 */
static int maptone(device_info_t *dinfo, int freq, char *dialstr, int delayuntilsilence)
{
	int i = 0;
	int res = 0;
	short inbuf[TEST_DURATION]; /* changed from BUFFER_LENGTH - this buffer is for short values, so it should be allocated using the length of the test */
	FILE *outfile = NULL;
	int leadin = 50;
	int trailout = 100;
	silence_info_t sinfo;
	float power_result;
	float power_waveform;
	float echo;

	outfile = fopen("fxotune_dump.vals", "w");
	if (!outfile) {
		fprintf(stdout, "Cannot create fxotune_dump.vals\n");
		return -1;
	}

	/* Configure the channel */
	if ( configure_channel( dinfo ) ) {
		fprintf(stderr, "Unable to configure the channel!\n" );
		return -1;
	}

	/* Fill the output buffers */
	for (i = 0; i < leadin; i++)
		outbuf[i] = 0;
	for (; i < TEST_DURATION - trailout; i++){
		outbuf[i] = freq > 0 ? gentone(freq, i) : genwaveform(i); /* if frequency is negative, use a multi-part waveform instead of a single frequency */
	}
	for (; i < TEST_DURATION; i++)
		outbuf[i] = 0;

	/* Make sure the line is clear */
	memset(&sinfo, 0, sizeof(sinfo));
	sinfo.device = dinfo;
	sinfo.dialstr = dialstr;
	sinfo.initial_delay = delayuntilsilence;
	sinfo.reset_after = 4; /* doesn't matter - we are only running one test */


	if (ensure_silence(&sinfo)){
		fprintf(stderr, "Unable to get a clear outside line\n");
		return -1;
	}

	/* Flush buffers */
	if (sangoma_flush_bufs(dinfo->fd, &dinfo->tdm_api)) {
		fprintf(stderr, "Unable to flush I/O: %s\n", strerror(errno));
		return -1;
	}

	/* send data out on line */
	res = write_data(dinfo, outbuf, BUFFER_LENGTH); /* sending a TEST_DURATION length array of shorts (which are 2 bytes each) */
	if (res != BUFFER_LENGTH) { 
		fprintf(stderr, "Could not write all data to line\n");
		return -1;
	}

retry:
	/* read return response */
	res = fxotune_read(dinfo, inbuf, BUFFER_LENGTH);
	if (res != BUFFER_LENGTH) {
		printf( "fxotune_read:  res = %d, BUFFER_LENGTH = %d\n", res, BUFFER_LENGTH );
		goto retry;
	}

	/* write content of output buffer to debug file */
	power_result = power_of(inbuf, TEST_DURATION, 1);
	power_waveform = power_of(outbuf, TEST_DURATION, 1);
	echo = power_result/power_waveform;
	
	fprintf(outfile, "Buffers, freq=%d, outpower=%0.0f, echo=%0.4f\n", freq, power_result, echo);
	fprintf(outfile, "Sample, Input (received from the line), Output (sent to the line)\n");
	for (i = 0; i < TEST_DURATION; i++){
		fprintf(outfile, "%d, %d, %d\n", 
			i,
			inbuf[i],
			outbuf[i]
		);
	}

	fclose(outfile);
	
	fprintf(stdout, "echo ratio = %0.4f (%0.1f / %0.1f)\n", echo, power_result, power_waveform);
	
	return 0;
}


/**
 *  Initialize the data store for storing off best calculated results
 */
static void init_topresults(void)
{
	topresults.numactive = 0;
}


/**
 *  If this is a best result candidate, store in the top results data store
 * 		This is dependent on being the lowest echo value
 *
 *  @param tbleoffset - The offset into the echo_trys table used
 *  @param setting - Pointer to the settings used to achieve the fgiven value
 *  @param echo - The calculated echo return value (in dB)
 *  @param echo - The calculated magnitude of the response
 */
static void set_topresults(int tbloffset, struct stdm_echo_coefs *setting, float echo, float freqres)
{
	int place;
	int idx;

	for ( place = 0; place < MAX_RESULTS && place < topresults.numactive; place++) {
		if (echo < topresults.results[place].echo) {
			break;
		}
	}

	if (place < MAX_RESULTS) {
		/*  move results to the bottom */
		for (idx = topresults.numactive-2; idx >= place; idx--) {
			topresults.results[idx+1] = topresults.results[idx];
		}
		topresults.results[place].idx = tbloffset;
		topresults.results[place].settings = *setting;
		topresults.results[place].echo = echo;
		topresults.results[place].freqres = freqres;
		if (MAX_RESULTS > topresults.numactive) {
			topresults.numactive++;
		}
	}
}


/**
 *  Prints the top results stored to stdout
 *
 *  @param header - Text that goes in the header of the response
 */
static void print_topresults(char * header)
{
	int item;

	fprintf(stdout, "Top %d results for %s\n", topresults.numactive, header);
	for (item = 0; item < topresults.numactive; item++) {
		fprintf(stdout, "Res #%d: index=%d, %3d,%3d,%3d,%3d,%3d,%3d,%3d,%3d,%3d: magnitude = %0.0f, echo = %0.4f dB\n",
				item+1, topresults.results[item].idx, topresults.results[item].settings.acim,
				topresults.results[item].settings.coef1, topresults.results[item].settings.coef2,
				topresults.results[item].settings.coef3, topresults.results[item].settings.coef4,
				topresults.results[item].settings.coef5, topresults.results[item].settings.coef6,
				topresults.results[item].settings.coef7, topresults.results[item].settings.coef8,
				topresults.results[item].freqres, topresults.results[item].echo);
		
	}
}


/**
 * Perform calibration type 2 on the specified device
 * 
 * Determine optimum echo coefficients for the specified device
 * 
 * New tuning strategy.  If we have a number that we can dial that will result in silence from the
 * switch, the tune will be *much* faster (we don't have to keep hanging up and dialing a digit, etc...)
 * The downside is that the user needs to actually find a 'no tone' phone number at their CO's switch - but for
 * really fixing echo problems, this is what it takes.
 *
 * Also, for the purposes of optimizing settings, if we pick a single frequency and test with that,
 * we can try a whole bunch of impedence/echo coefficients.  This should give better results than trying
 * a bunch of frequencies, and we can always do a a frequency sweep to pick between the best 3 or 4
 * impedence/coefficients configurations.
 *   
 * Note:  It may be possible to take this even further and do some pertubation analysis on the echo coefficients
 * 		 themselves (maybe use the 72 entry sweep to find some settings that are close to working well, then
 * 		 deviate the coefficients a bit to see if we can improve things).  A better way to do this would be to
 * 		 use the optimization strategy from silabs.  For reference, here is an application note that describes
 * 		 the echo coefficients (and acim values):
 * 		 
 * 		 http://www.silabs.com/Support%20Documents/TechnicalDocs/an84.pdf
 *
 * 		 See Table 13 in this document for a breakdown of acim values by region.
 *
 * 		 http://www.silabs.com/Support%20Documents/TechnicalDocs/si3050-18-19.pdf
 * 		 
 */
static int acim_tune2(device_info_t *dinfo, int freq, char *dialstr, int delayuntilsilence, int silencegoodfor, struct stdm_echo_coefs *coefs_out)
{
	int i = 0;
	int res = 0;
	int lowesttry = -1;
	float lowesttryresult = 999999999999.0;
	float lowestecho = 999999999999.0;
	short inbuf[TEST_DURATION * 2];
	silence_info_t sinfo;
	int echo_trys_size = 72;
	int trys = 0;
	float waveform_power;
	float freq_result;
	float echo;

	init_topresults();

	if (debug && !debugoutfile) {
		if (!(debugoutfile = fopen("fxotune.vals", "w"))) {
			fprintf(stdout, "Cannot create fxotune.vals\n");
			return -1;
		}
	}

	/* Set echo settings */
	if (set_echo_registers(dinfo->fd, &echo_trys[0])) {
		fprintf(stderr, "Unable to set impedance on fd %d\n", dinfo->fd);
		return -1;
	}

	/* Configure the channel */
	if ( configure_channel( dinfo ) ) {
		fprintf(stderr, "Unable to configure the channel.\n" );
		return -1;
	}

	if (sangoma_tdm_txsig_offhook(dinfo->fd, &dinfo->tdm_api)) {
		fprintf(stderr, "Cannot bring fd %d off hook\n", dinfo->fd);
		return -1;
	}
	/* Set up silence settings */
	memset(&sinfo, 0, sizeof(sinfo));
	sinfo.device = dinfo;
	sinfo.dialstr = dialstr;
	sinfo.initial_delay = delayuntilsilence;
	sinfo.reset_after = silencegoodfor;

	/* Fill the output buffers */
	for (i = 0; i < TEST_DURATION; i++)
		outbuf[i] = freq > 0 ? gentone(freq, i) : genwaveform(i); /* if freq is negative, use a multi-frequency waveform */
	
	/* compute power of input (so we can later compute echo levels relative to input) */
	waveform_power = calc_magnitude(outbuf, TEST_DURATION);

	/* sweep through the various coefficient settings and see how our responses look */

	for (trys = 0; trys < echo_trys_size; trys++){
		
		/* ensure silence on the line */
		if (ensure_silence(&sinfo)){
			fprintf(stderr, "Unable to get a clear outside line\n");
			return -1;
		}
		
		if (set_echo_registers(dinfo->fd, &echo_trys[trys])) {
			fprintf(stderr, "Unable to set echo coefficients on fd %d\n", dinfo->fd);
			return -1;
		}

		/* Flush buffers */
		if (sangoma_flush_bufs(dinfo->fd, &dinfo->tdm_api)) {
			fprintf(stderr, "Unable to flush I/O: %s\n", strerror(errno));
			return -1;
		}

		/* send data out on line */
		res = write_data(dinfo, outbuf, BUFFER_LENGTH);
		if (res != BUFFER_LENGTH) {
			fprintf(stderr, "Could not write all data to line\n");
			return -1;
		}

retry:
		/* read return response */
		res = fxotune_read(dinfo, inbuf, BUFFER_LENGTH * 2);
		if (res != BUFFER_LENGTH * 2) {
			printf( "fxotune_read:  res = %d, BUFFER_LENGTH = %d\n", res, BUFFER_LENGTH );
			goto retry;
		}

		freq_result = calc_magnitude(inbuf, TEST_DURATION * 2);
		echo = db_loss(freq_result, waveform_power);
		
#if 0
		if (debug > 0)
			fprintf(stdout, "%3d,%d,%d,%d,%d,%d,%d,%d,%d: magnitude = %0.0f, echo = %0.4f dB\n", 
					echo_trys[trys].acim, echo_trys[trys].coef1, echo_trys[trys].coef2,
					echo_trys[trys].coef3, echo_trys[trys].coef4, echo_trys[trys].coef5,
					echo_trys[trys].coef6, echo_trys[trys].coef7, echo_trys[trys].coef8,
					freq_result, echo);
#endif

		if (freq_result < lowesttryresult){
			lowesttry = trys;
			lowesttryresult = freq_result;
			lowestecho = echo;
		}
		if (debug) {
			char result[256];
			snprintf(result, sizeof(result), "%3d,%3d,%3d,%3d,%3d,%3d,%3d,%3d,%3d,%f,%f", 
						echo_trys[trys].acim, 
						echo_trys[trys].coef1, 
						echo_trys[trys].coef2, 
						echo_trys[trys].coef3, 
						echo_trys[trys].coef4, 
						echo_trys[trys].coef5, 
						echo_trys[trys].coef6, 
						echo_trys[trys].coef7, 
						echo_trys[trys].coef8, 
						freq_result,
						echo
					);
			
			fprintf(debugoutfile, "%s\n", result);
			fprintf(stdout, "%3d,%3d,%3d,%3d,%3d,%3d,%3d,%3d,%3d: magnitude = %0.0f, echo = %0.4f dB\n",
					echo_trys[trys].acim, echo_trys[trys].coef1, echo_trys[trys].coef2,
					echo_trys[trys].coef3, echo_trys[trys].coef4, echo_trys[trys].coef5,
					echo_trys[trys].coef6, echo_trys[trys].coef7, echo_trys[trys].coef8,
					freq_result, echo);
		}

		if (printbest) {
			set_topresults(trys, &echo_trys[trys], echo, freq_result);
		}
	}

	if (debug > 0)
		fprintf(stdout, "Config with lowest response = %d, magnitude = %0.0f, echo = %0.4f dB\n", lowesttry, lowesttryresult, lowestecho);

	memcpy(coefs_out, &echo_trys[lowesttry], sizeof(struct stdm_echo_coefs));
	if (printbest) {
		print_topresults("Acim2_tune Test");
	}

	return 0;
}

/**
 *  Perform calibration type 1 on the specified device.  Only tunes the line impedance.  Look for best response range 
 */
static int acim_tune(device_info_t *dinfo, char *dialstr, int delayuntilsilence, int silencegoodfor, struct stdm_echo_coefs *coefs_out)
{
	int i = 0, freq = 0, acim = 0;
	int res = 0;
	struct stdm_echo_coefs coefs;
	short inbuf[TEST_DURATION]; /* changed from BUFFER_LENGTH - this buffer is for short values, so it should be allocated using the length of the test */
	int lowest = 0;
	FILE *outfile = NULL;
	float acim_results[16];
	silence_info_t sinfo;

	if (debug) {
		outfile = fopen("fxotune.vals", "w");
		if (!outfile) {
			fprintf(stdout, "Cannot create fxotune.vals\n");
			return -1;
		}
	}

	/* Set up silence settings */
	memset(&sinfo, 0, sizeof(sinfo));
	sinfo.device = dinfo;
	sinfo.dialstr = dialstr;
	sinfo.initial_delay = delayuntilsilence;
	sinfo.reset_after = silencegoodfor;

	/* Set echo settings */
	memset(&coefs, 0, sizeof(coefs));
	if (set_echo_registers(dinfo->fd, &coefs)) {
		fprintf(stdout, "Skipping non-TDM / non-FXO\n");
		return -1;
	}

	/* Configure the channel */
	if ( configure_channel( dinfo ) ) {
		fprintf(stderr, "Unable to configure the channel!\n" );
		return -1;
	}

	for (acim = 0; acim < 16; acim++) {
		float freq_results[15];

		coefs.acim = acim;
		if (set_echo_registers(dinfo->fd, &coefs)) {
			fprintf(stderr, "Unable to set impedance on fd %d\n", dinfo->fd);
			return -1;
		}

		for (freq = 200; freq <=3000; freq+=200) {
			/* Fill the output buffers */
			for (i = 0; i < TEST_DURATION; i++)
				outbuf[i] = gentone(freq, i);

			/* Make sure line is ready for next test iteration */
			if (ensure_silence(&sinfo)){
				fprintf(stderr, "Unable to get a clear line\n");
				return -1;
			}
			

			/* Flush buffers */
			if (sangoma_flush_bufs(dinfo->fd, &dinfo->tdm_api)) {
				fprintf(stderr, "Unable to flush I/O: %s\n", strerror(errno));
				return -1;
			}
	
			/* send data out on line */
			res = write_data(dinfo, outbuf, BUFFER_LENGTH);
			if (res != BUFFER_LENGTH) {
				fprintf(stderr, "Could not write all data to line\n");
				return -1;
			}

			/* read return response */
retry:
			/* read return response */
			res = fxotune_read(dinfo, inbuf, BUFFER_LENGTH);
			if (res != BUFFER_LENGTH) {
				printf( "fxotune_read:  res = %d, BUFFER_LENGTH = %d\n", res, BUFFER_LENGTH );
				goto retry;
			}

			/* calculate power of response */
			
			freq_results[(freq/200)-1] = power_of(inbuf+SKIP_SAMPLES, TEST_DURATION-SKIP_SAMPLES, 1); /* changed from inbuf+SKIP_BYTES, BUFFER_LENGTH-SKIP_BYTES, 1 */
			if (debug) fprintf(outfile, "%d,%d,%f\n", acim, freq, freq_results[(freq/200)-1]);
		}
		acim_results[acim] = power_of(freq_results, 15, 0);
	}

	if (debug) {
		for (i = 0; i < 16; i++)
			fprintf(outfile, "acim_results[%d] = %f\n", i, acim_results[i]);
	}
	/* Find out what the "best" impedance is for the line */
	lowest = 0;
	for (i = 0; i < 16; i++) {
		if (acim_results[i] < acim_results[lowest]) {
			lowest = i;
		}
	}

	coefs_out->acim = lowest;
	coefs_out->coef1 = 0;
	coefs_out->coef2 = 0;
	coefs_out->coef3 = 0;
	coefs_out->coef4 = 0;
	coefs_out->coef5 = 0;
	coefs_out->coef6 = 0;
	coefs_out->coef7 = 0;
	coefs_out->coef8 = 0;
	
	return 0;
}

/**
 * Reads echo register settings from the configuration file and pushes them into
 * the appropriate devices
 * 
 * @param configfilename the path of the file that the calibration results should be written to
 * 
 * @return 0 if successful, !0 otherwise
 */	
static int do_set(char *configfilename)
{
	FILE *fp = NULL;
	int res = 0;
	sng_fd_t fd = 0;
		
	fp = fopen(configfilename, "r");
	
    if (!fp) {
            fprintf(stdout, "Cannot open %s!\n",configfilename);
            return -1;
    }

	while (res != EOF) {
		struct stdm_echo_coefs mycoefs;
		int span,chan,myacim,mycoef1,mycoef2,mycoef3,mycoef4,mycoef5,mycoef6,mycoef7,mycoef8;


		res = fscanf(fp, "s%dc%d=%d,%d,%d,%d,%d,%d,%d,%d,%d\n",&span,&chan,&myacim,&mycoef1,
				&mycoef2,&mycoef3,&mycoef4,&mycoef5,&mycoef6,&mycoef7,
				&mycoef8);

		if (res == EOF || res == 0) {
			break;
		}

		if (debug) {
			printf("s%dc%d=%d,%d,%d,%d,%d,%d,%d,%d,%d\n",span,chan,myacim,mycoef1,
				mycoef2,mycoef3,mycoef4,mycoef5,mycoef6,mycoef7,
				mycoef8);
		}

		/* Check to be sure conversion is done correctly */
		if (OUT_OF_BOUNDS(myacim) || OUT_OF_BOUNDS(mycoef1)||
			OUT_OF_BOUNDS(mycoef2)|| OUT_OF_BOUNDS(mycoef3)||
			OUT_OF_BOUNDS(mycoef4)|| OUT_OF_BOUNDS(mycoef5)||
			OUT_OF_BOUNDS(mycoef6)|| OUT_OF_BOUNDS(mycoef7)|| OUT_OF_BOUNDS(mycoef8)) {

			fprintf(stdout, "Bounds check error on inputs from %s:s%dc%d\n", configfilename, span, chan);
			return -1;
		}

		mycoefs.acim = myacim;
		mycoefs.coef1 = mycoef1;
		mycoefs.coef2 = mycoef2;
		mycoefs.coef3 = mycoef3;
		mycoefs.coef4 = mycoef4;
		mycoefs.coef5 = mycoef5;
		mycoefs.coef6 = mycoef6;
		mycoefs.coef7 = mycoef7;
		mycoefs.coef8 = mycoef8;
	

		if ( use_reserved_open ) {
			if (debug)
			   fprintf( stdout, "Using reserved open span chan for s%dc%d.\n", span, chan);
			fd = __sangoma_open_api_span_chan( span, chan );
		} else {
			fd = sangoma_open_api_span_chan( span, chan );
		}
		if (fd < 0) {
			fprintf(stdout, "open error on s%dc%ds: %s\n", span, chan, strerror(errno));
			return -1;
		}

		if (set_echo_registers(fd, &mycoefs)) {
			fprintf(stdout, "s%dc%d: %s\n", span, chan, strerror(errno));
			return -1;
		}

		sangoma_close(&fd);
	}

	fclose(fp);


	if (debug)
		fprintf(stdout, "fxotune: successfully set echo coeffecients on FXO modules\n");
	return 0;	
}

/**
 * Output waveform information from a single test
 * 
 * Clears the line, then sends a single waveform (multi-tone, or single tone), and listens
 * for the response on the line.  Output is written to fxotune_dump.vals
 * 
 * @param span the span of device to test
 * @param chan the channel of device to test
 * @param dialstr the string that should be dialed to clear the dialtone from the line
 * @param delayuntilsilence the number of seconds to wait after dialing dialstr before starting the test
 * @param silencegoodfor the number of seconds that the test can run before having to reset the line again
 * 			(this is basically the amount of time it takes before the 'if you'd like to make a call...' message
 * 			kicks in after you dial dialstr.  This test is so short that the value is pretty much ignored.
 * @param waveformtype the type of waveform to use - -1 = multi-tone waveform, otherwise the specified value
 * 			is used as the frequency of a single tone.  A value of 0 will output silence.
 */
static int do_dump(int span, int chan, char* dialstr, int delayuntilsilence, int silencegoodfor, int waveformtype)
{
	int res = 0;
	device_info_t dinfo;
	
	memset(&dinfo, 0, sizeof(dinfo));
	dinfo.span = span;
	dinfo.chan = chan;
	res = open_span_chan( &dinfo );
	if (res != 0 ) {
		fprintf(stdout, "s%dc%d absent: %s\n", span, chan, strerror(errno));
		return -1;
	}

	fprintf(stdout, "Dumping module s%dc%d\n", span, chan);
	res = maptone(&dinfo, waveformtype, dialstr, delayuntilsilence); 

	// hangup
	if (sangoma_tdm_txsig_onhook(dinfo.fd, &dinfo.tdm_api)) {
		fprintf(stderr, "Unable to hang up fd %d\n", dinfo.fd);
	}
	delay( 2000 );

	sangoma_close(&dinfo.fd);

	if (res) {
		fprintf(stdout, "Failure!\n");
		return res;
	} else {
		fprintf(stdout, "Done!\n");
		return 0;
	}

}	

/**
 * Performs calibration on the specified device
 * 
 * @param span the span of device to calibrate
 * @param chan the channel of device to calibrate
 * @param calibtype the type of calibration to perform.  1=old style (loops through individual frequencies
 * 			doesn't optimize echo coefficients.  2=new style (uses multi-tone and optimizes echo coefficients
 * 			and acim setting)
 * @param configfd the file descriptor that the calibration results should be written to
 * @param dialstr the string that should be dialed to clear the dialtone from the line
 * @param delayuntilsilence the number of seconds to wait after dialing dialstr before starting the test
 * @param silencegoodfor the number of seconds that the test can run before having to reset the line again
 * 			(this is basically the amount of time it takes before the 'if you'd like to make a call...' message
 * 			kicks in after you dial dialstr
 * 
 * @return 0 if successful, -1 for serious error such as device not available , > 0 indicates the number of channels
 */	
static int do_calibrate(int span, int chan, int calibtype, int configfd, char* dialstr, int delayuntilsilence, int silencegoodfor)
{
		int problems = 0;
		int res = 0;
		struct stdm_echo_coefs coefs;
		device_info_t dinfo;


		memset(&dinfo, 0, sizeof(dinfo));
		dinfo.span = span;
		dinfo.chan = chan;
		res = open_span_chan( &dinfo );
		if (res != 0 ) {
			fprintf(stdout, "s%dc%d absent: %s\n", span, chan, strerror(errno));
			problems++;
			return problems;
		}

		fprintf(stdout, "Tuning module s%dc%d\n", span, chan);

		
		if (1 == calibtype)
		    res = acim_tune(&dinfo, dialstr, delayuntilsilence, silencegoodfor, &coefs);
		else
			res = acim_tune2(&dinfo, -1, dialstr, delayuntilsilence, silencegoodfor, &coefs);

		// hangup
		if (sangoma_tdm_txsig_onhook(dinfo.fd, &dinfo.tdm_api)) {
			fprintf(stderr, "Unable to hang up fd %d\n", dinfo.fd);
		}
		delay( 2000 );

		sangoma_close(&dinfo.fd);
		
		if (res) {
			fprintf(stdout, "Failure!\n");
			problems++;
		} else {
			fprintf(stdout, "Done!\n");
		}

		if (res == 0) {
			
			/* Do output to file */
			int len = 0;
			static char output[255] = "";

			snprintf(output, sizeof(output), "s%dc%d=%d,%d,%d,%d,%d,%d,%d,%d,%d\n", 
				span,
				chan,
				coefs.acim, 
				coefs.coef1, 
				coefs.coef2, 
				coefs.coef3, 
				coefs.coef4, 
				coefs.coef5, 
				coefs.coef6, 
				coefs.coef7, 
				coefs.coef8
			);

			if (debug)
				fprintf(stdout, "Found best echo coefficients: %s\n", output);

			len = strlen(output);
			res = write(configfd, output, strlen(output));
			if (res != len) {
				fprintf(stdout, "Unable to write line \"%s\" to file.\n", output);
				return -1;
			}
		}


	
		return problems;
}	


// Calibrates all FXO lines starting from startspan/startchan to stopspan/stopchan
static int do_calibrate_many(int startspan, int startchan, int stopspan, int stopchan, int calibtype, char* configfilename, char* dialstr, int delayuntilsilence, int silencegoodfor)
{
	int configfd;

	int wp, mod, startmod, stopmod;
	int rc;
	sng_fd_t wp_fd;
	hardware_info_t *hwinfo;
	port_management_struct_t port_mgmnt;
	int tune_count = 0;

	int problems = 0;

	configfd = open(configfilename, O_CREAT|O_TRUNC|O_WRONLY, 0666);

	if (configfd < 0) {
		fprintf(stderr, "Cannot generate config file %s: open: %s\n", configfilename, strerror(errno));
		return -1;
	}

	for (wp = startspan; wp <= stopspan; wp++) {

		wp_fd = sangoma_open_driver_ctrl( wp );
		if (wp_fd == INVALID_HANDLE_VALUE) {
			if (debug) printf( "Skipping wanpipe%d.\n", wp );
			continue; // skip
		}
		rc = sangoma_driver_get_hw_info(wp_fd, &port_mgmnt, wp);
		sangoma_close(&wp_fd);
		if (rc) {
			//printf( "sangoma_driver_get_hw_info() failed for wanpipe%d!\n", wp);
			if (debug) printf( "Skipping wanpipe%d.\n", wp );
			continue; // skip
		}
		hwinfo = (hardware_info_t *) &port_mgmnt.data;
		if (hwinfo->fxo_map) {

			startmod = 1;
			stopmod = MAX_REMORA_MODULES;
			if (startspan == wp) {
				startmod = startchan;
			}
			if (stopspan == wp) {
				stopmod = stopchan;
			}

			for (mod = startmod; mod <= stopmod; mod++) {
				if ( hwinfo->fxo_map & (1 << mod) ) {

					// do the calibration
					rc = do_calibrate(wp, mod, calibtype, configfd, dialstr, delayuntilsilence, silencegoodfor);
					problems += rc;
					if ( rc == 0 ) {
						tune_count += 1;
					}
						
				}
			}
		}

	}

	close(configfd);

	if ( tune_count == 0 ) {
		fprintf(stdout, "Unable to tune any devices\n");
		return 1;
	}

	if (problems)
		fprintf(stdout, "Unable to tune %d devices, even though those devices are present\n", problems);
			
	return problems;
}


int main(int argc , char **argv)
{
	int startspan = 1; /* -b */
	int startchan = 1;
	int stopspan = WP_MAX_PORTS; /* -e */
	int stopchan = MAX_FXOFXS_CHANNELS;
	int calibtype = 2; /* -t */
	int waveformtype = -1; /* -w multi-tone by default.  If > 0, single tone of specified frequency */
	int delaytosilence = 0; /* -l */
	int silencegoodfor = 18; /* -m */
	char* dialstr = "5"; /* -n */
	int res = 0;
	int doset = 0; /* -s */
	int docalibrate = 0; /* -i <dialstr> */
	int dodump = 0; /* -d */
	int i = 0;
	int moreargs;
	char *configfile = CONFIG_FILE;
	
	for (i = 1; i < argc; i++){
		if (!(argv[i][0] == '-' || argv[i][0] == '/') || (strlen(argv[i]) <= 1)){
			fprintf(stdout, "Unknown option : %s\n", argv[i]);
			/* Show usage */
			fputs(usage, stdout);
			return -1;
		}

		moreargs = (i < argc - 1);
		
		switch(argv[i][1]){
			case 's':
				doset=1;
				continue;
			case 'i':
				docalibrate = 1;
				if (moreargs){ /* we need to check for a value after 'i' for backwards compatability with command line options of old fxotune */
					if (argv[i+1][0] != '-' && argv[i+1][0] != '/')
						dialstr = argv[++i];
				}
				continue;
			case 'c':
				configfile = moreargs ? argv[++i] : configfile;
				continue;
			case 'd':
				dodump = 1;
				continue;
			case 'b':
				if (moreargs) sscanf(argv[++i], "s%dc%d", &startspan, &startchan);
				break;
			case 'e':
				if (moreargs) sscanf(argv[++i], "s%dc%d", &stopspan, &stopchan);
				break;
			case 't':
				calibtype = moreargs ? atoi(argv[++i]) : calibtype;
				break;
			case 'w':
				waveformtype = moreargs ? atoi(argv[++i]) : waveformtype;
				break;
			case 'l':
				delaytosilence = moreargs ? atoi(argv[++i]) : delaytosilence;
				break;
			case 'm':
				silencegoodfor = moreargs ? atoi(argv[++i]) : silencegoodfor;
				break;
			case 'n':
				dialstr = moreargs ? argv[++i] : dialstr;
				break;
			case 'p':
				printbest++;
				break;
			case 'r':
				use_reserved_open = 1;
				break;
			case 'x':
				use_table = 1;
				break;
			case 'v':
				debug = strlen(argv[i])-1;
				break;
			case 'o':
				if (moreargs) {
					audio_dump_fd = open(argv[++i], O_WRONLY|O_CREAT|O_TRUNC, 0666);
					if (audio_dump_fd == -1) {
						fprintf(stdout, "Unable to open file %s: %s\n", argv[i], strerror(errno));
						return -1;
					}
					break;
				} else {
					fprintf(stdout, "No path supplied to -o option!\n");
					return -1;
				}
			default:
				fprintf(stdout, "Unknown option : %s\n", argv[i]);
				/* Show usage */
				fputs(usage, stdout);
				return -1;
				
		}
	}
	
	if (debug > 3){
		fprintf(stdout, "Running with parameters:\n");
		fprintf(stdout, "\tdoset=%d\n", doset);	
		fprintf(stdout, "\tdocalibrate=%d\n", docalibrate);	
		fprintf(stdout, "\tdodump=%d\n", dodump);	
		fprintf(stdout, "\tprint best settings=%d\n", printbest);
		fprintf(stdout, "\tstartspan=%d\n", startspan);
		fprintf(stdout, "\tstartchan=%d\n", startchan);	
		fprintf(stdout, "\tstopspan=%d\n", stopspan);
		fprintf(stdout, "\tstopchan=%d\n", stopchan);	
		fprintf(stdout, "\tcalibtype=%d\n", calibtype);	
		fprintf(stdout, "\twaveformtype=%d\n", waveformtype);	
		fprintf(stdout, "\tdelaytosilence=%d\n", delaytosilence);	
		fprintf(stdout, "\tsilencegoodfor=%d\n", silencegoodfor);	
		fprintf(stdout, "\tdialstr=%s\n", dialstr);	
		fprintf(stdout, "\tdebug=%d\n", debug);	
		fprintf(stdout, "\tconfigfile=%s\n", configfile);	
	}

	if(use_table) {
		init_sinetable();
	}
	
	if (docalibrate){
		res = do_calibrate_many(startspan, startchan, stopspan, stopchan, calibtype, configfile, dialstr, delaytosilence, silencegoodfor);
		if (!res)
			return do_set(configfile);	
		else
			return -1;
	}

	if (doset)
		return do_set(configfile);
				
	if (dodump){
		res = do_dump(startspan, startchan, dialstr, delaytosilence, silencegoodfor, waveformtype);
		if (!res)
			return 0;
		else
			return -1;
	}

	fputs(usage, stdout);
	return -1;
}
