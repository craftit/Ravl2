
#include <opencv2/core/types.hpp>
#include <opencv2/core/affine.hpp>

#include "Ravl2/Geometry/Geometry.hh"
#include "Ravl2/Geometry/Range.hh"
#include "Ravl2/Geometry/Affine.hh"

namespace Ravl2
{
  template<typename RealT>
  inline constexpr auto toPoint(const cv::Point_<RealT> &point) {
    return Point<RealT,2>(point.y, point.x);
  }

  template<typename RealT>
  inline constexpr auto toCvPoint(const Point<RealT,2> &point) {
    return cv::Point_<RealT>(point[1], point[0]);
  }

  template<typename RealT, int N,size_t M = N>
  cv::Vec<RealT, N> toCvVec(const Vector<RealT,M> &vec) {
    cv::Vec<RealT, N> result;
    for(unsigned i = 0; i < N; ++i) {
      result[int(i)] = vec[i];
    }
    return result;
  }

  template<typename RealT,size_t N, size_t M>
  cv::Mat toCvMat(const Matrix<RealT,N,M> &mat) {
    cv::Mat result(N, M, cv::DataType<RealT>::type);
    for(unsigned i = 0; i < N; ++i) {
      for(unsigned j = 0; j < M; ++j) {
	result.at<RealT>(int(i), int(j)) = mat(i, j);
      }
    }
    return result;
  }

  //! Convert to fixed size cv matrix
  template<typename RealT,unsigned N, unsigned M>
  cv::Matx<RealT, N, M> toCvMatx(const Matrix<RealT,N,M> &mat) {
    cv::Matx<RealT, N, M> result;
    for(unsigned i = 0; i < N; ++i) {
      for(unsigned j = 0; j < M; ++j) {
	result(i, j) = mat(i, j);
      }
    }
    return result;
  }

  //! To matrix
  template<typename RealT, unsigned N, unsigned M>
  Matrix<RealT,N,M> toMatrix(const cv::Mat &mat) {
    Matrix<RealT,N,M> result;
    // Check that the matrix is the right size
    if(mat.rows != N || mat.cols != M) {
      throw std::runtime_error("toMatrix: cv::Mat has wrong size");
    }
    for(unsigned i = 0; i < N; ++i) {
      for(unsigned j = 0; j < M; ++j) {
	result(i, j) = mat.at<RealT>(i, j);
      }
    }
    return result;
  }

  //! Fixed size matrix conversions
  template<typename RealT, unsigned N, unsigned M>
  Matrix<RealT,N,M> toMatrix(const cv::Matx<RealT, N, M> &mat) {
    Matrix<RealT,N,M> result;
    for(unsigned i = 0; i < N; ++i) {
      for(unsigned j = 0; j < M; ++j) {
	result(i, j) = mat(i, j);
      }
    }
    return result;
  }


  //! To Vector
  template<typename RealT, unsigned N>
  Vector<RealT,N> toVector(const cv::Vec<RealT, N> &vec)
  {
    Vector<RealT,N> result;
    for(unsigned i = 0; i < N; ++i) {
      result[i] = vec[i];
    }
    return result;
  }

  //! Convert to Affine<RealT,3>
  template<typename RealT>
  Affine<RealT,3> toAffine(const cv::Affine3<RealT> &affine)
  {
    return Affine<RealT, 3>(toMatrix<RealT, 3, 3>(affine.rvec), toVector<RealT, 3>(affine.tvec));
  }

  //! Convert to cv::Affine3
  template<typename RealT>
  cv::Affine3<RealT> toCvAffine(const Affine<RealT,3> &affine)
  {
    return cv::Affine3<RealT>(toCvMatx(affine.rotation()), toCvVec(affine.translation()));
  }

  //! Convert a cv::Rect to a Range
  template<typename RealT = float>
  inline constexpr auto toRange(const cv::Rect_<RealT> &rect) {
    return Range<RealT,2>(toPoint(rect.tl()), toPoint(rect.br()));
  }

  //! Convert a Range to a cv::Rect
  template<typename RealT>
  inline constexpr auto toCvRect(const Range<RealT,2> &range) {
    return cv::Rect_<RealT>(toCvPoint(range.min()), toCvPoint(range.max()));
  }

  //! Convert a cv::Size to a Vector
  template<typename RealT>
  inline constexpr auto toVector(const cv::Size_<RealT> &size) {
    return Vector<RealT,2>(size.height, size.width);
  }

  //! Convert a Vector to a cv::Size
  template<typename RealT>
  inline constexpr auto toCvSize(const Vector<RealT,2> &size) {
    return cv::Size_<RealT>(size[1], size[0]);
  }

  // Provide some common instantiations
  extern template cv::Vec<float, 2> toCvVec(const Vector<float, 2> &vec);
  extern template cv::Vec<float, 3> toCvVec(const Vector<float, 3> &vec);
  extern template cv::Vec<double, 2> toCvVec(const Vector<double, 2> &vec);
  extern template cv::Vec<double, 3> toCvVec(const Vector<double, 3> &vec);
  
  extern template cv::Mat toCvMat(const Matrix<float, 3, 3> &mat);
  extern template cv::Mat toCvMat(const Matrix<double, 3, 3> &mat);
  extern template cv::Mat toCvMat(const Matrix<float, 4, 4> &mat);
  extern template cv::Mat toCvMat(const Matrix<double, 4, 4> &mat);

}
