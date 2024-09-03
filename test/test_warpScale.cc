//
// Created by charles galambos on 02/09/2024.
//

#include "checks.hh"
#include "Ravl2/Image/WarpScale.hh"
#include "Ravl2/Image/SummedAreaTable.hh"
#include "Ravl2/ArrayIterZip.hh"

TEST_CASE("WarpScale")
{
  using namespace Ravl2;

  SECTION("Scale Small")
  {
    const int imgSize0 = 10;
    const int imgSize1 = 11;
    Array<float,2> img1({imgSize0, imgSize1});
    for(int r = 0; r < imgSize0; r++) {
      for(int c = 0; c < imgSize1; c++) {
        img1[r][c] = float(c);
      }
    }

    Array<float,2> img2({imgSize0, imgSize1});
    for(int r = 0; r < imgSize0; r++) {
      for(int c = 0; c < imgSize1; c++) {
        img2[r][c] = float(imgSize1 - c);
      }
    }

    Vector<float, 2> scale = toVector < float > (1.1, 2.12);
    Array<float, 2> res1;
    warpSubsample(res1, img1, scale);

    //SPDLOG_INFO("Image1: {}", res1);

    Array<float, 2> res2;
    warpSubsample(res2, img2, scale);
    //SPDLOG_INFO("Image2: {}", res2);
    //create image

    float sum = res1[0][0] + res2[0][0];
    SPDLOG_INFO("Sum: {}", sum);

    for(auto it = zip(res1, res2); it.valid(); ++it) {
      float s1 = it.template data<0>() + it.template data<1>();
      float dif = s1 - sum;
      float limit = (s1 + sum) * 5e-6f;
      if(std::abs(dif) >= limit) {
        SPDLOG_INFO("err: {} {}  @ {}", dif, s1, it.index());
      }
      CHECK(std::abs(dif) < limit);
    }
  }

  SECTION("Scale Large")
  {
    const int imgSize = 998;
    Array<float,2> img1({imgSize, imgSize});
    for(int r = 0; r < imgSize; r++) {
      for(int c = 0; c < imgSize; c++) {
        img1[r][c] = float(c);
      }
    }

    Array<float,2> img2({imgSize, imgSize});
    for(int r = 0; r < imgSize; r++) {
      for(int c = 0; c < imgSize; c++) {
        img2[r][c] = float(imgSize - c);
      }
    }

    //  warpScale(Array<OutT, 2> &result, const Array<InT, 2> &img)
    IndexRange<2> range({40,50});
    Array<float, 2> res1(range);
    warpScale(res1, img1);
#if 0

    Array<float, 2> res2(range);
    warpScale(res2, img2);
    //create image

    SPDLOG_INFO("Fr1: {}", res1.range());
    SPDLOG_INFO("Fr2: {}", res2.range());
    float sum = res1[0][0] + res2[0][0];
    SPDLOG_INFO("Sum: {}", sum);
    SPDLOG_INFO("Max: {} {} ",res1[res1.range().max()], res2[res2.range().max()]);
    SPDLOG_INFO("Max: {} {} ",res1[res1.range().max()+toIndex(-1,0)], res2[res2.range().max()+toIndex(-1,0)]);
    SPDLOG_INFO("Max: {} {} ",res1[res1.range().max()+toIndex(-2,0)], res2[res2.range().max()+toIndex(-2,0)]);
    SPDLOG_INFO("Max: {} {} ",res1[res1.range().max()+toIndex(0,-1)], res2[res2.range().max()+toIndex(0,-1)]);


    for(auto it = zip(res1, res2); it.valid(); ++it) {
      float s1 = it.template data<0>() + it.template data<1>();
      float dif = s1 - sum;
      float limit = (s1 + sum) * 5e-6f;
      if(std::abs(dif) >= limit) {
	SPDLOG_INFO("err: {} {}  @ {}", dif, s1, it.index());
      }
      CHECK(std::abs(dif) < limit);
    }
#endif
  }
}

TEST_CASE("SummedAreaTable")
{
  using namespace Ravl2;
  Array<int,2> img({5,5}, 1);
  SummedAreaTableC<unsigned> tab = SummedAreaTableC<unsigned>::BuildTable(img);
  SPDLOG_INFO("Table: {}", tab);
#if 0
  CHECK(tab.Sum(img.range()) == 25);
  IndexRange<2> srect(img.range());
  srect = srect.shrink(1);
  CHECK(tab.Sum(srect) == 9);

  // Build a more interesting image.
  int sum = 1;
  for(auto &it : img)
    it = sum++;
  tab = SummedAreaTableC<unsigned>::BuildTable(img);
  //cerr << img << "\n";
  //cerr << tab << "\n";
  IndexRange<2> rec2(0,1,0,1);
  //cerr <<"Sum=" << tab.Sum(rec2) << "\n";
  if(tab.Sum(rec2) != 16) return __LINE__;

  IndexRange<2> rec3(0,1,1,2);
  //cerr <<"Sum=" << tab.Sum(rec3) << "\n";
  if(tab.Sum(rec3) != 20) return __LINE__;

  IndexRange<2> rec4(1,2,1,2);
  //cerr <<"Sum=" << tab.Sum(rec4) << "\n";
  if(tab.Sum(rec4) != 40) return __LINE__;
#endif
}

