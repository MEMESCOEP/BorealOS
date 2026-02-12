#include "FBConsole.h"

namespace FBConsole {
    void Console::Initialize() {
        // Make sure limine provided a framebuffer for us to use
        if (!framebuffer_request.response || framebuffer_request.response->framebuffer_count <= 0) PANIC("Limine did not provide a framebuffer!");
        LOG_INFO("Limine provided %u64 framebuffer(s).", framebuffer_request.response->framebuffer_count);

        // Get the framebuffer, we'll only use one for now
        framebuffer = framebuffer_request.response->framebuffers[0];

        // Initialize flanterm
        ftContext = flanterm_fb_init(
            nullptr,                               // malloc
            nullptr,                               // free
            reinterpret_cast<uint32_t*>(framebuffer->address),
            framebuffer->width,
            framebuffer->height,
            framebuffer->pitch,
            framebuffer->red_mask_size,
            framebuffer->red_mask_shift,
            framebuffer->green_mask_size,
            framebuffer->green_mask_shift,
            framebuffer->blue_mask_size,
            framebuffer->blue_mask_shift,
            nullptr,                               // canvas
            nullptr,                               // ansi_colours
            nullptr,                               // ansi_bright_colours
            nullptr,                               // default_bg
            nullptr,                               // default_fg
            nullptr,                               // default_bg_bright
            nullptr,                               // default_fg_bright
            nullptr,                               // font
            0,                                     // font_width
            0,                                     // font_height
            1,                                     // font_spacing
            1,                                     // font_scale_x
            1,                                     // font_scale_y
            0,                                     // margin
            0                                      // rotation
        );

        initialized = true;
        LOG_INFO("Framebuffer is %u64x%u64 @ %u16bpp (pitch is %u64, address is %p).", framebuffer->width, framebuffer->height, framebuffer->bpp, framebuffer->pitch, framebuffer->address);
    }

    void Console::PrintString(const char* str) {
        if (!initialized) return;
        
        size_t len = 0;
        while (str[len] != '\0') {
            ++len;
        }

        flanterm_write(ftContext, str, len);
    }
}