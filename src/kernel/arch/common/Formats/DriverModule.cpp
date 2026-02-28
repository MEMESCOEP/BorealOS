#include <Formats/DriverModule.h>

namespace Formats {
    DriverModule::DriverModule(ELF elf) : _isValid(false), _elf(elf), _moduleInfo("", "", {0,0,0}, Importance::Optional), _relianceList(nullptr), _relianceCount(0) {
        ParseELF(&elf);
    }

    void DriverModule::ParseELF(ELF *elf) {
        if (!elf->IsValid()) {
            LOG_ERROR("Invalid ELF file passed to DriverModule constructor!");
            return;
        }

        auto *header = (elf64_hdr*)elf->GetHeader();
        const auto* sectionHeaders = reinterpret_cast<const elf64_shdr *>((uint8_t *) elf->GetHeader() + header->e_shoff);
        const char* stringTable = static_cast<const char *>(elf->GetHeader()) + sectionHeaders[header->e_shstrndx].sh_offset;

        for (size_t i = 0; i < header->e_shnum; i++) {
            auto& section = sectionHeaders[i];
            const char* sectionName = stringTable + section.sh_name;
            if (strcmp(sectionName, MODULE_SECTION_NAME) == 0) {
                if (section.sh_size != sizeof(Module)) {
                    LOG_ERROR("Invalid module section size in ELF file passed to DriverModule constructor! Expected %u64, got %u64", sizeof(Module), section.sh_size);
                    return;
                }
                memcpy(&_moduleInfo, (const void*)((uint8_t*)elf->GetHeader() + section.sh_offset), sizeof(Module));
                _isValid = true;
            } else if (strcmp(sectionName, MODULE_RELIANCE_SECTION_NAME) == 0) {
                if (section.sh_size % sizeof(Reliance) != 0) {
                    LOG_ERROR("Invalid reliance section size in ELF file passed to DriverModule constructor! Size must be a multiple of %u64, got %u64", sizeof(Reliance), section.sh_size);
                    return;
                }
                _relianceCount = section.sh_size / sizeof(Reliance);
                _relianceList = reinterpret_cast<Reliance *>(new uint8_t[section.sh_size]);
                memcpy(_relianceList, (const void*)((uint8_t*)elf->GetHeader() + section.sh_offset), section.sh_size);
            }
        }
    }
}
