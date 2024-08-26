//
// Created by charles galambos on 04/08/2024.
//

#pragma once

namespace Ravl2
{
  //! @brief A sentinel type which can be used to represent the end of a range.
  //! The default comparison operators look for a valid() method on the iterator.
  //! If the iterator has a valid() method, the sentinel will compare equal to
  //! the iterator when the iterator is not valid().
  class Sentinel
  {
  public:
    //! Default constructor.
    constexpr Sentinel() = default;
  };

  //! Compare two sentinels for equality, always returns true.
  inline constexpr bool operator==([[maybe_unused]] const Sentinel &lhs, [[maybe_unused]] const Sentinel &rhs)
  {
    return true;
  }

  //! Compare two sentinels for inequality, always returns false.
  inline constexpr bool operator!=([[maybe_unused]] const Sentinel &lhs, [[maybe_unused]] const Sentinel &rhs)
  {
    return false;
  }

  //! Compare with a iterator which implements the valid() method.
  template <typename IterT>
  inline constexpr bool operator==([[maybe_unused]] const Sentinel &lhs, const IterT &rhs)
  {
    return !rhs.valid();
  }

  //! Compare with a iterator which implements the valid() method.
  template <typename IterT>
  inline constexpr bool operator==(const IterT &lhs, [[maybe_unused]] const Sentinel &rhs)
  {
    return !lhs.valid();
  }

  //! Compare with a iterator which implements the valid() method.
  template <typename IterT>
  inline constexpr bool operator!=([[maybe_unused]] const Sentinel &lhs, const IterT &rhs)
  {
    return rhs.valid();
  }

  //! Compare with a iterator which implements the valid() method.
  template <typename IterT>
  inline constexpr bool operator!=(const IterT &lhs, [[maybe_unused]] const Sentinel &rhs)
  {
    return lhs.valid();
  }

}// namespace Ravl2