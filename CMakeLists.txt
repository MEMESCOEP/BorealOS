cmake_minimum_required(VERSION 3.31)
project(BorealOS LANGUAGES C ASM)

set(CMAKE_C_STANDARD 23)
set(CMAKE_C_COMPILER i686-elf-gcc)

file(GLOB_RECURSE C_SOURCES CONFIGURE_DEPENDS "src/*.c" "src/*.h")
file(GLOB_RECURSE ASM_SOURCES CONFIGURE_DEPENDS "src/*.S")

include_directories(${CMAKE_SOURCE_DIR}/src)

set_source_files_properties(${ASM_SOURCES} PROPERTIES
        LANGUAGE ASM
)

set(CMAKE_ASM_COMPILER ${CMAKE_C_COMPILER})

add_executable(BorealOS.elf ${C_SOURCES} ${ASM_SOURCES})

# Compiler flags
target_compile_options(BorealOS.elf PRIVATE
        -g
        -ffreestanding
        -fno-asynchronous-unwind-tables
        -Wall -Wextra -Werror
        -Wpedantic
)

# Linker options
target_link_options(BorealOS.elf PRIVATE
        -g
        -T ${CMAKE_SOURCE_DIR}/config/link.ld

        -nostdlib
        -nostartfiles
)

# Get flanterm automatically
include(FetchContent)
FetchContent_Declare(
        flanterm
        GIT_REPOSITORY https://codeberg.org/Mintsuki/Flanterm.git
        GIT_TAG v2.1.0
)

FetchContent_MakeAvailable(flanterm)

file(GLOB_RECURSE FLANTERM_SOURCES ${flanterm_SOURCE_DIR}/src/*.c)

target_sources(BorealOS.elf PRIVATE ${FLANTERM_SOURCES})
target_include_directories(BorealOS.elf PRIVATE ${flanterm_SOURCE_DIR}/src)

FetchContent_Declare(
        lai
        GIT_REPOSITORY https://github.com/managarm/lai.git
        GIT_TAG master
)

FetchContent_MakeAvailable(lai)

file(GLOB_RECURSE LAI_SOURCES ${lai_SOURCE_DIR}/*.c)

# Disable wpedantic for uacpi sources
set_source_files_properties(${LAI_SOURCES} PROPERTIES COMPILE_FLAGS "-Wno-pedantic -Wno-error=format -w")

target_sources(BorealOS.elf PRIVATE ${LAI_SOURCES})
target_include_directories(BorealOS.elf PRIVATE ${lai_SOURCE_DIR}/include)

## Enable the UACPI_PHYS_ADDR_IS_32BITS, UACPI_SIZED_FREES, UACPI_NATIVE_ALLOC_ZEROED macros
#target_compile_definitions(BorealOS.elf PRIVATE UACPI_PHYS_ADDR_IS_32BITS UACPI_SIZED_FREES UACPI_NATIVE_ALLOC_ZEROED)
## Set UACPI_DEFAULT_LOG_LEVEL to UACPI_LOG_DEBUG
#target_compile_definitions(BorealOS.elf PRIVATE UACPI_DEFAULT_LOG_LEVEL=UACPI_LOG_DEBUG)

# Now we need to create the actual ISO.
# First in the build directory, create a directory called "iso" and create the following directory structure:
# boot/grub/fonts
# Then copy ./config/grub.cfg to boot/grub/grub.cfg
# Then copy the compiled kernel to boot/BorealOS.elf
# Then copy the font file from ./config/fonts/Terminus-18.pf2 to boot/grub/fonts/Terminus-18.pf2
# And then finally, we can copy the shellx64.efi in config/EFI to the root of the ISO.
# Then using grub-mkrescue, we can create the ISO file.

add_custom_target(iso
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/iso/boot/grub/fonts
        COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/config/grub.cfg ${CMAKE_BINARY_DIR}/iso/boot/grub/grub.cfg
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:BorealOS.elf> ${CMAKE_BINARY_DIR}/iso/boot/BorealOS.elf
        COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/config/fonts/Terminus-18.pf2 ${CMAKE_BINARY_DIR}/iso/boot/grub/fonts/Terminus-18.pf2
        COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/config/images/wallpaper.png ${CMAKE_BINARY_DIR}/iso/boot/grub/images/wallpaper.png
        COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/config/EFI/shellx64.efi ${CMAKE_BINARY_DIR}/iso/shellx64.efi
        COMMAND grub-mkrescue -o BorealOS.iso ${CMAKE_BINARY_DIR}/iso
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)

# Add a custom target to build the ISO
add_custom_target(build-iso
        DEPENDS BorealOS.elf iso
        COMMENT "Building ISO image..."
)

# Using the
add_custom_target(create-hdd
        COMMAND qemu-img create -f raw HDD.img 512M
)