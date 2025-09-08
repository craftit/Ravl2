#include "Ravl2/Video/FfmpegMediaContainer.hh"
#include <fmt/core.h>
#include <fmt/format.h>
#include <string>
#include <CLI/CLI.hpp>
#include <spdlog/spdlog.h>

#include "Ravl2/config.hh"
#include "Ravl2/IO/OutputSequence.hh"
#include "Ravl2/IO/InputSequence.hh"
#include "Ravl2/Resource.hh"
#include "Ravl2/Video/VideoFrame.hh"
#include "Ravl2/Video/StreamIterator.hh"
#include "Ravl2/OpenCV/ImageIO.hh"
#include "Ravl2/OpenCV/Display.hh"
#include "Ravl2/Pixel/Colour.hh"


// A simple example program that reads an MP4 file and prints information about its streams
int main(int argc, char *argv[])
{
  Ravl2::initOpenCVImageIO();
  Ravl2::initColourConversion();

  CLI::App app{"Media container information example program"};

  std::string filePath;
  std::string outPath = "display://Video";
  bool verbose = false;
  bool show_version = false;
  bool seq = false;
  using PixelT = Ravl2::PixelYUV8;

  // Define command line options
  app.add_option("file", filePath, "Input media file path")->required();
  app.add_option("-o,--output", outPath, "Output path for frames");
  app.add_flag("-s,--seq", seq, "Process sequence (loop through frames)");
  app.add_flag("-v,--verbose", verbose, "Verbose mode");
  app.add_flag("--version", show_version, "Show version information");

  // Parse command line arguments
  CLI11_PARSE(app, argc, argv);

  if (show_version) {
    fmt::print("{}\n", Ravl2::cmake::project_version);
    return EXIT_SUCCESS;
  }

  if (verbose) {
    fmt::print("Opening file: {}\n", filePath);
  }

  // Open the media container
  auto result = Ravl2::Video::FfmpegMediaContainer::openFile(filePath);
  if (!result.isSuccess()) {
    SPDLOG_ERROR("Failed to open file. Error code: {}", static_cast<int>(result.error()));
    return EXIT_FAILURE;
  }

  auto container = result.value();
  if (verbose) {
    fmt::print("File opened successfully!\n");
  }

  // Print container metadata
  fmt::print("\nContainer Metadata:\n");
  fmt::print("===================\n");
  auto metadata = container->metadata();
  if (metadata.empty()) {
    fmt::print("No metadata available\n");
  } else {
    for (const auto& [key, value] : metadata) {
      fmt::print("{}: {}\n", key, value);
    }
  }

  // Print container duration
  auto duration = container->duration();
  double durationSecs = std::chrono::duration<double>(duration).count();
  fmt::print("\nDuration: {:.2f} seconds\n", durationSecs);

  // Print information about each stream
  fmt::print("\nStreams Information:\n");
  fmt::print("===================\n");
  fmt::print("Total streams: {}\n", container->streamCount());

  for (std::size_t i = 0; i < container->streamCount(); ++i) {
    auto type = container->streamType(i);
    fmt::print("\nStream #{}:\n", i);

    switch (type) {
      case Ravl2::Video::StreamType::Video: {
        auto props = container->videoProperties(i);
        if (props.isSuccess()) {
          const auto& videoProps = props.value();
          fmt::print("  Type: Video\n");
          fmt::print("  Codec: {} ({})\n", videoProps.codec.name, videoProps.codec.longName);
          fmt::print("  Resolution: {}x{}\n", videoProps.width, videoProps.height);
          fmt::print("  Frame rate: {:.2f} fps", videoProps.frameRate);
          if (videoProps.isVariableFrameRate) {
            fmt::print(" (variable)");
          }
          fmt::print("\n");
          fmt::print("  Pixel format: {}\n", videoProps.pixelFormat);
          fmt::print("  Duration: {:.2f} seconds\n", std::chrono::duration<double>(videoProps.duration).count());
          fmt::print("  Frames: {}\n", videoProps.totalFrames);
        } else {
          SPDLOG_ERROR("Failed to get video properties. Error code: {}", static_cast<int>(props.error()));
        }
        break;
      }
      case Ravl2::Video::StreamType::Audio: {
        auto props = container->audioProperties(i);
        if (props.isSuccess()) {
          const auto& audioProps = props.value();
          fmt::print("  Type: Audio\n");
          fmt::print("  Codec: {} ({})\n", audioProps.codec.name, audioProps.codec.longName);
          fmt::print("  Sample rate: {} Hz\n", audioProps.sampleRate);
          fmt::print("  Channels: {}\n", audioProps.channels);
          fmt::print("  Bits per sample: {}\n", audioProps.bitsPerSample);
          fmt::print("  Duration: {:.2f} seconds\n", std::chrono::duration<double>(audioProps.duration).count());
          fmt::print("  Total samples: {}\n", audioProps.totalSamples);
        } else {
          SPDLOG_ERROR("Failed to get audio properties. Error code: {}", static_cast<int>(props.error()));
        }
        break;
      }
      case Ravl2::Video::StreamType::Data: {
        auto props = container->dataProperties(i);
        if (props.isSuccess()) {
          const auto& dataProps = props.value();
          fmt::print("  Type: Data\n");
          fmt::print("  Format: {}\n", dataProps.format);
          fmt::print("  Data type: {}\n", dataProps.dataTypeName);
          fmt::print("  Duration: {:.2f} seconds\n", std::chrono::duration<double>(dataProps.duration).count());
          fmt::print("  Total items: {}\n", dataProps.totalItems);
        } else {
          SPDLOG_ERROR("Failed to get data properties. Error code: {}", static_cast<int>(props.error()));
        }
        break;
      }
      case Ravl2::Video::StreamType::Subtitle:
        fmt::print("  Type: Subtitle\n");
        // For subtitles, we could add more specific info if needed
        break;
      case Ravl2::Video::StreamType::Unknown:
      default:
        fmt::print("  Type: Unknown\n");
        break;
    }
  }

  // Process video frames if requested
  if (seq) {
    // Find the first video stream
    std::size_t videoStreamIndex = container->streamCount();
    for (std::size_t i = 0; i < container->streamCount(); ++i) {
      if (container->streamType(i) == Ravl2::Video::StreamType::Video) {
        videoStreamIndex = i;
        break;
      }
    }

    if (videoStreamIndex == container->streamCount()) {
      SPDLOG_ERROR("No video stream found in the file");
      return EXIT_FAILURE;
    }

    // Create stream iterator for the video stream
    auto iterResult = container->createIterator(videoStreamIndex);
    if (!iterResult.isSuccess()) {
      SPDLOG_ERROR("Failed to create iterator for video stream. Error code: {}",
                  static_cast<int>(iterResult.error()));
      return EXIT_FAILURE;
    }
    Ravl2::Video::VideoStreamIterator<PixelT> iterator(iterResult.value());

    // Set up an output stream if needed
    Ravl2::StreamOutputProxy<Ravl2::Array<PixelT,2>> outputStream;
    if ( !outPath.empty()) {
      outputStream = Ravl2::openOutputStream<Ravl2::Array<PixelT,2>>(outPath,Ravl2::defaultSaveFormatHint(true));
      if (!outputStream.valid()) {
        SPDLOG_ERROR("Failed to open output stream at '{}'", outPath);
        return EXIT_FAILURE;
      }
      if (verbose) {
        fmt::print("Output stream opened at '{}'\n", outPath);
      }
    }

    // Process frames
    int frameCount = 0;
    std::chrono::steady_clock::duration totalTime{};
    fmt::print("Processing frames... {} \n", iterator.isValid());
    while (iterator.isValid()) {
      auto startTime = std::chrono::steady_clock::now();

      // Get the next frame
      auto frameResult = iterator.videoFrame();

      if (outputStream.valid()) {
        fmt::print("Writing frame {} to output stream\n", frameCount);
        outputStream.put(frameResult);
        Ravl2::waitKey(1);
      }

      // Move to the next frame
      auto nextResult = iterator.next();
      if (!nextResult.isSuccess()) {
        if (nextResult.error() != Ravl2::Video::VideoErrorCode::EndOfStream)
        {
          SPDLOG_ERROR("Failed to advance to next frame. Error code: {}",
                      static_cast<int>(nextResult.error()));
        }
        break;
      }

      auto endTime = std::chrono::steady_clock::now();
      totalTime += (endTime - startTime);
      frameCount++;

      if (verbose && frameCount % 100 == 0) {
        fmt::print("Processed {} frames\n", frameCount);
      }
    }

    if (frameCount > 0) {
      double timeSeconds = std::chrono::duration<double>(totalTime).count();
      double fps = frameCount / timeSeconds;
      fmt::print("\nProcessed {} frames in {:.2f} seconds ({:.2f} fps)\n",
                frameCount, timeSeconds, fps);
    }

    Ravl2::waitKey(0);
  }

  // Close the container
  auto closeResult = container->close();
  if (!closeResult.isSuccess()) {
    SPDLOG_ERROR("Warning: Failed to close container properly. Error code: {}", static_cast<int>(closeResult.error()));
  }

  if (verbose) {
    fmt::print("\nMedia container closed.\n");
  }


  return EXIT_SUCCESS;
}
