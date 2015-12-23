/*
 * Copyright (c) 2001
 *	Alex Feldman <al.feldman@sangoma.com>.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by Alex Feldman.
 * 4. Neither the name of the author nor the names of any co-contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY Alex Feldman AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL Alex Feldman OR THE VOICES IN HIS HEAD
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 *
 *	$Id: sdla_te1_ds.h,v 1.15 2008-02-06 19:32:19 sangoma Exp $
 */

/*****************************************************************************
 * sdla_te1_ds.h	Sangoma TE1 configuration definitions (Dallas).
 *
 * Author:      Alex Feldman
 *
 * ============================================================================
 * Aprl 30, 2001	Alex Feldman	Initial version.
 ****************************************************************************
*/
#ifndef	__SDLA_TE1_DS_H
#    define	__SDLA_TE1_DS_H

/******************************************************************************
			  DEFINES AND MACROS
******************************************************************************/

/* Global Register Definitions */
#define REG_GTCR1		0xF0

#define REG_GFCR		0xF1
#define BIT_GFCR_TCBCS		0x02
#define BIT_GFCR_RCBCS		0x01

#define REG_GTCR2		0xF2

#define REG_GTCCR		0xF3

#define REG_GLSRR		0xF5

#define REG_GFSRR		0xF6

#define REG_IDR			0xF8
#define DEVICE_ID_DS26519       0xD9
#define DEVICE_ID_DS26528	0x0B
#define DEVICE_ID_DS26524	0x0C
#define DEVICE_ID_DS26522	0x0D
#define DEVICE_ID_DS26521	0x0E
#define DEVICE_ID_BAD		0x00
#define DEVICE_ID_SHIFT		3
#define DEVICE_ID_MASK		0x1F
#define DEVICE_ID_DS(dev_id)	((dev_id) >> DEVICE_ID_SHIFT) & DEVICE_ID_MASK
#define DECODE_CHIPID(chip_id)					\
		(chip_id == DEVICE_ID_DS26519) ? "DS26519" :	\
		(chip_id == DEVICE_ID_DS26528) ? "DS26528" :	\
		(chip_id == DEVICE_ID_DS26524) ? "DS26524" :	\
		(chip_id == DEVICE_ID_DS26522) ? "DS26522" :	\
		(chip_id == DEVICE_ID_DS26521) ? "DS26521" : "Unknown"

#define REG_GFISR		0xF9

#define REG_GBISR		0xFA

#define REG_GLISR		0xFB

#define REG_GFIMR		0xFC

#define REG_GBIMR		0xFD

#define REG_GLIMR		0xFE


/* RX Framer Register Definitions */
#define REG_RSIGC		0x13
#define BIT_RSIGC_CASMS		0x10

#define REG_T1RCR2		0x14
#define BIT_T1RCR2_RSLC96	0x10
#define BIT_T1RCR2_OOF2		0x08
#define BIT_T1RCR2_OOF1		0x04
#define BIT_T1RCR2_RAIIE	0x02
#define BIT_T1RCR2_RD4RM	0x01

#define REG_E1RSAIMR		0x14
#define BIT_E1RSAIMR_Rsa4IM	0x10
#define BIT_E1RSAIMR_Rsa5IM	0x08
#define BIT_E1RSAIMR_Rsa6IM	0x04
#define BIT_E1RSAIMR_Rsa7IM	0x02
#define BIT_E1RSAIMR_Rsa8IM	0x01

#define REG_T1RBOCC		0x15
#define BIT_T1RBOCC_RBR		0x80
#define BIT_T1RBOCC_RBD1	0x02
#define BIT_T1RBOCC_RBD0	0x01
#define BIT_T1RBOCC_RBF1	0x04
#define BIT_T1RBOCC_RBF0	0x02

#define REG_RS1			0x40
#define BIT_RS_A		0x08
#define BIT_RS_B		0x04
#define BIT_RS_C		0x02
#define BIT_RS_D		0x01
#define REG_RS2			0x41
#define REG_RS3			0x42
#define REG_RS4			0x43
#define REG_RS5			0x44
#define REG_RS6			0x45
#define REG_RS7			0x46
#define REG_RS8			0x47
#define REG_RS9			0x48
#define REG_RS10		0x49
#define REG_RS11		0x4A
#define REG_RS12		0x4B
#define REG_RS13		0x4C
#define REG_RS14		0x4D
#define REG_RS15		0x4E
#define REG_RS16		0x4F

