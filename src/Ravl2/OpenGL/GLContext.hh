
#pragma once

#ifndef GL_SILENCE_DEPRECATION
#define GL_SILENCE_DEPRECATION 1
#endif

#include <vector>
#include <GLFW/glfw3.h>
#include "Ravl2/Types.hh"
#include "Ravl2/CallbackArray.hh"

namespace Ravl2
{

  //: OpenGL Context
  
  class GLContext
  {
  public:
    //: Constructor.
    GLContext() = default;

    //: Destructor.
    virtual ~GLContext() = default;

    //! Delete the copy constructor
    GLContext(const GLContext&) = delete;
    GLContext& operator=(const GLContext&) = delete;
    GLContext(GLContext&&) = delete;
    GLContext& operator=(GLContext&&) = delete;


    //: Do we have a valid context.
    [[nodiscard]] virtual bool IsReady() const;

    //! Swap the front and back buffers
    virtual void swapBuffers();

    //: Switch to GL context.
    virtual bool Begin();

    //: Flag that we're finished with the context.
    virtual void End();

    //! Thread safe freeing of textures.
    bool FreeTextures(const std::vector<unsigned int> &textureIds);

    //! Put a function on the queue to be executed in the main thread
    virtual void put(std::function<void()> &&f) = 0;

  };

}

