// This file is part of RAVL, Recognition And Vision Library 
// Copyright (C) 2003, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here

#include "Ravl2/PatternRec/Parameters.hh"
#include "Ravl2/SArray1dIter5.hh"
#include "Ravl2/Random.hh"
#include "Ravl2/config.h"
#include <stdlib.h>
#include "Ravl2/StdConst.hh"
#include <spdlog/spdlog.h>

#define DODEBUG 0
#if DODEBUG
#define ONDEBUG(x) x
#else
#define ONDEBUG(x)
#endif

namespace Ravl2 {

  ParametersBodyC::ParametersBodyC (const VectorT<RealT> &minP,
				    const VectorT<RealT> &maxP,
				    const std::vector<IntT> &steps)
    : _minP(minP),
      _maxP(maxP),
      _constP(steps.size()),
      _steps(steps),
      _mask(steps.size()),
      m_cacheDirty(true)
  {
    RavlAssertMsg (minP.size() == maxP.size() && minP.size() == steps.size(), "Error: all arguments for parameters must have same dimension");
    _mask.fill(1);
    _constP.fill(0);
  }
  
  ParametersBodyC::ParametersBodyC (const VectorT<RealT> &minP,
				    const VectorT<RealT> &maxP,
				    const std::vector<IntT> &steps,
				    const std::vector<IntT> &mask)
    : _minP(minP),
      _maxP(maxP),
      _constP(steps.size()),
      _steps(steps),
      _mask(mask),
      m_cacheDirty(true)
  {
    RavlAssertMsg (minP.size() == maxP.size() && minP.size() == steps.size() && minP.size() == mask.size(), "Error: all arguments for parameters must have same dimension");
    _constP.fill(0);
  }
  
  /////////////////////////////////////////
  //: Constructor.
  // This setup nparams with defaults settings of :
  // minP=0 maxP=1 Steps=1 mask=0 (constP = 0)
  
  ParametersBodyC::ParametersBodyC (size_t nparams,bool unlimited )
    :_minP(nparams),
     _maxP(nparams),
     _constP(nparams),
     _steps(nparams),
     _mask(nparams),
      m_cacheDirty(true)
  {
    if(!unlimited) {
      _minP.fill(0);
      _maxP.fill(1);
      _mask.fill(0);
    } else {
      _minP.fill(-RavlConstN::maxReal);
      _maxP.fill(RavlConstN::maxReal);
      _mask.fill(1);
    }
    _steps.fill(1);
    _constP.fill(0);
    ONDEBUG(SPDLOG_TRACE("Setting up {} parameters. Unlimited={} ",(unsigned) nparams,(bool) unlimited));
  }
  
  ParametersBodyC::ParametersBodyC (std::istream &in)
    : m_cacheDirty(true)
  { in >> _minP >> _maxP >> _constP >> _steps >> _mask; }
  
  ParametersBodyC::ParametersBodyC (const ParametersBodyC &oth)
    :_minP(oth._minP.Copy()),
     _maxP(oth._maxP.Copy()),
     _constP(oth._constP.Copy()),
     _steps(oth._steps.Copy()),
     _mask(oth._mask.Copy()),
     m_cacheDirty(true)
  {}
  
  ParametersBodyC & ParametersBodyC::Copy () const
  { return *(new ParametersBodyC (*this)); }

  //: Transformation between P and X
  VectorT<RealT> ParametersBodyC::TransP2X (const VectorT<RealT> &P) const {
    if(m_cacheDirty) UpdateCache();
    VectorT<RealT> ret(m_maskMap.size());
    for(unsigned i = 0 ;i < m_maskMap.size();i++)
      ret[i] = P[m_maskMap[i]];
    return ret;
    //return m_transP2X * P;
  }

  //: Transformation between X and P
  VectorT<RealT> ParametersBodyC::TransX2P (const VectorT<RealT> &X) const {
    if(m_cacheDirty) UpdateCache();
    // Check if its the an identiy matrix.
    if(m_sizeX == _mask.size())
      return X;
    VectorT<RealT> ret = _constP.Copy();
    for(unsigned i = 0 ;i < m_maskMap.size();i++)
      ret[m_maskMap[i]] = X[i];
    return ret;
  }

