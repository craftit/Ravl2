// This file is part of RAVL, Recognition And Vision Library 
// Copyright (C) 2003, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here

#include "Ravl2/PatternRec/OptimiseBrent.hh"
#include "Ravl2/StrStream.hh"

#define DODEBUG 0
#if DODEBUG
#define ONDEBUG(x) x
#else
#define ONDEBUG(x)
#endif

namespace Ravl2 {

  OptimiseBrentBodyC::OptimiseBrentBodyC (unsigned iterations, RealT tolerance)
    :OptimiseBodyC("OptimiseBrentBodyC"),
     _iterations(iterations),
     _tolerance(tolerance)
  {
  }
  
  OptimiseBrentBodyC::OptimiseBrentBodyC (std::istream &in)
    :OptimiseBodyC("OptimiseBrentBodyC",in)
  {
    in >> _iterations >> _tolerance;
  }
  
  // ------------------------------------------------------------------------
  // **********  OptimalX    ************************************************
  // ------------------------------------------------------------------------
  //
  // Parabolic Interpolation and Brent's Method in One Dimension. Uses a combination
  // of Golden Section search when the quadratic is uncooperative and parabolic
  // interpolation when it is cooperative!
  //
  VectorT<RealT> OptimiseBrentBodyC::MinimalX (const CostC &domain, RealT startCost, RealT &minimumCost) const
  {
    // Only works for cost functions of 1 dimension
    //RavlAssert();

    const RealT cgold = 0.3819660;
    const RealT smallVal = 1.0e-10;
    VectorT<RealT> iterX1(1);
    VectorT<RealT> iterX0(1);

    RealT d = 0,etemp,fx0,fx1,fx2,fx3,p,q,r,tol1,tol2,x3,x2,xm;
    RealT &x1 = iterX1[0];                  // Cunning trick to allow setting value in iterX directly
    RealT &x0 = iterX0[0];                  // Ditto
    RealT e = 0.0;                          // This will be the distance moved on the step before last.

    RealT a = domain.MinX()[0];
    RealT b = domain.MaxX()[0];
    ONDEBUG(SPDLOG_TRACE(" a={} b={} ",a,b));

    // Make sure min and max are the right way around.
    if(a > b) std::swap(a,b);
    
    x1 = x2 = x3 = domain.StartX()[0];                      // Initialisations...
    fx1 = fx2 = fx3 = startCost;

    // Main iteration loop
    for (unsigned iter = 0; iter < _iterations; iter++) {
      xm = (a + b) * 0.5;
      tol1 = _tolerance * fabs(x1) + smallVal;
      tol2 = 2.0 * tol1;
      // test for termination
      if (fabs(x1 - xm) <= (tol2 - 0.5 * (b - a))) {
        minimumCost = fx1;
        return iterX1;
      }
      if (fabs(e) <= tol1) {
        e = (x1 >= xm? a - x1: b - x1);
        d = cgold * e;
      }
      else {
        r = (fx1 - fx3) * (x1 - x2);
        q = (fx1 - fx2) * (x1 - x3);
        p = (x1 - x3) * q - (x1 - x2) * r;
        q = 2.0 * (q - r);
        if (q > 0.0) p = -p;
        q = fabs(q);
        etemp = e;
        e = d;
        // determine the acceptability of the parabolic fit.
        if (fabs(p) >= fabs(0.5 * q * etemp) || p <= q * (a - x1) || p >= q * (b - x1)) {
          e = (x1 >= xm ? a - x1: b - x1);
          d = cgold * e;
        }
        else {
          d = p / q;                          // Take the parabolic step.
          x0 = x1 + d;
          if (x0 - a < tol2 || b - x0 < tol2)
            d = Sign(tol1, xm - x1);
        }
      }
      x0 = (fabs(d) >= tol1? x1 + d: x1 + Sign(tol1, d));
      fx0 = domain.Cost (iterX0);             // Evaluate the function at the new point.
      if (fx0 > fx1) {                       // Now decide what to do with our function evaluation.
        if (x0 < x1)
          a = x0;
        else
          b = x0;
        if (fx0 <= fx2 || x2 == x1) {
          x3 = x2; x2 = x0;
          fx3 = fx2; fx2 = fx0;
        }
        else
          if (fx0 <= fx3 || x3 == x1 || x3 == x2) {
            x3 = x0;
            fx3 = fx0;
          }
      }
      else {
        if (x0 >= x1)
          a = x1;
        else
          b = x1;
        x3 = x2; x2 = x1; x1 = x0;
        fx3 = fx2; fx2 = fx1; fx1 = fx0;
      }
    }
    minimumCost = fx1;
    return iterX1;
  }
  
  const std::string OptimiseBrentBodyC::GetInfo () const
  {
    Strstd::unique_ptr<std::ostream> stream;
    stream << OptimiseBodyC::GetInfo () << "\n";
    stream << "Brent optimization algorithm. Iterations = " << _iterations;
    return stream.String();
  }
  
  bool OptimiseBrentBodyC::Save (std::ostream &out) const
  {
    OptimiseBodyC::Save (out);
    out << _iterations << " " << _tolerance << "\n";
    return true;
  }
  
}
