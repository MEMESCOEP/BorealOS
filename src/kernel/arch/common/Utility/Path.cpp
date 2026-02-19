#include <Utility/Path.h>

namespace Utility {
    size_t Path::GetComponentCount(const char *path, char delimiter) {
        size_t count = 0;
        const char* p = path;

        while (*p) {
            while (*p == delimiter) p++; // Skip consecutive delimiters
            if (*p) {
                count++;
                while (*p && *p != delimiter) p++; // Move to the next delimiter
            }
        }

        return count;
    }

    size_t Path::GetMaxComponentLength(const char *path, char delimiter) {
        size_t maxLength = 0;
        size_t currentLength = 0;
        const char* p = path;

        while (*p) {
            if (*p == delimiter) {
                if (currentLength > maxLength) {
                    maxLength = currentLength;
                }
                currentLength = 0; // Reset for the next component
            } else {
                currentLength++;
            }
            p++;
        }

        // Check the last component
        if (currentLength > maxLength) {
            maxLength = currentLength;
        }

        return maxLength;
    }

    size_t Path::SplitPath(const char *path, char delimiter, const char **components) {
        size_t count = 0;
        const char* p = path;

        while (*p) {
            while (*p == delimiter) p++; // Skip consecutive delimiters
            if (*p) {
                components[count++] = p; // Store the start of the component
                while (*p && *p != delimiter) p++; // Move to the next delimiter
                if (*p) {
                    *const_cast<char*>(p) = '\0'; // Null-terminate the component
                    p++; // Move past the delimiter for the next iteration
                }
            }
        }

        return count;
    }
}
