// This file is part of RAVL, Recognition And Vision Library
// Copyright (C) 2001, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here
//! author="Charles Galambos"

#pragma once

#include <array>
#include "Ravl2/Geometry/Geometry.hh"
#include "Ravl2/IndexRange.hh"

namespace Ravl2
{

  template <typename RealT, unsigned N>
  class Range;

  //: 1D Range of real values.

  template <typename RealT>
  class Range<RealT, 1>
  {
  public:
    // Creates the index range <0, dim-1>.
    inline explicit constexpr Range(RealT size = 0)
        : mMin(0),
          mMax(size)
    {}

    //! Create real range from an IndexRange.
    // Note that the upper limit of the Range object is incremented by 1
    // to make the range consistent.
    explicit inline constexpr Range(const IndexRange<1> &rng)
        : mMin(RealT(rng.min())),
          mMax(RealT(rng.max() + 1))
    {}

    //! Creates the range <minReal, maxReal>.
    inline constexpr Range(RealT minReal, RealT maxReal)
        : mMin(minReal),
          mMax(maxReal)
    {}

    //! Creates the index range from the input stream.
    inline explicit Range(std::istream &s);

    //! Returns the size of the range.
    [[nodiscard]] inline constexpr RealT size() const
    {
      return (mMax - mMin);
    }

    //! Returns this object.
    [[nodiscard]] inline constexpr const Range<RealT, 1> &range() const
    {
      return *this;
    }

    //! Returns the minimum value of the range.
    [[nodiscard]] inline constexpr RealT min() const
    {
      return mMin;
    }

    //! Returns the maximum value of the range.
    [[nodiscard]] inline constexpr RealT max() const
    {
      return mMax;
    }

    //! Returns the minimum value of the range.
    inline constexpr RealT &min()
    {
      return mMin;
    }

    //! Returns the maximum value of the range.
    inline constexpr RealT &max()
    {
      return mMax;
    }

    //! Returns the index in the middle of the range, eg. (max()+min())/2.
    [[nodiscard]] inline constexpr RealT Center() const
    {
      return (min() + max()) / 2;
    }

    //! Returns the index previous the middle of the range, eg. (max()+min())/2.
    [[nodiscard]] inline constexpr RealT CenterD() const
    {
      return (min() + max()) / 2;
    }

    //! Returns the index which is in the 'p' % of the whole range.
    [[nodiscard]] inline constexpr RealT Percentage(RealT p) const
    {
      return (max() - min()) * p / RealT(100.0) + min();
    }

    //! Returns true if the minimum limit is bigger than the maximum limit.
    [[nodiscard]] inline constexpr bool empty() const
    {
      return min() > max();
    }

    //! @brief Create the range which creates the largest negative area.
    //! This is useful if you want guarantee the first point involved in
    //! the rectangle is always covered
    static constexpr auto mostEmpty()
    {
      return Range<RealT, 1>(std::numeric_limits<RealT>::max(), std::numeric_limits<RealT>::min());
    }

    //! Returns true if the minimum limit is smaller than or equal to the maximum value
    [[nodiscard]] inline constexpr bool IsValid() const
    {
      return min() <= max();
    }

    //! Returns true if this range contains the index 'i'.
    [[nodiscard]] inline constexpr bool contains(RealT i) const
    {
      return (min() <= i) && (i <= max());
    }

    //! Returns true if this range contains the subrange 'range'.
    [[nodiscard]] inline constexpr bool contains(const Range<RealT, 1> &range) const
    {
      return contains(range.min()) && contains(range.max());
    }

    //! Returns true if both index ranges have the same limits.
    [[nodiscard]] inline constexpr bool operator==(const Range &range) const
    {
      return (min() == range.min()) && (max() == range.max());
    }

    //! Returns true if both the ranges have different limits.
    [[nodiscard]] inline constexpr bool operator!=(const Range &range) const
    {
      return (min() != range.min()) || (max() != range.max());
    }

    //! Returns true if this range is inside of the 'range'.
    [[nodiscard]] bool In(const Range &range) const;

