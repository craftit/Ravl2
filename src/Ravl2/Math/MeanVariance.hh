// This file is part of RAVL, Recognition And Vision Library
// Copyright (C) 2002, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here

#pragma once

#include <numbers>
#include <iostream>
#include <vector>
#include <cereal/cereal.hpp>
#include "Ravl2/Math.hh"

namespace Ravl2
{
  //! Enum indicating if we should use sample statistics.
  enum class SampleStatisticsT
  {
    SAMPLE,
    POPULATION
  };

  //! Mean and variance of a single variable.
  //! If you want to build up statistics about a sample use the Sums1d2C
  //! class to accumulate the information and create a MeanVariance<RealT> from
  //! there.

  template <typename RealT>
  class MeanVariance
  {
  public:
    //! Default constructor.
    constexpr MeanVariance() = default;

    //! Copy constructor.
    constexpr MeanVariance(const MeanVariance<RealT> &) = default;

    //! Construct from parts
    constexpr MeanVariance(size_t nn, RealT nmean, RealT nvar)
        : mN(nn),
          mMean(nmean),
          mVar(nvar)
    {}

    //! Construct from a single value.
    constexpr MeanVariance &operator=(const MeanVariance<RealT> &) = default;

    //! Get the standard deviation.
    [[nodiscard]] constexpr RealT stdDeviation() const
    {
      return (mVar < 0) ? 0 : std::sqrt(mVar);
    }

    //! Access the variance.
    [[nodiscard]] constexpr RealT variance() const
    {
      return (mVar < 0) ? 0 : mVar;
    }

    //! Access the variance, this isn't limited to values zero or above.
    [[nodiscard]] constexpr RealT rawVariance() const
    {
      return mVar;
    }

    //! Access the number of samples.
    [[nodiscard]] constexpr const size_t &count() const
    {
      return mN;
    }

    //! Access the number of samples.
    [[nodiscard]] constexpr size_t &count()
    {
      return mN;
    }

    //! Access the mean.
    [[nodiscard]] constexpr RealT mean() const
    {
      return mMean;
    }

    //! Value of the normal (Gaussian) distribution at x, using this mean and variance.
    [[nodiscard]] constexpr RealT Gauss(RealT x) const;

    //! Add another MeanVariance to this one.
    constexpr MeanVariance<RealT> &operator+=(const MeanVariance<RealT> &mv);

    //! Add another sample
    constexpr MeanVariance<RealT> &operator+=(const RealT &value);

    //! Remove another MeanVariance from this one.
    constexpr MeanVariance<RealT> &operator-=(const MeanVariance<RealT> &mv);

    //! Calculate the product of the two probability density functions.
    // (The number of samples is ignored)
    constexpr MeanVariance<RealT> operator*(const MeanVariance<RealT> &oth) const;

    //! Serialization support
    template <class Archive>
    constexpr void serialize(Archive &ar)
    {
      ar(cereal::make_nvp("n", mN),
         cereal::make_nvp("mean", mMean),
         cereal::make_nvp("variance", mVar));
    }

  private:
    size_t mN = 0;
    RealT mMean = 0;
    RealT mVar = 0;
  };

  //! Calculate the mean and variance from an array of numbers.
  //! @param data The data to compute the mean and variance of.
  //! @param sampleStatistics - When true compute statistics as a sample of a random variable. (Normalise covariance by n-1 )
  template <typename RealT>
  constexpr MeanVariance<RealT> computeMeanVariance(const std::vector<RealT> &data, SampleStatisticsT sampleStatistics)
  {
    auto n = data.size();
    RealT var = 0;
    RealT sum = 0;
    for(auto it : data) {
      sum += it;
      var += sqr(it);
    }
    const RealT rn = RealT(n);
    const RealT mean = sum / rn;
    RealT sn = rn;
    if(sampleStatistics == SampleStatisticsT::SAMPLE) sn--;
    var = (var - sqr(sum) / rn) / sn;
    return MeanVariance<RealT>(n, mean, var);
  }

  //: Add another MeanVariance to this one.

  template <typename RealT>
  constexpr MeanVariance<RealT> &MeanVariance<RealT>::operator+=(const MeanVariance<RealT> &mv)
  {
    if(mv.count() == 0) {
      return *this;
    }
    const RealT number1 = RealT(count());
    const RealT number2 = RealT(mv.count());
    const RealT nDen = number1 + number2;
    const RealT p1 = number1 / nDen;
    const RealT p2 = number2 / nDen;

    mVar *= p1;
    mVar += mv.variance() * p2;
    mVar += ((mv.mean() - mean()) * p1 * p2);

    // Update the mean.
    mMean = mMean * p1 + mv.mean() * p2;
    mN += mv.count();
    return *this;
  }

  //: Add another sample
  template <typename RealT>
  constexpr MeanVariance<RealT> &MeanVariance<RealT>::operator+=(const RealT &value)
  {
    mN += 1;
    RealT rn = RealT(mN);
    RealT delta = value - mMean;
    mMean += delta / rn;
    mVar = (mVar * (rn - 1) + (delta * (value - mMean))) / rn;
    return *this;
  }

  //: Remove another MeanVariance from this one.

  template <typename RealT>
  constexpr MeanVariance<RealT> &MeanVariance<RealT>::operator-=(const MeanVariance<RealT> &mv)
  {
    if(mv.count() == 0)
      return *this;
    const RealT number1 = RealT(count());
    const RealT number2 = RealT(mv.count());
    const RealT nDen = number1 - number2;
    const RealT p1 = nDen / number1;
    const RealT p2 = number2 / number1;

    // Update the mean.
    mMean = (mMean - mv.mean() * p2) / p1;

    // Update the variance.
    mVar -= ((mv.mean() - mean()) * p1 * p2);
    mVar -= mv.variance() * p2;
    mVar /= p1;

    mN -= mv.count();
    return *this;
  }

  //: Value of the gauss distribution at x.

  template <typename RealT>
  constexpr RealT MeanVariance<RealT>::Gauss(RealT x) const
  {
    RealT sig = std::sqrt(mVar);
    return std::exp(RealT(-0.5) * sqr((x - mMean) / sig)) / (sig * std::sqrt(2 * std::numbers::pi_v<RealT>));
  }

  //: Calculate the product of the two probability density functions.
  // (The number of samples is ignored)

  template <typename RealT>
  constexpr MeanVariance<RealT> MeanVariance<RealT>::operator*(const MeanVariance<RealT> &oth) const
  {
    RealT sum = variance() + oth.variance();
    RealT newMean = (variance() * oth.mean() / sum) + (oth.variance() * mean() / sum);
    RealT newVar = variance() * oth.variance() / sum;
    return MeanVariance(count() + oth.count(), newMean, newVar);
  }

  template <typename RealT>
  std::ostream &operator<<(std::ostream &s, const MeanVariance<RealT> &mv)
  {
    s << mv.count() << ' ' << mv.mean() << ' ' << mv.variance();
    return s;
  }

  template <typename RealT>
  std::istream &operator>>(std::istream &s, MeanVariance<RealT> &mv)
  {
    size_t n;
    RealT v1;
    RealT v2;
    s >> n >> v1 >> v2;
    mv = MeanVariance(n, v1, v2);
    return s;
  }

  extern template class MeanVariance<double>;
  extern template class MeanVariance<float>;

}// namespace Ravl2
