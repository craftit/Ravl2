// This file is part of RAVL, Recognition And Vision Library 
// Copyright (C) 2002, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here
#ifndef RAVLIMAGE_SUMMEDAREATABLE2_HEADER
#define RAVLIMAGE_SUMMEDAREATABLE2_HEADER 1
//! rcsid="$Id$"
//! author="Charles Galambos"
//! date="28/11/2002"
//! docentry="Ravl.API.Images.Misc"
//! lib=RavlImageProc
//! file="Ravl/Image/Processing/Tools/SummedAreaTable2.hh"

#include "Ravl/Image/Image.hh"
#include "Ravl/Array2dSqr2Iter2.hh"
#include "Ravl/Image/DrawFrame.hh"
#include "Ravl/TFVector.hh"

namespace RavlImageN {

  //! userlevel=Normal
  //: Summed area and sum of squares table.
  // This class allows the summing of any area in an image in constant time.
  // The class builds the table with a single pass over the input image. Once
  // this is done the sum of any area can be computed by looking up the corners
  // of the rectangle. <br>
  // Note this class is more prone to overflow problems than normal SummedAreaTables's so
  // care must be taken when selecting types to use for the template.
  
  template<class DataT>
  class SummedAreaTable2C
    : public Array2dC<TFVectorC<DataT,2> >
  {
  public:
    SummedAreaTable2C()
    {}
    //: Default constructor
    
    template<class InT>
    SummedAreaTable2C(const Array2dC<InT> &in) 
    { BuildTable(in); }

  protected:    
    template<class InT>
    inline static TFVectorC<DataT,2> SumAndSqr(const InT &data) { 
      TFVectorC<DataT,2> ret;
      DataT val = static_cast<DataT>(data);
      ret[0] = val;
      ret[1] = Sqr(val);
      return ret;
    }
    //: Create vector containing value and its square.
    
  public:
    template<class InT>
    void BuildTable(const Array2dC<InT> &in) {
      clipRange = in.Frame();
      IndexRange2dC rng(in.Frame());
      rng.LCol()--;
      rng.TRow()--;
      if(this->Frame() != rng) {
	TFVectorC<DataT,2> zero;
	SetZero(zero);
	(*this).Array2dC<TFVectorC<DataT,2> >::operator=(Array2dC<TFVectorC<DataT,2> >(rng));
	DrawFrame((*this),zero,rng); // We only really need the top row and left column cleared.
      }
      Array2dC<TFVectorC<DataT,2> > work((*this),in.Frame());
      Array2dSqr2Iter2C<TFVectorC<DataT,2> ,InT> it(work,in);
      // First pixel.
      it.DataTL1() = SumAndSqr(it.DataTL2());
      TFVectorC<DataT,2> rowSum = SumAndSqr(it.DataBL2());
      it.DataBL1() = it.DataTL1() + rowSum;
      // Do first row.
      do {
	it.DataTR1() = it.DataTL1() + SumAndSqr(it.DataTR2());
	rowSum += SumAndSqr(it.DataBR2());
	it.DataBR1() = it.DataTR1() + rowSum;
      } while(it.Next()) ;
      // Do rest of image.
      for(;it;) {
	// Do beginning of row thing.
	rowSum = SumAndSqr(it.DataBL2());
	it.DataBL1() = it.DataTL1() + rowSum;
	// Do rest of row.
	do {
	  rowSum += SumAndSqr(it.DataBR2());
	  it.DataBR1() = it.DataTR1() + rowSum;
	} while(it.Next()) ;
      }
    }
    //: Build table form an array of values.
    
    TFVectorC<DataT,2> Sum(IndexRange2dC range) const {
      range.ClipBy(clipRange);
      if(range.Area() == 0)
	return (*this)[this->Frame().Origin()]; // Return 0.
      range.LCol()--;
      range.TRow()--;
      // Could speed this up by seperating out row accesses ?
      return (*this)[range.End()] - (*this)[range.TopRight()] - (*this)[range.BottomLeft()] + (*this)[range.TopLeft()];
    }
    //: Calculate the sum and sum of squares of the pixel values in the rectangle 'range'.
    
    RealT Sum1(IndexRange2dC range) const {
      range.ClipBy(clipRange);
      if(range.Area() == 0)
	return (*this)[this->Frame().Origin()][0]; // Return 0.
      range.LCol()--;
      range.TRow()--;
      // Could speed this up by seperating out row accesses ?
      return (*this)[range.End()][0] - (*this)[range.TopRight()][0] - (*this)[range.BottomLeft()][0] + (*this)[range.TopLeft()][0];
    }
    //: Calculate the sum of the pixel's in the rectangle 'range'.
    
    RealT Variance(IndexRange2dC range) const {
      TFVectorC<DataT,2> sum = Sum(range);
      RealT area = (RealT) range.Area();
      return ((RealT) sum[1] - Sqr((RealT) sum[0])/area) / (area-1);
    }
    //: Calculate variance of the image in 'range'.
    
    IntT MeanVariance(IndexRange2dC range,RealT &mean,RealT &var) const {
      TFVectorC<DataT,2> sum = Sum(range);
      IntT area = range.Area();
      mean = (RealT) sum[0] / area;
      var = ((RealT) sum[1] - Sqr((RealT) sum[0])/area) / (area-1);
      return area;
    }
    //: Calculate mean, variance and area of the image in 'range'.
    
    TFVectorC<DataT,2> VerticalDifference2(IndexRange2dC range,IntT mid) const {
      // Could speed this up by seperating out row accesses ?
      return (*this)[range.TopLeft()] 
	+ ((*this)[mid][range.RCol()] - (*this)[mid][range.LCol()]) * 2 
	- (*this)[range.BottomLeft()] 
	- (*this)[range.TopRight()]
	+ (*this)[range.End()];
    }
    //: Calculate the diffrence between two halfs of the rectangle split vertically.
    // This mid point is an absolute row location and should be within the rectangle.
    
    TFVectorC<DataT,2> HorizontalDifference2(IndexRange2dC range,IntT mid) const {
      // Could speed this up by seperating out row accesses ?
      return (*this)[range.TopLeft()]
	+ ((*this)[range.BRow()][mid] - (*this)[range.TRow()][mid]) * 2 
	- (*this)[range.BottomLeft()] 
	+ (*this)[range.TopRight()]
	- (*this)[range.End()];
    }
    //: Calculate the diffrence between two halfs of the rectangle split horizontally.
    // This mid point is an absolute column location and should be within the rectangle.

    TFVectorC<DataT,2> VerticalDifference3(const IndexRange2dC &range,const IndexRangeC &rng) const {
      RavlAssert(range.Range2().Contains(rng));
      IndexRange2dC rng2(range.Range1(),rng);
      return Sum(range) - Sum(rng2);
    }
    //: Calculate the diffrence between two halfs of the rectangle split vertially.
    // This mid point is an absolute row location and should be within the rectangle.
    
    TFVectorC<DataT,2> HorizontalDifference3(const IndexRange2dC &range,const IndexRangeC &rng) const {
      RavlAssert(range.Range1().Contains(rng));
      IndexRange2dC rng2(rng,range.Range2());
      return Sum(range) - Sum(rng2);
    }
    //: Calculate the diffrence between two rectangles one lying inside the other in the horizontal dimention.
    // This mid point is an absolute column location and should be within the rectangle.
    
    void SetClipRange(IndexRange2dC &nClipRange) 
    { clipRange = nClipRange; }
    //: Set the clip range.
    
    const IndexRange2dC &ClipRange() const
    { return clipRange; }
    //: Return range of value positions. 
    
  protected:
    IndexRange2dC clipRange;
  };
  
