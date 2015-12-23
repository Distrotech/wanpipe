/*****************************************************************************
* sdla_xilinx.h	WANPIPE(tm) S51XX Xilinx Hardware Support
* 		
* Authors: 	Nenad Corbic <ncorbic@sangoma.com>
*
* Copyright:	(c) 2003 Sangoma Technologies Inc.
*
*		This program is free software; you can redistribute it and/or
*		modify it under the terms of the GNU General Public License
*		as published by the Free Software Foundation; either version
*		2 of the License, or (at your option) any later version.
* ============================================================================
* Jan 07, 2003	Nenad Corbic	Initial version.
*****************************************************************************/


#ifndef _SDLA_XILINX_H
#define _SDLA_XILINX_H


/*======================================================
 *
 * XILINX Chip PCI Registers
 *
 *=====================================================*/

/* Main configuration and status register */
#define XILINX_CHIP_CFG_REG		0x40

/* Interface registers used to program
 * board framer i.e. front end 
 * (PCM,Level1...) */
#define XILINX_MCPU_INTERFACE		0x44
#define XILINX_MCPU_INTERFACE_ADDR	0x46

#define TE3_LOCAL_CONTROL_STATUS_REG    0x48

#define XILINX_GLOBAL_INTER_MASK	0x4C

/* HDLC interrupt pending registers used
 * in case of an HDLC tx or rx abort condition */
#define XILINX_HDLC_TX_INTR_PENDING_REG	0x50
#define XILINX_HDLC_RX_INTR_PENDING_REG	0x54


/* Maximum number of frames in a fifo
 * where frame lengths vary from 1 to 4 bytes */
#define WP_MAX_FIFO_FRAMES	7


/* DMA interrupt pending reg, used to
 * signal dma tx/rx complete event on a 
 * specific channel */
#define XILINX_DMA_TX_INTR_PENDING_REG	0x58
#define XILINX_DMA_RX_INTR_PENDING_REG	0x5C


/* Timeslot addr reg (bits: 16-20) is used to access
 * control memory in order to setup
 * (physical) T1/E1 timeslot channels
 *
 * Logic (HDLC) Channel addr reg (bits: 0-4) is used to
 * access HDLC Core internal registers.
 * Used to access and configure each 
 * HDCL channel (tx and rx),  
 * in HDCL core.*/
#define XILINX_TIMESLOT_HDLC_CHAN_REG 	0x60

/* T3 Tx/Rx Channel Select
 *
 * 0=Tx Channel
 * 1=Rx Channel */
#define AFT_T3_RXTX_ADDR_SELECT_REG 	0x60

#define XILINX_CURRENT_TIMESLOT_MASK	0x00001F00     
#define XILINX_CURRENT_TIMESLOT_SHIFT   8

/* HDLC data register used to 
 * access and configure HDLC core
 * control status registers */
#define XILINX_HDLC_CONTROL_REG		0x64

/* HDLC data register used to
 * access and configure HDLC core
 * address registers */
#define XILINX_HDLC_ADDR_REG		0x68


/* Control RAM data buffer, used to
 * write/read data to/from control
 * (chip) memory.  
 *
 * The address of the memory access
 * must be specified in XILINX_TIMESLOT_HDLC_CHAN_REG.
 */
#define XILINX_CONTROL_RAM_ACCESS_BUF	0x6C


/* Global DMA control register used to
 * setup DMA engine */
#define XILINX_DMA_CONTROL_REG		0x70

/* DMA tx status register is a bitmap
 * status of each logical tx (HDLC) channel.
 * 
 * Bit 0: Fifo full
 * Bit 1: Fifo is ready to dma tx data from
 *        pci memory to tx fifo.
 */
#define XILINX_DMA_TX_STATUS_REG	0x74

/* TX Watchdog Timer Register
 *
 * Delay constant for watchdog
 * timer. From 1 to 255. 
 *
 * Minimum Interval = 1ms
 *
 * 0    =Reset/Disable Timer
 *       Ack Interrupt pending flag
 *       
 * 1-255=Timer enabled 1ms intervals
 * 
 * */
#define AFT_TE3_TX_WDT_CTRL_REG		0x74

/* DMA rx status register is a bitmap
 * status of each logical rx (HDLC) channel.
 * 
 * Bit 0: Fifo is empty or not ready to
 *        dma data to pci memory.
 * Bit 1: Fifo is ready to dma rx data 
 *        into pci memory.
 */
#define XILINX_DMA_RX_STATUS_REG	0x78

/* RX Watchdog Timer Register
 *
 * Delay constant for watchdog
 * timer. From 1 to 255. 
 *
 * Minimum Interval = 1ms
 *
 * 0    =Reset/Disable Timer
 *       Ack Interrupt pending flag
 *       
 * 1-255=Timer enabled 1ms intervals
 * 
 * */
#define AFT_TE3_RX_WDT_CTRL_REG		0x78


/* TE3 Fractional Encapsulation Control
 * Status Register
 *
 */
