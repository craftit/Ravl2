// This file is part of RAVL, Recognition And Vision Library
// Copyright (C) 2001, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here

#pragma once

#include "Ravl2/Assert.hh"
#include "Ravl2/Concepts.hh"
#include "Ravl2/Geometry/Affine.hh"
#include "Ravl2/Math/LeastSquares.hh"

namespace Ravl2
{
  //! Compute the affine normalisation transform for a mean and scale.
  template <typename RealT, unsigned N>
   requires std::is_floating_point_v<RealT> && (N > 0)
  Affine<RealT,N> meanScaleToAffine(const Point<RealT,N> &mean, RealT scale)
  {
    Matrix<RealT,N,N> normMat = xt::zeros<RealT>({N,N});
    for(unsigned i = 0; i < N; i++)
      normMat(i,i) = scale;
    return Affine<RealT,N>(normMat,-mean * scale);
  }


  //! Fit an 2d affine transform from mapping of 3 points.
  template <typename RealT>
   requires std::is_floating_point_v<RealT>
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

  //! Fit a general affine transform from a set of points.
  //! There needs to be at least N+1 points to fit an N-dimensional affine transform.
  template <typename RealT, unsigned N, SimpleContainer ContainerOfPointAT, SimpleContainer ContainerOfPointBT>
   requires std::is_floating_point_v<RealT> && (N > 0)
  RealT fit(Affine<RealT, N> &affine, const ContainerOfPointAT &to, const ContainerOfPointBT &from)
  {
    RavlAssertMsg(from.size() == to.size(),"Affine2dC FitAffine(), Point arrays must have the same size.");

    auto samples = from.size();
    if ( samples < (N+1) ) {
      throw std::runtime_error("Sample size too small in fit(Affine...) ");
    }
    // Normalise 'from' points.
    auto [fromMean,fromScale] =  meanAndScale<RealT,N>(from);
    auto [toMean,toScale] =  meanAndScale<RealT,N>(to);

    Tensor<RealT,2> A({samples,N+1});
    std::array<VectorT<RealT>,N> eqs;
    for(auto &eq : eqs) {
      eq = xt::empty<RealT>({samples});
    }
    size_t i = 0;

    auto toIt = to.begin();
    auto toEnd = to.end();
    auto frIt = from.begin();
    auto frEnd = from.end();

    for(;toIt != toEnd && frIt!=frEnd;++toIt,++frIt,++i) {
      Point<RealT,N> p1 = normalisePoint<RealT>(*toIt,toMean,toScale);
      Point<RealT,N> p2 = normalisePoint<RealT>(*frIt,fromMean,fromScale);

      for(size_t j = 0; j < N; j++) {
	A(i,j) = p2[j];
      }
      A(i,N) = 1;

      for(size_t j = 0; j < N; ++j) {
	eqs[j](i) = p1[j];
      }
    }
    Matrix<RealT,N,N> sr;
    Vector<RealT,N> tr;
    RealT residual = 0;
    if(A.shape(0) == A.shape(1)) {
      for(size_t j = 0; j < N; j++) {
	auto solA = xt::linalg::solve(A, eqs[j]);
	for(size_t k = 0; k < N; k++) {
	  sr(j, k) = solA[k];
	}
	tr[j] = solA[N];
      }
    } else {
      for(size_t j = 0; j < N; j++) {
	auto [solA, residualA, rankA, sA] = xt::linalg::lstsq(A, eqs[j]);
	for(size_t k = 0; k < N; k++) {
	  sr(j, k) = solA[k];
	}
	tr[j] = solA[N];
	residual += xt::sum(residualA)();
      }
    }

    auto fromNorm = meanScaleToAffine<RealT,2>(fromMean,fromScale);
    auto toNorm = *inverse(meanScaleToAffine<RealT,2>(toMean,toScale));

    // Compose result
    affine = toNorm(Affine<RealT,N>(sr,tr))(fromNorm);
    return residual;
  }

}// namespace Ravl2