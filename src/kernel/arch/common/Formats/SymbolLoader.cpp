#include <Formats/SymbolLoader.h>

#include "Utility/StringFormatter.h"

namespace Formats {
    SymbolLoader::SymbolLoader(uint8_t *symbolTable, size_t symbolTableSize) {
        ParseSymbolTable(symbolTable, symbolTableSize);
    }

    uintptr_t SymbolLoader::GetSymbolAddress(const char *symbolName) const {
        // Binary search for the symbol name
        size_t left = 0;
        size_t right = _entryCount;

        while (left < right) {
            size_t mid = left + (right - left) / 2;
            int cmp = strcmp(symbolName, _entries[mid].name);
            if (cmp == 0) {
                return _entries[mid].address;
            } else if (cmp < 0) {
                right = mid;
            } else {
                left = mid + 1;
            }
        }

        return 0;
    }

    void SymbolLoader::ParseSymbolTable(const uint8_t *symbolTable, size_t symbolTableSize) {
        size_t offset = 0;
        _entryCount = 0;

        for (size_t i = 0; i < symbolTableSize; i++) {
            if (symbolTable[i] == '\n') {
                _entryCount++;
            }
        }

        _entries = new SymbolEntry[_entryCount];

        const char* currentSymbolAddress = nullptr;
        size_t currentSymbolAddressLength = 0;
        const char* currentSymbolName = nullptr;
        size_t currentSymbolNameLength = 0;

        _entryCount = 0;

        for (size_t i = 0; i < symbolTableSize; i++) {
            if (symbolTable[i] == ' ') {
                currentSymbolName = reinterpret_cast<const char *>(symbolTable + offset);
                currentSymbolNameLength = i - offset;
                offset = i + 1;
            } else if (symbolTable[i] == '\n') {
                currentSymbolAddress = reinterpret_cast<const char *>(symbolTable + offset);
                currentSymbolAddressLength = i - offset;
                offset = i + 1;

                // Store the symbol entry
                char* nameCopy = new char[currentSymbolNameLength + 1];
                memcpy(nameCopy, currentSymbolName, currentSymbolNameLength);
                nameCopy[currentSymbolNameLength] = '\0'; // Null-terminate the string
                _entries[_entryCount++] = {nameCopy, Utility::StringFormatter::HexToSize(currentSymbolAddress, currentSymbolAddressLength)};
            }
        }
    }
}
