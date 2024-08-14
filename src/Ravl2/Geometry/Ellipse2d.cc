// This file is part of RAVL, Recognition And Vision Library 
// Copyright (C) 2004, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here

#include "Ravl2/Geometry/Ellipse2d.hh"
//#include "Ravl2/Conic2d.hh"
//#include "Ravl2/SVD.hh"

#define DODEBUG 0
#if DODEBUG
#define ONDEBUG(x) x
#else
#define ONDEBUG(x)
#endif

namespace Ravl2 {

#if 0

  //: Fit ellipse to points.
  
  bool FitEllipse(const std::vector<Point<RealT,2>> &points,Ellipse2dC &ellipse) {
    Conic2dC conic;
    if(!FitEllipse(points,conic))
      return false;
    return conic.AsEllipse(ellipse);
  }

  //: Is point on the curve ?
  
  bool Ellipse2dC::IsOnCurve(const Point<RealT,2> &pnt) const {
    Point<RealT,2> mp = p.Inverse() * pnt;
    RealT d = mp.SumOfSqr() - 1;
    return IsSmall(d);
  }

  //: Compute various ellipse parameters.
  //!param: centre - Centre of ellipse.
  //!param: major - Size of major axis.
  //!param: minor - Size of minor axis
  //!param: angle - Angle of major axis.
  
  bool Ellipse2dC::EllipseParameters(Point<RealT,2> &centre,RealT &major,RealT &minor,RealT &angle) const {
    centre = p.Translation();
    FMatrixC<2,2> U,V;
    FVectorC<2> S = SVD(p.SRMatrix(), U, V);
    ONDEBUG(std::cerr << "U:\n"<<U<<"V:\n"<<V);
    U = U * V.T();
    ONDEBUG(cerr<<"U*V.T():\n"<<U<<endl);
    // U contains the rotation in the form:
    // cos -sin
    // sin  cos
    // hence angle can be computed as:
    angle = atan(U[1][0]/U[0][0]);
    major = S[0];
    minor = S[1];

    ONDEBUG(std::cerr << "Center=" << centre << " Major=" << major << " Minor=" << minor << " Angle=" << angle << "\n");
    return true;
  }
  
  //: Compute the size of major and minor axis.
  //!param: major - Size of major axis.
  //!param: minor - Size of minor axis
  
  bool Ellipse2dC::Size(RealT &major,RealT &minor) const {
    FVectorC<2> vec = SVD(p.SRMatrix());
    major = vec[0];
    minor = vec[1];
    return true;
  }
  

  //: Compute an ellipse from a covariance matrix, mean, and standard deviation.
  
  Ellipse2dC EllipseMeanCovariance(const Matrix<RealT,2,2> &covar,const Point<RealT,2> &mean,RealT stdDev) {
    Vector<RealT,2> dv;
    Matrix<RealT,2,2> E;
    EigenVectors(covar,E,dv);
    ONDEBUG(cerr<<"l: "<<dv<<"\nE\n"<<E<<endl);
    Matrix<RealT,2,2> d(stdDev*Sqrt(dv[0]),0,
		0,stdDev*Sqrt(dv[1]));
    // TODO:- Multiply out by hand to make it faster.
    Matrix<RealT,2,2> sr = E * d;
    return Ellipse2dC(sr,mean);
  }

#endif
  
}
