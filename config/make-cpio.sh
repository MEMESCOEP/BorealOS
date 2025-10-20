#!/bin/bash

# 1: CD into the first argument directory
# 2: Create cpio archive and export it to the second argument
cd "$1" || exit 1
find . | cpio -o --format=newc > "$2"
cd .. || exit 1
echo "Created cpio archive at $2"