#define REG_LCVCR1		0x50
#define REG_LCVCR2		0x51

#define REG_PCVCR1		0x52
#define REG_PCVCR2		0x53

#define REG_FOSCR1		0x54
#define REG_FOSCR2		0x55

#define REG_E1EBCR1		0x56
#define REG_E1EBCR2		0x57

#define REG_T1RBOC		0x63

#define REG_E1RNAF		0x65
#define BIT_E1RNAF_A		0x20

#define REG_SaBITS		0x6E
#define BIT_SaBITS_Sa4		0x10
#define BIT_SaBITS_Sa5		0x08
#define BIT_SaBITS_Sa6		0x04
#define BIT_SaBITS_Sa7		0x02
#define BIT_SaBITS_Sa8		0x01

#define REG_Sa6CODE		0x6F

#define REG_RMMR		0x80
#define BIT_RMMR_FRM_EN		0x80
#define BIT_RMMR_INIT_DONE	0x40
#define BIT_RMMR_SFTRST		0x02
#define BIT_RMMR_T1E1		0x01

#define REG_RCR1		0x81
#define BIT_RCR1_T1_SYNCT	0x80
#define BIT_RCR1_T1_RB8ZS	0x40
#define BIT_RCR1_T1_RFM		0x20
#define BIT_RCR1_T1_ARC		0x10
#define BIT_RCR1_T1_SYNCC	0x08
#define BIT_RCR1_T1_RJC		0x04
#define BIT_RCR1_T1_SYNCE	0x02
#define BIT_RCR1_T1_RESYNC	0x01
#define BIT_RCR1_E1_RHDB3	0x40
#define BIT_RCR1_E1_RSIGM	0x20
#define BIT_RCR1_E1_RG802	0x10
#define BIT_RCR1_E1_RCRC4	0x08
#define BIT_RCR1_E1_FRC		0x04
#define BIT_RCR1_E1_SYNCE	0x02
#define BIT_RCR1_E1_RESYNC	0x01

#define REG_T1RIBCC		0x82
#define BIT_T1RIBCC_RUP2	0x20
#define BIT_T1RIBCC_RUP1	0x10
#define BIT_T1RIBCC_RUP0	0x08
#define BIT_T1RIBCC_RDN2	0x04
#define BIT_T1RIBCC_RDN1	0x02
#define BIT_T1RIBCC_RDN0	0x01

#define REG_RCR3		0x83
#define BIT_RCR3_IDF		0x80
#define BIT_RCR3_RSERC		0x20
#define BIT_RCR3_PLB		0x02
#define BIT_RCR3_FLB		0x01

#define REG_RIOCR		0x84
#define BIT_RIOCR_RCLKINV	0x80
#define BIT_RIOCR_RSYNCINV	0x40
#define BIT_RIOCR_H100EN	0x20
#define BIT_RIOCR_RSCLKM	0x10
#define BIT_RIOCR_RSMS		0x08
#define BIT_RIOCR_RSIO		0x04
#define BIT_RIOCR_RSMS2		0x02
#define BIT_RIOCR_RSMS1		0x01

#define REG_RESCR		0x85
#define BIT_RESCR_RGCLKEN	0x40
#define BIT_RESCR_RESE		0x01

#define REG_ERCNT		0x86
#define BIT_ERCNT_1SECS		0x80
#define BIT_ERCNT_MCUS		0x40
#define BIT_ERCNT_MECU		0x20
#define BIT_ERCNT_ECUS		0x10
#define BIT_ERCNT_EAMS		0x08
#define BIT_ERCNT_FSBE		0x04
#define BIT_ERCNT_MOSCR		0x02
#define BIT_ERCNT_LCVCRF	0x01

#define REG_RXPC		0x8A
#define BIT_RXPC_RBPDIR		0x04
#define BIT_RXPC_RBPFUS		0x02
#define BIT_RXPC_RBPEN		0x01

#define REG_RLS1		0x90
#define BIT_RLS1_RRAIC		0x80
#define BIT_RLS1_RAISC		0x40
#define BIT_RLS1_RLOSC		0x20
#define BIT_RLS1_RLOFC		0x10
#define BIT_RLS1_RRAID		0x08
#define BIT_RLS1_RAISD		0x04
#define BIT_RLS1_RLOSD		0x02
#define BIT_RLS1_RLOFD		0x01

