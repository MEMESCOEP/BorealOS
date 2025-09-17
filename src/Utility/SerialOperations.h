#ifndef BOREALOS_SERIALOPERATIONS_H
#define BOREALOS_SERIALOPERATIONS_H

#include <Definitions.h>

static INLINE void outb(uint16_t port, uint8_t val) {
    ASM ("outb %0, %1" : : "a"(val), "Nd"(port));
}

static INLINE uint8_t inb(uint16_t port) {
    uint8_t ret;
    ASM ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static INLINE void io_wait(void) {
    // This is a no-op, but it ensures that the I/O operations are completed
    ASM ("outb %%al, $0x80" : : "a"(0));
}

static INLINE bool is_transmit_empty(uint16_t BasePort) {
    return inb(BasePort + 5) & 0x20;
}

static INLINE void write_serial(uint16_t BasePort, char a) {
    while (is_transmit_empty(BasePort) == 0) {}
    outb(BasePort, a);
}

static INLINE int serial_received(uint16_t BasePort) {
    return inb(BasePort + 5) & 1;
}

static INLINE char read_serial(uint16_t BasePort) {
    while (serial_received(BasePort) == 0) {}
    return inb(BasePort);
}

#endif //BOREALOS_SERIALOPERATIONS_H