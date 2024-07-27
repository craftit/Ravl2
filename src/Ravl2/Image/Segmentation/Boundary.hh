// This file is part of RAVL, Recognition And Vision Library 
// Copyright (C) 2002, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here
//! author="Radek Marik"
//! date="26/10/1992"

#pragma once

#include <array>
#include <unordered_map>
#include "Ravl2/Array.hh"
#include "Ravl2/Image/Segmentation/Crack.hh"
#include "Ravl2/Image/Array2Sqr2Iter.hh"
#include "Ravl2/Index.hh"

namespace Ravl2
{
  class Polygon2dC;

  enum class BoundaryTypeT { OUTSIDE, INSIDE };

  //! Crack code boundary

  // <p>The class BoundaryC represents the 4-connected oriented boundary of an
  // image/array region. If the image is interpreted as an array of pixels in
  // the form of square tiles, the boundary follows the edges of the tiles.</p>
  //
  // <p>If the region has holes in it, or several regions are labelled with the
  // same label, the object will include all of the relevant boundaries. These
  // can be separated using the <code>OrderEdges()</code> method.</p>

  class BoundaryC
  {
  public:

    //! Create the boundary from the list of edges with an appropriate orientation.
    // The 'edgeList' will be a part of boundary.  If orient is true, the object
    // is on the left of the boundary.
    explicit BoundaryC(const std::vector<CrackC> & edgeList, bool orient = false)
     : orientation(orient), mEdges(edgeList)
    {}

    //! Create the boundary from the list of edges with an appropriate orientation.
    // The 'edgeList' will be a part of boundary.  If orient is true, the object
    // is on the left of the boundary.
    explicit BoundaryC(std::vector<CrackC> && edgeList, bool orient = false)
      : orientation(orient), mEdges(std::move(edgeList))
    {}

    //! Empty boundary with orientation 'orient'.
    //! If orient is true, the object is on the left of the boundary.
    explicit BoundaryC(bool orient)
      : orientation(orient)
    {}

    BoundaryC() = default;
    BoundaryC(const BoundaryC &) = default;
    BoundaryC(BoundaryC &&) = default;
    BoundaryC & operator=(const BoundaryC &) = default;
    BoundaryC & operator=(BoundaryC &&) = default;

    //! Creates an unsorted list of boundary elements (CrackC) from the edges between 'inLabel' pixels and other values

    template<typename ArrayT,typename DataT = typename ArrayT::value_type>
    requires WindowedArray<ArrayT,DataT,2>
    static BoundaryC traceBoundary(const ArrayT &emask, DataT inLabel);

    //BoundaryC(const std::vector<DLIterC<CrackC> > & edgeList, bool orient = true);
    //: Creates the boundary from the list of pointers to the elementary edges.
    // If orient is true, the object is on the left of the boundary.

    int area() const;
    //: Get the area of the region which is determined by the 'boundary'.
    // Note: The area of the region can be negative, if it is a 'hole' in
    // a plane. This can be inverted with the BReverse() method.

    std::vector<BoundaryC> OrderEdges() const;
    //: Generate a list of boundaries.
    // Each item in the list corresponds to a single complete boundary contour.<br>
    // The edges in each boundary are ordered along the boundary.<br> 
    // The direction of the boundaries is determined by the constructor.<br>
    // Boundaries that terminate at the edge of the array/image are left open.

    std::vector<BoundaryC> Order(const CrackC & firstCrack, bool orient = true);
    //!deprecated: Order boundary from edge. <br> Order the edgels of this boundary such that it can be traced continuously along the direction of the first edge. The orientation of the boundary is set according to 'orient'. If the boundary is open, 'firstCrack' and 'orient' are ignored.<br>  Note: There is a bug in this code which can cause an infinite loop for some edge patterns. In particular where the two edges go through the same vertex.

    [[nodiscard]] bool Orient() const
    { return orientation; }
    //: Return the orientation of the boundary.
    // true: object is on the left side of edges relative to their
    // direction;<br> false: on the right.

    void Invert()
    { orientation = !orientation; }
    //: Invert the boundary.

    //! Reverse the order of the edges.
    [[nodiscard]] BoundaryC reverse() const;

    //! Reverse the order of the edges.
    BoundaryC & BReverse();

