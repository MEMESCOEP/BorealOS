#include <Definitions.h>
#include <Kernel.h>

#include "KernelData.h"

extern "C" [[noreturn]] void main() {
    auto kernel = Kernel<KernelData>::GetInstance();
    kernel->Initialize();
    kernel->Start();

    while (true) {
        asm ("hlt");
    }
}
