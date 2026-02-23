cmake_minimum_required (VERSION 3.30)

#We need C here because we're building C libraries like SDL, Flecs
project (Synthwave VERSION 0.1 LANGUAGES CXX C)


set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

include(FetchContent)
set(FETCHCONTENT_QUIET FALSE)
include(CheckIPOSupported)

# Enable Hot Reload for MSVC compilers if supported.
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

#=============================================
#Executable
#=============================================

add_executable (Synthwave "Synthwave.cpp")

#Using /bigobj we hit the 65000 segment limit in translation unit,
#most likely because of heavy usage of templated code in Flecs
if(MSVC)
    target_compile_options(Synthwave PRIVATE /MP #Compile using multi cores if possbile
    /utf-8 # use utf-8 encoding
    /bigobj)
endif()


#Get all the .ixx files and add them as modules,
#When a new modules is added we must save this file again so it runs, TODO maybe check when we run the application
# This would have to be changed to "*.cppm" most likely we use compile with clang
file(GLOB_RECURSE MODULE_INTERFACE_UNITS "*.ixx")

target_sources(Synthwave
    PUBLIC
    FILE_SET CXX_MODULES FILES
             ${MODULE_INTERFACE_UNITS}
)

#=============================================
# Slang-shader language and compiler - using Prebuilt Binary because building from scratch took a few minutes.
#=============================================
set(SLANG_VERSION "2026.2.1") #note v is removed from the tag becuase SLANG_ARCHIVE_NAME does not have the v.

# Detect platform
if(WIN32)
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "ARM64|arm64|aarch64")
        set(SLANG_PLATFORM "windows-aarch64")
    else()
        set(SLANG_PLATFORM "windows-x86_64")
    endif()
elseif(APPLE)
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "arm64|aarch64")
        set(SLANG_PLATFORM "macos-aarch64")
    else()
        set(SLANG_PLATFORM "macos-x86_64")
    endif()
elseif(UNIX)
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64|arm64")
        set(SLANG_PLATFORM "linux-aarch64")
    else()
        set(SLANG_PLATFORM "linux-x86_64")
    endif()
else()
    message(FATAL_ERROR "Unsupported platform for Slang prebuilt binary")
endif()

set(SLANG_ARCHIVE_NAME "slang-${SLANG_VERSION}-${SLANG_PLATFORM}")
set(SLANG_DOWNLOAD_URL
    "https://github.com/shader-slang/slang/releases/download/v${SLANG_VERSION}/${SLANG_ARCHIVE_NAME}.zip"
)

