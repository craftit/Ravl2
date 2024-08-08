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
  //: Get homography
  // This returns the projection normalised so that the projective scales are both = 1
  
  Matrix<RealT,3,3> Projection2dC::Homography() const {
    Matrix<RealT,3,3> mat1(1,0,0,
		   0,1,0,
		   0,0,iz);
    Matrix<RealT,3,3> mat2(1,0,0,
		   0,1,0,
		   0,0,oz);
    Matrix<RealT,3,3> ret = mat2.Inverse() * trans * mat1;
    return ret;
  }

  FAffineC<2> Projection2dC::AffineApproximation() const {
    Matrix<RealT,3,3> htrans = Homography();
    RealT t1 = htrans[0][2] / htrans[2][2];
    RealT t2 = htrans[1][2] / htrans[2][2];
    RealT h1 = htrans[0][0] / htrans[2][2]  - t1 * htrans[2][0];
    RealT h2 = htrans[0][1] / htrans[2][2] - t1 * htrans[2][1];
    RealT h3 = htrans[1][0] / htrans[2][2] - t2 * htrans[2][0];
    RealT h4 = htrans[1][1] / htrans[2][2] - t2 * htrans[2][1];
    return FAffineC<2>(Matrix<RealT,2,2>(h1,h2,h3,h4), Vector<RealT,2>(t1,t2));
  }
  //: Get an affine approximation of this projective transform
  //!return: the affine approximation
  


  template <class DataContainerT> // assumed to be a container of Point<RealT,2>
  bool FitProjectionTempl(const DataContainerT &from,const  DataContainerT &to,Matrix<RealT,3,3> &proj)
  {
    RavlAssert(from.size() == to.size());
    unsigned neq = from.size();
    if(neq < 4) return false;
    
    // Normalise 'from' points.
    
    DataContainerT fromN;
    Matrix<RealT,3,3> fromNorm;
    Normalise(from,fromN,fromNorm);
    
    // Normalise 'to' points.
    
    DataContainerT toN;
    Matrix<RealT,3,3> toNorm;
    Normalise(to,toN,toNorm);
    
    // Build matrix.
    
    MatrixC A(neq * 2,9);
    IntT i = 0;
    for(typename DataContainerT::IteratorT toIt(toN), frIt(fromN);toIt;toIt++, frIt++) {
      const Point<RealT,2> &x = frIt.Data();
      
      SizeBufferAccessC<RealT> row1 = A[i++];
      
      RealT r = toIt.Data()[0];
      RealT c = toIt.Data()[1];
      
      row1[0] = 0;
      row1[1] = 0;
      row1[2] = 0;
      
      row1[3] = x[0] * -1;
      row1[4] = x[1] * -1;
      row1[5] = -1;
      
      row1[6] = x[0] * c;
      row1[7] = x[1] * c;
      row1[8] = c;
      
      SizeBufferAccessC<RealT> row2 = A[i++];
      
      row2[0] = x[0];
      row2[1] = x[1];
      row2[2] = 1;
      
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
  //: Fit projection to a set of points.  4 or point correspondances are required

  template <class DataContainerT>   // DataContainerT assumed to be a PairC<Point<RealT,2>>
  bool FitProjectionTempl(const DataContainerT &matchPair, Matrix<RealT,3,3> &proj)
  {
    unsigned neq = matchPair.size();
    if(neq < 4) return false;
    
    
    BufferC<Point<RealT,2>> mpBuf(matchPair.size()*2, (Point<RealT,2>*)&(matchPair[0]));
    Slice1dC<Point<RealT,2>> from(mpBuf, matchPair.size(), 0, 2);
    Slice1dC<Point<RealT,2>> to(mpBuf, matchPair.size(), 1, 2);

    return FitProjectionTempl(from, to, proj);

  }


  template <class DataContainerT> // DataContainerT must be a container of Point<RealT,2>
  Projection2dC FitProjectionTempl(const DataContainerT &org,const DataContainerT &newPos,RealT &residual) {
    
    //for some contianers it is slow to find size so cache it here    
    unsigned orgSize(org.size());

    RavlAssertMsg(orgSize == newPos.size(),"Projection2dC FitProjection(), Point arrays must have the same size.");
    // we need at least four points to fit a 2D homography
    if (orgSize < 4)
      throw ExceptionC("Sample size too small in Projection2dC. ");
    RealT zh1 = 1.0,zh2 = 1.0;
    if (orgSize == 4) {
      // FIXME:- Pick better values for zh1 and zh2 !!
      
      // initialise homography P by fitting to four point pairs, assuming that
      // bottom-right element P[2][2] is not zero.

      // Construct 8x8 matrix of linear equations
      MatrixC A(8,8);
      A.fill(0.0);
      VectorC b(8);

      // distinguish between explicit and implicit forms of point observations
      IntT i=0;
      typename DataContainerT::IteratorT it1(org);
      typename DataContainerT::IteratorT it2(newPos);
      for(;it1;it1++,it2++,i++) {
        RealT x1, y1, x2, y2;
        x1=(*it1)[0]; y1=(*it1)[1];
        x2=(*it2)[0]; y2=(*it2)[1];

        A[i*2][0] = x1*zh2; A[i*2][1] = y1*zh2; A[i*2][2] = zh1*zh2;
        A[i*2][6] = -x1*x2; A[i*2][7] = -y1*x2;
        b[i*2] = zh1*x2;
        A[i*2+1][3] = x1*zh2; A[i*2+1][4] = y1*zh2; A[i*2+1][5] = zh1*zh2;
        A[i*2+1][6] = -x1*y2; A[i*2+1][7] = -y1*y2;
        b[i*2+1] = zh1*y2;
      }
      
      // solve for solution vector
      if(!SolveIP(A,b))
        throw ExceptionNumericalC("Dependent linear equations in Projection2dC FitProjection(). ");
      residual = 0.0;
      Matrix<RealT,3,3> P(b[0], b[1], b[2],
                  b[3], b[4], b[5],
                  b[6], b[7], 1.0);
      return Projection2dC (P,zh1,zh2);
    }
    
    residual = -1.0; // Make it obvious we're not computing it!
    Matrix<RealT,3,3> P(1.0,0.0,0.0,
                0.0,1.0,0.0,
                0.0,0.0,1.0);
    FitProjectionTempl(org,newPos,P);
    return Projection2dC (P,zh1,zh2);
  }  
  //: Fit a projective transform given to the mapping between original and newPos.
  // Note: In the current version of the routine 'residual' isn't currently computed.

  template <class DataContainerT> // Assumes DataContainerT is a container of PairC<Point<RealT,2>>
  Projection2dC FitProjectionTempl(const DataContainerT &matchPairs, RealT &residual) {
    
    BufferC<Point<RealT,2>> mpBuf(matchPairs.size()*2, (Point<RealT,2>*)&(matchPairs[0]));
    Slice1dC<Point<RealT,2>> org(mpBuf, matchPairs.size(), 0, 2);
    Slice1dC<Point<RealT,2>> newPos(mpBuf, matchPairs.size(), 1, 2);    

    return FitProjectionTempl(org, newPos, residual);
  }  
  //: Fit a projective transform given to the mapping between original and newPos.
  // Note: In the current version of the routine 'residual' isn't currently computed.

  Projection2dC FitProjection(const std::vector<PairC<Point<RealT,2>> > &matchPairs, RealT &residual)
  {
    return FitProjectionTempl(matchPairs, residual);
  }

  Projection2dC FitProjection(const std::vector<Point<RealT,2>> &org,const std::vector<Point<RealT,2>> &newPos,RealT &residual)
  {
    return FitProjectionTempl(org, newPos, residual);
  }

  Projection2dC FitProjection(const std::vector<Point<RealT,2>> &org,const std::vector<Point<RealT,2>> &newPos,RealT &residual)
  {
    return FitProjectionTempl(org, newPos, residual);
  }

  //: Fit a projective transform given to the mapping between original and newPos.
  
  Projection2dC FitProjection(const std::vector<Point<RealT,2>> &org,const std::vector<Point<RealT,2>> &newPos) {
    RealT residual;
    return FitProjection(org,newPos,residual);
  }
  
  //: Fit a projective transform given to the mapping between original and newPos.
  
  Projection2dC FitProjection(const std::vector<Point<RealT,2>> &org,const std::vector<Point<RealT,2>> &newPos) {
    RealT residual;
    return FitProjection(org,newPos,residual);    
  }
  
  


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
  

  //: Read from a stream.
  
  std::istream &operator>>(std::istream &s,Projection2dC &proj) {
    s >> proj.Matrix() >> proj.IZ() >> proj.OZ();
    return s;
  }

  //: Write to a stream.
  
  std::ostream &operator<<(std::ostream &s,const Projection2dC &proj) {
    s << proj.Matrix() << ' ' << proj.IZ() << ' ' << proj.OZ();
    return s;
  }

#endif
}
