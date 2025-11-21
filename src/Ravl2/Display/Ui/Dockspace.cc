#include "Ravl2/Display/Ui/Dockspace.hh"

#if defined(RAVL2_WITH_IMGUI)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
#if defined(__clang__)
#pragma GCC diagnostic ignored "-Wgnu-zero-variadic-macro-arguments"
#endif
#include <imgui.h>
#pragma GCC diagnostic pop
#endif

namespace Ravl2::DebugDisplay::Ui
{

  void buildDockspace()
  {
#if defined(RAVL2_WITH_IMGUI)
    ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport());
#endif
  }

}// namespace Ravl2::DebugDisplay::Ui
