#include "libstelephony_tone.h"
#include "wp_libteletone_generate.h" /* TODO: May not need this one */
#include "wp_g711.h"

extern int16_t TELETONE_SINES[SINE_TABLE_MAX];


#define test_flag(obj, flag) ((obj)->flags & flag)
#define set_flag(obj, flag) (obj)->flags |= (flag)

typedef enum {
        BUFFER_FLAG_DYNAMIC = (1 << 0)
} buffer_flag_t;


int slin2alaw(void* data, size_t max, size_t *datalen);
int slin2ulaw(void* data, size_t max, size_t *datalen);

int buffer_id = 0;

size_t stelephony_buffer_inuse(void *pbuffer);
void bitstream_init(bitstream_t *bsp, uint8_t *data, uint32_t datalen, endian_t endian, uint8_t ss);
int fsk_modulator_init(fsk_modulator_t *, fsk_modem_types_t, uint32_t ,fsk_data_state_t*,float , uint32_t, uint32_t, uint32_t, fsk_write_sample_t ,void*);
int8_t bitstream_get_bit(bitstream_t *bsp);
void fsk_modulator_send_data(fsk_modulator_t *fsk_trans);
size_t fsk_modulator_generate_bit(fsk_modulator_t *fsk_trans, int8_t bit, int16_t *buf, size_t buflen);
size_t stelephony_buffer_read(void *pbuffer, void *data, size_t datalen);
size_t stelephony_buffer_read_ulaw(void *pbuffer, unsigned char *data, int* dlen, int max);
size_t stelephony_buffer_read_alaw(void *pbuffer, unsigned char *data, int* dlen, int max);
void fsk_modulator_generate_chan_sieze(fsk_modulator_t *fsk_trans);

int fsk_write_sample(short *buf, unsigned int buflen, void *user_data)
{
	helper_t *foo = (struct helper *) user_data;
	

	buffer_write(foo->fsk_buffer, (char*) buf, buflen*2);	
	return 0;
}


size_t stelephony_buffer_inuse(void *pbuffer)
{
	buffer_t *buffer=(buffer_t*)pbuffer;
	assert(buffer != NULL);

	return buffer->used;
}


int fsk_data_init(fsk_data_state_t *state, uint8_t *data, uint32_t datalen)
{
	memset(state, 0, sizeof(*state));
	state->buf = data;
	state->bufsize = datalen;
	state->bpos = 2;

	return 0;
}

int fsk_demod_init(fsk_data_state_t *state, int rate, uint8_t *buf, size_t bufsize)
{

	dsp_fsk_attr_t fsk1200_attr;
	
	if (state->fsk1200_handle) {
		dsp_fsk_destroy(&state->fsk1200_handle);
	}

	memset(state, 0, sizeof(*state));
	memset(buf, 0, bufsize);
	state->buf = buf;
	state->bufsize = bufsize;
	
	dsp_fsk_attr_init (&fsk1200_attr);
	dsp_fsk_attr_set_samplerate (&fsk1200_attr, rate);
	dsp_fsk_attr_set_bytehandler (&fsk1200_attr, fsk_byte_handler, state);
	state->fsk1200_handle = dsp_fsk_create (&fsk1200_attr);

	if (state->fsk1200_handle == NULL) {
		return -1;
	}

	return 0;
}

int fsk_demod_destroy(fsk_data_state_t *state)
{
	if (state->fsk1200_handle) {
		dsp_fsk_destroy(&state->fsk1200_handle);
	}
	memset(state, 0, sizeof(*state));
	return 0;
}


int fsk_data_add_sdmf(fsk_data_state_t *state, const char *date, char *number)
{
	size_t dlen = strlen(date);
	size_t nlen = strlen(number);

	state->buf[0] = CID_TYPE_SDMF;
	memcpy(&state->buf[state->bpos], date, dlen);
	state->bpos += dlen;
	memcpy(&state->buf[state->bpos], number, nlen);
	state->bpos += nlen;

	return 0;
}

