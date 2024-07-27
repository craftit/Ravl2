// This file is part of RAVL, Recognition And Vision Library 
// Copyright (C) 2001, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here
////////////////////////////////////////////////////////////////////////////
//! author="Radek Marik, modified by Charles Galambos"
//! date="17/10/2000"
//! example=exConnectedComponents.cc 

#pragma once

#include "Ravl2/Array.hh"
//#include "Ravl2/DP/Process.hh"
//#include "Ravl2/Array2dIter2.hh"
//#include "Ravl2/RCHash.hh"
//#include "Ravl2/HashIter.hh"
//#include <tuple>
//#include <vector>
//#include "Ravl2/Array2dSqr2Iter.hh"
//#include "Ravl2/Array2dSqr2Iter2.hh"

namespace Ravl2
{

  //: Connected component labelling base class
  
  class ConnectedComponentsBaseBodyC {
  public:
    ConnectedComponentsBaseBodyC(unsigned nmaxLabel=1000, bool ignore=false)
      : maxLabel(nmaxLabel), 
	ignoreZero(ignore)
    {}
    //: Constructor.
    
    static unsigned RelabelTable(std::vector<unsigned> &labelTable, unsigned currentMaxLabel);
    
  protected:
    size_t maxLabel;
    bool ignoreZero;
  };


  //: Default <code>CompareT</code> comparison operator for connected components
  
  template <class DataTypeT>
  class ConnectedComponentsCompareC
  {
  public:
    ConnectedComponentsCompareC()
    {}
    //: Default constructor.
    
    inline bool operator()(const DataTypeT &v1,const DataTypeT &v2)
    { return v1 == v2; }
  };


  //: Connected component labelling. 
  
  template <class DataTypeT,class CompareT = ConnectedComponentsCompareC<DataTypeT> >
  class ConnectedComponentsBodyC
    : public ConnectedComponentsBaseBodyC
  {
  public:
    ConnectedComponentsBodyC (unsigned nmaxLabel = 10000, bool ignoreZero=false,const CompareT &compMethod = CompareT())
      : ConnectedComponentsBaseBodyC(nmaxLabel, ignoreZero),
	compare(compMethod)
    { SetZero(zero); }
    //: Constructor
    // (See handle class ConnectedComponentsC)

    ConnectedComponentsBodyC (unsigned nmaxLabel,const CompareT &compMethod,const DataTypeT &zeroValue)
      : ConnectedComponentsBaseBodyC(nmaxLabel, true),
	compare(compMethod),
	zero(zeroValue)
    {}
    //: Constructor
    // (See handle class ConnectedComponentsC)
    
    std::tuple<Array<unsigned,2>,unsigned> Apply (const Array<DataTypeT,2> &im);
    //: Performs the connected component labelling
    
  protected:
    CompareT compare;
    DataTypeT zero; // Zero value to use.
  };

  //: Connected component labelling. 
  // <p>This class identifies each connected region in an image and labels it
  // with a unique integer.  The label set is contiguous, starting from 1.</p>
  //
  // <p>The criterion for connectedness will vary with the pixel type - it can
  // be over-ridden by the <code><a href=
  // "RavlImageN.ConnectedComponentsCompareC.html">
  // ConnectedComponentsCompareC </a> compMethod</code> argument.  The default
  // comparison operator is the "==" operator for the pixel type.  So for
  // example, if the input image is the result of a binary segmentation, it
  // will look for a connected region of identical pixel values.</p>
  
