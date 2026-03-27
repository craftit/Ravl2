#pragma once

#include <atomic>
#include <functional>
#include <string>

#include "Ravl2/Display/Channel.hh"
#include "Ravl2/Display/IRenderCommand.hh"

namespace Ravl2::DebugDisplay::Ui
{

  //! Build the Controls panel UI.
  //! @param fbw Framebuffer width in pixels.
  //! @param fbh Framebuffer height in pixels.
  //! @param channels Channel registry to read/write view/normalization state.
  //! @param enqueueCmd Callback to enqueue commands (e.g., normalization changes).
  //! @param invalidated Flag to set when UI changes require a redraw.
  //! @param zoomMin Minimum zoom value.
  //! @param zoomMax Maximum zoom value.
  void buildControlsPanel(float fbw, float fbh,
                          ChannelRegistry &channels,
                          const std::function<void(std::shared_ptr<IRenderCommand>)> &enqueueCmd,
                          std::atomic_bool &invalidated,
                          float zoomMin,
                          float zoomMax);

}// namespace Ravl2::DebugDisplay::Ui
