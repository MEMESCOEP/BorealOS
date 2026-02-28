#include "DriverManager.h"

namespace Core::Drivers {
    DriverManager::DriverManager(const char* directory, Formats::SymbolLoader *symbols, FileSystem::InitRam *fileSystem, Memory::Paging *paging, Memory::PMM *pmm) : _symbols(symbols), _fileSystem(
        fileSystem), _paging(paging), _pmm(pmm), _directory(directory), _loadedModules(nullptr), _loadedModuleCount(0) {

    }

    DriverManager::~DriverManager() {
        for (size_t i = 0; i < _loadedModuleCount; i++) {
            auto& module = _loadedModules[i];
            if (module.isLoaded) {
                _paging->UnmapPages((uintptr_t)module.baseAddress, ALIGN_UP(module.size, Architecture::KernelPageSize) / Architecture::KernelPageSize);
                _pmm->FreePages((uintptr_t)module.baseAddress - DriverBaseAddress, ALIGN_UP(module.size, Architecture::KernelPageSize) / Architecture::KernelPageSize);
            }
            delete module.module;
        }
        delete[] _loadedModules;
    }

    void DriverManager::LoadDriversFromFileSystem() {
        size_t count = 0;
        auto drivers = GetDriversInDirectory(_directory, count);
        if (!drivers) {
            LOG_ERROR("Failed to get drivers from directory %s!", _directory);
            return;
        }

        LOG_INFO("Found %u64 driver(s) in directory %s!", count, _directory);

        Formats::DriverModule *validModules[count];
        size_t validModuleCount = 0;

        // Convert the files from raw data, to elf modules, and filter out the invalid ones.
        for (size_t i = 0; i < count; i++) {
            const char* driverPath = drivers[i];

            auto file = _fileSystem->Open(driverPath);
            if (!file) {
                LOG_ERROR("Failed to open driver file %s!", driverPath);
                continue;
            }

            FileSystem::FileInfo info;
            _fileSystem->GetFileInfo(file, &info);

            auto fileData = new uint8_t[info.size];
            _fileSystem->Read(file, fileData, info.size);

            Formats::ELF elf(fileData, info.size);
            if (!elf.IsValid()) {
                LOG_ERROR("Invalid ELF file for driver %s!", driverPath);
                delete[] fileData;
                continue;
            }

            auto driverModule = new Formats::DriverModule(elf);
            if (!driverModule->IsValid()) {
                LOG_ERROR("Invalid driver module in ELF file for driver %s!", driverPath);
                delete[] fileData;
                continue;
            }
            validModules[validModuleCount++] = driverModule;
        }

        // Gather all modules, and get them ready for loading.
        LoadedModule* loadedModules[validModuleCount];
        size_t loadedModuleCount = 0;
        for (size_t i = 0; i < validModuleCount; i++) {
            LOG_DEBUG("Processing driver module %s with %u64 reliance(s)", validModules[i]->GetModuleInfo()->name, validModules[i]->GetRelianceCount());
            auto module = LoadModule(validModules[i]);
            if (module) {
                loadedModules[loadedModuleCount++] = module;
            }
        }

        LoadInTopologicalOrder(loadedModules, loadedModuleCount);
    }

