//! @file Geometry.hh
//! @brief Conversions between Ravl2 and OpenCV geometry types.
//!
//! @section coord_conventions Coordinate conventions
//!
//! **2D image points:**
//! Ravl2 uses (row, col) ordering — index 0 is the vertical axis, index 1 is horizontal.
//! OpenCV uses (x, y) ordering — x is horizontal, y is vertical.
//! All 2D point conversions (toImagePoint, toCvImagePoint, toRange, toCvRect, toVector(cv::Size), toCvSize)
//! swap the two components to account for this.
//!
//! **3D world points:**
//! Both Ravl2 and OpenCV use (x, y, z) ordering for 3D points.
//! toCvPoint3() does NOT swap — the conventions already match.
//!
//! **Camera intrinsics:**
//! PinholeCamera0::intrinsicMatrix<OpenCV>() returns a matrix with fx/fy and cx/cy
//! swapped relative to the Native format, so that the matrix works directly with
//! OpenCV functions like cv::solvePnP when the image points have been converted
//! via toCvPoint().

#pragma once

#include <opencv2/core/types.hpp>
#include <opencv2/core/affine.hpp>
#include <opencv2/calib3d.hpp>

#include "Ravl2/Geometry/Geometry.hh"
#include "Ravl2/Geometry/Range.hh"
#include "Ravl2/Geometry/Affine.hh"
#include "Ravl2/Geometry/Isometry3.hh"
#include "Ravl2/Geometry/Quaternion.hh"

namespace Ravl2
{
  //! Convert OpenCV 2D image point to Ravl2 point.
  //! Swaps (x,y) → (row,col): OpenCV x→col (index 1), OpenCV y→row (index 0).
  template <typename RealT>
  inline constexpr auto toImagePoint(const cv::Point_<RealT> &point)
  {
    return Point<RealT, 2>(point.y, point.x);
  }

  //! @deprecated Use toImagePoint() instead.
  template <typename RealT>
  [[deprecated("Use toImagePoint() instead — makes coordinate swap explicit")]]
  inline constexpr auto toPoint(const cv::Point_<RealT> &point)
  {
    return toImagePoint(point);
  }

  //! Convert Ravl2 2D image point to OpenCV point.
  //! Swaps (row,col) → (x,y): index 1 (col)→x, index 0 (row)→y.
  template <typename RealT>
  inline constexpr auto toCvImagePoint(const Point<RealT, 2> &point)
  {
    return cv::Point_<RealT>(point[1], point[0]);
  }

  //! @deprecated Use toCvImagePoint() instead.
  template <typename RealT>
  [[deprecated("Use toCvImagePoint() instead — makes coordinate swap explicit")]]
  inline constexpr auto toCvPoint(const Point<RealT, 2> &point)
  {
    return toCvImagePoint(point);
  }

  //! Convert Ravl2 Vector to OpenCV Vec. No coordinate swap — elements are copied as-is.
  template <typename RealT, int N, IndexSizeT M = N>
  cv::Vec<RealT, N> toCvVec(const Vector<RealT, M> &vec)
  {
    cv::Vec<RealT, N> result;
    for(int i = 0; i < N; ++i) {
      result[i] = vec[i];
    }
    return result;
  }

  //! Convert Ravl2 fixed-size Matrix to OpenCV dynamic cv::Mat.
  template <typename RealT, IndexSizeT N, IndexSizeT M>
  cv::Mat toCvMat(const Matrix<RealT, N, M> &mat)
  {
    constexpr int rows = static_cast<int>(N);
    constexpr int cols = static_cast<int>(M);
    cv::Mat result(rows, cols, cv::DataType<RealT>::type);
    for(int i = 0; i < rows; ++i) {
      for(int j = 0; j < cols; ++j) {
        result.at<RealT>(i, j) = mat(i, j);
      }
    }
    return result;
  }

