
#pragma once

#include <cassert>
#include "Ravl2/Array.hh"

namespace Ravl2
{

  template<unsigned N,typename DataT, typename Data2T>
  class ArrayIterZip
  {
  public:
      ArrayIterZip()
      = default;

      ArrayIterZip(const IndexRange<1> *rng, DataT *data, const int *strides,
                   const IndexRange<1> *rng2, Data2T *data2, const int *strides2
                   ) noexcept
      {
        assert(data != nullptr);
        assert(data != nullptr);
        DataT *ptr = data;
        Data2T *ptr2 = data2;
        for(int i = 0;i < N;i++) {
          ptr += rng[i].min() * strides[i];
          ptr2 += rng2[i].min() * strides2[i];
        }
        for(int i = N-1;i >= 0;i--) {
          m_at[i] = ptr;
          m_end[i] = ptr + rng[i].size() * strides[i];
          mZipAt[i] = ptr2;
        }
        m_strides = strides;
        mZipStrides = strides2;
      }

      //! Access two arrays at the same time.
      ArrayIterZip(ArrayAccess<DataT,N> img, ArrayAccess<Data2T,N> img2) noexcept
        : ArrayIterZip(img.range().range_data(),img.origin_address(),img.strides(),
                       img2.range().range_data(),img2.origin_address(),img2.strides())
      {
        //assert(img.range().contains(img2.range()));
      }

      ArrayIterZip(const Array<DataT,N> &img, ArrayAccess<Data2T,N> &img2) noexcept
        : ArrayIterZip(img.range().range_data(),img.origin_address(),img.strides(),
                       img2.range().range_data(),img2.origin_address(),img2.strides())
      {
        //assert(img.range().size(img2.range()));
      }

      ArrayIterZip(const Array<DataT,N> &img, ArrayView<Data2T,N> &img2) noexcept
              : ArrayIterZip(img.range().range_data(),img.origin_address(),img.strides(),
                             img2.range().range_data(),img2.origin_address(),img2.strides())
      {
        //assert(img.range().contains(img2.range()));
      }


  protected:
      //! The slower part of going to the next row,
      //! this allows the fast path to be inlined without bloating the code.
      void next_row()
      {
        mZipAt[N-2] += mZipStrides[N-2];
        mZipAt[N-1] = mZipAt[N-2];
        m_at[N-2] += m_strides[N-2];
        m_at[N-1] = m_at[N-2];
        m_end[N-1] += m_strides[N-2];
        for(int i = N-2;i > 0;i--) {
          if(m_at[i-1] != m_end[i-1]) {
            break;
          }
          mZipAt[i-1] += mZipStrides[i-1];
          mZipAt[i] = mZipAt[i-1];
          m_at[i-1] += m_strides[i-1];
          m_at[i] = m_at[i-1];
          m_end[i] += m_strides[i-1];
        }
      }

  public:
      //! Increment iterator
      inline auto &operator++()
      {
        mZipAt[N-1]++;
        m_at[N-1]++;
        if(m_at[N-1] == m_end[N-1]) {
          next_row();
        }
        return *this;
      }

      //! Increment iterator
      //! Returns true while we're on the same row.
      inline bool next_col()
      {
        mZipAt[N-1]++;
        m_at[N-1]++;
        if(m_at[N-1] == m_end[N-1]) {
          next_row();
          return false;
        }
        return true;
      }

      //! Test if the iterator is valid.
      [[nodiscard]] inline bool valid() const
      {
        return m_at[0] != m_end[0];
      }

      //! Test if the iterator is finished.
      [[nodiscard]] inline bool done() const
      {
        return m_at[0] == m_end[0];
      }

      //! Get the current value.
      [[nodiscard]] inline auto operator*()
      {
        return std::tuple<DataT &,Data2T&>({*m_at[N-1], *mZipAt[N-1]});
      }

      [[nodiscard]] inline DataT &data1()
      {
        return *m_at[N-1];
      }

      [[nodiscard]] inline Data2T &data2()
      {
        return *mZipAt[N-1];
      }


  protected:
      std::array<DataT *,N> m_at {};
      std::array<DataT *,N> m_end {};
      const int *m_strides = nullptr;
      std::array<Data2T *,N> mZipAt {};
      const int *mZipStrides = nullptr;
  };

}