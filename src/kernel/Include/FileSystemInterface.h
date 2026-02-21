#ifndef BOREALOS_FILESYSTEM_H
#define BOREALOS_FILESYSTEM_H

#include <Definitions.h>
#include "Allocator.h"

/// Abstract interface for a file system.
namespace FileSystem {
    struct File; // Opaque file struct, the actual definition is up to the implementation of the FileSystem.

    struct FileInfo {
        size_t size = 0; // The size of the file in bytes.
        bool isDirectory = false; // Whether the file is a directory or not.
    };

    struct Capabilities {
        bool canRead = false;
        bool canWrite = false;
    };

    struct DirectoryInfo {
        size_t entryCount = 0;
        const char** entries = nullptr; // Array of null-terminated strings representing the names of the entries in the directory.
    };

    class FileSystemInterface {
    public:
        explicit FileSystemInterface(Allocator* allocator) : _allocator(allocator) {}
        virtual ~FileSystemInterface() = default;

        /// Returns the capabilities of this file system, such as whether it supports reading and writing.
        [[nodiscard]] virtual Capabilities GetCapabilities() const = 0;

        /// Opens a file at the given path and returns a pointer to a File struct representing it, or nullptr if the file doesn't exist or couldn't be opened.
        [[nodiscard]] virtual File* Open(const char* path) = 0;

        /// Reads up to size bytes from the given file into the provided buffer, and returns the number of bytes actually read, or -1 if there was an error.
        virtual size_t Read(File* file, void* buffer, size_t size) = 0;

        /// Writes size bytes from the provided buffer to the given file, and returns the number of bytes actually written, or -1 if there was an error.
        virtual size_t Write(File* file, const void* buffer, size_t size) = 0;

        /// Returns information about the given file, such as its size and whether it's a directory. Returns true on success, or false if there was an error.
        virtual bool GetFileInfo(File* file, FileInfo* info) = 0;

        /// If the given file is a directory, fills the provided DirectoryInfo struct with information about the directory.
        virtual bool GetDirectoryInfo(File* file, DirectoryInfo* info) = 0;

        /// Frees any resources associated with the given DirectoryInfo struct, such as the entries array.
        virtual void FreeDirectoryInfo(DirectoryInfo* info) = 0;

        /// Closes the given file and releases any resources associated with it.
        virtual void Close(File* file) = 0;

    protected:
        Allocator *_allocator;
    };
}// FileSystem

#endif //BOREALOS_FILESYSTEM_H