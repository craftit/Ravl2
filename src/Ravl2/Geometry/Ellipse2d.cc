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
  //: Compute various ellipse parameters.
  //!param: centre - Centre of ellipse.
  //!param: major - Size of major axis.
  //!param: minor - Size of minor axis
  //!param: angle - Angle of major axis.
  
  bool Ellipse2dC::EllipseParameters(Point<RealT,2> &centre,RealT &major,RealT &minor,RealT &angle) const
{
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


#endif
  
}
