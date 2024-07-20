// This file is part of RAVL, Recognition And Vision Library 
// Copyright (C) 2001, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here
//! rcsid="$Id$"
//! lib=RavlImageProc
//! author="Joel Mitchelson"
//! file="Ravl/Image/Processing/Segmentation/ChromaThreshold.cc"

#include "Ravl/Image/ChromaThreshold.hh"
#include "Ravl/Array2dIter2.hh"
#include "Ravl/Array2dIter3.hh"

namespace RavlImageN
{
  ChromaThresholdRGBC::ChromaThresholdRGBC(const ImageC<ByteRGBValueC>& image,
					   RealT tolerance /* = 1.0 */,
					   RealT nblack_thresh /* = 0.1 */,
					   ByteT nlabel_match /* = 255 */,
					   ByteT nlabel_no_match /* = 0 */,
					   ByteT nlabel_black /* = 0 */ ) :

    black_thresh(nblack_thresh),
    label_match(nlabel_match),
    label_no_match(nlabel_no_match),
    label_black(nlabel_black)
    
  {
    RealT r_mean = 0.0;
    RealT g_mean = 0.0;
    RealT b_mean = 0.0;
    RealT r_mean_sq = 0.0;
    RealT g_mean_sq = 0.0;
    RealT b_mean_sq = 0.0;
    UIntT n = 0;

    RavlAssert(image.Rows() > 0);
    RavlAssert(image.Cols() > 0);

    RealT scale_black_thresh = black_thresh * 3.0 * 255.0;

    for (UIntT y = 0; y < image.Rows(); y++)
    {
      for (UIntT x = 0; x < image.Cols(); x++)
      {
	RealT r = image[y][x].Red();
	RealT g = image[y][x].Green();
	RealT b = image[y][x].Blue();
	// RealT I = r + g + b;
	RealT I = 0.299*r + 0.587*g +  0.114*b;

	if (I < scale_black_thresh)
	  continue;

	RealT nr = r / I;
	RealT ng = g / I;
	RealT nb = b / I;
	r_mean += nr;
	g_mean += ng;
	b_mean += nb;
	r_mean_sq += nr*nr;
	g_mean_sq += ng*ng;
	b_mean_sq += nb*nb;	

	n++;
      }
    }

    // calculate means and mean of sqs
    r_mean /= n;
    g_mean /= n;
    b_mean /= n;
    r_mean_sq /= n;
    g_mean_sq /= n;
    b_mean_sq /= n;	

    // variances
    RealT var_r = (r_mean_sq - r_mean*r_mean);
    RealT var_g = (g_mean_sq - g_mean*g_mean);
    RealT var_b = (b_mean_sq - b_mean*b_mean);


    // clamp variance to avoid divide by zero
    if (var_r < 1E-12)
      var_r = 1E-12;
    if (var_g < 1E-12)
      var_g = 1E-12;
    if (var_b < 1E-12)
      var_b = 1E-12;

    // store means
    r0 = r_mean;
    g0 = g_mean;
    b0 = b_mean;

    // store weights
    rw = 1.0 / (3.0 * tolerance * var_r);
    gw = 1.0 / (3.0 * tolerance * var_g);
    bw = 1.0 / (3.0 * tolerance * var_b);

  }
  
  void ChromaThresholdRGBC::Apply(ImageC<ByteT>& result, const ImageC<ByteRGBValueC>& image) const {
    RavlAssert( result.Rows() == image.Rows() );
    RavlAssert( result.Cols() == image.Cols() );
    
    RealT scale_black_thresh = black_thresh * 3.0 * 255.0;
    
    for(Array2dIter2C<ByteT,ByteRGBValueC> it(result,image);it;it++) {
      const ByteRGBValueC& pixel = it.Data2();
      
      RealT r = pixel.Red();
      RealT g = pixel.Green();
      RealT b = pixel.Blue();
      RealT I = 0.299*r + 0.587*g +  0.114*b;
      // RealT I = r + g + b;
      
      if (I < scale_black_thresh) {
	it.Data1() = label_black;
	continue;
      }
      
      RealT nr = r / I;
      RealT ng = g / I;
      RealT nb = b / I;
      
      RealT C = rw*(nr-r0)*(nr-r0) + gw*(ng-g0)*(ng-g0) + bw*(nb-b0)*(nb-b0);
      it.Data1() = ((C < 1.0) ? label_match : label_no_match);
    }
  }
  