    //! @brief Returns true if this range contains at least one common index with the range 'r'.
    [[nodiscard]] inline constexpr bool overlaps(const Range<RealT, 1> &r) const
    {
      return (!empty() && !r.empty())
        && (min() <= r.max() && max() >= r.min());
    }

    //:-------------------
    //: Special operations.

    //! Set the origin of the range to 'position'.
    // Returns a reference to this range.
    inline constexpr const Range &SetOrigin(RealT position)
    {
      max() = position + max() - min();
      min() = position;
      return *this;
    }

    //! Move both the max and min of the range along 1.
    // Returns a reference to this range.
    inline constexpr Range &operator++()
    {
      min()++;
      max()++;
      return *this;
    }

    //! Move both the max and min of the range back 1.
    // Returns a reference to this range.
    inline constexpr Range &operator--()
    {
      min()--;
      max()--;
      return *this;
    }

    //! Both minimum and maximum limits are shifted by adding the offset 'i'.
    // Returns a reference to this range.
    inline constexpr const Range &operator+=(RealT i);

    //! Both minimum and maximum limits are shifted by subtracting the offset 'i'.
    // Returns a reference to this range.
    inline constexpr const Range &operator-=(RealT i);

    //! Create a new Range with minimum and maximum limits shifted by adding the offset 'i'.
    inline constexpr Range operator+(RealT i) const
    {
      return Range(min() + i, max() + i);
    }

    //! Create a new Range with minimum and maximum limits shifted by subtracting the offset 'i'.
    inline constexpr Range operator-(RealT i) const
    {
      return Range(min() - i, max() - i);
    }

    //! This index range is clipped in place to contain at most the index range 'r'.
    inline constexpr Range &clipBy(const Range &r);

    //! The value 'r' is clipped to be within this range.
    [[nodiscard]] inline RealT Clip(const RealT &r) const
    {
      RealT lower = min() > r ? min() : r;
      return lower < max() ? lower : max();
    }

    //! Returns the index range < min(), (max()+min())/2 >.
    [[nodiscard]] constexpr inline Range FirstHalf() const
    {
      return Range(min(), Center());
    }

    //! Returns the index range < min(), (max()+min())/2 >.
    [[nodiscard]] constexpr inline Range FirstHalfD() const
    {
      return Range(min(), CenterD());
    }

    //! Returns the index range whose number of elements is enlarged by
    //! the factor 'f'. The upper limits is changed.
    [[nodiscard]] constexpr Range Enlarge(RealT f) const
    {
      return Range(min(), min() + (size() * f) - 1);
    }

    //! Returns the range extended by adding 'n' items on both limits of
    //! this range.
    [[nodiscard]] constexpr Range expand(RealT n) const
    {
      return Range(mMin - n, mMax + n);
    }

    //! Returns the range extended by adding 'n' items on both limits of
    //! this range.
    [[nodiscard]] constexpr Range shrink(RealT n) const
    {
      return Range(mMin + n, mMax - n);
    }

    //! Returns the range shrunk by reducing the max of the range by 'n'.
    [[nodiscard]] constexpr Range shrinkMax(RealT n) const
    {
      return Range(mMin, mMax - n);
    }

    //! Scale range
    constexpr Range operator*(RealT scale) const
    {
      return Range(min() * scale, max() * scale);
    }

    //! Modify this range to ensure index i is contained within it.
    constexpr const Range &involve(RealT i)
    {
      if(mMin > i) mMin = i;
      if(mMax < i) mMax = i;
      return *this;
    }

    //! Modify this range to ensure subRange is contained within it.
    constexpr const Range &involve(const Range<RealT, 1> &subRange)
    {
      involve(subRange.min());
      involve(subRange.max());
      return *this;
    }

    //! Get the smallest integer range containing the real range.
    [[nodiscard]] constexpr IndexRange<1> toIndexRange() const
    {
      return IndexRange<1>(int_floor(mMin), int_ceil(mMax));
    }

  private:
    RealT mMin = 0;
    RealT mMax = 0;
  };

  //: Returns true if the index 'i' is inside the index range 'r'.
  template <typename RealT>
  inline bool IsInside(RealT i, const Range<RealT, 1> &range)
  {
    return (range.min() <= i) && (i <= range.max());
  }

