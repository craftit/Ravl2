// This file is part of RAVL, Recognition And Vision Library 
// Copyright (C) 2003, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here
#ifndef RAVL_OPTIMISE_HH
#define RAVL_OPTIMISE_HH
////////////////////////////////////////////////////////////////////////////
//! author="Robert Crida"
//! date="9/2/1998"
//! example=testOptimise.cc
//! docentry="Ravl.API.Pattern Recognition.Optimisation"

#include "Ravl2/RCHandleV.hh"
#include "Ravl2/String.hh"
#include "Ravl2/Vector.hh"
#include "Ravl2/PatternRec/Cost.hh"

namespace Ravl2 {
  
  // --------------------------------------------------------------------------
  // **********  OptimiseBodyC  ***********************************************
  // --------------------------------------------------------------------------
  //: Implementation base class for numerical optimisation functions.
  //
  // Class hierarchy for numerical optimisation in the PatternRec set of tools.
  // You should not use this class directly but rather the handle class
  // OptimiseC.
  
  class OptimiseBodyC: public RCBodyVC
  {
  public:
    OptimiseBodyC (const XMLFactoryContextC & factory);
    //: Constructor from xml factory.

    OptimiseBodyC (const std::string &name);
    //: Constructor with derived class name
    // The name is provided by the concrete class at instantiation so that the
    // base class knows what derived type it is.

    OptimiseBodyC (const std::string &name, std::istream &in);
    //: Constructs from stream with derived class name
    // The name or type must be known so that the correct virtual constructor
    // can be called.
    
    OptimiseBodyC (std::istream &strm);
    //: Load from stream.
    
    OptimiseBodyC (Binstd::unique_ptr<std::istream> &strm);
    //: Load from stream.
    
    virtual VectorT<RealT> MinimalX (const CostC &domain, RealT &minimumCost) const;
    //: Determines Xmin=arg min_{X} domain(X)
    //! @param  domain      - the cost function that will be minimised
    //! @param  minimumCost - the maximum cost value found
    //! @return  the X value which gives the minimum cost
    // A minimisation algorithm must be provided for each derived optimisation
    // algorithm. It is not necessary to provide one for maximisation since
    // that is achieved using a cost function inverter as described in the next
    // member function.
    
    virtual VectorT<RealT> MinimalX (const CostC &domain, RealT startCost, RealT &minimumCost) const;
    //: Determines Xmin=arg min_{X} domain(X)
    //! @param  domain      - the cost function that will be minimised
    //! @param  startCost   - the cost at the start position.
    //! @param  minimumCost - the maximum cost value found
    //! @return  the X value which gives the minimum cost
    // A minimisation algorithm must be provided for each derived optimisation
    // algorithm. It is not necessary to provide one for maximisation since
    // that is achieved using a cost function inverter as described in the next
    // member function.
    
    virtual VectorT<RealT> MaximalX (const CostC &domain, RealT &maximumCost) const;
    //: Determines Xmax=arg max_{X} domain(X)
    //! @param  domain - the cost function that will be maximised
    //! @param  maximumCost - the maximum cost value found
    //! @return  the X value which gives the maximum cost
    
    virtual VectorT<RealT> MaximalX (const CostC &domain, RealT startCost, RealT &maximumCost) const;
    //: Determines Xmax=arg max_{X} domain(X)
    //! @param  domain - the cost function that will be maximised
    //! @param  startCost   - the cost at the start position, to save it being computed.
    //! @param  maximumCost - the maximum cost value found
    //! @return  the X value which gives the maximum cost
    
    virtual const std::string GetInfo () const;
    //: Prints derived class information
    
    const std::string GetName () const;
    //: Derived class type
    
    virtual bool Save (std::ostream &out) const;
    //: Writes object to stream, can be loaded using constructor
    
    virtual bool Save (Binstd::unique_ptr<std::ostream> &out) const;
    //: Writes object to stream, can be loaded using constructor
    
  protected:
    std::string _name;
  };
  
