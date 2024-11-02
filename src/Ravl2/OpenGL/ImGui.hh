//
// Created by charles galambos on 22/10/2024.
//

#pragma once

#include "imgui.h"
#include "Ravl2/OpenGL/GLWindow.hh"

namespace Ravl2
{

  class ImGUI
  {
  public:
    ImGUI() = default;

    explicit ImGUI(std::shared_ptr<Ravl2::GLWindow> window);
    ~ImGUI();

    // Copy constructor deleted
    ImGUI &operator= (const ImGUI&) = delete;
    ImGUI(const ImGUI&) = delete;

    //! Start a new frame
    void newFrame();

    //! Render the frame
    void render();

    //! Shutdown ImGUI, called by the destructor
    void shutdown();

  private:
    //bool m_manageWindow = false;
    std::shared_ptr<Ravl2::GLWindow> mWindow;
    ImVec4 m_clearColor = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
  };

} // Ravl2
