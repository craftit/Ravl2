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

  class Corner
  {
  public:
    using RealT = float;

    //! @brief Default constructor.
    //! Contents of class are undefined.
    Corner() = default;

    //! Constructor.
    Corner(const Point<float, 2> &location, RealT ndV, RealT ndH, uint8_t nlevel)
        : mLocation(location),
	  mGrad({ndV, ndH}),
	  mLevel(nlevel)
    {}

    //! Constructor.
    Corner(const Point<float, 2> &location, const Vector<float, 2> &ngrad, uint8_t nlevel)
        : mLocation(location),
	  mGrad(ngrad),
	  mLevel(nlevel)
    {}

    //! Get location of corner.
    Point<float, 2> &location()
    {
      return mLocation;
    }

    //! Get location of corner.
    const Point<float, 2> &location() const
    {
      return mLocation;
    }

    //! Get gradient.
    Vector<float, 2> &gradient()
    {
      return mGrad;
    }

    //! Get gradient.
    const Vector<float, 2> &gradient() const
    {
      return mGrad;
    }

    //! Vertical component of gradient.
    RealT &dVert() { return mGrad[0]; }

    //! Horizontal component of gradient.
    RealT &dHor() { return mGrad[1]; }

    //! Grey level of pixel.
    uint8_t &level() { return mLevel; }

    //! Grey level of pixel.
    const uint8_t &level() const
    {
      return mLevel;
    }

    //! City block distance from another pixel.
    auto distance(const Point<float, 2> &oth) const
    {
      float d = xt::sum(xt::abs(mLocation - oth))();
      return d;
    }

    //! A somewhat arbitrary distance measure between two corners.
    inline RealT distance(const Corner &Oth) const;

  private:
    Point<float, 2> mLocation;  //!< Location of corner.
    Vector<float, 2> mGrad;//!< gradient of point.
    uint8_t mLevel;        //!< Intensity of point.
  };

  std::ostream &operator<<(std::ostream &out, const Corner &corn);
  //: Write corner to a stream.

  std::istream &operator>>(std::istream &in, Corner &corn);
  //: Read corner from a stream.

  //////////////////////////////////////
  // A somewhat arbitrary distance measure between two corners.

  inline float Corner::distance(const Corner &Oth) const
  {
    return xt::sum(xt::abs(mLocation - Oth.mLocation))() + xt::sum(xt::abs(mGrad - Oth.mGrad))() + std::abs(RealT(mLevel) - RealT(Oth.mLevel));
  }

}// namespace Ravl2
