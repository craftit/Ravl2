//
// Created by charles galambos on 09/02/2025.
//

#include "Ravl2/Catch2checks.hh"
#include "Ravl2/SpatialHash.hh"
#include "Ravl2/Geometry/Geometry.hh"

TEST_CASE("SpatialHash")
{
  SECTION("SpatialHash 2D")
  {
    using namespace Ravl2;
    // Test creation of 2 dimensional array.

    Ravl2::SpatialHash<float,2,int> hash({1,1});

    int c = 0;
    for(int i = 0; i < 10; i++) {
      for(int j = 0; j < 10; j++) {
        hash.insert(toPoint<float>(i,j),c++);
      }
    }

    // Check the value of the hash
    for(int i = 0; i < 10; i++) {
      for(int j = 0; j < 10; j++) {
        auto val = hash.closest(toPoint<float>(i,j),0.2f);
        CHECK(val.has_value());
        CHECK_EQ(*val,i*10+j);
      }
    }

    // Check the index function
    CHECK_EQ(hash.index({0.4,0.4}),Ravl2::Index<2>({0,0}));
    CHECK_EQ(hash.index({0.6,0.6}),Ravl2::Index<2>({1,1}));
    CHECK_EQ(hash.index({-0.6,-0.6}),Ravl2::Index<2>({-1,-1}));
    CHECK_EQ(hash.index({9.4,9.4}),Ravl2::Index<2>({9,9}));
    CHECK_EQ(hash.index({10.5,10.5}),Ravl2::Index<2>({10,10}));
    CHECK_EQ(hash.index({10.4,10.4}),Ravl2::Index<2>({10,10}));
    CHECK_EQ(hash.index({10.6,10.6}),Ravl2::Index<2>({11,11}));

    // Check the center function
    // CHECK_EQ(hash.center({0,0}),Ravl2::Vector<float,2>({0.5,0.5}));
    // CHECK_EQ(hash.center({1,1}),Ravl2::Vector<float,2>({1.5,1.5}));
    // CHECK_EQ(hash.center({-1,-1}),Ravl2::Vector<float,2>({-0.5,-0.5}));
    // CHECK_EQ(hash.center({9,9}),Ravl2::Vector<float,2>({9.5,9.5}));
    // CHECK_EQ(hash.center({10,10}),Ravl2::Vector<float,2>({10.5,10.5}));
  }
}

