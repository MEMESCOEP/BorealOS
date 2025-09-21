#ifndef BOREALOS_DRAWING_H
#define BOREALOS_DRAWING_H

#include <Definitions.h>
void DrawString(char* Text, uint32_t XPos, uint32_t YPos, uint32_t FGColor, uint32_t BGColor);
void DrawChar(char Character, uint32_t XPos, uint32_t YPos, uint32_t FGColor, uint32_t BGColor);
void ClearScreen(uint32_t Color);

#endif //BOREALOS_DRAWING_H