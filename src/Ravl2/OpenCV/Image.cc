//
// Created by charles on 24/07/24.
//

#include "Ravl2/OpenCV/Image.hh"

namespace Ravl2
{
  template cv::Mat toCvMat(const Array<uint8_t, 2> &m);
  template cv::Mat toCvMat(const Array<float, 2> &m);
  template cv::Mat toCvMat(const Array<double, 2> &m);
  template cv::Mat toCvMat(const Array<uint8_t, 3> &m);
  
  template Array<uint8_t, 2> toArray<uint8_t, 2>(const cv::Mat &m);
  template Array<float, 2> toArray<float, 2>(const cv::Mat &m);
  template Array<double, 2> toArray<double, 2>(const cv::Mat &m);
  template Array<uint8_t, 3> toArray<uint8_t, 3>(const cv::Mat &m);
  
  template ArrayView<uint8_t, 2> toArrayView<uint8_t, 2>(const cv::Mat &m);
  template ArrayView<float, 2> toArrayView<float, 2>(const cv::Mat &m);
  template ArrayView<double, 2> toArrayView<double, 2>(const cv::Mat &m);
  template ArrayView<uint8_t, 3> toArrayView<uint8_t, 3>(const cv::Mat &m);
  
}