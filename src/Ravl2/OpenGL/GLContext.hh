
#include "Ravl2/Types.hh"
#include <vector>

namespace Ravl2
{
  class GLContextC;
  
  //: OpenGL Context
  
  class GLContextBodyC 
  {
  public:
    //: Constructor.
    GLContextBodyC(GtkWidget *widget);

    //: Destructor.
    virtual ~GLContextBodyC();

    virtual bool IsReady() const;
    //: Do we have a valid context.
    
    virtual bool Begin();
    //: Switch to GL context.
    
    virtual void End();
    //: Flag that we're finished with the context.
    
    virtual bool FreeTextures(const RavlN::std::vector<unsigned int> &textureIds);
    //: Thread safe freeing of textures.
    
  protected:
    virtual bool GUIFreeTextures(RavlN::std::vector<unsigned int> textureIds);
    //: Thread 
    
    GtkWidget *m_widget;
    
    friend class GLContextC;
  };

}

