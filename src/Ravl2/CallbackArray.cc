// (C) 2020 React AI Ltd.
// Available under the MIT license

#include "Ravl2/CallbackArray.hh"

namespace Ravl2
{

  // -----------------------------------------------------

  void CallbackSet::removeAll()
  {
    while(!m_callbacks.empty()) {
      m_callbacks.back().remove();
      m_callbacks.pop_back();
    }
  }

}// namespace ReasonN
