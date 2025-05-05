#ifndef ISRIDT_H
#define ISRIDT_H

/* LIBRARIES */
#include <stdint.h>


/* DEFINITIONS */
#define IDT_MAX_DESCRIPTORS 48


/* VARIABLES */
typedef struct {
    uint64_t r15;
    uint64_t r14;
    uint64_t r13;
    uint64_t r12;
    uint64_t r11;
    uint64_t r10;
    uint64_t r9;
    uint64_t r8;
    uint64_t rdi;
    uint64_t rsi;
    uint64_t rbp;
    uint64_t rbx;
    uint64_t rdx;
    uint64_t rcx;
    uint64_t rax;
	uint64_t InterruptNum;
	uint64_t ErrorCode;
	uint64_t RIP;
	uint64_t CS;
	uint64_t RFLAGS;
	uint64_t SS_RSP;
} StackState;

struct MouseState {
	int8_t DeltaSigns[2]; // X, Y
	int8_t ScrollDelta;
	int8_t DeltaX;
	int8_t DeltaY;
	bool MiddleButtonPressed;
	bool RightButtonPressed;
	bool LeftButtonPressed;
};

extern struct MouseState CurrentMouseState;
extern bool IDTInitialized;
extern StackState* CrashStackState;
extern int MouseDelta[2];



/* FUNCTIONS */
void TestIDT(uint8_t ExceptionSelector);
void InitIDT();

#endif