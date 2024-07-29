// This file is part of RAVL, Recognition And Vision Library
// Copyright (C) 2002, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here
//! author="Charles Galambos based on code by S.M.Smith"

#include "Ravl2/Image/PeakDetector.hh"
#include "Ravl2/Image/CornerDetectorSusan.hh"
#include "Ravl2/Math.hh"

#define DODEBUG 0
#if DODEBUG
#define ONDEBUG(x) x
#else
#define ONDEBUG(x)
#endif

namespace Ravl2 
{

  inline static int FTOI(float a)
  { return ( (a) < 0 ? (int(a-0.5f)) : (int(a+0.5f)) ); }


  ///////////////////////////////////
  // Constructor.

  CornerDetectorSusan::CornerDetectorSusan(int nTheshold)
    : threshold(nTheshold)
  {
    SetupLUT(6);
  }

  //: Setup LUT.

  void CornerDetectorSusan::SetupLUT(int form)
  {
    int   k;
    float temp;

    bp=&Lut[258];
    auto bpPtr = &Lut[258];
    auto thresh = float(threshold);
    for(k=-256;k<257;k++) {
      temp=float(k)/thresh;
      temp=temp*temp;
      if (form==6)
	temp=temp*temp*temp;
      temp=100.0f * std::exp(-temp);
      bpPtr[k]= uint8_t(temp);
    }
  }

  std::vector<CornerC> CornerDetectorSusan::apply(const Array<ByteT, 2> &img) const
  {
    ONDEBUG(SPDLOG_INFO("CornerDetectorSusan::apply(), Called. "));
    Array<int,2> cornerImage(img.range());
    auto lst = Corners(img,cornerImage);
    Peaks(lst,cornerImage);
    return lst;
  }

