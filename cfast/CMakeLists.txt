cmake_minimum_required(VERSION 3.20)
project(cfast) 

# ==============================================================================
# Global Configuration
# ==============================================================================

# Force C11 standard
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

# Set output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Windows-specific settings
if(WIN32)
    # Use MultiThreaded DLL runtime
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
    
    # Enable hot reload friendly settings
    set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS OFF)  # We'll use explicit exports
    
    # Set Windows version
    add_compile_definitions(_WIN32_WINNT=0x0601)  # Windows 7 minimum
endif()

# Configuration options
option(ENABLE_HOT_RELOAD "Enable hot reload support" ON)
option(BUILD_EDITOR "Build editor tools" ON)
option(USE_SANITIZERS "Enable address sanitizers in debug" OFF)

# ==============================================================================
# Compiler Settings
# ==============================================================================

if(MSVC)
    # Visual Studio specific flags
    add_compile_options(
        /W4                 # Warning level 4
        /WX-                # Don't treat warnings as errors (optional: change to /WX)
        /permissive-        # Strict standard conformance
        /Zc:__cplusplus     # Enable updated __cplusplus macro
        /Zc:inline          # Remove unreferenced COMDAT
        /fp:fast            # Fast floating point
        /GR-                # Disable RTTI (we don't need it for C)
        /EHsc               # Enable C++ exceptions (for any C++ in the future)
    )
    
    # Debug-specific flags
    add_compile_options($<$<CONFIG:Debug>:/Zi>)      # Debug information
    add_compile_options($<$<CONFIG:Debug>:/Od>)      # Disable optimization
    add_compile_options($<$<CONFIG:Debug>:/RTC1>)    # Runtime checks
    add_compile_options($<$<CONFIG:Debug>:/DDEBUG>)
    add_compile_options($<$<CONFIG:Debug>:/D_DEBUG>)
    
    # Release-specific flags
    add_compile_options($<$<CONFIG:Release>:/O2>)    # Optimize for speed
    add_compile_options($<$<CONFIG:Release>:/Oi>)    # Enable intrinsics
    add_compile_options($<$<CONFIG:Release>:/GL>)    # Whole program optimization
    add_compile_options($<$<CONFIG:Release>:/DNDEBUG>)
    
    # Release linker flags
    add_link_options($<$<CONFIG:Release>:/LTCG>)     # Link time code generation
    add_link_options($<$<CONFIG:Release>:/OPT:REF>)  # Remove unreferenced code
    add_link_options($<$<CONFIG:Release>:/OPT:ICF>)  # Identical COMDAT folding
    
    # Incremental linking for hot reload
    if(ENABLE_HOT_RELOAD)
        add_link_options($<$<CONFIG:Debug>:/INCREMENTAL>)
    endif()
else()
    # GCC/Clang flags (for MinGW on Windows)
    add_compile_options(
        -Wall
        -Wextra
        -Wpedantic
        -Wno-unused-parameter
        $<$<CONFIG:Debug>:-g>
        $<$<CONFIG:Debug>:-O0>
        $<$<CONFIG:Debug>:-DDEBUG>
        $<$<CONFIG:Release>:-O3>
        $<$<CONFIG:Release>:-DNDEBUG>
    )
endif()

# Sanitizers for debug builds
if(USE_SANITIZERS AND CMAKE_BUILD_TYPE STREQUAL "Debug")
    if(MSVC)
        add_compile_options(/fsanitize=address)
    else()
        add_compile_options(-fsanitize=address -fno-omit-frame-pointer)
        add_link_options(-fsanitize=address)
    endif()
endif()

# ==============================================================================
# Source Files Organization
# ==============================================================================

# Core reflection system sources (in main executable)
set(REFLECTION_CORE_SOURCES
    source/reflection_core.h
    source/reflection_core.c
)

# Shared type definitions
set(SHARED_HEADERS
    source/game_types.h
)

# Main executable sources
set(MAIN_SOURCES
    source/main.c
    ${REFLECTION_CORE_SOURCES}
)

# Game module sources (DLL)
set(GAME_MODULE_SOURCES
    source/game_module.c
)

# Editor sources (optional)
set(EDITOR_SOURCES
    source/editor.c
)

# Hot reload system (optional)
set(HOT_RELOAD_SOURCES
    source/hot_reload.c
)

# ==============================================================================
# Core Reflection Library (static library for sharing)
# ==============================================================================

add_library(reflection_core STATIC
    ${REFLECTION_CORE_SOURCES}
)

target_include_directories(reflection_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/source
)

# Core needs to know about hot reload setting
if(ENABLE_HOT_RELOAD)
    target_compile_definitions(reflection_core PUBLIC HOT_RELOAD_ENABLED)
endif()

# ==============================================================================
# Game Module (DLL that can be hot-reloaded)
# ==============================================================================

add_library(game_module SHARED
    ${GAME_MODULE_SOURCES}
)

target_include_directories(game_module PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/source
)

target_link_libraries(game_module PRIVATE
    reflection_core
)

# Export symbols from DLL
if(WIN32)
    target_compile_definitions(game_module PRIVATE
        GAME_MODULE_EXPORTS  # Define this when building the DLL
    )
endif()

# Generate import library
set_target_properties(game_module PROPERTIES
    WINDOWS_EXPORT_ALL_SYMBOLS OFF
    PREFIX ""  # No 'lib' prefix on Windows
)

# For hot reload: copy DLL to a different name each time
if(ENABLE_HOT_RELOAD)
    add_custom_command(TARGET game_module POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
            $<TARGET_FILE:game_module>
            $<TARGET_FILE_DIR:game_module>/game_module_live.dll
        COMMENT "Creating hot-reload copy of game module"
    )