  void ChromaThresholdRGBC::Apply(ImageC<ByteT>& result, 
				  const ImageC<ByteRGBAValueC>& image) const
  {
    RavlAssert( result.Rows() == image.Rows() );
    RavlAssert( result.Cols() == image.Cols() );

    RealT scale_black_thresh = black_thresh * 3.0 * 255.0;

    for(Array2dIter2C<ByteT,ByteRGBAValueC> it(result,image);it;it++) {
      const ByteRGBAValueC& pixel = it.Data2();
      
      RealT r = pixel.Red();
      RealT g = pixel.Green();
      RealT b = pixel.Blue();
      RealT I = 0.299*r + 0.587*g +  0.114*b;
      // RealT I = r + g + b;
      
      if (I < scale_black_thresh) {
	it.Data1() = label_black;
	continue;
      }
      
      RealT nr = r / I;
      RealT ng = g / I;
      RealT nb = b / I;
      
      RealT C = rw*(nr-r0)*(nr-r0) + gw*(ng-g0)*(ng-g0) + bw*(nb-b0)*(nb-b0);
      ByteT label  = ((C < 1.0) ? label_match : label_no_match);
      it.Data1() = label;
    }
  }

  void ChromaThresholdRGBC::ApplyCopyAlpha(ImageC<ByteT>& result, 
					   const ImageC<ByteRGBAValueC>& image, 
					   ImageC<ByteRGBAValueC>& auximage) const
  {
    RavlAssert( result.Rows() == image.Rows() );
    RavlAssert( result.Cols() == image.Cols() );
    RavlAssert( auximage.Rows() == image.Rows() );
    RavlAssert( auximage.Cols() == image.Cols() );

    RealT scale_black_thresh = black_thresh * 3.0 * 255.0;

    for(Array2dIter3C<ByteT,ByteRGBAValueC,ByteRGBAValueC> it(result,image,auximage);it;it++) {
      const ByteRGBAValueC& pixel = it.Data2();
      RealT r = pixel.Red();
      RealT g = pixel.Green();
      RealT b = pixel.Blue();
      RealT I = 0.299*r + 0.587*g +  0.114*b;
      
      if (I < scale_black_thresh) {
	it.Data1() = label_black;
	it.Data3()[3] = label_black;
	continue;
      }
      
      RealT nr = r / I;
      RealT ng = g / I;
      RealT nb = b / I;
      
      RealT C = rw*(nr-r0)*(nr-r0) + gw*(ng-g0)*(ng-g0) + bw*(nb-b0)*(nb-b0);
      ByteT label  = ((C < 1.0) ? label_match : label_no_match);
      it.Data1() = label;
      it.Data3()[3] = label;
    }
  }
  
  ostream& operator<<(ostream& os, const ChromaThresholdRGBC& c) {
    os << c.r0 << " " << c.g0 << " " << c.b0 << endl;
    os << c.rw << " " << c.gw << " " << c.bw << endl;
    os << c.black_thresh << endl;
    os << (int)c.label_match << " " << (int)c.label_no_match << " " << (int)c.label_black << endl;
    return os;
  }

  istream& operator>>(istream& is, ChromaThresholdRGBC& c) {
    is >> c.r0 >> c.g0 >> c.b0;
    is >> c.rw >> c.gw >> c.bw;
    is >> c.black_thresh;
    
    int label_match, label_no_match, label_black;
    is >> label_match >> label_no_match >> label_black;

    c.label_match = (ByteT)label_match;
    c.label_no_match = (ByteT)label_no_match;
    c.label_black = (ByteT)label_black;

    return is;
  }

}
