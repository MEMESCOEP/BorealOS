cmake_minimum_required(VERSION 3.31)
project(BorealOS)

if(NOT ARCH)
    set(ARCH "x86_64")
    message(STATUS "ARCH not specified. Defaulting to x86_64.")
else()
    message(STATUS "Building for architecture: ${ARCH}")
endif()

# Initialize compiler and linker options variables
if(ARCH STREQUAL "x86_64")
    set(CXX_COMPILER x86_64-elf-g++)
    set(ASM_COMPILER x86_64-elf-gcc)
    set(ARCH_DIR "x86_64")

    set(COMPILE_FLAGS
            -mcmodel=large
            -mno-red-zone
            -m64
            -mabi=sysv
    )

    set(LINKER_OPTIONS
            -mcmodel=large
            -mno-red-zone
            -m64
            -Wl,--hash-style=sysv
            -mabi=sysv
    )
else()
    message(FATAL_ERROR "Unsupported architecture: ${ARCH}. Supported architectures is currently only x86_64.")
endif()

# Some common compile options:
set(KERNEL_COMPILE_FLAGS_COMMON
        -ffreestanding
        -fno-asynchronous-unwind-tables
        -Wall -Wextra -Werror -Wpedantic
        -Wno-unused-parameter
        -Wno-unused-variable
        -Wno-unused-function
        ${COMPILE_FLAGS}
)

set(KERNEL_COMPILE_FLAGS_CXX
        ${KERNEL_COMPILE_FLAGS_COMMON}
        -fno-exceptions
        -fno-rtti
        -fno-common
        -Wpedantic
        -Wno-vla
        -Wno-zero-length-array
        -Wno-gnu-flexible-array-member
        -fno-use-cxa-atexit
)

set(KERNEL_COMPILE_FLAGS_C
        ${KERNEL_COMPILE_FLAGS_COMMON}
        -fno-exceptions
        -Wno-vla
        -Wno-zero-length-array
)

set(CMAKE_CXX_STANDARD_LIBRARIES "")
set(CMAKE_CXX_IMPLICIT_LINK_LIBRARIES "")
set(CMAKE_CXX_IMPLICIT_LINK_DIRECTORIES "")

set(CMAKE_C_STANDARD_LIBRARIES "")
set(CMAKE_C_IMPLICIT_LINK_LIBRARIES "")
set(CMAKE_C_IMPLICIT_LINK_DIRECTORIES "")
# End of common compile options

# Add the kernel project
add_subdirectory(src/kernel)

set(MODULE_OUTPUT_DIR ${CMAKE_SOURCE_DIR}/ramfs/modules)
add_subdirectory(src/modules)

# Add a build target called Kernel that depends on the kernel.elf target and installs it into ./src/config/boot/kernel.elf
add_custom_target(CopyKernel ALL
        DEPENDS kernel.elf
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:kernel.elf> ${CMAKE_SOURCE_DIR}/src/config/boot/kernel.elf
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)

add_custom_command(TARGET CopyKernel POST_BUILD
        COMMAND sh -c "nm -f posix ${CMAKE_SOURCE_DIR}/src/config/boot/kernel.elf | awk '{print \$1 \" \" \$3}' | sort -k 1,1 > ${CMAKE_SOURCE_DIR}/ramfs/kernel.sym"
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        VERBATIM
)

# Add a target called "Kernel" which builds kernel.elf and the target Modules, but make sure kernel.elf is built BEFROE Modules, because Modules depends on kernel.elf
add_custom_target(Kernel ALL
        DEPENDS CopyKernel Modules
)