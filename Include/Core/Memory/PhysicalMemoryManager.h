#ifndef PHYSICALMEMORYMANAGER_H
#define PHYSICALMEMORYMANAGER_H

#include <stdint.h>

typedef struct {
    uint32_t type;
    uint32_t size;
    uint32_t entry_size;
    uint32_t entry_version;
    struct {
        uint64_t addr;
        uint64_t len;
        uint32_t type;
        uint32_t reserved;
    } entries[];
} __attribute__((packed)) MB2MemoryMap_t;

#endif //PHYSICALMEMORYMANAGER_H
