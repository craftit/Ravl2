// This file is part of RAVL, Recognition And Vision Library
// Copyright (C) 2002, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here
//! author="Charles Galambos"
//! date="18/12/1995"

#include "Ravl2/Index.hh"
#include "Ravl2/Geometry/Geometry.hh"
#include "Ravl2/Math.hh"

namespace Ravl2
{

  //: Corner descriptor.

  class CornerC
  {
  public:
    using RealT = float;

    CornerC()
    {}
    //: Default constructor.
    // Contents of class are undefined.

    CornerC(const Point<float, 2> &location, RealT ndV, RealT ndH, uint8_t nlevel)
        : loc(location),
          grad({ndV, ndH}),
          level(nlevel)
    {}
    //: Constructor.

    CornerC(const Point<float, 2> &location, const Vector<float, 2> &ngrad, uint8_t nlevel)
        : loc(location),
          grad(ngrad),
          level(nlevel)
    {}
    //: Constructor.

    Point<float, 2> &location()
    {
      return loc;
    }
    //: Get location of corner.

    const Point<float, 2> &location() const
    {
      return loc;
    }
    //: Get location of corner.

    Vector<float, 2> &gradient()
    {
      return grad;
    }
    // Get gradient.

    const Vector<float, 2> &gradient() const
    {
      return grad;
    }
    // Get gradient.

    RealT &DVert() { return grad[0]; }
    // Vertical component of gradient.

    RealT &DHor() { return grad[1]; }
    // Horizontal component of gradient.

    uint8_t &Level() { return level; }
    // Grey level of pixel.

    const uint8_t &Level() const
    {
      return level;
    }
    // Grey level of pixel.

    auto Distance(const Point<float, 2> &oth) const
    {
      return xt::sum(xt::abs(loc - oth));
      //return cityBlockDistance(loc,loc);
    }
    //: City block distance from another pixel.

    inline RealT Distance(const CornerC &Oth) const;
    // A somewhat arbitrary distance measure between two corners.
    // Suggestions for a better measure are welcome.

  private:
    Point<float, 2> loc;  // Location of corner.
    Vector<float, 2> grad;// gradient of point.
    uint8_t level;        // Intensity of point.
  };

  std::ostream &operator<<(std::ostream &out, const CornerC &corn);
  //: Write corner to a stream.

  std::istream &operator>>(std::istream &in, CornerC &corn);
  //: Read corner from a stream.

  //////////////////////////////////////
  // A somewhat arbitrary distance measure between two corners.

  inline float CornerC::Distance(const CornerC &oth) const
  {
    return xt::sum(xt::abs(loc - oth.loc))() + xt::sum(xt::abs(grad - oth.grad))() + std::abs(RealT(level) - RealT(oth.level));
  }

}// namespace Ravl2
