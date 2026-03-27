#pragma once

#include <memory>
#include <string>
#include <vector>
#include <optional>

#include "Ravl2/Types.hh"
#include "Ravl2/Display/IRenderCommand.hh"

namespace Ravl2::DebugDisplay
{

  //! Command to set/replace a 3D point cloud for a channel.
  //! @warning Experimental: stores CPU-side data only; no GPU rendering yet (Phase 6b-6f).
  struct SetPointCloud3D : public IRenderCommand {
    std::string channel;                            //!< Target channel name (filled by sink)
    std::vector<Eigen::Vector3f> positions;         //!< Points in world space
    std::optional<std::vector<uint32_t>> colorsAbgr;//!< Optional per-vertex color (ABGR packed)

    SetPointCloud3D() = default;
    explicit SetPointCloud3D(std::string ch) : channel(std::move(ch)) {}

    //! Apply to channel registry: ensure 3D viewport exists and record point count.
    void apply(ChannelRegistry &channels) override;
  };

}// namespace Ravl2::DebugDisplay
