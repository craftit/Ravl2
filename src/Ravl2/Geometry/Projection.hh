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
    using PointT = Point<RealT, N>;

    //! Default constructor.
    //! Creates an identity transform.
    constexpr Projection() = default;

    //! Construct a projective transform.
    //! @param: transform - the 2D projective transformation
    //! @param: Iz, Oz - the projective scale values for the input and output vectors
    //! <p>These are the scale values that the last term in the projective vectors must have for correct normalisation.  They are usually set = 1.
    //! However for some optimisation operations better results are obtained if values more representative of typical components of the vector are used.
    //! In the projection "b = P a", Iz and Oz is the scale values for a and b respectively.</p>
    //! <p> This constructor assumes that the values of the last column of "transform" have already been set to correspond to the value of "iz".</p>
    constexpr explicit Projection(const Matrix<RealT, N+1, N+1> &transform, RealT Oz = 1, RealT Iz = 1)
      : trans(transform),
	    iz(Iz),
	    oz(Oz)
    {}

    //! Create a null projection, all points are mapped to the origin.
    static constexpr auto null()
    { return Projection(Matrix<RealT, N+1, N+1>::Zero(), 1, 1); }

    //! Construct a projective transform from an affine one
    //! @param: affineTransform - the 2D affine transform
    //! @param: Iz, Oz - the projective scale values for the input and output vectors
    //! The parameters that are not specified by the affine transform are set to 0.
    constexpr explicit Projection(const Affine<RealT, N> &affineTransform, RealT Oz = 1, RealT Iz = 1)
        : trans(),
          iz(Iz),
          oz(Oz)
    {
      for(IndexT i = 0; i < IndexT(N); i++) {
        for(IndexT j = 0; j < IndexT(N); j++) {
          trans(i, j) = affineTransform.SRMatrix()(i, j);
        }
        trans(i, N) = affineTransform.translation()[i] / iz;
        trans(N, i) = 0;
      }
      trans(N, N) = oz / iz;
    }

    //! Construct from a scale /translation
    explicit constexpr Projection(const ScaleTranslate<RealT, N> &st, RealT Oz = 1, RealT Iz = 1)
     : trans(Matrix<RealT,N+1,N+1>::Zero()),
       iz(Iz),
       oz(Oz)
    {
      for(IndexT i = 0; i < IndexT(N); i++) {
        trans(i, i) = st.scaleVector()[i];
        trans(i, N) = st.translation()[i] / iz;
      }
      trans(N, N) = oz / iz;
    }

    //! Construct from a scale /translation
    explicit constexpr Projection(const Translate<RealT, N> &st, RealT Oz = 1, RealT Iz = 1)
        : trans(Matrix<RealT,N+1,N+1>::Zero()),
          iz(Iz),
          oz(Oz)
    {
      for(IndexT i = 0; i < IndexT(N); i++) {
        trans(i, i) = 1;
        trans(i, N) = st.translation()[i] / iz;
      }
      trans(N, N) = oz / iz;
    }

    //! Construct from a transform
    template<GeometryTransform TransformT>
    explicit constexpr Projection(const TransformT &transform)
      : trans(transform.projectiveMatrix()),
        iz(1),
        oz(1)
    {}

    //! Returns identity projection
    static constexpr Projection<RealT, N> identity(RealT oz = 1, RealT iz = 1)
    {
      Matrix<RealT, N + 1, N + 1> m = Matrix<RealT, N + 1, N + 1>::Identity();
      m(N, N) = oz / iz;
      return Projection(m, oz, iz);
    }

    //! Create a translation projection
    static constexpr Projection<RealT, N> translation(const Point<RealT,N> trans,RealT oz = 1, RealT iz = 1)
    {
      Matrix<RealT, N + 1, N + 1> m = Matrix<RealT, N + 1, N + 1>::Identity();
      for(IndexT i = 0; i < IndexT(N); i++) {
        m(i, N) = trans[i] / iz;
      }
      m(N, N) = oz / iz;
      return Projection(m, oz, iz);
    }

    //! project a point through the transform.
    [[nodiscard]] constexpr Point<RealT, N> project(const Point<RealT, N> &pnt) const
    {
      Vector<RealT, N + 1> vi;
      for(IndexT i = 0; i < IndexT(N); i++) {
        vi[i] = pnt[i];
      }
      vi[N] = iz;
      Vector<RealT, N+1> vo = trans * vi;
      return ((oz * vo.array()) / vo[N]).head(N);
    }

    //! project a point through the transform.
    [[nodiscard]] constexpr Point<RealT, N> operator()(const Point<RealT, N> &pnt) const
    {
      return project(pnt);
    }

    //! Combine two transforms
    //! @param: oth - the other transform to be combined with this one
    //! @return: the result of cascading this transform with the other one.<br>
    //! Note that the iz and oz values of the two transforms are combined
    //! for the resulting one.
    [[nodiscard]] constexpr Projection<RealT, N> operator()(const Projection<RealT, N> &oth) const
    {
      Matrix<RealT, N + 1, N + 1> diag = Matrix<RealT, N + 1, N + 1>::Identity();
      diag(N, N) = iz / oth.oz;
      Matrix<RealT, N + 1, N + 1> transform = trans * diag * oth.trans;
      return Projection<RealT, N>(transform, oz, oth.iz);
    }

    //! Invert transform.
    [[nodiscard]] constexpr Projection inverse() const
    {
      return Projection(Ravl2::inverse(trans).value(), iz, oz);
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
    [[nodiscard]] constexpr bool isNearAffine(const RealT tolerance = 1e-6) const
    {
      return (std::abs(trans(N, N)) + std::abs(trans(N, 0))) * (iz / oz) < tolerance;
    }

    //! Get homography
    //! This returns the projection normalised to make the projective scales both = 1
    [[nodiscard]] constexpr Matrix<RealT, N + 1, N + 1> homography() const
    {
      Matrix<RealT, N + 1, N + 1> ret = trans;
      ret.col(N) *= iz;
      ret.row(N) /= oz;
      return ret;
    }

    //! Get as a projective matrix
    [[nodiscard]] constexpr Matrix<RealT, N + 1, N + 1> projectiveMatrix() const
    { return homography(); }

    //! Get an affine approximation of this projective transform around the origin.
    //! @return: the affine approximation
    [[nodiscard]] constexpr Affine<RealT, N> affineApproximation() const
    {
      Matrix<RealT, N+1, N+1> h = homography();
      h /= h(N, N);
      return Affine<RealT, N>(h.template block<N, N>(0, 0),
                          h.template block<N, 1>(0, N));
    }

    //! True if not the zero projection and Matrix<RealT,3,3> is "real"
    [[nodiscard]] constexpr inline bool IsValid() const
    {
      return !isNearZero(trans.cwiseAbs().sum()) && trans.array().isFinite().all();
    }

    //! Are all the values in the transform real.
    //! Used to detect nan and inf values.
    [[nodiscard]] constexpr bool isReal() const
    {
      return Eigen::isfinite(trans.array()).all() && std::isfinite(iz) && std::isfinite(oz);
    }

    //! Serialization support
    template <class Archive>
    void serialize(Archive &ar)
    {
      ar(cereal::make_nvp("transform", trans), cereal::make_nvp("iz", iz), cereal::make_nvp("oz", oz));
    }

  private:
    Matrix<RealT, N + 1, N + 1> trans = Matrix<RealT, N + 1, N + 1>::Identity();
    RealT iz = 1;
    RealT oz = 1;
  };

  //! @brief Convert ScaleTranslate to projection.
  template <typename DataT, unsigned N>
  Projection<DataT, N> toProjection(Translate<DataT, N> const &translate)
  {
    return Projection<DataT, N>(translate);
  }

  //! @brief Convert ScaleTranslate to projection.
  template <typename DataT, unsigned N>
  Projection<DataT, N> toProjection(ScaleTranslate<DataT, N> const &st)
  {
    return Projection<DataT, N>(st);
  }
  
  //! @brief Convert ScaleTranslate to projection.
  template <typename DataT, unsigned N>
  Projection<DataT, N> toProjection(Affine<DataT, N> const &affine)
  {
    return Projection<DataT, N>(affine);
  }


  //! @brief Compose transforms
  template <typename DataT, unsigned N>
  Projection<DataT, N> operator*(const Projection<DataT, N> &lhs, const Projection<DataT, N> &rhs)
  {
    return lhs(rhs);
  }

  //! @brief Compose transforms
  template <typename DataT, unsigned N>
  Projection<DataT, N> operator*(const Projection<DataT, N> &lhs, const ScaleTranslate<DataT, N> &rhs)
  {
    return lhs(toProjection(rhs));
  }

  template <typename DataT, unsigned N>
  Projection<DataT, N> operator*(const Projection<DataT, N> &lhs, const Translate<DataT, N> &rhs)
  {
    return lhs(toProjection(rhs));
  }


  //! @brief Compose transforms
  template <typename DataT, unsigned N>
  Projection<DataT, N> operator*(const ScaleTranslate<DataT, N> &lhs, const Projection<DataT, N> &rhs)
  {
    return toProjection(lhs)(rhs);
  }

  //! @brief Compose transforms
  template <typename DataT, unsigned N>
  Projection<DataT, N> operator*(const Translate<DataT, N> &lhs, const Projection<DataT, N> &rhs)
  {
    return toProjection(lhs)(rhs);
  }

  template <typename DataT, unsigned N>
  Projection<DataT, N> operator*(const Projection<DataT, N> &lhs, const Affine<DataT, N> &rhs)
  {
    return lhs(toProjection(rhs));
  }
  
  //! @brief Compose transforms
  template <typename DataT, unsigned N>
  Projection<DataT, N> operator*(const Affine<DataT, N> &lhs, const Projection<DataT, N> &rhs)
  {
    return toProjection(lhs)(rhs);
  }
  
  
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
    s << "Projection(" << Eigen::WithFormat(proj.matrix(),defaultEigenFormat()) << " iz=" << proj.IZ() << " oz=" << proj.OZ() << ")";
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