    //! Compute the convex hull.
    // The convex hull is created from the original Jordan boundary using
    // Melkman's method.
    // Ref.: A V Melkman. On-line construction of the convex hull of a
    //                    simple polyline. Information Processing Letters,
    //                    25(1):11-12, 1987.
    // Ref.: M Sonka, V Hlavac: Image Processing.
    std::vector<std::vector<CrackC> > ConvexHull() const;

    //! Get the bounding box around all pixels on the inside edge of the boundary.
    IndexRange<2> BoundingBox() const;

    Polygon2dC Polygon2d(bool bHalfPixelOffset = false) const;
    //: Convert a boundry to a polygon.
    //!param: bHalfPixelOffset - should (-0.5,-0.5) be added to the polygon points?
    // This assumes 'bnd' is a single ordered boundry (See BoundryC::OrderEdges();). 
    // Straight edges are compressed into a single segment.

    //! Get number of endges in the boundry
    [[nodiscard]] auto size() const
    { return mEdges.size(); }

    //! Check if the boundary is empty.
    [[nodiscard]] bool empty() const
    { return mEdges.empty(); }

    //! Get edges
    [[nodiscard]] const std::vector<CrackC> & edges() const
    { return mEdges; }

  private:
    //! Orientation of the boundary. true means that
    //! an object is on the left side of edges.
    bool orientation = true;

    std::vector<CrackC> mEdges;

    std::unordered_map<BVertexC, std::array<BVertexC,2> > CreateHashtable() const;
    // Returns the hash table for the boundary; all end points which are only
    // connected to one other point will have at least one invalid
    // neighbour (-1, -1).

    BoundaryC OrderContinuous(const std::unordered_map<BVertexC, std::array<BVertexC,2> > & hashtable, const CrackC & firstCrack, bool orient) const;
    // Returns a continuous boundary; if the boundary is open, 'orient' will be 
    // ignored and 'firstCrack' must be one of the end points of the boundary.

    std::vector<BVertexC> FindEndpoints(const std::unordered_map<BVertexC, std::array<BVertexC,2> > & hashtable) const;
    // Returns the endpoints of the boundary, i.e. if the boundary is closed,
    // the list will be empty.
  };

  //! Write out the boundary to a stream.
  std::ostream & operator<<(std::ostream & os, const BoundaryC & bnd);

  //: Creates a boundary which connects both boundary vertexes.
  BoundaryC Line2Boundary(const BVertexC & startVertex, const BVertexC & endVertex);

  //: Creates a boundary around the rectangle.
  BoundaryC toBoundary(IndexRange<2> rect, BoundaryTypeT type = BoundaryTypeT::OUTSIDE);

  template<typename ArrayT,typename DataT>
  requires WindowedArray<ArrayT,DataT,2>
  BoundaryC BoundaryC::traceBoundary(const ArrayT &emask, DataT inLabel)
  {
    BoundaryC ret;
    if(emask.range(0).size() < 3 || emask.range(1).size() < 3) {
      SPDLOG_ERROR("RegionMaskBodyC::Boundary(), Mask too small to compute boundary. ");
      return {};
    }
    std::vector<CrackC> mEdges;
    mEdges.reserve(size_t((emask.range(0).size() + emask.range(1).size()) * 2));
    for(Array2dSqr2IterC<DataT> it(emask); it.valid(); ++it) {
      if(it.DataBR() == inLabel) {
	// TL       TR(0)
	//          <-
	// BL(0) \/ BR(1)
        if(it.DataTR() != inLabel)
          mEdges.emplace_back(it.indexBR() + toIndex(0,1), CrackCodeT::CR_LEFT);
        if(it.DataBL() != inLabel)
          mEdges.emplace_back(it.indexBR(), CrackCodeT::CR_DOWN);
      } else {
	// TL       TR(1)
	//           ->
	// BL(1) /\ BR(0)
	if(it.DataTR() == inLabel)
          mEdges.emplace_back(it.indexBR(), CrackCodeT::CR_RIGHT);
        if(it.DataBL() == inLabel)
          mEdges.emplace_back(it.indexBR() + toIndex(1,0), CrackCodeT::CR_UP);
      }
    }
    return BoundaryC(std::move(mEdges),true);
  }

}

namespace fmt
{
  template<> struct formatter<Ravl2::BoundaryC> : ostream_formatter {};
}



