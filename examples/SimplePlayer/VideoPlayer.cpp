//
// Created on September 12, 2025
//

#include "VideoPlayer.h"
#include "Ravl2/Video/MediaContainer.hh"
#include "Ravl2/Video/StreamIterator.hh"
#include "Ravl2/Video/VideoFrame.hh"
#include "Ravl2/Video/VideoTypes.hh"

#include <QImage>
#include <QMutexLocker>
#include <QDebug>
#include <QDateTime>

VideoPlayer::VideoPlayer(QObject *parent)
    : QObject(parent)
{
}

VideoPlayer::~VideoPlayer()
{
    //! Clean up resources
    if (mediaContainer) {
        mediaContainer->close();
    }
}

bool VideoPlayer::openFile(const QString &filePath)
{
    QMutexLocker locker(&mutex);

    //! Close any existing container
    if (mediaContainer) {
        mediaContainer->close();
        mediaContainer.reset();
        videoIterator.reset();
    }

    //! Open the file with Ravl2 MediaContainer
    auto result = Ravl2::Video::MediaContainer::openFile(filePath.toStdString());
    if (!result.isSuccess()) {
        qWarning() << "Failed to open file:" << filePath
                   << "Error:" << QString::fromUtf8(Ravl2::Video::toString(result.error()));
        return false;
    }

    mediaContainer = result.value();

    //! Find the first video stream
    bool foundVideoStream = false;
    for (std::size_t i = 0; i < mediaContainer->streamCount(); ++i) {
        if (mediaContainer->streamType(i) == Ravl2::Video::StreamType::Video) {
            videoStreamIndex = i;
            foundVideoStream = true;
            break;
        }
    }

    if (!foundVideoStream) {
        qWarning() << "No video stream found in file:" << filePath;
        mediaContainer->close();
        mediaContainer.reset();
        return false;
    }

    //! Create an iterator for the video stream
    auto iteratorResult = mediaContainer->createIterator(videoStreamIndex);
    if (!iteratorResult.isSuccess()) {
        qWarning() << "Failed to create stream iterator. Error:"
                   << QString::fromUtf8(Ravl2::Video::toString(iteratorResult.error()));
        mediaContainer->close();
        mediaContainer.reset();
        return false;
    }

    videoIterator = iteratorResult.value();

    //! Get the first frame
    if (!updateFrame()) {
        qWarning() << "Failed to get the first video frame";
        mediaContainer->close();
        mediaContainer.reset();
        videoIterator.reset();
        return false;
    }

    //! Emit signals with video information
    emit durationChanged(getDuration());
    emit positionChanged(getPosition());
    emit videoInfoChanged(getVideoInfo());

    return true;
}

void VideoPlayer::play()
{
    if (!mediaContainer || !videoIterator) {
        return;
    }

    isPlaying = true;
}

void VideoPlayer::pause()
{
    isPlaying = false;
}

void VideoPlayer::stop()
{
    QMutexLocker locker(&mutex);

    if (!mediaContainer || !videoIterator) {
        return;
    }

    isPlaying = false;

    //! Seek to the beginning
    auto result = videoIterator->reset();
    if (!result.isSuccess()) {
        qWarning() << "Failed to reset video iterator. Error:"
                   << QString::fromUtf8(Ravl2::Video::toString(result.error()));
        return;
    }

    updateFrame();
    emit positionChanged(getPosition());
}

void VideoPlayer::seek(int64_t position)
{
    QMutexLocker locker(&mutex);

    if (!mediaContainer || !videoIterator) {
        return;
    }

    //! Convert position to Ravl2 MediaTime
    Ravl2::Video::MediaTime timestamp(position);

    //! Seek to the specified position
    auto result = videoIterator->seek(timestamp, Ravl2::Video::SeekFlags::Keyframe);
    if (!result.isSuccess()) {
        qWarning() << "Failed to seek to position:" << position
                   << "Error:" << QString::fromUtf8(Ravl2::Video::toString(result.error()));
        return;
    }

    updateFrame();
    emit positionChanged(getPosition());
}

bool VideoPlayer::nextFrame()
{
    QMutexLocker locker(&mutex);

    if (!mediaContainer || !videoIterator) {
        return false;
    }

    //! Move to the next frame
    auto result = videoIterator->next();
    if (!result.isSuccess()) {
        if (result.error() == Ravl2::Video::VideoErrorCode::EndOfStream) {
            //! Reached the end of the stream
            return false;
        }

        qWarning() << "Failed to move to next frame. Error:"
                   << QString::fromUtf8(Ravl2::Video::toString(result.error()));
        return false;
    }

    updateFrame();
    emit positionChanged(getPosition());
    return true;
}

bool VideoPlayer::previousFrame()
{
    QMutexLocker locker(&mutex);

    if (!mediaContainer || !videoIterator) {
        return false;
    }

    //! Check if we can seek backward
    if (!videoIterator->canSeek()) {
        return false;
    }

    //! Move to the previous frame
    auto result = videoIterator->previous();
    if (!result.isSuccess()) {
        qWarning() << "Failed to move to previous frame. Error:"
                   << QString::fromUtf8(Ravl2::Video::toString(result.error()));
        return false;
    }

    updateFrame();
    emit positionChanged(getPosition());
    return true;
}

