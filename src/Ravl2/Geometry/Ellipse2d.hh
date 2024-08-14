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

#include "Ravl2/Types.hh"
#include "Ravl2/Geometry/Affine.hh"

namespace Ravl2 {
  template<typename RealT>
  class Conic2dC;
  
  //! @brief Ellipse in 2d space.
  //! This uses a form of inverted Euclidean representation, in contrast to the general 2-D conic <a href="RavlN.Conic2dC.html">Conic2dC</a>.<br>
  //! The representation is the affine transform that transforms (scales, rotates, translates) a point from the unit circle to the "corresponding"
  //! point on the ellipse.

  template<typename RealT>
  class Ellipse2dC
  {
  public:
    //! @brief Default constructor.
    //! The parameters of the ellipse are left undefined.
    Ellipse2dC() = default;

    //! Create from conic parameters.
    //!param: conicParams - Conic parameters a to f, where a * sqr(row) + b * row * col + c * sqr(col) + d * row + e * col + f = 0
    explicit Ellipse2dC(const Vector<RealT,6> &conicParams)
    {
      Conic2dC conic(conicParams);
      conic.AsEllipse(*this); // What to do if this fails?
    }

    //! Construct from affine transform from unit circle centered on the origin
    //!param: np - Transform from unit circle centered on the origin
    explicit Ellipse2dC(const Affine<RealT,2> &np)
      : p(np)
    {}

    //! Construct from affine transform from unit circle centered on the origin
    //!param: sr - scale rotation matrix.
    //!param: off - offset from origin
    Ellipse2dC(const Matrix<RealT,2,2> &sr,const Vector<RealT,2> &off)
      : p(sr,off)
    {}

    //! Create an new ellipse
    //!param: centre - Centre of ellipse.
    //!param: major - Size of major axis. (at given angle)
    //!param: minor - Size of minor axis.
    //!param: angle - Angle of major axis.
    Ellipse2dC(const Point<RealT,2> &centre,RealT major,RealT minor,RealT angle)
    {
      p = FAffineC<2>(Matrix<RealT, 2, 2>(std::cos(angle), -std::sin(angle),
                                          std::sin(angle), std::cos(angle)) *
                      Matrix<RealT, 2, 2>(major, 0,
                                          0, minor),
                      centre);
    }


    //! Compute point on ellipse.
    [[nodiscard]] Point<RealT,2> point(RealT angle) const
    { return p * Angle2Vector2d(angle); }

    //! Access as projection from unit circle centered on the origin
    [[nodiscard]] const Affine<RealT,2> &Projection() const
    { return p; }

    //! Centre of the ellipse.
    [[nodiscard]] inline Point<RealT,2> Centre() const
    { return p.Translation(); }

    //! Is point on the curve ?
    [[nodiscard]] bool IsOnCurve(const Point<RealT,2> &pnt) const;

    //: Compute various ellipse parameters.
    //!param: centre - Centre of ellipse.
    //!param: major - Size of major axis.
    //!param: minor - Size of minor axis
    //!param: angle - Angle of major axis.
    bool EllipseParameters(Point<RealT,2> &centre,RealT &major,RealT &minor,RealT &angle) const;

#if 0
    //: Compute the size of major and minor axis.
    //!param: major - Size of major axis.
    //!param: minor - Size of minor axis
    bool Size(RealT &major,RealT &minor) const
    {
      FVectorC<2> vec = SVD(p.SRMatrix());
      major = vec[0];
      minor = vec[1];
      return true;
    }
#endif

  protected:    
    Affine<RealT,2> p; // Projection from unit circle.
  };

  //! @brief Represent conic as an ellipse.
  //! @param  conic - Conic to turn into an Ellipse
  //!return: Ellipse if conic is an ellipse, otherwise std::nullopt.
  template<typename RealT>
  std::optional<Ellipse2dC<RealT> > toEllipse(Conic2dC<RealT> &ellipse);


  //! @brief Fit ellipse to points.
  //!param: points -  Set of points to fit to an ellipse.
  //!param: ellipse - Ellipse structure to store result in.
  //! Based on method presented in 'Numerically Stable Direct Least Squares Fitting of Ellipses'
  //! by Radim Halir and Jan Flusser.
  template<typename RealT>
  bool FitEllipse(const std::vector<Point<RealT,2>> &points,Ellipse2dC<RealT> &ellipse);

  //! docentry="Ravl.API.Math.Statistics;Ravl.API.Math.Geometry.2D"
  //! @brief Compute an ellipse from a 2d covariance matrix, mean, and standard deviation.
  //! The ellipse is the contour of a 2-D Gaussian random variable which lies "stdDev" standard deviations from the mean.
  template<typename RealT>
  Ellipse2dC<RealT> EllipseMeanCovariance(const Matrix<RealT,2,2> &covar,const Point<RealT,2> &mean,RealT stdDev = 1.0);

  //:-
  //! docentry="Ravl.API.Math.Geometry.2D"

  //! Write ellipse to text stream.
  template<typename RealT>
  std::ostream &operator<<(std::ostream &s,const Ellipse2dC<RealT> &obj)
  {
    s << obj.Projection();
    return s;
  }

  //! Read ellipse from text stream.
  template<typename RealT>
  std::istream &operator>>(std::istream &s,Ellipse2dC<RealT> &obj)
  {
    Affine<RealT,2> aff;
    s >> aff;
    obj = Ellipse2dC(aff);
    return s;
  }

  
}