#define TE3_FRACT_ENCAPSULATION_REG	0x7C


/* AFT TE3 Current DMA descriptor address
 * 
 * Bit 0-3: Current Tx Dma Descr
 * Bit 4-7: Current Rx Dma Descr
 */
#define AFT_TE3_CRNT_DMA_DESC_ADDR_REG	0x80


/* DMA descritors, RAM 128x32 
 *
 * To be used as offsets to the TX/RX DMA
 * Descriptor, based on the channel number.
 * 
 * Addr = Channel_Number*0xF + Tx/Rx_DMA_DESCRIPTOR_LO/HI
 * 
 * */
#define XILINX_TxDMA_DESCRIPTOR_LO	0x100
#define XILINX_TxDMA_DESCRIPTOR_HI	0x104
#define XILINX_RxDMA_DESCRIPTOR_LO	0x108
#define XILINX_RxDMA_DESCRIPTOR_HI	0x10C





/*======================================================
 * XILINX_CHIP_CFG_REG  Bit Map
 *
 * Main configuration and status register
 *=====================================================*/

/* Select between T1/T3 or E1/E3 front end
 * Bit=0 : T1/T3   Bit=1 : E1/E3 */
#define INTERFACE_TYPE_T1_E1_BIT	0
#define INTERFACE_TYPE_T3_E3_BIT	0

/* Enable onboard led
 * 0= On
 * 1= Off  */
#define XILINX_RED_LED			1

/* T3 HDLC Controller Bit
 * 0= HDLC mode
 * 1= Transparent mode  */
#define AFT_T3_HDLC_TRANS_MODE		1

/* Enable frame flag generation on
 * front end framers (T1/E1...) */
#define FRONT_END_FRAME_FLAG_ENABLE_BIT	2

/* Select T3/E3 clock source 
 * 0=External/Normal
 * 1=Internal/Master */
#define AFT_T3_CLOCK_MODE		2

/* Interface mode: 
 * 	DS3 '0' - M13, "1" - C-Bit
 * 	E3  '0' - G.751, "1" - G.832 */
#define INTERFACE_MODE_DS3_C_BIT	3
#define INTERFACE_MODE_E3_G832		3

/* Reset front end framer (T1/E1 ...)
 * Bit=1: Rest Active */
#define FRONT_END_RESET_BIT		4

/* Reset the whole chip logic except
 * HDLC Core: Bit=1*/
#define CHIP_RESET_BIT			5

/* Reset HDLC Core: Bit=1 */
#define HDLC_CORE_RESET_BIT		6

/* HDLC core ready flag
 * Bit=1: HDLC Core ready for operation
 * Bit=0: HDLC Core in reset state */
#define HDLC_CORE_READY_FLAG_BIT	7

/* Enable or Disable all chip interrupts
 * Bit=1: Enable   Bit=0: Disable */
#define GLOBAL_INTR_ENABLE_BIT		8

/* Enable chip interrupt on any error event.
 * Bit=1: Enable   Bit=0: Disable */
#define ERROR_INTR_ENABLE_BIT		9

/* Reload Xilinx firmware
 * Used to re-configure xilinx hardware */
#define FRONT_END_INTR_ENABLE_BIT	10

/* Enable HDLC decapsualtion
 * on the TE3 Rx fractional 
 * data stream.
 */
#define ENABLE_TE3_FRACTIONAL	12



/* TE3 Remote Fractional Equipment
 *  
 * This option, indicates the remote
 * TE3 Fractional Device, since each
 * device might have custom data endcoding.
 */
#define TE3_FRACT_REMOTE_VENDOR_SHIFT	16	
#define TE3_FRACT_REMOTE_VENDOR_MASK	0xFFF8FFFF	

#define TE3_FRACT_VENDOR_KENTROX	0
#define TE3_FRACT_VENDOR_ADTRAN		1
#define TE3_FRACT_VENDOR_DIGITAL	2
#define TE3_FRACT_VENDOR_LARSCOM	3
#define TE3_FRACT_VENDOR_VERILINK	4

#ifdef WAN_KERNEL
static __inline void
te3_enable_fractional(unsigned int *reg, unsigned int vendor)
{
	*reg&=TE3_FRACT_REMOTE_VENDOR_MASK;	
	*reg|=(vendor<<TE3_FRACT_REMOTE_VENDOR_SHIFT); 

	wan_set_bit(ENABLE_TE3_FRACTIONAL,reg);
}
#endif

/* 8bit error type, used to check for
 * chip error conditions.
 * Value=0: Normal  Value=non zero: Error */
#define CHIP_ERROR_MASK			0x00FF0000

/* This bit indicates that Tx
 * Watchdog timer interrupt pending
 * flag is asserted. */
#define AFT_TE3_TX_WDT_INTR_PND	26


/* This bit indicates that Rx
 * Watchdog timer interrupt pending
 * flag is asserted. */
#define AFT_TE3_RX_WDT_INTR_PND	27


