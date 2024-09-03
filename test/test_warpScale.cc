//
// Created by charles galambos on 02/09/2024.
//

#include "checks.hh"
#include "Ravl2/Image/WarpScale.hh"
#include "Ravl2/ArrayIterZip.hh"

TEST_CASE("WarpScale")
{
  using namespace Ravl2;
  const int imgSize = 1000;
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

  SECTION("Scale 1")
  {
    Vector<float, 2> scale = toVector < float > (1.1, 2.12);
    Array<float, 2> res1;
    warpSubsample(img1, scale, res1);

    Array<float, 2> res2;
    warpSubsample(img2, scale, res2);
    //create image

    SPDLOG_INFO("Fr1: {}", res1.range());
    SPDLOG_INFO("Fr2: {}", res2.range());
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
}
