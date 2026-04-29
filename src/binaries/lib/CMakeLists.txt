cmake_minimum_required(VERSION 3.31)

# This gathers all the libraries in the ./common and ./${ARCH} directories, and builds them. Each binary is expected to have a CMakeLists.txt file that defines the binary.

file(GLOB MODULE_FOLDERS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/${ARCH}/*)
set(OUTPUT_TARGET "")
set(OUTPUT_TARGETS "")
foreach(MODULE_FOLDER ${MODULE_FOLDERS})
    if(IS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/${MODULE_FOLDER})
        set(OUTPUT_TARGET "")
        add_subdirectory(${MODULE_FOLDER})
        if(NOT "${OUTPUT_TARGET}" STREQUAL "")
            list(APPEND OUTPUT_TARGETS ${OUTPUT_TARGET})
            message(STATUS "Added binary ${MODULE_FOLDER} with output target ${OUTPUT_TARGET}")
        endif()
    endif()
endforeach()

# Find all common binaries in the directory Common and add them as subdirectories
file(GLOB COMMON_MODULE_FOLDERS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/Common/*)
foreach(MODULE_FOLDER ${COMMON_MODULE_FOLDERS})
    if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/${MODULE_FOLDER}")
        set(OUTPUT_TARGET "")
        add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/${MODULE_FOLDER})
        if(NOT "${OUTPUT_TARGET}" STREQUAL "")
            list(APPEND OUTPUT_TARGETS ${OUTPUT_TARGET})
            message(STATUS "Added common binary ${MODULE_FOLDER} with output target ${OUTPUT_TARGET}")
        endif()
    endif()
endforeach()

add_custom_target(
        Libs ALL
        DEPENDS ${OUTPUT_TARGETS}
)