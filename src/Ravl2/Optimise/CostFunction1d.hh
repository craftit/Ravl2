// This file is part of RAVL, Recognition And Vision Library 
// Copyright (C) 2003, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here
#ifndef RAVL_COSTFUNCTION1D_HH
#define RAVL_COSTFUNCTION1D_HH
////////////////////////////////////////////////////////////////////////////
//! author="Robert Crida"
//! date="6/8/2003"
//! example=testCost.cc
//! docentry="Ravl.API.Pattern Recognition.Optimisation.Cost Functions"

#include "Ravl2/PatternRec/CostFunction.hh"

namespace Ravl2 {

  // --------------------------------------------------------------------------
  // **********  CostFunction1dBodyC  *****************************************
  // --------------------------------------------------------------------------
  //: Implementation class for 1D cost functions interfacing to CostFunctionC.
  //
  // This is the implementation class for cost functions which interface to
  // functions derived from CFunctionC. The CostFunction1dC handle class should
  // be used.
  
  class CostFunction1dBodyC: public CostBodyC
  {
  public:
    CostFunction1dBodyC (const ParametersC parameters,
                         const CostC &cost,
                         const VectorT<RealT> &point,
                         const VectorT<RealT> &direction);
    //: Constructor
    //! @param  cost   - the function to be optimised
    
    CostFunction1dBodyC (std::istream &in);
    //: Contructs from stream
    
    virtual RealT Cost (const VectorT<RealT> &X) const;
    //: Evaluate cost function at X

    virtual VectorT<RealT> Point (const VectorT<RealT> &X) const;
    //: Evaluate the input vector for the internal cost function
    //! @param  X - one dimensional input value
    
    virtual bool Save (std::ostream &out) const;
    //: Writes object to stream, can be loaded using constructor
    
    virtual RealT Apply1(const VectorT<RealT> &data) const;
    //: Apply function to 'data'

  protected:
    CostC _cost;
    VectorT<RealT> _point;
    VectorT<RealT> _direction;
  };
  
  
  //: Optimisation cost function interface to functions derived from CostFunctionC.
  //
  // This is used to make a 1D version of a CostC. Specify a fixed point
  // and direction vector then an optimiser can be used to search along the line
  // for the minimum.
  
  class CostFunction1dC: public CostC
  {
  public:
    CostFunction1dC (const ParametersC parameters,
                     const CostC &cost,
                     const VectorT<RealT> &point,
                     const VectorT<RealT> &direction)
      :CostC(*(new CostFunction1dBodyC (parameters,cost,point,direction))) {}
    //: Constructor
    //! @param  cost  - the function to be optimised
    //! @param  point     - reference point for 1D line
    //! @param  direction - direction of 1D line
    // The optimiser will return a value X which minimizes the cost.
    // Given the P = point and D = direction, then the minimal point in the
    // original CostC space is P + DX[0]. Note that X.size() == 1.

    VectorT<RealT> Point (const VectorT<RealT> &X)
    { return Body().Point(X); }
  protected:
    inline CostFunction1dBodyC & Body()
    { return static_cast<CostFunction1dBodyC&>(CostC::Body()); }
    //: Access body.
    
    inline const CostFunction1dBodyC & Body() const 
    { return static_cast<const CostFunction1dBodyC&>(CostC::Body()); }
    //: Access body.
    
  };

}

#endif