int fsk_data_add_mdmf(fsk_data_state_t *state, mdmf_type_t type, const uint8_t *data, uint32_t datalen)
{
	state->buf[0] = CID_TYPE_MDMF;
	state->buf[state->bpos++] = type;
	state->buf[state->bpos++] = (uint8_t)datalen;
	memcpy(&state->buf[state->bpos], data, datalen);
	state->bpos += datalen;
	return 0;
}

int fsk_data_add_checksum(fsk_data_state_t *state)
{
	uint32_t i;
	uint8_t check = 0;

	state->buf[1] = (uint8_t)(state->bpos - 2);

	for (i = 0; i < state->bpos; i++) {
		check = check + state->buf[i];
	}

	state->checksum = state->buf[state->bpos] = 256 - check;
	state->bpos++;

	state->dlen = state->bpos;
	state->blen = state->buf[1];

	return 0;
}



void fsk_byte_handler (void *x, int data)
{
	fsk_data_state_t *state = (fsk_data_state_t *) x;
	uint8_t byte = (uint8_t)data;

 top:
	if (state->init == 3) {
		return;
	}

	if (state->dlen) {
		goto add_byte;
	}
	
	if (state->bpos == 1) {
		state->blen = byte;

		if ((uint32_t)(state->dlen = state->bpos + byte + 2) > state->bufsize) {
			state->dlen = state->bufsize;
		}
		goto top;
	}

 add_byte:

	if (state->bpos <= state->dlen) {
		state->buf[state->bpos++] = byte;
	} else {
		state->init = 3;
	}
}


int fsk_demod_feed(fsk_data_state_t *state, int16_t *data, int samples)
{
	int x;
	int16_t *sp = data;

	if (state->init == 3) {
		return 0;
	}

	for (x = 0; x < samples; x++) {
		dsp_fsk_sample (state->fsk1200_handle, (double) *sp++ / 32767.0);
		if (state->dlen && state->bpos >= state->dlen) {
			state->init = 3;
			return -1;
		}
	}

	return 0;
}


int fsk_data_parse(fsk_data_state_t *state, size_t *type, char **data, size_t *len)
{

	size_t i;
	int sum = 0;
	
top:

	if (state->checksum != 0 || state->ppos >= state->dlen - 1) {
		return -1;
	}

	if (!state->ppos) {
		for(i = 0; i < state->bpos; i++) {
			sum += state->buf[i];
		}
		state->checksum = sum % 256;
		state->ppos = 2;		

		if (state->buf[0] != CID_TYPE_MDMF && state->buf[0] != CID_TYPE_SDMF) {
			state->checksum = -1;
		}
		goto top;
	}

	if (state->buf[0] == CID_TYPE_SDMF) {
		/* convert sdmf to mdmf so we don't need 2 parsers */
		if (state->ppos == 2) {
			*type = MDMF_DATETIME;
			*len = 8;
		} else {
			if (state->buf[state->ppos] == 'P' || state->buf[state->ppos] == 'O') {
				*type = MDMF_NO_NUM;
				*len = 1;
			} else {
				*type = MDMF_PHONE_NUM;
				*len = state->blen - 8;
			}
		}
		*data = (char *)&state->buf[state->ppos];
		state->ppos += *len;		
		return 0;
	} else if (state->buf[0] == CID_TYPE_MDMF) {
		*type = state->buf[state->ppos++];
		*len = state->buf[state->ppos++];
		*data = (char *)&state->buf[state->ppos];
		state->ppos += *len;
		return 0;
	}

	return -1;
}

#if 0
int cnt = 0;
int my_write_sample(int16_t *buf, size_t buflen, void *user_data)
{
	struct helper *foo = (struct helper *) user_data;
	
	buffer_write(foo->fsk_buffer, (char*) buf, buflen*2);
	return 0;
}
#endif

void bitstream_init(bitstream_t *bsp, uint8_t *data, uint32_t datalen, endian_t endian, uint8_t ss)
{
	memset(bsp, 0, sizeof(*bsp));
	bsp->data = data;
	bsp->datalen = datalen;
	bsp->endian = endian;
	bsp->ss = ss;
	
	if (endian < 0) {
		bsp->top = bsp->bit_index = 7;
		bsp->bot = 0;
	} else {
		bsp->top = bsp->bit_index = 0;
		bsp->bot = 7;
	}

}


