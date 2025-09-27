cxxopts usage notes

This document provides practical notes for using cxxopts in this project. It complements the main guidelines.md and shows consistent patterns we use across executables.

Key points
- Always include <cxxopts.hpp> in executables that parse CLI args.
- Prefer long option names with dashes (e.g., --input-dim) to match our conventions.
- Provide sensible defaults via default_value, so binaries run without flags.
- Expose a --help option and return 0 after printing help.
- Catch cxxopts::exceptions::exception when parsing fails and return non‑zero after logging.
- Keep logging initialization in main() or a single entry point to avoid double initialization. Use SPDLOG_* macros for messages.

Basic pattern

#include <iostream>
#include <cxxopts.hpp>

int main(int argc, char** argv) {
  try {
    cxxopts::Options options("my_app", "Short description of the app");
    options.add_options()
      ("input-dim", "Input dimension", cxxopts::value<int>()->default_value("16"))
      ("epochs", "Training epochs", cxxopts::value<int>()->default_value("100"))
      ("no-plot", "Disable plotting", cxxopts::value<bool>()->default_value("false"))
      ("help", "Print usage");

    auto result = options.parse(argc, argv);

    if (result.count("help")) {
      std::cout << options.help() << std::endl;
      return 0;
    }

    int inputDim = result["input-dim"].as<int>();
    int epochs = result["epochs"].as<int>();
    bool noPlot = result["no-plot"].as<bool>();

    // ... program logic ...
  } catch (const cxxopts::exceptions::exception &e) {
    SPDLOG_ERROR("Error parsing options: {}", e.what());
    return 1;
  }
}

Notes on option types
- Numbers: use cxxopts::value<int>(), cxxopts::value<float>(), etc.
- Booleans: define as value<bool>() and set default_value("false") or "true" as needed.
- Strings and paths: use std::string with cxxopts::value<std::string>().

Conventions used in this repository
- Prefer descriptive, kebab‑case option names: --plot-interval, --n-obs-steps, --weight-decay.
- When adding a new executable, ensure a --help option exists and provides a concise description.
- Validate combinations if necessary after parsing (e.g., horizon > 0, batch-size >= 1).
- For fast local runs, choose conservative defaults (short epochs, limited steps) to avoid long runtimes.

Error handling and logging
- Parse inside a try/catch for cxxopts::exceptions::exception.
- Log the error with SPDLOG_ERROR at the boundary where you decide to exit.
- Prefer returning an error code (1) on parse errors.

Examples in the codebase
- src/Reason/DeepLearning/Diffusion/doDiffusePrediction.cc uses cxxopts with defaults, --help handling, and exception safety.
- src/Reason/Agent/doEnkiEnvAgent.cc demonstrates positional help and short aliases (e.g., -c, -f, -e).

Testing quick checklist
- my_app --help prints a usable help message and exits 0.
- my_app runs with no flags, using defaults.
- my_app rejects malformed values with a clear error message and non‑zero exit code.

FAQ
- Boolean flags without explicit value: with value<bool> and default_value("false"), simply passing --flag sets it to true.
- Short options: define as ("c,config", ...) to support -c as an alias for --config.
- Mixing positional args: use options.positional_help() and options.parse_positional({ ... }) if needed.
