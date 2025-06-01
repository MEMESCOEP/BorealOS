#!/bin/sh
# -cpu <model>,-sse2,-sse disables SSE & SSE2
# -no-reboot forces qemu to exit on a reboot or triple fault; this makes debugging a bit easier
if [ -n "$1" ] && [ "$1" = "--UEFI" ]; then
    # Emulate using UEFI
    UEFI_ARGS="-bios /usr/share/ovmf/OVMF.fd"
    QEMU_EXEC="qemu-system-x86_64"
    CPU="qemu64"
else
    # Emulate using BIOS
    QEMU_EXEC="qemu-system-i386"
    CPU="qemu32"
fi

# Start QEMU
$QEMU_EXEC -no-shutdown -no-reboot -serial file:Logs/Serial.log -cpu $CPU,vendor=QEMU_VirtSys -smp 2 -d int,cpu_reset -m 128M -hda HDD.img -cdrom BorealOS.iso -audiodev pa,id=snd0 -machine pcspk-audiodev=snd0 $UEFI_ARGS