#define REG_RLS2		0x91
#define BIT_RLS2_T1_RPDV	0x80
#define BIT_RLS2_T1_COFA	0x20
#define BIT_RLS2_T1_8ZD		0x10
#define BIT_RLS2_T1_16ZD	0x08
#define BIT_RLS2_T1_SEFE	0x04
#define BIT_RLS2_T1_B8ZS	0x02
#define BIT_RLS2_T1_FBE		0x01
#define BIT_RLS2_E1_CRCRC	0x40
#define BIT_RLS2_E1_CASRC	0x20
#define BIT_RLS2_E1_FASRC	0x10
#define BIT_RLS2_E1_RSA1	0x08
#define BIT_RLS2_E1_RSA0	0x04
#define BIT_RLS2_E1_RCMF	0x02
#define BIT_RLS2_E1_RAF		0x01

#define REG_RLS3		0x92
#define BIT_RLS3_T1_LORCC	0x80
#define BIT_RLS3_T1_LSPC	0x40
#define BIT_RLS3_T1_LDNC	0x20
#define BIT_RLS3_T1_LUPC	0x10
#define BIT_RLS3_T1_LORCD	0x08
#define BIT_RLS3_T1_LSPD	0x04
#define BIT_RLS3_T1_LDND	0x02
#define BIT_RLS3_T1_LUPD	0x01
#define BIT_RLS3_E1_LORCC	0x80
#define BIT_RLS3_E1_V52LNKC	0x20
#define BIT_RLS3_E1_RDMAC	0x10
#define BIT_RLS3_E1_LORCD	0x08
#define BIT_RLS3_E1_V52LNKD	0x02
#define BIT_RLS3_E1_RDMAD	0x01

#define REG_RLS4		0x93
#define BIT_RLS4_RESF		0x80
#define BIT_RLS4_RESEM		0x40
#define BIT_RLS4_RSLIP		0x20
#define BIT_RLS4_RSCOS		0x08
#define BIT_RLS4_1SEC		0x04
#define BIT_RLS4_TIMER		0x02
#define BIT_RLS4_RMF		0x01

#define REG_RLS5		0x94
#define BIT_RLS5_ROVR		0x20
#define BIT_RLS5_RHOBT		0x10
#define BIT_RLS5_RPE		0x08
#define BIT_RLS5_RPS		0x03
#define BIT_RLS5_RHWMS		0x02
#define BIT_RLS5_RNES		0x01

#define REG_RLS6		0x95

#define REG_RLS7		0x96
#define BIT_RLS7_T1_RRAI_CI	0x20
#define BIT_RLS7_T1_RAIS_CI	0x10
#define BIT_RLS7_T1_RSLC96	0x08
#define BIT_RLS7_T1_RFDLF	0x04
#define BIT_RLS7_T1_BC		0x02
#define BIT_RLS7_T1_BD		0x01
#define BIT_RLS7_E1_Sa6CD	0x02
#define BIT_RLS7_E1_SaXCD	0x01

#define REG_RSS1		0x98
#define BIT_RSS1_CH1		0x80
#define BIT_RSS1_CH2		0x40
#define BIT_RSS1_CH3		0x20
#define BIT_RSS1_CH4		0x10
#define BIT_RSS1_CH5		0x08
#define BIT_RSS1_CH6		0x04
#define BIT_RSS1_CH7		0x02
#define BIT_RSS1_CH8		0x01

#define REG_RSS2		0x99
#define BIT_RSS2_CH9		0x80
#define BIT_RSS2_CH10		0x40
#define BIT_RSS2_CH11		0x20
#define BIT_RSS2_CH12		0x10
#define BIT_RSS2_CH13		0x08
#define BIT_RSS2_CH14		0x04
#define BIT_RSS2_CH15		0x02
#define BIT_RSS2_CH16		0x01

#define REG_RSS3		0x9A
#define BIT_RSS3_CH17		0x80
#define BIT_RSS3_CH18		0x40
#define BIT_RSS3_CH19		0x20
#define BIT_RSS3_CH20		0x10
#define BIT_RSS3_CH21		0x08
#define BIT_RSS3_CH22		0x04
#define BIT_RSS3_CH23		0x02
#define BIT_RSS3_CH24		0x01

#define REG_RSS4		0x9B
#define BIT_RSS4_CH25		0x80
#define BIT_RSS4_CH26		0x40
#define BIT_RSS4_CH27		0x20
#define BIT_RSS4_CH28		0x10
#define BIT_RSS4_CH29		0x08
#define BIT_RSS4_CH30		0x04
#define BIT_RSS4_CH31		0x02
#define BIT_RSS4_CH32		0x01

