#ifndef MATHUTILS_H
#define MATHUTILS_H

#include <stddef.h>
#include <stdint.h>

/* FUNCTIONS */
//float Log10(float Number);
void SSEVectorAdd(float *A, float *B, float *Result, int Size);
float IntExpPow(float Base, int Exponent);
float AbsValue(float Number);
int IntMax(int Num1, int Num2);
int IntMin(int Num1, int Num2);
int IntSquare(int Num);
uintptr_t AlignUp(uintptr_t Value, size_t Alignment);

#endif