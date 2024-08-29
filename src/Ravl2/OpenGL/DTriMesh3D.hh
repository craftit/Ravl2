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
  using namespace Ravl2;

  //: Draw a TriMesh

  class DTriMesh3DBodyC : public DObject3DBodyC
  {
  public:
    DTriMesh3DBodyC(const TriMeshC &oTriMesh);
    //: Constructor.

    virtual bool GUIRender(Canvas3DC &c3d) const;
    //: Render object.

    virtual Vector<RealT,3> GUICenter() const;
    //: Get center of object.
    // defaults to 0,0,0

    virtual RealT GUIExtent() const;
    //: Get extent of object.
    // defaults to 1

  protected:
    TriMeshC model;

    bool mUseMeshColour = true;

    void ComputeInfo() const;
    //: Comput center and extent of mesh.

    mutable bool doneInfo;
    mutable Vector<RealT,3> center;
    mutable RealT extent;
  };

}
