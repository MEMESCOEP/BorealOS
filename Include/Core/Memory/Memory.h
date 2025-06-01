#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

void *MemSet(void *ptr, int value, size_t num);
void *MemCpy(void *dest, const void *src, size_t num);
void *MemMove(void *dest, const void *src, size_t num);
bool MemCmp(const void *ptr1, const void *ptr2, size_t num);