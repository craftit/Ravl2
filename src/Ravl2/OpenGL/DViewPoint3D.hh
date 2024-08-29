// This file is part of RAVL, Recognition And Vision Library
// Copyright (C) 2001, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here
//! docentry="Ravl.API.Graphics.3D"
//! author="Charles Galambos"
//! date="12/04/99"

#pragma once

#include "Ravl2/OpenGL/DObject3D.hh"

namespace Ravl2 {

  //: Setup a view point in the 3D world.
  // See handles description for more information.

  class DViewPoint3DBodyC : public DObject3DBodyC
  {
  public:
    DViewPoint3DBodyC(RealT nFov,
		      Point<RealT,3> nEye,    // Position of eye.
		      Point<RealT,3> nCentre, // Centre of object to look at.
		      Vector<RealT,3> nUp,   // Up direction.
		      RealT dNear,
		      RealT dFar
		      )
      : fov(nFov),
	eye(nEye),
	centre(nCentre),
	up(nUp),
	m_dNear(dNear),
	m_dFar(dFar)
    {}
    //: Default constructor.

    virtual bool GUIRender(Canvas3DC &c3d) const;
    //: Render object.

    virtual Vector<RealT,3> GUICenter() const
      { return Vector<RealT,3>(0, 0, 0); }
    //: Get center of object.
    // defaults to 0,0,0

    virtual RealT GUIExtent() const
      { return 1; }
    //: Get extent of object.
    // defaults to 1

  protected:
    RealT fov;       // Field of view angle.
    Point<RealT,3> eye;    // Position of eye.
    Point<RealT,3> centre; // Centre of object to look at.
    Vector<RealT,3> up;    // Up direction.
    RealT m_dNear, m_dFar; // Clipping planes
  };

  //: Setup a view point in the 3D world.
  // This also does some default setup of depth buffers, and shape
  // models.  If you wish to configures these aspects yours self you
  // should do so after using this class. (Or not use it at all.)

  class DViewPoint3DC : public DObject3DC
  {
  public:
    DViewPoint3DC(RealT fov = 90,
		  Point<RealT,3> nEye = Point<RealT,3>(0, 0, 10),
		  Point<RealT,3> nCentre  = Point<RealT,3>(0, 0, 0),
		  Vector<RealT,3> nUp = Vector<RealT,3>(0,1,0),
		  RealT dNear = 0, RealT dFar = 100)
    : DObject3DC(*new DViewPoint3DBodyC(fov,nEye,nCentre,nUp,dNear,dFar))
      {}
    //: Constructor.
    // Position of eye. (0,0,-1)
    // Centre of object (0,0,0)
    // Up direction.  (Y Axis.)
  };

}

