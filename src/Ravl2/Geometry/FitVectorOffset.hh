
#pragma once

#include "Ravl2/Geometry/VectorOffset.hh"

namespace Ravl2
{

  //! Least squares fit of a plane to a set of points in 3d
  //! At least 3 points are needed.
  template <typename RealT, IndexSizeT N>
  bool fit(VectorOffset<RealT, N> &plane, const std::vector<Point<RealT, N>> &points)
  {
    if(points.size() < 3)
      return false;

    Matrix<RealT, N, N> covar = Matrix<RealT, N, N>::Zero();

    auto [mean, scale] = normalise<RealT, N>(points, [&covar](const Point<RealT, N> &pnt) {
      for(IndexT i = 0; i < IndexT(N); i++) {
        for(IndexT j = i; j < IndexT(N); j++) {
          covar(i, j) += pnt[i] * pnt[j];
        }
      }
    });

    // Make it symmetric.
    for(IndexT i = 0; i < IndexT(N); i++) {
      for(IndexT j = i + 1; j < IndexT(N); j++) {
        covar(j, i) = covar(i, j);
      }
    }

    //auto [u, s, v] = xt::linalg::svd(covar, true, true);
    Eigen::template JacobiSVD<Eigen::Matrix<RealT, N, N> > svd(covar,Eigen::ComputeFullV);
    auto s = svd.singularValues();
    auto v = svd.matrixV();

    // Find the smallest singular value,
    // they are normally sorted, but let's be paranoid.
    RealT min = s[0];
    IndexT minI = 0;
    for(IndexT i = 1; i < IndexT(N); i++) {
      if(s[i] < min) {
        minI = i;
        min = s[i];
      }
    }

    Vector<RealT, N> normal = v.col(minI);
    //SPDLOG_INFO("V: {}", v);
    //SPDLOG_INFO("Normal: {}  At:{}  Scale:{}", normal,minI, scale);
    RealT d = 0;
    for(IndexT i = 0; i < IndexT(N); i++) {
      normal[i] *= scale;
      d += normal[i] * mean[i];
    }
    plane = VectorOffset<RealT, N>(normal, -d);
    return true;
  }
  
  //extern template bool fit(VectorOffset<float, 3u> &plane, const std::vector<Point<float, 3u>> &points);

}// namespace Ravl2