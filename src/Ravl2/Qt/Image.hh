//
// Created by charles on 04/08/24.
//

#pragma once

#include <QtGui/QImage>
#include "Ravl2/Array.hh"


namespace Ravl2
{

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
  template<class DataT>
  Array<DataT, 2> toArray(const QImage &image)
  {
    assert(image.format() == QImage::Format_Grayscale8);
    assert(image.depth() == 8);
    assert(image.width() > 0);
    assert(image.height() > 0);
    IndexRange<2> rng = IndexRange<2>::fromSize(image.width(), image.height());
    auto bytesPerLine = image.bytesPerLine();
    if((bytesPerLine % sizeof(DataT)) != 0)
      throw std::runtime_error("Invalid bytes per line, not a multiple of sizeof(T)");
    auto dataPtr = reinterpret_cast<const DataT *>(image.bits());
    return Array<DataT, 2>(dataPtr,rng,{bytesPerLine / sizeof(DataT),1},std::shared_ptr<DataT[]>(dataPtr, [img = image]([[maybe_unused]] DataT *delPtr) {}));
  }


  //! Convert an ArrayView to a QImage
  template<class T, unsigned N>
  QImage toQImage(const ArrayView<T, N> &array)
  {
    assert(array.size() > 0);
    assert(array.size() <= std::numeric_limits<int>::max());
    assert(array.size() == array.shape().size());
    assert(array.shape().size() == 2);
    //QImage::QImage(uchar *data, int width, int height, qsizetype bytesPerLine, QImage::Format format, QImageCleanupFunction cleanupFunction = nullptr, void *cleanupInfo = nullptr)


    QImage image(array.shape()[0], array.shape()[1], QImage::Format_Grayscale8);

    return image;
  }

}
