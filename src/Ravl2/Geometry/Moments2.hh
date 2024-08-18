// This file is part of RAVL, Recognition And Vision Library
// Copyright (C) 2001, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here
// Based on code by Radek Marik

#pragma once

#include <stdexcept>
#include "Ravl2/Math.hh"
#include "Ravl2/Index.hh"
#include "Ravl2/Geometry/Geometry.hh"

namespace Ravl2
{

  //! @brief The moments up to 2nd order in 2D space

  template <class SumT>
  class Moments2
  {
  public:
    //! Default constructor
    // Creates the moment object with all moments set to be zero.
    constexpr Moments2() = default;

    //! Convert from another set of moments with different SumT type.
    template<typename Sum2T>
    constexpr explicit Moments2(const Moments2<Sum2T> &mom)
      : m00(SumT(mom.M00())), m10(SumT(mom.M10())), m01(SumT(mom.M01())),
        m20(SumT(mom.M20())), m11(SumT(mom.M11())), m02(SumT(mom.M02()))
    {}

    //! Constructor from a set of values.
    constexpr Moments2(SumT nm00, SumT nm10, SumT nm01, SumT nm20, SumT nm11, SumT nm02)
        : m00(nm00), m10(nm10), m01(nm01), m20(nm20), m11(nm11), m02(nm02)
    {}

    //! reset all counters to zero.
    constexpr void reset()
    {
      m00 = m10 = m01 = m20 = m11 = m02 = SumT(0);
    }

    //! Adds a pixel to the object and updates sums.
    constexpr inline void addPixel(const Index<2> &pxl);

    //! Adds a position with a weight to the object and updates sums.
    constexpr inline void addPixel(const Point<SumT, 2> &pxl);

    //! Adds a position with a weight to the object and updates sums.
    constexpr inline void addPixel(const Point<SumT, 2> &pxl, SumT weight);

    //! Add pixel to set.
    constexpr const Moments2 &operator+=(const Index<2> &pxl)
    {
      addPixel(pxl);
      return *this;
    }

    //! Add pixel to set.
    constexpr const Moments2 &operator+=(const Point<SumT, 2> &point)
    {
      addPixel(point);
      return *this;
    }

    //! Calculate the size of the principle axis.
    // It returns the new values for M02 and M20,
    // the largest is the first element of the vector.
    [[nodiscard]] constexpr Vector<SumT, 2> principalAxisSize() const;

    //! Returns the ratio of the difference and the sum of moments m02 and m20.
    //! The value 0 means that objects is a symmetrical object,
    //! the value 1 corresponds to a one-dimensional object.
    [[nodiscard]] constexpr static SumT elongatedness(const Vector<SumT, 2> &principalAxisSize)
    {
      SumT sumM = principalAxisSize[0] + principalAxisSize[1];
      return (sumM != 0) ? std::abs((principalAxisSize[0] - principalAxisSize[1]) / sumM) : 0;
    }

    //! Access 00 component.
    [[nodiscard]] constexpr inline SumT M00() const
    {
      return m00;
    }

    //! Access 10 component.
    [[nodiscard]] constexpr inline SumT M10() const
    {
      return m10;
    }

    //! Access 01 component.
    [[nodiscard]] constexpr inline SumT M01() const
    {
      return m01;
    }

    //! Access 20 component.
    [[nodiscard]] constexpr inline SumT M20() const
    {
      return m20;
    }

    //! Access 11 component.
    [[nodiscard]] constexpr inline SumT M11() const
    {
      return m11;
    }

    //! Access 02 component.
    [[nodiscard]] constexpr inline SumT M02() const
    {
      return m02;
    }

    //! Access 00 component.
    [[nodiscard]] constexpr inline SumT &M00()
    {
      return m00;
    }

    //! Access 10 component.
    [[nodiscard]] constexpr inline SumT &M10()
    {
      return m10;
    }

    //! Access 01 component.
    [[nodiscard]] constexpr inline SumT &M01()
    {
      return m01;
    }

    //! Access 20 component.
    [[nodiscard]] constexpr inline SumT &M20()
    {
      return m20;
    }

    //! Access 11 component.
    [[nodiscard]] constexpr inline SumT &M11()
    {
      return m11;
    }

    //! Access 02 component.
    [[nodiscard]] constexpr inline SumT &M02()
    {
      return m02;
    }

    //! Returns the moment m00, ie the area of the 2 dimensional object.
    [[nodiscard]] constexpr inline SumT area() const
    {
      return m00;
    }

    //! Returns the x co-ordinate of the centroid.
    //! The M00 moment must not be 0.
    [[nodiscard]] constexpr inline SumT centroidX() const
    {
      return M10() / M00();
    }

    //! Returns the y co-ordinate of the centroid.
    //! The M00 moment must not be 0.
    [[nodiscard]] constexpr inline SumT centroidY() const
    {
      return M01() / M00();
    }

    //! Returns the variance of the x.
    [[nodiscard]] constexpr inline SumT varX() const
    {
      return m20 / m00 - sqr(centroidX());
    }