  //: Transformation between P and X
  // Equivelent of inMat * TransX2P ()
  // TransX2P.range(0).size() = P.size() Full parameter set. _mask.size()
  // TransX2P.range(1).size() = X.size() Optimise set. m_sizeX

  Tensor<RealT,2> ParametersBodyC::TransP2X (const Tensor<RealT,2> &inMat) const {
    if(m_cacheDirty) UpdateCache();
    //return inMat * m_transX2P;
    // RavlAssert(inMat.range(1).size() == m_transX2P.range(0).size());
    RavlAssert(inMat.range(1).size() == _mask.size());
    // Check if its the an identiy matrix.
    if(m_sizeX == _mask.size())
      return inMat;
    // Else pick out the columns required.
    Tensor<RealT,2> ret(inMat.range(0).size(), m_sizeX);
    for(unsigned i = 0;i < m_sizeX;i++) {
      ret.SetColumn(i,const_cast<Tensor<RealT,2> &>(inMat).SliceColumn(m_maskMap[i]));
    }
    return ret;
  }


  void ParametersBodyC::SetMask (const std::vector<IntT> &mask)
  {
    if (mask.size() != _mask.size()) {
      std::cerr << "Error: parameter mask must be same size!\n";
      exit(1);
    }
    _mask = mask;
    m_cacheDirty = true;
  }
  
  void ParametersBodyC::SetConstP (const VectorT<RealT> &constP)
  { 
    _constP = constP.Copy(); 
    m_cacheDirty = true;
  }
  
  void ParametersBodyC::UpdateCache() const {
    m_sizeX = _mask.Sum();
    m_maskMap = std::vector<unsigned>(m_sizeX);
    unsigned at = 0;
    for(unsigned i = 0 ;i < _mask.size();i++) {
      if(_mask[i] > 0)
        m_maskMap[at++] = i;
    }
    RavlAssert(m_maskMap.size() == at);

    if(m_stepsP.size() != m_sizeX)
      m_stepsP = std::vector<IntT>(m_sizeX);
    
    if(m_constP.size() != _mask.size())
      m_constP = VectorT<RealT>(_mask.size());
    m_constP.Fill (0);
    
    int counter = 0;
    for (SArray1dIterC<IntT> it (_mask); it; it++) {
      if (*it == 1) {
        m_stepsP[counter] = _steps[it.index()];
        counter++;
      } else {
        m_constP[it.index()] = _constP[it.index()];
      }
    }
    RavlAssert(counter == m_sizeX);
    
    m_cacheDirty = false;

    m_minX = TransP2X(_minP);
    m_maxX = TransP2X(_maxP);
    m_startX = TransP2X(_constP);
    
  }
  
  
  //////////////////////////////////
  //: Setup parameter p.
  
  void ParametersBodyC::Setup(int p,RealT min,RealT max,IntT steps,IntT mask)
  {
    _minP[p] = min;
    _maxP[p] = max;
    _steps[p] = steps;
    _mask[p] = mask;
    m_cacheDirty = true;
  }
  
  //////////////////////////////////
  //: Setup parameter p, and constant value.
  
  void ParametersBodyC::Setup(int p,RealT min,RealT max,IntT steps,RealT constV,IntT mask) 
  {
    _minP[p] = min;
    _maxP[p] = max;
    _steps[p] = steps;
    _mask[p] = mask;
    _constP[p] = constV;
    m_cacheDirty = true;
  }
  
  
  void ParametersBodyC::Save (std::ostream &out) const
  {
    out << _minP << "\n" << _maxP << "\n" << _constP << "\n";
    out << _steps << "\n" << _mask << "\n";
  }


  //: Generate a random position in the parameter space.
  
  VectorT<RealT> ParametersBodyC::Random() {
    VectorT<RealT> ret(_minP.size());
    for(SArray1dIter5C<RealT,RealT,RealT,RealT,IntT> it(ret,_minP,_maxP,_constP,_mask);it;it++) {
      if(it.Data5() == 0) { // Use constant value ?
        it.data<0>() = it.Data4();
        continue;
      }
      RealT diff = it.data<2>() - it.data<1>();
      it.data<0>() = it.data<1>() + diff * Random1();
    }
    return ret;
  }

  
  
}
