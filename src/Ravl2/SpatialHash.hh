//
// Created by charles galambos on 09/02/2025.
//

#pragma once

#include <optional>
#include "Ravl2/Types.hh"
#include "Ravl2/IndexRange.hh"
#include "Ravl2/Geometry/Geometry.hh"

namespace Ravl2 {

  //! Container for spatial hashing
  //! This is a simple spatial hash that uses a fixed bin size.
  //! It is useful for quickly finding items in a region of space.

  template<typename RealT, unsigned N, typename DataT>
  class SpatialHash
  {
  public:
    //! Default constructor, uses bin size of 1
    SpatialHash() = default;

    //! Construct with bin size
    explicit SpatialHash(const Vector<RealT,N> &binSize)
      : mBinSize(binSize)
    {}

    //! Reserves space for the hash
    void reserve(size_t size)
    {
      mHash.reserve(size);
    }

    //! Get the total number of elements
    [[nodiscard]] size_t size() const
    {
      return mHash.size();
    }

    //! Get the bin for a position
    [[nodiscard]] Index<N> index(const Point<RealT,N> &pos) const
    {
      // Note: toIndex goes to the nearest integer
      Point<float,2> pnt = pos.array()/mBinSize.array();
      return toIndex<N>(pnt);
    }

    //! Get the center of a bin
    [[nodiscard]] constexpr Point<RealT,N> center(const Index<N> &bin) const
    {
      return toPoint(bin).array() * mBinSize.array();
    }

    //! Insert a data item at a position
    void insert(const Point<RealT,N> &pos, const DataT &data)
    {
      mHash.emplace(index(pos),std::tuple(pos, data));
    }

    //! Visit all items in a bin
    //! The function should be of the form void(Point<RealT,N>, DataT &data)
    template<typename FuncT>
    void visit(const Index<N> &bin, FuncT &&func)
    {
      auto range = mHash.equal_range(bin);
      for(auto it = range.first; it != range.second; ++it) {
        func(std::get<0>(it->second), std::get<1>(it->second));
      }
    }

    //! Visit all items in a bin
    //! The function should be of the form void(Point<RealT,N>, DataT &data)
    template<typename FuncT>
    void visit(const Index<N> &bin, FuncT &&func) const
    {
      auto range = mHash.equal_range(bin);
      for(auto it = range.first; it != range.second; ++it) {
        func(std::get<0>(it->second), std::get<1>(it->second));
      }
    }

    //! Visit all items within a distance of a point
    template<typename FuncT>
    void visitNear(const Vector<RealT,N> &pos, RealT distance, FuncT &&func)
    {

      Point<RealT,N> minP = pos - Vector<RealT,N>::Constant(distance);
      Point<RealT,N> maxP = pos + Vector<RealT,N>::Constant(distance);
      IndexRange<N> range(toIndex(minP), toIndex(maxP));
      for(auto i : range) {
        visit(i, func);
      }
    }

    //! Visit all items within a distance of a point
    template<typename FuncT>
    void visitNear(const Vector<RealT,N> &pos, RealT distance, FuncT &&func) const
    {
      Point<RealT,N> minP = pos - Vector<RealT,N>::Constant(distance);
      Point<RealT,N> maxP = pos + Vector<RealT,N>::Constant(distance);
      IndexRange<N> range(toIndex<N>(minP), toIndex<N>(maxP));
      for(auto i : range) {
        visit(i, func);
      }
    }

    //! Find the closest value to a point within a distance
    [[nodiscard]] std::optional<DataT> closest(const Vector<RealT,N> &pos, RealT distance) const
    {
      const DataT *closest = nullptr;
      RealT closestDistance = sqr(distance);
      visitNear(pos, distance, [&](const Point<RealT,N> &at,const DataT &data) {
        RealT dist = (pos - at).squaredNorm();
        if(dist < closestDistance) {
          closest = &data;
          closestDistance = dist;
        }
      });
      if(closest == nullptr) return std::nullopt;
      return *closest;
    }

  private:
    Vector<RealT,N> mBinSize = Vector<RealT,N>::Ones();
    std::unordered_multimap<Index<N>, std::tuple<Point<RealT,N>,DataT> > mHash {};
  };

} // Ravl2
