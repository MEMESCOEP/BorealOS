#ifndef BOREALOS_PS2_DEFINITIONS_H
#define BOREALOS_PS2_DEFINITIONS_H

// IRQs
#define KEYBOARD_IRQ 1
#define MOUSE_IRQ    12

// Ports
#define STATUS_CMD 0x64
#define DATA       0x60

// Controller commands
#define WRITE_CONFIG_BYTE_CMD 0x60
#define READ_CONFIG_BYTE_CMD  0x20
#define CONTROLLER_SELF_TEST  0xAA
#define PERIPHERAL_IDENTIFY   0xF2
#define PERIPHERAL_RESET      0xFF
#define SELECT_PORT_2         0xD4
#define PORT_1_ENABLE         0xAE
#define PORT_2_ENABLE         0xA8
#define PORT_1_DISABLE        0xAD
#define PORT_2_DISABLE        0xA7
#define PORT_1_TEST           0xAB
#define PORT_2_TEST           0xA9

// Controller responses
#define CONTROLLER_SELF_TEST_PASSED 0x55
#define PERIPHERAL_RESET_NEXTCMD_ID 0xAA
#define PERIPHERAL_RESET_FAILED     0xFC
#define CONTROLLER_ACK              0xFA
#define DATA_RESEND                 0xFE

#endif // BOREALOS_PS2_DEFINITIONS_H