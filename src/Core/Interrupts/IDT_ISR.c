/* LIBRARIES */
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "Utilities/MathUtils.h"
#include "Utilities/StrUtils.h"
#include "Drivers/HID/Keyboard.h"
#include "Drivers/HID/Mouse.h"
#include "Drivers/IO/Serial.h"
#include "Core/Graphics/Terminal.h"
#include "Core/Graphics/Graphics.h"
#include "Core/Devices/PIC.h"
#include "Core/IO/RegisterIO.h"
#include "IDT_ISR.h"
#include "Kernel.h"
#include "GDT.h"


/* VARIABLES */
typedef struct {
	uint16_t    ISRLow;      // The lower 16 bits of the ISR's address
	uint16_t    KernelCS;    // The GDT segment selector that the CPU will load into CS before calling the ISR
	uint8_t	    IST;         // The IST in the TSS that the CPU will load into RSP; set to zero for now
	uint8_t     Attributes;  // Type and attributes; see the IDT page
	uint16_t    ISRMid;      // The higher 16 bits of the lower 32 bits of the ISR's address
	uint32_t    ISRHigh;     // The higher 32 bits of the ISR's address
	uint32_t    Reserved;    // Set to zero
} __attribute__((packed)) IDTEntry;

typedef struct {
	uint16_t	Limit;
	uint64_t	Base;
} __attribute__((packed)) IDTTable;

__attribute__((aligned(0x10))) 
static IDTEntry IDTEntries[256];

char* CPUExceptions[] = {
    "<FATAL_CPU_EXCEPTION_0> Division By Zero",
	"<FATAL_CPU_EXCEPTION_1> Debug",
	"<FATAL_CPU_EXCEPTION_2> Non Maskable Interrupt",
	"<FATAL_CPU_EXCEPTION_3> Breakpoint",
	"<FATAL_CPU_EXCEPTION_4> Detected Overflow",
	"<FATAL_CPU_EXCEPTION_5> Out of Bounds",
	"<FATAL_CPU_EXCEPTION_6> Invalid Opcode",
	"<FATAL_CPU_EXCEPTION_7> No Coprocessor",
	"<FATAL_CPU_EXCEPTION_8> Double Fault",
	"<FATAL_CPU_EXCEPTION_9> Coprocessor Segment Overrun",
	"<FATAL_CPU_EXCEPTION_10> Bad TSS",
	"<FATAL_CPU_EXCEPTION_11> Segment Not Present",
	"<FATAL_CPU_EXCEPTION_12> Stack Fault",
	"<FATAL_CPU_EXCEPTION_13> General Protection Fault",
	"<FATAL_CPU_EXCEPTION_14> Page Fault",
	"<FATAL_CPU_EXCEPTION_15> Unknown Interrupt",
	"<FATAL_CPU_EXCEPTION_16> Coprocessor Fault",
	"<FATAL_CPU_EXCEPTION_17> Alignment Check failure (486+)",
	"<FATAL_CPU_EXCEPTION_18> Machine Check failure (Pentium/586+)",
	"<FATAL_CPU_EXCEPTION_19> Reserved",
	"<FATAL_CPU_EXCEPTION_20> Reserved",
	"<FATAL_CPU_EXCEPTION_21> Reserved",
	"<FATAL_CPU_EXCEPTION_22> Reserved",
	"<FATAL_CPU_EXCEPTION_23> Reserved",
	"<FATAL_CPU_EXCEPTION_24> Reserved",
	"<FATAL_CPU_EXCEPTION_25> Reserved",
	"<FATAL_CPU_EXCEPTION_26> Reserved",
	"<FATAL_CPU_EXCEPTION_27> Reserved",
	"<FATAL_CPU_EXCEPTION_28> Reserved",
	"<FATAL_CPU_EXCEPTION_29> Reserved",
	"<FATAL_CPU_EXCEPTION_30> Reserved",
	"<FATAL_CPU_EXCEPTION_31> Reserved",
};

StackState* CrashStackState;
uint8_t MouseCycle = 0;
uint8_t MaxDelta = 200;
struct MouseState CurrentMouseState;
static IDTTable IDTr;
static bool Vectors[IDT_MAX_DESCRIPTORS];
extern void* ISRStubTable[];
bool IDTInitialized = false;
char DescriptorsBuffer[4] = "";
char IRQBuffer[4] = "";
int MouseDelta[2] = {0, 0};


