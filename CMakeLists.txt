cmake_minimum_required(VERSION 3.31)
project(BorealOS)

if(NOT ARCH)
    set(ARCH "x86_64")
    message(STATUS "ARCH not specified. Defaulting to x86_64.")
else()
    message(STATUS "Building for architecture: ${ARCH}")
endif()

# Add the kernel project
add_subdirectory(src/kernel)

# Add a build target called Kernel that depends on the kernel.elf target and installs it into ./src/config/boot/kernel.elf
add_custom_target(Kernel ALL
        DEPENDS kernel.elf
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:kernel.elf> ${CMAKE_SOURCE_DIR}/src/config/boot/kernel.elf
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)