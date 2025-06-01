#pragma once

/* LIBRARIES */
#include <stdbool.h>
#include <stdint.h>


/* FUNCTIONS */
static inline void OutB(uint16_t port, uint8_t value)
{
    asm volatile ("outb %0, %1" : : "a"(value), "Nd"(port));
}

static inline void OutW(uint16_t port, uint16_t value)
{
    asm volatile ("outw %0, %1" : : "a"(value), "Nd"(port));
}

static inline void OutL(uint16_t port, uint32_t value)
{
    asm volatile ("outl %0, %1" : : "a"(value), "Nd"(port));
}

static inline uint8_t InB(uint16_t port)
{
    uint8_t ret;
    asm volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static inline uint16_t InW(uint16_t port)
{
    uint16_t ret;
    asm volatile ("inw %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static inline uint32_t InL(uint16_t port)
{
    uint32_t ret;
    asm volatile ("inl %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static inline void IoDelay(void)
{
    asm volatile ("outb %%al, $0x80" : : "a"(0));
}

static inline bool InterruptsEnabled()
{
    unsigned long Flags;
    asm volatile ( "pushf\n\t"
                   "pop %0"
                   : "=g"(Flags) );
    return Flags & (1 << 9);
}