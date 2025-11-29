Can # GoPro Metadata Integration Plan

## Overview

This document outlines the plan for integrating GoPro GPMF (GoPro Metadata Format) parsing capabilities into RAVL2. This will enable extraction of telemetry data (GPS, gyroscope, accelerometer, etc.) from GoPro video files alongside video and audio streams.

## Goals

1. Extract GoPro metadata (GPS, gyro, accelerometer, etc.) from GoPro video files
2. Integrate seamlessly with existing Video module and FFmpeg parser
3. Synchronize metadata streams with video/audio streams using timestamps
4. Make the feature optional (can be disabled via CMake)
5. Follow RAVL2 coding conventions and architecture patterns
6. Minimize external dependencies (auto-fetch gpmf-parser via CMake)

## Architecture Overview

### High-Level Design

```
┌─────────────────────────────────────────────────────────────┐
│                    MediaContainer                            │
│  (FfmpegMediaContainer - existing)                           │
│  - Detects GPMF data streams in video files                 │
│  - Creates iterators for video/audio/data streams           │
└────────────────┬────────────────────────────────────────────┘
                 │
                 ├─────────────────┬──────────────────┐
                 │                 │                  │
         ┌───────▼──────┐  ┌──────▼──────┐  ┌────────▼────────┐
         │   Video      │  │    Audio     │  │   GPMF Data     │
         │   Stream     │  │    Stream    │  │   Stream        │
         └──────────────┘  └──────────────┘  └─────────┬───────┘
                                                        │
                                          ┌─────────────▼────────────┐
                                          │  GoPro::GpmfParser       │
                                          │  - Parses GPMF data      │
                                          │  - Extracts telemetry    │
                                          └──────┬──────────────────┘
                                                 │
                     ┌───────────────────────────┼─────────────────────┐
                     │                           │                     │
              ┌──────▼──────┐          ┌────────▼────────┐   ┌────────▼────────┐
              │  GPS Frame  │          │  Gyro Frame     │   │  Accel Frame    │
              │  (lat/lon)  │          │  (x/y/z rates)  │   │  (x/y/z accel)  │
              └─────────────┘          └─────────────────┘   └─────────────────┘
```

### Integration Points

1. **FFmpeg Data Stream Extraction**: FFmpeg already supports extracting data streams from MP4 containers. GoPro stores GPMF data in a data track (often codec type `gpmd` or similar).

2. **GPMF Parser**: The `gpmf-parser` library will be used to decode the raw GPMF KLV data into structured telemetry.

3. **Frame Types**: New frame types extending `MetaDataFrame<T>` will represent different telemetry streams.

4. **Stream Registration**: Extend the Video module to recognize and handle GPMF data streams.

## Directory Structure

```
src/Ravl2/GoPro/
├── CMakeLists.txt              # Build configuration with optional support
├── GpmfParser.hh               # C++ wrapper for gpmf-parser library
├── GpmfParser.cc
├── GpmfFrame.hh                # Base class for GPMF frames
├── GpmfFrame.cc
├── GpsFrame.hh                 # GPS-specific frame type
├── GpsFrame.cc
├── GyroFrame.hh                # Gyroscope frame type
├── GyroFrame.cc
├── AccelFrame.hh               # Accelerometer frame type
├── AccelFrame.cc
├── GpmfStreamIterator.hh       # Iterator for GPMF data streams
├── GpmfStreamIterator.cc
├── GpmfTypes.hh                # Data structures for GPMF telemetry
├── GpmfTypes.cc
├── GpmfUtilities.hh            # Utility functions for telemetry processing
└── GpmfUtilities.cc            # (coordinate transforms, filtering, etc.)
```

## CMake Integration

### 1. Fetch gpmf-parser via CPM

Add to `Dependencies.cmake`:

