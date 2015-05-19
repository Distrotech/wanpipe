/*****************************************************************************
* sdlapci.h	WANPIPE(tm) Multiprotocol WAN Link Driver.
*		Definitions for the SDLA PCI adapter.
*
* Author:	Gideon Hack	<ghack@sangoma.com>
*
* Copyright:	(c) 1999-2000 Sangoma Technologies Inc.
*
*		This program is free software; you can redistribute it and/or
*		modify it under the terms of the GNU General Public License
*		as published by the Free Software Foundation; either version
*		2 of the License, or (at your option) any later version.
* ============================================================================
* Jun 02, 1999	Gideon Hack	Initial version.
*****************************************************************************/
#ifndef	_SDLAPCI_H
#define	_SDLAPCI_H

/****** Defines *************************************************************/

/* Definitions for identifying and finding S514 PCI adapters */
#define V3_VENDOR_ID			0x11B0	/* V3 vendor ID number */
#define V3_DEVICE_ID  			0x0002	/* V3 device ID number */
#define SANGOMA_SUBSYS_VENDOR 		0x4753	/* ID for Sangoma */
#define PCI_DEV_SLOT_MASK		0x1F	/* mask for slot numbering */
#define PCI_IRQ_NOT_ALLOCATED		0xFF	/* interrupt line for no IRQ */

/* Vendor id lisr for Sangoma cards */
#define SANGOMA_PCI_VENDOR		0x1923	/* Old value -> 0x11B0 */
#define SANGOMA_PCI_VENDOR_OLD		0x10EE	/* Old value -> 0x11B0 */

/* Device id list for Sangoma cards */
#define SANGOMA_PCI_DEVICE		0x0300	/* Old value -> 0x0200 */
#define SANGOMA_PCI_4_DEVICE		0x0400	/* */
#define SANGOMA_S2E_300_PCI_DEVICE	0x0030	/* Xilinx-300  SPARTAN-2e */
#define SANGOMA_S3_400_PCI_DEVICE	0x0040	/* Xilinx-400  SPARTAN-3  */
#define SANGOMA_S3_1000_PCI_DEVICE	0x0100	/* Xilinx-1000 SPARTAN-3  */
#define SANGOMA_S3E_250_PCI_DEVICE	0x0025	/* Xilinx-250  SPARTAN-3e */
#define SANGOMA_S3E_500_PCI_DEVICE	0x0050	/* Xilinx-500  SPARTAN-3e */
#define SANGOMA_S3E_1200_PCI_DEVICE	0x0120	/* Xilinx-1200 SPARTAN-3e */
#define SANGOMA_GP_ADSL_PCI_DEVICE	0x0001	/* Globe-Span ADSL */

#if defined(__WINDOWS__)
#define SANGOMA_PCI_4_SHARK_DEVICE	0x0100	/* A104D */
#define SANGOMA_PCI_16_SHARK_DEVICE	0x0160	/* A116 */
#define SANGOMA_PCI_A200_SHARK_DEVICE	0x0040	/* A200 (D) */
#define SANGOMA_PCI_A056_SHARK_DEVICE	0x0020	/* A056 */
#define SANGOMA_PCI_B600_SHARK_DEVICE	0x0025	/* B600 */
#endif

/* Definition for identifying and finding PLX PCI bridge adapters */
#define PLX_VENDOR_ID			0x10b5	/* PLX vendor ID number */
#define PLX_DEVICE_ID  			0x8111	/* PLX device ID number */
#define PLX2_DEVICE_ID  		0x8112	/* PLX device ID number rev 2 */

#define TUNDRA_VENDOR_ID		0x10E3	/* TUNDRA vendor ID number */
#define TUNDRA_DEVICE_ID		0x8111	/* TUNDRA device ID number */

#define PLX_EEPROM_ENABLE		0x5A
#define PLX_EEPROM_VENDOR_OFF		0x7C	/* 2 bytes for Card vendor id */

#define A101_1TE1_SUBSYS_VENDOR			0xA010	/* A101 with T1/E1 1 line  */
#define A101_2TE1_SUBSYS_VENDOR			0xA011	/* A101 with T1/E1 2 lines */
#define A104_4TE1_SUBSYS_VENDOR			0xA013	/* A104 with T1/E1 4 lines */
#define A300_UTE3_SUBSYS_VENDOR			0xA020	/* A300 with T3/E3 (unchannelized) */
#define A305_CT3_SUBSYS_VENDOR			0xA030	/* A305 with T3 (channelized) */

#define S5141_SERIAL_SUBSYS_VENDOR		0x0011	/* S5141, single CPU, serial */
#define S5142_SERIAL_SUBSYS_VENDOR		0x0012	/* S5142, dual CPU, serial */
#define S5143_SERIAL_SUBSYS_VENDOR		0x0013	/* S5143, single CPU, FT1 */
#define S5144_SERIAL_SUBSYS_VENDOR		0x0014	/* S5144, single CPU, T1/E1 */
#define S5145_SERIAL_SUBSYS_VENDOR		0x0015	/* S5145, single CPU, 56K */
#define S5147_SERIAL_SUBSYS_VENDOR		0x0017  /* S5147, dual CPU, T1/E1 */
#define S5148_SERIAL_SUBSYS_VENDOR		0x0018	/* S5148, single CPU, T1/E1 */

