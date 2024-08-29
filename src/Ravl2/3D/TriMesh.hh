// This file is part of RAVL, Recognition And Vision Library 
// Copyright (C) 2002, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here
//! docentry="Ravl.API.3D.Mesh"
//! author="Charles Galambos"

#pragma once

#include <vector>
#include "Ravl2/3D/Tri.hh"
#include "Ravl2/Index.hh"
#include "Ravl2/Geometry/Isometry3.hh"

namespace Ravl2
{

  //! Tri mesh
  //! This is designed to store meshes, and fast access for vertex and normal value.
  //! It is not really suitable for making changes to the mesh topology.

  template<class RealT>
  class TriMesh
  {
  public:
    //: Default constructor
    TriMesh() = default;

    //: Copy constructor
    TriMesh(const TriMesh& oth);

    //: Construct a mesh with a number of vertices and faces.
    // The initial values in the mesh are undefined.
    TriMesh(unsigned noVertices,unsigned noFaces)
      : vertices(noVertices),
	faces(noFaces)
    {}

    //: Construct from an array of vertices and an array of indices.
    // The length of faceInd should be a power of 3, successive triples of vertex indexes are taken to
    // form the faces of the mesh.
    TriMesh(const std::vector<Vector<RealT,3>> &v,const std::vector<unsigned> &faceInd);

    //: Construct from a list of vertices and a list of indices.
    TriMesh(const std::vector<Vector<RealT,3>> &v,const std::vector<Index<3>> &faceInd);

    //: Construct from an array of vertices and an array of tri's.
    // NOTE: The vertices referred to in the Tri<RealT>'s must be elements of array 'v'.
    TriMesh(std::vector<Vertex<RealT>> &&v,std::vector<Tri<RealT>> &&nfaces,bool haveTextureCoord=false)
      : vertices(std::move(v)),
	faces(std::move(nfaces)),
	haveTexture(haveTextureCoord)
    {}

    //: Access a list of all vertices in the mesh.
    std::vector<Vertex<RealT>> &Vertices()
    { return vertices; }

    //: Access a list of all vertices in the mesh.
    [[nodiscard]] const std::vector<Vertex<RealT>> &Vertices() const
    { return vertices; }

    //: Access array of triangles in the mesh.
    std::vector<Tri<RealT>> &Faces()
    { return faces; }

    //: Access array of triangles in the mesh.
    [[nodiscard]] const std::vector<Tri<RealT>> &Faces() const
    { return faces; }

    //: Flips the mesh surface
    void FlipTriangles(void);

    //: Centre of triset.
    // - average vertex position
    Vector<RealT,3> Centroid() const;

    //: transform each vert by rt
    void Transform(const Isometry3<RealT>&  rt);

    //: Create an array of faces indices.
    // each successive triple of indices represents a face in the mesh.
    std::vector<unsigned> FaceIndices() const;

    //: Find the index of a Tri<RealT>'s verte
    // Tri must be from this mesh.
    // no should be 0 to 2.
    [[nodiscard]] unsigned index(const Tri<RealT> &tri,unsigned no) const
    { return unsigned (tri.VertexPtr(no) - &(vertices[0])); }

    //: Find largest and smallest for each compoent of all vertices.
    void Limits(Vector<RealT,3> &min,Vector<RealT,3> &max) const;

    //: Recalculate vertex and face normals from actual angle of faces.
    void UpdateVertexNormals();

    //: Offset and Scale mesh by given values.
    void OffsetScale(const Vector<RealT,3> &off,RealT scale);

    //: Do we have texture coordinates?
    [[nodiscard]] bool HaveTextureCoord() const
    { return haveTexture; }

    //: Set have texture coordinates flag.
    void SetTextureCoord(bool val)
    { haveTexture = val; }

    //: Add another mesh to this one
    TriMesh<RealT> operator+ (const TriMesh<RealT> &t2) const;

    //: Automatically generate texture coordinates.
    bool GenerateTextureCoords();

    template <class Archive>
    void save(Archive &archive) const
    {
      archive(CEREAL_NVP(vertices),
              CEREAL_NVP(faces),
              CEREAL_NVP(haveTexture));
      auto faceIndices = FaceIndices();
      archive(CEREAL_NVP(faceIndices));
    }

