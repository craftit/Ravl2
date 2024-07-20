
#include "Ravl/Image/SegmentExtrema.hh"
#include "Ravl/Image/DrawFrame.hh"
#include "Ravl/IO.hh"
#include "Ravl/Image/ConnectedComponents.hh"
#include "Ravl/Image/FloodRegion.hh"

using namespace RavlImageN;

int testConnectComp();
int testFloodRegion();
int testSegmentExtrema();

int main() {
  int ln;
  if((ln = testFloodRegion()) != 0) {
    cerr << "Test failed on line " << ln << "\n";
    return 1;
  }
#if 1
  if((ln = testConnectComp()) != 0) {
    cerr << "Test failed at " << ln << "\n";
    return 1;
  }
  if((ln = testSegmentExtrema()) != 0) {
    cerr << "Test failed on line " << ln << "\n";
    return 1;
  }
#endif
  return 0;
}


int testConnectComp() {
  ImageC<UIntT> test(8,8);
  test.Fill(0);
  
  test[1][1] = 1;
  test[1][2] = 1;
  test[2][1] = 1;
  test[6][6] = 1;
  test[5][6] = 1;
  //cerr << test;
  ConnectedComponentsC<UIntT> conComp(false);
  Tuple2C<ImageC<UIntT>,UIntT> result = conComp.Apply(test);
  ImageC<UIntT> segMap = result.Data1();
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

int testFloodRegion() {
  ImageC<ByteT> img(10,10);
  img.Fill(0);
  DrawFrame(img,(ByteT) 9,IndexRange2dC(1,8,1,8),false);
  DrawFrame(img,(ByteT) 9,IndexRange2dC(3,6,3,6),false);
  img[6][5] = 0;
  cerr << "Orig=" << img << "\n";
  ImageC<ByteT> mask;
  
  FloodRegionC<ByteT> floodRegion(img);
  
  floodRegion.GrowRegion(Index2dC(5,5),5,mask);
  if(!mask.Contains(Index2dC(2,5))) return __LINE__;
  if(mask[2][5] == 0) return __LINE__;
  cerr << mask << "\n";

  return 0;
}

int testSegmentExtrema() {
  ImageC<ByteT> img(100,100);
  img.Fill(196);
  DrawFrame(img,(ByteT) 128,IndexRange2dC(10,90,10,90),true);
  DrawFrame(img,(ByteT) 196,IndexRange2dC(20,30,20,30),true);
  DrawFrame(img,(ByteT) 64,IndexRange2dC(20,30,40,50),true);
  DrawFrame(img,(ByteT) 196,IndexRange2dC(40,50,40,50),true);
  SegmentExtremaC<ByteT> segExt(5);
  DListC<BoundaryC> bnd = segExt.Apply(img);
  DListC<ImageC<IntT> > segs = segExt.ApplyMask(img);
  cerr << "Bounds=" << bnd.Size() << " " << segs.Size() << "\n";

  for(DLIterC<ImageC<IntT> > it(segs);it;it++) {
    IndexRange2dC frame = it->Frame();
    frame.ClipBy(img.Frame());
    for(Array2dIter2C<ByteT,IntT> iit(img,*it,frame);iit;iit++) 
      if(iit.Data2() != 0) iit.Data1() = 255;
  }
  //Save("@X",img);
  
  return 0;
}
