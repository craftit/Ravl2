// This file is part of RAVL, Recognition And Vision Library
// Copyright (C) 2003, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here
#ifndef RAVLIMAGE_PIXELCLASSIFYCOLOUR_HEADER
#define RAVLIMAGE_PIXELCLASSIFYCOLOUR_HEADER 1
//////////////////////////////////////////////////////////////
//! docentry="Ravl.API.Images.Segmentation"
//! userlevel=Normal
//! example=exSegmentation.cc
//! file="Ravl/Image/Processing/Segmentation/PixelClassifyColour.hh"
//! rcsid="$Id$"
//! lib=RavlImageProc
//! author="Ratna Rambaruth"
//! date="22/07/1998"

#include "Ravl/Index2d.hh"

namespace RavlImageN
{
  using namespace RavlN;

  //! userlevel=Normal
  //: Class to determine whether a pixel belongs to a region based on its colour.
  // At the moment, the only statistic which can be calculated for the region is
  // the mean. The colour representation can only be 3-dimensional.
  // StatT can be any pixel representation class which has 3 components
  // ValT can be any pixel representation which can be used to construct an object of class StatT.

  template <class ValT, class StatT>
  class PixelClassifyColourC
  {
  public:
    PixelClassifyColourC()
    {
      threshold = StatT(20, 20, 20);
    }
    //: default constructor

    PixelClassifyColourC(IntT thr)
    {
      threshold = StatT(thr, thr, thr);
    }
    //: constructor

    PixelClassifyColourC(ValT thr)
    {
      threshold = StatT(thr);
    }
    //: constructor

    inline void Initialise()
    {
      mean = StatT(0, 0, 0), count = 0;
      total = StatT(0, 0, 0);
    }
    //: Resets the statistics for the region

    inline bool Contains(const Index2dC &pxl, const ValT &val) const
    {
      StatT dist = StatT(val) - mean;
      return (fabs(dist[0]) < threshold[0] && fabs(dist[1]) < threshold[1] && fabs(dist[2]) < threshold[2]);
    }
    //: Return true if pxl belongs to the region

    inline RealT Distance(const Index2dC &pxl, const ValT &val) const
    {
      return mean.SqrEuclidDistance(StatT(val));
    }
    //: Distance to the mean

    inline void Include(const Index2dC &pxl, const ValT &val)
    {
      count++;
      total += StatT(val);
      mean = total / count;
    }
    //: Update the statistics of the region for the included pixel

    inline void Remove(const Index2dC &pxl, const ValT &val)
    {
      count--;
      total -= StatT(val);
      mean = total / count;
    }
    //: Update the statistics of the region for the stolen pixel

    inline IntT Counter()
    {
      return count;
    }
    //:the number of the member pixels within this cluster

    inline StatT Stat()
    {
      return mean;
    }
    //: Return the statistics about the region

    bool Save(ostream &out) const
    {
      out << count << " " << total << " " << mean << " " << threshold;
      return true;
    }
    //: Save to standard stream

  protected:
    int count;
    StatT total, mean, threshold;
  };

  template <class ValT, class StatT>
  inline ostream &operator<<(ostream &s, const PixelClassifyColourC<ValT, StatT> &out)
  {
    out.Save(s);
    return s;
  }
  //: output stream operator

  template <class ValT, class StatT>
  inline istream &operator>>(istream &s, PixelClassifyColourC<ValT, StatT> &in)
  {
    RavlAssert(0);
    return s;
  }
  //: input stream operator
}// namespace RavlImageN
#endif
