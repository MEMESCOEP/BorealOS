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
extern int InputBuffer[2];
extern int LastReleasedInput;
extern int LastInput;


/* FUNCTIONS */
unsigned char GetMappedKey(bool WaitForData);
bool IsCharacterKey(uint8_t Scancode);
int NumpadScancodeToCharScancode(int ScancodeToChange, unsigned char* Keymap);
int PS2SetScancodeSet(int ScancodeSet);
int GetPS2Scancode();
void PS2KBWaitForData();
void InitPS2Keyboard();
void PS2KBSetLEDs(bool NumLock, bool CapsLock, bool ScrollLock);
void PS2KBHandleScancode();

#endif