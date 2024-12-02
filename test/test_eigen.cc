
#include <spdlog/spdlog.h>
#include <catch2/catch_test_macros.hpp>
#include <eigen3/Eigen/Dense>

#include "Ravl2/Math.hh"
#include "Ravl2/IO/Cereal.hh"

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
  
  SECTION("Cereal Vec<3,1>")
  {
    Eigen::Matrix<float, 3, 1> vec {{1.1f,2.2f,3.3f}};
    std::stringstream ss;
    {
      cereal::JSONOutputArchive oarchive(ss);
      oarchive(vec);
    }
    //SPDLOG_INFO("VecString: {}", ss.str());
    {
      cereal::JSONInputArchive iarchive(ss);
      Eigen::Matrix<float, 3, 1> vec2;
      iarchive(vec2);
      CHECK(vec.isApprox(vec2));
    }
  }
  SECTION("Cereal Vec<3,1>")
  {
    Eigen::Matrix<float, 1, 3> vec {1.1f,2.2f,3.3f};
    std::stringstream ss;
    {
      cereal::JSONOutputArchive oarchive(ss);
      oarchive(vec);
    }
    //SPDLOG_INFO("VecString: {}", ss.str());
    {
      cereal::JSONInputArchive iarchive(ss);
      Eigen::Matrix<float, 1, 3> vec2;
      iarchive(vec2);
      //SPDLOG_INFO("Vec: {} -> {}", vec, vec2);
      CHECK(vec.isApprox(vec2));
    }
  }
  SECTION("Cereal Matrix<Dyn,Dyn>")
  {
    Eigen::Matrix<float, Eigen::Dynamic, Eigen::Dynamic> vec = Eigen::Matrix<float, Eigen::Dynamic, Eigen::Dynamic>::Random(2,3);
    
    std::stringstream ss;
    {
      cereal::JSONOutputArchive oarchive(ss);
      oarchive(vec);
    }
    //SPDLOG_INFO("VecString: '{}'", ss.str());
    {
      cereal::JSONInputArchive iarchive(ss);
      Eigen::Matrix<float, Eigen::Dynamic, Eigen::Dynamic> vec2;
      iarchive(vec2);
      //SPDLOG_INFO("Vec: {} -> {}", vec, vec2);
      CHECK(vec.isApprox(vec2));
    }
  }
  SECTION("Cereal Matrix<2,Dyn>")
  {
    Eigen::Matrix<float, 2, Eigen::Dynamic> vec = Eigen::Matrix<float, 2, Eigen::Dynamic>::Random(2,3);

    std::stringstream ss;
    {
      cereal::JSONOutputArchive oarchive(ss);
      oarchive(vec);
    }
    //SPDLOG_INFO("VecString: '{}'", ss.str());
    {
      cereal::JSONInputArchive iarchive(ss);
      Eigen::Matrix<float, 2, Eigen::Dynamic> vec2;
      iarchive(vec2);
      //SPDLOG_INFO("Vec: {} -> {}", vec, vec2);
      CHECK(vec.isApprox(vec2));
    }
  }

  SECTION("Cereal Matrix<Dyn,2>")
  {
    Eigen::Matrix<float, Eigen::Dynamic, 2> vec = Eigen::Matrix<float, Eigen::Dynamic, 2>::Random(3,2);

    std::stringstream ss;
    {
      cereal::JSONOutputArchive oarchive(ss);
      oarchive(vec);
    }
    //SPDLOG_INFO("VecString: '{}'", ss.str());
    {
      cereal::JSONInputArchive iarchive(ss);
      Eigen::Matrix<float, Eigen::Dynamic, 2> vec2;
      iarchive(vec2);
      //SPDLOG_INFO("Vec: {} -> {}", vec, vec2);
      CHECK(vec.isApprox(vec2));
    }
  }

  
  
  
}
