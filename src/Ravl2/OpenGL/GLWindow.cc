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

#if 1
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
#endif
  }
  

  GLWindow::GLWindow(int width, int height, std::string_view title)
  {
    checkGLFWInit();

#if defined(IMGUI_IMPL_OPENGL_ES2)
    // GL ES 2.0 + GLSL 100
    mGlsl_version = "#version 100";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
#elif defined(__APPLE__)
    // GL 3.2 + GLSL 150
    mGlsl_version = "#version 150";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    // glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
    //glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // Required on Mac
#else
    // GL 3.0 + GLSL 130
    mGlsl_version = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    //glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
    //glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // 3.0+ only
#endif

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
      SPDLOG_WARN("Window is null");
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
    glfwMakeContextCurrent(mWindow);
    glfwSwapBuffers(mWindow);
  }
  
  //! Handle key events
  void GLWindow::keyCallback(int key, int scancode, int action, int mods)
  {
    SPDLOG_INFO("Key: {} Scancode: {} Action: {} Mods: {}", key, scancode, action, mods);
    //      if (key == GLFW_KEY_E && action == GLFW_PRESS)
    //        activate_airship();
    int changed = mKeyModState ^ mods;
    mKeyModState = mods;
    if (changed & GLFW_MOD_SHIFT) {
      SPDLOG_INFO("Shift key changed");
    }
    if (changed & GLFW_MOD_ALT) {
      SPDLOG_INFO("Alt key changed");
    }
    if (changed & GLFW_MOD_CONTROL) {
      SPDLOG_INFO("Ctrl key changed");
    }
  }
  
  //! Handle cursor position events
  void GLWindow::cursorPositionCallback(double xpos, double ypos)
  {
    (void)xpos;
    (void)ypos;
    //SPDLOG_INFO("Cursor position: {} {}", xpos, ypos);
    mCursorPositionCB.call(xpos, ypos);
    mMouseLastX = float(xpos);
    mMouseLastY = float(ypos);
    MouseEvent event(MouseEventTypeT::MouseMove, mMouseLastX, mMouseLastY, mMouseButtonState,0, mKeyModState);
    mMouseEventCB.call(event);
  }
  
  //! Handle mouse button events
  void GLWindow::mouseButtonCallback(int button, int action, int mods)
  {
    SPDLOG_INFO("Button: {} Action: {} Mods: {}", button, action, mods);

    mMouseButtonCB.call(button, action, mods);

    int const changedMask = 1 << button;
    int const newState = (action == GLFW_PRESS) ? (mMouseButtonState | changedMask) : (mMouseButtonState & ~(changedMask));
    mMouseButtonState = newState;
    MouseEvent event((action == GLFW_PRESS) ? MouseEventTypeT::MousePress : MouseEventTypeT::MouseRelease, mMouseLastX, mMouseLastY, newState, changedMask ,mKeyModState);
    mMouseEventCB.call(event);
  }
  

  //! Put a function on the queue to be executed in the main thread
  void GLWindow::put(std::function<void()> &&func)
  {
    mQueue.push(std::move(func));
    glfwPostEmptyEvent();
  }

  //! Add a function to be called on each frame render
  CallbackHandle GLWindow::addFrameRender(std::function<void()> &&f)
  {
    return mFrameRender.add(std::move(f));
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

    while(!gTerminateMain && glfwWindowShouldClose(mWindow) == 0) {
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
      {
        mFrameRender.call();
      }
    }

    glfwTerminate();
  }

}