int fsk_modulator_init(fsk_modulator_t *fsk_trans,
									fsk_modem_types_t modem_type,
									uint32_t sample_rate,
									fsk_data_state_t *fsk_data,
									float db_level,
									uint32_t carrier_bits_start,
									uint32_t carrier_bits_stop,
									uint32_t chan_sieze_bits,
									fsk_write_sample_t write_sample_callback,
									void *user_data)
{
	memset(fsk_trans, 0, sizeof(*fsk_trans));
	fsk_trans->modem_type = modem_type;
	teletone_dds_state_set_tone(&fsk_trans->dds, fsk_modem_definitions[fsk_trans->modem_type].freq_space, sample_rate, 0);
	teletone_dds_state_set_tone(&fsk_trans->dds, fsk_modem_definitions[fsk_trans->modem_type].freq_mark, sample_rate, 1);
	fsk_trans->bit_factor = (uint32_t)((fsk_modem_definitions[fsk_trans->modem_type].baud_rate * FSK_MOD_FACTOR) / (float)sample_rate);
	fsk_trans->samples_per_bit = (uint32_t) (sample_rate / fsk_modem_definitions[fsk_trans->modem_type].baud_rate);
	fsk_trans->est_bytes = (int32_t)(((fsk_data->dlen * 10) + carrier_bits_start + carrier_bits_stop + chan_sieze_bits) * ((fsk_trans->samples_per_bit + 1) * 2));
	fsk_trans->bit_accum = 0;
	fsk_trans->fsk_data = fsk_data;
	teletone_dds_state_set_tx_level(&fsk_trans->dds, db_level);
	
	bitstream_init(&fsk_trans->bs, fsk_trans->fsk_data->buf, (uint32_t)fsk_trans->fsk_data->dlen, MY_ENDIAN_BIG, 1);
	fsk_trans->carrier_bits_start = carrier_bits_start;
	fsk_trans->carrier_bits_stop = carrier_bits_stop;
	fsk_trans->chan_sieze_bits = chan_sieze_bits;
	fsk_trans->write_sample_callback = write_sample_callback;
	fsk_trans->user_data = user_data;

	return 0;
}

size_t fsk_modulator_generate_bit(fsk_modulator_t *fsk_trans, int8_t bit, int16_t *buf, size_t buflen)
{
	size_t i;
		
	for(i=0 ; i<buflen; i++) {
		fsk_trans->bit_accum += fsk_trans->bit_factor;
		if (fsk_trans->bit_accum >= FSK_MOD_FACTOR) {
			fsk_trans->bit_accum -= (FSK_MOD_FACTOR + fsk_trans->bit_factor);
			break;
		}
		
		buf[i] = teletone_dds_state_modulate_sample(&fsk_trans->dds, bit);
		
	}	
	return i;
}



void buffer_zero(buffer_t *buffer)
{
	assert(buffer != NULL);
	assert(buffer->data != NULL);

	buffer->used = 0;
	buffer->actually_used = 0;
	buffer->head = buffer->data;
}

int8_t bitstream_get_bit(bitstream_t *bsp)
{
	int8_t bit = -1;
	

	if (bsp->byte_index >= bsp->datalen) {
		goto done;
	}

	if (bsp->ss) {
		if (!bsp->ssv) {
			bsp->ssv = 1;
			return 0;
		} else if (bsp->ssv == 2) {
			bsp->byte_index++;
			bsp->ssv = 0;
			return 1;
		}
	}

	bit = (bsp->data[bsp->byte_index] >> (bsp->bit_index)) & 1;
	
	if (bsp->bit_index == bsp->bot) {
		bsp->bit_index = bsp->top;
		if (bsp->ss) {
			bsp->ssv = 2;
			goto done;
		} 

		if (++bsp->byte_index > bsp->datalen) {
			bit = -1;
			goto done;
		}
		
	} else {
		bsp->bit_index = bsp->bit_index + bsp->endian;
	}


 done:
	return bit;
}



