

//
// Created by charles galambos on 22/10/2023.
//

#include <catch2/catch_test_macros.hpp>
#include <spdlog/spdlog.h>
#include "Ravl2/Assert.hh"
#include "Ravl2/Geometry/Quaternion.hh"

#define CHECK_EQ(a, b) CHECK((a) == (b))
#define CHECK_NE(a, b) CHECK_FALSE((a) == (b))
#define EXPECT_TRUE(a) CHECK(a)
#define EXPECT_EQ(a, b) CHECK((a) == (b))
#define EXPECT_NE(a, b) CHECK_FALSE((a) == (b))
#define ASSERT_EQ(a, b) REQUIRE((a) == (b))
#define ASSERT_NE(a, b) REQUIRE_FALSE((a) == (b))
#define ASSERT_FLOAT_EQ(a, b) REQUIRE(Ravl2::isNearZero((a) -(b)))

#if 0
TEST(Quaternion, Basics)
{
  using namespace ReasonN;
  SetSPDLogLevel beQuiet(spdlog::level::off);

  // Setup a zero rotation
  Quaternion const q1 = Quaternion::fromAngleAxis(0.0, {0.0, 0.0, 1.0});

  // Check we've got a zero rotation
  Vector3f const e1{1.0, 2.0, 3.0};
  Vector3f rot = q1.rotate(e1);

  EXPECT_FLOAT_EQ(rot[0], 1.0);
  EXPECT_FLOAT_EQ(rot[1], 2.0);
  EXPECT_FLOAT_EQ(rot[2], 3.0);

  auto q1inv = q1.inverse();
  Vector3f irot = q1inv.rotate(rot);

  EXPECT_FLOAT_EQ(irot[0], 1.0);
  EXPECT_FLOAT_EQ(irot[1], 2.0);
  EXPECT_FLOAT_EQ(irot[2], 3.0);

  // Setup a 90 degree rotation about the z axis
  Quaternion const q2 = Quaternion::fromAngleAxis(float(M_PI/2.0), {0.0, 0.0, 1.0});

  // Check we've got the right rotation
  rot = q2.rotate(e1);
  EXPECT_FLOAT_EQ(rot[0], -2.0);
  EXPECT_FLOAT_EQ(rot[1], 1.0);
  EXPECT_FLOAT_EQ(rot[2], 3.0);

  auto q2inv = q2.inverse();
  irot = q2inv.rotate(rot);

  EXPECT_FLOAT_EQ(irot[0], 1.0);
  EXPECT_FLOAT_EQ(irot[1], 2.0);
  EXPECT_FLOAT_EQ(irot[2], 3.0);


  // Setup a 90 degree rotation about the x axis
  Quaternion const q3 = Quaternion::fromAngleAxis(float(M_PI/2.0), {1.0, 0.0, 0.0});

  // Check we've got the right rotation
  rot = q3.rotate(e1);
  EXPECT_FLOAT_EQ(rot[0], 1.0);
  EXPECT_FLOAT_EQ(rot[1], -3.0);
  EXPECT_FLOAT_EQ(rot[2], 2.0);

  auto q3inv = q3.inverse();
  irot = q3inv.rotate(rot);

  EXPECT_FLOAT_EQ(irot[0], 1.0);
  EXPECT_NEAR(irot[1], 2.0,1e-5);
  EXPECT_NEAR(irot[2], 3.0,1e-5);

  // Setup a 90 degree rotation about the y axis
  Quaternion const q4 = Quaternion::fromAngleAxis(float(M_PI/2.0), {0.0, 1.0, 0.0});

  // Check we've got the right rotation
  rot = q4.rotate(e1);
  EXPECT_FLOAT_EQ(rot[0], 3.0);
  EXPECT_FLOAT_EQ(rot[1], 2.0);
  EXPECT_NEAR(rot[2], -1.0,1e-5);

  auto q4inv = q4.inverse();
  irot = q4inv.rotate(rot);

  EXPECT_NEAR(irot[0], 1.0, 1e-6);
  EXPECT_FLOAT_EQ(irot[1], 2.0);
  EXPECT_FLOAT_EQ(irot[2], 3.0);

  // Check rotation composition
  Quaternion const q5 = q4inv * q4;
  rot = q5.rotate(e1);

  EXPECT_FLOAT_EQ(rot[0], 1.0);
  EXPECT_FLOAT_EQ(rot[1], 2.0);
  EXPECT_FLOAT_EQ(rot[2], 3.0);

}


