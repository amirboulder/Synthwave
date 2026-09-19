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
#include <mutex>

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



//Flecs 
#include <flecs.h>


//Jolt
#include <Jolt/Jolt.h>





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



//meshoptimizer
#include <meshoptimizer.h>


//magic_enum
#include <magic_enum/magic_enum.hpp>

//Optick 
//#include "optick.h"