#ifndef BOREALOS_SYMBOLLOADER_H
#define BOREALOS_SYMBOLLOADER_H

#include <Definitions.h>

namespace Formats {
    class SymbolLoader {
    public:
        /// Expects a symbol table in the format of "symbolName symbolAddress\n" for each symbol, where symbolAddress is a hexadecimal string (e.g. "1234ABCD"). It should be sorted by symbol name in ascending order for faster lookups.
        explicit SymbolLoader(uint8_t* symbolTable, size_t symbolTableSize);
        uintptr_t GetSymbolAddress(const char* symbolName) const;

        [[nodiscard]] size_t GetSymbolCount() const { return _entryCount; }

    private:
        struct SymbolEntry {
            const char* name;
            uintptr_t address;
        };

        SymbolEntry* _entries{};
        size_t _entryCount{};

        void ParseSymbolTable(const uint8_t* symbolTable, size_t symbolTableSize);
    };
}

#endif //BOREALOS_SYMBOLLOADER_H
