/* LIBRARIES */
#include <Utilities/MathUtils.h>
//#include <xmmintrin.h>


/* FUNCTIONS */
/*void SSEVectorAdd(float *A, float *B, float *Result, int Size)
{
    for (int i = 0; i < Size; i += 4)
    {
        // Load four floats from each array into SSE registers
        __m128 V1 = _mm_loadu_ps(&A[i]);
        __m128 V2 = _mm_loadu_ps(&B[i]);

        // Perform the addition using SSE
        __m128 VResult = _mm_add_ps(V1, V2);

        // Store the result back into the result array
        _mm_storeu_ps(&Result[i], VResult);
    }
}*/

// Return a decimal base raised to an integer exponent.
float IntExpPow(float Base, int Exponent)
{
    // Anything to the 0th power is equal to 1. This is placed before we declare the
    // Result and Abs(Exponent) variables because it avoids allocating memory for what
    // is in this case unused variables.
    if (Exponent == 0)
    {
        return 1.0;
    }

    float Result = 0.0;
    
    // We can use the exponent to determine the number of loops. Each loop iteration
    // multiplies the result by the base.
    for (int I = 0; I < AbsValue(Exponent); I++)
    {
        if (I == 0)
        {
            Result = Base;
            continue;    
        }
        
        Result *= Base;
    }
    
    // Negative exponents are equal to the reciprocal of: <Base> to the power of <Exponent>.
    if (Exponent < 0)
    {
        return 1.0 / Result;
    }
    else
    {
        return Result;
    }
}

// Return the absolute value (always positive) of a number.
float AbsValue(float Number)
{
    if (Number < 0)
    {
        return -Number;
    }
    else
    {
        return Number;
    }
}

int IntMax(int Num1, int Num2)
{
    if (Num1 > Num2)
    {
        return Num1;
    }
    else
    {
        return Num2;
    }
}

int IntMin(int Num1, int Num2)
{
    if (Num1 < Num2)
    {
        return Num1;
    }
    else
    {
        return Num2;
    }
}

// Square a signed integer
int IntSquare(int Num)
{
    return Num * Num;
}

uintptr_t AlignUp(uintptr_t Address, size_t Alignment)
{
    if (Alignment == 0 || (Alignment & (Alignment - 1)) != 0) {
        // Alignment must be a power of two
        return Address;
    }

    uintptr_t Mask = Alignment - 1;
    return (Address + Mask) & ~Mask;
}

uintptr_t AlignDown(uintptr_t Address, size_t Alignment)
{
    if (Alignment == 0 || (Alignment & (Alignment - 1)) != 0) {
        // Alignment must be a power of two
        return Address;
    }

    uintptr_t Mask = Alignment - 1;
    return Address & ~Mask;
}