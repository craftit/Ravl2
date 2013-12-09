/*
 * testRavl2Core.cc
 *
 *  Created on: Dec 8, 2013
 *      Author: charlesgalambos
 */

#include <iostream>
#include <vector>

#include "Ravl2/Index.hh"
#include "Ravl2/ArrayAccess.hh"
#include "Ravl2/UnitTest.hh"


int testIndexRange();
int testArrayAccess();

int main(int nargs,char **argv)
{
  RAVL_RUN_TEST(testIndexRange());
  RAVL_RUN_TEST(testArrayAccess());

  std::cout << "Tests passed ok. " << std::endl;
  return 0;
}

int testIndexRange()
{
  {
    Ravl2::IndexRange<1> range1(10);
    RAVL_TEST_TRUE(range1.contains(1));
    RAVL_TEST_FALSE(range1.contains(10));
    int count = 0;
    for(int x : range1) {
      RAVL_TEST_EQUALS(x,count);
      RAVL_TEST_TRUE(range1.contains(x));
      count++;
    }
    RAVL_TEST_EQUALS(range1.size(),count);
  }

  {
    Ravl2::IndexRange<2> range2A{5,7};
    RAVL_TEST_EQUALS(range2A[0].size(),5);
    RAVL_TEST_EQUALS(range2A[0].min(),0);
    RAVL_TEST_EQUALS(range2A[0].max(),4);

    RAVL_TEST_EQUALS(range2A[1].size(),7);
    RAVL_TEST_EQUALS(range2A[1].min(),0);
    RAVL_TEST_EQUALS(range2A[1].max(),6);

    //std::cout << "Range: " << range2A << " \n";
    int count = 0;
    for(auto at : range2A) {
      count++;
      //std::cout << at << " \n";
      RAVL_TEST_TRUE(range2A.contains(at));
    }
    RAVL_TEST_EQUALS(range2A.elements(),count);
  }

  {
    Ravl2::IndexRange<2> range2B{{1,5},{3,7}};
    RAVL_TEST_EQUALS(range2B[0].size(),5);
    RAVL_TEST_EQUALS(range2B[0].min(),1);
    RAVL_TEST_EQUALS(range2B[0].max(),5);

    RAVL_TEST_EQUALS(range2B[1].size(),5);
    RAVL_TEST_EQUALS(range2B[1].min(),3);
    RAVL_TEST_EQUALS(range2B[1].max(),7);

    //std::cout << "Range: " << range2B << " \n";
    int count = 0;
    for(auto at : range2B) {
      count++;
      //std::cout << at << " \n";
      RAVL_TEST_TRUE(range2B.contains(at));
    }
    RAVL_TEST_EQUALS(range2B.elements(),count);
  }
  return 0;
}


int testArrayAccess()
{
  {
    // Test creation of 1 dimensional array.

    Ravl2::IndexRange<1> aRange(10);
    Ravl2::ArrayAccess<int,1> val(aRange);

    int c = 0;
    for(auto a : aRange) {
      val[a] = c++;
    }

    c = 0;
    for(auto a : aRange) {
      RAVL_TEST_EQUALS(val[a],c++);
    }

    c = 0;
    for(int i = aRange.min();i <= aRange.max();i++) {
      RAVL_TEST_EQUALS(val[i],c++);
    }

    RAVL_TEST_EQUALS(c,10);
  }

  {
    // Test creation of 2 dimensional array.

    Ravl2::IndexRange<2> aRange {10,11};
    Ravl2::ArrayAccess<int,2> val(aRange);

    // Write some data
    int c = 0;
    for(auto a : aRange) {
      val[a] = c++;
    }

    // Check what we wrote is still there.
    c = 0;
    for(auto a : aRange) {
      RAVL_TEST_EQUALS(val[a],c++);
    }

    // Index dimensions individually
    c = 0;
    for(auto i : aRange[0]) {
      for(auto j : aRange[1]) {
        RAVL_TEST_EQUALS(val[i][j],c++);
      }
    }

    // Using plain c style indexing.
    c = 0;
    for(int i = aRange[0].min();i <= aRange[0].max();i++) {
      for(int j = aRange[1].min();j <= aRange[1].max();j++) {
        RAVL_TEST_EQUALS(val[i][j],c++);
      }
    }


    RAVL_TEST_EQUALS(c,110);
  }

  {
    // Test creation of 3 dimensional array.

    Ravl2::IndexRange<3> aRange {10,11,12};
    Ravl2::ArrayAccess<int,3> val(aRange);

    // Write some data
    int c = 0;
    for(auto a : aRange) {
      val[a] = c++;
    }

    // Check what we wrote is still there.
    c = 0;
    for(auto a : aRange) {
      RAVL_TEST_EQUALS(val[a],c++);
    }

    // Index dimensions individually
    c = 0;
    for(auto i : aRange[0]) {
      for(auto j : aRange[1]) {
        for(auto k : aRange[2]) {
          RAVL_TEST_EQUALS(val[i][j][k],c++);
        }
      }
    }

    // Using plain c style indexing.
    c = 0;
    for(int i = aRange[0].min();i <= aRange[0].max();i++) {
      for(int j = aRange[1].min();j <= aRange[1].max();j++) {
        for(int k = aRange[2].min();k <= aRange[2].max();k++) {
          RAVL_TEST_EQUALS(val[i][j][k],c++);
        }
      }
    }

    RAVL_TEST_EQUALS(c,1320);
  }

  {
    // Test conversion from a c++ vector

    std::vector<int> acvec {1,2,3};
    Ravl2::ArrayAccess<int,1> anAccess(acvec);

    RAVL_TEST_EQUALS(anAccess.range().size(),3);

    RAVL_TEST_EQUALS(acvec[0],1);
    RAVL_TEST_EQUALS(acvec[1],2);
    RAVL_TEST_EQUALS(acvec[2],3);

    for(auto a : anAccess.range()) {
      //std::cout << " " << acvec[a] << "\n";
    }
  }


  return 0;
}


