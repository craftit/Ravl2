// This file is part of RAVL, Recognition And Vision Library
// Copyright (C) 2003, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here
//! author="Charles Galambos"
//! date="11/10/1999"

#pragma once

#include <vector>
#include "Ravl2/IndexRange.hh"

namespace Ravl2
{
  //! Rectangle set.
  //! Handles a set region defined by a set of non-overlapping rectangles.
  //! The methods in this class ensure that each part of the range is only
  //! covered by a single rectangle.
  //! The implementation is not particularly efficient, but it is simple.
  //! and should be fine for small numbers of rectangles.

  template <unsigned N>
  class IndexRangeSet : public std::vector<IndexRange<N>>
  {
  public:
    using value_type = Index<N>;
    constexpr static unsigned dimensions = N;

    constexpr IndexRangeSet() = default;

    //! Constructor from a single range.
    explicit constexpr IndexRangeSet(const IndexRange<N> &rect)
    {
      this->push_back(rect);
    }

    //! Subtract rect2 from rect1.
    static constexpr IndexRangeSet subtract(const IndexRange<N> &rect1, const IndexRange<N> &rect2)
    {
      IndexRangeSet ret;
      if(!rect1.overlaps(rect2)) {
        ret = IndexRangeSet(rect1);
        return ret;
      }
      ret.reserve(4);
      IndexRange<N> remainder(rect1);

      for(unsigned i = 0; i < N; ++i) {
        if(remainder.min(i) < rect2.min(i)) {
          IndexRange<N> r;
          for(unsigned j = 0; j < N; ++j) {
            if(j == i) {
              r.min(j) = remainder.min(j);
              r.max(j) = rect2.min(j) - 1;
            } else {
              r.min(j) = remainder.min(j);
              r.max(j) = remainder.max(j);
            }
          }
          ret.push_back(r);
          remainder.min(i) = rect2.min(i);// Cut it down.
        }
        if(remainder.max(i) > rect2.max(i)) {
          IndexRange<N> r;
          for(unsigned j = 0; j < N; ++j) {
            if(j == i) {
              r.min(j) = rect2.max(j) + 1;
              r.max(j) = remainder.max(j);
            } else {
              r.min(j) = remainder.min(j);
              r.max(j) = remainder.max(j);
            }
          }
          ret.push_back(r);
          remainder.max(i) = rect2.max(i);// Cut it down.
        }
      }

      return ret;
    }

    //! Add rect2 and rect1.
    static constexpr IndexRangeSet add(const IndexRange<N> &rect1, const IndexRange<N> &rect2)
    {
      IndexRangeSet ret;
      if(!rect1.overlaps(rect2)) {// No overlap.
        ret.push_back(rect1);
        ret.push_back(rect2);
        return ret;
      }
      // Try and keep rects approximately the same size....
      if(rect1.area() > rect2.area()) {
        auto diff = subtract(rect1, rect2);
        ret.insert(std::end(ret), std::begin(diff), std::end(diff));
        ret.push_back(rect2);
      } else {
        auto diff = subtract(rect2, rect1);
        ret.insert(std::end(ret), std::begin(diff), std::end(diff));
        ret.push_back(rect1);
      }
      return ret;
    }

    //! Get axis align enclosing rectangle for set.
    [[nodiscard]] constexpr IndexRange<N> enclosingRange() const
    {
      auto it = this->begin();
      auto end = this->end();
      if(it == end)
        return IndexRange<N>();
      IndexRange<N> ret(*it);
      ++it;
      for(; it != end; ++it)
        ret.involve(*it);
      return ret;
    }

    //! Remove 'rect' rectangle from the region given by the set.
    [[nodiscard]] constexpr IndexRangeSet<N> subtract(const IndexRange<N> &rect) const
    {
      IndexRangeSet<N> ret;
      for(auto it : *this) {
        auto diff = subtract(it, rect);
        ret.insert(std::end(ret), std::begin(diff), std::end(diff));
      }
      return ret;
    }

    //! Remove 'rectset' from the region given by the set.
    [[nodiscard]] constexpr IndexRangeSet<N> subtract(const IndexRangeSet<N> &rectSet) const
    {
      IndexRangeSet<N> ret = (*this);
      for(auto it : rectSet)
        ret = ret.subtract(it);
      return ret;
    }

    //! Remove set from rect.
    [[nodiscard]] constexpr IndexRangeSet<N> subtractFrom(const IndexRange<N> &rect) const
    {
      return IndexRangeSet<N>(rect).subtract(*this);
    }

    //! Add this rectangle to the set.
    [[nodiscard]] constexpr IndexRangeSet<N> add(const IndexRange<N> &rect) const
    {
      IndexRangeSet<N> ret = subtract(rect);
      ret.push_back(rect);// Well it works doesn't it!!!!
      return ret;
    }

    //! Add rectangle set to this set.
    [[nodiscard]] constexpr IndexRangeSet<N> add(const IndexRangeSet<N> &rect) const
    {
      IndexRangeSet<N> ret(*this);
      for(auto it : rect)
        ret = ret.add(it);
      return ret;
    }

    //! Does this set completely contain 'rect' ?
    [[nodiscard]] constexpr bool contains(const IndexRange<N> &rect) const
    {
      IndexRangeSet<N> remainder(rect);
      for(auto it : *this) {
        remainder = remainder.subtract(it);
        if(remainder.empty())
          return true;
      }
      return false;
    }

    //! Total area of the set.
    [[nodiscard]] constexpr int area() const
    {
      int ret = 0;
      for(auto it : *this) {
        ret += it.area();
      }
      return ret;
    }
  };

#if 1
    template<class Archive,unsigned N>
    void save(Archive & archive,
              IndexRangeSet<N> const & m)
    {
      cereal::size_type s = m.size();
      archive(cereal::make_size_tag(s));
      for(auto &i : m) {
        archive(i);
      }
    }

    template<class Archive,unsigned N>
    void load(Archive & archive,
              IndexRangeSet<N> & m)
    {
      cereal::size_type s = m.size();
      archive(cereal::make_size_tag(s));
      if(s != m.size()) {
        m.resize(s);
      }
      for(auto &i : m) {
        archive(i);
      }
    }

#endif

  // Let everyone know there's an implementation already generated for common cases
  extern template class IndexRangeSet<3>;
  extern template class IndexRangeSet<2>;
  extern template class IndexRangeSet<1>;
}// namespace Ravl2
