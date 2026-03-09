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

// Scancode set 2 modifier bytes
#define KEYBOARD_EXTENDED_MODIFIER 0xE0
#define KEYBOARD_RELEASE_MODIFIER  0xF0

// Scancode set 2 special keys
#define SPECIAL_KEY_SCROLLLOCK 0x7E
#define SPECIAL_KEY_CAPSLOCK   0x58
#define SPECIAL_KEY_NUMLOCK    0x77

// Keyboard commands
#define KEYBOARD_SET_LEDS 0xED

// Set 2 regular scancode lookup table (index is the scancode)
static const HID::KeyCode Set2KeyCodes[] = {
    HID::KeyCode::KeyNone,        // 0x00
    HID::KeyCode::F9,             // 0x01
    HID::KeyCode::KeyNone,        // 0x02
    HID::KeyCode::F5,             // 0x03
    HID::KeyCode::F3,             // 0x04
    HID::KeyCode::F1,             // 0x05
    HID::KeyCode::F2,             // 0x06
    HID::KeyCode::F12,            // 0x07
    HID::KeyCode::KeyNone,        // 0x08
    HID::KeyCode::F10,            // 0x09
    HID::KeyCode::F8,             // 0x0A
    HID::KeyCode::F6,             // 0x0B
    HID::KeyCode::F4,             // 0x0C
    HID::KeyCode::Tab,            // 0x0D
    HID::KeyCode::GraveTilde,     // 0x0E
    HID::KeyCode::KeyNone,        // 0x0F
    HID::KeyCode::KeyNone,        // 0x10
    HID::KeyCode::LeftAlt,        // 0x11
    HID::KeyCode::LeftShift,      // 0x12
    HID::KeyCode::KeyNone,        // 0x13
    HID::KeyCode::LeftControl,    // 0x14
    HID::KeyCode::Q,              // 0x15
    HID::KeyCode::Num1,           // 0x16
    HID::KeyCode::KeyNone,        // 0x17
    HID::KeyCode::KeyNone,        // 0x18
    HID::KeyCode::KeyNone,        // 0x19
    HID::KeyCode::Z,              // 0x1A
    HID::KeyCode::S,              // 0x1B
    HID::KeyCode::A,              // 0x1C
    HID::KeyCode::W,              // 0x1D
    HID::KeyCode::Num2,           // 0x1E
    HID::KeyCode::KeyNone,        // 0x1F
    HID::KeyCode::KeyNone,        // 0x20
    HID::KeyCode::C,              // 0x21
    HID::KeyCode::X,              // 0x22
    HID::KeyCode::D,              // 0x23
    HID::KeyCode::E,              // 0x24
    HID::KeyCode::Num4,           // 0x25
    HID::KeyCode::Num3,           // 0x26
    HID::KeyCode::KeyNone,        // 0x27
    HID::KeyCode::KeyNone,        // 0x28
    HID::KeyCode::SpaceBar,       // 0x29
    HID::KeyCode::V,              // 0x2A
    HID::KeyCode::F,              // 0x2B
    HID::KeyCode::T,              // 0x2C
    HID::KeyCode::R,              // 0x2D
    HID::KeyCode::Num5,           // 0x2E
    HID::KeyCode::KeyNone,        // 0x2F
    HID::KeyCode::KeyNone,        // 0x30
    HID::KeyCode::N,              // 0x31
    HID::KeyCode::B,              // 0x32
    HID::KeyCode::H,              // 0x33
    HID::KeyCode::G,              // 0x34
    HID::KeyCode::Y,              // 0x35
    HID::KeyCode::Num6,           // 0x36
    HID::KeyCode::KeyNone,        // 0x37
    HID::KeyCode::KeyNone,        // 0x38
    HID::KeyCode::KeyNone,        // 0x39
    HID::KeyCode::M,              // 0x3A
    HID::KeyCode::J,              // 0x3B
    HID::KeyCode::U,              // 0x3C
    HID::KeyCode::Num7,           // 0x3D
    HID::KeyCode::Num8,           // 0x3E
    HID::KeyCode::KeyNone,        // 0x3F
    HID::KeyCode::KeyNone,        // 0x40
    HID::KeyCode::Comma,          // 0x41
    HID::KeyCode::K,              // 0x42
    HID::KeyCode::I,              // 0x43
    HID::KeyCode::O,              // 0x44
    HID::KeyCode::Num0,           // 0x45
    HID::KeyCode::Num9,           // 0x46
    HID::KeyCode::KeyNone,        // 0x47
    HID::KeyCode::KeyNone,        // 0x48
    HID::KeyCode::Dot,            // 0x49
    HID::KeyCode::Slash,          // 0x4A
    HID::KeyCode::L,              // 0x4B
    HID::KeyCode::Semicolon,      // 0x4C
    HID::KeyCode::P,              // 0x4D
    HID::KeyCode::Underscore,     // 0x4E
    HID::KeyCode::KeyNone,        // 0x4F
    HID::KeyCode::KeyNone,        // 0x50
    HID::KeyCode::KeyNone,        // 0x51
    HID::KeyCode::Apostrophe,     // 0x52
    HID::KeyCode::KeyNone,        // 0x53
    HID::KeyCode::OpenBrace,      // 0x54
    HID::KeyCode::Equal,          // 0x55
    HID::KeyCode::KeyNone,        // 0x56
    HID::KeyCode::KeyNone,        // 0x57
    HID::KeyCode::CapsLock,       // 0x58
    HID::KeyCode::RightShift,     // 0x59
    HID::KeyCode::Return,         // 0x5A
    HID::KeyCode::CloseBrace,     // 0x5B
    HID::KeyCode::KeyNone,        // 0x5C
    HID::KeyCode::Backslash,      // 0x5D
    HID::KeyCode::KeyNone,        // 0x5E
    HID::KeyCode::KeyNone,        // 0x5F
    HID::KeyCode::KeyNone,        // 0x60
    HID::KeyCode::At102,          // 0x61
    HID::KeyCode::KeyNone,        // 0x62
    HID::KeyCode::KeyNone,        // 0x63
    HID::KeyCode::KeyNone,        // 0x64
    HID::KeyCode::KeyNone,        // 0x65
    HID::KeyCode::Backspace,      // 0x66
    HID::KeyCode::KeyNone,        // 0x67
    HID::KeyCode::KeyNone,        // 0x68
    HID::KeyCode::Keypad1,        // 0x69
    HID::KeyCode::KeyNone,        // 0x6A
    HID::KeyCode::Keypad4,        // 0x6B
    HID::KeyCode::Keypad7,        // 0x6C
    HID::KeyCode::KeyNone,        // 0x6D
    HID::KeyCode::KeyNone,        // 0x6E
    HID::KeyCode::KeyNone,        // 0x6F
    HID::KeyCode::Keypad0,        // 0x70
    HID::KeyCode::KeypadDot,      // 0x71
    HID::KeyCode::Keypad2,        // 0x72
    HID::KeyCode::Keypad5,        // 0x73
    HID::KeyCode::Keypad6,        // 0x74
    HID::KeyCode::Keypad8,        // 0x75
    HID::KeyCode::Escape,         // 0x76
    HID::KeyCode::KeypadNumLock,  // 0x77
    HID::KeyCode::F11,            // 0x78
    HID::KeyCode::KeypadPlus,     // 0x79
    HID::KeyCode::Keypad3,        // 0x7A
    HID::KeyCode::KeypadMinus,    // 0x7B
    HID::KeyCode::KeypadAsterisk, // 0x7C
    HID::KeyCode::Keypad9,        // 0x7D
    HID::KeyCode::ScrollLock,     // 0x7E
    HID::KeyCode::KeyNone,        // 0x7F
    HID::KeyCode::KeyNone,        // 0x80
    HID::KeyCode::KeyNone,        // 0x81
    HID::KeyCode::KeyNone,        // 0x82
    HID::KeyCode::F7,             // 0x83
};

