#!/bin/bash

# Check if the debug argument is provided, if so add the -s -S flags
FLAGS=""
if [ "$1" == "debug" ]; then
    echo "Running in debug mode..."
    FLAGS="-s -S"
else
    echo "Running in normal mode..."
fi

mkdir -p logs

if [ "$1" == "--Bochs" ]; then
    echo "[== IMPORTANT NOTE ==]"
    echo "Bochs may start its debugger ('<bochs:1>' prompt), simply press C+ENTER to continue."
    bochs -f BochsCFG.bochs
elif [ "$1" == "--QEMU" ]; then
    qemu-system-i386 $FLAGS -no-shutdown -no-reboot -serial file:logs/serial.log -cpu qemu32,vendor=QEMU_VirtSys -smp 2 -d int,cpu_reset -m 128M -hda HDD.img -cdrom ./cmake-build-debug/BorealOS.iso -audiodev pa,id=snd0 -machine pcspk-audiodev=snd0 -display gtk
fi