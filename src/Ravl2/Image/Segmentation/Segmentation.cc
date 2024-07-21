// This file is part of RAVL, Recognition And Vision Library 
// Copyright (C) 2001, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here

#include "Ravl2/Image/Segmentation/Segmentation.hh"
//#include "Ravl/Array2dSqr2Iter.hh"
//#include "Ravl/SArray1d.hh"
//#include "Ravl/SArray1dIter.hh"
//#include "Ravl/SArray1dIter2.hh"
//#include "Ravl/Array2dIter.hh"
//#include "Ravl/Array2dIter2.hh"
//#include "Ravl/SobolSequence.hh"

namespace Ravl2
{

  SegmentationBodyC::SegmentationBodyC(const Array<unsigned,2> &nsegmap)
      : segmap(nsegmap.range()),
        labels(0)
  {
    for(ArrayIterZip<2,unsigned,int> it(segmap,nsegmap);it;++it) {
      it.data1() = it.data2() >= 0 ? it.data2() : 0;
      if(it..data1() > labels)
        labels = it..data1();
    }
    labels++;
  }

  //: Generate a table of region adjacencies.
  
  std::vector<std::vector<unsigned> > SegmentationBodyC::Adjacency(bool biDir)
  {
    std::vector<std::vector<unsigned> > ret(labels);
    if(biDir) {
      for(Array2dSqr2IterC<unsigned> it(segmap);it;) {
	if(it.DataBR() != it.DataTR()) {
	  ret[it.DataBR()] += it.DataTR();
	  ret[it.DataTR()] += it.DataBR();
	}
	for(;it.Next();) { // The rest of the image row.
	  if(it.DataBR() != it.DataTR()) {
	    ret[it.DataBR()] += it.DataTR();
	    ret[it.DataTR()] += it.DataBR();
	  }
	  if(it.DataBR() != it.DataBL()) {
	    ret[it.DataBR()] += it.DataBL();
	    ret[it.DataBL()] += it.DataBR();
	  }
	}
      }
    } else {
      for(Array2dSqr2IterC<unsigned> it(segmap);it;) {
	if(it.DataBR() != it.DataTR()) {
	  if(it.DataBR() < it.DataTR())
	    ret[it.DataBR()] += it.DataTR();
	  else
	    ret[it.DataTR()] += it.DataBR();
	}
	for(;it.Next();) { // The rest of the image row.
	  if(it.DataBR() != it.DataTR()) {
	    if(it.DataBR() < it.DataTR())
	      ret[it.DataBR()] += it.DataTR();
	    else
	      ret[it.DataTR()] += it.DataBR();
	  }
	  if(it.DataBR() != it.DataBL()) {
	    if(it.DataBR() < it.DataBL())
	      ret[it.DataBR()] += it.DataBL();
	    else
	      ret[it.DataBL()] += it.DataBR();
	  }
	}
      }
    }
    return ret;
  }
  
  //: Generate a table of region adjacencies boundary lengths.
  // only adjacenies from regions with a smaller id to those 
  // with a larger ID are generated
  
