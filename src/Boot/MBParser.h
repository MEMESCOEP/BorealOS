#ifndef BOREALOS_MBPARSER_H
#define BOREALOS_MBPARSER_H

#include <Definitions.h>
#include "multiboot.h"

typedef struct {
    uint32_t Type;
    uint32_t Size;
} MB2Tag_t;

void* MBGetTag(uint32_t InfoPtr, uint32_t Type);
void* MBGetFirstTag(uint32_t InfoPtr);
void* MBGetNextTag(void* CurrentTag);

#endif //BOREALOS_MBPARSER_H