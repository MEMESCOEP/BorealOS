#ifndef BOREALOS_FRAMEBUFFER_H
#define BOREALOS_FRAMEBUFFER_H

#include <Definitions.h>
#include <Core/Memory/Paging.h>

typedef struct FramebufferConfig {
    uint32_t* Address; // Bitmap for tracking allocated pages
    size_t Width; // Width of the FB in pixels
    size_t Height; // Height of the FB in pixels
    size_t Pitch;
    size_t BitDepth; // The number of bits per pixel (often referred to as BPP); more bits means more colors
    bool CanUse; // Whether the framebuffer is usable or not
} FramebufferConfig;

extern FramebufferConfig KernelFramebuffer;

/// Initialize the framebuffer for the system.
/// InfoPtr is a pointer to the multiboot2 information structure.
Status FramebufferInit(uint32_t InfoPtr);

/// Map the framebuffer into the current paging context
/// This is necessary to allow the kernel to access the framebuffer after enabling paging
void FramebufferMapSelf(PagingState* paging);

/// Shift the framebuffer content up by a number of lines, filling the new space with a specified color
void FramebufferShift(size_t lines, uint32_t fillColor);

#endif //BOREALOS_FRAMEBUFFER_H