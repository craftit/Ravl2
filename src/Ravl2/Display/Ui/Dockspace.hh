#pragma once

#include <cstdint>

namespace Ravl2::DebugDisplay::Ui
{

  //! Builds a global ImGui dockspace over the main viewport.
  //! Safe to call each frame; no-op if ImGui is not available.
  void buildDockspace();

}// namespace Ravl2::DebugDisplay::Ui
