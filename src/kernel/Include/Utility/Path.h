#ifndef BOREALOS_PATH_H
#define BOREALOS_PATH_H

#include <Definitions.h>

namespace Utility {
    class Path {
    public:
        static size_t GetMaxComponentLength(const char* path, char delimiter);
        static size_t GetComponentCount(const char* path, char delimiter);
        static size_t SplitPath(const char* path, char delimiter, const char** components);
    };
}

#endif //BOREALOS_PATH_H