/* Front end interrupt status bit
 * Bit=1: Interrupt requested */
#define FRONT_END_INTR_FLAG		28

/* Security Status Flag
 * Bit=1: Security compromised and
 *        chip will stop operation */
#define SECURITY_STATUS_FLAG		29

/* Error interrupt status bit
 * Bit=1: Error occured on chip */
#define ERROR_INTR_FLAG			30


/* DMA interrupt status bit 
 * Bit=1: DMA inter pending register 
 *        not equal to 0. 
 *
 * i.e. one of DMA channels has
 *      generated an interrupt */
#define DMA_INTR_FLAG			31


#define XILINX_GLOBAL_INTER_STATUS	0xD0000000


/*======================================================
 * XILINX_TIMESLOT_HDLC_CHAN_REG  Bit Map
 *
 * Timeslot addr reg (bits: 16-20) is used to access
 * control memory in order to setup
 * (physical) T1/E1 timeslot channels 
 *
 * Logic (HDLC) Channel addr reg (bits: 0-4) is used to
 * access HDLC Core internal registers.
 * Used to access and configure each 
 * HDCL channel (tx and rx),  
 * in HDCL core. 
 *=====================================================*/

/* Bit shift timeslot value (0-31) to the
 * appropriate register location
 * Bits: 16-20 */
#define	TIMESLOT_BIT_SHIFT	16
#define TIMESLOT_BIT_MASK	0x001F0000
#define HDLC_LOGIC_CH_BIT_MASK	0x0000001F 

#define HDLC_LCH_TIMESLOT_MASK  0x001F001F

/*======================================================
 * XILINX_HDLC_CONTROL_REG
 * 
 *=====================================================*/

/*======================================= 
 * HDLC RX Channel Status bits: 0-4
 * Indicate status of single, specified
 * HDLC channel */

/* Status of rx channel: 1=inactive 0=active */
#define	HDLC_RX_CHAN_ENABLE_BIT		0

/* Rx status of a channel: 
 * 1=frame receiving    0=not receiving*/
#define	HDLC_RX_FRAME_DATA_BIT   	1

/* Indicates data on HDLC channel
 * 1=data present 	0=idle (all 1) */
#define	HDLC_RC_CHAN_ACTIVE_BIT		2

/* Indicates that last frame contained an error.
 * 1=frame error        0=no frame error */
#define HDLC_RX_FRAME_ERROR_BIT		3

/* Indicates that last frame was aborted 
 * 1=frame aborted	0=frame not aborted */
#define HDLC_RX_FRAME_ABORT_BIT		4



/*======================================= 
 * HDLC RX Channel Control bits: 16-21
 * Used to configure a single, specified
 * rx HDLC channel */

/* Enable or disable HDLC protocol on 
 * specified channel. 
 * 1: Transpared Mode 	0: HDLC enabled */
#define HDLC_RX_PROT_DISABLE_BIT	16

/* HDLC Address recognition disable bit
 * 1: No address matching (default)
 * 0: normal address matching */
#define HDLC_RX_ADDR_RECOGN_DIS_BIT 	17

/* HDLC Address filed discard
 * 1: addr filed in rx frame discarded
 * 0: addr filed in rx frame passed up (dflt) */
#define HDLC_RX_ADDR_FIELD_DISC_BIT	18

/* HDLC Address size
 * 1: 16 bit		0: 8 bit */
#define HDLC_RX_ADDR_SIZE_BIT		19

/* HDLC broadcast address matching
 * 1: disable (does not match all 1's brdcst)
 * 0: enable (dflt) */
#define HDLC_RX_BRD_ADDR_MATCH_BIT	20

/* FCS Size 
 * 1: CRC-32 		0: CRC-16 (default) */
#define HDLC_RX_FCS_SIZE_BIT		21


/* Idle flags are present on rx line 
 * 1: Idle present	0: no idle flags */
#define HDLC_CORE_RX_IDLE_LINE_BIT	22

/* Abort flags are present on rx line
 * 1: Abort present	0: no aborts */
#define HDLC_CODE_RX_ABORT_LINE_BIT	23



/*=======================================
 * HDLC TX Channel Control bits: 24-31
 * Used to configure a single, specified
 * tx HDLC channel */

/* Enable or disable a specified 
 * HDLC tx channel. 
 * 1: Channel Enabled 	
 * 0: Channel Disabled 
 *    (tx idle all 1's in transparent mode
 *    (tx idle 7E in hdlc mode) */
#define HDLC_TX_CHAN_ENABLE_BIT		24

/* Enable or disable HDLC protocol on 
 * specified tx channel. 
 * 1: Transpared Mode 	0: HDLC enabled */
#define HDLC_TX_PROT_DISABLE_BIT	25


/* HDLC address insertion
 * 1: enable		0: disable (default) */
#define HDLC_TX_ADDR_INSERTION_BIT	26


/* HDLC Tx Address size
 * 1: 16 bit		0: 8 bit */
