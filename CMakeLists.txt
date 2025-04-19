# CMakeList.txt : CMake project for Synthwave, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.28)
include(FetchContent)

# Enable Hot Reload for MSVC compilers if supported.
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project ("Synthwave")


# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


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

#assimp for importing assets
FetchContent_Declare(
  assimp # Name for FetchContent internal tracking
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


target_link_libraries(Synthwave PRIVATE 
glad 
glfw 
assimp 
glm::glm 
stb_image 
nlohmann_json::nlohmann_json
freetype
)




file(COPY core/src/renderer/shaders DESTINATION ${CMAKE_BINARY_DIR})
file(COPY assets DESTINATION ${CMAKE_BINARY_DIR})
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/data)