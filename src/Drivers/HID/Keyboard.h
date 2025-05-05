#ifndef KEYBOARD_H
#define KEYBOARD_H

/* LIBRARIES */
#include <stdbool.h>


/* DEFINITIONS */
#define KEYBOARD_COMMAND_REG 0x64
#define KEYBOARD_DATA_REG    0x60


/* VARIABLES */
extern unsigned char KBScanmapUnshifted[128];
extern unsigned char KBScanmapShifted[128];
extern int InputBuffer[8];
extern bool ScrollLockToggled;
extern bool CapsLockToggled;
extern bool NumLockToggled;
extern bool RightControlPressed;
extern bool RightShiftPressed;
extern bool RightAltPressed;
extern bool LeftControlPressed;
extern bool LeftShiftPressed;
extern bool LeftAltPressed;
extern bool KeyPressed;
extern int LastReleasedInput;
extern int LastInput;


/* FUNCTIONS */
int PS2SetScancodeSet(int ScancodeSet);
int GetPS2Scancode();
void PS2KBWaitForData();
void InitPS2Keyboard();
void PS2KBSetLEDs(bool NumLock, bool CapsLock, bool ScrollLock);
void PS2KBHandleScancode(int Scancode);

#endif