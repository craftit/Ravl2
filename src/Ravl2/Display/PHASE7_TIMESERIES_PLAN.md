# Phase 7 — Time Series Plots Implementation Plan

## Current Status

### ✅ Already Complete
- **ImPlot integrated** - Dependencies.cmake auto-downloads ImPlot v0.16 via CPM
- **Placeholder panel exists** - `Ui/Plots.{hh,cc}` provides dockable window (line 22-23 in CMakeLists.txt)
- **Architecture ready** - Command/Node pattern from Phase 4.9 supports clean integration
- **CMake configured** - ImPlot linked when `RAVL2_ENABLE_DISPLAY_STACK=ON`

### 🔄 Current State
- Plots panel shows placeholder text (`Ui/Plots.cc:29-32`)
- No time series data structures in `Channel.hh`
- No plot-related commands (`AddSeriesData`, `ClearPlot`, etc.)
- No type converters for 1D arrays → plot commands
- ImPlot context not initialized

## Implementation Roadmap

### 7a. ImPlot Context + Basic Line Plot MVP

**Goal:** Wire up ImPlot and render a simple static test plot

**Tasks:**
1. Initialize ImPlot context alongside ImGui in `DebugDisplay.cc`
   - Add `ImPlot::CreateContext()` after `ImGui::CreateContext()`
   - Add `ImPlot::DestroyContext()` in shutdown
2. Modify `Ui/Plots.cc::buildPlotsPanel()` to render test plot
   - Replace placeholder text with `ImPlot::BeginPlot()`
   - Render static line series with `ImPlot::PlotLine()`
   - Add legend, axes labels, basic styling
3. Verify docking, resizing, and pan/zoom work correctly

**Files to modify:**
- `src/Ravl2/Display/DebugDisplay.cc` (context lifecycle)
- `src/Ravl2/Display/Ui/Plots.cc` (rendering)
- `src/Ravl2/Display/Ui/Plots.hh` (optional: add helper declarations)

**Dependencies:** None (ImPlot already available)

**Acceptance:**
- ImPlot window displays with test sine wave
- Docking into main viewport works
- Mouse pan/zoom functional
- No crashes or resource leaks

---

### 7b. Data Structures and Commands

**Goal:** Add plot-specific channel state and render commands

**Tasks:**
1. Extend `Channel.hh` with plot state:
   ```cpp
   struct SeriesData {
     std::vector<float> x;
     std::vector<float> y;
     std::string label;
     // Optional: color, line style, markers
   };

   struct PlotState {
     std::unordered_map<std::string, SeriesData> series;
     std::string xAxisLabel = "X";
     std::string yAxisLabel = "Y";
     bool autoFitAxes = true;
     // Ring buffer config for streaming
     size_t maxHistoryPoints = 10000;
   };
   ```
2. Add `PlotState` member to `ChannelState`:
   ```cpp
   std::optional<PlotState> plotState; // Present when viewMode supports plots
   ```
3. Create `AddSeriesData` command:
   - Header: `src/Ravl2/Display/AddSeriesData.hh`
   - Implements `IRenderCommand::apply()`
   - Modes: `Replace` (clear + set), `Append` (add to end), `RingBuffer` (bounded)
4. Create `ClearPlot` command:
   - Clears all series or specific series by name
5. Create `SetPlotConfig` command:
   - Set axis labels, limits, styling

**Files to create/modify:**
- `src/Ravl2/Display/Channel.hh` (add `PlotState`)
- `src/Ravl2/Display/AddSeriesData.hh` (new command)
- `src/Ravl2/Display/ClearPlot.hh` (new command)
- `src/Ravl2/Display/SetPlotConfig.hh` (new command)
- `src/Ravl2/Display/CMakeLists.txt` (add new source files)

**Acceptance:**
- Commands compile and link
- Applying commands updates channel plot state
- Multiple series per channel supported

---

### 7c. PlotNode Scene Node

**Goal:** Implement `ISceneNode` for rendering plots with ImPlot

