//
// Created by charles galambos on 23/12/2024.
//

#include <numbers>
#include <random>
#include <spdlog/spdlog.h>
#include <cereal/archives/json.hpp>

#include "Ravl2/Catch2checks.hh"
#include "Ravl2/Geometry/Plane3PVV.hh"

namespace Ravl2
{
  TEST_CASE("Planes")
  {
    using RealT = float;
    std::mt19937 rng(static_cast<std::mt19937::result_type>(random()));
    std::uniform_real_distribution<RealT> random1(-1.0, 1.0);
    std::normal_distribution<RealT> randomGauss(0.0, 1.0);
    auto randomValue = [&random1,&rng](RealT scale) -> RealT
    { return (random1(rng)* scale); };


    SECTION("VectorOffset")
    {

      for(int i =0 ;i < 100;i++) {
        auto pntOnPlane = toPoint<RealT>(randomValue(10),randomValue(10),randomValue(10));
        VectorOffset<RealT,3> plane(toVector<RealT>(randomValue(10),randomValue(10),randomValue(10)),pntOnPlane);

        // Check point on plane has zero distance.
        CHECK(plane.distance(pntOnPlane) < 0.00001f);

        auto testPoint = toPoint<RealT>(randomValue(10),randomValue(10),randomValue(10));

        auto closestPoint = plane.projection(testPoint);
        RealT distance = euclidDistance(closestPoint, testPoint);
        CHECK(std::abs(distance - plane.distance(testPoint)) < 0.00001f);
      }
    }

    SECTION("Fit VectorOffset")
    {
      std::vector<Point<RealT,3>> points;
      points.push_back(toPoint<RealT>(1,1,1));
      points.push_back(toPoint<RealT>(1,2,1));
      points.push_back(toPoint<RealT>(2,1,2));
      VectorOffset<RealT,3> plane;
      CHECK(fit<RealT,3>(plane,points));

      for(auto pnt: points) {
        RealT dist = plane.distance(pnt);
        CHECK(dist < 1e-6f);
      }
    }

    SECTION("PlanePVV3 Project")
    {
      Plane3PVV<RealT> const plane(toPoint<RealT>(1,2,3),
                             toVector<RealT>(4,5,6),
                             toVector<RealT>(7,8,9)
                      );

      auto const projMat = plane.projectiveMatrix();
      //SPDLOG_INFO("ProjMat:{}", projMat);
      auto const testPnt2 = toPoint<RealT>(10,11);
      auto const pnt3 = plane.at(testPnt2);
      Point<RealT,4> const projPoint = projMat * toHomogeneous(testPnt2);
      auto const projPnt3 = fromHomogeneous(projPoint);
      //SPDLOG_INFO("Pnt3:{}", projPnt3);
      CHECK(euclidDistance(pnt3, projPnt3) < 1e-6f);
    }

  }
}

