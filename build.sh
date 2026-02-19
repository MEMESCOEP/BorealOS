#!/bin/bash

# Currently only builds for X86_64 architecture

set -euo pipefail
# Check if ./deps/Limine exists
if [ ! -d "./deps/Limine" ]; then
    echo "Limine not found! Make sure to init submodules."
    exit 1
fi

if [ ! -d "./deps/Limine/limine" ]; then
    echo "Limine binary not found! Building Limine..."
    cd deps/Limine
    make
    cd ../../
fi

# Build the cmake target 'Kernel'
if [ ! -d "build" ]; then
    cmake -B build -D CMAKE_BUILD_TYPE=Debug .
fi

cmake --build build --target Kernel

# Create the ISO
# Copy over ./src/config verbatim to ./iso
mkdir -p iso
cp -r src/config/* iso/

mkdir -p iso/boot/limine
cp deps/Limine/limine-uefi-cd.bin iso/boot/limine/
cp deps/Limine/limine-bios.sys iso/boot/limine/
cp deps/Limine/limine-bios-cd.bin iso/boot/limine/

mkdir -p iso/boot/rootfs
. ./src/scripts/make_cpio.sh ramfs iso/boot/rootfs/initramfs.cpio

mkdir -p iso/EFI/BOOT
cp deps/Limine/BOOTX64.EFI iso/EFI/BOOT/

if [ ! -d "./bin" ]; then
    mkdir ./bin
fi

xorriso -as mkisofs -R -r -J -b boot/limine/limine-bios-cd.bin \
        -no-emul-boot -boot-load-size 4 -boot-info-table -hfsplus \
        -apm-block-size 2048 --efi-boot boot/limine/limine-uefi-cd.bin \
        -efi-boot-part --efi-boot-image --protective-msdos-label \
        iso -o ./bin/BorealOS.iso iso

sync

rm -rf iso

./deps/Limine/limine bios-install ./bin/BorealOS.iso