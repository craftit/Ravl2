// This file is part of RAVL, Recognition And Vision Library 
// Copyright (C) 2003, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here
//! rcsid="$Id$"
//! lib=Optimisation
//! file="Ravl/PatternRec/Optimise/Parameters.cc"

#include "Ravl/PatternRec/Parameters.hh"
#include "Ravl/SArray1dIter5.hh"
#include "Ravl/Random.hh"
#include "Ravl/config.h"
#include <stdlib.h>
#include "Ravl/StdConst.hh"
#include "Ravl/SysLog.hh"

#define DODEBUG 0
#if DODEBUG
#define ONDEBUG(x) x
#else
#define ONDEBUG(x)
#endif

namespace RavlN {

  ParametersBodyC::ParametersBodyC (const VectorC &minP,
				    const VectorC &maxP,
				    const SArray1dC<IntT> &steps)
    : _minP(minP),
      _maxP(maxP),
      _constP(steps.Size()),
      _steps(steps),
      _mask(steps.Size()),
      m_cacheDirty(true)
  {
    RavlAssertMsg (minP.Size() == maxP.Size() && minP.Size() == steps.Size(), "Error: all arguments for parameters must have same dimension");
    _mask.Fill(1);
    _constP.Fill(0);
  }
  
  ParametersBodyC::ParametersBodyC (const VectorC &minP,
				    const VectorC &maxP,
				    const SArray1dC<IntT> &steps,
				    const SArray1dC<IntT> &mask)
    : _minP(minP),
      _maxP(maxP),
      _constP(steps.Size()),
      _steps(steps),
      _mask(mask),
      m_cacheDirty(true)
  {
    RavlAssertMsg (minP.Size() == maxP.Size() && minP.Size() == steps.Size() && minP.Size() == mask.Size(), "Error: all arguments for parameters must have same dimension");
    _constP.Fill(0);
  }
  
  /////////////////////////////////////////
  //: Constructor.
  // This setup nparams with defaults settings of :
  // minP=0 maxP=1 Steps=1 mask=0 (constP = 0)
  
  ParametersBodyC::ParametersBodyC (SizeT nparams,bool unlimited )
    :_minP(nparams),
     _maxP(nparams),
     _constP(nparams),
     _steps(nparams),
     _mask(nparams),
      m_cacheDirty(true)
  {
    if(!unlimited) {
      _minP.Fill(0);
      _maxP.Fill(1);
      _mask.Fill(0);
    } else {
      _minP.Fill(-RavlConstN::maxReal);
      _maxP.Fill(RavlConstN::maxReal);
      _mask.Fill(1);
    }
    _steps.Fill(1);
    _constP.Fill(0);
    ONDEBUG(RavlDebug("Setting up %u parameters. Unlimited=%d ",(unsigned) nparams.V(),(bool) unlimited));
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
  VectorC ParametersBodyC::TransP2X (const VectorC &P) const {
    if(m_cacheDirty) UpdateCache();
    VectorC ret(m_maskMap.Size());
    for(unsigned i = 0 ;i < m_maskMap.Size();i++)
      ret[i] = P[m_maskMap[i]];
    return ret;
    //return m_transP2X * P;
  }

  //: Transformation between X and P
  VectorC ParametersBodyC::TransX2P (const VectorC &X) const {
    if(m_cacheDirty) UpdateCache();
    // Check if its the an identiy matrix.
    if(m_sizeX == _mask.Size())
      return X;
    VectorC ret = _constP.Copy();
    for(unsigned i = 0 ;i < m_maskMap.Size();i++)
      ret[m_maskMap[i]] = X[i];
    return ret;
  }

  //: Transformation between P and X
  // Equivelent of inMat * TransX2P ()
  // TransX2P.Rows() = P.Size() Full parameter set. _mask.Size()
  // TransX2P.Cols() = X.Size() Optimise set. m_sizeX

  MatrixC ParametersBodyC::TransP2X (const MatrixC &inMat) const {
    if(m_cacheDirty) UpdateCache();
    //return inMat * m_transX2P;
    // RavlAssert(inMat.Cols() == m_transX2P.Rows());
    RavlAssert(inMat.Cols() == _mask.Size());
    // Check if its the an identiy matrix.
    if(m_sizeX == _mask.Size())
      return inMat;
    // Else pick out the columns required.
    MatrixC ret(inMat.Rows(), m_sizeX);
    for(unsigned i = 0;i < m_sizeX;i++) {
      ret.SetColumn(i,const_cast<MatrixC &>(inMat).SliceColumn(m_maskMap[i]));
    }
    return ret;
  }


  void ParametersBodyC::SetMask (const SArray1dC<IntT> &mask)
  {
    if (mask.Size() != _mask.Size()) {
      std::cerr << "Error: parameter mask must be same size!\n";
      exit(1);
    }
    _mask = mask;
    m_cacheDirty = true;
  }
  
  void ParametersBodyC::SetConstP (const VectorC &constP)
  { 
    _constP = constP.Copy(); 
    m_cacheDirty = true;
  }
  
  void ParametersBodyC::UpdateCache() const {
    m_sizeX = _mask.Sum();
    m_maskMap = SArray1dC<unsigned>(m_sizeX);
    unsigned at = 0;
    for(unsigned i = 0 ;i < _mask.Size();i++) {
      if(_mask[i] > 0)
        m_maskMap[at++] = i;
    }
    RavlAssert(m_maskMap.Size() == at);

    if(m_stepsP.Size() != m_sizeX)
      m_stepsP = SArray1dC<IntT>(m_sizeX);
    
    if(m_constP.Size() != _mask.Size())
      m_constP = VectorC(_mask.Size());
    m_constP.Fill (0);
    
    IndexC counter = 0;
    for (SArray1dIterC<IntT> it (_mask); it; it++) {
      if (*it == 1) {
        m_stepsP[counter] = _steps[it.Index()];
        counter++;
      } else {
        m_constP[it.Index()] = _constP[it.Index()];
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
  
  void ParametersBodyC::Setup(IndexC p,RealT min,RealT max,IntT steps,IntT mask)
  {
    _minP[p] = min;
    _maxP[p] = max;
    _steps[p] = steps;
    _mask[p] = mask;
    m_cacheDirty = true;
  }
  
  //////////////////////////////////
  //: Setup parameter p, and constant value.
  
  void ParametersBodyC::Setup(IndexC p,RealT min,RealT max,IntT steps,RealT constV,IntT mask) 
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
  
  VectorC ParametersBodyC::Random() {
    VectorC ret(_minP.Size());
    for(SArray1dIter5C<RealT,RealT,RealT,RealT,IntT> it(ret,_minP,_maxP,_constP,_mask);it;it++) {
      if(it.Data5() == 0) { // Use constant value ?
        it.Data1() = it.Data4();
        continue;
      }
      RealT diff = it.Data3() - it.Data2();
      it.Data1() = it.Data2() + diff * Random1();
    }
    return ret;
  }

  
  
}
