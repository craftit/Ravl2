// This file is part of RAVL, Recognition And Vision Library
// Copyright (C) 2001, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here
/////////////////////////////////////////////////////

#include <string>
#include <GL/gl.h>

#include "Ravl2/OpenGL/Canvas3D.hh"
#include "Ravl2/Array.hh"
//#include "Ravl2/Image/Reflect.hh"
//#include "Ravl2/Image/ByteRGBValue.hh"
//#include "Ravl2/Threads/SemaphoreRC.hh"
//#include "Ravl2/CallMethodRefs.hh"

#include "Ravl2/OpenGL/GLContext.hh"

#define DODEBUG 0
#if DODEBUG
#define ONDEBUG(x) x
#else
#define ONDEBUG(x)
#endif

namespace Ravl2
{
  Canvas3D::Canvas3D(int sizeX,int sizeY)
    : mViewSize({sizeY,sizeX})
  {}

  //! Create a 3D canvas
  Canvas3D::Canvas3D(const std::shared_ptr<GLContext> &context, bool autoConfigure)
   : m_autoConfigure(autoConfigure),
     m_glContext(context)
  {}
  
  //: Call before using any GL commands.
  bool Canvas3D::GUIBeginGL()
  {
    assert(m_glContext);
    return m_glContext->Begin();
  }

  //: Call after finished with GL
  bool Canvas3D::GUIEndGL()
  {
    assert(m_glContext);
    m_glContext->End();
    return true;
  }

  //: swap buffers.
  bool Canvas3D::GUISwapBuffers()
  {
    assert(m_glContext);
    m_glContext->swapBuffers();
    return true;
  }

  //: clear buffers (make sure you called GUIBeginGL before)
  bool Canvas3D::GUIClearBuffers()
  {
    GLenum whichBuffers(GL_COLOR_BUFFER_BIT);
    if(glIsEnabled(GL_DEPTH_TEST) ) {
      whichBuffers |= (GL_DEPTH_BUFFER_BIT);
    }
    glClear(whichBuffers);
    return true;
  }

  //: Process OpenGL requests.
  bool Canvas3D::GUIProcessReq(DObject3D &obj) {
    ONDEBUG(std::cerr << "Canvas3D::GUIProcessReq(), Called. \n");
    if(!GUIBeginGL()) {
      std::cerr << "Canvas3D::GUIProcessReq(), Failed to BeginGL(). \n";
      return false;
    }
    obj.GUIRenderDL(*this);
    GUIEndGL();
    return true;
  }

  //: Put render instruction into pipe.
  bool Canvas3D::put(std::shared_ptr<DObject3D> r) {
    m_glContext->put([this,r]() {
      GUIProcessReq(*r);
      return true;
    });
    return true;
  }

  //: Handle configure event
  bool Canvas3D::CBConfigureEvent(int width, int height) {
    ONDEBUG(std::cerr << "Canvas3D::CBConfigureEvent, Called. ");
    if(!GUIBeginGL())
      return false;
    if(m_autoConfigure) {
#if 0
      ONDEBUG(std::cerr << "Reshape. " << widget->allocation.width << " " << widget->allocation.height << "\n");
      glViewport(0, 0, widget->allocation.width, widget->allocation.height);
#endif
    }
    
    GUIEndGL();
    return true;
  }

  //: Enable or disable lighting
  //: Put End Of Stream marker.
  bool Canvas3D::SetLightingMode(bool bLighting) {
    m_bLighting = bLighting;
    m_glContext->put([this,bLighting]() {
      m_bLighting = bLighting;
      GUIDoLighting();
      return true;
    });
    return true;
  }
  
  //: Write contents of widget to an image.

#if 0
  bool Canvas3D::SaveToImageInternal(Ravl2::Array<PixelRGB8,2> *img,SemaphoreRC &done) {
    if(!GUIBeginGL()) {
      std::cerr << "Failed to begin context.";
      return false;
    }
    GLint x=0,y=0;
    GLsizei width=Size()[1],height=Size()[0];
    std::cerr << "Canvas3D::SaveToImage, Width=" << width << " Height=" << height << "\n";
    width -= width%4;
    GLenum format=GL_RGB, wtype=GL_UNSIGNED_BYTE;
    Ravl2::Array<PixelRGB8,2> out({height,width});
    GLvoid* buf = (GLvoid*) &(out(0,0));
    glReadPixels(x,y,width,height,format,wtype,buf);
    Ravl2::ReflectHorizontal(out,*img);
    GUIEndGL();
    done.Post();
    return true;
  }
#endif

  //: Write contents of screen to an image.
  
  bool Canvas3D::SaveToImage(Ravl2::Array<PixelRGB8,2> &img) {
#if 0
    SemaphoreRC done(0);
    if(!Manager.IsGUIThread()) {
      Manager.Queue(TriggerR(*this,&Canvas3D::SaveToImageInternal,&img,done));
      done.Wait();
      return true;
    }
    return SaveToImageInternal(&img,done);
#else
    (void) img;
    return false;
#endif
  }

} // end of namespace


