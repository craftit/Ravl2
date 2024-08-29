// This file is part of RAVL, Recognition And Vision Library
// Copyright (C) 2001, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here
#ifndef RAVLGUI_CANVAS3D_HEADER
#define RAVLGUI_CANVAS3D_HEADER 1
/////////////////////////////////////////////////////
//! docentry="Ravl.API.Graphics.3D"
//! example=exCanvas3D.cc
//! author="Charles Galambos"
//! date="12/04/1999"

#include "Ravl2/GUI/Widget.hh"
#include "Ravl2/GUI/DObject3D.hh"
#include "Ravl2/GUI/Util.hh"
#include "Ravl2/GUI/DViewPoint3D.hh"
#include "Ravl2/GUI/DLight3D.hh"
#include "Ravl2/GUI/GLContext.hh"
#include "Ravl2/CallMethods.hh"
#include "Ravl2/CallMethodRefs.hh"


namespace Ravl2 {
  class SemaphoreRC;
}

namespace Ravl2 {
  template<typename DataT> class ImageC;
}

//#include "Ravl2/Threads/EventHandlerRef.hh"

typedef struct _GdkGLContext GdkGLContext;
typedef struct _GdkVisual GdkVisual;

namespace Ravl2 {


  //: 3D Canvas Rendering modes
  enum Canvas3DRenderMode {
    C3D_POINT,
    C3D_WIRE,
    C3D_FLAT,
    C3D_SMOOTH
  };

  //: 3D Canvas body.
  class Canvas3DBodyC 
    : public WidgetBodyC
  {
  public:
    Canvas3DBodyC(int x, int y, int *nglattrlist = 0, bool autoConfigure = true);
    //: Create a 3D canvas

    bool GUIInitGL();
    //: Initialise GL info
    // Returns false if GL is not available.

    virtual bool Create()
    { return Create(NULL); }
    //: Create widget (GUI only)

    virtual bool Create(GtkWidget *Parent);
    //: Create with a widget supplied from elsewhere. (GUI only)

    bool GUIBeginGL();
    //: Call before using any GL commands.
    // This is needed to select correct gl context for the canvas 3d widget
    // Should only be called by GUI thread.

    bool GUIEndGL();
    //: Call aftern finished with GL
    // Should only be called by GUI thread.

    bool GUISwapBuffers();
    //: swap buffers.

    bool GUIClearBuffers();
    //: clears the buffers
    // depth buffer and colour buffer is cleared

    bool GUIProcessReq(DObject3DC &obj);
    //: Process OpenGL requests. (renders obj on the 3d canvas)

    bool Put(const DObject3DC &r);
    //: Put render instruction into pipe.

    bool SetTextureMode(bool& bTexture)
    { m_bTexture = bTexture; return true; }
    //: Enable or disable texturing

    bool SetLightingMode(bool& bLighting);
    //: Enable or disable lighting

    bool SetRenderMode(Canvas3DRenderMode& eRenderMode)
    { m_eRenderMode = eRenderMode; return true; }
    //: Set rendering mode

    bool GetTextureMode(void) const
    { return m_bTexture; }
    //: Is texturing enabled?

    bool GetLightingMode(void) const
    { return m_bLighting; }
    //: Is lighting enabled?

    Canvas3DRenderMode GetRenderMode(void) const
    { return m_eRenderMode; }
    //: Get rendering mode

    bool GUIDoLighting() {
      if (m_bLighting) glEnable(GL_LIGHTING);
      else glDisable(GL_LIGHTING);
      return true;
    }
    //: Setup lighting 
    
    bool HaveExtNonPowerOfTwoTexture() const
    { return m_glExtNonPowerOfTwoTexture; }
    //: Do we have non power of two textures?
    
    const GLContextC &GUIGLContext() { 
      if(!m_glContext.IsValid())
        m_glContext = GLContextC(widget);
      return m_glContext; 
    }
    //: Get opengl context.
    
    bool SaveToImage(Ravl2::Array<ByteRGBValueC,2> &img);
    //: Write contents of widget to an image.
  protected:
    
    bool SaveToImageInternal(Ravl2::Array<ByteRGBValueC,2> *img,SemaphoreRC &done);
    //: Write contents of widget to an image.
    
    virtual bool CBConfigureEvent(GdkEvent *event);
    //: Handle configure event

    int *glattrlist;
    //: Attribute list.
    // see GUI/3D/gdkgl.h for a list of attributes.
    // the attribute list should be terminated with
    // GDK_GL_NONE.

    int sx, sy;
    // Size of view port.
    // Only needed for widget creation

    Canvas3DRenderMode m_eRenderMode; //: Rendering mode

    bool m_bTexture;
    //: Texture mode
    // true = use texture when rendering.

    bool m_bLighting;
    //: Lighting mode
    // true = Use lighting when rendering.

    bool m_autoConfigure;  //: Handle viewport configure events internally.
    
    bool m_glExtNonPowerOfTwoTexture; // Do we have non power of two textures ?
    bool m_initDone;
    
    GLContextC m_glContext;
  private:
    Canvas3DBodyC(const Canvas3DBodyC &)
    {}
    //: Never do this.
  };