    template <class Archive>
    void load(Archive &archive)
    {
      archive(CEREAL_NVP(vertices),
           CEREAL_NVP(faces),
           CEREAL_NVP(haveTexture));
      std::vector<unsigned> faceIndices;
      archive(CEREAL_NVP(faceIndices));
      for(size_t i = 0;i < faces.size();i++) {
        auto &face = faces[i];
        auto &index = faceIndices[i*3];
        face.VertexPtr(0) = &(vertices[index]);
        index = faceIndices[i*3+1];
        face.VertexPtr(1) = &(vertices[index]);
        index = faceIndices[i*3+2];
        face.VertexPtr(2) = &(vertices[index]);
      }

    }

  protected:
    std::vector<Vertex<RealT>> vertices; //!< Array of vertex positions.
    std::vector<Tri<RealT>> faces;     //!< Array of triangles.
    bool haveTexture = false; //!< Do we have texture coordinates?
  };

  template<class RealT>
  TriMesh<RealT>::TriMesh(const std::vector<Vector<RealT,3>> &v,const std::vector<unsigned> &faceInd)
  {
    faces.reserve(faceInd.size()/3);
    vertices.reserve(v.size());

    Vector<RealT,3> zero = toVector<RealT>()(0,0,0);
    for(auto it : v) {
      vertices.push_back(Vertex<RealT>(it,zero));
    }
    auto iit = faceInd.begin();
    for(unsigned i = 0;i < faceInd.size()/3;i++) {
      faces.push_back(Tri<RealT>(vertices[*iit],vertices[*(iit+1)],vertices[*(iit+2)]));
      iit += 3;
      auto &face = faces.back();
      face.UpdateFaceNormal();
      Vector<RealT,3> norm = face.FaceNormal();
      for(unsigned j = 0;j < 3;j++) {
        face.Normal(j) += norm;
      }
    }
    for(auto it : vertices) {
      it.makeUnitNormal();
    }
  }


  template<class RealT>
  TriMesh<RealT>::TriMesh(const std::vector<Vector<RealT,3>> &v,const std::vector<Index<3>> &faceInd)
  {
    vertices.reserve(v.size());
    faces.reserve(faceInd.size());

    // Create the vertices from the vertex positions v
    Vector<RealT,3> zero = toVector<RealT>()(0,0,0);
    for(auto it : v) {
      vertices.push_back(Vertex<RealT>(it,zero));
    }

    // Create the tri faces from the face vertex indices
    RavlAssert(faceInd.size() == (faces.size()));
    for(unsigned i = 0;i < faceInd.size();i++) {
      faces.push_back(Tri<RealT>(vertices[unsigned(faceInd[i][0])],vertices[unsigned(faceInd[i][1])],vertices[unsigned(faceInd[i][2])]));
      faces.back().UpdateFaceNormal();
      Vector<RealT,3> norm = faces.back().FaceNormal();
      for(unsigned j = 0;j < 3;j++) {
        faces.back().Normal(j) += norm;
      }
    }
    for(auto it : vertices) {
      it.makeUnitNormal();
    }
  }


  template<class RealT>
  TriMesh<RealT>::TriMesh(const TriMesh& oth)
   : vertices(oth.vertices),
     faces(oth.faces),
     haveTexture(oth.haveTexture)
  {
    // Update vertex pointers in faces
    for(size_t i = 0;i < faces.size();i++) {
      auto &face = faces[i];
      auto &othFace = oth.faces[i];
      for(unsigned j = 0; j < 3; j++) {
        face.VertexPtr(j) = &(vertices[oth.index(othFace, j)]);
      }
    }
  }

  template<class RealT>
  void TriMesh<RealT>::FlipTriangles() {
    for(auto it : faces) {
      it.Flip();
    }
  }

  template<class RealT>
  Vector<RealT,3> TriMesh<RealT>::Centroid() const {
    Vector<RealT,3> ret {0,0,0};
    if(vertices.size() == 0)
      return ret; // Can't take a centroid of an empty mesh.
    for(auto it : vertices) {
      ret += it.Position();
    }
    return ret / vertices.size();
  }


