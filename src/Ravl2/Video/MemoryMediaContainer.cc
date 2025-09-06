// filepath: /home/charles/src/Ravl2/src/Ravl2/Video/MemoryMediaContainer.cc
//
// Created on September 6, 2025
// Implementation of the in-memory MediaContainer
//

#include "Ravl2/Video/MemoryMediaContainer.hh"
#include <algorithm>
#include <functional>

namespace Ravl2::Video {

//! Constructor with streams data
MemoryMediaContainer::MemoryMediaContainer(const std::vector<StreamData>& streams,
                                          const std::map<std::string, std::string>& metadata)
  : m_streams(streams)
  , m_metadata(metadata)
  , m_isOpen(true)
  , m_duration(0)
{
  // Calculate container duration (longest stream)
  if (!m_streams.empty()) {
    for (const auto& stream : m_streams) {
      // Extract duration based on stream type
      MediaTime streamDuration = std::visit([](const auto& props) -> MediaTime {
          return props.duration;
      }, stream.mProperties);

      // Update container duration if this stream is longer
      if (streamDuration > m_duration) {
        m_duration = streamDuration;
      }
    }
  }
}

//! Check if the container is open
bool MemoryMediaContainer::isOpen() const {
  std::lock_guard<std::mutex> lock(m_mutex);
  return m_isOpen;
}

//! Close the container and release resources
VideoResult<void> MemoryMediaContainer::close() {
  std::lock_guard<std::mutex> lock(m_mutex);
  if (!m_isOpen) {
    return VideoResult<void>(VideoErrorCode::InvalidOperation);
  }

  m_isOpen = false;
  return VideoResult<void>();
}

//! Get the number of streams in the container
std::size_t MemoryMediaContainer::streamCount() const {
  std::lock_guard<std::mutex> lock(m_mutex);
  return m_streams.size();
}

//! Get the type of stream at the specified index
StreamType MemoryMediaContainer::streamType(std::size_t streamIndex) const {
  std::lock_guard<std::mutex> lock(m_mutex);
  if (!isValidStreamIndex(streamIndex)) {
    return StreamType::Unknown;
  }

  return m_streams[streamIndex].mType;
}

//! Get properties for a video stream
VideoResult<VideoProperties> MemoryMediaContainer::videoProperties(std::size_t streamIndex) const {
  std::lock_guard<std::mutex> lock(m_mutex);
  if (!isValidStreamIndex(streamIndex)) {
    return VideoResult<VideoProperties>(VideoErrorCode::InvalidOperation);
  }

  const auto& stream = m_streams[streamIndex];
  if (stream.mType != StreamType::Video) {
    return VideoResult<VideoProperties>(VideoErrorCode::InvalidOperation);
  }

  try {
    const VideoProperties& props = std::get<VideoProperties>(stream.mProperties);
    return VideoResult<VideoProperties>(props);
  } catch (const std::bad_variant_access&) {
    return VideoResult<VideoProperties>(VideoErrorCode::InvalidOperation);
  }
}

//! Get properties for an audio stream
VideoResult<AudioProperties> MemoryMediaContainer::audioProperties(std::size_t streamIndex) const {
  std::lock_guard<std::mutex> lock(m_mutex);
  if (!isValidStreamIndex(streamIndex)) {
    return VideoResult<AudioProperties>(VideoErrorCode::InvalidOperation);
  }

  const auto& stream = m_streams[streamIndex];
  if (stream.mType != StreamType::Audio) {
    return VideoResult<AudioProperties>(VideoErrorCode::InvalidOperation);
  }

  try {
    const AudioProperties& props = std::get<AudioProperties>(stream.mProperties);
    return VideoResult<AudioProperties>(props);
  } catch (const std::bad_variant_access&) {
    return VideoResult<AudioProperties>(VideoErrorCode::InvalidOperation);
  }
}

//! Get properties for a data stream
VideoResult<DataProperties> MemoryMediaContainer::dataProperties(std::size_t streamIndex) const {
  std::lock_guard<std::mutex> lock(m_mutex);
  if (!isValidStreamIndex(streamIndex)) {
    return VideoResult<DataProperties>(VideoErrorCode::InvalidOperation);
  }

  const auto& stream = m_streams[streamIndex];
  if (stream.mType != StreamType::Data) {
    return VideoResult<DataProperties>(VideoErrorCode::InvalidOperation);
  }

  try {
    const DataProperties& props = std::get<DataProperties>(stream.mProperties);
    return VideoResult<DataProperties>(props);
  } catch (const std::bad_variant_access&) {
    return VideoResult<DataProperties>(VideoErrorCode::InvalidOperation);
  }
}

//! Get the total duration of the container (longest stream)
MediaTime MemoryMediaContainer::duration() const {
  std::lock_guard<std::mutex> lock(m_mutex);
  return m_duration;
}

//! Create an iterator for a specific stream
VideoResult<std::shared_ptr<StreamIterator>> MemoryMediaContainer::createIterator(std::size_t streamIndex) {
  std::lock_guard<std::mutex> lock(m_mutex);
  if (!m_isOpen) {
    return VideoResult<std::shared_ptr<StreamIterator>>(VideoErrorCode::ResourceUnavailable);
  }

  if (!isValidStreamIndex(streamIndex)) {
    return VideoResult<std::shared_ptr<StreamIterator>>(VideoErrorCode::InvalidOperation);
  }

  auto iterator = std::make_shared<MemoryStreamIterator>(std::dynamic_pointer_cast<MemoryMediaContainer>(shared_from_this()), streamIndex);
  return VideoResult<std::shared_ptr<StreamIterator>>(std::static_pointer_cast<StreamIterator>(iterator));
}

//! Get global container metadata
std::map<std::string, std::string> MemoryMediaContainer::metadata() const {
  std::lock_guard<std::mutex> lock(m_mutex);
  return m_metadata;
}

//! Get specific metadata value
std::string MemoryMediaContainer::metadata(const std::string& key) const {
  std::lock_guard<std::mutex> lock(m_mutex);
  auto it = m_metadata.find(key);
  return (it != m_metadata.end()) ? it->second : "";
}

//! Check if a specific metadata key exists
bool MemoryMediaContainer::hasMetadata(const std::string& key) const {
  std::lock_guard<std::mutex> lock(m_mutex);
  return m_metadata.find(key) != m_metadata.end();
}

//! Factory method to create memory container from streams
std::shared_ptr<MemoryMediaContainer> MemoryMediaContainer::create(
    const std::vector<StreamData>& streams,
    const std::map<std::string, std::string>& metadata) {
  return std::make_shared<MemoryMediaContainer>(streams, metadata);
}

//! Check if a stream index is valid
bool MemoryMediaContainer::isValidStreamIndex(std::size_t streamIndex) const {
  return streamIndex < m_streams.size();
}

//----------------------------------------------------------------------
// MemoryStreamIterator Implementation
//----------------------------------------------------------------------

MemoryStreamIterator::MemoryStreamIterator(std::shared_ptr<MemoryMediaContainer> container, std::size_t streamIndex)
  : StreamIterator(container, streamIndex)
  , m_frames(nullptr)
{
  if (container && streamIndex < container->streamCount()) {
    // Get a pointer to the frames vector for the specified stream
    // This is safe because we're storing a pointer to a vector that's owned by the container
    // and we hold a shared_ptr to the container to ensure it outlives this iterator
    m_frames = &container->m_streams[streamIndex].mFrames;
  }
}


bool MemoryStreamIterator::isAtEnd() const {
  return !isValidStream() || !m_frames || m_currentPosition < 0 || m_currentPosition >= static_cast<std::ptrdiff_t>(m_frames->size());
}

VideoResult<void> MemoryStreamIterator::next() {
  if (!isValidStream() || !m_frames) {
    return VideoResult<void>(VideoErrorCode::InvalidOperation);
  }

  if (isAtEnd()) {
    return VideoResult<void>(VideoErrorCode::EndOfStream);
  }

  ++m_currentPosition;
  if (m_currentPosition >= static_cast<std::ptrdiff_t>(m_frames->size()))
  {
    m_currentPosition = -1;
    return VideoResult<void>(VideoErrorCode::EndOfStream);
  }
  setCurrentFrame(m_frames->at(static_cast<std::size_t>(m_currentPosition)));
  return VideoResult<void>();
}

VideoResult<void> MemoryStreamIterator::previous() {
  if (!isValidStream() || !m_frames) {
    return VideoResult<void>(VideoErrorCode::InvalidOperation);
  }

  if (m_currentPosition == 0) {
    return VideoResult<void>(VideoErrorCode::InvalidOperation);
  }

  --m_currentPosition;
  if (m_currentPosition < 0)
  {
    m_currentPosition = -1;
    return VideoResult<void>(VideoErrorCode::EndOfStream);
  }
  setCurrentFrame(m_frames->at(static_cast<std::size_t>(m_currentPosition)));
  return VideoResult<void>();
}

VideoResult<void> MemoryStreamIterator::seek(MediaTime timestamp, SeekFlags flags) {
  if (!isValidStream() || !m_frames || m_frames->empty()) {
    return VideoResult<void>(VideoErrorCode::InvalidOperation);
  }

  // Handle different seek flags
  switch (flags) {
    case SeekFlags::Precise: {
      // Find the frame with the closest timestamp (prefer exact match if available)
      auto it = std::find_if(m_frames->begin(), m_frames->end(),
                           [timestamp](const auto& frame) {
                             return frame->timestamp() == timestamp;
                           });

      // If exact match not found, find closest
      if (it == m_frames->end()) {
        it = std::min_element(m_frames->begin(), m_frames->end(),
                            [timestamp](const auto& a, const auto& b) {
                              auto diffA = std::abs((a->timestamp() - timestamp).count());
                              auto diffB = std::abs((b->timestamp() - timestamp).count());
                              return diffA < diffB;
                            });
      }

      if (it != m_frames->end()) {
        m_currentPosition = std::distance(m_frames->begin(), it);
        return VideoResult<void>();
      }
      break;
    }

    case SeekFlags::Keyframe: {
      // In-memory container doesn't need special keyframe handling
      // for this simple implementation, just use Precise seeking
      return seek(timestamp, SeekFlags::Precise);
    }

    case SeekFlags::Previous: {
      // Find the last frame with timestamp <= requested timestamp
      auto it = std::find_if(m_frames->rbegin(), m_frames->rend(),
                           [timestamp](const auto& frame) {
                             return frame->timestamp() <= timestamp;
                           });

      if (it != m_frames->rend()) {
        m_currentPosition = static_cast<std::ptrdiff_t>(m_frames->size()) - 1 - std::distance(m_frames->rbegin(), it);
        return VideoResult<void>();
      }
      break;
    }

    case SeekFlags::Next: {
      // Find the first frame with timestamp >= requested timestamp
      auto it = std::find_if(m_frames->begin(), m_frames->end(),
                           [timestamp](const auto& frame) {
                             return frame->timestamp() >= timestamp;
                           });

      if (it != m_frames->end()) {
        m_currentPosition = std::distance(m_frames->begin(), it);
        return VideoResult<void>();
      }
      break;
    }
  }

  return VideoResult<void>(VideoErrorCode::SeekFailed);
}

VideoResult<void> MemoryStreamIterator::seekToIndex(int64_t index) {
  if (!isValidStream() || !m_frames) {
    return VideoResult<void>(VideoErrorCode::InvalidOperation);
  }

  if (index < 0 || static_cast<std::size_t>(index) >= m_frames->size()) {
    return VideoResult<void>(VideoErrorCode::SeekFailed);
  }

  m_currentPosition = static_cast<std::ptrdiff_t>(index);
  return {};
}

VideoResult<std::shared_ptr<Frame>> MemoryStreamIterator::getFrameById(StreamItemId id) const {
  if (!isValidStream() || !m_frames) {
    return VideoResult<std::shared_ptr<Frame>>(VideoErrorCode::InvalidOperation);
  }

  auto it = std::find_if(m_frames->begin(), m_frames->end(),
                       [id](const auto& frame) {
                         return frame->id() == id;
                       });

  if (it != m_frames->end()) {
    return VideoResult<std::shared_ptr<Frame>>(*it);
  }

  return VideoResult<std::shared_ptr<Frame>>(VideoErrorCode::ResourceUnavailable);
}

VideoResult<void> MemoryStreamIterator::reset() {
  if (!isValid()) {
    return VideoResult<void>(VideoErrorCode::InvalidOperation);
  }

  m_currentPosition = 0;
  return VideoResult<void>();
}


MediaTime MemoryStreamIterator::duration() const {
  if (!isValid()) {
    return MediaTime(0);
  }
  return media().duration();
}


} // namespace Ravl2::Video
