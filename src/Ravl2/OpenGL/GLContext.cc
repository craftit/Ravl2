
#include "Ravl2/OpenGL/GLContext.hh"

#define DODEBUG 0
#if DODEBUG
#define ONDEBUG(x) x
#else
#define ONDEBUG(x)
#endif

namespace Ravl2
{

  //: Do we have a valid context.
  
  bool GLContext::IsReady() const {
    return true;
  }

  //! Swap the front and back buffers
  void GLContext::swapBuffers()
  {
  }


  //: Switch to GL context.
  
  bool GLContext::Begin()
  {
    return true;
  }
  
  //: Flag that we're finished with the context.
  
  void GLContext::End()
  {
    return ;
  }

  //! Put a function on the queue to be executed in the main thread
  void GLContext::put(std::function<void()> &&f)
  {
    (void)f;
    assert(false);
  }

  //: Thread safe freeing of textures.

  bool GLContext::FreeTextures(const std::vector<GLuint> &textureIds)
  {
    put([textureIds]() {
      glDeleteTextures(GLsizei(textureIds.size()), textureIds.data());
    });
    return true;
  }




}
