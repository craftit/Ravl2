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
  //! Audio chunks, this wrapper mainly exists to distinguish the data from images.

  template<typename SampleT>
  class AudioChunk
  {
  public:
    //! Default constructor
    AudioChunk() = default;

    //! Default constructor
    explicit AudioChunk(Array<SampleT, 2> data)
      : mAudioData(std::move(data))
    {}

    const Array<SampleT, 2> &data() const
    { return mAudioData; } //!< The audio data


  private:
    Array<SampleT, 2> mAudioData; //!< The audio data
  };

} // namespace Ravl2::Video
