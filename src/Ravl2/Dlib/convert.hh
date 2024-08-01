//
// Created by charles galambos on 31/07/2024.
//

#pragma once

#if 0
#include "Ravl2/Geometry/Geometry.hh"
// We need to convince dlib to that there is a cblas interface defined.
// Otherwise both xtensor and dlib will try to define it.
#define CBLAS_H 1
#endif

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdouble-promotion"
#pragma GCC diagnostic ignored "-Wshadow"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#pragma GCC diagnostic ignored "-Wfloat-conversion"
#pragma GCC diagnostic ignored "-Wunused-local-typedefs"
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wpessimizing-move"
#pragma GCC diagnostic ignored "-Wold-style-cast"
#pragma GCC diagnostic ignored "-Wsuggest-override"
#if defined(__clang__)
#pragma GCC diagnostic ignored "-Wunused-but-set-variable"
#pragma GCC diagnostic ignored "-Wdollar-in-identifier-extension"
#pragma GCC diagnostic ignored "-Wcast-align"
#else
#pragma GCC diagnostic ignored "-Wduplicated-branches"
#pragma GCC diagnostic ignored "-Wuseless-cast"
#endif
#include <dlib/array2d.h>
#include <dlib/matrix.h>
#include <dlib/geometry/line.h>

#pragma GCC diagnostic pop

#include "Ravl2/Array.hh"
#include "Ravl2/Index.hh"
#include "Ravl2/Assert.hh"

// As Dlib uses template functions, we'll put this in a namespace to avoid potential conflicts.

namespace Ravl2::DLibConvert
{

  //! Returns the number of rows in the given image
  //!  ensures
  //!    - returns the number of rows in the given image
  template<typename ArrayT,typename DataT = typename ArrayT::value_type,unsigned N = ArrayT::dimensions>
  requires WindowedArray<ArrayT,DataT,N>
  long num_rows(const ArrayT& img)
  { return img.range().size()[0]; }

  //! Returns the number of columns in the given image
  //!
  //!  ensures
  //!    - returns the number of columns in the given image
  template<typename ArrayT,typename DataT = typename ArrayT::value_type,unsigned N = ArrayT::dimensions>
  requires WindowedArray<ArrayT,DataT,N>
  long num_columns(const ArrayT& img)
  { return img.range().size()[1]; }

  //! Set the number of rows and columns in the given image.
  //!
  //! requires
  //!    - rows >= 0 && cols >= 0
  //! ensures
  //!     - num_rows(#img) == rows
  //!     - num_columns(#img) == cols
  //!
  template<typename ArrayT,typename DataT = typename ArrayT::value_type,unsigned N = ArrayT::dimensions>
  requires WindowedArray<ArrayT,DataT,N>
  void set_image_size(ArrayT &img,
                long rows,
                long cols
              )
  {
    IndexRange<N> newRange({{0,rows-1},{0,cols-1}});
    if (img.range().contains(newRange)) {
      img = clip(img, newRange);
      return;
    }
    if constexpr (std::is_same_v<ArrayT,Array<DataT,N> >) {
      // I don't think set_image_size is expected to preserve the data?
      img = ArrayT(newRange);
      return;
    }
    RavlAssertMsg(false,"set_image_size: Can't increase size of access type");
    // If asserts are disabled, throw an exception
    throw std::runtime_error("set_image_size: Can't increase size of access type");
  }

  //! Address image data.
  //! dlib always has an image origin at 0,0. This shifts the image origin to 0,0
  //!
  //! ensures
  //!    - returns a non-const pointer to the pixel at row and column position 0,0
  //!      in the given image.  Or if the image has zero rows or columns in it
  //!      then this function returns NULL.
  //!    - The image lays pixels down in row major order.  However, there might
  //!      be padding at the end of each row.  The amount of padding is given by
  //!       width_step(img).

  template<typename ArrayT,typename DataT = typename ArrayT::value_type,unsigned N = ArrayT::dimensions>
  requires WindowedArray<ArrayT,DataT,N>
  void* image_data(ArrayT& img)
  { return addressOfMin(img); }

