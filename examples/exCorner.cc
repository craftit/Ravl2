// This file is part of RAVL, Recognition And Vision Library 
// Copyright (C) 2002, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here
//! rcsid="$Id$"
//! lib=RavlImageProc
//! file="Ravl/Image/Processing/Corners/exCorner.cc"
//! docentry="Ravl.API.Images.Corner Detection"
//! author="Charles Galambos"
//! userlevel=Normal

//#include "Ravl2/Image/CornerDetectorHarris.hh"
#include "Ravl2/Image/CornerDetectorSusan.hh"
#include "Ravl2/Image/DrawFrame.hh"


int main(int nargs,char **argv)
{
  using namespace Ravl2;

  OptionC opt(nargs,argv);
  int threshold = opt.Int("t",100,"Threshold. ");
  int w = opt.Int("w",3,"width of filter mask. ");
  bool useHarris = opt.Boolean("h",false,"Use harris corner detector, else use susan. ");
  bool useTopHat = opt.Boolean("th",true,"Use top hat filter in harris corner detector. ");
  bool seq = opt.Boolean("seq",false,"Process a sequence. ");
  bool verb = opt.Boolean("v",false,"Verbose mode. ");
  bool deinterlace = opt.Boolean("d",false,"Deinterlace images");
  
  UIntT frameLimit = opt.Int("fl",-1,"Limit on the number of frames to process in a sequence. ");
  StringC inf = opt.String("","test.ppm","Input image. ");
  StringC outf = opt.String("","out.ppm","Output image. ");
  opt.Check();
  
  // Setup corner detector.
  
  CornerDetectorC cornerDet;
  if(useHarris)
    cornerDet = CornerDetectorHarrisC(threshold,w,useTopHat);
  else
    cornerDet = CornerDetectorSusanC(threshold);
  
  if(!seq) {
    // Process a signle image
    
    ImageC<ByteT> img;
    if(!RavlN::Load(inf,img)) { // Load an image.
      cerr << "Failed to load image '" << inf << "'\n";
      return 1;
    }
    
    // Find the corners.
    
    auto corners = cornerDet.Apply(img);
    
    // Draw boxes around the corners.
    
    ByteT val = 255;
    for(DLIterC<CornerC> it(corners);it;it++) {
      IndexRange2dC rect(it->Location(),5,5);
      DrawFrame(img,val,rect);
    }
    
    // Save image to a file.
    
    if(!RavlN::Save(outf,img)) {
      cerr << "Failed to save image '" << inf << "'\n";
      return 1;
    }
  } else {
    // Process a sequence
    
    DPIPortC<ImageC<ByteT> > imgIn;
    if(!OpenISequence(imgIn,inf,"",verb)) {
      cerr << "Failed to open input '" << inf << "' \n";
      return 1;
    }
    
    DPOPortC<ImageC<ByteT> > imgOut;
    if(!outf.IsEmpty()) { // If there's no output specificied do nothing, (Good for profiling.)
      if(!OpenOSequence(imgOut,outf,"",verb)) {
        cerr << "Failed to open input '" << outf << "' \n";
        return 1;
      }
    }
    
    ImageC<ByteT> img;
    while(imgIn.Get(img) && frameLimit-- != 0) {
      if(deinterlace)
        img = DeinterlaceSubsample(img);
      
      // Find the corners.
      
      auto corners = cornerDet.Apply(img);
      
      // Draw boxes around the corners.
      if(imgOut.IsValid()) {
        uint8_t val = 255;
        for(auto it : corners) {
          IndexRange<2> rect(it.Location(),5);
          DrawFrame(img,val,rect);
        }
        
        // Write image out.
        
        imgOut.Put(img);
      }
    }
  }
  return 0;
}
