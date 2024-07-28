// This file is part of RAVL, Recognition And Vision Library 
// Copyright (C) 2002, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here
#ifndef RAVLIMAGE_SUMMEDAREATABLE_HEADER
#define RAVLIMAGE_SUMMEDAREATABLE_HEADER 1
//! rcsid="$Id$"
//! author="Charles Galambos"
//! date="3/4/2002"
//! docentry="Ravl.API.Images.Misc"
//! lib=RavlImageProc
//! file="Ravl/Image/Processing/Tools/SummedAreaTable.hh"

#include "Ravl/Image/Image.hh"
#include "Ravl/Array2dSqr2Iter2.hh"
#include "Ravl/Image/DrawFrame.hh"

namespace RavlImageN {

  //! userlevel=Normal
  //: Summed Area Table.
  // This class allows the summing of any area in an image in constant time.
  // The class builds the table with a single pass over the input image. Once
  // this is done the sum of any area can be computed by looking up the corners
  // of the rectangle. 
  
  template<class DataT>
  class SummedAreaTableC
    : public Array2dC<DataT>
  {
  public:
    SummedAreaTableC()
    {}
    //: Default constructor.
    
    template<class InT>
    SummedAreaTableC(const Array2dC<InT> &in) 
    { BuildTable(in); }
    
    template<class InT>
    void BuildTable(const Array2dC<InT> &in) {
      clipRange = in.Frame();
      IndexRange2dC rng(in.Frame());
      rng.LCol()--;
      rng.TRow()--;
      if(this->Frame() != rng) {
	DataT zero;
	SetZero(zero); // We can't rely on '= 0' working.
	(*this).Array2dC<DataT>::operator=(Array2dC<DataT>(rng));
	DrawFrame((*this),zero,rng); // We only really need the top row and left column cleared.
      }
      Array2dC<DataT> work((*this),in.Frame());
      Array2dSqr2Iter2C<DataT,InT> it(work,in);
      // First pixel.
      it.DataTL1() = (DataT) it.DataTL2();
      DataT rowSum = (DataT) it.DataBL2();
      it.DataBL1() = it.DataTL1() + rowSum;
      // Do first row.
      do {
	it.DataTR1() = it.DataTL1() + (DataT) it.DataTR2();
	rowSum += (DataT) it.DataBR2();
	it.DataBR1() = it.DataTR1() + rowSum;
      } while(it.Next()) ;
      // Do rest of image.
      for(;it;) {
	// Do beginning of row thing.
	rowSum = (DataT) it.DataBL2();
	it.DataBL1() = it.DataTL1() + rowSum;
	// Do rest of row.
	do {
	  rowSum += (DataT) it.DataBR2();
	  it.DataBR1() = it.DataTR1() + rowSum;
	} while(it.Next()) ;
      }
    }
    //: Build table form an array of values.
    
    DataT Sum(IndexRange2dC range) const {
      range.ClipBy(clipRange);
      if(range.Area() == 0)
	return (*this)[this->Frame().Origin()]; // Return 0.
      range.LCol()--;
      range.TRow()--;
      // Could speed this up by seperating out row accesses ?
      return (*this)[range.End()] - (*this)[range.TopRight()] - (*this)[range.BottomLeft()] + (*this)[range.TopLeft()];
    }
    //: Calculate the sum of the pixel's in the rectangle 'range'.
    
    DataT VerticalDifference2(IndexRange2dC range,IntT mid) const {
      // Could speed this up by seperating out row accesses ?
      return (*this)[range.TopLeft()] 
	+ ((*this)[mid][range.RCol()] - (*this)[mid][range.LCol()]) * 2 
	- (*this)[range.BottomLeft()] 
	- (*this)[range.TopRight()]
	+ (*this)[range.End()];
    }
    //: Calculate the diffrence between two halfs of the rectangle split vertically.
    // This mid point is an absolute row location and should be within the rectangle.
    
    DataT HorizontalDifference2(IndexRange2dC range,IntT mid) const {
      // Could speed this up by seperating out row accesses ?
      return (*this)[range.TopLeft()]
	+ ((*this)[range.BRow()][mid] - (*this)[range.TRow()][mid]) * 2 
	- (*this)[range.BottomLeft()] 
	+ (*this)[range.TopRight()]
	- (*this)[range.End()];
    }
    //: Calculate the diffrence between two halfs of the rectangle split horizontally.
    // This mid point is an absolute column location and should be within the rectangle.

    DataT VerticalDifference3(const IndexRange2dC &range,const IndexRangeC &rng) const {
      RavlAssert(range.Range2().Contains(rng));
      IndexRange2dC rng2(range.Range1(),rng);
      return Sum(range) - Sum(rng2);
    }
    //: Calculate the diffrence between two halfs of the rectangle split vertially.
    // This mid point is an absolute row location and should be within the rectangle.
    
    DataT HorizontalDifference3(const IndexRange2dC &range,const IndexRangeC &rng) const {
      RavlAssert(range.Range1().Contains(rng));
      IndexRange2dC rng2(rng,range.Range2());
      return Sum(range) - Sum(rng2);
    }
    //: Calculate the diffrence between two rectangles one lying inside the other in the horizontal dimention.
    // This mid point is an absolute column location and should be within the rectangle.
    
    const IndexRange2dC &ClipRange() const
    { return clipRange; }
    //: Return range of value positions. 
    
  protected:
    IndexRange2dC clipRange;
  };
  
}

#endif
