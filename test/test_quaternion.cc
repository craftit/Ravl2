//
// Created by charles galambos on 22/10/2023.
//

#include "checks.hh"
#include "Ravl2/Geometry/Quaternion.hh"
#include "Ravl2/Geometry/Isometry3.hh"


TEST_CASE("Quaternion")
{
  using namespace Ravl2;
  using RealT = float;
  //SetSPDLogLevel beQuiet(spdlog::level::off);

  SECTION("AxisAngle rotations")
  {
    // Setup a zero rotation
    Quaternion const q1 = Quaternion<RealT>::fromAngleAxis(0.0, {0.0, 0.0, 1.0});

    // Check we've got a zero rotation
    Vector<RealT,3> const e1 {1.0, 2.0, 3.0};
    Vector<RealT,3> rot = q1.rotate(e1);

    EXPECT_FLOAT_EQ(rot[0], 1.0f);
    EXPECT_FLOAT_EQ(rot[1], 2.0f);
    EXPECT_FLOAT_EQ(rot[2], 3.0f);

    auto q1inv = q1.inverse();
    Vector3f irot = q1inv.rotate(rot);

    EXPECT_FLOAT_EQ(irot[0], 1.0f);
    EXPECT_FLOAT_EQ(irot[1], 2.0f);
    EXPECT_FLOAT_EQ(irot[2], 3.0f);

    // Setup a 90 degree rotation about the z axis
    Quaternion const q2 = Quaternion<RealT>::fromAngleAxis(RealT(std::numbers::pi / 2.0), {0.0, 0.0, 1.0});

    // Check we've got the right rotation
    rot = q2.rotate(e1);
    EXPECT_FLOAT_EQ(rot[0], -2.0f);
    EXPECT_FLOAT_EQ(rot[1], 1.0f);
    EXPECT_FLOAT_EQ(rot[2], 3.0f);

    auto q2inv = q2.inverse();
    irot = q2inv.rotate(rot);

    EXPECT_FLOAT_EQ(irot[0], 1.0f);
    EXPECT_FLOAT_EQ(irot[1], 2.0f);
    EXPECT_FLOAT_EQ(irot[2], 3.0f);

    // Setup a 90 degree rotation about the x axis
    Quaternion const q3 = Quaternion<RealT>::fromAngleAxis(RealT(std::numbers::pi / 2.0), {1.0, 0.0, 0.0});

    // Check we've got the right rotation
    rot = q3.rotate(e1);
    EXPECT_FLOAT_EQ(rot[0], 1.0f);
    EXPECT_FLOAT_EQ(rot[1], -3.0f);
    EXPECT_FLOAT_EQ(rot[2], 2.0f);

    auto q3inv = q3.inverse();
    irot = q3inv.rotate(rot);

    EXPECT_FLOAT_EQ(irot[0], 1.0f);
    EXPECT_NEAR(irot[1], 2.0f, 1e-5f);
    EXPECT_NEAR(irot[2], 3.0f, 1e-5f);

    // Setup a 90 degree rotation about the y axis
    Quaternion const q4 = Quaternion<RealT>::fromAngleAxis(RealT(std::numbers::pi / 2.0), {0.0, 1.0, 0.0});

    // Check we've got the right rotation
    rot = q4.rotate(e1);
    EXPECT_FLOAT_EQ(rot[0], 3.0f);
    EXPECT_FLOAT_EQ(rot[1], 2.0f);
    EXPECT_NEAR(rot[2], -1.0f, 1e-5f);

    auto q4inv = q4.inverse();
    irot = q4inv.rotate(rot);

    EXPECT_NEAR(irot[0], 1.0f, 1e-6f);
    EXPECT_FLOAT_EQ(irot[1], 2.0f);
    EXPECT_FLOAT_EQ(irot[2], 3.0f);

    // Check rotation composition
    Quaternion const q5 = q4inv * q4;
    rot = q5.rotate(e1);

    EXPECT_FLOAT_EQ(rot[0], 1.0f);
    EXPECT_FLOAT_EQ(rot[1], 2.0f);
    EXPECT_FLOAT_EQ(rot[2], 3.0f);
  }

  SECTION("Spherical Linear Interpolation")
  {
    // Setup a zero rotation
    Quaternion const q1 = Quaternion<RealT>::fromAngleAxis(0.0, {0.0, 0.0, 1.0});
    Quaternion const q2 = Quaternion<RealT>::fromAngleAxis(RealT(std::numbers::pi/4.0), {0.0, 0.0, 1.0});

    // At 0 we should be close to q1
    Quaternion const q3 = slerp(q1,q2, 0);
    EXPECT_FLOAT_EQ(q3.angle(),0);

    // At 1 we should be close to q2
    Quaternion const q4 = slerp(q1, q2, 1.0);
    Quaternion const tq4 = q4 * q2.inverse();
    EXPECT_NEAR(tq4.angle(),0,1e-6f);

    Quaternion q5 = slerp(q1,q2, 0.5);
    EXPECT_FLOAT_EQ(q5.angle(),RealT(std::numbers::pi/16.0));

    // At 0.5 we should be halfway between q1 and q2
    const int sampleCount = 10;
    RealT step = RealT(1.0)/sampleCount;
    float f = 0;
    float endAngle = q2.angle();
    for(int i =0; i < sampleCount; i++) {
      Quaternion q6 = slerp(q1,q2, f);
      //SPDLOG_INFO("Angle: {}  Diff: {} ",rad2deg(q6.angle()), (q6.angle() - f*endAngle));
      EXPECT_NEAR(q6.angle(),f*endAngle,1e-7f);
      f += step;
    }
  }

  SECTION("EulerAngles")
  {

    // Setup a zero rotation
    Quaternion<RealT> const qZero = Quaternion<RealT>::fromEulerAngles({0.0, 0.0, 0.0});
    EXPECT_FLOAT_EQ(qZero.angle(),0);

    // Setup a 45 degree rotation about the x axis
    Quaternion<RealT> const q1 = Quaternion<RealT>::fromEulerAngles({RealT(std::numbers::pi/4.0), 0.0, 0.0 });
    EXPECT_FLOAT_EQ(q1.angle(),RealT(std::numbers::pi/8.0));
    auto angles1 = q1.eulerAngles();
    EXPECT_FLOAT_EQ(angles1[0],RealT(std::numbers::pi/4.0));
    EXPECT_FLOAT_EQ(angles1[1],0);
    EXPECT_FLOAT_EQ(angles1[2],0);

    // Setup a 45 degree rotation about the y axis
    Quaternion<RealT> const q2 = Quaternion<RealT>::fromEulerAngles({0.0, RealT(std::numbers::pi/4.0), 0.0 });
    EXPECT_FLOAT_EQ(q1.angle(),RealT(std::numbers::pi/8.0));
    auto angles2 = q2.eulerAngles();
    EXPECT_FLOAT_EQ(angles2[0],0);
    EXPECT_FLOAT_EQ(angles2[1],RealT(std::numbers::pi/4.0));
    EXPECT_FLOAT_EQ(angles2[2],0);

    // Setup a 45 degree rotation about the z axis
    Quaternion<RealT> const q3 = Quaternion<RealT>::fromEulerAngles({0.0, 0.0, RealT(std::numbers::pi/4.0) });
    EXPECT_FLOAT_EQ(q1.angle(),RealT(std::numbers::pi/8.0));
    auto angles3 = q3.eulerAngles();
    EXPECT_FLOAT_EQ(angles3[0],0);
    EXPECT_FLOAT_EQ(angles3[1],0);
    EXPECT_FLOAT_EQ(angles3[2],RealT(std::numbers::pi/4.0));

    // Setup a combined rotation
    Quaternion<RealT> const q4 = Quaternion<RealT>::fromEulerAngles({RealT(std::numbers::pi/4.0), RealT(std::numbers::pi/5.0), RealT(std::numbers::pi/6.0) });
    auto angles4 = q4.eulerAngles();
    EXPECT_FLOAT_EQ(angles4[0],RealT(std::numbers::pi/4.0));
    EXPECT_FLOAT_EQ(angles4[1],RealT(std::numbers::pi/5.0));
    EXPECT_FLOAT_EQ(angles4[2],RealT(std::numbers::pi/6.0));
  }
  SECTION("Inverse")
  {
    Vector3d testVec;
    Vector3d position;
    // Create a random number generator
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<double> dis(-1.0, 1.0);

    for(int i = 0; i < 10; i++) {
      // set a random position
      testVec = {dis(gen), dis(gen), dis(gen)};
      position = {dis(gen), dis(gen), dis(gen)};
      auto q = Quaternion<RealT>::fromAngleAxis(RealT(dis(gen)), Vector3d{dis(gen), dis(gen), dis(gen)});

      auto restored = q.inverse().rotate(q.rotate(testVec));

      EXPECT_NEAR(xt::norm_l2(restored - testVec)(), 0.0, 1e-5);
    }
  }

}

TEST_CASE("Isometry3")
{
  using namespace Ravl2;
  using RealT = float;

  SECTION("transform")
  {
    Isometry3<RealT> test(Quaternion<RealT>::fromEulerAngles({RealT(std::numbers::pi / 4.0), RealT(std::numbers::pi / 5.0), RealT(std::numbers::pi / 6.0)}), Vector3f {1, 2, 3});

    Vector<RealT, 3> testVec {3, 2, 1};

    Vector<RealT, 3> restored = test.inverse().transform(test.transform(testVec));

    EXPECT_NEAR(testVec[0], restored[0], RealT(1e-5));
    EXPECT_NEAR(testVec[1], restored[1], RealT(1e-5));
    EXPECT_NEAR(testVec[2], restored[2], RealT(1e-5));
  }

}

