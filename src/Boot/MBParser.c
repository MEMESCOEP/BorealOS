#include "MBParser.h"

void * MBGetTag(uint32_t InfoPtr, uint32_t Type) {
    uint8_t* Ptr = (uint8_t*)InfoPtr + 8; // skip total_size + reserved

    while (true)
    {
        MB2Tag_t* Tag = (MB2Tag_t*)Ptr;

        if (Tag->Type == 0) break; // End tag
        if (Tag->Type == Type) return Tag;

        Ptr += (Tag->Size + 7) & ~7; // Align to 8 bytes
    }

    return NULL;
}
