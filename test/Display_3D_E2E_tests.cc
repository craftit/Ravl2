#include "Ravl2/Catch2checks.hh"

#include <string>
#include <thread>
#include <chrono>

#include "Ravl2/Geometry/PointSet.hh"
#include "Ravl2/IO/Save.hh"

// Display stack public API
#include "Ravl2/Display/DebugDisplay.hh"

using namespace std::chrono_literals;

TEST_CASE("Display3D defaults to 3D when saving PointSet<float,3>") {
  using Ravl2::PointSet;
  using Ravl2::Point;

  // Ensure display stack can start; on some platforms this is required on main thread
  // Enable headless mode so tests run without opening a window or initializing graphics backends
  Ravl2::DebugDisplay::setHeadless(true);
  Ravl2::DebugDisplay::initDisplay();

  // Construct a tiny 3-point set (triangle in XY plane)
  PointSet<float,3> ps({ Point<float,3>{0.f, 0.f, 0.f},
                         Point<float,3>{1.f, 0.f, 0.f},
                         Point<float,3>{0.f, 1.f, 0.f} });

  const std::string url = "@debug:Cloud1"; // no mode hint; should infer 3D by payload type
  bool ok = ioSave(url, ps);

  REQUIRE(ok); // Writer (debug display sink) should accept PointSet<float,3>

  // Give the display thread a moment to apply the command (non-deterministic UI thread)
  std::this_thread::sleep_for(50ms);
}
