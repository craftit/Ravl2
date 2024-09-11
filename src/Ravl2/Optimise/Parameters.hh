// This file is part of RAVL, Recognition And Vision Library 
// Copyright (C) 2003, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here
#ifndef RAVL_PARAMETERS_HH
#define RAVL_PARAMETERS_HH
////////////////////////////////////////////////////////////////////////////
//! author="Robert Crida"
//! date="10/2/1998"
//! example=testCost.cc
//! docentry="Ravl.API.Pattern Recognition.Optimisation.Cost Functions"

#include "Ravl2/Vector.hh"
#include "Ravl2/Matrix.hh"

namespace Ravl2 {

  // --------------------------------------------------------------------------
  // **********  ParametersBodyC  *********************************************
  // --------------------------------------------------------------------------
  //: Implementation class for optimisation parameter bounds.
  //
  // This is the implementation class for optimisation parameter bounds. The
  // ParametersC handle class should be used.
  
  class ParametersBodyC
    : public RCBodyC
  {
  public:
    ParametersBodyC (const VectorT<RealT> &minP, 
		     const VectorT<RealT> &maxP, 
		     const std::vector<IntT> &steps);
    //: Constructor
    //! @param  minP  - lower bound on function input
    //! @param  maxP  - upper bound on function input
    //! @param  steps - number of steps to use with each parameter (if applicable)
    // All parameters are enabled in optimisation.
    
    ParametersBodyC (const VectorT<RealT> &minP, 
		     const VectorT<RealT> &maxP, 
		     const std::vector<IntT> &steps,
		     const std::vector<IntT> &mask);
    //: Constructor
    //! @param  minP  - lower bound on function input
    //! @param  maxP  - upper bound on function input
    //! @param  steps - number of steps to use with each parameter (if applicable)
    //! @param  mask  - specifies which elements of P to use in X
    // Only the parameters with a mask value of 1 are presented to the optimiser.
    
    ParametersBodyC (size_t nparams,bool unlimited = false);
    //: Constructor.
    // This setup nparams with defaults settings of :
    // minP=0 maxP=1 Steps=1 mask=0 (constP = 0)
    // if unlimited is true parameters can be any real number, and all parameters are enabled.
    
    ParametersBodyC (const ParametersBodyC &other);
    //: Copy Constructor.
    
    ParametersBodyC (std::istream &in);
    //: Constructs for a stream
    
    ParametersBodyC & Copy () const;
    //: Makes a deep copy
    
    VectorT<RealT> Random();
    //: Generate a random position in the parameter space.
    
  protected:
    inline size_t size() const
    { return _minP.size(); }
    //: Get number of parameters in set.
    
    void SetMask (const std::vector<IntT> &mask);
    //: Changes the parameter mask
    
    void SetConstP (const VectorT<RealT> &constP);
    //: Specifies the values to use for constant parameters
    
    const VectorT<RealT> &MinX () const {
      if(m_cacheDirty) UpdateCache();
      return m_minX;
    }
    //: Lower parameter bound for optimisation
    
    const VectorT<RealT> &MaxX () const {
      if(m_cacheDirty) UpdateCache();
      return m_maxX; 
    }
    //: Upper parameter bound for optimisation
    
    const std::vector<IntT> &Steps () const {
      if(m_cacheDirty) UpdateCache();      
      return m_stepsP;
    }
    //: Number of steps in each parameter for optimisation

    VectorT<RealT> TransP2X (const VectorT<RealT> &inVec) const;
    //: Transformation between P and X

    VectorT<RealT> TransX2P (const VectorT<RealT> &inVec) const;
    //: Transformation between X and P

    Tensor<RealT,2> TransP2X (const Tensor<RealT,2> &inMat) const;
    //: Transformation between P and X
    // Equivelent of inMat * TransX2P ()
    
    const VectorT<RealT> &ConstP () const {
      if(m_cacheDirty) UpdateCache();
      return m_constP;
    }
    //: Vector with constant P elements set, else 0
    
    const VectorT<RealT> &StartX () const {
      if(m_cacheDirty) UpdateCache();
      return m_startX;
    }
    //: Returns initial parameter value which is TransP2X * constP
    
    void Setup(int p,RealT min,RealT max,IntT steps,IntT mask = 1);
    //: Setup parameter p.
    
    void Setup(int p,RealT min,RealT max,IntT steps,RealT constV,IntT mask = 1);
    //: Setup parameter p, and constant value.
    
    void Save (std::ostream &out) const;
    //: Saves to stream, from which can be constructed
    
    friend class ParametersC;
    //: Handle class
    
  private:
    VectorT<RealT> _minP;
    VectorT<RealT> _maxP;
    VectorT<RealT> _constP;
    std::vector<IntT> _steps;
    std::vector<IntT> _mask;

    void UpdateCache() const;
    
    mutable unsigned m_sizeX;
    mutable bool m_cacheDirty;
    mutable std::vector<unsigned> m_maskMap;
    mutable VectorT<RealT> m_minX;
    mutable VectorT<RealT> m_maxX;
    mutable VectorT<RealT> m_constP;
    mutable VectorT<RealT> m_startX;
    mutable std::vector<IntT> m_stepsP;
  };
  
