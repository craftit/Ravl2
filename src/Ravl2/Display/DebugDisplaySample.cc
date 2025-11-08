#include <iostream>
#include <string>
#include <thread>
#include <chrono>

#include <spdlog/spdlog.h>

#include "Ravl2/Array.hh"
#include "Ravl2/Pixel/Pixel.hh"
#include "Ravl2/IO/Load.hh"
#include "Ravl2/IO/Save.hh"
#include "Ravl2/Display/DebugDisplay.hh"

using namespace std::chrono_literals;

namespace {
  // Try a few common locations for a default image (lena.jpg)
  std::string findDefaultImage() {
    // Prefer repo data path candidates relative to common run dirs
    const char* candidates[] = {
      "data/lena.jpg",
      "../data/lena.jpg",
      "../../data/lena.jpg",
      "../../../data/lena.jpg"
    };
    for (auto c : candidates) {
      if (FILE* f = std::fopen(c, "rb")) { std::fclose(f); return c; }
    }
    return {};
  }
}

int main(int argc, char** argv) {
  try {
    spdlog::set_pattern("[%Y-%m-%d %T.%e] [%^%l%$] [%t] %v");
    spdlog::set_level(spdlog::level::info);

    std::string imagePath;
    if (argc > 1) {
      imagePath = argv[1];
    } else {
      imagePath = findDefaultImage();
      if (imagePath.empty()) {
        spdlog::warn("No image path provided and default lena.jpg not found. A synthetic image will be generated.");
      } else {
        spdlog::info("Using default image: {}", imagePath);
      }
    }

    using namespace Ravl2;
    using Ravl2::PixelRGB8;

    Array<PixelRGB8, 2> img;

    bool loaded = false;
    if (!imagePath.empty()) {
      loaded = ioLoad(img, imagePath);
      if (!loaded) {
        spdlog::error("Failed to load image from '{}'. Will generate a synthetic test image instead.", imagePath);
      }
    }

    if (!loaded) {
      // Generate a small RGB gradient test image (256x256)
      const int W = 256, H = 256;
      img = Array<PixelRGB8, 2>({H, W});
      for (int y = 0; y < H; ++y) {
        for (int x = 0; x < W; ++x) {
          uint8_t r = static_cast<uint8_t>(x);
          uint8_t g = static_cast<uint8_t>(y);
          uint8_t b = static_cast<uint8_t>((x + y) / 2);
          img[{y, x}] = PixelRGB8{r, g, b};
        }
      }
      spdlog::info("Generated synthetic {}x{} RGB8 image.", W, H);
    }

    // Start the new debug display subsystem and attempt to display via @debug scheme
    Ravl2::DebugDisplay::ensureStarted({});

    // Save to the debug display channel. Note: Rendering adapters are coming in later phases; this
    // call exercises the interface and command queue and may be a no-op until adapters are implemented.
    const std::string channel = "@debug:Image:Clear";
    if (!ioSave(channel, img)) {
      spdlog::warn("ioSave('{}', img) did not find a writer (expected before adapters land).", channel);
    } else {
      spdlog::info("Queued image to {}", channel);
    }

    // Keep the process alive briefly so the SDL window (from the debug display thread) is visible.
    spdlog::info("Sample running. Close the debug window to exit, or wait a moment...");
    std::this_thread::sleep_for(2s);

    return 0;
  } catch (const std::exception& e) {
    spdlog::critical("Unhandled exception: {}", e.what());
    return 1;
  }
}
