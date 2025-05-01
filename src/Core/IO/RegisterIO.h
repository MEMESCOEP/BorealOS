#ifndef REGISTERIO_H
#define REGISTERIO_H

/* LIBRARIES */
#include <stdbool.h>
#include <stdint.h>


/* FUNCTIONS */
static inline void OutB(uint16_t Port, uint8_t Value)
{
    __asm__ volatile ( "outb %b0, %w1" : : "a"(Value), "Nd"(Port) : "memory");
    /* There's an outb %al, $imm8 encoding, for compile-time constant port numbers that fit in 8b. (N constraint).
     * Wider immediate constants would be truncated at assemble-time (e.g. "i" constraint).
     * The  outb  %al, %dx  encoding is the only option for all other cases.
     * %1 expands to %dx because  port  is a uint16_t.  %w1 could be used if we had the port number a wider C type */
}

static inline uint8_t InB(uint16_t Port)
{
    uint8_t Ret;
    __asm__ volatile ( "inb %w1, %b0"
                   : "=a"(Ret)
                   : "Nd"(Port)
                   : "memory");
    return Ret;
}

static inline bool InterruptsEnabled()
{
    unsigned long Flags;
    asm volatile ( "pushf\n\t"
                   "pop %0"
                   : "=g"(Flags) );
    return Flags & (1 << 9);
}

static inline void IOWait()
{
    OutB(0x80, 0);
}

#endif