# Ravl2 Debug Display — Design Document (SDL2 + bgfx + Dear ImGui)

## Purpose
Provide a lightweight, cross‑platform debug visualization module for 1D/2D/3D data with minimal intrusion into client code via `ioSave("@debug:Name", obj)`. The module auto‑initializes on first use, supports overlays, pixel queries with original values, dockable UI, and future features (plots, timelines, VR, audio).

## Goals and non‑goals
### Goals
- Cross‑platform: Linux, macOS, Windows.
- Display 2D textures, 3D meshes/point clouds, and simple 1D series.
- Minimal dependency footprint with strong future‑proofing.
- No special setup in `main()`; lazy init on first `ioSave`.
- Handle multiple named debug channels with overlays and `:Clear` control.
- Pixel query on 2D images returns original (unnormalized) values.
- Low idle cost: render on invalidation; dynamic framerate cap.
- Extensible type conversion registry (Ravl2 native types → renderables).

### Non‑goals (initial phase)
- Full scene graph or heavy editor features.
- High‑end photorealistic rendering. (Optional ray tracing later.)
- Complex persistence of UI layouts beyond basic docking save/restore.

## Technology choices
- Windowing/input/audio: SDL2
  - Built‑in cross‑platform audio, robust input, controller support, future web build potential.
- Rendering abstraction: bgfx
  - Future‑proof across D3D11/12, Metal, Vulkan, GL/GLES.
- GUI: Dear ImGui (docking) + ImPlot
  - Excellent developer UX for debug tools; docking for multi‑view layout; ImPlot for plots/timelines.
- Optional VR: OpenXR SDK
  - Device‑agnostic VR interface; integrates with bgfx via native swapchain interop.
- Math: Eigen (CPU‑side math); simple float4x4 helpers for GPU submissions.
- Logging: spdlog.
- Time: `std::chrono`.
- Error transport: `std::expected` in new code.

## High‑level architecture
### Public API surface
- Continue to use `ioSave("@debug:Name[:Control]", obj)`.
- Parse `@debug` scheme, extract `Name` (channel id) and optional controls such as `:Clear`, `:Hold`, `:Normalize=...`.

### DisplayManager (lazy singleton)
- Responsibilities:
  - Start GUI thread on first debug `ioSave`.
  - Initialize SDL2 window, bgfx context, ImGui/ImPlot.
  - Manage event loop with `SDL_WaitEventTimeout` for low idle CPU/GPU.
  - Maintain channel registry and render scheduling.
- Lifecycle:
  - Lazy create on first `ioSave` via `std::call_once`.
  - Stop cleanly at process exit (`std::atexit`) or when last window closes.

### Channel model
- A channel corresponds to a named view in the docked UI.
- Each channel has:
  - Base layer (e.g., image/mesh) and zero or more overlay layers.
  - State (zoom/pan for 2D, camera for 3D, normalization policy, tooltip settings).
- Channels are shown as dockable ImGui windows under a single main application window.

### Type Conversion Registry
- Map source C++ types (`std::type_index`) → converter functions that create typed `IRenderCommand` instances.
- Examples (converters produce commands):
  - `Array<uint8_t,2>`, `Array<float,2>` → `SetBaseImage2D` (CPU copy retained for pixel queries).
  - `std::vector<Point<float,2>>`, `std::vector<Line<float,2>>` → `AddOverlayPoints2D` / `AddOverlayLines2D`.
  - `Mesh3f`, `PointCloud3f` → `SetMesh3D` / `SetPointCloud3D`.
- Registration happens in a `.cc` TU at module init time and is easily extended with new commands.

### Message path and threading
- `ioSave` enqueues a typed `IRenderCommand` into a multi‑producer, single‑consumer queue owned by `DisplayManager`.
- Each command carries its own context (e.g., `channel` and parameters) and contains only immutable CPU data.
- On the GUI thread, commands are applied to per‑channel state. Commands create/update persistent `ISceneNode` objects (which own GPU resources) inside the channel.
- Use `std::jthread` + `std::stop_token` for the GUI thread. Protect maps with `std::mutex`. All GPU work occurs on the GUI thread; producer threads never touch GPU resources.

