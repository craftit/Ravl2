#include "Ravl2/Video/FfmpegMediaContainer.hh"
#include <iostream>
#include <iomanip>
#include <string>

// A simple example program that reads an MP4 file and prints information about its streams
int main(int argc, char *argv[]) {
  if (argc < 2) {
    std::cerr << "Usage: " << argv[0] << " <video_file.mp4>" << std::endl;
    return 1;
  }

  std::string filePath = argv[1];
  std::cout << "Opening file: " << filePath << std::endl;

  // Open the media container
  auto result = Ravl2::Video::FfmpegMediaContainer::openFile(filePath);
  if (!result.isSuccess()) {
    std::cerr << "Failed to open file. Error code: " << static_cast<int>(result.error()) << std::endl;
    return 1;
  }

  auto container = result.value();
  std::cout << "File opened successfully!" << std::endl;

  // Print container metadata
  std::cout << "\nContainer Metadata:\n";
  std::cout << "===================" << std::endl;
  auto metadata = container->metadata();
  if (metadata.empty()) {
    std::cout << "No metadata available" << std::endl;
  } else {
    for (const auto& [key, value] : metadata) {
      std::cout << key << ": " << value << std::endl;
    }
  }

  // Print container duration
  auto duration = container->duration();
  double durationSecs = std::chrono::duration<double>(duration).count();
  std::cout << "\nDuration: " << durationSecs << " seconds" << std::endl;

  // Print information about each stream
  std::cout << "\nStreams Information:\n";
  std::cout << "===================" << std::endl;
  std::cout << "Total streams: " << container->streamCount() << std::endl;

  for (std::size_t i = 0; i < container->streamCount(); ++i) {
    auto type = container->streamType(i);
    std::cout << "\nStream #" << i << ":" << std::endl;

    switch (type) {
      case Ravl2::Video::StreamType::Video: {
        auto props = container->videoProperties(i);
        if (props.isSuccess()) {
          const auto& videoProps = props.value();
          std::cout << "  Type: Video" << std::endl;
          std::cout << "  Codec: " << videoProps.codec.name << " (" << videoProps.codec.longName << ")" << std::endl;
          std::cout << "  Resolution: " << videoProps.width << "x" << videoProps.height << std::endl;
          std::cout << "  Frame rate: " << std::fixed << std::setprecision(2) << videoProps.frameRate << " fps";
          if (videoProps.isVariableFrameRate) {
            std::cout << " (variable)";
          }
          std::cout << std::endl;
          std::cout << "  Pixel format: " << videoProps.pixelFormat << std::endl;
          std::cout << "  Duration: " << std::chrono::duration<double>(videoProps.duration).count() << " seconds" << std::endl;
          std::cout << "  Frames: " << videoProps.totalFrames << std::endl;
        } else {
          std::cout << "  Failed to get video properties. Error code: " << static_cast<int>(props.error()) << std::endl;
        }
        break;
      }
      case Ravl2::Video::StreamType::Audio: {
        auto props = container->audioProperties(i);
        if (props.isSuccess()) {
          const auto& audioProps = props.value();
          std::cout << "  Type: Audio" << std::endl;
          std::cout << "  Codec: " << audioProps.codec.name << " (" << audioProps.codec.longName << ")" << std::endl;
          std::cout << "  Sample rate: " << audioProps.sampleRate << " Hz" << std::endl;
          std::cout << "  Channels: " << audioProps.channels << std::endl;
          std::cout << "  Bits per sample: " << audioProps.bitsPerSample << std::endl;
          std::cout << "  Duration: " << std::chrono::duration<double>(audioProps.duration).count() << " seconds" << std::endl;
          std::cout << "  Total samples: " << audioProps.totalSamples << std::endl;
        } else {
          std::cout << "  Failed to get audio properties. Error code: " << static_cast<int>(props.error()) << std::endl;
        }
        break;
      }
      case Ravl2::Video::StreamType::Data: {
        auto props = container->dataProperties(i);
        if (props.isSuccess()) {
          const auto& dataProps = props.value();
          std::cout << "  Type: Data" << std::endl;
          std::cout << "  Format: " << dataProps.format << std::endl;
          std::cout << "  Data type: " << dataProps.dataTypeName << std::endl;
          std::cout << "  Duration: " << std::chrono::duration<double>(dataProps.duration).count() << " seconds" << std::endl;
          std::cout << "  Total items: " << dataProps.totalItems << std::endl;
        } else {
          std::cout << "  Failed to get data properties. Error code: " << static_cast<int>(props.error()) << std::endl;
        }
        break;
      }
      case Ravl2::Video::StreamType::Subtitle:
        std::cout << "  Type: Subtitle" << std::endl;
        // For subtitles, we could add more specific info if needed
        break;
      case Ravl2::Video::StreamType::Unknown:
      default:
        std::cout << "  Type: Unknown" << std::endl;
        break;
    }
  }

  // Close the container
  auto closeResult = container->close();
  if (!closeResult.isSuccess()) {
    std::cerr << "Warning: Failed to close container properly. Error code: " << static_cast<int>(closeResult.error()) << std::endl;
  }

  std::cout << "\nMedia container closed." << std::endl;
  return 0;
}
