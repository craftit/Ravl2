#ifndef RAVL2_REFCOUNTER_WRAP
#define RAVL2_REFCOUNTER_WRAP 1

#include "Ravl2/RefCounter.hh"
#include <typeinfo>

namespace Ravl2
{
  //! Abstract pointer to type.

  class AbstractPtr
   : public SmartPtr<RefCounter>
  {
  public:
    //! Default constructor
    AbstractPtr()
    {}

    //! Construct from a pointer
    AbstractPtr(RefCounter *obj)
     : SmartPtr<RefCounter>(obj)
    {}

    //! Construct from a pointer
    AbstractPtr(const RefCounter *obj)
     : SmartPtr<RefCounter>(const_cast<RefCounter *>(obj))
    {}

    //! Default constructor
    AbstractPtr(const SmartPtr<RefCounter> &obj)
     : SmartPtr<RefCounter>(obj)
    {}

    //! Get value
    template<typename DataT>
    bool get(DataT &data);

    //! Assign to an object.
    template<typename DataT>
    AbstractPtr &operator=(const DataT &data);
  };

  //! Base class that allows access to information about the stored type.

  class RefCounterWrapBase
    : public RefCounter
  {
  public:
    //! Default constructor.
    RefCounterWrapBase()
    {}

    //! Access type that's been stored.
    virtual const std::type_info &stored_typeid() const;

    //! Access address of stored value.
    virtual void *stored_address();

    //! Access address of stored value.
    virtual const void *stored_address() const;
  };

  //! Wrap a value in a reference counter.

  template<typename DataT>
  class RefCounterWrap
   : public RefCounter
  {
  public:
    //! Construct from value.
    RefCounterWrap(const DataT &value)
     : m_data(value)
    {}

    //! Construct from value.
    RefCounterWrap(DataT &&value)
     : m_data(std::move(value))
    {}

    //! Access wrapped data.
    DataT &Data()
    { return m_data; }

    //! Access wrapped data.
    const DataT &Data() const
    { return m_data; }

    //! Access type that's been stored.
    virtual const std::type_info &storedType() const
    { return typeid(m_data); }

    //! Access address of stored value.
    virtual void *storedAddress()
    { return &m_data; }

    //! Access address of stored value.
    virtual const void *storedAddress() const
    { return &m_data; }

  protected:
    DataT m_data;
  };

  //! Wrap selecting best type of wrapper
  template<typename DataT>
  AbstractPtr refcounter_wrap_select(const DataT &data,const RefCounter *tmp)
  { return AbstractPtr(&data); }

  //! Wrap selecting best type of wrapper
  template<typename DataT>
  AbstractPtr refcounter_wrap_select(const DataT &data,const void *tmp)
  { return AbstractPtr(&data); }

  //! Wrap selecting best type of wrapper
  template<typename DataT>
  AbstractPtr refcounter_wrap_select(DataT &&data,const RefCounter *tmp)
  { return AbstractPtr(&data); }

  //! Wrap selecting best type of wrapper
  template<typename DataT>
  AbstractPtr refcounter_wrap_select(DataT &&data,const void *tmp)
  { return new RefCounterWrap<DataT>(std::move(data)); }

  //! Wrap value
  template<typename DataT>
  AbstractPtr refcounter_wrap(const DataT &data)
  { return refcounter_wrap_select(data,&data); }

  template<typename DataT>
  AbstractPtr refcounter_wrap(DataT &&data)
  { return refcounter_wrap_select(data,&data); }

  template<typename DataT>
  bool refcounter_unwrap(AbstractPtr &ptr,SmartPtr<DataT> &data)
  {
    data = dynamic_cast<DataT *>(ptr.get());
    return data.is_valid();
  }

  template<typename DataT>
  bool refcounter_unwrap(AbstractPtr &ptr,DataT &data)
  {
    RefCounterWrap<DataT> *wrapper = dynamic_cast<RefCounterWrap<DataT> *>(ptr.get());
    if(wrapper == nullptr)
      return false;
    data = wrapper->Data();
    return true;
  }

}


#endif
