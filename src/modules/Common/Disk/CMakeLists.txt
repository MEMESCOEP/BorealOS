set(MODULE_NAME Disk)
project(${MODULE_NAME} LANGUAGES C CXX ASM)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_EXTENSIONS ON)

set(CMAKE_CXX_COMPILER ${CXX_COMPILER})
set(CMAKE_ASM_COMPILER ${ASM_COMPILER})

# Here you'd add source files, or other CMake commands to set up your module
# Every module requires configuration, which is done using the MODULE macro, and if it relies on something the RELY_ON macro.

file(GLOB_RECURSE SOURCE_FILES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp ${CMAKE_CURRENT_SOURCE_DIR}/*.c ${CMAKE_CURRENT_SOURCE_DIR}/*.S ${CMAKE_CURRENT_SOURCE_DIR}/*.h)
add_executable(${MODULE_NAME}.drv ${SOURCE_FILES})

target_link_options(${MODULE_NAME}.drv PRIVATE ${DEFAULT_LINKER_OPTIONS})
target_compile_options(${MODULE_NAME}.drv PRIVATE ${DEFAULT_CXX_COMPILE_OPTIONS})
target_link_libraries(${MODULE_NAME}.drv PRIVATE kernel.api)

set(DESTINATION_PATH "${MODULE_OUTPUT_DIR}/${MODULE_NAME}.drv")

add_custom_command(
        OUTPUT "${DESTINATION_PATH}"
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:${MODULE_NAME}.drv> "${DESTINATION_PATH}"
        DEPENDS ${MODULE_NAME}.drv
        COMMENT "Syncing ${MODULE_NAME}.drv to output directory"
)

set(COPY_TARGET_NAME "Copy${MODULE_NAME}Elf")
add_custom_target(${COPY_TARGET_NAME} ALL DEPENDS "${DESTINATION_PATH}")

set(OUTPUT_TARGET ${COPY_TARGET_NAME} PARENT_SCOPE)