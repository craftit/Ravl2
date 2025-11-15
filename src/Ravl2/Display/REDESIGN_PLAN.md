# Display Module Redesign Plan

## Overview

This document outlines the plan to redesign the Display module's image node architecture to support arbitrary pixel types with flexible display formatting. The current design is limited to single-channel U8 and F32 images. The new design will support multi-channel images, integer types, label maps, and custom pixel types while maintaining type safety and clean abstractions.

## Motivation

The Display module is used for debugging diverse data types:
- **Label images** - Integer IDs representing semantic classes ("person", "car", "background")
- **Tracking IDs** - Integer identifiers for tracked objects
- **Statistics** - Arbitrary numeric or categorical data
- **Multi-channel data** - RGB images, multi-spectral data
- **Measurement data** - Float arrays with specific units and ranges

The current design has these limitations:
1. Only supports `U8` and `F32` single-channel formats
2. Hard-coded storage as `std::vector<uint8_t>` or `std::vector<float>`
3. Pixel queries return numeric values only (no semantic interpretation)
4. No support for RGB or other multi-channel formats
5. Adding new pixel types requires modifying `Image2DNode` internals

## Design Goals

1. **Support arbitrary pixel types** - Any type that can be stored in `Array<T,2>` should be displayable
2. **Type-safe storage** - Preserve original typed data without type erasure
3. **Flexible formatting** - Each pixel type controls how its values are displayed (numeric, labels, custom strings)
4. **Clean abstractions** - Hide template complexity behind polymorphic base class
5. **Extensible** - Add new pixel types without modifying existing code
6. **Maintain performance** - Keep original values in CPU memory for fast pixel queries
7. **Backward compatible** - Existing U8/F32 code paths continue to work

## Proposed Architecture

### Unified Scene Graph

All renderable content implements `ISceneNode`, enabling a unified scene graph architecture:

```
ISceneNode (interface)
  ├─ prepare(RenderContext)
  ├─ render(RenderContext)
  ├─ queryPixelInfo(x, y) → PixelQueryResult
  └─ supportsPixelQuery() → bool
    ↓
    ├─── Image2DNodeBase (abstract base)
    │      ├─ width, height (dimensions)
    │      ├─ textureHandleIdx (GPU texture handle)
    │      ├─ gpuDirty (upload flag)
    │      ├─ prepare(RenderContext) [final] - calls uploadToGPU()
    │      ├─ render(RenderContext) [final] - shared rendering
    │      ├─ supportsPixelQuery() → true [final]
    │      ├─ queryPixelInfo(x, y) [pure virtual]
    │      └─ uploadToGPU() [pure virtual]
    │        ↓
    │      Image2DNode<T> (templated derived)
    │        ├─ std::vector<T> pixelData (typed storage)
    │        ├─ queryPixelInfo(x, y) [override] - type-specific formatting
    │        ├─ uploadToGPU() [override] - type-specific GPU upload
    │        └─ setData(vector<T>, width, height)
    │
    ├─── Overlay2DNode (2D overlay base)
    │      ├─ render(RenderContext) - ImGui draw list rendering
    │      └─ Points2DNode, Lines2DNode, etc.
    │
    ├─── Viewport3DNode (3D scene viewport)
    │      └─ Contains camera, 3D scene nodes
    │
    └─── CompositeNode (scene graph composition)
           ├─ children: vector<unique_ptr<ISceneNode>>
           └─ Renders children in order (base + overlays)
```

### Simplified ChannelState

```cpp
struct ChannelState {
  std::string name;
  ViewMode viewMode = ViewMode::View2D;  // View2D or View3D

  // Single scene content node - either 2D or 3D based on viewMode
  // For 2D: typically a CompositeNode containing Image2DNode + Overlay2DNodes
  // For 3D: typically a Viewport3DNode
  std::unique_ptr<ISceneNode> sceneContent;

  // View transforms and settings
  ScaleTranslate<float, 2> view2D = ScaleTranslate<float, 2>::identity();
  NormalizationSettings norm = {};

  Flags flags;  // Runtime UI flags
};
```

