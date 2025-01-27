// This file is part of RAVL, Recognition And Vision Library
// Copyright (C) 2001, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here
//! docentry="Ravl.API.Graphics.3D"
//! author="James Smith"
//! date="2/4/2001"
//! example=exDTexTriMesh3d.cc

#pragma once

#include "Ravl2/OpenGL/DTriMesh3D.hh"
#include "Ravl2/OpenGL/GLContext.hh"
#include "Ravl2/3D/TexTriMesh.hh"

namespace Ravl2
{

  //! Draw a TexTriMesh
  class DTexTriMesh3DBodyC
    : public DTriMesh3D
  {
  public:
    DTexTriMesh3DBodyC(const std::shared_ptr<TexTriMesh<RealT> > &oTexTriMesh);
    //! Constructor.

    ~DTexTriMesh3DBodyC() override;
    //! Destructor

    bool GUIRender(Canvas3D &c3d) const override;
    //! Render object.

  protected:
    std::shared_ptr<TexTriMesh<float> > tmodel;
    std::vector<unsigned int> texNames;
    std::shared_ptr<GLContext> m_glContext; // Used for freeing textures.
  };
  

}
