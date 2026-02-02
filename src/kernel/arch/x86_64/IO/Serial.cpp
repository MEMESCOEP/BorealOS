#include "Serial.h"

void IO::Serial::outb(uint16_t port, uint8_t value) {
    asm volatile ("outb %0, %1" : : "a"(value), "Nd"(port));
}

uint8_t IO::Serial::inb(uint16_t port) {
    uint8_t ret;
    asm volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

uint16_t IO::Serial::inw(uint16_t port) {
    uint16_t ret;
    asm volatile ("inw %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

uint32_t IO::Serial::Available(uint16_t comPort) {
    return inb(comPort + 5) & 0x20;
}

uint32_t IO::Serial::TransmitEmpty(uint16_t comPort) {
    return inb(comPort + 5) & 0x20;
}

void IO::Serial::WriteChar(uint16_t comPort, char c) {
    while (TransmitEmpty(comPort) == 0) {}
    outb(comPort, c);
}

void IO::Serial::WriteString(uint16_t comPort, const char *str) {
    while (*str) {
        WriteChar(comPort, *str++);
    }
}
