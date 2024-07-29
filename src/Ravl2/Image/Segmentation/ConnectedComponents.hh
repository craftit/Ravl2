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
#include "Ravl2/Image/Array2Sqr2Iter.hh"
#include "Ravl2/Image/Array2Sqr2Iter2.hh"

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
    
    static unsigned RelabelTable(std::vector<unsigned> &labelTable);
    
  protected:
    size_t maxLabel;
    bool ignoreZero;
  };


  //: Default <code>CompareT</code> comparison operator for connected components
  
  template <class DataTypeT>
  class ConnectedComponentsCompareC
  {
  public:
    inline bool operator()(const DataTypeT &v1,const DataTypeT &v2)
    { return v1 == v2; }
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

  template <class DataTypeT,class CompareT = ConnectedComponentsCompareC<DataTypeT> >
  class ConnectedComponentsBodyC
    : public ConnectedComponentsBaseBodyC
  {
  public:
    ConnectedComponentsBodyC (unsigned nmaxLabel = 10000, bool setIgnoreZero=false,const CompareT &compMethod = CompareT())
      : ConnectedComponentsBaseBodyC(nmaxLabel, setIgnoreZero),
	compare(compMethod)
    {}
    //: Constructor
    // (See handle class ConnectedComponentsC)

    ConnectedComponentsBodyC (unsigned nmaxLabel,const CompareT &compMethod,const DataTypeT &zeroValue)
      : ConnectedComponentsBaseBodyC(nmaxLabel, true),
	compare(compMethod),
	zero(zeroValue)
    {}
    //: Constructor
    // (See handle class ConnectedComponentsC)
    
    std::tuple<Array<unsigned,2>,unsigned> apply (const Array<DataTypeT,2> &im);
    //: Performs the connected component labelling
    
  protected:
    CompareT compare;
    DataTypeT zero {}; // Zero value to use.
  };

    
  
  /////////////////////////////////////////////////////////////////////////
  // Implementation:
  
  template <class DataTypeT,class CompareT >
  std::tuple<Array<unsigned,2>,unsigned> ConnectedComponentsBodyC<DataTypeT,CompareT>::apply (const Array<DataTypeT,2> &ip) {
    std::vector<unsigned> labelTable;
    labelTable.reserve(maxLabel+1);
    // If there are two labels for the same component, the bigger label bin
    // contains the value of the smaller label.
    Array<unsigned,2> jp(ip.range());
    
    // Label the first row.
    labelTable.push_back(0); // usually a special label
    {
      auto it1 = begin(ip,jp);
      if(!it1.valid())
	return {jp,0}; // Zero size image.
      auto newLab = unsigned(labelTable.size()); // Label first pixel in the image.
      it1.template data<1>() = newLab;
      labelTable.push_back(newLab); // Setup first label.
      DataTypeT lastValue = it1.template data<0>();
      for (;it1.next();) { // Only iterate through the first row.
	if(compare(it1.template data<0>(),lastValue))
	  it1.template data<1>() = newLab;
	else { // a new component
          newLab = unsigned(labelTable.size()); // Label first pixel in the image.
	  it1.template data<1>() = newLab;
	  labelTable.push_back(newLab);
	}
	lastValue = it1.template data<0>();
      }
    }

    for(Array2dSqr2Iter2C<DataTypeT,unsigned> it(ip,jp);it.valid();) {
      // Label the first column pixel.
      if (compare(it.DataBL1(),it.DataTL1()))
	it.DataBL2() = it.DataTL2();
      else {
        auto newLab = unsigned(labelTable.size()); // Label first pixel in the image.
	it.DataBL2() = newLab;
	labelTable.push_back(newLab);
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
        auto newLab = unsigned(labelTable.size()); // Label first pixel in the image.
	it.DataBR2() = newLab;
	labelTable.push_back(newLab);
	// +2 according to the first column pixel
	if (newLab > maxLabel)  { // Try condensate the 'labelTable'.
	  unsigned newLastLabel = RelabelTable(labelTable);
	  Index<2> iat = it.template index<0>();
	  // Change labels in the processed part of the image.
	  IndexRange<2> subRect(jp.range());
	  int ix = iat[0];
	  int iy = iat[1];
	  subRect.max(0) = ix - 1;
	  for(auto its = begin(clip(jp,subRect));its.valid();++its)
	    *its = labelTable[*its];
	  
	  // Change labels in the processed part of the row.
	  for (int jy = ip.range().min()[1]; jy <= iy; ++jy) {
            unsigned &oldLabel = jp[ix][jy];
            assert(oldLabel < labelTable.size());
            oldLabel = labelTable[oldLabel];
          }
	  
	  // Create the correct label scheme.
	  if ((newLastLabel + newLastLabel/2) > maxLabel) {
	    // The size of the 'labelTable' is too small.
	    maxLabel *= 2; // Double the size of the table.
	  }
	  // Create an identity label set.
	  unsigned ll = 0;
          labelTable.resize(newLastLabel+1);
          for(auto &itx : labelTable) {
            itx = ll++;
          }
	}
      } while(it.next());
    }
    
    // Relabel the whole image
    if (labelTable.size() < 2) {
      return {jp, unsigned(labelTable.size())};
    }
    
    // newLastLabel =
    RelabelTable(labelTable);
    // Change labels in the have been processed area
    for(auto &it : jp) {
      it = labelTable[it];
    }
    
    // The user may have requested to ignore the empty pixels (e.g. zeros) in the original image
    if(ignoreZero) {
      std::vector<int> arr(labelTable.size()+1,-1);
      int curr = 1;
      for(auto it = begin(ip, jp);it.valid();++it) {
	if(it.template data<0>()==zero)
	  it.template data<1>() = 0;
	else {
	  if(arr[it.template data<1>()]==-1) {
	    arr[it.template data<1>()] = curr;
	    curr++;
	  }
	  it.template data<1>() = unsigned(arr[it.template data<1>()]);
	}
      }
    }

    return {jp,labelTable.size()};
  }

}