  template <class DataTypeT,typename CompareT = ConnectedComponentsCompareC<DataTypeT> >
  class ConnectedComponentsC
    : public RCHandleC<ConnectedComponentsBodyC<DataTypeT,CompareT> >
  {
  public:
    ConnectedComponentsC (bool ignoreZero=false,const CompareT &compMethod = CompareT())
      : RCHandleC<ConnectedComponentsBodyC<DataTypeT> >(*new ConnectedComponentsBodyC<DataTypeT>(10000, ignoreZero,compMethod))
    {}
    //: Constructor used either to ignore pixels whose value is given by SetZero() or to not ignore any pixels at all.
    //!param: ignoreZero - if true, ignores the zeros on the input image (as defined by the SetZero() function for the pixel type "DataTypeT")
    //!param: compMethod - valid comparison operator for pixel type "DataTypeT"
    
    ConnectedComponentsC (const CompareT &compMethod,const DataTypeT &ignoreValue)
      : RCHandleC<ConnectedComponentsBodyC<DataTypeT> >(*new ConnectedComponentsBodyC<DataTypeT>(10000,compMethod,ignoreValue))
    {}
    //: Constructor used to ignore pixels of given value
    //!param: compMethod - valid comparison operator for pixel type "DataTypeT"
    //!param: ignoreValue - value of pixels not to be included in labelling

    
    std::tuple<Array<unsigned,2>,unsigned> Apply (const Array<DataTypeT,2> &im)
    { return Body().apply(im); }
    //: Performs the connected component labelling
    // The returned std::tuple object can be used directly to construct a SegmentationC object. 

  protected:
    ConnectedComponentsBodyC<DataTypeT,CompareT> &Body()
    { return RCHandleC<ConnectedComponentsBodyC<DataTypeT,CompareT> >::Body(); }
    //: Access body
    
    const ConnectedComponentsBodyC<DataTypeT,CompareT> &Body() const
    { return RCHandleC<ConnectedComponentsBodyC<DataTypeT,CompareT> >::Body(); }
    //: Access body
    
  };
  
    
  
  /////////////////////////////////////////////////////////////////////////
  // Implementation:
  
