#!/bin/sh
# -cpu xxx,-sse2,-sse disables SSE & SSE2
if [ -n "$1" ] && [ "$1" = "--UEFI" ]; then
    # Emulate using UEFI
    qemu-system-x86_64 -cpu qemu64,vendor=QEMU_VirtSys -smp 2 -d int,cpu_reset -m 128M -hda HDD.img -cdrom BorealOS.iso -bios /usr/share/ovmf/OVMF.fd -no-reboot -audiodev pa,id=snd0 -machine pcspk-audiodev=snd0
else
    # Emulate using BIOS
    qemu-system-x86_64 -cpu qemu64,vendor=QEMU_VirtSys -smp 2 -d int,cpu_reset -m 128M -hda HDD.img -cdrom BorealOS.iso -no-reboot -audiodev pa,id=snd0 -machine pcspk-audiodev=snd0
fi
