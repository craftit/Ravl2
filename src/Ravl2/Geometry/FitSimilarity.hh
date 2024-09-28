//
// Created by charles galambos on 28/09/2024.
//

#pragma once

#include "Ravl2/Assert.hh"
#include "Ravl2/Concepts.hh"
#include "Ravl2/Geometry/Affine.hh"
#include "Ravl2/Math/LeastSquares.hh"

namespace Ravl2
{

  //! @brief Fit a similarity transformation between two sets of points.
  //! See 'Least-Squares Estimation of Transformation Parameters Between Two Point Patterns' by
  //! Shinji Umeyama.  IEEE Transactions on Pattern Analysis and Machine Intelligence Vol 13, No 4
  //! April 1991. Page 376
  //! @param rotation - The affine transformation to be computed.
  //! @param translation - The translation to be computed.
  //! @param scale - The scale to be computed.
  //! @param points1 - The first set of points.
  //! @param points2 - The second set of points.
  //! @param forceUnitScale - If true, the scale is forced to 1.
  //! @return True if the transformation was computed.

  template<typename RealT,size_t N, SimpleContainer ContainerOfPointAT, SimpleContainer ContainerOfPointBT>
  bool fitSimilarity(Matrix<RealT,N,N> &rotation,
		     Vector<RealT,N> &translation,
		     RealT &scale,
		     const ContainerOfPointAT &points1,
		     const ContainerOfPointBT &points2,
		     bool forceUnitScale
  )
  {
    // Compute the means.
    RavlAssertMsg(points1.size() == points2.size(),"fitSimilarity(), Point arrays must have the same size.");

    RealT n = points1.size();
    if(n < 2) {
      return false;
    }
    Point<RealT,N> mean1 = xt::zeros<RealT>({N});
    Point<RealT,N> mean2 = xt::zeros<RealT>({N});

    for(auto x : points1) {
      mean1 += x;
    }
    for(auto x : points2) {
      mean2 += x;
    }
    SPDLOG_INFO("mean1: {}", mean1);
    SPDLOG_INFO("mean2: {}", mean2);

    mean1 /= n;
    mean2 /= n;

    // Compute the covariance matrix.
    auto points1It = points1.begin();
    auto points2It = points2.begin();
    auto points1End = points1.end();
    auto points2End = points2.end();
    RealT ps1 = 0,ps2 = 0;
    Matrix<RealT,N,N> covar = xt::zeros<RealT>({N,N});
    for(;points1It != points1End && points2It != points2End;++points1It,++points2It) {
      Point<RealT,N> p1 = *points1It - mean1;
      Point<RealT,N> p2 = *points2It - mean2;
      ps1 += xt::sum(xt::square(p1))();
      ps2 += xt::sum(xt::square(p2))();
      for(unsigned i = 0;i < N;++i) {
	for(unsigned j = 0;j < N;++j) {
	  covar(i,j) += p1[j] * p2[i];
	}
      }
    }

    if(isNearZero(ps1) || isNearZero(ps2)) {
      return false;
    }

    // Compute the scaling.
    scale = std::sqrt(ps2/ps1);

    // Compute the rotation from the covariance matrix.
    covar /= n;

    auto [u, d, v] = xt::linalg::svd(covar, true, true);

    Matrix<RealT,N,N> s = xt::eye<RealT>(N);

    // Correct mirroring.
    if((xt::linalg::det(u) * xt::linalg::det(v)) < 0) {
      s(N-1,N-1) = -1;
      d[N-1] *= -1;
    }

    rotation = xt::linalg::dot(xt::linalg::dot(u, s),v);

    // Compute the translation.
    if(forceUnitScale) {
      translation = mean2 - rotation * mean1;
    } else {
      translation = mean2 - rotation * mean1 * scale;
    }

    return true;
  }

  //! @brief Fit an affine similarity transformation between two sets of points.
  //! @param affine - The affine transformation to be computed.
  //! @param points1 - The first set of points.
  //! @param points2 - The second set of points.
  //! @param forceUnitScale - If true, the scale is forced to 1.
  //! @return True if the transformation was computed.
  template<typename RealT,unsigned N, SimpleContainer ContainerOfPointAT, SimpleContainer ContainerOfPointBT>
  bool fitSimilarity(Affine<RealT,N> &affine,
			   const ContainerOfPointAT &points1,
			   const ContainerOfPointBT &points2,
			   bool forceUnitScale = false)
  {
    RealT scale = 0;
    if(!fitSimilarity(affine.SRMatrix(), affine.Translation(), scale, points1, points2, forceUnitScale)) {
      return false;
    }
    if(!forceUnitScale) {
      affine.SRMatrix() *= scale;
    }
    return true;
  }





} // Ravl2
