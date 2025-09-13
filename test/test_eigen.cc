
#include <spdlog/spdlog.h>
#include <catch2/catch_test_macros.hpp>
#include <eigen3/Eigen/Dense>

#include "Ravl2/Catch2checks.hh"
#include "Ravl2/Math.hh"
#include "Ravl2/Eigen.hh"
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

    {
      auto anArray = asArrayView(vec);
      STATIC_REQUIRE(anArray.dimensions == 1);
      CHECK(anArray.range(0).size() == 3);
      EXPECT_FLOAT_EQ(anArray[0], 1.1f);
      EXPECT_FLOAT_EQ(anArray[1], 2.2f);
      EXPECT_FLOAT_EQ(anArray[2], 3.3f);
    }
    {
      // The view is transposed because Eigen is column major
      auto anArray = asArrayView(static_cast<const decltype(vec) &>(vec));
      STATIC_REQUIRE(anArray.dimensions == 1);
      CHECK(anArray.range(0).size() == 3);
      EXPECT_FLOAT_EQ(anArray[0], 1.1f);
      EXPECT_FLOAT_EQ(anArray[1], 2.2f);
      EXPECT_FLOAT_EQ(anArray[2], 3.3f);
    }

  }
  SECTION("Cereal Vec<1,3>")
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
    {
      // The view is transposed because Eigen is column major
      auto anArray = asArrayView(vec);
      STATIC_REQUIRE(anArray.dimensions == 2);
      CHECK(anArray.range(0).size() == vec.cols());
      CHECK(anArray.range(1).size() == vec.rows());
      for(int i = 0; i < vec.rows(); i++) {
        for(int j = 0; j < vec.cols(); j++) {
          EXPECT_FLOAT_EQ(vec(i, j), anArray(j, i));
        }
      }
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

    {
      // The view is transposed because Eigen is column major
      auto anArray = asArrayView(vec);
      STATIC_REQUIRE(anArray.dimensions == 2);
      CHECK(anArray.range(0).size() == vec.cols());
      CHECK(anArray.range(1).size() == vec.rows());
      for(int i = 0; i < vec.rows(); i++) {
        for(int j = 0; j < vec.cols(); j++) {
          EXPECT_FLOAT_EQ(vec(i, j), anArray(j, i));
        }
      }
    }

    {
      // The view is transposed because Eigen is column major
      auto anArray = asArrayView(static_cast<const decltype(vec) &>(vec));
      STATIC_REQUIRE(anArray.dimensions == 2);
      CHECK(anArray.range(0).size() == vec.cols());
      CHECK(anArray.range(1).size() == vec.rows());
      for(int i = 0; i < vec.rows(); i++) {
        for(int j = 0; j < vec.cols(); j++) {
          EXPECT_FLOAT_EQ(vec(i, j), anArray(j, i));
        }
      }
    }
  }
  SECTION("Cereal Matrix<2,Dyn>")
  {
    Eigen::Matrix<float, 2, Eigen::Dynamic> vec = Eigen::Matrix<float, 2, Eigen::Dynamic>::Random(2, 3);

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

    {
      // The view is transposed because Eigen is column major
      auto anArray = asArrayView(vec);
      STATIC_REQUIRE(anArray.dimensions == 2);
      CHECK(anArray.range(0).size() == vec.cols());
      CHECK(anArray.range(1).size() == vec.rows());
      for(int i = 0; i < vec.rows(); i++) {
        for(int j = 0; j < vec.cols(); j++) {
          EXPECT_FLOAT_EQ(vec(i, j), anArray(j, i));
        }
      }
    }

    {
      // The view is transposed because Eigen is column major
      auto anArray = asArrayView(static_cast<const decltype(vec) &>(vec));
      STATIC_REQUIRE(anArray.dimensions == 2);
      CHECK(anArray.range(0).size() == vec.cols());
      CHECK(anArray.range(1).size() == vec.rows());
      for(int i = 0; i < vec.rows(); i++) {
        for(int j = 0; j < vec.cols(); j++) {
          EXPECT_FLOAT_EQ(vec(i, j), anArray(j, i));
        }
      }
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

    {
      // The view is transposed because Eigen is column major
      auto anArray = asArrayView(vec);
      STATIC_REQUIRE(anArray.dimensions == 2);
      CHECK(anArray.range(0).size() == vec.cols());
      CHECK(anArray.range(1).size() == vec.rows());
      for(int i = 0; i < vec.rows(); i++) {
        for(int j = 0; j < vec.cols(); j++) {
          EXPECT_FLOAT_EQ(vec(i, j), anArray(j, i));
        }
      }
    }

    {
      // The view is transposed because Eigen is column major
      auto anArray = asArrayView(static_cast<const decltype(vec) &>(vec));
      STATIC_REQUIRE(anArray.dimensions == 2);
      CHECK(anArray.range(0).size() == vec.cols());
      CHECK(anArray.range(1).size() == vec.rows());
      for(int i = 0; i < vec.rows(); i++) {
        for(int j = 0; j < vec.cols(); j++) {
          EXPECT_FLOAT_EQ(vec(i, j), anArray(j, i));
        }
      }
    }

  }

  
  
  
}
