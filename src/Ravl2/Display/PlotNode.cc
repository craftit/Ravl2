#include "Ravl2/Display/PlotNode.hh"
#include "Ravl2/Display/RenderContext.hh"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
#if defined(__clang__)
#pragma GCC diagnostic ignored "-Wgnu-zero-variadic-macro-arguments"
#endif
#include <imgui.h>
#include <implot.h>
#pragma GCC diagnostic pop

#include <spdlog/spdlog.h>

namespace Ravl2::DebugDisplay
{

  void PlotNode::render([[maybe_unused]] RenderContext &ctx)
  {
    // Note: This is a placeholder implementation. The actual rendering
    // is done in Ui/ChannelWindows.cc which accesses the channel's
    // plotState directly and renders plots inline in channel windows.
  }

}// namespace Ravl2::DebugDisplay
