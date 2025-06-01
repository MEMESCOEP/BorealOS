#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include <Core/Memory/Memory.h>
#include <Core/Graphics/Console.h>

void InitWMAlloc(void *startAddr, size_t size);
void *WMAlloc(size_t size);
/// @brief Free memory allocated by WMAlloc, only possible if freeing the last allocated block
void WMFree(void *ptr);