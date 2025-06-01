#!/bin/sh
qemu-system-i386 -m 32M -cdrom BorealOS.iso -s -S &
gdb-multiarch -ex "file build/boot/kernel.elf" -ex "set architecture i386" -ex "target remote :1234" 