#define AFT_1TE1_SHARK_SUBSYS_VENDOR		0xA111	/* A101-SHARK T1/E1 1 lines */
#define AFT_2TE1_SHARK_SUBSYS_VENDOR		0xA112	/* A102-SHARK T1/E1 2 lines */
#define AFT_4TE1_SHARK_SUBSYS_VENDOR		0xA113	/* A104-SHARK T1/E1 4 lines */
#define AFT_8TE1_SHARK_SUBSYS_VENDOR		0xA114	/* A108-SHARK T1/E1 8 lines */
#define A300_UTE3_SHARK_SUBSYS_VENDOR		0xA115	/* A300-SHARK with T3/E3 (unchannelized) */
#define A305_CTE3_SHARK_SUBSYS_VENDOR		0xA116	/* A305-SHARK with T3 (channelized) */
#define AFT_16TE1_SHARK_SUBSYS_VENDOR		0xA117	/* A116-SHARK T1/E1 16 lines */
#define AFT_T116_SUBSYS_VENDOR			0xA118  /* T116 T1/E1 16 lines Tapping */
#define A200_REMORA_SHARK_SUBSYS_VENDOR		0xA200	/* AFT-REMORA SHARK analog board */
#define A400_REMORA_SHARK_SUBSYS_VENDOR		0xA400	/* AFT-REMORA SHARK analog board */

#define AFT_ISDN_BRI_SHARK_SUBSYS_VENDOR	0xA500	/* AFT-ISDN BRI SHARK board */
#define B500_SHARK_SUBSYS_VENDOR	    0xB500	/* AFT-B500 SHARK BRI B500 board */
#define A700_SHARK_SUBSYS_VENDOR		0xA700	/* AFT-700 SHARK FLEX-BRI board */
#define AFT_56K_SHARK_SUBSYS_VENDOR		0xA056	/* AFT-56K SHARK board */

#define AFT_A600_SUBSYS_VENDOR			0xA600	/* AFT-B600 Series board */
#define AFT_B601_SUBSYS_VENDOR			0xA601	/* AFT-B600 Series board */
#define AFT_B610_SUBSYS_VENDOR			0xA610	/* AFT-B610 single FXS Series board */

#define AFT_W400_SUBSYS_VENDOR                  0xF400  /* AFT-W400 (GSM card) */

#define AFT_B800_SUBSYS_VENDOR			0xB800	/* AFT-B800 2/4 FXO or 2/4 FXS board */

#define AFT_2SERIAL_V35X21_SUBSYS_VENDOR	0xA031	/* AFT-A142 2 Port V.35/X.21 board */
#define AFT_4SERIAL_V35X21_SUBSYS_VENDOR	0xA032	/* AFT-A144 4 Port V.35/X.21 board */
#define AFT_2SERIAL_RS232_SUBSYS_VENDOR		0xA033	/* AFT-A142 2 Port RS232 board */
#define AFT_4SERIAL_RS232_SUBSYS_VENDOR		0xA034	/* AFT-A144 4 Port RS232 board */


#define AFT_CORE_ID_MASK	0x00FF
#define AFT_CORE_REV_MASK	0xFF00
#define AFT_CORE_REV_SHIFT	8
#define AFT_HDLC_CORE_ID	0x00	/* HDLC core */
#define AFT_ANALOG_FE_CORE_ID	0x00	/* A200 FXS/FXO core */
#define AFT_ATM_CORE_ID		0x01	/* ATM core */
#define AFT_SS7_CORE_ID		0x02	/* SS7 core */
#define AFT_HDLC_EC_CORE_ID	0x10	/* HDLC & EC */
#define AFT_PMC_FE_CORE_ID	0x10	/* HDLC & EC (with PMC FE) 	*/
#define AFT_DS_FE_CORE_ID	0x13	/* HDLC & EC (with Dallas FE) 	*/
#define AFT_CORE_ID(subsys_id)	(subsys_id & AFT_CORE_ID_MASK)
#define AFT_CORE_REV(subsys_id)	((subsys_id >> AFT_CORE_REV_SHIFT) & 0xFF)
#define AFT_CORE_ID_DECODE(core_id)					\
		(core_id == AFT_HDLC_CORE_ID)		? "HDLC"  :	\
		(core_id == AFT_ATM_CORE_ID)		? "ATM"   :	\
		(core_id == AFT_SS7_CORE_ID)		? "SS7"   :	\
		(core_id == AFT_HDLC_EC_CORE_ID)	? "HDLC"  :	\
		(core_id == AFT_DS_FE_CORE_ID)		? "HDLC (DS)" :	\
								"Unknown"

