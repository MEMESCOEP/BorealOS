#ifndef BOREALOS_CAT_H
#define BOREALOS_CAT_H

// Simple cat like utility to print file contents to the console
// Eventually when we have user programs this will be moved there, but for now it's just a kernel utility.
#include <Definitions.h>
#include <Drivers/IO/FS/VFS.h>
Status CatFile(VFSState* vfs, const char* path);

#endif //BOREALOS_CAT_H