### Rendering pipeline (per frame)
1. Wait events with `SDL_WaitEventTimeout(≈33 ms)`.
2. Drain message queue; apply updates.
3. If `needsRender` or heartbeat:
   - Build ImGui docking space and per‑channel windows.
   - For each channel:
     - 2D: submit textured quad with zoom/pan transform via bgfx; overlays via ImGui draw lists or a bgfx line/point pass.
     - 3D: submit camera + draw calls for meshes/points; optional axes/grid overlay.
   - Render ImGui via bgfx; present frame.
4. Else, continue waiting to keep idle usage low.

### Pixel query (2D)
- Convert mouse → image coordinates via current view transform.
- Sample from CPU copy (avoid GPU readbacks).
- Tooltip displays: original value (float), displayed value (post normalization), coordinates.

### Normalization policies
- Auto: per‑frame min/max.
- Fixed: user‑specified [min, max].
- Percentile: e.g., 1–99% clamp from histogram.
- Policy is per‑channel; changeable via UI or `:Normalize=...` tokens.

### Audio (future use)
- `SDL_OpenAudioDevice` with queueing mode for debug playback.
- Optional per‑channel audio view for 1D signals; link to timeline/plots.

### VR (stretch)
- OpenXR to acquire swapchains; render per‑eye via bgfx using XR poses.
- Mirror a debug view into the main window.

## Public interfaces (sketch)
```cpp
// Debug display entrypoint; typically called by IO layer when seeing @debug scheme.
namespace Ravl2::DebugDisplay {
  struct InitOptions {
    int maxFps = 60;
    bool startHidden = false;
  };

  void ensureStarted(const InitOptions& opts = {}); // lazy init

  // Enqueue a typed render command (thread-safe). The command carries its own context (e.g., channel).
  std::expected<void, std::string> enqueue(
      std::unique_ptr<IRenderCommand> command);
}
```

```cpp
// Command executed on the GUI thread to mutate channel state / scene graph.
struct IRenderCommand {
  virtual ~IRenderCommand() = default;
  virtual void apply(ChannelRegistry& channels) = 0;
};

// Persistent scene node (owns GPU resources; lives in a Channel).
struct ISceneNode {
  virtual ~ISceneNode() = default;
  virtual void prepare(RenderContext& ctx) = 0; // upload/update GPU resources
  virtual void render(RenderContext& ctx) = 0;  // submit draw calls
  // Optional capabilities for future features
  virtual bool hitTest(float x, float y, HitResult& out) { return false; }
  virtual void onUi() {}
};
```

```cpp
// Examples: a command + its node
struct SetBaseImage2D : IRenderCommand {
  std::shared_ptr<const Image2Df> image; // CPU copy for queries; created by converter
  MessageFlags flags{MessageFlags::None};
  void apply(ChannelRegistry& channels) override; // creates/updates Image2DNode in the channel
};

struct Image2DNode : ISceneNode {
  std::shared_ptr<const Image2Df> cpuImage; // used for pixel queries
  bgfx::TextureHandle gpuTex{BGFX_INVALID_HANDLE};
  Normalization policy;
  ZoomPan view;
  void prepare(RenderContext& ctx) override;
  void render(RenderContext& ctx) override;
};
```

## Data structures
- Commands queue
  - `std::unique_ptr<IRenderCommand>` items where each command carries its own context (e.g., `channel`) and parameters.
  - No generic flags or meta map; use explicit command types (e.g., `ClearChannelCommand`) or fields on concrete commands.

- `ChannelState`
  - `std::string name;`
  - Scene graph root (e.g., `std::vector<std::unique_ptr<ISceneNode>>` or `GroupNode`).
  - 2D view: `float zoom; ImVec2 pan;` active texture/node references.
  - 3D view: camera params; GPU buffers owned by nodes.
  - Overlays: nodes or subtrees with points/lines/shapes.
  - Normalization policy/config.

