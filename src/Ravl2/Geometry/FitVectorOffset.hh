
#pragma once

#include "Ravl2/Geometry/VectorOffset.hh"

namespace Ravl2
{

  //! Least squares fit of a plane to a set of points in 3d
  //! At least 3 points are needed.
  template <typename RealT, size_t N>
  bool fit(VectorOffset<RealT, N> &plane, const std::vector<Point<RealT, N>> &points)
  {
    if(points.size() < 3)
      return false;

    Matrix<RealT, N, N> covar = xt::zeros<RealT>({N, N});

    auto [mean, scale] = normalise<RealT, N>(points, [&covar](const Point<RealT, N> &pnt) {
      for(size_t i = 0; i < N; i++) {
        for(size_t j = i; j < 3; j++)
          covar(i, j) += pnt[i] * pnt[j];
      }
    });

    // Make it symmetric.
    for(size_t i = 0; i < N; i++) {
      for(size_t j = i + 1; j < N; j++) {
        covar(j, i) = covar(i, j);
      }
    }

    auto [u, s, v] = xt::linalg::svd(covar, true, true);

    // Find the smallest singular value,
    // they are normally sorted, but let's be paranoid.
    RealT min = s[0];
    size_t minI = 0;
    for(size_t i = 1; i < N; i++) {
      if(s[i] < min) {
        minI = i;
        min = s[i];
      }
    }

    Vector<RealT, N> normal = xt::view(v, minI, xt::all());
    RealT d = 0;
    for(size_t i = 0; i < N; i++) {
      normal[i] *= scale;
      d += normal[i] * mean[i];
    }
    plane = VectorOffset<RealT, N>(normal, -d);
    return true;
  }
  
  //extern template bool fit(VectorOffset<float, 3u> &plane, const std::vector<Point<float, 3u>> &points);

}// namespace Ravl2