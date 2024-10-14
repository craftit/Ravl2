//
// Created by charles on 02/09/24.
//

#pragma once

#ifndef GL_SILENCE_DEPRECATION
#define GL_SILENCE_DEPRECATION 1
#endif

#include <string_view>
#include <GLFW/glfw3.h>
#include "Ravl2/ThreadedQueue.hh"
#include "Ravl2/OpenGL/GLContext.hh"

namespace Ravl2
{

  //! @brief A GLFW window

  class GLWindow
    : public GLContext
  {
  public:
    //! Construct a new window
    GLWindow(int width, int height, std::string_view title);

    //! Destructor
    ~GLWindow() override;

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

    //: Switch to GL context.
    bool Begin() override;

    //! Make the window the current context
    void makeCurrent();

    //! Swap the front and back buffers
    void swapBuffers() override;

    //! Get the GLFW window
    [[nodiscard]] GLFWwindow* getGLFWwindow() const { return mWindow; }

    //! Run the main loop
    //! On some platforms this must be called from the main thread
    void runMainLoop();

    //! Put a function on the queue to be executed in the main thread
    void put(std::function<void()> &&f) override;

  private:
    GLFWwindow* mWindow = nullptr;
    ThreadedQueue<std::function<void()>> mQueue;
  };


}

