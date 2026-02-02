#include "SerialPort.h"

#include "Serial.h"

IO::SerialPort::SerialPort(uint16_t port) {
    _port = port;
    _initialized = false;
}

void IO::SerialPort::Initialize() {
    // https://wiki.osdev.org/Serial_Ports#Initialization
    IO::Serial::outb(_port + 1, 0x00);
    IO::Serial::outb(_port + 3, 0x80);
    IO::Serial::outb(_port + 0, 0x03);
    IO::Serial::outb(_port + 1, 0x00);
    IO::Serial::outb(_port + 3, 0x03);
    IO::Serial::outb(_port + 2, 0xC7);
    IO::Serial::outb(_port + 4, 0x0B);
    IO::Serial::outb(_port + 4, 0x1E);
    IO::Serial::outb(_port + 0, 0xAE);
    if (IO::Serial::inb(_port) != 0xAE) {
        _initialized = false;
        return;
    }

    IO::Serial::outb(_port + 4, 0x0F); // Set to normal operation mode
    _initialized = true;
}

void IO::SerialPort::WriteChar(char c) const {
    if (!_initialized) {
        return;
    }

    IO::Serial::WriteChar(_port, c);
}

void IO::SerialPort::WriteString(const char* str) const {
    if (!_initialized) {
        return;
    }

    IO::Serial::WriteString(_port, str);
}

bool IO::SerialPort::IsInitialized() const {
    return _initialized;
}
