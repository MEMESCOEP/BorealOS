#ifndef GRAPHICS_H
#define GRAPHICS_H

/* LIBRARIES */
#include <stdbool.h>


/* VARIABLES */
extern struct limine_framebuffer *Framebuffer;
extern volatile uint32_t *MainFBPointer;
extern volatile uint32_t *CurrentFB;
extern bool FBExists;
extern int FBPitchOffset;
extern int ScreenWidth;
extern int ScreenHeight;
extern int FBCount;


/* FUNCTIONS */
void InitGraphics();
struct limine_framebuffer* GetFramebufferAtIndex(int FBIndex);
void DrawChar(int X, int Y, unsigned char Character, int FGColor, int BGColor);
void DrawString(int X, int Y, char* Message, int FGColor, int BGColor);
void SetPixel(int X, int Y, int Color);
int GetPixel(int X, int Y);
void DrawRect(int X, int Y, int Width, int Height, int Color);
void DrawFilledRect(int X, int Y, int Width, int Height, int Color);
void MoveCursor(int Column, int Row);
void DrawCursor();
void DrawLine(int X1, int Y1, int X2, int Y2, uint32_t Color);
void DrawFilledTriangle(int X1, int Y1, int X2, int Y2, int X3, int Y3, int FillColor, int BorderColor);

#endif