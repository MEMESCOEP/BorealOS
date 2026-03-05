#ifndef BOREALOS_MEMORYUTILS_H
#define BOREALOS_MEMORYUTILS_H

#include <Definitions.h>
#include "../Boot/LimineDefinitions.h"

#define HIGHER_HALF(addr) ((uint64_t)addr + (uint64_t)hhdm_request.response->offset)

#endif