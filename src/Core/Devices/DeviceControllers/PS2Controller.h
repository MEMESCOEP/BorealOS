#ifndef PS2CONTROLLER_H
#define PS2CONTROLLER_H

/* DEFINITIONS */
#define PS2_MAX_SELFTEST_TRIES 5
#define PS2_CMD_STATUS_PORT 0x64
#define PS2_DATA_PORT 0x60


/* FUNCTIONS */
void InitPS2Controller();
void PS2Wait(uint8_t BitToCheck, bool WaitForSet);
//bool PS2ControllerExists();

#endif