  //: Transform mesh with RT

  template<class RealT>
  void TriMesh<RealT>::Transform(const Isometry3<RealT> & rt)
  {
    for(auto &it : vertices)
    {
      Vector<RealT,3> v(it.Position());
      it.Position() = rt(v);
      it.Normal()=rt.rotation()(it.Normal());
    }
    for(auto &it : faces)
      it.FaceNormal()=rt.rotation()(it.FaceNormal());
  }

  template<class RealT>
  std::vector<unsigned> TriMesh<RealT>::FaceIndices() const {
    std::vector<unsigned> ret;
    ret.reserve(faces.size()*3);
    if(faces.size() == 0)
      return ret;
    const Vertex<RealT> *x = &(vertices[0]);
    for(auto it : faces) {
      ret.push_back(unsigned(it.VertexPtr(0) - x));
      ret.push_back(unsigned(it.VertexPtr(1) - x));
      ret.push_back(unsigned(it.VertexPtr(2) - x));
    }
    return ret;
  }

  template<class RealT>
  void TriMesh<RealT>::Limits(Vector<RealT,3> &min,Vector<RealT,3> &max) const
  {
    auto it = vertices.begin();
    const auto end = vertices.end();
    if(it == end)
      return ; // Empty mesh!
    min = it->Position();
    max = it->Position();
    for(it++;it != end;++it) {
      for(unsigned i = 0;i < 3;i++) {
        if(min[i] > it->Position()[i])
          min[i] = it->Position()[i];
        if(max[i] < it->Position()[i])
          max[i] = it->Position()[i];
      }
    }
  }

  template<class RealT>
  void TriMesh<RealT>::OffsetScale(const Vector<RealT,3> &off,RealT scale) {
    for(auto &it : vertices) {
      it.Position() = (it.Position() + off) * scale;
    }
  }

  template<class RealT>
  void TriMesh<RealT>::UpdateVertexNormals() {
    Vector<RealT,3> zero  {0,0,0};
    for(auto it : vertices) {
      it.Normal() = zero;
    }
    for(auto &it : faces) {
      it.UpdateFaceNormal();
      Vector<RealT,3> norm = it.FaceNormal();
      for(unsigned i = 0;i < 3;i++) {
        it.Normal(i) += norm;
      }
    }
    for(auto it : vertices) {
      it.makeUnitNormal();
    }
  }



  template<class RealT>
  TriMesh<RealT> TriMesh<RealT>::operator+ (const TriMesh<RealT> &t2) const {
    std::vector<Vertex<RealT>> verts;
    verts.reserve(Vertices().size()+t2.Vertices().size());

    // put this in first
    for(auto it : Vertices()) {
      verts.push_back(it);
    }
    for(auto it : t2.Vertices()) {
      verts.push_back(it);
    }

    std::vector<Tri<RealT>> newFaces;
    newFaces.reserve(Faces().size()+t2.Faces().size());

    const Vertex<RealT> *Lv0 = &(Vertices()[0]);

    for(auto it : Faces()) {
      unsigned i0=unsigned(it.VertexPtr(0)-Lv0);
      unsigned i1=unsigned(it.VertexPtr(1)-Lv0);
      unsigned i2=unsigned(it.VertexPtr(2)-Lv0);

      newFaces.push_back(Tri<RealT>(verts[i0],verts[i1],verts[i2]));
      newFaces.back().FaceNormal()=it.FaceNormal();
    }

    unsigned nVerts = unsigned(verts.size());
    for(auto it : t2.Faces()) {
      unsigned i0=unsigned(it.VertexPtr(0)-Lv0+nVerts);
      unsigned i1=unsigned(it.VertexPtr(1)-Lv0+nVerts);
      unsigned i2=unsigned(it.VertexPtr(2)-Lv0+nVerts);

      newFaces.push_back(Tri<RealT>(verts[i0],verts[i1],verts[i2]));
      newFaces.back().FaceNormal()=it.FaceNormal();
    }

    return TriMesh<RealT>(std::move(verts),std::move(newFaces));
  }



