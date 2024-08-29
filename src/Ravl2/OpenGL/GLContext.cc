
#include "Ravl2/OpenGL/GLContext.hh"

//#include "Ravl2/StdError.hh"
#if !RAVL_USE_GTKGLEXT
#include "Ravl2/GUI/gdkgl.h"
#include "Ravl2/GUI/gtkglarea.h"
#endif

#include "Ravl2/GUI/Manager.hh"
#include "Ravl2/FMatrix.hh"

#include <gtk/gtk.h>

#if RAVL_USE_GTKGLEXT
#include <GL/glu.h>
#include <gtk/gtkgl.h>
#include <gdk/gdkgl.h>
#else
#include <GL/glx.h>
#endif

#define DODEBUG 0
#if DODEBUG
#define ONDEBUG(x) x
#else
#define ONDEBUG(x)
#endif

namespace Ravl2 {

  //: Constructor.
  
  GLContextBodyC::GLContextBodyC(GtkWidget *widget) 
    : m_widget(widget)
  {
    if(m_widget != 0)
      g_object_ref(m_widget);
  }
  
  //: Destructor.
  
  GLContextBodyC::~GLContextBodyC() {
    if(m_widget != 0)
      g_object_unref(m_widget);
  }
  
  //: Do we have a valid context.
  
  bool GLContextBodyC::IsReady() const {
    return (m_widget != 0);
  }
  
  //: Switch to GL context.
  
  bool GLContextBodyC::Begin() {
    if(m_widget == 0) {
      ONDEBUG(std::cerr << "GLContextBodyC::Begin(), ERROR: Called with invalid widget. \n");
      return false;
    }
#if RAVL_USE_GTKGLEXT
    GdkGLContext *glcontext = gtk_widget_get_gl_context (m_widget);
    GdkGLDrawable *gldrawable = gtk_widget_get_gl_drawable (m_widget);
    if (!gdk_gl_drawable_gl_begin (gldrawable, glcontext))
      return FALSE;
#else
    if (!gtk_gl_area_make_current(GTK_GL_AREA(m_widget))) {
      ONDEBUG(std::cerr << "WARNING: Canvas3DBodyC::BeginGL(), Failed. \n");
      return false;
    }
#endif
    return true;
  }
  
  //: Flag that we're finished with the context.
  
  void GLContextBodyC::End() {
#if RAVL_USE_GTKGLEXT
    GdkGLDrawable *gldrawable = gtk_widget_get_gl_drawable (m_widget);
    gdk_gl_drawable_gl_end (gldrawable);
#endif
    return ;
  }

  //: Thread safe freeing of textures.
  
  bool GLContextBodyC::FreeTextures(const RavlN::std::vector<GLuint> &textureIds) {
    if(!Manager.IsGUIThread()) {
      Manager.Queue(Trigger(GLContextC(*this),&GLContextC::GUIFreeTextures,textureIds));
    } else {
      return GUIFreeTextures(textureIds); 
    }
    return true;
  }
  
  //: Thread 
  
  bool GLContextBodyC::GUIFreeTextures(RavlN::std::vector<GLuint> textureIds) {
    //std::cerr << "Freeing " << textureIds.size() << " textures\n";
    glDeleteTextures(textureIds.size(), &(textureIds[0]));
    return true;
  }

  
}