void fsk_modulator_send_data(fsk_modulator_t *fsk_trans)
{
	size_t r = 0;
	int8_t bit = 0;
	
	while((bit = bitstream_get_bit(&fsk_trans->bs)) > -1) {
		if ((r = fsk_modulator_generate_bit(fsk_trans, bit, fsk_trans->sample_buffer, sizeof(fsk_trans->sample_buffer) / 2))) {
			if (fsk_trans->write_sample_callback(fsk_trans->sample_buffer, r, fsk_trans->user_data) != 0) {
				break;
			}
		} else {
			break;
		}
	}
}



int32_t fsk_modulator_generate_carrier_bits(fsk_modulator_t *fsk_trans, uint32_t bits)
{
	uint32_t i = 0;
	size_t r = 0;
	int8_t bit = 1;

	for (i = 0; i < bits; i++) {
		if ((r = fsk_modulator_generate_bit(fsk_trans, bit, fsk_trans->sample_buffer, sizeof(fsk_trans->sample_buffer) / 2))) {
			if (fsk_trans->write_sample_callback(fsk_trans->sample_buffer, r, fsk_trans->user_data) != 0) {
				break;
			}
		} else {
			break;
		}
	}

	return i;
}

void fsk_modulator_generate_chan_sieze(fsk_modulator_t *fsk_trans)
{
	uint32_t i = 0;
	size_t r = 0;
	int8_t bit = 0;
	
	for (i = 0; i < fsk_trans->chan_sieze_bits; i++) {
		r = fsk_modulator_generate_bit(fsk_trans, bit, fsk_trans->sample_buffer, sizeof(fsk_trans->sample_buffer) / 2);
	
		if (r) {
			if (fsk_trans->write_sample_callback(fsk_trans->sample_buffer, r, fsk_trans->user_data) != 0) {
				break;
			}
		} else {
			break;
		}
		bit = !bit;
	}
}	


void fsk_modulator_send_all(fsk_modulator_t *fsk_trans)
{
	fsk_modulator_generate_chan_sieze(fsk_trans);
	fsk_modulator_generate_carrier_bits(fsk_trans,fsk_trans->carrier_bits_start);
	fsk_modulator_send_data(fsk_trans);
	fsk_modulator_generate_carrier_bits(fsk_trans, fsk_trans->carrier_bits_stop);
}

size_t buffer_write(buffer_t *buffer, char *data, unsigned int datalen)
{
	size_t freespace, actual_freespace;

	assert(buffer != NULL);
	assert(data != NULL);
	assert(buffer->data != NULL);

	if (!datalen) {
		return buffer->used;
	}

	actual_freespace = buffer->datalen - buffer->actually_used;
	if (actual_freespace < datalen && (!buffer->max_len || (buffer->used + datalen <= buffer->max_len))) {
		memmove(buffer->data, buffer->head, buffer->used);
		buffer->head = buffer->data;
		buffer->actually_used = buffer->used;
	}

	freespace = buffer->datalen - buffer->used;

	/*
	  if (buffer->data != buffer->head) {
	  memmove(buffer->data, buffer->head, buffer->used);
	  buffer->head = buffer->data;
	  }
	*/
	
	if (freespace < datalen) {
		size_t new_size, new_block_size;
		void *data;
		
		new_size = buffer->datalen + datalen;
		new_block_size = buffer->datalen + buffer->blocksize;

		if (new_block_size > new_size) {
			new_size = new_block_size;
		}
		buffer->head = buffer->data;
		data = realloc(buffer->data, new_size);
		if (!data) {
			return 0;
		}
		buffer->data = (unsigned char*) data;
		buffer->head = buffer->data;
		buffer->datalen = new_size;
	}
	

	freespace = buffer->datalen - buffer->used;

	if (freespace < datalen) {
		return 0;
	} else {
		memcpy(buffer->head + buffer->used, data, datalen);
		buffer->used += datalen;
		buffer->actually_used += datalen;
	}
	/* if (buffer->id == 4) printf("%u i %d = %d\n", buffer->id, (unsigned)datalen, (unsigned)buffer->used); */

	return buffer->used;
}


