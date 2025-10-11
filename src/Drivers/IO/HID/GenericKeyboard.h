#ifndef BOREALOS_GENERICKEYBOARD_H
#define BOREALOS_GENERICKEYBOARD_H

#include <Definitions.h>

typedef enum {
    SCANCODE_SET_1 = 0,
    SCANCODE_SET_2 = 1,
} ScancodeSet;

typedef struct {
    ScancodeSet CurrentScancodeSet;
    bool ScrollLockOn;
    bool CapsLockOn;
    bool NumLockOn;
    bool ShiftPressed;
    bool CtrlPressed;
    bool AltPressed;
} KeyboardState;

#endif //BOREALOS_GENERICKEYBOARD_H