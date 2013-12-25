
#include "Ravl2/RefCounterWrap.hh"

namespace Ravl2
{

  //! Access type that's been stored.
  const std::type_info &RefCounterWrapBase::stored_typeid() const
  { return typeid(void); }

  //! Access address of stored value.
  void *RefCounterWrapBase::stored_address()
  { return 0; }

  //! Access address of stored value.
  const void *RefCounterWrapBase::stored_address() const
  { return 0; }

}
