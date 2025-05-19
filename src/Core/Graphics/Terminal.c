/* LIBRARIES */
#include <stdbool.h>
#include <stddef.h>
#include <Limine.h>
#include "Utilities/MathUtils.h"
#include "Drivers/HID/Keyboard.h"
#include "Drivers/IO/Serial.h"
#include "Terminal.h"
#include "Graphics.h"


/* VARIABLES */
bool TerminalDrawCursor = true;
bool TerminalDrawCharBG = true;
int TerminalBGColor = 0x000000;
int TerminalFGColor = 0xFFFFFF;
int CursorXPos = 0;
int CursorYPos = 0;
int PrevCursorColumn = 0;
int PrevCursorRow = 0;
int CursorColumn = 0;
int CursorRow = 0;
int ScrollRow = FONT_HEIGHT;


/* FUNCTIONS */
void TerminalDrawChar(unsigned char Character, bool UpdateCursor)
{
    if (Character == '\n')
    {
        CursorColumn--;
        CursorRow++;
    }
    else if (Character == '\r')
    {
        CursorColumn = -1;
    }
    else if (Character == '\b')
    {
        if (FBExists == true)
            DrawChar(CursorXPos, CursorYPos, ' ', TerminalFGColor, TerminalBGColor);

        MoveCursor(CursorColumn - 1, CursorRow);
    }
    else if (Character == '\t')
    {
        for (int I = 0; I < 4; I++)
        {
            if (FBExists == true)
                DrawChar(CursorXPos, CursorYPos, ' ', TerminalFGColor, TerminalBGColor);

            MoveCursor(CursorColumn + 1, CursorRow);
        }
    }
    else if (FBExists == true)
    {
        DrawChar(CursorXPos, CursorYPos, Character, TerminalFGColor, TerminalBGColor);
    }

    if (Character != '\b')
        MoveCursor(CursorColumn + 1, CursorRow);

    if (UpdateCursor == true && FBExists == true)
    {
        DrawCursor();
    }

    if (SerialExists == true && Character != '\n')
    {
        SendCharSerial(ActiveSerialPort, Character);
    }
}

void TerminalDrawString(char* Message)
{
    if (FBExists == true && TerminalDrawCursor == true)
        DrawFilledRect(PrevCursorColumn * FONT_WIDTH, PrevCursorRow * FONT_HEIGHT, FONT_WIDTH, FONT_HEIGHT, TerminalBGColor);
    
    for (int i = 0; Message[i] != '\0'; i++)
    {
        TerminalDrawChar(Message[i], false);
    }

    if (FBExists == true)
        DrawCursor();
}

void TerminalDrawMessage(char* Message, enum MessageTypes MSGType)
{
    int PreviousFGColor = TerminalFGColor;

    switch (MSGType)
    {
        case INFO:
            TerminalFGColor = 0xFFFFFF;
            TerminalDrawChar('[', true);

            TerminalFGColor = 0x00FF00;
            TerminalDrawString("INFO");

            TerminalFGColor = 0xFFFFFF;
            TerminalDrawString("] >> ");
            break;

        case WARNING:
            TerminalFGColor = 0xFFFFFF;
            TerminalDrawChar('[', true);

            TerminalFGColor = 0xFFFF00;
            TerminalDrawString("WARN");

            TerminalFGColor = 0xFFFFFF;
            TerminalDrawString("] >> ");
            break;

        case ERROR:
            TerminalFGColor = 0xFFFFFF;
            TerminalDrawChar('[', true);

            TerminalFGColor = 0xFF0000;
            TerminalDrawString("ERROR");

            TerminalFGColor = 0xFFFFFF;
            TerminalDrawString("] >> ");
            break;

        case DEBUG:
            TerminalFGColor = 0xFFFFFF;
            TerminalDrawChar('[', true);

            TerminalFGColor = 0xFF00FF;
            TerminalDrawString("INFO");

            TerminalFGColor = 0xFFFFFF;
            TerminalDrawString("] >> ");
            break;
    }

    TerminalFGColor = PreviousFGColor;
    TerminalDrawString(Message);
}

void ClearTerminal()
{
    DrawFilledRect(0, 0, ScreenWidth, ScreenHeight, TerminalBGColor);
    MoveCursor(0, 0);
}

// Set the row that the terminal will scroll at. This is useful for preserving graphics that are above text.
void SetScrollRow(int Row)
{
    ScrollRow = (FONT_HEIGHT * Row) + FONT_HEIGHT;
}

void ScrollTerminal(int Direction)
{
    if (AbsValue(Direction) != 1)
    {
        return;
    }

    if (Direction > 0)
    {
        // Scroll up by X rows
        for (int Row = ScrollRow; Row < ScreenHeight; Row++)
        {
            for (int Column = 0; Column < ScreenWidth; Column++)
            {
                CurrentFB[(Row - FONT_HEIGHT) * FBPitchOffset + Column] = CurrentFB[Row * FBPitchOffset + Column];
            }
        }
        
        for (int Row = ScreenHeight - FONT_HEIGHT; Row < ScreenHeight; Row++)
        {
            for (int Column = 0; Column < ScreenWidth; Column++)
            {
                CurrentFB[Row * FBPitchOffset + Column] = 0x00000000;
            }
        }
    }
    else
    {
        // Scroll the screen DOWN by FONT_HEIGHT rows
        // Move lines bottom to top to avoid overwriting
        for (int Row = ScreenHeight - FONT_HEIGHT - 1; Row >= 0; Row--)
        {
            for (int Column = 0; Column < ScreenWidth; Column++)
            {
                CurrentFB[(Row + FONT_HEIGHT) * FBPitchOffset + Column] = CurrentFB[Row * FBPitchOffset + Column];
            }
        }

        // Clear the TOP FONT_HEIGHT rows (they're now new whitespace)
        for (int Row = 0; Row < FONT_HEIGHT; Row++)
        {
            for (int Column = 0; Column < ScreenWidth; Column++)
            {
                CurrentFB[Row * FBPitchOffset + Column] = TerminalBGColor;
            }
        }
    }
}

void InvertColorOfTextBlock()
{
    for (int Y = 0; Y < FONT_HEIGHT; Y++)
    {
        for (int X = 0; X < FONT_WIDTH; X++)
        {
            if (GetPixel(X + CursorXPos, Y + CursorYPos) == TerminalBGColor)
            {
                SetPixel(X + CursorXPos, Y + CursorYPos, TerminalFGColor);
            }
            else if (GetPixel(X + CursorXPos, Y + CursorYPos) == TerminalFGColor)
            {
                SetPixel(X + CursorXPos, Y + CursorYPos, TerminalBGColor);
            }
        }
    }
}