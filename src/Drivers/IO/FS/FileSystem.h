#ifndef BOREALOS_FILESYSTEM_H
#define BOREALOS_FILESYSTEM_H

#include <Definitions.h>
#include <Drivers/IO/Disk/Block.h>

// This is the interface for a filesystem driver.
// It is designed to be as generic as possible, to allow for different types of filesystems to be implemented.

// Supported functions:
// - Open: Open a file or directory.
// - Close: Close a file or directory.
// - Read: Read data from a file.
// - Write: Write data to a file.
// - Seek: Move the file pointer to a specific location.
// - Tell: Get the current file pointer location.
// - Stat: Get information about a file.
// - ListDir: List the contents of a directory.
// - CreateFile: Create a new file.
// - CreateDir: Create a new directory.
// - Delete: Delete a file or directory.
// - Rename: Rename a file or directory.
// - Mount: Mount the filesystem to a specific path.
// - Unmount: Unmount the filesystem.

typedef struct FileHandle FSFileHandle; // Differs per filesystem implementation

typedef struct FileStat {
    size_t Size; // Size of the file in bytes
    bool IsDirectory; // Whether the file is a directory
    uint32_t Permissions; // File permissions (e.g., read, write, execute)
    uint64_t CreatedAt; // Timestamp of when the file was created
    uint64_t ModifiedAt; // Timestamp of when the file was last modified
} FileStat;

typedef enum {
    MOUNT_FLAG_READONLY = 0x1,
    MOUNT_FLAG_NOEXEC = 0x2,
    MOUNT_FLAG_NOSUID = 0x4
} MountFlags;

typedef struct ListItem {
    char* Name;
    bool IsDirectory;
} FSListItem;

typedef struct FileSystem FileSystem;
typedef struct VFSState VFSState;

typedef Status (*FSOpenFn)              (FileSystem* fs, const char* path, FSFileHandle** outHandle);
typedef Status (*FSCloseFn)             (FileSystem* fs, FSFileHandle* handle);
typedef Status (*FSReadFn)              (FileSystem* fs, FSFileHandle* handle, void* buffer, size_t size, size_t* outBytesRead);
typedef Status (*FSWriteFn)             (FileSystem* fs, FSFileHandle* handle, const void* buffer, size_t size, size_t* outBytesWritten);
typedef Status (*FSSeekFn)              (FileSystem* fs, FSFileHandle* handle, size_t position);
typedef Status (*FSTellFn)              (FileSystem* fs, FSFileHandle* handle, size_t* outPosition);
typedef Status (*FSStatFn)              (FileSystem* fs, FSFileHandle* handle, FileStat* outStat);
typedef Status (*FSListDirFn)           (FileSystem* fs, const char* path, FSListItem** outEntries, size_t* outEntryCount);
typedef Status (*FSCreateFileFn)        (FileSystem* fs, const char* path);
typedef Status (*FSCreateDirectoryFn)   (FileSystem* fs, const char* path);
typedef Status (*FSDeleteFn)            (FileSystem* fs, const char* path);
typedef Status (*FSRenameFn)            (FileSystem* fs, const char* oldPath, const char* newPath);

typedef void* FileSystemData; // Opaque pointer to filesystem-specific data

typedef struct FileSystem {
    const char* Name;
    char* MountPoint; // Where the filesystem is mounted
    FileSystemData Data; // Pointer to filesystem-specific data
    BlockDevice* Device; // Underlying block device (what it operates on)
    FSOpenFn Open;
    FSCloseFn Close;
    FSReadFn Read;
    FSWriteFn Write;
    FSSeekFn Seek;
    FSTellFn Tell;
    FSStatFn Stat;
    FSListDirFn ListDir;
    FSCreateFileFn CreateFile;
    FSCreateDirectoryFn CreateDirectory;
    FSDeleteFn Delete;
    FSRenameFn Rename;
} FileSystem;

#endif //BOREALOS_FILESYSTEM_H