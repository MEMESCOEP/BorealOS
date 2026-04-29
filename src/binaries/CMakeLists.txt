cmake_minimum_required(VERSION 3.31)

# This CMakeLists.txt file is responsible for gathering, and building all library & executable binaries.

set(DEFAULT_CXX_COMPILE_OPTIONS
        -fno-exceptions
        -fno-rtti
        -fno-common
        -ffreestanding
)

set(DEFAULT_C_COMPILE_OPTIONS
        -fno-common
        -ffreestanding
)

set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -nostdlib")

add_subdirectory(lib)
add_subdirectory(bin)

add_custom_target(
        Binaries ALL
        DEPENDS Libs Bins
)