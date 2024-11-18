// This file is part of RAVL, Recognition And Vision Library
// Copyright (C) 2001, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here
//! docentry="Ravl.API.Graphics.3D"
//! author="Joel Mitchelson"
//! date="31/1/2002"

#pragma once

#include "Ravl2/OpenGL/DObject3D.hh"
#include "Ravl2/IndexRange.hh"
#include "Ravl2/3D/PinholeCamera0.hh"

namespace Ravl2
{

  //: Use PinholeCamera0C class to set OpenGL viewpoint
  class DPinholeCamera03DBodyC
    : public DObject3D
  {
  public:
    DPinholeCamera03DBodyC(const PinholeCamera0<float>& _camera,
                           const IndexRange<2>& _canvas_region)
      : camera(_camera),
        canvas_region(_canvas_region)
     {}
    // Constructor.

    bool GUIRender(Canvas3D &c3d) const override;
    //: Render object.

    [[nodiscard]] Vector<float,3> GUICenter() const override
    { return toVector<float>(0, 0, 0); }
    //: Get center of object.
    // defaults to 0,0,0

    float GUIExtent() const override
    { return 1; }
    //: Get extent of object.
    // defaults to 1

  protected:
    PinholeCamera0<float> camera;
    IndexRange<2> canvas_region;
  };


}

