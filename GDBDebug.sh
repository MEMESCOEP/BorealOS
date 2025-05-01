#!/bin/sh
qemu-system-x86_64 -m 32M -cdrom BorealOS.iso -s -S &
gdb -ex "target remote :1234" -ex "file bin/BorealOS"
