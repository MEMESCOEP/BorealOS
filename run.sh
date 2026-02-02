#!/bin/bash

./build.sh || exit 1

# Run using QEMU
if [ ! -d "logs" ]; then
    mkdir logs
fi

if [ ! -f "HDD.img" ]; then
    src/scripts/create_hdd.sh "$PWD"/HDD.img 2G || exit 1
fi

# Check if theres a --debug flag
DEBUG_FLAGS=""
if [ "$1" == "--debug" ]; then
    DEBUG_FLAGS="-s -S"
fi

qemu-system-x86_64 $DEBUG_FLAGS -no-shutdown -no-reboot -serial file:logs/qemu.log -cpu qemu64,vendor=QEMU_VirtSys -smp 2 -d int,cpu_reset -m 128M -hda HDD.img -cdrom ./bin/BorealOS.iso -bios /usr/share/ovmf/OVMF.fd -netdev user,id=net0,hostfwd=tcp::2222-:22 -device e1000,netdev=net0 -monitor stdio -boot order=d || exit 1