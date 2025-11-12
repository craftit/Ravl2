# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

RAVL2 (Recognition And Vision Library 2) is a computer vision library written in C++23. It's a modernized port of the original RAVL library with efficient interoperability with OpenCV and other vision libraries. The project is work in progress with incomplete implementations that may change significantly.

## Build Commands

### Standard Build
```bash
mkdir build
cd build
cmake ..
cmake --build .
```

### Build Types
- **Default**: Development build with address and undefined sanitizers enabled
- **Debug**: Standard debug build without sanitizers
- **Release**: Optimized build with LTO/IPO enabled
- **RelWithDebInfo**: Release with debug symbols

### Running Tests
```bash
cd build
ctest
```

### Running a Single Test
Tests use Catch2 framework. To run specific tests:
```bash
./build/test/tests "test name or [tag]"
```

### Building Examples
Examples are built by default. Disable with:
```bash
cmake -DRAVL2_BUILD_EXAMPLES=OFF ..
```

## Code Structure

The library is organized into modules under `src/Ravl2/`:

- **3D**: 3D geometry, pinhole cameras, triangle meshes, vertices
- **Display**: Debug visualization system with multiple backends (SDL2+bgfx+ImGui)
  - `Backends/`: Rendering backend implementations
  - `Commands/`: Display command handling
  - `Ui/`: User interface components
  - `3D/`: 3D scene rendering nodes
- **DLib**: Interoperability with DLib library
- **Geometry**: Affine transforms and geometric constructions
- **Image**: Image processing, segmentation, boundaries, region growing
- **IO**: File handling and input/output operations
- **Math**: Linear algebra, statistics, optimization algorithms
- **OpenCV**: Interoperability with OpenCV (primary external dependency)
- **OpenGL**: OpenGL rendering utilities
- **Optimise**: Optimization algorithms (Bayesian, particle swarm, random search)
- **Pixel**: Pixel types and colour space handling
- **Qt**: Qt framework interoperability (optional)
- **Video**: Video I/O using FFmpeg, frame handling, media containers

Core utilities at `src/Ravl2/`:
- `Array.hh`, `Array1.hh`: N-dimensional array types (thread-safe reference-counted)
- `Index.hh`, `IndexRange.hh`: Index and range types for array access
- `Configuration.hh`: JSON-based configuration system
- `Types.hh`: Common type definitions and utilities

## Array Architecture

The Array<T,N> type is fundamental to RAVL2:
- Last dimension is contiguous in memory for efficient access
- Thread-safe reference-counted handles to data
- Views can be reshaped without copying
- Support common operations: `fill()`, `copy()`, `clip()`, `clamp()`, `view()`, `access()`, `clone()`
- Image coordinates: origin at top-left, first dimension is vertical (rows), second is horizontal (columns)

## Key Design Patterns

### Common Functions
- `fill(x, value)` - Fill x with value
- `copy(x, y)` - Copy y to x (output first argument)
- `clip(x, y)` - Return array x clipped to range y
- `clamp(x, y)` - Return value x limited to range y
- `view(x)` - Return a view of x that can be reshaped
- `access(x)` - Return reference with broadcast semantics
- `clone(x)` - Return deep copy
- `inverse(x)` - Return inverse (usually std::optional<> for transforms/matrices)
- `fit(&x, y)` - Fit model x to data y

### Argument Order
Output parameters come first (e.g., `copy(output, input)`).

## Coding Conventions

### Language: C++23
- Use modern features: `std::span`, smart pointers, concepts
- No raw pointers - use `std::shared_ptr` or `std::unique_ptr`
- Source files: `.cc`, header files: `.hh`
- Use `#pragma once` instead of include guards
- RAII for resource management
- Avoid `using namespace ...` except for literals (e.g., `std::chrono_literals`)

### Naming
- Classes/namespaces: `PascalCase`
- Functions/variables: `camelCase`
- Member variables: prefix with `m` (e.g., `mPlanInterval`)
- Template arguments: suffix with `T` (e.g., `DataT`, `RealT`)

### Libraries
- **Logging**: `spdlog` with capitalized methods: `SPDLOG_INFO("Message {}", var)`
- **Math**: Eigen for vectors and matrices
- **Time**: `std::chrono`
- **Images**: RAVL2 types for image/file IO
- **JSON**: `nlohmann_json::json`
- **Formatting**: `fmt::format("{}", var)` - avoid `std::cout`/`std::cerr`
- **CLI**: CLI11 for command-line parsing
- **Serialization**: cereal
- **Testing**: Catch2

### Code Style
- Indentation: 2 spaces, no tabs
- Format with `.clang-format` in root directory
- Document with Doxygen: `//!` comments with `@` tags
- Include examples in comments if usage isn't clear
- Keep code out of headers unless templates or small inline methods
- Thread safety: Document which methods are thread-safe

### Testing
Unit tests in `test/` directory use Catch2. Write concise tests covering required functionality with minimal test cases.

## Dependencies

Required:
- CMake 3.21+
- C++23 compiler (Clang/GCC/MSVC)
- OpenCV 4.5+
- BLAS/LAPACK

Auto-downloaded if not found:
- cereal, spdlog, xtensor, xtensor-blas, catch2, fmt, nlohmann_json, CLI11, Eigen3

Optional (controlled by `RAVL2_MIN_DEPS` option):
- Qt5/Qt6 (for Qt interoperability)
- DLib (for DLib interoperability)
- SDL2, bgfx, ImGui (for Display stack - controlled by `RAVL2_ENABLE_DISPLAY_STACK`)

## Project Options

Key CMake options in `ProjectOptions.cmake`:
- `RAVL2_BUILD_EXAMPLES`: Build example programs (default ON)
- `RAVL2_ENABLE_CLANG_TIDY`: Static analysis (default ON)
- `RAVL2_ENABLE_CACHE`: Use ccache (default ON)
- `RAVL2_MIN_DEPS`: Minimal dependencies build (default OFF)
- `RAVL2_ENABLE_DISPLAY_STACK`: Enable SDL2+bgfx+ImGui display (default ON)

## Key Files

- `Dependencies.cmake`: Full dependency list and configuration
- `ProjectOptions.cmake`: Build options and compiler settings
- `.github/copilot-instructions.md`: Additional coding guidelines
- `.clang-format`: Code formatting rules
- `scripts/autoport_ravl.sh`: Script for porting from original RAVL library

## Licensing

Original RAVL code: LGPL
New code in this project: MIT