```cmake
# GoPro GPMF parser (optional)
option(WITH_GPMF "Build with GoPro GPMF metadata support" ON)

if(WITH_GPMF)
  if(NOT TARGET gpmf-parser)
    CPMAddPackage(
      NAME gpmf-parser
      GITHUB_REPOSITORY gopro/gpmf-parser
      GIT_TAG master  # Or specific version/tag
      OPTIONS
        "BUILD_SHARED_LIBS OFF"
    )

    if(gpmf-parser_ADDED)
      # The library might need some adjustments for proper CMake integration
      # Create an alias for consistent naming
      add_library(gpmf::parser ALIAS GPMF_PARSER_LIB)
      message(STATUS "GoPro GPMF parser: Built from source")
      set(HAVE_GPMF 1)
    endif()
  else()
    message(STATUS "Found gpmf-parser")
    set(HAVE_GPMF 1)
  endif()
else()
  message(STATUS "GoPro GPMF support disabled")
endif()
```

### 2. GoPro Module CMakeLists.txt

```cmake
# src/Ravl2/GoPro/CMakeLists.txt

option(WITH_GPMF "Build with GoPro GPMF metadata support" ON)

if(NOT WITH_GPMF OR NOT HAVE_GPMF)
  message(STATUS "GoPro module disabled (GPMF parser not available)")
  return()
endif()

set(GOPRO_SOURCES
    GpmfTypes.cc GpmfTypes.hh
    GpmfParser.cc GpmfParser.hh
    GpmfFrame.cc GpmfFrame.hh
    GpsFrame.cc GpsFrame.hh
    GyroFrame.cc GyroFrame.hh
    AccelFrame.cc AccelFrame.hh
    GpmfStreamIterator.cc GpmfStreamIterator.hh
)

add_library(Ravl2GoPro ${GOPRO_SOURCES})
add_library(ravl2::Ravl2GoPro ALIAS Ravl2GoPro)

target_link_libraries(Ravl2GoPro
    PUBLIC
        ravl2::Ravl2Core
        ravl2::Ravl2Video
    PRIVATE
        RAVL2_options
        RAVL2_warnings
        gpmf::parser
)

target_include_directories(Ravl2GoPro
    PUBLIC
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src>
        $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/include>
)

# Installation
install(TARGETS Ravl2GoPro
    EXPORT Ravl2Targets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install(DIRECTORY ${PROJECT_SOURCE_DIR}/src/Ravl2/GoPro
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/Ravl2
    FILES_MATCHING PATTERN "*.hh"
)
```

### 3. Integrate into Main CMakeLists.txt

Add after Video module:

```cmake
# GoPro metadata support (optional)
if(WITH_GPMF AND HAVE_GPMF)
  add_subdirectory(src/Ravl2/GoPro)
endif()
```

## Data Structures

### Design Philosophy: RAVL2 Native Types

All telemetry data structures use RAVL2 native types (`GPSCoordinate`, `Vector3f`, `Quaternion`, etc.) rather than raw scalar fields. This provides several benefits:

1. **Seamless Integration**: Data can be directly used with RAVL2 geometry, math, and 3D modules
2. **Geodetic Accuracy**: `GPSCoordinate` provides proper Earth ellipsoid calculations (GRS84) instead of treating Earth as flat
3. **Consistent API**: Familiar interface for RAVL2 users
4. **Vector Operations**: Built-in support for vector math (dot product, cross product, norms)
5. **Type Safety**: Strong typing helps prevent dimension-related errors
6. **Future-Proof**: Easy to extend with additional RAVL2 functionality (transforms, filtering, etc.)

Key examples:
- GPS data uses `GPSCoordinate` for accurate geodetic distance calculations and Cartesian conversions
- Gyro data as `Vector3f` integrates directly with `Quaternion` for orientation tracking
- Accelerometer data as `Vector3f` enables gravity compensation and motion analysis
- All types support RAVL2's serialization (cereal)

### Core Telemetry Types

