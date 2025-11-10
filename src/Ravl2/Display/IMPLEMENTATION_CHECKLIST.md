# Ravl2 Debug Display — Implementation Checklist

This checklist tracks progress across the phased implementation plan. Update as tasks are completed.

Legend: [ ] = todo, [*] = in progress, [x] = done

## Build notes (local)
- Use the preconfigured Ninja build directory at the project root: `cmake-build-debug`.
  - Build: `cmake --build cmake-build-debug`
  - Clean: `ninja -C cmake-build-debug clean`
  - Notes: This preset has all display stack options enabled (SDL2 + bgfx + ImGui), so no extra flags are needed.

## Phase 1 — Foundations
- [x] Design document added (`DebugDisplay_Design.md`)
- [x] Rename Message.hh → DisplayMessage.hh in design document
- [x] Public API header scaffold (`DebugDisplay.hh`)
- [x] Channel state placeholder (`Channel.hh`)
- [x] Minimal implementation stub (`DebugDisplay.cc`) with `ensureStarted` and `enqueue`
- [x] CMake target scaffold for `ravl2_debug_display` (conditional on `RAVL2_ENABLE_DISPLAY_STACK`)
- [x] Remove `DisplayMessage` struct (command-only queue) — keep header as deprecation stub

## Phase 2 — ImGui docking + event loop
- [x] Add SDL2 window creation (GUI thread via `std::jthread`)
- [x] Initialize bgfx with backend policy (Linux: Vulkan→OpenGL; Windows: D3D12→D3D11→OpenGL; macOS: Metal/SDL path)
- [*] Integrate Dear ImGui (docking) and set up dockspace — Step A: vendor backends and wire minimal frame
- [x] Idle-friendly loop using `SDL_WaitEventTimeout` and render-on-invalidation

### Phase 2 — Staged migration plan
- [x] Step A scope approved: Initialize bgfx + ImGui while keeping SDL renderer for first pixels
- [x] Step A deliverable: bgfx initialized against SDL window; ImGui dockspace visible; SDL still blits image
  - [x] Implement `BGFXContext` with real `init/resize/frame/shutdown` using `SDL_SysWMinfo`
  - [x] ImGui backends wired from `imgui` package (using SDL2 + SDL_Renderer backend for UI in Step A)
  - [x] CMake: link `imgui` and add backend sources; set `RAVL2_WITH_IMGUI`
  - [x] GUI thread: create ImGui context (docking enabled), init SDL2 + SDL_Renderer backends
  - [x] Per-frame: ImGui NewFrame → DockSpace → simple "Channels" window → Render via ImGui SDL_Renderer backend
  - [x] Window events: forward SDL input to ImGui backend; on resize call `BGFXContext::resize`
  - [x] First-frame: ensure non-zero backbuffer size before initial bgfx init (already handled for SDL)
- [*] Step B deliverable: move 2D image display to bgfx textures; remove SDL_Renderer path entirely
  - [x] Add bgfx `TextureHandle` to `Image2DNode` and lifetime management
  - [x] U8 path: upload to R8 texture (recreate on size change)
  - [x] F32 path: CPU normalize to U8 (policy: Auto/Fixed/Percentile) and upload to R8
  - [x] Draw in channel window via ImGui `Image` (bgfx texture ID)
  - [x] Apply pan/zoom using `ChannelState::view2D` (transform positions/UVs or use draw list)
  - [x] Remove SDL texture cache and `SDL_RenderCopy` usage; keep SDL only for window/events when bgfx is available
  - [x] Verify pixel query (uses CPU copy); display in ImGui status/tooltip (window title MVP)

## Phase 3 — Message bus and channels (Command/Node architecture)
- [x] Define `IRenderCommand` base (applied on GUI thread)
- [x] Define `ISceneNode` base (persistent per-channel, owns GPU resources)
- [x] Add `enqueue(std::shared_ptr<IRenderCommand>)` API (channel/controls embedded in commands)
- [x] Define bounded, thread-safe queue for render commands
- [x] Implement `enqueue()` push with backpressure / drop-oldest policy
- [x] Channel registry (create/find/reset) and parse `:Clear` control
- [x] Back-compat shim (optional): convert old payload-based enqueue to commands (temporary)
- [x] ioSave → `std::shared_ptr<IRenderCommand>` via `TypeConverter` and generic `@debug` OutputFormat (command sink)

