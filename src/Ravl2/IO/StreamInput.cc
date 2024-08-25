//
// Created by charles on 25/08/24.
//


#include "Ravl2/IO/StreamInput.hh"
#include "Ravl2/Types.hh"

namespace Ravl2
{
  //! Get name of the object.
  [[nodiscard]] std::string StreamInputBase::typeName() const
  { return Ravl2::typeName(type()); }


}
