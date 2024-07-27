// This file is part of RAVL, Recognition And Vision Library 
// Copyright (C) 2001, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here

#include "Ravl2/Image/Segmentation/Segmentation.hh"
#include "Ravl2/ArrayIterZip.hh"
//#include "Ravl2/Array2dSqr2Iter.hh"
//#include <vector>
//#include "Ravl2/SArray1dIter2.hh"
//#include "Ravl2/Array2dIter2.hh"
//#include "Ravl2/SobolSequence.hh"

namespace Ravl2
{

  SegmentationBodyC::SegmentationBodyC(const Array<int , 2> &nsegmap)
      : segmap(nsegmap.range()),
        labels(0)
  {
    for(auto it = begin(segmap,nsegmap);it.valid();++it) {
      it.data<0>() = unsigned(it.data<1>() >= 0 ? it.data<1>() : 0);
      if(it.data<0>() > labels)
        labels = it.data<0>();
    }
    labels++;
  }

  //: Generate a table of region adjacencies.
  
  std::vector<std::set<unsigned> > SegmentationBodyC::Adjacency(bool biDir)
  {
    std::vector<std::set<unsigned> > ret(labels);
    if(biDir) {
      for(Array2dSqr2IterC<unsigned> it(segmap);it;) {
	if(it.DataBR() != it.DataTR()) {
	  ret[it.DataBR()].insert(it.DataTR());
	  ret[it.DataTR()].insert(it.DataBR());
	}
	for(;it.next();) { // The rest of the image row.
	  if(it.DataBR() != it.DataTR()) {
	    ret[it.DataBR()].insert(it.DataTR());
	    ret[it.DataTR()].insert(it.DataBR());
	  }
	  if(it.DataBR() != it.DataBL()) {
	    ret[it.DataBR()].insert(it.DataBL());
	    ret[it.DataBL()].insert(it.DataBR());
	  }
	}
      }
    } else {
      for(Array2dSqr2IterC<unsigned> it(segmap);it;) {
	if(it.DataBR() != it.DataTR()) {
	  if(it.DataBR() < it.DataTR())
	    ret[it.DataBR()].insert(it.DataTR());
	  else
	    ret[it.DataTR()].insert(it.DataBR());
	}
	for(;it.next();) { // The rest of the image row.
	  if(it.DataBR() != it.DataTR()) {
	    if(it.DataBR() < it.DataTR())
	      ret[it.DataBR()].insert(it.DataTR());
	    else
	      ret[it.DataTR()].insert(it.DataBR());
	  }
	  if(it.DataBR() != it.DataBL()) {
	    if(it.DataBR() < it.DataBL())
	      ret[it.DataBR()].insert(it.DataBL());
	    else
	      ret[it.DataBL()].insert(it.DataBR());
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
      for(;it.next();) {
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
        for(;it.next();) { 
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
      for(;it.next();) {
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
        for(;it.next();) { 
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
  
  std::vector<unsigned> SegmentationBodyC::LocalBoundaryLength()
  {
    std::vector<unsigned> ret(labels,0);
    Array2dSqr2IterC<unsigned> it(segmap);
    if(!it) return ret;
    // First pixel
    if(it.DataBL() != it.DataTL()) {
      ret[it.DataBL()]++;
      ret[it.DataTL()]++;
    }
    // First row.
    for(;it.next();) {
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
      for(;it.next();) { 
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
  
  std::vector<unsigned> SegmentationBodyC::RedoArea(std::vector<unsigned> area,std::vector<unsigned> map)
  {
    std::vector<unsigned> ret(labels,0);
    assert(area.size() == map.size());
    for(size_t i = 0;i < area.size();i++) {
      assert(map[i] < labels);
      ret[map[i]] += area[i];
    }
    return ret;
  }
  
  //: Compute the areas of all the segmented regions.
  
  std::vector<unsigned> SegmentationBodyC::Areas() const {
    // Compute areas of components
    std::vector<unsigned> area(labels+1,0);
    for(auto pix : segmap)
      area[pix]++;
    return area;
  }
  
  //: Compute the bounding box and area of each region in the segmentation.
  
  std::vector<std::tuple<IndexRange<2>,unsigned> > SegmentationBodyC::BoundsAndArea() const {
    std::vector<std::tuple<IndexRange<2>,unsigned>  > ret(labels+1,std::tuple<IndexRange<2>,unsigned>(IndexRange<2>(),0));
    for(auto it = segmap.begin();it.valid();) {
      Index<2> at = it.index();
      do {
        std::tuple<IndexRange<2>,unsigned> &entry = ret[*it];
        if(std::get<1>(entry) == 0)
          std::get<0>(entry) = IndexRange<2>(at,at);
        std::get<0>(entry).involve(at);
        std::get<1>(entry)++;
        at[1]++;
      } while(it.next()) ;
    }
    return ret;
  }


  //: Make an array of labels mapping to themselves.
  
  std::vector<unsigned> SegmentationBodyC::IdentityLabel() {
    // Make an identity mapping.
    std::vector<unsigned> minLab(labels);
    unsigned c = 0;
    for(auto &it : minLab) {
      it = c++;
    }
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
    auto end = labelTable.begin() + currentMaxLabel + 1;
    // Make all trees of labels to be with depth one.
    {
      for (auto it = labelTable.begin(); it != end; ++it)
        *it = labelTable[*it];
    }

    // Now all components in the 'labelTable' have a unique label.
    // But there can exist holes in the sequence of labels.
    
    // Squeeze the table. 
    unsigned n = 0; // the next new label
    for(auto it = labelTable.begin(); it != end; ++it) {
      auto m = labelTable[*it];  // the label of the tree root
      
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
  
  unsigned SegmentationBodyC::CompressAndRelabel(std::vector<unsigned> &newLabs)
  {
    // ------ Compress labels -----
    // First make sure they form a directed tree
    // ending in the lowest valued label. 
    for(unsigned n = 0;n < unsigned(newLabs.size());n++) {
      if(newLabs[n] <= n)
        continue;
      // Minimize label.
      unsigned nat,at = newLabs[n];
      unsigned min = n;
      for(;at > n;at = nat) {
        nat = newLabs[at];
        if(nat < min)
          min = nat;
        else
          newLabs[at] = min;
      }
      newLabs[n] = min;
    }
    
    // Now we can minimize the labels.
    unsigned newLastLabel = RelabelTable(newLabs,labels-1);
    
    // And relabel the image.
    for(auto &iti : segmap)
      iti = newLabs[iti];
    
    labels = newLastLabel+1;
    return labels;
  }
  
  
  //: Remove small components from map, label them as 0.
  
  unsigned SegmentationBodyC::RemoveSmallComponents(unsigned thrSize) {
    if(labels <= 1)
      return labels;
    
    std::vector<unsigned> area = Areas();
    
    // Assign new labels to the regions according their sizes and
    // another requirements.
    unsigned newLabel = 1;
    auto it = area.begin();
    auto end = area.end();
    *it = 0;
    ++it;
    for(;it != end;++it) {
      if (*it < thrSize)
	*it = 0;
      else 
	*it = newLabel++;
    }
    
    // Remove small components
    for(auto &iti : segmap)
      iti = area[iti];
    
    labels = newLabel;
    return newLabel;    
  }

  //: Compute moments for each of the segmented regions.
  // if ignoreZero is true, region labeled 0 is ignored.

  std::vector<Moments2<float> > SegmentationBodyC::ComputeMoments(bool ignoreZero)
  {
    std::vector<Moments2<float>> ret(labels+1);
    
    if(ignoreZero) {
      // Ignore regions with label 0.
      for(auto it = segmap.begin();it.valid();++it) {
	if(*it == 0)
	  continue;
	ret[*it].addPixel(it.index());
      }
    } else {
      for(auto it = segmap.begin();it.valid();++it)
	{  ret[*it].addPixel(it.index()); }
    }
    return ret;
  }


  Array<uint8_t,2> SegmentationBodyC::ByteImage() const
  {
    Array<uint8_t,2> byteimage(segmap.range());
    forEach<2>([](unsigned &seg,uint8_t &byte) { byte = uint8_t(seg); }, segmap,byteimage);
    return byteimage;
  }

#if 0
  Array<ByteRGBValueC,2> SegmentationBodyC::RandomImage() const {
    //generate Sobol sequence
    SobolSequenceC sobel_sequence(3);
    std::vector<ByteRGBValueC> colours(labels);    
    sobel_sequence.First();
    for (SArray1dIterC<ByteRGBValueC> it(colours);it;it++,sobel_sequence.next())
      *it = ByteRGBValueC((ByteT) (sobel_sequence.Data()[0]*255), 
			  (ByteT) (sobel_sequence.Data()[1]*255),
			  (ByteT) (sobel_sequence.Data()[2]*255));
    
    //create RGBImage
    Array<ByteRGBValueC,2> rgbimage(segmap.range());
    for(Array2dIter2C<unsigned, ByteRGBValueC> seg_it(segmap, rgbimage); seg_it; seg_it++)
      seg_it.data2() = colours[seg_it..data1()];
    
    return rgbimage;
  }
  
  Array<ByteYUVValueC,2> SegmentationBodyC::RandomTaintImage(ByteT max) const {
    //generate Sobol sequence
    SobolSequenceC sobol_sequence(2);
    
    std::vector<ByteYUVValueC> colours(labels);    
    for (SArray1dIterC<ByteYUVValueC> it(colours);it;it++,sobol_sequence.next())
      *it = ByteYUVValueC(127, (ByteT) (sobol_sequence.Data()[0]*max), (ByteT)( sobol_sequence.Data()[1]*max));
    
    //create RGBImage
    Array<ByteYUVValueC,2> yuvimage(segmap.range());
    for(Array2dIter2C<unsigned, ByteYUVValueC> seg_it(segmap, yuvimage); seg_it; seg_it++)
      seg_it.data2() = colours[seg_it..data1()];
    return yuvimage;
  }
#endif
  
}
