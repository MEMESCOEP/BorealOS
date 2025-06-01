/* LIBRARIES */
#include <Core/Multiboot/MB2Parser.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>


/* FUNCTIONS */
void* FindMB2Tag(uint32_t Type, void* MB2Info)
{
    uint8_t* Ptr = (uint8_t*)MB2Info + 8; // skip total_size + reserved

    while (true)
    {
        MB2Tag_t* Tag = (MB2Tag_t*)Ptr;

        if (Tag->Type == 0) break; // End tag
        if (Tag->Type == Type) return Tag;
        
        Ptr += (Tag->Size + 7) & ~7; // Align to 8 bytes
    }

    return NULL;
}