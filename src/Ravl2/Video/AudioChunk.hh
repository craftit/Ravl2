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

namespace Ravl2::Video {

//! Base class for audio chunks, regardless of sample type
class AudioChunkBase : public Frame {
public:
  //! Get the sample rate
  virtual int sampleRate() const = 0;

  //! Get the number of channels
  virtual int channels() const = 0;

  //! Get the number of samples in this chunk
  virtual int samples() const = 0;

  //! Get duration of this chunk
  MediaTime duration() const {
    return MediaTime(static_cast<int64_t>((static_cast<double>(samples()) / sampleRate()) * 1000000));
  }

  //! Get the stream type
  StreamType streamType() const override {
    return StreamType::Audio;
  }

protected:
  //! Constructor with ID and timestamp
  AudioChunkBase(StreamItemId id, MediaTime timestamp)
    : Frame(id, timestamp)
  {}

  //! Default constructor
  AudioChunkBase() = default;
};

//! Forward declarations for audio implementations
template<typename SampleT, int Dimensions = 1>
class AudioChunkImpl;

//! Interface class for audio chunks with type erasure for template functions
class AudioChunk : public AudioChunkBase {
public:
  //! Virtual destructor
  virtual ~AudioChunk() = default;

  //! Create a single-channel audio chunk
  template<typename SampleT>
  static std::shared_ptr<AudioChunk> createMono(const Array<SampleT>& audioData,
                                                StreamItemId id,
                                                MediaTime timestamp,
                                                int sampleRate) {
    return std::make_shared<AudioChunkImpl<SampleT, 1>>(audioData, id, timestamp, sampleRate);
  }

  //! Create a multi-channel audio chunk
  template<typename SampleT>
  static std::shared_ptr<AudioChunk> createMultiChannel(const Array<SampleT, 2>& audioData,
                                                       StreamItemId id,
                                                       MediaTime timestamp,
                                                       int sampleRate) {
    return std::make_shared<AudioChunkImpl<SampleT, 2>>(audioData, id, timestamp, sampleRate);
  }

  //! Get single-channel audio data
  template<typename SampleT>
  Array<SampleT> getMonoData() const {
    if (!isMonoChannel() || sampleTypeName() != typeid(SampleT).name()) {
      throw std::runtime_error("Sample type mismatch or not mono audio");
    }
    return getMonoDataTyped(static_cast<const SampleT*>(nullptr));
  }

  //! Get multi-channel audio data
  template<typename SampleT>
  Array<SampleT, 2> getMultiChannelData() const {
    if (isMonoChannel() || sampleTypeName() != typeid(SampleT).name()) {
      throw std::runtime_error("Sample type mismatch or not multi-channel audio");
    }
    return getMultiChannelDataTyped(static_cast<const SampleT*>(nullptr));
  }

  //! Get the sample type name
  virtual std::string sampleTypeName() const = 0;

  //! Check if this is float or integer data
  virtual bool isFloatSample() const = 0;

  //! Check if this is mono or multi-channel
  virtual bool isMonoChannel() const = 0;


protected:
  //! Constructor with ID and timestamp
  AudioChunk(StreamItemId id, MediaTime timestamp)
    : AudioChunkBase(id, timestamp)
  {}

  //! Default constructor
  AudioChunk() = default;

  //! Type-erased template method for mono audio
  template<typename SampleT>
  Array<SampleT> getMonoDataTyped(const SampleT*) const {
    throw std::runtime_error("Unsupported sample type for mono audio");
  }

  //! Type-erased template method for multi-channel audio
  template<typename SampleT>
  Array<SampleT, 2> getMultiChannelDataTyped(const SampleT*) const {
    throw std::runtime_error("Unsupported sample type for multi-channel audio");
  }
};

//! Template implementation of AudioChunk for mono audio
template<typename SampleT>
class AudioChunkImpl<SampleT, 1> : public AudioChunk {
public:
  //! Constructor with audio data, ID, timestamp, and sample rate
  AudioChunkImpl(const Array<SampleT>& audioData, StreamItemId id, MediaTime timestamp, int sampleRate)
    : AudioChunk(id, timestamp)
    , m_audioData(audioData)
    , m_sampleRate(sampleRate)
  {}

  //! Get the sample rate
  int sampleRate() const override { return m_sampleRate; }

  //! Get the number of channels (always 1 for mono)
  int channels() const override { return 1; }

  //! Get the number of samples in this chunk
  int samples() const override { return m_audioData.size(); }

  //! Get the sample type name
  std::string sampleTypeName() const override {
    return typeid(SampleT).name();
  }

  //! Check if this is float or integer data
  bool isFloatSample() const override {
    return std::is_floating_point<SampleT>::value;
  }

  //! Check if this is mono or multi-channel (always true for mono)
  bool isMonoChannel() const override { return true; }

  //! Check if the chunk has valid data
  bool isValid() const override {
    return !m_audioData.empty();
  }

protected:
  //! Specialization for the actual sample type
  Array<SampleT> getMonoDataTyped(const SampleT*) const override {
    return m_audioData;
  }

private:
  Array<SampleT> m_audioData;   //!< The audio data
  int m_sampleRate = 0;         //!< Sample rate in Hz
};

//! Template implementation of AudioChunk for multi-channel audio
template<typename SampleT>
class AudioChunkImpl<SampleT, 2> : public AudioChunk {
public:
  //! Constructor with audio data, ID, timestamp, and sample rate
  AudioChunkImpl(const Array<SampleT, 2>& audioData, StreamItemId id, MediaTime timestamp, int sampleRate)
    : AudioChunk(id, timestamp)
    , m_audioData(audioData)
    , m_sampleRate(sampleRate)
  {}

  //! Get the sample rate
  int sampleRate() const override { return m_sampleRate; }

  //! Get the number of channels
  int channels() const override { return m_audioData.size(1); }

  //! Get the number of samples in this chunk
  int samples() const override { return m_audioData.size(0); }

  //! Get the sample type name
  std::string sampleTypeName() const override {
    return typeid(SampleT).name();
  }

  //! Check if this is float or integer data
  bool isFloatSample() const override {
    return std::is_floating_point<SampleT>::value;
  }

  //! Check if this is mono or multi-channel (always false for mono)
  bool isMonoChannel() const override { return false; }

  //! Check if the chunk has valid data
  bool isValid() const override {
    return !m_audioData.empty();
  }

protected:
  //! Specialization for the actual sample type
  Array<SampleT, 2> getMultiChannelDataTyped(const SampleT*) const override {
    return m_audioData;
  }

private:
  Array<SampleT, 2> m_audioData;  //!< The audio data
  int m_sampleRate = 0;           //!< Sample rate in Hz
};


} // namespace Ravl2::Video
