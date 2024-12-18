
// Test program to bench mark the relative speed of different ways of accessing
// elements in a 2D array.  The test is to convolve a 128x128 image with a 16x16
// kernel.  The kernel is a simple square with the value of the element being
// the square of the difference between the row and column index.  The image is

#include <chrono>
#include <iostream>

// Check if we're using an x86 processor.
#if defined(__i386__) || defined(_M_IX86) || defined(__x86_64__) || defined(_M_X64)
#include <emmintrin.h>
#define RAVL2_USE_SSE 1
#else
#define RAVL2_USE_SSE 0
#endif

#include "Ravl2/Geometry/Geometry.hh"

#include "Ravl2/Types.hh"
#include "Ravl2/Array.hh"
#include "Ravl2/ScanWindow.hh"
#include "Ravl2/ArrayIterZip.hh"

using std::chrono::steady_clock;
using std::chrono::duration;
using std::chrono::duration_cast;

int generateTestData();
int testPlainAccess();


int main(int nargs,char **argv)
{

  testPlainAccess();

  return 0;
}


#if RAVL2_USE_SSE
using Ravl2::is16ByteAligned;
void SSEConvolveKernelF(const float *vi, // Scanned image, probably not aligned.
                        const float *vk, // Kernel, expected to be aligned.
                        size_t rows,
                        size_t cols,
                        int byteStride,
                        float *result
                       )
{
  //std::cerr << "Rows=" << rows << " Cols=" << cols << " vk=" << std::hex << (void*) vk << " vi=" << (void*) vi << " Stride=" << byteStride << std::dec <<"\n";
  __m128 sum = _mm_setzero_ps ();
  const size_t cols4 = cols >> 2;

  if(is16ByteAligned(vk) && ((cols & 0x3) == 0)) {
    // Kernel is byte aligned.
    for(size_t i = rows; i > 0; i--) {
      const float *vir = vi; // Image row.
      if(is16ByteAligned(vir)) {
        for(size_t j = cols4; j > 0; j--) {
          sum = _mm_add_ps(sum,_mm_mul_ps(_mm_load_ps(vk),_mm_load_ps(vir)));
          vk += 4;
          vir += 4;
        }
      } else {
        for(size_t j = cols4; j > 0; j--) {
          sum = _mm_add_ps(sum,_mm_mul_ps(_mm_load_ps(vk),_mm_loadu_ps(vir)));
          vk += 4;
          vir += 4;
        }
      }

      // Add stride bytes.
      vi = reinterpret_cast<const float *>(reinterpret_cast<const char *>(vi) + byteStride);
    }

  } else {
    // Kernel is not byte aligned.
    float remainder = 0;
    for(size_t i = rows; i > 0; i--) {
      const float *vir = vi; // Image row.
      if(is16ByteAligned(vir)) {
        for(size_t j = cols4; j > 0; j--) {
          sum = _mm_add_ps(sum,_mm_mul_ps(_mm_loadu_ps(vk),_mm_load_ps(vir)));
          vk += 4;
          vir += 4;
        }
      } else {
        for(size_t j = cols4; j > 0; j--) {
          sum = _mm_add_ps(sum,_mm_mul_ps(_mm_loadu_ps(vk),_mm_loadu_ps(vir)));
          vk += 4;
          vir += 4;
        }
      }

      //finish the row
      for(int j = cols & 0x3; j > 0; j--) {
        remainder += *vk * *vir;
        vk++;
        vir++;
      }

      // Add stride bytes.
      vi = reinterpret_cast<const float *>(reinterpret_cast<const char *>(vi) + byteStride);
    }
    sum = _mm_add_ps(sum,_mm_load_ss(&remainder));
  }

  sum = _mm_add_ps(sum,_mm_shuffle_ps(sum,sum, _MM_SHUFFLE(2,3,0,1)));
  sum = _mm_add_ps(sum,_mm_shuffle_ps(sum,sum, _MM_SHUFFLE(1,0,3,2)));

  _mm_store_ss(result,sum);
}
#endif

void ConvolveKernelPtr(const float *matrix,
		       const float *kernel,
		       size_t rows, size_t cols,
		       int matrixStride, float *result)
{
  float ret = 0;
  const float *vi = matrix;
  const float *vk = kernel;
  for(size_t i = 0;i < rows;i++) {
    const float *vir = vi;
    for(size_t j = 0;j < cols;j++)
      ret += *(vk++) * *(vir++);
    vi += matrixStride;
  }
  *result = ret;
}

