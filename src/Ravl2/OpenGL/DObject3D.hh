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
#include "Ravl2/Types.hh"
#include "Ravl2/Assert.hh"


namespace Ravl2
{
#if 0
  class Canvas3DC;

  //: Body of a basic display object in a 3D world.
  class DObject3DBodyC

  {
  public:
    using RealT = float;

    DObject3DBodyC()
      : id(-1)
    {}
    //: Default constructor.

    virtual ~DObject3DBodyC();
    //: Destructor.
    // Make sure display list is free'd

    virtual bool GUIRender(Canvas3DC &c3d) const = 0;
    //: Render object. Make sure you called Canvas3DC::GUIBeginGL just before

    virtual Vector<RealT,3> GUICenter() const = 0;
    //  { return Vector<RealT,3>(0,0,0); }
    //: Get center of object.
    // defaults to 0,0,0

    virtual RealT GUIExtent() const = 0;
    //  { return 1; }
    //: Get extent of object.
    // defaults to 1

    bool GUIRenderDL(Canvas3DC &c3d);
    //: Render, checking for display lists.

    static inline
    void GLColour(const RealRGBValueC &colour)
      { glColor3d(colour.Red(), colour.Green(), colour.Blue()); }
    //: Set current colour.

    static inline
    void GLVertex(const Point<RealT,3> &v)
      { glVertex3d(v[0], v[1], v.Z()); }
    //: Set a vertex.

    static inline
    void GLNormal(const Vector<RealT,3> &v)
      { glNormal3d(v[0], v[1], v.Z()); }
    //: Set a vertex.

    static inline
    void GLTexCoord(const Point<RealT,2> &p)
      { glTexCoord2d(p[1], p[0]); }
    //: Set a texture coordinate

    void EnableDisplayList()
      { id = -2; }
    //: Enable generation of a display list.

    IntT DisplayListID() const
      { return id; }
    //: Access display list ID.

    IntT &DisplayListID()
      { return id; }
    //: Access display list ID.

  protected:
    IntT id; // Display list id. -1=None. -2=To be generated...
  };


  ////////////////////////////////////
  //: Body for OpenGL code invocation class.

  class DOpenGLBodyC : public DObject3DBodyC
  {
  public:
    DOpenGLBodyC(const TriggerC &se,
                 const Vector<RealT,3> &ncenter = Vector<RealT,3>(0,0,0), RealT nextent = 1)
      : sigEvent(se),
	center(ncenter),
	extent(nextent)
    {}
    //: Constructor.

    virtual Vector<RealT,3> GUICenter() const
      { return center; }
    //: Get center of object.
    // defaults to 0,0,0

    virtual RealT GUIExtent() const
      { return extent; }
    //: Get extent of object.
    // defaults to 1

  protected:
    virtual bool GUIRender(Canvas3DC &c3d) const;
    //: Render object.

    mutable TriggerC sigEvent;
    Vector<RealT,3> center;
    RealT extent;
  };

  //: OpenGL code invocation class.
  class DOpenGLC : public DObject3DC
  {
  public:
    DOpenGLC()
    {}
    //: Default constructor.
    // NB. Creates an invalid handle.

    DOpenGLC(const TriggerC &se,
             const Vector<RealT,3> &ncenter = Vector<RealT,3>(0,0,0), RealT nextent = 1)
      : DObject3DC(*new DOpenGLBodyC(se, ncenter, nextent))
    {}
    //: Constructor.
  };



  ////////////////////////////////////

  //: Body of an  object set in a 3D world.
  class DObjectSet3DBodyC
      : public DObject3DBodyC
  {
  public:
    DObjectSet3DBodyC();
    //: Default constructor.

    virtual bool GUIRender(Canvas3DC &c3d) const;
    //: Render object.

    virtual Vector<RealT,3> GUICenter() const;
    //: Get center of object.
    // defaults to 0,0,0

    virtual RealT GUIExtent() const;
    //: Get extent of object.
    // defaults to 1

    inline void GUIAdd(const DObject3DC &obj)
      { parts.push_back(obj); gotExtent = gotCenter = false; }
    //: Add object into list.

  protected:
    std::vector<DObject3DC> parts;
    mutable bool gotCenter;
    mutable Vector<RealT,3> center;
    mutable bool gotExtent;
    mutable RealT extent;
  };

#endif
}

