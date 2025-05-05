/* BASIC KEYBOARD DEVICE DRIVER */
// This is a basic keyboard driver.


/* LIBRARIES */
#include "Core/Graphics/Terminal.h"
#include "Core/Devices/DeviceControllers/PS2Controller.h"
#include "Core/Devices/PIC.h"
#include "Core/IO/RegisterIO.h"
#include "Keyboard.h"
#include "Kernel.h"


/* VARIABLES */
bool ScrollLockToggled = false;
bool CapsLockToggled = false;
bool NumLockToggled = false;
bool RightControlPressed = false;
bool RightShiftPressed = false;
bool RightAltPressed = false;
bool LeftControlPressed = false;
bool LeftShiftPressed = false;
bool LeftAltPressed = false;
bool KeyPressed = false;
int LastReleasedInput = 0;
int LastInput = 0;

// The following scancode sets only work for scancode set 1
unsigned char KBScanmapUnshifted[128] =
{
    0,  27, '1', '2', '3', '4', '5', '6', '7', '8',
    '9', '0', '-', '=', '\b',                               /* Backspace */
    '\t',                                                   /* Tab */
    'q', 'w', 'e', 'r',                                     /* 19 */
    't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',           /* Enter key */
    0,                                                      /* 29   - Control */
    'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';',       /* 39 */
    '\'', '`',   0,                                         /* Left shift */
    '\\', 'z', 'x', 'c', 'v', 'b', 'n',                     /* 49 */
    'm', ',', '.', '/',   0,                                /* Right shift */
    '*',
    0,                                                      /* Alt */
    ' ',                                                    /* Space bar */
    0,                                                      /* Caps lock */
    0,                                                      /* 59 - F1 key ... > */
    0,   0,   0,   0,   0,   0,   0,   0,
    0,                                                      /* < ... F10 */
    0,                                                      /* 69 - Num lock*/
    0,                                                      /* Scroll Lock */
    0,                                                      /* Home key */
    0,                                                      /* Up Arrow */
    0,                                                      /* Page Up */
    '-',
    0,                                                      /* Left Arrow */
    0,
    0,                                                      /* Right Arrow */
    '+',
    0,                                                      /* 79 - End key*/
    0,                                                      /* Down Arrow */
    0,                                                      /* Page Down */
    0,                                                      /* Insert Key */
    0,                                                      /* Delete Key */
    0,   0,   0,
    0,                                                      /* F11 Key */
    0,                                                      /* F12 Key */
    0,                                                      /* All other keys are undefined */
};

unsigned char KBScanmapShifted[128] =
{
    0,  27, '!', '@', '#', '$', '%', '^', '&', '*',
    '(', ')', '_', '+', '\b',                               /* Backspace */
    '\t',                                                   /* Tab */
    'Q', 'W', 'E', 'R',                                     /* 19 */
    'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n',           /* Enter key */
    0,                                                      /* 29   - Control */
    'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':',       /* 39 */
    '"', '~',   0,                                          /* Left shift */
    '|', 'Z', 'X', 'C', 'V', 'B', 'N',                      /* 49 */
    'M', '<', '>', '?',   0,                                /* Right shift */
    '*',
    0,                                                      /* Alt */
    ' ',                                                    /* Space bar */
    0,                                                      /* Caps lock */
    0,                                                      /* 59 - F1 key ... > */
    0,   0,   0,   0,   0,   0,   0,   0,
    0,                                                      /* < ... F10 */
    0,                                                      /* 69 - Num lock*/
    0,                                                      /* Scroll Lock */
    0,                                                      /* Home key */
    0,                                                      /* Up Arrow */
    0,                                                      /* Page Up */
    '-',
    0,                                                      /* Left Arrow */
    0,
    0,                                                      /* Right Arrow */
    '+',
    0,                                                      /* 79 - End key*/
    0,                                                      /* Down Arrow */
    0,                                                      /* Page Down */
    0,                                                      /* Insert Key */
    0,                                                      /* Delete Key */
    0,   0,   0,
    0,                                                      /* F11 Key */
    0,                                                      /* F12 Key */
    0,                                                      /* All other keys are undefined */
};


/* FUNCTIONS */
// Change the scancode set that the PS/2 keyboard uses. Valid values are: 0x01, 0x02, 0x03
int PS2SetScancodeSet(int ScancodeSet)
{
    // Start scancode set selection
    OutB(0x60, 0xF0);
    PS2Wait(0x01, true);
    if (InB(0x60) != 0xFA) return -1;

    // Send the scancode set
    OutB(0x60, ScancodeSet);
    PS2Wait(0x01, true);
    if (InB(0x60) != 0xFA) return -2;

    // The operation suceeded, return a success code
    return 0;
}

int GetPS2Scancode()
{
    unsigned char Status = InB(KEYBOARD_COMMAND_REG);

    // The lowest bit of 'Status' will be set if buffer is not empty.
    if (Status & 0x01)
    {
        return InB(KEYBOARD_DATA_REG);
    }

    return 0;
}