#define REG_T1RSCD1		0x9C
#define REG_T1RSCD2		0x9D

#define REG_RIIR		0x9F
#define BIT_RIIR_RLS7		0x40
#define BIT_RIIR_RLS6		0x20
#define BIT_RIIR_RLS5		0x10
#define BIT_RIIR_RLS4		0x08
#define BIT_RIIR_RLS3		0x04
#define BIT_RIIR_RLS2		0x02
#define BIT_RIIR_RLS1		0x01

#define REG_RIM1		0xA0
#define BIT_RIM1_RRAIC		0x80
#define BIT_RIM1_RAISC		0x40
#define BIT_RIM1_RLOSC		0x20
#define BIT_RIM1_RLOFC		0x10
#define BIT_RIM1_RRAID		0x08
#define BIT_RIM1_RAISD		0x04
#define BIT_RIM1_RLOSD		0x02
#define BIT_RIM1_RLOFD		0x01

#define REG_RIM2		0xA1
#define BIT_RIM2_E1_RSA1	0x08
#define BIT_RIM2_E1_RSA0	0x04
#define BIT_RIM2_E1_RCMF	0x02
#define BIT_RIM2_E1_RAF		0x01

#define REG_RIM3		0xA2
#define BIT_RIM3_T1_LORCC	0x80
#define BIT_RIM3_T1_LSPC	0x40
#define BIT_RIM3_T1_LDNC	0x20
#define BIT_RIM3_T1_LUPC	0x10
#define BIT_RIM3_T1_LORCD	0x08
#define BIT_RIM3_T1_LSPD	0x04
#define BIT_RIM3_T1_LDND	0x02
#define BIT_RIM3_T1_LUPD	0x01
#define BIT_RIM3_E1_LORCC	0x80
#define BIT_RIM3_E1_V52LNKC	0x20
#define BIT_RIM3_E1_RDMAC	0x10
#define BIT_RIM3_E1_LORCD	0x08
#define BIT_RIM3_E1_V52LNKD	0x02
#define BIT_RIM3_E1_RDMAD	0x01

#define REG_RIM4		0xA3
#define BIT_RIM4_RESF		0x80
#define BIT_RIM4_RESEM		0x40
#define BIT_RIM4_RSLIP		0x20
#define BIT_RIM4_RSCOS		0x08
#define BIT_RIM4_1SEC		0x04
#define BIT_RIM4_TIMER		0x02
#define BIT_RIM4_RMF		0x01

#define REG_RIM5		0xA4
#define BIT_RIM5_ROVR		0x20
#define BIT_RIM5_RHOBT		0x10
#define BIT_RIM5_RPE		0x08
#define BIT_RIM5_RPS		0x04
#define BIT_RIM5_RHWMS		0x02
#define BIT_RIM5_RNES		0x01

#define REG_RIM7		0xA6
#define BIT_RIM7_T1_RSLC96	0x08
#define BIT_RIM7_T1_RFDLF	0x04
#define BIT_RIM7_T1_BC		0x02
#define BIT_RIM7_T1_BD		0x01

#define REG_RSCSE1		0xA8
#define BITS_RSCSE1_ALL		0xFF
#define BIT_RSCSE1_CH1		0x80
#define BIT_RSCSE1_CH2		0x40
#define BIT_RSCSE1_CH3		0x20
#define BIT_RSCSE1_CH4		0x10
#define BIT_RSCSE1_CH5		0x08
#define BIT_RSCSE1_CH6		0x04
#define BIT_RSCSE1_CH7		0x02
#define BIT_RSCSE1_CH8		0x01

#define REG_RSCSE2		0xA9
#define BITS_RSCSE2_ALL		0xFF
#define BIT_RSCSE2_CH9		0x80
#define BIT_RSCSE2_CH10		0x40
#define BIT_RSCSE2_CH11		0x20
#define BIT_RSCSE2_CH12		0x10
#define BIT_RSCSE2_CH13		0x08
#define BIT_RSCSE2_CH14		0x04
#define BIT_RSCSE2_CH15		0x02
#define BIT_RSCSE2_CH16		0x01

