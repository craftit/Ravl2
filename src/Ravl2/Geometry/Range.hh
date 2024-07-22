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

  template<typename RealT,unsigned N>
  class Range;

  //! userlevel=Normal
  //: 1D Range of real values.

  template<typename RealT>
  class Range<RealT,1>
  {
  public:
    //:----------------------------------------------
    // Constructors, copy, assigment, and destructor.

    // Creates the index range <0, dim-1>.
    inline explicit Range(RealT size = 0)
        : minV(0),
          maxV(size)
    {}

    //! Create real range from an IndexRange.
    // Note that the upper limit of the Range object is incremented by 1
    // to make the range consistent.
    explicit inline Range(const IndexRange<1> &rng)
        : minV(rng.min()),
          maxV(rng.max()+1)
    {}

    inline Range(RealT minReal, RealT maxReal)
        : minV(minReal),
          maxV(maxReal)
    {}
    //: Creates the index range <minReal, maxReal>.

    inline explicit Range(std::istream & s);
    //: Creates the index range from the input stream.

    //:---------------------------------
    //: Access to the object information.

    [[nodiscard]] inline RealT Size() const
    { return (maxV-minV); }
    //: Returns the number of elements in the range.

    [[nodiscard]] inline const Range<RealT,1> & range() const
    { return *this; }
    //: Returns this object.

    [[nodiscard]] inline const RealT & min()  const
    { return minV; }
    //: Returns the minimum index of the range.

    [[nodiscard]] inline const RealT & max()  const
    { return maxV; }
    //: Returns the maximum index of the range.

    inline RealT & min()
    { return minV; }
    //: Returns the minimum index of the range.

    inline RealT & max()
    { return maxV; }
    //: Returns the maximum index of the range.

    [[nodiscard]] inline RealT Center() const
    { return (min() + max())/2; }
    //: Returns the index in the middle of the range, eg. (max()+min())/2.

    [[nodiscard]] inline RealT CenterD() const
    { return (min() + max())/2; }
    //: Returns the index previous the middle of the range, eg. (max()+min())/2.

    [[nodiscard]] inline RealT Percentage(RealT p) const
    { return (max() - min()) * p/100.0 + min(); }
    //: Returns the index which is in the 'p' % of the whole range.

    //:-------------------
    //: Logical operations.

    [[nodiscard]] inline bool IsEmpty() const
    { return min() > max(); }
    //: Returns true if the minimum limit is bigger than the maximum limit.

    [[nodiscard]] inline bool IsValid() const
    { return min() <= max(); }
    //: Returns true if the minimum limit is smaller than or equal to the maximum value

    [[nodiscard]] inline bool Contains(RealT i) const
    { return (min() <= i) && (i <= max()); }
    //: Returns true if this range contains the index 'i'.

    [[nodiscard]] inline bool Contains(const Range & range) const
    { return Contains(range.min()) && Contains(range.max()); }
    //: Returns true if this range contains the subrange 'range'.

    inline bool operator==(const Range & range) const
    { return (min() == range.min()) && (max() == range.max()); }
    //: Returns true if both index ranges have the same limits.

    inline bool operator!=(const Range & range) const
    { return (min() != range.min()) || (max() != range.max()); }
    //: Returns true if both the ranges have different limits.

    [[nodiscard]] bool In(const Range & range) const;
    //: Returns true if this range is inside of the 'range'.

    [[nodiscard]] inline bool IsOverlapping(const Range & r) const;
    //: Returns true if this range contains at least one common index with
    //: the range 'r'.

    //:-------------------
    //: Special operations.

    inline const Range &SetOrigin(RealT position) {
      max() = position + max() - min();
      min() = position;
      return *this;
    }
    //: Set the origin of the range to 'position'.
    // Returns a reference to this range.

    inline Range &operator++()
    { min()++; max()++; return *this; }
    //: Move both the max and min of the range along 1.
    // Returns a reference to this range.

    inline Range &operator--()
    { min()--; max()--; return *this; }
    //: Move both the max and min of the range back 1.
    // Returns a reference to this range.

    inline const Range & operator+=(RealT i);
    //: Both minimum and maximum limits are shifted by adding the offset 'i'.
    // Returns a reference to this range.

    inline const Range & operator-=(RealT i);
    //: Both minimum and maximum limits are shifted by subtracting the offset 'i'.
    // Returns a reference to this range.

    inline Range operator+(RealT i) const
    { return Range(min() + i,max() + i); }
    //: Create a new Range with minimum and maximum limits shifted by adding the offset 'i'.

    inline Range operator-(RealT i) const
    { return Range(min() - i,max() - i); }
    //: Create a new Range with minimum and maximum limits shifted by subtracting the offset 'i'.

    inline Range & ClipBy(const Range & r);
    //: This index range is clipped to contain at most the index range 'r'.

    [[nodiscard]] inline RealT Clip(const RealT & r) const {
      RealT lower = min() > r? min(): r;
      return lower < max()? lower: max();
    }
    //: The value 'r' is clipped to be within this range.

    [[nodiscard]] inline Range FirstHalf() const
    { return Range(min(),Center()); }
    //: Returns the index range < min(), (max()+min())/2 >.

    [[nodiscard]] inline Range FirstHalfD() const
    { return Range(min(),CenterD()); }
    //: Returns the index range < min(), (max()+min())/2 >.

    [[nodiscard]] inline Range Enlarge(RealT f) const
    { return Range(min(), min() + Size()*f - 1); }
    //: Returns the index range whose number of elements is enlarged by
    //: the factor 'f'. The upper limits is changed.

    [[nodiscard]] inline Range Expand(RealT n) const
    { return Range(minV - n,maxV + n); }
    //: Returns the range extended by adding 'n' items on both limits of
    //: this range.

    [[nodiscard]] inline Range Shrink(RealT n) const
    { return Range(minV + n,maxV - n); }
    //: Returns the range extended by adding 'n' items on both limits of
    //: this range.

    inline Range & ShrinkHigh(RealT n)
    { max() -= n; return *this; }
    //: Returns the range shrinked by removing of the
    //: last 'n' items on both limits of this range.

    Range operator*(RealT scale) const
    { return Range(min()*scale,max()*scale); }
    //: Scale range

    const Range &Involve(RealT i) {
      if(minV > i) minV = i;
      if(maxV < i) maxV = i;
      return *this;
    }
    //: Modify this range to ensure index i is contained within it.

    const Range &Involve(const Range &subRange) {
      Involve(subRange.min());
      Involve(subRange.max());
      return *this;
    }
    //: Modify this range to ensure subRange is contained within it.

    [[nodiscard]] IndexRange<1> toIndexRange() const
    { return IndexRange<1>(int_floor(minV),int_ceil(maxV));  }
    //: Get the smallest integer range containing the real range.

  private:
    RealT minV = 0; // Minimum index.
    RealT maxV = 0; // Maximum index.

    //friend std::istream & operator>>(std::istream & s, Range & range);
  };

  template<typename RealT>
  inline bool IsInside(RealT i, const Range<RealT,1> & range)
  { return (range.min() <= i) && (i <= range.max()); }
  //: Returns true if the index 'i' is inside the index range 'r'.

  template<typename RealT>
  IndexRange<2> operator*(const Range<RealT,1> &realRange,const IndexRange<2> &indexRange);
  //: Multiply an index range by a real range.
  // Multiplying by a real range of 0-1 is a unit transform.

  template<typename RealT>
  std::istream & operator>>(std::istream & s, Range<RealT,1> & r);
  //: Read range from input stream.
  // Read information from the intput stream 's' and sets the real range
  // according obtained data.

  template<typename RealT>
  std::ostream & operator<<(std::ostream & s, const Range<RealT,1> & r);
  //: Saves the index range 'r' into the output stream 's'.


  template<typename RealT>
  inline bool Range<RealT,1>::IsOverlapping(const Range & r) const {
    return (!IsEmpty() && !r.IsEmpty())
           && (((min() <= r.max()) && (r.min() <= max()))
               || ((r.min() <= max()) && (min() <= r.max())));
  }

  template<typename RealT>
  inline const Range<RealT,1> & Range<RealT,1>::operator+=(RealT i) {
    min() += i;
    max() += i;
    return *this;
  }

  template<typename RealT>
  inline const Range<RealT,1> &Range<RealT,1>::operator-=(RealT i) {
    min() -= i;
    max() -= i;
    return *this;
  }

  template<typename RealT>
  inline Range<RealT,1> &Range<RealT,1>::ClipBy(const Range & r) {
    if (min() < r.min()) {
      min() = r.min();
      if(max() < r.min()) // Make sure there is some overlap.
        max() = r.min(); // Make range zero size.
    }
    if (max() > r.max()) {
      max() = r.max();
      if(min() > r.max()) // Make sure there is some overlap.
        min() = r.max(); // To make range zero size.
    }
    return *this;
  }


  //-----------------------------------------------------------------------------
  //! userlevel=Normal
  //: An index range of a 2D array

  template<typename RealT,unsigned N>
  class Range
  {
  public:
    //: Default constructor.
    Range()
    = default;

    //! Construct from an IndexRange<N>.
    // Note that the upper limits of the Range object are incremented by 1
    // to make the range consistent.
    explicit Range(const IndexRange<N> &rng)
    {
      for (int i = 0; i < N; ++i) {
        ranges[i] = rng[i];
      }
    }

    //! Constructor.
    Range(std::initializer_list<Range<RealT,1>> list)
     : ranges( list)
    {}

    //! Constructor.
    Range(const Range<RealT,N> & range) = default;

    //! Create an 2d range from corner points.
    Range(const Vector<RealT,N> &org,const Vector<RealT,N> &end)
    {
      for (int i = 0; i < N; ++i) {
        ranges[i] = Range<RealT,1>(org[i],end[i]);
      }
    }

    //! Create an 2d range from a center point and a size.
    // Size is the distance from the center to the edge, so
    // a size of 0 gives a single pixel, and a size of 1 generates
    // a 3x3 square.
    Range(const Vector<RealT,N> &center,RealT size)
    {
      for (int i = 0; i < N; ++i) {
        ranges[i] = Range<RealT,1>(center[i] - size/2,center[i] + size/2);
      }
    }

    //! Set the origin of the range to 'newOrigin'
    // Returns a reference to this rectangle.
    const Range &SetOrigin(const Vector<RealT,N> &newOrigin) {
      for (int i = 0; i < N; ++i) {
        ranges[i].SetOrigin(newOrigin[i]);
      }
      return *this;
    }

    //: Returns the top-left index of the rectangle.
    [[nodiscard]] inline Vector<RealT,N> Origin() const
    { 
      Vector<RealT,N> ret;
      for (int i = 0; i < N; ++i) {
        ret[i] = ranges[i].min();
      }
      return ret;
    }

    //: Returns the bottom-right index of the rectangle.
    [[nodiscard]] inline Vector<RealT,N>  End() const
    {
      Vector<RealT,N> ret;
      for (int i = 0; i < N; ++i) {
        ret[i] = ranges[i].max();
      }
      return ret;
    }
      

    //! Returns the index which is in the middle of the rectangle
    [[nodiscard]] inline Vector<RealT,N> Center() const
    {
      Vector<RealT,N> ret;
      for (int i = 0; i < N; ++i) {
        ret[i] = ranges[i].Center();
      }
      return ret;
    }

    //! Returns the area of the image rectangle expressed in number of indexs.
    [[nodiscard]] inline RealT Area() const
    {
      RealT area = 1;
      for (int i = 0; i < N; ++i) {
        area *= ranges[i].Size();
      }
      return area;
    }


    //! Returns an rectangle expanded by 'n' on each side.
    [[nodiscard]] inline Range Expand(RealT n) const
    {
        Range ret;
        for (int i = 0; i < N; ++i) {
          ret[i] = ranges[i].Expand(n);
        }
        return ret;
    }

    //! Returns a new rectangle which has layer of the width of 'n'
    //! removed.
    [[nodiscard]] inline Range Shrink(RealT n) const
    {
      Range ret;
      for (int i = 0; i < N; ++i) {
        ret[i] = ranges[i].Shrink(n);
      }
      return ret;
    }

    inline Range & ClipBy(const Range & r) {
      for (int i = 0; i < N; ++i) {
        ranges[i].ClipBy(r.Range1());
      }
      return *this;
    }
    //: This index range is clipped to contain at most the index range 'r'.

    inline Vector<RealT,N> Clip(const Vector<RealT,N> & r) {
      Vector<RealT,N> result;
      for (int i = 0; i < N; ++i) {
        result[i] = ranges[i].Clip(r[i]);
      }
      return result;
    }
    //: The value 'r' is clipped to be within this range.

    [[nodiscard]] inline bool Contains(const Range<RealT,N> & oth) const
    {
      for (int i = 0; i < N; ++i) {
        if (!ranges[i].Contains(oth[i])) {
          return false;
        }
      }
      return true;
    }

    //: Returns true if this range contains the subrange 'oth'.

    [[nodiscard]] inline bool Contains(const Vector<RealT,N> & oth) const
    {
      for (int i = 0; i < N; ++i) {
        if (!ranges[i].Contains(oth[i])) {
          return false;
        }
      }
      return true;
    }
    //: Returns true if this range contains the subrange 'oth'.

    inline const Range & operator+=(const Vector<RealT,N> & offset);
    //: Shifts the rectangle to the new position.

    inline const Range & operator-=(const Vector<RealT,N> & offset);
    //: Shifts the rectangle to the new position.

    inline Range<RealT,N> operator+(const Vector<RealT,N> & offset) const
    {
      Range<RealT,N> ret;
      for (int i = 0; i < N; ++i) {
        ret[i] = ranges[i] + offset[i];
      }
      return ret;
    }
    //: Shifts the rectangle to the new position.

    inline Range<RealT,N> operator-(const Vector<RealT,N> & offset) const
    {
      Range<RealT,N> ret;
      for (int i = 0; i < N; ++i) {
        ret[i] = ranges[i] - offset[i];
      }
      return ret;
    }
    //: Shifts the rectangle to the new position.
    
    [[nodiscard]] inline const Range<RealT,N> & range(unsigned d) const
    { return ranges[d]; }
    //: Access row range.

    [[nodiscard]] inline Range<RealT,N> & range(unsigned d)
    { return ranges[d]; }
    //: Access row range.

    //! Ensures this rectangle contains given index.
    // This method checks and changes, if necessary, the 2-dimensional range
    // to contain the 'index'.
    inline void Involve(const Vector<RealT,N> & index);

    //! Ensures this rectangle contains given sub rectangle.
    // This method checks and changes, if necessary, the 2-dimensional range
    // to contain the 'subrectangle'.
    inline void Involve(const Range<RealT,N> &subrectangle)
    {
      for(unsigned i = 0; i < N; i++)
        ranges[i].Involve(subrectangle[i]);
    }

    //! Returns true if this rectangle contains at least one index.
    [[nodiscard]] inline bool IsValid() const
    {
      for(unsigned i = 0; i < N; i++)
        if(!ranges[i].IsValid())
          return false;
      return true;
    }

    //! Are two ranges equal ?
    bool operator==(const Range<RealT,N> &oth) const
    {
      for (unsigned i = 0; i < N; i++) {
        if (oth.range(i) != ranges[i])
          return false;
      }
      return true;
    }


    //! Are two ranges unequal ?
    bool operator!=(const Range<RealT,N> &oth) const
    {
      for (unsigned i = 0; i < N; i++) {
        if (oth.range(i) != ranges[i])
          return true;
      }
      return false;
    }

    //Range Rotate180(VectorC<RealT,N> centre);
    //: Rotate rectangle 180 degree's around the given center.

    //! Returns true if this range contains a common area with
    //! the range 'r'.
    [[nodiscard]] inline bool IsOverlapping(const Range<RealT,N> & r) const
    {
      for (unsigned i = 0; i < N; i++) {
        if (!ranges[i].IsOverlapping(r.range(i))) {
          return false;
        }
      }
      return true;
    }

    //! Get the smallest integer range containing the real range.
    [[nodiscard]] IndexRange<N> toIndexRange() const
    {
      Ravl2::IndexRange<N> ret;
      for (unsigned i = 0; i < N; i++) {
        ret[i] = ranges[i].toIndexRange();
      }
      return ret;
    }

    //! Scale range
    [[nodiscard]] Range<RealT,N> operator*(const Vector<RealT,N> &scale) const
    {
      Range<RealT,N> ret;
      for (unsigned i = 0; i < N; i++) {
        ret[i] = ranges[i] * scale[i];
      }
      return ret;
    }

    //! Scale range
    [[nodiscard]] Range<RealT,N> operator*(RealT scale) const
    {
      Range<RealT,N> ret;
      for (unsigned i = 0; i < N; i++) {
        ret[i] = ranges[i] * scale;
      }
      return ret;
    }

    //! Access item.
    Range<RealT,1> &operator[](unsigned ind)
    { return ranges[ind]; }

    const Range<RealT,N> &operator[](unsigned ind) const
    { return ranges[ind]; }
    //: Access item.

  private:
    std::array<Range<RealT,1>, N> ranges;
  };

  template<typename RealT,unsigned N>
  IndexRange<N> operator*(const Range<RealT,N> &realRange,const IndexRange<N> &indexRange);
  //: Multiply a 2d index range by a real 2d range.
  // Multiplying by a real range of 0-1,0-1 is a unit transform.

  template<typename RealT,unsigned N>
  std::ostream &operator<<(std::ostream &s,const Range<RealT,N> &ir);

  template<typename RealT,unsigned N>
  std::istream &operator>>(std::istream &s,Range<RealT,N> &ir);

  ///////////////////////////////////////////////////////

  template<typename RealT,unsigned N>
  inline
  const Range<RealT,N> &Range<RealT,N>::operator+=(const Vector<RealT,N> & offset) {
    for(unsigned i = 0; i < N; i++)
      ranges[i] += offset[i];
    return *this;
  }

  template<typename RealT,unsigned N>
  inline
  const Range<RealT,N> & Range<RealT,N>::operator-=(const Vector<RealT,N> & offset) {
    for(unsigned i = 0; i < N; i++)
      ranges[i] += offset[i];
    return *this;
  }

  template<typename RealT,unsigned N>
  inline
  void Range<RealT,N>::Involve(const Vector<RealT,N> & index) {
    for(unsigned i = 0; i < N; i++)
      ranges[i].Involve(index[i]);
  }

}

