#ifndef BOREALOS_SERIAL_H
#define BOREALOS_SERIAL_H

#include <Definitions.h>

namespace IO::Serial {
    constexpr uint16_t COM1 = 0x3F8;
    constexpr uint16_t COM2 = 0x2F8;
    constexpr uint16_t COM3 = 0x3E8;
    constexpr uint16_t COM4 = 0x2E8;
    constexpr uint16_t COM5 = 0x5F8;
    constexpr uint16_t COM6 = 0x4F8;
    constexpr uint16_t COM7 = 0x5E8;
    constexpr uint16_t COM8 = 0x4E8;

    // X86 I/O port operations
    void outb(uint16_t port, uint8_t value);
    uint8_t inb(uint16_t port);
    uint16_t inw(uint16_t port);

    uint32_t Available(uint16_t comPort);
    uint32_t TransmitEmpty(uint16_t comPort);
    bool SerialPortExists(uint16_t COMPort);
    void WriteChar(uint16_t comPort, char c);
    void WriteString(uint16_t comPort, const char* str);
    void IOWait();
}

#endif //BOREALOS_SERIAL_H