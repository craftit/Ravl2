// This file is part of RAVL, Recognition And Vision Library 
// Copyright (C) 2002, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here
//! rcsid="$Id$"
//! lib=RavlImageProc
//! file="Ravl/Image/Processing/Filters/Matching.cc"

#include "Ravl/config.h"
#include "Ravl/Array2dIter2.hh"

#if RAVL_USE_MMX  
#include "Ravl/mmx.hh"
#endif

#include "Ravl/Image/Matching.hh"

namespace RavlImageN {
  
#if RAVL_USE_MMX
  IntT MatchSumAbsDifference(const Array2dC<ByteT> &imgTemplate,
			     const Array2dC<ByteT> &img,
			     const Index2dC &origin,
			     IntT &diff) {
    diff = 0;
    IndexRange2dC srect(imgTemplate.Frame());
    srect += origin;
    RavlAssert(img.Frame().Contains(srect)); 
    INITMMX;
    int cols = imgTemplate.Frame().Cols();
    switch(cols) { // Choose cols.
    case 8: {
      BufferAccess2dIter2C<ByteT,ByteT> it(imgTemplate,imgTemplate.Range1(),imgTemplate.Range2(),
					   img,srect.Range1(),srect.Range2());
      
      __asm__ volatile ("\n\t pxor       %%mm7, %%mm7 "
			"\n\t pxor       %%mm6, %%mm6 "
			: : "m" (cols) ); // Dummy arg to fix bug in gcc 2.95.3
      
      do {
	__asm__ volatile (
	   "\n\t movq       %0,    %%mm0             # grab 1st half of row from bptr "
	   "\n\t movq       %1,    %%mm1             # grab 1st half of row from cptr "
	   "\n\t movq       %%mm0, %%mm2             # make a copy for abs diff operation "
	   "\n\t psubusb    %%mm1, %%mm0             # do subtraction one way (w/saturation) "
	   "\n\t psubusb    %%mm2, %%mm1             # do subtraction the other way (w/saturation) "     
	   "\n\t por        %%mm1, %%mm0             # merge results of 1st half "
	   "\n\t movq       %%mm0, %%mm1             # keep a copy "     
	   "\n\t punpcklbw  %%mm6, %%mm0             # unpack to higher precision for accumulation "
	   "\n\t psrlq      $32 ,  %%mm1             # shift results for accumulation "
	   "\n\t punpcklbw  %%mm6, %%mm1             # unpack to higher precision for accumulation "     
	   "\n\t paddw      %%mm0, %%mm7             # accumulate difference... "
	   "\n\t paddw      %%mm1, %%mm7             # accumulate difference... "
	   : : "m"(it.Data1()), "m"(it.Data2())
	   );
      } while(it.NextRow()) ;
      
      __asm__ volatile 
	(
	 "\n\t movq       %%mm7, %%mm0 "
	 "\n\t punpcklwd  %%mm6, %%mm7 "
	 "\n\t punpckhwd  %%mm6, %%mm0 "
	 "\n\t paddd      %%mm7, %%mm0 "
	 "\n\t movq       %%mm0, %%mm1 "
	 "\n\t psrlq      $32,   %%mm1 "			
	 "\n\t paddd      %%mm1, %%mm0 " // Don't actually use the upper part of the sum.
	 "\n\t movd       %%mm0, %0 "
	 : "=m" (diff) : 
	 );
      
    } break;
    
    case 16: {
      BufferAccess2dIter2C<ByteT,ByteT> it(imgTemplate,imgTemplate.Range1(),imgTemplate.Range2(),
					   img,srect.Range1(),srect.Range2());
      
      __asm__ volatile ("\n\t pxor       %%mm7, %%mm7 "
			"\n\t pxor       %%mm6, %%mm6 "
			: : "m" (cols) ); // Dummy arg to fix bug in gcc 2.95.3
      
      do {
	__asm__ volatile 
	  (
	   "\n\t movq       %0,    %%mm0             # grab 1st half of row from bptr "
	   "\n\t movq       %1,    %%mm1             # grab 1st half of row from cptr "
	   "\n\t movq       %%mm0, %%mm2             # make a copy for abs diff operation "
	   "\n\t movq       %2,    %%mm3             # grab 2nd half of row from bptr "
	   "\n\t psubusb    %%mm1, %%mm0             # do subtraction one way (w/saturation) "
	   "\n\t psubusb    %%mm2, %%mm1             # do subtraction the other way (w/saturation) "
	   "\n\t movq       %3,    %%mm4             # grab 2nd half of row from cptr "
	   "\n\t por        %%mm1, %%mm0             # merge results of 1st half "
	   "\n\t movq       %%mm3, %%mm5             # make a copy for abs diff operation "
	   "\n\t movq       %%mm0, %%mm1             # keep a copy "
	   "\n\t psubusb    %%mm4, %%mm3             # do subtraction one way (w/saturation) "
	   "\n\t punpcklbw  %%mm6, %%mm0             # unpack to higher precision for accumulation "
	   "\n\t psubusb    %%mm5, %%mm4             # do subtraction the other way (w/saturation) "
	   "\n\t psrlq      $32 ,  %%mm1             # shift results for accumulation "
	   "\n\t por        %%mm4, %%mm3             # merge results of 2nd half "
	   "\n\t punpcklbw  %%mm6, %%mm1             # unpack to higher precision for accumulation "
	   "\n\t movq       %%mm3, %%mm4             # keep a copy "
	   "\n\t punpcklbw  %%mm6, %%mm3             # unpack to higher precision for accumulation "
	   "\n\t paddw      %%mm0, %%mm7             # accumulate difference... "
	   "\n\t psrlq      $32,   %%mm4             # shift results for accumulation "
	   "\n\t paddw      %%mm1, %%mm7             # accumulate difference... "
	   "\n\t punpcklbw  %%mm6, %%mm4             # unpack to higher precision for accumulation "
	   "\n\t paddw      %%mm3, %%mm7             # accumulate difference... "
	   "\n\t paddw      %%mm4, %%mm7             # accumulate difference... "
	   : : "m"(it.Data1()), "m"(it.Data2()),"m"(*(&(it.Data1()) + 8)),"m"(*(&(it.Data2())+8)) 
	   );
      } while(it.NextRow()) ;
      
      __asm__ volatile 
	(
	 "\n\t movq       %%mm7, %%mm0 "
	 "\n\t punpcklwd  %%mm6, %%mm7 "
	 "\n\t punpckhwd  %%mm6, %%mm0 "
	 "\n\t paddd      %%mm7, %%mm0 "
	 "\n\t movq       %%mm0, %%mm1 "
	 "\n\t psrlq      $32,   %%mm1 "			
	 "\n\t paddd      %%mm1, %%mm0 " // Don't actually use the upper part of the sum.
	 "\n\t movd       %%mm0, %0 "
	 : "=m" (diff) : 
	 );
      //cerr << "Diff=" << hex << diff << "\n";
    } break;
    default:
      RangeBufferAccess2dC<ByteT> subImg(img,srect); 
      if(cols < 8) { // 1 - 7
	for(BufferAccess2dIter2C<ByteT,ByteT> it(imgTemplate,imgTemplate.Range2(),subImg,subImg.Range2());it;it++)
	  diff += Abs((IntT) it.Data1() - (IntT) it.Data2());
      } else if(cols < 16) { // 9 - 15
	
	BufferAccess2dIter2C<ByteT,ByteT> it(imgTemplate,imgTemplate.Range2(),subImg,subImg.Range2());
	__asm__ volatile ("\n\t pxor       %%mm7, %%mm7 "
			  "\n\t pxor       %%mm6, %%mm6 "
			  : : "m" (cols) ); // Dummy arg to fix bug in gcc 2.95.3
	int diff1 = 0;
	while(it) {
	  __asm__ volatile 
	    ("\n\t movq       %0,    %%mm0             # grab 1st half of row from bptr "
	     "\n\t movq       %1,    %%mm1             # grab 1st half of row from cptr "
	     "\n\t movq       %%mm0, %%mm2             # make a copy for abs diff operation "
	     "\n\t psubusb    %%mm1, %%mm0             # do subtraction one way (w/saturation) "
	     "\n\t psubusb    %%mm2, %%mm1             # do subtraction the other way (w/saturation) "     
	     "\n\t por        %%mm1, %%mm0             # merge results of 1st half "
	     "\n\t movq       %%mm0, %%mm1             # keep a copy "     
	     "\n\t punpcklbw  %%mm6, %%mm0             # unpack to higher precision for accumulation "
	     "\n\t psrlq      $32 ,  %%mm1             # shift results for accumulation "
	     "\n\t punpcklbw  %%mm6, %%mm1             # unpack to higher precision for accumulation "     
	     "\n\t paddw      %%mm0, %%mm7             # accumulate difference... "
	     "\n\t paddw      %%mm1, %%mm7             # accumulate difference... "
	     : : "m"(it.Data1()), "m"(it.Data2())
	     );
	  it.NextCol(8); // Skip ones we've added.
	  do {
	    diff1 += Abs((IntT) it.Data1() - (IntT) it.Data2());
	  } while(it.Next());
	}
	
	__asm__ volatile 
	  ("\n\t movq       %%mm7, %%mm0 "
	   "\n\t punpcklwd  %%mm6, %%mm7 "
	   "\n\t punpckhwd  %%mm6, %%mm0 "
	   "\n\t paddd      %%mm7, %%mm0 "
	   "\n\t movq       %%mm0, %%mm1 "
	   "\n\t psrlq      $32,   %%mm1 "			
	   "\n\t paddd      %%mm1, %%mm0 " // Don't actually use the upper part of the sum.
	   "\n\t movd       %%mm0, %0 "
	   : "=m" (diff) : 
	   );
	diff += diff1;
      } else { // 17 and upwards.
	
	BufferAccess2dIter2C<ByteT,ByteT> it(imgTemplate,imgTemplate.Range2(),subImg,subImg.Range2());
	__asm__ volatile ("\n\t pxor       %%mm7, %%mm7 "
			  "\n\t pxor       %%mm6, %%mm6 "
			  : : "m" (cols) ); // Dummy arg to fix bug in gcc 2.95.3
	int diff1 = 0;
	while(it) {
	  __asm__ volatile 
	    ("\n\t movq       %0,    %%mm0             # grab 1st half of row from bptr "
	     "\n\t movq       %1,    %%mm1             # grab 1st half of row from cptr "
	     "\n\t movq       %%mm0, %%mm2             # make a copy for abs diff operation "
	     "\n\t movq       %2,    %%mm3             # grab 2nd half of row from bptr "
	     "\n\t psubusb    %%mm1, %%mm0             # do subtraction one way (w/saturation) "
	     "\n\t psubusb    %%mm2, %%mm1             # do subtraction the other way (w/saturation) "
	     "\n\t movq       %3,    %%mm4             # grab 2nd half of row from cptr "
	     "\n\t por        %%mm1, %%mm0             # merge results of 1st half "
	     "\n\t movq       %%mm3, %%mm5             # make a copy for abs diff operation "
	     "\n\t movq       %%mm0, %%mm1             # keep a copy "
	     "\n\t psubusb    %%mm4, %%mm3             # do subtraction one way (w/saturation) "
	     "\n\t punpcklbw  %%mm6, %%mm0             # unpack to higher precision for accumulation "
	     "\n\t psubusb    %%mm5, %%mm4             # do subtraction the other way (w/saturation) "
	     "\n\t psrlq      $32 ,  %%mm1             # shift results for accumulation "
	     "\n\t por        %%mm4, %%mm3             # merge results of 2nd half "
	     "\n\t punpcklbw  %%mm6, %%mm1             # unpack to higher precision for accumulation "
	     "\n\t movq       %%mm3, %%mm4             # keep a copy "
	     "\n\t punpcklbw  %%mm6, %%mm3             # unpack to higher precision for accumulation "
	     "\n\t paddw      %%mm0, %%mm7             # accumulate difference... "
	     "\n\t psrlq      $32,   %%mm4             # shift results for accumulation "
	     "\n\t paddw      %%mm1, %%mm7             # accumulate difference... "
	     "\n\t punpcklbw  %%mm6, %%mm4             # unpack to higher precision for accumulation "
	     "\n\t paddw      %%mm3, %%mm7             # accumulate difference... "
	     "\n\t paddw      %%mm4, %%mm7             # accumulate difference... "
	     : : "m"(it.Data1()), "m"(it.Data2()),"m"(*(&(it.Data1()) + 8)),"m"(*(&(it.Data2())+8)) 
	     );
	  it.NextCol(16); // Skip ones we've added.
	  do {
	    diff1 += Abs((IntT) it.Data1() - (IntT) it.Data2());
	  } while(it.Next());
	}
	
	__asm__ volatile 
	  ("\n\t movq       %%mm7, %%mm0 "
	   "\n\t punpcklwd  %%mm6, %%mm7 "
	   "\n\t punpckhwd  %%mm6, %%mm0 "
	   "\n\t paddd      %%mm7, %%mm0 "
	   "\n\t movq       %%mm0, %%mm1 "
	   "\n\t psrlq      $32,   %%mm1 "			
	   "\n\t paddd      %%mm1, %%mm0 " // Don't actually use the upper part of the sum.
	   "\n\t movd       %%mm0, %0 "
	   : "=m" (diff) : 
	   );
	diff += diff1;
      }
    }
    CLOSEMMX;
    return diff;
  }
#endif
}
