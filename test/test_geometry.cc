
#include <numbers>
#include <catch2/catch_test_macros.hpp>
#include <spdlog/spdlog.h>
#include <cereal/archives/json.hpp>

#include "Ravl2/Math.hh"
#include "Ravl2/Angle.hh"
#include "Ravl2/Geometry/Geometry.hh"
#include "Ravl2/Geometry/Moments2.hh"
#include "Ravl2/Geometry/Circle.hh"
#include "Ravl2/Geometry/FitCircle.hh"
#include "Ravl2/Geometry/CircleIter.hh"
#include "Ravl2/Geometry/Conic2d.hh"
#include "Ravl2/Geometry/FitConic.hh"
#include "Ravl2/Geometry/LineABC2d.hh"
#include "Ravl2/Geometry/Affine.hh"
#include "Ravl2/Geometry/ScaleTranslate.hh"
#include "Ravl2/Geometry/VectorOffset.hh"
#include "Ravl2/Geometry/FitVectorOffset.hh"
#include "Ravl2/Geometry/PlanePVV3d.hh"
#include "Ravl2/Geometry/FitAffine.hh"
#include "Ravl2/Math/FastFourierTransform.hh"

#define CHECK_EQ(a,b) CHECK((a) == (b))
#define CHECK_NE(a,b) CHECK_FALSE((a) == (b))
#define EXPECT_TRUE(a) CHECK(a)
#define EXPECT_EQ(a,b) CHECK((a) == (b))
#define EXPECT_NE(a,b) CHECK_FALSE((a) == (b))
#define ASSERT_EQ(a,b) REQUIRE((a) == (b))
#define ASSERT_NE(a,b) REQUIRE_FALSE((a) == (b))
#define ASSERT_FLOAT_EQ(a,b) REQUIRE(Ravl2::isNearZero((a) -(b)))

TEST_CASE("Moments")
{
  using namespace Ravl2;

  SECTION("AddPixel")
  {
    Moments2<double> moments;
    moments.addPixel(Index<2>(0, 0));
    moments.addPixel(Index<2>(1, 0));
    moments.addPixel(Index<2>(0, 1));
    moments.addPixel(Index<2>(1, 1));

    auto principal_axis = moments.principalAxisSize();
    ASSERT_FLOAT_EQ(principal_axis[0], 0.25);
    ASSERT_FLOAT_EQ(principal_axis[1], 0.25);
    ASSERT_FLOAT_EQ(Moments2<double>::elongatedness(principal_axis), 0.0);
    ASSERT_FLOAT_EQ(moments.centroid()[0], 0.5);
    ASSERT_FLOAT_EQ(moments.centroid()[1], 0.5);
  }
  SECTION("Shift")
  {
    Moments2<int> momentsA;
    momentsA.addPixel(Index<2>(1,1));

    Moments2<int> momentsB;
    std::vector<Index<2>> pnts {
      {1,1},
      {1,2},
      {2,2}
    };
    for(auto p : pnts)
      momentsB.addPixel(p);

    std::vector<Index<2>> offsets {
      {0,0},
      {1,0},
      {0,1},
      {1,1},
      {2,1},
      {1,2},
      {2,2}
    };

    for(auto offset : offsets) {
      {
        Moments2<int> moments;
        moments.addPixel(Index<2>(1, 1) + offset);
        moments.shift(-offset);
        //SPDLOG_INFO("A Offset {} : {}  shifted ", offset, moments);
        CHECK(momentsA == moments);
      }
      {
        Moments2<int> moments;
        for(auto p : pnts) {
          moments.addPixel(p + offset);
        }
        moments.shift(-offset);
        //SPDLOG_INFO("B Offset {} : {}  shifted ", offset, moments);
        CHECK(momentsB == moments);
      }
    }
  }

  SECTION("Cereal IO")
  {
    Moments2<double> moments(1,2,3,4,5,6);

    std::stringstream ss;
    {
      cereal::JSONOutputArchive oarchive(ss);
      oarchive(moments);
    }
    SPDLOG_TRACE("Json: {}", ss.str());
    {
      cereal::JSONInputArchive iarchive(ss);
      Moments2<double> moments2;
      iarchive(moments2);
      ASSERT_FLOAT_EQ(moments2.M00(), 1);
      ASSERT_FLOAT_EQ(moments2.M10(), 2);
      ASSERT_FLOAT_EQ(moments2.M01(), 3);
      ASSERT_FLOAT_EQ(moments2.M20(), 4);
      ASSERT_FLOAT_EQ(moments2.M11(), 5);
      ASSERT_FLOAT_EQ(moments2.M02(), 6);
    }
  }
}

