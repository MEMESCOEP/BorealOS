CC := i686-elf-gcc
AS := nasm
LD := i686-elf-ld

CFLAGS  := -std=gnu11 -Wall -Wextra -ffreestanding -g -O0 -MMD -MP -IInclude
ASFLAGS := -f elf32
LDFLAGS := -T config/link.ld -m elf_i386 -nostdlib -O0 -g
DFLAGS  := #-DTHIN_FONT

KERNEL := build/boot/kernel.elf
ISO    := BorealOS.iso

CSRC := $(shell find src -name "*.c")
ASRC := $(shell find src -name "*.asm")
OBJ  := $(CSRC:.c=.o) $(ASRC:.asm=.o)
CDEP := $(CSRC:.c=.d)

all: $(ISO)

$(ISO): $(KERNEL)
	grub-mkrescue -o $(ISO) build/

$(KERNEL): $(OBJ)
	@mkdir -p build/boot/grub/images
	@mkdir -p build/boot/grub/fonts
	@cp config/grub.cfg build/boot/grub/
	@cp config/Images/*.* build/boot/grub/images/
	@cp config/Fonts/*.pf2 build/boot/grub/fonts/
	@cp config/EFI/shellx64.efi build/
	$(LD) $(LDFLAGS) -o $@ $(OBJ)

-include $(CDEP)
%.o: %.c
	$(CC) $(CFLAGS) $(DFLAGS) -c $< -o $@

%.o: %.asm
	$(AS) $(ASFLAGS) $< -o $@

clean:
	rm -rf build iso logs
	rm -f $(OBJ) $(CDEP)