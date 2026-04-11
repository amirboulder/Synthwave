# Synthwave

## Overview
Synthwave is a personal game engine and game project, built from scratch in C++20 as both 
a learning exercise and a long-term publishing goal.

The engine is in its early stages. The current focus is on building a functional scene editor —
a UI overlay that lets you place objects into a 3D scene and transform them with translate, 
rotate, and scale controls.


### Current state

- [x] Scene editor UI overlay (ImGui, docking layout)
- [x] Object placement in scene
- [ ] Translate / rotate / scale controls
- [ ] Asset browser
- [ ] Scene serialization / save & load ( working but mostly broken because I'm changing things so much)
- [x] In game menus
- [x] Shader Compilation module with shader reflection
- [ ]  Blinn Phong Lighting
- [ ]  Frustum Culling
- [ ]  Support for skinned meshes
- [ ]  Mesh deformation based on ragdolls
- [ ]  Ragdoll builder - WIP

## Requirements

### Build tools
- **CMake** ≥ 3.30
- A C++20-capable compiler with module support 


### System dependencies

- **Vulkan SDK** — must be installed and findable by CMake

### Dependencies (fetched automatically)
All libraries below are downloaded and built by CMake at configure time so no manual installation required.

| Library | Version | Purpose |
|---------|---------|---------|
| [SDL3](https://github.com/libsdl-org/SDL) | 3.4.0 | Cross-platform window creation, input handling, Graphics API abstraction |
| [SDL_image](https://github.com/libsdl-org/SDL_image) | 3.2.4 | Image loading |
| [SDL_ttf](https://github.com/libsdl-org/SDL_ttf) | 3.2.2 | Font rendering / FreeType |
| [Slang](https://github.com/shader-slang/slang) | 2026.2.1 | Shader language & compiler |
| [Assimp](https://github.com/assimp/assimp) | 5.4.0 | 3D asset importing (OBJ, glTF) |
| [GLM](https://github.com/g-truc/glm) | `bf71a83` | Math library |
| [Jolt Physics](https://github.com/jrouwe/JoltPhysics) | 5.5.0 | Physics simulation |
| [ImGui](https://github.com/ocornut/imgui) | 1.92.5-docking | Immediate-mode GUI |
| [Flecs](https://github.com/SanderMertens/flecs) | `8a126f0` | Entity Component System |
| [Optick](https://github.com/bombomby/optick) | 1.4.0.0 | CPU profiling |
| [inih](https://github.com/benhoyt/inih) | r61 | INI config file parsing |
| [RapidJSON](https://github.com/Tencent/rapidjson) | `24b5e7a` | JSON parsing |


> **Note:** Slang is downloaded as a prebuilt binary rather than compiled from source to because it takes a long time to compile. All other libraries are compiled from source.


## Building

### Prerequisites
Before building, ensure you have the following installed:

- [Visual Studio 2022](https://visualstudio.microsoft.com/) with the **Desktop development with C++** workload
- [CMake](https://cmake.org/download/) ≥ 3.30 (bundled with Visual Studio or install separately)

---

### Visual Studio

1. Clone the repository
```bash
   git clone https://github.com/your-org/synthwave.git
   cd synthwave
```

2. Open Visual Studio and select **File → Open → CMake...**

3. Navigate to the repo root and select `CMakeLists.txt`

4. Visual Studio will automatically configure the project and begin downloading dependencies via FetchContent — this may take several minutes on the first run

5. Select a configuration from the toolbar (`Debug`, `Release`)

6. **Build → Build All** (`Ctrl+Shift+B`)

The `Synthwave.exe` binary will be output to the `out/build/<config>/` directory alongside all required DLLs and assets.


### Notes

- **First build** is slow — CMake downloads and compiles all dependencies from source (except Slang, which is a prebuilt binary). Subsequent builds are faster.
- The `data/` and `config/` directories are created automatically in the build output folder.
- Shader files are copied from `core/src/renderer/shaders/slang/` to `build/shaders/slang/` at build time. Any changes to shaders do not require a full rebuild.
- If you add a new `.ixx` module file, re-save `CMakeLists.txt` to force CMake to pick it up.

---

### Platform support

Windows x86-64 for now, this project was created with multiplatform support in mind, however, as it is still in its early stages I have chosen to focus on one platform for now.