```cpp
// GpmfTypes.hh

#include "Ravl2/Geometry/GPSCoordinate.hh"

namespace Ravl2::GoPro
{
  //! GPS fix data from GoPro GPMF stream
  //! Uses RAVL2's GPSCoordinate which provides:
  //! - GRS84 ellipsoid calculations
  //! - Cartesian coordinate conversion
  //! - Distance calculations (great circle)
  //! - Error bounds tracking
  struct GpsFix
  {
    GPSCoordinate location;   //!< GPS position with latitude, longitude, height
    Point<float, 2> speed;    //!< 2D and 3D speed in m/s [speed2d, speed3d]
    int fix = 0;              //!< GPS fix type (0=no lock, 2=2D, 3=3D)
    int satellites = 0;       //!< Number of satellites
    float precision = 0;      //!< Dilution of precision (DOP)

    //! Default constructor
    GpsFix() = default;

    //! Construct with all parameters
    GpsFix(const GPSCoordinate& loc, const Point<float, 2>& spd, int fixType, int sats, float dop)
      : location(loc), speed(spd), fix(fixType), satellites(sats), precision(dop)
    {}

    //! Convenience accessors
    [[nodiscard]] double latitude() const { return location.latitude(); }
    [[nodiscard]] double longitude() const { return location.longitude(); }
    [[nodiscard]] double height() const { return location.height(); }
    [[nodiscard]] float speed2d() const { return speed[0]; }
    [[nodiscard]] float speed3d() const { return speed[1]; }

    //! Serialization support
    template<class Archive>
    void serialize(Archive& archive)
    {
      archive(location, speed, fix, satellites, precision);
    }
  };

  //! 3-axis gyroscope reading (angular velocity)
  //! Using Vector3f for easy integration with rotation calculations
  struct GyroSample
  {
    Vector3f angularVelocity; //!< Angular velocity in rad/s [x, y, z]

    //! Convenience accessors
    [[nodiscard]] float x() const { return angularVelocity[0]; }
    [[nodiscard]] float y() const { return angularVelocity[1]; }
    [[nodiscard]] float z() const { return angularVelocity[2]; }
  };

  //! 3-axis accelerometer reading (linear acceleration)
  //! Using Vector3f for easy integration with physics calculations
  struct AccelSample
  {
    Vector3f acceleration;    //!< Acceleration in m/s² [x, y, z]

    //! Convenience accessors
    [[nodiscard]] float x() const { return acceleration[0]; }
    [[nodiscard]] float y() const { return acceleration[1]; }
    [[nodiscard]] float z() const { return acceleration[2]; }
  };

  //! Temperature reading
  struct TemperatureSample
  {
    float celsius = 0;        //!< Temperature in Celsius
  };

  //! Magnetometer reading (magnetic field strength)
  //! Using Vector3f for easy integration with orientation calculations
  struct MagnetometerSample
  {
    Vector3f magneticField;   //!< Magnetic field in μT [x, y, z]

    //! Convenience accessors
    [[nodiscard]] float x() const { return magneticField[0]; }
    [[nodiscard]] float y() const { return magneticField[1]; }
    [[nodiscard]] float z() const { return magneticField[2]; }
  };

  //! Camera orientation (derived from accelerometer/gyro/magnetometer fusion)
  //! Using RAVL2 Quaternion for rotation representation
  struct OrientationSample
  {
    Quaternion<float> orientation; //!< Camera orientation as quaternion
    Vector3f eulerAngles;          //!< Euler angles (roll, pitch, yaw) in radians

    //! Convenience accessors
    [[nodiscard]] float roll() const { return eulerAngles[0]; }
    [[nodiscard]] float pitch() const { return eulerAngles[1]; }
    [[nodiscard]] float yaw() const { return eulerAngles[2]; }
  };
}
```

### Frame Types

Extend `MetaDataFrame<T>` from Video module:

