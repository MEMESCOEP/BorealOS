cmake_minimum_required(VERSION 3.31)
project(Template LANGUAGES C CXX ASM)

# Each project is expected to have more cmake commands, since it is expected to be more complex, unlike kernel modules which always have the same structure.

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_EXTENSIONS ON)

set(CMAKE_CXX_COMPILER ${CXX_COMPILER})
set(CMAKE_ASM_COMPILER ${ASM_COMPILER})
set(CMAKE_C_COMPILER ${ASM_COMPILER})

# Here you'd add source files, or other CMake commands to set up your binary
file(GLOB_RECURSE SOURCE_FILES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp ${CMAKE_CURRENT_SOURCE_DIR}/*.c ${CMAKE_CURRENT_SOURCE_DIR}/*.S ${CMAKE_CURRENT_SOURCE_DIR}/*.h)
add_executable(${PROJECT_NAME} ${SOURCE_FILES})
# or
# add_library(${PROJECT_NAME} SHARED ${SOURCE_FILES}) if it's a shared library, or
# add_library(${PROJECT_NAME} STATIC ${SOURCE_FILES}) if it's a static library

set(DESTINATION_PATH "${BINARY_OUTPUT_DIR}/${PROJECT_NAME}") # or ${LIBRARY_OUTPUT_DIR} if it's a library

add_custom_command(
        OUTPUT "${DESTINATION_PATH}"
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:${PROJECT_NAME}> "${DESTINATION_PATH}"
        DEPENDS ${PROJECT_NAME}
        COMMENT "Syncing ${PROJECT_NAME} to output directory"
)

set(COPY_TARGET_NAME "Copy${PROJECT_NAME}Elf")
add_custom_target(${COPY_TARGET_NAME} ALL DEPENDS "${DESTINATION_PATH}")

set(OUTPUT_TARGET ${COPY_TARGET_NAME} PARENT_SCOPE)