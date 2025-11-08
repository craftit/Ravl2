# Ravl2 Debug Display — Implementation Checklist

This checklist tracks progress across the phased implementation plan. Update as tasks are completed.

Legend: [ ] = todo, [*] = in progress, [x] = done

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
- [ ] Initialize bgfx with backend policy (Vulkan default, per-platform fallbacks)
- [ ] Integrate Dear ImGui (docking) and set up dockspace
- [x] Idle-friendly loop using `SDL_WaitEventTimeout` and render-on-invalidation

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
- [ ] Textured quad shader (bgfx) — pending bgfx integration
- [x] Zoom/pan controls (SDL MVP; to be moved to ImGui later)
- [x] Pixel query tooltip (original + displayed values) — window title MVP

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