TEST_CASE("Vector and Matrix")
{
  using namespace Ravl2;
  SECTION( "Point<float,2> Cereal. ")
  {
    Point<float,2> p1({1,2});
    std::stringstream ss;
    {
      cereal::JSONOutputArchive oarchive(ss);
      //serialize(oarchive, p1);
      oarchive(p1);
    }
    SPDLOG_TRACE("Point<float,2>: {}", ss.str());
    {
      cereal::JSONInputArchive iarchive(ss);
      Point<float,2> p2;
      //serialize(iarchive, p2);
      iarchive(p2);
      CHECK(isNearZero(p1(0) - p2(0)));
      CHECK(isNearZero(p1(1) - p2(1)));
    }
  }
  SECTION( "Matrix<float,2,3> Cereal. ")
  {
    Matrix<float,2,3> m1({{1,2,3},{4,5,6}});
    std::stringstream ss;
    {
      cereal::JSONOutputArchive oarchive(ss);
      oarchive(m1);
    }
    // SPDLOG_INFO("Matrix<float,2,3>: {}", ss.str());
    {
      cereal::JSONInputArchive iarchive(ss);
      Matrix<float,2,3> m2;
      iarchive(m2);
      CHECK(isNearZero(m1(0,0) - m2(0,0)));
      CHECK(isNearZero(m1(0,1) - m2(0,1)));
      CHECK(isNearZero(m1(0,2) - m2(0,2)));
      CHECK(isNearZero(m1(1,0) - m2(1,0)));
      CHECK(isNearZero(m1(1,1) - m2(1,1)));
      CHECK(isNearZero(m1(1,2) - m2(1,2)));
    }
  }
}