#define REG_RSCSE3		0xAA
#define BITS_RSCSE3_ALL		0xFF
#define BIT_RSCSE3_CH17		0x80
#define BIT_RSCSE3_CH18		0x40
#define BIT_RSCSE3_CH19		0x20
#define BIT_RSCSE3_CH20		0x10
#define BIT_RSCSE3_CH21		0x08
#define BIT_RSCSE3_CH22		0x04
#define BIT_RSCSE3_CH23		0x02
#define BIT_RSCSE3_CH24		0x01

#define REG_RSCSE4		0xAB
#define BITS_RSCSE4_ALL		0xFF
#define BIT_RSCSE4_CH25		0x80
#define BIT_RSCSE4_CH26		0x40
#define BIT_RSCSE4_CH27		0x20
#define BIT_RSCSE4_CH28		0x10
#define BIT_RSCSE4_CH29		0x08
#define BIT_RSCSE4_CH30		0x04
#define BIT_RSCSE4_CH31		0x02
#define BIT_RSCSE4_CH32		0x01

#define REG_T1RUPCD1		0xAC
#define REG_T1RUPCD2		0xAD
#define REG_T1RDNCD1		0xAE
#define REG_T1RNDCD2		0xAF

#define REG_RRTS1		0xB0
#define BIT_RRTS1_RRAI		0x08
#define BIT_RRTS1_RAIS		0x04
#define BIT_RRTS1_RLOS		0x02
#define BIT_RRTS1_RLOF		0x01

#define REG_RRTS3		0xB2
#define BIT_RRTS3_T1_LORC	0x08
#define BIT_RRTS3_T1_LSP	0x08
#define BIT_RRTS3_T1_LDN	0x02
#define BIT_RRTS3_T1_LUP	0x01
#define BIT_RRTS3_E1_LORC	0x08
#define BIT_RRTS3_E1_V52LNK	0x02
#define BIT_RRTS3_E1_RDMA	0x01

#define REG_RGCCS1		0xCC
#define REG_RGCCS2		0xCD
#define REG_RGCCS3		0xCE
#define REG_RGCCS4		0xCF

#define REG_RCICE1		0xD0
#define REG_RCICE2		0xD1
#define REG_RCICE3		0xD2
#define REG_RCICE4		0xD3

#define REG_RBPCS1		0xD4
#define REG_RBPCS2		0xD5
#define REG_RBPCS3		0xD6
#define REG_RBPCS4		0xD7

/* TX Framer Register Definitions */
#define REG_THC2		0x113
#define BIT_THC2_TABT		0x80
#define BIT_THC2_SBOC		0x40
#define BIT_THC2_THCEN		0x20
#define BIT_THC2_THCS4		0x10
#define BIT_THC2_THCS3		0x08
#define BIT_THC2_THCS2		0x04
#define BIT_THC2_THCS1		0x02
#define BIT_THC2_THCS0		0x01

#define REG_E1TSACR		0x114
#define BIT_E1TSACR_SiAF	0x80
#define BIT_E1TSACR_SiNAF	0x40
#define BIT_E1TSACR_RA		0x20
#define BIT_E1TSACR_Sa4		0x10
#define BIT_E1TSACR_Sa5		0x08
#define BIT_E1TSACR_Sa6		0x04
#define BIT_E1TSACR_Sa7		0x02
#define BIT_E1TSACR_Sa8		0x01

#define REG_SSIE1		0x118
#define BITS_SSIE1_ALL		0xFF
#define BIT_SSIE1_CH1		0x80
#define BIT_SSIE1_CH2		0x40
#define BIT_SSIE1_CH3		0x20
#define BIT_SSIE1_CH4		0x10
#define BIT_SSIE1_CH5		0x08
#define BIT_SSIE1_CH6		0x04
#define BIT_SSIE1_CH7		0x02
#define BIT_SSIE1_CH8		0x01

#define REG_SSIE2		0x119
#define BITS_SSIE2_ALL		0xFF
#define BIT_SSIE2_CH9		0x80
#define BIT_SSIE2_CH10		0x40
#define BIT_SSIE2_CH11		0x20
#define BIT_SSIE2_CH12		0x10
#define BIT_SSIE2_CH13		0x08
#define BIT_SSIE2_CH14		0x04
#define BIT_SSIE2_CH15		0x02
#define BIT_SSIE2_CH16		0x01

