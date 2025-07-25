#include "Ravl2/Optimise/ParticleSwarmOptimise.hh"
#include <random>
#include <spdlog/spdlog.h>
#include <algorithm>
#include <thread>
#include <future>
#include <atomic>
#include <cmath>
#include <limits>

namespace Ravl2
{

  //! Construct from configuration
  ParticleSwarmOptimise::ParticleSwarmOptimise(Configuration &config)
      : Optimise(config),
      mMaxIters(config.getUnsigned("maxIters", "Maximum number of iterations", 100, 1, 10000)),
      mSwarmSize(config.getUnsigned("swarmSize", "Number of particles in the swarm", 30, 5, 1000)),
      mInertiaWeight(config.getReal("inertiaWeight", "Inertia weight for particle velocity (w)", 0.7f, 0.0f, 1.0f)),
      mCognitiveWeight(config.getReal("cognitiveWeight", "Cognitive weight for particle's own best position (c1)", 1.5f, 0.0f, 4.0f)),
      mSocialWeight(config.getReal("socialWeight", "Social weight for swarm's best position (c2)", 1.5f, 0.0f, 4.0f)),
      mMaxThreads(config.getUnsigned("maxThreads", "Maximum number of threads for parallel evaluation (0 = single-threaded)", 0, 0, 128)),
      mFixedSeed(config.getBool("fixedSeed", "Whether to use a fixed random seed for reproducibility", true)),
      mSeed(config.getUnsigned("seed", "Random seed value if fixedSeed is true", 42, 0, 1000000)),
      mTolerance(config.getReal("tolerance", "Convergence tolerance for early stopping", 1e-6f, 0.0f, 1.0f)),
      mMinItersBeforeConvergence(config.getUnsigned("minItersBeforeConvergence", "Minimum iterations before checking convergence", 10, 0, 1000))
  {
  }

  //! Programmatic constructor
  ParticleSwarmOptimise::ParticleSwarmOptimise(
      size_t maxIters,
      size_t swarmSize,
      RealT inertiaWeight,
      RealT cognitiveWeight,
      RealT socialWeight,
      size_t maxThreads,
      bool verbose,
      bool fixedSeed,
      unsigned seed,
      RealT tolerance,
      size_t minItersBeforeConvergence
  ) : Optimise(verbose),
      mMaxIters(maxIters),
      mSwarmSize(swarmSize),
      mInertiaWeight(inertiaWeight),
      mCognitiveWeight(cognitiveWeight),
      mSocialWeight(socialWeight),
      mMaxThreads(maxThreads),
      mFixedSeed(fixedSeed),
      mSeed(seed),
      mTolerance(tolerance),
      mMinItersBeforeConvergence(minItersBeforeConvergence)
  {
    // Parameter validation
    if(maxIters == 0) {
      SPDLOG_ERROR("Invalid parameter: maxIters must be greater than 0, got {}", maxIters);
      throw std::invalid_argument("maxIters must be greater than 0");
    }
    if(swarmSize < 5) {
      SPDLOG_ERROR("Invalid parameter: swarmSize must be at least 5, got {}", swarmSize);
      throw std::invalid_argument("swarmSize must be at least 5");
    }
    if(inertiaWeight < 0.0f || inertiaWeight > 1.0f) {
      SPDLOG_ERROR("Invalid parameter: inertiaWeight must be between 0.0 and 1.0, got {}", inertiaWeight);
      throw std::invalid_argument("inertiaWeight must be between 0.0 and 1.0");
    }
    if(cognitiveWeight < 0.0f || cognitiveWeight > 4.0f) {
      SPDLOG_ERROR("Invalid parameter: cognitiveWeight must be between 0.0 and 4.0, got {}", cognitiveWeight);
      throw std::invalid_argument("cognitiveWeight must be between 0.0 and 4.0");
    }
    if(socialWeight < 0.0f || socialWeight > 4.0f) {
      SPDLOG_ERROR("Invalid parameter: socialWeight must be between 0.0 and 4.0, got {}", socialWeight);
      throw std::invalid_argument("socialWeight must be between 0.0 and 4.0");
    }
  }

