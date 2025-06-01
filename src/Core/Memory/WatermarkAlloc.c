#include <Core/Memory/WatermarkAlloc.h>

static void *startAddr = NULL;
static void *lastAllocAddr = NULL;
static size_t lastAllocSize = 0;
static size_t totalSize = 0;
static size_t usedSize = 0;

void InitWMAlloc(void *addr, size_t size)
{
    startAddr = addr;
    totalSize = size;
    usedSize = 0;

    LOG_KERNEL_MSG("Watermark Allocator Initialized.", INFO);
}

void *WMAlloc(size_t size)
{
    if (startAddr == NULL || totalSize == 0)
    {
        LOG_KERNEL_MSG("Watermark Allocator not initialized!", ERROR);
        return NULL;
    }

    if (usedSize + size > totalSize)
    {
        LOG_KERNEL_MSG("Watermark Allocator out of memory!", ERROR);
        return NULL;
    }

    void *ptr = (void *)((uintptr_t)startAddr + usedSize);
    usedSize += size;
    lastAllocAddr = ptr;
    lastAllocSize = size;
    LOG_KERNEL_MSG("Allocated memory at address", INFO);

    return ptr;
}

void WMFree(void *ptr)
{
    if (ptr != lastAllocAddr)
    {
        LOG_KERNEL_MSG("Watermark Allocator can only free the last allocated block!", ERROR);
        return;
    }

    if (ptr == NULL)
    {
        LOG_KERNEL_MSG("Cannot free NULL!", ERROR);
        return;
    }

    usedSize -= lastAllocSize;
    lastAllocAddr = NULL;
    lastAllocSize = 0;

    LOG_KERNEL_MSG("Freed memory at address", INFO);
}