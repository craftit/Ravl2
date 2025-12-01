//
// Example program to read and display GoPro GPMF metadata from video files
//

#include "Ravl2/Video/FfmpegMediaContainer.hh"
#include "Ravl2/Video/FfmpegMultiStreamIterator.hh"
#include <fmt/core.h>
#include <fmt/format.h>
#include <string>
#include <CLI/CLI.hpp>
#include <spdlog/spdlog.h>
#include <nlohmann/json.hpp>

#include "Ravl2/config.hh"
#include "Ravl2/Resource.hh"
#include "Ravl2/EntryPnt.hh"

#ifdef WITH_GPMF
#include "Ravl2/GoPro/GpmfTypes.hh"
#include "Ravl2/Video/MetaDataFrame.hh"
#endif

//! Example program that reads GoPro GPMF metadata from a video file
int RAVL2_MAIN(int argc, char *argv[])
{
  using namespace std::chrono_literals;

  CLI::App app{"GoPro GPMF metadata reader example"};

  std::string filePath;
  bool verbose = false;
  bool show_version = false;
  int maxFrames = 100;

  // Define command line options
  app.add_option("file", filePath, "Input GoPro video file path")->required();
  app.add_option("-m,--max", maxFrames, "Maximum number of metadata frames to process");
  app.add_flag("-v,--verbose", verbose, "Verbose mode");
  app.add_flag("--version", show_version, "Show version information");

  // Parse command line arguments
  CLI11_PARSE(app, argc, argv);

  if (show_version) {
    fmt::print("{}\n", Ravl2::cmake::project_version);
    return EXIT_SUCCESS;
  }

#ifndef WITH_GPMF
  fmt::print("Error: This example requires GoPro GPMF support to be enabled.\n");
  fmt::print("Rebuild with WITH_GPMF=ON to enable GoPro metadata parsing.\n");
  return EXIT_FAILURE;
#else

  // Verify file exists
  std::filesystem::path p(filePath);
  if (!std::filesystem::exists(p)) {
    SPDLOG_ERROR("File not found: {}", filePath);
    return EXIT_FAILURE;
  }

  if (verbose) {
    spdlog::set_level(spdlog::level::debug);
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
  for (const auto& [key, value] : metadata) {
    fmt::print("{}: {}\n", key, value);
  }

  // Find all DATA streams (potential GPMF streams)
  std::vector<std::size_t> dataStreamIndices;
  fmt::print("\nStreams:\n");
  fmt::print("========\n");
  for (std::size_t i = 0; i < container->streamCount(); ++i) {
    auto type = container->streamType(i);
    fmt::print("Stream #{}: ", i);

    switch (type) {
      case Ravl2::Video::StreamType::Video:
        fmt::print("Video\n");
        break;
      case Ravl2::Video::StreamType::Audio:
        fmt::print("Audio\n");
        break;
      case Ravl2::Video::StreamType::Data:
        fmt::print("Data (potential GPMF metadata)\n");
        dataStreamIndices.push_back(i);
        break;
      case Ravl2::Video::StreamType::Subtitle:
        fmt::print("Subtitle\n");
        break;
      default:
        fmt::print("Unknown\n");
        break;
    }
  }

  if (dataStreamIndices.empty()) {
    fmt::print("\nNo DATA streams found in this file.\n");
    fmt::print("This may not be a GoPro video file with GPMF metadata.\n");
    return EXIT_SUCCESS;
  }

  auto iterator = container->createIterator(std::vector<std::size_t>{}).value();

  // Process frames and extract GoPro metadata
  fmt::print("\nProcessing GoPro GPMF metadata:\n");
  fmt::print("===============================\n");

  int frameCount = 0;
  int gpsCount = 0;
  int gyroCount = 0;
  int accelCount = 0;

  while (!iterator->isAtEnd() && frameCount < maxFrames) {
    auto frame = iterator->currentFrame();

    if (frame) {
      // Check if this is a GPS frame
      auto* gpsFrame = dynamic_cast<Ravl2::Video::MetaDataFrame<Ravl2::GoPro::GpsFix>*>(frame.get());
      if (gpsFrame) {
        gpsCount++;
        if (verbose || gpsCount == 1) {
          const auto& fix = gpsFrame->data();
          auto timestamp = gpsFrame->timestamp();
          double timeSecs = std::chrono::duration<double>(timestamp).count();

          fmt::print("\nGPS Fix #{} at {:.3f}s:\n", gpsCount, timeSecs);
          fmt::print("  Location: {:.6f}°, {:.6f}° (alt: {:.1f}m)\n",
                    fix.latitude(), fix.longitude(), fix.height());
          fmt::print("  Speed: {:.2f} m/s (2D), {:.2f} m/s (3D)\n",
                    fix.speed2d(), fix.speed3d());
          fmt::print("  Fix type: {}, Satellites: {} precision: {} \n",
                    fix.fix, fix.satellites, fix.precision);
        }
      }

      // Check if this is a gyroscope frame
      auto* gyroFrame = dynamic_cast<Ravl2::Video::MetaDataFrame<Ravl2::GoPro::GyroSamples>*>(frame.get());
      if (gyroFrame) {
        gyroCount++;
        if (verbose || gyroCount == 1) {
          const auto& gyroData = gyroFrame->data();
          auto timestamp = gyroFrame->timestamp();
          double timeSecs = std::chrono::duration<double>(timestamp).count();

          fmt::print("\nGyro Frame #{} at {:.3f}s:\n", gyroCount, timeSecs);
          fmt::print("  Samples: {}, Rate: {:.1f} Hz\n",
                    gyroData.size(), gyroData.sampleRate);
          if (!gyroData.samples.empty()) {
            const auto& first = gyroData.samples[0];
            fmt::print("  First sample: [{:.3f}, {:.3f}, {:.3f}] rad/s\n",
                      first[0], first[1], first[2]);
          }
        }
      }

      // Check if this is an accelerometer frame
      auto* accelFrame = dynamic_cast<Ravl2::Video::MetaDataFrame<Ravl2::GoPro::AccelSamples>*>(frame.get());
      if (accelFrame) {
        accelCount++;
        if (verbose || accelCount == 1) {
          const auto& accelData = accelFrame->data();
          auto timestamp = accelFrame->timestamp();
          double timeSecs = std::chrono::duration<double>(timestamp).count();

          fmt::print("\nAccel Frame #{} at {:.3f}s:\n", accelCount, timeSecs);
          fmt::print("  Samples: {}, Rate: {:.1f} Hz\n",
                    accelData.size(), accelData.sampleRate);
          if (!accelData.samples.empty()) {
            const auto& first = accelData.samples[0];
            fmt::print("  First sample: [{:.2f}, {:.2f}, {:.2f}] m/s²\n",
                      first[0], first[1], first[2]);
          }
        }
      }

      auto* jsonFrame = dynamic_cast<Ravl2::Video::MetaDataFrame<nlohmann::json> *>(frame.get());
      if(jsonFrame) {
        if (verbose) {
          fmt::print("Json: {}\n", jsonFrame->data().dump(2));
        }
      }

    }

    // Move to the next frame
    auto nextResult = iterator->next();
    if (!nextResult.isSuccess()) {
      if (nextResult.error() != Ravl2::Video::VideoErrorCode::EndOfStream) {
        SPDLOG_ERROR("Failed to advance to next frame. Error code: {}",
                    static_cast<int>(nextResult.error()));
      }
      break;
    }

    frameCount++;

    if (!verbose && frameCount % 100 == 0) {
      fmt::print("Processed {} frames...\n", frameCount);
    }
  }

  // Print summary
  fmt::print("\n\nSummary:\n");
  fmt::print("========\n");
  fmt::print("Total frames processed: {}\n", frameCount);
  fmt::print("GPS fixes found: {}\n", gpsCount);
  fmt::print("Gyro frames found: {}\n", gyroCount);
  fmt::print("Accel frames found: {}\n", accelCount);

  // Close the container
  auto closeResult = container->close();
  if (!closeResult.isSuccess()) {
    SPDLOG_ERROR("Warning: Failed to close container properly. Error code: {}",
                static_cast<int>(closeResult.error()));
  }

  return EXIT_SUCCESS;
#endif
}
