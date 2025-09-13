// This file is part of RAVL, Recognition And Vision Library
// Copyright (C) 2003, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here
#ifndef RAVLIMAGE_REGIONCLASIFYMOTION_HEADER
#define RAVLIMAGE_REGIONCLASIFYMOTION_HEADER 1
//////////////////////////////////////////////////////////////
//! docentry="Ravl.API.Images.Segmentation"
//! userlevel=Normal
//! file="Ravl/Image/Processing/Segmentation/PixelClassifyMotion.hh"
//! lib=RavlImageProc
//! example=exSegmentation.cc
//! rcsid="$Id$"
//! author="Fangxiang Cheng"
//! date="13/03/2000"

#include "Ravl/Vector.hh"
#include "Ravl/Index2d.hh"
#include "Ravl/Math.hh"

namespace RavlN
{

  //! userlevel=Normal
  //: Class to determine whether a pixel belongs to a region based on its motion vector.

  template <class ValT, class StatT>
  class PixelClassifyMotionC
  {
  public:
    PixelClassifyMotionC()
    {
      threshold = 0;
    }
    //: default constructor

    PixelClassifyMotionC(RealT thr)
    {
      threshold = thr;
    }
    //: Constructor

    inline void Initialise()
    {
      count = 0;
      mean.SetZero();
      total.SetZero();
    }
    //: Resets the statistics for the region

    inline bool Contains(const Index2dC &pxl, const ValT &val) const
    {
      return Sqrt(Distance(pxl, val)) <= threshold;
    }
    //: Return true if pxl belongs to the region

    inline RealT Distance(const Index2dC &pxl, const ValT &val) const
    {
      return mean.SqrEuclidDistance(val);
    }
    //: Distance to the mean

    inline void Include(const Index2dC &pxl, const ValT &val)
    {
      count++;
      total += val;
      mean = total / count;
    }
    //: Update the statistics of the region for the included pixel

    inline void Remove(const Index2dC &pxl, const ValT &val)
    {
      count--;
      total -= val;
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

    virtual bool Save(ostream &out) const
    {
      out << (*this);
      return true;
    }

  protected:
    int count;
    ValT mean, total;
    RealT threshold;
  };

  template <class ValT, class StatT>
  inline ostream &operator<<(ostream &s, const PixelClassifyMotionC<ValT, StatT> &out)
  {
    out.Save(s);
    return s;
  }
  //: output stream operator

  template <class ValT, class StatT>
  inline istream &operator>>(istream &s, PixelClassifyMotionC<ValT, StatT> &in)
  {
    RavlAssert(0);
    return s;
  }
  //: input stream operator
}// namespace RavlImageN
#endif
