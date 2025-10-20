#include "LS.h"

#include <Core/Kernel.h>

#include "Utility/StringTools.h"
#include "Utility/PathUtils.h"

void LSDirectory(VFSState* vfs, const char* path, size_t indentLevel, char* parentPath) {
    if (!vfs || !path) {
        return;
    }

    FSListItem* entries = nullptr;
    size_t entryCount = 0;
    auto status = vfs->ListDir(vfs, path, &entries, &entryCount);
    if (status != STATUS_SUCCESS) {
        // Probably not a directory or doesn't exist, doesnt matter just skip
        return;
    }

    for (size_t i = 0; i < entryCount; i++) {
        char* entryName = entries[i].Name;

        // add parentPath ontop of entryName to get full path
        char* fullPath = nullptr;
        if (strcmp(parentPath, "/") != 0) {
            fullPath = StringConcat3 (&Kernel.Heap, parentPath, "/", entryName);
        }
        else {
            if (entryName[0] == '/') {
                fullPath = StringDuplicate(&Kernel.Heap, entryName);
            } else {
                fullPath = StringConcat(&Kernel.Heap, parentPath, entryName);
            }
        }
        if (!fullPath) {
            continue;
        }

        // Recurse into the entry
        for (size_t j = 0; j < indentLevel; j++) {
            PRINT("  ");
        }
        PRINTF("%s", entryName);
        if (entries[i].IsDirectory) {
            PRINT("/\n");
        } else {
            PRINT("\n");
        }

        LSDirectory(vfs, fullPath, indentLevel + 1, fullPath);
        HeapFree(&Kernel.Heap, fullPath);
        HeapFree(&Kernel.Heap, entryName);
    }

    HeapFree(&Kernel.Heap, entries);
}
