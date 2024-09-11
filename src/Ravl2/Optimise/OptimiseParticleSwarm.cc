// This file is part of RAVL, Recognition And Vision Library 
// Copyright (C) 2003, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here
//! rcsid="$Id$"
//! lib=Optimisation
//! file="Ravl/PatternRec/Optimise/OptimiseParticleSwarm.cc"

#include "Ravl/PatternRec/OptimiseParticleSwarm.hh"
#include "Ravl/PatternRec/DataSet4.hh"
#include "Ravl/PatternRec/DataSet4Iter.hh"
#include "Ravl/PatternRec/SampleReal.hh"
#include "Ravl/Random.hh"
#include "Ravl/StrStream.hh"
#include "Ravl/SobolSequence.hh"
#include "Ravl/Plot/Plot2d.hh"
#include "Ravl/SArray1dIter2.hh"
#include "Ravl/SArray1dIter3.hh"
#include "Ravl/OS/Date.hh"
#include "Ravl/Threads/LaunchThread.hh"
#include "Ravl/CallMethodPtrs.hh"
#include "Ravl/Tuple4.hh"

namespace RavlN {

  OptimiseParticleSwarmBodyC::OptimiseParticleSwarmBodyC(UIntT numberOfParticles,
                                                        RealT omega,
                                                        RealT phiP,
                                                        RealT phiG,
                                                        UIntT numberOfIterations,
                                                        RealT terminationCriterion,
                                                        UIntT numberOfThreads) :
      OptimiseBodyC("OptimiseParticleSwarmBodyC"),
      m_numberOfParticles(numberOfParticles),
      m_omega(omega),
      m_phiP(phiP),
      m_phiG(phiG),
      m_numberOfIterations(numberOfIterations),
      m_terminationCriterion(terminationCriterion),
      m_numberOfThreads(numberOfThreads)
  {
  }
  
  OptimiseParticleSwarmBodyC::OptimiseParticleSwarmBodyC(std::istream &in) :
      OptimiseBodyC("OptimiseParticleSwarmBodyC", in),
      m_numberOfParticles(0),
      m_omega(0),
      m_phiP(0),
      m_phiG(0),
      m_numberOfIterations(0),
      m_terminationCriterion(0),
      m_numberOfThreads(0)
  {
    in >> m_numberOfParticles >> m_omega >> m_phiP >> m_phiG >> m_numberOfIterations >> m_terminationCriterion >> m_numberOfThreads;
  }
  
  VectorC RandomVectorPSO(int dim, RealT scale)
  {
    VectorC ret(dim);
    for (SArray1dIterC<RealT> it(ret); it; it++)
      *it = Random1() * scale;
    return ret;
  }

  VectorC RandomVelocityPSO(const VectorC & min, const VectorC & max)
  {
    VectorC ret(min.Size());
    for (SArray1dIter3C<RealT, RealT, RealT> it(ret, min, max); it; it++)
      it.Data1() = ((Random1() * Abs(it.Data3() - it.Data2())) + it.Data2()) * 2.0;
    return ret;
  }

  void BoundVectorPSO(const VectorC & min, const VectorC & max, VectorC & in)
  {
    for (SArray1dIter3C<RealT, RealT, RealT> it(min, max, in); it; it++) {
      if (it.Data3() < it.Data1())
        it.Data3() = it.Data1();
      if (it.Data3() > it.Data2())
        it.Data3() = it.Data2();
    }
  }

  // Very simple cost function
  bool Cost(const CostC & domain, const VectorC & X, VectorC & V, UIntT index, Tuple4C<VectorC, VectorC, RealT, UIntT> * result)
  {
    RealT score = domain.Cost(X);
    result->Data1() = X;
    result->Data2() = V;
    result->Data3() = score;
    result->Data4() = index;
    return true;
  }

  // ------------------------------------------------------------------------
  // **********  OptimalX    ************************************************
  // ------------------------------------------------------------------------
  //
  // Random optimizer with uniform density. Randomly samples the parameter
  // space to find the minimum cost position.
  
