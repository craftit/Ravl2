
#pragma once

#include <cassert>
#include "Ravl2/Array.hh"

namespace Ravl2
{

  template<typename ElementT, unsigned N>
  class SlaveIter
  {
    SlaveIter(const IndexRange<1> *rng, ElementT *data, const int *strides) noexcept
      : mAccess(rng,data,strides)
    {
      mIndex[N-1] = rng[N-1].min();
      mPtrStart = data + rng[N-1].min();
      mPtr = mPtrStart;
      for(unsigned i = 0;i < N-1;i++)
      {
	mPtr += rng[i].min() * strides[i];
	mIndex[i] = rng[i].min();
      }
    }

    void next()
    { mPtr++; }

    void next_ptr()
    {
      for(unsigned i = N-2;i > 0;--i) {
	++mIndex[i];
	if(mIndex[i] <= mAccess.range(i).max())
	  goto iterDone;
	mIndex[i] = mAccess.range(i).min();
      }
      // On the last index we don't need to update
      ++mIndex[0];
    iterDone:
      mPtr = mPtrStart;
      for(int i = 0;i < N-1;i++)
      { mPtr += mAccess.stride(i) * mIndex[i]; }
    }

    [[nodiscard]] ElementT &data()
    {  return *mPtr; }

    [[nodiscard]] ElementT *dataPtr()
    {  return mPtr; }

    [[nodiscard]] Index<N> index() const
    {
      Index<N> ret = mIndex;
      ret[N-1] += mPtr - mPtrStart;
      return ret;
    }

    [[nodiscard]] const IndexRange<N> &range() const
    { return mAccess.range(); }

    [[nodiscard]] const int *strides() const
    { return mAccess.strides(); }

  private:
    ElementT * mPtr {};
    ElementT * mPtrStart {};
    Index<N> mIndex {}; // Index of the beginning of the last dimension.
    Ravl2::ArrayAccess<ElementT,N> mAccess;
  };

  template<unsigned N,typename ...ArrayT>
  class ArrayIterZipN
  {
  public:

  private:
    std::tuple<SlaveIter<N,typename ArrayT::ValueT>...> mIters;
  };

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
      template<typename Array1T, typename Array2T>
      requires WindowedArray<Array1T,DataT,N> && WindowedArray<Array2T,Data2T,N>
      ArrayIterZip(Array1T &img, Array2T &img2) noexcept
        : ArrayIterZip(img.range().range_data(),img.origin_address(),img.strides(),
                       img2.range().range_data(),img2.origin_address(),img2.strides())
      {}


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

      //! Increment iterator, return true while we're on the same row.
      inline bool next()
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

    [[nodiscard]] inline auto dataPtr1()
    {
      return mPtr;
    }

    [[nodiscard]] inline auto dataPtr2()
    {
      return mZipAt;
    }

    //! Get the current value.
    [[nodiscard]] inline auto operator*()
    {
      return std::tuple<DataT &,Data2T&>({data1(), data2()});
    }

    //! Get strides
    [[nodiscard]] inline auto strides1() const noexcept
    {
      return m_access.strides();
    }

    //! Get strides
    [[nodiscard]] inline auto strides2() const noexcept
    {
      return mZipAccess.strides();
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

  template<typename Array1T, typename Array2T, unsigned N = Array1T::dimensions, typename DataT = Array1T::ValueT, typename Data2T = Array2T::ValueT>
  requires WindowedArray<Array1T,DataT,N> && WindowedArray<Array2T,Data2T,N>
  auto zipArrayIter(Array1T &img, Array2T &img2) noexcept -> ArrayIterZip<N,DataT,Data2T>
  {
    return ArrayIterZip<N,DataT,Data2T>(img,img2);
  }

}