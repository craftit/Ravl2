//
// Stream position helpers shared by the input and output stream classes.
//

#pragma once

#include <ios>
#include <limits>

namespace Ravl2
{
  //! @brief Position used for "no end" / "next object" in a stream.
  //! @details std::numeric_limits<std::streampos> is not specialised (std::fpos is a class), so its max() is 0
  //! and must not be used. A function rather than a variable, so it is safe to use during static initialisation.
  [[nodiscard]] inline std::streampos streamPosEnd() noexcept
  {
    return std::streampos(std::numeric_limits<std::streamoff>::max());
  }
}// namespace Ravl2
