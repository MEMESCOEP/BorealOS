cmake_minimum_required(VERSION 3.31)

# This CMakeLists.txt file is responsible for gathering, and building all the kernel modules.
# It does this by looking for subdirectories in the current directory, and adding them as subdirectories. Each subdirectory is expected to contain a CMakeLists.txt file that defines a module.
# The output targets of each module are collected in the variable OUTPUT_TARGETS, which is then used as a dependency for the custom target Modules, which is the main target of this CMakeLists.txt file.

if (ARCH STREQUAL "x86_64")
    set(LINKER_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/x86_64/module.ld)
else()
    message(FATAL_ERROR "Unsupported architecture: ${ARCH}. Supported architectures is currently only x86_64.")
endif()

set(DEFAULT_LINKER_OPTIONS
        -r
        -T ${LINKER_SCRIPT}
)

set(DEFAULT_CXX_COMPILE_OPTIONS
        ${KERNEL_COMPILE_FLAGS_CXX}
        -fno-pie -no-pie
)

# Find all folders in the directory ${ARCH} and add them as subdirectories
file(GLOB MODULE_FOLDERS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/${ARCH}/*)
set(OUTPUT_TARGET "")
set(OUTPUT_TARGETS "")
foreach(MODULE_FOLDER ${MODULE_FOLDERS})
    if(IS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/${MODULE_FOLDER})
        set(OUTPUT_TARGET "")
        add_subdirectory(${MODULE_FOLDER})

        if(NOT "${OUTPUT_TARGET}" STREQUAL "")
            list(APPEND OUTPUT_TARGETS ${OUTPUT_TARGET})
            message(STATUS "Added module ${MODULE_FOLDER} with output target ${OUTPUT_TARGET}")
        endif()
    endif()
endforeach()

# Find all common modules in the directory Common and add them as subdirectories
file(GLOB COMMON_MODULE_FOLDERS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/Common/*)
foreach(MODULE_FOLDER ${COMMON_MODULE_FOLDERS})
    if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/${MODULE_FOLDER}")
        set(OUTPUT_TARGET "")
        add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/${MODULE_FOLDER})

        if(NOT "${OUTPUT_TARGET}" STREQUAL "")
            list(APPEND OUTPUT_TARGETS ${OUTPUT_TARGET})
            message(STATUS "Added common module ${MODULE_FOLDER} with output target ${OUTPUT_TARGET}")
        endif()
    endif()
endforeach()

add_custom_target(
        Modules ALL
        DEPENDS ${OUTPUT_TARGETS} kernel.elf
)