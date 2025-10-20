#include "RamDisk.h"
#include <Boot/MBParser.h>
#include <Boot/multiboot.h>
#include <Core/Kernel.h>
#include <Core/Memory/Memory.h>
#include <Utility/StringTools.h>
#include "Utility/PathUtils.h"

// Ramdisk is an in memory filesystem, so we use a tree structure to represent files and directories.
typedef struct RamDiskNode {
    char* Name;
    bool IsDirectory;
    size_t Size;
    size_t Capacity;
    void* Data; // Pointer to file data in memory
    struct RamDiskNode** Children; // For directories, an array of child nodes
    size_t ChildCount;
    size_t ChildCapacity;
} RamDiskNode;

typedef struct RamDiskFileHandle {
    RamDiskNode* Node;
    size_t Position; // Current position for read/write
} RamDiskFileHandle;

typedef struct RamDiskData {
    HeapAllocatorState *Heap;
    RamDiskNode *Root;
} RamDiskData;

// Some helper functions for traversing.
static RamDiskNode* RamDiskFindNode(RamDiskNode* currentNode, const char* path) {
    if (!currentNode || !path) return nullptr;

    // Root path returns current node itself
    if (strcmp(path, "/") == 0) return currentNode;

    // Skip leading '/' if present
    const char* cleanPath = path[0] == '/' ? path + 1 : path;

    size_t segmentCount = 0;
    char** segments = StringSplit(cleanPath, '/', &segmentCount);
    if (!segments || segmentCount == 0) return nullptr;

    RamDiskNode* node = currentNode;

    for (size_t i = 0; i < segmentCount; i++) {
        const char* seg = segments[i];
        if (!seg || seg[0] == '\0') continue; // skip empty segments

        bool found = false;
        for (size_t j = 0; j < node->ChildCount; j++) {
            RamDiskNode* child = node->Children[j];
            if (strcmp(child->Name, seg) == 0) {
                node = child;
                found = true;
                break;
            }
        }

        if (!found) {
            node = nullptr;
            break;
        }
    }

    // Free the segments
    for (size_t i = 0; i < segmentCount; i++) {
        if (segments[i]) HeapFree(&Kernel.Heap, segments[i]);
    }
    HeapFree(&Kernel.Heap, segments);

    return node;
}

static bool RamDiskAddChild(RamDiskNode* parent, RamDiskNode* child, HeapAllocatorState* heap) {
    if (parent->ChildCount >= parent->ChildCapacity) {
        size_t newCapacity = parent->ChildCapacity == 0 ? 4 : parent->ChildCapacity * 2;
        RamDiskNode** newChildren = (RamDiskNode**)HeapAlloc(heap, sizeof(RamDiskNode*) * newCapacity);
        if (!newChildren) {
            return false;
        }
        for (size_t i = 0; i < parent->ChildCount; i++) {
            newChildren[i] = parent->Children[i];
        }
        HeapFree(heap, parent->Children);
        parent->Children = newChildren;
        parent->ChildCapacity = newCapacity;
    }
    parent->Children[parent->ChildCount++] = child;
    return true;
}

static Status RamDiskOpen(FileSystem* fs, const char* path, FSFileHandle** outHandle) {
    // Find the node corresponding to the path
    RamDiskData* data = (RamDiskData*)fs->Data;
    RamDiskNode* node = RamDiskFindNode(data->Root, path);
    if (!node) {
        return STATUS_NOT_FOUND;
    }

    // Now allocate a new FileHandle for this node
    RamDiskFileHandle* r = HeapAlloc(data->Heap, sizeof(RamDiskFileHandle));
    if (!r) {
        return STATUS_OUT_OF_MEMORY;
    }
    r->Node = node;
    r->Position = 0;
    *outHandle = (FSFileHandle*)r;

    return STATUS_SUCCESS;
}

static Status RamDiskClose(FileSystem* fs, FSFileHandle* handle) {
    // Just free the handle
    if (handle) {
        RamDiskData* data = (RamDiskData*)fs->Data;
        HeapFree(data->Heap, handle);
    }
    return STATUS_SUCCESS;
}