## Phase 4 — 2D image path (MVP)
- [x] Command sink via TypeConverter for `Array<uint8_t,2]` → `SetBaseImage2D`
- [x] Command sink via TypeConverter for `Array<float,2]` → `SetBaseImage2D` with normalization (auto/fixed/percentile)
  - [x] Auto normalization (min/max) for display of f32
  - [x] Fixed range and percentile policies (via URL controls and per-channel state)
- [ ] Non‑ImGui bgfx renderer (textured quad) — in progress
  - [x] Shaders: add `vs_image.sc`, `fs_image.sc`, and `varying.def.sc` (pass‑through + R8 sampling)
  - [x] Build: compile shaders with bgfx `shaderc` via CMake custom target; output to `${CMAKE_CURRENT_BINARY_DIR}/shaders`; define `RAVL2_SHADER_DIR`
  - [x] Runtime: load shader binaries based on renderer (Vulkan→SPIR‑V, OpenGL→GLSL) and create `bgfx::Program`
  - [x] Geometry: create screen‑quad (transient VBO/IBO) and vertex layout (position + uv)
  - [x] Uniforms: create sampler uniform and bind uploaded R8 texture from `prepare()`
  - [*] Render: compute aspect‑fit from channel view (scale/translation) and submit quad each frame to view 0
  - [*] Resize: update view rect on window resize; recompute quad as needed
  - [*] Diagnostics: keep dbgText HUD (backend, frame) toggleable; log shader load/creation failures
  - [*] Fallback: if bgfx init or shader load fails, fall back to SDL renderer automatically
- [x] Zoom/pan controls (SDL MVP; to be moved to ImGui later)
- [x] Pixel query tooltip (original + displayed values) — window title MVP

## Phase 4.9 — DebugDisplay.cc structure review and refactor

- [ ] Execution order (do these in sequence; build and run after each step):
  1. [x] Snapshot current file size and hotspots (functions >150 lines; duplicated logic) — record in design notes
  2. [x] Refactor safety prep: introduce named `constexpr` for magic numbers (e.g., `kControlsInitialPos`), confirm logging pattern and error policy (RAII + `std::expected` in new code)
  3. [x] Extract minimal helpers without behavior change: `buildDockspace(...)`, `buildControlsUI(...)`, `buildChannelWindows(...)`, `updateWindowTitlePixelInfo(...)`
  4. [x] Split setup/teardown into dedicated functions; keep the main loop orchestration under ~100 lines
  5. [x] Introduce `SdlApp` wrapper (window/events); migrate lifecycle calls (no functional change)
  6. [x] Harden `BgfxContext` single‑responsibility; move ImGui+bgfx glue into `ImguiBgfxBridge`
  7. [x] Move UI composition into `Ui::Dockspace`, `Ui::ControlsPanel`, `Ui::ChannelWindows`; update call sites
  8. [x] Add `InputController2D` (per‑channel state) and route pan/zoom through it
  9. [x] Add `PixelInspector2D` and switch title/pixel info to use it (consults normalization)
  10. [x] Error handling and logging pass: convert new/refactored paths to `std::expected`; ensure boundary‑only logging and exception‑safe RAII
  11. [x] Prepare extension points: define `OverlayRenderer2D` interface and trivial stubs (points/lines); add small overlay registry hook in channel windows
  11.1 [ ] Define PolyLine overlay command and conversion pipeline (see Phase 5) — AddPolylineOverlay2D (Append/Replace/Closed/Color/Width).*
  11.2 [ ] Register TypeConverter: `PolyLine<float,2>` → `shared_ptr<IRenderCommand>` producing `AddPolylineOverlay2D`.*
  11.3 [ ] Extend `@debug` sink controls: `:Mode`, `:Color`, `:Width`, `:Closed`, `:ClearOverlays`; inject channel and hints; enqueue.*
  11.4 [ ] Ensure `Lines2DOverlay` supports `closed` and style; render closing segment when requested.✓
  11.5 [ ] Build + manual verify with two images and multiple overlays per channel.*
  12. [ ] Isolate ImPlot integration hooks so Phase 7 can add plots without touching the rendering core
  13. [ ] Tests and documentation: Doxygen for new helpers/classes; unit tests for normalization/percentile helpers; update `DebugDisplay_Design.md` with module diagram/data flow
  14. [ ] Acceptance criteria verification: size reduced; no regressions for Phases 2–4; overlay stubs exercised

