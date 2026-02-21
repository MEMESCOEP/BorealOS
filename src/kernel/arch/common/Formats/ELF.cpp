#include <Formats/ELF.h>

namespace Formats {
    ELF::ELF(const uint8_t *data, size_t size, EndianessEnum systemEndianess) : _header(nullptr), _isValid(false) {
        if (size < sizeof(ELFHeaderBase)) {
            return; // Not enough data for even the base header
        }

        auto* baseHeader = reinterpret_cast<const ELFHeaderBase*>(data);
        if (baseHeader->Magic[0] != 0x7F || baseHeader->Magic[1] != 'E' || baseHeader->Magic[2] != 'L' || baseHeader->Magic[3] != 'F') {
            return; // Invalid magic number
        }

        if (baseHeader->OSABI != 0) {
            return; // Unsupported OSABI
        }

        if (baseHeader->Endianness != systemEndianess) {
            return; // Endianess doesn't match the system's endianess
        }

        if (baseHeader->BitMode == 1) {
            if (size < sizeof(ELFHeader32)) {
                return; // Not enough data for a 32-bit header
            }

            auto* header32 = reinterpret_cast<const ELFHeader32*>(data);
            if (header32->ELFVersion != 1) {
                return; // Unsupported ELF version
            }

            _header = header32;
        } else if (baseHeader->BitMode == 2) {
            if (size < sizeof(ELFHeader64)) {
                return; // Not enough data for a 64-bit header
            }

            auto* header64 = reinterpret_cast<const ELFHeader64*>(data);
            if (header64->ELFVersion != 1) {
                return; // Unsupported ELF version
            }

            _header = header64;
        } else {
            return; // Invalid bit mode
        }

        // Now it's valid!
        _isValid = true;
    }
}