static Status RamDiskRead(UNUSED FileSystem* fs, FSFileHandle* handle, void* buffer, size_t size, size_t* outBytesRead) {
    // Get the node from the handle
    RamDiskFileHandle* fileHandle = (RamDiskFileHandle*)handle;
    RamDiskNode* node = fileHandle->Node;
    if (!node || node->IsDirectory) {
        return STATUS_INVALID_PARAMETER;
    }
    size_t bytesToRead = size;
    if (fileHandle->Position + bytesToRead > node->Size) {
        bytesToRead = node->Size - fileHandle->Position;
    }
    memcpy(buffer, (uint8_t*)node->Data + fileHandle->Position, bytesToRead);
    fileHandle->Position += bytesToRead;
    if (outBytesRead) {
        *outBytesRead = bytesToRead;
    }
    return STATUS_SUCCESS;
}

static Status RamDiskWrite(FileSystem* fs, FSFileHandle* handle, const void* buffer, size_t size, size_t* outBytesWritten) {
    // Get the node from the handle
    RamDiskData* data = (RamDiskData*)fs->Data;

    RamDiskFileHandle* fileHandle = (RamDiskFileHandle*)handle;
    RamDiskNode* node = fileHandle->Node;
    if (!node || node->IsDirectory) {
        return STATUS_INVALID_PARAMETER;
    }
    // Ensure capacity
    size_t requiredSize = fileHandle->Position + size;
    if (requiredSize > node->Capacity) {
        size_t newCapacity = node->Capacity == 0 ? 1024 : node->Capacity;
        while (newCapacity < requiredSize) {
            newCapacity *= 2;
        }
        void* newData = HeapAlloc(data->Heap, newCapacity);
        if (!newData) {
            return STATUS_OUT_OF_MEMORY;
        }
        if (node->Data) {
            memcpy(newData, node->Data, node->Size);
            HeapFree(data->Heap, node->Data);
        }
        node->Data = newData;
        node->Capacity = newCapacity;
    }
    memcpy((uint8_t*)node->Data + fileHandle->Position, buffer, size);
    fileHandle->Position += size;
    if (fileHandle->Position > node->Size) {
        node->Size = fileHandle->Position;
    }
    if (outBytesWritten) {
        *outBytesWritten = size;
    }
    return STATUS_SUCCESS;
}

static Status RamDiskSeek(UNUSED FileSystem* fs, FSFileHandle* handle, size_t position) {
    // Get the node from the handle
    RamDiskFileHandle* fileHandle = (RamDiskFileHandle*)handle;
    RamDiskNode* node = fileHandle->Node;
    if (!node || node->IsDirectory) {
        return STATUS_INVALID_PARAMETER;
    }
    if (position > node->Size) {
        return STATUS_INVALID_PARAMETER;
    }
    fileHandle->Position = position;
    return STATUS_SUCCESS;
}

static Status RamDiskTell(UNUSED FileSystem* fs, FSFileHandle* handle, size_t* outPosition) {
    // Get the node from the handle
    RamDiskFileHandle* fileHandle = (RamDiskFileHandle*)handle;
    RamDiskNode* node = fileHandle->Node;
    if (!node || node->IsDirectory) {
        return STATUS_INVALID_PARAMETER;
    }
    if (outPosition) {
        *outPosition = fileHandle->Position;
    }
    return STATUS_SUCCESS;
}

static Status RamDiskStat(UNUSED FileSystem* fs, FSFileHandle* handle, FileStat* outStat) {
    // Get the node from the handle
    RamDiskFileHandle* fileHandle = (RamDiskFileHandle*)handle;
    RamDiskNode* node = fileHandle->Node;
    if (!node) {
        return STATUS_INVALID_PARAMETER;
    }
    outStat->Size = node->Size;
    outStat->IsDirectory = node->IsDirectory;
    // Permissions, CreatedAt, ModifiedAt can be set to defaults for now
    outStat->Permissions = 0;
    outStat->CreatedAt = 0;
    outStat->ModifiedAt = 0;
    return STATUS_SUCCESS;
}