  //! Convert Ravl2 fixed-size Matrix to OpenCV fixed-size cv::Matx.
  template <typename RealT, IndexSizeT N, IndexSizeT M>
  auto toCvMatx(const Matrix<RealT, N, M> &mat)
  {
    constexpr int rows = static_cast<int>(N);
    constexpr int cols = static_cast<int>(M);
    cv::Matx<RealT, rows, cols> result;
    for(int i = 0; i < rows; ++i) {
      for(int j = 0; j < cols; ++j) {
        result(i, j) = mat(i, j);
      }
    }
    return result;
  }

  //! Convert dynamic cv::Mat to Ravl2 fixed-size Matrix. Throws if dimensions don't match.
  template <typename RealT, IndexSizeT N, IndexSizeT M>
  Matrix<RealT, N, M> toMatrix(const cv::Mat &mat)
  {
    constexpr int rows = static_cast<int>(N);
    constexpr int cols = static_cast<int>(M);
    Matrix<RealT, N, M> result;
    if(mat.rows != rows || mat.cols != cols) {
      throw std::runtime_error("toMatrix: cv::Mat has wrong size");
    }
    for(int i = 0; i < rows; ++i) {
      for(int j = 0; j < cols; ++j) {
        result(i, j) = mat.at<RealT>(i, j);
      }
    }
    return result;
  }

  //! Convert fixed-size cv::Matx to Ravl2 fixed-size Matrix.
  template <typename RealT, IndexSizeT N, IndexSizeT M>
  Matrix<RealT, N, M> toMatrix(const cv::Matx<RealT, static_cast<int>(N), static_cast<int>(M)> &mat)
  {
    constexpr int rows = static_cast<int>(N);
    constexpr int cols = static_cast<int>(M);
    Matrix<RealT, N, M> result;
    for(int i = 0; i < rows; ++i) {
      for(int j = 0; j < cols; ++j) {
        result(i, j) = mat(i, j);
      }
    }
    return result;
  }

  //! Convert OpenCV Vec to Ravl2 Vector. No coordinate swap — elements are copied as-is.
  template <typename RealT, IndexSizeT N>
  Vector<RealT, N> toVector(const cv::Vec<RealT, static_cast<int>(N)> &vec)
  {
    constexpr int len = static_cast<int>(N);
    Vector<RealT, N> result;
    for(int i = 0; i < len; ++i) {
      result[i] = vec[i];
    }
    return result;
  }

  //! Convert to Affine<RealT,3>
  template <typename RealT>
  Affine<RealT, 3> toAffine(const cv::Affine3<RealT> &affine)
  {
    return Affine<RealT, 3>(toMatrix<RealT, 3, 3>(affine.rvec), toVector<RealT, 3>(affine.tvec));
  }

  //! Convert to cv::Affine3
  template <typename RealT>
  cv::Affine3<RealT> toCvAffine(const Affine<RealT, 3> &affine)
  {
    return cv::Affine3<RealT>(toCvMatx(affine.rotation()), toCvVec(affine.translation()));
  }

  //! Convert a cv::Rect to a Range. Swaps x/y → row/col via toPoint().
  template <typename RealT = float>
  inline constexpr auto toRange(const cv::Rect_<RealT> &rect)
  {
    return Range<RealT, 2>(toImagePoint(rect.tl()), toImagePoint(rect.br()));
  }

  //! Convert a Range to a cv::Rect. Swaps row/col → x/y via toCvPoint().
  template <typename RealT>
  inline constexpr auto toCvRect(const Range<RealT, 2> &range)
  {
    return cv::Rect_<RealT>(toCvImagePoint(range.min()), toCvImagePoint(range.max()));
  }

  //! Convert a cv::Size to a Vector. Swaps: cv::Size(width,height) → Vector(height,width) = (row,col).
  template <typename RealT>
  inline constexpr auto toVector(const cv::Size_<RealT> &size)
  {
    return Vector<RealT, 2>(size.height, size.width);
  }

  //! Convert a Vector to a cv::Size. Swaps: Vector(row,col) → cv::Size(width=col, height=row).
  template <typename RealT>
  inline constexpr auto toCvSize(const Vector<RealT, 2> &size)
  {
    return cv::Size_<RealT>(size[1], size[0]);
  }

