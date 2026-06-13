cmake_minimum_required(VERSION 3.28)
project(MyCoolGame LANGUAGES CXX)

# Link to engine
add_library(CrashTheSim SHARED
    src/GameMain.cpp
    src/Systems/PlayerSystem.cpp
    src/Components/Player.h
)

target_include_directories(CrashTheSim PRIVATE
    ${CMAKE_SOURCE_DIR}/core/src/include
)
target_link_libraries(CrashTheSim PRIVATE Synthwave)

# Output compiled DLL 
set_target_properties(CrashTheSim PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/games/CrashTheSim
)