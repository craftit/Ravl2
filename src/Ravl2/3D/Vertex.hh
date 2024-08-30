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

  //: Vertex in a mesh.
  // This holds a position and a normal.

  template<class RealT>
  class Vertex
  {
  public:
    //! Default constructor.
    Vertex() = default;

    Vertex(const Point<RealT,3> &pos,const Vector<RealT,3> &norm)
      : at(pos),
	normal(norm)
    {}
    //: Constructor from position and normal
    //! @param  pos - Position of vertex.
    //! @param  norm - Direction of surface normal at vertex.
    
    explicit Vertex(const Point<RealT,3> &pos)
      : at(pos)
    {}
    //: Constructor from position
    //! @param  pos - Position of vertex.
    // The surface normal is left undefined.
    
    [[nodiscard]] Vector<RealT,3> &Position()
    { return at; }
    //: Access position of vertex.
    
    [[nodiscard]] const Vector<RealT,3> &Position() const
    { return at; }
    //: Access position of vertex.
    
    [[nodiscard]] Vector<RealT,3> &Normal()
    { return normal; }
    //: Access normal at vertex.

    [[nodiscard]] const Vector<RealT,3> &Normal() const
    { return normal; }
    //: Access normal at vertex.

    //! Make unit normal.
    void makeUnitNormal()
    {
      RealT normLen = RealT(xt::norm_l2(normal)());
      if(normLen > RealT(0.0)) {
        normal /= normLen;
      }
    }

    //! Serialization support
    template <class Archive>
    constexpr void serialize(Archive &archive)
    {
      archive(cereal::make_nvp("at", at));
      archive(cereal::make_nvp("normal", normal));
    }

  protected:
    Vector<RealT,3> at;        // Position of vertex.
    Vector<RealT,3> normal;    // Normal to vertex.
  };

  extern template class Vertex<float>;

  template<class RealT>
  std::ostream &operator<<(std::ostream &s,const Vertex<RealT> &v) {
    s << v.Position() << ' ' << v.Normal();
    return s;
  }

  template<class RealT>
  std::istream &operator>>(std::istream &s, Vertex<RealT> &v) {
    s >> v.Position() >> v.Normal();
    return s;
  }

}

