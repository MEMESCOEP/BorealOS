#include "FramebufferConsole.h"

#include <Drivers/Graphics/DefaultFont.h>
#include <Utility/Drawing.h>

FramebufferConsoleState FramebufferConsole = {};

Status FramebufferConsoleInit(size_t width, size_t height, uint32_t foregroundColor, uint32_t backgroundColor) {
    FramebufferConsole.Width = width;
    FramebufferConsole.Height = height;
    FramebufferConsole.CursorX = 0;
    FramebufferConsole.CursorY = 0;
    FramebufferConsole.ForegroundColor = foregroundColor;
    FramebufferConsole.BackgroundColor = backgroundColor;

    FramebufferConsoleClear();

    return STATUS_SUCCESS;
}

void FramebufferConsolePutChar(char c) {
    if (c == '\n')
    {
        FramebufferConsole.CursorX = 0;
        FramebufferConsole.CursorY++;

    }
    else if (c == '\r')
    {
        FramebufferConsole.CursorX = 0;
    }
    else if (c == '\t')
    {
        FramebufferConsole.CursorX += FRAMEBUFFER_CONSOLE_TAB_SIZE;
    }
    else
    {
        DrawChar(c, FramebufferConsole.CursorX * FONT_WIDTH, FramebufferConsole.CursorY * FONT_HEIGHT, FramebufferConsole.ForegroundColor, FramebufferConsole.BackgroundColor);
        FramebufferConsole.CursorX++;
    }

    if (FramebufferConsole.CursorX > FramebufferConsole.Width)
    {
        FramebufferConsole.CursorX = 0;
        FramebufferConsole.CursorY++;
    }
}

void FramebufferConsoleWriteString(const char *str) {
    while (*str) {
        FramebufferConsolePutChar(*str++);
    }
}

void FramebufferConsoleClear() {
    ClearScreen(FramebufferConsole.BackgroundColor);
    FramebufferConsole.CursorX = 0;
    FramebufferConsole.CursorY = 0;
}