**Tasks:**
1. Create `PlotNode` class:
   - Header: `src/Ravl2/Display/PlotNode.hh`
   - Impl: `src/Ravl2/Display/PlotNode.cc`
   - Implements `ISceneNode::render(RenderContext&)`
2. Render logic:
   - Iterate over `PlotState::series` map
   - Call `ImPlot::PlotLine()` for each series
   - Handle auto-fit vs manual axis limits
   - Support legend, grid, tooltips
3. Integrate into `Ui/Plots.cc::buildPlotsPanel()`:
   - Enumerate channels with plot data
   - Create ImPlot sub-plots or tabs for each channel
   - Invoke `PlotNode::render()` per channel

**Files to create/modify:**
- `src/Ravl2/Display/PlotNode.hh` (new scene node)
- `src/Ravl2/Display/PlotNode.cc` (implementation)
- `src/Ravl2/Display/Ui/Plots.cc` (integrate rendering)
- `src/Ravl2/Display/CMakeLists.txt` (add sources)

**Acceptance:**
- Plot renders multiple series correctly
- Legend displays series names
- Pan/zoom works via ImPlot built-ins
- No GPU resource leaks

---

### 7d. Type Converters for 1D Data

**Goal:** Enable `ioSave(array, "display://")` for time series

**Tasks:**
1. Add type converters in `IOFormatAdapter.cc`:
   - `Array<float,1>` → `AddSeriesData` command
   - `std::vector<float>` → `AddSeriesData` command
   - Optional: `std::pair<std::vector<float>, std::vector<float>>` for explicit X,Y
2. Parse URL controls for plots:
   - `:Series=<name>` - Series identifier
   - `:Mode=Append|Replace|RingBuffer` - Update mode
   - `:XAxis=Auto|Index|Time` - X-axis generation strategy
   - `:ClearPlot` - Clear before adding
   - `:MaxPoints=<n>` - Ring buffer size
3. Implement auto X-axis generation:
   - `Auto`: Use vector index (0, 1, 2, ...)
   - `Index`: Same as Auto
   - `Time`: Use system clock or provided timestamps
4. Register converters with `TypeConverter` registry

**Files to modify:**
- `src/Ravl2/Display/IOFormatAdapter.cc` (add converters)

**Example usage:**
```cpp
std::vector<float> signal = {1.0, 2.5, 3.7, 2.1};
ioSave(signal, "display://:channel=sensor:series=temperature:mode=append");

Array<float,1> samples = /* ... */;
ioSave(samples, "display://:channel=scope:series=ch1:xaxis=time");
```

**Acceptance:**
- `ioSave()` of 1D arrays creates plot
- Append mode accumulates data over time
- Replace mode updates plot entirely
- URL controls parsed correctly

---

### 7e. Incremental Updates and Streaming

**Goal:** Support real-time streaming data with bounded history

**Tasks:**
1. Implement ring buffer mode in `AddSeriesData`:
   - When `mode=RingBuffer` and `maxHistoryPoints` exceeded:
   - Drop oldest points (circular buffer or `std::deque`)
   - Maintain consistent X/Y pairing
2. Add performance optimizations:
   - Avoid reallocation on every append (reserve capacity)
   - Consider `std::deque` for efficient front-pop
   - Option to decimate (downsample) on render if too many points
3. Add configuration API:
   - `SetPlotConfig` command to change `maxHistoryPoints`
   - Per-series or per-channel limit
4. Document thread-safety:
   - All updates go through command queue (already thread-safe)
   - Rendering only on GUI thread (already guaranteed)

**Files to modify:**
- `src/Ravl2/Display/AddSeriesData.hh` (ring buffer logic)
- `src/Ravl2/Display/Channel.hh` (document thread-safety)

**Acceptance:**
- Stream 10K points without memory growth
- Smooth real-time updates (60 FPS target)
- No data corruption or race conditions

---

### 7f. Timeline Widget (Optional — Advanced)

**Goal:** Synchronized playback scrubber for temporal data

**Tasks:**
1. Design `TimelineState`:
   ```cpp
   struct TimelineState {
     double currentTime = 0.0;
     double minTime = 0.0;
     double maxTime = 1.0;
     bool playing = false;
     double playbackSpeed = 1.0;
   };
   ```
