/* LIBRARIES */
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <Limine.h>
#include "Utilities/StrUtils.h"
#include "Terminal.h"
#include "Graphics.h"
#include "Kernel.h"


/* ATTRIBUTES */
// The Limine requests can be placed anywhere, but it is important that
// the compiler does not optimize them away, so, usually, they should
// be made volatile or equivalent, _and_ they should be accessed at least
// once or marked as used with the "used" attribute as done here.

__attribute__((used, section(".limine_requests")))
static volatile struct limine_framebuffer_request FBRequest = {
    .id = LIMINE_FRAMEBUFFER_REQUEST,
    .revision = 0
};


/* VARIABLES */
struct limine_framebuffer *Framebuffer;
volatile uint32_t *MainFBPointer;
volatile uint32_t *CurrentFB;
bool FBExists = false;
int FBPitchOffset = 0;
int ScreenWidth = 0;
int ScreenHeight = 0;
int FBCount = 0;


/* FUNCTIONS */
void InitGraphics()
{
    // Make sure the bootloader gave us a framebuffer to use.
    if (FBRequest.response == NULL || FBRequest.response->framebuffer_count < 1)
    {
        KernelPanic(-2, "No framebuffer was provided!");
    }

    // Fetch the first framebuffer (assume that it is RGB @ 32 byte pixels).
    Framebuffer = FBRequest.response->framebuffers[0];
    MainFBPointer = Framebuffer->address;
    CurrentFB = MainFBPointer;
    ScreenWidth = Framebuffer->width;
    ScreenHeight = Framebuffer->height;
    FBExists = true;
    FBCount = FBRequest.response->framebuffer_count;

    // Make sure we have 32bpp.
    if (Framebuffer->bpp != 32)
    {
        FBExists = false;
        
        char ErrorBuffer[64];
        char BPPBuffer[3];

        IntToStr(Framebuffer->bpp, BPPBuffer, 10);
        StrCat(BPPBuffer, "bpp framebuffer mode is not yet supported!", ErrorBuffer);
        KernelPanic(0, ErrorBuffer);
    }

    FBPitchOffset = (Framebuffer->pitch / 4);

    // Clear the screen so any leftover data will be gone.
    DrawFilledRect(0, 0, ScreenWidth, ScreenHeight, TerminalBGColor);
}

void Swap(int *A, int *B)
{
    int Temp = *A;
    *A = *B;
    *B = Temp;
}

struct limine_framebuffer* GetFramebufferAtIndex(int FBIndex)
{
    if (FBIndex < 0 || FBIndex >= FBCount)
        KernelPanic(-4, "Framebuffer index is out of range!");

    return FBRequest.response->framebuffers[FBIndex];
}

void DrawChar(int X, int Y, unsigned char Character, int FGColor, int BGColor)
{
    for (int Yi = 0; Yi < FONT_HEIGHT; Yi++)
    {
        for (int Xo = 0; Xo < FONT_WIDTH; Xo++)
        {
            if (TerminalFont[Character * FONT_HEIGHT + Yi] & (1 << (7 - Xo)))
            {
                CurrentFB[(Y + Yi) * ScreenWidth + X + Xo] = FGColor;
            }
            else if (TerminalDrawCharBG == true)
            {
                CurrentFB[(Y + Yi) * ScreenWidth + X + Xo] = BGColor;
            }
        }
    }
}

void DrawString(int X, int Y, char* Message, int FGColor, int BGColor)
{
    for (int Character = 0; Message[Character] != '\0'; Character++)
    {
        DrawChar(X + (Character * FONT_WIDTH), Y, Message[Character], FGColor, BGColor);
    }
}

void SetPixel(int X, int Y, int Color)
{
    CurrentFB[Y * (Framebuffer->pitch / 4) + X] = Color;
}

int GetPixel(int X, int Y)
{
    return CurrentFB[Y * (Framebuffer->pitch / 4) + X];
}

void DrawRect(int X, int Y, int Width, int Height, int Color)
{
    DrawLine(X, Y, X + Width, Y, Color);
    DrawLine(X, Y, X, Y + Height, Color);
    DrawLine(X + Width, Y + Height, X , Y + Height, Color);
    DrawLine(X + Width, Y, X + Width, Y + Height, Color);
}

void DrawFilledRect(int X, int Y, int Width, int Height, int Color)
{
    for (int YPos = Y; YPos < Y + Height; YPos++)
    {
        for (int XPos = X; XPos < X + Width; XPos++)
        {
            CurrentFB[YPos * (Framebuffer->pitch / 4) + XPos] = Color;
        }
    }
}

