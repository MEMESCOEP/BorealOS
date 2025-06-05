#ifndef ARCHUTILS_H
#define ARCHUTILS_H

/* LIBRARIES */
#include <Utilities/StrUtils.h>


/* DEFINITIONS */
#if defined(__x86_64__) || defined(_M_X64)
    #define ARCH_NAME "x86_64"
#elif defined(__i386__) || defined(_M_IX86)
    #define ARCH_NAME "i386"
#elif defined(__aarch64__)
    #define ARCH_NAME "aarch64"
#elif defined(__arm__) || defined(_M_ARM)
    #define ARCH_NAME "arm"
#elif defined(__powerpc64__)
    #define ARCH_NAME "ppc64"
#elif defined(__riscv)
    #define ARCH_NAME "riscv"
#else
    #define ARCH_NAME "unknown"
#endif

#endif