size_t stelephony_buffer_read(void *pbuffer, void *data, size_t datalen)
{
	size_t reading = 0;
	buffer_t *buffer=(buffer_t*)pbuffer;

	assert(buffer != NULL);
	assert(data != NULL);


	if (buffer->used < 1) {
		buffer->used = 0;
		return 0;
	} else if (buffer->used >= datalen) {
		reading = datalen;
	} else {
		reading = buffer->used;
	}

	memcpy(data, buffer->head, reading);
	buffer->used -= reading;
	buffer->head += reading;

	/* if (buffer->id == 4) printf("%u o %d = %d\n", buffer->id, (unsigned)reading, (unsigned)buffer->used); */
	return reading;
}

size_t stelephony_buffer_read_ulaw(void *pbuffer, unsigned char *data, int* dlen, int max)
{
	int br;
	int len;

	*dlen*=2;

	len = *dlen;

	br = stelephony_buffer_read(pbuffer, data, len);
	if (br < *dlen) {
		memset(data+br, 0, (*dlen) - br);
	}
	
	slin2ulaw(data, max, (size_t*) dlen);
	
	return *dlen;	
}

size_t stelephony_buffer_read_alaw(void *buffer, unsigned char *data, int* dlen, int max)
{
	int br;
	int len;

	*dlen*=2;

	len = *dlen;

	br = stelephony_buffer_read(buffer, data, len);
	if (br < *dlen) {
		memset(data+br, 0, (*dlen) - br);
	}

	slin2alaw(data, max, (size_t*) dlen);
	return *dlen;	
}

int buffer_create_dynamic(buffer_t **buffer, size_t blocksize, size_t start_len, size_t max_len)
{
	buffer_t *new_buffer;

        if ((new_buffer = (buffer_t*)malloc(sizeof(*new_buffer)))) {
                memset(new_buffer, 0, sizeof(*new_buffer));

                if (start_len) {
                        if (!(new_buffer->data = (unsigned char*) malloc(start_len))) {
                                free(new_buffer);
                                return -1;
                        }
                        memset(new_buffer->data, 0, start_len);
                }

                new_buffer->max_len = max_len;
                new_buffer->datalen = start_len;
                new_buffer->id = buffer_id++;
                new_buffer->blocksize = blocksize;
                new_buffer->head = new_buffer->data;
                set_flag(new_buffer, BUFFER_FLAG_DYNAMIC);

                *buffer = new_buffer;
                return 0;
        }

        return -1;
}

int buffer_create(buffer_t **buffer, size_t blocksize, size_t start_len, size_t max_len)
{
	buffer_t *new_buffer;

	new_buffer = (buffer_t*) malloc(sizeof(*new_buffer));
	if (new_buffer) {
		memset(new_buffer, 0, sizeof(*new_buffer));

		if (start_len) {
			new_buffer->data = (unsigned char*) malloc(start_len);
			if (!new_buffer->data) {
				free(new_buffer);
				return -1;
			}
			memset(new_buffer->data, 0, start_len);
		}

		new_buffer->max_len = max_len;
		new_buffer->datalen = start_len;
		new_buffer->id = buffer_id++;
		new_buffer->blocksize = blocksize;
		new_buffer->head = new_buffer->data;

		*buffer = new_buffer;
		return 0;
	}

	return -1;
}

int slin2alaw(void* data, size_t max, size_t *datalen)
{
	int16_t sln_buf[512] = {0}, *sln = sln_buf;
	uint8_t *lp = (unsigned char*) data;
	uint32_t i;
	size_t len = *datalen;

	if (max > len) {
		max = len;
	}

	memcpy(sln, data, max);
	
	for(i = 0; i < max; i++) {
		*lp++ = linear_to_alaw(*sln++);
	}

	*datalen = max / 2;

	return 0;
}

int slin2ulaw(void* data, size_t max, size_t *datalen)
{
	int16_t sln_buf[512] = {0}, *sln = sln_buf;
	uint8_t *lp = (unsigned char*) data;
	uint32_t i;
	size_t len = *datalen;

	if (max > len) {
		max = len;
	}

	memcpy(sln, data, max);
	
	for(i = 0; i < max; i++) {
		*lp++ = linear_to_ulaw(*sln++);
	}

	*datalen = max / 2;

	return 0;
}

