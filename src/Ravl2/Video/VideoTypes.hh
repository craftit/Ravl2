//
// Created on September 6, 2025
//

#pragma once

#include <cstdint>
#include <string>
#include <chrono>
#include "Ravl2/Types.hh"

namespace Ravl2 {
namespace Video {

/// Stream types available in a media container
enum class StreamType {
  Video,    ///< Video stream containing visual frames
  Audio,    ///< Audio stream containing audio samples
  Subtitle, ///< Subtitle/caption stream
  Data,     ///< Data stream (may contain metadata or other auxiliary information)
  Unknown   ///< Unrecognized stream type
};

/// Unique identifier for a frame/chunk within a stream
using StreamItemId = int64_t;

/// Error codes for video operations
enum class VideoErrorCode {
  Success,                ///< Operation completed successfully
  EndOfStream,            ///< End of stream reached
  InvalidOperation,       ///< Operation is not valid in current state
  ResourceUnavailable,    ///< Required resource is not available
  DecodingError,          ///< Error occurred during decoding
  SeekFailed,             ///< Failed to seek to requested position
  CorruptedData,          ///< Data is corrupted
  UnsupportedFormat,      ///< Format is not supported
  ResourceAllocationError ///< Failed to allocate required resources
};

/// Timestamp type for media time points
using MediaTime = std::chrono::microseconds;

/// Structure to represent codec information
struct CodecInfo {
  std::string name;       ///< Codec name
  std::string longName;   ///< Long/descriptive name
  bool isLossless;        ///< Whether the codec is lossless
};

/// Structure to represent video stream properties
struct VideoProperties {
  int width;                  ///< Frame width in pixels
  int height;                 ///< Frame height in pixels
  float frameRate;            ///< Frames per second (can be variable)
  bool isVariableFrameRate;   ///< Whether the frame rate is variable
  CodecInfo codec;            ///< Codec information
  MediaTime duration;         ///< Total duration
  int64_t totalFrames;        ///< Total number of frames (may be estimated)
  std::string pixelFormat;    ///< Pixel format string
};

/// Structure to represent audio stream properties
struct AudioProperties {
  int sampleRate;         ///< Samples per second
  int channels;           ///< Number of audio channels
  int bitsPerSample;      ///< Bits per sample
  CodecInfo codec;        ///< Codec information
  MediaTime duration;     ///< Total duration
  int64_t totalSamples;   ///< Total number of samples
};

/// Structure to represent metadata stream properties
struct DataProperties {
  std::string format;          ///< Format identifier (e.g., "GPS", "ACCELEROMETER")
  std::string dataTypeName;    ///< Name of the data type for this metadata stream
  MediaTime duration;          ///< Total duration
  int64_t totalItems;          ///< Total number of metadata items (may be estimated)
  float sampleRate;            ///< Items per second (if applicable)
};

/// Seeking behavior flags
enum class SeekFlags {
  Precise,    ///< Precise seeking (may be slower)
  Keyframe,   ///< Seek to nearest keyframe (faster)
  Previous,   ///< Seek to previous frame/chunk
  Next        ///< Seek to next frame/chunk
};

/// Result type for operations that can fail
template<typename T>
class VideoResult {
public:
  /// Construct a successful result with a value
  explicit VideoResult(const T& value) : m_value(value), m_error(VideoErrorCode::Success) {}

  /// Construct a failed result with an error code
  explicit VideoResult(VideoErrorCode error) : m_error(error) {}

  /// Check if the operation was successful
  bool isSuccess() const { return m_error == VideoErrorCode::Success; }

  /// Get the error code
  VideoErrorCode error() const { return m_error; }

  /// Get the value (only valid if isSuccess() is true)
  const T& value() const { return m_value; }

private:
  T m_value;                 ///< The result value
  VideoErrorCode m_error;    ///< The error code
};

// Specialization for void results
template<>
class VideoResult<void> {
public:
  /// Construct a successful result
  VideoResult() : m_error(VideoErrorCode::Success) {}

  /// Construct a failed result with an error code
  explicit VideoResult(VideoErrorCode error) : m_error(error) {}

  /// Check if the operation was successful
  bool isSuccess() const { return m_error == VideoErrorCode::Success; }

  /// Get the error code
  VideoErrorCode error() const { return m_error; }

private:
  VideoErrorCode m_error;    ///< The error code
};

} // namespace Video
} // namespace Ravl2
