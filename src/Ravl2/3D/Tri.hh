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
#include "Ravl2/Pixel/Pixel.hh"
#include "Ravl2/Geometry/Geometry.hh"

namespace Ravl2
{
  //: Triangle in TriMesh.
  // Tri's must be associated with a tri set, in the parent
  // triset goes out of scope the behaviour of a tri from
  // it is undefined.

  template <typename RealT,typename PixelT = PixelRGB8>
  class Tri
  {
  public:
    Tri() = default;
    //: Default constructor.
    
    explicit Tri(const std::array<Vertex<RealT> *,3> &v)
      : mVertices(v)
    {}
    //: Construct from another vector.
    
    Tri(Vertex<RealT> &v0,Vertex<RealT> &v1,Vertex<RealT> &v2)
    {
      mVertices[0] = &v0;
      mVertices[1] = &v1;
      mVertices[2] = &v2;
    }
    //: Construct from vertices.

    Tri(Vertex<RealT> &v0,Vertex<RealT> &v1,Vertex<RealT> &v2,
	 const Point<RealT,2> &tex0,const Point<RealT,2> &tex1, const Point<RealT,2> &tex2,
	 uint8_t texId = 0
	 )
    {
      mVertices[0] = &v0;
      mVertices[1] = &v1;
      mVertices[2] = &v2;
      mTexture[0] = tex0;
      mTexture[1] = tex1;
      mTexture[2] = tex2;
      mTextureID = texId;
      mColour[0] = 196;
      mColour[1] = 196;
      mColour[2] = 196;
    }
    //: Construct from vertices and texture coordinates.
    
    Tri(const std::array<Vertex<RealT> *,3> &v,
	 const std::array<Vector<RealT,2>,3> &t,
	 uint8_t texID)
      : mVertices(v),
        mTexture(t),
        mTextureID(texID)
    {}
    //: Construct from vertices, texture co-ords, and texture ID.

    void Flip()
    {
      std::swap(mVertices[0],mVertices[2]);
      std::swap(mTexture[0],mTexture[2]);
      mNormal = mNormal * -1;
    }
    //: Flips the triangle.
    // Reverse the order of the vertices in the triangle.
    
    Vertex<RealT> &vertex(unsigned ind) {
      RavlAssert(ind < 3);
      return *(mVertices[ind]);
    }
    //: Access vertex.
    
    [[nodiscard]] const Vertex<RealT> &vertex(unsigned ind) const
    { return *(mVertices[ind]); }
    //: Access vertex.
    
    const Vector<RealT,3> &operator[](unsigned ind) const
    { return mVertices[ind]->position(); }
    //: Access position of vertex.

    Vector<RealT,3> &operator[](unsigned ind)
    { return mVertices[ind]->position(); }
    //: Access position of vertex.
    
    [[nodiscard]] const Vector<RealT,3> &FaceNormal() const
    { return mNormal; }
    //: Unit normal orthogonal to triangle plane
    
    Vector<RealT,3> &FaceNormal()
    { return mNormal; }
    //: Unit normal orthogonal to triangle plane
    
    void SetFaceNormal(const Vector<RealT,3> &val)
    { mNormal = val; }
    //: Update the face normal.
    
    [[nodiscard]] Vector<RealT,3> &normal(unsigned n)
    { return mVertices[n]->normal(); }
    //: Access normal for a vertex.
    
    [[nodiscard]] Vector<RealT,3> normal(unsigned n) const
    { return mVertices[n]->normal(); }
    //: Access normal for a vertex.

    [[nodiscard]] Vector<RealT,3> &Position(unsigned n)
    { return mVertices[n]->position(); }
    //: Access normal for a vertex.
    
    [[nodiscard]] const Vector<RealT,3> &Position(unsigned n) const
    { return mVertices[n]->position(); }
    //: Access normal for a vertex.
    
    void UpdateFaceNormal()
    {
      mNormal = cross<RealT>(Vector<RealT,3>(Position(1) - Position(0)),(Position(2) - Position(0)));
      mNormal /= mNormal.norm();
    }

    //: Update the face normal.
    
    Vertex<RealT> *&VertexPtr(unsigned n)
    { return mVertices[n]; }
    //: Access vertex pointer.
    // Advanced users only.
    
    void SetVertexPtr(unsigned n,Vertex<RealT> *vp)
    { mVertices[n] = vp; }
    //: Access vertex pointer.
    // Advanced users only.
    
    [[nodiscard]] Vertex<RealT> *VertexPtr(unsigned n) const
    { return mVertices[n]; }
    //: Access vertex pointer.
    // Advanced users only.
    
    [[nodiscard]] uint8_t& TextureID()
    { return mTextureID; }
    //: Access the texture ID.
    
    [[nodiscard]] uint8_t TextureID() const
    { return mTextureID; }
    //: Access the texture ID.
    
    void SetTextureID(uint8_t id)
    { mTextureID = id; }
    //: Set the mTexture id.
    
    [[nodiscard]] Vector<RealT,2> &TextureCoord(unsigned n)
    { return mTexture[n]; }
    //: Access texture co-ordinates.
    
    void SetTextureCoord(unsigned n,const Vector<RealT,2> &tc) 
    { mTexture[n] = tc; }
    //: Access texture co-ordinates.
    
    [[nodiscard]] const Vector<RealT,2> &TextureCoord(unsigned n) const
    { return mTexture[n]; }
    //: Access texture co-ordinates.
    
    [[nodiscard]] std::array<Vector<RealT,2>,3> &TextureCoords()
    { return mTexture; }
    //: Access texture co-ordinates.
    
    [[nodiscard]] const std::array<Vector<RealT,2>,3> &TextureCoords() const
    { return mTexture; }
    //: Access texture co-ordinates.
    
    [[nodiscard]] PixelT &Colour()
    { return mColour; }
    //: Colour of face.
    
    void SetColour(const PixelT &col)
    { mColour = col; }
    //: Set colour of face.
    
    [[nodiscard]] const auto &Colour() const
    { return mColour; }
    //: Colour of face.

    template <class Archive>
    void serialize(Archive &archive)
    {
      // We can't save vertices directly, so we save the indices of the vertices elsewhere
      archive(cereal::make_nvp("texture", mTexture),
              cereal::make_nvp("normal", mNormal),
              cereal::make_nvp("colour", mColour),
              cereal::make_nvp("textureID", mTextureID));
    }

  protected:
    std::array<Vertex<RealT> *,3> mVertices;
    std::array<Vector<RealT,2>,3> mTexture;
    Vector<RealT,3> mNormal {0,0,0} ;
    PixelT mColour { 196, 196, 196 };
    uint8_t mTextureID = 0;
  };

  extern template class Tri<float>;

}