  // --------------------------------------------------------------------------
  // **********  OptimiseC  ************************************************
  // --------------------------------------------------------------------------
  //: Handle class for numerical optimisers
  //
  // Handle class for numerical optimisers. It is used in conjunction with cost
  // functions derived from NumCostC. The cost function is used to specify the
  // range of parameters for optimisation etc. The optimiser is simply used by
  // providing a cost function and requesting the X vector that minimises or
  // maximises the cost function given a guess at the solution. See any of the
  // derived optimisers for specific algorithm implementations.
  
  class OptimiseC
    : public RCHandleVC<OptimiseBodyC>
  {
  public:
    OptimiseC ();
    //: Default constructor
    
    OptimiseC (std::istream &in);
    //: Constructs from stream
    
    OptimiseC (OptimiseBodyC &oth);
    //: Constructs from base class

    OptimiseC (OptimiseBodyC *oth);
    //: Constructs from base class

    OptimiseC (const RCHandleVC<OptimiseBodyC> &opt)
     : RCHandleVC<OptimiseBodyC>(opt)
    {}
    //: Constructs from base handle

  protected:
    inline OptimiseBodyC & Body ()
    { return RCHandleC<OptimiseBodyC>::Body(); }
    
    inline const OptimiseBodyC & Body () const
    { return RCHandleC<OptimiseBodyC>::Body(); }
    

  public:
    
    inline VectorT<RealT> MinimalX (const CostC &domain, RealT startCost, RealT &minimumCost) const
    { return Body().MinimalX (domain,startCost,minimumCost); }
    //: Do the Optimisation.
    //! @param  domain      - the cost function that will be minimised
    //! @param  startCost   - the cost at the start position.
    //! @param  minimumCost - the maximum cost value found
    //! @return  the X value which gives the minimum cost
    // Determines which X gives minimum cost function value and gives access to
    // calculated minimum cost

    inline VectorT<RealT> MinimalX (const CostC &domain, RealT &minimumCost) const
    { return Body().MinimalX (domain,minimumCost); }
    //: Do the Optimisation.
    //! @param  domain      - the cost function that will be minimised
    //! @param  minimumCost - the maximum cost value found
    //! @return  the X value which gives the minimum cost
    // Determines which X gives minimum cost function value and gives access to
    // calculated minimum cost

    inline VectorT<RealT> MinimalX (const CostC &domain) const
    { RealT minimumCost; return Body().MinimalX (domain,minimumCost); }
    //: Do the Optimisation. Determines which X gives minimum cost function value
    //! @param  domain      - the cost function that will be minimised
    //! @return  the X value which gives the minimum cost
    
    inline VectorT<RealT> MaximalX (const CostC &domain, RealT startCost, RealT &maximumCost) const
    { return Body().MaximalX (domain,startCost,maximumCost); }
    //: Do the Optimisation.
    //! @param  domain - the cost function that will be maximised
    //! @param  startCost   - the cost at the start position, to save it being computed.
    //! @param  maximumCost - the maximum cost value found
    //! @return  the X value which gives the maximum cost
    // Determines which X gives maximum cost function value and gives access to 
    // calculated maximum cost

    inline VectorT<RealT> MaximalX (const CostC &domain, RealT &maximumCost) const
    { return Body().MaximalX (domain,maximumCost); }
    //: Do the Optimisation.
    //! @param  domain - the cost function that will be maximised
    //! @param  maximumCost - the maximum cost value found
    //! @return  the X value which gives the maximum cost
    // Determines which X gives maximum cost function value and gives access to 
    // calculated maximum cost

    inline VectorT<RealT> MaximalX (const CostC &domain) const
    { RealT maximumCost; return Body().MaximalX (domain,maximumCost); }
    //: Do the Optimisation. Determines which X gives maximum cost function value
    //! @param  domain - the cost function that will be maximised
    //! @return  the X value which gives the maximum cost
    
    inline const std::string GetInfo () const
    { return Body().GetInfo (); }
    //: Gets string describing object
    
    inline const std::string GetName () const
    { return Body().GetName (); }
    //: Gets type name of the object
    
    inline bool Save (std::ostream &out) const
    { return Body().Save (out); }
    //: Writes object to stream, can be loaded using constructor
  };
  
}

#endif