  //! Address image data.
  //! dlib always has an image origin at 0,0. This shifts the image origin to 0,0
  //!
  //! ensures
  //!    - returns a const pointer to the pixel at row and column position 0,0 in
  //!      the given image.  Or if the image has zero rows or columns in it then
  //!      this function returns NULL.
  //!    - The image lays pixels down in row major order.  However, there might
  //!      be padding at the end of each row.  The amount of padding is given by
  //!      width_step(img).
  template<typename ArrayT,typename DataT = typename ArrayT::value_type,unsigned N = ArrayT::dimensions>
  requires WindowedArray<ArrayT,DataT,N>
  const void* image_data(const ArrayT& img)
  { return addressOfMin(img); }

  //! Get the stride of the image.
  //! ensures
  //!  - returns the size of one row of the image, in bytes.  More precisely,
  //!    return a number N such that: (char*)image_data(img) + N*R == a
  //!    pointer to the first pixel in the R-th row of the image. This means
  //!    that the image must lay its pixels down in row major order.
  template<typename ArrayT,typename DataT = typename ArrayT::value_type,unsigned N = ArrayT::dimensions>
  requires WindowedArray<ArrayT,DataT,N>
  long width_step(const ArrayT& img)
  { return img.strides()[0] * sizeof(DataT); }

  //! Swaps the contents of two images.
  //! We can only swap they are the same type.  We can expand this if needed.
  //!
  //! ensures
  //!    - swaps the state of a and b
  template<typename ArrayT,typename DataT = typename ArrayT::value_type,unsigned N = ArrayT::dimensions>
  requires WindowedArray<ArrayT,DataT,N>
  void swap(
      ArrayT& a,
      ArrayT& b
   ) noexcept
  { std::swap(a,b); }

  //! A concept that defines the requirements for an image type that can be used with the functions in this file.

  template<typename ArrayT>
  concept DlibArray2d = requires(ArrayT img)
  {
    { num_rows(img) } -> std::convertible_to<long>;
    { num_columns(img) } -> std::convertible_to<long>;
    { set_image_size(img,0,0) };
    { image_data(img) } -> std::convertible_to<const void*>;
    { width_step(img) } -> std::convertible_to<long>;
  };

  //! Create a Ravl2::ArrayView from a dlib::array2d
  //! This creates a view, it is up to the user to ensure the array2d is not destroyed before the view.

  template<typename ArrayT,typename DataT = typename ArrayT::type>
  requires DlibArray2d<ArrayT>
  ArrayView<DataT,2> toArrayView(ArrayT&anArray)
  {
    IndexRange<2> indexRange({{0,num_rows(anArray)-1},{0,num_columns(anArray)-1}});
    auto step = width_step(anArray);
    if((step % sizeof(DataT)) != 0) {
      throw std::runtime_error("toArray: width_step is not a multiple of sizeof(DataT)");
    }
    return ArrayView<DataT,2>(reinterpret_cast<DataT*>(image_data(anArray)), indexRange, {step/sizeof(DataT),1});
  }

#if 0
  //! Convert to a matrix.
  //! Perhaps we should go straight to xtensor equivalent?

  template<typename DataT, long num_rows = 0, long num_cols = 0, typename mem_manager = dlib::default_memory_manager, typename layout = dlib::row_major_layout>
  requires (num_rows > 0) && (num_cols > 0)
  Matrix<DataT,num_cols,num_rows> toMatrix(const dlib::matrix<DataT,num_rows,num_cols,mem_manager,layout>& m)
  {
    Matrix<DataT,num_cols,num_rows> ret;
    for(long r = 0; r < num_rows; r++) {
      for(long c = 0; c < num_cols; c++) {
         ret(c,r) = m(r,c);
      }
    }
    return ret;
  }

  // Deal with column vector: dlib::matrix<double,0,1>
#endif

}