    //! Returns the variance of the y.
    [[nodiscard]] constexpr inline SumT varY() const
    {
      return m02 / m00 - sqr(centroidY());
    }

    //! Returns the slope dY/dX. The used criterion is Sum(Y-y)^2 -> min.
    // It means dY/dX = k, where y = k*x+q.
    [[nodiscard]] constexpr inline SumT slopeY() const;

    //! Returns the slope dX/dY. The used criterion is Sum(X-x)^2 -> min.
    // It means dX/dY = k, where x = k*y+q.
    [[nodiscard]] constexpr inline SumT slopeX() const;

    //! Returns the estimate of q, if y = k*x+q.
    // The used criterion is Sum(Y-y)^2 -> min.
    [[nodiscard]] constexpr inline SumT interceptY() const;

    //! Returns the estimate of q, if y = k*y+q.
    // The used criterion is Sum(X-x)^2 -> min.
    [[nodiscard]] constexpr inline SumT interceptX() const;

    //! Return the covariance matrix.
    [[nodiscard]] constexpr Matrix<SumT, 2, 2> covariance() const;

    //! Calculate the centroid.
    template<unsigned Axis>
      requires (Axis < 2)
    [[nodiscard]] constexpr SumT centroid() const
    {
      if constexpr(Axis == 0) {
        return M10() / M00();
      } else {
        return M01() / M00();
      }
    }

    //! Calculate the variance
    template<unsigned Axis>
      requires (Axis < 2)
    [[nodiscard]] constexpr SumT var() const
    {
      if constexpr(Axis == 0) {
        return m20 / m00 - sqr(centroid<0>());
      } else {
        return m02 / m00 - sqr(centroid<1>());
      }
    }

    //! Calculate the centroid.
    [[nodiscard]] constexpr Point<SumT, 2> centroid() const
    {
      return Point<SumT, 2>({centroid<0>(), centroid<1>()});
    }

    //! Add to sets of moments together.
    [[nodiscard]] constexpr Moments2 operator+(const Moments2 &m) const
    {
      return Moments2(m00 + m.M00(), m10 + m.M10(), m01 + m.M01(), m20 + m.M20(), m11 + m.M11(), m02 + m.M02());
    }

    //! Subtract one set of moments from another.
    [[nodiscard]] constexpr Moments2 operator-(const Moments2 &m) const
    {
      return Moments2(m00 - m.M00(), m10 - m.M10(), m01 - m.M01(), m20 - m.M20(), m11 - m.M11(), m02 - m.M02());
    }

    //! Add to sets of moments to this one.
    constexpr const Moments2 &operator+=(const Moments2 &m);

    //! Subtract a set of moments to this one.
    constexpr const Moments2 &operator-=(const Moments2 &m);

    //! Swap X and Y co-ordinates.
    constexpr void swapXY()
    {
      std::swap(m10, m01);
      std::swap(m20, m02);
    }

    //! Shift the moments by a vector.
    constexpr void shift(const Point<SumT, 2> &shift)
    {
      m20 += 2 * m10 * shift[0] + m00 * sqr(shift[0]);
      m02 += 2 * m01 * shift[1] + m00 * sqr(shift[1]);
      m11 += m10 * shift[1] + m01 * shift[0] + m00 * shift[0] * shift[1];
      m10 += m00 * shift[0];
      m01 += m00 * shift[1];
    }

    //! Shift the moments by an index.
    constexpr inline void shift(const Index<2> &shift)
    {
      this->shift(toPoint<SumT>(shift));
    }

    //! Serialization support
    template <class Archive>
    void serialize(Archive &ar)
    {
      ar(cereal::make_nvp("m00", m00),
         cereal::make_nvp("m10", m10),
         cereal::make_nvp("m01", m01),
         cereal::make_nvp("m20", m20),
         cereal::make_nvp("m11", m11),
         cereal::make_nvp("m02", m02));
    }

  private:
    SumT m00 = 0;
    SumT m10 = 0;
    SumT m01 = 0;
    SumT m20 = 0;
    SumT m11 = 0;
    SumT m02 = 0;
  };

  template <class SumT>
  std::ostream &
  operator<<(std::ostream &os, const Moments2<SumT> &mom);

  template <class SumT>
  std::istream &
  operator>>(std::istream &is, Moments2<SumT> &mom);

  //: Return the covariance matrix.

  template <class SumT>
  constexpr Matrix<SumT, 2, 2> Moments2<SumT>::covariance() const
  {
    if(isNearZero(m00))
      return Matrix<SumT, 2, 2>({{1, 0}, {0, 1}});// if m00 is too small (prevent effective division by zero)
    SumT cent1 = centroid<0>();
    SumT cent2 = centroid<1>();
    SumT diag = m11 / m00 - cent1 * cent2;
    return Matrix<SumT, 2, 2>({{m20 / m00 - sqr(cent1), diag},
                                {diag, m02 / m00 - sqr(cent2)}});
  }

