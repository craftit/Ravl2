

#pragma once

#include "Ravl2/Index.hh"

#include "Ravl2/Image/Segmentation/Boundary.hh"
#include "Ravl2/Geometry/Moments2.hh"

namespace Ravl2
{

  //! @brief Compute the moments of the region defined by the boundary.
  //! This is in boundary coordinates, which are shift by half a pixel.
  //! @param boundary - the boundary of the region.
  //! @return the moments of the region.
  //! The sums can get large, ideally the boundary should be shifted to the origin.
  template <typename SumT>
    requires std::is_signed_v<SumT>
  [[nodiscard]] Moments2<SumT> moments2(const Boundary &boundary)
  {
    Moments2<SumT> moments;
    for(const auto &edge : boundary.edges()) {
      switch(edge.crackCode()) {
        case CrackCodeT::CR_UP: {// 1,1 start
          SumT x = edge.at()[0] - 1;
          SumT y = edge.at()[1];
          moments.M00() += y;
          moments.M10() += x * y;
          moments.M20() += x * x * y;
          moments.M11() += x * y * (y - 1) / 2;

        } break;
        case CrackCodeT::CR_DOWN: {// 0,0 start
          SumT x = edge.at()[0];
          SumT y = edge.at()[1];
          moments.M00() -= y;
          moments.M10() -= x * y;
          moments.M20() -= x * x * y;
          moments.M11() -= x * y * (y - 1) / 2;
        } break;
        case CrackCodeT::CR_RIGHT: {// 1,0 start
          SumT x = edge.at()[0];
          SumT y = edge.at()[1];
          moments.M01() += x * y;
          moments.M02() += x * y * y;
        } break;
        case CrackCodeT::CR_LEFT: {// 0,1 start
          SumT x = edge.at()[0];
          SumT y = edge.at()[1] - 1;
          moments.M01() -= x * y;
          moments.M02() -= x * y * y;
        } break;
        case CrackCodeT::CR_NODIR: break;
      }
    }
    return moments;
  }

  //! Mean and covariance of the region defined by the boundary.
  //! The boundary vertexes are on the pixel centers, this is corrected by adding 0.5 to the centroid.
  //! @param boundary - the boundary of the region.
  //! @return the area, mean and covariance of the region.
  template <typename RealT>
  [[nodiscard]] std::tuple<RealT, Point<RealT, 2>, Matrix<RealT, 2, 2>> meanCovariance(const Boundary &boundary)
  {
    auto moments = Ravl2::moments2<int64_t>(boundary);
    auto intCentroid = moments.centroid();
    moments.shift(-intCentroid);

    Ravl2::Moments2<RealT> floatMoments(moments);
    [[maybe_unused]] auto covariance = floatMoments.covariance();

    Ravl2::Point<RealT, 2> floatCentroid = floatMoments.centroid() + Ravl2::toPoint<RealT>(intCentroid) + Ravl2::toPoint<RealT>(0.5, 0.5);// Convert from boundary to pixel coordinates.

    return {RealT(moments.area()), floatCentroid, covariance};
  }

  // Some common instantiations
  extern template Moments2<double> moments2<double>(const Boundary &boundary);
  extern template Moments2<int64_t> moments2<int64_t>(const Boundary &boundary);

}// namespace Ravl2