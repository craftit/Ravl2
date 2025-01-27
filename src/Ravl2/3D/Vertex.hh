// This file is part of RAVL, Recognition And Vision Library 
// Copyright (C) 2002, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here
//! docentry="Ravl.API.3D.Mesh"
//! author="Charles Galambos"

#include "Ravl2/Types.hh"

namespace Ravl2
{

  //! Vertex in a mesh.
  // This holds a position and a normal.

  template<class RealT>
  class Vertex
  {
  public:
    //! Default constructor.
    Vertex() = default;

    //! Constructor from position and normal
    //! @param  pos - position of vertex.
    //! @param  norm - Direction of surface normal at vertex.
    Vertex(const Point<RealT,3> &pos,const Vector<RealT,3> &norm)
      : at(pos),
	mNormal(norm)
    {}
    
    //! Constructor from position
    //! @param  pos - position of vertex.
    // The surface normal is left undefined.
    explicit Vertex(const Point<RealT,3> &pos)
      : at(pos)
    {}
    
    //! Access position of vertex.
    [[nodiscard]] Vector<RealT,3> &position()
    { return at; }
    
    //! Access position of vertex.
    [[nodiscard]] const Vector<RealT,3> &position() const
    { return at; }
    
    //! Access normal at vertex.
    [[nodiscard]] Vector<RealT,3> &normal()
    { return mNormal; }

    //! Access normal at vertex.
    [[nodiscard]] const Vector<RealT,3> &normal() const
    { return mNormal; }

    //! Make unit normal.
    void makeUnitNormal()
    {
      RealT normLen = RealT(mNormal.norm());
      if(normLen > RealT(0.0)) {
        mNormal /= normLen;
      }
    }

    //! Serialization support
    template <class Archive>
    constexpr void serialize(Archive &archive)
    {
      archive(cereal::make_nvp("at", at));
      archive(cereal::make_nvp("normal", mNormal));
    }

  private:
    Vector<RealT,3> at;        // Position of vertex.
    Vector<RealT,3> mNormal;    // Normal to vertex.
  };

  extern template class Vertex<float>;

  template<class RealT>
  std::ostream &operator<<(std::ostream &s,const Vertex<RealT> &v) {
    s << v.position() << ' ' << v.Normal();
    return s;
  }

  template<class RealT>
  std::istream &operator>>(std::istream &s, Vertex<RealT> &v) {
    s >> v.position() >> v.Normal();
    return s;
  }

}

