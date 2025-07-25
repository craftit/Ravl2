#pragma once

#include "Optimise.hh"
#include <vector>
#include <tuple>
#include <functional>
#include <random>
#include <memory>

namespace Ravl2 {

//! Particle Swarm Optimization (PSO) with parallel evaluation support.
//!
//! Implements PSO algorithm to find global minima using a swarm of particles
//! that move through the search space based on their individual and collective experiences.
//! Supports parallel evaluation of the fitness function for improved performance.

class ParticleSwarmOptimise : public Optimise {
public:
  /// Real number type used for calculations
  using RealT = Optimise::RealT;

  /// Default constructor
  ParticleSwarmOptimise() = default;

  //! Constructor with verbosity setting
  //! @param verbose Whether to output progress information
  explicit ParticleSwarmOptimise(bool verbose)
    : Optimise(verbose) {}

  //! Programmatic constructor with common parameters.
  //! @param maxIters Maximum number of iterations
  //! @param swarmSize Number of particles in the swarm
  //! @param inertiaWeight Inertia weight for particle velocity (w)
  //! @param cognitiveWeight Cognitive weight for particle's own best position (c1)
  //! @param socialWeight Social weight for swarm's best position (c2)
  //! @param maxThreads Maximum number of threads for parallel evaluation (0 = single-threaded)
  //! @param verbose Whether to output progress information
  //! @param fixedSeed Whether to use a fixed random seed for reproducibility
  //! @param seed Random seed value if fixedSeed is true
  //! @param tolerance Convergence tolerance for early stopping
  //! @param minItersBeforeConvergence Minimum iterations before checking convergence
  explicit ParticleSwarmOptimise(
    size_t maxIters = 100,
    size_t swarmSize = 30,
    RealT inertiaWeight = static_cast<RealT>(0.7),
    RealT cognitiveWeight = static_cast<RealT>(1.5),
    RealT socialWeight = static_cast<RealT>(1.5),
    size_t maxThreads = 0,
    bool verbose = false,
    bool fixedSeed = true,
    unsigned seed = 42,
    RealT tolerance = static_cast<RealT>(1e-6),
    size_t minItersBeforeConvergence = 10
  );

  //! Constructor from configuration
  //! @param config Configuration object containing optimization parameters
  explicit ParticleSwarmOptimise(Configuration &config);

  //! Find minimum of a function
  //! @param domain Domain of the function to minimize
  //! @param func Function to minimize
  //! @param start Optional starting point (used to seed one particle if provided)
  //! @return Tuple containing the minimum point and its value
  [[nodiscard]] std::tuple<VectorT<RealT>, RealT> minimise(
      const CostDomain<RealT> &domain,
      const std::function<RealT(const VectorT<RealT> &)> &func,
      const VectorT<RealT> &start = VectorT<RealT>()
  ) const override;

  // Setters for configuration parameters
  void setMaxIters(size_t maxIters) { mMaxIters = maxIters; }
  void setSwarmSize(size_t swarmSize) { mSwarmSize = swarmSize; }
  void setInertiaWeight(RealT weight) { mInertiaWeight = weight; }
  void setCognitiveWeight(RealT weight) { mCognitiveWeight = weight; }
  void setSocialWeight(RealT weight) { mSocialWeight = weight; }
  void setMaxThreads(size_t maxThreads) { mMaxThreads = maxThreads; }
  void setFixedSeed(bool fixedSeed) { mFixedSeed = fixedSeed; }
  void setSeed(unsigned seed) { mSeed = seed; }
  void setTolerance(RealT tolerance) { mTolerance = tolerance; }
  void setMinItersBeforeConvergence(size_t minIters) { mMinItersBeforeConvergence = minIters; }

private:
  // PSO parameters
  size_t mMaxIters = 100;               //!< Maximum number of iterations
  size_t mSwarmSize = 30;               //!< Number of particles in the swarm
  RealT mInertiaWeight = 0.7f;          //!< Inertia weight (w)
  RealT mCognitiveWeight = 1.5f;        //!< Cognitive weight (c1)
  RealT mSocialWeight = 1.5f;           //!< Social weight (c2)
  size_t mMaxThreads = 0;               //!< Maximum number of threads for parallel evaluation (0 = single-threaded)
  bool mFixedSeed = true;               //!< Whether to use a fixed random seed
  unsigned mSeed = 42;                  //!< Random seed for reproducibility
  RealT mTolerance = 1e-6f;             //!< Convergence tolerance
  size_t mMinItersBeforeConvergence = 10; //!< Minimum iterations before checking convergence

  //! Internal particle structure for PSO algorithm
  struct Particle {
    VectorT<RealT> position;       //!< Current position
    VectorT<RealT> velocity;       //!< Current velocity
    VectorT<RealT> bestPosition;   //!< Best position found by this particle
    RealT bestFitness;             //!< Best fitness found by this particle

    Particle() : bestFitness(std::numeric_limits<RealT>::max()) {}
  };

  //! Initialize particles within the domain constraints
  std::vector<Particle> initializeSwarm(
      const CostDomain<RealT> &domain,
      std::mt19937 &rng,
      const VectorT<RealT> &start = VectorT<RealT>()
  ) const;

  //! Evaluate fitness for all particles in parallel
  void evaluateSwarm(
      std::vector<Particle> &swarm,
      const std::function<RealT(const VectorT<RealT> &)> &func,
      VectorT<RealT> &globalBestPosition,
      RealT &globalBestFitness,
      size_t maxThreads
  ) const;

  //! Update particle velocities and positions
  void updateSwarm(
      std::vector<Particle> &swarm,
      const CostDomain<RealT> &domain,
      const VectorT<RealT> &globalBestPosition,
      std::mt19937 &rng
  ) const;
};

void initParticleSwarmOptimise();

} // namespace Ravl2
