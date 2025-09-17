#include "Serial.h"
#include "Utility/SerialOperations.h"

Status SerialInit(uint16_t BasePort, SerialPort *port) {
    port->Initialized = false;
    port->BasePort = BasePort;

    // Shamelessly stolen from https://wiki.osdev.org/Serial_Ports
    outb(BasePort + 1, 0x00);    // Disable all interrupts
    outb(BasePort + 3, 0x80);    // Enable DLAB (set baud rate divisor)
    outb(BasePort + 0, 0x03);    // Set divisor to 3 (lo byte) 38400 baud
    outb(BasePort + 1, 0x00);    //                  (hi byte)
    outb(BasePort + 3, 0x03);    // 8 bits, no parity, one stop bit
    outb(BasePort + 2, 0xC7);    // Enable FIFO, clear them, with 14-byte threshold
    outb(BasePort + 4, 0x0B);    // IRQs enabled, RTS/DSR set
    outb(BasePort + 4, 0x1E);    // Set in loopback mode, test the serial chip
    outb(BasePort + 0, 0xAE);    // Test serial chip (send byte 0xAE and check if serial returns same byte)

    if (inb(BasePort + 0) != 0xAE) {
        return STATUS_FAILURE; // Serial port not functioning
    }

    outb(BasePort + 4, 0x0F);
    port->Initialized = true;
    return STATUS_SUCCESS;
}

Status SerialWriteChar(const SerialPort *port, char c) {
    if (!port->Initialized) {
        return STATUS_FAILURE;
    }
    write_serial(port->BasePort, c);
    return STATUS_SUCCESS;
}

Status SerialWriteString(const SerialPort *port, const char *str) {
    if (!port->Initialized) {
        return STATUS_FAILURE;
    }

    while (*str) {
        SerialWriteChar(port, *str++);
    }

    return STATUS_SUCCESS;
}

Status SerialReadChar(const SerialPort *port, char *outChar) {
    if (!port->Initialized) {
        return STATUS_FAILURE;
    }

    *outChar = read_serial(port->BasePort);

    return STATUS_SUCCESS;
}