  //! Initialize swarm within domain constraints
  std::vector<ParticleSwarmOptimise::Particle> ParticleSwarmOptimise::initializeSwarm(
      const CostDomain<RealT> &domain,
      std::mt19937 &rng,
      const VectorT<RealT> &start
  ) const {
    std::vector<Particle> swarm(mSwarmSize);
    const size_t dim = domain.size();

    // Setup distributions for position and velocity initialization
    std::vector<std::uniform_real_distribution<RealT>> posDistributions;
    std::vector<std::uniform_real_distribution<RealT>> velDistributions;

    for (Eigen::Index d = 0; d < static_cast<Eigen::Index>(dim); ++d) {
      const RealT minVal = domain.min(d);
      const RealT maxVal = domain.max(d);
      const RealT range = maxVal - minVal;

      posDistributions.emplace_back(minVal, maxVal);
      velDistributions.emplace_back(-range * 0.1f, range * 0.1f); // Velocity is typically 10% of the range
    }

    // Initialize particles
    for (size_t i = 0; i < mSwarmSize; ++i) {
      Particle &p = swarm[i];
      p.position.resize(static_cast<Eigen::Index>(dim));
      p.velocity.resize(static_cast<Eigen::Index>(dim));
      p.bestPosition.resize(static_cast<Eigen::Index>(dim));

      // Initialise position and velocity
      for (size_t d = 0; d < dim; ++d) {
        p.position[static_cast<Eigen::Index>(d)] = posDistributions[d](rng);
        p.velocity[static_cast<Eigen::Index>(d)] = velDistributions[d](rng);
      }

      // Initialize best position to current position
      p.bestPosition = p.position;
    }

    // If a starting point is provided, use it for the first particle
    if (start.size() == static_cast<Eigen::Index>(dim)) {
      // Validate that start is within domain bounds
      if (domain.isInDomain(start)) {
        swarm[0].position = start;
        swarm[0].bestPosition = start;
      } else if (verbose()) {
        SPDLOG_WARN("Starting point is outside domain bounds, ignoring it");
      }
    }

    return swarm;
  }

  //! Evaluate fitness for all particles in parallel
  void ParticleSwarmOptimise::evaluateSwarm(
      std::vector<Particle> &swarm,
      const std::function<RealT(const VectorT<RealT> &)> &func,
      VectorT<RealT> &globalBestPosition,
      RealT &globalBestFitness,
      size_t maxThreads
  ) const {
    const size_t numParticles = swarm.size();

    // Single-threaded evaluation
    if (maxThreads == 0 || maxThreads == 1 || numParticles <= 1) {
      for (auto &particle : swarm) {
        RealT fitness = func(particle.position);

        // Update particle's personal best
        if (fitness < particle.bestFitness) {
          particle.bestFitness = fitness;
          particle.bestPosition = particle.position;

          // Update global best
          if (fitness < globalBestFitness) {
            globalBestFitness = fitness;
            globalBestPosition = particle.position;
          }
        }
      }
    }
    // Multi-threaded evaluation
    else {
      // Limit threads to number of particles and available hardware
      size_t numThreads = std::min({maxThreads, numParticles,
                                   static_cast<size_t>(std::thread::hardware_concurrency())});

      // Create futures for parallel evaluation
      std::vector<std::future<void>> futures;
      std::mutex bestMutex; // To protect updates to global best

      // Distribute particles among threads
      for (size_t t = 0; t < numThreads; ++t) {
        futures.push_back(std::async(std::launch::async, [&, t, numThreads]() {
          for (size_t i = t; i < numParticles; i += numThreads) {
            Particle &particle = swarm[i];
            RealT fitness = func(particle.position);

            // Update particle's personal best
            if (fitness < particle.bestFitness) {
              particle.bestFitness = fitness;
              particle.bestPosition = particle.position;

              // Update global best (thread-safe)
              std::lock_guard<std::mutex> lock(bestMutex);
              if (fitness < globalBestFitness) {
                globalBestFitness = fitness;
                globalBestPosition = particle.position;
              }
            }
          }
        }));
      }

      // Wait for all evaluations to complete
      for (auto &future : futures) {
        future.wait();
      }
    }
  }

