// ReSharper disable CppDFANullDereference
#include <Formats/ELF.h>

namespace Formats {
    ELF::ELF(const uint8_t *data, size_t size, Endianness systemEndianess) : _isValid(false), _data(data), _size(size) {
        // Check ELF magic number
        if (size < 4 || data[EI_MAG0] != ELFMAG0 || data[EI_MAG1] != ELFMAG1 || data[EI_MAG2] != ELFMAG2 || data[EI_MAG3] != ELFMAG3) {
            LOG_ERROR("Invalid ELF magic number!");
            return;
        }

        // Check endianness matches system endianness
        if (data[EI_DATA] == ELFDATA2LSB && systemEndianess != Endianness::LittleEndian) {
            LOG_ERROR("ELF endianness does not match system endianness!");
            return;
        }

        if (data[EI_DATA] == ELFDATA2MSB && systemEndianess != Endianness::BigEndian) {
            LOG_ERROR("ELF endianness does not match system endianness!");
            return;
        }

        _isValid = true; // im trusting it atp
    }
}
