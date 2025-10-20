#include "VFS.h"

#include <Core/Kernel.h>
#include <Utility/StringTools.h>

typedef struct VFSFileHandle {
    FileSystem* FS; // The filesystem this handle belongs to
    FSFileHandle* InnerHandle; // The actual file handle from the filesystem
} FileHandle;

static char* GetRelativePath(const char* fullPath, const char* mountPoint) {
    size_t mountPointLen = strlen(mountPoint);
    if (strncmp(fullPath, mountPoint, mountPointLen) == 0) {
        const char* relativePathStart = fullPath + mountPointLen;
        if (*relativePathStart == '/') {
            relativePathStart++; // Skip leading slash
        }
        return StringDuplicate(&Kernel.Heap, relativePathStart);
    }
    return nullptr;
}

UNUSED static char* GetMountPoint(const char* path, VFSState* vfs) {
    for (size_t i = 0; i < vfs->MountedCount; i++) {
        MountedFileSystem* mount = &vfs->MountedFileSystems[i];
        size_t mountPointLen = strlen(mount->MountPoint);
        if (strncmp(path, mount->MountPoint, mountPointLen) == 0) {
            return mount->MountPoint;
        }
    }
    return nullptr;
}

static FileSystem* ResolveFileSystemForPath(VFSState* vfs, const char* path, char** outRelativePath) {
    FileSystem* bestFS = nullptr;
    char* bestRelativePath = nullptr;
    size_t bestLen = 0;

    for (size_t i = 0; i < vfs->MountedCount; i++) {
        MountedFileSystem* mount = &vfs->MountedFileSystems[i];
        size_t mountLen = strlen(mount->MountPoint);

        if (strncmp(path, mount->MountPoint, mountLen) == 0 && mountLen > bestLen) {
            // Found a longer match
            bestLen = mountLen;

            if (bestRelativePath) {
                // Free previous best if needed
                HeapFree(&Kernel.Heap, bestRelativePath);
            }

            bestRelativePath = GetRelativePath(path, mount->MountPoint);
            bestFS = mount->FS;
        }
    }

    if (bestFS) {
        *outRelativePath = bestRelativePath;
        return bestFS;
    }

    return nullptr;
}

static Status OpenFn(VFSState* vfs, const char* path, FileHandle** outHandle) {
    char* relativePath = nullptr;
    FileSystem* fs = ResolveFileSystemForPath(vfs, path, &relativePath);
    if (!fs) {
        return STATUS_NOT_FOUND;
    }
    FSFileHandle* innerHandle = nullptr;
    Status status = fs->Open(fs, relativePath, &innerHandle);
    HeapFree(&Kernel.Heap, relativePath);
    if (status != STATUS_SUCCESS) {
        return status;
    }
    FileHandle* vfsHandle = (FileHandle*)HeapAlloc(&Kernel.Heap, sizeof(FileHandle));
    if (!vfsHandle) {
        fs->Close(fs, innerHandle);
        return STATUS_OUT_OF_MEMORY;
    }
    vfsHandle->FS = fs;
    vfsHandle->InnerHandle = innerHandle;
    *outHandle = vfsHandle;
    return status;
}

// Bunch of unused's because the VFSFileHandle's file system is already stored in them so we don't need the VFSState pointer
static Status CloseFn(UNUSED VFSState* vfs, FileHandle* handle) {
    Status status = handle->FS->Close(handle->FS, handle->InnerHandle);
    HeapFree(&Kernel.Heap, handle);
    return status;
}

static Status ReadFn(UNUSED VFSState* vfs, FileHandle* handle, void* buffer, size_t size, size_t* outBytesRead) {
    return handle->FS->Read(handle->FS, handle->InnerHandle, buffer, size, outBytesRead);
}

static Status WriteFn(UNUSED VFSState* vfs, FileHandle* handle, const void* buffer, size_t size, size_t* outBytesWritten) {
    return handle->FS->Write(handle->FS, handle->InnerHandle, buffer, size, outBytesWritten);
}

