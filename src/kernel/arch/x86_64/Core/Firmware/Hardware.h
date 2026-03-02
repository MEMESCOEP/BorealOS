#ifndef BOREALOS_SYSTEM_H
#define BOREALOS_SYSTEM_H

#include <Definitions.h>
#include "ACPI.h"

namespace Core::Firmware {
    class Hardware {
    private:
        class Power {
        public:
            [[noreturn]]
            void Shutdown();
            [[noreturn]]
            void Reboot();
        };
    public:
        explicit Hardware(ACPI* acpi);

        Power PowerManagement{};
    private:
        ACPI *_acpi;
    };
}

#endif //BOREALOS_SYSTEM_H
