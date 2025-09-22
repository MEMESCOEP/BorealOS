#!/bin/bash

FLAGS=""
EMULATOR=""

# Parse arguments
for arg in "$@"; do
    case $arg in
        debug)
            echo "Running in debug mode..."
            FLAGS="-s -S"
            ;;
        --Bochs)
            EMULATOR="bochs"
            ;;
        --QEMU)
            EMULATOR="qemu"
            ;;
    esac
done

mkdir -p logs

case $EMULATOR in
    bochs)
        echo "[== IMPORTANT NOTE ==]"
        echo "Bochs may start its debugger ('<bochs:1>' prompt), simply press C+ENTER to continue."
        bochs -f BochsCFG.bochs
        ;;
    qemu)
        echo "Running in normal mode..."
        qemu-system-i386 $FLAGS -no-shutdown -no-reboot -serial file:logs/serial.log -cpu qemu32,vendor=QEMU_VirtSys -smp 2 -d int,cpu_reset -m 128M -hda HDD.img -cdrom ./cmake-build-debug/BorealOS.iso -audiodev pa,id=snd0 -machine pcspk-audiodev=snd0 -display gtk
        ;;
    *)
        echo "No emulator specified. Use --Bochs or --QEMU."
        exit 1
        ;;
esac