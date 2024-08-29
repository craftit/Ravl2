// This file is part of RAVL, Recognition And Vision Library
// Copyright (C) 2002, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here

#include "Ravl2/OpenGL/DSurfacePointVertexArray.hh"

namespace Ravl2
{
  void DSurfacePointVertexArray(const SurfacePoint3dArrayC<Vertex<RealT>>& s)
  {
    glBegin(GL_POINTS);
    for (SArray1dIterC<Vertex<RealT>> i(s); i; i++)
    {
      glNormal3dv(&(*i).Normal()[0]);
      glVertex3dv(&(*i).Position()[0]);
    }
    glEnd();
  }

  //: Get center of object.
  Vector<RealT,3> DSurfacePointVertexArrayBodyC::GUICenter() const
  {
    Vector<RealT,3> vec(0,0,0);
    for(SArray1dIterC<Vertex<RealT>> it(surface);it;it++) {
      vec += it->Position();
    }
    vec /= (RealT)surface.size();
    return vec;
  }

  //: Get extent of object.
  RealT DSurfacePointVertexArrayBodyC::GUIExtent() const
  {
    Vector<RealT,3> vec(0,0,0);
    for(SArray1dIterC<Vertex<RealT>> it(surface);it;it++) {
      vec += it->Position();
    }
    vec /= (RealT)surface.size();
    RealT dist = 0;
    for(SArray1dIterC<Vertex<RealT>> it(surface);it;it++)
      dist = Max(dist,vec.EuclidDistance(it->Position()));
    return dist;
  }

}
