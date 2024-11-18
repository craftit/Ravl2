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
#include "Ravl2/CallbackArray.hh"

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

    //! Switch to GL context.
    //! The same as 'makeCurrent'.
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

    //! Add a function to be called on each frame render
    //! This can't be called from within the rendering thread
    void addFrameRender(std::function<void()> &&f);

    //! Put a function on the queue to be executed in the main thread
    void put(std::function<void()> &&f) override;

    //! Handle key events
    void keyCallback(int key, int scancode, int action, int mods);
    
    //! Handle cursor position events
    void cursorPositionCallback(double xpos, double ypos);
  
    //! Handle mouse button events
    void mouseButtonCallback(int button, int action, int mods);

    //! Get GLSL version
    [[nodiscard]] const char* glslVersion() const { return mGlsl_version; }
  private:
    GLFWwindow* mWindow = nullptr;
    ThreadedQueue<std::function<void()>> mQueue;
    std::mutex mMutex;
    std::vector<std::function<void()>> mFrameRender;
    const char* mGlsl_version = "#version 130";
  };


}