  //: Multiply an index range by a real range.
  // Multiplying by a real range of 0-1 is a unit transform.
  template <typename RealT>
  IndexRange<2> operator*(const Range<RealT, 1> &realRange, const IndexRange<2> &indexRange);

  //: Read range from input stream.
  // Read information from the intput stream 's' and sets the real range
  // according obtained data.
  template <typename RealT>
  std::istream &operator>>(std::istream &s, Range<RealT, 1> &r);

  //: Saves the index range 'r' into the output stream 's'.
  template <typename RealT>
  std::ostream &operator<<(std::ostream &s, const Range<RealT, 1> &r);

  template <typename RealT>
  inline constexpr const Range<RealT, 1> &Range<RealT, 1>::operator+=(RealT i)
  {
    min() += i;
    max() += i;
    return *this;
  }

  template <typename RealT>
  inline constexpr const Range<RealT, 1> &Range<RealT, 1>::operator-=(RealT i)
  {
    min() -= i;
    max() -= i;
    return *this;
  }

  template <typename RealT>
  inline constexpr Range<RealT, 1> &Range<RealT, 1>::clipBy(const Range &r)
  {
    if(min() < r.min()) {
      min() = r.min();
      if(max() < r.min())// Make sure there is some overlap.
        max() = r.min(); // Make range zero size.
    }
    if(max() > r.max()) {
      max() = r.max();
      if(min() > r.max())// Make sure there is some overlap.
        min() = r.max(); // To make range zero size.
    }
    return *this;
  }

  //! @brief An index range of a 2D array

  template <typename RealT, unsigned N>
  class Range
  {
  public:
    //: Default constructor.
    constexpr Range() = default;

    //! @brief Construct from an IndexRange<N>.
    //! Note that the upper limits of the Range object are incremented by 1
    //! to make the range consistent.
    explicit constexpr Range(const IndexRange<N> &rng)
    {
      for(unsigned i = 0; i < N; ++i) {
        mRanges[i] = Range<RealT, 1>(rng[i]);
      }
    }

    //! Constructor.
    constexpr Range(std::initializer_list<Range<RealT, 1>> list)
    {
      assert(list.size() == N);
      std::copy(list.begin(), list.end(), mRanges.begin());
    }

    //! Create an 2d range from corner points.
    constexpr Range(const Vector<RealT, N> &org, const Vector<RealT, N> &end)
    {
      for(unsigned i = 0; i < N; ++i) {
        mRanges[i] = Range<RealT, 1>(org[i], end[i]);
      }
    }

    //! Create an 2d range from a center point and a size.
    // Size is the distance from the center to the edge, so
    // a size of 0 gives a single pixel, and a size of 1 generates
    // a 3x3 square.
    constexpr Range(const Point<RealT, N> &center, RealT size)
    {
      for(unsigned i = 0; i < N; ++i) {
        mRanges[i] = Range<RealT, 1>(center[i] - size / 2, center[i] + size / 2);
      }
    }

    //! Set the origin of the range to 'newOrigin'
    // Returns a reference to this rectangle.
    constexpr const Range &SetOrigin(const Vector<RealT, N> &newOrigin)
    {
      for(unsigned i = 0; i < N; ++i) {
        mRanges[i].SetOrigin(newOrigin[i]);
      }
      return *this;
    }

    //! Returns the top-left index of the rectangle.
    [[nodiscard]] inline constexpr Vector<RealT, N> origin() const
    {
      Vector<RealT, N> ret;
      for(unsigned i = 0; i < N; ++i) {
        ret[i] = mRanges[i].min();
      }
      return ret;
    }

    //! Returns the bottom-right index of the rectangle.
    [[nodiscard]] inline constexpr Vector<RealT, N> End() const
    {
      Vector<RealT, N> ret;
      for(unsigned i = 0; i < N; ++i) {
        ret[i] = mRanges[i].max();
      }
      return ret;
    }

