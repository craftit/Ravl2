//
// Created by charles galambos on 09/08/2024.
//

#pragma once

#include <cassert>

namespace Ravl2
{

  //! @brief Get the next element in a container, if the iterator is at the end, return the first element
  //! The iterator must be at a valid position in the container, and the container must contain at least one element
  //! @param container the container
  //! @param it the iterator
  //! @return the next element
  template <typename ContainerT, typename IteratorT, typename DataT = typename ContainerT::value_type>
  const DataT &nextDataCrc(const ContainerT &container, const IteratorT &it)
  {
    assert(!container.empty());
    if((it + 1) == container.end())
      return container.front();
    return *(it + 1);
  }

  //! @brief Iterator that holds a position and allows for circular iteration, from the end, go to the beginning
  //! This is particularly useful for porting code that originally used a circular linked list.

  template <typename Container,typename IteratorT = Container::iterator>
  class LoopIter
  {
  public:
    //! Constructor
    explicit LoopIter(Container &c)
        : container(c),
          iter(c.begin())
    {
    }

    //! Constructor
    LoopIter(Container &c, const IteratorT &it)
        : container(c),
          iter(it)
    {
    }
    
    //! Copy constructor
    LoopIter(const LoopIter &it)
      : container(it.container),
        iter(it.iter)
    {}
    
    //! Compare two iterators
    bool
    operator==(const LoopIter &other) const
    {
      assert(&container == &other.container);
      return iter == other.iter;
    }

    //! Compare two iterators
    bool
    operator==(const IteratorT &other) const
    {
      return iter == other;
    }

    //! Compare two iterators
    bool
    operator!=(const LoopIter &other) const
    {
      assert(&container == &other.container);
      return iter != other.iter;
    }

    //! Access the current element
    auto &data()
    {
      return *iter;
    }

    //! Circular get the element after the current one
    auto &nextCrcData()
    {
      auto next = iter;
      ++next;
      if(next == container.end())
        next = container.begin();
      return *next;
    }
    
    //! Assign the iterator
    LoopIter &operator=(const IteratorT &it)
    {
      iter = it;
      return *this;
    }
    
    //! Assign the iterator to another iterator
    LoopIter &operator=(const LoopIter &it)
    {
      assert(&container == &it.container);
      iter = it.iter;
      return *this;
    }
    
    auto &operator++()
    {
      ++iter;
      if(iter == container.end())
        iter = container.begin();
      return *this;
    }
    
    //! Access data
    auto operator*()
    { return *iter; }
    
    //! Access data
    auto operator*() const
    { return *iter; }

  protected:
    Container &container;
    IteratorT iter;
  };
  
  //! Helper function to create a circular iterator
  template <typename ContainerT,typename IteratorT>
  LoopIter<ContainerT,IteratorT> loopIter(ContainerT &c, const IteratorT &it)
  {
    return LoopIter<ContainerT,IteratorT>(c, it);
  }
  
}// namespace Ravl2