#include "Ravl2/Video/FfmpegMediaContainer.hh"
#include <fmt/core.h>
#include <fmt/format.h>
#include <string>
#include <CLI/CLI.hpp>
#include <spdlog/spdlog.h>

#include "Ravl2/config.hh"
#include "Ravl2/Video/VideoFrame.hh"
#include "Ravl2/Video/StreamIterator.hh"
#include "Ravl2/Display/DebugDisplay.hh"
#include "Ravl2/ImageIO/ImageIOInit.hh"
#include "Ravl2/Pixel/Colour.hh"
#include "Ravl2/IO/OutputSequence.hh"
#include "Ravl2/EntryPnt.hh"

// Example program that captures video from a webcam
int RAVL2_MAIN(int argc, char **argv)
{
  Ravl2::initImageIO();
  Ravl2::DebugDisplay::initDisplay();

  Ravl2::initColourConversion();
  Ravl2::initPlaneConversion();

  CLI::App app{"Webcam capture example program"};

  std::string devicePath;
  std::string outPath = "display://Webcam";
  int width = 0;
  int height = 0;
  int frameRate = 0;
  int maxFrames = 0;
  bool listDevices = false;
  bool verbose = true;
  bool show_version = false;
  std::string pixelFormat;

  using PixelT = Ravl2::PixelRGB8;

  // Define command line options
  app.add_option("-d,--device", devicePath, "Device path (e.g., /dev/video0, empty for default)");
  app.add_option("-o,--output", outPath, "Output path for frames (default: display://Webcam)");
  app.add_option("-w,--width", width, "Requested capture width (0 for device default)");
  app.add_option("--height", height, "Requested capture height (0 for device default)");
  app.add_option("-f,--fps", frameRate, "Requested frame rate (0 for device default)");
  app.add_option("-p,--pixel-format", pixelFormat, "Requested pixel format (e.g., mjpeg, yuyv422)");
  app.add_option("-n,--max-frames", maxFrames, "Maximum number of frames to capture (0 for unlimited)");
  app.add_flag("-l,--list", listDevices, "List available capture devices");
  app.add_flag("-v,--verbose", verbose, "Verbose mode");
  app.add_flag("--version", show_version, "Show version information");

  // Parse command line arguments
  CLI11_PARSE(app, argc, argv);

  if (show_version) {
    fmt::print("{}\n", Ravl2::cmake::project_version);
    return EXIT_SUCCESS;
  }

  // List devices if requested
  if (listDevices) {
    fmt::print("Available capture devices:\n");
    fmt::print("=========================\n");

    auto devicesResult = Ravl2::Video::enumerateDevices();
    if (!devicesResult.isSuccess()) {
      SPDLOG_ERROR("Failed to enumerate devices. Error code: {}", static_cast<int>(devicesResult.error()));
      return EXIT_FAILURE;
    }

    const auto& devices = devicesResult.value();
    if (devices.empty()) {
      fmt::print("No capture devices found.\n");
    } else {
      for (const auto& device : devices) {
        fmt::print("\nDevice: {}\n", device.path);
        fmt::print("  Name: {}\n", device.name);
        fmt::print("  Driver: {}\n", device.driver);
        fmt::print("  Bus Info: {}\n", device.busInfo);
        if (!device.supportedFormats.empty()) {
          fmt::print("  Supported formats:\n");
          for (const auto& format : device.supportedFormats) {
            fmt::print("    - {}\n", format);
          }
        }
      }
    }

    return EXIT_SUCCESS;
  }

  // Set up device parameters
  Ravl2::Video::DeviceParameters params;
  params.devicePath = devicePath;
  params.width = width;
  params.height = height;
  params.frameRate = static_cast<float>(frameRate);
  params.pixelFormat = pixelFormat;

  if (verbose) {
    fmt::print("Opening capture device...\n");
    if (!devicePath.empty()) {
      fmt::print("  Device: {}\n", devicePath);
    } else {
      fmt::print("  Device: (default)\n");
    }
    if (width > 0 && height > 0) {
      fmt::print("  Resolution: {}x{}\n", width, height);
    }
    if (frameRate > 0) {
      fmt::print("  Frame rate: {} fps\n", frameRate);
    }
    if (!pixelFormat.empty()) {
      fmt::print("  Pixel format: {}\n", pixelFormat);
    }
  }

  // Open the capture device
  auto result = Ravl2::Video::MediaContainer::openDevice(params);
  if (!result.isSuccess()) {
    SPDLOG_ERROR("Failed to open capture device. Error code: {}", static_cast<int>(result.error()));
    return EXIT_FAILURE;
  }

  auto container = result.value();
  if (verbose) {
    fmt::print("Device opened successfully!\n");
  }

  // Print stream information
  fmt::print("\nStream Information:\n");
  fmt::print("===================\n");
  fmt::print("Total streams: {}\n", container->streamCount());

  for (std::size_t i = 0; i < container->streamCount(); ++i) {
    auto type = container->streamType(i);
    fmt::print("\nStream #{}:\n", i);

    if (type == Ravl2::Video::StreamType::Video) {
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
      }
    }
  }

  // Find the first video stream
  std::size_t videoStreamIndex = container->streamCount();
  for (std::size_t i = 0; i < container->streamCount(); ++i) {
    if (container->streamType(i) == Ravl2::Video::StreamType::Video) {
      videoStreamIndex = i;
      break;
    }
  }

  if (videoStreamIndex == container->streamCount()) {
    SPDLOG_ERROR("No video stream found in the device");
    return EXIT_FAILURE;
  }

  // Create stream iterator for the video stream
  auto iterResult = container->createIterator(videoStreamIndex);
  if (!iterResult.isSuccess()) {
    SPDLOG_ERROR("Failed to create iterator for video stream. Error code: {}",
                static_cast<int>(iterResult.error()));
    return EXIT_FAILURE;
  }

  using ImageT = Ravl2::Array<PixelT, 2>;
  Ravl2::Video::VideoStreamIterator<ImageT> iterator(iterResult.value());

  // Set up an output stream if needed
  Ravl2::StreamOutputProxy<ImageT> outputStream;
  if (!outPath.empty()) {
    outputStream = Ravl2::openOutputStream<ImageT>(outPath, Ravl2::defaultSaveFormatHint(verbose));
    if (!outputStream.valid()) {
      SPDLOG_ERROR("Failed to open output stream at '{}'", outPath);
      return EXIT_FAILURE;
    }
    if (verbose) {
      fmt::print("Output stream opened at '{}'\n", outPath);
    }
  }

  // Capture frames
  int frameCount = 0;
  auto startTime = std::chrono::steady_clock::now();
  fmt::print("\nCapturing frames...\n");
  fmt::print("Press Ctrl+C to stop.\n\n");

  while (iterator.isValid()) {
    // Get the current frame
    auto frameResult = iterator.videoFrame();

    if (outputStream.valid()) {
      outputStream.put(frameResult);
    }

    frameCount++;

    if (verbose && frameCount % 30 == 0) {
      auto currentTime = std::chrono::steady_clock::now();
      auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(currentTime - startTime).count();
      if (elapsed > 0) {
        double fps = static_cast<double>(frameCount) / static_cast<double>(elapsed);
        fmt::print("Captured {} frames ({:.2f} fps)\n", frameCount, fps);
      }
    }

    // Check if we've reached the maximum frame count
    if (maxFrames > 0 && frameCount >= maxFrames) {
      fmt::print("Reached maximum frame count of {}.\n", maxFrames);
      break;
    }

    // Move to the next frame
    auto nextResult = iterator.next();
    if (!nextResult.isSuccess()) {
      if (nextResult.error() != Ravl2::Video::VideoErrorCode::EndOfStream) {
        SPDLOG_ERROR("Failed to advance to next frame. Error code: {}",
                    static_cast<int>(nextResult.error()));
      }
      break;
    }
  }

  auto endTime = std::chrono::steady_clock::now();
  auto totalSeconds = std::chrono::duration_cast<std::chrono::seconds>(endTime - startTime).count();

  if (frameCount > 0 && totalSeconds > 0) {
    double fps = static_cast<double>(frameCount) / static_cast<double>(totalSeconds);
    fmt::print("\nCaptured {} frames in {} seconds ({:.2f} fps)\n",
              frameCount, totalSeconds, fps);
  } else {
    fmt::print("\nCaptured {} frames\n", frameCount);
  }

  // Close the container
  auto closeResult = container->close();
  if (!closeResult.isSuccess()) {
    SPDLOG_ERROR("Warning: Failed to close container properly. Error code: {}",
                static_cast<int>(closeResult.error()));
  }

  if (verbose) {
    fmt::print("\nCapture device closed.\n");
  }

  return EXIT_SUCCESS;
}
