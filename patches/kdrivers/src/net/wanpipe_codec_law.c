/**************************************************************************
 * wanpipe_codec_law.c	WANPIPE(tm) Multiprotocol WAN Link Driver. 
 *				TDM voice board configuration.
 *
 * Author: 	Nenad Corbic  <ncorbic@sangoma.com>
 *
 * Copyright:	(c) 1995-2005 Sangoma Technologies Inc.
 *
 *		This program is free software; you can redistribute it and/or
 *		modify it under the terms of the GNU General Public License
 *		as published by the Free Software Foundation; either version
 *		2 of the License, or (at your option) any later version.
 ******************************************************************************
 */
/*
 ******************************************************************************
			   INCLUDE FILES
 ******************************************************************************
*/


#include "wanpipe_includes.h"
#include "wanpipe_defines.h"
#include "wanpipe_debug.h"
#include "wanpipe_common.h"
#include "wanpipe.h"
#include "wanpipe_codec.h"


#if (defined __WINDOWS__)
# define __init
# define inline __inline
#endif


/* MULAW table */
static unsigned short u2s[] = {
 /*Part 2 - this is the only part used. The 2 parts are switched
   from it's original locations because we only intrested in
   absolute values, not interested in sign.
 */
 0x7D7C, 0x797C, 0x757C, 0x717C, 0x6D7C, 0x697C, 0x657C, 0x617C,
 0x5D7C, 0x597C, 0x557C, 0x517C, 0x4D7C, 0x497C, 0x457C, 0x417C,
 0x3E7C, 0x3C7C, 0x3A7C, 0x387C, 0x367C, 0x347C, 0x327C, 0x307C,
 0x2E7C, 0x2C7C, 0x2A7C, 0x287C, 0x267C, 0x247C, 0x227C, 0x207C,
 0x1EFC, 0x1DFC, 0x1CFC, 0x1BFC, 0x1AFC, 0x19FC, 0x18FC, 0x17FC,
 0x16FC, 0x15FC, 0x14FC, 0x13FC, 0x12FC, 0x11FC, 0x10FC, 0x0FFC,
 0x0F3C, 0x0EBC, 0x0E3C, 0x0DBC, 0x0D3C, 0x0CBC, 0x0C3C, 0x0BBC,
 0x0B3C, 0x0ABC, 0x0A3C, 0x09BC, 0x093C, 0x08BC, 0x083C, 0x07BC,
 0x075C, 0x071C, 0x06DC, 0x069C, 0x065C, 0x061C, 0x05DC, 0x059C,
 0x055C, 0x051C, 0x04DC, 0x049C, 0x045C, 0x041C, 0x03DC, 0x039C,
 0x036C, 0x034C, 0x032C, 0x030C, 0x02EC, 0x02CC, 0x02AC, 0x028C,
 0x026C, 0x024C, 0x022C, 0x020C, 0x01EC, 0x01CC, 0x01AC, 0x018C,
 0x0174, 0x0164, 0x0154, 0x0144, 0x0134, 0x0124, 0x0114, 0x0104,
 0x00F4, 0x00E4, 0x00D4, 0x00C4, 0x00B4, 0x00A4, 0x0094, 0x0084,
 0x0078, 0x0070, 0x0068, 0x0060, 0x0058, 0x0050, 0x0048, 0x0040,
 0x0038, 0x0030, 0x0028, 0x0020, 0x0018, 0x0010, 0x0008, 0x0000,

 /*Part 1*/
 0x8284, 0x8684, 0x8A84, 0x8E84, 0x9284, 0x9684, 0x9A84, 0x9E84,
 0xA284, 0xA684, 0xAA84, 0xAE84, 0xB284, 0xB684, 0xBA84, 0xBE84,
 0xC184, 0xC384, 0xC584, 0xC784, 0xC984, 0xCB84, 0xCD84, 0xCF84,
 0xD184, 0xD384, 0xD584, 0xD784, 0xD984, 0xDB84, 0xDD84, 0xDF84,
 0xE104, 0xE204, 0xE304, 0xE404, 0xE504, 0xE604, 0xE704, 0xE804,
 0xE904, 0xEA04, 0xEB04, 0xEC04, 0xED04, 0xEE04, 0xEF04, 0xF004,
 0xF0C4, 0xF144, 0xF1C4, 0xF244, 0xF2C4, 0xF344, 0xF3C4, 0xF444,
 0xF4C4, 0xF544, 0xF5C4, 0xF644, 0xF6C4, 0xF744, 0xF7C4, 0xF844,
 0xF8A4, 0xF8E4, 0xF924, 0xF964, 0xF9A4, 0xF9E4, 0xFA24, 0xFA64,
 0xFAA4, 0xFAE4, 0xFB24, 0xFB64, 0xFBA4, 0xFBE4, 0xFC24, 0xFC64,
 0xFC94, 0xFCB4, 0xFCD4, 0xFCF4, 0xFD14, 0xFD34, 0xFD54, 0xFD74,
 0xFD94, 0xFDB4, 0xFDD4, 0xFDF4, 0xFE14, 0xFE34, 0xFE54, 0xFE74,
 0xFE8C, 0xFE9C, 0xFEAC, 0xFEBC, 0xFECC, 0xFEDC, 0xFEEC, 0xFEFC,
 0xFF0C, 0xFF1C, 0xFF2C, 0xFF3C, 0xFF4C, 0xFF5C, 0xFF6C, 0xFF7C,
 0xFF88, 0xFF90, 0xFF98, 0xFFA0, 0xFFA8, 0xFFB0, 0xFFB8, 0xFFC0,
 0xFFC8, 0xFFD0, 0xFFD8, 0xFFE0, 0xFFE8, 0xFFF0, 0xFFF8, 0x0000
};

