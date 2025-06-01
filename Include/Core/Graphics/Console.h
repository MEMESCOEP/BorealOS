#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include <Core/Graphics/Graphics.h>
#define LOG_KERNEL_MSG(MESSAGE, LOGLEVEL) LogWithStackTrace(MESSAGE, LOGLEVEL, __LINE__, __FILE__)

enum VgaColor
{
    BLACK = 0,
    BLUE = 1,
    GREEN = 2,
    CYAN = 3,
    RED = 4,
    MAGENTA = 5,
    YELLOW = 6,
    LIGHT_GRAY = 7,
    DARK_GRAY = 8,
    LIGHT_BLUE = 9,
    LIGHT_GREEN = 10,
    LIGHT_CYAN = 11,
    LIGHT_RED = 12,
    LIGHT_MAGENTA = 13,
    LIGHT_YELLOW = 14,
    WHITE = 15
};

enum LogLevel
{
    NONE,
    INFO,
    WARN,
    ERROR
};

void ConsoleInit(uint8_t width, uint8_t height); // WxH in chars
void ConsoleClear(void);
void ConsoleSetColor(uint8_t fg, uint8_t bg);
void ConsoleSetColorEx(uint8_t fg, uint8_t bg);
void ConsoleResetColor(void);
void ConsolePutChar(unsigned char c);
void ConsolePutString(const unsigned char *str);
void ConsoleScroll(void);
void ConsoleRedraw(void);

void ConsoleSetCursor(uint8_t x, uint8_t y);
void ConsoleChangeCursorPos(uint8_t x, uint8_t y);
void ConsoleGetCursorPos(uint8_t *x, uint8_t *y);

/// Built out console functionality
void LogWithStackTrace(char *str, enum LogLevel level, int LineNumber, char* Filename);