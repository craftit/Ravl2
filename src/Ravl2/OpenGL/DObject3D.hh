// This file is part of RAVL, Recognition And Vision Library
// Copyright (C) 2001, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here
//! docentry="Ravl.API.Graphics.3D"
//! author="Charles Galambos"
//! date="12/04/1999"

#pragma once

#include <GL/gl.h>
#include "Ravl2/Pixel/Pixel.hh"
#include "Ravl2/Pixel/Colour.hh"
#include "Ravl2/Types.hh"
#include "Ravl2/Geometry/Geometry.hh"
#include "Ravl2/Assert.hh"

namespace Ravl2
{
  class Canvas3D;

  //! Display object base class in a 3D world.
  class DObject3D
  {
  public:
    //: Default constructor.
    DObject3D() = default;

    //: Destructor.
    // Make sure display list is free'd
    virtual ~DObject3D() = default;

    //: Render object. Make sure you called Canvas3DC::GUIBeginGL just before
    virtual bool GUIRender(Canvas3D &c3d) const = 0;

    //: Get center of object.
    // defaults to 0,0,0
    [[nodiscard]] virtual Vector<float,3> GUICenter() const = 0;
    //  { return Vector<RealT,3>(0,0,0); }

    //: Get extent of object.
    // defaults to 1
    [[nodiscard]] virtual float GUIExtent() const = 0;
    //  { return 1; }

    //: Render, checking for display lists.
    bool GUIRenderDL(Canvas3D &c3d);

    //: Set current colour.
    template<typename PixelT>
    static inline
    void GLColour(const PixelT &colour)
    { glColor3f(get<ImageChannel::Red,float>(colour),
		get<ImageChannel::Green,float>(colour),
		get<ImageChannel::Blue,float>(colour));
    }

    //! Set a colour.
    static inline
    void GLColour(const PixelRGB8 &colour)
    { glColor3ub(colour.get<ImageChannel::Red>(), colour.get<ImageChannel::Green>(), colour.get<ImageChannel::Blue>()); }

    //: Set a vertex.
    static inline
    void GLVertex(const Point<float,3> &v)
      { glVertex3f(v[0], v[1], v[2]); }

    //: Set a vertex.
    static inline
    void GLVertex(const Point<double,3> &v)
    { glVertex3d(v[0], v[1], v[2]); }

    static inline
    void GLNormal(const Vector<float,3> &v)
    { glNormal3f(v[0], v[1], v[2]); }
    //: Set a vertex.

    static inline
    void GLTexCoord(const Point<float,2> &p)
    { glTexCoord2f(p[1], p[0]); }
    //: Set a texture coordinate

    void EnableDisplayList()
    { m_useDisplayList = true; }
    //: Enable generation of a display list.

    [[nodiscard]] auto DisplayListID() const
      { return id; }
    //: Access display list ID.

    [[nodiscard]] auto &DisplayListID()
      { return id; }
    //: Access display list ID.

  protected:
    bool m_useDisplayList = false;
    GLuint id = std::numeric_limits<GLuint>::max(); // Display list id. -1=None. -2=To be generated...
  };


  ////////////////////////////////////
  //: Body for OpenGL code invocation class.

  class DOpenGLBodyC
    : public DObject3D
  {
  public:
    //: Constructor.
    DOpenGLBodyC(const std::function<void(Canvas3D &)> &se,
                 const Vector<float,3> &ncenter = toVector<float>(0,0,0),
                 float nextent = 1)
      : sigEvent(se),
	center(ncenter),
	extent(nextent)
    {}
    
    //: Get center of object.
    // defaults to 0,0,0
    Vector<float,3> GUICenter() const override
    { return center; }
    
    //: Get extent of object.
    // defaults to 1
    float GUIExtent() const override
    { return extent; }

  protected:
    //: Render object.
    bool GUIRender(Canvas3D &c3d) const override;

    std::function<void(Canvas3D &)> sigEvent;
    Vector<float,3> center = toVector<float>(0,0,0);
    float extent = 1.0;
  };
  

  ////////////////////////////////////

  //: Body of an  object set in a 3D world.
  class DObjectSet3DBodyC
      : public DObject3D
  {
  public:
    //: Default constructor.
    DObjectSet3DBodyC() = default;
    
    //: Render object.
    bool GUIRender(Canvas3D &c3d) const override;
    
    //: Get center of object.
    // defaults to 0,0,0
    Vector<float,3> GUICenter() const override;
    
    //: Get extent of object.
    // defaults to 1
    float GUIExtent() const override;
    
    //: Add object into list.
    void GUIAdd(const std::shared_ptr<DObject3D> &obj);

    //! Update the center and extent of the object.
    void UpdateCenterExtent();
  protected:
    std::vector<std::shared_ptr<DObject3D> > parts;
    Vector<float,3> center = toVector<float>(0,0,0);
    float extent = 1.0f;
    bool mUpdateNeeded = true;
  };
}