    //! Returns the index which is in the middle of the rectangle
    [[nodiscard]] inline constexpr Vector<RealT, N> Center() const
    {
      Vector<RealT, N> ret;
      for(unsigned i = 0; i < N; ++i) {
        ret[i] = mRanges[i].Center();
      }
      return ret;
    }

    //! Returns the area of the image rectangle expressed in number of indexs.
    [[nodiscard]] inline constexpr RealT area() const
    {
      RealT area = 1;
      for(unsigned i = 0; i < N; ++i) {
        area *= mRanges[i].size();
      }
      return area;
    }

    //! Returns an rectangle expanded by 'n' on each side.
    [[nodiscard]] inline constexpr Range expand(RealT n) const
    {
      Range ret;
      for(unsigned i = 0; i < N; ++i) {
        ret[i] = mRanges[i].expand(n);
      }
      return ret;
    }

    //! @brief Test if the range is empty.
    [[nodiscard]] constexpr inline bool empty() const
    {
      for(unsigned i = 0; i < N; ++i) {
        if(mRanges[i].empty()) {
          return true;
        }
      }
      return false;
    }

    //! @brief Create the range which creates the largest negative area.
    //! This is useful if you want guarantee the first point involved in
    //! the rectangle is always covered
    static constexpr auto mostEmpty()
    {
      Range ret;
      for(unsigned i = 0; i < N; ++i) {
        ret[i] = Range<RealT, 1>::mostEmpty();
      }
      return ret;
    }

    //! @brief Returns a new rectangle which is n smaller on all sides.
    [[nodiscard]] inline constexpr Range shrink(RealT n) const
    {
      Range ret;
      for(unsigned i = 0; i < N; ++i) {
        ret[i] = mRanges[i].shrink(n);
      }
      return ret;
    }

    //! Returns the range shrunk by removing of the
    //! last 'n' items on both limits of this range in each dimension.
    [[nodiscard]] inline constexpr Range shrinkMax(RealT n) const
    {
      Range ret;
      for(unsigned i = 0; i < N; ++i) {
        ret[i] = mRanges[i].shrinkMax(n);
      }
      return ret;
    }

    //! @brief Clip this range by 'r'.
    //! This index range is clipped to contain at most the index range 'r'.
    inline constexpr Range &clipBy(const Range &r)
    {
      for(unsigned i = 0; i < N; ++i) {
        mRanges[i].clipBy(r.range(i));
      }
      return *this;
    }

    //! The value 'r' is clipped to be within this range.
    inline constexpr Vector<RealT, N> Clip(const Vector<RealT, N> &r)
    {
      Vector<RealT, N> result;
      for(unsigned i = 0; i < N; ++i) {
        result[i] = mRanges[i].Clip(r[i]);
      }
      return result;
    }

    //! Returns true if this range contains the subrange 'oth'.
    [[nodiscard]] constexpr bool contains(const Range<RealT, N> &oth) const
    {
      for(unsigned i = 0; i < N; ++i) {
        if(!mRanges[i].contains(oth.range(i))) {
          return false;
        }
      }
      return true;
    }

    //! Returns true if this range contains the point 'oth'.
    [[nodiscard]] constexpr bool contains(const Vector<RealT, N> &oth) const
    {
      for(unsigned i = 0; i < N; ++i) {
        if(!mRanges[i].contains(oth[i])) {
          return false;
        }
      }
      return true;
    }

    //! Shifts the rectangle to the new position.
    inline constexpr const Range &operator+=(const Vector<RealT, N> &offset);

    //! Shifts the rectangle to the new position.
    inline constexpr const Range &operator-=(const Vector<RealT, N> &offset);

    //! Shifts the rectangle to the new position.
    inline constexpr Range<RealT, N> operator+(const Vector<RealT, N> &offset) const
    {
      Range<RealT, N> ret;
      for(unsigned i = 0; i < N; ++i) {
        ret[i] = mRanges[i] + offset[i];
      }
      return ret;
    }

    //! Shifts the rectangle to the new position.
    inline constexpr Range<RealT, N> operator-(const Vector<RealT, N> &offset) const
    {
      Range<RealT, N> ret;
      for(unsigned i = 0; i < N; ++i) {
        ret[i] = mRanges[i] - offset[i];
      }
      return ret;
    }

