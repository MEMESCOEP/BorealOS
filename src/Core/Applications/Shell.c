/* LIBRARIES */
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "Utilities/MathUtils.h"
#include "Utilities/StrUtils.h"
#include "Drivers/HID/Keyboard.h"
#include "Core/Graphics/Graphics.h"
#include "Core/Graphics/Terminal.h"
#include "Core/Devices/CPU.h"
#include "Core/Power/ACPI.h"
#include "Kernel.h"
#include "Shell.h"


/* VARIABLES */
bool DrawNewCursor = true;
char CommandHistory[32][MAX_CMD_LEN];
char Command[MAX_CMD_LEN] = "";
int CommandsInHistory = 0;
int CommandLength = 0;
int CMDHistIndex = 0;
int PromptLength = 23;
int CharIndex = 0;


/* FUNCTIONS */
// Converts a cmdline string into a command and a set of arguments (sets FullCommand to the command and populates ArgListOut with the command args)
size_t SplitCommand(char *FullCommand, char *ArgListOut[])
{
    size_t ArgC = 0;

    while (*FullCommand != '\0' && ArgC < MAX_ARGS)
    {
        // Skip leading spaces
        while (*FullCommand == ' ')
        {
            *FullCommand = '\0'; // Null out consecutive spaces
            FullCommand++;
        }

        if (*FullCommand == '\0') break;

        // Start of new token
        ArgListOut[ArgC++] = FullCommand;

        // Find the end of the token
        while (*FullCommand != '\0' && *FullCommand != ' ')
        {
            FullCommand++;
        }
    }

    return ArgC;
}