/* FUNCTIONS */
// Throw a kernel panic when the CPU encounters an exception.
void CPUExceptionHandler(StackState* CurrentStackState)
{
	CrashStackState = CurrentStackState;

	// Disable interrupts so nothing will cause more interrupts. This prevents a bad
	// situation from becoming even worse.
	__asm__ volatile ("cli");

	// Since the meanings for CPU exceptions are well-defined, we can simply look up
	// the error type using the interrupt number
	KernelPanic(CurrentStackState->InterruptNum, CPUExceptions[CurrentStackState->InterruptNum]);
}

// Called when a non-exception interrupt is fired. This will be refactored, and device
// IRQ handlers will be given their own functions.
void IRQHandler(StackState* CurrentStackState)
{
	// IRQs start from 32
	switch (CurrentStackState->InterruptNum)
	{
		// Keyboard (IRQ 1)
		case 33:
			PS2KBHandleScancode(GetPS2Scancode());
			break;

		// Mouse (IRQ 12)
		case 44:
			uint8_t Status = InB(PS2_MOUSE_STATUS_REG);

			// Make sure data is actually available by checking bit 0; it should always be 1 if the packet has data.
			if ((Status & 0x01) != 1)
			{
				//DrawString(0, 0, "No mouse data.", 0xFFFFFF, 0x161616);
				break;
			}

			// Validate the packet by checking if bit 5 is set, as it always should be.
			// See the status byte section of the osdev mouse input wiki page:
			// https://wiki.osdev.org/Mouse_Input#Additional_Useless_Mouse_Commands
			if ((Status & 0x20) == 0)
			{
				//DrawString(0, 0, "Invalid PS/2 mouse packet.", 0xFFFFFF, 0x161616);
				break;
			}

			switch (MouseCycle)
			{
				// Mouse byte #1, state byte.
				case 0:
					uint8_t State = MouseRead();

					//CurrentMouseState.ScrollDelta = 0;
					CurrentMouseState.DeltaX = 0;
					CurrentMouseState.DeltaY = 0;

					// Verify the state byte.
					if (State == 0x00)
					{
						break;
					}

					if (!(State & 0x08))
					{
						//DrawString(0, 0, "PS/2 mouse state byte verification failure:", 0xFFFFFF, 0x161616);
						
						for (int i = sizeof(State) - 1; i >= 0; i--)
						{
							// Check the ith bit of num
							if ((State >> i) & 1)
							{
								DrawString(i * 8, 16, "1", 0xFFFFFF, 0x161616);
							}
							else
							{
								DrawString(i * 8, 16, "0", 0xFFFFFF, 0x161616);
							}
						}

						break;
					}

					// Discard the packet if the X and/or Y overflow bits are set.
					if ((State & 0x80) != 0 || (State & 0x40) != 0)
					{
						//DrawString(0, 0, "PS/2 mouse packet discarded due to X/Y overflow.", 0xFFFFFF, 0x161616);
						break;
					}

					// Check bits 00000111 (In order from right to left: left, right, middle).
					CurrentMouseState.LeftButtonPressed = (State & 0x01) != 0;
					CurrentMouseState.RightButtonPressed = (State & 0x02) != 0;
					CurrentMouseState.MiddleButtonPressed = (State & 0x04) != 0;
					MouseCycle++;
					break;

				// Mouse byte #2, X delta.
				case 1:
					CurrentMouseState.DeltaX = MouseRead();
					CurrentMouseState.DeltaSigns[0] = (CurrentMouseState.DeltaX > 0) - (CurrentMouseState.DeltaX < 0);
					MouseCycle++;
					break;

				// Mouse byte #3, Y delta.
				case 2:
					CurrentMouseState.DeltaY = MouseRead();
					CurrentMouseState.DeltaSigns[1] = (CurrentMouseState.DeltaY > 0) - (CurrentMouseState.DeltaY < 0);

					// Clamp the mouse delta to any int between -MaxDelta and MaxDelta, and add it to the current mouse position
					MousePos[0] += IntMin(IntMax(CurrentMouseState.DeltaX, -MaxDelta), MaxDelta) * XSensitivity;
					MousePos[1] -= IntMin(IntMax(CurrentMouseState.DeltaY, -MaxDelta), MaxDelta) * YSensitivity;

					// Ensure the mouse position is inside of the screen boundaries.
					MousePos[0] = IntMin(IntMax(MousePos[0], 0), ScreenWidth);
					MousePos[1] = IntMin(IntMax(MousePos[1], 0), ScreenHeight);
					
					if (MouseID != 3)
					{
						MouseCycle = 0;
					}
					else
					{
						MouseCycle++;
					}

					break;

				// Optional mouse byte #4.
				case 3:
					CurrentMouseState.ScrollDelta = MouseRead();
					MouseCycle = 0;
					break;
			}

			break;

		// This IRQ hasn't been configured yet.
		default:
			IntToStr(CurrentStackState->InterruptNum, IRQBuffer, 10); //CurrentStackState->InterruptNum - 32
			TerminalDrawString("Interrput #");
			TerminalDrawString(IRQBuffer);
			TerminalDrawString(" is not set up yet.\n\r");
			break;
	}

	// Acknowledge the IRQ.
	PICSendEOI(CurrentStackState->InterruptNum);
}

