// This file is part of RAVL, Recognition And Vision Library 
// Copyright (C) 2001, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here

#pragma once

#include "Ravl2/Geometry/Affine.hh"

//namespace Ravl2
//{
//  template<class DataT> class SArray1dC;
//  template<class DataT> class DArray1dC;
//  template<class DataT> class DListC;
//  template<class DataT> class PairC;
//  class Point2dC;
//  class PointSet2dC;
//  class Polygon2dC;
//
//  //! userlevel=Normal
//  //: 2-D affine transformation
//
//  // This class is used to apply a (6-parameter) 2-D affine transformation.  Note
//  // that it transforms the point w.r.t. the coordinate system; it does
//  // <b>not</b> transform the coordinate system itself.
//
//  class Affine2fC
//    : public Affine<float,2>
//  {
//  public:
//    inline Affine2dC()
//      : Affine<2>()
//    {}
//    //: Construct identity transformation.
//
//    inline Affine2dC(const FAffineC<2> &Oth)
//      : FAffineC<2>(Oth)
//    {}
//    //: Construct from base template.
//
//    inline Affine2dC(const Affine2dC &Oth)
//      : Affine<2>(Oth)
//    {}
//    //: Copy constructor.
//
//    inline Affine2dC(const Matrix2dC &SR, const Vector2dC &T)
//      : FAffineC<2>(SR,T)
//    {}
//    //: Constructor from scaling/rotation matrix and translation vector.
//
//    inline Affine2dC(const Vector2dC &Scale, RealT Angle, const Vector2dC &Trans);
//    //: Constructor for arbitrary anisotropic scaling, rotation and translation.
//    // The scaling axes are aligned with the coordinate axes.
//
//    inline Affine2dC(const Vector2dC &Scale, RealT ScaleAngle, RealT Angle, const Vector2dC &Trans);
//    //: Constructor for arbitrary anisotropic scaling, rotation and
//    //: translation.
//    // The scaling axes are aligned at an angle <code>ScaleAngle</code> w.r.t. the coordinate axes.
//
//    void Rotate(RealT Angle);
//    //: Add rotation <code>Angle</code> to transformation
//
//    RealT OrthogonalCrossProduct() const
//    { return SRMatrix()[0][0] * SRMatrix()[1][1] - SRMatrix()[0][1] * SRMatrix()[1][0]; }
//    //: This returns the cross product of the projection of two orthogonal unit vectors.
//    // This is useful if you want to test if the transform is mirrored. i.e. changes
//    // the direction of rotations.
//
//    void Decompose(Point2dC &translation,Vector2dC &scaling,RealT &skew,RealT &rotation) const;
//    //: Decompose affine transform.
//    //!param: translation - Translation of used in the transform
//    //!param: scaling - Row and column components of scaling.
//    //!param: skew - Skew component of scaling.
//    //!param: rotation - Angle of rotation.
//    // The decomposition is done such that: <br>
//    // Matrix2dC ScalingRotation = Matrix2dC(Cos(rotation),Sin(rotation),-Sin(rotation),Cos(rotation)) * Matrix2dC(scaling[0],skew,skew,scaling[1]) <br>
//    // The translation component is simply copied.
//
//    static Affine2dC Compose(Point2dC translation,Vector2dC scaling,RealT skew,RealT rotation);
//    //: Compose an affine transform the components generated from decompose.
//    //!param: translation - Translation of used in the transform
//    //!param: scaling - Row and column components of scaling.
//    //!param: skew - Skew component of scaling.
//    //!param: rotation - Angle of rotation.
//    // The composition is done such that: <br>
//    // Matrix2dC ScalingRotation = Matrix2dC(Cos(rotation),Sin(rotation),-Sin(rotation),Cos(rotation)) * Matrix2dC(scaling[0],skew,skew,scaling[1]) <br>
//    // The translation component is simply copied.
//
//    inline FAffineC<2> Inverse() const {
//      FAffineC<2> ret;
//      Matrix2dC(SR).Invert(ret.SRMatrix());
//      Mul(ret.SRMatrix(),T,ret.Translation());
//      ret.Translation() *= -1;
//      return ret;
//    }
//
//    FAffineC<2> I() const
//    { return Inverse(); }
//    //: Generate an inverse transformation.
//    // This is obsolete, use Inverse().
//  };
//
//  bool FitAffine(const Point2dC &p1a,const Point2dC &p1b,
//		 const Point2dC &p2a,const Point2dC &p2b,
//		 const Point2dC &p3a,const Point2dC &p3b,
//		 Affine2dC &affine);
//  //: Fit affine transform from mapping of 3 points.
//
//  Affine2dC FitAffine(const SArray1dC<Point2dC> &orig,const SArray1dC<Point2dC> &newPos,RealT &residual);
//  //: Fit an affine transform to 2 arrays of corresponding points
//  // A "least sum of squares" fitter is used.  The result transforms the points in "orig" to those in "newPos". The residual from the fit is returned as "residual".
//
//  Affine2dC FitAffine(const SArray1dC<Point2dC> &orig,const SArray1dC<Point2dC> &newPos);
//  //: Fit an affine transform to 2 arrays of corresponding points
//  // A "least sum of squares" fitter is used.  The result transforms the points in "orig" to those in "newPos".
//
//
//
//  Affine2dC FitAffine(const SArray1dC<Point2dC> &org,const SArray1dC<Point2dC> &newPos, const SArray1dC<RealT> &weights, RealT& residual);
//  //: Fit an affine transform to 2 arrays of corresponding points
//  // A "weighted least sum of squares" fitter is used.  The result transforms the points in "orig" to those in "newPos".
//
//
//  Affine2dC FitAffine(const SArray1dC<Point2dC> &orig,const SArray1dC<Point2dC> &newPos, const SArray1dC<RealT> &weights);
//  //: Fit an affine transform to 2 arrays of corresponding points
//  // A "least sum of squares" fitter is used.  The result transforms the points in "orig" to those in "newPos".
//
//
//  Affine2dC FitAffine(const DListC<Point2dC> &orig,const DListC<Point2dC> &newPos,RealT &residual);
//  //: Fit an affine transform to 2 lists of corresponding points
//  // A "least sum of squares" fitter is used.  The result transforms the points in "orig" to those in "newPos". The residual from the fit is returned as "residual".
//
//
//  Affine2dC FitAffine(const DListC<Point2dC> &orig,const DListC<Point2dC> &newPos);
//  //: Fit an affine transform to 2 lists of corresponding points
//  // A "least sum of squares" fitter is used.  The result transforms the points in "orig" to those in "newPos".
//
//  Affine2dC FitAffine(const SArray1dC<PairC<Point2dC> > &matchPairs,RealT &residual);
//  //: Fit an affine transform to a list of corresponding points
//  // A "least sum of squares" fitter is used.  The result transforms the points in element 1 of the point pairs to those in element 2 of the point pairs. The residual from the fit is returned as "residual".
//
//  Affine2dC FitAffine(const DArray1dC<PairC<Point2dC> > &matchPairs,RealT &residual);
//  //: Fit an affine transform to a list of corresponding points
//  // A "least sum of squares" fitter is used.  The result transforms the points in element 1 of the point pairs to those in element 2 of the point pairs. The residual from the fit is returned as "residual".
//
//  bool FitSimilarity(const SArray1dC<Point2dC> &points1,
//                     const SArray1dC<Point2dC> &points2,
//                     Matrix2dC &rotation,
//                     Vector2dC &translation,
//                     RealT &scaling,
//                     bool forceUnitScale = false,
//                     bool forceUnitRotation = false
//                     );
//  //: <p>Least squares fit of a similarity transform between the two point sets.
//  // If 'forceUnitScale' is true then unit scaling will be assumed, though
//  // the measured scale will still be assigned to 'scaling'.  This may
//  // effect the computed translation in particular. The transform will
//  // map points1 to points2.</p>
//  // <p> To obtain angle as a scalar:  <code>ATan2(R[0][1],R[0][0])</code></p>
//
//
//  bool FitSimilarity(const SArray1dC<Point2dC> &points1,
//                     const SArray1dC<Point2dC> &points2,
//                     Affine2dC &transform,
//                     bool forceUnitScale = false,
//                     bool forceUnitRotation = false
//                     );
//  //: Least squares fit of a similarity transform between the two point sets.
//  // If 'forceUnitScale' is true then unit scaling will be assumed.
//  // The transform will map points1 to points2
//
//
//  inline bool FitIsometry(const SArray1dC<Point2dC> &points1,
//                          const SArray1dC<Point2dC> &points2,
//                          Affine2dC &transform)
//  { return FitSimilarity(points1,points2,transform,true); }
//  //: Least squares fit of a isometry transform between the two point sets.
//
//
//  PointSet2dC operator*(const FAffineC<2> &trans,const PointSet2dC &points);
//  //: Apply a affine transform to a point set
//
//  Polygon2dC operator*(const FAffineC<2> &trans,const Polygon2dC &points);
//  //: Apply a affine transform to a polygon
//
//  SArray1dC<Point2dC> operator*(const FAffineC<2> &trans,const SArray1dC<Point2dC> &points);
//  //: Apply a affine transform to an array of points.
//
//  /////////////////////////////////
//
//  inline
//  Affine2dC::Affine2dC(const Vector2dC &Sc, RealT Angle, const Vector2dC &Trans)
//    : FAffineC<2>(Matrix2dC(1,0,0,1),Trans)
//  {
//    Scale(Sc);
//    Rotate(Angle);
//  }
//
//  inline
//  Affine2dC::Affine2dC(const Vector2dC &Sc, RealT ScaleAngle, RealT Angle, const Vector2dC &Trans)
//    : FAffineC<2>(Matrix2dC(1,0,0,1),Trans)
//  {
//    Rotate(-ScaleAngle);
//    Scale(Sc);
//    Rotate(ScaleAngle+Angle);
//  }
//
//}