## Concurrency and performance
- GUI thread owns bgfx and ImGui; all GPU resource creation happens there.
- Producers (`ioSave`) only create typed commands with CPU data and enqueue messages; avoid heavy work on producer threads.
- Idle behavior: `SDL_WaitEventTimeout(33)` plus render‑on‑invalidations; optional heartbeat every ~500 ms to keep UI responsive.
- Large data: share immutable buffers; consider ring buffers for animations; cap per‑channel queue depth with drop‑oldest policy.

## Error handling and logging
- Use `std::expected<void, Error>` for enqueue/conversion errors back to the IO layer when appropriate.
- spdlog levels:
  - INFO: viewer started, channel created, backend chosen
  - WARN: adapter missing for type, dropping frame, queue overflow
  - ERROR: bgfx init failure, SDL device open failure
- Avoid duplicate logging across layers.

## Configuration
- Optional JSON under `~/.config/ravl2/debug_display.json`:
  - Backend preference (auto/D3D/Vulkan/Metal/GL), max FPS, default normalization, initial window size, docking layout enable/restore.
- UI controls mirror settings and persist on exit.

## CMake and directory structure
### Targets
- `ravl2_debug_display` (library)
  - Depends: SDL2, bgfx, Dear ImGui, ImPlot, Eigen, spdlog
  - PRIVATE links to third‑party; PUBLIC minimal headers for adapters/enqueue API
- `ravl2_debug_display_example` (demo executable, optional)

### Layout
```
src/Ravl2/Display/
  DebugDisplay.hh            // public enqueue/start APIs
  DebugDisplay.cc            // singleton, thread, event loop
  Channel.hh/cc              // ChannelState, per‑view state
  Adapters/
    AdapterRegistry.hh/cc
    Image2DAdapter.hh/cc
    Points2DAdapter.hh/cc
    Mesh3DAdapter.hh/cc
  UI/
    DockSpace.cc             // main dockspace
    ChannelWindows.cc        // per‑channel UI logic
  Backends/
    BGFXContext.hh/cc        // platformData hookup, resize, frame
    ImGuiLayer.hh/cc         // imgui_impl_sdl2 + imgui_impl_bgfx
  Shaders/                   // bgfx shaders for textured quad, lines, points
```

## Implementation plan (phased)
1. Foundations
   - Select dependency acquisition (vcpkg/Conan/submodules). Define CMake targets and options. Implement `DebugDisplay::ensureStarted()` and GUI thread with SDL2 window + bgfx init. Add spdlog logs.
2. ImGui docking + event loop
   - Integrate `imgui_impl_sdl2` and an ImGui renderer for bgfx. Create a main window with docking space. Add `SDL_WaitEventTimeout` and render‑on‑invalidations.
3. Message bus and channels
   - Implement `DisplayMessage`, queue, `enqueue()` API. Channel registry map with create/find/reset; implement `:Clear` handling.
4. 2D image path (MVP)
   - Adapter for `Array<uint8_t,2>` and `Array<float,2>`: CPU copy + normalized GPU texture; textured quad shader; zoom/pan; pixel query tooltip with original + displayed values; normalization policy UI.
5. Overlays (2D)
   - Adapters for `std::vector<Point<float,2>>` and lines; render via ImGui draw lists or a bgfx line pass; color/size controls.
6. 3D basic rendering
   - Camera orbit controls; adapters for `PointCloud3f` and `Mesh3f`. Submit via bgfx with depth test; axes/grid overlay.
7. ImPlot integration
   - Plots panel. 1D series debug channel and a basic timeline. Waterfall as a scrolling texture prototype.
8. Persistence and config
   - Save/restore docking layout and per‑channel settings. JSON config support; environment overrides.
9. Audio (optional)
   - Minimal audio viewer/playback using SDL audio device for PCM streams.
10. VR (stretch)
   - OpenXR for HMD detection and stereo rendering; mirror view in main window.
11. QA and tests
   - Unit tests for adapters and normalization; headless smoke tests (where possible) and CI builds on all platforms.

