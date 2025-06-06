#ifndef MATHUTILS_H
#define MATHUTILS_H

/* LIBRARIES */
#include <stddef.h>
#include <stdint.h>


/* DEFINITIONS*/
#define OffsetOf(type, member) ((size_t)&(((type *)0)->member))


/* FUNCTIONS */
//float Log10(float Number);
void SSEVectorAdd(float *A, float *B, float *Result, int Size);
float IntExpPow(float Base, int Exponent);
float AbsValue(float Number);
int IntMax(int Num1, int Num2);
int IntMin(int Num1, int Num2);
int IntSquare(int Num);
uintptr_t AlignUp(uintptr_t Value, size_t Alignment);
uintptr_t AlignDown(uintptr_t Value, size_t Alignment);

#endif