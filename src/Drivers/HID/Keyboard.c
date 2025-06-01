/* BASIC KEYBOARD DEVICE DRIVER */
// This is a basic keyboard driver.


/* LIBRARIES */
#include <Utilities/StrUtils.h>
#include <Drivers/HID/Keyboard.h>
#include <Core/Graphics/Console.h>
#include <Core/IO/PS2Controller.h>
#include <Core/IO/RegisterIO.h>
#include <Core/IO/PIC.h>
//#include "Kernel.h"


/* VARIABLES */
bool NewKeyIsAvailable = false;
bool ScrollLockToggled = false;
bool CapsLockToggled = false;
bool NumLockToggled = false;
bool RightControlPressed = false;
bool RightShiftPressed = false;
bool RightAltPressed = false;
bool LeftControlPressed = false;
bool LeftShiftPressed = false;
bool LeftAltPressed = false;
int InputBuffer[2] = {0x00, 0x00};
int InvalidScancodeCount = 2;
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
// Convert a scancode from set 1 into a value in the keymap, taking shifting into account
unsigned char GetMappedKey(bool WaitForData)
{
    if (WaitForData == true)
    {
        while (NewKeyIsAvailable == false) asm volatile ("hlt");
        NewKeyIsAvailable = false;
    }

    // Handle numpad cases
    if (LastInput >= 0x47 && LastInput <= 0x53)
    {
        if (LastInput == 0x4E)
        {
            return NumpadScancodeToCharScancode(LastInput, KBScanmapShifted);
        }
        else
        {
            return NumpadScancodeToCharScancode(LastInput, KBScanmapUnshifted);
        }        
    }

    // Regular keys
    else if (CapsLockToggled == true || LeftShiftPressed == true || RightShiftPressed == true)
    {
        return KBScanmapShifted[LastInput];
    }
    else
    {
        return KBScanmapUnshifted[LastInput];
    }
}

// Check if the scancode corresponds to a valid character key for scancode map 1
// Check the scancode set 1 at https://wiki.osdev.org/PS/2_Keyboard for more info
bool IsCharacterKey(uint8_t Scancode)
{
    // Remove 0x80 from the scancode if it's over that in order to handle released keys
    int CorrectedScancode = Scancode > 0x80 ? (Scancode - 0x80) : Scancode;

    // Space bar - keypad *
    if (Scancode == 0x37 || Scancode == 0x39)
        return true;

    // 1-=
    if (Scancode >= 0x02 && Scancode <= 0x0D)
        return true;

    // Q-]
    if (Scancode >= 0x10 && Scancode <= 0x1B)
        return true;

    // A-`
    if (Scancode >= 0x1E && Scancode <= 0x29)
        return true;

    // \-/
    if (Scancode >= 0x2B && Scancode <= 0x35)
        return true;

    // Keypad 7 - keypad .
    if (Scancode >= 0x47 && Scancode <= 0x53)
    {
        if (NumLockToggled == true || Scancode == 0x4C || Scancode == 0x4A || Scancode == 0x4E)
        {
            return true;
        }
        else
        {
            return false;
        }
    }

    return false;  // Not a character key
}

// Convert a numpad scancode into a normal scancode if numlock is pressed, or convert
// it to an "action key" scancode (Home, End, Arrow keys, etc)
int NumpadScancodeToCharScancode(int ScancodeToChange, unsigned char* Keymap)
{
    // If numlock is on, we need to handle keys like they're characters
    if (NumLockToggled == false && ScancodeToChange != 0x4C && ScancodeToChange != 0x4A && ScancodeToChange != 0x4E)
    {
        return ScancodeToChange;
    }
    else
    {
        // Numpad 7-9
        if (ScancodeToChange >= 0x47 && ScancodeToChange <= 0x49)
        {
            return Keymap[ScancodeToChange - 0x3F];
        }

        // Numpad 4-6
        else if (ScancodeToChange >= 0x4B && ScancodeToChange <= 0x4D)
        {
            return Keymap[ScancodeToChange - 0x46];
        }

        // Numpad 1-3
        else if (ScancodeToChange >= 0x4F && ScancodeToChange <= 0x51)
        {
            return Keymap[ScancodeToChange - 0x4D];
        }

        // Numpad 0
        if (ScancodeToChange == 0x52)
        {
            return Keymap[ScancodeToChange - 0x47];
        }

        // Numpad .
        if (ScancodeToChange == 0x53)
        {
            return Keymap[ScancodeToChange - 0x1F];
        }

        // Numpad -
        if (ScancodeToChange == 0x4A)
        {
            return Keymap[ScancodeToChange - 0x3E];
        }

        // Numpad +
        if (ScancodeToChange == 0x4E)
        {
            return Keymap[ScancodeToChange - 0x41];
        }

        // Unknown scancode, just return it
        return ScancodeToChange;
    }
}

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

void PrintScancodeAsHex(int Scancode)
{
    char ScancodeBuffer[4];
    IntToStr(Scancode, ScancodeBuffer, 16);
    ConsolePutString("0x");
    ConsolePutString(ScancodeBuffer);
}

// Wait for the status register to have bit 1 set, which means that the input buffer is full
void PS2KBWaitForData()
{
    int Timeout = 10000;

    while ((InB(KEYBOARD_COMMAND_REG) & 0x01) == 0)
    {
        if (Timeout <= 0)
        {
            break;
        }

        Timeout--;
    }
}

// Set the PS/2 keyboard's scancode set to 1 and unmask PIC IRQ 1
void InitPS2Keyboard()
{
    LOG_KERNEL_MSG("\tConfiguring interrupt handler...\n\r", NONE);
    IDTSetIRQHandler(1, PS2KBHandleScancode);

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

    
    //ConsolePutString("[INFO] >> Setting PS/2 keyboard LEDs...\n\r");
    //PS2KBSetLEDs(NumLockToggled, CapsLockToggled, ScrollLockToggled);

    LOG_KERNEL_MSG("\tUnmasking keyboard interrupt #1...\n\r", NONE);
    PICClearIRQMask(1);
    
    NewKeyIsAvailable = false;
    LastReleasedInput = 0;
    LastInput = 0;

    LOG_KERNEL_MSG("\tPS/2 keyboard init finished.\n\n\r", NONE);
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
void PS2KBHandleScancode()
{
    uint8_t status = InB(0x64);

    if (!(status & 0x20)) // Keyboard data ready
    {
        uint8_t Scancode = InB(0x60);

        // Unknown data that seems to be sent only once after init
        if ((Scancode == 0x03 || Scancode == 0x07) && InvalidScancodeCount > 0)
        {
            InvalidScancodeCount--;
            return;
        }

        if (Scancode < 0x80)
            ConsolePutChar(KBScanmapUnshifted[Scancode]);
    }

    /*NewKeyIsAvailable = true;

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

        case 0x0:
            break;

        default:
            LastInput = Scancode;
            break;
    }*/
}