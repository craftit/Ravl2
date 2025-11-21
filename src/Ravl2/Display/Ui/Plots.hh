#pragma once

#include <string>

namespace Ravl2::DebugDisplay::Ui::Plots
{

  //! Build a dockable placeholder panel for plots.
  //! This isolates ImPlot integration hooks so Phase 7 can plug in without
  //! touching rendering core or event routing.
  void buildPlotsPanel();

}// namespace Ravl2::DebugDisplay::Ui::Plots
