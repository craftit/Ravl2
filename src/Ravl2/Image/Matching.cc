// This file is part of RAVL, Recognition And Vision Library
// Copyright (C) 2002, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here

#include "Ravl2/config.hh"
#include "Ravl2/Image/Matching.hh"

#if RAVL_USE_MMX
#include "Ravl2/mmx.hh"
#endif

namespace Ravl2
{

#if RAVL_USE_MMX
  IntT matchSumAbsDifference(const Array<ByteT, 2> &imgTemplate,
                             const Array<ByteT, 2> &img)
  {
    diff = 0;
    INITMMX;
    int cols = imgTemplate.range().range().size(1);
    switch(cols) {// Choose cols.
      case 8: {
        BufferAccess2dIter2C<ByteT, ByteT> it(imgTemplate, imgTemplate.range(0), imgTemplate.range(1),
                                              img, srect.range(0), srect.range(1));

        __asm__ volatile("\n\t pxor       %%mm7, %%mm7 "
                         "\n\t pxor       %%mm6, %%mm6 "
                         : : "m"(cols));// Dummy arg to fix bug in gcc 2.95.3

        do {
          __asm__ volatile(
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
            : : "m"(it.data<0>()), "m"(it.data<1>()));
        } while(it.NextRow());

        __asm__ volatile(
          "\n\t movq       %%mm7, %%mm0 "
          "\n\t punpcklwd  %%mm6, %%mm7 "
          "\n\t punpckhwd  %%mm6, %%mm0 "
          "\n\t paddd      %%mm7, %%mm0 "
          "\n\t movq       %%mm0, %%mm1 "
          "\n\t psrlq      $32,   %%mm1 "
          "\n\t paddd      %%mm1, %%mm0 "// Don't actually use the upper part of the sum.
          "\n\t movd       %%mm0, %0 "
          : "=m"(diff) :);

      } break;

      case 16: {
        BufferAccess2dIter2C<ByteT, ByteT> it(imgTemplate, imgTemplate.range(0), imgTemplate.range(1),
                                              img, srect.range(0), srect.range(1));

        __asm__ volatile("\n\t pxor       %%mm7, %%mm7 "
                         "\n\t pxor       %%mm6, %%mm6 "
                         : : "m"(cols));// Dummy arg to fix bug in gcc 2.95.3

        do {
          __asm__ volatile(
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
            : : "m"(it.data<0>()), "m"(it.data<1>()), "m"(*(&(it.data<0>()) + 8)), "m"(*(&(it.data<1>()) + 8)));
        } while(it.NextRow());

        __asm__ volatile(
          "\n\t movq       %%mm7, %%mm0 "
          "\n\t punpcklwd  %%mm6, %%mm7 "
          "\n\t punpckhwd  %%mm6, %%mm0 "
          "\n\t paddd      %%mm7, %%mm0 "
          "\n\t movq       %%mm0, %%mm1 "
          "\n\t psrlq      $32,   %%mm1 "
          "\n\t paddd      %%mm1, %%mm0 "// Don't actually use the upper part of the sum.
          "\n\t movd       %%mm0, %0 "
          : "=m"(diff) :);
        //cerr << "Diff=" << hex << diff << "\n";
      } break;
      default:
        RangeBufferAccess2dC<ByteT> subImg(img, srect);
        if(cols < 8) {// 1 - 7
          for(BufferAccess2dIter2C<ByteT, ByteT> it(imgTemplate, imgTemplate.range(1), subImg, subImg.range(1)); it; it++)
            diff += std::abs((IntT)it.data<0>() - (IntT)it.data<1>());
        } else if(cols < 16) {// 9 - 15

          BufferAccess2dIter2C<ByteT, ByteT> it(imgTemplate, imgTemplate.range(1), subImg, subImg.range(1));
          __asm__ volatile("\n\t pxor       %%mm7, %%mm7 "
                           "\n\t pxor       %%mm6, %%mm6 "
                           : : "m"(cols));// Dummy arg to fix bug in gcc 2.95.3
          int diff1 = 0;
          while(it) {
            __asm__ volatile("\n\t movq       %0,    %%mm0             # grab 1st half of row from bptr "
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
                             : : "m"(it.data<0>()), "m"(it.data<1>()));
            it.NextCol(8);// Skip ones we've added.
            do {
              diff1 += std::abs((IntT)it.data<0>() - (IntT)it.data<1>());
            } while(it.next());
          }

          __asm__ volatile("\n\t movq       %%mm7, %%mm0 "
                           "\n\t punpcklwd  %%mm6, %%mm7 "
                           "\n\t punpckhwd  %%mm6, %%mm0 "
                           "\n\t paddd      %%mm7, %%mm0 "
                           "\n\t movq       %%mm0, %%mm1 "
                           "\n\t psrlq      $32,   %%mm1 "
                           "\n\t paddd      %%mm1, %%mm0 "// Don't actually use the upper part of the sum.
                           "\n\t movd       %%mm0, %0 "
                           : "=m"(diff) :);
          diff += diff1;
        } else {// 17 and upwards.

          BufferAccess2dIter2C<ByteT, ByteT> it(imgTemplate, imgTemplate.range(1), subImg, subImg.range(1));
          __asm__ volatile("\n\t pxor       %%mm7, %%mm7 "
                           "\n\t pxor       %%mm6, %%mm6 "
                           : : "m"(cols));// Dummy arg to fix bug in gcc 2.95.3
          int diff1 = 0;
          while(it) {
            __asm__ volatile("\n\t movq       %0,    %%mm0             # grab 1st half of row from bptr "
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
                             : : "m"(it.data<0>()), "m"(it.data<1>()), "m"(*(&(it.data<0>()) + 8)), "m"(*(&(it.data<1>()) + 8)));
            it.NextCol(16);// Skip ones we've added.
            do {
              diff1 += std::abs((IntT)it.data<0>() - (IntT)it.data<1>());
            } while(it.next());
          }

          __asm__ volatile("\n\t movq       %%mm7, %%mm0 "
                           "\n\t punpcklwd  %%mm6, %%mm7 "
                           "\n\t punpckhwd  %%mm6, %%mm0 "
                           "\n\t paddd      %%mm7, %%mm0 "
                           "\n\t movq       %%mm0, %%mm1 "
                           "\n\t psrlq      $32,   %%mm1 "
                           "\n\t paddd      %%mm1, %%mm0 "// Don't actually use the upper part of the sum.
                           "\n\t movd       %%mm0, %0 "
                           : "=m"(diff) :);
          diff += diff1;
        }
    }
    CLOSEMMX;
    return diff;
  }
#endif
}// namespace Ravl2