**Design rationale:**
- **Single `sceneContent` field**: A channel displays either 2D or 3D content, not both simultaneously
- **Unified interface**: All renderable content (images, overlays, 3D viewports) implements `ISceneNode`
- **Composition**: Use `CompositeNode` to layer multiple nodes (base image + overlays)
- **Simplicity**: No separate storage for overlays or view-specific nodes

### Key Interfaces

#### ISceneNode Extension

```cpp
struct PixelQueryResult {
  bool valid;                              // Hit test success
  std::string coordinateText;              // "x: 123, y: 456"
  std::string valueText;                   // Type-specific formatted value
  std::optional<std::string> extraInfo;    // Optional additional details
};

struct ISceneNode {
  virtual ~ISceneNode() = default;
  virtual void prepare(RenderContext&) {}
  virtual void render(RenderContext&) {}

  // New methods for pixel inspection
  virtual bool supportsPixelQuery() const { return false; }
  virtual PixelQueryResult queryPixelInfo(int x, int y) const {
    return {.valid = false};
  }
};
```

**Design rationale:**
- Default implementation returns "not supported" for non-image nodes
- 2D image nodes override to provide pixel data
- 3D nodes, overlays, etc. gracefully decline queries

#### Image2DNodeBase

```cpp
class Image2DNodeBase : public ISceneNode {
protected:
  // Common properties
  int width = 0;
  int height = 0;
  uint16_t textureHandleIdx = UINT16_MAX;
  uint16_t texWidth = 0, texHeight = 0;
  bool gpuDirty = true;

public:
  virtual ~Image2DNodeBase();

  // Final implementations (shared across all image types)
  void prepare(RenderContext& ctx) final;
  void render(RenderContext& ctx) final;
  bool supportsPixelQuery() const final { return true; }

  // Pure virtual - subclasses must implement
  virtual PixelQueryResult queryPixelInfo(int x, int y) const = 0;
  virtual void uploadToGPU() = 0;

  // Accessors
  int getWidth() const { return width; }
  int getHeight() const { return height; }
  uint16_t getTextureHandle() const { return textureHandleIdx; }
};
```

**Design rationale:**
- `prepare()` is final - ensures consistent GPU resource management
- `uploadToGPU()` is virtual - each pixel type handles its own format conversion
- Common texture handle management
- Polymorphic storage in `ChannelState::sceneContent`

#### CompositeNode

```cpp
class CompositeNode : public ISceneNode {
private:
  std::vector<std::unique_ptr<ISceneNode>> children;

public:
  void addChild(std::unique_ptr<ISceneNode> node) {
    children.push_back(std::move(node));
  }

  void prepare(RenderContext& ctx) override {
    for (auto& child : children) {
      child->prepare(ctx);
    }
  }

  void render(RenderContext& ctx) override {
    for (auto& child : children) {
      child->render(ctx);
    }
  }

  // Query forwarded to first child that supports it (typically base image)
  bool supportsPixelQuery() const override {
    for (const auto& child : children) {
      if (child->supportsPixelQuery()) return true;
    }
    return false;
  }

  PixelQueryResult queryPixelInfo(int x, int y) const override {
    for (const auto& child : children) {
      if (child->supportsPixelQuery()) {
        return child->queryPixelInfo(x, y);
      }
    }
    return {.valid = false};
  }

  // Access for managing children
  const std::vector<std::unique_ptr<ISceneNode>>& getChildren() const { return children; }
  void clear() { children.clear(); }
};
```

**Design rationale:**
- Enables layering base image + overlays in a single scene graph
- Renders children in order (base image first, then overlays on top)
- Pixel queries delegated to first supporting child (base image)
- Simple, composable architecture

#### Overlay2DNode