  //: 3D Canvas Widget.
  class Canvas3DC : public WidgetC
  {
  public:
    Canvas3DC()
    {}
    //: Default constructor.
    // Creates an invalid handle.

    Canvas3DC(int x, int y, int *nglattrlist = 0, bool autoConfigure = true)
      : WidgetC(*new Canvas3DBodyC(x, y, nglattrlist, autoConfigure))
    {}
    //: Constructor.
    // see GUI/3D/gdkgl.h for a list of attributes for nglattrlist,
    // the attribute list should be terminated with
    // GDK_GL_NONE.

    Canvas3DC(Canvas3DBodyC &bod) : WidgetC(bod)
    {}
    //: Body constructor

    bool GUIProcessReq(DObject3DC &obj)
    { return Body().GUIProcessReq(obj); }
    //: Process OpenGL requests.

    bool GUIDoLighting()
    { return Body().GUIDoLighting(); }
    //: Enable or disable lighting

    bool GUISwapBuffers()
    { return Body().GUISwapBuffers(); }
    //: swap buffers.
    // NB. Only call from the GUI thread.

    bool GUIBeginGL()
    { return Body().GUIBeginGL(); }
    //: Call before using any GL commands.
    // Should only be called by GUI thread.

    bool GUIEndGL()
    { return Body().GUIEndGL(); }
    //: Call aftern finished with GL
    // Should only be called by GUI thread.

    bool GUIClearBuffers()
    { return Body().GUIClearBuffers() ; }
    //: clear buffers
    // This will clear depth and color buffers
    // NB. Only call from the GUI thread

    bool Put(const DObject3DC &r)
    { return Body().Put(r); }
    //: Put render object on canvas.

    void ViewPoint(RealT fov = 90,
		   Point<RealT,3> nEye = Point<RealT,3>(0, 0, 5),
		   Point<RealT,3> nCentre  = Point<RealT,3>(0, 0, 0),
		   Vector<RealT,3> nUp = Vector<RealT,3>(0, 1, 0))
    { Put(DViewPoint3DC(fov, nEye, nCentre, nUp)); }
    //: Set the view point.
    // Thread safe.

    void Light(const RealRGBValueC &colour, const Point<RealT,3> &position, int nn = 0)
    { Put(DLight3DC(colour, position, nn)); }
    //: Setup a light nn.
    // Thread safe

    void Transform(RealT nAngle, const Vector<RealT,3> &nAxis)
    { Put(DTransform3DC(nAngle, nAxis)); }
    //: Rotation Constructor.

    void Transform(RealT nAngle, const Vector<RealT,3> &nAxis, const Vector<RealT,3> &nTrans)
    { Put(DTransform3DC(nAngle, nAxis, nTrans)); }
    //: Rotation/Translation Constructor.

    void Transform(RealT nAngle, const Vector<RealT,3> &nAxis, const Vector<RealT,3> &nTrans,const DObject3DC &obj)
    { Put(DTransform3DC(nAngle, nAxis, nTrans, obj)); }
    //: Rotation/Translation Constructor.

    void Transform(const Vector<RealT,3> &nTrans)
    { Put(DTransform3DC(nTrans)); }
    //: Translation Constructor.

    void Transform(RealT nAngle, const Vector<RealT,3> &nAxis, const DObject3DC &obj)
    { Put(DTransform3DC(nAngle, nAxis, obj)); }
    //: Constructor.

    void Render(bool (*nfunc)())
    { Put(DOpenGLC(CallFunc0C<bool>(nfunc))); }
    //: Call OpenGL rendering function.

    template<class DataT>
    void Render(bool (*nfunc)(DataT &dat),const DataT &dat)
    { Put(DOpenGLC(CallFunc1C<DataT &>(nfunc, dat))); }
    //: Call OpenGL rendering function.