// Set 2 extended (0xE0 prefixed) scancode lookup table
static const HID::KeyCode Set2ExtendedKeyCodes[] = {
    HID::KeyCode::KeyNone,        // 0x00
    HID::KeyCode::KeyNone,        // 0x01
    HID::KeyCode::KeyNone,        // 0x02
    HID::KeyCode::KeyNone,        // 0x03
    HID::KeyCode::KeyNone,        // 0x04
    HID::KeyCode::KeyNone,        // 0x05
    HID::KeyCode::KeyNone,        // 0x06
    HID::KeyCode::KeyNone,        // 0x07
    HID::KeyCode::KeyNone,        // 0x08
    HID::KeyCode::KeyNone,        // 0x09
    HID::KeyCode::KeyNone,        // 0x0A
    HID::KeyCode::KeyNone,        // 0x0B
    HID::KeyCode::KeyNone,        // 0x0C
    HID::KeyCode::KeyNone,        // 0x0D
    HID::KeyCode::KeyNone,        // 0x0E
    HID::KeyCode::KeyNone,        // 0x0F
    HID::KeyCode::KeyNone,        // 0x10
    HID::KeyCode::RightAlt,       // 0x11
    HID::KeyCode::KeyNone,        // 0x12
    HID::KeyCode::KeyNone,        // 0x13
    HID::KeyCode::RightControl,   // 0x14
    HID::KeyCode::KeyNone,        // 0x15
    HID::KeyCode::KeyNone,        // 0x16
    HID::KeyCode::KeyNone,        // 0x17
    HID::KeyCode::KeyNone,        // 0x18
    HID::KeyCode::KeyNone,        // 0x19
    HID::KeyCode::KeyNone,        // 0x1A
    HID::KeyCode::KeyNone,        // 0x1B
    HID::KeyCode::KeyNone,        // 0x1C
    HID::KeyCode::KeyNone,        // 0x1D
    HID::KeyCode::KeyNone,        // 0x1E
    HID::KeyCode::LeftGui,        // 0x1F
    HID::KeyCode::KeyNone,        // 0x20
    HID::KeyCode::KeyNone,        // 0x21
    HID::KeyCode::KeyNone,        // 0x22
    HID::KeyCode::KeyNone,        // 0x23
    HID::KeyCode::KeyNone,        // 0x24
    HID::KeyCode::KeyNone,        // 0x25
    HID::KeyCode::KeyNone,        // 0x26
    HID::KeyCode::RightGui,       // 0x27
    HID::KeyCode::KeyNone,        // 0x28
    HID::KeyCode::KeyNone,        // 0x29
    HID::KeyCode::KeyNone,        // 0x2A
    HID::KeyCode::KeyNone,        // 0x2B
    HID::KeyCode::KeyNone,        // 0x2C
    HID::KeyCode::KeyNone,        // 0x2D
    HID::KeyCode::KeyNone,        // 0x2E
    HID::KeyCode::Application,    // 0x2F
    HID::KeyCode::KeyNone,        // 0x30
    HID::KeyCode::KeyNone,        // 0x31
    HID::KeyCode::KeyNone,        // 0x32
    HID::KeyCode::KeyNone,        // 0x33
    HID::KeyCode::KeyNone,        // 0x34
    HID::KeyCode::KeyNone,        // 0x35
    HID::KeyCode::KeyNone,        // 0x36
    HID::KeyCode::KeyNone,        // 0x37
    HID::KeyCode::KeyNone,        // 0x38
    HID::KeyCode::KeyNone,        // 0x39
    HID::KeyCode::KeyNone,        // 0x3A
    HID::KeyCode::KeyNone,        // 0x3B
    HID::KeyCode::KeyNone,        // 0x3C
    HID::KeyCode::KeyNone,        // 0x3D
    HID::KeyCode::KeyNone,        // 0x3E
    HID::KeyCode::KeyNone,        // 0x3F
    HID::KeyCode::KeyNone,        // 0x40
    HID::KeyCode::KeyNone,        // 0x41
    HID::KeyCode::KeyNone,        // 0x42
    HID::KeyCode::KeyNone,        // 0x43
    HID::KeyCode::KeyNone,        // 0x44
    HID::KeyCode::KeyNone,        // 0x45
    HID::KeyCode::KeyNone,        // 0x46
    HID::KeyCode::KeyNone,        // 0x47
    HID::KeyCode::KeyNone,        // 0x48
    HID::KeyCode::KeyNone,        // 0x49
    HID::KeyCode::KeypadSlash,    // 0x4A
    HID::KeyCode::KeyNone,        // 0x4B
    HID::KeyCode::KeyNone,        // 0x4C
    HID::KeyCode::KeyNone,        // 0x4D
    HID::KeyCode::KeyNone,        // 0x4E
    HID::KeyCode::KeyNone,        // 0x4F
    HID::KeyCode::KeyNone,        // 0x50
    HID::KeyCode::KeyNone,        // 0x51
    HID::KeyCode::KeyNone,        // 0x52
    HID::KeyCode::KeyNone,        // 0x53
    HID::KeyCode::KeyNone,        // 0x54
    HID::KeyCode::KeyNone,        // 0x55
    HID::KeyCode::KeyNone,        // 0x56
    HID::KeyCode::KeyNone,        // 0x57
    HID::KeyCode::KeyNone,        // 0x58
    HID::KeyCode::KeyNone,        // 0x59
    HID::KeyCode::KeypadEnter,    // 0x5A
    HID::KeyCode::KeyNone,        // 0x5B
    HID::KeyCode::KeyNone,        // 0x5C
    HID::KeyCode::KeyNone,        // 0x5D
    HID::KeyCode::KeyNone,        // 0x5E
    HID::KeyCode::KeyNone,        // 0x5F
    HID::KeyCode::KeyNone,        // 0x60
    HID::KeyCode::KeyNone,        // 0x61
    HID::KeyCode::KeyNone,        // 0x62
    HID::KeyCode::KeyNone,        // 0x63
    HID::KeyCode::KeyNone,        // 0x64
    HID::KeyCode::KeyNone,        // 0x65
    HID::KeyCode::KeyNone,        // 0x66
    HID::KeyCode::KeyNone,        // 0x67
    HID::KeyCode::KeyNone,        // 0x68
    HID::KeyCode::End,            // 0x69
    HID::KeyCode::KeyNone,        // 0x6A
    HID::KeyCode::Left,           // 0x6B
    HID::KeyCode::Home,           // 0x6C
    HID::KeyCode::KeyNone,        // 0x6D
    HID::KeyCode::KeyNone,        // 0x6E
    HID::KeyCode::KeyNone,        // 0x6F
    HID::KeyCode::Insert,         // 0x70
    HID::KeyCode::Delete,         // 0x71
    HID::KeyCode::Down,           // 0x72
    HID::KeyCode::KeyNone,        // 0x73
    HID::KeyCode::Right,          // 0x74
    HID::KeyCode::Up,             // 0x75
    HID::KeyCode::KeyNone,        // 0x76
    HID::KeyCode::KeyNone,        // 0x77
    HID::KeyCode::KeyNone,        // 0x78
    HID::KeyCode::KeyNone,        // 0x79
    HID::KeyCode::Pagedown,       // 0x7A
    HID::KeyCode::KeyNone,        // 0x7B
    HID::KeyCode::KeyNone,        // 0x7C
    HID::KeyCode::Pageup,         // 0x7D
};

