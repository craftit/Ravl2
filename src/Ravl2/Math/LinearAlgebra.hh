
#pragma once

#include <optional>
#include "Ravl2/Types.hh"

namespace Ravl2
{

  //: Returns the value of the determinant of this matrix.
  template <typename RealT>
  [[nodiscard]] inline RealT determinant(const Matrix<RealT,2,2> &data)
  {
    return data[0][0]*data[1][1] - data[0][1]*data[1][0];
  }

  //: Compute the inverse of a 2x2 matrix

  template <typename RealT>
  [[nodiscard]] inline std::optional<Matrix<RealT,2,2> > inverse(const Matrix<RealT,2,2> &data)
  {
    RealT det = determinant(data);
    if(isNearZero(det))
      return std::nullopt;
    Matrix<RealT,2,2> op;
    op[0][0] =  data[1][1]/det;
    op[0][1] = -data[0][1]/det;
    op[1][0] = -data[1][0]/det;
    op[1][1] =  data[0][0]/det;
    return op;
  }

  //: Returns the value of the determinant of this matrix.
  template <typename RealT>
  [[nodiscard]] inline RealT determinant(const Matrix<RealT,3,3> &data)
  {
    RealT d1 = data[1][1]*data[2][2]-data[1][2]*data[2][1];
    RealT d2 = data[1][2]*data[2][0]-data[1][0]*data[2][2];
    RealT d3 = data[1][0]*data[2][1]-data[1][1]*data[2][0];
    return data[0][0]*d1 + data[0][1]*d2 + data[0][2]*d3;
  }

  template <typename RealT>
  [[nodiscard]] std::optional<Matrix<RealT,3,3> > inverse(const Matrix<RealT,3,3> &data)
  {
    RealT d1 = data[1][1]*data[2][2]-data[1][2]*data[2][1];
    RealT d2 = data[1][2]*data[2][0]-data[1][0]*data[2][2];
    RealT d3 = data[1][0]*data[2][1]-data[1][1]*data[2][0];
    RealT det = data[0][0]*d1 + data[0][1]*d2 + data[0][2]*d3;

    if(isNearZero(det)) {
      return std::nullopt;// Matrix singular.
    }

    Matrix<RealT,3,3> op;
    op[0][0] = d1/det;
    op[0][1] = (data[2][1]*data[0][2]-data[2][2]*data[0][1])/det;
    op[0][2] = (data[0][1]*data[1][2]-data[0][2]*data[1][1])/det;
    op[1][0] = d2/det;
    op[1][1] = (data[2][2]*data[0][0]-data[2][0]*data[0][2])/det;
    op[1][2] = (data[0][2]*data[1][0]-data[0][0]*data[1][2])/det;
    op[2][0] = d3/det;
    op[2][1] = (data[2][0]*data[0][1]-data[2][1]*data[0][0])/det;
    op[2][2] = (data[0][0]*data[1][1]-data[0][1]*data[1][0])/det;
    return op;
  }

}