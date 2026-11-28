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

#include <array>
#include <cstring>
#include <memory>
#include <stdexcept>

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

  namespace detail
  {
    //! Rows are dimension 0 (height), columns dimension 1 (width).
    template <class DataT>
    IndexRange<2> qImageRange(const QImage &image)
    {
      return IndexRange<2>::fromSize(image.height(), image.width());
    }

    template <class DataT>
    std::ptrdiff_t qImageRowStride(const QImage &image)
    {
      const auto bytesPerLine = static_cast<std::ptrdiff_t>(image.bytesPerLine());
      if((bytesPerLine % static_cast<std::ptrdiff_t>(sizeof(DataT))) != 0)
        throw std::runtime_error("Invalid bytes per line, not a multiple of sizeof(T)");
      return bytesPerLine / static_cast<std::ptrdiff_t>(sizeof(DataT));
    }

    //! Copy any 2-D array into a new QImage that owns its data.
    template <typename ArrayT, typename DataT = typename ArrayT::value_type>
    QImage copyToQImage(const ArrayT &array)
    {
      const IndexRange<2> rng = array.range();
      QImage newImage(rng.size(1), rng.size(0), toQImageFormat<DataT, CopyModeT::Never>());
      for(int row = rng.min(0); row <= rng.max(0); ++row) {
        const auto *srcPtr = &array[{row, rng.min(1)}];
        auto *dstPtr = newImage.scanLine(row - rng.min(0));
        std::memcpy(dstPtr, srcPtr, static_cast<size_t>(rng.size(1)) * sizeof(DataT));
      }
      return newImage;
    }
  }// namespace detail

  //! View the pixels of a QImage without copying.
  //! The QImage must outlive the view, and must already be in the format matching DataT.
  template <class DataT>
  ArrayView<const DataT, 2> toArrayView(const QImage &image)
  {
    if(image.format() != toQImageFormat<DataT, CopyModeT::Never>())
      throw std::runtime_error("QImage format does not match the requested pixel type");
    return ArrayView<const DataT, 2>(reinterpret_cast<const DataT *>(image.constBits()), detail::qImageRange<DataT>(image),
                                     std::array<std::ptrdiff_t, 2> {detail::qImageRowStride<DataT>(image), 1});
  }

  //! Convert a QImage to an Array.
  //! With CopyModeT::Auto the Array shares the QImage's pixels when the format matches (keeping a reference to
  //! the QImage), and converts otherwise. CopyModeT::Always always copies; CopyModeT::Never throws if the format
  //! doesn't match.
  template <class DataT, CopyModeT copyMode = CopyModeT::Auto>
  Array<DataT, 2> toArray(const QImage &image)
  {
    if(image.format() != toQImageFormat<DataT, CopyModeT::Never>()) {
      if constexpr(copyMode == CopyModeT::Never) {
        throw std::runtime_error("Incompatible image format");
      } else {
        // Converting already makes a new image, which the result can share.
        QImage converted = image.convertToFormat(toQImageFormat<DataT, CopyModeT::Never>());
        return toArray<DataT, CopyModeT::Auto>(converted);
      }
    }
    if constexpr(copyMode == CopyModeT::Always) {
      auto view = toArrayView<DataT>(image);
      Array<DataT, 2> ret(view.range());
      copy(ret, view);
      return ret;
    } else {
      // Share the pixels without copying, keeping a (shallow, implicitly shared) QImage alive for as long as the
      // Array's buffer. The Array aliases those pixels: writing through it changes every QImage sharing them,
      // bypassing Qt's copy-on-write. Use CopyModeT::Always for an independent copy.
      QImage owner = image;
      auto *dataPtr = const_cast<DataT *>(reinterpret_cast<const DataT *>(owner.constBits()));
      const auto stride = detail::qImageRowStride<DataT>(owner);
      const auto rng = detail::qImageRange<DataT>(owner);
      return Array<DataT, 2>(dataPtr, rng, std::array<std::ptrdiff_t, 2> {stride, 1},
                             std::shared_ptr<DataT[]>(dataPtr, [img = std::move(owner)]([[maybe_unused]] DataT *delPtr) {}));
    }
  }

  //! Copy any 2-D array view into a new QImage.
  template <typename ArrayT, CopyModeT copyMode = CopyModeT::Auto, typename DataT = typename ArrayT::value_type, unsigned N = ArrayT::dimensions>
    requires WindowedArray<ArrayT, DataT, N> && (N == 2)
  QImage toQImage(const ArrayT &array)
  {
    static_assert(copyMode != CopyModeT::Never, "Only an Array can be shared with a QImage without copying");
    return detail::copyToQImage(array);
  }

  //! Convert an Array to a QImage.
  //! Unless copyMode is Always, the QImage shares the Array's pixels and keeps the Array's buffer alive.
  //! Rows whose byte length isn't a multiple of 4 are copied, as QImage requires.
  template <class DataT, CopyModeT copyMode = CopyModeT::Auto>
  QImage toQImage(const Array<DataT, 2> &array)
  {
    // Handle QImage needing bytesperline as different type depending on Qt version
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    using QtBytesPerLineT = qsizetype;
#else
    using QtBytesPerLineT = int;
#endif
    const auto stride = array.stride(0);
    const auto bytesPerLine = QtBytesPerLineT(stride * static_cast<std::ptrdiff_t>(sizeof(DataT)));
    if constexpr(copyMode == CopyModeT::Always) {
      return detail::copyToQImage(array);
    } else {
      if(stride <= 0 || (bytesPerLine % 4) != 0) {
        if constexpr(copyMode == CopyModeT::Never) {
          throw std::runtime_error("QImage needs positive strides and rows that are a multiple of 4 bytes");
        } else {
          return detail::copyToQImage(array);
        }
      }
      // Hand QImage a copy of the Array handle, released by its cleanup function.
      auto holder = std::make_unique<Array<DataT, 2>>(array);
      QImage image(reinterpret_cast<uchar *>(addressOfMin(array)), array.range(1).size(), array.range(0).size(), bytesPerLine,
                   toQImageFormat<DataT, copyMode>(),
                   [](void *info) { delete static_cast<Array<DataT, 2> *>(info); }, holder.get());
      holder.release();
      return image;
    }
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
