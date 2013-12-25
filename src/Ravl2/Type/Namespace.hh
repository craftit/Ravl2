#ifndef RAVL2_TYPE_NAMESPACE_HEADER
#define RAVL2_TYPE_NAMESPACE_HEADER

#include <mutex>
#include "Ravl2/Type/Scope.hh"

namespace Ravl2 { namespace Types {

  //! Namespace holding a set of types.

  class Namespace
   : public Scope
  {
  public:
    //! Construct with name
    Namespace(const std::string &name);

    //! Construct from a factory.
    Namespace(const Factory &factory);

    //! List the contents of the scope.
    virtual void list(std::vector<Scope::Ref> &members) const;

    //! Add a new scope to this one.
    virtual bool add(const Scope &scope);

    //! Lookup a named member of the scope.
    virtual bool lookup(const std::string &name,Scope::Ref &result) const;

    //! Lookup a named member of the scope.
    template<class DataT>
    bool lookup(const std::string &name,Ravl2::SmartPtr<DataT> &result) const {
      Scope::Ref theScope;
#if 0
      if(!lookup(name,theScope))
        return false;
      result = dynamic_cast<const DataT *>(theScope.pointer());
      if(!result.is_valid()) {
        RavlWarning("Lookup of '%s' in namespace '%s' gave type '%s', expected '%s' ",
            name.data(),
            Name().data(),
            RavlN::TypeName(typeid(*theScope)),
            RavlN::TypeName(typeid(DataT)));
      }
      return result.IsValid();
#endif
    }

    //! Check namespace contains a type.
    bool has(const std::string &name) const;

#if 0
    //! Register an operator.
    void register_operator(OperatorTypeT operationType,const TypeC &t1,const TypeC &t2,const Operator21C &func);

    //! Find an operator.
    const Operator21C *find_operator(OperatorTypeT operationType,const TypeC &t1,const TypeC &t2) const;
#endif

    //! Dump namespace contents to stream
    void dump(std::ostream &strm) const;

    //! Handle to a name space.
    typedef Ravl2::SmartPtr<Namespace> Ref;

  protected:
    mutable std::mutex m_access;
    std::vector<Scope::Ref> m_contents;
    //RavlN::HashC<OperatorKeyC,Operator21C::RefT> m_key2operator;
  };

  //! Write namespace to stream
  std::ostream &operator<<(std::ostream &strm,const Namespace &obj);

}}

#endif