  std::vector<CornerC>CornerDetectorSusan::Corners(const Array<uint8_t,2> &img,Array<int,2> &cornerMap) const
  {
    std::vector<CornerC> cornerList;
    if(img.empty())
      return cornerList;
    cornerList.reserve(size_t(img.range().area()/100));
    cornerMap.fill(0);
    static const int max_no = 1850;

    ONDEBUG(size_t CCount = 0);

    IndexRange<2> window = img.range().shrink(8);
    ONDEBUG(SPDLOG_INFO("CornerDetectorSusan::Corners(), Window range: {}.",window));
    const int colMin = window.range(1u).min();
    const int colMax = window.range(1u).max();
    const int rowMin = window.range(0u).min();
    const int rowMax = window.range(0u).max();

    for (int i=rowMin;i<=rowMax;i++)
      for (int j=colMin;j<=colMax;j++) {
	int n = 100;
        const uint8_t *p = &(img[(i-3)][j - 1]);
        const uint8_t *cp = &bp[(img[i][j])];

	n+=*(cp-*p++);
	n+=*(cp-*p++);
	n+=*(cp-*p);

	p = &(img[(i-2)][j - 2]);

	n+=*(cp-*p++);
	n+=*(cp-*p++);
	n+=*(cp-*p++);
	n+=*(cp-*p++);
	n+=*(cp-*p);

	p = &(img[(i-1)][j - 3]);

	n+=*(cp-*p++);
	n+=*(cp-*p++);
	n+=*(cp-*p++);
	n+=*(cp-*p++);
	n+=*(cp-*p++);
	n+=*(cp-*p++);
	n+=*(cp-*p);

	p = &(img[i][j - 3]);

	n+=*(cp-*p++);
	n+=*(cp-*p++);
	n+=*(cp-*p);

	if (n>=max_no) continue;
	p+=2;
	n+=*(cp-*p++);
	if (n>=max_no) continue;
	n+=*(cp-*p++);
	if (n>=max_no) continue;
	n+=*(cp-*p);
	if (n>=max_no) continue;
	p = &(img[i+1][j - 3]);

	n+=*(cp-*p++);
	if (n>=max_no) continue;
	n+=*(cp-*p++);
	if (n>=max_no) continue;
	n+=*(cp-*p++);
	if (n>=max_no) continue;
	n+=*(cp-*p++);
	if (n>=max_no) continue;
	n+=*(cp-*p++);
	if (n>=max_no) continue;
	n+=*(cp-*p++);
	if(n>=max_no) continue;
	n+=*(cp-*p);
	if(n>=max_no) continue;
	p = &(img[i+2][j - 2]);

	n+=*(cp-*p++);
	if(n>=max_no) continue;
	n+=*(cp-*p++);
	if(n>=max_no) continue;
	n+=*(cp-*p++);
	if(n>=max_no) continue;
	n+=*(cp-*p++);
	if(n>=max_no) continue;
	n+=*(cp-*p);
	if(n>=max_no) continue;
	p = &(img[i+3][j - 1]);

	n+=*(cp-*p++);
	if(n>=max_no) continue;
	n+=*(cp-*p++);
	if(n>=max_no) continue;
	n+=*(cp-*p);

	if(n>=max_no) continue;

	int x=0;
        int y=0;
	p=&(img[i-3][j - 1]);

        uint8_t c=*(cp-*p++);x-=c;y-=3*c;
	c=*(cp-*p++);y-=3*c;
	c=*(cp-*p);x+=c;y-=3*c;

	p=&(img[i-2][j - 2]);

	c=*(cp-*p++);x-=2*c;y-=2*c;
	c=*(cp-*p++);x-=c;y-=2*c;
	c=*(cp-*p++);y-=2*c;
	c=*(cp-*p++);x+=c;y-=2*c;
	c=*(cp-*p);x+=2*c;y-=2*c;

	p=&(img[i-1][j - 3]);

	c=*(cp-*p++);x-=3*c;y-=c;
	c=*(cp-*p++);x-=2*c;y-=c;
	c=*(cp-*p++);x-=c;y-=c;
	c=*(cp-*p++);y-=c;
	c=*(cp-*p++);x+=c;y-=c;
	c=*(cp-*p++);x+=2*c;y-=c;
	c=*(cp-*p);x+=3*c;y-=c;

	p=&(img[i][j - 3]);

	c=*(cp-*p++);x-=3*c;
	c=*(cp-*p++);x-=2*c;
	c=*(cp-*p);x-=c;
	p+=2;
	c=*(cp-*p++);x+=c;
	c=*(cp-*p++);x+=2*c;
	c=*(cp-*p);x+=3*c;

	p=&(img[i+1][j - 3]);

	c=*(cp-*p++);x-=3*c;y+=c;
	c=*(cp-*p++);x-=2*c;y+=c;
	c=*(cp-*p++);x-=c;y+=c;
	c=*(cp-*p++);y+=c;
	c=*(cp-*p++);x+=c;y+=c;
	c=*(cp-*p++);x+=2*c;y+=c;
	c=*(cp-*p);  x+=3*c;y+=c;

	p=&(img[i+2][j - 2]);

	c=*(cp-*p++);x-=2*c;y+=2*c;
	c=*(cp-*p++);x-=c;y+=2*c;
	c=*(cp-*p++);y+=2*c;
	c=*(cp-*p++);x+=c;y+=2*c;
	c=*(cp-*p);  x+=2*c;y+=2*c;

	p=&(img[i+3][j - 1]);

	c=*(cp-*p++);x-=c;y+=3*c;
	c=*(cp-*p++);y+=3*c;
	c=*(cp-*p);  x+=c;y+=3*c;

	int xx=x*x;
        int yy=y*y;
        int sq=xx+yy;
	if ( sq <= ((n*n)/2) )
	  continue;
	sq=sq/2;
	if(yy < sq) {
	  float divide=float(y)/float(std::abs(x));
	  sq=std::abs(x)/x;
	  sq=*(cp-img[(i+FTOI(divide))][j+sq]) +
	    *(cp-img[(i+FTOI(2*divide))][j+2*sq]) +
	    *(cp-img[(i+FTOI(3*divide))][j+3*sq]);
	} else if(xx < sq) {
          float divide=float(x)/float(std::abs(y));
	  sq=std::abs(y)/y;
	  sq=*(cp-img[(i+sq)][j+FTOI(divide)]) +
	    *(cp-img[(i+2*sq)][j+FTOI(2*divide)]) +
	    *(cp-img[(i+3*sq)][j+FTOI(3*divide)]);
	}
	if(sq <= 290)
	  continue;
	cornerMap[i][j] = max_no - n;
        RavlAssert(window.contains(toIndex(i,j)));
	cornerList.emplace_back(toPoint<float>(i,j),
				   (51*RealT(x))/RealT(n),(51*RealT(y))/RealT(n),
				   img[i][j]
			  );
	ONDEBUG(CCount++);
      }
    ONDEBUG(SPDLOG_INFO("CornerDetectorSusan::Corners(), Found {} corners ({}).",cornerList.size(),CCount));
    ONDEBUG(assert(CCount == cornerList.size()));
    return cornerList;
  }

  //: Remove non-maximal peaks.

  void CornerDetectorSusan::Peaks(std::vector<CornerC> &list,const Array<int,2> &cornerMap)
  {
    auto end = list.end();
    for(auto it = list.begin();it != end;) {
      ONDEBUG(SPDLOG_INFO("CornerDetectorSusan::Peaks: Processing corner at {}.",it->Location()));
      auto pos = toIndex<2, float>(it->Location());
      RavlAssertMsg(cornerMap.range().shrink(3).contains(pos),"CornerDetectorSusan::Peaks: Position {}  out of range {}  .",pos,cornerMap.range().shrink(3));
      if(!PeakDetect7(cornerMap, pos)) {
	// Take element from end of list and move it here.
        ONDEBUG(SPDLOG_INFO("CornerDetectorSusan::Peaks: Removing corner at {}.",it->Location()));
	*it = list.back();
	end = list.erase(--end);
      } else {
	++it;
      }
    }
  }

}