    //! Access range in given dimension.
    [[nodiscard]] inline constexpr const Range<RealT, 1> &range(unsigned d) const
    {
      return mRanges[d];
    }

    //! Access range in given dimension.
    [[nodiscard]] inline constexpr Range<RealT, 1> &range(unsigned d)
    {
      return mRanges[d];
    }

    //! Ensures this rectangle contains given index.
    //! This method checks and changes, if necessary, the 2-dimensional range
    //! to contain the 'index'.
    inline constexpr void involve(const Vector<RealT, N> &index);

    //! Ensures this rectangle contains given sub rectangle.
    //! This method checks and changes, if necessary, the 2-dimensional range
    //! to contain the 'subrectangle'.
    inline constexpr void involve(const Range<RealT, N> &subrectangle)
    {
      for(unsigned i = 0; i < N; i++)
        mRanges[i].involve(subrectangle.range(i));
    }

    //! Returns true if this rectangle contains at least one index.
    [[nodiscard]] inline constexpr bool IsValid() const
    {
      for(unsigned i = 0; i < N; i++)
        if(!mRanges[i].IsValid())
          return false;
      return true;
    }

    //! Are two ranges equal ?
    constexpr bool operator==(const Range<RealT, N> &oth) const
    {
      for(unsigned i = 0; i < N; i++) {
        if(oth.range(i) != mRanges[i])
          return false;
      }
      return true;
    }

    //! Are two ranges unequal ?
    constexpr bool operator!=(const Range<RealT, N> &oth) const
    {
      for(unsigned i = 0; i < N; i++) {
        if(oth.range(i) != mRanges[i])
          return true;
      }
      return false;
    }

    //! Returns true if this range contains a common area with
    //! the range 'r'.
    [[nodiscard]] inline constexpr bool overlaps(const Range<RealT, N> &r) const
    {
      for(unsigned i = 0; i < N; i++) {
        if(!mRanges[i].overlaps(r.range(i))) {
          return false;
        }
      }
      return true;
    }

    //! Get the smallest integer range containing the real range.
    [[nodiscard]] constexpr IndexRange<N> toIndexRange() const
    {
      Ravl2::IndexRange<N> ret;
      for(unsigned i = 0; i < N; i++) {
        ret[i] = mRanges[i].toIndexRange();
      }
      return ret;
    }

    //! scale range
    [[nodiscard]] constexpr Range<RealT, N> operator*(const Vector<RealT, N> &scale) const
    {
      Range<RealT, N> ret;
      for(unsigned i = 0; i < N; i++) {
        ret[i] = mRanges[i] * scale[i];
      }
      return ret;
    }

    //! scale range
    [[nodiscard]] constexpr Range<RealT, N> operator*(RealT scale) const
    {
      Range<RealT, N> ret;
      for(unsigned i = 0; i < N; i++) {
        ret[i] = mRanges[i] * scale;
      }
      return ret;
    }

    //! Access item.
    constexpr Range<RealT, 1> &operator[](unsigned ind)
    {
      return mRanges[ind];
    }

    //! Access item.
    constexpr const Range<RealT, 1> &operator[](unsigned ind) const
    {
      return mRanges[ind];
    }

    //! Minimum value of the range.
    [[nodiscard]] constexpr Vector<RealT, N> min() const
    {
      Vector<RealT, N> ret;
      for(unsigned i = 0; i < N; i++) {
        ret[i] = mRanges[i].min();
      }
      return ret;
    }

    //! Maximum value of the range.
    [[nodiscard]] constexpr Vector<RealT, N> max() const
    {
      Vector<RealT, N> ret;
      for(unsigned i = 0; i < N; i++) {
        ret[i] = mRanges[i].max();
      }
      return ret;
    }

    //! Minimum value of the range in the given dimension.
    [[nodiscard]] constexpr RealT min(unsigned n) const
    {
      return mRanges[n].min();
    }

    //! Maximum value of the range in the given dimension.
    [[nodiscard]] constexpr RealT max(unsigned n) const
    {
      return mRanges[n].max();
    }

