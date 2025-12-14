#pragma once

#include <string>

namespace Ravl2::DebugDisplay
{
  struct ChannelRegistry;  // Forward declaration
}

namespace Ravl2::DebugDisplay::Ui::Plots
{

  //! Build a dockable panel for plots.
  //!
  //! Renders all channels that have plotState defined. Each channel with plot data
  //! gets its own sub-plot within the Plots window. Supports multiple series per channel,
  //! auto-fit axes, and interactive pan/zoom via ImPlot.
  //!
  //! @param channels Channel registry to query for plot data
  void buildPlotsPanel(ChannelRegistry &channels);

}// namespace Ravl2::DebugDisplay::Ui::Plots
