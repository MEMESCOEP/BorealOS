#include <Drivers/IO/Serial.h>
#include <Core/Graphics/Graphics.h>

static uint32_t *framebuffer = NULL;
static uint32_t fbWidth = 0;
static uint32_t fbHeight = 0;
static uint32_t fbBpp = 0;
static uint32_t fbPitch = 0;

void GfxInit(void *address, uint32_t width, uint32_t height, uint32_t bpp, uint32_t pitch)
{
    SendStringSerial(SERIAL_COM1, "Graphics address: 0x");
    char framebufferAddrStr[25];
    IntToStr(&address, framebufferAddrStr, 16);
    SendStringSerial(SERIAL_COM1, framebufferAddrStr);
    SendStringSerial(SERIAL_COM1, "\n\r");

    framebuffer = (uint32_t *)address;
    fbWidth = width;
    fbHeight = height;
    fbBpp = bpp;
    fbPitch = pitch;

    GfxClearScreen();
}

void GfxClearScreen(void)
{
    MemSet(framebuffer, 0x00, fbWidth * fbHeight * (fbBpp / 8));
}

void GfxClearScreenColor(uint32_t color)
{
    for (uint32_t y = 0; y < fbHeight; y++)
    {
        for (uint32_t x = 0; x < fbWidth; x++)
        {
            GfxDrawPixel(x, y, color);
        }
    }
}

void GfxDrawPixel(uint32_t x, uint32_t y, uint32_t color)
{
    if (x >= fbWidth || y >= fbHeight) return;
    framebuffer[y * (fbPitch / 4) + x] = color;
}

void GfxDrawChar(unsigned char c, uint32_t x, uint32_t y, uint32_t fg, uint32_t bg)
{
    uint32_t mask[8] = {0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01};
    uint8_t *glyph = BitFont + c * FONT_HEIGHT;

    for (int cy = 0; cy < FONT_HEIGHT; cy++)
    {
        for (int cx = 0; cx < FONT_WIDTH; cx++)
        {
            GfxDrawPixel(x + cx, y + cy, (glyph[cy] & mask[cx]) ? fg : bg);
        }
    }
}

void GfxDrawString(const unsigned char *str, uint32_t x, uint32_t y, uint32_t fg, uint32_t bg)
{
    while (*str)
    {
        GfxDrawChar(*str++, x, y, fg, bg);
        x += FONT_WIDTH;
    }
}

void *GfxGetFramebuffer(void)
{
    return framebuffer;
}

void GfxGetInfo(GfxInfo *info)
{
    info->width = fbWidth;
    info->height = fbHeight;
    info->bpp = fbBpp;
    info->pitch = fbPitch;
}

void GfxGetDimensions(uint32_t* Width, uint32_t* Height)
{
    *Width = fbWidth;
    *Height = fbHeight;
}