TEST_CASE("Affine")
{
  using namespace Ravl2;
  SECTION("Core")
  {
    Affine<float, 2>
      a1 = affineFromScaleAngleTranslation(toVector<float>(2, 2), std::numbers::pi_v<float> / 2, toVector<float>(0, 0));

    Point<float, 2> p = a1(toPoint<float>(0, 0));
    //Point<float, 2> p = a1 * toPoint<float>(0, 0);
    CHECK(euclidDistance(p, toPoint<float>(0, 0)) < 0.001f);
  }
  SECTION("Composition")
  {
    Affine<float, 2>
      a1 = affineFromScaleAngleTranslation(toVector<float>(2, 2), std::numbers::pi_v<float> / 2, toVector<float>(0, 0));
    Affine<float, 2> a2 = affineFromScaleAngleTranslation(toVector<float>(1, 1), 0.0f, toVector<float>(10, 20));
    Point<float, 2> p = toPoint<float>(0, 0);
    Point<float, 2> pnt0 = a2(a1(p));
    //SPDLOG_INFO("At: {} {} ", pnt0(0), pnt0(1));
    CHECK(euclidDistance(pnt0, toPoint<float>(10, 20)) < 0.001f);
    Point<float, 2> pnt = a1(a2(p));
    //SPDLOG_INFO("At: {} {} ", pnt(0), pnt(1));
    CHECK(euclidDistance(pnt, toPoint<float>(-40, 20)) < 0.001f);
    Point<float, 2> q = toPoint<float>(5, 4);
    CHECK(Ravl2::euclidDistance(a2(a1)(q), a2(a1(q))) < 0.001f);
  }
  SECTION("Inverse")
  {
    Affine<float, 2>
      a1 = affineFromScaleAngleTranslation(toVector<float>(2, 2), std::numbers::pi_v<float> / 2, toVector<float>(1, 2));
    auto a2 = a1.inverse();
    CHECK(a2.has_value());
    Point<float, 2> p = toPoint<float>(3, 4);
    Point<float, 2> pnt = a1(a2.value()(p));
    CHECK(euclidDistance(pnt, p) < 0.001f);
  }
  SECTION("Cereal")
  {
    Affine<float, 2>
      a1 = affineFromScaleAngleTranslation(toVector<float>(1, 2), std::numbers::pi_v<float> / 3, toVector<float>(4, 5));
    std::stringstream ss;
    {
      cereal::JSONOutputArchive oarchive(ss);
      oarchive(a1);
    }
    //SPDLOG_INFO("Affine<float, 2>: {}", ss.str());
    {
      cereal::JSONInputArchive iarchive(ss);
      Affine<float, 2> a2;
      iarchive(a2);
      CHECK(isNearZero(a1.Translation()[0] - a2.Translation()[0]));
      CHECK(isNearZero(a1.Translation()[1] - a2.Translation()[1]));
      CHECK(isNearZero(a1.SRMatrix()(0,0) - a2.SRMatrix()(0,0)));
      CHECK(isNearZero(a1.SRMatrix()(0,1) - a2.SRMatrix()(0,1)));
      CHECK(isNearZero(a1.SRMatrix()(1,0) - a2.SRMatrix()(1,0)));
      CHECK(isNearZero(a1.SRMatrix()(1,1) - a2.SRMatrix()(1,1)));
    }
  }
  SECTION("FitPoints")
  {
    using RealT = float;
    std::vector<Point<RealT,2>> ipnt;
    ipnt.push_back(toPoint<RealT>(1,1));
    ipnt.push_back(toPoint<RealT>(2,1));
    ipnt.push_back(toPoint<RealT>(1,3));

    std::vector<Point<RealT,2>> opnt;
    opnt.push_back(toPoint<RealT>(2,2));
    opnt.push_back(toPoint<RealT>(3,2));
    opnt.push_back(toPoint<RealT>(2,3));

    Affine<RealT,2> aff;
    CHECK(fit(aff,
        opnt[0], ipnt[0],
        opnt[1], ipnt[1],
        opnt[2], ipnt[2]));

    for(size_t i=0;i < ipnt.size();i++) {
      CHECK(euclidDistance(aff(ipnt[i]), opnt[i]) < 0.001f);
    }
  }
  SECTION("Fit")
  {
    using RealT = float;
    std::vector<Point<RealT,2>> ipnt;
    ipnt.push_back(toPoint<RealT>(1,1));
    ipnt.push_back(toPoint<RealT>(2,1));
    ipnt.push_back(toPoint<RealT>(1,3));

    std::vector<Point<RealT,2>> opnt;
    opnt.push_back(toPoint<RealT>(2,2));
    opnt.push_back(toPoint<RealT>(3,2));
    opnt.push_back(toPoint<RealT>(2,3));

    Affine<RealT,2> aff;
    auto residual = fit(aff, opnt, ipnt);
    CHECK(isNearZero(residual));

    SPDLOG_TRACE("Affine: {} Residual:{}", aff,residual);

    for(size_t i=0;i < ipnt.size();i++) {
      CHECK(euclidDistance(aff(ipnt[i]), opnt[i]) < 0.001f);
    }
  }
  SECTION("FitLSQ")
  {
    using RealT = float;

    // By providing 4 points we switch to a least squares fit, we're duplicating a point though so it should still fit exactly.

    std::vector<Point<RealT,2>> ipnt;
    ipnt.push_back(toPoint<RealT>(1,1));
    ipnt.push_back(toPoint<RealT>(2,1));
    ipnt.push_back(toPoint<RealT>(1,3));
    ipnt.push_back(toPoint<RealT>(1,3));

    std::vector<Point<RealT,2>> opnt;
    opnt.push_back(toPoint<RealT>(2,2));
    opnt.push_back(toPoint<RealT>(3,2));
    opnt.push_back(toPoint<RealT>(2,3));
    opnt.push_back(toPoint<RealT>(2,3));

    Affine<RealT,2> aff;
    auto residual = fit(aff, opnt, ipnt);

    //SPDLOG_INFO("Affine: {} Residual:{}", aff,residual);
    CHECK(isNearZero(residual));

    for(size_t i=0;i < ipnt.size();i++) {
      CHECK(euclidDistance(aff(ipnt[i]), opnt[i]) < 0.001f);
    }
  }
}