```cpp
class Overlay2DNode : public ISceneNode {
protected:
  // Overlays render using ImGui draw lists
  virtual void renderOverlay(ImDrawList* drawList,
                            const SDL_FPoint& origin,
                            const ScaleTranslate<float,2>& view,
                            int imgW, int imgH) = 0;

public:
  void render(RenderContext& ctx) override {
    // Extract ImGui draw list and view info from context
    // Call renderOverlay() with appropriate parameters
  }

  // Overlays don't support pixel queries
  bool supportsPixelQuery() const final { return false; }
};

class Points2DNode : public Overlay2DNode {
  std::vector<SDL_FPoint> points;  // image-space points
  uint32_t rgba = 0xff00ffffu;     // default magenta
  float radius = 2.0f;             // radius in screen pixels

  void renderOverlay(...) override {
    // Existing Points2DOverlay::render() logic
  }
};

class Lines2DNode : public Overlay2DNode {
  std::vector<SDL_FPoint> vertices;  // image-space
  uint32_t rgba = 0xff00ff00u;       // default green
  float thickness = 1.0f;            // line thickness
  bool closed = false;               // draw closing segment

  void renderOverlay(...) override {
    // Existing Lines2DOverlay::render() logic
  }
};
```

**Design rationale:**
- Overlays are now `ISceneNode` types, enabling unified scene graph
- Migration path: move logic from `OverlayRenderer2D` to `Overlay2DNode`
- Can be composed with images using `CompositeNode`
- Same rendering behavior, cleaner architecture

#### Image2DNode<T> Template

```cpp
template<typename T>
class Image2DNode : public Image2DNodeBase {
private:
  std::vector<T> pixelData;  // Original typed data preserved

protected:
  // Type-specific formatting (can be specialized)
  virtual std::string formatValue(const T& value) const;
  virtual std::string formatExtra(const T& value) const;

public:
  // Set image data
  void setData(std::vector<T> data, int w, int h) {
    pixelData = std::move(data);
    width = w;
    height = h;
    gpuDirty = true;
  }

  // Query interface implementation
  PixelQueryResult queryPixelInfo(int x, int y) const override {
    if (x < 0 || x >= width || y < 0 || y >= height) {
      return {.valid = false};
    }

    const T& value = pixelData[y * width + x];
    return {
      .valid = true,
      .coordinateText = std::format("x: {}, y: {}", x, y),
      .valueText = formatValue(value),
      .extraInfo = formatExtra(value)
    };
  }

  // GPU upload - type-specific conversion
  void uploadToGPU() override;

  // Direct access for advanced use cases
  const std::vector<T>& getData() const { return pixelData; }
  const T& samplePixel(int x, int y) const {
    return pixelData[y * width + x];
  }
};
```

**Design rationale:**
- Type-safe storage with `std::vector<T>`
- `formatValue()` can be specialized per type for custom display
- `uploadToGPU()` specialization handles type-specific GPU format conversion
- Original values always available for inspection

## Type-Specific Implementations

### Template Specializations for Common Types

#### Single-Channel Grayscale (uint8_t)

```cpp
template<>
class Image2DNode<uint8_t> : public Image2DNodeBase {
  // Standard implementation with formatValue specialization
  std::string formatValue(const uint8_t& value) const override {
    return std::format("Value: {}", value);
  }

  void uploadToGPU() override {
    // Convert to RGBA8: replicate grayscale to RGB, alpha=255
    // (Current implementation approach)
  }
};
```

#### Single-Channel Float

```cpp
template<>
class Image2DNode<float> : public Image2DNodeBase {
private:
  NormalizationSettings norm;
  float cachedMin = 0.0f;
  float cachedMax = 1.0f;

  std::string formatValue(const float& value) const override {
    float displayValue = applyNormalization(value);
    return std::format("Raw: {:.3f}, Display: {:.3f}", value, displayValue);
  }

  std::string formatExtra(const float& value) const override {
    return std::format("Range: [{:.3f}, {:.3f}]", cachedMin, cachedMax);
  }

  void uploadToGPU() override {
    // Normalize float → uint8, then RGBA8 texture
  }

public:
  void setNormalization(const NormalizationSettings& settings);
};
```

#### RGB Images (PixelRGB8)

```cpp
template<>
class Image2DNode<PixelRGB8> : public Image2DNodeBase {
  std::string formatValue(const PixelRGB8& value) const override {
    return std::format("RGB: ({}, {}, {})",
                       value.red(), value.green(), value.blue());
  }

  void uploadToGPU() override {
    // Direct upload as RGBA8 (add alpha=255)
  }
};
```

#### Label Images (int32_t with lookup)