Ravl2::Array<float,2> ConvolveKernelIndexN(const Ravl2::Array<float,2> &matrix,
                                           const Ravl2::Array<float,2> &kernel)
{
  Ravl2::IndexRange<2> scanRange = matrix.range().shrink(kernel.range());
  Ravl2::Array<float,2> result(scanRange);
  for(auto si : scanRange) {
    float sum = 0;
    for(auto kr : kernel.range())
      sum += kernel[kr] * matrix[kr + si];
    result[si] = sum;
  }
  return result;
}

Ravl2::Array<float,2> ConvolveKernelView(const Ravl2::Array<float,2> &matrix,
                                         const Ravl2::Array<float,2> &kernel)
{
  Ravl2::IndexRange<2> scanRange = matrix.range().shrink(kernel.range());
  Ravl2::Array<float,2> result(scanRange);
  for(auto si : scanRange) {
    Ravl2::IndexRange<2> rng = kernel.range() + si;
    auto view = clip(matrix,rng);
    float sum = 0;
    for(auto it = Ravl2::zip(kernel,view);it.valid();) {
      do {
	sum += it.data<0>() * it.data<1>();
      } while(it.next());
    }
    result[si] = sum;
  }
  return result;
}

Ravl2::Array<float,2> ConvolveKernelScanZip(const Ravl2::Array<float,2> &matrix,
                                            const Ravl2::Array<float,2> &kernel)
{
  Ravl2::ScanWindow<float,2> scan(matrix, kernel.range());
  Ravl2::Array<float,2> result(scan.scanArea());
  //auto kernelEnd = kernel.end();
  auto scanIter = scan.scanArea().begin();
  for(;scan.valid();++scan,++scanIter) {
    float sum = 0;
    auto window = scan.window();
    for(auto it = Ravl2::zip(kernel,window);it.valid();++it) {
      sum += it.data<0>() * it.data<1>();
    }
    result[*scanIter] = sum;
  }
  return result;
}

Ravl2::Array<float,2> ConvolveKernelScanZipRow(const Ravl2::Array<float,2> &matrix,
                                            const Ravl2::Array<float,2> &kernel)
{
  Ravl2::ScanWindow<float,2> scan(matrix, kernel.range());
  Ravl2::Array<float,2> result(scan.scanArea());
  //auto kernelEnd = kernel.end();
  auto scanIter = scan.scanArea().begin();
  for(;scan.valid();) {
    do {
      float sum = 0;
      auto window = scan.window();
      for(auto it = Ravl2::zip(kernel, window); it.valid();) {
	// This is a bit faster than the for loop.
	do {
	  sum += it.data<0>() * it.data<1>();
	} while(it.next());
      }
      result[*scanIter] = sum;
      ++scanIter;
    } while( scan.next() );
  }
  return result;
}

Ravl2::Array<float,2> ConvolveKernelScanIndex(const Ravl2::Array<float,2> &matrix,
                                         const Ravl2::Array<float,2> &kernel)
{
  Ravl2::ScanWindow<float,2> scan(matrix, kernel.range());
  Ravl2::Array<float,2> result(scan.scanArea());
  //auto kernelEnd = kernel.end();
  auto scanIter = scan.scanArea().begin();
  for(;scan.valid();++scan,++scanIter) {
    float sum = 0;
    auto window = scan.window();
    for(auto kr : kernel.range()[0]) {
      for(auto kc : kernel.range()[1]) {
        sum += kernel[kr][kc] * window[kr][kc];
      }
    }
    result[*scanIter] = sum;
  }
  return result;
}

Ravl2::Array<float,2> ConvolveKernelIndex1(const Ravl2::Array<float,2> &matrix,
                                           const Ravl2::Array<float,2> &kernel)
{
  Ravl2::IndexRange<2> scanRange = matrix.range().shrink(kernel.range());
  Ravl2::Array<float,2> result(scanRange);
  for(auto sr : scanRange[0]) {
    for(auto sc : scanRange[1]) {
      float sum = 0;
      for(auto kr : kernel.range()[0]) {
        for(auto kc : kernel.range()[1]) {
          sum += kernel[kr][kc] * matrix[kr + sr][kc + sc];
        }
      }
      result[sr][sc] = sum;
    }
  }
  return result;
}