```cpp
// GpsFrame.hh
namespace Ravl2::GoPro
{
  //! Frame containing GPS telemetry data
  //! Uses RAVL2's GPSCoordinate for full geodetic support
  class GpsFrame : public Video::MetaDataFrame<GpsFix>
  {
  public:
    GpsFrame(const GpsFix& data,
             Video::StreamItemId id,
             Video::MediaTime timestamp);

    [[nodiscard]] std::string dataTypeName() const override;

    //! Convenience: Get the GPSCoordinate directly
    [[nodiscard]] const GPSCoordinate& location() const { return mData.location; }
  };

  //! Frame containing gyroscope telemetry (may contain multiple samples)
  //! High-frequency sensors like gyro/accel typically have multiple samples per frame
  //! to match the GPMF packet structure
  class GyroFrame : public Video::MetaDataFrame<std::vector<GyroSample>>
  {
  public:
    GyroFrame(const std::vector<GyroSample>& data,
              Video::StreamItemId id,
              Video::MediaTime timestamp,
              float sampleRate);

    [[nodiscard]] float sampleRate() const { return mSampleRate; }
    [[nodiscard]] std::string dataTypeName() const override;

    //! Convenience: Access raw Vector3f array for processing
    //! This allows direct use with RAVL2 Array or signal processing
    [[nodiscard]] std::vector<Vector3f> asVectorArray() const {
      std::vector<Vector3f> result;
      result.reserve(mData.size());
      for (const auto& sample : mData) {
        result.push_back(sample.angularVelocity);
      }
      return result;
    }

  private:
    float mSampleRate;  //!< Samples per second
  };

  //! Similar for AccelFrame, TemperatureFrame, etc.
  //! AccelFrame also provides asVectorArray() for convenient access to acceleration vectors
}
```

### Utility Functions

The GpmfUtilities module provides helper functions that leverage RAVL2 types:

```cpp
// GpmfUtilities.hh

namespace Ravl2::GoPro
{
  //! Convert GPS track to local coordinate system for visualization
  //! Returns points as Vector3f for easy use with 3D rendering
  //! Uses the first point as origin
  std::vector<Vector3f> gpsTrackToLocal(
    const std::vector<GpsFix>& track);

  //! Convert GPS track to local coordinate system with specified origin
  std::vector<Vector3f> gpsTrackToLocal(
    const std::vector<GpsFix>& track,
    const GPSCoordinate& origin);

  //! Calculate distance between two GPS coordinates (meters)
  //! Uses GPSCoordinate's built-in great circle distance calculation
  //! which accounts for Earth's ellipsoid shape (GRS84)
  inline double gpsDistance(
    const GPSCoordinate& gps1,
    const GPSCoordinate& gps2)
  {
    // GPSCoordinate provides cartesian() method for accurate distance
    return euclidDistance(gps1.cartesian(), gps2.cartesian());
  }

  //! Integrate gyroscope data to estimate orientation changes
  //! Returns a vector of Quaternions representing orientation over time
  std::vector<Quaternion<float>> integrateGyroToOrientation(
    const std::vector<GyroSample>& gyroSamples,
    float sampleRate,
    const Quaternion<float>& initialOrientation = Quaternion<float>::identity());

  //! Remove gravity from accelerometer data using orientation
  //! Useful for extracting linear acceleration (motion) from total acceleration
  Vector3f removeGravity(
    const Vector3f& acceleration,
    const Quaternion<float>& orientation);

  //! Apply low-pass filter to vector data (for smoothing)
  template<typename VectorT>
  std::vector<VectorT> lowPassFilter(
    const std::vector<VectorT>& data,
    float alpha);
}
```

### Serialization Support

All telemetry types should support cereal serialization (consistent with RAVL2):

```cpp
// In GpmfTypes.hh

namespace Ravl2::GoPro
{
  struct GpsFix
  {
    // ... fields ...

    template<class Archive>
    void serialize(Archive& archive)
    {
      archive(location, speed, fix, satellites, precision);
    }
  };

  struct GyroSample
  {
    // ... fields ...

    template<class Archive>
    void serialize(Archive& archive)
    {
      archive(angularVelocity);
    }
  };

  // Similar for other types
}
```

This allows telemetry data to be saved/loaded using RAVL2's standard serialization:

```cpp
// Save telemetry
{
  std::ofstream os("telemetry.cereal", std::ios::binary);
  cereal::BinaryOutputArchive archive(os);
  archive(gpsFixes, gyroSamples, accelSamples);
}

// Load telemetry
{
  std::ifstream is("telemetry.cereal", std::ios::binary);
  cereal::BinaryInputArchive archive(is);
  archive(gpsFixes, gyroSamples, accelSamples);
}
```

## FFmpeg Integration Strategy

