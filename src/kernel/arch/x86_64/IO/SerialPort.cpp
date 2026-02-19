#include "SerialPort.h"

#include "Serial.h"

IO::SerialPort::SerialPort(uint16_t port) {
    _port = port;
    _initialized = false;
}


void IO::SerialPort::Initialize() {
    // https://wiki.osdev.org/Serial_Ports#Initialization
    // This code skips the loopback test because we have "modern" ICs and rarely have to worry about miswired or faulty UART systems. We still need to check if a port exists after initialization though.
    IO::Serial::outb(_port + 1, 0x00);
    IO::Serial::outb(_port + 3, 0x80);
    IO::Serial::outb(_port + 0, 0x03);
    IO::Serial::outb(_port + 1, 0x00);
    IO::Serial::outb(_port + 3, 0x03);
    IO::Serial::outb(_port + 2, 0xC7);
    IO::Serial::outb(_port + 4, 0x0B);
    IO::Serial::outb(_port + 4, 0x0F);

    // We still check if the port actually exists, which prevents TX/RX loop stalls and slowdowns
    _initialized = IO::Serial::SerialPortExists(_port);
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
