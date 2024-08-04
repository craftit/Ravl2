//
// Created by charles on 04/08/24.
//

#pragma once

#include <QtGui/QImage>
#include "Ravl2/Assert.hh"
#include "Ravl2/Array.hh"
#include "Ravl2/Pixel/PixelRGB.hh"
#include "Ravl2/Pixel/PixelRGBA.hh"
#include "Ravl2/ArrayIterZip.hh"

namespace Ravl2
{
  //! Define allowed copy modes for converting between QImage and ArrayView.
  //! If auto is selected data will be copied / converted if the QImage format is
  //! not compatible with the ArrayView type
  enum class CopyModeT
  {
    Never,
    Auto,
    Always
  };

  //! Convert a Ravl2 pixel type to a QImage format

  template<class DataT, CopyModeT copyMode = CopyModeT::Auto>
  constexpr inline QImage::Format toQImageFormat()
  {
    if constexpr (std::is_same_v<DataT, uint8_t>) {
      return QImage::Format_Grayscale8;
    }
    if constexpr (std::is_same_v<DataT, uint16_t>) {
      return QImage::Format_Grayscale16;
    }
    if constexpr (std::is_same_v<DataT, PixelRGB<uint8_t>>) {
      return QImage::Format_RGB888;
    }
    if constexpr (std::is_same_v<DataT, PixelRGB<uint16_t>>) {
      static_assert(copyMode == CopyModeT::Never, "PixelRGB<uint16_t> is not supported");
      return QImage::Format_Invalid;
    }
    if constexpr (std::is_same_v<DataT, PixelRGBA<uint8_t>>) {
      return QImage::Format_RGBA8888;
    }
    if constexpr (std::is_same_v<DataT, PixelRGBA<uint16_t>>) {
      return QImage::Format_RGBA64;
    }
    if constexpr (std::is_same_v<DataT, PixelRGBA<float>>) {
      return QImage::Format_RGBA32FPx4;
    }

    throw std::runtime_error("Unknown qt pixel type");
  }

  //! Convert a QImage to an ArrayView

  template<class DataT>
  ArrayView<DataT, 2> toArrayView(const QImage &image)
  {
    assert(image.depth() == sizeof(DataT) * 8);
    assert(image.width() > 0);
    assert(image.height() > 0);
    // Const expr check if DataT is uin8_t
    if constexpr (std::is_same_v<DataT, uint8_t>, "DataT must be uint8_t") {
      if(image.format() != QImage::Format_Grayscale8)
        throw std::runtime_error("Invalid image format, must be Format_Grayscale8");
    }
    if constexpr (std::is_same_v<DataT, uint16_t>, "DataT must be uint8_t") {
      if(image.format() != QImage::Format_Grayscale16)
        throw std::runtime_error("Invalid image format, must be Format_Grayscale16");
    }

    IndexRange<2> rng = IndexRange<2>::fromSize(image.width(), image.height());
    auto bytesPerLine = image.bytesPerLine();
    if((bytesPerLine % sizeof(DataT)) != 0)
      throw std::runtime_error("Invalid bytes per line, not a multiple of sizeof(T)");
    return ArrayView<DataT, 2>(reinterpret_cast<DataT *>(image.bits()), rng, {bytesPerLine / sizeof(DataT), 1});
  }

