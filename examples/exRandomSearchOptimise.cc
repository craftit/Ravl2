#include "Ravl2/Optimise/RandomSearchOptimise.hh"
#include "Ravl2/Optimise/CostDomain.hh"
#include <iostream>
#include <cmath>

using namespace Ravl2;

// Example objective function: Rosenbrock function
float rosenbrock(const VectorT<float> &x) {
  if (x.size() != 2) {
    throw std::invalid_argument("Rosenbrock function requires 2D input");
  }
  float a = 1.0f;
  float b = 100.0f;
  return (a - x[0]) * (a - x[0]) + b * (x[1] - x[0] * x[0]) * (x[1] - x[0] * x[0]);
}

// Example objective function: Sphere function
float sphere(const VectorT<float> &x) {
  float sum = 0.0f;
  for (const auto& xi : x) {
    sum += xi * xi;
  }
  return sum;
}

int main() {
  std::cout << "RandomSearchOptimise Example\n";
  std::cout << "============================\n\n";

  // Define the domain for optimization (2D for Rosenbrock)
  CostDomain<float> domain({-5.0f,-5.0f},{5.0f,5.0F});

  // Test different sampling patterns
  std::vector<RandomSearchOptimise::PatternType> patterns = {
    RandomSearchOptimise::PatternType::Random,
    RandomSearchOptimise::PatternType::Grid,
    RandomSearchOptimise::PatternType::Sobol
  };

  std::vector<std::string> patternNames = {"Random", "Grid", "Sobol"};

  for (size_t i = 0; i < patterns.size(); ++i) {
    std::cout << "Testing " << patternNames[i] << " sampling:\n";
    std::cout << "----------------------------------------\n";

    // Create optimizer with current pattern
    RandomSearchOptimise optimizer(
      100,           // maxEvals
      patterns[i],   // pattern type
      10,            // batchSize
      0,             // maxThreads (single-threaded for this example)
      true,          // verbose
      true,          // fixedSeed
      42,            // seed
      10             // gridPointsPerDim
    );

    // Optimize Rosenbrock function
    auto [bestPoint, bestValue] = optimizer.minimise(domain, rosenbrock);

    std::cout << "Best point: (" << bestPoint[0] << ", " << bestPoint[1] << ")\n";
    std::cout << "Best value: " << bestValue << "\n";
    std::cout << "Distance from true minimum (1,1): "
              << std::sqrt((bestPoint[0] - 1.0f) * (bestPoint[0] - 1.0f) +
                          (bestPoint[1] - 1.0f) * (bestPoint[1] - 1.0f)) << "\n\n";
  }

  // Example with higher-dimensional sphere function
  std::cout << "Testing 5D Sphere function with Random sampling:\n";
  std::cout << "================================================\n";

  CostDomain<float> domain5D({-10.0F,-10.0F,-10.0F,-10.0F,-10.0F}, {10.0F,10.0F,10.0F,10.0F,10.0F});

  RandomSearchOptimise optimizer5D(
    500,                                           // maxEvals
    RandomSearchOptimise::PatternType::Random,     // pattern type
    20,                                            // batchSize
    4,                                             // maxThreads (use parallel evaluation)
    true,                                          // verbose
    true,                                          // fixedSeed
    42                                             // seed
  );

  auto [bestPoint5D, bestValue5D] = optimizer5D.minimise(domain5D, sphere);

  std::cout << "Best point: (";
  for (size_t i = 0; i < bestPoint5D.size(); ++i) {
    std::cout << bestPoint5D[i];
    if (i < bestPoint5D.size() - 1) std::cout << ", ";
  }
  std::cout << ")\n";
  std::cout << "Best value: " << bestValue5D << "\n";

  return 0;
}