Ravl2::Array<float,2> ConvolveKernelPtr(const Ravl2::Array<float,2> &matrix,
                                        const Ravl2::Array<float,2> &kernel)
{
  Ravl2::IndexRange<2> scanRange = matrix.range().shrink(kernel.range());
  Ravl2::Array<float,2> result(scanRange);
  for(auto sr : scanRange[0]) {
    for(auto sc : scanRange[1]) {
      ConvolveKernelPtr(&matrix[sr-kernel.range()[0].min()][sc-kernel.range()[1].min()],
                        &kernel[kernel.range()[0].min()][kernel.range()[1].min()],
                        kernel.range()[0].size(),
                        kernel.range()[1].size(),
                        matrix.stride(0),
                        &result[sr][sc]);
    }
  }
  return result;
}

#if RAVL2_USE_SSE
Ravl2::Array<float,2> ConvolveKernelSSE(const Ravl2::Array<float,2> &matrix,
                                        const Ravl2::Array<float,2> &kernel)
{
  Ravl2::IndexRange<2> scanRange = matrix.range().shrink(kernel.range());
  Ravl2::Array<float,2> result(scanRange);
  for(auto sr : scanRange[0]) {
    for(auto sc : scanRange[1]) {
      SSEConvolveKernelF(&matrix[sr-kernel.range()[0].min()][sc-kernel.range()[1].min()],
                        &kernel[kernel.range()[0].min()][kernel.range()[1].min()],
                        kernel.range()[0].size(),
                        kernel.range()[1].size(),
                        matrix.stride(0) * sizeof(float),
                        &result[sr][sc]);
    }
  }
  return result;
}
#endif


int generateTestData(Ravl2::Array<float,2> &matrix, Ravl2::Array<float,2> &kernel)
{
  matrix = Ravl2::Array<float,2> {128, 128};
  kernel = Ravl2::Array<float,2> {16, 16};

  for(auto r : matrix.range()[0])
    for(auto c : matrix.range()[1])
      matrix[r][c] = (r-c) * (r-c);

  for(auto r : kernel.range()[0])
    for(auto c : kernel.range()[1])
      kernel[r][c] = (r-c) * (r-c);

  return 0;
}



float sumElem(const Ravl2::Array<float,2> &array) {
  float sum = 0;
  for(auto x : array) {
    sum += x;
  }
  return sum;
}

float sumElemInd(const Ravl2::Array<float,2> &array)
{
  float sum = 0;
  for(auto ind : array.range())
    sum += array[ind];
  return sum;
}

float sumElemNext(const Ravl2::Array<float,2> &array)
{
  float sum = 0;
  for(auto it= array.begin();it.valid();) {
    do {
      sum += *it;
    } while(it.next());
  }
  return sum;
}


template<typename DataT>
float sumElemX(const DataT &array) {
  float sum = 0;
  for(auto r = 0; r < array.shape(0); r++)
    for(auto c = 0; c < array.shape(1); c++)
	sum += array(r,c);
  return sum;
}


