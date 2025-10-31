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
        qemu-system-i386 \
            $FLAGS \
            -no-shutdown -no-reboot \
            -serial file:logs/serial.log \
            -cpu Opteron_G1-v1,vendor=QEMU_VirtSys \
            -smp 2 \
            -m 64M \
            -d guest_errors,cpu_reset \
            -hda HDD.img \
            -cdrom ./cmake-build-debug/BorealOS.iso \
            -audiodev pa,id=snd0 \
            -machine q35,pcspk-audiodev=snd0 \
            -display gtk \
            -device virtio-net-pci,netdev=net0 \
            -netdev user,id=net0 \
            -boot order=d
        ;;
    *)
        echo "No emulator specified. Use --Bochs or --QEMU."
        exit 1
        ;;
esac