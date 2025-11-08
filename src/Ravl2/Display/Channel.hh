#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include <mutex>

#include "Ravl2/Display/ISceneNode.hh"

namespace Ravl2::DebugDisplay {

//! Per-channel state.
//! Holds per-view settings (zoom/pan/camera) and top-level scene nodes.
struct ChannelState {
  std::string name;
  // 2D view params (default values)
  float zoom = 1.0f;
  float panX = 0.0f;
  float panY = 0.0f;

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

private:
  std::unordered_map<std::string, ChannelState> m_channels;
  std::mutex m_mutex;
};

} // namespace Ravl2::DebugDisplay
