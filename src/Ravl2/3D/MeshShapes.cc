// This file is part of RAVL, Recognition And Vision Library 
// Copyright (C) 2007, OmniPerception Ltd.
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here
//! author="Charles Galambos"

#include <vector>
#include "Ravl2/3D/MeshShapes.hh"

namespace Ravl2 {
  
  static Point<float,2> TexPnt(float v1,float v2)
  { return toPoint<float>(v1,v2); }
  
  static Point<float,3> World3d(float a1,float a2,float a3)
  { return toPoint<float>(a1,a2,a3); }
  
  //: Create a flat plane
  
  TriMesh<float> createTriMeshPlane(float size) {
    float hSize = size/2.0f;
    
    std::vector<Vertex<float>> vertex(4);
    Vector<float,3> norm = toPoint<float>(0,0,1);
    vertex[0] = Vertex<float>(World3d(-hSize,-hSize,0),norm);
    vertex[1] = Vertex<float>(World3d(-hSize, hSize,0),norm);
    vertex[2] = Vertex<float>(World3d( hSize, hSize,0),norm);
    vertex[3] = Vertex<float>(World3d( hSize,-hSize,0),norm);
    
    std::vector<Tri<float>> tri(2);
    tri[0] = Tri<float>(vertex[0],vertex[2],vertex[1],TexPnt(0,0),TexPnt(1,1),TexPnt(0,1));
    tri[1] = Tri<float>(vertex[0],vertex[3],vertex[2],TexPnt(0,0),TexPnt(1,0),TexPnt(1,1));
    
    tri[0].SetFaceNormal(norm);
    tri[1].SetFaceNormal(norm);
    
    return TriMesh<float>(std::move(vertex),std::move(tri),true);
  }

  // Create a cube.
  
  TriMesh<float> createTriMeshCube(float size, const Point<float,3> &origin)
  {
    float hSize = size/2.0f;
    std::vector<Vertex<float>> vertex(8);
    vertex[0] = Vertex<float>(World3d( hSize, hSize,-hSize) + origin);
    vertex[1] = Vertex<float>(World3d( hSize,-hSize,-hSize) + origin);
    vertex[2] = Vertex<float>(World3d(-hSize,-hSize,-hSize) + origin);
    vertex[3] = Vertex<float>(World3d(-hSize, hSize,-hSize) + origin);
    
    vertex[4] = Vertex<float>(World3d( hSize, hSize,hSize) + origin);
    vertex[5] = Vertex<float>(World3d( hSize,-hSize,hSize) + origin);
    vertex[6] = Vertex<float>(World3d(-hSize,-hSize,hSize) + origin);
    vertex[7] = Vertex<float>(World3d(-hSize, hSize,hSize) + origin);
    
    std::vector<Tri<float>> tri(12);
    unsigned tn = 0;
    float sep = 1.0f/60.0f;
    
    float tr0 = sep;
    float tr1 = 0.5f - sep;
    float tr2 = 0.5f + sep;
    float tr3 = 1.0f - sep;
    
    float tc0 = sep;
    float tc1 = 1.0f/3.0f - sep;
    float tc2 = 1.0f/3.0f + sep;
    
    float tc3 = 2.0f/3.0f - sep;
    float tc4 = 2.0f/3.0f + sep;
    float tc5 = 1.0f - sep;
    
    // Front
    tri[tn++] = Tri<float>(vertex[0],vertex[1],vertex[2],TexPnt(tr0,tc0),TexPnt(tr0,tc1),TexPnt(tr1,tc1));
    tri[tn++] = Tri<float>(vertex[2],vertex[3],vertex[0],TexPnt(tr1,tc1),TexPnt(tr1,tc0),TexPnt(tr0,tc0));
    
    // Back
    tri[tn++] = Tri<float>(vertex[6],vertex[5],vertex[4],TexPnt(tr2,tc0),TexPnt(tr2,tc1),TexPnt(tr3,tc1));
    tri[tn++] = Tri<float>(vertex[4],vertex[7],vertex[6],TexPnt(tr3,tc1),TexPnt(tr3,tc0),TexPnt(tr2,tc0));
    
    // Top
    tri[tn++] = Tri<float>(vertex[4],vertex[5],vertex[1],TexPnt(tr0,tc2),TexPnt(tr0,tc3),TexPnt(tr1,tc3));
    tri[tn++] = Tri<float>(vertex[0],vertex[4],vertex[1],TexPnt(tr1,tc2),TexPnt(tr0,tc2),TexPnt(tr1,tc3));
    
    // Bottom
    tri[tn++] = Tri<float>(vertex[7],vertex[3],vertex[2],TexPnt(tr2,tc2),TexPnt(tr3,tc2),TexPnt(tr3,tc3));
    tri[tn++] = Tri<float>(vertex[2],vertex[6],vertex[7],TexPnt(tr3,tc3),TexPnt(tr2,tc3),TexPnt(tr2,tc2));
    
    // Left
    tri[tn++] = Tri<float>(vertex[4],vertex[0],vertex[7],TexPnt(tr0,tc4),TexPnt(tr0,tc5),TexPnt(tr1,tc4));
    tri[tn++] = Tri<float>(vertex[0],vertex[3],vertex[7],TexPnt(tr0,tc5),TexPnt(tr1,tc5),TexPnt(tr1,tc4));
    
    // Right
    tri[tn++] = Tri<float>(vertex[1],vertex[5],vertex[2],TexPnt(tr2,tc5),TexPnt(tr2,tc4),TexPnt(tr3,tc5));
    tri[tn++] = Tri<float>(vertex[5],vertex[6],vertex[2],TexPnt(tr2,tc4),TexPnt(tr3,tc4),TexPnt(tr3,tc5));
    
    TriMesh<float> ret(std::move(vertex),std::move(tri),true);
    ret.UpdateVertexNormals();
    return ret;
  }
  