    DriverManager::LoadedModule * DriverManager::LoadModule(Formats::DriverModule *module) {
        // Order of operations:
        // 1. Parse ELF sections to determine total memory size needed for the driver, and
        //    the virtual address where each section should be loaded.
        // 2. Allocate physical memory for the driver, and map it to the virtual address
        //    where the driver will be loaded.
        // 3. Copy the sections from the ELF file to the allocated memory.
        // 4. Perform relocations for the driver, using the symbol table and the provided symbols from the kernel.
        // 5. Return a LoadedModule struct containing the loaded driver information, such as the base address and size of the loaded driver in memory, and pointers to the compatible and load functions.

        // 1: Parse ELF sections to determine total memory size needed.
        auto *elf = module->GetELF();
        auto *header = (Formats::Elf64_Ehdr*)elf->GetHeader();
        auto *elfBase = (uint8_t*)elf->GetHeader();
        auto *shdrs = (Formats::Elf64_Shdr*)(elfBase + header->e_shoff);

        Formats::Elf64_Sym *symtab = nullptr; // Symbol table
        size_t symCount = 0;
        const char* strtab = nullptr; // String table for symbol names

        for (uint16_t i = 0; i < header->e_shnum; i++) {
            if (shdrs[i].sh_type == SHT_SYMTAB) {
                symtab = (Formats::Elf64_Sym*)(elfBase + shdrs[i].sh_offset);
                symCount = shdrs[i].sh_size / shdrs[i].sh_entsize;
                strtab = (const char*)(elfBase + shdrs[shdrs[i].sh_link].sh_offset);
                break;
            }
        }

        if (!symtab || !strtab)
        {
            LOG_ERROR("Driver %s has no symbol table", module->GetModuleInfo()->name);
            return nullptr;
        }

        uintptr_t sectionAddr[header->e_shnum];
        memset(sectionAddr, 0, sizeof(sectionAddr));

        // Get all allocatable sections
        size_t totalSize = 0;
        for (uint16_t i = 0; i < header->e_shnum; i++) {
            auto &sh = shdrs[i];
            if (!(sh.sh_flags & SHF_ALLOC)) continue;

            if (sh.sh_addralign > 1) {
                totalSize = ALIGN_UP(totalSize, (size_t)sh.sh_addralign);
            }

            sectionAddr[i] = totalSize;
            totalSize += sh.sh_size;
        }

        if (totalSize == 0)
        {
            LOG_ERROR("Driver %s has no allocatable sections", module->GetModuleInfo()->name);
            return nullptr;
        }

        // 2: Allocate physical memory for the driver, and map it to the virtual address where the driver will be loaded.
        size_t pageCount = ALIGN_UP(totalSize, Architecture::KernelPageSize) / Architecture::KernelPageSize;
        uintptr_t physBase = _pmm->AllocatePages(pageCount);

        if (!physBase) {
            LOG_ERROR("PMM out of memory for driver %s", module->GetModuleInfo()->name);
            return nullptr;
        }

        uintptr_t loadBase = DriverBaseAddress + driverBaseAddressOffset;
        _paging->MapPages(loadBase, physBase, pageCount, Memory::PageFlags::Present | Memory::PageFlags::ReadWrite);
        memset((void*)loadBase, 0, pageCount * Architecture::KernelPageSize);

        // Keep track of the base address of each section, so we can perform relocations later.
        for (uint16_t i = 0; i < header->e_shnum; i++)
        {
            if (shdrs[i].sh_flags & SHF_ALLOC)
                sectionAddr[i] += loadBase;
        }

        // 3: Copy the sections from the ELF file to the allocated memory.
        for (uint16_t i = 0; i < header->e_shnum; i++)
        {
            auto& sh = shdrs[i];
            if (!(sh.sh_flags & SHF_ALLOC)) continue;

            if (sh.sh_type != SHT_NOBITS) {
                memcpy((void*)sectionAddr[i], elfBase + sh.sh_offset, sh.sh_size);
            }
            // NOBITS is already zeroed from the memset above
        }

        // 4: Perform relocations for the driver, using the symbol table and the provided symbols from the kernel.
        for (uint16_t i = 0; i < header->e_shnum; i++)
        {
            auto& sh = shdrs[i];
            if (sh.sh_type != SHT_RELA) continue; // This isn't a relocatable section

            size_t relaCount = sh.sh_size / sh.sh_entsize;
            auto relas = reinterpret_cast<Formats::elf64_rela *>(elfBase + sh.sh_offset);
            uint16_t targetSectionIdx = sh.sh_info;
            if (!(shdrs[targetSectionIdx].sh_flags & SHF_ALLOC)) continue; // If this target section is not allocated, skip it. Otherwise, we page fault when we try to relocate.

            for (size_t j = 0; j < relaCount; j++) {
                auto &rela = relas[j];
                uint32_t symIdx = ELF64_R_SYM(rela.r_info);
                uint32_t type = ELF64_R_TYPE(rela.r_info);

                uintptr_t P = sectionAddr[targetSectionIdx] + rela.r_offset; // the place in the code where the relocation should be applied
                uint64_t A = rela.r_addend; // the constant addend used to compute the value of the relocation.
                uintptr_t S = 0; // the value of the symbol, which may require looking up the symbol table, and possibly resolving symbols from the kernel symbol loader if the symbol is undefined in the driver. If the symbol is absolute, S is just the value of the symbol.

                if (symIdx != 0) {
                    auto &sym = symtab[symIdx];

                    if (sym.st_shndx == SHN_UNDEF) {
                        const char* name = strtab + sym.st_name;
                        S = _symbols->GetSymbolAddress(name);
                        if (!S) {
                            LOG_ERROR("Failed to resolve symbol %s for driver %s", name, module->GetModuleInfo()->name);
                            return nullptr;
                        }
                    }
                    else if (sym.st_shndx == SHN_ABS) { // Absolute symbol, value is not an address but a constant
                        S = sym.st_value;
                    }
                    else {
                        S = sectionAddr[sym.st_shndx] + sym.st_value;
                    }
                }

                switch (type)
                {
                    case R_X86_64_64:
                        *(uint64_t*)P = S + A;
                        break;
                    case R_X86_64_32:
                        *(uint32_t*)P = (uint32_t)(S + A);
                        break;
                    case R_X86_64_32S:
                        *(int32_t*)P = (int32_t)(S + A);
                        break;
                    case R_X86_64_PC32:
                    case R_X86_64_PLT32:
                        *(uint32_t*)P = (uint32_t)(S + A - P);
                        break;
                    case R_X86_64_PC64:
                        *(uint64_t*)P = S + A - P;
                        break;
                    default:
                        LOG_ERROR("Unsupported reloc type %u32 in driver %s", type,
                                  module->GetModuleInfo()->name);
                        _pmm->FreePages(physBase, pageCount);
                        _paging->UnmapPages(loadBase, pageCount);
                        return nullptr;
                }
            }
        }

        // 5: Return a LoadedModule struct containing the loaded driver information, such as the base address and size of the loaded driver in memory, and pointers to the compatible and load functions.
        uintptr_t compatibleAddr = 0;
        uintptr_t startAddr = 0;

        for (size_t i = 0; i < symCount; i++) {
            auto& sym = symtab[i];
            if (sym.st_shndx == SHN_UNDEF) continue;

            const char* name = strtab + sym.st_name;

            if (strcmp(name, STRINGIFY(MODULE_COMPATIBLE_FUNC_NAME)) == 0)
                compatibleAddr = sectionAddr[sym.st_shndx] + sym.st_value; // The address of the compatible function.
            else if (strcmp(name, "_start") == 0)
                startAddr = sectionAddr[sym.st_shndx] + sym.st_value; // The address of the load function.
        }

        if (!compatibleAddr) {
            LOG_ERROR("Driver %s has no compatible function", module->GetModuleInfo()->name);
            _pmm->FreePages(physBase, pageCount);
            _paging->UnmapPages(loadBase, pageCount);
            return nullptr;
        }

        if (!startAddr) {
            LOG_ERROR("Driver %s has no load function", module->GetModuleInfo()->name);
            _pmm->FreePages(physBase, pageCount);
            _paging->UnmapPages(loadBase, pageCount);
            return nullptr;
        }

        auto compatibleFunc = reinterpret_cast<CompatibleFunc>(compatibleAddr);
        if (!compatibleFunc()) {
            LOG_ERROR("Driver %s is not compatible with this system", module->GetModuleInfo()->name);
            _pmm->FreePages(physBase, pageCount);
            _paging->UnmapPages(loadBase, pageCount);
            return nullptr;
        }

        auto loadFunc = reinterpret_cast<LoadFunc>(startAddr);

        auto loadedModule = new LoadedModule{
                .module = module,
                .baseAddress = (void*)loadBase,
                .size = pageCount * Architecture::KernelPageSize,
                .compatibleFunc = compatibleFunc,
                .loadFunc = loadFunc,
                .isLoaded = false
        };

        driverBaseAddressOffset += (pageCount + 1) * Architecture::KernelPageSize; // Increment the offset for the next driver, leaving a guard page between drivers to catch any overflows.
        return loadedModule;
    }

