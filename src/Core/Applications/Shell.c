/* LIBRARIES */
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "Utilities/MathUtils.h"
#include "Utilities/StrUtils.h"
#include "Drivers/HID/Keyboard.h"
#include "Core/Graphics/Graphics.h"
#include "Core/Graphics/Terminal.h"
#include "Shell.h"


/* VARIABLES */
char Command[MAX_CMD_LEN] = "";
int PromptLength = 3;
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
    else
    {
        TerminalDrawString("Invalid command \"");
        TerminalDrawString(Command);
        TerminalDrawString("\".\n\n\r");
    }
}

void StartShell()
{
    TerminalDrawString(">> ");
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
                TerminalDrawString(">> ");
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

                DrawFilledRect(CursorColumn * FONT_WIDTH, CursorRow * FONT_HEIGHT, FONT_WIDTH, FONT_HEIGHT, TerminalFGColor);
                break;
        }
    }
}