# Vulkan Template (C++)

A small, modular Vulkan 1.3 starting point in modern C++, built with CMake. It opens a window and renders a spinning, vertex-colored cube with a depth buffer — everything you need before writing your own rendering code.

<p align="center">
  <img src="docs/screenshot.png" width="600" alt="Spinning colored cube rendered by the template">
</p>

## Features

- **Vulkan 1.3**: dynamic rendering (no render pass or framebuffer objects) and synchronization2 barriers
- **Swapchain recreation** on resize and when out of date; waits while minimized
- **Two frames in flight** with per-frame command buffers and fences, per-image present semaphores
- **Depth buffer** with automatic format selection
- **GPU selection** that prefers a discrete GPU and checks swapchain, surface and Vulkan 1.3 support
- **Validation layers** in Debug builds, including synchronization validation, with warnings and errors routed through the app's logger and Vulkan objects labeled with debug names
- Mesh data uploaded through a staging buffer into GPU-only memory; per-frame uniform buffers stay persistently mapped
- Uniform buffer + descriptor set for model/view/projection matrices
- Camera, input (keyboard and mouse delta), timer and math helpers
- GLSL shaders compiled to SPIR-V for Vulkan 1.3 by the build, rebuilt when included files change, and loaded from beside the executable

## Requirements

- A C++17 compiler (GCC, Clang or MSVC)
- CMake 3.21 or newer
- Vulkan 1.3 capable GPU and driver
- Vulkan headers and loader, `glslc`, and (for Debug builds) the Khronos validation layer
- GLFW 3 and GLM

### Arch Linux

```bash
sudo pacman -S cmake ninja vulkan-headers vulkan-icd-loader vulkan-validation-layers shaderc glfw glm
```

### Ubuntu / Debian

```bash
sudo apt install cmake ninja-build libvulkan-dev vulkan-validationlayers glslc libglfw3-dev libglm-dev
```

Older releases may lack `glslc`; install the [LunarG Vulkan SDK](https://vulkan.lunarg.com/) instead.

### Windows

1. Install the [LunarG Vulkan SDK](https://vulkan.lunarg.com/) (provides headers, loader, `glslc` and validation layers).
2. Install [vcpkg](https://vcpkg.io/) and pass its toolchain when configuring: `-DCMAKE_TOOLCHAIN_FILE=<vcpkg-root>/scripts/buildsystems/vcpkg.cmake`. GLFW and GLM are then installed automatically from `vcpkg.json`.

## Build and run

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build
./build/VulkanTemplate
```

- **Debug** builds enable the validation layer (with synchronization validation) and debug names for Vulkan objects; **Release** (`-DCMAKE_BUILD_TYPE=Release`) disables both.
- The executable and its `shaders/` folder are placed in the build directory, so it can be launched from any working directory.
- On Windows, `cmake -S . -B build -G "Visual Studio 17 2022"` generates a solution with `VulkanTemplate` as the startup project (or open the folder directly in Visual Studio).
- On laptops with hybrid graphics the discrete GPU is chosen automatically; the selected GPU is printed at startup.

## Project structure

```
CMakeLists.txt        Build configuration (app name, sources, shader compilation)
shaders/              GLSL sources, compiled to SPIR-V at build time
src/
  main.cpp            Entry point
  VulkanContext       Owns everything; init, frame loop, swapchain recreation, cleanup
  VulkanWindow        GLFW window, surface, resize tracking
  VulkanInstance      Instance, validation layers, debug messenger
  VulkanDevice        Physical device selection, logical device, queues, debug names
  VulkanSwapChain     Swapchain images and views
  VulkanPipeline      Graphics pipeline and descriptor set layout
  VulkanCommand       Command buffers and per-frame recording (barriers + dynamic rendering)
  VulkanSync          Semaphores and fences
  VulkanBuffer        Buffer + memory helper
  VulkanImage         Image + memory + view helper (used for depth)
  Mesh, CubeMesh      Vertex/index buffers and the demo cube
  Camera, Input, Timer, Math, Utils, Paths, ShaderLoader
```

## Starting a new project

1. Rename the project: change `project(VulkanTemplate)` and `APP_TITLE` at the top of `CMakeLists.txt`.
2. Replace `CubeMesh` with your own geometry and edit the shaders in `shaders/` (add new ones to the `SHADERS` list in `CMakeLists.txt`).
3. Per-frame logic lives in `VulkanContext::drawFrame()`; draw commands are recorded in `VulkanCommand::recordCommandBuffer()`.

## Credits

Forked from [ragulnathMB/VulkanProjectTemplate](https://github.com/ragulnathMB/VulkanProjectTemplate).

## License

MIT License — see the [LICENSE](LICENSE) file.
