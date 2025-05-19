#ifndef PIC_H
#define PIC_H

/* LIBRARIES */
#include <stdint.h>


/* DEFINITIONS */
#define PIC1		         0x20		/* IO base address for master PIC */
#define PIC2		         0xA0		/* IO base address for slave PIC */
#define PIC1_COMMAND	     PIC1
#define PIC1_DATA	         (PIC1+1)
#define PIC2_COMMAND	     PIC2
#define PIC2_DATA	         (PIC2+1)
#define PIC_EOI		         0x20		/* End-of-interrupt command code */
#define ICW1_ICW4	         0x01		/* Indicates that ICW4 will be present */
#define ICW1_SINGLE	         0x02		/* Single (cascade) mode */
#define ICW1_INTERVAL4	     0x04		/* Call address interval 4 (8) */
#define ICW1_LEVEL	         0x08		/* Level triggered (edge) mode */
#define ICW1_INIT	         0x10		/* Initialization - required! */
#define ICW4_8086	         0x01		/* 8086/88 (MCS-80/85) mode */
#define ICW4_AUTO	         0x02		/* Auto (normal) EOI */
#define ICW4_BUF_SLAVE       0x08		/* Buffered mode/slave */
#define ICW4_BUF_MASTER	     0x0C		/* Buffered mode/master */
#define ICW4_SFNM	         0x10		/* Special fully nested (not) */
#define PIC_READ_IRR         0x0A       /* OCW3 irq ready next CMD read */
#define PIC_READ_ISR         0x0B       /* OCW3 irq service next CMD read */
#define APIC_BASE_MSR        0x1B
#define APIC_BASE_MSR_BSP    0x100      // Processor is a BSP
#define APIC_BASE_MSR_ENABLE 0x800


/* FUNCTIONS */
void InitAPIC();
void InitPIC();
void PICSendEOI(uint8_t IRQ);
void PICClearIRQMask(uint8_t IRQLine);
void PICSetIRQMask(uint8_t IRQLine);

#endif