// This file is part of RAVL, Recognition And Vision Library 
// Copyright (C) 2002, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here
#pragma once

#include "Ravl2/Array.hh"
#include "Ravl2/Geometry/Geometry.hh"

namespace Ravl2
{

  //: Test if position 'pos' is the largest value in a 3 by 3 area.
  // Is is the users responsibility to ensure that all pixels around 'pos'
  // are in the image.
  template<class DataT>
  inline 
  bool PeakDetect3(const Array<DataT,2> &img,const Index<2> &pos) {
    const DataT *rt = &(img[pos]);
    const DataT &cent = rt[0];
    if(rt[-1] >= cent || rt[1] >= cent)
      return false;
    rt = &(img[pos[0]-1][pos[1]]);
    if(rt[-1] >= cent || rt[0] >= cent || rt[1] >= cent)
      return false;
    rt = &(img[pos[0]+1][pos[1]]);
    return (rt[-1] < cent && rt[0] < cent && rt[1] < cent);
  }

  template<class DataT>
  inline
  bool PeakDetect5(const Array<DataT,2> &img,const Index<2> &pos) {
    const DataT *rt = &(img[pos]);
    const DataT &cent = rt[0];
    const int cc = pos[1];
    const int cr = pos[0];
    // Check the middle row first as we already have a pointer to it.
    if(rt[-2] >= cent || rt[-1] >= cent || rt[1] >= cent || rt[2] >= cent)
      return false;
    
    rt = &(img[cr-1][cc]);
    if(rt[-2] >= cent || rt[-1] >= cent || rt[0] >= cent || rt[1] >= cent || rt[2] >= cent)
      return false;

    rt = &(img[cr+1][cc]);
    if(rt[-2] >= cent || rt[-1] >= cent || rt[0] >= cent || rt[1] >= cent || rt[2] >= cent)
      return false;

    rt = &(img[cr-2][cc]);
    if(rt[-1] >= cent || rt[0] >= cent || rt[1] >= cent)
      return false;
        
    rt = &(img[cr+2][cc]);
    if(rt[-1] >= cent || rt[0] >= cent || rt[1] >= cent)
      return false;
    return true;
  }
  //: Test if position 'pos' is the largest value in a 5 by 5 area.
  // Is is the users responsibility to ensure that all pixels around 'pos'
  // are in the image.  The corners of the area are not mask to bring
  // the area checked closer to a circle.

  template<class DataT>
  inline
  bool PeakDetect7(const Array<DataT,2> &img,const Index<2> &pos) {
    const DataT &cent = rt[0];
    const int cc = pos[1];
    const int cr = pos[0];

    const DataT *rt = &(img[pos]);
    // Check the middle row first as we already have a pointer to it.
    if(rt[-3] >= cent || rt[-2] >= cent || rt[-1] >= cent || rt[1] >= cent || rt[2] >= cent || rt[3] >= cent)
      return false;
    
    rt = &(img[cr-1][cc]);
    if(rt[-3] >= cent || rt[-2] >= cent || rt[-1] >= cent || rt[0] >= cent || rt[1] >= cent || rt[2] >= cent || rt[3] >= cent)
      return false;
    
    rt = &(img[cr+1][cc]);
    if(rt[-3] >= cent || rt[-2] >= cent || rt[-1] >= cent || rt[0] >= cent || rt[1] >= cent || rt[2] >= cent || rt[3] >= cent)
      return false;
    
    rt = &(img[cr-2][cc]);
    if(rt[-2] >= cent || rt[-1] >= cent || rt[0] >= cent || rt[1] >= cent || rt[2] >= cent)
      return false;
    
    rt = &(img[cr+2][cc]);
    if(rt[-2] >= cent || rt[-1] >= cent || rt[0] >= cent || rt[1] >= cent || rt[2] >= cent)
      return false;
    
    rt = &(img[cr-3][cc]);
    if(rt[-1] >= cent || rt[0] >= cent || rt[1] >= cent)
      return false;

    rt = &(img[cr+3][cc]);
    if(rt[-1] >= cent || rt[0] >= cent || rt[1] >= cent)
      return false;
    return true;
  }
  //: Test if position 'pos' is the largest value in a 7 by 7 area.
  // Is is the users responsibility to ensure that all pixels around 'pos'
  // are in the image.  The corners of the area are not mask to bring
  // the area checked closer to a circle.



