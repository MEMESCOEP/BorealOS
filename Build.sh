#!/bin/sh
make clean
make -B

BUILD_EXITCODE=$?

if [ $BUILD_EXITCODE -ne 0 ]; then
      echo "Build failed, exiting with status $BUILD_EXITCODE..."
      exit $BUILD_EXITCODE
fi

# Create a directory which will be our ISO root and copy the relevant files into it.
mkdir -p ISORoot
mkdir -p ISORoot/UI
mkdir -p ISORoot/Boot/Limine
cp -v bin/BorealOS ISORoot/Boot/
cp -v BootloaderWallpaper.png ISORoot/UI/
cp -v Limine.conf limine/limine-bios.sys limine/limine-bios-cd.bin \
      limine/limine-uefi-cd.bin ISORoot/Boot/Limine/

# Create the EFI boot tree and copy Limine's EFI executables over.
mkdir -p ISORoot/EFI/BOOT
cp -v limine/BOOTX64.EFI ISORoot/EFI/BOOT/
cp -v limine/BOOTIA32.EFI ISORoot/EFI/BOOT/

# Create the bootable ISO.
xorriso -as mkisofs -R -r -J -b Boot/Limine/limine-bios-cd.bin \
        -no-emul-boot -boot-load-size 4 -boot-info-table -hfsplus \
        -apm-block-size 2048 --efi-boot Boot/Limine/limine-uefi-cd.bin \
        -efi-boot-part --efi-boot-image --protective-msdos-label \
        ISORoot -o BorealOS.iso

# Install Limine stage 1 and 2 for legacy BIOS boot.
./limine/limine bios-install BorealOS.iso
