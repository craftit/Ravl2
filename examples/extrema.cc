// This file is part of RAVL, Recognition And Vision Library 
// Copyright (C) 2002, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here
//! rcsid="$Id$"
//! lib=RavlImageProc
//! author="Charles Galambos"
//! docentry="Ravl.API.Images.Segmentation"
//! userlevel=Normal
//! file="Ravl/Image/Processing/Segmentation/extrema.cc"

#include "Ravl2/Image/SegmentExtrema.hh"
#include "Ravl/Option.hh"
#include "Ravl/Image/Image.hh"
#include "Ravl/IO.hh"
#include "Ravl/DP/SequenceIO.hh"
#include "Ravl/OS/Date.hh"

using namespace RavlImageN;

int main(int nargs,char **argv) {
  OptionC opt(nargs,argv);
  IntT minSize = opt.Int("ms",10,"Minimum region size. ");
  RealT minMargin = opt.Real("mm",10,"Minimum margin. ");
  IntT limit = opt.Int("l",255,"Limit on difference to consider.");
  bool seq = opt.Boolean("s",false,"Process a sequence. ");
  bool drawResults = opt.Boolean("d",false,"Draw results into a window.");
  bool drawBlack = opt.Boolean("db",false,"Draw results into a black background.");
  bool invert = opt.Boolean("inv",false,"Invert image before processing. ");
  bool verbose = opt.Boolean("v",false,"Verbose mode. ");
  bool useMasks = opt.Boolean("m",false,"Use masks. ");
  IntT trim = opt.Int("t",0,"Trim the image being processed. ");
  StringC displayImage = opt.String("di","@X","Display image destination. ");
  StringC fn = opt.String("","test.pgm","Input image. ");
  StringC ofn = opt.String("","","Output boundaries. ");
  opt.Check();

  
  ImageC<ByteT> img;
  
  SegmentExtremaC<ByteT> lst(minSize,minMargin,limit);
  
  // Open image source.
  DPIPortC<ImageC<ByteT> > inp;
  if(!OpenISequence(inp,fn)) {
    cerr <<  "Failed to open input sequence " << fn << "\n";
    return 1;
  }
  
  DPOPortC<DListC<BoundaryC> > outp;
  if(!ofn.IsEmpty()) {
    if(!OpenOSequence(outp,ofn)) {
      cerr <<  "Failed to open output sequence " << ofn << "\n";
      return 1;
    }
  }

  IndexRange2dSetC trimSet;
  trimSet = trimSet.Add(IndexRange2dC(trim,trim));
  
  ImageC<ByteT> pimg;
  int numberOfFrames = 0;
  RealT totalTime = 0;
  while(inp.Get(img)) {
    if(invert) {
      if(pimg.IsEmpty())
	pimg = ImageC<ByteT>(img.Frame());
      for(Array2dIter2C<ByteT,ByteT> it(pimg,img);it;it++)
	it.Data1() = 255 - it.Data2();
    } else
      pimg = img;
    //RavlN::Save("@X",pimg);
    if(useMasks) {
      DListC<ImageC<IntT> > masks = lst.ApplyMask(pimg);
      numberOfFrames++;
    } else {
      DListC<BoundaryC> bounds;
      DateC start = DateC::NowUTC();
      if(trim > 0)
	bounds = lst.Apply(pimg,trimSet);
      else
	bounds = lst.Apply(pimg);
#if 0
      for(DLIterC<BoundaryC> it(bounds);it;it++)
        it->OrderEdges();
#endif
      DateC end = DateC::NowUTC();
      totalTime += (end-start).Double();
      numberOfFrames++;
      if(verbose)
        cerr << "Regions=" << bounds.Size() << "\n";
      if(drawResults) {
        ImageC<ByteT> res;
        if(!drawBlack)
          res = ImageC<ByteT>(img.Copy());
        else {
          res = ImageC<ByteT>(img.Frame());
          res.Fill(0);
        }
        
        // Draw boundaries into image and display.
        for(DLIterC<BoundaryC> bit(bounds);bit;bit++)
          for(DLIterC<CrackC> it(*bit);it;it++)
            res[it->LPixel()] = 255;
        Save(displayImage,res);
      }
      if(outp.IsValid()) {
        if(!outp.Put(bounds)) {
          cerr << "ABORT: Failed to write output. \n";
          return 1;
        }
      }
    }
    if(!seq)
      break;
  }
  cerr << "Frames a second " << numberOfFrames/totalTime << "\n";
  cerr << "Pixels a second " << (img.Frame().Area() * numberOfFrames)/totalTime << "\n";
  return 0;
}