  template<class RealT>
  bool TriMesh<RealT>::GenerateTextureCoords()
  {
    // Check that we have a valid mesh
    auto iNumFaces = faces.size();
    if (iNumFaces==0) return false;

    // Work out how many squares we need to take all our triangles
    auto iNumSquares = iNumFaces / 2;
    if ( (iNumFaces%2) ) iNumSquares++;
    // Work out how many squares to a side of the texture (rounded up square root)
    int iDim = int(ceil(std::sqrt(iNumSquares)));
    RealT dSize = RealT(1.0)/RealT(iDim);
    // Generate texture coordinates for each triangle.
    auto itFaces = faces.begin();
    const auto endFaces = faces.end();
    int x,y;
    for (x=0; (x<iDim && (itFaces!=endFaces)); ++x) {
      RealT dXBase = RealT(x)*dSize;
      for (y=0; (y<iDim && (itFaces!=endFaces));++y) {
        RealT dYBase = RealT(y)*dSize;
        // Generate texture coordinates for the triangle in the top left corner of the square
        Tri<RealT>& faceTL = *itFaces;
        faceTL.TextureID() = 0;
        faceTL.TextureCoord(0) = toVector<RealT>(dXBase + dSize*RealT(0.05), dYBase + dSize*RealT(0.90));
        faceTL.TextureCoord(1) = toVector<RealT>(dXBase + dSize*RealT(0.05), dYBase + dSize*RealT(0.05));
        faceTL.TextureCoord(2) = toVector<RealT>(dXBase + dSize*RealT(0.90), dYBase + dSize*RealT(0.05));
        ++itFaces;
        if (itFaces != endFaces) {
          // Generate texture coordinates for the triangle in the bottom right corner of the square
          Tri<RealT>& faceBR = *itFaces;
          faceBR.TextureID() = 0;
          faceBR.TextureCoord(0) = toVector<RealT>(dXBase + dSize*RealT(0.95), dYBase + dSize*RealT(0.10));
          faceBR.TextureCoord(1) = toVector<RealT>(dXBase + dSize*RealT(0.95), dYBase + dSize*RealT(0.95));
          faceBR.TextureCoord(2) = toVector<RealT>(dXBase + dSize*RealT(0.10), dYBase + dSize*RealT(0.95));
          ++itFaces;
        }
      }
    }

    haveTexture = true;

    return true;
  }



  template<class RealT>
  std::ostream &operator<<(std::ostream &s,const TriMesh<RealT> &ts) {
    RavlAssert(ts.IsValid());
    s << ts.Vertices();
    s << int(ts.HaveTextureCoord()) << '\n';
    s << ts.Faces().size() << '\n';
    const Vertex<RealT> *x = &(ts.Vertices()[0]);
    for(auto it : ts.Faces()) {
      s << (it.VertexPtr(0) - x) << ' '
        << (it.VertexPtr(1) - x) << ' '
        << (it.VertexPtr(2) - x) << ' ';
      s << it.TextureID() << ' ';
      s << it.TextureCoords() << ' ';
      s << it.Colour() << '\n';
    }
    return s;
  }

  template<class RealT>
  std::istream &operator>>(std::istream &s,TriMesh<RealT> &ts) {

    std::vector<Vertex<RealT>> verts;
    s >> verts;
    int iHaveTexture;
    s >> iHaveTexture;
    bool bHaveTexture = (iHaveTexture) ? true : false;
    unsigned nfaces,i1,i2,i3;
    s >> nfaces;
    std::vector<Tri<RealT>> faces(nfaces);
    for(auto it : faces) {
      s >> i1 >> i2 >> i3;
      s >> it.TextureID();
      s >> it.TextureCoords();
      s >> it.Colour();
      it.VertexPtr(0) = &(verts[i1]);
      it.VertexPtr(1) = &(verts[i2]);
      it.VertexPtr(2) = &(verts[i3]);
      it.UpdateFaceNormal();
    }
    ts = TriMesh<RealT>(std::move(verts),std::move(faces),bHaveTexture);
    return s;
  }

  extern template class TriMesh<float>;

}