#define REG_SSIE3		0x11A
#define BITS_SSIE3_ALL		0xFF
#define BIT_SSIE3_CH17		0x80
#define BIT_SSIE3_CH18		0x40
#define BIT_SSIE3_CH19		0x20
#define BIT_SSIE3_CH20		0x10
#define BIT_SSIE3_CH21		0x08
#define BIT_SSIE3_CH22		0x04
#define BIT_SSIE3_CH23		0x02
#define BIT_SSIE3_CH24		0x01

#define REG_SSIE4		0x11B
#define BITS_SSIE4_ALL		0xFF
#define BIT_SSIE4_CH25		0x80
#define BIT_SSIE4_CH26		0x40
#define BIT_SSIE4_CH27		0x20
#define BIT_SSIE4_CH28		0x10
#define BIT_SSIE4_CH29		0x08
#define BIT_SSIE4_CH30		0x04
#define BIT_SSIE4_CH31		0x02
#define BIT_SSIE4_CH32		0x01

#define REG_TS1			0x140
#define BIT_TS_A		0x08
#define BIT_TS_B		0x04
#define BIT_TS_C		0x02
#define BIT_TS_D		0x01
#define REG_TS2			0x141
#define REG_TS3			0x142
#define REG_TS4			0x143
#define REG_TS5			0x144
#define REG_TS6			0x145
#define REG_TS7			0x146
#define REG_TS8			0x147
#define REG_TS9			0x148
#define REG_TS10		0x149
#define REG_TS11		0x14A
#define REG_TS12		0x14B
#define REG_TS13		0x14C
#define REG_TS14		0x14D
#define REG_TS15		0x14E
#define REG_TS16		0x14F

#define REG_TCICE1		0x150
#define REG_TCICE2		0x151
#define REG_TCICE3		0x152
#define REG_TCICE4		0x153

#define REG_T1TFDL		0x162

#define REG_T1TBOC		0x163

#define REG_E1TAF		0x164

#define REG_E1TNAF		0x165
#define BIT_E1TNAF_A		0x20

#define REG_E1TSa4		0x169
#define REG_E1TSa5		0x16A
#define REG_E1TSa6		0x16B
#define REG_E1TSa7		0x16C
#define REG_E1TSa8		0x16D

#define REG_TMMR		0x180
#define BIT_TMMR_FRM_EN		0x80
#define BIT_TMMR_INIT_DONE	0x40
#define BIT_TMMR_SFTRST		0x02
#define BIT_TMMR_T1E1		0x01

#define REG_TCR1		0x181
#define BIT_TCR1_T1_TJC		0x80
#define BIT_TCR1_T1_TFPT	0x40
#define BIT_TCR1_T1_TCPT	0x20
#define BIT_TCR1_T1_TSSE	0x10
#define BIT_TCR1_T1_GB7S	0x08
#define BIT_TCR1_T1_TB8ZS	0x04
#define BIT_TCR1_T1_TAIS	0x02
#define BIT_TCR1_T1_TRAI	0x01
#define BIT_TCR1_E1_TTPT	0x80
#define BIT_TCR1_E1_T16S	0x40
#define BIT_TCR1_E1_TG802	0x20
#define BIT_TCR1_E1_TSiS	0x10
#define BIT_TCR1_E1_TSA1	0x08
#define BIT_TCR1_E1_THDB3	0x04
#define BIT_TCR1_E1_TAIS	0x02
#define BIT_TCR1_E1_TCRC4	0x01

#define REG_TCR2		0x182
#define BIT_TCR2_T1_TFDLS	0x80
#define BIT_TCR2_T1_TSLC96	0x40
#define BIT_TCR2_T1_TD4RM	0x04
#define BIT_TCR2_E1_AEBE	0x80		/* EBIT */
#define BIT_TCR2_E1_ARA 	0x20
#define BIT_TCR2_T1_TCR2_PDE 0x02

#define REG_TCR3		0x183
#define BIT_TCR3_ODF		0x80
#define BIT_TCR3_ODM		0x40
#define BIT_TCR3_TCSS1		0x20
#define BIT_TCR3_TCSS0		0x10
#define BIT_TCR3_MFRS		0x08
#define BIT_TCR3_TFM		0x04
#define BIT_TCR3_IBPV		0x02
#define BIT_TCR3_TLOOP		0x01

#define REG_TIOCR		0x184
#define BIT_TIOCR_TCLKINV	0x80
#define BIT_TIOCR_TSYNCINV	0x40
#define BIT_TIOCR_TSSYNCINV	0x20
#define BIT_TIOCR_TSCLKM	0x10
#define BIT_TIOCR_TSSM		0x08
#define BIT_TIOCR_TSIO		0x04
#define BIT_TIOCR_TSDW		0x02
#define BIT_TIOCR_TSM		0x01

