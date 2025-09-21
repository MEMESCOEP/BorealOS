#include "Framebuffer.h"

#include <Boot/MBParser.h>
#include <Utility/Drawing.h>
#include <Utility/Color.h>

#include "Core/Kernel.h"

FramebufferConfig KernelFramebuffer = {};

Status FramebufferInit(uint32_t InfoPtr)
{
    PRINT("Initializing framebuffer...\n");

    // Attempt to get the framebuffer information using multiboot2; if the request failed, return a failure status and don't try to init further
    struct multiboot_tag_framebuffer_common* FBInfo = MBGetTag(InfoPtr, MULTIBOOT_TAG_TYPE_FRAMEBUFFER);

    if (!FBInfo)
    {
        PRINT("Failed to get framebuffer information from multiboot2!\n");
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
    PRINTF("Framebuffer at address %p is set up for %ux%u@%u\n",
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

    for (size_t i = 0; i < pages; i++) {
        void *vAddr = (void *)((size_t)KernelFramebuffer.Address + (i * PMM_PAGE_SIZE));
        void *pAddr = (void *)ALIGN_DOWN((size_t)KernelFramebuffer.Address + (i * PMM_PAGE_SIZE), PMM_PAGE_SIZE);
        PagingMapPage(paging, vAddr, pAddr, true, false);
    }
}
