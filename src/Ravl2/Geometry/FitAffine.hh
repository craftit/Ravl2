//
// Created by charles on 12/09/24.
//

#pragma once

#include "Ravl2/Assert.hh"
#include "Ravl2/Geometry/Affine.hh"
#include "Ravl2/Math/LeastSquares.hh"

namespace Ravl2
{
  //! Compute the affine normalisation transform for a mean and scale.
  template <typename RealT, unsigned N>
  Affine<RealT,N> meanScaleToAffine(const Point<RealT,N> &mean, RealT scale)
  {
    Matrix<RealT,N,N> normMat = xt::zeros<RealT>({N,N});
    for(unsigned i = 0; i < N; i++)
      normMat(i,i) = scale;
    return Affine<RealT,N>(normMat,-mean * scale);
  }


  //! Fit affine transform from mapping of 3 points.
  template <typename RealT>
  bool fit(Affine<RealT, 2> &affine,
           const Point<RealT, 2> &p1b, const Point<RealT, 2> &p1a,
           const Point<RealT, 2> &p2b, const Point<RealT, 2> &p2a,
           const Point<RealT, 2> &p3b, const Point<RealT, 2> &p3a)
  {
    // Form equations

    Matrix<RealT, 3, 3> A({{p1a[0], p1a[1], 1.0},
                           {p2a[0], p2a[1], 1.0},
                           {p3a[0], p3a[1], 1.0}});

    Vector<RealT, 3> b({p1b[0], p2b[0], p3b[0]});
    Vector<RealT, 3> c({p1b[1], p2b[1], p3b[1]});

    auto sab = xt::linalg::solve(A, b);
    auto sac = xt::linalg::solve(A, c);

    affine.Translation()[0] = sab[2];
    affine.Translation()[1] = sac[2];
    affine.SRMatrix() = Matrix<RealT,2,2>({{sab[0], sab[1]},{sac[0], sac[1]}});

    return true;
  }

  template <typename RealT, typename ContainerOfPoint2T>
  RealT fit(Affine<RealT, 2> &affine,const ContainerOfPoint2T &to,const ContainerOfPoint2T &from)
  {
    RavlAssertMsg(from.size() == to.size(),"Affine2dC FitAffine(), Point arrays must have the same size.");

    auto samples = from.size();
    if ( samples < 3 ) {
      throw std::runtime_error("Sample size too small in fit(Affine...) ");
    }
    RealT residual = 0;
    // Normalise 'from' points.
    auto [fromMean,fromScale] =  meanAndScale<RealT,2>(from);
    auto [toMean,toScale] =  meanAndScale<RealT,2>(to);

    Tensor<RealT,2> A({samples,3});
    VectorT<RealT> b = xt::empty<RealT>({samples});
    VectorT<RealT> c = xt::empty<RealT>({samples});
    size_t i = 0;

    auto toIt = to.begin();
    auto toEnd = to.end();
    auto frIt = from.begin();
    auto frEnd = from.end();

    for(;toIt != toEnd && frIt!=frEnd;++toIt,++frIt,++i) {
      RealT x1=((*frIt)[0] - fromMean[0]) * fromScale;
      RealT y1=((*frIt)[1] - fromMean[1]) * fromScale;
      RealT x2=((*toIt)[0] - toMean[0]) * toScale;
      RealT y2=((*toIt)[1] - toMean[1]) * toScale;

      A(i,0) = x1;
      A(i,1) = y1;
      A(i,2) = 1;
      b(i) = x2;
      c(i) = y2;
    }

    VectorT<RealT> sab;
    VectorT<RealT> sac;

    if(A.shape(0) == A.shape(1)) {
      // solve for solution vector
      sab = xt::linalg::solve(A, b);
      sac = xt::linalg::solve(A, c);
    } else {
      // solve for least squares solution
      auto [solA, residualA, rankA, sA] = xt::linalg::lstsq(A, b);
      auto [solB, residualB, rankB, sB] = xt::linalg::lstsq(A, c);
      sab = solA;
      sac = solB;
      residual += xt::sum(residualA + residualB)();
    }

    Matrix<RealT,3,3> sr({{sab[0],sab[1]},
                          {sac[0],sac[1]}});
    Vector<RealT,2> tr({sab[2],sac[2]});

    auto fromNorm = meanScaleToAffine<RealT,2>(fromMean,fromScale);
    auto toNorm = *inverse(meanScaleToAffine<RealT,2>(toMean,toScale));

    // Compose result
    affine = toNorm(Affine<RealT,2>(sr,tr))(fromNorm);
    return residual;
  }

}// namespace Ravl2
