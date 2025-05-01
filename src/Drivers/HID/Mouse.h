#ifndef MOUSE_H
#define MOUSE_H

/* DEFINITIONS */
#define PS2_MOUSE_STATUS_REG 0x64
#define PS2_MOUSE_DATA_REG   0x60
#define PS2_MOUSE_TIMEOUT    100000
#define PS2_MOUSE_ABIT       0x02
#define PS2_MOUSE_BBIT       0x01
#define PS2_MOUSE_WRITE      0xD4
#define PS2_MOUSE_F_BIT      0x20
#define PS2_MOUSE_V_BIT      0x08
#define PS2_MOUSE_ACK        0xFA


/* VARIABLES */
extern uint8_t MouseID;
extern int MousePos[2];
extern int XSensitivity;
extern int YSensitivity;


/* FUNCTIONS */
void InitPS2Mouse();
void MouseWait(uint8_t MType);

#endif