## Testing strategy
- Unit tests (Catch2):
  - Normalization functions (auto, fixed, percentile) with edge cases (NaNs, inf, constant images).
  - Adapter registry selection logic.
  - Message enqueue/dequeue backpressure and overflow policy.
- Manual/demos:
  - Example apps pushing images, overlays, 3D points; verify pixel queries and performance under rapid updates.
- CI:
  - Build matrix across Linux/macOS/Windows; warnings as errors; clang‑tidy basic checks.

## Risks and mitigations
- macOS GL deprecation → use bgfx with Metal backend by default on macOS.
- Event loop ownership → dedicated GUI thread; do not require changes to client `main()`.
- GPU resource lifetime/races → create/destroy strictly on GUI thread; CPU copies via `shared_ptr<const T>`.
- Queue growth under bursty producers → bounded queue with drop‑oldest per channel; log WARN on drops.
- ImGui + bgfx backend maintenance → pin known‑good versions via submodules or vcpkg lockfile.

## Deliverables (MVP)
- `ravl2_debug_display` library with documented public headers.
- Minimal shaders and adapters for 2D float/uint8 images and 2D point overlays.
- Dockable UI with per‑channel windows, pixel query tooltip, normalization controls.
- Example app demonstrating 2D/3D and overlay usage.

## Open questions / decisions
- Dependency management: vcpkg vs Conan vs submodules?
- Initial backend policy per OS (e.g., Metal on macOS, Vulkan on Linux where available, D3D11/12 on Windows).
- Default normalization (auto vs fixed) and histogram binning cost for percentile.
- Per‑channel queue sizing and drop policy thresholds.



## `@debug` command sink via TypeConverter (implemented)
The `@debug:` output now accepts `std::shared_ptr<IRenderCommand>` as the canonical sink type. The IO layer uses the `TypeConverter` registry to convert supported Ravl2 types into render commands.

### URL controls (current prototype)
Controls are appended after the channel name, separated by `:`. Multiple controls may be combined.

- `:Clear` — clears the channel before applying the command.
- `:Norm=Auto` or `:Normalize=Auto` — use auto normalization (min/max) for float images.
- `:Norm=Fixed=min,max` — use fixed normalization range.
- `:Norm=Pct:low,high` or `:Normalize=Percentile:low,high` — compute percentiles on the float image and use the resulting range.

Examples:
- `@debug:Image:Clear:Norm=Auto`
- `@debug:Image:Norm=Fixed=0.0,255.0`
- `@debug:Image:Normalize=Percentile:1,99`

A `SetNormalization2D` command is enqueued before the primary command when a normalization control is present. The URL's channel is authoritative and will be applied to `SetBaseImage2D` if needed.

### Interaction (SDL MVP)
- Pan: left-drag inside the image view updates `ChannelState::view2D.translation()`.
- Zoom: mouse wheel zooms around the cursor; `ChannelState::view2D.scaleVector()` is clamped to [0.05, 32].
- Pixel query: the window title displays `(x,y)` along with original and displayed values under the mouse.

- OutputFormat: a generic sink recognizes `@debug:<Channel>[:Control...]` URLs.
  - It applies URL controls like `:Clear` by enqueuing a small `ClearChannelCommand` before the main command.
  - It enforces the channel from the URL on known commands (e.g., sets `SetBaseImage2D::channel`).
  - It enqueues the resulting command via `DebugDisplay::enqueue`.
- Conversions currently registered:
  - `Array<uint8_t,2>` → `std::shared_ptr<IRenderCommand>` building `SetBaseImage2D` (u8).
  - `Array<float,2>` → `std::shared_ptr<IRenderCommand>` building `SetBaseImage2D` (f32); auto normalization (min/max) is computed in `Image2DNode`.
- Backward compatibility:
  - Legacy per-type `@debug` adapters have been removed in the prototype; the command sink is the authoritative path going forward.
- Separation of concerns:
  - View state (pan/zoom) lives in `ChannelState::view2D` using `ScaleTranslate<float,2>`; scene nodes do not own view transforms.