TEST_CASE("CircleIter")
{
  using namespace Ravl2;
  using RealT = float;

  int i = 0;
  unsigned rad = 20;
  RealT maxDist = 0;
  Point<float,2> origin({0,0});
  Point<float,2> at;
  for(CircleIterC it(rad);it.valid();it.next(),i++) {
    at[0] = float(it.Data()[0]);
    at[1] = float(it.Data()[1]);
    RealT dst =  euclidDistance<float,2>(at,origin);
    RealT diff = std::abs(RealT(rad) - dst);
    //cerr << "Dst:" << std::abs((RealT) rad - dst) << " ";
    CHECK(diff < 0.5f); // Should never be greater than 0.5 !
    if(diff > maxDist)
      maxDist = diff;
    //cout << it.Data() << std::endl;
  }
  //SPDLOG_INFO("Points:{} Largest error:{}", i, maxDist);
  EXPECT_EQ(i,112);
}

TEST_CASE("Circle")
{
  using namespace Ravl2;

  std::vector<Point<float,2>> pnts(5);
  pnts[0] = Point<float,2>({1,0});
  pnts[1] = Point<float,2>({-1,2});
  pnts[2] = Point<float,2>({3,2});
  pnts[3] = Point<float,2>({1,4});
  pnts[4] = Point<float,2>({1,4.01f}); // Just to add a slight error.

  SECTION("Fit 3 points.")
  {
    Circle2dC<float> circle2;
    EXPECT_TRUE(circle2.Fit(pnts[0], pnts[1], pnts[2]));
    SPDLOG_TRACE("Center={} Radius={}", circle2.Centre(), circle2.Radius());
    float sqrMag = xt::sum(xt::square(Point<float, 2>(circle2.Centre() - Point<float, 2>({1, 2}))))[0];
    CHECK(sqrMag < 0.01f);
    CHECK(std::abs(circle2.Radius() - 2) < 0.01f);
  }

  SECTION("Fit N points.")
  {
    Circle2dC<float> circle;
    auto residual = Ravl2::fit(circle, pnts);
    CHECK(residual.has_value());
    SPDLOG_TRACE("Center={} Radius={} Residual={}", circle.Centre(), circle.Radius(), residual.value());
    CHECK(sumOfSqr(Point<float, 2>(circle.Centre() - toPoint<float>(1, 2))) < 0.01f);
    CHECK(std::abs(circle.Radius() - 2) < 0.01f);
  }
}

TEST_CASE("Conic")
{
  using namespace Ravl2;
  std::vector<Point<float,2>> pnts;
  pnts.reserve(5);
  pnts.push_back(Point<float,2>({1,0}));
  pnts.push_back(Point<float,2>({2,-1}));
  pnts.push_back(Point<float,2>({3,0}));
  pnts.push_back(Point<float,2>({3,1}));
  pnts.push_back(Point<float,2>({2,4}));
  Ravl2::Conic2dC<float> conic {};
  auto residual = fit(conic, pnts);
  CHECK(residual.has_value());
  SPDLOG_TRACE("Conic: {}", conic);
  //=-0.264764 -0.132382 -0.066191 1.05906 0.463337 -0.794292
  Ravl2::Conic2dC<float> conic2(-0.264764f, -0.132382f, -0.066191f, 1.05906f, 0.463337f, -0.794292f);

  for(auto p : pnts) {
    //SPDLOG_INFO("Point {} is on curve: {} 2:{} ", p, conic.Residue(p), conic2.Residue(p));
    CHECK(conic.IsOnCurve(p,1e-4f));
    CHECK(conic2.IsOnCurve(p,1e-4f));
  }
}

