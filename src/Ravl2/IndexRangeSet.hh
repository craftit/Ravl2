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
#include "Ravl2/Index.hh"

namespace Ravl2 {
  //! Rectangle set.
  // Handles a set region defined by a set of non-overlapping rectangles.
  // The methods in this class ensure that each part of the range is only
  // covered by a single rectangle.
  
  template<unsigned N>
  class IndexRangeSet
    : public std::vector<IndexRange<N>>
  {
  public:
    IndexRangeSet()
    = default;

    //! Constructor from a single range.
    explicit IndexRangeSet(const IndexRange<N> &rect)
    { this->push_back(rect); }

    //! Subtract rect2 from rect1.
    static IndexRangeSet subtract(const IndexRange<N> &rect1,const IndexRange<N> &rect2)
    {
      IndexRangeSet ret;
      if(!rect1.overlaps(rect2)) {
        ret = IndexRangeSet(rect1);
        return ret;
      }
      IndexRange<N> remainder(rect1);

      //ONDEBUG(std::cerr << "Rectangles overlap. \n");
      
      // Cut top.

      if(remainder.min(0) < rect2.min(0)) {
        //ONDEBUG(std::cerr << "Top Cut. \n");
        ret.push_back(IndexRange<N>({{remainder.min(0),rect2.min(0)-1},{remainder.min(1),remainder.max(1)}}));
        remainder.min(0) = rect2.min(0); // Cut it down.
      }

      // Cut left.

      if(remainder.min(1) < rect2.min(1)) {
        //ONDEBUG(std::cerr << "Left Cut. \n");
        ret.push_back(IndexRange<N>({{remainder.min(0),remainder.max(0)},{remainder.min(1),rect2.min(1)-1}}));
        remainder.min(1) = rect2.min(1); // Cut it down.
      }

      // Cut bottom.

      if(remainder.max(0) > rect2.max(0)) {
        //ONDEBUG(std::cerr << "Bottom Cut. \n");
        ret.push_back(IndexRange<N>({{rect2.max(0)+1,remainder.max(0)},{remainder.min(1),remainder.max(1)}}));
        remainder.max(0) = rect2.max(0); // Cut it down.
      }

      // Cut right.

      if(remainder.max(1) > rect2.max(1)) {
        //ONDEBUG(std::cerr << "Right Cut. \n");
        ret.push_back(IndexRange<N>({{remainder.min(0),remainder.max(0)},{rect2.max(1)+1,remainder.max(1)}}));
        remainder.max(1) = rect2.max(1); // Cut it down.
      }

      return ret;
    }

    //: Subtract rect2 from rect1.

    //: Add rect2 and rect1.
    static IndexRangeSet add(const IndexRange<N> &rect1,const IndexRange<N> &rect2)
    {
      IndexRangeSet ret;
      if(!rect1.overlaps(rect2)) { // No overlap.
        ret.push_back(rect1);
        ret.push_back(rect2);
        return ret;
      }
      // Try and keep rects approximately the same size....
      if(rect1.area() > rect2.area()) {
        ret += Subtract(rect1,rect2);
        ret.push_back(rect2);
      } else {
        ret += Subtract(rect2,rect1);
        ret.push_back(rect1);
      }
      return ret;
    }


    //! Get axis align enclosing rectangle for set.
    [[nodiscard]] IndexRange<N> enclosingRange() const
    {
      auto it = this->begin();
      auto end = this->end();
      if(it == end)
        return IndexRange<N>({});
      IndexRange<N> ret(*it);
      ++it;
      for(;it != end;++it)
        ret.involve(*it);
      return ret;
    }

    [[nodiscard]] IndexRangeSet subtract(const IndexRange<N> &rect) const
    {
      IndexRangeSet ret;
      for(auto it : *this)
        ret += Subtract(it,rect);
      return ret;
    }

    //: Remove 'rect' rectangle from the region given by the set.

    [[nodiscard]] IndexRangeSet subtract(const IndexRangeSet &rectSet) const
    {
      IndexRangeSet ret = (*this);
      for(auto it : rectSet)
        ret = ret.Subtract(it);
      return ret;
    }

    //: Remove 'rectset' from the region given by the set.

    //! Remove set from rect.
    [[nodiscard]] IndexRangeSet subtractFrom(const IndexRange<N> &rect) const
    { return IndexRangeSet(rect).Subtract(*this); }

    [[nodiscard]] IndexRangeSet add(const IndexRange<N> &rect) const
    {
      IndexRangeSet ret = Subtract(rect);
      ret.push_back(rect); // Well it works doesn't it!!!!
      return ret;
    }
    //: Add this rectangle to the set.

    //! Add rectangle set to this set.
    [[nodiscard]] IndexRangeSet add(const IndexRangeSet &rect) const
    {
      IndexRangeSet ret(*this);
      for(auto it : *this)
        ret = ret.add(it);
      return ret;
    }

    //: Does this set completely contain 'rect' ?
    [[nodiscard]] bool contains(const IndexRange<N> &rect) const
    {
      IndexRangeSet remainder(rect);
      for(auto it : *this) {
        remainder = remainder.Subtract(it);
        if(remainder.empty())
          return true;
      }
      return false;
    }

    //! Total area of set.
    [[nodiscard]] int area() const
    {
      int ret = 0;
      for(auto it : *this) {
        ret += it.area();
      }
      return ret;
    }
  };

}