### Option 1: Parse GPMF in FfmpegMultiStreamIterator (Recommended)

**Pros:**
- Centralized handling of all stream types
- Automatic timestamp synchronization
- Consistent with existing architecture

**Cons:**
- Couples Video module to GoPro module (mitigated by optional compilation)

**Implementation:**
1. Detect data streams with GPMF codec tag in `FfmpegMediaContainer`
2. In `FfmpegMultiStreamIterator::decodePacket()`, check if stream is GPMF type
3. Pass raw data to `GpmfParser` to extract telemetry
4. Create appropriate `GpmfFrame` subtype based on FourCC
5. Return via the standard `Frame` interface

### Option 2: Register Custom Stream Handlers

**Pros:**
- Better separation of concerns
- GoPro module completely independent

**Cons:**
- Requires adding plugin/registry mechanism to Video module
- More complex architecture

**Deferred:** Start with Option 1, refactor to Option 2 if other custom parsers are needed.

## Detailed Implementation Plan

### Phase 1: Foundation (Week 1)

1. **CMake Setup**
   - Add gpmf-parser to Dependencies.cmake
   - Create GoPro/CMakeLists.txt
   - Test that gpmf-parser builds and links correctly

2. **Data Structures**
   - Implement GpmfTypes.hh/cc with telemetry structures
   - Create basic GpmfFrame base class
   - Implement GpsFrame, GyroFrame, AccelFrame

3. **GPMF Parser Wrapper**
   - Create C++ wrapper around C gpmf-parser API
   - Handle initialization, parsing, cleanup
   - Extract common telemetry types (GPS, Gyro, Accel)

### Phase 2: FFmpeg Integration (Week 2)

1. **Stream Detection**
   - Modify FfmpegMediaContainer to recognize GPMF data streams
   - Update DataProperties to include GPMF-specific metadata
   - Extend StreamType enum if needed (or use existing `Data` type with format string)

2. **Data Extraction**
   - Extract raw GPMF data packets from FFmpeg
   - Verify data format and codec tag

3. **Parser Integration**
   - In FfmpegMultiStreamIterator, detect GPMF streams
   - Parse GPMF packets using GpmfParser
   - Create appropriate Frame objects

### Phase 3: Testing & Examples (Week 3)

1. **Unit Tests**
   - Test GPMF parser with known sample data
   - Test frame creation and data access
   - Test timestamp synchronization

2. **Example Application**
   - Create example that reads GoPro video
   - Displays video frames alongside GPS track
   - Shows gyro/accel data synchronized with video
   - Example: `examples/GoProTelemetry.cc`

3. **Documentation**
   - Update README with GoPro support
   - Document data structures and API
   - Provide usage examples

### Phase 4: Polish & Optimization (Week 4)

1. **Performance**
   - Profile parsing overhead
   - Optimize memory allocations
   - Consider caching parsed telemetry

2. **Error Handling**
   - Graceful handling of corrupt GPMF data
   - Missing or partial telemetry streams
   - Version compatibility

3. **Additional Sensors**
   - Temperature
   - Magnetometer
   - Exposure settings
   - Image orientation

## Video Module Modifications Required

### Minimal Changes Approach

To minimize coupling, we can:

1. **Keep StreamType::Data generic**: No need to add GPMF-specific enum values
2. **Use DataProperties.format**: Set to "GPMF" or "gpmd" to identify GoPro streams
3. **Optional compilation**: Wrap GPMF handling in `#ifdef HAVE_GPMF`

### Code Changes in Video Module

**FfmpegMediaContainer.cc**:
```cpp
// In mapFfmpegStreamType()
case AVMEDIA_TYPE_DATA:
  return StreamType::Data;

// In dataProperties()
DataProperties props;
if (codecContext->codec_tag == MKTAG('g','p','m','d')) {
  props.format = "GPMF";
}
```

**FfmpegMultiStreamIterator.cc**:
```cpp
// In decodePacket() for Data streams
#ifdef HAVE_GPMF
if (m_streamTypes[streamIndex] == StreamType::Data) {
  auto props = m_ffmpegContainer->dataProperties(streamIndex);
  if (props.isSuccess() && props.value().format == "GPMF") {
    return parseGpmfPacket(packet, streamIndex);
  }
}
#endif
```

