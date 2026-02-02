cmake_minimum_required(VERSION 3.31)
project(kernel LANGUAGES CXX ASM)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_EXTENSIONS ON)

# check for the variable called "ARCH" and set the compilers accordingly
if(ARCH STREQUAL "x86_64")
    set(CXX_COMPILER x86_64-elf-g++)
    set(ASM_COMPILER x86_64-elf-gcc)
    set(ARCH_DIR "x86_64")
    set(COMPILE_FLAGS "-m64 -mcmodel=large")
    set(LINKER_OPTIONS "-mcmodel=large")
else()
    message(FATAL_ERROR "Unsupported architecture: ${ARCH}. Supported architectures is currently only x86_64.")
endif()

set(CMAKE_CXX_COMPILER ${CXX_COMPILER})
set(CMAKE_ASM_COMPILER ${ASM_COMPILER})

set(CMAKE_CXX_STANDARD_LIBRARIES "")
set(CMAKE_CXX_IMPLICIT_LINK_LIBRARIES "")
set(CMAKE_CXX_IMPLICIT_LINK_DIRECTORIES "")
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

file(GLOB_RECURSE SOURCES "arch/${ARCH_DIR}/*.cpp" "arch/${ARCH_DIR}/*.S")
file(GLOB_RECURSE HEADERS "Include/*.h" "arch/${ARCH_DIR}/*.h")
file(GLOB_RECURSE COMMON_SOURCES "arch/common/*.cpp" "arch/common/*.S") # the common headers are located in Include/Common

set_source_files_properties(${SOURCES} PROPERTIES COMPILE_FLAGS "-g ${COMPILE_FLAGS} -ffreestanding -fno-exceptions -fno-rtti -fno-asynchronous-unwind-tables -fno-common -Wall -Wextra -Werror -Wpedantic -Wno-unused-parameter -Wno-unused-variable -Wno-unused-function -Wno-vla -Wno-zero-length-array -Wno-gnu-flexible-array-member")

add_executable(kernel.elf ${SOURCES} ${COMMON_SOURCES} ${HEADERS})
target_include_directories(kernel.elf PRIVATE Include)
target_include_directories(kernel.elf PRIVATE ${CMAKE_SOURCE_DIR}/deps/Limine)

target_link_options(kernel.elf PRIVATE
        -g
        -T ${CMAKE_SOURCE_DIR}/src/config/linker.ld
        -nostdlib
        -nostartfiles
        -mno-red-zone
        ${LINKER_OPTIONS}
)