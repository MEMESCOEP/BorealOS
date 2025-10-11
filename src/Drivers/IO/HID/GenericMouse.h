#ifndef BOREALOS_GENERICMOUSE_H
#define BOREALOS_GENERICMOUSE_H

#include <Definitions.h>

typedef struct {
    int64_t PosX;
    int64_t PosY;
    int8_t ScrollDeltaX;
    int8_t ScrollDeltaY;
    bool WillSendFourPackets;
    bool IsFiveButtonMouse;
    bool FifthButtonDown;
    bool FourthButtonDown;
    bool MiddleClickDown;
    bool RightClickDown;
    bool LeftClickDown;
} MouseState;

#endif //BOREALOS_GENERICMOUSE_H