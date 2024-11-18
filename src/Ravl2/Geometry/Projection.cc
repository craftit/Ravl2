// This file is part of RAVL, Recognition And Vision Library
// Copyright (C) 2002, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here
//! date="17/10/2002"
//! author="Charles Galambos"

#include "Ravl2/Geometry/Projection.hh"

namespace Ravl2
{
  static_assert(PointTransform<Projection<float, 2>>, "Projection<int,2> does not satisfy PointTransform");


#if 0

  //: Fit a projective transform given to the mapping between original and newPos with weighting for points.
  
  Projection2dC FitProjection(const std::vector<Point<RealT,2>> &org,const std::vector<Point<RealT,2>> &newPos,const std::vector<RealT> &weight) {
    RealT zh1 = 1.0,zh2 = 1.0;
    Matrix<RealT,3,3> P(1.0,0.0,0.0,
                0.0,1.0,0.0,
                0.0,0.0,1.0);
    FitProjection(org,newPos,weight,P);
    return Projection2dC (P,zh1,zh2); 
  }
  
  //: Fit projection to a set of points.  4 or point correspondences are required

  bool FitProjection(const std::vector<Point<RealT,2>> &from,const std::vector<Point<RealT,2>> &to,const std::vector<RealT> &weight,Matrix<RealT,3,3> &proj) {
    RavlAssert(from.size() == to.size());
    RavlAssert(from.size() == weight.size());
    unsigned neq = from.size();
    if(neq < 4) return false;
    
    // Normalise 'from' points.
    
    std::vector<Point<RealT,2>> fromN;
    Matrix<RealT,3,3> fromNorm;
    Normalise(from,fromN,fromNorm);
    
    // Normalise 'to' points.
    
    std::vector<Point<RealT,2>> toN;
    Matrix<RealT,3,3> toNorm;
    Normalise(to,toN,toNorm);
    
    // Compute normalisation for weights.
    
    RealT meanWeight = weight.Sum() / static_cast<RealT>(weight.size());
    if(meanWeight <= 0)
      meanWeight = 1;
    
    // Build matrix.
    
    MatrixC A(neq * 2,9);
    IntT i = 0;
    for(SArray1dIter3C<Point<RealT,2>,Point<RealT,2>,RealT> it(toN,fromN,weight);it;it++) {
      const Point<RealT,2> &x = it.data<1>();
      RealT w = it.data<2>() / meanWeight; // Normalise weight.
      SizeBufferAccessC<RealT> row1 = A[i++];
      
      RealT r = it.data<0>()[0] * w;
      RealT c = it.data<0>()[1] * w;
      
      row1[0] = 0;
      row1[1] = 0;
      row1[2] = 0;
      
      row1[3] = x[0] * -1 * w;
      row1[4] = x[1] * -1 * w;
      row1[5] = -w;
      
      row1[6] = x[0] * c;
      row1[7] = x[1] * c;
      row1[8] = c;
      
      SizeBufferAccessC<RealT> row2 = A[i++];
      
      row2[0] = x[0] * w;
      row2[1] = x[1] * w;
      row2[2] = w;
      
      row2[3] = 0;
      row2[4] = 0;
      row2[5] = 0;
      
      row2[6] = x[0] * -r;
      row2[7] = x[1] * -r;
      row2[8] = -r;
    }
    
    // Should check the rank of A?
    
    VectorC v;
    if(!LeastSquaresEq0Mag1(A,v))
      return false;
    //cerr << "A=" << A << " V=" << v << "\n";
    Matrix<RealT,3,3> mat(v[0],v[1],v[2],
		  v[3],v[4],v[5],
		  v[6],v[7],v[8]);
    
    toNorm.InvertIP();
    proj =  toNorm * mat * fromNorm;
    return true;    
  }

#endif
  template class Projection<float, 2>;

}// namespace Ravl2
