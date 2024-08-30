// This file is part of RAVL, Recognition And Vision Library
// Copyright (C) 2001, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here
//! docentry="Ravl.API.Graphics.3D"
//! example=exCanvas3D.cc
//! author="Charles Galambos"
//! date="12/04/1999"

#pragma once

#include "Ravl2/OpenGL/DObject3D.hh"
#include "Ravl2/OpenGL/Util.hh"
#include "Ravl2/OpenGL/DViewPoint3D.hh"
#include "Ravl2/OpenGL/DLight3D.hh"
#include "Ravl2/OpenGL/GLContext.hh"

typedef struct _GdkGLContext GdkGLContext;
typedef struct _GdkVisual GdkVisual;

namespace Ravl2
{

  //: 3D Canvas Rendering modes
  enum Canvas3DRenderMode {
    C3D_POINT,
    C3D_WIRE,
    C3D_FLAT,
    C3D_SMOOTH
  };

  //: 3D Canvas body.
  class Canvas3DBodyC 
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


}