// Configure an IRQ descriptor in the IDT.
void IDTSetDescriptor(uint8_t Vector, void* ISR, uint8_t Flags)
{
    IDTEntry* Descriptor = &IDTEntries[Vector];

    Descriptor->ISRLow        = (uint64_t)ISR & 0xFFFF;
    Descriptor->KernelCS      = 0x28;
    Descriptor->IST           = 0;
    Descriptor->Attributes    = Flags;
    Descriptor->ISRMid        = ((uint64_t)ISR >> 16) & 0xFFFF;
    Descriptor->ISRHigh       = ((uint64_t)ISR >> 32) & 0xFFFFFFFF;
    Descriptor->Reserved      = 0;
}

// Test the IDT by triggering various CPU exceptions. This should throw a kernel panic and not cause a triple fault.
// Exception selection values:
//	0: Division by zero
//	14: Page fault
void TestIDT(uint8_t ExceptionSelector)
{
	switch (ExceptionSelector)
	{
		// Division by zero
		case 0:
			TerminalDrawMessage("Testing IDT with Div/0...\n\r", INFO);
			asm volatile("int $0");
			break;

		// Throw a software breakpoint
		case 3:
			TerminalDrawMessage("Testing IDT with a software breakpoint...\n\r", INFO);
			__asm__("int $3");
			break;

		// Attempt to run an invalid opcode
		case 6:
			TerminalDrawMessage("Testing IDT with an invalid opcode ()...\n\r", INFO);
			__asm__("ud2");
			break;

		// Page fault (NULL pointer deref)
		case 14:
			TerminalDrawMessage("Testing IDT with page fault (NULL pointer deref)...\n\r", INFO);
			volatile int *ptr = NULL;
			volatile int value = *ptr;
			break;

		// Invalid mode, just panic
		default:
			KernelPanic(ExceptionSelector, "Invalid IDT test selection; selection must be between 0 and 31");
			break;
	}
}

void InitIDT()
{
	if (GDTInitialized == false)
	{
		KernelPanic(-7, "The GDT must be initialized before the IDT can be initialized!");
	}

	TerminalDrawMessage("Configuring IDTr base & limit...\n\r", INFO);
    IDTr.Base = (uintptr_t)IDTEntries;
    IDTr.Limit = (uint16_t)sizeof(IDTEntry) * IDT_MAX_DESCRIPTORS - 1;

	IntToStr(IDT_MAX_DESCRIPTORS, DescriptorsBuffer, 10);
	TerminalDrawMessage("Configuring ", INFO);
	TerminalDrawString(DescriptorsBuffer);
	TerminalDrawString(" IDT descriptors...\n\r");

    for (uint8_t Vector = 0; Vector < IDT_MAX_DESCRIPTORS; Vector++)
	{
        IDTSetDescriptor(Vector, ISRStubTable[Vector], 0x8E);
        Vectors[Vector] = true;
    }

	// Load the new IDT and enable interrupts
	TerminalDrawMessage("Loading new IDT & enabling interrupts...\n\r", INFO);
    __asm__ volatile ("lidt %0" : : "m"(IDTr));
    __asm__ volatile ("sti");

	if (InterruptsEnabled() == false)
    {
        KernelPanic(-8, "IDT init process failed to enable interrupts!");
    }

	IDTInitialized = true;
}