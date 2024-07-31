
#pragma once

#include <spdlog/spdlog.h>
#include <opencv2/core.hpp>
#include "Ravl2/Array.hh"

namespace Ravl2
{

  //! Create a cv::Mat from a Ravl2::Array
  //! The array data is referenced by the cv::Mat, so the array
  //! must exist for the lifetime of the cv::Mat
  template<typename ArrayT,typename DataT = typename ArrayT::value_type, unsigned N = ArrayT::dimensions>
  requires WindowedArray<ArrayT,DataT,N>
  cv::Mat toCvMat(const ArrayT& m)
  {
    Index<N> sizes = m.range().size();
    Index<N> minInd  = m.range().min();
    return cv::Mat(N, sizes.data(), cv::DataType<DataT>::type, reinterpret_cast<void*>(&m[minInd]));
  }

  //! Create a Ravl2::Array from a cv::Mat
  //! The type of the array must be the same as the cv::Mat
  //! This uses reference counting to ensure that the cv::Mat is not destroyed before the array

  template<typename DataT,unsigned N>
  Array<DataT,N> toArray(const cv::Mat& m)
  {
    IndexRange<N> sizes;
    if(m.dims != N)
      throw std::runtime_error("fromCvMat: cv::Mat has wrong number of dimensions");
    if(m.type() != cv::DataType<DataT>::type) {
      SPDLOG_INFO("m.type() = {}", m.type());
      throw std::runtime_error("fromCvMat: cv::Mat has wrong data type");
    }
    IndexRange<N> indexRange;
    std::array<int,N> strides;
    for (unsigned i = 0; i < N; ++i) {
      indexRange[i] = IndexRange<1>(0,m.size[int(i)]-1);
      strides[i] = m.step[int(i)] / sizeof(DataT);
    }
    auto dataPtr = reinterpret_cast<DataT*>(m.data);
    return Array<DataT,N>(dataPtr, indexRange, strides,
                          std::shared_ptr<DataT[]>(dataPtr,[val = cv::Mat(m)]([[maybe_unused]] DataT *delPtr)
                          {assert(static_cast<void *>(delPtr) == val.data);}));
  }

  //! Create a Ravl2::ArrayView from a cv::Mat
  //! The type of the array must be the same as the cv::Mat
  //! This is a view into the cv::Mat, so the cv::Mat must exist for the lifetime of the array view

  template<typename DataT,unsigned N>
  ArrayView<DataT,N> toArrayView(const cv::Mat& m)
  {
    IndexRange<N> sizes;
    if(m.dims != N)
      throw std::runtime_error("fromCvMat: cv::Mat has wrong number of dimensions");
    if(m.type() != cv::DataType<DataT>::type) {
      SPDLOG_INFO("m.type() = {}", m.type());
      throw std::runtime_error("fromCvMat: cv::Mat has wrong data type");
    }
    IndexRange<N> indexRange;
    std::array<int,N> strides;
    for (unsigned i = 0; i < N; ++i) {
      indexRange[i] = IndexRange<1>(0,m.size[int(i)]-1);
      strides[i] = m.step[int(i)] / sizeof(DataT);
    }
    return ArrayView<DataT,N>(reinterpret_cast<DataT*>(m.data), indexRange, strides);
  }



}