```cpp
template<>
class Image2DNode<int32_t> : public Image2DNodeBase {
private:
  std::optional<std::unordered_map<int32_t, std::string>> labelMap;

  std::string formatValue(const int32_t& value) const override {
    if (labelMap && labelMap->count(value)) {
      return std::format("{} (ID: {})", labelMap->at(value), value);
    }
    return std::format("ID: {}", value);
  }

  void uploadToGPU() override {
    // Convert label IDs to colormap for visualization
    // or encode as texture for GPU-based coloring
  }

public:
  void setLabelMap(std::unordered_map<int32_t, std::string> map) {
    labelMap = std::move(map);
  }
};
```

## Command Pattern Updates

### Templated SetBaseImage2D Command

```cpp
template<typename T>
struct SetBaseImage2D : public IRenderCommand {
  std::string channel;
  int width;
  int height;
  std::vector<T> data;  // Typed data

  SetBaseImage2D(std::string ch, int w, int h, std::vector<T> d)
    : channel(std::move(ch)), width(w), height(h), data(std::move(d)) {}

  void apply(ChannelRegistry& channels) override {
    auto& ch = channels.getOrCreateChannel(channel);

    // Get or create the image node
    Image2DNode<T>* imageNode = nullptr;

    // Check if sceneContent is already an Image2DNode<T>
    if (ch.sceneContent && typeid(*ch.sceneContent) == typeid(Image2DNode<T>)) {
      imageNode = static_cast<Image2DNode<T>*>(ch.sceneContent.get());
    }
    // Check if sceneContent is a CompositeNode with Image2DNode<T> as first child
    else if (auto* composite = dynamic_cast<CompositeNode*>(ch.sceneContent.get())) {
      if (!composite->getChildren().empty()) {
        auto* firstChild = composite->getChildren()[0].get();
        if (typeid(*firstChild) == typeid(Image2DNode<T>)) {
          imageNode = static_cast<Image2DNode<T>*>(firstChild);
        }
      }
    }

    // Create new node if needed or type changed
    if (!imageNode) {
      auto newImage = std::make_unique<Image2DNode<T>>();
      imageNode = newImage.get();

      // If there's existing content with overlays, preserve them in a composite
      if (auto* composite = dynamic_cast<CompositeNode*>(ch.sceneContent.get())) {
        composite->getChildren()[0] = std::move(newImage);
      } else {
        ch.sceneContent = std::move(newImage);
      }
    }

    imageNode->setData(std::move(data), width, height);
    SPDLOG_INFO("Updated image {}x{} on channel '{}'", width, height, channel);
  }
};
```

**Design rationale:**
- Command carries typed data from IO thread
- Type safety maintained end-to-end
- Automatic node recreation if pixel type changes

### TypeConverter Registration

```cpp
// In RenderCommandSink.cc

// Register uint8_t grayscale
registerConverter<Array<uint8_t, 2>>(
  [](const Array<uint8_t, 2>& arr, const std::string& channel) {
    int height = arr.range()[0].size();
    int width = arr.range()[1].size();
    std::vector<uint8_t> data = arrayToVector(arr);
    return std::make_shared<SetBaseImage2D<uint8_t>>(
      channel, width, height, std::move(data)
    );
  }
);

// Register float grayscale
registerConverter<Array<float, 2>>(...);

// Register RGB images
registerConverter<Array<PixelRGB<uint8_t>, 2>>(...);

// Register label images
registerConverter<Array<int32_t, 2>>(...);

// Register int16 images
registerConverter<Array<int16_t, 2>>(...);
```

## Pixel Inspector Updates

### Updated PixelInspector2D

```cpp
class PixelInspector2D {
public:
  // Query through ISceneNode interface
  std::optional<PixelQueryResult> inspect(
    const SDL_FPoint& mousePos,
    const std::string& channelName,
    ChannelRegistry& channels,
    const std::unordered_map<std::string, SDL_FRect>& contentRects,
    const std::unordered_map<std::string, SDL_FPoint>& imageOrigins
  ) {
    auto* ch = channels.findChannel(channelName);
    if (!ch || !ch->baseImage2D) return std::nullopt;

    auto* node = ch->baseImage2D.get();
    if (!node->supportsPixelQuery()) return std::nullopt;

    // Transform mouse to image coordinates
    auto [ix, iy] = transformMouseToImage(
      mousePos, ch->view2D, contentRects.at(channelName)
    );

    // Query through interface
    auto result = node->queryPixelInfo(ix, iy);
    return result.valid ? std::optional(result) : std::nullopt;
  }
};
```