static Status SeekFn(UNUSED VFSState* vfs, FileHandle* handle, size_t position) {
    return handle->FS->Seek(handle->FS, handle->InnerHandle, position);
}

static Status TellFn(UNUSED VFSState* vfs, FileHandle* handle, size_t* outPosition) {
    return handle->FS->Tell(handle->FS, handle->InnerHandle, outPosition);
}

static Status StatFn(UNUSED VFSState* vfs, FileHandle* handle, FileStat* outStat) {
    return handle->FS->Stat(handle->FS, handle->InnerHandle, outStat);
}

static Status ListDirFn(VFSState* vfs, const char* path, FSListItem** outEntries, size_t* outEntryCount) {
    char* relativePath = nullptr;
    FileSystem* fs = ResolveFileSystemForPath(vfs, path, &relativePath);
    if (!fs) {
        // This means it was executed on a path that doesn't exist in any mounted filesystem.
        // List out all mounted filesystems falling under path, by checking if a mount point starts with path
        size_t mountCount = 0;
        for (size_t i = 0; i < vfs->MountedCount; i++) {
            MountedFileSystem* mount = &vfs->MountedFileSystems[i];
            if (StringStartsWith(mount->MountPoint, path)) {
                mountCount++;
            }
        }
        if (mountCount == 0) {
            return STATUS_NOT_FOUND;
        }
        FSListItem* mounts = (FSListItem*)HeapAlloc(&Kernel.Heap, sizeof(FSListItem) * mountCount);
        if (!mounts) {
            return STATUS_OUT_OF_MEMORY;
        }
        size_t index = 0;
        for (size_t i = 0; i < vfs->MountedCount; i++) {
            MountedFileSystem* mount = &vfs->MountedFileSystems[i];
            if (StringStartsWith(mount->MountPoint, path)) {
                mounts[index].Name = StringDuplicate(&Kernel.Heap, mount->MountPoint);
                mounts[index].IsDirectory = true; // Mount points are directories
                index++;
            }
        }

        *outEntries = mounts;
        *outEntryCount = mountCount;
        return STATUS_SUCCESS;
    }

    FSListItem* entries = nullptr;
    size_t entryCount = 0;
    Status status = fs->ListDir(fs, relativePath, &entries, &entryCount);
    if (status != STATUS_SUCCESS) {
        return status;
    }

    *outEntries = entries;
    *outEntryCount = entryCount;
    HeapFree(&Kernel.Heap, relativePath);
    return STATUS_SUCCESS;
}

    static Status CreateFileFn(VFSState* vfs, const char* path) {
    char* relativePath = nullptr;
    FileSystem* fs = ResolveFileSystemForPath(vfs, path, &relativePath);
    if (!fs) {
        return STATUS_NOT_FOUND;
    }
    Status status = fs->CreateFile(fs, relativePath);
    HeapFree(&Kernel.Heap, relativePath);
    return status;
}

static Status CreateDirectoryFn(VFSState* vfs, const char* path) {
    char* relativePath = nullptr;
    FileSystem* fs = ResolveFileSystemForPath(vfs, path, &relativePath);
    if (!fs) {
        return STATUS_NOT_FOUND;
    }
    Status status = fs->CreateDirectory(fs, relativePath);
    HeapFree(&Kernel.Heap, relativePath);
    return status;
}

static Status DeleteFn(VFSState* vfs, const char* path) {
    char* relativePath = nullptr;
    FileSystem* fs = ResolveFileSystemForPath(vfs, path, &relativePath);
    if (!fs) {
        return STATUS_NOT_FOUND;
    }
    Status status = fs->Delete(fs, relativePath);
    HeapFree(&Kernel.Heap, relativePath);
    return status;
}