include(FetchContent)
FetchContent_Declare(Slang
    URL                        ${SLANG_DOWNLOAD_URL}
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_MakeAvailable(Slang)

set(SLANG_DIR "${slang_SOURCE_DIR}")

# Post v2025.21: real library is slang-compiler.dll
# slang.dll still exists as a proxy redirect until end of 2026
if(WIN32)
    set(SLANG_DLL      "${SLANG_DIR}/bin/slang-compiler.dll")
    set(SLANG_IMPLIB   "${SLANG_DIR}/lib/slang-compiler.lib")
elseif(APPLE)
    file(GLOB SLANG_DLL "${SLANG_DIR}/lib/libslang-compiler.*.dylib")
    list(GET SLANG_DLL 0 SLANG_DLL)
elseif(UNIX)
    file(GLOB SLANG_DLL "${SLANG_DIR}/lib/libslang-compiler.so.*.*")
    list(GET SLANG_DLL 0 SLANG_DLL)
endif()

find_path(SLANG_INCLUDE_DIR
    NAMES slang.h
    PATHS "${SLANG_DIR}/include"
    NO_DEFAULT_PATH
    REQUIRED
)

# Imported target
add_library(Slang::slang SHARED IMPORTED GLOBAL)
if(WIN32)
    set_target_properties(Slang::slang PROPERTIES
        IMPORTED_LOCATION             "${SLANG_DLL}"
        IMPORTED_IMPLIB               "${SLANG_IMPLIB}"
        INTERFACE_INCLUDE_DIRECTORIES "${SLANG_INCLUDE_DIR}"
    )
else()
    set_target_properties(Slang::slang PROPERTIES
        IMPORTED_LOCATION             "${SLANG_DLL}"
        INTERFACE_INCLUDE_DIRECTORIES "${SLANG_INCLUDE_DIR}"
    )
endif()

# Copy DLL next to your executable at build time (Windows only)
if(WIN32)
    add_custom_command(TARGET Synthwave POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${SLANG_DLL}"
            $<TARGET_FILE_DIR:Synthwave>
        # Also copy the proxy slang.dll if you have dependencies that still expect it
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${SLANG_DIR}/bin/slang.dll"
            $<TARGET_FILE_DIR:Synthwave>
    )
endif()

message(STATUS "Slang ${SLANG_VERSION} prebuilt from: ${SLANG_DIR}")
message(STATUS "Slang DLL:  ${SLANG_DLL}")

#=============================================
#SDL
#=============================================
FetchContent_Declare(
        SDL3
        GIT_REPOSITORY "https://github.com/libsdl-org/SDL.git"
        GIT_TAG release-3.4.0
        GIT_SHALLOW TRUE
        EXCLUDE_FROM_ALL
    )
FetchContent_MakeAvailable(SDL3)

#=============================================
#SDL_IMAGE
#=============================================

FetchContent_Declare(
    SDL_image
    GIT_REPOSITORY https://github.com/libsdl-org/SDL_image.git
    GIT_TAG release-3.2.4
    EXCLUDE_FROM_ALL
)
# disable formats we don't use to make the build faster and smaller.
set(SDL3_IMAGE_AVIF OFF CACHE BOOL "Disable AVIF support" FORCE)
set(SDLIMAGE_AVIF OFF)	
set(SDLIMAGE_BMP OFF)
set(SDLIMAGE_JPEG OFF)
set(SDLIMAGE_WEBP OFF)
FetchContent_MakeAvailable(SDL_image)

#=============================================
#SDL TTF
#=============================================
FetchContent_Declare(
        SDL_ttf
        GIT_REPOSITORY "https://github.com/libsdl-org/SDL_ttf.git"
        GIT_TAG release-3.2.2
        GIT_SHALLOW TRUE
        EXCLUDE_FROM_ALL
    )
FetchContent_MakeAvailable(SDL_ttf)

#=============================================
#assimp for importing assets
#=============================================
FetchContent_Declare(
  assimp 
  GIT_REPOSITORY https://github.com/assimp/assimp.git
  GIT_TAG v5.4.0

)
    
set(ASSIMP_NO_CHARSET ON CACHE BOOL "" FORCE)   

set(ASSIMP_NO_EXPORT            ON  CACHE BOOL "" FORCE)  
set(ASSIMP_BUILD_ASSIMP_TOOLS   OFF CACHE BOOL "" FORCE)  
set(ASSIMP_BUILD_SAMPLES        OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_TESTS          OFF CACHE BOOL "" FORCE)

# Disable all importers except obj and gltf
set(ASSIMP_BUILD_ALL_IMPORTERS_BY_DEFAULT OFF CACHE BOOL "" FORCE)

set(ASSIMP_BUILD_OBJ_IMPORTER  ON  CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_GLTF_IMPORTER ON  CACHE BOOL "" FORCE)

set(ASSIMP_BUILD_ZLIB          ON CACHE BOOL "" FORCE) 
set(ASSIMP_INSTALL             OFF CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(assimp)

#=============================================
#glm
#=============================================
FetchContent_Declare(
	glm
	GIT_REPOSITORY	https://github.com/g-truc/glm.git
	GIT_TAG 	bf71a834948186f4097caa076cd2663c69a10e1e 
    GIT_SHALLOW TRUE
)

FetchContent_MakeAvailable(glm)

#=============================================
#JOLT physics
#=============================================
FetchContent_Declare(
        JoltPhysics
        GIT_REPOSITORY "https://github.com/jrouwe/JoltPhysics"
        GIT_TAG "v5.5.0"
        GIT_SHALLOW TRUE
        SOURCE_SUBDIR "Jolt"
)
check_ipo_supported(RESULT IS_IPO_SUPPORTED)
set(DOUBLE_PRECISION OFF)
set(GENERATE_DEBUG_SYMBOLS OFF)
set(CROSS_PLATFORM_DETERMINISTIC ON)
set(FLOATING_POINT_EXCEPTIONS_ENABLED OFF)
set(OBJECT_LAYER_BITS 32)
set(USE_SSE4_1 ON)
set(USE_SSE4_2 ON)
set(USE_AVX ON)
set(USE_AVX2 ON)
set(USE_AVX512 OFF)
set(USE_LZCNT ON)
set(USE_TZCNT ON)
set(USE_F16C ON)
set(USE_FMADD ON)
set(ENABLE_OBJECT_STREAM ON CACHE BOOL "" FORCE)
# set(BUILD_SHARED_LIBS ON)
set(INTERPROCEDURAL_OPTIMIZATION=${IS_IPO_SUPPORTED})
if(NOT JoltPhysics_POPULATED)
    FetchContent_MakeAvailable(JoltPhysics)
  set(PHYSICS_REPO_ROOT ${joltphysics_SOURCE_DIR})
  include(${joltphysics_SOURCE_DIR}/Jolt/Jolt.cmake)
  
  # Enable object stream support for serialization
  target_compile_definitions(Jolt PUBLIC JPH_OBJECT_STREAM)
  
  target_compile_definitions(Jolt PUBLIC "$<$<CONFIG:RelWithDebInfo>:JPH_PROFILE_ENABLED;JPH_DEBUG_RENDERER>")
  target_compile_definitions(Jolt PUBLIC "$<$<CONFIG:Debug>:JPH_PROFILE_ENABLED;JPH_DEBUG_RENDERER>")
  target_compile_definitions(Jolt PUBLIC "$<$<CONFIG:Release>:JPH_PROFILE_ENABLED;JPH_DEBUG_RENDERER>")
  if (FLOATING_POINT_EXCEPTIONS_ENABLED AND "${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
      target_compile_definitions(Jolt PUBLIC "$<$<CONFIG:RelWithDebInfo>:JPH_FLOATING_POINT_EXCEPTIONS_ENABLED>")
      target_compile_definitions(Jolt PUBLIC "$<$<CONFIG:Debug>:JPH_FLOATING_POINT_EXCEPTIONS_ENABLED>")
  endif()
endif()

#=============================================
#optick for profiling
#=============================================
FetchContent_Declare(
    optick
    GIT_REPOSITORY https://github.com/bombomby/optick.git
    GIT_TAG 1.4.0.0
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(optick)

#=============================================
#inih for config files
#=============================================
FetchContent_Declare(
    inih
    GIT_REPOSITORY https://github.com/benhoyt/inih.git
    GIT_TAG r61

)
set(INIH_BUILD_TESTS OFF CACHE INTERNAL "")
set(INIH_BUILD_EXAMPLES OFF CACHE INTERNAL "")
FetchContent_MakeAvailable(inih)

add_library(inih STATIC 
    ${inih_SOURCE_DIR}/ini.c
    ${inih_SOURCE_DIR}/cpp/INIReader.cpp
)

# Include INIReader header
target_include_directories(Synthwave PRIVATE ${inih_SOURCE_DIR}/cpp)

#=============================================
#flecs Entity Component System
#=============================================

FetchContent_Declare(
    flecs
    GIT_REPOSITORY https://github.com/SanderMertens/flecs.git
    GIT_TAG 8a126f0
)

set(FLECS_STATIC ON CACHE BOOL "" FORCE)
set(FLECS_CPP ON CACHE BOOL "" FORCE)
set(FLECS_PARSER OFF CACHE BOOL "" FORCE)
set(FLECS_HTTP OFF CACHE BOOL "" FORCE)
set(FLECS_REST OFF CACHE BOOL "" FORCE)
set(FLECS_DOC OFF CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(flecs)

#=============================================
#rapidjson JSON
#=============================================

FetchContent_Declare(
    rapidjson
    GIT_REPOSITORY https://github.com/Tencent/rapidjson.git
    GIT_TAG        24b5e7a          
    GIT_SHALLOW    TRUE
)

set(RAPIDJSON_BUILD_DOC OFF CACHE BOOL "" FORCE)
set(RAPIDJSON_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(RAPIDJSON_BUILD_TESTS OFF CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(rapidjson)

target_include_directories(Synthwave PRIVATE ${rapidjson_SOURCE_DIR}/include)

#=============================================
# IMGUI for GUI
#=============================================

include(FetchContent)
FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG v1.92.5-docking
)
FetchContent_MakeAvailable(imgui)

add_library(imgui STATIC
    # Core ImGui files (always needed)
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_demo.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    
    # SDL3 backend (for SDL_GPU which uses SDL3)
    ${imgui_SOURCE_DIR}/backends/imgui_impl_sdl3.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_sdlgpu3.cpp

    # Add FreeType support
    ${imgui_SOURCE_DIR}/misc/freetype/imgui_freetype.cpp
)

# ImGui include directories
target_include_directories(imgui PUBLIC
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
    ${imgui_SOURCE_DIR}/misc/freetype
)

# Enable FreeType in ImGui
target_compile_definitions(imgui PUBLIC 
IMGUI_ENABLE_FREETYPE
IMGUI_USE_WCHAR32  # Required for emojis above 0x10000
IMGUI_ENABLE_FREETYPE_PLUTOSVG 
)

# linking SDL3 because we're using sdlgpu3 backend for rendring
# ,SDL_TTF because it includes freeType which is needed for rendring emojis
# plutosvg which is in SDL3_ttf for SVG based emojis
target_link_libraries(imgui PUBLIC SDL3::SDL3 SDL3_ttf::SDL3_ttf Freetype::Freetype plutosvg::plutosvg)

#=============================================
#=============================================
#=============================================

target_link_libraries(Synthwave PUBLIC
imgui
SDL3::SDL3
Slang::slang
SDL3_image::SDL3_image
SDL3_ttf::SDL3_ttf
assimp 
glm::glm 
Jolt
OptickCore
inih
flecs::flecs
)


target_precompile_headers(Synthwave PRIVATE 
    "${CMAKE_SOURCE_DIR}/core/src/pch.h"
)

#Without this modules have access to the PCH without having to include the file, which also cause intellisense issues.
foreach(MODULE_FILE ${MODULE_INTERFACE_UNITS})
    set_source_files_properties(${MODULE_FILE}
        PROPERTIES SKIP_PRECOMPILE_HEADERS ON
    )
endforeach()

# Glob all shader source files so any addition/change triggers a recopy
file(GLOB_RECURSE SHADER_SOURCE_FILES
    "${CMAKE_SOURCE_DIR}/core/src/renderer/shaders/slang/*"
)

# If any shader src file is updated OR added, copy it into the build folder
add_custom_command(
    OUTPUT ${CMAKE_BINARY_DIR}/shaders/slang
    COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different
            ${CMAKE_SOURCE_DIR}/core/src/renderer/shaders/slang
            ${CMAKE_BINARY_DIR}/shaders/slang
    DEPENDS ${SHADER_SOURCE_FILES}
    COMMENT "Copying shader files..."
)


add_custom_target(copy_shaders ALL
    DEPENDS ${CMAKE_BINARY_DIR}/shaders/slang)

add_dependencies(Synthwave copy_shaders)

file(COPY assets DESTINATION ${CMAKE_BINARY_DIR})
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/data)
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/config)
