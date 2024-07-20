
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
	: m_access(rng,data,strides),
	  mZipAccess(rng2,data2,strides2)
      {
	assert(data != nullptr);
	assert(data2 != nullptr);
	assert(strides[N-1] == 1);
	assert(strides2[N-1] == 1);
	mIndex[N-1] = rng[N-1].min();
        mIndexZip[N-1] = rng2[N-1].min();
	mPtrStart = data + rng[N-1].min();
	mZipStart = data2 + rng2[N-1].min();
	mPtr = mPtrStart;
	mZipAt = mZipStart;
	for(unsigned i = 0;i < N-1;i++)
	{
	  mPtr += rng[i].min() * strides[i];
	  mZipAt += rng2[i].min() * strides2[i];
	  mIndex[i] = rng[i].min();
          mIndexZip[i] = rng2[i].min();
	}
	mEnd = mPtr + rng[N-1].size();
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
    void next_ptr()
    {
      for(unsigned i = N-2;i > 0;--i) {
	++mIndex[i];
        ++mIndexZip[i];
	if(mIndex[i] <= m_access.range(i).max())
	  goto done;
	mIndex[i] = m_access.range(i).min();
        mIndexZip[i] = mZipAccess.range(i).min();
      }
      // On the last index we don't need to update
      ++mIndex[0];
      ++mIndexZip[0];
    done:
      mPtr = mPtrStart;
      mZipAt = mZipStart;
      for(int i = 0;i < N-1;i++)
      {
	mPtr += m_access.stride(i) * mIndex[i];
	mZipAt += mZipAccess.stride(i) * mIndexZip[i];
      }
      mEnd = mPtr + m_access.range(N-1).size();
    }

  public:
      //! Increment iterator
      inline auto &operator++()
      {
	mZipAt++;
	mPtr++;
	if(mPtr == mEnd) {
          next_ptr();
        }
        return *this;
      }

      //! Increment iterator
      //! Returns true while we're on the same row.
      inline bool next_col()
      {
        mZipAt++;
        mPtr++;
        if(mPtr == mEnd) {
          next_ptr();
          return false;
        }
        return true;
      }

    //! Test if the iterator is valid.
    [[nodiscard]] bool valid() const noexcept
    { return mIndex[0] <= m_access.range(0).max();  }

    //! Test if the iterator is valid.
    [[nodiscard]]
    operator bool() const noexcept
    { return valid(); }

    //! Test if the iterator is finished.
    [[nodiscard]] bool done() const noexcept
    { return mIndex[0] > m_access.range(0).max();  }

    [[nodiscard]] inline DataT &data1()
    {
      return *mPtr;
    }

    [[nodiscard]] inline Data2T &data2()
    {
      return *mZipAt;
    }

    //! Get the current value.
    [[nodiscard]] inline auto operator*()
    {
      return std::tuple<DataT &,Data2T&>({data1(), data2()});
    }

  protected:
    DataT * mPtr {};
    const DataT * mEnd {};
    DataT * mPtrStart {};
    Index<N> mIndex {}; // Index of the beginning of the last dimension.
    ArrayAccess<DataT,N> m_access;

    Data2T * mZipAt {};
    Data2T * mZipStart {};
    Index<N> mIndexZip {}; // Index of the beginning of the last dimension.
    Ravl2::ArrayAccess<Data2T,N> mZipAccess;
  };

}