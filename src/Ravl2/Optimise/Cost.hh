// This file is part of RAVL, Recognition And Vision Library 
// Copyright (C) 2003, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here
#ifndef RAVL_COST_HH
#define RAVL_COST_HH
////////////////////////////////////////////////////////////////////////////
//! author="Robert Crida"
//! lib=Optimisation
//! date="4/2/1998"
//! userlevel=Normal
//! example=testCost.cc
//! file="Ravl/PatternRec/Optimise/Cost.hh"
//! docentry="Ravl.API.Pattern Recognition.Optimisation.Cost Functions"

#include "Ravl/PatternRec/Function1.hh"
#include "Ravl/PatternRec/Parameters.hh"

namespace RavlN {

  // --------------------------------------------------------------------------
  // **********  CostBodyC  ***************************************************
  // --------------------------------------------------------------------------
  //: Implementation base class for optimisation cost functions.
  //
  // Class hierarchy for cost functions. It contains information about the
  // selection of input parameters and which ones are to be used for
  // optimisation. The other parameters will be held constant. Derived classes
  // include cost of the distance between a function output and a desired
  // setting, ie s = |f(X)-Yd| where the function f and the distance metric
  // can be specified. A cost function can be inverted to change optimisation
  // to give maxima instead of minima and cost functions can be combined by
  // summing them.
  
  class CostBodyC
    : public Function1BodyC
  {
  public:
    CostBodyC (const ParametersC &parameters);
    //: Constructor
    //!param: parameters - structure which describes X to P relationship
    
    CostBodyC () {
      InputSize (0);
      OutputSize (1);
    }
    //: Constructor
    
    CostBodyC (std::istream &in);
    //: Contructs from stream with derived class name
    
    const VectorC &ConstP () const
    { return _parameters.ConstP (); }
    //: Constant component of X vector

    virtual RealT Cost (const VectorC &X) const=0;
    //: Determines cost of X
    
    void SetMask (const SArray1dC<IntT> &mask);
    //: Used to specify which elements are fixed
    
    void SetConstP (const VectorC &constP)
    { _parameters.SetConstP (constP); }
    //: Used to specify the vector of fixed constant parameters
    
    const VectorC &MinX () const
    { return _parameters.MinX (); }
    //: Lower bound on variable parameters
    
    const VectorC &MaxX () const
    { return _parameters.MaxX (); }
    //: Upper bound on variable parameters
    
    const VectorC &StartX () const
    { return _parameters.StartX (); }
    //: Returns the initial point for X as non-const part of constP
    
    VectorC ConvertX2P (const VectorC &X) const
    { return _parameters.TransX2P(X); }
    //: Expands X to P using TransX2P * X + Const P

    MatrixC ConvertP2X (const MatrixC &inMat) const
    { return _parameters.TransP2X(inMat); }
    //: Transformation between X and P
    // Equivelent of inMat * TransX2P ()

    void ConvertX2P (const VectorC &X,VectorC &out) const
    { out = _parameters.TransX2P(X); }
    //: Expands X to P using TransX2P * X + Const P
    
    const SArray1dC<IntT> &Steps () const
    { return _parameters.Steps (); }
    //: Number of steps for each parameter
    
    VectorC ClipX (const VectorC &X) const;
    //: Returns the input vector clipped by the parameter range

    
    const ParametersC & GetParameters () const
    { return _parameters; }
    //: Access to the parameter storage structure

    void SetParameters(const ParametersC &parameters);
    //: Change the parameter storage structure
    
    virtual bool Save (std::ostream &out) const;
    //: Writes object to stream, can be loaded using constructor

  protected:
    ParametersC _parameters;
    
    virtual VectorC Apply(const VectorC &data) const;
    //: Apply function to 'data'
    
  };
  
  extern MatrixC numCostEmptyMatrix;
  
  // --------------------------------------------------------------------------
  // **********  CostC  ****************************************************
  // --------------------------------------------------------------------------
  //: Handle class for optimisation cost function evaluators
  //
  // Handle class for cost functions. They are to be used with classes derived
  // from OptimiseC for performing numerical optimisation tasks.<p>
  //
  // Note that there is a distinction made below between the full parameter
  // vector P and the parameters used for optimisation X. X is a subset of P.
  // The elements of P that are not in X are kept constant at values
  // specified SetConstP. The initial value of X will also be obtained from
  // SetConstP. The optimiser should only be aware of the vector X and the
  // the function being optimised should only be aware of the vector P.
  // NumCostC provides the interface between these two.
  
  class CostC
    : public Function1C
  {
  protected:
    CostC (CostBodyC &oth)
      : Function1C(oth)
    {}
    //: Body constructor
    
    inline CostBodyC & Body()
    { return static_cast<CostBodyC&>(FunctionC::Body()); }
    //: Access body.
    
    inline const CostBodyC & Body() const 
    { return static_cast<const CostBodyC&>(FunctionC::Body()); }
    //: Access body.
    
  public:
    CostC ()
    {}
    //: Default constructor
    
    CostC (std::istream &in)
      : Function1C(in)
    { CheckHandleType(Body()); }
    //: Constructs from stream
    
    CostC (CostBodyC *oth)
      : Function1C(oth)
    {}
    //: Body constructor.

    
    inline RealT Cost (const VectorC &X) const
    { return Body().Cost (X); }
    //: Calculates the cost of the current vector X
    
    inline void SetMask (const SArray1dC<IntT> &mask)
    { Body().SetMask (mask); }
    //: Sets the mask of enables parameters
    
    inline void SetConstP (const VectorC &constP)
    { Body().SetConstP (constP); }
    //: Sets the values for constant parameters.
    // The value of non-constant parameters will be used as a starting point
    
    inline const VectorC &MinX () const
    { return Body().MinX (); }
    //: Lower bound on optimisation vector X
    
    inline const VectorC &MaxX () const
    { return Body().MaxX (); }
    //: Upper bound on optimisation vector X
    
    inline const VectorC &StartX () const
    { return Body().StartX (); }
    //: Initial X value obtained from SetConstP
    
    inline VectorC ConvertX2P (const VectorC &X) const
    { return Body().ConvertX2P (X); }
    //: Converts an X vector to a full P vector

    MatrixC ConvertP2X (const MatrixC &inMat) const
    { return Body().ConvertP2X(inMat); }
    //: Transformation between X and P
    // Equivelent of inMat * TransX2P ()

    inline VectorC ClipX (const VectorC &X) const
    { return Body().ClipX (X); }
    //: Returns the input vector clipped by the parameter range
    
    inline const SArray1dC<IntT> &Steps () const
    { return Body().Steps (); }
    //: Gets number of steps to use for each dimension
    
    inline const ParametersC & GetParameters () const
    { return Body().GetParameters (); }
    //: Access to the parameter storage class
    
    inline void SetParameters(const ParametersC &parameters)
    { Body().SetParameters (parameters); }
    //: Change the parameter storage structure
  };
  
  
}

#endif
