
#include <chrono>
#include <iostream>
#include <emmintrin.h>


#include "Ravl2/ArrayAccess.hh"

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

template<typename DataT>
static inline bool Is16ByteAligned(const DataT *data)
{ return (((unsigned long int) data) & 0xf) == 0; }


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

  if(Is16ByteAligned(vk) && ((cols & 0x3) == 0)) {
    // Kernel is byte aligned.
    for(size_t i = rows; i > 0; i--) {
      const float *vir = vi; // Image row.
      if(Is16ByteAligned(vir)) {
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
      if(Is16ByteAligned(vir)) {
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

void ConvolveKernelPtr(const float *matrix,
                            const float *kernel,
                            size_t rows,size_t cols,
                            int matrixByteStride,float *result)
{
  register float ret = 0;
  const float *vi = matrix;
  const float *vk = kernel;
  for(size_t i = 0;i < rows;i++) {
    const float *vir = vi;
    for(size_t j = 0;j < cols;j++)
      ret += *(vk++) * *(vir++);
    vi = reinterpret_cast<const float *>(reinterpret_cast<const char *>(vi) + matrixByteStride);
  }
  *result = ret;
}

Ravl2::ArrayAccess<float,2> ConvolveKernel1(const Ravl2::ArrayAccess<float,2> &matrix,
                                           const Ravl2::ArrayAccess<float,2> &kernel)
{
  Ravl2::IndexRange<2> scanRange = matrix.range().shrink(kernel.range());
  Ravl2::ArrayAccess<float,2> result(scanRange);
  for(auto si : scanRange) {
    float sum = 0;
    for(auto kr : kernel.range())
      sum += kernel[kr] * matrix[kr + si];
    result[si] = sum;
  }
  return result;
}

Ravl2::ArrayAccess<float,2> ConvolveKernel2(const Ravl2::ArrayAccess<float,2> &matrix,
                                           const Ravl2::ArrayAccess<float,2> &kernel)
{
  Ravl2::IndexRange<2> scanRange = matrix.range().shrink(kernel.range());
  Ravl2::ArrayAccess<float,2> result(scanRange);
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

Ravl2::ArrayAccess<float,2> ConvolveKernel3(const Ravl2::ArrayAccess<float,2> &matrix,
                                           const Ravl2::ArrayAccess<float,2> &kernel)
{
  Ravl2::IndexRange<2> scanRange = matrix.range().shrink(kernel.range());
  Ravl2::ArrayAccess<float,2> result(scanRange);
  for(auto sr : scanRange[0]) {
    for(auto sc : scanRange[1]) {
      ConvolveKernelPtr(&matrix[sr-kernel.range()[0].min()][sc-kernel.range()[1].min()],
                        &kernel[kernel.range()[0].min()][kernel.range()[1].min()],
                        kernel.range()[0].size(),
                        kernel.range()[1].size(),
                        matrix.stride(0) * sizeof(float),
                        &result[sr][sc]);
    }
  }
  return result;
}

Ravl2::ArrayAccess<float,2> ConvolveKernel4(const Ravl2::ArrayAccess<float,2> &matrix,
                                           const Ravl2::ArrayAccess<float,2> &kernel)
{
  Ravl2::IndexRange<2> scanRange = matrix.range().shrink(kernel.range());
  Ravl2::ArrayAccess<float,2> result(scanRange);
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


int generateTestData(Ravl2::ArrayAccess<float,2> &matrix,Ravl2::ArrayAccess<float,2> &kernel)
{
  matrix = Ravl2::ArrayAccess<float,2> {128,128};
  kernel = Ravl2::ArrayAccess<float,2> {16,16};

  for(auto r : matrix.range()[0])
    for(auto c : matrix.range()[1])
      matrix[r][c] = (r-c) * (r-c);

  for(auto r : kernel.range()[0])
    for(auto c : kernel.range()[1])
      kernel[r][c] = (r-c) * (r-c);

  return 0;
}

float sumElem(const Ravl2::ArrayAccess<float,2> &array) {
  float sum = 0;
  for(auto ind : array.range())
    sum += array[ind];
  return sum;
}

int testPlainAccess()
{
  Ravl2::ArrayAccess<float,2> matrix;
  Ravl2::ArrayAccess<float,2> kernel;
  generateTestData(matrix,kernel);

  {
    steady_clock::time_point t1 = steady_clock::now();

    float theSum = 0;
    for(int i = 0;i < 100;i++) {
      Ravl2::ArrayAccess<float,2> result = ConvolveKernel1(matrix,kernel);
      theSum += sumElem(result);
    }
    steady_clock::time_point t2 = steady_clock::now();
    duration<double> time_span = duration_cast<duration<double>>(t2 - t1);
    std::cout << "Index   took " << time_span.count() << " seconds  to sum " << theSum << std::endl;
  }

  {
    steady_clock::time_point t1 = steady_clock::now();
    float theSum = 0;
    for(int i = 0;i < 100;i++) {
      Ravl2::ArrayAccess<float,2> result = ConvolveKernel2(matrix,kernel);
      theSum += sumElem(result);
    }
    steady_clock::time_point t2 = steady_clock::now();
    duration<double> time_span = duration_cast<duration<double>>(t2 - t1);
    std::cout << "Element took " << time_span.count() << " seconds  to sum " << theSum << std::endl;
  }

  {
    steady_clock::time_point t1 = steady_clock::now();
    float theSum = 0;
    for(int i = 0;i < 100;i++) {
      Ravl2::ArrayAccess<float,2> result = ConvolveKernel3(matrix,kernel);
      theSum += sumElem(result);
    }
    steady_clock::time_point t2 = steady_clock::now();
    duration<double> time_span = duration_cast<duration<double>>(t2 - t1);
    std::cout << "Pointer took " << time_span.count() << " seconds  to sum " << theSum << std::endl;
  }

  {
    steady_clock::time_point t1 = steady_clock::now();
    float theSum = 0;
    for(int i = 0;i < 100;i++) {
      Ravl2::ArrayAccess<float,2> result = ConvolveKernel4(matrix,kernel);
      theSum += sumElem(result);
    }
    steady_clock::time_point t2 = steady_clock::now();
    duration<double> time_span = duration_cast<duration<double>>(t2 - t1);
    std::cout << "SSE     took " << time_span.count() << " seconds  to sum " << theSum << std::endl;
  }


  return 0;
}
