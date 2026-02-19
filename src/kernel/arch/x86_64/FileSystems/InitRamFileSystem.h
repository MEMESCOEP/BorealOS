#ifndef BOREALOS_INITRAMFILESYSTEM_H
#define BOREALOS_INITRAMFILESYSTEM_H

#include "FileSystem.h"
#include "Boot/c_limine.h"

namespace FileSystems {
    class InitRamFileSystem : public FileSystem {
    public:
        explicit InitRamFileSystem(limine_file* cpioArchive, Allocator *allocator);

        [[nodiscard]] Capabilities GetCapabilities() const override;
        [[nodiscard]] File* Open(const char *path) override;
        size_t Read(File *file, void *buffer, size_t size) override;
        size_t Write(File *file, const void *buffer, size_t size) override;
        bool GetFileInfo(File *file, FileInfo *info) override;
        bool GetDirectoryInfo(File *file, DirectoryInfo *info) override;
        void FreeDirectoryInfo(DirectoryInfo *info) override;
        void Close(File *file) override;

    private:
        static constexpr char CpioNewcMagic[] = "070701";
        static constexpr char CpioNewcTrailer[] = "TRAILER!!!";
        static constexpr size_t CpioNewcHeaderSize = 110;

        limine_file* _cpioArchive;
        File** _files; // For now, we just preload the entire archive into memory
        size_t _fileCount;
    };
} // FileSystems

#endif //BOREALOS_INITRAMFILESYSTEM_H