  private:
    std::array<Range<RealT, 1>, N> mRanges;
  };

  template <typename RealT, unsigned N>
  constexpr IndexRange<N> operator*(const Range<RealT, N> &realRange, const IndexRange<N> &indexRange);
  //: Multiply a 2d index range by a real 2d range.
  // Multiplying by a real range of 0-1,0-1 is a unit transform.

  template <typename RealT, unsigned N>
  std::ostream &operator<<(std::ostream &s, const Range<RealT, N> &ir);

  template <typename RealT, unsigned N>
  std::istream &operator>>(std::istream &s, Range<RealT, N> &ir);

  ///////////////////////////////////////////////////////

  template <typename RealT, unsigned N>
  inline constexpr const Range<RealT, N> &Range<RealT, N>::operator+=(const Vector<RealT, N> &offset)
  {
    for(unsigned i = 0; i < N; i++)
      mRanges[i] += offset[i];
    return *this;
  }

  template <typename RealT, unsigned N>
  inline constexpr const Range<RealT, N> &Range<RealT, N>::operator-=(const Vector<RealT, N> &offset)
  {
    for(unsigned i = 0; i < N; i++)
      mRanges[i] += offset[i];
    return *this;
  }

  template <typename RealT, unsigned N>
  inline constexpr void Range<RealT, N>::involve(const Vector<RealT, N> &index)
  {
    for(unsigned i = 0; i < N; i++)
      mRanges[i].involve(index[i]);
  }

  template <typename RealT, unsigned N>
  inline constexpr Range<RealT, N> toRange(const IndexRange<N> &ir)
  {
    Range<RealT, N> ret;
    for(unsigned i = 0; i < N; i++)
      ret[i] = toRange<RealT>(ir[i]);
    return ret;
  }

  //! Clamp a value to a range.
  template <typename RealT>
  constexpr RealT clamp(RealT val, const Range<RealT, 1> &rng)
  {
    return std::clamp(val, rng.min(), rng.max());
  }

  //! Clamp a point to be within a range.
  template <typename RealT, size_t N>
  constexpr auto clamp(const Point<RealT, N> &pnt, const Range<RealT, unsigned(N)> &rng)
  {
    Point<RealT, N> ret;
    for(size_t i = 0; i < N; i++)
      ret[i] = clamp(pnt[i], rng[i]);
    return ret;
  }

  template <typename RealT>
  inline constexpr Range<RealT, 1> toRange(IndexRange<1> ir)
  {
    return Range<RealT, 1>(RealT(ir.min()), RealT(ir.max() + 1));
  }

  //! Convert to smallest integer range containing the real range.
  template <typename RealT, unsigned N>
  inline constexpr IndexRange<N> toIndexRange(const Range<RealT, N> &ir)
  {
    IndexRange<N> ret;
    for(unsigned i = 0; i < N; i++)
      ret[i] = toIndexRange(ir[i]);
    return ret;
  }

  //! Convert to smallest integer range containing the real range.
  template <typename RealT>
  inline constexpr IndexRange<1> toIndexRange(Range<RealT, 1> ir)
  {
    return IndexRange<1>(int_floor(ir.min()), int_ceil(ir.max()));
  }

  //! Serialization support
  template <class Archive, typename RealT>
  constexpr void serialize(Archive &archive, Range<RealT, 1> &range)
  {
    cereal::size_type s = 2;
    archive(cereal::make_size_tag(s));
    if(s != 2) {
      throw std::runtime_error("range has incorrect length");
    }
    archive(range.min(), range.max());
  }

  //! Serialization support
  template <class Archive, typename RealT, unsigned N>
    requires(N > 1)
  constexpr void serialize(Archive &archive, Range<RealT, N> &range)
  {
    cereal::size_type s = N;
    archive(cereal::make_size_tag(s));
    if(s != N) {
      throw std::runtime_error("range has incorrect length");
    }
    for(auto &r : range.ranges()) {
      archive(r);
    }
  }

  extern template class Range<float, 1>;
  extern template class Range<float, 2>;

}// namespace Ravl2
