// This file is part of RAVL, Recognition And Vision Library 
// Copyright (C) 2003, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here

#include "Ravl2/PatternRec/OptimiseParticleSwarm.hh"
#include "Ravl2/PatternRec/DataSet4.hh"
#include "Ravl2/PatternRec/DataSet4Iter.hh"
#include "Ravl2/PatternRec/SampleReal.hh"
#include "Ravl2/Random.hh"
#include "Ravl2/StrStream.hh"
#include "Ravl2/SobolSequence.hh"
#include "Ravl2/Plot/Plot2d.hh"
#include "Ravl2/SArray1dIter2.hh"
#include "Ravl2/SArray1dIter3.hh"
#include "Ravl2/OS/Date.hh"
#include "Ravl2/Threads/LaunchThread.hh"
#include "Ravl2/CallMethodPtrs.hh"
#include "Ravl2/Tuple4.hh"

namespace Ravl2 {

  OptimiseParticleSwarmBodyC::OptimiseParticleSwarmBodyC(unsigned numberOfParticles,
                                                        RealT omega,
                                                        RealT phiP,
                                                        RealT phiG,
                                                        unsigned numberOfIterations,
                                                        RealT terminationCriterion,
                                                        unsigned numberOfThreads) :
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
  
  VectorT<RealT> RandomVectorPSO(int dim, RealT scale)
  {
    VectorT<RealT> ret(dim);
    for (SArray1dIterC<RealT> it(ret); it; it++)
      *it = Random1() * scale;
    return ret;
  }

  VectorT<RealT> RandomVelocityPSO(const VectorT<RealT> & min, const VectorT<RealT> & max)
  {
    VectorT<RealT> ret(min.size());
    for (SArray1dIter3C<RealT, RealT, RealT> it(ret, min, max); it; it++)
      it.data<0>() = ((Random1() * std::abs(it.data<2>() - it.data<1>())) + it.data<1>()) * 2.0;
    return ret;
  }

  void BoundVectorPSO(const VectorT<RealT> & min, const VectorT<RealT> & max, VectorT<RealT> & in)
  {
    for (SArray1dIter3C<RealT, RealT, RealT> it(min, max, in); it; it++) {
      if (it.data<2>() < it.data<0>())
        it.data<2>() = it.data<0>();
      if (it.data<2>() > it.data<1>())
        it.data<2>() = it.data<1>();
    }
  }

  // Very simple cost function
  bool Cost(const CostC & domain, const VectorT<RealT> & X, VectorT<RealT> & V, unsigned index, Tuple4C<VectorT<RealT>, VectorT<RealT>, RealT, unsigned> * result)
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
  