#define HDLC_TX_ADDR_SIZE_BIT		27


/* FCS Size 
 * 1: CRC-32 		0: CRC-16 (default) */
#define HDLC_TX_FCS_SIZE_BIT		28

/* Frame abort in progres or force it
 * 1: frame abort	0: normal
 * Set to 1 to FORCE tx frame abort */
#define HDLC_TX_FRAME_ABORT_BIT		29

/* Stop tx operation on frame abort
 * 1: enable		0: disable (default)*/
#define HDLC_TX_STOP_TX_ON_ABORT_BIT	30

/* 1: transmit channel enabled
 * 0: tx frame started */
#define	HDLC_TX_CHANNEL_ACTIVE_BIT	31



/*=======================================
 * XILINX_CONTROL_RAM_ACCESS_BUF bit map
 *
 * Control RAM data buffer, used to
 * write/read data to/from control
 * (chip) memory.  
 *
 * The address of the memory access
 * must be specified in 
 * XILINX_TIMESLOT_HDLC_CHAN_REG.
 *======================================*/

#define CONTROL_RAM_DATA_MASK		0x0000001F 

/* Used to define the start address of
 * the fifo, per logic channel */
#define HDLC_FIFO_BASE_ADDR_SHIFT        16
#define HDLC_FIFO_BASE_ADDR_MASK         0x1F

/* Used to define the size of the
 * channel fifo */
#define HDLC_FIFO_SIZE_SHIFT           	 8
#define HDLC_FIFO_SIZE_MASK              0x1F

#define HDLC_FREE_LOGIC_CH		 31
#define TRANSPARENT_MODE_BIT		 31

/*=======================================
 * XILINX_DMA_CONTROL_REG bit map
 * 
 * Global DMA control register used to
 * setup DMA engine
 *======================================*/

#define DMA_SIZE_BIT_SHIFT		0

/* Limit used by rx dma engine 
 * to trigger rx dma */
#define DMA_FIFO_HI_MARK_BIT_SHIFT	4

/* Limit used by tx dma engine 
 * to trigger tx dma */
#define DMA_FIFO_LO_MARK_BIT_SHIFT	8
#define DMA_FIFO_T3_MARK_BIT_SHIFT	8

/* Number of active DMA channel 
 * pars tx/rx used: i.e. 1=1tx+1rx */
#define DMA_ACTIVE_CHANNEL_BIT_SHIFT	16
#define DMA_ACTIVE_CHANNEL_BIT_MASK	0xFFE0FFFF

/* Enable or Disable DMA engine
 * 1: enable		0: disable */ 
#define DMA_RX_ENGINE_ENABLE_BIT	30 

/* Enable or Disable DMA engine
 * 1: enable		0: disable */ 
#define DMA_TX_ENGINE_ENABLE_BIT	31 

/* Maximum size of DMA chain for
 * Tx and Rx TE3 adapter 
 *
 * 0=1
 * 1=2
 * ...
 * Current maximumx 15= 16 descriptors
 * per chain.
 */
#define DMA_CHAIN_TE3_MASK		0x0000000F		

/*========================================
 * XILINX_TxDMA_DESCRIPTOR_LO 
 * XILINX_TxDMA_DESCRIPTOR_HI
 *
 * TX DMA descritors Bit Map 
 *=======================================*/

/* Pointer to PC memory, i.e. pointer to
 * start of a tx data frame in PC.  
 * Note: Pointer address must end with binary 00.
 *       i.e. 32 bit alignment */
#define TxDMA_LO_PC_ADDR_PTR_BIT_MASK	0xFFFFFFFC


/* The tx data frame length must defined
 * in increments of 32bits words. The
 * alignment is used to specify real lenght
 * of frames. Offset from the 32bit boundary 
 *
 * Eg: Alignment 0x10 defines the on last
 *     word of tx data frame only two
 *     bytes are valid, and the rest is padding. 
 *
 *          N * 32 bit
 * ----------------------------
 * |                          |
 * |                          |
 * |     |      |      |      |  Last 32bit word of data
 * ----------------------------
 *   11    10     01     00   :  Memory Byte offset (binary)
 *  				 
 *  				 Alignment Value (binary)	 
 *   v      v       v     v   :   00   
 *   p      p       p     v   :   01
 *   p      p       v     v   :   10
 *   p      v       v     v   :   11
 *  
 *   v=valid data byte
 *   p=pad value
 */
#define TxDMA_LO_ALIGNMENT_BIT_MASK	0x00000003


/* Length of tx data dma block 
 * defined in 32bit words increments */
#define TxDMA_HI_DMA_DATA_LENGTH_MASK	0x00000FFF

/* DMA status bits: 11-14
 * 0: Normal Operation */
#define TxDMA_HI_DMA_PCI_ERROR_MASK		0x00007800
#define TxDMA_HI_DMA_PCI_ERROR_M_ABRT		0x00000800
#define TxDMA_HI_DMA_PCI_ERROR_T_ABRT		0x00001000
#define TxDMA_HI_DMA_PCI_ERROR_DS_TOUT		0x00002000
#define TxDMA_HI_DMA_PCI_ERROR_RETRY_TOUT	0x00004000

