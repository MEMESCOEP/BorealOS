#include "InitRam.h"

#include "Definitions.h"
#include "Utility/StringFormatter.h"

struct FileSystem::File {
    const char* path;
    const size_t* children; // For directories, this is an array of indices into the _files array representing the children of this directory. For regular files, this is nullptr.
    bool isDirectory;
    size_t offset;
    size_t size;
};

namespace File::Systems {
    InitRam::InitRam(limine_file *cpioArchive, Allocator *allocator) : FileSystem(allocator), _cpioArchive(cpioArchive) {
        LOG_INFO("Loading %s", cpioArchive->path);

        // Load the CPIO archive.
        size_t offset = 0;
        size_t fileCount = 0;

        while (offset < cpioArchive->size) {
            auto* p = reinterpret_cast<const char*>(reinterpret_cast<uintptr_t>(cpioArchive->address) + offset);

            size_t filesize  = Utility::StringFormatter::HexToSize((p + 54), 8);
            size_t namesize  = Utility::StringFormatter::HexToSize((p + 94), 8);

            char *name = const_cast<char *>(p + 110);
            if (memcmp(name, CpioNewcTrailer, 11) == 0) {
                break; // Reached the end of the archive
            }

            fileCount++;
            offset += ALIGN_UP(CpioNewcHeaderSize + namesize + ALIGN_UP(filesize, 4), 4);
        }

        _files = new File*[fileCount + 1]; // +1 for a root directory!
        offset = 0;
        size_t fileIndex = 0;

        while (offset < cpioArchive->size) {
            auto* p = reinterpret_cast<const char*>(reinterpret_cast<uintptr_t>(cpioArchive->address) + offset);
            size_t mode      = Utility::StringFormatter::HexToSize(p + 14, 8);
            size_t namesize  = Utility::StringFormatter::HexToSize((p + 94), 8);
            size_t filesize  = Utility::StringFormatter::HexToSize((p + 54), 8);
            size_t dataOffset = ALIGN_UP(CpioNewcHeaderSize + namesize + offset, 4);

            char *name = const_cast<char *>(p + 110);
            if (memcmp(name, CpioNewcTrailer, 11) == 0) {
                break; // Reached the end of the archive
            }

            if ((mode & 0xF000) == 0x4000) {
                _files[fileIndex] = new File{name, nullptr, true, 0, 0}; // For now we won't actually populate the children array for directories since we don't support listing directories or anything like that yet
            }
            else if ((mode & 0xF000) == 0x8000) {
                _files[fileIndex] = new File{name, nullptr, false, dataOffset, filesize};
            }
            else if ((mode & 0xF000) == 0xA000) {
                PANIC("Symlinks are not supported in the init ram filesystem!");
            }

            fileIndex++;
            offset += ALIGN_UP(CpioNewcHeaderSize + namesize + ALIGN_UP(filesize, 4), 4);
        }

        // Resolve children for directories.
        for (size_t i = 0; i < fileCount; i++) {
            if (_files[i]->isDirectory) {
                size_t childCount = 0;

                for (size_t j = 0; j < fileCount; j++) {
                    if (i != j && strncmp(_files[j]->path, _files[i]->path, Utility::StringFormatter::strlen(_files[i]->path)) == 0 && _files[j]->path[Utility::StringFormatter::strlen(_files[i]->path)] == '/') {
                        childCount++;
                    }
                }

                auto* children = new size_t[childCount];
                size_t childIndex = 0;

                for (size_t j = 0; j < fileCount; j++) {
                    if (i != j && strncmp(_files[j]->path, _files[i]->path, Utility::StringFormatter::strlen(_files[i]->path)) == 0 && _files[j]->path[Utility::StringFormatter::strlen(_files[i]->path)] == '/') {
                        children[childIndex++] = j;
                    }
                }

                _files[i]->children = children;
            }
        }

        // Add the root directory, which is a special case since it doesn't actually exist in the CPIO archive but we want it to be there for convenience.
        _files[fileCount] = new File{"/", nullptr, true, 0, 0};
        size_t rootChildCount = 0;
        for (size_t i = 0; i < fileCount; i++) {
            if (strchr(_files[i]->path, '/') == nullptr) {
                rootChildCount++;
            }
        }

        auto* rootChildren = new size_t[rootChildCount];
        size_t rootChildIndex = 0;
        for (size_t i = 0; i < fileCount; i++) {
            // Any file that contains no '/' characters is a child of the root directory
            if (strchr(_files[i]->path, '/') == nullptr) {
                rootChildren[rootChildIndex++] = i;
            }
        }

        _files[fileCount]->children = rootChildren;
        _fileCount = fileCount + 1; // +1 for the root directory
        // List out the root directory for debugging purposes
        LOG_INFO("Root directory:");
        for (size_t i = 0; i < rootChildCount; i++) {
            LOG_INFO(" - %s", _files[rootChildren[i]]->path);
        }
    }

    FileSystem::Capabilities InitRam::GetCapabilities() const {
        return {true, false}; // Can read, can't write
    }

    FileSystem::File* InitRam::Open(const char *path) {
        if (strcmp(path, "/") == 0) {
            return _files[_fileCount - 1]; // The last file is the root directory
        }

        if (path[0] == '/') {
            path++; // Skip the leading '/' since our file paths don't have it
        }

        for (size_t i = 0; _files[i] != nullptr; i++) {
            if (strcmp(_files[i]->path, path) == 0) {
                return _files[i];
            }
        }

        return nullptr; // File not found
    }

    size_t InitRam::Read(File *file, void *buffer, size_t size) {
        auto* p = reinterpret_cast<const char*>(reinterpret_cast<uintptr_t>(_cpioArchive->address) + file->offset);

        if (size > file->size) {
            size = file->size; // Don't read past the end of the file
        }

        memcpy(buffer, p, size);
        return size;
    }

    size_t InitRam::Write(File *file, const void *buffer, size_t size) {
        return -1; // Writing is not supported
    }

    bool InitRam::GetFileInfo(File *file, FileInfo *info) {
        info->size = file->size;
        info->isDirectory = false;
        return true;
    }

    bool InitRam::GetDirectoryInfo(File *file, DirectoryInfo *info) {
        if (!file->isDirectory) {
            return false; // Not a directory
        }

        info->entryCount = 0;
        while (file->children[info->entryCount] != 0) {
            info->entryCount++;
        }

        info->entries = new const char*[info->entryCount];
        for (size_t i = 0; i < info->entryCount; i++) {
            info->entries[i] = _files[file->children[i]]->path;
        }

        return true;
    }

    void InitRam::FreeDirectoryInfo(DirectoryInfo *info) {
        delete[] info->entries;
    }

    void InitRam::Close(File *file) {
        // Does nothing, we preloaded the entire archive into memory.
    }
} // FileSystems