  VectorC OptimiseParticleSwarmBodyC::MinimalX(const CostC &domain, RealT &minimumCost) const
  {

    /*
     * First lets make our random distribution.
     */
    VectorC startX = domain.StartX();
    UIntT dim = startX.Size();
    SobolSequenceC sobol(dim);
    sobol.First();
    sobol.Next(); // JUST WHILE TESTING!!!
    VectorC diff = domain.MaxX() - domain.MinX();
    // Current position, best position, best cost, velocity
    DataSet4C<SampleVectorC, SampleVectorC, SampleRealC, SampleVectorC> dset(m_numberOfParticles);
    RealT bestGlobalCostSoFar = RavlConstN::maxReal;
    VectorC bestGlobalXSoFar;
    VectorC boundVelocityMax = diff;
    VectorC boundVelocityMin = diff * -1.0;

    UIntT j = 0;
    for (;;) {

      CollectionC<LaunchThreadC> threads(m_numberOfThreads);
      SArray1dC<Tuple4C<VectorC, VectorC, RealT, UIntT> > results(m_numberOfThreads);

      for (UIntT i = 0; i < m_numberOfThreads; i++) {

        // Create initial position
        VectorC X(sobol.Data());
        X = (X * diff) + domain.MinX();
        // Make an initial velocity
        VectorC V = RandomVelocityPSO(domain.MinX(), domain.MaxX());

        TriggerC trig = Trigger(&Cost, domain, X, V, j, &results[i]);
        threads.Append(LaunchThreadC(trig));

        j++;

        // have we reached max number of particles
        if (j >= m_numberOfParticles) {
          break;
        }
        // unlikely this will happen
        if (!sobol.Next()) {
          RavlError("Run out of Sobol Samples...");
          return VectorC();
        }

      } // end number of threads

      // Lets wait for all threads to finish
      for (SArray1dIter2C<LaunchThreadC, Tuple4C<VectorC, VectorC, RealT, UIntT> > threadIt(threads.SArray1d(), results); threadIt; threadIt++) {
        threadIt.Data1().WaitForExit();
        dset.Append(threadIt.Data2().Data1(), threadIt.Data2().Data1(), threadIt.Data2().Data3(), threadIt.Data2().Data2()); // initially the particles initial and best position are the same
        if (threadIt.Data2().Data3() < bestGlobalCostSoFar) {
          bestGlobalCostSoFar = threadIt.Data2().Data3();
          bestGlobalXSoFar = threadIt.Data2().Data1();
        }
      }

      if (j >= m_numberOfParticles) {
        break;
      }

    }

    RavlDebug("Initial best particle '%s' in population has cost %0.4f", StringOf(bestGlobalXSoFar).data(), bestGlobalCostSoFar);

    // make a plot of the initial population
    Plot2dC::RefT plot = CreatePlot2d("PSO");
    if (dim == 2) {
      plot->SetXRange(RealRangeC(domain.MinX()[0], domain.MaxX()[0]));
      plot->SetYRange(RealRangeC(domain.MinX()[1], domain.MaxX()[1]));
      plot->Plot(dset.Sample1());
    }

    for (UIntT i = 0; i < m_numberOfIterations; i++) {

      DataSet4IterC<SampleVectorC, SampleVectorC, SampleRealC, SampleVectorC> it(dset);
      UIntT p=0;
      for (;;) {
        CollectionC<LaunchThreadC> threads(m_numberOfThreads);
        SArray1dC<Tuple4C<VectorC, VectorC, RealT, UIntT> > results(m_numberOfThreads);

        for (UIntT j = 0; j < m_numberOfThreads; j++) {
#if 0
          RealT rP = Random1();
          RealT rG = Random1();
          VectorC currentVelocity = (it.Data4() * m_omega);
          VectorC updateOnParticleBestPosition = (it.Data2() - it.Data1()) * rP * m_phiP;
          VectorC updateOnGlobalBestPosition = (bestGlobalXSoFar - it.Data1()) * rG * m_phiG;
#else
          VectorC randomParticleVec = RandomVectorPSO(dim, m_phiP);
          VectorC randomGlobalVec = RandomVectorPSO(dim, m_phiG);
          VectorC currentVelocity = (it.Data4() * m_omega);
          VectorC updateOnParticleBestPosition = randomParticleVec * (it.Data2() - it.Data1());
          VectorC updateOnGlobalBestPosition = randomGlobalVec * (bestGlobalXSoFar - it.Data1());
#endif
          VectorC velocity = currentVelocity + updateOnParticleBestPosition + updateOnGlobalBestPosition;
          BoundVectorPSO(boundVelocityMin, boundVelocityMax, velocity);

          VectorC particleNewPosition = it.Data1() + velocity;
          BoundVectorPSO(domain.MinX(), domain.MaxX(), particleNewPosition);

          TriggerC trig = Trigger(&Cost, domain, particleNewPosition, velocity, p, &results[j]);
          threads.Append(LaunchThreadC(trig));

          // goto next particle. If reached end then break.
          p++;
          it++;
          if (!it.IsElm()) {
            break;
          }

          //RavlDebug("New Position '%s' -> '%s' %0.2f", StringOf(it.Data1()).data(), StringOf(particleNewPosition).data(), newCost);
          //RavlDebug("New Velocity '%s' -> '%s'", StringOf(it.Data4()).data(), StringOf(velocity).data());
        }

        // Lets wait for all threads to finish
        for (SArray1dIter2C<LaunchThreadC, Tuple4C<VectorC, VectorC, RealT, UIntT> > threadIt(threads.SArray1d(), results); threadIt; threadIt++) {
          threadIt.Data1().WaitForExit();

          // make it easier to read
          VectorC particleNewPosition = threadIt.Data2().Data1();
          VectorC velocity = threadIt.Data2().Data2();
          RealT newCost = threadIt.Data2().Data3();
          UIntT p = threadIt.Data2().Data4(); // particle number

          // Has particle beaten its own position
          if (newCost < it.Data3()) {
            dset.Sample2()[p] = particleNewPosition;
            dset.Sample3()[p] = newCost;
          }

          // Update global if we have a best new particle....
          if (newCost < bestGlobalCostSoFar) {
            bestGlobalCostSoFar = newCost;
            bestGlobalXSoFar = particleNewPosition;
            RavlDebug("New best particle '%s' in population has cost %0.12f at iter %d", StringOf(bestGlobalXSoFar).data(), bestGlobalCostSoFar, i);
          }
          // Update the position and velocity
          dset.Sample1()[p] = threadIt.Data2().Data1();
          dset.Sample4()[p] = threadIt.Data2().Data2();
        }

        // have we reached the end
        if(!it.IsElm())
          break;

      } // end an iteration

      if (bestGlobalCostSoFar < m_terminationCriterion) {
        RavlDebug("Reached best solution, terminating early.");
        break;
      }

      if (dim == 2) {
        RavlN::Sleep(0.1); // otherwise it goes too fast!
        plot->Plot(dset.Sample1());
      }

    }
    minimumCost = bestGlobalCostSoFar;
    return domain.ConvertX2P(bestGlobalXSoFar); // Return final estimate

  }
  
  const StringC OptimiseParticleSwarmBodyC::GetInfo() const
  {
    StrOStreamC stream;
    stream << OptimiseBodyC::GetInfo() << "\n";
    stream << "Particle Swarm Optimisation with population " << m_numberOfParticles;
    return stream.String();
  }
  
  bool OptimiseParticleSwarmBodyC::Save(std::ostream &out) const
  {
    OptimiseBodyC::Save(out);
    out << m_numberOfParticles << '\n';
    out << m_omega << '\n';
    out << m_phiP << '\n';
    out << m_phiG << '\n';
    out << m_numberOfIterations << '\n';
    out << m_terminationCriterion << '\n';
    out << m_numberOfThreads << '\n';
    return true;
  }

}
