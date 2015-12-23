#ifndef __WAN_AFT_FLASH_T116_H__
#define __WAN_AFT_FLASH_T116_H__

#if defined(__WINDOWS__)
# include "wanpipe_time.h"	/* usleep() */
#endif

#define FPGA_ID         0x00
#define FW_VERSION      0x01
#define FPGA_CTRL_STAT  0x02
#define DCM_STATUS      0x03
#define FLASH_CTRL_REG  0x04
#define FLASH_DATA_REG  0x05

#define M25P40_COMMAND_WREN      0x06 /* write enable */
#define M25P40_COMMAND_WRDI      0x04 /* write disable */
#define M25P40_COMMAND_RDID      0x9F /* read identification */
#define M25P40_COMMAND_RDSR      0x05 /* read status register */
#define M25P40_COMMAND_WRSR      0x01 /* write status register */
#define M25P40_COMMAND_READ      0x03 /* read data bytes */
#define M25P40_COMMAND_FAST_READ 0x0B /* fast read */
#define M25P40_COMMAND_PP        0x02 /* page program */
#define M25P40_COMMAND_SE        0xD8 /* sector erase */
#define M25P40_COMMAND_BE        0xC7 /* bulk erase */
#define M25P40_COMMAND_DP        0xB9 /* deep power-down */
#define M25P40_COMMAND_RES       0xAB /* Release from deep power-down */


#define  M25P40_SR_BIT_WIP    (1<<0)
#define  M25P40_SR_BIT_WEL    (1<<1)
#define  M25P40_SR_BIT_BP0    (1<<2)
#define  M25P40_SR_BIT_BP1    (1<<3)
#define  M25P40_SR_BIT_BP2    (1<<4)
#define  M25P40_SR_BIT_SRWD   (1<<7)

#define  EEPROM_READY_BIT     (1<<22) 

#define M25P40_ID_MF    0x20     /* manufacturer ID */
#define M25P40_ID_MT    0x20     /* memory type */
#define M25P40_ID_MC    0x13     /* memory capacity */

#define EEPROM_CNTRL_REG 	0x1080
#define EEPROM_DATA_REG_OUT 	0x1084
#define EEPROM_DATA_REG_IN 	0x1088


#define MAX_RETRIES 2000
#define M25P40_FLASH_SIZE 524288


#define ST_FLASH_DELAY usleep(5);


#define AFT_T116_FLASH_VER_OFF	0x7FFFF





























#endif /*__WAN_AFT_FLASH_T116_H__*/
