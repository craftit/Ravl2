//
// Created by charles galambos on 07/09/2024.
//

#include <spdlog/spdlog.h>
#include "checks.hh"
#include "Ravl2/Geometry/Projection.hh"
#include "Ravl2/Geometry/Polygon.hh"

TEST_CASE("Projective2")
{
  using namespace Ravl2;
  using RealT = double;
  Projection<RealT, 2> proj(Projection<RealT, 2>::identity(2, 10)); // Create a unit projection with arbitrary iz & oz.

  //SPDLOG_INFO("Projection: {}", proj);
  // Check homography
  CHECK(sumOfSqr(proj.Homography() - xt::eye(3)) < 0.001);

  std::vector<Point<RealT,2>> ipnt;
  ipnt.push_back(toPoint<RealT>(1, 1));
  ipnt.push_back(toPoint<RealT>(2, 1));
  ipnt.push_back(toPoint<RealT>(1, 3));

  std::vector<Point<RealT,2>> opnt = ipnt;

  // Check polygon transform.
  Polygon<RealT> poly(ipnt);

  Polygon<RealT> tpoly = poly;
  tpoly *= proj;

  // Check that the points are transformed correctly.
  for(size_t i = 0;i < ipnt.size(); i++) {
    CHECK(sumOfSqr(tpoly[i] - opnt[i])< 0.001);
  }

  // Check projection multiplication
  Projection<RealT, 2> proj2(Projection<RealT, 2>::identity(14, 3)); // Another arbitrary unit projection
  CHECK(xt::sum(xt::abs((proj * proj2)(ipnt[1]) - ipnt[1]))() < 0.001);

  // Check affine constructor, using arbitrary projective scale values
  auto affine = affineFromScaleAngleTranslation<RealT>(toVector<RealT>(3, 1),0, toVector<RealT>(2, 2));
  Projection<RealT, 2> proj3(affine, 2, 7);
  Point<RealT,2> result = proj3(toPoint<RealT>(1, 0));
  auto direct = affine(toPoint<RealT>(1, 0));
  CHECK(sumOfSqr(result - direct) < 0.001);
}