  std::vector<std::map<unsigned,unsigned> > SegmentationBodyC::BoundaryLength(bool biDir) {
    std::vector<std::map<unsigned,unsigned> > ret(labels);
    if(biDir) {
      Array2dSqr2IterC<unsigned> it(segmap);
      if(!it) return ret;
      // First pixel
      if(it.DataBL() != it.DataTL()) {
        ret[it.DataBL()][it.DataTL()]++;
        ret[it.DataTL()][it.DataBL()]++;
      }
      // First row.
      for(;it.Next();) {
        if(it.DataBR() != it.DataTR()) {
          ret[it.DataBR()][it.DataTR()]++;
          ret[it.DataTR()][it.DataBR()]++;
        }
        if(it.DataBR() != it.DataBL()) {
          ret[it.DataBR()][it.DataBL()]++;
          ret[it.DataBL()][it.DataBR()]++;
        }
        if(it.DataTR() != it.DataTL()) {
          ret[it.DataTR()][it.DataTL()]++;
          ret[it.DataTL()][it.DataTR()]++;
        }
      }
      // Rest of image.
      for(;it;) {
        // First pixel in row.
        if(it.DataBL() != it.DataTL()) {
          ret[it.DataBL()][it.DataTL()]++;
          ret[it.DataTL()][it.DataBL()]++;
        }
        // The rest of the image row.
        for(;it.Next();) { 
          if(it.DataBR() != it.DataTR()) {
            ret[it.DataBR()][it.DataTR()]++;
            ret[it.DataTR()][it.DataBR()]++;
          }
          if(it.DataBR() != it.DataBL()) {
            ret[it.DataBR()][it.DataBL()]++;
            ret[it.DataBL()][it.DataBR()]++;
          }
        }
      }
    } else {
      Array2dSqr2IterC<unsigned> it(segmap);
      if(!it) return ret;
      // First pixel
      if(it.DataBL() != it.DataTL()) {
        if(it.DataBL() < it.DataTL())
          ret[it.DataBL()][it.DataTL()]++;
        else
          ret[it.DataTL()][it.DataBL()]++;
      }
      // First row.
      for(;it.Next();) {
        if(it.DataBR() != it.DataTR()) {
          if(it.DataBR() < it.DataTR())
            ret[it.DataBR()][it.DataTR()]++;
          else
            ret[it.DataTR()][it.DataBR()]++;
        }
        if(it.DataBR() != it.DataBL()) {
          if(it.DataBR() < it.DataBL())
            ret[it.DataBR()][it.DataBL()]++;
          else
            ret[it.DataBL()][it.DataBR()]++;
        }
        if(it.DataTR() != it.DataTL()) {
          if(it.DataTR() < it.DataTL())
            ret[it.DataTR()][it.DataTL()]++;
          else
            ret[it.DataTL()][it.DataTR()]++;
        }
      }
      // Rest of image.
      for(;it;) {
        // First pixel in row.
        if(it.DataBL() != it.DataTL()) {
          if(it.DataBL() < it.DataTL())
            ret[it.DataBL()][it.DataTL()]++;
          else
            ret[it.DataTL()][it.DataBL()]++;
        }
        // The rest of the image row.
        for(;it.Next();) { 
          if(it.DataBR() != it.DataTR()) {
            if(it.DataBR() < it.DataTR())
              ret[it.DataBR()][it.DataTR()]++;
            else
              ret[it.DataTR()][it.DataBR()]++;
          }
          if(it.DataBR() != it.DataBL()) {
            if(it.DataBR() < it.DataBL())
              ret[it.DataBR()][it.DataBL()]++;
            else
              ret[it.DataBL()][it.DataBR()]++;
          }
        }
      }
    }
    return ret;
  }
  
  //: Generate a table of the length of the boundary for each region
  
  std::vector<unsigned> SegmentationBodyC::LocalBoundaryLength() {
    std::vector<unsigned> ret(labels,0);
    Array2dSqr2IterC<unsigned> it(segmap);
    if(!it) return ret;
    // First pixel
    if(it.DataBL() != it.DataTL()) {
      ret[it.DataBL()]++;
      ret[it.DataTL()]++;
    }
    // First row.
    for(;it.Next();) {
      if(it.DataBR() != it.DataTR()) {
        ret[it.DataBR()]++;
        ret[it.DataTR()]++;
      }
      if(it.DataBR() != it.DataBL()) {
        ret[it.DataBR()]++;
        ret[it.DataBL()]++;
      }
      if(it.DataTR() != it.DataTL()) {
        ret[it.DataTR()]++;
        ret[it.DataTL()]++;
      }
    }
    // Rest of image.
    for(;it;) {
      // First pixel in row.
      if(it.DataBL() != it.DataTL()) {
        ret[it.DataBL()]++;
        ret[it.DataTL()]++;
      }
      // The rest of the image row.
      for(;it.Next();) { 
        if(it.DataBR() != it.DataTR()) {
          ret[it.DataBR()]++;
          ret[it.DataTR()]++;
        }
        if(it.DataBR() != it.DataBL()) {
          ret[it.DataBR()]++;
          ret[it.DataBL()]++;
        }
      }
    }
    return ret;
  }
  
  
  //: recompute the areas from the original areas and a mapping.
  