/* DMA cmd used to initialize fifo */
#define INIT_DMA_FIFO_CMD_BIT		28


/* Used to indicate that the first byte
 * of tx DMA frame should trigger an
 * HDLC frame start flag.
 * 1: Enable  			*/
#define TxDMA_HI_DMA_FRAME_START_BIT	30


/* Used to indicate that the last byte
 * of tx DMA frame should trigger an
 * HDLC frame end flag.
 * 1: Enable  			*/
#define TxDMA_HI_DMA_FRAME_END_BIT	29

/* Used to trigger DMA operation for
 * this channel
 *     Write bit       Read bit
 * 1: Start dma or   dma active 	
 * 0: Stop dma  or   dma stopped */
#define TxDMA_HI_DMA_GO_READY_BIT	31


/* Used to define the start address of
 * the fifo, per logic channel */
#define DMA_FIFO_BASE_ADDR_SHIFT	20
#define DMA_FIFO_BASE_ADDR_MASK		0x1F

/* Used to define the size of the
 * channel fifo */
#define DMA_FIFO_SIZE_SHIFT		15
#define DMA_FIFO_SIZE_MASK		0x1F

/* This mask us used to clear both
 * base addres and fifo size from
 * the DMA descriptor */
#define DMA_FIFO_PARAM_CLEAR_MASK	0xFE007FFF

#define FIFO_32B			0x00
#define FIFO_64B			0x01
#define FIFO_128B			0x03
#define FIFO_256B			0x07
#define FIFO_512B			0x0F
#define FIFO_1024B			0x1F


/*========================================
 * XILINX_RxDMA_DESCRIPTOR_LO 
 * XILINX_RxDMA_DESCRIPTOR_HI
 *
 * RX DMA descritors Bit Map 
 *=======================================*/

/* Pointer to PC memory, i.e. pointer to
 * start of a rx data buffer in PC.  
 * Note: Pointer address must end with binary 00.
 *       i.e. 32 bit alignment */
#define RxDMA_LO_PC_ADDR_PTR_BIT_MASK	0xFFFFFFFC


/* The alignment is used to specify real lenght
 * of a rx data frame. Offset from the 32bit boundary. 
 *
 * Eg: Alignment 10 (binary) indicates that last
 *     word of rx data frame only two
 *     bytes are valid, and the rest is padding. 
 *
 *          N * 32 bit
 * ----------------------------
 * |                          |
 * |                          |
 * |     |      |      |      |  Last 32bit word of data
 * ----------------------------
 *   11    10     01     00   :  Memory Byte offset (binary)
 *  				 
 *  				 Alignment Value (binary)	 
 *   v      v       v     v   :   11   
 *   p      v       v     v   :   01
 *   p      p       v     v   :   10
 *   p      p       p     v   :   00
 *  
 *   v=valid data byte
 *   p=pad value
 *
 *   MaxLength=User specifed maximum rx
 *             buffer size
 *             
 *   Length   =After DMA rx interrupt, length
 *             will indicate number of bytes
 *             left in user buffer
 *             
 *   Alignment=After DMA rx interrupt, indicates
 *             number of padded bytes to 32bit
 *             boundary. 
 * 
 *   Real Frame Len (in bytes)=
 *   	(MaxLength-Length)*4 - ~(Alignment);
 */
#define RxDMA_LO_ALIGNMENT_BIT_MASK	0x00000003


/* Length of rx data dma block 
 * defined in 32bit words increments */
#define RxDMA_HI_DMA_DATA_LENGTH_MASK	0x00000FFF

/* DMA status bits: 16-21
 * 0: Normal Operation */
#define RxDMA_HI_DMA_PCI_ERROR_MASK		0x00007800
#define RxDMA_HI_DMA_PCI_ERROR_M_ABRT           0x00000800
#define RxDMA_HI_DMA_PCI_ERROR_T_ABRT           0x00001000
#define RxDMA_HI_DMA_PCI_ERROR_DS_TOUT          0x00002000
#define RxDMA_HI_DMA_PCI_ERROR_RETRY_TOUT       0x00004000


/* Enable irq if rx fifo is empty */
# define DMA_HI_TE3_IFT_INTR_ENB_BIT	15

/* Not used */
#define RxDMA_HI_DMA_COMMAND_BIT_SHIFT	28


/* Used to indicate that the frame
 * start flags exists, this is the
 * first DMA transfer of an HDLC frame. 
 * 1: Frame start event	0: No frame start */
#define RxDMA_HI_DMA_FRAME_START_BIT	30


/* Indicates that crc error occured 
 * on the last frame 
 * 1: crc error		0: no error */
#define RxDMA_HI_DMA_CRC_ERROR_BIT	25


