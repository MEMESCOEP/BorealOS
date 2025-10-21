#include "Cat.h"

#include <Core/Kernel.h>

Status CatFile(VFSState *vfs, const char *path) {
    FileHandle* handle = nullptr;
    Status status = vfs->Open(vfs, path, &handle);
    if (status != STATUS_SUCCESS) {
        LOGF(LOG_ERROR, "Failed to open file '%s': %d\n", path, status);
        return status;
    }

    FileStat stat = {};
    if (vfs->Stat(vfs, handle, &stat) != STATUS_SUCCESS) {
        LOGF(LOG_ERROR, "Failed to stat file '%s'\n", path);
        vfs->Close(vfs, handle);
        return STATUS_FAILURE;
    }

    if (stat.IsDirectory) {
        LOGF(LOG_ERROR, "'%s' is a directory, cannot cat.\n", path);
        vfs->Close(vfs, handle);
        return STATUS_INVALID_PARAMETER;
    }
    const size_t bufferSize = 256;
    uint8_t buffer[bufferSize];
    buffer[255] = '\0'; // Null terminate for safety in case of text output
    size_t totalBytesRead = 0;
    while (totalBytesRead < stat.Size) {
        size_t bytesToRead = bufferSize - 1; // Leave space for null terminator
        if (totalBytesRead + bytesToRead > stat.Size) {
            bytesToRead = stat.Size - totalBytesRead;
        }

        size_t bytesRead = 0;
        status = vfs->Read(vfs, handle, buffer, bytesToRead, &bytesRead);
        if (status != STATUS_SUCCESS) {
            LOGF(LOG_ERROR, "Failed to read from file '%s'\n", path);
            vfs->Close(vfs, handle);
            return status;
        }

        if (bytesRead == 0) {
            break; // EOF
        }

        // Write to console
        PRINTF("%s", (char*)buffer);

        totalBytesRead += bytesRead;
    }
    vfs->Close(vfs, handle);
    PRINT("\n"); // Final newline
    return STATUS_SUCCESS;
}