2. Add timeline UI component (`Ui/Timeline.{hh,cc}`):
   - Scrubber slider with ImGui
   - Play/Pause/Reset buttons
   - Speed control (0.1x, 1x, 2x)
3. Link to 2D/3D channels:
   - Channels store frame timestamps
   - Scrubbing updates displayed frame
   - Commands: `SetFrameTime`, `LinkToTimeline`
4. Sync with plot vertical line:
   - ImPlot vertical drag line at `currentTime`
   - Callback updates timeline when line moved

**Files to create/modify:**
- `src/Ravl2/Display/TimelineState.hh` (new state)
- `src/Ravl2/Display/Ui/Timeline.{hh,cc}` (new UI component)
- `src/Ravl2/Display/Channel.hh` (add frame timestamp support)
- `src/Ravl2/Display/DebugDisplay.cc` (integrate timeline panel)

**Acceptance:**
- Timeline scrubber moves vertical line on plot
- Play button animates scrubber automatically
- 2D/3D channels update frame when scrubbing (if implemented)

---

### 7g. Waterfall Display (Optional — Stretch Goal)

**Goal:** 2D scrolling texture for frequency/time spectrograms

**Tasks:**
1. Create `WaterfallNode` scene node:
   - Treats each incoming 1D signal as a row
   - Accumulates rows into 2D texture (height = history)
   - Scrolls up (or circular buffer overwrite)
2. Upload to bgfx texture:
   - R8 or RGBA8 format
   - Update rows incrementally (`bgfx::updateTexture2D`)
3. Apply colormap:
   - CPU-side: map float → RGBA via lookup table
   - GPU-side (advanced): use palette texture + shader
   - Built-in colormaps: Grayscale, Viridis, Jet, Hot
4. Render as textured quad:
   - Reuse existing image shaders or create specialized version
   - Add colorbar legend UI
5. Command: `AddWaterfallRow`:
   - Channel name, 1D data, normalization policy

**Files to create/modify:**
- `src/Ravl2/Display/WaterfallNode.{hh,cc}` (new scene node)
- `src/Ravl2/Display/AddWaterfallRow.hh` (new command)
- `src/Ravl2/Display/Colormap.{hh,cc}` (colormap utilities)
- `src/Ravl2/Display/IOFormatAdapter.cc` (type converter)

**Example usage:**
```cpp
// FFT output: frequency bins over time
Array<float,1> spectrum = computeFFT(audioSamples);
ioSave(spectrum, "display://:channel=spectrogram:mode=waterfall:colormap=viridis");
```

**Acceptance:**
- Streaming FFT bins creates scrolling spectrogram
- Colormap visually distinguishes amplitude
- Smooth updates without flicker
- History depth configurable

---

## Implementation Phases Summary

| Phase | Description | Complexity | Priority |
|-------|-------------|------------|----------|
| 7a | ImPlot context + test plot | Low | **Must** |
| 7b | Data structures + commands | Medium | **Must** |
| 7c | PlotNode scene node | Medium | **Must** |
| 7d | Type converters for 1D data | Medium | **Must** |
| 7e | Incremental updates | Low | **Should** |
| 7f | Timeline widget | High | *Optional* |
| 7g | Waterfall display | High | *Optional* |

**Recommended order:** 7a → 7b → 7c → 7d → 7e → (7f, 7g as stretch goals)

---

## Key Design Patterns (from existing code)

### Command Pattern
```cpp
struct AddSeriesData : public IRenderCommand {
  std::string channel;
  std::string seriesName;
  std::vector<float> x, y;
  enum class Mode { Replace, Append, RingBuffer } mode;

  void apply(ChannelRegistry &channels) override {
    auto &ch = channels.getOrCreateChannel(channel);
    // Update ch.plotState
  }
};
```

### Scene Node Pattern
```cpp
struct PlotNode : public ISceneNode {
  void render(RenderContext &ctx) override {
    // Use ctx to access channel state
    // Call ImPlot::PlotLine() for each series
  }
};
```

