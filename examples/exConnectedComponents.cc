// This file is part of RAVL, Recognition And Vision Library 
// Copyright (C) 2003, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here
///////////////////////////////////////////////////////
//! lib=RavlImageProc
//! file="Ravl/Image/Processing/Segmentation/exConnectedComponents.cc"
//! author="Lee Gregory"
//! docentry="Ravl.API.Images.Segmentation"
//! rcsid="$Id$

#include "Ravl/Image/ConnectedComponents.hh"
#include "Ravl/Option.hh" 
#include "Ravl/IO.hh"

using namespace RavlN ; 
using namespace RavlImageN ; 

int main (int argc, char ** argv) 
{

  // get some options 
  OptionC opts ( argc,argv) ; 
  StringC inputFile = opts.String("i", "in.pgm", "The input image") ; 
  StringC outputFile = opts.String("o", "out.pgm", "The labeled output image") ; 
  opts.Check() ; 
  
  // specify our pixel type
  typedef ByteT PixelT ; 
  
  // make the analysis classes ; 
  ConnectedComponentsC<PixelT> connected ; 
  
  // Load the image 
   ImageC<PixelT> img ; 
  if ( ! RavlN::Load ( inputFile, img ) ) { cerr << "\n Error failed to load input image " ; exit (1) ; }   
  
  // Apply the algorithm 
  Tuple2C<ImageC<UIntT>, UIntT> result = connected.Apply(img) ; 

  // output the result 
  if ( ! RavlN::Save ( outputFile, result.Data1() )) { cerr << "\n Error failed to load output image " ; exit (1) ; } 
  cout << "\n\n The segmented image contains " << result.Data2() << " labels \n\n"  ;  
  return 0 ; 
}; 
