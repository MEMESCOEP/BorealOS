#include "System.h"
#include "lai_include.h"

namespace Core::Firmware {
    Hardware::Hardware(ACPI *acpi) {
        _acpi = acpi;
    }

    [[noreturn]]
    void Hardware::Power::Shutdown() {
        lai_enter_sleep(5);
        PANIC("How is the system still awake??");
    }

    [[noreturn]]
    void Hardware::Power::Reboot() {
        lai_acpi_reset();
        PANIC("How did the system fail to reboot??");
    }
} // Core::Firmware