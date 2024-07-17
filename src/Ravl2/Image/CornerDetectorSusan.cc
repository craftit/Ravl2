// This file is part of RAVL, Recognition And Vision Library
// Copyright (C) 2002, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here
//! rcsid="$Id$"
//! lib=RavlImageProc
//! author="Charles Galambos based on code by S.M.Smith"
//! file="Ravl/Image/Processing/Corners/CornerDetectorSusan.cc"

#include "Ravl2/Image/PeakDetector.hh"
#include "Ravl2/Image/CornerDetectorSusan.hh"
#include "Ravl2/Math.hh"

#define DODEBUG 0
#if DODEBUG
#define ONDEBUG(x) x
#else
#define ONDEBUG(x)
#endif

namespace RavlImageN {

  inline static int FTOI(float a)
  { return ( (a) < 0 ? ((int)(a-0.5)) : ((int)(a+0.5)) ); }


  ///////////////////////////////////
  // Constructor.

  CornerDetectorSusanBodyC::CornerDetectorSusanBodyC(int nTheshold)
    : threshold(nTheshold)
  {
    SetupLUT(6);
  }

  //: Setup LUT.

  void CornerDetectorSusanBodyC::SetupLUT(int form) {
    int   k;
    float temp;

    bp=&Lut[258];
    float thresh = threshold;
    for(k=-256;k<257;k++) {
      temp=((float)k)/thresh;
      temp=temp*temp;
      if (form==6)
	temp=temp*temp*temp;
      temp=100.0*Exp(-temp);
      bp[k]= (ByteT)temp;
    }
  }

  DListC<CornerC> CornerDetectorSusanBodyC::Apply(const ImageC<ByteT> &img) {
    ONDEBUG(cerr << "CornerDetectorSusanBodyC::Apply(), Called. \n");
    ImageC<IntT> cornerImage(img.Frame());
    DListC<CornerC> lst = Corners(img,cornerImage);
    Peaks(lst,cornerImage);
    return lst;
  }

  DListC<CornerC> CornerDetectorSusanBodyC::Corners(const ImageC<ByteT> &img,ImageC<IntT> &cornerMap) {

    DListC<CornerC> cornerList;
    cornerMap.Fill(0);
    const int max_no = 1850;

    ONDEBUG(int CCount = 0);

    int   n,x,y,sq,xx,yy;
    IntT i,j;
    float divide;
    const ByteT *p,*cp;
    ByteT c;

    const IntT colMin = img.Rectangle().Origin().Col().V() + 5;
    const IntT colMax = img.Rectangle().End().Col().V() - 5;
    const IntT rowMin = img.Rectangle().Origin().Row().V() + 5;
    const IntT rowMax = img.Rectangle().End().Row().V() - 5;

    for (i=rowMin;i<rowMax;i++)
      for (j=colMin;j<colMax;j++) {
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
	  divide=(float)y/(float)Abs(x);
	  sq=Abs(x)/x;
	  sq=*(cp-img[(i+FTOI(divide))][j+sq]) +
	    *(cp-img[(i+FTOI(2*divide))][j+2*sq]) +
	    *(cp-img[(i+FTOI(3*divide))][j+3*sq]);
	} else if(xx < sq) {
	  divide=(float)x/(float)Abs(y);
	  sq=Abs(y)/y;
	  sq=*(cp-img[(i+sq)][j+FTOI(divide)]) +
	    *(cp-img[(i+2*sq)][j+FTOI(2*divide)]) +
	    *(cp-img[(i+3*sq)][j+FTOI(3*divide)]);
	}
	if(sq <= 290)
	  continue;
	cornerMap[i][j] = max_no - n;
	cornerList.InsLast(CornerC(Point2dC(i,j),
				   (51*x)/n,(51*y)/n,
				   img[i][j])
			  );
	ONDEBUG(CCount++);
      }
    ONDEBUG(cerr << "CCount: " << CCount << "\n");
    return cornerList;
  }

  //: Remove non-maximal peaks.

  void CornerDetectorSusanBodyC::Peaks(DListC<CornerC> &list,const ImageC<IntT> &cornerMap) {
    for(DLIterC<CornerC> it(list);it;it++) {
      if(!PeakDetect7(cornerMap,it->Location()))
	it.Del();
    }
  }

}
