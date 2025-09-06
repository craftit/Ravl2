#include "Ravl2/Video/FfmpegMediaContainer.hh"
#include <fmt/core.h>
#include <fmt/format.h>
#include <string>
#include <CLI/CLI.hpp>
#include <spdlog/spdlog.h>

#include "Ravl2/config.hh"

// A simple example program that reads an MP4 file and prints information about its streams
int main(int argc, char *argv[]) {
  CLI::App app{"Media container information example program"};

  std::string filePath;
  bool verbose = false;
  bool show_version = false;

  // Define command line options
  app.add_option("file", filePath, "Input media file path")->required();
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
