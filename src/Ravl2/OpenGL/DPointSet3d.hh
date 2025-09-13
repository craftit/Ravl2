// This file is part of RAVL, Recognition And Vision Library
// Copyright (C) 2001, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here
//! docentry="Ravl.API.Graphics.3D"
//! author="Joel Mitchelson"
//! date="31/1/2002"

#include "Ravl2/OpenGL/DObject.hh"
#include "Ravl2/Geometry/PointSet.hh"

namespace Ravl2
{

  //! Draw a Point Set

  class DPointSet : public DObject
  {
  public:
    using RealT = float;
    
    DPointSet(const PointSet<RealT,3>& oPointSet)
      : pointSet(oPointSet)
    {}
    //! Constructor.

    bool GUIRender(Canvas3D &c3d) const override;
    //! Render object.

    Vector<RealT,3> GUICenter() const override;
    //! Get center of object.
    // defaults to 0,0,0

    RealT GUIExtent() const override;
    //! Get extent of object.
    // defaults to 1

  protected:
    PointSet<RealT,3> pointSet;
  };

}

