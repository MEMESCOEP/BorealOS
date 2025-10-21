#ifndef BOREALOS_PATHUTILS_H
#define BOREALOS_PATHUTILS_H

#include <Definitions.h>

// Basic path utilities for manipulating file paths.
// This does NOT resolve . or .. components, it just does basic string manipulations.

/// Modify the given path in-place to remove redundant slashes and ensure it starts with a single slash.
/// Returns the normalized path (same as input pointer).
/// Example: "//foo///bar/" becomes "/foo/bar/"
char* PathNormalize(char* path);

/// Get the parent directory of the given path.
/// The returned string is heap allocated on the kernel heap, and must be freed by the caller.
/// The outSize parameter will be set to the length of the returned string (excluding null terminator).
/// Example: "/foo/bar/baz.txt" returns "/foo/bar"
char* PathGetParent(const char* path, size_t *outSize);

/// Get the filename component of the given path.
/// Example: "/foo/bar/baz.txt" returns "baz.txt"
char* PathGetFilename(const char* path);

#endif //BOREALOS_PATHUTILS_H