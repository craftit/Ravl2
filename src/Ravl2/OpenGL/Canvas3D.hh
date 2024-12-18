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
#include "Ravl2/OpenGL/DTransform.hh"
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
  class Canvas3D 
  {
  public:
    //! Create a 3D canvas
    Canvas3D(int x, int y, bool autoConfigure = true);
    
    //! Create a 3D canvas
    Canvas3D(const std::shared_ptr<GLContext> &context, bool autoConfigure = true);
    
    //! We don't want to copy this object.
    Canvas3D(const Canvas3D &) = delete;
    Canvas3D &operator=(const Canvas3D &) = delete;
    Canvas3D(Canvas3D &&) = delete;
    Canvas3D &operator=(Canvas3D &&) = delete;

    virtual ~Canvas3D() = default;

    //: Call before using any GL commands.
    // This is needed to select correct gl context for the canvas 3d widget
    // Should only be called by GUI thread.
    bool GUIBeginGL();

    //: Call after finished with GL
    // Should only be called by GUI thread.
    bool GUIEndGL();

    //: swap buffers.
    bool GUISwapBuffers();

    //: clears the buffers
    // depth buffer and colour buffer is cleared
    bool GUIClearBuffers();

    //: Process OpenGL requests. (renders obj on the 3d canvas)
    bool GUIProcessReq(DObject3D &obj);

    //: Put render instruction into pipe.
    bool put(std::shared_ptr<DObject3D> r);

    //: Enable or disable texturing
    bool SetTextureMode(bool bTexture)
    { m_bTexture = bTexture; return true; }

    //: Enable or disable lighting
    bool SetLightingMode(bool bLighting);

    //: Set rendering mode
    bool SetRenderMode(Canvas3DRenderMode& eRenderMode)
    { m_eRenderMode = eRenderMode; return true; }

    //: Is texturing enabled?
    [[nodiscard]] bool GetTextureMode(void) const
    { return m_bTexture; }

    //: Is lighting enabled?
    [[nodiscard]] bool GetLightingMode(void) const
    { return m_bLighting; }

    //: Get rendering mode
    [[nodiscard]] Canvas3DRenderMode GetRenderMode(void) const
    { return m_eRenderMode; }

    //: Setup lighting
    bool GUIDoLighting() const {
      if (m_bLighting) glEnable(GL_LIGHTING);
      else glDisable(GL_LIGHTING);
      return true;
    }

    //: Do we have non power of two textures?
    [[nodiscard]] bool HaveExtNonPowerOfTwoTexture() const
    { return m_glExtNonPowerOfTwoTexture; }

    //: Get opengl context.
    GLContext &GUIGLContext() {
      return *m_glContext;
    }

    bool SaveToImage(Ravl2::Array<PixelRGB8,2> &img);
    //: Write contents of widget to an image.
  protected:
    
    //bool SaveToImageInternal(Ravl2::Array<PixelRGB8,2> *img,SemaphoreRC &done);
    //: Write contents of widget to an image.
    
    virtual bool CBConfigureEvent();
    //: Handle configure event
    
    // Size of view port.
    // Only needed for widget creation
    int sx = 100;
    int sy = 100;

    Canvas3DRenderMode m_eRenderMode = C3D_SMOOTH; //: Rendering mode

    //: Texture mode
    // true = use texture when rendering.
    bool m_bTexture = false;

    //: Lighting mode
    // true = Use lighting when rendering.
    bool m_bLighting = true;

    bool m_autoConfigure = true;  //: Handle viewport configure events internally.
    
    bool m_glExtNonPowerOfTwoTexture = false; // Do we have non power of two textures ?
    bool m_initDone = false;
    
    std::shared_ptr<GLContext> m_glContext;
  private:
    //: Never do this.
  };


}

