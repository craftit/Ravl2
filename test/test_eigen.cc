
#include <catch2/catch_test_macros.hpp>
#include <eigen3/Eigen/Dense>

#include "Ravl2/Math.hh"

TEST_CASE("EigenIntegration")
{
  using namespace Ravl2;

  SECTION("Sizes")
  {

    Eigen::Matrix<float, 3, 1> vec;
    CHECK(sizeof(vec) == 3*sizeof(float));

    Eigen::Matrix<float, 3, 3> mat {{1,2,3},{4,5,6},{7,8,9}};
    CHECK(sizeof(mat) == 3*3*sizeof(float));
    CHECK(mat(0,1) == 2);
    CHECK(mat(1,0) == 4);

  }



}
