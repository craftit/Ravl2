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

#include "Ravl2/Types.hh"
#include "Ravl2/Geometry/Affine.hh"

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
    constexpr explicit Projection(const Matrix<RealT, N, N> &transform, RealT Oz = 1, RealT Iz = 1)
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
      trans[0][0] = affineTransform.SRMatrix()[0][0];
      trans[1][0] = affineTransform.SRMatrix()[1][0];
      trans[0][1] = affineTransform.SRMatrix()[0][1];
      trans[1][1] = affineTransform.SRMatrix()[1][1];
      trans[0][2] = affineTransform.Translation()[0] / iz;
      trans[1][2] = affineTransform.Translation()[1] / iz;
      trans[2][2] = oz / iz;
      trans[2][0] = trans[2][1] = 0;
    }

    //! project a point through the transform.
    constexpr Point<RealT, N> project(const Point<RealT, 2> &pnt) const
    {
      Vector<RealT, N + 1> vo = xt::linalg::dot(trans, Vector<RealT, 3>(pnt[0], pnt[1], iz));
      return oz * Vector<RealT, N>(vo) / vo[N];
    }

    //! project a point through the transform.
    constexpr Point<RealT, N> operator()(const Point<RealT, N> &pnt) const
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

    LinePP2dC Project(const LinePP2dC &line) const
    { return LinePP2dC(this->Project(line[0]),this->Project(line[1])); }
    //: Project a line through the transform.
    
    LinePP2dC operator*(const LinePP2dC &line) const
    { return LinePP2dC(this->Project(line[0]),this->Project(line[1])); }
    //: Project a line through the transform.

    Projection2dC operator*(const Projection2dC &oth) const {
      Matrix<RealT,3,3> diag(Matrix<RealT,3,3>::I());
      diag[2][2] = iz/oth.oz;
      Matrix<RealT,3,3> transform = trans * diag * oth.trans;
      return Projection2dC(transform, oz, oth.iz);
    }
    //: Combine two transforms
    //!param: oth - the other transform to be combined with this one
    //!return: the result of cascading this transform with the other one.<br>
    // Note that the iz and oz values of the two transforms are combined
    // for the resulting one.
#endif

    //! Invert transform.
    constexpr Projection Inverse() const
    {
      return Projection(inverse(trans), iz, oz);
    }

    //! Returns identity projection
    static constexpr Projection identity(RealT oz = 1, RealT iz = 1)
    {
      Matrix<RealT, 3, 3> m(Matrix<RealT, 3, 3>::I());
      m[2][2] = oz / iz;
      return Projection(m, oz, iz);
    }

    //! Access transformation matrix.
    //! This is NOT the homography between images unless the scaling factors are both 1.
    constexpr Matrix<RealT, 3, 3> &matrix()
    {
      return trans;
    }

    //! Access transformation matrix.
    //! This is NOT the homography between images unless the scaling factors are both 1.
    constexpr const Matrix<RealT, 3, 3> &matrix() const
    {
      return trans;
    }

    //! Accesss iz.
    constexpr RealT IZ() const
    {
      return iz;
    }

    //! Accesss oz.
    constexpr RealT OZ() const
    {
      return oz;
    }

    //! Accesss iz.
    constexpr RealT &IZ()
    {
      return iz;
    }

    //! Accesss oz.
    constexpr RealT &OZ()
    {
      return oz;
    }

    //! Test if projection is near affine.
    constexpr bool IsNearAffine(const RealT tolerance = 1e-6) const
    {
      return (std::abs(trans[2][2]) + std::abs(trans[2][0])) * (iz / oz) < tolerance;
    }

    //! Get homography
    //! This returns the projection normalised to make the projective scales both = 1
    constexpr Matrix<RealT, 3, 3> Homography() const;

    //! Get an affine approximation of this projective transform
    //! @return: the affine approximation
    constexpr Affine<RealT, 2> AffineApproximation() const;

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
  
  PointSet2dC operator*(const Projection2dC &trans,const PointSet2dC &points);
  //: Apply a projective transform to a point set
  
  Polygon2dC operator*(const Projection2dC &trans,const Polygon2dC &points);
  //: Apply a projective transform to a polygon
  
  std::istream &operator>>(std::istream &s,Projection2dC &proj);  
  //: Read from a stream.
  
  std::ostream &operator<<(std::ostream &s,const Projection2dC &proj);  
  //: Write to a stream.

#endif
}// namespace Ravl2
