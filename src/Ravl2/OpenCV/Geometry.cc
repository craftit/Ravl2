//
// Created by charles galambos on 04/08/2024.
//

#include "Ravl2/OpenCV/Geometry.hh"

namespace Ravl2
{
  template cv::Vec<float, 2> toCvVec(const Vector<float, 2> &vec);
  template cv::Vec<float, 3> toCvVec(const Vector<float, 3> &vec);
  template cv::Vec<double, 2> toCvVec(const Vector<double, 2> &vec);
  template cv::Vec<double, 3> toCvVec(const Vector<double, 3> &vec);

  template cv::Mat toCvMat(const Matrix<float, 3, 3> &mat);
  template cv::Mat toCvMat(const Matrix<double, 3, 3> &mat);
  template cv::Mat toCvMat(const Matrix<float, 4, 4> &mat);
  template cv::Mat toCvMat(const Matrix<double, 4, 4> &mat);

}// namespace Ravl2