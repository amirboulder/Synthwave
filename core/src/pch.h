#pragma once 

// Standard Template library
#include <iostream>
#include <vector>
#include <iomanip> 
#include <string>
#include <thread>
#include <fstream>
#include <filesystem>
#include <queue>
#include <chrono>
#include <random>

using std::vector;
using std::cout;
using std::string;
namespace fs = std::filesystem;

///////////////////Third-party

//Vulkan
#include <vulkan/vulkan.h>

//SDL
#include <SDL3/SDL.h>
#include <SDL3/SDL_gpu.h>
#include <SDL3_image/SDL_image.h>
//#include <SDL3_ttf/SDL_ttf.h>

#include "SDL3/SDL_keyboard.h"
#include "SDL3/SDL_mouse.h"


//Flecs 
#include <flecs.h>


//Jolt
#include <Jolt/Jolt.h>


//fastgltf
#include "fastgltf/core.hpp"
#include "fastgltf/util.hpp"
#include "fastgltf/math.hpp"
#include "fastgltf/glm_element_traits.hpp"
#include "fastgltf/tools.hpp"

//stb_image
//#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"


//GLM
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/matrix_decompose.hpp>


//IMGUI
#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_sdlgpu3.h"
#include "imgui_freetype.h"


//rapidjson
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/istreamwrapper.h"

//xxhash
#include <xxhash.h>

//meshoptimizer
#include <meshoptimizer.h>

//INIReader
#include "INIReader.h"

//magic_enum
#include <magic_enum/magic_enum.hpp>

//Optick 
//#include "optick.h"