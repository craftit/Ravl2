#include "Ravl2/Display/PlotNode.hh"
#include "Ravl2/Display/RenderContext.hh"

#if defined(RAVL2_WITH_IMGUI)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
#if defined(__clang__)
#pragma GCC diagnostic ignored "-Wgnu-zero-variadic-macro-arguments"
#endif
#include <imgui.h>
#include <implot.h>
#pragma GCC diagnostic pop
#endif

#include <spdlog/spdlog.h>

namespace Ravl2::DebugDisplay
{

  void PlotNode::render([[maybe_unused]] RenderContext &ctx)
  {
#if defined(RAVL2_WITH_IMGUI)
    // Note: This is a placeholder implementation. The actual rendering
    // is done in Ui/Plots.cc buildPlotsPanel() which accesses the
    // channel's plotState directly. This render() method would be used
    // if we wanted plots embedded within channel windows rather than
    // in a separate plots panel.

    // For Phase 7c, we'll update buildPlotsPanel() to iterate over
    // channels with plotState and render them there.
#endif
  }

}// namespace Ravl2::DebugDisplay
