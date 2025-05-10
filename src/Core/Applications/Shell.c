/* LIBRARIES */
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "Utilities/MathUtils.h"
#include "Utilities/StrUtils.h"
#include "Drivers/HID/Keyboard.h"
#include "Core/Graphics/Graphics.h"
#include "Core/Graphics/Terminal.h"
#include "Kernel.h"
#include "Shell.h"
#include "Core/Devices/CPU.h"


/* VARIABLES */
char Command[MAX_CMD_LEN] = "";
int PromptLength = 23;
int CharIndex = 0;


/* FUNCTIONS */
void ParseCommand(char* Command)
{
    // Check for an empty command first
    if(StrCmp(Command, "") == 0)
    {
        return;
    }
    else if(StrCmp(Command, "clear") == 0)
    {
        ClearTerminal();
    }
    else if(StrCmp(Command, "about") == 0)
    {
        TerminalDrawString("[== ABOUT ==]\n\rBorealOS, by MEMESCOEP & MartinPrograms\n\r");
        TerminalDrawString("Github repo: https://github.com/memescoep/BorealOS\n\n\r");
    }
    else if(StrCmp(Command, "help") == 0)
    {
        TerminalDrawString("No help yet lol\n\n\r");
    }
    else if(StrCmp(Command, "sysinfo") == 0)
    {
        DisplaySystemInfo();
        TerminalDrawString("\n\r");
    }
    else if(StrCmp(Command, "cpuinfo") == 0)
    {
        char CPUVendorBuffer[13] = "";
        char CPUBrandString[48] = "";
        char CPUCountBuffer[8] = "";
    
        IntToStr(ProcessorCount, CPUCountBuffer, 10);
        GetCPUVendorID(CPUVendorBuffer);
        GetCPUBrandStr(CPUBrandString);
        TerminalDrawString("[== CPU INFORMATION (CPUID) ==]\n\r");
        TerminalDrawString("Available processors: ");
        TerminalDrawString(CPUCountBuffer);
        TerminalDrawString("\n\r");

        TerminalDrawString("Vendor ID: \"");
        TerminalDrawString(CPUVendorBuffer);
        TerminalDrawString("\"\n\r");

        TerminalDrawString("Brand string: \"");
        TerminalDrawString(CPUBrandString);
        TerminalDrawString("\"\n\n\r");
    }
    else
    {
        TerminalDrawMessage("Invalid command \"", ERROR);
        TerminalDrawString(Command);
        TerminalDrawString("\"\n\n\r");
    }
}

void DrawPrompt()
{
    int PreviousFGColor = TerminalFGColor;

    TerminalFGColor = 0xFF00FF;
    TerminalDrawString("root");

    TerminalFGColor = 0xFFFFFF;
    TerminalDrawChar('@', true);

    TerminalFGColor = 0x55FFFF;
    TerminalDrawString("BorealOS");

    TerminalFGColor = 0xFF0000;
    TerminalDrawChar('[', true);

    TerminalFGColor = 0xFFFFFF;
    TerminalDrawString("NoFS");

    TerminalFGColor = 0xFF0000;
    TerminalDrawChar(']', true);

    TerminalFGColor = 0x00FF00;
    TerminalDrawString(" >> ");

    TerminalFGColor = PreviousFGColor;
}

void StartShell()
{
    DrawPrompt();
    DrawFilledRect(CursorColumn * FONT_WIDTH, CursorRow * FONT_HEIGHT, FONT_WIDTH, FONT_HEIGHT, TerminalFGColor);

    while (true)
    {
        char TermChar = GetMappedKey(true);

        switch (LastInput)
        {
            // Enter
            case 0x1C:
                Command[CharIndex] = '\0';
                DrawFilledRect(CursorColumn * FONT_WIDTH, CursorRow * FONT_HEIGHT, FONT_WIDTH, FONT_HEIGHT, TerminalBGColor);
                TerminalDrawString("\n\r");
                ParseCommand(Command);
                DrawPrompt();
                Command[0] = '\0';
                CharIndex = 0;
                break;

            // Backspace
            case 0x0E:
                if (CursorColumn > PromptLength)
                {
                    DrawFilledRect(CursorColumn * FONT_WIDTH, CursorRow * FONT_HEIGHT, FONT_WIDTH, FONT_HEIGHT, TerminalBGColor);
                    MoveCursor(CursorColumn - 1, CursorRow);
                    TerminalDrawChar(' ', true);
                    MoveCursor(CursorColumn - 1, CursorRow);
                }

                CharIndex = IntMax(CharIndex - 1, 0);
                Command[CharIndex] = '\0';
                break;

            default:
                if (IsCharacterKey(LastInput))
                {
                    TerminalDrawChar(TermChar, true);
                    Command[CharIndex] = TermChar;
                    CharIndex = IntMin(CharIndex + 1, MAX_CMD_LEN);
                }

                break;
        }

        DrawFilledRect(CursorColumn * FONT_WIDTH, CursorRow * FONT_HEIGHT, FONT_WIDTH, FONT_HEIGHT, TerminalFGColor);
    }
}