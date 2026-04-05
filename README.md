# OpenSplat 🚀

**OpenSplat** is a high-performance, all-in-one C++ solution for **3D Gaussian Splatting**. It integrates native COLMAP Structure-from-Motion (SfM) pipelines, a custom CUDA-accelerated training engine, and a real-time interactive OpenGL viewport.

Designed to replace disjointed Python/PyTorch workflows, OpenSplat provides a monolithic, portable, and extremely fast environment for creating and editing 3D Gaussian scenes from raw imagery.

---

## ✨ Key Features

- **Integrated COLMAP Pipeline**: Automated SfM workflow (Feature Extraction, Matching, Mapping) with live progress tracking and stdout parsing.
- **Real-time 3D Viewport**: High-performance OpenGL-based renderer with dynamic point sizing and turbo depth colormapping.
- **Blender-Style Controls**: Familiar navigation using Middle Mouse (Orbit), Shift + Middle Mouse (Pan), and Scroll (Zoom).
- **Scene Framing**: Instant bounding-box calculation and view centering with the `F` key.
- **Lock-Free Data Streaming**: Asynchronous background data transfer from SfM/Training threads to the render loop via a lock-free SPSC queue.
- **Modern UI**: Clean ImGui-based editor with floating overlays for camera settings, gizmos, and pipeline status.
- **Cross-Platform CMake Build**: Easy to build and deploy on Windows and Linux (coming soon).

---

## 🛠 Tech Stack

- **Core**: C++17
- **Graphics**: OpenGL 3.3+, GLFW, GLM
- **UI**: Dear ImGui
- **SFM**: COLMAP (Native Process Integration)
- **Acceleration**: CUDA (for upcoming Rasterizer and Training modules)
- **Logging**: spdlog

---

## 🚀 Getting Started

### Prerequisites

- **CMake** (v3.18+)
- **NVIDIA GPU** with CUDA Toolkit (for training features)
- **COLMAP** (must be in your PATH or configured in settings)
- **Visual Studio 2022** (Windows) or **GCC/Clang** (Linux)

### Build Instructions

1. Clone the repository:
   ```bash
   git clone https://github.com/yourusername/OpenSplat.git
   cd OpenSplat
   ```

2. Generate project files:
   ```bash
   cmake -B build
   ```

3. Build the project:
   ```bash
   cmake --build build --config Release
   ```

4. Run the executable:
   ```bash
   ./build/Release/OpenSplat.exe
   ```

---

## 🎮 Viewport Controls

| Input | Action |
| :--- | :--- |
| **Middle Mouse** | Orbit around target |
| **Shift + Middle Mouse** | Pan horizontally/vertically |
| **Mouse Scroll** | Zoom in / out |
| **F** | Frame Scene (Snap to points) |

---

## 🗺 Roadmap

- [x] Phase 1: Interactive Viewport & COLMAP Pipeline Integration
- [ ] Phase 2: Native CUDA-based Gaussian Training Engine
- [ ] Phase 3: Real-time Gaussian Splat Rasterizer Integration
- [ ] Phase 4: Advanced Scene Editing Tools (Transform, Clip, Filter)

---

## 📄 License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.
