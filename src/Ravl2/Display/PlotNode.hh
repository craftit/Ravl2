#pragma once

#include "Ravl2/Display/ISceneNode.hh"
#include "Ravl2/Display/Channel.hh"

namespace Ravl2::DebugDisplay
{

  //! Scene node for rendering time series plots using ImPlot.
  //!
  //! PlotNode renders all series from a channel's PlotState using ImPlot.
  //! It handles multiple series, auto-fit axes, custom labels, and styling.
  //! The node is lightweight and stateless—all data lives in PlotState.
  //!
  //! @see PlotState for the data structure
  //! @see AddSeriesData for updating series
  struct PlotNode : public ISceneNode {
    std::string channelName;  //!< Channel name (for logging/debugging)

    explicit PlotNode(std::string channel) : channelName(std::move(channel)) {}

    void render(RenderContext &ctx) override;
  };

}// namespace Ravl2::DebugDisplay
