#include <iostream>
#include <string>
#include <thread>
#include <chrono>

#include <spdlog/spdlog.h>

#include "Ravl2/Array.hh"
#include "Ravl2/Pixel/Pixel.hh"
#include "Ravl2/IO/Load.hh"
#include "Ravl2/IO/Save.hh"
#include "Ravl2/Resource.hh"
#include "Ravl2/OpenCV/ImageIO.hh"
#include "Ravl2/Display/DebugDisplay.hh"
#include "Ravl2/Geometry/PolyLine.hh"
#include <cxxopts.hpp>

using namespace std::chrono_literals;


int RAVL2_MAIN(int argc, char** argv)
{
  // Set logging early before any DebugDisplay initialization
  spdlog::set_pattern("[%Y-%m-%d %T.%e] [%^%l%$] [%t] %v");
  spdlog::set_level(spdlog::level::debug);

  Ravl2::DebugDisplay::initDisplay();
  Ravl2::initOpenCVImageIO();

  Ravl2::addResourcePath("data",RAVL_SOURCE_DIR "/data");

  std::string imagePath = "lena.jpg";
  cxxopts::Options options(argv[0], "doDisplay");
  try {
    options
      .positional_help("[optional args]")
      .show_positional_help();

    options
      .set_tab_expansion()
      .add_options()
        ("f,filename", "Filename", cxxopts::value<std::string>(imagePath))
        ("help", "Print help");

  } catch(const cxxopts::exceptions::exception &e) {
    SPDLOG_ERROR("error parsing options: {}", e.what());
    exit(1);
  }

  {
    auto foundFile = Ravl2::findFileResource("data",imagePath);
    if (!foundFile.empty()) {
      imagePath = foundFile;
    }
  }

  using namespace Ravl2;
  using Ravl2::PixelRGB8;

  Array<PixelRGB8, 2> imgRgb;
  Array<uint8_t, 2> imgGray;

  bool loaded = false;
  if (!imagePath.empty()) {
    loaded = ioLoad(imgRgb, imagePath);
    if (!loaded) {
      SPDLOG_ERROR("Failed to load image from '{}'. Will generate a synthetic test image instead.", imagePath);
    }
  }

  if (loaded) {
    // Convert RGB8 to grayscale (luminance)
    const int H = imgRgb.range()[0].size();
    const int W = imgRgb.range()[1].size();
    imgGray = Array<uint8_t, 2>({H, W});
    for (int y = 0; y < H; ++y) {
      for (int x = 0; x < W; ++x) {
        auto p = imgRgb[{y, x}];
        const float r = static_cast<float>(p.get<Ravl2::ImageChannel::Red>());
        const float g = static_cast<float>(p.get<Ravl2::ImageChannel::Green>());
        const float b = static_cast<float>(p.get<Ravl2::ImageChannel::Blue>());
        float yv = 0.299f * r + 0.587f * g + 0.114f * b;
        if (yv < 0.f) yv = 0.f;
        if (yv > 255.f) yv = 255.f;
        imgGray[{y, x}] = static_cast<uint8_t>(yv + 0.5f);
      }
    }
    SPDLOG_INFO("Loaded '{}' ({}x{}), converted to grayscale.", imagePath, W, H);
  } else {
    // Generate a small grayscale gradient test image (256x256)
    const int W = 256, H = 256;
    imgGray = Array<uint8_t, 2>({H, W});
    for (int y = 0; y < H; ++y) {
      for (int x = 0; x < W; ++x) {
        uint8_t v = static_cast<uint8_t>((x + y) / 2);
        imgGray[{y, x}] = v;
      }
    }
    SPDLOG_INFO("Generated synthetic {}x{} grayscale image.", W, H);
  }

  // Start the new debug display subsystem and attempt to display via @debug scheme
  //Ravl2::DebugDisplay::ensureStarted({});

  // Save to the debug display channel. This will enqueue a SetBaseImage2D command via the @debug adapter.
  const std::string channel1 = "@debug:Image1:Clear";
  if (!ioSave(channel1, imgGray)) {
    SPDLOG_WARN("ioSave('{}', imgGray) did not find a writer.", channel1);
  } else {
    SPDLOG_INFO("Queued image to {}", channel1);
  }

  const std::string channel2 = "@debug:Image2:Clear";
  if (!ioSave(channel2, imgGray)) {
    SPDLOG_WARN("ioSave('{}', imgGray) did not find a writer.", channel2);
  } else {
    SPDLOG_INFO("Queued image to {}", channel2);
  }

  // Construct a tiny 3-point set (triangle in XY plane)
  PointSet<float,3> ps({ Point<float,3>{0.f, 0.f, 0.f},
                         Point<float,3>{1.f, 0.f, 0.f},
                         Point<float,3>{0.f, 1.f, 0.f} });

  const std::string url = "@debug:Cloud1"; // no mode hint; should infer 3D by payload type
  bool ok = ioSave(url, ps);

  // Add a simple test polyline overlay to Image1 so we can verify overlay rendering
  {
    using Poly2f = Ravl2::PolyLine<float,2>;
    const int H = imgGray.range()[0].size();
    const int W = imgGray.range()[1].size();
    Poly2f poly({ {10.f, 10.f}, {static_cast<float>(W-10), 10.f}, {static_cast<float>(W-10), static_cast<float>(H-10)} });
    const std::string overlay1 = "@debug:Image1:Mode=Append:Color=#00ff00ff:Width=2"; // green 2px line
    if (!ioSave(overlay1, poly)) {
      SPDLOG_WARN("ioSave('{}', polyline) did not find a writer.", overlay1);
    } else {
      SPDLOG_INFO("Queued test polyline overlay to {} ({} points)", overlay1, poly.size());
    }
  }

  // Keep the process alive briefly so the SDL window (from the debug display thread) is visible.
  SPDLOG_INFO("Sample running. Close the debug window to exit, or wait a moment...");
  std::this_thread::sleep_for(30s);

  return 0;
}
