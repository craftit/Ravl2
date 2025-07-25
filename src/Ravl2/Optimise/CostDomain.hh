
#pragma once


#include "Ravl2/Types.hh"
#include "Ravl2/Configuration.hh"

namespace Ravl2
{
  //! Cost function domain, this provides information on the minimum and maximum values of the cost function

  template<typename RealT = float>
  class CostDomain
  {
  public:
    using VectorType = VectorT<RealT>;

    //! Construct a default domain
    CostDomain() = default;

    //! Construct from minimum and maximum values
    CostDomain(const VectorType &min,
               const VectorType &max)
      : mMin(min),
        mMax(max)
    {}

    CostDomain(const std::initializer_list<RealT> &min,
               const std::initializer_list<RealT> &max)
      : mMin(min.size()),
        mMax(max.size())
    {
      std::copy(min.begin(), min.end(), mMin.begin());
      std::copy(max.begin(), max.end(), mMax.begin());
    }

    //! Access the minimum value of for the arguments of the cost function
    [[nodiscard]] const VectorType &min () const
    { return mMin; }

    //! Access the maximum value of for the arguments of the cost function
    [[nodiscard]] const VectorType &max () const
    { return mMax; }

    //! Access the min value for a specific dimension
    [[nodiscard]] RealT min (Eigen::Index dim) const
    {
      if (dim >= mMin.size()) {
        throw std::out_of_range("Dimension out of range");
      }
      return mMin[dim];
    }

    //! Access the max value for a specific dimension
    [[nodiscard]] RealT max (Eigen::Index dim) const
    {
      if (dim >= mMax.size()) {
        throw std::out_of_range("Dimension out of range");
      }
      return mMax[dim];
    }

    //! Check if a value for a dimension is within the domain
    [[nodiscard]] bool isInDomain (Eigen::Index dim, RealT value) const
    {
      if (dim >= mMin.size() || dim >= mMax.size()) {
        SPDLOG_ERROR("Dimension {} out of range for domain with {} dimensions", dim, mMin.size());
        throw std::out_of_range("Dimension out of range");
      }
      return value >= mMin[dim] && value <= mMax[dim];
    }

    //! Access mid-point of the domain
    [[nodiscard]] VectorType mid () const
    { return (mMin + mMax) / RealT(2); }

    //! Get the number of dimensions
    [[nodiscard]] size_t size () const
    { return static_cast<size_t>(mMin.size()); }

    //! Dimensions of the cost function
    [[nodiscard]] Eigen::Index dim () const
    { return mMin.size(); }

    //! Test if domain is empty
    [[nodiscard]] bool empty () const
    { return mMin.size() == 0 || mMax.size() == 0; }
  private:
    VectorType mMin;
    VectorType mMax;
  };


}

