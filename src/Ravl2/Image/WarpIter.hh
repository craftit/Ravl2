//
// Created by charles galambos on 22/01/2025.
//

#pragma once

#include "Ravl2/Geometry/Affine.hh"
#include "Ravl2/Geometry/Projection.hh"
#include "Ravl2/Concepts.hh"

namespace Ravl2 {

  //! @brief CRTP for warping iterators.
  //! This attaches an extra method to a 2-dimensional iterator that keeps
  //! track of a warped point.  This can often be done more efficiently than
  //! recomputing the warped point each time as index increments have an integer
  //! step size.

  template<typename ImplT, typename CoordTypeT>
  class WarpIter {
  protected:
    //! Default constructor.
    WarpIter() = default;

  private:
    //! Access the iterator we're using.
    auto &iter()
    { return static_cast<ImplT &>(*this).mIter; }

    //! Access the iterator we're using.
    const auto &iter() const
    { return static_cast<const ImplT &>(*this).mIter; }

    //! Access the derived class.
    ImplT &derived()
    { return static_cast<ImplT &>(*this); }

    //! Access the derived class.
    const ImplT &derived() const
    { return static_cast<const ImplT &>(*this); }

  public:

    //! Access current index.
    [[nodiscard]] Index<2> index() const
    {
      return iter().index();
    }

    //! Access current index as a point.
    [[nodiscard]] Point<CoordTypeT,2> indexPoint() const
    {
      return Ravl2::toPoint<CoordTypeT>(this->index());
    }

    //! Check if iterator is valid.
    [[nodiscard]] bool valid() const
    {
      return iter().valid();
    }

    //! Goto next point
    //! @return true if we're on the same row.
    bool next()
    {
      if(!iter().next()) {
        derived().tNextRow();
        return false;
      }
      // Fast path.
      derived().tNext();
      return true;
    }

    //! Goto next element.
    const ImplT &operator++()
    {
      next();
      return derived();
    }

    //! Access the current element.
    //! This returns the type reference by the iterator.
    [[nodiscard]] auto operator*() const
    {
      return *iter();
    }

    //! Access the current element.
    //! This returns the type reference by the iterator.
    [[nodiscard]] auto operator*()
    {
      return *iter();
    }

    //! Check the derived type implements 'indexWarped'
    [[nodiscard]] auto warpedIndex() const
    {
      return derived().warpedIndex();
    }

  };

  //! General iterator for warping.
  //! This makes as few assumptions as possible about the transform.
  //! It is the slowest of the iterators.

  template<typename TransformT, WindowedIterator IteratorT, typename CoordTypeT = float>
  class WarpGeneralIter
    : public WarpIter< WarpGeneralIter<TransformT, IteratorT, CoordTypeT>, CoordTypeT>
  {
  public:

    WarpGeneralIter(const IteratorT &iter, const TransformT &transform)
      : mIter(iter),
        mTransform(transform)
    {}

    //! Access warped coordinate corresponding to the current index.
    [[nodiscard]] Point<CoordTypeT,2> warpedIndex() const
    {
      return mTransform(this->indexPoint());
    }

  protected:
    //! Goto next row
    static void tNextRow()
    {}

    //! Goto next point.
    //! Return true if we're on the same row.
    static void tNext()
    {}

    IteratorT mIter;
    TransformT mTransform;
    friend class WarpIter< WarpGeneralIter<TransformT, IteratorT>, CoordTypeT>;
  };

  //! Iterator for affine warping.

  template<WindowedIterator IteratorT, typename CoordTypeT = float>
  class WarpAffineIter
    : public WarpIter< WarpAffineIter<IteratorT, CoordTypeT>, CoordTypeT>
  {
  public:
    WarpAffineIter(const IteratorT &iter, const Affine<CoordTypeT,2> &affine)
      : mIter(iter),
        mAffine(affine.SRMatrix()),
        mAt(affine(this->indexPoint())),
        mRowStart(mAt)
    {}

    //! Access warped coordinate corresponding to the current index.
    [[nodiscard]] const Point<CoordTypeT,2> &warpedIndex() const
    {
      return mAt;
    }

  protected:
    //! Goto next row
    void tNextRow()
    {
      mRowStart += mAffine.col(0);
      mAt = mRowStart;
    }

    //! Goto next point.
    //! Return true if we're on the same row.
    void tNext()
    {
      mAt += mAffine.col(1);
    }

    IteratorT mIter;

  private:
    Matrix<float,2,2> mAffine;
    Point<float,2> mAt;
    Point<float,2> mRowStart;

    friend class WarpIter< WarpAffineIter, CoordTypeT>;
  };


  //! Deal with projective warping.

  template<WindowedIterator IteratorT, typename CoordTypeT = float>
  class WarpProjectiveIter
    : public WarpIter< WarpProjectiveIter<IteratorT, CoordTypeT>, CoordTypeT>
  {
  public:
    WarpProjectiveIter(const IteratorT &iter, const Projection<CoordTypeT,2> &projection)
      : mIter(iter),
        mHomography(projection.homography()),
        mProjAt(mHomography * toHomogeneous(this->indexPoint())),
        mRowStart(mProjAt)
    {}

    //! Access warped coordinate corresponding to the current index.
    [[nodiscard]] Point<CoordTypeT,2> warpedIndex() const
    {
      return fromHomogeneous(mProjAt);
    }

  protected:
    //! Goto next row
    void tNextRow()
    {
      mRowStart += mHomography.col(0);
      mProjAt = mRowStart;
    }

    //! Goto next point.
    void tNext()
    {
      mProjAt += mHomography.col(1);
    }

    IteratorT mIter;

  private:
    Matrix<float,3,3> mHomography;
    Point<float,3> mProjAt;
    Point<float,3> mRowStart;

    friend class WarpIter< WarpProjectiveIter, CoordTypeT>;
  };


  //! Begin a generic warp iterator.
  template <typename ContainerT, typename TransformT>
  auto beginWarp(ContainerT &container, const TransformT &transform)
  {
    return WarpGeneralIter(container.begin(), transform);
  }

  //! Begin an affine warp iterator.
  template <typename ContainerT,typename CoordTypeT>
  auto beginWarp(ContainerT &container, const Affine<CoordTypeT,2> &transform)
  {
    return WarpAffineIter(container.begin(), transform);
  }

  //! Begin a projective warp iterator.
  template <typename ContainerT,typename CoordTypeT>
  auto beginWarp(ContainerT &container, const Projection<CoordTypeT,2> &transform)
  {
    return WarpProjectiveIter(container.begin(), transform);
  }


} // Ravl2