int testPlainAccess()
{
  Ravl2::Array<float,2> matrix;
  Ravl2::Array<float,2> kernel;
  generateTestData(matrix,kernel);


  {
    steady_clock::time_point t1 = steady_clock::now();

    float theSum = 0;
    for(int i = 0;i < 10000;i++) {
      theSum += sumElem(matrix);
      Ravl2::doNothing(); // Prevent optimization.
    }
    steady_clock::time_point t2 = steady_clock::now();
    duration<double> time_span = duration_cast<duration<double>>(t2 - t1);
    std::cout << "sumElem  took " << time_span.count() << " seconds  to sum " << theSum << std::endl;
  }

  {
    steady_clock::time_point t1 = steady_clock::now();

    float theSum = 0;
    for(int i = 0;i < 10000;i++) {
      theSum += sumElemInd(matrix);
      Ravl2::doNothing(); // Prevent optimization.
    }
    steady_clock::time_point t2 = steady_clock::now();
    duration<double> time_span = duration_cast<duration<double>>(t2 - t1);
    std::cout << "sumElemInd  took " << time_span.count() << " seconds  to sum " << theSum << std::endl;
  }

  {
    steady_clock::time_point t1 = steady_clock::now();

    float theSum = 0;
    for(int i = 0;i < 10000;i++) {
      theSum += sumElemNext(matrix);
      Ravl2::doNothing();  // Prevent optimization.
    }
    steady_clock::time_point t2 = steady_clock::now();
    duration<double> time_span = duration_cast<duration<double>>(t2 - t1);
    std::cout << "sumElemNext  took " << time_span.count() << " seconds  to sum " << theSum << std::endl;
  }


  std::cout << "\n";

  {
    steady_clock::time_point t1 = steady_clock::now();

    float theSum = 0;
    for(int i = 0;i < 100;i++) {
      Ravl2::Array<float,2> result = ConvolveKernelIndexN(matrix, kernel);
      theSum += sumElem(result);
    }
    steady_clock::time_point t2 = steady_clock::now();
    duration<double> time_span = duration_cast<duration<double>>(t2 - t1);
    std::cout << "IndexN  took " << time_span.count() << " seconds  to sum " << theSum << std::endl;
  }

  {
    steady_clock::time_point t1 = steady_clock::now();

    float theSum = 0;
    for(int i = 0;i < 100;i++) {
      Ravl2::Array<float,2> result = ConvolveKernelView(matrix, kernel);
      theSum += sumElem(result);
    }
    steady_clock::time_point t2 = steady_clock::now();
    duration<double> time_span = duration_cast<duration<double>>(t2 - t1);
    std::cout << "View    took " << time_span.count() << " seconds  to sum " << theSum << std::endl;
  }

  {
    steady_clock::time_point t1 = steady_clock::now();

    float theSum = 0;
    for(int i = 0;i < 100;i++) {
      Ravl2::Array<float,2> result = ConvolveKernelScanZip(matrix, kernel);
      theSum += sumElem(result);
    }
    steady_clock::time_point t2 = steady_clock::now();
    duration<double> time_span = duration_cast<duration<double>>(t2 - t1);
    std::cout << "ScanZip    took " << time_span.count() << " seconds  to sum " << theSum << std::endl;
  }

  {
    steady_clock::time_point t1 = steady_clock::now();

    float theSum = 0;
    for(int i = 0;i < 100;i++) {
      Ravl2::Array<float,2> result = ConvolveKernelScanZipRow(matrix, kernel);
      theSum += sumElem(result);
    }
    steady_clock::time_point t2 = steady_clock::now();
    duration<double> time_span = duration_cast<duration<double>>(t2 - t1);
    std::cout << "ScanZipRow took " << time_span.count() << " seconds  to sum " << theSum << std::endl;
  }

  {
    steady_clock::time_point t1 = steady_clock::now();

    float theSum = 0;
    for(int i = 0;i < 100;i++) {
      Ravl2::Array<float,2> result =   ConvolveKernelScanIndex(matrix, kernel);
      theSum += sumElem(result);
    }
    steady_clock::time_point t2 = steady_clock::now();
    duration<double> time_span = duration_cast<duration<double>>(t2 - t1);
    std::cout << "ScanInd took " << time_span.count() << " seconds  to sum " << theSum << std::endl;
  }


  {
    steady_clock::time_point t1 = steady_clock::now();
    float theSum = 0;
    for(int i = 0;i < 100;i++) {
      Ravl2::Array<float,2> result = ConvolveKernelIndex1(matrix, kernel);
      theSum += sumElem(result);
    }
    steady_clock::time_point t2 = steady_clock::now();
    duration<double> time_span = duration_cast<duration<double>>(t2 - t1);
    std::cout << "Index1  took " << time_span.count() << " seconds  to sum " << theSum << std::endl;
  }

  {
    steady_clock::time_point t1 = steady_clock::now();
    float theSum = 0;
    for(int i = 0;i < 100;i++) {
      Ravl2::Array<float,2> result = ConvolveKernelPtr(matrix, kernel);
      theSum += sumElem(result);
    }
    steady_clock::time_point t2 = steady_clock::now();
    duration<double> time_span = duration_cast<duration<double>>(t2 - t1);
    std::cout << "Pointer took " << time_span.count() << " seconds  to sum " << theSum << std::endl;
  }

#if RAVL2_USE_SSE
  {
    steady_clock::time_point t1 = steady_clock::now();
    float theSum = 0;
    for(int i = 0;i < 100;i++) {
      Ravl2::Array<float,2> result = ConvolveKernelSSE(matrix, kernel);
      theSum += sumElem(result);
    }
    steady_clock::time_point t2 = steady_clock::now();
    duration<double> time_span = duration_cast<duration<double>>(t2 - t1);
    std::cout << "SSE     took " << time_span.count() << " seconds  to sum " << theSum << std::endl;
  }
#endif

  return 0;
}
