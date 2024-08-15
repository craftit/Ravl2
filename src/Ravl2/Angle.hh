// This file is part of RAVL, Recognition And Vision Library 
// Copyright (C) 2001, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here
////////////////////////////////////////////////////////
//! file="Ravl/Core/Math/Angle.hh"
//! lib=RavlCore
//! userlevel=Normal
//! author="Charles Galambos"
//! docentry="Ravl.API.Math"
//! rcsid="$Id$"
//! date="09/02/1998"

#include <numbers>
#include "Ravl2/Types.hh"

namespace Ravl2
{
  //! @brief This class is designed to deal with angles in radians.
  //! It keeps the values normalised and provides arithmetic operations. <p>
  //! N is the number of pi's to wrap around at.
  //!  for directed lines this should be 2 (*pi).
  //!  for undirected lines is should be 1 (*pi).

  template<class RealT, unsigned N = 2>
   requires std::is_arithmetic_v<RealT>
  class AngleC {
  public:
    //: Construct from value in radians.
    inline explicit AngleC(RealT anglerad = 0)
      : angle(anglerad)
    { Normalise(); }


    //: Normalise the angle to values between 0 and max.
    inline void Normalise()
    { angle -= std::floor(angle/m_max) * m_max; }

    //: Normalise the angle to values between 0 and max.
    // Returns the normalised angle.
    [[nodiscard]] inline RealT Normalise(RealT value) const
    { return value - std::floor(value/m_max) * m_max; }

    //: Subtract angles.
    inline AngleC operator- (const AngleC &val) const
    { return AngleC(angle - val.angle); }

    //: Add angles.
    inline AngleC operator+ (const AngleC &val) const
    { return AngleC(angle + val.angle); }

    //: Subtract angles.
    inline const AngleC &operator-= (const AngleC &val) {
      angle -= val.angle;
      Normalise();
      return *this;
    }

    //: Add angles.
    inline const AngleC &operator+= (const AngleC &val)  {
      angle += val.angle;
      Normalise();
      return *this;
    }

    //: Test if this angle lies between angle1 and angle2.
    [[nodiscard]] inline bool IsBetween(RealT angle1,RealT angle2) const {
      RealT diff1 = Normalise(angle2  - angle1);
      RealT diff2 = Normalise(Value() - angle1);
      return (diff1 < diff2);
    }

    //: Test if this angle lies between angle1 and angle2.
    [[nodiscard]] inline bool IsBetween(const AngleC &angle1,const AngleC &angle2) const
    { return IsBetween(angle1.Value(),angle2.Value()); }

    //: Find the difference between two angles.
    // it returns values in the rangle +/- max/2.
    [[nodiscard]] inline RealT Diff(const AngleC<RealT,N> &val) const
    {
      RealT ret = angle - val.angle;
      RealT maxb2 = m_max / 2;
      if(ret > maxb2)
        ret -= m_max;
      else {
        if(ret < -maxb2)
          ret += m_max;
      }
      return ret;
    }

    //: Get maximum angle.
    [[nodiscard]] inline RealT MaxAngle() const { return m_max; };

    //: Get value of angle.
    [[nodiscard]] inline RealT Value() const { return angle; }

    //: Get sin of angle.
    [[nodiscard]] inline RealT Sin() const { return std::sin(angle); }

    //: Get cos of angle.
    [[nodiscard]] inline RealT Cos() const { return std::cos(angle); }

    //: Dump to stream.
    void Dump(std::ostream &out) const
    { out << "Ang:" << angle << " Max:" << m_max; }
  protected:
    RealT angle;
    static constexpr RealT m_max = std::numbers::pi_v<RealT> * RealT(N);
  };

  template<class RealT, unsigned N>
  inline std::ostream &operator<<(std::ostream &out, const AngleC<RealT,N> &angle)
  {
    angle.Dump(out);
    return out;
  }

  extern template class AngleC<float>;
}

namespace fmt
{
  template<typename RealT, RealT m_max >
  struct formatter<Ravl2::AngleC<RealT,m_max>> : ostream_formatter {
  };
}// namespace fmt

