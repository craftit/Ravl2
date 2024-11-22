// This file is part of RAVL, Recognition And Vision Library
// Copyright (C) 2001, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here

#pragma once

#include "Ravl2/Concepts.hh"
#include "Ravl2/Geometry/Projection.hh"
#include "Ravl2/Math/LeastSquares.hh"

namespace Ravl2
{
  template <typename RealT, unsigned N>
  Matrix<RealT,N+1,N+1> projectiveNormalisationMatrix(Point<RealT,N> mean, RealT scale)
  {
    Matrix<RealT,N+1,N+1> normMat = Matrix<RealT,N+1,N+1>::Identity() * scale;
    for(unsigned i = 0; i < N; i++)
      normMat(i,N) = -mean[i] * scale;
    normMat(N,N) = 1;
    return normMat;
  }


  //: Fit projection to a set of points.  4 or point correspondences are required

  template <typename RealT, SimpleContainer DataContainer1T,SimpleContainer DataContainer2T> // assumed to be a container of Point<RealT,2>
   requires std::is_floating_point_v<RealT>
  bool fitProjection(Matrix<RealT,3,3> &proj, const DataContainer1T &to, const DataContainer2T &from)
  {
    RavlAssert(from.size() == to.size());
    auto neq = from.size();
    if(neq < 4) return false;

    // Normalise 'from' points.
    auto [fromMean,fromScale] =  meanAndScale<RealT,2>(from);
    auto [toMean,toScale] =  meanAndScale<RealT,2>(to);

    // Build matrix.
    MatrixT<RealT> A(neq * 2, 9);

    IndexT i = 0;
    auto toIt = to.begin();
    auto toEnd = to.end();
    auto frIt = from.begin();
    auto frEnd = from.end();

    for(;toIt != toEnd && frIt!=frEnd;++toIt,++frIt) {
      const Point<RealT,2> x = normalisePoint<RealT,2>(*frIt,fromMean,fromScale);
      const Point<RealT,2> y = normalisePoint<RealT,2>(*toIt,toMean,toScale);

      //auto row1 = A[i++];
      auto row1 = A.row(i++);

      RealT r = y[0];
      RealT c = y[1];

      row1[0] = 0;
      row1[1] = 0;
      row1[2] = 0;

      row1[3] = x[0] * -1;
      row1[4] = x[1] * -1;
      row1[5] = -1;

      row1[6] = x[0] * c;
      row1[7] = x[1] * c;
      row1[8] = c;

      //auto row2 = A[i++];
      auto row2 = A.row(i++);

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

    VectorT<RealT> v;
    if(!LeastSquaresEq0Mag1(A,v))
      return false;
    //cerr << "A=" << A << " V=" << v << "\n";
    Matrix<RealT,3,3> mat({{v[0],v[1],v[2]},
			   {v[3],v[4],v[5]},
			   {v[6],v[7],v[8]}});

    auto fromNorm = projectiveNormalisationMatrix<RealT,2>(fromMean,fromScale);
    auto toNorm = inverse(projectiveNormalisationMatrix<RealT,2>(toMean,toScale));
    if(!toNorm.has_value()) {
      SPDLOG_ERROR("Failed to invert normalisation matrix. {} from mean {} scale {} ",projectiveNormalisationMatrix<RealT,2>(toMean,toScale),toMean,toScale);
      return false;
    }
    proj =  toNorm.value() * mat * fromNorm;
    return true;
  }


  //: Fit a projective transform given to the mapping between original and newPos.
  template <typename RealT,SimpleContainer DataContainer1T,SimpleContainer DataContainer2T> // DataContainer1/2T must be a container of Point<RealT,2>
   requires std::is_floating_point_v<RealT>
  bool fit(Projection<RealT,2> &proj, const DataContainer2T &newPos, const DataContainer1T &org)
  {

    //for some containers it is slow to find size so cache it here
    auto orgSize = org.size();

    RavlAssertMsg(orgSize == newPos.size(),"Projection2dC FitProjection(), Point arrays must have the same size.");
    // we need at least four points to fit a 2D homography
    if (orgSize < 4) {
      //throw std::runtime_error("Sample size too small in Projection2dC. ");
      return false;
    }
    RealT zh1 = 1.0,zh2 = 1.0;
    if (orgSize == 4) {
      // FIXME:- Pick better values for zh1 and zh2 !!

      // initialise homography P by fitting to four point pairs, assuming that
      // bottom-right element P[2][2] is not zero.

      // Construct 8x8 matrix of linear equations
      Matrix<RealT,8,8> A = Matrix<RealT,8,8>::Zero();
      Vector<RealT,8> b = Vector<RealT,8>::Zero();

      // distinguish between explicit and implicit forms of point observations
      IndexT i=0;
      auto it1 = org.begin();
      const auto it1end = org.end();
      auto it2 = newPos.begin();
      const auto it2end = newPos.end();
      for(;it1 != it1end && it2 != it2end;++it1,++it2,i++) {
	RealT x1, y1, x2, y2;
	x1=(*it1)[0]; y1=(*it1)[1];
	x2=(*it2)[0]; y2=(*it2)[1];

	A(i*2,0) = x1*zh2; A(i*2,1) = y1*zh2; A(i*2,2) = zh1*zh2;
	A(i*2,6) = -x1*x2; A(i*2,7) = -y1*x2;
	b[i*2] = zh1*x2;
	A(i*2+1,3) = x1*zh2; A(i*2+1,4) = y1*zh2; A(i*2+1,5) = zh1*zh2;
	A(i*2+1,6) = -x1*y2; A(i*2+1,7) = -y1*y2;
	b[i*2+1] = zh1*y2;
      }

      // solve for solution vector
      try {
	//auto sb = xt::linalg::solve(A, b);
        auto solver = A.bdcSvd(Eigen::ComputeThinU | Eigen::ComputeThinV);
        auto sb = solver.solve(b);
	Matrix<RealT, 3, 3> P({{sb[0], sb[1], sb[2]},
			       {sb[3], sb[4], sb[5]},
			       {sb[6], sb[7], 1.0}}
			     );
	proj = Projection<RealT, 2>(P, zh1, zh2);
      } catch (const std::exception &e) {
	SPDLOG_WARN("FitProjection() failed to solve for homography: {}", e.what());
	return false;
      }
      return true;
    }

    Matrix<RealT,3,3> P = Matrix<RealT,3,3>::Identity();
    if(!fitProjection(P,newPos,org))
      return false;
    proj = Projection<RealT,2> (P,zh1,zh2);
    return true;
  }

}
