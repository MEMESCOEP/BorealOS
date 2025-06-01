#ifndef MB2PARSER_H
#define MB2PARSER_H

/* LIBRARIES */
#include <stdint.h>


/* DEFINITIONS */
#define MB2_TAG_FRAMEBUFFER 0x08


/* VARIABLES */
typedef struct {
    uint32_t Type;
    uint32_t Size;
} MB2Tag_t;

typedef struct {
    MB2Tag_t Tag;
    uint8_t RSDPAddress[];
} MB2ACPITag_t;


/* FUNCTIONS */
void* FindMB2Tag(uint32_t Type, void* MB2Info);

#endif