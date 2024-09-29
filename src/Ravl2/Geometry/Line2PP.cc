// This file is part of RAVL, Recognition And Vision Library
// Copyright (C) 2002, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here

#include "Ravl2/Geometry/Line2PP.hh"

namespace Ravl2
{
  namespace
  {

    enum class ContainCodeT : unsigned
    {
      TOP = 0x1,
      BOTTOM = 0x2,
      RIGHT = 0x4,
      LEFT = 0x8
    };

    //! Or operator for ContainCodeT
    inline unsigned operator|(unsigned a, ContainCodeT b)
    {
      return a | static_cast<unsigned>(b);
    }

    //! Or operator for ContainCodeT
    inline unsigned operator&(unsigned a, ContainCodeT b)
    {
      return (a) & static_cast<unsigned>(b);
    }

    //! In place or operator for ContainCodeT
    inline unsigned &operator|=(unsigned &a, ContainCodeT b)
    {
      a = a | b;
      return a;
    }

    template <typename RealT>
    [[nodiscard]] inline unsigned containsCode(const Point<RealT, 2> &pnt, const Range<RealT, 2> &rng)
    {
      unsigned ret = 0;
      if(pnt[0] > rng.range(0).max())
        ret |= ContainCodeT::BOTTOM;
      else if(pnt[0] < rng.range(0).min())
        ret |= ContainCodeT::TOP;
      if(pnt[1] > rng.range(1).max())
        ret |= ContainCodeT::RIGHT;
      else if(pnt[1] < rng.range(1).min())
        ret |= ContainCodeT::LEFT;
      return ret;
    }
  }// namespace

  //! @brief Clip line by given rectangle.
  //! @return false if no part of the line is in the rectangle.
  //! Uses the Cohen and Sutherland line clipping algorithm.

  template <typename RealT>
  bool Line2PP<RealT>::clipBy(const Range<RealT, 2> &rng)
  {
    auto &line = *this;
    bool accept = false;
    auto oc0 = containsCode(line.P1(), rng);
    auto oc1 = containsCode(line.P2(), rng);

    do {
      if(!(oc0 | oc1)) {
        accept = true;
        break;
      }
      if(oc0 & oc1)
        break;
      Point<RealT, 2> np;
      auto oc = (oc0 != 0) ? oc0 : oc1;
      if(oc & ContainCodeT::TOP) {
        np[0] = rng.range(0).min();
        np[1] = line.P1()[1] + (line.P2()[1] - line.P1()[1]) * (rng.range(0).min() - line.P1()[0]) / (line.P2()[0] - line.P1()[0]);
      } else if(oc & ContainCodeT::BOTTOM) {
        np[0] = rng.range(0).max();
        np[1] = line.P1()[1] + (line.P2()[1] - line.P1()[1]) * (rng.range(0).max() - line.P1()[0]) / (line.P2()[0] - line.P1()[0]);
      } else if(oc & ContainCodeT::RIGHT) {
        np[0] = line.P1()[0] + (line.P2()[0] - line.P1()[0]) * (rng.range(1).max() - line.P1()[1]) / (line.P2()[1] - line.P1()[1]);
        np[1] = rng.range(1).max();
      } else {// CLEFT
        np[0] = line.P1()[0] + (line.P2()[0] - line.P1()[0]) * (rng.range(1).min() - line.P1()[1]) / (line.P2()[1] - line.P1()[1]);
        np[1] = rng.range(1).min();
      }
      if(oc == oc0) {
        line.P1() = np;
        oc0 = containsCode(line.P1(), rng);
      } else {
        line.P2() = np;
        oc1 = containsCode(line.P2(), rng);
      }
    } while(true);
    return accept;
  }

  template <typename RealT>
  bool Line2PP<RealT>::IsPointIn(const Point<RealT, 2> &pnt, RealT tolerance) const
  {
    if(!IsPointOn(pnt, tolerance))
      return false;

    // If ab not vertical, check betweenness on x; else on y.
    if(this->P1()[0] != this->P2()[0])
      return ((this->P1()[0] <= pnt[0]) && (pnt[0] <= this->P2()[0]))
        || ((this->P1()[0] >= pnt[0]) && (pnt[0] >= this->P2()[0]));
    else
      return ((this->P1()[1] <= pnt[1]) && (pnt[1] <= this->P2()[1]))
        || ((this->P1()[1] >= pnt[1]) && (pnt[1] >= this->P2()[1]));
  }

  template <typename RealT>
  Point<RealT, 2> Line2PP<RealT>::Intersection(const Line2PP &l) const
  {
    Vector<RealT, 2> n1(perpendicular(this->direction()));
    Vector<RealT, 2> n2(perpendicular(l.direction()));
    RealT d1 = -dot(n1, this->FirstPoint())();
    RealT d2 = -dot(n2, l.FirstPoint())();
    RealT det = cross(n1, n2);
    if(isNearZero(det))
      return Point<RealT, 2>({0.0, 0.0});
    return toPoint<RealT>((n1[1] * d2 - n2[1] * d1) / det,
                          (n2[0] * d1 - n1[0] * d2) / det);
  }

  template <typename RealT>
  bool Line2PP<RealT>::Intersection(const Line2PP &l, Point<RealT, 2> &here) const
  {
    Vector<RealT, 2> n1(perpendicular(this->direction()));
    Vector<RealT, 2> n2(perpendicular(l.direction()));
    RealT det = cross(n1, n2);
    if(isNearZero(det))
      return false;
    RealT d1 = -dot(n1, (this->FirstPoint()))();
    RealT d2 = -dot(n2, l.FirstPoint())();
    here[0] = (n1[1] * d2 - n2[1] * d1) / det;
    here[1] = (n2[0] * d1 - n1[0] * d2) / det;
    return true;
  }

  template <typename RealT>
  [[nodiscard]] std::optional<Point<RealT, 2>> Line2PP<RealT>::innerIntersection(const Line2PP &l) const
  {
    RealT p = ParIntersection(l);
    if(p < RealT(0) || p > RealT(1))
      return std::nullopt;
    return this->P1() + this->direction() * p;
  }

  template <typename RealT>
  bool Line2PP<RealT>::IntersectRow(RealT row, RealT &col) const
  {
    Vector<RealT, 2> dir = this->P2() - this->P1();
    row -= this->P1()[0];
    if(dir[0] == 0)
      return false;
    col = ((row * dir[1]) / dir[0]) + this->P1()[1];
    return true;
  }

  template <typename RealT>
  RealT Line2PP<RealT>::ParIntersection(const Line2PP &l) const
  {
    auto u2P = toPoint<RealT>(l.point[0][1] - l.point[1][1], l.point[1][0] - l.point[0][0]);// u2p = l.direction().Perpendicular();
    return (dot((l.FirstPoint() - this->FirstPoint()), u2P) / dot(this->direction(), u2P))();
  }

  template <typename RealT>
  bool Line2PP<RealT>::HasInnerIntersection(const Line2PP &l) const
  {
    RealT t = ParIntersection(l);
    return t >= 0 && t <= 1;
  }

  template <typename RealT>
  RealT Line2PP<RealT>::DistanceWithin(const Point<RealT, 2> &pt) const
  {
    RealT t = this->ParClosest(pt);
    if(t < RealT(0)) return euclidDistance<RealT, 2>(this->P1(), pt);
    if(t > RealT(1)) return euclidDistance<RealT, 2>(this->P2(), pt);
    return this->Distance(pt);
  }

  template class Line2PP<double>;
  template class Line2PP<float>;

}// namespace Ravl2
