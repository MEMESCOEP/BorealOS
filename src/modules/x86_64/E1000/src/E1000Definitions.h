#ifndef BOREALOS_E1000DEFINITIONS_H
#define BOREALOS_E1000DEFINITIONS_H

#define E1000_VENDOR_ID 0x8086
#define E1000_DEVICE_ID 0x100E

// E1000 register offsets
#define E1000_REGISTER_INTERRUPT_MASK_SET 0x00D0
#define E1000_REGISTER_INTERRUPT_CAUSE 	  0x00C0
#define E1000_REGISTER_CONTROL 		      0x0000
#define E1000_REGISTER_EEPROM   	      0x0014
#define E1000_REGISTER_STATUS 		      0x0008

// Control register and its bits
#define E1000_CONTROL_AUTO_SPEED_DETECT (1 << 5)
#define E1000_CONTROL_SET_LINK_UP 		(1 << 6)
#define E1000_CONTROL_RESET			 	(1 << 26)

// EEPROM
#define E1000_EEPROM_READ_START (1 << 0)
#define E1000_EEPROM_READ_DONE  (1 << 4)

// MMIO register read/write
uint32_t ReadRegister(uint32_t offset);
void WriteRegister(uint32_t offset, uint32_t value);

#endif