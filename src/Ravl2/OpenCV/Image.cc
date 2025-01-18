//
// Created by charles on 24/07/24.
//

#include "Ravl2/OpenCV/Image.hh"

namespace Ravl2
{
  template cv::Mat toCvMat(const Array<uint8_t, 2> &m);
  template cv::Mat toCvMat(const Array<float, 2> &m);
  template cv::Mat toCvMat(const Array<double, 2> &m);
  template cv::Mat toCvMat(const Array<int8_t, 3> &m);
  template cv::Mat toCvMat(const Array<int16_t, 3> &m);
  template cv::Mat toCvMat(const Array<uint16_t, 3> &m);

  template Array<uint8_t, 2> toArray<uint8_t, 2>(const cv::Mat &m);
  template Array<float, 2> toArray<float, 2>(const cv::Mat &m);
  template Array<double, 2> toArray<double, 2>(const cv::Mat &m);
  template Array<int8_t, 3> toArray<int8_t, 3>(const cv::Mat &m);
  template Array<int16_t, 3> toArray<int16_t, 3>(const cv::Mat &m);
  template Array<uint16_t, 3> toArray<uint16_t, 3>(const cv::Mat &m);

  template ArrayView<uint8_t, 2> toArrayView<uint8_t, 2>(const cv::Mat &m);
  template ArrayView<float, 2> toArrayView<float, 2>(const cv::Mat &m);
  template ArrayView<double, 2> toArrayView<double, 2>(const cv::Mat &m);
  template ArrayView<int8_t, 3> toArrayView<int8_t, 3>(const cv::Mat &m);
  template ArrayView<uint16_t, 3> toArrayView<uint16_t, 3>(const cv::Mat &m);
  template ArrayView<int16_t, 3> toArrayView<int16_t, 3>(const cv::Mat &m);

  //! Make to Array for  Array<PixelBGR8, N>
  template <>
  Array<PixelBGR8, 2> toArray(const cv::Mat &m)
  {
    if(m.type() != CV_8UC3) {
      SPDLOG_INFO("m.type() = {}", m.type());
      throw std::runtime_error("fromCvMat: cv::Mat has wrong data type");
    }
    IndexRange<2> indexRange;
    std::array<int, 2> strides {};
    for(unsigned i = 0; i < 2; ++i) {
      indexRange[i] = IndexRange<1>(0, m.size[int(i)] - 1);
      strides[i] = int(m.step[int(i)] / sizeof(PixelBGR8));
    }
    auto dataPtr = reinterpret_cast<PixelBGR8 *>(m.data);
    return Array<PixelBGR8, 2>(dataPtr, indexRange, strides,
                               std::shared_ptr<PixelBGR8[]>(dataPtr, [val = cv::Mat(m)]([[maybe_unused]] PixelBGR8 *delPtr) { assert(static_cast<void *>(delPtr) == val.data); }));
  }

  //! Make to Array for  Array<PixelBGRA8, N>
  template <>
  Array<PixelBGRA8, 2> toArray(const cv::Mat &m)
  {
    if(m.type() != CV_8UC4) {
      SPDLOG_INFO("m.type() = {}", m.type());
      throw std::runtime_error("fromCvMat: cv::Mat has wrong data type");
    }
    IndexRange<2> indexRange;
    std::array<int, 2> strides {};
    for(unsigned i = 0; i < 2; ++i) {
      indexRange[i] = IndexRange<1>(0, m.size[int(i)] - 1);
      strides[i] = int(m.step[int(i)] / sizeof(PixelBGR8));
    }
    auto dataPtr = reinterpret_cast<PixelBGRA8 *>(m.data);
    return Array<PixelBGRA8, 2>(dataPtr, indexRange, strides,
                               std::shared_ptr<PixelBGRA8[]>(dataPtr, [val = cv::Mat(m)]([[maybe_unused]] PixelBGRA8 *delPtr) { assert(static_cast<void *>(delPtr) == val.data); }));
  }


  template <>
  cv::Mat toCvMat(const Array<PixelBGR8, 2> &m)
  {
    cv::Mat img(cv::Size(m.range(1).size(), m.range(0).size()), CV_8UC3, addressOfMin(m));
    return img;
  }

}// namespace Ravl2