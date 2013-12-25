#ifndef RAVL2_TYPE_FACTORY
#define RAVL2_TYPE_FACTORY 1

namespace Ravl2 {

  class Variant;
  class Domain;

  //! Abstract factory

  class Factory
  {
  public:
    //! Default constructor
    Factory();

    //! Name of element
    virtual const std::name &name() const;

    //! Path to current component
    virtual const std::name &path() const;

    //! Get value as variant
    virtual bool get(const std::string &name,AbstractPtr &value,Domain *domain = 0);

    //! Get value.
    template <typename DataT>
    virtual bool get(const std::string &name,DataT &value,Domain *domain = 0)
    {
      AbstractPtr vvalue;
      get(name,vvalue,domain);
      return vvalue.get(value);
    }

    //! Use component
    //! Returns true if component found.
    virtual bool useComponent(const std::string &name,Variant &var);

    //! Create new instance of component
    //! Returns true if component found and created
    virtual bool createComponent(const std::string &name,Variant &var);

    //! Use component
    template<typename DataT>
    void useComponent(const std::string &name,DataT &value,const DataT &defaultValue);

  };

}

#endif