static Status RamDiskListDir(FileSystem* fs, const char* path, FSListItem** outEntries, size_t* outEntryCount) {
    RamDiskData* data = (RamDiskData*)fs->Data;
    RamDiskNode* node = RamDiskFindNode(data->Root, path);
    if (!node || !node->IsDirectory) {
        return STATUS_NOT_FOUND;
    }

    size_t count = node->ChildCount;
    FSListItem* entries = (FSListItem*)HeapAlloc(data->Heap, sizeof(FSListItem) * count);
    if (!entries) {
        return STATUS_OUT_OF_MEMORY;
    }
    for (size_t i = 0; i < count; i++) {
        entries[i].Name = StringDuplicate(data->Heap, node->Children[i]->Name);
        if (!entries[i].Name) {
            // Free previously allocated names
            for (size_t j = 0; j < i; j++) {
                HeapFree(data->Heap, entries[j].Name);
            }
            HeapFree(data->Heap, entries);
            return STATUS_OUT_OF_MEMORY;
        }
        entries[i].IsDirectory = node->Children[i]->IsDirectory;
    }
    *outEntries = entries;
    *outEntryCount = count;

    return STATUS_SUCCESS;
}

static Status RamDiskCreateFile(FileSystem* fs, const char* path) {
    // First check if the file already exists
    RamDiskData* data = (RamDiskData*)fs->Data;

    RamDiskNode* existingNode = RamDiskFindNode(data->Root, path);
    if (existingNode) {
        return STATUS_ALREADY_EXISTS;
    }

    // Now get the parent directory.
    size_t lastSlash = 0;
    char* parentPath = PathGetParent(path, &lastSlash);
    RamDiskNode* parentNode = RamDiskFindNode(data->Root, parentPath);
    HeapFree(data->Heap, parentPath);
    if (!parentNode || !parentNode->IsDirectory) {
        return STATUS_NOT_FOUND;
    }

    // Create the new file node
    RamDiskNode* newFile = (RamDiskNode*)HeapAlloc(data->Heap, sizeof(RamDiskNode));
    if (!newFile) {
        return STATUS_OUT_OF_MEMORY;
    }
    // Normalize the name to just the last segment
    const char* fileName = path + lastSlash + (lastSlash == 0 ? 0 : 1);
    newFile->Name = StringDuplicate(data->Heap, fileName);
    if (!newFile->Name) {
        HeapFree(data->Heap, newFile);
        return STATUS_OUT_OF_MEMORY;
    }

    newFile->IsDirectory = false;
    newFile->Size = 0;
    newFile->Capacity = 0;
    newFile->Data = nullptr;
    newFile->Children = nullptr;
    newFile->ChildCount = 0;
    newFile->ChildCapacity = 0;

    // Add to parent's children
    if (!RamDiskAddChild(parentNode, newFile, data->Heap)) {
        HeapFree(data->Heap, newFile->Name);
        HeapFree(data->Heap, newFile);
        return STATUS_OUT_OF_MEMORY;
    }

    return STATUS_SUCCESS;
}

static Status RamDiskCreateDirectory(FileSystem* fs, const char* path) {
    // First check if the directory already exists
    RamDiskData* data = (RamDiskData*)fs->Data;

    RamDiskNode* existingNode = RamDiskFindNode(data->Root, path);
    if (existingNode) {
        return STATUS_ALREADY_EXISTS;
    }

    // Now get the parent directory.
    size_t lastSlash = 0;
    char* parentPath = PathGetParent(path, &lastSlash);

    RamDiskNode* parentNode = RamDiskFindNode(data->Root, parentPath);
    HeapFree(data->Heap, parentPath);
    if (!parentNode || !parentNode->IsDirectory) {
        return STATUS_NOT_FOUND;
    }

    // Create the new directory node
    RamDiskNode* newDir = (RamDiskNode*)HeapAlloc(data->Heap, sizeof(RamDiskNode));
    if (!newDir) {
        return STATUS_OUT_OF_MEMORY;
    }

    // Normalize the name to just the last segment
    const char* dirName = path + lastSlash + (lastSlash == 0 ? 0 : 1);
    newDir->Name = StringDuplicate(data->Heap, dirName);
    if (!newDir->Name) {
        HeapFree(data->Heap, newDir);
        return STATUS_OUT_OF_MEMORY;
    }

    newDir->IsDirectory = true;
    newDir->Size = 0;
    newDir->Capacity = 0;
    newDir->Data = nullptr;
    newDir->Children = nullptr;
    newDir->ChildCount = 0;
    newDir->ChildCapacity = 0;

    // Add to parent's children
    if (!RamDiskAddChild(parentNode, newDir, data->Heap)) {
        HeapFree(data->Heap, newDir->Name);
        HeapFree(data->Heap, newDir);
        return STATUS_OUT_OF_MEMORY;
    }

    return STATUS_SUCCESS;
}

