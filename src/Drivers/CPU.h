#ifndef BOREALOS_CPU_H
#define BOREALOS_CPU_H

#include <Definitions.h>

uint64_t CPUReadMSR(uint32_t msr);
void CPUWriteMSR(uint32_t msr, uint64_t value);

bool CPUHasPAT();
Status CPUSetupPAT();

#endif //BOREALOS_CPU_H