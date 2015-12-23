#ifndef	_SFMDEF_H
#define	_SFMDEF_H
/*****************************************************************************
* FILE:		sfmdef.h	SDLA Firmware Module Definitions
* AUTHOR:	Gene Kozin
* ----------------------------------------------------------------------------
* (c) Copyright 1995-1996 Sangoma Technologies Inc.  All Rights Reserved.
* ============================================================================
* Rev.#	Date		Who	Details
* -----	-----------	---	----------------------------------------------
*  2.00	Apr.16,1996	GK	Changed adapter & firmware codes.
*  1.01	Dec.15,1995	GK	
*  1.00	Nov.09,1995	GK	Initial version.
*****************************************************************************/

/****** Defines *************************************************************/

#define	SFM_VERSION	2
#define	SFM_SIGNATURE	"SFM - Sangoma SDLA Firmware Module"

/*
 *----- Min/Max --------------------------------------------------------------
 */
#define	SFM_IMAGE_SIZE	0x8000	/* max size of SDLA code image file */
#define	SFM_DESCR_LEN	256	/* max length of description string */
#define	SFM_MAX_SDLA	16	/* max number of compatible adapters */

/*
 *----- Adapter Types --------------------------------------------------------
 */
#ifndef	SDLA_TYPES
#  define SDLA_TYPES
#  define SDLA_S502A	5020
#  define SDLA_S502E	5021
#  define SDLA_S503	5030
#  define SDLA_S508	5080
#  define SDLA_S507	5070
#  define SDLA_S509	5090
#endif	/* SDLA_TYPES */

/*
 *----- Firmware Identification Numbers --------------------------------------
 *    0  ..  999	Test & Diagnostics
 *  1000 .. 1999	Streaming HDLC
 *  2000 .. 2999	Bisync
 *  3000 .. 3999	SDLC
 *  4000 .. 4999	HDLC
 *  5000 .. 5999	X.25
 *  6000 .. 6999	Frame Relay
 *  7000 .. 7999	PPP
 */
#ifndef	FIRMWARE_IDS
#  define FIRMWARE_IDS
#  define SFID_CALIB502	 200
#  define SFID_STRM502	1200
#  define SFID_STRM508	1800
#  define SFID_BSC502	2200
#  define SFID_SDLC502	3200
#  define SFID_HDLC502	4200
#  define SFID_X25_502	5200
#  define SFID_FR502	6200
#  define SFID_FR508	6800
#  define SFID_PPP502	7200
#  define SFID_PPP508	7800
#endif	/* FIRMWARE_IDS */

/****** Data Types **********************************************************/

typedef struct	SFMInfo		/* SDLA Firmware Module Information Block */
{
  unsigned short	codeid;		/* firmware ID */
  unsigned short	version;	/* firmaware version number */
  unsigned short	adapter[SFM_MAX_SDLA];	/* compatible adapter types */
  unsigned long		memsize;	/* minimum memory size */
  unsigned short	reserved[2];	/* reserved */
  unsigned short	startoffs;	/* entry point offset */
  unsigned short	winoffs;	/* dual-port memory window offset */
  unsigned short	codeoffs;	/* code load offset */
  unsigned short	codesize;	/* code size */
  unsigned short	dataoffs;	/* configuration data load offset */
  unsigned short	datasize;	/* configuration data size */
} SFMInfo_t;

typedef struct	SFM		/* SDLA Firmware Module Structire */
{
  char			signature[80];	/* SFM file signature */
  unsigned short	version;	/* file format version */
  unsigned short	checksum;	/* info + image */
  unsigned short	reserved[6];	/* reserved */
  char			descr[SFM_DESCR_LEN]; /* optional description string */
  SFMInfo_t		info;		/* firmware module info */
  unsigned char		image[0];	/* loadable code image */
} SFM_t;

#endif	/* _SFMDEF_H */

