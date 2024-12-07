// This file is part of RAVL, Recognition And Vision Library
// Copyright (C) 2001, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here

#pragma once

#include "Ravl2/Geometry/Geometry.hh"
#include "Ravl2/Geometry/Range.hh"

namespace Ravl2
{

  //! Pure translation

  template <typename DataT, unsigned N>
  class Translate
  {
  public:
    using value_type = DataT;
    constexpr static unsigned dimension = N;

    //! Construct no-change transform.
    //! Scale = 1, Translate = 0.
    inline constexpr Translate() = default;

    //! Copy constructor.
    inline constexpr Translate(const Translate &Oth) = default;


    //! Construct from scale and a translation vector.
    constexpr explicit Translate(const Vector<DataT, N> &translate)
      : mT(translate)
    {}

    //! Construct an identity transform.
    [[nodiscard]] static constexpr Translate identity()
    { return Translate(Vector<DataT, N>::Zero()); }

    //! Access the translation component of the transformation.
    [[nodiscard]]inline constexpr Vector<DataT, N> &translation() { return mT; }

    //! Constant access to the translation component of the transformation.
    [[nodiscard]] inline constexpr const Vector<DataT, N> &translation() const { return mT; }

    //! Add a translation in direction T.
    inline constexpr void translate(const Vector<DataT, N> &T) {
      mT += T;
    }

    //! Generate an inverse transformation.
    [[nodiscard]] constexpr Translate<DataT, N> inverse() const  {
      return Translate<DataT, N>(mT * -1);
    }

    //! Is the transform real.
    [[nodiscard]] constexpr bool isReal() const
    {
      for(auto x : mT) {
        if(std::isinf(x) || std::isnan(x))
          return false;
      }
      return true;
    }

    //! Transform point
    //! Take a point and put it though the transformation.
    [[nodiscard]] constexpr Point<DataT,N> operator()(const Point<DataT, N> &pnt) const
    {
      return pnt + mT;
    }

    //! Compose this transform with 'In'
    [[nodiscard]] constexpr inline auto operator()(const Translate &In) const
    {
      return Translate(In.translation() + mT);
    }

    //! Transform a range.
    [[nodiscard]] constexpr inline auto operator()(const Range<DataT,N> &In) const
    {
      return Range<DataT,N>(In.min() + mT, In.max() + mT);
    }

    //! Transform a point
    Vector<DataT, N> constexpr operator*(const Vector<DataT, N> &in) const
    {
      return (*this)(in);
    }

    //! Serialization support
    template <class Archive>
    void serialize(Archive &ar)
    {
      ar(cereal::make_nvp("T", mT));
    }

  protected:
    Vector<DataT, N> mT = Vector<DataT, N>::Zero();//!< Translate.
  };

  //! Generate an inverse transformation.
  template <typename DataT, unsigned N>
  [[nodiscard]] constexpr Translate<DataT, N> inverse(const Translate<DataT, N> &in)
  {
    return in.inverse();
  }

  //! Construct a translation transform.
  template <typename DataT, unsigned N>
  [[nodiscard]] constexpr Translate<DataT, N> translate(const Vector<DataT, N> &T)
  {
    return Translate<DataT, N>(T);
  }

  template <typename DataT, unsigned N>
  [[nodiscard]] constexpr Translate<DataT, N> operator*(const Translate<DataT, N> &st,const Translate<DataT, N> &in)
  {
    return st(in);
  }

  template <typename DataT, unsigned N>
  [[nodiscard]] constexpr Range<DataT, N> operator*(const Translate<DataT, N> &st, const Range<DataT, N> &in)
  {
    return st(in);
  }

  //! Fit a scale translate between ranges.
  template <typename DataT, unsigned N>
  bool fit(Translate<DataT,N> &transform,const Point<DataT,N> &rngTo,const Point<DataT,N> &from) {
    transform = Translate<DataT,N>(rngTo - from);
    return true;
  }

  //! Fit a scale to translate between index ranges.
  template <typename DataT, unsigned N>
  bool fit(Translate<DataT,N> &transform,const Index<N> &rngTo,const Index<N> &from) {
    transform = Translate<DataT,N>(toPoint(rngTo - from));
    return true;
  }

  //! Stream output.
  template <typename DataT, unsigned N>
  std::ostream &operator<<(std::ostream &os, const Translate<DataT, N> &In)
  {
    os << "Translate: " << In.translation();
    return os;
  }

  extern template class Translate<float, 2>;
  extern template class Translate<float, 3>;

}// namespace Ravl2

#if FMT_VERSION >= 90000
template <typename RealT, unsigned N>
struct fmt::formatter<Ravl2::Translate<RealT, N>> : fmt::ostream_formatter {
};
#endif