- [ ] Partition responsibilities for maintainability:
  - [ ] Window/SDL lifecycle: `SdlApp` (init/shutdown, window events)
  - [ ] Rendering backends: `BgfxContext` (already exists) — ensure single-responsibility; move ImGui+bgfx glue into `ImguiBgfxBridge`
  - [ ] ImGui UI composition: `Ui::Dockspace`, `Ui::ControlsPanel`, `Ui::ChannelWindows`
  - [ ] Input handling for pan/zoom: `InputController2D` with per-channel state
  - [ ] Pixel query and normalization bridge: `PixelInspector2D` (CPU-side, consults normalization)
- [ ] Refactor `DebugDisplay.cc`:
  - [ ] Extract helpers: `buildDockspace(...)`, `buildControlsUI(...)`, `buildChannelWindows(...)`, `updateWindowTitlePixelInfo(...)`
  - [ ] Keep the main loop orchestration under ~100 lines; move setup/teardown into dedicated functions
  - [ ] Replace magic constants with named `constexpr` (e.g., `kControlsInitialPos`)
- [ ] Error handling and logging pass:
  - [ ] Prefer `std::expected` returns in new code paths; log at boundaries only (no duplicate logging)
  - [ ] Ensure RAII and early-returns keep initialization/shutdown exception-safe
- [ ] Prepare extension points for upcoming phases:
  - [ ] Define `OverlayRenderer2D` interface and stub implementations (points/lines) to support Phase 5
  - [ ] Ensure channel windows can register overlays via a small registry (composition over inheritance)
  - [ ] Isolate ImPlot integration hooks so Phase 7 can add plots without touching rendering core
- [ ] Tests and documentation:
  - [ ] Doxygen for new helpers/classes with thread-safety notes
  - [ ] Unit tests for normalization helpers (ties into Phase 11); quick tests for percentile math
  - [ ] Update `DebugDisplay_Design.md` with a module diagram and data flow
- [ ] Acceptance criteria:
  - [ ] `DebugDisplay.cc` reduced in size (e.g., main loop and per-frame UI functions < 100 lines each)
  - [ ] No behavior regressions for Phases 2–4 features (dockspace, channel windows, pixel query)
  - [ ] Overlay integration points exist and are exercised by trivial stubs

## Phase 5 — Overlays (2D)
- [ ] Adapter for `std::vector<Point<float,2>>`
- [ ] Adapter for 2D lines
- [ ] Render via ImGui draw lists or bgfx line pass (choose and document)

## Phase 6 — 3D basic rendering
- [ ] Camera orbit controls
- [ ] PointCloud3f and Mesh3f adapters
- [ ] Depth-tested draw calls; axes/grid overlay

## Phase 7 — ImPlot integration
- [ ] Plots panel and 1D series channel
- [ ] Basic timeline widget
- [ ] Waterfall prototype as scrolling texture

## Phase 8 — Persistence and config
- [ ] Save/restore docking layout
- [ ] JSON config (backend preference, FPS cap, normalization defaults)

## Phase 9 — Audio (optional)
- [ ] SDL2 audio device hookup; simple PCM playback viewer

## Phase 10 — VR (stretch)
- [ ] OpenXR integration; stereo rendering and mirror view

## Phase 11 — QA and tests
- [ ] Unit tests: normalization functions and edge cases
- [ ] Unit tests: adapter registry selection
- [ ] Stress tests: enqueue/dequeue backpressure and overflow

## Notes / Decisions
- Vulkan default with MoltenVK on macOS; fallbacks documented in Dependencies.cmake status messages.
- Display stack remains optional: enable with `-DRAVL2_ENABLE_DISPLAY_STACK=ON`.
- First-pixels MVP uses SDL_Renderer for 2D blit; to be replaced by bgfx + ImGui in later phases.
