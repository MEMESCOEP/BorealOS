#ifndef BOREALOS_FRAMEBUFFERCONSOLE_H
#define BOREALOS_FRAMEBUFFERCONSOLE_H

#include <Definitions.h>

#define FRAMEBUFFER_CONSOLE_TAB_SIZE 4

typedef struct FramebufferConsoleState {
    struct flanterm_context* TermContext; // The flanterm context
    size_t Width; // Width of the console in characters
    size_t Height; // Height of the console in characters
    bool CanUse; // Used to show when the console can be written to
} FramebufferConsoleState;

extern FramebufferConsoleState FramebufferConsole;

Status FramebufferConsoleInit(size_t width, size_t height, uint32_t foregroundColor, uint32_t backgroundColor);
void FramebufferConsoleWriteString(const char* str);
void FramebufferConsoleClear();

#endif //BOREALOS_FRAMEBUFFERCONSOLE_H