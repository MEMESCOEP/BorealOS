

// This file does some math stuff we dont have because this is compiled for 32 bit x86

#include <Definitions.h>

uint64_t __udivdi3(uint64_t num, uint64_t den) {
    uint64_t quot = 0, qbit = 1;

    // Left-justify denominator and count shift
    while ((int64_t)den >= 0) {
        den <<= 1;
        qbit <<= 1;
    }

    while (qbit) {
        if (den <= num) {
            num -= den;
            quot += qbit;
        }
        den >>= 1;
        qbit >>= 1;
    }
    return quot;
}