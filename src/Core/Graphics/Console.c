/* LIBRARIES */
#include <Utilities/StrUtils.h>
#include <Drivers/IO/Serial.h>
#include <Core/Graphics/Console.h>


/* VARIABLES */
static GfxInfo gfxInfo;
static uint16_t *consoleBuffer = NULL;
static uint8_t consoleWidth = 0;
static uint8_t consoleHeight = 0;
static uint8_t curX = 0;
static uint8_t curY = 0;
static uint8_t color = 0x00;

static uint32_t colorCodes[16] =
{
    0x000000, // BLACK
    0x0000AA, // BLUE
    0x00AA00, // GREEN
    0x00AAAA, // CYAN
    0xAA0000, // RED
    0xAA00AA, // MAGENTA
    0xAA5500, // YELLOW
    0xAAAAAA, // LIGHT_GRAY
    0x555555, // DARK_GRAY
    0x5555FF, // LIGHT_BLUE
    0x55FF55, // LIGHT_GREEN
    0x55FFFF, // LIGHT_CYAN
    0xFF5555, // LIGHT_RED
    0xFF55FF, // LIGHT_MAGENTA
    0xFFFF55, // LIGHT_YELLOW
    0xFFFFFF  // WHITE
};


/* FUNCTIONS */
static inline uint8_t _MakeColor(uint8_t fg, uint8_t bg);
static inline uint16_t _MakeEntry(unsigned char c, uint8_t color);

void ConsoleInit(uint8_t width, uint8_t height)
{
    consoleWidth = width;
    consoleHeight = height;
    curX = 0;
    curY = 0;

    GfxGetInfo(&gfxInfo);
    ConsoleClear();
}

void ConsoleClear(void)
{
    MemSet(consoleBuffer, 0, consoleWidth * consoleHeight * sizeof(uint16_t));
    ConsoleSetColor(LIGHT_GRAY, BLACK);
    GfxClearScreen();
}

void ConsoleSetColor(uint8_t fg, uint8_t bg)
{
    _MakeColor(fg, bg);
    color &= 0x7F;
}

/// Sets the terminal colors without disabling the blinking attribute.
void ConsoleSetColorEx(uint8_t fg, uint8_t bg)
{
    _MakeColor(fg, bg);
}

void ConsoleResetColor(void)
{
    color = _MakeColor(LIGHT_GRAY, BLACK);
}

void ConsolePutChar(unsigned char c)
{
    GfxDrawChar(c, curX * FONT_WIDTH, curY * FONT_HEIGHT, colorCodes[color & 0x0F], colorCodes[color >> 4]);
    consoleBuffer[curY * consoleWidth + curX] = _MakeEntry(c, color);
    curX++;
    if (curX >= consoleWidth)
    {
        curX = 0;
        curY++;
    }
}

void ConsolePutString(const unsigned char *str)
{
    while (*str)
    {
        if (*str == '\n')
        {
            curY++;
        }
        else if (*str == '\r')
        {
            curX = 0;
        }
        else if (*str == '\t')
        {
            curX += 4;
        }
        else
        {
            ConsolePutChar(*str);
        }

        str++;

        if (curY >= consoleHeight)
        {
            ConsoleScroll();
        }
    }
}

void ConsoleScroll(void)
{
    if (consoleBuffer == NULL)
    {
        ConsoleSetCursor(0, 0);
        ConsoleClear();
        return;
    }

    for (uint8_t y = 1; y < consoleHeight; y++)
    {
        for (uint8_t x = 0; x < consoleWidth; x++)
        {
            consoleBuffer[(y - 1) * consoleWidth + x] = consoleBuffer[y * consoleWidth + x];
        }
    }

    for (uint8_t x = 0; x < consoleWidth; x++)
    {
        consoleBuffer[(consoleHeight - 1) * consoleWidth + x] = _MakeEntry(' ', color);
    }

    curY--;
    ConsoleRedraw();
}

void ConsoleRedraw(void)
{
    for (uint8_t y = 0; y < consoleHeight; y++)
    {
        for (uint8_t x = 0; x < consoleWidth; x++)
        {
            uint16_t entry = consoleBuffer[y * consoleWidth + x];
            GfxDrawChar(entry & 0xFF, x * FONT_WIDTH, y * FONT_HEIGHT, colorCodes[entry >> 8 & 0x0F], colorCodes[entry >> 12]);
        }
    }
}

void LogWithStackTrace(char *str, enum LogLevel level, int LineNumber, char* Filename)
{
    ConsoleResetColor();

    if (str[0] == '\t') //level == NONE && 
    {
        ConsolePutString("\t");
        str++;
    }

    ConsolePutChar('[');

    // Write the filename and line number from the stack trace
    ConsoleSetColor(WHITE, BLACK);
    ConsolePutString(Filename + LastIndexOfChar(Filename, '/') + 1);

    ConsoleResetColor();
    ConsolePutChar(':');
    
    ConsoleSetColor(CYAN, BLACK);
    PrintNum(LineNumber, 10);
    
    ConsoleResetColor();
    
    if (level != NONE)
        ConsolePutString(" - ");
    
    // Send the same data over serial
    SendCharSerial(ActiveSerialPort, '[');
    SendStringSerial(ActiveSerialPort, Filename);
    SendCharSerial(ActiveSerialPort, ':');


    switch (level)
    {
        case INFO:
            ConsoleSetColor(GREEN, BLACK);
            ConsolePutString("INFO");
            ConsoleResetColor();
            ConsolePutString("] >> ");
            SendStringSerial(ActiveSerialPort, "INFO] >> ");
            break;

        case WARN:
            ConsoleSetColor(YELLOW, BLACK);
            ConsolePutString("WARNING");
            ConsoleResetColor();
            ConsolePutString("] >> ");
            SendStringSerial(ActiveSerialPort, "WARN] >> ");
            break;

        case ERROR:
            ConsoleSetColor(RED, BLACK);
            ConsolePutString("ERROR");
            ConsoleResetColor();
            ConsolePutString("] >> ");
            SendStringSerial(ActiveSerialPort, "ERROR] >> ");
            break;

        case NONE:
            ConsolePutString("] >> ");
            break;

        default:
            LOG_KERNEL_MSG("Invalid log level specified!\n\r", ERROR);
            break;
    }

    ConsolePutString(str);
    SendStringSerial(ActiveSerialPort, str);
}

void ConsoleSetCursor(uint8_t x, uint8_t y)
{
    if (x < consoleWidth && y < consoleHeight)
    {
        curX = x;
        curY = y;
    }
}

void ConsoleChangeCursorPos(uint8_t x, uint8_t y)
{
    if (x < consoleWidth && y < consoleHeight)
    {
        curX = x;
        curY = y;
    }
}

void ConsoleGetCursorPos(uint8_t *x, uint8_t *y)
{
    if (x != NULL) *x = curX;
    if (y != NULL) *y = curY;
}

static inline uint8_t _MakeColor(uint8_t fg, uint8_t bg)
{
    color = fg | (bg << 4);
}

static inline uint16_t _MakeEntry(unsigned char c, uint8_t color)
{
    return (uint16_t)c | ((uint16_t)color << 8);
}