TEST_CASE("Ellipse")
{
  using namespace Ravl2;

  SECTION("Simple Fit")
  {
    std::vector<Point<float, 2>> pnts;
    pnts.reserve(5);
    pnts.push_back(Point<float, 2>({1, 0}));
    pnts.push_back(Point<float, 2>({2, -1}));
    pnts.push_back(Point<float, 2>({3, 0}));
    pnts.push_back(Point<float, 2>({3, 1}));
    pnts.push_back(Point<float, 2>({2, 4}));
#if 1
    Conic2dC<float> conic {};
    auto residual = fit(conic, pnts);
    //auto residual = fitEllipse(conic, pnts);
    CHECK(residual.has_value());
    //SPDLOG_INFO("Ellipse: {}", conic);

    auto optEllipse = toEllipse(conic);
    REQUIRE(optEllipse.has_value());

    auto ellipse = optEllipse.value();
    for(auto p : pnts) {
      //SPDLOG_INFO("Point {} is on curve: {} ", p, ellipse.residue(p));
      CHECK(ellipse.IsOnCurve(p, 1e-4f));
    }
#endif
  }
  SECTION("Mean Covariance")
  {
    using RealT = float;
    Matrix<RealT,2,2> covar({{4,0},
      {0,1}});
    Vector<RealT,2> mean = toVector<RealT>(50,50);

    Ellipse2dC<RealT> ellipse = EllipseMeanCovariance(covar,mean,1.0f);
    //SPDLOG_INFO("Ellipse: {}", ellipse);
    Point<RealT,2> centre;
    RealT min,maj,ang;
    ellipse.EllipseParameters(centre,maj,min,ang);
    //SPDLOG_INFO("Parameters={} {} {} {} ", centre, maj, min, ang);

    CHECK((std::abs(maj - RealT(2))) < 0.0000001f);
    CHECK(std::abs(min - 1) < 0.0000001f);
    CHECK(std::abs(Angle<RealT, 1>(ang).diff(Angle<RealT, 1>(0))) < 1e-5f);

  }
#if 0
  SECTION("Fitting Orientations")
  {
    using RealT = float;

    // Generates series of ellipses with orientations from 0 to pi
    size_t numSteps = 10;
    RealT angleStep = std::numbers::pi_v<RealT>/RealT(numSteps);
    for(size_t j = 0;j < numSteps;j++) {
      RealT tangle = RealT(j) * angleStep;
      Point<RealT,2> gtc = toPoint<RealT>(50,50);
      // Generate ellispe
      Ellipse2dC<RealT> ellipse(gtc,RealT(40),RealT(20),tangle);
      // Generate set of points on ellipse
      RealT step = std::numbers::pi_v<RealT>/5;
      std::vector<Point<RealT,2>> points(10);
      size_t i = 0;
      for(RealT a = 0;i < 10;a += step,i++) {
        points[i] = ellipse.point(a);
      }
      // Fit set of points to ellipse as conic
      Conic2dC<RealT> conic;
      CHECK(fitEllipse(conic, points));
      //cerr << "Conic=" << conic.C() << "\n";
      Point<RealT,2> centre;
      RealT min,maj,ang;
      conic.EllipseParameters(centre,maj,min,ang);
      //cerr << "Conic representation parameters=" << centre << " " << maj << " " << min << " " << ang << "   Diff=" << Angle(ang,std::numbers::pi_v<RealT>).Diff(Angle(tangle,std::numbers::pi_v<RealT>)) << "\n";
      CHECK(xt::sum(xt::abs(centre - gtc))() < 0.00000001f);
#if 0
      if(std::abs(maj - 40) > 0.000000001) return __LINE__;
      if(std::abs(min - 20) > 0.000000001) return __LINE__;
      if(std::abs(Angle(ang,std::numbers::pi_v<RealT>).Diff(Angle(tangle,std::numbers::pi_v<RealT>))) > 0.000001) return __LINE__;
      // Fit same set of points to ellipse as Ellipse2dC
      Ellipse2dC ellipse2;
      CHECK(FitEllipse(points,ellipse2));
      // Check that fitted ellipse has same params as original
      ellipse2.EllipseParameters(centre,maj,min,ang);
      //cerr << "Ellipse representation parameters=" << centre << " " << maj << " " << min << " " << ang << "  Diff=" << Angle(ang,std::numbers::pi_v<RealT>).Diff(Angle(tangle,std::numbers::pi_v<RealT>)) << "\n";
      if((centre - gtc).SumOfstd::abs() > 0.00000001) return __LINE__;
      if(std::abs(maj - 40) > 0.000000001) return __LINE__;
      if(std::abs(min - 20) > 0.000000001) return __LINE__;
      //cerr << "param angle vs orig: " << ang << " " << tangle << endl;
      if(std::abs(Angle(ang,std::numbers::pi_v<RealT>).diff(Angle(tangle,std::numbers::pi_v<RealT>))) > 0.000001) return __LINE__;
#endif
    }
  }
#endif
}



