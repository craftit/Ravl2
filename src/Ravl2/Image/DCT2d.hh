// Original Copyright:
//    This file forms a part of ImageLib, a C++ class library for image
//    processing.
//
//    Copyright (C) 1998-2003 Brendt Wohlberg  <brendt@dip1.ee.uct.ac.za>
//    This library is free software; you can redistribute it and/or
//    modify it under the terms of the GNU Library General Public
//    License as published by the Free Software Foundation; either
//    version 2 of the License, or (at your option) any later version.
//
//    This library is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//    Library General Public License for more details.
//
//    You should have received a copy of the GNU Library General Public
//    License along with this library; if not, write to the Free
//    Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//
// Modified by Charles Galambos.
//! author="Brendt Wohlberg, Modified by Charles Galambos"
//! license=own

#pragma once

#include "Ravl2/Array.hh"

namespace Ravl2
{
  class VectorC;

  //! Perform DCT on src, result in dest
  //! Note: the classes ChanDCTC or VecRadDCTC should be faster than this.
  //! Only works on square images.
  void DCT(const Array<float, 2> &src, Array<float, 2> &dest);

  //! Perform Inverse DCT on src, result in dest
  //! Only works on square images.
  void IDCT(const Array<float, 2> &src, Array<float, 2> &dest);

  //! Pack first n components of image 'img' in a zig zag pattern from the to left corner of 'img'.
  VectorC PackZigZag(const Array<float, 2> &img, unsigned n);

  //! Unpack components of image vec in a zig zag pattern from the to left corner of 'img'.
  void UnpackZigZag(const VectorC &vec, Array<float, 2> &img);

  //! Class implementing Fast DCT
  // class ChanDCT is an encapsulation of software (URL:
  // ftp://etro.vub.ac.be/pub/COMPRESSION/DCT_ALGORITHMS/) written by
  // Charilaos A. Christopoulos (Email:chchrist@etro.vub.ac.be), based on
  // the paper: <p>
  // S. C. Chan and K. L. Ho, "A new two-dimensional fast cosine transform
  // algorithm", IEEE Trans. on Signal Processing, Vol. 39, No. 2, pp. 481-485,
  // Feb. 1991.

  class ChanDCTC
  {
  public:
    using RealT = float;

    ChanDCTC();
    //: Default constructor.
    // 'Setup' must be called before DCT computation.

    ChanDCTC(unsigned int size);
    //: Construct DCT for image of 'size' rows by 'size' columns

    ~ChanDCTC();
    //: Destructor.

    void Setup(unsigned int size);
    //: Setup tables for dct of given size.
    //!param:size - Size of dct image.

    void dct_in_place(Array<RealT, 2> &im) const;
    //: Do an inplace dct of im.

    Array<RealT, 2> operator()(const Array<RealT, 2> &im) const
    {
      return DCT(im);
    }
    //: Compute the dct of im.

    Array<RealT, 2> DCT(const Array<RealT, 2> &im) const;
    //: Compute the DCT of im, return the result.

    void DCT(const Array<RealT, 2> &src, Array<RealT, 2> &dest) const;
    //: Compute the dct of 'src', place the result in 'dest'.

    auto Size() const
    {
      return N;
    }
    //: Access the size of a side of the dct rectangle.

  private:
    const ChanDCTC &operator=(const ChanDCTC &oth)
    {
      return *this;
    }
    //: Make assigment operator private.

    int N;
    int m;
    RealT *cosines;

    RealT scaleDC;
    RealT scaleMix;
    RealT scaleAC;

    void makecosinetable();
    void columnspostadditions(Array<RealT, 2> &fi) const;
    void rowspostadditions(Array<RealT, 2> &fi) const;
    void rowsbitreversal(Array<RealT, 2> &fi) const;
    void columnsbitreversal(Array<RealT, 2> &fi) const;
    void columnsinputmapping(Array<RealT, 2> &fi) const;
    void rowsinputmapping(Array<RealT, 2> &fi) const;
  };

  //! Class implementing Fast DCT
  // This class allows a subset of the DCT coefficents to be calculated.
  // NOTE: This class is NOT thread safe.
  //
  // class VecRadDCT is an encapsulation of software (URL:
  // ftp://etro.vub.ac.be/pub/COMPRESSION/DCT_ALGORITHMS/) written by
  // Charilaos A. Christopoulos (Email:chchrist@etro.vub.ac.be), based on
  // the papers: <p>
  //
  // C.A. Christopoulos, J. Bormans, A.N. Skodras and J. Cornelis, "Efficient
  // computation of the two-dimensional fast cosine transform", SPIE Hybrid Image
  // and Signal Processing IV, 5-8 April 1994, Orlando, Florida, USA. Accepted. <p>
  //
  // and <P>
  //
  // C.A. Christopoulos and A.N. Skodras, "Pruning the two-dimensional fast cosine
  // transform", To be presented at the VII European Signal Processing Conference ,
  // September 13-16, 1994, Edinburgh, Scotland.

  class VecRadDCTC
  {
  public:
    using RealT = float;

    VecRadDCTC(unsigned int size, unsigned int pts);
    //: Constructor.
    //!param: size - Size of input image. Must be a power of 2
    //!param: pts  - Size of output image .Must be a power of 2

    VecRadDCTC();
    //: Default constructor.
    // You must call Initalise to setup the transform before performing a dct transform.

    ~VecRadDCTC();
    //: Destructor.

    void Setup(unsigned int size, unsigned int pts);
    //: Initialise for image of 'size' by 'size'
    //!param: size - Size of input image. Must be a power of 2
    //!param: pts  - Size of output image .Must be a power of 2

    void dct_in_place(Array<RealT, 2> &im, bool modifyOutputRect = false) const;
    //: Compute the dct in place.
    //!param: im = Image to compute DCT on.
    //!param: modifyOutputRect = If true change image frame to be of the correct scale, otherwise result is left in top left of the original image.
    // Note the size of m will be reduced.

    Array<RealT, 2> operator()(const Array<RealT, 2> &im) const
    {
      return DCT(im);
    }

    Array<RealT, 2> DCT(const Array<RealT, 2> &im) const;
    //: Compute the DCT of img.

    void DCT(const Array<RealT, 2> &src, Array<RealT, 2> &dest) const;
    //: Compute the DCT of src, place the result in 'dest'.

    int Size() const
    {
      return N;
    }
    //: Access the size of a side of the input dct rectangle.

    int OutputSize() const
    {
      return N0;
    }
    //: Access the size of the output array.

  private:
    void DeleteArrays();
    //: Free all array's

    const VecRadDCTC &operator=(const VecRadDCTC &oth)
    {
      return *this;
    }
    //: Make assigment operator private.

    typedef RealT LFloatT;// Local definition of a float.

    int N;
    int N0;
    int m;
    LFloatT *ct;
    LFloatT *ct2d;
    unsigned int **r;

    LFloatT *cosine_array;
    unsigned int MASK[2];

    LFloatT scaleDC;
    LFloatT scaleMix;
    LFloatT scaleAC;

    void lut_of_cos();
    void expand1d_lookup_table();
    void make2Darray();
    void bitreversalrows() const;
    void bitreversalcolumns() const;
    void firo3(Array<RealT, 2> &fi) const;
    void post_adds(Array<RealT, 2> &fi) const;
  };

}// namespace Ravl2
