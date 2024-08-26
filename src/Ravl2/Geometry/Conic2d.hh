// This file is part of RAVL, Recognition And Vision Library
// Copyright (C) 2004, OmniPerception Ltd
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here
//! author="Charles Galambos"
//! date="21/03/2004"
//! docentry="Ravl.API.Math.Geometry.2D"

#pragma once

#include <vector>
#include <spdlog/spdlog.h>
#include "Ravl2/Types.hh"
#include "Ravl2/Geometry/LineABC2d.hh"

//#include "Ravl2/LeastSquares.hh"
//#include "Ravl2/Ellipse2d.hh"
//#include "Ravl2/Eigen.hh"

#define DODEBUG 1
#if DODEBUG
#define ONDEBUG(x) x
#else
#define ONDEBUG(x)
#endif

namespace Ravl2
{

  //! @brief Conic in 2d space.
  //!  a * sqr(row) + b * row * col + c * sqr(col) + d * row + e * col + f = 0

  template <typename RealT>
  class Conic2dC
  {
  public:
    //! @brief Default constructor.
    //! The conic is undefined.
    constexpr Conic2dC() = default;

    //! @brief Construct from parameter vector.
    //! @param  params - Parameters, entry 0 = a, 1 = b, 2 = c, 3 = d, 4 = e, 5 = f
    constexpr explicit Conic2dC(const Vector<RealT, 6> &params)
        : p(params)
    {}

    //! @brief Construct from parameter vector.
    constexpr explicit Conic2dC(const VectorT<RealT> &params)
    {
      if(params.size() != 6) {
        throw std::invalid_argument("Conic2dC: Invalid parameter vector size");
      }
      p = params;
    }

    //! Construct from parameters
    constexpr Conic2dC(RealT a, RealT b, RealT c, RealT d, RealT e, RealT f)
    {
      p[0] = a;
      p[1] = b;
      p[2] = c;
      p[3] = d;
      p[4] = e;
      p[5] = f;
    }

    //! @brief Construct from matrix C in projective ellipse equation x.T() * C * x = 0.
    //! Thus matrix stores parameters as:<pre>
    //!      (  a  b/2 d/2 )
    //!  C = ( b/2  c  e/2 )
    //!      ( d/2 e/2  f  )</pre>
    constexpr explicit Conic2dC(const Matrix<RealT, 3, 3> &matrix)
    {
      // Should check matrix is symmetric ?
      p[0] = matrix(0, 0);
      p[1] = matrix(0, 1) + matrix(1, 0);
      p[2] = matrix(1, 1);
      p[3] = matrix(0, 2) + matrix(2, 0);
      p[4] = matrix(1, 2) + matrix(2, 1);
      p[5] = matrix(2, 2);
    }

    //! @brief Is point on curve ?
    //! @param pnt - Point to test.
    //! @return true if point is on curve.
    [[nodiscard]] constexpr bool IsOnCurve(const Point<RealT, 2> &pnt, RealT tolerance = RealT(1e-5)) const
    {
      return isNearZero(Residue(pnt), tolerance);
    }

    //! @brief Compute the residue
    //! Compute x.T() * C * x, where x is projective version of pnt. <br>
    //! Hence gives a measure of distance of point from curve
    [[nodiscard]] constexpr RealT Residue(const Point<RealT, 2> &pnt) const
    {
      return p[0] * sqr(pnt[0]) + p[1] * pnt[0] * pnt[1] + p[2] * sqr(pnt[1]) + p[3] * pnt[0] + p[4] * pnt[1] + p[5];
    }

    //! Get the coefficient matrix. 'C'
    //! Such that  x.T() * C * x = 0
    [[nodiscard]] constexpr Matrix<RealT, 3, 3> C() const
    {
      return Matrix<RealT, 3, 3>({{p[0], p[1] / 2, p[3] / 2},
                                  {p[1] / 2, p[2], p[4] / 2},
                                  {p[3] / 2, p[4] / 2, p[5]}});
    }

