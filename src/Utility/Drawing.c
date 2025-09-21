#include "Drawing.h"

#include <Core/Memory/Memory.h>
#include <Drivers/Graphics/Framebuffer.h>

#include "Core/Kernel.h"
#include "Drivers/Graphics/DefaultFont.h"

uint8_t CharMask[8] = {0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01};

void DrawString(char* Text, uint32_t XPos, uint32_t YPos, uint32_t FGColor, uint32_t BGColor)
{
    if (!KernelFramebuffer.CanUse)
        return; // If the framebuffer isn't usable, don't attempt to draw anything

    while (*Text)
    {
        char CurrentChar = *Text++;
        const uint8_t* Glyph = BitFont + (CurrentChar) * FONT_HEIGHT;

        if (CurrentChar == '\n')
        {
            YPos += FONT_HEIGHT;
            continue;
        }

        else if (CurrentChar == '\r')
        {
            XPos = 0;
            continue;
        }

        for (int cy = 0; cy < FONT_HEIGHT; cy++)
        {
            for (int cx = 0; cx < FONT_WIDTH; cx++)
            {
                uint32_t CharDrawX = XPos + cx;
                uint32_t CharDrawY = YPos + cy;

                // Skip drawing pixels that are outside of the screen bounds
                if (CharDrawX >= KernelFramebuffer.Width || CharDrawY >= KernelFramebuffer.Height)
                    continue;

                KernelFramebuffer.Address[CharDrawY * (KernelFramebuffer.Pitch / 4) + CharDrawX] = (Glyph[cy] & CharMask[cx]) ? FGColor : BGColor;
            }
        }

        XPos += FONT_WIDTH;
    }
}

void DrawChar(char Character, uint32_t XPos, uint32_t YPos, uint32_t FGColor, uint32_t BGColor)
{
    if (!KernelFramebuffer.CanUse)
        return; // If the framebuffer isn't usable, don't attempt to draw anything

    const uint8_t* Glyph = BitFont + (Character) * FONT_HEIGHT;

    for (int cy = 0; cy < FONT_HEIGHT; cy++)
    {
        for (int cx = 0; cx < FONT_WIDTH; cx++)
        {
            uint32_t CharDrawX = XPos + cx;
            uint32_t CharDrawY = YPos + cy;

            // Skip drawing pixels that are outside of the screen bounds
            if (CharDrawX >= KernelFramebuffer.Width || CharDrawY >= KernelFramebuffer.Height)
                continue;

            KernelFramebuffer.Address[CharDrawY * (KernelFramebuffer.Pitch / 4) + CharDrawX] = (Glyph[cy] & CharMask[cx]) ? FGColor : BGColor;
        }
    }
}

void ClearScreen(uint32_t Color)
{
    if (!KernelFramebuffer.CanUse)
        return; // If the framebuffer isn't usable, don't attempt to draw anything

    if (KernelFramebuffer.Pitch == KernelFramebuffer.Width * 4)
    {
        memset32_8086(KernelFramebuffer.Address, Color,
            KernelFramebuffer.Width * KernelFramebuffer.Height);
    }
    else
    {
        for (size_t row = 0; row < KernelFramebuffer.Height; row++) {
            uint32_t* rowAddr = (uint32_t*)((uint8_t*)KernelFramebuffer.Address + row * KernelFramebuffer.Pitch);
            memset32_8086(rowAddr, Color, KernelFramebuffer.Width);
        }
    }
}