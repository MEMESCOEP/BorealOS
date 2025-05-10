#ifndef STRUTILS_H
#define STRUTILS_H

/* LIBRARIES */
#include <stdint.h>


/* DEFINITIONS */
#define STRINGIFY(X) #X
#define PREPROCESSOR_TOSTRING(X) STRINGIFY(X) // Expands X and converts the result into a string


/* FUNCTIONS */
void StrCat(char* Str1, char* Str2, char* Buffer);
void IntToStr(uint64_t Num, char *Buffer, int Base);
void FloatToStr(float Num, char *Buffer, int DecimalPlaces);
void InsertCharIntoString(char* String, char Character, int Index);
void InsertString(char* OriginalString, char* StringToAppend, int Index);
void TrimTrailingWhitespace(char* String);
int StrLen(char* String);
int StrPixelLength(char* String);
int StrCmp(const char *Str1, const char *Str2);

#endif