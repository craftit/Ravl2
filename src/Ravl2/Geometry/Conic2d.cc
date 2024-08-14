// This file is part of RAVL, Recognition And Vision Library 
// Copyright (C) 2004, OmniPerception Ltd
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here

#include "Ravl2/Geometry/Conic2d.hh"

#define DODEBUG  0
#if DODEBUG
#define ONDEBUG(x) x
#else
#define ONDEBUG(x)
#endif

namespace Ravl2 {

  //: Fit a conic to a set of points.
  


#if 0

  //: Represent conic as an ellipse.
  //! @param  ellipse - Ellipse into which representation is written.
  //!return: true if conversion is possible.
  // Returns false if conic is a hyperbola or is
  // otherwise degenerate.
  
  bool Conic2dC::AsEllipse(Ellipse2dC &ellipse) const {
    // Ellipse representation is transformation required to transform unit
    // circle into ellipse.  This is the inverse of the "square root" of
    // Euclidean matrix representation
    Matrix<RealT,2,2> euc; // Euclidean representation of elipse equation
    Point<RealT,2> centre;
    // Separate projective ellipse representation into Euclidean + translation
    if(!ComputeEllipse(centre,euc))
      return false;
    ONDEBUG(std::cerr << "Euclidean ellipse is:\n" << euc << endl);

    // Then decompose to get orientation and scale
    FVectorC<2> lambda;
    FMatrixC<2,2> E;
    EigenVectors(euc,E,lambda);
    // lambda now contains inverted squared *minor* & *major* axes respectively
    // (N.B.: check: E[0][1] *MUST* have same sign as ellipse orientation)
    ONDEBUG(std::cerr << "Eigen decomp is:\n" << E << "\n" << lambda << endl);

    Matrix<RealT,2,2> scale(0,                 1/Sqrt(lambda[0]),
                    1/Sqrt(lambda[1]), 0                );
    // Columns are swapped in order to swap x & y to compensate for eigenvalue
    // ordering.  I.e. so that [1,0] on unit circle gets mapped to
    // end of major axis rather than minor axis.

    // TODO:- Multiply out by hand to make it faster.
    ellipse = Ellipse2dC(E * scale, centre);
    ONDEBUG(cerr<<"Ellipse2dC:\n"<<ellipse<<endl);
    ONDEBUG(cerr<<"[1,0] on unit circle goes to "<<ellipse.Projection()*(Vector<RealT,2>(1,0))<<" on ellipse"<<endl;);
    return true;
  }
  
  //: Compute various ellipse parameters.
  //! @param  centre - Centre of ellipse.
  //! @param  major - Size of major axis.
  //! @param  minor - Size of minor axis
  //! @param  angle - Angle of major axis.
  
  bool Conic2dC::EllipseParameters(Point<RealT,2> &centre,RealT &major,RealT &minor,RealT &angle) const {
    Matrix<RealT,2,2> t;
    if(!ComputeEllipse(centre,t))
      return false;
    angle = atan2(-2*t[0][1],-t[0][0]+t[1][1])/2;
    RealT cosa=Cos(angle);
    RealT sina=Sin(angle);
    RealT w = 2*t[0][1]*cosa*sina;
    major =Sqrt(1.0/(t[0][0]*cosa*cosa+w+t[1][1]*sina*sina));
    minor =Sqrt(1.0/(t[0][0]*sina*sina-w+t[1][1]*cosa*cosa));
    ONDEBUG(std::cerr << "Center=" << centre << " Major=" << major << " Minor=" << minor << " Angle=" << angle << "\n");
    return true;
    
  }
  
  
  //: Fit a conic to a set of points.
  
  Conic2dC FitConic(const Array<Point<RealT,2>,1> &points)
  { return FitConic(const_cast<Array<Point<RealT,2>,1> &>(points).SArray1d(true)); } 


  //: Fit ellipse to points.
  
  bool FitEllipse(const std::vector<Point<RealT,2>> &points,Conic2dC &conic) {
    if(points.size() < 5)
      return false;
    
    // Normalise points.
    
    Matrix<RealT,3,3> Hi;
    std::vector<Point<RealT,2>> normPoints;
    Normalise(points,normPoints,Hi);
    
    
    // Fill in 'D'
    MatrixC D1(points.size(),3);
    for(BufferAccessIter2C<Point<RealT,2>,BufferAccessC<RealT> > it(normPoints,D1);it;it++) {
      const Point<RealT,2> &l = it.data<0>();
      it.data<1>()[0] = sqr(l[0]);
      it.data<1>()[1] = l[0] * l[1];
      it.data<1>()[2] = sqr(l[1]);
    }
    MatrixC D2(points.size(),3);
    for(BufferAccessIter2C<Point<RealT,2>,BufferAccessC<RealT> > it(normPoints,D2);it;it++) {
      const Point<RealT,2> &l = it.data<0>();
      it.data<1>()[0] = l[0];
      it.data<1>()[1] = l[1];
      it.data<1>()[2] = 1;
    }
    
    MatrixC S1 = D1.TMul(D1);
    MatrixC S2 = D1.TMul(D2);
    MatrixC S3 = D2.TMul(D2);
    S3.InverseIP();
    MatrixC T = S3.MulT(S2) * -1;
    MatrixC M = S1 + S2 * T;
    M = MatrixC(M[2][0]/2,M[2][1]/2,M[2][2]/2,
		-M[1][0],-M[1][1],-M[1][2],
		M[0][0]/2,M[0][1]/2,M[0][2]/2);
    
    ONDEBUG(std::cerr << "M=" << M << "\n");
    EigenValueC<RealT> evs(M);
    MatrixC eD;
    evs.getD(eD);
    ONDEBUG(std::cerr << "D=" << eD << "\n");
    MatrixC ev = evs.EigenVectors();
    ONDEBUG(std::cerr << "ev=" << ev << "\n");
    VectorC cond = 
      VectorC(ev.SliceRow(0)) * VectorC(ev.SliceRow(2)) * 4 
      - VectorC(ev.SliceRow(1)) * VectorC(ev.SliceRow(1));
    
    //cerr << "Cond=" << cond << "\n";
    VectorC a1 = ev.SliceColumn(cond.IndexOfMax());
    VectorC a = a1.Append(T * a1);

    // Undo normalisation.
#if 1
#if RAVL_COMPILER_VISUALCPP
    Conic2dC Cr(static_cast<const FVectorC<6> &>(a));
#else
    Conic2dC Cr(static_cast<const Vector<RealT,6> &>(a));
#endif
    Matrix<RealT,3,3> nC = Hi.TMul(Cr.C()) * Hi;
    conic = Conic2dC(nC);
#else
    conic = Conic2dC(a);
#endif
    
    return true;
  }
#endif

}