// Wait for the status register to have bit 1 set, which means that the input buffer is full
void PS2KBWaitForData()
{    
    while ((InB(KEYBOARD_COMMAND_REG) & 0x01) == 0);
}

// Set the PS/2 keyboard's scancode set to 1 and unmask PIC IRQ 1
void InitPS2Keyboard()
{
    // Set the keyboard scancode set to 1 and unmask IRQ1
    int ScancodeSetResult = PS2SetScancodeSet(0x01);

    switch (ScancodeSetResult)
    {
        case 0:
            break;

        case 1:
            KernelPanic(ScancodeSetResult, "PS/2 keyboard init failed, couldn't enter scancode selection mode.");
            break;

        case 2:
            KernelPanic(ScancodeSetResult, "PS/2 keyboard init failed, couldn't set scancode.");
            break;

        default:
            KernelPanic(ScancodeSetResult, "PS/2 keyboard init failed, unknown error.");
            break;
    }

    TerminalDrawString("[INFO] >> Unmasking PS/2 keyboard interrupt #1...\n\r");
    PICClearIRQMask(1);
    
    //TerminalDrawString("[INFO] >> Setting PS/2 keyboard LEDs...\n\r");
    //PS2KBSetLEDs(NumLockToggled, CapsLockToggled, ScrollLockToggled);
}

// Turn the keyboard LEDs on/off with booleans
void PS2KBSetLEDs(bool NumLock, bool CapsLock, bool ScrollLock)
{
    // Create the byte. 0x00 means all LEDs are off.
    uint8_t LEDByte = 0x00;

    // Set the corresponding bits based on the booleans
    if (ScrollLock)
    {
        LEDByte |= 0x01;
    }

    if (NumLock)
    {
        LEDByte |= 0x02;
    }

    if (CapsLock)
    {
        LEDByte |= 0x04;
    }

    OutB(0x60, 0xED);
    PS2KBWaitForData();
    OutB(0x60, LEDByte);
}

// Handle a scancode that a PS/2 keyboard generated
void PS2KBHandleScancode(int Scancode)
{
    switch (Scancode)
    {
        // Left control press/release.
        case 0x1D:
            LeftControlPressed = true;
            break;

        case 0x9D:
            LeftControlPressed = false;
            break;

        // Left shift press/release.
        case 0x2A:
            LeftShiftPressed = true;
            break;

        case 0xAA:
            LeftShiftPressed = false;
            break;

        // Left alt press/release.
        case 0x38:
            LeftAltPressed = true;
            break;

        case 0xB8:
            LeftAltPressed = false;
            break;

        case 0xE0:
            PS2KBWaitForData();
            Scancode = GetPS2Scancode();
            LastInput = Scancode;

            // Right control press/release.
            if (Scancode == 0x1D)
            {
                RightControlPressed = true;
            }
            else if (Scancode == 0x9D)
            {
                RightControlPressed = false;
            }

            // Right alt press/release.
            else if (Scancode == 0x38)
            {
                RightAltPressed = true;
            }
            else if (Scancode == 0xB8)
            {
                RightAltPressed = false;
            }
            
            break;

        // Right shift press/release.
        case 0x36:
            RightShiftPressed = true;
            break;

        case 0xB6:
            RightShiftPressed = false;
            break;		
            
        // Toggle num lock and set the keyboard LEDs accordingly.
        case 0x45:
            NumLockToggled = !NumLockToggled;
            PS2KBSetLEDs(NumLockToggled, CapsLockToggled, ScrollLockToggled);
            break;

        // Toggle caps lock and set the keyboard LEDs accordingly.
        case 0x3A:
            CapsLockToggled = !CapsLockToggled;
            PS2KBSetLEDs(NumLockToggled, CapsLockToggled, ScrollLockToggled);
            break;

        // Toggle scroll lock and set the keyboard LEDs accordingly.
        case 0x46:
            ScrollLockToggled = !ScrollLockToggled;
            PS2KBSetLEDs(NumLockToggled, CapsLockToggled, ScrollLockToggled);
            break;

        case 0xE1:
            PS2KBWaitForData();
            Scancode = GetPS2Scancode();

            // The pause/break key was pressed, we need to read the next four bytes so the driver doesn't erroneously interpret false keystrokes.
            if (Scancode == 0x1D)
            {
                for (int I = 0; I < 4; I++)
                {
                    PS2KBWaitForData();
                    Scancode = GetPS2Scancode();
                }
            }
            
            LastInput = Scancode;
            break;

        default:
            break;
    }

    // "Key pressed" scancodes are all less than 0x80 for scancode set 1
    if (Scancode < 0x80)
    {
        LastInput = Scancode;
    }
    else
    {
        LastReleasedInput = Scancode;
    }
}