  //////////////////////////////////////////
  
  // --------------------------------------------------------------------------
  // **********  ParametersC  *************************************************
  // --------------------------------------------------------------------------
  //: Handle class for optimisation parameter bounds.
  //
  // Handle class for parameter bounds class. Used to specify constraints for
  // the optimiser on starting conditions, parameter boundaries and resolution
  // for each parameter. Also used to fix certain parameters as constants during
  // optimisation. this is achieved as follows: A mask is used to disable some
  // parameters which will then be treated by the cost function as constants
  // and become invisible to the optimiser.<p>
  //
  // Note that there is a distinction made here between the vector X which is
  // input to the cost function and optimised and the input parameters to the
  // function P. X is a subset of P.
  
  class ParametersC : public RCHandleC<ParametersBodyC>
  {
  public:
    inline ParametersC ()
    {}
    //: Default constructor
    
    ParametersC (const VectorT<RealT> &minP, 
		 const VectorT<RealT> &maxP, 
		 const std::vector<IntT> &steps)
      : RCHandleC<ParametersBodyC>(*new ParametersBodyC (minP,maxP,steps))
    {}
    //: Constructor
    //! @param  minP  - lower bound on function input
    //! @param  maxP  - upper bound on function input
    //! @param  steps - number of steps to use with each parameter (if applicable)
    // All parameters are enabled in optimisation.
    
    ParametersC (const VectorT<RealT> &minP, 
		 const VectorT<RealT> &maxP, 
		 const std::vector<IntT> &steps,
		 const std::vector<IntT> &mask)
      : RCHandleC<ParametersBodyC>(*new ParametersBodyC (minP,maxP,steps,mask))
    {}
    //: Constructor
    //! @param  minP  - lower bound on function input
    //! @param  maxP  - upper bound on function input
    //! @param  steps - number of steps to use with each parameter (if applicable)
    //! @param  mask  - specifies which elements of P to use in X
    // Only the parameters with a mask value of 1 are presented to the optimiser.
    
    ParametersC (size_t nparams,bool unlimited = false)
      : RCHandleC<ParametersBodyC>(*new ParametersBodyC (nparams,unlimited))
    {}
    //: Constructor.
    // This setup nparams with defaults settings of :
    // minP=0 maxP=1 Steps=1 mask=0 (constP = 0)
    // if unlimited is true parameters can be any real number, and all parameters are enabled.
    
    ParametersC (std::istream &in)
      : RCHandleC<ParametersBodyC>(*new ParametersBodyC (in))
    {}
    //: Constructs from stream
    
    inline size_t size() const
    { return Body().size(); }
    //: Get number of parameters in set.
    
    inline void SetMask (const std::vector<IntT> &mask)
    { Body().SetMask (mask); }
    //: Sets which parameters are enabled
    
    inline void SetConstP (const VectorT<RealT> &constP)
    { Body().SetConstP (constP); }
    //: Sets const parameter values and starting point for enabled ones
    
    inline const VectorT<RealT> &MinX () const
    { return Body().MinX (); }
    //: Lower bound on optimisation parameters
    
    inline const VectorT<RealT> &MaxX () const
    { return Body().MaxX (); }
    //: Upper bound on optimisation parameters
    
    inline const std::vector<IntT> &Steps () const
    { return Body().Steps (); }
    //: Number of steps to use for each dimension

    inline const VectorT<RealT> &ConstP () const
    { return Body().ConstP(); }
    //: Vector containing constant P elements and 0s
    
    inline const VectorT<RealT> &StartX () const
    { return Body().StartX (); }
    //: Starting vector for X which is subset of value specified in SetConstP.

    VectorT<RealT> TransP2X (const VectorT<RealT> &inVec) const
    { return Body().TransP2X(inVec); }
    //: Transformation between P and X

    VectorT<RealT> TransX2P (const VectorT<RealT> &inVec) const
    { return Body().TransX2P(inVec); }
    //: Transformation between X and P

    Tensor<RealT,2> TransP2X (const Tensor<RealT,2> &inVec) const
    { return Body().TransP2X(inVec); }
    //: Transformation between P and X
    // Equivelent of inMat * TransX2P ()

    inline void Setup(int p,RealT min,RealT max,IntT steps,IntT mask = 1)
    { Body().Setup(p,min,max,steps,mask); }
    //: Setup parameter p.
    
    inline void Setup(int p,RealT min,RealT max,IntT steps,RealT constV,IntT mask = 1)
    { Body().Setup(p,min,max,steps,constV,mask); }
    //: Setup parameter p, and constant value.
    
    inline void Save (std::ostream &out) const
    { Body().Save (out); }
    //: Writes object to stream, can be loaded using constructor
    
    inline VectorT<RealT> Random()
    { return Body().Random(); }
    //: Generate a random position in the parameter space.
  };
  
}

#endif
