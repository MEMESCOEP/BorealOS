#include "Framebuffer.h"

#include <Boot/MBParser.h>
#include <Core/Memory/Memory.h>
#include <Drivers/CPU.h>
#include <Utility/Drawing.h>
#include <Utility/Color.h>

#include "Core/Kernel.h"

FramebufferConfig KernelFramebuffer = {};

Status FramebufferInit(uint32_t InfoPtr)
{
    LOG(LOG_INFO, "Initializing framebuffer...\n");

    // Attempt to get the framebuffer information using multiboot2; if the request failed, return a failure status and don't try to init further
    struct multiboot_tag_framebuffer_common* FBInfo = MBGetTag(InfoPtr, MULTIBOOT_TAG_TYPE_FRAMEBUFFER);

    if (!FBInfo)
    {
        LOG(LOG_ERROR, "Failed to get framebuffer information from multiboot2!\n");
        return STATUS_FAILURE;
    }

    // Set up the framebuffer state struct using the values we got from the multiboot2 bootloader
    KernelFramebuffer.BitDepth = FBInfo->framebuffer_bpp;
    KernelFramebuffer.Address = (void*)((uint32_t)FBInfo->framebuffer_addr);
    KernelFramebuffer.Height = FBInfo->framebuffer_height;
    KernelFramebuffer.Width = FBInfo->framebuffer_width;
    KernelFramebuffer.Pitch = FBInfo->framebuffer_pitch;
    KernelFramebuffer.CanUse = true;

    // Print the framebuffer properties
    LOGF(LOG_INFO, "Framebuffer at address %p is set up for %ux%u@%u\n",
        KernelFramebuffer.Address,
        KernelFramebuffer.Width,
        KernelFramebuffer.Height,
        KernelFramebuffer.BitDepth
    );

    // Clear the framebuffer to remove any previous data that may have been there, it's always a good idea to start with a known state
    ClearScreen(OURBLE);

    // Let whoever called this function know that the init succeeded
    return STATUS_SUCCESS;
}

void FramebufferMapSelf(PagingState *paging) {
    size_t fbSize = KernelFramebuffer.Height * KernelFramebuffer.Pitch;
    size_t pages = (fbSize / PMM_PAGE_SIZE) + 1;

    uint32_t fbCacheFlags = 0;
    if (CPUHasPAT()) {
        fbCacheFlags = PAGE_WRITE_THROUGH;
    }

    for (size_t i = 0; i < pages; i++) {
        void *vAddr = (void *)((size_t)KernelFramebuffer.Address + (i * PMM_PAGE_SIZE));
        void *pAddr = (void *)ALIGN_DOWN((size_t)KernelFramebuffer.Address + (i * PMM_PAGE_SIZE), PMM_PAGE_SIZE);
        PagingMapPage(paging, vAddr, pAddr, true, false, fbCacheFlags);
    }
}

void FramebufferShift(size_t lines, uint32_t fillColor) {
    if (!KernelFramebuffer.CanUse || lines == 0)
        return;

    if (lines >= KernelFramebuffer.Height)
        lines = KernelFramebuffer.Height;

    size_t rowBytes = KernelFramebuffer.Pitch;
    size_t copyBytes = (KernelFramebuffer.Height - lines) * rowBytes;

    uint8_t* fb = (uint8_t*) KernelFramebuffer.Address;

    // Shift framebuffer up
    memmove(fb, fb + lines * rowBytes, copyBytes);

    // Fill bottom region
    for (size_t y = KernelFramebuffer.Height - lines; y < KernelFramebuffer.Height; y++) {
        uint32_t* row = (uint32_t*)(fb + y * rowBytes);
        for (size_t x = 0; x < KernelFramebuffer.Width; x++) {
            row[x] = fillColor;
        }
    }
}