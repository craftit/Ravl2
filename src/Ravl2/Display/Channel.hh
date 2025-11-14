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

namespace Ravl2::DebugDisplay {

//! Exclusive view mode per channel.
enum class ViewMode {
  View2D,
  View3D
};

//! Per-channel state.
//! Holds per-view settings and top-level scene node.
struct ChannelState {
  std::string name;

  // Exclusive view selection for this channel
  ViewMode viewMode = ViewMode::View2D;

  // Per-channel UI/runtime flags
  struct Flags {
    bool wantsFocus3D = false;   //!< Request focus on 3D window next frame
    bool showViewToggle = false; //!< Developer toggle to expose 2D/3D radios (hidden by default)
  } flags;

  // 2D view transform: scale (per-axis) and translate in pixels.
  // This represents the view (pan/zoom) state, not owned by scene nodes.
  ScaleTranslate<float, 2> view2D = ScaleTranslate<float, 2>::identity();

  // Display normalization settings for float images (per-view)
  NormalizationSettings norm = {};

  // Single scene content node - either 2D or 3D based on viewMode
  // For 2D: typically a CompositeNode containing Image2DNode + Overlay2DNodes
  // For 3D: typically a Viewport3DNode
  std::unique_ptr<ISceneNode> sceneContent;
};

//! Registry of channels with basic thread-safe access helpers.
struct ChannelRegistry {
  ChannelState &getOrCreateChannel(const std::string &name) {
    std::scoped_lock lk(m_mutex);
    auto it = m_channels.find(name);
    if (it == m_channels.end()) {
      ChannelState st;
      st.name = name;
      auto [insIt, ok] = m_channels.emplace(name, std::move(st));
      (void)ok;
      return insIt->second;
    }
    return it->second;
  }

  void clearChannel(const std::string &name) {
    std::scoped_lock lk(m_mutex);
    auto it = m_channels.find(name);
    if (it != m_channels.end()) {
      // Reset to defaults while preserving the entry
      ChannelState st; st.name = name; // ensure all fields are value-initialized
      it->second = std::move(st);
    }
  }

  // Enumerate channels (invokes callback while holding registry lock).
  template <typename Fn>
  void forEachChannel(Fn &&fn) {
    std::scoped_lock lk(m_mutex);
    for (auto &kv : m_channels) fn(kv.second);
  }

private:
  std::unordered_map<std::string, ChannelState> m_channels;
  std::mutex m_mutex;
};

} // namespace Ravl2::DebugDisplay
