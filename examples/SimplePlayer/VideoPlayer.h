//
// Created on September 12, 2025
//

#pragma once

#include <QObject>
#include <QPixmap>
#include <QString>
#include <QMutex>
#include <memory>
#include <string>

namespace Ravl2::Video
{
  class MediaContainer;
  class StreamIterator;
  class VideoFrameBase; //!< Base class for video frames
}

//! VideoPlayer class that interfaces with Ravl2 MediaContainer
class VideoPlayer : public QObject
{
  Q_OBJECT

public:
  //! Constructor
  explicit VideoPlayer(QObject* parent = nullptr);

  //! Destructor
  ~VideoPlayer() override;

  //! Open a video file
  bool openFile(const QString&filePath);

  //! Start playback
  void play();

  //! Pause playback
  void pause();

  //! Stop playback and reset to beginning
  void stop();

  //! Seek to position (in microseconds)
  void seek(int64_t position);

  //! Move to next frame
  bool nextFrame();

  //! Move to previous frame
  bool previousFrame();

  //! Get current position (in microseconds)
  int64_t getPosition() const;

  //! Get total duration (in microseconds)
  int64_t getDuration() const;

  //! Check if a file is loaded
  bool isFileLoaded() const;

  //! Get the current frame as a QPixmap
  QPixmap getCurrentFrame();

signals:
  //! Signal emitted when the video duration changes
  void durationChanged(int64_t duration);

  //! Signal emitted when the current position changes
  void positionChanged(int64_t position);

  //! Signal emitted when video information changes
  void videoInfoChanged(const QString&info);

private:
  //! Update the current frame
  bool updateFrame();

  //! Handle playback state updates and frame advancement
  void updatePlaybackState();

  //! Convert Ravl2 VideoFrame to QPixmap
  QPixmap convertFrameToPixmap(const std::shared_ptr<Ravl2::Video::VideoFrameBase>&frame);

  //! Get video stream information as a string
  QString getVideoInfo() const;

  std::shared_ptr<Ravl2::Video::MediaContainer> mediaContainer; //!< The media container
  std::shared_ptr<Ravl2::Video::StreamIterator> videoIterator; //!< Iterator for the video stream
  std::size_t videoStreamIndex = 0; //!< Index of the video stream in the container
  QMutex mutex; //!< Mutex for thread safety
  QPixmap currentFrame; //!< Current frame as a QPixmap
  bool isPlaying = false; //!< Flag indicating whether playback is active
};