  template <class SumT>
  constexpr Vector<SumT, 2> Moments2<SumT>::principalAxisSize() const
  {
    Matrix<SumT, 2, 2> mat = covariance();
    auto vec = xt::linalg::eigvalsh(mat);
    if(vec[0] < vec[1])
      std::swap(vec[0], vec[1]);
    return vec;
  }

  template <class SumT>
  std::ostream &operator<<(std::ostream &os, const Moments2<SumT> &mom)
  {
    os << mom.M00() << ' ' << mom.M10() << ' ' << mom.M01() << ' '
       << mom.M20() << ' ' << mom.M11() << ' ' << mom.M02();
    return os;
  }

  template <class SumT>
  std::istream &operator>>(std::istream &is, Moments2<SumT> &mom)
  {
    is >> mom.m00 >> mom.m10 >> mom.m01
      >> mom.m20 >> mom.m11 >> mom.m02;
    return is;
  }

  //! @brief Equality operator
  //! This makes sense if SumT is integer type, the comparison is exact.
  template<typename SumT>
  [[nodiscard]] constexpr bool operator==(const Moments2<SumT> &lhs, const Moments2<SumT> &rhs)
  {
    return lhs.M00() == rhs.M00() && lhs.M10() == rhs.M10() && lhs.M01() == rhs.M01() &&
           lhs.M20() == rhs.M20() && lhs.M11() == rhs.M11() && lhs.M02() == rhs.M02();
  }

  //! In equality operator
  //! This makes sense if SumT is integer type, the comparison is exact.
  template<typename SumT>
  [[nodiscard]] constexpr inline bool operator!=(const Moments2<SumT> &lhs, const Moments2<SumT> &rhs)
  {
    return !(lhs == rhs);
  }


  template <class SumT>
  constexpr inline void Moments2<SumT>::addPixel(const Index<2> &pxl)
  {
    auto a = SumT(pxl[0]);
    auto b = SumT(pxl[1]);

    m00++;
    m01 += b;
    m10 += a;
    m11 += a * b;
    m02 += b * b;
    m20 += a * a;
  }

  template <class SumT>
  constexpr inline void Moments2<SumT>::addPixel(const Point<SumT, 2> &pxl)
  {
    SumT a = pxl[0];
    SumT b = pxl[1];

    m00++;
    m01 += b;
    m10 += a;
    m11 += a * b;
    m02 += b * b;
    m20 += a * a;
  }

  template <class SumT>
  constexpr void Moments2<SumT>::addPixel(const Point<SumT, 2> &pxl, SumT weight)
  {
    SumT a = pxl[0];
    SumT b = pxl[1];

    m00 += weight;
    SumT wa = a * weight;
    SumT wb = b * weight;
    m01 += wb;
    m10 += wa;
    m11 += a * wb;
    m02 += b * wb;
    m20 += a * wa;
  }

  template <class SumT>
  constexpr inline const Moments2<SumT> &Moments2<SumT>::operator+=(const Moments2<SumT> &m)
  {
    m00 += m.M00();
    m10 += m.M10();
    m01 += m.M01();
    m20 += m.M20();
    m11 += m.M11();
    m02 += m.M02();
    return *this;
  }

  template <class SumT>
  constexpr inline const Moments2<SumT> &Moments2<SumT>::operator-=(const Moments2<SumT> &m)
  {
    m00 -= m.M00();
    m10 -= m.M10();
    m01 -= m.M01();
    m20 -= m.M20();
    m11 -= m.M11();
    m02 -= m.M02();
    return *this;
  }

  template <class SumT>
  constexpr inline SumT Moments2<SumT>::slopeY() const
  {
    SumT det = m00 * m20 - m10 * m10;
    if(isNearZero(det))
      throw std::underflow_error("Moments2<SumT>::slopeY(), Determinant near zero. ");
    return (m00 * m11 - m10 * m01) / det;
  }

  template <class SumT>
  constexpr inline SumT Moments2<SumT>::interceptY() const
  {
    SumT det = m00 * m20 - m10 * m10;
    if(isNearZero(det))
      throw std::underflow_error("Moments2::interceptY(), Determinant near zero. ");
    return (m20 * m01 - m10 * m11) / det;
  }

  template <class SumT>
  constexpr inline SumT Moments2<SumT>::slopeX() const
  {
    SumT det = m00 * m02 - m01 * m01;
    if(isNearZero(det))
      throw std::underflow_error("Moments2::slopeX(), Determinant near zero. ");
    return (m00 * m11 - m01 * m10) / det;
  }

  template <class SumT>
  constexpr inline SumT Moments2<SumT>::interceptX() const
  {
    SumT det = m00 * m02 - m01 * m01;
    if(isNearZero(det))
      throw std::underflow_error("Moments2::interceptX(), Determinant near zero. ");
    return (m02 * m10 - m01 * m11) / det;
  }

  extern template class Moments2<float>;
  extern template class Moments2<double>;

}// namespace Ravl2

#if FMT_VERSION >= 90000
template <typename SumT>
struct fmt::formatter<Ravl2::Moments2<SumT> > : fmt::ostream_formatter {
};
#endif

