# CMakeList.txt : CMake project for Synthwave, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.28)

project ("Synthwave")

include(FetchContent)
include(CheckIPOSupported)

# Enable Hot Reload for MSVC compilers if supported.
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}" )


# Add source to this project's executable.
add_executable (Synthwave "Synthwave.cpp" "Synthwave.h" )


# Create GLAD static library
add_library(glad STATIC thirdParty/glad/src/glad.c )
target_include_directories(glad PUBLIC ${CMAKE_SOURCE_DIR}/thirdParty/glad/include)



# Define GLFW as an imported static library
add_library(glfw STATIC IMPORTED)
set_target_properties(glfw PROPERTIES
    IMPORTED_LOCATION "${CMAKE_SOURCE_DIR}/thirdParty/glfw/lib/glfw3.lib" # Path to dll
    INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_SOURCE_DIR}/thirdParty/glfw/include" # Path to headers
)


FetchContent_Declare(
        SDL3
        GIT_REPOSITORY "https://github.com/libsdl-org/SDL.git"
        GIT_TAG release-3.2.12
        EXCLUDE_FROM_ALL
    )
    FetchContent_MakeAvailable(SDL3)


#assimp for importing assets
FetchContent_Declare(
  assimp 
  GIT_REPOSITORY https://github.com/assimp/assimp.git
  GIT_TAG v5.2.5 # Use a specific, stable tag or commit hash
)
set(BUILD_SHARED_LIBS ON CACHE BOOL "Build Assimp as shared library")
FetchContent_MakeAvailable(assimp)


#Set the DLL directory
set(ASSIMP_DLL_DIR "${CMAKE_BINARY_DIR}/_deps/assimp-build/bin")

# Conditionally set the DLL name based on build type
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(ASSIMP_DLL_NAME "assimp-vc143-mtd.dll")
else()
    set(ASSIMP_DLL_NAME "assimp-vc143-mt.dll")
endif()

# Copy the appropriate DLL
add_custom_command(TARGET Synthwave POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${ASSIMP_DLL_DIR}/${ASSIMP_DLL_NAME}"
        $<TARGET_FILE_DIR:Synthwave>
    COMMENT "Copying Assimp DLL to executable directory"
)


#glm
FetchContent_Declare(
	glm
	GIT_REPOSITORY	https://github.com/g-truc/glm.git
	GIT_TAG 	bf71a834948186f4097caa076cd2663c69a10e1e #refs/tags/1.0.1
)

FetchContent_MakeAvailable(glm)

# Create an interface library for stb_image
add_library(stb_image INTERFACE)
target_include_directories(stb_image INTERFACE ${CMAKE_SOURCE_DIR}/thirdParty/stb)

#nlohmann_json
FetchContent_Declare(json URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz)
FetchContent_MakeAvailable(json)

# Enable FetchContent for FreeType
include(FetchContent)
FetchContent_Declare(
    freetype
    GIT_REPOSITORY https://github.com/freetype/freetype.git
    GIT_TAG VER-2-13-3 
)
add_custom_command(TARGET Synthwave POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:freetype>
        $<TARGET_FILE_DIR:Synthwave>
)
FetchContent_MakeAvailable(freetype)



FetchContent_Declare(
        JoltPhysics
        GIT_REPOSITORY "https://github.com/jrouwe/JoltPhysics"
        GIT_TAG "v5.2.0"
        UPDATE_DISCONNECTED TRUE
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




target_link_libraries(Synthwave PRIVATE 
glad
SDL3::SDL3
glfw 
assimp 
glm::glm 
stb_image 
nlohmann_json::nlohmann_json
freetype
Jolt
)


file(COPY core/src/renderer/shaders DESTINATION ${CMAKE_BINARY_DIR})
file(COPY assets DESTINATION ${CMAKE_BINARY_DIR})
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/data)