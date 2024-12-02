//
// Created by charles galambos on 07/09/2024.
//

#include <spdlog/spdlog.h>
#include "checks.hh"
#include "Ravl2/Geometry/Projection.hh"
#include "Ravl2/Geometry/FitProjection.hh"
#include "Ravl2/Geometry/Polygon.hh"

TEST_CASE("Projective2")
{
  using namespace Ravl2;
  using RealT = double;
  Projection<RealT, 2> proj(Projection<RealT, 2>::identity(2, 10)); // Create a unit projection with arbitrary iz & oz.

  //SPDLOG_INFO("Projection: {}", proj);
  // Check homography
  CHECK(sumOfSqr(proj.homography() - Matrix<RealT,3,3>::Identity()) < 0.001);

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
  CHECK(((proj * proj2)(ipnt[1]) - ipnt[1]).cwiseAbs().sum() < 0.001);

  // Check affine constructor, using arbitrary projective scale values
  auto affine = affineFromScaleAngleTranslation<RealT>(toVector<RealT>(3, 1),0, toVector<RealT>(2, 2));
  Projection<RealT, 2> proj3(affine, 2, 7);
  Point<RealT,2> result = proj3(toPoint<RealT>(1, 0));
  auto direct = affine(toPoint<RealT>(1, 0));
  CHECK(sumOfSqr(result - direct) < 0.001);
}

TEST_CASE("FitProjection")
{
  using namespace Ravl2;
  using RealT = float;

  SECTION("Solve 4 points")
  {
    PointSet<float, 2> poly({Point2f({396.887756f, 296.020416f}), {429.030609f, 538.367371f}, {243.31633f, 538.877563f},
			     {240.765305f, 297.040802f}}
			   );
    PointSet<float, 2> poly2({Point2f({-1.0, -1.0}), {1.0, -1.0}, {1.0, 1.0}, {-1.0, 1.0}});

    Projection<RealT, 2> proj;
    CHECK(fit<RealT>(proj, poly, poly2));

    // Check that the points are transformed correctly.
    for(size_t i = 0; i < poly.size(); i++) {
      SPDLOG_TRACE("poly2: {} -> {} == {}", poly2[i], proj(poly2[i]), poly[i]);
      CHECK(sumOfSqr(proj(poly2[i]) - poly[i]) < 0.001f);
    }
    SPDLOG_TRACE("Projection: {}  Center:{} ", proj, proj(toPoint<RealT>(0,0)));

  }
  //0,0 -> 326.636932,  407.114777
  SECTION("Least squares 5 points")
  {
    PointSet<float, 2> poly({Point2f({396.887756f, 296.020416f}), {429.030609f, 538.367371f}, {243.31633f, 538.877563f},
			     {240.765305f, 297.040802f}, {326.636932f, 407.114777f}}
			   );
    PointSet<float, 2> poly2({Point2f({-1.0, -1.0}), {1.0, -1.0}, {1.0, 1.0}, {-1.0, 1.0},{0,0}});
    Projection<RealT, 2> proj;
    CHECK(fit<RealT>(proj, poly, poly2));

    // Check that the points are transformed correctly.
    for(size_t i = 0; i < poly.size(); i++) {
      SPDLOG_TRACE("poly2: {} -> {} == {}", poly2[i], proj(poly2[i]), poly[i]);
      CHECK(sumOfSqr(proj(poly2[i]) - poly[i]) < 0.001f);
    }
  }


}
