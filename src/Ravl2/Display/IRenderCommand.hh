#pragma once

#include <memory>
#include <string>

namespace Ravl2::DebugDisplay
{

  struct ChannelRegistry;// fwd decl

  //! Command executed on the GUI thread to mutate channel state / scene graph.
  struct IRenderCommand {
    virtual ~IRenderCommand() = default;
    virtual void apply(ChannelRegistry &channels) = 0;
  };

}// namespace Ravl2::DebugDisplay
