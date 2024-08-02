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
#include "Ravl2/Index.hh"

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
    [[nodiscard]] inline const Range<RealT, 1> &range() const
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

    [[nodiscard]] inline constexpr RealT Center() const
    {
      return (min() + max()) / 2;
    }
    //: Returns the index in the middle of the range, eg. (max()+min())/2.

    [[nodiscard]] inline constexpr RealT CenterD() const
    {
      return (min() + max()) / 2;
    }
    //: Returns the index previous the middle of the range, eg. (max()+min())/2.

    [[nodiscard]] inline constexpr RealT Percentage(RealT p) const
    {
      return (max() - min()) * p / RealT(100.0) + min();
    }
    //: Returns the index which is in the 'p' % of the whole range.

    //:-------------------
    //: Logical operations.

    [[nodiscard]] inline constexpr bool empty() const
    {
      return min() > max();
    }
    //: Returns true if the minimum limit is bigger than the maximum limit.

    [[nodiscard]] inline constexpr bool IsValid() const
    {
      return min() <= max();
    }
    //: Returns true if the minimum limit is smaller than or equal to the maximum value

    [[nodiscard]] inline constexpr bool contains(RealT i) const
    {
      return (min() <= i) && (i <= max());
    }
    //: Returns true if this range contains the index 'i'.

    [[nodiscard]] inline constexpr bool contains(const Range<RealT, 1> &range) const
    {
      return contains(range.min()) && contains(range.max());
    }
    //: Returns true if this range contains the subrange 'range'.

    [[nodiscard]] inline constexpr bool operator==(const Range &range) const
    {
      return (min() == range.min()) && (max() == range.max());
    }
    //: Returns true if both index ranges have the same limits.

    [[nodiscard]] inline constexpr bool operator!=(const Range &range) const
    {
      return (min() != range.min()) || (max() != range.max());
    }
    //: Returns true if both the ranges have different limits.

    [[nodiscard]] bool In(const Range &range) const;
    //: Returns true if this range is inside of the 'range'.

    [[nodiscard]] inline bool overlaps(const Range<RealT, 1> &r) const;
    //: Returns true if this range contains at least one common index with
    //: the range 'r'.

    //:-------------------
    //: Special operations.

    inline const Range &SetOrigin(RealT position)
    {
      max() = position + max() - min();
      min() = position;
      return *this;
    }
    //: Set the origin of the range to 'position'.
    // Returns a reference to this range.

    inline Range &operator++()
    {
      min()++;
      max()++;
      return *this;
    }
    //: Move both the max and min of the range along 1.
    // Returns a reference to this range.

    inline Range &operator--()
    {
      min()--;
      max()--;
      return *this;
    }
    //: Move both the max and min of the range back 1.
    // Returns a reference to this range.

    inline const Range &operator+=(RealT i);
    //: Both minimum and maximum limits are shifted by adding the offset 'i'.
    // Returns a reference to this range.

    inline const Range &operator-=(RealT i);
    //: Both minimum and maximum limits are shifted by subtracting the offset 'i'.
    // Returns a reference to this range.

    inline Range operator+(RealT i) const
    {
      return Range(min() + i, max() + i);
    }
    //: Create a new Range with minimum and maximum limits shifted by adding the offset 'i'.

    inline Range operator-(RealT i) const
    {
      return Range(min() - i, max() - i);
    }
    //: Create a new Range with minimum and maximum limits shifted by subtracting the offset 'i'.

    //! This index range is clipped in place to contain at most the index range 'r'.
    inline Range &clipBy(const Range &r);

    [[nodiscard]] inline RealT Clip(const RealT &r) const
    {
      RealT lower = min() > r ? min() : r;
      return lower < max() ? lower : max();
    }
    //: The value 'r' is clipped to be within this range.

    [[nodiscard]] inline Range FirstHalf() const
    {
      return Range(min(), Center());
    }
    //: Returns the index range < min(), (max()+min())/2 >.

    [[nodiscard]] inline Range FirstHalfD() const
    {
      return Range(min(), CenterD());
    }
    //: Returns the index range < min(), (max()+min())/2 >.

    [[nodiscard]] inline Range Enlarge(RealT f) const
    {
      return Range(min(), min() + size() * f - 1);
    }
    //: Returns the index range whose number of elements is enlarged by
    //: the factor 'f'. The upper limits is changed.

    [[nodiscard]] inline Range expand(RealT n) const
    {
      return Range(mMin - n, mMax + n);
    }
    //: Returns the range extended by adding 'n' items on both limits of
    //: this range.

    [[nodiscard]] inline Range shrink(RealT n) const
    {
      return Range(mMin + n, mMax - n);
    }
    //: Returns the range extended by adding 'n' items on both limits of
    //: this range.

    inline Range &ShrinkHigh(RealT n)
    {
      max() -= n;
      return *this;
    }
    //: Returns the range shrinked by removing of the
    //: last 'n' items on both limits of this range.

    Range operator*(RealT scale) const
    {
      return Range(min() * scale, max() * scale);
    }
    //: Scale range

    const Range &involve(RealT i)
    {
      if(mMin > i) mMin = i;
      if(mMax < i) mMax = i;
      return *this;
    }
    //: Modify this range to ensure index i is contained within it.

    const Range &involve(const Range<RealT, 1> &subRange)
    {
      involve(subRange.min());
      involve(subRange.max());
      return *this;
    }
    //: Modify this range to ensure subRange is contained within it.

    [[nodiscard]] IndexRange<1> toIndexRange() const
    {
      return IndexRange<1>(int_floor(mMin), int_ceil(mMax));
    }
    //: Get the smallest integer range containing the real range.

  private:
    RealT mMin = 0;// Minimum index.
    RealT mMax = 0;// Maximum index.

    //friend std::istream & operator>>(std::istream & s, Range & range);
  };

  template <typename RealT>
  inline bool IsInside(RealT i, const Range<RealT, 1> &range)
  {
    return (range.min() <= i) && (i <= range.max());
  }
  //: Returns true if the index 'i' is inside the index range 'r'.

  template <typename RealT>
  IndexRange<2> operator*(const Range<RealT, 1> &realRange, const IndexRange<2> &indexRange);
  //: Multiply an index range by a real range.
  // Multiplying by a real range of 0-1 is a unit transform.

  template <typename RealT>
  std::istream &operator>>(std::istream &s, Range<RealT, 1> &r);
  //: Read range from input stream.
  // Read information from the intput stream 's' and sets the real range
  // according obtained data.

  template <typename RealT>
  std::ostream &operator<<(std::ostream &s, const Range<RealT, 1> &r);
  //: Saves the index range 'r' into the output stream 's'.

  template <typename RealT>
  inline bool Range<RealT, 1>::overlaps(const Range<RealT, 1> &r) const
  {
    return (!empty() && !r.empty())
      && (min() <= r.max() && max() >= r.min());
  }

  template <typename RealT>
  inline const Range<RealT, 1> &Range<RealT, 1>::operator+=(RealT i)
  {
    min() += i;
    max() += i;
    return *this;
  }

  template <typename RealT>
  inline const Range<RealT, 1> &Range<RealT, 1>::operator-=(RealT i)
  {
    min() -= i;
    max() -= i;
    return *this;
  }

  template <typename RealT>
  inline Range<RealT, 1> &Range<RealT, 1>::clipBy(const Range &r)
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

  //-----------------------------------------------------------------------------
  //: An index range of a 2D array

  template <typename RealT, unsigned N>
  class Range
  {
  public:
    //: Default constructor.
    Range() = default;

    //! Construct from an IndexRange<N>.
    // Note that the upper limits of the Range object are incremented by 1
    // to make the range consistent.
    explicit Range(const IndexRange<N> &rng)
    {
      for(unsigned i = 0; i < N; ++i) {
        mRanges[i] = Range<RealT, 1>(rng[i]);
      }
    }

    //! Constructor.
    Range(std::initializer_list<Range<RealT, 1>> list)
    {
      assert(list.size() == N);
      std::copy(list.begin(), list.end(), mRanges.begin());
    }

    //! Create an 2d range from corner points.
    Range(const Vector<RealT, N> &org, const Vector<RealT, N> &end)
    {
      for(unsigned i = 0; i < N; ++i) {
        mRanges[i] = Range<RealT, 1>(org[i], end[i]);
      }
    }

    //! Create an 2d range from a center point and a size.
    // Size is the distance from the center to the edge, so
    // a size of 0 gives a single pixel, and a size of 1 generates
    // a 3x3 square.
    Range(const Point<RealT, N> &center, RealT size)
    {
      for(unsigned i = 0; i < N; ++i) {
        mRanges[i] = Range<RealT, 1>(center[i] - size / 2, center[i] + size / 2);
      }
    }

    //! Set the origin of the range to 'newOrigin'
    // Returns a reference to this rectangle.
    const Range &SetOrigin(const Vector<RealT, N> &newOrigin)
    {
      for(unsigned i = 0; i < N; ++i) {
        mRanges[i].SetOrigin(newOrigin[i]);
      }
      return *this;
    }

    //! Returns the top-left index of the rectangle.
    [[nodiscard]] inline Vector<RealT, N> origin() const
    {
      Vector<RealT, N> ret;
      for(unsigned i = 0; i < N; ++i) {
        ret[i] = mRanges[i].min();
      }
      return ret;
    }

    //: Returns the bottom-right index of the rectangle.
    [[nodiscard]] inline Vector<RealT, N> End() const
    {
      Vector<RealT, N> ret;
      for(unsigned i = 0; i < N; ++i) {
        ret[i] = mRanges[i].max();
      }
      return ret;
    }

    //! Returns the index which is in the middle of the rectangle
    [[nodiscard]] inline Vector<RealT, N> Center() const
    {
      Vector<RealT, N> ret;
      for(unsigned i = 0; i < N; ++i) {
        ret[i] = mRanges[i].Center();
      }
      return ret;
    }

    //! Returns the area of the image rectangle expressed in number of indexs.
    [[nodiscard]] inline RealT area() const
    {
      RealT area = 1;
      for(unsigned i = 0; i < N; ++i) {
        area *= mRanges[i].size();
      }
      return area;
    }

    //! Returns an rectangle expanded by 'n' on each side.
    [[nodiscard]] inline Range expand(RealT n) const
    {
      Range ret;
      for(unsigned i = 0; i < N; ++i) {
        ret[i] = mRanges[i].expand(n);
      }
      return ret;
    }

    //! Returns a new rectangle which has layer of the width of 'n'
    //! removed.
    [[nodiscard]] inline Range shrink(RealT n) const
    {
      Range ret;
      for(unsigned i = 0; i < N; ++i) {
        ret[i] = mRanges[i].shrink(n);
      }
      return ret;
    }

    inline Range &clipBy(const Range &r)
    {
      for(unsigned i = 0; i < N; ++i) {
        mRanges[i].clipBy(r.range(i));
      }
      return *this;
    }
    //: This index range is clipped to contain at most the index range 'r'.

    inline Vector<RealT, N> Clip(const Vector<RealT, N> &r)
    {
      Vector<RealT, N> result;
      for(unsigned i = 0; i < N; ++i) {
        result[i] = mRanges[i].Clip(r[i]);
      }
      return result;
    }
    //: The value 'r' is clipped to be within this range.

    [[nodiscard]] bool contains(const Range<RealT, N> &oth) const
    {
      for(unsigned i = 0; i < N; ++i) {
        if(!mRanges[i].contains(oth.range(i))) {
          return false;
        }
      }
      return true;
    }

    //: Returns true if this range contains the subrange 'oth'.

    [[nodiscard]] bool contains(const Vector<RealT, N> &oth) const
    {
      for(unsigned i = 0; i < N; ++i) {
        if(!mRanges[i].contains(oth[i])) {
          return false;
        }
      }
      return true;
    }
    //: Returns true if this range contains the subrange 'oth'.

    inline const Range &operator+=(const Vector<RealT, N> &offset);
    //: Shifts the rectangle to the new position.

    inline const Range &operator-=(const Vector<RealT, N> &offset);
    //: Shifts the rectangle to the new position.

    inline Range<RealT, N> operator+(const Vector<RealT, N> &offset) const
    {
      Range<RealT, N> ret;
      for(unsigned i = 0; i < N; ++i) {
        ret[i] = mRanges[i] + offset[i];
      }
      return ret;
    }
    //: Shifts the rectangle to the new position.

    inline Range<RealT, N> operator-(const Vector<RealT, N> &offset) const
    {
      Range<RealT, N> ret;
      for(unsigned i = 0; i < N; ++i) {
        ret[i] = mRanges[i] - offset[i];
      }
      return ret;
    }
    //: Shifts the rectangle to the new position.

    [[nodiscard]] inline const Range<RealT, 1> &range(unsigned d) const
    {
      return mRanges[d];
    }
    //: Access row range.

    [[nodiscard]] inline Range<RealT, 1> &range(unsigned d)
    {
      return mRanges[d];
    }
    //: Access row range.

    //! Ensures this rectangle contains given index.
    //! This method checks and changes, if necessary, the 2-dimensional range
    //! to contain the 'index'.
    inline void involve(const Vector<RealT, N> &index);

    //! Ensures this rectangle contains given sub rectangle.
    //! This method checks and changes, if necessary, the 2-dimensional range
    //! to contain the 'subrectangle'.
    inline void involve(const Range<RealT, N> &subrectangle)
    {
      for(unsigned i = 0; i < N; i++)
        mRanges[i].involve(subrectangle.range(i));
    }

    //! Returns true if this rectangle contains at least one index.
    [[nodiscard]] inline bool IsValid() const
    {
      for(unsigned i = 0; i < N; i++)
        if(!mRanges[i].IsValid())
          return false;
      return true;
    }

    //! Are two ranges equal ?
    bool operator==(const Range<RealT, N> &oth) const
    {
      for(unsigned i = 0; i < N; i++) {
        if(oth.range(i) != mRanges[i])
          return false;
      }
      return true;
    }

    //! Are two ranges unequal ?
    bool operator!=(const Range<RealT, N> &oth) const
    {
      for(unsigned i = 0; i < N; i++) {
        if(oth.range(i) != mRanges[i])
          return true;
      }
      return false;
    }

    //Range Rotate180(VectorC<RealT,N> centre);
    //: Rotate rectangle 180 degree's around the given center.

    //! Returns true if this range contains a common area with
    //! the range 'r'.
    [[nodiscard]] inline bool overlaps(const Range<RealT, N> &r) const
    {
      for(unsigned i = 0; i < N; i++) {
        if(!mRanges[i].overlaps(r.range(i))) {
          return false;
        }
      }
      return true;
    }

    //! Get the smallest integer range containing the real range.
    [[nodiscard]] IndexRange<N> toIndexRange() const
    {
      Ravl2::IndexRange<N> ret;
      for(unsigned i = 0; i < N; i++) {
        ret[i] = mRanges[i].toIndexRange();
      }
      return ret;
    }

    //! scale range
    [[nodiscard]] Range<RealT, N> operator*(const Vector<RealT, N> &scale) const
    {
      Range<RealT, N> ret;
      for(unsigned i = 0; i < N; i++) {
        ret[i] = mRanges[i] * scale[i];
      }
      return ret;
    }

    //! scale range
    [[nodiscard]] Range<RealT, N> operator*(RealT scale) const
    {
      Range<RealT, N> ret;
      for(unsigned i = 0; i < N; i++) {
        ret[i] = mRanges[i] * scale;
      }
      return ret;
    }

    //! Access item.
    Range<RealT, 1> &operator[](unsigned ind)
    {
      return mRanges[ind];
    }

    const Range<RealT, 1> &operator[](unsigned ind) const
    {
      return mRanges[ind];
    }
    //: Access item.

  private:
    std::array<Range<RealT, 1>, N> mRanges;
  };

  template <typename RealT, unsigned N>
  IndexRange<N> operator*(const Range<RealT, N> &realRange, const IndexRange<N> &indexRange);
  //: Multiply a 2d index range by a real 2d range.
  // Multiplying by a real range of 0-1,0-1 is a unit transform.

  template <typename RealT, unsigned N>
  std::ostream &operator<<(std::ostream &s, const Range<RealT, N> &ir);

  template <typename RealT, unsigned N>
  std::istream &operator>>(std::istream &s, Range<RealT, N> &ir);

  ///////////////////////////////////////////////////////

  template <typename RealT, unsigned N>
  inline const Range<RealT, N> &Range<RealT, N>::operator+=(const Vector<RealT, N> &offset)
  {
    for(unsigned i = 0; i < N; i++)
      mRanges[i] += offset[i];
    return *this;
  }

  template <typename RealT, unsigned N>
  inline const Range<RealT, N> &Range<RealT, N>::operator-=(const Vector<RealT, N> &offset)
  {
    for(unsigned i = 0; i < N; i++)
      mRanges[i] += offset[i];
    return *this;
  }

  template <typename RealT, unsigned N>
  inline void Range<RealT, N>::involve(const Vector<RealT, N> &index)
  {
    for(unsigned i = 0; i < N; i++)
      mRanges[i].involve(index[i]);
  }

  template <typename RealT, unsigned N>
  inline Range<RealT, N> toRange(const IndexRange<N> &ir)
  {
    Range<RealT, N> ret;
    for(unsigned i = 0; i < N; i++)
      ret[i] = toRange<RealT>(ir[i]);
    return ret;
  }

  template <typename RealT>
  inline Range<RealT, 1> toRange(IndexRange<1> ir)
  {
    return Range<RealT, 1>(RealT(ir.min()), RealT(ir.max() + 1));
  }

  template <typename RealT, unsigned N>
  inline IndexRange<N> toIndexRange(const Range<RealT, N> &ir)
  {
    IndexRange<N> ret;
    for(unsigned i = 0; i < N; i++)
      ret[i] = toIndexRange(ir[i]);
    return ret;
  }

  template <typename RealT>
  inline IndexRange<1> toIndexRange(Range<RealT, 1> ir)
  {
    return IndexRange<1>(int_floor(ir.min()), int_ceil(ir.max()));
  }

  //! Serialization support
  template <class Archive, typename RealT>
  void serialize( Archive & archive, Range<RealT, 1> & range )
  {
    cereal::size_type s = 2;
    archive(cereal::make_size_tag(s));
    if (s != 2) {
      throw std::runtime_error("range has incorrect length");
    }
    archive( range.min(), range.max() );
  }

  //! Serialization support
  template <class Archive,typename RealT, unsigned N>
  requires (N > 1)
  void serialize( Archive & archive, Range<RealT, N> & range )
  {
    cereal::size_type s = N;
    archive(cereal::make_size_tag(s));
    if (s != N) {
      throw std::runtime_error("range has incorrect length");
    }
    for (auto& r : range.ranges()) {
      archive(r);
    }
  }



  extern template class Range<float, 1>;
  extern template class Range<float, 2>;

}// namespace Ravl2