void MoveCursor(int Column, int Row)
{
    if (Column < 0)
    {
        Column = 0;
    }

    else if (Column > (ScreenWidth / FONT_WIDTH) - 1)
    {
        Column = 0;
        Row++;
    }

    if (Row < 0)
    {
        ScrollTerminal(-1);
    }

    else if (Row > (ScreenHeight / FONT_HEIGHT) - 1)
    {
        ScrollTerminal(1);
        Row = (ScreenHeight / FONT_HEIGHT) - 1;
    }

    CursorXPos = Column * FONT_WIDTH;
    CursorYPos = Row * FONT_HEIGHT;
    PrevCursorColumn = CursorColumn;
    PrevCursorRow = CursorRow;
    CursorColumn = Column;
    CursorRow = Row;
}

void DrawCursor()
{
    PrevCursorColumn = CursorColumn;
    PrevCursorRow = CursorRow;

    if (TerminalDrawCursor == true)
        DrawFilledRect(CursorColumn * FONT_WIDTH, CursorRow * FONT_HEIGHT, FONT_WIDTH, FONT_HEIGHT, TerminalFGColor);
}

void DrawLine(int X1, int Y1, int X2, int Y2, uint32_t Color)
{
    int DX = X2 - X1;
    int DY = Y2 - Y1;

    if (DX < 0)
    {
        DX = -DX;  // Absolute value of DX
    }

    if (DY < 0)
    {
        DY = -DY;  // Absolute value of DY
    }

    int SX = (X1 < X2) ? 1 : -1; // Direction of step in X
    int SY = (Y1 < Y2) ? 1 : -1; // Direction of step in Y
    int Err = DX - DY;

    while (true)
    {
        // If the current pixel is within screen bounds, draw it
        if (X1 >= 0 && X1 < ScreenWidth && Y1 >= 0 && Y1 < ScreenHeight)
        {
            CurrentFB[Y1 * (Framebuffer->pitch / 4) + X1] = Color;
        }

        if (X1 == X2 && Y1 == Y2) break;

        int E2 = Err * 2;

        // Step in X direction
        if (E2 > -DY)
        {
            Err -= DY;
            X1 += SX;
        }

        // Step in Y direction
        if (E2 < DX)
        {
            Err += DX;
            Y1 += SY;
        }
    }
}

void DrawFilledTriangle(int X1, int Y1, int X2, int Y2, int X3, int Y3, int FillColor, int BorderColor)
{
    int DX1, DX2, XStart, XEnd;

    // Step 1: Sort vertices by y-coordinate (ascending).
    if (Y1 > Y2) { Swap(&X1, &X2); Swap(&Y1, &Y2); }
    if (Y2 > Y3) { Swap(&X2, &X3); Swap(&Y2, &Y3); }
    if (Y1 > Y2) { Swap(&X1, &X2); Swap(&Y1, &Y2); }

    // Step 2: Draw the triangle's border first.
    DrawLine(X1, Y1, X2, Y2, BorderColor);
    DrawLine(X2, Y2, X3, Y3, BorderColor);
    DrawLine(X3, Y3, X1, Y1, BorderColor);

    // Step 3: Fill the triangle's interior.
    for (int Y = Y1 + 1; Y < Y3; Y++)
    {
        if (Y < Y2)
        {
            // Calculate left edge (X1, Y1) to (X2, Y2)
            if (Y2 != Y1)
            {
                DX1 = (X2 - X1) * (Y - Y1) / (Y2 - Y1);
            }
            else
            {
                DX1 = 0;
            }

            XStart = X1 + DX1;
            
            // Calculate right edge (X1, Y1 -> X3, Y3)
            if (Y3 != Y1)
            {
                DX2 = (X3 - X1) * (Y - Y1) / (Y3 - Y1);
            }
            else
            {
                DX2 = 0;
            }

            XEnd = X1 + DX2;
        }
        else 
        {
            // Calculate left edge (X2, Y2 -> X3, Y3)
            if (Y3 != Y2)
            {
                DX1 = (X3 - X2) * (Y - Y2) / (Y3 - Y2);
            } 
            else
            {
                DX1 = 0;
            }

            XStart = X2 + DX1;
            
            // Calculate right edge (X1, Y1 -> X3, Y3)
            if (Y3 != Y1)
            {
                DX2 = (X3 - X1) * (Y - Y1) / (Y3 - Y1);
            } 
            else
            {
                DX2 = 0;
            }

            XEnd = X1 + DX2;
        }

        // Ensure XStart is always less than XEnd.
        if (XStart > XEnd)
        {
            Swap(&XStart, &XEnd);
        }

        // Fill the horizontal line between XStart and XEnd, skipping the border pixels (XStart + 1).
        for (int X = XStart + 1; X < XEnd; X++)
        {
            if (X >= ScreenWidth || Y >= ScreenHeight)
            {
                continue;
            }

            CurrentFB[Y * (Framebuffer->pitch / 4) + X] = FillColor;
        }
    }
}