/* ALAW table */
static unsigned short a2s[] = {
/* part 2 (positive) */
0x1500, 0x1400, 0x1700, 0x1600, 0x1100, 0x1000, 0x1300, 0x1200, 
0x1D00, 0x1C00, 0x1F00, 0x1E00, 0x1900, 0x1800, 0x1B00, 0x1A00, 
0x0A80, 0x0A00, 0x0B80, 0x0B00, 0x0880, 0x0800, 0x0980, 0x0900, 
0x0E80, 0x0E00, 0x0F80, 0x0F00, 0x0C80, 0x0C00, 0x0D80, 0x0D00, 
0x5400, 0x5000, 0x5C00, 0x5800, 0x4400, 0x4000, 0x4C00, 0x4800, 
0x7400, 0x7000, 0x7C00, 0x7800, 0x6400, 0x6000, 0x6C00, 0x6800, 
0x2A00, 0x2800, 0x2E00, 0x2C00, 0x2200, 0x2000, 0x2600, 0x2400, 
0x3A00, 0x3800, 0x3E00, 0x3C00, 0x3200, 0x3000, 0x3600, 0x3400, 
0x0150, 0x0140, 0x0170, 0x0160, 0x0110, 0x0100, 0x0130, 0x0120, 
0x01D0, 0x01C0, 0x01F0, 0x01E0, 0x0190, 0x0180, 0x01B0, 0x01A0, 
0x0050, 0x0040, 0x0070, 0x0060, 0x0010, 0x0000, 0x0030, 0x0020, 
0x00D0, 0x00C0, 0x00F0, 0x00E0, 0x0090, 0x0080, 0x00B0, 0x00A0, 
0x0540, 0x0500, 0x05C0, 0x0580, 0x0440, 0x0400, 0x04C0, 0x0480, 
0x0740, 0x0700, 0x07C0, 0x0780, 0x0640, 0x0600, 0x06C0, 0x0680, 
0x02A0, 0x0280, 0x02E0, 0x02C0, 0x0220, 0x0200, 0x0260, 0x0240,
0x03A0, 0x0380, 0x03E0, 0x03C0, 0x0320, 0x0300, 0x0360, 0x0340,

/* part 1 (negative) */
0xEB00, 0xEC00, 0xE900, 0xEA00, 0xEF00, 0xF000, 0xED00, 0xEE00,
0xE300, 0xE400, 0xE100, 0xE200, 0xE700, 0xE800, 0xE500, 0xE600, 
0xF580, 0xF600, 0xF480, 0xF500, 0xF780, 0xF800, 0xF680, 0xF700, 
0xF180, 0xF200, 0xF080, 0xF100, 0xF380, 0xF400, 0xF280, 0xF300, 
0xAC00, 0xB000, 0xA400, 0xA800, 0xBC00, 0xC000, 0xB400, 0xB800, 
0x8C00, 0x9000, 0x8400, 0x8800, 0x9C00, 0xA000, 0x9400, 0x9800, 
0xD600, 0xD800, 0xD200, 0xD400, 0xDE00, 0xE000, 0xDA00, 0xDC00, 
0xC600, 0xC800, 0xC200, 0xC400, 0xCE00, 0xD000, 0xCA00, 0xCC00, 
0xFEB0, 0xFEC0, 0xFE90, 0xFEA0, 0xFEF0, 0xFF00, 0xFED0, 0xFEE0, 
0xFE30, 0xFE40, 0xFE10, 0xFE20, 0xFE70, 0xFE80, 0xFE50, 0xFE60, 
0xFFB0, 0xFFC0, 0xFF90, 0xFFA0, 0xFFF0, 0x0000, 0xFFD0, 0xFFE0, 
0xFF30, 0xFF40, 0xFF10, 0xFF20, 0xFF70, 0xFF80, 0xFF50, 0xFF60, 
0xFAC0, 0xFB00, 0xFA40, 0xFA80, 0xFBC0, 0xFC00, 0xFB40, 0xFB80, 
0xF8C0, 0xF900, 0xF840, 0xF880, 0xF9C0, 0xFA00, 0xF940, 0xF980, 
0xFD60, 0xFD80, 0xFD20, 0xFD40, 0xFDE0, 0xFE00, 0xFDA0, 0xFDC0, 
0xFC60, 0xFC80, 0xFC20, 0xFC40, 0xFCE0, 0xFD00, 0xFCA0, 0xFCC0
};

