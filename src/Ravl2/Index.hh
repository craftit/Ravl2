/*
 * Index.hh
 *
 *  Created on: Dec 8, 2013
 *      Author: charlesgalambos
 */

#pragma once

#include <cassert>
#include <vector>
#include <array>
#include <algorithm>
#include <iostream>
#include <fmt/format.h>
#include <fmt/ostream.h>
#include <cereal/cereal.hpp>
#include <cereal/types/array.hpp>
#include <cereal/archives/json.hpp>

#include "Ravl2/Sentinel.hh"

namespace Ravl2
{
  //! N-dimensional index.

  template <unsigned N>
  class Index
  {
  public:
    using value_type = int;
    constexpr static unsigned dimensions = N;

    //! array of indexs
    constexpr Index(std::initializer_list<int> val)
    {
      if(val.size() == 1) {
        for(unsigned i = 0; i < N; i++)
          m_index[i] = val.begin()[0];
        return;
      }
      assert(val.size() == N);
      for(unsigned i = 0; i < N; i++)
        m_index[i] = val.begin()[i];
    }

    //! Unpack using a template
    template <typename... Args>
    constexpr explicit Index(Args... args)
    {
      static_assert(sizeof...(args) == N, "Incorrect number of arguments");
      std::array<int, N> vals {args...};
      for(unsigned i = 0; i < N; i++)
        m_index[i] = vals[i];
    }

    //! Default constructor
    constexpr Index() = default;

    //! Access location in the i th dimension.
    [[nodiscard]] constexpr int index(unsigned i) const noexcept
    {
      assert(i < N);
      return m_index[i];
    }

    //! Access index for dimension n.
    [[nodiscard]] constexpr int operator[](unsigned n) const
    {
      assert(n < N);
      return m_index[n];
    }

    //! Access index for dimension n.
    [[nodiscard]] constexpr int &operator[](unsigned n)
    {
      assert(n < N);
      return m_index[n];
    }

    //! Add index in place
    constexpr Index<N> &operator+=(const Index<N> &ind) noexcept
    {
      for(unsigned i = 0; i < N; i++)
        m_index[i] += ind[i];
      return *this;
    }

    //! Subtract index in place
    constexpr Index<N> &operator-=(const Index<N> &ind) noexcept
    {
      for(unsigned i = 0; i < N; i++)
        m_index[i] -= ind[i];
      return *this;
    }

    //! Add two index's together.
    [[nodiscard]] constexpr Index<N> operator+(const Index<N> &ind) const
    {
      Index<N> ret;
      for(unsigned i = 0; i < N; i++)
        ret[i] = m_index[i] + ind[i];
      return ret;
    }

    //! Subtract an index from this one.
    [[nodiscard]] constexpr Index<N> operator-(const Index<N> &ind) const
    {
      Index<N> ret;
      for(unsigned i = 0; i < N; i++)
        ret[i] = m_index[i] - ind[i];
      return ret;
    }

    //! Equality test.
    [[nodiscard]] constexpr bool operator==(const Index<N> &ind) const
    {
      for(unsigned i = 0; i < N; i++)
        if(m_index[i] != ind[i])
          return false;
      return true;
    }

    //! Equality test.
    [[nodiscard]] constexpr bool operator!=(const Index<N> &ind) const
    {
      return !operator==(ind);
    }

    //! begin
    [[nodiscard]] constexpr int *begin()
    {
      return m_index.data();
    }

    //! end
    [[nodiscard]] constexpr int *end()
    {
      return &(m_index.data()[N]);
    }

    //! begin
    [[nodiscard]] constexpr const int *begin() const
    {
      return m_index.data();
    }

    //! end
    [[nodiscard]] constexpr const int *end() const
    {
      return &(m_index.data()[N]);
    }

    //! Access as const ptr.
    [[nodiscard]] constexpr const int *data() const
    {
      return m_index.data();
    }

    //! Access as ptr.
    [[nodiscard]] constexpr int *data()
    {
      return m_index.data();
    }

    //! Access size
    [[nodiscard]] constexpr size_t size() const
    {
      return size_t(N);
    }

  protected:
    std::array<int, N> m_index = {0};
  };

  template <unsigned N>
  std::ostream &operator<<(std::ostream &strm, const Index<N> &ind)
  {
    strm << ind[0];
    for(unsigned i = 1; i < N; i++)
      strm << " " << ind[i];
    return strm;
  }

  //! Convert a parameter list of RealT to a point
  template <typename... DataT, unsigned N = sizeof...(DataT)>
  constexpr inline Index<N> toIndex(DataT... data)
  {
    return Index<N>({int(data)...});
  }

  //! Serialization support
  template <class Archive, unsigned N>
  constexpr void serialize(Archive &archive, Index<N> &ind)
  {
    cereal::size_type s = N;
    archive(cereal::make_size_tag(s));
    if(s != N) {
      throw std::runtime_error("array has incorrect length");
    }
    for(auto &i : ind) {
      archive(i);
    }
  }


  // Let everyone know there's an implementation already generated for common cases
  extern template class Index<1>;
  extern template class Index<2>;
  extern template class Index<3>;
}// namespace Ravl2


// Custom specialization of std::hash injected in namespace std.
template<unsigned N>
requires (N > 0)
struct std::hash<Ravl2::Index<N> >
{
  std::size_t operator()(const Ravl2::Index<N>& s) const noexcept
  {
    std::size_t ret = std::size_t(s[0]);
    for(unsigned i = 1; i < N; i++) {
      ret ^= std::size_t(s[i]) << i;
    }
    return ret;
  }
};

namespace fmt
{
  template <unsigned N>
  struct formatter<Ravl2::Index<N>> : ostream_formatter {
  };
}// namespace fmt