TEST(Quaternion, Slerp)
{
  using namespace ReasonN;
  SetSPDLogLevel const beQuiet(spdlog::level::off);

  // Setup a zero rotation
  Quaternion const q1 = Quaternion::fromAngleAxis(0.0, {0.0, 0.0, 1.0});
  Quaternion const q2 = Quaternion::fromAngleAxis(float(M_PI/4.0), {0.0, 0.0, 1.0});

  // At 0 we should be close to q1
  Quaternion const q3 = q1.slerp(0,q2);
  EXPECT_FLOAT_EQ(q3.angle(),0);

  // At 1 we should be close to q2
  Quaternion const q4 = q1.slerp(1.0,q2);
  Quaternion const tq4 = q4 * q2.inverse();
  EXPECT_NEAR(tq4.angle(),0,1e-6);

  Quaternion q5 = q1.slerp(0.5, q2);
  EXPECT_FLOAT_EQ(q5.angle(),float(M_PI/16.0));

  // At 0.5 we should be half way between q1 and q2
  for(float f = 0;f < 1.0f;f += 0.1f) {
    Quaternion q6 = q1.slerp(f, q2);
    //q5.normalise();
    SPDLOG_INFO("Angle: {}",q6.angle());
  }
  //Quaternion const tq5 = q5 * q2.inverse();

}

TEST(Quaternion, EulerAngles)
{
  using namespace ReasonN;
  SetSPDLogLevel const beQuiet(spdlog::level::off);

  // Setup a zero rotation
  Quaternion const qZero = Quaternion::fromEulerAngles({0.0, 0.0, 0.0});
  EXPECT_FLOAT_EQ(qZero.angle(),0);

  // Setup a 45 degree rotation about the x axis
  Quaternion const q1 = Quaternion::fromEulerAngles({float(M_PI/4.0), 0.0, 0.0 });
  EXPECT_FLOAT_EQ(q1.angle(),float(M_PI/8.0));
  auto angles1 = q1.eulerAngles();
  EXPECT_FLOAT_EQ(angles1[0],float(M_PI/4.0));
  EXPECT_FLOAT_EQ(angles1[1],0.0);
  EXPECT_FLOAT_EQ(angles1[2],0.0);

  // Setup a 45 degree rotation about the y axis
  Quaternion const q2 = Quaternion::fromEulerAngles({0.0, float(M_PI/4.0), 0.0 });
  EXPECT_FLOAT_EQ(q1.angle(),float(M_PI/8.0));
  auto angles2 = q2.eulerAngles();
  EXPECT_FLOAT_EQ(angles2[0],0.0);
  EXPECT_FLOAT_EQ(angles2[1],float(M_PI/4.0));
  EXPECT_FLOAT_EQ(angles2[2],0.0);

  // Setup a 45 degree rotation about the z axis
  Quaternion const q3 = Quaternion::fromEulerAngles({0.0, 0.0, float(M_PI/4.0) });
  EXPECT_FLOAT_EQ(q1.angle(),float(M_PI/8.0));
  auto angles3 = q3.eulerAngles();
  EXPECT_FLOAT_EQ(angles3[0],0.0);
  EXPECT_FLOAT_EQ(angles3[1],0.0);
  EXPECT_FLOAT_EQ(angles3[2],float(M_PI/4.0));

  // Setup a combined rotation
  Quaternion const q4 = Quaternion::fromEulerAngles({float(M_PI/4.0), float(M_PI/5.0), float(M_PI/6.0) });
  auto angles4 = q4.eulerAngles();
  EXPECT_FLOAT_EQ(angles4[0],float(M_PI/4.0));
  EXPECT_FLOAT_EQ(angles4[1],float(M_PI/5.0));
  EXPECT_FLOAT_EQ(angles4[2],float(M_PI/6.0));

}


TEST(Quaternion, Inverse)
{
  using ReasonN::Quaternion;
  using ReasonN::Vector3f;
  using ReasonN::Vector3d;

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
    auto q = Quaternion::fromAngleAxis(float(dis(gen)), Vector3d{dis(gen), dis(gen), dis(gen)});

    auto restored = q.inverse().rotate(q.rotate(testVec));

    EXPECT_NEAR(xt::norm_l2(restored - testVec)(), 0.0, 1e-5);
  }
}

TEST(Isometry3d, Inverse)
{
  using ReasonN::Isometry3d;
  using ReasonN::Quaternion;
  using ReasonN::Vector3f;

  Isometry3d test(Quaternion::fromEulerAngles({float(M_PI/4.0), float(M_PI/5.0), float(M_PI/6.0) }), Vector3f {1,2,3});

  Vector3f testVec {3,2,1};

  Vector3f restored = test.inverse().transform(test.transform(testVec));

  EXPECT_NEAR(testVec[0],restored[0],1e-5);
  EXPECT_NEAR(testVec[1],restored[1],1e-5);
  EXPECT_NEAR(testVec[2],restored[2],1e-5);

}

#endif