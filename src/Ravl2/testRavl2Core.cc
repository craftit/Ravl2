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

  //testArrayAccess();
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

    std::cout << "Range: " << range2B << " \n";
    int count = 0;
    for(auto at : range2B) {
      count++;
      std::cout << at << " \n";
      RAVL_TEST_TRUE(range2B.contains(at));
    }
    RAVL_TEST_EQUALS(range2B.elements(),count);
  }
  return 0;
}

std::vector<int> makeVector()
{
  std::vector<int> ret {1,2,3};
  return ret;
}

int testArrayAccess()
{
  Ravl2::ArrayAccess<int,1> val(Ravl2::IndexRange<1>(10));

  val[1] = 0;
  val[2] = 0;
  std::vector<int> vals = makeVector();
  Ravl2::ArrayAccess<int,1> anAccess(vals);

  for(int i = 0;i < anAccess.range().size();i++) {
    if(anAccess[i] != vals[i])
      return __LINE__;
  }

  return 0;
}