## API Design

### Basic Usage Example

```cpp
#include "Ravl2/Video/MediaContainer.hh"
#include "Ravl2/GoPro/GpsFrame.hh"
#include "Ravl2/GoPro/GyroFrame.hh"

using namespace Ravl2;

// Open GoPro video file
auto container = Video::MediaContainer::openFile("GOPR0001.MP4");
if (!container.isSuccess()) {
  SPDLOG_ERROR("Failed to open file");
  return;
}

// Find GPMF data stream
size_t gpmfStreamIndex = 0;
for (size_t i = 0; i < container.value()->streamCount(); i++) {
  if (container.value()->streamType(i) == Video::StreamType::Data) {
    auto props = container.value()->dataProperties(i);
    if (props.isSuccess() && props.value().format == "GPMF") {
      gpmfStreamIndex = i;
      break;
    }
  }
}

// Create iterator
auto iter = container.value()->createIterator(gpmfStreamIndex);
if (!iter.isSuccess()) {
  SPDLOG_ERROR("Failed to create iterator");
  return;
}

// Iterate through telemetry
while (!iter.value()->isAtEnd()) {
  auto frame = iter.value()->currentFrame();

  // Check frame type and process
  if (auto gpsFrame = std::dynamic_pointer_cast<GoPro::GpsFrame>(frame)) {
    const auto& gpsFix = gpsFrame->data();
    const auto& location = gpsFix.location;
    SPDLOG_INFO("GPS: lat={}, lon={}, height={}, satellites={}",
                location.latitude(), location.longitude(),
                location.height(), gpsFix.satellites);
  }
  else if (auto gyroFrame = std::dynamic_pointer_cast<GoPro::GyroFrame>(frame)) {
    const auto& samples = gyroFrame->data();
    SPDLOG_INFO("Gyro samples: {} at {} Hz",
                samples.size(), gyroFrame->sampleRate());
  }

  iter.value()->next();
}
```

### Multi-Stream Synchronization

```cpp
// Create multi-stream iterator for video + GPMF
std::vector<size_t> streams = {videoStreamIndex, gpmfStreamIndex};
auto multiIter = container.value()->createMultiStreamIterator(streams);

// Frames are returned in timestamp order
while (!multiIter->isAtEnd()) {
  auto frame = multiIter->currentFrame();
  auto streamIdx = multiIter->currentStreamIndex();

  if (streamIdx == videoStreamIndex) {
    // Process video frame
  } else if (streamIdx == gpmfStreamIndex) {
    // Process telemetry frame (synchronized with video)
  }

  multiIter->next();
}
```

## Testing Strategy

### Unit Tests

1. **GpmfParser Tests** (`test/TestGpmfParser.cc`)
   - Parse known GPMF sample data
   - Verify GPS extraction
   - Verify gyro/accel extraction
   - Handle malformed data

2. **Frame Tests** (`test/TestGpmfFrames.cc`)
   - Create and access frame data
   - Type casting and conversion
   - Timestamp handling

3. **Integration Tests** (`test/TestGoProVideo.cc`)
   - Read actual GoPro video file (checked into test/data/)
   - Verify all streams are detected
   - Verify timestamp synchronization
   - Compare extracted data with known ground truth

### Test Data

- Include small GoPro video clip in `test/data/` directory
- Extract known telemetry values for validation
- Consider synthetic GPMF data for unit tests

## Future Enhancements

### 1. Additional Telemetry Types
- Exposure settings (ISO, shutter speed)
- White balance
- Image stabilization data
- Face detection data
- Scene classification

### 2. Higher-Level Processing

Using RAVL2 types makes advanced processing straightforward:

