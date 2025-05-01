#ifndef KERNEL_H
#define KERNEL_H

/* DEFINITIONS */
#define LIMINE_HHDM_REQUEST { LIMINE_COMMON_MAGIC, 0x48dcf1cb8ad2b852, 0x63984e959a98244b }
#define LIMINE_FIRMWARE_TYPE_X86BIOS 0
#define LIMINE_FIRMWARE_TYPE_UEFI32 1
#define LIMINE_FIRMWARE_TYPE_UEFI64 2
#define LIMINE_FIRMWARE_TYPE_SBI 3
#define KERNEL_CODENAME "Minokaira"
#define KERNEL_VERSION "P_1" // Prototype versions have versions such as "P_<revision_num>", while alpha builds have versions like "A_<MAJOR_REVISION_NUM>.<MINOR_REVISION_NUM>"


/* VARIABLES */
extern char* FirmwareType;
extern int ProcessorCount;


/* FUNCTIONS */
void KernelPanic(uint64_t ErrorCode, char* Message);
void HaltSystem(void);

#endif