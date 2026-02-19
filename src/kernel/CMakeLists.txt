cmake_minimum_required(VERSION 3.31)
project(kernel LANGUAGES CXX ASM)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_EXTENSIONS ON)

# check for the variable called "ARCH" and set the compilers accordingly
if(ARCH STREQUAL "x86_64")
    set(CXX_COMPILER x86_64-elf-g++)
    set(ASM_COMPILER x86_64-elf-gcc)
    set(ARCH_DIR "x86_64")
    set(COMPILE_FLAGS
        -m64
        -mcmodel=large
        -mno-red-zone
    )

    set(LINKER_OPTIONS
        -mcmodel=large
        -mno-red-zone
        -m64
    )
else()
    message(FATAL_ERROR "Unsupported architecture: ${ARCH}. Supported architectures is currently only x86_64.")
endif()

set(CMAKE_CXX_COMPILER ${CXX_COMPILER})
set(CMAKE_ASM_COMPILER ${ASM_COMPILER})

set(CMAKE_CXX_STANDARD_LIBRARIES "")
set(CMAKE_CXX_IMPLICIT_LINK_LIBRARIES "")
set(CMAKE_CXX_IMPLICIT_LINK_DIRECTORIES "")

set(CMAKE_C_STANDARD_LIBRARIES "")
set(CMAKE_C_IMPLICIT_LINK_LIBRARIES "")
set(CMAKE_C_IMPLICIT_LINK_DIRECTORIES "")

set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

file(GLOB_RECURSE SOURCES "arch/${ARCH_DIR}/*.cpp" "arch/${ARCH_DIR}/*.S")
file(GLOB_RECURSE HEADERS "Include/*.h" "arch/${ARCH_DIR}/*.h")
file(GLOB_RECURSE COMMON_SOURCES "arch/common/*.cpp" "arch/common/*.S") # the common headers are located in Include/Common

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

include(FetchContent)

FetchContent_Declare(
        flanterm
        GIT_REPOSITORY https://codeberg.org/Mintsuki/Flanterm
        GIT_TAG        trunk
)

FetchContent_MakeAvailable(flanterm)

file(GLOB_RECURSE FLANTERM_SOURCES ${flanterm_SOURCE_DIR}/src/*.c)

add_executable(kernel.elf ${SOURCES} ${COMMON_SOURCES} ${HEADERS} ${FLANTERM_SOURCES})
target_include_directories(kernel.elf PRIVATE Include)
target_include_directories(kernel.elf PRIVATE ${CMAKE_SOURCE_DIR}/deps/Limine)
target_include_directories(kernel.elf PRIVATE ${flanterm_SOURCE_DIR}/src)

target_compile_options(kernel.elf PRIVATE
        $<$<COMPILE_LANGUAGE:CXX>:${KERNEL_COMPILE_FLAGS_CXX}>
        $<$<COMPILE_LANGUAGE:C>:${KERNEL_COMPILE_FLAGS_C}>
)

target_link_options(kernel.elf PRIVATE
        -g
        -T ${CMAKE_SOURCE_DIR}/src/config/linker.ld
        -nostdlib
        -nostartfiles
        ${LINKER_OPTIONS}
)