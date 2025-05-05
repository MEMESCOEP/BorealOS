/* LIBRARIES */
#include <stdbool.h>
#include <stddef.h>
#include <Limine.h>
#include "Drivers/HID/Keyboard.h"
#include "Drivers/IO/Serial.h"
#include "Terminal.h"
#include "Graphics.h"


/* VARIABLES */
bool TerminalDrawCursor = true;
bool TerminalDrawCharBG = true;
int TerminalBGColor = 0x000000;
int TerminalFGColor = 0x00FF00;
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

void ScrollTerminal(int Rows __attribute__((unused)))
{
    // Scroll up by X rows
    for (int Row = ScrollRow; Row < ScreenHeight; Row++) {
        for (int Column = 0; Column < ScreenWidth; Column++) {
            CurrentFB[(Row - FONT_HEIGHT) * FBPitchOffset + Column] =
                CurrentFB[Row * FBPitchOffset + Column];
        }
    }

    // Clear the bottom row
    for (int Row = ScreenHeight - FONT_HEIGHT; Row < ScreenHeight; Row++) {
        for (int Column = 0; Column < ScreenWidth; Column++) {
            CurrentFB[Row * FBPitchOffset + Column] = 0x00000000;
        }
    }
}