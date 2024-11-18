// This file is part of RAVL, Recognition And Vision Library 
// Copyright (C) 2002, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here
//! docentry="Ravl.API.3D.Mesh"
//! author="Jonathan Starck"

#pragma once

#include <vector>
#include "Ravl2/3D/TriMesh.hh"
#include "Ravl2/Array.hh"
#include "Ravl2/Pixel/Pixel.hh"

namespace Ravl2 {
  
  //: Textured tri mesh body.
  // Adds texture to a TriMeshC

  template<class RealT,class PixelT = PixelRGB8>
  class TexTriMesh
    : public TriMesh<RealT>
  {
  public:
    //: Default constructor
    TexTriMesh() = default;

    //: Copy constructor
    TexTriMesh(const TexTriMesh& oth);

    //: Construct from an array of vertices and an array of indices.
    // Successive triples are taken from faceIndices to form the faces in the mesh.
    // A texture map and texture coordinates are generated for the mesh.
    TexTriMesh(const std::vector<Vector<RealT,3>> &newVertices,
		    const std::vector<unsigned> &faceIndices)
      : TriMesh<RealT>(newVertices,faceIndices)
    { GenerateTextureMap(); }

    //: Construct from an array of vertices and an array of tri's.
    // The Tri<RealT>'s must refer to elements in 'v'.
    // A texture map and texture coordinates are generated for the mesh.
    TexTriMesh(std::vector<Vertex<RealT>> &&newVertices,
		std::vector<Tri<RealT>> &&newFaces,
                    bool generateTexCoords = true)
      : TriMesh<RealT>(std::move(newVertices),std::move(newFaces),false)
    { if(generateTexCoords) GenerateTextureMap(); }

    //: Construct from an array of vertices, an array of tri's and the texture images.
    // The Tri<RealT>'s must refer to elements in 'v' and must have texture coordinates.
    TexTriMesh(std::vector<Vertex<RealT>> &&newVertices,
		std::vector<Tri<RealT>> &&newFaces,
		    const std::vector< Array<PixelT,2> >& textures,
		    const std::vector< std::string >& texFilenames = std::vector< std::string >())
      : TriMesh<RealT>(std::move(newVertices),std::move(newFaces),true),
	m_textures(textures),
	m_strFilenames(texFilenames)
    {}


  public:
    [[nodiscard]] std::vector< Array<PixelRGB8,2> >& Textures()
    { return m_textures; }
    //: Access the textures.

    [[nodiscard]] const std::vector< Array<PixelRGB8,2> >& Textures() const
    { return m_textures; }
    //: Access the textures.

    [[nodiscard]] auto NumTextures() const
    { return m_textures.size(); }
    //: Return the number of textures

    [[nodiscard]] std::vector< std::string >& TexFilenames()
    { return m_strFilenames; }
    //: Access the textures.

    [[nodiscard]] const std::vector< std::string >& TexFilenames() const
    { return m_strFilenames; }
    //: Access the textures.
    
    bool GenerateTextureMap(void);
    //: Automatically generate texture coordinates and a texture image

    //! Serialization support
    template <class Archive>
    void serialize(Archive &archive)
    {
      archive(cereal::base_class<TriMesh<RealT>>(this),
              CEREAL_NVP(m_textures),
              CEREAL_NVP(m_strFilenames));
    }
    
  protected:
    // The texture images and associated filenames
    std::vector< Array<PixelT ,2> > m_textures;
    std::vector< std::string > m_strFilenames;
  };

  template<class RealT,class PixelT>
  TexTriMesh<RealT,PixelT>::TexTriMesh(const TexTriMesh<RealT,PixelT>& oth)
   : TriMesh<RealT>(oth)
  {
    // Copy the textures
    m_strFilenames = oth.m_strFilenames;
    m_textures.reserve(oth.m_textures.size());
    for(auto &tex : oth.m_textures) {
      m_textures.push_back(clone(tex));
    }
  }

  template<class RealT,class PixelT>
  bool TexTriMesh<RealT,PixelT>::GenerateTextureMap(void)
  {
    // Create coordinates.
    this->GenerateTextureCoords();

    // Create the texture image
    m_textures.push_back(Array<PixelT,2>({1024,1024}));
    m_strFilenames.emplace_back("texture.ppm");

    // Done
    return true;
  }

  extern template class TexTriMesh<float,PixelRGB8>;

}

