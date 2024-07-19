/*
 * testRavl2Core.cc
 *
 *  Created on: Dec 8, 2013
 *      Author: charlesgalambos
 */

#include <iostream>
#include <vector>
#include <gtest/gtest.h>

#include "Ravl2/Math.hh"
#include "Ravl2/Index.hh"
#include "Ravl2/Array.hh"
#include "Ravl2/Geometry/Geometry.hh"

TEST(Ravl2, Math)
{
  // Check the int_floor
  ASSERT_EQ(Ravl2::int_floor(1.0),1);
  ASSERT_EQ(Ravl2::int_floor(0.1),0);
  ASSERT_EQ(Ravl2::int_floor(0.9),0);
  ASSERT_EQ(Ravl2::int_floor(-0.1),-1);
  ASSERT_EQ(Ravl2::int_floor(0.0),0);
  ASSERT_EQ(Ravl2::int_floor(-0.0),0);
  ASSERT_EQ(Ravl2::int_floor(-1.0),-1);
  ASSERT_EQ(Ravl2::int_floor(-2.0),-2);

  // Check the int_round
  ASSERT_EQ(Ravl2::int_round(0.1),0);
  ASSERT_EQ(Ravl2::int_round(0.9),1);
  ASSERT_EQ(Ravl2::int_round(-0.1),0);
  ASSERT_EQ(Ravl2::int_round(0.0),0);
  ASSERT_EQ(Ravl2::int_round(-0.0),0);
  ASSERT_EQ(Ravl2::int_round(-0.9),-1);

  // Check the sign
  ASSERT_EQ(Ravl2::sign(1),1);
  ASSERT_EQ(Ravl2::sign(0),0);
  ASSERT_EQ(Ravl2::sign(-1),-1);
  ASSERT_EQ(Ravl2::sign(0.1),1);
  ASSERT_EQ(Ravl2::sign(-0.1),-1);
  ASSERT_EQ(Ravl2::sign(0.0),0);

}


TEST(Ravl2, IndexRange)
{
  {
    Ravl2::IndexRange<1> range1(10);
    ASSERT_TRUE(range1.contains(1));
    ASSERT_FALSE(range1.contains(10));
    int count = 0;
    for(int x : range1) {
      ASSERT_EQ(x,count);
      ASSERT_TRUE(range1.contains(x));
      count++;
    }
    ASSERT_EQ(range1.size(),count);
  }

  {
    Ravl2::IndexRange<2> range2A{5,7};
    ASSERT_EQ(range2A[0].size(),5);
    ASSERT_EQ(range2A[0].min(),0);
    ASSERT_EQ(range2A[0].max(),4);

    ASSERT_EQ(range2A[1].size(),7);
    ASSERT_EQ(range2A[1].min(),0);
    ASSERT_EQ(range2A[1].max(),6);

    //std::cout << "Range: " << range2A << " \n";
    int count = 0;
    for(auto at : range2A) {
      count++;
      //std::cout << at << " \n";
      ASSERT_TRUE(range2A.contains(at));
    }
    ASSERT_EQ(range2A.elements(),count);
  }

  {
    Ravl2::IndexRange<2> range2B{{1,5},{3,7}};
    ASSERT_EQ(range2B[0].size(),5);
    ASSERT_EQ(range2B[0].min(),1);
    ASSERT_EQ(range2B[0].max(),5);

    ASSERT_EQ(range2B[1].size(),5);
    ASSERT_EQ(range2B[1].min(),3);
    ASSERT_EQ(range2B[1].max(),7);

    //std::cout << "Range: " << range2B << " \n";
    int count = 0;
    for(auto at : range2B) {
      count++;
      //std::cout << at << " \n";
      ASSERT_TRUE(range2B.contains(at));
    }
    ASSERT_EQ(range2B.elements(),count);
  }
}


TEST(Ravl2, Array)
{
  {
    // Test creation of 1 dimensional array.

    Ravl2::IndexRange<1> aRange(10);
    Ravl2::Array<int,1> val(aRange);

    int c = 0;
    for(auto a : aRange) {
      val[a] = c++;
    }

    c = 0;
    for(auto a : aRange) {
      ASSERT_EQ(val[a],c++);
    }

    c = 0;
    for(int i = aRange.min();i <= aRange.max();i++) {
      ASSERT_EQ(val[i],c++);
    }

    ASSERT_EQ(c,10);
  }

  {
    // Test creation of 2 dimensional array.

    Ravl2::IndexRange<2> aRange {10,11};

    // Create a 2 dimensional array of objects.
    Ravl2::Array<int,2> val(aRange);

    // Write some data
    int c = 0;
    for(auto a : aRange) {
      val[a] = c++;
    }

    // Check what we wrote is still there.
    c = 0;
    for(auto a : aRange) {
      ASSERT_EQ(val[a],c++);
    }

    // Index dimensions individually
    c = 0;
    for(auto i : aRange[0]) {
      for(auto j : aRange[1]) {
        ASSERT_EQ(val[i][j],c++);
      }
    }

    // Using plain c style indexing.
    c = 0;
    for(int i = aRange[0].min();i <= aRange[0].max();i++) {
      for(int j = aRange[1].min();j <= aRange[1].max();j++) {
        ASSERT_EQ(val[i][j],c++);
      }
    }


    ASSERT_EQ(c,110);
  }

  {
    // Test creation of 3 dimensional array.

    Ravl2::IndexRange<3> aRange {10,11,12};
    Ravl2::Array<int,3> val(aRange);

    // Write some data
    int c = 0;
    for(auto a : aRange) {
      val[a] = c++;
    }

    // Check what we wrote is still there.
    c = 0;
    for(auto a : aRange) {
      ASSERT_EQ(val[a],c++);
    }

    // Index dimensions individually
    c = 0;
    for(auto i : aRange[0]) {
      for(auto j : aRange[1]) {
        for(auto k : aRange[2]) {
          ASSERT_EQ(val[i][j][k],c++);
        }
      }
    }

    // Using plain c style indexing.
    c = 0;
    for(int i = aRange[0].min();i <= aRange[0].max();i++) {
      for(int j = aRange[1].min();j <= aRange[1].max();j++) {
        for(int k = aRange[2].min();k <= aRange[2].max();k++) {
          ASSERT_EQ(val[i][j][k],c++);
        }
      }
    }

    ASSERT_EQ(c,1320);
  }

  {
    // Test conversion from a c++ vector

    std::vector<int> acvec {1,2,3};
    Ravl2::Array<int,1> anAccess(acvec);

    ASSERT_EQ(anAccess.range().size(),3);

    ASSERT_EQ(acvec[0],1);
    ASSERT_EQ(acvec[1],2);
    ASSERT_EQ(acvec[2],3);

    for(auto a : anAccess.range()) {
      //std::cout << " " << acvec[a] << "\n";
    }
  }

}



