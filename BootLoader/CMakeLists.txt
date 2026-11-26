cmake_minimum_required(VERSION 3.22)

#
# This file is generated only once,
# and is not re-generated if converter is called multiple times.
#
# User is free to modify the file as much as necessary
#

# Setup compiler settings
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)


# Define the build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif()

# Set the project name
set(CMAKE_PROJECT_NAME BootLoader)

# Enable compile command to ease indexing with e.g. clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

# Core project settings
project(${CMAKE_PROJECT_NAME})
message("Build type: " ${CMAKE_BUILD_TYPE})

# Enable CMake support for ASM and C languages
enable_language(C ASM)

# Create an executable object type
add_executable(${CMAKE_PROJECT_NAME})

# Add STM32CubeMX generated sources
add_subdirectory(cmake/stm32cubemx)

# Link directories setup
target_link_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined library search paths
)

# Add sources to executable
target_sources(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user sources here
)

# Add include paths
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined include paths
)

# Add project symbols (macros)
target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined symbols
)

# Remove wrong libob.a library dependency when using cpp files
list(REMOVE_ITEM CMAKE_C_IMPLICIT_LINK_LIBRARIES ob)

# Add linked libraries
target_link_libraries(${CMAKE_PROJECT_NAME}
    stm32cubemx

    # Add user defined libraries
)

# 共享 APP 已有的 BSP 接口，避免两份存储格式/USB 队列实现漂移。
target_sources(${CMAKE_PROJECT_NAME} PRIVATE
    UserFile/BootLoader.c
    UserFile/BootFlash.c
    UserFile/IapUsb.c
    ../UserFile/Bsp/BspFlash.c
    ../UserFile/Bsp/BspUsbVcp.c
)
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    UserFile ../UserFile/Bsp
)
# USB HAL、协议和 Flash 日志必须一起落在 32KB BootLoader 分区。
foreach(target_name IN ITEMS ${CMAKE_PROJECT_NAME} STM32_Drivers USB_Device_Library)
    target_compile_options(${target_name} PRIVATE -Os)
endforeach()
set(FIRMWARE_BIN "${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_PROJECT_NAME}.bin")
add_custom_command(TARGET ${CMAKE_PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:${CMAKE_PROJECT_NAME}> ${FIRMWARE_BIN}
    BYPRODUCTS ${FIRMWARE_BIN}
    COMMENT "Generating ${FIRMWARE_BIN}"
    VERBATIM
)

# 临时排障：正常复位立即进入 APP，不依赖 BootInfo 或启动校验。
option(BOOT_DEBUG_DIRECT_APP "Boot APP immediately without startup validation" ON)
target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
    BOOT_DEBUG_DIRECT_APP=$<BOOL:${BOOT_DEBUG_DIRECT_APP}>
)
