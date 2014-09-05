#ifndef __FSK_CALLER_ID_H_
#define __FSK_CALLER_ID_H_

#include "wp_fsk.h"
#include "wp_libteletone_generate.h"

#define my_copy_string(x,y,z) strncpy(x, y, z - 1) 

#define FSK_MOD_FACTOR 0x10000

#define u8 	unsigned char
#define u16	unsigned short
#define s16	signed short


#ifdef __cplusplus
extern "C" {	/* for C++ users */
#endif

typedef enum {
	CID_TYPE_SDMF = 0x04,
	CID_TYPE_MDMF = 0x80
} cid_type_t;

typedef enum {
	MY_ENDIAN_BIG = 1,
	MY_ENDIAN_LITTLE = -1
} endian_t;

typedef struct fsk_data_state fsk_data_state_t;
typedef struct buffer buffer_t;
typedef struct bitstream bitstream_t;
typedef struct fsk_modulator fsk_modulator_t;
typedef int (*fsk_write_sample_t) (short *buf, unsigned int buflen, void *user_data);

typedef enum {
	MDMF_DATETIME = 1,
	MDMF_PHONE_NUM = 2,
	MDMF_DDN = 3,
	MDMF_NO_NUM = 4,
	MDMF_PHONE_NAME = 7,
	MDMF_NO_NAME = 8,
	MDMF_ALT_ROUTE = 9,
	MDMF_INVALID = 10
} mdmf_type_t;

struct bitstream {
	uint8_t *data;
	uint32_t datalen;
	uint32_t byte_index;
	uint8_t bit_index;
	int8_t endian;
	uint8_t top;
	uint8_t bot;
	uint8_t ss;
	uint8_t ssv;
};

struct fsk_modulator {
	teletone_dds_state_t dds;
	bitstream_t bs;
	uint32_t carrier_bits_start;
	uint32_t carrier_bits_stop;
	uint32_t chan_sieze_bits;
	uint32_t bit_factor;
	uint32_t bit_accum;
	uint32_t sample_counter;
	int32_t samples_per_bit;
	int32_t est_bytes;
	fsk_modem_types_t modem_type;
	fsk_data_state_t *fsk_data;
	fsk_write_sample_t write_sample_callback;
	void *user_data;
	int16_t sample_buffer[64];
};

struct fsk_data_state {
	dsp_fsk_handle_t *fsk1200_handle;
	uint8_t init;
	uint8_t *buf;
	size_t bufsize;
	size_t blen;
	size_t bpos;
	size_t dlen;
	size_t ppos;
	int checksum;
};


struct buffer {
	unsigned char *data;
	unsigned char *head;
	size_t used;
	size_t actually_used;
	size_t datalen;
	size_t max_len;
	size_t blocksize;
	unsigned int flags;
	unsigned id;
	int loops;
};

typedef enum {
	CODEC_TYPE_SLIN = 0,
	CODEC_TYPE_ALAW,
	CODEC_TYPE_ULAW
} codec_type_t;


typedef struct helper {
	int fd;
	int wrote;
	buffer_t *fsk_buffer;
	buffer_t *dtmf_buffer;
}helper_t;



static __inline__ char *my_clean_string(char *s)
{
	char *p;

	for (p = s; p && *p; p++) {
		uint8_t x = (uint8_t) *p;
		if (x < 32 || x > 127) {
			*p = ' ';
		}
	}

	return s;
}

int slin2ulaw(void* data, size_t max, size_t *datalen);
void convert_ulaw_to_linear(u16 *linear_buffer, u8 *ulaw_buffer, int size);



int fsk_modulator_init(fsk_modulator_t *fsk_trans,
									fsk_modem_types_t modem_type,
									uint32_t sample_rate,
									fsk_data_state_t *fsk_data,
									float db_level,
									uint32_t carrier_bits_start,
									uint32_t carrier_bits_stop,
									uint32_t chan_sieze_bits,
									fsk_write_sample_t write_sample_callback,
									void *user_data);

void fsk_modulator_send_all(fsk_modulator_t *fsk_trans);

int fsk_demod_feed(fsk_data_state_t *state, int16_t *data, int samples);
int fsk_demod_init(fsk_data_state_t *state, int rate, uint8_t *buf, size_t bufsize);
int fsk_demod_destroy(fsk_data_state_t *state);

void fsk_byte_handler (void *x, int data);
int fsk_data_parse(fsk_data_state_t *state, size_t *type, char **data, size_t *len);
int fsk_data_init(fsk_data_state_t *state, uint8_t *data, uint32_t datalen);
char* decode_msg_type(int type);
int fsk_data_add_mdmf(fsk_data_state_t *state, mdmf_type_t type, const uint8_t *data, uint32_t datalen);
int fsk_data_add_sdmf(fsk_data_state_t *state, const char *date, char *number);
int fsk_data_add_checksum(fsk_data_state_t *state);
int my_write_sample(int16_t *buf, size_t buflen, void *user_data);
size_t buffer_write(buffer_t *buffer, char *data, unsigned int datalen);

int buffer_create(buffer_t **buffer, size_t blocksize, size_t start_len, size_t max_len);
int buffer_create_dynamic(buffer_t **buffer, size_t blocksize, size_t start_len, size_t max_len);
void buffer_zero(buffer_t *buffer);

int fsk_write_sample(short *buf, unsigned int buflen, void *user_data);


#ifdef __cplusplus
}
#endif


#endif /* __FSK_CALLER_ID_H_ */
