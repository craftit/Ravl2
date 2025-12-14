#pragma once

#include <memory>
#include <string>
#include <vector>
#include <algorithm>

#include <spdlog/spdlog.h>

#include "Ravl2/Display/IRenderCommand.hh"
#include "Ravl2/Display/Channel.hh"

namespace Ravl2::DebugDisplay
{

  //! Update mode for series data.
  enum class SeriesUpdateMode
  {
    Replace,    //!< Clear existing data and replace with new data
    Append,     //!< Append new data to existing series
    RingBuffer  //!< Append with bounded history (drop oldest when maxHistoryPoints exceeded)
  };

  //! Command to add or update time series data in a plot channel.
  //!
  //! This command creates or updates a named series within a channel's plot state.
  //! The series is identified by name and can be updated in three modes:
  //! - Replace: Clear and replace all data
  //! - Append: Add new points to the end
  //! - RingBuffer: Append with automatic oldest-point removal when limit exceeded
  //!
  //! X values can be provided explicitly or auto-generated as indices (0, 1, 2, ...).
  //!
  //! @see PlotState for the data structure
  //! @see PlotNode for rendering
  struct AddSeriesData : public IRenderCommand {
    std::string channel;           //!< Target channel name
    std::string seriesName;        //!< Series identifier within the plot
    std::vector<float> x;          //!< X-axis data (empty = auto-generate indices)
    std::vector<float> y;          //!< Y-axis data (required)
    SeriesUpdateMode mode = SeriesUpdateMode::Append;  //!< Update mode

    // Optional styling (0/empty = use defaults)
    uint32_t color = 0;            //!< RGBA color (0 = use ImPlot palette)
    float lineWidth = 0.0f;        //!< Line thickness (0 = use ImPlot default)
    bool showMarkers = false;      //!< Display point markers

    explicit AddSeriesData(std::string ch, std::string series)
      : channel(std::move(ch)), seriesName(std::move(series))
    {}

    void apply(ChannelRegistry &channels) override
    {
      // Warn if channel name is empty
      if(channel.empty()) {
        static bool warnedOnce = false;
        if(!warnedOnce) {
          SPDLOG_WARN("DebugDisplay: AddSeriesData called with empty channel name. Check URL format: display://channelname:series=...");
          warnedOnce = true;
        }
      }

      auto &ch = channels.getOrCreateChannel(channel);

      // Ensure plotState exists
      if(!ch.plotState.has_value()) {
        ch.plotState = PlotState{};
        SPDLOG_DEBUG("DebugDisplay: Created plot state for channel '{}'", channel.empty() ? "(empty)" : channel);
      }

      auto &plotState = ch.plotState.value();

      // Get or create series
      auto it = plotState.series.find(seriesName);
      if(it == plotState.series.end()) {
        // Create new series
        SeriesData newSeries;
        newSeries.label = seriesName;
        newSeries.color = color;
        newSeries.lineWidth = (lineWidth > 0.0f) ? lineWidth : 1.0f;
        newSeries.showMarkers = showMarkers;

        auto [insertIt, ok] = plotState.series.emplace(seriesName, std::move(newSeries));
        (void)ok;
        it = insertIt;
        SPDLOG_DEBUG("DebugDisplay: Created new series '{}' in channel '{}'", seriesName, channel);
      }

      SeriesData &series = it->second;

      // Generate X values if not provided
      std::vector<float> xData = x;
      if(xData.empty() && !y.empty()) {
        xData.reserve(y.size());
        float startIdx = 0.0f;
        if(mode == SeriesUpdateMode::Append && !series.x.empty()) {
          // Continue from last X value
          startIdx = series.x.back() + 1.0f;
        }
        for(size_t i = 0; i < y.size(); ++i) {
          xData.push_back(startIdx + static_cast<float>(i));
        }
      }

      // Validate matching sizes
      if(xData.size() != y.size()) {
        SPDLOG_WARN("DebugDisplay: X and Y data size mismatch for series '{}' ({} vs {})",
                    seriesName, xData.size(), y.size());
        return;
      }

      // Apply update based on mode
      switch(mode) {
        case SeriesUpdateMode::Replace:
          series.x = std::move(xData);
          series.y = std::move(y);
          SPDLOG_DEBUG("DebugDisplay: Replaced series '{}' with {} points", seriesName, series.x.size());
          break;

        case SeriesUpdateMode::Append:
          series.x.insert(series.x.end(), xData.begin(), xData.end());
          series.y.insert(series.y.end(), y.begin(), y.end());
          SPDLOG_DEBUG("DebugDisplay: Appended {} points to series '{}' (total: {})",
                       y.size(), seriesName, series.x.size());
          break;

        case SeriesUpdateMode::RingBuffer: {
          // Append new data
          series.x.insert(series.x.end(), xData.begin(), xData.end());
          series.y.insert(series.y.end(), y.begin(), y.end());

          // Trim if exceeds limit
          size_t maxPoints = plotState.maxHistoryPoints;
          if(maxPoints > 0 && series.x.size() > maxPoints) {
            size_t excess = series.x.size() - maxPoints;
            series.x.erase(series.x.begin(), series.x.begin() + static_cast<long>(excess));
            series.y.erase(series.y.begin(), series.y.begin() + static_cast<long>(excess));
            SPDLOG_DEBUG("DebugDisplay: Ring buffer trimmed {} points from series '{}' (limit: {})",
                         excess, seriesName, maxPoints);
          }
          SPDLOG_DEBUG("DebugDisplay: Ring buffer appended {} points to series '{}' (total: {}, limit: {})",
                       y.size(), seriesName, series.x.size(), maxPoints);
          break;
        }
      }

      // Update styling if provided
      if(color != 0) {
        series.color = color;
      }
      if(lineWidth > 0.0f) {
        series.lineWidth = lineWidth;
      }
      series.showMarkers = showMarkers;
    }
  };

}// namespace Ravl2::DebugDisplay