TEST_CASE("ScaleTranslate")
{
  using namespace Ravl2;
  SECTION( "Basic ops ")
  {
    ScaleTranslate<float, 2> a1(toVector<float>(2, 2), toVector<float>(1, 2));
    CHECK(euclidDistance(a1(toPoint<float>(0, 0)), toPoint<float>(1, 2)) < 0.001f);
    CHECK(euclidDistance(a1(toPoint<float>(1, 1)), toPoint<float>(3, 4)) < 0.001f);
    ScaleTranslate<float, 2> a2(toVector<float>(2, 1), toVector<float>(1, 2));
    CHECK(euclidDistance(a2(toPoint<float>(1, 1)), toPoint<float>(3, 3)) < 0.001f);
  }
  SECTION( "Cereal. ")
  {
    ScaleTranslate<float, 2> a1(toVector<float>(1, 2), toVector<float>(3, 4));
    std::stringstream ss;
    {
      cereal::JSONOutputArchive oarchive(ss);
      oarchive(a1);
    }
    //SPDLOG_INFO("ScaleTranslate<float, 2>: {}", ss.str());
    {
      cereal::JSONInputArchive iarchive(ss);
      ScaleTranslate<float, 2> a2;
      iarchive(a2);
      CHECK(isNearZero(a1.translation()[0] - a2.translation()[0]));
      CHECK(isNearZero(a1.translation()[1] - a2.translation()[1]));
      CHECK(isNearZero(a1.scaleVector()[0] - a2.scaleVector()[0]));
      CHECK(isNearZero(a1.scaleVector()[1] - a2.scaleVector()[1]));
    }
  }
}


TEST_CASE("Planes")
{
  using namespace Ravl2;
  using RealT = float;
  std::mt19937 rng(static_cast<std::mt19937::result_type>(random()));
  std::uniform_real_distribution<RealT> random1(-1.0, 1.0);
  std::normal_distribution<RealT> randomGauss(0.0, 1.0);
  auto randomValue = [&random1,&rng](RealT scale) -> RealT
  { return (random1(rng)* scale); };


  SECTION("VectorOffset")
  {

    for(int i =0 ;i < 100;i++) {

       VectorOffset<RealT,3> plane(toVector<RealT>(randomValue(10),randomValue(10),randomValue(10)),
                         toPoint<RealT>(randomValue(10),randomValue(10),randomValue(10)));

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
    CHECK(fit(plane,points));

    for(auto pnt: points) {
      RealT dist = plane.distance(pnt);
      CHECK(dist < 1e-6f);
    }
  }

  SECTION("Fit PlanePVV3")
  {
#if 0
    for(int i =0 ;i < 100;i++) {

      PlanePVV3dC<RealT> plane(toPoint<RealT>(randomValue(10),randomValue(10),randomValue(10)),
                        toVector<RealT>(randomValue(10),randomValue(10),randomValue(10)),
                        toVector<RealT>(randomValue(10),randomValue(10),randomValue(10))
      );

      auto testPoint = toPoint<RealT>(randomValue(10),randomValue(10),randomValue(10));

      auto closestPoint = plane.ClosestPoint(testPoint);
      RealT distance = euclidDistance(closestPoint,testPoint);
      CHECK(std::abs(distance - plane.distance(testPoint)) < 1e-6f);

      // Check 'ProjectionOnto'
      auto pCloesestPoint = plane.Projection(testPoint);
      CHECK(euclidDistance(closestPoint,plane.at(pCloesestPoint)) < 1e-6f);
    }
#endif
  }


#if 0
  SECTION( "Fit PointVectorVector")
  {
    PlanePVV3dC plane;
    if(!FitPlane(points,plane))
      return __LINE__;

    for(int i =0;i < 3;i++) {
      if(plane.EuclideanDistance(points[i]) > 0.001)
        return __LINE__;
    }
  }

#endif



}