#define REG_TESCR		0x185
#define BIT_TESCR_TDATFMT	0x80
#define BIT_TESCR_TGPCKEN	0x40
#define BIT_TESCR_TESE		0x01

#define REG_TCR4		0x186
#define BIT_TCR4_TRAIM		0x08
#define BIT_TCR4_TAISM		0x04
#define BIT_TCR4_TC1		0x02
#define BIT_TCR4_TC0		0x01
#define BITS_TCR4_CL_ENCODE(len)				\
		((len)==6 || (len)==3) 	? BIT_TCR4_TC0 :	\
		((len)==7) 		? BIT_TCR4_TC1 :	\
		((len)==5)		? 0x00 : (BIT_TCR4_TC1|BIT_TCR4_TC0)
		
	


#define REG_TXPC		0x18A
#define BIT_TXPC_TBPDIR		0x04
#define BIT_TXPC_TBPFUS		0x02
#define BIT_TXPC_TBPEN		0x01

#define REG_TLS1		0x190
#define BIT_TLS1_TPDV		0x08
#define BIT_TLS1_TMF		0x04
#define BIT_TLS1_LOTCC		0x02
#define BIT_TLS1_LOTC		0x01

#define REG_TLS2		0x191
#define REG_TLS3		0x192

#define REG_TIIR		0x19F
#define BIT_TIIR_TLS3		0x04
#define BIT_TIIR_TLS2		0x02
#define BIT_TIIR_TLS1		0x01

#define REG_TIM1		0x1A0
#define REG_TIM2		0x1A1
#define REG_TIM3		0x1A2

#define REG_T1TCD1		0x1AC
#define REG_T1TCD2		0x1AD

#define REG_TGCCS1		0x1CC
#define REG_TGCCS2		0x1CD
#define REG_TGCCS3		0x1CE
#define REG_TGCCS4		0x1CF

#define REG_PCL1		0x1D0
#define REG_PCL2		0x1D1
#define REG_PCL3		0x1D2
#define REG_PCL4		0x1D3

#define REG_TBPCS1		0x1D4
#define REG_TBPCS2		0x1D5
#define REG_TBPCS3		0x1D6
#define REG_TBPCS4		0x1D7

/* LIU Register Definitions */
#define REG_LTRCR		0x1000
#define BIT_LTRCR_JADS		0x10
#define BIT_LTRCR_JAPS1		0x08
#define BIT_LTRCR_JAPS0		0x04
#define BIT_LTRCR_T1J1E1S	0x02
#define BIT_LTRCR_LSC		0x01

#define REG_LTITSR		0x1001
#define BIT_LTITSR_TIMPTOFF	0x40
#define BIT_LTITSR_TIMPL1	0x20
#define BIT_LTITSR_TIMPL0	0x10
#define BIT_LTITSR_L2		0x04
#define BIT_LTITSR_L1		0x02
#define BIT_LTITSR_L0		0x01

#define REG_LMCR		0x1002
#define BIT_LMCR_TAIS		0x80
#define BIT_LMCR_ATAIS		0x40
#define BIT_LMCR_LLB		0x20
#define BIT_LMCR_ALB		0x10
#define BIT_LMCR_RLB		0x08
#define BIT_LMCR_TPDE		0x04
#define BIT_LMCR_RPLDE		0x02
#define BIT_LMCR_TE		0x01

#define REG_LRSR		0x1003
#define BIT_LRSR_OEQ		0x20
#define BIT_LRSR_UEQ		0x10
#define BIT_LRSR_SCS		0x04
#define BIT_LRSR_OCS		0x02
#define BIT_LRSR_LOSS		0x01

#define REG_LSIMR		0x1004
#define BIT_LSIMR_JALTCIM	0x80
#define BIT_LSIMR_OCCIM		0x40
#define BIT_LSIMR_SCCIM		0x20
#define BIT_LSIMR_LOSCIM	0x10
#define BIT_LSIMR_JALTSIM	0x08
#define BIT_LSIMR_OCDIM		0x04
#define BIT_LSIMR_SCDIM		0x02
#define BIT_LSIMR_LOSDIM	0x01

