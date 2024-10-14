//
// Created by charles on 02/09/24.
//

#include <spdlog/spdlog.h>
#include "Ravl2/OpenGL/GLWindow.hh"

namespace Ravl2
{
  namespace
  {
    std::atomic<bool> gTerminateMain = false;
    std::atomic<bool> gMainStarted = false;
    std::atomic<bool> gInitDone = false;

    void glfw_error_callback(int error, const char *description)
    {
      SPDLOG_ERROR("GLFW Error {} : {}", error, description);
    }

    void checkGLFWInit()
    {
      bool status = false;
      if(!gInitDone.compare_exchange_strong(status, true))
        return;
      glfwSetErrorCallback(glfw_error_callback);
      if(!glfwInit()) {
        SPDLOG_INFO("Failed to initialize GLFW");
        return;
      }
    }
  }

  GLWindow::GLWindow(int width, int height, std::string_view title)
  {
    checkGLFWInit();

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    mWindow = glfwCreateWindow(width, height, title.data(), nullptr, nullptr);
    if (mWindow != nullptr) {
      glfwMakeContextCurrent(mWindow);
      glfwSwapInterval(1);// Enable vsync
    }

  }

  GLWindow::~GLWindow()
  {
    if(mWindow != nullptr) {
      glfwDestroyWindow(mWindow);
    }
  }

  void GLWindow::makeCurrent()
  {
    if (mWindow == nullptr) {
      return;
    }
    glfwMakeContextCurrent(mWindow);
  }

  //: Switch to GL context.
  bool GLWindow::Begin()
  {
    makeCurrent();
    return true;
  }


  void GLWindow::swapBuffers()
  {
    if (mWindow == nullptr) {
      return;
    }
    glfwSwapBuffers(mWindow);
  }

  //! Put a function on the queue to be executed in the main thread
  void GLWindow::put(std::function<void()> &&f)
  {
    mQueue.push(std::move(f));
    glfwPostEmptyEvent();
  }


  void GLWindow::runMainLoop()
  {
    // Check if this is the first time the main loop is run
    bool status = false;
    if(!gMainStarted.compare_exchange_strong(status, true)) {
      SPDLOG_WARN("Main loop already running");
      return;
    }
    checkGLFWInit();

    while(!gTerminateMain) {
      // Poll and handle events (inputs, window resize, etc.)
      // This can be made to exit early by calling 'glfwPostEmptyEvent()'
      glfwWaitEventsTimeout(0.2);
      //glfwSwapBuffers(window);
      while(!gTerminateMain) {
	std::function<void()> func;
	if(!mQueue.tryPop(func)) {
	  break;
	}
	func();
      }
    }

    glfwTerminate();
  }

}
