// This file is part of RAVL, Recognition And Vision Library 
// Copyright (C) 2007, OmniPerception Ltd.
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here
//! docentry="Ravl.API.3D.Mesh"
//! author="Charles Galambos"

#include "Ravl2/3D/TriMesh.hh"

namespace Ravl2 {
  
  //: Create a flat plane
  // Where the length of each side is 'size'
  
  TriMesh<float> createTriMeshPlane(float size = 1.0);
  
  //: Create a cube.
  // Where the length of each side is 'size'
  
  TriMesh<float> createTriMeshCube(float size = 1.0, const Point<float,3> &origin = toPoint<float>(0, 0, 0));
  
  //: Create a sphere.
  // There must be at least 2 layers, and 2 slices.
  TriMesh<float> createTriMeshSphere(unsigned layers,unsigned slices,float radius = 1.0);
  
}

