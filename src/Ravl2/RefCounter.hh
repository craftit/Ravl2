/*
 * RefCounter.hh
 *
 *  Created on: Dec 8, 2013
 *      Author: charlesgalambos
 */

#ifndef RAVL2_REFCOUNTER_HH_
#define RAVL2_REFCOUNTER_HH_

#include <atomic>
#include <utility>
#include <assert.h>

namespace Ravl2
{
  class RefCounter;
  void intrusive_ptr_add_ref(RefCounter *body) noexcept;
  void intrusive_ptr_release(RefCounter *body) noexcept;

  //! Intrinsic part of a reference counter.
  // References to this object should always be taken with SmartPtr class.

  class RefCounter
  {
  public:
    //! Constructor.
    RefCounter()
    {}

    //! Copy constructor.
    // Make sure reference count isn't copied.
    RefCounter(const RefCounter &)
    {}

    //! Assignment operator, make sure references aren't copied.
    RefCounter &operator=(const RefCounter &other)
    { return *this; }

    //! Destructor.
    virtual ~RefCounter();

    //! Access use count for object
    int use_count() const noexcept
    { return m_referenceCount; }

  protected:
    std::atomic<int> m_referenceCount {0};
    friend void intrusive_ptr_add_ref(RefCounter *body) noexcept;
    friend void intrusive_ptr_release(RefCounter *body) noexcept;
  };

  //! Add reference to object.
  inline void intrusive_ptr_add_ref(RefCounter *body) noexcept
  {
    int count = body->m_referenceCount.load(std::memory_order_relaxed);
    assert(count >= 0);
    // If there are no other pointers to the object, there is no race condition
    // as this is the only thread that can have access to it.
    if(count == 0) {
      body->m_referenceCount.store(1,std::memory_order_relaxed);
    } else {
      ++(body->m_referenceCount);
    }
  }

  //! Release reference count.
  inline void intrusive_ptr_release(RefCounter *body) noexcept
  {
    int count = body->m_referenceCount.load(std::memory_order_relaxed);
    assert(count > 0);
    // If there is only 1 reference to the object, there is no race condition
    // for destruction as this is the only thread that can have access to it.
    if(count == 1) {
      body->m_referenceCount.store(0,std::memory_order_relaxed);
      delete body;
    } else {
      if((body->m_referenceCount.fetch_sub(1)) == 1)
        delete body;
    }
  }

  //! Smart pointer to intrinsically reference counter object.

  template<typename DataT>
  class SmartPtr
  {
  public:
    //! Default constructor
    SmartPtr()
    {}

    //! Construct from a pointer
    SmartPtr(DataT *obj)
     : m_ptr(obj)
    {
      if(obj != nullptr)
        intrusive_ptr_add_ref(obj);
    }

    //! Construct from a pointer
    SmartPtr(const DataT *obj)
     : m_ptr(const_cast<DataT *>(obj))
    {
      if(obj != nullptr)
        intrusive_ptr_add_ref(m_ptr);
    }

    //! Default constructor
    SmartPtr(const SmartPtr<DataT> &obj)
     : m_ptr(obj.m_ptr)
    {
      if(m_ptr != nullptr)
        intrusive_ptr_add_ref(m_ptr);
    }

    //! Destructor
    ~SmartPtr()
    {
      if(m_ptr != nullptr)
        intrusive_ptr_release(m_ptr);
    }

    //! Access as plain pointer
    DataT *get()
    { return m_ptr; }

    //! Access as plain pointer
    const DataT *get() const
    { return m_ptr; }

    //! Assign to plain pointer
    SmartPtr<DataT> &operator=(const DataT *ptr)
    {
      if(ptr != nullptr)
        intrusive_ptr_add_ref(const_cast<DataT *>(ptr));
      DataT *oldPtr = m_ptr;
      m_ptr = const_cast<DataT *>(ptr);
      if(oldPtr != nullptr)
        intrusive_ptr_release(oldPtr);
      return *this;
    }

    //! Assign to another pointer
    SmartPtr<DataT> &operator=(const SmartPtr<DataT> &ptr)
    { return (*this) = ptr.get(); }

    //! Access object
    DataT *operator->()
    {
      assert(m_ptr != nullptr);
      return m_ptr;
    }

    //! Access object
    const DataT *operator->() const
    {
      assert(m_ptr != nullptr);
      return m_ptr;
    }

    // Dereference pointer
    DataT &operator*()
    {
      assert(m_ptr != nullptr);
      return *m_ptr;
    }

    //! Swap two pointers
    //! Don't need to do anything with reference counts.
    void swap(SmartPtr<DataT> &ptr)
    {
      DataT *tmp = ptr.m_ptr;
      ptr.m_ptr = m_ptr;
      m_ptr = tmp;
    }

    //! Is this a valid pointer?
    bool is_valid() const noexcept
    { return m_ptr != nullptr; }

    //! Access use count for object
    int use_count() const noexcept
    {
      if(m_ptr == nullptr)
        return 0;
      return m_ptr->m_referenceCount;
    }

    //! Is this the only pointer to the object?
    bool unique() const noexcept
    {
      if(m_ptr == nullptr)
        return false;
      return m_ptr->m_referenceCount == 1;
    }
  protected:
    DataT *m_ptr {nullptr};
  };

  template<typename DataT>
  void swap(SmartPtr<DataT> &ptr1,SmartPtr<DataT> &ptr2)
  { ptr1.swap(ptr2); }



}




#endif /* REFCOUNTER_HH_ */
