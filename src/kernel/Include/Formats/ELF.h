#ifndef BOREALOS_ELF_H
#define BOREALOS_ELF_H

#include <Definitions.h>

namespace Formats {
    class ELF {
    public:
        enum class EndianessEnum : uint8_t {
            LittleEndian = 1,
            BigEndian = 2
        };

        explicit ELF(const uint8_t *data, size_t size, EndianessEnum systemEndianess = EndianessEnum::LittleEndian);

        enum class InstructionSetEnum : uint16_t {
            None = 0,
            Sparc = 0x02,
            x86 = 0x03,
            MIPS = 0x08,
            PowerPC = 0x14,
            ARM  = 0x28,
            SuperH = 0x2A,
            IA_64 = 0x32,
            x86_64 = 0x3E,
            AArch64 = 0xB7,
            RISC_V = 0xF3
        };

        struct ELFHeaderBase {
            uint8_t Magic[4];
            uint8_t BitMode; // 1 = 32-bit, 2 = 64-bit
            EndianessEnum Endianness; // 1 = little-endian, 2 = big-endian
            uint8_t HeaderVersion;
            uint8_t OSABI; // 0 = System V, 1 = HP-UX, 2 = NetBSD, 3 = Linux, etc. We will only support System V (0)
            uint64_t _reserved0;
            uint16_t Type; // 1 = relocatable, 2 = executable, 3 = shared object, 4 = core
            InstructionSetEnum InstructionSet;
            uint32_t ELFVersion; // 1 for the original version of ELF
        } PACKED;

        struct ELFHeader32 : public ELFHeaderBase {
            uint32_t EntryPoint;
            uint32_t ProgramHeaderOffset;
            uint32_t SectionHeaderOffset;
            uint32_t Flags;
            uint16_t ELFHeaderSize;
            uint16_t ProgramHeaderEntrySize;
            uint16_t ProgramHeaderEntryCount;
            uint16_t SectionHeaderEntrySize;
            uint16_t SectionHeaderEntryCount;
            uint16_t SectionHeaderStringTableIndex;
        } PACKED;

        struct ELFHeader64 : public ELFHeaderBase {
            uint64_t EntryPoint;
            uint64_t ProgramHeaderOffset;
            uint64_t SectionHeaderOffset;
            uint32_t Flags;
            uint16_t ELFHeaderSize;
            uint16_t ProgramHeaderEntrySize;
            uint16_t ProgramHeaderEntryCount;
            uint16_t SectionHeaderEntrySize;
            uint16_t SectionHeaderEntryCount;
            uint16_t SectionHeaderStringTableIndex;
        } PACKED;

        enum class ELFProgramHeaderType : uint32_t {
            Null = 0,
            Load = 1,
            Dynamic = 2,
            Interp = 3,
            Note = 4,
            ShLib = 5,
            PHdr = 6
        };

        struct ProgramHeaderBase {
            ELFProgramHeaderType Type;
            uint32_t Flags;
        } PACKED;

        struct ELFProgramHeader32 : public ProgramHeaderBase {
            uint32_t Offset;
            uint32_t VirtualAddress;
            uint32_t PhysicalAddress;
            uint32_t FileSize;
            uint32_t MemorySize;
            uint32_t Align;
        } PACKED;

        struct ELFProgramHeader64 : public ProgramHeaderBase {
            uint64_t Offset;
            uint64_t VirtualAddress;
            uint64_t PhysicalAddress;
            uint64_t FileSize;
            uint64_t MemorySize;
            uint64_t Align;
        } PACKED;

        [[nodiscard]] bool IsValid() const { return _isValid; }
        [[nodiscard]] const ELFHeaderBase* GetHeader() const { return _header; }

        [[nodiscard]] const ELFHeader64* GetHeader64() const {
            if (_header->BitMode != 2)
                return nullptr;
            return (ELFHeader64*)_header;
        }

        [[nodiscard]] const ELFHeader32* GetHeader32() const {
            if (_header->BitMode != 1)
                return nullptr;
            return (ELFHeader32*)_header;
        }

        [[nodiscard]] const ProgramHeaderBase* GetProgramHeaders() const {
            if (!_isValid) {
                return nullptr;
            }
            return reinterpret_cast<const ProgramHeaderBase*>(reinterpret_cast<const uint8_t*>(_header) + (_header->BitMode == 1 ? GetHeader32()->ProgramHeaderOffset : GetHeader64()->ProgramHeaderOffset));
        }

        [[nodiscard]] const ELFProgramHeader32* GetProgramHeader32(size_t index) const {
            if (_header->BitMode != 1 || index >= GetHeader32()->ProgramHeaderEntryCount) {
                return nullptr;
            }
            return reinterpret_cast<const ELFProgramHeader32*>(reinterpret_cast<const uint8_t*>(_header) + GetHeader32()->ProgramHeaderOffset + index * GetHeader32()->ProgramHeaderEntrySize);
        }

        [[nodiscard]] const ELFProgramHeader64* GetProgramHeader64(size_t index) const {
            if (_header->BitMode != 2 || index >= GetHeader64()->ProgramHeaderEntryCount) {
                return nullptr;
            }
            return reinterpret_cast<const ELFProgramHeader64*>(reinterpret_cast<const uint8_t*>(_header) + GetHeader64()->ProgramHeaderOffset + index * GetHeader64()->ProgramHeaderEntrySize);
        }

    private:
        const ELFHeaderBase* _header;
        bool _isValid;
    };
}

#endif //BOREALOS_ELF_H