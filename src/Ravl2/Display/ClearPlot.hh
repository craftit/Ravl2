#pragma once

#include <memory>
#include <string>
#include <optional>

#include <spdlog/spdlog.h>

#include "Ravl2/Display/IRenderCommand.hh"
#include "Ravl2/Display/Channel.hh"

namespace Ravl2::DebugDisplay
{

  //! Command to clear plot data from a channel.
  //!
  //! Clears either all series in a channel's plot or a specific named series.
  //! If seriesName is empty, all series are cleared. If seriesName is specified,
  //! only that series is removed.
  //!
  //! @see PlotState for the data structure
  //! @see AddSeriesData for adding series
  struct ClearPlot : public IRenderCommand {
    std::string channel;                     //!< Target channel name
    std::optional<std::string> seriesName;   //!< Series to clear (empty = clear all)

    explicit ClearPlot(std::string ch, std::optional<std::string> series = std::nullopt)
      : channel(std::move(ch)), seriesName(std::move(series))
    {}

    void apply(ChannelRegistry &channels) override
    {
      auto &ch = channels.getOrCreateChannel(channel);

      if(!ch.plotState.has_value()) {
        SPDLOG_DEBUG("DebugDisplay: No plot state to clear for channel '{}'", channel);
        return;
      }

      auto &plotState = ch.plotState.value();

      if(seriesName.has_value()) {
        // Clear specific series
        auto it = plotState.series.find(seriesName.value());
        if(it != plotState.series.end()) {
          plotState.series.erase(it);
          SPDLOG_DEBUG("DebugDisplay: Cleared series '{}' from channel '{}'", seriesName.value(), channel);
        } else {
          SPDLOG_DEBUG("DebugDisplay: Series '{}' not found in channel '{}'", seriesName.value(), channel);
        }
      } else {
        // Clear all series
        plotState.series.clear();
        SPDLOG_DEBUG("DebugDisplay: Cleared all series from channel '{}'", channel);
      }
    }
  };

}// namespace Ravl2::DebugDisplay
