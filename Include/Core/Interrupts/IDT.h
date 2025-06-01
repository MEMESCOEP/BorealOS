#pragma once

#include <Core/Graphics/Console.h>
#include <Core/IO/RegisterIO.h>
#include <Core/IO/PIC.h>
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

typedef struct
{
    uint16_t isrLow;
    uint16_t kernelCode;
    uint8_t reserved;
    uint8_t flags;
    uint16_t isrHigh;
} __attribute__((packed)) IDTEntry;

typedef struct
{
    uint16_t limit;
    uint32_t base;
} __attribute__((packed)) IDTDescriptor;

void IDTInit(void);
void IDTSetIRQHandler(uint8_t irq, void (*handler)(void));
void TestIDT(uint8_t ExceptionSelector);