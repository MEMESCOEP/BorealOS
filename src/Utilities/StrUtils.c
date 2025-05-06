/* LIBRARIES */
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include "Core/Graphics/Terminal.h"
#include "Core/Devices/FPU.h"
#include "StrUtils.h"
#include "Kernel.h"


/* FUNCTIONS */
// Concatenate two strings.
void StrCat(char* Str1, char* Str2, char* Buffer)
{
    size_t i, j;

    // Copy Str1 into Buffer
    for (i = 0; Str1[i] != '\0'; i++)
    {
        Buffer[i] = Str1[i];
    }

    // Append Str2 to Buffer
    for (j = 0; Str2[j] != '\0'; j++)
    {
        Buffer[i + j] = Str2[j];
    }

    // Null-terminate the concatenated string
    Buffer[i + j] = '\0';
}

void IntToStr(uint64_t Num, char *Buffer, int Base)
{
    // Zero is a special case, so we'll handle this manually.
    if (Num == 0)
    {
        Buffer[0] = '0';
        Buffer[1] = '\0';
    }
    else
    {
        int CharIndex = 0;
        bool IsNegative = false;

        // Process each digit.
        while (Num != 0)
        {
            if (Base == 0)
            {
                Num = 0;
            }

            int Remainder = Num % Base;

            // For hexadecimal (Base 16), use uppercase letters for A-F.
            if (Remainder > 9)
            {
                Buffer[CharIndex++] = (Remainder - 10) + 'A';  // Use 'A' instead of 'a'
            }
            else
            {
                Buffer[CharIndex++] = Remainder + '0';
            }

            Num = Num / Base;
        }

        if (IsNegative)
        {
            Buffer[CharIndex++] = '-';
        }        

        // Reverse the string, because we process the integer from right to left.
        for (int ReverseCharIndex = 0; ReverseCharIndex < CharIndex / 2; ReverseCharIndex++)
        {
            char Temp = Buffer[ReverseCharIndex];

            Buffer[ReverseCharIndex] = Buffer[CharIndex - ReverseCharIndex - 1];
            Buffer[CharIndex - ReverseCharIndex - 1] = Temp;
        }

        Buffer[CharIndex] = '\0';
    }
}

// Convert a decimal number of type float to a string to X decimal places.
void FloatToStr(float Num, char *Buffer, int DecimalPlaces)
{
    bool NegativeNum = false;

    if (FPUInitialized == false)
    {
        KernelPanic(0, "FPU must be initialized before using floating point math operations!");
    }

    if (Num < 0.0f)
    {
        Buffer[0] = '-';
        NegativeNum = true;
        Num = -Num;
    }

    // Zero is a special case, so we'll handle this manually.
    if (Num == 0.0f)
    {
        Buffer[0] = '0';
        Buffer[1] = '\0';
    }
    else
    {
        int IntegerPart = (int)Num;
        float DecimalPart = Num - IntegerPart;

        // Convert the integer part to string
        IntToStr(IntegerPart, Buffer, 10);

        // If there's a decimal part, process it
        

        if (DecimalPlaces > 0)
        {
            int CharIndex = NegativeNum == false ? 0 : 1;

            while (Buffer[CharIndex] != '\0')
                CharIndex++; // Find the length of the integer part

            Buffer[CharIndex] = '.';
            CharIndex++;

            // Finally, process each digit of the decimal part
            for (int i = 0; i < DecimalPlaces; i++)
            {
                DecimalPart *= 10;
                int digit = (int)DecimalPart;

                Buffer[CharIndex] = '0' + digit;
                DecimalPart -= digit;
                CharIndex++;
            }

            // Ensure the buffer is properly null-terminated
            Buffer[CharIndex] = '\0';
        }
    }
}

// Insert a character into a string at a specific index.
void InsertCharIntoString(char* String, char Character, int Index)
{
    for (int i = StrLen(String); i >= Index; i--)
    {
        String[i + 1] = String[i];  // Move each character one position to the right
    }

    String[Index] = Character;
    String[StrLen(String)] = '\0';
}

// Insert a string into another at a specific index.
void InsertString(char* OriginalString, char* StringToAppend, int Index)
{
    int AppendLength = StrLen(StringToAppend);

    // Move the characters in the original string after the specified index by the
    // length of the string to append. This will make room for the string to be inserted. 
    for (int i = StrLen(OriginalString); i >= Index; i--)
    {
        OriginalString[i + AppendLength] = OriginalString[i];
    }
    
    // Insert the string into the original string.
    for (int I = 0; I < AppendLength; I++)
    {
        OriginalString[Index + I] = StringToAppend[I];
    }

    OriginalString[StrLen(OriginalString)] = '\0';
}

// Determine the number of characters in a string.
int StrLen(char* String)
{
    int CharIndex = 0;
    int Chars = 0;

    while (String[CharIndex] != '\0')
    {
        CharIndex++;
        Chars++;
    }

    return Chars;
}

// Determine the amount of pixels a string takes up horizontally.
int StrPixelLength(char* String)
{
    return StrLen(String) * FONT_WIDTH;
}

int StrCmp(const char *Str1, const char *Str2)
{
    while (*Str1 && *Str1 == *Str2) { ++Str1; ++Str2; }
    return (int)(unsigned char)(*Str1) - (int)(unsigned char)(*Str2);
}