int64_t VideoPlayer::getPosition() const
{
    if (!videoIterator) {
        return 0;
    }

    return videoIterator->position().count();
}

int64_t VideoPlayer::getDuration() const
{
    if (!mediaContainer) {
        return 0;
    }

    return mediaContainer->duration().count();
}

bool VideoPlayer::isFileLoaded() const
{
    return (mediaContainer && videoIterator);
}

QPixmap VideoPlayer::getCurrentFrame()
{
    QMutexLocker locker(&mutex);

    if (isPlaying) {
        updatePlaybackState();
    }

    return currentFrame;
}

void VideoPlayer::updatePlaybackState()
{
    if (isPlaying && mediaContainer && videoIterator) {
        //! Check if it's time to move to the next frame
        int64_t currentTime = QDateTime::currentMSecsSinceEpoch();

        if (lastFrameTime == 0) {
            lastFrameTime = currentTime;
            return;
        }

        //! Get video properties to calculate frame rate
        auto propertiesResult = mediaContainer->videoProperties(videoStreamIndex);
        if (!propertiesResult.isSuccess()) {
            return;
        }

        auto properties = propertiesResult.value();

        //! Calculate time per frame in milliseconds
        float frameRateMs = 1000.0f / properties.frameRate;

        //! Check if enough time has passed for the next frame
        if (currentTime - lastFrameTime >= frameRateMs) {
            //! Move to the next frame
            auto result = videoIterator->next();
            if (!result.isSuccess()) {
                if (result.error() == Ravl2::Video::VideoErrorCode::EndOfStream) {
                    //! Reached the end of the stream, seek back to beginning
                    videoIterator->reset();
                } else {
                    //! Error moving to next frame
                    qWarning() << "Failed to move to next frame. Error:"
                               << QString::fromUtf8(Ravl2::Video::toString(result.error()));
                    return;
                }
            }

            //! Update the current frame
            updateFrame();

            //! Update the position
            emit positionChanged(getPosition());

            //! Update the lastFrameTime
            lastFrameTime = currentTime;
        }
    }
}

bool VideoPlayer::updateFrame()
{
    if (!videoIterator) {
        return false;
    }

    //! Get the current frame
    auto frame = videoIterator->currentFrame();
    if (!frame) {
        return false;
    }

    //! Check if it's a video frame
    auto videoFrame = std::dynamic_pointer_cast<Ravl2::Video::VideoFrameBase>(frame);
    if (!videoFrame) {
        return false;
    }

    //! Convert frame to QPixmap
    currentFrame = convertFrameToPixmap(videoFrame);
    return !currentFrame.isNull();
}

QPixmap VideoPlayer::convertFrameToPixmap(const std::shared_ptr<Ravl2::Video::VideoFrameBase> &frame)
{
    if (!frame) {
        return QPixmap();
    }

    //! Get frame dimensions
    int width = frame->width();
    int height = frame->height();

    if (width <= 0 || height <= 0) {
        return QPixmap();
    }

    //! Try to get the frame data using the frameData() method which returns std::any
    std::any frameData = frame->frameData();

    //! Check if we have Ravl2's Qt conversion utilities
    //! If so, we'll try to use them to convert the frame to a QImage
    try {
        //! Try to create a QImage using the QImage constructors
        QImage image(width, height, QImage::Format_RGB888);

        //! For demonstration, create a solid color image as fallback
        //! In a real application, you'd need to convert from various pixel formats
        image.fill(Qt::black);

        //! Check if we can access an RGB array from frameData
        //! This part would need to be customized based on your actual frame data type
        if (frameData.has_value()) {
            //! In a real implementation, you would extract the pixel data from frameData
            //! and populate the QImage with it
            qDebug() << "Frame data type: " << QString::fromStdString(frameData.type().name());

            //! Try to use Ravl2's built-in conversion functions if available
            try {
                //! For now, we'll just create a gradient as a placeholder
                for (int y = 0; y < height; ++y) {
                    QRgb *scanLine = reinterpret_cast<QRgb*>(image.scanLine(y));
                    for (int x = 0; x < width; ++x) {
                        //! Create a gradient pattern as a placeholder
                        scanLine[x] = qRgb(
                            static_cast<int>(255.0 * x / width),
                            static_cast<int>(255.0 * y / height),
                            128);
                    }
                }
            } catch (const std::exception &e) {
                qWarning() << "Error converting frame data: " << e.what();
            }
        }

        return QPixmap::fromImage(image);
    } catch (const std::exception &e) {
        qWarning() << "Error creating QImage from frame: " << e.what();
        return QPixmap();
    }
}

QString VideoPlayer::getVideoInfo() const
{
    if (!mediaContainer || !videoIterator) {
        return QString();
    }

    auto propertiesResult = mediaContainer->videoProperties(videoStreamIndex);
    if (!propertiesResult.isSuccess()) {
        return QString("Error getting video properties");
    }

    auto properties = propertiesResult.value();

    return QString("%1x%2, %3 fps, %4 codec")
        .arg(properties.width)
        .arg(properties.height)
        .arg(properties.frameRate, 0, 'f', 2)
        .arg(QString::fromStdString(properties.codec.name));
}