endif()

# ==============================================================================
# Main Executable
# ==============================================================================

add_executable(reflection_demo
    ${MAIN_SOURCES}
)

target_include_directories(reflection_demo PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/source
)

target_link_libraries(reflection_demo PRIVATE
    reflection_core
)

# Windows subsystem (console app)
if(WIN32)
    set_target_properties(reflection_demo PROPERTIES
        WIN32_EXECUTABLE FALSE  # Console application
    )
endif()

# Include hot reload if enabled
if(ENABLE_HOT_RELOAD)
    target_sources(reflection_demo PRIVATE ${HOT_RELOAD_SOURCES})
    target_compile_definitions(reflection_demo PRIVATE HOT_RELOAD_ENABLED)
endif()

# ==============================================================================
# Editor Executable (optional)
# ==============================================================================

if(BUILD_EDITOR)
    add_executable(reflection_editor
        ${EDITOR_SOURCES}
        source/editor_main.c  # Separate main for editor
    )
    
    target_include_directories(reflection_editor PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/source
    )
    
    target_link_libraries(reflection_editor PRIVATE
        reflection_core
    )
    
    # Editor always needs hot reload for live editing
    if(ENABLE_HOT_RELOAD)
        target_sources(reflection_editor PRIVATE ${HOT_RELOAD_SOURCES})
        target_compile_definitions(reflection_editor PRIVATE HOT_RELOAD_ENABLED)
    endif()
endif()

# ==============================================================================
# Installation Rules
# ==============================================================================

install(TARGETS reflection_demo
    RUNTIME DESTINATION bin
)

install(TARGETS game_module
    LIBRARY DESTINATION bin  # DLLs go in bin on Windows
    RUNTIME DESTINATION bin
)

if(BUILD_EDITOR)
    install(TARGETS reflection_editor
        RUNTIME DESTINATION bin
    )
endif()

# Install headers for SDK
install(FILES 
    ${REFLECTION_CORE_SOURCES}
    ${SHARED_HEADERS}
    DESTINATION include/reflection
)

# ==============================================================================
# Testing Support
# ==============================================================================

enable_testing()

# Simple test executable
add_executable(reflection_test
    source/test/test_reflection.c
)

target_link_libraries(reflection_test PRIVATE
    reflection_core
)

add_test(NAME BasicReflectionTest COMMAND reflection_test)

# ==============================================================================
# Helper Targets
# ==============================================================================

# Clean all generated files
add_custom_target(clean_all
    COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_BINARY_DIR}/bin
    COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_BINARY_DIR}/lib
    COMMENT "Cleaning all build artifacts"
)

# Format source code (requires clang-format)
add_custom_target(format
    COMMAND clang-format -i ${CMAKE_CURRENT_SOURCE_DIR}/source/*.c
    COMMAND clang-format -i ${CMAKE_CURRENT_SOURCE_DIR}/source/*.h
    COMMENT "Formatting source code"
)

# ==============================================================================
# Visual Studio Specific Settings
# ==============================================================================

if(MSVC_IDE)
    # Set the startup project
    set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} 
        PROPERTY VS_STARTUP_PROJECT reflection_demo)
    
    # Organize files in folders
    source_group("Core" FILES ${REFLECTION_CORE_SOURCES})
    source_group("Headers" FILES ${SHARED_HEADERS})
    source_group("Game Module" FILES ${GAME_MODULE_SOURCES})
    source_group("Editor" FILES ${EDITOR_SOURCES})
    source_group("Hot Reload" FILES ${HOT_RELOAD_SOURCES})
    
    # Enable Edit and Continue for debug builds
    if(ENABLE_HOT_RELOAD)
        set_property(TARGET reflection_demo PROPERTY 
            VS_DEBUGGER_ENVIRONMENT "PATH=$<TARGET_FILE_DIR:game_module>;%PATH%")
    endif()
endif()

# ==============================================================================
# Configuration Summary
# ==============================================================================

message(STATUS "")
message(STATUS "=== Hybrid Reflection System Configuration ===")
message(STATUS "Build type:        ${CMAKE_BUILD_TYPE}")
message(STATUS "C Standard:        C${CMAKE_C_STANDARD}")
message(STATUS "Compiler:          ${CMAKE_C_COMPILER_ID} ${CMAKE_C_COMPILER_VERSION}")
message(STATUS "Hot Reload:        ${ENABLE_HOT_RELOAD}")
message(STATUS "Build Editor:      ${BUILD_EDITOR}")
message(STATUS "Sanitizers:        ${USE_SANITIZERS}")
message(STATUS "Install prefix:    ${CMAKE_INSTALL_PREFIX}")
message(STATUS "")

# ==============================================================================
# Example Build Instructions (in comments)
# ==============================================================================

# Build instructions:
# 
# 1. Create build directory:
#    mkdir build
#    cd build
#
# 2. Configure for Visual Studio 2022 (64-bit):
#    cmake -G "Visual Studio 17 2022" -A x64 ..
#
# 3. Build Debug:
#    cmake --build . --config Debug
#
# 4. Build Release:
#    cmake --build . --config Release
#
# 5. Run tests:
#    ctest -C Debug
#
# 6. Install:
#    cmake --install . --config Release
#
# Alternative: Using Ninja for faster builds:
#    cmake -G Ninja -DCMAKE_BUILD_TYPE=Debug ..
#    cmake --build .
#
# Options:
#    -DENABLE_HOT_RELOAD=OFF     # Disable hot reload
#    -DBUILD_EDITOR=OFF          # Don't build editor
#    -DUSE_SANITIZERS=ON         # Enable sanitizers in debug