/* Indicates that HDLC rx abort flag
 * from the line 
 * 1: abort occured	0: normal */
#define RxDMA_HI_DMA_FRAME_ABORT_BIT	26


/* Used to indicate that the frame
 * end flags exists, this is the
 * last DMA transfer of an HDLC frame.
 * 
 * 1: Frame end event	0: No frame end */
#define RxDMA_HI_DMA_FRAME_END_BIT	29

/* Used to trigger DMA operation for
 * this channel
 *     Write bit       Read bit
 * 1: Start dma or   dma active 	
 * 0: Stop dma  or   dma stopped */
#define RxDMA_HI_DMA_GO_READY_BIT	31


/* TE3 Only bit maps for TX and RX
 * Descriptors
 *
 * Interrupt Disable Bit
 * 
 * 1:	Interrpupt doesn't get generated
 * 	after the end of dma descriptor.
 * 
 * 0:   Generate interrupt after
 *      dma descriptor 
 */
#define DMA_HI_TE3_INTR_DISABLE_BIT	27

/* Last Frame Bit
 * 
 * 1:   This dma descr is not last thus
 *      proceed to the next dma chain
 *
 * 0:   This dma descr is last in the
 *      dma chain.  Do not go to the
 *      next dma desc.
 *
 */
#define DMA_HI_TE3_NOT_LAST_FRAME_BIT	24



/*========================================
 * TE3_FRACT_ENCAPSULATION_REG
 *
 * TE3 Fractional Encapsulaton 
 * Control/Status Register   
 *=======================================*/
 

/* RX Section */

/* Read Only
 * Rx Encasulated Frame Size Status
 *
 * Used to determine the tx encasulated
 * frame size 
 */
#define	TE3_RX_FRACT_FRAME_SIZE_SHIFT	0
#define	TE3_RX_FRACT_FRAME_SIZE_MASK	0x000000FF

#ifdef WAN_KERNEL
static __inline unsigned long 
get_te3_rx_fract_frame_size(unsigned long reg)
{
	return reg&TE3_RX_FRACT_FRAME_SIZE_MASK;
}
#endif


/* Read Only
 * Rx Encapsulation CRC Cnt
 *
 * TE3 Fractionalization is a two stage
 * HDLC process. This is a crc count of
 * the first stage!
 */
#define TE3_RX_FRACT_CRC_ERR_CNT_SHIFT	8
#define TE3_RX_FRACT_CRC_ERR_CNT_MASK	0x0000FF00

#ifdef WAN_KERNEL
static __inline unsigned long 
get_te3_rx_fract_crc_cnt(unsigned long reg)
{
        return ((reg&TE3_RX_FRACT_CRC_ERR_CNT_MASK)>>TE3_RX_FRACT_CRC_ERR_CNT_SHIFT);
}
#endif

/* TX Section */

/* Tx Fractional baud rate
 *
 * This is a 14 bit counter, used to
 * specify the period of tx buffers.
 * 
 * The period is used to thorttle the
 * tx speed from 1 to 35Mhz.
 */
#define TE3_TX_FRACT_BAUD_RATE_SHIFT	16
#define TE3_TX_FRACT_BAUD_RATE_MASK     0xFF80FFFF

#ifdef WAN_KERNEL
static __inline void 
set_te3_tx_fract_baud_rate(unsigned int	  *reg, 
			   unsigned long  bitrate_khz, 
                           unsigned int   buffer_size, 
			   unsigned char  e3)
{
	/* Period for T3 and E3 clock in ns */
	unsigned long period_ns=e3?29:22;
	unsigned long data_period_ns=0;
	unsigned long counter=0;

	/* Get the number of bits */
	buffer_size*=8;	
		
	data_period_ns=(buffer_size*1000000)/bitrate_khz;

	counter=data_period_ns/period_ns;

	counter=(counter>>7)&0x7F;

	*reg&=TE3_TX_FRACT_BAUD_RATE_MASK;
	*reg|=(counter<<TE3_TX_FRACT_BAUD_RATE_SHIFT);	
}
#endif

/* Tx Encasulated Frame Size 
 *
 * Used to SET the tx encasulated
 * frame size
 *
 * The Tx size must be equal to the
 * above RX size MINUS 3 bytes.
 * (Because of CRC and address)
 */
#define TE3_TX_FRACT_FRAME_SIZE_SHIFT	24
#define TE3_TX_FRACT_FRAME_SIZE_MASK	0x00FFFFFF

#ifdef WAN_KERNEL
static __inline void
set_te3_tx_fract_frame_size(u32 *reg, unsigned int frame_size)
{
        *reg&=TE3_TX_FRACT_FRAME_SIZE_MASK;
	*reg|=(frame_size-3)<<TE3_TX_FRACT_FRAME_SIZE_SHIFT;
}
#endif



/*=======================================
 *  AFT_TE3_CRNT_DMA_DESC_ADDR_REG
 *
 */

#define AFT_TE3_CRNT_TX_DMA_MASK	0x0000000F

