#ifndef BOREALOS_LS_H
#define BOREALOS_LS_H

// A simple ls like utility which recursively lists files and directories at a given path.
// Eventually when we have user programs this will be moved there, but for now it's just a kernel utility.
#include <Definitions.h>
#include <Drivers/IO/FS/VFS.h>

void LSDirectory(VFSState* vfs, const char* path, size_t indentLevel, char* parentPath);

#endif //BOREALOS_LS_H