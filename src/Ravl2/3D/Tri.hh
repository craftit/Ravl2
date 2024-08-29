// This file is part of RAVL, Recognition And Vision Library 
// Copyright (C) 2002, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here
//! docentry="Ravl.API.3D.Mesh"
//! author="Charles Galambos"

#pragma once

#include <array>
#include "Ravl2/3D/Vertex.hh"
#include "Ravl2/Assert.hh"
#include "Ravl2/Types.hh"
#include "Ravl2/Geometry/Geometry.hh"

namespace Ravl2
{
  //: Triangle in TriMesh.
  // Tri's must be associated with a tri set, in the parent
  // triset goes out of scope the behaviour of a tri from
  // it is undefined.

  template <typename RealT>
  class Tri
  {
  public:
    Tri() = default;
    //: Default constructor.
    
    explicit Tri(const std::array<Vertex<RealT> *,3> &v)
      : vertices(v)
    {}
    //: Construct from another vector.
    
    Tri(Vertex<RealT> &v0,Vertex<RealT> &v1,Vertex<RealT> &v2)
    {
      vertices[0] = &v0;
      vertices[1] = &v1;
      vertices[2] = &v2;
    }
    //: Construct from vertices.

    Tri(Vertex<RealT> &v0,Vertex<RealT> &v1,Vertex<RealT> &v2,
	 const Point<RealT,2> &tex0,const Point<RealT,2> &tex1, const Point<RealT,2> &tex2,
	 uint8_t texId = 0
	 )
    {
      vertices[0] = &v0;
      vertices[1] = &v1;
      vertices[2] = &v2;
      texture[0] = tex0;
      texture[1] = tex1;
      texture[2] = tex2;
      textureID = texId;
      colour[0] = 196;
      colour[1] = 196;
      colour[2] = 196;
    }
    //: Construct from vertices and texture coordinates.
    
    Tri(const std::array<Vertex<RealT> *,3> &v,
	 const std::array<Vector<RealT,2>,3> &t,
	 uint8_t texID)
      : vertices(v), 
        texture(t),
        textureID(texID)
    {}
    //: Construct from vertices, texture co-ords, and texture ID.

    void Flip()
    {
      std::swap(vertices[0],vertices[2]);
      std::swap(texture[0],texture[2]);
      normal = normal * -1;
    }
    //: Flips the triangle.
    // Reverse the order of the vertices in the triangle.
    
    Vertex<RealT> &vertex(unsigned ind) {
      RavlAssert(ind < 3);
      return *(vertices[ind]);
    }
    //: Access vertex.
    
    [[nodiscard]] const Vertex<RealT> &vertex(unsigned ind) const
    { return *(vertices[ind]); }
    //: Access vertex.
    
    const Vector<RealT,3> &operator[](unsigned ind) const
    { return vertices[ind]->Position(); }
    //: Access position of vertex.

    Vector<RealT,3> &operator[](unsigned ind)
    { return vertices[ind]->Position(); }
    //: Access position of vertex.
    
    [[nodiscard]] const Vector<RealT,3> &FaceNormal() const
    { return normal; }
    //: Unit normal orthogonal to triangle plane
    
    Vector<RealT,3> &FaceNormal()
    { return normal; }
    //: Unit normal orthogonal to triangle plane
    
    void SetFaceNormal(const Vector<RealT,3> &val)
    { normal = val; }
    //: Update the face normal.
    
    [[nodiscard]] Vector<RealT,3> &Normal(unsigned n)
    { return vertices[n]->Normal(); }
    //: Access normal for a vertex.
    
    [[nodiscard]] const Vector<RealT,3> Normal(unsigned n) const
    { return vertices[n]->Normal(); }
    //: Access normal for a vertex.

    [[nodiscard]] Vector<RealT,3> &Position(unsigned n)
    { return vertices[n]->Position(); }
    //: Access normal for a vertex.
    
    [[nodiscard]] const Vector<RealT,3> &Position(unsigned n) const
    { return vertices[n]->Position(); }
    //: Access normal for a vertex.
    
    void UpdateFaceNormal()
    {
      normal = cross(Vector<RealT,3>(Position(1) - Position(0)),(Position(2) - Position(0)));
      normal /= xt::norm_l2(normal);
    }

    //: Update the face normal.
    
    Vertex<RealT> *&VertexPtr(unsigned n)
    { return vertices[n]; }
    //: Access vertex pointer.
    // Advanced users only.
    
    void SetVertexPtr(unsigned n,Vertex<RealT> *vp)
    { vertices[n] = vp; }
    //: Access vertex pointer.
    // Advanced users only.
    
    [[nodiscard]] Vertex<RealT> *VertexPtr(unsigned n) const
    { return vertices[n]; }
    //: Access vertex pointer.
    // Advanced users only.
    
    [[nodiscard]] uint8_t& TextureID()
    { return textureID; }
    //: Access the texture ID.
    
    [[nodiscard]] uint8_t TextureID() const
    { return textureID; }
    //: Access the texture ID.
    
    void SetTextureID(uint8_t id)
    { textureID = id; }
    //: Set the texture id.
    
    [[nodiscard]] Vector<RealT,2> &TextureCoord(unsigned n)
    { return texture[n]; }
    //: Access texture co-ordinates.
    
    void SetTextureCoord(unsigned n,const Vector<RealT,2> &tc) 
    { texture[n] = tc; }
    //: Access texture co-ordinates.
    
    [[nodiscard]] const Vector<RealT,2> &TextureCoord(unsigned n) const
    { return texture[n]; }
    //: Access texture co-ordinates.
    
    [[nodiscard]] std::array<Vector<RealT,2>,3> &TextureCoords()
    { return texture; }
    //: Access texture co-ordinates.
    
    [[nodiscard]] const std::array<Vector<RealT,2>,3> &TextureCoords() const
    { return texture; }
    //: Access texture co-ordinates.
    
    [[nodiscard]] Vector<uint8_t,3> &Colour()
    { return colour; }
    //: Colour of face.
    
    void SetColour(const Vector<uint8_t,3> &col)
    { colour = col; }
    //: Set colour of face.
    
    [[nodiscard]] const Vector<uint8_t,3> &Colour() const
    { return colour; }
    //: Colour of face.
    
  protected:
    std::array<Vertex<RealT> *,3> vertices;
    std::array<Vector<RealT,2>,3> texture;
    Vector<RealT,3> normal {0,0,0} ;
    Vector<uint8_t,3> colour { 196, 196, 196 };
    uint8_t textureID = 0;
  };

  extern template class Tri<float>;

}
