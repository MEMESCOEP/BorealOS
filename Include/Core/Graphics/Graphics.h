#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include <Core/Multiboot/multiboot.h>
#include <Core/Memory/Memory.h>
#include <Core/Graphics/DefaultFont.h>

typedef struct
{
    uint32_t width;
    uint32_t height;
    uint32_t bpp;
    uint32_t pitch;
} GfxInfo;

void GfxConfig(uint32_t* address, uint32_t width, uint32_t height, uint32_t bpp, uint32_t pitch);

void GfxClearScreen(void);
void GfxClearScreenColor(uint32_t color);
void GfxDrawPixel(uint32_t x, uint32_t y, uint32_t color);
void GfxDrawChar(unsigned char c, uint32_t x, uint32_t y, uint32_t fg, uint32_t bg);
void GfxDrawString(const unsigned char *str, uint32_t x, uint32_t y, uint32_t fg, uint32_t bg);
void *GfxGetFramebuffer(void);
void GfxGetInfo(GfxInfo *info);
void GfxGetInfo(GfxInfo *info);
void GfxGetDimensions(uint32_t* Width, uint32_t* Height);