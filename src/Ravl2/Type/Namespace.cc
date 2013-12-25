
#include "Ravl2/Type/Namespace.hh"
#include <iostream>
//#include "Ravl/TypeName.hh"

namespace Ravl2 { namespace Types {

  Namespace::Namespace(const std::string &name)
   : Scope(name)
  {}

  //! Construct from a factory.
  Namespace::Namespace(const Factory &factory)
   : Scope(factory)
  {
    //factory.UseComponentGroup("Members",m_contents);
  }

  //! List the contents of the scope.
  void Namespace::list(std::vector<Scope::Ref> &members) const
  {
    std::lock_guard<std::mutex> lock(m_access);
    members = m_contents;
  }

  //! Dump namespace contents to stream
  void Namespace::dump(std::ostream &strm) const {
    std::lock_guard<std::mutex> lock(m_access);

    strm << "Namespace '" << name() << "' : \n";
    for(auto &i : m_contents) {
      strm << " '" << i->name() << "\n";
    }
  }

  //! Add a new scope to this one.
  bool Namespace::add(const Scope &scope) {
    std::lock_guard<std::mutex> lock(m_access);
    for(unsigned i = 0;i < m_contents.size();i++) {
      if(m_contents[i]->name() == scope.name()) {
#if 0
        RavlError("Name %s already registered ",scope.Name().c_str());
        RavlIssueError("Duplicate name registration");
#endif
        return false;
      }
    }
    //RavlDebug("Registering %s ",scope.Name().c_str());
    m_contents.push_back(&scope);
    return true;
  }

  //! Lookup a named member of the scope.
  bool Namespace::lookup(const std::string &name,Scope::Ref &result) const
  {
    std::lock_guard<std::mutex> lock(m_access);
    for(unsigned i = 0;i < m_contents.size();i++) {
      if(m_contents[i]->name() == name) {
        result = m_contents[i];
        return true;
      }
    }
    return false;
  }

  //! Check namespace contains a type.
  bool Namespace::has(const std::string &name) const
  {
    std::lock_guard<std::mutex> lock(m_access);
    Scope::Ref result;
    return lookup(name,result);
  }

#if 0
  //! Register an operator.
  void Namespace::RegisterOperator(OperatorTypeT operationType,
                                    const TypeC &t1,
                                    const TypeC &t2,
                                    const Operator21C &func)
  {
    OperatorKeyC ok(operationType,t1,t2);
    RWLockHoldC lock(m_access,RavlN::RWLOCK_WRITE);
    if(m_key2operator.IsElm(ok)) {
      RavlWarning("Overriding existing operator. '%s'  t1:%s t2:%s ",
          RavlN::StringOf(operationType).c_str(),t1.Name().c_str(),t2.Name().c_str());
    }
    m_key2operator[ok] = &func;
  }

  //! Find an operator.
  const Operator21C *Namespace::FindOperator(OperatorTypeT operationType,const TypeC &t1,const TypeC &t2) const
  {
    OperatorKeyC ok(operationType,t1,t2);
    RWLockHoldC lock(m_access,RavlN::RWLOCK_READONLY);
    const Operator21C::RefT *ref = m_key2operator.Lookup(ok);
    if(ref == 0)
      return 0;
    return ref->BodyPtr();
  }
#endif

  //! Write namespace to stream
  std::ostream &operator<<(std::ostream &strm,const Namespace &obj)
  {
    obj.dump(strm);
    return strm;
  }

  //static XMLFactoryRegisterConvertC<Namespace,Scope> g_registerNamespace("RavlN::TypeC::Namespace");

}}