**Design rationale:**
- No knowledge of concrete node types
- Works with any `ISceneNode` that supports queries
- Type-specific formatting handled by nodes

### ImGui Tooltip Display

Replace current window title display with proper tooltip:

```cpp
// In ChannelWindows::build()
if (ImGui::IsItemHovered()) {
  auto queryResult = pixelInspector.inspect(...);
  if (queryResult) {
    ImGui::BeginTooltip();
    ImGui::Text("%s", queryResult->coordinateText.c_str());
    ImGui::Text("%s", queryResult->valueText.c_str());
    if (queryResult->extraInfo) {
      ImGui::Separator();
      ImGui::Text("%s", queryResult->extraInfo->c_str());
    }
    ImGui::EndTooltip();
  }
}
```

## Implementation Phases

### Status snapshot (2025-11-15)

This section summarizes current progress versus the phases defined below. File paths refer to `src/Ravl2/Display/...` unless otherwise stated.

- Phase 0 — CompositeNode + Unified ChannelState: DONE
  - Implemented `CompositeNode` with forwarded pixel-query: `CompositeNode.hh`
  - Unified `ChannelState` with single `sceneContent` and `ViewMode`: `Channel.hh`
  - UI renders via `sceneContent->prepare/render`: `Ui/ChannelWindows.cc`
  - Overlay command composes overlays: `Commands/AddPolylineOverlay2D.cc`

- Phase 1 — Image2D class hierarchy: DONE
  - `Image2DNodeBase` with final `prepare/render` and common fields: `Image2DNodeBase.hh/.cc`
  - Templated `Image2DNode<T>` plus specializations for `uint8_t`, `float`, `PixelRGB8`: `Image2DNode.hh/.cc`
  - SetBaseImage2D templated command wired via sink: `Commands/SetBaseImage2D.hh`, `Adapters/RenderCommandSink.cc`

- Phase 2 — Query interface: MOSTLY DONE
  - `ISceneNode` exposes `PixelQueryResult`, `supportsPixelQuery`, `queryPixelInfo`: `ISceneNode.hh`
  - Implemented per-type formatting and queries in image nodes: `Image2DNode.hh/.cc`
  - ImGui tooltip on hover using interface: `Ui/ChannelWindows.cc`
  - Remaining cleanup: retire legacy `PixelInspector2D::inspect(...)` dynamic-cast path; standardize callers on interface or keep only `inspectWithQuery(...)`.

- Phase 3 — RGB support: DONE (verify at runtime)
  - `Image2DNode<PixelRGB8>` specialization present; converter registered: `Image2DNode.hh`, `Adapters/RenderCommandSink.cc`
  - GPU upload path indicated; confirm visually with a sample RGB image.

- Phase 4 — Overlays as ISceneNode: PARTIAL
  - `Polyline2DNode` implemented as `ISceneNode`: `Overlays/Polyline2DNode.hh/.cc`
  - Composition with base image via `CompositeNode`: `Commands/AddPolylineOverlay2D.cc`
  - Remaining: introduce `Overlay2DNode` base; add `Points2DNode`, `Lines2DNode`; migrate any remaining overlay renderers.

- Phase 5 — Integer type support: NOT STARTED
  - Add `Image2DNode<int16_t>` and `Image2DNode<int32_t>` with normalization-to-U8 visualization.
  - Add optional label map on `int32_t` specialization and an API/command to set it.
  - Register converters for `Array<int16_t,2>` and `Array<int32_t,2>` in `RenderCommandSink.cc`.

- Phase 6 — Advanced features: NOT STARTED
  - Per-channel normalization for float RGB; histogram display; value statistics; copy-to-clipboard, etc.

Additionally requested (2025-11-15):
- Headless mode for tests and an API to disable the display window. See new section “Headless Mode & Testability”.

