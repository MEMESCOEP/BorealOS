#ifndef BOREALOS_DRIVERMANAGER_H
#define BOREALOS_DRIVERMANAGER_H

#include <Definitions.h>
#include <Formats/DriverModule.h>

#include "../../FileSystems/InitRam.h"
#include "../../Memory/Paging.h"
#include "Formats/SymbolLoader.h"

namespace Core::Drivers {
    class DriverManager {
    public:
        DriverManager(const char* directory, Formats::SymbolLoader *symbols, FileSystem::InitRam* fileSystem, Memory::Paging* paging, Memory::PMM *pmm);
        ~DriverManager();

        void LoadDriversFromFileSystem();

        static constexpr uint64_t DriverBaseAddress = 512 * Constants::GiB; // 512 GiB, this is the virtual address where we will start loading drivers. Probably enough.
    private:
        size_t driverBaseAddressOffset = 0;

        Formats::SymbolLoader *_symbols;
        FileSystem::InitRam* _fileSystem;
        Memory::Paging* _paging;
        Memory::PMM* _pmm;
        const char* _directory;

        typedef int (*CompatibleFunc)();
        typedef STATUS (*LoadFunc)();

        struct LoadedModule {
            Formats::DriverModule *module;
            void* baseAddress; // where the .text, .data, and .bss sections are loaded in memory
            size_t size; // total size of the loaded module in memory
            CompatibleFunc compatibleFunc;
            LoadFunc loadFunc;
            bool isLoaded;
        };

        LoadedModule *_loadedModules;
        size_t _loadedModuleCount;

        LoadedModule* LoadModule(Formats::DriverModule *module);
        void LoadInTopologicalOrder(LoadedModule ** loaded_modules, size_t size);
        const char **GetDriversInDirectory(const char * str, size_t &count) const;
    };
}

#endif //BOREALOS_DRIVERMANAGER_H
