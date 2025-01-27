// This file is part of RAVL, Recognition And Vision Library
// Copyright (C) 2001, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here
//! docentry="Ravl.API.Graphics.3D"
//! author="James Smith"
//! date="2/4/2001"

#pragma once

#include "Ravl2/OpenGL/DObject3D.hh"
#include "Ravl2/3D/TriMesh.hh"

namespace Ravl2 {

  //: Draw a TriMesh

  class DTriMesh3D
    : public DObject3D
  {
  public:
    using RealT = float;

    //: Constructor.
    explicit DTriMesh3D(const std::shared_ptr<TriMesh<RealT> > &oTriMesh);

    //: Render object.
    bool GUIRender(Canvas3D &c3d) const override;

    //: Get center of object.
    // defaults to 0,0,0
    [[nodiscard]] Vector<float,3> GUICenter() const override;

    //: Get extent of object.
    // defaults to 1
    float GUIExtent() const override;

  protected:
    //: Compute center and extent of mesh.
    void ComputeInfo();

    std::shared_ptr<TriMesh<RealT> > model;
    bool mUseMeshColour = true;
    Vector<RealT,3> center = {0,0,0};
    RealT extent = 1.0;
  };

}