#define AFT_CHIP_UNKNOWN	0x0000
#define AFT_CHIP_OLD_X300	0x0300
#define AFT_CHIP_OLD_X400	0x0400
#define AFT_CHIP_X200		0x0020
#define AFT_CHIP_X250		0x0025
#define AFT_CHIP_X300		0x0030
#define AFT_CHIP_X400		0x0040
#define AFT_CHIP_X1000		0x0100
#define AFT_CHIP_X1600		0x0160

#define AFT_PCI_MEM_SIZE	0x002FF
#define XILINX_PCI_LATENCY	0x0000FF00

#define AFT2_PCI_MEM_SIZE	0x07FFF
#define AFT4_PCI_MEM_SIZE	0x0FFFF
#define AFT8_PCI_MEM_SIZE	0x1FFFF
#define AFT16_PCI_MEM_SIZE	0x3FFFF

#define XILINX_PCI_CMD_REG	0x04
#define XILINX_PCI_LATENCY_REG  0x0D

/* Local PCI register offsets */ 
#define PCI_VENDOR_ID_WORD	0x00		/* vendor ID */
#define PCI_DEVICE_ID_WORD	0x02		/* device ID */
#define PCI_REVISION_ID_BYTE	0x08		/* revision ID */
#define PCI_SUBCLASS_ID_BYTE	0x0a		/* subclass ID byte */
#define PCI_IO_BASE_DWORD	0x10		/* IO base */	
#define PCI_MEM_BASE0_DWORD	0x14		/* memory base - apperture 0 */
#define PCI_MEM_BASE1_DWORD     0x18		/* memory base - apperture 1 */
#define PCI_SUBSYS_VENDOR_WORD 	0x2C		/* subsystem vendor ID */
#define PCI_SUBSYS_ID_WORD 	0x2E		/* subsystem ID */
#define PCI_INT_LINE_BYTE	0x3C		/* interrupt line */
#define PCI_INT_PIN_BYTE	0x3D		/* interrupt pin */
#define PCI_MAP0_DWORD		0x40		/* PCI to local bus address 0 */
#define PCI_MAP1_DWORD          0x44		/* PCI to local bus address 1 */
#define PCI_INT_STATUS          0x48		/* interrupt status */
#define PCI_INT_CONFIG		0x4C		/* interrupt configuration */
#define PCI_CORE_REV_REG	0xFC 		/* firmware version */
#define WAN_INVALID_FIRMWARE 0x42 /* Implies the card is using new style firmware version */
  
/* Local PCI register usage */
#define PCI_MEMORY_ENABLE	0x00000003	/* enable PCI memory */
#define PCI_CPU_A_MEM_DISABLE	0x00000002	/* disable CPU A memory */
#define PCI_CPU_B_MEM_DISABLE  	0x00100002	/* disable CPU B memory */
#define PCI_ENABLE_IRQ_CPU_A	0x005A0004	/* enable IRQ for CPU A */
#define PCI_ENABLE_IRQ_CPU_B    0x005A0008	/* enable IRQ for CPU B */
#define PCI_ENABLE_IRQ_DMA0     0x01000000	/* enable IRQ for DMA 0 */
#define PCI_ENABLE_IRQ_DMA1     0x02000000	/* enable IRQ for DMA 1 */
#define PCI_DISABLE_IRQ_CPU_A   0x00000004	/* disable IRQ for CPU A */
#define PCI_DISABLE_IRQ_CPU_B   0x00000008	/* disable IRQ for CPU B */
#define PCI_DISABLE_IRQ_DMA0     0x01000000	/* disable IRQ for DMA 0 */
#define PCI_DISABLE_IRQ_DMA1     0x02000000	/* disable IRQ for DMA 1 */
 
/* Setting for the Interrupt Status register */  
#define IRQ_DMA0		0x01000000	/* IRQ for DMA0 */
#define IRQ_DMA1		0x02000000	/* IRQ for DMA1 */
#define IRQ_LOCAL_CPU_A         0x00000004	/* IRQ for CPU A */
#define IRQ_LOCAL_CPU_B		0x00000008	/* IRQ for CPU B */
#define IRQ_CPU_A               0x04            /* IRQ for CPU A */
#define IRQ_CPU_B               0x08		/* IRQ for CPU B */

/* The maximum size of the S514 memory */
#define MAX_SIZEOF_S514_MEMORY	(256 * 1024)

/* S514 control register offsets within the memory address space */
#define S514_CTRL_REG_BYTE	0x80000
 
/* S514 adapter control bytes */
#define S514_CPU_HALT 		0x00
#define S514_CPU_START		0x01

/* The maximum number of S514 adapters supported */
#define MAX_S514_CARDS		20	

/* ADSL Hardware Defines */
#define GSI_PCI_MEMORY_SIZE         (8 * (4 * 2 * 1024))
#define PCI_VENDOR_ID_GSI           0x14BC
#define PCI_DEVICE_ID_GSI_PULSAR    0xD002
#define PCI_DEVICE_ID_GSI_ADSL      PCI_DEVICE_ID_GSI_PULSAR
#define PCI_DEVICE_ID_GSI_ADSL_V2   0xD001

#endif	/* _SDLAPCI_H */
