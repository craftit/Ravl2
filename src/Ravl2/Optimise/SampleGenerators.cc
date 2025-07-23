#include "Ravl2/Optimise/SampleGenerators.hh"
#include <spdlog/spdlog.h>
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace Ravl2 {

// RandomSampleGenerator implementation

RandomSampleGenerator::RandomSampleGenerator(bool fixedSeed, unsigned seed)
    : mFixedSeed(fixedSeed), mSeed(seed), mGenerator(fixedSeed ? seed : std::random_device{}()) {
}

RandomSampleGenerator::RandomSampleGenerator(Configuration &config)
    : mFixedSeed(config.getBool("fixedSeed", "Use fixed random seed for reproducibility", true)),
      mSeed(config.getUnsigned("seed", "Random seed value if fixedSeed is true", 42, 0, std::numeric_limits<unsigned>::max())),
      mGenerator(mFixedSeed ? mSeed : std::random_device{}()) {
}

std::vector<VectorT<RandomSampleGenerator::RealT>> RandomSampleGenerator::generatePoints(
    size_t numPoints) {

  if (mDomain.dim() == 0) {
    SPDLOG_ERROR("Domain not set. Call reset(domain) first. ");
    throw std::runtime_error("Domain not set. Call reset(domain) first.");
  }

  std::vector<VectorT<RealT>> points;
  points.reserve(numPoints);

  std::uniform_real_distribution<RealT> dist(0.0, 1.0);

  for (Eigen::Index i = 0; i < Eigen::Index(numPoints); ++i) {
    VectorT<RealT> point(mDomain.size());
    for (Eigen::Index j = 0; j < mDomain.dim(); ++j) {
      RealT u = dist(mGenerator);
      point[j] = mDomain.min(j) + u * (mDomain.max(j) - mDomain.min(j));
    }
    points.push_back(point);
  }

  mPosition += numPoints;
  return points;
}

std::shared_ptr<SampleGenerator> RandomSampleGenerator::clone() const {
  return std::make_shared<RandomSampleGenerator>(mFixedSeed, mSeed);
}

void RandomSampleGenerator::skip(size_t n) {
  if (mDomain.dim() == 0) {
    SPDLOG_ERROR("Domain not set. Call reset(domain) first. ");
    throw std::runtime_error("Domain not set. Call reset(domain) first.");
  }

  // For random generators, we need to advance the generator state
  // Generate and discard random values for each dimension for n points
  std::uniform_real_distribution<RealT> dist(0.0, 1.0);
  for (size_t i = 0; i < n; ++i) {
    for (Eigen::Index j = 0; j < mDomain.dim(); ++j) {
      dist(mGenerator); // Advance the generator by consuming random values
    }
  }
  mPosition += n;
}

// GridSampleGenerator implementation

GridSampleGenerator::GridSampleGenerator(size_t pointsPerDim)
    : mPointsPerDim(pointsPerDim) {
  if (pointsPerDim < 2) {
    SPDLOG_ERROR("Invalid parameter: pointsPerDim must be at least 2, got {}", pointsPerDim);
    throw std::invalid_argument("pointsPerDim must be at least 2");
  }
}

GridSampleGenerator::GridSampleGenerator(Configuration &config)
    : mPointsPerDim(config.getUnsigned("pointsPerDim", "Number of points per dimension for grid sampling", 10, 2, 1000)) {
}

std::vector<VectorT<GridSampleGenerator::RealT>> GridSampleGenerator::generatePoints(
    size_t numPoints) {

  if (mDomain.dim() == 0) {
    SPDLOG_ERROR("Domain not set. Call reset(domain) first.");
    throw std::runtime_error("Domain not set. Call reset(domain) first.");
  }

  Eigen::Index dims = mDomain.dim();

  // Calculate points per dimension to get approximately numPoints total
  Eigen::Index pointsPerDim = static_cast<Eigen::Index>(std::round(std::pow(numPoints, 1.0 / dims)));
  pointsPerDim = std::max(pointsPerDim, Eigen::Index(2)); // At least 2 points per dimension

  std::vector<VectorT<RealT>> points;
  points.reserve(numPoints);

  // Generate points starting from current position
  size_t totalGridPoints = static_cast<size_t>(std::pow(pointsPerDim, dims));

  for (size_t pointIdx = 0; pointIdx < numPoints; ++pointIdx) {
    size_t currentIndex = (mPosition + pointIdx) % totalGridPoints;
    VectorT<RealT> point(dims);

    // Convert linear index to multi-dimensional coordinates
    Eigen::Index temp = static_cast<Eigen::Index>(currentIndex);
    for (Eigen::Index dim = 0; dim < dims; ++dim) {
      Eigen::Index coord = temp % pointsPerDim;
      temp /= pointsPerDim;

      RealT t = (pointsPerDim == 1) ? 0.5 : static_cast<RealT>(coord) / (pointsPerDim - 1);
      point[dim] = mDomain.min(dim) + t * (mDomain.max(dim) - mDomain.min(dim));
    }

    points.push_back(point);
  }

  mPosition += numPoints;
  return points;
}

std::shared_ptr<SampleGenerator> GridSampleGenerator::clone() const {
  return std::make_shared<GridSampleGenerator>(mPointsPerDim);
}

// SobolSampleGenerator implementation

SobolSampleGenerator::SobolSampleGenerator(Configuration &config) {
  // Sobol generator currently has no configurable parameters,
  // but we include the constructor for consistency and future extensibility
  (void)config; // Suppress unused parameter warning
}

std::vector<VectorT<SobolSampleGenerator::RealT>> SobolSampleGenerator::generatePoints(
    size_t numPoints) {

  if (mDomain.dim() == 0) {
    throw std::runtime_error("Domain not set. Call reset(domain) first.");
  }

  std::vector<VectorT<RealT>> points;
  points.reserve(numPoints);

  // Use different prime bases for each dimension
  std::vector<size_t> bases = {2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41, 43, 47};

  // Generate points starting from current position
  for (size_t i = 0; i < numPoints; ++i) {
    VectorT<RealT> point(mDomain.size());
    for (Eigen::Index j = 0; j < mDomain.dim(); ++j) {
      size_t base = bases[static_cast<size_t>(j) % bases.size()];
      // Use mPosition + i + 1 to continue the sequence from the current position
      double u = vanDerCorput(mPosition + i + 1, base);
      point[j] = mDomain.min(j) + static_cast<RealT>(u) * (mDomain.max(j) - mDomain.min(j));
    }
    points.push_back(point);
  }

  mPosition += numPoints;
  return points;
}

std::shared_ptr<SampleGenerator> SobolSampleGenerator::clone() const {
  return std::make_shared<SobolSampleGenerator>();
}

double SobolSampleGenerator::vanDerCorput(size_t n, size_t base) {
  double result = 0.0;
  double f = 1.0 / base;
  while (n > 0) {
    result += f * (n % base);
    n /= base;
    f /= base;
  }
  return result;
}

// AcquisitionSampleGenerator implementation

AcquisitionSampleGenerator::AcquisitionSampleGenerator(
    RealT explorationWeight, RealT localSearchRadius, size_t numLocalCandidates,
    size_t candidateMultiplier, bool fixedSeed, unsigned seed)
    : mExplorationWeight(explorationWeight), mLocalSearchRadius(localSearchRadius),
      mNumLocalCandidates(numLocalCandidates), mCandidateMultiplier(candidateMultiplier),
      mFixedSeed(fixedSeed), mSeed(seed), mRandomGen(fixedSeed ? seed : std::random_device{}()) {
}

AcquisitionSampleGenerator::AcquisitionSampleGenerator(Configuration &config)
    : mExplorationWeight(static_cast<RealT>(config.getNumber("explorationWeight",
        "Weight for exploration vs exploitation", 0.1, 0.0, 1.0))),
      mLocalSearchRadius(static_cast<RealT>(config.getNumber("localSearchRadius",
        "Radius for local search around best points", 0.1, 0.01, 0.5))),
      mNumLocalCandidates(config.getUnsigned("numLocalCandidates",
        "Number of candidates around best points (0=auto)", 0, 0, 1000)),
      mCandidateMultiplier(config.getUnsigned("candidateMultiplier",
        "Multiplier for number of candidates vs requested points", 100, 10, 1000)),
      mFixedSeed(config.getBool("fixedSeed", "Use fixed random seed", true)),
      mSeed(config.getUnsigned("seed", "Random seed value", 42, 0, std::numeric_limits<unsigned>::max())),
      mRandomGen(mFixedSeed ? mSeed : std::random_device{}()) {
}

std::vector<VectorT<AcquisitionSampleGenerator::RealT>> AcquisitionSampleGenerator::generatePoints(
    size_t numPoints) {

  if (mDomain.dim() == 0) {
    throw std::runtime_error("Domain not set. Call reset(domain) first.");
  }

  if (!mGP || mX.empty() || mY.empty()) {
    // Fallback to Sobol sampling if no GP data available
    mSobolGen.reset(mDomain);
    return mSobolGen.generatePoints(numPoints);
  }

  // Generate candidate points
  size_t numCandidates = mCandidateMultiplier * numPoints;
  auto candidates = generateCandidates(numCandidates);

  if (candidates.empty()) {
    // Fallback to Sobol sampling
    mSobolGen.reset(mDomain);
    return mSobolGen.generatePoints(numPoints);
  }

  // Find best current value
  RealT bestY = *std::min_element(mY.begin(), mY.end());

  // Compute Expected Improvement for all candidates
  auto eiScores = computeExpectedImprovement(candidates, bestY);

  // Select top candidates
  std::vector<VectorT<RealT>> selected;
  selected.reserve(numPoints);

  size_t maxSelect = std::min(numPoints, eiScores.size());
  for (size_t i = 0; i < maxSelect; ++i) {
    selected.push_back(candidates[eiScores[i].second]);
  }

  mPosition += numPoints;
  return selected;
}

std::shared_ptr<SampleGenerator> AcquisitionSampleGenerator::clone() const {
  return std::make_shared<AcquisitionSampleGenerator>(
    mExplorationWeight, mLocalSearchRadius, mNumLocalCandidates,
    mCandidateMultiplier, mFixedSeed, mSeed);
}

void AcquisitionSampleGenerator::setEvaluationData(const std::vector<VectorT<RealT>>& X,
                                                   const std::vector<RealT>& Y,
                                                   const GaussianProcess<RealT>* gp) {
  mX = X;
  mY = Y;
  mGP = gp;
}

std::vector<VectorT<AcquisitionSampleGenerator::RealT>> AcquisitionSampleGenerator::generateCandidates(
    size_t numCandidates) {

  std::vector<VectorT<RealT>> candidates;
  candidates.reserve(numCandidates);

  // Strategy 1: Sobol sequence for global exploration (60% of candidates)
  size_t globalCandidates = static_cast<size_t>(0.6 * numCandidates);
  mSobolGen.reset(mDomain);
  auto globalPoints = mSobolGen.generatePoints(globalCandidates);
  candidates.insert(candidates.end(), globalPoints.begin(), globalPoints.end());

  // Strategy 2: Random exploration with higher variance (25% of candidates)
  size_t randomCandidates = static_cast<size_t>(0.25 * numCandidates);
  std::uniform_real_distribution<RealT> uniformDist(0.0, 1.0);
  for (size_t i = 0; i < randomCandidates; ++i) {
    VectorT<RealT> point(mDomain.dim());
    for (Eigen::Index d = 0; d < mDomain.dim(); ++d) {
      RealT u = uniformDist(mRandomGen);
      point[d] = mDomain.min(d) + u * (mDomain.max(d) - mDomain.min(d));
    }
    candidates.push_back(point);
  }

  // Strategy 3: Local search around the best points (15% of candidates)
  if (!mY.empty()) {
    size_t localCandidates = numCandidates - globalCandidates - randomCandidates;
    size_t numLocalActual = mNumLocalCandidates > 0 ? mNumLocalCandidates : localCandidates;
    numLocalActual = std::min(numLocalActual, localCandidates);

    // Find best points
    std::vector<std::pair<RealT, size_t>> sortedY;
    for (size_t i = 0; i < mY.size(); ++i) {
      sortedY.emplace_back(mY[i], i);
    }
    std::sort(sortedY.begin(), sortedY.end());

    // Generate candidates around top points
    size_t numBestPoints = std::min(static_cast<size_t>(3), mY.size());
    std::normal_distribution<RealT> normalDist(0.0, 1.0);

    for (size_t i = 0; i < numLocalActual; ++i) {
      size_t bestIdx = sortedY[i % numBestPoints].second;
      const auto& bestPoint = mX[bestIdx];

      VectorT<RealT> localPoint(mDomain.dim());
      for (Eigen::Index d = 0; d < mDomain.dim(); ++d) {
        RealT domainRange = mDomain.max(d) - mDomain.min(d);
        RealT noise = normalDist(mRandomGen) * mLocalSearchRadius * domainRange;
        localPoint[d] = std::max(mDomain.min(d),
                        std::min(mDomain.max(d), bestPoint[d] + noise));
      }
      candidates.push_back(localPoint);
    }
  }

  return candidates;
}

std::vector<std::pair<AcquisitionSampleGenerator::RealT, size_t>>
AcquisitionSampleGenerator::computeExpectedImprovement(
    const std::vector<VectorT<RealT>>& candidates, RealT bestY) {

  if (!mGP || candidates.empty()) {
    return {};
  }

  const size_t nCandidates = candidates.size();
  const auto nDims = candidates[0].size();

  // Convert candidates to matrix format
  typename GaussianProcess<RealT>::Matrix Xtest(static_cast<Eigen::Index>(nCandidates), nDims);
  for (size_t i = 0; i < nCandidates; ++i) {
    Xtest.row(static_cast<Eigen::Index>(i)) = candidates[i].transpose();
  }

  // Predict with GP
  VectorT<RealT> mean, variance;
  mGP->predict(Xtest, mean, variance);

  // Compute Expected Improvement
  std::vector<std::pair<RealT, size_t>> eiScores;
  eiScores.reserve(nCandidates);

  for (size_t i = 0; i < nCandidates; ++i) {
    const auto mu = mean[static_cast<Eigen::Index>(i)];
    const auto sigma = std::sqrt(std::max(variance[static_cast<Eigen::Index>(i)], static_cast<RealT>(1e-9)));

    if (sigma > 0) {
      const auto z = static_cast<double>((bestY - mu) / sigma);
      const auto ei = static_cast<double>(bestY - mu) * std::erfc(-z/std::sqrt(2))/2 +
                     static_cast<double>(sigma) * std::exp(-0.5*z*z)/std::sqrt(2*M_PI);
      eiScores.emplace_back(static_cast<RealT>(ei), i);
    } else {
      eiScores.emplace_back(0.0, i);
    }
  }

  // Sort by EI score (descending)
  std::sort(eiScores.rbegin(), eiScores.rend());

  return eiScores;
}

// Component registration
namespace {
  [[maybe_unused]] static auto randomGeneratorReg =
    defaultConfigFactory().registerNamedType<SampleGenerator, RandomSampleGenerator>("Random");

  [[maybe_unused]] static auto gridGeneratorReg =
    defaultConfigFactory().registerNamedType<SampleGenerator, GridSampleGenerator>("Grid");

  [[maybe_unused]] static auto sobolGeneratorReg =
    defaultConfigFactory().registerNamedType<SampleGenerator, SobolSampleGenerator>("Sobol");

  [[maybe_unused]] static auto acquisitionGeneratorReg =
    defaultConfigFactory().registerNamedType<SampleGenerator, AcquisitionSampleGenerator>("Acquisition");
}

} // namespace Ravl2
