// This file is part of RAVL, Recognition And Vision Library 
// Copyright (C) 2003, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here
#ifndef RAVLIMAGE_FLOODREGION_HEADER
#define RAVLIMAGE_FLOODREGION_HEADER 1
//! rcsid="$Id$"
//! lib=RavlImageProc
//! author="Charles Galambos, based on code by Jiri Matas."
//! docentry="Ravl.API.Images.Segmentation"
//! userlevel=Normal
//! file="Ravl/Image/Processing/Segmentation/FloodRegion.hh"

#include "Ravl2/Array.hh"
#include "Ravl2/Image/DrawFrame.hh"
//#include "Ravl2/Array2dIter2.hh"
//#include "Ravl2/Array1d.hh"
//#include "Ravl2/BlkQueue.hh"
//#include "Ravl2/Boundary.hh"

namespace Ravl2
{
  
  //! userlevel=Develop
  //: Scan line info
  
  class FloodRegionLineC {
  public:
    FloodRegionLineC(Index<2> nstart,int colEnd,int ndr)
      : start(nstart),
        end(colEnd),
        dr(ndr)
    {}
    //: Constructor.

    FloodRegionLineC(int row,int col,int colEnd,int ndr)
      : start(row,col),
        end(colEnd),
        dr(ndr)
    {}
    //: Constructor.
    
    const Index<2> &Start() const
    { return start; }
    //: Access start location
    
    int End() const
    { return end;}
    //: Get end of line.

    int DR() const
    { return dr; }
    //: Row direction.
    
  protected:
    Index<2> start;
    int end;
    int dr;
  };
  
  //! userlevel=Normal
  //: Threshold comparison class.
  
  template<class PixelT>
  class FloorRegionThresholdC {
  public:
    FloorRegionThresholdC()
    {}
    //: Default constructor.
    
    FloorRegionThresholdC(const PixelT &pix)
      : value(pix)
    {}
    //: Construct from pixel value.
    
    bool operator()(const PixelT &pix) const
    { return pix <= value; }
    //: Should pixel be included in the region ?
    
  protected:
    PixelT value;
  };
  
  //! userlevel=Normal
  //: Flood based region growing.
  // Grow a region from 'seed' including all connected pixel less than or equal to threshold, generate a boundary as the result.
  
  template<class PixelT,class InclusionTestT = FloorRegionThresholdC<PixelT> >
  class FloodRegionC {
  public:
    FloodRegionC()
      : pixQueue(1024)
    {}
    //: Default constructor.
    
    FloodRegionC(const Array<PixelT,2> &nimg)
      : pixQueue(1024)
    { SetupImage(nimg); }
    //: Constructor with image to segment.
    
    const Array<PixelT,2> &Image() const
    { return img; }
    //: Access the image we're currently segmenting.
    
    bool SetupImage(const Array<PixelT,2> &nimg) {
      img = nimg;
      IndexRange<2> rng = img.Frame().Expand(1);
      if(!marki.range().contains(rng)) {
	marki = Array<int,2>(rng);
	marki.fill(0);
	id = 1;
      }
      return true;
    }
    //: Setup new image for processing.
    
    bool GrowRegion(const Index<2> &seed,const InclusionTestT &inclusionCriteria,BoundaryC &boundary,int maxSize = 0);
    //: Grow a region from 'seed' including all connected pixel less than or equal to threshold, generate a mask as the result.
    // Returns true if the boundry has a non zero area.
    
    template<typename MaskT>
    int GrowRegion(const Index<2> &seed,const InclusionTestT &inclusionCriteria,ImageC<MaskT> &mask,int padding = 0,int maxSize = 0);
    //: Grow a region from 'seed' including all connected pixel less than or equal to threshold, generate a mask as the result.
    // The mask images are generated with a boundary
    // Returns the region size.
    
    Array<int,2> &MarkImage()
    { return marki; }
    //: Access marked pixel image.
    
    int MarkId() const
    { return id; }
    //: Access current region id.
    
  protected:
    bool BaseGrowRegion(const Index<2> &seed,const InclusionTestT &inclusionCriteria,IndexRange<2> &rng);
    //: Base grow region routine.
    
    Array<PixelT,2> img;
    Array<int,2> marki;
    int id;
    InclusionTestT inclusionTest;
    BlkQueueC<FloodRegionLineC > pixQueue;    
  };
  
  
  //: Base grow region routine.
  // A rewrite of code from: A Seed Fill Algorithm by Paul Heckbert from "Grahics Gems", Academic Press, 1990
  
