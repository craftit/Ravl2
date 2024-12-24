// This file is part of RAVL, Recognition And Vision Library
// Copyright (C) 2004, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here
//! author="Charles Galambos"
//! date="6/4/2004"
//! docentry="Ravl.API.Math.Geometry.2D"

#pragma once

#include <eigen3/Eigen/SVD>
#include "Ravl2/Types.hh"
#include "Ravl2/Geometry/Affine.hh"
#include "Ravl2/Geometry/Conic2.hh"
#include "Ravl2/Math/LinearAlgebra.hh"

#define DODEBUG 0
#if DODEBUG
#define ONDEBUG(x) x
#else
#define ONDEBUG(x)
#endif

namespace Ravl2
{
  template <typename RealT>
  class Conic2;

  //! @brief Ellipse in 2d space.
  //! This uses a form of inverted Euclidean representation, in contrast to the general 2-D conic <a href="RavlN.Conic2.html">Conic2</a>.<br>
  //! The representation is the affine transform that transforms (scales, rotates, translates) a point from the unit circle to the "corresponding"
  //! point on the ellipse.

  template <typename RealT>
  class Ellipse
  {
  public:
    //! @brief Default constructor.
    //! The parameters of the ellipse are left undefined.
    constexpr Ellipse() = default;

    //! Create from conic parameters.
    //!param: conicParams - Conic parameters a to f, where a * sqr(row) + b * row * col + c * sqr(col) + d * row + e * col + f = 0
    constexpr explicit Ellipse(const Vector<RealT, 6> &conicParams)
    {
      Conic2<RealT> conic(conicParams);
      *this = *toEllipse(conic);// What to do if this fails?
    }

    //! Construct from affine transform from unit circle centered on the origin
    //!param: np - Transform from unit circle centered on the origin
    constexpr explicit Ellipse(const Affine<RealT, 2> &np)
        : p(np)
    {}

    //! Construct from affine transform from unit circle centered on the origin
    //!param: sr - scale rotation matrix.
    //!param: off - offset from origin
    constexpr Ellipse(const Matrix<RealT, 2, 2> &sr, const Vector<RealT, 2> &off)
        : p(sr, off)
    {}

    //! Create an new ellipse
    //!param: centre - Centre of ellipse.
    //!param: major - Size of major axis. (at given angle)
    //!param: minor - Size of minor axis.
    //!param: angle - Angle of major axis.
    constexpr Ellipse(const Point<RealT, 2> &centre, RealT major, RealT minor, RealT angle)
    {
      p = Affine<RealT, 2>(Matrix<RealT, 2, 2>({{std::cos(angle), -std::sin(angle)},
                                                                {std::sin(angle), std::cos(angle)}}) *
                                           Matrix<RealT, 2, 2>({{major, 0},
                                                                {0, minor}}),
                           centre);
    }

    //! Compute point on ellipse.
    [[nodiscard]] constexpr Point<RealT, 2> point(RealT angle) const
    {
      return p(toPoint<RealT>(std::cos(angle), std::sin(angle)));
    }

    //! Access as projection from unit circle centered on the origin
    [[nodiscard]] constexpr const Affine<RealT, 2> &Projection() const
    {
      return p;
    }

    //! Centre of the ellipse.
    [[nodiscard]] constexpr Point<RealT, 2> Centre() const
    {
      return p.Translation();
    }

    //! Is point on the curve ?
    [[nodiscard]] constexpr bool IsOnCurve(const Point<RealT, 2> &pnt, RealT tolerance = std::numeric_limits<RealT>::epsilon()) const
    {
      Point<RealT, 2> mp = (inverse(p))(pnt);
      RealT d = sumOfSqr(mp) - 1;
      return isNearZero(d, tolerance);
    }

    //! Compute the residue from
    [[nodiscard]] constexpr RealT residue(const Point<RealT, 2> &pnt) const
    {
      Point<RealT, 2> mp = (inverse(p))(pnt);
      return sumOfSqr(mp) - 1;
    }

    //! @brief Compute various ellipse parameters.
    //!param: centre - Centre of ellipse.
    //!param: major - Size of major axis.
    //!param: minor - Size of minor axis
    //!param: angle - Angle of major axis.
    constexpr bool EllipseParameters(Point<RealT, 2> &centre, RealT &major, RealT &minor, RealT &angle) const
    {
      centre = p.Translation();
      ONDEBUG(std::cerr << "SRMatrix:\n"
                        << p.SRMatrix() << std::endl);
      //auto [u, s, vt] = xt::linalg::svd(p.SRMatrix(), true, true);
      auto svd = p.SRMatrix().jacobiSvd(Eigen::ComputeFullU);
      auto s = svd.singularValues();
      auto u = svd.matrixU();
      ONDEBUG(std::cerr << "U:\n"
                        << u << "\nS:" << s << "V:\n"
                        << vt << std::endl);
      // U contains the rotation in the form:
      // cos -sin
      // sin  cos
      // hence angle can be computed as:
      angle = std::atan2(u(1, 0), u(0, 0));
      major = s[0];
      minor = s[1];
      ONDEBUG(std::cerr << "Center=" << centre << " Major=" << major << " Minor=" << minor << " Angle=" << angle << "\n");
      return true;
    }

