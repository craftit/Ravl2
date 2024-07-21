// This file is part of RAVL, Recognition And Vision Library 
// Copyright (C) 2001, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here
//! rcsid="$Id$"
//! lib=RavlImageProc
//! file="Ravl/Image/Processing/Segmentation/testConnectedComponents.cc"
//! docentry="Ravl.API.Images.Segmentation"
//! userlevel=Develop
//! author="Charles Galambos"

#include "Ravl2/Image/Segmentation/ConnectedComponents.hh"

using namespace RavlImageN;

int testConnectComp();

int main(int nargs,char **argv)
{
  int ln;
  if((ln = testConnectComp()) != 0) {
    std::cerr << "Test failed at " << ln << "\n";
    return 1;
  }
    
  return 0;
}


int testConnectComp() {
  Ravl2::Array<unsigned, 2> test(8,8);
  test.fill(0);
  
  test[1][1] = 1;
  test[1][2] = 1;
  test[2][1] = 1;
  test[6][6] = 1;
  test[5][6] = 1;
  //cerr << test;
  ConnectedComponentsC<unsigned> conComp(false);
  auto result = conComp.apply(test);
  ImageC<unsigned> segMap = result.Data1();
  //cerr << "Regions=" << result.Data2() << "\n";
  //cerr << segMap;
  if(result.Data2() != 4) return __LINE__;
  if(segMap[1][1] != segMap[1][2]) return __LINE__;
  if(segMap[1][2] != segMap[2][1]) return __LINE__;
  if(segMap[6][6] != segMap[5][6]) return __LINE__;
  if(segMap[6][6] == segMap[1][1]) return __LINE__;
  if(segMap[6][6] == segMap[0][0]) return __LINE__;
  if(segMap[1][2] == segMap[0][0]) return __LINE__;
  return 0;
}