#define AFT_TE3_CRNT_RX_DMA_MASK	0x000000F0
#define AFT_TE3_CRNT_RX_DMA_SHIFT	4

#ifdef WAN_KERNEL
static __inline unsigned int
get_current_rx_dma_ptr(u32 reg)
{
	return ((reg & AFT_TE3_CRNT_RX_DMA_MASK) >> AFT_TE3_CRNT_RX_DMA_SHIFT);
}

static __inline unsigned int
get_current_tx_dma_ptr(u32 reg)
{
	return ((reg & AFT_TE3_CRNT_TX_DMA_MASK));
}

static __inline void
set_current_rx_dma_ptr(u32 *reg, u32 val)
{
	*reg &= ~AFT_TE3_CRNT_RX_DMA_MASK;
	*reg |= (val << AFT_TE3_CRNT_RX_DMA_SHIFT) & AFT_TE3_CRNT_RX_DMA_MASK; 
}

static __inline void
set_current_tx_dma_ptr(u32 *reg, u32 val)
{
	*reg &= ~AFT_TE3_CRNT_TX_DMA_MASK;
	*reg |= val & AFT_TE3_CRNT_TX_DMA_MASK;
}

#endif

/*=======================================
 *  TE3_LOCAL_CONTROL_STATUS_REG
 *
 */

enum {
	WAN_AFT_RED,
	WAN_AFT_GREEN
};

enum {
	WAN_AFT_OFF,
	WAN_AFT_ON
};

#define TE3_D5_RED	0
#define TE3_D5_GREEN	1
#define TE3_D6_RED	2
#define TE3_D6_GREEN	3

#ifdef WAN_KERNEL
static __inline void
aft_te3_set_led(unsigned int color, int led_pos, int on, u32 *reg)
{
	if (led_pos == 0){
		if (color == WAN_AFT_RED){
			if (on == WAN_AFT_OFF){
				wan_set_bit(TE3_D5_RED,reg);
			}else{
				wan_clear_bit(TE3_D5_RED,reg);
			}
		}else{
			if (on == WAN_AFT_OFF){
				wan_set_bit(TE3_D5_GREEN,reg);
			}else{
				wan_clear_bit(TE3_D5_GREEN,reg);
			}
		}
	}else{
		if (color == WAN_AFT_RED){
			if (on == WAN_AFT_OFF){
				wan_set_bit(TE3_D6_RED,reg);
			}else{
				wan_clear_bit(TE3_D6_RED,reg);
			}
		}else{
			if (on == WAN_AFT_OFF){
				wan_set_bit(TE3_D6_GREEN,reg);
			}else{
				wan_clear_bit(TE3_D6_GREEN,reg);
			}
		}
	}
}
#endif





/*===============================================
 */


typedef struct xilinx_config
{
	unsigned long xilinx_chip_cfg_reg;
	unsigned long xilinx_dma_control_reg; 

}xilinx_config_t;


/* Default length of the DMA burst
 * Value indicates the maximum number
 * of DMA transactions per burst */
#define XILINX_DMA_SIZE		10	

/* Used by DMA engine to transfer data from Fifo to PC.
 * DMA engine start rx when fifo level becomes greater
 * than the DMA FIFI UP value */
#define XILINX_DMA_FIFO_UP	8	

/* Used by DMA engine to transfer data from PC to Fifo.
 * DMA engine start tx when free space in the fifo
 * becomes greater than DMA FIFO LO value */
#define XILINX_DMA_FIFO_LO	8

/* Used by DMA engine to transfer data from PC to Fifo.
 * DMA engine start tx/rx when fifo is greater/lower
 * than the DMA FIFO MARK value */
#define AFT_T3_DMA_FIFO_MARK	8

/* Default Active DMA channel used by the
 * DMA Engine */
#define XILINX_DEFLT_ACTIVE_CH  0

#define MAX_XILINX_TX_DMA_SIZE  0xFFFF

#define MIN_WP_PRI_MTU		10	
#define DEFAULT_WP_PRI_MTU 	1500

/* Maximum MTU for AFT card
 * 8192-4=8188.  This is a hardware
 * limitation. 
 */
#define MAX_WP_PRI_MTU		8188


enum {
#if defined(__LINUX__)
	SDLA_HDLC_READ_REG = SIOC_ANNEXG_PLACE_CALL,
#else
	SDLA_HDLC_READ_REG = 0, /* FIXME !!!*/
#endif
	SDLA_HDLC_WRITE_REG,
	SDLA_HDLC_SET_PCI_BIOS
};

#define MAX_DATA_SIZE 2000
struct sdla_hdlc_api{
        unsigned int  cmd;
        unsigned short len;
        unsigned char  bar;
        unsigned short offset;
        unsigned char data[MAX_DATA_SIZE];
};

struct wan_aften_api{
	unsigned int  cmd;
        unsigned short len;
        unsigned char  bar;
        unsigned short offset;
        unsigned char data[MAX_DATA_SIZE];
};

