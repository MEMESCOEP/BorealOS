#include "CPIO.h"

#include <Boot/MBParser.h>
#include <Boot/multiboot.h>
#include <Core/Kernel.h>
#include <Drivers/IO/FS/FileSystem.h>
#include <Utility/StringTools.h>

#include "Core/Memory/Memory.h"

#define CPIO_NEWC_MAGIC "070701"
#define CPIO_NEWC_END_MAGIC "TRAILER!!!"
#define CPIO_HEADER_SIZE 110

Status CPIOLoadArchive(void *baseAddress, size_t size, FileSystem *outFS) {
    (void)outFS;
    size_t offset = 0;
    while (offset < size) {

        uint8_t *p = (uint8_t *)baseAddress + offset;
        if (strncmp((char *)p, CPIO_NEWC_MAGIC, 6) != 0) {
            LOG(LOG_ERROR, "Invalid CPIO magic number, aborting load.\n");
            return STATUS_FAILURE;
        }

        UNUSED size_t inode     = StringHexToSize((char*)(p + 6), 8);
        UNUSED size_t mode      = StringHexToSize((char*)(p + 14), 8);
        UNUSED size_t uid       = StringHexToSize((char*)(p + 22), 8);
        UNUSED size_t gid       = StringHexToSize((char*)(p + 30), 8);
        UNUSED size_t nlink     = StringHexToSize((char*)(p + 38), 8);
        UNUSED size_t mtime     = StringHexToSize((char*)(p + 46), 8);
        UNUSED size_t filesize  = StringHexToSize((char*)(p + 54), 8);
        UNUSED size_t devmajor  = StringHexToSize((char*)(p + 62), 8);
        UNUSED size_t devminor  = StringHexToSize((char*)(p + 70), 8);
        UNUSED size_t rdevmajor = StringHexToSize((char*)(p + 78), 8);
        UNUSED size_t rdevminor = StringHexToSize((char*)(p + 86), 8);
        UNUSED size_t namesize  = StringHexToSize((char*)(p + 94), 8);
        UNUSED size_t check     = StringHexToSize((char*)(p + 102),8);

        char *name = (char *)(p + 110);
        void *data = (void *)ALIGN_UP((uintptr_t)p + CPIO_HEADER_SIZE + namesize, 4);

        if (strcmp(name, CPIO_NEWC_END_MAGIC) == 0) {
            break;
        }

        // Decide file type based on mode
        if ((mode & 0xF000) == 0x4000) { // directory
            if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0) {
                offset += ALIGN_UP(CPIO_HEADER_SIZE + namesize + ALIGN_UP(filesize, 4), 4);
                continue; // skip . and ..
            }
            outFS->CreateDirectory(outFS, name);
        } else if ((mode & 0xF000) == 0x8000) { // regular file
            outFS->CreateFile(outFS, name);

            FSFileHandle* handle = nullptr;
            outFS->Open(outFS, name, &handle);
            size_t bytesWritten = 0;
            outFS->Write(outFS, handle, data, filesize, &bytesWritten);
            outFS->Close(outFS, handle);
        } else if ((mode & 0xF000) == 0xA000) { // symlink
            // We dont support symlinks, warn
            LOGF(LOG_WARNING, "CPIO symlink '%s' found, but symlinks are not supported. Skipping.\n", name);
        }

        offset += ALIGN_UP(CPIO_HEADER_SIZE + namesize + ALIGN_UP(filesize, 4), 4);
    }
    return STATUS_SUCCESS;
}

Status CPIOFromModule(uint32_t MB2InfoPtr, HeapAllocatorState *heap, void **outBase, size_t *outSize) {
    struct multiboot_tag_module* initrdTag = (struct multiboot_tag_module*)MBGetTag(MB2InfoPtr, MULTIBOOT_TAG_TYPE_MODULE);
    if (initrdTag) {
        if (strncmp(initrdTag->cmdline, "initrd", 6) != 0) {
            LOGF(LOG_WARNING, "No initrd module found, only found: %s\n", initrdTag->cmdline);
            return STATUS_FAILURE;
        }
    } else {
        LOG(LOG_WARNING, "No initrd module found.\n");
        return STATUS_FAILURE;
    }

    size_t moduleSize = initrdTag->mod_end - initrdTag->mod_start;
    *outBase = HeapAllocate(heap, moduleSize, 8); // 4-byte alignment
    if (!*outBase) {
        return STATUS_OUT_OF_MEMORY;
    }
    *outSize = moduleSize;
    memcpy(*outBase, (void*)(uintptr_t)initrdTag->mod_start, moduleSize);

    return STATUS_SUCCESS;
}
