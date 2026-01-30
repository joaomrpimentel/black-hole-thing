# Black Hole Shader Visualizer

A real-time, ray-traced black hole simulation running on the CPU/GPU using **C++**, **OpenGL**, and **GLSL**.

![Unreal Black Hole](black_hole_screenshot.png)

## Features
- **Physically Inspired Rendering**: Ray-marching with gravitational lensing (Schwarzschild metric).
- **Accretion Disk**: Volumetric-style rendering with noise textures and Doppler shifting.
- **Customization**: Real-time controls for Mass, Radius, Colors, and Glow via Dear ImGui.
- **High-Res Export**: Save 4K PNG screenshots directly to disk.

## Dependencies

This project uses a mix of **Vendored** libraries and **CMake FetchContent**.

### Vendored (Included in `vendor/`)
These are included in the repository, so you don't need to install them manually.
- **GLAD**: OpenGL 3.3 Core Profile loader (Source included to avoid CMake version issues).
- **stb_image_write**: for saving PNG screenshots.

### Fetched Automatically (by CMake)
The build system will automatically download and build these:
- **GLFW**: Window and Input management.
- **GLM**: Mathematics library.
- **Dear ImGui**: User Interface.

## Building

### Requirements
- CMake 3.16+
- C++17 Compiler (GCC, Clang, or MSVC)
- Linux: `xorg-dev`, `libglfw3-dev` (optional, CMake can fetch GLFW)

### Quick Start (Makefile)

```bash
# Build and Run
make run

# Just Build
make build
```

### Manual Build

```bash
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . -j$(nproc)
./BlackHoleThing
```

## Controls
- **Radius**: Size of the Event Horizon.
- **Glow**: Intensity of the photon ring/disk.
- **Disk Controls**: Adjust inner/outer radius, thickness, and colors.
- **Camera**: Orbit the black hole.
- **Export**: Generates a timestamped PNG in the project root.