### Phase 0: Introduce CompositeNode and Unified ChannelState
**Goal:** Unify scene graph architecture first

1. Create `CompositeNode` class implementing `ISceneNode`
2. Update `ChannelState` to use single `sceneContent` field
3. Remove separate `baseImage2D`, `viewport3D`, and `overlays` fields
4. Update rendering code to work with unified `sceneContent`
5. Migrate existing overlay storage to use `CompositeNode` when overlays present
6. Verify existing 2D rendering still works

**Files to create:**
- `CompositeNode.hh/.cc` - New composite scene node

**Files to modify:**
- `Channel.hh` - Unify to single `sceneContent` field
- `Ui/ChannelWindows.cc` - Render via `sceneContent` interface
- Commands that manage overlays - Use `CompositeNode`

Status: Completed
- Implemented `CompositeNode` (`CompositeNode.hh`) and unified `ChannelState` (`Channel.hh`).
- UI and overlay commands use `sceneContent` and compose overlays (`Ui/ChannelWindows.cc`, `Commands/AddPolylineOverlay2D.cc`).

### Phase 1: Introduce Image2D Class Hierarchy
**Goal:** Refactor image nodes without breaking functionality

1. Create `Image2DNodeBase` abstract class
2. Extract common properties (width, height, texture handle)
3. Move current `Image2DNode` to `Image2DNode<uint8_t>` specialization
4. Create `Image2DNode<float>` specialization from existing F32 path
5. Update commands to work with templated nodes
6. Verify existing code still works

**Files to modify:**
- `ISceneNode.hh` - Add query methods
- `Image2DNode.hh` - Split into base + template
- `Image2DNode.cc` - Split implementation
- `Commands/SetBaseImage2D.hh/.cc` - Template command

Status: Completed
- `Image2DNodeBase` and templated/specialized `Image2DNode<T>` implemented (`Image2DNodeBase.hh/.cc`, `Image2DNode.hh/.cc`).
- SetBaseImage2D templated command created and integrated via sink (`Commands/SetBaseImage2D.hh`, `Adapters/RenderCommandSink.cc`).

### Phase 2: Add Query Interface
**Goal:** Enable pixel inspection through interface

1. Add `queryPixelInfo()` to `ISceneNode`
2. Implement in `Image2DNode<uint8_t>` and `Image2DNode<float>`
3. Add `PixelQueryResult` struct
4. Update `PixelInspector2D` to use interface method
5. Implement ImGui tooltip display
6. Remove window title pixel display

**Files to modify:**
- `ISceneNode.hh` - Add query interface
- `Image2DNode.hh` - Implement query
- `PixelInspector2D.hh/.cc` - Use interface
- `Ui/ChannelWindows.cc` - Add tooltip rendering

Status: Mostly completed
- Interface is in `ISceneNode.hh`; `Image2DNode<T>` specializations implement `queryPixelInfo`.
- Tooltip implemented inline in `Ui/ChannelWindows.cc` when image is hovered.
- Remaining: remove/deprecate legacy `PixelInspector2D::inspect(...)` dynamic-cast path and standardize on interface; keep `inspectWithQuery(...)` as needed.

### Phase 3: Add RGB Support
**Goal:** Support multi-channel color images

1. Create `Image2DNode<PixelRGB8>` specialization
2. Implement RGB-specific `formatValue()`
3. Implement RGB-specific `uploadToGPU()` (RGBA8 texture)
4. Register TypeConverter for `Array<PixelRGB<uint8_t>, 2>`
5. Test with RGB images

**Files to create/modify:**
- `Image2DNode.cc` - Add RGB specialization
- `Adapters/RenderCommandSink.cc` - Register converter
- Test with `doDisplay` using RGB images

Status: Completed (pending runtime verification)
- `Image2DNode<PixelRGB8>` present; converter registered in `RenderCommandSink.cc`.
- Verify visually with a small RGB test image.

### Phase 4: Migrate Overlays to ISceneNode
**Goal:** Convert overlays to unified scene node architecture

