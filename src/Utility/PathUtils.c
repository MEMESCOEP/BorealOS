#include "PathUtils.h"

#include "StringTools.h"
#include "Core/Kernel.h"
#include "Core/Memory/HeapAllocator.h"

char * PathNormalize(char *path) {
    // Remove redundant slashes and ensure it starts with a single slash
    size_t writeIndex = 0;
    bool lastWasSlash = false;
    for (size_t readIndex = 0; path[readIndex] != '\0'; readIndex++) {
        if (path[readIndex] == '/') {
            if (!lastWasSlash) {
                path[writeIndex++] = '/';
                lastWasSlash = true;
            }
        } else {
            path[writeIndex++] = path[readIndex];
            lastWasSlash = false;
        }
    }
    // Remove trailing slash if it's not the root
    if (writeIndex > 1 && path[writeIndex - 1] == '/') {
        writeIndex--;
    }
    path[writeIndex] = '\0';
    return path;
}

char * PathGetParent(const char *path, size_t *outSize) {
    size_t lastSlash = StringLastIndexOf(path, '/');
    if (lastSlash == (size_t)-1) {
        // We are creating a directory in the root
        lastSlash = 0;
    }
    char* parentPath = nullptr;
    if (lastSlash == 0) {
        parentPath = StringDuplicate(&Kernel.Heap, "/");
    } else {
        parentPath = (char*)HeapAlloc(&Kernel.Heap, lastSlash + 1);
        if (!parentPath) {
            return nullptr;
        }
        for (size_t i = 0; i < lastSlash; i++) {
            parentPath[i] = path[i];
        }
        parentPath[lastSlash] = '\0';
    }
    if (outSize) {
        *outSize = lastSlash;
    }
    return parentPath;
}

char * PathGetFilename(const char *path) {
    size_t lastSlash = StringLastIndexOf(path, '/');
    const char* filename = (lastSlash == (size_t)-1) ? path : path + lastSlash + 1;
    return StringDuplicate(&Kernel.Heap, filename);
}