cmake_minimum_required(VERSION 3.31)
project(kernel LANGUAGES CXX ASM)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_EXTENSIONS ON)

set(CMAKE_CXX_COMPILER ${CXX_COMPILER})
set(CMAKE_ASM_COMPILER ${ASM_COMPILER})

set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

file(GLOB_RECURSE SOURCES "arch/${ARCH_DIR}/*.cpp" "arch/${ARCH_DIR}/*.S")
file(GLOB_RECURSE HEADERS "Include/*.h" "arch/${ARCH_DIR}/*.h")
file(GLOB_RECURSE COMMON_SOURCES "arch/common/*.cpp" "arch/common/*.S") # the common headers are located in Include/Common

include(FetchContent)

FetchContent_Declare(
        flanterm
        GIT_REPOSITORY https://codeberg.org/Mintsuki/Flanterm
        GIT_TAG        trunk
)

FetchContent_Declare(
        lai
        GIT_REPOSITORY https://github.com/managarm/lai.git
        GIT_TAG        master
)

FetchContent_Declare(
        tlsf
        GIT_REPOSITORY https://github.com/mattconte/tlsf.git
        GIT_TAG        master
)

FetchContent_MakeAvailable(flanterm lai tlsf)

file(GLOB_RECURSE FLANTERM_SOURCES ${flanterm_SOURCE_DIR}/src/*.c)
file(GLOB_RECURSE LAI_SOURCES ${lai_SOURCE_DIR}/*.c)
FILE(GLOB_RECURSE TLSF_SOURCES ${tlsf_SOURCE_DIR}/*.c)

set_source_files_properties(${LAI_SOURCES} PROPERTIES COMPILE_FLAGS "-w -Wno-pedantic -Wno-error")
set_source_files_properties(${tlsf_SOURCE_DIR}/tlsf.c PROPERTIES COMPILE_FLAGS "-D tlsf_assert=kernel_assert -Wno-implicit-function-declaration -Wno-unused-function")

add_executable(kernel.elf ${SOURCES} ${COMMON_SOURCES} ${HEADERS} ${FLANTERM_SOURCES} ${LAI_SOURCES} ${TLSF_SOURCES})
target_include_directories(kernel.elf PRIVATE Include)
target_include_directories(kernel.elf PRIVATE ${CMAKE_SOURCE_DIR}/deps/Limine)
target_include_directories(kernel.elf PRIVATE ${flanterm_SOURCE_DIR}/src)
target_include_directories(kernel.elf PRIVATE ${lai_SOURCE_DIR}/include)
target_include_directories(kernel.elf PRIVATE ${tlsf_SOURCE_DIR})

target_compile_options(kernel.elf PRIVATE
        $<$<COMPILE_LANGUAGE:CXX>:${KERNEL_COMPILE_FLAGS_CXX}>
        $<$<COMPILE_LANGUAGE:C>:${KERNEL_COMPILE_FLAGS_C}>
)

target_link_options(kernel.elf PRIVATE
        -g
        -T ${CMAKE_SOURCE_DIR}/src/config/linker.ld
        -nostdlib
        -fvisibility=default
        -ggdb
        -nostartfiles
        ${LINKER_OPTIONS}
)