static const char Set2ASCII[] = {
    '?', '?', '?', '?', '?', '?', '?', '?', // 0x00-0x07
    '?', '?', '?', '?', '?', '\t','`', '?', // 0x08-0x0F
    '?', '?', '?', '?', '?', 'q', '1', '?', // 0x10-0x17
    '?', '?', 'z', 's', 'a', 'w', '2', '?', // 0x18-0x1F
    '?', 'c', 'x', 'd', 'e', '4', '3', '?', // 0x20-0x27
    '?', ' ', 'v', 'f', 't', 'r', '5', '?', // 0x28-0x2F
    '?', 'n', 'b', 'h', 'g', 'y', '6', '?', // 0x30-0x37
    '?', '?', 'm', 'j', 'u', '7', '8', '?', // 0x38-0x3F
    '?', ',', 'k', 'i', 'o', '0', '9', '?', // 0x40-0x47
    '?', '.', '/', 'l', ';', 'p', '-', '?', // 0x48-0x4F
    '?', '?','\'', '?', '[', '=', '?', '?', // 0x50-0x57
    '?', '?','\n', ']', '?','\\', '?', '?', // 0x58-0x5F
    '?', '?', '?', '?', '?', '?', '\b','?', // 0x60-0x67
    '?', '1', '?', '4', '7', '?', '?', '?', // 0x68-0x6F
    '0', '.', '2', '5', '6', '8', '?', '?', // 0x70-0x77
    '?', '+', '3', '-', '*', '9', '?', '?', // 0x78-0x7F
    '?', '?', '?', '?',                      // 0x80-0x83
};

#endif // BOREALOS_PS2_DEFINITIONS_H