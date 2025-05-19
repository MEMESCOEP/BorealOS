#ifndef PHYSICALALLOCATOR_H
#define PHYSICALALLOCATOR_H

#include <stdint.h>

void InitPhysicalMemoryAllocator();
void TestPhysicalMemoryAllocator();

void* AllocatePhysicalMemory(uint64_t Size);
void FreePhysicalMemory(void* Address);

#endif //PHYSICALALLOCATOR_H
