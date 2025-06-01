/* LIBRARIES */
#include <Utilities/StrUtils.h>
#include <Core/Interrupts/IDT.h>
#include <Core/Graphics/Console.h>
#include <Core/Kernel/Panic.h>


/* VARIABLES */
__attribute__((aligned(16)))
extern void *tbl_ISRStubTable[];
static void _SetIDTEntry(uint8_t vector, void *isr, uint8_t flags);
static IDTDescriptor idtDescriptor;
static IDTEntry idtTable[256] = {0};
static bool vectorSet[256] = {0};
static bool irqSet[16] = {0};
void (*irqHandlers[16])(void) = {0};

static const char* ExceptionStrings[] = {
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


/* FUNCTIONS */
void IDTInit(void)
{
	LOG_KERNEL_MSG("\tSetting descriptor base and limit...\n\r", NONE);
    idtDescriptor.base = (uintptr_t)&idtTable[0];
    idtDescriptor.limit = (uint16_t)sizeof(IDTEntry) * 256 - 1;

	LOG_KERNEL_MSG("\tSetting 48 entries...\n\r", NONE);

    for (uint8_t vec = 0; vec < 48; vec++)
    {
        _SetIDTEntry(vec, tbl_ISRStubTable[vec], 0x8E);
        vectorSet[vec] = true;
    }

    for (uint8_t irqHandle = 0; irqHandle < 16; irqHandle++)
	{
        irqSet[irqHandle] = false;
        irqHandlers[irqHandle] = NULL;
	}

    LOG_KERNEL_MSG("\tIssuing \"LIDT\" command to CPU...\n\r", NONE);
    asm volatile("lidt %0" : : "m"(idtDescriptor));

    LOG_KERNEL_MSG("\tIssuing \"STI\" command to CPU...\n\r", NONE);
    asm volatile("sti");

    LOG_KERNEL_MSG("\tIDT init finished.\n\n\r", NONE);
}

void IDTSetIRQHandler(uint8_t irq, void (*handler)(void))
{
    if (irq < 16)
    {
        irqHandlers[irq] = handler;
        irqSet[irq] = true;
    }
    else
    {
        uint8_t X, Y;

        LOG_KERNEL_MSG("Invalid IRQ #", ERROR);
        ConsoleGetCursorPos(&X, &Y);
        ConsoleSetCursor(23, Y - 1);
        PrintNum(irq, 10);
        ConsolePutString("\n\r");
    }
}

static void _SetIDTEntry(uint8_t vector, void *isr, uint8_t flags)
{
    IDTEntry *entry = &idtTable[vector];

    entry->isrLow = (uint32_t)isr & 0xFFFF;
    entry->kernelCode = 0x08; // Kernel code segment
    entry->reserved = 0;
    entry->flags = flags;
    entry->isrHigh = (uint32_t)isr >> 16;
}

__attribute__((noreturn))
void ExceptionHandler(uint32_t errNum)
{
    // Disable interrupts as fast as possible to prevent a bad situation from getting worse
    asm volatile("cli");

    // Pass the exception on to the kernel
    KernelPanic(errNum, ExceptionStrings[errNum]);
}

void IRQHandler(uint8_t irq)
{
    if (irqSet[irq - 0x20] && irqHandlers[irq - 0x20])
    {
        irqHandlers[irq - 0x20]();
    }
    else
    {
        uint8_t highNibble = (irq - 0x20) >> 4;
        uint8_t lowNibble = (irq - 0x20) & 0x0F;

        LOG_KERNEL_MSG("Unhandled IRQ ", ERROR);
        PrintNum(irq - 0x20, 10);
        ConsolePutString(" (HN=0x");
        PrintNum(highNibble, 16);
        ConsolePutString(", LN=0x");
        PrintNum(lowNibble, 16);
        ConsolePutString(")\n\r");
    }

    PICSendEOI(irq);
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
			LOG_KERNEL_MSG("Testing IDT with Div/0...\n\r", INFO);
			asm volatile("int $0");
			break;

		// Throw a software breakpoint
		case 3:
			LOG_KERNEL_MSG("Testing IDT with a software breakpoint...\n\r", INFO);
			__asm__("int $3");
			break;

		// Attempt to run an invalid opcode
		case 6:
			LOG_KERNEL_MSG("Testing IDT with an invalid opcode ()...\n\r", INFO);
			__asm__("ud2");
			break;

		// Page fault (NULL pointer deref)
		case 14:
			LOG_KERNEL_MSG("Testing IDT with page fault (NULL pointer deref)...\n\r", INFO);
			volatile int *ptr = NULL;
			volatile int value = *ptr;
			break;

		// Invalid mode, just panic
		default:
			KernelPanic(ExceptionSelector, "Invalid IDT test selection; selection must be between 0 and 31");
			break;
	}
}