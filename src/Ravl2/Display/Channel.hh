#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include <mutex>

namespace Ravl2::DebugDisplay {

//! Per-channel state placeholder.
//! This will expand to hold per-view settings (zoom/pan/camera) and resources.
struct ChannelState {
  std::string name;
  // 2D view params (to be expanded)
  float zoom = 1.0f;
  float panX = 0.0f;
  float panY = 0.0f;
  // TODO: scene nodes will be stored here in a future step.
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
      it->second = ChannelState{.name = name};
    }
  }

private:
  std::unordered_map<std::string, ChannelState> m_channels;
  std::mutex m_mutex;
};

} // namespace Ravl2::DebugDisplay
