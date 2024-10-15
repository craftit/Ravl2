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
    
    void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
    {
      GLWindow *theWindow = reinterpret_cast<GLWindow *>(glfwGetWindowUserPointer(window));
      if (theWindow != nullptr) {
        theWindow->keyCallback(key, scancode, action, mods);
      }
    }
    
    void cursor_position_callback(GLFWwindow* window, double xpos, double ypos)
    {
      GLWindow *theWindow = reinterpret_cast<GLWindow *>(glfwGetWindowUserPointer(window));
      if (theWindow != nullptr) {
        theWindow->cursorPositionCallback(xpos, ypos);
      }
    }
    
    void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
    {
      GLWindow *theWindow = reinterpret_cast<GLWindow *>(glfwGetWindowUserPointer(window));
      if (theWindow != nullptr) {
            theWindow->mouseButtonCallback(button, action, mods);
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
      glfwSetWindowUserPointer(mWindow, this);
      
      glfwSetCursorPosCallback(mWindow, cursor_position_callback);
      glfwSetMouseButtonCallback(mWindow, mouse_button_callback);
      glfwSetKeyCallback(mWindow, key_callback);
      
      glfwMakeContextCurrent(mWindow);
      glfwSwapInterval(1);// Enable vsync
    }
    
    
  }

  GLWindow::~GLWindow()
  {
    if(mWindow != nullptr) {
      glfwSetWindowUserPointer(mWindow, nullptr);
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
  
  //! Handle key events
  void GLWindow::keyCallback(int key, int scancode, int action, int mods)
  {
    SPDLOG_INFO("Key: {} Scancode: {} Action: {} Mods: {}", key, scancode, action, mods);
    //      if (key == GLFW_KEY_E && action == GLFW_PRESS)
    //        activate_airship();
  }
  
  //! Handle cursor position events
  void GLWindow::cursorPositionCallback(double xpos, double ypos)
  {
    //SPDLOG_INFO("Cursor position: {} {}", xpos, ypos);
  
  }
  
  //! Handle mouse button events
  void GLWindow::mouseButtonCallback(int button, int action, int mods)
  {
    SPDLOG_INFO("Button: {} Action: {} Mods: {}", button, action, mods);
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
