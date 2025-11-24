//
// PNG Image IO registration (libpng-backed)
//

#pragma once

namespace Ravl2
{
  //! Initialize/register the PNG Image IO plugin.
  //! This ensures the input/output registration TU is linked in when called.
  extern void initPngImageIO();
}// namespace Ravl2
