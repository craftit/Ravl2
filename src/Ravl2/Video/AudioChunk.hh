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

namespace Ravl2::Video
{
  //! Base class for audio chunks, regardless of sample type
  class AudioChunkBase : public Frame
  {
  public:
    //! Get the number of channels
    [[nodiscard]] virtual int channels() const = 0;

    //! Get the number of samples in this chunk
    [[nodiscard]] virtual int samples() const = 0;

    //! Get the stream type
    [[nodiscard]] StreamType streamType() const override
    {
      return StreamType::Audio;
    }

  protected:
    //! Constructor with ID and timestamp
    AudioChunkBase(StreamItemId id, MediaTime timestamp)
      : Frame(id, timestamp)
    {
    }

    //! Default constructor
    AudioChunkBase() = default;
  };

  //! Interface class for audio chunks with type erasure for template functions
  template<typename SampleT> class AudioChunk : public AudioChunkBase
  {
  public:
    //! Virtual destructor
    ~AudioChunk() override = default;

    //! Create an audio chunk
    AudioChunk(const Array<SampleT, 2>&audioData, StreamItemId id, MediaTime timestamp)
      : AudioChunkBase(id, timestamp)
        , m_audioData(audioData)
    {
    }

    //! Get the number of channels
    [[nodiscard]] int channels() const override { return m_audioData.range().size(1); }

    //! Get the number of samples in this chunk
    [[nodiscard]] int samples() const override { return m_audioData.range().size(0); }

    //! Access the audio data
    [[nodiscard]] const Array<SampleT, 2>& audioData() const { return m_audioData; }

    //! Check if this frame is valid
    [[nodiscard]] bool isValid() const override
    {
      return !m_audioData.empty();
    }

    //! Access data
    [[nodiscard]] std::any frameData() const override
    {
      return m_audioData;
    }

  protected:
    //! Constructor with ID and timestamp
    AudioChunk(StreamItemId id, MediaTime timestamp)
      : AudioChunkBase(id, timestamp)
    {
    }

    //! Default constructor
    AudioChunk() = default;

  private:
    Array<SampleT, 2> m_audioData; //!< The audio data
  };
} // namespace Ravl2::Video
