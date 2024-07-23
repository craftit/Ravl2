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

  std::vector<CornerC> CornerDetectorSusan::Apply(const Array<uint8_t,2> &img) const
  {
    ONDEBUG(cerr << "CornerDetectorSusan::Apply(), Called. \n");
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
    const int max_no = 1850;

    ONDEBUG(int CCount = 0);

    int   n,x,y,sq,xx,yy;
    int i,j;
    float divide;
    const uint8_t *p,*cp;
    uint8_t c;

    IndexRange<2> window = img.range().shrink(5);
    const int colMin = window.range(1u).min();
    const int colMax = window.range(1u).max();
    const int rowMin = window.range(0u).min();
    const int rowMax = window.range(0u).max();

    for (i=rowMin;i<=rowMax;i++)
      for (j=colMin;j<=colMax;j++) {
	n = 100;
	p = &(img[(i-3)][j - 1]);
	cp = &bp[(img[i][j])];

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

	x=0;y=0;
	p=&(img[i-3][j - 1]);

	c=*(cp-*p++);x-=c;y-=3*c;
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

	xx=x*x;
	yy=y*y;
	sq=xx+yy;
	if ( sq <= ((n*n)/2) )
	  continue;
	sq=sq/2;
	if(yy < sq) {
	  divide=float(y)/float(std::abs(x));
	  sq=std::abs(x)/x;
	  sq=*(cp-img[(i+FTOI(divide))][j+sq]) +
	    *(cp-img[(i+FTOI(2*divide))][j+2*sq]) +
	    *(cp-img[(i+FTOI(3*divide))][j+3*sq]);
	} else if(xx < sq) {
	  divide=float(x)/float(std::abs(y));
	  sq=std::abs(y)/y;
	  sq=*(cp-img[(i+sq)][j+FTOI(divide)]) +
	    *(cp-img[(i+2*sq)][j+FTOI(2*divide)]) +
	    *(cp-img[(i+3*sq)][j+FTOI(3*divide)]);
	}
	if(sq <= 290)
	  continue;
	cornerMap[i][j] = max_no - n;
	cornerList.push_back(CornerC(toPoint<float>(float(i),float(j)),
				   RealT((51*RealT(x))/RealT(n)),RealT((51*RealT(y))/RealT(n)),
				   img[i][j])
			  );
	ONDEBUG(CCount++);
      }
    ONDEBUG(cerr << "CCount: " << CCount << "\n");
    return cornerList;
  }

  //: Remove non-maximal peaks.

  void CornerDetectorSusan::Peaks(std::vector<CornerC> &list,const Array<int,2> &cornerMap) const
  {
    auto end = list.end();
    for(auto it = list.begin();it != end;++it) {
      if(!PeakDetect7(cornerMap, toIndex<float,2>(it->Location()))) {
	// Take element from end of list and move it here.
	*it = list.back();
	list.pop_back();
	end = list.end();
      } else {
	++it;
      }
    }
  }

}
