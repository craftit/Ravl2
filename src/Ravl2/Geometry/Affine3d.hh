// This file is part of RAVL, Recognition And Vision Library 
// Copyright (C) 2007, OmniPerception Ltd.
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here
#ifndef RAVL_AFFINE3D_HEADER
#define RAVL_AFFINE3D_HEADER 1
//////////////////////////////////////////////////////
//! file="Ravl/Math/Geometry/Euclidean/3D/Affine3d.hh"
//! lib=RavlMath
//! userlevel=Normal
//! author="Charles Galambos"
//! date="17/3/2007"
//! docentry="Ravl.API.Math.Geometry.3D"
//! rcsid="$Id$"

#include "Ravl2/Geometry/Affine.hh"
//#include "Ravl/Matrix3d.hh"
//#include "Ravl/Vector3d.hh"
//#include "Ravl/Point3d.hh"
//#include "Ravl/Point2d.hh"

namespace RavlN {
//  template<class DataT> class SArray1dC;
//  template<class DataT> class DListC;
//  class Point3dC;
//  class Point2dC;
//  class PointSet3dC;
//  class Matrix3dC;
//
//  //! userlevel=Normal
//  //: 3-D Affine transformation
//
//  // This class is used to apply a (12-parameter) 3-D affine transformation.  Note
//  // that it transforms the point w.r.t. the coordinate system; it does
//  // <b>not</b> transform the coordinate system itself.
//
//  class Affine3dC
//    : public FAffineC<3>
//  {
//  public:
//    inline Affine3dC()
//      : FAffineC<3>()
//    {}
//    //: Construct an identity transformation.
//
//    inline Affine3dC(const FAffineC<3> &Oth)
//      : FAffineC<3>(Oth)
//    {}
//    //: Construct from base class
//
//    inline Affine3dC(const Matrix3dC &SR, const Vector3dC &T)
//      : FAffineC<3>(SR,T)
//    {}
//    //: Constructor from scaling/rotation matrix and translation vector.
//
//    inline FAffineC<3> Inverse() const {
//      FAffineC<3> ret;
//      Matrix3dC(SR).Invert(ret.SRMatrix());
//      Mul(ret.SRMatrix(),T,ret.Translation());
//      ret.Translation() *= -1;
//      return ret;
//    }
//
//  };
//
//  Affine3dC FitAffine(const SArray1dC<Point3dC> &orig,const SArray1dC<Point3dC> &newPos,RealT &residual);
//  //: Fit an affine transformation
//
//  Affine3dC FitAffine(const SArray1dC<Point2dC> &orig,const SArray1dC<Point3dC> &newPos,RealT &residual);
//  //: Fit points on a 2d plane (this assumes the z position is always zero) to a 3d position.
//
//  Affine3dC FitAffineDirection(const SArray1dC<Point3dC> &points,const SArray1dC<Vector3dC> &directions);
//  //: Fit an affine transformation given some directions and positions.
//
//  inline Affine3dC FitAffine(const SArray1dC<Point3dC> &orig,const SArray1dC<Point3dC> &newPos) {
//    RealT residual;
//    return FitAffine(orig,newPos,residual);
//  }
//  //: Fit an affine transformation
//
//  inline Affine3dC FitAffine(const SArray1dC<Point2dC> &orig,const SArray1dC<Point3dC> &newPos) {
//    RealT residual;
//    return FitAffine(orig,newPos,residual);
//  }
//  //: Fit points on a 2d plane (this assumes the z position is always zero) to a 3d position.
//
//  bool FitSimilarity(const SArray1dC<Point3dC> &points1,
//                     const SArray1dC<Point3dC> &points2,
//                     Matrix3dC &rotation,
//                     Vector3dC &translation,
//                     RealT &scaling,
//                     bool forceUnitScale = false
//                     );
//  //: Least squares fit of a similarity transform between the two point sets.
//  // If 'forceUnitScale' is true then unit scaling will be assumed, though
//  // the measured scale will still be assigned to 'scaling'.  This may
//  // effect the computed translation in particular. The transform will
//  // map points1 to points2
//
//  bool FitSimilarity(const SArray1dC<Point3dC> &points1,
//                     const SArray1dC<Point3dC> &points2,
//                     Affine3dC &transform,
//                     bool forceUnitScale = false
//                     );
//  //: Least squares fit of a similarity transform between the two point sets.
//  // If 'forceUnitScale' is true then unit scaling will be assumed.
//  // The transform will map points1 to points2
//
//  inline bool FitIsometry(const SArray1dC<Point3dC> &points1,
//                          const SArray1dC<Point3dC> &points2,
//                          Affine3dC &transform)
//  { return FitSimilarity(points1,points2,transform,true); }
//  //: Least squares fit of a isometry transform between the two point sets.
//
  
}

#endif
