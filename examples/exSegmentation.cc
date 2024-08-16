// This file is part of RAVL, Recognition And Vision Library 
// Copyright (C) 2003, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here

#include "Ravl2/Array.hh"

#include "Ravl/Image/RegionSet.hh"
#include "Ravl/Image/RegionGrow.hh"
#include "Ravl/Image/RegionGrowSteal.hh"
#include "Ravl/Image/PixelClassifyColour.hh"
#include "Ravl/Image/PixelSelectStack.hh"
#include "Ravl/Image/RealRGBValue.hh"

#include "Ravl/DP/FileFormatIO.hh"
#include "Ravl/Option.hh"

using namespace RavlN;
using namespace RavlImageN;

int main (int argc, char** argv)
{
  OptionC cmd(argc,argv);
  StringC file_in  = cmd.String("in", "in.pgm", "Original image");  
  StringC file_out = cmd.String("out", "out.pgm", "Segmentation map");
  IntT thresh = cmd.Int("t",35,"Colour threshold .");
  bool steal = cmd.Boolean("s",false,"Using segmentation with pixel stealing.");
  cmd.Check();
  
  ImageC<ByteRGBValueC> image;
  if(!RavlN::Load(file_in, image)) {
    cerr << "Failed to load " << file_in << "\n";
    return 1;
  }
  RegionSetC<RealRGBValueC> regions;
  
  PixelClassifyColourC<ByteRGBValueC, RealRGBValueC> classifier(ByteRGBValueC(thresh,thresh,thresh));

  if(!steal) {
    RegionGrowC<PixelSelectStackC, PixelClassifyColourC<ByteRGBValueC, RealRGBValueC>, ByteRGBValueC, RealRGBValueC > reg_grower(classifier);
    regions = reg_grower.Apply(image);
  } else {
    RegionGrowStealC<PixelSelectStackC, PixelClassifyColourC<ByteRGBValueC, RealRGBValueC>, ByteRGBValueC, RealRGBValueC > reg_grower(classifier);
    regions = reg_grower.Apply(image);
  }
  
  if(!RavlN::Save(file_out, regions.RandomImage())) {
    cerr << "Failed to save " << file_out << "\n";
  }
  
  return 0;
}
