// This file is part of RAVL, Recognition And Vision Library 
// Copyright (C) 2002, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here
//! author="Radek Marik"
//! date="26/10/1992"

#pragma once

#include "Ravl2/Array.hh"
#include "Ravl2/Image/Segmentation/Crack.hh"
#include "Ravl2/Image/Array2Sqr2Iter.hh"
#include "Ravl2/Index.hh"

namespace Ravl2
{
  class Polygon2dC;
  
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
    BoundaryC(const std::vector<CrackC> & edgeList, bool orient = true)
     : orientation(orient), mEdges(edgeList)
    {}

    //! Empty boundary with orientation 'orient'.
    //! If orient is true, the object is on the left of the boundary.
    BoundaryC(bool orient = true)
      : orientation(orient)
    {}

    //! Creates an unsorted list of boundary elements (CrackC) from the edges between 'inLabel' pixels and other values
    template<typename PixelT>
    BoundaryC(const Array<PixelT,2> &emask,PixelT inLabel)
      : orientation(false)
    {
      if(emask.Frame().Rows() < 3 || emask.Frame().Cols() < 3) {
        std::cerr << "RegionMaskBodyC::Boundary(), Mask too small to compute boundary. \n";
        return;
      }
      mEdges.reserve((emask.range(0).size() + emask.range(1).size())*2);
      for(Array2dSqr2IterC<PixelT> it(emask);it;it++) {
        if(it.DataBR() == inLabel) {
          if(it.DataTR() != inLabel)
            mEdges.emplace_back(it.Index(),CrackCodeT::CR_LEFT);
          if(it.DataBL() != inLabel)
            mEdges.emplace_back(it.Index(),CrackCodeT::CR_DOWN);
        } else {
          if(it.DataTR() == inLabel)
            mEdges.emplace_back(it.Index()+Index<2>({-1,0}),CrackCodeT::CR_RIGHT);
          if(it.DataBL() == inLabel)
            mEdges.emplace_back(it.Index()+Index<2>({0,-1}),CrackCodeT::CR_UP);
        }
      }
    }


    //: Creates an unsorted list of boundary elements (CrackC) from the edges between 'inLabel' pixels and other values
    // <code>emask</code> could be for example the image returned by the
    // <code><a href= "RavlImageN.SegmentationC.html#SegMapObvoidCb">
    // SegmentationC::SegMap()</a> method.

    template<typename PixelT>
    static BoundaryC trace(const Array<PixelT,2> &emask,PixelT inLabel)
    {
      if(emask.Frame().Rows() < 3 || emask.Frame().Cols() < 3) {
        std::cerr << "RegionMaskBodyC::Boundary(), Mask too small to compute boundary. \n";
        return BoundaryC();
      }
      std::vector<CrackC> edges;
      for(Array2dSqr3IterC<Uint> it(emask);it;it++) {
        if(it.DataMM() != (Uint)inLabel)
          continue;
        if(it.DataMR() != (Uint)inLabel)
          InsLast(CrackC(it.Index(),CR_UP));
        if(it.DataML() != (Uint)inLabel)
          InsLast(CrackC(it.Index(),CR_DOWN));
        if(it.DataTM() != (Uint)inLabel)
          InsLast(CrackC(it.Index(),CR_LEFT));
        if(it.DataBM() != (Uint)inLabel)
          InsLast(CrackC(it.Index(),CR_RIGHT));
      }
      return BoundaryC(edges,false);
    }


    BoundaryC(const DListC<DLIterC<CrackC> > & edgeList, bool orient = true);
    //: Creates the boundary from the list of pointers to the elementary edges.
    // If orient is true, the object is on the left of the boundary.
    
    BoundaryC(const IndexRange<2> & rect,bool asHole = true);
    //: The boundary of the rectangle.
    // The boundary goes clockwise around the rectangle. If asHole is true, 
    // then the rectangle is 'outside' the region. otherwise its inside. 
    
    int Area() const;
    //: Get the area of the region which is determined by the 'boundary'.
    // Note: The area of the region can be negative, if it is a 'hole' in
    // a plane. This can be inverted with the BReverse() method.
    
    DListC<BoundaryC> OrderEdges() const;
    //: Generate a list of boundaries.
    // Each item in the list corresponds to a single complete boundary contour.<br>
    // The edges in each boundary are ordered along the boundary.<br> 
    // The direction of the boundaries is determined by the constructor.<br>
    // Boundaries that terminate at the edge of the array/image are left open.
    
    DListC<BoundaryC> Order(const CrackC & firstCrack, bool orient = true);
    //!deprecated: Order boundary from edge. <br> Order the edgels of this boundary such that it can be traced continuously along the direction of the first edge. The orientation of the boundary is set according to 'orient'. If the boundary is open, 'firstCrack' and 'orient' are ignored.<br>  Note: There is a bug in this code which can cause an infinite loop for some edge patterns. In particular where the two edges go through the same vertex.
    
    bool Orient() const
    { return orientation; }
    //: Return the orientation of the boundary.
    // true: object is on the left side of edges relative to their
    // direction;<br> false: on the right.
    
    void Invert()
    { orientation = !orientation; }
    //: Invert the boundary.
    
    BoundaryC & BReverse();
    //: Reverse the order of the edges.
    
    DListC<DLIterC<CrackC> > ConvexHull() const;
    //: Compute the convex hull.
    // The convex hull is created from the original Jordan boundary using
    // Melkman's method.
    // Ref.: A V Melkman. On-line construction of the convex hull of a 
    //                    simple polyline. Information Processing Letters,
    //                    25(1):11-12, 1987.
    // Ref.: M Sonka, V Hlavac: Image Processing.    
    
    IndexRange<2> BoundingBox() const;
    //: Get the bounding box around all pixels on the inside edge of the boundry.
    
    Polygon2dC Polygon2d(bool bHalfPixelOffset = false) const;
    //: Convert a boundry to a polygon.
    //!param: bHalfPixelOffset - should (-0.5,-0.5) be added to the polygon points?
    // This assumes 'bnd' is a single ordered boundry (See BoundryC::OrderEdges();). 
    // Straight edges are compressed into a single segment.
    
  private:s
    //! Orientation of the boundary. true means that
    //! an object is on the left side of edges.
    bool orientation = true;

    std::vector<CrackC> mEdges;

    RCHashC<BVertexC, PairC<BVertexC> > CreateHashtable() const;
    // Returns the hash table for the boundary; all end points which are only
    // connected to one other point will have at least one invalid
    // neighbour (-1, -1).

    BoundaryC OrderContinuous(const RCHashC<BVertexC, PairC<BVertexC> > & hashtable, const CrackC & firstCrack, bool orient) const;
    // Returns a continuous boundary; if the boundary is open, 'orient' will be 
    // ignored and 'firstCrack' must be one of the end points of the boundary.
    
    DListC<BVertexC> FindEndpoints(const RCHashC<BVertexC, PairC<BVertexC> > & hashtable) const;
    // Returns the endpoints of the boundary, i.e. if the boundary is closed,
    // the list will be empty.
  };
  
//  inline std::ostream & operator<<(std::ostream & s, const BoundaryC & b)
//  { s << (DListC<CrackC> &) b; return s; }
//  //: Write the boundary to output stream 's'.
//
//  inline std::istream & operator<<(std::istream & s,BoundaryC & b)
//  { s >> (DListC<CrackC> &) b; return s;  }
//  //: Read the boundary from input stream 's'.
  
  BoundaryC Line2Boundary(const BVertexC & startVertex, const BVertexC & endVertex);
  //: Creates a boundary which connects both boundary vertexes.

}


