# Ravl2 Debug Display -- Status and Roadmap

## Build Notes

Use the preconfigured Ninja build directory at the project root: `cmake-build-debug`.
- Build: `cmake --build cmake-build-debug`
- Clean: `ninja -C cmake-build-debug clean`
- This preset has all display stack options enabled (SDL2 + bgfx + ImGui).

## Completed

### Core Architecture (Phases 1-3)
- Command/Node pipeline: `IRenderCommand`, `ISceneNode`, `ChannelRegistry`, `ChannelState`
- Thread-safe bounded queue with backpressure / drop-oldest policy
- `display://` URL scheme with `TypeConverter` integration (`IOFormatAdapter.cc`)
- Headless mode for testing (`setHeadless()` / `setHeadlessForTests()`)
- SDL2 window + bgfx backend (`BGFXContext`) + ImGui docking (`ImguiBgfxBridge`)

### 2D Image Display (Phase 4)
- Pixel types: `uint8_t`, `float`, `PixelRGB8`, `int16_t`, `int32_t`
- Normalization policies: Auto (min/max), Fixed (user range), Percentile
- Pan/zoom via `InputController2D`
- Pixel query tooltips with type-specific formatting
- bgfx texture upload displayed via `ImGui::Image`

### 2D Overlays (Phase 5 -- partial)
- `Polyline2DNode` with Append/Replace modes, color, width, closed
- `TypeConverter`: `PolyLine<float,2>` -> `AddPolylineOverlay2D`
- `CompositeNode` composition for image + overlays

### Time Series Plots (Phase 7a-7e)
- ImPlot integration with per-channel `PlotState`
- `AddSeriesData`: Replace / Append / RingBuffer modes
- Multi-series: `map<string,float>`, `map<string,vector<float>>`
- `ClearPlot`, `SetPlotXAxis`, followMode, auto-fit axes
- TypeConverters: `Array<float,1>`, `vector<float>`, maps

### UI Framework (Phase 4.9)
- ImGui docking: `Dockspace`, `ChannelWindows`, `ControlsPanel`, `StatusBar`, `Plots`
- `InputController2D` per-channel state for pan/zoom

### 3D Scaffolding (Phase 6a -- experimental, no rendering)
- `Viewport3DNode`: viewport rect and camera state management
- `OrbitCamera`: orbit/pan/dolly with view/projection matrix math
- `Grid3DOverlay`: CPU-projected XZ grid (always visible)
- `SetPointCloud3D`: command pipeline (stores data, does not render to screen)

## Remaining Work

### Phase 5 -- Additional 2D Overlay Adapters
- [ ] TypeConverter: `std::vector<Point<float,2>>` -> polyline overlay
- [ ] TypeConverter: 2D line segments

### Phase 4 -- Direct bgfx Renderer (optional; ImGui path works)
- [ ] Aspect-fit textured quad via bgfx draw (shaders ready: `vs_image.sc`, `fs_image.sc`)
- [ ] Resize handling for direct quad path
- [ ] Fallback: automatic switch if bgfx shader load fails

### Phase 6 -- 3D Rendering
- [ ] 6b: Depth test, axes triad, per-vertex color, point size
- [ ] 6c: Mesh renderer (PN) with MeshShapes smoke tests
- [ ] 6d: TypeConverter adapters for Ravl2/Eigen geometry types
- [ ] 6e: PinholeCamera interop, fit-to-bounds (AABB)
- [ ] 6f: 3D shaders (`vs_point3d`, `fs_point3d`, `vs_mesh3d`, `fs_mesh3d`)
- [ ] 6g: Unit tests for camera math, AABB, adapter validation

## Deferred / Stretch

- [ ] Phase 7f: Timeline widget (scrubber linked to 2D/3D frame display)
- [ ] Phase 7g: Waterfall display (2D texture accumulation with colormaps)
- [ ] Phase 8: Persistence (docking layout save/restore, JSON config)
- [ ] Phase 9: Audio (SDL2 PCM playback viewer)
- [ ] Phase 10: VR (OpenXR stereo rendering)
- [ ] Phase 11: QA stress tests (backpressure, overflow, adapter registry)
