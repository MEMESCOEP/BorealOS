#include "GDT.h"

namespace Interrupts {
    uint64_t GDT::entries[3];
    GDT::GDTPointer GDT::gdtp;

    void GDT::initialize() {

    }
} // Interrupts