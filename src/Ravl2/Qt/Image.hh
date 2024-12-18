//
// Created by charles on 04/08/24.
//

#pragma once

#include <QtGui/QImage>
#include "Ravl2/Assert.hh"
#include "Ravl2/Array.hh"
#include "Ravl2/Pixel/Pixel.hh"
#include "Ravl2/Pixel/Colour.hh"
#include "Ravl2/ArrayIterZip.hh"

namespace Ravl2
{

  //! Convert a Ravl2 pixel type to a QImage format

  template <class DataT, CopyModeT copyMode = CopyModeT::Auto>
  constexpr QImage::Format toQImageFormat()
  {
    if constexpr(std::is_same_v<DataT, uint8_t> || std::is_same_v<DataT, PixelY8>) {
      return QImage::Format_Grayscale8;
    }
    if constexpr(std::is_same_v<DataT, uint16_t> || std::is_same_v<DataT, PixelY16>) {
      return QImage::Format_Grayscale16;
    }
    if constexpr(std::is_same_v<DataT, PixelRGB8>) {
      return QImage::Format_RGB888;
    }
    if constexpr(std::is_same_v<DataT, PixelRGB16>) {
      static_assert(copyMode == CopyModeT::Never, "PixelRGB16 is not supported");
      return QImage::Format_Invalid;
    }
    if constexpr(std::is_same_v<DataT, PixelRGBA8>) {
      return QImage::Format_RGBA8888;
    }
    if constexpr(std::is_same_v<DataT, PixelRGBA16>) {
      return QImage::Format_RGBA64;
    }
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    // Only available in Qt 6
    if constexpr(std::is_same_v<DataT, PixelRGBA32F>) {
      return QImage::Format_RGBA32FPx4;
    }
#endif

    throw std::runtime_error("Unknown qt pixel type");
  }

  //! Convert a QImage to an ArrayView

  template <class DataT>
  ArrayView<DataT, 2> toArrayView(const QImage &image)
  {
    assert(image.depth() == sizeof(DataT) * 8);
    assert(image.width() > 0);
    assert(image.height() > 0);
    // Const expr check if DataT is uin8_t
    if constexpr(std::is_same_v<DataT, uint8_t>, "DataT must be uint8_t") {
      if(image.format() != QImage::Format_Grayscale8)
        throw std::runtime_error("Invalid image format, must be Format_Grayscale8");
    }
    if constexpr(std::is_same_v<DataT, uint16_t>, "DataT must be uint8_t") {
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
  template <class DataT, CopyModeT copyMode = CopyModeT::Auto>
  Array<DataT, 2> toArray(const QImage &image)
  {
    assert(image.format() == QImage::Format_Grayscale8);
    assert(image.depth() == 8);
    assert(image.width() > 0);
    assert(image.height() > 0);

    if(image.format() != toQImageFormat<DataT>()) {
      if constexpr(copyMode == CopyModeT::Never) {
        throw std::runtime_error("Incompatible image format");
      }
      // We need to convert the image to the correct format
      QImage converted = image.convertToFormat(toQImageFormat<DataT>());
      return toArray<DataT, CopyModeT::Never>(converted);
    }
    // If we're always copying and we have the right type, just clone the data.
    if constexpr(copyMode == CopyModeT::Always) {
      return clone(toArrayView<DataT>(image));
    }

    // Make a shared pointer to the image data so it doesn't get deleted when the QImage goes out of scope
    IndexRange<2> rng = IndexRange<2>::fromSize(image.width(), image.height());
    auto bytesPerLine = image.bytesPerLine();
    if((size_t(bytesPerLine) % sizeof(DataT)) != 0)
      throw std::runtime_error("Invalid bytes per line, not a multiple of sizeof(T)");
    // We need to const cast the data pointer as QImage::bits() returns a const uchar *
    // Maybe we should restrict this to only allow use with 'const DataT' ?
    auto dataPtr = const_cast<DataT *>(reinterpret_cast<const DataT *>(image.bits()));
    return Array<DataT, 2>(dataPtr, rng, {int(ssize_t(bytesPerLine) / ssize_t(sizeof(DataT))), 1}, std::shared_ptr<DataT[]>(dataPtr, [img = image]([[maybe_unused]] DataT *delPtr) {}));
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
                  toQImageFormat<DataT, copyMode>());
  }

  //! Convert an Array to a QImage
  template <class DataT, CopyModeT copyMode = CopyModeT::Auto>
  QImage toQImage(const Array<DataT, 2> &array)
  {
    assert(array.stride(0) > 0);
    // Handle QImage needing bytesperline as different type depending on Qt version
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    using QtBytesPerLineT = qsizetype;
#else
    using QtBytesPerLineT = int;
#endif

    QtBytesPerLineT bytesPerLine = QtBytesPerLineT(ssize_t(array.stride(0)) * ssize_t(sizeof(DataT)));
    if((bytesPerLine % 4) != 0) {
      // QImage requires the bytes per line to be a multiple of 4
      if(copyMode == CopyModeT::Never) {
        throw std::runtime_error("Bytes per line must be a multiple of 4");
      }
      IndexRange<2> newRng = array.range();
      auto targetBytePerLine = (bytesPerLine + 3) & ~3;
      newRng[0].max() = newRng[0].min() + int((targetBytePerLine / ssize_t(sizeof(DataT))) - 1);
      assert(newRng[0].size() * ssize_t(sizeof(DataT)) % 4 == 0);
      // Adjust the range to match the new bytes per line
      Array<DataT, 2> newArray(newRng);
      copy(array, clipUnsafe(newArray, array.range()));
      // Check we've achieved the correct bytes per line
      assert((newArray.stride(0) * ssize_t(sizeof(DataT))) % 4 == 0);
      return toQImage<DataT, CopyModeT::Never>(newArray);
    }
    return QImage(reinterpret_cast<uint8_t *>(addressOfMin(array)),
                  array.range(0).size(), array.range(1).size(),
                  bytesPerLine,
                  toQImageFormat<DataT, copyMode>());
  }

  //! Declare some common instantiations
  extern template QImage toQImage<uint8_t>(const Array<uint8_t, 2> &array);
  extern template QImage toQImage<uint16_t>(const Array<uint16_t, 2> &array);
  extern template QImage toQImage<PixelRGB8>(const Array<PixelRGB8, 2> &array);
  extern template QImage toQImage<PixelRGBA8>(const Array<PixelRGBA8, 2> &array);
  extern template QImage toQImage<PixelRGBA32F>(const Array<PixelRGBA32F, 2> &array);

  extern template Array<uint8_t, 2> toArray<uint8_t>(const QImage &image);
  extern template Array<uint16_t, 2> toArray<uint16_t>(const QImage &image);
  extern template Array<PixelRGB8, 2> toArray<PixelRGB8>(const QImage &image);
  extern template Array<PixelRGBA8, 2> toArray<PixelRGBA8>(const QImage &image);

}// namespace Ravl2