1. Create `Overlay2DNode` base class implementing `ISceneNode`
2. Create `Points2DNode` and `Lines2DNode` derived classes
3. Move rendering logic from `OverlayRenderer2D` subclasses
4. Update overlay commands to create `Overlay2DNode` instances
5. Use `CompositeNode` to combine images + overlays
6. Deprecate `OverlayRenderer2D` interface

**Files to create:**
- `Overlays/Overlay2DNode.hh/.cc` - Base overlay node
- `Overlays/Points2DNode.hh/.cc` - Points overlay as ISceneNode
- `Overlays/Lines2DNode.hh/.cc` - Lines overlay as ISceneNode

**Files to modify:**
- Commands that add overlays - Create nodes instead of renderers
- Remove `ChannelState::overlays` vector (now use CompositeNode)

Status: Partial
- `Overlays/Polyline2DNode` exists and is used by `AddPolylineOverlay2D`.
- Remaining: add `Overlay2DNode` base; add `Points2DNode`, `Lines2DNode`; migrate any remaining overlay types.

### Phase 5: Add Integer Type Support
**Goal:** Support int16, int32 for labels/IDs

1. Create `Image2DNode<int16_t>` specialization
2. Create `Image2DNode<int32_t>` specialization
3. Implement integer-specific `uploadToGPU()` (colormap or encoded texture)
4. Register TypeConverters for integer array types
5. Add label map support to int32_t specialization
6. Add label map command or URL parameter

**Files to create/modify:**
- `Image2DNode.cc` - Add integer specializations
- `Commands/SetLabelMap2D.hh/.cc` - New command (optional)
- `Adapters/RenderCommandSink.cc` - Register converters

Status: Not started
- To implement: `Image2DNode<int16_t>` and `Image2DNode<int32_t>` with simple normalization to U8 display (per your note: “scale like HDR”).
- Add optional label map on `int32_t` specialization and API/command for setting it.
- Register converters for `Array<int16_t,2>` and `Array<int32_t,2>`.

### Phase 6: Advanced Features
**Goal:** Polish and enhance usability

1. Multi-channel selection UI (view R/G/B separately)
2. Per-channel normalization for float RGB
3. Custom colormap support for label visualization
4. Histogram display for numeric types
5. Value statistics (min/max/mean) display
6. Copy pixel value to clipboard

**Files to modify:**
- `Ui/ControlsPanel.cc` - Add channel selection
- `Image2DNode.hh` - Add colormap support
- `Ui/Plots.cc` - Add histogram display

Status: Not started
- Leave for later after integer types and overlay base are complete.

## Headless Mode & Testability

Unit tests for Display must run in environments without opening a window. Provide an API to disable window creation and UI rendering so tests can exercise data paths (commands, node queries, normalization, converters) without graphics.

### Requirements
- Tests run in CI/console without an SDL window, bgfx context, or ImGui frame loop.
- Optionally still support pixel-query/formatting unit tests through `Image2DNode<T>::queryPixelInfo(...)` without GPU.
- Maintain current behavior when headless mode is not enabled.

### Design
- Add a global/process-level switch to run the display in headless mode.
  - Expose as an API call (e.g., `DebugDisplay::setHeadless(bool on)`), default OFF.
  - Optionally support environment variable `RAVL2_HEADLESS=1` to force headless in CI.
- When headless:
  - Skip SDL window creation and bgfx initialization.
  - Skip ImGui setup and UI rendering paths.
  - Guard GPU-only fields/paths behind `RAVL2_WITH_BGFX` and headless checks.
  - Allow constructing nodes and applying render commands so channel state can be validated.
- Ensure image nodes’ CPU-side methods (`setData`, `queryPixelInfo`, normalization helpers) work without GPU.

### Implementation tasks
1. API toggle
   - Add `DebugDisplay::setHeadless(bool)` and `DebugDisplay::isHeadless()`.
   - In `Adapters/RenderCommandSink.cc`/Display startup, respect the flag and short-circuit window creation.
2. Conditional rendering
   - In `Ui/ChannelWindows.cc`, early-out render path if headless, but still run per-channel bookkeeping needed by tests if applicable.
   - Ensure `RenderContext` can be default-constructed and safely used without bgfx/imgui in headless.
