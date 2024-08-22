
#include "checks.hh"

#include "Ravl2/3D/PinholeCamera0.hh"

TEST_CASE("PinholeCamera0")
{
  using namespace Ravl2;

  SECTION("Cereal. ")
  {
    Matrix<float,3,3> rot  = xt::eye<float>(3);
    Vector<float,3> trans =  {5,6,7};
    PinholeCamera0<float> cam(1.0f, 2.0f, 3.0f, 4.0f, rot, trans, IndexRange<2>({{0, 100},{0, 100}}));
    std::stringstream ss;
    {
      cereal::JSONOutputArchive oarchive(ss);
      oarchive(cam);
    }
    //SPDLOG_INFO("Cam {}", ss.str());
    {
      cereal::JSONInputArchive iarchive(ss);
      PinholeCamera0<float> cam2;
      iarchive(cam2);
      EXPECT_FLOAT_EQ(cam.cx(), cam2.cx());
      EXPECT_FLOAT_EQ(cam.cy(), cam2.cy());
      EXPECT_FLOAT_EQ(cam.fx(), cam2.fx());
      EXPECT_FLOAT_EQ(cam.fy(), cam2.fy());
      EXPECT_TRUE(xt::allclose(cam.R(), cam2.R()));
      EXPECT_TRUE(xt::allclose(cam.t(), cam2.t()));
    }
  }

}