#define REG_LLSR		0x1005
#define BIT_LLSR_JALTC		0x80
#define BIT_LLSR_OCC		0x40
#define BIT_LLSR_SCC		0x20
#define BIT_LLSR_LOSC		0x10
#define BIT_LLSR_JALTS		0x08
#define BIT_LLSR_OCD		0x04
#define BIT_LLSR_SCD		0x03
#define BIT_LLSR_LOSD		0x01	

#define REG_LRSL		0x1006
#define REG_LRSL_SHIFT		4
#define REG_LRSL_MASK		0x0F
#define BIT_LRSL_RSL3		0x80
#define BIT_LRSL_RSL2		0x40
#define BIT_LRSL_RSL1		0x20
#define BIT_LRSL_RSL0		0x10

#define REG_LRISMR		0x1007
#define BIT_LRISMR_RG703	0x80
#define BIT_LRISMR_RIMPOFF	0x40
#define BIT_LRISMR_RIMPM1	0x20
#define BIT_LRISMR_RIMPM0	0x10
#define BIT_LRISMR_RTR		0x08
#define BIT_LRISMR_RMONEN	0x04
#define BIT_LRISMR_RSMS1	0x02
#define BIT_LRISMR_RSMS0	0x01


#define REG_LRISMR_TAP		0x1007
#define BIT_LRISMR_TAP_REXTON	0x80
#define BIT_LRISMR_TAP_RIMPON	0x40
#define BIT_LRISMR_TAP_RIMPM2	0x04
#define BIT_LRISMR_TAP_RIMPM1	0x02
#define BIT_LRISMR_TAP_RIMPM0	0x01


#define REG_LRCR_TAP		0x1008
#define BIT_LRCR_TAP_RG703	0x80
#define BIT_LRCR_TAP_RTR	0x08
#define BIT_LRCR_TAP_RMONEN	0x04
#define BIT_LRCR_TAP_RSMS1	0x02
#define BIT_LRCR_TAP_RSMS0	0x01



#define REG_LTXLAE		0x100C

/* BERT Register Definitions */
#define REG_BAWC		0x1100

#define REG_BRP1		0x1101
#define REG_BRP2		0x1102
#define REG_BRP3		0x1103
#define REG_BRP4		0x1104

#define REG_BC1			0x1105
#define BIT_BC1_TC		0x80
#define BIT_BC1_TINV		0x40
#define BIT_BC1_RINV		0x20
#define BIT_BC1_PS2		0x10
#define BIT_BC1_PS1		0x08
#define BIT_BC1_PS0		0x04
#define BIT_BC1_LC		0x02
#define BIT_BC1_RESYNC		0x01

#define REG_BC2			0x1106
#define BIT_BC2_EIB2		0x80
#define BIT_BC2_EIB1		0x40
#define BIT_BC2_EIB0		0x20
#define BIT_BC2_SBE		0x10
#define BIT_BC2_RPL3		0x08
#define BIT_BC2_RPL2		0x04
#define BIT_BC2_RPL1		0x02
#define BIT_BC2_RPL0		0x01

#define REG_BBC1		0x1107
#define REG_BBC2		0x1108
#define REG_BBC3		0x1109
#define REG_BBC4		0x110A

#define REG_BEC1		0x110B
#define REG_BEC2		0x110C
#define REG_BEC3		0x110D

#define REG_BLSR		0x110E
#define BIT_BLSR_BBED		0x40
#define BIT_BLSR_BBCO		0x20
#define BIT_BLSR_BECO		0x10
#define BIT_BLSR_BRA1		0x08
#define BIT_BLSR_BRA0		0x04
#define BIT_BLSR_BRLOS		0x02
#define BIT_BLSR_BSYNC		0x01

#define REG_BSIM		0x110F
#define BIT_BSIM_BBED		0x40
#define BIT_BSIM_BBCO		0x20
#define BIT_BSIM_BECO		0x10
#define BIT_BSIM_BRA1		0x08
#define BIT_BSIM_BRA0		0x04
#define BIT_BSIM_BRLOS		0x02
#define BIT_BSIM_BSYNC		0x01

#define REG_BLSR		0x110E
#define BIT_BLSR_BBED		0x40
#define BIT_BLSR_BBCO		0x20
#define BIT_BLSR_BECO		0x10
#define BIT_BLSR_BRA1		0x08
#define BIT_BLSR_BRA0		0x04
#define BIT_BLSR_BRLOS		0x02
#define BIT_BLSR_BSYNC		0x01

#endif /* __SDLA_TE1_DS_H */