static Status RenameFn(VFSState* vfs, const char* oldPath, const char* newPath) {
    char* relativeOldPath = nullptr;
    FileSystem* fs = ResolveFileSystemForPath(vfs, oldPath, &relativeOldPath);
    if (!fs) {
        return STATUS_NOT_FOUND;
    }
    char* relativeNewPath = nullptr;
    FileSystem* fsNew = ResolveFileSystemForPath(vfs, newPath, &relativeNewPath);
    if (fs != fsNew) {
        HeapFree(&Kernel.Heap, relativeOldPath);
        return STATUS_INVALID_PARAMETER; // Can't rename across filesystems
    }
    Status status = fs->Rename(fs, relativeOldPath, relativeNewPath);
    HeapFree(&Kernel.Heap, relativeOldPath);
    HeapFree(&Kernel.Heap, relativeNewPath);
    return status;
}

Status VFSInit(VFSState *vfs) {
    vfs->MountedCapacity = 8;
    vfs->MountedCount = 0;

    vfs->MountedFileSystems = (MountedFileSystem*)HeapAlloc(&Kernel.Heap, sizeof(MountedFileSystem) * vfs->MountedCapacity);
    if (!vfs->MountedFileSystems) {
        return STATUS_FAILURE;
    }

    vfs->Open = OpenFn;
    vfs->Close = CloseFn;
    vfs->Read = ReadFn;
    vfs->Write = WriteFn;
    vfs->Seek = SeekFn;
    vfs->Tell = TellFn;
    vfs->Stat = StatFn;
    vfs->ListDir = ListDirFn;
    vfs->CreateFile = CreateFileFn;
    vfs->CreateDirectory = CreateDirectoryFn;
    vfs->Delete = DeleteFn;
    vfs->Rename = RenameFn;

    // :3

    return STATUS_SUCCESS;
}

Status VFSRegisterFileSystem(VFSState *vfs, FileSystem *fs, const char *mountPoint, MountFlags flags) {
    for (size_t i = 0; i < vfs->MountedCount; i++) {
        MountedFileSystem* mount = &vfs->MountedFileSystems[i];
        if (strcmp(mount->MountPoint, mountPoint) == 0) {
            return STATUS_ALREADY_EXISTS; // Mount point already in use
        }
    }

    if (vfs->MountedCount >= vfs->MountedCapacity) {
        // Reallocate with double capacity
        size_t newCapacity = vfs->MountedCapacity * 2;
        MountedFileSystem* newArray = (MountedFileSystem*)HeapRealloc(&Kernel.Heap, vfs->MountedFileSystems, sizeof(MountedFileSystem) * newCapacity);
        if (!newArray) {
            return STATUS_OUT_OF_MEMORY;
        }
        vfs->MountedFileSystems = newArray;
        vfs->MountedCapacity = newCapacity;
    }
    MountedFileSystem* mount = &vfs->MountedFileSystems[vfs->MountedCount++];
    mount->FS = fs;
    mount->MountPoint = StringDuplicate(&Kernel.Heap, mountPoint);
    if (!mount->MountPoint) {
        vfs->MountedCount--;
        return STATUS_OUT_OF_MEMORY;
    }
    mount->Flags = flags;

    return STATUS_SUCCESS;
}

Status VFSUnregisterFileSystem(VFSState *vfs, const char *mountPoint) {
    for (size_t i = 0; i < vfs->MountedCount; i++) {
        MountedFileSystem* mount = &vfs->MountedFileSystems[i];
        if (strcmp(mount->MountPoint, mountPoint) == 0) {
            // Found the mount point to unregister
            HeapFree(&Kernel.Heap, mount->MountPoint);
            // Shift remaining mounts down
            for (size_t j = i; j < vfs->MountedCount - 1; j++) {
                vfs->MountedFileSystems[j] = vfs->MountedFileSystems[j + 1];
            }
            vfs->MountedCount--;
            return STATUS_SUCCESS;
        }
    }
    return STATUS_NOT_FOUND;
}