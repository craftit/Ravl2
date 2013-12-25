
#include "Ravl2/Type/Scope.hh"

namespace Ravl2 { namespace Types {

  //! Default constructor
  Scope::Scope()
   : m_const(false)
  {}

  //! Construct with just a name
  Scope::Scope(const std::string &name)
   : m_const(false),
     m_name(name),
     m_localName(name)
  {}

#if 0
  //! Construct from a factory.
  Scope::Scope(const Factory &factory)
   : m_const(false),
     m_name(factory.AttributeString("name",factory.Name()).data()),
     m_localName(factory.AttributeString("localName",factory.Name()).data())
  {}
#endif

  //! Make the scope constant, forbid any further changes.
  void Scope::make_const()
  {
    m_const = true;
  }

  //! List the contents of the scope.
  void Scope::list(std::vector<Scope::Ref> &members) const
  {}

}}