void ParseCommand(char* FullCommand)
{
    char* Args[MAX_ARGS];
    size_t ArgCount = SplitCommand(FullCommand, Args);

    // Check for an empty command first
    if(StrCmp(FullCommand, "") == 0)
    {
        return;
    }
    else if(StrCmp(FullCommand, "clear") == 0)
    {
        ClearTerminal();
    }
    else if(StrCmp(FullCommand, "about") == 0)
    {
        TerminalDrawString("[== ABOUT ==]\n\rBorealOS, by MEMESCOEP & MartinPrograms\n\r");
        TerminalDrawString("Github repo: https://github.com/memescoep/BorealOS\n\n\r");
    }
    else if(StrCmp(FullCommand, "help") == 0)
    {
        TerminalDrawString("No help yet lol\n\n\r");
    }
    else if(StrCmp(FullCommand, "sysinfo") == 0)
    {
        DisplaySystemInfo();
        TerminalDrawString("\n\r");
    }
    else if(StrCmp(FullCommand, "cpuinfo") == 0)
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
    else if (StrCmp(FullCommand, "echo") == 0)
    {
        for(size_t ArgIndex = 1; ArgIndex < ArgCount; ArgIndex++)
        {
            TerminalDrawString(Args[ArgIndex]);
            TerminalDrawString("\n\r");
        }
    }
    else if (StrCmp(FullCommand, "history") == 0)
    {
        for (int CMDIndex = 0; CMDIndex < CommandsInHistory; CMDIndex++)
        {
            TerminalDrawString(CommandHistory[CMDIndex]);
            TerminalDrawString("\n\r");
        }
    }
    else
    {
        TerminalDrawMessage("Invalid command \"", ERROR);
        TerminalDrawString(FullCommand);
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
    TerminalDrawString(SystemHostname);

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
                Command[CommandLength] = '\0';

                if (CharIndex >= CommandLength)
                {
                    DrawFilledRect(CursorColumn * FONT_WIDTH, CursorRow * FONT_HEIGHT, FONT_WIDTH, FONT_HEIGHT, TerminalBGColor);
                }
                else
                {
                    // Invert the current text block
                    InvertColorOfTextBlock();
                }

                if (StrCmp(Command, "") != 0)
                {
                    if (CommandsInHistory > 0 && StrCmp(Command, CommandHistory[1]) == 0)
                    {

                    }
                    else
                    {
                        // Add the command to the history list and shift others back by one position (sorts by newest -> oldest)
                        for (int CMDIndex = 30; CMDIndex >= 0; CMDIndex--)
                        {
                            for (int CMDCharIndex = 0; CMDCharIndex < MAX_CMD_LEN; CMDCharIndex++)
                            {
                                CommandHistory[CMDIndex + 1][CMDCharIndex] = CommandHistory[CMDIndex][CMDCharIndex];
                        
                                if (CommandHistory[CMDIndex][CMDCharIndex] == '\0')
                                {
                                    break;
                                }
                            }
                        }

                        // Copy the most recent command into the history list at index 0
                        for (int CMDCharIndex = 0; CMDCharIndex < MAX_CMD_LEN - 1; CMDCharIndex++)
                        {
                            CommandHistory[0][CMDCharIndex] = Command[CMDCharIndex];
                        
                            if (Command[CMDCharIndex] == '\0')
                            {
                                break;
                            }
                        }
                        
                        CommandHistory[0][MAX_CMD_LEN - 1] = '\0';
                        CommandsInHistory++;
                    }
                }
                
                TerminalDrawString("\n\r");
                ParseCommand(Command);
                DrawPrompt();
                Command[0] = '\0';
                CommandLength = 0;
                CMDHistIndex = 0;
                CharIndex = 0;
                DrawFilledRect(CursorColumn * FONT_WIDTH, CursorRow * FONT_HEIGHT, FONT_WIDTH, FONT_HEIGHT, TerminalFGColor);
                break;

            // Backspace
            case 0x0E:
                if (CharIndex > 0 || CursorColumn > PromptLength)
                {
                    if (CharIndex >= CommandLength)
                    {
                        DrawFilledRect(CursorColumn * FONT_WIDTH, CursorRow * FONT_HEIGHT, FONT_WIDTH, FONT_HEIGHT, TerminalBGColor);
                        MoveCursor(CursorColumn - 1, CursorRow);
                        TerminalDrawChar(' ', true);
                        MoveCursor(CursorColumn - 1, CursorRow);
                        DrawFilledRect(CursorColumn * FONT_WIDTH, CursorRow * FONT_HEIGHT, FONT_WIDTH, FONT_HEIGHT, TerminalFGColor);
                        Command[CharIndex - 1] = '\0';
                    }
                    else
                    {
                        InvertColorOfTextBlock();
                        MoveCursor(CursorColumn - 1, CursorRow);

                        // We'll have to redraw the command now
                        for (int DrawCharIndex = CharIndex; DrawCharIndex <= CommandLength; DrawCharIndex++)
                        {
                            TerminalDrawChar(Command[DrawCharIndex], true);
                            Command[DrawCharIndex - 1] = Command[DrawCharIndex];
                        }

                        MoveCursor(CursorColumn - (CommandLength - CharIndex) - 1, CursorRow);
                        InvertColorOfTextBlock();
                        Command[CommandLength - 1] = '\0';
                    }                    
                    
                    CommandLength--;
                }

                CharIndex = IntMax(CharIndex - 1, 0);
                break;

            // Delete
            case 0x53:
                if ((CharIndex >= 0 || CursorColumn > PromptLength) && CharIndex < CommandLength)
                {
                    InvertColorOfTextBlock();

                    // We'll have to redraw the command now
                    for (int DrawCharIndex = CharIndex + 1; DrawCharIndex <= CommandLength; DrawCharIndex++)
                    {
                        TerminalDrawChar(Command[DrawCharIndex], true);
                        Command[DrawCharIndex - 1] = Command[DrawCharIndex];
                    }

                    MoveCursor(CursorColumn - (CommandLength - CharIndex), CursorRow);
                    InvertColorOfTextBlock();
                    Command[CommandLength - 1] = '\0';
                    CommandLength--;
                }

                break;

            // Left arrow key
            case 0x4B:
                if (CharIndex > 0)
                {
                    DrawNewCursor = false;

                    // Invert the current text block
                    InvertColorOfTextBlock();
                    MoveCursor(CursorColumn - 1, CursorRow);
                    
                    // Invert the current text block again
                    InvertColorOfTextBlock();
                    CharIndex--;
                }

                break;

            // Right arrow key
            case 0x4D:
                if (CharIndex < CommandLength)
                {
                    DrawNewCursor = false;

                    // Invert the current text block
                    InvertColorOfTextBlock();
                    MoveCursor(CursorColumn + 1, CursorRow);
                    
                    // Invert the current text block again
                    InvertColorOfTextBlock();                    
                    CharIndex++;
                }

                break;

            // Up arrow key
            case 0x48:
                DrawFilledRect(CursorColumn * FONT_WIDTH, CursorRow * FONT_HEIGHT, FONT_WIDTH, FONT_HEIGHT, TerminalBGColor);
                MoveCursor(PromptLength + CommandLength, CursorRow);

                for (int CLRCharIndex = 0; CLRCharIndex < CommandLength; CLRCharIndex++)
                {
                    DrawFilledRect(CursorColumn * FONT_WIDTH, CursorRow * FONT_HEIGHT, FONT_WIDTH, FONT_HEIGHT, TerminalBGColor);
                    MoveCursor(CursorColumn - 1, CursorRow);
                }

                TerminalDrawString(CommandHistory[CMDHistIndex]);
                DrawFilledRect(CursorColumn * FONT_WIDTH, CursorRow * FONT_HEIGHT, FONT_WIDTH, FONT_HEIGHT, TerminalFGColor);
                CommandLength = StrLen(CommandHistory[CMDHistIndex]);
                CharIndex = CommandLength;

                for (int CMDCharIndex = 0; CMDCharIndex < CommandLength; CMDCharIndex++)
                {
                    Command[CMDCharIndex] = CommandHistory[CMDHistIndex][CMDCharIndex];
                }

                CMDHistIndex = IntMin(CMDHistIndex + 1, CommandsInHistory - 1);
                break;

            // Down arrow key
            case 0x50:
                DrawFilledRect(CursorColumn * FONT_WIDTH, CursorRow * FONT_HEIGHT, FONT_WIDTH, FONT_HEIGHT, TerminalBGColor);
                MoveCursor(PromptLength + CommandLength, CursorRow);

                for (int CLRCharIndex = 0; CLRCharIndex < CommandLength; CLRCharIndex++)
                {
                    DrawFilledRect(CursorColumn * FONT_WIDTH, CursorRow * FONT_HEIGHT, FONT_WIDTH, FONT_HEIGHT, TerminalBGColor);
                    MoveCursor(CursorColumn - 1, CursorRow);
                }
                
                if (CMDHistIndex >= 0)
                {
                    TerminalDrawString(CommandHistory[CMDHistIndex]);
                    CommandLength = StrLen(CommandHistory[CMDHistIndex]);
                    CharIndex = CommandLength;

                    for (int CMDCharIndex = 0; CMDCharIndex < CommandLength; CMDCharIndex++)
                    {
                        Command[CMDCharIndex] = CommandHistory[CMDHistIndex][CMDCharIndex];
                    }
                }
                else
                {
                    CommandLength = 0;
                    CharIndex = 0;
                    Command[0] = '\0';
                }
                
                DrawFilledRect(CursorColumn * FONT_WIDTH, CursorRow * FONT_HEIGHT, FONT_WIDTH, FONT_HEIGHT, TerminalFGColor);
                CMDHistIndex = IntMax(CMDHistIndex - 1, -1);
                break;

            default:
                if (IsCharacterKey(LastInput))
                {
                    TerminalDrawChar(TermChar, true);

                    if (CharIndex >= CommandLength)
                    {
                        Command[CharIndex] = TermChar;
                    }
                    else
                    {
                        InsertCharIntoString(Command, TermChar, CharIndex);

                        // We'll have to redraw the command now
                        for (int DrawCharIndex = CharIndex + 1; DrawCharIndex <= CommandLength; DrawCharIndex++)
                        {
                            TerminalDrawChar(Command[DrawCharIndex], true);
                        }

                        MoveCursor(CursorColumn - (CommandLength - CharIndex), CursorRow);
                    }
                    
                    CharIndex = IntMin(CharIndex + 1, MAX_CMD_LEN);
                    CommandLength++;

                    if (CharIndex >= CommandLength)
                    {
                        DrawFilledRect(CursorColumn * FONT_WIDTH, CursorRow * FONT_HEIGHT, FONT_WIDTH, FONT_HEIGHT, TerminalFGColor);
                    }
                    else
                    {
                        InvertColorOfTextBlock();
                    }
                }
                
                break;
        }      
    }
}