  template<class PixelT,class InclusionTestT>
  bool FloodRegionC<PixelT,InclusionTestT>::BaseGrowRegion(const Index<2> &seed,const InclusionTestT &inclusionCriteria,IndexRange<2> &rng) {
    inclusionTest = inclusionCriteria;
    pixQueue.Empty();
    // Check seed.
    RavlAssert(img.Frame().Contains(seed));
    if(!inclusionTest(img[seed]))
      return false; // Empty region.
    
    // Setup stack.
    
    const auto &imgCols = img.Range2();
    const auto &imgRows = img.Range1();
    
    if(img.Frame().Range1().Contains(seed.Row()+1))
      pixQueue.InsFirst(FloodRegionLineC(seed.Row()+1,seed.Col(),seed.Col(),1));
    pixQueue.InsFirst(FloodRegionLineC(seed.Row(),seed.Col(),seed.Col(),-1));
    
    // Misc bits a pieces.
    rng = IndexRange<2>(seed,seed);
    id++;
    
    // Lookout for id wrap around.
    if(id == 0) {
      marki.Fill(0); 
      id++;
    }
    
    int l = 0;
    while(!pixQueue.IsEmpty()) {
      // Pop next line to examine off the stack.
      FloodRegionLineC line = pixQueue.GetFirst();
      const int &lsc = line.Start().Col();
      const int &lsr = line.Start().Row();
      //cerr << "Line=" << lsr << " " << lsc << " " << line.End() << " DR=" << line.DR() << "\n";
      // Involve scan line in rec.
      rng.Range1().Involve(lsr);
      
      // Do some prep for fast row access.
      const RangeBufferAccessC<PixelT> irow = img[lsr];
      RangeBufferAccessC<int> mrow = marki[lsr];
      
      // segment of scan line lsr - line.DY() for lsc <= c <= line.End() was previously filled,
      // now explore adjacent pixels in scan line lsr
      int c;
      for (c = lsc; c >= imgCols.Min() && (mrow[c] != id) && inclusionTest(irow[c]); c--) 
        mrow[c] = id;
      
      if (c >= lsc)
        goto skip;
      
      rng.Range2().Involve(c);
      l = c+1;
      if (l < lsc) { // Leak on the left ?
        int newRow = lsr-line.DR();
        if(imgRows.Contains(newRow))
          pixQueue.InsFirst(FloodRegionLineC(newRow,l,lsc-1,-line.DR()));
      }
      
      c = lsc+1;
      
      do {
        for (; c <= imgCols.Max() && (mrow[c] != id) && inclusionTest(irow[c]); c++)
          mrow[c] = id;
        rng.Range2().Involve(c);
        
        {
          int newRow = lsr+line.DR();
          if(imgRows.Contains(newRow))
            pixQueue.InsFirst(FloodRegionLineC(newRow,l,c-1,line.DR()));
        }
        
        if (c > (line.End() + 1)) { // Leak on the right ?
          int newRow = lsr-line.DR();
          if(imgRows.Contains(newRow))
            pixQueue.InsFirst(FloodRegionLineC(newRow,(line.End() + 1),c-1,-line.DR()));
        }
        
      skip:    
        for (c++; c <= line.End() && ((mrow[c] == id) || !inclusionTest(irow[c])); c++);
        l = c;
      } while (c <= line.End());
    }
    return true;
  }
  

  template<class PixelT,class InclusionTestT>
  bool FloodRegionC<PixelT,InclusionTestT>::GrowRegion(const Index<2> &seed,const InclusionTestT &inclusionCriteria,BoundaryC &boundary,int maxSize) {
    IndexRange<2> rng;
    if(!BaseGrowRegion(seed,inclusionCriteria,rng) || rng.Area() <= 0) {
      boundary = BoundaryC();
      return false;
    }
    rng = rng.Expand(1);
    rng.ClipBy(marki.Frame());
    boundary = BoundaryC(Array<int,2>(marki,rng),id);
    return true;
  }
  
  
  template<class PixelT,class InclusionTestT>
  template<typename MaskT>
  int FloodRegionC<PixelT,InclusionTestT>::GrowRegion(const Index<2> &seed,const InclusionTestT &inclusionCriteria,ImageC<MaskT> &mask,int padding,int maxSize) {
    IndexRange<2> rng;
    
    if(!BaseGrowRegion(seed,inclusionCriteria,rng)) {
      mask = ImageC<MaskT>();
      return 0;
    }
    
    // Extract region.
    mask = ImageC<MaskT>(rng.Expand(padding));
    if(padding > 0)
      DrawFrame(mask,(MaskT) 0,padding,mask.Frame());
    int size = 0;
    for(Array2dIter2C<MaskT,int> it(mask,marki,rng);it;it++) {
      if(it.Data2() == id) {
        size++;
        it.Data1() = 1;
      } else
        it.Data1() = 0;
    }
    return size;
  }
    
}

#endif