    template<class Data1T,class Data2T>
    void Render(bool (*nfunc)(Data1T &dat1, Data2T &dat2), const Data1T &dat1, const Data2T &dat2)
    { Put(DOpenGLC(CallFunc2C<Data1T &,Data2T &>(nfunc, dat1, dat2))); }
    //: Call OpenGL rendering function.

    template<class ObjT>
    void Render(const ObjT &nobj,bool (ObjT::*nfunc)())
    { Put(DOpenGLC(CallMethod0C<ObjT>(nobj, nfunc))); }
    //: Call OpenGL rendering function.

    template<class ObjT,class DataT>
    void Render(const ObjT &nobj,bool (ObjT::*nfunc)(DataT &),const DataT &dat)
    { Put(DOpenGLC(CallMethod1C<ObjT,DataT &>(nobj,nfunc,dat))); }
    //: Call OpenGL rendering function.

    template<class ObjT,class Data1T,class Data2T>
    void Render(const ObjT &nobj,bool (ObjT::*nfunc)(Data1T &,Data2T &),const Data1T &dat1,const Data2T &dat2)
    { Put(DOpenGLC(CallMethod2C<ObjT,Data1T &,Data2T &>(nobj,nfunc,dat1,dat2))); }
    //: Call OpenGL rendering function.

    template<class ObjT>
    void RenderRef(ObjT &nobj,bool (ObjT::*nfunc)())
    { Put(DOpenGLC(CallMethod0C<ObjT &>(nobj,nfunc))); }
    //: Call OpenGL rendering function.
    // Use only a reference to 'nobj', not a copy.
    // NB. This means the reference counter will NOT be incremented.

    template<class ObjT,class DataT>
    void RenderRef(ObjT &nobj,bool (ObjT::*nfunc)(DataT &),const DataT &dat)
    { Put(DOpenGLC(CallMethod1C<ObjT &,DataT &>(nobj,nfunc,dat))); }
    //: Call OpenGL rendering function.
    // Use only a reference to 'nobj', not a copy.
    // NB. This means the reference counter will NOT be incremented.

    template<class ObjT,class Data1T,class Data2T>
    void RenderRef(ObjT &nobj,bool (ObjT::*nfunc)(Data1T &,Data2T &),const Data1T &dat1,const Data2T &dat2)
    { Put(DOpenGLC(CallMethod2C<ObjT &,Data1T &,Data2T &>(nobj,nfunc,dat1,dat2))); }
    //: Call OpenGL rendering function.
    // Use only a reference to 'nobj', not a copy.
    // NB. This means the reference counter will NOT be incremented.

    void SwapBuffers()
    { RenderRef(*this,&Canvas3DC::GUISwapBuffers); }
    //: Swap display buffers.
    // Thread safe.

    void ClearBuffers(void)
    { RenderRef(*this, &Canvas3DC::GUIClearBuffers) ; }
    //: Clear buffers
    // Clears color buffer and depth buffer
    // Thread safe

    bool SetTextureMode(bool &bTexture)
    { return Body().SetTextureMode(bTexture); }
    //: Enable or disable texturing

    bool SetLightingMode(bool &bLighting)
    { return Body().SetLightingMode(bLighting); }
    //: Enable or disable lighting

    bool SetRenderMode(Canvas3DRenderMode eRenderMode)
    { return Body().SetRenderMode(eRenderMode); }
    //: Set rendering mode

    bool GetTextureMode(void) const
    { return Body().GetTextureMode(); }
    //: Is texturing enabled?

    bool GetLightingMode(void) const
    { return Body().GetLightingMode(); }
    //: Is lighting enabled?

    Canvas3DRenderMode GetRenderMode(void) const
    { return Body().GetRenderMode(); }
    //: Get rendering mode

    bool HaveExtNonPowerOfTwoTexture() const
    { return Body().HaveExtNonPowerOfTwoTexture(); }
    //: Test if we have non power of two textures
    
    const GLContextC &GUIGLContext() 
    { return Body().GUIGLContext(); }
    //: Access opengl context for canvas.
    
    bool SaveToImage(Ravl2::Array<ByteRGBValueC,2> &img)
    { return Body().SaveToImage(img); }
    //: Write contents of widget to an image.
    
  protected:
    Canvas3DBodyC &Body()
    { return static_cast<Canvas3DBodyC &>(WidgetC::Body()); }

    const Canvas3DBodyC &Body() const
    { return static_cast<const Canvas3DBodyC &>(WidgetC::Body()); }

  };

}

#endif