/*==========================================
 * Board CPLD Interface Section
 *
 *=========================================*/


#define PMC_CONTROL_REG		0x00

/* Used to reset the pcm 
 * front end 
 *    0: Reset Enable 
 *    1: Normal Operation 
 */
#define PMC_RESET_BIT		0	

/* Used to set the pmc clock
 * source:
 *   0 = E1
 *   1 = T1 
 */
#define PMC_CLOCK_SELECT	1

#define LED_CONTROL_REG		0x01

#define JP8_VALUE               0x02
#define JP7_VALUE               0x01
#define SW0_VALUE               0x04
#define SW1_VALUE               0x08


#define SECURITY_CPLD_REG       0x09
#define CUSTOMER_CPLD_ID_REG	0x0A

#define SECURITY_CPLD_MASK	0x03
#define SECURITY_CPLD_SHIFT	0x02

#define SECURITY_1LINE_UNCH	0x00
#define SECURITY_1LINE_CH	0x01
#define SECURITY_2LINE_UNCH	0x02
#define SECURITY_2LINE_CH	0x03

/* -------------------------------------- */

#define WRITE_DEF_SECTOR_DSBL   0x01
#define FRONT_END_TYPE_MASK     0x38

#define BIT_DEV_ADDR_CLEAR      0x600
#define BIT_DEV_ADDR_CPLD       0x200

#define MEMORY_TYPE_SRAM        0x00
#define MEMORY_TYPE_FLASH       0x01
#define MASK_MEMORY_TYPE_SRAM   0x10
#define MASK_MEMORY_TYPE_FLASH  0x20

#define BIT_A18_SECTOR_SA4_SA7  0x20
#define USER_SECTOR_START_ADDR  0x40000

#define MAX_TRACE_QUEUE           100

#define MAX_TRACE_BUFFER	(MAX_LGTH_UDP_MGNT_PKT - 	\
				 sizeof(iphdr_t) - 		\
	 			 sizeof(udphdr_t) - 		\
				 sizeof(wan_mgmt_t) - 		\
				 sizeof(wan_trace_info_t) - 	\
		 		 sizeof(wan_cmd_t))

 

typedef struct wp_rx_element
{
	unsigned int dma_addr;
	unsigned int reg;
	unsigned int align;
	unsigned char  pkt_error;
}wp_rx_element_t;


#if defined(WAN_KERNEL)

static __inline unsigned short xilinx_valid_mtu(unsigned short mtu)
{
	if (mtu <= 128){
		return 128;
	}else if (mtu <= 256){
		return 256;
	}else if (mtu <= 512){
		return 512;
	}else if (mtu <= 1024){
		return 1024;
	}else if (mtu <= 2048){
		return 2048;
	}else if (mtu <= 4096){
		return 4096;
	}else if (mtu <= 8188){
		return 8188;
	}else{
		return 0;
	}	
}

static __inline unsigned short xilinx_dma_buf_bits(unsigned short dma_bufs)
{
	if (dma_bufs < 2){
		return 0;
	}else if (dma_bufs < 3){
		return 1;
	}else if (dma_bufs < 5){
		return 2;
	}else if (dma_bufs < 9){
		return 3;
	}else if (dma_bufs < 17){
		return 4;
	}else{
		return 0;
	}	
}

#define AFT_TX_TIMEOUT 5
#define AFT_RX_TIMEOUT 2
#define AFT_MAX_WTD_TIMEOUT 2
#define AFT_IFT_FIMR_VER 0x11

static __inline void aft_reset_rx_watchdog(sdla_t *card)
{
	card->hw_iface.bus_write_1(card->hw,AFT_TE3_RX_WDT_CTRL_REG,0);
}

static __inline void aft_enable_rx_watchdog(sdla_t *card, unsigned char timeout)
{
	aft_reset_rx_watchdog(card);

	/* Rx Watchdog is not used if firmware supports IFT interrupt */
#ifdef AFT_TE3_IFT_FEATURE_DISABLE 
#warning "AFT_TE3_IFT_FEATURE_DISABLE is defined!"   
    card->hw_iface.bus_write_1(card->hw,AFT_TE3_RX_WDT_CTRL_REG,timeout);
#else
	if (card->u.aft.firm_ver < AFT_IFT_FIMR_VER) {
		card->hw_iface.bus_write_1(card->hw,AFT_TE3_RX_WDT_CTRL_REG,timeout);
	}
#endif

}

static __inline void aft_reset_tx_watchdog(sdla_t *card)
{
	card->hw_iface.bus_write_1(card->hw,AFT_TE3_TX_WDT_CTRL_REG,0);
}

static __inline void aft_enable_tx_watchdog(sdla_t *card, unsigned char timeout)
{
	aft_reset_tx_watchdog(card);
	card->hw_iface.bus_write_1(card->hw,AFT_TE3_TX_WDT_CTRL_REG,timeout);	
}


#endif /* WAN_KERNEL */



#endif
