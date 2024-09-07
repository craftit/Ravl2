// This file is part of RAVL, Recognition And Vision Library
// Copyright (C) 2002, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here
//! author="Charles Galambos"
//! date="17/10/2002"
//! docentry="Ravl.API.Math.Geometry.2D;Ravl.API.Math.Projective Geometry.2D"

#pragma once

#include <spdlog/spdlog.h>
#include "Ravl2/Types.hh"
#include "Ravl2/Geometry/Affine.hh"
#include "Ravl2/Geometry/Geometry.hh"
#include "Ravl2/Math/LinearAlgebra.hh"

namespace Ravl2
{

  //! @brief Projective transform.
  //! <p>Represents a perspective projection in ND space. </p>
  //! <p>The difference between this class and <a href="RavlN.PProjection2dC.html">PProjection2dC</a> is:</p><ul>
  //! <li> This class (Projection2dC) includes the relevant projective scaling parameters within the projection object.  Use this class when dealing with Point<RealT,2> Euclidean 2D points.</li>
  //! <li> PProjection2dC does not include the projective scaling parameters: it is for use with projective 2D points PPoint<RealT,2> which themselves already contain the scaling parameters.</li></ul>

  template <typename RealT, unsigned N>
  class Projection
  {
  public:
    using value_type = RealT;
    constexpr static unsigned dimension = N;

    //! Default constructor.
    //! Creates an identity transform.
    constexpr Projection() = default;

    //! Construct a projective transform.
    //! @param: transform - the 2D projective transformation
    //! @param: Iz, Oz - the projective scale values for the input and output vectors
    //! <p>These are the scale values that the last term in the projective vectors must have for correct normalisation.  They are usually set = 1.  However for some optimisation operations better results are obtained if values more representative of typical components of the vector are used.
    //! In the projection "b = P a", Iz and Oz is the scale values for a and b respectively.</p>
    //! <p> This constructor assumes that the values of the last column of "transform" have already been set to correspond to the value of "iz".</p>
    constexpr explicit Projection(const Matrix<RealT, N+1, N+1> &transform, RealT Oz = 1, RealT Iz = 1)
      : trans(transform),
	iz(Iz),
	oz(Oz)
    {}

    //! Construct a projective transform from an affine one
    //! @param: affineTransform - the 2D affine transform
    //! @param: Iz, Oz - the projective scale values for the input and output vectors
    //! The parameters that are not specified by the affine transform are set to 0.
    constexpr explicit Projection(const Affine<RealT, N> &affineTransform, RealT Oz = 1, RealT Iz = 1)
        : iz(Iz),
          oz(Oz)
    {
      for(size_t i = 0; i < N; i++) {
        for(size_t j = 0; j < N; j++) {
          trans(i, j) = affineTransform.SRMatrix()(i, j);
        }
        trans(i, N) = affineTransform.Translation()[i] / iz;
        trans(i, N) = affineTransform.Translation()[i] / iz;
        trans(N, i) = 0;
      }
      trans(N, N) = oz / iz;
    }

    //! project a point through the transform.
    [[nodiscard]] constexpr Point<RealT, N> project(const Point<RealT, N> &pnt) const
    {
      Vector<RealT, N + 1> vi;
      for(size_t i = 0; i < N; i++) {
        vi[i] = pnt[i];
      }
      vi[N] = iz;
      auto vo = xt::linalg::dot(trans, vi);
      return oz * Vector<RealT, N>(vo) / vo[N];
    }

    //! project a point through the transform.
    [[nodiscard]] constexpr Point<RealT, N> operator()(const Point<RealT, N> &pnt) const
    {
      return project(pnt);
    }

#if 0
    LineABC2dC Project(const LineABC2dC &line) const {
      Vector<RealT,3> vo = trans.Inverse() * Vector<RealT,3>(line.A(),line.B(),line.C()/iz);
      return LineABC2dC(vo[0],vo[1],vo[2]*oz);          
    }
    //: Project a line through the transform.
    // Current implementation is slow, as it inverts the projection each time the method is called.
    
    LineABC2dC operator*(const LineABC2dC &line) const
    { return Project(line); }
    //: Project a line through the transform.
    // Current implementation is slow, as it inverts the projection each time the method is called.
#endif

    //! Combine two transforms
    //! @param: oth - the other transform to be combined with this one
    //! @return: the result of cascading this transform with the other one.<br>
    //! Note that the iz and oz values of the two transforms are combined
    //! for the resulting one.
    [[nodiscard]] Projection<RealT, N> operator*(const Projection<RealT, N> &oth) const
    {
      Matrix<RealT, N + 1, N + 1> diag = xt::eye<RealT>(N + 1);
      diag(N, N) = iz / oth.oz;
      using xt::linalg::dot;
      Matrix<RealT, N + 1, N + 1> transform = dot(dot(trans, diag), oth.trans);
      return Projection<RealT, N>(transform, oz, oth.iz);
    }

    //! Invert transform.
    [[nodiscard]] constexpr Projection inverse() const
    {
      return Projection(Ravl2::inverse(trans).value(), iz, oz);
    }

    //! Returns identity projection
    static constexpr Projection<RealT, N> identity(RealT oz = 1, RealT iz = 1)
    {
      Matrix<RealT, N + 1, N + 1> m = xt::eye<RealT>(N + 1);
      m(N, N) = oz / iz;
      return Projection(m, oz, iz);
    }

    //! Access transformation matrix.
    //! This is NOT the homography between images unless the scaling factors are both 1.
    [[nodiscard]] constexpr auto &matrix()
    {
      return trans;
    }

