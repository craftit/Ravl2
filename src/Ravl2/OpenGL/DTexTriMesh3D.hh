// This file is part of RAVL, Recognition And Vision Library
// Copyright (C) 2001, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here
#ifndef RAVL_DTEXTRIMESH3D_HEADER
#define RAVL_DTEXTRIMESH3D_HEADER 1
///////////////////////////////////////////////////
//! docentry="Ravl.API.Graphics.3D"
//! author="James Smith"
//! date="2/4/2001"
//! example=exDTexTriMesh3d.cc

#include "Ravl2/GUI/DTriMesh3D.hh"
#include "Ravl2/GUI/GLContext.hh"
#include "Ravl2/3D/TexTriMesh.hh"

namespace Ravl2 {
  
  using namespace Ravl2;

  //: Draw a TexTriMesh
  class DTexTriMesh3DBodyC : public DTriMesh3DBodyC
  {
  public:
    DTexTriMesh3DBodyC(const TexTriMeshC &oTexTriMesh);
    //: Constructor.

    ~DTexTriMesh3DBodyC();
    //: Destructor

    virtual bool GUIRender(Canvas3DC &c3d) const;
    //: Render object.

  protected:
    TexTriMeshC tmodel;
    mutable std::vector<unsigned int> texNames;
    mutable GLContextC m_glContext; // Used for freeing textures.
  };
  
  //: Draw a TexTriMesh
  class DTexTriMesh3DC : public DTriMesh3DC
  {
  public:
    DTexTriMesh3DC()
      {}
    //: Default constructor.
    // creat an invalid handle.

    DTexTriMesh3DC(const TexTriMeshC &oTexTriMesh)
      : DTriMesh3DC(*new DTexTriMesh3DBodyC(oTexTriMesh))
      {}
    //: Constructor.
  };

  std::ostream &operator<<(std::ostream &strm,const DTexTriMesh3DC &);
  std::istream &operator>>(std::istream &strm,DTexTriMesh3DC &);
}
#endif
