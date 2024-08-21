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

  template <class Container>
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
    LoopIter(Container &c, typename Container::iterator it)
        : container(c),
          iter(it)
    {
    }

    //! Get a iterator at the last element
    //! The container must not be empty
    static LoopIter
    Last(Container &c)
    {
      auto iter = c.end();
      assert(c.empty() != c.end());
      iter--;
      return LoopIter(c, iter);
    }

    //! Get a iterator at the first element
    static LoopIter
    First(Container &c)
    {
      return LoopIter(c, c.begin());
    }

    void
    GotoLast()
    {
      iter = container.end();
      --iter;
    }

    void
    GotoFirst()
    {
      iter = container.begin();
    }

    //! Compare two iterators
    bool
    operator==(const LoopIter &other) const
    {
      assert(&container == &other.container);
      return iter == other.iter;
    }

    //! Compare two iterators
    bool
    operator==(const typename Container::iterator &other) const
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
    auto &
    Data()
    {
      return *iter;
    }

    //! Circular get the element after the current one
    auto &
    NextCrcData()
    {
      auto next = iter;
      ++next;
      if(next == container.end())
        next = container.begin();
      return *next;
    }

    //! Get the data after the current one
    auto &
    NextData()
    {
      auto next = iter;
      ++next;
      assert(next != container.end());
      return *next;
    }

    auto &
    operator++()
    {
      ++iter;
      return *this;
    }

  protected:
    Container &container;
    typename Container::iterator iter;
  };

  //! Helper function to create a circular iterator
  //! This creates an iterator that points to the last valid element
  template <class Container>
  LoopIter<Container>
  beginLoopLast(Container &c)
  {
    return LoopIter<Container>::Last(c);
  }

  //! Helper function to create a circular iterator
  template <class Container>
  LoopIter<Container>
  beginLoopFirst(Container &c)
  {
    return LoopIter<Container>::First(c);
  }
}// namespace Ravl2