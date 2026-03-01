#ifndef BOREALOS_DRIVERMODULE_H
#define BOREALOS_DRIVERMODULE_H

#include <Definitions.h>

#include "ELF.h"

namespace Formats {
    /// Load driver metadata from an ELF file, this requires the new/delete operators to be available.
    class DriverModule {
    public:
        struct Version {
            uint16_t major;
            uint16_t minor;
            uint16_t patch;
        } PACKED;

        enum class Importance : uint8_t {
            Required,
            Optional
        };

        struct Module {
            const char name[64];
            const char description[256];
            Version version;
            Importance importance;
        } PACKED;

        struct Reliance {
            const char name[64];
            Version version;
        } PACKED;

        explicit DriverModule(ELF elf);

        [[nodiscard]] bool IsValid() const { return _isValid; }
        [[nodiscard]] const Module* GetModuleInfo() const { return &_moduleInfo; }
        [[nodiscard]] const Reliance* GetRelianceList() const { return _relianceList; }
        [[nodiscard]] size_t GetRelianceCount() const { return _relianceCount; }
        [[nodiscard]] const ELF* GetELF() const { return &_elf; }

    private:
        bool _isValid;
        ELF _elf;
        Module _moduleInfo;
        Reliance* _relianceList;
        size_t _relianceCount;

        void ParseELF(ELF * elf);
    };
} // Formats

#endif //BOREALOS_DRIVERMODULE_H