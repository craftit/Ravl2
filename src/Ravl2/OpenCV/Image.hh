
#pragma once

#include <spdlog/spdlog.h>
#include <opencv2/core.hpp>
#include "Ravl2/IndexRange.hh"
#include "Ravl2/Array.hh"
#include "Ravl2/Pixel/Pixel.hh"

namespace Ravl2
{

  //! Create a cv::Mat from a Ravl2::Array
  //! The array data is referenced by the cv::Mat, so the array
  //! must exist for the lifetime of the cv::Mat
  template <typename ArrayT, typename DataT = typename ArrayT::value_type, unsigned N = ArrayT::dimensions>
    requires WindowedArray<ArrayT, DataT, N>
  cv::Mat toCvMat(const ArrayT &m)
  {
    Index<N> sizes = m.range().size();
    return cv::Mat(N, sizes.data(), cv::DataType<DataT>::type, addressOfMin(m));
  }

  //! Create a Ravl2::Array from a cv::Mat
  //! The type of the array must be the same as the cv::Mat
  //! This uses reference counting to ensure that the cv::Mat is not destroyed before the array

  template <typename PixelT, unsigned N>
  Array<PixelT, N> toArray(const cv::Mat &m)
  {
    if(m.dims != N)
      throw std::runtime_error("fromCvMat: cv::Mat has wrong number of dimensions");
    if(m.type() != cv::DataType<PixelT>::type) {
      SPDLOG_INFO("m.type() = {}", m.type());
      throw std::runtime_error("fromCvMat: cv::Mat has wrong data type");
    }
    IndexRange<N> indexRange;
    std::array<int, N> strides;
    for(unsigned i = 0; i < N; ++i) {
      indexRange[i] = IndexRange<1>(0, m.size[int(i)] - 1);
      strides[i] = int(m.step[int(i)] / sizeof(PixelT));
    }
    auto dataPtr = reinterpret_cast<PixelT *>(m.data);
    return Array<PixelT, N>(dataPtr, indexRange, strides,
                            std::shared_ptr<PixelT[]>(dataPtr, [val = cv::Mat(m)]([[maybe_unused]] PixelT *delPtr) { assert(static_cast<void *>(delPtr) == val.data); }));
  }

  //! Make to Array for  Array<PixelBGR8, N>
  template <>
  Array<PixelBGR8, 2> toArray(const cv::Mat &m);

  //! Make to Array for  Array<PixelBGRA8, N>
  template <>
  Array<PixelBGRA8, 2> toArray(const cv::Mat &m);

  //! Make to cv::Mat for  Array<PixelBGR8, N>
  template <>
  cv::Mat toCvMat(const Array<PixelBGR8, 2> &m);

  //! Create a Ravl2::ArrayView from a cv::Mat
  //! The type of the array must be the same as the cv::Mat
  //! This is a view into the cv::Mat, so the cv::Mat must exist for the lifetime of the array view

  template <typename DataT, unsigned N>
  ArrayView<DataT, N> toArrayView(const cv::Mat &m)
  {
    if(m.dims != N)
      throw std::runtime_error("fromCvMat: cv::Mat has wrong number of dimensions");
    if(m.type() != cv::DataType<DataT>::type) {
      SPDLOG_INFO("m.type() = {}", m.type());
      throw std::runtime_error("fromCvMat: cv::Mat has wrong data type");
    }
    IndexRange<N> indexRange;
    std::array<int, N> strides;
    for(unsigned i = 0; i < N; ++i) {
      indexRange[i] = IndexRange<1>(0, m.size[int(i)] - 1);
      strides[i] = int(m.step[int(i)] / sizeof(DataT));
    }
    return ArrayView<DataT, N>(reinterpret_cast<DataT *>(m.data), indexRange, strides);
  }

  // Instantiate the template functions for the types we need often
  extern template cv::Mat toCvMat(const Array<uint8_t, 2> &m);
  extern template cv::Mat toCvMat(const Array<float, 2> &m);
  extern template cv::Mat toCvMat(const Array<double, 2> &m);
  //extern template cv::Mat toCvMat(const Array<int8_t, 2> &m);
  extern template cv::Mat toCvMat(const Array<uint16_t, 3> &m);
  extern template cv::Mat toCvMat(const Array<int16_t, 3> &m);

  extern template Array<uint8_t, 2> toArray<uint8_t, 2>(const cv::Mat &m);
  extern template Array<float, 2> toArray<float, 2>(const cv::Mat &m);
  extern template Array<double, 2> toArray<double, 2>(const cv::Mat &m);
  extern template Array<int8_t, 3> toArray<int8_t, 3>(const cv::Mat &m);
  extern template Array<uint16_t, 3> toArray<uint16_t, 3>(const cv::Mat &m);
  extern template Array<int16_t, 3> toArray<int16_t, 3>(const cv::Mat &m);

  extern template ArrayView<uint8_t, 2> toArrayView<uint8_t, 2>(const cv::Mat &m);
  extern template ArrayView<float, 2> toArrayView<float, 2>(const cv::Mat &m);
  extern template ArrayView<double, 2> toArrayView<double, 2>(const cv::Mat &m);
  extern template ArrayView<int8_t, 3> toArrayView<int8_t, 3>(const cv::Mat &m);
  extern template ArrayView<uint16_t, 3> toArrayView<uint16_t, 3>(const cv::Mat &m);
  extern template ArrayView<int16_t, 3> toArrayView<int16_t, 3>(const cv::Mat &m);

}// namespace Ravl2