static u_char __wp_lin2mu[16384];
static u_char __wp_lin2a[16384];
static short __wp_mulaw[256];
static short __wp_alaw[256];


#define ZEROTRAP    /* turn on the trap as per the MIL-STD */
#define BIAS 0x84   /* define the add-in bias for 16 bit samples */
#define CLIP 32635


static unsigned char  __init
__wp_lineartoulaw(short sample)
{
  static int exp_lut[256] = {0,0,1,1,2,2,2,2,3,3,3,3,3,3,3,3,
                             4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
                             5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
                             5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
                             6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
                             6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
                             6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
                             6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
                             7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
                             7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
                             7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
                             7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
                             7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
                             7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
                             7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
                             7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7};
  int sign, exponent, mantissa;
  unsigned char ulawbyte;

  /* Get the sample into sign-magnitude. */
  sign = (sample >> 8) & 0x80;          /* set aside the sign */
  if (sign != 0) sample = -sample;              /* get magnitude */
  if (sample > CLIP) sample = CLIP;             /* clip the magnitude */

  /* Convert from 16 bit linear to ulaw. */
  sample = sample + BIAS;
  exponent = exp_lut[(sample >> 7) & 0xFF];
  mantissa = (sample >> (exponent + 3)) & 0x0F;
  ulawbyte = ~(sign | (exponent << 4) | mantissa);
#ifdef ZEROTRAP
  if (ulawbyte == 0) ulawbyte = 0x02;   /* optional CCITT trap */
#endif
  if (ulawbyte == 0xff) ulawbyte = 0x7f;   /* never return 0xff */
  return(ulawbyte);
}

#define AMI_MASK 0x55

static inline unsigned char __init
__wp_lineartoalaw (short linear)
{
    int mask;
    int seg;
    int pcm_val;
    static int seg_end[8] =
    {
         0xFF, 0x1FF, 0x3FF, 0x7FF, 0xFFF, 0x1FFF, 0x3FFF, 0x7FFF
    };
    
    pcm_val = linear;
    if (pcm_val >= 0)
    {
        /* Sign (7th) bit = 1 */
        mask = AMI_MASK | 0x80;
    }
    else
    {
        /* Sign bit = 0 */
        mask = AMI_MASK;
        pcm_val = -pcm_val;
    }

    /* Convert the scaled magnitude to segment number. */
    for (seg = 0;  seg < 8;  seg++)
    {
        if (pcm_val <= seg_end[seg])
	    break;
    }
    /* Combine the sign, segment, and quantization bits. */
    return  ((seg << 4) | ((pcm_val >> ((seg)  ?  (seg + 3)  :  4)) & 0x0F)) ^ mask;
}


static inline short int __init wp_alaw2linear (uint8_t alaw)
{
    int i;
    int seg;

    alaw ^= AMI_MASK;
    i = ((alaw & 0x0F) << 4);
    seg = (((int) alaw & 0x70) >> 4);
    if (seg)
        i = (i + 0x100) << (seg - 1);
    return (short int) ((alaw & 0x80)  ?  i  :  -i);
}



static int wanpipe_codec_convert_law2s(u8 *data, 
				       int len,
		               u16 *buf, 
				       u32 *power_ptr, 
			               enum wan_codec_source_format src_codec,
				       u8 *gain,
				       u8 usr)
{
	int 	i;
	u16 	power=0;
	short   *codec;
	unsigned short *pcodec;

	len  = len*2;

	if (src_codec == WP_MULAW){
		codec=__wp_mulaw;	
		pcodec=u2s;
	}else{
		codec=__wp_alaw;
		pcodec=a2s;
	}

	for (i=0;i<len;i++){
		if (gain) {
                	data[i]=gain[data[i]];		
		}
		if (usr) {
#if defined(__WINDOWS__)
			DEBUG_ERROR("%s(): Error: copy_to_user() is NOT supported!!\n",
				__FUNCTION__);
#else
			int err;
			err=copy_to_user(&buf[i],&codec[ data[i] ],sizeof(u16));
#endif
		} else {	
			buf[i]=codec[ data[i] ];
		}	
		power+=pcodec[ data[i] & 0x7F ];		
	}

	power = (u16)(power/len);

	if (power_ptr){
		*power_ptr=power;
	}

	return len;
}