static Status RamDiskDelete(FileSystem* fs, const char* path) {
    // Find the node corresponding to the path
    RamDiskData* data = (RamDiskData*)fs->Data;

    RamDiskNode* node = RamDiskFindNode(data->Root, path);
    if (!node) {
        return STATUS_NOT_FOUND;
    }

    // If it is a directory, and not empty, return error
    if (node->IsDirectory && node->ChildCount > 0) {
        return STATUS_INVALID_PARAMETER;
    }

    // Find the parent node
    size_t lastSlash = 0;
    char* parentPath = PathGetParent(path, &lastSlash);
    RamDiskNode* parentNode = RamDiskFindNode(data->Root, parentPath);
    HeapFree(data->Heap, parentPath);
    if (!parentNode) {
        return STATUS_NOT_FOUND;
    }

    // Remove from parent's children
    size_t index = (size_t)-1;
    for (size_t i = 0; i < parentNode->ChildCount; i++) {
        if (parentNode->Children[i] == node) {
            index = i;
            break;
        }
    }

    if (index == (size_t)-1) {
        return STATUS_NOT_FOUND;
    }

    for (size_t i = index; i < parentNode->ChildCount - 1; i++) {
        parentNode->Children[i] = parentNode->Children[i + 1];
    }
    parentNode->ChildCount--;
    // Free the node
    HeapFree(data->Heap, node->Name);
    if (node->Data) {
        HeapFree(data->Heap, node->Data);
    }
    HeapFree(data->Heap, node->Children);
    HeapFree(data->Heap, node);

    return STATUS_UNSUPPORTED;
}

static Status RamDiskRename(FileSystem* fs, const char* oldPath, const char* newPath) {
    // Find the node corresponding to oldPath
    RamDiskData* data = (RamDiskData*)fs->Data;

    RamDiskNode* node = RamDiskFindNode(data->Root, oldPath);
    if (!node) {
        return STATUS_NOT_FOUND;
    }
    // Change the name to the new name (last segment of newPath)
    size_t lastSlash = 0;
    PathGetParent(newPath, &lastSlash);
    const char* newName = newPath + lastSlash + (lastSlash == 0 ? 0 : 1);
    char* newNameDup = StringDuplicate(data->Heap, newName);
    if (!newNameDup) {
        return STATUS_OUT_OF_MEMORY;
    }
    HeapFree(data->Heap, node->Name);
    node->Name = newNameDup;
    return STATUS_SUCCESS;
}

Status RamDiskInit(HeapAllocatorState *Heap, FileSystem** outFS) {
    FileSystem* fs = (FileSystem*)HeapAlloc(&Kernel.Heap, sizeof(FileSystem));
    if (!fs) {
        return STATUS_FAILURE;
    }

    RamDiskData* data = (RamDiskData*)HeapAlloc(&Kernel.Heap, sizeof(RamDiskData));
    if (!data) {
        HeapFree(&Kernel.Heap, fs);
        return STATUS_FAILURE;
    }
    data->Heap = Heap;
    data->Root = HeapAlloc(Heap, sizeof(RamDiskNode));
    if (!data->Root) {
        HeapFree(&Kernel.Heap, data);
        HeapFree(&Kernel.Heap, fs);
        return STATUS_FAILURE;
    }
    data->Root->Name = StringDuplicate(Heap, "/");
    data->Root->IsDirectory = true;
    data->Root->Size = 0;
    data->Root->Capacity = 0;
    data->Root->Data = nullptr;
    data->Root->Children = nullptr;
    data->Root->ChildCount = 0;
    data->Root->ChildCapacity = 0;

    fs->Name = RAMDISK_FILESYSTEM_NAME;
    fs->Data = data;
    fs->Device = nullptr; // No underlying block device

    fs->Open = RamDiskOpen;
    fs->Close = RamDiskClose;
    fs->Read = RamDiskRead;
    fs->Write = RamDiskWrite;
    fs->Seek = RamDiskSeek;
    fs->Tell = RamDiskTell;
    fs->Stat = RamDiskStat;
    fs->ListDir = RamDiskListDir;
    fs->CreateFile = RamDiskCreateFile;
    fs->CreateDirectory = RamDiskCreateDirectory;
    fs->Delete = RamDiskDelete;
    fs->Rename = RamDiskRename;

    *outFS = fs;
    return STATUS_SUCCESS;
}