  //! Convert a Ravl2 3D world point to an OpenCV Point3.
  //! No coordinate swap — both libraries use (x,y,z) for 3D world points.
  //! This differs from toCvImagePoint() which DOES swap for 2D image points.
  template <typename RealT>
  inline cv::Point3_<RealT> toCvWorldPoint3(const Point<RealT, 3> &point)
  {
    return cv::Point3_<RealT>(point[0], point[1], point[2]);
  }

  //! @deprecated Use toCvWorldPoint3() instead.
  template <typename RealT>
  [[deprecated("Use toCvWorldPoint3() instead — makes no-swap explicit")]]
  inline cv::Point3_<RealT> toCvPoint3(const Point<RealT, 3> &point)
  {
    return toCvWorldPoint3(point);
  }

  //! Convert OpenCV solvePnP output (rvec, tvec) to an Isometry3 in world coordinates.
  //! solvePnP returns the object-to-camera transform; this converts it to object-to-world
  //! by composing with the inverse of the camera extrinsic (world-to-camera) transform.
  //! @param rvec Rodrigues rotation vector from solvePnP
  //! @param tvec Translation vector from solvePnP
  //! @param cameraR Camera extrinsic rotation matrix (world → camera)
  //! @param cameraT Camera extrinsic translation (world → camera, in camera coords)
  //! @return Isometry3 taking points from object frame to world frame
  template <typename RealT>
  Isometry3<RealT> isometryFromPnP(const cv::Mat &rvec,
                                    const cv::Mat &tvec,
                                    const Matrix<RealT, 3, 3> &cameraR,
                                    const Vector<RealT, 3> &cameraT)
  {
    // Convert Rodrigues vector to rotation matrix
    cv::Mat rotMat;
    cv::Rodrigues(rvec, rotMat);

    // solvePnP always outputs CV_64F (double) rvec/tvec regardless of input type
    assert(tvec.type() == CV_64F && "isometryFromPnP expects double tvec (as produced by cv::solvePnP)");
    assert(rvec.type() == CV_64F && "isometryFromPnP expects double rvec (as produced by cv::solvePnP)");

    // solvePnP gives object-to-camera transform: p_cam = R_pnp * p_obj + t_pnp
    Matrix<RealT, 3, 3> objToCamR = toMatrix<RealT, 3, 3>(rotMat);
    Vector<RealT, 3> objToCamT;
    for(int i = 0; i < 3; i++) {
      objToCamT[i] = static_cast<RealT>(tvec.at<double>(i));
    }

    // Camera extrinsic: p_cam = cameraR * p_world + cameraT
    // Inverse: p_world = cameraR^T * (p_cam - cameraT)
    // Compose: p_world = cameraR^T * (R_pnp * p_obj + t_pnp - cameraT)
    //                   = (cameraR^T * R_pnp) * p_obj + cameraR^T * (t_pnp - cameraT)
    Matrix<RealT, 3, 3> camRinv = cameraR.transpose();
    Matrix<RealT, 3, 3> finalR = camRinv * objToCamR;
    Vector<RealT, 3> finalT = camRinv * (objToCamT - cameraT);

    return Isometry3<RealT>(Quaternion<RealT>::fromMatrix(finalR), finalT);
  }

  // Provide some common instantiations
#if 0
  extern template cv::Vec<float, 2> toCvVec(const Vector<float, 2> &vec);
  extern template cv::Vec<float, 3> toCvVec(const Vector<float, 3> &vec);
  extern template cv::Vec<double, 2> toCvVec(const Vector<double, 2> &vec);
  extern template cv::Vec<double, 3> toCvVec(const Vector<double, 3> &vec);

  extern template cv::Mat toCvMat(const Matrix<float, 3, 3> &mat);
  extern template cv::Mat toCvMat(const Matrix<double, 3, 3> &mat);
  extern template cv::Mat toCvMat(const Matrix<float, 4, 4> &mat);
  extern template cv::Mat toCvMat(const Matrix<double, 4, 4> &mat);
#endif

}// namespace Ravl2
