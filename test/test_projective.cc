//
// Created by charles galambos on 07/09/2024.
//

#include <spdlog/spdlog.h>
#include "Ravl2/Catch2checks.hh"
#include "Ravl2/Geometry/Projection.hh"
#include "Ravl2/Geometry/FitProjection.hh"
#include "Ravl2/Geometry/FitAffine.hh"
#include "Ravl2/Geometry/Polygon.hh"

namespace Ravl2
{
  TEST_CASE("Projection")
  {
    using RealT = double;

    SECTION("Basics")
    {
      // Simple sanity check that the projection is working.
      Projection<RealT, 2> proj(Projection<RealT, 2>::identity(2, 10)); // Create a unit projection with arbitrary iz & oz.

      // Check homography
      CHECK(sumOfSqr(proj.homography() - Matrix<RealT,3,3>::Identity()) < 0.001);
      CHECK(sumOfSqr(proj.affineApproximation().projectiveMatrix() - Matrix<RealT,3,3>::Identity()) < 0.001);

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

      // Check simple projection multiplication
      Projection<RealT, 2> proj2(Projection<RealT, 2>::identity(14, 3)); // Another arbitrary unit projection
      CHECK(((proj * proj2)(ipnt[1]) - ipnt[1]).cwiseAbs().sum() < 0.001);
    }

    SECTION("projectiveMatrix")
    {
      Matrix<RealT, 3, 3> tran { {1, 2, 3}, {4, 5, 6}, {7, 8, 9}};
      Projection<RealT, 2> a1(tran,2,3); // Create a unit projection with arbitrary iz & oz.

      auto const projMat = a1.projectiveMatrix();
      //SPDLOG_INFO("ProjMat:{}", projMat);
      auto const testPnt = toPoint<RealT>(2,3);
      auto const pnt = a1(testPnt);
      Point<RealT,3> const projPoint = projMat * toHomogeneous(testPnt);
      auto const projPnt = fromHomogeneous(projPoint);
      //SPDLOG_INFO("Pnt:{} projPnt:{} ", pnt, projPnt);
      CHECK(euclidDistance(pnt, projPnt) < 1e-6);
    }
  }


  TEST_CASE("Conversion to projection")
  {
    using RealT = double;
    SECTION("Scaled")
    {
      ScaleTranslate<float, 2> st1(toVector<float>(2, 2), toVector<float>(1, 2));
      Projection<float, 2> a1(st1,3,7);
      Point<float, 2> p = toPoint<float>(3, 4);
      Point<float, 2> pnt1 = a1(p);
      Point<float, 2> pnt2 = st1(p);
      CHECK(euclidDistance(pnt1, pnt2) < 0.001f);
    }
    SECTION("Translate")
    {
      Vector<float,2> trans = toVector<float>(2, 3);
      ScaleTranslate<float, 2> st1(toVector<float>(1, 1), trans);
      Projection<float, 2> a1 = Projection<float, 2>::translation(trans, 3, 7);
      Point<float, 2> p = toPoint<float>(3, 4);
      Point<float, 2> pnt1 = a1(p);
      Point<float, 2> pnt2 = st1(p);
      CHECK(euclidDistance(pnt1, pnt2) < 0.001f);
    }

    SECTION("Affine Constructor")
    {
      // Check affine constructor, using arbitrary projective scale values
      auto affine = affineFromScaleAngleTranslation<RealT>(toVector<RealT>(3, 1),0.4, toVector<RealT>(2, 2));
      Projection<RealT, 2> proj3(affine, 2, 7);
      Point<RealT,2> testPnt = toPoint<RealT>(1, 3);

      Point<RealT,2> result = proj3(testPnt);
      auto direct = affine(testPnt);
      CHECK(sumOfSqr(result - direct) < 0.001);

      // Check affine approximation of the projection
      auto affineApprox = proj3.affineApproximation();
      CHECK(sumOfSqr(affineApprox(testPnt) - result) < 0.001);
    }

    SECTION("Affine Approximation")
    {
      Matrix<RealT, 3, 3> tran { {1, 2, 3}, {4, 5, 6}, {7, 8, 9}};
      Projection<RealT, 2> proj(tran,2,3); // Create a unit projection with arbitrary iz & oz.
      auto affineApprox = proj.affineApproximation();

      Point<RealT,2> tp = toPoint<RealT>(0, 0);
      CHECK(sumOfSqr(affineApprox(tp) - proj(tp)) < 0.001);

      {
        const RealT offset = 1.0;
        std::vector<Point<RealT,2>> const ipnts = {
          toPoint<RealT>(offset, offset),
          toPoint<RealT>(offset, 0),
          toPoint<RealT>(offset, -offset),
          toPoint<RealT>(0, -offset),
          toPoint<RealT>(-offset, -offset),
          toPoint<RealT>(-offset, 0),
          toPoint<RealT>(-offset, offset),
          toPoint<RealT>(0, offset),
          toPoint<RealT>(0, 0) };
        std::vector<Point<RealT,2>> opnts;
        opnts.reserve(ipnts.size());
        for(auto p : ipnts) {
          opnts.push_back(proj(p));
        }
        Affine<RealT, 2> fittedAffineApprox;
        fit(fittedAffineApprox, opnts, ipnts);

        RealT errApprox = 0;
        RealT errFitted = 0;
        for(auto p : ipnts) {
          auto pnt = proj(p);
          auto pnt2 = affineApprox(p);
          auto error = sumOfSqr(pnt - pnt2);
          errApprox += error;
          CHECK(error < 1.1);

          auto pnt3 = fittedAffineApprox(p);
          auto error2 = sumOfSqr(pnt - pnt3);
          errFitted += error2;
          SPDLOG_INFO("Pnt: {} -> True: {} Approx:{} ({}) Fitted:{} ({}) ", p, pnt, pnt2, error, pnt3,error2);
        }
        errApprox /= ipnts.size();
        errFitted /= ipnts.size();
        SPDLOG_INFO("Approx: {}  Fitted:{} ", errApprox, errFitted);
        CHECK(errApprox < 0.26);
        CHECK(errFitted < 0.1);
      }

    }

  }

  TEST_CASE("FitProjection")
  {
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
}