  //! Update particle velocities and positions
  void ParticleSwarmOptimise::updateSwarm(
      std::vector<Particle> &swarm,
      const CostDomain<RealT> &domain,
      const VectorT<RealT> &globalBestPosition,
      std::mt19937 &rng
  ) const {
    std::uniform_real_distribution<RealT> dist(0.0, 1.0);

    for (auto &particle : swarm) {
      // Update velocity and position for each dimension
      for (Eigen::Index d = 0; d < domain.dim(); ++d) {
        // Generate random factors
        RealT r1 = dist(rng);
        RealT r2 = dist(rng);

        // Update velocity using PSO equation:
        // v = w*v + c1*r1*(pbest - x) + c2*r2*(gbest - x)
        particle.velocity[d] = mInertiaWeight * particle.velocity[d] +
                              mCognitiveWeight * r1 * (particle.bestPosition[d] - particle.position[d]) +
                              mSocialWeight * r2 * (globalBestPosition[d] - particle.position[d]);

        // Apply velocity clamping if needed (optional, based on domain range)
        const RealT range = domain.max(d) - domain.min(d);
        const RealT vMax = range * 0.1f; // Limit velocity to 10% of domain range
        particle.velocity[d] = std::max(std::min(particle.velocity[d], vMax), -vMax);

        // Update position: x = x + v
        particle.position[d] += particle.velocity[d];

        // Ensure position stays within domain bounds
        particle.position[d] = std::max(std::min(particle.position[d], domain.max(d)), domain.min(d));
      }
    }
  }

  //! Minimize a function using Particle Swarm Optimization
  std::tuple<VectorT<ParticleSwarmOptimise::RealT>, ParticleSwarmOptimise::RealT>
  ParticleSwarmOptimise::minimise(
      const CostDomain<RealT> &domain,
      const std::function<RealT(const VectorT<RealT> &)> &func,
      const VectorT<RealT> &start
  ) const {
    // Check domain
    if (domain.empty()) {
      SPDLOG_ERROR("Empty domain provided to ParticleSwarmOptimise::minimise");
      throw std::invalid_argument("Empty domain provided to ParticleSwarmOptimise::minimise");
    }

    // Initialize random number generator
    std::mt19937 rng;
    if (mFixedSeed) {
      rng.seed(mSeed);
    } else {
      std::random_device rd;
      rng.seed(rd());
    }

    // Initialize particles
    std::vector<Particle> swarm = initializeSwarm(domain, rng, start);

    // Initialize global best
    VectorT<RealT> globalBestPosition(domain.size());
    RealT globalBestFitness = std::numeric_limits<RealT>::max();

    // Initial evaluation of the swarm
    evaluateSwarm(swarm, func, globalBestPosition, globalBestFitness, mMaxThreads);

    // Variables for convergence checking
    RealT prevBestFitness = globalBestFitness;
    size_t stagnantCount = 0;

    // Main optimization loop
    for (size_t iter = 0; iter < mMaxIters; ++iter) {
      // Update particle velocities and positions
      updateSwarm(swarm, domain, globalBestPosition, rng);

      // Evaluate updated particles
      evaluateSwarm(swarm, func, globalBestPosition, globalBestFitness, mMaxThreads);

      // Log progress if verbose
      if (verbose() && (iter % 10 == 0 || iter == mMaxIters - 1)) {
        SPDLOG_INFO("PSO Iteration {}/{}: Best fitness = {}",
                   iter + 1, mMaxIters, globalBestFitness);
      }

      // Check for convergence after minimum iterations
      if (iter >= mMinItersBeforeConvergence) {
        RealT improvement = std::abs(prevBestFitness - globalBestFitness);
        if (improvement < mTolerance) {
          stagnantCount++;
          if (stagnantCount >= 5) { // Converged if no significant improvement for 5 iterations
            if (verbose()) {
              SPDLOG_INFO("PSO converged after {} iterations", iter + 1);
            }
            break;
          }
        } else {
          stagnantCount = 0;
        }
        prevBestFitness = globalBestFitness;
      }
    }

    // Return the best solution found
    return std::make_tuple(globalBestPosition, globalBestFitness);
  }

  void initParticleSwarmOptimise() {
  }

  namespace {
    [[maybe_unused]] bool g_register = defaultConfigFactory().registerNamedType<Optimise, ParticleSwarmOptimise>("ParticleSwarmOptimise");
  }

} // namespace Ravl2
