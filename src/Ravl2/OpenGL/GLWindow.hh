//
// Created by charles on 02/09/24.
//

#pragma once

#include <string_view>
#include <GLFW/glfw3.h>
#include "Ravl2/ThreadedQueue.hh"

namespace Ravl2
{

  //! @brief A GLFW window

  class GLWindow
  {
  public:
    //! Construct a new window
    GLWindow(int width, int height, std::string_view title);

    //! Destructor
    ~GLWindow();

    //! Delete the copy constructor
    GLWindow(const GLWindow&) = delete;
    GLWindow& operator=(const GLWindow&) = delete;

    //! Move constructor
    GLWindow(GLWindow&& oth) noexcept
     : mWindow(oth.mWindow)
    {
      oth.mWindow = nullptr;
    }

    //! Move assignment
    GLWindow& operator=(GLWindow&& oth) noexcept
    {
      if (this != &oth) {
        mWindow = oth.mWindow;
        oth.mWindow = nullptr;
      }
      return *this;
    }

    //! Make the window the current context
    void makeCurrent();

    //! Swap the front and back buffers
    void swapBuffers();

    //! Get the GLFW window
    [[nodiscard]] GLFWwindow* getGLFWwindow() const { return mWindow; }

    //! Run the main loop
    //! On some platforms this must be called from the main thread
    static void runMainLoop();

  private:
    GLFWwindow* mWindow = nullptr;
  };


}

