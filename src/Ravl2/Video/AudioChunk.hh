//
// Created on September 6, 2025
//

#pragma once

#include <chrono>
#include <memory>
#include <string>
#include <type_traits>
#include "Ravl2/Array.hh"
#include "Ravl2/Video/VideoTypes.hh"
#include "Ravl2/Video/Frame.hh"

namespace Ravl2 {
namespace Video {

/// Base class for audio chunks, regardless of sample type
class AudioChunkBase : public Frame {
public:
  /// Get the number of channels
  virtual int channels() const = 0;

  /// Get the number of samples in this chunk
  virtual int samples() const = 0;

  /// Get duration of this chunk
  MediaTime duration() const {
    return MediaTime(static_cast<int64_t>((static_cast<double>(samples()) / sampleRate()) * 1000000));
  }

  /// Get the stream type
  StreamType streamType() const override {
    return StreamType::Audio;
  }

protected:
  /// Constructor with ID and timestamp
  AudioChunkBase(StreamItemId id, MediaTime timestamp)
    : Frame(id, timestamp)
  {}

  /// Default constructor
  AudioChunkBase() = default;
};

/// Forward declarations for audio implementations
template<typename SampleT, int Dimensions = 1>
class AudioChunkImpl;

/// Interface class for audio chunks
class AudioChunk : public AudioChunkBase {
public:
  /// Virtual destructor
  virtual ~AudioChunk() = default;

  /// Create a single-channel audio chunk
  template<typename SampleT>
  static std::shared_ptr<AudioChunk> createMono(const Array<SampleT>& audioData,
                                                StreamItemId id,
                                                MediaTime timestamp,
                                                int sampleRate) {
    return std::make_shared<AudioChunkImpl<SampleT, 1>>(audioData, id, timestamp, sampleRate);
  }

  /// Create a multi-channel audio chunk
  template<typename SampleT>
  static std::shared_ptr<AudioChunk> createMultiChannel(const Array<SampleT, 2>& audioData,
                                                       StreamItemId id,
                                                       MediaTime timestamp,
                                                       int sampleRate) {
    return std::make_shared<AudioChunkImpl<SampleT, 2>>(audioData, id, timestamp, sampleRate);
  }

  /// Get the sample type name
  virtual std::string sampleTypeName() const = 0;

  /// Check if this is float or integer data
  virtual bool isFloatSample() const = 0;

  /// Check if this is mono or multi-channel
  virtual bool isMultiChannel() const = 0;

  /// Check if the chunk has valid data
  virtual bool isValid() const = 0;

protected:
  /// Constructor with ID and timestamp
  AudioChunk(StreamItemId id, MediaTime timestamp)
    : AudioChunkBase(id, timestamp)
  {}

  /// Default constructor
  AudioChunk() = default;

  /// Implementation of getting mono data for a specific sample type
  template<typename SampleT>
  virtual Array<SampleT> getMonoDataImpl() const {
    throw std::runtime_error("Unsupported sample type or dimensions for mono data");
  }

  /// Implementation of getting multi-channel data for a specific sample type
  template<typename SampleT>
  virtual Array<SampleT, 2> getMultiChannelDataImpl() const {
    throw std::runtime_error("Unsupported sample type or dimensions for multi-channel data");
  }
};

/// Template implementation of AudioChunk for mono audio
template<typename SampleT>
class AudioChunkImpl<SampleT> : public AudioChunk {
public:
  /// Constructor with audio data, ID, timestamp, and sample rate
  AudioChunkImpl(const Array<SampleT>& audioData, StreamItemId id, MediaTime timestamp, int sampleRate)
    : AudioChunk(id, timestamp)
    , m_audioData(audioData)
    , m_sampleRate(sampleRate)
    , m_samples(audioData.size())
  {}

  /// Get the number of channels (always 1 for mono)
  int channels() const override { return m_audioData.size(0); }

  /// Get the number of samples in this chunk
  int samples() const override { return m_autoData.size(1); }

  /// Get the sample type name
  std::string sampleTypeName() const override {
    return typeid(SampleT).name();
  }

  /// Check if this is float or integer data
  bool isFloatSample() const override {
    return std::is_floating_point<SampleT>::value;
  }

  /// Check if this is mono or multi-channel (always false for mono)
  bool isMultiChannel() const override { return m_audioData.size(0) > 1; }

  /// Check if the chunk has valid data
  bool isValid() const override
  { return !m_audioData.empty(); }

private:
  Array<SampleT,2> m_audioData;   ///< The audio data
};


} // namespace Video
} // namespace Ravl2