TEST(Ravl2, ArrayIter1)
{
  Ravl2::Array<int,1> val(Ravl2::IndexRange<1>(10));
  int sum = 0;
  for(auto a : val.range()) {
    val[a] = a;
    sum += val[a];
  }
  ASSERT_EQ(sum,45);
  sum = 0;
  for(auto a : val) {
    sum += a;
  }
  ASSERT_EQ(sum,45);
}

TEST(Ravl2, ArrayAccessIter1)
{
  Ravl2::Array<int,1> val(Ravl2::IndexRange<1>(10));
  Ravl2::ArrayAccess<int,1> view(val);
  int sum = 0;
  for(auto a : view.range()) {
    val[a] = a;
    sum += val[a];
  }
  ASSERT_EQ(sum,45);
  sum = 0;
  for(auto a : view) {
    sum += a;
  }
  ASSERT_EQ(sum,45);
}

TEST(Ravl2, ArrayIter2)
{
  // Check 1x2 case
  {
    Ravl2::Array<int, 2> val(Ravl2::IndexRange<2>({1, 2}));
    int at = 0;
    for (auto a: val.range()) {
      val[a] = at++;
    }

    auto iter = val.begin();
    auto end = val.end();
    ASSERT_EQ(iter, iter);
    ASSERT_NE(iter, end);
    ASSERT_EQ(*iter, 0);
    ASSERT_TRUE(iter.valid());
    ++iter;
    ASSERT_EQ(iter, iter);
    ASSERT_NE(iter, end);
    ASSERT_EQ(*iter, 1);
    ASSERT_TRUE(iter.valid());
    ++iter;
    ASSERT_EQ(iter, end);
    ASSERT_FALSE(iter.valid());
  }

  // Check 2x1 case
  {
    Ravl2::Array<int, 2> val(Ravl2::IndexRange<2>({2, 1}));
    int at = 0;
    for (auto a: val.range()) {
      val[a] = at++;
    }

    auto iter = val.begin();
    auto end = val.end();
    ASSERT_EQ(iter, iter);
    ASSERT_NE(iter, end);
    ASSERT_EQ(*iter, 0);
    ASSERT_TRUE(iter.valid());
    ++iter;
    ASSERT_EQ(iter, iter);
    ASSERT_NE(iter, end);
    ASSERT_EQ(*iter, 1);
    ASSERT_TRUE(iter.valid());
    ++iter;
    ASSERT_EQ(iter, iter);
    ASSERT_EQ(iter, end);
    ASSERT_FALSE(iter.valid());
  }

  // Check 1x1 case
  {
    Ravl2::Array<int, 2> val(Ravl2::IndexRange<2>({1, 1}));
    int at = 0;
    for (auto a: val.range()) {
      val[a] = at++;
    }

    auto iter = val.begin();
    auto end = val.end();
    ASSERT_EQ(iter, iter);
    ASSERT_NE(iter, end);
    ASSERT_EQ(*iter, 0);
    ASSERT_TRUE(iter.valid());
    ++iter;
    ASSERT_EQ(iter, end);
    ASSERT_FALSE(iter.valid());
  }

  {
    Ravl2::Array<int, 2> val(Ravl2::IndexRange<2>({10, 20}));
    int sum = 0;
    int at = 0;
    for (auto a: val.range()) {
      val[a] = at++;
      sum += val[a];
    }

    auto iter = val.begin();
    ASSERT_EQ(*iter, 0);
    ASSERT_EQ(iter, iter);

    auto iterEnd = val.end();
    ASSERT_NE(iter, iterEnd);

    ++iter;
    ASSERT_EQ(*iter, 1);
    ASSERT_NE(iter, iterEnd);
    ASSERT_EQ(sum, 19900);
    sum = 0;
    for (auto a: val) {
      sum += a;
    }
    ASSERT_EQ(sum, 19900);
  }
}

TEST(Ravl2, ArrayViewIter2)
{

}