  std::vector<unsigned> SegmentationBodyC::RedoArea(std::vector<unsigned> area,std::vector<unsigned> map) {
    std::vector<unsigned> ret(labels,0);
    for(SArray1dIter2C<unsigned,unsigned> it(area,map);it;it++)
      ret[it.data2()] += it.data1();
    return ret;
  }
  
  //: Compute the areas of all the segmented regions.
  
  std::vector<unsigned> SegmentationBodyC::Areas() const {
    // Compute areas of components
    std::vector<unsigned> area(labels+1,0);
    for(Array2dIterC<unsigned> it(segmap);it;it++)
      area[*it]++;
    return area;
  }
  
  //: Compute the bounding box and area of each region in the segmentation.
  
  std::vector<std::tuple<IndexRange<2>,unsigned> > SegmentationBodyC::BoundsAndArea() const {
    std::vector<std::tuple<IndexRange<2>,unsigned>  > ret(labels+1,std::tuple<IndexRange<2>,unsigned>(IndexRange<2>(),0));
    for(ArrayIter<unsigned,2> it(segmap);it;) {
      Index<2> at = it.Index();
      do {
        Tuple2C<IndexRange<2>,unsigned> &entry = ret[*it];
        if(entry.data2() == 0)
          entry..data1() = IndexRange<2>(at,1);
        entry..data1().Involve(at);
        entry.data2()++;
        at.Col()++;
      } while(it.Next()) ;
    }
    return ret;
  }


  //: Make an array of labels mapping to themselves.
  
  std::vector<unsigned> SegmentationBodyC::IdentityLabel() {
    // Make an identity mapping.
    std::vector<unsigned> minLab(labels);
    unsigned c = 0;
    for(auto it : minLab)
      it = c++;
    return minLab;
  }
  
  //: Compress labels.
  // The 'labelTable' represents a look-up table for labels. 
  // Each item contains a new label which can be the same
  // as the index of the item or smaller. Such 'labelTable' contains
  // a forest of labels, every tree of labels represents one component
  // which should have the same label. It is valid that a root item
  // of a tree has the same label value as the item index.
  
  unsigned SegmentationBodyC::RelabelTable(std::vector<unsigned> &labelTable, unsigned currentMaxLabel) {
    SArray1dIterC<unsigned> it(labelTable,currentMaxLabel+1);
    // Make all trees of labels to be with depth one.
    for(;it;it++)
      *it = labelTable[*it];
    
    // Now all components in the 'labelTable' have a unique label.
    // But there can exist holes in the sequence of labels.
    
    // Squeeze the table. 
    unsigned n = 0;                     // the next new label  
    for(it.First();it;it++) {
      unsigned m = labelTable[*it];  // the label of the tree root
      
      // In the case m >= n the item with the index 'l' contains 
      // the root of the new tree,
      // because all processed roots have a label smaller than 'n'.
      // The root label 'm' has the same value as the index 'l'.
      // The root will be relabeled by a new label.
      *it = (m >= n) ? n++ : m;
    }
    
    return n - 1;  // the new last used label
  }
  
  //: Compress newlabs and re-label segmentation.
  
  unsigned SegmentationBodyC::CompressAndRelabel(std::vector<unsigned> &newLabs) {
    // ------ Compress labels -----
    // First make sure they form a directed tree
    // ending in the lowest valued label. 
    unsigned n = 0;
    for(SArray1dIterC<unsigned> it(newLabs);it;it++,n++) {
      if(*it > n) {
	// Minimize label.
	unsigned nat,at = *it;
	unsigned min = n;
	for(;at > n;at = nat) {
	  nat = newLabs[at];
	  if(nat < min)
	    min = nat;
	  else
	    newLabs[at] = min;	
	}
	*it = min;
      }
    }
    
    // Now we can minimize the labels.
    unsigned newLastLabel = RelabelTable(newLabs,labels-1);
    
    // And relable the image.
    for(Array2dIterC<unsigned> iti(segmap);iti;iti++)
      *iti = newLabs[*iti];
    
    labels = newLastLabel+1;
    return labels;
  }
  
  
  //: Remove small components from map, label them as 0.
  
