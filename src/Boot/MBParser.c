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

void * MBGetFirstTag(uint32_t InfoPtr) {
    uint8_t* Ptr = (uint8_t*)InfoPtr + 8; // skip total_size + reserved
    MB2Tag_t* Tag = (MB2Tag_t*)Ptr;
    if (Tag->Type == 0) return NULL; // No tags
    return Tag;
}

void * MBGetNextTag(void *CurrentTag) {
    // Calculate the next tag position
    uint8_t* Ptr = (uint8_t*)CurrentTag;
    MB2Tag_t* Tag = (MB2Tag_t*)Ptr;
    Ptr += (Tag->Size + 7) & ~7; // Align to 8 bytes
    Tag = (MB2Tag_t*)Ptr;
    if (Tag->Type == 0) return NULL; // No more tags
    return Tag;
}