  template<class DataT>
  ostream &operator<<(ostream &strm,const SummedAreaTable2C<DataT> &data) {
    strm << static_cast<const Array2dC<TFVectorC<DataT,2> > &>(data);
    return strm;
  }
  //: Write to text stream.
  
  template<class DataT>
  istream &operator>>(istream &strm,SummedAreaTable2C<DataT> &data) {
    strm >> static_cast<Array2dC<TFVectorC<DataT,2> > &>(data);
    IndexRange2dC clipRange = data.Frame();
    clipRange.LCol()++;
    clipRange.TRow()++;
    data.SetClipRange(clipRange);
    return strm;
  }
  //: Read from text stream.

  template<class DataT>
  BinOStreamC &operator<<(BinOStreamC &strm,const SummedAreaTable2C<DataT> &data) {
    strm << static_cast<const Array2dC<TFVectorC<DataT,2> > &>(data);
    return strm;
  }
  //: Write to binary stream.
  
  template<class DataT>
  BinIStreamC &operator>>(BinIStreamC &strm,SummedAreaTable2C<DataT> &data) {
    strm >> static_cast<Array2dC<TFVectorC<DataT,2> > &>(data);
    IndexRange2dC clipRange = data.Frame();
    clipRange.LCol()++;
    clipRange.TRow()++;
    data.SetClipRange(clipRange);
    return strm;
  }
  //: Read from binary stream.
  
}

#endif
