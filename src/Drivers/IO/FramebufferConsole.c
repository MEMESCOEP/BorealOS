#include "FramebufferConsole.h"

#include <flanterm.h>
#include <flanterm_backends/fb.h>
#include <Drivers/Graphics/DefaultFont.h>
#include <Drivers/Graphics/Framebuffer.h>
#include <Utility/StringTools.h>
#include <Utility/Drawing.h>
#include <Boot/multiboot.h>
#include <Core/Kernel.h>

#define ANSI_CLEAR_SCREEN "\033[2J\033[H"

FramebufferConsoleState FramebufferConsole = {};

Status FramebufferConsoleInit(size_t width, size_t height, uint32_t foregroundColor, uint32_t backgroundColor) {
    FramebufferConsole.Width = width;
    FramebufferConsole.Height = height;

    // Initialize flanterm
    FramebufferConsole.TermContext = flanterm_fb_init(
        NULL,
        NULL,
        KernelFramebuffer.Address, KernelFramebuffer.Width, KernelFramebuffer.Height, KernelFramebuffer.Pitch,
        KernelFramebuffer.FBInfo->framebuffer_red_mask_size, KernelFramebuffer.FBInfo->framebuffer_red_field_position,
        KernelFramebuffer.FBInfo->framebuffer_green_mask_size, KernelFramebuffer.FBInfo->framebuffer_green_field_position,
        KernelFramebuffer.FBInfo->framebuffer_blue_mask_size, KernelFramebuffer.FBInfo->framebuffer_blue_field_position,
        NULL,
        NULL, NULL,
        &backgroundColor, &foregroundColor,
        NULL, NULL,
        NULL, 0, 0, 1,
        0, 0,
        0
    );

    // Clear the framebuffer and mark it as ready for use
    FramebufferConsoleClear();
    FramebufferConsole.CanUse = true;
    return STATUS_SUCCESS;
}

void FramebufferConsoleWriteString(const char *str) {
    if (FramebufferConsole.CanUse == false || KernelFramebuffer.CanUse == false || FramebufferConsole.TermContext == NULL)
        return;

    flanterm_write(FramebufferConsole.TermContext, str, strlen(str));
}

void FramebufferConsoleClear() {
    flanterm_write(FramebufferConsole.TermContext, ANSI_CLEAR_SCREEN, strlen(ANSI_CLEAR_SCREEN));
}