    //! @brief Compute the size of major and minor axis.
    //! @return Size of major and minor axis, respectively.
    [[nodiscard]]
    constexpr std::tuple<RealT, RealT> size() const
    {
      Eigen::template JacobiSVD<Matrix<RealT, 2, 2> > svd(p.SRMatrix());
      //auto [u, s, vt] = xt::linalg::svd(p.SRMatrix(), false, false);
      auto s = svd.singularValues();
      return {s[0], s[1]};
    }

  protected:
    Affine<RealT, 2> p;// Projection from unit circle.
  };

  //! @brief Represent conic as an conic.
  //! @param  conic - Conic to turn into an Ellipse
  //! @return Ellipse if conic is an conic, otherwise if hyperbola or degenerate std::nullopt.
  template <typename RealT>
  std::optional<Ellipse<RealT>> toEllipse(const Conic2<RealT> &conic)
  {
    // Ellipse representation is transformation required to transform unit
    // circle into conic.  This is the inverse of the "square root" of
    // Euclidean matrix representation
    Matrix<RealT, 2, 2> euc;// Euclidean representation of eclipse equation
    Point<RealT, 2> centre;
    // Separate projective conic representation into Euclidean + translation
    if(!conic.computeEllipse(centre, euc))
      return std::nullopt;
    ONDEBUG(std::cerr << "Euclidean conic is:\n"
                      << euc << "\n Center=" << centre << std::endl);

    // Then decompose to get orientation and scale
    //auto [lambda, E] = xt::linalg::eigh(euc);
    Eigen::SelfAdjointEigenSolver<Matrix<RealT, 2, 2>> solver(euc);
    Matrix<RealT,2,2> E = solver.eigenvectors();
    Vector<RealT,2> lambda = solver.eigenvalues();

    // lambda now contains inverted squared *minor* & *major* axes respectively
    // (N.B.: check: E[0][1] *MUST* have same sign as conic orientation)
    ONDEBUG(std::cerr << "Eigen decomp is:\n"
                      << E << "\n"
                      << lambda << std::endl);

    Matrix<RealT, 2, 2> scale(
      {{0, RealT(1 / RealT(std::sqrt(lambda[0])) )},
       {RealT(1 / std::sqrt(lambda[1])), 0}});
    // Columns are swapped in order to swap x & y to compensate for eigenvalue
    // ordering.  I.e. so that [1,0] on unit circle gets mapped to
    // end of major axis rather than minor axis.

    auto ret = Ellipse<RealT>(E * scale, centre);
    ONDEBUG(std::cerr << "Ellipse:\n"
                      << ret << std::endl);
    ONDEBUG(std::cerr << "[1,0] on unit circle goes to " << ret.Projection()(toVector<RealT>(1, 0)) << " on conic" << std::endl);
    return ret;
  }
  
  //! docentry="Ravl.API.Math.Statistics;Ravl.API.Math.Geometry.2D"
  //! @brief Compute an ellipse from a 2d covariance matrix, mean, and standard deviation.
  //! The ellipse is the contour of a 2-D Gaussian random variable which lies "stdDev" standard deviations from the mean.
  template <typename RealT>
  [[nodiscard]] constexpr Ellipse<RealT> ellipseMeanCovariance(const Matrix<RealT, 2, 2> &covar, const Point<RealT, 2> &mean, RealT stdDev = 1.0)
  {
    //auto [dv, E] = xt::linalg::eigh(covar);
    Eigen::SelfAdjointEigenSolver<Eigen::Matrix<RealT, 2, 2>> solver(covar);

    Matrix<RealT,2,2> E = solver.eigenvectors();
    Vector<RealT,2> dv = solver.eigenvalues();
    ONDEBUG(std::cerr << "l: " << dv << "\nE\n"
                      << E << std::endl);
    Matrix<RealT, 2, 2> d(
      {{stdDev * std::sqrt(dv[0]), 0},
       {0, stdDev * std::sqrt(dv[1])}});
    Matrix<RealT, 2, 2> sr = E * d;
    return Ellipse(sr, mean);
  }

  //:-
  //! docentry="Ravl.API.Math.Geometry.2D"

  //! Write ellipse to text stream.
  template <typename RealT>
  std::ostream &operator<<(std::ostream &s, const Ellipse<RealT> &obj)
  {
    s << obj.Projection();
    return s;
  }

  //! Read ellipse from text stream.
  template <typename RealT>
  std::istream &operator>>(std::istream &s, Ellipse<RealT> &obj)
  {
    Affine<RealT, 2> aff;
    s >> aff;
    obj = Ellipse(aff);
    return s;
  }

  extern template class Ellipse<float>;
}// namespace Ravl2

#if FMT_VERSION >= 90000
template <typename RealT>
struct fmt::formatter<Ravl2::Ellipse<RealT>> : fmt::ostream_formatter {
};
#endif

#undef DODEBUG
#undef ONDEBUG
