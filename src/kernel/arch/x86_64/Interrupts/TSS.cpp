#include "TSS.h"

extern "C" {
    extern char _fault_handler_stack_bottom[];
    extern char _fault_handler_stack_top[];
}

namespace Interrupts {
    TSS::TSSStruct TSS::_tss = {};

    void TSS::Initialize() {
        // Set RSP0 to the top of the kernel stack
        // TODO: RSP0 must be set per CPU when we implement SMP support, so uhh yeah update this then.
        _tss.RSP0 = reinterpret_cast<uint64_t>(Architecture::KernelStackTop);

        // Set IST1 to the top of the double fault stack (defined in entry.S)
        _tss.IST1 = reinterpret_cast<uint64_t>(&_fault_handler_stack_top[0]);

        // We don't use an I/O map, so set the base to the size of the TSS
        _tss.IOMapBase = sizeof(TSSStruct);
    }

    TSS::TSSStruct* TSS::GetTSSStruct() {
        return &_tss;
    }
} // Interrupts