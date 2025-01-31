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

#include "Ravl2/Pixel/Pixel.hh"
#include "Ravl2/OpenGL/DObject.hh"
#include "Ravl2/OpenGL/GLShader.hh"

namespace Ravl2
{

  //! Body of a  object set in a 3D world.

  class DCube3D
    : public DObject
  {
  public:
    DCube3D() = default;

    //! Constructor.
    explicit DCube3D(
      const Vector<float,3> &nDiag,
      const PixelRGB8 &col = {255, 128, 128})
      : mDiag(nDiag),
        mColour(col)
    {}

    //! Register object with OpenGL.
    bool GUIInit(Canvas3D &c3d) override;

    //! Render object.
    bool GUIRender(Canvas3D &c3d) const override;

    //! Get center of object.
    // defaults to 0,0,0
    [[nodiscard]] Vector<float,3> GUICenter() const override
    { return Vector<float,3>({0, 0, 0}); }

    //! Get extent of object.
    //! defaults to 1
    [[nodiscard]] float GUIExtent() const override
    { return std::max(mDiag[0], std::max(mDiag[1], mDiag[2])); }

  private:
    std::shared_ptr<GLShaderProgram> mShaderProgram;
    GLint mViewProjectionLocation = 0;
    Vector<float,3> mDiag = {1, 1, 1};
    PixelRGB8 mColour = {255, 128, 128};

    std::shared_ptr<GLVertexBuffer> mVertexBuffer;
    GLVertexArray mVertexArray;

  };


}

