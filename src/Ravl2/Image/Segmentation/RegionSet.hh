// This file is part of RAVL, Recognition And Vision Library 
// Copyright (C) 2003, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here

//! date="12/06/1998"
//! author="Ratna Rambaruth"
//! example=exSegmentation.cc

#pragma once

#include "Ravl2/Image/Segmentation/Segmentation.hh"

namespace RavlImageN {
  
  //! userlevel=Develop
  //: Enhanced segmentation map
  // Includes a segmentation map and statistics associated to each region

  template<class StatT>
  class RegionSetBodyC 
    : public SegmentationBodyC 
  {
  public:
    RegionSetBodyC() 
    {}
    //: Default constructor
    
    RegionSetBodyC(const ImageC<UIntT> & map,UIntT nLabels, const HashC<UIntT,StatT> & stats)
      : SegmentationBodyC(map,nLabels)
    { statset = stats;}
    //: Constructor from the segmentation map and the associated statistics for each region 
    
    inline HashC<UIntT,StatT> Stats() const
    { return statset; }
    //: Returns the statistics associated to each region in the segmentation map
    
    inline HashC<UIntT,StatT> & Stats()
    { return statset; }
    //: Returns the statistics associated to each region in the segmentation map
    
    inline ImageC<ByteT> ByteImage() const;
    //: Returns the segmentation map in the form of a ByteImageC
    
    inline  ImageC<ByteRGBValueC> RandomImage() const;
    //: Returns the segmentation map in the form of a colour random image; this means that segmentation maps with more than 255 labels can be saved to disk

    inline  ImageC<ByteYUVValueC> RandomTaintImage(ByteT max=100) const;
    //: Returns the segmentation map in the form of a colour random image. The Y channel is left blank (e.g., for displaying the original data). The labels in the U and V channels are in the range 0 to 'max'.
    
    void SetMap(const ImageC<UIntT> & map)
    { this->setmap = map; }
    //: The segmentation map for this is changed to map; UpdateStats should be performed after this operation
    
    template<class ClassT, class PValueT> void UpdateStats(const ImageC<PValueT> & img);
    //: Calculates the statistics of the regions in the segmentation map from scratch
    
    UIntT CompressAndRelabel(SArray1dC<UIntT> &newLabs) {
      HashC<UIntT,StatT> newStats;
      for(HashIterC<UIntT,StatT> it(statset);it;it++)
	newStats[newLabs[it.Key()]] = it.Data();
      statset = newLabs;
      return statset.Size();
    }
    //: Compress newlabs and re-label segmentation.
    // this correctly resolves multilevel mappings.
    // Note: newLabs will be changed to contain a mapping
    // from the original labels to their new values.
    
  protected:
    HashC<UIntT,StatT> statset;
  };
  

  template<class StatT>
  template<class ClassT, class PValueT>
  void RegionSetBodyC<StatT>::UpdateStats(const ImageC<PValueT> & img) {
    HashC<IntT, ClassT> hash;
    for(Array2dIter2C<UIntT, PValueT> seg_it(segmap, img); seg_it; seg_it++){
      //insert pixel in the right hash table bin
      if (hash.Lookup(seg_it.Data1())==NULL){
	hash.Insert(seg_it.Data1(), ClassT());
	hash[seg_it.Data1()].Initialise();
      } else 
	hash[seg_it.Data1()].Include(seg_it.Pixel(), seg_it.Data2()); 
    }
    
    statset.Empty();
    for(HashIterC<IntT, ClassT> hash_it(hash); hash_it; hash_it++)
      statset[hash_it.Key()] = hash_it.Stat();
  }  
  
  
}

#endif
