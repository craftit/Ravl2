#include <array>
//
// Created by charles galambos on 02/09/2024.
//

#include "Ravl2/Catch2checks.hh"
#include "Ravl2/Image/WarpScale.hh"
#include "Ravl2/Image/SummedAreaTable.hh"
#include "Ravl2/ArrayIterZip.hh"

namespace Ravl2
{
  TEST_CASE("WarpScale")
  {

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
      SPDLOG_TRACE("Sum: {}", sum);

      for(auto it = zip(res1, res2); it.valid(); ++it) {
        float s1 = it.template data<0>() + it.template data<1>();
        float dif = s1 - sum;
        float limit = (s1 + sum) * 5e-6f;
        if(std::abs(dif) >= limit) {
          SPDLOG_ERROR("err: {} {}  @ {}", dif, s1, it.index());
        }
        CHECK(std::abs(dif) < limit);
      }
    }

    SECTION("Scale Large")
    {
#if 0
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

  TEST_CASE("WarpScale stays inside the image")
  {
    // 10x10 by 3: output pixel 3 would average source 9..11; only 0..2 fit.
    Array<float, 2> img({10, 10}, 1.0f);
    Array<float, 2> sub;
    warpSubsample(sub, img, toVector<float>(3, 3));
    CHECK(sub.range() == IndexRange<2>({{0, 2}, {0, 2}}));
    CHECK(std::abs(sub[2][2] - 1.0f) < 1e-6f);

    // A result larger than the valid output is reallocated rather than filled from outside the image.
    Array<float, 2> big({5, 5}, -1.0f);
    warpSubsample(big, img, toVector<float>(3, 3));
    CHECK(big.range() == IndexRange<2>({{0, 2}, {0, 2}}));

    // Upsampling an image whose rows start at 100: rows used to be compared with the image size.
    Array<float, 2> rows(IndexRange<2>({{100, 104}, {0, 3}}));
    for(auto r : rows.range(0))
      for(auto c : rows.range(1))
        rows[r][c] = float(r);
    Array<float, 2> up;
    REQUIRE(warpScaleBilinear(up, rows, toVector<float>(0.5f, 0.5f)));
    CHECK(std::abs(up[201][0] - 100.5f) < 1e-5f);
  }

  TEST_CASE("warpScale enlarges to any size")
  {
    // Enlarging used in/out as the step, so the last output pixel sampled past the image and warpScale threw.
    // A ramp shows the corners map onto the corners and the edge pixels are read, not ones beyond them.
    for(auto [inRows, inCols, outRows, outCols] : {std::array {41, 61, 64, 64}, std::array {3, 7, 1000, 999},
                                                   std::array {2, 2, 64, 3}, std::array {64, 10, 32, 100}}) {
      INFO(inRows << "x" << inCols << " -> " << outRows << "x" << outCols);
      Array<uint8_t, 2> img({inRows, inCols});
      for(int r = 0; r < inRows; ++r)
        for(int c = 0; c < inCols; ++c)
          img[r][c] = uint8_t(r + 2 * c);
      Array<float, 2> result({outRows, outCols});
      REQUIRE(warpScale(result, img));
      CHECK(result.range() == IndexRange<2>({outRows, outCols}));
      CHECK(std::abs(result[0][0] - 0.0f) < 1e-3f);
      if(outCols > inCols)
        CHECK(std::abs(result[0][outCols - 1] - float(2 * (inCols - 1))) < 1e-2f);
      if(outRows > inRows && outCols > inCols)
        CHECK(std::abs(result[outRows - 1][outCols - 1] - float(inRows - 1 + 2 * (inCols - 1))) < 1e-2f);
    }
  }

  TEST_CASE("SummedAreaTable")
  {
    SECTION("Simple")
    {
      Ravl2::SetSPDLogLevel beQuiet(spdlog::level::off);
      Array<int, 2> img({5, 5}, 1);
      SummedAreaTable<unsigned> tab = SummedAreaTable<unsigned>::buildTable(img);
      SPDLOG_INFO("Table: {}", tab);
      CHECK(tab.sum(img.range()) == 25);
      IndexRange<2> srect(img.range());
      srect = srect.shrink(1);
      CHECK(tab.sum(srect) == 9);
    }

    SECTION("Rectangles")
    {
      Array<int, 2> img({5, 5}, 0);
      // Build a more interesting image.
      int sum = 1;
      for(auto &it : img)
        it = sum++;
      SummedAreaTable<unsigned> tab = SummedAreaTable<unsigned>::buildTable(img);
      IndexRange<2> rec2({{0, 1},{ 0, 1}});
      CHECK(tab.sum(rec2) == 16);

      IndexRange<2> rec3({{0, 1}, {1, 2}});
      CHECK(tab.sum(rec3) == 20) ;

      IndexRange<2> rec4({{1, 2}, {1, 2}});
      CHECK(tab.sum(rec4) == 40);
    }

    SECTION("Grid Scale 1")
    {
      Ravl2::SetSPDLogLevel beQuiet(spdlog::level::off);
      Array<int, 2> img({6, 6}, 0);
      int sum = 1;
      for(auto &it : img)
        it = sum++;

      SummedAreaTable<int> tab = SummedAreaTable<int>::buildTable(img);
      IndexRange<2> rec2({{1, 4}, {1, 4}});
      Array<int, 2> img2(rec2);
      tab.sampleGrid<int>(img2, toVector<float>(1,1));

      SPDLOG_INFO("Img: {}", img);
      SPDLOG_INFO("Img2: {}", img2);
      for(auto it = zip(img2,clip(img,img2.range()));it.valid();++it) {
        CHECK(it.template data<0>() == it.template data<1>());
      }

      // Try a full scale image.
      Array<int, 2> img3(img.range());
      tab.sampleGrid(img3, toVector<float>(1,1));

      SPDLOG_INFO("Img3: {}", img3);

      for(auto it = zip(img3,img);it.valid();++it) {
        CHECK(it.template data<0>() == it.template data<1>());
      }

    }

    SECTION("Grid Scale 2")
    {
      Ravl2::SetSPDLogLevel beQuiet(spdlog::level::off);
      Array<int, 2> imgOrg({6, 6}, 0);
      int sum = 1;
      for(auto &it : imgOrg)
        it = sum++;

      Array<int, 2> img({12, 12}, 0);
      for(auto it : img.range()) {
        img[it] = imgOrg[it[0]/2][it[1]/2];
      }

      SummedAreaTable<int> tab = SummedAreaTable<int>::buildTable(img);
      IndexRange<2> rec2({{1, 4}, {1, 4}});
      Array<int, 2> img2(rec2, -1);
      tab.sampleGrid(img2, toVector<float>(2,2));

      SPDLOG_INFO("ImgOrg: {}", imgOrg);
      SPDLOG_INFO("Img: {}", img);
      SPDLOG_INFO("Img2: {}", img2);
      for(auto it = zip(img2,clip(imgOrg,img2.range()));it.valid();++it) {
        CHECK(it.template data<0>() == it.template data<1>());
      }
    }

  }
}

