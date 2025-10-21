#ifndef BOREALOS_VFS_H
#define BOREALOS_VFS_H

#include <Definitions.h>

// This is the virtual file system interface
// It manages all mounted file systems and provides a unified interface for file operations.
// It resolves paths to the correct file system, and forwards file operations to the appropriate file system implementation.
// So if you had a ramdisk at /dev/ram0 and a FAT32 filesystem at /mnt/fat32, opening /dev/ram0/file.txt would go to the ramdisk driver, while opening /mnt/fat32/file.txt would go to the FAT32 driver.
#include "FileSystem.h"

// The VFS functions (done to keep consistency with FileSystem function architecture, but could also have been global functions)

typedef struct VFSState VFSState;
typedef struct VFSFileHandle FileHandle;

typedef Status (*VFSOpenFn)              (VFSState* vfs, const char* path, FileHandle** outHandle);
typedef Status (*VFSCloseFn)             (VFSState* vfs, FileHandle* handle);
typedef Status (*VFSReadFn)              (VFSState* vfs, FileHandle* handle, void* buffer, size_t size, size_t* outBytesRead);
typedef Status (*VFSWriteFn)             (VFSState* vfs, FileHandle* handle, const void* buffer, size_t size, size_t* outBytesWritten);
typedef Status (*VFSSeekFn)              (VFSState* vfs, FileHandle* handle, size_t position);
typedef Status (*VFSTellFn)              (VFSState* vfs, FileHandle* handle, size_t* outPosition);
typedef Status (*VFSStatFn)              (VFSState* vfs, FileHandle* handle, FileStat* outStat);
typedef Status (*VFSListDirFn)           (VFSState* vfs, const char* path, FSListItem** outEntries, size_t* outEntryCount);
typedef Status (*VFSCreateFileFn)        (VFSState* vfs, const char* path);
typedef Status (*VFSCreateDirectoryFn)   (VFSState* vfs, const char* path);
typedef Status (*VFSDeleteFn)            (VFSState* vfs, const char* path);
typedef Status (*VFSRenameFn)            (VFSState* vfs, const char* oldPath, const char* newPath);

typedef struct MountedFileSystem {
    FileSystem* FS; // The mounted file system
    char* MountPoint; // The mount point path
    MountFlags Flags; // Mount flags
} MountedFileSystem;

/// The VFS is responsible for managing multiple mounted file systems and routing file operations to the correct one.
/// You can consider it the global root (/) of all file systems in the OS.
/// You can mount a file system to a specific path, and all file operations under that path will be routed to that file system with the mount point stripped from the path.
typedef struct VFSState {
    MountedFileSystem* MountedFileSystems; // Array of mounted file systems
    size_t MountedCount; // Number of mounted file systems
    size_t MountedCapacity; // Capacity of the mounted file systems array

    // VFS function pointers
    VFSOpenFn Open;
    VFSCloseFn Close;
    VFSReadFn Read;
    VFSWriteFn Write;
    VFSSeekFn Seek;
    VFSTellFn Tell;
    VFSStatFn Stat;
    VFSListDirFn ListDir;
    VFSCreateFileFn CreateFile;
    VFSCreateDirectoryFn CreateDirectory;
    VFSDeleteFn Delete;
    VFSRenameFn Rename;
} VFSState;

/// Initialize the VFS subsystem
Status VFSInit(VFSState* vfs);

/// Register (mount) a file system at the given mount point
Status VFSRegisterFileSystem(VFSState* vfs, FileSystem* fs, const char* mountPoint, MountFlags flags);

/// Unregister (unmount) a file system from the given mount point
Status VFSUnregisterFileSystem(VFSState* vfs, const char* mountPoint);

#endif //BOREALOS_VFS_H