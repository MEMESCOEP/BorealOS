# Todo

This is the tracker for features & bugs.

## Done:

- Basic log, panic & printf support using custom function handlers
- Serial support
- Physical memory allocator
- IDT & PIC support
- Paging support
- Framebuffer support
- Text console
- Virtual memory allocator
- Basic ATA driver (read/write)

## Doing:

- USB (OHCI, UHCI) (m & andrew)
- FAT32 (m & andrew)

## To Do:

* Highest priority:
  - Drivers
    - USB (HID, storage, etc) (m & andrew)
    - Storage Drivers
      - AHCI (SATA) (andrew)
      - NVMe (m)
    - ACPI (m & andrew)
    - NIC
      - AMD PCNet III (andrew)
      - Intel PRO/1000 (andrew)

  - Filesystem support
    - FAT32 (m & andrew)
    - ISO9660 (andrew)
    - EXT2 (m)
    - ExFAT (m)
    - UDF
  
  - Syscalls (m & andrew)

* Lower priority:
  - Drivers
    - PS/2 (Controller/KB/Mouse) (andrew)
    - Storage Drivers
      - Floppy (andrew, maybe?)
    - Sound
      - AC97 (andrew)
      - Soundblaster 16
     
    - Graphics Acceleration
      - Generic AMD/Intel/Nvidia drivers (m & andrew)

  - Multitasking
  - User mode
  - Networking
  - Sound
  - More features