  unsigned SegmentationBodyC::RemoveSmallComponents(int thrSize) {
    if(labels <= 1)
      return labels;
    
    std::vector<unsigned> area = Areas();
    
    // Assign new labels to the regions according their sizes and
    // another requirements.
    int newLabel = 1;
    SArray1dIterC<unsigned> it(area);
    *it = 0;
    it.Next();
    for(;it;it++) {
      if (*it < ((unsigned) thrSize)) 
	*it = 0;
      else 
	*it = newLabel++;
    }
    
    // Remove small components
    for(Array2dIterC<unsigned> iti(segmap);iti;iti++)
      *iti = area[*iti];
    
    labels = newLabel;
    return newLabel;    
  }

  //: Compute moments for each of the segmented regions.
  // if ignoreZero is true, region labeled 0 is ignored.
  
  std::vector<Moments2d2C> SegmentationBodyC::ComputeMoments(bool ignoreZero) {
    std::vector<Moments2d2C> ret(labels+1);
    
    if(ignoreZero) {
      // Ignore regions with label 0.
      for(Array2dIterC<unsigned> it(segmap);it;it++) {
	if(*it == 0)
	  continue;
	ret[*it].AddPixel(it.Index());
      }
    } else {
      for(Array2dIterC<unsigned> it(segmap);it;it++) 
	{  ret[*it].AddPixel(it.Index()); }
    }
    return ret;
  }


  ImageC<ByteT> SegmentationBodyC::ByteImage() const
  {
    ImageC<ByteT> byteimage(segmap.Rectangle());
    Array2dIter2C<unsigned, ByteT> seg_it(segmap, byteimage);
    for(seg_it.First(); seg_it.IsElm(); seg_it.Next())
      seg_it.data2() = (unsigned char)seg_it..data1();
    return byteimage;
  }

#if 0
  ImageC<ByteRGBValueC> SegmentationBodyC::RandomImage() const {
    //generate Sobol sequence
    SobolSequenceC sobel_sequence(3);
    std::vector<ByteRGBValueC> colours(labels);    
    sobel_sequence.First();
    for (SArray1dIterC<ByteRGBValueC> it(colours);it;it++,sobel_sequence.Next())
      *it = ByteRGBValueC((ByteT) (sobel_sequence.Data()[0]*255), 
			  (ByteT) (sobel_sequence.Data()[1]*255),
			  (ByteT) (sobel_sequence.Data()[2]*255));
    
    //create RGBImage
    ImageC<ByteRGBValueC> rgbimage(segmap.Frame());
    for(Array2dIter2C<unsigned, ByteRGBValueC> seg_it(segmap, rgbimage); seg_it; seg_it++)
      seg_it.data2() = colours[seg_it..data1()];
    
    return rgbimage;
  }
  
  ImageC<ByteYUVValueC> SegmentationBodyC::RandomTaintImage(ByteT max) const {
    //generate Sobol sequence
    SobolSequenceC sobol_sequence(2);
    
    std::vector<ByteYUVValueC> colours(labels);    
    for (SArray1dIterC<ByteYUVValueC> it(colours);it;it++,sobol_sequence.Next())
      *it = ByteYUVValueC(127, (ByteT) (sobol_sequence.Data()[0]*max), (ByteT)( sobol_sequence.Data()[1]*max));
    
    //create RGBImage
    ImageC<ByteYUVValueC> yuvimage(segmap.Rectangle());
    for(Array2dIter2C<unsigned, ByteYUVValueC> seg_it(segmap, yuvimage); seg_it; seg_it++)
      seg_it.data2() = colours[seg_it..data1()];
    return yuvimage;
  }
#endif
  
}