  // Create a sphere.
  // TODO: Create a nicer default texture map.
  // TODO: Fill in normals as we go
  
  TriMesh<float> createTriMeshSphere(unsigned layers,unsigned slices,float radius) {
    RavlAssert(layers >= 1);
    RavlAssert(slices >= 2);
    
    float texRowSize = 1.0f/float(layers);
    float texColSize = 1.0f/float(slices);
    
    float sliceStep = 2*std::numbers::pi_v<float> / float(slices);
    float layerStep = std::numbers::pi_v<float> / float(layers);
    
    std::vector<Vertex<float>> vertex(2 + (layers-1) * slices);
    unsigned numTri = slices * 2 + (layers-2) * slices * 2;
    //std::vector<unsigned> tri(numTri * 3);
    std::vector<Tri<float>> tri(numTri);
    
    
    unsigned tn = 0,vn = 0;
    
    // Cache cos and sin values for slice angles.
    std::vector<float> sliceCos(slices);
    std::vector<float> sliceSin(slices);
    
    float sliceAngle = -std::numbers::pi_v<float>;
    for(unsigned s = 0;s < slices;s++) {
      sliceCos[s] = std::cos(sliceAngle) * radius;
      sliceSin[s] = std::sin(sliceAngle) * radius;
      sliceAngle += sliceStep;
    }
      
    // Vertex's on last layer
    std::vector<unsigned> vLastLayer(slices);
    
    // ----- Put in top fan. -----
    
    auto topVert = vn;
    vertex[vn++] = Vertex<float>(World3d(0,radius,0));
    
    float layerAngle = layerStep;
    
    float lc = std::cos(layerAngle) * radius;
    float ls = std::sin(layerAngle);
    
    vLastLayer[0] = vn;
    vertex[vn++] = Vertex<float>(World3d(ls * sliceSin[0],lc,ls  * sliceCos[0]));
    
    float texRow0 = 0;
    float texRow1 = texRowSize;

    for(unsigned s = 1;s < slices;s++) {
      
      float texCol0 = texColSize * float(s);
      float texCol1 = texCol0 + texColSize;
      
      tri[tn++] = Tri<float>(vertex[topVert],vertex[vn-1],vertex[vn],
                       TexPnt(texRow0,texCol0 + texColSize/2.0f),TexPnt(texRow1,texCol0),TexPnt(texRow1,texCol1));
      
      vLastLayer[s] = vn;
      vertex[vn++] = Vertex<float>(World3d(ls * sliceSin[s],lc,ls  * sliceCos[s]));
    }

    {
      float texCol0 = 0;
      float texCol1 = texColSize;

      tri[tn++] = Tri<float>(vertex[topVert], vertex[vn - 1], vertex[vLastLayer[0]],
                             TexPnt(texRow0, texCol0 + texColSize / 2.0f), TexPnt(texRow1, texCol0), TexPnt(texRow1, texCol1));
    }
    
    
    // ---- Put in mid layers ----
    
    for(unsigned l = 1;l < (layers-1);l++) {
      layerAngle += layerStep;
      lc = std::cos(layerAngle) * radius;
      ls = std::sin(layerAngle);
      
      unsigned lastTopVert = vLastLayer[0];
      unsigned lastBotVert = vn;
      unsigned firstVirt = vn;
      vertex[vn++] = Vertex<float>(World3d(ls * sliceSin[0],lc,ls  * sliceCos[0]));
      
      texRow0 = float(l) * texRowSize;
      texRow1 = texRow0 + texRowSize;
      
      for(unsigned s = 1;s < slices;s++) {
        float texCol0 = texColSize * float(s);
        float texCol1 = texCol0 + texColSize;
        
	// Put in face triangles.
        tri[tn++] = Tri<float>(vertex[vn],vertex[lastTopVert],vertex[lastBotVert],
                         TexPnt(texRow1,texCol1),TexPnt(texRow0,texCol0),TexPnt(texRow1,texCol0));
        
        tri[tn++] = Tri<float>(vertex[vLastLayer[s]],vertex[lastTopVert],vertex[vn],
                         TexPnt(texRow0,texCol1),TexPnt(texRow0,texCol0),TexPnt(texRow1,texCol1));
        
	// Put in new vertex.
	lastTopVert = vLastLayer[s];
	lastBotVert = vn;
	vLastLayer[s] = vn;
	vertex[vn++] = Vertex<float>(World3d(ls * sliceSin[s],lc,ls  * sliceCos[s]));
      }
      
      float texCol0 = 0;
      float texCol1 = texColSize;
      
      // Put in final face triangles.
      tri[tn++] = Tri<float>(vertex[firstVirt],vertex[lastTopVert],vertex[lastBotVert],
                       TexPnt(texRow1,texCol1),TexPnt(texRow0,texCol0),TexPnt(texRow1,texCol0));
      
      tri[tn++] = Tri<float>(vertex[vLastLayer[0]],vertex[lastTopVert],vertex[firstVirt],
                       TexPnt(texRow0,texCol1),TexPnt(texRow0,texCol0),TexPnt(texRow1,texCol1));
      
      vLastLayer[0] = firstVirt;
    }
    
    // ---- Put in bottom fan ----
    
    auto botVert = vn;
    vertex[vn++] = Vertex<float>(World3d(0,-radius,0));
    
    // Put together face.
    texRow0 = 1.0f-texRowSize;
    texRow1 = 1.0f;
    
    unsigned nextVert = vLastLayer[0];
    {
      float texCol0 = 0;
      float texCol1 = texColSize;

      //tri[tn++] = Tri<float>(vertex[vLastLayer[slices-1]],vertex[botVert],vertex[nextVert],
      //                 TexPnt(texRow0,texCol1),TexPnt(texRow0,texCol0),TexPnt(texRow1,texCol0 + texColSize/2.0));

      tri[tn++] = Tri<float>(vertex[nextVert], vertex[vLastLayer[slices - 1]], vertex[botVert],
                             TexPnt(texRow0, texCol1), TexPnt(texRow0, texCol0), TexPnt(texRow1, texCol0 + texColSize / 2.0f));
    }
    
    unsigned lastVert = nextVert;
    
    
    for(unsigned s = 1;s < slices;s++) {
      // Put together face.
      nextVert = vLastLayer[s];
      
      float texCol0 = texColSize * float(s);
      float texCol1 = texCol0 + texColSize;
      
      tri[tn++] = Tri<float>(vertex[nextVert],vertex[lastVert],vertex[botVert],
                       TexPnt(texRow0,texCol1),TexPnt(texRow0,texCol0),TexPnt(texRow1,texCol0 + texColSize/2.0f));
      
      lastVert = nextVert;
    }
    
    RavlAssert(tn == tri.size());
    RavlAssert(vn == vertex.size());
    
    TriMesh<float> ret(std::move(vertex),std::move(tri),true);
    //ret.GenerateTextureCoords();
    ret.UpdateVertexNormals();
    return ret;
  }
  
}