  template <class DataTypeT,class CompareT >
  std::tuple<Array<unsigned,2>,unsigned> ConnectedComponentsBodyC<DataTypeT,CompareT>::Apply (const Array<DataTypeT,2> &ip) { 
    std::vector<unsigned> labelTable(maxLabel+1);
    //labelTable.fill(-1);
    // If there are two labels for the same component, the bigger label bin
    // contains the value of the smaller label.
    Array<unsigned,2> jp(ip.range());
    
    // Label the first row.
    labelTable[0] = 0; // usually a special label
    unsigned lab = 1; // the last used label 
    {
      Array2dIter2C<DataTypeT,unsigned> it1(ip,jp);
      if(!it1.valid())
	return std::tuple<Array<unsigned,2>,unsigned>(jp,0); // Zero size image.
      it1.data<1>() = lab; // Label first pixel in the image.
      labelTable[lab] = lab; // Setup first label.
      DataTypeT lastValue = it1.data<0>();
      for (;it1.next();) { // Only iterate through the first row.
	if(compare(it1.data<0>(),lastValue)) 
	  it1.data<1>() = lab;
	else { // a new component
	  lab++;
	  RavlAssert(lab < maxLabel);
	  it1.data<1>() = lab;
	  labelTable[lab] = lab;
	}
	lastValue = it1.data<0>();
      }
    }
    
    for(Array2dSqr2Iter2C<DataTypeT,unsigned> it(ip,jp);it;) {
      // Label the first column pixel.
      if (compare(it.DataBL1(),it.DataTL1()))
	it.DataBL2() = it.DataTL2();
      else {
	lab++;
	RavlAssert(lab < maxLabel);
	it.DataBL2() = lab;
	labelTable[lab] = lab;
      }
      // DataU() = jp[ix-1][iy]
      // DataB() = jp[ix][iy-1]
      
      do { // The rest of the image row.
	if (compare(it.DataBR1(),it.DataTR1())) { 
	  // The upper pixel belongs to the same component.
	  if (!(compare(it.DataBR1(),it.DataBL1()))) {
	    // The processed pixel belongs to the upper component.
	    it.DataBR2() = it.DataTR2();
	    continue;
	  }
	  // The left pixel belongs to the same component.
	  // All pixels belong to the old created component.
	  it.DataBR2() = it.DataBL2(); 
	  unsigned upperRoot = labelTable[it.DataTR2()];
	  unsigned leftRoot = labelTable[it.DataBL2()];
	  if (upperRoot == leftRoot) {
	    // The processed pixel belongs to the upper component.
	    continue; // Same label, just get on with it.
	  }
	  
	  // There are two root labels for the same component.
	  // Find the root labels.
	  for (; upperRoot != labelTable[upperRoot];)
	    upperRoot  = labelTable[upperRoot];
	  for (; leftRoot != labelTable[leftRoot];)
	    leftRoot  = labelTable[leftRoot];
	  
	  // Join both tree of labels.
	  unsigned newRoot = (upperRoot >= leftRoot) ? leftRoot : upperRoot;
	  labelTable[upperRoot] = newRoot;
	  labelTable[leftRoot]  = newRoot;
	  
	  // Relabel both branches in the trees.
	  unsigned il = it.DataTR2();
	  for (unsigned hl = labelTable[il];il != newRoot;hl = labelTable[il = hl])
	    labelTable[il] = newRoot;
	  il = it.DataBL2();
	  for (unsigned hhl = labelTable[il];il != newRoot;hhl = labelTable[il = hhl])
	    labelTable[il] = newRoot;
	  continue;
	}
	// The upper pixel belongs to the different component.
	if (compare(it.DataBR1(),it.DataBL1())) { // The left pixel belongs to the same component.
	  // The processed pixel belongs to the left component.
	  it.DataBR2() = it.DataBL2();
	  continue;
	}
	// The left pixel belongs to the different component.
	// The processed pixel belongs to the new component.
	lab++;
	RavlAssert(lab < maxLabel);
	it.DataBR2() = lab; 
	labelTable[lab] = lab;
	// +2 according to the first column pixel
	if (lab + 2 > maxLabel)  { // Try condensate the 'labelTable'.
	  unsigned newLastLabel = RelabelTable(labelTable,lab);
	  Index<2> iat = it.Index();
	  // Change labels in the processed part of the image.
	  ImageRectangleC subRect(jp.range());
	  int ix = iat[0];
	  int iy = iat[1];
	  subRect.max(0) = ix - 1;
	  for(Array2dIterC<unsigned> its(jp,subRect);its;its++)
	    *its = labelTable[*its];
	  
	  // Change labels in the processed part of the row.
	  for (int jy = ip.range().min()[1]; jy <= iy; ++jy)
	    jp[ix][jy] = labelTable[jp[ix][jy]];
	  
	  // Create the correct label scheme.
	  if (((float) newLastLabel * 1.5) > maxLabel) { 
	    //cerr << "Resizing label table. Max:" << maxLabel << "\n";
	    // The size of the 'labelTable' is too small.
	    maxLabel *= 2; // Double the size of the table.
	    labelTable = std::vector<unsigned>(maxLabel+1);
	    //labelTable.fill(((unsigned) -1)); // Debug measure...
	  }
	  // Create an identity label set.
	  unsigned ll = 0;
	  for(SArray1dIterC<unsigned> itx(labelTable,newLastLabel+1);itx;itx++,ll++)
	    *itx = ll;
	  lab = newLastLabel;
	}
      } while(it.next());
    }
    
    // Relabel the whole image
    if (lab == 0)
      return std::tuple<Array<unsigned,2>,unsigned>(jp,lab);
    
    // newLastLabel =
    RelabelTable(labelTable,lab);
    // Change labels in the have been processed area
    for(Array2dIterC<unsigned> it(jp);it;it++) 
      *it = labelTable[*it];  
    
    // The user may of requested to ignore the empty pixels (eg. zeros) in the original image
    if(ignoreZero) {
      std::vector<IntT> arr(lab+2);
      arr.fill(-1);
      unsigned curr = 1;
      for(Array2dIter2C<DataTypeT, unsigned> it(ip, jp);it;it++) {
	if(it.data<0>()==zero) 
	  it.data<1>() = 0;
	else {
	  if(arr[it.data<1>()]==-1) {
	    arr[it.data<1>()] = curr;
	    curr++;
	  }
	  it.data<1>() = arr[it.data<1>()];
	}
      }
      lab=curr;
    }

    //cout << "\n " << labelTable << "*****" ; 
    
    // find highest value in lable table , this assumes that they run in sequence. 
    //unsigned maxLabel = 0 ; 
    //for ( SArray1dIterC<unsigned> iter ( labelTable) ; iter.valid() ; iter.next() )
      //if ( iter.Data() > maxLabel ) maxLabel = iter.Data() ; 
    //
    return std::tuple<Array<unsigned,2>,unsigned>(jp,lab);
  }

}
