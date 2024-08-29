#ifndef RAVLGUI_GLCONTEXT_HEADER
#define RAVLGUI_GLCONTEXT_HEADER 1

#include "Ravl2/RCHandleV.hh"
#include <vector>

extern "C" {
  typedef struct _GtkWidget GtkWidget;
}


namespace Ravl2 {
  
  class GLContextC;
  
  //: OpenGL Context
  
  class GLContextBodyC 
    : public RavlN::RCBodyVC
  {
  public:
    GLContextBodyC(GtkWidget *widget);
    //: Constructor.
    
    ~GLContextBodyC();
    //: Destructor.
    
    
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
  
  
  //: OpenGL Context
  
  class GLContextC 
    : public RavlN::RCHandleVC<GLContextBodyC>
  {
  public:
    GLContextC()
    {}
    //: Default constructor
    // Creates an invalid handle.
    
    GLContextC(GtkWidget *widget)
      : RavlN::RCHandleVC<GLContextBodyC>(new GLContextBodyC(widget))
    {}
    //: Construct from a widget handle
    
    bool IsReady() const
    { return Body().IsReady(); }
    //: Do we have a valid context that is ready to use.
    
    bool Begin()
    { return Body().Begin(); }
    //: Switch to GL context.
    
    void End()
    { Body().End(); }
    //: Flag that we're finished with the context.
    
    bool FreeTextures(const RavlN::std::vector<unsigned int> &textureIds)
    { return Body().FreeTextures(textureIds); }
    //: Thread safe freeing of textures.
    
  protected:
    GLContextC(GLContextBodyC &body)
      : RavlN::RCHandleVC<GLContextBodyC>(body)
    {}
    //: Body constructor
    
    bool GUIFreeTextures(RavlN::std::vector<unsigned int> textures)
    { return Body().GUIFreeTextures(textures); }
    //: Thread 
    
    friend class GLContextBodyC;
  };
  
}

#endif