    //! Find the tangent at point 'pnt', where pnt is on the conic.
    [[nodiscard]] constexpr LineABC2dC<RealT> tangent(const Point<RealT, 2> &pnt)
    {
      // TODO:- This can be simplified a lot.
      Vector<RealT, 3> res = dot(C(), toVector<RealT>(pnt[0], pnt[1], 1));
      return LineABC2dC<RealT>(res[0], res[1], res[2]);
    }

    //! Access parameter vector.
    //! 0-a 1-b 2-c 3-d 4-e 5-f
    [[nodiscard]] constexpr const Vector<RealT, 6> &Parameters() const
    {
      return p;
    }

    //! @brief Compute various ellipse parameters.
    //! @param  centre - Centre of ellipse.
    //! @param  major - Size of major axis.
    //! @param  minor - Size of minor axis
    //! @param  angle - Angle of major axis.
    constexpr bool EllipseParameters(Point<RealT, 2> &centre, RealT &major, RealT &minor, RealT &angle) const
    {
      Matrix<RealT, 2, 2> t;
      if(!ComputeEllipse(centre, t))
        return false;
      angle = std::atan2(-2 * t(0, 1), -t(0, 0) + t(1, 1)) / 2;
      RealT cosa = std::cos(angle);
      RealT sina = std::sin(angle);
      RealT w = 2 * t(0, 1) * cosa * sina;
      major = std::sqrt(1 / (t(0, 0) * cosa * cosa + w + t(1, 1) * sina * sina));
      minor = std::sqrt(1 / (t(0, 0) * sina * sina - w + t(1, 1) * cosa * cosa));
      ONDEBUG(SPDLOG_INFO("Center={} Major={} Minor={} Angle={}", centre, major, minor, angle));
      return true;
    }

    //! @brief Compute ellipse parameters.
    // Assumes conic is ellipse, computes ellipse centre and returns remaining parameters as symmetric 2D matrix
    constexpr bool ComputeEllipse(Point<RealT, 2> &c, Matrix<RealT, 2, 2> &mat) const
    {
      // (Bill Xmas) I think what this does is:
      // - compute centre by completing the square on ax^2 +bxy + .....
      // - shift the centre of the ellipse to the origin
      // - sort out the projective scale
      // returning the result in Euclidean form in "c" & "mat"

      // compute centre by completing the square on ax^2 +bxy + .....
      Vector<RealT, 6> u = p;
      RealT idet = RealT(1) / (u[0] * u[2] - sqr(u[1]) * RealT(0.25));
      if(idet <= 0) {
        //cerr << "Not an ellipse.\n";
        return false;
      }
      //ONDEBUG(std::cerr << "idet=" << idet << "\n");

      u *= std::sqrt(idet * RealT(0.25));

      c = toPoint<RealT>((-u[3] * u[2] + u[4] * u[1] * RealT(0.5)) * 2,
                         (-u[0] * u[4] + u[3] * u[1] * RealT(0.5)) * 2);

      RealT scale = -1 / (u[0] * sqr(c[0]) + u[1] * c[0] * c[1] + u[2] * sqr(c[1]) + u[3] * c[0] + u[4] * c[1] + u[5]);

      RealT t = u[1] * scale * RealT(0.5);
      mat = Matrix<RealT, 2, 2>({{u[0] * scale, t},
                                 {t, u[2] * scale}});
      return true;
    }

  private:
    Vector<RealT, 6> p;// 0-a 1-b 2-c 3-d 4-e 5-f
  };

#if 0

  bool FitEllipse(const std::vector<Point<RealT,2>> &points,Conic2dC &conic);
  //: Fit ellipse
#endif

  //! Write ellipse to text stream.
  template <typename RealT>
  std::ostream &operator<<(std::ostream &s, const Conic2dC<RealT> &obj)
  {
    s << obj.Parameters();
    return s;
  }

  //! Read ellipse from text stream.
  template <typename RealT>
  std::istream &operator>>(std::istream &s, Conic2dC<RealT> &obj)
  {
    Vector<RealT, 6> p;
    s >> p;
    obj = Conic2dC(p);
    return s;
  }

}// namespace Ravl2

#if FMT_VERSION >= 90000
template <typename RealT>
struct fmt::formatter<Ravl2::Conic2dC<RealT>> : fmt::ostream_formatter {
};
#endif

#undef DODEBUG
#undef ONDEBUG