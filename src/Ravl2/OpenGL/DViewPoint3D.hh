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
  // This also does some default setup of depth buffers, and shape
  // models.  If you wish to configures these aspects yours self you
  // should do so after using this class. (Or not use it at all.)

  class DViewPoint3DBodyC : public DObject3D
  {
  public:
    using RealT = float;

    //: Constructor.
    // Position of eye. (0,0,-1)
    // Centre of object (0,0,0)
    // Up direction.  (Y Axis.)
    DViewPoint3DBodyC(RealT nFov = 90,
		      Point<RealT,3> nEye = toPoint<RealT>(0, 0, 10),    // Position of eye.
		      Point<RealT,3> nCentre = toPoint<RealT>(0, 0, 0), // Centre of object to look at.
		      Vector<RealT,3> nUp = toVector<RealT>(0,1,0),   // Up direction.
		      RealT dNear = 0,
		      RealT dFar = 100
		      )
      : fov(nFov),
	eye(nEye),
	centre(nCentre),
	up(nUp),
	m_dNear(dNear),
	m_dFar(dFar)
    {}
    //: Default constructor.

    bool GUIRender(Canvas3D &c3d) const override;
    //: Render object.

    [[nodiscard]] Vector<RealT,3> GUICenter() const override
      { return Vector<RealT,3>({0, 0, 0}); }
    //: Get center of object.
    // defaults to 0,0,0

    [[nodiscard]] RealT GUIExtent() const override
      { return 1; }
    //: Get extent of object.
    // defaults to 1

  protected:
    RealT fov = 1.0;       // Field of view angle.
    Point<RealT,3> eye = {0,0,10};    // Position of eye.
    Point<RealT,3> centre = {0,0,0}; // Centre of object to look at.
    Vector<RealT,3> up = {0,1,0};    // Up direction.
    RealT m_dNear=0;
    RealT m_dFar=100; // Clipping planes
  };


}

