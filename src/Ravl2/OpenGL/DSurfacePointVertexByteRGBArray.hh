// This file is part of RAVL, Recognition And Vision Library
// Copyright (C) 2002, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here
//! docentry="Ravl.API.Graphics.3D"

#pragma once

#include "Ravl2/OpenGL/DObject3D.hh"
#include "Ravl2/3D/SurfacePoint3dArray.hh"
#include "Ravl2/3D/Vertex<RealT>olourByteRGB.hh"

namespace Ravl2
{

  //: Direct OpenGL render of vertex array, using current colour.
  void DSurfacePointVertexByteRGBArray(const SurfacePoint3dArrayC<VertexColourByteRGBC>& s);

  //: Rendering class for vertex array.
  //  Does not set the colour, so will use current OpenGL colour.
  class DSurfacePointVertexByteRGBArrayBodyC : public DObject3DBodyC
  {
  public:
    DSurfacePointVertexByteRGBArrayBodyC(const SurfacePoint3dArrayC<VertexColourByteRGBC>& s)
      : surface(s)
      {}
    // constructor

    virtual bool GUIRender(Canvas3DC &c3d) const
    {
      DSurfacePointVertexByteRGBArray(surface);
      return true;
    }
    //: render object

    virtual Vector<RealT,3> GUICenter() const;
    //: Get center of object.

    virtual RealT GUIExtent() const;
    //: Get extent of object.

  protected:
    SurfacePoint3dArrayC<Vertex<RealT>olourByteRGBC> surface;
  };

}


#endif
