#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include <mutex>
#include <functional>

#include <SDL2/SDL.h>

#include "Ravl2/Display/ISceneNode.hh"
#include "Ravl2/Geometry/ScaleTranslate.hh"
#include "Ravl2/Display/Normalization.hh"

namespace Ravl2::DebugDisplay
{

  //! Exclusive view mode per channel.
  //!
  //! Determines the rendering mode for the channel. Each channel can display either
  //! 2D content (images with overlays) or 3D content (meshes, point clouds) but not both
  //! simultaneously. The view mode affects which scene nodes are active and how input
  //! is processed.
  enum class ViewMode
  {
    View2D,  //!< 2D image display with optional overlays (polylines, points)
    View3D   //!< 3D scene rendering with camera controls and depth testing
  };

  //! Single series data for plotting.
  //!
  //! Represents one line/scatter series within a plot. X and Y vectors must have
  //! matching sizes. The series can be updated via Append, Replace, or RingBuffer modes.
  struct SeriesData {
    std::vector<float> x;      //!< X-axis data points
    std::vector<float> y;      //!< Y-axis data points
    std::string label;         //!< Series name for legend
    uint32_t color = 0;        //!< RGBA color (0 = use ImPlot default palette)
    float lineWidth = 1.0f;    //!< Line thickness (0 = use ImPlot default)
    bool showMarkers = false;  //!< Display point markers on line
  };

  //! Plot state for time series and 1D data visualisation.
  //!
  //! Manages multiple series within a single plot. Each series is identified by name
  //! and can be independently updated. The plot supports auto-fit axes, custom labels,
  //! and bounded history via ring buffer mode.
  //!
  //! @see AddSeriesData command for updating series data
  //! @see PlotNode for rendering implementation
  struct PlotState {
    std::unordered_map<std::string, SeriesData> series;  //!< Series data by name
    std::string xAxisLabel = "X";                        //!< X-axis label text
    std::string yAxisLabel = "Y";                        //!< Y-axis label text
    bool autoFitAxes = true;                             //!< Auto-scale axes to fit data
    size_t maxHistoryPoints = 10000;                     //!< Ring buffer size limit (0 = unlimited)

    // Optional explicit axis limits (when autoFitAxes = false)
    double xMin = 0.0, xMax = 1.0;
    double yMin = -1.0, yMax = 1.0;
  };

  //! Per-channel state.
  //! 
  //! ChannelState maintains all the state and content for a single debug display channel.
  //! Each channel represents a separate view that can be docked in the UI. The state includes
  //! rendering settings, view transforms, normalization parameters, and the scene content.
  //! 
  //! @see DebugDisplay for the overall display system
  //! @see ISceneNode for the scene graph interface
  struct ChannelState {
    std::string name;  //!< Unique channel name/identifier

    // Exclusive view selection for this channel
    ViewMode viewMode = ViewMode::View2D;

    // Per-channel UI/runtime flags
    struct Flags {
      bool wantsFocus3D = false;  //!< Request focus on 3D window next frame (for camera controls)
      bool showViewToggle = false;//!< Developer toggle to expose 2D/3D radios (hidden by default)
    } flags;

    // 2D view transform: scale (per-axis) and translate in pixels.
    // This represents the view (pan/zoom) state, not owned by scene nodes.
    ScaleTranslate<float, 2> view2D = ScaleTranslate<float, 2>::identity();

    // Display normalization settings for float images (per-view)
    NormalizationSettings norm = {};

    // Plot state for time series data (optional, created when plot data is added)
    std::optional<PlotState> plotState;

    // Single scene content node - either 2D or 3D based on viewMode
    // For 2D: typically a CompositeNode containing Image2DNode + Overlay2DNodes
    // For 3D: typically a Viewport3DNode
    std::unique_ptr<ISceneNode> sceneContent;
  };

  //! Registry of channels with basic thread-safe access helpers.
  //! 
  //! ChannelRegistry provides thread-safe access to the collection of debug display channels.
  //! It supports creating, accessing, and enumerating channels with proper synchronization.
  //! All methods are thread-safe and can be called from any thread.
  struct ChannelRegistry {
    //! Get or create a channel by name.
    //! 
    //! If the channel doesn't exist, it will be created with default settings.
    //! If it exists, the existing channel state is returned.
    //! 
    //! @param name Channel name/identifier
    //! @return Reference to the channel state (existing or newly created)
    //! @threadsafe Yes
    ChannelState &getOrCreateChannel(const std::string &name)
    {
      std::scoped_lock lk(m_mutex);
      auto it = m_channels.find(name);
      if(it == m_channels.end()) {
        ChannelState st;
        st.name = name;
        auto [insIt, ok] = m_channels.emplace(name, std::move(st));
        (void)ok;
        return insIt->second;
      }
      return it->second;
    }

    //! Clear a channel's state, resetting it to defaults.
    //! 
    //! This removes all scene content and resets view parameters while preserving
    //! the channel entry. Useful for implementing ':Clear' control messages.
    //! 
    //! @param name Channel name to clear
    //! @threadsafe Yes
    void clearChannel(const std::string &name)
    {
      std::scoped_lock lk(m_mutex);
      auto it = m_channels.find(name);
      if(it != m_channels.end()) {
        // Reset to defaults while preserving the entry
        ChannelState st;
        st.name = name;// ensure all fields are value-initialized
        it->second = std::move(st);
      }
    }

    //! Enumerate all channels by invoking a callback for each.
    //! 
    //! The callback is invoked while holding the registry lock, ensuring thread-safe
    //! access to channel data. The callback should be fast to avoid blocking other threads.
    //! 
    //! @tparam Fn Callback type with signature void(ChannelState&)
    //! @param fn Callback function to invoke for each channel
    //! @threadsafe Yes
    template <typename Fn>
    void forEachChannel(Fn &&fn)
    {
      std::scoped_lock lk(m_mutex);
      for(auto &kv : m_channels) fn(kv.second);
    }

  private:
    std::unordered_map<std::string, ChannelState> m_channels;
    std::mutex m_mutex;
  };

}// namespace Ravl2::DebugDisplay
