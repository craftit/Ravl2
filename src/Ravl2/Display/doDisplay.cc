#include <iostream>
#include <string>
#include <thread>
#include <chrono>

#include <spdlog/spdlog.h>

#include "Ravl2/Array.hh"
#include "Ravl2/Pixel/Pixel.hh"
#include "Ravl2/IO/Load.hh"
#include "Ravl2/IO/Save.hh"
#include "Ravl2/IO/OutputSequence.hh"
#include "Ravl2/Resource.hh"
#include "Ravl2/ImageIO/JpegTurboImageIO.hh"
#include "Ravl2/Display/DebugDisplay.hh"
#include "Ravl2/Geometry/PolyLine.hh"
#include "Ravl2/Image/DrawText.hh"
#include "Ravl2/ImageIO/ImageIOInit.hh"

#include <cxxopts.hpp>
#include <cmath>
#include <vector>
#include <unordered_map>

using namespace std::chrono_literals;

int RAVL2_MAIN(int argc, char **argv)
{
  SPDLOG_INFO("Started main.");
  Ravl2::DebugDisplay::initDisplay();
  Ravl2::initImageIO();

  Ravl2::addResourcePath("data", RAVL_SOURCE_DIR "/data");

  std::string imagePath = "lena.jpg";
  cxxopts::Options options(argv[0], "doDisplay");
  try {
    options
      .positional_help("[optional args]")
      .show_positional_help();

    options
      .set_tab_expansion()
      .add_options()("f,filename", "Filename", cxxopts::value<std::string>(imagePath))("help", "Print help");

  } catch(const cxxopts::exceptions::exception &e) {
    SPDLOG_ERROR("error parsing options: {}", e.what());
    exit(1);
  }

  // Set logging early before any DebugDisplay initialization
  spdlog::set_level(spdlog::level::info);

  {
    auto foundFile = Ravl2::findFileResource("data", imagePath);
    if(!foundFile.empty()) {
      imagePath = foundFile;
    }
  }

  using namespace Ravl2;
  using Ravl2::PixelRGB8;

  // --- 2D Image Display Demo ---
  Array<PixelRGB8, 2> imgRgb;
  Array<uint8_t, 2> imgGray;

  bool loaded = false;
  if(!imagePath.empty()) {
    loaded = ioLoad(imgRgb, imagePath);
    if(!loaded) {
      SPDLOG_ERROR("Failed to load image from '{}'. Will generate a synthetic test image instead.", imagePath);
    }
  }

  if(loaded) {
    // Convert RGB8 to grayscale (luminance)
    const int H = imgRgb.range()[0].size();
    const int W = imgRgb.range()[1].size();
    imgGray = Array<uint8_t, 2>({H, W});
    for(int y = 0; y < H; ++y) {
      for(int x = 0; x < W; ++x) {
        auto p = imgRgb[{y, x}];
        const float r = static_cast<float>(p.get<Ravl2::ImageChannel::Red>());
        const float g = static_cast<float>(p.get<Ravl2::ImageChannel::Green>());
        const float b = static_cast<float>(p.get<Ravl2::ImageChannel::Blue>());
        float yv = 0.299f * r + 0.587f * g + 0.114f * b;
        if(yv < 0.f) yv = 0.f;
        if(yv > 255.f) yv = 255.f;
        imgGray[{y, x}] = static_cast<uint8_t>(yv + 0.5f);
      }
    }
    SPDLOG_INFO("Loaded '{}' ({}x{}), converted to grayscale.", imagePath, W, H);
  } else {
    // Generate a small grayscale gradient test image (256x256)
    const int W = 256, H = 256;
    imgGray = Array<uint8_t, 2>({H, W});
    for(int y = 0; y < H; ++y) {
      for(int x = 0; x < W; ++x) {
        uint8_t v = static_cast<uint8_t>((x + y) / 2);
        imgGray[{y, x}] = v;
      }
    }
    SPDLOG_INFO("Generated synthetic {}x{} grayscale image.", W, H);
  }

  // Display grayscale and RGB images
  const std::string channel1 = "display://Image1:Clear";
  if(!ioSave(channel1, imgGray)) {
    SPDLOG_WARN("ioSave('{}', imgGray) did not find a writer.", channel1);
  } else {
    SPDLOG_INFO("Queued image to {}", channel1);
  }

  if(loaded) {
    const std::string channel2 = "display://Image2:Clear";
    if(!ioSave(channel2, imgRgb)) {
      SPDLOG_WARN("ioSave('{}', imgRgb) did not find a writer.", channel2);
    } else {
      SPDLOG_INFO("Queued image to {}", channel2);
    }
  }

  // --- 2D Polyline Overlay Demo ---
  {
    using Poly2f = Ravl2::PolyLine<float, 2>;
    const int H = imgGray.range()[0].size();
    const int W = imgGray.range()[1].size();
    Poly2f poly({{10.f, 10.f}, {static_cast<float>(W - 10), 10.f}, {static_cast<float>(W - 10), static_cast<float>(H - 10)}});
    const std::string overlay1 = "display://Image1:Mode=Append:Color=#00ff00ff:Width=2";
    if(!ioSave(overlay1, poly)) {
      SPDLOG_WARN("ioSave('{}', polyline) did not find a writer.", overlay1);
    } else {
      SPDLOG_INFO("Queued test polyline overlay to {} ({} points)", overlay1, poly.size());
    }
  }

  // --- 3D Point Cloud (scaffolding only — displays grid and camera, no visible points yet) ---
#if 0
  {
    PointSet<float, 3> ps({Point<float, 3> {0.f, 0.f, 0.f},
                           Point<float, 3> {1.f, 0.f, 0.f},
                           Point<float, 3> {0.f, 1.f, 0.f}});
    ioSave("display://Cloud1", ps);
  }
#endif

  // --- Video Sequence (requires loaded image) ---
#if 0
  if(loaded) {
    std::string outPath = "display://Video1";
    auto outputStream = Ravl2::openOutputStream<Ravl2::Array<PixelRGB8,2>>(outPath, Ravl2::defaultSaveFormatHint(true));
    const int maxCount = 20;
    for(int i = 0; i < maxCount; ++i) {
      auto newImg = clone(imgRgb);
      Ravl2::DrawText(newImg, PixelRGB8(255,255,255), Ravl2::Index<2>({10,10}), fmt::format("{}/{}", i, maxCount));
      outputStream.put(newImg);
      std::this_thread::sleep_for(100ms);
    }
  }
#endif
  // Test time series plotting (Phase 7)
  SPDLOG_INFO("Generating test plots...");
  {
    // Static plot with sine and cosine
    std::vector<float> sine_data, cosine_data;
    for(int i = 0; i < 100; ++i) {
      float x = i * 0.1f;
      sine_data.push_back(std::sin(x));
      cosine_data.push_back(std::cos(x));
    }

    ioSave("display://trig:series=sin:mode=replace", sine_data);
    ioSave("display://trig:series=cos:mode=replace", cosine_data);
    SPDLOG_INFO("Plotted sine and cosine to 'trig' channel");

    // Parametric plot: use sin as x-axis, cos as y-axis (creates a circle!)
    ioSave("display://circle:series=x:mode=replace:xaxis=x", sine_data);
    ioSave("display://circle:series=y:mode=replace", cosine_data);
    SPDLOG_INFO("Plotted parametric circle (sin vs cos) to 'circle' channel");

    // Multi-series with vectors: create a Lissajous figure (x=sin(at), y=cos(bt))
    std::vector<float> lissajous_x, lissajous_y, lissajous_z;
    for(int i = 0; i < 200; ++i) {
      float t = i * 0.05f;
      lissajous_x.push_back(std::sin(3.0f * t));
      lissajous_y.push_back(std::cos(4.0f * t));
      lissajous_z.push_back(std::sin(2.0f * t) * 0.5f);
    }
    std::unordered_map<std::string, std::vector<float>> lissajous_data = {
      {"x_axis", lissajous_x},
      {"y_series", lissajous_y},
      {"z_series", lissajous_z}
    };
    ioSave("display://lissajous:mode=replace:xaxis=x_axis", lissajous_data);
    SPDLOG_INFO("Plotted Lissajous figure with map of vectors to 'lissajous' channel");

    // Multi-series plot using map (new feature!)
    SPDLOG_INFO("Streaming multi-series metrics...");
    for(int i = 0; i < 50; ++i) {
      // Simulate multiple metrics at once
      std::unordered_map<std::string, float> metrics = {
        {"cpu_usage", 50.0f + 20.0f * std::sin(i * 0.1f) + (rand() % 100 - 50) / 10.0f},
        {"memory_usage", 70.0f + 15.0f * std::cos(i * 0.15f) + (rand() % 100 - 50) / 10.0f},
        {"network_io", 30.0f + 25.0f * std::sin(i * 0.2f + 1.0f) + (rand() % 100 - 50) / 10.0f}
      };
      ioSave("display://metrics:mode=append", metrics);
      std::this_thread::sleep_for(50ms);
    }
    SPDLOG_INFO("Finished streaming multi-series metrics");

    // Streaming plot with single series (backward compatibility)
    SPDLOG_INFO("Streaming noisy signal...");
    for(int i = 0; i < 50; ++i) {
      float value = std::sin(i * 0.2f) + (rand() % 100 - 50) / 200.0f; // noisy sine
      std::vector<float> sample = {value};
      ioSave("display://noisy:series=signal:mode=append", sample);
      std::this_thread::sleep_for(50ms);
    }
    SPDLOG_INFO("Finished streaming plot");
  }

  // Keep the process alive briefly so the SDL window (from the debug display thread) is visible.
  SPDLOG_INFO("Sample running. Close the debug window to exit, or wait a moment...");
  std::this_thread::sleep_for(30s);

  return 0;
}