    //! Access transformation matrix.
    //! This is NOT the homography between images unless the scaling factors are both 1.
    [[nodiscard]] constexpr const auto &matrix() const
    {
      return trans;
    }

    //! Access iz.
    [[nodiscard]] constexpr RealT IZ() const
    {
      return iz;
    }

    //! Access oz.
    [[nodiscard]] constexpr RealT OZ() const
    {
      return oz;
    }

    //! Access iz.
    [[nodiscard]] constexpr RealT &IZ()
    {
      return iz;
    }

    //! Access oz.
    [[nodiscard]] constexpr RealT &OZ()
    {
      return oz;
    }

    //! Test if projection is near affine.
    [[nodiscard]] constexpr bool IsNearAffine(const RealT tolerance = 1e-6) const
    {
      return (std::abs(trans(N, N)) + std::abs(trans(N, 0))) * (iz / oz) < tolerance;
    }

    //! Get homography
    //! This returns the projection normalised to make the projective scales both = 1
    [[nodiscard]] constexpr Matrix<RealT, N + 1, N + 1> Homography() const
    {
      Matrix<RealT, N + 1, N + 1> mat1 = xt::eye<RealT>(3);
      mat1(N, N) = iz;
      Matrix<RealT, N + 1, N + 1> mat2 = xt::eye<RealT>(3);
      mat2(N, N) = oz;
      using xt::linalg::dot;
      Matrix<RealT, N + 1, N + 1> ret = dot(dot(Ravl2::inverse(mat2).value(), trans), mat1);
      return ret;
    }

    //! Get an affine approximation of this projective transform
    //! @return: the affine approximation
    [[nodiscard]] constexpr Affine<RealT, N> AffineApproximation() const
    {
      Matrix<RealT,3,3> htrans = Homography();
      RealT t1 = htrans(0,2) / htrans(2,2);
      RealT t2 = htrans(1,2) / htrans(2,2);
      RealT h1 = htrans(0,0) / htrans(2,2)  - t1 * htrans(2,0);
      RealT h2 = htrans(0,1) / htrans(2,2) - t1 * htrans(2,1);
      RealT h3 = htrans(1,0) / htrans(2,2) - t2 * htrans(2,0);
      RealT h4 = htrans(1,1) / htrans(2,2) - t2 * htrans(2,1);
      return Affine<RealT,N>(Matrix<RealT,2,2>({{h1,h2},{h3,h4}}), Vector<RealT,2>({t1,t2}));
    }

    //! True if not the zero projection and Matrix<RealT,3,3> is "real"
    [[nodiscard]] constexpr inline bool IsValid() const
    {
      return !isNearZero(xt::sum(xt::abs(trans))()) && isReal(trans);
    }

    //! Serialization support
    template <class Archive>
    void serialize(Archive &ar)
    {
      ar(cereal::make_nvp("transform", trans), cereal::make_nvp("iz", iz), cereal::make_nvp("oz", oz));
    }

  protected:
    Matrix<RealT, N + 1, N + 1> trans;// = xt::eye<RealT>();
    RealT iz = 1;
    RealT oz = 1;
  };

#if 0
  Projection2dC FitProjection(const std::vector<Point<RealT,2>> &org,const std::vector<Point<RealT,2>> &newPos,RealT &residual);
  //: Fit a projective transform given to the mapping between original and newPos.
  // Note: In the current version of the routine 'residual' isn't currently computed.
  
  Projection2dC FitProjection(const std::vector<Point<RealT,2>> &org,const std::vector<Point<RealT,2>> &newPos);
  //: Fit a projective transform given to the mapping between original and newPos.
  
  Projection2dC FitProjection(const std::vector<Point<RealT,2>> &org,const std::vector<Point<RealT,2>> &newPos,RealT &residual);
  //: Fit a projective transform given to the mapping between original and newPos.
  // Note: In the current version of the routine 'residual' isn't currently computed.

  Projection2dC FitProjection(const std::vector<PairC<Point<RealT,2>> > &matchPairs, RealT &residual);
  //: Fit a projective transform given an array of matched points.
  
  Projection2dC FitProjection(const std::vector<Point<RealT,2>> &org,const std::vector<Point<RealT,2>> &newPos);
  //: Fit a projective transform given to the mapping between original and newPos.
  
  Projection2dC FitProjection(const std::vector<Point<RealT,2>> &org,const std::vector<Point<RealT,2>> &newPos,const std::vector<RealT> &weight);
  //: Fit a projective transform given to the mapping between original and newPos with weighting for points.
  
  bool FitProjection(const std::vector<Point<RealT,2>> &from,const std::vector<Point<RealT,2>> &to,Matrix<RealT,3,3> &proj);
  //: Fit a projective matrix.
  
  bool FitProjection(const std::vector<Point<RealT,2>> &from,const std::vector<Point<RealT,2>> &to,const std::vector<RealT> &weight,Matrix<RealT,3,3> &proj);
  //: Fit a projective matrix with weighting for points.

#endif

  //! Read from a stream.
  template <typename RealT, unsigned N>
  std::istream &operator>>(std::istream &s, Projection<RealT, N> &proj)
  {
    s >> proj.Matrix() >> proj.IZ() >> proj.OZ();
    return s;
  }

  //! Write to a stream.
  template <typename RealT, unsigned N>
  std::ostream &operator<<(std::ostream &s, const Projection<RealT, N> &proj)
  {
    s << proj.matrix() << ' ' << proj.IZ() << ' ' << proj.OZ();
    return s;
  }

  extern template class Projection<float, 2>;

}// namespace Ravl2

namespace fmt
{
  template <typename DataT, unsigned N>
  struct formatter<Ravl2::Projection<DataT, N>> : ostream_formatter {
  };
}// namespace fmt

