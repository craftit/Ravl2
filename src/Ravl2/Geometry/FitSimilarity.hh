//
// Created by charles galambos on 28/09/2024.
//

#pragma once

#include "Ravl2/Assert.hh"
#include "Ravl2/Concepts.hh"
#include "Ravl2/Geometry/Affine.hh"
#include "Ravl2/Geometry/Isometry3.hh"
#include "Ravl2/Geometry/Plane3PVV.hh"
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
  //! @param pointsFrom - The first set of points.
  //! @param pointsTo - The second set of points.
  //! @param forceUnitScale - If true, the scale is forced to 1.
  //! @return True if the transformation was computed.

  template<typename RealT,IndexSizeT N, SimpleContainer ContainerOfPointAT, SimpleContainer ContainerOfPointBT>
  bool fitSimilarity(Matrix<RealT,N,N> &rotation,
		     Vector<RealT,N> &translation,
		     RealT &scale,
		     const ContainerOfPointAT &pointsTo,
		     const ContainerOfPointBT &pointsFrom,
		     bool forceUnitScale
  )
  {
    // Compute the means.
    RavlAssertMsg(pointsFrom.size() == pointsTo.size(), "fitSimilarity(), Point arrays must have the same size.");

    RealT n = RealT(pointsFrom.size());
    if(n < 2) {
      return false;
    }
    Point<RealT,N> mean1 = Point<RealT,N>::Zero();
    Point<RealT,N> mean2 = Point<RealT,N>::Zero();

    for(auto x : pointsFrom) {
      mean1 += x;
    }
    for(auto x : pointsTo) {
      mean2 += x;
    }

    mean1 /= n;
    mean2 /= n;

    // Compute the covariance matrix.
    auto points1It = pointsFrom.begin();
    auto points2It = pointsTo.begin();
    auto points1End = pointsFrom.end();
    auto points2End = pointsTo.end();
    RealT ps1 = 0,ps2 = 0;
    Matrix<RealT,N,N> covar = Matrix<RealT,N,N>::Zero();
    for(;points1It != points1End && points2It != points2End;++points1It,++points2It) {
      Point<RealT,N> p1 = *points1It - mean1;
      Point<RealT,N> p2 = *points2It - mean2;
      ps1 += p1.squaredNorm();
      ps2 += p2.squaredNorm();
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

    //auto [u, d, v] = xt::linalg::svd(covar, true, true);
    //Eigen::template JacobiSVD<Matrix<RealT, N, N>, Eigen::ComputeFullU|Eigen::ComputeFullV> svd(covar);
    auto svd = covar.jacobiSvd(Eigen::ComputeFullU | Eigen::ComputeFullV);
    auto u = svd.matrixU();
    auto d = svd.singularValues();
    auto v = svd.matrixV();


    Matrix<RealT,N,N> s = Matrix<RealT,N,N>::Identity();

    // Correct mirroring.
    if((u.determinant() * v.determinant()) < 0) {
      s(N-1,N-1) = -1;
      d[N-1] *= -1;
    }

    rotation = u * s * v.transpose();

    // Compute the translation.
    if(forceUnitScale) {
      translation = mean2 - rotation * mean1;
    } else {
      translation = mean2 - (rotation * mean1) * scale;
    }

    return true;
  }

  //! @brief Fit an affine similarity transformation between two sets of points.
  //! Preserves angles, and distances with a scaling.
  //! @param affine - The affine transformation to be computed.
  //! @param pointsTo - The first set of points.
  //! @param pointsFrom - The second set of points.
  //! @param forceUnitScale - If true, the scale is forced to 1.
  //! @return True if the transformation was computed.
  template<typename RealT,unsigned N, SimpleContainer ContainerOfPointAT, SimpleContainer ContainerOfPointBT>
  bool fitSimilarity(Affine<RealT,N> &affine,
			   const ContainerOfPointAT &pointsTo,
			   const ContainerOfPointBT &pointsFrom)
  {
    RealT scale = 0;
    if(!fitSimilarity<RealT,N>(affine.SRMatrix(), affine.translation(), scale, pointsTo, pointsFrom, false)) {
      return false;
    }
    affine.SRMatrix() *= scale;
    return true;
  }

  //! @brief Fit an affine isometry transformation between two sets of points.
  //! Preserves angles and distances.
  //! @param affine - The affine transformation to be computed.
  //! @param pointsTo - The first set of points.
  //! @param pointsFrom - The second set of points.
  //! @return True if the transformation was computed.
  template<typename RealT,unsigned N, SimpleContainer ContainerOfPointAT, SimpleContainer ContainerOfPointBT>
  bool fitIsometry(Affine<RealT,N> &affine,
		     const ContainerOfPointAT &pointsTo,
		     const ContainerOfPointBT &pointsFrom)
  {
    RealT scale = 0;
    if(!fitSimilarity<RealT,N>(affine.SRMatrix(), affine.translation(), scale, pointsTo, pointsFrom, true)) {
      return false;
    }
    affine.SRMatrix() *= scale;
    return true;
  }

  //! @brief Fit an affine isometry transformation between two sets of points.
  //! Preserves angles and distances.
  //! @param isometry3 - The affine transformation to be computed.
  //! @param pointsTo - The first set of points.
  //! @param pointsFrom - The second set of points.
  //! @return True if the transformation was computed.
  template<typename RealT, SimpleContainer ContainerOfPointAT, SimpleContainer ContainerOfPointBT>
  bool fit(Isometry3<RealT> &isometry3,
		    const ContainerOfPointAT &pointsTo,
		    const ContainerOfPointBT &pointsFrom)
  {
    RealT scale = 0;
    Matrix<RealT,3,3> rotation;
    Vector<RealT,3> translation;
    if(!fitSimilarity<RealT,3>(rotation, translation, scale, pointsTo, pointsFrom, true)) {
      return false;
    }
    isometry3 = Isometry3<RealT>(Quaternion<RealT>::fromMatrix(rotation), translation);
    return true;
  }

  //! @brief Fit an affine isometry transformation between two sets of points.
  //! Preserves angles and distances.
  //! @param isometry3 - The affine transformation to be computed.
  //! @param pointsTo - The first set of points.
  //! @param pointsFrom - The second set of points.
  //! @return True if the transformation was computed.
  template<typename RealT>
  bool fit(Isometry3<RealT> &isometry3,const Plane3PVV<RealT> &p1,const Plane3PVV<RealT> &p2)
  {
    std::array<Point<RealT,3>,3> pointsTo = {p1.origin(),p1.origin() + p1.vector1(),p1.origin() + p1.vector2()};
    std::array<Point<RealT,3>,3> pointsFrom = {p2.origin(),p2.origin() + p2.vector1(),p2.origin() + p2.vector2()};
    return fit(isometry3,pointsTo,pointsFrom);
  }




} // Ravl2
