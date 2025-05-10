/* LIBRARIES */
#include <stdbool.h>
#include <stdint.h>
#include "Core/Graphics/Terminal.h"
#include "Core/IO/RegisterIO.h"
#include "Kernel.h"
#include "GDT.h"


/* VARIABLES */
struct GDTEntry {
    uint16_t Limit;
    uint16_t BaseLow;
    uint8_t  BaseMid;
    uint8_t  Access;
    uint8_t  Gran;
    uint8_t  BaseHigh;
} __attribute__((packed));

struct GDTr {
    uint16_t Size;
    uint64_t Offset;
} __attribute__((packed));

struct GDTEntry GDTEntries[9];
struct GDTr GDTDesriptor;

bool GDTInitialized = false;


/* FUNCTIONS */
void GDTSetEntry(uint8_t EntryIndex, uint16_t Limit, uint32_t Base, uint8_t Access, uint8_t Gran)
{
    GDTEntries[EntryIndex].Access = Access;
    GDTEntries[EntryIndex].BaseHigh = (Base >> 24) & 0xff;
    GDTEntries[EntryIndex].BaseMid = (Base >> 16) & 0xff;
    GDTEntries[EntryIndex].BaseLow = Base & 0xffff;
    GDTEntries[EntryIndex].Limit = Limit;
    GDTEntries[EntryIndex].Gran = Gran;
}

void InitGDT()
{
    // Check if interrupts are enabled before initializing the GDT; they MUST be disabled or else the computer might do some weird stuff.
    if (InterruptsEnabled() == true)
    {
        TerminalDrawMessage("Interrupts are currently enabled, they will be disabled because the GDT cannot be initialized with them enabled.\n\r", WARNING);
        __asm__ volatile ("cli");
    }

    TerminalDrawMessage("Setting 8 GDT entries...\n\r", INFO);
    GDTSetEntry(0, 0x0000, 0x00000000, 0x00, 0x00);
    GDTSetEntry(1, 0xFFFF, 0x00000000, 0x9A, 0xCF);
    GDTSetEntry(2, 0xFFFF, 0x00000000, 0x93, 0xCF);
    GDTSetEntry(3, 0xFFFF, 0x00000000, 0x9A, 0xCF);
    GDTSetEntry(4, 0xFFFF, 0x00000000, 0x93, 0xCF);
    GDTSetEntry(5, 0xFFFF, 0x00000000, 0x9B, 0xAF);
    GDTSetEntry(6, 0xFFFF, 0x00000000, 0x93, 0xAF);
    GDTSetEntry(7, 0xFFFF, 0x00000000, 0xFB, 0xAF);
    GDTSetEntry(8, 0xFFFF, 0x00000000, 0xF3, 0xAF);

    GDTDesriptor.Size = sizeof(struct GDTEntry) * 9 - 1;
    GDTDesriptor.Offset = (int64_t)&GDTEntries;
    asm volatile ("lgdt %0" :: "m"(GDTDesriptor) : "memory");
    GDTInitialized = true;
}