3. GPU guards
   - Audit `Image2DNodeBase::prepare/render` and specializations to no-op when headless or when `RAVL2_WITH_BGFX` is off.
4. Tests
   - Add unit tests that:
     - Construct channels and push `SetBaseImage2D_*` commands for `uint8_t`, `float`, `PixelRGB8` (and later `int16_t`, `int32_t`).
     - Validate node types, dimensions, cached min/max, and `queryPixelInfo` formatting.
     - Run under headless mode to ensure no window is opened.

### Notes
- Current code already guards many GPU calls behind `RAVL2_WITH_BGFX` and keeps CPU arrays in nodes. This lowers the cost of adding headless mode.
- The inline tooltip in `ChannelWindows.cc` is already optional (depends on ImGui hover state); headless mode should skip UI entirely.

## Migration Strategy

### Backward Compatibility

All existing code continues to work:
- `Array<uint8_t, 2>` → `Image2DNode<uint8_t>`
- `Array<float, 2>` → `Image2DNode<float>`
- Existing TypeConverters remain functional
- GPU upload logic migrated to specializations

### API Stability

Public interfaces remain stable:
- `ioSave("display://Channel", array)` unchanged
- URL parameters (`:Clear`, `:Norm=`) unchanged
- Channel registry interface unchanged
- Commands remain copyable/movable

### Testing Strategy

1. **Unit tests** for each `Image2DNode<T>` specialization
2. **Integration tests** for TypeConverter → Command → Node pipeline
3. **Visual tests** using `doDisplay` with various image types
4. **Performance tests** for pixel query overhead

## Open Questions

### 1. GPU Upload for Arbitrary Integer Types

**Question:** How to visualize int16/int32 images on GPU?

**Options:**
- **A. Normalize to [0,255] like floats** - Simple, loses precision in display
- **B. Colormap lookup on CPU, upload RGB** - Flexible, higher memory
- **C. Encode in texture + fragment shader colormap** - Efficient, complex

**Recommendation:** Start with A, add C later for performance.

By default scale them like they were HDR image, we can add a option to introduce false colours later.

### 2. Label Map Management

**Question:** How do users provide label maps for semantic images?

**Options:**
- **A. Separate API call** - `setLabelMap("display://Channel", map)`
- **B. URL parameter** - `display://Channel:Labels=path/to/json`
- **C. Command chaining** - Second command after image

**Recommendation:** Start with A (programmatic), add B for convenience.

A is fine for now.  Being able to save the mapping in the same was as a overlay maybe the best overall.

### 3. Multi-Channel Float Images

**Question:** Support `Array<PixelRGB<float>, 2>` with per-channel normalization?

**Options:**
- **A. Single normalization** across all channels
- **B. Per-channel normalization** (independent min/max)
- **C. User-selectable**

**Recommendation:** A initially, add C with UI controls.

For pixel types assume the range for floats is 0 to 1.   Normalisation is used for processing data. 

### 4. Custom Pixel Types

**Question:** How do users define formatting for custom types?

**Options:**
- **A. Template specialization** - Requires recompilation
- **B. Format callback registration** - Runtime, more complex
- **C. ToString() method requirement** - Simple, limited

**Recommendation:** A for now (debug system, recompilation acceptable).

An overloaded 'toString' method is used in quite a bit of the code already.  

## Benefits Summary

**For Users:**
- Display any `Array<T,2>` type out-of-the-box
- Semantic labels show as strings, not numbers
- RGB images display correctly
- Hover inspection shows meaningful information
- Custom pixel types supported via specialization

**For Developers:**
- Type-safe storage and processing
- Clean abstractions hide complexity
- Easy to add new pixel types
- Template specialization for custom behavior
- Backward compatible migration path

**For Performance:**
- Original values in CPU memory (fast queries)
- No GPU readback required
- Efficient type-specific GPU upload
- Zero overhead for unused types (templates)

## References

- Current implementation: `src/Ravl2/Display/Image2DNode.{hh,cc}`
- Design document: `DebugDisplay_Design.md`
- Requirements: `Requirements.md`
- Type system: `src/Ravl2/Pixel/Pixel.hh`
- IO adapters: `src/Ravl2/Display/Adapters/RenderCommandSink.cc`
