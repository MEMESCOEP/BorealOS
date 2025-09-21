#ifndef BOREALOS_FRAMEBUFFERCONSOLE_H
#define BOREALOS_FRAMEBUFFERCONSOLE_H

#include <Definitions.h>

#define FRAMEBUFFER_CONSOLE_TAB_SIZE 4

typedef struct FramebufferConsoleState {
    size_t Width; // Width of the console in characters
    size_t Height; // Height of the console in characters
    size_t CursorX; // Current cursor X position (in characters)
    size_t CursorY; // Current cursor Y position (in characters)
    uint32_t ForegroundColor; // Text color
    uint32_t BackgroundColor; // Background color
} FramebufferConsoleState;

extern FramebufferConsoleState FramebufferConsole;

Status FramebufferConsoleInit(size_t width, size_t height, uint32_t foregroundColor, uint32_t backgroundColor);
void FramebufferConsolePutChar(char c);
void FramebufferConsoleWriteString(const char* str);
void FramebufferConsoleClear();

#endif //BOREALOS_FRAMEBUFFERCONSOLE_H