  template<class DataT>
  Point2f LocatePeakSubPixel(const Array<DataT,2> &img,const Index<2> &pos,float pof) {
    // apply geometric fitting in image-proportional coordinates.
    if(!img.Frame().Erode().Contains(pos))
      return {float(pos[0]),float(pos[1])};
    
    const DataT *rt = &(img[pos[0]-1][pos[1]]);
    const float spp = std::pow((float) rt[-1],pof);
    const float spc = std::pow((float) rt[ 0],pof);
    const float spn = std::pow((float) rt[ 1],pof);
    
    rt = &(img[pos]);
    const float scp = std::pow((float) rt[-1],pof);
    const float scc = std::pow((float) rt[ 0],pof);
    const float scn = std::pow((float) rt[ 1],pof);
    
    rt = &(img[pos[0]+1][pos[1]]);
    const float snp = pow((float) rt[-1],pof);
    const float snc = std::pow((float) rt[ 0],pof);
    const float snn = std::pow((float) rt[ 1],pof);
    
    // Use least-squares to fit quadratic to local corner strengths.
    float Pxx = (spp - 2.0*spc + spn + scp - 2.0*scc + scn + snp - 2.0*snc + snn)/3.0;
    float Pxy = (spp - spn - snp + snn)/4.0;
    float Pyy = (spp + spc + spn - 2.0*scp - 2.0*scc - 2.0*scn + snp + snc + snn)/3.0;
    float Px = (- spp - scp - snp + spn + scn + snn)/6.0;
    float Py = (- spp - spc - spn + snp + snc + snn)/6.0;
    float det = Pxy*Pxy - Pxx*Pyy;

    Vector2f indf = {float(pos[0]),float(pos[1])};
    if(det == 0)
      return indf;
    
    // calculate sub-pixel corrections to the corner position.
    Vector2f corr({(Pyy*Px - Pxy*Py)/det,(Pxx*Py - Pxy*Px)/det});
    
    // pull the corrections inside the pixel.
    if (corr[0] > 0.5) 
     corr[0]=0.5; 
    if (corr[0] < -0.5) 
      corr[0]= -0.5;
    if (corr[1] > 0.5) 
      corr[1]=0.5; 
    if (corr[1] < -0.5) 
      corr[1]= -0.5;
    return indf + corr;
  }
  //: Locate peak with sub-pixel precision.
  // Fits a quadratic to the peak and works out the center. The position of the
  // peak is returned. 'img' should contain values surrounding the center of
  // the peak at 'pos'.

  template<class DataT>
  Point2f LocatePeakSubPixel(const Array<DataT,2> &img,const Index<2> &pos) {
    // apply geometric fitting in image-proportional coordinates.
    Point2f fpos = {float(pos[0]),float(pos[1])};
    if(!img.range().shrink(1).contains(pos))
      return fpos;
    
    const DataT *rt = &(img[pos[0]-1][pos[1]]);
    float spp = (float) rt[-1];
    float spc = (float) rt[ 0];
    float spn = (float) rt[ 1];
    
    rt = &(img[pos]);
    float scp = (float) rt[-1];
    float scc = (float) rt[ 0];
    float scn = (float) rt[ 1];
    
    rt = &(img[pos[0]+1][pos[1]]);
    float snp = (float) rt[-1];
    float snc = (float) rt[ 0];
    float snn = (float) rt[ 1];
    
    // Use least-squares to fit quadratic to local corner strengths.
    float Pxx = (spp - 2.0*spc + spn + scp - 2.0*scc + scn + snp - 2.0*snc + snn)/3.0;
    float Pxy = (spp - spn - snp + snn)/4.0;
    float Pyy = (spp + spc + spn - 2.0*scp - 2.0*scc - 2.0*scn + snp + snc + snn)/3.0;
    float Px = (- spp - scp - snp + spn + scn + snn)/6.0;
    float Py = (- spp - spc - spn + snp + snc + snn)/6.0;
    float det = Pxy*Pxy - Pxx*Pyy;
    
    if(det == 0)
      return fpos;
    
    // calculate sub-pixel corrections to the corner position.
    Vector2f corr = {(Pyy*Px - Pxy*Py)/det,(Pxx*Py - Pxy*Px)/det};
    
    // pull the corrections inside the pixel.
    if (corr[0] > 0.5) 
     corr[0]=0.5; 
    if (corr[0] < -0.5) 
      corr[0]= -0.5;
    if (corr[1] > 0.5) 
      corr[1]=0.5; 
    if (corr[1] < -0.5) 
      corr[1]= -0.5;
    return fpos + corr;
  }
  //: Locate peak with sub-pixel precision.
  // Fits a quadratic to the peak and works out the center. The position of the
  // peak is returned. 'img' should contain values surrounding the center of
  // the peak at 'pos'.
}