    void DriverManager::LoadInTopologicalOrder(LoadedModule **loaded_modules, size_t size) {
        // Now we have all the drivers loaded in memory, but we haven't called their load functions yet.
        // Modules can rely on other modules/services those modules export. So to ensure that we load the modules in the correct order, we need to perform a topological sort on the modules based on their dependencies.
        // https://en.wikipedia.org/wiki/Directed_acyclic_graph

        // Steps:
        // 1. Create a graph where each node is a module, and there is a directed edge from module A to module B if module A relies on module B.
        // 2. Perform a topological sort on the graph to get an order of modules to load that respects the dependencies.
        // 3. Load the modules in the order determined by the topological sort, and if any module fails to load, mark all of its dependents as failed as well.

        struct ModuleNode {
            const char* id{};
            size_t originalIndex = 0;
            int inDegree = 0;
            bool processed = false;
            bool failed = false;
            struct Edge* dependentsHead = nullptr;
        };

        struct Edge {
            ModuleNode* dependent;
            Edge* next;
        };

        // Initialize nodes
        ModuleNode nodes[size];
        for (size_t i = 0; i < size; i++) {
            auto& module = loaded_modules[i]->module;
            auto& node = nodes[i];
            node.id = module->GetModuleInfo()->name;
            node.originalIndex = i;
            node.inDegree = 0;
            node.processed = false;
            node.failed = false;
            node.dependentsHead = nullptr;
        }

        // 1: Create graph edges based on module dependencies, and calculate in-degrees for topological sort.
        for (size_t i = 0; i < size; i++) {
            auto& module = loaded_modules[i]->module;
            auto& node = nodes[i];
            for (size_t j = 0; j < module->GetRelianceCount(); j++) {
                auto& reliance = module->GetRelianceList()[j];

                ModuleNode* relianceNode = nullptr;
                for (size_t k = 0; k < size; k++) {
                    if (strcmp(nodes[k].id, reliance.name) == 0) {
                        relianceNode = &nodes[k];
                        break;
                    }
                }

                if (relianceNode == nullptr) {
                    LOG_ERROR("Driver %s relies on missing module %s", node.id, reliance.name);
                    node.failed = true;
                    continue;
                }

                // Add an edge from the reliance node to the current node, and increment the in-degree of the current node.
                auto* edge = new Edge{.dependent = &node, .next = relianceNode->dependentsHead};
                relianceNode->dependentsHead = edge;
                node.inDegree++;
            }
        }

        size_t sortedIndices[size];
        size_t processedCount = 0;

        // 2: Perform topological sort using Kahn's algorithm, and propagate failure to dependents during the sort as well.
        while (processedCount < size) {
            bool progress = false;
            for (size_t i = 0; i < size; i++) {
                auto& node = nodes[i];

                if (!node.processed && node.inDegree == 0) {
                    node.processed = true;
                    sortedIndices[processedCount++] = i;
                    progress = true;

                    // Propagate failure to dependents during sort
                    Edge* edge = node.dependentsHead;
                    while (edge != nullptr) {
                        if (node.failed) {
                            edge->dependent->failed = true;
                        }
                        edge->dependent->inDegree--;
                        edge = edge->next;
                    }
                }
            }

            if (!progress) {
                PANIC("Circular dependency detected in drivers to load!");
            }
        }

        // 3: Load the modules in the order determined by the topological sort, and if any module fails to load, mark all of its dependents as failed as well.
        for (size_t i = 0; i < size; i++) {
            auto& node = nodes[sortedIndices[i]];
            auto& loadedDriver = loaded_modules[node.originalIndex];

            if (node.failed) {
                LOG_ERROR("Driver %s skipped due to dependency failure", node.id);
                if (loaded_modules[node.originalIndex]->module->GetModuleInfo()->importance == Formats::DriverModule::Importance::Required) {
                    PANIC("Required driver failed due to dependencies!");
                }
                continue;
            }

            if (!loadedDriver->loadFunc) {
                LOG_ERROR("Driver %s has no load function", node.id);
                node.failed = true;
            } else if (loadedDriver->loadFunc() != STATUS::SUCCESS) {
                LOG_ERROR("Driver %s failed to load", node.id);
                node.failed = true;
            } else {
                loadedDriver->isLoaded = true;
                LOG_INFO("Driver %s loaded successfully", node.id);
            }

            // If loading failed just now, propagate to all dependents down the chain
            if (node.failed) {
                Edge* edge = node.dependentsHead;
                while (edge != nullptr) {
                    edge->dependent->failed = true;
                    edge = edge->next;
                }

                if (loadedDriver->module->GetModuleInfo()->importance == Formats::DriverModule::Importance::Required) {
                    PANIC("Required driver failed to load!");
                }
            }
        }
    }

    const char **DriverManager::GetDriversInDirectory(const char *str, size_t &count) const {
        auto dir = _fileSystem->Open(str);
        if (!dir) {
            count = 0;
            return nullptr;
        }

        FileSystem::DirectoryInfo info;
        if (!_fileSystem->GetDirectoryInfo(dir, &info)) {
            count = 0;
            return nullptr;
        }

        size_t actualCount = 0;
        for (size_t i = 0; i < info.entryCount; i++) {
            if (strstr(info.entries[i], ".drv") != nullptr) {
                actualCount++;
            }
        }

        const char** result = new const char*[actualCount];
        size_t index = 0;
        for (size_t i = 0; i < info.entryCount; i++) {
            if (strstr(info.entries[i], ".drv") != nullptr) {
                result[index] = info.entries[i];
                index++;
            }
        }

        count = actualCount;
        return result;
    }
}