static int wanpipe_codec_convert_s2law(u16 *data,
				       int len, 
			     	       u8  *buf, 
			     	       enum wan_codec_source_format src_codec,
				       u8 *gain,
				       int usr)
{
	int 	i;
	u8      *codec;
	u16 	kdata;

	len  = len/2;

	if (src_codec == WP_MULAW){
		codec=__wp_lin2mu;	
	}else{
		codec=__wp_lin2a;
	}

	for (i=0;i<len;i++){
		if (usr) {
#if defined(__WINDOWS__)
			DEBUG_ERROR("%s(): Error: copy_from_user() is NOT supported!!\n",
				__FUNCTION__);
#else
			int err;
			err=copy_from_user(&kdata,&data[i],sizeof(u16));
			buf[i] = codec[kdata>>2];
#endif
		}else{
			buf[i] = codec[data[i]>>2];
		}	
		if (gain) {
                	buf[i]=gain[buf[i]];
		}
	}

	return len;
}



/************* PUBLIC FUNCTIONS *********************/

int wanpipe_codec_convert_ulaw_2_s(u8 *data, 
				       int len,
			               u16 *buf, 
				       u32 *power_ptr, 
				       u8 *gain,
				       u8 usr)
{
	return wanpipe_codec_convert_law2s(data, 
				       	   len,
			                   buf, 
				           power_ptr,  
			     		   WP_MULAW,
					   gain,
					   usr);
}

int wanpipe_codec_convert_alaw_2_s(u8 *data, 
				   int len,
			           u16 *buf, 
				   u32 *power_ptr,
				   u8  *gain,
				   u8 usr)
{
	return wanpipe_codec_convert_law2s(data, 
				           len,
		                   buf, 
				           power_ptr, 
			     		   WP_ALAW,
					   gain,
					   usr);
}


int wanpipe_codec_convert_s_2_ulaw(u16 *data,
				   int len, 
			     	   u8  *buf,
				   u8  *gain,
				   u8 usr)
{
	return wanpipe_codec_convert_s2law(data, 
		     		len, 
					buf,
			     		WP_MULAW,
					gain,
					usr);
}

int wanpipe_codec_convert_s_2_alaw(u16 *data,
				   int len, 
			     	   u8  *buf,
				   u8  *gain,
				   u8 usr)
{
	return wanpipe_codec_convert_s2law(data,
				   	   len, 
			     	   	   buf,
			     		   WP_ALAW,
					   gain,
					   usr);
}

#ifdef __LINUX__
__init int wanpipe_codec_law_init(void)
#else
int wanpipe_codec_law_init(void)
#endif
{
	int i;

	for(i = -32768; i < 32768; i += 4) {
		__wp_lin2mu[((unsigned short)(short)i) >> 2] = __wp_lineartoulaw((short)i);
		__wp_lin2a[((unsigned short)(short)i) >> 2] = __wp_lineartoalaw((short)i);
	}

	for(i = 0;i < 256;i++) {
		short mu,e,f,y;
		static short etab[]={0,132,396,924,1980,4092,8316,16764};

		mu = 255-i;
		e = (mu & 0x70)/16;
		f = mu & 0x0f;
		y = f * (1 << (e + 3));
		y += etab[e];
		if (mu & 0x80) y = -y;
		__wp_mulaw[i] = y;
		__wp_alaw[i] = wp_alaw2linear((uint8_t)i);
	}

	return 0;
}

/*! \brief Decode an u-law sample to a linear value.
    \param ulaw The u-law sample to decode.
    \return The linear value.
*/
int wanpipe_codec_get_ulaw_to_linear(u8 ulaw)
{
    int t;

    /* Complement to obtain normal u-law value. */
    ulaw = ~ulaw;
    /*
     * Extract and bias the quantization bits. Then
     * shift up by the segment number and subtract out the bias.
     */
    t = (((ulaw & 0x0F) << 3) + G711_ULAW_BIAS) << (((int) ulaw & 0x70) >> 4);
    return  (int16_t) ((ulaw & 0x80)  ?  (G711_ULAW_BIAS - t)  :  (t - G711_ULAW_BIAS));
}

int wanpipe_codec_get_alaw_to_linear(u8 alaw)
{
    int i;
    int seg;

    alaw ^= G711_ALAW_AMI_MASK;
    i = ((alaw & 0x0F) << 4);
    seg = (((int) alaw & 0x70) >> 4);
    if (seg)
        i = (i + 0x108) << (seg - 1);
    else
        i += 8;
    return (int) ((alaw & 0x80)  ?  i  :  -i);
}