```cpp
// Example: Integrate gyro data to estimate orientation
Quaternion<float> orientation = Quaternion<float>::identity();
for (const auto& gyroSample : gyroFrames) {
  // Angular velocity is already a Vector3f, ready to use
  auto deltaQ = Quaternion<float>::fromAngularVelocity(
    gyroSample.angularVelocity, deltaTime);
  orientation = orientation * deltaQ;
}

// Example: Calculate distance between GPS points
const GPSCoordinate& gps1 = gpsFrame1->data().location;
const GPSCoordinate& gps2 = gpsFrame2->data().location;
// Use GPSCoordinate's cartesian conversion for accurate distance
double distance = euclidDistance(gps1.cartesian(), gps2.cartesian());

// Example: Filter accelerometer data
std::vector<Vector3f> accelData;
for (const auto& sample : accelFrames) {
  accelData.push_back(sample.acceleration);
}
// Can now use RAVL2 math/signal processing on Vector3f data
```

Additional processing capabilities:
- GPS track smoothing and filtering using RAVL2 geometry
- Gyro integration for orientation estimation using `Quaternion`
- Sensor fusion (GPS + IMU) using RAVL2 math utilities
- Speed and distance calculations using `euclidDistance`
- Elevation profile generation
- Gravity compensation using `Vector3f` operations

### 3. Visualization Tools
- 3D GPS track visualization
- IMU data plots synchronized with video
- Dashboard display for telemetry

### 4. Export Capabilities
- Export to GPX format
- Export to KML for Google Earth
- Export synchronized CSV files
- Overlay telemetry on video frames

### 5. Multi-Camera Support
- Handle multiple GoPro files
- Synchronize across cameras using GPS timestamps
- Merge telemetry streams

## Risk Assessment & Mitigation

### Risk 1: GPMF Format Variations
**Risk**: Different GoPro models may have format variations
**Mitigation**:
- Test with multiple GoPro models (Hero 7, 8, 9, 10, 11)
- Implement version detection
- Gracefully handle unknown FourCC codes

### Risk 2: FFmpeg Codec Support
**Risk**: FFmpeg might not expose GPMF data stream properly
**Mitigation**:
- Test with various FFmpeg versions
- Document minimum FFmpeg version
- Provide workaround using direct MP4 parsing if needed

### Risk 3: Timestamp Synchronization
**Risk**: Telemetry and video timestamps might not align perfectly
**Mitigation**:
- Research GoPro timestamp alignment
- Implement interpolation for GPS (low sample rate)
- Allow user-specified time offset correction

### Risk 4: Performance Overhead
**Risk**: GPMF parsing might slow down video processing
**Mitigation**:
- Make GPMF parsing optional per-stream
- Implement lazy parsing (parse on access)
- Profile and optimize hot paths

## Dependencies Summary

### Required
- gpmf-parser (MIT/Apache 2.0) - auto-fetched via CPM
- FFmpeg (LGPL) - already required by Video module
- spdlog, fmt - already in RAVL2

### Optional
- None for basic functionality

## Compatibility

### GoPro Models Supported
- HERO5 Black and later
- HERO5 Session and later
- MAX, Fusion (360 cameras)

### Platforms
- Linux (tested)
- macOS (should work)
- Windows (should work with MSVC)

## Documentation Requirements

1. **User Documentation**
   - How to enable GoPro support in CMake
   - API reference for GoPro namespace
   - Example code snippets
   - Supported GoPro models

2. **Developer Documentation**
   - GPMF format overview
   - Integration architecture
   - How to add new telemetry types
   - Testing procedures

3. **Example Programs**
   - Basic telemetry extraction
   - GPS track visualization
   - Video + telemetry synchronization
   - Export to standard formats

## Success Criteria

The implementation will be considered successful when:

1. [ ] GoPro module compiles optionally (can be disabled)
2. [ ] gpmf-parser is auto-fetched via CMake
3. [ ] Can detect and iterate GPMF data streams
4. [ ] GPS, gyro, and accelerometer data can be extracted
5. [ ] Timestamps are synchronized with video frames
6. [ ] Unit tests pass with >90% coverage
7. [ ] Example application demonstrates usage
8. [ ] Documentation is complete and clear
9. [ ] No performance regression in non-GoPro workflows
10. [ ] Works with at least 3 different GoPro video samples

## Timeline Estimate

