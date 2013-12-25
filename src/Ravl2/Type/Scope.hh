#ifndef RAVL2_TYPE_SCOPE_HEADER
#define RAVL2_TYPE_SCOPE_HEADER

#include "Ravl2/RefCounter.hh"
#include <vector>
#include <string>


namespace Ravl2 {

  class Factory;

  namespace Types {

  //! Scope containing other entities.

  class Scope
    : public RefCounter
  {
  public:
    //! Handle to type.
    typedef Ravl2::SmartPtr<Scope> Ref;

    //! Default constructor
    Scope();

    //! Constructor
    Scope(const std::string &name);

    //! Construct from a factory.
    Scope(const Factory &factory);

    //! Access full name of scope.
    const std::string &name() const
    { return m_name; }

    //! List the contents of the scope.
    virtual void list(std::vector<Scope::Ref> &members) const;

    //! Make the scope constant, forbid any further changes.
    void make_const();

    //! Test if the scope is constant.
    bool is_const() const
    { return m_const; }


  protected:
    bool m_const;
    std::string m_name; //! Globally unique name.
    std::string m_localName; //! Unique name within its parent scope.
  };
}}

#endif