  VectorT<RealT> OptimiseParticleSwarmBodyC::MinimalX(const CostC &domain, RealT &minimumCost) const
  {

    /*
     * First lets make our random distribution.
     */
    VectorT<RealT> startX = domain.StartX();
    unsigned dim = startX.size();
    SobolSequenceC sobol(dim);
    sobol.First();
    sobol.next(); // JUST WHILE TESTING!!!
    VectorT<RealT> diff = domain.MaxX() - domain.MinX();
    // Current position, best position, best cost, velocity
    DataSet4C<SampleVectorT<RealT>, SampleVectorT<RealT>, SampleRealC, SampleVectorT<RealT>> dset(m_numberOfParticles);
    RealT bestGlobalCostSoFar = RavlConstN::maxReal;
    VectorT<RealT> bestGlobalXSoFar;
    VectorT<RealT> boundVelocityMax = diff;
    VectorT<RealT> boundVelocityMin = diff * -1.0;

    unsigned j = 0;
    for (;;) {

      std::vector<LaunchThreadC> threads(m_numberOfThreads);
      std::vector<Tuple4C<VectorT<RealT>, VectorT<RealT>, RealT, unsigned> > results(m_numberOfThreads);

      for (unsigned i = 0; i < m_numberOfThreads; i++) {

        // Create initial position
        VectorT<RealT> X(sobol.Data());
        X = (X * diff) + domain.MinX();
        // Make an initial velocity
        VectorT<RealT> V = RandomVelocityPSO(domain.MinX(), domain.MaxX());

        TriggerC trig = Trigger(&Cost, domain, X, V, j, &results[i]);
        threads.Append(LaunchThreadC(trig));

        j++;

        // have we reached max number of particles
        if (j >= m_numberOfParticles) {
          break;
        }
        // unlikely this will happen
        if (!sobol.next()) {
          SPDLOG_ERROR("Run out of Sobol Samples...");
          return VectorT<RealT>();
        }

      } // end number of threads

      // Lets wait for all threads to finish
      for (SArray1dIter2C<LaunchThreadC, Tuple4C<VectorT<RealT>, VectorT<RealT>, RealT, unsigned> > threadIt(threads.SArray1d(), results); threadIt; threadIt++) {
        threadIt.data<0>().WaitForExit();
        dset.Append(threadIt.data<1>().data<0>(), threadIt.data<1>().data<0>(), threadIt.data<1>().data<2>(), threadIt.data<1>().data<1>()); // initially the particles initial and best position are the same
        if (threadIt.data<1>().data<2>() < bestGlobalCostSoFar) {
          bestGlobalCostSoFar = threadIt.data<1>().data<2>();
          bestGlobalXSoFar = threadIt.data<1>().data<0>();
        }
      }

      if (j >= m_numberOfParticles) {
        break;
      }

    }

    SPDLOG_TRACE("Initial best particle '{}' in population has cost {:0.4f}", StringOf(bestGlobalXSoFar).data(), bestGlobalCostSoFar);

    // make a plot of the initial population
    Plot2dC::RefT plot = CreatePlot2d("PSO");
    if (dim == 2) {
      plot->SetXRange(RealRangeC(domain.MinX()[0], domain.MaxX()[0]));
      plot->SetYRange(RealRangeC(domain.MinX()[1], domain.MaxX()[1]));
      plot->Plot(dset.Sample1());
    }

    for (unsigned i = 0; i < m_numberOfIterations; i++) {

      DataSet4IterC<SampleVectorT<RealT>, SampleVectorT<RealT>, SampleRealC, SampleVectorT<RealT>> it(dset);
      unsigned p=0;
      for (;;) {
        std::vector<LaunchThreadC> threads(m_numberOfThreads);
        std::vector<Tuple4C<VectorT<RealT>, VectorT<RealT>, RealT, unsigned> > results(m_numberOfThreads);

        for (unsigned j = 0; j < m_numberOfThreads; j++) {
#if 0
          RealT rP = Random1();
          RealT rG = Random1();
          VectorT<RealT> currentVelocity = (it.Data4() * m_omega);
          VectorT<RealT> updateOnParticleBestPosition = (it.data<1>() - it.data<0>()) * rP * m_phiP;
          VectorT<RealT> updateOnGlobalBestPosition = (bestGlobalXSoFar - it.data<0>()) * rG * m_phiG;
#else
          VectorT<RealT> randomParticleVec = RandomVectorPSO(dim, m_phiP);
          VectorT<RealT> randomGlobalVec = RandomVectorPSO(dim, m_phiG);
          VectorT<RealT> currentVelocity = (it.Data4() * m_omega);
          VectorT<RealT> updateOnParticleBestPosition = randomParticleVec * (it.data<1>() - it.data<0>());
          VectorT<RealT> updateOnGlobalBestPosition = randomGlobalVec * (bestGlobalXSoFar - it.data<0>());
#endif
          VectorT<RealT> velocity = currentVelocity + updateOnParticleBestPosition + updateOnGlobalBestPosition;
          BoundVectorPSO(boundVelocityMin, boundVelocityMax, velocity);

          VectorT<RealT> particleNewPosition = it.data<0>() + velocity;
          BoundVectorPSO(domain.MinX(), domain.MaxX(), particleNewPosition);

          TriggerC trig = Trigger(&Cost, domain, particleNewPosition, velocity, p, &results[j]);
          threads.Append(LaunchThreadC(trig));

          // goto next particle. If reached end then break.
          p++;
          it++;
          if (!it.valid()) {
            break;
          }

          //SPDLOG_TRACE("New Position '{}' -> '{}' {:0.2f}", StringOf(it.data<0>()).data(), StringOf(particleNewPosition).data(), newCost);
          //SPDLOG_TRACE("New Velocity '{}' -> '{}'", StringOf(it.Data4()).data(), StringOf(velocity).data());
        }

        // Lets wait for all threads to finish
        for (SArray1dIter2C<LaunchThreadC, Tuple4C<VectorT<RealT>, VectorT<RealT>, RealT, unsigned> > threadIt(threads.SArray1d(), results); threadIt; threadIt++) {
          threadIt.data<0>().WaitForExit();

          // make it easier to read
          VectorT<RealT> particleNewPosition = threadIt.data<1>().data<0>();
          VectorT<RealT> velocity = threadIt.data<1>().data<1>();
          RealT newCost = threadIt.data<1>().data<2>();
          unsigned p = threadIt.data<1>().Data4(); // particle number

          // Has particle beaten its own position
          if (newCost < it.data<2>()) {
            dset.Sample2()[p] = particleNewPosition;
            dset.Sample3()[p] = newCost;
          }

          // Update global if we have a best new particle....
          if (newCost < bestGlobalCostSoFar) {
            bestGlobalCostSoFar = newCost;
            bestGlobalXSoFar = particleNewPosition;
            SPDLOG_TRACE("New best particle '{}' in population has cost {:0.12f} at iter {}", StringOf(bestGlobalXSoFar).data(), bestGlobalCostSoFar, i);
          }
          // Update the position and velocity
          dset.Sample1()[p] = threadIt.data<1>().data<0>();
          dset.Sample4()[p] = threadIt.data<1>().data<1>();
        }

        // have we reached the end
        if(!it.valid())
          break;

      } // end an iteration

      if (bestGlobalCostSoFar < m_terminationCriterion) {
        SPDLOG_TRACE("Reached best solution, terminating early.");
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
  
  const std::string OptimiseParticleSwarmBodyC::GetInfo() const
  {
    Strstd::unique_ptr<std::ostream> stream;
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
