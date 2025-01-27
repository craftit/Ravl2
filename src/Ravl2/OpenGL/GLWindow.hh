//
// Created by charles on 02/09/24.
//

#pragma once

#ifndef GL_SILENCE_DEPRECATION
#define GL_SILENCE_DEPRECATION 1
#include "MouseEvent.hh"
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
    CallbackHandle addFrameRender(std::function<void()> &&f);

    //! Put a function on the queue to be executed in the main thread
    void put(std::function<void()> &&f) override;

    //! Get GLSL version
    [[nodiscard]] const char* glslVersion() const { return mGlsl_version; }

    //! Add a cursor position callback
    CallbackHandle addCursorPositionCallback(std::function<void(double xpos, double ypos)> &&f)
    {
      return mCursorPositionCB.add(std::move(f));
    }

    //! Add a key callback
    CallbackHandle addKeyCallback(std::function<void(int key, int scancode, int action, int mods)> &&f)
    {
      return mKeyCB.add(std::move(f));
    }

    //! Add a mouse button callback
    CallbackHandle addMouseButtonCallback(std::function<void(int button, int action, int mods)> &&f)
    {
      return mMouseButtonCB.add(std::move(f));
    }

    //! Add mouse event callback
    CallbackHandle addMouseEventCallback(std::function<void(const MouseEvent &event)> &&f)
    {
      return mMouseEventCB.add(std::move(f));
    }

    //! Handle key events
    void keyCallback(int key, int scancode, int action, int mods);
    
    //! Handle cursor position events
    void cursorPositionCallback(double xpos, double ypos);
  
    //! Handle mouse button events
    void mouseButtonCallback(int button, int action, int mods);

  private:
    CallbackArray<std::function<void(const MouseEvent &event)>> mMouseEventCB;
    CallbackArray<std::function<void(double xpos, double ypos)>> mCursorPositionCB;
    CallbackArray<std::function<void(int key, int scancode, int action, int mods)>> mKeyCB;
    CallbackArray<std::function<void(int button, int action, int mods)>> mMouseButtonCB;

    int mKeyModState = 0;
    int mMouseButtonState = 0;
    float mMouseLastX = 0;
    float mMouseLastY = 0;
    GLFWwindow* mWindow = nullptr;
    ThreadedQueue<std::function<void()>> mQueue;
    std::mutex mMutex;
    CallbackArray<std::function<void()>> mFrameRender;
    const char* mGlsl_version = "#version 130";
  };


}