  //! Convert a QImage to an ArrayView
  template<class DataT, CopyModeT copyMode = CopyModeT::Auto>
  Array<DataT, 2> toArray(const QImage &image)
  {
    assert(image.format() == QImage::Format_Grayscale8);
    assert(image.depth() == 8);
    assert(image.width() > 0);
    assert(image.height() > 0);

    if(image.format() != toQImageFormat<DataT>()) {
      if constexpr (copyMode == CopyModeT::Never) {
	throw std::runtime_error("Incompatible image format");
      }
      // We need to convert the image to the correct format
      QImage converted = image.convertToFormat(toQImageFormat<DataT>());
      return toArray<DataT, CopyModeT::Never>(converted);
    }
    // If we're always copying and we have the right type, just clone the data.
    if constexpr (copyMode == CopyModeT::Always) {
      return clone(toArrayView<DataT>(image));
    }

    // Make a shared pointer to the image data so it doesn't get deleted when the QImage goes out of scope
    IndexRange<2> rng = IndexRange<2>::fromSize(image.width(), image.height());
    auto bytesPerLine = image.bytesPerLine();
    if((bytesPerLine % sizeof(DataT)) != 0)
      throw std::runtime_error("Invalid bytes per line, not a multiple of sizeof(T)");
    auto dataPtr = reinterpret_cast<const DataT *>(image.bits());
    return Array<DataT, 2>(dataPtr,rng,{bytesPerLine / sizeof(DataT),1},std::shared_ptr<DataT[]>(dataPtr, [img = image]([[maybe_unused]] DataT *delPtr) {}));
  }


  //! Convert an ArrayView to a QImage, this does not try and create a reference to the data
  template <typename ArrayT, CopyModeT copyMode = CopyModeT::Auto, typename DataT = typename ArrayT::value_type, unsigned N = ArrayT::dimensions>
     requires WindowedArray<ArrayT, DataT, N>
  QImage toQImage(const ArrayT &array)
  {
    assert(array.size() > 0);
    assert(array.size() <= std::numeric_limits<int>::max());
    assert(array.size() == array.shape().size());
    assert(array.shape().size() == 2);
    //QImage::QImage(uchar *data, int width, int height, qsizetype bytesPerLine, QImage::Format format, QImageCleanupFunction cleanupFunction = nullptr, void *cleanupInfo = nullptr)
    qsizetype bytesPerLine = array.range(0).size() * sizeof(DataT);
    return QImage(addressOfMin(array),
		  array.range(0).size(), array.range(1).size(),
		  bytesPerLine,
		  toQImageFormat<DataT,copyMode>());
  }

  //! Convert an Array to a QImage
  template<class DataT, CopyModeT copyMode = CopyModeT::Auto>
  QImage toQImage(const Array<DataT, 2> &array)
  {
    assert(array.stride(0) > 0);
    qsizetype bytesPerLine = ssize_t(array.stride(0)) * ssize_t(sizeof(DataT));
    if((bytesPerLine % 4) != 0) {
      // QImage requires the bytes per line to be a multiple of 4
      if(copyMode == CopyModeT::Never) {
	throw std::runtime_error("Bytes per line must be a multiple of 4");
      }
      IndexRange<2> newRng = array.range();
      auto targetBytePerLine = (bytesPerLine + 3) & ~3;
      newRng[0].max() = newRng[0].min() + int((targetBytePerLine / ssize_t(sizeof(DataT))) - 1);
      assert(newRng[0].size() * sizeof(DataT) % 4 == 0);
      // Adjust the range to match the new bytes per line
      Array<DataT, 2> newArray(newRng);
      copy(array, clip(newArray,array.range()));
      // Check we've achieved the correct bytes per line
      assert((newArray.stride(0) * sizeof(DataT)) % 4 == 0);
    }
    return QImage(reinterpret_cast<uchar *>(addressOfMin(array)),
		  array.range(0).size(), array.range(1).size(),
		  bytesPerLine,
		  toQImageFormat<DataT, copyMode>());
  }

  //! Declare some common instantiations
  extern template QImage toQImage<uint8_t>(const Array<uint8_t, 2> &array);
  extern template QImage toQImage<uint16_t>(const Array<uint16_t, 2> &array);
  extern template QImage toQImage<PixelRGB<uint8_t>>(const Array<PixelRGB<uint8_t>, 2> &array);
  extern template QImage toQImage<PixelRGBA<uint8_t>>(const Array<PixelRGBA<uint8_t>, 2> &array);
  extern template QImage toQImage<PixelRGBA<uint16_t>>(const Array<PixelRGBA<uint16_t>, 2> &array);
  extern template QImage toQImage<PixelRGBA<float>>(const Array<PixelRGBA<float>, 2> &array);


}
