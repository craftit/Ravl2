
#include "Ravl2/Dlib/convert.hh"

namespace Ravl2::DLibConvert
{

  // Check that the DlibArray2d concept is satisfied by dlib::array2d.
  static_assert(DlibArray2d<dlib::array2d<float>>, "dlib::array2d<float> does not satisfy DlibArray2d concept");

  static_assert(DlibArray2d<Array<float,2>>, "Array<float,2> does not satisfy DlibArray2d concept");


}