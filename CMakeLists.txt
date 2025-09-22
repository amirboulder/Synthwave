cmake_minimum_required (VERSION 3.28)

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

project ("Synthwave")

include(FetchContent)
set(FETCHCONTENT_QUIET FALSE)
include(CheckIPOSupported)

# Enable Hot Reload for MSVC compilers if supported.
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()


# Add source to this project's executable.
add_executable (Synthwave "Synthwave.cpp" "Synthwave.h" )



#=============================================
#Slang
#=============================================

FetchContent_Declare(Slang
    GIT_REPOSITORY https://github.com/shader-slang/slang.git
    GIT_TAG "v2025.10.2"
    GIT_SHALLOW ON
    GIT_PROGRESS TRUE
    OVERRIDE_FIND_PACKAGE
    EXCLUDE_FROM_ALL
    SYSTEM
)

set(SLANG_EMBED_CORE_MODULE ON)
set(SLANG_EMBED_CORE_MODULE_SOURCE ON)
set(SLANG_ENABLE_DXIL OFF)
set(SLANG_ENABLE_ASAN OFF)
set(SLANG_ENABLE_FULL_IR_VALIDATION OFF)
set(SLANG_ENABLE_IR_BREAK_ALLOC OFF)
set(SLANG_ENABLE_GFX OFF) 
set(SLANG_ENABLE_SLANGD OFF)
set(SLANG_ENABLE_SLANGC OFF)
set(SLANG_ENABLE_SLANGRT OFF)
set(SLANG_ENABLE_SLANG_GLSLANG OFF)
set(SLANG_ENABLE_TESTS OFF)
set(SLANG_ENABLE_EXAMPLES OFF)
set(SLANG_LIB_TYPE "SHARED")
set(SLANG_ENABLE_RELEASE_DEBUG_INFO OFF)
set(SLANG_ENABLE_SPLIT_DEBUG_INFO OFF)
set(SLANG_SLANG_LLVM_FLAVOR "DISABLE")

set(SLANG_ENABLE_CUDA OFF)
set(SLANG_ENABLE_OPTIX OFF)
set(SLANG_ENABLE_NVAPI OFF)
set(SLANG_ENABLE_AFTERMATH OFF)
set(SLANG_ENABLE_XLIB OFF)

set(SLANG_ENABLE_DX_ON_VK OFF)
set(SLANG_ENABLE_SLANG_RHI OFF)
set(SLANG_USE_SYSTEM_MINIZ OFF)
set(SLANG_USE_SYSTEM_LZ4 OFF)
set(SLANG_USE_SYSTEM_VULKAN_HEADERS ON)
set(SLANG_USE_SYSTEM_SPIRV_HEADERS OFF)
set(SLANG_USE_SYSTEM_UNORDERED_DENSE OFF)

FetchContent_MakeAvailable(Slang)

#=============================================
#SDL
#=============================================
FetchContent_Declare(
        SDL3
        GIT_REPOSITORY "https://github.com/libsdl-org/SDL.git"
        GIT_TAG release-3.2.12
        GIT_SHALLOW TRUE
        EXCLUDE_FROM_ALL
    )
FetchContent_MakeAvailable(SDL3)
set_property(DIRECTORY "${SDL3_SOURCE_DIR}" PROPERTY EXCLUDE_FROM_ALL TRUE)



#=============================================
#SDL_IMAGE
#=============================================

FetchContent_Declare(
    SDL_image
    GIT_REPOSITORY https://github.com/libsdl-org/SDL_image.git
    GIT_TAG release-3.2.4
)
set(SDL3_IMAGE_AVIF OFF CACHE BOOL "Disable AVIF support" FORCE)
set(SDLIMAGE_AVIF OFF)	# disable formats we don't use to make the build faster and smaller.
set(SDLIMAGE_BMP OFF)
set(SDLIMAGE_JPEG OFF)
set(SDLIMAGE_WEBP OFF)
FetchContent_MakeAvailable(SDL_image)
set_property(DIRECTORY "${SDL_image_SOURCE_DIR}" PROPERTY EXCLUDE_FROM_ALL TRUE)


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
  GIT_TAG v5.2.5 # Use a specific, stable tag or commit hash
  GIT_SHALLOW TRUE
)

FetchContent_MakeAvailable(assimp)

#=============================================
#glm
#=============================================
FetchContent_Declare(
	glm
	GIT_REPOSITORY	https://github.com/g-truc/glm.git
	GIT_TAG 	bf71a834948186f4097caa076cd2663c69a10e1e #refs/tags/1.0.1
    GIT_SHALLOW TRUE
)

FetchContent_MakeAvailable(glm)


#=============================================
#nlohmann_json
#=============================================
FetchContent_Declare(json URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz)
FetchContent_MakeAvailable(json)

#=============================================
#JOLT physics
#=============================================
FetchContent_Declare(
        JoltPhysics
        GIT_REPOSITORY "https://github.com/jrouwe/JoltPhysics"
        GIT_TAG "v5.3.0"
        UPDATE_DISCONNECTED TRUE
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
# set(BUILD_SHARED_LIBS ON)
set(INTERPROCEDURAL_OPTIMIZATION=${IS_IPO_SUPPORTED})
if(NOT JoltPhysics_POPULATED)
    FetchContent_MakeAvailable(JoltPhysics)

  set(PHYSICS_REPO_ROOT ${joltphysics_SOURCE_DIR})
  include(${joltphysics_SOURCE_DIR}/Jolt/Jolt.cmake)
  target_compile_definitions(Jolt PUBLIC "$<$<CONFIG:RelWithDebInfo>:JPH_PROFILE_ENABLED;JPH_DEBUG_RENDERER>")
  target_compile_definitions(Jolt PUBLIC "$<$<CONFIG:Debug>:JPH_PROFILE_ENABLED;JPH_DEBUG_RENDERER>")
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
#inih for congig files
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
#=============================================
#=============================================

target_link_libraries(Synthwave PUBLIC
SDL3::SDL3
slang
SDL3_image::SDL3_image
SDL3_ttf::SDL3_ttf
assimp 
glm::glm 
nlohmann_json::nlohmann_json
Jolt
OptickCore
inih
)



file(COPY core/src/renderer/shaders/slang DESTINATION ${CMAKE_BINARY_DIR}/shaders/)
file(COPY assets DESTINATION ${CMAKE_BINARY_DIR})
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/data)
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/config)
