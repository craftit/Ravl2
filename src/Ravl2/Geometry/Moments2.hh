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

  //: The moments up to 2nd order in 2D space

  template <class RealT>
  class Moments2
  {
  public:
    //! Default constructor
    // Creates the moment object with all moments set to be zero.
    Moments2() = default;

    //! Constructor from a set of values.
    Moments2(RealT nm00, RealT nm10, RealT nm01, RealT nm20, RealT nm11, RealT nm02)
        : m00(nm00), m10(nm10), m01(nm01), m20(nm20), m11(nm11), m02(nm02)
    {}

    //! reset all counters to zero.
    void reset()
    {
      m00 = m10 = m01 = m20 = m11 = m02 = 0.0;
    }

    //! Adds a pixel to the object and updates sums.
    inline void addPixel(const Index<2> &pxl);

    //! Adds a position with a weight to the object and updates sums.
    inline void addPixel(const Point<RealT, 2> &pxl);

    //! Adds a position with a weight to the object and updates sums.
    inline void addPixel(const Point<RealT, 2> &pxl, RealT weight);

    //! Add pixel to set.
    const Moments2 &operator+=(const Index<2> &pxl)
    {
      addPixel(pxl);
      return *this;
    }

    //! Add pixel to set.
    const Moments2 &operator+=(const Point<RealT, 2> &point)
    {
      addPixel(point);
      return *this;
    }

    //! Calculate the size of the principle axis.
    // It returns the new values for M02 and M20,
    // the largest is the first element of the vector.
    Vector<RealT, 2> principalAxisSize() const;

    //! Returns the ratio of the difference and the sum of moments m02 and m20.
    //! The value 0 means that objects is a symmetrical object,
    //! the value 1 corresponds to a one-dimensional object.
    static RealT elongatedness(const Vector<RealT, 2> &principalAxisSize)
    {
      RealT sumM = principalAxisSize[0] + principalAxisSize[1];
      return (sumM != 0) ? std::abs((principalAxisSize[0] - principalAxisSize[1]) / sumM) : 0;
    }

    //! Access 00 component.
    inline RealT M00() const
    {
      return m00;
    }

    //! Access 10 component.
    inline RealT M10() const
    {
      return m10;
    }

    //! Access 01 component.
    inline RealT M01() const
    {
      return m01;
    }

    //! Access 20 component.
    inline RealT M20() const
    {
      return m20;
    }

    //! Access 11 component.
    inline RealT M11() const
    {
      return m11;
    }

    //! Access 02 component.
    inline RealT M02() const
    {
      return m02;
    }

    //! Access 00 component.
    inline RealT &M00()
    {
      return m00;
    }

    //! Access 10 component.
    inline RealT &M10()
    {
      return m10;
    }

    //! Access 01 component.
    inline RealT &M01()
    {
      return m01;
    }

    //! Access 20 component.
    inline RealT &M20()
    {
      return m20;
    }

    //! Access 11 component.
    inline RealT &M11()
    {
      return m11;
    }

    //! Access 02 component.
    inline RealT &M02()
    {
      return m02;
    }

    //! Returns the moment m00, ie the area of the 2 dimensional object.
    inline RealT area() const
    {
      return m00;
    }

    //! Returns the x co-ordinate of the centroid.
    //! The M00 moment must not be 0.
    inline RealT centroidX() const
    {
      return M10() / M00();
    }

    //! Returns the y co-ordinate of the centroid.
    //! The M00 moment must not be 0.
    inline RealT centroidY() const
    {
      return M01() / M00();
    }

    //! Returns the variance of the x.
    inline RealT varX() const
    {
      return m20 / m00 - sqr(centroidX());
    }

    //! Returns the variance of the y.
    inline RealT varY() const
    {
      return m02 / m00 - sqr(centroidY());
    }

    //! Returns the slope dY/dX. The used criterion is Sum(Y-y)^2 -> min.
    // It means dY/dX = k, where y = k*x+q.
    inline RealT slopeY() const;

    //! Returns the slope dX/dY. The used criterion is Sum(X-x)^2 -> min.
    // It means dX/dY = k, where x = k*y+q.
    inline RealT slopeX() const;

    //! Returns the estimate of q, if y = k*x+q.
    // The used criterion is Sum(Y-y)^2 -> min.
    inline RealT interceptY() const;

    //! Returns the estimate of q, if y = k*y+q.
    // The used criterion is Sum(X-x)^2 -> min.
    inline RealT interceptX() const;

    //! Return the covariance matrix.
    [[nodiscard]] Matrix<RealT, 2, 2> covariance() const;

    //! Calculate the centroid.
    [[nodiscard]] Point<RealT, 2> centroid() const
    {
      return Point<RealT, 2>({centroidX(), centroidY()});
    }

    //! Calculate the centroid.
    template<unsigned Axis>
    [[nodiscard]] RealT centroid() const
    {
      if constexpr(Axis == 0) {
        return M10() / M00();
      } else  if constexpr(Axis == 1) {
        return M01() / M00();
      } else {
        static_assert(Axis < 2,"Axis must be 0 or 1 ");
      }
    }

    //! Add to sets of moments together.
    Moments2 operator+(const Moments2 &m) const
    {
      return Moments2(m00 + m.M00(), m10 + m.M10(), m01 + m.M01(), m20 + m.M20(), m11 + m.M11(), m02 + m.M02());
    }

    //! Subtract one set of moments from another.
    Moments2 operator-(const Moments2 &m) const
    {
      return Moments2(m00 - m.M00(), m10 - m.M10(), m01 - m.M01(), m20 - m.M20(), m11 - m.M11(), m02 - m.M02());
    }

    //! Add to sets of moments to this one.
    const Moments2 &operator+=(const Moments2 &m);

    //! Subtract a set of moments to this one.
    const Moments2 &operator-=(const Moments2 &m);

    //! Swap X and Y co-ordinates.
    void swapXY()
    {
      std::swap(m10, m01);
      std::swap(m20, m02);
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
    RealT m00 = 0;
    RealT m10 = 0;
    RealT m01 = 0;
    RealT m20 = 0;
    RealT m11 = 0;
    RealT m02 = 0;
  };

  template <class RealT>
  std::ostream &
  operator<<(std::ostream &os, const Moments2<RealT> &mom);

  template <class RealT>
  std::istream &
  operator>>(std::istream &is, Moments2<RealT> &mom);

  //: Return the covariance matrix.

  template <class RealT>
  Matrix<RealT, 2, 2> Moments2<RealT>::covariance() const
  {
    if(isNearZero(m00))
      return Matrix<RealT, 2, 2>({{1, 0}, {0, 1}});// if m00 is too small (prevent effective division by zero)
    RealT cent1 = centroidX();
    RealT cent2 = centroidY();
    RealT diag = m11 / m00 - cent1 * cent2;
    return Matrix<RealT, 2, 2>({{m20 / m00 - sqr(cent1), diag},
                                {diag, m02 / m00 - sqr(cent2)}});
  }

  template <class RealT>
  Vector<RealT, 2> Moments2<RealT>::principalAxisSize() const
  {
    Matrix<RealT, 2, 2> mat = covariance();
    auto vec = xt::linalg::eigvalsh(mat);
    if(vec[0] < vec[1])
      std::swap(vec[0], vec[1]);
    return vec;
  }

  template <class RealT>
  std::ostream &operator<<(std::ostream &os, const Moments2<RealT> &mom)
  {
    os << mom.M00() << ' ' << mom.M10() << ' ' << mom.M01() << ' '
       << mom.M20() << ' ' << mom.M11() << ' ' << mom.M02();
    return os;
  }

  template <class RealT>
  std::istream &operator>>(std::istream &is, Moments2<RealT> &mom)
  {
    is >> mom.m00 >> mom.m10 >> mom.m01
      >> mom.m20 >> mom.m11 >> mom.m02;
    return is;
  }

  template <class RealT>
  inline void Moments2<RealT>::addPixel(const Index<2> &pxl)
  {
    auto a = RealT(pxl[0]);
    auto b = RealT(pxl[1]);

    m00++;
    m01 += b;
    m10 += a;
    m11 += a * b;
    m02 += b * b;
    m20 += a * a;
  }

  template <class RealT>
  inline void Moments2<RealT>::addPixel(const Point<RealT, 2> &pxl)
  {
    RealT a = pxl[0];
    RealT b = pxl[1];

    m00++;
    m01 += b;
    m10 += a;
    m11 += a * b;
    m02 += b * b;
    m20 += a * a;
  }

  template <class RealT>
  void Moments2<RealT>::addPixel(const Point<RealT, 2> &pxl, RealT weight)
  {
    RealT a = pxl[0];
    RealT b = pxl[1];

    m00 += weight;
    RealT wa = a * weight;
    RealT wb = b * weight;
    m01 += wb;
    m10 += wa;
    m11 += a * wb;
    m02 += b * wb;
    m20 += a * wa;
  }

  template <class RealT>
  inline const Moments2<RealT> &Moments2<RealT>::operator+=(const Moments2<RealT> &m)
  {
    m00 += m.M00();
    m10 += m.M10();
    m01 += m.M01();
    m20 += m.M20();
    m11 += m.M11();
    m02 += m.M02();
    return *this;
  }

  template <class RealT>
  inline const Moments2<RealT> &Moments2<RealT>::operator-=(const Moments2<RealT> &m)
  {
    m00 -= m.M00();
    m10 -= m.M10();
    m01 -= m.M01();
    m20 -= m.M20();
    m11 -= m.M11();
    m02 -= m.M02();
    return *this;
  }

  template <class RealT>
  inline RealT Moments2<RealT>::slopeY() const
  {
    RealT det = m00 * m20 - m10 * m10;
    if(isNearZero(det))
      throw std::underflow_error("Moments2<RealT>::slopeY(), Determinant near zero. ");
    return (m00 * m11 - m10 * m01) / det;
  }

  template <class RealT>
  inline RealT Moments2<RealT>::interceptY() const
  {
    RealT det = m00 * m20 - m10 * m10;
    if(isNearZero(det))
      throw std::underflow_error("Moments2::interceptY(), Determinant near zero. ");
    return (m20 * m01 - m10 * m11) / det;
  }

  template <class RealT>
  inline RealT Moments2<RealT>::slopeX() const
  {
    RealT det = m00 * m02 - m01 * m01;
    if(isNearZero(det))
      throw std::underflow_error("Moments2::slopeX(), Determinant near zero. ");
    return (m00 * m11 - m01 * m10) / det;
  }

  template <class RealT>
  inline RealT Moments2<RealT>::interceptX() const
  {
    RealT det = m00 * m02 - m01 * m01;
    if(isNearZero(det))
      throw std::underflow_error("Moments2::interceptX(), Determinant near zero. ");
    return (m02 * m10 - m01 * m11) / det;
  }

  extern template class Moments2<float>;
  extern template class Moments2<double>;

}// namespace Ravl2

#if FMT_VERSION >= 90000
template <typename RealT>
struct fmt::formatter<Ravl2::Moments2<RealT> > : fmt::ostream_formatter {
};
#endif

