
#include "Ravl2/Dlib/Image.hh"

namespace Ravl2
{

  using namespace DLibIO;

  // Check that the DlibArray2d concept is satisfied by dlib::array2d.
  static_assert(DlibArray2d<dlib::array2d<float>>, "dlib::array2d<float> does not satisfy DlibArray2d concept");
  static_assert(DlibArray2d<Array<float, 2>>, "Array<float,2> does not satisfy DlibArray2d concept");

  //! Create a dlib RGB image from a Ravl2::Array
  //! This creates a view, it is up to the user to ensure the array2d is not destroyed before the view.
  dlib::array2d<dlib::rgb_pixel> toDlib(const ArrayView<PixelRGB8, 2> &anArray)
  {
    dlib::array2d<dlib::rgb_pixel> ret(anArray.range().size()[0], anArray.range().size()[1]);
    for(int r = 0; r < anArray.range().size()[0]; r++) {
      for(int c = 0; c < anArray.range().size()[1]; c++) {
        ret[r][c].red = anArray[r][c].get<ImageChannel::Red>();
        ret[r][c].green = anArray[r][c].get<ImageChannel::Green>();
        ret[r][c].blue = anArray[r][c].get<ImageChannel::Blue>();
      }
    }
    return ret;
  }

  //! Create a dlib grey image from a Ravl2::Array
  [[nodiscard]] dlib::array2d<uint8_t> toDlib(const ArrayView<Ravl2::PixelY8, 2> &anArray)
  {
    dlib::array2d<uint8_t> ret(anArray.range().size()[0], anArray.range().size()[1]);
    for(int r = 0; r < anArray.range().size()[0]; r++) {
      for(int c = 0; c < anArray.range().size()[1]; c++) {
        ret[r][c] = anArray[r][c].get<ImageChannel::Luminance>();
      }
    }
    return ret;
  }

  //! Create a dlib grey image from a Ravl2::Array
  dlib::array2d<uint8_t> toDlib(const ArrayView<uint8_t, 2> &anArray)
  {
    dlib::array2d<uint8_t> ret(anArray.range().size()[0], anArray.range().size()[1]);
    for(int r = 0; r < anArray.range().size()[0]; r++) {
      for(int c = 0; c < anArray.range().size()[1]; c++) {
        ret[r][c] = anArray[r][c];
      }
    }
    return ret;
  }

  //! Create a dlib grey image from a Ravl2::Array
  [[nodiscard]] dlib::array2d<uint16_t> toDlib(const ArrayView<uint16_t, 2> &anArray)
  {
    dlib::array2d<uint16_t> ret(anArray.range().size()[0], anArray.range().size()[1]);
    for(int r = 0; r < anArray.range().size()[0]; r++) {
      for(int c = 0; c < anArray.range().size()[1]; c++) {
        ret[r][c] = anArray[r][c];
      }
    }
    return ret;
  }

}// namespace Ravl2