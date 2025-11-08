#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include <mutex>
#include <functional>

#include "Ravl2/Display/ISceneNode.hh"
#include "Ravl2/Geometry/ScaleTranslate.hh"

namespace Ravl2::DebugDisplay {

//! Per-channel state.
//! Holds per-view settings and top-level scene nodes.
struct ChannelState {
  std::string name;

  // 2D view transform: scale (per-axis) and translate in pixels.
  // This represents the view (pan/zoom) state, not owned by scene nodes.
  ScaleTranslate<float, 2> view2D{};

  // Base image node for 2D view (optional)
  std::unique_ptr<ISceneNode> baseImage2D;
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