- **Week 1**: Foundation (CMake, data structures, parser wrapper)
- **Week 2**: FFmpeg integration (stream detection, parsing)
- **Week 3**: Testing and examples
- **Week 4**: Polish, optimization, documentation

**Total**: Approximately 4 weeks for full implementation

## Open Questions

All design questions have been resolved:

1. **Multi-sample frames**: ✓ Confirmed - Use arrays within frames to match GPMF structure and reduce frame overhead

2. **Stream iterator vs. specialized API**: ✓ Confirmed - Use standard StreamIterator for consistency; add convenience functions in GoPro namespace

3. **Interpolation**: ✓ Confirmed - No interpolation in base implementation; provide as separate utility function

4. **Thread safety**: ✓ Confirmed - Follow Video module's pattern - thread-safe container, independent iterators

5. **GPS Coordinate Type**: ✓ Confirmed - Use RAVL2's existing `GPSCoordinate` class for geodetic accuracy (GRS84 ellipsoid)

## RAVL2 Type Usage Summary

Using native RAVL2 types throughout the GoPro integration provides significant advantages:

### Direct Library Integration
- **GPSCoordinate Class**: Full support for geodetic calculations including:
  - GRS84 ellipsoid model (Earth's actual shape)
  - Cartesian coordinate conversion for accurate 3D distances
  - Built-in error bounds tracking
  - Text parsing (DMS format) and serialization
- **Geometry Module**: GPS coordinates work with transforms, ranges, and geometric algorithms
- **3D Module**: Can visualize GPS tracks using existing 3D rendering infrastructure via `cartesian()`
- **Math Module**: Vector operations (norm, dot, cross) available on all sensor data
- **Quaternion Support**: Gyro data integrates seamlessly with orientation tracking

### Type-Safe API
- Prevents dimension errors (can't accidentally swap lat/lon vs lon/lat)
- Compile-time checking of vector operations
- Consistent indexing `[0]`, `[1]`, `[2]` across all RAVL2 code

### Performance
- Header-only Eigen types (no virtual calls)
- Vectorization opportunities for batch processing
- Memory layout compatible with GPU operations

### Developer Experience
- Familiar API for RAVL2 users
- IntelliSense/IDE support for vector operations
- Reduced mental overhead (same types everywhere)

### Example Workflow
```cpp
// Extract GPS track from video
std::vector<GpsFix> gpsTrack = extractGpsFromVideo("video.mp4");

// Get GPSCoordinate objects
std::vector<GPSCoordinate> coordinates;
for (const auto& fix : gpsTrack) {
  coordinates.push_back(fix.location);
}

// Convert to local 3D coordinates for visualization
auto localPoints = gpsTrackToLocal(gpsTrack);

// Calculate path length using GPSCoordinate's accurate distance
double totalDistance = 0;
for (size_t i = 1; i < coordinates.size(); i++) {
  totalDistance += gpsDistance(coordinates[i-1], coordinates[i]);
}

// Get lat/lon for region analysis
std::vector<Point<double, 2>> latLonPoints;
for (const auto& coord : coordinates) {
  latLonPoints.push_back({coord.latitude(), coord.longitude()});
}
PointSet<double, 2> pointSet(latLonPoints);
auto bbox = pointSet.boundingRectangle();

// Visualize with RAVL2 Display module (if enabled)
// localPoints are Vector3f, ready for 3D rendering
```

## References

- [GPMF Parser GitHub](https://github.com/gopro/gpmf-parser)
- [GPMF Spec](https://github.com/gopro/gpmf-parser/blob/master/README.md)
- RAVL2 Video Module Documentation (src/Ravl2/Video/ReadMe.md)
- RAVL2 Geometry Module (src/Ravl2/Geometry/)
- RAVL2 GPSCoordinate Class (src/Ravl2/Geometry/GPSCoordinate.hh) - GRS84 ellipsoid implementation
- RAVL2 Quaternion Class (src/Ravl2/Geometry/Quaternion.hh) - For orientation tracking
- RAVL2 Types (src/Ravl2/Types.hh)
- FFmpeg Data Stream Documentation