### Type Converter Pattern
```cpp
// In IOFormatAdapter.cc
TypeConverter::registerConverter<Array<float,1>>(
  [](const Array<float,1> &data, const std::string &url) {
    auto cmd = std::make_shared<AddSeriesData>();
    cmd->channel = parseChannel(url);
    cmd->seriesName = parseSeries(url);
    cmd->y = std::vector<float>(data.begin(), data.end());
    // Generate X values based on :XAxis control
    enqueue(cmd);
  }
);
```

---

## Testing Strategy

### Unit Tests (non-GUI)
- Ring buffer logic (append, overflow, wrap)
- X-axis generation modes (auto, index, time)
- Command application to channel state
- Colormap value mapping

### Manual Integration Tests
1. Static plot display (hardcoded data)
2. Single series append mode (add points over time)
3. Multiple series on one plot
4. Ring buffer overflow behavior
5. URL control parsing
6. Docking and window management

### Example Test Program
```cpp
// examples/exTimeSeries.cc
int main() {
  // Generate sine and cosine waves
  for (int i = 0; i < 1000; ++i) {
    float t = i * 0.01f;
    float s = std::sin(2 * M_PI * t);
    float c = std::cos(2 * M_PI * t);

    std::vector<float> sineData = {s};
    std::vector<float> cosData = {c};

    ioSave(sineData, "display://:channel=trig:series=sine:mode=append");
    ioSave(cosData, "display://:channel=trig:series=cosine:mode=append");

    std::this_thread::sleep_for(std::chrono::milliseconds(16)); // ~60 FPS
  }
  return 0;
}
```

---

## Files to Create

### New Headers
- `src/Ravl2/Display/AddSeriesData.hh`
- `src/Ravl2/Display/ClearPlot.hh`
- `src/Ravl2/Display/SetPlotConfig.hh`
- `src/Ravl2/Display/PlotNode.hh`
- `src/Ravl2/Display/WaterfallNode.hh` (optional)
- `src/Ravl2/Display/Colormap.hh` (optional)
- `src/Ravl2/Display/TimelineState.hh` (optional)
- `src/Ravl2/Display/Ui/Timeline.hh` (optional)

### New Source Files
- `src/Ravl2/Display/PlotNode.cc`
- `src/Ravl2/Display/WaterfallNode.cc` (optional)
- `src/Ravl2/Display/Colormap.cc` (optional)
- `src/Ravl2/Display/Ui/Timeline.cc` (optional)

### Modified Files
- `src/Ravl2/Display/Channel.hh` (add `PlotState`)
- `src/Ravl2/Display/Ui/Plots.cc` (real ImPlot rendering)
- `src/Ravl2/Display/DebugDisplay.cc` (ImPlot context init)
- `src/Ravl2/Display/IOFormatAdapter.cc` (type converters)
- `src/Ravl2/Display/CMakeLists.txt` (add new sources)
- `src/Ravl2/Display/IMPLEMENTATION_CHECKLIST.md` (track progress)

---

## References

- **ImPlot Documentation:** https://github.com/epezent/implot
- **Existing Command Pattern:** `SetBaseImage2D.hh:16-47`
- **Existing Scene Node:** `Image2DNode.{hh,cc}`
- **Type Converter Registry:** `IOFormatAdapter.cc`
- **Phase 4.9 Refactor:** `IMPLEMENTATION_CHECKLIST.md:75-96`

---

## Notes

- **Thread Safety:** All commands go through bounded queue → GUI thread only. No additional locking needed.
- **Memory Management:** Use `std::vector` for series data; consider `std::deque` for ring buffer efficiency.
- **Performance:** ImPlot handles 100K+ points efficiently. Downsample only if needed (e.g., 1M+ points).
- **Styling:** ImPlot inherits ImGui styling. Use `ImPlot::PushStyleVar()` for custom colors/line widths.
- **Future:** OpenGL texture path for waterfall could be replaced with bgfx for consistency.

---

**Last Updated:** 2025-12-14
**Status:** Planning phase — ready for implementation
