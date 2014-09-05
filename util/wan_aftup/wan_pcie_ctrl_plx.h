#ifndef __WAN_PCIE_CTRL_PLX_H
#define __WAN_PCIE_CTRL_PLX_H


//This is the address of the ECCTL register , page 149 of PEX8111 datasheet.
#define EECTL 0x1004	
#define BIT_EECTL_RELOAD	0x80000000

//Change this accordingly.!!!
#define EEPROM_SIZE 0xFF

#define EEPROM_BUSY 19
#define EEPROM_CS_ENABLE 18
#define EEPROM_BYTE_READ_START 17
#define EEPROM_READ_DATA 8
#define EEPROM_WRITE_DATA 0
#define EEPROM_BYTE_WRITE_START 16

//EEPROM COMMANDS
#define READ_STATUS_EE_OPCODE	0x05
#define WREN_EE_OPCODE 		0x06
#define WRITE_EE_OPCODE		0x02
#define READ_EE_OPCODE		0x03





#endif    /* __WAN_PCIE_CTRL_PLX_H */
