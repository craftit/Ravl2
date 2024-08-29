// This file is part of RAVL, Recognition And Vision Library
// Copyright (C) 2001, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here

#include "Ravl2/GUI/DPointSet3d.hh"
#include "Ravl2/GUI/Canvas3D.hh"
#include "GL/gl.h"

#define DODEBUG 0
#if DODEBUG
#define ONDEBUG(x) x
#else
#define ONDEBUG(x)
#endif

namespace Ravl2 {

  // Render object.
  bool DPointSet3dBodyC::GUIRender(Canvas3DC& canvas) const
  {
    ONDEBUG(std::cerr << "DPointSet3dBodyC::GUIRender(), Called. \n");
    // std::cerr << "Point set render number: " << pointSet.RenderNumber() << std::endl;

    glColor3d(1.0,1.0,1.0);
    glBegin(GL_POINTS);
    for (DLIterC<Point<RealT,3>> it(pointSet); it; it++) {
      Point<RealT,3> v = *it;
      glVertex3d(v[0],v[1],v[2]);
    }
    glEnd();

#if 0
    std::cerr << "vertex[0].Position(): " << verts[0].Position() << std::endl;
    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(3,GL_DOUBLE,sizeof(Vertex<RealT>),&verts[0].Position()[0]);
    glDrawArrays(GL_POINTS,0,pointSet.RenderNumber());
#endif

    return true;
  }


  //: Get center of object.
  // defaults to 0,0,0
  Vector<RealT,3> DPointSet3dBodyC::GUICenter() const
  {
    return pointSet.Centroid();
  }

  //: Get extent of object.
  // defaults to 1
  RealT DPointSet3dBodyC::GUIExtent() const
  {
    Vector<RealT,3> ncenter = pointSet.Centroid();
    RealT dist = 0;
    for(DLIterC<Point<RealT,3>> it(pointSet);it;it++)
      dist = Max(dist,Sqrt((ncenter - *it).SumOfSqr()));
    return dist;
  }

}

